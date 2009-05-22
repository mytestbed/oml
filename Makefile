
VERSION = 2.0.1

# All Target
#all: client server
#
#
#client:
#	@echo '>>> Building $@'
#	@cd client/src/c; make
#
#server:
#	@echo '>>> Building $@'
#	@cd server/src/c; make
#
#install:
#	@echo '>>> Building $@'
#	@cd server/src/c; make $@
#	@cd client/src/c; make $@

DIRS = client app


TOP_DIR = $(shell pwd)

all: build

apt:
	@for d in $(DIRS) ; do \
		make TOP_DIR=$(TOP_DIR) -C $$d $* apt; \
	done

HAVE_CLEAN_TARGET=1
include $(TOP_DIR)/config.mk
include $(TOP_DIR)/tool/dir-makefile.mk


man: build/oml2.sub
	pod2man --section 3 --center "oml2" --release "@VERSION@" \
	   --date "`date`" --name oml2 build/oml2.sub oml2.man


oml2.html: oml2.sub
	pod2html --title="oml2" --infile=build/oml2.sub --outfile=oml2.html

README: build/oml2.sub
	pod2text build/oml2.sub README

build/oml2.sub: build_dir oml2.pod
	sed -e 's/OML2_VERSION/$(VERSION)/' < oml2.pod > build/oml2.sub

build_dir:
	mkdir build

# Other Targets
clean:
	-$(RM) -r build
	-$(RM) oml2.man oml2.html
	-@echo ' '


.PHONY: all clean client server apt
.SECONDARY:
