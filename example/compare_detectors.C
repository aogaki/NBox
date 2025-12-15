// compare_detectors.C
// Compare performance of different detector configurations
//
// Usage: root -l compare_detectors.C
//        Then call: compare_detectors()
//        Or: compare_configurations("config1_output.root", "config2_output.root", 100000)

#include <iostream>
#include <map>
#include <set>
#include <vector>
#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TPaveText.h"
#include "TStyle.h"

// Structure to hold detector statistics
struct DetectorStats {
    std::string name;
    Long64_t total_hits;
    Long64_t unique_events;
    Double_t total_energy;
    Double_t avg_energy;
    Double_t efficiency;
};

// Analyze single configuration file
std::map<Int_t, DetectorStats> analyze_file(const char* filename, Long64_t total_neutrons) {
    std::map<Int_t, DetectorStats> results;

    TFile* f = TFile::Open(filename);
    if (!f || f->IsZombie()) {
        std::cerr << "Error: Cannot open " << filename << std::endl;
        return results;
    }

    TTree* hits = (TTree*)f->Get("hits");
    if (!hits) {
        std::cerr << "Error: Cannot find hits tree" << std::endl;
        f->Close();
        return results;
    }

    Int_t eventID, detectorID;
    Double_t edep_keV;
    std::string* detectorName = nullptr;

    hits->SetBranchAddress("EventID", &eventID);
    hits->SetBranchAddress("DetectorID", &detectorID);
    hits->SetBranchAddress("DetectorName", &detectorName);
    hits->SetBranchAddress("Edep_keV", &edep_keV);

    std::map<Int_t, std::set<Int_t>> eventsPerDetector;
    std::map<Int_t, Long64_t> hitsPerDetector;
    std::map<Int_t, Double_t> energyPerDetector;
    std::map<Int_t, std::string> detectorNames;

    Long64_t nEntries = hits->GetEntries();
    for (Long64_t i = 0; i < nEntries; i++) {
        hits->GetEntry(i);

        eventsPerDetector[detectorID].insert(eventID);
        hitsPerDetector[detectorID]++;
        energyPerDetector[detectorID] += edep_keV;
        detectorNames[detectorID] = *detectorName;
    }

    // Populate results
    for (auto& p : hitsPerDetector) {
        Int_t detID = p.first;
        DetectorStats stats;

        stats.name = detectorNames[detID];
        stats.total_hits = p.second;
        stats.unique_events = eventsPerDetector[detID].size();
        stats.total_energy = energyPerDetector[detID];
        stats.avg_energy = stats.total_energy / stats.total_hits;
        stats.efficiency = (double)stats.unique_events / total_neutrons * 100.0;

        results[detID] = stats;
    }

    delete detectorName;
    f->Close();
    delete f;

    return results;
}

