ACLOCAL_AMFLAGS = -I ../m4 -Wnone

SCAFFOLD = ruby ../ruby/oml2-scaffold

pkgdatadir = $(datadir)/$(installed_name)
# The name under which this binary will be installed after applying the
# transform.
installed_name = `echo '$(PACKAGE)' | $(SED) '$(transform)'`

AM_CPPFLAGS = \
	-I $(top_srcdir)/lib/client \
	-I $(top_srcdir)/lib/ocomm \
	-I $(top_srcdir)/lib/shared \
	-DLOCAL_STATE_DIR=\"$(localstatedir)\" \
	-DPKG_LOCAL_STATE_DIR=\"$(pkglocalstatedir)\"

bin_PROGRAMS = oml2-server

noinst_LTLIBRARIES = libserver-test.la

oml2_server_SOURCES = \
	oml2-server.c \
	oml2-server_oml.h \
	client_handler.c \
	client_handler.h \
	database.c \
	database.h \
	hook.c \
	hook.h \
	database_adapter.c \
	database_adapter.h \
	monitoring_server.c \
	monitoring_server.h \
	sqlite_adapter.c \
	sqlite_adapter.h \
	table_descr.c \
	table_descr.h

libserver_test_la_CPPFLAGS = $(AM_CPPFLAGS) -UHAVE_CONFIG_H -DNOOML
libserver_test_la_SOURCES = \
			    client_handler.c \
			    hook.c \
			    hook.h \
			    sqlite_adapter.c \
			    sqlite_adapter.h \
			    database_adapter.c \
			    database_adapter.h \
			    database.c \
			    database.h \
			    table_descr.c \
			    table_descr.h

BUILT_SOURCES = oml2-server.rb \
		oml2-server_oml.h

CLEANFILES = $(BUILT_SOURCES)

pkgdata_DATA = oml2-server.rb

EXTRA_DIST = oml2-server.rb.in

if HAVE_LIBPQ
oml2_server_SOURCES += psql_adapter.c psql_adapter.h
oml2_server_LDFLAGS = $(PQLIBPATH)
endif

oml2_server_CPPFLAGS = $(PQINCPATH) $(AM_CPPFLAGS)

oml2_server_LDADD = \
	$(top_builddir)/lib/client/liboml2.la \
	$(top_builddir)/lib/ocomm/libocomm.la \
	$(top_builddir)/lib/shared/libshared.la \
	$(M_LIBS) $(POPT_LIBS) $(SQLITE3_LIBS) $(LIBPQ_LIBS)

oml2-server_oml.h: oml2-server.rb
	$(SCAFFOLD) --oml $<

oml2-server.rb: oml2-server.rb.in
	VER=`echo $(VERSION) | $(SED) "s/.*+oml//;s/-dirty//;s/\([0-9]\{1,\}\(\.[0-9]\{1,\}\)\{1,2\}\).*/\1/;s/\./, /g"`; \
	    $(SED) -e 's|@bindir[@]|$(bindir)|g' \
		-e "/app\.path/s/oml2-server/$(installed_name)/" \
		-e "s/app\.version.*/app.version($$VER)/" \
		$< > $@

install-data-hook:
	$(install_sh) -d -m 755 $(DESTDIR)$(pkglocalstatedir)

uninstall-hook:
	rm -rf $(DESTDIR)$(pkglocalstatedir)
	rm -rf $(DESTDIR)$(pkgdatadir)/oml2-server.rb
