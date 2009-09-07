ACLOCAL_AMFLAGS = -I ../m4 -Wnone

SUBDIRS = simple omf_nmetrics omf_trace iperf otg

AM_CPPFLAGS = \
	-I $(top_srcdir)/client/src/c \
	-I $(top_srcdir)/lib/ocomm \
	-I $(top_srcdir)/libsigar/include 

bin_PROGRAMS = gps_oml2 wlanconfig_oml2

gps_oml2_SOURCES = \
	src/c/Gps.cpp \
	src/c/Gps.hpp

gps_oml2_LDADD = \
	$(top_builddir)/client/liboml2.la \
	$(top_builddir)/lib/ocomm/libocomm.la

wlanconfig_oml2_SOURCES = src/c/wlanconfig_oml.c

wlanconfig_oml2_LDADD = \
	$(top_builddir)/client/liboml2.la \
	$(top_builddir)/lib/ocomm/libocomm.la
