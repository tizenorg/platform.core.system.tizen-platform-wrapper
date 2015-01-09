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
 
#ifndef TIZEN_PLATFORM_WRAPPER_GROUPS_H
#define TIZEN_PLATFORM_WRAPPER_GROUPS_H

#include "tzplatform_variables.h"

/*
 * This feature aims to know if a user belongs to a specified group
 */

/*
 * Return 0 if the given uid is not in the specified group.
 * Return 1 if the given uid is in the specified group.
 * 
 * If you pass the -1 value to this function it will take the current uid given
 * by the POSIX function getuid();
*/
int _has_specified_group_static_(uid_t uid, enum tzplatform_variable group);

#ifdef __cplusplus
}
#endif

#endif /* TIZEN_PLATFORM_WRAPPER_GROUPS_H  */