// Compare ring performance for ELIGANT-TN style configurations
void compare_rings(const char* filename, Long64_t total_neutrons) {
    std::cout << "Analyzing ring performance..." << std::endl;

    auto detectorStats = analyze_file(filename, total_neutrons);

    // Define rings (adjust these based on your configuration)
    // ELIGANT-TN: A (0-3), B (4-11), C (12-27)
    struct RingStats {
        std::string name;
        std::vector<Int_t> detectorIDs;
        Long64_t total_hits;
        Long64_t unique_events;
        Double_t avg_efficiency;
    };

    std::vector<RingStats> rings = {
        {"A (Inner)", {0, 1, 2, 3}, 0, 0, 0.0},
        {"B (Middle)", {4, 5, 6, 7, 8, 9, 10, 11}, 0, 0, 0.0},
        {"C (Outer)", {12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27}, 0, 0, 0.0}
    };

    // Accumulate statistics per ring
    for (auto& ring : rings) {
        std::set<Int_t> ringEvents;
        for (Int_t detID : ring.detectorIDs) {
            if (detectorStats.find(detID) != detectorStats.end()) {
                auto& stats = detectorStats[detID];
                ring.total_hits += stats.total_hits;
                ring.avg_efficiency += stats.efficiency;
            }
        }
        ring.avg_efficiency /= ring.detectorIDs.size();
    }

    // Print comparison
    std::cout << "\n========================================" << std::endl;
    std::cout << "RING COMPARISON" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Ring     | Detectors | Total Hits | Avg Eff/Det" << std::endl;
    std::cout << "---------|-----------|------------|-------------" << std::endl;

    for (auto& ring : rings) {
        printf("%-8s | %9zu | %10lld | %10.4f%%\n",
               ring.name.c_str(),
               ring.detectorIDs.size(),
               ring.total_hits,
               ring.avg_efficiency);
    }

    std::cout << "========================================" << std::endl;

    // Create visualization
    TCanvas* c = new TCanvas("c_rings", "Ring Comparison", 1200, 800);
    c->Divide(2, 2);

    // Total hits per ring
    c->cd(1);
    TH1D* h_hits = new TH1D("h_hits", "Total Hits per Ring;Ring;Total Hits", 3, 0, 3);
    for (size_t i = 0; i < rings.size(); i++) {
        h_hits->SetBinContent(i + 1, rings[i].total_hits);
        h_hits->GetXaxis()->SetBinLabel(i + 1, rings[i].name.c_str());
    }
    h_hits->SetFillColor(kCyan);
    h_hits->Draw("HIST");

    // Average efficiency per ring
    c->cd(2);
    TH1D* h_eff = new TH1D("h_eff", "Average Efficiency per Ring;Ring;Efficiency [%]", 3, 0, 3);
    for (size_t i = 0; i < rings.size(); i++) {
        h_eff->SetBinContent(i + 1, rings[i].avg_efficiency);
        h_eff->GetXaxis()->SetBinLabel(i + 1, rings[i].name.c_str());
    }
    h_eff->SetFillColor(kOrange);
    h_eff->Draw("HIST");

    // Hits per detector in each ring
    c->cd(3);
    TH1D* h_a = new TH1D("h_a", "Ring A", 4, -0.5, 3.5);
    TH1D* h_b = new TH1D("h_b", "Ring B", 8, -0.5, 7.5);
    TH1D* h_c = new TH1D("h_c", "Ring C", 16, -0.5, 15.5);

    for (size_t i = 0; i < rings[0].detectorIDs.size(); i++) {
        Int_t detID = rings[0].detectorIDs[i];
        if (detectorStats.find(detID) != detectorStats.end()) {
            h_a->SetBinContent(i + 1, detectorStats[detID].total_hits);
        }
    }

    h_a->SetLineColor(kRed);
    h_a->SetLineWidth(2);
    h_a->GetXaxis()->SetTitle("Detector Index in Ring");
    h_a->GetYaxis()->SetTitle("Hits");
    h_a->Draw("HIST");

    // Summary text
    c->cd(4);
    TPaveText* pt = new TPaveText(0.1, 0.1, 0.9, 0.9, "NDC");
    pt->AddText("RING SUMMARY");
    pt->AddLine(0, 0.85, 1, 0.85);
    pt->AddText(" ");

    for (auto& ring : rings) {
        pt->AddText(Form("%s: %lld hits (%.3f%% eff)",
                         ring.name.c_str(),
                         ring.total_hits,
                         ring.avg_efficiency));
    }

    pt->SetTextAlign(12);
    pt->SetTextSize(0.04);
    pt->Draw();

    c->SaveAs("ring_comparison.png");
    std::cout << "Ring comparison saved to: ring_comparison.png" << std::endl;

    delete h_hits;
    delete h_eff;
    delete h_a;
    delete h_b;
    delete h_c;
    delete pt;
    delete c;
}

