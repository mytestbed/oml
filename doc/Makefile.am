ACLOCAL_AMFLAGS = -I ../m4 -Wnone

DATE=`date "+%Y-%m-%d"`
MAN1_FILES= \
	oml2-server.txt \
	oml2-proxy-server.txt \
	liboml2.txt

MAN3_FILES= \
	omlc_init.txt \
	omlc_add_mp.txt \
	omlc_start.txt \
	omlc_inject.txt \
	omlc_close.txt \
	omlc_set_int32.txt

EXTRA_DIST=$(MAN1_FILES) $(MAN3_FILES)

A2X_ARGS = \
	--destination-dir=$(builddir) --attribute='oml_version=$(VERSION)' --attribute='date=$(DATE)' -f manpage

man_MANS = $(MAN1_FILES:.txt=.1) $(MAN3_FILES:.txt=.3) liboml2.3

if HAVE_A2X
$(MAN1_FILES:.txt=.1):  %.1: %.txt
	$(A2X) $(A2X_ARGS) $<
	test -f "$(<:.txt=.xml)" && rm $(<:.txt=.xml) || true

liboml2.3: liboml2-api.txt
	$(A2X) $(A2X_ARGS) $<
	test -f "$(<:.txt=.xml)" && rm $(<:.txt=.xml) || true

$(MAN3_FILES:.txt=.3):  %.3: %.txt
	$(A2X) $(A2X_ARGS) $<
	test -f "$(<:.txt=.xml)" && rm $(<:.txt=.xml) || true
else
	@echo "*** a2x (asciidoc) is required to generate $(@) ***"
endif

CLEANFILES = $(MAN1_FILES:.txt=.1) $(MAN3_FILES:.txt=.3) liboml2.3