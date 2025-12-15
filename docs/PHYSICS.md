# NBox Physics Background

Understanding the physics behind He-3 neutron detection.

## Table of Contents

1. [He-3 Neutron Detection Principle](#he-3-neutron-detection-principle)
2. [Neutron Moderation](#neutron-moderation)
3. [Cross Sections](#cross-sections)
4. [Energy Deposition](#energy-deposition)
5. [Detection Efficiency](#detection-efficiency)
6. [Neutron Sources](#neutron-sources)

---

## He-3 Neutron Detection Principle

### The Nuclear Reaction

He-3 detectors work via the neutron capture reaction:

```
³He + n → ³H + p + 764 keV
```

**Reaction Details:**
- **Target:** ³He (Helium-3 isotope)
- **Projectile:** Thermal neutron (n)
- **Products:**
  - Triton (³H, tritium nucleus)
  - Proton (p)
- **Q-value:** 764 keV (energy released)

### Energy Sharing

The 764 keV is shared between products according to momentum conservation:

| Particle | Energy | Percentage |
|----------|--------|------------|
| Proton (p) | 573 keV | 75% |
| Triton (³H) | 191 keV | 25% |

**Range in He-3 gas:**
- Proton: ~3-5 cm at 4 atm
- Triton: ~1-2 cm at 4 atm

Both particles deposit their energy within the detector volume → Full 764 keV measured.

### Why He-3?

**Advantages:**
1. **High cross-section** at thermal energies (~5330 barns at 0.025 eV)
2. **Noble gas** - Easy to use in proportional counters
3. **Clean signature** - Sharp 764 keV peak
4. **No gamma sensitivity** - Neutron-specific detection
5. **Room temperature** operation

**Disadvantages:**
1. **Expensive** (~$1000-2000/liter at STP)
2. **Limited supply** (byproduct of tritium decay)
3. **Only sensitive to thermal neutrons** - Fast neutrons need moderation

---

## Neutron Moderation

### Why Moderation?

Fast neutrons (MeV range) have **very low** He-3 cross-section.
- Fast (1 MeV): σ ≈ 0.01 barns
- Thermal (0.025 eV): σ ≈ 5330 barns

**Solution:** Slow down (moderate) neutrons before detection.

### Moderation Mechanism

**Elastic scattering** with light nuclei:

```
n + ¹H → n + ¹H   (hydrogen: best moderator)
n + ¹²C → n + ¹²C  (carbon: good moderator)
```

**Energy loss per collision:**

Average fractional energy loss:
```
ξ = 1 - ((A-1)²/(2A)) × ln((A+1)/(A-1))

For hydrogen (A=1):  ξ = 1.00  (loses ~100% energy per collision)
For carbon (A=12):   ξ = 0.158 (loses ~15% energy per collision)
```

**Number of collisions to thermalize:**

From E₀ = 2 MeV to Eth = 0.025 eV:
```
n = ln(E₀/Eth) / ξ

Hydrogen:  ~18 collisions
Carbon:    ~114 collisions
```

### Polyethylene as Moderator

**Material:** (CH₂)ₙ
- **Density:** 0.94 g/cm³
- **Composition:** 14.4% H, 85.6% C by mass
- **Hydrogen density:** ~0.08 g/cm³

**Why polyethylene?**
1. High hydrogen content (best for moderation)
2. Low cost
3. Easy to machine
4. Room temperature stable
5. Low neutron absorption (no strong absorbers)

**Moderation length:**

Neutron slowing-down length in polyethylene:
```
L_s ≈ 3-5 cm for MeV neutrons
```

**Design rule:** Box thickness should be **5-10 cm** around detectors for optimal moderation.

### Thermal Spectrum

After moderation, neutrons reach **thermal equilibrium** with the moderator:

**Maxwell-Boltzmann distribution:**
```
φ(E) ∝ E × exp(-E/kT)

Where:
  k = Boltzmann constant
  T = temperature (typically 293 K = 20°C)
  Most probable energy: kT = 0.025 eV at room temperature
```

---

## Cross Sections

### Energy Dependence

He-3 neutron capture cross-section follows the **1/v law**:

```
σ(E) = σ₀ × √(E₀/E)

Where:
  σ₀ = 5330 barns at E₀ = 0.025 eV (thermal)
```

### Cross Section vs Energy

| Energy | Type | σ(He-3) | Description |
|--------|------|---------|-------------|
| 0.001 eV | Cold | ~26,000 barns | Very high capture |
| 0.025 eV | Thermal | ~5,330 barns | Peak sensitivity |
| 1 eV | Epithermal | ~134 barns | Moderate |
| 1 keV | Slow | ~4 barns | Low |
| 1 MeV | Fast | ~0.01 barns | Very low |

**Visualization:**
```
σ (barns)
  │
10⁴├─╲
    │  ╲
10³ │   ╲___  1/v region
    │       ╲___
10² │           ╲___
    │               ╲___
10¹ │                   ╲___
    │                       ╲___
10⁰ │___________________________╲___
    └────────────────────────────────> E
    0.001 eV   0.1 eV    10 eV   1 MeV
```

### Mean Free Path

Distance neutron travels before capture:

```
λ = 1 / (n × σ)

Where:
  n = He-3 number density
  σ = cross-section
```

**Example:** 4 atm He-3 at thermal energy
```
n = 9.8 × 10¹⁹ atoms/cm³
σ = 5330 × 10⁻²⁴ cm²
λ = 1 / (9.8 × 10¹⁹ × 5330 × 10⁻²⁴) = 1.9 cm
```

**Design implication:** For 50% capture probability, detector diameter should be **~2 cm**.

---

## Energy Deposition

### Full Energy Peak

**Ideal case:** Both p and ³H deposit full energy in detector
```
E_total = E_p + E_t = 573 + 191 = 764 keV
```

**Result:** Sharp peak at 764 keV in energy spectrum.

### Partial Energy Deposition (Wall Effect)

If reaction occurs near detector wall:
- One particle may escape the active volume
- Only partial energy deposited

**Scenarios:**
1. **Full energy:** 764 keV (both particles contained)
2. **Proton only:** ~573 keV (triton escapes)
3. **Triton only:** ~191 keV (proton escapes)
4. **Partial:** Variable (particle partially exits)

**Spectrum shape:**
```
Counts
  │
  │    ┌──┐  ← 764 keV peak
  │    │  │
  │    │  │
  │  ┌─┘  └─┐  ← Wall effect continuum
  │ ┌┘      └─┐
  └─┴─────────┴────────> Energy (keV)
    0   500   764  1000
```

### Wall Effect Minimization

**Strategies:**
1. **Larger diameter** - More volume vs surface
2. **Higher pressure** - Shorter particle ranges
3. **Energy threshold** - Accept only ~600-800 keV (reject wall events)

---

## Detection Efficiency

### Intrinsic Efficiency

**Definition:** Probability that a neutron entering the detector is captured.

```
ε_intrinsic = 1 - exp(-n × σ × L)

Where:
  n = He-3 number density
  σ = capture cross-section
  L = path length through detector
```

### Efficiency Factors

#### 1. Pressure Dependence

| Pressure | Density | λ (thermal) | ε (for D=2.54cm) |
|----------|---------|-------------|------------------|
| 1 atm | 1× | 7.6 cm | ~28% |
| 4 atm | 4× | 1.9 cm | ~75% |
| 10 atm | 10× | 0.76 cm | ~95% |

**Trade-off:** Higher pressure → Better efficiency BUT higher voltage needed.

#### 2. Diameter Dependence

For 4 atm He-3, thermal neutrons:

| Diameter | Efficiency |
|----------|------------|
| 1 inch (2.54 cm) | ~75% |
| 2 inch (5.08 cm) | ~93% |

#### 3. Energy Dependence

For 4 atm, 1-inch tube:

| Neutron Energy | σ (barns) | ε |
|----------------|-----------|---|
| 0.025 eV (thermal) | 5330 | ~75% |
| 1 eV | 134 | ~5% |
| 1 keV | 4 | ~0.15% |
| 1 MeV | 0.01 | ~0.0004% |

**Conclusion:** He-3 detectors are **thermal-neutron specific**.

### System Efficiency

**Total efficiency = Moderation efficiency × Intrinsic efficiency**

```
ε_total = ε_mod × ε_int

Where:
  ε_mod = Fraction of neutrons thermalized
  ε_int = Intrinsic detector efficiency
```

**Typical values:**
- Thermal source: ε_mod ≈ 1.0, ε_int ≈ 0.75 → **ε_total ≈ 75%**
- Fast source (Cf-252): ε_mod ≈ 0.01, ε_int ≈ 0.75 → **ε_total ≈ 0.75%**

---

## Neutron Sources

### Common Sources

#### 1. Thermal Neutron Source

**Energy spectrum:** Maxwell-Boltzmann at 293K
```
Most probable: 0.025 eV
Mean energy: 0.0375 eV
```

**Detection:**
- Very high cross-section
- Minimal moderation needed
- Efficiency: 10-20%

**Applications:** Reactor neutrons, pre-moderated sources

#### 2. Cf-252 (Californium-252)

**Type:** Spontaneous fission
**Energy spectrum:** Watt fission spectrum
```
N(E) = C × exp(-E/a) × sinh(√(b×E))

Parameters:
  a = 1.025 MeV
  b = 2.926 MeV⁻¹
  Mean energy: 2.13 MeV
  Most probable: 0.7 MeV
```

**Detection:**
- Low cross-section (fast neutrons)
- Requires thick moderator
- Efficiency: 0.3-1% (with moderation)

**Applications:** Calibration, portable sources, oil well logging

#### 3. AmBe (Americium-Beryllium)

**Type:** (α,n) reaction
```
²⁴¹Am → α + ...
α + ⁹Be → ¹²C + n
```

**Energy spectrum:**
```
Broad peak around 4-5 MeV
Mean energy: ~4.2 MeV
```

**Detection:**
- Fast neutrons
- Moderate moderation needed
- Efficiency: 0.5-2% (with moderation)

**Applications:** Well logging, non-destructive testing

#### 4. DD Fusion (Deuterium-Deuterium)

**Reaction:**
```
²H + ²H → ³He + n + 3.27 MeV
```

**Energy:** Mono-energetic at **2.45 MeV**

**Applications:** Accelerator-based, fusion research

#### 5. DT Fusion (Deuterium-Tritium)

**Reaction:**
```
²H + ³H → ⁴He + n + 17.6 MeV
```

**Energy:** Mono-energetic at **14.1 MeV**

**Applications:** Fusion plasma diagnostics

### Source Comparison Table

| Source | Type | Energy | Moderation | Detection Efficiency |
|--------|------|--------|------------|----------------------|
| Thermal | Thermalized | 0.025 eV | Minimal | 10-20% |
| Reactor | Mixed | 0.01-10 MeV | Medium | 5-15% |
| Cf-252 | Fission | 0.7 MeV peak | Heavy | 0.3-1% |
| AmBe | (α,n) | 4 MeV peak | Heavy | 0.5-2% |
| DD | Fusion | 2.45 MeV | Medium | 1-3% |
| DT | Fusion | 14.1 MeV | Very heavy | 0.1-0.5% |

---

## Optimization Principles

### Maximize Efficiency

1. **Use thermal or moderated sources** when possible
2. **Optimize moderator thickness:**
   - Too thin: Insufficient moderation
   - Too thick: Neutrons captured in moderator
   - Optimal: 5-10 cm polyethylene for MeV neutrons

3. **Optimize detector pressure:**
   - Higher pressure → Better efficiency
   - Limit: Voltage breakdown, cost

4. **Optimize geometry:**
   - Multiple detectors capture more solid angle
   - Rings at different radii maximize coverage

### Minimize Background

1. **Energy threshold:** Reject events below ~600 keV
2. **Time-of-flight:** Fast events likely gamma-ray interactions
3. **Multiplicity:** Neutrons may scatter to multiple tubes

---

## References

1. **He-3 Neutron Detectors:**
   - NIST: https://www.nist.gov/programs-projects/neutron-detection
   - Knoll, G.F., "Radiation Detection and Measurement"

2. **Cross-section data:**
   - ENDF/B-VIII.0: https://www.nndc.bnl.gov/exfor/
   - JANIS: https://www.oecd-nea.org/janis/

3. **Neutron sources:**
   - ISO 8529: Reference neutron radiations
   - Reijonen et al., "Compact neutron generators for medical, homeland security and planetary exploration"

---

## Next Steps

- **Configure your detector:** [CONFIGURATION.md](CONFIGURATION.md)
- **Analyze your data:** [ANALYSIS.md](ANALYSIS.md)
- **Optimization techniques:** [TIPS.md](TIPS.md)
