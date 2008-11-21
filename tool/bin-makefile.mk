#
# Makefile for a binary
#
# Assumes the declaration of a PROGRAM variable
#
# $Id: bin-makefile.mk 11 2005-05-06 19:09:19Z max $
#


build: $(PROGRAM)
	@echo "BUILT: $(PROGRAM)"

include $(TOP_DIR)/tool/common-targets.mk

