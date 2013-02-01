
TESTS_ENVIRONMENT = top_builddir=$(top_builddir) POSTGRES=$(POSTGRES) TIMEOUT=$(TIMEOUT) VERSION=$(VERSION)
TESTS = run.sh run-long.sh
if HAVE_LIBPQ
if HAVE_POSTGRES
TESTS += runpg.sh runpg-long.sh
endif #HAVE_POSTGRES
endif #HAVE_LIBPQ
EXTRA_DIST = run.sh run-long.sh runpg.sh runpg-long.sh

check_PROGRAMS = blobgen

CLEANFILES = memstats.csv \
	     clientblobgen--longpg.csv \
	     clientblobgen--longsq3.csv \
	     clientblobgenpg.csv \
	     clientblobgensq3.csv \
	     serverblobgen--longpg.csv \
	     serverblobgen--longsq3.csv \
	     serverblobgenpg.csv \
	     serverblobgensq3.csv

blobgen_SOURCES = blobgen.c

blobgen_CPPFLAGS = \
	-I$(top_srcdir)/lib/client \
	-I$(top_srcdir)/lib/ocomm

blobgen_LDADD = \
	$(top_builddir)/lib/ocomm/libocomm.la \
	$(top_builddir)/lib/client/liboml2.la \
	-lpopt

clean-local:
	rm -rf sq3*/ pg*/
