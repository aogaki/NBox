#include "NBoxSD.hh"
#include "G4HCofThisEvent.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4SDManager.hh"
#include "G4SystemOfUnits.hh"

NBoxSD::NBoxSD(const G4String& name, const G4String& hitsCollectionName, G4int detectorID)
    : G4VSensitiveDetector(name), fDetectorID(detectorID)
{
    collectionName.insert(hitsCollectionName);
}

void NBoxSD::Initialize(G4HCofThisEvent* hce)
{
    fHitsCollection = new NBoxHitsCollection(SensitiveDetectorName, collectionName[0]);

    if (fHCID < 0) {
        fHCID = G4SDManager::GetSDMpointer()->GetCollectionID(collectionName[0]);
    }
    hce->AddHitsCollection(fHCID, fHitsCollection);

    // イベント毎に1つのヒット（エネルギー合算用）
    fCurrentHit = new NBoxHit();
    fCurrentHit->SetDetectorName(SensitiveDetectorName);
    fCurrentHit->SetDetectorID(fDetectorID);
    fHitsCollection->insert(fCurrentHit);
    fFirstHit = true;  // Reset for new event
}

G4bool NBoxSD::ProcessHits(G4Step* step, G4TouchableHistory*)
{
    G4double edep = step->GetTotalEnergyDeposit();
    if (edep == 0.) return false;

    // Use cached hit pointer (avoid collection lookup)
    fCurrentHit->AddEdep(edep);

    // Record first hit position and time (use flag instead of time==0 check)
    if (fFirstHit) {
        fFirstHit = false;
        auto* preStep = step->GetPreStepPoint();
        fCurrentHit->SetPosition(preStep->GetPosition());
        fCurrentHit->SetTime(preStep->GetGlobalTime());
    }

    return true;
}

void NBoxSD::EndOfEvent(G4HCofThisEvent*)
{
    // 必要に応じてここで処理
}
