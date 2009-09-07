ACLOCAL_AMFLAGS = -I ../m4 -Wnone

SUBDIRS = . tests

oml2incdir = $(includedir)/oml2

oml2inc_HEADERS = \
	oml2/omlc.h \
	oml2/oml_filter.h \
	oml2/oml_writer.h

AM_CPPFLAGS = \
	-I $(top_srcdir)/lib/client \
	-I $(top_srcdir)/lib/ocomm \
	-I $(top_srcdir)/lib/shared \
	$(LIBXML2_CFLAGS)

lib_LTLIBRARIES = liboml2.la

liboml2_la_SOURCES = \
	api.c \
	client.h \
	file_writer.c \
	filter.c \
	init.c \
	misc.c \
	net_writer.c \
	parse_config.c \
	version.h \
	writer.h \
	filter/factory.c \
	filter/factory.h \
	filter/first_filter.c \
	filter/average_filter.c \
	filter/histogram_filter.c \
	filter/stddev_filter.c \
	filter/first_filter.h \
	filter/average_filter.h \
	filter/histogram_filter.h \
	filter/stddev_filter.h \
	oml2/omlc.h \
	oml2/oml_filter.h \
	oml2/oml_writer.h

liboml2_la_LIBADD = \
	$(top_builddir)/lib/ocomm/libocomm.la \
	$(top_builddir)/lib/shared/libshared.la \
	$(LIBXML2_LIBS) -lpthread -lpopt

liboml2_la_LDFLAGS = -version-info 0:0:0

script_src = $(srcdir)/ruby/oml2_scaffold

bin_SCRIPTS = oml2_scaffold
CLEANFILES = $(bin_SCRIPTS)
EXTRA_DIST = $(script_src)

oml2_scaffold:	$(script_src) Makefile
	cp $(script_src) oml2_scaffold
	chmod +x oml2_scaffold
