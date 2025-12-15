// create_AmBe_source.C
// Creates an AmBe (Americium-Beryllium) neutron energy spectrum
// for use with NBox simulation
//
// AmBe source: 241Am (α,n) 9Be reaction
// Produces neutrons with broad spectrum peaking around 4-5 MeV
//
// Usage: root -l -b -q create_AmBe_source.C

void create_AmBe_source() {
    // Create histogram for AmBe neutron spectrum
    // Energy range: 0 to 12 MeV
    // Peak around 4-5 MeV
    const int nBins = 1200;
    const double Emin = 0.0;    // MeV
    const double Emax = 12.0;   // MeV

    TH1D* h = new TH1D("ambe_neutron_spectrum",
                       "AmBe Neutron Spectrum",
                       nBins, Emin, Emax);

    // AmBe spectrum approximation
    // Based on ISO 8529-1 reference neutron sources
    // Simplified analytical model

    // Spectrum has multiple peaks due to excited states of 12C
    // Main peaks at: 2.5, 4.0, 5.5, 7.5, 9.0 MeV

    for (int i = 1; i <= nBins; i++) {
        double E = h->GetBinCenter(i);  // Energy in MeV

        if (E < 0.5) {
            // Very low energy: small contribution
            h->SetBinContent(i, 0.01);
        } else {
            // Multi-peak structure
            // Use sum of Gaussians for main peaks

            double flux = 0.0;

            // Peak 1: ~2.5 MeV
            flux += 0.15 * TMath::Gaus(E, 2.5, 0.8);

            // Peak 2: ~4.0 MeV (strongest)
            flux += 0.35 * TMath::Gaus(E, 4.0, 0.9);

            // Peak 3: ~5.5 MeV
            flux += 0.25 * TMath::Gaus(E, 5.5, 0.7);

            // Peak 4: ~7.5 MeV
            flux += 0.15 * TMath::Gaus(E, 7.5, 0.6);

            // Peak 5: ~9.0 MeV
            flux += 0.10 * TMath::Gaus(E, 9.0, 0.8);

            // Add exponential tail for high energies
            if (E > 8.0) {
                flux += 0.05 * TMath::Exp(-(E - 8.0) / 1.5);
            }

            h->SetBinContent(i, flux);
        }
    }

    // Normalize to unit integral
    double integral = h->Integral();
    if (integral > 0) {
        h->Scale(1.0 / integral);
    }

    // Create output file
    TFile* fout = TFile::Open("ambe_source.root", "RECREATE");
    if (!fout || fout->IsZombie()) {
        std::cerr << "Error: Cannot create output file ambe_source.root" << std::endl;
        return;
    }

    // Write histogram
    h->Write();
    fout->Close();

    // Calculate mean energy
    double meanEnergy = h->GetMean();
    double rmsEnergy = h->GetRMS();

    // Print summary
    std::cout << "========================================" << std::endl;
    std::cout << "AmBe Neutron Source Created" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Output file: ambe_source.root" << std::endl;
    std::cout << "Spectrum type: TH1D" << std::endl;
    std::cout << "Object name: ambe_neutron_spectrum" << std::endl;
    std::cout << "Energy range: " << Emin << " to " << Emax << " MeV" << std::endl;
    std::cout << "Mean energy: " << meanEnergy << " MeV" << std::endl;
    std::cout << "RMS energy: " << rmsEnergy << " MeV" << std::endl;
    std::cout << "Peak energies: ~2.5, 4.0, 5.5, 7.5, 9.0 MeV" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nPhysics:" << std::endl;
    std::cout << "  Reaction: 241Am → α + 237Np" << std::endl;
    std::cout << "           α + 9Be → 12C* + n" << std::endl;
    std::cout << "           12C* → 12C + γ (various levels)" << std::endl;
    std::cout << "  Typical source activity: 1-10 Ci" << std::endl;
    std::cout << "  Neutron yield: ~60 n/s per μCi of 241Am" << std::endl;
    std::cout << "========================================" << std::endl;

    // Optional: Draw and save plot
    TCanvas* c = new TCanvas("c", "AmBe Neutron Spectrum", 800, 600);
    c->SetLogy();
    h->GetXaxis()->SetTitle("Energy [MeV]");
    h->GetYaxis()->SetTitle("Relative Flux (arbitrary units)");
    h->SetLineColor(kRed);
    h->SetLineWidth(2);
    h->SetFillColor(kRed);
    h->SetFillStyle(3004);
    h->Draw("HIST");

    // Add text box with info
    TLatex* tex = new TLatex();
    tex->SetNDC();
    tex->SetTextSize(0.03);
    tex->DrawLatex(0.55, 0.75, "241Am-Be Source");
    tex->DrawLatex(0.55, 0.70, Form("Mean E: %.2f MeV", meanEnergy));

    c->SaveAs("ambe_spectrum.png");
    std::cout << "Plot saved to: ambe_spectrum.png" << std::endl;

    delete tex;
    delete c;
    delete h;
    delete fout;

    std::cout << "\nTo use this source in NBox:" << std::endl;
    std::cout << "./build/nbox_sim -s ambe_source.root -g geometry.json -d detector.json -m run.mac" << std::endl;
    std::cout << "\nNote: AmBe produces fast neutrons - ensure adequate moderation!" << std::endl;
    std::cout << "      Recommended moderator thickness: 10-15 cm polyethylene" << std::endl;
}
