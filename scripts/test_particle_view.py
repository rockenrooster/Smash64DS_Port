"""Host proof for the Q12 particle view-center helper and its pixel budget.

Run with ``python scripts/test_particle_view.py``.  The C harness includes the
production header directly; Python supplies independent integer/rational
oracles and a perspective comparison for the previous world-space billboards.
"""

from __future__ import annotations

import math
import os
import shutil
import struct
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
HEADER_DIR = ROOT / "include" / "nds"
Q12 = 1 << 12
Q13 = 1 << 13
Q8 = 1 << 8
V16_PER_WORLD = 16
SCREEN_W = 256
SCREEN_H = 192


HARNESS = r"""
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nds_particle_view.h"

int main(void)
{
    unsigned bits;
    int32_t view[4][4];
    int32_t center[3];
    int32_t before[3];
    unsigned row;
    unsigned col;

    while (scanf("%u", &bits) == 1)
    {
        for (row = 0; row < 4; row++)
        {
            for (col = 0; col < 4; col++)
            {
                if (scanf("%" SCNd32, &view[row][col]) != 1) { return 2; }
            }
        }
        for (col = 0; col < 3; col++)
        {
            if (scanf("%" SCNd32, &center[col]) != 1) { return 2; }
        }
        memcpy(before, center, sizeof(center));
        {
            int ok = ndsParticleViewCenter(view, center, bits);
            int unchanged = (center[0] == before[0]) &&
                            (center[1] == before[1]) &&
                            (center[2] == before[2]);
            printf("%d %" PRId32 " %" PRId32 " %" PRId32 " %d\n",
                   ok, center[0], center[1], center[2], unchanged);
        }
    }
    return 0;
}
"""


def _trunc_div(value: int, divisor: int) -> int:
    """C signed division rounded toward zero, using integer-only arithmetic."""
    magnitude = abs(value) // divisor
    return -magnitude if value < 0 else magnitude


def _q12_floor(value: int) -> int:
    """The header's signed arithmetic ``>> 12`` result, stated independently."""
    return value // Q12


def center_oracle(view: tuple[tuple[int, ...], ...],
                  center: tuple[int, int, int], bits: int):
    """Exact Python-integer interpretation of the documented affine mapping."""
    if bits not in (8, 12):
        return None
    one = 1 << bits
    return tuple(
        _q12_floor(
            view[0][axis] * center[0]
            + view[1][axis] * center[1]
            + view[2][axis] * center[2]
            + view[3][axis] * one
        )
        for axis in range(3)
    )


def _identity_view() -> tuple[tuple[int, ...], ...]:
    return (
        (Q12, 0, 0, 0),
        (0, Q12, 0, 0),
        (0, 0, Q12, 0),
        (0, 0, 0, Q12),
    )


def _dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def _cross(a, b):
    return (
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    )


def _normalize(v):
    length = math.sqrt(_dot(v, v))
    if length == 0.0:
        raise ValueError("look-at basis has a zero vector")
    return tuple(value / length for value in v)


def _f32(value: float) -> float:
    """Round through binary32 as the source camera/particle inputs do."""
    return struct.unpack("<f", struct.pack("<f", value))[0]


def _trunc_fixed(value: float, bits: int) -> int:
    return math.trunc(_f32(value) * (1 << bits))


def look_at_q12(eye, target, up_hint=(0.0, 1.0, 0.0)):
    """Build a normalized row-vector affine view matrix, quantized to Q12."""
    forward = _normalize(tuple(t - e for e, t in zip(eye, target)))
    right = _normalize(_cross(forward, up_hint))
    up = _normalize(_cross(right, forward))
    back = tuple(-value for value in forward)
    axes = (right, up, back)
    # The renderer stores a row-vector matrix: each row names a source axis,
    # while each column names the resulting camera axis.
    rows = tuple(
        tuple(_trunc_fixed(axes[col][row], 12) for col in range(3)) + (0,)
        for row in range(3)
    )
    translation = tuple(
        _trunc_fixed(-_dot(eye, axis), 12) for axis in axes
    )
    matrix = rows + ((translation[0], translation[1], translation[2], Q12),)
    return matrix, right, up


