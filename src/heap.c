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

#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>

#include "heap.h"

/* align to a size_t size */
inline static size_t align(size_t size)
{
    /* we assume that sizeof(size_t) is a power of 2 */
    assert( (sizeof(size_t) & (sizeof(size_t)-1)) == 0 );
    return (size + (sizeof(size_t)-1)) & ~(sizeof(size_t)-1);
}

/* align to a page size */
inline static size_t pagealign( size_t size)
{
    static size_t pagemask = 0;
    /* we assume that pagesize is a power of 2 */
    if (!pagemask) {
	pagemask = (size_t)sysconf(_SC_PAGE_SIZE) - 1;
	assert( pagemask );
	assert( (pagemask & (pagemask+1)) == 0 );
    }
    return (size + pagemask) & ~pagemask;
}

int heap_create( struct heap *heap, size_t capacity)
{
    char *data;

    /* allocation of the heap */
    capacity = pagealign(capacity ? capacity : 1);
    data = mmap(NULL, capacity, PROT_READ|PROT_WRITE,
                                    MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);

    /* success of the allocation? */
    if (data == MAP_FAILED)
        return -1;

    /* yes. initialisae the heap */
    heap->data = data;
    heap->capacity = capacity;
    heap->size = 0;
    return 0;
}

int heap_destroy( struct heap *heap)
{
    return munmap( heap->data, heap->capacity);
}

int heap_resize( struct heap *heap, size_t size)
{
    /* has heap enought data? */
    if (size > heap->capacity) {
        
        /* no. resizing of the heap. */
        /* compute the sizes and realloc */
        size_t capa = pagealign(size);
        char *data = mremap(heap->data, heap->capacity, capa, MREMAP_MAYMOVE);

        /* error if failure */
        if (data == MAP_FAILED)
            return -1;

        /* record new parameters. */
        heap->data = data;
        heap->capacity = capa;
    }
    heap->size = size;

    return 0;
}

size_t heap_alloc( struct heap *heap, size_t count)
{
    size_t result = heap->size;
    if( heap_resize( heap, align( result + count)) != 0)
        result = HNULL;
    return result;
}

size_t heap_strndup( struct heap *heap, const char *value, size_t length)
{
    size_t offset;
    char *string;

    offset = heap_alloc( heap, length+1);
    if (offset != HNULL) {
        string = heap_address( heap, offset);
        memcpy( string, value, length);
        string[length] = 0;
    }
    return offset;
}

size_t heap_strdup( struct heap *heap, const char *string)
{
    return heap_strndup( heap, string, strlen(string));
}

int heap_read_write( struct heap *heap)
{
    return mprotect(heap->data, heap->capacity, PROT_READ|PROT_WRITE);
}

int heap_read_only( struct heap *heap)
{
    return mprotect(heap->data, heap->capacity, PROT_READ);
}



