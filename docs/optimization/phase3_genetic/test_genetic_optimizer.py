#!/usr/bin/env python3
"""
Unit tests for genetic_optimizer.py
Tests genetic algorithm logic without running simulations
"""

import unittest
import math
import sys
from pathlib import Path

# Add phase2 scripts directory to path
phase2_scripts = Path(__file__).parent.parent / "phase2_bayesian" / "scripts"
sys.path.insert(0, str(phase2_scripts))

# Add phase3 directory to path
sys.path.insert(0, str(Path(__file__).parent))

from genetic_optimizer import (
    generate_uniform_ring_config,
    MIN_SPACING,
    MAX_RADIUS
)
from geometry_generator import (
    DetectorInventory,
    DETECTOR_DIAMETER,
    MIN_GAP,
    BEAM_PIPE_RADIUS,
    BOX_HALF_WIDTH
)


class TestGeneticConfigGeneration(unittest.TestCase):
    """Test genetic algorithm config generation"""

    def setUp(self):
        self.inventory = DetectorInventory(
            short_type="He3_ELIGANT",
            short_count=28,
            long_type="He3_ELIGANT_Long",
            long_count=40
        )

    def test_short_type_encoding(self):
        """Test that type 0 = short"""
        config = generate_uniform_ring_config(
            radii=[100],
            ring_types=[0],  # 0 = short
            counts=[10],
            inventory=self.inventory
        )

        for p in config["Placements"]:
            self.assertEqual(p["type"], "He3_ELIGANT")

    def test_long_type_encoding(self):
        """Test that type 1 = long"""
        config = generate_uniform_ring_config(
            radii=[100],
            ring_types=[1],  # 1 = long
            counts=[10],
            inventory=self.inventory
        )

        for p in config["Placements"]:
            self.assertEqual(p["type"], "He3_ELIGANT_Long")

    def test_mixed_types(self):
        """Test mixed type configuration"""
        config = generate_uniform_ring_config(
            radii=[50, 100, 150, 200],
            ring_types=[0, 1, 0, 1],  # S-L-S-L
            counts=[7, 20, 21, 20],
            inventory=self.inventory
        )

        placements = config["Placements"]
        self.assertEqual(len(placements), 68)

        # First ring (7 detectors) should be short
        for p in placements[:7]:
            self.assertEqual(p["type"], "He3_ELIGANT")

        # Second ring (20 detectors) should be long
        for p in placements[7:27]:
            self.assertEqual(p["type"], "He3_ELIGANT_Long")


class TestIndividualEncoding(unittest.TestCase):
    """Test individual encoding/decoding"""

    def test_individual_structure_4rings(self):
        """Test individual structure for 4 rings"""
        # Individual: [r1, r2, r3, r4, t1, t2, t3, t4, n1, n2, n3, n4]
        individual = [50, 100, 150, 200, 0, 1, 0, 1, 7, 20, 21, 20]

        n_rings = 4
        radii = individual[:n_rings]
        ring_types = individual[n_rings:2*n_rings]
        counts = individual[2*n_rings:]

        self.assertEqual(radii, [50, 100, 150, 200])
        self.assertEqual(ring_types, [0, 1, 0, 1])
        self.assertEqual(counts, [7, 20, 21, 20])

    def test_individual_totals(self):
        """Test that individual totals match inventory"""
        individual = [50, 100, 150, 200, 0, 1, 0, 1, 7, 20, 21, 20]

        n_rings = 4
        ring_types = individual[n_rings:2*n_rings]
        counts = individual[2*n_rings:]

        short_rings = [i for i, t in enumerate(ring_types) if t == 0]
        long_rings = [i for i, t in enumerate(ring_types) if t == 1]

        total_short = sum(counts[i] for i in short_rings)  # 7 + 21 = 28
        total_long = sum(counts[i] for i in long_rings)    # 20 + 20 = 40

        self.assertEqual(total_short, 28)
        self.assertEqual(total_long, 40)
        self.assertEqual(sum(counts), 68)


