#include "RunAction.hh"
#include "NBoxConstants.hh"

#include "G4AccumulableManager.hh"
#include "G4AnalysisManager.hh"
#include "G4Run.hh"
#include "G4SystemOfUnits.hh"
#include "G4Threading.hh"

RunAction::RunAction(G4bool enableFluxMap)
    : fEnableFluxMap(enableFluxMap)
{
    G4AccumulableManager::Instance()->Register(fEventCount);

    // Configure ROOT file output
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->SetDefaultFileType("root");
    analysisManager->SetVerboseLevel(1);

    // Disable file merging - each thread maintains separate file
    analysisManager->SetNtupleMerging(false);

    // Ntuple 0: Energy Deposition (existing)
    analysisManager->CreateNtuple("NBox", "Energy Deposition");
    analysisManager->CreateNtupleIColumn("EventID");      // column 0
    analysisManager->CreateNtupleIColumn("DetectorID");   // column 1
    analysisManager->CreateNtupleSColumn("DetectorName"); // column 2
    analysisManager->CreateNtupleDColumn("Edep_keV");     // column 3
    analysisManager->CreateNtupleDColumn("Time_ns");      // column 4
    analysisManager->FinishNtuple();

    // Ntuple 1: Flux Map (optional, for thermal neutron tracking)
    if (fEnableFluxMap) {
        analysisManager->CreateNtuple("FluxMap", "Thermal Neutron Flux");
        analysisManager->CreateNtupleIColumn("EventID");      // column 0
        analysisManager->CreateNtupleDColumn("X_mm");         // column 1
        analysisManager->CreateNtupleDColumn("Y_mm");         // column 2
        analysisManager->CreateNtupleDColumn("Z_mm");         // column 3
        analysisManager->CreateNtupleDColumn("Energy_eV");    // column 4
        analysisManager->CreateNtupleDColumn("StepLength_mm"); // column 5
        analysisManager->FinishNtuple();
        G4cout << "Flux map recording enabled" << G4endl;
    }
}

RunAction::~RunAction()
{
    // G4AnalysisManager is a singleton - do NOT delete it manually
    // It will be cleaned up automatically by Geant4
}

void RunAction::BeginOfRunAction(const G4Run* run)
{
    G4AccumulableManager::Instance()->Reset();

    auto analysisManager = G4AnalysisManager::Instance();

    if (IsMaster()) {
        G4cout << "Running with " << G4Threading::GetNumberOfRunningWorkerThreads()
               << " worker threads" << G4endl;
        if (fEnableFluxMap) {
            G4cout << "Flux map recording: ENABLED" << G4endl;
        }
    }

    // Open file for each thread
    G4String fileName = "output_run" + std::to_string(run->GetRunID());
    analysisManager->OpenFile(fileName);
}

void RunAction::EndOfRunAction(const G4Run* run)
{
    G4int nofEvents = run->GetNumberOfEvent();
    if (nofEvents == 0) return;

    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->Write();
    analysisManager->CloseFile();

    G4AccumulableManager::Instance()->Merge();

    if (IsMaster()) {
        G4cout << G4endl;
        G4cout << "========== Run Summary ==========" << G4endl;
        G4cout << " Run ID: " << run->GetRunID() << G4endl;
        G4cout << " Number of events: " << nofEvents << G4endl;
        G4cout << " Events with hits: " << fEventCount.GetValue() << G4endl;
        if (fEnableFluxMap) {
            G4cout << " Flux map recording: ENABLED" << G4endl;
        }
        G4cout << "=================================" << G4endl;
    }
}
