ACLOCAL_AMFLAGS = -I ../m4 -Wnone

scaffold_src = oml2-scaffold

bin_SCRIPTS = $(scaffold_src)
OML4R_GEM_SRC = \
	     oml4r/Rakefile \
	     oml4r/lib/oml4r/oml4r-wlanconfig.rb \
	     oml4r/lib/oml4r/oml4r-simple-example.rb \
	     oml4r/README.md \
	     oml4r/oml4r.gemspec \
	     oml4r/LICENSE \
	     oml4r/Gemfile \
	     oml4r/.yardopts
EXTRA_DIST = \
	     $(scaffold_src) \
	     oml4r/lib/oml4r.rb.in \
	     $(OML4R_GEM_SRC)
CLEANFILES =

progname=scaffold
installed_name = `echo -n "oml2-scaffold" | sed '${program_transform_name}'`
installed_link_name = $(DESTDIR)$(bindir)/`echo -n "oml2_$(progname)" | sed '$(transform)'`
install-exec-hook:
	$(LN_S) $(installed_name) $(installed_link_name)
uninstall-hook:
	rm -f $(installed_link_name)

sharedir = $(datadir)/@PACKAGE@
share_DATA = 

if HAVE_GEM_BUNDLER
OML4R_GEM = oml4r/pkg/oml4r-$(PACKAGE_VERSION).gem
CLEANFILES += $(OML4R_GEM)
if CLEANUP_OML4R_GEM_SRC
CLEANFILES += $(OML4R_GEM_SRC)
endif

share_DATA += oml4r/pkg/oml4r-$(PACKAGE_VERSION).gem \
	      oml4r/lib/oml4r/oml4r-simple-example.rb \
	      oml4r/lib/oml4r/oml4r-wlanconfig.rb

oml4r-gem: $(OML4R_GEM)
$(OML4R_GEM):
if CLEANUP_OML4R_GEM_SRC
	cd $(top_builddir); ./config.status oml4r-vpath-copy
endif
	cd oml4r; $(RAKE) build

oml4r-push: $(OML4R_GEM)
	$(MAKE) -C $(top_builddir)/test/ruby check
	$(GEM) push $(OML4R_GEM)
.PHONY: oml4r-gem oml4r-push
endif
