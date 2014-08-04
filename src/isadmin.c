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

#include "isadmin.h"
 
char is_admin(int uid) {
	
	struct passwd *userinfo = NULL;
	uid_t myuid = 0;
	gid_t *groups = NULL;
	int i, nbgroups = 0;
	
	
	if(uid == -1)
		/* Get current uid */
		myuid = getuid();
	else
		myuid = uid;
		
	userinfo = getpwuid(myuid);
	
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
		fprintf( stderr, "isadmin ERROR: cannot get groups\n");
		return -1;
	}
	
	for(i = 0 ; i < nbgroups ; i++) {
		if(groups[i] == ADMIN_GID)
			return 1;
	}
	
	return 0;
}
