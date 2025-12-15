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

    G4cout << "Neutron source initialized at (0, 0, 0)" << G4endl;
}

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
    delete fParticleGun;
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
    // Sample neutron energy from ROOT histogram or function
    auto* config = ConfigManager::GetInstance();
    TH1* sourceHist = config->GetSourceHistogram();
    TF1* sourceFunc = config->GetSourceFunction();

    G4double energy = 1.0 * MeV;  // Default energy
    if (sourceHist != nullptr) {
        // Sample energy from histogram (histogram is in MeV)
        energy = sourceHist->GetRandom() * MeV;
    }
    else if (sourceFunc != nullptr) {
        // Sample energy from function (function is in MeV)
        energy = sourceFunc->GetRandom() * MeV;
    }
    fParticleGun->SetParticleEnergy(energy);

    // Generate isotropic 4π direction
    // Method: Sample cos(theta) uniformly in [-1, 1] and phi uniformly in [0, 2π]
    G4double cosTheta = 2.0 * G4UniformRand() - 1.0;  // Uniform in [-1, 1]
    G4double sinTheta = std::sqrt(1.0 - cosTheta * cosTheta);
    G4double phi = 2.0 * M_PI * G4UniformRand();      // Uniform in [0, 2π]

    G4double x = sinTheta * std::cos(phi);
    G4double y = sinTheta * std::sin(phi);
    G4double z = cosTheta;

    fParticleGun->SetParticleMomentumDirection(G4ThreeVector(x, y, z));

    fParticleGun->GeneratePrimaryVertex(event);
}
