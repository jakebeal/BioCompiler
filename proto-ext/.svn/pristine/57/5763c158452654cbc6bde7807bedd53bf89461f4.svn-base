# NOTE: order is important in this list!
SUBDIRS = \
	shared \
	compiler \
	sim \
	vm \
	core_plugins \
	tests \
	.

INCLUDES = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/shared \
	-I$(top_srcdir)/src/kernel \
	-I$(top_srcdir)/src/compiler \
	-I$(top_srcdir)/src/sim \
	-I$(top_srcdir)/src/vm

bin_PROGRAMS = \
	proto \
	p2b

proto_SOURCES = \
	sim-app.cpp

proto_LDADD = \
	shared/libshared.la \
	compiler/libcompiler.la \
	sim/libsim.la \
	sim/libprotosimplugin.la \
	sim/libdefaultplugin.la \
	shared/libshared.la \
	compiler/libcompiler.la

proto_LDFLAGS = \
	-export-dynamic \
	-rpath ${libdir}

p2b_SOURCES = \
	compiler-app.cpp

p2b_LDADD = \
	shared/libshared.la \
	compiler/libcompiler.la \
	shared/libshared.la

p2b_LDFLAGS = \
	-export-dynamic \
	-rpath ${libdir}

pkginclude_HEADERS = \
	proto_version.h

# registry builder:
buildregistrydir=$(plugindir)
buildregistry_SOURCES = \
	createRegistry.cpp
buildregistry_LDADD = \
	shared/libshared.la \
	compiler/libcompiler.la \
	sim/libsim.la \
	sim/libprotosimplugin.la \
	shared/libshared.la \
	compiler/libcompiler.la
buildregistry_LDFLAGS = -lltdl -export-dynamic -rpath ${libdir}
buildregistry_PROGRAMS = \
	buildregistry

# run the registry builder once everything's installed
install-data-hook:
	$(buildregistrydir)/buildregistry$(EXEEXT)
