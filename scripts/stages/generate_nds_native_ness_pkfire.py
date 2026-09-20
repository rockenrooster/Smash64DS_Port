#!/usr/bin/env python3
"""Generate/check Ness PK Fire's exact two-triangle native weapon packet.

The source ownership is deliberately cross-file.  NessSpecial1 (file 240)
contains the WPAttributes at offset zero, but their `data` pointer resolves to
NessSpecial3 (file 336) root 0x0168.  That root draws two one-triangle batches
and calls segment E slot 0, then slot 1.  The two corresponding MObjSubs have
flags 0x3000 (LIGHT1|LIGHT2) and no texture/palette/color mutation, so those
live light colours remain runtime state while every geometry/state word is AOT.
"""

from __future__ import annotations

import argparse
import hashlib
import struct
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(REPO / "scripts" / "stages"))

import generate_nds_native_stage as sm  # noqa: E402

OUT = REPO / "src/nds/generated/nds_native_ness_pkfire.generated.inc"
OUT_HEADER = REPO / "include/nds/generated/nds_native_ness_pkfire.generated.h"

MODEL_FILE = sm.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/NessSpecial3",
    "5495f90a2d16eebebd8d93af2c42c53cbdc3e5a35d316eae234000ef77fa3071",
    336, 22, 0,
    "ba9cf122f9db7ff477f04ad708a2798a4ebe31bd4d1b124ca0a277aec821ab1b")
ATTR_FILE = sm.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/NessSpecial1",
    "76361159feb5c5dd3a3132e7c781776577948cc5246ab76e84684864b4f659bb",
    240, 0, 5,
    "a6a886be1f9d8481cd6ec7e4801c4abb4a8477bddb511267c11bcf10eb46d605")

ASSET = 336
ATTR_ASSET = 240
ATTR_OFFSET = 0x0000
ROOT = 0x0168
ROOT_WORDS = 15
MOBJSUB_HEADS = 0x00F8
MATERIAL_OFFSETS = (0x0008, 0x0080)
VERTEX_WORDS = (5, 9)
TRI_WORDS = (6, 10)
VERTEX_OFFSETS = (0x0108, 0x0138)
EXPECTED_OPS = (
    0xE7, 0xFC, 0xDE, 0xD7, 0xD9, 0x01, 0x05, 0xE7,
    0xDE, 0x01, 0x05, 0xE7, 0xE7, 0xD9, 0xDF,
)
EXPECTED_ATTR_REFS = (
    (0x0000, ASSET, ROOT),
    (0x0004, ASSET, 0x0000),
    (0x000C, ASSET, 0x01E0),
    (0x0034, ASSET, 0x0A08),
    (0x003C, ASSET, 0x0AF0),
)


def words_at(payload: bytes, off: int, count: int):
    return [struct.unpack_from(">II", payload, off + i * 8) for i in range(count)]


def decode():
    model = sm.load_o2r(REPO, MODEL_FILE)
    attr = sm.load_o2r(REPO, ATTR_FILE)

    refs = tuple(sorted(
        (slot, ref.asset_id, ref.offset) for slot, ref in attr.external.items()))
    if refs != EXPECTED_ATTR_REFS:
        raise RuntimeError(f"NessSpecial1 external refs changed: {refs!r}")
    data_ref = attr.pointer_at(ATTR_OFFSET)
    mobj_ref = attr.pointer_at(ATTR_OFFSET + 4)
    if data_ref is None or (data_ref.asset_id, data_ref.offset) != (ASSET, ROOT):
        raise RuntimeError(f"PK Fire WPAttributes.data changed: {data_ref!r}")
    if mobj_ref is None or (mobj_ref.asset_id, mobj_ref.offset) != (ASSET, 0):
        raise RuntimeError(f"PK Fire p_mobjsubs changed: {mobj_ref!r}")

    # The MObjSub array at +0 points at exactly two live materials, then NULL.
    for index, want in enumerate(MATERIAL_OFFSETS):
        ref = model.pointer_at(MOBJSUB_HEADS + index * 4)
        if ref is None or (ref.asset_id, ref.offset) != (ASSET, want):
            raise RuntimeError(f"PK Fire material {index} changed: {ref!r}")
        flags = struct.unpack_from(">H", model.payload, want + 0x30)[0]
        if flags != 0x3000:
            raise RuntimeError(
                f"PK Fire material {index} flags {flags:#06x} != LIGHT1|LIGHT2")
    if model.pointer_at(MOBJSUB_HEADS + 8) is not None or \
       struct.unpack_from(">I", model.payload, MOBJSUB_HEADS + 8)[0] != 0:
        raise RuntimeError("PK Fire gained a third material")

    raw = words_at(model.payload, ROOT, ROOT_WORDS)
    ops = tuple(w0 >> 24 for w0, _ in raw)
    if ops != EXPECTED_OPS:
        raise RuntimeError(f"PK Fire opcode census changed: {ops!r}")
    if raw[2][1] != 0x0E000000 or raw[8][1] != 0x0E000008:
        raise RuntimeError("PK Fire segment-E slot order changed")

    groups = []
    for index, (vword, tword, voff) in enumerate(
            zip(VERTEX_WORDS, TRI_WORDS, VERTEX_OFFSETS)):
        vref = model.pointer_at(ROOT + vword * 8 + 4)
        if vref is None or (vref.asset_id, vref.offset) != (ASSET, voff):
            raise RuntimeError(f"PK Fire group {index} vertex ref changed: {vref!r}")
        w0 = raw[vword][0]
        count = (w0 >> 12) & 0xFF
        end = (w0 >> 1) & 0x7F
        if (count, end - count) != (3, 0):
            raise RuntimeError(f"PK Fire group {index} vertex-cache shape changed")
        verts = tuple(sm.decode_vertex(model, voff + i * 16) for i in range(3))
        tris = tuple(sm.decode_triangles(raw[tword][0] >> 24,
                                        raw[tword][0], raw[tword][1]))
        if tris != ((2, 1, 0),):
            raise RuntimeError(f"PK Fire group {index} triangle changed: {tris!r}")
        groups.append(verts)
    return raw, tuple(groups)


