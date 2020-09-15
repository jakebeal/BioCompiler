AC_DEFUN([PROTO_WITHOUT_GC], [
    AC_ARG_ENABLE(gc, [AS_HELP_STRING([--enable-gc], [Turn garbage collection in the VM ON]),
AS_HELP_STRING([--disable-gc], [Turn garbage collection in the VM OFF])])

    if test "x$enable_gc" != xno; then
        AC_DEFINE([GC_COMP], 1,
                  [Define if you want to turn Garbage Collection in the VM on])
    fi
])
