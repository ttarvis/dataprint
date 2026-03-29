#!/usr/bin/env bash
# generate.sh -- generates JSONL benchmark data files

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BENCH_DIR="$(dirname "$SCRIPT_DIR")"
DATA_DIR="$BENCH_DIR/data"
GENERATE_SCRIPT="$BENCH_DIR/../scripts/generate_flat.py"

mkdir -p "$DATA_DIR"

# Check Python is available
if ! command -v python3 &>/dev/null; then
    echo "Error: python3 is required to generate benchmark data"
    exit 1
fi

# Check generation script exists
if [ ! -f "$GENERATE_SCRIPT" ]; then
    echo "Error: generation script not found at $GENERATE_SCRIPT"
    exit 1
fi

echo "============================================"
echo " Benchmark Data Generation"
echo "============================================"
echo ""

# Generate 1GB file
FILE_1GB="$DATA_DIR/flat_1gb.jsonl"
if [ -f "$FILE_1GB" ]; then
    echo "1GB file already exists, skipping: $FILE_1GB"
else
    echo "Generating 1GB flat JSONL file..."
    python3 "$GENERATE_SCRIPT" "$FILE_1GB" 1
    echo "Done: $FILE_1GB"
fi

echo ""

# Generate 10GB file
FILE_10GB="$DATA_DIR/flat_10gb.jsonl"
if [ -f "$FILE_10GB" ]; then
    echo "10GB file already exists, skipping: $FILE_10GB"
else
    echo "Generating 10GB flat JSONL file..."
    python3 "$GENERATE_SCRIPT" "$FILE_10GB" 10
    echo "Done: $FILE_10GB"
fi

echo ""
echo "Data generation complete."
echo ""
echo "Files:"
ls -lh "$DATA_DIR"/*.jsonl 2>/dev/null || echo "No files found."
