echo "#define PROTO_VERSION \"`svnversion -c proto/ | awk -F ':' '{ print $NF }'`\"" > proto/src/proto_version.h
echo "#define KERNEL_VERSION \"`svnversion -c proto/src/kernel/ | awk -F ':' '{ print $NF }'`\"" > proto/src/kernel/kernel_version.h
