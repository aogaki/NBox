#ifndef FluxSteppingAction_h
#define FluxSteppingAction_h

#include "G4UserSteppingAction.hh"
#include "globals.hh"

/// SteppingAction for recording thermal neutron flux in the moderator
/// Records position and energy of neutrons below thermal energy threshold

class FluxSteppingAction : public G4UserSteppingAction
{
public:
    FluxSteppingAction();
    ~FluxSteppingAction() override = default;

    void UserSteppingAction(const G4Step* step) override;

    // Enable/disable flux recording
    static void SetEnabled(G4bool enabled) { fEnabled = enabled; }
    static G4bool IsEnabled() { return fEnabled; }

private:
    // Thermal neutron energy threshold (0.5 eV)
    static constexpr G4double kThermalEnergyCut = 0.5; // eV

    // Global enable flag (set via macro command or compile flag)
    static G4bool fEnabled;
};

#endif
