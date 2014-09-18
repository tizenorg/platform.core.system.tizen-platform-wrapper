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
#ifndef TIZEN_PLATFORM_WRAPPER_SCRATCH_H
#define TIZEN_PLATFORM_WRAPPER_SCRATCH_H

/*
 Return a scratch buffer containing the concatenation of the strings of the
 array 'strings'. If 'ispath' isn't zero (then is "true") the directory
 separator is inserted between the strings. The last item of 'strings' must
 be NULL to indicate the end.

 The returned value is a scratch buffer (unique for the thread) that is
 available until the next call.

 Can return NULL in case of internal error (memory depletion).

 Example:

    char *array[] = { "hello", " ", "world", "!", NULL };

    scratchcat( 0, array) will return "hello world!"

    scratchcat( 1, array) will return "hello/ /world/!"
*/
const char *scratchcat( int ispath, const char **strings);


#endif

