#!/usr/bin/env python3
"""
Unit tests for geometry_generator.py
"""

import unittest
import math
import json
import os
import tempfile
from geometry_generator import (
    is_valid_configuration,
    generate_ring_placements,
    generate_geometry_config,
    save_geometry_config,
    total_detectors,
    summarize_config,
    DetectorInventory,
    DETECTOR_DIAMETER,
    MIN_GAP,
    BEAM_PIPE_RADIUS,
    BOX_HALF_WIDTH
)


class TestDetectorInventory(unittest.TestCase):
    """Test DetectorInventory dataclass"""

    def test_default_values(self):
        """Test default inventory values"""
        inv = DetectorInventory()
        self.assertEqual(inv.short_type, "He3_ELIGANT")
        self.assertEqual(inv.short_count, 28)
        self.assertEqual(inv.long_type, "He3_ELIGANT_Long")
        self.assertEqual(inv.long_count, 40)

    def test_total_available(self):
        """Test total_available calculation"""
        inv = DetectorInventory(short_count=10, long_count=20)
        self.assertEqual(inv.total_available(), 30)

    def test_custom_values(self):
        """Test custom inventory values"""
        inv = DetectorInventory(
            short_type="Custom_Short",
            short_count=15,
            long_type="Custom_Long",
            long_count=25
        )
        self.assertEqual(inv.short_type, "Custom_Short")
        self.assertEqual(inv.short_count, 15)
        self.assertEqual(inv.total_available(), 40)


class TestIsValidConfiguration(unittest.TestCase):
    """Test configuration validation"""

    def setUp(self):
        self.inventory = DetectorInventory(short_count=28, long_count=40)

    def test_valid_basic_config(self):
        """Test a valid basic configuration"""
        radii = [50, 100, 150, 200]
        counts = [5, 10, 15, 20]
        is_valid, error = is_valid_configuration(radii, counts)
        self.assertTrue(is_valid, f"Should be valid but got: {error}")

    def test_beam_pipe_clearance(self):
        """Test beam pipe clearance check"""
        # Detector at r=30, with diameter 25.4, edge at 17.3 < BEAM_PIPE_RADIUS (22)
        radii = [30, 100, 150, 200]
        counts = [5, 10, 15, 20]
        is_valid, error = is_valid_configuration(radii, counts)
        self.assertFalse(is_valid)
        self.assertIn("beam pipe", error.lower())

    def test_box_boundary(self):
        """Test box boundary check"""
        # Detector at r=490, with diameter 25.4, edge at 502.7 > BOX_HALF_WIDTH (500)
        radii = [50, 100, 150, 490]
        counts = [5, 10, 15, 20]
        is_valid, error = is_valid_configuration(radii, counts)
        self.assertFalse(is_valid)
        self.assertIn("boundary", error.lower())

    def test_ring_spacing(self):
        """Test ring spacing check"""
        # Rings too close: 100 - 90 = 10 < DETECTOR_DIAMETER + MIN_GAP (30.4)
        radii = [50, 90, 100, 200]
        counts = [5, 10, 15, 20]
        is_valid, error = is_valid_configuration(radii, counts)
        self.assertFalse(is_valid)
        self.assertIn("too close", error.lower())

    def test_detector_spacing_in_ring(self):
        """Test detector spacing within ring"""
        # Ring with r=50mm, circumference=314mm
        # 50 detectors -> spacing = 6.3mm < DETECTOR_DIAMETER + MIN_GAP (30.4)
        radii = [50, 100, 150, 200]
        counts = [50, 10, 15, 20]  # Too many in first ring
        is_valid, error = is_valid_configuration(radii, counts)
        self.assertFalse(is_valid)
        self.assertIn("spacing", error.lower())

    def test_inventory_short_exceeded(self):
        """Test short detector inventory constraint"""
        radii = [50, 100, 150, 200]
        counts = [10, 15, 20, 25]  # Total 70
        short_counts = [10, 15, 10, 0]  # 35 short > 28 available
        is_valid, error = is_valid_configuration(radii, counts, short_counts, self.inventory)
        self.assertFalse(is_valid)
        self.assertIn("short", error.lower())

    def test_inventory_long_exceeded(self):
        """Test long detector inventory constraint"""
        radii = [50, 100, 150, 200]
        counts = [10, 15, 20, 25]  # Total 70
        short_counts = [0, 0, 0, 5]  # 5 short -> 65 long > 40 available
        is_valid, error = is_valid_configuration(radii, counts, short_counts, self.inventory)
        self.assertFalse(is_valid)
        self.assertIn("long", error.lower())

    def test_short_count_exceeds_total(self):
        """Test short count exceeding total count in ring"""
        radii = [50, 100, 150, 200]
        counts = [5, 10, 15, 20]
        short_counts = [10, 5, 5, 5]  # First ring: 10 short > 5 total
        is_valid, error = is_valid_configuration(radii, counts, short_counts, self.inventory)
        self.assertFalse(is_valid)
        self.assertIn("short count", error.lower())

    def test_negative_short_count(self):
        """Test negative short count"""
        radii = [50, 100, 150, 200]
        counts = [5, 10, 15, 20]
        short_counts = [-1, 5, 5, 5]
        is_valid, error = is_valid_configuration(radii, counts, short_counts, self.inventory)
        self.assertFalse(is_valid)
        self.assertIn("negative", error.lower())

    def test_length_mismatch(self):
        """Test radii and counts length mismatch"""
        radii = [50, 100, 150]
        counts = [5, 10, 15, 20]
        is_valid, error = is_valid_configuration(radii, counts)
        self.assertFalse(is_valid)
        self.assertIn("length", error.lower())


