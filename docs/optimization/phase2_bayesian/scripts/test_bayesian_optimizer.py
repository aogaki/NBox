#!/usr/bin/env python3
"""
Unit tests for bayesian_optimizer_constrained.py
Tests the optimization logic without running actual simulations
"""

import unittest
import math
import sys
from pathlib import Path

# Add scripts directory to path
sys.path.insert(0, str(Path(__file__).parent))

from bayesian_optimizer_constrained import (
    generate_uniform_ring_config,
    is_valid_uniform_config,
)
from geometry_generator import (
    DetectorInventory,
    DETECTOR_DIAMETER,
    MIN_GAP,
    BEAM_PIPE_RADIUS,
    BOX_HALF_WIDTH
)


class TestGenerateUniformRingConfig(unittest.TestCase):
    """Test uniform ring config generation"""

    def setUp(self):
        self.inventory = DetectorInventory(
            short_type="He3_ELIGANT",
            short_count=28,
            long_type="He3_ELIGANT_Long",
            long_count=40
        )

    def test_all_short_ring(self):
        """Test ring with all short detectors"""
        config = generate_uniform_ring_config(
            radii=[100],
            ring_types=['short'],
            counts=[10],
            inventory=self.inventory
        )

        self.assertEqual(len(config["Placements"]), 10)
        for p in config["Placements"]:
            self.assertEqual(p["type"], "He3_ELIGANT")

    def test_all_long_ring(self):
        """Test ring with all long detectors"""
        config = generate_uniform_ring_config(
            radii=[100],
            ring_types=['long'],
            counts=[10],
            inventory=self.inventory
        )

        self.assertEqual(len(config["Placements"]), 10)
        for p in config["Placements"]:
            self.assertEqual(p["type"], "He3_ELIGANT_Long")

    def test_mixed_rings(self):
        """Test multiple rings with different types"""
        config = generate_uniform_ring_config(
            radii=[50, 100, 150, 200],
            ring_types=['short', 'long', 'short', 'long'],
            counts=[7, 20, 21, 20],
            inventory=self.inventory
        )

        total = len(config["Placements"])
        self.assertEqual(total, 68)

        # Count by type
        short_count = sum(1 for p in config["Placements"] if p["type"] == "He3_ELIGANT")
        long_count = sum(1 for p in config["Placements"] if p["type"] == "He3_ELIGANT_Long")
        self.assertEqual(short_count, 28)  # 7 + 21
        self.assertEqual(long_count, 40)   # 20 + 20

    def test_ring_naming(self):
        """Test ring naming convention A, B, C, D"""
        config = generate_uniform_ring_config(
            radii=[50, 100, 150, 200],
            ring_types=['short', 'long', 'short', 'long'],
            counts=[5, 5, 5, 5],
            inventory=self.inventory
        )

        placements = config["Placements"]
        # First ring -> A
        for p in placements[:5]:
            self.assertTrue(p["name"].startswith("A"))
        # Second ring -> B
        for p in placements[5:10]:
            self.assertTrue(p["name"].startswith("B"))

    def test_angle_distribution(self):
        """Test detectors are evenly distributed in angle"""
        config = generate_uniform_ring_config(
            radii=[100],
            ring_types=['short'],
            counts=[4],
            inventory=self.inventory
        )

        angles = [p["Phi"] for p in config["Placements"]]
        self.assertAlmostEqual(angles[0], 0, places=1)
        self.assertAlmostEqual(angles[1], 90, places=1)
        self.assertAlmostEqual(angles[2], 180, places=1)
        self.assertAlmostEqual(angles[3], 270, places=1)

    def test_box_configuration(self):
        """Test box configuration in output"""
        config = generate_uniform_ring_config(
            radii=[100],
            ring_types=['short'],
            counts=[5],
            inventory=self.inventory,
            box_size=(800, 800, 800),
            beam_pipe_diameter=50
        )

        self.assertEqual(config["Box"]["x"], 800)
        self.assertEqual(config["Box"]["y"], 800)
        self.assertEqual(config["Box"]["z"], 800)
        self.assertEqual(config["Box"]["BeamPipe"], 50)


