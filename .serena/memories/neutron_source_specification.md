# Neutron Source Specification

## Source Characteristics

### Energy Distribution
- **Source**: ROOT histogram (TH1) loaded from file
- **Sampling**: Use `TH1::GetRandom()` to sample energy from histogram
- **Units**: Histogram should be in MeV (Geant4 native energy unit)

### Angular Distribution
- **Type**: Isotropic (uniform in 4π solid angle)
- **Direction**: Random uniform distribution over entire sphere
- **Implementation**: Sample (theta, phi) uniformly

## Isotropic Direction Sampling

To generate uniform random directions in 4π:

```cpp
// Method 1: Using cosine of theta
double cosTheta = 2.0 * G4UniformRand() - 1.0;  // cos(theta) in [-1, 1]
double sinTheta = sqrt(1.0 - cosTheta * cosTheta);
double phi = 2.0 * CLHEP::pi * G4UniformRand();  // phi in [0, 2π]

double px = sinTheta * cos(phi);
double py = sinTheta * sin(phi);
double pz = cosTheta;

G4ThreeVector direction(px, py, pz);
```

**Why this works**: 
- Uniform sampling in cos(theta) gives uniform solid angle distribution
- Uniform sampling in phi gives azimuthal symmetry
- Result: Every direction in 4π has equal probability

## Source Position
- **Position**: Center of box (0, 0, 0) - unless specified otherwise in config
- **Type**: Point source (zero spatial extent)

## Implementation in PrimaryGeneratorAction

```cpp
void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
    auto* config = ConfigManager::GetInstance();
    
    // Sample energy from histogram
    if (config->IsSourceLoaded()) {
        TH1* hist = config->GetSourceHistogram();
        double energy = hist->GetRandom();  // MeV
        fParticleGun->SetParticleEnergy(energy * MeV);
    }
    
    // Generate isotropic direction (4π)
    double cosTheta = 2.0 * G4UniformRand() - 1.0;
    double sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    double phi = 2.0 * CLHEP::pi * G4UniformRand();
    
    double px = sinTheta * cos(phi);
    double py = sinTheta * sin(phi);
    double pz = cosTheta;
    
    fParticleGun->SetParticleMomentumDirection(G4ThreeVector(px, py, pz));
    
    // Set particle type to neutron
    auto* neutron = G4ParticleTable::GetParticleTable()->FindParticle("neutron");
    fParticleGun->SetParticleDefinition(neutron);
    
    // Set source position (center of box)
    fParticleGun->SetParticlePosition(G4ThreeVector(0, 0, 0));
    
    fParticleGun->GeneratePrimaryVertex(event);
}
```

## Physics Notes

### He3 Neutron Detection
- **Reaction**: ³He(n,p)³H
- **Q-value**: 764 keV
- **Cross-section**: Very high for thermal neutrons (~5330 barns at 0.025 eV)
- **Products**: 
  - Proton: ~573 keV
  - Triton: ~191 keV
  - Total deposited energy: ~764 keV

### Expected Behavior
- Neutrons emitted isotropically from center
- Neutrons interact with He3 gas in detector tubes
- Energy deposition recorded in sensitive detector
- Optimization goal: Find tube placements that maximize detection efficiency

## Test Histogram Creation

The test histogram (`test_source.root`) created in Phase 1 uses an exponential distribution:
```cpp
double energy = rng.Exp(0.5);  // Mean = 0.5 MeV
```

For realistic neutron spectra, users should provide:
- Thermal neutron spectrum (peaked at ~0.025 eV)
- Reactor spectrum
- Am-Be source spectrum
- Or custom spectrum matching their application
