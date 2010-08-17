ACLOCAL_AMFLAGS = -I ../m4 -Wnone

DATE=`date "+%Y-%m-%d"`
MAN1_FILES= \
	oml2-server.txt \
	oml2-proxy-server.txt \
	liboml2.txt \
	oml2_scaffold.txt

MAN3_FILES= \
	omlc_init.txt \
	omlc_add_mp.txt \
	omlc_start.txt \
	omlc_inject.txt \
	omlc_close.txt \
	omlc_set_int32.txt

MAN5_FILES= \
	liboml2.conf.txt

EXTRA_DIST=$(MAN1_FILES) $(MAN3_FILES) $(MAN5_FILES) liboml2-api.txt

A2X_ARGS = \
	--destination-dir=$(builddir) --attribute='oml_version=$(VERSION)' --attribute='date=$(DATE)' -f manpage

if HAVE_A2X
man_MANS = $(MAN1_FILES:.txt=.1) $(MAN3_FILES:.txt=.3) $(MAN5_FILES:.txt=.5) \
	liboml2.3 \
	omlc_set_uint32.3 \
	omlc_set_int64.3 \
	omlc_set_uint64.3 \
	omlc_set_double.3 \
	omlc_set_string.3 \
	omlc_set_const_string.3

$(MAN1_FILES:.txt=.1):  %.1: %.txt
	$(A2X) $(A2X_ARGS) $<
	test -f "$(<:.txt=.xml)" && rm $(<:.txt=.xml) || true

liboml2.3: liboml2-api.txt
	$(A2X) $(A2X_ARGS) $<
	test -f "$(<:.txt=.xml)" && rm $(<:.txt=.xml) || true

$(MAN3_FILES:.txt=.3):  %.3: %.txt
	$(A2X) $(A2X_ARGS) $<
	test -f "$(<:.txt=.xml)" && rm $(<:.txt=.xml) || true

$(MAN5_FILES:.txt=.5):  %.5: %.txt
	$(A2X) $(A2X_ARGS) $<
	test -f "$(<:.txt=.xml)" && rm $(<:.txt=.xml) || true

# Set up roff links for the omlc_set_* macros (they share the same manpage).
omlc_set_%.3:	omlc_set_int32.txt
	echo ".so man3/omlc_set_int32.3" > $@


EXTRA_DIST+=$(MAN1_FILES:.txt=.1) $(MAN3_FILES:.txt=.3) $(MAN5_FILES:.txt=.5) \
	liboml2.3 \
	omlc_set_uint32.3 \
	omlc_set_int64.3 \
	omlc_set_uint64.3 \
	omlc_set_double.3 \
	omlc_set_string.3 \
	omlc_set_const_string.3
else
	@echo "*** a2x (asciidoc) is required to generate $(@) ***"
endif

CLEANFILES = $(MAN1_FILES:.txt=.1) $(MAN3_FILES:.txt=.3) $(MAN5_FILES:.txt=.5) liboml2.3