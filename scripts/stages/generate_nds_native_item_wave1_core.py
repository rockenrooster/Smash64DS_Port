#!/usr/bin/env python3
"""Source-checked generators for the first five common-item native owners.

This is build tooling only. It reads the SHA-pinned BattleShip O2R payload,
proves each selected root's exact opcode/material shape, decodes the fixed
geometry, and emits DS-native packet helpers. Runtime code never scans or
interprets the source display lists.
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(REPO / "scripts" / "stages"))

import generate_nds_native_stage as sm  # noqa: E402


MODEL_FILE = sm.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/MiscData086",
    "96e987d81b24497ad0c314372edb79f123016b59187b5f94b57b73e4bb122c04",
    86, 402, 0,
    "1c642e60401ca5e6df2fe1b0d6ddeb6e322b0288f4c13a1e608875322fbf9438")
ATTR_FILE = sm.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_items/ITCommonData",
    "8ad38613162d33e712f79f9d5584540dadd3fbddad90287dedf8cfc59cc76f32",
    251, 0, 68,
    "264b5ef35dfe41bd22cc94a82a80034e6913b9d1d827839392a6328ac9a60145")

ASSET = 86


def _words(model, off: int, count: int):
    return tuple(struct.unpack_from(">II", model.payload, off + i * 8)
                 for i in range(count))


def _expect_ptr(res, slot: int, off: int, label: str) -> None:
    ref = res.pointer_at(slot)
    if ref is None or (ref.asset_id, ref.offset) != (ASSET, off):
        raise RuntimeError(f"{label} changed: {ref!r}")


def _expect_null(res, slot: int, label: str) -> None:
    if (res.pointer_at(slot) is not None or
            struct.unpack_from(">I", res.payload, slot)[0] != 0):
        raise RuntimeError(f"{label} is no longer NULL")


def _expect_ops(raw, expected, label: str) -> None:
    got = tuple(w0 >> 24 for w0, _ in raw)
    if got != expected:
        raise RuntimeError(f"{label} opcode census changed: {got!r}")
    if (raw[-1][0] >> 24) != 0xDF:
        raise RuntimeError(f"{label} no longer ends in G_ENDDL")


def _expect_de(raw, expected, label: str) -> None:
    got = tuple((i, w0, w1) for i, (w0, w1) in enumerate(raw)
                if (w0 >> 24) == 0xDE)
    if got != expected:
        raise RuntimeError(f"{label} 0xDE shape changed: {got!r}")


def _decode_verts(model, off: int, count: int):
    return tuple(sm.decode_vertex(model, off + i * 16) for i in range(count))


def _decode_tris(raw, words):
    out = []
    for index, opcode in words:
        out.extend(sm.decode_triangles(opcode, *raw[index]))
    return tuple(out)


def _ptr_offsets(model, table: int, count: int, label: str):
    out = []
    for i in range(count):
        ref = model.pointer_at(table + i * 4)
        if ref is None or ref.asset_id != ASSET:
            raise RuntimeError(f"{label}[{i}] changed: {ref!r}")
        out.append(ref.offset)
    return tuple(out)


def _arrays(prefix: str, verts, tris) -> str:
    lines = [
        f"static const u16 sNdsNativeItem{prefix}TriIndices[{len(tris) * 3}] =",
        "{",
    ]
    for tri in tris:
        lines.append("    " + " ".join(f"{v}u," for v in tri))
    lines.extend(("};", "",
                  f"static const s16 sNdsNativeItem{prefix}Verts[{len(verts) * 5}] =",
                  "{"))
    for v in verts:
        lines.append(f"    {v[0]}, {v[1]}, {v[2]}, {v[3]}, {v[4]},")
    lines.extend(("};", "",
                  f"static const u32 sNdsNativeItem{prefix}VertColors[{len(verts)}] =",
                  "{"))
    for v in verts:
        lines.append(f"    0x{v[5]:08x}u,")
    lines.extend(("};", ""))
    return "\n".join(lines)


def _othermode(raw, index: int) -> str:
    return (f"    ndsRendererRecordOtherMode(stats, 0x{raw[index][0] >> 24:02x}u, "
            f"0x{raw[index][0]:08x}u, 0x{raw[index][1]:08x}u);")


def _header(slug: str, macro: str, body) -> str:
    guard = f"NDS_NATIVE_ITEM_{macro}_GENERATED_H"
    lines = [
        f"/* {slug} native item constants (generated).",
        f" * Do not hand-edit; regenerate with generate_nds_native_item_{slug}.py. */",
        f"#ifndef {guard}", f"#define {guard}", "",
    ]
    lines.extend(body)
    lines.extend(("", "#endif", ""))
    return "\n".join(lines)


def _paths(slug: str):
    return (
        REPO / f"src/nds/generated/nds_native_item_{slug}.generated.inc",
        REPO / f"include/nds/generated/nds_native_item_{slug}.generated.h",
    )


STAR_OPS = (
    0xE7, 0xD9, 0xE3, 0xFC, 0xE8, 0xF5, 0xF5, 0xF5,
    0xDE, 0xE6, 0xF0, 0xE7, 0xD7, 0xF2, 0xFD, 0xE6,
    0xF3, 0xE7, 0x01, 0x05, 0xE7, 0xE8, 0xF5, 0xDE,
    0xE6, 0xF0, 0xE7, 0xD7, 0xF2, 0x01, 0x06, 0xE7,
    0xE7, 0xD9, 0xE3, 0xDF,
)


def _star(model, attr):
    root = 0x1440
    raw = _words(model, root, 36)
    _expect_ops(raw, STAR_OPS, "Star 0x1440")
    _expect_de(raw, ((8, 0xDE000000, 0x0E000000),
                     (23, 0xDE000000, 0x0E000008)), "Star")
    _expect_ptr(attr, 0x148, 0x1560, "Star ITAttributes.data")
    _expect_ptr(attr, 0x14C, 0x12B8, "Star ITAttributes.p_mobjsubs")
    _expect_null(attr, 0x150, "Star ITAttributes.anim_joints")
    _expect_ptr(attr, 0x154, 0x15F0, "Star ITAttributes.p_matanim_joints")
    _expect_ptr(model, 0x1590, root, "Star DObj root")
    _expect_ptr(model, 0x12BC, 0x13C0, "Star MObj head")
    _expect_ptr(model, 0x13C0, 0x12D0, "Star MObjSub 0")
    _expect_ptr(model, 0x13C4, 0x1348, "Star MObjSub 1")
    for sub in (0x12D0, 0x1348):
        flags = struct.unpack_from(">H", model.payload, sub + 0x30)[0]
        if flags != 0x0004:
            raise RuntimeError(f"Star MObjSub {sub:#x} flags changed: {flags:#x}")
        if model.pointer_at(sub + 4) is not None:
            raise RuntimeError("Star MObj unexpectedly gained a sprite stream")
    _expect_ptr(model, root + 14 * 8 + 4, 0x1238, "Star image")
    _expect_ptr(model, root + 18 * 8 + 4, 0x13D0, "Star vertices 0")
    _expect_ptr(model, root + 29 * 8 + 4, 0x1400, "Star vertices 1")
    palettes = _ptr_offsets(model, 0x12C0, 2, "Star palettes 0")
    if _ptr_offsets(model, 0x12C8, 2, "Star palettes 1") != palettes:
        raise RuntimeError("Star MObjs no longer share the same palette pair")
    verts0 = _decode_verts(model, 0x13D0, 3)
    verts1 = _decode_verts(model, 0x1400, 4)
    tris0 = _decode_tris(raw, ((19, 0x05),))
    tris1 = _decode_tris(raw, ((30, 0x06),))
    header = _header("star", "STAR", (
        f"#define NDS_NATIVE_ITEM_STAR_ASSET {ASSET}u",
        f"#define NDS_NATIVE_ITEM_STAR_ROOT 0x{root:04x}u",
        f"#define NDS_NATIVE_ITEM_STAR_DL_BYTES {len(raw) * 8}u",
        f"#define NDS_NATIVE_ITEM_STAR_FILE_END 0x{root + len(raw) * 8:04x}u",
        "#define NDS_NATIVE_ITEM_STAR_HOOK0_W0 0xde000000u",
        "#define NDS_NATIVE_ITEM_STAR_HOOK0_W1 0x0e000000u",
        "#define NDS_NATIVE_ITEM_STAR_HOOK1_W0 0xde000000u",
        "#define NDS_NATIVE_ITEM_STAR_HOOK1_W1 0x0e000008u",
        "#define NDS_NATIVE_ITEM_STAR_IMAGE_OFFSET 0x1238u",
        f"#define NDS_NATIVE_ITEM_STAR_IMAGE_W0 0x{raw[14][0]:08x}u",
        "#define NDS_NATIVE_ITEM_STAR_VERTEX0_OFFSET 0x13d0u",
        "#define NDS_NATIVE_ITEM_STAR_VERTEX1_OFFSET 0x1400u",
        f"#define NDS_NATIVE_ITEM_STAR_VERTEX0_COUNT {len(verts0)}u",
        f"#define NDS_NATIVE_ITEM_STAR_TRIANGLE0_COUNT {len(tris0)}u",
        f"#define NDS_NATIVE_ITEM_STAR_CORNER0_COUNT {len(tris0) * 3}u",
        f"#define NDS_NATIVE_ITEM_STAR_VERTEX1_COUNT {len(verts1)}u",
        f"#define NDS_NATIVE_ITEM_STAR_TRIANGLE1_COUNT {len(tris1)}u",
        f"#define NDS_NATIVE_ITEM_STAR_CORNER1_COUNT {len(tris1) * 3}u",
    ))
    lines = [
        "/* Star native packet, generated from file 86 root 0x1440.",
        " * Exact live calls: word 8 -> 0x0e000000, word 23 -> 0x0e000008.",
        " * Both MObjs are PALETTE_IMAGE-only; image/geometry are baked. */",
        "#include <nds/generated/nds_native_item_star.generated.h>", "",
        _arrays("Star0", verts0, tris0), _arrays("Star1", verts1, tris1),
        "static const u32 sNdsNativeItemStarPaletteOffsets[2] =", "{",
        *(f"    0x{x:04x}u," for x in palettes), "};", "",
        "static void ndsNativeItemStarSetup(NDSRendererStats *stats)", "{",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[1][0]:08x}u) | 0x{raw[1][1]:08x}u;",
        _othermode(raw, 2),
        f"    ndsRendererRecordSetCombine(stats, 0x{raw[3][0]:08x}u, 0x{raw[3][1]:08x}u);",
        f"    stats->blend_color = 0x{raw[4][1]:08x}u;",
        *(f"    ndsRendererRecordSetTile(stats, 0x{raw[i][0]:08x}u, 0x{raw[i][1]:08x}u);" for i in (5, 6, 7)),
        "}", "",
        "static void ndsNativeItemStarAfterMaterial0(NDSRendererStats *stats, const void *image)", "{",
        f"    ndsRendererRecordLoadTlut(stats, 0x{raw[10][1]:08x}u);",
        f"    ndsRendererRecordTextureState(stats, 0x{raw[12][0]:08x}u, 0x{raw[12][1]:08x}u);",
        f"    ndsRendererRecordSetTileSize(stats, 0x{raw[13][0]:08x}u, 0x{raw[13][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{raw[14][0]:08x}u, (u32)(uintptr_t)image);",
        f"    ndsRendererRecordLoadBlock(stats, 0x{raw[16][0]:08x}u, 0x{raw[16][1]:08x}u);",
        "}", "",
        "static void ndsNativeItemStarBeforeMaterial1(NDSRendererStats *stats)", "{",
        f"    ndsRendererRecordSetTile(stats, 0x{raw[22][0]:08x}u, 0x{raw[22][1]:08x}u);",
        "}", "",
        "static void ndsNativeItemStarAfterMaterial1(NDSRendererStats *stats)", "{",
        f"    ndsRendererRecordLoadTlut(stats, 0x{raw[25][1]:08x}u);",
        f"    ndsRendererRecordTextureState(stats, 0x{raw[27][0]:08x}u, 0x{raw[27][1]:08x}u);",
        f"    ndsRendererRecordSetTileSize(stats, 0x{raw[28][0]:08x}u, 0x{raw[28][1]:08x}u);",
        "}", "",
        "static void ndsNativeItemStarFinish(NDSRendererStats *stats)", "{",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[33][0]:08x}u) | 0x{raw[33][1]:08x}u;",
        _othermode(raw, 34), "}", "",
    ]
    check = ("ITEM_STAR_NATIVE_OK root=0x1440 verts=3+4 tris=1+2 "
             "material=PALETTE_IMAGE,PALETTE_IMAGE")
    return "\n".join(lines), header, check


SWORD_BLADE_OPS = (0xE7, 0xDB, 0xDB, 0xDB, 0xDB, 0xFC, 0xD7, 0x01,
                   0x06, 0x06, 0x06, 0x06, 0x06, 0x05, 0xDF)
SWORD_HILT_OPS = (0xE7, 0xE2, 0xFC, 0xFA, 0xFB, 0xF9, 0xF5, 0xF5,
                  0xD7, 0xF2, 0xFD, 0xE6, 0xF3, 0xE7, 0xD9, 0x01,
                  0x06, 0xE7, 0xD9, 0xE2, 0xDF)


def _sword(model, attr):
    blade_root, hilt_root = 0x17D8, 0x1850
    blade, hilt = _words(model, blade_root, 15), _words(model, hilt_root, 21)
    _expect_ops(blade, SWORD_BLADE_OPS, "Sword blade")
    _expect_ops(hilt, SWORD_HILT_OPS, "Sword hilt")
    _expect_de(blade, (), "Sword blade")
    _expect_de(hilt, (), "Sword hilt")
    _expect_ptr(attr, 0x190, 0x1918, "Sword ITAttributes.data")
    for slot, label in ((0x194, "p_mobjsubs"), (0x198, "anim_joints"),
                        (0x19C, "p_matanim_joints")):
        _expect_null(attr, slot, f"Sword ITAttributes.{label}")
    _expect_ptr(model, 0x18FC, blade_root, "Sword blade root")
    _expect_ptr(model, 0x190C, hilt_root, "Sword hilt root")
    _expect_ptr(model, blade_root + 7 * 8 + 4, 0x16E8, "Sword blade vertices")
    _expect_ptr(model, hilt_root + 10 * 8 + 4, 0x1668, "Sword hilt image")
    _expect_ptr(model, hilt_root + 15 * 8 + 4, 0x1798, "Sword hilt vertices")
    blade_verts = _decode_verts(model, 0x16E8, 11)
    hilt_verts = _decode_verts(model, 0x1798, 4)
    blade_tris = _decode_tris(
        blade, tuple((i, 0x06) for i in range(8, 13)) + ((13, 0x05),))
    hilt_tris = _decode_tris(hilt, ((16, 0x06),))
    file_end = hilt_root + len(hilt) * 8
    header = _header("sword", "SWORD", (
        f"#define NDS_NATIVE_ITEM_SWORD_ASSET {ASSET}u",
        f"#define NDS_NATIVE_ITEM_SWORD_BLADE_ROOT 0x{blade_root:04x}u",
        f"#define NDS_NATIVE_ITEM_SWORD_HILT_ROOT 0x{hilt_root:04x}u",
        f"#define NDS_NATIVE_ITEM_SWORD_FILE_END 0x{file_end:04x}u",
        "#define NDS_NATIVE_ITEM_SWORD_BLADE_VERTEX_OFFSET 0x16e8u",
        "#define NDS_NATIVE_ITEM_SWORD_HILT_IMAGE_OFFSET 0x1668u",
        f"#define NDS_NATIVE_ITEM_SWORD_HILT_IMAGE_W0 0x{hilt[10][0]:08x}u",
        "#define NDS_NATIVE_ITEM_SWORD_HILT_VERTEX_OFFSET 0x1798u",
        f"#define NDS_NATIVE_ITEM_SWORD_BLADE_VERTEX_COUNT {len(blade_verts)}u",
        f"#define NDS_NATIVE_ITEM_SWORD_BLADE_TRIANGLE_COUNT {len(blade_tris)}u",
        f"#define NDS_NATIVE_ITEM_SWORD_BLADE_CORNER_COUNT {len(blade_tris) * 3}u",
        f"#define NDS_NATIVE_ITEM_SWORD_HILT_VERTEX_COUNT {len(hilt_verts)}u",
        f"#define NDS_NATIVE_ITEM_SWORD_HILT_TRIANGLE_COUNT {len(hilt_tris)}u",
        f"#define NDS_NATIVE_ITEM_SWORD_HILT_CORNER_COUNT {len(hilt_tris) * 3}u",
    ))
    lines = [
        "/* Beam Sword packet, generated from file 86 roots 0x17d8/0x1850.",
        " * Neither root contains 0xDE; both are bake-everything. */",
        "#include <nds/generated/nds_native_item_sword.generated.h>", "",
        _arrays("SwordBlade", blade_verts, blade_tris),
        _arrays("SwordHilt", hilt_verts, hilt_tris),
        "static void ndsNativeItemSwordBladeSetup(",
        "    NDSRendererStats *stats, NDSRendererTraversalState *state)", "{",
        *(f"    ndsRendererApplyMatrixMoveWordCommand(stats, state, 0x{blade[i][0]:08x}u, 0x{blade[i][1]:08x}u);" for i in (1, 2, 3, 4)),
        f"    ndsRendererRecordSetCombine(stats, 0x{blade[5][0]:08x}u, 0x{blade[5][1]:08x}u);",
        f"    ndsRendererRecordTextureState(stats, 0x{blade[6][0]:08x}u, 0x{blade[6][1]:08x}u);",
        "}", "",
        "static void ndsNativeItemSwordHiltSetup(NDSRendererStats *stats, const void *image)", "{",
        _othermode(hilt, 1),
        f"    ndsRendererRecordSetCombine(stats, 0x{hilt[2][0]:08x}u, 0x{hilt[2][1]:08x}u);",
        f"    stats->prim_color = 0x{hilt[3][1]:08x}u;",
        f"    stats->env_color = 0x{hilt[4][1]:08x}u;",
        f"    stats->blend_color = 0x{hilt[5][1]:08x}u;",
        *(f"    ndsRendererRecordSetTile(stats, 0x{hilt[i][0]:08x}u, 0x{hilt[i][1]:08x}u);" for i in (6, 7)),
        f"    ndsRendererRecordTextureState(stats, 0x{hilt[8][0]:08x}u, 0x{hilt[8][1]:08x}u);",
        f"    ndsRendererRecordSetTileSize(stats, 0x{hilt[9][0]:08x}u, 0x{hilt[9][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{hilt[10][0]:08x}u, (u32)(uintptr_t)image);",
        f"    ndsRendererRecordLoadBlock(stats, 0x{hilt[12][0]:08x}u, 0x{hilt[12][1]:08x}u);",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{hilt[14][0]:08x}u) | 0x{hilt[14][1]:08x}u;",
        "}", "",
        "static void ndsNativeItemSwordHiltFinish(NDSRendererStats *stats)", "{",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{hilt[18][0]:08x}u) | 0x{hilt[18][1]:08x}u;",
        _othermode(hilt, 19), "}", "",
    ]
    check = ("ITEM_SWORD_NATIVE_OK roots=0x17d8,0x1850 verts=11+4 "
             "tris=11+2 material=none")
    return "\n".join(lines), header, check


HAMMER_OPS = (
    0xE7, 0xDB, 0xDB, 0xDB, 0xDB, 0xE3, 0xFC, 0xE8,
    0xF5, 0xF5, 0xF5, 0xFD, 0xE6, 0xF0, 0xE7, 0xD7,
    0xF2, 0xFD, 0xE6, 0xF3, 0xE7, 0x01, 0x06, 0x06,
    0x06, 0x06, 0x06, 0x06, 0xE7, 0xE3, 0xDB, 0xDB,
    0xDB, 0xDB, 0xFC, 0xD7, 0x01, 0x06, 0x06, 0x06,
    0x06, 0x06, 0x06, 0xDF,
)


def _hammer(model, attr):
    root = 0x25F0
    raw = _words(model, root, 44)
    _expect_ops(raw, HAMMER_OPS, "Hammer")
    _expect_de(raw, (), "Hammer")
    _expect_ptr(attr, 0x374, 0x2750, "Hammer ITAttributes.data")
    for slot, label in ((0x378, "p_mobjsubs"), (0x37C, "anim_joints"),
                        (0x380, "p_matanim_joints")):
        _expect_null(attr, slot, f"Hammer ITAttributes.{label}")
    _expect_ptr(model, 0x2780, root, "Hammer DObj root")
    _expect_ptr(model, root + 11 * 8 + 4, 0x22C8, "Hammer TLUT")
    _expect_ptr(model, root + 17 * 8 + 4, 0x22F0, "Hammer image")
    _expect_ptr(model, root + 21 * 8 + 4, 0x23B0, "Hammer vertices 0")
    _expect_ptr(model, root + 36 * 8 + 4, 0x2470, "Hammer vertices 1")
    verts0 = _decode_verts(model, 0x23B0, 12)
    verts1 = _decode_verts(model, 0x2470, 24)
    tris0 = _decode_tris(raw, tuple((i, 0x06) for i in range(22, 28)))
    tris1 = _decode_tris(raw, tuple((i, 0x06) for i in range(37, 43)))
    header = _header("hammer", "HAMMER", (
        f"#define NDS_NATIVE_ITEM_HAMMER_ASSET {ASSET}u",
        f"#define NDS_NATIVE_ITEM_HAMMER_ROOT 0x{root:04x}u",
        f"#define NDS_NATIVE_ITEM_HAMMER_DL_BYTES {len(raw) * 8}u",
        f"#define NDS_NATIVE_ITEM_HAMMER_FILE_END 0x{root + len(raw) * 8:04x}u",
        "#define NDS_NATIVE_ITEM_HAMMER_TLUT_OFFSET 0x22c8u",
        f"#define NDS_NATIVE_ITEM_HAMMER_TLUT_W0 0x{raw[11][0]:08x}u",
        "#define NDS_NATIVE_ITEM_HAMMER_IMAGE_OFFSET 0x22f0u",
        f"#define NDS_NATIVE_ITEM_HAMMER_IMAGE_W0 0x{raw[17][0]:08x}u",
        "#define NDS_NATIVE_ITEM_HAMMER_VERTEX0_OFFSET 0x23b0u",
        "#define NDS_NATIVE_ITEM_HAMMER_VERTEX1_OFFSET 0x2470u",
        f"#define NDS_NATIVE_ITEM_HAMMER_VERTEX0_COUNT {len(verts0)}u",
        f"#define NDS_NATIVE_ITEM_HAMMER_TRIANGLE0_COUNT {len(tris0)}u",
        f"#define NDS_NATIVE_ITEM_HAMMER_CORNER0_COUNT {len(tris0) * 3}u",
        f"#define NDS_NATIVE_ITEM_HAMMER_VERTEX1_COUNT {len(verts1)}u",
        f"#define NDS_NATIVE_ITEM_HAMMER_TRIANGLE1_COUNT {len(tris1)}u",
        f"#define NDS_NATIVE_ITEM_HAMMER_CORNER1_COUNT {len(tris1) * 3}u",
    ))
    lines = [
        "/* Hammer packet, generated from file 86 root 0x25f0.",
        " * No 0xDE appears: palette, image, state and both geometry pools bake. */",
        "#include <nds/generated/nds_native_item_hammer.generated.h>", "",
        _arrays("Hammer0", verts0, tris0), _arrays("Hammer1", verts1, tris1),
        "static void ndsNativeItemHammerSetup0(",
        "    NDSRendererStats *stats, NDSRendererTraversalState *state,",
        "    const void *tlut, const void *image)", "{",
        *(f"    ndsRendererApplyMatrixMoveWordCommand(stats, state, 0x{raw[i][0]:08x}u, 0x{raw[i][1]:08x}u);" for i in (1, 2, 3, 4)),
        _othermode(raw, 5),
        f"    ndsRendererRecordSetCombine(stats, 0x{raw[6][0]:08x}u, 0x{raw[6][1]:08x}u);",
        *(f"    ndsRendererRecordSetTile(stats, 0x{raw[i][0]:08x}u, 0x{raw[i][1]:08x}u);" for i in (8, 9, 10)),
        f"    ndsRendererRecordSetImage(stats, 0x{raw[11][0]:08x}u, (u32)(uintptr_t)tlut);",
        f"    ndsRendererRecordLoadTlut(stats, 0x{raw[13][1]:08x}u);",
        f"    ndsRendererRecordTextureState(stats, 0x{raw[15][0]:08x}u, 0x{raw[15][1]:08x}u);",
        f"    ndsRendererRecordSetTileSize(stats, 0x{raw[16][0]:08x}u, 0x{raw[16][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{raw[17][0]:08x}u, (u32)(uintptr_t)image);",
        f"    ndsRendererRecordLoadBlock(stats, 0x{raw[19][0]:08x}u, 0x{raw[19][1]:08x}u);",
        "}", "",
        "static void ndsNativeItemHammerSetup1(",
        "    NDSRendererStats *stats, NDSRendererTraversalState *state)", "{",
        _othermode(raw, 29),
        *(f"    ndsRendererApplyMatrixMoveWordCommand(stats, state, 0x{raw[i][0]:08x}u, 0x{raw[i][1]:08x}u);" for i in (30, 31, 32, 33)),
        f"    ndsRendererRecordSetCombine(stats, 0x{raw[34][0]:08x}u, 0x{raw[34][1]:08x}u);",
        f"    ndsRendererRecordTextureState(stats, 0x{raw[35][0]:08x}u, 0x{raw[35][1]:08x}u);",
        "}", "",
    ]
    check = ("ITEM_HAMMER_NATIVE_OK root=0x25f0 verts=12+24 tris=12+12 "
             "material=none")
    return "\n".join(lines), header, check


MBALL0_OPS = (
    0xE7, 0xD9, 0xE3, 0xE2, 0xE2, 0xFC, 0xF9, 0xE8,
    0xF5, 0xF5, 0xF5, 0xFD, 0xE6, 0xF0, 0xE7, 0xD7,
    0xF2, 0xFD, 0xE6, 0xF3, 0xE7, 0x01, 0x06, 0xE7,
    0xE7, 0xD9, 0xE3, 0xE2, 0xE2, 0xDF,
)
MBALL1_OPS = (
    0xE7, 0xE3, 0xE2, 0xE2, 0xFC, 0xF9, 0xE8, 0xF5,
    0xF5, 0xF5, 0xFD, 0xE6, 0xF0, 0xE7, 0xD7, 0xF2,
    0xDE, 0xE6, 0xF3, 0xE7, 0xD9, 0x01, 0x06, 0xE7,
    0xE7, 0xD9, 0xE3, 0xE2, 0xE2, 0xDF,
)


def _mball(model, attr):
    root0, root1 = 0x9250, 0x9340
    a, b = _words(model, root0, 30), _words(model, root1, 30)
    _expect_ops(a, MBALL0_OPS, "MBall root 0x9250")
    _expect_ops(b, MBALL1_OPS, "MBall root 0x9340")
    _expect_de(a, (), "MBall root 0x9250")
    _expect_de(b, ((16, 0xDE000000, 0x0E000000),), "MBall root 0x9340")
    _expect_ptr(attr, 0x6E4, 0x9430, "MBall ITAttributes.data")
    _expect_ptr(attr, 0x6E8, 0x9120, "MBall ITAttributes.p_mobjsubs")
    _expect_null(attr, 0x6EC, "MBall ITAttributes.anim_joints")
    _expect_null(attr, 0x6F0, "MBall ITAttributes.p_matanim_joints")
    for slot in (0x9120, 0x9124, 0x9128):
        _expect_null(model, slot, f"MBall MObj head {slot:#x}")
    _expect_ptr(model, 0x912C, 0x91C8, "MBall live MObj head")
    _expect_ptr(model, 0x91C8, 0x9150, "MBall live MObjSub")
    flags = struct.unpack_from(">H", model.payload, 0x9150 + 0x30)[0]
    if flags != 0x0001:
        raise RuntimeError(f"MBall live MObj flags changed: {flags:#x}")
    images = _ptr_offsets(model, 0x9130, 8, "MBall live images")
    _expect_ptr(model, 0x948C, root0, "MBall root0 DObj")
    _expect_ptr(model, 0x94B8, root1, "MBall root1 DObj")
    for slot, off, label in (
        (root0 + 11 * 8 + 4, 0x7DB8, "MBall root0 TLUT"),
        (root0 + 17 * 8 + 4, 0x8E20, "MBall root0 image"),
        (root0 + 21 * 8 + 4, 0x91D0, "MBall root0 vertices"),
        (root1 + 10 * 8 + 4, 0x7D90, "MBall root1 TLUT"),
        (root1 + 21 * 8 + 4, 0x9210, "MBall root1 vertices"),
    ):
        _expect_ptr(model, slot, off, label)
    verts0 = _decode_verts(model, 0x91D0, 4)
    verts1 = _decode_verts(model, 0x9210, 4)
    tris0 = _decode_tris(a, ((22, 0x06),))
    tris1 = _decode_tris(b, ((22, 0x06),))
    header = _header("mball", "MBALL", (
        f"#define NDS_NATIVE_ITEM_MBALL_ASSET {ASSET}u",
        f"#define NDS_NATIVE_ITEM_MBALL_BAKED_ROOT 0x{root0:04x}u",
        f"#define NDS_NATIVE_ITEM_MBALL_LIVE_ROOT 0x{root1:04x}u",
        f"#define NDS_NATIVE_ITEM_MBALL_FILE_END 0x{root1 + len(b) * 8:04x}u",
        "#define NDS_NATIVE_ITEM_MBALL_BAKED_TLUT_OFFSET 0x7db8u",
        f"#define NDS_NATIVE_ITEM_MBALL_BAKED_TLUT_W0 0x{a[11][0]:08x}u",
        "#define NDS_NATIVE_ITEM_MBALL_BAKED_IMAGE_OFFSET 0x8e20u",
        f"#define NDS_NATIVE_ITEM_MBALL_BAKED_IMAGE_W0 0x{a[17][0]:08x}u",
        "#define NDS_NATIVE_ITEM_MBALL_BAKED_VERTEX_OFFSET 0x91d0u",
        "#define NDS_NATIVE_ITEM_MBALL_LIVE_TLUT_OFFSET 0x7d90u",
        f"#define NDS_NATIVE_ITEM_MBALL_LIVE_TLUT_W0 0x{b[10][0]:08x}u",
        "#define NDS_NATIVE_ITEM_MBALL_LIVE_HOOK_W0 0xde000000u",
        "#define NDS_NATIVE_ITEM_MBALL_LIVE_HOOK_W1 0x0e000000u",
        "#define NDS_NATIVE_ITEM_MBALL_LIVE_VERTEX_OFFSET 0x9210u",
        f"#define NDS_NATIVE_ITEM_MBALL_BAKED_VERTEX_COUNT {len(verts0)}u",
        f"#define NDS_NATIVE_ITEM_MBALL_BAKED_TRIANGLE_COUNT {len(tris0)}u",
        f"#define NDS_NATIVE_ITEM_MBALL_BAKED_CORNER_COUNT {len(tris0) * 3}u",
        f"#define NDS_NATIVE_ITEM_MBALL_LIVE_VERTEX_COUNT {len(verts1)}u",
        f"#define NDS_NATIVE_ITEM_MBALL_LIVE_TRIANGLE_COUNT {len(tris1)}u",
        f"#define NDS_NATIVE_ITEM_MBALL_LIVE_CORNER_COUNT {len(tris1) * 3}u",
    ))
    lines = [
        "/* Poké Ball / Monster Ball packet, generated from file 86 roots 0x9250/0x9340.",
        " * 0x9250 has no 0xDE. 0x9340 has one exact word-16 call to 0x0e000000",
        " * and its MObj flags 0x0001 make that hook CURRENT_IMAGE-only. */",
        "#include <nds/generated/nds_native_item_mball.generated.h>", "",
        _arrays("MBallBaked", verts0, tris0), _arrays("MBallLive", verts1, tris1),
        "static const u32 sNdsNativeItemMBallLiveImageOffsets[8] =", "{",
        *(f"    0x{x:04x}u," for x in images), "};", "",
        "static void ndsNativeItemMBallBakedSetup(NDSRendererStats *stats, const void *tlut, const void *image)", "{",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{a[1][0]:08x}u) | 0x{a[1][1]:08x}u;",
        _othermode(a, 2), _othermode(a, 3), _othermode(a, 4),
        f"    ndsRendererRecordSetCombine(stats, 0x{a[5][0]:08x}u, 0x{a[5][1]:08x}u);",
        f"    stats->blend_color = 0x{a[6][1]:08x}u;",
        *(f"    ndsRendererRecordSetTile(stats, 0x{a[i][0]:08x}u, 0x{a[i][1]:08x}u);" for i in (8, 9, 10)),
        f"    ndsRendererRecordSetImage(stats, 0x{a[11][0]:08x}u, (u32)(uintptr_t)tlut);",
        f"    ndsRendererRecordLoadTlut(stats, 0x{a[13][1]:08x}u);",
        f"    ndsRendererRecordTextureState(stats, 0x{a[15][0]:08x}u, 0x{a[15][1]:08x}u);",
        f"    ndsRendererRecordSetTileSize(stats, 0x{a[16][0]:08x}u, 0x{a[16][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{a[17][0]:08x}u, (u32)(uintptr_t)image);",
        f"    ndsRendererRecordLoadBlock(stats, 0x{a[19][0]:08x}u, 0x{a[19][1]:08x}u);",
        "}", "",
        "static void ndsNativeItemMBallBakedFinish(NDSRendererStats *stats)", "{",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{a[25][0]:08x}u) | 0x{a[25][1]:08x}u;",
        _othermode(a, 26), _othermode(a, 27), _othermode(a, 28), "}", "",
        "static void ndsNativeItemMBallLiveSetup(NDSRendererStats *stats, const void *tlut)", "{",
        _othermode(b, 1), _othermode(b, 2), _othermode(b, 3),
        f"    ndsRendererRecordSetCombine(stats, 0x{b[4][0]:08x}u, 0x{b[4][1]:08x}u);",
        f"    stats->blend_color = 0x{b[5][1]:08x}u;",
        *(f"    ndsRendererRecordSetTile(stats, 0x{b[i][0]:08x}u, 0x{b[i][1]:08x}u);" for i in (7, 8, 9)),
        f"    ndsRendererRecordSetImage(stats, 0x{b[10][0]:08x}u, (u32)(uintptr_t)tlut);",
        f"    ndsRendererRecordLoadTlut(stats, 0x{b[12][1]:08x}u);",
        f"    ndsRendererRecordTextureState(stats, 0x{b[14][0]:08x}u, 0x{b[14][1]:08x}u);",
        f"    ndsRendererRecordSetTileSize(stats, 0x{b[15][0]:08x}u, 0x{b[15][1]:08x}u);",
        "}", "",
        "static void ndsNativeItemMBallLiveAfterMaterial(NDSRendererStats *stats)", "{",
        f"    ndsRendererRecordLoadBlock(stats, 0x{b[18][0]:08x}u, 0x{b[18][1]:08x}u);",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{b[20][0]:08x}u) | 0x{b[20][1]:08x}u;",
        "}", "",
        "static void ndsNativeItemMBallLiveFinish(NDSRendererStats *stats)", "{",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{b[25][0]:08x}u) | 0x{b[25][1]:08x}u;",
        _othermode(b, 26), _othermode(b, 27), _othermode(b, 28), "}", "",
    ]
    check = ("ITEM_MBALL_NATIVE_OK roots=0x9250,0x9340 verts=4+4 tris=2+2 "
             "material=none,CURRENT_IMAGE hook=word16")
    return "\n".join(lines), header, check


GSHELL_OPS = (
    0xE7, 0xE3, 0xE2, 0xE2, 0xFC, 0xF9, 0xE8, 0xF5,
    0xF5, 0xF5, 0xD7, 0xF2, 0xDE, 0xE6, 0xF3, 0xE7,
    0xD9, 0x01, 0x06, 0xE7, 0xD9, 0xE3, 0xE2, 0xE2, 0xDF,
)


def _gshell(model, attr):
    root = 0x5EC0
    raw = _words(model, root, 25)
    _expect_ops(raw, GSHELL_OPS, "Green Shell")
    _expect_de(raw, ((12, 0xDE000000, 0x0E000000),), "Green Shell")
    _expect_ptr(attr, 0x53C, 0x5F88, "Green Shell ITAttributes.data")
    _expect_ptr(attr, 0x540, 0x5DE0, "Green Shell ITAttributes.p_mobjsubs")
    _expect_null(attr, 0x544, "Green Shell ITAttributes.anim_joints")
    _expect_null(attr, 0x548, "Green Shell ITAttributes.p_matanim_joints")
    _expect_null(model, 0x5DE0, "Green Shell MObj head 0")
    _expect_ptr(model, 0x5DE4, 0x5E78, "Green Shell MObj head 1")
    _expect_ptr(model, 0x5E78, 0x5E00, "Green Shell MObjSub")
    flags = struct.unpack_from(">H", model.payload, 0x5E00 + 0x30)[0]
    if flags != 0x0005:
        raise RuntimeError(f"Green Shell MObj flags changed: {flags:#x}")
    images = _ptr_offsets(model, 0x5DE8, 4, "Green Shell images")
    palettes = _ptr_offsets(model, 0x5DF8, 2, "Green Shell palettes")
    _expect_ptr(model, 0x5FB8, root, "Green Shell DObj root")
    _expect_ptr(model, root + 17 * 8 + 4, 0x5E80, "Green Shell vertices")
    verts = _decode_verts(model, 0x5E80, 4)
    tris = _decode_tris(raw, ((18, 0x06),))
    header = _header("gshell", "GSHELL", (
        f"#define NDS_NATIVE_ITEM_GSHELL_ASSET {ASSET}u",
        f"#define NDS_NATIVE_ITEM_GSHELL_ROOT 0x{root:04x}u",
        f"#define NDS_NATIVE_ITEM_GSHELL_DL_BYTES {len(raw) * 8}u",
        f"#define NDS_NATIVE_ITEM_GSHELL_FILE_END 0x{root + len(raw) * 8:04x}u",
        "#define NDS_NATIVE_ITEM_GSHELL_HOOK_W0 0xde000000u",
        "#define NDS_NATIVE_ITEM_GSHELL_HOOK_W1 0x0e000000u",
        "#define NDS_NATIVE_ITEM_GSHELL_VERTEX_OFFSET 0x5e80u",
        f"#define NDS_NATIVE_ITEM_GSHELL_VERTEX_COUNT {len(verts)}u",
        f"#define NDS_NATIVE_ITEM_GSHELL_TRIANGLE_COUNT {len(tris)}u",
        f"#define NDS_NATIVE_ITEM_GSHELL_CORNER_COUNT {len(tris) * 3}u",
    ))
    lines = [
        "/* Green Shell packet, generated from file 86 root 0x5ec0.",
        " * Exact word-12 call 0xDE000000 0x0E000000 is live. MObj flags 0x0005",
        " * require PALETTE_IMAGE|PALETTE_TLUT|CURRENT_IMAGE exactly. */",
        "#include <nds/generated/nds_native_item_gshell.generated.h>", "",
        _arrays("GShell", verts, tris),
        "static const u32 sNdsNativeItemGShellImageOffsets[4] =", "{",
        *(f"    0x{x:04x}u," for x in images), "};", "",
        "static const u32 sNdsNativeItemGShellPaletteOffsets[2] =", "{",
        *(f"    0x{x:04x}u," for x in palettes), "};", "",
        "static void ndsNativeItemGShellSetup(NDSRendererStats *stats)", "{",
        _othermode(raw, 1), _othermode(raw, 2), _othermode(raw, 3),
        f"    ndsRendererRecordSetCombine(stats, 0x{raw[4][0]:08x}u, 0x{raw[4][1]:08x}u);",
        f"    stats->blend_color = 0x{raw[5][1]:08x}u;",
        *(f"    ndsRendererRecordSetTile(stats, 0x{raw[i][0]:08x}u, 0x{raw[i][1]:08x}u);" for i in (7, 8, 9)),
        f"    ndsRendererRecordTextureState(stats, 0x{raw[10][0]:08x}u, 0x{raw[10][1]:08x}u);",
        f"    ndsRendererRecordSetTileSize(stats, 0x{raw[11][0]:08x}u, 0x{raw[11][1]:08x}u);",
        "}", "",
        "static void ndsNativeItemGShellAfterMaterial(NDSRendererStats *stats)", "{",
        f"    ndsRendererRecordLoadBlock(stats, 0x{raw[14][0]:08x}u, 0x{raw[14][1]:08x}u);",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[16][0]:08x}u) | 0x{raw[16][1]:08x}u;",
        "}", "",
        "static void ndsNativeItemGShellFinish(NDSRendererStats *stats)", "{",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[20][0]:08x}u) | 0x{raw[20][1]:08x}u;",
        _othermode(raw, 21), _othermode(raw, 22), _othermode(raw, 23), "}", "",
    ]
    check = ("ITEM_GSHELL_NATIVE_OK root=0x5ec0 verts=4 tris=2 "
             "material=PALETTE_IMAGE|PALETTE_TLUT|CURRENT_IMAGE hook=word12")
    return "\n".join(lines), header, check


GENERATORS = {
    "star": _star,
    "sword": _sword,
    "hammer": _hammer,
    "mball": _mball,
    "gshell": _gshell,
}


def run(slug: str) -> int:
    if slug not in GENERATORS:
        raise RuntimeError(f"unknown item generator {slug!r}")
    ap = argparse.ArgumentParser()
    ap.add_argument("--emit", action="store_true")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    model = sm.load_o2r(REPO, MODEL_FILE)
    attr = sm.load_o2r(REPO, ATTR_FILE)
    packet, header, check_line = GENERATORS[slug](model, attr)
    packet_path, header_path = _paths(slug)
    outputs = ((packet_path, packet), (header_path, header))
    if args.emit:
        for path, body in outputs:
            path.parent.mkdir(parents=True, exist_ok=True)
            if not path.exists() or path.read_text() != body:
                path.write_text(body)
        print("emitted " + " and ".join(str(p.relative_to(REPO)) for p, _ in outputs))
    if args.check or not args.emit:
        for path, body in outputs:
            if not path.exists() or path.read_text() != body:
                raise RuntimeError(f"generated artefact stale: {path.relative_to(REPO)}")
        print(check_line)
    return 0
