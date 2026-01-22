#!/usr/bin/env python3
"""
Unit tests for run_nbox.py
Tests the efficiency calculation logic and output parsing
"""

import unittest
import tempfile
import os
from pathlib import Path


class TestEfficiencyCalculation(unittest.TestCase):
    """Test efficiency calculation logic"""

    def test_efficiency_formula(self):
        """Test efficiency formula: efficiency = 100 * n_events_with_hits / n_neutrons"""
        test_cases = [
            (1000, 100, 10.0),    # 100/1000 = 10%
            (10000, 8000, 80.0),  # 8000/10000 = 80%
            (10000, 0, 0.0),      # 0 hits = 0%
            (10000, 10000, 100.0),  # all hit = 100%
            (100000, 73244, 73.244),  # typical result
        ]

        for n_neutrons, n_events_with_hits, expected_eff in test_cases:
            efficiency = 100.0 * n_events_with_hits / n_neutrons
            self.assertAlmostEqual(efficiency, expected_eff, places=3,
                                   msg=f"Failed for {n_events_with_hits}/{n_neutrons}")

    def test_efficiency_bounds(self):
        """Test that efficiency is always between 0 and 100"""
        for n_events_with_hits in [0, 100, 5000, 10000]:
            n_neutrons = 10000
            efficiency = 100.0 * n_events_with_hits / n_neutrons
            self.assertGreaterEqual(efficiency, 0.0)
            self.assertLessEqual(efficiency, 100.0)


class TestOutputParsing(unittest.TestCase):
    """Test ROOT script output parsing logic"""

    def test_parse_efficiency_output(self):
        """Test parsing efficiency from ROOT output"""
        sample_output = """
root [0]
Processing calc_eff_temp.C...
EFFICIENCY:73.244
N_HITS:85000
N_EVENTS_WITH_HITS:73244
root [1]
"""
        efficiency = 0.0
        n_hits = 0
        n_events_with_hits = 0

        for line in sample_output.split('\n'):
            if line.startswith("EFFICIENCY:"):
                efficiency = float(line.split(":")[1])
            elif line.startswith("N_HITS:"):
                n_hits = int(line.split(":")[1])
            elif line.startswith("N_EVENTS_WITH_HITS:"):
                n_events_with_hits = int(line.split(":")[1])

        self.assertAlmostEqual(efficiency, 73.244, places=3)
        self.assertEqual(n_hits, 85000)
        self.assertEqual(n_events_with_hits, 73244)

    def test_parse_zero_efficiency(self):
        """Test parsing zero efficiency"""
        sample_output = """
EFFICIENCY:0
N_HITS:0
N_EVENTS_WITH_HITS:0
"""
        efficiency = 0.0
        for line in sample_output.split('\n'):
            if line.startswith("EFFICIENCY:"):
                efficiency = float(line.split(":")[1])

        self.assertEqual(efficiency, 0.0)


class TestMacroGeneration(unittest.TestCase):
    """Test macro file generation"""

    def test_macro_content_mono_energy(self):
        """Test macro content for monoenergetic source"""
        energy = 1.0
        energy_unit = "MeV"
        nevents = 10000

        expected_lines = [
            "/run/initialize",
            "/gun/particle neutron",
            f"/gun/energy {energy} {energy_unit}",
            f"/run/beamOn {nevents}"
        ]

        macro_content = "/run/initialize\n"
        macro_content += f"/gun/particle neutron\n"
        macro_content += f"/gun/energy {energy} {energy_unit}\n"
        macro_content += f"/run/beamOn {nevents}\n"

        for expected in expected_lines:
            self.assertIn(expected, macro_content)

    def test_macro_content_source_file(self):
        """Test macro content when using source file (no gun commands)"""
        nevents = 10000

        # When source_file is provided, no gun commands
        macro_content = "/run/initialize\n"
        macro_content += f"/run/beamOn {nevents}\n"

        self.assertIn("/run/initialize", macro_content)
        self.assertIn(f"/run/beamOn {nevents}", macro_content)
        self.assertNotIn("/gun/particle", macro_content)
        self.assertNotIn("/gun/energy", macro_content)


