#include "EventAction.hh"
#include "ConfigManager.hh"

#include <iomanip>

#include "G4AnalysisManager.hh"
#include "G4Event.hh"
#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4SDManager.hh"
#include "G4SystemOfUnits.hh"
#include "NBoxHit.hh"
#include "RunAction.hh"

void EventAction::BeginOfEventAction(const G4Event *)
{
  // 必要に応じて初期化
}

void EventAction::EndOfEventAction(const G4Event *event)
{
  // Progress report: print every 1000 events
  G4int eventID = event->GetEventID();
  if (eventID % 1000 == 0 && eventID > 0) {
    G4RunManager *runManager = G4RunManager::GetRunManager();
    const G4Run *currentRun = runManager->GetCurrentRun();
    G4int totalEvents = currentRun->GetNumberOfEventToBeProcessed();
    G4double progress = 100.0 * eventID / totalEvents;
    G4cout << "\tProgress: " << eventID << " / " << totalEvents << " events ("
           << std::fixed << std::setprecision(1) << progress << "%)" << G4endl;
  }

  // Initialize hits collection IDs on first event
  if (!fHCIDsInitialized) {
    auto* config = ConfigManager::GetInstance();
    auto* sdManager = G4SDManager::GetSDMpointer();

    fHCIDs.clear();
    for (G4int i = 0; i < config->GetNumPlacements(); i++) {
      const auto& placement = config->GetPlacement(i);
      G4String hcName = G4String("He3HitsCollection_") + placement.name.c_str();
      G4int hcID = sdManager->GetCollectionID(hcName);
      fHCIDs.push_back(hcID);
    }
    fHCIDsInitialized = true;
  }

  auto *hce = event->GetHCofThisEvent();
  if (!hce) return;

  auto *runAction = const_cast<RunAction *>(static_cast<const RunAction *>(
      G4RunManager::GetRunManager()->GetUserRunAction()));

  auto analysisManager = G4AnalysisManager::Instance();

  // Process hits from all detectors
  for (size_t detIdx = 0; detIdx < fHCIDs.size(); detIdx++) {
    auto *hc = static_cast<NBoxHitsCollection *>(hce->GetHC(fHCIDs[detIdx]));
    if (!hc) continue;

    G4int nHits = hc->entries();
    if (nHits == 0) continue;

    for (G4int i = 0; i < nHits; ++i) {
      auto *hit = (*hc)[i];
      G4double edep = hit->GetEdep();

      if (edep > 0.) {
        runAction->CountEvent();

        // ROOT ntupleにデータを書き込む
        analysisManager->FillNtupleIColumn(0, event->GetEventID());
        analysisManager->FillNtupleIColumn(1, hit->GetDetectorID());
        analysisManager->FillNtupleSColumn(2, hit->GetDetectorName());
        analysisManager->FillNtupleDColumn(3, edep / CLHEP::keV);
        analysisManager->FillNtupleDColumn(4, hit->GetTime() / CLHEP::ns);
        analysisManager->AddNtupleRow();
      }
    }
  }
}
