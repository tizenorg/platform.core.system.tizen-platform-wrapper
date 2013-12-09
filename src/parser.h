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
#ifndef TIZEN_PLATFORM_WRAPPER_PARSER_H
#define TIZEN_PLATFORM_WRAPPER_PARSER_H

/* structure used for parsing config files */
struct parsing {

    /* The buffer to parse. */
    const char *buffer; 

    /* The length of the buffer to parse. */
    size_t length; 

    /* The maximum data size allowed */
    size_t maximum_data_size;

    /* Some user data, please use it as you wish. */
    void * data;

    /* Should escape the key values (inserting \ where needed) */
    int should_escape;

    /*
      Callback function to resolve the variables.
      Should return the value of the variable of 'key' that
      length is 'length' without terminating zero.
    */
    const char *(*get)( struct parsing *parsing, 
                const char *key, size_t length,
                size_t begin_pos, size_t end_pos);

    /*
      Callback function to receive new values. 
      Should add/insert/replace the key/value pair
      given. This values aren't zero terminated.
      The given length is the one without terminating nul.
    */
    int (*put)( struct parsing *parsing, 
                const char *key, size_t key_length, 
                const char *value, size_t value_length,
                size_t begin_pos, size_t end_pos);

    /*
      Callback function to report errors.
      'buffer' is the scanned buffer.
      'position' is the position of the character raising the error.
      'message' is a short explanation of the error.
      Should return 0 to stop parsing;
    */
    int (*error)( struct parsing *parsing, 
                size_t position, const char *message);
};

/*
   Parse the config file using data of 'parsing'. 
   Return 0 if not error found or a negative number
   corresponding to the opposite of the count of found errors 
   (ex: -5 means 5 errors).
   Note: works on utf8 data.
*/
int parse_utf8_config(
    struct parsing *parsing
);

/* Structure for getting information about a position. */
struct parsinfo {
    const char *begin; /* pointer to the first char of the line */
    const char *end;   /* pointer to the char just after the end of the line */
    size_t length;     /* length of the line (== end-begin) */ 
    int lino;          /* number of the line within the buffer */
    int colno;         /* number of the column within the line */
};

/*
  Fill the data of 'info' for the current position 'pos' that is an offset
  into the buffer of 'parsing'.
  This function computes into info the pointers of the line containig the
  char of offset 'pos', the number of this line and the column number of
  the position within the line.
  Note: works on utf8 data.
*/
void parse_utf8_info(
    struct parsing *parsing,
    struct parsinfo *info,
    size_t pos
);


#endif

