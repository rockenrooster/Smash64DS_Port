#!/usr/bin/env python3
"""Generate/check Pikachu's GROUND Thunder Jolt: six one-triangle segments.

WHAT THIS OBJECT IS.  `llPikachuSpecial1ThunderJoltGroundWeaponAttributes` is
0x34 (reloc_data.us.h:3726), and that slot's `data` field resolves to file 342
(PikachuSpecial3) DObjDesc 0x1888.  So this is the GROUND Thunder Jolt, sibling
of the AIR variant at root 0x0270 whose attributes sit at 0x0 in the same file;
`nWPKindThunderJoltAir` and `nWPKindThunderJoltGround` are adjacent at
`wp/wpdef.h:52-53`.

SIX LISTS, ONE WEAPON.  DObjDesc 0x1888 holds two non-drawable entries and then
six drawable children, all id 2, whose DObjDLLinks at 0x1828, 0x1838, 0x1848,
0x1858, 0x1868 and 0x1878 resolve to the six roots below.  Each root draws
exactly ONE triangle and carries exactly ONE segment-0xE call, and
`p_mobjsubs` at 342:0x1018 is NULL in heads 0 and 1 -- matching the two
non-drawable DObjs -- with six MObjSub lists for the six children.  It is the
segmented ground spark.

ALL SIX ARE NEEDED, MEASURED.  A reconnaissance witness read a root mask of
0x3f on a Pikachu mirror at 1,200 presents, so every one of the six is walked.
The failure record latched the FOURTH child rather than the first only because
`ndsRendererAdapterSubmitStageDObjTreeDepth` skips a `DOBJ_FLAG_HIDDEN` child
AND its subtree silently while still walking siblings, so the visible subset
varies frame to frame -- which is also why the reject count is not a multiple
of six.

THE LIVE HALF IS THE CURRENT IMAGE, MEASURED.  The same witness read a material
effects word of 0x200 across all six roots, which is
`NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE` alone.  `wppikachuthunder.c:172`
and `:196` write `mobj->texture_id_curr`, once to a fixed 3 and once to
`syUtilsRandIntRange(WPPIKACHUTHUNDER_TEXTURES_NUM - 1)`.  So this is exactly
the Mushroom Kingdom Pakkun contract: the image is chosen live through segment
0xE and everything else -- combiner, prim, env, both tiles, texture state, tile
size, geometry mode -- is immutable and bakes here.

ROOT 0x15C0 IS THE ODD ONE and the generator must not paper over it: it is 20
words, not 19, because it issues TWO G_VTX commands (one vertex into cache slot
0, then two ending at slot 3) before its single triangle.  Its corners are
therefore drawn from two separate source pools.  The emitter flattens each
root's three corners in draw order, so the cache-slot arithmetic is resolved
here once rather than reproduced at runtime.

THE EDGE IS STORED ALPHA, NOT A PALETTE AND NOT A FILTER, MEASURED.  The six
roots carry NO G_LOADTLUT at all: this owner is not CI4.  Their render tile is
`fmt=IA siz=8b masks=maskt=5 cms=cmt=G_TX_CLAMP`, so each of the three live
images is 32x32 IA8 -- 1,024 bytes of FOUR BITS INTENSITY plus FOUR BITS ALPHA
-- and the baked combiner is
`rgb = (PRIMITIVE - ENVIRONMENT) * TEXEL0 + ENVIRONMENT, alpha = TEXEL0_A`
with PRIM white and ENV (0, 89, 255).  Intensity is therefore the COLOUR ramp
between the two endpoints and the alpha nibble is the COVERAGE, and they vary
independently: over the three images 3,072 texels use all sixteen intensity
levels and all sixteen alpha levels, and 2,225 of them (72.4%) sit strictly
between transparent and opaque.

GL_RGB8_A5 IS THE FORMAT, CHOSEN BY MEASURED ERROR.  The sixteen resolved
colours are distinct but the intensity histogram is 58% zero, while the alpha
histogram is spread across every level with its mass in the middle; so coverage
precision buys far more than palette width here.  Composited over black and
over white, against the exact N64 result at the DS's own RGB555 ceiling:

    PAL16 + 1-bit alpha (shipping)   max err 238/255   mean 91.56
    GL_RGB32_A3 (32 colours, 8 a)    max err  18/255   mean  6.21
    GL_RGB8_A5  ( 8 colours, 32 a)   max err   8/255   mean  3.51

A5I3 wins on every metric AND represents all sixteen source coverage levels
exactly -- `(n * 0x11) >> 3` is injective into 0..31 -- so the ramp is carried,
not approximated.  A3I5 would additionally erase the faintest level outright
(round(1 * 7 / 15) == 0).  The eight palette steps hold the prim/env lerp at
the MIDPOINT of the intensity pair each one serves, through the same rounding
the shared RGB5A1 bake uses, so these colours agree with every other BLENDPE
surface in the game.

AND IT COSTS LESS VRAM, WHICH IS NOT THE USUAL DIRECTION.  The resolved image
has SEVENTEEN distinct halfwords -- sixteen lerp colours plus the transparent
one -- so the shipping path's PAL16 repack refuses it and each image uploads as
direct colour, 2,048 bytes.  One byte a texel is 1,024 plus a 16-byte palette,
so all three resident is 3,120 bytes rather than 6,144.
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

OUT = REPO / "src/nds/generated/nds_native_pikachu_thunderground.generated.inc"
OUT_HEADER = REPO / "include/nds/generated/nds_native_pikachu_thunderground.generated.h"

MODEL_FILE = sm.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/PikachuSpecial3",
    "b8a9eb805db18a30c2366fcc97ba031e0bfc668bc5250cf9ea5326e354e51a4b",
    342, 104, 0,
    "e09ecb102e8e963c375d32584e461868db536371be7a85ed1e14ab0ee8b99faa")
ATTR_FILE = sm.InputSpec(
    "decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/PikachuSpecial1",
    "8ebd04da94a0e76960f856921f9202a0c74a1ca266a9e8748cd155263ebf1c41",
    244, 0, 6,
    "698a215ec264eb4585272ca1c876fc786ce5f709d9d25e5a5fa3b6aac0f0ad25")

TEXT_PINS = (
    ("decomp/BattleShip-main/include/reloc_data.us.h",
     ("#define llPikachuSpecial1ThunderJoltGroundWeaponAttributes ((intptr_t)0x34)",)),
    ("decomp/BattleShip-main/decomp/src/wp/wppikachu/wppikachuthunder.c",
     ("DObjGetStruct(weapon_gobj)->mobj->texture_id_curr = 3;",)),
)

ASSET = 342
ATTR_ASSET = 244
ATTR_OFFSET = 0x0034          # llPikachuSpecial1ThunderJoltGroundWeaponAttributes
DOBJDESC = 0x1888
MOBJSUBS = 0x1018
ANIMJOINT = 0x1A20

ROOTS = (0x1490, 0x1528, 0x15C0, 0x1660, 0x16F8, 0x1790)
DLLINKS = (0x1828, 0x1838, 0x1848, 0x1858, 0x1868, 0x1878)
# Root 0x15C0 issues two G_VTX commands, so it is one word longer.
ROOT_WORDS = (19, 19, 20, 19, 19, 19)

# Shared by every root: the state words sit at the same indices in all six, and
# only the two-VTX root shifts its geometry tail by one.
COMBINE_WORD = 1
PRIM_WORD = 2
ENV_WORD = 3
TILE_WORDS = (4, 5)
TEXTURE_WORD = 6
TILESIZE_WORD = 7
SEGMENT_WORD = 8
LOADBLOCK_WORD = 10
GEOM_WORD = 12
GEOM_RESTORE_WORD_OFFSET = -2   # from ENDDL

EXPECTED_ATTR_REFS = (
    (0x0000, ASSET, 0x0270), (0x0008, ASSET, 0x0360),
    (0x0034, ASSET, DOBJDESC), (0x0038, ASSET, MOBJSUBS),
    (0x003C, ASSET, ANIMJOINT), (0x0040, ASSET, 0x1AE0),
)

# --- THE COVERAGE HALF -------------------------------------------------------
# Each MObjSub head resolves to an 8-byte list node whose first word is the
# 120-byte MObjSub itself.  That record carries the fmt/siz the segment-0xE hook
# issues as G_SETTIMG and a THREE-entry sprite table; all six tables name the
# same three images, so `texture_id_curr` selects one of three, not one of four.
# (WPPIKACHUTHUNDER_TEXTURES_NUM is the Thunder HEAD's count, a different
# weapon -- do not size this table from it.)
MOBJSUB_STRIDE = 0x78
SPRITES_PER_MOBJ = 3
IMAGE_WIDTH = 32
IMAGE_HEIGHT = 32
IMAGE_BYTES = IMAGE_WIDTH * IMAGE_HEIGHT        # IA8 is one byte a texel
EXPECTED_IMAGES = (0x0408, 0x0810, 0x0C18)
# (MObjSub index, sprite slot) pairs whose source pointer is genuinely NULL.
NULL_SPRITE_SLOTS = ((2, 1),)
EXPECTED_MOBJ_FLAGS = 0x0001                    # -> CURRENT_IMAGE and nothing else
# The render tile the roots bake, and the endpoints its combiner lerps between.
TILE_FMT_IA = 3
TILE_SIZ_8B = 1
TILE_CLAMP = 2                                  # G_TX_CLAMP
TILE_MASK = 5                                   # 1 << 5 == 32
PRIM_COLOR = 0xFFFFFFFF
ENV_COLOR = 0x0059FFFF
# G_SETCOMBINE mux selectors for (PRIM - ENV) * TEXEL0 + ENV / (0,0,0,TEXEL0).
CCMUX_TEXEL0 = 1
CCMUX_PRIMITIVE = 3
CCMUX_ENVIRONMENT = 5
ACMUX_TEXEL0 = 1
ACMUX_ZERO = 7
# GL_RGB8_A5: three index bits and five alpha bits, one byte a texel.
COVERAGE_PALETTE_ENTRIES = 8
COVERAGE_ALPHA_BITS = 5
SOURCE_COVERAGE_LEVELS = 16                     # the IA8 alpha nibble
# Bumped whenever the encoding below changes meaning.  The runtime keeps it in
# its converted-texture key so a hard-alpha upload can never satisfy a
# soft-alpha request after a rebuild that changed only this file.
COVERAGE_CLASS = 0x0A501001


def check_text_pins() -> None:
    for path, tokens in TEXT_PINS:
        text = (REPO / path).read_text(encoding="utf-8", errors="replace")
        for token in tokens:
            if token not in text:
                raise RuntimeError(f"{path} pin missing {token!r}")


def words_at(payload: bytes, off: int, count: int):
    return [struct.unpack_from(">II", payload, off + i * 8) for i in range(count)]


def decode_tile(w0: int, w1: int) -> dict:
    return {
        "fmt": (w0 >> 21) & 0x7, "siz": (w0 >> 19) & 0x3,
        "line": (w0 >> 9) & 0x1FF, "tmem": w0 & 0x1FF,
        "tile": (w1 >> 24) & 0x7, "palette": (w1 >> 20) & 0xF,
        "cmt": (w1 >> 18) & 0x3, "maskt": (w1 >> 14) & 0xF,
        "cms": (w1 >> 8) & 0x3, "masks": (w1 >> 4) & 0xF,
    }


def decode_combine(w0: int, w1: int) -> dict:
    """First-cycle muxes only; both cycles of this combine are identical."""
    return {
        "a": (w0 >> 20) & 0xF, "b": (w1 >> 28) & 0xF,
        "c": (w0 >> 15) & 0x1F, "d": (w1 >> 15) & 0x7,
        "Aa": (w0 >> 12) & 0x7, "Ab": (w1 >> 12) & 0x7,
        "Ac": (w0 >> 9) & 0x7, "Ad": (w1 >> 9) & 0x7,
    }


def census() -> tuple:
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
            if ref.asset_id == ASSET:
                hits.append((res.file_id, slot, ref.offset))
    return scanned, tuple(sorted(hits))


def blend_prim_env_texel0(weight: int) -> tuple[int, int, int]:
    """The runtime's ndsRendererHardwareBlendPrimEnvTexel0, exactly.

    `weight` is the 5-bit TEXEL0 lane the IA/I converters write, and the result
    is the RGB555 triple the shared bake already produces for every other
    BLENDPE surface.  Reproduced here rather than approximated so the dedicated
    A5I3 palette and the generic path cannot drift apart.
    """
    out = []
    for shift in (27, 19, 11):
        prim = (PRIM_COLOR >> shift) & 0x1F
        env = (ENV_COLOR >> shift) & 0x1F
        out.append(((env * (31 - weight)) + (prim * weight) + 15) // 31)
    return tuple(out)


def coverage_tables() -> tuple:
    """(palette, alpha5 LUT, index3 LUT) for the GL_RGB8_A5 encoding.

    alpha5 is `(n * 0x11) >> 3`, which is round(n * 31 / 15) at all sixteen
    inputs and injective, so the source's coverage survives intact.  index3
    rounds the intensity nibble into eight bins and each palette entry holds the
    lerp at its bin's MIDPOINT, which halves the worst colour error against
    taking the nearer endpoint (8.23/255 versus 16.00/255, measured).
    """
    alpha5 = tuple(((n * 0x11) >> 3) for n in range(SOURCE_COVERAGE_LEVELS))
    index3 = tuple(((n * 7) + 7) // 15 for n in range(SOURCE_COVERAGE_LEVELS))
    palette = tuple(
        blend_prim_env_texel0((((4 * j) + 1) * 0x11) >> 4)
        for j in range(COVERAGE_PALETTE_ENTRIES))
    if sorted(set(alpha5)) != list(alpha5):
        raise RuntimeError("alpha5 LUT is no longer a strictly rising ramp")
    if (index3[0] != 0) or (index3[-1] != COVERAGE_PALETTE_ENTRIES - 1):
        raise RuntimeError("index3 LUT no longer spans the palette")
    if any(index3[n] > index3[n + 1] for n in range(len(index3) - 1)):
        raise RuntimeError("index3 LUT is no longer monotone")
    return palette, alpha5, index3


def decode_coverage(model) -> dict:
    """Pin the live image set and prove the source edge is graded alpha."""
    subs = []
    for head in range(2, 8):
        node = model.pointer_at(MOBJSUBS + head * 4)
        if node is None:
            raise RuntimeError(f"ground jolt MObjSub head {head} went NULL")
        sub = model.pointer_at(node.offset)
        if sub is None:
            raise RuntimeError(
                f"ground jolt MObjSub node 0x{node.offset:04x} lost its record")
        subs.append(sub.offset)
    for index in range(1, len(subs)):
        if subs[index] - subs[index - 1] != MOBJSUB_STRIDE:
            raise RuntimeError(f"ground jolt MObjSub stride changed: {subs!r}")

    images: list[int] = []
    for index, off in enumerate(subs):
        fmt = model.payload[off + 2]
        siz = model.payload[off + 3]
        flags = struct.unpack_from(">H", model.payload, off + 0x30)[0]
        prim = struct.unpack_from(">I", model.payload, off + 0x50)[0]
        env = struct.unpack_from(">I", model.payload, off + 0x58)[0]
        if (fmt, siz) != (TILE_FMT_IA, 2):
            raise RuntimeError(
                f"ground jolt MObjSub {index} SETTIMG fmt/siz changed: {fmt},{siz}")
        if flags != EXPECTED_MOBJ_FLAGS:
            raise RuntimeError(
                f"ground jolt MObjSub {index} flags changed: 0x{flags:04x}")
        if (prim, env) != (PRIM_COLOR, ENV_COLOR):
            raise RuntimeError(
                f"ground jolt MObjSub {index} endpoints changed: "
                f"0x{prim:08x}/0x{env:08x}")
        sprites = model.pointer_at(off + 4)
        if sprites is None:
            raise RuntimeError(f"ground jolt MObjSub {index} lost its sprite table")
        for slot in range(SPRITES_PER_MOBJ):
            word = struct.unpack_from(">I", model.payload, sprites.offset + slot * 4)[0]
            ref = model.pointer_at(sprites.offset + slot * 4)
            if ref is None:
                # A REAL NULL IN REAL SOURCE DATA, not a decode failure: the O2R
                # relocation chain steps straight over 0x1054 and the word there
                # is zero.  So one (child, texture_id) pair genuinely selects no
                # image, and the runtime has to decline that combination rather
                # than convert whatever address zero points at.  Pinned by
                # position so a different hole is still caught.
                if word != 0 or (index, slot) not in NULL_SPRITE_SLOTS:
                    raise RuntimeError(
                        f"ground jolt MObjSub {index} sprite {slot} changed: "
                        f"0x{word:08x}")
                continue
            if ref.asset_id != ASSET:
                raise RuntimeError(
                    f"ground jolt MObjSub {index} sprite {slot} left the asset: {ref!r}")
            images.append(ref.offset)

    distinct = tuple(sorted(set(images)))
    if distinct != EXPECTED_IMAGES:
        raise RuntimeError(f"ground jolt live image set changed: {distinct!r}")

    intensity = [0] * SOURCE_COVERAGE_LEVELS
    alpha = [0] * SOURCE_COVERAGE_LEVELS
    for image in distinct:
        end = image + IMAGE_BYTES
        if end > len(model.payload):
            raise RuntimeError(f"ground jolt image 0x{image:04x} runs past the file")
        for value in model.payload[image:end]:
            intensity[value >> 4] += 1
            alpha[value & 0x0F] += 1
    partial = sum(alpha[1:-1])
    total = len(distinct) * IMAGE_BYTES
    # THE FALSIFIER FOR THIS WHOLE ROW.  If the alpha nibble ever collapses to
    # two levels the source edge really is binary and the dedicated soft-alpha
    # owner is dead weight; refuse to emit rather than keep converting a ramp
    # that no longer exists.
    if min(alpha) == 0 or min(intensity) == 0:
        raise RuntimeError(
            f"ground jolt images no longer use every level: "
            f"alpha={alpha!r} intensity={intensity!r}")
    if partial * 2 < total:
        raise RuntimeError(
            f"ground jolt coverage is no longer mostly partial: {partial}/{total}")
    return {
        "subs": tuple(subs),
        "images": distinct,
        "intensity": tuple(intensity),
        "alpha": tuple(alpha),
        "partial": partial,
        "total": total,
    }


def decode_root(model, root: int, expect_words: int):
    """Return (raw words, three corner vertices in DRAW order).

    The corners are flattened out of the source's vertex cache here: a root may
    load its three corners with one G_VTX or with two, and the triangle indexes
    cache SLOTS, not pool positions.
    """
    raw = words_at(model.payload, root, expect_words)
    if (raw[-1][0] >> 24) != 0xDF:
        raise RuntimeError(f"root {root:#06x} does not end in G_ENDDL")
    if (raw[SEGMENT_WORD][0] >> 24) != 0xDE:
        raise RuntimeError(f"root {root:#06x} lost its segment-E material call")
    if raw[SEGMENT_WORD][1] != 0x0E000000:
        raise RuntimeError(
            f"root {root:#06x} segment-E target changed: {raw[SEGMENT_WORD][1]:#010x}")

    slots: dict[int, tuple] = {}
    tri = None
    for index, (w0, w1) in enumerate(raw):
        op = w0 >> 24
        if op == 0x01:
            count = (w0 >> 12) & 0xFF
            end = (w0 >> 1) & 0x7F
            ref = model.pointer_at(root + index * 8 + 4)
            if ref is None or ref.asset_id != ASSET:
                raise RuntimeError(f"root {root:#06x} vertex ref changed: {ref!r}")
            first = end - count
            for k in range(count):
                slots[first + k] = sm.decode_vertex(model, ref.offset + k * 16)
        elif op == 0x05:
            got = tuple(sm.decode_triangles(0x05, w0, w1))
            if len(got) != 1:
                raise RuntimeError(f"root {root:#06x} is no longer one triangle")
            tri = got[0]
        elif op == 0x06:
            raise RuntimeError(f"root {root:#06x} gained a G_TRI2")
    if tri is None:
        raise RuntimeError(f"root {root:#06x} has no triangle")
    corners = []
    for slot in tri:
        if slot not in slots:
            raise RuntimeError(
                f"root {root:#06x} triangle uses unloaded cache slot {slot}")
        corners.append(slots[slot])
    return raw, tuple(corners)


def decode(run_census: bool = True):
    model = sm.load_o2r(REPO, MODEL_FILE)
    attr = sm.load_o2r(REPO, ATTR_FILE)
    check_text_pins()

    # WPAttributes at PikachuSpecial1 + 0x34.  p_mobjsubs is NOT null here --
    # unlike the air jolt -- which is exactly why this owner takes a live
    # material and the air one does not.
    data_ref = attr.pointer_at(ATTR_OFFSET)
    if data_ref is None or (data_ref.asset_id, data_ref.offset) != (ASSET, DOBJDESC):
        raise RuntimeError(f"ground jolt WPAttributes.data changed: {data_ref!r}")
    mobj_ref = attr.pointer_at(ATTR_OFFSET + 4)
    if mobj_ref is None or (mobj_ref.asset_id, mobj_ref.offset) != (ASSET, MOBJSUBS):
        raise RuntimeError(f"ground jolt p_mobjsubs changed: {mobj_ref!r}")
    attr_refs = tuple(sorted(
        (slot, ref.asset_id, ref.offset)
        for slot, ref in attr.external.items() if ref.asset_id == ASSET))
    if attr_refs != EXPECTED_ATTR_REFS:
        raise RuntimeError(f"file 244 -> 342 census changed: {attr_refs!r}")

    # The DObjDesc: two non-drawable entries, six drawable children all id 2,
    # then the id-18 terminator.
    for index in (0, 1):
        entry = DOBJDESC + index * 0x2C
        if struct.unpack_from(">i", model.payload, entry)[0] != index:
            raise RuntimeError(f"ground jolt DObjDesc entry {index} id changed")
        if model.pointer_at(entry + 4) is not None:
            raise RuntimeError(f"ground jolt DObjDesc entry {index} gained a list")
    for child, (link, root) in enumerate(zip(DLLINKS, ROOTS)):
        entry = DOBJDESC + (child + 2) * 0x2C
        if struct.unpack_from(">i", model.payload, entry)[0] != 2:
            raise RuntimeError(f"ground jolt child {child} is no longer id 2")
        ref = model.pointer_at(entry + 4)
        if ref is None or (ref.asset_id, ref.offset) != (ASSET, link):
            raise RuntimeError(f"ground jolt child {child} dl link changed: {ref!r}")
        ref = model.pointer_at(link + 4)
        if ref is None or (ref.asset_id, ref.offset) != (ASSET, root):
            raise RuntimeError(f"ground jolt child {child} root changed: {ref!r}")
    if struct.unpack_from(">i", model.payload, DOBJDESC + 8 * 0x2C)[0] != 18:
        raise RuntimeError("ground jolt DObjDesc terminator changed")

    # Six MObjSub heads for six drawable children; heads 0 and 1 are NULL
    # because the first two DObjs draw nothing.
    for head in (0, 1):
        if model.pointer_at(MOBJSUBS + head * 4) is not None:
            raise RuntimeError(f"ground jolt MObjSub head {head} is no longer NULL")
    for head in range(2, 8):
        if model.pointer_at(MOBJSUBS + head * 4) is None:
            raise RuntimeError(f"ground jolt MObjSub head {head} went NULL")

    if run_census:
        scanned, hits = census()
        want = tuple((ATTR_ASSET, slot, off) for slot, _, off in EXPECTED_ATTR_REFS)
        if hits != want:
            raise RuntimeError(
                f"whole-image census into asset {ASSET} changed ({scanned} files): {hits!r}")

    decoded = []
    for root, expect in zip(ROOTS, ROOT_WORDS):
        raw, corners = decode_root(model, root, expect)
        decoded.append((root, raw, corners))

    # Every root shares one immutable material, so a divergence would silently
    # give one segment a different look; pin it rather than emit six copies of
    # a value nobody compared.
    first = decoded[0][1]
    for root, raw, _ in decoded[1:]:
        for name, index in (("combine", COMBINE_WORD), ("prim", PRIM_WORD),
                            ("env", ENV_WORD), ("texture", TEXTURE_WORD),
                            ("tilesize", TILESIZE_WORD),
                            ("tile0", TILE_WORDS[0]), ("tile1", TILE_WORDS[1]),
                            ("loadblock", LOADBLOCK_WORD), ("geom", GEOM_WORD)):
            if raw[index] != first[index]:
                raise RuntimeError(
                    f"root {root:#06x} {name} word diverges from 0x1490: "
                    f"{raw[index]!r} != {first[index]!r}")

    # THE MATERIAL CONTRACT THE DEDICATED COVERAGE OWNER DEPENDS ON.  None of
    # this is inferred from the format name: the render tile says IA8 32x32
    # clamped, the combiner says the alpha mux is TEXEL0 alone, and no root
    # issues a G_LOADTLUT, so there is no palette for a "CI4 means one bit is
    # enough" reading to hang on.
    if any((raw[index][0] >> 24) == 0xF0 for _, raw, _ in decoded
           for index in range(len(raw))):
        raise RuntimeError("ground jolt gained a G_LOADTLUT; it is no longer IA")
    render_tile = decode_tile(*first[TILE_WORDS[1]])
    if (render_tile["fmt"], render_tile["siz"]) != (TILE_FMT_IA, TILE_SIZ_8B):
        raise RuntimeError(f"ground jolt render tile is no longer IA8: {render_tile!r}")
    if (render_tile["cms"], render_tile["cmt"]) != (TILE_CLAMP, TILE_CLAMP):
        raise RuntimeError(f"ground jolt render tile lost G_TX_CLAMP: {render_tile!r}")
    if (render_tile["masks"], render_tile["maskt"]) != (TILE_MASK, TILE_MASK):
        raise RuntimeError(f"ground jolt render tile is no longer 32x32: {render_tile!r}")
    if first[PRIM_WORD][1] != PRIM_COLOR or first[ENV_WORD][1] != ENV_COLOR:
        raise RuntimeError(
            f"ground jolt endpoints changed: 0x{first[PRIM_WORD][1]:08x}/"
            f"0x{first[ENV_WORD][1]:08x}")
    combine = decode_combine(*first[COMBINE_WORD])
    if ((combine["a"], combine["b"], combine["c"], combine["d"]) !=
            (CCMUX_PRIMITIVE, CCMUX_ENVIRONMENT, CCMUX_TEXEL0, CCMUX_ENVIRONMENT)):
        raise RuntimeError(f"ground jolt colour mux is no longer BLENDPE: {combine!r}")
    if ((combine["Aa"], combine["Ab"], combine["Ac"], combine["Ad"]) !=
            (ACMUX_ZERO, ACMUX_ZERO, ACMUX_ZERO, ACMUX_TEXEL0)):
        raise RuntimeError(
            f"ground jolt alpha mux no longer passes TEXEL0 coverage: {combine!r}")

    coverage = decode_coverage(model)
    return decoded, coverage


def render_header(decoded, coverage) -> str:
    lines: list[str] = []
    a = lines.append
    a("/* Pikachu ground Thunder Jolt native weapon constants (generated).")
    a(" * Do not hand-edit; regenerate with"
      " generate_nds_native_pikachu_thunderground.py. */")
    a("#ifndef NDS_NATIVE_PIKACHU_THUNDERGROUND_GENERATED_H")
    a("#define NDS_NATIVE_PIKACHU_THUNDERGROUND_GENERATED_H")
    a("")
    a(f"#define NDS_NATIVE_THUNDERGROUND_ASSET {ASSET}u")
    a(f"#define NDS_NATIVE_THUNDERGROUND_ROOT_COUNT {len(ROOTS)}u")
    a("#define NDS_NATIVE_THUNDERGROUND_CORNER_COUNT 3u")
    for index, (root, raw, _) in enumerate(decoded):
        a(f"#define NDS_NATIVE_THUNDERGROUND_ROOT{index} 0x{root:04x}u")
        a(f"#define NDS_NATIVE_THUNDERGROUND_ROOT{index}_BYTES {len(raw) * 8}u")
    a("")
    a("/* The live half: three 32x32 IA8 images selected through the MObj's")
    a(" * sprite table.  Four bits of intensity pick the colour, four bits of")
    a(" * alpha ARE the coverage, and the runtime uploads them as GL_RGB8_A5 so")
    a(" * that second nibble survives.  Counts below are over all three images. */")
    a(f"#define NDS_NATIVE_THUNDERGROUND_IMAGE_COUNT {len(coverage['images'])}u")
    for index, image in enumerate(coverage["images"]):
        a(f"#define NDS_NATIVE_THUNDERGROUND_IMAGE{index}_OFFSET 0x{image:04x}u")
    a(f"#define NDS_NATIVE_THUNDERGROUND_IMAGE_WIDTH {IMAGE_WIDTH}u")
    a(f"#define NDS_NATIVE_THUNDERGROUND_IMAGE_HEIGHT {IMAGE_HEIGHT}u")
    a(f"#define NDS_NATIVE_THUNDERGROUND_IMAGE_BYTES {IMAGE_BYTES}u")
    a(f"#define NDS_NATIVE_THUNDERGROUND_TILE_FMT {TILE_FMT_IA}u")
    a(f"#define NDS_NATIVE_THUNDERGROUND_TILE_SIZ {TILE_SIZ_8B}u")
    a(f"#define NDS_NATIVE_THUNDERGROUND_TILE_MASK {TILE_MASK}u")
    a(f"#define NDS_NATIVE_THUNDERGROUND_TILE_CLAMP {TILE_CLAMP}u")
    a(f"#define NDS_NATIVE_THUNDERGROUND_PRIM 0x{PRIM_COLOR:08x}u")
    a(f"#define NDS_NATIVE_THUNDERGROUND_ENV 0x{ENV_COLOR:08x}u")
    a("#define NDS_NATIVE_THUNDERGROUND_COVERAGE_PALETTE_ENTRIES "
      f"{COVERAGE_PALETTE_ENTRIES}u")
    a(f"#define NDS_NATIVE_THUNDERGROUND_COVERAGE_LEVELS {SOURCE_COVERAGE_LEVELS}u")
    a(f"#define NDS_NATIVE_THUNDERGROUND_COVERAGE_CLASS 0x{COVERAGE_CLASS:08x}u")
    a("/* One byte a texel, so the upload is the texel count. */")
    a(f"#define NDS_NATIVE_THUNDERGROUND_COVERAGE_UPLOAD_BYTES {IMAGE_BYTES}u")
    a("/* Texels strictly between transparent and opaque, and the population")
    a(" * they were counted over.  A 1-bit upload forces every one of these to")
    a(" * full opacity, which is the defect this owner exists to remove. */")
    a("#define NDS_NATIVE_THUNDERGROUND_COVERAGE_PARTIAL_TEXELS "
      f"{coverage['partial']}u")
    a(f"#define NDS_NATIVE_THUNDERGROUND_COVERAGE_TOTAL_TEXELS {coverage['total']}u")
    a("")
    a("#endif")
    return "\n".join(lines) + "\n"


def render(decoded, coverage) -> str:
    first = decoded[0][1]
    lines: list[str] = []
    a = lines.append
    a("/* Pikachu ground Thunder Jolt native weapon packet (generated).")
    a(" * Six one-triangle segments from SHA-pinned file 342, reached from")
    a(" * PikachuSpecial1's WPAttributes at 0x34.  Each root carries one")
    a(" * segment-0xE call and the measured live material is CURRENT_IMAGE only")
    a(" * (mobj->texture_id_curr), so the image is chosen at runtime and every")
    a(" * other word here is immutable.  Corners are flattened out of the")
    a(" * source vertex cache in DRAW order, which is why root 0x15c0's two")
    a(" * G_VTX loads need no special case at runtime.")
    a(" * Do not hand-edit; regenerate with"
      " generate_nds_native_pikachu_thunderground.py. */")
    a("#include <nds/generated/nds_native_pikachu_thunderground.generated.h>")
    a("")
    a(f"static const s16 sNdsNativeThunderGroundVerts[{len(decoded) * 3 * 5}] =")
    a("{")
    for root, _, corners in decoded:
        a(f"    /* 0x{root:04x} */")
        for v in corners:
            a(f"    {v[0]}, {v[1]}, {v[2]}, {v[3]}, {v[4]},")
    a("};")
    a("")
    a(f"static const u32 sNdsNativeThunderGroundVertColors[{len(decoded) * 3}] =")
    a("{")
    for _, _, corners in decoded:
        for v in corners:
            a(f"    0x{v[5]:08x}u,")
    a("};")
    a("")
    a("/* One immutable material for all six segments; the generator refuses to")
    a(" * emit if any root's combine, prim, env, tile, texture, tile-size,")
    a(" * load-block or geometry word diverges from the first. */")
    a("static void ndsNativeThunderGroundSetup(NDSRendererStats *stats)")
    a("{")
    a(f"    ndsRendererRecordSetCombine(stats, 0x{first[COMBINE_WORD][0]:08x}u,"
      f" 0x{first[COMBINE_WORD][1]:08x}u);")
    a(f"    stats->prim_color = 0x{first[PRIM_WORD][1]:08x}u;")
    a(f"    stats->env_color = 0x{first[ENV_WORD][1]:08x}u;")
    for index in TILE_WORDS:
        a(f"    ndsRendererRecordSetTile(stats, 0x{first[index][0]:08x}u,"
          f" 0x{first[index][1]:08x}u);")
    a(f"    ndsRendererRecordTextureState(stats, 0x{first[TEXTURE_WORD][0]:08x}u,"
      f" 0x{first[TEXTURE_WORD][1]:08x}u);")
    a(f"    ndsRendererRecordSetTileSize(stats, 0x{first[TILESIZE_WORD][0]:08x}u,"
      f" 0x{first[TILESIZE_WORD][1]:08x}u);")
    a(f"    ndsRendererRecordLoadBlock(stats, 0x{first[LOADBLOCK_WORD][0]:08x}u,"
      f" 0x{first[LOADBLOCK_WORD][1]:08x}u);")
    a(f"    stats->geometry_mode = (stats->geometry_mode &"
      f" 0x{first[GEOM_WORD][0]:08x}u) | 0x{first[GEOM_WORD][1]:08x}u;")
    a("}")
    a("")
    a("static void ndsNativeThunderGroundFinish(NDSRendererStats *stats)")
    a("{")
    restore = first[len(first) + GEOM_RESTORE_WORD_OFFSET]
    a(f"    stats->geometry_mode = (stats->geometry_mode & 0x{restore[0]:08x}u) |"
      f" 0x{restore[1]:08x}u;")
    a("}")
    a("")
    palette, alpha5, index3 = coverage_tables()
    a("/* THE COVERAGE TABLES.  The source edge is the IA8 alpha nibble, not a")
    a(" * palette and not a filter: these roots issue no G_LOADTLUT at all.")
    a(" * alpha5[] is (n * 0x11) >> 3 -- injective into 0..31, so all sixteen")
    a(" * source levels survive -- and index3[] rounds the intensity nibble into")
    a(" * the eight GL_RGB8_A5 palette slots.  Each palette entry is")
    a(" * (PRIM - ENV) * TEXEL0 + ENV at the MIDPOINT of the intensity pair it")
    a(" * serves, through the same rounding ndsRendererHardwareBlendPrimEnvTexel0")
    a(" * uses, so these colours agree with every other BLENDPE surface. */")
    a(f"static const u16 sNdsNativeThunderGroundCoveragePalette"
      f"[{COVERAGE_PALETTE_ENTRIES}] =")
    a("{")
    for red, green, blue in palette:
        packed = red | (green << 5) | (blue << 10)
        a(f"    0x{packed:04x}u, /* r{red:2d} g{green:2d} b{blue:2d} */")
    a("};")
    a("")
    a(f"static const u8 sNdsNativeThunderGroundCoverageAlpha5"
      f"[{SOURCE_COVERAGE_LEVELS}] =")
    a("{")
    a("    " + " ".join(f"{value}u," for value in alpha5))
    a("};")
    a("")
    a(f"static const u8 sNdsNativeThunderGroundCoverageIndex3"
      f"[{SOURCE_COVERAGE_LEVELS}] =")
    a("{")
    a("    " + " ".join(f"{value}u," for value in index3))
    a("};")
    a("")
    a(f"/* census: roots={len(ROOTS)} tris={len(ROOTS)} corners_each=3"
      " material=CURRENT_IMAGE referrer=244:0x0034"
      f" images={len(coverage['images'])} coverage=A5I3"
      f" partial={coverage['partial']}/{coverage['total']} */")
    return "\n".join(lines) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--emit", action="store_true")
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--census", action="store_true")
    args = ap.parse_args()
    if args.census:
        scanned, hits = census()
        print(f"scanned {scanned} O2R files")
        for file_id, slot, off in hits:
            print(f"  file {file_id} slot 0x{slot:04x} -> {ASSET}:0x{off:04x}")
        return 0
    decoded, coverage = decode()
    text = render(decoded, coverage)
    header = render_header(decoded, coverage)
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
        print("PIKACHU_THUNDERGROUND_NATIVE_OK roots=6 tris=6 "
              "material=CURRENT_IMAGE referrer=244:0x0034 "
              f"images={len(coverage['images'])} coverage=A5I3 "
              f"partial={coverage['partial']}/{coverage['total']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
