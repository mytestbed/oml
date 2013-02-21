ACLOCAL_AMFLAGS = -I ../../m4 -Wnone
AM_TESTS_ENVIRONMENT = CK_VERBOSITY=verbose

if HAVE_CHECK
TESTS = check_server
if HAVE_RUBY_MIN_1_8_7
TESTS += msggen.rb
if HAVE_RUBY_MIN_1_9
TESTS += issue610.rb
endif
endif
TESTS_ENVIRONMENT=TOPBUILDDIR=$(top_builddir)
check_PROGRAMS = msgloop check_server

msgloop_SOURCES = \
	msgloop.c \
	$(top_srcdir)/proxy_server/receiver.c \
	$(top_srcdir)/proxy_server/proxy_client.c \
	$(top_srcdir)/proxy_server/proxy_client.h \
	$(top_srcdir)/proxy_server/message_queue.c \
	$(top_srcdir)/proxy_server/message_queue.h \
	$(top_srcdir)/lib/client/oml2/omlc.h \
	$(top_srcdir)/lib/client/oml2/oml_filter.h

check_server_SOURCES = \
	check_server.c \
	check_server.h \
	check_server_suites.h \
	check_text_protocol.c \
	check_binary_protocol.c \
	$(top_srcdir)/lib/shared/mem.h \
	$(top_srcdir)/lib/shared/mbuf.h \
	$(top_srcdir)/server/client_handler.c \
	$(top_srcdir)/server/hook.c \
	$(top_srcdir)/server/hook.h \
	$(top_srcdir)/server/sqlite_adapter.c \
	$(top_srcdir)/server/sqlite_adapter.h \
	$(top_srcdir)/server/database_adapter.c \
	$(top_srcdir)/server/database_adapter.h \
	$(top_srcdir)/server/database.c \
	$(top_srcdir)/server/database.h \
	$(top_srcdir)/server/table_descr.c \
	$(top_srcdir)/server/table_descr.h

msgloop_LDADD = \
	$(top_builddir)/lib/shared/libshared.la \
	$(top_builddir)/lib/ocomm/libocomm.la
check_server_CFLAGS = @CHECK_CFLAGS@ -UHAVE_CONFIG_H

check_server_LDADD = @CHECK_LIBS@ @SQLITE3_LIBS@ \
	$(top_builddir)/lib/shared/libshared.la \
	$(top_builddir)/lib/ocomm/libocomm.la

endif

AM_CPPFLAGS = \
	-I $(top_srcdir)/proxy_server \
	-I $(top_srcdir)/lib/client \
	-I $(top_srcdir)/lib/ocomm \
	-I $(top_srcdir)/lib/shared \
	-I $(top_srcdir)/server \
	-I $(srcdir)/../system \
	-D PKG_LOCAL_STATE_DIR=\".\" \
	-DOML_TEST

EXTRA_DIST = msggen.rb issue610.rb.in
CLEANFILES = \
	check_server_oml.log \
	dummy.bin \
	log.txt \
	text-test.sq3 \
	text-test.sq3-journal \
	text-flex-test.sq3 \
	text-flex-test.sq3-journal \
	text-meta-test.sq3 \
	text-meta-test.sq3-journal \
	binary-resync-test.sq3 \
	binary-resync-test.sq3-journal \
	binary-flex-test.sq3 \
	binary-flex-test.sq3-journal \
	binary-meta-test.sq3 \
	binary-meta-test.sq3-journal \
	issue610.sq3 \
	issue610.sq3-journal
