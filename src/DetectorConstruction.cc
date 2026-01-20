#include "DetectorConstruction.hh"
#include "NBoxSD.hh"
#include "ConfigManager.hh"
#include "NBoxConstants.hh"

#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4SDManager.hh"

#include <cmath>

DetectorConstruction::DetectorConstruction(const G4String& geometryFile, const G4String& detectorFile)
    : fGeometryFile(geometryFile), fDetectorFile(detectorFile)
{
    if (!fGeometryFile.empty()) {
        G4cout << "DetectorConstruction: Geometry file = " << fGeometryFile << G4endl;
    }
    if (!fDetectorFile.empty()) {
        G4cout << "DetectorConstruction: Detector file = " << fDetectorFile << G4endl;
    }
}

void DetectorConstruction::DefineMaterials()
{
    G4NistManager* nist = G4NistManager::Instance();

    // He3 Isotope and Element (critical for thermal neutron capture)
    G4Isotope* he3Isotope = new G4Isotope("He3",
        NBoxConstants::HE3_ATOMIC_NUMBER,
        NBoxConstants::HE3_MASS_NUMBER,
        NBoxConstants::HE3_MOLAR_MASS * g/mole);

    fMaterials.he3Element = new G4Element("Helium3", "He3", 1);
    fMaterials.he3Element->AddIsotope(he3Isotope, NBoxConstants::HE3_ISOTOPE_ABUNDANCE * perCent);

    // Standard materials from NIST
    fMaterials.plastic = nist->FindOrBuildMaterial("G4_POLYETHYLENE");
    fMaterials.aluminum = nist->FindOrBuildMaterial("G4_Al");
    fMaterials.air = nist->FindOrBuildMaterial("G4_AIR");
    fMaterials.vacuum = nist->FindOrBuildMaterial("G4_Galactic");
}

void DetectorConstruction::CreateVisAttributes()
{
    fVisAttributes.plastic = new G4VisAttributes(G4Colour(
        NBoxConstants::VIS_PLASTIC_R,
        NBoxConstants::VIS_PLASTIC_G,
        NBoxConstants::VIS_PLASTIC_B,
        NBoxConstants::VIS_PLASTIC_ALPHA));

    fVisAttributes.aluminum = new G4VisAttributes(G4Colour(
        NBoxConstants::VIS_ALUMINUM_R,
        NBoxConstants::VIS_ALUMINUM_G,
        NBoxConstants::VIS_ALUMINUM_B,
        NBoxConstants::VIS_ALUMINUM_ALPHA));

    fVisAttributes.he3Gas = new G4VisAttributes(G4Colour(
        NBoxConstants::VIS_HE3_R,
        NBoxConstants::VIS_HE3_G,
        NBoxConstants::VIS_HE3_B,
        NBoxConstants::VIS_HE3_ALPHA));

    fVisAttributes.beamPipe = new G4VisAttributes(G4Colour(
        NBoxConstants::VIS_BEAMPIPE_R,
        NBoxConstants::VIS_BEAMPIPE_G,
        NBoxConstants::VIS_BEAMPIPE_B,
        NBoxConstants::VIS_BEAMPIPE_ALPHA));
}

G4LogicalVolume* DetectorConstruction::ConstructWorld()
{
    G4double worldSize = NBoxConstants::WORLD_SIZE;

    auto* worldS = new G4Box("World", worldSize/2, worldSize/2, worldSize/2);
    auto* worldLV = new G4LogicalVolume(worldS, fMaterials.air, "World");
    worldLV->SetVisAttributes(G4VisAttributes::GetInvisible());

    return worldLV;
}

G4LogicalVolume* DetectorConstruction::ConstructModeratorBox(G4LogicalVolume* worldLV)
{
    auto* config = ConfigManager::GetInstance();

    G4double boxX = config->GetBoxX() * mm;
    G4double boxY = config->GetBoxY() * mm;
    G4double boxZ = config->GetBoxZ() * mm;

    G4cout << "Moderator box: " << boxX/mm << " x " << boxY/mm << " x " << boxZ/mm << " mm³" << G4endl;

    auto* boxS = new G4Box("ModeratorBox", boxX/2, boxY/2, boxZ/2);
    auto* boxLV = new G4LogicalVolume(boxS, fMaterials.plastic, "ModeratorBox_LV");

    new G4PVPlacement(nullptr, {}, boxLV, "ModeratorBox", worldLV, false, 0);
    boxLV->SetVisAttributes(fVisAttributes.plastic);

    return boxLV;
}

void DetectorConstruction::ConstructBeamPipe(G4LogicalVolume* moderatorLV, G4double boxZ)
{
    auto* config = ConfigManager::GetInstance();

    if (!config->HasBeamPipe()) {
        return;
    }

    G4double radius = config->GetBeamPipeDiameter() / 2.0 * mm;
    G4cout << "Beam pipe: diameter = " << config->GetBeamPipeDiameter() << " mm (vacuum)" << G4endl;

    auto* beamPipeS = new G4Tubs("BeamPipe", 0, radius, boxZ/2, 0, 360*deg);
    auto* beamPipeLV = new G4LogicalVolume(beamPipeS, fMaterials.vacuum, "BeamPipe_LV");

    new G4PVPlacement(nullptr, {}, beamPipeLV, "BeamPipe", moderatorLV, false, 0);
    beamPipeLV->SetVisAttributes(fVisAttributes.beamPipe);
}

