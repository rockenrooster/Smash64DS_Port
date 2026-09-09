#!/usr/bin/env python3
"""Shared generator core for Saffron City's single-hook live-image monsters.

Hitokage and Fushigibana use the same source program shape: immutable palette,
render state and quad geometry with exactly one segment-0xE call at word 17.
The call supplies CURRENT_IMAGE from the live MObj.  Per-item scripts below
carry every measured address/value pin; this helper only keeps the conversion
and validation discipline identical for the pair.
"""

from __future__ import annotations

import argparse
import hashlib
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(REPO / "scripts" / "stages"))

import generate_nds_native_stage as sm  # noqa: E402


MODEL_FILE = sm.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/MiscDataBank159",
    "7b8c444662623b4a1948c82371401cdae5bdf3108223b798b696542adba0d1b2",
    159, 43, 0,
    "977292623f0a4a3e5175fc77e4e39105d23e76fca4bf0ebc76613413f84e0ab6")
ATTR_FILE = sm.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_stages/GRYamabukiMap",
    "8e17aa95be010e865711353a8ea965f9dc4f8ec9c17f615e453e0a8b3fb4505d",
    264, 1, 23,
    "4236015e06f24d4f8b701053b89df003bae5eac210cf7d3b7fbcddaf5fc53b85")

ASSET = 159
ATTR_ASSET = 264
DL_WORDS = 30
TLUT_ENTRIES = 16
TLUT_WORD = 11
LOADTLUT_WORD = 13
HOOK_WORD = 17
LOADBLOCK_WORD = 19
VTX_WORD = 21
TRI_WORD = 22

OPS = (
    0xE7, 0xD9, 0xE3, 0xE2, 0xE2, 0xFC, 0xF9, 0xE8,
    0xF5, 0xF5, 0xF5, 0xFD, 0xE6, 0xF0, 0xE7, 0xD7,
    0xF2, 0xDE, 0xE6, 0xF3, 0xE7, 0x01, 0x06, 0xE7,
    0xE7, 0xD9, 0xE3, 0xE2, 0xE2, 0xDF,
)
HOOK = (0xDE000000, 0x0E000000)
TRIS = ((3, 2, 1), (0, 3, 1))
EXPECTED_ATTR_REFS = (
    (0x00BC, ASSET, 0x0360), (0x00C4, ASSET, 0x03F0),
    (0x0104, ASSET, 0x0790), (0x010C, ASSET, 0x0820),
    (0x016C, ASSET, 0x0EA0), (0x0174, ASSET, 0x0F30),
    (0x01FC, ASSET, 0x1990), (0x0200, ASSET, 0x17D0),
    (0x0204, ASSET, 0x1A20), (0x0278, ASSET, 0x2340),
    (0x027C, ASSET, 0x2180), (0x0280, ASSET, 0x23D0),
    (0x0308, ASSET, 0x2A50),
)


@dataclass(frozen=True)
class YamabukiLiveItemSpec:
    name: str
    slug: str
    macro: str
    kind_token: str
    attr_token: str
    source_path: str
    attr_offset: int
    dobjdesc: int
    mobj_heads: int
    mobj_child: int
    animjoint: int
    anim_child: int
    root: int
    vtx: int
    tlut: int
    verts: tuple[tuple[int, int, int, int, int, int], ...]
    tlut_values: tuple[int, ...]
    tile_size: tuple[int, int]
    load_block: tuple[int, int]
    check_line: str


def _words_at(payload: bytes, off: int, count: int):
    return [struct.unpack_from(">II", payload, off + i * 8)
            for i in range(count)]


