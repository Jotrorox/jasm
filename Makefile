# Makefile for jasm
NAME = jasm
VERSION = 0.1

# Directories
SRC_DIR = src
BUILD_DIR = build
LIB_DIR = lib

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -I$(LIB_DIR) -O2

# Source files: all .c files within the src directory.
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# Final binary name.
TARGET = $(BUILD_DIR)/$(NAME)-$(VERSION)

# Targets
all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean