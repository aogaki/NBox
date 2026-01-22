#!/usr/bin/env python3
"""
Geometry Generator for NBox Optimization
Generates JSON configuration files for detector placements
Supports multiple detector types with count constraints
"""

import json
import math
from typing import List, Tuple, Dict, Any, Optional
from dataclasses import dataclass


# Physical constants [mm]
# These values are based on the ELIGANT-TN detector specifications
DETECTOR_DIAMETER = 25.4  # mm (1 inch) - standard He-3 tube diameter
MIN_GAP = 5.0             # mm - minimum gap between detector surfaces
BEAM_PIPE_RADIUS = 22.0   # mm - beam pipe outer radius (diameter = 44mm)
BOX_HALF_WIDTH = 500.0    # mm - half of 1m cubic moderator box


@dataclass
class DetectorInventory:
    """
    Available detector inventory with count constraints.

    Default values (28 short + 40 long = 68 total) represent the actual
    ELIGANT-TN He-3 detector inventory available for the experiment.
    These are physical constraints based on available hardware.
    """
    short_type: str = "He3_ELIGANT"       # Short detector type name
    short_count: int = 28                  # Available short detectors (ELIGANT-TN inventory)
    long_type: str = "He3_ELIGANT_Long"   # Long detector type name
    long_count: int = 40                   # Available long detectors (ELIGANT-TN inventory)

    def total_available(self) -> int:
        return self.short_count + self.long_count


def is_valid_configuration(radii: List[float], counts: List[int],
                          short_counts: Optional[List[int]] = None,
                          inventory: Optional[DetectorInventory] = None) -> Tuple[bool, str]:
    """
    Check if detector configuration is physically valid

    Parameters:
    -----------
    radii : list of float
        Ring radii [r1, r2, r3, r4] in mm
    counts : list of int
        Total number of detectors per ring [n1, n2, n3, n4]
    short_counts : list of int, optional
        Number of short detectors per ring (rest are long)
    inventory : DetectorInventory, optional
        Detector inventory constraints

    Returns:
    --------
    tuple (bool, str)
        (is_valid, error_message)
    """
    if len(radii) != len(counts):
        return False, "Radii and counts must have same length"

    n_rings = len(radii)

    # Check beam pipe clearance
    if radii[0] - DETECTOR_DIAMETER / 2 < BEAM_PIPE_RADIUS:
        return False, f"Ring 1 (r={radii[0]:.1f}mm) too close to beam pipe"

    # Check box boundary
    if radii[-1] + DETECTOR_DIAMETER / 2 > BOX_HALF_WIDTH:
        return False, f"Outer ring (r={radii[-1]:.1f}mm) exceeds box boundary"

    # Check ring spacing
    for i in range(n_rings - 1):
        if radii[i+1] - radii[i] < DETECTOR_DIAMETER + MIN_GAP:
            return False, f"Rings {i+1} and {i+2} too close: {radii[i+1] - radii[i]:.1f}mm < {DETECTOR_DIAMETER + MIN_GAP}mm"

    # Check detector spacing within each ring
    for i, (r, n) in enumerate(zip(radii, counts)):
        if n <= 0:
            continue
        circumference = 2 * math.pi * r
        spacing = circumference / n
        if spacing < DETECTOR_DIAMETER + MIN_GAP:
            return False, f"Ring {i+1}: spacing {spacing:.1f}mm < {DETECTOR_DIAMETER + MIN_GAP}mm"

    # Check detector inventory constraints
    if short_counts is not None and inventory is not None:
        total_short = sum(short_counts)
        total_long = sum(counts) - total_short

        if total_short > inventory.short_count:
            return False, f"Short detectors ({total_short}) exceed inventory ({inventory.short_count})"
        if total_long > inventory.long_count:
            return False, f"Long detectors ({total_long}) exceed inventory ({inventory.long_count})"

        # Check that short_counts don't exceed total counts
        for i, (n_total, n_short) in enumerate(zip(counts, short_counts)):
            if n_short > n_total:
                return False, f"Ring {i+1}: short count ({n_short}) > total count ({n_total})"
            if n_short < 0:
                return False, f"Ring {i+1}: negative short count ({n_short})"

    return True, ""