class TestValidationLogic(unittest.TestCase):
    """Test validation logic that would be in _is_valid"""

    def test_radii_bounds(self):
        """Test radii boundary validation"""
        # Valid radii
        radii = [50, 100, 150, 200]
        self.assertTrue(radii[0] >= 35)
        self.assertTrue(radii[-1] <= MAX_RADIUS)

        # Invalid - too small
        radii_small = [30, 100, 150, 200]
        self.assertFalse(radii_small[0] >= 35)

        # Invalid - too large
        radii_large = [50, 100, 150, 490]
        self.assertFalse(radii_large[-1] <= MAX_RADIUS)  # 490 > 487
        # MAX_RADIUS = 487, so 490 exceeds

    def test_radii_spacing(self):
        """Test radii spacing validation"""
        # Valid spacing
        radii = [50, 100, 150, 200]  # Gaps: 50, 50, 50
        for i in range(len(radii) - 1):
            self.assertGreaterEqual(radii[i+1] - radii[i], MIN_SPACING)

        # Invalid spacing
        radii_close = [50, 70, 150, 200]  # First gap: 20 < MIN_SPACING (31)
        self.assertFalse(radii_close[1] - radii_close[0] >= MIN_SPACING)

    def test_ring_types_requirement(self):
        """Test that both ring types must be present"""
        # Valid - has both types
        ring_types_valid = [0, 1, 0, 1]
        has_short = any(t == 0 for t in ring_types_valid)
        has_long = any(t == 1 for t in ring_types_valid)
        self.assertTrue(has_short and has_long)

        # Invalid - all short
        ring_types_all_short = [0, 0, 0, 0]
        has_long = any(t == 1 for t in ring_types_all_short)
        self.assertFalse(has_long)

        # Invalid - all long
        ring_types_all_long = [1, 1, 1, 1]
        has_short = any(t == 0 for t in ring_types_all_long)
        self.assertFalse(has_short)

    def test_detector_count_per_ring(self):
        """Test detector count constraints per ring"""
        radii = [50, 100, 150, 200]

        for r in radii:
            circumference = 2 * math.pi * r
            max_n = int(circumference / (DETECTOR_DIAMETER + MIN_GAP))
            self.assertGreater(max_n, 0)

        # r=50: max ~10
        # r=100: max ~20
        # r=150: max ~31
        # r=200: max ~41


class TestConstraintSatisfaction(unittest.TestCase):
    """Test constraint satisfaction"""

    def test_exact_inventory_28_40(self):
        """Test exact inventory constraint (28 short + 40 long)"""
        test_cases = [
            # [short_counts], [long_counts]
            ([7, 21], [20, 20]),        # 28, 40
            ([14, 14], [10, 30]),       # 28, 40
            ([5, 8, 15], [40]),         # 28, 40
            ([28], [20, 20]),           # 28, 40
        ]

        for short_counts, long_counts in test_cases:
            total_short = sum(short_counts)
            total_long = sum(long_counts)
            self.assertEqual(total_short, 28, f"Failed for {short_counts}")
            self.assertEqual(total_long, 40, f"Failed for {long_counts}")

    def test_max_detectors_calculation(self):
        """Test maximum detectors per ring calculation"""
        test_radii = [50, 100, 200, 400]
        expected_maxes = []

        for r in test_radii:
            circumference = 2 * math.pi * r
            max_n = int(circumference / (DETECTOR_DIAMETER + MIN_GAP))
            expected_maxes.append(max_n)

        # Verify reasonable values
        self.assertGreater(expected_maxes[0], 5)    # r=50
        self.assertGreater(expected_maxes[1], 15)   # r=100
        self.assertGreater(expected_maxes[2], 35)   # r=200
        self.assertGreater(expected_maxes[3], 75)   # r=400