def _check_text_pins(spec: YamabukiLiveItemSpec) -> None:
    source = (REPO / spec.source_path).read_text(
        encoding="utf-8", errors="replace")
    for token in (
        spec.kind_token,
        spec.attr_token,
        "dobj->mobj->texture_id_curr = 1;",
        "else dobj->mobj->texture_id_curr = 0;",
    ):
        if token not in source:
            raise RuntimeError(f"{spec.source_path} pin missing {token!r}")
    reloc_path = REPO / "decomp/BattleShip-main/include/reloc_data.us.h"
    reloc = reloc_path.read_text(encoding="utf-8", errors="replace")
    attr_define = (
        f"#define {spec.attr_token.lstrip('&')} "
        f"((intptr_t)0x{spec.attr_offset:X})")
    if attr_define not in reloc:
        raise RuntimeError(f"reloc_data.us.h pin missing {attr_define!r}")


def _census() -> tuple[int, tuple[tuple[int, int, int], ...]]:
    root = REPO / "decomp/BattleShip-main/BattleShip_o2r"
    hits = []
    scanned = 0
    for path in sorted(root.rglob("*")):
        if not path.is_file():
            continue
        blob = path.read_bytes()
        if len(blob) < 0x50 or blob[4:8] != b"OLER":
            continue
        scanned += 1
        rel = str(path.relative_to(REPO)).replace("\\", "/")
        res = sm.load_o2r(
            REPO, sm.InputSpec(rel, hashlib.sha256(blob).hexdigest()))
        for slot, ref in res.external.items():
            if ref.asset_id == ASSET:
                hits.append((res.file_id, slot, ref.offset))
    return scanned, tuple(sorted(hits))


