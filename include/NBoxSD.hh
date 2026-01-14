#ifndef NBoxSD_h
#define NBoxSD_h

#include "G4VSensitiveDetector.hh"
#include "NBoxHit.hh"

class NBoxSD : public G4VSensitiveDetector
{
public:
    NBoxSD(const G4String& name, const G4String& hitsCollectionName, G4int detectorID);
    ~NBoxSD() override = default;

    void Initialize(G4HCofThisEvent* hitCollection) override;
    G4bool ProcessHits(G4Step* step, G4TouchableHistory* history) override;
    void EndOfEvent(G4HCofThisEvent* hitCollection) override;

private:
    NBoxHitsCollection* fHitsCollection = nullptr;
    NBoxHit* fCurrentHit = nullptr;  // Cache pointer to avoid collection lookup
    G4int fHCID = -1;
    G4int fDetectorID = -1;
    G4bool fFirstHit = true;  // Track first hit to avoid repeated time==0 check
};

#endif
