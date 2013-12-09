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

#ifndef INITIAL_SCRATCH_CAPACITY
#define INITIAL_SCRATCH_CAPACITY  240
#endif

#if INITIAL_SCRATCH_CAPACITY <= 0
#error "bad value for INITIAL_SCRATCH_CAPACITY"
#endif

#ifndef NOT_MULTI_THREAD_SAFE
#include <pthread.h>
static pthread_key_t tlskey;
static int key_initialized = 0;
#else
static void *global_scratch = NULL;
#endif

/* CAUTION: in a multitheaded context, it is expected that
=========== the function scratchcat is call under a mutex. 
If it is not the case please check for initializing 'tlskey' 
only one time before use of it. */

const char *scratchcat( int ispath, const char **strings)
{
    void *scratch, *p;
    char *result;
    size_t length, capacity;
    const char *instr;
    char c, pc;

    /* get the recorded pointer on scrtch area */
#ifndef NOT_MULTI_THREAD_SAFE
    if (!key_initialized) {
        key_initialized = 1;
        pthread_key_create( &tlskey, (void(*)(void*))free);
    }
    scratch = pthread_getspecific( tlskey);
#else
    scratch = global_scratch;
#endif

    /* create the scratch area if needed */
    if (scratch == NULL) {
        capacity = INITIAL_SCRATCH_CAPACITY;
        p = malloc( capacity + sizeof(size_t));
        if (p == NULL)
            return NULL;
        *((size_t*)p) = capacity;
        scratch = p;
#ifndef NOT_MULTI_THREAD_SAFE
        pthread_setspecific( tlskey, p);
#else
        global_scratch = p;
#endif
    }

    /* set local data for scratch area */
    capacity = *((size_t*)scratch);
    result = (char*)(1+((size_t*)scratch));
    length = 0;

    /* copy the strings */
    c = 0;
    pc = 1;
    while(pc) {

        if (c == 0) {
            instr = *strings++;
            if (instr != NULL) {
                c = *instr;
                if (c == 0)
                    continue;
                if (!ispath)
                    instr++;
                else if(c != '/' && pc != '/')
                    c = '/';
                else if(c == '/' && pc == '/') {
                    instr++;
                    continue;
                }
                else
                    instr++;
            }
        }
        else {
            c = *instr;
            if (c == 0)
                continue;
            instr++;
        }

        /* extend the scratch area if needed */
        if (length == capacity) {
            capacity = 2 * capacity;
            p = realloc( scratch, capacity + sizeof(size_t));
            if (p == NULL)
                return NULL;
            *((size_t*)p) = capacity;
            if (p != scratch) {
                scratch = p;
#ifndef NOT_MULTI_THREAD_SAFE
                pthread_setspecific( tlskey, p);
#else
                global_scratch = p;
#endif
                result = (char*)(1+((size_t*)p));
            }
        }

        /* append the char */
        pc = result[length++] = c;
    }

    return result;
}


#ifdef TEST_SCRATCH
#include <stdio.h>
int main(int argc, const char**argv) {
    int ispath;
    argv++;
    ispath = argv[0] && argv[0][0] == '-' && argv[0][1] == 'p';
    printf("%s\n",scratchcat(ispath,argv+ispath));
    return 0;
}
#endif
