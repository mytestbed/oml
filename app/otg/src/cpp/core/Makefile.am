ACLOCAL_AMFLAGS = -I ../../../../../m4 -Wnone

AM_CPPFLAGS = \
	-I $(top_srcdir)/client/src/c \
	-I $(top_srcdir)/external/ocommLib \
	-I $(srcdir)/..

noinst_LTLIBRARIES = libotg2core.la

libotg2core_la_SOURCES = \
	component.cpp \
	address.cpp \
	flow.cpp \
	packet.cpp \
	stream.cpp \
	unixtime.cpp

libotg2core_la_LIBADD = \
	$(top_builddir)/external/ocommLib/libocomm.la
