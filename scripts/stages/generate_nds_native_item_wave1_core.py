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


BAT_HEADER_OPS = (
    0xE7, 0xD9, 0xE3, 0xFC, 0xF9, 0xE8, 0xF5, 0xF5, 0xD7, 0xDE, 0xDF,
)
BAT_CALLEE_OPS = (
    0xE7, 0xE2, 0xE2, 0xF5, 0xFD, 0xE6, 0xF0, 0xE7,
    0xF2, 0xFD, 0xE6, 0xF3, 0xE7, 0x01, 0x05, 0xE7,
    0xE2, 0xE2, 0xF5, 0xFD, 0xE6, 0xF0, 0xE7, 0xF2,
    0xFD, 0xE6, 0xF3, 0xE7, 0x01, 0x06, 0x05, 0xDF,
)
BAT_SECOND_OPS = (
    0xE7, 0xE2, 0xE2, 0xF5, 0xFD, 0xE6, 0xF0, 0xE7,
    0xF2, 0xFD, 0xE6, 0xF3, 0xE7, 0xD9, 0x01, 0x06,
    0xE7, 0xE7, 0xD9, 0xE3, 0xE2, 0xE2, 0xDF,
)


def _bat(model, attr):
    """Bespoke branch-shaped Bat owner; the segment-7 callee is inlined."""
    header_root, callee_root, second_root = 0x1BF0, 0x1C48, 0x1D48
    header_raw = _words(model, header_root, 11)
    callee_raw = _words(model, callee_root, 32)
    second_raw = _words(model, second_root, 23)
    _expect_ops(header_raw, BAT_HEADER_OPS, "Bat header")
    _expect_ops(callee_raw, BAT_CALLEE_OPS, "Bat callee")
    _expect_ops(second_raw, BAT_SECOND_OPS, "Bat second root")
    _expect_de(header_raw, ((9, 0xDE000000, 0x071B0712),), "Bat header")
    _expect_de(callee_raw, (), "Bat callee")
    _expect_de(second_raw, (), "Bat second root")
    _expect_ptr(attr, 0x1D8, 0x1E00, "Bat ITAttributes.data")
    for slot, label in ((0x1DC, "p_mobjsubs"), (0x1E0, "anim_joints"),
                        (0x1E4, "p_matanim_joints")):
        _expect_null(attr, slot, f"Bat ITAttributes.{label}")
    if ((struct.unpack_from(">i", model.payload, 0x1E00)[0] != 0) or
            (model.pointer_at(0x1E04) is not None)):
        raise RuntimeError("Bat DObjDesc root changed")
    if struct.unpack_from(">i", model.payload, 0x1E2C)[0] != 1:
        raise RuntimeError("Bat DObjDesc child 1 id changed")
    _expect_ptr(model, 0x1E30, header_root, "Bat DObjDesc child 1")
    if struct.unpack_from(">i", model.payload, 0x1E58)[0] != 0x4002:
        raise RuntimeError("Bat DObjDesc child 2 id changed")
    _expect_ptr(model, 0x1E5C, second_root, "Bat DObjDesc child 2")
    if struct.unpack_from(">i", model.payload, 0x1E84)[0] != 18:
        raise RuntimeError("Bat DObjDesc terminator changed")
    _expect_ptr(model, header_root + 9 * 8 + 4, callee_root,
                "Bat segment-7 branch callee")
    for slot, off, label in (
        (callee_root + 4 * 8 + 4, 0x19D8, "Bat TLUT 0"),
        (callee_root + 9 * 8 + 4, 0x1A68, "Bat image 0"),
        (callee_root + 13 * 8 + 4, 0x1AF0, "Bat vertices 0"),
        (callee_root + 19 * 8 + 4, 0x1A20, "Bat TLUT 1"),
        (callee_root + 24 * 8 + 4, 0x1AB0, "Bat image 1"),
        (callee_root + 28 * 8 + 4, 0x1B20, "Bat vertices 1"),
        (second_root + 4 * 8 + 4, 0x19D8, "Bat second TLUT"),
        (second_root + 9 * 8 + 4, 0x1A68, "Bat second image"),
        (second_root + 14 * 8 + 4, 0x1BB0, "Bat vertices 2"),
    ):
        _expect_ptr(model, slot, off, label)
    verts0 = _decode_verts(model, 0x1AF0, 3)
    verts1 = _decode_verts(model, 0x1B20, 9)
    verts2 = _decode_verts(model, 0x1BB0, 4)
    tris0 = _decode_tris(callee_raw, ((14, 0x05),))
    tris1 = _decode_tris(callee_raw, ((29, 0x06), (30, 0x05)))
    tris2 = _decode_tris(second_raw, ((15, 0x06),))
    packet_bytes = (
        len(verts0) * 14 + len(tris0) * 6 +
        len(verts1) * 14 + len(tris1) * 6 +
        len(verts2) * 14 + len(tris2) * 6
    )
    header = _header("bat", "BAT", (
        f"#define NDS_NATIVE_ITEM_BAT_ASSET {ASSET}u",
        f"#define NDS_NATIVE_ITEM_BAT_HEADER_ROOT 0x{header_root:04x}u",
        f"#define NDS_NATIVE_ITEM_BAT_CALLEE_ROOT 0x{callee_root:04x}u",
        f"#define NDS_NATIVE_ITEM_BAT_SECOND_ROOT 0x{second_root:04x}u",
        "#define NDS_NATIVE_ITEM_BAT_FILE_END 0x1e00u",
        "#define NDS_NATIVE_ITEM_BAT_BRANCH_W0 0xde000000u",
        "#define NDS_NATIVE_ITEM_BAT_BRANCH_RAW_W1 0x071b0712u",
        "#define NDS_NATIVE_ITEM_BAT_TLUT0_OFFSET 0x19d8u",
        "#define NDS_NATIVE_ITEM_BAT_IMAGE0_OFFSET 0x1a68u",
        "#define NDS_NATIVE_ITEM_BAT_TLUT1_OFFSET 0x1a20u",
        "#define NDS_NATIVE_ITEM_BAT_IMAGE1_OFFSET 0x1ab0u",
        "#define NDS_NATIVE_ITEM_BAT_VERTEX0_OFFSET 0x1af0u",
        "#define NDS_NATIVE_ITEM_BAT_VERTEX1_OFFSET 0x1b20u",
        "#define NDS_NATIVE_ITEM_BAT_VERTEX2_OFFSET 0x1bb0u",
        f"#define NDS_NATIVE_ITEM_BAT_VERTEX0_COUNT {len(verts0)}u",
        f"#define NDS_NATIVE_ITEM_BAT_TRIANGLE0_COUNT {len(tris0)}u",
        f"#define NDS_NATIVE_ITEM_BAT_CORNER0_COUNT {len(tris0) * 3}u",
        f"#define NDS_NATIVE_ITEM_BAT_VERTEX1_COUNT {len(verts1)}u",
        f"#define NDS_NATIVE_ITEM_BAT_TRIANGLE1_COUNT {len(tris1)}u",
        f"#define NDS_NATIVE_ITEM_BAT_CORNER1_COUNT {len(tris1) * 3}u",
        f"#define NDS_NATIVE_ITEM_BAT_VERTEX2_COUNT {len(verts2)}u",
        f"#define NDS_NATIVE_ITEM_BAT_TRIANGLE2_COUNT {len(tris2)}u",
        f"#define NDS_NATIVE_ITEM_BAT_CORNER2_COUNT {len(tris2) * 3}u",
        f"#define NDS_NATIVE_ITEM_BAT_PACKET_ROM_BYTES {packet_bytes}u",
        f"#define NDS_NATIVE_ITEM_BAT_PACKET_RAM_BYTES {packet_bytes}u",
    ))
    lines = [
        "/* Home-Run Bat native packet, generated from file 86.",
        " * Root 0x1bf0 word 9 is the exact branch 0xDE000000 0x071B0712;",
        " * its relocated callee is 0x1c48 and is inlined here. Root 0x1d48",
        " * is bake-only. No live material survives either DObj. */",
        "#include <nds/generated/nds_native_item_bat.generated.h>", "",
        _arrays("Bat0", verts0, tris0),
        _arrays("Bat1", verts1, tris1),
        _arrays("Bat2", verts2, tris2),
        "static void ndsNativeItemBatHeaderSetup(NDSRendererStats *stats)", "{",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{header_raw[1][0]:08x}u) | 0x{header_raw[1][1]:08x}u;",
        _othermode(header_raw, 2),
        f"    ndsRendererRecordSetCombine(stats, 0x{header_raw[3][0]:08x}u, 0x{header_raw[3][1]:08x}u);",
        f"    stats->blend_color = 0x{header_raw[4][1]:08x}u;",
        *(f"    ndsRendererRecordSetTile(stats, 0x{header_raw[i][0]:08x}u, 0x{header_raw[i][1]:08x}u);" for i in (6, 7)),
        f"    ndsRendererRecordTextureState(stats, 0x{header_raw[8][0]:08x}u, 0x{header_raw[8][1]:08x}u);",
        "}", "",
        "static void ndsNativeItemBatRun0Setup(NDSRendererStats *stats, const void *tlut, const void *image)", "{",
        _othermode(callee_raw, 1), _othermode(callee_raw, 2),
        f"    ndsRendererRecordSetTile(stats, 0x{callee_raw[3][0]:08x}u, 0x{callee_raw[3][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{callee_raw[4][0]:08x}u, (u32)(uintptr_t)tlut);",
        f"    ndsRendererRecordLoadTlut(stats, 0x{callee_raw[6][1]:08x}u);",
        f"    ndsRendererRecordSetTileSize(stats, 0x{callee_raw[8][0]:08x}u, 0x{callee_raw[8][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{callee_raw[9][0]:08x}u, (u32)(uintptr_t)image);",
        f"    ndsRendererRecordLoadBlock(stats, 0x{callee_raw[11][0]:08x}u, 0x{callee_raw[11][1]:08x}u);",
        "}", "",
        "static void ndsNativeItemBatRun1Setup(NDSRendererStats *stats, const void *tlut, const void *image)", "{",
        _othermode(callee_raw, 16), _othermode(callee_raw, 17),
        f"    ndsRendererRecordSetTile(stats, 0x{callee_raw[18][0]:08x}u, 0x{callee_raw[18][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{callee_raw[19][0]:08x}u, (u32)(uintptr_t)tlut);",
        f"    ndsRendererRecordLoadTlut(stats, 0x{callee_raw[21][1]:08x}u);",
        f"    ndsRendererRecordSetTileSize(stats, 0x{callee_raw[23][0]:08x}u, 0x{callee_raw[23][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{callee_raw[24][0]:08x}u, (u32)(uintptr_t)image);",
        f"    ndsRendererRecordLoadBlock(stats, 0x{callee_raw[26][0]:08x}u, 0x{callee_raw[26][1]:08x}u);",
        "}", "",
        "static void ndsNativeItemBatSecondSetup(NDSRendererStats *stats, const void *tlut, const void *image)", "{",
        _othermode(second_raw, 1), _othermode(second_raw, 2),
        f"    ndsRendererRecordSetTile(stats, 0x{second_raw[3][0]:08x}u, 0x{second_raw[3][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{second_raw[4][0]:08x}u, (u32)(uintptr_t)tlut);",
        f"    ndsRendererRecordLoadTlut(stats, 0x{second_raw[6][1]:08x}u);",
        f"    ndsRendererRecordSetTileSize(stats, 0x{second_raw[8][0]:08x}u, 0x{second_raw[8][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{second_raw[9][0]:08x}u, (u32)(uintptr_t)image);",
        f"    ndsRendererRecordLoadBlock(stats, 0x{second_raw[11][0]:08x}u, 0x{second_raw[11][1]:08x}u);",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{second_raw[13][0]:08x}u) | 0x{second_raw[13][1]:08x}u;",
        "}", "",
        "static void ndsNativeItemBatSecondFinish(NDSRendererStats *stats)", "{",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{second_raw[18][0]:08x}u) | 0x{second_raw[18][1]:08x}u;",
        _othermode(second_raw, 19), _othermode(second_raw, 20), _othermode(second_raw, 21),
        "}", "",
    ]
    check = (
        "ITEM_BAT_NATIVE_OK roots=0x1bf0->0x1c48,0x1d48 verts=3+9+4 "
        "tris=1+3+2 material=none branch=word9:0x071b0712"
    )
    return "\n".join(lines), header, check


HARISEN_OPS = (
    0xE7, 0xDB, 0xDB, 0xDB, 0xDB, 0xE3, 0xFC, 0xE8,
    0xF5, 0xF5, 0xF5, 0xFD, 0xE6, 0xF0, 0xE7, 0xD7,
    0xF2, 0xFD, 0xE6, 0xF3, 0xE7, 0xD9, 0x01, 0x06,
    0x06, 0x06, 0x06, 0xE7, 0xD9, 0xE3, 0xDF,
)


def _harisen(model, attr):
    root = 0x20A0
    raw = _words(model, root, 31)
    _expect_ops(raw, HARISEN_OPS, "Harisen")
    _expect_de(raw, (), "Harisen")
    _expect_ptr(attr, 0x220, 0x2198, "Harisen ITAttributes.data")
    for slot, label in ((0x224, "p_mobjsubs"), (0x228, "anim_joints"),
                        (0x22C, "p_matanim_joints")):
        _expect_null(attr, slot, f"Harisen ITAttributes.{label}")
    _expect_null(model, 0x219C, "Harisen DObj child 0")
    _expect_null(model, 0x21C8, "Harisen DObj child 1")
    _expect_ptr(model, 0x21F4, root, "Harisen DObj drawable child")
    for slot, off, label in (
        (root + 11 * 8 + 4, 0x1EB8, "Harisen TLUT"),
        (root + 17 * 8 + 4, 0x1EE0, "Harisen image"),
        (root + 22 * 8 + 4, 0x1F20, "Harisen vertices"),
    ):
        _expect_ptr(model, slot, off, label)
    verts = _decode_verts(model, 0x1F20, 24)
    tris = _decode_tris(raw, tuple((i, 0x06) for i in range(23, 27)))
    packet_bytes = len(verts) * 14 + len(tris) * 6
    header = _header("harisen", "HARISEN", (
        f"#define NDS_NATIVE_ITEM_HARISEN_ASSET {ASSET}u",
        f"#define NDS_NATIVE_ITEM_HARISEN_ROOT 0x{root:04x}u",
        "#define NDS_NATIVE_ITEM_HARISEN_FILE_END 0x2198u",
        "#define NDS_NATIVE_ITEM_HARISEN_TLUT_OFFSET 0x1eb8u",
        f"#define NDS_NATIVE_ITEM_HARISEN_TLUT_W0 0x{raw[11][0]:08x}u",
        "#define NDS_NATIVE_ITEM_HARISEN_IMAGE_OFFSET 0x1ee0u",
        f"#define NDS_NATIVE_ITEM_HARISEN_IMAGE_W0 0x{raw[17][0]:08x}u",
        "#define NDS_NATIVE_ITEM_HARISEN_VERTEX_OFFSET 0x1f20u",
        f"#define NDS_NATIVE_ITEM_HARISEN_VERTEX_COUNT {len(verts)}u",
        f"#define NDS_NATIVE_ITEM_HARISEN_TRIANGLE_COUNT {len(tris)}u",
        f"#define NDS_NATIVE_ITEM_HARISEN_CORNER_COUNT {len(tris) * 3}u",
        f"#define NDS_NATIVE_ITEM_HARISEN_PACKET_ROM_BYTES {packet_bytes}u",
        f"#define NDS_NATIVE_ITEM_HARISEN_PACKET_RAM_BYTES {packet_bytes}u",
    ))
    lines = [
        "/* Fan / Harisen packet, generated from file 86 root 0x20a0.",
        " * No 0xDE appears: TLUT, image, state and geometry all bake. */",
        "#include <nds/generated/nds_native_item_harisen.generated.h>", "",
        _arrays("Harisen", verts, tris),
        "static void ndsNativeItemHarisenSetup(NDSRendererStats *stats, NDSRendererTraversalState *state, const void *tlut, const void *image)", "{",
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
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[21][0]:08x}u) | 0x{raw[21][1]:08x}u;",
        "}", "",
        "static void ndsNativeItemHarisenFinish(NDSRendererStats *stats)", "{",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[28][0]:08x}u) | 0x{raw[28][1]:08x}u;",
        _othermode(raw, 29), "}", "",
    ]
    check = "ITEM_HARISEN_NATIVE_OK root=0x20a0 verts=24 tris=8 material=none"
    return "\n".join(lines), header, check