def decode(spec: YamabukiLiveItemSpec, run_census: bool = True):
    model = sm.load_o2r(REPO, MODEL_FILE)
    attr = sm.load_o2r(REPO, ATTR_FILE)
    _check_text_pins(spec)

    if len(model.payload) < spec.root + DL_WORDS * 8:
        raise RuntimeError(f"file 159 no longer contains the {spec.name} list")
    raw = _words_at(model.payload, spec.root, DL_WORDS)
    ops = tuple(w0 >> 24 for w0, _ in raw)
    if ops != OPS:
        raise RuntimeError(
            f"{spec.name} {spec.root:#06x} opcode census changed: {ops!r}")
    de_words = tuple((i, w0, w1) for i, (w0, w1) in enumerate(raw)
                     if (w0 >> 24) == 0xDE)
    if de_words != ((HOOK_WORD, HOOK[0], HOOK[1]),):
        raise RuntimeError(
            f"{spec.name} live-material hook changed: {de_words!r}")
    if model.pointer_at(spec.root + HOOK_WORD * 8 + 4) is not None:
        raise RuntimeError(f"{spec.name} segment-E hook gained a relocation")

    data_ref = attr.pointer_at(spec.attr_offset)
    mobj_ref = attr.pointer_at(spec.attr_offset + 4)
    anim_ref = attr.pointer_at(spec.attr_offset + 8)
    if data_ref is None or (data_ref.asset_id, data_ref.offset) != (
            ASSET, spec.dobjdesc):
        raise RuntimeError(f"{spec.name} ITAttributes.data changed: {data_ref!r}")
    if mobj_ref is None or (mobj_ref.asset_id, mobj_ref.offset) != (
            ASSET, spec.mobj_heads):
        raise RuntimeError(
            f"{spec.name} ITAttributes.p_mobjsubs changed: {mobj_ref!r}")
    if anim_ref is None or (anim_ref.asset_id, anim_ref.offset) != (
            ASSET, spec.animjoint):
        raise RuntimeError(
            f"{spec.name} ITAttributes.anim_joints changed: {anim_ref!r}")
    if (attr.pointer_at(spec.attr_offset + 12) is not None or
            struct.unpack_from(">I", attr.payload, spec.attr_offset + 12)[0] != 0):
        raise RuntimeError(f"{spec.name} gained material animation")
    flags = struct.unpack_from(">I", attr.payload, spec.attr_offset + 0x10)[0]
    if (flags & 0xE0000000) != 0x40000000:
        raise RuntimeError(f"{spec.name} display flags changed: {flags:#010x}")

    if struct.unpack_from(">i", model.payload, spec.dobjdesc)[0] != 0:
        raise RuntimeError(f"{spec.name} DObjDesc entry 0 id changed")
    if model.pointer_at(spec.dobjdesc + 4) is not None:
        raise RuntimeError(f"{spec.name} DObjDesc entry 0 gained a list")
    if struct.unpack_from(">i", model.payload, spec.dobjdesc + 0x2C)[0] != 1:
        raise RuntimeError(f"{spec.name} DObjDesc entry 1 id changed")
    child = model.pointer_at(spec.dobjdesc + 0x30)
    if child is None or (child.asset_id, child.offset) != (ASSET, spec.root):
        raise RuntimeError(f"{spec.name} drawable child changed: {child!r}")
    if struct.unpack_from(">i", model.payload, spec.dobjdesc + 0x58)[0] != 18:
        raise RuntimeError(f"{spec.name} DObjDesc terminator changed")

    if model.pointer_at(spec.mobj_heads) is not None:
        raise RuntimeError(f"{spec.name} MObj head 0 is no longer NULL")
    mobj_child = model.pointer_at(spec.mobj_heads + 4)
    if mobj_child is None or (mobj_child.asset_id, mobj_child.offset) != (
            ASSET, spec.mobj_child):
        raise RuntimeError(f"{spec.name} MObj head 1 changed: {mobj_child!r}")
    if model.pointer_at(spec.animjoint) is not None:
        raise RuntimeError(f"{spec.name} anim_joints[0] is no longer NULL")
    anim_child = model.pointer_at(spec.animjoint + 4)
    if anim_child is None or (anim_child.asset_id, anim_child.offset) != (
            ASSET, spec.anim_child):
        raise RuntimeError(f"{spec.name} anim_joints[1] changed: {anim_child!r}")

    refs = tuple(sorted(slot for slot, ref in model.internal.items()
                        if ref.offset == spec.root))
    expected_root_ref = spec.dobjdesc + 0x30
    if refs != (expected_root_ref,):
        raise RuntimeError(
            f"{spec.name} root referrer census changed: {refs!r}")
    attr_refs = tuple(sorted(
        (slot, ref.asset_id, ref.offset)
        for slot, ref in attr.external.items() if ref.asset_id == ASSET))
    if attr_refs != EXPECTED_ATTR_REFS:
        raise RuntimeError(f"file 264 -> 159 census changed: {attr_refs!r}")
    if run_census:
        scanned, hits = _census()
        want = tuple((ATTR_ASSET, slot, off)
                     for slot, _, off in EXPECTED_ATTR_REFS)
        if hits != want:
            raise RuntimeError(
                f"whole-image census into asset 159 changed "
                f"({scanned} files): {hits!r}")

    tlut_ref = model.pointer_at(spec.root + TLUT_WORD * 8 + 4)
    vtx_ref = model.pointer_at(spec.root + VTX_WORD * 8 + 4)
    if tlut_ref is None or (tlut_ref.asset_id, tlut_ref.offset) != (
            ASSET, spec.tlut):
        raise RuntimeError(f"{spec.name} TLUT ref changed: {tlut_ref!r}")
    if vtx_ref is None or (vtx_ref.asset_id, vtx_ref.offset) != (
            ASSET, spec.vtx):
        raise RuntimeError(f"{spec.name} vertex ref changed: {vtx_ref!r}")
    if raw[HOOK_WORD] != HOOK:
        raise RuntimeError(f"{spec.name} hook words changed: {raw[HOOK_WORD]!r}")
    entries = struct.unpack_from(">16H", model.payload, spec.tlut)
    if entries != spec.tlut_values:
        raise RuntimeError(f"{spec.name} TLUT changed: {entries!r}")
    if (((raw[LOADTLUT_WORD][1] >> 14) & 0x3FF) + 1) != TLUT_ENTRIES:
        raise RuntimeError(f"{spec.name} LOADTLUT count changed")
    if raw[16] != spec.tile_size:
        raise RuntimeError(f"{spec.name} SETTILESIZE changed: {raw[16]!r}")
    if raw[LOADBLOCK_WORD] != spec.load_block:
        raise RuntimeError(f"{spec.name} LOADBLOCK changed: {raw[19]!r}")
    if raw[TRI_WORD] != (0x06060402, 0x00000602):
        raise RuntimeError(f"{spec.name} triangle word changed: {raw[22]!r}")
    if raw[5] != (0xFC127E24, 0xFFFFF3F9):
        raise RuntimeError(f"{spec.name} SETCOMBINE changed: {raw[5]!r}")
    vtx_w0 = raw[VTX_WORD][0]
    if ((vtx_w0 >> 12) & 0xFF) != 4 or ((vtx_w0 >> 1) & 0x7F) != 4:
        raise RuntimeError(f"{spec.name} G_VTX changed: {vtx_w0:#010x}")
    verts = tuple(sm.decode_vertex(model, spec.vtx + i * 16) for i in range(4))
    if verts != spec.verts:
        raise RuntimeError(f"{spec.name} vertices changed: {verts!r}")
    tris = tuple(sm.decode_triangles(0x06, *raw[TRI_WORD]))
    if tris != TRIS:
        raise RuntimeError(f"{spec.name} triangles changed: {tris!r}")
    return raw, verts


