
TESTS_ENVIRONMENT = top_builddir=$(top_builddir) POSTGRES=$(POSTGRES)
TESTS = run.sh run-long.sh
if HAVE_LIBPQ
if HAVE_POSTGRES
TESTS += runpg.sh runpg-long.sh
endif #HAVE_POSTGRES
endif #HAVE_LIBPQ
EXTRA_DIST = run.sh run-long.sh runpg.sh runpg-long.sh fromsq3.sh bin2hex.rb diff.sh

check_PROGRAMS = blobgen

blobgen_SOURCES = blobgen.c

blobgen_CPPFLAGS = \
	-I$(top_srcdir)/lib/client \
	-I$(top_srcdir)/lib/ocomm

blobgen_LDADD = \
	$(top_builddir)/lib/ocomm/libocomm.la \
	$(top_builddir)/lib/client/liboml2.la \
	-lpopt

CLEANFILES=*.hex *.bin *.sq3 *.log

clean-local:
	rm -rf short*/ long*/
