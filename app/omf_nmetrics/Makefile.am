ACLOCAL_AMFLAGS = -I ../../m4 -Wnone

AM_CPPFLAGS = \
	-I $(top_srcdir)/client/src/c \
	-I $(top_srcdir)/libsigar/include

progname = omf_nmetrics

bin_PROGRAMS = 

if !DARWIN
	bin_PROGRAMS += nmetrics_oml2
	nmetrics_oml2_SOURCES = \
		src/c/omf_nmetrics.c
	nmetrics_oml2_LDADD = \
		$(top_builddir)/client/liboml2.la \
		-lsigar -lpopt
endif



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
