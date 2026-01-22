#!/usr/bin/env python3
"""
Bayesian Optimization for He-3 Detector Placement
Uses Optuna for Gaussian Process-based optimization
Supports multiple detector types with inventory constraints
"""

import optuna
from optuna.samplers import TPESampler
import json
import argparse
import sys
import os
from pathlib import Path
from datetime import datetime
import shutil

# Add scripts directory to path
sys.path.insert(0, str(Path(__file__).parent))

from geometry_generator import (
    generate_geometry_config,
    save_geometry_config,
    is_valid_configuration,
    total_detectors,
    summarize_config,
    DetectorInventory
)
from run_nbox import NBoxRunner


class DetectorOptimizer:
    def __init__(self, build_dir: str, detector_config: str,
                 n_events: int = 10000, source_file: str = None,
                 energy: float = 1.0, energy_unit: str = "MeV",
                 output_dir: str = "results", n_rings: int = 4,
                 n_short: int = 28, n_long: int = 40,
                 short_type: str = "He3_ELIGANT",
                 long_type: str = "He3_ELIGANT_Long"):
        """
        Initialize optimizer

        Parameters:
        -----------
        build_dir : str
            Path to NBox build directory
        detector_config : str
            Path to detector description JSON file
        n_events : int
            Number of events per simulation
        source_file : str, optional
            Path to source spectrum ROOT file
        energy : float
            Neutron energy (if source_file not provided)
        energy_unit : str
            Energy unit
        output_dir : str
            Output directory for results
        n_rings : int
            Number of detector rings (2, 3, or 4)
        n_short : int
            Maximum number of short detectors available
        n_long : int
            Maximum number of long detectors available
        short_type : str
            Short detector type name in detector config
        long_type : str
            Long detector type name in detector config
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

        # Configuration storage
        self.configs_dir = self.output_dir / "configs"
        self.configs_dir.mkdir(exist_ok=True)

        # Trial counter
        self.trial_count = 0

    def objective(self, trial: optuna.Trial) -> float:
        """
        Objective function for optimization

        Parameters:
        -----------
        trial : optuna.Trial
            Optuna trial object

        Returns:
        --------
        float
            Detection efficiency (to be maximized)
        """
        self.trial_count += 1

        # Physical constraints
        # Detector diameter: 25.4mm, min gap: 5mm, required spacing: 30.4mm
        # Beam pipe radius: 22mm, box half-width: 500mm (1m cube)
        # Min inner radius: 22 + 25.4/2 = 34.7mm (round to 35mm)
        # Max outer radius: 500 - 25.4/2 = 487.3mm (round to 487mm)
        min_spacing = 31.0  # mm (slightly more than 30.4mm for safety)
        max_radius = 487.0  # mm

        # Maximum total detectors available
        max_total = self.inventory.total_available()

        # Sample parameters based on number of rings
        # Use sequential sampling with minimum spacing to ensure valid configurations
        if self.n_rings == 2:
            r1 = trial.suggest_float('r1', 35, 200)
            r2 = trial.suggest_float('r2', r1 + min_spacing, max_radius)
            radii = [r1, r2]
            counts = [
                trial.suggest_int('n1', 4, 20),
                trial.suggest_int('n2', 8, 40),
            ]
            # Sample short detector counts per ring
            short_counts = [
                trial.suggest_int('s1', 0, min(counts[0], self.inventory.short_count)),
                trial.suggest_int('s2', 0, min(counts[1], self.inventory.short_count)),
            ]
        elif self.n_rings == 3:
            r1 = trial.suggest_float('r1', 35, 150)
            r2 = trial.suggest_float('r2', r1 + min_spacing, 300)
            r3 = trial.suggest_float('r3', r2 + min_spacing, max_radius)
            radii = [r1, r2, r3]
            counts = [
                trial.suggest_int('n1', 4, 16),
                trial.suggest_int('n2', 8, 28),
                trial.suggest_int('n3', 12, 40),
            ]
            # Sample short detector counts per ring
            short_counts = [
                trial.suggest_int('s1', 0, min(counts[0], self.inventory.short_count)),
                trial.suggest_int('s2', 0, min(counts[1], self.inventory.short_count)),
                trial.suggest_int('s3', 0, min(counts[2], self.inventory.short_count)),
            ]
        else:  # 4 rings
            r1 = trial.suggest_float('r1', 35, 120)
            r2 = trial.suggest_float('r2', r1 + min_spacing, 220)
            r3 = trial.suggest_float('r3', r2 + min_spacing, 350)
            r4 = trial.suggest_float('r4', r3 + min_spacing, max_radius)
            radii = [r1, r2, r3, r4]
            counts = [
                trial.suggest_int('n1', 4, 12),
                trial.suggest_int('n2', 8, 20),
                trial.suggest_int('n3', 12, 24),
                trial.suggest_int('n4', 16, 28),
            ]
            # Sample short detector counts per ring
            short_counts = [
                trial.suggest_int('s1', 0, min(counts[0], self.inventory.short_count)),
                trial.suggest_int('s2', 0, min(counts[1], self.inventory.short_count)),
                trial.suggest_int('s3', 0, min(counts[2], self.inventory.short_count)),
                trial.suggest_int('s4', 0, min(counts[3], self.inventory.short_count)),
            ]

        # Check validity (including inventory constraints)
        is_valid, error = is_valid_configuration(radii, counts, short_counts, self.inventory)
        if not is_valid:
            print(f"  Trial {trial.number}: Invalid - {error}")
            return 0.0

        # Generate geometry config
        try:
            config = generate_geometry_config(radii, counts, short_counts, self.inventory)
        except ValueError as e:
            print(f"  Trial {trial.number}: Config error - {e}")
            return 0.0

        # Save config (use absolute path for NBox)
        config_path = self.configs_dir / f"trial_{trial.number:04d}.json"
        config_path = config_path.resolve()
        save_geometry_config(config, str(config_path))

        # Summarize configuration
        summary = summarize_config(counts, short_counts)

        # Run simulation
        print(f"  Trial {trial.number}: r={[f'{r:.1f}' for r in radii]}, n={counts}, "
              f"short={short_counts}, total={summary['total']} ({summary['short']}S+{summary['long']}L)")

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
        trial.set_user_attr("counts", counts)
        trial.set_user_attr("short_counts", short_counts)
        trial.set_user_attr("total_detectors", summary['total'])
        trial.set_user_attr("total_short", summary['short'])
        trial.set_user_attr("total_long", summary['long'])
        trial.set_user_attr("n_hits", eff_result.get("n_hits", 0))

        return efficiency

    def optimize(self, n_trials: int = 100, n_startup_trials: int = 20) -> optuna.Study:
        """
        Run optimization

        Parameters:
        -----------
        n_trials : int
            Total number of trials
        n_startup_trials : int
            Number of random trials before TPE kicks in

        Returns:
        --------
        optuna.Study
            Completed study object
        """
        # Create study
        study = optuna.create_study(
            direction="maximize",
            sampler=TPESampler(n_startup_trials=n_startup_trials),
            study_name=f"detector_optimization_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        )

        print(f"Starting optimization with {n_trials} trials")
        print(f"  Events per trial: {self.n_events}")
        print(f"  Number of rings: {self.n_rings}")
        print(f"  Detector inventory: {self.inventory.short_count} short ({self.inventory.short_type}), "
              f"{self.inventory.long_count} long ({self.inventory.long_type})")
        if self.source_file:
            print(f"  Source: {self.source_file}")
        else:
            print(f"  Energy: {self.energy} {self.energy_unit}")
        print()

        # Run optimization
        study.optimize(self.objective, n_trials=n_trials, show_progress_bar=True)

        # Save results
        self._save_results(study)

        return study

    def _save_results(self, study: optuna.Study):
        """Save optimization results"""
        # Best parameters
        best_params = {
            "best_efficiency": study.best_value,
            "best_params": study.best_params,
            "best_trial": study.best_trial.number,
            "radii": study.best_trial.user_attrs.get("radii", []),
            "counts": study.best_trial.user_attrs.get("counts", []),
            "short_counts": study.best_trial.user_attrs.get("short_counts", []),
            "total_detectors": study.best_trial.user_attrs.get("total_detectors", 0),
            "total_short": study.best_trial.user_attrs.get("total_short", 0),
            "total_long": study.best_trial.user_attrs.get("total_long", 0),
            "n_events": self.n_events,
            "n_rings": self.n_rings,
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
            if trial.state == optuna.trial.TrialState.COMPLETE:
                history.append({
                    "trial": trial.number,
                    "efficiency": trial.value,
                    "params": trial.params,
                    "radii": trial.user_attrs.get("radii", []),
                    "counts": trial.user_attrs.get("counts", []),
                    "short_counts": trial.user_attrs.get("short_counts", []),
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
        print(f"  Best counts: {best_params['counts']}")
        print(f"  Best short counts: {best_params['short_counts']}")
        print(f"  Total detectors: {best_params['total_detectors']} "
              f"({best_params['total_short']} short + {best_params['total_long']} long)")
        print(f"  Results saved to: {self.output_dir}")


def main():
    parser = argparse.ArgumentParser(description="Bayesian optimization for detector placement")
    parser.add_argument("--build-dir", required=True, help="NBox build directory")
    parser.add_argument("--detector-config", required=True, help="Detector description JSON")
    parser.add_argument("--n-trials", type=int, default=50, help="Number of optimization trials")
    parser.add_argument("--n-events", type=int, default=10000, help="Events per simulation")
    parser.add_argument("--source-file", help="Source spectrum ROOT file")
    parser.add_argument("--energy", type=float, default=1.0, help="Neutron energy")
    parser.add_argument("--energy-unit", default="MeV", help="Energy unit")
    parser.add_argument("--n-rings", type=int, default=4, choices=[2, 3, 4], help="Number of rings")
    parser.add_argument("--output", default="results", help="Output directory")

    # Detector inventory options
    parser.add_argument("--n-short", type=int, default=28,
                        help="Number of short detectors available (default: 28)")
    parser.add_argument("--n-long", type=int, default=40,
                        help="Number of long detectors available (default: 40)")
    parser.add_argument("--short-type", default="He3_ELIGANT",
                        help="Short detector type name (default: He3_ELIGANT)")
    parser.add_argument("--long-type", default="He3_ELIGANT_Long",
                        help="Long detector type name (default: He3_ELIGANT_Long)")

    args = parser.parse_args()

    optimizer = DetectorOptimizer(
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
