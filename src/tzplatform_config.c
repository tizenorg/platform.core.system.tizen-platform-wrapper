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

#define _USER_NOT_SET_  ((uid_t)-1)

/* local and static variables */
static const char metafilepath[] = CONFIGPATH;
static const char emptystring[] = "";
static const char *var_names[_TZPLATFORM_VARIABLES_COUNT_];

enum STATE { RESET=0, ERROR, VALID };

struct tzplatform_context {
#ifndef NOT_MULTI_THREAD_SAFE
    pthread_mutex_t mutex;
#endif
    enum STATE state;
    uid_t user;
    struct heap heap;
    const char *values[_TZPLATFORM_VARIABLES_COUNT_];
};

/* structure for reading config files */
struct reading {
    int errcount;
    struct tzplatform_context *context;
    size_t dynvars[_FOREIGN_COUNT_];
    size_t offsets[_TZPLATFORM_VARIABLES_COUNT_];
};

static struct tzplatform_context global_context = {
#ifndef NOT_MULTI_THREAD_SAFE
    .mutex = PTHREAD_MUTEX_INITIALIZER,
#endif
    .state = RESET,
    .user = _USER_NOT_SET_
};

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

static uid_t get_uid(struct tzplatform_context *context)
{
    uid_t result;

    result = context->user;
    if (result == _USER_NOT_SET_)
        result = getuid();

    return result;
}

#if _FOREIGN_HAS_(EUID)
static uid_t get_euid(struct tzplatform_context *context)
{
    uid_t result;

    result = context->user;
    if (result == _USER_NOT_SET_)
        result = geteuid();

    return result;
}
#endif

#if _FOREIGN_HAS_(GID)
static gid_t get_gid(struct tzplatform_context *context)
{
    return getgid();
}
#endif

