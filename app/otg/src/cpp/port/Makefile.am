ACLOCAL_AMFLAGS = -I ../../../../../m4 -Wnone

AM_CPPFLAGS = \
	-I $(top_srcdir)/lib/client/src/c \
	-I $(top_srcdir)/lib/ocomm \
	-I $(srcdir)/.. \
	-DWITH_OML

noinst_LTLIBRARIES = libotg2port.la

libotg2port_la_SOURCES = \
	socket.cpp \
	factory.cpp \
	udp_outport.cpp \
	udp_inport.cpp \
	null_outport.cpp

libotg2port_la_LIBADD = \
	$(top_builddir)/lib/ocomm/libocomm.la