// Compare two different configurations
void compare_configurations(const char* file1, const char* file2,
                             Long64_t total_neutrons,
                             const char* label1 = "Config 1",
                             const char* label2 = "Config 2") {

    std::cout << "Comparing two configurations..." << std::endl;
    std::cout << "  " << label1 << ": " << file1 << std::endl;
    std::cout << "  " << label2 << ": " << file2 << std::endl;

    // Analyze both files
    auto stats1 = analyze_file(file1, total_neutrons);
    auto stats2 = analyze_file(file2, total_neutrons);

    // Calculate overall efficiencies
    std::set<Int_t> events1, events2;
    Long64_t hits1 = 0, hits2 = 0;

    for (auto& p : stats1) {
        events1.insert(p.first);
        hits1 += p.second.total_hits;
    }

    for (auto& p : stats2) {
        events2.insert(p.first);
        hits2 += p.second.total_hits;
    }

    // Calculate total efficiency
    // Need to re-read files to get unique events
    TFile* f1 = TFile::Open(file1);
    TTree* t1 = (TTree*)f1->Get("hits");
    Int_t evtID;
    t1->SetBranchAddress("EventID", &evtID);
    std::set<Int_t> uniqueEvents1;
    for (Long64_t i = 0; i < t1->GetEntries(); i++) {
        t1->GetEntry(i);
        uniqueEvents1.insert(evtID);
    }
    f1->Close();

    TFile* f2 = TFile::Open(file2);
    TTree* t2 = (TTree*)f2->Get("hits");
    t2->SetBranchAddress("EventID", &evtID);
    std::set<Int_t> uniqueEvents2;
    for (Long64_t i = 0; i < t2->GetEntries(); i++) {
        t2->GetEntry(i);
        uniqueEvents2.insert(evtID);
    }
    f2->Close();

    double eff1 = (double)uniqueEvents1.size() / total_neutrons * 100.0;
    double eff2 = (double)uniqueEvents2.size() / total_neutrons * 100.0;

    // Print comparison
    std::cout << "\n========================================" << std::endl;
    std::cout << "CONFIGURATION COMPARISON" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Metric              | " << label1 << " | " << label2 << " | Ratio" << std::endl;
    std::cout << "--------------------|----------|----------|-------" << std::endl;
    printf("Detected events     | %8zu | %8zu | %6.3f\n",
           uniqueEvents1.size(), uniqueEvents2.size(),
           (double)uniqueEvents1.size() / uniqueEvents2.size());
    printf("Total hits          | %8lld | %8lld | %6.3f\n",
           hits1, hits2, (double)hits1 / hits2);
    printf("Efficiency (%%)      | %8.3f | %8.3f | %6.3f\n",
           eff1, eff2, eff1 / eff2);
    printf("Hits/event          | %8.2f | %8.2f | %6.3f\n",
           (double)hits1 / uniqueEvents1.size(),
           (double)hits2 / uniqueEvents2.size(),
           ((double)hits1 / uniqueEvents1.size()) / ((double)hits2 / uniqueEvents2.size()));
    std::cout << "========================================" << std::endl;

    if (eff1 > eff2) {
        double improvement = (eff1 - eff2) / eff2 * 100.0;
        std::cout << label1 << " is " << improvement << "% better than " << label2 << std::endl;
    } else {
        double improvement = (eff2 - eff1) / eff1 * 100.0;
        std::cout << label2 << " is " << improvement << "% better than " << label1 << std::endl;
    }

    std::cout << "========================================" << std::endl;

    delete f1;
    delete f2;
}

// Main comparison function (example usage)
void compare_detectors() {
    std::cout << "NBox Detector Comparison Tool" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nAvailable functions:" << std::endl;
    std::cout << "1. compare_rings(filename, total_neutrons)" << std::endl;
    std::cout << "   - Compare inner/middle/outer ring performance" << std::endl;
    std::cout << "\n2. compare_configurations(file1, file2, total_neutrons, label1, label2)" << std::endl;
    std::cout << "   - Compare two different detector setups" << std::endl;
    std::cout << "\nExample usage:" << std::endl;
    std::cout << "  compare_rings(\"output_run0_t0.root\", 100000)" << std::endl;
    std::cout << "  compare_configurations(\"config1.root\", \"config2.root\", 100000, \"Setup A\", \"Setup B\")" << std::endl;
    std::cout << "========================================" << std::endl;
}
