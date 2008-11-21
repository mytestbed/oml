BUILD_TOP_DIR = $(BUILD_DIR)/$(PKG_NAME)
APT_VERSION = $(shell grep -o -E '[0-9]+\.[0-9.]+' -m 1 ./changelog)
BUILD_APT_DIR = $(BUILD_TOP_DIR)/$(APT_NAME)-$(APT_VERSION)

apt: build
	rm -rf $(BUILD_APT_DIR)
	install -d $(BUILD_APT_DIR)
	cp -rp $(TOP_DIR)/tool/debian $(BUILD_APT_DIR)
	rm -rf $(BUILD_APT_DIR)/debian/.svn
	cp -rp src/debian $(BUILD_APT_DIR)
	rm -rf $(BUILD_APT_DIR)/debian/.svn
	mv $(BUILD_APT_DIR)/debian/Makefile $(BUILD_APT_DIR)
	cp changelog $(BUILD_APT_DIR)/debian
	sed -e s/@APT_NAME@/$(APT_NAME)/ $(BUILD_APT_DIR)/debian/rules.in > $(BUILD_APT_DIR)/debian/rules
	chmod 755 $(BUILD_APT_DIR)/debian/rules
	cp ../copyright $(BUILD_APT_DIR)/debian
	if [ -r README.debian ] ; then \
		cp README.debian $(BUILD_APT_DIR)/debian; \
	fi
	env APT_VERSION=$(APT_VERSION) $(MAKE) -C $(BUILD_APT_DIR) deb
#	$(MAKE) DESTDIR=$(BUILD_APT_DIR)/debian/$(APT_NAME) install

apt-install: apt
	scp $(BUILD_APT_DIR)/../*.deb $(REPOSITORY):$(REPOSITORY_ROOT)/binary-i386
	scp $(BUILD_APT_DIR)/../*.gz $(REPOSITORY):$(REPOSITORY_ROOT)/source
	scp $(BUILD_APT_DIR)/../*.dsc $(REPOSITORY):$(REPOSITORY_ROOT)/source
	ssh $(REPOSITORY) sudo $(REPOSITORY_ROOT)/rebuild.sh
