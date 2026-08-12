#!/usr/bin/env python3
"""Decode Fox's blaster model part from MiscData315 and emit the DS tables.

BUGS.md "Fox's pistol model is missing", draw half. The gun is model part 13 on
joint 17 (FTFOX_BLASTER_HOLD_JOINT); its geometry is a single display list in
reloc file 315, which the port already ships as a nitrofs asset.

The runtime does NOT walk this display list. It is 22 triangles that never
change, so walking G_VTX/G_TRI2 every frame would be pure per-frame cost for a
constant answer -- exactly what PROJECT_GOAL's "compute once, not every frame"
forbids. This script resolves the whole thing offline and prints the tables
`src/nds/nds_fox_gun.c` carries, the same way `nds_firegrind.c` carries its
offline-evaluated colour ramps and spawn cones.

Run it to REPRODUCE those tables, not to generate a build artifact:

    python scripts/fox_gun_bake.py --check src/nds/nds_fox_gun.c
    python scripts/fox_gun_bake.py            # print the tables

Layout, verified against the file's own header (see `--dump`):

    file id 0x13b, header 0x50 bytes, data_size 0x520
    data 0x008  palette   16 x RGBA5551  (already a DS format)
    data 0x030  texture   CI4 32x16      (already a DS format)
    data 0x130  Vtx[44]
    data 0x3F0  Gfx[38]   -> 2 x G_VTX (32 + 12) and 11 x G_TRI2 = 22 triangles

Nothing here is approximated: the DS's CI4 and RGBA5551 are the source formats
bit for bit, and the vertex payload is copied through unchanged. The only
conversion is texcoord scale, which is a shift.
"""

from __future__ import annotations

import argparse
import hashlib
import re
import struct
import sys
from pathlib import Path

ASSET = Path("decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/MiscData315")
ASSET_SHA256 = "2bb01cdd7c846c63b0946cae9b83c2a0d4ddd532f42921fe6eaece5c61b72cc7"

HEADER_BYTES = 0x50
OFF_PALETTE = 0x008
OFF_TEXTURE = 0x030
OFF_VERTICES = 0x130
OFF_DISPLAY_LIST = 0x3F0

PALETTE_ENTRIES = 16
VERTEX_COUNT = 44
COMMAND_COUNT = 38
TRIANGLE_COUNT = 22

G_VTX = 0x01
G_TRI2 = 0x06
G_SETTILESIZE = 0xF2
G_SETTILE = 0xF5
G_ENDDL = 0xDF


class BakeError(RuntimeError):
    """A deterministic falsifier: the asset is not what this bake assumes."""


def load_data(repo_root: Path) -> bytes:
    path = repo_root / ASSET
    if not path.is_file():
        raise BakeError(
            f"{ASSET} is absent. It is third-party reference data; rebuild the "
            "tree with scripts/fetch-battleship-reference.ps1."
        )
    raw = path.read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    if digest != ASSET_SHA256:
        raise BakeError(
            f"{ASSET} sha256 {digest} != pinned {ASSET_SHA256}; the tables below "
            "would not describe the shipped asset."
        )
    file_id, _, _, data_size = struct.unpack("<IIII", raw[0x40:0x50])
    if file_id != 0x13B:
        raise BakeError(f"file id 0x{file_id:x} != 0x13b")
    if HEADER_BYTES + data_size != len(raw):
        raise BakeError(
            f"header({HEADER_BYTES}) + data_size({data_size}) != file {len(raw)}"
        )
    return raw[HEADER_BYTES:HEADER_BYTES + data_size]


