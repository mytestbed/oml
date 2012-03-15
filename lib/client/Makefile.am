ACLOCAL_AMFLAGS = -I ../m4 -Wnone

oml2incdir = $(includedir)/oml2

oml2inc_HEADERS = \
	oml2/omlc.h \
	oml2/oml_filter.h \
	oml2/oml_writer.h \
	oml2/oml_out_stream.h

AM_CPPFLAGS = \
	-I $(top_srcdir)/lib/client \
	-I $(top_srcdir)/lib/ocomm \
	-I $(top_srcdir)/lib/shared \
	$(LIBXML2_CFLAGS)

lib_LTLIBRARIES = liboml2.la

liboml2_la_SOURCES = \
	api.c \
	client.h \
	filter.c \
	init.c \
	misc.c \
	text_writer.c \
	bin_writer.c \
	file_stream.c \
	net_stream.c \
	buffered_writer.c \
	buffered_writer.h \
	parse_config.c \
	version.h \
	filter/factory.c \
	filter/factory.h \
	filter/first_filter.c \
	filter/last_filter.c \
	filter/average_filter.c \
	filter/histogram_filter.c \
	filter/stddev_filter.c \
	filter/sum_filter.c \
	filter/delta_filter.c \
	filter/first_filter.h \
	filter/last_filter.h \
	filter/average_filter.h \
	filter/histogram_filter.h \
	filter/stddev_filter.h \
	filter/sum_filter.h \
	filter/delta_filter.h \
	$(oml2inc_HEADERS)

liboml2_la_LIBADD = \
	$(top_builddir)/lib/ocomm/libocomm.la \
	$(top_builddir)/lib/shared/libshared.la \
	$(LIBXML2_LIBS) -lpthread -lm

# To quote the APR build scripts:
#    "Yes, ${minor}:${patch}:${minor} is correct due to libtool idiosyncrasy"
liboml2_la_LDFLAGS = -version-info $(OML_VER_MINOR):$(OML_VER_PATCH_LT):$(OML_VER_MINOR)

scaffold_src = $(srcdir)/ruby/oml2-scaffold

bin_SCRIPTS = oml2-scaffold
CLEANFILES = $(bin_SCRIPTS)
EXTRA_DIST = $(scaffold_src)

oml2-scaffold:	$(scaffold_src) Makefile
	cp $(scaffold_src) oml2-scaffold
	chmod +x oml2-scaffold

progname=scaffold
installed_name = $(bindir)/oml2-$(progname)
installed_link_name = $(DESTDIR)$(bindir)/oml2_$(progname)
install-exec-hook:
	rm -f $(installed_link_name)
	$(LN_S) $(installed_name) $(installed_link_name)

sharedir = $(datadir)/@PACKAGE@
dist_share_DATA = $(srcdir)/ruby/oml4r.rb \
             $(srcdir)/ruby/oml4r-example.rb
