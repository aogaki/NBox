#!/usr/bin/env python3
"""
analyze_flux_root.py - Thermal neutron flux map analysis using PyROOT

Analyzes FluxMap Ntuple data from NBox simulation output.
Generates 3D flux maps and 2D projections.

Usage:
    python analyze_flux_root.py <root_files_pattern> [-o <output_dir>] [-v <voxel_size>]

Example:
    python analyze_flux_root.py "output_run0_t*.root" -o results/ -v 10
"""

import argparse
import ROOT
import numpy as np
from pathlib import Path
import os


class FluxMapAnalyzer:
    """Analyzes thermal neutron flux distribution in moderator using PyROOT"""

    def __init__(self, file_pattern: str, voxel_size: float = 10.0):
        """
        Parameters
        ----------
        file_pattern : str
            Glob pattern for ROOT files (e.g., "output_run0_t*.root")
        voxel_size : float
            Voxel size in mm for binning (default: 10 mm)
        """
        self.file_pattern = file_pattern
        self.voxel_size = voxel_size
        self.chain = None

        # Moderator dimensions (from specification)
        self.box_x = 460  # mm
        self.box_y = 460  # mm
        self.box_z = 640  # mm (current geometry, will be 1100 for optimization)

    def load_data(self):
        """Load FluxMap data from ROOT files using TChain"""
        print(f"Loading data from pattern: {self.file_pattern}")

        self.chain = ROOT.TChain("FluxMap")
        n_files = self.chain.Add(self.file_pattern)

        if n_files == 0:
            raise ValueError(f"No files found matching pattern: {self.file_pattern}")

        n_entries = self.chain.GetEntries()
        print(f"Found {n_files} files with {n_entries} total entries")

        return self

    def create_histograms(self, output_dir: str, energy_label: str = "test"):
        """Create flux map histograms"""
        output_dir = Path(output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)

        # Calculate bin numbers
        n_bins_x = int(self.box_x / self.voxel_size)
        n_bins_y = int(self.box_y / self.voxel_size)
        n_bins_z = int(self.box_z / self.voxel_size)

        # XY projection (Z=0 slice, but we'll integrate over small Z range)
        h_xy = ROOT.TH2D("h_xy", "Thermal Neutron Flux (XY projection);X [mm];Y [mm]",
                         n_bins_x, -self.box_x/2, self.box_x/2,
                         n_bins_y, -self.box_y/2, self.box_y/2)

        # XZ projection (Y=0 slice)
        h_xz = ROOT.TH2D("h_xz", "Thermal Neutron Flux (XZ projection);Z [mm];X [mm]",
                         n_bins_z, -self.box_z/2, self.box_z/2,
                         n_bins_x, -self.box_x/2, self.box_x/2)

        # Radial profile
        r_max = min(self.box_x, self.box_y) / 2
        n_bins_r = int(r_max / self.voxel_size)
        h_radial = ROOT.TH1D("h_radial", "Radial Thermal Neutron Flux Profile;Radius [mm];Flux [arb. units]",
                             n_bins_r, 0, r_max)

        # Fill histograms
        print("Filling histograms...")
        n_entries = self.chain.GetEntries()

        for i, entry in enumerate(self.chain):
            if i % 100000 == 0:
                print(f"  Processing entry {i}/{n_entries} ({100*i/n_entries:.1f}%)")

            x = entry.X_mm
            y = entry.Y_mm
            z = entry.Z_mm
            step_length = entry.StepLength_mm

            # XY projection (integrate over all Z)
            h_xy.Fill(x, y, step_length)

            # XZ projection (integrate over all Y)
            h_xz.Fill(z, x, step_length)

            # Radial profile
            r = np.sqrt(x*x + y*y)
            h_radial.Fill(r, step_length)

        print("Done filling histograms")

        # Normalize radial profile by ring area
        for i in range(1, h_radial.GetNbinsX() + 1):
            r_low = h_radial.GetBinLowEdge(i)
            r_high = r_low + h_radial.GetBinWidth(i)
            area = np.pi * (r_high**2 - r_low**2)
            if area > 0:
                h_radial.SetBinContent(i, h_radial.GetBinContent(i) / area)

        # Find optimal radius
        max_bin = h_radial.GetMaximumBin()
        optimal_radius = h_radial.GetBinCenter(max_bin)
        print(f"\nOptimal radius: {optimal_radius:.1f} mm")

        # Draw and save plots
        ROOT.gStyle.SetOptStat(0)
        ROOT.gStyle.SetPalette(ROOT.kBird)

        # XY projection
        c_xy = ROOT.TCanvas("c_xy", "XY Flux Map", 800, 800)
        h_xy.Draw("COLZ")
        # Draw beam pipe circle
        beam_pipe = ROOT.TEllipse(0, 0, 22, 22)
        beam_pipe.SetFillStyle(0)
        beam_pipe.SetLineColor(ROOT.kCyan)
        beam_pipe.SetLineWidth(2)
        beam_pipe.Draw("same")
        c_xy.SaveAs(str(output_dir / f"fluxmap_{energy_label}_xy.png"))

        # XZ projection
        c_xz = ROOT.TCanvas("c_xz", "XZ Flux Map", 1200, 600)
        h_xz.Draw("COLZ")
        c_xz.SaveAs(str(output_dir / f"fluxmap_{energy_label}_xz.png"))

        # Radial profile
        c_radial = ROOT.TCanvas("c_radial", "Radial Profile", 1000, 600)
        h_radial.SetLineColor(ROOT.kBlue)
        h_radial.SetLineWidth(2)
        h_radial.Draw("HIST")

        # Add optimal radius line
        line_opt = ROOT.TLine(optimal_radius, 0, optimal_radius, h_radial.GetMaximum())
        line_opt.SetLineColor(ROOT.kRed)
        line_opt.SetLineStyle(2)
        line_opt.SetLineWidth(2)
        line_opt.Draw("same")

        # Add beam pipe line
        line_beam = ROOT.TLine(22, 0, 22, h_radial.GetMaximum())
        line_beam.SetLineColor(ROOT.kCyan)
        line_beam.SetLineStyle(3)
        line_beam.SetLineWidth(2)
        line_beam.Draw("same")

        # Legend
        legend = ROOT.TLegend(0.6, 0.7, 0.88, 0.88)
        legend.AddEntry(h_radial, "Flux profile", "l")
        legend.AddEntry(line_opt, f"Optimal R = {optimal_radius:.1f} mm", "l")
        legend.AddEntry(line_beam, "Beam pipe (R=22 mm)", "l")
        legend.Draw()

        c_radial.SaveAs(str(output_dir / f"radial_profile_{energy_label}.png"))

        # Save histograms to ROOT file
        f_out = ROOT.TFile(str(output_dir / f"fluxmap_{energy_label}.root"), "RECREATE")
        h_xy.Write()
        h_xz.Write()
        h_radial.Write()
        f_out.Close()
        print(f"Saved histograms to {output_dir / f'fluxmap_{energy_label}.root'}")

        return optimal_radius


def main():
    parser = argparse.ArgumentParser(description='Analyze thermal neutron flux map')
    parser.add_argument('file_pattern', help='Input ROOT file pattern (e.g., "output_run0_t*.root")')
    parser.add_argument('-o', '--output', default='results',
                        help='Output directory (default: results)')
    parser.add_argument('-v', '--voxel-size', type=float, default=10.0,
                        help='Voxel size in mm (default: 10)')
    parser.add_argument('-l', '--label', default='test',
                        help='Energy label for output files (default: test)')

    args = parser.parse_args()

    # Suppress ROOT info messages
    ROOT.gROOT.SetBatch(True)
    ROOT.gErrorIgnoreLevel = ROOT.kWarning

    analyzer = FluxMapAnalyzer(args.file_pattern, voxel_size=args.voxel_size)
    analyzer.load_data()
    analyzer.create_histograms(args.output, args.label)


if __name__ == '__main__':
    main()
