# NBox - He3 Neutron Detector Optimization

A Geant4 Monte Carlo simulation for optimizing He3 neutron counter tube placements in a plastic moderator box.

## Quick Start

**New to NBox?** → See [QUICKSTART.md](QUICKSTART.md) to get running in 5 minutes!

**Want a full tutorial?** → See [example/TUTORIAL.md](example/TUTORIAL.md) for step-by-step instructions.

## Overview

NBox simulates neutron interactions with He3 gas-filled detector tubes to find optimal detector configurations for maximum neutron detection efficiency. The simulation uses:

- **Multiple He3 tubes** with configurable positions, sizes, and pressures
- **Plastic moderator box** for neutron thermalization
- **JSON-based configuration** for easy geometry modifications
- **TH1/TF1 source support** for flexible neutron energy spectra (see [TF1_SOURCE_SUPPORT.md](TF1_SOURCE_SUPPORT.md))
- **Multi-threaded execution** for fast simulation

## Features

- ✅ **Flexible Configuration**: Define detector types and placements via JSON
- ✅ **Multi-Detector Support**: Simulate multiple He3 tubes simultaneously
- ✅ **Dual Source Support**: TH1 histograms or TF1 functions for neutron spectra
- ✅ **Pressure-Dependent Density**: Accurate He3 gas modeling
- ✅ **Detailed Output**: EventID, DetectorID, DetectorName, Energy, Time
- ✅ **Multi-Threading**: Utilizes all available CPU cores
- ✅ **ROOT Integration**: Input energy spectra and output analysis-ready data

## Requirements

- **Geant4** 11.0 or later (with multi-threading support)
- **ROOT** 6.x or later
- **CMake** 3.16 or later
- **C++11** compiler or later

