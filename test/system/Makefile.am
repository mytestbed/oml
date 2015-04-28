
TESTS_ENVIRONMENT = top_srcdir=$(top_srcdir) srcdir=$(srcdir) \
		    top_builddir=$(top_builddir) builddir=$(builddir) \
		    VERSION=$(VERSION) CC=$(CC) CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" LIBADD="$(LIBADD)" \
		    POSTGRES=$(POSTGRES) TIMEOUT="$(TIMEOUT)" \
		    MALLOC_CHECK_=3
TESTS = scaffold.sh reconnect.sh reconnect-text.sh reconnect-gzip.sh reconnect-text-gzip.sh runsq3.sh runsq3-long.sh
if HAVE_LIBZ
TESTS += runsq3-gzip.sh
endif #HAVE_LIBZ

if HAVE_LIBPQ
if HAVE_POSTGRES
TESTS += runpg.sh runpg-long.sh

if HAVE_LIBZ
TESTS += runpg-long-gzip.sh
endif #HAVE_LIBZ

endif #HAVE_POSTGRES
endif #HAVE_LIBPQ


if HAVE_PYTHON
TESTS += self-inst.sh
endif

EXTRA_DIST = \
	     tap_helper.sh \
	     run.sh \
	     runsq3.sh runsq3-long.sh \
	     runpg.sh runpg-long.sh \
	     runsq3-gzip.sh runpg-long-gzip.sh \
	     scaffold.sh \
	     reconnect.sh reconnect-text.sh \
	     reconnect-gzip.sh reconnect-text-gzip.sh \
	     self-inst.sh self-inst.py

check_PROGRAMS = blobgen

CLEANFILES = memstats.csv client.csv server.csv \
	     runsq3_client.csv runsq3_server.csv \
	     runsq3-long_client.csv \ runsq3-long_server.csv \
	     runpg_client.csv runpg_server.csv \
	     runpg-long_client.csv runpg-long_server.csv \
	     runsq3-gzip_client.csv runsq3-gzip_server.csv \
	     runpg-long-gzip_client.csv runpg-long-gzip_server.csv \
	     scaffold.log \
	     reconnect.log reconnect-text.log \
	     reconnect-gzip.log reconnect-text-gzip.log \
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

if HAVE_LIBZ
check_PROGRAMS += inflate

inflate_SOURCES = inflate.c

inflate_CPPFLAGS = \
	-I$(top_srcdir)/lib/shared \
	-I$(top_srcdir)/lib/ocomm

inflate_LDADD = \
	$(top_builddir)/lib/ocomm/libocomm.la \
	@LIBZ_LIBS@
endif

clean-local:
	rm -rf sq3*/ pg*/
