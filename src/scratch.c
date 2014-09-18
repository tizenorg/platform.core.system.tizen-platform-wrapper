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
#include <memory.h>

#ifndef INITIAL_SCRATCH_CAPACITY
#define INITIAL_SCRATCH_CAPACITY  240
#endif

#if INITIAL_SCRATCH_CAPACITY <= 0
#error "bad value for INITIAL_SCRATCH_CAPACITY"
#endif

#define INSTANCIATE 1
#if INSTANCIATE
#define SET_CAPACITY        97
#define HASHCODE_INIT       5381
#define HASHCODE_NEXT(H,C)  (((H) << 5) + (H) + (C))
#endif

#ifndef NOT_MULTI_THREAD_SAFE
#include <pthread.h>
#endif

#if !(defined(NOT_MULTI_THREAD_SAFE) || INSTANCIATE)
static __thread void *global_scratch = NULL;
#else
static void *global_scratch = NULL;
#endif
#if INSTANCIATE && !defined(NOT_MULTI_THREAD_SAFE)
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

#if INSTANCIATE
/* structure for recording items in the hash map */
struct set_item {
    struct set_item *next;        /* chain to next item */
    size_t hashcode;                /* hash of the string */
    size_t length;                /* length of the string including null */
};

/* the array of recorded strings */
static struct set_item *global_path_set[SET_CAPACITY]; /* initialized to  zeros */

/* instanciate (or retrieve an instance) of the 'string' that
is granted to have the 'length' including terminating null and a
hash code 'hashcode' */
static const char *instantiate(const char *string, size_t length, size_t hashcode)
{
    struct set_item **pp, *item;
    char *result;

    /* get first item in the table */
    pp = &global_path_set[hashcode % SET_CAPACITY];
    result = 0;
    do {
        /* inspect the item */
        item = *pp;
        if (!item) {
            /* no item: create it */
            item = malloc(length + sizeof * item);
            if (!item)
                return NULL;
            /* init it */
            item->next = 0;
            item->hashcode = hashcode;
            item->length = length;
            result = (char *)(item + 1);
            memcpy(result, string, length);
            /* record it */
            *pp = item;
        } else if (item->hashcode == hashcode
                && item->length == length
                && 0 == strcmp(string, (const char *)(item + 1))) {
            /* item found */
            result = (char *)(item + 1);
        } else {
            /* try the next */
            pp = &item->next;
        }
    } while (!result);

    return result;
}
#endif

/* CAUTION: in a multitheaded context, it is expected that
=========== the function scratchcat is call under a mutex. 
If it is not the case please check for initializing 'tlskey' 
only one time before use of it. */

#if INSTANCIATE && !defined(NOT_MULTI_THREAD_SAFE)
static const char *_scratchcat( int ispath, const char **strings);

const char *scratchcat( int ispath, const char **strings)
{
    const char *result;
    pthread_mutex_lock(&mutex);
    result = _scratchcat( ispath, strings);
    pthread_mutex_unlock(&mutex);
    return result;
}

static const char *_scratchcat( int ispath, const char **strings)
#else
const char *scratchcat( int ispath, const char **strings)
#endif
{
    void *scratch, *p;
    char *result;
    size_t length, capacity;
    const char *instr;
    char c, pc;
#if INSTANCIATE
    size_t hashcode = HASHCODE_INIT;
#endif

    /* get the recorded pointer on scrtch area */
    scratch = global_scratch;

    /* create the scratch area if needed */
    if (scratch == NULL) {
        capacity = INITIAL_SCRATCH_CAPACITY;
        p = malloc( capacity + sizeof(size_t));
        if (p == NULL)
            return NULL;
        *((size_t*)p) = capacity;
        scratch = p;
        global_scratch = p;
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
                global_scratch = p;
                result = (char*)(1+((size_t*)p));
            }
        }

        /* append the char */
        pc = result[length++] = c;
#if INSTANCIATE
        hashcode = HASHCODE_NEXT(hashcode, (size_t)c);
#endif
    }

#if INSTANCIATE
    return instantiate(result, length, hashcode);
#else
    return result;
#endif
}


#ifdef TEST_SCRATCH
#include <stdio.h>
int main(int argc, const char**argv) {
    int ispath, iter, i;
    argv++;
    ispath = argv[0] && argv[0][0] == '-' && argv[0][1] == 'p';
    for (i = 0 ; i < 2 ; i++) {
        iter = ispath;
        while (iter < argc) {
            const char *p = scratchcat(ispath,argv+iter++);
            printf("%p: %s\n",p,p);
        }
    }
    return 0;
}
#endif
