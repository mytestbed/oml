
PROGRAM = omf_nmetrics

INSTALL_DIR = $(DESTDIR)/usr/bin

#### FOR EXPERTS ONLY  ###
TOP = ../../../..

SRCS = ${PROGRAM}.c

#CFLAGS = -g -Wall
CFLAGS = -g -I. \
    -I$(TOP)/client/src/c -I/usr/local/include -I/usr/include/libsigar
LDFLAGS =
LIBS = -lsigar -loml2 -lxml2 -lpopt -lpthread


BUILD_TOP = $(TOP)/build
BUILD_DIR = $(BUILD_TOP)/$(PROGRAM)
OBJ_DIR = $(BUILD_DIR)/objs
BIN_DIR = $(BUILD_DIR)/bin

OBJS = $(SRCS:%.c=$(OBJ_DIR)/%.o)

build:  $(BIN_DIR)/$(PROGRAM)

$(BIN_DIR)/$(PROGRAM): $(BIN_DIR) $(OBJ_DIR) $(SRCS) $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)

$(OBJ_DIR)/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJ_DIR):
	mkdir -p $@

$(BIN_DIR):
	mkdir -p $@

$(BUILD_DIR)/$(PROGRAM):
	mkdir -p $@

install: $(PROGRAM)
	install -m 755 $(BIN_DIR)/$(PROGRAM) $(INSTALL_DIR)

clean:
	rm -fr $(BUILD_DIR) *~

run: ARGS = -i eth0 -m -c
run: $(PROGRAM)
	$(BIN_DIR)/$(PROGRAM) $(ARGS) --oml-file -

debug:
	echo $(OBJS)

$(OBJ_DIR)/$(PROGRAM).o: opts.h oml.h o_log.h

opts.h: $(PROGRAM).app
	ruby oml_scaffold.rb --opts -f opts.h

oml.h: $(PROGRAM).app
#	ruby oml_scaffold.rb --oml -f oml.h