LGUN_OPS = (
    0xE7, 0xDB, 0xDB, 0xDB, 0xDB, 0xE3, 0xFC, 0xE8,
    0xF5, 0xF5, 0xF5, 0xFD, 0xE6, 0xF0, 0xE7, 0xD7,
    0xF2, 0xFD, 0xE6, 0xF3, 0xE7, 0xD9, 0x01, 0x06,
    0xE7, 0xD9, 0xF5, 0xF2, 0xFD, 0xE6, 0xF3, 0xE7,
    0x01, 0x06, 0x06, 0xE7, 0xF5, 0xF2, 0xFD, 0xE6,
    0xF3, 0xE7, 0xD9, 0x01, 0x06, 0x06, 0x06, 0x05,
    0xE7, 0xD9, 0xE3, 0xDF,
)


def _lgun(model, attr):
    root = 0x3DB0
    raw = _words(model, root, 52)
    _expect_ops(raw, LGUN_OPS, "LGun")
    _expect_de(raw, (), "LGun")
    _expect_ptr(attr, 0x268, 0x3F50, "LGun ITAttributes.data")
    for slot, label in ((0x26C, "p_mobjsubs"), (0x270, "anim_joints"),
                        (0x274, "p_matanim_joints")):
        _expect_null(attr, slot, f"LGun ITAttributes.{label}")
    _expect_ptr(model, 0x3F80, root, "LGun DObj drawable child")
    for slot, off, label in (
        (root + 11 * 8 + 4, 0x3A88, "LGun TLUT"),
        (root + 17 * 8 + 4, 0x3B80, "LGun image 0"),
        (root + 22 * 8 + 4, 0x3BC0, "LGun vertices 0"),
        (root + 28 * 8 + 4, 0x3B38, "LGun image 1"),
        (root + 32 * 8 + 4, 0x3C00, "LGun vertices 1"),
        (root + 38 * 8 + 4, 0x3AF0, "LGun image 2"),
        (root + 43 * 8 + 4, 0x3CC0, "LGun vertices 2"),
    ):
        _expect_ptr(model, slot, off, label)
    verts0 = _decode_verts(model, 0x3BC0, 4)
    verts1 = _decode_verts(model, 0x3C00, 12)
    verts2 = _decode_verts(model, 0x3CC0, 15)
    tris0 = _decode_tris(raw, ((23, 0x06),))
    tris1 = _decode_tris(raw, ((33, 0x06), (34, 0x06)))
    tris2 = _decode_tris(raw, ((44, 0x06), (45, 0x06), (46, 0x06), (47, 0x05)))
    packet_bytes = (
        len(verts0) * 14 + len(tris0) * 6 +
        len(verts1) * 14 + len(tris1) * 6 +
        len(verts2) * 14 + len(tris2) * 6
    )
    header = _header("lgun", "LGUN", (
        f"#define NDS_NATIVE_ITEM_LGUN_ASSET {ASSET}u",
        f"#define NDS_NATIVE_ITEM_LGUN_ROOT 0x{root:04x}u",
        "#define NDS_NATIVE_ITEM_LGUN_FILE_END 0x3f50u",
        "#define NDS_NATIVE_ITEM_LGUN_TLUT_OFFSET 0x3a88u",
        f"#define NDS_NATIVE_ITEM_LGUN_TLUT_W0 0x{raw[11][0]:08x}u",
        "#define NDS_NATIVE_ITEM_LGUN_IMAGE0_OFFSET 0x3b80u",
        "#define NDS_NATIVE_ITEM_LGUN_IMAGE1_OFFSET 0x3b38u",
        "#define NDS_NATIVE_ITEM_LGUN_IMAGE2_OFFSET 0x3af0u",
        "#define NDS_NATIVE_ITEM_LGUN_VERTEX0_OFFSET 0x3bc0u",
        "#define NDS_NATIVE_ITEM_LGUN_VERTEX1_OFFSET 0x3c00u",
        "#define NDS_NATIVE_ITEM_LGUN_VERTEX2_OFFSET 0x3cc0u",
        f"#define NDS_NATIVE_ITEM_LGUN_VERTEX0_COUNT {len(verts0)}u",
        f"#define NDS_NATIVE_ITEM_LGUN_TRIANGLE0_COUNT {len(tris0)}u",
        f"#define NDS_NATIVE_ITEM_LGUN_CORNER0_COUNT {len(tris0) * 3}u",
        f"#define NDS_NATIVE_ITEM_LGUN_VERTEX1_COUNT {len(verts1)}u",
        f"#define NDS_NATIVE_ITEM_LGUN_TRIANGLE1_COUNT {len(tris1)}u",
        f"#define NDS_NATIVE_ITEM_LGUN_CORNER1_COUNT {len(tris1) * 3}u",
        f"#define NDS_NATIVE_ITEM_LGUN_VERTEX2_COUNT {len(verts2)}u",
        f"#define NDS_NATIVE_ITEM_LGUN_TRIANGLE2_COUNT {len(tris2)}u",
        f"#define NDS_NATIVE_ITEM_LGUN_CORNER2_COUNT {len(tris2) * 3}u",
        f"#define NDS_NATIVE_ITEM_LGUN_PACKET_ROM_BYTES {packet_bytes}u",
        f"#define NDS_NATIVE_ITEM_LGUN_PACKET_RAM_BYTES {packet_bytes}u",
    ))
    lines = [
        "/* Ray Gun / LGun packet, generated from file 86 root 0x3db0.",
        " * No 0xDE appears; all three fixed geometry/image phases bake. */",
        "#include <nds/generated/nds_native_item_lgun.generated.h>", "",
        _arrays("LGun0", verts0, tris0),
        _arrays("LGun1", verts1, tris1),
        _arrays("LGun2", verts2, tris2),
        "static void ndsNativeItemLGunSetup0(NDSRendererStats *stats, NDSRendererTraversalState *state, const void *tlut, const void *image)", "{",
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
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[21][0]:08x}u) | 0x{raw[21][1]:08x}u;",
        "}", "",
        "static void ndsNativeItemLGunSetup1(NDSRendererStats *stats, const void *image)", "{",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[25][0]:08x}u) | 0x{raw[25][1]:08x}u;",
        f"    ndsRendererRecordSetTile(stats, 0x{raw[26][0]:08x}u, 0x{raw[26][1]:08x}u);",
        f"    ndsRendererRecordSetTileSize(stats, 0x{raw[27][0]:08x}u, 0x{raw[27][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{raw[28][0]:08x}u, (u32)(uintptr_t)image);",
        f"    ndsRendererRecordLoadBlock(stats, 0x{raw[30][0]:08x}u, 0x{raw[30][1]:08x}u);",
        "}", "",
        "static void ndsNativeItemLGunSetup2(NDSRendererStats *stats, const void *image)", "{",
        f"    ndsRendererRecordSetTile(stats, 0x{raw[36][0]:08x}u, 0x{raw[36][1]:08x}u);",
        f"    ndsRendererRecordSetTileSize(stats, 0x{raw[37][0]:08x}u, 0x{raw[37][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{raw[38][0]:08x}u, (u32)(uintptr_t)image);",
        f"    ndsRendererRecordLoadBlock(stats, 0x{raw[40][0]:08x}u, 0x{raw[40][1]:08x}u);",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[42][0]:08x}u) | 0x{raw[42][1]:08x}u;",
        "}", "",
        "static void ndsNativeItemLGunFinish(NDSRendererStats *stats)", "{",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[49][0]:08x}u) | 0x{raw[49][1]:08x}u;",
        _othermode(raw, 50), "}", "",
    ]
    check = "ITEM_LGUN_NATIVE_OK root=0x3db0 verts=4+12+15 tris=2+4+7 material=none"
    return "\n".join(lines), header, check


BOMBHEI_OPS = (
    0xE7, 0xD9, 0xE3, 0xE2, 0xE2, 0xFC, 0xF9, 0xE8,
    0xF5, 0xF5, 0xF5, 0xFD, 0xE6, 0xF0, 0xE7, 0xD7,
    0xF2, 0xDE, 0xE6, 0xF3, 0xE7, 0x01, 0x06, 0xE7,
    0xD9, 0xE3, 0xE2, 0xE2, 0xDF,
)


def _bombhei(model, attr):
    root = 0x3310
    raw = _words(model, root, 29)
    _expect_ops(raw, BOMBHEI_OPS, "BombHei")
    _expect_de(raw, ((17, 0xDE000000, 0x0E000000),), "BombHei")
    _expect_ptr(attr, 0x424, 0x33F8, "BombHei ITAttributes.data")
    _expect_ptr(attr, 0x428, 0x3230, "BombHei ITAttributes.p_mobjsubs")
    _expect_null(attr, 0x42C, "BombHei ITAttributes.anim_joints")
    _expect_null(attr, 0x430, "BombHei ITAttributes.p_matanim_joints")
    _expect_null(model, 0x3230, "BombHei MObj head 0")
    _expect_ptr(model, 0x3234, 0x32C8, "BombHei MObj head 1")
    _expect_ptr(model, 0x32C8, 0x3250, "BombHei MObjSub")
    flags = struct.unpack_from(">H", model.payload, 0x3250 + 0x30)[0]
    if flags != 0x0001:
        raise RuntimeError(f"BombHei MObj flags changed: {flags:#x}")
    _expect_ptr(model, 0x3254, 0x3238, "BombHei image table")
    images = _ptr_offsets(model, 0x3238, 5, "BombHei live images")
    _expect_ptr(model, 0x3428, root, "BombHei DObj drawable child")
    _expect_ptr(model, root + 11 * 8 + 4, 0x27E8, "BombHei TLUT")
    _expect_ptr(model, root + 21 * 8 + 4, 0x32D0, "BombHei vertices")
    verts = _decode_verts(model, 0x32D0, 4)
    tris = _decode_tris(raw, ((22, 0x06),))
    packet_bytes = len(verts) * 14 + len(tris) * 6 + len(images) * 4
    header = _header("bombhei", "BOMBHEI", (
        f"#define NDS_NATIVE_ITEM_BOMBHEI_ASSET {ASSET}u",
        f"#define NDS_NATIVE_ITEM_BOMBHEI_ROOT 0x{root:04x}u",
        "#define NDS_NATIVE_ITEM_BOMBHEI_FILE_END 0x33f8u",
        "#define NDS_NATIVE_ITEM_BOMBHEI_TLUT_OFFSET 0x27e8u",
        f"#define NDS_NATIVE_ITEM_BOMBHEI_TLUT_W0 0x{raw[11][0]:08x}u",
        "#define NDS_NATIVE_ITEM_BOMBHEI_HOOK_W0 0xde000000u",
        "#define NDS_NATIVE_ITEM_BOMBHEI_HOOK_W1 0x0e000000u",
        "#define NDS_NATIVE_ITEM_BOMBHEI_VERTEX_OFFSET 0x32d0u",
        f"#define NDS_NATIVE_ITEM_BOMBHEI_VERTEX_COUNT {len(verts)}u",
        f"#define NDS_NATIVE_ITEM_BOMBHEI_TRIANGLE_COUNT {len(tris)}u",
        f"#define NDS_NATIVE_ITEM_BOMBHEI_CORNER_COUNT {len(tris) * 3}u",
        f"#define NDS_NATIVE_ITEM_BOMBHEI_IMAGE_COUNT {len(images)}u",
        f"#define NDS_NATIVE_ITEM_BOMBHEI_PACKET_ROM_BYTES {packet_bytes}u",
        f"#define NDS_NATIVE_ITEM_BOMBHEI_PACKET_RAM_BYTES {packet_bytes}u",
    ))
    lines = [
        "/* Bob-omb / BombHei packet, generated from file 86 root 0x3310.",
        " * Word 17 is exactly 0xDE000000 0x0E000000. MObj flags 0x0001",
        " * make CURRENT_IMAGE the only live material input. */",
        "#include <nds/generated/nds_native_item_bombhei.generated.h>", "",
        _arrays("BombHei", verts, tris),
        "static const u32 sNdsNativeItemBombHeiImageOffsets[NDS_NATIVE_ITEM_BOMBHEI_IMAGE_COUNT] =", "{",
        *(f"    0x{x:04x}u," for x in images), "};", "",
        "static void ndsNativeItemBombHeiSetup(NDSRendererStats *stats, const void *tlut)", "{",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[1][0]:08x}u) | 0x{raw[1][1]:08x}u;",
        _othermode(raw, 2), _othermode(raw, 3), _othermode(raw, 4),
        f"    ndsRendererRecordSetCombine(stats, 0x{raw[5][0]:08x}u, 0x{raw[5][1]:08x}u);",
        f"    stats->blend_color = 0x{raw[6][1]:08x}u;",
        *(f"    ndsRendererRecordSetTile(stats, 0x{raw[i][0]:08x}u, 0x{raw[i][1]:08x}u);" for i in (8, 9, 10)),
        f"    ndsRendererRecordSetImage(stats, 0x{raw[11][0]:08x}u, (u32)(uintptr_t)tlut);",
        f"    ndsRendererRecordLoadTlut(stats, 0x{raw[13][1]:08x}u);",
        f"    ndsRendererRecordTextureState(stats, 0x{raw[15][0]:08x}u, 0x{raw[15][1]:08x}u);",
        f"    ndsRendererRecordSetTileSize(stats, 0x{raw[16][0]:08x}u, 0x{raw[16][1]:08x}u);",
        "}", "",
        "static void ndsNativeItemBombHeiAfterMaterial(NDSRendererStats *stats)", "{",
        f"    ndsRendererRecordLoadBlock(stats, 0x{raw[19][0]:08x}u, 0x{raw[19][1]:08x}u);",
        "}", "",
        "static void ndsNativeItemBombHeiFinish(NDSRendererStats *stats)", "{",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[24][0]:08x}u) | 0x{raw[24][1]:08x}u;",
        _othermode(raw, 25), _othermode(raw, 26), _othermode(raw, 27), "}", "",
    ]
    check = "ITEM_BOMBHEI_NATIVE_OK root=0x3310 verts=4 tris=2 material=CURRENT_IMAGE hook=word17"
    return "\n".join(lines), header, check


