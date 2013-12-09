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
#ifndef BUFFER_H
#define BUFFER_H

/* structure of the buffer */
struct buffer {
    char    *buffer;    /* start address */
    size_t   length;    /* length in byte */
    int      mapped;    /* is memory mapped */
};

/*
   Create the 'buffer' from reading content of the file of 'pathname'.
   Returns 0 if success, -1 if error occured (see then errno)
*/
int buffer_create( struct buffer *buffer, const char *pathname);

/*
   Destroy the 'buffer'.
   Returns 0 if success, -1 if error occured (see then errno)
*/
int buffer_destroy( struct buffer *buffer);

#endif

