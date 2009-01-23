
LIBRARY = oml2

INSTALL_LIB_DIR = /usr/lib
INSTALL_INC_DIR = /usr/include

#### FOR EXPERTS ONLY  ###
SRCS = file_writer.c filter.c init.c misc.c net_writer.c \
    parse_config.c api.c 
FILTER_SRCS = factory.c average_filter.c first_filter.c
SHARED_SRCS = marshall.c

SAMPLES = generator

#CFLAGS = -g -Wall
CFLAGS = -g -I. -I$(TOP)/external/ocommLib \
    -I$(TOP)/shared/src/c -I/usr/local/include -I/usr/include/libxml2
LDFLAGS =
TOP = ../../..

SAMPLE_LIBS = -lpopt -lxml2 -lpthread -lm -lpcap

BUILD_TOP = $(TOP)/build
BUILD_DIR = $(BUILD_TOP)/client
OBJ_DIR = $(BUILD_DIR)/objs
LIB_DIR = $(BUILD_DIR)/lib
BIN_DIR = $(BUILD_DIR)/bin

LIB_FILE_A = lib$(LIBRARY).a
LIB_FILE_SO = lib$(LIBRARY).so
OBJS = $(SRCS:%.c=$(OBJ_DIR)/%.o) $(FILTER_SRCS:%.c=$(OBJ_DIR)/filter/%.o) \
        $(SHARED_SRCS:%.c=$(OBJ_DIR)/shared/%.o)

LIB_FILES = $(LIB_FILE_A) $(LIB_FILE_SO)


#SAMPLE_SRCS = $(SAMPLES:%=sample/%.c) $(SAMPLES:%=sample/%_wrapper.c)
#SAMPLE_OBJS = $(SAMPLE_SRCS:%.c=$(BUILD_DIR)/%.o)
#SAMPLE_BINS = $(SAMPLES:%=$(BUILD_DIR)/sample/%)

build: $(LIBRARY) $(BIN_DIR)/test_app
#	echo $(LIB_FILES)

$(LIBRARY): $(OBJ_DIR)/filter $(OBJ_DIR)/shared ocommLib $(LIB_DIR) $(LIB_FILES:%=$(LIB_DIR)/%)

$(LIB_DIR)/lib$(LIBRARY).a:  $(SRCS) $(LIB_DIR) $(OBJS)
	ar cr $@ $(OBJS) $(shell ls $(OBJ_DIR)/ocomm/*.o)

$(LIB_DIR)/lib%.so: $(SRCS) $(LIB_DIR) $(OBJS) $(MT_SRCS) $(MT_OBJS)
	$(CC) -fPIC -shared -o $@ $(OBJS) $(shell ls $(OBJ_DIR)/ocomm/*.o) $(LDFLAGS) $(LIBS)


ocommLib:
	@echo '>>> Building $@'
	cd ../../../external/ocommLib; make BUILD_DIR=../../build/client/objs/ocomm objs

$(BIN_DIR)/test_app: $(BIN_DIR) $(OBJ_DIR)/test_app.o $(OBJS)
	$(CC) -o $@  $(OBJ_DIR)/test_app.o $(OBJS) $(shell ls $(OBJ_DIR)/ocomm/*.o) \
	   -lpopt -lxml2 -lpthread -lm -lpcap

#samples: $(LIBRARY) $(BUILD_DIR)/sample  $(SAMPLE_BINS)

TEST_ID = foo
TEST_ARGS = --oml-id $(TEST_ID) --oml-exp-id test1
#TEST_ARGS = --oml-file -
run-test: $(BIN_DIR)/test_app
	$(BIN_DIR)/test_app --oml-server localhost \
        --oml-log-file - --oml-log-level 99 $(TEST_ARGS)

#$(BUILD_DIR)/sample/%: $(BUILD_DIR)/sample/%.o $(BUILD_DIR)/sample/%_wrapper.o $(LIB_DIR)/$(LIB_FILE_A)
#	$(CC) -o $@ $< $(<:%.o=%_wrapper.o) $(LIB_DIR)/$(LIB_FILE_A) $(SAMPLE_LIBS)

$(OBJ_DIR)/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJ_DIR)/filter/%.o: filter/%.c
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJ_DIR)/shared/%.o: $(TOP)/shared/src/c/%.c
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/sample/%.o: sample/%.c
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJ_DIR):
	mkdir -p $@

$(OBJ_DIR)/filter:
	mkdir -p $@

$(OBJ_DIR)/shared:
	mkdir -p $@

$(LIB_DIR):
	mkdir -p $@

$(BIN_DIR):
	mkdir -p $@

$(BUILD_DIR)/sample:
	mkdir -p $@

install: $(LIBRARY)
	install -m 644 $(LIB_DIR)/$(LIB_FILE_A) $(INSTALL_LIB_DIR)
	install -m 644 $(LIB_DIR)/$(LIB_FILE_SO) $(INSTALL_LIB_DIR)
	install -d $(INSTALL_INC_DIR)/oml2
	install -m 644 oml2/*.h $(INSTALL_INC_DIR)/oml2

clean:
	rm -fr $(BUILD_DIR) *~

debug:
	echo $(LIB_FILES)
	echo "$(OBJ_DIR)/shared/marshall.o: $(TOP)/shared/src/c/marshall.h"

filter.c: client.h oml2/omlc.h oml2/oml.h
init.c: client.h oml2/omlc.h oml2/oml.h
misc.c: client.h oml2/omlc.h oml2/oml.h
wrapper.c: client.h oml2/omlc.h oml2/oml.h
net_writer.c: $(TOP)/shared/src/c/marshall.h
$(TOP)/shared/src/c/marshall.c: $(TOP)/shared/src/c/marshall.h
