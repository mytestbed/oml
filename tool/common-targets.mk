

BUILD_TOP_DIR = $(BUILD_DIR)/$(PKG_NAME)
ifdef PKG_COMPONENT
BUILD_OBJ_DIR = $(BUILD_TOP_DIR)/obj/$(PKG_COMPONENT)
else
BUILD_OBJ_DIR = $(BUILD_TOP_DIR)/obj
endif
BUILD_BIN_DIR = $(BUILD_TOP_DIR)/bin
BUILD_LIB_DIR = $(BUILD_TOP_DIR)/lib
BUILD_INC_DIR = $(BUILD_TOP_DIR)/include

TOOL_DIR = $(TOP_DIR)/tool

ifndef PREFIX
PREFIX  = $(DESTDIR)/usr
endif
ifndef EXEC_PREFIX
EXEC_PREFIX = $(PREFIX)
endif

INSTALL_BIN_DIR = $(EXEC_PREFIX)/bin
INSTALL_LIB_DIR = $(EXEC_PREFIX)/lib
INSTALL_INC_DIR = $(PREFIX)/include
ifdef INC_PKG
INSTALL_INC_DIR := $(INSTALL_INC_DIR)/$(INC_PKG)
endif

SRCS    = $(C_SRCS) $(CPP_SRCS)
OBJS	= $(C_SRCS:%.c=$(BUILD_OBJ_DIR)/%.o) \
		$(CPP_SRCS:%.cpp=$(BUILD_OBJ_DIR)/%.oo)
PUBLIC_INCS = $(INCS:%.h=$(BUILD_INC_DIR)/%.h)
LIB_FILE = lib$(LIBRARY).so

INSTALL_INCS = $(INCS:%.h=$(INSTALL_INC_DIR)/%.h)

ifdef PROGRAM
INSTALL_PROG = $(INSTALL_BIN_DIR)/$(PROGRAM)

$(PROGRAM): $(BUILD_BIN_DIR)/$(PROGRAM)
	@

$(BUILD_BIN_DIR)/$(PROGRAM): $(C_SRCS) $(AUTO) $(BUILD_OBJ_DIR) $(BUILD_BIN_DIR) $(OBJS)
	for d in $(DIRS) ; do \
		make TOP_DIR=$(TOP_DIR) -C $$d $* build; \
	done
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)
endif

ifdef LIBRARY
INSTALL_LIBS = $(INSTALL_LIB_DIR)/$(LIB_FILE)

$(LIBRARY): $(SRCS) $(EXT) $(BUILD_LIB_DIR)/$(LIB_FILE) $(BUILD_INC_DIR) $(PUBLIC_INCS)

$(BUILD_LIB_DIR)/$(LIB_FILE): $(SRCS) $(BUILD_OBJ_DIR) $(BUILD_LIB_DIR) $(OBJS)
	$(CC) -fPIC -shared -o $@ $(OBJS) $(LDFLAGS) $(LIBS)
endif

install: build $(INSTALL_INC_DIR) $(INSTALL_INCS) $(INSTALL_BIN_DIR) $(INSTALL_PROG) $(INSTALL_LIB_DIR) $(INSTALL_LIBS)


clean:
	rm -rf $(BUILD_DIR)/$(PKG_NAME) *~

debug:
	echo $(INSTALL_LIBS)

$(BUILD_OBJ_DIR)/%.o: %.c
	$(CC) -c -fPIC -DPIC $(CFLAGS) $< -o $@

$(BUILD_OBJ_DIR)/%.oo: %.cpp
	$(CPP) -c -fPIC -DPIC $(CFLAGS) $< -o $@

$(BUILD_INC_DIR)/%.h:  %.h
	install -d `dirname $@`
	cp $< $@

$(INSTALL_INC_DIR):
	install -d $@

$(INSTALL_INC_DIR)/%.h:  %.h
	install -d `dirname $@`
	install -m 644 $< $@


$(INSTALL_LIB_DIR):
	install -d $@

$(INSTALL_LIB_DIR)/%.a:  $(BUILD_LIB_DIR)/%.a
	install -m 644 $< $@

$(INSTALL_LIB_DIR)/%:  $(BUILD_LIB_DIR)/%
	install -m 755 $< $@

$(INSTALL_BIN_DIR):
	install -d $@

$(INSTALL_BIN_DIR)/%:  $(BUILD_BIN_DIR)/%
	install -m 755 $< $@

$(BUILD_OBJ_DIR):
	mkdir -p $(BUILD_OBJ_DIR)

$(BUILD_LIB_DIR):
	mkdir -p $(BUILD_LIB_DIR)


$(BUILD_BIN_DIR):
	mkdir -p $(BUILD_BIN_DIR)

$(BUILD_INC_DIR):
	mkdir -p $(BUILD_INC_DIR)