def render_header() -> str:
    return f"""/* Ness PK Fire native weapon constants (generated).
 * Do not hand-edit; regenerate with generate_nds_native_ness_pkfire.py. */
#ifndef NDS_NATIVE_NESS_PKFIRE_GENERATED_H
#define NDS_NATIVE_NESS_PKFIRE_GENERATED_H

#define NDS_NATIVE_NESS_PKFIRE_ASSET {ASSET}u
#define NDS_NATIVE_NESS_PKFIRE_ROOT 0x{ROOT:04x}u
#define NDS_NATIVE_NESS_PKFIRE_DL_BYTES {ROOT_WORDS * 8}u
#define NDS_NATIVE_NESS_PKFIRE_GROUP_COUNT 2u
#define NDS_NATIVE_NESS_PKFIRE_VERTEX_COUNT 6u
#define NDS_NATIVE_NESS_PKFIRE_TRIANGLE_COUNT 2u
#define NDS_NATIVE_NESS_PKFIRE_MATERIAL_EFFECTS 0x0000000cu

#endif
"""


def render(raw, groups) -> str:
    verts = tuple(v for group in groups for v in group)
    lines = [
        "/* Ness PK Fire native weapon packet (generated).",
        " * Source: SHA-pinned NessSpecial3 file 336 root 0x0168.",
        " * Two one-triangle batches select live segment-E material slots 0/1;",
        " * both materials are LIGHT1|LIGHT2 only, so their animated light",
        " * colours remain live while geometry and immutable RSP/RDP state bake.",
        " * Do not hand-edit; regenerate with generate_nds_native_ness_pkfire.py. */",
        "#include <nds/generated/nds_native_ness_pkfire.generated.h>",
        "",
        "static const s16 sNdsNativeNessPKFireVerts[30] =",
        "{",
    ]
    for i, v in enumerate(verts):
        if i in (0, 3):
            lines.append(f"    /* material {(i // 3)} */")
        lines.append(f"    {v[0]}, {v[1]}, {v[2]}, {v[3]}, {v[4]},")
    lines += ["};", "", "static const u32 sNdsNativeNessPKFireColors[6] =", "{"]
    lines += [f"    0x{v[5]:08x}u," for v in verts]
    lines += [
        "};", "",
        "static void ndsNativeNessPKFireSetup(NDSRendererStats *stats)",
        "{",
        f"    ndsRendererRecordSetCombine(stats, 0x{raw[1][0]:08x}u, 0x{raw[1][1]:08x}u);",
        f"    ndsRendererRecordTextureState(stats, 0x{raw[3][0]:08x}u, 0x{raw[3][1]:08x}u);",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[4][0]:08x}u) | 0x{raw[4][1]:08x}u;",
        "}", "",
        "static void ndsNativeNessPKFireFinish(NDSRendererStats *stats)",
        "{",
        f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[13][0]:08x}u) | 0x{raw[13][1]:08x}u;",
        "}", "",
        "/* census: roots=1 groups=2 tris=2 materials=LIGHT1|LIGHT2",
        " * referrer=240:0x0000 material-heads=336:0x0008,0x0080 */",
        "",
    ]
    return "\n".join(lines)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--emit", action="store_true")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    raw, groups = decode()
    packet = render(raw, groups)
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
        print("NESS_PKFIRE_NATIVE_OK asset=336 root=0x0168 groups=2 tris=2 materials=LIGHT1|LIGHT2")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
