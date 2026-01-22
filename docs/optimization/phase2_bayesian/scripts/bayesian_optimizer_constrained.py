#!/usr/bin/env python3
"""
Bayesian Optimization for He-3 Detector Placement
Constrained version:
- All detectors must be used (28 short + 40 long = 68 total)
- Each ring uses only one type of detector (all short or all long)
"""

import optuna
from optuna.samplers import TPESampler
import json
import argparse
import sys
from pathlib import Path
from datetime import datetime
import shutil
import math

# Add scripts directory to path
sys.path.insert(0, str(Path(__file__).parent))

from geometry_generator import (
    save_geometry_config,
    DETECTOR_DIAMETER,
    MIN_GAP,
    BEAM_PIPE_RADIUS,
    BOX_HALF_WIDTH,
    DetectorInventory
)
from run_nbox import NBoxRunner


def generate_uniform_ring_config(radii: list, ring_types: list, counts: list,
                                  inventory: DetectorInventory,
                                  box_size: tuple = (1000, 1000, 1000),
                                  beam_pipe_diameter: float = 44) -> dict:
    """
    Generate geometry config where each ring has uniform detector type.

    Parameters:
    -----------
    radii : list of float
        Ring radii in mm
    ring_types : list of str
        Detector type for each ring ('short' or 'long')
    counts : list of int
        Number of detectors per ring
    inventory : DetectorInventory
        Detector inventory for type names
    """
    placements = []

    for ring_id, (r, ring_type, n) in enumerate(zip(radii, ring_types, counts), start=1):
        if n == 0:
            continue

        ring_name = chr(ord('A') + ring_id - 1)
        angle_step = 360.0 / n

        det_type = inventory.short_type if ring_type == 'short' else inventory.long_type

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


def is_valid_uniform_config(radii: list, counts: list, ring_types: list,
                            inventory: DetectorInventory) -> tuple:
    """
    Validate configuration with uniform ring type constraint.

    Returns (is_valid, error_message)
    """
    n_rings = len(radii)

    # Check beam pipe clearance
    if radii[0] - DETECTOR_DIAMETER / 2 < BEAM_PIPE_RADIUS:
        return False, f"Ring 1 too close to beam pipe"

    # Check box boundary
    if radii[-1] + DETECTOR_DIAMETER / 2 > BOX_HALF_WIDTH:
        return False, f"Outer ring exceeds box boundary"

    # Check ring spacing
    for i in range(n_rings - 1):
        if radii[i+1] - radii[i] < DETECTOR_DIAMETER + MIN_GAP:
            return False, f"Rings {i+1} and {i+2} too close"

    # Check detector spacing within each ring
    for i, (r, n) in enumerate(zip(radii, counts)):
        if n <= 0:
            continue
        circumference = 2 * math.pi * r
        spacing = circumference / n
        if spacing < DETECTOR_DIAMETER + MIN_GAP:
            return False, f"Ring {i+1}: spacing too small"

    # Count total short and long detectors
    total_short = sum(c for c, t in zip(counts, ring_types) if t == 'short')
    total_long = sum(c for c, t in zip(counts, ring_types) if t == 'long')

    # Check exact inventory usage
    if total_short != inventory.short_count:
        return False, f"Short count {total_short} != {inventory.short_count}"
    if total_long != inventory.long_count:
        return False, f"Long count {total_long} != {inventory.long_count}"

    return True, ""


