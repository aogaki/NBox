#include "PrimaryGeneratorAction.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"

PrimaryGeneratorAction::PrimaryGeneratorAction(const G4String& sourceFile)
    : fSourceFile(sourceFile)
{
    if (!fSourceFile.empty()) {
        G4cout << "PrimaryGeneratorAction: Source file = " << fSourceFile << G4endl;
    }

    fParticleGun = new G4ParticleGun(1);

    auto* gamma = G4ParticleTable::GetParticleTable()->FindParticle("gamma");
    fParticleGun->SetParticleDefinition(gamma);
    fParticleGun->SetParticleEnergy(511.0*keV);
    fParticleGun->SetParticlePosition(G4ThreeVector(0, 0, -3.0*cm));
    fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
}

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
    delete fParticleGun;
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
    fParticleGun->GeneratePrimaryVertex(event);
}
