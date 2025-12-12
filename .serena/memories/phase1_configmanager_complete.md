# Phase 1: ConfigManager Implementation - COMPLETE

## Summary
ConfigManager singleton has been successfully implemented and tested with TDD principles.

## Completed Tasks
- ✅ Downloaded nlohmann/json header-only library to `include/json.hpp`
- ✅ Created `include/ConfigManager.hh` with proper data structures
- ✅ Created `src/ConfigManager.cc` with full implementation
- ✅ Updated `CMakeLists.txt` to link ROOT libraries
- ✅ Modified `nbox.cc` to initialize ConfigManager and load config files
- ✅ Created sample configuration files for testing
- ✅ Built project successfully with no errors
- ✅ Tested ConfigManager with all three file types

## Key Design Decisions

### Detector Configuration Structure
Each detector **type** is defined with specifications:
```cpp
struct DetectorConfig {
    std::string name;      // e.g., "He3_Short", "He3_Long"
    double diameter;       // mm
    double length;         // mm
    double wallT;          // wall thickness, mm
    double pressure;       // kPa
};
```

### Detector Placement Structure
Each detector **instance** references a type and has position:
```cpp
struct DetectorPlacement {
    std::string name;      // Unique instance name (e.g., "Det1", "Det2")
    std::string type;      // References DetectorConfig by name
    double R;              // Radial distance from center, mm
    double Phi;            // Azimuthal angle, degrees
};
```

### JSON File Formats

**Detector File** (`sample_detector.json`):
```json
{
  "detectors": [
    {
      "name": "He3_Short",
      "Diameter": 30,
      "Length": 500,
      "WallT": 1,
      "Pressure": 1000
    },
    {
      "name": "He3_Long",
      "Diameter": 50,
      "Length": 1000,
      "WallT": 2,
      "Pressure": 2000
    }
  ]
}
```

**Geometry File** (`sample_geometry.json`):
```json
{
  "Box": {
    "Type": "Box",
    "x": 1000,
    "y": 1000,
    "z": 1000
  },
  "Placements": [
    {
      "name": "Det1",
      "type": "He3_Short",
      "R": 100,
      "Phi": 0
    },
    {
      "name": "Det2",
      "type": "He3_Short",
      "R": 100,
      "Phi": 90
    },
    {
      "name": "Det3",
      "type": "He3_Long",
      "R": 200,
      "Phi": 45
    }
  ]
}
```

**Source File** (ROOT histogram):
- Created via `TODO/create_test_source.C`
- Contains TH1 histogram for neutron energy spectrum
- Sampled during event generation

## ConfigManager API

```cpp
// Singleton access
ConfigManager* config = ConfigManager::GetInstance();

// Load files
config->LoadDetectorFile("detector.json");
config->LoadGeometryFile("geometry.json");
config->LoadSourceFile("source.root");

// Box geometry
double x = config->GetBoxX();
double y = config->GetBoxY();
double z = config->GetBoxZ();

// Detector configurations
int nTypes = config->GetNumDetectorConfigs();
const DetectorConfig& cfg = config->GetDetectorConfig("He3_Short");
bool exists = config->HasDetectorType("He3_Short");

// Placements
int nDets = config->GetNumPlacements();
const DetectorPlacement& pl = config->GetPlacement(0);

// Source
TH1* hist = config->GetSourceHistogram();

// Status
bool loaded = config->IsGeometryLoaded();
config->PrintConfiguration();
```

## Testing Results
Tested with 3 configuration files:
- 2 detector types (He3_Short, He3_Long)
- 1 box geometry (1000x1000x1000 mm)
- 3 detector placements
- 1 neutron energy histogram

Output confirms all data loaded correctly.

## Files Created
- `include/ConfigManager.hh` - Header
- `src/ConfigManager.cc` - Implementation
- `include/json.hpp` - JSON library
- `TODO/sample_detector.json` - Sample detector config
- `TODO/sample_geometry.json` - Sample geometry config
- `TODO/create_test_source.C` - ROOT macro to create test histogram
- `TODO/test_source.root` - Test neutron energy spectrum

## Files Modified
- `nbox.cc` - Added ConfigManager initialization
- `CMakeLists.txt` - Added ROOT library linking

## Next Steps (Phase 2)
- Add Time_ns column to output
- Replace DetectorName (string) with DetectorID (int)
- Modify EventAction to write new columns
