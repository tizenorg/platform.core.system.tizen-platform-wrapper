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
#ifndef FOREIGN_H
#define FOREIGN_H

#define _FOREIGN_MASK_HOME_       1
#define _FOREIGN_MASK_UID_        2
#define _FOREIGN_MASK_USER_       4
#define _FOREIGN_MASK_GID_        8
#define _FOREIGN_MASK_EHOME_     16
#define _FOREIGN_MASK_EUID_      32
#define _FOREIGN_MASK_EUSER_     64
#ifdef HAVE_SYSROOT
#define _FOREIGN_MASK_SYSROOT_  128

#define _FOREIGNS_TO_USE_   ( _FOREIGN_MASK_HOME_    \
                            | _FOREIGN_MASK_USER_    \
                            | _FOREIGN_MASK_SYSROOT_ )
#else /* HAVE_SYSROOT */
#define _FOREIGNS_TO_USE_   ( _FOREIGN_MASK_HOME_  \
                            | _FOREIGN_MASK_USER_  )
#endif /* HAVE_SYSROOT */

#define _FOREIGN_HAS_(x)  (0 != ((_FOREIGNS_TO_USE_) & (_FOREIGN_MASK_##x##_)))

enum fkey {
    _FOREIGN_INVALID_ = -1,
#if _FOREIGN_HAS_(HOME)
    HOME,
#endif
#if _FOREIGN_HAS_(UID)
    UID,
#endif
#if _FOREIGN_HAS_(USER)
    USER,
#endif
#if _FOREIGN_HAS_(GID)
    GID, 
#endif
#if _FOREIGN_HAS_(EHOME)
    EHOME,
#endif
#if _FOREIGN_HAS_(EUID)
    EUID,
#endif
#if _FOREIGN_HAS_(EUSER)
    EUSER,
#endif
#ifdef HAVE_SYSROOT
#if _FOREIGN_HAS_(SYSROOT)
    SYSROOT,
#endif
#endif /* HAVE_SYSROOT */
    _FOREIGN_COUNT_
};

/* get the foreign key for the 'name' of 'length' */
enum fkey foreign( const char *name, size_t length);

#endif

