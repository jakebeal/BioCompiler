AC_DEFUN([PROTO_WITH_GLUT], [
    AC_REQUIRE([AX_CHECK_GLUT])

    AC_ARG_WITH([glut],  dnl funny spacing here gets the output formatting right
[AS_HELP_STRING([--with-glut], [use GLUT])
AS_HELP_STRING([--without-glut], [do not use GLUT, even if available])],
                [use_glut=$withval],
                [use_glut=meh])

    if test "$no_glut" = "yes"; then
        # no GLUT support
        case "$use_glut" in
            yes)
                case "$target" in
                    *-apple-darwin*)
                        AC_MSG_ERROR([No glut support found; consider --with-apple-opengl-framework])
                        ;;
                    *)
                        AC_MSG_ERROR([No glut support found])
                        ;;
                esac
                ;;
            meh)
                WANT_GLUT=false
                AC_MSG_WARN([No glut support found])
                ;;
            no)
                WANT_GLUT=false # everyone's happy
                ;;
        esac
    else
        # GLUT support found
        case "$use_glut" in
            yes|meh)
                WANT_GLUT=true # everyone's happy
                ;;
            no)
                # reset the vars that AX_CHECK_GLUT so carefully discovered
                GLUT_LIBS=
                GLUT_CFLAGS=
                WANT_GLUT=false
                ;;
        esac
    fi

    if $WANT_GLUT; then
        AC_CHECK_HEADERS([windows.h GL/gl.h OpenGL/gl.h])
    fi

    AM_CONDITIONAL(WANT_GLUT, $WANT_GLUT)
    if $WANT_GLUT; then
        AC_DEFINE([WANT_GLUT], 1,
                  [Define if you have a GLUT library available])
    fi

    # FIXME: only include this for the simulator
    CFLAGS="${CFLAGS} ${GLUT_CFLAGS}"
    LIBS="${GLUT_LIBS} ${LIBS}"
])
