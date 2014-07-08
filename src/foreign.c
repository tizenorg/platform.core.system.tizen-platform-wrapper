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

#include "foreign.h"

enum fkey foreign( const char *name, size_t length)
{
    if (length == 3) { /* UID */
        if (name[1]=='I' && name[2]=='D')
            switch (name[0]) {
#if _FOREIGN_HAS_(UID)
            case 'G': /* GID */ return GID;
#endif
#if _FOREIGN_HAS_(GID)
            case 'U': /* UID */ return UID;
#endif
            default: break;
            }
    }
    else if (length == 4) {
        switch (name[0]) {
#if _FOREIGN_HAS_(HOME)
        case 'H': /* HOME */
            if (name[1]=='O' && name[2]=='M' && name[3]=='E')
                return HOME;
            break;
#endif
#if _FOREIGN_HAS_(EUID)
        case 'E': /* EUID */
            if (name[1]=='U' && name[2]=='I' && name[3]=='D')
                return EUID;
            break;
#endif
#if _FOREIGN_HAS_(USER)
        case 'U': /* USER */
            if (name[1]=='S' && name[2]=='E' && name[3]=='R')
                return USER;
            break;
#endif
        default: break;
        }
    }
    else if (length == 5 && name[0]=='E') {
        switch (name[1]) {
#if _FOREIGN_HAS_(EHOME)
        case 'H': /* EHOME */
            if (name[2]=='O' && name[3]=='M' && name[4]=='E')
                return EHOME;
            break;
#endif
#if _FOREIGN_HAS_(EUSER)
        case 'U': /* EUSER */
            if (name[2]=='S' && name[3]=='E' && name[4]=='R')
                return EUSER;
            break;
#endif
        default: break;
        }
    }
#if _FOREIGN_HAS_(SYSROOT)
    else if (length == 7)
        if (name[0]=='S' && name[1]=='Y' && name[2]=='S' && name[3]=='R' && name[4]=='O' && name[5]=='O' && name[6]=='T')
            return SYSROOT;
#endif
    return _FOREIGN_INVALID_;
}

