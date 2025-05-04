CC = gcc
CFLAGS = -Wall -g -std=c11 -Iinclude -fPIC
LDFLAGS = -shared

SRC_DIR = src
BIN_DIR = bin
INCLUDE_DIR = include

SRC_FILES = $(SRC_DIR)/VCParser.c $(SRC_DIR)/LinkedListAPI.c $(SRC_DIR)/VCHelpers.c $(SRC_DIR)/wrappers.c
OBJ_FILES = $(BIN_DIR)/VCParser.o $(BIN_DIR)/LinkedListAPI.o $(BIN_DIR)/VCHelpers.o $(BIN_DIR)/wrappers.o
TARGET = $(BIN_DIR)/libvcparser.so
TEST_EXEC = test_program

all: parser

parser: $(BIN_DIR) $(OBJ_FILES)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJ_FILES)

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TARGET) $(SRC_DIR)/main.c
	$(CC) $(CFLAGS) -o $(TEST_EXEC) $(SRC_DIR)/main.c -L$(BIN_DIR) -lvcparser
	@echo "Run the test program with: LD_LIBRARY_PATH=$(BIN_DIR) ./$(TEST_EXEC)"

clean:
	rm -f $(OBJ_FILES) $(TARGET) $(TEST_EXEC)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

.PHONY: all clean test