def _rshell(model, attr):
    root = 0x5EC0
    raw = _words(model, root, 25)
    _expect_ops(raw, GSHELL_OPS, "Red Shell")
    _expect_de(raw, ((12, 0xDE000000, 0x0E000000),), "Red Shell")
    _expect_ptr(attr, 0x584, 0x5F88, "Red Shell ITAttributes.data")
    _expect_ptr(attr, 0x588, 0x5DE0, "Red Shell ITAttributes.p_mobjsubs")
    _expect_null(attr, 0x58C, "Red Shell ITAttributes.anim_joints")
    _expect_null(attr, 0x590, "Red Shell ITAttributes.p_matanim_joints")
    _expect_null(model, 0x5DE0, "Red Shell MObj head 0")
    _expect_ptr(model, 0x5DE4, 0x5E78, "Red Shell MObj head 1")
    _expect_ptr(model, 0x5E78, 0x5E00, "Red Shell MObjSub")
    flags = struct.unpack_from(">H", model.payload, 0x5E00 + 0x30)[0]
    if flags != 0x0005:
        raise RuntimeError(f"Red Shell MObj flags changed: {flags:#x}")
    _expect_ptr(model, 0x5FB8, root, "Red Shell DObj root")
    _expect_ptr(model, root + 17 * 8 + 4, 0x5E80, "Red Shell vertices")
    # The Red and Green shells point at the exact same DD/root/material tables.
    # Reuse the already-linked Green Shell packet instead of spending RAM twice.
    images = _ptr_offsets(model, 0x5DE8, 4, "Red Shell images")
    palettes = _ptr_offsets(model, 0x5DF8, 2, "Red Shell palettes")
    if len(images) != 4 or len(palettes) != 2:
        raise RuntimeError("Red Shell material tables changed")
    header = _header("rshell", "RSHELL", (
        f"#define NDS_NATIVE_ITEM_RSHELL_ASSET {ASSET}u",
        f"#define NDS_NATIVE_ITEM_RSHELL_ROOT 0x{root:04x}u",
        "#define NDS_NATIVE_ITEM_RSHELL_FILE_END 0x5f88u",
        "#define NDS_NATIVE_ITEM_RSHELL_HOOK_W0 0xde000000u",
        "#define NDS_NATIVE_ITEM_RSHELL_HOOK_W1 0x0e000000u",
        "#define NDS_NATIVE_ITEM_RSHELL_VERTEX_OFFSET 0x5e80u",
        "#define NDS_NATIVE_ITEM_RSHELL_VERTEX_COUNT 4u",
        "#define NDS_NATIVE_ITEM_RSHELL_TRIANGLE_COUNT 2u",
        "#define NDS_NATIVE_ITEM_RSHELL_CORNER_COUNT 6u",
        "#define NDS_NATIVE_ITEM_RSHELL_IMAGE_COUNT 4u",
        "#define NDS_NATIVE_ITEM_RSHELL_PALETTE_COUNT 2u",
        "#define NDS_NATIVE_ITEM_RSHELL_PACKET_ROM_BYTES 0u",
        "#define NDS_NATIVE_ITEM_RSHELL_PACKET_RAM_BYTES 0u",
    ))
    packet = "\n".join((
        "/* Red Shell packet aliases the byte-identical Green Shell source packet.",
        " * File 86 DD 0x5f88/root 0x5ec0 and all MObj tables are shared. */",
        "#include <nds/generated/nds_native_item_rshell.generated.h>",
        "#define sNdsNativeItemRShellTriIndices sNdsNativeItemGShellTriIndices",
        "#define sNdsNativeItemRShellVerts sNdsNativeItemGShellVerts",
        "#define sNdsNativeItemRShellVertColors sNdsNativeItemGShellVertColors",
        "#define sNdsNativeItemRShellImageOffsets sNdsNativeItemGShellImageOffsets",
        "#define sNdsNativeItemRShellPaletteOffsets sNdsNativeItemGShellPaletteOffsets",
        "#define ndsNativeItemRShellSetup ndsNativeItemGShellSetup",
        "#define ndsNativeItemRShellAfterMaterial ndsNativeItemGShellAfterMaterial",
        "#define ndsNativeItemRShellFinish ndsNativeItemGShellFinish",
        "",
    ))
    check = "ITEM_RSHELL_NATIVE_OK root=0x5ec0 verts=4 tris=2 material=PALETTE_IMAGE|PALETTE_TLUT|CURRENT_IMAGE hook=word12 shared=gshell"
    return packet, header, check


def _heart(model, attr):
    root = 0x0FF8
    raw = _words(model, root, 42)
    _expect_ops(raw, (
        0xE7, 0xD9, 0xE3, 0xE2, 0xFC, 0xFA, 0xF9, 0xE8,
        0xF5, 0xF5, 0xF5, 0xFD, 0xE6, 0xF0, 0xE7, 0xD7,
        0xF2, 0xFD, 0xE6, 0xF3, 0xE7, 0x01, 0x06, 0xE7,
        0xE3, 0xE2, 0xD9, 0xD9, 0xFC, 0xFA, 0xF5, 0xF5,
        0xFD, 0xE6, 0xF3, 0xE7, 0x01, 0x06, 0xE7, 0xE7,
        0xD9, 0xDF,
    ), "Heart")
    _expect_de(raw, (), "Heart")
    _expect_ptr(attr, 0x100, 0x1158, "Heart ITAttributes.data")
    for slot, label in ((0x104, "p_mobjsubs"), (0x108, "anim_joints"),
                        (0x10C, "p_matanim_joints")):
        _expect_null(attr, slot, f"Heart ITAttributes.{label}")
    if struct.unpack_from(">i", model.payload, 0x1158)[0] != 0:
        raise RuntimeError("Heart DObjDesc root changed")
    if struct.unpack_from(">i", model.payload, 0x1184)[0] != 1:
        raise RuntimeError("Heart DObjDesc child changed")
    _expect_ptr(model, 0x1188, 0x1148, "Heart DObjDLLink table")
    if struct.unpack_from(">I", model.payload, 0x1148)[0] != 1:
        raise RuntimeError("Heart DObjDLLink selector changed")
    _expect_ptr(model, 0x114C, root, "Heart DObjDLLink root")
    if ((struct.unpack_from(">I", model.payload, 0x1150)[0] != 4) or
            (model.pointer_at(0x1154) is not None)):
        raise RuntimeError("Heart DObjDLLink terminator changed")
    if struct.unpack_from(">i", model.payload, 0x11B0)[0] != 18:
        raise RuntimeError("Heart DObjDesc terminator changed")
    for slot, off, label in (
        (root + 11 * 8 + 4, 0x0B48, "Heart TLUT"),
        (root + 17 * 8 + 4, 0x0D78, "Heart image 0"),
        (root + 21 * 8 + 4, 0x0F78, "Heart vertices 0"),
        (root + 32 * 8 + 4, 0x0B70, "Heart image 1"),
        (root + 36 * 8 + 4, 0x0FB8, "Heart vertices 1"),
    ):
        _expect_ptr(model, slot, off, label)
    verts0 = _decode_verts(model, 0x0F78, 4)
    verts1 = _decode_verts(model, 0x0FB8, 4)
    tris0 = _decode_tris(raw, ((22, 0x06),))
    tris1 = _decode_tris(raw, ((37, 0x06),))
    packet_bytes = (
        len(verts0) * 14 + len(tris0) * 6 +
        len(verts1) * 14 + len(tris1) * 6
    )
    header = _header("heart", "HEART", (
        f"#define NDS_NATIVE_ITEM_HEART_ASSET {ASSET}u",
        f"#define NDS_NATIVE_ITEM_HEART_ROOT 0x{root:04x}u",
        "#define NDS_NATIVE_ITEM_HEART_FILE_END 0x11dcu",
        "#define NDS_NATIVE_ITEM_HEART_TLUT_OFFSET 0x0b48u",
        f"#define NDS_NATIVE_ITEM_HEART_TLUT_W0 0x{raw[11][0]:08x}u",
        "#define NDS_NATIVE_ITEM_HEART_IMAGE0_OFFSET 0x0d78u",
        f"#define NDS_NATIVE_ITEM_HEART_IMAGE0_W0 0x{raw[17][0]:08x}u",
        "#define NDS_NATIVE_ITEM_HEART_IMAGE1_OFFSET 0x0b70u",
        f"#define NDS_NATIVE_ITEM_HEART_IMAGE1_W0 0x{raw[32][0]:08x}u",
        "#define NDS_NATIVE_ITEM_HEART_VERTEX0_OFFSET 0x0f78u",
        "#define NDS_NATIVE_ITEM_HEART_VERTEX1_OFFSET 0x0fb8u",
        f"#define NDS_NATIVE_ITEM_HEART_VERTEX0_COUNT {len(verts0)}u",
        f"#define NDS_NATIVE_ITEM_HEART_TRIANGLE0_COUNT {len(tris0)}u",
        f"#define NDS_NATIVE_ITEM_HEART_CORNER0_COUNT {len(tris0) * 3}u",
        f"#define NDS_NATIVE_ITEM_HEART_VERTEX1_COUNT {len(verts1)}u",
        f"#define NDS_NATIVE_ITEM_HEART_TRIANGLE1_COUNT {len(tris1)}u",
        f"#define NDS_NATIVE_ITEM_HEART_CORNER1_COUNT {len(tris1) * 3}u",
        f"#define NDS_NATIVE_ITEM_HEART_PACKET_ROM_BYTES {packet_bytes}u",
        f"#define NDS_NATIVE_ITEM_HEART_PACKET_RAM_BYTES {packet_bytes}u",
    ))
    lines = [
        "/* Heart Container packet, generated from file 86 root 0x0ff8.",
        " * No 0xDE appears; both fixed material/geometry phases bake. */",
        "#include <nds/generated/nds_native_item_heart.generated.h>", "",
        _arrays("Heart0", verts0, tris0),
        _arrays("Heart1", verts1, tris1),
        "static void ndsNativeItemHeartSetup0(",
        "    NDSRendererStats *stats, const void *tlut, const void *image)", "{",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[1][0]:08x}u) | 0x{raw[1][1]:08x}u;",
        _othermode(raw, 2), _othermode(raw, 3),
        f"    ndsRendererRecordSetCombine(stats, 0x{raw[4][0]:08x}u, 0x{raw[4][1]:08x}u);",
        f"    stats->prim_color = 0x{raw[5][1]:08x}u;",
        f"    stats->blend_color = 0x{raw[6][1]:08x}u;",
        *(f"    ndsRendererRecordSetTile(stats, 0x{raw[i][0]:08x}u, 0x{raw[i][1]:08x}u);" for i in (8, 9, 10)),
        f"    ndsRendererRecordSetImage(stats, 0x{raw[11][0]:08x}u, (u32)(uintptr_t)tlut);",
        f"    ndsRendererRecordLoadTlut(stats, 0x{raw[13][1]:08x}u);",
        f"    ndsRendererRecordTextureState(stats, 0x{raw[15][0]:08x}u, 0x{raw[15][1]:08x}u);",
        f"    ndsRendererRecordSetTileSize(stats, 0x{raw[16][0]:08x}u, 0x{raw[16][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{raw[17][0]:08x}u, (u32)(uintptr_t)image);",
        f"    ndsRendererRecordLoadBlock(stats, 0x{raw[19][0]:08x}u, 0x{raw[19][1]:08x}u);",
        "}", "",
        "static void ndsNativeItemHeartSetup1(NDSRendererStats *stats, const void *image)", "{",
        _othermode(raw, 24), _othermode(raw, 25),
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[26][0]:08x}u) | 0x{raw[26][1]:08x}u;",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[27][0]:08x}u) | 0x{raw[27][1]:08x}u;",
        f"    ndsRendererRecordSetCombine(stats, 0x{raw[28][0]:08x}u, 0x{raw[28][1]:08x}u);",
        f"    stats->prim_color = 0x{raw[29][1]:08x}u;",
        f"    ndsRendererRecordSetTile(stats, 0x{raw[30][0]:08x}u, 0x{raw[30][1]:08x}u);",
        f"    ndsRendererRecordSetTile(stats, 0x{raw[31][0]:08x}u, 0x{raw[31][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{raw[32][0]:08x}u, (u32)(uintptr_t)image);",
        f"    ndsRendererRecordLoadBlock(stats, 0x{raw[34][0]:08x}u, 0x{raw[34][1]:08x}u);",
        "}", "",
        "static void ndsNativeItemHeartFinish(NDSRendererStats *stats)", "{",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[40][0]:08x}u) | 0x{raw[40][1]:08x}u;",
        "}", "",
    ]
    check = "ITEM_HEART_NATIVE_OK root=0x0ff8 verts=4+4 tris=2+2 material=none de=none"
    return "\n".join(lines), header, check


