
PROGRAM = app_sigar_oml2
PROGRAM2 = app_pcap_oml2
PROGRAM3 = gps_oml2
PROGRAM4 = wlanconfig_oml2
INSTALL_DIR = /usr/bin

#### FOR EXPERTS ONLY  ###
TOP = ../../..

SRCS = app_sigar.c log.c
SRCS2 = app_pcap.c log.c
SRCS3 = Gps.cpp log.c
SRCS4 = wlanconfig_oml.c log.c
#CFLAGS = -g -Wall
CFLAGS = -g -I. \
    -I$(TOP)/client/src/c -I/usr/local/include -I/usr/include/libsigar  -I$(SHARED_SRC_DIR) -I/usr/include/libxml2
LDFLAGS = -L$(TOP)/build/client/lib
LIBS = -lsigar -loml2 -lxml2 -lpopt -lpthread -lpcap


BUILD_TOP = $(TOP)/build
BUILD_DIR = $(BUILD_TOP)/$(PROGRAM)
BUILD_DIR2 = $(BUILD_TOP)/$(PROGRAM2)
BUILD_DIR3 = $(BUILD_TOP)/$(PROGRAM3)
BUILD_DIR4 = $(BUILD_TOP)/$(PROGRAM4)
OBJ_DIR = $(BUILD_DIR)/objs
BIN_DIR = $(BUILD_DIR)/bin
OBJ_DIR2 = $(BUILD_DIR2)/objs
BIN_DIR2 = $(BUILD_DIR2)/bin
OBJ_DIR3 = $(BUILD_DIR3)/objs
BIN_DIR3 = $(BUILD_DIR3)/bin
OBJ_DIR4 = $(BUILD_DIR4)/objs
BIN_DIR4 = $(BUILD_DIR4)/bin


OBJS = $(SRCS:%.c=$(OBJ_DIR)/%.o)
OBJS2 = $(SRCS2:%.c=$(OBJ_DIR2)/%.o)
OBJS3 = $(SRCS3:%.c=$(OBJ_DIR3)/%.o)
OBJS4 = $(SRCS4:%.c=$(OBJ_DIR4)/%.o)

build: $(PROGRAM) $(PROGRAM2) $(PROGRAM3) $(PROGRAM4)

$(PROGRAM): $(BIN_DIR)/$(PROGRAM)
$(PROGRAM2): $(BIN_DIR2)/$(PROGRAM2)
$(PROGRAM3): $(BIN_DIR3)/$(PROGRAM3)
$(PROGRAM4): $(BIN_DIR4)/$(PROGRAM4)

$(BIN_DIR)/$(PROGRAM): $(BIN_DIR) $(OBJ_DIR) $(SRCS) $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)

$(BIN_DIR2)/$(PROGRAM2): $(BIN_DIR2) $(OBJ_DIR2) $(SRCS2) $(OBJS2)
	$(CC) -o $@ $(OBJS2) $(LDFLAGS) $(LIBS)

$(BIN_DIR3)/$(PROGRAM3): $(BIN_DIR3) $(OBJ_DIR3) $(SRCS3) $(OBJS3)
	g++ -o $@ $(OBJS3) $(LDFLAGS) $(LIBS)

$(BIN_DIR4)/$(PROGRAM4): $(BIN_DIR4) $(OBJ_DIR4) $(SRCS4) $(OBJS4)
	$(CC) -o $@ $(OBJS4) $(LDFLAGS) $(LIBS)

run-test: $(PROGRAM)
	$(BIN_DIR)/$(PROGRAM) --debug-level 99 --logfile -

$(OBJ_DIR)/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJ_DIR2)/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJ_DIR3)/%.o: %.c
	g++ -c $(CFLAGS) $< -o $@

$(OBJ_DIR4)/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/app_sigar_oml2/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR2)/app_pcap_oml2/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR3)/gps_oml2/%.o: %.c
	g++ -c $(CFLAGS) $< -o $@

$(BUILD_DIR4)/wlanconfig_oml/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJ_DIR):
	mkdir -p $@

$(OBJ_DIR2):
	mkdir -p $@

$(OBJ_DIR3):
	mkdir -p $@

$(OBJ_DIR4):
	mkdir -p $@

$(BIN_DIR):
	mkdir -p $@

$(BIN_DIR2):
	mkdir -p $@

$(BIN_DIR3):
	mkdir -p $@

$(BIN_DIR4):
	mkdir -p $@

$(BUILD_DIR)/app_sigar_oml2:
	mkdir -p $@

$(BUILD_DIR2)/app_pcap_oml2:
	mkdir -p $@

$(BUILD_DIR3)/gps_oml2:
	mkdir -p $@

$(BUILD_DIR4)/gps_oml2:
	mkdir -p $@

install: $(PROGRAM) $(PROGRAM2)
	install -m 755 $(BIN_DIR)/$(PROGRAM) $(INSTALL_DIR)
	install -m 755 $(BIN_DIR2)/$(PROGRAM2) $(INSTALL_DIR)
	install -m 755 $(BIN_DIR3)/$(PROGRAM3) $(INSTALL_DIR)
	install -m 755 $(BIN_DIR4)/$(PROGRAM4) $(INSTALL_DIR)

clean:
	rm -fr $(BUILD_DIR) *~ $(BUILD_DIR2) *~ $(BUILD_DIR3)

run:
	$(BIN_DIR)/$(PROGRAM) --logfile - -d 5


debug:
	echo $(BUILD_DIR)

app_sigar.c:app_sigar.h log.h
app_pcap.c:app_pcap.h log.h
Gps.cpp:Gps.hpp log.h
wlanconfig_oml.c:log.h
log.c:log.h
