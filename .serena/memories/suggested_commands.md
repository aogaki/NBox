# Suggested Commands for NBox Development

## Build Commands
```bash
# Configure build
cmake -S . -B build

# Build project
cmake --build build

# Clean build
rm -rf build && cmake -S . -B build && cmake --build build
```

## Run Commands
```bash
# Interactive mode with visualization
./build/nbox_sim

# Batch mode with macro file
./build/nbox_sim -m run.mac

# With configuration files
./build/nbox_sim -m run.mac -g geometry.json -d detector.json -s source.root

# Command-line options:
#   -m <file>  Macro file
#   -g <file>  Geometry file (JSON format)
#   -d <file>  Detector description file (JSON format)
#   -s <file>  Source term file (ROOT format)
#   -h         Help message
```

## Testing Commands
Currently no formal test framework is set up. The TDD plan includes creating tests with a framework like Google Test.

## File Operations (macOS Darwin)
```bash
# List files
ls -la

# Find files
find . -name "*.cc"

# Search in files
grep -r "pattern" src/

# Copy files
cp source destination

# Move/rename files
mv old_name new_name
```

## ROOT Commands
```bash
# View ROOT file
root -l output.root

# Create test histogram (from ROOT macro)
root -l create_test_source.C
```

## Git Commands (if repository initialized)
```bash
git status
git add .
git commit -m "message"
git log
```

## Utility
```bash
# Check Geant4 installation
geant4-config --version
geant4-config --cflags
geant4-config --libs

# Check ROOT installation
root-config --version
root-config --cflags
root-config --libs
```
