#include "FluxSteppingAction.hh"

#include "G4AnalysisManager.hh"
#include "G4Neutron.hh"
#include "G4RunManager.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"
#include "G4VPhysicalVolume.hh"
#include "G4Event.hh"

// Initialize static member
G4bool FluxSteppingAction::fEnabled = false;

FluxSteppingAction::FluxSteppingAction()
{
}

void FluxSteppingAction::UserSteppingAction(const G4Step* step)
{
    // Skip if flux recording is disabled
    if (!fEnabled) {
        return;
    }

    // Only track neutrons
    auto particle = step->GetTrack()->GetParticleDefinition();
    if (particle != G4Neutron::Neutron()) {
        return;
    }

    // Get pre-step point
    auto preStepPoint = step->GetPreStepPoint();
    if (!preStepPoint) {
        return;
    }

    // Check if in moderator (Plastic volume)
    auto volume = preStepPoint->GetPhysicalVolume();
    if (!volume) {
        return;
    }

    G4String volumeName = volume->GetName();
    if (volumeName != "ModeratorBox") {
        return;
    }

    // Get neutron kinetic energy in eV
    G4double energy = preStepPoint->GetKineticEnergy() / eV;

    // Only record thermal neutrons (E < 0.5 eV)
    if (energy > kThermalEnergyCut) {
        return;
    }

    // Get position
    G4ThreeVector pos = preStepPoint->GetPosition();

    // Get step length
    G4double stepLength = step->GetStepLength() / mm;

    // Get event ID
    G4int eventID = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();

    // Record to Ntuple (ID = 1, flux map data)
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->FillNtupleIColumn(1, 0, eventID);          // EventID
    analysisManager->FillNtupleDColumn(1, 1, pos.x() / mm);     // X [mm]
    analysisManager->FillNtupleDColumn(1, 2, pos.y() / mm);     // Y [mm]
    analysisManager->FillNtupleDColumn(1, 3, pos.z() / mm);     // Z [mm]
    analysisManager->FillNtupleDColumn(1, 4, energy);           // Energy [eV]
    analysisManager->FillNtupleDColumn(1, 5, stepLength);       // StepLength [mm]
    analysisManager->AddNtupleRow(1);
}
