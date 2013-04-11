ACLOCAL_AMFLAGS = -I ../m4 -Wnone

GEM_VERSION = @OML_ALT_VER@

scaffold_src = oml2-scaffold

bin_SCRIPTS = $(scaffold_src)
EXTRA_DIST = \
	     $(scaffold_src).in
CLEANFILES =

progname=scaffold
installed_name = `echo "oml2-scaffold" | sed '${program_transform_name}'`
installed_link_name = $(DESTDIR)$(bindir)/`echo "oml2_$(progname)" | sed '$(transform)'`
install-exec-hook:
	-test -e $(installed_link_name) && mv $(installed_link_name) $(installed_link_name).pre$(PACKAGE_VERSION)
	$(LN_S) $(installed_name) $(installed_link_name)
uninstall-hook:
	rm -f $(installed_link_name)
