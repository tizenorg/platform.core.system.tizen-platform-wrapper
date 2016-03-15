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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>

#include <tzplatform_config.h>

#define basename(x) (x)

static const char usage [] = "type '%s --help' to get help.\n";

static const char help [] = "\
\n\
usage: %s [options...] [keys...]\n\
\n\
options:\n\
\n\
-a --all        all keys (must not be followed by keys)\n\
-n --not        not in the keys\n\
-e --export     prefix with export\n\
-l --list       prints only the key names, not the values\n\
-s --space      separate with spaces instead of new-lines\n\
-q --query      silently check that given variables are existing\n\
-c --continue   continue to process if error\n\
-u --user  id   set the user using its 'id' (name or numeric)\n\
\n\
";

int main(int argc, char **argv)
{
	char *progname = *argv++, *user = 0;
	int all = 0, not = 0, query = 0, export = 0, space = 0, list = 0, cont = 0;
	int i, n, *sel, p;
	enum tzplatform_variable id;
	struct passwd pwd, *spw;
	char buf[1024];

	/* parse args */
	while(*argv && **argv=='-') {
		char c, *opt = 1+*argv++;
		while(opt) {
			if (*opt == '-') {
				char *x = 0;
				c = *++opt;
				switch(c) {
					case 'a': x = "all"; break;
					case 'n': x = "not"; break;
					case 'q': x = "query"; break;
					case 'e': x = "export"; break;
					case 's': x = "space"; break;
					case 'l': x = "list"; break;
					case 'c': x = "continue"; break;
					case 'h': x = "help"; break;
					case 'u': x = "user"; break;
				}
				if (!x || strcmp(x,opt))
					c = 0;
				opt = 0;
			}
			else {
				c = *opt;
				if (!*++opt)
					opt = 0;
			}
			switch(c) {
			case 'a': all = 1; break;
			case 'n': not = 1; break;
			case 'q': query = 1; break;
			case 'e': export = 1; break;
			case 's': space = 1; break;
			case 'l': list = 1; break;
			case 'c': cont = 1; break;
			case 'u': user = *argv; if (user) argv++; break;
			case 'h':
				fprintf( stdout, help, basename(progname));
				return 0;
			default:
				fprintf( stderr, usage, basename(progname));
				return 1;
			}
		}
	}

	/* some checks */
	if (query) {
		if (all) {
			fprintf( stderr, 
				"error! --all and --query aren't compatibles.\n");
			return 1;
		}
		if (not) {
			fprintf( stderr, 
				"error! --not and --query aren't compatibles.\n");
			return 1;
		}
		if (list) {
			fprintf( stderr, 
				"error! --list and --query aren't compatibles.\n");
			return 1;
		}
		if (cont) {
			fprintf( stderr, 
				"error! --continue and --query aren't compatibles.\n");
			return 1;
		}
		if (space) {
			fprintf( stderr, 
				"warning! --space option ignored for queries.\n");
		}
		if (export) {
			fprintf( stderr, 
				"warning! --export option ignored for queries.\n");
		}
	}
	if (all) {
		if (*argv) {
			fprintf( stderr, 
				"error! --all doesn't accept any key.\n");
			return 1;
		}
		/* all is like not nothing!! */
		not = 1;
	}

	/* process */
	n = tzplatform_getcount();
	sel = calloc( sizeof(int), n);
	if (sel == NULL) {
		fprintf( stderr, "error! out of memory!\n");
		return 1;
	}

	/* get the variables from the list */
	while (*argv) {
		id = tzplatform_getid( *argv);
		if (id == _TZPLATFORM_VARIABLES_INVALID_) {
			if (query)
				return 1;
			fprintf( stderr, "error! %s isn't a variable.\n", *argv);
			if (!cont)
				return 1;
		}
		else {
			sel[(int)id] = 1;
		}
		argv++;
	}

	/* finished for queries */
	if (query) 
		return 0;

	/* set the user */
	if (user) {
		for (i=0 ; '0' <= user[i] && user[i] <= '9' ; i++);
		if (user[i])
			getpwnam_r(user, &pwd, buf, sizeof(buf), &spw);
		else
			getpwuid_r(user, &pwd, buf, sizeof(buf), &spw);
		if (!spw) {
			fprintf( stderr, "error! %s isn't standing for a valid user.\n", user);
			if (!cont)
				return 1;
		} else {
			i = tzplatform_set_user(spw->pw_uid);
			if (i) {
				fprintf( stderr, "error! can't set the valid user %s.\n", user);
				if (!cont)
					return 1;
			}
		}
	}

	/* emits the result */
	for (p = i = 0 ; i < n ; i++) {
		if (sel[i] != not) {
			if (p++) 
				printf( space ? " " : export ? "\nexport " : "\n");
			else if (export)
				printf( "export ");
			id = (enum tzplatform_variable) i;
			printf( "%s", tzplatform_getname(id));
			if (!list)	
				printf( "=%s", tzplatform_getenv(id));
		}
	}
	if (p)
		printf("\n");
	return 0;
}

