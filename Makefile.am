ACLOCAL_AMFLAGS = -I m4

SUBDIRS = gnulib lib server proxy_server python ruby doc example test

EXTRA_DIST = autogen.sh \
	     autoclean.sh \
	     m4/gnulib-cache.m4 \
	     $(top_srcdir)/.version \
	     $(top_srcdir)/build-aux/gen-authors.sh \
	     m4/ax_compare_version.m4 \
	     m4/ax_prog_doxygen.m4 \
	     m4/ax_prog_python_version.m4 \
	     m4/ax_prog_ruby_version.m4 \
	     m4/ax_with_prog.m4 \
	     m4/oml_check_macosx.m4 \
	     m4/oml_distribution_is.m4 \
	     m4/oml_git_search_treeish.m4

BUILT_SOURCES = $(top_srcdir)/.version
$(top_srcdir)/.version:
	echo $(VERSION) > $@-t && mv $@-t $@
dist-hook:
	echo $(VERSION) > $(distdir)/.tarball-version
	if test -d .git; then							\
		$(top_srcdir)/build-aux/gen-authors.sh > $(distdir)/AUTHORS;	\
	fi

if ENABLE_DOC
doc-publish:
	$(MAKE) -C doc/ publish
endif

if BUILD_OML4PY_EGG
oml4py-egg oml4py-push:
	$(MAKE) -C python/ $@
endif

if BUILD_GEM
oml4r-gem oml4r-push:
	$(MAKE) -C ruby/ $@
endif

if ENABLE_PACKAGING

PKG_PACKAGES=
PKG_CLEANFILES=

if BUILD_ANDROID
PKG_AND_BASE_NAME=$(PACKAGE)-$(VERSION)-android
PKG_AND_BUILD_DIR=p-android
PKG_AND_OUT_DIR=$(PKG_AND_BUILD_DIR)/$(PKG_AND_BASE_NAME)

PKG_PACKAGES+=pkg-and
PKG_CLEANFILES+=$(PKG_AND_BUILD_DIR)

$(PKG_AND_BUILD_DIR):
	rm -rf $(PKG_AND_BUILD_DIR)
	mkdir -p $(PKG_AND_OUT_DIR)/external/liboml2/lib/client/filter
	mkdir -p $(PKG_AND_OUT_DIR)/external/liboml2/lib/client/oml2
	mkdir -p $(PKG_AND_OUT_DIR)/external/liboml2/lib/shared
	mkdir -p $(PKG_AND_OUT_DIR)/external/liboml2/lib/ocomm/ocomm
	mkdir -p $(PKG_AND_OUT_DIR)/external/liboml2/lib/ocomm/test
if HAVE_RUBY_MIN_1_8_7
# It's OK to do the following as --enable-packaging is not compatible with VPATH
# builds (it needs a Git checkout).
	$(MAKE) SCAFFOLD="$(RUBY) $(abs_top_builddir)/ruby/oml2-scaffold" -C $(top_srcdir)/example/liboml2 generator_oml.h generator_popt.h
	mkdir -p $(PKG_AND_OUT_DIR)/external/example-oml2
	cd $(top_srcdir)/example/liboml2; \
		find . \( -name "*.c" -o -name "*.h" \) -exec $(install_sh_DATA) {} $(abs_top_builddir)/$(PKG_AND_OUT_DIR)/external/example-oml2/{} \;
# Clean up files created above. It assumes they weren't built before, but
# they shouldn't have been as they are examples not part of the actual
# build.
	rm $(top_srcdir)/example/liboml2/generator_oml.h $(top_srcdir)/example/liboml2/generator_popt.h
endif #HAVE_RUBY_MIN_1_8_7
	cd $(top_srcdir)/lib; \
		find . \( -name "*.c" -o -name "*.h" \) -exec $(install_sh_DATA) {} $(abs_top_builddir)/$(PKG_AND_OUT_DIR)/external/liboml2/{} \;
	mkdir -p $(PKG_AND_OUT_DIR)/external/oml2-proxy-server
	cd $(top_srcdir)/proxy_server; \
		find . \( -name "*.c" -o -name "*.h" \) -exec $(install_sh_DATA) {} $(abs_top_builddir)/$(PKG_AND_OUT_DIR)/external/oml2-proxy-server/{} \;
	$(GIT) archive $(GITANDREF) | $(am__untar) -C $(PKG_AND_OUT_DIR)/