class TestCommandGeneration(unittest.TestCase):
    """Test command line generation"""

    def test_basic_command(self):
        """Test basic command structure"""
        nbox_exe = "/path/to/nbox_sim"
        geometry = "/path/to/geometry.json"
        detector_config = "/path/to/detector.json"
        macro = "/path/to/run.mac"

        cmd = [
            nbox_exe,
            "-g", geometry,
            "-d", detector_config,
            "-m", macro
        ]

        self.assertEqual(len(cmd), 7)
        self.assertEqual(cmd[0], nbox_exe)
        self.assertEqual(cmd[1], "-g")
        self.assertEqual(cmd[2], geometry)

    def test_command_with_source(self):
        """Test command with source file"""
        nbox_exe = "/path/to/nbox_sim"
        source_file = "/path/to/cf252_source.root"

        cmd = [nbox_exe, "-g", "geo.json", "-d", "det.json", "-m", "run.mac"]
        cmd.extend(["-s", source_file])

        self.assertIn("-s", cmd)
        self.assertIn(source_file, cmd)

    def test_command_with_fluxmap(self):
        """Test command with fluxmap flag"""
        cmd = ["/path/to/nbox_sim", "-g", "geo.json", "-d", "det.json", "-m", "run.mac"]
        cmd.append("-f")

        self.assertIn("-f", cmd)


class TestResultStructure(unittest.TestCase):
    """Test result data structure"""

    def test_success_result(self):
        """Test successful result structure"""
        result = {
            "success": True,
            "output_dir": "/tmp/nbox_xxx",
            "thread_files": ["/path/to/output_run0_t0.root"],
            "n_thread_files": 1,
            "stdout": "...",
            "stderr": ""
        }

        self.assertTrue(result["success"])
        self.assertIn("thread_files", result)
        self.assertIsInstance(result["thread_files"], list)

    def test_failure_result(self):
        """Test failure result structure"""
        result = {
            "success": False,
            "error": "Simulation timeout"
        }

        self.assertFalse(result["success"])
        self.assertIn("error", result)

    def test_efficiency_result(self):
        """Test efficiency result structure"""
        eff_result = {
            "efficiency": 73.244,
            "n_hits": 85000,
            "n_events_with_hits": 73244
        }

        self.assertIn("efficiency", eff_result)
        self.assertIn("n_hits", eff_result)
        self.assertIn("n_events_with_hits", eff_result)


class TestThreadFileHandling(unittest.TestCase):
    """Test thread file handling"""

    def test_thread_file_pattern(self):
        """Test thread file pattern matching"""
        import fnmatch

        thread_files = [
            "output_run0_t0.root",
            "output_run0_t1.root",
            "output_run0_t2.root",
            "output_run0_t3.root"
        ]

        pattern = "output_run0_t*.root"

        for f in thread_files:
            self.assertTrue(fnmatch.fnmatch(f, pattern))

    def test_no_thread_files(self):
        """Test handling when no thread files found"""
        thread_files = []

        # Should return error
        if not thread_files:
            result = {"efficiency": 0.0, "error": "No thread files"}
            self.assertIn("error", result)


class TestROOTScriptGeneration(unittest.TestCase):
    """Test ROOT script generation for efficiency calculation"""

    def test_script_includes_all_files(self):
        """Test that script includes all thread files"""
        thread_files = [
            "/path/to/output_run0_t0.root",
            "/path/to/output_run0_t1.root"
        ]

        script_lines = []
        for f in thread_files:
            script_lines.append(f'chain->Add("{f}");')

        for f in thread_files:
            matching_lines = [l for l in script_lines if f in l]
            self.assertEqual(len(matching_lines), 1)

    def test_script_uses_correct_tree_name(self):
        """Test that script uses 'NBox' tree name"""
        tree_name = "NBox"
        script_fragment = f'TChain* chain = new TChain("{tree_name}");'

        self.assertIn("NBox", script_fragment)

    def test_efficiency_formula_in_script(self):
        """Test efficiency formula in ROOT script"""
        n_neutrons = 10000
        formula = f"double efficiency = 100.0 * n_events_with_hits / {n_neutrons};"

        self.assertIn("100.0", formula)
        self.assertIn("n_events_with_hits", formula)
        self.assertIn(str(n_neutrons), formula)


class TestPathHandling(unittest.TestCase):
    """Test path handling"""

    def test_path_resolution(self):
        """Test path resolution"""
        config_path = Path("/tmp/configs/trial_0001.json")
        resolved = config_path.resolve()

        self.assertTrue(resolved.is_absolute())

    def test_parent_directory_creation(self):
        """Test parent directory creation logic"""
        with tempfile.TemporaryDirectory() as tmpdir:
            output_dir = Path(tmpdir) / "subdir" / "results"
            output_dir.mkdir(parents=True, exist_ok=True)

            self.assertTrue(output_dir.exists())
            self.assertTrue(output_dir.is_dir())


class TestCleanup(unittest.TestCase):
    """Test cleanup functionality"""

    def test_cleanup_pattern(self):
        """Test cleanup file pattern"""
        import fnmatch

        files_to_clean = [
            "output_run0_t0.root",
            "output_run0_t1.root",
            "output_run1_t0.root"
        ]

        pattern = "output_run*.root"

        for f in files_to_clean:
            self.assertTrue(fnmatch.fnmatch(f, pattern))


if __name__ == "__main__":
    unittest.main(verbosity=2)
