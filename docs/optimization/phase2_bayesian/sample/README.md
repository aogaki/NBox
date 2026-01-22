# Phase 2: Bayesian Optimization for Detector Placement

Automatically optimize detector placement using Bayesian optimization with Optuna (TPE sampler).

## File List

```
sample/
├── detector.json    # Detector definitions
├── run_1MeV.mac     # 1 MeV neutrons (10000 events)
├── run_10MeV.mac    # 10 MeV neutrons
└── run_cf252.mac    # Cf-252 spectrum
```

## Prerequisites

```bash
cd /Users/aogaki/WorkSpace/NBox/docs/optimization/phase2_bayesian
python3 -m venv venv
source venv/bin/activate
pip install optuna numpy
```

## Execution Steps

### 1. Copy samples

```bash
cd /Users/aogaki/WorkSpace/NBox/docs/optimization/phase2_bayesian
cp sample/* .
```

### 2. Run Bayesian optimization

#### For 1 MeV neutrons

```bash
python bayesian_optimizer.py \
    --nbox-executable ../../build/nbox_sim \
    --detector-file detector.json \
    --macro-file run_1MeV.mac \
    --n-trials 50 \
    --n-events 10000 \
    --output-dir results_1MeV
```

#### For 10 MeV neutrons

```bash
python bayesian_optimizer.py \
    --nbox-executable ../../build/nbox_sim \
    --detector-file detector.json \
    --macro-file run_10MeV.mac \
    --n-trials 50 \
    --n-events 10000 \
    --output-dir results_10MeV
```

#### For Cf-252 source

```bash
python bayesian_optimizer.py \
    --nbox-executable ../../build/nbox_sim \
    --detector-file detector.json \
    --macro-file run_cf252.mac \
    --source-file ../../build/cf252_source.root \
    --n-trials 50 \
    --n-events 10000 \
    --output-dir results_cf252
```

### 3. Check results

```bash
ls results_*/
```

Output files:
- `optimization_history.csv` - History of all trials
- `best_config.json` - Geometry configuration of the optimal placement
- `optimization_history.png` - Optimization progress graph

### 4. Verify optimal placement with simulation

```bash
cd ../../build
./nbox_sim -g ../docs/optimization/phase2_bayesian/results_1MeV/best_config.json \
           -d ../docs/optimization/phase2_bayesian/detector.json \
           run_1MeV.mac
```

## Parameter Description

- `--n-trials`: Number of optimization trials (recommended: 50-100)
- `--n-events`: Number of events per trial (recommended: 10000-100000)
- `--n-short`: Number of short detectors (default: 28)
- `--n-long`: Number of long detectors (default: 40)

## Cleanup

```bash
rm -rf results_*
rm -rf configs/
rm -f *.json *.mac
```
