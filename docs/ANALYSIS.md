# NBox Analysis Cookbook

Practical ROOT analysis examples for NBox simulation data.

## Table of Contents

1. [Output File Structure](#output-file-structure)
2. [Basic Analysis](#basic-analysis)
3. [Efficiency Calculations](#efficiency-calculations)
4. [Detector Comparisons](#detector-comparisons)
5. [Time Analysis](#time-analysis)
6. [Multi-File Analysis](#multi-file-analysis)
7. [Advanced Techniques](#advanced-techniques)

---

## Output File Structure

### File Naming Convention

```
output_run0.root         # Main merged file (if merging enabled)
output_run0_t0.root      # Thread 0 output
output_run0_t1.root      # Thread 1 output
...
output_run0_tN.root      # Thread N output
```

### TTree Structure

**Tree name:** `hits`

**Branches:**

| Column | Type | Description | Example Values |
|--------|------|-------------|----------------|
| `EventID` | int | Unique event number | 0, 1, 2, ... |
| `DetectorID` | int | Detector instance ID | 0-27 for 28 tubes |
| `DetectorName` | string | Detector instance name | "A1", "B5", "C12" |
| `Edep_keV` | double | Energy deposition (keV) | 764, 382, 120 |
| `Time_ns` | double | Hit time (nanoseconds) | 10.5, 234.7 |

### Quick Inspection

```bash
root -l output_run0_t0.root
```

```cpp
// ROOT prompt

// List contents
.ls

// Check tree structure
hits->Print()

// Get number of entries
hits->GetEntries()

// Show first 10 entries
hits->Scan("EventID:DetectorID:DetectorName:Edep_keV:Time_ns", "", "", 10)
```

---

## Basic Analysis

### Energy Spectrum

#### Full Energy Spectrum

```cpp
// Open file
TFile* f = TFile::Open("output_run0_t0.root");
TTree* hits = (TTree*)f->Get("hits");

// Draw energy histogram
TCanvas* c1 = new TCanvas("c1", "Energy Spectrum", 800, 600);
hits->Draw("Edep_keV>>h_energy(200, 0, 1000)", "", "");

// Get histogram and format
TH1F* h_energy = (TH1F*)gDirectory->Get("h_energy");
h_energy->SetTitle("Energy Deposition;Energy [keV];Counts");
h_energy->SetLineColor(kBlue);
h_energy->SetLineWidth(2);

// Identify peak
int maxBin = h_energy->GetMaximumBin();
double peakEnergy = h_energy->GetBinCenter(maxBin);
cout << "Peak at: " << peakEnergy << " keV" << endl;

c1->SaveAs("energy_spectrum.png");
```

**Expected result:** Sharp peak at ~764 keV (He-3 reaction Q-value)

#### Energy Spectrum by Detector

```cpp
// Compare first 4 detectors
TCanvas* c2 = new TCanvas("c2", "Energy by Detector", 1200, 800);
c2->Divide(2, 2);

for (int det = 0; det < 4; det++) {
    c2->cd(det + 1);
    TString cut = TString::Format("DetectorID==%d", det);
    TString hist = TString::Format("h_det%d(200, 0, 1000)", det);
    hits->Draw("Edep_keV>>" + hist, cut, "");

    TH1F* h = (TH1F*)gDirectory->Get(TString::Format("h_det%d", det));
    h->SetTitle(TString::Format("Detector %d;Energy [keV];Counts", det));
    h->SetLineColor(kBlue + det);
}

c2->SaveAs("energy_by_detector.png");
```

### Detector Hit Distribution

```cpp
// Count hits per detector
TCanvas* c3 = new TCanvas("c3", "Hits per Detector", 1000, 600);
hits->Draw("DetectorID>>h_detid(28, -0.5, 27.5)", "", "");

TH1F* h_detid = (TH1F*)gDirectory->Get("h_detid");
h_detid->SetTitle("Hit Distribution;Detector ID;Number of Hits");
h_detid->SetFillColor(kGreen);
h_detid->SetStats(1);

// Print statistics
for (int i = 1; i <= h_detid->GetNbinsX(); i++) {
    int detID = i - 1;
    int counts = h_detid->GetBinContent(i);
    cout << "Detector " << detID << ": " << counts << " hits" << endl;
}

c3->SaveAs("hits_per_detector.png");
```

### 2D Plots

#### Energy vs Detector

```cpp
TCanvas* c4 = new TCanvas("c4", "Energy vs Detector", 1000, 700);
hits->Draw("DetectorID:Edep_keV>>h2d(100, 0, 1000, 28, -0.5, 27.5)", "", "colz");

TH2F* h2d = (TH2F*)gDirectory->Get("h2d");
h2d->SetTitle("Energy vs Detector;Energy [keV];Detector ID");
h2d->GetXaxis()->SetTitleOffset(1.2);
h2d->GetYaxis()->SetTitleOffset(1.2);

c4->SetRightMargin(0.15);
c4->SaveAs("energy_vs_detector.png");
```

#### Energy vs Time

```cpp
TCanvas* c5 = new TCanvas("c5", "Energy vs Time", 1000, 700);
hits->Draw("Time_ns:Edep_keV>>h2d_time(100, 0, 1000, 100, 0, 1000)", "", "colz");

TH2F* h2d_time = (TH2F*)gDirectory->Get("h2d_time");
h2d_time->SetTitle("Energy vs Time;Energy [keV];Time [ns]");

c5->SetRightMargin(0.15);
c5->SaveAs("energy_vs_time.png");
```

---

## Efficiency Calculations

### Intrinsic Efficiency

**Definition:** Fraction of emitted neutrons that were detected.

```cpp
void calculate_efficiency(const char* filename, int total_neutrons) {
    TFile* f = TFile::Open(filename);
    TTree* hits = (TTree*)f->Get("hits");

    // Count unique events with hits
    Long64_t detected_events = hits->GetEntries("Edep_keV > 0");

    // Calculate efficiency
    double efficiency = (double)detected_events / total_neutrons * 100.0;

    cout << "========================================" << endl;
    cout << "Efficiency Analysis" << endl;
    cout << "========================================" << endl;
    cout << "Total neutrons emitted:  " << total_neutrons << endl;
    cout << "Detected events:         " << detected_events << endl;
    cout << "Intrinsic efficiency:    " << efficiency << " %" << endl;
    cout << "========================================" << endl;

    f->Close();
}

// Usage:
// calculate_efficiency("output_run0_t0.root", 100000)
```

### Efficiency by Energy Threshold

```cpp
void efficiency_vs_threshold(const char* filename, int total_neutrons) {
    TFile* f = TFile::Open(filename);
    TTree* hits = (TTree*)f->Get("hits");

    TCanvas* c = new TCanvas("c", "Efficiency vs Threshold", 800, 600);

    const int nPoints = 50;
    double threshold[nPoints], eff[nPoints];

    for (int i = 0; i < nPoints; i++) {
        threshold[i] = i * 20.0;  // 0 to 1000 keV
        TString cut = TString::Format("Edep_keV > %.1f", threshold[i]);
        Long64_t count = hits->GetEntries(cut.Data());
        eff[i] = (double)count / total_neutrons * 100.0;
    }

    TGraph* gr = new TGraph(nPoints, threshold, eff);
    gr->SetTitle("Efficiency vs Energy Threshold;Threshold [keV];Efficiency [%]");
    gr->SetLineColor(kBlue);
    gr->SetLineWidth(2);
    gr->SetMarkerStyle(20);
    gr->SetMarkerColor(kBlue);
    gr->Draw("ALP");

    c->SaveAs("efficiency_vs_threshold.png");

    f->Close();
}
```

### Detector-Specific Efficiency

```cpp
void detector_efficiency(const char* filename, int total_neutrons) {
    TFile* f = TFile::Open(filename);
    TTree* hits = (TTree*)f->Get("hits");

    // Get number of detectors
    int nDetectors = hits->GetMaximum("DetectorID") + 1;

    TCanvas* c = new TCanvas("c", "Detector Efficiency", 1000, 600);
    TH1F* h_eff = new TH1F("h_eff", "Efficiency by Detector;Detector ID;Efficiency [%]",
                           nDetectors, -0.5, nDetectors - 0.5);

    for (int det = 0; det < nDetectors; det++) {
        TString cut = TString::Format("DetectorID==%d && Edep_keV>0", det);
        Long64_t count = hits->GetEntries(cut.Data());
        double eff = (double)count / total_neutrons * 100.0;
        h_eff->SetBinContent(det + 1, eff);

        cout << "Detector " << det << ": " << eff << " %" << endl;
    }

    h_eff->SetFillColor(kOrange);
    h_eff->SetStats(0);
    h_eff->Draw("HIST");

    c->SaveAs("detector_efficiency.png");

    f->Close();
}
```

---

## Detector Comparisons

### Compare Rings (ELIGANT-TN)

```cpp
void compare_rings(const char* filename) {
    TFile* f = TFile::Open(filename);
    TTree* hits = (TTree*)f->Get("hits");

    // Define rings
    // A: 0-3, B: 4-11, C: 12-27
    struct Ring {
        const char* name;
        int minID;
        int maxID;
        int color;
    };

    Ring rings[] = {
        {"A (Inner)", 0, 3, kRed},
        {"B (Middle)", 4, 11, kBlue},
        {"C (Outer)", 12, 27, kGreen}
    };

    TCanvas* c = new TCanvas("c", "Ring Comparison", 1200, 800);
    c->Divide(2, 2);

    // Energy spectra
    c->cd(1);
    TLegend* leg1 = new TLegend(0.6, 0.6, 0.89, 0.89);
    for (int i = 0; i < 3; i++) {
        TString cut = TString::Format("DetectorID>=%d && DetectorID<=%d",
                                      rings[i].minID, rings[i].maxID);
        TString hist = TString::Format("h_ring%d(200, 0, 1000)", i);
        hits->Draw("Edep_keV>>" + hist, cut, i == 0 ? "" : "same");

        TH1F* h = (TH1F*)gDirectory->Get(TString::Format("h_ring%d", i));
        h->SetLineColor(rings[i].color);
        h->SetLineWidth(2);
        leg1->AddEntry(h, rings[i].name, "l");
    }
    leg1->Draw();
    gPad->SetLogy();

    // Hit counts
    c->cd(2);
    TH1F* h_counts = new TH1F("h_counts", "Hits by Ring;Ring;Number of Hits", 3, 0, 3);
    for (int i = 0; i < 3; i++) {
        TString cut = TString::Format("DetectorID>=%d && DetectorID<=%d",
                                      rings[i].minID, rings[i].maxID);
        Long64_t count = hits->GetEntries(cut.Data());
        h_counts->SetBinContent(i + 1, count);
        h_counts->GetXaxis()->SetBinLabel(i + 1, rings[i].name);
    }
    h_counts->SetFillColor(kCyan);
    h_counts->Draw("HIST");

    // Time distribution
    c->cd(3);
    TLegend* leg2 = new TLegend(0.6, 0.6, 0.89, 0.89);
    for (int i = 0; i < 3; i++) {
        TString cut = TString::Format("DetectorID>=%d && DetectorID<=%d",
                                      rings[i].minID, rings[i].maxID);
        TString hist = TString::Format("h_time%d(100, 0, 500)", i);
        hits->Draw("Time_ns>>" + hist, cut, i == 0 ? "" : "same");

        TH1F* h = (TH1F*)gDirectory->Get(TString::Format("h_time%d", i));
        h->SetLineColor(rings[i].color);
        h->SetLineWidth(2);
        leg2->AddEntry(h, rings[i].name, "l");
    }
    leg2->Draw();

    // Statistics table
    c->cd(4);
    TPaveText* pt = new TPaveText(0.1, 0.1, 0.9, 0.9, "NDC");
    pt->AddText("Ring Statistics");
    pt->AddLine(0, 0.85, 1, 0.85);

    for (int i = 0; i < 3; i++) {
        TString cut = TString::Format("DetectorID>=%d && DetectorID<=%d",
                                      rings[i].minID, rings[i].maxID);
        Long64_t count = hits->GetEntries(cut.Data());
        int nTubes = rings[i].maxID - rings[i].minID + 1;
        double avgPerTube = (double)count / nTubes;

        TString line = TString::Format("%s: %lld hits (%d tubes, %.1f avg)",
                                       rings[i].name, count, nTubes, avgPerTube);
        pt->AddText(line);
    }
    pt->Draw();

    c->SaveAs("ring_comparison.png");

    f->Close();
}
```

### Compare Source Types

```cpp
void compare_sources() {
    // Compare thermal vs Cf-252
    TFile* f1 = TFile::Open("output_thermal_t0.root");
    TFile* f2 = TFile::Open("output_cf252_t0.root");

    TTree* thermal = (TTree*)f1->Get("hits");
    TTree* cf252 = (TTree*)f2->Get("hits");

    TCanvas* c = new TCanvas("c", "Source Comparison", 1400, 600);
    c->Divide(2, 1);

    // Energy comparison
    c->cd(1);
    thermal->Draw("Edep_keV>>h_thermal(200, 0, 1000)", "", "");
    cf252->Draw("Edep_keV>>h_cf252(200, 0, 1000)", "", "same");

    TH1F* h_thermal = (TH1F*)gDirectory->Get("h_thermal");
    TH1F* h_cf252 = (TH1F*)gDirectory->Get("h_cf252");

    h_thermal->SetLineColor(kRed);
    h_thermal->SetLineWidth(2);
    h_cf252->SetLineColor(kBlue);
    h_cf252->SetLineWidth(2);

    TLegend* leg = new TLegend(0.6, 0.7, 0.89, 0.89);
    leg->AddEntry(h_thermal, "Thermal", "l");
    leg->AddEntry(h_cf252, "Cf-252", "l");
    leg->Draw();

    // Time comparison
    c->cd(2);
    thermal->Draw("Time_ns>>h_time_thermal(100, 0, 1000)", "", "");
    cf252->Draw("Time_ns>>h_time_cf252(100, 0, 1000)", "", "same");

    TH1F* h_time_thermal = (TH1F*)gDirectory->Get("h_time_thermal");
    TH1F* h_time_cf252 = (TH1F*)gDirectory->Get("h_time_cf252");

    h_time_thermal->SetLineColor(kRed);
    h_time_thermal->SetLineWidth(2);
    h_time_cf252->SetLineColor(kBlue);
    h_time_cf252->SetLineWidth(2);

    c->SaveAs("source_comparison.png");

    f1->Close();
    f2->Close();
}
```

---

## Time Analysis

### Time-of-Flight Distribution

```cpp
void analyze_time_of_flight(const char* filename) {
    TFile* f = TFile::Open(filename);
    TTree* hits = (TTree*)f->Get("hits");

    TCanvas* c = new TCanvas("c", "Time of Flight", 1200, 800);
    c->Divide(2, 2);

    // Overall time distribution
    c->cd(1);
    hits->Draw("Time_ns>>h_time(200, 0, 500)", "", "");
    TH1F* h_time = (TH1F*)gDirectory->Get("h_time");
    h_time->SetTitle("Time of Flight;Time [ns];Counts");
    h_time->SetLineColor(kBlue);
    h_time->SetLineWidth(2);
    gPad->SetLogy();

    // Early vs late hits (energy)
    c->cd(2);
    hits->Draw("Edep_keV>>h_early(200, 0, 1000)", "Time_ns<50", "");
    hits->Draw("Edep_keV>>h_late(200, 0, 1000)", "Time_ns>200", "same");

    TH1F* h_early = (TH1F*)gDirectory->Get("h_early");
    TH1F* h_late = (TH1F*)gDirectory->Get("h_late");

    h_early->SetLineColor(kRed);
    h_early->SetLineWidth(2);
    h_late->SetLineColor(kBlue);
    h_late->SetLineWidth(2);

    TLegend* leg = new TLegend(0.6, 0.7, 0.89, 0.89);
    leg->AddEntry(h_early, "Early (<50 ns)", "l");
    leg->AddEntry(h_late, "Late (>200 ns)", "l");
    leg->Draw();

    // Time vs detector
    c->cd(3);
    hits->Draw("DetectorID:Time_ns>>h2d_time_det(100, 0, 500, 28, -0.5, 27.5)", "", "colz");
    TH2F* h2d = (TH2F*)gDirectory->Get("h2d_time_det");
    h2d->SetTitle("Time vs Detector;Time [ns];Detector ID");

    // Mean time per detector
    c->cd(4);
    TProfile* prof = new TProfile("prof", "Mean Time per Detector;Detector ID;Mean Time [ns]",
                                  28, -0.5, 27.5);
    hits->Draw("Time_ns:DetectorID>>prof", "", "prof");
    prof->SetMarkerStyle(20);
    prof->SetMarkerColor(kBlue);
    prof->SetLineColor(kBlue);

    c->SaveAs("time_of_flight_analysis.png");

    f->Close();
}
```

### Hit Multiplicity vs Time

```cpp
void multiplicity_vs_time(const char* filename) {
    TFile* f = TFile::Open(filename);
    TTree* hits = (TTree*)f->Get("hits");

    // Count hits per event
    map<int, int> eventHits;
    map<int, double> eventTime;

    int eventID, detectorID;
    double time_ns;

    hits->SetBranchAddress("EventID", &eventID);
    hits->SetBranchAddress("DetectorID", &detectorID);
    hits->SetBranchAddress("Time_ns", &time_ns);

    Long64_t nEntries = hits->GetEntries();
    for (Long64_t i = 0; i < nEntries; i++) {
        hits->GetEntry(i);
        eventHits[eventID]++;
        if (eventTime.find(eventID) == eventTime.end()) {
            eventTime[eventID] = time_ns;
        } else {
            eventTime[eventID] = min(eventTime[eventID], time_ns);
        }
    }

    // Create graphs
    TGraph* gr = new TGraph();
    int point = 0;
    for (auto& p : eventHits) {
        int evtID = p.first;
        int mult = p.second;
        double t = eventTime[evtID];
        gr->SetPoint(point++, t, mult);
    }

    TCanvas* c = new TCanvas("c", "Multiplicity vs Time", 1000, 700);
    gr->SetTitle("Hit Multiplicity vs Time;First Hit Time [ns];Multiplicity");
    gr->SetMarkerStyle(20);
    gr->SetMarkerSize(0.5);
    gr->SetMarkerColor(kBlue);
    gr->Draw("AP");

    c->SaveAs("multiplicity_vs_time.png");

    f->Close();
}
```

---

## Multi-File Analysis

### Merge Thread Files

```cpp
void merge_thread_files(const char* pattern, const char* output) {
    TChain* chain = new TChain("hits");
    chain->Add(pattern);  // e.g., "output_run0_t*.root"

    TFile* fout = TFile::Open(output, "RECREATE");
    TTree* merged = chain->CloneTree(-1, "fast");
    merged->Write();
    fout->Close();

    cout << "Merged " << chain->GetNtrees() << " files" << endl;
    cout << "Total entries: " << merged->GetEntries() << endl;

    delete chain;
}

// Usage:
// merge_thread_files("output_run0_t*.root", "output_merged.root")
```

### Analyze Multiple Runs

```cpp
void compare_runs() {
    const char* files[] = {
        "run1_output_t0.root",
        "run2_output_t0.root",
        "run3_output_t0.root"
    };
    int nFiles = 3;

    TCanvas* c = new TCanvas("c", "Run Comparison", 1200, 800);
    c->Divide(2, 2);

    TLegend* leg = new TLegend(0.6, 0.6, 0.89, 0.89);

    for (int i = 0; i < nFiles; i++) {
        TFile* f = TFile::Open(files[i]);
        TTree* hits = (TTree*)f->Get("hits");

        c->cd(1);
        TString hist = TString::Format("h_energy%d(200, 0, 1000)", i);
        hits->Draw("Edep_keV>>" + hist, "", i == 0 ? "" : "same");

        TH1F* h = (TH1F*)gDirectory->Get(TString::Format("h_energy%d", i));
        h->SetLineColor(kBlue + i * 2);
        h->SetLineWidth(2);
        leg->AddEntry(h, TString::Format("Run %d", i + 1), "l");

        // Add more comparisons...
    }

    c->cd(1);
    leg->Draw();

    c->SaveAs("run_comparison.png");
}
```

---

## Advanced Techniques

### Custom Analysis Class

See [example/analyze_efficiency.C](../example/analyze_efficiency.C) for a complete example.

### Batch Processing Script

```bash
#!/bin/bash
# process_all.sh

for file in output_run0_t*.root; do
    echo "Processing $file"
    root -l -b -q "analyze.C(\"$file\")"
done

echo "All files processed"
```

### Python Analysis (using uproot)

```python
import uproot
import matplotlib.pyplot as plt
import numpy as np

# Read ROOT file
file = uproot.open("output_run0_t0.root")
tree = file["hits"]

# Get data
edep = tree["Edep_keV"].array(library="np")
detid = tree["DetectorID"].array(library="np")
time = tree["Time_ns"].array(library="np")

# Plot
fig, axes = plt.subplots(2, 2, figsize=(12, 10))

# Energy spectrum
axes[0, 0].hist(edep, bins=200, range=(0, 1000))
axes[0, 0].set_xlabel("Energy [keV]")
axes[0, 0].set_ylabel("Counts")
axes[0, 0].set_title("Energy Spectrum")

# Detector distribution
axes[0, 1].hist(detid, bins=28, range=(-0.5, 27.5))
axes[0, 1].set_xlabel("Detector ID")
axes[0, 1].set_ylabel("Counts")
axes[0, 1].set_title("Hit Distribution")

# Time distribution
axes[1, 0].hist(time, bins=100, range=(0, 500))
axes[1, 0].set_xlabel("Time [ns]")
axes[1, 0].set_ylabel("Counts")
axes[1, 0].set_title("Time of Flight")

# 2D: Energy vs Detector
h, xedges, yedges = np.histogram2d(edep, detid, bins=[100, 28], range=[[0, 1000], [-0.5, 27.5]])
axes[1, 1].imshow(h.T, aspect='auto', origin='lower', extent=[0, 1000, -0.5, 27.5])
axes[1, 1].set_xlabel("Energy [keV]")
axes[1, 1].set_ylabel("Detector ID")
axes[1, 1].set_title("Energy vs Detector")

plt.tight_layout()
plt.savefig("analysis_python.png")
```

---

## Quick Reference Scripts

All example scripts are in [example/](../example/) directory:

- `analyze_efficiency.C` - Calculate detection efficiency
- `compare_detectors.C` - Compare detector performance
- `create_plots.C` - Generate standard plots

---

## Next Steps

- **Understand the physics:** [PHYSICS.md](PHYSICS.md)
- **Optimize simulation:** [TIPS.md](TIPS.md)
- **Configure detectors:** [CONFIGURATION.md](CONFIGURATION.md)
