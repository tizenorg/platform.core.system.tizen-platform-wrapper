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
#include <alloca.h>
#include <ctype.h>

#include "parser.h"

#ifndef MINIMUM_DATA_SIZE
# define MINIMUM_DATA_SIZE             128
#endif
#ifndef DEFAULT_MAXIMUM_DATA_SIZE
# define DEFAULT_MAXIMUM_DATA_SIZE   32768
#endif

#ifndef _
# define _(x) x
#endif

int parse_utf8_config( struct parsing *parsing)
{
    char c, q, acc, overflow, escape;
    char *bdata;
    const char *value, *head, *end, *skey, *svar, *bvar;
    size_t lkey, lvar, ldata, datasz;
    int  errors;

#define iskeybeg(x)     (isalpha(x)||(x)=='_')
#define iskey(x)        (isalnum(x)||(x)=='_')
#define atend           ((head-end) >= 0)

#define pos(x)          ((size_t)((x)-parsing->buffer))
#define error(p,x)      do{ errors++; \
                            if(parsing->error \
                                && !parsing->error(parsing,pos(p),x)) \
                                goto stop; \
                        }while(0)

    /* alloc data buffer */
    datasz = parsing->maximum_data_size;
    if (datasz == 0)
        datasz = DEFAULT_MAXIMUM_DATA_SIZE;
    else if (datasz < MINIMUM_DATA_SIZE)
        datasz = MINIMUM_DATA_SIZE;
    bdata = alloca(datasz);

    /* init */
    escape = parsing->should_escape ? 1 : 0;
    errors = 0;
    head = parsing->buffer;
    end = head + parsing->length;
    goto initial;

next_initial:

    head++;

initial: /* expecting key, comment or ??? */

    if (atend)
        goto end_ok;

    c = *head;

    if (iskeybeg(c)) {
        skey = head;
        goto key;
    }

    if (c == '#')
        goto comment;

    if (!isspace(c)) 
        error(head,_("unexpected character while looking to a key start"));

    goto next_initial;

comment: /* skipping a comment */

    head++;
    if (atend)
        goto end_ok;
    c = *head;
    if (c == '\n')
        goto next_initial;
    goto comment;

key: /* reading a key */

    head++;
    if (atend) {
        error(head,_("end in a key"));
        goto stop;
    }
    c = *head;
    if (iskey(c))
        goto key;
    if (c != '=') {
        error(head,_("unexpected character while looking to ="));
        goto next_initial;
    }
    lkey = head - skey;
    overflow = ldata = 0;
    q = 0;
    goto next_value;

escapable:

    if (escape && ldata < datasz)
        bdata[ldata++] = '\\';

add_value: /* add the current char to data */

    if (ldata < datasz)
        bdata[ldata++] = c;
    else
        overflow = 1;

next_value:

    head++;

value: /* reading the value */

    if (atend) {
        /* end in the value */
        if (!q)
            goto end_of_value;

        error(head,_("end in quoted value"));
        goto stop;
    }

    c = *head;
    switch (c) {

    case '\\':
        if (q == '\'')
            goto escapable;
        ++head;
        if (atend) {
            error(head,_("end after escape char"));
            goto stop;
        }
        c = *head;
        goto escapable;

    case '\'':
        if (q == '"')
            goto escapable;
        q = q ^ c;
        goto next_value;

    case '"':
        if (q == '\'')
            goto escapable;
        q = q ^ c;
        goto next_value;

    case '$':
        if (q == '\'')
            goto escapable;

        /* begin of a variable, just after $ */
        bvar = head++;
        if (atend) {
            error(head,_("end after $"));
            goto stop;
        }
        c = *head;
        acc = c;
        if (c == '{') {
            ++head;
            if (atend) {
                error(head,_("end after ${"));
                goto stop;
            }
            c = *head;
        }
        if (!iskeybeg(c)) {
            error(head,_("invalid character after $ or ${"));
            goto value;
        }
        svar = head;
        goto variable;

    default:
        if (q || !isspace(c)) 
            goto add_value;

        goto end_of_value;
    }

end_of_value: /* end of the value */

    if (overflow)
        error(head,_("value too big"));
    else if (parsing->put)
        parsing->put( parsing, skey, lkey, bdata, ldata, pos(skey), pos(head));
    if (atend)
        goto initial;
    goto next_initial;

variable: /* read a variable */

    ++head;
    if (atend) {
        if (acc == '{')
            error(head,_("unmatched pair { }"));
        lvar = head - svar;
    }
    else {
        c = *head;
        if (iskey(c))
            goto variable;
        lvar = head - svar;
        if (acc == '{') {
            if (c == '}')
                ++head;
            else
                error(head,_("unmatched pair { }"));
        }
    }
    if (parsing->get) {
        value = parsing->get( parsing, svar,lvar,pos(bvar),pos(head));
        if (value == NULL)
            error(bvar,_("no value for the variable"));
        else {
            while (*value) {
                if (ldata < datasz)
                    bdata[ldata++] = *value++;
                else {
                    overflow = 1;
                    break;
                }
            }
        }
    }
    goto value;

stop:
end_ok:
    return -errors;
    
#undef error
#undef pos
#undef atend
#undef iskey
#undef iskeybeg
}

void parse_utf8_info(
            struct parsing *parsing,
            struct parsinfo *info,
            size_t pos
)
{
    const char *buf, *begin, *end;
    size_t length;
    int lino, colno;

    /* init */
    lino = 1, colno = 1;
    buf = begin = end = parsing->buffer;
    length = parsing->length;
    if (length < pos)
        pos = length;

    /* search the begin of the line */
    while ((end - buf) != pos) {
        /* dealing utf8 */
        colno += ((*end & '\xc0') != '\x80');
        /* dealing with lines */
        if(*end++ == '\n') { 
            begin = end; 
            lino++;
            colno = 1;
        }
    }

    /* search the end of the line */
    while ((end - buf) < length && *end != '\n') 
        end++;

    /* record computed values */
    info->begin = begin;
    info->end = end;
    info->length = end - begin;
    info->lino = lino;
    info->colno = colno;
}

