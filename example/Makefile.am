ACLOCAL_AMFLAGS = -I ../m4

AM_CPPFLAGS = \
	-I $(top_srcdir)/lib/client \
	-I $(top_srcdir)/lib/ocomm

noinst_PROGRAMS = oml_example

oml_example_SOURCES = generator.c

oml_example_LDADD = \
	$(top_builddir)/lib/client/liboml2.la \
	$(top_builddir)/lib/ocomm/libocomm.la \
	-lpopt
