/*
 * Copyright (C) 2013 Intel Corporation.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authors:
 *   José Bollo <jose.bollo@open.eurogiciel.org>
 *   Stéphane Desneux <stephane.desneux@open.eurogiciel.org>
 *   Jean-Benoit Martin <jean-benoit.martin@open.eurogiciel.org>
 *
 */
#define _GNU_SOURCE

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <stdarg.h>
#include <alloca.h>
#include <pwd.h>

#ifndef NOT_MULTI_THREAD_SAFE
#include <pthread.h>
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static inline void lock() { pthread_mutex_lock(&mutex); }
static inline void unlock() { pthread_mutex_unlock(&mutex); }
#else
static inline void lock() { }
static inline void unlock() { }
#endif

#ifndef CONFIGPATH
#define CONFIGPATH "/etc/tizen-platform.conf"
#endif

#include "tzplatform_variables.h"
#include "tzplatform_config.h"
#include "parser.h"
#include "heap.h"
#include "buffer.h"
#include "foreign.h"
#include "scratch.h"
#include "passwd.h"

#include "isadmin.h"

#define _HAS_IDS_   (  _FOREIGN_HAS_(UID)  \
                    || _FOREIGN_HAS_(EUID) \
                    || _FOREIGN_HAS_(GID)  )

#define _HAS_PWS_   (  _FOREIGN_HAS_(HOME)  \
                    || _FOREIGN_HAS_(USER)  \
                    || _FOREIGN_HAS_(EHOME) \
                    || _FOREIGN_HAS_(EUSER) )

/* structure for reading config files */
struct reading {
    int errcount;
    size_t dynvars[_FOREIGN_COUNT_];
    size_t offsets[_TZPLATFORM_VARIABLES_COUNT_];
};

/* local and static variables */
static const char metafilepath[] = CONFIGPATH;
static const char emptystring[] = "";
static struct heap heap;
static enum { RESET=0, ERROR, VALID } state = RESET;
static const char *var_values[_TZPLATFORM_VARIABLES_COUNT_];
static const char *var_names[_TZPLATFORM_VARIABLES_COUNT_];

#include "hash.inc"

/* write the error message */
static void writerror( const char *format, ...)
{
    va_list ap;
    fprintf( stderr, "tzplatform_config ERROR: ");
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    fprintf( stderr, "\n");
}

#if _HAS_IDS_
/* fill the foreign variables for ids */
static void foreignid( struct reading *reading)
{
    int n;
    char buffer[50];

#if _FOREIGN_HAS_(UID)
    /* set the uid */
    if (reading->dynvars[UID] == HNULL) {
        n = snprintf( buffer, sizeof buffer, "%d", (int)getuid());
        if (0 < n && n < (int)(sizeof buffer))
            reading->dynvars[UID] = heap_strndup( &heap, buffer, (size_t)n);
    }
#endif

#if _FOREIGN_HAS_(EUID)
    /* set the euid */
    if (reading->dynvars[EUID] == HNULL) {
        n = snprintf( buffer, sizeof buffer, "%d", (int)geteuid());
        if (0 < n && n < (int)(sizeof buffer))
            reading->dynvars[EUID] = heap_strndup( &heap, buffer, (size_t)n);
    }
#endif

#if _FOREIGN_HAS_(GID)
    /* set the gid */
    if (reading->dynvars[GID] == HNULL) {
        n = snprintf( buffer, sizeof buffer, "%d", (int)getgid());
        if (0 < n && n < (int)(sizeof buffer))
            reading->dynvars[GID] = heap_strndup( &heap, buffer, (size_t)n);
    }
#endif
}
#endif

