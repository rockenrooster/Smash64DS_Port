#!/usr/bin/env python3
"""Generate/check Ness PK Thunder head/trail native weapon packets."""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(REPO / "scripts" / "stages"))

import generate_nds_native_stage as sm  # noqa: E402

OUT = REPO / "src/nds/generated/nds_native_ness_pkthunder.generated.inc"
OUT_HEADER = REPO / "include/nds/generated/nds_native_ness_pkthunder.generated.h"

MODEL = sm.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/NessModel",
    "d4d8f1dfe24667fb1274dbeb82dd18fcb7b274d31db223bbab9f633a6b9a378a",
    335, 364, 4,
    "c191a9494c41d56060748bc1447028bd7e8c1781245fc72110b624dd49d198ad")

ASSET = 335
ROOTS = (0x7BD0, 0x8A98, 0x9948)
ROOT_WORDS = (23, 21, 23)
VERTEX_WORDS = (17, 15, 17)
TRI_WORDS = (18, 16, 18)
VERTEX_OFFSETS = (0x7B90, 0x8A58, 0x9908)
MATERIAL_OFFSETS = (0x7B10, 0x89D8, 0x9888)
SPRITE_TABLE_OFFSETS = (0x7B00, 0x89CC, 0x987C)
SPRITE_COUNTS = (3, 3, 2)
EXPECTED_OPS = (
    (0xE7, 0xDB, 0xDB, 0xDB, 0xDB, 0xD9, 0xFC, 0xFA, 0xFB, 0xF5,
     0xF5, 0xD7, 0xF2, 0xDE, 0xE6, 0xF3, 0xE7, 0x01, 0x06, 0xE7,
     0xE7, 0xD9, 0xDF),
    (0xE7, 0xDB, 0xDB, 0xDB, 0xDB, 0xD9, 0xFC, 0xF5, 0xF5, 0xD7,
     0xF2, 0xDE, 0xE6, 0xF3, 0xE7, 0x01, 0x06, 0xE7, 0xE7, 0xD9, 0xDF),
    (0xE7, 0xDB, 0xDB, 0xDB, 0xDB, 0xD9, 0xFC, 0xFA, 0xFB, 0xF5,
     0xF5, 0xD7, 0xF2, 0xDE, 0xE6, 0xF3, 0xE7, 0x01, 0x06, 0xE7,
     0xE7, 0xD9, 0xDF),
)


def words_at(payload: bytes, off: int, count: int):
    return [struct.unpack_from(">II", payload, off + i * 8) for i in range(count)]


def decode():
    model = sm.load_o2r(REPO, MODEL)
    roots = []
    for index, root in enumerate(ROOTS):
        raw = words_at(model.payload, root, ROOT_WORDS[index])
        ops = tuple(w0 >> 24 for w0, _ in raw)
        if ops != EXPECTED_OPS[index]:
            raise RuntimeError(f"PK Thunder root {root:#x} opcode census changed: {ops!r}")

        vref = model.pointer_at(root + VERTEX_WORDS[index] * 8 + 4)
        if vref is None or (vref.asset_id, vref.offset) != (ASSET, VERTEX_OFFSETS[index]):
            raise RuntimeError(f"PK Thunder root {root:#x} vertex ref changed: {vref!r}")
        vword = raw[VERTEX_WORDS[index]][0]
        count = (vword >> 12) & 0xFF
        end = (vword >> 1) & 0x7F
        if (count, end - count) != (4, 0):
            raise RuntimeError(f"PK Thunder root {root:#x} vertex cache changed")
        verts = tuple(sm.decode_vertex(model, vref.offset + i * 16) for i in range(4))

        tw0, tw1 = raw[TRI_WORDS[index]]
        tris = tuple(sm.decode_triangles(tw0 >> 24, tw0, tw1))
        if tris != ((3, 2, 1), (0, 3, 1)):
            raise RuntimeError(f"PK Thunder root {root:#x} triangles changed: {tris!r}")

        material = MATERIAL_OFFSETS[index]
        flags = struct.unpack_from(">H", model.payload, material + 0x30)[0]
        if flags != 0x0001:
            raise RuntimeError(f"PK Thunder root {root:#x} material flags {flags:#x} != TEXTURE")
        sprites = model.pointer_at(material + 4)
        if sprites is None or (sprites.asset_id, sprites.offset) != (ASSET, SPRITE_TABLE_OFFSETS[index]):
            raise RuntimeError(f"PK Thunder root {root:#x} sprite table changed: {sprites!r}")
        for sprite_index in range(SPRITE_COUNTS[index]):
            ref = model.pointer_at(SPRITE_TABLE_OFFSETS[index] + sprite_index * 4)
            if ref is None or ref.asset_id != ASSET:
                raise RuntimeError(f"PK Thunder root {root:#x} sprite {sprite_index} changed: {ref!r}")
        roots.append((raw, verts))
    return tuple(roots)


