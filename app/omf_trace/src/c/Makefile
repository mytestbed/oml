

PROGRAM = omf_trace

INSTALL_DIR = $(DESTDIR)/usr/bin

#### FOR EXPERTS ONLY  ###
TOP = .

SRCS = ${PROGRAM}.c

CFLAGS = -g -I. -I/usr/local/include
LDFLAGS =
LIBS = -ltrace -loml2 -lxml2 -lpopt -lpthread



#BUILD_TOP = $(TOP)/build
#BUILD_DIR = $(BUILD_TOP)/$(PROGRAM)
BUILD_DIR = build

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

install: build
	install -m 755 $(BIN_DIR)/$(PROGRAM) $(INSTALL_DIR)

clean:
	rm -fr $(BUILD_DIR) *~ $(PROGRAM)_popt.h $(PROGRAM)_oml.h

RUN_IF = eth1
run: ARGS = -i pcapint:$(RUN_IF)
run: build
	$(BIN_DIR)/$(PROGRAM) $(ARGS) --oml-file -

debug:
	echo $(OBJS)

$(OBJ_DIR)/$(PROGRAM).o: $(PROGRAM)_popt.h $(PROGRAM)_oml.h

$(PROGRAM)_popt.h: $(PROGRAM).app
	/home/gjourjon/oml2/MaxChanges/client/src/ruby/oml2_scaffold --opts $<

$(PROGRAM)_oml.h: $(PROGRAM).app
	/home/gjourjon/oml2/MaxChanges/client/src/ruby/oml2_scaffold --oml $<
