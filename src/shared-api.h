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

#ifndef SHARED_API_H
#define SHARED_API_H

extern const char* _getname_tzplatform_(int id, char signup[33]);
extern int _getid_tzplatform_(const char *name, char signup[33]);
extern const char* _getenv_tzplatform_(int id, char signup[33]) ;
extern const char* _context_getenv_tzplatform_(int id, char signup[33], struct tzplatform_context *context);
extern int _getenv_int_tzplatform_(int id, char signup[33]);
extern int _context_getenv_int_tzplatform_(int id, char signup[33], struct tzplatform_context *context);
extern const char* _mkstr_tzplatform_(int id, const char * str, char signup[33]);
extern const char* _context_mkstr_tzplatform_(int id, const char *str, char signup[33], struct tzplatform_context *context);
extern const char* _mkpath_tzplatform_(int id, const char * path, char signup[33]);
extern const char* _context_mkpath_tzplatform_(int id, const char *path, char signup[33], struct tzplatform_context *context);
extern const char* _mkpath3_tzplatform_(int id, const char * path, const char* path2, char signup[33]);
extern const char* _context_mkpath3_tzplatform_(int id, const char *path, const char *path2, char signup[33], struct tzplatform_context *context);
extern const char* _mkpath4_tzplatform_(int id, const char * path, const char* path2, const char *path3, char signup[33]);
extern const char* _context_mkpath4_tzplatform_(int id, const char *path, const char *path2, const char *path3, char signup[33], struct tzplatform_context *context);
extern uid_t _getuid_tzplatform_(int id, char signup[33]);
extern uid_t _context_getuid_tzplatform_(int id, char signup[33], struct tzplatform_context *context);
extern gid_t _getgid_tzplatform_(int id, char signup[33]);
extern gid_t _context_getgid_tzplatform_(int id, char signup[33], struct tzplatform_context *context);

#endif

