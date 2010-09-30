ACLOCAL_AMFLAGS = -I ../../m4 -Wnone

TESTS = msggen.rb
check_PROGRAMS = msgloop

msgloop_SOURCES = \
	msgloop.c
	$(top_srcdir)/proxy_server/proxy_client_handler.c \
	$(top_srcdir)/lib/client/oml2/omlc.h \
	$(top_srcdir)/lib/client/oml2/oml_filter.h

msgloop_LDADD = \
	$(top_builddir)/lib/shared/libshared.la

AM_CPPFLAGS = \
	-I  $(top_srcdir)/lib/client \
	-I  $(top_srcdir)/lib/ocomm \
	-I  $(top_srcdir)/lib/shared \
	-I $(top_srcdir)/server \
	-I $(top_srcdir)/proxy_server

CLEANFILES = \
	check_oml2.log
