ACLOCAL_AMFLAGS = -I ../m4 -Wnone

DATE=`date "+%Y-%m-%d"`
MAN1_FILES= \
	oml2-server.txt \
	oml2-proxy-server.txt

A2X_ARGS = \
	--destination-dir=$(builddir) --attribute='oml_version=$(VERSION)' --attribute='date=$(DATE)' -f manpage

man_MANS = $(MAN1_FILES:.txt=.1)

$(MAN1_FILES:.txt=.1):  %.1: %.txt
if HAVE_A2X
	$(A2X) $(A2X_ARGS) $<
	test -f "$(<:.txt=.xml)" && rm $(<:.txt=.xml) || true
else
	@echo "*** a2x (asciidoc) is required to generate $(@) ***"
endif

CLEANFILES = $(MAN1_FILES:.txt=.1)