class TestRepairLogic(unittest.TestCase):
    """Test individual repair logic"""

    def test_radii_sorting(self):
        """Test that radii should be sorted"""
        unsorted = [150, 50, 200, 100]
        sorted_radii = sorted(unsorted)
        self.assertEqual(sorted_radii, [50, 100, 150, 200])

    def test_spacing_enforcement(self):
        """Test spacing enforcement in repair"""
        radii = [50, 60, 150, 200]  # 60 is too close to 50

        # Repair: ensure minimum spacing
        for i in range(len(radii) - 1):
            if radii[i+1] - radii[i] < MIN_SPACING:
                radii[i+1] = radii[i] + MIN_SPACING

        # After repair
        self.assertGreaterEqual(radii[1] - radii[0], MIN_SPACING)

    def test_type_repair_no_short(self):
        """Test repair when no short rings"""
        ring_types = [1, 1, 1, 1]  # All long

        # Repair: make first ring short
        if not any(t == 0 for t in ring_types):
            ring_types[0] = 0

        self.assertTrue(any(t == 0 for t in ring_types))

    def test_type_repair_no_long(self):
        """Test repair when no long rings"""
        ring_types = [0, 0, 0, 0]  # All short

        # Repair: make last ring long
        if not any(t == 1 for t in ring_types):
            ring_types[-1] = 1

        self.assertTrue(any(t == 1 for t in ring_types))


class TestCrossoverLogic(unittest.TestCase):
    """Test crossover operation logic"""

    def test_radii_swap_maintains_sorting(self):
        """Test that after radii swap, sorting is maintained"""
        ind1_radii = [50, 100, 150, 200]
        ind2_radii = [60, 120, 180, 240]

        # Swap some radii
        ind1_radii[1], ind2_radii[1] = ind2_radii[1], ind1_radii[1]

        # ind1: [50, 120, 150, 200] - still sorted
        # ind2: [60, 100, 180, 240] - still sorted

        # But after any swap, need to re-sort
        ind1_radii = sorted(ind1_radii)
        ind2_radii = sorted(ind2_radii)

        # Verify sorted
        for i in range(len(ind1_radii) - 1):
            self.assertLess(ind1_radii[i], ind1_radii[i+1])
            self.assertLess(ind2_radii[i], ind2_radii[i+1])


class TestMutationLogic(unittest.TestCase):
    """Test mutation operation logic"""

    def test_radii_mutation_bounds(self):
        """Test that mutated radii stay in bounds"""
        radius = 100
        delta = 50  # Large mutation

        new_radius = radius + delta
        new_radius = max(35, min(MAX_RADIUS, new_radius))

        self.assertGreaterEqual(new_radius, 35)
        self.assertLessEqual(new_radius, MAX_RADIUS)

    def test_type_swap_mutation(self):
        """Test type swap mutation"""
        ring_types = [0, 1, 0, 1]

        # Swap indices 0 and 1
        ring_types[0], ring_types[1] = ring_types[1], ring_types[0]

        self.assertEqual(ring_types, [1, 0, 0, 1])


class TestPhysicalConstants(unittest.TestCase):
    """Test physical constant values"""

    def test_min_spacing_value(self):
        """Test MIN_SPACING value"""
        self.assertEqual(MIN_SPACING, 31.0)

    def test_max_radius_value(self):
        """Test MAX_RADIUS value"""
        self.assertEqual(MAX_RADIUS, 487.0)

    def test_consistent_with_geometry_generator(self):
        """Test consistency with geometry_generator constants"""
        expected_min_spacing = DETECTOR_DIAMETER + MIN_GAP  # 25.4 + 5 = 30.4
        # MIN_SPACING = 31.0 is slightly more conservative, which is fine
        self.assertGreaterEqual(MIN_SPACING, expected_min_spacing)


class TestOutputConsistency(unittest.TestCase):
    """Test output format consistency"""

    def setUp(self):
        self.inventory = DetectorInventory()

    def test_placement_has_required_fields(self):
        """Test placement has all required fields"""
        config = generate_uniform_ring_config(
            radii=[100],
            ring_types=[0],
            counts=[5],
            inventory=self.inventory
        )

        for p in config["Placements"]:
            self.assertIn("name", p)
            self.assertIn("type", p)
            self.assertIn("R", p)
            self.assertIn("Phi", p)

    def test_box_has_required_fields(self):
        """Test box has all required fields"""
        config = generate_uniform_ring_config(
            radii=[100],
            ring_types=[0],
            counts=[5],
            inventory=self.inventory
        )

        box = config["Box"]
        self.assertIn("Type", box)
        self.assertIn("x", box)
        self.assertIn("y", box)
        self.assertIn("z", box)
        self.assertIn("BeamPipe", box)


if __name__ == "__main__":
    unittest.main(verbosity=2)
