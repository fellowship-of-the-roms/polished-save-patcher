# Makefile for polished_save_patcher with WebAssembly

# Detect the operating system
OS := $(shell uname -s)

# Compiler and flags
CXX := emcc
CXXFLAGS := -Iinclude -std=c++17

# Directories
SRC_DIR := src
INCLUDE_DIR := include
RESOURCES_DIR := resources
VERSION7_DIR := $(RESOURCES_DIR)/version7
VERSION8_DIR := $(RESOURCES_DIR)/version8
BUILD_DIR := build

# Source files
SOURCES := $(SRC_DIR)/CommonPatchFunctions.cpp \
           $(SRC_DIR)/PatchVersion7to8.cpp \
           $(SRC_DIR)/PatchVersion7to8_unorderedmaps.cpp \
           $(SRC_DIR)/SaveBinary.cpp \
           $(SRC_DIR)/SymbolDatabase.cpp \
           $(SRC_DIR)/Logging.cpp \
           $(SRC_DIR)/main.cpp

# Object files
OBJECTS := $(SOURCES:.cpp=.o)

# Executable name
TARGET := $(BUILD_DIR)/polished_save_patcher.html

# Additional output files
ADDITIONAL_FILES := $(BUILD_DIR)/polished_save_patcher.data \
                    $(BUILD_DIR)/polished_save_patcher.js \
                    $(BUILD_DIR)/polished_save_patcher.wasm \
                    $(BUILD_DIR)/polished_save_patcher.mem \
                    $(BUILD_DIR)/polished_save_patcher.worker.js

# Windows-specific settings
ifeq ($(OS), Windows_NT)
	CXX := emcc
	RM := del
else
	RM := rm -f
endif

# Build target
all: $(BUILD_DIR) $(TARGET) copy-index

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Linking
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS) -s WASM=1 -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' -s FORCE_FILESYSTEM=1 --preload-file resources --bind

# Compilation
$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Copy index.html to build directory
copy-index:
	cp index.html $(BUILD_DIR)/index.html

# Clean
clean:
	$(RM) $(OBJECTS) $(TARGET) $(ADDITIONAL_FILES)

# Phony targets
.PHONY: all clean copy-index
