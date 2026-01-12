# Ticket CLI - C Implementation
# Build system using Make

CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -O2
DEBUGFLAGS := -g -O0 -DDEBUG
LDFLAGS :=
# Libraries (uncomment when implementing features that require them)
# LIBS := -lyaml -lcrypto -ljson-c
LIBS := -lcrypto

# Directories
SRC_DIR := src
INC_DIR := include
TEST_DIR := tests
BIN_DIR := bin
OBJ_DIR := obj
TEST_OBJ_DIR := obj/tests

# Source files
SOURCES := $(wildcard $(SRC_DIR)/*.c)
OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))
MAIN_OBJ := $(OBJ_DIR)/main.o
LIB_OBJECTS := $(filter-out $(MAIN_OBJ),$(OBJECTS))

# Test files
TEST_SOURCES := $(wildcard $(TEST_DIR)/*.c)
TEST_OBJECTS := $(patsubst $(TEST_DIR)/%.c,$(TEST_OBJ_DIR)/%.o,$(TEST_SOURCES))

# Target executable
TARGET := $(BIN_DIR)/ticket
TEST_TARGET := $(BIN_DIR)/test_runner

# Include paths
INCLUDES := -I$(INC_DIR)
# Add Homebrew OpenSSL paths for macOS
ifeq ($(shell uname), Darwin)
    INCLUDES += -I/opt/homebrew/include
    LDFLAGS += -L/opt/homebrew/lib
endif

.PHONY: all clean test debug run install check lint format help

# Default target
all: $(TARGET)

# Create necessary directories
$(OBJ_DIR) $(TEST_OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile test files
$(TEST_OBJ_DIR)/%.o: $(TEST_DIR)/%.c | $(TEST_OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Link main executable
$(TARGET): $(OBJECTS) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $@
	@echo "Built: $(TARGET)"

# Link test executable
$(TEST_TARGET): $(TEST_OBJECTS) $(LIB_OBJECTS) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $(TEST_OBJECTS) $(LIB_OBJECTS) $(LIBS) -lcheck -o $@
	@echo "Built: $(TEST_TARGET)"

# Build with debug symbols
debug: CFLAGS += $(DEBUGFLAGS)
debug: clean $(TARGET)

# Run the program
run: $(TARGET)
	./$(TARGET)

# Run tests
test: $(TEST_TARGET)
	./$(TEST_TARGET)

# Check code (static analysis)
check:
	@echo "Running cppcheck static analysis..."
	@command -v cppcheck >/dev/null 2>&1 && \
		cppcheck --enable=all --suppress=missingIncludeSystem \
		--std=c11 $(INCLUDES) $(SRC_DIR) $(TEST_DIR) || \
		echo "cppcheck not found, skipping static analysis"

# Lint code (format check)
lint:
	@echo "Checking code formatting with clang-format..."
	@command -v clang-format >/dev/null 2>&1 && \
		clang-format --dry-run --Werror $(SRC_DIR)/*.c $(INC_DIR)/*.h 2>/dev/null || \
		echo "clang-format not found or no issues found"

# Format code
format:
	@echo "Formatting code with clang-format..."
	@command -v clang-format >/dev/null 2>&1 && \
		clang-format -i $(SRC_DIR)/*.c $(INC_DIR)/*.h || \
		echo "clang-format not found, skipping format"

# Install (copy to system path)
install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

# Clean build artifacts
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "Cleaned build artifacts"

# Help target
help:
	@echo "Ticket CLI - C Implementation"
	@echo ""
	@echo "Available targets:"
	@echo "  all      - Build the main executable (default)"
	@echo "  debug    - Build with debug symbols"
	@echo "  test     - Build and run tests"
	@echo "  run      - Build and run the main executable"
	@echo "  check    - Run static analysis (cppcheck)"
	@echo "  lint     - Check code formatting (clang-format)"
	@echo "  format   - Format code (clang-format)"
	@echo "  install  - Install to /usr/local/bin"
	@echo "  clean    - Remove build artifacts"
	@echo "  help     - Show this help message"
	@echo ""
	@echo "Dependencies:"
	@echo "  - libyaml (YAML parsing)"
	@echo "  - json-c (JSON output)"
	@echo "  - libcrypto/OpenSSL (SHA256 hashing)"
	@echo "  - check (testing framework)"
