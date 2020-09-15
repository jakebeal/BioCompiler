#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.60)

# increment the version number here on each release, so that the
# version number in the source-code repo is one *more* than the
# latest release.
AC_INIT([Proto], [8], [proto-bugs@csail.mit.edu])

AC_CONFIG_SRCDIR([src/compiler/compiler.cpp])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([config])
AC_CONFIG_HEADER([src/config.h])

AM_INIT_AUTOMAKE

# CPPUnit
AM_PATH_CPPUNIT(1.9.6)

AC_PROG_LIBTOOL
# Compiler stuff

AC_PROG_CXX
AC_LANG(C++)

## Checks for programs.

AC_PROG_MAKE_SET
AC_PROG_LEX
AX_WITH_PYTHON([echo "no python found"; exit 1; : ]) # only needed for tests

## options for the user

PROTO_WITH_GLUT
PROTO_WITH_NEOCOMPILER
PROTO_WITHOUT_GC
PROTO_WITH_SIMULATOR_IN_BROWSER

## check for doxygen

BB_ENABLE_DOXYGEN

## directories

PROTO_PROTOLIBDIR

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
                 lib/core/Makefile
                 lib/neo/Makefile
                 lib/paleo/Makefile
                 p2b
                 proto
                 src/Makefile
                 src/compiler/Makefile
                 src/compiler/unittests/Makefile
                 src/core_plugins/Makefile
                 src/vm/Makefile
                 src/shared/Makefile
                 src/sim/Makefile
                 src/tests/Makefile
                 doc/Makefile
                 doc/Doxyfile
                 doc/html/Makefile
                ])

# Make proto and p2b scripts executable.

AC_CONFIG_COMMANDS([make-proto-executable], [chmod +x ./proto])
AC_CONFIG_COMMANDS([make-p2b-executable], [chmod +x ./p2b])

AC_OUTPUT
