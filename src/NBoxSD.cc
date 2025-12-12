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
    auto* hit = new NBoxHit();
    hit->SetDetectorName(SensitiveDetectorName);
    hit->SetDetectorID(fDetectorID);
    fHitsCollection->insert(hit);
}

G4bool NBoxSD::ProcessHits(G4Step* step, G4TouchableHistory*)
{
    G4double edep = step->GetTotalEnergyDeposit();
    if (edep == 0.) return false;

    // 最初のヒットに加算
    auto* hit = (*fHitsCollection)[0];
    hit->AddEdep(edep);

    // 最初のヒット位置と時間を記録（オプション）
    if (hit->GetTime() == 0.) {
        hit->SetPosition(step->GetPreStepPoint()->GetPosition());
        hit->SetTime(step->GetPreStepPoint()->GetGlobalTime());
    }

    return true;
}

void NBoxSD::EndOfEvent(G4HCofThisEvent*)
{
    // 必要に応じてここで処理
}
