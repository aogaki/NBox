#!/usr/bin/env python3
"""
Genetic Algorithm Optimization for He-3 Detector Placement
Uses DEAP library for evolutionary optimization
Constrained version: all detectors used, uniform ring types
"""

import random
import json
import argparse
import sys
from pathlib import Path
from datetime import datetime
import shutil
from typing import List, Tuple, Dict, Any
import math

try:
    from deap import base, creator, tools, algorithms
except ImportError:
    print("DEAP library not found. Install with: pip install deap")
    sys.exit(1)

# Add phase2 scripts directory to path for reusing utilities
phase2_scripts = Path(__file__).parent.parent / "phase2_bayesian" / "scripts"
sys.path.insert(0, str(phase2_scripts))

from geometry_generator import (
    save_geometry_config,
    DETECTOR_DIAMETER,
    MIN_GAP,
    BEAM_PIPE_RADIUS,
    BOX_HALF_WIDTH,
    DetectorInventory
)
from run_nbox import NBoxRunner


# Physical constraints [mm]
# MIN_SPACING: slightly conservative value (> DETECTOR_DIAMETER + MIN_GAP = 30.4mm)
# MAX_RADIUS: BOX_HALF_WIDTH - DETECTOR_DIAMETER/2 = 500 - 12.7 = 487.3mm (rounded down)
MIN_SPACING = 31.0  # mm - minimum center-to-center distance between rings
MAX_RADIUS = 487.0  # mm - maximum ring radius to stay within moderator box


def generate_uniform_ring_config(radii: list, ring_types: list, counts: list,
                                  inventory: DetectorInventory,
                                  box_size: tuple = (1000, 1000, 1000),
                                  beam_pipe_diameter: float = 44) -> dict:
    """Generate geometry config where each ring has uniform detector type."""
    placements = []

    for ring_id, (r, ring_type, n) in enumerate(zip(radii, ring_types, counts), start=1):
        if n == 0:
            continue

        ring_name = chr(ord('A') + ring_id - 1)
        angle_step = 360.0 / n

        det_type = inventory.short_type if ring_type == 0 else inventory.long_type

        for i in range(n):
            angle = i * angle_step
            placement = {
                "name": f"{ring_name}{i + 1}",
                "type": det_type,
                "R": round(r, 2),
                "Phi": round(angle, 2)
            }
            placements.append(placement)

    config = {
        "Box": {
            "Type": "Box",
            "x": box_size[0],
            "y": box_size[1],
            "z": box_size[2],
            "BeamPipe": beam_pipe_diameter
        },
        "Placements": placements
    }

    return config


