#ifndef ActionInitialization_h
#define ActionInitialization_h

#include "G4VUserActionInitialization.hh"
#include "globals.hh"

class ActionInitialization : public G4VUserActionInitialization
{
public:
    ActionInitialization(const G4String& sourceFile = "", G4bool enableFluxMap = false);
    ~ActionInitialization() override = default;

    void BuildForMaster() const override;
    void Build() const override;

private:
    G4String fSourceFile;
    G4bool fEnableFluxMap{false};
};

#endif
