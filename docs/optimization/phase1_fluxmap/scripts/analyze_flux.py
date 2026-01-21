#!/usr/bin/env python3
"""
analyze_flux.py - Thermal neutron flux map analysis

Analyzes FluxMap Ntuple data from NBox simulation output.
Generates 3D flux maps and 2D projections.

Usage:
    python analyze_flux.py <root_file> [-o <output_dir>] [-v <voxel_size>]

Example:
    python analyze_flux.py simulations/1MeV.root -o results/ -v 10
"""

import argparse
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path

try:
    import uproot
except ImportError:
    print("Error: uproot not installed. Run: pip install uproot awkward")
    exit(1)


class FluxMapAnalyzer:
    """Analyzes thermal neutron flux distribution in moderator"""

    def __init__(self, root_file: str, voxel_size: float = 10.0):
        """
        Parameters
        ----------
        root_file : str
            Path to ROOT file with FluxMap Ntuple
        voxel_size : float
            Voxel size in mm for binning (default: 10 mm)
        """
        self.root_file = Path(root_file)
        self.voxel_size = voxel_size
        self.data = None

        # Moderator dimensions (from specification)
        self.box_x = 460  # mm
        self.box_y = 460  # mm
        self.box_z = 1100  # mm

    def load_data(self):
        """Load FluxMap data from ROOT file"""
        print(f"Loading data from {self.root_file}...")

        with uproot.open(self.root_file) as f:
            # Check available trees
            print(f"Available keys: {f.keys()}")

            # Try to find FluxMap tree
            tree_name = None
            for key in f.keys():
                if "FluxMap" in key:
                    tree_name = key
                    break

            if tree_name is None:
                raise ValueError("FluxMap tree not found in ROOT file. "
                                 "Make sure to run simulation with -f flag.")

            tree = f[tree_name]
            print(f"Found tree: {tree_name}")
            print(f"Entries: {tree.num_entries}")

            # Load arrays
            self.data = {
                'x': tree['X_mm'].array(library='np'),
                'y': tree['Y_mm'].array(library='np'),
                'z': tree['Z_mm'].array(library='np'),
                'energy': tree['Energy_eV'].array(library='np'),
                'step_length': tree['StepLength_mm'].array(library='np'),
            }

        print(f"Loaded {len(self.data['x'])} thermal neutron steps")
        return self

    def create_histogram_3d(self):
        """Create 3D flux histogram"""
        # Calculate bin edges
        n_bins_x = int(self.box_x / self.voxel_size)
        n_bins_y = int(self.box_y / self.voxel_size)
        n_bins_z = int(self.box_z / self.voxel_size)

        x_edges = np.linspace(-self.box_x / 2, self.box_x / 2, n_bins_x + 1)
        y_edges = np.linspace(-self.box_y / 2, self.box_y / 2, n_bins_y + 1)
        z_edges = np.linspace(-self.box_z / 2, self.box_z / 2, n_bins_z + 1)

        # Create 3D histogram weighted by step length (track length estimator)
        hist, edges = np.histogramdd(
            (self.data['x'], self.data['y'], self.data['z']),
            bins=(x_edges, y_edges, z_edges),
            weights=self.data['step_length']
        )

        self.hist_3d = hist
        self.edges = (x_edges, y_edges, z_edges)
        return hist, edges

    def get_xy_projection(self, z_slice: float = 0.0):
        """Get XY slice at given Z position"""
        z_edges = self.edges[2]
        z_idx = np.searchsorted(z_edges, z_slice) - 1
        z_idx = max(0, min(z_idx, self.hist_3d.shape[2] - 1))
        return self.hist_3d[:, :, z_idx]

    def get_xz_projection(self, y_slice: float = 0.0):
        """Get XZ slice at given Y position"""
        y_edges = self.edges[1]
        y_idx = np.searchsorted(y_edges, y_slice) - 1
        y_idx = max(0, min(y_idx, self.hist_3d.shape[1] - 1))
        return self.hist_3d[:, y_idx, :]

    def get_radial_profile(self):
        """Calculate radial flux profile (averaged over Z)"""
        x_centers = (self.edges[0][:-1] + self.edges[0][1:]) / 2
        y_centers = (self.edges[1][:-1] + self.edges[1][1:]) / 2

        # Sum over Z
        xy_sum = np.sum(self.hist_3d, axis=2)

        # Calculate radial profile
        r_max = min(self.box_x, self.box_y) / 2
        r_bins = np.linspace(0, r_max, int(r_max / self.voxel_size) + 1)
        r_profile = np.zeros(len(r_bins) - 1)
        r_counts = np.zeros(len(r_bins) - 1)

        for i, x in enumerate(x_centers):
            for j, y in enumerate(y_centers):
                r = np.sqrt(x**2 + y**2)
                r_idx = np.searchsorted(r_bins, r) - 1
                if 0 <= r_idx < len(r_profile):
                    r_profile[r_idx] += xy_sum[i, j]
                    r_counts[r_idx] += 1

        # Normalize by bin count
        mask = r_counts > 0
        r_profile[mask] /= r_counts[mask]

        r_centers = (r_bins[:-1] + r_bins[1:]) / 2
        return r_centers, r_profile

    def find_optimal_radius(self):
        """Find radius with maximum flux"""
        r_centers, r_profile = self.get_radial_profile()
        max_idx = np.argmax(r_profile)
        return r_centers[max_idx], r_profile[max_idx]

    def plot_xy_slice(self, z_slice: float = 0.0, output_path: str = None):
        """Plot XY slice at given Z"""
        xy = self.get_xy_projection(z_slice)

        fig, ax = plt.subplots(figsize=(8, 8))
        x_edges = self.edges[0]
        y_edges = self.edges[1]

        im = ax.pcolormesh(x_edges, y_edges, xy.T, cmap='hot')
        ax.set_xlabel('X [mm]')
        ax.set_ylabel('Y [mm]')
        ax.set_title(f'Thermal Neutron Flux (Z = {z_slice:.0f} mm)')
        ax.set_aspect('equal')
        plt.colorbar(im, ax=ax, label='Flux [arb. units]')

        # Draw beam pipe
        circle = plt.Circle((0, 0), 22, fill=False, color='cyan', linewidth=2)
        ax.add_patch(circle)

        if output_path:
            plt.savefig(output_path, dpi=150, bbox_inches='tight')
            print(f"Saved: {output_path}")
        plt.close()

    def plot_xz_slice(self, y_slice: float = 0.0, output_path: str = None):
        """Plot XZ slice at given Y"""
        xz = self.get_xz_projection(y_slice)

        fig, ax = plt.subplots(figsize=(12, 6))
        x_edges = self.edges[0]
        z_edges = self.edges[2]

        im = ax.pcolormesh(z_edges, x_edges, xz, cmap='hot')
        ax.set_xlabel('Z [mm]')
        ax.set_ylabel('X [mm]')
        ax.set_title(f'Thermal Neutron Flux (Y = {y_slice:.0f} mm)')
        plt.colorbar(im, ax=ax, label='Flux [arb. units]')

        if output_path:
            plt.savefig(output_path, dpi=150, bbox_inches='tight')
            print(f"Saved: {output_path}")
        plt.close()

    def plot_radial_profile(self, output_path: str = None):
        """Plot radial flux profile"""
        r_centers, r_profile = self.get_radial_profile()
        r_opt, flux_opt = self.find_optimal_radius()

        fig, ax = plt.subplots(figsize=(10, 6))
        ax.plot(r_centers, r_profile, 'b-', linewidth=2)
        ax.axvline(r_opt, color='r', linestyle='--',
                   label=f'Optimal R = {r_opt:.1f} mm')
        ax.axvline(22, color='cyan', linestyle=':',
                   label='Beam pipe (R=22 mm)')

        ax.set_xlabel('Radius [mm]')
        ax.set_ylabel('Flux [arb. units]')
        ax.set_title('Radial Thermal Neutron Flux Profile')
        ax.legend()
        ax.grid(True, alpha=0.3)

        if output_path:
            plt.savefig(output_path, dpi=150, bbox_inches='tight')
            print(f"Saved: {output_path}")
        plt.close()

        return r_opt

    def generate_all_plots(self, output_dir: str):
        """Generate all analysis plots"""
        output_dir = Path(output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)

        # Get energy label from filename
        energy_label = self.root_file.stem

        # XY slice at Z=0
        self.plot_xy_slice(z_slice=0, output_path=output_dir / f'fluxmap_{energy_label}_xy.png')

        # XZ slice at Y=0
        self.plot_xz_slice(y_slice=0, output_path=output_dir / f'fluxmap_{energy_label}_xz.png')

        # Radial profile
        r_opt = self.plot_radial_profile(output_path=output_dir / f'radial_profile_{energy_label}.png')

        print(f"\nOptimal radius for {energy_label}: {r_opt:.1f} mm")

        return r_opt


def main():
    parser = argparse.ArgumentParser(description='Analyze thermal neutron flux map')
    parser.add_argument('root_file', help='Input ROOT file')
    parser.add_argument('-o', '--output', default='results',
                        help='Output directory (default: results)')
    parser.add_argument('-v', '--voxel-size', type=float, default=10.0,
                        help='Voxel size in mm (default: 10)')

    args = parser.parse_args()

    analyzer = FluxMapAnalyzer(args.root_file, voxel_size=args.voxel_size)
    analyzer.load_data()
    analyzer.create_histogram_3d()
    analyzer.generate_all_plots(args.output)


if __name__ == '__main__':
    main()
