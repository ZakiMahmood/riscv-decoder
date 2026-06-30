CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -g -Iinclude
LDFLAGS =

SRC_DIR  = src
OBJ_DIR  = build
BIN_DIR  = bin
TEST_DIR = test

SRC     = $(wildcard $(SRC_DIR)/*.c)
OBJ     = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
TARGET  = $(BIN_DIR)/riscv-decoder

# decoder.o gets reused by the unit test binary so we don't duplicate
# the decode logic - only main.o is excluded since it has its own main()
DECODER_OBJS = $(filter-out $(OBJ_DIR)/main.o,$(OBJ))
TEST_BIN     = $(BIN_DIR)/test_decoder

.PHONY: all clean test debug valgrind dirs help

all: dirs $(TARGET)

dirs:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "Build complete: $@"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# the test binary links the decoder object files plus its own test main,
# so it's a separate executable from the real CLI tool
$(TEST_BIN): dirs $(DECODER_OBJS) $(OBJ_DIR)/test_decoder.o
	$(CC) $(LDFLAGS) -o $@ $(DECODER_OBJS) $(OBJ_DIR)/test_decoder.o

$(OBJ_DIR)/test_decoder.o: $(TEST_DIR)/test_decoder.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_BIN) all
	@echo "--- unit tests ---"
	./$(TEST_BIN)
	@echo "--- sample run ---"
	./$(TARGET) $(TEST_DIR)/programs/mixed.hex

debug: CFLAGS += -DDEBUG -O0
debug: all

valgrind: all
	valgrind --leak-check=full ./$(TARGET) $(TEST_DIR)/programs/mixed.hex

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

help:
	@echo "Targets: all, test, debug, valgrind, clean, help"
