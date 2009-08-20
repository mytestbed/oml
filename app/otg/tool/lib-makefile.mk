
# PROGRAM=unknown

build: $(LIBRARY) 
	@echo "BUILT: $(LIBRARY)"

ifdef TEST_PROGRAM
test: $(TEST_PROGRAM) 
	export LD_LIBRARY_PATH=$(TEST_LD_LIBRARY_PATH); \
	$(BUILD_OBJ_DIR)/$(TEST_PROGRAM) $(TEST_ARGS)

test-gdb: $(TEST_PROGRAM) 
	export LD_LIBRARY_PATH=$(TEST_LD_LIBRARY_PATH); \
	gdb $(BUILD_OBJ_DIR)/$(TEST_PROGRAM) $(TEST_ARGS)
else
test: ;
	@echo "No test program defined"

endif

include $(TOP_DIR)/tool/common-targets.mk

ifdef TEST_PROGRAM

TEST_OBJS = $(TEST_C_SRCS:%.c=$(BUILD_OBJ_DIR)/%.o) \
		$(TEST_CPP_SRCS:%.cpp=$(BUILD_OBJ_DIR)/%.oo)

$(TEST_PROGRAM): $(BUILD_OBJ_DIR)/$(TEST_PROGRAM)
	@

$(BUILD_OBJ_DIR)/$(TEST_PROGRAM): $(LIBRARY) $(TEST_SRCS) $(BUILD_OBJ_DIR) $(TEST_OBJS)
	$(CC) -o $@ $(TEST_OBJS) $(OBJS) $(LDFLAGS) $(LIBS)

endif