def _starrod(model, attr):
    header_root, callee_root, second_root = 0x49B0, 0x4A18, 0x4AA0
    header_raw = _words(model, header_root, 13)
    callee_raw = _words(model, callee_root, 17)
    second_raw = _words(model, second_root, 24)
    _expect_ops(header_raw, (
        0xE7, 0xDB, 0xDB, 0xDB, 0xDB, 0xE3, 0xFC, 0xE8,
        0xF5, 0xD7, 0xF9, 0xDE, 0xDF,
    ), "Star Rod header")
    _expect_ops(callee_raw, (
        0xE7, 0xF5, 0xF5, 0xFD, 0xE6, 0xF0, 0xE7, 0xF2,
        0xFD, 0xE6, 0xF3, 0xE7, 0x01, 0x06, 0x06, 0x06, 0xDF,
    ), "Star Rod callee")
    _expect_ops(second_raw, (
        0xE7, 0xE2, 0xE2, 0xF5, 0xF5, 0xFD, 0xE6, 0xF0,
        0xE7, 0xF2, 0xFD, 0xE6, 0xF3, 0xE7, 0xD9, 0x01,
        0x06, 0xE7, 0xE7, 0xD9, 0xE3, 0xE2, 0xE2, 0xDF,
    ), "Star Rod second root")
    _expect_de(header_raw, ((11, 0xDE000000, 0x128D1286),), "Star Rod header")
    _expect_de(callee_raw, (), "Star Rod callee")
    _expect_de(second_raw, (), "Star Rod second root")
    _expect_ptr(attr, 0x48C, 0x4B60, "Star Rod ITAttributes.data")
    for slot, label in ((0x490, "p_mobjsubs"), (0x494, "anim_joints"),
                        (0x498, "p_matanim_joints")):
        _expect_null(attr, slot, f"Star Rod ITAttributes.{label}")
    if struct.unpack_from(">i", model.payload, 0x4B60)[0] != 0:
        raise RuntimeError("Star Rod DObjDesc root changed")
    if struct.unpack_from(">i", model.payload, 0x4B8C)[0] != 1:
        raise RuntimeError("Star Rod DObjDesc child 1 changed")
    _expect_ptr(model, 0x4B90, header_root, "Star Rod first root")
    if struct.unpack_from(">i", model.payload, 0x4BB8)[0] != 0x4002:
        raise RuntimeError("Star Rod DObjDesc child 2 changed")
    _expect_ptr(model, 0x4BBC, second_root, "Star Rod second root")
    if struct.unpack_from(">i", model.payload, 0x4BE4)[0] != 18:
        raise RuntimeError("Star Rod DObjDesc terminator changed")
    _expect_ptr(model, header_root + 11 * 8 + 4, callee_root,
                "Star Rod branch callee")
    for slot, off, label in (
        (callee_root + 3 * 8 + 4, 0x4798, "Star Rod TLUT 0"),
        (callee_root + 8 * 8 + 4, 0x47E8, "Star Rod image 0"),
        (callee_root + 12 * 8 + 4, 0x48F0, "Star Rod vertices 0"),
        (second_root + 5 * 8 + 4, 0x47C0, "Star Rod TLUT 1"),
        (second_root + 10 * 8 + 4, 0x4870, "Star Rod image 1"),
        (second_root + 15 * 8 + 4, 0x4970, "Star Rod vertices 1"),
    ):
        _expect_ptr(model, slot, off, label)
    verts0 = _decode_verts(model, 0x48F0, 8)
    verts1 = _decode_verts(model, 0x4970, 4)
    tris0 = _decode_tris(callee_raw, ((13, 0x06), (14, 0x06), (15, 0x06)))
    tris1 = _decode_tris(second_raw, ((16, 0x06),))
    packet_bytes = (
        len(verts0) * 14 + len(tris0) * 6 +
        len(verts1) * 14 + len(tris1) * 6
    )
    header = _header("starrod", "STARROD", (
        f"#define NDS_NATIVE_ITEM_STARROD_ASSET {ASSET}u",
        f"#define NDS_NATIVE_ITEM_STARROD_HEADER_ROOT 0x{header_root:04x}u",
        f"#define NDS_NATIVE_ITEM_STARROD_CALLEE_ROOT 0x{callee_root:04x}u",
        f"#define NDS_NATIVE_ITEM_STARROD_SECOND_ROOT 0x{second_root:04x}u",
        "#define NDS_NATIVE_ITEM_STARROD_FILE_END 0x4c10u",
        "#define NDS_NATIVE_ITEM_STARROD_BRANCH_W0 0xde000000u",
        "#define NDS_NATIVE_ITEM_STARROD_BRANCH_RAW_W1 0x128d1286u",
        "#define NDS_NATIVE_ITEM_STARROD_TLUT0_OFFSET 0x4798u",
        "#define NDS_NATIVE_ITEM_STARROD_IMAGE0_OFFSET 0x47e8u",
        "#define NDS_NATIVE_ITEM_STARROD_VERTEX0_OFFSET 0x48f0u",
        "#define NDS_NATIVE_ITEM_STARROD_TLUT1_OFFSET 0x47c0u",
        "#define NDS_NATIVE_ITEM_STARROD_IMAGE1_OFFSET 0x4870u",
        "#define NDS_NATIVE_ITEM_STARROD_VERTEX1_OFFSET 0x4970u",
        f"#define NDS_NATIVE_ITEM_STARROD_VERTEX0_COUNT {len(verts0)}u",
        f"#define NDS_NATIVE_ITEM_STARROD_TRIANGLE0_COUNT {len(tris0)}u",
        f"#define NDS_NATIVE_ITEM_STARROD_CORNER0_COUNT {len(tris0) * 3}u",
        f"#define NDS_NATIVE_ITEM_STARROD_VERTEX1_COUNT {len(verts1)}u",
        f"#define NDS_NATIVE_ITEM_STARROD_TRIANGLE1_COUNT {len(tris1)}u",
        f"#define NDS_NATIVE_ITEM_STARROD_CORNER1_COUNT {len(tris1) * 3}u",
        f"#define NDS_NATIVE_ITEM_STARROD_PACKET_ROM_BYTES {packet_bytes}u",
        f"#define NDS_NATIVE_ITEM_STARROD_PACKET_RAM_BYTES {packet_bytes}u",
    ))
    lines = [
        "/* Star Rod native packet, generated from file 86.",
        " * Root 0x49b0 word 11 is 0xDE000000 0x128D1286 and relocates to",
        " * bake-only callee 0x4a18. Root 0x4aa0 is bake-only. */",
        "#include <nds/generated/nds_native_item_starrod.generated.h>", "",
        _arrays("StarRod0", verts0, tris0),
        _arrays("StarRod1", verts1, tris1),
        "static void ndsNativeItemStarRodHeaderSetup(",
        "    NDSRendererStats *stats, NDSRendererTraversalState *state)", "{",
        *(f"    ndsRendererApplyMatrixMoveWordCommand(stats, state, 0x{header_raw[i][0]:08x}u, 0x{header_raw[i][1]:08x}u);" for i in (1, 2, 3, 4)),
        _othermode(header_raw, 5),
        f"    ndsRendererRecordSetCombine(stats, 0x{header_raw[6][0]:08x}u, 0x{header_raw[6][1]:08x}u);",
        f"    ndsRendererRecordSetTile(stats, 0x{header_raw[8][0]:08x}u, 0x{header_raw[8][1]:08x}u);",
        f"    ndsRendererRecordTextureState(stats, 0x{header_raw[9][0]:08x}u, 0x{header_raw[9][1]:08x}u);",
        f"    stats->blend_color = 0x{header_raw[10][1]:08x}u;",
        "}", "",
        "static void ndsNativeItemStarRodRun0Setup(",
        "    NDSRendererStats *stats, const void *tlut, const void *image)", "{",
        f"    ndsRendererRecordSetTile(stats, 0x{callee_raw[1][0]:08x}u, 0x{callee_raw[1][1]:08x}u);",
        f"    ndsRendererRecordSetTile(stats, 0x{callee_raw[2][0]:08x}u, 0x{callee_raw[2][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{callee_raw[3][0]:08x}u, (u32)(uintptr_t)tlut);",
        f"    ndsRendererRecordLoadTlut(stats, 0x{callee_raw[5][1]:08x}u);",
        f"    ndsRendererRecordSetTileSize(stats, 0x{callee_raw[7][0]:08x}u, 0x{callee_raw[7][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{callee_raw[8][0]:08x}u, (u32)(uintptr_t)image);",
        f"    ndsRendererRecordLoadBlock(stats, 0x{callee_raw[10][0]:08x}u, 0x{callee_raw[10][1]:08x}u);",
        "}", "",
        "static void ndsNativeItemStarRodRun1Setup(",
        "    NDSRendererStats *stats, const void *tlut, const void *image)", "{",
        _othermode(second_raw, 1), _othermode(second_raw, 2),
        f"    ndsRendererRecordSetTile(stats, 0x{second_raw[3][0]:08x}u, 0x{second_raw[3][1]:08x}u);",
        f"    ndsRendererRecordSetTile(stats, 0x{second_raw[4][0]:08x}u, 0x{second_raw[4][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{second_raw[5][0]:08x}u, (u32)(uintptr_t)tlut);",
        f"    ndsRendererRecordLoadTlut(stats, 0x{second_raw[7][1]:08x}u);",
        f"    ndsRendererRecordSetTileSize(stats, 0x{second_raw[9][0]:08x}u, 0x{second_raw[9][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{second_raw[10][0]:08x}u, (u32)(uintptr_t)image);",
        f"    ndsRendererRecordLoadBlock(stats, 0x{second_raw[12][0]:08x}u, 0x{second_raw[12][1]:08x}u);",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{second_raw[14][0]:08x}u) | 0x{second_raw[14][1]:08x}u;",
        "}", "",
        "static void ndsNativeItemStarRodRun1Finish(NDSRendererStats *stats)", "{",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{second_raw[19][0]:08x}u) | 0x{second_raw[19][1]:08x}u;",
        _othermode(second_raw, 20), _othermode(second_raw, 21), _othermode(second_raw, 22),
        "}", "",
    ]
    check = (
        "ITEM_STARROD_NATIVE_OK roots=0x49b0->0x4a18,0x4aa0 verts=8+4 "
        "tris=6+2 material=none branch=word11:0x128d1286"
    )
    return "\n".join(lines), header, check


def _fflower(model, attr):
    branch_root, callee_root, live_root = 0x4520, 0x4578, 0x4608
    branch_raw = _words(model, branch_root, 11)
    callee_raw = _words(model, callee_root, 18)
    live_raw = _words(model, live_root, 21)
    _expect_ops(branch_raw, (
        0xE7, 0xE3, 0xE2, 0xE2, 0xFC, 0xF9, 0xFD, 0xD7,
        0xD9, 0xDE, 0xDF,
    ), "Fire Flower branch root")
    _expect_ops(callee_raw, (
        0xE7, 0xE8, 0xF5, 0xF5, 0xF5, 0xE6, 0xF0, 0xE7,
        0xF2, 0xFD, 0xE6, 0xF3, 0xE7, 0x01, 0x06, 0x06,
        0x06, 0xDF,
    ), "Fire Flower branch callee")
    _expect_ops(live_raw, (
        0xE7, 0xF5, 0xF5, 0xDE, 0xE6, 0xF0, 0xE7, 0xF2,
        0xFD, 0xE6, 0xF3, 0xE7, 0x01, 0x06, 0xE7, 0xE7,
        0xD9, 0xE3, 0xE2, 0xE2, 0xDF,
    ), "Fire Flower live root")
    _expect_de(branch_raw, ((9, 0xDE000000, 0x1171115E),),
               "Fire Flower branch root")
    _expect_de(callee_raw, (), "Fire Flower branch callee")
    _expect_de(live_raw, ((3, 0xDE000000, 0x0E000000),),
               "Fire Flower live root")
    _expect_ptr(attr, 0x2E4, 0x46B0, "Fire Flower ITAttributes.data")
    _expect_ptr(attr, 0x2E8, 0x4388, "Fire Flower ITAttributes.p_mobjsubs")
    _expect_null(attr, 0x2EC, "Fire Flower ITAttributes.anim_joints")
    _expect_ptr(attr, 0x2F0, 0x4760, "Fire Flower ITAttributes.p_matanim_joints")
    _expect_null(model, 0x4388, "Fire Flower MObj head 0")
    _expect_null(model, 0x438C, "Fire Flower MObj head 1")
    _expect_ptr(model, 0x4390, 0x4418, "Fire Flower MObj head 2")
    _expect_ptr(model, 0x4418, 0x43A0, "Fire Flower MObjSub")
    flags = struct.unpack_from(">H", model.payload, 0x43A0 + 0x30)[0]
    if flags != 0x0004:
        raise RuntimeError(f"Fire Flower MObj flags changed: {flags:#x}")
    if model.pointer_at(0x43A0 + 4) is not None:
        raise RuntimeError("Fire Flower unexpectedly gained a sprite stream")
    palettes = _ptr_offsets(model, 0x4394, 2, "Fire Flower live palettes")
    if struct.unpack_from(">i", model.payload, 0x46B0)[0] != 0:
        raise RuntimeError("Fire Flower DObjDesc root changed")
    if struct.unpack_from(">i", model.payload, 0x46DC)[0] != 1:
        raise RuntimeError("Fire Flower DObjDesc child 1 changed")
    _expect_ptr(model, 0x46E0, branch_root, "Fire Flower branch root")
    if struct.unpack_from(">i", model.payload, 0x4708)[0] != 0x4002:
        raise RuntimeError("Fire Flower DObjDesc child 2 changed")
    _expect_ptr(model, 0x470C, live_root, "Fire Flower live root")
    if struct.unpack_from(">i", model.payload, 0x4734)[0] != 18:
        raise RuntimeError("Fire Flower DObjDesc terminator changed")
    _expect_ptr(model, branch_root + 9 * 8 + 4, callee_root,
                "Fire Flower branch callee")
    for slot, off, label in (
        (branch_root + 6 * 8 + 4, 0x4168, "Fire Flower branch TLUT"),
        (callee_root + 9 * 8 + 4, 0x4200, "Fire Flower branch image"),
        (callee_root + 13 * 8 + 4, 0x4420, "Fire Flower branch vertices"),
        (live_root + 8 * 8 + 4, 0x4308, "Fire Flower live image"),
        (live_root + 12 * 8 + 4, 0x44E0, "Fire Flower live vertices"),
    ):
        _expect_ptr(model, slot, off, label)
    verts0 = _decode_verts(model, 0x4420, 12)
    verts1 = _decode_verts(model, 0x44E0, 4)
    tris0 = _decode_tris(callee_raw, ((14, 0x06), (15, 0x06), (16, 0x06)))
    tris1 = _decode_tris(live_raw, ((13, 0x06),))
    packet_bytes = (
        len(verts0) * 14 + len(tris0) * 6 +
        len(verts1) * 14 + len(tris1) * 6 + len(palettes) * 4
    )
    header = _header("fflower", "FFLOWER", (
        f"#define NDS_NATIVE_ITEM_FFLOWER_ASSET {ASSET}u",
        f"#define NDS_NATIVE_ITEM_FFLOWER_BRANCH_ROOT 0x{branch_root:04x}u",
        f"#define NDS_NATIVE_ITEM_FFLOWER_CALLEE_ROOT 0x{callee_root:04x}u",
        f"#define NDS_NATIVE_ITEM_FFLOWER_LIVE_ROOT 0x{live_root:04x}u",
        "#define NDS_NATIVE_ITEM_FFLOWER_FILE_END 0x4760u",
        "#define NDS_NATIVE_ITEM_FFLOWER_BRANCH_W0 0xde000000u",
        "#define NDS_NATIVE_ITEM_FFLOWER_BRANCH_RAW_W1 0x1171115eu",
        "#define NDS_NATIVE_ITEM_FFLOWER_HOOK_W0 0xde000000u",
        "#define NDS_NATIVE_ITEM_FFLOWER_HOOK_W1 0x0e000000u",
        "#define NDS_NATIVE_ITEM_FFLOWER_MOBJ_FLAGS 0x0004u",
        "#define NDS_NATIVE_ITEM_FFLOWER_BRANCH_TLUT_OFFSET 0x4168u",
        "#define NDS_NATIVE_ITEM_FFLOWER_BRANCH_IMAGE_OFFSET 0x4200u",
        "#define NDS_NATIVE_ITEM_FFLOWER_BRANCH_VERTEX_OFFSET 0x4420u",
        "#define NDS_NATIVE_ITEM_FFLOWER_LIVE_IMAGE_OFFSET 0x4308u",
        "#define NDS_NATIVE_ITEM_FFLOWER_LIVE_VERTEX_OFFSET 0x44e0u",
        f"#define NDS_NATIVE_ITEM_FFLOWER_VERTEX0_COUNT {len(verts0)}u",
        f"#define NDS_NATIVE_ITEM_FFLOWER_TRIANGLE0_COUNT {len(tris0)}u",
        f"#define NDS_NATIVE_ITEM_FFLOWER_CORNER0_COUNT {len(tris0) * 3}u",
        f"#define NDS_NATIVE_ITEM_FFLOWER_VERTEX1_COUNT {len(verts1)}u",
        f"#define NDS_NATIVE_ITEM_FFLOWER_TRIANGLE1_COUNT {len(tris1)}u",
        f"#define NDS_NATIVE_ITEM_FFLOWER_CORNER1_COUNT {len(tris1) * 3}u",
        f"#define NDS_NATIVE_ITEM_FFLOWER_PALETTE_COUNT {len(palettes)}u",
        f"#define NDS_NATIVE_ITEM_FFLOWER_PACKET_ROM_BYTES {packet_bytes}u",
        f"#define NDS_NATIVE_ITEM_FFLOWER_PACKET_RAM_BYTES {packet_bytes}u",
    ))
    lines = [
        "/* Fire Flower native packet, generated from file 86.",
        " * Root 0x4520 word 9 is 0xDE000000 0x1171115E and relocates to",
        " * bake-only 0x4578. Root 0x4608 word 3 is the live 0x0E000000",
        " * palette hook; its MObj flags are exactly 0x0004. */",
        "#include <nds/generated/nds_native_item_fflower.generated.h>", "",
        _arrays("FFlower0", verts0, tris0),
        _arrays("FFlower1", verts1, tris1),
        "static const u32 sNdsNativeItemFFlowerPaletteOffsets[NDS_NATIVE_ITEM_FFLOWER_PALETTE_COUNT] =", "{",
        *(f"    0x{x:04x}u," for x in palettes), "};", "",
        "static void ndsNativeItemFFlowerBranchSetup(",
        "    NDSRendererStats *stats, NDSRendererTraversalState *state,",
        "    const void *tlut, const void *image)", "{",
        _othermode(branch_raw, 1), _othermode(branch_raw, 2), _othermode(branch_raw, 3),
        f"    ndsRendererRecordSetCombine(stats, 0x{branch_raw[4][0]:08x}u, 0x{branch_raw[4][1]:08x}u);",
        f"    stats->blend_color = 0x{branch_raw[5][1]:08x}u;",
        f"    ndsRendererRecordSetImage(stats, 0x{branch_raw[6][0]:08x}u, (u32)(uintptr_t)tlut);",
        f"    ndsRendererRecordTextureState(stats, 0x{branch_raw[7][0]:08x}u, 0x{branch_raw[7][1]:08x}u);",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{branch_raw[8][0]:08x}u) | 0x{branch_raw[8][1]:08x}u;",
        *(f"    ndsRendererRecordSetTile(stats, 0x{callee_raw[i][0]:08x}u, 0x{callee_raw[i][1]:08x}u);" for i in (2, 3, 4)),
        f"    ndsRendererRecordLoadTlut(stats, 0x{callee_raw[6][1]:08x}u);",
        f"    ndsRendererRecordSetTileSize(stats, 0x{callee_raw[8][0]:08x}u, 0x{callee_raw[8][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{callee_raw[9][0]:08x}u, (u32)(uintptr_t)image);",
        f"    ndsRendererRecordLoadBlock(stats, 0x{callee_raw[11][0]:08x}u, 0x{callee_raw[11][1]:08x}u);",
        "    (void)state;",
        "}", "",
        "static void ndsNativeItemFFlowerLiveBeforeMaterial(NDSRendererStats *stats)", "{",
        f"    ndsRendererRecordSetTile(stats, 0x{live_raw[1][0]:08x}u, 0x{live_raw[1][1]:08x}u);",
        f"    ndsRendererRecordSetTile(stats, 0x{live_raw[2][0]:08x}u, 0x{live_raw[2][1]:08x}u);",
        "}", "",
        "static void ndsNativeItemFFlowerLiveAfterMaterial(NDSRendererStats *stats, const void *image)", "{",
        f"    ndsRendererRecordLoadTlut(stats, 0x{live_raw[5][1]:08x}u);",
        f"    ndsRendererRecordSetTileSize(stats, 0x{live_raw[7][0]:08x}u, 0x{live_raw[7][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{live_raw[8][0]:08x}u, (u32)(uintptr_t)image);",
        f"    ndsRendererRecordLoadBlock(stats, 0x{live_raw[10][0]:08x}u, 0x{live_raw[10][1]:08x}u);",
        "}", "",
        "static void ndsNativeItemFFlowerLiveFinish(NDSRendererStats *stats)", "{",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{live_raw[16][0]:08x}u) | 0x{live_raw[16][1]:08x}u;",
        _othermode(live_raw, 17), _othermode(live_raw, 18), _othermode(live_raw, 19),
        "}", "",
    ]
    check = (
        "ITEM_FFLOWER_NATIVE_OK roots=0x4520->0x4578,0x4608 verts=12+4 tris=6+2 "
        "material=none,PALETTE_IMAGE branch=word9:0x1171115e hook=word3:0x0e000000"
    )
    return "\n".join(lines), header, check


