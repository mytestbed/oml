ACLOCAL_AMFLAGS = -I ../../m4 -Wnone

AM_CPPFLAGS = \
	-I $(top_srcdir)/lib/client \
	-I $(top_srcdir)/lib/ocomm

progname = omf_trace

bin_PROGRAMS = trace_oml2

trace_oml2_SOURCES = \
	src/c/omf_trace.c

trace_oml2_LDADD = \
	$(top_builddir)/lib/client/liboml2.la \
	-ltrace -lpopt

BUILT_SOURCES = \
	$(progname)_popt.h \
	$(progname)_oml.h

CLEANFILES = \
	$(progname)_popt.h \
	$(progname)_oml.h

EXTRA_DIST=src/c/$(progname).app

OML2_SCAFFOLD = $(top_builddir)/lib/client/oml2_scaffold

$(progname)_popt.h:	src/c/$(progname).app Makefile
	$(OML2_SCAFFOLD) --opts $<

$(progname)_oml.h:	src/c/$(progname).app Makefile
	$(OML2_SCAFFOLD) --oml $<