# XXX: This sed invocation requires GNU sed.
	cd $(PKG_AND_OUT_DIR); \
		find . -name "Android.mk" -exec $(SED) -i "s/@version@/$(VERSION)/g" {} \;

pkg-and: $(PKG_AND_BUILD_DIR)/$(PKG_AND_BASE_NAME).tar.gz
$(PKG_AND_BUILD_DIR)/$(PKG_AND_BASE_NAME).tar.gz: $(PKG_AND_BUILD_DIR)
	cd $(PKG_AND_BUILD_DIR); \
		tardir=$(PKG_AND_BASE_NAME) && $(am__tar) | \
		GZIP=$(GZIP_ENV) gzip -c > $(PKG_AND_BASE_NAME).tar.gz

endif #BUILD_ANDROID

if BUILD_ARCH
PKG_ARCH_BASE_NAME=$(PACKAGE)-$(OML_ALT_VER)
PKG_ARCH_BUILD_DIR=p-arch
PKG_ARCH_BUILD=$(PKG_ARCH_BUILD_DIR)/PKGBUILD

PKG_PACKAGES+=pkg-arch
PKG_CLEANFILES+=$(PKG_ARCH_BUILD_DIR)

MAKEPKG_FLAGS=-fc $(MAKEPKG_FLAGS_ADD)

$(PKG_ARCH_BUILD):
	rm -rf $(PKG_ARCH_BUILD_DIR)
	mkdir -p $(PKG_ARCH_BUILD_DIR)/tmp
	$(MAKE) distdir=$(PKG_ARCH_BUILD_DIR)/tmp/$(PKG_ARCH_BASE_NAME) distdir
	$(GIT) archive $(GITARCHREF) | $(am__untar) -C $(PKG_ARCH_BUILD_DIR)
# Releases should already know where to get the tarball
if !GITISTAG
	cd $(PKG_ARCH_BUILD_DIR)/tmp/; \
		tardir=$(PKG_ARCH_BASE_NAME) && $(am__tar) | \
		GZIP=$(GZIP_ENV) gzip -c >../$(PKG_ARCH_BASE_NAME).tar.gz
endif
	cat $(PKG_ARCH_BUILD).proto | \
		$(SED) -e "/<GIT/,/GIT>/d" | \
		$(SED) -e "/!GIT.*/d" -e "s/^pkgver.*=.*/pkgver=$(OML_ALT_VER)/" \
		> $(PKG_ARCH_BUILD)
	cd $(PKG_ARCH_BUILD_DIR); $(MAKEPKG) -p PKGBUILD -g >> PKGBUILD
	cat $(PKG_ARCH_BUILD).proto | \
		$(SED) -e "/<!GIT/,/!GIT>/d" -e "/GIT.*/d" \
		> $(PKG_ARCH_BUILD)-git
	cd $(PKG_ARCH_BUILD_DIR); $(MAKEPKG) -p PKGBUILD-git -g >> PKGBUILD-git

pkg-arch: $(PKG_ARCH_BUILD)
	cd $(PKG_ARCH_BUILD_DIR); $(MAKEPKG) $(MAKEPKG_FLAGS)
	cd $(PKG_ARCH_BUILD_DIR); $(MAKEPKG) $(MAKEPKG_FLAGS) --source
	cd $(PKG_ARCH_BUILD_DIR); $(MAKEPKG) $(MAKEPKG_FLAGS) -p PKGBUILD-git --source
endif #BUILD_ARCH

if BUILD_DEBIAN
PKG_DEB_BASE_NAME=$(PACKAGE)_$(VERSION)
PKG_DEB_BUILD_DIR=p-debian
PKG_DEB_ORIG_SRC=$(PKG_DEB_BUILD_DIR)/$(PKG_DEB_BASE_NAME).orig.tar.gz
PKG_DEB_RULES=$(PKG_DEB_BUILD_DIR)/$(distdir)/debian/rules

