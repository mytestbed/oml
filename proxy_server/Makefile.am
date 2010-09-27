ACLOCAL_AMFLAGS = -I ../m4 -Wnone

AM_CPPFLAGS = \
	-I $(top_srcdir)/lib/client \
	-I $(top_srcdir)/lib/ocomm \
	-I $(top_srcdir)/lib/shared

bin_PROGRAMS = oml2-proxy-server

oml2_proxy_server_SOURCES = \
	main.c \
	sm.c \
	sender.c \
	sender.h \
	session.c \
	session.h \
	client.c \
	client.h \
	message_queue.c \
	message_queue.h \
	proxy_client_handler.c \
	proxy_client_handler.h \
	version.h

oml2_proxy_server_LDADD = \
	$(top_builddir)/lib/ocomm/libocomm.la \
	$(top_builddir)/lib/shared/libshared.la \
	-lpopt
