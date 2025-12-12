#ifndef CONFIGMANAGER_HH
#define CONFIGMANAGER_HH

#include <string>
#include <vector>

class TH1;

// Individual detector configuration
struct DetectorConfig {
    std::string name;
    double diameter;  // mm
    double length;    // mm
    double wallT;     // wall thickness, mm
    double pressure;  // kPa
};

// Detector instance (placement + identity)
struct DetectorPlacement {
    std::string name;  // Unique detector instance name (e.g., "Det1", "Det2")
    std::string type;  // Detector type (e.g., "He3_Short") - references DetectorConfig
    double R;          // Radial distance from center, mm
    double Phi;        // Azimuthal angle, degrees
};

class ConfigManager {
public:
    static ConfigManager* GetInstance();

    // Load configuration files
    void LoadDetectorFile(const std::string& filepath);
    void LoadGeometryFile(const std::string& filepath);
    void LoadSourceFile(const std::string& filepath);

    // Box geometry accessors
    double GetBoxX() const { return fBoxX; }
    double GetBoxY() const { return fBoxY; }
    double GetBoxZ() const { return fBoxZ; }

    // Detector configuration accessors
    int GetNumDetectorConfigs() const { return fDetectorConfigs.size(); }
    const DetectorConfig& GetDetectorConfig(const std::string& typeName) const;
    bool HasDetectorType(const std::string& typeName) const;

    // Placement accessors
    int GetNumPlacements() const { return fPlacements.size(); }
    const DetectorPlacement& GetPlacement(int index) const;

    // Source accessors
    TH1* GetSourceHistogram() const { return fSourceHist; }

    // Validation
    bool IsGeometryLoaded() const { return fGeometryLoaded; }
    bool IsDetectorLoaded() const { return fDetectorLoaded; }
    bool IsSourceLoaded() const { return fSourceLoaded; }

    void PrintConfiguration() const;

private:
    ConfigManager() = default;
    ~ConfigManager();
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    static ConfigManager* fInstance;

    // Box geometry
    double fBoxX = 0;
    double fBoxY = 0;
    double fBoxZ = 0;

    // Detector configurations (can have multiple types)
    // Stored in a vector, accessed by name lookup
    std::vector<DetectorConfig> fDetectorConfigs;

    // Detector placements (references configs by index)
    std::vector<DetectorPlacement> fPlacements;

    // Source histogram
    TH1* fSourceHist = nullptr;

    // Load status flags
    bool fGeometryLoaded = false;
    bool fDetectorLoaded = false;
    bool fSourceLoaded = false;
};

#endif
