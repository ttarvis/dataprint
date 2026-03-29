#!/usr/bin/env bash
# run.sh -- runs fingerprint benchmarks and records timing results

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BENCH_DIR="$(dirname "$SCRIPT_DIR")"
DATA_DIR="$BENCH_DIR/data"
BINARY="$BENCH_DIR/../build/bin/dataprint"

# Number of runs per test -- best of N is reported
NUM_RUNS=3

# ============================================================================
# Helpers
# ============================================================================

check_prereqs() {
    if [ ! -f "$BINARY" ]; then
        echo "Error: binary not found at $BINARY"
        echo "Run 'make' from the project root first"
        exit 1
    fi

    if [ ! -d "$DATA_DIR" ] || [ -z "$(ls -A "$DATA_DIR"/*.jsonl 2>/dev/null)" ]; then
        echo "Error: no benchmark data files found in $DATA_DIR"
        echo "Run 'make generate' from the benchmarks directory first"
        exit 1
    fi
}

# Run a single timed benchmark and return wall time in seconds
# Usage: run_timed <label> <args...>
run_timed() {
    local label="$1"
    shift
    local best_wall=999999
    local best_user=0
    local best_sys=0
    local best_cpu=0

    for i in $(seq 1 $NUM_RUNS); do
        # Use /usr/bin/time for consistent output across platforms
        if command -v gtime &>/dev/null; then
            TIME_CMD="gtime"
        else
            TIME_CMD="/usr/bin/time"
        fi

        result=$( { $TIME_CMD -f "%e %U %S %P" "$BINARY" "$@" 2>&1 >/dev/null; } 2>&1 )
        wall=$(echo "$result" | awk '{print $1}')
        user=$(echo "$result" | awk '{print $2}')
        sys=$(echo "$result"  | awk '{print $3}')
        cpu=$(echo "$result"  | awk '{print $4}' | tr -d '%')

        # Keep best (lowest) wall time
        if (( $(echo "$wall < $best_wall" | bc -l) )); then
            best_wall=$wall
            best_user=$user
            best_sys=$sys
            best_cpu=$cpu
        fi
    done

    printf "  %-40s  wall: %6.3fs  user: %6.3fs  sys: %5.3fs  cpu: %s%%\n" \
        "$label" "$best_wall" "$best_user" "$best_sys" "$best_cpu"
}

# Warm the OS page cache for a file
warm_cache() {
    local file="$1"
    dd if="$file" of=/dev/null bs=64M 2>/dev/null
}

# ============================================================================
# System Info
# ============================================================================

print_system_info() {
    echo "============================================"
    echo " System Information"
    echo "============================================"
    echo ""

    if command -v lscpu &>/dev/null; then
        echo "CPU:"
        lscpu | grep -E "Model name|CPU\(s\)|Thread\(s\) per core|Core\(s\) per socket|Socket\(s\)" | \
            sed 's/^/  /'
    elif [ "$(uname)" = "Darwin" ]; then
        echo "CPU:"
        sysctl -n machdep.cpu.brand_string | sed 's/^/  /'
        echo "  Cores: $(sysctl -n hw.physicalcpu)"
        echo "  Logical CPUs: $(sysctl -n hw.logicalcpu)"
    fi

    echo ""
    echo "Memory:"
    if command -v free &>/dev/null; then
        free -h | grep Mem | awk '{print "  Total: " $2}'
    elif [ "$(uname)" = "Darwin" ]; then
        sysctl -n hw.memsize | awk '{printf "  Total: %.1fGB\n", $1/1024/1024/1024}'
    fi

    echo ""
    echo "OS:"
    uname -sr | sed 's/^/  /'

    echo ""
    echo "Binary:"
    echo "  $BINARY"
    $BINARY --version 2>/dev/null | sed 's/^/  /' || true

    echo ""
    echo "Date: $(date)"
    echo ""
}

# ============================================================================
# Benchmarks
# ============================================================================

run_benchmarks() {
    echo "============================================"
    echo " Fingerprint Benchmarks"
    echo "============================================"
    echo ""
    echo "Each result is the best of $NUM_RUNS runs with warm cache."
    echo ""

    for file in "$DATA_DIR"/*.jsonl; do
        filename=$(basename "$file")
        filesize=$(du -sh "$file" | cut -f1)

        echo "--------------------------------------------"
        echo " File: $filename ($filesize)"
        echo "--------------------------------------------"
        echo ""

        # Warm the cache
        echo "  Warming cache..."
        warm_cache "$file"
        echo ""

        # Single threaded
        run_timed "single threaded" fingerprint --single "$file"

        # Warm cache again between tests
        warm_cache "$file"

        # Multithreaded
        run_timed "multithreaded" fingerprint "$file"

        echo ""

        # Calculate speedup
        single_time=$(  { /usr/bin/time -f "%e" "$BINARY" fingerprint --single "$file" 2>&1 >/dev/null; } 2>&1 | tail -1 )
        multi_time=$(   { /usr/bin/time -f "%e" "$BINARY" fingerprint         "$file" 2>&1 >/dev/null; } 2>&1 | tail -1 )
        speedup=$(echo "scale=2; $single_time / $multi_time" | bc)
        throughput=$(echo "scale=2; $(stat -c%s "$file" 2>/dev/null || stat -f%z "$file") / $multi_time / 1024 / 1024 / 1024" | bc)

        echo "  Speedup:    ${speedup}x"
        echo "  Throughput: ${throughput} GB/s (multithreaded)"
        echo ""
    done
}

# ============================================================================
# Main
# ============================================================================

check_prereqs
print_system_info
run_benchmarks

echo "============================================"
echo " Benchmark complete"
echo "============================================"
