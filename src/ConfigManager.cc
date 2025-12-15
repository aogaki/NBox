#include "ConfigManager.hh"
#include "json.hpp"
#include "globals.hh"
#include "TFile.h"
#include "TH1.h"
#include "TF1.h"
#include "TKey.h"
#include "TROOT.h"
#include "TClass.h"

#include <fstream>
#include <stdexcept>

using json = nlohmann::json;

ConfigManager* ConfigManager::fInstance = nullptr;

ConfigManager* ConfigManager::GetInstance()
{
    if (fInstance == nullptr) {
        fInstance = new ConfigManager();
    }
    return fInstance;
}

ConfigManager::~ConfigManager()
{
    if (fSourceHist) {
        delete fSourceHist;
        fSourceHist = nullptr;
    }
    if (fSourceFunc) {
        delete fSourceFunc;
        fSourceFunc = nullptr;
    }
}

void ConfigManager::Reset()
{
    // Clear all configuration data
    fDetectorConfigs.clear();
    fPlacements.clear();

    // Clean up ROOT objects
    if (fSourceHist) {
        delete fSourceHist;
        fSourceHist = nullptr;
    }
    if (fSourceFunc) {
        delete fSourceFunc;
        fSourceFunc = nullptr;
    }

    // Reset all flags
    fGeometryLoaded = false;
    fDetectorLoaded = false;
    fSourceLoaded = false;

    // Reset box geometry
    fBoxX = 0;
    fBoxY = 0;
    fBoxZ = 0;
}

void ConfigManager::LoadDetectorFile(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open detector file: " + filepath);
    }

    json data = json::parse(file);

    // Clear existing configurations
    fDetectorConfigs.clear();

    // Parse detector configurations array
    if (!data.contains("detectors")) {
        throw std::runtime_error("Detector file missing 'detectors' array");
    }

    for (const auto& det : data["detectors"]) {
        DetectorConfig config;
        config.name = det["name"];
        config.diameter = det["Diameter"];
        config.length = det["Length"];
        config.wallT = det["WallT"];
        config.pressure = det["Pressure"];
        fDetectorConfigs.push_back(config);
    }

    fDetectorLoaded = true;
    G4cout << "Loaded " << fDetectorConfigs.size() << " detector configurations" << G4endl;
}

void ConfigManager::LoadGeometryFile(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open geometry file: " + filepath);
    }

    json data = json::parse(file);

    // Parse box dimensions
    if (!data.contains("Box")) {
        throw std::runtime_error("Geometry file missing 'Box' section");
    }

    const auto& box = data["Box"];
    fBoxX = box["x"];
    fBoxY = box["y"];
    fBoxZ = box["z"];

    // Parse detector placements
    if (!data.contains("Placements")) {
        throw std::runtime_error("Geometry file missing 'Placements' array");
    }

    fPlacements.clear();
    for (const auto& placement : data["Placements"]) {
        DetectorPlacement pl;
        pl.name = placement["name"];
        pl.type = placement["type"];
        pl.R = placement["R"];
        pl.Phi = placement["Phi"];
        fPlacements.push_back(pl);
    }

    fGeometryLoaded = true;
    G4cout << "Loaded box geometry (" << fBoxX << ", " << fBoxY << ", " << fBoxZ << ") mm" << G4endl;
    G4cout << "Loaded " << fPlacements.size() << " detector placements" << G4endl;
}

