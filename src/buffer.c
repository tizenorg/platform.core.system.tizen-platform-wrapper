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
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "buffer.h"

static int buffread( struct buffer *buffer, int fd)
{
    const int size = 4096;
    void *p;
    char *memory = NULL;
    size_t length = 0;
    ssize_t status;

    /* do read */
    for(;;) {
        p = realloc( memory, length + size);
        if (p == NULL) {
            free( memory);
            return -1;
        }
        memory = p;
        status = read( fd, memory+length, size);
        if (status == 0) {
            buffer->buffer = memory;
            buffer->length = length;
            buffer->mapped = 0;
            return 0;
        }
        if (status > 0) {
            length = length + (int)status;
        }
        else if (errno != EAGAIN && errno != EINTR) {
            free( memory);
            return -1;
        }   
    }
}

int buffer_create( struct buffer *buffer, const char *pathname)
{
    int fd, result;
    struct stat bstat;
    void *memory;
    size_t length;

    result = open(pathname, O_RDONLY);
    if (result >= 0)
    {
        fd = result;
        result = fstat(fd, &bstat);
        if (result == 0)
        {
            length = (size_t)bstat.st_size;
            if (bstat.st_size != (off_t)length)
            {
                errno = EOVERFLOW;
                result = -1;
            }
            else
            {
                memory = mmap(NULL,length,PROT_READ,MAP_PRIVATE,fd,0);
                if (memory != MAP_FAILED) {
                    buffer->buffer = memory;
                    buffer->length = length;
                    buffer->mapped = 1;
                }
                else {
                    result = buffread( buffer, fd);
                }
            }
        }
        close(fd);
    }
    return result;
}

int buffer_destroy( struct buffer *buffer)
{
    if (buffer->mapped)
        return munmap(buffer->buffer, buffer->length);
    free( buffer->buffer);
    return 0;
}


