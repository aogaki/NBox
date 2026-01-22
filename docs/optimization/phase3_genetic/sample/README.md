# Phase 3: Genetic Algorithm for Detector Placement

Optimize detector placement using genetic algorithms with DEAP (Distributed Evolutionary Algorithms in Python).

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
cd /Users/aogaki/WorkSpace/NBox/docs/optimization/phase3_genetic
python3 -m venv venv
source venv/bin/activate
pip install deap numpy
```

## Execution Steps

### 1. Copy samples

```bash
cd /Users/aogaki/WorkSpace/NBox/docs/optimization/phase3_genetic
cp sample/* .
```

### 2. Run genetic algorithm optimization

#### For 1 MeV neutrons

```bash
python genetic_optimizer.py \
    --nbox-executable ../../build/nbox_sim \
    --detector-file detector.json \
    --macro-file run_1MeV.mac \
    --n-generations 30 \
    --population-size 20 \
    --n-events 10000 \
    --output-dir results_1MeV
```

#### For 10 MeV neutrons

```bash
python genetic_optimizer.py \
    --nbox-executable ../../build/nbox_sim \
    --detector-file detector.json \
    --macro-file run_10MeV.mac \
    --n-generations 30 \
    --population-size 20 \
    --n-events 10000 \
    --output-dir results_10MeV
```

#### For Cf-252 source

```bash
python genetic_optimizer.py \
    --nbox-executable ../../build/nbox_sim \
    --detector-file detector.json \
    --macro-file run_cf252.mac \
    --source-file ../../build/cf252_source.root \
    --n-generations 30 \
    --population-size 20 \
    --n-events 10000 \
    --output-dir results_cf252
```

### 3. Check results

```bash
ls results_*/
```

Output files:
- `evolution_history.csv` - Best efficiency per generation
- `best_config.json` - Geometry configuration of the optimal placement
- `evolution_history.png` - Evolution progress graph

### 4. Verify optimal placement with simulation

```bash
cd ../../build
./nbox_sim -g ../docs/optimization/phase3_genetic/results_1MeV/best_config.json \
           -d ../docs/optimization/phase3_genetic/detector.json \
           run_1MeV.mac
```

## Parameter Description

- `--n-generations`: Number of generations (recommended: 30-50)
- `--population-size`: Population size (recommended: 20-50)
- `--n-events`: Number of events per evaluation (recommended: 10000-100000)
- `--mutation-prob`: Mutation probability (default: 0.2)
- `--crossover-prob`: Crossover probability (default: 0.5)
- `--n-short`: Number of short detectors (default: 28)
- `--n-long`: Number of long detectors (default: 40)

## Comparison with Bayesian Optimization

| Item | Bayesian Optimization (Phase 2) | Genetic Algorithm (Phase 3) |
|------|--------------------------------|----------------------------|
| Search Method | Probabilistic model-based | Evolutionary search |
| Convergence Speed | Fast | Slower |
| Local Optima Avoidance | Moderate | Strong |
| Recommended Use | Initial exploration | Fine-tuning optimization |

## Cleanup

```bash
rm -rf results_*
rm -rf configs/
rm -f *.json *.mac
```
