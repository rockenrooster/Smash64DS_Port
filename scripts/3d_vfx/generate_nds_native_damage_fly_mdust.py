#!/usr/bin/env python3
"""Generate the native DamageFlyMDust source-root contract.

DamageFlyMDust is BattleShip's animated IA16 billboard in EFCommonEffects1.
The executor keeps the live source MObj and its stepped CURRENT_IMAGE; this
generator owns only the immutable source root, quad topology, and seven frame
addresses.  It never edits the decomp tree.
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path


REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / "scripts" / "stages"))

import generate_nds_native_stage as sm  # noqa: E402


OUT = REPO / "src/nds/generated/nds_native_damage_fly_mdust.generated.inc"
OUT_HEADER = REPO / "include/nds/generated/nds_native_damage_fly_mdust.generated.h"

MODEL_FILE = sm.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_effects/EFCommonEffects1",
    "9b15042901498b224302ab4271e341a041101b6753ec958f1305dd4e616c8931",
    83,
)

ASSET = 83
ROOT = 0xCA58
ROOT_BYTES = 14 * 8
VERTEX_OFFSET = 0xCA18
VERTEX_COUNT = 4
DOBJ_DESC = 0xCAC8
DOBJ_DL_POINTER_SLOT = DOBJ_DESC + 4
MOBJ_SUB_HEAD = 0xC978
MOBJ_SUB = 0xC998
MOBJ_SUB_BYTES = 0x78
MOBJ_SUB_FLAGS = 0x0000  # pad00 is 0x0010; the flags field is at +0x30.
MOBJ_FRAME_TABLE = 0xC97C
ANIM_JOINT = 0xCAE0
MAT_ANIM_JOINT = 0xCB40
MAT_ANIM_SCRIPT = 0xCB44
FRAME_OFFSETS = (0xC178, 0xB970, 0xB168, 0xA960, 0xA158, 0x9950, 0x9148)
FRAME_WIDTH = 32
FRAME_HEIGHT = 32
FRAME_BYTES = FRAME_WIDTH * FRAME_HEIGHT * 2  # IA16

ROOT_OPCODES = (
    0xE7, 0xD9, 0xFC, 0xDE, 0xF5, 0xE6, 0xF3,
    0xE7, 0xF5, 0x01, 0x06, 0xE7, 0xD9, 0xDF,
)
EXPECTED_TRIANGLES = ((3, 2, 1), (0, 3, 1))

TEXT_PINS = (
    ("decomp/BattleShip-main/include/reloc_data.us.h", (
        "#define llEFCommonEffects1DamageFlyMDustMObjSub ((intptr_t)0xC978)",
        "#define llEFCommonEffects1DamageFlyMDustDObjDesc ((intptr_t)0xCAC8)",
        "#define llEFCommonEffects1DamageFlyMDustAnimJoint ((intptr_t)0xCAE0)",
        "#define llEFCommonEffects1DamageFlyMDustMatAnimJoint ((intptr_t)0xCB40)",
    )),
    ("decomp/BattleShip-main/decomp/src/ef/efdef.h", (
        "nEFKindDamageFlyMDust,",
        "nEFKindDamageFlyMDustReverse,",
    )),
    ("decomp/BattleShip-main/decomp/src/sys/objtypes.h", (
        "#define MOBJ_FLAG_FRAC              (1 << 4)",
    )),
    ("decomp/BattleShip-main/decomp/src/ef/efmanager.c", (
        "EFDesc dEFManagerDamageFlyMDustEffectDesc =",
        "gcDrawDObjTreeDLLinksForGObj,",
        "&llEFCommonEffects1DamageFlyMDustDObjDesc",
        "&llEFCommonEffects1DamageFlyMDustMObjSub",
        "&llEFCommonEffects1DamageFlyMDustAnimJoint",
        "&llEFCommonEffects1DamageFlyMDustMatAnimJoint",
        "efManagerMakeEffectNoForce(&dEFManagerDamageFlyMDustEffectDesc)",
    )),
    ("decomp/BattleShip-main/decomp/src/relocData/83_EFCommonEffects1.c", (
        "/* DamageFlyMDust sprite frame textures: 7 IA16 32x32 frames",
        "void *dEFCommonEffects1_DamageFlyMDust_MObjSub[7] =",
        "MObjSub dEFCommonEffects1_DamageFlyMDust_MObjSub_real[1] =",
        "/* Vtx: DamageFlyMDust @ 0xCA18 (4 vertices) */",
        "/* DisplayList: DamageFlyMDust @ 0xCA58 (112 bytes) */",
        "{ 1, dEFCommonEffects1_DamageFlyMDust_DisplayList },",
        "u32 dEFCommonEffects1_DamageFlyMDust_MatAnimJoint_0xCB44[] =",
        "aobjEvent32SetValBlock(AOBJ_MATFLAG_TEXID, 5),",
    )),
)


def words_at(payload: bytes, offset: int, count: int) -> tuple[tuple[int, int], ...]:
    return tuple(
        struct.unpack_from(">II", payload, offset + i * 8)
        for i in range(count)
    )


def check_text_pins() -> None:
    for path, tokens in TEXT_PINS:
        text = (REPO / path).read_text(encoding="utf-8", errors="replace")
        for token in tokens:
            if token not in text:
                raise RuntimeError(f"{path} source pin missing {token!r}")


def decode() -> dict[str, object]:
    model = sm.load_o2r(REPO, MODEL_FILE)
    check_text_pins()

    if model.file_id != ASSET:
        raise RuntimeError(f"EFCommonEffects1 file ID {model.file_id} != {ASSET}")
    if len(model.payload) < ROOT + ROOT_BYTES:
        raise RuntimeError("EFCommonEffects1 no longer contains the dust root")
    if struct.unpack_from(">I", model.payload, MOBJ_SUB + 0x30)[0] != MOBJ_SUB_FLAGS:
        raise RuntimeError("DamageFlyMDust default material flags changed")
    if model.pointer_at(DOBJ_DL_POINTER_SLOT) != sm.PointerRef(ASSET, ROOT):
        raise RuntimeError("DamageFlyMDust DObjDLLink no longer owns root 0xCA58")
    if model.pointer_at(MOBJ_SUB_HEAD) != sm.PointerRef(ASSET, 0xCA10):
        raise RuntimeError("DamageFlyMDust MObjSub head moved")
    if model.pointer_at(MOBJ_SUB + 4) != sm.PointerRef(ASSET, MOBJ_FRAME_TABLE):
        raise RuntimeError("DamageFlyMDust frame-table pointer moved")
    if model.pointer_at(0xCA10) != sm.PointerRef(ASSET, MOBJ_SUB):
        raise RuntimeError("DamageFlyMDust MObjSub root moved")

    raw = words_at(model.payload, ROOT, len(ROOT_OPCODES))
    opcodes = tuple(w0 >> 24 for w0, _w1 in raw)
    if opcodes != ROOT_OPCODES:
        raise RuntimeError(f"DamageFlyMDust root opcodes changed: {opcodes!r}")
    if raw[3] != (0xDE000000, 0x0E000000):
        raise RuntimeError("DamageFlyMDust material segment-E seam changed")
    if raw[10][0] >> 24 != 0x06:
        raise RuntimeError("DamageFlyMDust source topology is no longer TRI2")

    vertex_ref = model.pointer_at(ROOT + 9 * 8 + 4)
    if vertex_ref != sm.PointerRef(ASSET, VERTEX_OFFSET):
        raise RuntimeError(f"DamageFlyMDust VTX pointer changed: {vertex_ref!r}")
    vertices = tuple(
        sm.decode_vertex(model, VERTEX_OFFSET + i * 16)
        for i in range(VERTEX_COUNT)
    )
    if any(vertex[5] != 0xFFFFFFFF for vertex in vertices):
        raise RuntimeError("DamageFlyMDust source quad vertex colours changed")

    triangles = sm.decode_triangles(0x06, *raw[10])
    if triangles != EXPECTED_TRIANGLES:
        raise RuntimeError(f"DamageFlyMDust TRI2 topology changed: {triangles!r}")

    for slot, offset in zip(
        range(MOBJ_FRAME_TABLE, MOBJ_FRAME_TABLE + len(FRAME_OFFSETS) * 4, 4),
        FRAME_OFFSETS,
    ):
        if model.pointer_at(slot) != sm.PointerRef(ASSET, offset):
            raise RuntimeError(
                f"DamageFlyMDust frame pointer {slot:#x} moved: "
                f"{model.pointer_at(slot)!r}"
            )
        if offset + FRAME_BYTES > len(model.payload):
            raise RuntimeError(f"DamageFlyMDust frame {offset:#x} is truncated")

    if model.pointer_at(ANIM_JOINT) != sm.PointerRef(ASSET, ANIM_JOINT + 4):
        raise RuntimeError("DamageFlyMDust AnimJoint pointer moved")
    if model.pointer_at(MAT_ANIM_JOINT) != sm.PointerRef(
        ASSET, MAT_ANIM_SCRIPT + 0x6C
    ):
        raise RuntimeError("DamageFlyMDust MatAnimJoint pointer moved")

    return {
        "root_words": raw,
        "vertices": vertices,
        "triangles": triangles,
        # Host O2R payload is big-endian. The runtime swaps each word at load;
        # only its in-memory reader uses byte-index XOR 3 to recover this data.
        "band_masks": tuple(sum(1 << band for band in range(4)
            if any((model.payload[offset + i] >> 6) == band and
                   (model.payload[offset + i + 1] >> 3) != 0
                   for i in range(0, FRAME_BYTES, 2)))
            for offset in FRAME_OFFSETS),
    }


def render_header() -> str:
    return "\n".join([
        "/* DamageFlyMDust source contract (generated from BattleShip).",
        " * Do not hand-edit; regenerate with generate_nds_native_damage_fly_mdust.py. */",
        "#ifndef NDS_NATIVE_DAMAGE_FLY_MDUST_GENERATED_H",
        "#define NDS_NATIVE_DAMAGE_FLY_MDUST_GENERATED_H",
        "",
        f"#define NDS_NATIVE_DAMAGE_FLY_MDUST_ASSET {ASSET}u",
        f"#define NDS_NATIVE_DAMAGE_FLY_MDUST_ROOT 0x{ROOT:04x}u",
        f"#define NDS_NATIVE_DAMAGE_FLY_MDUST_ROOT_BYTES {ROOT_BYTES}u",
        f"#define NDS_NATIVE_DAMAGE_FLY_MDUST_MOBJ_SUB 0x{MOBJ_SUB:04x}u",
        f"#define NDS_NATIVE_DAMAGE_FLY_MDUST_MOBJ_SUB_BYTES 0x{MOBJ_SUB_BYTES:02x}u",
        f"#define NDS_NATIVE_DAMAGE_FLY_MDUST_MOBJ_SUB_FLAGS 0x{MOBJ_SUB_FLAGS:04x}u",
        "/* Default MObj flags emit CURRENT_IMAGE, RENDER_TILE_SIZE, TEXTURE. */",
        "#define NDS_NATIVE_DAMAGE_FLY_MDUST_MATERIAL_EFFECTS 0x1600u",
        f"#define NDS_NATIVE_DAMAGE_FLY_MDUST_FRAME_COUNT {len(FRAME_OFFSETS)}u",
        f"#define NDS_NATIVE_DAMAGE_FLY_MDUST_FRAME_WIDTH {FRAME_WIDTH}u",
        f"#define NDS_NATIVE_DAMAGE_FLY_MDUST_FRAME_HEIGHT {FRAME_HEIGHT}u",
        f"#define NDS_NATIVE_DAMAGE_FLY_MDUST_FRAME_BYTES {FRAME_BYTES}u",
        f"#define NDS_NATIVE_DAMAGE_FLY_MDUST_FRAME_TABLE 0x{MOBJ_FRAME_TABLE:04x}u",
        f"#define NDS_NATIVE_DAMAGE_FLY_MDUST_ANIM_JOINT 0x{ANIM_JOINT:04x}u",
        f"#define NDS_NATIVE_DAMAGE_FLY_MDUST_MAT_ANIM_JOINT 0x{MAT_ANIM_JOINT:04x}u",
        f"#define NDS_NATIVE_DAMAGE_FLY_MDUST_VERTEX_COUNT {VERTEX_COUNT}u",
        f"#define NDS_NATIVE_DAMAGE_FLY_MDUST_TRIANGLE_COUNT {len(EXPECTED_TRIANGLES)}u",
        f"#define NDS_NATIVE_DAMAGE_FLY_MDUST_CORNER_COUNT {len(EXPECTED_TRIANGLES) * 3}u",
        "",
        "#endif",
        "",
    ])


def render(decoded: dict[str, object]) -> str:
    raw = decoded["root_words"]
    vertices = decoded["vertices"]
    triangles = decoded["triangles"]
    assert isinstance(raw, tuple)
    assert isinstance(vertices, tuple)
    assert isinstance(triangles, tuple)

    lines = [
        "/* DamageFlyMDust immutable source geometry and display-list words.",
        " * Seven step-selected current images; no fractional TEXID track. */",
        "#include <nds/generated/nds_native_damage_fly_mdust.generated.h>",
        "",
        "static const u32 sNdsDamageFlyMDustRootWords[NDS_NATIVE_DAMAGE_FLY_MDUST_ROOT_BYTES / 4u] = {",
    ]
    for w0, w1 in raw:
        lines.append(f"    0x{w0:08x}u, 0x{w1:08x}u,")
    lines.extend([
        "};",
        "",
        f"static const u32 sNdsDamageFlyMDustFrameOffsets[{len(FRAME_OFFSETS)}] = {{",
        "    " + ", ".join(f"0x{offset:04x}u" for offset in FRAME_OFFSETS) + ",",
        "};",
        "",
        f"static const s16 sNdsDamageFlyMDustVertices[{VERTEX_COUNT * 5}u] = {{",
    ])
    for vertex in vertices:
        lines.append("    " + ", ".join(str(value) for value in vertex[:5]) + ",")
    lines.extend([
        "};",
        "",
        "static const u8 sNdsDamageFlyMDustBandMasks[7] = {",
        "    " + ", ".join(f"0x{mask:x}u" for mask in decoded["band_masks"]) + ",",
        "};",
        "",
        f"static const u16 sNdsDamageFlyMDustTriIndices[{len(triangles) * 3}u] = {{",
    ])
    for triangle in triangles:
        lines.append("    " + ", ".join(f"{index}u" for index in triangle) + ",")
    lines.extend([
        "};",
        "",
        "/* Source: one segment-E MObj branch, a stepped IA16 image, then TRI2. */",
        "",
    ])
    return "\n".join(lines)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--emit", action="store_true")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    decoded = decode()
    outputs = ((OUT, render(decoded)), (OUT_HEADER, render_header()))
    if args.emit:
        for path, body in outputs:
            path.parent.mkdir(parents=True, exist_ok=True)
            if (not path.exists()) or path.read_text(encoding="utf-8") != body:
                path.write_text(body, encoding="utf-8", newline="\n")
        print("emitted native DamageFlyMDust source contract/header")
    if args.check or not args.emit:
        for path, body in outputs:
            if not path.exists() or path.read_text(encoding="utf-8") != body:
                raise RuntimeError(f"generated artefact stale: {path.relative_to(REPO)}")
        print("DAMAGE_FLY_MDUST_NATIVE_OK asset=83 root=0xca58 verts=4 tris=2 frames=7")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
