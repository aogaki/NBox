#include "EventAction.hh"
#include "ConfigManager.hh"
#include "NBoxConstants.hh"

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
  // Initialize as needed
}

void EventAction::EndOfEventAction(const G4Event *event)
{
  G4int eventID = event->GetEventID();

  // Initialize on first event (cache pointers and HCIDs)
  if (!fHCIDsInitialized) {
    auto* config = ConfigManager::GetInstance();
    auto* sdManager = G4SDManager::GetSDMpointer();

    fHCIDs.clear();
    fHCIDs.reserve(config->GetNumPlacements());
    for (G4int i = 0; i < config->GetNumPlacements(); i++) {
      const auto& placement = config->GetPlacement(i);
      G4String hcName = G4String("He3HitsCollection_") + placement.name.c_str();
      G4int hcID = sdManager->GetCollectionID(hcName);
      fHCIDs.push_back(hcID);
    }

    // Cache pointers to avoid per-event lookups
    fRunAction = const_cast<RunAction*>(static_cast<const RunAction*>(
        G4RunManager::GetRunManager()->GetUserRunAction()));
    fAnalysisManager = G4AnalysisManager::Instance();
    fTotalEvents = G4RunManager::GetRunManager()->GetCurrentRun()->GetNumberOfEventToBeProcessed();

    fHCIDsInitialized = true;
  }

  // Progress report: print every N events (use cached totalEvents)
  if (eventID % NBoxConstants::PROGRESS_REPORT_INTERVAL == 0 && eventID > 0) {
    G4double progress = 100.0 * eventID / fTotalEvents;
    G4cout << "\tProgress: " << eventID << " / " << fTotalEvents << " events ("
           << std::fixed << std::setprecision(1) << progress << "%)" << G4endl;
  }

  auto *hce = event->GetHCofThisEvent();
  if (!hce) return;

  // Process hits from all detectors (use cached pointers)
  for (size_t detIdx = 0; detIdx < fHCIDs.size(); detIdx++) {
    auto *hc = static_cast<NBoxHitsCollection *>(hce->GetHC(fHCIDs[detIdx]));
    if (!hc) continue;

    G4int nHits = hc->entries();
    if (nHits == 0) continue;

    for (G4int i = 0; i < nHits; ++i) {
      auto *hit = (*hc)[i];
      G4double edep = hit->GetEdep();

      if (edep > 0.) {
        fRunAction->CountEvent();

        // Write data to ROOT ntuple (use cached manager)
        fAnalysisManager->FillNtupleIColumn(NBoxConstants::NTUPLE_COL_EVENT_ID, eventID);
        fAnalysisManager->FillNtupleIColumn(NBoxConstants::NTUPLE_COL_DETECTOR_ID, hit->GetDetectorID());
        fAnalysisManager->FillNtupleSColumn(NBoxConstants::NTUPLE_COL_DETECTOR_NAME, hit->GetDetectorName());
        fAnalysisManager->FillNtupleDColumn(NBoxConstants::NTUPLE_COL_EDEP_KEV, edep / CLHEP::keV);
        fAnalysisManager->FillNtupleDColumn(NBoxConstants::NTUPLE_COL_TIME_NS, hit->GetTime() / CLHEP::ns);
        fAnalysisManager->AddNtupleRow();
      }
    }
  }
}