class TestGenerateRingPlacements(unittest.TestCase):
    """Test ring placement generation"""

    def test_single_type_ring(self):
        """Test ring with single detector type"""
        placements = generate_ring_placements(100, 10, 0, 1)  # 10 short, 0 long
        self.assertEqual(len(placements), 10)
        for p in placements:
            self.assertEqual(p["type"], "He3_ELIGANT")
            self.assertEqual(p["R"], 100)

    def test_mixed_type_ring(self):
        """Test ring with mixed detector types"""
        placements = generate_ring_placements(100, 5, 5, 1)  # 5 short, 5 long
        self.assertEqual(len(placements), 10)

        short_count = sum(1 for p in placements if p["type"] == "He3_ELIGANT")
        long_count = sum(1 for p in placements if p["type"] == "He3_ELIGANT_Long")
        self.assertEqual(short_count, 5)
        self.assertEqual(long_count, 5)

    def test_angle_distribution(self):
        """Test even angle distribution"""
        placements = generate_ring_placements(100, 4, 0, 1)
        angles = [p["Phi"] for p in placements]
        self.assertAlmostEqual(angles[0], 0, places=1)
        self.assertAlmostEqual(angles[1], 90, places=1)
        self.assertAlmostEqual(angles[2], 180, places=1)
        self.assertAlmostEqual(angles[3], 270, places=1)

    def test_ring_naming(self):
        """Test ring naming convention"""
        placements = generate_ring_placements(100, 3, 0, 1)  # Ring 1 -> A
        for p in placements:
            self.assertTrue(p["name"].startswith("A"))

        placements = generate_ring_placements(100, 3, 0, 2)  # Ring 2 -> B
        for p in placements:
            self.assertTrue(p["name"].startswith("B"))

    def test_empty_ring(self):
        """Test empty ring"""
        placements = generate_ring_placements(100, 0, 0, 1)
        self.assertEqual(len(placements), 0)


class TestGenerateGeometryConfig(unittest.TestCase):
    """Test geometry config generation"""

    def test_basic_config_structure(self):
        """Test basic config structure"""
        config = generate_geometry_config([50, 100], [5, 10])

        self.assertIn("Box", config)
        self.assertIn("Placements", config)
        self.assertEqual(config["Box"]["Type"], "Box")
        self.assertEqual(config["Box"]["x"], 1000)
        self.assertEqual(config["Box"]["y"], 1000)
        self.assertEqual(config["Box"]["z"], 1000)

    def test_custom_box_size(self):
        """Test custom box size"""
        config = generate_geometry_config([50, 100], [5, 10], box_size=(800, 800, 800))
        self.assertEqual(config["Box"]["x"], 800)

    def test_beam_pipe_diameter(self):
        """Test beam pipe diameter"""
        config = generate_geometry_config([50, 100], [5, 10], beam_pipe_diameter=50)
        self.assertEqual(config["Box"]["BeamPipe"], 50)

    def test_total_placements(self):
        """Test total number of placements"""
        # Use inventory that allows 30 detectors
        inventory = DetectorInventory(short_count=30, long_count=40)
        config = generate_geometry_config([50, 100, 150], [5, 10, 15], inventory=inventory)
        self.assertEqual(len(config["Placements"]), 30)

    def test_invalid_config_raises(self):
        """Test that invalid config raises ValueError"""
        with self.assertRaises(ValueError):
            # Ring too close to beam pipe
            generate_geometry_config([25, 100], [5, 10])


