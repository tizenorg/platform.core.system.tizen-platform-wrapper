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
#ifndef TIZEN_PLATFORM_WRAPPER_PASSWD_H
#define TIZEN_PLATFORM_WRAPPER_PASSWD_H

struct pwget {
    int set;
    const char *id;
    size_t user;
    size_t home;
};

int pw_get( struct heap *heap, struct pwget **items);
int pw_get_uid( const char *name, uid_t *uid);
int pw_get_gid( const char *name, gid_t *gid);
int pw_has_uid( uid_t uid);

#endif

