
PROGRAM = oml2-proxy-server

INSTALL_DIR = $(DESTDIR)/usr/bin

#### FOR EXPERTS ONLY  ###
TOP = ../../..

SRCS = main.c proxy_client_handler.c  \

SHARED_SRCS = marshall.c

SHARED_SRC_DIR = $(TOP)/shared/src/c

#CFLAGS = -g -Wall
CFLAGS = -g -I. -I$(TOP)/external/ocommLib \
    -I$(TOP)/client/src/c -I/usr/local/include    -I$(SHARED_SRC_DIR) -I/usr/include/libxml2
LDFLAGS =
LIBS = -lpopt -lpthread


BUILD_TOP = $(TOP)/build
BUILD_DIR = $(BUILD_TOP)/$(PROGRAM)
OBJ_DIR = $(BUILD_DIR)/objs
BIN_DIR = $(BUILD_DIR)/bin

OBJS = $(SRCS:%.c=$(OBJ_DIR)/%.o) $(SHARED_SRCS:%.c=$(OBJ_DIR)/shared/%.o)

build: $(PROGRAM)

$(PROGRAM): $(BIN_DIR)/$(PROGRAM)

$(BIN_DIR)/$(PROGRAM): $(BIN_DIR) $(OBJ_DIR)/shared $(SRCS) $(OBJS) ocommLib
	$(CC) -o $@ $(OBJS) $(shell ls $(OBJ_DIR)/ocomm/*.o) $(LDFLAGS) $(LIBS)

ocommLib:
	@echo '>>> Building $@'
	cd ../../../external/ocommLib; make BUILD_DIR=../../build/$(PROGRAM)/objs/ocomm objs

run-test: $(PROGRAM)
	$(BIN_DIR)/$(PROGRAM) --debug-level 99 --logfile -

$(OBJ_DIR)/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJ_DIR)/shared/%.o: $(TOP)/shared/src/c/%.c
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/sample/%.o: sample/%.c
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJ_DIR):
	mkdir -p $@

$(OBJ_DIR)/shared:
	mkdir -p $@

$(BIN_DIR):
	mkdir -p $@

$(BUILD_DIR)/sample:
	mkdir -p $@

install: $(PROGRAM)
	install -m 755 $(BIN_DIR)/$(PROGRAM) $(INSTALL_DIR)

clean:
	rm -fr $(BUILD_DIR) *~

run:
	$(BIN_DIR)/$(PROGRAM) --logfile - -d 5


debug:
	echo $(BUILD_DIR)
	echo $(SHARED_SRC_DIR)/marshall.h

main.c: version.h proxy_server.h
proxy_client_handler.c: proxy_server.h proxy_client_handler.h $(SHARED_SRC_DIR)/marshall.h
$(TOP)/shared/src/c/marshall.c: $(TOP)/shared/src/c/marshall.h
