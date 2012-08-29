ACLOCAL_AMFLAGS = -I m4

SUBDIRS = gnulib lib server proxy_server ruby doc test

EXTRA_DIST = m4/gnulib-cache.m4 \
	     $(top_srcdir)/.version \
	     m4/ax_compare_version.m4 \
	     m4/ax_prog_doxygen.m4 \
	     m4/ax_prog_ruby_version.m4 \
	     m4/ax_with_prog.m4 \
	     example/README \
	     example/generator.rb \
	     example/generator.c \
	     example/version.h \
	     example/Makefile \
	     example/config.xml \
	     example/config_text_stream.xml \
	     example/config_two_streams.xml

BUILT_SOURCES = $(top_srcdir)/.version
$(top_srcdir)/.version:
	echo $(VERSION) > $@-t && mv $@-t $@
dist-hook:
	echo $(VERSION) > $(distdir)/.tarball-version

pkgdata_DATA = example/README \
	       example/generator.rb \
	       example/generator.c \
	       example/version.h \
	       example/Makefile \
	       example/config.xml \
	       example/config_text_stream.xml \
	       example/config_two_streams.xml

if HAVE_GEM_BUNDLER
oml4r-gem oml4r-push:
	$(MAKE) -C ruby/ $@
endif

if ENABLE_PACKAGING

PKG_PACKAGES=
PKG_CLEANFILES=

if BUILD_DEBIAN
PKG_DEB_BASE_NAME=$(PACKAGE)_$(VERSION)
PKG_DEB_BUILD_DIR=p-debian
PKG_DEB_ORIG_SRC=$(PKG_DEB_BUILD_DIR)/$(PKG_DEB_BASE_NAME).orig.tar.gz

PKG_PACKAGES+=pkg-deb
PKG_CLEANFILES+= $(PKG_DEB_BUILD_DIR)

DPKG_BUILDPACKAGE_FLAGS=-us -uc

$(PKG_DEB_BUILD_DIR)/$(distdir)/debian/rules: dist-gzip
	rm -rf $(PKG_DEB_BUILD_DIR)
	mkdir -p $(PKG_DEB_BUILD_DIR)
	test -e $(PKG_DEB_ORIG_SRC) || $(LN_S) ../$(distdir).tar.gz $(PKG_DEB_ORIG_SRC)
	$(MAKE) distdir=$(PKG_DEB_BUILD_DIR)/$(distdir)/ distdir
	$(GIT) archive $(GITDEBREF) | $(am__untar) -C $(PKG_DEB_BUILD_DIR)/$(distdir)/
if !GITISTAG
	cd $(PKG_DEB_BUILD_DIR)/$(distdir)/; $(DCH) -v $(VERSION)-1 "Development build (`$(GIT) describe` on branch `$(GIT) describe --contains --all HEAD`), not for general distribution; changes since $(GITTAG) follow"
	cd $(PKG_DEB_BUILD_DIR)/$(distdir)/; $(GIT) log --pretty='%s' $(GITTAG)..HEAD | while read LINE; do $(DCH) -a "$${LINE}"; done
endif

pkg-deb: $(PKG_DEB_BUILD_DIR)/$(distdir)/debian/rules
	cd $(PKG_DEB_BUILD_DIR)/$(distdir)/; $(DPKG_BUILDPACKAGE) $(DPKG_BUILDPACKAGE_FLAGS)
endif #BUILD_DEBIAN

if BUILD_RPM
PKG_RPM_BASE_NAME=$(PACKAGE)-$(OML_ALT_VER)
PKG_RPM_SPEC=$(PKG_RPM_BUILD_DIR)/SPECS/$(PACKAGE).spec
PKG_RPM_BUILD_DIR=p-rpm

PKG_PACKAGES+=pkg-rpm
PKG_CLEANFILES+=$(PKG_RPM_BUILD_DIR)

RPMBUILD_FLAGS=-bb --clean --nodeps

$(PKG_RPM_SPEC):
	rm -rf $(PKG_RPM_BUILD_DIR)
	mkdir -p $(PKG_RPM_BUILD_DIR)/{BUILD,RPMS,SOURCES,SPECS,SRPMS}
	$(MAKE) distdir=$(PKG_RPM_BUILD_DIR)/BUILD/$(PKG_RPM_BASE_NAME) distdir
	$(GIT) archive $(GITRPMREF) | $(am__untar) -C $(PKG_RPM_BUILD_DIR)/SPECS/
# Releases should have a valid specfile and know where to get the tarball
if !GITISTAG
	cd $(PKG_RPM_BUILD_DIR)/BUILD/; \
		tardir=$(PKG_RPM_BASE_NAME) && $(am__tar) | \
		GZIP=$(GZIP_ENV) gzip -c >../SOURCES/$(PKG_RPM_BASE_NAME).tar.gz
	mv $(PKG_RPM_SPEC) $(PKG_RPM_SPEC).orig
	cat $(PKG_RPM_SPEC).orig | \
		$(SED) -e "s/^%define version.*/%define version	$(OML_ALT_VER)/" \
			-e "s/^%define libmax.*/%define libmax $(OML_VER_MINOR_LT)/" \
			-e "s/^%define libpatch.*/%define libpatch $(OML_VER_PATCH_LT)/" \
		> $(PKG_RPM_SPEC)
	rm $(PKG_RPM_SPEC).orig
endif

pkg-rpm: $(PKG_RPM_SPEC)
	$(RPMBUILD) $(RPMBUILD_FLAGS) --define "_topdir $(abs_top_builddir)/$(PKG_RPM_BUILD_DIR)" $(PKG_RPM_SPEC)
endif #BUILD_RPM

pkg-all: $(PKG_PACKAGES)

pkg-clean:
	rm -rf $(PKG_CLEANFILES)

distclean-local: pkg-clean

endif #ENABLE_PACKAGING
