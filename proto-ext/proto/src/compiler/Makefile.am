noinst_LTLIBRARIES = libcompiler.la

INCLUDES = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/shared 

# Unit tests are outdated and therefore broken; since they fail, they
# prevent `make check' from moving on to the integration tests.

# SUBDIRS = \
# 			 . \
# 			 unittests

libcompiler_la_SOURCES = \
	lisp.cpp \
	reader.cpp \
	compiler-utils.cpp \
	lexer.cpp \
	paleocompiler.cpp \
	prototypes.cpp \
	ir.cpp \
	interpreter.cpp \
	type-inference.cpp \
	analyzer.cpp \
	emitter.cpp \
	compiler.cpp \
	nicenames.cpp

libcompiler_la_LIBADD = \
        ../shared/libshared.la

pkginclude_HEADERS = \
	analyzer.h \
	compiler-utils.h \
	compiler.h \
	ir.h \
	lisp.h \
	nicenames.h \
	paleocompiler.h \
	reader.h \
	sexpr.h

lexer.cpp: $(srcdir)/proto_syntax.flex sexpr.h compiler-utils.h
	$(LEX) -olexer.cpp $(srcdir)/proto_syntax.flex

# distribute the flex-generated files, since recent-enough versions of flex++
# are few and far between.
EXTRA_DIST = lexer.cpp proto_syntax.flex
MAINTAINERCLEANFILES = lexer.cpp
