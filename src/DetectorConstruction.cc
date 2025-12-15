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
#include "G4VisAttributes.hh"
#include "G4RotationMatrix.hh"

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

G4VPhysicalVolume* DetectorConstruction::Construct()
{
    // Get configuration
    auto* config = ConfigManager::GetInstance();

    G4NistManager* nist = G4NistManager::Instance();

    // ==================== Materials ====================

    // Define unit conversions
    G4double kPa = NBoxConstants::KPA_TO_PASCAL * hep_pascal;

    // 1. He3 Isotope and Element (critical for thermal neutron capture!)
    // IMPORTANT: Must use isotope, not natural helium (which is 99.99986% He4)
    // He3(n,p)H3 has ~5330 barn cross-section at thermal energies
    G4Isotope* He3_isotope = new G4Isotope("He3",
        NBoxConstants::HE3_ATOMIC_NUMBER,
        NBoxConstants::HE3_MASS_NUMBER,
        NBoxConstants::HE3_MOLAR_MASS*g/mole);
    G4Element* He3 = new G4Element("Helium3", "He3", 1);
    He3->AddIsotope(He3_isotope, NBoxConstants::HE3_ISOTOPE_ABUNDANCE*perCent);

    // 2. Plastic Box Material (High-density polyethylene)
    G4Material* plastic = nist->FindOrBuildMaterial("G4_POLYETHYLENE");

    // 3. Aluminum for tube walls
    G4Material* aluminum = nist->FindOrBuildMaterial("G4_Al");

    // 4. Air for world
    G4Material* air = nist->FindOrBuildMaterial("G4_AIR");

    // ==================== World Volume ====================

    // World size should be larger than the plastic box
    G4double worldSize = NBoxConstants::WORLD_SIZE;

    auto* worldS = new G4Box("World", worldSize/2, worldSize/2, worldSize/2);
    auto* worldLV = new G4LogicalVolume(worldS, air, "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLV, "World", nullptr, false, 0);

    // ==================== Plastic Box ====================

    G4double boxX = config->GetBoxX() * mm;
    G4double boxY = config->GetBoxY() * mm;
    G4double boxZ = config->GetBoxZ() * mm;

    G4cout << "Plastic box: " << boxX/mm << " x " << boxY/mm << " x " << boxZ/mm << " mm³" << G4endl;

    auto* plasticBoxS = new G4Box("PlasticBox", boxX/2, boxY/2, boxZ/2);
    auto* plasticBoxLV = new G4LogicalVolume(plasticBoxS, plastic, "PlasticBox_LV");

    new G4PVPlacement(nullptr, {}, plasticBoxLV, "PlasticBox", worldLV, false, 0);

    // Visualization for plastic box
    auto* plasticVis = new G4VisAttributes(G4Colour(
        NBoxConstants::VIS_PLASTIC_R,
        NBoxConstants::VIS_PLASTIC_G,
        NBoxConstants::VIS_PLASTIC_B,
        NBoxConstants::VIS_PLASTIC_ALPHA));
    plasticBoxLV->SetVisAttributes(plasticVis);

    // ==================== He3 Detector Tubes ====================

    G4int numPlacements = config->GetNumPlacements();
    G4cout << "Building " << numPlacements << " He3 detector tubes..." << G4endl;

    fHe3TubeLVs.clear();

    for (G4int i = 0; i < numPlacements; i++) {
        const auto& placement = config->GetPlacement(i);

        // Get detector configuration for this placement
        if (!config->HasDetectorType(placement.type)) {
            G4cerr << "ERROR: Detector type '" << placement.type << "' not found in configuration!" << G4endl;
            continue;
        }

        const auto& detConfig = config->GetDetectorConfig(placement.type);

        // Tube dimensions
        G4double tubeOuterRadius = detConfig.diameter / 2.0 * mm;
        G4double tubeLength = detConfig.length * mm;
        G4double tubeWallThickness = detConfig.wallT * mm;
        G4double tubeInnerRadius = tubeOuterRadius - tubeWallThickness;

        // Create He3 gas material for this tube with specific pressure
        G4double he3Pressure = detConfig.pressure * kPa;
        G4double temperature = NBoxConstants::ROOM_TEMPERATURE;
        G4double molarMass = NBoxConstants::HE3_MOLAR_MASS * g/mole;
        G4double gasConstant = NBoxConstants::GAS_CONSTANT * joule/(mole*kelvin);
        G4double he3Density = (he3Pressure * molarMass) / (gasConstant * temperature);

        G4String he3MatName = G4String("He3Gas_") + placement.name.c_str();
        auto* he3Gas = new G4Material(he3MatName, he3Density, 1, kStateGas, temperature, he3Pressure);
        he3Gas->AddElement(He3, 1.0);

        // Position: convert (R, Phi) to (x, y, z=0)
        G4double R = placement.R * mm;
        G4double phi = placement.Phi * deg;
        G4double x = R * std::cos(phi);
        G4double y = R * std::sin(phi);
        G4double z = 0.0;

        G4ThreeVector position(x, y, z);

        G4cout << "  Detector " << i << " (" << placement.name << ", type=" << placement.type << "): "
               << "R=" << R/mm << " mm, Phi=" << placement.Phi << "°, "
               << "Pos=(" << x/mm << ", " << y/mm << ", " << z/mm << ") mm, "
               << "Diameter=" << detConfig.diameter << " mm, Length=" << detConfig.length << " mm" << G4endl;

        // Create aluminum tube (outer cylinder)
        G4String alTubeName = G4String("AlTube_") + placement.name.c_str();
        G4String alTubeLVName = G4String("AlTube_LV_") + placement.name.c_str();
        auto* alTubeS = new G4Tubs(alTubeName, 0, tubeOuterRadius, tubeLength/2, 0, NBoxConstants::FULL_CIRCLE_DEG*deg);
        auto* alTubeLV = new G4LogicalVolume(alTubeS, aluminum, alTubeLVName);

        // Rotation: tubes are along Z-axis by default, keep them vertical
        new G4PVPlacement(nullptr, position, alTubeLV,
                         alTubeName, plasticBoxLV, false, i);

        // Create He3 gas volume (inner cylinder)
        G4String he3GasName = G4String("He3Gas_") + placement.name.c_str();
        G4String he3GasLVName = G4String("He3Gas_LV_") + placement.name.c_str();
        auto* he3GasS = new G4Tubs(he3GasName, 0, tubeInnerRadius, tubeLength/2, 0, NBoxConstants::FULL_CIRCLE_DEG*deg);
        auto* he3GasLV = new G4LogicalVolume(he3GasS, he3Gas, he3GasLVName);

        // Place He3 gas inside aluminum tube (at center)
        new G4PVPlacement(nullptr, {}, he3GasLV,
                         he3GasName, alTubeLV, false, i);

        // Store He3 gas logical volume for SD assignment
        fHe3TubeLVs.push_back(he3GasLV);

        // Visualization
        auto* alVis = new G4VisAttributes(G4Colour(
            NBoxConstants::VIS_ALUMINUM_R,
            NBoxConstants::VIS_ALUMINUM_G,
            NBoxConstants::VIS_ALUMINUM_B,
            NBoxConstants::VIS_ALUMINUM_ALPHA));
        alTubeLV->SetVisAttributes(alVis);

        auto* he3Vis = new G4VisAttributes(G4Colour(
            NBoxConstants::VIS_HE3_R,
            NBoxConstants::VIS_HE3_G,
            NBoxConstants::VIS_HE3_B,
            NBoxConstants::VIS_HE3_ALPHA));
        he3GasLV->SetVisAttributes(he3Vis);
    }

    // Make world invisible
    worldLV->SetVisAttributes(G4VisAttributes::GetInvisible());

    G4cout << "Geometry construction complete: " << numPlacements << " He3 tubes created." << G4endl;

    return worldPV;
}

void DetectorConstruction::ConstructSDandField()
{
    // MT: Called for each worker thread

    G4cout << "ConstructSDandField: Assigning sensitive detectors to "
           << fHe3TubeLVs.size() << " He3 tubes..." << G4endl;

    auto* config = ConfigManager::GetInstance();

    // Assign unique sensitive detector to each He3 tube
    for (size_t i = 0; i < fHe3TubeLVs.size(); i++) {
        const auto& placement = config->GetPlacement(i);

        // Create unique SD name and hits collection name
        G4String sdName = G4String("He3SD_") + placement.name.c_str();
        G4String hcName = G4String("He3HitsCollection_") + placement.name.c_str();

        // Create SD with detector ID = i
        auto* sd = new NBoxSD(sdName, hcName, i);
        G4SDManager::GetSDMpointer()->AddNewDetector(sd);
        SetSensitiveDetector(fHe3TubeLVs[i], sd);

        G4cout << "  Assigned SD '" << sdName << "' with ID=" << i
               << " to detector '" << placement.name << "'" << G4endl;
    }

    G4cout << "ConstructSDandField complete." << G4endl;
}
