AC_DEFUN([PROTO_WITH_NEOCOMPILER], [
    AC_ARG_WITH([neocompiler],
                [AS_HELP_STRING([--with-neocompiler],
                   [use new-version Proto compiler])],
                [use_neocompiler=$withval],
#                [use_neocompiler=meh])
                [use_neocompiler="no"])

    if test "x$use_neocompiler" != xno; then
        USE_NEOCOMPILER=true
    else
        USE_NEOCOMPILER=false
    fi

    AM_CONDITIONAL(USE_NEOCOMPILER, $USE_NEOCOMPILER)
    if $USE_NEOCOMPILER; then
        AC_DEFINE([USE_NEOCOMPILER], 1,
                  [Define if you want to use the new-version Proto compiler])
    fi
])
