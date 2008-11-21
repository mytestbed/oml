#
# Makefile for directories.
#
# Assumes the declaration of a DIRS variable
#
# $Id: dir-makefile.mk 679 2005-10-11 19:07:22Z max $
#


build:
	@for d in $(DIRS) ; do \
		make TOP_DIR=$(TOP_DIR) -C $$d $* build; \
	done

install:
	@for d in $(DIRS) ; do \
		make TOP_DIR=$(TOP_DIR) -C $$d $* install; \
	done

dist:
	@for d in $(DIRS) ; do \
		make TOP_DIR=$(TOP_DIR) -C $$d $* dist; \
	done


ifndef HAVE_CLEAN_TARGET
clean:
	@for d in $(DIRS) ; do \
		make TOP_DIR=$(TOP_DIR) -C $$d $* clean; \
	done
endif

.PHONY: build