class TestSaveGeometryConfig(unittest.TestCase):
    """Test config saving"""

    def test_save_and_load(self):
        """Test saving and loading config"""
        config = generate_geometry_config([50, 100], [5, 10])

        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            filepath = f.name

        try:
            save_geometry_config(config, filepath)

            with open(filepath, 'r') as f:
                loaded = json.load(f)

            self.assertEqual(loaded["Box"]["x"], config["Box"]["x"])
            self.assertEqual(len(loaded["Placements"]), len(config["Placements"]))
        finally:
            os.unlink(filepath)


class TestHelperFunctions(unittest.TestCase):
    """Test helper functions"""

    def test_total_detectors(self):
        """Test total detector count"""
        self.assertEqual(total_detectors([5, 10, 15, 20]), 50)
        self.assertEqual(total_detectors([]), 0)

    def test_summarize_config(self):
        """Test config summary"""
        counts = [10, 15, 20, 25]
        short_counts = [5, 10, 5, 8]
        summary = summarize_config(counts, short_counts)

        self.assertEqual(summary["total"], 70)
        self.assertEqual(summary["short"], 28)
        self.assertEqual(summary["long"], 42)


class TestPhysicalConstraints(unittest.TestCase):
    """Test physical constraint calculations"""

    def test_max_detectors_per_ring(self):
        """Test maximum detectors that can fit in a ring"""
        for radius in [50, 100, 200, 400]:
            circumference = 2 * math.pi * radius
            max_n = int(circumference / (DETECTOR_DIAMETER + MIN_GAP))

            # Verify this many detectors is valid
            radii = [radius]
            counts = [max_n]
            is_valid, _ = is_valid_configuration(radii, counts)
            self.assertTrue(is_valid, f"Failed for r={radius}, n={max_n}")

            # Verify one more is invalid
            counts = [max_n + 1]
            is_valid, _ = is_valid_configuration(radii, counts)
            self.assertFalse(is_valid, f"Should fail for r={radius}, n={max_n+1}")

    def test_minimum_inner_radius(self):
        """Test minimum inner radius for beam pipe clearance"""
        min_radius = BEAM_PIPE_RADIUS + DETECTOR_DIAMETER / 2 + 1  # +1 for margin

        is_valid, _ = is_valid_configuration([min_radius], [3])
        self.assertTrue(is_valid)

        is_valid, _ = is_valid_configuration([min_radius - 5], [3])
        self.assertFalse(is_valid)

    def test_maximum_outer_radius(self):
        """Test maximum outer radius for box boundary"""
        max_radius = BOX_HALF_WIDTH - DETECTOR_DIAMETER / 2 - 1  # -1 for margin

        is_valid, _ = is_valid_configuration([50, max_radius], [5, 10])
        self.assertTrue(is_valid)

        is_valid, _ = is_valid_configuration([50, max_radius + 5], [5, 10])
        self.assertFalse(is_valid)


class TestConstrainedConfiguration(unittest.TestCase):
    """Test configurations with full constraint set (68 total, uniform rings)"""

    def setUp(self):
        self.inventory = DetectorInventory(short_count=28, long_count=40)

    def test_exact_inventory_usage(self):
        """Test configuration using exactly all detectors"""
        # 2 short rings (total 28) + 2 long rings (total 40)
        # Need larger radii to fit detectors with proper spacing
        radii = [50, 120, 200, 280]
        counts = [7, 21, 20, 20]  # 7+21=28 short, 20+20=40 long
        short_counts = [7, 21, 0, 0]  # Rings 1,2 are short; 3,4 are long

        is_valid, error = is_valid_configuration(radii, counts, short_counts, self.inventory)
        self.assertTrue(is_valid, f"Should be valid: {error}")

    def test_mixed_pattern(self):
        """Test S-L-S-L pattern"""
        radii = [50, 100, 160, 220]
        counts = [7, 20, 21, 20]  # S:7+21=28, L:20+20=40
        short_counts = [7, 0, 21, 0]  # S-L-S-L

        is_valid, error = is_valid_configuration(radii, counts, short_counts, self.inventory)
        self.assertTrue(is_valid, f"Should be valid: {error}")


if __name__ == "__main__":
    unittest.main(verbosity=2)
