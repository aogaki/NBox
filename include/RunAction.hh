#ifndef RunAction_h
#define RunAction_h

#include "G4UserRunAction.hh"
#include "G4Accumulable.hh"
#include "globals.hh"

class RunAction : public G4UserRunAction
{
public:
    RunAction();
    ~RunAction() override;

    void BeginOfRunAction(const G4Run*) override;
    void EndOfRunAction(const G4Run*) override;

    void CountEvent() { fEventCount += 1; }

private:
    G4Accumulable<G4int> fEventCount{0};
};

#endif