def _msbomb(model, attr):
    root0, root1 = 0x37A0, 0x38B0
    raw0, raw1 = _words(model, root0, 34), _words(model, root1, 30)
    _expect_ops(raw0, (
        0xE7, 0xDB, 0xDB, 0xDB, 0xDB, 0xE3, 0xE2, 0xE2,
        0xFC, 0xF9, 0xE8, 0xF5, 0xF5, 0xF5, 0xFD, 0xE6,
        0xF0, 0xE7, 0xD7, 0xF2, 0xFD, 0xE6, 0xF3, 0xE7,
        0xD9, 0x01, 0x06, 0xE7, 0xE7, 0xD9, 0xE3, 0xE2,
        0xE2, 0xDF,
    ), "MS Bomb root 0")
    _expect_ops(raw1, (
        0xE7, 0xE3, 0xE2, 0xE2, 0xFC, 0xF9, 0xE8, 0xF5,
        0xF5, 0xF5, 0xFD, 0xE6, 0xF0, 0xE7, 0xD7, 0xF2,
        0xFD, 0xE6, 0xF3, 0xE7, 0xD9, 0x01, 0x06, 0xE7,
        0xE7, 0xD9, 0xE3, 0xE2, 0xE2, 0xDF,
    ), "MS Bomb root 1")
    _expect_de(raw0, (), "MS Bomb root 0")
    _expect_de(raw1, (), "MS Bomb root 1")
    _expect_ptr(attr, 0x3BC, 0x39A0, "MS Bomb ITAttributes.data")
    for slot, label in ((0x3C0, "p_mobjsubs"), (0x3C4, "anim_joints"),
                        (0x3C8, "p_matanim_joints")):
        _expect_null(attr, slot, f"MS Bomb ITAttributes.{label}")
    if struct.unpack_from(">i", model.payload, 0x39A0)[0] != 0:
        raise RuntimeError("MS Bomb DObjDesc root changed")
    if struct.unpack_from(">i", model.payload, 0x39CC)[0] != 1:
        raise RuntimeError("MS Bomb DObjDesc child 1 changed")
    if struct.unpack_from(">i", model.payload, 0x39F8)[0] != 2:
        raise RuntimeError("MS Bomb DObjDesc child 2 changed")
    _expect_ptr(model, 0x39FC, root0, "MS Bomb root 0")
    if struct.unpack_from(">i", model.payload, 0x3A24)[0] != 2:
        raise RuntimeError("MS Bomb DObjDesc child 3 changed")
    _expect_ptr(model, 0x3A28, root1, "MS Bomb root 1")
    if struct.unpack_from(">i", model.payload, 0x3A50)[0] != 18:
        raise RuntimeError("MS Bomb DObjDesc terminator changed")
    for slot, off, label in (
        (root0 + 14 * 8 + 4, 0x3630, "MS Bomb TLUT 0"),
        (root0 + 20 * 8 + 4, 0x36E0, "MS Bomb image 0"),
        (root0 + 25 * 8 + 4, 0x3720, "MS Bomb vertices 0"),
        (root1 + 10 * 8 + 4, 0x3608, "MS Bomb TLUT 1"),
        (root1 + 16 * 8 + 4, 0x3658, "MS Bomb image 1"),
        (root1 + 21 * 8 + 4, 0x3760, "MS Bomb vertices 1"),
    ):
        _expect_ptr(model, slot, off, label)
    verts0, verts1 = _decode_verts(model, 0x3720, 4), _decode_verts(model, 0x3760, 4)
    tris0, tris1 = _decode_tris(raw0, ((26, 0x06),)), _decode_tris(raw1, ((22, 0x06),))
    packet_bytes = (
        len(verts0) * 14 + len(tris0) * 6 +
        len(verts1) * 14 + len(tris1) * 6
    )
    header = _header("msbomb", "MSBOMB", (
        f"#define NDS_NATIVE_ITEM_MSBOMB_ASSET {ASSET}u",
        f"#define NDS_NATIVE_ITEM_MSBOMB_ROOT0 0x{root0:04x}u",
        f"#define NDS_NATIVE_ITEM_MSBOMB_ROOT1 0x{root1:04x}u",
        "#define NDS_NATIVE_ITEM_MSBOMB_FILE_END 0x3a7cu",
        "#define NDS_NATIVE_ITEM_MSBOMB_TLUT0_OFFSET 0x3630u",
        "#define NDS_NATIVE_ITEM_MSBOMB_IMAGE0_OFFSET 0x36e0u",
        "#define NDS_NATIVE_ITEM_MSBOMB_VERTEX0_OFFSET 0x3720u",
        "#define NDS_NATIVE_ITEM_MSBOMB_TLUT1_OFFSET 0x3608u",
        "#define NDS_NATIVE_ITEM_MSBOMB_IMAGE1_OFFSET 0x3658u",
        "#define NDS_NATIVE_ITEM_MSBOMB_VERTEX1_OFFSET 0x3760u",
        f"#define NDS_NATIVE_ITEM_MSBOMB_VERTEX0_COUNT {len(verts0)}u",
        f"#define NDS_NATIVE_ITEM_MSBOMB_TRIANGLE0_COUNT {len(tris0)}u",
        f"#define NDS_NATIVE_ITEM_MSBOMB_CORNER0_COUNT {len(tris0) * 3}u",
        f"#define NDS_NATIVE_ITEM_MSBOMB_VERTEX1_COUNT {len(verts1)}u",
        f"#define NDS_NATIVE_ITEM_MSBOMB_TRIANGLE1_COUNT {len(tris1)}u",
        f"#define NDS_NATIVE_ITEM_MSBOMB_CORNER1_COUNT {len(tris1) * 3}u",
        f"#define NDS_NATIVE_ITEM_MSBOMB_PACKET_ROM_BYTES {packet_bytes}u",
        f"#define NDS_NATIVE_ITEM_MSBOMB_PACKET_RAM_BYTES {packet_bytes}u",
    ))
    lines = [
        "/* Motion-Sensor Bomb packet, generated from file 86 roots 0x37a0/0x38b0.",
        " * Neither source root contains 0xDE; both fixed programs bake. */",
        "#include <nds/generated/nds_native_item_msbomb.generated.h>", "",
        _arrays("MSBomb0", verts0, tris0),
        _arrays("MSBomb1", verts1, tris1),
        "static void ndsNativeItemMSBombSetup0(",
        "    NDSRendererStats *stats, NDSRendererTraversalState *state,",
        "    const void *tlut, const void *image)", "{",
        *(f"    ndsRendererApplyMatrixMoveWordCommand(stats, state, 0x{raw0[i][0]:08x}u, 0x{raw0[i][1]:08x}u);" for i in (1, 2, 3, 4)),
        _othermode(raw0, 5), _othermode(raw0, 6), _othermode(raw0, 7),
        f"    ndsRendererRecordSetCombine(stats, 0x{raw0[8][0]:08x}u, 0x{raw0[8][1]:08x}u);",
        f"    stats->blend_color = 0x{raw0[9][1]:08x}u;",
        *(f"    ndsRendererRecordSetTile(stats, 0x{raw0[i][0]:08x}u, 0x{raw0[i][1]:08x}u);" for i in (11, 12, 13)),
        f"    ndsRendererRecordSetImage(stats, 0x{raw0[14][0]:08x}u, (u32)(uintptr_t)tlut);",
        f"    ndsRendererRecordLoadTlut(stats, 0x{raw0[16][1]:08x}u);",
        f"    ndsRendererRecordTextureState(stats, 0x{raw0[18][0]:08x}u, 0x{raw0[18][1]:08x}u);",
        f"    ndsRendererRecordSetTileSize(stats, 0x{raw0[19][0]:08x}u, 0x{raw0[19][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{raw0[20][0]:08x}u, (u32)(uintptr_t)image);",
        f"    ndsRendererRecordLoadBlock(stats, 0x{raw0[22][0]:08x}u, 0x{raw0[22][1]:08x}u);",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw0[24][0]:08x}u) | 0x{raw0[24][1]:08x}u;",
        "}", "",
        "static void ndsNativeItemMSBombFinish0(NDSRendererStats *stats)", "{",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw0[29][0]:08x}u) | 0x{raw0[29][1]:08x}u;",
        _othermode(raw0, 30), _othermode(raw0, 31), _othermode(raw0, 32),
        "}", "",
        "static void ndsNativeItemMSBombSetup1(",
        "    NDSRendererStats *stats, const void *tlut, const void *image)", "{",
        _othermode(raw1, 1), _othermode(raw1, 2), _othermode(raw1, 3),
        f"    ndsRendererRecordSetCombine(stats, 0x{raw1[4][0]:08x}u, 0x{raw1[4][1]:08x}u);",
        f"    stats->blend_color = 0x{raw1[5][1]:08x}u;",
        *(f"    ndsRendererRecordSetTile(stats, 0x{raw1[i][0]:08x}u, 0x{raw1[i][1]:08x}u);" for i in (7, 8, 9)),
        f"    ndsRendererRecordSetImage(stats, 0x{raw1[10][0]:08x}u, (u32)(uintptr_t)tlut);",
        f"    ndsRendererRecordLoadTlut(stats, 0x{raw1[12][1]:08x}u);",
        f"    ndsRendererRecordTextureState(stats, 0x{raw1[14][0]:08x}u, 0x{raw1[14][1]:08x}u);",
        f"    ndsRendererRecordSetTileSize(stats, 0x{raw1[15][0]:08x}u, 0x{raw1[15][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{raw1[16][0]:08x}u, (u32)(uintptr_t)image);",
        f"    ndsRendererRecordLoadBlock(stats, 0x{raw1[18][0]:08x}u, 0x{raw1[18][1]:08x}u);",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw1[20][0]:08x}u) | 0x{raw1[20][1]:08x}u;",
        "}", "",
        "static void ndsNativeItemMSBombFinish1(NDSRendererStats *stats)", "{",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw1[25][0]:08x}u) | 0x{raw1[25][1]:08x}u;",
        _othermode(raw1, 26), _othermode(raw1, 27), _othermode(raw1, 28),
        "}", "",
    ]
    check = "ITEM_MSBOMB_NATIVE_OK roots=0x37a0,0x38b0 verts=4+4 tris=2+2 material=none de=none"
    return "\n".join(lines), header, check


