#ifndef RunAction_h
#define RunAction_h

#include "G4UserRunAction.hh"
#include "G4Accumulable.hh"
#include "globals.hh"

class RunAction : public G4UserRunAction
{
public:
    RunAction(G4bool enableFluxMap = false);
    ~RunAction() override;

    void BeginOfRunAction(const G4Run*) override;
    void EndOfRunAction(const G4Run*) override;

    void CountEvent() { fEventCount += 1; }

    G4bool IsFluxMapEnabled() const { return fEnableFluxMap; }

private:
    G4Accumulable<G4int> fEventCount{0};
    G4bool fEnableFluxMap{false};
};

#endif
