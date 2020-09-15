AC_DEFUN([BIOCOMPILER_DIRS], [
    biocompilerdir="${datarootdir}/proto/platforms/biocompiler/"
    AC_SUBST(biocompilerdir)
    AC_DEFINE_DIR([BIOCOMPILERDIR], [biocompilerdir], [default directory for biocopmiler .proto and image files])
])
