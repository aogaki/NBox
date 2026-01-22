#!/usr/bin/env python3
"""
compare_energies.py - Compare flux profiles across different energies

Usage:
    python compare_energies.py <root_files...> -o <output_dir>

Example:
    python compare_energies.py simulations/*.root -o results/
"""

import argparse
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path
from analyze_flux import FluxMapAnalyzer


def compare_radial_profiles(root_files: list, output_dir: str, voxel_size: float = 10.0):
    """Compare radial flux profiles from multiple energy simulations"""
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    fig, ax = plt.subplots(figsize=(12, 8))

    results = []
    colors = plt.cm.viridis(np.linspace(0, 1, len(root_files)))

    for root_file, color in zip(root_files, colors):
        root_path = Path(root_file)
        energy_label = root_path.stem

        try:
            analyzer = FluxMapAnalyzer(root_file, voxel_size=voxel_size)
            analyzer.load_data()
            analyzer.create_histogram_3d()

            r_centers, r_profile = analyzer.get_radial_profile()
            r_opt, flux_opt = analyzer.find_optimal_radius()

            # Normalize profile for comparison
            r_profile_norm = r_profile / np.max(r_profile) if np.max(r_profile) > 0 else r_profile

            ax.plot(r_centers, r_profile_norm, color=color, linewidth=2, label=energy_label)

            results.append({
                'energy': energy_label,
                'optimal_radius': r_opt,
                'max_flux': flux_opt
            })
        except Exception as e:
            print(f"Error processing {root_file}: {e}")

    # Add beam pipe marker
    ax.axvline(22, color='cyan', linestyle=':', linewidth=2, label='Beam pipe (R=22 mm)')

    ax.set_xlabel('Radius [mm]', fontsize=12)
    ax.set_ylabel('Normalized Flux', fontsize=12)
    ax.set_title('Radial Thermal Neutron Flux Profile Comparison', fontsize=14)
    ax.legend(loc='best')
    ax.grid(True, alpha=0.3)

    output_path = output_dir / 'radial_profile_comparison.png'
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    print(f"Saved: {output_path}")
    plt.close()

    # Save optimal radii to CSV
    csv_path = output_dir / 'optimal_radii.csv'
    with open(csv_path, 'w') as f:
        f.write('Energy,OptimalRadius_mm,MaxFlux\n')
        for r in results:
            f.write(f"{r['energy']},{r['optimal_radius']:.1f},{r['max_flux']:.6e}\n")
    print(f"Saved: {csv_path}")

    # Print summary
    print("\n" + "=" * 50)
    print("OPTIMAL RADIUS SUMMARY")
    print("=" * 50)
    for r in results:
        print(f"  {r['energy']:>10s}: R = {r['optimal_radius']:.1f} mm")
    print("=" * 50)

    return results


def main():
    parser = argparse.ArgumentParser(description='Compare flux profiles across energies')
    parser.add_argument('root_files', nargs='+', help='Input ROOT files')
    parser.add_argument('-o', '--output', default='results',
                        help='Output directory (default: results)')
    parser.add_argument('-v', '--voxel-size', type=float, default=10.0,
                        help='Voxel size in mm (default: 10)')

    args = parser.parse_args()

    compare_radial_profiles(args.root_files, args.output, args.voxel_size)


if __name__ == '__main__':
    main()