**Installation help:** See [example/TUTORIAL.md - Appendix A](example/TUTORIAL.md#appendix-a-installing-prerequisites)

## Building

```bash
# Clone repository
git clone https://github.com/aogaki/NBox.git
cd NBox

# Create build directory and compile
mkdir build && cd build
cmake .. && cmake --build . -j$(nproc)
cd ..
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

ROOT file containing either:
- **TH1** histogram with neutron energy spectrum (in MeV), OR
- **TF1** function defining the energy spectrum analytically

**Create example sources:**
```bash
# Cf-252 fission spectrum (TF1)
root -l -b -q example/create_cf252_source.C

# Thermal neutron spectrum (TH1)
root -l -b -q example/create_thermal_source.C

# AmBe source spectrum (TH1)
root -l -b -q example/create_AmBe_source.C
```

**See:** [TF1_SOURCE_SUPPORT.md](TF1_SOURCE_SUPPORT.md) for details on TH1 vs TF1 sources.

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

## Documentation

### For New Users

- **[QUICKSTART.md](QUICKSTART.md)** - Get up and running in 5 minutes
- **[example/TUTORIAL.md](example/TUTORIAL.md)** - Complete step-by-step tutorial with installation guide
- **[docs/PHYSICS.md](docs/PHYSICS.md)** - Understanding He-3 neutron detection physics
- **[docs/CONFIGURATION.md](docs/CONFIGURATION.md)** - Detailed guide to JSON configuration files
- **[docs/ANALYSIS.md](docs/ANALYSIS.md)** - ROOT analysis examples and techniques

### For Advanced Users

- **[docs/TIPS.md](docs/TIPS.md)** - Performance optimization and troubleshooting
- **[docs/API.md](docs/API.md)** - Developer reference and code architecture
- **[TF1_SOURCE_SUPPORT.md](TF1_SOURCE_SUPPORT.md)** - TH1 vs TF1 source implementation

### Example Scripts

Located in [example/](example/) directory:
- `create_cf252_source.C` - Cf-252 fission spectrum (TF1)
- `create_thermal_source.C` - Thermal neutron spectrum (TH1)
- `create_AmBe_source.C` - AmBe source spectrum (TH1)
- `analyze_efficiency.C` - Comprehensive efficiency analysis
- `compare_detectors.C` - Compare detector configurations

## Example Analysis

**Quick analysis:**
```bash
root -l output_run0_t0.root
```

```cpp
// In ROOT prompt
hits->Draw("Edep_keV")              // Energy spectrum (should see 764 keV peak)
hits->Draw("DetectorID")            // Hit distribution per detector
hits->Draw("Time_ns")               // Time-of-flight distribution
hits->Draw("DetectorID:Edep_keV", "", "colz")  // 2D plot
```

**Comprehensive analysis:**
```bash
root -l -q 'example/analyze_efficiency.C("output_run0_t0.root", 100000)'
```

**See [docs/ANALYSIS.md](docs/ANALYSIS.md) for more examples.**

## Directory Structure

```
NBox/
├── nbox.cc                       # Main program
├── CMakeLists.txt                # Build configuration
├── README.md                     # This file
├── QUICKSTART.md                 # 5-minute quick start guide
├── TF1_SOURCE_SUPPORT.md         # TH1/TF1 source documentation
├── include/                      # Header files
│   ├── ConfigManager.hh          # Configuration singleton
│   ├── DetectorConstruction.hh   # Geometry builder
│   ├── PrimaryGeneratorAction.hh # Neutron source
│   ├── NBoxSD.hh                 # Sensitive detector
│   └── NBoxHit.hh                # Hit data structure
├── src/                          # Implementation files
├── docs/                         # Comprehensive documentation
│   ├── CONFIGURATION.md          # Configuration file guide
│   ├── ANALYSIS.md               # Analysis techniques
│   ├── PHYSICS.md                # Physics background
│   ├── TIPS.md                   # Optimization tips
│   └── API.md                    # Developer reference
├── example/                      # Example files and scripts
│   ├── TUTORIAL.md               # Step-by-step tutorial
│   ├── eligant_tn_detector.json  # ELIGANT-TN detector config
│   ├── eligant_tn_geometry.json  # ELIGANT-TN geometry
│   ├── eligant_tn_test.mac       # Test macro
│   ├── create_cf252_source.C     # Cf-252 spectrum generator
│   ├── create_thermal_source.C   # Thermal spectrum generator
│   ├── create_AmBe_source.C      # AmBe spectrum generator
│   ├── analyze_efficiency.C      # Efficiency analysis script
│   └── compare_detectors.C       # Configuration comparison
└── *.mac                         # Geant4 macro files
```

## Project Status

### Completed Features

- ✅ **Phase 1**: ConfigManager - JSON/ROOT file loading
- ✅ **Phase 2**: Output Enhancement - 5-column ntuple
- ✅ **Phase 3**: Detector ID Assignment - Unique IDs per tube
- ✅ **Phase 4**: He3 Geometry - Multiple tubes with plastic box
- ✅ **Phase 5**: TH1/TF1 Source Support - Flexible neutron energy spectra
- ✅ **Complete Documentation** - Comprehensive user and developer guides

**Status:** Production-ready ✅

## Development

### For Developers

See [docs/API.md](docs/API.md) for:
- Class architecture and design patterns
- Adding new features
- Code style guidelines
- Build system details

### Code Style

- **Classes**: PascalCase (e.g., `ConfigManager`, `NBoxHit`)
- **Member variables**: fVariableName (prefix 'f')
- **Functions**: PascalCase (following Geant4 conventions)
- **Files**: `.hh` for headers, `.cc` for implementation

### Design Principles

- **KISS**: Keep It Simple, Stupid - no over-engineering
- **Modularity**: Singleton ConfigManager, separate SD/Hit classes
- **Thread-safe**: Multi-threaded execution with per-thread output

## Contributing

This is a research project for neutron detector optimization. Contributions, bug reports, and suggestions are welcome!

## License

[Your license here]

## Authors

[Your name/institution]

## Citation

If you use NBox in your research, please cite:

```
[Your citation here]
```

## References

1. Geant4 Collaboration. "Geant4 - A Simulation Toolkit." NIM A 506 (2003) 250-303
2. He3 neutron detection: https://www.osti.gov/biblio/4074688
3. ISO 8529: Reference neutron radiations for calibration of neutron-measuring devices

## Support

- **Documentation:** See [docs/](docs/) directory
- **Examples:** See [example/](example/) directory
- **Issues:** Report bugs via GitHub Issues
- **Questions:** Refer to documentation or contact maintainers
