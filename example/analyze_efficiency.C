// analyze_efficiency.C
// Comprehensive efficiency analysis for NBox simulation output
//
// Usage: root -l analyze_efficiency.C
//        Then call: analyze_efficiency("output_run0_t0.root", 100000)
//        where 100000 is the total number of neutrons emitted

#include <iostream>
#include <map>
#include <set>
#include <vector>
#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TPaveText.h"
#include "TGraph.h"
#include "TStyle.h"

void analyze_efficiency(const char* filename, Long64_t total_neutrons) {
    // Open ROOT file
    TFile* f = TFile::Open(filename);
    if (!f || f->IsZombie()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return;
    }

    // Get tree
    TTree* hits = (TTree*)f->Get("hits");
    if (!hits) {
        std::cerr << "Error: Cannot find 'hits' tree in file" << std::endl;
        f->Close();
        return;
    }

    // Set branch addresses
    Int_t eventID, detectorID;
    Double_t edep_keV, time_ns;
    std::string* detectorName = nullptr;

    hits->SetBranchAddress("EventID", &eventID);
    hits->SetBranchAddress("DetectorID", &detectorID);
    hits->SetBranchAddress("DetectorName", &detectorName);
    hits->SetBranchAddress("Edep_keV", &edep_keV);
    hits->SetBranchAddress("Time_ns", &time_ns);

    // Data structures for analysis
    std::set<Int_t> uniqueEvents;
    std::map<Int_t, Long64_t> hitsPerDetector;
    std::map<Int_t, Double_t> energyPerDetector;
    std::map<Int_t, std::set<Int_t>> eventsPerDetector;

    // Histograms
    TH1D* h_energy = new TH1D("h_energy", "Energy Deposition;Energy [keV];Counts", 200, 0, 1000);
    TH1D* h_energy_full = new TH1D("h_energy_full", "Energy (>600 keV);Energy [keV];Counts", 200, 0, 1000);
    TH1D* h_time = new TH1D("h_time", "Time of Flight;Time [ns];Counts", 100, 0, 500);

    // Process all hits
    Long64_t nEntries = hits->GetEntries();
    std::cout << "Processing " << nEntries << " hits..." << std::endl;

    for (Long64_t i = 0; i < nEntries; i++) {
        hits->GetEntry(i);

        // Track unique events
        uniqueEvents.insert(eventID);

        // Fill histograms
        h_energy->Fill(edep_keV);
        h_time->Fill(time_ns);

        // Apply energy threshold for "full energy" analysis
        if (edep_keV > 600) {
            h_energy_full->Fill(edep_keV);
        }

        // Statistics per detector
        hitsPerDetector[detectorID]++;
        energyPerDetector[detectorID] += edep_keV;
        eventsPerDetector[detectorID].insert(eventID);
    }

    // Calculate efficiencies
    Long64_t detected_events = uniqueEvents.size();
    double intrinsic_efficiency = (double)detected_events / total_neutrons * 100.0;

    // Count events with energy > 600 keV (full energy peak)
    Long64_t full_energy_events = 0;
    std::set<Int_t> fullEnergyEvents;
    for (Long64_t i = 0; i < nEntries; i++) {
        hits->GetEntry(i);
        if (edep_keV > 600) {
            fullEnergyEvents.insert(eventID);
        }
    }
    full_energy_events = fullEnergyEvents.size();
    double full_energy_efficiency = (double)full_energy_events / total_neutrons * 100.0;

    // Get number of detectors
    Int_t nDetectors = hitsPerDetector.size();

    // Print summary report
    std::cout << "\n========================================" << std::endl;
    std::cout << "EFFICIENCY ANALYSIS SUMMARY" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Input file: " << filename << std::endl;
    std::cout << "Total neutrons emitted: " << total_neutrons << std::endl;
    std::cout << "Total hits recorded: " << nEntries << std::endl;
    std::cout << "Unique events detected: " << detected_events << std::endl;
    std::cout << "Number of detectors: " << nDetectors << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "OVERALL EFFICIENCY:" << std::endl;
    std::cout << "  Intrinsic efficiency (any hit): " << intrinsic_efficiency << " %" << std::endl;
    std::cout << "  Full-energy efficiency (>600 keV): " << full_energy_efficiency << " %" << std::endl;
    std::cout << "  Average hits per detected event: " << (double)nEntries / detected_events << std::endl;
    std::cout << "========================================" << std::endl;

    // Per-detector efficiency
    std::cout << "EFFICIENCY PER DETECTOR:" << std::endl;
    std::cout << "DetID  |  Hits  | Events | Eff(%)  | Avg E (keV)" << std::endl;
    std::cout << "-------|--------|--------|---------|-------------" << std::endl;

    for (auto& p : hitsPerDetector) {
        Int_t detID = p.first;
        Long64_t hits_count = p.second;
        Long64_t events_count = eventsPerDetector[detID].size();
        double eff = (double)events_count / total_neutrons * 100.0;
        double avg_energy = energyPerDetector[detID] / hits_count;

        printf("%5d  | %6lld | %6lld | %6.3f%% | %10.2f\n",
               detID, hits_count, events_count, eff, avg_energy);
    }

    std::cout << "========================================" << std::endl;

    // Efficiency vs threshold analysis
    std::cout << "EFFICIENCY VS ENERGY THRESHOLD:" << std::endl;
    const int nThresholds = 20;
    double thresholds[nThresholds];
    double efficiencies[nThresholds];

    for (int i = 0; i < nThresholds; i++) {
        thresholds[i] = i * 50.0;  // 0 to 950 keV
        std::set<Int_t> eventsAboveThreshold;

        for (Long64_t j = 0; j < nEntries; j++) {
            hits->GetEntry(j);
            if (edep_keV > thresholds[i]) {
                eventsAboveThreshold.insert(eventID);
            }
        }

        efficiencies[i] = (double)eventsAboveThreshold.size() / total_neutrons * 100.0;
        printf("  E > %4.0f keV: %6.3f%%\n", thresholds[i], efficiencies[i]);
    }

    // Create summary plots
    gStyle->SetOptStat(111111);

    TCanvas* c1 = new TCanvas("c1", "Efficiency Analysis", 1400, 1000);
    c1->Divide(2, 2);

    // Plot 1: Energy spectrum
    c1->cd(1);
    h_energy->SetLineColor(kBlue);
    h_energy->SetLineWidth(2);
    h_energy->Draw("HIST");
    gPad->SetLogy();

    // Plot 2: Efficiency vs threshold
    c1->cd(2);
    TGraph* gr_eff = new TGraph(nThresholds, thresholds, efficiencies);
    gr_eff->SetTitle("Efficiency vs Energy Threshold");
    gr_eff->GetXaxis()->SetTitle("Energy Threshold [keV]");
    gr_eff->GetYaxis()->SetTitle("Efficiency [%]");
    gr_eff->SetLineColor(kRed);
    gr_eff->SetLineWidth(2);
    gr_eff->SetMarkerStyle(20);
    gr_eff->SetMarkerColor(kRed);
    gr_eff->Draw("ALP");

    // Plot 3: Hits per detector
    c1->cd(3);
    TH1D* h_hits_per_det = new TH1D("h_hits_per_det", "Hits per Detector;Detector ID;Number of Hits",
                                     nDetectors, -0.5, nDetectors - 0.5);
    for (auto& p : hitsPerDetector) {
        h_hits_per_det->Fill(p.first, p.second);
    }
    h_hits_per_det->SetFillColor(kGreen);
    h_hits_per_det->Draw("HIST");

    // Plot 4: Summary text
    c1->cd(4);
    TPaveText* pt = new TPaveText(0.1, 0.1, 0.9, 0.9, "NDC");
    pt->AddText("EFFICIENCY SUMMARY");
    pt->AddLine(0, 0.85, 1, 0.85);
    pt->AddText(" ");
    pt->AddText(Form("Total neutrons: %lld", total_neutrons));
    pt->AddText(Form("Detected events: %lld", detected_events));
    pt->AddText(Form("Total hits: %lld", nEntries));
    pt->AddText(" ");
    pt->AddText(Form("Intrinsic eff: %.3f%%", intrinsic_efficiency));
    pt->AddText(Form("Full-energy eff (>600 keV): %.3f%%", full_energy_efficiency));
    pt->AddText(" ");
    pt->AddText(Form("Number of detectors: %d", nDetectors));
    pt->AddText(Form("Avg hits/event: %.2f", (double)nEntries / detected_events));
    pt->SetTextAlign(12);
    pt->SetTextSize(0.04);
    pt->Draw();

    c1->SaveAs("efficiency_analysis.png");
    std::cout << "\nPlots saved to: efficiency_analysis.png" << std::endl;

    // Create detailed per-detector canvas
    TCanvas* c2 = new TCanvas("c2", "Per-Detector Analysis", 1400, 800);
    c2->Divide(2, 1);

    // Efficiency per detector
    c2->cd(1);
    TH1D* h_eff_per_det = new TH1D("h_eff_per_det", "Efficiency per Detector;Detector ID;Efficiency [%]",
                                    nDetectors, -0.5, nDetectors - 0.5);
    for (auto& p : eventsPerDetector) {
        Int_t detID = p.first;
        double eff = (double)p.second.size() / total_neutrons * 100.0;
        h_eff_per_det->Fill(detID, eff);
    }
    h_eff_per_det->SetFillColor(kOrange);
    h_eff_per_det->Draw("HIST");

    // Average energy per detector
    c2->cd(2);
    TH1D* h_avg_energy = new TH1D("h_avg_energy", "Average Energy per Detector;Detector ID;Average Energy [keV]",
                                   nDetectors, -0.5, nDetectors - 0.5);
    for (auto& p : hitsPerDetector) {
        Int_t detID = p.first;
        double avg_e = energyPerDetector[detID] / p.second;
        h_avg_energy->Fill(detID, avg_e);
    }
    h_avg_energy->SetFillColor(kCyan);
    h_avg_energy->Draw("HIST");

    c2->SaveAs("per_detector_analysis.png");
    std::cout << "Plots saved to: per_detector_analysis.png" << std::endl;

    // Cleanup
    delete h_energy;
    delete h_energy_full;
    delete h_time;
    delete h_hits_per_det;
    delete h_eff_per_det;
    delete h_avg_energy;
    delete gr_eff;
    delete pt;
    delete c1;
    delete c2;
    delete detectorName;
    f->Close();
    delete f;

    std::cout << "\nAnalysis complete!" << std::endl;
}

// Helper function to analyze multiple files (e.g., all thread files)
void analyze_efficiency_merged(const char* pattern, Long64_t total_neutrons) {
    std::cout << "Merging files matching pattern: " << pattern << std::endl;

    TChain* chain = new TChain("hits");
    Int_t nFiles = chain->Add(pattern);

    if (nFiles == 0) {
        std::cerr << "Error: No files found matching pattern " << pattern << std::endl;
        delete chain;
        return;
    }

    std::cout << "Found " << nFiles << " files to merge" << std::endl;

    // Create temporary merged file
    TFile* ftemp = TFile::Open("_temp_merged.root", "RECREATE");
    TTree* merged = chain->CloneTree(-1, "fast");
    merged->Write();
    ftemp->Close();

    // Analyze merged file
    analyze_efficiency("_temp_merged.root", total_neutrons);

    // Cleanup
    delete chain;
    delete merged;
    delete ftemp;

    // Remove temporary file
    gSystem->Unlink("_temp_merged.root");
}