def _project_view_v16(point_v16, fovy_degrees, aspect=4.0 / 3.0,
                      near=1.0, far=1000.0):
    """Project 12.4 view coordinates through a DS-sized perspective viewport."""
    x, y, z = (value / V16_PER_WORLD for value in point_v16)
    tangent = math.tan(math.radians(fovy_degrees) * 0.5)
    clip_x = x / (aspect * tangent)
    clip_y = y / tangent
    clip_z = -((far + near) / (far - near)) * z \
        - (2.0 * far * near / (far - near))
    clip_w = -z
    if clip_w <= 0.0:
        raise AssertionError(f"test point is behind the camera: {point_v16}")
    ndc_x = clip_x / clip_w
    ndc_y = clip_y / clip_w
    ndc_z = clip_z / clip_w
    return (
        (ndc_x + 1.0) * (SCREEN_W * 0.5),
        (1.0 - ndc_y) * (SCREEN_H * 0.5),
        ndc_z,
    )


def _old_world_quad_view_v16(view, center_q8, size_q8, right, up):
    """Model the established Q8/Q13 -> world V16 -> camera path."""
    right_q13 = tuple(_trunc_fixed(c, 13) for c in right)
    up_q13 = tuple(_trunc_fixed(c, 13) for c in up)
    right_leg_q8 = tuple(_trunc_div(c * size_q8, Q13) for c in right_q13)
    up_leg_q8 = tuple(_trunc_div(c * size_q8, Q13) for c in up_q13)
    corners = []
    for sx, sy in ((-1, -1), (1, -1), (1, 1), (-1, 1)):
        world_v16 = tuple(
            _trunc_div(center_q8[axis]
                       + sx * right_leg_q8[axis]
                       + sy * up_leg_q8[axis], 1 << 4)
            for axis in range(3)
        )
        view_v16 = tuple(
            _q12_floor(
                view[0][axis] * world_v16[0]
                + view[1][axis] * world_v16[1]
                + view[2][axis] * world_v16[2]
                + view[3][axis] * V16_PER_WORLD
            )
            for axis in range(3)
        )
        corners.append(view_v16)
    return corners


def _new_view_quad_v16(center_view_q8, size_q8):
    """Model center-only view transform followed by view-XY V16 corners."""
    corners = []
    for sx, sy in ((-1, -1), (1, -1), (1, 1), (-1, 1)):
        corner_q8 = (
            center_view_q8[0] + sx * size_q8,
            center_view_q8[1] + sy * size_q8,
            center_view_q8[2],
        )
        corners.append(tuple(_trunc_div(c, 1 << 4) for c in corner_q8))
    return corners


def _matrix_input(bits, view, center):
    flat = [value for row in view for value in row]
    return " ".join(str(value) for value in (bits, *flat, *center))


class ParticleViewHeaderTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = shutil.which("gcc") or shutil.which("clang")
        if compiler is None:
            raise unittest.SkipTest("host gcc/clang is required for the C header fixture")
        cls._temporary = tempfile.TemporaryDirectory(prefix="particle-view-")
        temp = Path(cls._temporary.name)
        source = temp / "particle_view_harness.c"
        cls.executable = temp / ("particle_view_harness.exe" if os.name == "nt"
                                 else "particle_view_harness")
        source.write_text(HARNESS, encoding="utf-8")
        result = subprocess.run(
            [compiler, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
             "-I", str(HEADER_DIR), str(source), "-o", str(cls.executable)],
            capture_output=True, text=True, check=False,
        )
        if result.returncode:
            raise AssertionError(
                f"host C header harness did not compile with {compiler}:\n"
                f"{result.stdout}{result.stderr}"
            )

    @classmethod
    def tearDownClass(cls):
        if hasattr(cls, "_temporary"):
            cls._temporary.cleanup()

    def run_c_cases(self, cases):
        input_text = "\n".join(_matrix_input(bits, view, center)
                                for bits, view, center in cases) + "\n"
        result = subprocess.run(
            [str(self.executable)], input=input_text, capture_output=True,
            text=True, check=False,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        lines = result.stdout.splitlines()
        self.assertEqual(len(lines), len(cases), result.stdout)
        parsed = []
        for line in lines:
            values = [int(value) for value in line.split()]
            self.assertEqual(len(values), 5, line)
            parsed.append((values[0], tuple(values[1:4]), values[4]))
        return parsed

    def assert_cases_match_oracle(self, cases):
        actual = self.run_c_cases(cases)
        for (bits, view, center), (ok, output, unchanged) in zip(cases, actual):
            expected = center_oracle(view, center, bits)
            if expected is None:
                self.assertEqual(ok, 0)
                self.assertEqual(output, center)
                self.assertEqual(unchanged, 1)
            else:
                self.assertEqual(ok, 1)
                self.assertEqual(output, expected)
                self.assertEqual(unchanged, int(output == center))

    def test_identity_and_camera_translation_for_q8_and_q12(self):
        identity = _identity_view()
        translated = (
            (Q12, 0, 0, 0),
            (0, Q12, 0, 0),
            (0, 0, Q12, 0),
            (14336, -29696, 512, Q12),
        )
        cases = [
            (8, identity, (15, -29, 400)),
            (12, identity, (17, -81, -4096)),
            (8, translated, (-25, 51, -128)),
            (12, translated, (-4096, 2048, -12345)),
        ]
        self.assert_cases_match_oracle(cases)

    def test_axis_swap_sign_and_oblique_quantized_look_at(self):
        swap = (
            (0, -Q12, 0, 0),
            (Q12, 0, 0, 0),
            (0, 0, Q12, 0),
            (0, 0, 0, Q12),
        )
        oblique, right, up = look_at_q12(
            (27.0, -13.0, 64.0), (-4.0, 6.0, 2.0)
        )
        self.assertTrue(any(0 < abs(v) < Q12 for row in oblique[:3]
                            for v in row[:3]))
        self.assertLess(abs(sum(x * x for x in right) - 1.0), 1e-12)
        self.assertLess(abs(sum(x * x for x in up) - 1.0), 1e-12)
        cases = [
            (8, swap, (-23, 45, -76)),
            (12, swap, (-1057, 2049, -3073)),
            (8, oblique, (-31 * Q8, 19 * Q8, -7 * Q8)),
            (12, oblique, (-31 * Q12, 19 * Q12, -7 * Q12)),
        ]
        self.assert_cases_match_oracle(cases)

    def test_overflow_and_invalid_fraction_leave_input_unchanged(self):
        overflow = (
            (Q12, 0, 0, 0),
            (0, Q12, 0, 0),
            (0, 0, 2 * Q12, 0),
            (0, 0, 0, Q12),
        )
        maximum = (0, 0, 2**31 - 1)
        invalid = [0, 7, 9, 13, 31, 32, 255]
        cases = [(12, overflow, maximum)] + [
            (bits, _identity_view(), (-123456, 789, -456)) for bits in invalid
        ]
        actual = self.run_c_cases(cases)
        for (bits, _view, center), (ok, output, unchanged) in zip(cases, actual):
            self.assertEqual(ok, 0, f"bits={bits} should reject")
            self.assertEqual(output, center, f"bits={bits} mutated input on failure")
            self.assertEqual(unchanged, 1)

    def test_view_xy_billboards_stay_within_one_pixel_under_perspective(self):
        cameras = (
            ((0.0, 0.0, 220.0), (0.0, 0.0, 0.0), 45.0),
            ((32.0, 18.0, 250.0), (0.0, 0.0, 0.0), 50.0),
            ((-60.0, 35.0, 170.0), (0.0, 0.0, 0.0), 55.0),
            ((0.0, 0.0, 22.0), (0.0, 0.0, 0.0), 45.0),
        )
        particles = (
            ((9.375, -4.5, 0.0), 0.75),
            ((-21.25, 12.125, -8.0), 3.5),
            ((38.0, -26.5, 19.25), 8.0),
        )
        cases = []
        details = []
        for eye, target, fovy in cameras:
            view, right, up = look_at_q12(eye, target)
            for position, size in particles:
                center_q8 = tuple(_trunc_fixed(c, 8) for c in position)
                size_q8 = _trunc_fixed(size, 8)
                cases.append((8, view, center_q8))
                details.append((view, right, up, center_q8, size_q8, fovy))

        transformed = self.run_c_cases(cases)
        worst_pixels = 0.0
        worst_depth = 0.0
        for detail, result in zip(details, transformed):
            view, right, up, center_q8, size_q8, fovy = detail
            ok, center_view_q8, unchanged = result
            self.assertEqual(ok, 1)
            self.assertEqual(unchanged, 0)
            old = _old_world_quad_view_v16(view, center_q8, size_q8,
                                           right, up)
            new = _new_view_quad_v16(center_view_q8, size_q8)
            for old_corner, new_corner in zip(old, new):
                old_screen = _project_view_v16(old_corner, fovy)
                new_screen = _project_view_v16(new_corner, fovy)
                worst_pixels = max(
                    worst_pixels,
                    math.hypot(old_screen[0] - new_screen[0],
                               old_screen[1] - new_screen[1]),
                )
                worst_depth = max(worst_depth, abs(old_screen[2] - new_screen[2]))

        print(
            "particle view-space quantization: max screen error "
            f"{worst_pixels:.6f}px (limit 1.0px); "
            f"max NDC depth error {worst_depth:.9g}; not bit-exact"
        )
        self.assertLessEqual(
            worst_pixels, 1.0,
            f"view-space conversion exceeded the 1-pixel budget: {worst_pixels:.6f}px",
        )


if __name__ == "__main__":
    unittest.main(verbosity=2)
