#!/usr/bin/env python3
"""Project Fox's baked blaster through a captured MVP and print its screen rect.

BUGS.md "Fox's pistol model is missing". `tris = 22 x draws` proves the overlay
submitted; it says nothing about where the triangles landed. This closes that
gap on the host, with no ROM build: feed it the sixteen words
`probe-fox-gun-matrix.ps1` dumps at `ndsRendererSubmitFoxGun` and it reports the
clip, NDC and screen-space bounds of the exact 44 vertices the submit pushes.

The arithmetic is the DS geometry engine's, not an approximation of it. A
`MTX_LOAD_4x4` matrix is 20.12 and `VTX_16` is 1.12. The production gun submit
does NOT load the captured CPU-composed matrix verbatim: before GX sees it,
`ndsRendererLoadHardwareRawComposedMatrix` calls
`ndsRendererBuildRawHardwareMatrix`, which divides the complete homogeneous
row 3 by `1 << NDS_RENDERER_HW_WORLD_UNIT_SHIFT` (256). This is the matching
half of the source-unit -> DS-v16 encoding used by the vertices. The engine then
evaluates

    clip[i] = (SUM_k  M[k][i] * v[k]) >> 12,     v = (x, y, z, 4096)

so the translation row contributes `M[3][i] * 4096 >> 12 == M[3][i]` while a
vertex contributes `M[0][i] * x >> 12`. Applying the row-3 normalization is
therefore mandatory when replaying a matrix dumped at the C entry point. The
old version of this script omitted it and kept reporting the already-fixed
0.036-pixel c128 failure after the renderer had moved to the normalized loader.

Pass --body with a second matrix (the fighter root's `composed_matrices[0]`,
dumped on the same frame) to get the like-for-like comparison: same camera,
same frame, same units, and the body is known to be visible.

    python scripts/fox_gun_screen_bounds.py --matrix "8358 1910 ... 15450092"
    python scripts/fox_gun_screen_bounds.py --from-probe artifacts/verification/x.txt
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

SOURCE = Path("src/nds/nds_fox_gun.c")
VERTEX_SCALE = 16          # NDS_FOX_GUN_VERTEX_SCALE
WORLD_UNIT_SHIFT = 8       # NDS_RENDERER_HW_WORLD_UNIT_SHIFT
SCREEN_W = 256
SCREEN_H = 192


def load_vertices(repo_root: Path) -> list[tuple[int, int, int]]:
    text = (repo_root / SOURCE).read_text(encoding="utf-8")
    block = text.split("sNdsFoxGunVertices[] = {", 1)
    if len(block) != 2:
        raise SystemExit(f"FAIL: no sNdsFoxGunVertices table in {SOURCE}")
    body = block[1].split("};", 1)[0]
    rows = re.findall(
        r"\{\s*\{\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*\}", body)
    if not rows:
        raise SystemExit("FAIL: could not parse any vertex rows")
    return [(int(a), int(b), int(c)) for a, b, c in rows]


def parse_matrix(words: list[int]) -> list[list[int]]:
    if len(words) != 16:
        raise SystemExit(f"FAIL: need 16 matrix words, got {len(words)}")
    return [words[0:4], words[4:8], words[8:12], words[12:16]]


def round_shift_signed(value: int, shift: int) -> int:
    """Mirror ndsRendererRoundShiftS64: nearest, halves away from zero."""
    if shift == 0:
        return value
    magnitude = -value if value < 0 else value
    magnitude = (magnitude + (1 << (shift - 1))) >> shift
    return -magnitude if value < 0 else magnitude


def raw_hardware_matrix(cpu_composed: list[list[int]]) -> list[list[int]]:
    """Mirror ndsRendererBuildRawHardwareMatrix for a captured gun MVP."""
    out = [row[:] for row in cpu_composed]
    out[3] = [round_shift_signed(v, WORLD_UNIT_SHIFT) for v in out[3]]
    return out


def transform(m: list[list[int]], v: tuple[int, int, int]) -> tuple[int, ...]:
    x, y, z = v
    out = []
    for col in range(4):
        acc = (m[0][col] * x + m[1][col] * y + m[2][col] * z +
               m[3][col] * 4096)
        out.append(acc >> 12)
    return tuple(out)


def report(label: str, m: list[list[int]],
           vertices: list[tuple[int, int, int]]) -> None:
    m = raw_hardware_matrix(m)
    scaled = [(x * VERTEX_SCALE, y * VERTEX_SCALE, z * VERTEX_SCALE)
              for x, y, z in vertices]
    clips = [transform(m, v) for v in scaled]
    origin = transform(m, (0, 0, 0))

    behind = sum(1 for c in clips if c[3] <= 0)
    print(f"[{label}]")
    print(f"  local origin clip  x={origin[0]} y={origin[1]} "
          f"z={origin[2]} w={origin[3]}")
    if origin[3] > 0:
        ox = (origin[0] / origin[3] + 1.0) * (SCREEN_W / 2)
        oy = (1.0 - origin[1] / origin[3]) * (SCREEN_H / 2)
        print(f"  local origin screen  x={ox:.2f} y={oy:.2f} "
              f"z_ndc={origin[2] / origin[3]:+.4f}")
    print(f"  vertices behind/at camera (w<=0): {behind}/{len(clips)}")
    if behind:
        return

    xs = [(c[0] / c[3] + 1.0) * (SCREEN_W / 2) for c in clips]
    ys = [(1.0 - c[1] / c[3]) * (SCREEN_H / 2) for c in clips]
    zs = [c[2] / c[3] for c in clips]
    print(f"  screen x  {min(xs):9.3f} .. {max(xs):9.3f}   "
          f"span {max(xs) - min(xs):.3f} px")
    print(f"  screen y  {min(ys):9.3f} .. {max(ys):9.3f}   "
          f"span {max(ys) - min(ys):.3f} px")
    print(f"  ndc z     {min(zs):+.5f} .. {max(zs):+.5f}")
    on = sum(1 for x, y in zip(xs, ys)
             if 0 <= x < SCREEN_W and 0 <= y < SCREEN_H)
    print(f"  corners inside the 256x192 viewport: {on}/{len(clips)}")
    span = max(max(xs) - min(xs), max(ys) - min(ys))
    verdict = "SUB-PIXEL -- cannot rasterize" if span < 1.0 else "rasterizable"
    print(f"  largest span {span:.3f} px  -> {verdict}")


def words_from_text(text: str) -> list[list[int]]:
    """Pull every `x/16dw` dump out of a probe capture, in order."""
    rows: list[int] = []
    blocks: list[list[int]] = []
    for line in text.splitlines():
        hit = re.match(r"^0x[0-9a-f]+\s*(<[^>]*>)?:\s*(.*)$", line.strip())
        if not hit:
            continue
        values = re.findall(r"-?\d+", hit.group(2))
        if len(values) != 4:
            continue
        rows.extend(int(v) for v in values)
        while len(rows) >= 16:
            blocks.append(rows[:16])
            rows = rows[16:]
    return blocks


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--matrix", help="16 whitespace-separated 20.12 words")
    parser.add_argument("--body", help="16 words for the fighter root, same frame")
    parser.add_argument("--from-probe", type=Path,
                        help="probe capture; every 16-word dump is reported")
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parent.parent
    vertices = load_vertices(repo_root)
    print(f"{len(vertices)} baked vertices, scale x{VERTEX_SCALE}")

    if args.from_probe is not None:
        text = args.from_probe.read_text(encoding="utf-8", errors="replace")
        blocks = words_from_text(text)
        if not blocks:
            raise SystemExit("FAIL: no 16-word dumps found in the capture")
        for index, block in enumerate(blocks):
            report(f"dump {index}", parse_matrix(block), vertices)
        return 0

    if args.matrix is None:
        parser.print_help()
        return 2
    report("gun", parse_matrix([int(v) for v in args.matrix.split()]), vertices)
    if args.body:
        report("body", parse_matrix([int(v) for v in args.body.split()]),
               vertices)
    return 0


if __name__ == "__main__":
    sys.exit(main())
