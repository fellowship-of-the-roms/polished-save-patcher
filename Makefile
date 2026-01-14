# Makefile for polished_save_patcher with WebAssembly

# Detect the operating system
ifeq ($(OS),)
    OS := $(shell uname -s 2>/dev/null || echo Windows_NT)
endif

ifeq ($(OS), Windows_NT)
	SHELL := cmd
else
	SHELL := /usr/bin/bash
endif

# Compiler and flags
CXX := emcc
CXXFLAGS := -Iinclude -std=c++17

# Directories
SRC_DIR := src
INCLUDE_DIR := include
RESOURCES_DIR := resources
VERSION_DIRS := resources/version7 resources/version8 resources/version9 resources/version10
BUILD_DIR := build

# Source files
SOURCES := $(SRC_DIR)/core/CommonPatchFunctions.cpp \
           $(SRC_DIR)/core/SaveBinary.cpp \
           $(SRC_DIR)/core/SymbolDatabase.cpp \
           $(SRC_DIR)/core/Logging.cpp \
           $(SRC_DIR)/patching/PatchVersion7to8.cpp \
           $(SRC_DIR)/patching/PatchVersion7to8_unorderedmaps.cpp \
           $(SRC_DIR)/patching/PatchVersion8to9.cpp \
		   $(SRC_DIR)/patching/PatchVersion9to10.cpp \
           $(SRC_DIR)/patching/FixVersion8NoForm.cpp \
           $(SRC_DIR)/patching/FixVersion9RegisteredKeyItems.cpp \
           $(SRC_DIR)/patching/FixVersion9PCWarpID.cpp \
           $(SRC_DIR)/patching/FixVersion9PGOBattleEvent.cpp \
           $(SRC_DIR)/patching/FixVersion9RoamMap.cpp \
           $(SRC_DIR)/patching/FixVersion9MagikarpPlainForm.cpp \
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
                    $(BUILD_DIR)/index.html \
                    $(BUILD_DIR)/styles.css

LDFLAGS := -s ALLOW_MEMORY_GROWTH=1 -s INITIAL_MEMORY=33554432 # 32MB initial memory

# Windows-specific settings
ifeq ($(OS), Windows_NT)
	CXX := emcc
	RM := del
	GZIP := powershell -ExecutionPolicy Bypass -file tools/gzip.ps1 -k -f
else
	RM := rm -f
	GZIP := gzip -kf
endif

# Find all .sym files in version* directories
SYM_FILES := $(foreach DIR, $(VERSION_DIRS), $(wildcard $(DIR)/*.sym))

# We'll produce .sym.filtered files, then compress those
FILTERED_SYM_FILES := $(SYM_FILES:.sym=.sym.filtered)
COMPRESSED_SYM_FILES := $(SYM_FILES:.sym=.sym.gz)
COMPRESSED_SYM_FILES_CXX := $(SYM_FILES:.sym=.sym.cpp)
COMPRESSED_SYM_FILES_O := $(COMPRESSED_SYM_FILES_CXX:.sym.cpp=.sym.o)


$(info VERSION_DIRS: $(VERSION_DIRS))
$(info SYM_FILES: $(SYM_FILES))
$(info COMPRESSED_SYM_FILES: $(COMPRESSED_SYM_FILES))



# Build target
all: $(BUILD_DIR) compress-symbols $(TARGET) copy-index

# Release target with optimizations
release: CXXFLAGS += -O3
release: LDFLAGS += -O3
release: all

# Create build directory
$(BUILD_DIR):
ifeq ($(OS), Windows_NT)
	if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
else
	mkdir -p $(BUILD_DIR)
endif

bin2c.exe: src/bin2c.c
	$(CC) -o $@ $^

%.sym.filtered: %.sym
	python tools/filter_sym.py $< $@

# Compress .sym files
compress-symbols: $(COMPRESSED_SYM_FILES)

# Rule to compress .sym files
%.sym.gz: %.sym.filtered
	$(GZIP) -c $< > $@

$(COMPRESSED_SYM_FILES_CXX): bin2c.exe

%.sym.cpp: %.sym.gz
	./bin2c.exe -C $< > $@


# Linking
$(TARGET): $(OBJECTS) $(COMPRESSED_SYM_FILES_O)
	$(CXX) $^ -o $@ $(LDFLAGS) -s WASM=1 -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' --bind -sUSE_ZLIB=1

# Compilation
$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ -sUSE_ZLIB=1

# Copy index.html to build directory
copy-index:
ifeq ($(OS), Windows_NT)
	copy index.html $(BUILD_DIR)\index.html
	copy styles.css $(BUILD_DIR)\styles.css
else
	cp index.html $(BUILD_DIR)/index.html
	cp styles.css $(BUILD_DIR)/styles.css
endif


clean:
ifeq ($(OS), Windows_NT)
	powershell -Command "& { \
		$(foreach FILE, $(OBJECTS), Remove-Item -Path '$(FILE)' -Force -ErrorAction SilentlyContinue; ) \
		$(foreach FILE, $(TARGET), Remove-Item -Path '$(FILE)' -Force -ErrorAction SilentlyContinue; ) \
		$(foreach FILE, $(ADDITIONAL_FILES), Remove-Item -Path '$(FILE)' -Force -ErrorAction SilentlyContinue; ) \
		$(foreach FILE, $(FILTERED_SYM_FILES), Remove-Item -Path '$(FILE)' -Force -ErrorAction SilentlyContinue; ) \
		$(foreach FILE, $(COMPRESSED_SYM_FILES), Remove-Item -Path '$(FILE)' -Force -ErrorAction SilentlyContinue; ) \
	}"
	if exist "$(BUILD_DIR)" rmdir /S /Q "$(BUILD_DIR)"
else
	$(RM) $(OBJECTS) $(TARGET) $(ADDITIONAL_FILES) $(FILTERED_SYM_FILES) $(COMPRESSED_SYM_FILES) $(COMPRESSED_SYM_FILES_CXX) $(COMPRESSED_SYM_FILES_O)
	rm -rf $(BUILD_DIR)
endif



# Phony targets
.PHONY: all clean copy-index release compress-symbols