#if _HAS_PWS_
/* fill the foreign variables for home and user */
static void foreignpw( struct reading *reading)
{
    int n = 0;
    struct pwget *array[3];
#if _FOREIGN_HAS_(HOME) || _FOREIGN_HAS_(USER)
    struct pwget uid;
    char suid[50];
#endif
#if _FOREIGN_HAS_(EHOME) || _FOREIGN_HAS_(EUSER)
    struct pwget euid;
    char seuid[50];
#endif

#if _FOREIGN_HAS_(HOME) || _FOREIGN_HAS_(USER)
    if (
#if _FOREIGN_HAS_(HOME)
        reading->dynvars[HOME] == HNULL
#endif
#if _FOREIGN_HAS_(HOME) && _FOREIGN_HAS_(USER)
        ||
#endif
#if _FOREIGN_HAS_(USER)
        reading->dynvars[USER] == HNULL
#endif
    ) {
        snprintf( suid, sizeof suid, "%u", (unsigned)getuid());
        uid.id = suid;
        array[n++] = &uid;
    }
    else {
        uid.set = 0;
    }
#endif

#if _FOREIGN_HAS_(EHOME) || _FOREIGN_HAS_(EUSER)
    if (
#if _FOREIGN_HAS_(EHOME)
        reading->dynvars[EHOME] == HNULL
#endif
#if _FOREIGN_HAS_(EHOME) && _FOREIGN_HAS_(USER)
        ||
#endif
#if _FOREIGN_HAS_(EUSER)
        reading->dynvars[EUSER] == HNULL
#endif
    ) {
        snprintf( seuid, sizeof seuid, "%u", (unsigned)geteuid());
        euid.id = seuid;
        array[n++] = &euid;
    }
    else {
        euid.set = 0;
    }
#endif

    if (n) {
        array[n] = NULL;
        if (pw_get( &heap, array) == 0) {
#if _FOREIGN_HAS_(HOME)
            if (uid.set)
                reading->dynvars[HOME] = uid.home;
#endif
#if _FOREIGN_HAS_(USER)
            if (uid.set)
                reading->dynvars[USER] = uid.user;
#endif
#if _FOREIGN_HAS_(EHOME)
            if (euid.set)
                reading->dynvars[EHOME] = euid.home;
#endif
#if _FOREIGN_HAS_(EUSER)
            if (euid.set)
                reading->dynvars[EUSER] = euid.user;
#endif
        }
    }
}
#endif

/* get the foreign variable */
static const char *foreignvar( struct reading *reading, 
                                            const char *name, size_t length)
{
    enum fkey key = foreign( name, length);
    size_t offset;

    switch (key) {
#if _HAS_PWS_
#if _FOREIGN_HAS_(HOME)
    case HOME:
#endif
#if _FOREIGN_HAS_(USER)
    case USER:
#endif
#if _FOREIGN_HAS_(EHOME)
    case EHOME:
#endif
#if _FOREIGN_HAS_(EUSER)
    case EUSER:
#endif
        foreignpw( reading);
        break;
#endif
 
#if _HAS_IDS_
#if _FOREIGN_HAS_(UID)
    case UID:
#endif
#if _FOREIGN_HAS_(GID)
    case GID:
#endif
#if _FOREIGN_HAS_(EUID)
    case EUID:
#endif
        foreignid( reading);
        break;
#endif

    default:
        return NULL;
    }
    offset = reading->dynvars[key];
    return offset==HNULL ? NULL : heap_address( &heap, offset);
}

/* callback for parsing errors */
static int errcb( struct parsing *parsing, 
            size_t position, const char *message)
{
    struct parsinfo info;
    struct reading *reading = parsing->data;

    /* count the error */
    reading->errcount++;

    /* print the error */
    parse_utf8_info( parsing, &info, position);
    writerror( "%s (file %s line %d)", message, metafilepath, info.lino);

    /* continue to parse */
    return 0;
}

/* callback for solving variables */
static const char *getcb( struct parsing *parsing, 
            const char *key, size_t length,
            size_t begin_pos, size_t end_pos)
{
    struct parsinfo info;
    const char *result;
    const struct varassoc *vara;
    size_t offset;
    struct reading *reading = parsing->data;

    /* try to find a tzplatform variable */
    vara = hashvar( key, length);
    if (vara) {
        /* found: try to use it */
        offset = reading->offsets[(int)vara->id];
        if (offset != HNULL)
            result = heap_address( &heap, offset);
        else 
            result = NULL;
    }
    else {
        /* that is a foreign variable */
        result = foreignvar( reading, key, length);
    }

    /* emit the error and then return */
    if (result == NULL) {
        reading->errcount++;
        parse_utf8_info( parsing, &info, begin_pos);
        writerror( "undefined value for %.*s (file %s line %d)",
            length, key, metafilepath, info.lino);
        result = emptystring; /* avoid error of the parser */
    }
    return result;
}

