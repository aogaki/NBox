#ifndef EventAction_h
#define EventAction_h

#include "G4UserEventAction.hh"
#include "G4AnalysisManager.hh"
#include "globals.hh"

class RunAction;

class EventAction : public G4UserEventAction
{
public:
    EventAction() = default;
    ~EventAction() override = default;

    void BeginOfEventAction(const G4Event*) override;
    void EndOfEventAction(const G4Event*) override;

private:
    std::vector<G4int> fHCIDs;  // Multiple hits collection IDs
    G4bool fHCIDsInitialized = false;

    // Cached pointers to avoid repeated lookups
    RunAction* fRunAction = nullptr;
    G4AnalysisManager* fAnalysisManager = nullptr;
    G4int fTotalEvents = 0;
};

#endif
