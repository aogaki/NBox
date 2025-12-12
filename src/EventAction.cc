#include "EventAction.hh"

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

  // HitsCollection IDを取得
  if (fHCID < 0) {
    fHCID = G4SDManager::GetSDMpointer()->GetCollectionID("NBoxHitsCollection");
  }

  auto *hce = event->GetHCofThisEvent();
  if (!hce) return;

  auto *hc = static_cast<NBoxHitsCollection *>(hce->GetHC(fHCID));
  if (!hc) return;

  // ヒットを処理
  G4int nHits = hc->entries();
  if (nHits == 0) return;

  auto *runAction = const_cast<RunAction *>(static_cast<const RunAction *>(
      G4RunManager::GetRunManager()->GetUserRunAction()));

  auto analysisManager = G4AnalysisManager::Instance();

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