class ConstrainedDetectorOptimizer:
    def __init__(self, build_dir: str, detector_config: str,
                 n_events: int = 10000, source_file: str = None,
                 energy: float = 1.0, energy_unit: str = "MeV",
                 output_dir: str = "results", n_rings: int = 4,
                 n_short: int = 28, n_long: int = 40,
                 short_type: str = "He3_ELIGANT",
                 long_type: str = "He3_ELIGANT_Long"):
        """
        Initialize constrained optimizer.

        Constraints:
        - All detectors must be used (n_short + n_long total)
        - Each ring uses only one type of detector
        """
        self.build_dir = Path(build_dir)
        self.runner = NBoxRunner(build_dir, detector_config)
        self.n_events = n_events
        self.source_file = source_file
        self.energy = energy
        self.energy_unit = energy_unit
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        self.n_rings = n_rings

        # Detector inventory
        self.inventory = DetectorInventory(
            short_type=short_type,
            short_count=n_short,
            long_type=long_type,
            long_count=n_long
        )

        self.total_detectors = n_short + n_long

        # Configuration storage
        self.configs_dir = self.output_dir / "configs"
        self.configs_dir.mkdir(exist_ok=True)

        # Trial counter
        self.trial_count = 0

    def objective(self, trial: optuna.Trial) -> float:
        """
        Objective function with constraints:
        - All detectors must be used
        - Each ring uses uniform detector type
        """
        self.trial_count += 1

        # Physical constraints
        min_spacing = 31.0  # mm
        max_radius = 487.0  # mm (for 1m box)

        # Sample ring types (0 = short, 1 = long)
        ring_types = []
        for i in range(self.n_rings):
            t = trial.suggest_categorical(f'type_{i}', ['short', 'long'])
            ring_types.append(t)

        # Count how many rings are short vs long
        n_short_rings = sum(1 for t in ring_types if t == 'short')
        n_long_rings = self.n_rings - n_short_rings

        # Cannot have zero rings of either type if we need to use all detectors
        if n_short_rings == 0 and self.inventory.short_count > 0:
            return 0.0
        if n_long_rings == 0 and self.inventory.long_count > 0:
            return 0.0

        # Sample radii with proper spacing
        if self.n_rings == 2:
            r1 = trial.suggest_float('r1', 35, 300)
            r2 = trial.suggest_float('r2', r1 + min_spacing, max_radius)
            radii = [r1, r2]
        elif self.n_rings == 3:
            r1 = trial.suggest_float('r1', 35, 200)
            r2 = trial.suggest_float('r2', r1 + min_spacing, 350)
            r3 = trial.suggest_float('r3', r2 + min_spacing, max_radius)
            radii = [r1, r2, r3]
        else:  # 4 rings
            r1 = trial.suggest_float('r1', 35, 150)
            r2 = trial.suggest_float('r2', r1 + min_spacing, 250)
            r3 = trial.suggest_float('r3', r2 + min_spacing, 380)
            r4 = trial.suggest_float('r4', r3 + min_spacing, max_radius)
            radii = [r1, r2, r3, r4]

        # Calculate maximum detectors that can fit in each ring
        max_per_ring = []
        for r in radii:
            circumference = 2 * math.pi * r
            max_n = int(circumference / (DETECTOR_DIAMETER + MIN_GAP))
            max_per_ring.append(max(max_n, 1))

        # Distribute detectors to meet exact inventory constraints
        # Short rings must total exactly n_short, long rings must total exactly n_long
        short_ring_indices = [i for i, t in enumerate(ring_types) if t == 'short']
        long_ring_indices = [i for i, t in enumerate(ring_types) if t == 'long']

        counts = [0] * self.n_rings

        # Distribute short detectors among short rings
        if short_ring_indices:
            remaining_short = self.inventory.short_count
            for idx in short_ring_indices[:-1]:
                max_here = min(max_per_ring[idx], remaining_short - (len(short_ring_indices) - short_ring_indices.index(idx) - 1))
                min_here = max(1, remaining_short - sum(max_per_ring[j] for j in short_ring_indices if j > idx))
                if min_here > max_here:
                    return 0.0  # Cannot satisfy constraint
                n = trial.suggest_int(f'n{idx+1}', min_here, max_here)
                counts[idx] = n
                remaining_short -= n
            # Last short ring gets the remainder
            last_short_idx = short_ring_indices[-1]
            if remaining_short > max_per_ring[last_short_idx] or remaining_short < 1:
                return 0.0
            counts[last_short_idx] = remaining_short

        # Distribute long detectors among long rings
        if long_ring_indices:
            remaining_long = self.inventory.long_count
            for idx in long_ring_indices[:-1]:
                max_here = min(max_per_ring[idx], remaining_long - (len(long_ring_indices) - long_ring_indices.index(idx) - 1))
                min_here = max(1, remaining_long - sum(max_per_ring[j] for j in long_ring_indices if j > idx))
                if min_here > max_here:
                    return 0.0  # Cannot satisfy constraint
                n = trial.suggest_int(f'n{idx+1}', min_here, max_here)
                counts[idx] = n
                remaining_long -= n
            # Last long ring gets the remainder
            last_long_idx = long_ring_indices[-1]
            if remaining_long > max_per_ring[last_long_idx] or remaining_long < 1:
                return 0.0
            counts[last_long_idx] = remaining_long

        # Validate configuration
        is_valid, error = is_valid_uniform_config(radii, counts, ring_types, self.inventory)
        if not is_valid:
            print(f"  Trial {trial.number}: Invalid - {error}")
            return 0.0

        # Generate geometry config
        try:
            config = generate_uniform_ring_config(radii, ring_types, counts, self.inventory)
        except ValueError as e:
            print(f"  Trial {trial.number}: Config error - {e}")
            return 0.0

        # Save config
        config_path = self.configs_dir / f"trial_{trial.number:04d}.json"
        config_path = config_path.resolve()
        save_geometry_config(config, str(config_path))

        # Calculate totals
        total_short = sum(c for c, t in zip(counts, ring_types) if t == 'short')
        total_long = sum(c for c, t in zip(counts, ring_types) if t == 'long')

        # Run simulation
        print(f"  Trial {trial.number}: r={[f'{r:.1f}' for r in radii]}, "
              f"types={ring_types}, n={counts}, "
              f"total={sum(counts)} ({total_short}S+{total_long}L)")

        self.runner.cleanup_thread_files()

        result = self.runner.run_simulation(
            geometry_config=str(config_path),
            nevents=self.n_events,
            source_file=self.source_file,
            energy=self.energy,
            energy_unit=self.energy_unit
        )

        if not result["success"]:
            print(f"  Trial {trial.number}: Simulation failed - {result.get('error', 'Unknown error')}")
            return 0.0

        # Calculate efficiency
        eff_result = self.runner.calculate_efficiency(
            result["thread_files"],
            self.n_events
        )

        efficiency = eff_result.get("efficiency", 0.0)
        print(f"  Trial {trial.number}: Efficiency = {efficiency:.4f}%")

        # Store additional info
        trial.set_user_attr("radii", radii)
        trial.set_user_attr("ring_types", ring_types)
        trial.set_user_attr("counts", counts)
        trial.set_user_attr("total_detectors", sum(counts))
        trial.set_user_attr("total_short", total_short)
        trial.set_user_attr("total_long", total_long)
        trial.set_user_attr("n_hits", eff_result.get("n_hits", 0))

        return efficiency

    def optimize(self, n_trials: int = 100, n_startup_trials: int = 20) -> optuna.Study:
        """Run optimization"""
        study = optuna.create_study(
            direction="maximize",
            sampler=TPESampler(n_startup_trials=n_startup_trials),
            study_name=f"constrained_optimization_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        )

        print(f"Starting CONSTRAINED optimization with {n_trials} trials")
        print(f"  Constraints:")
        print(f"    - All detectors must be used: {self.inventory.short_count} short + {self.inventory.long_count} long = {self.total_detectors} total")
        print(f"    - Each ring uses only one detector type (all short or all long)")
        print(f"  Events per trial: {self.n_events}")
        print(f"  Number of rings: {self.n_rings}")
        if self.source_file:
            print(f"  Source: {self.source_file}")
        else:
            print(f"  Energy: {self.energy} {self.energy_unit}")
        print()

        study.optimize(self.objective, n_trials=n_trials, show_progress_bar=True)

        self._save_results(study)

        return study

    def _save_results(self, study: optuna.Study):
        """Save optimization results"""
        best_params = {
            "best_efficiency": study.best_value,
            "best_params": study.best_params,
            "best_trial": study.best_trial.number,
            "radii": study.best_trial.user_attrs.get("radii", []),
            "ring_types": study.best_trial.user_attrs.get("ring_types", []),
            "counts": study.best_trial.user_attrs.get("counts", []),
            "total_detectors": study.best_trial.user_attrs.get("total_detectors", 0),
            "total_short": study.best_trial.user_attrs.get("total_short", 0),
            "total_long": study.best_trial.user_attrs.get("total_long", 0),
            "n_events": self.n_events,
            "n_rings": self.n_rings,
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

        # Trial history
        history = []
        for trial in study.trials:
            if trial.state == optuna.trial.TrialState.COMPLETE and trial.value > 0:
                history.append({
                    "trial": trial.number,
                    "efficiency": trial.value,
                    "params": trial.params,
                    "radii": trial.user_attrs.get("radii", []),
                    "ring_types": trial.user_attrs.get("ring_types", []),
                    "counts": trial.user_attrs.get("counts", []),
                    "total_detectors": trial.user_attrs.get("total_detectors", 0),
                    "total_short": trial.user_attrs.get("total_short", 0),
                    "total_long": trial.user_attrs.get("total_long", 0)
                })

        with open(self.output_dir / "optimization_history.json", 'w') as f:
            json.dump(history, f, indent=2)

        # Copy best config
        best_config_src = self.configs_dir / f"trial_{study.best_trial.number:04d}.json"
        if best_config_src.exists():
            shutil.copy(best_config_src, self.output_dir / "best_geometry.json")

        print(f"\nOptimization complete!")
        print(f"  Best efficiency: {study.best_value:.4f}%")
        print(f"  Best radii: {best_params['radii']}")
        print(f"  Ring types: {best_params['ring_types']}")
        print(f"  Counts per ring: {best_params['counts']}")
        print(f"  Total detectors: {best_params['total_detectors']} "
              f"({best_params['total_short']} short + {best_params['total_long']} long)")
        print(f"  Results saved to: {self.output_dir}")


def main():
    parser = argparse.ArgumentParser(
        description="Constrained Bayesian optimization for detector placement")
    parser.add_argument("--build-dir", required=True, help="NBox build directory")
    parser.add_argument("--detector-config", required=True, help="Detector description JSON")
    parser.add_argument("--n-trials", type=int, default=50, help="Number of optimization trials")
    parser.add_argument("--n-events", type=int, default=10000, help="Events per simulation")
    parser.add_argument("--source-file", help="Source spectrum ROOT file")
    parser.add_argument("--energy", type=float, default=1.0, help="Neutron energy")
    parser.add_argument("--energy-unit", default="MeV", help="Energy unit")
    parser.add_argument("--n-rings", type=int, default=4, choices=[2, 3, 4], help="Number of rings")
    parser.add_argument("--output", default="results_constrained", help="Output directory")

    # Detector inventory options
    parser.add_argument("--n-short", type=int, default=28,
                        help="Number of short detectors (must all be used)")
    parser.add_argument("--n-long", type=int, default=40,
                        help="Number of long detectors (must all be used)")
    parser.add_argument("--short-type", default="He3_ELIGANT",
                        help="Short detector type name")
    parser.add_argument("--long-type", default="He3_ELIGANT_Long",
                        help="Long detector type name")

    args = parser.parse_args()

    optimizer = ConstrainedDetectorOptimizer(
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

    study = optimizer.optimize(n_trials=args.n_trials)


if __name__ == "__main__":
    main()
