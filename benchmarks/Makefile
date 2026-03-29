# Benchmark Makefile
# Run from project root via: make benchmark
# Or from this directory: make all, make generate, make run

BINARY := ../build/bin/dataprint
DATA_DIR := data
RESULTS_DIR := results

.PHONY: all generate run clean help

all: generate run

generate:
	@echo "Generating benchmark data files..."
	@bash scripts/generate.sh

run:
	@echo "Running benchmarks..."
	@mkdir -p $(RESULTS_DIR)
	@bash scripts/run.sh | tee $(RESULTS_DIR)/latest.txt
	@echo ""
	@echo "Results saved to $(RESULTS_DIR)/latest.txt"

clean:
	@echo "Cleaning benchmark data and results..."
	@rm -rf $(DATA_DIR)
	@rm -rf $(RESULTS_DIR)
	@echo "Done."

help:
	@echo "Benchmark targets:"
	@echo "  make all       - Generate files and run benchmarks"
	@echo "  make generate  - Generate test data files only"
	@echo "  make run       - Run benchmarks only (requires generated files)"
	@echo "  make clean     - Remove generated files and results"
