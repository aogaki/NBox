#!/usr/bin/env python3
"""
NBox Simulation Runner
Wrapper for running NBox simulations and collecting results
"""

import subprocess
import os
import glob
import tempfile
from pathlib import Path
from typing import Optional, Dict, Any
import json


class NBoxRunner:
    def __init__(self, build_dir: str, detector_config: str):
        """
        Initialize NBox runner

        Parameters:
        -----------
        build_dir : str
            Path to NBox build directory
        detector_config : str
            Path to detector description JSON file
        """
        self.build_dir = Path(build_dir)
        self.nbox_exe = self.build_dir / "nbox_sim"
        self.detector_config = Path(detector_config)

        if not self.nbox_exe.exists():
            raise FileNotFoundError(f"NBox executable not found: {self.nbox_exe}")
        if not self.detector_config.exists():
            raise FileNotFoundError(f"Detector config not found: {self.detector_config}")

    def run_simulation(self, geometry_config: str, nevents: int,
                      source_file: Optional[str] = None,
                      energy: Optional[float] = None,
                      energy_unit: str = "MeV",
                      output_dir: Optional[str] = None,
                      enable_fluxmap: bool = False) -> Dict[str, Any]:
        """
        Run NBox simulation

        Parameters:
        -----------
        geometry_config : str
            Path to geometry JSON file
        nevents : int
            Number of events to simulate
        source_file : str, optional
            Path to source spectrum ROOT file
        energy : float, optional
            Monoenergetic neutron energy (used if source_file not provided)
        energy_unit : str
            Energy unit (eV, keV, MeV)
        output_dir : str, optional
            Output directory for results
        enable_fluxmap : bool
            Enable flux map recording

        Returns:
        --------
        dict
            Simulation results including output file paths
        """
        if output_dir is None:
            output_dir = tempfile.mkdtemp(prefix="nbox_")
        output_dir = Path(output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)

        # Create macro file
        macro_content = "/run/initialize\n"
        if source_file is None:
            if energy is None:
                energy = 1.0
                energy_unit = "MeV"
            macro_content += f"/gun/particle neutron\n"
            macro_content += f"/gun/energy {energy} {energy_unit}\n"
        macro_content += f"/run/beamOn {nevents}\n"

        macro_path = output_dir / "run.mac"
        macro_path.write_text(macro_content)

        # Build command
        cmd = [
            str(self.nbox_exe),
            "-g", str(geometry_config),
            "-d", str(self.detector_config),
            "-m", str(macro_path)
        ]

        if source_file is not None:
            cmd.extend(["-s", str(source_file)])

        if enable_fluxmap:
            cmd.append("-f")

        # Run simulation
        try:
            result = subprocess.run(
                cmd,
                cwd=str(self.build_dir),
                capture_output=True,
                text=True,
                timeout=3600  # 1 hour timeout
            )
        except subprocess.TimeoutExpired:
            return {"success": False, "error": "Simulation timeout"}

        if result.returncode != 0:
            return {
                "success": False,
                "error": result.stderr,
                "stdout": result.stdout
            }

        # Collect output files
        thread_files = list(self.build_dir.glob("output_run0_t*.root"))

        return {
            "success": True,
            "output_dir": str(output_dir),
            "thread_files": [str(f) for f in thread_files],
            "n_thread_files": len(thread_files),
            "stdout": result.stdout,
            "stderr": result.stderr
        }

    def calculate_efficiency(self, thread_files: list, n_neutrons: int) -> Dict[str, float]:
        """
        Calculate detection efficiency from simulation results

        Parameters:
        -----------
        thread_files : list
            List of thread output ROOT file paths
        n_neutrons : int
            Number of incident neutrons

        Returns:
        --------
        dict
            Efficiency values
        """
        if not thread_files:
            return {"efficiency": 0.0, "error": "No thread files"}

        # Use ROOT to calculate efficiency
        # Note: function name must match file name for ROOT to auto-execute
        root_script = f'''
#include <iostream>
#include "TChain.h"
#include "TH1.h"

void calc_eff_temp() {{
    TChain* chain = new TChain("NBox");
'''

        for f in thread_files:
            root_script += f'    chain->Add("{f}");\n'

        root_script += f'''
    Long64_t n_hits = chain->GetEntries();

    // Count unique events with any hit
    chain->Draw("EventID>>h_events({n_neutrons}, 0, {n_neutrons})", "", "goff");
    TH1* h = (TH1*)gDirectory->Get("h_events");
    Int_t n_events_with_hits = 0;
    for (Int_t i = 1; i <= h->GetNbinsX(); i++) {{
        if (h->GetBinContent(i) > 0) n_events_with_hits++;
    }}

    // Calculate efficiency
    double efficiency = 100.0 * n_events_with_hits / {n_neutrons};

    std::cout << "EFFICIENCY:" << efficiency << std::endl;
    std::cout << "N_HITS:" << n_hits << std::endl;
    std::cout << "N_EVENTS_WITH_HITS:" << n_events_with_hits << std::endl;
}}
'''

        # Save and run script
        script_path = self.build_dir / "calc_eff_temp.C"
        script_path.write_text(root_script)

        try:
            # Run ROOT directly with full path (avoid sourcing issues in subprocess)
            result = subprocess.run(
                ["/opt/ROOT/bin/root", "-l", "-b", "-q", str(script_path)],
                cwd=str(self.build_dir),
                capture_output=True,
                text=True,
                timeout=60
            )

            # Parse output
            efficiency = 0.0
            n_hits = 0
            n_events_with_hits = 0

            for line in result.stdout.split('\n'):
                if line.startswith("EFFICIENCY:"):
                    efficiency = float(line.split(":")[1])
                elif line.startswith("N_HITS:"):
                    n_hits = int(line.split(":")[1])
                elif line.startswith("N_EVENTS_WITH_HITS:"):
                    n_events_with_hits = int(line.split(":")[1])

            return {
                "efficiency": efficiency,
                "n_hits": n_hits,
                "n_events_with_hits": n_events_with_hits
            }

        except Exception as e:
            return {"efficiency": 0.0, "error": str(e)}

        finally:
            if script_path.exists():
                script_path.unlink()

    def cleanup_thread_files(self):
        """Remove thread output files from build directory"""
        for f in self.build_dir.glob("output_run*.root"):
            f.unlink()


if __name__ == "__main__":
    # Test the runner
    import sys

    if len(sys.argv) < 3:
        print("Usage: run_nbox.py <build_dir> <geometry.json>")
        sys.exit(1)

    build_dir = sys.argv[1]
    geometry = sys.argv[2]

    runner = NBoxRunner(
        build_dir=build_dir,
        detector_config=f"{build_dir}/eligant_tn_detector.json"
    )

    result = runner.run_simulation(
        geometry_config=geometry,
        nevents=1000,
        energy=1.0,
        energy_unit="MeV"
    )

    print(json.dumps(result, indent=2))
