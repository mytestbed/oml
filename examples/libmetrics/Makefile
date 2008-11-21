
PROGRAM = oml_metrics

#### FOR EXPERTS ONLY  ###
TOP = ../..

SRCS = metrics.c

#CFLAGS = -g -Wall
CFLAGS = -g -I$(TOP)/client/src/c
LDFLAGS = -L$(TOP)/client/lib
LIBS = -lmetrics -loml2 -lpopt -lpthread


BUILD_TOP = $(TOP)/build
BUILD_DIR = $(BUILD_TOP)/$(PROGRAM)
OBJ_DIR = $(BUILD_DIR)/objs
BIN_DIR = $(BUILD_DIR)/bin

OBJS = $(SRCS:%.c=$(OBJ_DIR)/%.o)

$(PROGRAM): $(BIN_DIR)/$(PROGRAM)

$(BIN_DIR)/$(PROGRAM): $(BIN_DIR) $(OBJ_DIR) $(SRCS) $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)

$(OBJ_DIR)/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJ_DIR):
	mkdir -p $@

$(BIN_DIR):
	mkdir -p $@

install:

clean:
	rm -fr $(BUILD_DIR) *~

run:
	$(BIN_DIR)/$(PROGRAM) --logfile - -d 5 --oml-file -


debug:
	echo $(BUILD_DIR)


