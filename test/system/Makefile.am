
TESTS_ENVIRONMENT = top_srcdir=$(top_srcdir) srcdir=$(srcdir) \
		    top_builddir=$(top_builddir) builddir=$(builddir) \
		    VERSION=$(VERSION) CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" LIBADD="$(LIBADD)" \
		    POSTGRES=$(POSTGRES) TIMEOUT="$(TIMEOUT)"
TESTS = scaffold.sh reconnect.sh reconnect-text.sh run.sh run-long.sh
if HAVE_LIBPQ
if HAVE_POSTGRES
TESTS += runpg.sh runpg-long.sh
endif #HAVE_POSTGRES
endif #HAVE_LIBPQ

if HAVE_PYTHON
TESTS += self-inst.sh
endif

EXTRA_DIST = \
	     tap_helper.sh \
	     run.sh run-long.sh runpg.sh runpg-long.sh \
	     scaffold.sh reconnect.sh reconnect-text.sh \
	     self-inst.sh self-inst.py

check_PROGRAMS = blobgen

CLEANFILES = memstats.csv \
	     clientblobgen--longpg.csv \
	     clientblobgen--longsq3.csv \
	     clientblobgenpg.csv \
	     clientblobgensq3.csv \
	     serverblobgen--longpg.csv \
	     serverblobgen--longsq3.csv \
	     serverblobgenpg.csv \
	     serverblobgensq3.csv \
	     scaffold.log \
	     reconnect.log reconnect-text.log \
	     self-inst.sq3 \
	     self-inst.log \
	     self-inst_server.log

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
