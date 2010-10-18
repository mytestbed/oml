
TESTS_ENVIRONMENT = top_builddir=$(top_builddir)
TESTS = run.sh

check_PROGRAMS = blobgen

blobgen_SOURCES = blobgen.c

blobgen_CPPFLAGS = \
	-I$(top_srcdir)/lib/client \
	-I$(top_srcdir)/lib/ocomm

blobgen_LDADD = $(top_builddir)/lib/client/liboml2.la -lpopt

CLEAN_FILES=*.hex *.bin blobgen_exp.sq3 blobgen-server.log
