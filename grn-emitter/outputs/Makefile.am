noinst_LTLIBRARIES = liboutputs.la

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libSBOLc/include

liboutputs_la_LDFLAGS = -dynamiclib -avoid-version

liboutputs_la_SOURCES = \
	cheapo-xml.cpp \
	to-matlab.cpp \
	to-sbol.cpp \
	to-dot.cpp
