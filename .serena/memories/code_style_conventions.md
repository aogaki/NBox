# Code Style and Conventions

## Naming Conventions

### Classes
- **PascalCase** for class names
- Example: `DetectorConstruction`, `NBoxHit`, `ConfigManager`

### Member Variables
- **fVariableName** - prefix `f` for class member variables
- Example: `fParticleGun`, `fDetectorName`, `fEdep`

### Constants
- **kConstantName** - prefix `k` for constants (not heavily used in current code)

### Functions/Methods
- **PascalCase** for public methods following Geant4 conventions
- Example: `GetInstance()`, `LoadDetectorFile()`, `GeneratePrimaries()`

### Files
- **Header files**: `.hh` extension
- **Implementation files**: `.cc` extension
- **Naming**: Match class name (e.g., `NBoxHit.hh`, `NBoxHit.cc`)

## Code Structure

### Headers
- Include guards using `#ifndef CLASSNAME_HH` / `#define CLASSNAME_HH` / `#endif`
- Minimal includes - use forward declarations where possible
- Member variables at end of class (private section)

### Implementation Files
- Include corresponding header first
- Then Geant4 headers
- Then standard library headers

### Comments
- Minimal comments - code should be self-documenting
- Comments used for clarification when necessary
- No over-documentation (KISS principle)

## Geant4-Specific Patterns

### Units
- Always use Geant4 units: `keV`, `mm`, `cm`, `ns`, etc.
- Example: `511.0*keV`, `3.0*cm`

### Singleton Pattern
- Used for managers and global configuration
- Static `GetInstance()` method
- Private constructor and deleted copy/assignment

### Threading
- ConfigManager initialized before multi-threading starts
- Read-only after initialization
- Each worker thread has own instances of Action classes

## Design Principles
- **KISS**: Keep It Simple, Stupid - no over-engineering
- **TDD**: Test-Driven Development - write tests first
- **DRY**: Don't Repeat Yourself - use ConfigManager for shared data
- **Single Responsibility**: Each class has one clear purpose