def _nbumper(model, attr):
    root = 0x7558
    raw = _words(model, root, 30)
    _expect_ops(raw, (
        0xE7, 0xE3, 0xE2, 0xE2, 0xFC, 0xF9, 0xE8, 0xF5,
        0xF5, 0xF5, 0xDE, 0xE6, 0xF0, 0xE7, 0xD7, 0xF2,
        0xFD, 0xE6, 0xF3, 0xE7, 0xD9, 0x01, 0x06, 0xE7,
        0xE7, 0xD9, 0xE3, 0xE2, 0xE2, 0xDF,
    ), "NBumper")
    _expect_de(raw, ((10, 0xDE000000, 0x0E000000),), "NBumper")
    _expect_ptr(attr, 0x69C, 0x7648, "NBumper ITAttributes.data")
    _expect_ptr(attr, 0x6A0, 0x7488, "NBumper ITAttributes.p_mobjsubs")
    _expect_null(attr, 0x6A4, "NBumper ITAttributes.anim_joints")
    _expect_null(attr, 0x6A8, "NBumper ITAttributes.p_matanim_joints")
    _expect_null(model, 0x7488, "NBumper MObj head 0")
    _expect_ptr(model, 0x748C, 0x7510, "NBumper MObj head 1")
    _expect_ptr(model, 0x7510, 0x7498, "NBumper MObjSub")
    flags = struct.unpack_from(">H", model.payload, 0x7498 + 0x30)[0]
    if flags != 0x0004:
        raise RuntimeError(f"NBumper MObj flags changed: {flags:#x}")
    if model.pointer_at(0x7498 + 4) is not None:
        raise RuntimeError("NBumper unexpectedly gained a sprite stream")
    palettes = _ptr_offsets(model, 0x7490, 2, "NBumper palettes")
    if struct.unpack_from(">i", model.payload, 0x7648)[0] != 0:
        raise RuntimeError("NBumper DObjDesc root changed")
    if struct.unpack_from(">i", model.payload, 0x7674)[0] != 1:
        raise RuntimeError("NBumper DObjDesc child changed")
    _expect_ptr(model, 0x7678, root, "NBumper DObj root")
    if struct.unpack_from(">i", model.payload, 0x76A0)[0] != 18:
        raise RuntimeError("NBumper DObjDesc terminator changed")
    _expect_ptr(model, root + 16 * 8 + 4, 0x7288, "NBumper image")
    _expect_ptr(model, root + 21 * 8 + 4, 0x7518, "NBumper vertices")
    verts = _decode_verts(model, 0x7518, 4)
    tris = _decode_tris(raw, ((22, 0x06),))
    packet_bytes = len(verts) * 14 + len(tris) * 6 + len(palettes) * 4
    header = _header("nbumper", "NBUMPER", (
        f"#define NDS_NATIVE_ITEM_NBUMPER_ASSET {ASSET}u",
        f"#define NDS_NATIVE_ITEM_NBUMPER_ROOT 0x{root:04x}u",
        "#define NDS_NATIVE_ITEM_NBUMPER_FILE_END 0x76ccu",
        "#define NDS_NATIVE_ITEM_NBUMPER_HOOK_W0 0xde000000u",
        "#define NDS_NATIVE_ITEM_NBUMPER_HOOK_W1 0x0e000000u",
        "#define NDS_NATIVE_ITEM_NBUMPER_MOBJ_FLAGS 0x0004u",
        "#define NDS_NATIVE_ITEM_NBUMPER_IMAGE_OFFSET 0x7288u",
        "#define NDS_NATIVE_ITEM_NBUMPER_VERTEX_OFFSET 0x7518u",
        f"#define NDS_NATIVE_ITEM_NBUMPER_VERTEX_COUNT {len(verts)}u",
        f"#define NDS_NATIVE_ITEM_NBUMPER_TRIANGLE_COUNT {len(tris)}u",
        f"#define NDS_NATIVE_ITEM_NBUMPER_CORNER_COUNT {len(tris) * 3}u",
        f"#define NDS_NATIVE_ITEM_NBUMPER_PALETTE_COUNT {len(palettes)}u",
        f"#define NDS_NATIVE_ITEM_NBUMPER_PACKET_ROM_BYTES {packet_bytes}u",
        f"#define NDS_NATIVE_ITEM_NBUMPER_PACKET_RAM_BYTES {packet_bytes}u",
    ))
    lines = [
        "/* Bumper / NBumper packet, generated from file 86 root 0x7558.",
        " * Word 10 is exactly 0xDE000000 0x0E000000. MObj flags 0x0004",
        " * make PALETTE_IMAGE the only live material input. */",
        "#include <nds/generated/nds_native_item_nbumper.generated.h>", "",
        _arrays("NBumper", verts, tris),
        "static const u32 sNdsNativeItemNBumperPaletteOffsets[NDS_NATIVE_ITEM_NBUMPER_PALETTE_COUNT] =", "{",
        *(f"    0x{x:04x}u," for x in palettes), "};", "",
        "static void ndsNativeItemNBumperSetup(NDSRendererStats *stats)", "{",
        _othermode(raw, 1), _othermode(raw, 2), _othermode(raw, 3),
        f"    ndsRendererRecordSetCombine(stats, 0x{raw[4][0]:08x}u, 0x{raw[4][1]:08x}u);",
        f"    stats->blend_color = 0x{raw[5][1]:08x}u;",
        *(f"    ndsRendererRecordSetTile(stats, 0x{raw[i][0]:08x}u, 0x{raw[i][1]:08x}u);" for i in (7, 8, 9)),
        "}", "",
        "static void ndsNativeItemNBumperAfterMaterial(NDSRendererStats *stats, const void *image)", "{",
        f"    ndsRendererRecordLoadTlut(stats, 0x{raw[12][1]:08x}u);",
        f"    ndsRendererRecordTextureState(stats, 0x{raw[14][0]:08x}u, 0x{raw[14][1]:08x}u);",
        f"    ndsRendererRecordSetTileSize(stats, 0x{raw[15][0]:08x}u, 0x{raw[15][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{raw[16][0]:08x}u, (u32)(uintptr_t)image);",
        f"    ndsRendererRecordLoadBlock(stats, 0x{raw[18][0]:08x}u, 0x{raw[18][1]:08x}u);",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[20][0]:08x}u) | 0x{raw[20][1]:08x}u;",
        "}", "",
        "static void ndsNativeItemNBumperFinish(NDSRendererStats *stats)", "{",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[25][0]:08x}u) | 0x{raw[25][1]:08x}u;",
        _othermode(raw, 26), _othermode(raw, 27), _othermode(raw, 28),
        "}", "",
    ]
    check = "ITEM_NBUMPER_NATIVE_OK root=0x7558 verts=4 tris=2 material=PALETTE_IMAGE hook=word10:0x0e000000"
    return "\n".join(lines), header, check


def _box(model, attr):
    root = 0x6630
    raw = _words(model, root, 41)
    _expect_ops(raw, (
        0xE7, 0xD9, 0xE3, 0xFC, 0xE8, 0xF5, 0xF5, 0xF5,
        0xFD, 0xE6, 0xF0, 0xE7, 0xD7, 0xF2, 0xFD, 0xE6,
        0xF3, 0xE7, 0x01, 0x06, 0x06, 0x06, 0x06, 0xE7,
        0xFD, 0xE6, 0xF0, 0xE7, 0xFD, 0xE6, 0xF3, 0xE7,
        0x01, 0x01, 0x06, 0x06, 0xE7, 0xE7, 0xD9, 0xE3,
        0xDF,
    ), "Box")
    _expect_de(raw, (), "Box")
    _expect_ptr(attr, 0x5CC, 0x6778, "Box ITAttributes.data")
    for slot, label in ((0x5D0, "p_mobjsubs"), (0x5D4, "anim_joints"),
                        (0x5D8, "p_matanim_joints")):
        _expect_null(attr, slot, f"Box ITAttributes.{label}")
    if ((struct.unpack_from(">i", model.payload, 0x6778)[0] != 0) or
            (model.pointer_at(0x677C) is not None)):
        raise RuntimeError("Box DObjDesc root changed")
    if struct.unpack_from(">i", model.payload, 0x67A4)[0] != 1:
        raise RuntimeError("Box DObjDesc child changed")
    _expect_ptr(model, 0x67A8, root, "Box DObj display list")
    if struct.unpack_from(">i", model.payload, 0x67D0)[0] != 18:
        raise RuntimeError("Box DObjDesc terminator changed")
    for slot, off, label in (
        (root + 8 * 8 + 4, 0x60C0, "Box TLUT 0"),
        (root + 14 * 8 + 4, 0x62F0, "Box image 0"),
        (root + 18 * 8 + 4, 0x64F0, "Box vertices 0"),
        (root + 24 * 8 + 4, 0x6098, "Box TLUT 1"),
        (root + 28 * 8 + 4, 0x60E8, "Box image 1"),
        (root + 32 * 8 + 4, 0x65A0, "Box vertices 1 prefix"),
        (root + 33 * 8 + 4, 0x65D0, "Box vertices 1 suffix"),
    ):
        _expect_ptr(model, slot, off, label)
    verts0 = _decode_verts(model, 0x64F0, 14)
    verts1 = _decode_verts(model, 0x65A0, 2) + _decode_verts(model, 0x65D0, 6)
    tris0 = _decode_tris(raw, tuple((i, 0x06) for i in range(19, 23)))
    tris1 = _decode_tris(raw, ((34, 0x06), (35, 0x06)))
    packet_bytes = (len(verts0) * 14 + len(tris0) * 6 +
                    len(verts1) * 14 + len(tris1) * 6)
    header = _header("box", "BOX", (
        f"#define NDS_NATIVE_ITEM_BOX_ASSET {ASSET}u",
        f"#define NDS_NATIVE_ITEM_BOX_ROOT 0x{root:04x}u",
        "#define NDS_NATIVE_ITEM_BOX_FILE_END 0x6778u",
        "#define NDS_NATIVE_ITEM_BOX_TLUT0_OFFSET 0x60c0u",
        f"#define NDS_NATIVE_ITEM_BOX_TLUT0_W0 0x{raw[8][0]:08x}u",
        "#define NDS_NATIVE_ITEM_BOX_IMAGE0_OFFSET 0x62f0u",
        f"#define NDS_NATIVE_ITEM_BOX_IMAGE0_W0 0x{raw[14][0]:08x}u",
        "#define NDS_NATIVE_ITEM_BOX_VERTEX0_OFFSET 0x64f0u",
        "#define NDS_NATIVE_ITEM_BOX_TLUT1_OFFSET 0x6098u",
        f"#define NDS_NATIVE_ITEM_BOX_TLUT1_W0 0x{raw[24][0]:08x}u",
        "#define NDS_NATIVE_ITEM_BOX_IMAGE1_OFFSET 0x60e8u",
        f"#define NDS_NATIVE_ITEM_BOX_IMAGE1_W0 0x{raw[28][0]:08x}u",
        "#define NDS_NATIVE_ITEM_BOX_VERTEX1A_OFFSET 0x65a0u",
        "#define NDS_NATIVE_ITEM_BOX_VERTEX1B_OFFSET 0x65d0u",
        f"#define NDS_NATIVE_ITEM_BOX_VERTEX0_COUNT {len(verts0)}u",
        f"#define NDS_NATIVE_ITEM_BOX_TRIANGLE0_COUNT {len(tris0)}u",
        f"#define NDS_NATIVE_ITEM_BOX_CORNER0_COUNT {len(tris0) * 3}u",
        f"#define NDS_NATIVE_ITEM_BOX_VERTEX1_COUNT {len(verts1)}u",
        f"#define NDS_NATIVE_ITEM_BOX_TRIANGLE1_COUNT {len(tris1)}u",
        f"#define NDS_NATIVE_ITEM_BOX_CORNER1_COUNT {len(tris1) * 3}u",
        f"#define NDS_NATIVE_ITEM_BOX_PACKET_ROM_BYTES {packet_bytes}u",
        f"#define NDS_NATIVE_ITEM_BOX_PACKET_RAM_BYTES {packet_bytes}u",
    ))
    lines = [
        "/* Crate / Box packet, generated from file 86 root 0x6630.",
        " * No 0xDE appears; both fixed material/geometry phases bake.",
        " * The second phase preserves the source's 2+6 vertex reload. */",
        "#include <nds/generated/nds_native_item_box.generated.h>", "",
        _arrays("Box0", verts0, tris0), _arrays("Box1", verts1, tris1),
        "static void ndsNativeItemBoxSetup0(NDSRendererStats *stats, const void *tlut, const void *image)", "{",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[1][0]:08x}u) | 0x{raw[1][1]:08x}u;",
        _othermode(raw, 2),
        f"    ndsRendererRecordSetCombine(stats, 0x{raw[3][0]:08x}u, 0x{raw[3][1]:08x}u);",
        *(f"    ndsRendererRecordSetTile(stats, 0x{raw[i][0]:08x}u, 0x{raw[i][1]:08x}u);" for i in (5, 6, 7)),
        f"    ndsRendererRecordSetImage(stats, 0x{raw[8][0]:08x}u, (u32)(uintptr_t)tlut);",
        f"    ndsRendererRecordLoadTlut(stats, 0x{raw[10][1]:08x}u);",
        f"    ndsRendererRecordTextureState(stats, 0x{raw[12][0]:08x}u, 0x{raw[12][1]:08x}u);",
        f"    ndsRendererRecordSetTileSize(stats, 0x{raw[13][0]:08x}u, 0x{raw[13][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{raw[14][0]:08x}u, (u32)(uintptr_t)image);",
        f"    ndsRendererRecordLoadBlock(stats, 0x{raw[16][0]:08x}u, 0x{raw[16][1]:08x}u);",
        "}", "",
        "static void ndsNativeItemBoxSetup1(NDSRendererStats *stats, const void *tlut, const void *image)", "{",
        f"    ndsRendererRecordSetImage(stats, 0x{raw[24][0]:08x}u, (u32)(uintptr_t)tlut);",
        f"    ndsRendererRecordLoadTlut(stats, 0x{raw[26][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{raw[28][0]:08x}u, (u32)(uintptr_t)image);",
        f"    ndsRendererRecordLoadBlock(stats, 0x{raw[30][0]:08x}u, 0x{raw[30][1]:08x}u);",
        "}", "",
        "static void ndsNativeItemBoxFinish(NDSRendererStats *stats)", "{",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[38][0]:08x}u) | 0x{raw[38][1]:08x}u;",
        _othermode(raw, 39), "}", "",
    ]
    check = "ITEM_BOX_NATIVE_OK root=0x6630 verts=14+8 tris=8+4 material=none de=none"
    return "\n".join(lines), header, check


def _taru(model, attr):
    root = 0x7000
    raw = _words(model, root, 53)
    _expect_ops(raw, (
        0xE7, 0xDB, 0xDB, 0xDB, 0xDB, 0xE3, 0xFC, 0xE8,
        0xF5, 0xF5, 0xF5, 0xFD, 0xE6, 0xF0, 0xE7, 0xD7,
        0xF2, 0xFD, 0xE6, 0xF3, 0xE7, 0x01, 0x06, 0x06,
        0x06, 0xE7, 0xDB, 0xDB, 0xF5, 0xFD, 0xE6, 0xF0,
        0xE7, 0xF2, 0xFD, 0xE6, 0xF3, 0xE7, 0x01, 0x06,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        0x06, 0xE7, 0xE7, 0xE3, 0xDF,
    ), "Taru")
    _expect_de(raw, (), "Taru")
    _expect_ptr(attr, 0x634, 0x71A8, "Taru ITAttributes.data")
    for slot, label in ((0x638, "p_mobjsubs"), (0x63C, "anim_joints"),
                        (0x640, "p_matanim_joints")):
        _expect_null(attr, slot, f"Taru ITAttributes.{label}")
    if ((struct.unpack_from(">i", model.payload, 0x71A8)[0] != 0) or
            (model.pointer_at(0x71AC) is not None)):
        raise RuntimeError("Taru DObjDesc root changed")
    if struct.unpack_from(">i", model.payload, 0x71D4)[0] != 1:
        raise RuntimeError("Taru DObjDesc child changed")
    _expect_ptr(model, 0x71D8, root, "Taru DObj display list")
    if struct.unpack_from(">i", model.payload, 0x7200)[0] != 18:
        raise RuntimeError("Taru DObjDesc terminator changed")
    for slot, off, label in (
        (root + 11 * 8 + 4, 0x6A10, "Taru TLUT 0"),
        (root + 17 * 8 + 4, 0x6C40, "Taru image 0"),
        (root + 21 * 8 + 4, 0x6E40, "Taru vertices 0"),
        (root + 29 * 8 + 4, 0x69E8, "Taru TLUT 1"),
        (root + 34 * 8 + 4, 0x6A38, "Taru image 1"),
        (root + 38 * 8 + 4, 0x6EE0, "Taru vertices 1"),
    ):
        _expect_ptr(model, slot, off, label)
    verts0 = _decode_verts(model, 0x6E40, 10)
    verts1 = _decode_verts(model, 0x6EE0, 18)
    tris0 = _decode_tris(raw, tuple((i, 0x06) for i in range(22, 25)))
    tris1 = _decode_tris(raw, tuple((i, 0x06) for i in range(39, 49)))
    packet_bytes = (len(verts0) * 14 + len(tris0) * 6 +
                    len(verts1) * 14 + len(tris1) * 6)
    header = _header("taru", "TARU", (
        f"#define NDS_NATIVE_ITEM_TARU_ASSET {ASSET}u",
        f"#define NDS_NATIVE_ITEM_TARU_ROOT 0x{root:04x}u",
        "#define NDS_NATIVE_ITEM_TARU_FILE_END 0x71a8u",
        "#define NDS_NATIVE_ITEM_TARU_TLUT0_OFFSET 0x6a10u",
        f"#define NDS_NATIVE_ITEM_TARU_TLUT0_W0 0x{raw[11][0]:08x}u",
        "#define NDS_NATIVE_ITEM_TARU_IMAGE0_OFFSET 0x6c40u",
        f"#define NDS_NATIVE_ITEM_TARU_IMAGE0_W0 0x{raw[17][0]:08x}u",
        "#define NDS_NATIVE_ITEM_TARU_VERTEX0_OFFSET 0x6e40u",
        "#define NDS_NATIVE_ITEM_TARU_TLUT1_OFFSET 0x69e8u",
        f"#define NDS_NATIVE_ITEM_TARU_TLUT1_W0 0x{raw[29][0]:08x}u",
        "#define NDS_NATIVE_ITEM_TARU_IMAGE1_OFFSET 0x6a38u",
        f"#define NDS_NATIVE_ITEM_TARU_IMAGE1_W0 0x{raw[34][0]:08x}u",
        "#define NDS_NATIVE_ITEM_TARU_VERTEX1_OFFSET 0x6ee0u",
        f"#define NDS_NATIVE_ITEM_TARU_VERTEX0_COUNT {len(verts0)}u",
        f"#define NDS_NATIVE_ITEM_TARU_TRIANGLE0_COUNT {len(tris0)}u",
        f"#define NDS_NATIVE_ITEM_TARU_CORNER0_COUNT {len(tris0) * 3}u",
        f"#define NDS_NATIVE_ITEM_TARU_VERTEX1_COUNT {len(verts1)}u",
        f"#define NDS_NATIVE_ITEM_TARU_TRIANGLE1_COUNT {len(tris1)}u",
        f"#define NDS_NATIVE_ITEM_TARU_CORNER1_COUNT {len(tris1) * 3}u",
        f"#define NDS_NATIVE_ITEM_TARU_PACKET_ROM_BYTES {packet_bytes}u",
        f"#define NDS_NATIVE_ITEM_TARU_PACKET_RAM_BYTES {packet_bytes}u",
    ))
    lines = [
        "/* Barrel / Taru packet, generated from file 86 root 0x7000.",
        " * No 0xDE appears; both fixed material/geometry phases bake. */",
        "#include <nds/generated/nds_native_item_taru.generated.h>", "",
        _arrays("Taru0", verts0, tris0), _arrays("Taru1", verts1, tris1),
        "static void ndsNativeItemTaruSetup0(NDSRendererStats *stats, NDSRendererTraversalState *state, const void *tlut, const void *image)", "{",
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
        "static void ndsNativeItemTaruSetup1(NDSRendererStats *stats, NDSRendererTraversalState *state, const void *tlut, const void *image)", "{",
        *(f"    ndsRendererApplyMatrixMoveWordCommand(stats, state, 0x{raw[i][0]:08x}u, 0x{raw[i][1]:08x}u);" for i in (26, 27)),
        f"    ndsRendererRecordSetTile(stats, 0x{raw[28][0]:08x}u, 0x{raw[28][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{raw[29][0]:08x}u, (u32)(uintptr_t)tlut);",
        f"    ndsRendererRecordLoadTlut(stats, 0x{raw[31][1]:08x}u);",
        f"    ndsRendererRecordSetTileSize(stats, 0x{raw[33][0]:08x}u, 0x{raw[33][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{raw[34][0]:08x}u, (u32)(uintptr_t)image);",
        f"    ndsRendererRecordLoadBlock(stats, 0x{raw[36][0]:08x}u, 0x{raw[36][1]:08x}u);",
        "}", "",
        "static void ndsNativeItemTaruFinish(NDSRendererStats *stats)", "{",
        _othermode(raw, 51), "}", "",
    ]
    check = "ITEM_TARU_NATIVE_OK root=0x7000 verts=10+18 tris=6+20 material=none de=none"
    return "\n".join(lines), header, check


