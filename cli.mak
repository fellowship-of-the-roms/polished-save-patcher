SRCS= \
./src/patching/PatchVersion7to8_unorderedmaps.cpp \
./src/patching/FixVersion9RegisteredKeyItems.cpp \
./src/patching/PatchVersion9to10.cpp \
./src/patching/FixVersion9RoamMap.cpp \
./src/patching/PatchVersion8to9.cpp \
./src/patching/FixVersion9PGOBattleEvent.cpp \
./src/patching/PatchVersion7to8.cpp \
./src/patching/FixVersion9MagikarpPlainForm.cpp \
./src/patching/FixVersion9PCWarpID.cpp \
./src/patching/FixVersion8NoForm.cpp \
./src/core/SymbolDatabase.cpp \
./src/core/SaveBinary.cpp \
./src/core/CommonPatchFunctions.cpp \
./src/core/Logging.cpp \
./src/main.cpp \

OBJS=$(SRCS:.cpp=.o)

PROG=polished-patcher-cli

CPPFLAGS=-DCLI_VERSION -Iinclude
CXXFLAGS=-std=gnu++20

GZIP := gzip -kf

RESOURCES_DIR := resources
VERSION_DIRS := resources/version7 resources/version8 resources/version9 resources/version10

# Find all .sym files in version* directories
SYM_FILES := $(foreach DIR, $(VERSION_DIRS), $(wildcard $(DIR)/*.sym))

# We'll produce .sym.filtered files, then compress those
FILTERED_SYM_FILES := $(SYM_FILES:.sym=.sym.filtered)
COMPRESSED_SYM_FILES := $(SYM_FILES:.sym=.sym.gz)
COMPRESSED_SYM_FILES_C := $(SYM_FILES:.sym=.sym.c)
COMPRESSED_SYM_FILES_O := $(COMPRESSED_SYM_FILES_C:.sym.c=.sym.o)

$(info VERSION_DIRS: $(VERSION_DIRS))
$(info SYM_FILES: $(SYM_FILES))
$(info COMPRESSED_SYM_FILES: $(COMPRESSED_SYM_FILES))


$(PROG): $(OBJS) $(COMPRESSED_SYM_FILES_O)
	#$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ -lz
	$(CXX) $(CXXFLAGS) $(OBJS) $(COMPRESSED_SYM_FILES_O) -o $@ -lz $(LDFLAGS)

bin2c: src/bin2c.c
	$(CC) -o $@ $^

%.sym.filtered: %.sym
	python3 tools/filter_sym.py $< $@

# Compress .sym files
compress-symbols: $(COMPRESSED_SYM_FILES)

# Rule to compress .sym files
%.sym.gz: %.sym.filtered
	$(GZIP) -c $< > $@

$(COMPRESSED_SYM_FILES_C): bin2c

%.sym.c: %.sym.gz
	./bin2c $< > $@

clean:
	rm -f $(OBJS) $(PROG) $(COMPRESSED_SYM_FILES_C) $(COMPRESSED_SYM_FILES_O)

.PHONY: clean
