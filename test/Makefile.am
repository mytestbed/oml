ACLOCAL_AMFLAGS = -I ../m4 -Wnone

SUBDIRS = lib server system/blob ruby

AM_CPPFLAGS = \
	-I  $(top_srcdir)/lib/client \
	-I  $(top_srcdir)/lib/ocomm \
	-I  $(top_srcdir)/lib/shared

noinst_PROGRAMS = testclient

testclient_SOURCES = testclient.c

testclient_LDADD = $(top_builddir)/lib/client/liboml2.la
