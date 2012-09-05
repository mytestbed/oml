

PROGRAM = generator

INSTALL_DIR = $(DESTDIR)/usr/local/bin

#### FOR EXPERTS ONLY  ###
TOP = .

SRCS = ${PROGRAM}.c

CFLAGS = -g -I. -I/usr/local/include
LDFLAGS =
LIBS = -loml2 -lxml2 -lpopt -lpthread -lm


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

debian: build
	mkdir -p $(BUILD_DIR)/debian
	v=`cat version.h | sed -n 's/.*VERSION[^"]*"([^"]*).*//p'`; 	epm --output-dir $(BUILD_DIR)/debian -n -f deb $(DEB_NAME) 		build_dir=$(BUILD_DIR) version=$$v 		$(PROGRAM).epm

clean:
	rm -fr $(BUILD_DIR) *~ $(PROGRAM)_popt.h $(PROGRAM)_oml.h

run: ARGS = --loop --delay 1
run: build
	$(BIN_DIR)/$(PROGRAM) $(ARGS) --oml-file -

debug:
	echo $(OBJS)

$(OBJ_DIR)/$(PROGRAM).o: version.h $(PROGRAM)_popt.h $(PROGRAM)_oml.h

$(PROGRAM)_popt.h: $(PROGRAM).rb
	oml2-scaffold --opts $<

$(PROGRAM)_oml.h: $(PROGRAM).rb
	oml2-scaffold --oml $<
