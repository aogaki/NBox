#include "ActionInitialization.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "EventAction.hh"
#include "FluxSteppingAction.hh"

ActionInitialization::ActionInitialization(const G4String& sourceFile, G4bool enableFluxMap)
    : fSourceFile(sourceFile), fEnableFluxMap(enableFluxMap)
{
    if (!fSourceFile.empty()) {
        G4cout << "ActionInitialization: Source file = " << fSourceFile << G4endl;
    }
    if (fEnableFluxMap) {
        G4cout << "ActionInitialization: Flux map recording enabled" << G4endl;
    }
}

void ActionInitialization::BuildForMaster() const
{
    SetUserAction(new RunAction(fEnableFluxMap));
}

void ActionInitialization::Build() const
{
    SetUserAction(new PrimaryGeneratorAction(fSourceFile));
    SetUserAction(new RunAction(fEnableFluxMap));
    SetUserAction(new EventAction());

    // Add SteppingAction for flux map recording
    if (fEnableFluxMap) {
        FluxSteppingAction::SetEnabled(true);
        SetUserAction(new FluxSteppingAction());
    }
}
