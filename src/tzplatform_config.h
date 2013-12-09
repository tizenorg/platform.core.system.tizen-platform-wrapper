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
#ifndef LIBTIZEN_PLATFORM_WRAPPER
#define LIBTIZEN_PLATFORM_WRAPPER

#ifdef __cplusplus
extern "C" {
#endif

#include <tzplatform_variables.h>

/*
 Enforces the removal of the previously evaluated tizen platform variables.

 Call this function in case of changing of user inside the application.
*/
extern
void tzplatform_reset();

/*
 Return the count of variables.
*/
extern
int tzplatform_getcount();

/*
 Return the name of the variable 'id' as a string.
 Return NULL if 'id' is invalid.
*/
extern
const char* tzplatform_getname(enum tzplatform_variable id);

/*
 Return the id of the variable of 'name'.
 Return _TZPLATFORM_VARIABLES_INVALID_ if 'name' doesn't match a 
 valid variable name.
*/
extern
enum tzplatform_variable tzplatform_getid(const char *name);

/*
 Return the read-only string value of the tizen plaform variable 'id'.
 The tizen global values are globals to the application.
 Can return NULL in case of internal error or when 'id' isn't defined.
*/
extern
const char* tzplatform_getenv(enum tzplatform_variable id);

/*
 Return the integer value of the tizen plaform variable 'id'.
*/
extern
int tzplatform_getenv_int(enum tzplatform_variable id);

/*
 Return the string resulting of the concatenation of string value of the 
 tizen plaform variable 'id' and the given string 'str'.

 The returned value is a scratch buffer (unique for the thread) that is
 available until the next call to 'tzplatform_mkstr', 'tzplatform_mkpath', or,
 'tzplatform_mkpath3'.

 Can return NULL in case of internal error.

 Example:
    if TZ_SYS_HOME == "/opt/home" then calling

       tzplatform_mkstr(TZ_SYS_HOME,"-yes") 

    will return "/opt/home-yes"
*/
extern
const char* tzplatform_mkstr(enum tzplatform_variable id, const char * str);

/*
 Return the string resulting of the path-concatenation of string value of the 
 tizen plaform variable 'id' and the given string 'path'.

 path-concatenation is the concatenation taking care of / characters.

 The returned value is a scratch buffer (unique for the thread) that is
 available until the next call to 'tzplatform_mkstr', 'tzplatform_mkpath', or,
 'tzplatform_mkpath3'.

 Can return NULL in case of internal error.

 Example:
    if TZ_SYS_HOME == "/opt/home" then calling

       tzplatform_mkpath(TZ_SYS_HOME,"yes") 

    will return "/opt/home/yes"
*/
extern
const char* tzplatform_mkpath(enum tzplatform_variable id, const char * path);

/*
 Return the string resulting of the path-concatenation of string value of the 
 tizen plaform variable 'id' and the given strings 'path' and 'path2'.

 path-concatenation is the concatenation taking care of / characters.

 The returned value is a scratch buffer (unique for the thread) that is
 available until the next call to 'tzplatform_mkstr', 'tzplatform_mkpath', or,
 'tzplatform_mkpath3'.

 Can return NULL in case of internal error.

 Example:
    if TZ_SYS_HOME == "/opt/home" then calling

       tzplatform_mkpath3(TZ_SYS_HOME,"yes","no")

    will return "/opt/home/yes/no"
*/
extern
const char* tzplatform_mkpath3(enum tzplatform_variable id, const char * path, 
                                                            const char* path2);

/*
 Return the uid for a given user name, stored in variable <id>
 Retun -1 in case of error.

 Example:
	if TZ_USER_NAME=="app" then calling:
	   
	   tzplatform_getuid(TZ_USER_NAME)
	
	will return the uid of the user 'app'
*/
extern
uid_t tzplatform_getuid(enum tzplatform_variable id);

/*
 Return the gid for a given group name, stored in variable <id>
 Retun -1 in case of error.

 Example:
	if TZ_USER_GROUP=="app" then calling:
	   
	   tzplatform_getuid(TZ_USER_GROUP)
	
	will return the gid of the group 'app'
*/
extern
gid_t tzplatform_getgid(enum tzplatform_variable id);

#ifdef __cplusplus
}
#endif

#endif // _LIBTIZEN-PLATFORM-WRAPPER
