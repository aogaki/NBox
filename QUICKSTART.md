# NBox Quick Start Guide

Get up and running with NBox in 5 minutes!

## Prerequisites

This guide assumes you already have:
- ✅ Geant4 11.x installed and environment sourced
- ✅ ROOT 6.x installed and environment sourced
- ✅ CMake 3.16+ installed
- ✅ C++11 compiler available

**Don't have these?** See [example/TUTORIAL.md](example/TUTORIAL.md#appendix-a-installing-prerequisites)

## 1. Build (1 minute)

```bash
# Clone and enter directory
git clone https://github.com/aogaki/NBox.git
cd NBox

# Build
mkdir build && cd build
cmake .. && cmake --build . -j$(nproc)
cd ..
```

## 2. Create Neutron Source (30 seconds)

```bash
root -l -b -q example/create_cf252_source.C
```

This creates `cf252_source.root` - a Californium-252 fission neutron spectrum.

## 3. Run First Simulation (2 minutes)

### Option A: Interactive (Visualization)

```bash
./build/nbox_sim \
    -d example/eligant_tn_detector.json \
    -g example/eligant_tn_geometry.json \
    -s cf252_source.root
```

**What you'll see:**
- 3D geometry with 28 He-3 tubes in polyethylene box
- Rotate view with mouse
- Type `/run/beamOn 10` to see neutron tracks

Exit by clicking "Exit" or typing `exit`.

### Option B: Batch (Fast simulation)

```bash
./build/nbox_sim \
    -d example/eligant_tn_detector.json \
    -g example/eligant_tn_geometry.json \
    -s cf252_source.root \
    -m example/eligant_tn_test.mac
```

**What happens:**
- Simulates 1 million neutron events
- Uses all CPU cores
- Takes ~10-30 minutes
- Creates `output_run0_t*.root` files

## 4. Analyze Results (1 minute)

```bash
root -l output_run0_t0.root
```

```cpp
// In ROOT prompt:

// See data structure
hits->Print()

// Energy spectrum (should see 764 keV peak from He-3 reaction)
hits->Draw("Edep_keV")

// Hits per detector (28 detectors)
hits->Draw("DetectorID")

// 2D: Energy vs Detector
hits->Draw("DetectorID:Edep_keV", "", "colz")

// Count total hits
hits->GetEntries()
```

## Expected Results

**Energy Spectrum:**
- Sharp peak at ~764 keV (He-3 + neutron → proton + triton)
- Some lower energy deposits from partial energy deposition

**Detector Distribution:**
- Inner ring (A: 4 tubes) → Fewer hits
- Middle ring (B: 8 tubes) → Medium hits
- Outer ring (C: 16 tubes) → Most hits (more surface area)

**Detection Efficiency:**
- Fast neutrons (Cf-252): ~0.3-1% (needs moderation)
- Thermal neutrons: ~10-15% (high cross-section)

## Next Steps

### Customize Your Simulation

**Change detector layout:**
```bash
cp example/eligant_tn_geometry.json my_geometry.json
nano my_geometry.json  # Edit positions
./build/nbox_sim -g my_geometry.json -d example/eligant_tn_detector.json -s cf252_source.root
```

**Change detector specs:**
```bash
cp example/eligant_tn_detector.json my_detector.json
nano my_detector.json  # Edit diameter, pressure, length
./build/nbox_sim -g example/eligant_tn_geometry.json -d my_detector.json -s cf252_source.root
```

**Try thermal neutrons:**
```bash
root -l -b -q example/create_thermal_source.C
./build/nbox_sim -s thermal_source.root -d ... -g ... -m ...
```

### Learn More

- **Full Tutorial:** [example/TUTORIAL.md](example/TUTORIAL.md) - Step-by-step guide
- **Configuration:** [docs/CONFIGURATION.md](docs/CONFIGURATION.md) - JSON file details
- **Analysis:** [docs/ANALYSIS.md](docs/ANALYSIS.md) - ROOT analysis examples
- **Physics:** [docs/PHYSICS.md](docs/PHYSICS.md) - Understanding the science
- **Tips:** [docs/TIPS.md](docs/TIPS.md) - Optimization & techniques

## Quick Reference

### Command Template
```bash
./build/nbox_sim \
    -m <macro_file> \
    -g <geometry_json> \
    -d <detector_json> \
    -s <source_root>
```

### Common Macros

**Quick test (100 events):**
```
/run/numberOfThreads 4
/run/initialize
/run/beamOn 100
```

**Production run (10M events):**
```
/run/numberOfThreads 14
/run/initialize
/run/beamOn 10000000
```

## Troubleshooting

**Build fails:**
```bash
# Source Geant4 and ROOT
source /path/to/geant4/bin/geant4.sh
source /path/to/root/bin/thisroot.sh
```

**No visualization:**
- Use batch mode with `-m` flag instead

**No hits in output:**
- Increase events: Edit macro `/run/beamOn` value
- Check source type matches detector (thermal vs fast neutrons)

**Slow simulation:**
- Reduce threads if CPU overheats: `/run/numberOfThreads 4`
- Use smaller event count for testing

---

**Ready to optimize your detector?** Start experimenting! 🚀
