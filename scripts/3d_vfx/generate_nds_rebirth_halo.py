#!/usr/bin/env python3
"""Generate the immutable DS-native RebirthHalo mesh and texture payloads.

The source of truth is BattleShip EFCommonEffects3 (file 85).  Geometry is
flattened in source triangle order after resolving the tiny wrapper-DL graph;
CI4 textures are sampled through the same fast+oracle converter used by the
battle static-texture pipeline, then losslessly repacked as DS PAL16.  The one
I4 beam is converted to DS A5I3 because its texel intensity is source alpha.

Nothing generated here requires N64 vertex, display-list, texture or TLUT
decoding at runtime.
"""

from __future__ import annotations

import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))

import generate_battle_playable_texture_census as census
import generate_battle_playable_static_textures as static

SOURCE = ROOT / "decomp/BattleShip-main/decomp/assets/us/relocData/85.vpk0.bin"
O2R_SPEC = census.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_effects/EFCommonEffects3",
    "f0310bc543527d5099f52b513f8a0f4c72a5a5ea3d124a00a487309de63242d1",
    85,
)
OUTPUT = ROOT / "src/nds/nds_rebirth_halo.generated.inc"

G_VTX = 0x01
G_TRI1 = 0x05
G_TRI2 = 0x06
G_DL = 0xDE
G_ENDDL = 0xDF
G_SETTIMG = 0xFD

GROUP_MAIN_UNTEXTURED = 0
GROUP_MAIN_E58 = 1
GROUP_MAIN_DD0 = 2
GROUP_MAIN_BC8 = 3
GROUP_BEAM = 4
GROUP_LEAVES = 5

TEXTURE_A40 = 0
TEXTURE_BC8 = 1
TEXTURE_DD0 = 2
TEXTURE_E58 = 3
TEXTURE_BEAM = 4


def words(data: bytes, off: int) -> tuple[int, int]:
    return struct.unpack_from(">II", data, off)


def o2r_pointer(word: int) -> int:
    return (word & 0xFFFF) * 4


def decode_vertex(data: bytes, off: int) -> tuple[int, ...]:
    x, y, z, _flag, s, t, r, g, b, a = struct.unpack_from(
        ">hhhHhhBBBB", data, off
    )
    if a == 0:
        a = 255
    return x, y, z, s, t, r, g, b, a