def generate_ring_placements(radius: float, n_short: int, n_long: int,
                             ring_id: int,
                             short_type: str = "He3_ELIGANT",
                             long_type: str = "He3_ELIGANT_Long") -> List[Dict[str, Any]]:
    """
    Generate detector placements for a single ring with mixed detector types

    Parameters:
    -----------
    radius : float
        Ring radius in mm
    n_short : int
        Number of short detectors in the ring
    n_long : int
        Number of long detectors in the ring
    ring_id : int
        Ring identifier (1, 2, 3, ...)
    short_type : str
        Short detector type name
    long_type : str
        Long detector type name

    Returns:
    --------
    list of dict
        Placement configurations
    """
    placements = []
    n_total = n_short + n_long

    if n_total == 0:
        return placements

    angle_step = 360.0 / n_total

    # Ring names: A, B, C, D, ...
    ring_name = chr(ord('A') + ring_id - 1)

    # Interleave short and long detectors for uniform distribution
    # Short detectors first, then long detectors
    for i in range(n_total):
        angle = i * angle_step

        # Determine detector type: first n_short are short, rest are long
        if i < n_short:
            det_type = short_type
        else:
            det_type = long_type

        placement = {
            "name": f"{ring_name}{i + 1}",
            "type": det_type,
            "R": round(radius, 2),
            "Phi": round(angle, 2)
        }
        placements.append(placement)

    return placements


def generate_geometry_config(radii: List[float], counts: List[int],
                            short_counts: Optional[List[int]] = None,
                            inventory: Optional[DetectorInventory] = None,
                            box_size: Tuple[float, float, float] = (1000, 1000, 1000),
                            beam_pipe_diameter: float = 44) -> Dict[str, Any]:
    """
    Generate complete geometry configuration with mixed detector types

    Parameters:
    -----------
    radii : list of float
        Ring radii [r1, r2, ...] in mm
    counts : list of int
        Total number of detectors per ring [n1, n2, ...]
    short_counts : list of int, optional
        Number of short detectors per ring (rest are long)
        If None, all detectors are short type
    inventory : DetectorInventory, optional
        Detector inventory for type names
    box_size : tuple
        Moderator box size (x, y, z) in mm
    beam_pipe_diameter : float
        Beam pipe diameter in mm

    Returns:
    --------
    dict
        Complete geometry configuration
    """
    if inventory is None:
        inventory = DetectorInventory()

    # If short_counts not specified, use all short detectors
    if short_counts is None:
        short_counts = counts.copy()

    # Validate configuration
    is_valid, error = is_valid_configuration(radii, counts, short_counts, inventory)
    if not is_valid:
        raise ValueError(f"Invalid configuration: {error}")

    # Generate placements for all rings
    all_placements = []
    for ring_id, (r, n_total, n_short) in enumerate(zip(radii, counts, short_counts), start=1):
        n_long = n_total - n_short
        placements = generate_ring_placements(
            r, n_short, n_long, ring_id,
            inventory.short_type, inventory.long_type
        )
        all_placements.extend(placements)

    config = {
        "Box": {
            "Type": "Box",
            "x": box_size[0],
            "y": box_size[1],
            "z": box_size[2],
            "BeamPipe": beam_pipe_diameter
        },
        "Placements": all_placements
    }

    return config


def save_geometry_config(config: Dict[str, Any], filepath: str) -> None:
    """Save geometry configuration to JSON file"""
    with open(filepath, 'w') as f:
        json.dump(config, f, indent=2)


def total_detectors(counts: List[int]) -> int:
    """Calculate total number of detectors"""
    return sum(counts)


def summarize_config(counts: List[int], short_counts: List[int]) -> Dict[str, int]:
    """Summarize detector configuration"""
    total_short = sum(short_counts)
    total_long = sum(counts) - total_short
    return {
        "total": sum(counts),
        "short": total_short,
        "long": total_long
    }


if __name__ == "__main__":
    # Example: Generate a 4-ring configuration with mixed detector types
    inventory = DetectorInventory(
        short_type="He3_ELIGANT",
        short_count=28,
        long_type="He3_ELIGANT_Long",
        long_count=40
    )

    radii = [59.0, 110.0, 155.0, 195.0]
    counts = [8, 16, 20, 24]        # Total per ring
    short_counts = [4, 8, 10, 6]    # Short detectors per ring (rest are long)

    print(f"Generating {len(radii)}-ring configuration:")
    print(f"  Radii: {radii} mm")
    print(f"  Total counts: {counts}")
    print(f"  Short counts: {short_counts}")
    print(f"  Long counts: {[c - s for c, s in zip(counts, short_counts)]}")

    summary = summarize_config(counts, short_counts)
    print(f"  Summary: {summary['total']} total ({summary['short']} short, {summary['long']} long)")
    print(f"  Inventory: {inventory.short_count} short, {inventory.long_count} long available")

    is_valid, error = is_valid_configuration(radii, counts, short_counts, inventory)
    if is_valid:
        config = generate_geometry_config(radii, counts, short_counts, inventory)
        save_geometry_config(config, "test_geometry.json")
        print(f"  Configuration saved to test_geometry.json")
    else:
        print(f"  ERROR: {error}")
