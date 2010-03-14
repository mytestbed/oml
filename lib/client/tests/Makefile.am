ACLOCAL_AMFLAGS = -I ../../m4 -Wnone

if HAVE_CHECK
TESTS = check_liboml2
check_PROGRAMS = check_liboml2
check_liboml2_SOURCES = \
	util.c \
	util.h \
	check_liboml2.c \
	check_liboml2_api.c \
	check_liboml2_filters.c \
	check_liboml2_parser.c \
	check_liboml2_marshal.c \
	check_liboml2_mbuf.c \
	check_liboml2_suites.h \
	$(top_srcdir)/lib/client/oml2/omlc.h \
	$(top_srcdir)/lib/client/oml2/oml_filter.h
check_liboml2_CFLAGS = @CHECK_CFLAGS@

check_liboml2_LDADD = $(top_builddir)/lib/client/liboml2.la @CHECK_LIBS@
endif

noinst_PROGRAMS = testclient

testclient_SOURCES = testclient.c

testclient_LDADD = $(top_builddir)/lib/client/liboml2.la

AM_CPPFLAGS = \
	-I  $(top_srcdir)/lib/client \
	-I  $(top_srcdir)/lib/ocomm \
	-I  $(top_srcdir)/lib/shared

BUILT_SOURCES = \
	stddev_0.c \
	stddev_1.c

CLEANFILES = \
	stddev_0.c \
	stddev_1.c \
	check_oml2.log

STDDEV = $(srcdir)/stddev.py

dist_check_SCRIPTS=$(STDDEV)

stddev_0.c stddev_1.c:	Makefile
	$(STDDEV)