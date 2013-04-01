ACLOCAL_AMFLAGS = -I ../../m4 -Wnone
AM_TESTS_ENVIRONMENT = CK_VERBOSITY=verbose

if HAVE_CHECK
TESTS = check_liboml2 check_libshared
check_PROGRAMS = check_liboml2 check_libshared

AM_CPPFLAGS = \
	-I  $(top_srcdir)/lib/client \
	-I  $(top_srcdir)/lib/ocomm \
	-I  $(top_srcdir)/lib/shared

check_liboml2_SOURCES = \
	check_util.c \
	check_util.h \
	check_liboml2.c \
	check_liboml2_api.c \
	check_liboml2_base64.c \
	check_liboml2_bswap.c \
	check_liboml2_cbuf.c \
	check_liboml2_filters.c \
	check_liboml2_log.c \
	check_liboml2_mbuf.c \
	check_liboml2_omlvalue.c \
	check_liboml2_suites.h \
	check_liboml2_string_utils.c \
	check_liboml2_writers.c \
	$(top_srcdir)/lib/client/oml2/omlc.h \
	$(top_srcdir)/lib/client/oml2/oml_filter.h

check_libshared_SOURCES = \
	check_util.c \
	check_util.h \
	check_libshared.c \
	check_libshared_suites.h \
	check_libshared_mstring.c \
	check_libshared_util.c \
	check_libshared_headers.c \
	check_libshared_marshal.c

check_liboml2_CFLAGS = $(CHECK_CFLAGS)
check_libshared_CFLAGS = $(CHECK_CFLAGS)

check_liboml2_LDADD = $(CHECK_LIBS) $(XML2_LIBS) $(M_LIBS) \
	$(top_builddir)/lib/client/liboml2.la \
	$(top_builddir)/lib/ocomm/libocomm.la

check_libshared_LDADD = $(CHECK_LIBS) $(M_LIBS) \
	$(top_builddir)/lib/shared/libshared.la \
	$(top_builddir)/lib/ocomm/libocomm.la

endif

BUILT_SOURCES = \
	stddev_0.c \
	stddev_1.c

CLEANFILES = \
	stddev_0.c \
	stddev_1.c \
	check_liboml2_oml.log \
	check_liboml2_log.log \
	check_libshared_oml.log \
	test_api_basic \
	test_api_metadata \
	test_fw_create_buffered

STDDEV = $(srcdir)/stddev.py

dist_check_SCRIPTS=$(STDDEV)

stddev_0.c stddev_1.c:	Makefile $(STDDEV)
	$(STDDEV)
