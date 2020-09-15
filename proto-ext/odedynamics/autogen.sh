#! /bin/bash

# clean up first
rm -f aclocal.m4
rm -rf autom4te*.cache
rm -f configure

# select right libtoolize name:
if [ `uname -s` = Darwin ]
 then
    LIBTOOLIZE=glibtoolize
 else
    LIBTOOLIZE=libtoolize
 fi

# note: these includes should match those in the toplevel Makefile.am
aclocal --force -I . -I config || exit 1
autoconf || exit 1
autoheader || exit 1
$LIBTOOLIZE --automake
automake --add-missing --force --copy || exit 1
