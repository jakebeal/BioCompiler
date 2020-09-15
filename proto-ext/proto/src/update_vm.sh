#!/usr/bin/env sh
curl -L https://github.com/delftproto/delftproto/tarball/master > dpvm.tar.gz || exit 1
tar -xzvf dpvm.tar.gz
rm dpvm.tar.gz
mv vm/Makefile.am vm_Makefile.am
mv vm/.svn vm.svn
mv vm/instructions/.svn vm.instructions.svn
rm -rf vm
dpvm_version=`ls -d delftproto-* | sed -re 's/delftproto-delftproto-(.*)/\1/'`
mv delftproto-* dpvm
cp -r dpvm/vm/src/vm .
echo "#define KERNEL_VERSION \"DelftProto VM $dpvm_version\"" > vm/kernelversion.h
rm -rf dpvm
mv vm_Makefile.am vm/Makefile.am
mv vm.svn vm/.svn
mv vm.instructions.svn vm/instructions/.svn
