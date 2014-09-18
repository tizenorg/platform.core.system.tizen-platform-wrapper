/*
 * Copyright (C) 2013-2014 Intel Corporation.
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
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <syslog.h>

#ifndef NOT_MULTI_THREAD_SAFE
#include <pthread.h>
#endif

#include "tzplatform_variables.h"
#include "tzplatform_config.h"
#include "heap.h"
#include "scratch.h"
#include "passwd.h"
#include "foreign.h"
#include "context.h"
#include "hashing.h"
#include "init.h"
#include "shared-api.h"


/* the global context */
static struct tzplatform_context global_context = {
#ifndef NOT_MULTI_THREAD_SAFE
    .mutex = PTHREAD_MUTEX_INITIALIZER,
#endif
    .state = RESET,
    .user = _USER_NOT_SET_
};

/* the signup of names */
#include "signup.inc"

/* validate the signup */
static void validate_signup(char signup[33])
{
    if (memcmp(signup+1, tizen_platform_config_signup+1, 32)) {
        syslog(LOG_CRIT, "Bad signup of the client of tizen-platform-config");
        abort();
    }
    signup[0] = 1;
}

/* check the signup */
static inline void check_signup(char signup[33])
{
    if (!signup[0])
        validate_signup(signup);
}

/* locks the context */
inline static void lock(struct tzplatform_context *context)
{
#ifndef NOT_MULTI_THREAD_SAFE
    pthread_mutex_lock( &context->mutex);
#endif
}

/* unlock the context */
inline static void unlock(struct tzplatform_context *context)
{
#ifndef NOT_MULTI_THREAD_SAFE
    pthread_mutex_unlock( &context->mutex);
#endif
}

static inline const char *get_lock(int id, struct tzplatform_context *context)
{
    lock( context);

    if (id < 0 || (int)_TZPLATFORM_VARIABLES_COUNT_ <= id)
        return NULL;

    if (context->state == RESET)
        initialize( context);

    return context->state == ERROR ? NULL : context->values[id];
}

/*************** PUBLIC API begins here **************/

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
    tzplatform_context_reset(&global_context);
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

uid_t tzplatform_get_user(char signup[33])
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

int tzplatform_set_user(uid_t uid)
{
    return tzplatform_context_set_user( &global_context, uid);
}

int tzplatform_context_set_user(struct tzplatform_context *context, uid_t uid)
{
    lock( context);
    if (context->user != uid) {
        if (uid != _USER_NOT_SET_ && !pw_has_uid( uid)) {
            unlock( context);
            return -1;
	}
        else {
            if (context->state == VALID)
                heap_destroy( &context->heap);
            context->state = RESET;
            context->user = uid;
        }
    }
    unlock( context);

    return 0;
}

/*************** PUBLIC INTERNAL API begins here **************/

const char* _getname_tzplatform_(int id, char signup[33])
{
    check_signup(signup);
    return 0 <= id && id < _TZPLATFORM_VARIABLES_COUNT_ ? keyname(id) : NULL;
}

int _getid_tzplatform_(const char *name, char signup[33])
{
    check_signup(signup);
    return hashid(name, strlen(name));
}

const char* _getenv_tzplatform_(int id, char signup[33]) 
{
    return _context_getenv_tzplatform_(id, signup, &global_context);
}

const char* _context_getenv_tzplatform_(int id, char signup[33], struct tzplatform_context *context)
{
    const char *array[2];
    const char *result;

    check_signup(signup);
    result = get_lock(id, context);
    if (result != NULL) {
        array[0] = result;
        array[1] = NULL;
        result = scratchcat( 0, array);
    }
    unlock( context);
    return result;
}

int _getenv_int_tzplatform_(int id, char signup[33])
{
    return _context_getenv_int_tzplatform_(id, signup, &global_context);
}

int _context_getenv_int_tzplatform_(int id, char signup[33], struct tzplatform_context *context)
{
    const char *value;
    int result;

    check_signup(signup);
    value = get_lock(id, context);
    result = value==NULL ? -1 : atoi(value);
    unlock( context);
    return result;
}

const char* _mkstr_tzplatform_(int id, const char * str, char signup[33])
{
    return _context_mkstr_tzplatform_(id, str, signup,  &global_context);
}

const char* _context_mkstr_tzplatform_(int id, const char *str, char signup[33], struct tzplatform_context *context)
{
    const char *array[3];
    const char *result;

    check_signup(signup);
    result = get_lock(id, context);
    if (result != NULL) {
        array[0] = result;
        array[1] = str;
        array[2] = NULL;
        result = scratchcat( 0, array);
    }
    unlock( context);
    return result;
}

const char* _mkpath_tzplatform_(int id, const char * path, char signup[33])
{
    return _context_mkpath_tzplatform_(id, path, signup,  &global_context);
}

const char* _context_mkpath_tzplatform_(int id, const char *path, char signup[33], struct tzplatform_context *context)
{
    const char *array[3];
    const char *result;

    check_signup(signup);
    result = get_lock(id, context);
    if (result != NULL) {
        array[0] = result;
        array[1] = path;
        array[2] = NULL;
        result = scratchcat( 1, array);
    }
    unlock( context);
    return result;
}

const char* _mkpath3_tzplatform_(int id, const char * path, const char* path2, char signup[33])
{
    return _context_mkpath3_tzplatform_( id, path, path2, signup,  &global_context);
}

const char* _context_mkpath3_tzplatform_(int id, const char *path, const char *path2, char signup[33], struct tzplatform_context *context)
{
    const char *array[4];
    const char *result;

    check_signup(signup);
    result = get_lock(id, context);
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

const char* _mkpath4_tzplatform_(int id, const char * path, const char* path2, const char *path3, char signup[33])
{
    return _context_mkpath4_tzplatform_( id, path, path2, path3, signup,  &global_context);
}

const char* _context_mkpath4_tzplatform_(int id, const char *path, const char *path2, const char *path3, char signup[33], struct tzplatform_context *context)
{
    const char *array[5];
    const char *result;

    check_signup(signup);
    result = get_lock(id, context);
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

uid_t _getuid_tzplatform_(int id, char signup[33])
{
    return _context_getuid_tzplatform_( id, signup,  &global_context);
}

uid_t _context_getuid_tzplatform_(int id, char signup[33], struct tzplatform_context *context)
{
    uid_t result;
    const char *value;

    check_signup(signup);
    result = (uid_t)-1;
    value = get_lock(id, context);
    if (value != NULL) {
        pw_get_uid( value, &result);
    }
    unlock( context);
    return result;
}

gid_t _getgid_tzplatform_(int id, char signup[33])
{
    return _context_getgid_tzplatform_( id, signup,  &global_context);
}

gid_t _context_getgid_tzplatform_(int id, char signup[33], struct tzplatform_context *context)
{
    gid_t result;
    const char *value;

    check_signup(signup);
    result = (uid_t)-1;
    value = get_lock(id, context);
    if (value != NULL) {
        pw_get_gid( value, &result);
    }
    unlock( context);
    return result;
}

