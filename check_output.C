// ROOT macro to check ntuple structure
void check_output()
{
    TFile* f = TFile::Open("build/output_run0_t0.root", "READ");
    if (!f || f->IsZombie()) {
        std::cout << "Cannot open file!" << std::endl;
        return;
    }

    TTree* tree = (TTree*)f->Get("NBox");
    if (!tree) {
        std::cout << "Cannot find NBox tree!" << std::endl;
        f->Close();
        return;
    }

    std::cout << "=== NBox Tree Structure ===" << std::endl;
    std::cout << "Entries: " << tree->GetEntries() << std::endl;
    std::cout << "\nBranches:" << std::endl;
    tree->Print();

    if (tree->GetEntries() > 0) {
        std::cout << "\n=== First 10 Entries ===" << std::endl;
        tree->Scan("", "", "", 10);
    }

    f->Close();
}
