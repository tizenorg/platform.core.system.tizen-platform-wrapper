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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdarg.h>

#include "parser.h"
#include "heap.h"
#include "buffer.h"
#include "foreign.h"
#include "sha256sum.h"

/*======================================================================*/

#ifndef CONFIGPATH
# define CONFIGPATH "/etc/tizen-platform.conf"
#endif

#ifndef TOOLNAME
# define TOOLNAME "tzplatform-tool"
#endif

/*== TYPES =============================================================*/

/* for recording read keys */
struct key {
    struct key *next;       /* link to next */
    const char *name;       /* name of the key */
    const char *value;      /* value of the key */
    size_t      begin;      /* positions of begin (used by pretty) */
    size_t      end;        /* positions of end (used by pretty) */
    int         dependant;  /* is dependant (used by rpm) */
};

/* for recording used variables */
struct var {
    struct var *next;       /* link to next */
    const char *name;       /* name of the variable */
    const char *normal;     /* normalized value: "${name}" (for pretty) */
    int         dependant;  /* is dependant (used by rpm) */
};

/*== STATIC DATA =======================================================*/

static char help[] = "\
\n\
usage: "TOOLNAME" [command] [--] [file]\n\
\n\
You can specify the 'file' to process.\n\
The default file is "CONFIGPATH"\n\
Specifying - mean the standard input.\n\
\n\
Commands:\n\
\n\
help       Display this help\n\
check      Check validity of 'file' (this is the default command)\n\
pretty     Pretty print of the 'file' (the normalized format)\n\
h          Produce the C header with the enumeration of the variables\n\
c          Produce the C code to hash the variable names\n\
rpm        Produce the macro file to use with RPM\n\
signup     Produce the signup data for the proxy linked statically\n\
\n\
";

static char genh_head[] = "\
/* I'm generated. Dont edit me! */\n\
#ifndef TZPLATFORM_VARIABLES_H\n\
#define TZPLATFORM_VARIABLES_H\n\
#ifdef __cplusplus\n\
extern \"C\" {\n\
#endif\n\
enum tzplatform_variable {\n\
\t_TZPLATFORM_VARIABLES_INVALID_ = -1,\n\
";

static char genh_tail[] = "\
\t_TZPLATFORM_VARIABLES_COUNT_\n\
};\n\
#ifdef __cplusplus\n\
}\n\
#endif\n\
#endif\n\
";

static char gperf_head[] = "\
struct varassoc {\n\
  int offset;\n\
  int id;\n\
};\n\
%%\n\
";

static char *gperf_command[] = {
    "/bin/sh",
    "-c", 
    "gperf -r -m 100 --null-strings -C -P -L ANSI-C -c"
          " -t -N hashvar -Q varpool -K offset -G -W namassoc"
          " -F \", _TZPLATFORM_VARIABLES_INVALID_\"",
    NULL
};

static char rpm_head[] = "\
# I'm generated. Dont edit me! \n\
\n\
";

static char signup_head[] = "\
/* I'm generated. Dont edit me! */\n\
static char tizen_platform_config_signup[33] = {\n\
    '\\x00',\n\
";

static char signup_tail[] = "\
  };\n\
";

/*== GLOBALS VARIABLES =================================================*/

/* name of the meta file to process */
static const char * metafilepath = CONFIGPATH;

/* list of the read keys */
static struct key *keys = NULL;

/* list of the used variables */
static struct var *vars = NULL;

/* count of errors */
static int errcount = 0;

/* dependency state */
static int dependant = 0;

/* action to perform */
static enum { CHECK, PRETTY, GENC, GENH, RPM, SIGNUP } action = CHECK;

/* output of error */
static int notstderr = 0;

/*======================================================================*/

/* write the error message */
static void vwriterror( const char *format, va_list ap)
{
    vfprintf(notstderr ? stdout : stderr, format, ap);
}

/* write the error message */
static void writerror( const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vwriterror( format, ap);
    va_end(ap);
}

/* write error and exit */
static void fatal( const char *format, ...)
{
    va_list ap;

    writerror("Error, ");
    va_start(ap, format);
    vwriterror( format, ap);
    va_end(ap);
    writerror(".\n");

    /* exit */
    exit(1);
}

