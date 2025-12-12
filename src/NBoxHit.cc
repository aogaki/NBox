#include "NBoxHit.hh"

G4ThreadLocal G4Allocator<NBoxHit>* NBoxHitAllocator = nullptr;

G4bool NBoxHit::operator==(const NBoxHit& right) const
{
    return (this == &right);
}