#if _HAS_IDS_
/* fill the foreign variables for ids */
static void foreignid( struct reading *reading)
{
    int n;
    char buffer[50];

#if _FOREIGN_HAS_(UID)
    /* set the uid */
    if (reading->dynvars[UID] == HNULL) {
        n = snprintf( buffer, sizeof buffer, "%d", (int)get_uid(reading->context));
        if (0 < n && n < (int)(sizeof buffer))
            reading->dynvars[UID] = heap_strndup( &reading->context->heap, buffer, (size_t)n);
    }
#endif

#if _FOREIGN_HAS_(EUID)
    /* set the euid */
    if (reading->dynvars[EUID] == HNULL) {
        n = snprintf( buffer, sizeof buffer, "%d", (int)get_euid(reading->context));
        if (0 < n && n < (int)(sizeof buffer))
            reading->dynvars[EUID] = heap_strndup( &reading->context->heap, buffer, (size_t)n);
    }
#endif

#if _FOREIGN_HAS_(GID)
    /* set the gid */
    if (reading->dynvars[GID] == HNULL) {
        n = snprintf( buffer, sizeof buffer, "%d", (int)get_gid(reading->context));
        if (0 < n && n < (int)(sizeof buffer))
            reading->dynvars[GID] = heap_strndup( &reading->context->heap, buffer, (size_t)n);
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
        snprintf( suid, sizeof suid, "%u", (unsigned)get_uid(reading->context));
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
        snprintf( seuid, sizeof seuid, "%u", (unsigned)get_euid(reading->context));
        euid.id = seuid;
        array[n++] = &euid;
    }
    else {
        euid.set = 0;
    }
#endif

    if (n) {
        array[n] = NULL;
        if (pw_get( &reading->context->heap, array) == 0) {
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
    return offset==HNULL ? NULL : heap_address( &reading->context->heap, offset);
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
            result = heap_address( &reading->context->heap, offset);
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
        offset = heap_alloc( &reading->context->heap, value_length+1);
        if (offset == HNULL) {
            /* error of allocation */
            reading->errcount++;
            writerror( "out of memory");
        }
        else {
            /* record the variable value */
            reading->offsets[(int)vara->id] = offset;
            string = heap_address( &reading->context->heap, offset);
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
static void initialize(struct tzplatform_context *context)
{
    struct buffer buffer;
    struct parsing parsing;
    struct reading reading;
    size_t offset;
    int i, result;

    /* clear the variables */
    reading.errcount = 0;
    reading.context = context;
    for (i = 0 ; i < (int)_FOREIGN_COUNT_ ; i++) {
        reading.dynvars[i] = HNULL;
    }
    for (i = 0 ; i < (int)_TZPLATFORM_VARIABLES_COUNT_ ; i++) {
        context->values[i] = NULL;
        reading.offsets[i] = HNULL;
    }

    /* read the configuration file */
    result = buffer_create( &buffer, metafilepath);
    if (result != 0) {
        writerror( "can't read file %s",metafilepath);
        context->state = ERROR;
        return;
    }

    /* create the heap */
    result = heap_create( &context->heap, 1);
    if (result != 0) {
        buffer_destroy( &buffer);
        writerror( "out of memory");
        context->state = ERROR;
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
    heap_read_only( &context->heap);
    for (i = 0 ; i < (int)_TZPLATFORM_VARIABLES_COUNT_ ; i++) {
        offset = reading.offsets[i];
        if (offset != HNULL)
            context->values[i] = heap_address( &context->heap, offset);
        else
            writerror( "the variable %s isn't defined in file %s",
                tzplatform_getname((enum tzplatform_variable)i), metafilepath);
            /* TODO undefined variable */;
    }
    context->state = VALID;
}

inline static void lock(struct tzplatform_context *context)
{
#ifndef NOT_MULTI_THREAD_SAFE
    pthread_mutex_lock( &context->mutex);
#endif
}

inline static void unlock(struct tzplatform_context *context)
{
#ifndef NOT_MULTI_THREAD_SAFE
    pthread_mutex_unlock( &context->mutex);
#endif
}

static const char *get_lock(struct tzplatform_context *context, enum tzplatform_variable id)
{
    const char *result;
    int offset;

    lock( context);
    offset = (int)id;
    if (offset < 0 || (int)_TZPLATFORM_VARIABLES_COUNT_ <= offset) {
        /*error("invalid id"); TODO*/
        result = NULL;
    }
    else {
        if (context->state == RESET)
            initialize( context);
        result = context->state == ERROR ? NULL : context->values[offset];
    }
    return result;
}

int tzplatform_context_create(struct tzplatform_context **result)
{
    struct tzplatform_context *context;

    context = malloc( sizeof * context);
    *result = context;
    if (context == NULL)
        return -1;

    context->state = RESET;
    context->user = _USER_NOT_SET_;
#ifndef NOT_MULTI_THREAD_SAFE
    pthread_mutex_init( &context->mutex, NULL);
#endif
    return 0;
}

void tzplatform_context_destroy(struct tzplatform_context *context)
{
    if (context->state == VALID)
            heap_destroy( &context->heap);
    context->state = ERROR;
    free( context);
}

void tzplatform_reset()
{
    tzplatform_context_reset( &global_context);
}

void tzplatform_context_reset(struct tzplatform_context *context)
{
    lock( context);
    if (context->state != RESET) {
        if (context->state == VALID)
            heap_destroy( &context->heap);
        context->state = RESET;
    }
    unlock( context);
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
    return tzplatform_context_getenv( &global_context, id);
}

const char* tzplatform_context_getenv(struct tzplatform_context *context, enum tzplatform_variable id)
{
    const char *array[2];
    const char *result = get_lock( context, id);
    if (result != NULL) {
        array[0] = result;
        array[1] = NULL;
        result = scratchcat( 0, array);
    }
    unlock( context);
    return result;
}

int tzplatform_getenv_int(enum tzplatform_variable id)
{
    return tzplatform_context_getenv_int( &global_context, id);
}

int tzplatform_context_getenv_int(struct tzplatform_context *context, enum tzplatform_variable id)
{
    const char *value = get_lock( context, id);
    int result = value==NULL ? -1 : atoi(value);
    unlock( context);
    return result;
}

const char* tzplatform_mkstr(enum tzplatform_variable id, const char * str)
{
    return tzplatform_context_mkstr( &global_context, id, str);
}

const char* tzplatform_context_mkstr(struct tzplatform_context *context, enum tzplatform_variable id, const char *str)
{
    const char *array[3];
    const char *result = get_lock( context, id);
    if (result != NULL) {
        array[0] = result;
        array[1] = str;
        array[2] = NULL;
        result = scratchcat( 0, array);
    }
    unlock( context);
    return result;
}

const char* tzplatform_mkpath(enum tzplatform_variable id, const char * path)
{
    return tzplatform_context_mkpath( &global_context, id, path);
}

const char* tzplatform_context_mkpath(struct tzplatform_context *context, enum tzplatform_variable id, const char *path)
{
    const char *array[3];
    const char *result = get_lock( context, id);
    if (result != NULL) {
        array[0] = result;
        array[1] = path;
        array[2] = NULL;
        result = scratchcat( 1, array);
    }
    unlock( context);
    return result;
}

const char* tzplatform_mkpath3(enum tzplatform_variable id, const char * path,
                                                        const char* path2)
{
    return tzplatform_context_mkpath3( &global_context, id, path, path2);
}

const char* tzplatform_context_mkpath3(struct tzplatform_context *context, enum tzplatform_variable id, const char *path,
                                                            const char *path2)
{
    const char *array[4];
    const char *result = get_lock( context, id);
    if (result != NULL) {
        array[0] = result;
        array[1] = path;
        array[2] = path2;
        array[3] = NULL;
        result = scratchcat( 1, array);
    }
    unlock( context);
    return result;
}

const char* tzplatform_mkpath4(enum tzplatform_variable id, const char * path,
                                          const char* path2, const char *path3)
{
    return tzplatform_context_mkpath4( &global_context, id, path, path2, path3);
}

const char* tzplatform_context_mkpath4(struct tzplatform_context *context, enum tzplatform_variable id, const char *path,
                                        const char *path2, const char *path3)
{
    const char *array[5];
    const char *result = get_lock( context, id);
    if (result != NULL) {
        array[0] = result;
        array[1] = path;
        array[2] = path2;
        array[3] = path3;
        array[4] = NULL;
        result = scratchcat( 1, array);
    }
    unlock( context);
    return result;
}

uid_t tzplatform_getuid(enum tzplatform_variable id)
{
    return tzplatform_context_getuid( &global_context, id);
}

uid_t tzplatform_context_getuid(struct tzplatform_context *context, enum tzplatform_variable id)
{
    uid_t result = (uid_t)-1;
    const char *value = get_lock( context, id);
    if (value != NULL) {
        pw_get_uid( value, &result);
    }
    unlock( context);
    return result;
}

gid_t tzplatform_getgid(enum tzplatform_variable id)
{
    return tzplatform_context_getgid( &global_context, id);
}

gid_t tzplatform_context_getgid(struct tzplatform_context *context, enum tzplatform_variable id)
{
    gid_t result = (uid_t)-1;
    const char *value = get_lock( context, id);
    if (value != NULL) {
        pw_get_gid( value, &result);
    }
    unlock( context);
    return result;
}

int tzplatform_set_user(uid_t uid)
{
    return tzplatform_context_set_user( &global_context, uid);
}

int tzplatform_context_set_user(struct tzplatform_context *context, uid_t uid)
{
    int result;

    result = 0;
    lock( context);
    if (context->user != uid) {
	    if (uid != _USER_NOT_SET_ && !pw_has_uid( uid))
            result = -1;
        else {
            if (context->state == VALID)
                heap_destroy( &context->heap);
            context->state = RESET;
            context->user = uid;
        }
    }
    unlock( context);

    return result;
}

uid_t tzplatform_get_user()
{
    return tzplatform_context_get_user( &global_context);
}

uid_t tzplatform_context_get_user(struct tzplatform_context *context)
{
    uid_t result;

    lock( context);
    result = get_uid( context);
    unlock( context);

    return result;
}

void tzplatform_reset_user()
{
    tzplatform_context_reset_user( &global_context);
}

void tzplatform_context_reset_user(struct tzplatform_context *context)
{
    tzplatform_context_set_user( context, _USER_NOT_SET_);
}

#ifdef TEST
int main() {
    int i;
    struct tzplatform_context *context;
    enum tzplatform_variable id;
    const char *name;
    const char *value;
    int xid;
    uid_t uid;

    i = 0;
    while(i != tzplatform_getcount()) {
        id = (enum tzplatform_variable)i;
        name = tzplatform_getname(id);
        value = tzplatform_getenv(id);
        xid = (int)tzplatform_getid(name);
        printf("%d=%d\t%s=%s\n",i,xid,name,value?value:"<null>");
        i++;
    }

    printf("------------------------\n");
    i = tzplatform_context_create(&context);
    if (i) {
        printf("error while creating context %d\n",i);
        return 1;
    }

    uid = (uid_t)0;
    i = tzplatform_context_set_user(context, uid);
    if (i) {
        printf("error %d while switching to user %d\n",i,(int)uid);
        return 1;
    }
    i = 0;
    while(i != tzplatform_getcount()) {
        id = (enum tzplatform_variable)i;
        name = tzplatform_getname(id);
        value = tzplatform_context_getenv(context, id);
        xid = (int)tzplatform_getid(name);
        printf("%d=%d\t%s=%s\n",i,xid,name,value?value:"<null>");
        i++;
    }
    tzplatform_context_destroy(context);

    return 0;
}
#endif


