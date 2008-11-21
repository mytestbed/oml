
PROGRAM = marshall

INSTALL_DIR = /usr/bin

#### FOR EXPERTS ONLY  ###
TOP = ../../..

SRCS = marshall.c

#CFLAGS = -g -Wall
CFLAGS = -g -DMARSHALL_TEST -I. -I$(TOP)/external/ocommLib \
            -I$(TOP)/client/src/c
LDFLAGS =
LIBS = -lpopt -lsqlite3


BUILD_TOP = $(TOP)/build
BUILD_DIR = $(BUILD_TOP)/$(PROGRAM)
OBJ_DIR = $(BUILD_DIR)/objs
BIN_DIR = $(BUILD_DIR)/bin

OBJS = $(SRCS:%.c=$(OBJ_DIR)/%.o)

$(PROGRAM): $(BIN_DIR)/$(PROGRAM)

$(BIN_DIR)/$(PROGRAM): $(BIN_DIR) $(OBJ_DIR) $(SRCS) $(OBJS) ocommLib
	$(CC) -o $@ $(OBJS) $(shell ls $(OBJ_DIR)/ocomm/*.o) $(LDFLAGS) $(LIBS)

ocommLib:
	@echo '>>> Building $@'
	cd ../../../external/ocommLib; make BUILD_DIR=../../build/$(PROGRAM)/objs/ocomm objs


$(OBJ_DIR)/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJ_DIR):
	mkdir -p $@

$(BIN_DIR):
	mkdir -p $@

install:
	@echo "Nothing to install"

clean:
	rm -fr $(BUILD_DIR) *~

run:
	$(BIN_DIR)/$(PROGRAM)


debug:
	echo $(BUILD_DIR)

marshall.c: marshall.h