/* error in the command line */
static void argerror( const char *format, ...)
{
    va_list ap;

    writerror("Error, ");
    va_start(ap, format);
    vwriterror( format, ap);
    va_end(ap);
    writerror(".\nType '"TOOLNAME" help' to get usage.\n");

    /* exit */
    exit(1);
}

/*== MANAGEMENT OF THE LIST OF READ KEYS ===============================*/

/* search a key of 'name' and length 'lname' and return it or NULL */
static struct key *key_search( const char *name, size_t lname)
{
    struct key *result = keys;

    while(result!=NULL && (strncmp( result->name, name, lname)!=0
                    || result->name[lname]!=0))
        result = result->next;

    return result;
}

/* append a new key to the list and return it or NULL if allocations failed */
static struct key *key_add( const char *name, size_t lname, 
                            const char *value, size_t lvalue,
                            size_t begin_pos, size_t end_pos)
{
    struct key *result, *prev;
    char *sname, *svalue;

    /* allocations */
    result = malloc(sizeof *result);
    sname = strndup( name, lname);
    svalue = strndup( value, lvalue);
    if (result == NULL || sname == NULL || svalue == NULL) {
        /* failure of allocations */
        free( result);
        free( sname);
        free( svalue);
        result = NULL;
    }
    else {
        /* success: init the structure */
        result->next = NULL;
        result->name = sname;
        result->value = svalue;
        result->begin = begin_pos;
        result->end = end_pos;
        result->dependant = dependant;

        /* link at end of the list */
        prev = keys;
        if (prev == NULL)
            keys = result;
        else {
            while(prev!=NULL && prev->next!=NULL)
                prev = prev->next;
            prev->next = result;
        }
    }

    return result;
}

/*======================================================================*/

/* search a var of 'name' and length 'lname' and return it or NULL */
static struct var *var_search( const char *name, size_t lname)
{
    struct var *result = vars;
    while(result!=NULL && (strncmp( result->name, name, lname)!=0
                    || result->name[lname]!=0))
        result = result->next;

    return result;
}

/* append a new var to the list and return it or NULL if allocations failed */
static struct var *var_add( const char *name, size_t lname, 
                                                int depend, const char *value)
{
    struct var *result, *prev;
    char *sname, *normal;
    size_t length;

    /* check for the value */
    if (action == RPM && value != NULL) {
        length = strlen( value) + 1;
    }
    else {
        value = NULL;
        length = lname + 4;
    }
    /* allocations */
    result = malloc(sizeof *result);
    sname = strndup( name, lname);
    normal = malloc( length);
    if (result == NULL || sname == NULL || normal == NULL) {
        /* failure of allocations */
        free( result);
        free( sname);
        free( normal);
        result = NULL;
    }
    else {
        /* success: init the structure */
        result->next = NULL;
        result->name = sname;
        result->normal = normal;
        result->dependant = depend;
        if (value) {
            memcpy( normal, value, length);
        }
        else {
            *normal++ = '$';
            *normal++ = '{';
            memcpy( normal, name, lname);
            normal += lname;
            *normal++ = '}';
            *normal = 0;
        }

        /* link at end of the list */
        prev = vars;
        if (prev == NULL)
            vars = result;
        else {
            while(prev!=NULL && prev->next!=NULL)
                prev = prev->next;
            prev->next = result;
        }
    }

    return result;
}

/*== CALLBACK OF PARSING OF THE META FILE ==============================*/

static void conferr( const char *format, ...)
{
    va_list ap;

    notstderr = action == CHECK;
    writerror( "ERROR ");
    va_start(ap, format);
    vwriterror( format, ap);
    va_end(ap);
    notstderr = 0;
}

static int errcb( struct parsing *parsing,
                size_t pos, const char *message)
{
    struct parsinfo info;

    /* get the info */
    parse_utf8_info( parsing, &info, pos);

    /* emit the error */
    conferr("line %d: %s\n..: %.*s\n..: %*s\n", 
                info.lino, message, 
                (int)info.length, info.begin,
                info.colno, "^"); 

    /* count it */
    errcount++;

    /* continue to parse */
    return 1;
}

