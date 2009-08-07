ACLOCAL_AMFLAGS = -I ../../m4 -Wnone

AM_CPPFLAGS = \
	-I $(top_srcdir)/client/src/c

progname = omf_trace

bin_PROGRAMS = trace_oml2

trace_oml2_SOURCES = \
	src/c/omf_trace.c

trace_oml2_LDADD = \
	$(top_builddir)/client/liboml2.la \
	-ltrace -lpopt

BUILT_SOURCES = \
	$(progname)_popt.h \
	$(progname)_oml.h

CLEANFILES = \
	$(progname)_popt.h \
	$(progname)_oml.h

OML2_SCAFFOLD = $(top_builddir)/client/oml2_scaffold

$(progname)_popt.h:	src/c/$(progname).app Makefile
	$(OML2_SCAFFOLD) --opts $<

$(progname)_oml.h:	src/c/$(progname).app Makefile
	$(OML2_SCAFFOLD) --oml $<
