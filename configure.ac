#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([tizen-platform-wrapper], [2.0])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_SRCDIR([src/parser.h])
AC_CONFIG_HEADERS([src/config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.

# Checks for header files.
AC_FUNC_ALLOCA
AC_CHECK_HEADERS([fcntl.h stdint.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_CHECK_FUNCS([dup2 endpwent munmap strndup])

# Add option to build support for the "SYSROOT" variable
AC_ARG_WITH(SYSROOT, [--with-sysroot enable SYSROOT])
if test "$with_sysroot" == "yes"
then
    AC_DEFINE([HAVE_SYSROOT], [1], [Add SYSROOT to Tizen variables.])
    AC_MSG_NOTICE([with-sysroot=yes])
else
    AC_MSG_NOTICE([with-sysroot=no])
fi

AC_CONFIG_FILES([
Makefile
src/Makefile
tizen-platform-wrapper.pc:tizen-platform-wrapper.pc.in])
AC_OUTPUT
