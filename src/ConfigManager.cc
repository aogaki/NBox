#include "ConfigManager.hh"
#include "json.hpp"
#include "globals.hh"
#include "TFile.h"
#include "TH1.h"
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

    // Get first histogram in file
    TIter next(file->GetListOfKeys());
    TKey* key = nullptr;
    TH1* hist = nullptr;

    while ((key = (TKey*)next())) {
        TClass* cl = gROOT->GetClass(key->GetClassName());
        if (!cl->InheritsFrom("TH1")) continue;

        hist = (TH1*)key->ReadObj();
        if (hist) break;
    }

    if (!hist) {
        file->Close();
        delete file;
        throw std::runtime_error("No TH1 histogram found in source file: " + filepath);
    }

    // Clone histogram to avoid dependency on file
    fSourceHist = (TH1*)hist->Clone("source_histogram");
    fSourceHist->SetDirectory(nullptr);

    file->Close();
    delete file;

    fSourceLoaded = true;
    G4cout << "Loaded source histogram: " << fSourceHist->GetName() << G4endl;
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
        G4cout << "\nSource Histogram: " << fSourceHist->GetName() << G4endl;
    }

    G4cout << "==========================================" << G4endl;
}