static int putcb( struct parsing *parsing, 
                const char *name, size_t lname, 
                const char *value, size_t lvalue,
                size_t begin_pos, size_t end_pos)
{
    enum fkey fkey;
    struct key *key;
    struct parsinfo here, there;

    /* check that it is not a foreign key */
    fkey = foreign( name, lname);
    if (fkey != _FOREIGN_INVALID_) {
        parse_utf8_info( parsing, &here, begin_pos);
        conferr("line %d: reserved variable name '%.*s'\n" 
                "..: %.*s\n..: %*s\n", 
                   here.lino, (int)lname, name,
                   here.lino, (int)here.length, here.begin, here.colno, "^"); 

        errcount++;
        dependant = 0;
        return 0;
    }

    /* search if already defined */
    key = key_search( name, lname);
    if (key != NULL) {

        /* yes! that's an error */
        parse_utf8_info( parsing, &here, begin_pos);
        parse_utf8_info( parsing, &there, key->begin);
        conferr("line %d: redefinition of '%s'\n" 
                "...was defined line %d\n..: %.*s\n..: %*s\n"
                "...is redefined line %d\n..: %.*s\n..: %*s\n", 
                   here.lino, key->name,
                   there.lino, (int)there.length, there.begin, there.colno, "^",
                   here.lino, (int)here.length, here.begin, here.colno, "^"); 

        errcount++;
        dependant = 0;
        return 0;
    }

    /* create and record the key */
    key = key_add( name, lname, value, lvalue, begin_pos, end_pos);
    dependant = 0;
    if (key != NULL)
        return 0;

    /* no can't because of memory! */
    fatal("out of memory");
    errcount++;
    return -1;
}

static const char *getcb( struct parsing *parsing,
                const char *name, size_t length,
                size_t begin_pos, size_t end_pos)
{
    struct parsinfo here;
    struct var *var;
    enum fkey fkey;
    struct key *key;
    int depend;

    /* search if already defined */
    var = var_search( name, length);
    if (var != NULL) {
        /* yes cool, return the normalized form */
        if (var->dependant)
            dependant = 1;
        return var->normal;
    }

    /* search the variable */
    fkey = foreign( name, length);
    key = key_search( name, length);

    if (fkey == _FOREIGN_INVALID_ && key == NULL) {

        /* not a valid variable, emit the error */
        parse_utf8_info( parsing, &here, begin_pos);
        conferr("line %d: use of unknown variable '%.*s'\n"
                "..: %.*s\n..: %*s\n", 
                    here.lino, (int)length, name,
                    (int)here.length, here.begin, here.colno, "^"); 
        errcount++;
        dependant = 1; /* kind of invalidity */
        return "***error***"; /* avoid further error */
    }

    /* valid variables:  those of foreign or the already defined keys */

    /* set dependant state */
    depend = fkey != _FOREIGN_INVALID_ || key->dependant;
    if (depend)
        dependant = 1;

    /* create and record the variable */
    var = var_add( name, length, depend, key==NULL ? NULL : key->value);
    if (var != NULL)
        /* created, return the normalized form */
        return var->normal;

    /* memory depletion */
    fatal("out of memory");
    dependant = 1; /* kind of invalidity */
    errcount++;
    return "***error***"; /* avoid further error */
}

/*======================================================================*/

/* compare two keys */
static int keycmp(const void *a, const void *b)
{
    const struct key *ka = *(const struct key **)a;
    const struct key *kb = *(const struct key **)b;
    return strcmp(ka->name, kb->name);
}

/* sort the keys and return their count */
static int sortkeys()
{
    struct key *key = keys, **array;
    int count = 0, index;

    while (key) {
        key = key->next;
        count++;
    }

    array = malloc( count * sizeof * array);
    if (array == NULL)
        return -1;

    key = keys;
    index = 0;
    
    while (key) {
        array[index++] = key;
        key = key->next;
    }

    qsort(array, count, sizeof * array, keycmp);

    while (index) {
	array[--index]->next = key;
        key = array[index];
    }
    keys = key;
    free( array);

    return count;
}

