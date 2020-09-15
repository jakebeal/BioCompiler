if WANT_CPPUNIT
# Rules for unit tests
INCLUDES = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/shared \
	-I$(top_srcdir)/src/compiler

TESTS = unittests
bin_PROGRAMS = $(TESTS)
noinst_PROGRAMS = $(TESTS)
unittests_SOURCES= Main.cpp TypeCheckingTestCase.cpp TypeCheckingTestCase.h
unittests_CXXFLAGS = $(CPPUNIT_CFLAGS)
unittests_LDADD = $(CPPUNIT_LIBS) ../../shared/libshared.la ../libcompiler.la
unittests_LDFLAGS = -ldl
endif
