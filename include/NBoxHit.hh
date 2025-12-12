#ifndef NBoxHit_h
#define NBoxHit_h

#include "G4VHit.hh"
#include "G4THitsCollection.hh"
#include "G4Allocator.hh"
#include "G4ThreeVector.hh"
#include "G4Threading.hh"

class NBoxHit : public G4VHit
{
public:
    NBoxHit() = default;
    ~NBoxHit() override = default;
    NBoxHit(const NBoxHit&) = default;
    NBoxHit& operator=(const NBoxHit&) = default;

    G4bool operator==(const NBoxHit&) const;

    inline void* operator new(size_t);
    inline void operator delete(void*);

    // Setters
    void SetDetectorName(const G4String& name) { fDetectorName = name; }
    void SetDetectorID(G4int id) { fDetectorID = id; }
    void SetEdep(G4double edep) { fEdep = edep; }
    void AddEdep(G4double edep) { fEdep += edep; }
    void SetPosition(const G4ThreeVector& pos) { fPosition = pos; }
    void SetTime(G4double time) { fTime = time; }

    // Getters
    const G4String& GetDetectorName() const { return fDetectorName; }
    G4int GetDetectorID() const { return fDetectorID; }
    G4double GetEdep() const { return fEdep; }
    const G4ThreeVector& GetPosition() const { return fPosition; }
    G4double GetTime() const { return fTime; }

private:
    G4String fDetectorName;
    G4int fDetectorID = -1;
    G4double fEdep = 0.;
    G4ThreeVector fPosition;
    G4double fTime = 0.;
};

using NBoxHitsCollection = G4THitsCollection<NBoxHit>;

extern G4ThreadLocal G4Allocator<NBoxHit>* NBoxHitAllocator;

inline void* NBoxHit::operator new(size_t)
{
    if (!NBoxHitAllocator) {
        NBoxHitAllocator = new G4Allocator<NBoxHit>;
    }
    return NBoxHitAllocator->MallocSingle();
}

inline void NBoxHit::operator delete(void* hit)
{
    if (NBoxHitAllocator) {
        NBoxHitAllocator->FreeSingle(static_cast<NBoxHit*>(hit));
    }
}

#endif
