# NBox Configuration Guide

Complete guide to configuring your detector simulation.

## Table of Contents

1. [Overview](#overview)
2. [Detector Description File](#detector-description-file)
3. [Geometry Configuration File](#geometry-configuration-file)
4. [Source Files](#source-files)
5. [Macro Files](#macro-files)
6. [Complete Examples](#complete-examples)

---

## Overview

NBox uses four configuration files:

| File | Format | Purpose | Required |
|------|--------|---------|----------|
| Detector Description | JSON | Define detector tube types | Yes |
| Geometry Configuration | JSON | Define box size and tube placements | Yes |
| Source File | ROOT | Neutron energy spectrum | Yes |
| Macro File | Geant4 | Runtime parameters | Optional |

### Configuration Workflow

```
1. Define detector TYPES     → detector.json
2. Define detector PLACEMENTS → geometry.json
3. Define neutron SOURCE      → source.root
4. Define run PARAMETERS      → run.mac
```

---

## Detector Description File

**Purpose:** Define reusable detector tube specifications.

### File Format

```json
{
  "detectors": [
    {
      "name": "DetectorTypeName",
      "Diameter": <outer diameter in mm>,
      "Length": <tube length in mm>,
      "WallT": <aluminum wall thickness in mm>,
      "Pressure": <He3 gas pressure in kPa>
    }
  ]
}
```

### Parameters Explained

#### `name` (string)
- **Purpose:** Unique identifier for this detector type
- **Used in:** Referenced by geometry file
- **Example:** `"He3_Short"`, `"He3_Long"`, `"He3_ELIGANT"`

#### `Diameter` (float, mm)
- **Purpose:** Outer diameter of the tube
- **Range:** Typically 10-50 mm
- **Common values:**
  - 25.4 mm (1 inch standard)
  - 50.8 mm (2 inch)
- **Effect on detection:** Larger → more surface area → more captures

#### `Length` (float, mm)
- **Purpose:** Active length of the tube
- **Range:** Typically 100-2000 mm
- **Common values:**
  - 500 mm (short tubes)
  - 1000 mm (standard)
  - 1500 mm (long tubes)
- **Effect on detection:** Longer → more volume → more captures
- **Constraint:** Should fit within box dimensions

#### `WallT` (float, mm)
- **Purpose:** Aluminum tube wall thickness
- **Range:** Typically 0.5-2.0 mm
- **Common values:**
  - 0.8 mm (thin wall, less neutron absorption)
  - 1.5 mm (standard)
  - 2.0 mm (thick wall, structural)
- **Effect on detection:** Thicker → more neutron absorption in wall → lower efficiency
- **Trade-off:** Structural strength vs neutron transmission

#### `Pressure` (float, kPa)
- **Purpose:** He-3 gas pressure (determines gas density)
- **Range:** Typically 100-1000 kPa (1-10 atmospheres)
- **Common values:**
  - 101.3 kPa (1 atm)
  - 405.3 kPa (4 atm, common commercial)
  - 1013 kPa (10 atm, high efficiency)
- **Effect on detection:** Higher pressure → higher density → more captures per unit length
- **Formula:** Density = (P/P₀) × ρ₀, where P₀ = 101.325 kPa, ρ₀ = 0.166 kg/m³

### Pressure vs Efficiency

| Pressure | Atmospheres | Relative Density | Expected Efficiency |
|----------|-------------|------------------|---------------------|
| 101.3 kPa | 1 atm | 1× | Low |
| 202.6 kPa | 2 atm | 2× | Medium |
| 405.3 kPa | 4 atm | 4× | High |
| 1013 kPa | 10 atm | 10× | Very High |

### Example Detector Descriptions

#### Standard 1-inch Tube
```json
{
  "name": "He3_Standard_1inch",
  "Diameter": 25.4,
  "Length": 1000,
  "WallT": 0.8,
  "Pressure": 405.3
}
```

#### High-Pressure 2-inch Tube
```json
{
  "name": "He3_HighPressure_2inch",
  "Diameter": 50.8,
  "Length": 1500,
  "WallT": 1.5,
  "Pressure": 1013
}
```

#### Custom Short Tube
```json
{
  "name": "He3_Custom_Short",
  "Diameter": 30.0,
  "Length": 500,
  "WallT": 1.0,
  "Pressure": 202.6
}
```

### Multiple Detector Types

You can define multiple types in one file:

```json
{
  "detectors": [
    {
      "name": "He3_Inner",
      "Diameter": 25.4,
      "Length": 500,
      "WallT": 0.8,
      "Pressure": 405.3
    },
    {
      "name": "He3_Outer",
      "Diameter": 50.8,
      "Length": 1000,
      "WallT": 1.5,
      "Pressure": 1013
    }
  ]
}
```

Then use different types in different positions.

---

## Geometry Configuration File

**Purpose:** Define moderator box and detector placements.

### File Format

```json
{
  "Box": {
    "Type": "Box",
    "x": <width in mm>,
    "y": <depth in mm>,
    "z": <height in mm>
  },
  "mono_energy_MeV": <optional: fixed neutron energy in MeV>,
  "Placements": [
    {
      "name": "InstanceName",
      "type": "DetectorTypeName",
      "R": <radial distance in mm>,
      "Phi": <azimuthal angle in degrees>
    }
  ]
}
```

### Box Parameters

#### Material
- **Fixed:** Polyethylene (C₂H₄)
- **Density:** 0.94 g/cm³
- **Purpose:** Moderates fast neutrons to thermal energies

#### `x`, `y`, `z` (float, mm)
- **Purpose:** Box outer dimensions
- **Typical range:** 300-2000 mm
- **Design considerations:**
  - Too small → insufficient moderation
  - Too large → neutrons escape before thermalization
  - Rule of thumb: ~50-100 mm margin around detector array

#### Common Box Sizes

| Application | Dimensions (mm) | Volume | Weight |
|-------------|-----------------|--------|--------|
| Small lab detector | 400 × 400 × 600 | 96 L | ~90 kg |
| ELIGANT-TN | 660 × 660 × 1000 | 436 L | ~410 kg |
| Large array | 1000 × 1000 × 1500 | 1500 L | ~1400 kg |

### Neutron Source Configuration

#### `mono_energy_MeV` (float, optional)
- **Purpose:** Mono-energetic neutron source fallback
- **Units:** MeV (Mega-electron volts)
- **Range:** Typically 0.001 - 20 MeV
- **When used:** Only when no source file (-s option) is provided
- **Priority order:**
  1. Source histogram (ROOT TH1) - if loaded with `-s` option
  2. Source function (ROOT TF1) - if loaded with `-s` option
  3. `mono_energy_MeV` - if specified in this file
  4. Default fallback: 1.0 MeV

**Usage scenarios:**
- **Quick testing:** Simple mono-energetic runs without creating ROOT files
- **Energy scans:** Change energy by editing JSON instead of regenerating histograms
- **Calibration:** Fixed energy source for detector response testing

**Example:**
```json
{
  "Box": { "Type": "Box", "x": 660, "y": 660, "z": 1000 },
  "mono_energy_MeV": 2.5,
  "Placements": [ ... ]
}
```

**Common neutron energies:**
- `0.025e-6` MeV = 0.025 eV (thermal neutrons at room temperature)
- `0.001` MeV = 1 keV (epithermal)
- `1.0` MeV (fast neutrons)
- `2.5` MeV (typical fission neutrons)
- `14.1` MeV (D-T fusion neutrons)

**Note:** If you provide a source file with `-s source.root`, the histogram/function in the file takes precedence over `mono_energy_MeV`.

### Placement Parameters

#### `name` (string)
- **Purpose:** Unique identifier for this detector instance
- **Example:** `"A1"`, `"Det_North"`, `"Tube_042"`
- **Used in:** Output file `DetectorName` column

#### `type` (string)
- **Purpose:** References detector type from detector description file
- **Must match:** Exact name from detector.json
- **Example:** If detector.json has `"He3_Standard"`, use `"type": "He3_Standard"`

#### `R` (float, mm)
- **Purpose:** Radial distance from center (cylindrical coordinates)
- **Range:** 0 to (min(box_x, box_y)/2 - tube_diameter/2)
- **Formula:** Tube center is at distance R from z-axis

#### `Phi` (float, degrees)
- **Purpose:** Azimuthal angle in x-y plane
- **Range:** 0-360°
- **Convention:**
  - 0° = +X axis
  - 90° = +Y axis
  - 180° = -X axis
  - 270° = -Y axis

### Position Calculation

Tubes are positioned using cylindrical coordinates:

```
x = R × cos(Phi)
y = R × sin(Phi)
z = 0  (always centered in height)

Orientation: Tubes parallel to z-axis
```

### Designing Detector Arrays

#### Single Ring

```json
{
  "Placements": [
    {"name": "Det1", "type": "He3_Standard", "R": 100, "Phi": 0},
    {"name": "Det2", "type": "He3_Standard", "R": 100, "Phi": 90},
    {"name": "Det3", "type": "He3_Standard", "R": 100, "Phi": 180},
    {"name": "Det4", "type": "He3_Standard", "R": 100, "Phi": 270}
  ]
}
```

Creates 4 tubes in a square pattern.

#### Multiple Concentric Rings

**Design principle:** More tubes at larger radii for better coverage.

```json
{
  "Placements": [
    // Inner ring: 4 tubes at R=50mm
    {"name": "A1", "type": "He3_Inner", "R": 50, "Phi": 0},
    {"name": "A2", "type": "He3_Inner", "R": 50, "Phi": 90},
    {"name": "A3", "type": "He3_Inner", "R": 50, "Phi": 180},
    {"name": "A4", "type": "He3_Inner", "R": 50, "Phi": 270},

    // Outer ring: 8 tubes at R=120mm
    {"name": "B1", "type": "He3_Outer", "R": 120, "Phi": 0},
    {"name": "B2", "type": "He3_Outer", "R": 120, "Phi": 45},
    {"name": "B3", "type": "He3_Outer", "R": 120, "Phi": 90},
    {"name": "B4", "type": "He3_Outer", "R": 120, "Phi": 135},
    {"name": "B5", "type": "He3_Outer", "R": 120, "Phi": 180},
    {"name": "B6", "type": "He3_Outer", "R": 120, "Phi": 225},
    {"name": "B7", "type": "He3_Outer", "R": 120, "Phi": 270},
    {"name": "B8", "type": "He3_Outer", "R": 120, "Phi": 315}
  ]
}
```

#### Uniform Angular Spacing

For N tubes in a ring at radius R:

```python
# Python helper to generate angles
N = 8  # number of tubes
R = 120  # radius in mm
for i in range(N):
    phi = i * 360.0 / N
    print(f'{{"name": "Det{i+1}", "type": "He3_Standard", "R": {R}, "Phi": {phi}}}')
```

#### ELIGANT-TN Configuration (28 tubes)

```json
{
  "Box": {"Type": "Box", "x": 660, "y": 660, "z": 1000},
  "Placements": [
    // A ring: 4 tubes at 59mm
    {"name": "A1", "type": "He3_ELIGANT", "R": 59, "Phi": 0},
    {"name": "A2", "type": "He3_ELIGANT", "R": 59, "Phi": 90},
    {"name": "A3", "type": "He3_ELIGANT", "R": 59, "Phi": 180},
    {"name": "A4", "type": "He3_ELIGANT", "R": 59, "Phi": 270},

    // B ring: 8 tubes at 130mm
    {"name": "B1", "type": "He3_ELIGANT", "R": 130, "Phi": 0},
    {"name": "B2", "type": "He3_ELIGANT", "R": 130, "Phi": 45},
    // ... (B3-B8)

    // C ring: 16 tubes at 155mm
    {"name": "C1", "type": "He3_ELIGANT", "R": 155, "Phi": 0},
    {"name": "C2", "type": "He3_ELIGANT", "R": 155, "Phi": 22.5},
    // ... (C3-C16)
  ]
}
```

### Avoiding Tube Overlap

**Constraint:** Tubes at same radius must not overlap.

Minimum angular separation:
```
ΔΦ_min = 2 × arcsin(D / (2R))

Where:
  D = tube outer diameter
  R = radial position
```

**Example:** For D=25.4mm at R=100mm:
```
ΔΦ_min = 2 × arcsin(25.4 / 200) = 14.6°
```

Safe spacing: Use 20-30° to avoid edge effects.

---

## Source Files

**Purpose:** Define neutron energy spectrum.

### Supported Formats

NBox supports two ROOT object types:

1. **TH1** (Histogram) - Binned energy distribution
2. **TF1** (Function) - Analytical energy distribution

See [TF1_SOURCE_SUPPORT.md](../TF1_SOURCE_SUPPORT.md) for details.

### TH1 Histogram Source

**When to use:**
- Measured spectra
- Tabulated data
- Complex multi-peak spectra

**Requirements:**
- TH1D or TH1F object
- X-axis: Energy in MeV
- Y-axis: Arbitrary units (normalized internally)

**Example: Create thermal spectrum**
```cpp
// create_thermal_source.C
void create_thermal_source() {
    TH1D* h = new TH1D("thermal_spectrum", "Thermal Neutron", 100, 0, 0.1);

    // Maxwell-Boltzmann at 293K (0.025 eV peak)
    for (int i = 1; i <= h->GetNbinsX(); i++) {
        double E = h->GetBinCenter(i);  // MeV
        double E_eV = E * 1e6;
        double kT = 0.025;  // eV
        double flux = sqrt(E_eV) * exp(-E_eV / kT);
        h->SetBinContent(i, flux);
    }

    TFile* f = TFile::Open("thermal_source.root", "RECREATE");
    h->Write();
    f->Close();
}
```

### TF1 Function Source

**When to use:**
- Well-defined analytical forms
- Smooth distributions
- Memory efficiency needed

**Requirements:**
- TF1 object
- Range: Energy in MeV
- Formula: Any valid ROOT TF1 expression

**Example: Cf-252 Watt spectrum**
```cpp
// create_cf252_source.C
void create_cf252_source() {
    // Watt fission spectrum
    TF1* f = new TF1("cf252_watt_spectrum",
                     "[0] * exp(-x/[1]) * sinh(sqrt([2]*x))",
                     0, 20);  // 0-20 MeV range

    f->SetParameter(0, 1.0);     // Normalization
    f->SetParameter(1, 1.025);   // a = 1.025 MeV
    f->SetParameter(2, 2.926);   // b = 2.926 MeV^-1

    TFile* fout = TFile::Open("cf252_source.root", "RECREATE");
    f->Write();
    fout->Close();
}
```

### Common Neutron Sources

| Source | Type | Energy | Use Case |
|--------|------|--------|----------|
| Thermal | TH1 | ~0.025 eV | Reactor spectrum, moderated |
| Cf-252 | TF1 | 0.7-2 MeV peak | Fission spectrum |
| AmBe | TH1/TF1 | ~4 MeV average | (α,n) reaction |
| DD fusion | TF1 | 2.45 MeV mono | Accelerator |
| DT fusion | TF1 | 14.1 MeV mono | Accelerator |

### Validation

**Single source rule:** Only ONE TH1 or TF1 per file.

```bash
# Check file contents
root -l source.root
# ROOT> .ls

# Should see EITHER:
#   TH1D  neutron_energy;1
# OR:
#   TF1   neutron_spectrum;1
# NOT BOTH
```

---

## Macro Files

**Purpose:** Control Geant4 runtime parameters.

### Basic Macro Structure

```
# Comments start with #

# Set number of threads
/run/numberOfThreads 14

# Initialize geometry and physics
/run/initialize

# Run simulation
/run/beamOn 1000000
```

### Common Commands

#### Threading

```
/run/numberOfThreads <N>
```
- **N = 1:** Single-threaded (debugging)
- **N = nproc:** Use all cores (production)
- **Recommendation:** N = physical cores (not hyperthreads)

#### Event Count

```
/run/beamOn <events>
```
- **100-1000:** Quick test
- **10,000:** Debugging
- **100,000:** Initial optimization
- **1,000,000:** Production statistics
- **10,000,000:** High-precision studies

#### Verbosity (for debugging)

```
/run/verbose 1
/event/verbose 1
/tracking/verbose 1
```

Higher numbers = more output (use for debugging only).

#### Random Seed

```
/random/setSeeds 12345 67890
```

Set for reproducible results.

### Example Macros

#### Quick Test (`test.mac`)
```
/run/numberOfThreads 4
/run/initialize
/run/beamOn 1000
```

#### Production Run (`production.mac`)
```
# Production simulation
/run/numberOfThreads 14
/run/initialize

# Set random seed for reproducibility
/random/setSeeds 123456 789012

# Run 10M events
/run/beamOn 10000000
```

#### Debug Run (`debug.mac`)
```
# Single-threaded for debugging
/run/numberOfThreads 1
/run/verbose 2
/event/verbose 1
/tracking/verbose 0

/run/initialize
/run/beamOn 10
```

---

## Complete Examples

### Example 1: Small 4-Tube Detector

**detector_4tube.json:**
```json
{
  "detectors": [
    {
      "name": "He3_Small",
      "Diameter": 25.4,
      "Length": 500,
      "WallT": 0.8,
      "Pressure": 405.3
    }
  ]
}
```

**geometry_4tube.json:**
```json
{
  "Box": {
    "Type": "Box",
    "x": 400,
    "y": 400,
    "z": 600
  },
  "Placements": [
    {"name": "North", "type": "He3_Small", "R": 80, "Phi": 0},
    {"name": "East", "type": "He3_Small", "R": 80, "Phi": 90},
    {"name": "South", "type": "He3_Small", "R": 80, "Phi": 180},
    {"name": "West", "type": "He3_Small", "R": 80, "Phi": 270}
  ]
}
```

**run_4tube.mac:**
```
/run/numberOfThreads 4
/run/initialize
/run/beamOn 100000
```

**Command:**
```bash
./build/nbox_sim -d detector_4tube.json -g geometry_4tube.json \
                 -s cf252_source.root -m run_4tube.mac
```

### Example 2: Dual-Type Detector Array

**detector_dual.json:**
```json
{
  "detectors": [
    {
      "name": "He3_Inner_High",
      "Diameter": 25.4,
      "Length": 800,
      "WallT": 0.8,
      "Pressure": 1013
    },
    {
      "name": "He3_Outer_Standard",
      "Diameter": 50.8,
      "Length": 1000,
      "WallT": 1.5,
      "Pressure": 405.3
    }
  ]
}
```

**geometry_dual.json:**
```json
{
  "Box": {
    "Type": "Box",
    "x": 800,
    "y": 800,
    "z": 1200
  },
  "Placements": [
    // Inner ring: high-pressure small tubes
    {"name": "Inner_1", "type": "He3_Inner_High", "R": 60, "Phi": 0},
    {"name": "Inner_2", "type": "He3_Inner_High", "R": 60, "Phi": 120},
    {"name": "Inner_3", "type": "He3_Inner_High", "R": 60, "Phi": 240},

    // Outer ring: standard large tubes
    {"name": "Outer_1", "type": "He3_Outer_Standard", "R": 150, "Phi": 0},
    {"name": "Outer_2", "type": "He3_Outer_Standard", "R": 150, "Phi": 60},
    {"name": "Outer_3", "type": "He3_Outer_Standard", "R": 150, "Phi": 120},
    {"name": "Outer_4", "type": "He3_Outer_Standard", "R": 150, "Phi": 180},
    {"name": "Outer_5", "type": "He3_Outer_Standard", "R": 150, "Phi": 240},
    {"name": "Outer_6", "type": "He3_Outer_Standard", "R": 150, "Phi": 300}
  ]
}
```

---

## Best Practices

### Design Checklist

- ✅ Box dimensions provide 50-100mm clearance around detectors
- ✅ Tube length ≤ box z-dimension
- ✅ R + tube_radius < min(box_x, box_y)/2
- ✅ Angular spacing prevents tube overlap
- ✅ All detector types referenced in placements exist
- ✅ All placement names are unique
- ✅ Source file contains exactly one TH1 or TF1

### Optimization Tips

1. **Start simple:** 4 tubes → test → add more
2. **Use visualization:** Check geometry before production runs
3. **Test with low statistics:** 1000 events to verify setup
4. **Scale up gradually:** 100k → 1M → 10M events

### Common Mistakes

❌ **Tube longer than box:** `Length: 1200` in `z: 1000` box
✅ **Solution:** Reduce tube length or increase box size

❌ **Overlapping tubes:** Two tubes at R=50, Phi=10° and Phi=15° with D=25.4mm
✅ **Solution:** Increase angular spacing

❌ **Referencing non-existent type:** `"type": "He3_Standard"` when only `"He3_Small"` defined
✅ **Solution:** Match type names exactly

❌ **Multiple sources:** Both TH1 and TF1 in source.root
✅ **Solution:** Use separate files or remove one object

---

## Next Steps

- **See working examples:** [example/](../example/) directory
- **Learn analysis:** [ANALYSIS.md](ANALYSIS.md)
- **Understand physics:** [PHYSICS.md](PHYSICS.md)
- **Optimization tips:** [TIPS.md](TIPS.md)
