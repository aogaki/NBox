// Check Phase 5 neutron hits
void check_hits() {
    TFile* f = new TFile("output_run0_t0.root");
    TTree* tree = (TTree*)f->Get("NBox");

    if (!tree) {
        cout << "No NBox tree found!" << endl;
        return;
    }

    Long64_t nEntries = tree->GetEntries();
    cout << "\n========== Phase 5 Neutron Hits ==========" << endl;
    cout << "Total hits: " << nEntries << endl;

    if (nEntries > 0) {
        cout << "\nAll hits:" << endl;
        tree->Scan("EventID:DetectorID:DetectorName:Edep:Time");

        cout << "\nEnergy deposition statistics:" << endl;
        tree->Draw("Edep>>h(100,0,1000)", "", "goff");
        TH1F* h = (TH1F*)gDirectory->Get("h");
        cout << "  Mean:   " << h->GetMean() << " keV" << endl;
        cout << "  RMS:    " << h->GetRMS() << " keV" << endl;

        cout << "\nExpected: ~764 keV from He3(n,p)H3" << endl;
    }
    cout << "=========================================\n" << endl;
}
