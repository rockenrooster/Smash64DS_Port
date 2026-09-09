#!/usr/bin/env python3
"""Generate/check the Pikachu Thunder Jolt EFFECT: one live-material quad.

WHAT THIS OBJECT IS.  It is not either Thunder Jolt weapon.  It is the effect
the jolt spawns beside itself: `efManagerPikachuThunderJoltMakeEffect`
(efmanager.c:4536) creates `dEFManagerThunderJoltEffectDesc` (efmanager.c:670)
at the weapon's own translate and z-rotate, and `wppikachuthunderjolt.c:207`
and `:747` are its two call sites.  That is why the failure record identified
it as a `nGCCommonKindEffect` GObj (identity kind 0x3f3) rather than a Weapon,
in the same asset -- file 342, PikachuSpecial3 -- as both jolts.

THE THIRD ROOT IN FILE 342.  The EFDesc names
`llPikachuSpecial3ThunderJoltDObjDesc` (reloc_data.us.h:4180) at 0x2258, which
holds one non-drawable entry (id 0, no list), one drawable child (id 1) whose
list slot 0x2288 resolves to 342:0x2170, then the id-18 terminator.  A sweep of
file 342's own relocations finds exactly ONE pointer to 0x2170 -- that slot --
and the file has zero external fixups, so asset and root discriminate
completely and no live-kind test is needed beyond the Effect parent.

TWENTY-NINE WORDS, THE AIR JOLT'S SHELL WITH ONE WORD CHANGED.  Root 0x2170 is
opcode-for-opcode the air jolt's 29-word list at 0x0270 except at word 17: the
air jolt bakes a second SETTIMG there naming its own CI4 image, and this list
issues a segment-0xE call instead.  That single word is the whole difference
between the two owner shapes, which is why the two are separate generators
rather than one parameterized emitter -- a shared emitter would have to decide
at runtime which of them it was looking at.

THE LIVE HALF IS THE CURRENT IMAGE.  The DObjDesc carries an MObjSub
(0x20A0, head 0 NULL for the non-drawable entry and head 1 valid for the child)
and a MatAnimJoint (0x2350), so the material animates the texture across the
effect's life; the segment-0xE call is where that image arrives.  Everything
else in the list -- combiner, blender, all three tiles, the 16-entry RGBA5551
TLUT at 0x1C70, texture state, tile size, load block, both geometry modes and
all four other-mode words -- is immutable and bakes here.

WORD ORDER IS NOT NEGOTIABLE.  The source binds the palette (word 11), loads
the TLUT (13), sets texture state (15) and tile size (16), THEN takes the live
image (17), and only then loads the block (19).  So the emitted setup stops at
word 16 and a second function carries word 19, with the caller's
`ndsRendererNativeApplyMaterial` between them.  Folding the load block into
setup would load a block before the image it belongs to was chosen.
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

OUT = REPO / "src/nds/generated/nds_native_pikachu_thunderjolt_effect.generated.inc"
OUT_HEADER = (REPO /
              "include/nds/generated/nds_native_pikachu_thunderjolt_effect.generated.h")

MODEL_FILE = sm.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/PikachuSpecial3",
    "b8a9eb805db18a30c2366fcc97ba031e0bfc668bc5250cf9ea5326e354e51a4b",
    342, 104, 0,
    "e09ecb102e8e963c375d32584e461868db536371be7a85ed1e14ab0ee8b99faa")

TEXT_PINS = (
    ("decomp/BattleShip-main/include/reloc_data.us.h",
     ("#define llPikachuSpecial3ThunderJoltDObjDesc ((intptr_t)0x2258)",
      "#define llPikachuSpecial3ThunderJoltMObjSub ((intptr_t)0x20A0)",
      "#define llPikachuSpecial3ThunderJoltMatAnimJoint ((intptr_t)0x2350)",
      "#define llPikachuSpecial3FileID ((intptr_t)0x156)")),
    ("decomp/BattleShip-main/decomp/src/ef/efmanager.c",
     ("EFDesc dEFManagerThunderJoltEffectDesc =",
      "&llPikachuSpecial3ThunderJoltDObjDesc,               "
      "// DObj Setup attributes offset (?)",
      "GObj* efManagerPikachuThunderJoltMakeEffect(Vec3f *pos, f32 rotate)")),
    ("decomp/BattleShip-main/decomp/src/wp/wppikachu/wppikachuthunderjolt.c",
     ("efManagerPikachuThunderJoltMakeEffect(",)),
)

ASSET = 342
DOBJDESC = 0x2258
DESC_STRIDE = 0x2C
DESC_LIST_SLOT = DOBJDESC + DESC_STRIDE + 4   # 0x2288, the one drawable child
MOBJSUBS = 0x20A0
ANIMJOINT = 0x22E0
MATANIMJOINT = 0x2350

ROOT = 0x2170
DL_WORDS = 29
VTX = 0x2130
TLUT_OFFSET = 0x1C70
TLUT_ENTRIES = 16

TLUT_WORD = 11
LOADTLUT_WORD = 13
SEGMENT_WORD = 17
LOADBLOCK_WORD = 19
VTX_WORD = 21
TRI_WORD = 22

# The air jolt's shell with 0xFD at index 17 replaced by 0xDE.  Pinned rather
# than derived: an opcode drifting here would change which word means what.
OPS = (
    0xE7, 0xD9, 0xE3, 0xE2, 0xE2, 0xFC, 0xF9, 0xE8,
    0xF5, 0xF5, 0xF5, 0xFD, 0xE6, 0xF0, 0xE7, 0xD7,
    0xF2, 0xDE, 0xE6, 0xF3, 0xE7, 0x01, 0x06, 0xE7,
    0xE7, 0xD9, 0xE3, 0xE2, 0xDF,
)
VERTS = (
    (90, 0, 0, 2048, 0, 0xFFFFFFFF),
    (-90, 180, 0, 0, 1024, 0xFFFFFFFF),
    (90, 180, 0, 2048, 1024, 0xFFFFFFFF),
    (-90, 0, 0, 0, 0, 0xFFFFFFFF),
)
TRIS = ((3, 2, 1), (3, 0, 2))
# Pinned by value, not by a rule.  Entry 0 AND entry 15 both have the RGBA5551
# alpha bit clear, so "every entry but the first is opaque" would be false --
# the same trap the air jolt's TLUT sets, and this is byte-identical to it
# despite living 0x1C68 bytes further into the file.
TLUT = (0x003E, 0x003F, 0x18FD, 0x31BD, 0x423D, 0x4A7D, 0x633D, 0x7BFD,
        0x8C7D, 0x9CFD, 0xAD7D, 0xC63D, 0xD6BD, 0xF7BD, 0xFFFF, 0xEF7E)


def words_at(payload: bytes, off: int, count: int):
    return [struct.unpack_from(">II", payload, off + i * 8) for i in range(count)]


def check_text_pins() -> None:
    for path, tokens in TEXT_PINS:
        text = (REPO / path).read_text(encoding="utf-8", errors="replace")
        for token in tokens:
            if token not in text:
                raise RuntimeError(f"{path} pin missing {token!r}")


def census() -> tuple:
    """Every O2R file in the image, for pointers into asset 342."""
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
        res = sm.load_o2r(REPO, sm.InputSpec(rel, hashlib.sha256(blob).hexdigest()))
        for slot, ref in res.external.items():
            if ref.asset_id == ASSET and ref.offset == ROOT:
                hits.append((res.file_id, slot, ref.offset))
    return scanned, tuple(sorted(hits))


def decode(run_census: bool = True):
    model = sm.load_o2r(REPO, MODEL_FILE)
    check_text_pins()

    if len(model.payload) < ROOT + DL_WORDS * 8:
        raise RuntimeError("file 342 no longer contains the effect list")
    raw = words_at(model.payload, ROOT, DL_WORDS)
    ops = tuple(w0 >> 24 for w0, _ in raw)
    if ops != OPS:
        raise RuntimeError(f"effect root 0x2170 opcode census changed: {ops!r}")

    # The DObjDesc: one non-drawable entry, one drawable child, terminator.
    if struct.unpack_from(">i", model.payload, DOBJDESC)[0] != 0:
        raise RuntimeError("effect DObjDesc entry 0 id changed")
    if model.pointer_at(DOBJDESC + 4) is not None:
        raise RuntimeError("effect DObjDesc entry 0 gained a list")
    if struct.unpack_from(">i", model.payload, DOBJDESC + DESC_STRIDE)[0] != 1:
        raise RuntimeError("effect DObjDesc entry 1 id changed")
    child = model.pointer_at(DESC_LIST_SLOT)
    if child is None or (child.asset_id, child.offset) != (ASSET, ROOT):
        raise RuntimeError(f"effect DObjDesc child list changed: {child!r}")
    if struct.unpack_from(">i", model.payload, DOBJDESC + 2 * DESC_STRIDE)[0] != 18:
        raise RuntimeError("effect DObjDesc terminator changed")

    # Two MObjSub heads for two DObjs: NULL for the one that draws nothing and
    # valid for the child.  This non-NULL head is why the failure record read a
    # non-NULL material where the air jolt reads 0.
    if model.pointer_at(MOBJSUBS) is not None:
        raise RuntimeError("effect MObjSub head 0 is no longer NULL")
    if model.pointer_at(MOBJSUBS + 4) is None:
        raise RuntimeError("effect MObjSub head 1 went NULL")

    # Referrer census.  Exactly one pointer in file 342 reaches the root, and it
    # is the DObjDesc child slot; no file in the image reaches it externally.
    internal_roots = tuple(sorted(
        slot for slot, ref in model.internal.items() if ref.offset == ROOT))
    if internal_roots != (DESC_LIST_SLOT,):
        raise RuntimeError(f"referrers to the effect root changed: {internal_roots!r}")
    if model.external:
        raise RuntimeError("file 342 gained an external fixup")
    if run_census:
        scanned, hits = census()
        if hits != ():
            raise RuntimeError(
                f"a file outside 342 now reaches the effect root ({scanned} scanned): {hits!r}")

    # Bindings.  The palette and the vertices bake; the IMAGE does not, which is
    # the entire point of this owner -- assert that word 17 is still the hook.
    tlut_ref = model.pointer_at(ROOT + TLUT_WORD * 8 + 4)
    vtx_ref = model.pointer_at(ROOT + VTX_WORD * 8 + 4)
    for name, ref, want in (("TLUT", tlut_ref, TLUT_OFFSET),
                            ("vertices", vtx_ref, VTX)):
        if ref is None or (ref.asset_id, ref.offset) != (ASSET, want):
            raise RuntimeError(f"effect {name} ref changed: {ref!r}")
    if model.pointer_at(ROOT + SEGMENT_WORD * 8 + 4) is not None:
        raise RuntimeError("the effect's segment-E word gained a relocation")
    if raw[SEGMENT_WORD][1] != 0x0E000000:
        raise RuntimeError(
            f"effect segment-E target changed: {raw[SEGMENT_WORD][1]:#010x}")

    entries = struct.unpack_from(f">{TLUT_ENTRIES}H", model.payload, TLUT_OFFSET)
    if entries != TLUT:
        raise RuntimeError(f"effect TLUT changed: {tuple(hex(e) for e in entries)!r}")
    if (((raw[LOADTLUT_WORD][1] >> 14) & 0x3FF) + 1) != TLUT_ENTRIES:
        raise RuntimeError(
            f"effect LOADTLUT count changed: {raw[LOADTLUT_WORD][1]:#010x}")

    if ((raw[VTX_WORD][0] >> 12) & 0xFF) != len(VERTS):
        raise RuntimeError(f"effect G_VTX count changed: {raw[VTX_WORD][0]:#010x}")
    verts = tuple(sm.decode_vertex(model, VTX + i * 16) for i in range(len(VERTS)))
    if verts != VERTS:
        raise RuntimeError(f"effect vertices changed: {verts!r}")
    tris = tuple(sm.decode_triangles(0x06, *raw[TRI_WORD]))
    if tris != TRIS:
        raise RuntimeError(f"effect triangles changed: {tris!r}")
    return raw, verts


def render_header(raw) -> str:
    lines: list[str] = []
    a = lines.append
    a("/* Pikachu Thunder Jolt effect native constants (generated).")
    a(" * Do not hand-edit; regenerate with"
      " generate_nds_native_pikachu_thunderjolt_effect.py. */")
    a("#ifndef NDS_NATIVE_PIKACHU_THUNDERJOLT_EFFECT_GENERATED_H")
    a("#define NDS_NATIVE_PIKACHU_THUNDERJOLT_EFFECT_GENERATED_H")
    a("")
    a(f"#define NDS_NATIVE_THUNDERJOLTFX_ASSET {ASSET}u")
    a(f"#define NDS_NATIVE_THUNDERJOLTFX_ROOT 0x{ROOT:04x}u")
    a(f"#define NDS_NATIVE_THUNDERJOLTFX_DL_BYTES {DL_WORDS * 8}u")
    a(f"#define NDS_NATIVE_THUNDERJOLTFX_TLUT_OFFSET 0x{TLUT_OFFSET:04x}u")
    a(f"#define NDS_NATIVE_THUNDERJOLTFX_TLUT_W0 0x{raw[TLUT_WORD][0]:08x}u")
    a(f"#define NDS_NATIVE_THUNDERJOLTFX_TLUT_END 0x{TLUT_OFFSET + TLUT_ENTRIES * 2:04x}u")
    a(f"#define NDS_NATIVE_THUNDERJOLTFX_VERTEX_OFFSET 0x{VTX:04x}u")
    a(f"#define NDS_NATIVE_THUNDERJOLTFX_VERTEX_COUNT {len(VERTS)}u")
    a(f"#define NDS_NATIVE_THUNDERJOLTFX_TRIANGLE_COUNT {len(TRIS)}u")
    a(f"#define NDS_NATIVE_THUNDERJOLTFX_CORNER_COUNT {len(TRIS) * 3}u")
    a("")
    a("#endif")
    return "\n".join(lines) + "\n"


def _othermode(a, raw, index):
    a(f"    ndsRendererRecordOtherMode(stats, 0x{raw[index][0] >> 24:02x}u,"
      f" 0x{raw[index][0]:08x}u, 0x{raw[index][1]:08x}u);")


def render(raw, verts) -> str:
    lines: list[str] = []
    a = lines.append
    a("/* Pikachu Thunder Jolt effect native packet (generated).")
    a(" * Source: SHA-pinned file 342 root 0x2170 (Gfx[29]), reached from")
    a(" * llPikachuSpecial3ThunderJoltDObjDesc 0x2258 by the ONE drawable child")
    a(" * of dEFManagerThunderJoltEffectDesc.  Word 17 is a segment-0xE call, so")
    a(" * the IMAGE is live and arrives through the typed material path; the")
    a(" * 16-entry RGBA5551 TLUT at 0x1c70, the vertex pool at 0x2130 and every")
    a(" * other state word are immutable and bake here.")
    a(" * Do not hand-edit; regenerate with"
      " generate_nds_native_pikachu_thunderjolt_effect.py. */")
    a("#include <nds/generated/nds_native_pikachu_thunderjolt_effect.generated.h>")
    a("")
    a(f"static const u16 sNdsNativeThunderJoltFxTriIndices[{len(TRIS) * 3}] =")
    a("{")
    for tri in TRIS:
        a("    " + " ".join(f"{v}u," for v in tri))
    a("};")
    a("")
    a(f"static const s16 sNdsNativeThunderJoltFxVerts[{len(verts) * 5}] =")
    a("{")
    for v in verts:
        a(f"    {v[0]}, {v[1]}, {v[2]}, {v[3]}, {v[4]},")
    a("};")
    a("")
    a(f"static const u32 sNdsNativeThunderJoltFxVertColors[{len(verts)}] =")
    a("{")
    for v in verts:
        a(f"    0x{v[5]:08x}u,")
    a("};")
    a("")
    a("/* Words 0/7/12/14/18/20/23/24 are pipe/tile/load syncs and word 28 is")
    a(" * ENDDL: they carry no state.  This stops at word 16 ON PURPOSE -- word")
    a(" * 17 is the live image and word 19 the load block that follows it, so")
    a(" * the caller runs the material between this and the Block function")
    a(" * below.  Folding them together would load a block before its image was")
    a(" * chosen. */")
    a("static void ndsNativeThunderJoltFxSetup(")
    a("    NDSRendererStats *stats, const void *tlut)")
    a("{")
    a(f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[1][0]:08x}u) |"
      f" 0x{raw[1][1]:08x}u;")
    for i in (2, 3, 4):
        _othermode(a, raw, i)
    a(f"    ndsRendererRecordSetCombine(stats, 0x{raw[5][0]:08x}u,"
      f" 0x{raw[5][1]:08x}u);")
    a(f"    stats->blend_color = 0x{raw[6][1]:08x}u;")
    for i in (8, 9, 10):
        a(f"    ndsRendererRecordSetTile(stats, 0x{raw[i][0]:08x}u,"
          f" 0x{raw[i][1]:08x}u);")
    a(f"    ndsRendererRecordSetImage(stats, 0x{raw[TLUT_WORD][0]:08x}u,"
      "\n        (u32)(uintptr_t)tlut);")
    a(f"    ndsRendererRecordLoadTlut(stats, 0x{raw[LOADTLUT_WORD][1]:08x}u);")
    a(f"    ndsRendererRecordTextureState(stats, 0x{raw[15][0]:08x}u,"
      f" 0x{raw[15][1]:08x}u);")
    a(f"    ndsRendererRecordSetTileSize(stats, 0x{raw[16][0]:08x}u,"
      f" 0x{raw[16][1]:08x}u);")
    a("}")
    a("")
    a("/* Word 19, after the live image has been chosen. */")
    a("static void ndsNativeThunderJoltFxLoadBlock(NDSRendererStats *stats)")
    a("{")
    a(f"    ndsRendererRecordLoadBlock(stats, 0x{raw[LOADBLOCK_WORD][0]:08x}u,"
      f" 0x{raw[LOADBLOCK_WORD][1]:08x}u);")
    a("}")
    a("")
    a("/* Three restores and then ENDDL, exactly as the air jolt ends. */")
    a("static void ndsNativeThunderJoltFxFinish(NDSRendererStats *stats)")
    a("{")
    a(f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[25][0]:08x}u) |"
      f" 0x{raw[25][1]:08x}u;")
    for i in (26, 27):
        _othermode(a, raw, i)
    a("}")
    a("")
    a(f"/* census: dl_words={DL_WORDS} verts={len(verts)} tris={len(TRIS)}"
      f" material=CURRENT_IMAGE tlut={ASSET}:0x{TLUT_OFFSET:04x}"
      f" referrer={ASSET}:0x{DESC_LIST_SLOT:04x} */")
    return "\n".join(lines) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--emit", action="store_true")
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--census", action="store_true",
                    help="print the whole-image referrer sweep and exit")
    args = ap.parse_args()
    if args.census:
        scanned, hits = census()
        print(f"scanned {scanned} O2R files")
        for file_id, slot, off in hits:
            print(f"  file {file_id} slot 0x{slot:04x} -> {ASSET}:0x{off:04x}")
        if not hits:
            print(f"  (no external referrer; the only pointer is {ASSET}:"
                  f"0x{DESC_LIST_SLOT:04x}, internal)")
        return 0
    raw, verts = decode()
    text = render(raw, verts)
    header = render_header(raw)
    if args.emit:
        for path, body in ((OUT, text), (OUT_HEADER, header)):
            path.parent.mkdir(parents=True, exist_ok=True)
            if (not path.exists()) or path.read_text() != body:
                path.write_text(body)
        print(f"emitted {OUT.relative_to(REPO)} and {OUT_HEADER.relative_to(REPO)}")
    if args.check or not args.emit:
        for path, body in ((OUT, text), (OUT_HEADER, header)):
            if not path.exists() or path.read_text() != body:
                raise RuntimeError(f"generated artefact stale: {path.relative_to(REPO)}")
        print("PIKACHU_THUNDERJOLT_EFFECT_NATIVE_OK root=0x2170 verts=4 tris=2 "
              "material=CURRENT_IMAGE tlut=342:0x1c70 referrer=342:0x2288")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
