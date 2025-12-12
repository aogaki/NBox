#include "ActionInitialization.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "EventAction.hh"

ActionInitialization::ActionInitialization(const G4String& sourceFile)
    : fSourceFile(sourceFile)
{
    if (!fSourceFile.empty()) {
        G4cout << "ActionInitialization: Source file = " << fSourceFile << G4endl;
    }
}

void ActionInitialization::BuildForMaster() const
{
    SetUserAction(new RunAction());
}

void ActionInitialization::Build() const
{
    SetUserAction(new PrimaryGeneratorAction(fSourceFile));
    SetUserAction(new RunAction());
    SetUserAction(new EventAction());
}
