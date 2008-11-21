#
# Makefile for a binary and a library
#
# Assumes the declaration of a LIBRARY and PROGRAM variable
#
# $Id: bin-lib-makefile.mk 9 2005-05-01 04:32:29Z max $
#


build: $(LIBRARY) $(PROGRAM)
	@echo "BUILT: $(LIBRARY) and $(PROGRAM)"

include $(TOP_DIR)/tool/common-targets.mk