def _egg(model, attr):
    root = 0x103B0
    raw = _words(model, root, 30)
    _expect_ops(raw, (
        0xE7, 0xE3, 0xE2, 0xE2, 0xFC, 0xF9, 0xE8, 0xF5,
        0xF5, 0xF5, 0xFD, 0xE6, 0xF0, 0xE7, 0xD7, 0xF2,
        0xFD, 0xE6, 0xF3, 0xE7, 0xD9, 0x01, 0x05, 0xE7,
        0xE7, 0xD9, 0xE3, 0xE2, 0xE2, 0xDF,
    ), "Egg")
    _expect_de(raw, (), "Egg")
    _expect_ptr(attr, 0xACC, 0x104A0, "Egg ITAttributes.data")
    _expect_null(attr, 0xAD0, "Egg ITAttributes.p_mobjsubs")
    _expect_ptr(attr, 0xAD4, 0x10550, "Egg ITAttributes.anim_joints")
    _expect_null(attr, 0xAD8, "Egg ITAttributes.p_matanim_joints")
    if ((struct.unpack_from(">i", model.payload, 0x104A0)[0] != 0) or
            (model.pointer_at(0x104A4) is not None)):
        raise RuntimeError("Egg DObjDesc root changed")
    if ((struct.unpack_from(">i", model.payload, 0x104CC)[0] != 1) or
            (model.pointer_at(0x104D0) is not None)):
        raise RuntimeError("Egg DObjDesc transform child changed")
    if struct.unpack_from(">i", model.payload, 0x104F8)[0] != 2:
        raise RuntimeError("Egg DObjDesc drawable child changed")
    _expect_ptr(model, 0x104FC, root, "Egg DObj display list")
    if struct.unpack_from(">i", model.payload, 0x10524)[0] != 18:
        raise RuntimeError("Egg DObjDesc terminator changed")
    _expect_null(model, 0x10550, "Egg anim joint 0")
    _expect_null(model, 0x10554, "Egg anim joint 1")
    _expect_ptr(model, 0x10558, 0x1055C, "Egg drawable anim joint")
    for slot, off, label in (
        (root + 10 * 8 + 4, 0x10158, "Egg TLUT"),
        (root + 16 * 8 + 4, 0x10180, "Egg image"),
        (root + 21 * 8 + 4, 0x10380, "Egg vertices"),
    ):
        _expect_ptr(model, slot, off, label)
    verts = _decode_verts(model, 0x10380, 3)
    tris = _decode_tris(raw, ((22, 0x05),))
    packet_bytes = len(verts) * 14 + len(tris) * 6
    header = _header("egg", "EGG", (
        f"#define NDS_NATIVE_ITEM_EGG_ASSET {ASSET}u",
        f"#define NDS_NATIVE_ITEM_EGG_ROOT 0x{root:04x}u",
        "#define NDS_NATIVE_ITEM_EGG_FILE_END 0x104a0u",
        "#define NDS_NATIVE_ITEM_EGG_TLUT_OFFSET 0x10158u",
        f"#define NDS_NATIVE_ITEM_EGG_TLUT_W0 0x{raw[10][0]:08x}u",
        "#define NDS_NATIVE_ITEM_EGG_IMAGE_OFFSET 0x10180u",
        f"#define NDS_NATIVE_ITEM_EGG_IMAGE_W0 0x{raw[16][0]:08x}u",
        "#define NDS_NATIVE_ITEM_EGG_VERTEX_OFFSET 0x10380u",
        f"#define NDS_NATIVE_ITEM_EGG_VERTEX_COUNT {len(verts)}u",
        f"#define NDS_NATIVE_ITEM_EGG_TRIANGLE_COUNT {len(tris)}u",
        f"#define NDS_NATIVE_ITEM_EGG_CORNER_COUNT {len(tris) * 3}u",
        f"#define NDS_NATIVE_ITEM_EGG_PACKET_ROM_BYTES {packet_bytes}u",
        f"#define NDS_NATIVE_ITEM_EGG_PACKET_RAM_BYTES {packet_bytes}u",
    ))
    lines = [
        "/* Egg packet, generated from file 86 root 0x103b0.",
        " * No 0xDE appears; material/geometry bake while the source DObj",
        " * animation remains represented by the adapter's incoming modelview. */",
        "#include <nds/generated/nds_native_item_egg.generated.h>", "",
        _arrays("Egg", verts, tris),
        "static void ndsNativeItemEggSetup(NDSRendererStats *stats, const void *tlut, const void *image)", "{",
        _othermode(raw, 1), _othermode(raw, 2), _othermode(raw, 3),
        f"    ndsRendererRecordSetCombine(stats, 0x{raw[4][0]:08x}u, 0x{raw[4][1]:08x}u);",
        f"    stats->blend_color = 0x{raw[5][1]:08x}u;",
        *(f"    ndsRendererRecordSetTile(stats, 0x{raw[i][0]:08x}u, 0x{raw[i][1]:08x}u);" for i in (7, 8, 9)),
        f"    ndsRendererRecordSetImage(stats, 0x{raw[10][0]:08x}u, (u32)(uintptr_t)tlut);",
        f"    ndsRendererRecordLoadTlut(stats, 0x{raw[12][1]:08x}u);",
        f"    ndsRendererRecordTextureState(stats, 0x{raw[14][0]:08x}u, 0x{raw[14][1]:08x}u);",
        f"    ndsRendererRecordSetTileSize(stats, 0x{raw[15][0]:08x}u, 0x{raw[15][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{raw[16][0]:08x}u, (u32)(uintptr_t)image);",
        f"    ndsRendererRecordLoadBlock(stats, 0x{raw[18][0]:08x}u, 0x{raw[18][1]:08x}u);",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[20][0]:08x}u) | 0x{raw[20][1]:08x}u;",
        "}", "",
        "static void ndsNativeItemEggFinish(NDSRendererStats *stats)", "{",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[25][0]:08x}u) | 0x{raw[25][1]:08x}u;",
        _othermode(raw, 26), _othermode(raw, 27), _othermode(raw, 28), "}", "",
    ]
    check = "ITEM_EGG_NATIVE_OK root=0x103b0 verts=3 tris=1 material=none de=none anim=joint2"
    return "\n".join(lines), header, check


def _iwark(model, attr):
    root = 0xA050
    raw = _words(model, root, 30)
    _expect_ops(raw, (
        0xE7, 0xE3, 0xE2, 0xE2, 0xFC, 0xF9, 0xE8, 0xF5,
        0xF5, 0xF5, 0xFD, 0xE6, 0xF0, 0xE7, 0xD7, 0xF2,
        0xFD, 0xE6, 0xF3, 0xE7, 0xD9, 0x01, 0x06, 0xE7,
        0xE7, 0xD9, 0xE3, 0xE2, 0xE2, 0xDF,
    ), "Iwark")
    _expect_de(raw, (), "Iwark")
    _expect_ptr(attr, 0x72C, 0xA140, "Iwark/Wark ITAttributes.data")
    for slot, label in ((0x730, "p_mobjsubs"), (0x734, "anim_joints"),
                        (0x738, "p_matanim_joints")):
        _expect_null(attr, slot, f"Iwark/Wark ITAttributes.{label}")
    if ((struct.unpack_from(">i", model.payload, 0xA140)[0] != 0) or
            (model.pointer_at(0xA144) is not None)):
        raise RuntimeError("Iwark DObjDesc root changed")
    if struct.unpack_from(">i", model.payload, 0xA16C)[0] != 1:
        raise RuntimeError("Iwark DObjDesc child changed")
    _expect_ptr(model, 0xA170, root, "Iwark DObj display list")
    if struct.unpack_from(">i", model.payload, 0xA198)[0] != 18:
        raise RuntimeError("Iwark DObjDesc terminator changed")
    for slot, off, label in (
        (root + 10 * 8 + 4, 0x98E8, "Iwark TLUT"),
        (root + 16 * 8 + 4, 0x9910, "Iwark image"),
        (root + 21 * 8 + 4, 0xA010, "Iwark vertices"),
    ):
        _expect_ptr(model, slot, off, label)
    verts = _decode_verts(model, 0xA010, 4)
    tris = _decode_tris(raw, ((22, 0x06),))
    packet_bytes = len(verts) * 14 + len(tris) * 6
    header = _header("iwark", "IWARK", (
        f"#define NDS_NATIVE_ITEM_IWARK_ASSET {ASSET}u",
        f"#define NDS_NATIVE_ITEM_IWARK_ROOT 0x{root:04x}u",
        "#define NDS_NATIVE_ITEM_IWARK_FILE_END 0xa140u",
        "#define NDS_NATIVE_ITEM_IWARK_TLUT_OFFSET 0x98e8u",
        f"#define NDS_NATIVE_ITEM_IWARK_TLUT_W0 0x{raw[10][0]:08x}u",
        "#define NDS_NATIVE_ITEM_IWARK_IMAGE_OFFSET 0x9910u",
        f"#define NDS_NATIVE_ITEM_IWARK_IMAGE_W0 0x{raw[16][0]:08x}u",
        "#define NDS_NATIVE_ITEM_IWARK_VERTEX_OFFSET 0xa010u",
        f"#define NDS_NATIVE_ITEM_IWARK_VERTEX_COUNT {len(verts)}u",
        f"#define NDS_NATIVE_ITEM_IWARK_TRIANGLE_COUNT {len(tris)}u",
        f"#define NDS_NATIVE_ITEM_IWARK_CORNER_COUNT {len(tris) * 3}u",
        f"#define NDS_NATIVE_ITEM_IWARK_PACKET_ROM_BYTES {packet_bytes}u",
        f"#define NDS_NATIVE_ITEM_IWARK_PACKET_RAM_BYTES {packet_bytes}u",
    ))
    lines = [
        "/* Poké Ball Onix / Iwark packet, generated from file 86 root 0xa050.",
        " * No 0xDE appears; the complete fixed material/geometry program bakes. */",
        "#include <nds/generated/nds_native_item_iwark.generated.h>", "",
        _arrays("Iwark", verts, tris),
        "static void ndsNativeItemIwarkSetup(NDSRendererStats *stats, const void *tlut, const void *image)", "{",
        _othermode(raw, 1), _othermode(raw, 2), _othermode(raw, 3),
        f"    ndsRendererRecordSetCombine(stats, 0x{raw[4][0]:08x}u, 0x{raw[4][1]:08x}u);",
        f"    stats->blend_color = 0x{raw[5][1]:08x}u;",
        *(f"    ndsRendererRecordSetTile(stats, 0x{raw[i][0]:08x}u, 0x{raw[i][1]:08x}u);" for i in (7, 8, 9)),
        f"    ndsRendererRecordSetImage(stats, 0x{raw[10][0]:08x}u, (u32)(uintptr_t)tlut);",
        f"    ndsRendererRecordLoadTlut(stats, 0x{raw[12][1]:08x}u);",
        f"    ndsRendererRecordTextureState(stats, 0x{raw[14][0]:08x}u, 0x{raw[14][1]:08x}u);",
        f"    ndsRendererRecordSetTileSize(stats, 0x{raw[15][0]:08x}u, 0x{raw[15][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{raw[16][0]:08x}u, (u32)(uintptr_t)image);",
        f"    ndsRendererRecordLoadBlock(stats, 0x{raw[18][0]:08x}u, 0x{raw[18][1]:08x}u);",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[20][0]:08x}u) | 0x{raw[20][1]:08x}u;",
        "}", "",
        "static void ndsNativeItemIwarkFinish(NDSRendererStats *stats)", "{",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[25][0]:08x}u) | 0x{raw[25][1]:08x}u;",
        _othermode(raw, 26), _othermode(raw, 27), _othermode(raw, 28), "}", "",
    ]
    check = "ITEM_IWARK_NATIVE_OK root=0xa050 verts=4 tris=2 material=none de=none"
    return "\n".join(lines), header, check


