#ifndef NBOX_CONSTANTS_HH
#define NBOX_CONSTANTS_HH

#include "G4SystemOfUnits.hh"

namespace NBoxConstants {

// Progress reporting
constexpr int PROGRESS_REPORT_INTERVAL = 1000;  // Print progress every N events

// Ntuple column indices
constexpr int NTUPLE_COL_EVENT_ID = 0;
constexpr int NTUPLE_COL_DETECTOR_ID = 1;
constexpr int NTUPLE_COL_DETECTOR_NAME = 2;
constexpr int NTUPLE_COL_EDEP_KEV = 3;
constexpr int NTUPLE_COL_TIME_NS = 4;

// He-3 neutron detection reaction: 3He(n,p)3H
constexpr double HE3_REACTION_Q_VALUE = 764.0 * keV;  // Q-value of the reaction

// Material density adjustment factor
constexpr double MATERIAL_DENSITY_SCALING = 1.0;  // Can be used to scale material densities

// Thermal neutron energy (room temperature, 293K)
constexpr double THERMAL_NEUTRON_ENERGY = 0.025 * eV;

// Default box world size (if not specified in config)
constexpr double DEFAULT_WORLD_SIZE = 3000.0 * mm;

// Minimum energy deposit threshold for recording hits
constexpr double MIN_ENERGY_DEPOSIT = 0.0 * keV;  // Record all hits > 0

// ===== Detector Construction Constants =====

// World volume size (large enough for any configuration)
constexpr double WORLD_SIZE = 5.0 * CLHEP::m;

// Room temperature for gas calculations
constexpr double ROOM_TEMPERATURE = 293.15 * CLHEP::kelvin;

// He-3 isotope properties
constexpr double HE3_ATOMIC_NUMBER = 2;
constexpr double HE3_MASS_NUMBER = 3;
constexpr double HE3_MOLAR_MASS = 3.016029; // g/mole (used with CLHEP units)
constexpr double HE3_ISOTOPE_ABUNDANCE = 100.0; // percent (pure He-3)

// He-3 neutron capture cross-section
constexpr double HE3_THERMAL_CROSS_SECTION = 5330.0;  // barns at thermal energy

// Gas constant for ideal gas law
constexpr double GAS_CONSTANT = 8.314;  // J/(mol·K)

// Pressure conversion
constexpr double KPA_TO_PASCAL = 1000.0;  // 1 kPa = 1000 Pa

// Visualization colors (RGBA)
constexpr double VIS_PLASTIC_R = 0.8;
constexpr double VIS_PLASTIC_G = 0.8;
constexpr double VIS_PLASTIC_B = 0.8;
constexpr double VIS_PLASTIC_ALPHA = 0.3;  // Transparency

constexpr double VIS_ALUMINUM_R = 0.7;
constexpr double VIS_ALUMINUM_G = 0.7;
constexpr double VIS_ALUMINUM_B = 0.7;
constexpr double VIS_ALUMINUM_ALPHA = 0.5;

constexpr double VIS_HE3_R = 0.0;
constexpr double VIS_HE3_G = 1.0;
constexpr double VIS_HE3_B = 0.0;
constexpr double VIS_HE3_ALPHA = 0.3;

// Geometry constants
constexpr double FULL_CIRCLE_DEG = 360.0;  // degrees

}  // namespace NBoxConstants

#endif  // NBOX_CONSTANTS_HH
