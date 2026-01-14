#ifndef PrimaryGeneratorAction_h
#define PrimaryGeneratorAction_h

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "globals.hh"

class TH1;
class TF1;

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
public:
    PrimaryGeneratorAction(const G4String& sourceFile = "");
    ~PrimaryGeneratorAction() override;

    void GeneratePrimaries(G4Event* event) override;

private:
    G4ParticleGun* fParticleGun = nullptr;
    G4String fSourceFile;

    // Cached source configuration (avoid per-event ConfigManager queries)
    TH1* fSourceHist = nullptr;
    TF1* fSourceFunc = nullptr;
    G4double fMonoEnergy = 0.0;
    G4bool fHasMonoEnergy = false;

    // Pre-computed constants
    static constexpr G4double kTwoPi = 2.0 * 3.14159265358979323846;
};

#endif
