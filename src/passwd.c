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
#define _GNU_SOURCE

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>

#include "heap.h"
#include "passwd.h"

#ifndef NOT_PASSWD_ONLY

#include "buffer.h"

/* index of fields */
enum { iname, ipasswd, iuid, igid, icmt, idir, ishell };

static const char pwfile[] = "/etc/passwd";
static struct buffer buffer;
static size_t pos, lengths[7];
static const char *starts[7];

#define is(s,i) (!strncmp(s,starts[i],lengths[i]) && !s[lengths[i]])

/* open the passwd file */
static int oppw()
{
    pos = 0;
    return buffer_create( &buffer, pwfile);
}

/* close the passwd file */
static void clpw()
{
    buffer_destroy( &buffer);
}

/* read the passwd file */
static int rdpw()
{
    int col = 0;
    const char *head = buffer.buffer + pos;
    const char *end = buffer.buffer + buffer.length;
    while (head != end) {
        starts[col] = head;
        while(head!=end && *head!=':' && *head!='\n') head++;
        lengths[col] = head - starts[col];
        col++;
        if (col == 7) {
            if (head==end) {
                pos = head - buffer.buffer;
                return 1;
            }
            if (*head=='\n') {
                head++;
                pos = head - buffer.buffer;
                return 1;
            }
            while (head!=end && *head!='\n') head++;
            if (head!=end) head++;
            col = 0;
        }
        else {
            if (head != end) {
                if (*head++=='\n') 
                    col = 0;
            }
        }
    }
    pos = head - buffer.buffer;
    return 0;
}

int pw_get( struct heap *heap, struct pwget **items)
{
    size_t user, home;
    int result;
    int i, n, s;

    for( n = 0 ; items[n] != NULL ; n++ )
        items[n]->set = 0;

    result = oppw();
    if (result == 0) {
        s = n;
        while (s && rdpw()) {
            user = home = HNULL;
            for( i = 0 ; i < n ; i++ ) {
                if (!items[i]->set && is(items[i]->id,iuid)) {
                    if (user==HNULL) {
                        user = heap_strndup( heap,
                                            starts[iname], lengths[iname]);
                        home = heap_strndup( heap,
                                            starts[idir], lengths[idir]);
                    }
                    items[i]->set = 1;
                    items[i]->user = user;
                    items[i]->home = home;
                    s--;
                }
            }
        }
        clpw();
    }
    return result;
}

int pw_has_uid( uid_t uid)
{
    if (oppw() == 0) {
        while (rdpw()) {
            if (lengths[iuid] && (int)uid == atoi(starts[iuid])) {
                clpw();
                return 1;
            }
        }
        clpw();
    }
    return 0;
}

int pw_get_uid( const char *name, uid_t *uid)
{
    int result = oppw();
    if (result == 0) {
        while (rdpw()) {
            if (is(name,iname)) {
                *uid = (uid_t)atoi(starts[iuid]);
                clpw();
                return 0;
            }
        }
        clpw();
        result = -1;
        errno = EEXIST;
    }
    return result;
}

int pw_get_gid( const char *name, gid_t *gid)
{
    int result = oppw();
    if (result == 0) {
        while (rdpw()) {
            if (is(name,iname)) {
                *gid = (gid_t)atoi(starts[igid]);
                clpw();
                return 0;
            }
        }
        clpw();
        result = -1;
        errno = EEXIST;
    }
    return result;
}

#else

#include <pwd.h>

#define BUFSIZE  4096

int pw_get( struct heap *heap, struct pwget **items)
{
    char buffer[BUFSIZE];
    struct passwd entry, *pe;
    int result;
    int n;
    uid_t id;

    for( n = 0 ; items[n] != NULL ; n++ ) {
        id = (uid_t)atoi(items[n]->id);
        result = getpwuid_r( id, &entry, buffer, sizeof buffer, &pe);
        if (result != 0) {
            while(items[n] != NULL) items[n++]->set = 0;
            return result;
        }
        if (pe == NULL) 
            items[n]->set = 0;
        else {
            items[n]->set = 1;
            items[n]->user = heap_strdup( heap, pe->pw_name);
            items[n]->home = heap_strdup( heap, pe->pw_dir);
        }
    }
    return 0;
}

int pw_has_uid( uid_t uid)
{
    char buffer[BUFSIZE];
    struct passwd entry, *pe;
    int result;

    result = getpwuid_r( uid, &entry, buffer, sizeof buffer, &pe);
    return !result && pe;
}

int pw_get_uid( const char *name, uid_t *uid)
{
    char buffer[BUFSIZE];
    struct passwd entry, *pe;
    int result = getpwnam_r( name, &entry, buffer, sizeof buffer, &pe);
    if (result == 0) {
        if (pe == NULL) {
            errno = EEXIST;
            result = -1;
        }
        else {
            *uid = entry.pw_uid;
        }
    }
    return result;
}

int pw_get_gid( const char *name, gid_t *gid)
{
    char buffer[BUFSIZE];
    struct passwd entry, *pe;
    int result = getpwnam_r( name, &entry, buffer, sizeof buffer, &pe);
    if (result == 0) {
        if (pe == NULL) {
            errno = EEXIST;
            result = -1;
        }
        else {
            *gid = entry.pw_gid;
        }
    }
    return result;
}

#endif

#ifdef TEST_PASSWD
#include <stdio.h>
int main(int argc,char**argv) {
    struct heap heap;
    enum { uid, gid, names } action = uid;
    heap_create(&heap,1000);
    while(*++argv) {
        if (!strcmp(*argv,"-u"))
            action = uid;
        else if (!strcmp(*argv,"-g"))
            action = gid;
        else if (!strcmp(*argv,"-n"))
            action = names;
        else if (action == uid) {
            uid_t u = 0;
            int sts = pw_get_uid( *argv, &u);
            printf("uid(%s)=%d [%d]\n",*argv,(int)u,sts);
        }
        else if (action == gid) {
            gid_t g = 0;
            int sts = pw_get_gid( *argv, &g);
            printf("gid(%s)=%d [%d]\n",*argv,(int)g,sts);
        }
        else if (action == names) {
            struct pwget pw = { .id=*argv }, *ppw[2] = { &pw, NULL };
            int sts = pw_get( &heap, ppw);
            printf("names(%s) set=%d",*argv,pw.set);
            if (pw.set) {
                printf(" name=%s home=%s",
                    pw.user==HNULL ? "(null)" 
                                    : (char*)heap_address( &heap, pw.user),
                    pw.home==HNULL ? "(null)" 
                                    : (char*)heap_address( &heap, pw.home));
            }
            printf(" [%d]\n",sts);
        }
    }
    return 0;
}
#endif


