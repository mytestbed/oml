ACLOCAL_AMFLAGS = -I ../../m4 -Wnone

TESTS = msggen.rb
EXTRA_DIST = msggen.rb
check_PROGRAMS = msgloop

msgloop_SOURCES = \
	msgloop.c \
	$(top_srcdir)/proxy_server/receiver.c \
	$(top_srcdir)/proxy_server/client.c \
	$(top_srcdir)/proxy_server/client.h \
	$(top_srcdir)/proxy_server/message_queue.c \
	$(top_srcdir)/proxy_server/message_queue.h \
	$(top_srcdir)/lib/client/oml2/omlc.h \
	$(top_srcdir)/lib/client/oml2/oml_filter.h

msgloop_LDADD = \
	$(top_builddir)/lib/shared/libshared.la \
	$(top_builddir)/lib/ocomm/libocomm.la

AM_CPPFLAGS = \
	-I $(top_srcdir)/proxy_server \
	-I  $(top_srcdir)/lib/client \
	-I  $(top_srcdir)/lib/ocomm \
	-I  $(top_srcdir)/lib/shared \
	-I $(top_srcdir)/server \
	-DOML_TEST

CLEANFILES = \
	check_oml2.log \
	dummy.bin \
	log.txt
