#include "PrimaryGeneratorAction.hh"
#include "ConfigManager.hh"

#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"
#include "TH1.h"
#include "TF1.h"

#include <cmath>

PrimaryGeneratorAction::PrimaryGeneratorAction(const G4String& sourceFile)
    : fSourceFile(sourceFile)
{
    if (!fSourceFile.empty()) {
        G4cout << "PrimaryGeneratorAction: Source file = " << fSourceFile << G4endl;
    }

    fParticleGun = new G4ParticleGun(1);

    // Phase 5: Use neutron instead of gamma
    auto* neutron = G4ParticleTable::GetParticleTable()->FindParticle("neutron");
    fParticleGun->SetParticleDefinition(neutron);

    // Set source position to center of plastic box
    fParticleGun->SetParticlePosition(G4ThreeVector(0, 0, 0));

    // Cache source configuration once at construction (avoid per-event queries)
    auto* config = ConfigManager::GetInstance();
    fSourceHist = config->GetSourceHistogram();
    fSourceFunc = config->GetSourceFunction();
    fHasMonoEnergy = config->HasMonoEnergy();
    if (fHasMonoEnergy) {
        fMonoEnergy = config->GetMonoEnergy() * MeV;
    }

    G4cout << "Neutron source initialized at (0, 0, 0)" << G4endl;
}

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
    delete fParticleGun;
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
    // Sample neutron energy with priority (using cached pointers):
    // 1. ROOT histogram (if loaded)
    // 2. ROOT function (if loaded)
    // 3. Mono-energetic (if specified in config)
    // 4. Use G4ParticleGun's current energy (set by /gun/energy macro command)
    if (fSourceHist != nullptr) {
        fParticleGun->SetParticleEnergy(fSourceHist->GetRandom() * MeV);
    }
    else if (fSourceFunc != nullptr) {
        fParticleGun->SetParticleEnergy(fSourceFunc->GetRandom() * MeV);
    }
    else if (fHasMonoEnergy) {
        fParticleGun->SetParticleEnergy(fMonoEnergy);  // Already in Geant4 units
    }
    // else: use the energy already set in fParticleGun (via /gun/energy command)

    // Debug output for first event
    static bool firstEvent = true;
    if (firstEvent) {
        G4cout << "PRIMARY_ENERGY: " << fParticleGun->GetParticleEnergy() / CLHEP::eV << " eV" << G4endl;
        firstEvent = false;
    }

    // Generate isotropic 4π direction
    // Method: Sample cos(theta) uniformly in [-1, 1] and phi uniformly in [0, 2π]
    G4double cosTheta = 2.0 * G4UniformRand() - 1.0;
    G4double sinTheta = std::sqrt(1.0 - cosTheta * cosTheta);
    G4double phi = kTwoPi * G4UniformRand();

    fParticleGun->SetParticleMomentumDirection(
        G4ThreeVector(sinTheta * std::cos(phi),
                      sinTheta * std::sin(phi),
                      cosTheta));

    fParticleGun->GeneratePrimaryVertex(event);
}
