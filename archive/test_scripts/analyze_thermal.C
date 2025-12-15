// Analyze thermal neutron He3 capture results
void analyze_thermal() {
    // Merge thread files
    TChain* chain = new TChain("NBox");
    chain->Add("output_run0_t*.root");

    Long64_t nEntries = chain->GetEntries();

    cout << "\n========================================" << endl;
    cout << "Thermal Neutron He3 Capture Analysis" << endl;
    cout << "========================================" << endl;
    cout << "Total hits recorded: " << nEntries << endl;

    if (nEntries == 0) {
        cout << "No hits found!" << endl;
        return;
    }

    // Create canvas
    TCanvas* c1 = new TCanvas("c1", "Thermal Neutron Results", 1400, 900);
    c1->Divide(2, 2);

    // 1. Energy spectrum (linear)
    c1->cd(1);
    chain->Draw("Edep_keV>>h_edep(200, 0, 800)", "", "");
    TH1F* h_edep = (TH1F*)gDirectory->Get("h_edep");
    h_edep->SetTitle("Energy Deposition Spectrum;Energy (keV);Counts");
    h_edep->SetLineColor(kBlue);
    h_edep->SetLineWidth(2);

    // Mark expected peaks
    TLine* line764 = new TLine(764, 0, 764, h_edep->GetMaximum());
    line764->SetLineColor(kRed);
    line764->SetLineWidth(2);
    line764->SetLineStyle(2);
    line764->Draw();

    TLatex* text764 = new TLatex(764, h_edep->GetMaximum()*0.9, "764 keV (Q-value)");
    text764->SetTextColor(kRed);
    text764->SetTextSize(0.03);
    text764->Draw();

    // 2. Energy spectrum (log scale)
    c1->cd(2);
    chain->Draw("Edep_keV>>h_edep_log(200, 0, 800)", "", "");
    TH1F* h_edep_log = (TH1F*)gDirectory->Get("h_edep_log");
    h_edep_log->SetTitle("Energy Spectrum (Log);Energy (keV);Counts");
    h_edep_log->SetLineColor(kBlue);
    h_edep_log->SetLineWidth(2);
    gPad->SetLogy();

    // 3. Detector distribution
    c1->cd(3);
    chain->Draw("DetectorID>>h_det(3, -0.5, 2.5)", "", "");
    TH1F* h_det = (TH1F*)gDirectory->Get("h_det");
    h_det->SetTitle("Hits per Detector;Detector ID;Counts");
    h_det->SetLineColor(kGreen+2);
    h_det->SetLineWidth(2);
    h_det->SetFillColor(kGreen+2);
    h_det->SetFillStyle(3001);

    // 4. Time distribution
    c1->cd(4);
    chain->Draw("Time_ns>>h_time(100, 0, 100)", "", "");
    TH1F* h_time = (TH1F*)gDirectory->Get("h_time");
    h_time->SetTitle("Detection Time;Time (ns);Counts");
    h_time->SetLineColor(kMagenta);
    h_time->SetLineWidth(2);

    c1->SaveAs("thermal_neutron_results.png");

    // Print statistics
    cout << "\n--- Energy Deposition Statistics ---" << endl;
    cout << "  Mean:   " << h_edep->GetMean() << " keV" << endl;
    cout << "  RMS:    " << h_edep->GetRMS() << " keV" << endl;
    cout << "  Peak:   " << h_edep->GetBinCenter(h_edep->GetMaximumBin()) << " keV" << endl;

    cout << "\n--- Hits per Detector ---" << endl;
    for (int i = 0; i < 3; i++) {
        int hits = h_det->GetBinContent(i+1);
        double percent = 100.0 * hits / nEntries;
        cout << "  Detector " << i << ": " << hits << " hits (" << percent << "%)" << endl;
    }

    cout << "\n--- Energy Regions ---" << endl;
    int counts_full = chain->GetEntries("Edep_keV > 700 && Edep_keV < 800");
    int counts_wall = chain->GetEntries("Edep_keV > 200 && Edep_keV < 700");
    int counts_low = chain->GetEntries("Edep_keV < 200");
    cout << "  Full energy (700-800 keV): " << counts_full << " events (" << 100.0*counts_full/nEntries << "%)" << endl;
    cout << "  Wall effect (200-700 keV): " << counts_wall << " events (" << 100.0*counts_wall/nEntries << "%)" << endl;
    cout << "  Low energy (< 200 keV):    " << counts_low << " events (" << 100.0*counts_low/nEntries << "%)" << endl;

    cout << "\n--- Physics Explanation ---" << endl;
    cout << "He3(n,p)H3 Reaction:" << endl;
    cout << "  Q-value = 764 keV" << endl;
    cout << "  Proton energy = 573 keV" << endl;
    cout << "  Triton energy = 191 keV" << endl;
    cout << "\nFull Energy Peak (764 keV):" << endl;
    cout << "  Both particles deposit all energy in gas" << endl;
    cout << "\nWall Effect (< 764 keV):" << endl;
    cout << "  One or both particles hit tube wall" << endl;
    cout << "  Lose energy in aluminum before stopping" << endl;
    cout << "========================================\n" << endl;
}
