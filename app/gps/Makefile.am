ACLOCAL_AMFLAGS = -I ../../m4

AM_CPPFLAGS = \
	-I $(top_srcdir)/lib/client \
	-I $(top_srcdir)/lib/ocomm

bin_PROGRAMS = gps_oml2

gps_oml2_SOURCES = \
	Gps.cpp \
	Gps.hpp

gps_oml2_LDADD = \
	$(top_builddir)/lib/client/liboml2.la \
	$(top_builddir)/lib/ocomm/libocomm.la
