# NBox Detector Placement Optimization Manual

**Copy-paste ready** optimization guide.

---

## Quick Start (Just Copy-Paste)

### Step 1: Build NBox (first time only)

```bash
cd /Users/aogaki/WorkSpace/NBox
mkdir -p build && cd build
cmake ..
make -j4
```

### Step 2: Run samples for each phase

Ready-to-use files are available in the `sample/` directory for each phase.

---

## Phase 1: Flux Map (Determine Optimal Radius)

Measure neutron flux spatial distribution without detectors.

```bash
# 1. Go to build directory and copy samples
cd /Users/aogaki/WorkSpace/NBox/build
cp ../docs/optimization/phase1_fluxmap/sample/* .

# 2. Run simulation (-f enables flux map recording)
# For 1 MeV:
./nbox_sim -g geometry.json -d detector.json -f run_1MeV.mac

# For 10 MeV:
./nbox_sim -g geometry.json -d detector.json -f run_10MeV.mac

# For Cf-252:
./nbox_sim -g geometry.json -d detector.json -f --source-file cf252_source.root run_cf252.mac

# 3. Analysis
root -l -b -q 'analyze_flux.C("output_run0_t*.root", "results", "1MeV")'

# 4. Check results
ls results/
# → fluxmap_1MeV_xy.png, radial_1MeV.png
```

**Result**: The peak position of the radial profile indicates the optimal detector placement radius.

Details: [phase1_fluxmap/sample/README.md](phase1_fluxmap/sample/README.md)

---

## Phase 2: Bayesian Optimization (Automatic Detector Placement)

Automatically search for optimal detector placement using Bayesian optimization with Optuna.

```bash
# 1. Set up Python environment (first time only)
cd /Users/aogaki/WorkSpace/NBox/docs/optimization/phase2_bayesian
python3 -m venv venv
source venv/bin/activate
pip install optuna numpy

# 2. Copy samples
cp sample/* .

# 3. Run optimization
# For 1 MeV:
python bayesian_optimizer.py \
    --nbox-executable ../../build/nbox_sim \
    --detector-file detector.json \
    --macro-file run_1MeV.mac \
    --n-trials 50 \
    --n-events 10000 \
    --output-dir results_1MeV

# For 10 MeV:
python bayesian_optimizer.py \
    --nbox-executable ../../build/nbox_sim \
    --detector-file detector.json \
    --macro-file run_10MeV.mac \
    --n-trials 50 \
    --output-dir results_10MeV

# For Cf-252:
python bayesian_optimizer.py \
    --nbox-executable ../../build/nbox_sim \
    --detector-file detector.json \
    --macro-file run_cf252.mac \
    --source-file ../../build/cf252_source.root \
    --n-trials 50 \
    --output-dir results_cf252

# 4. Check results
cat results_1MeV/best_config.json
```

**Result**: `best_config.json` contains the optimal detector placement.

Details: [phase2_bayesian/sample/README.md](phase2_bayesian/sample/README.md)

---

## Phase 3: Genetic Algorithm (Broader Search)

Optimize placement using genetic algorithms with DEAP.

```bash
# 1. Set up Python environment (can share with Phase 2)
cd /Users/aogaki/WorkSpace/NBox/docs/optimization/phase3_genetic
python3 -m venv venv
source venv/bin/activate
pip install deap numpy

# 2. Copy samples
cp sample/* .

# 3. Run optimization
# For 1 MeV:
python genetic_optimizer.py \
    --nbox-executable ../../build/nbox_sim \
    --detector-file detector.json \
    --macro-file run_1MeV.mac \
    --n-generations 30 \
    --population-size 20 \
    --n-events 10000 \
    --output-dir results_1MeV

# For 10 MeV:
python genetic_optimizer.py \
    --nbox-executable ../../build/nbox_sim \
    --detector-file detector.json \
    --macro-file run_10MeV.mac \
    --n-generations 30 \
    --output-dir results_10MeV

# For Cf-252:
python genetic_optimizer.py \
    --nbox-executable ../../build/nbox_sim \
    --detector-file detector.json \
    --macro-file run_cf252.mac \
    --source-file ../../build/cf252_source.root \
    --n-generations 30 \
    --output-dir results_cf252

# 4. Check results
cat results_1MeV/best_config.json
```

Details: [phase3_genetic/sample/README.md](phase3_genetic/sample/README.md)

---

## Verify Optimal Placement

Run simulation with the optimized geometry:

```bash
cd /Users/aogaki/WorkSpace/NBox/build

# Using Phase 2 results
./nbox_sim \
    -g ../docs/optimization/phase2_bayesian/results_1MeV/best_config.json \
    -d ../docs/optimization/phase2_bayesian/detector.json \
    run_1MeV.mac

# Or using Phase 3 results
./nbox_sim \
    -g ../docs/optimization/phase3_genetic/results_1MeV/best_config.json \
    -d ../docs/optimization/phase3_genetic/detector.json \
    run_1MeV.mac
```

---

## File Structure

```
docs/optimization/
├── README.md                          # ← This file
├── phase1_fluxmap/
│   └── sample/
│       ├── detector.json              # Detector definitions
│       ├── geometry.json              # Geometry (empty Placements)
│       ├── run_1MeV.mac               # 1 MeV macro
│       ├── run_10MeV.mac              # 10 MeV macro
│       ├── run_thermal.mac            # Thermal neutron macro
│       ├── run_cf252.mac              # Cf-252 macro
│       ├── analyze_flux.C             # Analysis script
│       └── README.md
├── phase2_bayesian/
│   ├── bayesian_optimizer.py          # Main script
│   ├── geometry_generator.py          # Geometry generation
│   ├── run_nbox.py                    # NBox execution wrapper
│   └── sample/
│       ├── detector.json
│       ├── run_1MeV.mac
│       ├── run_10MeV.mac
│       ├── run_cf252.mac
│       └── README.md
└── phase3_genetic/
    ├── genetic_optimizer.py           # Main script
    └── sample/
        ├── detector.json
        ├── run_1MeV.mac
        ├── run_10MeV.mac
        ├── run_cf252.mac
        └── README.md
```

---

## Key Parameters

| Parameter | Phase 2 | Phase 3 | Recommended |
|-----------|---------|---------|-------------|
| Trials/Generations | `--n-trials` | `--n-generations` | 30-50 |
| Population Size | - | `--population-size` | 20 |
| Events | `--n-events` | `--n-events` | 10000 |
| Short Detectors | `--n-short` | `--n-short` | 28 |
| Long Detectors | `--n-long` | `--n-long` | 40 |

---

## Troubleshooting

### nbox_sim not found
```bash
cd /Users/aogaki/WorkSpace/NBox/build
make -j4
```

### Python packages not found
```bash
source venv/bin/activate
pip install optuna numpy deap
```

### Cf-252 source file not found
```bash
# Check file location
ls /Users/aogaki/WorkSpace/NBox/build/cf252_source.root
```

### ROOT not found
```bash
# Check ROOT path
which root
# Or set up environment
source /opt/ROOT/bin/thisroot.sh
```

---

## Constraints (Reference)

| Item | Value |
|------|-------|
| Detector Diameter | 25.4 mm |
| Minimum Gap | 5 mm |
| Beam Pipe Radius | 22 mm |
| Moderator Size | 1000 × 1000 × 1000 mm |
| Short Detectors (He3_ELIGANT) | 28 units |
| Long Detectors (He3_ELIGANT_Long) | 40 units |