void ConfigManager::LoadSourceFile(const std::string& filepath)
{
    TFile* file = TFile::Open(filepath.c_str(), "READ");
    if (!file || file->IsZombie()) {
        throw std::runtime_error("Cannot open source file: " + filepath);
    }

    // Search for TH1 and TF1 objects in the file
    TIter next(file->GetListOfKeys());
    TKey* key = nullptr;
    TH1* hist = nullptr;
    TF1* func = nullptr;
    int sourceCount = 0;

    while ((key = (TKey*)next())) {
        TClass* cl = gROOT->GetClass(key->GetClassName());

        // Check for TH1 (histogram)
        if (cl->InheritsFrom("TH1")) {
            hist = (TH1*)key->ReadObj();
            if (hist) sourceCount++;
        }
        // Check for TF1 (function)
        else if (cl->InheritsFrom("TF1")) {
            func = (TF1*)key->ReadObj();
            if (func) sourceCount++;
        }
    }

    // Validate: exactly one source term should exist
    if (sourceCount == 0) {
        file->Close();
        delete file;
        throw std::runtime_error("No TH1 histogram or TF1 function found in source file: " + filepath);
    }

    if (sourceCount > 1) {
        file->Close();
        delete file;
        throw std::runtime_error("Multiple source terms found in file. Please use only one source term (TH1 or TF1) in a file: " + filepath);
    }

    // Load the source term (either histogram or function)
    if (hist) {
        // Clone histogram to avoid dependency on file
        fSourceHist = (TH1*)hist->Clone("source_histogram");
        fSourceHist->SetDirectory(nullptr);
        G4cout << "Loaded source histogram: " << fSourceHist->GetName() << G4endl;
    }
    else if (func) {
        // Clone function to avoid dependency on file
        fSourceFunc = (TF1*)func->Clone("source_function");

        // Pre-compute CDF table for thread-safe GetRandom() calls
        // This must be done before multi-threading starts
        fSourceFunc->GetRandom();  // First call creates the CDF table

        G4cout << "Loaded source function: " << fSourceFunc->GetName()
               << " [" << fSourceFunc->GetXmin() << ", " << fSourceFunc->GetXmax() << "] MeV" << G4endl;
        G4cout << "  (CDF table pre-computed for thread-safe sampling)" << G4endl;
    }

    file->Close();
    delete file;

    fSourceLoaded = true;
}

const DetectorConfig& ConfigManager::GetDetectorConfig(const std::string& typeName) const
{
    for (const auto& config : fDetectorConfigs) {
        if (config.name == typeName) {
            return config;
        }
    }
    throw std::runtime_error("Detector type not found: " + typeName);
}

bool ConfigManager::HasDetectorType(const std::string& typeName) const
{
    for (const auto& config : fDetectorConfigs) {
        if (config.name == typeName) {
            return true;
        }
    }
    return false;
}

const DetectorPlacement& ConfigManager::GetPlacement(int index) const
{
    if (index < 0 || index >= (int)fPlacements.size()) {
        throw std::out_of_range("Placement index out of range");
    }
    return fPlacements[index];
}

void ConfigManager::ValidateConfiguration() const
{
    // Check that detector configurations are loaded if geometry is loaded
    if (fGeometryLoaded && !fDetectorLoaded) {
        throw std::runtime_error("Geometry is loaded but detector configurations are missing. Please provide detector file with -d option.");
    }

    // Check that all placements reference valid detector types
    if (fGeometryLoaded && fDetectorLoaded) {
        for (const auto& pl : fPlacements) {
            if (!HasDetectorType(pl.type)) {
                throw std::runtime_error("Placement '" + pl.name + "' references unknown detector type '" + pl.type + "'");
            }
        }
    }
}

void ConfigManager::PrintConfiguration() const
{
    G4cout << "========== ConfigManager Status ==========" << G4endl;
    G4cout << "Detector configs loaded: " << (fDetectorLoaded ? "YES" : "NO") << G4endl;
    G4cout << "Geometry loaded: " << (fGeometryLoaded ? "YES" : "NO") << G4endl;
    G4cout << "Source loaded: " << (fSourceLoaded ? "YES" : "NO") << G4endl;

    if (fDetectorLoaded) {
        G4cout << "\nDetector Configurations:" << G4endl;
        for (const auto& det : fDetectorConfigs) {
            G4cout << "  - " << det.name
                   << ": D=" << det.diameter << "mm"
                   << ", L=" << det.length << "mm"
                   << ", Wall=" << det.wallT << "mm"
                   << ", P=" << det.pressure << "kPa" << G4endl;
        }
    }

    if (fGeometryLoaded) {
        G4cout << "\nBox Geometry: (" << fBoxX << ", " << fBoxY << ", " << fBoxZ << ") mm" << G4endl;
        G4cout << "Detector Placements:" << G4endl;
        for (const auto& pl : fPlacements) {
            G4cout << "  - " << pl.name
                   << " (type: " << pl.type << ")"
                   << " at R=" << pl.R << "mm"
                   << ", Phi=" << pl.Phi << "°" << G4endl;
        }
    }

    if (fSourceLoaded) {
        if (fSourceHist) {
            G4cout << "\nSource: TH1 Histogram - " << fSourceHist->GetName() << G4endl;
        }
        if (fSourceFunc) {
            G4cout << "\nSource: TF1 Function - " << fSourceFunc->GetName()
                   << " [" << fSourceFunc->GetXmin() << ", " << fSourceFunc->GetXmax() << "] MeV" << G4endl;
        }
    }

    G4cout << "==========================================" << G4endl;
}
