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

#include <unistd.h>

#ifndef NOT_MULTI_THREAD_SAFE
#include <pthread.h>
#endif

#include "tzplatform_variables.h"
#include "heap.h"
#include "foreign.h"
#include "context.h"


inline uid_t get_uid(struct tzplatform_context *context)
{
    uid_t result;

    result = context->user;
    if (result == _USER_NOT_SET_)
        result = getuid();

    return result;
}

#if _FOREIGN_HAS_(EUID)
inline uid_t get_euid(struct tzplatform_context *context)
{
    uid_t result;

    result = context->user;
    if (result == _USER_NOT_SET_)
        result = geteuid();

    return result;
}
#endif

#if _FOREIGN_HAS_(GID)
inline gid_t get_gid(struct tzplatform_context *context)
{
    return getgid();
}
#endif

