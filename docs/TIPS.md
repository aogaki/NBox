# NBox Tips and Techniques

Optimization strategies, best practices, and troubleshooting for NBox simulations.

## Table of Contents

1. [Performance Optimization](#performance-optimization)
2. [Simulation Strategies](#simulation-strategies)
3. [Geometry Optimization](#geometry-optimization)
4. [Data Analysis Tips](#data-analysis-tips)
5. [Debugging Techniques](#debugging-techniques)
6. [Common Pitfalls](#common-pitfalls)
7. [Advanced Techniques](#advanced-techniques)

---

## Performance Optimization

### Thread Configuration

**Finding optimal thread count:**

```bash
# Get CPU core count
nproc  # Linux
sysctl -n hw.ncpu  # macOS
```

**Best practices:**
- **Physical cores:** Use actual cores, not hyperthreads
- **Leave headroom:** Use N-1 or N-2 threads to keep system responsive
- **Monitor temperature:** If CPU throttles, reduce threads

**Example thread configurations:**

| CPU Cores | Recommended Threads | Reason |
|-----------|---------------------|--------|
| 4 | 3-4 | Full utilization |
| 8 | 6-7 | Leave room for OS |
| 14+ | 12-14 | Avoid oversubscription |

**Macro setting:**
```
/run/numberOfThreads 12  # Adjust based on your CPU
```

### Event Count Optimization

**Staged approach:**

1. **Debug run:** 100-1000 events
   - Verify geometry loads
   - Check output format
   - Time: <1 minute

2. **Test run:** 10,000-100,000 events
   - Initial efficiency estimate
   - Identify issues
   - Time: 5-30 minutes

3. **Production run:** 1,000,000+ events
   - Final statistics
   - Publication quality
   - Time: 1-10 hours

**Statistical uncertainty:**
```
σ = √N  (Poisson statistics)

For 1% uncertainty: Need N = 10,000 counts
For 0.1% uncertainty: Need N = 1,000,000 counts
```

**Rule of thumb:** If detector efficiency is ~1%, need 1M source neutrons for 10k detections.

### Memory Management

**Per-thread memory:**
- Each thread creates separate output file
- Monitor disk space: `df -h`
- Typical size: ~1-10 MB per 100k events

**Cleanup old runs:**
```bash
# Remove old output files
rm output_run0_t*.root

# Or move to archive
mkdir archive
mv output_run0_t*.root archive/
```

### Build Optimization

**Release vs Debug build:**

```bash
# Release build (faster, ~2-3x speedup)
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(nproc)

# Debug build (slower, better error messages)
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . -j$(nproc)
```

**When to use:**
- **Release:** Production runs
- **Debug:** Development, troubleshooting

---

## Simulation Strategies

### Iterative Design Process

**Workflow:**
```
1. Start simple (4 detectors, small box)
   ↓
2. Run quick test (1000 events)
   ↓
3. Visualize geometry (check placement)
   ↓
4. Analyze results (basic efficiency)
   ↓
5. Adjust parameters
   ↓
6. Scale up (more detectors, more events)
   ↓
7. Final optimization
```

### Parameter Sweeps

**Example: Optimize moderator thickness**

```bash
# Create series of geometries
for size in 400 500 600 700 800; do
    # Modify geometry JSON (box size)
    sed "s/\"x\": 600/\"x\": $size/" geometry_base.json > geometry_${size}.json

    # Run simulation
    ./build/nbox_sim -g geometry_${size}.json -d detector.json -s source.root -m run.mac

    # Rename output
    mv output_run0_t0.root output_box${size}_t0.root
done

# Compare results
root -l -q "compare_boxes.C"
```

**Variables to sweep:**
- Moderator box size
- Detector radial positions
- Detector pressure
- Detector diameter
- Number of rings

### Variance Reduction

**Not applicable to NBox** - Standard Geant4 analog Monte Carlo.

**Instead, use:**
1. **More events:** Most direct approach
2. **Optimize geometry:** Focus on high-efficiency regions
3. **Multiple runs:** Average results, estimate error

---

## Geometry Optimization

### Moderator Box Sizing

**Rule of thumb:**
```
Box dimension = 2 × R_max + 2 × L_mod + margin

Where:
  R_max = maximum detector radius
  L_mod = moderation length (~5 cm for polyethylene)
  margin = safety margin (2-5 cm)
```

**Example:**
```
R_max = 155 mm (outer ring)
L_mod = 50 mm
margin = 30 mm
Box size = 2×155 + 2×50 + 2×30 = 570 mm → Use 600 mm
```

**Testing box size:**
```bash
# Too small: Neutrons escape before thermalization
# → Low efficiency

# Too large: Extra weight, cost, no benefit
# → Diminishing returns

# Optimal: Balance efficiency vs size
# → Test range: R_max + 5cm to R_max + 15cm
```

### Detector Placement Strategies

#### Maximize Coverage

**Principle:** Cover maximum solid angle from center.

**Single ring:**
```
N tubes, uniform spacing:
Δφ = 360° / N
```

**Multiple rings:**
```
Inner ring: Fewer tubes (small circumference)
Outer ring: More tubes (large circumference)

Example (28 tubes):
  R=59mm:  4 tubes (Δφ = 90°)
  R=130mm: 8 tubes (Δφ = 45°)
  R=155mm: 16 tubes (Δφ = 22.5°)
```

#### Avoid Overlap

**Minimum angular spacing:**
```
Δφ_min = 2 × arcsin(D / (2R))

For D=25.4mm, R=100mm:
Δφ_min = 14.6° → Use 20-30° for safety
```

**Check for overlaps:**
```bash
# Visualize in interactive mode
./build/nbox_sim -g geometry.json -d detector.json -s source.root

# Rotate view, look for intersecting tubes
# If overlap → Increase Δφ or decrease R
```

### Detector Specifications

**Pressure selection:**

| Application | Pressure | Efficiency | Cost | Voltage |
|-------------|----------|------------|------|---------|
| Low-cost | 1-2 atm | ~30-50% | $ | Low |
| Standard | 4 atm | ~75% | $$ | Medium |
| High-efficiency | 10 atm | ~95% | $$$ | High |

**Diameter selection:**

| Application | Diameter | Efficiency | Weight | Cost |
|-------------|----------|------------|--------|------|
| Compact | 1 inch | ~75% | Light | $ |
| Standard | 1.5 inch | ~85% | Medium | $$ |
| High-efficiency | 2 inch | ~93% | Heavy | $$$ |

**Length selection:**
```
Match box z-dimension:
If box z = 1000mm → Use tube length ~900-950mm
Leave 50-100mm clearance
```

---

## Data Analysis Tips

### Energy Threshold Selection

**Challenge:** Wall effects create low-energy background.

**Solution:** Apply energy cut to accept only full-energy events.

```cpp
// Accept events in 764 keV peak
hits->Draw("Edep_keV", "Edep_keV > 600 && Edep_keV < 900")

// Typical thresholds:
// Loose: >500 keV (high efficiency, some background)
// Standard: >600 keV (balanced)
// Tight: >700 keV (low background, lower efficiency)
```

**Determine optimal threshold:**
```cpp
// Plot efficiency vs threshold
void efficiency_scan() {
    TFile* f = TFile::Open("output.root");
    TTree* hits = (TTree*)f->Get("hits");

    int nPoints = 50;
    double threshold[50], efficiency[50];

    for (int i = 0; i < nPoints; i++) {
        threshold[i] = i * 20;  // 0 to 1000 keV
        TString cut = TString::Format("Edep_keV > %.0f", threshold[i]);
        Long64_t count = hits->GetEntries(cut.Data());
        efficiency[i] = (double)count / total_neutrons * 100;
    }

    TGraph* gr = new TGraph(nPoints, threshold, efficiency);
    gr->Draw("ALP");

    // Look for plateau around 600-700 keV
}
```

### Statistical Error Estimation

```cpp
// Binomial error for efficiency
double efficiency = detected / emitted;
double error = sqrt(efficiency * (1 - efficiency) / emitted);

// Example:
// 1000 detected out of 100,000 emitted
// ε = 1000/100000 = 1%
// σ = sqrt(0.01 × 0.99 / 100000) = 0.03% (absolute)
//   = 3% (relative)
```

### Handling Multiple Hits per Event

**Scenario:** One neutron detected by multiple tubes (scatter).

**Analysis approaches:**

1. **Count all hits** (default)
   - Simple
   - Overestimates efficiency for single-neutron events

2. **Count unique events**
   ```cpp
   // Get unique EventID count
   hits->Draw("EventID", "", "goff");
   Int_t nUnique = hits->GetSelectedRows();

   // OR use std::set
   std::set<int> uniqueEvents;
   for (Long64_t i = 0; i < hits->GetEntries(); i++) {
       hits->GetEntry(i);
       uniqueEvents.insert(eventID);
   }
   cout << "Unique events: " << uniqueEvents.size() << endl;
   ```

3. **Count first hit only**
   ```cpp
   // Find earliest hit per event
   std::map<int, double> firstHitTime;
   for (Long64_t i = 0; i < hits->GetEntries(); i++) {
       hits->GetEntry(i);
       if (firstHitTime.find(eventID) == firstHitTime.end()) {
           firstHitTime[eventID] = time_ns;
       }
   }
   ```

---

## Debugging Techniques

### Geometry Verification

**Always visualize before production runs:**

```bash
# Interactive mode
./build/nbox_sim -g geometry.json -d detector.json -s source.root

# Check:
# ✓ Tubes inside box
# ✓ No overlaps
# ✓ Reasonable spacing
# ✓ Correct orientation (parallel to z)
```

**Geant4 UI commands:**
```
/vis/viewer/zoom 1.5          # Zoom in
/vis/viewer/set/viewpointVector 1 1 1  # Change angle
/run/beamOn 10                # Show some tracks
/vis/viewer/refresh           # Refresh view
```

### Verbosity for Debugging

**Macro for debugging:**
```
# debug.mac
/run/numberOfThreads 1
/run/verbose 2
/event/verbose 1
/tracking/verbose 0  # 1 for track details (very verbose!)

/run/initialize
/run/beamOn 10
```

**Output interpretation:**
```
Event: 0          # Event number
Track: 1          # Primary neutron
  ...
Track: 2          # Secondary particle (p or ³H)
  ...
```

### Check Output Files

```bash
# List ROOT file contents
root -l output_run0_t0.root
# ROOT> .ls

# Quick check
rootls -t output_run0_t0.root

# Verify tree structure
root -l output_run0_t0.root
# ROOT> hits->Print()
# ROOT> hits->Scan("*", "", "", 10)
```

### Common Error Messages

**"G4Exception: Invalid placement"**
- **Cause:** Detector outside box or overlapping
- **Fix:** Check geometry JSON, visualize

**"Segmentation fault"**
- **Cause:** Memory corruption, invalid pointer
- **Fix:** Run with debug build, check recent code changes

**"Cannot open source file"**
- **Cause:** Missing or incorrect path to source.root
- **Fix:** Verify file exists with `ls -lh source.root`

**"Multiple source terms found"**
- **Cause:** Both TH1 and TF1 in source file
- **Fix:** Keep only one object type

---

## Common Pitfalls

### ❌ Tube Longer than Box

**Problem:**
```json
{"Box": {"z": 800}}
{"Detector": {"Length": 1000}}
```

**Solution:**
```json
{"Box": {"z": 1100}}  # Or reduce tube length to 750mm
```

### ❌ Insufficient Moderation

**Problem:** Fast neutrons, thin box → Low efficiency

**Symptoms:**
- Very few hits (<0.1% efficiency with MeV source)
- No 764 keV peak

**Solution:**
- Increase box thickness (test 500, 600, 700mm)
- Use thermal source for testing

### ❌ Wrong Energy Units

**Problem:** Creating source with wrong energy scale

**Correct:**
```cpp
// ROOT TH1 or TF1: Energy in MeV
TH1D* h = new TH1D("neutron", "Spectrum", 100, 0, 10);  // 0-10 MeV
```

**Wrong:**
```cpp
TH1D* h = new TH1D("neutron", "Spectrum", 100, 0, 10000);  // keV ❌
```

### ❌ Forgetting to Initialize

**Problem:** Macro without `/run/initialize`

**Wrong:**
```
/run/beamOn 1000  # ERROR: Geometry not initialized
```

**Correct:**
```
/run/numberOfThreads 4
/run/initialize    # Must initialize first
/run/beamOn 1000
```

### ❌ Thread File Confusion

**Problem:** Analyzing only one thread file

**Wrong:**
```bash
# Only thread 0 - missing 90% of data!
root -l output_run0_t0.root
```

**Correct:**
```bash
# Merge all threads OR use TChain
TChain* chain = new TChain("hits");
chain->Add("output_run0_t*.root");
chain->Draw("Edep_keV");
```

---

## Advanced Techniques

### Custom Analysis Scripts

**Template for analysis macro:**

```cpp
void analyze_custom(const char* filename) {
    // Open file
    TFile* f = TFile::Open(filename);
    TTree* hits = (TTree*)f->Get("hits");

    // Set branch addresses
    Int_t eventID, detectorID;
    Double_t edep_keV, time_ns;

    hits->SetBranchAddress("EventID", &eventID);
    hits->SetBranchAddress("DetectorID", &detectorID);
    hits->SetBranchAddress("Edep_keV", &edep_keV);
    hits->SetBranchAddress("Time_ns", &time_ns);

    // Create output histograms
    TH1F* h_energy = new TH1F("h_energy", "Energy;keV;Counts", 200, 0, 1000);

    // Loop over entries
    Long64_t nEntries = hits->GetEntries();
    for (Long64_t i = 0; i < nEntries; i++) {
        hits->GetEntry(i);

        // Apply cuts
        if (edep_keV < 600) continue;  // Energy threshold
        if (time_ns > 1000) continue;  // Time window

        // Fill histograms
        h_energy->Fill(edep_keV);

        // Custom analysis
        // ...
    }

    // Save results
    TFile* fout = TFile::Open("analysis_output.root", "RECREATE");
    h_energy->Write();
    fout->Close();

    f->Close();
}
```

### Automated Batch Processing

**Bash script for parameter sweep:**

```bash
#!/bin/bash
# sweep_pressure.sh

for pressure in 100 200 400 600 1000; do
    echo "Running pressure = ${pressure} kPa"

    # Create detector JSON
    cat > detector_p${pressure}.json <<EOF
{
  "detectors": [{
    "name": "He3_Test",
    "Diameter": 25.4,
    "Length": 1000,
    "WallT": 0.8,
    "Pressure": ${pressure}
  }]
}
EOF

    # Run simulation
    ./build/nbox_sim \
        -d detector_p${pressure}.json \
        -g geometry.json \
        -s source.root \
        -m run.mac

    # Rename output
    mv output_run0_t0.root output_p${pressure}_t0.root

    echo "Done: pressure = ${pressure}"
done

# Analyze all
root -l -b -q 'compare_pressure.C'
```

### ROOT Macro Automation

**Run ROOT scripts in batch:**

```bash
# No GUI, exit when done
root -l -b -q 'analyze.C("output.root")'

# Multiple files
for file in output_*.root; do
    root -l -b -q "analyze.C(\"$file\")"
done
```

### Version Control for Configurations

```bash
# Track geometry changes with git
git add geometry.json detector.json
git commit -m "Optimize detector spacing for ELIGANT-TN"

# Tag important versions
git tag -a v1.0-baseline -m "Initial ELIGANT-TN configuration"
git tag -a v2.0-optimized -m "Optimized ring spacing"

# Compare versions
git diff v1.0-baseline v2.0-optimized -- geometry.json
```

---

## Performance Benchmarks

### Typical Timing (14-core CPU)

| Events | Threads | Time | Events/sec |
|--------|---------|------|------------|
| 1,000 | 1 | 30s | 33 |
| 1,000 | 14 | 5s | 200 |
| 100,000 | 14 | 8min | 208 |
| 1,000,000 | 14 | 80min | 208 |

**Scaling:** Nearly linear up to physical core count.

### Disk Usage

| Events | File Size (per thread) |
|--------|------------------------|
| 1,000 | ~100 KB |
| 100,000 | ~10 MB |
| 1,000,000 | ~100 MB |

**Total:** N_threads × file_size

---

## Quick Checklist

### Before Running Simulation

- ✅ Geometry visualized (no overlaps)
- ✅ Source file created and validated
- ✅ Detector types match in both JSONs
- ✅ Box size appropriate for detectors
- ✅ Thread count set correctly
- ✅ Enough disk space

### After Simulation

- ✅ All thread files created
- ✅ No error messages in log
- ✅ Output files not empty
- ✅ Energy spectrum shows 764 keV peak
- ✅ Hit distribution reasonable

### For Production Runs

- ✅ Release build used
- ✅ Sufficient statistics (1M+ events)
- ✅ Multiple runs for error estimate
- ✅ Results documented
- ✅ Configuration files versioned

---

## Next Steps

- **Understand physics:** [PHYSICS.md](PHYSICS.md)
- **Configure detector:** [CONFIGURATION.md](CONFIGURATION.md)
- **Analyze results:** [ANALYSIS.md](ANALYSIS.md)
- **API documentation:** [API.md](API.md)
