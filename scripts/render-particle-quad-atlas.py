#!/usr/bin/env python3
"""Render the shipped particle quad atlas the way the DS hardware reads it.

This decodes `assets/particles/efcommon_particle_quads.a5i3.bin` -- the exact
bytes NitroFS ships and `ndsRendererHardwarePrepareParticleAtlas` uploads -- and
writes one PNG per sheet plus a per-cell contact sheet. It is evidence for a
human, not another host-side re-derivation of the generator's own arithmetic:
the generator could be wrong about the asset and still agree with itself, so
this reads the FILE and the GENERATED TABLE and nothing else.

A3I5, one byte per texel: the low five bits index that SHEET's palette and the
high three are alpha. The palette block follows the texels, one table of
NDS_PARTICLE_QUAD_PALETTE_ENTRIES per sheet at NDS_PARTICLE_QUAD_PALETTE_STRIDE_BYTES
-- so reading sheet N with sheet 0's table, which is what the runtime did before
2026-08-14, is the one mistake this file is shaped to make visible.

Checkerboard behind the alpha, because a particle cell is mostly transparent and
"black" and "clear" are the two things that must not look alike here.
"""

from __future__ import annotations

import argparse
import re
import struct
import zlib
from pathlib import Path


def png(path: Path, width: int, height: int,
        pixels: list[tuple[int, int, int]]) -> None:
    """Minimal RGB8 PNG writer -- no Pillow dependency in this toolchain."""
    raw = bytearray()
    for y in range(height):
        raw.append(0)
        for x in range(width):
            raw.extend(pixels[y * width + x])

    def chunk(tag: bytes, payload: bytes) -> bytes:
        return (struct.pack(">I", len(payload)) + tag + payload +
                struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF))

    path.write_bytes(
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(bytes(raw), 9))
        + chunk(b"IEND", b""))


def defines(header: str) -> dict[str, int]:
    out = {}
    for name, value in re.findall(r"#define\s+(NDS_PARTICLE_QUAD_\w+)\s+(\d+)u",
                                  header):
        out[name] = int(value)
    return out


def quad_rows(inc: str) -> list[tuple[int, ...]]:
    body = re.search(
        r"gNdsParticleQuadFrames\[NDS_PARTICLE_QUAD_FRAME_COUNT\]\s*=\s*\{(.*?)\n\};",
        inc, re.S)
    if body is None:
        raise SystemExit("cannot find gNdsParticleQuadFrames in the .inc")
    return [tuple(int(n) for n in row)
            for row in re.findall(r"\{\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+),"
                                  r"\s*(\d+),\s*(\d+),\s*(\d+)\s*\}", body[1])]


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path,
                        default=Path(__file__).resolve().parents[1])
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--scale", type=int, default=4)
    args = parser.parse_args()

    root = args.root
    header = (root / "include/nds/generated/nds_particle_banks.generated.h"
              ).read_text(errors="replace")
    inc = (root / "src/nds/generated/nds_particle_banks.generated.inc"
           ).read_text(errors="replace")
    payload = (root / "assets/particles/efcommon_particle_quads.a5i3.bin"
               ).read_bytes()

    d = defines(header)
    width = d["NDS_PARTICLE_QUAD_ATLAS_WIDTH"]
    height = d["NDS_PARTICLE_QUAD_ATLAS_HEIGHT"]
    sheets = d["NDS_PARTICLE_QUAD_ATLAS_SHEETS"]
    entries = d["NDS_PARTICLE_QUAD_PALETTE_ENTRIES"]
    stride = d["NDS_PARTICLE_QUAD_PALETTE_STRIDE_BYTES"]
    pal_off = d["NDS_PARTICLE_QUAD_PALETTE_OFFSET"]
    asset_bytes = d["NDS_PARTICLE_QUAD_ASSET_BYTES"]
    sheet_bytes = width * height

    if len(payload) != asset_bytes:
        raise SystemExit(f"asset is {len(payload)} bytes, header says "
                         f"{asset_bytes}")
    if pal_off + sheets * stride != asset_bytes:
        raise SystemExit("palette block does not end at the asset's end")

    palettes = []
    for sheet in range(sheets):
        table = []
        for i in range(entries):
            v = struct.unpack_from("<H", payload, pal_off + sheet * stride + i * 2)[0]
            table.append((((v >> 0) & 31) * 255 // 31,
                          ((v >> 5) & 31) * 255 // 31,
                          ((v >> 10) & 31) * 255 // 31))
        palettes.append(table)

    args.output.mkdir(parents=True, exist_ok=True)
    scale = max(1, args.scale)
    for sheet in range(sheets):
        out = []
        for y in range(height):
            for x in range(width):
                byte = payload[sheet * sheet_bytes + y * width + x]
                alpha = ((byte >> 5) & 7) * 255 // 7
                red, green, blue = palettes[sheet][byte & 31]
                # Checkerboard so transparent never reads as black.
                back = 96 if ((x >> 3) + (y >> 3)) & 1 else 144
                out.append(tuple((c * alpha + back * (255 - alpha)) // 255
                                 for c in (red, green, blue)))
        big = []
        for y in range(height * scale):
            row = y // scale
            for x in range(width * scale):
                big.append(out[row * width + x // scale])
        png(args.output / f"sheet{sheet}.png", width * scale, height * scale, big)

    rows = quad_rows(inc)
    if len(rows) != d["NDS_PARTICLE_QUAD_FRAME_COUNT"]:
        raise SystemExit(f"{len(rows)} table rows, header says "
                         f"{d['NDS_PARTICLE_QUAD_FRAME_COUNT']}")
    for texture, frame, sheet, x, y, w, h in rows:
        if (sheet >= sheets) or (x + w > width) or (y + h > height):
            raise SystemExit(f"texture {texture} frame {frame} cell "
                             f"sheet {sheet} {x},{y} {w}x{h} leaves the sheet")
        cell = []
        for row in range(h):
            for column in range(w):
                byte = payload[sheet * sheet_bytes + (y + row) * width + x + column]
                alpha = ((byte >> 5) & 7) * 255 // 7
                red, green, blue = palettes[sheet][byte & 31]
                back = 96 if ((column >> 2) + (row >> 2)) & 1 else 144
                cell.append(tuple((c * alpha + back * (255 - alpha)) // 255
                                  for c in (red, green, blue)))
        big = []
        for row in range(h * scale):
            src = row // scale
            for column in range(w * scale):
                big.append(cell[src * w + column // scale])
        png(args.output / f"texture{texture:03d}_frame{frame}.png",
            w * scale, h * scale, big)

    print(f"quad atlas rendered: {sheets} sheets {width}x{height}, "
          f"{len(rows)} cells, {len(payload)} asset bytes, "
          f"{sheets} palettes x {entries} entries -> {args.output}")


if __name__ == "__main__":
    main()
