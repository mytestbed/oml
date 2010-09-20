ACLOCAL_AMFLAGS = -I ../../m4 -Wnone

if HAVE_CHECK
TESTS = check_liboml2 check_server
check_PROGRAMS = check_liboml2 check_server

check_liboml2_SOURCES = \
	check_util.c \
	check_util.h \
	check_liboml2.c \
	check_liboml2_api.c \
	check_liboml2_filters.c \
	check_liboml2_parser.c \
	check_liboml2_bswap.c \
	check_liboml2_marshal.c \
	check_liboml2_mbuf.c \
	check_liboml2_suites.h \
	$(top_srcdir)/lib/client/oml2/omlc.h \
	$(top_srcdir)/lib/client/oml2/oml_filter.h

check_server_SOURCES = \
	check_util.c \
	check_util.h \
	check_server.c \
	check_server_suites.h \
	check_server_headers.c \
	$(top_srcdir)/lib/shared/mem.c \
	$(top_srcdir)/lib/shared/mem.h \
	$(top_srcdir)/server/util.c \
	$(top_srcdir)/server/util.h \
	$(top_srcdir)/server/headers.c \
	$(top_srcdir)/server/headers.h \
	$(top_srcdir)/server/schema.c \
	$(top_srcdir)/server/schema.h \
	$(top_srcdir)/server/binary.c \
	$(top_srcdir)/server/binary.h \
	$(top_srcdir)/server/text.c \
	$(top_srcdir)/server/text.h


check_liboml2_CFLAGS = @CHECK_CFLAGS@
check_server_CFLAGS = @CHECK_CFLAGS@

check_liboml2_LDADD = \
	$(top_builddir)/lib/client/liboml2.la \
	$(top_builddir)/lib/ocomm/libocomm.la \
	@CHECK_LIBS@

check_server_LDADD = @CHECK_LIBS@ \
	$(top_builddir)/lib/shared/libshared.la \
	$(top_builddir)/lib/ocomm/libocomm.la

endif

AM_CPPFLAGS = \
	-I  $(top_srcdir)/lib/client \
	-I  $(top_srcdir)/lib/ocomm \
	-I  $(top_srcdir)/lib/shared \
	-I $(top_srcdir)/server

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