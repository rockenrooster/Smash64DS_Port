#!/usr/bin/env python3
"""Cheap host checks for the VS Results OAM tiler and palette ramp."""

from __future__ import annotations

import itertools
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "src" / "nds" / "nds_results_oam.c"
MAX_TILES = 4


def source_shapes(text: str) -> list[tuple[int, int]]:
    start = text.index("static const NDSResultsOamShape sNdsResultsShapes[]")
    end = text.index("};", start)
    rows = re.findall(
        r"\{\s*(\d+)u,\s*(\d+)u,\s*SpriteSize_[^}]+\}", text[start:end]
    )
    return [(int(width), int(height)) for width, height in rows]


def choose_plan(
    shapes: list[tuple[int, int]], width: int, height: int
) -> tuple[tuple[int, int], ...] | None:
    for count in range(1, MAX_TILES + 1):
        best: tuple[tuple[int, int], ...] | None = None
        best_area = 0xFFFFFFFF
        candidates = [shape for shape in shapes if shape[1] >= height]
        for plan in itertools.product(candidates, repeat=count):
            if sum(shape[0] for shape in plan) < width:
                continue
            area = sum(shape[0] * shape[1] for shape in plan)
            if area < best_area:
                best = plan
                best_area = area
        if best is not None:
            return best
    return None


def rgb15(red: int, green: int, blue: int) -> int:
    return 0x8000 | (red >> 3) | ((green >> 3) << 5) | ((blue >> 3) << 10)


def palette_ramp(
    prim: tuple[int, int, int], env: tuple[int, int, int]
) -> list[int]:
    result = [0]
    for index in range(1, 16):
        step = index - 1
        inv = 14 - step
        rgb = tuple(
            (env[channel] * inv + prim[channel] * step + 7) // 14
            for channel in range(3)
        )
        result.append(rgb15(*rgb))
    return result


def main() -> int:
    text = SOURCE.read_text(encoding="utf-8")
    shapes = source_shapes(text)
    expected_shapes = {
        (8, 8), (16, 16), (32, 32), (64, 64),
        (16, 8), (32, 8), (32, 16), (64, 32),
        (8, 16), (8, 32), (16, 32), (32, 64),
    }
    assert set(shapes) == expected_shapes, shapes
    assert (64, 16) not in shapes

    cases = {
        (53, 11): ((64, 32),),
        (71, 15): ((64, 32), (8, 16)),
        (96, 10): ((64, 32), (32, 16)),
        (33, 32): ((64, 32),),
        (36, 31): ((64, 32),),
        (257, 16): None,
    }
    for dimensions, expected in cases.items():
        got = choose_plan(shapes, *dimensions)
        assert got == expected, f"{dimensions}: {got} != {expected}"
    print(f"PASS: Results OAM tiler ({len(cases)} cases)")

    ramps = [
        ((255, 255, 255), (0, 0, 0)),
        ((255, 64, 32), (8, 16, 24)),
        ((0, 128, 255), (255, 32, 0)),
    ]
    assert "u32 step = index - 1u;" in text
    assert "u32 inv = 14u - step;" in text
    assert "/ 14u" in text
    for prim, env in ramps:
        ramp = palette_ramp(prim, env)
        assert ramp[0] == 0
        assert ramp[1] == rgb15(*env)
        assert ramp[15] == rgb15(*prim)
        assert all((entry & 0x8000) != 0 for entry in ramp[1:])
    print(f"PASS: Results OAM palette ramp ({len(ramps)} ramps)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
