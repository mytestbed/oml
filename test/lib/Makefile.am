ACLOCAL_AMFLAGS = -I ../../m4 -Wnone
AM_TESTS_ENVIRONMENT = CK_VERBOSITY=verbose MALLOC_CHECK_=3
RANDDEV=/dev/urandom

if HAVE_CHECK
TESTS = check_libshared check_liboml2
check_PROGRAMS = check_libshared check_liboml2

AM_CPPFLAGS = \
	-I  $(top_srcdir)/lib/client \
	-I  $(top_srcdir)/lib/ocomm \
	-I  $(top_srcdir)/lib/shared

check_liboml2_SOURCES = \
	check_utils.c \
	check_utils.h \
	check_liboml2.c \
	check_liboml2_api.c \
	check_liboml2_bswap.c \
	check_liboml2_cbuf.c \
	check_liboml2_config.c \
	check_liboml2_filters.c \
	check_liboml2_log.c \
	check_liboml2_mbuf.c \
	check_liboml2_omlvalue.c \
	check_liboml2_suites.h \
	check_liboml2_writers.c \
	$(top_srcdir)/lib/client/oml2/omlc.h \
	$(top_srcdir)/lib/client/oml2/oml_filter.h \
	$(top_srcdir)/lib/client/file_stream.h

check_libshared_SOURCES = \
	check_utils.c \
	check_utils.h \
	check_libshared.c \
	check_libshared_base64.c \
	check_libshared_json.c \
	check_libshared_string_utils.c \
	check_libshared_suites.h \
	check_libshared_mstring.c \
	check_libshared_oml_utils.c \
	check_libshared_headers.c \
	check_libshared_marshal.c

check_liboml2_CFLAGS = @CHECK_CFLAGS@ \
		       @LIBZ_CPPFLAGS@
check_libshared_CFLAGS = @CHECK_CFLAGS@

check_liboml2_LDADD = @CHECK_LIBS@ \
		      @XML2_LIBS@ \
		      @M_LIBS@ \
		      @LIBZ_LIBS@ \
		      $(top_builddir)/lib/client/liboml2.la \
		      $(top_builddir)/lib/ocomm/libocomm.la

check_libshared_LDADD = @CHECK_LIBS@ \
			@M_LIBS@ \
			$(top_builddir)/lib/shared/libshared.la \
			$(top_builddir)/lib/ocomm/libocomm.la

if HAVE_LIBZ
check_libshared_SOURCES += check_libshared_zlib_utils.c
check_libshared_LDADD += @LIBZ_LIBS@
endif

blob:
	dd if=$(RANDDEV) of=$@ bs=10M count=1

endif

BUILT_SOURCES = \
	stddev_0.c \
	stddev_1.c \
	blob

CLEANFILES = \
	stddev_0.c \
	stddev_1.c \
	blob \
	check_liboml2_log.log \
	check_liboml2.oml.log \
	check_libshared.oml.log \
	test_api_basic \
	test_api_metadata \
	test_config_empty_collect.xml \
	test_config_empty_collect \
	test_config_metadata.xml \
	test_config_metadata \
	test_config_multi_collect.xml \
	test_config_multi_collect1 \
	test_config_multi_collect2 \
	test_fw_create_buffered \
	test_zw_create_buffered \
	test_zw_create_buffered.blob

STDDEV = $(srcdir)/stddev.py

dist_check_SCRIPTS=$(STDDEV)

stddev_0.c stddev_1.c:	Makefile $(STDDEV)
	$(STDDEV)
