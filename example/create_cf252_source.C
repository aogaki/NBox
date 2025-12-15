// ROOT macro to create Cf-252 spontaneous fission neutron spectrum using TF1
//
// Cf-252 neutron spectrum: Watt fission spectrum
// Parameterization: N(E) = C * exp(-E/a) * sinh(sqrt(b*E))
//
// Standard Cf-252 parameters (ISO 8529):
//   a = 1.025 MeV
//   b = 2.926 MeV^-1
//   Mean energy: ~2.13 MeV
//   Most probable energy: ~0.7 MeV
//
void create_cf252_source()
{
    // Watt spectrum parameters for Cf-252
    const double a = 1.025;    // MeV
    const double b = 2.926;    // MeV^-1

    // Create TF1 for Cf-252 Watt spectrum
    // Range: 0-20 MeV (Cf-252 spectrum extends to ~15 MeV with tail to 20)
    TF1* watt = new TF1("cf252_watt_spectrum",
                        "[0] * exp(-x/[1]) * sinh(sqrt([2]*x))",
                        0, 20);
    watt->SetParameter(0, 1.0);    // Normalization constant
    watt->SetParameter(1, a);      // a parameter
    watt->SetParameter(2, b);      // b parameter

    watt->SetTitle("Cf-252 Watt Fission Spectrum;Energy (MeV);Probability Density");
    watt->SetNpx(1000);  // Number of points for smooth function evaluation

    // Verify the spectrum by sampling
    TH1D* h_test = new TH1D("h_test", "Cf-252 Test Spectrum;Energy (MeV);Counts", 200, 0, 20);
    for (int i = 0; i < 100000; i++) {
        h_test->Fill(watt->GetRandom());
    }

    // Print statistics
    std::cout << "\n=== Cf-252 Neutron Spectrum (TF1) ===" << std::endl;
    std::cout << "Function: " << watt->GetName() << std::endl;
    std::cout << "Range: [" << watt->GetXmin() << ", " << watt->GetXmax() << "] MeV" << std::endl;
    std::cout << "\nTest sampling (100k events):" << std::endl;
    std::cout << "Mean energy:       " << h_test->GetMean() << " MeV" << std::endl;
    std::cout << "RMS:               " << h_test->GetRMS() << " MeV" << std::endl;
    std::cout << "Most probable:     " << h_test->GetBinCenter(h_test->GetMaximumBin()) << " MeV" << std::endl;
    std::cout << "\nExpected values for Cf-252:" << std::endl;
    std::cout << "Mean energy:       ~2.13 MeV" << std::endl;
    std::cout << "Most probable:     ~0.7 MeV" << std::endl;
    std::cout << "====================================\n" << std::endl;

    // Create visualization
    TCanvas* c = new TCanvas("c", "Cf-252 Neutron Spectrum", 1200, 500);
    c->Divide(2, 1);

    // Plot 1: TF1 function
    c->cd(1);
    gPad->SetLogy();
    watt->SetLineColor(kBlue);
    watt->SetLineWidth(2);
    watt->Draw();
    gPad->Update();

    // Plot 2: Test histogram from sampling
    c->cd(2);
    gPad->SetLogy();
    h_test->SetLineColor(kRed);
    h_test->SetLineWidth(2);
    h_test->Draw("HIST");

    TLine* line_mean = new TLine(h_test->GetMean(), h_test->GetMinimum(),
                                  h_test->GetMean(), h_test->GetMaximum());
    line_mean->SetLineColor(kBlack);
    line_mean->SetLineStyle(2);
    line_mean->Draw();

    TLegend* leg = new TLegend(0.5, 0.7, 0.89, 0.89);
    leg->AddEntry(h_test, "Sampled from TF1", "l");
    leg->AddEntry(line_mean, Form("Mean = %.2f MeV", h_test->GetMean()), "l");
    leg->SetBorderSize(0);
    leg->Draw();

    c->SaveAs("cf252_spectrum.png");
    std::cout << "Saved spectrum plot: cf252_spectrum.png" << std::endl;

    // Save TF1 to ROOT file
    TFile* f = new TFile("cf252_source.root", "RECREATE");
    watt->Write();
    f->Close();

    std::cout << "\nCreated cf252_source.root with Cf-252 neutron energy function (TF1)" << std::endl;
    std::cout << "Usage: ./build/nbox_sim -m run.mac -s cf252_source.root" << std::endl;
    std::cout << "\nNote: Direction will be sampled uniformly in 4π (isotropic)" << std::endl;

    delete h_test;
    delete c;
}
