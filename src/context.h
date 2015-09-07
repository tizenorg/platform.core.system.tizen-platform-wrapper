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
#ifndef CONTEXT_H
#define CONTEXT_H

#ifndef HEAP_H
#error "you should include heap.h"
#endif

#ifndef FOREIGN_H
#error "you should include foreign.h"
#endif

enum STATE { RESET=0, ERROR, VALID };

#define _USER_NOT_SET_  ((uid_t)-1)

struct tzplatform_context {
#ifndef NOT_MULTI_THREAD_SAFE
    pthread_mutex_t mutex;
#endif
    enum STATE state;
    uid_t user;
    struct heap heap;
    const char *values[_TZPLATFORM_VARIABLES_COUNT_];
};

uid_t get_uid(struct tzplatform_context *context);

#if _FOREIGN_HAS_(EUID)
uid_t get_euid(struct tzplatform_context *context);
#endif

#if _FOREIGN_HAS_(GID)
gid_t get_gid(struct tzplatform_context *context);
#endif

#endif

