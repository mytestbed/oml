ACLOCAL_AMFLAGS = -I ../m4 -Wnone

AM_CPPFLAGS = \
	-I $(top_srcdir)/client/src/c \
	-I $(top_srcdir)/external/ocommLib \
	-I $(top_srcdir)/shared/src/c

bin_PROGRAMS = oml2-server

oml2_server_SOURCES = \
	src/c/main.c \
	src/c/client_handler.c \
	src/c/client_handler.h \
	src/c/database.c \
	src/c/database.h \
	src/c/server.h \
	src/c/sqlite_adapter.c \
	src/c/sqlite_adapter.h \
	src/c/util.c \
	src/c/util.h \
	src/c/table_descr.c \
	src/c/table_descr.h \
	src/c/version.h

oml2_server_LDADD = \
	$(top_builddir)/external/ocommLib/libocomm.la \
	$(top_builddir)/shared/libshared.la \
	-lpopt -lsqlite3


