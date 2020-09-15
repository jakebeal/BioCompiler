#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.60)

# This should track the subversion version number automatically
define([svnversion], esyscmd([sh -c "svnversion|tr -d '\n'"]))dnl
AC_INIT([proto], [svnversion], [biotools@bbn.com])
AC_DEFINE(SVN_REVISION, "svnversion", [SVN Revision])

AC_CONFIG_SRCDIR([.])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([config])
AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE

AC_PROG_LIBTOOL
# Compiler stuff

AC_PROG_CXX
AC_LANG(C++)

## Checks for programs.

AC_PROG_MAKE_SET

## options for the user

PROTO_WITH_GLUT

## directories

BIOCOMPILER_DIRS

## Checks for header files.

# TODO: these are unused
AC_HEADER_STDC

## Checks for typedefs, structures, and compiler characteristics.

AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

## Checks for library functions.

AC_CHECK_LIB([dl], [dlopen])
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([floor gettimeofday memset pow select sqrt strcasecmp strrchr])

AC_CONFIG_FILES([Makefile
		lib/Makefile
		optimizers/Makefile
		outputs/Makefile
                ])
AC_OUTPUT
