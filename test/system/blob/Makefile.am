
TESTS_ENVIRONMENT = top_builddir=$(top_builddir)
TESTS = run.sh run-long.sh
EXTRA_DIST = run.sh run-long.sh fromsq3.sh bin2hex.rb diff.sh

check_PROGRAMS = blobgen

blobgen_SOURCES = blobgen.c

blobgen_CPPFLAGS = \
	-I$(top_srcdir)/lib/client \
	-I$(top_srcdir)/lib/ocomm

blobgen_LDADD = $(top_builddir)/lib/client/liboml2.la -lpopt

CLEANFILES=*.hex *.bin *.sq3 *.log