/* callback to define variables */
static int putcb( struct parsing *parsing, 
            const char *key, size_t key_length, 
            const char *value, size_t value_length,
            size_t begin_pos, size_t end_pos)
{
    struct parsinfo info;
    const struct varassoc *vara;
    size_t offset;
    char *string;
    struct reading *reading = parsing->data;

    /* try to find a tzplatform variable */
    vara = hashvar( key, key_length);
    if (vara) {
        /* check that the variable isn't already defined */
        offset = reading->offsets[(int)vara->id];
        if (offset != HNULL) {
            reading->errcount++;
            parse_utf8_info( parsing, &info, begin_pos);
            writerror( "redefinition of variable %.*s (file %s line %d)",
                    key_length, key, metafilepath, info.lino);
        }

        /* allocate the variable value */
        offset = heap_alloc( &heap, value_length+1);
        if (offset == HNULL) {
            /* error of allocation */
            reading->errcount++;
            writerror( "out of memory");
        }
        else {
            /* record the variable value */
            reading->offsets[(int)vara->id] = offset;
            string = heap_address( &heap, offset);
            memcpy( string, value, value_length);
            string[value_length] = 0;
        }
    }
    else {
        /* forbidden variable */
        parse_utf8_info( parsing, &info, begin_pos);
        writerror( "forbidden variable name %.*s (file %s line %d)",
            key_length, key, metafilepath, info.lino);
        
    }

    /* continue to parse */
    return 0;
}

/* initialize the environment */
static void initialize()
{
    struct buffer buffer;
    struct parsing parsing;
    struct reading reading;
    size_t offset;
    int i, result;

    /* clear the variables */
    reading.errcount = 0;
    for (i = 0 ; i < (int)_FOREIGN_COUNT_ ; i++) {
        reading.dynvars[i] = HNULL;
    }
    for (i = 0 ; i < (int)_TZPLATFORM_VARIABLES_COUNT_ ; i++) {
        var_values[i] = NULL;
        reading.offsets[i] = HNULL;
    }

    /* read the configuration file */
    result = buffer_create( &buffer, metafilepath);
    if (result != 0) {
        writerror( "can't read file %s",metafilepath);
        state = ERROR;
        return;
    }

    /* create the heap */
    result = heap_create( &heap, 1);
    if (result != 0) {
        buffer_destroy( &buffer);
        writerror( "out of memory");
        state = ERROR;
        return;
    }

    /* read the file */
    parsing.buffer = buffer.buffer;
    parsing.length = buffer.length;
    parsing.maximum_data_size = 0;
    parsing.should_escape = 0;
    parsing.data = &reading;
    parsing.get = getcb;
    parsing.put = putcb;
    parsing.error = errcb;
    result = parse_utf8_config( &parsing);
    buffer_destroy( &buffer);
    if (result != 0 || reading.errcount != 0) {
        writerror( "%d errors while parsing file %s",
                                            reading.errcount, metafilepath);
    }

    /* set the variables */
    heap_read_only( &heap);
    for (i = 0 ; i < (int)_TZPLATFORM_VARIABLES_COUNT_ ; i++) {
        offset = reading.offsets[i];
        if (offset != HNULL)
            var_values[i] = heap_address( &heap, offset);
        else
            writerror( "the variable %s isn't defined in file %s",
                tzplatform_getname((enum tzplatform_variable)i), metafilepath);
            /* TODO undefined variable */;
    }
    state = VALID;
}

static const char *get_lock(enum tzplatform_variable id)
{
    const char *result;
    int offset;

    lock();
    offset = (int)id;
    if (offset < 0 || (int)_TZPLATFORM_VARIABLES_COUNT_ <= offset) {
        /*error("invalid id"); TODO*/
        result = NULL;
    }
    else {
        if (state == RESET)
            initialize();
        result = var_values[offset];
    }
    return result;
}

