ACLOCAL_AMFLAGS = -I ../../m4

AM_CPPFLAGS = \
	-I $(top_srcdir)/lib/client/src/c \
	-I $(top_srcdir)/lib/ocomm

noinst_PROGRAMS = oml_ex_simple

oml_ex_simple_SOURCES = generator.c

oml_ex_simple_LDADD = \
	$(top_builddir)/lib/client/liboml2.la \
	$(top_builddir)/lib/ocomm/libocomm.la \
	-lpopt
