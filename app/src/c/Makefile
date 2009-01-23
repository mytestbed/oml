
PROGRAM = app_sigar_oml2

INSTALL_DIR = /usr/bin

#### FOR EXPERTS ONLY  ###
TOP = ../../..

SRCS = app_sigar.c

#CFLAGS = -g -Wall
CFLAGS = -g -I. \
    -I$(TOP)/client/src/c -I/usr/local/include    -I$(SHARED_SRC_DIR) -I/usr/include/libxml2
LDFLAGS = -L$(TOP)/build/client/lib
LIBS = -lsigar-x86-linux -loml2 -lxml2  -lpopt -lpthread -lpcap -lsqlite3


BUILD_TOP = $(TOP)/build
BUILD_DIR = $(BUILD_TOP)/$(PROGRAM)
OBJ_DIR = $(BUILD_DIR)/objs
BIN_DIR = $(BUILD_DIR)/bin

OBJS = $(SRCS:%.c=$(OBJ_DIR)/%.o)

build: $(PROGRAM)

$(PROGRAM): $(BIN_DIR)/$(PROGRAM)

$(BIN_DIR)/$(PROGRAM): $(BIN_DIR) $(SRCS) $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)



run-test: $(PROGRAM)
	$(BIN_DIR)/$(PROGRAM) --debug-level 99 --logfile -

$(OBJ_DIR)/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/app_pcap_oml2/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJ_DIR):
	mkdir -p $@

$(OBJ_DIR)/app_pcap_oml2:
	mkdir -p $@

$(BIN_DIR):
	mkdir -p $@

$(BUILD_DIR)/app_pcap_oml2:
	mkdir -p $@

install: $(PROGRAM)
	install -m 755 $(BIN_DIR)/$(PROGRAM) $(INSTALL_DIR)

clean:
	rm -fr $(BUILD_DIR) *~

run:
	$(BIN_DIR)/$(PROGRAM) --logfile - -d 5


debug:
	echo $(BUILD_DIR)

app_sigar.c:app_sigar.h