/*======================================================================*/

/* pretty print the read file */
static int pretty( const char *buffer, size_t length, FILE *output)
{
    int status;
    struct key *key = keys;
    size_t pos = 0;

    while (pos < length && key != NULL) {
        if (pos < key->begin) {
            status = fprintf(output, "%.*s", (int)(key->begin-pos), buffer+pos);
            if (status < 0)
                return status;
        }
        status = fprintf( output, "%s=%s", key->name, key->value);
        if (status < 0)
            return status;
        pos = key->end;
        key = key->next;
    }

    if (pos < length) {
        status = fprintf( output, "%.*s", (int)(length-pos), buffer+pos);
        if (status < 0)
            return status;
    }

    return 0;
}

/* generate the header */
static int genh( FILE *output)
{
    struct key *key;
    int status;

#ifndef NO_SORT_KEYS
    status = sortkeys();
    if (status < 0)
        return status;
#endif

    status = fprintf( output, "%s", genh_head);
    if (status < 0)
        return status;
    for (key = keys ; key != NULL ; key = key->next) {
        status = fprintf( output, "\t%s,\n", key->name);
        if (status < 0)
            return status;
    }
    status = fprintf( output, "%s", genh_tail);
    if (status < 0)
        return status;
    return 0;
}

/* generate hash code using gperf */
static int genc(FILE *output)
{
    struct key *key;
    int fds[2];
    pid_t pid;
    int result, sts;
    size_t l;

#ifndef NO_SORT_KEYS
    sts = sortkeys();
    if (sts < 0)
        return sts;
#endif

    result = pipe(fds);
    if (result != 0)
        return result;

    fflush( output);
    pid = fork();
    if (pid == -1) {
        close( fds[0]);
        close( fds[1]);
        fatal( "can't fork");
        result = -1;
    }
    else if (pid == 0) {
        dup2( fds[0], 0);
        close( fds[0]);
        close( fds[1]);
        if (fileno(output) != 1)
            dup2( fileno(output), 1);
        result = execve( gperf_command[0], gperf_command, environ);
        fatal("can't execute gperf");
        exit(1);
    }
    else {
        close( fds[0]);
        sts = write( fds[1], gperf_head, sizeof(gperf_head)-1);
        if (sts < 0)
            result = sts;
        for (key = keys ; key != NULL ; key = key->next) {
            l = strlen( key->name);
            sts = write( fds[1], key->name, l);
            if (sts < 0)
                result = sts;
            sts = write( fds[1], ", ", 2);
            if (sts < 0)
                result = sts;
            sts = write( fds[1], key->name, l);
            if (sts < 0)
                result = sts;
            sts = write( fds[1], "\n", 1);
            if (sts < 0)
                result = sts;
        }
        close( fds[1]);
        wait(&result);
        sts = WIFEXITED(result) && WEXITSTATUS(result)==0 ? 0 : -1;
        if (sts < 0)
            result = sts;
    }
    return result;
}

/* generate the rpm macros */
static int rpm( FILE *output)
{
    struct key *key;
    int status;

#ifndef NO_SORT_KEYS
    status = sortkeys();
    if (status < 0)
        return status;
#endif

    status = fprintf( output, "%s", rpm_head);
    if (status < 0)
        return status;
    for (key = keys ; key != NULL ; key = key->next) {
        if (!key->dependant) {
            status = fprintf( output, "%%%-40s %s\n", key->name, key->value);
            if (status < 0)
                return status;
        }
    }
    return 0;
}

