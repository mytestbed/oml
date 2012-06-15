ACLOCAL_AMFLAGS = -I ../m4 -Wnone

scaffold_src = oml2-scaffold

bin_SCRIPTS = $(scaffold_src)
EXTRA_DIST = $(scaffold_src)

progname=scaffold
installed_name = $(scaffold_installed_name)
installed_link_name = $(DESTDIR)$(bindir)/oml2_$(progname)
install-exec-hook:
	$(LN_S) $(installed_name) $(installed_link_name)
uninstall-hook:
	rm -f $(installed_link_name)

sharedir = $(datadir)/@PACKAGE@
dist_share_DATA = $(srcdir)/oml4r.rb \
             $(srcdir)/oml4r-example.rb
