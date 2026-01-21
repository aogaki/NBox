#!/bin/bash
# run_all_energies.sh - Run simulations for all energy points
#
# Usage: ./run_all_energies.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "========================================"
echo "Running all energy simulations"
echo "========================================"

# Energy points
"$SCRIPT_DIR/run_simulation.sh" 1 keV
"$SCRIPT_DIR/run_simulation.sh" 10 keV
"$SCRIPT_DIR/run_simulation.sh" 100 keV
"$SCRIPT_DIR/run_simulation.sh" 1 MeV
"$SCRIPT_DIR/run_simulation.sh" 5 MeV
"$SCRIPT_DIR/run_simulation.sh" 10 MeV

echo "========================================"
echo "All simulations complete!"
echo "========================================"
