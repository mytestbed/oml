ACLOCAL_AMFLAGS = -I ../../../../../m4 -Wnone

AM_CPPFLAGS = \
	-I $(top_srcdir)/lib/client \
	-I $(top_srcdir)/lib/ocomm \
	-I $(srcdir)/.. \
	-DWITH_OML

noinst_LTLIBRARIES = libotg2core.la

libotg2core_la_SOURCES = \
	component.cpp \
	address.cpp \
	flow.cpp \
	packet.cpp \
	stream.cpp \
	unixtime.cpp

libotg2core_la_LIBADD = \
	$(top_builddir)/lib/ocomm/libocomm.la
