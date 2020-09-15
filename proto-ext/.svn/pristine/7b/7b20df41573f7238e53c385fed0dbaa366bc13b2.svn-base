AC_DEFUN([PROTO_WITH_SIMULATOR_IN_BROWSER], [
    AC_ARG_WITH([simulator-in-browser],
                [AS_HELP_STRING([--with-simulator-in-browser],
                   [Must be used when build Proto simulator to run in a web browser])],
                [use_simulator_in_browser=$withval],
                [use_simulator_in_browser="no"])

    if test "x$use_simulator_in_browser" != xno; then
        SIMULATOR_IN_BROWSER=true
    else
        SIMULATOR_IN_BROWSER=false
    fi

    AM_CONDITIONAL(SIMULATOR_IN_BROWSER, $SIMULATOR_IN_BROWSER)
    if $SIMULATOR_IN_BROWSER; then
        AC_DEFINE([SIMULATOR_IN_BROWSER], 1,
                  [Define if you are want to run the Proto simulator in a web browser])
    fi
])
