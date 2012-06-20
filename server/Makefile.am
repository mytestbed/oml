ACLOCAL_AMFLAGS = -I ../m4 -Wnone

AM_CPPFLAGS = \
	-I $(top_srcdir)/lib/client \
	-I $(top_srcdir)/lib/ocomm \
	-I $(top_srcdir)/lib/shared \
	-DLOCAL_STATE_DIR=\"$(localstatedir)\" \
	-DPKG_LOCAL_STATE_DIR=\"$(pkglocalstatedir)\"

bin_PROGRAMS = oml2-server

oml2_server_SOURCES = \
	main.c \
	client_handler.c \
	client_handler.h \
	database.c \
	database.h \
	hook.c \
	hook.h \
	sqlite_adapter.c \
	sqlite_adapter.h \
	table_descr.c \
	table_descr.h \
	version.h

sharedir = $(datadir)/@PACKAGE@
dist_share_DATA = oml2-server-hook.sh

if HAVE_PG
oml2_server_SOURCES += psql_adapter.c psql_adapter.h
else
oml2_server_SOURCES +=
endif

oml2_server_CFLAGS = @LIBPQ_CFLAGS@

oml2_server_LDFLAGS = @LIBPQ_LDFLAGS@

oml2_server_LDADD = \
	$(top_builddir)/lib/ocomm/libocomm.la \
	$(top_builddir)/lib/shared/libshared.la \
	-lpopt -lsqlite3 @LIBPQ_LIBS@

install-data-hook:
	$(install_sh) -d -m 755 $(DESTDIR)$(pkglocalstatedir)

uninstall-hook:
	rm -rf $(DESTDIR)$(pkglocalstatedir)