G4Material* DetectorConstruction::GetOrCreateHe3Gas(const G4String& detectorType, G4double pressure)
{
    auto it = fHe3MaterialCache.find(detectorType);
    if (it != fHe3MaterialCache.end()) {
        return it->second;
    }

    // Create new He3 gas material
    G4double kPa = NBoxConstants::KPA_TO_PASCAL * hep_pascal;
    G4double temperature = NBoxConstants::ROOM_TEMPERATURE;
    G4double molarMass = NBoxConstants::HE3_MOLAR_MASS * g/mole;
    G4double gasConstant = NBoxConstants::GAS_CONSTANT * joule/(mole*kelvin);

    G4double he3Pressure = pressure * kPa;
    G4double he3Density = (he3Pressure * molarMass) / (gasConstant * temperature);

    G4String matName = G4String("He3Gas_") + detectorType;
    auto* he3Gas = new G4Material(matName, he3Density, 1, kStateGas, temperature, he3Pressure);
    he3Gas->AddElement(fMaterials.he3Element, 1.0);

    fHe3MaterialCache[detectorType] = he3Gas;
    return he3Gas;
}

void DetectorConstruction::ConstructHe3Detectors(G4LogicalVolume* moderatorLV)
{
    auto* config = ConfigManager::GetInstance();
    G4int numPlacements = config->GetNumPlacements();

    G4cout << "Building " << numPlacements << " He3 detector tubes..." << G4endl;

    fHe3TubeLVs.clear();
    fHe3TubeLVs.reserve(numPlacements);

    for (G4int i = 0; i < numPlacements; i++) {
        const auto& placement = config->GetPlacement(i);

        if (!config->HasDetectorType(placement.type)) {
            G4cerr << "ERROR: Detector type '" << placement.type << "' not found!" << G4endl;
            continue;
        }

        const auto& detConfig = config->GetDetectorConfig(placement.type);

        // Tube dimensions
        G4double outerRadius = detConfig.diameter / 2.0 * mm;
        G4double length = detConfig.length * mm;
        G4double wallThickness = detConfig.wallT * mm;
        G4double innerRadius = outerRadius - wallThickness;

        // Get or create He3 gas material
        G4Material* he3Gas = GetOrCreateHe3Gas(placement.type, detConfig.pressure);

        // Position from (R, Phi)
        G4double R = placement.R * mm;
        G4double phi = placement.Phi * deg;
        G4ThreeVector position(R * std::cos(phi), R * std::sin(phi), 0);

        // Create aluminum tube
        G4String alName = G4String("AlTube_") + placement.name.c_str();
        auto* alTubeS = new G4Tubs(alName, 0, outerRadius, length/2, 0, 360*deg);
        auto* alTubeLV = new G4LogicalVolume(alTubeS, fMaterials.aluminum, alName + "_LV");

        new G4PVPlacement(nullptr, position, alTubeLV, alName, moderatorLV, false, i);
        alTubeLV->SetVisAttributes(fVisAttributes.aluminum);

        // Create He3 gas volume inside
        G4String he3Name = G4String("He3Gas_") + placement.name.c_str();
        auto* he3GasS = new G4Tubs(he3Name, 0, innerRadius, length/2, 0, 360*deg);
        auto* he3GasLV = new G4LogicalVolume(he3GasS, he3Gas, he3Name + "_LV");

        new G4PVPlacement(nullptr, {}, he3GasLV, he3Name, alTubeLV, false, i);
        he3GasLV->SetVisAttributes(fVisAttributes.he3Gas);

        fHe3TubeLVs.push_back(he3GasLV);
    }

    G4cout << "Created " << fHe3TubeLVs.size() << " He3 detector tubes." << G4endl;
}

G4VPhysicalVolume* DetectorConstruction::Construct()
{
    auto* config = ConfigManager::GetInstance();

    // Initialize materials and visualization
    DefineMaterials();
    CreateVisAttributes();

    // Build geometry hierarchy
    G4LogicalVolume* worldLV = ConstructWorld();
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLV, "World", nullptr, false, 0);

    G4LogicalVolume* moderatorLV = ConstructModeratorBox(worldLV);

    G4double boxZ = config->GetBoxZ() * mm;
    ConstructBeamPipe(moderatorLV, boxZ);

    ConstructHe3Detectors(moderatorLV);

    G4cout << "Geometry construction complete." << G4endl;

    return worldPV;
}

void DetectorConstruction::ConstructSDandField()
{
    auto* config = ConfigManager::GetInstance();

    G4cout << "Assigning sensitive detectors to " << fHe3TubeLVs.size() << " He3 tubes..." << G4endl;

    for (size_t i = 0; i < fHe3TubeLVs.size(); i++) {
        const auto& placement = config->GetPlacement(i);

        G4String sdName = placement.name.c_str();
        G4String hcName = G4String("He3HitsCollection_") + placement.name.c_str();

        auto* sd = new NBoxSD(sdName, hcName, i);
        G4SDManager::GetSDMpointer()->AddNewDetector(sd);
        SetSensitiveDetector(fHe3TubeLVs[i], sd);
    }

    G4cout << "Sensitive detector assignment complete." << G4endl;
}