def _header_path(spec: YamabukiLiveItemSpec) -> Path:
    return REPO / f"include/nds/generated/nds_native_item_{spec.slug}.generated.h"


def _packet_path(spec: YamabukiLiveItemSpec) -> Path:
    return REPO / f"src/nds/generated/nds_native_item_{spec.slug}.generated.inc"


def render_header(spec: YamabukiLiveItemSpec, raw) -> str:
    p = f"NDS_NATIVE_ITEM_{spec.macro}"
    guard = f"{p}_GENERATED_H"
    return "\n".join((
        f"/* Saffron City {spec.name} native item constants (generated).",
        f" * Do not hand-edit; regenerate with generate_nds_native_item_{spec.slug}.py. */",
        f"#ifndef {guard}",
        f"#define {guard}",
        "",
        f"#define {p}_ASSET {ASSET}u",
        f"#define {p}_ROOT 0x{spec.root:04x}u",
        f"#define {p}_DL_BYTES {DL_WORDS * 8}u",
        f"#define {p}_TLUT_OFFSET 0x{spec.tlut:04x}u",
        f"#define {p}_TLUT_W0 0x{raw[TLUT_WORD][0]:08x}u",
        f"#define {p}_TLUT_END 0x{spec.tlut + TLUT_ENTRIES * 2:04x}u",
        f"#define {p}_HOOK_W0 0x{HOOK[0]:08x}u",
        f"#define {p}_HOOK_W1 0x{HOOK[1]:08x}u",
        f"#define {p}_VERTEX_OFFSET 0x{spec.vtx:04x}u",
        f"#define {p}_VERTEX_COUNT 4u",
        f"#define {p}_TRIANGLE_COUNT 2u",
        f"#define {p}_CORNER_COUNT 6u",
        "",
        "#endif",
        "",
    ))


def _othermode(lines: list[str], raw, index: int) -> None:
    lines.append(
        f"    ndsRendererRecordOtherMode(stats, 0x{raw[index][0] >> 24:02x}u, "
        f"0x{raw[index][0]:08x}u, 0x{raw[index][1]:08x}u);")


