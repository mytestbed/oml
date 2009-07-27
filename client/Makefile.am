ACLOCAL_AMFLAGS = -I ../m4 -Wnone

SUBDIRS = . tests

oml2incdir = $(includedir)/oml2

oml2inc_HEADERS = \
	src/c/oml2/o_log.h \
	src/c/oml2/omlc.h \
	src/c/oml2/oml_filter.h \
	src/c/oml2/oml.h \
	src/c/oml2/oml_writer.h

AM_CPPFLAGS = \
	-I $(top_srcdir)/client/src/c \
	-I $(top_srcdir)/external/ocommLib \
	-I $(top_srcdir)/shared/src/c \
	$(LIBXML2_CFLAGS)

lib_LTLIBRARIES = liboml2.la

liboml2_la_SOURCES = src/c/api.c \
	src/c/client.h \
	src/c/file_writer.c \
	src/c/filter.c \
	src/c/init.c \
	src/c/misc.c \
	src/c/net_writer.c \
	src/c/parse_config.c \
	src/c/version.h \
	src/c/writer.h \
	src/c/filter/factory.c \
	src/c/filter/factory.h \
	src/c/filter/first_filter.c \
	src/c/filter/average_filter.c \
	src/c/filter/histogram_filter.c \
	src/c/filter/first_filter.h \
	src/c/filter/average_filter.h \
	src/c/filter/histogram_filter.h \
	src/c/oml2/o_log.h \
	src/c/oml2/omlc.h \
	src/c/oml2/oml_filter.h \
	src/c/oml2/oml.h \
	src/c/oml2/oml_writer.h

liboml2_la_LIBADD = \
	$(top_builddir)/external/ocommLib/libocomm.la \
	$(top_builddir)/shared/libshared.la \
	$(LIBXML2_LIBS) -lpthread -lpopt

liboml2_la_LDFLAGS = -version-info 0:0:0

script_src = $(top_srcdir)/client/src/ruby/oml2_scaffold

bin_SCRIPTS = oml2_scaffold
CLEANFILES = $(bin_SCRIPTS)
EXTRA_DIST = $(script_src)

oml2_scaffold:	$(script_src) Makefile
	cp $(script_src) oml2_scaffold
	chmod +x oml2_scaffold