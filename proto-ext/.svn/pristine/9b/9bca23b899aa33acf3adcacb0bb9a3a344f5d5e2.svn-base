noinst_LTLIBRARIES = libshared.la

INCLUDES = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/compiler

libshared_la_LDFLAGS = -lltdl -export-dynamic

libshared_la_SOURCES = \
	utils.cpp \
	plugin_manager.cpp
if WANT_GLUT
libshared_la_SOURCES += \
        drawing_primitives.cpp \
        Trackball.cpp \
        palette.cpp \
        visualizer.cpp
endif

# Note: headers don't need gl/glut-conditional install
#  those that use graphics info manage conditional internally
pkginclude_HEADERS = \
	utils.h \
	proto_plugin.h \
	Trackball.h \
	drawing_primitives.h \
	palette.h \
	proto_opcodes.h \
	opcodes.def \
	instructions.def \
	visualizer.h
