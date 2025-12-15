# NBox ELIGANT-TN Simulation Tutorial

A step-by-step guide for new students to run neutron detector simulations.

## Prerequisites

This tutorial assumes you are using **Ubuntu Linux** or **macOS**. You will need to install:
- **Git** (for downloading the code)
- **Geant4** (version 11.x with multi-threading support)
- **ROOT** (version 6.x or later)
- **CMake** (version 3.16 or later)
- **C++ compiler** (supporting C++11 or later)

**Don't have these installed?** See [Appendix A: Installing Prerequisites](#appendix-a-installing-prerequisites) at the end of this tutorial.

## Table of Contents
0. [Downloading the Code](#0-downloading-the-code)
1. [Building the Project](#1-building-the-project)
2. [Creating the Neutron Source](#2-creating-the-neutron-source)
3. [Understanding the Configuration Files](#3-understanding-the-configuration-files)
4. [Running Interactive Mode (Visualization)](#4-running-interactive-mode-visualization)
5. [Running Batch Simulation](#5-running-batch-simulation)
6. [Analyzing the Results](#6-analyzing-the-results)

---

## 0. Downloading the Code

### Step 0.1: Choose your working directory
Open a terminal and navigate to where you want to store the project:
```bash
cd ~/Documents  # or any directory you prefer
```

### Step 0.2: Clone the repository from GitHub
```bash
git clone https://github.com/aogaki/NBox.git
```

**What this does:** Downloads the complete NBox project from GitHub to your computer.

**Expected output:**
```
Cloning into 'NBox'...
remote: Enumerating objects: 150, done.
remote: Counting objects: 100% (150/150), done.
remote: Compressing objects: 100% (95/95), done.
remote: Total 150 (delta 45), reused 120 (delta 30)
Receiving objects: 100% (150/150), 2.5 MiB | 1.2 MiB/s, done.
Resolving deltas: 100% (45/45), done.
```

### Step 0.3: Enter the project directory
```bash
cd NBox
```

### Step 0.4: Verify the download
Check that all necessary files are present:
```bash
ls -l
```

You should see:
- `CMakeLists.txt` - Build configuration
- `nbox.cc` - Main program
- `include/` - Header files
- `src/` - Source code files
- `example/` - Example configurations and this tutorial
- `README.md` - Project documentation

### Step 0.5: Check the current branch
```bash
git branch
```

You should see:
```
* main
```

### Optional: Update to the latest version

If you've already cloned the repository and want to get the latest updates:
```bash
git pull origin main
```

**What this does:** Downloads and merges the latest changes from GitHub.

---

## 1. Building the Project

### Step 1.1: Ensure you're in the project directory
If you just finished Step 0, you're already in the NBox directory. Otherwise:
```bash
cd /path/to/NBox
```

### Step 1.2: Create and enter the build directory
```bash
mkdir -p build
cd build
```

### Step 1.3: Configure the project with CMake
```bash
cmake ..
```

**What this does:** CMake reads the `CMakeLists.txt` file and generates build files for your system.

### Step 1.4: Build the executable
```bash
cmake --build . -j$(nproc)
```
On macOS, use:
```bash
cmake --build . -j$(sysctl -n hw.ncpu)
```

**What this does:** Compiles all source files and creates the `nbox_sim` executable.

**Expected output:** You should see compilation progress and finally:
```
[100%] Built target nbox_sim
```

### Step 1.5: Return to the project root
```bash
cd ..
```

---

## 2. Creating the Neutron Source

The simulation needs a neutron energy spectrum. We'll create a Cf-252 (Californium-252) neutron source.

### Step 2.1: Open ROOT
```bash
root
```

### Step 2.2: Load and run the source creation script
```ROOT
.L example/create_cf252_source.C
create_cf252_source()
```

**What this does:** Creates a ROOT file (`cf252_source.root`) containing the Cf-252 neutron energy spectrum as a mathematical function (TF1).

**Expected output:**
- A plot window showing the Cf-252 spectrum (0-20 MeV)
- A file named `cf252_source.root` in your directory

### Step 2.3: Exit ROOT
```ROOT
.q
```

### Step 2.4: Verify the source file was created
```bash
ls -lh cf252_source.root
```

You should see a file of approximately 3-4 KB.

---

## 3. Understanding the Configuration Files

The simulation uses three configuration files:

### 3.1: Detector Configuration (`eligant_tn_detector.json`)

Defines the He-3 tube specifications:
- **Diameter:** 25.4 mm (1 inch standard tube)
- **Length:** 1000 mm (matches the box length)
- **Wall thickness:** 0.8 mm (aluminum)
- **Pressure:** 405.3 kPa (~4 atmospheres of He-3 gas)

```json
{
  "detectors": [
    {
      "name": "He3_ELIGANT",
      "Diameter": 25.4,
      "Length": 1000,
      "WallT": 0.8,
      "Pressure": 405.3
    }
  ]
}
```

### 3.2: Geometry Configuration (`eligant_tn_geometry.json`)

Defines the detector layout:
- **Box:** 660 × 660 × 1000 mm³ polyethylene moderator
- **Detectors:** 28 He-3 tubes in 3 concentric circles:
  - **A ring:** 4 tubes at R=59mm
  - **B ring:** 8 tubes at R=130mm
  - **C ring:** 16 tubes at R=155mm

Each detector has:
- `name`: Unique identifier (A1-A4, B1-B8, C1-C16)
- `type`: References detector type from detector config
- `R`: Radial distance from center (mm)
- `Phi`: Azimuthal angle (degrees)

### 3.3: Macro File (`eligant_tn_test.mac`)

Defines simulation parameters:
```
/run/numberOfThreads 14    # Use 14 CPU cores
/run/initialize            # Initialize the geometry
/run/beamOn 1000000        # Simulate 1 million events
```

---

## 4. Running Interactive Mode (Visualization)

Interactive mode lets you visualize the geometry before running simulations.

### Step 4.1: Copy example files to project root (if not already there)
```bash
cp example/eligant_tn_detector.json .
cp example/eligant_tn_geometry.json .
```

### Step 4.2: Launch interactive mode
```bash
./build/nbox_sim -d eligant_tn_detector.json -g eligant_tn_geometry.json -s cf252_source.root
```

**What this does:**
- Loads the detector configuration
- Loads the geometry
- Loads the neutron source
- Opens a 3D visualization window

### Step 4.3: Explore the geometry

You'll see an OpenGL window with the detector geometry. Use these controls:

**Mouse controls:**
- **Left-click + drag:** Rotate the view
- **Right-click + drag:** Zoom in/out
- **Middle-click + drag:** Pan the view

**Geant4 UI commands** (type in the command window):
```
/run/beamOn 10             # Simulate 10 events and visualize tracks
/vis/viewer/refresh        # Refresh the display
/vis/viewer/zoom 1.5       # Zoom in by 1.5x
```

### Step 4.4: Exit interactive mode
Click the "Exit" button or type:
```
exit
```

---

## 5. Running Batch Simulation

Batch mode runs large simulations without visualization.

### Step 5.1: Copy the macro file (if not already in root)
```bash
cp example/eligant_tn_test.mac .
```

### Step 5.2: Edit the macro file for your CPU
```bash
nano eligant_tn_test.mac
```

Change the number of threads to match your CPU cores:
```
/run/numberOfThreads 14    # Change 14 to your core count
```

Find your core count:
- **Linux:** `nproc`
- **macOS:** `sysctl -n hw.ncpu`

### Step 5.3: Run the simulation
```bash
./build/nbox_sim -d eligant_tn_detector.json -g eligant_tn_geometry.json -s cf252_source.root -m eligant_tn_test.mac
```

**What this does:**
- Simulates 1 million neutrons from Cf-252 source
- Tracks neutron interactions in the polyethylene box
- Records He-3 detector responses
- Saves results to ROOT files

**Expected output:**
```
========== NBox Configuration ==========
Geometry file: eligant_tn_geometry.json
Detector description file: eligant_tn_detector.json
Source term file: cf252_source.root
========================================
Loaded box geometry (660, 660, 1000) mm
Loaded 28 detector placements
...
Building 28 He3 detector tubes...
...
```

### Step 5.4: Monitor progress

The simulation will show:
- Physics initialization
- Event processing: `Event: 10000`, `Event: 20000`, etc.
- Multi-threaded output from different workers

**Time estimate:**
- 1 million events typically takes 10-30 minutes depending on your CPU

### Step 5.5: Output files

When complete, you'll find output files:
```bash
ls -lh output_run0_t*.root
```

Each thread creates its own output file:
- `output_run0_t0.root` (thread 0)
- `output_run0_t1.root` (thread 1)
- ... and so on

---

## 6. Analyzing the Results

### Step 6.1: Open ROOT
```bash
root
```

### Step 6.2: Open an output file
```ROOT
TFile *f = TFile::Open("output_run0_t0.root")
```

### Step 6.3: List the contents
```ROOT
f->ls()
```

You should see an ntuple (TTree) named `hits`.

### Step 6.4: View the tree structure
```ROOT
hits->Print()
```

**Data columns:**
- `EventID`: Unique event number
- `DetectorID`: Which detector was hit (0-27)
- `Edep_keV`: Energy deposited in keV
- `Time_ns`: Time of the hit in nanoseconds

### Step 6.5: Make a simple histogram
```ROOT
// Energy spectrum in detector 0 (A1)
hits->Draw("Edep_keV", "DetectorID==0")

// Count hits per detector
hits->Draw("DetectorID")

// Time distribution
hits->Draw("Time_ns")

// 2D plot: Energy vs Detector
hits->Draw("DetectorID:Edep_keV", "", "colz")
```

### Step 6.6: Get statistics
```ROOT
// Total number of hits
hits->GetEntries()

// Hits in detector A1 (ID=0)
hits->GetEntries("DetectorID==0")

// Mean energy deposition
hits->Draw("Edep_keV>>h")
h->GetMean()
```

### Step 6.7: Exit ROOT
```ROOT
.q
```

---

## Common Issues and Solutions

### Issue 1: "Command not found: cmake"
**Solution:** Install CMake:
- **Ubuntu/Debian:** `sudo apt-get install cmake`
- **macOS:** `brew install cmake`

### Issue 2: "Geant4 not found"
**Solution:** Set up Geant4 environment:
```bash
source /path/to/geant4/bin/geant4.sh
```

### Issue 3: "Cannot open source file"
**Solution:** Make sure `cf252_source.root` exists:
```bash
ls -lh cf252_source.root
```
If missing, recreate it (see Section 2).

### Issue 4: Visualization doesn't open
**Solution:** Check if OpenGL is available:
```bash
export DISPLAY=:0  # On Linux
```
Or use batch mode instead (Section 5).

### Issue 5: "No hits in output"
**Solution:**
- Check that you're using a neutron source (not gamma)
- Verify detector configuration is loaded correctly
- Try increasing the number of events

---

## Quick Reference Commands

### Build the project:
```bash
cd build && cmake --build . -j$(nproc) && cd ..
```

### Interactive mode:
```bash
./build/nbox_sim -d eligant_tn_detector.json -g eligant_tn_geometry.json -s cf252_source.root
```

### Batch simulation:
```bash
./build/nbox_sim -d eligant_tn_detector.json -g eligant_tn_geometry.json -s cf252_source.root -m eligant_tn_test.mac
```

### Quick analysis:
```bash
root -l output_run0_t0.root
# Then in ROOT: hits->Draw("DetectorID")
```

---

## Next Steps

After completing this tutorial, you can:

1. **Modify detector positions:** Edit `eligant_tn_geometry.json`
2. **Change detector specifications:** Edit `eligant_tn_detector.json`
3. **Try different sources:** Create thermal neutron or other sources
4. **Increase statistics:** Modify `/run/beamOn` in the macro file
5. **Write analysis scripts:** Use ROOT to create custom analysis macros

For more information, see the main `README.md` file.

---

## Getting Help

If you encounter issues:
1. Check the error messages carefully
2. Verify all prerequisite software is installed
3. Make sure environment variables are set (Geant4, ROOT)
4. Ask your supervisor or check the project documentation

**Good luck with your simulations!**

---

## Appendix A: Installing Prerequisites

This appendix provides step-by-step instructions for installing all required software on Ubuntu Linux and macOS.

### A.1: Ubuntu Linux Installation

#### A.1.1: Update System Packages
```bash
sudo apt update
sudo apt upgrade -y
```

#### A.1.2: Install Basic Build Tools
```bash
sudo apt install -y git cmake g++ gcc make
```

**Verify installation:**
```bash
git --version
cmake --version
g++ --version
```

#### A.1.3: Install Geant4 Dependencies
```bash
sudo apt install -y libexpat1-dev libxerces-c-dev libxmu-dev libxi-dev \
    libx11-dev libxpm-dev libxft-dev libglu1-mesa-dev libglew-dev \
    qt5-default libqt5opengl5-dev
```

#### A.1.4: Install ROOT

**Option 1: Using Package Manager (Easier but may be older version)**
```bash
sudo apt install -y root-system libroot-dev
```

**Option 2: From Binary (Recommended - Latest version)**

Download ROOT from: https://root.cern/install/all_releases/

```bash
# Download ROOT (example for ROOT 6.28)
wget https://root.cern/download/root_v6.28.10.Linux-ubuntu22-x86_64-gcc11.3.tar.gz

# Extract
tar -xzf root_v6.28.10.Linux-ubuntu22-x86_64-gcc11.3.tar.gz

# Move to /opt
sudo mv root /opt/root

# Add to environment (add this to ~/.bashrc)
echo 'source /opt/root/bin/thisroot.sh' >> ~/.bashrc
source ~/.bashrc
```

**Verify ROOT installation:**
```bash
root --version
```

#### A.1.5: Install Geant4

**Download Geant4 data files and source:**

```bash
# Create installation directory
mkdir -p ~/geant4
cd ~/geant4

# Download Geant4 11.2 (or latest version)
wget https://geant4-data.web.cern.ch/releases/geant4-v11.2.0.tar.gz

# Extract
tar -xzf geant4-v11.2.0.tar.gz
```

**Build Geant4:**
```bash
# Create build directory
mkdir geant4-v11.2.0-build
cd geant4-v11.2.0-build

# Configure with CMake (enable multi-threading and Qt)
cmake -DCMAKE_INSTALL_PREFIX=/opt/geant4 \
      -DGEANT4_INSTALL_DATA=ON \
      -DGEANT4_USE_OPENGL_X11=ON \
      -DGEANT4_USE_QT=ON \
      -DGEANT4_BUILD_MULTITHREADED=ON \
      ../geant4-v11.2.0

# Build (this takes 30-60 minutes)
make -j$(nproc)

# Install
sudo make install
```

**Setup Geant4 environment:**
```bash
# Add to ~/.bashrc
echo 'source /opt/geant4/bin/geant4.sh' >> ~/.bashrc
source ~/.bashrc
```

**Verify Geant4 installation:**
```bash
geant4-config --version
```

#### A.1.6: Verify All Prerequisites
```bash
git --version          # Should show: git version 2.x.x
cmake --version        # Should show: cmake version 3.x.x
g++ --version          # Should show: g++ (GCC) 11.x or later
root --version         # Should show: ROOT Version: 6.x/xx
geant4-config --version # Should show: 11.x.x
```

---

### A.2: macOS Installation

#### A.2.1: Install Homebrew (if not installed)

Homebrew is a package manager for macOS.

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

**Follow the on-screen instructions** to add Homebrew to your PATH.

#### A.2.2: Install Basic Tools
```bash
brew install git cmake
```

**Verify installation:**
```bash
git --version
cmake --version
```

#### A.2.3: Install Xcode Command Line Tools

macOS needs Xcode command line tools for the C++ compiler:
```bash
xcode-select --install
```

Click "Install" in the popup window.

**Verify installation:**
```bash
clang++ --version
```

#### A.2.4: Install ROOT

```bash
brew install root
```

This will take 15-30 minutes as it compiles from source.

**Setup ROOT environment:**

Homebrew will tell you to add something like this to your shell profile:
```bash
# Add to ~/.zshrc (macOS default shell) or ~/.bash_profile
echo 'source $(brew --prefix root)/bin/thisroot.sh' >> ~/.zshrc
source ~/.zshrc
```

**Verify ROOT installation:**
```bash
root --version
```

#### A.2.5: Install Qt (for Geant4 visualization)

```bash
brew install qt@5
```

**Add Qt to PATH:**
```bash
echo 'export PATH="/opt/homebrew/opt/qt@5/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

#### A.2.6: Install Geant4

**Download Geant4:**
```bash
# Create installation directory
mkdir -p ~/geant4
cd ~/geant4

# Download Geant4 11.2 (or latest version)
curl -O https://geant4-data.web.cern.ch/releases/geant4-v11.2.0.tar.gz

# Extract
tar -xzf geant4-v11.2.0.tar.gz
```

**Build Geant4:**
```bash
# Create build directory
mkdir geant4-v11.2.0-build
cd geant4-v11.2.0-build

# Configure with CMake
cmake -DCMAKE_INSTALL_PREFIX=/usr/local/geant4 \
      -DGEANT4_INSTALL_DATA=ON \
      -DGEANT4_USE_OPENGL_X11=ON \
      -DGEANT4_USE_QT=ON \
      -DGEANT4_BUILD_MULTITHREADED=ON \
      -DQt5_DIR=$(brew --prefix qt@5)/lib/cmake/Qt5 \
      ../geant4-v11.2.0

# Build (this takes 30-60 minutes)
make -j$(sysctl -n hw.ncpu)

# Install
sudo make install
```

**Setup Geant4 environment:**
```bash
# Add to ~/.zshrc
echo 'source /usr/local/geant4/bin/geant4.sh' >> ~/.zshrc
source ~/.zshrc
```

**Verify Geant4 installation:**
```bash
geant4-config --version
```

#### A.2.7: Verify All Prerequisites
```bash
git --version          # Should show: git version 2.x.x
cmake --version        # Should show: cmake version 3.x.x
clang++ --version      # Should show: Apple clang version 15.x or later
root --version         # Should show: ROOT Version: 6.x/xx
geant4-config --version # Should show: 11.x.x
```

---

### A.3: Troubleshooting Installation Issues

#### Issue: "command not found" after installation

**Solution:** Make sure environment variables are loaded:
```bash
# For bash users:
source ~/.bashrc

# For zsh users (macOS default):
source ~/.zshrc
```

Or close and reopen your terminal.

#### Issue: Geant4 data files not found

**Solution:** Download data files manually:
```bash
# Ubuntu/macOS:
cd /tmp
wget https://geant4-data.web.cern.ch/datasets/G4NDL.4.7.tar.gz
tar -xzf G4NDL.4.7.tar.gz
sudo mv G4NDL4.7 /opt/geant4/share/Geant4/data/
```

Repeat for other required datasets (G4EMLOW, PhotonEvaporation, etc.).

#### Issue: CMake cannot find Qt5

**Ubuntu Solution:**
```bash
sudo apt install qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools
```

**macOS Solution:**
```bash
export Qt5_DIR=$(brew --prefix qt@5)/lib/cmake/Qt5
```

#### Issue: ROOT plots don't display

**Ubuntu Solution:**
```bash
sudo apt install libx11-dev libxpm-dev libxft-dev libxext-dev
```

**macOS Solution:**
```bash
# Install XQuartz for X11 support
brew install --cask xquartz
# Log out and log back in
```

---

### A.4: Quick Installation Script

**For experienced users only** - This script automates the installation but may not work on all systems:

**Ubuntu:**
```bash
#!/bin/bash
# Save as install_prerequisites_ubuntu.sh

sudo apt update
sudo apt install -y git cmake g++ gcc make \
    libexpat1-dev libxerces-c-dev libxmu-dev libxi-dev \
    libx11-dev libxpm-dev libxft-dev libglu1-mesa-dev \
    libglew-dev qt5-default libqt5opengl5-dev \
    root-system libroot-dev

# Note: This installs Geant4 dependencies but not Geant4 itself
# Follow A.1.5 to build Geant4 from source
```

**macOS:**
```bash
#!/bin/bash
# Save as install_prerequisites_macos.sh

# Install Homebrew if not installed
if ! command -v brew &> /dev/null; then
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
fi

brew install git cmake root qt@5

# Note: Still need to build Geant4 from source (see A.2.6)
```

---

**After installing all prerequisites, return to [Section 0: Downloading the Code](#0-downloading-the-code) to continue the tutorial.**