PKG_PACKAGES+=pkg-deb
PKG_CLEANFILES+= $(PKG_DEB_BUILD_DIR)

DPKG_BUILDPACKAGE_FLAGS=-us -uc $(DPKG_BUILDPACKAGE_FLAGS_ADD)

$(PKG_DEB_RULES): dist-gzip
	rm -rf $(PKG_DEB_BUILD_DIR)
	mkdir -p $(PKG_DEB_BUILD_DIR)
	test -e $(PKG_DEB_ORIG_SRC) || $(LN_S) ../$(distdir).tar.gz $(PKG_DEB_ORIG_SRC)
	$(MAKE) distdir=$(PKG_DEB_BUILD_DIR)/$(distdir)/ distdir
	$(GIT) archive $(GITDEBREF) | $(am__untar) -C $(PKG_DEB_BUILD_DIR)/$(distdir)/
if !IS_DEBIAN
	mv $(PKG_DEB_RULES) $(PKG_DEB_RULES).orig
	cat $(PKG_DEB_RULES).orig | \
		$(SED) -e "s/dh_shlibdeps/#&/" \
		> $(PKG_DEB_RULES)
	rm $(PKG_DEB_RULES).orig
endif
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

RPMBUILD_FLAGS=-bb --clean $(RPMBUILD_FLAGS_ADD)

$(PKG_RPM_SPEC):
	rm -rf $(PKG_RPM_BUILD_DIR)
	mkdir -p $(PKG_RPM_BUILD_DIR)/{BUILD,RPMS,SOURCES,SPECS,SRPMS}
	$(GIT) archive $(GITRPMREF) | $(am__untar) -C $(PKG_RPM_BUILD_DIR)
if GITISTAG
# Releases should have a valid specfile and know where to get the tarball
#
# Extract all defines from a SPECFILE, and reformat them into VAR=VALUE, to be
# evaluated by the shell. Then, replace %{VAR} with ${VAR} in the Source entries
# of the SPECFILE, and let the shell substitute these variables with their newly
# assigned values.
	URL=$$(eval "`sed -n "s/%define\s\+\(\S\+\)\s\+\(\S\+\)$$/\1=\2/p;/Source[0-9]*:[ \t]*http/{s/Source[0-9]*:\s*/echo /;s/%/\$$/g;p}" $(PKG_RPM_SPEC)`"); \
		wget -q $${URL} -O $(PKG_RPM_BUILD_DIR)/SOURCES/`basename $${URL}`
else
	$(MAKE) distdir=$(PKG_RPM_BUILD_DIR)/BUILD/$(PKG_RPM_BASE_NAME) distdir
	cd $(PKG_RPM_BUILD_DIR)/BUILD/; \
		tardir=$(PKG_RPM_BASE_NAME) && $(am__tar) | \
		GZIP=$(GZIP_ENV) gzip -c >../SOURCES/$(PKG_RPM_BASE_NAME).tar.gz
	mv $(PKG_RPM_SPEC) $(PKG_RPM_SPEC).orig
	cat $(PKG_RPM_SPEC).orig | \
		$(SED) -e "s/^%define version.*/%define version	$(OML_ALT_VER)/" \
		-e "s/^%define pkgver.*/%define pkgver $(VERSION)/" \
		-e "s/^%define liboml2cur.*/%define liboml2cur $(LIBOML2_LT_VER_CUR)/" \
		-e "s/^%define liboml2rev.*/%define liboml2rev $(LIBOML2_LT_VER_REV)/" \
		-e "s/^%define liboml2age.*/%define liboml2age $(LIBOML2_LT_VER_AGE)/" \
		-e "s/^%define libocommcur.*/%define libocommcur $(LIBOCOMM_LT_VER_CUR)/" \
		-e "s/^%define libocommrev.*/%define libocommrev $(LIBOCOMM_LT_VER_REV)/" \
		-e "s/^%define libocommage.*/%define libocommage $(LIBOCOMM_LT_VER_AGE)/" \
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
