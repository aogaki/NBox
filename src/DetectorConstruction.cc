#include "DetectorConstruction.hh"
#include "NBoxSD.hh"

#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4SDManager.hh"
#include "G4VisAttributes.hh"

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
    G4NistManager* nist = G4NistManager::Instance();

    // NBox材料定義 (LYSO: Lu2(1-x)Y2xSiO5:Ce)
    G4Element* Lu = nist->FindOrBuildElement("Lu");
    G4Element* Y  = nist->FindOrBuildElement("Y");
    G4Element* Si = nist->FindOrBuildElement("Si");
    G4Element* O  = nist->FindOrBuildElement("O");
    G4Element* Ce = nist->FindOrBuildElement("Ce");

    auto* nboxMaterial = new G4Material("NBox", 7.1*g/cm3, 5);
    nboxMaterial->AddElement(Lu, 71.43*perCent);
    nboxMaterial->AddElement(Y,   4.03*perCent);
    nboxMaterial->AddElement(Si,  6.37*perCent);
    nboxMaterial->AddElement(O,  18.14*perCent);
    nboxMaterial->AddElement(Ce,  0.03*perCent);

    // World
    G4Material* air = nist->FindOrBuildMaterial("G4_AIR");
    G4double worldSize = 10.0*cm;

    auto* worldS = new G4Box("World", worldSize/2, worldSize/2, worldSize/2);
    auto* worldLV = new G4LogicalVolume(worldS, air, "World");
    auto* worldPV = new G4PVPlacement(nullptr, {}, worldLV, "World", nullptr, false, 0);

    // NBox Crystal (3x3x20 mm³)
    G4double nboxX = 3.0*mm;
    G4double nboxY = 3.0*mm;
    G4double nboxZ = 20.0*mm;

    auto* nboxS = new G4Box("NBox", nboxX/2, nboxY/2, nboxZ/2);
    fNBox_LV = new G4LogicalVolume(nboxS, nboxMaterial, "NBox_LV");

    new G4PVPlacement(nullptr, {}, fNBox_LV, "NBox", worldLV, false, 0);

    // Visualization
    auto* nboxVis = new G4VisAttributes(G4Colour(0.0, 0.8, 1.0, 0.6));
    nboxVis->SetForceSolid(true);
    fNBox_LV->SetVisAttributes(nboxVis);
    worldLV->SetVisAttributes(G4VisAttributes::GetInvisible());

    return worldPV;
}

void DetectorConstruction::ConstructSDandField()
{
    // MT: 各ワーカースレッドで呼ばれる
    // Phase 3: Assign detector ID (currently using 0 for single LYSO test detector)
    auto* sd = new NBoxSD("NBox_SD", "NBoxHitsCollection", 0);
    G4SDManager::GetSDMpointer()->AddNewDetector(sd);
    SetSensitiveDetector(fNBox_LV, sd);
}
