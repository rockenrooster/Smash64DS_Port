#!/usr/bin/env python3
"""Generate/check Peach's Castle's exact native bumper item program.

File 86 (ITCommonObject) root 0x7558 is a Gfx[30] fixed two-triangle quad over
ONE 32x32 CI4 image at 0x7288.  Its only live material state is the MObjSub at
0x7498, whose flags are 0x0004 (MOBJ_FLAG_PALETTE) alone: with SPLIT and ALPHA
both clear, objdisplay.c:1261-1263 emits exactly one gDPSetTextureImage naming
mobj->sub.palettes[palette_id] and nothing else, and the root's OWN G_LOADTLUT
(word 12) loads it.  So the fixed and live halves are exchanged relative to the
Mushroom Kingdom Pakkun owner: Pakkun bakes the palette and takes the image
live, this one bakes the image and takes the palette live.

TWO OBJECTS REACH THIS ROOT.  The whole-game reference census below proves it:
file 251 slot 0x069C (dITCommonData_NBumper_ItemAttributes.data) and slot
0x0CF0 (dITCommonData_GBumper_ItemAttributes.data) BOTH resolve to file 86
0x7648, the one DObjDesc whose +0x30 slot is the only pointer to root 0x7558.
The renderer cannot tell the two kinds apart from asset/root/MObj, so the
adapter must read ITStruct.kind.  This generator pins the census so a third
referrer cannot appear without failing the build.

NO RE-BAKE.  The owner ships no image bytes and no palette bytes: the executor
points SETTIMG at the resident file-86 payload, and the runtime texture cache
keys on (image, tlut_image) -- nds_renderer_textures_effects.c:10603,:10607 --
so the two palettes produce two independent cache entries over one resident
image.  ndsRendererRecordLoadTlut latches texture_tlut_image from the live
texture_image (nds_renderer_dl_core.c:1058), which is why the executor MUST
record the material's palette SETTIMG BEFORE the root's LOADTLUT; emitting the
material afterwards keys both palettes to the same cache entry and the hit
flash silently disappears.
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(REPO / "scripts" / "stages"))

import generate_nds_native_stage as sm  # noqa: E402

OUT = REPO / "src/nds/generated/nds_native_castle_bumper.generated.inc"
OUT_HEADER = REPO / "include/nds/generated/nds_native_castle_bumper.generated.h"

OBJECT_FILE = sm.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/MiscData086",
    "96e987d81b24497ad0c314372edb79f123016b59187b5f94b57b73e4bb122c04",
    86, 402, 0,
    "1c642e60401ca5e6df2fe1b0d6ddeb6e322b0288f4c13a1e608875322fbf9438")
ATTR_FILE = sm.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_items/ITCommonData",
    "8ad38613162d33e712f79f9d5584540dadd3fbddad90287dedf8cfc59cc76f32",
    251, 0, 68,
    "264b5ef35dfe41bd22cc94a82a80034e6913b9d1d827839392a6328ac9a60145")

ROOT = 0x7558
DL_WORDS = 30
VTX = 0x7518
IMAGE = 0x7288
IMAGE_BYTES = 512
PALETTE0 = 0x7260          # mobj->sub.palettes[0], the resting face
PALETTE1 = 0x7238          # mobj->sub.palettes[1], the three-tick hit flash
PALETTE_ENTRIES = 16
PALETTE_ARRAY = 0x7490
MOBJSUB = 0x7498
DOBJDESC = 0x7648
MOBJ_FLAGS = 0x0004        # MOBJ_FLAG_PALETTE (objtypes.h:49), alone

MATERIAL_WORD = 10         # gSPDisplayList(0x0E000000): the live palette SETTIMG
LOADTLUT_WORD = 12         # G_LOADTLUT: latches texture_tlut_image

EXPECTED_OPS = (
    0xE7, 0xE3, 0xE2, 0xE2, 0xFC, 0xF9, 0xE8, 0xF5,
    0xF5, 0xF5, 0xDE, 0xE6, 0xF0, 0xE7, 0xD7, 0xF2,
    0xFD, 0xE6, 0xF3, 0xE7, 0xD9, 0x01, 0x06, 0xE7,
    0xE7, 0xD9, 0xE3, 0xE2, 0xE2, 0xDF,
)
EXPECTED_VERTS = (
    (180, -180, 0, 1024, 0, 0xFFFFFFFF),
    (-180, -180, 0, 0, 0, 0xFFFFFFFF),
    (-180, 180, 0, 0, 1024, 0xFFFFFFFF),
    (180, 180, 0, 1024, 1024, 0xFFFFFFFF),
)
EXPECTED_TRIS = ((3, 2, 1), (0, 3, 1))
# The whole-game census of ITAttributes rows that reach this model.  Both are
# bumpers, and they are why the adapter must read ITStruct.kind.
EXPECTED_ATTR_REFS = ((0x069C, 86, DOBJDESC), (0x0CF0, 86, DOBJDESC))
EXPECTED_MOBJSUB_REFS = ((0x06A0, 86, 0x7488), (0x0CF4, 86, 0x7488))


def words_at(payload: bytes, off: int, count: int):
    return [struct.unpack_from(">II", payload, off + i * 8) for i in range(count)]


def convert_rgba16(n64: int) -> int:
    """nds_renderer_textures_effects.c:8126, preserve_transparent_rgb FALSE."""
    if (n64 & 1) == 0:
        return 0
    return (0x8000 | ((n64 >> 11) & 0x1F) | (((n64 >> 6) & 0x1F) << 5) |
            (((n64 >> 1) & 0x1F) << 10))


def pack_resolved_pal16(pixels):
    """ndsRendererHardwarePackResolvedPal16, nds_renderer_textures_effects.c:8209.

    Returns (table, index_image, color0_transparent), or (None, None, None) if
    the resolved image needs more than sixteen DS colours.
    """
    table: list[int] = []
    for raw in pixels:
        color = raw if (raw & 0x8000) else 0
        if color not in table:
            if len(table) >= 16:
                return None, None, None
            table.append(color)
    transparent = False
    for i, value in enumerate(table):
        if value == 0:
            table[0], table[i] = 0, table[0]
            transparent = True
            break
    index_image = [table.index(raw if (raw & 0x8000) else 0) for raw in pixels]
    return table, index_image, transparent


def resolve_ci4(image: bytes, palette):
    out = []
    for i in range(len(image) * 2):
        packed = image[i >> 1]
        out.append(convert_rgba16(
            palette[(packed >> 4) if (i & 1) == 0 else (packed & 0x0F)]))
    return out


def assert_palette_indices_are_shared(obj):
    """THE ASSERTION THAT THE FIRST ATTEMPT AT THIS OWNER GOT WRONG.

    The owner serves BOTH palettes from ONE resident CI4 image.  That is sound
    only if palette index k names the same texel under both palettes -- i.e. if
    nothing between the source bytes and the DS texture reindexes one of them.
    Three things are pinned, in the order they can break:

    1. The image really spans the full 4-bit index space and both palettes
       really are sixteen distinct entries, so no shortened or deduplicated
       palette can be substituted without moving texels.
    2. The root's own G_LOADTLUT loads exactly PALETTE_ENTRIES entries, so the
       runtime's texture_tlut_count can never be smaller than the highest index
       ndsRendererHardwareCiPaletteEntriesUsed will find (16).
    3. Resolving the image through each palette and running the runtime's OWN
       repacker yields the SAME packed index image for both; only the
       sixteen-entry colour table differs.

    Clause 3 is the load-bearing one, and it is the clause the first attempt
    got wrong: that attempt compared the resolved texel VALUES, which
    necessarily differ -- that difference IS the hit flash -- concluded the two
    palettes could not share one image, and the work was reverted.  Measured
    here: the values differ and the INDICES do not, so one resident image is
    correct.
    """
    image = obj.payload[IMAGE:IMAGE + IMAGE_BYTES]
    nibbles = set()
    for byte in image:
        nibbles.add(byte >> 4)
        nibbles.add(byte & 0x0F)
    if nibbles != set(range(16)):
        raise RuntimeError(
            f"bumper CI4 image no longer spans all 16 indices: {sorted(nibbles)!r}")

    palettes = []
    for offset in (PALETTE0, PALETTE1):
        entries = struct.unpack_from(f">{PALETTE_ENTRIES}H", obj.payload, offset)
        if len(set(entries)) != PALETTE_ENTRIES:
            raise RuntimeError(
                f"bumper palette 0x{offset:04x} entries are no longer distinct")
        palettes.append(entries)
    if palettes[0] == palettes[1]:
        raise RuntimeError("the two bumper palettes are identical: no hit flash")

    packed = []
    for entries in palettes:
        table, index_image, transparent = pack_resolved_pal16(
            resolve_ci4(image, entries))
        if table is None:
            raise RuntimeError("a bumper palette no longer repacks into PAL16")
        if not transparent:
            raise RuntimeError(
                "a bumper palette lost its transparent entry; "
                "GL_TEXTURE_COLOR0_TRANSPARENT would stop applying")
        packed.append((tuple(table), tuple(index_image)))
    if packed[0][1] != packed[1][1]:
        raise RuntimeError(
            "the two bumper palettes no longer share one packed index image: "
            "one resident CI4 image cannot serve both")
    if packed[0][0] == packed[1][0]:
        raise RuntimeError("the two bumper palettes repack to one colour table")


def decode():
    obj = sm.load_o2r(REPO, OBJECT_FILE)
    attr = sm.load_o2r(REPO, ATTR_FILE)

    if len(obj.payload) < ROOT + DL_WORDS * 8:
        raise RuntimeError("file 86 no longer contains the bumper list")
    raw = words_at(obj.payload, ROOT, DL_WORDS)
    ops = tuple(w0 >> 24 for w0, _ in raw)
    if ops != EXPECTED_OPS:
        raise RuntimeError(f"bumper 0x7558 opcode census changed: {ops!r}")

    # ORDER IS THE WHOLE DESIGN: the segment-E material call, the only supplier
    # of the palette SETTIMG, must precede the LOADTLUT that latches
    # texture_tlut_image.  Assert the source order the executor mirrors.
    if raw[MATERIAL_WORD] != (0xDE000000, 0x0E000000):
        raise RuntimeError(f"bumper material hook changed: {raw[MATERIAL_WORD]!r}")
    if (raw[LOADTLUT_WORD][0] >> 24) != 0xF0 or MATERIAL_WORD >= LOADTLUT_WORD:
        raise RuntimeError("bumper LOADTLUT no longer follows the material hook")
    if (((raw[LOADTLUT_WORD][1] >> 14) & 0x3FF) + 1) != PALETTE_ENTRIES:
        raise RuntimeError(
            f"bumper LOADTLUT count changed: {raw[LOADTLUT_WORD][1]:#010x}")
    if raw[22] != (0x06060402, 0x00000602):
        raise RuntimeError(f"bumper triangle word changed: {raw[22]!r}")

    image_ref = obj.pointer_at(ROOT + 16 * 8 + 4)
    vtx_ref = obj.pointer_at(ROOT + 21 * 8 + 4)
    if image_ref is None or (image_ref.asset_id, image_ref.offset) != (86, IMAGE):
        raise RuntimeError(f"bumper image ref changed: {image_ref!r}")
    if vtx_ref is None or (vtx_ref.asset_id, vtx_ref.offset) != (86, VTX):
        raise RuntimeError(f"bumper vertex ref changed: {vtx_ref!r}")

    # Root identity: exactly one pointer to 0x7558 in the whole file, and it is
    # the shared NBumper/GBumper DObjDesc entry 1.
    roots = tuple(sorted(
        slot for slot, ref in obj.internal.items() if ref.offset == ROOT))
    if roots != (DOBJDESC + 0x30,):
        raise RuntimeError(f"root 0x7558 reference census changed: {roots!r}")
    palette_slots = tuple(
        obj.internal[PALETTE_ARRAY + i * 4].offset for i in range(2))
    if palette_slots != (PALETTE0, PALETTE1):
        raise RuntimeError(f"bumper palette array changed: {palette_slots!r}")
    if obj.internal[MOBJSUB + 0x2C].offset != PALETTE_ARRAY:
        raise RuntimeError("MObjSub 0x7498 no longer points at the palette array")

    # BOTH bumper kinds, and only those two, reach this model.
    attr_refs = tuple(sorted(
        (slot, ref.asset_id, ref.offset)
        for slot, ref in attr.external.items()
        if ref.asset_id == 86 and ref.offset == DOBJDESC))
    mobjsub_refs = tuple(sorted(
        (slot, ref.asset_id, ref.offset)
        for slot, ref in attr.external.items()
        if ref.asset_id == 86 and ref.offset == 0x7488))
    if attr_refs != EXPECTED_ATTR_REFS:
        raise RuntimeError(f"bumper DObjDesc referrer census changed: {attr_refs!r}")
    if mobjsub_refs != EXPECTED_MOBJSUB_REFS:
        raise RuntimeError(f"bumper MObjSub referrer census changed: {mobjsub_refs!r}")

    assert_palette_indices_are_shared(obj)

    verts = tuple(sm.decode_vertex(obj, VTX + i * 16) for i in range(4))
    if verts != EXPECTED_VERTS:
        raise RuntimeError(f"bumper vertices changed: {verts!r}")
    tris = tuple(sm.decode_triangles(0x06, *raw[22]))
    if tris != EXPECTED_TRIS:
        raise RuntimeError(f"bumper triangles changed: {tris!r}")
    return raw, verts


def render_header(raw, verts) -> str:
    """The constants only. The adapter admits this object in a DIFFERENT
    translation unit from the executor, so the numbers have to be in a header
    both can include."""
    lines: list[str] = []
    a = lines.append
    a("/* Peach's Castle bumper native item constants (generated).")
    a(" * Do not hand-edit; regenerate with"
      " generate_nds_native_castle_bumper.py. */")
    a("#ifndef NDS_NATIVE_CASTLE_BUMPER_GENERATED_H")
    a("#define NDS_NATIVE_CASTLE_BUMPER_GENERATED_H")
    a("")
    a("#define NDS_NATIVE_CASTLE_BUMPER_ASSET 86u")
    a(f"#define NDS_NATIVE_CASTLE_BUMPER_ROOT 0x{ROOT:04x}u")
    a(f"#define NDS_NATIVE_CASTLE_BUMPER_DL_BYTES {DL_WORDS * 8}u")
    a(f"#define NDS_NATIVE_CASTLE_BUMPER_IMAGE_OFFSET 0x{IMAGE:04x}u")
    a(f"#define NDS_NATIVE_CASTLE_BUMPER_IMAGE_END 0x{IMAGE + IMAGE_BYTES:04x}u")
    a(f"#define NDS_NATIVE_CASTLE_BUMPER_PALETTE0_OFFSET 0x{PALETTE0:04x}u")
    a(f"#define NDS_NATIVE_CASTLE_BUMPER_PALETTE1_OFFSET 0x{PALETTE1:04x}u")
    a(f"#define NDS_NATIVE_CASTLE_BUMPER_PALETTE_BYTES {PALETTE_ENTRIES * 2}u")
    a(f"#define NDS_NATIVE_CASTLE_BUMPER_MOBJ_FLAGS 0x{MOBJ_FLAGS:04x}u")
    a(f"#define NDS_NATIVE_CASTLE_BUMPER_VERTEX_COUNT {len(verts)}u")
    a("#define NDS_NATIVE_CASTLE_BUMPER_TRIANGLE_COUNT "
      f"{len(EXPECTED_TRIS)}u")
    a("#define NDS_NATIVE_CASTLE_BUMPER_CORNER_COUNT "
      f"{len(EXPECTED_TRIS) * 3}u")
    a("")
    a("#endif")
    return "\n".join(lines) + "\n"


def render(raw, verts) -> str:
    lines: list[str] = []
    a = lines.append
    a("/* Peach's Castle bumper native item packet (generated).")
    a(" * Source: SHA-pinned file 86 root 0x7558 + its own 32x32 CI4 image 0x7288.")
    a(" * The live half is the MOBJ_FLAG_PALETTE selection between the two file-86")
    a(" * palettes 0x7260 (rest) and 0x7238 (three-tick hit flash); the executor")
    a(" * MUST record that material BEFORE the LOADTLUT below or both palettes key")
    a(" * to one texture-cache entry.  No image or palette bytes are baked here.")
    a(" * Do not hand-edit; regenerate with"
      " generate_nds_native_castle_bumper.py. */")
    a("#include <nds/generated/nds_native_castle_bumper.generated.h>")
    a("")
    a("static const u16 sNdsNativeCastleBumperTriIndices"
      f"[{len(EXPECTED_TRIS) * 3}] =")
    a("{")
    for tri in EXPECTED_TRIS:
        a("    " + " ".join(f"{v}u," for v in tri))
    a("};")
    a("")
    a(f"static const s16 sNdsNativeCastleBumperVerts[{len(verts) * 5}] =")
    a("{")
    for v in verts:
        a(f"    {v[0]}, {v[1]}, {v[2]}, {v[3]}, {v[4]},")
    a("};")
    a("")
    a(f"static const u32 sNdsNativeCastleBumperVertColors[{len(verts)}] =")
    a("{")
    for v in verts:
        a(f"    0x{v[5]:08x}u,")
    a("};")
    a("")
    a("/* Source words 0/6/11/13/17/19/23/24 are pipe/tile/load syncs and word 29")
    a(" * is ENDDL: they carry no state.  Words 0-9 are everything BEFORE the")
    a(" * segment-E material call at word 10, so they are all this half may emit. */")
    a("static void ndsNativeCastleBumperSetup(NDSRendererStats *stats)")
    a("{")
    for i in (1, 2, 3):
        a(f"    ndsRendererRecordOtherMode(stats, 0x{raw[i][0] >> 24:02x}u,"
          f" 0x{raw[i][0]:08x}u, 0x{raw[i][1]:08x}u);")
    a(f"    ndsRendererRecordSetCombine(stats, 0x{raw[4][0]:08x}u,"
      f" 0x{raw[4][1]:08x}u);")
    a(f"    stats->blend_color = 0x{raw[5][1]:08x}u;")
    for i in (7, 8, 9):
        a(f"    ndsRendererRecordSetTile(stats, 0x{raw[i][0]:08x}u,"
          f" 0x{raw[i][1]:08x}u);")
    a("}")
    a("")
    a("/* Words 12-20, i.e. everything AFTER the segment-E material call.  The")
    a(" * LOADTLUT here is the latch: ndsRendererRecordLoadTlut copies")
    a(" * stats->texture_image into stats->texture_tlut_image, so it must run")
    a(" * with the material's palette SETTIMG still current, and the fixed image")
    a(" * SETTIMG must follow it, never precede it. */")
    a("static void ndsNativeCastleBumperSetupTail(")
    a("    NDSRendererStats *stats, const void *image)")
    a("{")
    a(f"    ndsRendererRecordLoadTlut(stats, 0x{raw[12][1]:08x}u);")
    a(f"    ndsRendererRecordTextureState(stats, 0x{raw[14][0]:08x}u,"
      f" 0x{raw[14][1]:08x}u);")
    a(f"    ndsRendererRecordSetTileSize(stats, 0x{raw[15][0]:08x}u,"
      f" 0x{raw[15][1]:08x}u);")
    a(f"    ndsRendererRecordSetImage(stats, 0x{raw[16][0]:08x}u,"
      "\n        (u32)(uintptr_t)image);")
    a(f"    ndsRendererRecordLoadBlock(stats, 0x{raw[18][0]:08x}u,"
      f" 0x{raw[18][1]:08x}u);")
    a(f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[20][0]:08x}u) |"
      f" 0x{raw[20][1]:08x}u;")
    a("}")
    a("")
    a("static void ndsNativeCastleBumperFinish(NDSRendererStats *stats)")
    a("{")
    a(f"    stats->geometry_mode = (stats->geometry_mode & 0x{raw[25][0]:08x}u) |"
      f" 0x{raw[25][1]:08x}u;")
    for i in (26, 27, 28):
        a(f"    ndsRendererRecordOtherMode(stats, 0x{raw[i][0] >> 24:02x}u,"
          f" 0x{raw[i][0]:08x}u, 0x{raw[i][1]:08x}u);")
    a("}")
    a("")
    a(f"/* census: dl_words={DL_WORDS} verts={len(verts)} tris={len(EXPECTED_TRIS)}"
      f" material_flags=0x{MOBJ_FLAGS:04x} image=86:0x{IMAGE:04x}"
      f" palettes=86:0x{PALETTE0:04x},0x{PALETTE1:04x} referrers=NBumper,GBumper */")
    return "\n".join(lines) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--emit", action="store_true")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    raw, verts = decode()
    text = render(raw, verts)
    header = render_header(raw, verts)
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
        print("CASTLE_BUMPER_NATIVE_OK root=0x7558 verts=4 tris=2 "
              "flags=0x0004 palettes=2 referrers=NBumper,GBumper")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
