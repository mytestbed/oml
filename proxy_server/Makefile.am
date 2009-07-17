ACLOCAL_AMFLAGS = -I ../m4 -Wnone

AM_CPPFLAGS = \
	-I $(top_srcdir)/client/src/c \
	-I $(top_srcdir)/external/ocommLib \
	-I $(top_srcdir)/shared/src/c

bin_PROGRAMS = oml2-proxy-server

oml2_proxy_server_SOURCES = \
	src/c/main.c \
	src/c/proxy_client_handler.c \
	src/c/proxy_client_handler.h \
	src/c/proxy_server.h \
	src/c/version.h

oml2_proxy_server_LDADD = \
	$(top_builddir)/external/ocommLib/libocomm.la \
	$(top_builddir)/shared/libshared.la \
	-lpopt

