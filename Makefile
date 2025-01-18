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
VERSION_DIRS := $(wildcard $(RESOURCES_DIR)/version*)
BUILD_DIR := build

# Source files
SOURCES := $(SRC_DIR)/core/CommonPatchFunctions.cpp \
           $(SRC_DIR)/core/SaveBinary.cpp \
           $(SRC_DIR)/core/SymbolDatabase.cpp \
           $(SRC_DIR)/core/Logging.cpp \
           $(SRC_DIR)/patching/PatchVersion7to8.cpp \
           $(SRC_DIR)/patching/PatchVersion7to8_unorderedmaps.cpp \
           $(SRC_DIR)/patching/PatchVersion8to9.cpp \
           $(SRC_DIR)/main.cpp

# Object files
OBJECTS := $(SOURCES:.cpp=.o)

# Executable name
TARGET := $(BUILD_DIR)/polished_save_patcher.html

# Additional output files
ADDITIONAL_FILES := $(BUILD_DIR)/polished_save_patcher.js \
                    $(BUILD_DIR)/polished_save_patcher.wasm \
                    $(BUILD_DIR)/polished_save_patcher.mem \
                    $(BUILD_DIR)/polished_save_patcher.worker.js \
                    $(BUILD_DIR)/index.html

LDFLAGS := -s ALLOW_MEMORY_GROWTH=1 -s INITIAL_MEMORY=33554432 # 32MB initial memory

# Windows-specific settings
ifeq ($(OS), Windows_NT)
	CXX := emcc
	RM := del
	GZIP := gzip.exe
else
	RM := rm -f
	GZIP := gzip
endif

# Find all .sym files in version* directories
SYM_FILES := $(shell find $(VERSION_DIRS) -name '*.sym')

# Compressed symbol files
COMPRESSED_SYM_FILES := $(SYM_FILES:.sym=.sym.gz)

# Build target
all: $(BUILD_DIR) compress-symbols $(TARGET) copy-index

# Release target with optimizations
release: CXXFLAGS += -O3
release: LDFLAGS += -O3
release: all

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compress .sym files
compress-symbols: $(COMPRESSED_SYM_FILES)

# Rule to compress .sym files
%.sym.gz: %.sym
	$(GZIP) -kf $<

# Linking
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS) -s WASM=1 -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' -s FORCE_FILESYSTEM=1 --embed-file resources --exclude-file *.sym --bind -sUSE_ZLIB=1

# Compilation
$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ -sUSE_ZLIB=1

# Copy index.html to build directory
copy-index:
	cp index.html $(BUILD_DIR)/index.html

# Clean
clean:
	$(RM) $(OBJECTS) $(TARGET) $(ADDITIONAL_FILES) $(COMPRESSED_SYM_FILES)

# Phony targets
.PHONY: all clean copy-index release compress-symbols
