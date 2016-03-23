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
 
 /*
 * DISCLAIMER :
 *  This source file and its associated header are present to maintain a 
 * temporary solution. We need to know if an user have the privilege for
 * a particular action.
 * 
 * At the end, this feature will be managed by Cynara
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <grp.h>
#include <pwd.h>

#include "isadmin.h"
#include "tzplatform_variables.h"
#include "tzplatform_config.h"

int _has_system_group_static_(uid_t uid) {
	
	struct passwd pwd, *userinfo = NULL;
	struct group grp, *systemgroupinfo = NULL;
	const char *sysgrpname = NULL;
	char buf[1024];
	uid_t myuid = 0;
	gid_t system_gid = 0;
	gid_t *groups = NULL;
	int i, nbgroups = 0;
	
	
	if(uid == -1)
		/* Get current uid */
		myuid = getuid();
	else
		myuid = uid;
	
	/* Get the gid of the group named "system" */
	sysgrpname = tzplatform_getname(TZ_SYS_ADMIN_GROUP);
	if(sysgrpname == NULL) {
		fprintf( stderr, "isadmin ERROR: variable TZ_SYS_ADMIN_GROUP is NULL");
		return -1;
	}
	getgrnam_r(sysgrpname, &grp, buf, sizeof(buf), &systemgroupinfo);
	if(systemgroupinfo == NULL) {
		fprintf( stderr, "isadmin ERROR: cannot find group named \"%s\"\n", sysgrpname);
		return -1;
	}
	
	system_gid = systemgroupinfo->gr_gid;
	
	/* Get all the gid of the given uid */
	
	getpwuid_r(myuid, &pwd, buf, sizeof(buf), &userinfo);
	
	/* Need to call this function now to get the number of group to make the
	   malloc correctly sized */
	if (getgrouplist(userinfo->pw_name, userinfo->pw_gid, groups, &nbgroups) != -1) {
		fprintf( stderr, "isadmin ERROR: cannot get number of groups\n");
		return -1;
	}
	
	groups = malloc(nbgroups * sizeof (gid_t));
	if (groups == NULL) {
		fprintf( stderr, "isadmin ERROR: malloc cannot allocate memory\n");
		return -1;
	}
	
	if (getgrouplist(userinfo->pw_name, userinfo->pw_gid, groups, &nbgroups) == -1) {
		free(groups);
		fprintf( stderr, "isadmin ERROR: cannot get groups\n");
		return -1;
	}
	
	/* Check if the given uid is in the system group */
	
	for(i = 0 ; i < nbgroups ; i++) {
		if(groups[i] == system_gid) {
			free(groups);
			return 1;
		}
	}
	free(groups);
	return 0;
}