class GeneticOptimizer:
    def __init__(self, build_dir: str, detector_config: str,
                 n_events: int = 10000, source_file: str = None,
                 energy: float = 1.0, energy_unit: str = "MeV",
                 output_dir: str = "results", n_rings: int = 4,
                 n_short: int = 28, n_long: int = 40,
                 short_type: str = "He3_ELIGANT",
                 long_type: str = "He3_ELIGANT_Long"):
        """Initialize genetic optimizer with constraints."""
        self.build_dir = Path(build_dir)
        self.runner = NBoxRunner(build_dir, detector_config)
        self.n_events = n_events
        self.source_file = source_file
        self.energy = energy
        self.energy_unit = energy_unit
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        self.n_rings = n_rings

        self.inventory = DetectorInventory(
            short_type=short_type,
            short_count=n_short,
            long_type=long_type,
            long_count=n_long
        )

        self.configs_dir = self.output_dir / "configs"
        self.configs_dir.mkdir(exist_ok=True)

        self.eval_count = 0
        self.best_efficiency = 0.0
        self.best_individual = None
        self.history = []

        # Setup DEAP
        self._setup_deap()

    def _setup_deap(self):
        """Setup DEAP evolutionary framework."""
        # Create fitness class (maximize efficiency)
        if not hasattr(creator, "FitnessMax"):
            creator.create("FitnessMax", base.Fitness, weights=(1.0,))
        if not hasattr(creator, "Individual"):
            creator.create("Individual", list, fitness=creator.FitnessMax)

        self.toolbox = base.Toolbox()

        # Individual structure:
        # [r1, r2, r3, r4, type1, type2, type3, type4, n1, n2, n3, n4]
        # radii: float [35, MAX_RADIUS]
        # types: int 0 or 1 (0=short, 1=long)
        # counts: int [1, max_per_ring]

        # Attribute generators
        self.toolbox.register("attr_radius", random.uniform, 35, MAX_RADIUS)
        self.toolbox.register("attr_type", random.randint, 0, 1)
        self.toolbox.register("attr_count", random.randint, 4, 30)

        # Individual generator
        self.toolbox.register("individual", self._create_individual)
        self.toolbox.register("population", tools.initRepeat, list, self.toolbox.individual)

        # Genetic operators
        self.toolbox.register("evaluate", self._evaluate)
        self.toolbox.register("mate", self._crossover)
        self.toolbox.register("mutate", self._mutate)
        self.toolbox.register("select", tools.selTournament, tournsize=3)

    def _create_individual(self):
        """Create a random valid individual."""
        max_attempts = 100
        for _ in range(max_attempts):
            # Generate random radii (sorted)
            radii = sorted([random.uniform(35, MAX_RADIUS - (self.n_rings - 1) * MIN_SPACING)
                           for _ in range(self.n_rings)])

            # Ensure minimum spacing
            valid = True
            for i in range(self.n_rings - 1):
                if radii[i+1] - radii[i] < MIN_SPACING:
                    radii[i+1] = radii[i] + MIN_SPACING
                if radii[i+1] > MAX_RADIUS:
                    valid = False
                    break

            if not valid:
                continue

            # Generate ring types
            ring_types = [random.randint(0, 1) for _ in range(self.n_rings)]

            # Count short and long rings
            short_rings = [i for i, t in enumerate(ring_types) if t == 0]
            long_rings = [i for i, t in enumerate(ring_types) if t == 1]

            if len(short_rings) == 0 or len(long_rings) == 0:
                continue

            # Calculate max detectors per ring
            max_per_ring = []
            for r in radii:
                circumference = 2 * math.pi * r
                max_n = int(circumference / (DETECTOR_DIAMETER + MIN_GAP))
                max_per_ring.append(max(max_n, 1))

            # Distribute detectors
            counts = [0] * self.n_rings

            # Distribute short detectors
            total_short_capacity = sum(max_per_ring[i] for i in short_rings)
            if total_short_capacity < self.inventory.short_count:
                continue

            remaining_short = self.inventory.short_count
            for i, idx in enumerate(short_rings):
                if i == len(short_rings) - 1:
                    counts[idx] = remaining_short
                else:
                    max_here = min(max_per_ring[idx], remaining_short - (len(short_rings) - i - 1))
                    min_here = max(1, remaining_short - sum(max_per_ring[j] for j in short_rings[i+1:]))
                    if min_here > max_here:
                        break
                    counts[idx] = random.randint(min_here, max_here)
                    remaining_short -= counts[idx]

            if remaining_short != 0:
                continue

            # Distribute long detectors
            total_long_capacity = sum(max_per_ring[i] for i in long_rings)
            if total_long_capacity < self.inventory.long_count:
                continue

            remaining_long = self.inventory.long_count
            for i, idx in enumerate(long_rings):
                if i == len(long_rings) - 1:
                    counts[idx] = remaining_long
                else:
                    max_here = min(max_per_ring[idx], remaining_long - (len(long_rings) - i - 1))
                    min_here = max(1, remaining_long - sum(max_per_ring[j] for j in long_rings[i+1:]))
                    if min_here > max_here:
                        break
                    counts[idx] = random.randint(min_here, max_here)
                    remaining_long -= counts[idx]

            if remaining_long != 0:
                continue

            # Validate counts
            valid = True
            for i, (c, m) in enumerate(zip(counts, max_per_ring)):
                if c < 1 or c > m:
                    valid = False
                    break

            if valid:
                individual = radii + ring_types + counts
                return creator.Individual(individual)

        # Fallback: return a known valid configuration
        # radii=[50,100,150,200]mm, types=[S,L,S,L], counts=[7,20,21,20]
        # This satisfies: 7+21=28 short, 20+20=40 long = 68 total
        return creator.Individual([50, 100, 150, 200, 0, 1, 0, 1, 7, 20, 21, 20])

    def _decode_individual(self, individual) -> Tuple[List[float], List[int], List[int]]:
        """Decode individual into radii, types, counts."""
        radii = individual[:self.n_rings]
        ring_types = individual[self.n_rings:2*self.n_rings]
        counts = individual[2*self.n_rings:]
        return radii, ring_types, counts

    def _is_valid(self, individual) -> Tuple[bool, str]:
        """Check if individual is valid."""
        radii, ring_types, counts = self._decode_individual(individual)

        # Check radii bounds
        if radii[0] < 35 or radii[-1] > MAX_RADIUS:
            return False, "Radii out of bounds"

        # Check radii spacing
        for i in range(self.n_rings - 1):
            if radii[i+1] - radii[i] < MIN_SPACING:
                return False, "Insufficient radii spacing"

        # Check ring types
        short_rings = [i for i, t in enumerate(ring_types) if t == 0]
        long_rings = [i for i, t in enumerate(ring_types) if t == 1]

        if len(short_rings) == 0 or len(long_rings) == 0:
            return False, "Must have both short and long rings"

        # Check counts
        for i, (r, c) in enumerate(zip(radii, counts)):
            circumference = 2 * math.pi * r
            max_n = int(circumference / (DETECTOR_DIAMETER + MIN_GAP))
            if c < 1 or c > max_n:
                return False, f"Ring {i+1} count {c} invalid (max {max_n})"

        # Check inventory
        total_short = sum(counts[i] for i in short_rings)
        total_long = sum(counts[i] for i in long_rings)

        if total_short != self.inventory.short_count:
            return False, f"Short count {total_short} != {self.inventory.short_count}"
        if total_long != self.inventory.long_count:
            return False, f"Long count {total_long} != {self.inventory.long_count}"

        return True, ""

    def _evaluate(self, individual) -> Tuple[float]:
        """Evaluate fitness of an individual."""
        self.eval_count += 1

        # Check validity
        is_valid, error = self._is_valid(individual)
        if not is_valid:
            return (0.0,)

        radii, ring_types, counts = self._decode_individual(individual)

        # Generate config
        try:
            config = generate_uniform_ring_config(radii, ring_types, counts, self.inventory)
        except Exception as e:
            return (0.0,)

        # Save config
        config_path = self.configs_dir / f"eval_{self.eval_count:04d}.json"
        config_path = config_path.resolve()
        save_geometry_config(config, str(config_path))

        # Calculate totals
        short_rings = [i for i, t in enumerate(ring_types) if t == 0]
        long_rings = [i for i, t in enumerate(ring_types) if t == 1]
        total_short = sum(counts[i] for i in short_rings)
        total_long = sum(counts[i] for i in long_rings)

        type_names = ['S' if t == 0 else 'L' for t in ring_types]
        print(f"  Eval {self.eval_count}: r={[f'{r:.1f}' for r in radii]}, "
              f"types={type_names}, n={counts}, total={sum(counts)} ({total_short}S+{total_long}L)")

        # Run simulation
        self.runner.cleanup_thread_files()

        result = self.runner.run_simulation(
            geometry_config=str(config_path),
            nevents=self.n_events,
            source_file=self.source_file,
            energy=self.energy,
            energy_unit=self.energy_unit
        )

        if not result["success"]:
            print(f"  Eval {self.eval_count}: Simulation failed")
            return (0.0,)

        # Calculate efficiency
        eff_result = self.runner.calculate_efficiency(
            result["thread_files"],
            self.n_events
        )

        efficiency = eff_result.get("efficiency", 0.0)
        print(f"  Eval {self.eval_count}: Efficiency = {efficiency:.4f}%")

        # Track best
        if efficiency > self.best_efficiency:
            self.best_efficiency = efficiency
            self.best_individual = list(individual)
            # Copy best config
            shutil.copy(config_path, self.output_dir / "best_geometry.json")

        # Record history
        self.history.append({
            "eval": self.eval_count,
            "efficiency": efficiency,
            "radii": radii,
            "ring_types": ring_types,
            "counts": counts,
            "total_short": total_short,
            "total_long": total_long
        })

        return (efficiency,)

    def _crossover(self, ind1, ind2):
        """Custom crossover preserving constraints."""
        # Two-point crossover on radii only
        if random.random() < 0.5:
            # Swap radii
            for i in range(self.n_rings):
                if random.random() < 0.5:
                    ind1[i], ind2[i] = ind2[i], ind1[i]

            # Sort radii
            radii1 = sorted(ind1[:self.n_rings])
            radii2 = sorted(ind2[:self.n_rings])

            for i in range(self.n_rings):
                ind1[i] = radii1[i]
                ind2[i] = radii2[i]

        # Swap ring types (keeping valid inventory)
        if random.random() < 0.3:
            # Swap all types
            for i in range(self.n_rings, 2*self.n_rings):
                ind1[i], ind2[i] = ind2[i], ind1[i]

        # Repair individuals
        self._repair(ind1)
        self._repair(ind2)

        return ind1, ind2

    def _mutate(self, individual):
        """Custom mutation preserving constraints."""
        # Mutate radii
        for i in range(self.n_rings):
            if random.random() < 0.2:
                delta = random.gauss(0, 20)
                individual[i] += delta
                individual[i] = max(35, min(MAX_RADIUS, individual[i]))

        # Sort radii
        radii = sorted(individual[:self.n_rings])
        for i in range(self.n_rings):
            individual[i] = radii[i]

        # Ensure spacing
        for i in range(self.n_rings - 1):
            if individual[i+1] - individual[i] < MIN_SPACING:
                individual[i+1] = individual[i] + MIN_SPACING

        # Mutate ring types
        if random.random() < 0.1:
            # Swap two ring types
            i, j = random.sample(range(self.n_rings), 2)
            individual[self.n_rings + i], individual[self.n_rings + j] = \
                individual[self.n_rings + j], individual[self.n_rings + i]

        # Repair individual
        self._repair(individual)

        return (individual,)

    def _repair(self, individual):
        """Repair individual to satisfy constraints."""
        radii, ring_types, counts = self._decode_individual(individual)

        # Ensure radii spacing
        for i in range(self.n_rings - 1):
            if radii[i+1] - radii[i] < MIN_SPACING:
                radii[i+1] = radii[i] + MIN_SPACING

        # Clamp radii
        for i in range(self.n_rings):
            radii[i] = max(35, min(MAX_RADIUS, radii[i]))

        # Update individual radii
        for i in range(self.n_rings):
            individual[i] = radii[i]

        # Ensure valid ring types
        short_rings = [i for i, t in enumerate(ring_types) if t == 0]
        long_rings = [i for i, t in enumerate(ring_types) if t == 1]

        if len(short_rings) == 0:
            # Make first ring short
            individual[self.n_rings] = 0
            short_rings = [0]
            long_rings = [i for i in range(1, self.n_rings)]

        if len(long_rings) == 0:
            # Make last ring long
            individual[2*self.n_rings - 1] = 1
            long_rings = [self.n_rings - 1]
            short_rings = [i for i in range(self.n_rings - 1)]

        ring_types = individual[self.n_rings:2*self.n_rings]
        short_rings = [i for i, t in enumerate(ring_types) if t == 0]
        long_rings = [i for i, t in enumerate(ring_types) if t == 1]

        # Calculate max per ring
        max_per_ring = []
        for r in radii:
            circumference = 2 * math.pi * r
            max_n = int(circumference / (DETECTOR_DIAMETER + MIN_GAP))
            max_per_ring.append(max(max_n, 1))

        # Redistribute short detectors
        total_short_capacity = sum(max_per_ring[i] for i in short_rings)
        if total_short_capacity >= self.inventory.short_count:
            remaining = self.inventory.short_count
            for i, idx in enumerate(short_rings):
                if i == len(short_rings) - 1:
                    individual[2*self.n_rings + idx] = remaining
                else:
                    share = remaining // (len(short_rings) - i)
                    share = max(1, min(share, max_per_ring[idx]))
                    individual[2*self.n_rings + idx] = share
                    remaining -= share

        # Redistribute long detectors
        total_long_capacity = sum(max_per_ring[i] for i in long_rings)
        if total_long_capacity >= self.inventory.long_count:
            remaining = self.inventory.long_count
            for i, idx in enumerate(long_rings):
                if i == len(long_rings) - 1:
                    individual[2*self.n_rings + idx] = remaining
                else:
                    share = remaining // (len(long_rings) - i)
                    share = max(1, min(share, max_per_ring[idx]))
                    individual[2*self.n_rings + idx] = share
                    remaining -= share

    def optimize(self, n_generations: int = 20, population_size: int = 20) -> Dict[str, Any]:
        """Run genetic optimization."""
        print(f"Starting GENETIC ALGORITHM optimization")
        print(f"  Constraints:")
        print(f"    - All detectors must be used: {self.inventory.short_count} short + "
              f"{self.inventory.long_count} long = {self.inventory.short_count + self.inventory.long_count} total")
        print(f"    - Each ring uses only one detector type")
        print(f"  Population size: {population_size}")
        print(f"  Generations: {n_generations}")
        print(f"  Events per simulation: {self.n_events}")
        if self.source_file:
            print(f"  Source: {self.source_file}")
        else:
            print(f"  Energy: {self.energy} {self.energy_unit}")
        print()

        # Create initial population
        pop = self.toolbox.population(n=population_size)

        # Statistics
        stats = tools.Statistics(lambda ind: ind.fitness.values)
        stats.register("avg", lambda x: sum(v[0] for v in x) / len(x) if x else 0)
        stats.register("max", lambda x: max(v[0] for v in x) if x else 0)
        stats.register("min", lambda x: min(v[0] for v in x) if x else 0)

        # Hall of fame
        hof = tools.HallOfFame(5)

        # Run evolution
        pop, logbook = algorithms.eaSimple(
            pop, self.toolbox,
            cxpb=0.7,  # Crossover probability
            mutpb=0.3,  # Mutation probability
            ngen=n_generations,
            stats=stats,
            halloffame=hof,
            verbose=True
        )

        # Save results
        self._save_results(hof, logbook)

        return {
            "best_efficiency": self.best_efficiency,
            "best_individual": self.best_individual,
            "generations": n_generations,
            "evaluations": self.eval_count
        }

    def _save_results(self, hof, logbook):
        """Save optimization results."""
        if self.best_individual is None:
            print("No valid solution found!")
            return

        radii, ring_types, counts = self._decode_individual(self.best_individual)
        short_rings = [i for i, t in enumerate(ring_types) if t == 0]
        long_rings = [i for i, t in enumerate(ring_types) if t == 1]
        total_short = sum(counts[i] for i in short_rings)
        total_long = sum(counts[i] for i in long_rings)

        best_params = {
            "best_efficiency": self.best_efficiency,
            "radii": radii,
            "ring_types": ['short' if t == 0 else 'long' for t in ring_types],
            "counts": counts,
            "total_detectors": sum(counts),
            "total_short": total_short,
            "total_long": total_long,
            "n_events": self.n_events,
            "n_rings": self.n_rings,
            "total_evaluations": self.eval_count,
            "algorithm": "genetic",
            "constraints": {
                "all_detectors_used": True,
                "uniform_ring_type": True
            },
            "inventory": {
                "short_type": self.inventory.short_type,
                "short_count": self.inventory.short_count,
                "long_type": self.inventory.long_type,
                "long_count": self.inventory.long_count
            }
        }

        with open(self.output_dir / "best_parameters.json", 'w') as f:
            json.dump(best_params, f, indent=2)

        # Save history
        with open(self.output_dir / "optimization_history.json", 'w') as f:
            json.dump(self.history, f, indent=2)

        # Save logbook
        gen_stats = []
        for record in logbook:
            gen_stats.append({
                "gen": record.get("gen", 0),
                "nevals": record.get("nevals", 0),
                "avg": record.get("avg", 0),
                "max": record.get("max", 0),
                "min": record.get("min", 0)
            })

        with open(self.output_dir / "generation_stats.json", 'w') as f:
            json.dump(gen_stats, f, indent=2)

        print(f"\nOptimization complete!")
        print(f"  Best efficiency: {self.best_efficiency:.4f}%")
        print(f"  Best radii: {[f'{r:.1f}' for r in radii]}")
        print(f"  Ring types: {best_params['ring_types']}")
        print(f"  Counts: {counts}")
        print(f"  Total detectors: {sum(counts)} ({total_short} short + {total_long} long)")
        print(f"  Total evaluations: {self.eval_count}")
        print(f"  Results saved to: {self.output_dir}")


