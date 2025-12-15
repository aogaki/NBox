// Script to create a simple thermal neutron source for testing
// Run with: root -l -q create_test_source.C

void create_test_source() {
    // Create output file
    TFile *file = new TFile("test_source.root", "RECREATE");

    // Create thermal neutron spectrum (Maxwell-Boltzmann at 293K)
    const double kT_eV = 0.025;  // Thermal energy at room temperature
    const double kT_MeV = kT_eV / 1.0e6;  // Convert to MeV

    // Create histogram from 1e-9 to 1e-7 MeV (thermal range)
    TH1D *h1 = new TH1D("neutron_energy", "Thermal Neutron Spectrum;Energy (MeV);Flux",
                        1000, 1e-9, 1e-7);

    // Fill with Maxwell-Boltzmann distribution
    for (int i = 0; i < 100000; ++i) {
        double E = gRandom->Exp(kT_MeV);  // Exponential distribution approximation
        if (E < 1e-7) {
            h1->Fill(E);
        }
    }

    // Get stats before closing
    double meanEnergy = h1->GetMean() * 1e6;  // Convert to eV
    int entries = h1->GetEntries();

    h1->Write();
    file->Close();

    std::cout << "Created test_source.root with thermal neutron spectrum" << std::endl;
    std::cout << "Mean energy: " << meanEnergy << " eV" << std::endl;
    std::cout << "Entries: " << entries << std::endl;

    // Don't delete file or h1 - ROOT will handle cleanup
}
