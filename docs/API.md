# NBox API Reference

Developer documentation for extending and modifying NBox.

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [ConfigManager Class](#configmanager-class)
3. [DetectorConstruction Class](#detectorconstruction-class)
4. [Hit and Sensitive Detector](#hit-and-sensitive-detector)
5. [Primary Generator](#primary-generator)
6. [Output Format](#output-format)
7. [Adding New Features](#adding-new-features)

---

## Architecture Overview

### Class Diagram

```
┌─────────────────┐
│  ConfigManager  │ (Singleton)
│  (Loads JSON/   │
│   ROOT files)   │
└────────┬────────┘
         │ Used by
         ├────────────────┐
         ↓                ↓
┌───────────────────┐  ┌──────────────────────┐
│ DetectorConstruction│  │ PrimaryGeneratorAction│
│ (Build geometry)  │  │ (Sample neutron     │
└─────────┬─────────┘  │  energy from source)│
          │            └──────────────────────┘
          ↓
┌─────────────────┐
│    NBoxSD       │ (Sensitive Detector)
│ (Record hits)   │
└────────┬────────┘
         ↓
┌─────────────────┐
│    NBoxHit      │ (Hit data)
└────────┬────────┘
         ↓
┌─────────────────┐
│  ROOT ntuple    │ (Output file)
└─────────────────┘
```

### File Structure

```
NBox/
├── nbox.cc                        # Main program
├── include/
│   ├── ConfigManager.hh           # Singleton config loader
│   ├── DetectorConstruction.hh    # Geometry builder
│   ├── PrimaryGeneratorAction.hh  # Neutron source
│   ├── NBoxSD.hh                  # Sensitive detector
│   ├── NBoxHit.hh                 # Hit data structure
│   ├── ActionInitialization.hh    # Geant4 initialization
│   └── RunAction.hh               # Run management + output
├── src/
│   ├── ConfigManager.cc
│   ├── DetectorConstruction.cc
│   ├── PrimaryGeneratorAction.cc
│   ├── NBoxSD.cc
│   ├── NBoxHit.cc
│   ├── ActionInitialization.cc
│   └── RunAction.cc
└── CMakeLists.txt                 # Build configuration
```

---

## ConfigManager Class

**Purpose:** Singleton class for loading and storing configuration data.

**Header:** [include/ConfigManager.hh](../include/ConfigManager.hh)

### Key Methods

#### Singleton Access

```cpp
ConfigManager* ConfigManager::GetInstance()
```
- **Returns:** Pointer to singleton instance
- **Usage:**
  ```cpp
  ConfigManager* config = ConfigManager::GetInstance();
  ```

#### Loading Configuration Files

```cpp
void LoadDetectorFile(const std::string& filename)
```
- **Purpose:** Load detector descriptions from JSON
- **Parameters:** Path to detector JSON file
- **Throws:** std::runtime_error if file not found or parse error
- **Example:**
  ```cpp
  config->LoadDetectorFile("detector.json");
  ```

```cpp
void LoadGeometryFile(const std::string& filename)
```
- **Purpose:** Load geometry configuration from JSON
- **Parameters:** Path to geometry JSON file
- **Example:**
  ```cpp
  config->LoadGeometryFile("geometry.json");
  ```

```cpp
void LoadSourceFile(const std::string& filename)
```
- **Purpose:** Load neutron source from ROOT file
- **Parameters:** Path to ROOT file containing TH1 or TF1
- **Details:**
  - Validates single source term (only one TH1 or TF1)
  - Clones object for thread safety
  - Pre-computes TF1 CDF table
- **Example:**
  ```cpp
  config->LoadSourceFile("cf252_source.root");
  ```

#### Geometry Access

```cpp
G4double GetBoxX() const
G4double GetBoxY() const
G4double GetBoxZ() const
```
- **Returns:** Box dimensions in mm
- **Usage:**
  ```cpp
  G4double boxX = config->GetBoxX();
  ```

#### Detector Configuration Access

```cpp
int GetNumDetectorConfigs() const
```
- **Returns:** Number of detector types defined

```cpp
const DetectorConfig& GetDetectorConfig(int i) const
```
- **Returns:** Reference to detector configuration
- **Parameters:** Index (0 to GetNumDetectorConfigs()-1)

```cpp
bool HasDetectorType(const std::string& type) const
```
- **Returns:** True if detector type exists
- **Parameters:** Detector type name

```cpp
const DetectorConfig* GetDetectorConfig(const std::string& type) const
```
- **Returns:** Pointer to detector config, or nullptr if not found
- **Parameters:** Detector type name

#### Placement Access

```cpp
int GetNumPlacements() const
```
- **Returns:** Number of detector placements

```cpp
const DetectorPlacement& GetPlacement(int i) const
```
- **Returns:** Reference to detector placement
- **Parameters:** Index (0 to GetNumPlacements()-1)

#### Source Access

```cpp
TH1* GetSourceHistogram() const
```
- **Returns:** Pointer to source histogram, or nullptr if TF1 used

```cpp
TF1* GetSourceFunction() const
```
- **Returns:** Pointer to source function, or nullptr if TH1 used

#### Status Checks

```cpp
bool IsGeometryLoaded() const
bool IsDetectorLoaded() const
bool IsSourceLoaded() const
```
- **Returns:** True if respective configuration loaded

#### Debugging

```cpp
void PrintConfiguration() const
```
- **Effect:** Prints configuration to G4cout

### Data Structures

#### DetectorConfig

```cpp
struct DetectorConfig {
    std::string name;    // Detector type name
    double diameter;     // Outer diameter (mm)
    double length;       // Tube length (mm)
    double wallT;        // Wall thickness (mm)
    double pressure;     // He3 pressure (kPa)
};
```

#### DetectorPlacement

```cpp
struct DetectorPlacement {
    std::string name;    // Instance name
    std::string type;    // References DetectorConfig::name
    double R;            // Radial position (mm)
    double Phi;          // Azimuthal angle (degrees)
};
```

### Example Usage

```cpp
// In your code
ConfigManager* config = ConfigManager::GetInstance();

// Load configuration
config->LoadDetectorFile("detector.json");
config->LoadGeometryFile("geometry.json");
config->LoadSourceFile("source.root");

// Access data
G4double boxSize = config->GetBoxX();
int nPlacements = config->GetNumPlacements();

for (int i = 0; i < nPlacements; i++) {
    const DetectorPlacement& place = config->GetPlacement(i);
    const DetectorConfig* det = config->GetDetectorConfig(place.type);

    G4cout << "Detector " << place.name << " of type " << place.type
           << " at R=" << place.R << ", Phi=" << place.Phi << G4endl;
    G4cout << "  Diameter=" << det->diameter << " mm" << G4endl;
}
```

---

## DetectorConstruction Class

**Purpose:** Construct Geant4 geometry from configuration.

**Header:** [include/DetectorConstruction.hh](../include/DetectorConstruction.hh)

### Key Methods

```cpp
G4VPhysicalVolume* Construct() override
```
- **Purpose:** Build geometry (called by Geant4)
- **Returns:** Pointer to world physical volume
- **Details:**
  - Creates world volume
  - Creates polyethylene box
  - Creates He3 detector tubes
  - Places detectors according to configuration

```cpp
void ConstructSDandField() override
```
- **Purpose:** Attach sensitive detectors (called by Geant4)
- **Details:**
  - Creates NBoxSD instance
  - Attaches to all He3 tube logical volumes

### Geometry Construction Process

1. **World volume:** Air-filled 2m × 2m × 2m box
2. **Moderator box:** Polyethylene, dimensions from ConfigManager
3. **For each placement:**
   - Get detector configuration
   - Create He3 gas volume (cylinder)
   - Create aluminum wall (tube)
   - Calculate position from R, Phi
   - Place in moderator box
   - Assign unique copy number (detector ID)

### Materials

**Defined in Construct():**
- `Air`: G4_AIR (world)
- `Polyethylene`: G4_POLYETHYLENE (moderator box)
- `He3Gas`: Custom material (pressure-dependent density)
- `Aluminum`: G4_Al (tube walls)

#### He3 Density Calculation

```cpp
// Pressure in kPa → density in g/cm³
double P_atm = pressure / 101.325;
double density = 0.000166 * P_atm;  // g/cm³
```

### Example: Adding Custom Material

```cpp
// In DetectorConstruction::Construct()

// Define new material
G4Material* myMaterial = new G4Material(
    "MyMaterial",           // name
    density,                // density
    nComponents             // number of elements
);

myMaterial->AddElement(G4NistManager::Instance()->FindOrBuildElement("H"), 2);
myMaterial->AddElement(G4NistManager::Instance()->FindOrBuildElement("C"), 1);

// Use in volume
G4LogicalVolume* myLV = new G4LogicalVolume(
    mySolid,
    myMaterial,
    "MyLV"
);
```

---

## Hit and Sensitive Detector

### NBoxHit Class

**Purpose:** Store hit information.

**Header:** [include/NBoxHit.hh](../include/NBoxHit.hh)

#### Methods

```cpp
void SetDetectorName(const G4String& name)
void SetDetectorID(G4int id)
void SetEdep(G4double edep)
void AddEdep(G4double edep)
void SetPosition(const G4ThreeVector& pos)
void SetTime(G4double time)

G4String GetDetectorName() const
G4int GetDetectorID() const
G4double GetEdep() const
G4ThreeVector GetPosition() const
G4double GetTime() const
```

### NBoxSD Class

**Purpose:** Process steps in sensitive volumes and create hits.

**Header:** [include/NBoxSD.hh](../include/NBoxSD.hh)

#### Key Method

```cpp
G4bool ProcessHits(G4Step* step, G4TouchableHistory* history) override
```

**Logic:**
1. Get energy deposition from step
2. If Edep > 0:
   - Create or retrieve hit for this event
   - Get detector ID from copy number
   - Get detector name from touchable
   - Add energy deposition
   - Record position and time

### Example: Custom Processing

```cpp
// In NBoxSD::ProcessHits()

G4double edep = step->GetTotalEnergyDeposit();
if (edep <= 0.) return false;

// Custom: Only record hits above threshold
const G4double threshold = 100 * keV;
if (edep < threshold) return false;

// Get hit
NBoxHit* hit = GetHit(eventID);

// Custom: Record particle type
G4String particleName = step->GetTrack()->GetDefinition()->GetParticleName();
hit->SetParticleName(particleName);

// Add energy
hit->AddEdep(edep);
```

---

## Primary Generator

**Purpose:** Generate primary neutrons with energy from source spectrum.

**Header:** [include/PrimaryGeneratorAction.hh](../include/PrimaryGeneratorAction.hh)

### Key Method

```cpp
void GeneratePrimaries(G4Event* event) override
```

**Logic:**
1. Sample energy from TH1 or TF1
2. Set neutron energy
3. Set random isotropic direction
4. Set position at origin (0, 0, 0)
5. Generate primary vertex

### Example: Custom Source Position

```cpp
// In PrimaryGeneratorAction::GeneratePrimaries()

// Random position within sphere of radius 10 cm
G4double r = 10 * cm * std::pow(G4UniformRand(), 1./3.);
G4double theta = std::acos(2 * G4UniformRand() - 1);
G4double phi = 2 * CLHEP::pi * G4UniformRand();

G4double x = r * std::sin(theta) * std::cos(phi);
G4double y = r * std::sin(theta) * std::sin(phi);
G4double z = r * std::cos(theta);

fParticleGun->SetParticlePosition(G4ThreeVector(x, y, z));
```

---

## Output Format

### ROOT Ntuple Structure

**File:** `output_run0_t<N>.root` (per thread)

**Tree name:** `hits`

**Branches:**

| Name | Type | Description |
|------|------|-------------|
| EventID | Int_t | Event number |
| DetectorID | Int_t | Detector copy number (0, 1, 2, ...) |
| DetectorName | std::string | Detector instance name from JSON |
| Edep_keV | Double_t | Energy deposition in keV |
| Time_ns | Double_t | Global time in nanoseconds |

### RunAction Class

**Purpose:** Manage output file creation and writing.

**Methods:**

```cpp
void BeginOfRunAction(const G4Run* run) override
```
- Creates ROOT file
- Creates ntuple with branches

```cpp
void EndOfRunAction(const G4Run* run) override
```
- Writes ntuple to file
- Closes ROOT file

### Example: Adding New Branch

**In RunAction.hh:**
```cpp
private:
    G4double fNewVariable;
```

**In RunAction::BeginOfRunAction():**
```cpp
fNtuple->Branch("NewVariable", &fNewVariable, "NewVariable/D");
```

**In RunAction::EndOfEventAction():**
```cpp
fNewVariable = someValue;
fNtuple->Fill();
```

---

## Adding New Features

### Example 1: Add Detector Position to Output

**Step 1:** Modify NBoxHit.hh
```cpp
// Add member
private:
    G4ThreeVector fDetectorPosition;

// Add methods
public:
    void SetDetectorPosition(const G4ThreeVector& pos) { fDetectorPosition = pos; }
    G4ThreeVector GetDetectorPosition() const { return fDetectorPosition; }
```

**Step 2:** Modify NBoxSD.cc
```cpp
// In ProcessHits()
G4ThreeVector detPos = step->GetPreStepPoint()->GetTouchableHandle()->GetTranslation();
hit->SetDetectorPosition(detPos);
```

**Step 3:** Modify RunAction
```cpp
// Add branches
fNtuple->Branch("DetX_mm", &fDetX, "DetX_mm/D");
fNtuple->Branch("DetY_mm", &fDetY, "DetY_mm/D");
fNtuple->Branch("DetZ_mm", &fDetZ, "DetZ_mm/D");

// In EndOfEventAction()
G4ThreeVector pos = hit->GetDetectorPosition();
fDetX = pos.x() / mm;
fDetY = pos.y() / mm;
fDetZ = pos.z() / mm;
```

### Example 2: Add New Detector Type

**Step 1:** Extend DetectorConfig struct (if needed)
```cpp
struct DetectorConfig {
    std::string name;
    double diameter;
    double length;
    double wallT;
    double pressure;
    std::string shape;  // NEW: "cylinder" or "box"
};
```

**Step 2:** Modify JSON parsing in ConfigManager.cc
```cpp
config.shape = det.value("shape", "cylinder");  // default = cylinder
```

**Step 3:** Modify DetectorConstruction::Construct()
```cpp
if (det->shape == "box") {
    // Create box detector
    G4Box* solidHe3 = new G4Box(/* ... */);
} else {
    // Create cylinder detector (existing code)
    G4Tubs* solidHe3 = new G4Tubs(/* ... */);
}
```

### Example 3: Custom Physics List

**Current:** Uses QGSP_BIC_HP (high-precision neutrons)

**To change:**

**In nbox.cc:**
```cpp
// Replace
G4VModularPhysicsList* physicsList = new QGSP_BIC_HP;

// With custom list
#include "MyPhysicsList.hh"
G4VModularPhysicsList* physicsList = new MyPhysicsList;
```

**Create MyPhysicsList.hh:**
```cpp
class MyPhysicsList : public G4VModularPhysicsList {
public:
    MyPhysicsList();
    virtual ~MyPhysicsList();
};
```

**Create MyPhysicsList.cc:**
```cpp
#include "MyPhysicsList.hh"
#include "G4EmStandardPhysics.hh"
#include "G4HadronElasticPhysicsHP.hh"
// ... include needed physics

MyPhysicsList::MyPhysicsList() {
    RegisterPhysics(new G4EmStandardPhysics());
    RegisterPhysics(new G4HadronElasticPhysicsHP());
    // ... register other physics
}
```

---

## Code Style Guidelines

### Naming Conventions

- **Classes:** PascalCase (e.g., `ConfigManager`, `NBoxHit`)
- **Methods:** PascalCase (e.g., `GetBoxX()`, `ProcessHits()`)
- **Member variables:** fVariableName (prefix `f`, e.g., `fBoxX`, `fDetectorID`)
- **Local variables:** camelCase (e.g., `boxSize`, `detectorName`)
- **Constants:** UPPER_CASE or kPascalCase (e.g., `kMaxDetectors`)

### File Naming

- **Headers:** `.hh`
- **Implementation:** `.cc`
- **Match class name:** `ConfigManager.hh`, `ConfigManager.cc`

### Comments

```cpp
// Single-line comment for brief explanations

/**
 * Multi-line comment for class/method documentation
 * @param eventID Event identifier
 * @return Pointer to hit object
 */
```

---

## Build System

### CMakeLists.txt

**Key sections:**

```cmake
# Geant4 setup
find_package(Geant4 REQUIRED ui_all vis_all)
include(${Geant4_USE_FILE})

# ROOT setup
find_package(ROOT REQUIRED)
include_directories(${ROOT_INCLUDE_DIRS})

# JSON library
include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/...)

# Executable
add_executable(nbox_sim nbox.cc ${sources})
target_link_libraries(nbox_sim ${Geant4_LIBRARIES} ${ROOT_LIBRARIES} nlohmann_json::nlohmann_json)
```

### Adding New Source Files

```cmake
# In CMakeLists.txt
set(sources
    ${PROJECT_SOURCE_DIR}/src/ConfigManager.cc
    ${PROJECT_SOURCE_DIR}/src/DetectorConstruction.cc
    # ... existing sources ...
    ${PROJECT_SOURCE_DIR}/src/MyNewClass.cc  # ADD THIS
)
```

### Rebuild

```bash
cd build
cmake ..
cmake --build . -j$(nproc)
```

---

## Debugging Tips

### Enable Debug Output

```cpp
// In your code
G4cout << "Debug: variable = " << variable << G4endl;
```

### Use Geant4 Verbosity

```cpp
// In macro
/run/verbose 2
/event/verbose 1
/tracking/verbose 1
```

### GDB Debugging

```bash
# Build with debug symbols
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .

# Run with GDB
gdb --args ./nbox_sim -m run.mac
(gdb) break DetectorConstruction::Construct
(gdb) run
(gdb) print variable
```

---

## References

- **Geant4 Documentation:** https://geant4.web.cern.ch/
- **ROOT Documentation:** https://root.cern/doc/
- **JSON Library:** https://github.com/nlohmann/json

---

## Next Steps

- **Understand physics:** [PHYSICS.md](PHYSICS.md)
- **Configure simulations:** [CONFIGURATION.md](CONFIGURATION.md)
- **Optimize performance:** [TIPS.md](TIPS.md)