class TestIsValidUniformConfig(unittest.TestCase):
    """Test uniform config validation"""

    def setUp(self):
        self.inventory = DetectorInventory(short_count=28, long_count=40)

    def test_valid_config(self):
        """Test a valid configuration"""
        radii = [50, 100, 170, 250]
        counts = [7, 20, 21, 20]
        ring_types = ['short', 'long', 'short', 'long']

        is_valid, error = is_valid_uniform_config(radii, counts, ring_types, self.inventory)
        self.assertTrue(is_valid, f"Should be valid: {error}")

    def test_beam_pipe_too_close(self):
        """Test beam pipe clearance check"""
        radii = [30, 100, 150, 200]  # First ring too close to beam pipe
        counts = [5, 15, 20, 28]
        ring_types = ['short', 'long', 'short', 'long']

        is_valid, error = is_valid_uniform_config(radii, counts, ring_types, self.inventory)
        self.assertFalse(is_valid)
        self.assertIn("beam pipe", error.lower())

    def test_exceeds_box_boundary(self):
        """Test box boundary check"""
        radii = [50, 100, 150, 490]  # Last ring too close to boundary
        counts = [5, 15, 8, 40]
        ring_types = ['short', 'short', 'short', 'long']

        is_valid, error = is_valid_uniform_config(radii, counts, ring_types, self.inventory)
        self.assertFalse(is_valid)
        self.assertIn("boundary", error.lower())

    def test_rings_too_close(self):
        """Test ring spacing check"""
        radii = [50, 70, 150, 200]  # Second ring too close to first
        counts = [5, 13, 10, 40]
        ring_types = ['short', 'short', 'short', 'long']

        is_valid, error = is_valid_uniform_config(radii, counts, ring_types, self.inventory)
        self.assertFalse(is_valid)
        self.assertIn("too close", error.lower())

    def test_detector_spacing_too_small(self):
        """Test detector spacing within ring"""
        radii = [50, 100, 150, 200]
        counts = [50, 15, 8, 5]  # Too many detectors in first ring
        ring_types = ['short', 'long', 'short', 'long']

        is_valid, error = is_valid_uniform_config(radii, counts, ring_types, self.inventory)
        self.assertFalse(is_valid)
        self.assertIn("spacing", error.lower())

    def test_short_count_mismatch(self):
        """Test exact short count requirement"""
        radii = [50, 100, 170, 250]
        counts = [10, 20, 18, 20]  # 10 + 18 = 28 short (correct), 20 + 20 = 40 long (correct)
        ring_types = ['short', 'long', 'short', 'long']

        is_valid, error = is_valid_uniform_config(radii, counts, ring_types, self.inventory)
        self.assertTrue(is_valid, f"Should be valid: {error}")

        # Now test with wrong short count
        counts = [5, 20, 18, 20]  # 5 + 18 = 23 short != 28
        is_valid, error = is_valid_uniform_config(radii, counts, ring_types, self.inventory)
        self.assertFalse(is_valid)
        self.assertIn("short", error.lower())

    def test_long_count_mismatch(self):
        """Test exact long count requirement"""
        radii = [50, 100, 170, 250]
        counts = [7, 15, 21, 20]  # 7 + 21 = 28 short (correct), 15 + 20 = 35 long != 40
        ring_types = ['short', 'long', 'short', 'long']

        is_valid, error = is_valid_uniform_config(radii, counts, ring_types, self.inventory)
        self.assertFalse(is_valid)
        self.assertIn("long", error.lower())


class TestConstraintLogic(unittest.TestCase):
    """Test constraint satisfaction logic"""

    def setUp(self):
        self.inventory = DetectorInventory(short_count=28, long_count=40)

    def test_slsl_pattern(self):
        """Test S-L-S-L pattern"""
        radii = [50, 100, 170, 250]
        counts = [7, 20, 21, 20]  # S:7+21=28, L:20+20=40
        ring_types = ['short', 'long', 'short', 'long']

        is_valid, error = is_valid_uniform_config(radii, counts, ring_types, self.inventory)
        self.assertTrue(is_valid, f"S-L-S-L should be valid: {error}")

    def test_ssll_pattern(self):
        """Test S-S-L-L pattern"""
        radii = [50, 120, 200, 280]
        counts = [7, 21, 20, 20]  # S:7+21=28, L:20+20=40
        ring_types = ['short', 'short', 'long', 'long']

        is_valid, error = is_valid_uniform_config(radii, counts, ring_types, self.inventory)
        self.assertTrue(is_valid, f"S-S-L-L should be valid: {error}")

    def test_llss_pattern(self):
        """Test L-L-S-S pattern"""
        # r=80: circumference=502mm, max=16 detectors
        # r=200: circumference=1257mm, max=41 detectors
        # r=280: circumference=1759mm, max=57 detectors
        # r=360: circumference=2262mm, max=74 detectors
        radii = [80, 200, 280, 360]
        counts = [8, 32, 14, 14]  # L:8+32=40, S:14+14=28
        ring_types = ['long', 'long', 'short', 'short']

        is_valid, error = is_valid_uniform_config(radii, counts, ring_types, self.inventory)
        self.assertTrue(is_valid, f"L-L-S-S should be valid: {error}")

    def test_lsls_pattern(self):
        """Test L-S-L-S pattern"""
        radii = [50, 100, 170, 250]
        counts = [8, 14, 32, 14]  # L:8+32=40, S:14+14=28
        ring_types = ['long', 'short', 'long', 'short']

        is_valid, error = is_valid_uniform_config(radii, counts, ring_types, self.inventory)
        self.assertTrue(is_valid, f"L-S-L-S should be valid: {error}")

    def test_total_68_detectors(self):
        """Verify all valid configs have exactly 68 detectors"""
        radii = [50, 100, 170, 250]
        counts = [7, 20, 21, 20]
        ring_types = ['short', 'long', 'short', 'long']

        is_valid, _ = is_valid_uniform_config(radii, counts, ring_types, self.inventory)
        self.assertTrue(is_valid)
        self.assertEqual(sum(counts), 68)


