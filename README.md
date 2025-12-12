# NBox - He3 Neutron Detector Optimization

A Geant4 Monte Carlo simulation for optimizing He3 neutron counter tube placements in a plastic moderator box.

## Overview

NBox simulates neutron interactions with He3 gas-filled detector tubes to find optimal detector configurations for maximum neutron detection efficiency. The simulation uses:

- **Multiple He3 tubes** with configurable positions, sizes, and pressures
- **Plastic moderator box** for neutron thermalization
- **JSON-based configuration** for easy geometry modifications
- **ROOT histogram** for neutron energy spectrum input
- **Multi-threaded execution** for fast simulation

## Features

- ✅ **Flexible Configuration**: Define detector types and placements via JSON
- ✅ **Multi-Detector Support**: Simulate multiple He3 tubes simultaneously
- ✅ **Pressure-Dependent Density**: Accurate He3 gas modeling
- ✅ **Detailed Output**: EventID, DetectorID, DetectorName, Energy, Time
- ✅ **Multi-Threading**: Utilizes all available CPU cores
- ✅ **ROOT Integration**: Input energy spectra and output analysis-ready data

## Requirements

- **Geant4** 11.03 or later (with multi-threading support)
- **ROOT** 6.x or later
- **CMake** 3.16 or later
- **C++11** compiler or later

## Building

```bash
# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
make -j$(nproc)
```

## Usage

### Basic Usage

```bash
# Run with default settings
./build/nbox_sim -m run.mac

# Run with custom configuration files
./build/nbox_sim -m run.mac -g geometry.json -d detector.json -s source.root
```

### Command-Line Options

- `-m <file>` : Geant4 macro file
- `-g <file>` : Geometry configuration (JSON)
- `-d <file>` : Detector description (JSON)
- `-s <file>` : Neutron source energy histogram (ROOT)

### Example Macro

```
/run/numberOfThreads 14
/run/initialize
/run/beamOn 100000
```

## Configuration Files

### Detector Description (`detector.json`)

Define detector types with their specifications:

```json
{
  "detectors": [
    {
      "name": "He3_Short",
      "Diameter": 30,
      "Length": 500,
      "WallT": 1,
      "Pressure": 1000
    },
    {
      "name": "He3_Long",
      "Diameter": 50,
      "Length": 1000,
      "WallT": 2,
      "Pressure": 2000
    }
  ]
}
```

**Parameters**:
- `Diameter`: Outer diameter in mm
- `Length`: Tube length in mm
- `WallT`: Aluminum wall thickness in mm
- `Pressure`: He3 gas pressure in kPa

### Geometry Configuration (`geometry.json`)

Define plastic box and detector placements:

```json
{
  "Box": {
    "Type": "Box",
    "x": 1000,
    "y": 1000,
    "z": 1000
  },
  "Placements": [
    {
      "name": "Det1",
      "type": "He3_Short",
      "R": 100,
      "Phi": 0
    },
    {
      "name": "Det2",
      "type": "He3_Short",
      "R": 100,
      "Phi": 90
    }
  ]
}
```

**Box Parameters**:
- `x`, `y`, `z`: Plastic box dimensions in mm

**Placement Parameters**:
- `name`: Unique detector instance name
- `type`: References a detector from detector.json
- `R`: Radial distance from center in mm
- `Phi`: Azimuthal angle in degrees (0° = +X axis)

Tubes are positioned at: `x = R×cos(Phi)`, `y = R×sin(Phi)`, `z = 0`

### Neutron Source (`source.root`)

ROOT file containing a TH1 histogram with neutron energy spectrum (in MeV).

Create example source:
```bash
root -l TODO/create_test_source.C
```

## Output

Simulation produces ROOT files with an ntuple containing:

| Column | Type | Description |
|--------|------|-------------|
| EventID | int | Event number |
| DetectorID | int | Detector instance ID (0, 1, 2, ...) |
| DetectorName | string | Detector instance name |
| Edep_keV | double | Energy deposition in keV |
| Time_ns | double | Hit time in nanoseconds |

**Output Files**:
- `output_run0.root`: Main merged file
- `output_run0_t0.root`, `output_run0_t1.root`, ...: Per-thread files

## Physics

### He3 Neutron Detection

**Reaction**: ³He(n,p)³H
- **Q-value**: 764 keV
- **Products**: Proton (573 keV) + Triton (191 keV)
- **Cross-section**: ~5330 barns at thermal (0.025 eV)

### Detection Process

1. Neutron emitted from source at center
2. Neutron thermalizes in plastic moderator
3. Thermal neutron captured by He3 gas
4. Energy deposited (~764 keV)
5. Hit recorded with detector ID and time

## Project Status

### Completed Phases

- ✅ **Phase 1**: ConfigManager - JSON/ROOT file loading
- ✅ **Phase 2**: Output Enhancement - 5-column ntuple
- ✅ **Phase 3**: Detector ID Assignment - Unique IDs per tube
- ✅ **Phase 4**: He3 Geometry - Multiple tubes with plastic box

### Current Phase

- ⏳ **Phase 5**: Neutron Source - Histogram energy sampling (IN PROGRESS)

## Example Analysis

```cpp
// ROOT analysis script
void analyze() {
    TFile* f = TFile::Open("output_run0.root");
    TTree* t = (TTree*)f->Get("NBox");

    // Plot energy spectrum for detector 0
    t->Draw("Edep_keV", "DetectorID==0");

    // Count hits per detector
    t->Draw("DetectorID", "", "goff");

    // Time distribution
    t->Draw("Time_ns", "Edep_keV>0", "goff");
}
```

## Directory Structure

```
NBox/
├── nbox.cc                    # Main program
├── CMakeLists.txt             # Build configuration
├── README.md                  # This file
├── include/                   # Header files
│   ├── ConfigManager.hh       # Configuration singleton
│   ├── DetectorConstruction.hh
│   ├── NBoxSD.hh              # Sensitive detector
│   └── NBoxHit.hh             # Hit data structure
├── src/                       # Implementation files
├── TODO/                      # Planning & documentation
│   ├── PROJECT_STATUS.md      # Current project status
│   ├── PHASE*_COMPLETE.md     # Phase completion reports
│   ├── sample_geometry.json   # Example geometry config
│   ├── sample_detector.json   # Example detector config
│   └── create_test_source.C   # ROOT macro for test source
└── *.mac                      # Geant4 macro files
```

## Development

### Code Style

- **Classes**: PascalCase
- **Member variables**: fVariableName (prefix 'f')
- **Functions**: PascalCase (following Geant4 conventions)
- **Files**: `.hh` for headers, `.cc` for implementation

### Design Principles

- **KISS**: Keep It Simple, Stupid - no over-engineering
- **TDD**: Test-Driven Development approach
- **Singleton Pattern**: ConfigManager for global configuration

## Contributing

This is a research project for neutron detector optimization. For questions or suggestions, please refer to the TODO directory for detailed planning documents.

## License

[Your license here]

## Authors

[Your name/institution]

## References

1. Geant4 Collaboration. "Geant4 - A Simulation Toolkit." NIM A 506 (2003) 250-303
2. He3 neutron detection: https://www.osti.gov/biblio/4074688
