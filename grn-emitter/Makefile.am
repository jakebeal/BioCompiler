SUBDIRS = \
	lib \
	optimizers \
	outputs

AM_CPPFLAGS = \
        -I$(top_srcdir)/optimizers

pkglib_LTLIBRARIES = libbiocompiler.la
libbiocompiler_la_SOURCES = \
	BioCompilerPlugin.cpp \
	grn.cpp \
	grn_utilities.cpp \
	biocompiler.cpp

libbiocompiler_la_LIBADD = \
        optimizers/liboptimize.la \
        outputs/liboutputs.la

libbiocompiler_la_LDFLAGS = -dynamiclib -avoid-version -lsbol #-lprotosimplugin

registrydir="${prefix}/lib/proto/"

install-data-hook:
	$(registrydir)/buildregistry

