#ifndef DetectorConstruction_h
#define DetectorConstruction_h

#include "G4VUserDetectorConstruction.hh"
#include "G4LogicalVolume.hh"
#include "globals.hh"

class DetectorConstruction : public G4VUserDetectorConstruction
{
public:
    DetectorConstruction(const G4String& geometryFile = "", const G4String& detectorFile = "");
    ~DetectorConstruction() override = default;

    G4VPhysicalVolume* Construct() override;
    void ConstructSDandField() override;

private:
    G4LogicalVolume* fNBox_LV = nullptr;
    G4String fGeometryFile;
    G4String fDetectorFile;
};

#endif