/* generate the signup */
static int signup( FILE *output)
{
    struct key *key;
    int status;
    int i;
    struct sha256sum *sum;
    char term;
    char signup[32];

#ifndef NO_SORT_KEYS
    status = sortkeys();
    if (status < 0)
        return status;
#endif

    sum = sha256sum_create();
    if (sum == NULL)
        return -1;

    term = ';';
    for (key = keys ; key != NULL ; key = key->next) {
        status = sha256sum_add_data(sum, key->name, strlen(key->name));
        if (status < 0) {
            sha256sum_destroy(sum);
            return status;
        }
        status = sha256sum_add_data(sum, &term, 1);
        if (status < 0) {
            sha256sum_destroy(sum);
            return status;
        }
    }

    status = sha256sum_get(sum, signup);
    sha256sum_destroy(sum);
    if (status < 0)
        return status;

    status = fprintf( output, "%s", signup_head);
    if (status < 0)
        return status;

    for (i=0 ; i<32 ; i++) {
        status = fprintf( output, "%s'\\x%02x'%s",
                    (i & 7) ? " " : "    ",
                    (int)(unsigned char)signup[i],
                    (i & 7) < 7 ? "," : i == 31 ? "\n" : ",\n");
        if (status < 0)
            return status;
    }
    status = fprintf( output, "%s", signup_tail);
    if (status < 0)
        return status;
    return 0;
}

/* main of processing */
static int process()
{
    struct parsing parsing;
    struct buffer buffer;
    int result;

    /* read the file */
    result = buffer_create( &buffer, metafilepath);
    if (result != 0) {
        fatal( "can't read file %s", metafilepath);
        return -1;
    }

    /* parse the file */
    parsing.buffer = buffer.buffer;
    parsing.length = buffer.length;
    parsing.maximum_data_size = 0;
    parsing.should_escape = action!=RPM;
    parsing.data = 0;
    parsing.get = getcb;
    parsing.put = putcb;
    parsing.error = errcb;
    dependant = 0;
    result = parse_utf8_config( &parsing);
    if (result != 0) {
        buffer_destroy( &buffer);
        fatal( "while parsing the file %s", metafilepath);
        return -1;
    }
    if (errcount != 0) {
        buffer_destroy( &buffer);
        fatal( "%d errors detected %s", errcount, metafilepath);
        return -1;
    }

    /* process */
    switch( action) {
    case CHECK:
        break;
    case PRETTY:
        pretty( buffer.buffer, buffer.length, stdout);
        break;
    case GENH:
        genh( stdout);
        break;
    case GENC:
        genc( stdout);
        break;
    case RPM:
        rpm( stdout);
        break;
    case SIGNUP:
        signup( stdout);
        break;
    }

    buffer_destroy( &buffer);
    return 0;
}

/*======================================================================*/

/* very simple argument parsing */
static int arguments( char **argv)
{
    /* skip the program name*/
    argv++;

    /* scan first arg */
    if (*argv == NULL) {
        /* no argument then default is to check */
        action = CHECK;
    }
    else {
        /* check if first argument is 'check', 'pretty', 'c', 'h', '-h',
           '--help' or 'help' */
        if (0 == strcmp( *argv, "check")) {
            action = CHECK;
            argv++;
        }
        else if (0 == strcmp( *argv, "pretty")) {
            action = PRETTY;
            argv++;
        }
        else if (0 == strcmp( *argv, "c")) {
            action = GENC;
            argv++;
        }
        else if (0 == strcmp( *argv, "h")) {
            action = GENH;
            argv++;
        }
        else if (0 == strcmp( *argv, "rpm")) {
            action = RPM;
            argv++;
        }
        else if (0 == strcmp( *argv, "signup")) {
            action = SIGNUP;
            argv++;
        }
        else if (0 == strcmp( *argv, "help") || 0 == strcmp( *argv, "--help")) {
            printf("%s", help);
            exit(0);
            return -1;
        }
        else if (**argv == '-') {
            if (0 == strcmp( *argv, "--") || 0 == strcmp( *argv, "-") ) {
                action = CHECK;
            }
            else {
                argerror( "unknown option '%s'", *argv);
                return -1;
            }
        }
        /* skip the -- arg if present */
        if (*argv != NULL && 0 == strcmp( *argv, "--")) {
            argv++;
        }
        /* get a meta file argument */
        if (*argv != NULL) {
            if (0 == strcmp( *argv, "-"))
                metafilepath = "/dev/stdin";
            else
                metafilepath = *argv;
            argv++;
        }
        /* check that there is no extra argument */
        if (*argv != NULL) {
            argerror("extra argument found '%s'",*argv);
            return -1;
        }   
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (arguments(argv) == 0)
        if (process() == 0)
            return 0;

    return 1;
}


