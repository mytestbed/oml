ACLOCAL_AMFLAGS = -I ../m4 -Wnone

AM_CPPFLAGS = \
	-I $(top_srcdir)/lib/shared \
	-I $(top_srcdir)/lib/client \
	-I $(top_srcdir)/lib/ocomm

noinst_LTLIBRARIES = libshared.la

libshared_la_SOURCES = \
	marshal.c \
	marshal.h \
	mbuf.c \
	mbuf.h \
	mstring.c \
	mstring.h \
	oml_value.c \
	oml_value.h

