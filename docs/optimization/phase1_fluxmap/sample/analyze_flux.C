// analyze_flux.C
// Thermal neutron flux map analysis for 1m cubic moderator box
//
// Usage: root -l -b -q 'analyze_flux.C("output_run0_t*.root", "results", "1MeV")'

#include <iostream>
#include <cmath>
#include "TChain.h"
#include "TFile.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TEllipse.h"
#include "TLine.h"
#include "TLegend.h"
#include "TSystem.h"

void analyze_flux(const char* file_pattern = "output_run0_t*.root",
                  const char* output_dir = "results",
                  const char* energy_label = "1MeV")
{
    // Create output directory
    gSystem->mkdir(output_dir, kTRUE);

    // Moderator box: 1m cube
    const double box_half = 500.0;  // mm (half of 1000mm)
    const double voxel_size = 10.0; // mm

    int n_bins = (int)(box_half * 2 / voxel_size);
    double r_max = box_half;
    int n_bins_r = (int)(r_max / voxel_size);

    // Load data
    std::cout << "Loading data from: " << file_pattern << std::endl;
    TChain* chain = new TChain("FluxMap");
    int n_files = chain->Add(file_pattern);

    if (n_files == 0) {
        std::cerr << "Error: No files found!" << std::endl;
        return;
    }

    Long64_t n_entries = chain->GetEntries();
    std::cout << "Found " << n_files << " files, " << n_entries << " entries" << std::endl;

    // Branch addresses
    Double_t x_mm, y_mm, z_mm, step_length_mm;
    chain->SetBranchAddress("X_mm", &x_mm);
    chain->SetBranchAddress("Y_mm", &y_mm);
    chain->SetBranchAddress("Z_mm", &z_mm);
    chain->SetBranchAddress("StepLength_mm", &step_length_mm);

    // Histograms
    TH2D* h_xy = new TH2D("h_xy", "Thermal Neutron Flux (XY);X [mm];Y [mm]",
                          n_bins, -box_half, box_half,
                          n_bins, -box_half, box_half);

    TH1D* h_radial = new TH1D("h_radial", "Radial Flux Profile;Radius [mm];Flux [arb.]",
                              n_bins_r, 0, r_max);

    // Fill histograms
    std::cout << "Filling histograms..." << std::endl;
    for (Long64_t i = 0; i < n_entries; i++) {
        if (i % 100000 == 0) {
            std::cout << "  " << i << "/" << n_entries << std::endl;
        }
        chain->GetEntry(i);
        h_xy->Fill(x_mm, y_mm, step_length_mm);
        double r = std::sqrt(x_mm*x_mm + y_mm*y_mm);
        h_radial->Fill(r, step_length_mm);
    }

    // Normalize radial by ring area
    for (int i = 1; i <= h_radial->GetNbinsX(); i++) {
        double r_low = h_radial->GetBinLowEdge(i);
        double r_high = r_low + h_radial->GetBinWidth(i);
        double area = M_PI * (r_high*r_high - r_low*r_low);
        if (area > 0) {
            h_radial->SetBinContent(i, h_radial->GetBinContent(i) / area);
        }
    }

    // Find optimal radius
    int max_bin = h_radial->GetMaximumBin();
    double optimal_r = h_radial->GetBinCenter(max_bin);
    std::cout << "\n=== OPTIMAL RADIUS: " << optimal_r << " mm ===" << std::endl;

    // Plot
    gStyle->SetOptStat(0);
    gStyle->SetPalette(kBird);

    // XY map
    TCanvas* c1 = new TCanvas("c1", "XY", 800, 800);
    h_xy->Draw("COLZ");
    TEllipse* beam = new TEllipse(0, 0, 22, 22);
    beam->SetFillStyle(0);
    beam->SetLineColor(kCyan);
    beam->SetLineWidth(2);
    beam->Draw("same");
    c1->SaveAs(Form("%s/fluxmap_%s_xy.png", output_dir, energy_label));

    // Radial profile
    TCanvas* c2 = new TCanvas("c2", "Radial", 1000, 600);
    h_radial->SetLineColor(kBlue);
    h_radial->SetLineWidth(2);
    h_radial->Draw("HIST");

    TLine* l1 = new TLine(optimal_r, 0, optimal_r, h_radial->GetMaximum());
    l1->SetLineColor(kRed);
    l1->SetLineStyle(2);
    l1->SetLineWidth(2);
    l1->Draw("same");

    TLine* l2 = new TLine(22, 0, 22, h_radial->GetMaximum());
    l2->SetLineColor(kCyan);
    l2->SetLineStyle(3);
    l2->Draw("same");

    TLegend* leg = new TLegend(0.55, 0.7, 0.88, 0.88);
    leg->AddEntry(h_radial, "Flux profile", "l");
    leg->AddEntry(l1, Form("Optimal R=%.1f mm", optimal_r), "l");
    leg->AddEntry(l2, "Beam pipe R=22 mm", "l");
    leg->Draw();
    c2->SaveAs(Form("%s/radial_%s.png", output_dir, energy_label));

    // Save ROOT file
    TFile* fout = new TFile(Form("%s/fluxmap_%s.root", output_dir, energy_label), "RECREATE");
    h_xy->Write();
    h_radial->Write();
    fout->Close();

    std::cout << "\nSaved to " << output_dir << "/" << std::endl;
    delete chain;
}
