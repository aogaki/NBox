// ROOT macro to create thermal neutron energy spectrum
// Thermal neutrons have much higher capture probability in He3
void create_thermal_source()
{
    TFile* f = new TFile("thermal_source.root", "RECREATE");

    // Create thermal neutron energy spectrum
    // Histogram units: MeV (Geant4 native energy unit)
    TH1D* h = new TH1D("thermal_neutrons", "Thermal Neutron Spectrum;Energy (MeV);Counts", 1000, 0, 0.001);

    // Thermal neutron peak at 0.025 eV = 0.000025 MeV
    // Maxwell-Boltzmann distribution at T=293K (room temperature)
    TRandom3 rng;

    // Most probable energy: E0 = kT where k = 8.617e-5 eV/K
    // At 293K: E0 = 0.0252 eV = 0.0000252 MeV
    double kT = 0.0000252;  // MeV

    // Fill with Maxwell-Boltzmann distribution
    // P(E) ∝ E * exp(-E/kT) / (kT)^2
    for (int i = 0; i < 1000000; i++) {
        // Sample from Maxwell-Boltzmann using rejection method
        double E, prob;
        do {
            E = rng.Exp(kT);  // Exponential sampling
            prob = E / kT;     // Weight factor for Maxwell-Boltzmann
        } while (rng.Uniform() > prob);

        if (E < 0.001) {  // Keep only < 1 keV (0.001 MeV)
            h->Fill(E);
        }
    }

    h->Write();
    f->Close();

    std::cout << "\n========================================" << std::endl;
    std::cout << "Created thermal_source.root" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Thermal neutron spectrum (T=293K)" << std::endl;
    std::cout << "  Peak energy: ~0.025 eV (0.000025 MeV)" << std::endl;
    std::cout << "  Energy range: 0-1 keV (0-0.001 MeV)" << std::endl;
    std::cout << "  Distribution: Maxwell-Boltzmann" << std::endl;
    std::cout << "  Mean energy: " << h->GetMean() * 1000 << " keV" << std::endl;
    std::cout << "\nExpected He3 behavior:" << std::endl;
    std::cout << "  Cross-section: ~5330 barns (very high!)" << std::endl;
    std::cout << "  Reaction: He3(n,p)H3" << std::endl;
    std::cout << "  Q-value: 764 keV" << std::endl;
    std::cout << "  Detection efficiency: MUCH higher than fast neutrons" << std::endl;
    std::cout << "========================================\n" << std::endl;
}
