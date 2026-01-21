#!/bin/bash
# run_simulation.sh - Run NBox simulation and merge results
#
# Usage: ./run_simulation.sh <energy> <unit>
# Example: ./run_simulation.sh 1 keV
#          ./run_simulation.sh 1 MeV

set -e

ENERGY=$1
UNIT=$2
OUTPUT_NAME="${ENERGY}${UNIT}"

# Paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PHASE1_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="/Users/aogaki/WorkSpace/NBox/build"
MACRO_FILE="${PHASE1_DIR}/configs/run_${OUTPUT_NAME}.mac"
OUTPUT_DIR="${PHASE1_DIR}/simulations"

# Check macro file exists
if [ ! -f "$MACRO_FILE" ]; then
    echo "Error: Macro file not found: $MACRO_FILE"
    exit 1
fi

echo "========================================"
echo "Running simulation: ${OUTPUT_NAME}"
echo "Macro file: ${MACRO_FILE}"
echo "========================================"

# Run NBox
cd "$BUILD_DIR"
./NBox -m "$MACRO_FILE"

# Merge thread files
echo "Merging thread files..."
THREAD_FILES=$(ls output_run0_t*.root 2>/dev/null || true)
if [ -n "$THREAD_FILES" ]; then
    hadd -f "${OUTPUT_DIR}/${OUTPUT_NAME}.root" output_run0_t*.root
    echo "Created: ${OUTPUT_DIR}/${OUTPUT_NAME}.root"

    # Cleanup thread files
    rm -f output_run0_t*.root
    echo "Cleaned up thread files"
else
    echo "Warning: No thread files found"
fi

echo "========================================"
echo "Simulation complete: ${OUTPUT_NAME}"
echo "========================================"