CAPSULE_HEADER_OPS = (
    0xE7, 0xD9, 0xE8, 0xF5, 0xF5, 0xDB, 0xDB, 0xDB,
    0xDB, 0xF9, 0xDE, 0xDF,
)
CAPSULE_CALLEE_OPS = (
    0xE7, 0xE3, 0xFC, 0xF5, 0xFD, 0xE6, 0xF0, 0xE7,
    0xD7, 0xF2, 0xFD, 0xE6, 0xF3, 0xE7, 0x01, 0x06,
    0x06, 0x06, 0x06, 0x06, 0x06, 0xE7, 0xE3, 0xD9,
    0xFC, 0xD7, 0x01, 0x06, 0x06, 0x06, 0x06, 0xDF,
)
CAPSULE_SECOND_OPS = (
    0xE7, 0xD9, 0xE3, 0xE2, 0xE2, 0xFC, 0xF5, 0xFD,
    0xE6, 0xF0, 0xE7, 0xD7, 0xF2, 0xFD, 0xE6, 0xF3,
    0xE7, 0x01, 0x06, 0xDF,
)
CAPSULE_THIRD_OPS = (
    0xE7, 0xFD, 0xE6, 0xF0, 0xE7, 0xFD, 0xE6, 0xF3,
    0xE7, 0x01, 0x06, 0xE7, 0xE7, 0xD9, 0xE3, 0xE2,
    0xE2, 0xDF,
)


def _capsule(model, attr):
    """Fixed two-sibling Capsule owner; its segment-1 callee is inlined."""
    header_root, callee_root = 0x03E0, 0x0440
    second_root, third_root = 0x0540, 0x05E0
    header_raw = _words(model, header_root, 12)
    callee_raw = _words(model, callee_root, 32)
    second_raw = _words(model, second_root, 20)
    third_raw = _words(model, third_root, 18)
    _expect_ops(header_raw, CAPSULE_HEADER_OPS, "Capsule header")
    _expect_ops(callee_raw, CAPSULE_CALLEE_OPS, "Capsule callee")
    _expect_ops(second_raw, CAPSULE_SECOND_OPS, "Capsule second root")
    _expect_ops(third_raw, CAPSULE_THIRD_OPS, "Capsule third root")
    _expect_de(header_raw, ((10, 0xDE000000, 0x01190110),), "Capsule header")
    _expect_de(callee_raw, (), "Capsule callee")
    _expect_de(second_raw, (), "Capsule second root")
    _expect_de(third_raw, (), "Capsule third root")

    _expect_ptr(attr, 0x050, 0x0670, "Capsule ITAttributes.data")
    for slot, label in ((0x054, "p_mobjsubs"), (0x058, "anim_joints"),
                        (0x05C, "p_matanim_joints")):
        _expect_null(attr, slot, f"Capsule ITAttributes.{label}")

    if ((struct.unpack_from(">i", model.payload, 0x0670)[0] != 0) or
            (model.pointer_at(0x0674) is not None)):
        raise RuntimeError("Capsule DObjDesc root changed")
    if struct.unpack_from(">i", model.payload, 0x069C)[0] != 1:
        raise RuntimeError("Capsule DObjDesc child 1 id changed")
    _expect_ptr(model, 0x06A0, header_root, "Capsule DObjDesc child 1")
    if struct.unpack_from(">i", model.payload, 0x06C8)[0] != 0x4002:
        raise RuntimeError("Capsule DObjDesc child 2 id changed")
    _expect_ptr(model, 0x06CC, second_root, "Capsule DObjDesc child 2")
    if struct.unpack_from(">i", model.payload, 0x06F4)[0] != 0x4002:
        raise RuntimeError("Capsule DObjDesc child 3 id changed")
    _expect_ptr(model, 0x06F8, third_root, "Capsule DObjDesc child 3")
    if struct.unpack_from(">i", model.payload, 0x0720)[0] != 18:
        raise RuntimeError("Capsule DObjDesc terminator changed")

    _expect_ptr(model, header_root + 10 * 8 + 4, callee_root,
                "Capsule segment-1 branch callee")
    for slot, off, label in (
        (callee_root + 4 * 8 + 4, 0x0008, "Capsule callee TLUT"),
        (callee_root + 10 * 8 + 4, 0x0080, "Capsule callee image"),
        (callee_root + 14 * 8 + 4, 0x0210, "Capsule callee vertices 0"),
        (callee_root + 26 * 8 + 4, 0x02F0, "Capsule callee vertices 1"),
        (second_root + 7 * 8 + 4, 0x0058, "Capsule second TLUT"),
        (second_root + 13 * 8 + 4, 0x0190, "Capsule second image"),
        (second_root + 17 * 8 + 4, 0x03A0, "Capsule second vertices"),
        (third_root + 1 * 8 + 4, 0x0030, "Capsule third TLUT"),
        (third_root + 5 * 8 + 4, 0x0108, "Capsule third image"),
        (third_root + 9 * 8 + 4, 0x03A0, "Capsule third vertices"),
    ):
        _expect_ptr(model, slot, off, label)

    verts0 = _decode_verts(model, 0x0210, 14)
    verts1 = _decode_verts(model, 0x02F0, 11)
    verts2 = _decode_verts(model, 0x03A0, 4)
    verts3 = _decode_verts(model, 0x03A0, 4)
    tris0 = _decode_tris(callee_raw, tuple((i, 0x06) for i in range(15, 21)))
    tris1 = _decode_tris(callee_raw, tuple((i, 0x06) for i in range(27, 31)))
    tris2 = _decode_tris(second_raw, ((18, 0x06),))
    tris3 = _decode_tris(third_raw, ((10, 0x06),))
    packet_bytes = (
        len(verts0) * 14 + len(tris0) * 6 +
        len(verts1) * 14 + len(tris1) * 6 +
        len(verts2) * 14 + len(tris2) * 6 +
        len(verts3) * 14 + len(tris3) * 6
    )

    header = _header("capsule", "CAPSULE", (
        f"#define NDS_NATIVE_ITEM_CAPSULE_ASSET {ASSET}u",
        f"#define NDS_NATIVE_ITEM_CAPSULE_HEADER_ROOT 0x{header_root:04x}u",
        f"#define NDS_NATIVE_ITEM_CAPSULE_CALLEE_ROOT 0x{callee_root:04x}u",
        f"#define NDS_NATIVE_ITEM_CAPSULE_SECOND_ROOT 0x{second_root:04x}u",
        f"#define NDS_NATIVE_ITEM_CAPSULE_THIRD_ROOT 0x{third_root:04x}u",
        "#define NDS_NATIVE_ITEM_CAPSULE_FILE_END 0x0670u",
        "#define NDS_NATIVE_ITEM_CAPSULE_BRANCH_W0 0xde000000u",
        "#define NDS_NATIVE_ITEM_CAPSULE_BRANCH_RAW_W1 0x01190110u",
        "#define NDS_NATIVE_ITEM_CAPSULE_TLUT0_OFFSET 0x0008u",
        "#define NDS_NATIVE_ITEM_CAPSULE_IMAGE0_OFFSET 0x0080u",
        "#define NDS_NATIVE_ITEM_CAPSULE_VERTEX0_OFFSET 0x0210u",
        "#define NDS_NATIVE_ITEM_CAPSULE_VERTEX1_OFFSET 0x02f0u",
        "#define NDS_NATIVE_ITEM_CAPSULE_TLUT2_OFFSET 0x0058u",
        "#define NDS_NATIVE_ITEM_CAPSULE_IMAGE2_OFFSET 0x0190u",
        "#define NDS_NATIVE_ITEM_CAPSULE_VERTEX2_OFFSET 0x03a0u",
        "#define NDS_NATIVE_ITEM_CAPSULE_TLUT3_OFFSET 0x0030u",
        "#define NDS_NATIVE_ITEM_CAPSULE_IMAGE3_OFFSET 0x0108u",
        "#define NDS_NATIVE_ITEM_CAPSULE_VERTEX3_OFFSET 0x03a0u",
        f"#define NDS_NATIVE_ITEM_CAPSULE_VERTEX0_COUNT {len(verts0)}u",
        f"#define NDS_NATIVE_ITEM_CAPSULE_TRIANGLE0_COUNT {len(tris0)}u",
        f"#define NDS_NATIVE_ITEM_CAPSULE_CORNER0_COUNT {len(tris0) * 3}u",
        f"#define NDS_NATIVE_ITEM_CAPSULE_VERTEX1_COUNT {len(verts1)}u",
        f"#define NDS_NATIVE_ITEM_CAPSULE_TRIANGLE1_COUNT {len(tris1)}u",
        f"#define NDS_NATIVE_ITEM_CAPSULE_CORNER1_COUNT {len(tris1) * 3}u",
        f"#define NDS_NATIVE_ITEM_CAPSULE_VERTEX2_COUNT {len(verts2)}u",
        f"#define NDS_NATIVE_ITEM_CAPSULE_TRIANGLE2_COUNT {len(tris2)}u",
        f"#define NDS_NATIVE_ITEM_CAPSULE_CORNER2_COUNT {len(tris2) * 3}u",
        f"#define NDS_NATIVE_ITEM_CAPSULE_VERTEX3_COUNT {len(verts3)}u",
        f"#define NDS_NATIVE_ITEM_CAPSULE_TRIANGLE3_COUNT {len(tris3)}u",
        f"#define NDS_NATIVE_ITEM_CAPSULE_CORNER3_COUNT {len(tris3) * 3}u",
        f"#define NDS_NATIVE_ITEM_CAPSULE_PACKET_ROM_BYTES {packet_bytes}u",
        f"#define NDS_NATIVE_ITEM_CAPSULE_PACKET_RAM_BYTES {packet_bytes}u",
    ))

    lines = [
        "/* Capsule native packet, generated from file 86.",
        " * Root 0x03e0 word 10 branches to relocated 0x0440, inlined here;",
        " * roots 0x0540/0x05e0 are the two bake-only sibling DObjs. */",
        "#include <nds/generated/nds_native_item_capsule.generated.h>", "",
        _arrays("Capsule0", verts0, tris0),
        _arrays("Capsule1", verts1, tris1),
        _arrays("Capsule2", verts2, tris2),
        _arrays("Capsule3", verts3, tris3),
        "static void ndsNativeItemCapsuleHeaderSetup(",
        "    NDSRendererStats *stats, NDSRendererTraversalState *state)", "{",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{header_raw[1][0]:08x}u) | 0x{header_raw[1][1]:08x}u;",
        *(f"    ndsRendererRecordSetTile(stats, 0x{header_raw[i][0]:08x}u, 0x{header_raw[i][1]:08x}u);" for i in (3, 4)),
        *(f"    ndsRendererApplyMatrixMoveWordCommand(stats, state, 0x{header_raw[i][0]:08x}u, 0x{header_raw[i][1]:08x}u);" for i in (5, 6, 7, 8)),
        f"    stats->blend_color = 0x{header_raw[9][1]:08x}u;",
        "}", "",
        "static void ndsNativeItemCapsuleRun0Setup(NDSRendererStats *stats, const void *tlut, const void *image)", "{",
        _othermode(callee_raw, 1),
        f"    ndsRendererRecordSetCombine(stats, 0x{callee_raw[2][0]:08x}u, 0x{callee_raw[2][1]:08x}u);",
        f"    ndsRendererRecordSetTile(stats, 0x{callee_raw[3][0]:08x}u, 0x{callee_raw[3][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{callee_raw[4][0]:08x}u, (u32)(uintptr_t)tlut);",
        f"    ndsRendererRecordLoadTlut(stats, 0x{callee_raw[6][1]:08x}u);",
        f"    ndsRendererRecordTextureState(stats, 0x{callee_raw[8][0]:08x}u, 0x{callee_raw[8][1]:08x}u);",
        f"    ndsRendererRecordSetTileSize(stats, 0x{callee_raw[9][0]:08x}u, 0x{callee_raw[9][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{callee_raw[10][0]:08x}u, (u32)(uintptr_t)image);",
        f"    ndsRendererRecordLoadBlock(stats, 0x{callee_raw[12][0]:08x}u, 0x{callee_raw[12][1]:08x}u);",
        "}", "",
        "static void ndsNativeItemCapsuleRun1Setup(NDSRendererStats *stats)", "{",
        _othermode(callee_raw, 22),
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{callee_raw[23][0]:08x}u) | 0x{callee_raw[23][1]:08x}u;",
        f"    ndsRendererRecordSetCombine(stats, 0x{callee_raw[24][0]:08x}u, 0x{callee_raw[24][1]:08x}u);",
        f"    ndsRendererRecordTextureState(stats, 0x{callee_raw[25][0]:08x}u, 0x{callee_raw[25][1]:08x}u);",
        "}", "",
        "static void ndsNativeItemCapsuleSecondSetup(NDSRendererStats *stats, const void *tlut, const void *image)", "{",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{second_raw[1][0]:08x}u) | 0x{second_raw[1][1]:08x}u;",
        _othermode(second_raw, 2), _othermode(second_raw, 3), _othermode(second_raw, 4),
        f"    ndsRendererRecordSetCombine(stats, 0x{second_raw[5][0]:08x}u, 0x{second_raw[5][1]:08x}u);",
        f"    ndsRendererRecordSetTile(stats, 0x{second_raw[6][0]:08x}u, 0x{second_raw[6][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{second_raw[7][0]:08x}u, (u32)(uintptr_t)tlut);",
        f"    ndsRendererRecordLoadTlut(stats, 0x{second_raw[9][1]:08x}u);",
        f"    ndsRendererRecordTextureState(stats, 0x{second_raw[11][0]:08x}u, 0x{second_raw[11][1]:08x}u);",
        f"    ndsRendererRecordSetTileSize(stats, 0x{second_raw[12][0]:08x}u, 0x{second_raw[12][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{second_raw[13][0]:08x}u, (u32)(uintptr_t)image);",
        f"    ndsRendererRecordLoadBlock(stats, 0x{second_raw[15][0]:08x}u, 0x{second_raw[15][1]:08x}u);",
        "}", "",
        "static void ndsNativeItemCapsuleSecondFinish(NDSRendererStats *stats)", "{",
        "    (void)stats;",
        "}", "",
        "static void ndsNativeItemCapsuleThirdSetup(NDSRendererStats *stats, const void *tlut, const void *image)", "{",
        f"    ndsRendererRecordSetImage(stats, 0x{third_raw[1][0]:08x}u, (u32)(uintptr_t)tlut);",
        f"    ndsRendererRecordLoadTlut(stats, 0x{third_raw[3][1]:08x}u);",
        f"    ndsRendererRecordSetImage(stats, 0x{third_raw[5][0]:08x}u, (u32)(uintptr_t)image);",
        f"    ndsRendererRecordLoadBlock(stats, 0x{third_raw[7][0]:08x}u, 0x{third_raw[7][1]:08x}u);",
        "}", "",
        "static void ndsNativeItemCapsuleThirdFinish(NDSRendererStats *stats)", "{",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{third_raw[13][0]:08x}u) | 0x{third_raw[13][1]:08x}u;",
        _othermode(third_raw, 14), _othermode(third_raw, 15), _othermode(third_raw, 16),
        "}", "",
    ]
    check = (
        "ITEM_CAPSULE_NATIVE_OK roots=0x03e0->0x0440,0x0540,0x05e0 "
        "verts=14+11+4+4 tris=12+8+2+2 material=none branch=word10:0x01190110"
    )
    return "\n".join(lines), header, check


GENERATORS = {
    "star": _star,
    "sword": _sword,
    "hammer": _hammer,
    "mball": _mball,
    "gshell": _gshell,
    "bat": _bat,
    "harisen": _harisen,
    "lgun": _lgun,
    "bombhei": _bombhei,
    "rshell": _rshell,
    "heart": _heart,
    "starrod": _starrod,
    "fflower": _fflower,
    "msbomb": _msbomb,
    "nbumper": _nbumper,
    "box": _box,
    "taru": _taru,
    "egg": _egg,
    "iwark": _iwark,
    "capsule": _capsule,
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
