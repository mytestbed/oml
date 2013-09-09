ACLOCAL_AMFLAGS = -I ../m4 -Wnone

AM_CPPFLAGS = \
	-I $(top_srcdir)/lib/client \
	-I $(top_srcdir)/lib/ocomm \
	-I $(top_srcdir)/lib/shared

bin_PROGRAMS = oml2-proxy-server

dist_bin_SCRIPTS = oml2-proxycon

noinst_LTLIBRARIES = libproxyserver-test.la

# Avoid objects ‘created with both libtool and without’ errors [0]
# [0] https://www.gnu.org/software/automake/manual/html_node/Objects-created-both-with-libtool-and-without.html
oml2_proxy_server_CFLAGS = $(AM_CFLAGS)

oml2_proxy_server_SOURCES = \
	oml2-proxy-server.c \
	receiver.c \
	sender.c \
	session.c \
	session.h \
	proxy_client.c \
	proxy_client.h \
	message_queue.c \
	message_queue.h


oml2_proxy_server_LDADD = \
	$(top_builddir)/lib/ocomm/libocomm.la \
	$(top_builddir)/lib/shared/libshared.la \
	$(M_LIBS) $(POPT_LIBS) $(PTHREAD_LIBS)

libproxyserver_test_la_SOURCES = \
	receiver.c \
	proxy_client.c \
	proxy_client.h \
	message_queue.c \
	message_queue.h