def decode_triangles(op: int, w0: int, w1: int) -> list[tuple[int, int, int]]:
    packed = [w0 & 0xFFFFFF]
    if op == G_TRI2:
        packed.append(w1 & 0xFFFFFF)
    return [
        (((value >> 16) & 0xFF) // 2,
         ((value >> 8) & 0xFF) // 2,
         (value & 0xFF) // 2)
        for value in packed
    ]


def flatten_root(data: bytes, root: int) -> list[tuple[int | None, tuple[int, ...], int]]:
    """Return (active image offset, vertex, triangle command pc) per corner."""
    cache: dict[int, int] = {}
    corners: list[tuple[int | None, tuple[int, ...], int]] = []
    active_image: int | None = None

    def walk(start: int) -> None:
        nonlocal active_image
        pc = start
        guard = 0
        while guard < 1024:
            guard += 1
            w0, w1 = words(data, pc)
            op = w0 >> 24
            if op == G_DL:
                walk(o2r_pointer(w1))
            elif op == G_ENDDL:
                return
            elif op == G_VTX:
                count = (w0 >> 12) & 0xFF
                end = (w0 >> 1) & 0x7F
                v0 = end - count
                src = o2r_pointer(w1)
                for i in range(count):
                    cache[v0 + i] = src + i * 16
            elif op == G_SETTIMG:
                active_image = o2r_pointer(w1)
            elif op in (G_TRI1, G_TRI2):
                for tri in decode_triangles(op, w0, w1):
                    for index in tri:
                        if index not in cache:
                            raise SystemExit(
                                f"vertex cache miss at 0x{pc:x}: slot {index}"
                            )
                        corners.append(
                            (active_image, decode_vertex(data, cache[index]), pc)
                        )
            pc += 8
        raise SystemExit(f"display-list guard expired at 0x{start:x}")

    walk(root)
    return corners


def split_groups(data: bytes) -> dict[int, list[tuple[int, ...]]]:
    groups: dict[int, list[tuple[int, ...]]] = {i: [] for i in range(6)}
    for image, vertex, _pc in flatten_root(data, 0x2378):
        if image is None:
            group = GROUP_MAIN_UNTEXTURED
        elif image == 0xE58:
            group = GROUP_MAIN_E58
        elif image == 0xDD0:
            group = GROUP_MAIN_DD0
        elif image == 0xBC8:
            group = GROUP_MAIN_BC8
        else:
            # SETTIMG points at a TLUT before a later SETTIMG selects texels.
            # Triangle-time image must always be one of the three texture bodies.
            raise SystemExit(f"unexpected main triangle image 0x{image:x}")
        groups[group].append(vertex)

    beam = flatten_root(data, 0x2890)
    groups[GROUP_BEAM].extend(vertex for _image, vertex, _pc in beam)
    leaves = flatten_root(data, 0x27E8)
    groups[GROUP_LEAVES].extend(vertex for _image, vertex, _pc in leaves)

    expected = (5, 10, 12, 6, 12, 8)
    for group, triangles in enumerate(expected):
        got = len(groups[group]) // 3
        if got != triangles or len(groups[group]) % 3:
            raise SystemExit(
                f"group {group} triangle count {got}, expected {triangles}"
            )
    return groups


def texture_records() -> dict[int, tuple[bytes, tuple[int, ...], int, int]]:
    resource = census.load_o2r(ROOT, O2R_SPEC)
    blocks = [{
        "identity": {"asset_id": 85, "offset": 0},
        "source_bytes": len(resource.payload),
    }]
    records: dict[object, static.PreparedRecord] = {}
    for root in (0x2378, 0x27E8):
        static.walk_display_list(
            resource, resource, root, 1, static.DisplayState(), blocks, records
        )

    result: dict[int, tuple[bytes, tuple[int, ...], int, int]] = {}
    for record in records.values():
        if record.image.offset not in (0xA40, 0xBC8, 0xDD0, 0xE58):
            continue
        ds_format, packed, palette = static.repack_paletted(record.pixels)
        if ds_format != static.DS_FORMAT_PAL16:
            raise SystemExit(f"0x{record.image.offset:x} did not repack as PAL16")
        palette = tuple(palette) + (0,) * (16 - len(palette))

        # DS GL_RGB16's COLOR0_TRANSPARENT semantics are index-based, while
        # the source RGBA5551 TLUT can put an opaque colour at index 0.  Keep
        # the conversion lossless by permuting that opaque source index with a
        # transparent TLUT slot, and apply the same permutation to every CI4
        # nibble.  DD0 uses opaque black at 0 / transparent 15; BC8 likewise
        # has opaque index 0 with unused transparent entries above its five
        # live colours.  Uploading either palette verbatim would punch holes
        # through the revival platform wherever source index 0 is used.
        if (palette[0] & 0x8000) != 0:
            transparent_index = next(
                (i for i in range(1, 16) if (palette[i] & 0x8000) == 0),
                None,
            )
            if transparent_index is None:
                raise SystemExit(
                    f"0x{record.image.offset:x} PAL16 has opaque index 0 and "
                    "no transparent slot for DS COLOR0 remap"
                )
            remapped = bytearray(len(packed))
            for i, value in enumerate(packed):
                lo = value & 0x0F
                hi = (value >> 4) & 0x0F
                if lo == 0:
                    lo = transparent_index
                elif lo == transparent_index:
                    lo = 0
                if hi == 0:
                    hi = transparent_index
                elif hi == transparent_index:
                    hi = 0
                remapped[i] = lo | (hi << 4)
            packed = bytes(remapped)
            palette_list = list(palette)
            palette_list[0], palette_list[transparent_index] = (
                palette_list[transparent_index],
                palette_list[0],
            )
            palette = tuple(palette_list)
        result[record.image.offset] = (
            packed, palette, record.upload_width, record.upload_height
        )
    if set(result) != {0xA40, 0xBC8, 0xDD0, 0xE58}:
        raise SystemExit(f"texture set mismatch: {sorted(result)}")
    return result


def beam_a5i3(data: bytes) -> bytes:
    # Source DL 0x2890 loads I4 8x16 at 0x9B8.  RGB is PRIMITIVE white and
    # alpha is TEXEL0, so DS A5I3 preserves the 16-step source coverage ramp.
    # LOADBLOCK's DXT is 0x800: the runtime resolver derives a 16-pixel I4
    # source stride (one 64-bit word per source row), while the render tile
    # consumes only the left 8 pixels. Do not tighten those rows to 8 pixels
    # before sampling; doing so aliases each unused right half into the next
    # displayed row.
    # This generator reads BattleShip's extracted relocData file, whose bytes
    # are already in logical N64 order (unlike the live O2R buffer, where the
    # runtime renderer must apply logical_byte ^ 3). Do not apply that runtime
    # lane correction a second time here.
    out = bytearray(8 * 16)
    for y in range(16):
        for x in range(8):
            source_index = y * 16 + x
            logical_byte = source_index >> 1
            packed = data[0x9B8 + logical_byte]
            intensity = (
                (packed >> 4) if (source_index & 1) == 0 else (packed & 0xF)
            )
            alpha5 = (intensity * 0x11) >> 3
            out[y * 8 + x] = (alpha5 << 3)  # palette index 0 = white
    return bytes(out)


def c_bytes(name: str, values: bytes, per_line: int = 16) -> list[str]:
    lines = [f"static const u8 {name}[{len(values)}] = {{"]
    for i in range(0, len(values), per_line):
        chunk = values[i:i + per_line]
        lines.append("    " + ", ".join(f"0x{x:02x}" for x in chunk) + ",")
    lines.append("};")
    return lines


def group_bounds(vertices: list[tuple[int, ...]]) -> tuple[int, int, int, int, int, int]:
    """Return the immutable integer-space AABB for one native draw group."""
    if not vertices:
        raise SystemExit("cannot bound an empty RebirthHalo group")
    xs = [vertex[0] for vertex in vertices]
    ys = [vertex[1] for vertex in vertices]
    zs = [vertex[2] for vertex in vertices]
    return min(xs), min(ys), min(zs), max(xs), max(ys), max(zs)


def main() -> None:
    data = SOURCE.read_bytes()
    groups = split_groups(data)
    textures = texture_records()
    beam = beam_a5i3(data)

    out: list[str] = [
        "/* Generated by scripts/3d_vfx/generate_nds_rebirth_halo.py. */",
        "/* BattleShip EFCommonEffects3 SHA-pinned O2R + source relocData. */",
        "",
        "#define NDS_REBIRTH_HALO_GROUP_COUNT 6u",
        "#define NDS_REBIRTH_HALO_TEXTURE_COUNT 5u",
        "#define NDS_REBIRTH_HALO_TEXTURE_A40 0u",
        "#define NDS_REBIRTH_HALO_TEXTURE_BC8 1u",
        "#define NDS_REBIRTH_HALO_TEXTURE_DD0 2u",
        "#define NDS_REBIRTH_HALO_TEXTURE_E58 3u",
        "#define NDS_REBIRTH_HALO_TEXTURE_BEAM 4u",
        "",
    ]

    flat: list[tuple[int, ...]] = []
    descriptors: list[tuple[int, int]] = []
    for group in range(6):
        descriptors.append((len(flat), len(groups[group]) // 3))
        flat.extend(groups[group])
    out.append(f"static const NDSRendererInputVertex sNdsRebirthHaloVertices[{len(flat)}] = {{")
    for vertex in flat:
        x, y, z, s, t, r, g, b, a = vertex
        out.append(
            f"    {{ {x:6d}, {y:6d}, {z:6d}, {s:6d}, {t:6d}, "
            f"{r:3d}, {g:3d}, {b:3d}, {a:3d} }},"
        )
    out.append("};")
    out.append("")
    out.append("static const NDSRebirthHaloGroup sNdsRebirthHaloGroups[NDS_REBIRTH_HALO_GROUP_COUNT] = {")
    for first, count in descriptors:
        out.append(f"    {{ {first}u, {count}u, 0u }},")
    out.append("};")
    out.append("")

    out.append("static const NDSRebirthHaloBounds sNdsRebirthHaloBounds[NDS_REBIRTH_HALO_GROUP_COUNT] = {")
    for group in range(6):
        lo_x, lo_y, lo_z, hi_x, hi_y, hi_z = group_bounds(groups[group])
        out.append(
            f"    {{ {lo_x}, {lo_y}, {lo_z}, {hi_x}, {hi_y}, {hi_z} }},"
        )
    out.append("};")
    out.append("")

    texture_order = (
        (0xA40, "A40"), (0xBC8, "BC8"), (0xDD0, "DD0"), (0xE58, "E58")
    )
    for offset, label in texture_order:
        packed, palette, width, height = textures[offset]
        out.append(f"#define NDS_REBIRTH_HALO_{label}_WIDTH {width}u")
        out.append(f"#define NDS_REBIRTH_HALO_{label}_HEIGHT {height}u")
        out.extend(c_bytes(f"sNdsRebirthHaloTexels{label}", packed))
        out.append(f"static const u16 sNdsRebirthHaloPalette{label}[16] = {{")
        out.append("    " + ", ".join(f"0x{x:04x}" for x in palette) + ",")
        out.append("};")
        out.append("")

    out.append("#define NDS_REBIRTH_HALO_BEAM_WIDTH 8u")
    out.append("#define NDS_REBIRTH_HALO_BEAM_HEIGHT 16u")
    out.extend(c_bytes("sNdsRebirthHaloTexelsBeam", beam))
    out.append("static const u16 sNdsRebirthHaloPaletteBeam[8] = {")
    out.append("    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,")
    out.append("};")

    OUTPUT.write_text("\n".join(out) + "\n", encoding="ascii")
    print(
        f"wrote {OUTPUT.relative_to(ROOT)}: {len(flat)//3} triangles, "
        f"{sum(len(v[0]) for v in textures.values()) + len(beam)} DS texel bytes"
    )


if __name__ == "__main__":
    main()
