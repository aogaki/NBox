# NBox Project Overview

## Purpose
NBox is a Geant4 Monte Carlo simulation designed to find optimal He3 neutron counter tube placements in a plastic box. The simulation records energy deposition, detector ID, and timing information to analyze neutron interaction efficiency with different detector arrangements.

## Key Features
- **Multi-threaded Geant4 simulation** (uses all available CPU cores)
- **JSON-based configuration** for geometry and detector parameters
- **ROOT histogram input** for neutron source energy spectrum
- **ROOT ntuple output** with EventID, DetectorID, Edep_keV, Time_ns columns
- **Configurable He3 tube placement** using (R, Phi) cylindrical coordinates

## Tech Stack
- **C++** (C++11 or later)
- **Geant4** (multi-threading enabled) - Physics simulation framework
- **ROOT** - Data analysis and I/O
- **CMake** (>= 3.16) - Build system
- **nlohmann/json** - JSON parsing (header-only library)

## Current State
The project is transitioning from a test LYSO crystal setup to production He3 neutron detector configuration. LYSO code is temporary and will be completely replaced.

## Project Structure
```
/Users/aogaki/WorkSpace/NBox/
├── nbox.cc              # Main entry point with CLI argument parsing
├── CMakeLists.txt       # Build configuration
├── include/             # Header files
│   ├── DetectorConstruction.hh
│   ├── ActionInitialization.hh
│   ├── PrimaryGeneratorAction.hh
│   ├── RunAction.hh
│   ├── EventAction.hh
│   ├── NBoxSD.hh        # Sensitive detector
│   └── NBoxHit.hh       # Hit data structure
├── src/                 # Implementation files
├── TODO/                # Planning documents
│   ├── COMPLETE_PROJECT_PLAN.md  # Full TDD implementation plan
│   └── ConfigManager_Plan.md     # Simple ConfigManager plan
├── build/               # Build directory
└── *.mac                # Geant4 macro files
```

## Development Approach
- **TDD (Test-Driven Development)**: Write tests first, then implement
- **KISS Principle**: Keep It Simple, Stupid - no over-engineering
- **Singleton Pattern**: ConfigManager for centralized configuration access