class TestPhysicalConstraintValues(unittest.TestCase):
    """Test that physical constraint values are reasonable"""

    def test_detector_diameter(self):
        """Verify detector diameter value"""
        self.assertAlmostEqual(DETECTOR_DIAMETER, 25.4, places=1)  # 1 inch

    def test_min_gap(self):
        """Verify minimum gap value"""
        self.assertEqual(MIN_GAP, 5.0)

    def test_beam_pipe_radius(self):
        """Verify beam pipe radius"""
        self.assertEqual(BEAM_PIPE_RADIUS, 22.0)

    def test_box_half_width(self):
        """Verify box half width"""
        self.assertEqual(BOX_HALF_WIDTH, 500.0)  # 1m box / 2

    def test_min_spacing(self):
        """Verify minimum spacing between detectors"""
        min_spacing = DETECTOR_DIAMETER + MIN_GAP
        self.assertAlmostEqual(min_spacing, 30.4, places=1)

    def test_max_detectors_calculation(self):
        """Test maximum detectors per ring calculation"""
        for radius in [50, 100, 200, 400]:
            circumference = 2 * math.pi * radius
            max_n = int(circumference / (DETECTOR_DIAMETER + MIN_GAP))

            # Verify the formula
            expected_spacing = circumference / max_n
            self.assertGreaterEqual(expected_spacing, DETECTOR_DIAMETER + MIN_GAP)


class TestEdgeCases(unittest.TestCase):
    """Test edge cases and boundary conditions"""

    def setUp(self):
        self.inventory = DetectorInventory(short_count=28, long_count=40)

    def test_minimum_radius_ring(self):
        """Test ring at minimum valid radius"""
        min_r = BEAM_PIPE_RADIUS + DETECTOR_DIAMETER / 2 + 1  # ~35.7mm
        # min_r circumference ~224mm, max ~7 detectors
        # Need larger radii for more detectors
        radii = [min_r, min_r + 50, min_r + 150, min_r + 350]  # ~35.7, 85.7, 185.7, 385.7
        counts = [5, 8, 15, 40]  # S:5+8+15=28, L:40
        ring_types = ['short', 'short', 'short', 'long']

        is_valid, error = is_valid_uniform_config(radii, counts, ring_types, self.inventory)
        self.assertTrue(is_valid, f"Min radius config should be valid: {error}")

    def test_empty_ring_count(self):
        """Test handling of zero count ring"""
        radii = [50, 100, 150, 200]
        counts = [0, 28, 20, 20]  # First ring empty
        ring_types = ['short', 'short', 'long', 'long']

        # This should fail because short count won't match
        is_valid, error = is_valid_uniform_config(radii, counts, ring_types, self.inventory)
        # With 0 short in ring 1 and 28 short in ring 2: total = 28 (OK)
        # But ring with 0 detectors might be handled specially
        # The function should still validate counts


class TestOutputFormat(unittest.TestCase):
    """Test output JSON format"""

    def setUp(self):
        self.inventory = DetectorInventory()

    def test_placement_format(self):
        """Test individual placement format"""
        config = generate_uniform_ring_config(
            radii=[100],
            ring_types=['short'],
            counts=[5],
            inventory=self.inventory
        )

        placement = config["Placements"][0]
        self.assertIn("name", placement)
        self.assertIn("type", placement)
        self.assertIn("R", placement)
        self.assertIn("Phi", placement)

        # Check types
        self.assertIsInstance(placement["name"], str)
        self.assertIsInstance(placement["type"], str)
        self.assertIsInstance(placement["R"], (int, float))
        self.assertIsInstance(placement["Phi"], (int, float))

    def test_box_format(self):
        """Test box configuration format"""
        config = generate_uniform_ring_config(
            radii=[100],
            ring_types=['short'],
            counts=[5],
            inventory=self.inventory
        )

        box = config["Box"]
        self.assertIn("Type", box)
        self.assertIn("x", box)
        self.assertIn("y", box)
        self.assertIn("z", box)
        self.assertIn("BeamPipe", box)

        self.assertEqual(box["Type"], "Box")


if __name__ == "__main__":
    unittest.main(verbosity=2)
