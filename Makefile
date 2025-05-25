# Makefile

CC = gcc
# -Iinclude tells the compiler to look for headers in the 'include' directory
CFLAGS = -Wall -Wextra -pedantic -std=c11 -D_POSIX_C_SOURCE=200809L -Iinclude
LDFLAGS = # Add linker flags here if needed (e.g., -pthread, -lrt)

TARGET = http_server
BUILD_DIR = build
SRC_DIR = src
INC_DIR = include

# Use wildcard to find all .c files in the src directory
SRC = $(wildcard $(SRC_DIR)/*.c)
# Generate object file paths, placing them in the build directory
# e.g., src/main.c -> build/main.o
OBJ = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC))

.PHONY: all clean debug run test

# Default target: builds the final executable in the build directory
all: $(BUILD_DIR)/$(TARGET)

# Rule to link object files into the final executable
$(BUILD_DIR)/$(TARGET): $(OBJ) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(OBJ) -o $@ $(LDFLAGS)

# Pattern rule for compiling .c files into .o files
# $< is the source file (e.g., src/main.c)
# $@ is the target file (e.g., build/main.o)
# The | $(BUILD_DIR) ensures the build directory exists before compiling
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to create the build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Clean target: removes the build directory and all its contents
clean:
	rm -rf $(BUILD_DIR)

# Debug target: adds debugging flags and builds the project
debug: CFLAGS += -g -DDEBUG
debug: $(BUILD_DIR)/$(TARGET)

# Run target: builds the project and then executes it from the build directory
run: $(BUILD_DIR)/$(TARGET)
	./$(BUILD_DIR)/$(TARGET)
