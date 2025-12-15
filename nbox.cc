#include "G4RunManagerFactory.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"
#include "G4PhysListFactory.hh"
#include "G4ThermalNeutrons.hh"

#include "DetectorConstruction.hh"
#include "ActionInitialization.hh"
#include "ConfigManager.hh"

#include <getopt.h>
#include <iostream>

void PrintUsage()
{
    G4cout << "Usage: nbox_sim [options]" << G4endl;
    G4cout << "Options:" << G4endl;
    G4cout << "  -m <file>  Macro file (default: run.mac)" << G4endl;
    G4cout << "  -g <file>  Geometry file (JSON format)" << G4endl;
    G4cout << "  -d <file>  Detector description file (JSON format)" << G4endl;
    G4cout << "  -s <file>  Source term file (ROOT format)" << G4endl;
    G4cout << "  -h         Show this help message" << G4endl;
    G4cout << G4endl;
    G4cout << "If no options are specified, interactive mode with visualization will start." << G4endl;
}

int main(int argc, char** argv)
{
    // Parse command-line arguments
    G4String macroFile = "";
    G4String geometryFile = "";
    G4String detectorFile = "";
    G4String sourceFile = "";

    int opt;
    while ((opt = getopt(argc, argv, "m:g:d:s:h")) != -1) {
        switch (opt) {
            case 'm':
                macroFile = optarg;
                break;
            case 'g':
                geometryFile = optarg;
                break;
            case 'd':
                detectorFile = optarg;
                break;
            case 's':
                sourceFile = optarg;
                break;
            case 'h':
                PrintUsage();
                return 0;
            default:
                PrintUsage();
                return 1;
        }
    }

    // Interactive mode if no macro file is specified
    G4bool interactiveMode = macroFile.empty();

    // Print configuration
    G4cout << "========== NBox Configuration ==========" << G4endl;
    if (!macroFile.empty()) {
        G4cout << "Macro file: " << macroFile << G4endl;
    }
    if (!geometryFile.empty()) {
        G4cout << "Geometry file: " << geometryFile << G4endl;
    }
    if (!detectorFile.empty()) {
        G4cout << "Detector description file: " << detectorFile << G4endl;
    }
    if (!sourceFile.empty()) {
        G4cout << "Source term file: " << sourceFile << G4endl;
    }
    G4cout << "========================================" << G4endl;

    // Initialize ConfigManager singleton
    auto* config = ConfigManager::GetInstance();

    try {
        if (!geometryFile.empty()) {
            config->LoadGeometryFile(geometryFile);
        }
        if (!detectorFile.empty()) {
            config->LoadDetectorFile(detectorFile);
        }
        if (!sourceFile.empty()) {
            config->LoadSourceFile(sourceFile);
        }
    } catch (const std::exception& e) {
        G4cerr << "Error loading configuration: " << e.what() << G4endl;
        return 1;
    }

    // Print configuration summary
    config->PrintConfiguration();

    // Create UI executive for interactive mode
    G4UIExecutive* ui = nullptr;
    if (interactiveMode) {
        ui = new G4UIExecutive(argc, argv);
    }

    auto* runManager = G4RunManagerFactory::CreateRunManager(
        G4RunManagerType::Default);

    // Use all available CPU cores
    G4int nThreads = G4Threading::G4GetNumberOfCores();
    runManager->SetNumberOfThreads(nThreads);
    G4cout << "Running with " << nThreads << " threads (all available cores)" << G4endl;

    runManager->SetUserInitialization(new DetectorConstruction(geometryFile, detectorFile));

    // Use QGSP_BIC_HP physics list with thermal neutron support
    G4PhysListFactory factory;
    G4VModularPhysicsList* phys = factory.GetReferencePhysList("QGSP_BIC_HP");
    phys->RegisterPhysics(new G4ThermalNeutrons());
    runManager->SetUserInitialization(phys);
    G4cout << "Physics: QGSP_BIC_HP + G4ThermalNeutrons (for He3 capture)" << G4endl;

    runManager->SetUserInitialization(new ActionInitialization(sourceFile));

    G4VisManager* visManager = new G4VisExecutive();
    visManager->Initialize();

    G4UImanager* UImanager = G4UImanager::GetUIpointer();

    if (ui) {
        // Interactive mode with visualization
        UImanager->ApplyCommand("/control/execute init_vis.mac");
        ui->SessionStart();
        delete ui;
    } else {
        // Batch mode
        UImanager->ApplyCommand("/control/execute " + macroFile);
    }

    delete visManager;
    delete runManager;
    return 0;
}
