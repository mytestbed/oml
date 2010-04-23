ACLOCAL_AMFLAGS = -I ../m4 -Wnone

AM_CPPFLAGS = \
	-I $(top_srcdir)/lib/client \
	-I $(top_srcdir)/lib/ocomm \
	-I $(top_srcdir)/lib/shared

bin_PROGRAMS = oml2-server

oml2_server_SOURCES = \
	main.c \
	client_handler.c \
	client_handler.h \
	database.c \
	database.h \
	sqlite_adapter.c \
	sqlite_adapter.h \
	util.c \
	util.h \
	table_descr.c \
	table_descr.h \
	schema.c\
	schema.h \
	version.h

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


