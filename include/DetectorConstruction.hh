#ifndef DetectorConstruction_h
#define DetectorConstruction_h

#include "G4VUserDetectorConstruction.hh"
#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "G4Element.hh"
#include "G4VisAttributes.hh"
#include "globals.hh"

#include <map>

class DetectorConstruction : public G4VUserDetectorConstruction
{
public:
    DetectorConstruction(const G4String& geometryFile = "", const G4String& detectorFile = "");
    ~DetectorConstruction() override = default;

    G4VPhysicalVolume* Construct() override;
    void ConstructSDandField() override;

private:
    // Materials used in construction
    struct Materials {
        G4Element* he3Element = nullptr;
        G4Material* plastic = nullptr;
        G4Material* aluminum = nullptr;
        G4Material* air = nullptr;
        G4Material* vacuum = nullptr;
    };

    // Visualization attributes (shared across volumes)
    struct VisAttributes {
        G4VisAttributes* plastic = nullptr;
        G4VisAttributes* aluminum = nullptr;
        G4VisAttributes* he3Gas = nullptr;
        G4VisAttributes* beamPipe = nullptr;
    };

    // Helper methods
    void DefineMaterials();
    void CreateVisAttributes();
    G4LogicalVolume* ConstructWorld();
    G4LogicalVolume* ConstructModeratorBox(G4LogicalVolume* worldLV);
    void ConstructBeamPipe(G4LogicalVolume* moderatorLV, G4double boxZ);
    void ConstructHe3Detectors(G4LogicalVolume* moderatorLV);
    G4Material* GetOrCreateHe3Gas(const G4String& detectorType, G4double pressure);

    // Member data
    Materials fMaterials;
    VisAttributes fVisAttributes;
    std::map<G4String, G4Material*> fHe3MaterialCache;
    std::vector<G4LogicalVolume*> fHe3TubeLVs;

    G4String fGeometryFile;
    G4String fDetectorFile;
};

#endif
