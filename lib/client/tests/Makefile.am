ACLOCAL_AMFLAGS = -I ../../m4 -Wnone

TESTS = check_liboml2

check_PROGRAMS = check_liboml2
check_liboml2_SOURCES = \
	util.c \
	util.h \
	check_liboml2.c \
	check_liboml2_api.c \
	check_liboml2_filters.c \
	check_liboml2_parser.c \
	$(top_srcdir)/lib/client/oml2/omlc.h \
	$(top_srcdir)/lib/client/oml2/oml_filter.h

AM_CPPFLAGS = \
	-I  $(top_srcdir)/lib/client \
	-I  $(top_srcdir)/lib/ocomm/

check_liboml2_CFLAGS = @CHECK_CFLAGS@

check_liboml2_LDADD = $(top_builddir)/lib/client/liboml2.la @CHECK_LIBS@

BUILT_SOURCES = \
	stddev_0.c \
	stddev_1.c

CLEANFILES = \
	stddev_0.c \
	stddev_1.c

STDDEV = $(srcdir)/stddev.py

stddev_0.c stddev_1.c:	Makefile
	$(STDDEV)