/* reset the state of the environement */
void tzplatform_reset()
{
    lock();
    if (state != RESET) {
        if (state == VALID)
            heap_destroy( &heap);
        state = RESET;
    }
    unlock();
}

int tzplatform_getcount()
{
    return (int)_TZPLATFORM_VARIABLES_COUNT_;
}

const char* tzplatform_getname(enum tzplatform_variable id)
{
    const struct varassoc *iter, *end;
    const char *result;
    int offset;

    offset = (int)id;
    if (offset < 0 || (int)_TZPLATFORM_VARIABLES_COUNT_ <= offset) {
        /*error("invalid id"); TODO*/
        result = NULL;
    }
    else {
        if (!var_names[0]) {
            iter = namassoc;
            end = iter + (sizeof namassoc / sizeof namassoc[0]);
            while (iter != end) {
                if (iter->offset >= 0) 
                    var_names[(int)iter->id] = varpool + iter->offset;
                iter++;
            }
        }
        result = var_names[offset];
    }
    return result;
}

enum tzplatform_variable tzplatform_getid(const char *name)
{
    const struct varassoc *vara = hashvar( name, strlen(name));
    return vara ? vara->id : _TZPLATFORM_VARIABLES_INVALID_;
}

const char* tzplatform_getenv(enum tzplatform_variable id) 
{
    const char *result = get_lock( id);
    unlock();
    return result;
}

int tzplatform_getenv_int(enum tzplatform_variable id)
{
    const char *value = get_lock( id);
    int result = value==NULL ? -1 : atoi(value);
    unlock();
    return result;
}

const char* tzplatform_mkstr(enum tzplatform_variable id, const char * str)
{
    const char *array[3];
    const char *result = get_lock( id);
    if (result != NULL) {
        array[0] = result;
        array[1] = str;
        array[2] = NULL;
        result = scratchcat( 0, array);
    }
    unlock();
    return result;
}

const char* tzplatform_mkpath(enum tzplatform_variable id, const char * path)
{
    const char *array[3];
    const char *result = get_lock( id);
    if (result != NULL) {
        array[0] = result;
        array[1] = path;
        array[2] = NULL;
        result = scratchcat( 1, array);
    }
    unlock();
    return result;
}

const char* tzplatform_mkpath3(enum tzplatform_variable id, const char * path,
                                                        const char* path2)
{
    const char *array[4];
    const char *result = get_lock( id);
    if (result != NULL) {
        array[0] = result;
        array[1] = path;
        array[2] = path2;
        array[3] = NULL;
        result = scratchcat( 1, array);
    }
    unlock();
    return result;
}

const char* tzplatform_mkpath4(enum tzplatform_variable id, const char * path,
                                          const char* path2, const char *path3)
{
    const char *array[5];
    const char *result = get_lock( id);
    if (result != NULL) {
        array[0] = result;
        array[1] = path;
        array[2] = path2;
        array[3] = path3;
        array[4] = NULL;
        result = scratchcat( 1, array);
    }
    unlock();
    return result;
}

uid_t tzplatform_getuid(enum tzplatform_variable id)
{
    uid_t result = (uid_t)-1;
    const char *value = get_lock( id);
    if (value != NULL) {
        pw_get_uid( value, &result);
    }
    unlock();
    return result;
}

gid_t tzplatform_getgid(enum tzplatform_variable id)
{
    gid_t result = (uid_t)-1;
    const char *value = get_lock( id);
    if (value != NULL) {
        pw_get_gid( value, &result);
    }
    unlock();
    return result;
}

char tzplatform_isadmin(int uid)
{
	return is_admin(uid);
}

#ifdef TEST
int main() {
    int i = 0;
    while(i != (int)_TZPLATFORM_VARIABLES_COUNT_) {
        enum tzplatform_variable id = (enum tzplatform_variable)i;
        const char*name = tzplatform_getname(id);
        const char*value = tzplatform_getenv(id);
        int xid = (int)tzplatform_getid(name);
        printf("%d=%d\t%s=%s\n",i,xid,name,value);
        i++;
    }
    return 0;
}
#endif
