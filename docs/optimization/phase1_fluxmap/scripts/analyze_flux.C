// analyze_flux.C
// Thermal neutron flux map analysis
//
// Usage: root -l -b -q 'analyze_flux.C("output_run0_t*.root", "results", "1MeV", 10.0)'

#include <iostream>
#include <cmath>
#include "TChain.h"
#include "TFile.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TH3D.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TEllipse.h"
#include "TLine.h"
#include "TLegend.h"
#include "TSystem.h"
#include "TView.h"

void analyze_flux(const char* file_pattern = "output_run0_t*.root",
                  const char* output_dir = "results",
                  const char* energy_label = "test",
                  double voxel_size = 10.0)
{
    // Create output directory
    gSystem->mkdir(output_dir, kTRUE);

    // Moderator dimensions
    const double box_x = 460.0;  // mm
    const double box_y = 460.0;  // mm
    const double box_z = 640.0;  // mm (current geometry)

    // Calculate bin numbers
    int n_bins_x = (int)(box_x / voxel_size);
    int n_bins_y = (int)(box_y / voxel_size);
    int n_bins_z = (int)(box_z / voxel_size);
    double r_max = std::min(box_x, box_y) / 2.0;
    int n_bins_r = (int)(r_max / voxel_size);

    // Load data using TChain
    std::cout << "Loading data from pattern: " << file_pattern << std::endl;

    TChain* chain = new TChain("FluxMap");
    int n_files = chain->Add(file_pattern);

    if (n_files == 0) {
        std::cerr << "Error: No files found matching pattern: " << file_pattern << std::endl;
        return;
    }

    Long64_t n_entries = chain->GetEntries();
    std::cout << "Found " << n_files << " files with " << n_entries << " total entries" << std::endl;

    // Set branch addresses
    Double_t x_mm, y_mm, z_mm, energy_eV, step_length_mm;
    Int_t eventID;
    chain->SetBranchAddress("X_mm", &x_mm);
    chain->SetBranchAddress("Y_mm", &y_mm);
    chain->SetBranchAddress("Z_mm", &z_mm);
    chain->SetBranchAddress("Energy_eV", &energy_eV);
    chain->SetBranchAddress("StepLength_mm", &step_length_mm);
    chain->SetBranchAddress("EventID", &eventID);

    // Create histograms
    TH2D* h_xy = new TH2D("h_xy", "Thermal Neutron Flux (XY projection);X [mm];Y [mm]",
                          n_bins_x, -box_x/2, box_x/2,
                          n_bins_y, -box_y/2, box_y/2);

    TH2D* h_xz = new TH2D("h_xz", "Thermal Neutron Flux (XZ projection);Z [mm];X [mm]",
                          n_bins_z, -box_z/2, box_z/2,
                          n_bins_x, -box_x/2, box_x/2);

    TH1D* h_radial = new TH1D("h_radial", "Radial Thermal Neutron Flux Profile;Radius [mm];Flux [arb. units]",
                              n_bins_r, 0, r_max);

    // 3D histogram
    TH3D* h_3d = new TH3D("h_3d", "Thermal Neutron Flux (3D);X [mm];Y [mm];Z [mm]",
                          n_bins_x, -box_x/2, box_x/2,
                          n_bins_y, -box_y/2, box_y/2,
                          n_bins_z, -box_z/2, box_z/2);

    // Fill histograms
    std::cout << "Filling histograms..." << std::endl;

    for (Long64_t i = 0; i < n_entries; i++) {
        if (i % 100000 == 0) {
            std::cout << "  Processing entry " << i << "/" << n_entries
                      << " (" << 100.0*i/n_entries << "%)" << std::endl;
        }

        chain->GetEntry(i);

        // XY projection
        h_xy->Fill(x_mm, y_mm, step_length_mm);

        // XZ projection
        h_xz->Fill(z_mm, x_mm, step_length_mm);

        // Radial profile
        double r = std::sqrt(x_mm*x_mm + y_mm*y_mm);
        h_radial->Fill(r, step_length_mm);

        // 3D flux map
        h_3d->Fill(x_mm, y_mm, z_mm, step_length_mm);
    }

    std::cout << "Done filling histograms" << std::endl;

    // Normalize radial profile by ring area
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
    double optimal_radius = h_radial->GetBinCenter(max_bin);
    std::cout << "\n========================================" << std::endl;
    std::cout << "OPTIMAL RADIUS: " << optimal_radius << " mm" << std::endl;
    std::cout << "========================================" << std::endl;

    // Set style
    gStyle->SetOptStat(0);
    gStyle->SetPalette(kBird);

    // XY projection plot
    TCanvas* c_xy = new TCanvas("c_xy", "XY Flux Map", 800, 800);
    h_xy->Draw("COLZ");
    TEllipse* beam_pipe = new TEllipse(0, 0, 22, 22);
    beam_pipe->SetFillStyle(0);
    beam_pipe->SetLineColor(kCyan);
    beam_pipe->SetLineWidth(2);
    beam_pipe->Draw("same");
    c_xy->SaveAs(Form("%s/fluxmap_%s_xy.png", output_dir, energy_label));

    // XZ projection plot
    TCanvas* c_xz = new TCanvas("c_xz", "XZ Flux Map", 1200, 600);
    h_xz->Draw("COLZ");
    c_xz->SaveAs(Form("%s/fluxmap_%s_xz.png", output_dir, energy_label));

    // Radial profile plot
    TCanvas* c_radial = new TCanvas("c_radial", "Radial Profile", 1000, 600);
    h_radial->SetLineColor(kBlue);
    h_radial->SetLineWidth(2);
    h_radial->Draw("HIST");

    TLine* line_opt = new TLine(optimal_radius, 0, optimal_radius, h_radial->GetMaximum());
    line_opt->SetLineColor(kRed);
    line_opt->SetLineStyle(2);
    line_opt->SetLineWidth(2);
    line_opt->Draw("same");

    TLine* line_beam = new TLine(22, 0, 22, h_radial->GetMaximum());
    line_beam->SetLineColor(kCyan);
    line_beam->SetLineStyle(3);
    line_beam->SetLineWidth(2);
    line_beam->Draw("same");

    TLegend* legend = new TLegend(0.55, 0.7, 0.88, 0.88);
    legend->AddEntry(h_radial, "Flux profile", "l");
    legend->AddEntry(line_opt, Form("Optimal R = %.1f mm", optimal_radius), "l");
    legend->AddEntry(line_beam, "Beam pipe (R=22 mm)", "l");
    legend->Draw();

    c_radial->SaveAs(Form("%s/radial_profile_%s.png", output_dir, energy_label));

    // 3D flux map plot
    TCanvas* c_3d = new TCanvas("c_3d", "3D Flux Map", 1000, 800);
    c_3d->SetTheta(30);
    c_3d->SetPhi(45);
    h_3d->Draw("BOX2 Z");
    c_3d->SaveAs(Form("%s/fluxmap_%s_3d.png", output_dir, energy_label));

    // Save histograms to ROOT file
    TFile* f_out = new TFile(Form("%s/fluxmap_%s.root", output_dir, energy_label), "RECREATE");
    h_xy->Write();
    h_xz->Write();
    h_radial->Write();
    h_3d->Write();
    f_out->Close();

    std::cout << "\nSaved plots to " << output_dir << "/" << std::endl;
    std::cout << "  - fluxmap_" << energy_label << "_xy.png" << std::endl;
    std::cout << "  - fluxmap_" << energy_label << "_xz.png" << std::endl;
    std::cout << "  - fluxmap_" << energy_label << "_3d.png" << std::endl;
    std::cout << "  - radial_profile_" << energy_label << ".png" << std::endl;
    std::cout << "  - fluxmap_" << energy_label << ".root" << std::endl;

    // Cleanup
    delete chain;
    delete c_xy;
    delete c_xz;
    delete c_radial;
    delete c_3d;
    delete f_out;
}
