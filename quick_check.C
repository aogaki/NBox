void quick_check() {
    TFile* f = TFile::Open("output_run0_t0.root");
    TNtuple* nt = (TNtuple*)f->Get("NBox");
    if (nt) {
        cout << "\nEntries: " << nt->GetEntries() << endl;
        nt->Print();
        cout << "\nFirst 5 entries:" << endl;
        nt->Scan("", "", "", 5);
    }
}
