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

#include <unistd.h>
#include "tzplatform_variables.h"
#include "tzplatform_config.h"

#include "shared-api.h"
#include "isadmin.h"

#include "signup.inc"

int tzplatform_getcount()
{
    return _TZPLATFORM_VARIABLES_COUNT_;
}

const char* tzplatform_getname(enum tzplatform_variable id)
{
    return _getname_tzplatform_(id, tizen_platform_config_signup);
}

enum tzplatform_variable tzplatform_getid(const char *name)
{
    return _getid_tzplatform_(name, tizen_platform_config_signup);
}

const char* tzplatform_getenv(enum tzplatform_variable id) 
{
    return _getenv_tzplatform_(id, tizen_platform_config_signup);
}

const char* tzplatform_context_getenv(struct tzplatform_context *context, enum tzplatform_variable id)
{
    return _context_getenv_tzplatform_(id, tizen_platform_config_signup, context);
}

int tzplatform_getenv_int(enum tzplatform_variable id)
{
    return _getenv_int_tzplatform_(id, tizen_platform_config_signup);
}

int tzplatform_context_getenv_int(struct tzplatform_context *context, enum tzplatform_variable id)
{
    return _context_getenv_int_tzplatform_(id, tizen_platform_config_signup, context);
}

const char* tzplatform_mkstr(enum tzplatform_variable id, const char * str)
{
    return _mkstr_tzplatform_(id, str, tizen_platform_config_signup);
}

const char* tzplatform_context_mkstr(struct tzplatform_context *context, enum tzplatform_variable id, const char *str)
{
    return _context_mkstr_tzplatform_(id, str, tizen_platform_config_signup, context);
}

const char* tzplatform_mkpath(enum tzplatform_variable id, const char * path)
{
    return _mkpath_tzplatform_(id, path, tizen_platform_config_signup);
}

const char* tzplatform_context_mkpath(struct tzplatform_context *context, enum tzplatform_variable id, const char *path)
{
    return _context_mkpath_tzplatform_(id, path, tizen_platform_config_signup, context);
}

const char* tzplatform_mkpath3(enum tzplatform_variable id, const char * path, const char* path2)
{
    return _mkpath3_tzplatform_(id, path, path2, tizen_platform_config_signup);
}

const char* tzplatform_context_mkpath3(struct tzplatform_context *context, enum tzplatform_variable id, const char *path, const char *path2)
{
    return _context_mkpath3_tzplatform_(id, path, path2, tizen_platform_config_signup, context);
}

const char* tzplatform_mkpath4(enum tzplatform_variable id, const char * path, const char* path2, const char *path3)
{
    return _mkpath4_tzplatform_(id, path, path2, path3, tizen_platform_config_signup);
}

const char* tzplatform_context_mkpath4(struct tzplatform_context *context, enum tzplatform_variable id, const char *path, const char *path2, const char *path3)
{
    return _context_mkpath4_tzplatform_(id, path, path2, path3, tizen_platform_config_signup, context);
}

uid_t tzplatform_getuid(enum tzplatform_variable id)
{
    return _getuid_tzplatform_(id, tizen_platform_config_signup);
}

uid_t tzplatform_context_getuid(struct tzplatform_context *context, enum tzplatform_variable id)
{
    return _context_getuid_tzplatform_(id, tizen_platform_config_signup, context);
}

gid_t tzplatform_getgid(enum tzplatform_variable id)
{
    return _getgid_tzplatform_(id, tizen_platform_config_signup);
}

gid_t tzplatform_context_getgid(struct tzplatform_context *context, enum tzplatform_variable id)
{
    return _context_getgid_tzplatform_(id, tizen_platform_config_signup, context);
}

int tzplatform_isadmin(uid_t uid) 
{
	return _is_admin_static_(uid);
}

#ifdef TEST
#include <stdlib.h>
#include <stdio.h>

int main() {
    int i;
    struct tzplatform_context *context;
    enum tzplatform_variable id;
    const char *name;
    const char *value;
    int xid;
    uid_t uid;

    i = 0;
    while(i != tzplatform_getcount()) {
        id = (enum tzplatform_variable)i;
        name = tzplatform_getname(id);
        value = tzplatform_getenv(id);
        xid = (int)tzplatform_getid(name);
        printf("%d=%d\t%s=%s\n",i,xid,name,value?value:"<null>");
        i++;
    }

    printf("------------------------\n");
    i = tzplatform_context_create(&context);
    if (i) {
        printf("error while creating context %d\n",i);
        return 1;
    }

    uid = (uid_t)0;
    i = tzplatform_context_set_user(context, uid);
    if (i) {
        printf("error %d while switching to user %d\n",i,(int)uid);
        return 1;
    }
    i = 0;
    while(i != tzplatform_getcount()) {
        id = (enum tzplatform_variable)i;
        name = tzplatform_getname(id);
        value = tzplatform_context_getenv(context, id);
        xid = (int)tzplatform_getid(name);
        printf("%d=%d\t%s=%s\n",i,xid,name,value?value:"<null>");
        i++;
    }
    tzplatform_context_destroy(context);

    return 0;
}
#endif


