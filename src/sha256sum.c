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

#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

#include "sha256sum.h"

static const char *arg[2] = { "/usr/bin/sha256sum", NULL };

struct sha256sum {
    enum { RUNNING, FAILED, SUCCESSED } state;
    pid_t pid;
    int tofd;
    int fromfd;
    char result[32];
};

struct sha256sum *sha256sum_create()
{
    int fds1[2];
    int fds2[2];
    pid_t pid;
    int sts;
    struct sha256sum *result;

    result = malloc(sizeof * result);
    if (result == NULL)
        return NULL;

    sts = pipe(fds1);
    if (sts != 0) {
        free(result);
        return NULL;
    }

    sts = pipe(fds2);
    if (sts != 0) {
        close( fds1[0]);
        close( fds1[1]);
        free(result);
        return NULL;
    }

    pid = fork();
    if (pid == -1) {
        close( fds1[0]);
        close( fds1[1]);
        close( fds2[0]);
        close( fds2[1]);
        free(result);
        return NULL;
    }

    if (pid == 0) {
        dup2( fds1[0], 0);
        dup2( fds2[1], 1);
        close( fds1[0]);
        close( fds1[1]);
        close( fds2[0]);
        close( fds2[1]);
        execve( arg[0], (char**)arg, environ);
        exit(1);
    }

    close( fds1[0]);
    close( fds2[1]);
    result->state = RUNNING;
    result->pid = pid;
    result->tofd = fds1[1];
    result->fromfd = fds2[0];
    return result;
}

void sha256sum_destroy(struct sha256sum *s)
{
    int sts;

    if (s->state == RUNNING) {
        close(s->fromfd);
        close(s->tofd);
        waitpid(s->pid, &sts, 0);
    }
    free(s);
}

int sha256sum_add_data(struct sha256sum *s, const void *data, size_t length)
{
    ssize_t sl;
    int sts;

    if (s->state != RUNNING)
        return -1;

    while (length) {
        do {
            sl = write(s->tofd, data, length);
        } while (sl == -1 && (errno == EINTR || errno == EAGAIN));
        if (sl == -1) {
            s->state = FAILED;
            close(s->fromfd);
            close(s->tofd);
            waitpid(s->pid, &sts, 0);
            return -1;
        }
        length -= (size_t)sl;
        data = (const void*)((const char*)data + (size_t)sl);
    }
    return 0;
}

int sha256sum_add_file(struct sha256sum *s, const char *filename)
{
    char buffer[16384];
    int fd;
    ssize_t rd;

    fd = open(filename, O_RDONLY);
    if (fd < 0)
        return -1;

    for (;;) {
        do {
            rd = read(fd, buffer, sizeof buffer);
        } while (rd == -1 && (errno == EINTR || errno == EAGAIN));
        if (rd == -1) {
            close(fd);
            return -1;
        }
        if (rd == 0) {
            close(fd);
            return 0;
        }
        if (sha256sum_add_data(s, buffer, (size_t)rd)) {
            close(fd);
            return -1;
        }
    }
}

int sha256sum_get(struct sha256sum *s, char result[32])
{
    int sts;
    int j;
    char buffer[65];
    char c1;
    char c2;

    if (s->state == RUNNING) {
        s->state = FAILED;
        close(s->tofd);
        waitpid(s->pid, &sts, 0);
        if (WIFEXITED(sts) && WEXITSTATUS(sts) == 0) {
            sts = read(s->fromfd, buffer, 65);
            if (sts == 65 && buffer[64] == ' ') {
                s->state = SUCCESSED;
                for (j = 0 ; j < 32 ; j++) {
                    c1 = buffer[j+j];
                    if ('0' <= c1 && c1 <= '9')
                        c1 = c1 - '0';
                    else if ('a' <= c1 && c1 <= 'f')
                        c1 = c1 - 'a' + '\012';
                    else if ('A' <= c1 && c1 <= 'F')
                        c1 = c1 - 'A' + '\012';
                    else {
                        s->state = FAILED;
                        break;
                    }
                    c2 = buffer[j+j+1];
                    if ('0' <= c2 && c2 <= '9')
                        c2 = c2 - '0';
                    else if ('a' <= c2 && c2 <= 'f')
                        c2 = c2 - 'a' + '\012';
                    else if ('A' <= c2 && c2 <= 'F')
                        c2 = c2 - 'A' + '\012';
                    else {
                        s->state = FAILED;
                        break;
                    }
                    s->result[j] = (c1 << 4) | c2;
                }
            }
        }
        close(s->fromfd);
    }

    if (s->state == FAILED)
        return -1;

    memcpy(result, s->result, 32);

    return 0;
}

#ifdef TEST
#include <stdio.h>
#include <assert.h>
int main(int argc, char **argv)
{
    char sum[32];
    int j, r;
    struct sha256sum *s;

    while (*++argv) {
        s = sha256sum_create();
        assert(s != NULL);
        r = sha256sum_add_file(s, *argv);
        assert(r == 0);
        r = sha256sum_get(s, sum);
        assert(r == 0);
        sha256sum_destroy(s);
        for (j=0 ; j < 32 ; j++)
            printf("%02x", (int)(unsigned char)sum[j]);
        printf("  %s\n", *argv);
    }
    return 0;
}
#endif

