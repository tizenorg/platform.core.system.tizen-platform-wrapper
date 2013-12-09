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
#ifndef HEAP_H
#define HEAP_H

/* null offset value */
#define HNULL               ((size_t)-1)

/* structure defining the heap */
struct heap {
    char  *data;     /* pointer to data of the heap */
    size_t size;     /* count of used byte of the heap */
    size_t capacity; /* count of byte of the heap */
};

/*
   Return the address of the 'heap' memory of 'offset'.
   The returned pointer is only valid until
   a call to 'heap_destroy', 'heap_resize', 'heap_alloc' 
*/
inline static void *heap_address( struct heap *heap, size_t offset)
{
    return heap->data + offset;
}


/*
   Initialize the 'heap' with a 'capacity' in byte.
   The allocated size will be zero.
   Returns 0 if success, -1 if error occured (see then errno)
*/
int heap_create( struct heap *heap, size_t capacity);

/*
   Resize the 'heap' to 'size'.
   Returns 0 if success, -1 if error occured (see then errno)
*/
int heap_resize( struct heap *heap, size_t size);

/*
   Allocation of 'count' bytes from the 'heap'.
   Returns the offset (in byte) from the start
   of the heap. Return HNULL in case of error (see then errno).
*/
size_t heap_alloc( struct heap *heap, size_t count);

/*
   Copy the 'string' of 'length' in the 'heap', appending a terminating null.
   Return the offset or HNULL if error.
*/
size_t heap_strndup( struct heap *heap, const char *string, size_t length);

/*
   Copy the 'string' in the 'heap' with the terminating null.
   Return the offset or HNULL if error.
*/
size_t heap_strdup( struct heap *heap, const char *string);

/*
   Destroy the 'heap'.
   Returns 0 if success, -1 if error occured (see then errno)
*/
int heap_destroy( struct heap *heap);

/*
   Set the heap as read/write
   Returns 0 if success, -1 if error occured (see then errno)
*/
int heap_read_write( struct heap *heap);

/*
   Set the heap as read only
   Returns 0 if success, -1 if error occured (see then errno)
*/
int heap_read_only( struct heap *heap);

#endif

