# Main Project Makefile
# Builds optimized release binaries for different target architectures

# ============================================================================
# Project Configuration
# ============================================================================

# Project name
PROJECT = dataprint

# Build directories
BUILD_DIR  := build
BIN_DIR    := $(BUILD_DIR)/bin
OBJ_DIR    := $(BUILD_DIR)/obj

# Source files for your main project
SRC_DIR  := src
TEST_DIR := tests

# All sources, then exclude main.cpp for lib objects (used by tests)
ALL_SOURCES  := $(wildcard $(SRC_DIR)/*.cpp)
MAIN_OBJ     := $(OBJ_DIR)/main.o
LIB_SOURCES  := $(filter-out $(SRC_DIR)/main.cpp, $(ALL_SOURCES))
LIB_OBJECTS  := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(LIB_SOURCES))
ALL_OBJECTS  := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(ALL_SOURCES))

# Test sources (all .cpp in tests/)
TEST_SOURCES := $(wildcard $(TEST_DIR)/*.cpp)
TEST_OBJECTS := $(patsubst $(TEST_DIR)/%.cpp, $(OBJ_DIR)/test_%.o, $(TEST_SOURCES))

# BLAKE3 library location
BLAKE3_DIR = lib/blake3
BLAKE3_LIB = $(BLAKE3_DIR)/libblake3.a

# Include paths for headers
INCLUDES = -Iinclude -I$(BLAKE3_DIR)

# used for the install target
PREFIX ?= /usr/local

# Formatting
CLANG_FORMAT       := clang-format
CLANG_FORMAT_FLAGS := -i --style=file
FORMAT_SOURCES     := $(shell find ./src ./include ./tests -type f \( -name '*.cpp' -o -name '*.h' -o -name '*.c' \))

# ============================================================================
# Architecture-Specific Build Configurations
# ============================================================================

# Default compiler
CC  = gcc
CXX = g++

# Base optimization flags (common to all architectures)
BASE_CFLAGS   = -O3 -flto -DNDEBUG -fPIC
BASE_CXXFLAGS = -O3 -flto -DNDEBUG -fPIC -std=c++17 -DSIMDJSON_THREADS_ENABLED=1
BASE_LDFLAGS  = -flto -lpthread

# Base debug flags
BASE_DEBUG_CFLAGS   = -O3 -flto -DDEBUG -fPIC
BASE_DEBUG_CXXFLAGS = -O3 -flto -DDEBUG -fPIC -std=c++17 -DSIMDJSON_THREADS_ENABLED=1
BASE_DEBUG_LDFLAGS  = -flto -lpthread

# Architecture-specific configurations
# Each target gets optimized for maximum performance on that specific CPU family

# --- x86_64 Targets ---

# Haswell (2013+): AVX2, FMA, BMI1/2 - most modern Intel/AMD CPUs
CFLAGS_haswell   = $(BASE_CFLAGS) -march=haswell -mtune=haswell
CXXFLAGS_haswell = $(BASE_CXXFLAGS) -march=haswell -mtune=haswell
LDFLAGS_haswell  = $(BASE_LDFLAGS)

# Skylake (2015+): Adds AVX-512 support on server CPUs
CFLAGS_skylake   = $(BASE_CFLAGS) -march=skylake-avx512 -mtune=skylake-avx512
CXXFLAGS_skylake = $(BASE_CXXFLAGS) -march=skylake-avx512 -mtune=skylake-avx512
LDFLAGS_skylake  = $(BASE_LDFLAGS)

# Zen 3 (AMD Ryzen 5000 series, 2020+): Modern AMD optimizations
CFLAGS_zen3   = $(BASE_CFLAGS) -march=znver3 -mtune=znver3
CXXFLAGS_zen3 = $(BASE_CXXFLAGS) -march=znver3 -mtune=znver3
LDFLAGS_zen3  = $(BASE_LDFLAGS)

# Zen 4 (AMD Ryzen 7000 series, 2022+): Latest AMD with AVX-512
CFLAGS_zen4   = $(BASE_CFLAGS) -march=znver4 -mtune=znver4
CXXFLAGS_zen4 = $(BASE_CXXFLAGS) -march=znver4 -mtune=znver4
LDFLAGS_zen4  = $(BASE_LDFLAGS)

# Generic x86-64-v3: Broad compatibility with AVX2 (Haswell-era and newer)
CFLAGS_x86_64_v3   = $(BASE_CFLAGS) -march=x86-64-v3 -mtune=generic
CXXFLAGS_x86_64_v3 = $(BASE_CXXFLAGS) -march=x86-64-v3 -mtune=generic
LDFLAGS_x86_64_v3  = $(BASE_LDFLAGS)

# --- ARM Targets ---

# ARM Cortex-A72 (Raspberry Pi 4, many embedded systems)
CFLAGS_cortexa72   = $(BASE_CFLAGS) -march=armv8-a+crc+simd -mtune=cortex-a72
CXXFLAGS_cortexa72 = $(BASE_CXXFLAGS) -march=armv8-a+crc+simd -mtune=cortex-a72
LDFLAGS_cortexa72  = $(BASE_LDFLAGS)

# Apple Silicon (M1/M2/M3)
CFLAGS_apple_m1   = $(BASE_CFLAGS) -march=armv8.5-a+fp16+bf16+i8mm -mtune=apple-m1
CXXFLAGS_apple_m1 = $(BASE_CXXFLAGS) -march=armv8.5-a+fp16+bf16+i8mm -mtune=apple-m1
LDFLAGS_apple_m1  = $(BASE_LDFLAGS)

# ARM Neoverse V1 (AWS Graviton3, cloud servers)
CFLAGS_neoversev1   = $(BASE_CFLAGS) -march=armv8.4-a+sve -mtune=neoverse-v1
CXXFLAGS_neoversev1 = $(BASE_CXXFLAGS) -march=armv8.4-a+sve -mtune=neoverse-v1
LDFLAGS_neoversev1  = $(BASE_LDFLAGS)

# Generic ARMv8 with NEON (broad ARM64 compatibility)
CFLAGS_armv8   = $(BASE_CFLAGS) -march=armv8-a+simd -mtune=generic
CXXFLAGS_armv8 = $(BASE_CXXFLAGS) -march=armv8-a+simd -mtune=generic
LDFLAGS_armv8  = $(BASE_LDFLAGS)

# ============================================================================
# Build Targets
# ============================================================================

# Default target: build for native architecture
all: native

# Builds release candidate build; builds all targets
release: all-x86 all-arm all-arch
	@echo "Built release candidate: $(BIN_DIR)/$(PROJECT)"

# Build for the architecture you're compiling on (fastest for local use)
native: CFLAGS   = $(BASE_CFLAGS) -march=native -mtune=native
native: CXXFLAGS = $(BASE_CXXFLAGS) -march=native -mtune=native
native: LDFLAGS  = $(BASE_LDFLAGS)
native: $(BIN_DIR)/$(PROJECT)
	@echo "Built native binary: $(BIN_DIR)/$(PROJECT)"

# --- Architecture-Specific Targets ---

haswell: CFLAGS   = $(CFLAGS_haswell)
haswell: CXXFLAGS = $(CXXFLAGS_haswell)
haswell: LDFLAGS  = $(LDFLAGS_haswell)
haswell: $(BIN_DIR)/$(PROJECT)-haswell
	@echo "Built Haswell-optimized binary: $(BIN_DIR)/$(PROJECT)-haswell"

skylake: CFLAGS   = $(CFLAGS_skylake)
skylake: CXXFLAGS = $(CXXFLAGS_skylake)
skylake: LDFLAGS  = $(LDFLAGS_skylake)
skylake: $(BIN_DIR)/$(PROJECT)-skylake
	@echo "Built Skylake-optimized binary: $(BIN_DIR)/$(PROJECT)-skylake"

zen3: CFLAGS   = $(CFLAGS_zen3)
zen3: CXXFLAGS = $(CXXFLAGS_zen3)
zen3: LDFLAGS  = $(LDFLAGS_zen3)
zen3: $(BIN_DIR)/$(PROJECT)-zen3
	@echo "Built Zen3-optimized binary: $(BIN_DIR)/$(PROJECT)-zen3"

zen4: CFLAGS   = $(CFLAGS_zen4)
zen4: CXXFLAGS = $(CXXFLAGS_zen4)
zen4: LDFLAGS  = $(LDFLAGS_zen4)
zen4: $(BIN_DIR)/$(PROJECT)-zen4
	@echo "Built Zen4-optimized binary: $(BIN_DIR)/$(PROJECT)-zen4"

x86_64_v3: CFLAGS   = $(CFLAGS_x86_64_v3)
x86_64_v3: CXXFLAGS = $(CXXFLAGS_x86_64_v3)
x86_64_v3: LDFLAGS  = $(LDFLAGS_x86_64_v3)
x86_64_v3: $(BIN_DIR)/$(PROJECT)-x86_64_v3
	@echo "Built x86-64-v3 binary: $(BIN_DIR)/$(PROJECT)-x86_64_v3"

cortexa72: CFLAGS   = $(CFLAGS_cortexa72)
cortexa72: CXXFLAGS = $(CXXFLAGS_cortexa72)
cortexa72: LDFLAGS  = $(LDFLAGS_cortexa72)
cortexa72: $(BIN_DIR)/$(PROJECT)-cortexa72
	@echo "Built Cortex-A72 binary: $(BIN_DIR)/$(PROJECT)-cortexa72"

apple_m1: CFLAGS   = $(CFLAGS_apple_m1)
apple_m1: CXXFLAGS = $(CXXFLAGS_apple_m1)
apple_m1: LDFLAGS  = $(LDFLAGS_apple_m1)
apple_m1: $(BIN_DIR)/$(PROJECT)-apple_m1
	@echo "Built Apple M1-optimized binary: $(BIN_DIR)/$(PROJECT)-apple_m1"

neoversev1: CFLAGS   = $(CFLAGS_neoversev1)
neoversev1: CXXFLAGS = $(CXXFLAGS_neoversev1)
neoversev1: LDFLAGS  = $(LDFLAGS_neoversev1)
neoversev1: $(BIN_DIR)/$(PROJECT)-neoversev1
	@echo "Built Neoverse-V1 binary: $(BIN_DIR)/$(PROJECT)-neoversev1"

armv8: CFLAGS   = $(CFLAGS_armv8)
armv8: CXXFLAGS = $(CXXFLAGS_armv8)
armv8: LDFLAGS  = $(LDFLAGS_armv8)
armv8: $(BIN_DIR)/$(PROJECT)-armv8
	@echo "Built generic ARMv8 binary: $(BIN_DIR)/$(PROJECT)-armv8"

# Build all x86_64 variants
all-x86: haswell skylake zen3 zen4 x86_64_v3
	@echo "All x86_64 binaries built"

# Build all ARM variants
all-arm: cortexa72 apple_m1 neoversev1 armv8
	@echo "All ARM binaries built"

# Build everything
all-arch: all-x86 all-arm
	@echo "All architecture-specific binaries built"

# Debug target
debug: CFLAGS   = $(BASE_DEBUG_CFLAGS) -march=native -mtune=native
debug: CXXFLAGS = $(BASE_DEBUG_CXXFLAGS) -march=native -mtune=native
debug: LDFLAGS  = $(BASE_DEBUG_LDFLAGS)
debug: $(BIN_DIR)/$(PROJECT)
	@echo "Built debug native binary: $(BIN_DIR)/$(PROJECT)"

# ============================================================================
# Test Target
# ============================================================================

# Build and run tests using Catch2 amalgamated headers.
# main.cpp is excluded; tests link against all other src objects.
test: CXXFLAGS = $(BASE_DEBUG_CXXFLAGS) -march=native -mtune=native
test: LDFLAGS  = $(BASE_DEBUG_LDFLAGS)
test: $(BIN_DIR)/test_runner
	@echo "Running tests..."
	$(BIN_DIR)/test_runner

$(BIN_DIR)/test_runner: $(LIB_OBJECTS) $(TEST_OBJECTS) $(BLAKE3_LIB) | $(BIN_DIR)
	@echo "Linking test runner..."
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(LIB_OBJECTS) $(TEST_OBJECTS) $(BLAKE3_LIB)

# Compile test source files (Catch2 amalgamated; no -O3/LTO for faster builds)
$(OBJ_DIR)/test_%.o: $(TEST_DIR)/%.cpp | $(OBJ_DIR)
	@echo "Compiling test $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# ============================================================================
# Generic Build Rules
# ============================================================================

# Ensure output directories exist
$(BIN_DIR) $(OBJ_DIR):
	@mkdir -p $@

# Pattern rule for building architecture-specific binaries
$(BIN_DIR)/$(PROJECT)-%: $(ALL_OBJECTS) $(BLAKE3_LIB) | $(BIN_DIR)
	@echo "Linking $@..."
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(ALL_OBJECTS) $(BLAKE3_LIB)

# Build default binary
$(BIN_DIR)/$(PROJECT): $(ALL_OBJECTS) $(BLAKE3_LIB) | $(BIN_DIR)
	@echo "Linking $(PROJECT)..."
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(ALL_OBJECTS) $(BLAKE3_LIB)

# Compile C++ source files into OBJ_DIR
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Build BLAKE3 library with the same flags as main project
$(BLAKE3_LIB): FORCE
	@echo "Building BLAKE3 library with CFLAGS: $(CFLAGS)"
	$(MAKE) -C $(BLAKE3_DIR) clean
	$(MAKE) -C $(BLAKE3_DIR) CC=$(CC) CFLAGS="$(CFLAGS)"

# ============================================================================
# Install/uninstall rules
# ============================================================================

install: $(BIN_DIR)/$(PROJECT)
	install -d $(PREFIX)/bin
	install -m 755 $(BIN_DIR)/$(PROJECT) $(PREFIX)/bin/$(PROJECT)

uninstall:
	rm -f $(PREFIX)/bin/$(PROJECT)

# ============================================================================
# Format Rules
# ============================================================================

format:
	@if [ -z "$(FORMAT_SOURCES)" ]; then \
		echo "No C++ source files found."; \
	else \
		echo "Formatting $(words $(FORMAT_SOURCES)) file(s)..."; \
		$(CLANG_FORMAT) $(CLANG_FORMAT_FLAGS) $(FORMAT_SOURCES); \
		echo "Done."; \
	fi

format-check:
	@if [ -z "$(FORMAT_SOURCES)" ]; then \
		echo "No C++ source files found."; \
	else \
		echo "Checking format of $(words $(FORMAT_SOURCES)) file(s)..."; \
		$(CLANG_FORMAT) --style=file --dry-run --Werror $(FORMAT_SOURCES); \
		echo "All files are correctly formatted."; \
	fi

# ============================================================================
# Benchmark targets
# ============================================================================

benchmark: $(BIN_DIR)/$(PROJECT)
	$(MAKE) -C benchmarks all

# ============================================================================
# Utility Targets
# ============================================================================

FORCE:

# Clean everything
clean:
	@echo "Cleaning..."
	rm -rf $(BUILD_DIR)
	$(MAKE) -C $(BLAKE3_DIR) clean
	@echo "Clean complete"

# Show build configuration
info:
	@echo "=== Build Configuration ==="
	@echo "CC:       $(CC)"
	@echo "CXX:      $(CXX)"
	@echo "CFLAGS:   $(CFLAGS)"
	@echo "CXXFLAGS: $(CXXFLAGS)"
	@echo "LDFLAGS:  $(LDFLAGS)"
	@echo "SOURCES:  $(ALL_SOURCES)"
	@echo "BIN_DIR:  $(BIN_DIR)"
	@echo "OBJ_DIR:  $(OBJ_DIR)"
	@echo "=========================="

# Help message
help:
	@echo "Available targets:"
	@echo "  make              - Build for native architecture"
	@echo "  make haswell      - Build for Intel/AMD Haswell (2013+, AVX2)"
	@echo "  make skylake      - Build for Intel Skylake (AVX-512)"
	@echo "  make zen3         - Build for AMD Zen 3 (Ryzen 5000)"
	@echo "  make zen4         - Build for AMD Zen 4 (Ryzen 7000, AVX-512)"
	@echo "  make x86_64_v3    - Build generic x86-64-v3 (AVX2, broad compat)"
	@echo "  make cortexa72    - Build for ARM Cortex-A72 (RPi4)"
	@echo "  make apple_m1     - Build for Apple Silicon (M1/M2/M3)"
	@echo "  make neoversev1   - Build for ARM Neoverse V1 (Graviton3)"
	@echo "  make armv8        - Build generic ARMv8"
	@echo "  make all-x86      - Build all x86_64 variants"
	@echo "  make all-arm      - Build all ARM variants"
	@echo "  make all-arch     - Build all architecture variants"
	@echo "  make test         - Build and run tests"
	@echo "  make debug        - Build debug binary (native)"
	@echo "  make clean        - Clean all build artifacts"
	@echo "  make info         - Show current build configuration"
	@echo "  make install      - Install to $(PREFIX)/bin (default: /usr/local/bin)"
	@echo "  make uninstall    - Remove installed binary"

.PHONY: all native haswell skylake zen3 zen4 x86_64_v3 cortexa72 apple_m1 neoversev1 armv8 \
        all-x86 all-arm all-arch debug test clean info help FORCE format format-check install uninstall
