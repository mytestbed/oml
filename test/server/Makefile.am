ACLOCAL_AMFLAGS = -I ../../m4 -Wnone

if HAVE_CHECK
TESTS = msggen.rb check_server
if HAVE_RUBY_MIN_1_9
TESTS += issue610.rb
endif
TESTS_ENVIRONMENT=TOPBUILDDIR=$(top_builddir)
check_PROGRAMS = msgloop check_server

msgloop_SOURCES = \
	msgloop.c \
	$(top_srcdir)/proxy_server/receiver.c \
	$(top_srcdir)/proxy_server/client.c \
	$(top_srcdir)/proxy_server/client.h \
	$(top_srcdir)/proxy_server/message_queue.c \
	$(top_srcdir)/proxy_server/message_queue.h \
	$(top_srcdir)/lib/client/oml2/omlc.h \
	$(top_srcdir)/lib/client/oml2/oml_filter.h

check_server_SOURCES = \
	check_server.c \
	check_server_suites.h \
	check_text_protocol.c \
	$(top_srcdir)/lib/shared/mem.h \
	$(top_srcdir)/lib/shared/mbuf.h \
	$(top_srcdir)/server/client_handler.c \
	$(top_srcdir)/server/sqlite_adapter.c \
	$(top_srcdir)/server/sqlite_adapter.h \
	$(top_srcdir)/server/database.c \
	$(top_srcdir)/server/database.h \
	$(top_srcdir)/server/table_descr.c \
	$(top_srcdir)/server/table_descr.h

msgloop_LDADD = \
	$(top_builddir)/lib/shared/libshared.la \
	$(top_builddir)/lib/ocomm/libocomm.la
check_server_CFLAGS = @CHECK_CFLAGS@

check_server_LDADD = @CHECK_LIBS@ -lsqlite3 \
	$(top_builddir)/lib/shared/libshared.la \
	$(top_builddir)/lib/ocomm/libocomm.la

endif

AM_CPPFLAGS = \
	-I $(top_srcdir)/proxy_server \
	-I  $(top_srcdir)/lib/client \
	-I  $(top_srcdir)/lib/ocomm \
	-I  $(top_srcdir)/lib/shared \
	-I  $(top_srcdir)/server \
	-I  $(srcdir)/../system \
	-DOML_TEST

EXTRA_DIST = msggen.rb issue610.rb.in
CLEANFILES = \
	check_oml2.log \
	dummy.bin \
	log.txt \
	text-test.sq3 \
	issue610.sq3