def main():
    parser = argparse.ArgumentParser(
        description="Genetic algorithm optimization for detector placement")
    parser.add_argument("--build-dir", required=True, help="NBox build directory")
    parser.add_argument("--detector-config", required=True, help="Detector description JSON")
    parser.add_argument("--n-generations", type=int, default=20, help="Number of generations")
    parser.add_argument("--population", type=int, default=20, help="Population size")
    parser.add_argument("--n-events", type=int, default=10000, help="Events per simulation")
    parser.add_argument("--source-file", help="Source spectrum ROOT file")
    parser.add_argument("--energy", type=float, default=1.0, help="Neutron energy")
    parser.add_argument("--energy-unit", default="MeV", help="Energy unit")
    parser.add_argument("--n-rings", type=int, default=4, choices=[2, 3, 4], help="Number of rings")
    parser.add_argument("--output", default="results_genetic", help="Output directory")

    parser.add_argument("--n-short", type=int, default=28,
                        help="Number of short detectors (must all be used)")
    parser.add_argument("--n-long", type=int, default=40,
                        help="Number of long detectors (must all be used)")
    parser.add_argument("--short-type", default="He3_ELIGANT",
                        help="Short detector type name")
    parser.add_argument("--long-type", default="He3_ELIGANT_Long",
                        help="Long detector type name")

    args = parser.parse_args()

    optimizer = GeneticOptimizer(
        build_dir=args.build_dir,
        detector_config=args.detector_config,
        n_events=args.n_events,
        source_file=args.source_file,
        energy=args.energy,
        energy_unit=args.energy_unit,
        output_dir=args.output,
        n_rings=args.n_rings,
        n_short=args.n_short,
        n_long=args.n_long,
        short_type=args.short_type,
        long_type=args.long_type
    )

    result = optimizer.optimize(
        n_generations=args.n_generations,
        population_size=args.population
    )


if __name__ == "__main__":
    main()
