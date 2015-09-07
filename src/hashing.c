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

#include <string.h>
#include <assert.h>

#include "tzplatform_variables.h"
#include "hash.inc"

static const char *var_names[_TZPLATFORM_VARIABLES_COUNT_];

int hashid(const char *text, unsigned int len)
{
    const struct varassoc *vara = hashvar(text, len);
    return vara ? vara->id : -1;
}

const char *keyname(int id)
{
    const struct varassoc *iter, *end;

    assert(0 <= id && id < _TZPLATFORM_VARIABLES_COUNT_);
    if (!var_names[0]) {
        iter = namassoc;
        end = iter + (sizeof namassoc / sizeof namassoc[0]);
        while (iter != end) {
            if (iter->offset >= 0) {
                assert(0 <= iter->id && iter->id < _TZPLATFORM_VARIABLES_COUNT_);
                var_names[iter->id] = varpool + iter->offset;
            }
            iter++;
        }
    }
    return var_names[id];
}
 