def render_header() -> str:
    return """/* Ness PK Thunder native weapon constants (generated).
 * Do not hand-edit; regenerate with generate_nds_native_ness_pkthunder.py. */
#ifndef NDS_NATIVE_NESS_PKTHUNDER_GENERATED_H
#define NDS_NATIVE_NESS_PKTHUNDER_GENERATED_H

#define NDS_NATIVE_NESS_PKTHUNDER_ASSET 335u
#define NDS_NATIVE_NESS_PKTHUNDER_ROOT_COUNT 3u
#define NDS_NATIVE_NESS_PKTHUNDER_HEAD_INDEX 0u
#define NDS_NATIVE_NESS_PKTHUNDER_TRAIL_INDEX 1u
#define NDS_NATIVE_NESS_PKTHUNDER_WAVE_INDEX 2u
#define NDS_NATIVE_NESS_PKTHUNDER_HEAD_ROOT 0x7bd0u
#define NDS_NATIVE_NESS_PKTHUNDER_TRAIL_ROOT 0x8a98u
#define NDS_NATIVE_NESS_PKTHUNDER_WAVE_ROOT 0x9948u
#define NDS_NATIVE_NESS_PKTHUNDER_VERTEX_COUNT 4u
#define NDS_NATIVE_NESS_PKTHUNDER_TRIANGLE_COUNT 2u
#define NDS_NATIVE_NESS_PKTHUNDER_MATERIAL_EFFECTS 0x00000200u
#define NDS_NATIVE_NESS_PKTHUNDER_TRAIL_COLOR_COUNT 4u

#endif
"""