def render_packet(spec: YamabukiLiveItemSpec, raw, verts) -> str:
    stem = spec.name
    macro = f"NDS_NATIVE_ITEM_{spec.macro}"
    lines: list[str] = []
    a = lines.append
    a(f"/* Saffron City {spec.name} native item packet (generated).")
    a(f" * Source: SHA-pinned file 159 root 0x{spec.root:04x} (Gfx[30]).")
    a(" * Word 17 is exactly 0xDE000000 0x0E000000: CURRENT_IMAGE is live")
    a(" * through the typed MObj material; palette, state and geometry bake here.")
    a(f" * Do not hand-edit; regenerate with generate_nds_native_item_{spec.slug}.py. */")
    a(f"#include <nds/generated/nds_native_item_{spec.slug}.generated.h>")
    a("")
    a(f"static const u16 sNdsNativeItem{stem}TriIndices[6] =")
    a("{")
    for tri in TRIS:
        a("    " + " ".join(f"{v}u," for v in tri))
    a("};")
    a("")
    a(f"static const s16 sNdsNativeItem{stem}Verts[20] =")
    a("{")
    for v in verts:
        a(f"    {v[0]}, {v[1]}, {v[2]}, {v[3]}, {v[4]},")
    a("};")
    a("")
    a(f"static const u32 sNdsNativeItem{stem}VertColors[4] =")
    a("{")
    for v in verts:
        a(f"    0x{v[5]:08x}u,")
    a("};")
    a("")
    a(f"static void ndsNativeItem{stem}Setup(")
    a("    NDSRendererStats *stats, const void *tlut)")
    a("{")
    a(f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[1][0]:08x}u) | 0x{raw[1][1]:08x}u;")
    for i in (2, 3, 4):
        _othermode(lines, raw, i)
    a(f"    ndsRendererRecordSetCombine(stats, 0x{raw[5][0]:08x}u, 0x{raw[5][1]:08x}u);")
    a(f"    stats->blend_color = 0x{raw[6][1]:08x}u;")
    for i in (8, 9, 10):
        a(f"    ndsRendererRecordSetTile(stats, 0x{raw[i][0]:08x}u, 0x{raw[i][1]:08x}u);")
    a(f"    ndsRendererRecordSetImage(stats, 0x{raw[11][0]:08x}u,")
    a("        (u32)(uintptr_t)tlut);")
    a(f"    ndsRendererRecordLoadTlut(stats, 0x{raw[13][1]:08x}u);")
    a(f"    ndsRendererRecordTextureState(stats, 0x{raw[15][0]:08x}u, 0x{raw[15][1]:08x}u);")
    a(f"    ndsRendererRecordSetTileSize(stats, 0x{raw[16][0]:08x}u, 0x{raw[16][1]:08x}u);")
    a("}")
    a("")
    a(f"static void ndsNativeItem{stem}LoadBlock(NDSRendererStats *stats)")
    a("{")
    a(f"    ndsRendererRecordLoadBlock(stats, 0x{raw[19][0]:08x}u, 0x{raw[19][1]:08x}u);")
    a("}")
    a("")
    a(f"static void ndsNativeItem{stem}Finish(NDSRendererStats *stats)")
    a("{")
    a(f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[25][0]:08x}u) | 0x{raw[25][1]:08x}u;")
    for i in (26, 27, 28):
        _othermode(lines, raw, i)
    a("}")
    a("")
    a(f"/* census: dl_words=30 verts=4 tris=2 material=CURRENT_IMAGE "
      f"tlut=159:0x{spec.tlut:04x} referrer=159:0x{spec.dobjdesc + 0x30:04x} */")
    return "\n".join(lines) + "\n"


def run(spec: YamabukiLiveItemSpec) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--emit", action="store_true")
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--census", action="store_true")
    args = ap.parse_args()
    if args.census:
        scanned, hits = _census()
        print(f"scanned {scanned} O2R files")
        for file_id, slot, off in hits:
            print(f"  file {file_id} slot 0x{slot:04x} -> 159:0x{off:04x}")
        return 0
    raw, verts = decode(spec)
    packet = render_packet(spec, raw, verts)
    header = render_header(spec, raw)
    outputs = ((_packet_path(spec), packet), (_header_path(spec), header))
    if args.emit:
        for path, body in outputs:
            path.parent.mkdir(parents=True, exist_ok=True)
            if (not path.exists()) or path.read_text() != body:
                path.write_text(body)
        print("emitted " + " and ".join(str(p.relative_to(REPO)) for p, _ in outputs))
    if args.check or not args.emit:
        for path, body in outputs:
            if not path.exists() or path.read_text() != body:
                raise RuntimeError(f"generated artefact stale: {path.relative_to(REPO)}")
        print(spec.check_line)
    return 0

