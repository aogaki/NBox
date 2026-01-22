# Phase 1: Flux Map Measurement

Measure the spatial distribution of thermal neutron flux to determine the optimal detector placement radius.

## File List

```
sample/
├── detector.json    # Detector definitions (required but no placements)
├── geometry.json    # Geometry settings (Placements is empty)
├── run_1MeV.mac     # 1 MeV neutrons
├── run_10MeV.mac    # 10 MeV neutrons
├── run_thermal.mac  # Thermal neutrons (0.0253 eV)
├── run_cf252.mac    # Cf-252 spectrum
└── analyze_flux.C   # Analysis script
```

## Execution Steps

### 1. Go to build directory and copy samples

```bash
cd /Users/aogaki/WorkSpace/NBox/build
cp ../docs/optimization/phase1_fluxmap/sample/* .
```

### 2. Run simulation

#### For 1 MeV neutrons

```bash
./nbox_sim -g geometry.json -d detector.json -f run_1MeV.mac
```

#### For 10 MeV neutrons

```bash
./nbox_sim -g geometry.json -d detector.json -f run_10MeV.mac
```

#### For thermal neutrons

```bash
./nbox_sim -g geometry.json -d detector.json -f run_thermal.mac
```

#### For Cf-252 source

```bash
./nbox_sim -g geometry.json -d detector.json -f --source-file cf252_source.root run_cf252.mac
```

**Note**: The `-f` option enables flux map recording.

### 3. Run analysis

```bash
# For 1 MeV
root -l -b -q 'analyze_flux.C("output_run0_t*.root", "results_1MeV", "1MeV")'

# For 10 MeV
root -l -b -q 'analyze_flux.C("output_run0_t*.root", "results_10MeV", "10MeV")'

# For thermal neutrons
root -l -b -q 'analyze_flux.C("output_run0_t*.root", "results_thermal", "thermal")'

# For Cf-252
root -l -b -q 'analyze_flux.C("output_run0_t*.root", "results_cf252", "cf252")'
```

### 4. Check results

Analysis results are saved in the `results_*` directory:

- `fluxmap_*.root` - Histogram data
- `fluxmap_*_xy.png` - XY plane flux map
- `radial_*.png` - Radial flux profile

The peak position of the radial profile indicates the optimal detector placement radius.

## Example Output

```
=== OPTIMAL RADIUS: 85.0 mm ===
```

Use this value as a reference for Phase 2 detector placement optimization.

## Cleanup

```bash
rm -f output_run0_t*.root
rm -rf results_*
```
