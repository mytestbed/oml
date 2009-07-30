ACLOCAL_AMFLAGS = -I ../../m4 -Wnone

AM_CPPFLAGS = \
	-I $(top_srcdir)/client/src/c

progname = omf_nmetrics

bin_PROGRAMS = $(progname)

omf_nmetrics_SOURCES = \
	src/c/omf_nmetrics.c

BUILT_SOURCES = \
	$(progname)_popt.h \
	$(progname)_oml.h

CLEANFILES = \
	$(progname)_popt.h \
	$(progname)_oml.h

omf_nmetrics_LDADD = \
	$(top_builddir)/client/liboml2.la \
	-lsigar -lpopt

OML2_SCAFFOLD = $(top_builddir)/client/oml2_scaffold

$(progname)_popt.h:	src/c/$(progname).app Makefile
	$(OML2_SCAFFOLD) --opts $<

$(progname)_oml.h:	src/c/$(progname).app Makefile
	$(OML2_SCAFFOLD) --oml $<