def decode_triangles(data: bytes) -> tuple[list[tuple[int, int, int]], list[int]]:
    """Walk the display list into a flat triangle list over Vtx[0..43].

    Each G_VTX names a destination slot window in the RSP vertex cache, so a
    G_TRI2 index is cache-relative. Both batches load at slot 0, and the second
    batch's source pointer is the tail of the same Vtx array, so the flat index
    is `batch_base + cache_index` where batch_base advances by the previous
    batch's count. Resolving this here is the whole reason the runtime needs no
    display list.
    """
    triangles: list[tuple[int, int, int]] = []
    batch_sizes: list[int] = []
    base = 0
    loaded = 0
    for index in range(COMMAND_COUNT):
        offset = OFF_DISPLAY_LIST + index * 8
        w0, _w1 = struct.unpack(">II", data[offset:offset + 8])
        opcode = w0 >> 24
        if opcode == G_VTX:
            count = (w0 >> 12) & 0xFF
            end_slot = (w0 >> 1) & 0x7F
            if end_slot != count:
                raise BakeError(
                    f"G_VTX at command {index} does not load at cache slot 0 "
                    f"(count {count}, end slot {end_slot}); the flat index "
                    "resolution below would be wrong."
                )
            base = loaded
            loaded += count
            batch_sizes.append(count)
        elif opcode == G_TRI2:
            corners = [
                (w0 >> 16) & 0xFF, (w0 >> 8) & 0xFF, w0 & 0xFF,
                (_w1 >> 16) & 0xFF, (_w1 >> 8) & 0xFF, _w1 & 0xFF,
            ]
            flat = [base + (corner // 2) for corner in corners]
            triangles.append((flat[0], flat[1], flat[2]))
            triangles.append((flat[3], flat[4], flat[5]))
        elif opcode == G_ENDDL:
            break
    if loaded != VERTEX_COUNT:
        raise BakeError(f"display list loads {loaded} vertices, expected {VERTEX_COUNT}")
    if len(triangles) != TRIANGLE_COUNT:
        raise BakeError(
            f"display list yields {len(triangles)} triangles, expected {TRIANGLE_COUNT}"
        )
    for triangle in triangles:
        for corner in triangle:
            if corner >= VERTEX_COUNT:
                raise BakeError(f"triangle corner {corner} out of range")
    return triangles, batch_sizes


def decode_texture_size(data: bytes) -> tuple[int, int]:
    """Read the CI4 extent out of the display list's own tile setup.

    CI4 makes 16x32 and 32x16 the same 256 bytes, so the payload length cannot
    tell them apart -- an earlier bake asserted 32x16 as a constant and got the
    transpose. Two independent commands carry the answer and both are required
    to agree here:

      G_SETTILE     maskS/maskT are log2 of the extent, and `line` is the row
                    stride in 64-bit words, which for CI4 is width/16.
      G_SETTILESIZE lrs/lrt are the inclusive lower-right corner in 10.2, so
                    the extent is (lrs >> 2) + 1.

    The tile that matters is the RENDER tile (tile 0), not the load tile 7 that
    G_LOADBLOCK uses to move bytes as 16-bit words.
    """
    tile_size: tuple[int, int] | None = None
    settile: tuple[int, int, int] | None = None

    for index in range(COMMAND_COUNT):
        offset = OFF_DISPLAY_LIST + index * 8
        w0, w1 = struct.unpack(">II", data[offset:offset + 8])
        opcode = w0 >> 24
        if opcode == G_SETTILE and ((w1 >> 24) & 0x7) == 0:
            settile = (
                (w1 >> 4) & 0xF,     # maskS
                (w1 >> 14) & 0xF,    # maskT
                (w0 >> 9) & 0x1FF,   # line, 64-bit words per row
            )
        elif opcode == G_SETTILESIZE and ((w1 >> 24) & 0x7) == 0:
            tile_size = (((w1 >> 12) & 0xFFF) >> 2) + 1, (w1 & 0xFFF) >> 2
            tile_size = (tile_size[0], tile_size[1] + 1)
        elif opcode == G_ENDDL:
            break

    if settile is None or tile_size is None:
        raise BakeError(
            "display list has no G_SETTILE/G_SETTILESIZE for render tile 0; "
            "the texture extent cannot be derived from source."
        )
    mask_s, mask_t, line = settile
    width, height = 1 << mask_s, 1 << mask_t
    if (width, height) != tile_size:
        raise BakeError(
            f"G_SETTILE says {width}x{height} but G_SETTILESIZE says "
            f"{tile_size[0]}x{tile_size[1]}"
        )
    if line * 16 != width:
        raise BakeError(
            f"G_SETTILE line {line} implies {line * 16} CI4 texels per row, "
            f"not {width}; the texture may not be 4bpp."
        )
    if (width * height) // 2 != 256:
        raise BakeError(
            f"{width}x{height} CI4 is {(width * height) // 2} bytes, not the "
            "256 the asset carries"
        )
    return width, height


def check_texcoords(vertices: list[tuple[int, ...]], width: int,
                    height: int) -> None:
    """Third, independent confirmation, from the geometry rather than the tile.

    Source Vtx texcoords are S10.5 over the texel grid, so the largest S is
    width * 32 and the largest T is height * 32. If the extent were transposed
    these two would swap, which is precisely the failure this guards.
    """
    max_s = max(v[3] for v in vertices)
    max_t = max(v[4] for v in vertices)
    if max_s > width * 32 or max_t > height * 32:
        raise BakeError(
            f"texcoords reach s={max_s} t={max_t}, past {width}x{height} "
            f"(s<={width * 32}, t<={height * 32}); the extent is transposed."
        )


def decode_vertices(data: bytes) -> list[tuple[int, ...]]:
    vertices = []
    for index in range(VERTEX_COUNT):
        offset = OFF_VERTICES + index * 16
        x, y, z, flag, s, t, nx, ny, nz, alpha = struct.unpack(
            ">hhhHhhbbbB", data[offset:offset + 16]
        )
        if flag != 0:
            raise BakeError(f"vertex {index} carries flag 0x{flag:04x}, expected 0")
        vertices.append((x, y, z, s, t, nx, ny, nz, alpha))
    return vertices


def emit(data: bytes) -> str:
    triangles, batch_sizes = decode_triangles(data)
    vertices = decode_vertices(data)
    width, height = decode_texture_size(data)
    check_texcoords(vertices, width, height)
    palette = struct.unpack(">16H", data[OFF_PALETTE:OFF_PALETTE + PALETTE_ENTRIES * 2])
    texels = data[OFF_TEXTURE:OFF_TEXTURE + (width * height) // 2]

    out: list[str] = []
    out.append(f"/* batches {batch_sizes}, {len(triangles)} triangles */")
    out.append("static const NDSFoxGunVertex sNdsFoxGunVertices[] = {")
    for x, y, z, s, t, nx, ny, nz, _alpha in vertices:
        out.append(
            f"    {{ {{ {x:5d}, {y:5d}, {z:5d} }}, {{ {s:5d}, {t:5d} }}, "
            f"{{ {nx:4d}, {ny:4d}, {nz:4d} }} }},"
        )
    out.append("};")
    out.append("")
    out.append("static const u8 sNdsFoxGunTriangles[][3] = {")
    for a, b, c in triangles:
        out.append(f"    {{ {a:2d}, {b:2d}, {c:2d} }},")
    out.append("};")
    out.append("")
    # RGBA5551 on N64 is rrrrrggg ggbbbbba; the DS wants abbbbbgg gggrrrrr.
    out.append("static const u16 sNdsFoxGunPalette[] = {")
    row: list[str] = []
    for value in palette:
        red = (value >> 11) & 0x1F
        green = (value >> 6) & 0x1F
        blue = (value >> 1) & 0x1F
        alpha = value & 1
        ds = (alpha << 15) | (blue << 10) | (green << 5) | red
        row.append(f"0x{ds:04x}")
        if len(row) == 8:
            out.append("    " + ", ".join(row) + ",")
            row = []
    if row:
        out.append("    " + ", ".join(row) + ",")
    out.append("};")
    out.append("")
    out.append(f"/* CI4 {width}x{height}, source order preserved. */")
    out.append("static const u8 sNdsFoxGunTexels[] = {")
    for start in range(0, len(texels), 16):
        chunk = texels[start:start + 16]
        out.append("    " + ", ".join(f"0x{byte:02x}" for byte in chunk) + ",")
    out.append("};")
    return "\n".join(out)


def check(source_path: Path, expected: str) -> int:
    if not source_path.is_file():
        print(f"FAIL: {source_path} does not exist")
        return 1
    text = source_path.read_text(encoding="utf-8")
    missing = []
    for block in ("sNdsFoxGunVertices", "sNdsFoxGunTriangles",
                  "sNdsFoxGunPalette", "sNdsFoxGunTexels"):
        if block not in text:
            missing.append(block)
    if missing:
        print("FAIL: " + source_path.name + " is missing " + ", ".join(missing))
        return 1
    normalized_source = re.sub(r"\s+", "", text)
    for line in expected.splitlines():
        stripped = re.sub(r"\s+", "", line)
        if not stripped or stripped.startswith("/*") or stripped.startswith("static"):
            continue
        if stripped.endswith("};"):
            continue
        if stripped not in normalized_source:
            print(f"FAIL: baked line absent from {source_path.name}: {line.strip()}")
            return 1
    print("FOX_GUN_BAKE=PASS  every decoded row is present in " + source_path.name)
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", type=Path, default=None,
                        help="verify a C file already carries these tables")
    parser.add_argument("--dump", action="store_true",
                        help="print the decoded display list instead of the tables")
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parent.parent
    try:
        data = load_data(repo_root)
    except BakeError as error:
        print(f"FAIL: {error}")
        return 1

    if args.dump:
        for index in range(COMMAND_COUNT):
            offset = OFF_DISPLAY_LIST + index * 8
            w0, w1 = struct.unpack(">II", data[offset:offset + 8])
            print(f"{index:2d}  {w0:08x} {w1:08x}  op={w0 >> 24:02x}")
        return 0

    try:
        tables = emit(data)
    except BakeError as error:
        print(f"FAIL: {error}")
        return 1

    if args.check is not None:
        return check(args.check, tables)
    print(tables)
    return 0


if __name__ == "__main__":
    sys.exit(main())
