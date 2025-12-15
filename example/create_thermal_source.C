// create_thermal_source.C
// Creates a thermal neutron energy spectrum (Maxwell-Boltzmann distribution)
// for use with NBox simulation
//
// Usage: root -l -b -q create_thermal_source.C

void create_thermal_source() {
    // Create histogram for thermal neutron spectrum
    // Energy range: 0 to 0.1 MeV (100 keV)
    // Most probable energy: 0.025 eV at room temperature (293 K)
    const int nBins = 1000;
    const double Emin = 0.0;      // MeV
    const double Emax = 0.0001;   // MeV (= 100 eV)

    TH1D* h = new TH1D("thermal_neutron_spectrum",
                       "Thermal Neutron Spectrum (293K)",
                       nBins, Emin, Emax);

    // Maxwell-Boltzmann distribution parameters
    const double kT_eV = 0.025;  // Thermal energy at 293K in eV
    const double kT_MeV = kT_eV * 1e-6;  // Convert to MeV

    // Fill histogram with Maxwell-Boltzmann distribution
    // φ(E) = C × E × exp(-E/kT)
    // where C is normalization constant

    for (int i = 1; i <= nBins; i++) {
        double E = h->GetBinCenter(i);  // Energy in MeV

        if (E > 0) {
            // Maxwell-Boltzmann formula
            double flux = E * TMath::Exp(-E / kT_MeV);
            h->SetBinContent(i, flux);
        }
    }

    // Normalize to unit integral
    double integral = h->Integral();
    if (integral > 0) {
        h->Scale(1.0 / integral);
    }

    // Create output file
    TFile* fout = TFile::Open("thermal_source.root", "RECREATE");
    if (!fout || fout->IsZombie()) {
        std::cerr << "Error: Cannot create output file thermal_source.root" << std::endl;
        return;
    }

    // Write histogram
    h->Write();
    fout->Close();

    // Print summary
    std::cout << "========================================" << std::endl;
    std::cout << "Thermal Neutron Source Created" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Output file: thermal_source.root" << std::endl;
    std::cout << "Spectrum type: TH1D" << std::endl;
    std::cout << "Object name: thermal_neutron_spectrum" << std::endl;
    std::cout << "Energy range: " << Emin*1e6 << " to " << Emax*1e6 << " eV" << std::endl;
    std::cout << "Temperature: 293 K (20°C)" << std::endl;
    std::cout << "Most probable energy: " << kT_eV << " eV" << std::endl;
    std::cout << "Mean energy: " << 1.5 * kT_eV << " eV" << std::endl;
    std::cout << "========================================" << std::endl;

    // Optional: Draw and save plot
    TCanvas* c = new TCanvas("c", "Thermal Neutron Spectrum", 800, 600);
    h->GetXaxis()->SetTitle("Energy [MeV]");
    h->GetYaxis()->SetTitle("Relative Flux (arbitrary units)");
    h->SetLineColor(kBlue);
    h->SetLineWidth(2);
    h->Draw("HIST");

    c->SaveAs("thermal_spectrum.png");
    std::cout << "Plot saved to: thermal_spectrum.png" << std::endl;

    delete c;
    delete h;
    delete fout;

    std::cout << "\nTo use this source in NBox:" << std::endl;
    std::cout << "./build/nbox_sim -s thermal_source.root -g geometry.json -d detector.json -m run.mac" << std::endl;
}
