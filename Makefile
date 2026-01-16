# Makefile for polished_save_patcher with WebAssembly

# Detect the operating system
ifeq ($(OS),)
    OS := $(shell uname -s 2>/dev/null || echo Windows_NT)
endif

ifeq ($(OS), Windows_NT)
	SHELL := cmd
	EXE := .exe
else
	SHELL := /usr/bin/bash
	EXE :=
endif

# Compiler and flags
ifeq ($(CLI_VERSION),)
CXX := emcc
CXXFLAGS := -Iinclude -std=c++17
else
# CXX either set via command line or using system default
CXXFLAGS := -Iinclude -std=c++17 -DCLI_VERSION=1
endif

# Directories
SRC_DIR := src
INCLUDE_DIR := include
RESOURCES_DIR := resources
VERSION_DIRS := resources/version7 resources/version8 resources/version9 resources/version10
BUILD_DIR := build
GEN_DIR := $(BUILD_DIR)/generated

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
ifeq ($(CLI_VERSION),)
TARGET := $(BUILD_DIR)/polished_save_patcher.html
else
TARGET := $(BUILD_DIR)/polished_save_patcher$(EXE)
endif

# Additional output files
ADDITIONAL_FILES := $(BUILD_DIR)/polished_save_patcher.js \
                    $(BUILD_DIR)/polished_save_patcher.wasm \
                    $(BUILD_DIR)/polished_save_patcher.mem \
                    $(BUILD_DIR)/polished_save_patcher.worker.js \
                    $(BUILD_DIR)/index.html \
                    $(BUILD_DIR)/styles.css


ifeq ($(CLI_VERSION),)
LDFLAGS := -s ALLOW_MEMORY_GROWTH=1 -s INITIAL_MEMORY=33554432 -s WASM=1 -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' --bind
endif

# Windows-specific settings
ifeq ($(OS), Windows_NT)
	RM := del
else
	RM := rm -f
endif

# Find all .sym files in version* directories
SYM_FILES := $(foreach DIR, $(VERSION_DIRS), $(wildcard $(DIR)/*.sym))

# We'll produce filtered .sym files and embed those directly.
# All generated artifacts live under $(GEN_DIR) so builds don't create untracked
# files in the source tree (e.g. under resources/).
FILTERED_SYM_FILES := $(patsubst %,$(GEN_DIR)/%,$(SYM_FILES))
FILTERED_SYM_FILES_CXX := $(FILTERED_SYM_FILES:.sym=.sym.cpp)
FILTERED_SYM_FILES_O := $(FILTERED_SYM_FILES_CXX:.sym.cpp=.sym.o)


$(info VERSION_DIRS: $(VERSION_DIRS))
$(info SYM_FILES: $(SYM_FILES))
$(info FILTERED_SYM_FILES: $(FILTERED_SYM_FILES))



# Build target
all: $(BUILD_DIR) $(TARGET) copy-index

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

BIN2C := $(BUILD_DIR)/bin2c$(EXE)
ifeq ($(OS), Windows_NT)
BIN2C_RUN := $(subst /,\,$(BIN2C))
else
BIN2C_RUN := $(BIN2C)
endif

$(BIN2C): src/bin2c.c | $(BUILD_DIR)
	$(CC) -o $@ $^

$(GEN_DIR)/%.sym: %.sym
ifeq ($(OS), Windows_NT)
	if not exist "$(dir $@)" mkdir "$(dir $@)"
else
	mkdir -p $(dir $@)
endif
	python3 tools/filter_sym.py $< $@

$(FILTERED_SYM_FILES_CXX): $(BIN2C)

$(GEN_DIR)/%.sym.cpp: $(GEN_DIR)/%.sym
	$(BIN2C_RUN) -C $< > $@


# Linking
$(TARGET): $(OBJECTS) $(FILTERED_SYM_FILES_O)
	$(CXX) $^ -o $@ $(LDFLAGS)

# Compilation
$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

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
		Remove-Item -Path 'bin2c.exe' -Force -ErrorAction SilentlyContinue; \
	}"
	if exist "$(BUILD_DIR)" rmdir /S /Q "$(BUILD_DIR)"
else
	$(RM) $(OBJECTS) bin2c.exe
	rm -rf $(BUILD_DIR)
endif



# Phony targets
.PHONY: all clean copy-index release prune-build


# Remove intermediate/generated artifacts from build/ but keep the web output.
# Intended for CI publishing.
prune-build:
ifeq ($(OS), Windows_NT)
	powershell -Command "& { \
		Remove-Item -Path '$(GEN_DIR)' -Recurse -Force -ErrorAction SilentlyContinue; \
		Remove-Item -Path '$(BIN2C)' -Force -ErrorAction SilentlyContinue; \
	}"
else
	rm -rf $(GEN_DIR)
	$(RM) $(BIN2C)
endif
