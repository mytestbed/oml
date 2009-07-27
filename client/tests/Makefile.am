ACLOCAL_AMFLAGS = -I ../../m4 -Wnone

TESTS = check_liboml2

check_PROGRAMS = check_liboml2
check_liboml2_SOURCES = \
	check_liboml2.c \
	check_liboml2_filters.c \
	check_liboml2_parser.c \
	$(top_srcdir)/client/src/c/oml2/omlc.h \
	$(top_srcdir)/client/src/c/oml2/oml_filter.h

AM_CPPFLAGS = \
	-I  $(top_srcdir)/client/src/c

check_liboml2_CFLAGS = @CHECK_CFLAGS@

check_liboml2_LDADD = $(top_builddir)/client/liboml2.la @CHECK_LIBS@