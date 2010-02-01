ACLOCAL_AMFLAGS = -I ../m4 -Wnone

DATE=`date "+%Y-%m-%d"`
MAN1_FILES= \
	oml2-server.txt

A2X_ARGS = \
	--attribute='oml_version=$(VERSION)' --attribute='date=$(DATE)' -f manpage

man_MANS = $(MAN1_FILES:.txt=.1)

$(MAN1_FILES:.txt=.1):  %.1: %.txt
if HAVE_A2X
	@echo $(MAN1_MANS)
	$(A2X) $(A2X_ARGS) $<
else
	@echo "*** a2x (asciidoc) is required to generate $(@) ***"
endif

CLEANFILES = \
	$(srcdir)/$(MAN1_FILES:.txt=.1) \
	$(srcdir)/$(MAN1_FILES:.txt=.xml)
