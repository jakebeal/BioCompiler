pkglib_LTLIBRARIES = libexampleplugin.la
libexampleplugin_la_SOURCES = \
	ExampleDistribution.cpp \
	ExampleTimeModel.cpp \
	ExampleLayer.cpp \
	ExamplePlugin.cpp
libexampleplugin_la_LDFLAGS = -dynamiclib -avoid-version -lprotosimplugin

registrydir="${prefix}/lib/proto/"

install-data-hook:
	$(registrydir)/buildregistry
