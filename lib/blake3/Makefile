# BLAKE3 Library Makefile
# This Makefile builds a high-performance static library (libblake3.a)
# It should be called from the parent project Makefile with appropriate flags

# ============================================================================
# Compiler and Flags (inherited from parent Makefile or use defaults)
# ============================================================================

# C compiler - can be overridden by parent Makefile
CC ?= gcc

# Architecture and optimization flags passed from parent Makefile
# Parent should set CFLAGS with -march, -mtune, and other architecture-specific flags
CFLAGS ?= -O3

# Additional flags for this library
# -std=c99: Use C99 standard (required by BLAKE3)
# -fPIC: Position Independent Code (good practice for libraries)
# -Wall -Wextra: Enable warnings
LOCAL_CFLAGS = -std=c99 -fPIC -Wall -Wextra

# Combine parent flags with local flags
ALL_CFLAGS = $(CFLAGS) $(LOCAL_CFLAGS)

# Archiver for creating static library
AR ?= ar
ARFLAGS = rcs

# ============================================================================
# Source Files
# ============================================================================

# Core BLAKE3 source files (always compiled)
CORE_SOURCES = blake3.c blake3_dispatch.c blake3_portable.c

# SIMD-optimized source files (compiled conditionally based on architecture)
# These provide significant performance improvements on supported platforms
SIMD_SOURCES_X86 = blake3_sse2.c blake3_sse41.c blake3_avx2.c blake3_avx512.c
SIMD_SOURCES_ARM = blake3_neon.c

# ============================================================================
# Object Files and Library Output
# ============================================================================

# Core object files (always built)
CORE_OBJECTS = $(CORE_SOURCES:.c=.o)

# Initialize SIMD objects as empty
SIMD_OBJECTS =

# Detect target architecture from compiler to determine which SIMD files to compile
# We auto-detect the architecture from the compiler's target triple
ARCH := $(shell $(CC) -dumpmachine)

# Check if we're targeting x86_64/AMD64
ifneq (,$(findstring x86_64,$(ARCH))$(findstring amd64,$(ARCH))$(findstring i386,$(ARCH))$(findstring i686,$(ARCH)))
    # On x86/x86_64, build all SIMD variants
    # The dispatcher will select the best one at runtime based on CPU features
    # Even if compiling with -march=haswell, we still build all variants
    # because BLAKE3's runtime dispatch will pick the fastest available
    SIMD_OBJECTS += blake3_sse2.o blake3_sse41.o blake3_avx2.o blake3_avx512.o
endif

# Check if we're targeting ARM/AArch64
ifneq (,$(findstring arm,$(ARCH))$(findstring aarch64,$(ARCH)))
    # On ARM, build NEON variant
    SIMD_OBJECTS += blake3_neon.o
endif

# All object files to be included in the library
ALL_OBJECTS = $(CORE_OBJECTS) $(SIMD_OBJECTS)

# Output static library name
LIBRARY = libblake3.a

# ============================================================================
# Build Rules
# ============================================================================

# Default target: build the static library
all: $(LIBRARY)

# Create the static library from all object files
$(LIBRARY): $(ALL_OBJECTS)
	@echo "Creating static library: $@"
	$(AR) $(ARFLAGS) $@ $^
	@echo "Library built successfully: $(LIBRARY)"

# ============================================================================
# Core Object Compilation Rules
# ============================================================================

# Compile core BLAKE3 files with standard flags
blake3.o: blake3.c
	@echo "Compiling $< (core)"
	$(CC) $(ALL_CFLAGS) -c $< -o $@

blake3_dispatch.o: blake3_dispatch.c
	@echo "Compiling $< (dispatch)"
	$(CC) $(ALL_CFLAGS) -c $< -o $@

blake3_portable.o: blake3_portable.c
	@echo "Compiling $< (portable fallback)"
	$(CC) $(ALL_CFLAGS) -c $< -o $@

# ============================================================================
# SIMD-Optimized Object Compilation Rules (x86_64)
# ============================================================================
# Each SIMD variant is compiled with specific instruction set flags
# This ensures the compiler generates optimized code for each instruction set

# SSE2 - Baseline x86_64 SIMD (supported on all x86_64 processors)
blake3_sse2.o: blake3_sse2.c
	@echo "Compiling $< (SSE2 optimized)"
	$(CC) $(ALL_CFLAGS) -msse2 -c $< -o $@

# SSE4.1 - More advanced SIMD operations
blake3_sse41.o: blake3_sse41.c
	@echo "Compiling $< (SSE4.1 optimized)"
	$(CC) $(ALL_CFLAGS) -msse4.1 -c $< -o $@

# AVX2 - 256-bit SIMD (Intel Haswell and later, AMD Excavator and later)
blake3_avx2.o: blake3_avx2.c
	@echo "Compiling $< (AVX2 optimized)"
	$(CC) $(ALL_CFLAGS) -mavx2 -c $< -o $@

# AVX-512 - 512-bit SIMD (Intel Skylake-X and later server CPUs)
# Uses AVX-512VL and AVX-512F instruction sets
blake3_avx512.o: blake3_avx512.c
	@echo "Compiling $< (AVX-512 optimized)"
	$(CC) $(ALL_CFLAGS) -mavx512f -mavx512vl -c $< -o $@

# ============================================================================
# SIMD-Optimized Object Compilation Rules (ARM)
# ============================================================================

# NEON - ARM's SIMD instruction set (standard on AArch64, optional on 32-bit ARM)
blake3_neon.o: blake3_neon.c
	@echo "Compiling $< (NEON optimized)"
	$(CC) $(ALL_CFLAGS) -c $< -o $@

# ============================================================================
# Utility Targets
# ============================================================================

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(CORE_OBJECTS) blake3_sse2.o blake3_sse41.o blake3_avx2.o blake3_avx512.o blake3_neon.o $(LIBRARY)
	@echo "Clean complete"

# Print configuration (useful for debugging)
info:
	@echo "=== BLAKE3 Library Build Configuration ==="
	@echo "CC:            $(CC)"
	@echo "CFLAGS:        $(CFLAGS)"
	@echo "ALL_CFLAGS:    $(ALL_CFLAGS)"
	@echo "Core objects:  $(CORE_OBJECTS)"
	@echo "SIMD objects:  $(SIMD_OBJECTS)"
	@echo "Library:       $(LIBRARY)"
	@echo "=========================================="

# Declare phony targets (targets that don't represent files)
.PHONY: all clean info
