ACLOCAL_AMFLAGS = -I ../../m4

AM_CPPFLAGS = -I $(top_srcdir)/client/src/c

bin_PROGRAMS = oml_ex_simple

oml_ex_simple_SOURCES = generator.c

oml_ex_simple_LDADD = \
	$(top_builddir)/client/liboml2.la \
	-lpopt
