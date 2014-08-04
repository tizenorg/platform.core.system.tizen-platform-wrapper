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
 *   Stephen Clymans <stephen.clymans@open.eurogiciel.org>
 */
#ifdef __cplusplus
extern "C" {
#endif
 
#ifndef TIZEN_PLATFORM_WRAPPER_ISADMIN_H
#define TIZEN_PLATFORM_WRAPPER_ISADMIN_H

/*
 * DISCLAIMER :
 *  This header and its associated source file are present to maintain a 
 * temporary solution. We need to know if an user have the privilege for
 * a particular action.
 * 
 * At the end, this feature will be managed by Cynara
 * 
 */

#include <pwd.h>
 
#define ADMIN_GID 9999

/*
 * Return 0 if the given uid is not in the admin group.
 * Return 1 if the given uid is in the admin group.
 * 
 * If you pass the -1 value to this function it will take the current uid given
 * by the POSIX function getuid();
*/
char is_admin(int uid);

#ifdef __cplusplus
}
#endif

#endif /* TIZEN_PLATFORM_WRAPPER_ISADMIN_H  */

