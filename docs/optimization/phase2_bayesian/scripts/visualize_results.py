#!/usr/bin/env python3
"""
Visualization for Bayesian Optimization Results
"""

import json
import argparse
import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path


def load_history(history_file: str) -> list:
    """Load optimization history from JSON file"""
    with open(history_file) as f:
        return json.load(f)


def plot_convergence(history: list, output_path: str):
    """Plot optimization convergence"""
    trials = [h["trial"] for h in history]
    efficiencies = [h["efficiency"] for h in history]

    # Calculate running best
    running_best = []
    best = 0
    for eff in efficiencies:
        best = max(best, eff)
        running_best.append(best)

    fig, ax = plt.subplots(figsize=(10, 6))

    ax.scatter(trials, efficiencies, alpha=0.5, label="Trial efficiency")
    ax.plot(trials, running_best, 'r-', linewidth=2, label="Best so far")

    ax.set_xlabel("Trial")
    ax.set_ylabel("Efficiency [%]")
    ax.set_title("Optimization Convergence")
    ax.legend()
    ax.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig(output_path, dpi=150)
    plt.close()
    print(f"Saved: {output_path}")


def plot_parameter_importance(history: list, output_path: str):
    """Plot parameter values vs efficiency"""
    # Extract parameters
    params = {}
    efficiencies = [h["efficiency"] for h in history]

    for h in history:
        for key, value in h["params"].items():
            if key not in params:
                params[key] = []
            params[key].append(value)

    n_params = len(params)
    fig, axes = plt.subplots(2, (n_params + 1) // 2, figsize=(12, 8))
    axes = axes.flatten()

    for i, (param_name, values) in enumerate(params.items()):
        if i >= len(axes):
            break
        ax = axes[i]
        ax.scatter(values, efficiencies, alpha=0.5)
        ax.set_xlabel(param_name)
        ax.set_ylabel("Efficiency [%]")
        ax.grid(True, alpha=0.3)

    # Hide unused axes
    for i in range(len(params), len(axes)):
        axes[i].set_visible(False)

    plt.suptitle("Parameter vs Efficiency")
    plt.tight_layout()
    plt.savefig(output_path, dpi=150)
    plt.close()
    print(f"Saved: {output_path}")


def plot_detector_distribution(history: list, output_path: str):
    """Plot detector count distribution for good configurations"""
    # Filter top 20% configurations
    sorted_history = sorted(history, key=lambda x: x["efficiency"], reverse=True)
    top_n = max(1, len(sorted_history) // 5)
    top_configs = sorted_history[:top_n]

    total_detectors = [h["total_detectors"] for h in top_configs]
    efficiencies = [h["efficiency"] for h in top_configs]

    fig, ax = plt.subplots(figsize=(8, 6))

    scatter = ax.scatter(total_detectors, efficiencies, c=range(len(top_configs)),
                        cmap='viridis', alpha=0.7)
    plt.colorbar(scatter, label="Rank (0=best)")

    ax.set_xlabel("Total Detectors")
    ax.set_ylabel("Efficiency [%]")
    ax.set_title(f"Top {top_n} Configurations: Detectors vs Efficiency")
    ax.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig(output_path, dpi=150)
    plt.close()
    print(f"Saved: {output_path}")


def plot_ring_configuration(best_params_file: str, output_path: str):
    """Visualize best detector ring configuration"""
    with open(best_params_file) as f:
        best = json.load(f)

    radii = best["radii"]
    counts = best["counts"]

    fig, ax = plt.subplots(figsize=(10, 10))

    # Draw moderator boundary
    theta = np.linspace(0, 2*np.pi, 100)
    box_half = 230
    ax.plot(box_half * np.cos(theta), box_half * np.sin(theta), 'k--', label="Box boundary")

    # Draw beam pipe
    beam_pipe_r = 22
    ax.fill(beam_pipe_r * np.cos(theta), beam_pipe_r * np.sin(theta),
            color='gray', alpha=0.5, label="Beam pipe")

    # Draw detector rings
    colors = plt.cm.Set1(np.linspace(0, 1, len(radii)))
    detector_r = 25.4 / 2  # Detector radius

    for i, (r, n, color) in enumerate(zip(radii, counts, colors)):
        # Draw ring circle
        ax.plot(r * np.cos(theta), r * np.sin(theta), '--', color=color, alpha=0.3)

        # Draw detectors
        angles = np.linspace(0, 2*np.pi, n, endpoint=False)
        for angle in angles:
            x = r * np.cos(angle)
            y = r * np.sin(angle)
            circle = plt.Circle((x, y), detector_r, color=color, alpha=0.7)
            ax.add_patch(circle)

        # Label
        ax.plot([], [], 'o', color=color, label=f"Ring {i+1}: r={r:.1f}mm, n={n}")

    ax.set_xlim(-250, 250)
    ax.set_ylim(-250, 250)
    ax.set_aspect('equal')
    ax.set_xlabel("X [mm]")
    ax.set_ylabel("Y [mm]")
    ax.set_title(f"Best Configuration: {sum(counts)} detectors, {best['best_efficiency']:.2f}% efficiency")
    ax.legend(loc='upper right')
    ax.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig(output_path, dpi=150)
    plt.close()
    print(f"Saved: {output_path}")


def main():
    parser = argparse.ArgumentParser(description="Visualize optimization results")
    parser.add_argument("--results-dir", required=True, help="Results directory")
    parser.add_argument("--output-dir", help="Output directory for plots (default: same as results)")

    args = parser.parse_args()

    results_dir = Path(args.results_dir)
    output_dir = Path(args.output_dir) if args.output_dir else results_dir

    history_file = results_dir / "optimization_history.json"
    best_params_file = results_dir / "best_parameters.json"

    if not history_file.exists():
        print(f"Error: {history_file} not found")
        return

    history = load_history(history_file)
    print(f"Loaded {len(history)} trials")

    # Generate plots
    plot_convergence(history, str(output_dir / "convergence.png"))
    plot_parameter_importance(history, str(output_dir / "parameter_importance.png"))
    plot_detector_distribution(history, str(output_dir / "detector_distribution.png"))

    if best_params_file.exists():
        plot_ring_configuration(str(best_params_file), str(output_dir / "best_configuration.png"))

    print("\nVisualization complete!")


if __name__ == "__main__":
    main()
