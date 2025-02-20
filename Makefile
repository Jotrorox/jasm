# Makefile for jasm
NAME = jasm
VERSION = 0.1
AUTHOR = Johannes (Jotrorox) Müller
COPYRIGHT = 2025 $(AUTHOR)

# Directories
SRC_DIR = src
BUILD_DIR = build
LIB_DIR = lib

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -I$(LIB_DIR) -O2
RELEASEFLAGS = -DNDEBUG -O3 -fomit-frame-pointer -finline-functions -fstrict-aliasing -flto -march=native -funroll-loops
DEBUGFLAGS = -g -fsanitize=address -fno-omit-frame-pointer -fno-inline -fno-strict-aliasing -fno-lto -O0

# Source files: all .c files within the src directory.
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# Final binary name.
TARGET = $(BUILD_DIR)/$(NAME)

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

release: CFLAGS += $(RELEASEFLAGS)
release: all

debug: CFLAGS += $(DEBUGFLAGS)
debug: all

help:
	@echo "Makefile for $(NAME) v$(VERSION)"
	@echo "Copyright (c) $(COPYRIGHT)"
	@echo ""
	@echo "Usage: make [target]"
	@echo "Targets:"
	@echo "  all        Build the project"
	@echo "  clean      Remove build artifacts"
	@echo "  release    Build the project with optimizations"
	@echo "  debug      Build the project with debugging information"
	@echo "  help       Show this help message"

.PHONY: all clean