def render(roots) -> str:
    lines = [
        "/* Ness PK Thunder native weapon packet (generated).",
        " * Source: SHA-pinned NessModel file 335 roots 0x7bd0 and 0x8a98.",
        " * Each root is one live-textured four-vertex quad / two triangles.",
        " * Geometry and immutable RSP/RDP state are AOT; CURRENT_IMAGE keeps",
        " * the source texture-id animation. Trail prim/env is supplied by the",
        " * live source trail_id, matching wpDisplayPKThunderProcDisplay.",
        " * Do not hand-edit; regenerate with generate_nds_native_ness_pkthunder.py. */",
        "#include <nds/generated/nds_native_ness_pkthunder.generated.h>",
        "",
        "static const s16 sNdsNativeNessPKThunderVerts[60] =",
        "{",
    ]
    for root_index, (_raw, verts) in enumerate(roots):
        lines.append(f"    /* root 0x{ROOTS[root_index]:04x} */")
        for v in verts:
            lines.append(f"    {v[0]}, {v[1]}, {v[2]}, {v[3]}, {v[4]},")
    lines += ["};", "", "static const u32 sNdsNativeNessPKThunderColors[12] =", "{"]
    for _raw, verts in roots:
        lines += [f"    0x{v[5]:08x}u," for v in verts]
    lines += [
        "};", "",
        "static const u16 sNdsNativeNessPKThunderTriangles[6] =",
        "{ 3u, 2u, 1u, 0u, 3u, 1u };", "",
        "static const u32 sNdsNativeNessPKThunderTrailPrim[4] =",
        "{ 0x5ea3ffffu, 0x98bdffffu, 0xc2d9ffffu, 0xb3f1ffffu };",
        "static const u32 sNdsNativeNessPKThunderTrailEnv[4] =",
        "{ 0x3a0083ffu, 0x5b00b2ffu, 0x8633d9ffu, 0xa774f8ffu };", "",
        "static void ndsNativeNessPKThunderSetup(u32 root_index, NDSRendererStats *stats)",
        "{",
        "    ndsRendererRecordSetCombine(stats, 0xfc30fe61u, 0x55fef379u);",
        "    stats->geometry_mode = (stats->geometry_mode & 0xd9ddfffbu);",
        "    if (root_index == NDS_NATIVE_NESS_PKTHUNDER_HEAD_INDEX)",
        "    {",
        "        stats->prim_color = 0xdeffffffu;",
        "        stats->env_color = 0x3c0071ffu;",
        "        ndsRendererRecordSetTile(stats, 0xf5700000u, 0x07014050u);",
        "        ndsRendererRecordSetTile(stats, 0xf5680800u, 0x000d4350u);",
        "        ndsRendererRecordTextureState(stats, 0xd7000002u, 0xffffffffu);",
        "        ndsRendererRecordSetTileSize(stats, 0xf2000000u, 0x000fc0fcu);",
        "        ndsRendererRecordLoadBlock(stats, 0xf3000000u, 0x071ff200u);",
        "    }",
        "    else if (root_index == NDS_NATIVE_NESS_PKTHUNDER_TRAIL_INDEX)",
        "    {",
        "        ndsRendererRecordSetTile(stats, 0xf5700000u, 0x07014050u);",
        "        ndsRendererRecordSetTile(stats, 0xf5680800u, 0x00094250u);",
        "        ndsRendererRecordTextureState(stats, 0xd7000002u, 0xffffffffu);",
        "        ndsRendererRecordSetTileSize(stats, 0xf2000000u, 0x0007c07cu);",
        "        ndsRendererRecordLoadBlock(stats, 0xf3000000u, 0x071ff200u);",
        "    }",
        "    else",
        "    {",
        "        stats->prim_color = 0x8fffffffu;",
        "        stats->env_color = 0x442ba6ffu;",
        "        ndsRendererRecordSetTile(stats, 0xf5700000u, 0x07014050u);",
        "        ndsRendererRecordSetTile(stats, 0xf5680800u, 0x00094350u);",
        "        ndsRendererRecordTextureState(stats, 0xd7000002u, 0xffffffffu);",
        "        ndsRendererRecordSetTileSize(stats, 0xf2000000u, 0x000fc07cu);",
        "        ndsRendererRecordLoadBlock(stats, 0xf3000000u, 0x071ff200u);",
        "    }",
        "}", "",
        "static void ndsNativeNessPKThunderFinish(NDSRendererStats *stats)",
        "{",
        "    stats->geometry_mode = (stats->geometry_mode & 0xd9ffffffu) | 0x00220004u;",
        "}", "",
        "/* census: roots=3 tris=6 materials=CURRENT_IMAGE asset=335 */", "",
    ]
    return "\n".join(lines)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--emit", action="store_true")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    roots = decode()
    packet = render(roots)
    header = render_header()
    if args.emit:
        for path, text in ((OUT, packet), (OUT_HEADER, header)):
            path.parent.mkdir(parents=True, exist_ok=True)
            if (not path.exists()) or path.read_text() != text:
                path.write_text(text)
        print(f"emitted {OUT.relative_to(REPO)} and {OUT_HEADER.relative_to(REPO)}")
    if args.check or not args.emit:
        for path, text in ((OUT, packet), (OUT_HEADER, header)):
            if not path.exists() or path.read_text() != text:
                raise RuntimeError(f"generated artefact stale: {path.relative_to(REPO)}")
        print("NESS_PKTHUNDER_NATIVE_OK asset=335 roots=0x7bd0,0x8a98,0x9948 tris=6")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
