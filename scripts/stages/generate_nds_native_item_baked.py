#!/usr/bin/env python3
"""Generate/check the baked item, weapon and effect roots.

The hand-written item owners (generate_nds_native_item_wave1_core.py) each
verify one root's opcode census and emit helper calls for its state words and
arrays for its geometry. The roots listed here have the same shape -- fixed
state, fixed geometry, at most live segment-E materials -- so one compiler
writes them all as data: a list of native state ops (the shared
NDSNativeStateDelta applier's own effect codes), material hooks and geometry
groups, executed by src/nds/nds_native_item_baked.exec.inc.

Build tooling only. It reads the SHA-pinned BattleShip O2R payloads, walks
each root once (same-file G_DL calls inlined, the vertex cache simulated) and
refuses anything outside the supported subset. Runtime code never scans or
interprets the source display lists: it runs this generator's list.

2026-09-30: the roots the full-match lab census found drawn with no native
owner (artifacts/performance/2026-09-30_p2-2p8-native-owners/).
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(REPO / "scripts" / "stages"))

import generate_nds_native_stage as sm  # noqa: E402

OUT = REPO / "src/nds/generated/nds_native_item_baked.generated.inc"
OUT_HEADER = REPO / "include/nds/generated/nds_native_item_baked.generated.h"

FILES = {
    "MiscData086": sm.InputSpec(
        "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/MiscData086",
        "96e987d81b24497ad0c314372edb79f123016b59187b5f94b57b73e4bb122c04",
        86, 402, 0,
        "1c642e60401ca5e6df2fe1b0d6ddeb6e322b0288f4c13a1e608875322fbf9438"),
    "MiscDataBank159": sm.InputSpec(
        "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/MiscDataBank159",
        "7b8c444662623b4a1948c82371401cdae5bdf3108223b798b696542adba0d1b2",
        159, 43, 0,
        "977292623f0a4a3e5175fc77e4e39105d23e76fca4bf0ebc76613413f84e0ab6"),
    "NessSpecial2": sm.InputSpec(
        "decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/NessSpecial2",
        "3512fa549138f4bc1541c7cbb1ca45d0521207af38462a4ed87ef17298ebdb8a",
        352, None, None, None),
    "BonusDataBank150": sm.InputSpec(
        "decomp/BattleShip-main/BattleShip_o2r/reloc_bonus/BonusDataBank150",
        "28e3af3f9b7fb8d553b261715ae0fb6e7b36e226b8f6aec9b2eb79487260bf1d",
        150, 4, 0, None),
    "Bonus2Common": sm.InputSpec(
        "decomp/BattleShip-main/BattleShip_o2r/reloc_bonus/Bonus2Common",
        "fd673c99070c36bde367a5423966d532655225c1d8af80e0e95e9bb6c1478e33",
        136, 155, 0,
        "e6c01d89c93069509f3be95c6a60318c8902264ae708f38233e46a02cfca8d19"),
    "MiscData162": sm.InputSpec(
        "decomp/BattleShip-main/BattleShip_o2r/reloc_extern_data/MiscData162",
        "361173fa9420fbfee4c344f9ddce12fce05c94a9746063520779f20dc60614fd",
        162, 19, 0,
        "52378c9f1a32b66f659e1a60bb31becedd832a05f16e95191099ca4a7fab4781"),
    "NessSpecial3": sm.InputSpec(
        "decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/NessSpecial3",
        "5495f90a2d16eebebd8d93af2c42c53cbdc3e5a35d316eae234000ef77fa3071",
        336, 22, 0,
        "ba9cf122f9db7ff477f04ad708a2798a4ebe31bd4d1b124ca0a277aec821ab1b"),
}

# GObj ids (decomp sys/objdef.h): the owner a root is admitted for.
GOBJ_EFFECT = 1011
GOBJ_WEAPON = 1012
GOBJ_ITEM = 1013
# A stage layer's own GObj (nGCCommonKindGroundDisplay): Board the Platforms'
# platforms are DObj subtrees hung under its layer-1 yakumono DObjs.
GOBJ_GROUND_DISPLAY = 1009

# Every weapon draws after wpDisplayDrawNormal (wpdisplay.c:131):
# gDPSetRenderMode(G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF2), with G_ZBUFFER
# cleared. The port's weapon traversal starts from ndsRendererInitStats
# (render mode 0, opaque) and seeds only the geometry mode, so a weapon list
# that sets no render mode of its own (Koffing's smog, the Ray Gun's shot)
# drew its alpha-faded texture as solid squares. Each weapon root starts
# with the source's render mode; one that sets its own still overrides it.
WEAPON_RENDER_MODE = (0xE200001C, 0x005041C8)

# (name, file, root, gobj kind, source note). A root that already has a
# hand-written owner must not appear here (the adapter would draw it twice).
ENTRIES = (
    ("BombHeiWalkLeft", "MiscData086", 0x34C0, GOBJ_ITEM,
     "itbombhei.c:214 llITCommonDataBombHeiWalkLeftDisplayList"),
    ("LGunAmmo", "MiscData086", 0x40A8, GOBJ_WEAPON,
     "ITCommonData 0x2b0 LGunAmmoWeaponAttributes.data"),
    ("NBumperWait", "MiscData086", 0x7AF8, GOBJ_ITEM,
     "itnbumper.c llITCommonDataNBumperWaitDisplayList"),
    ("Tosakinto", "MiscData086", 0xB618, GOBJ_ITEM,
     "ITCommonData 0x7f0 TosakintoItemAttributes (Goldeen)"),
    ("Dogas", "MiscData086", 0x12730, GOBJ_ITEM,
     "ITCommonData 0xbf8 DogasItemAttributes (Koffing)"),
    ("DogasSmog", "MiscData086", 0x13050, GOBJ_WEAPON,
     "ITCommonData 0xc40 DogasSmogWeaponAttributes"),
    ("WarkRock", "MiscData086", 0xAAA8, GOBJ_WEAPON,
     "ITCommonData 0x774 WarkRockWeaponAttributes (Onix's rocks)"),
    ("Kabigon", "MiscData086", 0xB070, GOBJ_ITEM,
     "ITCommonData 0x7a8 KabigonItemAttributes (Snorlax)"),
    ("Mew", "MiscData086", 0xBBD0, GOBJ_ITEM,
     "ITCommonData 0x838 MewItemAttributes"),
    ("Nyars", "MiscData086", 0xC040, GOBJ_ITEM,
     "ITCommonData 0x880 NyarsItemAttributes (Meowth)"),
    ("NyarsCoin", "MiscData086", 0xC430, GOBJ_WEAPON,
     "ITCommonData 0x8c8 NyarsCoinWeaponAttributes"),
    ("Lizardon", "MiscData086", 0xD4E0, GOBJ_ITEM,
     "ITCommonData 0x8fc LizardonItemAttributes (Charizard)"),
    ("Spear", "MiscData086", 0xDE48, GOBJ_ITEM,
     "ITCommonData 0x98c SpearItemAttributes (Beedrill)"),
    ("SpearSwarm", "MiscData086", 0xE3B8, GOBJ_WEAPON,
     "ITCommonData 0x9d4 SpearSwarmWeaponAttributes"),
    ("Kamex", "MiscData086", 0xE970, GOBJ_ITEM,
     "ITCommonData 0xa08 KamexItemAttributes (Blastoise)"),
    ("KamexAlt", "MiscData086", 0xED60, GOBJ_ITEM,
     "llITCommonDataKamexDisplayList (itkamex.c, runtime-selected)"),
    ("KamexHydro0", "MiscData086", 0xF848, GOBJ_WEAPON,
     "ITCommonData 0xa50 KamexHydroWeaponAttributes, list 0"),
    ("KamexHydro1", "MiscData086", 0xF920, GOBJ_WEAPON,
     "ITCommonData 0xa50 KamexHydroWeaponAttributes, list 1"),
    ("MLucky", "MiscData086", 0xFF10, GOBJ_ITEM,
     "ITCommonData 0xa84 MLuckyItemAttributes (Chansey)"),
    ("Starmie", "MiscData086", 0x111B0, GOBJ_ITEM,
     "ITCommonData 0xb34 StarmieItemAttributes"),
    ("StarmieSwift", "MiscData086", 0x11918, GOBJ_WEAPON,
     "ITCommonData 0xb7c StarmieSwiftWeaponAttributes"),
    ("Sawamura", "MiscData086", 0x11E50, GOBJ_ITEM,
     "ITCommonData 0xbb0 SawamuraItemAttributes (Hitmonlee)"),
    ("SawamuraAlt", "MiscData086", 0x12340, GOBJ_ITEM,
     "llITCommonDataSawamuraDisplayList (itsawamura.c, runtime-selected)"),
    ("Pippi", "MiscData086", 0x134B0, GOBJ_ITEM,
     "ITCommonData 0xc74 PippiItemAttributes (Clefairy)"),
    ("PippiSwarm", "MiscData086", 0x134B0, GOBJ_WEAPON,
     "ITCommonData 0xcbc PippiSwarmWeaponAttributes (the same list)"),
    ("BoxEffect", "MiscData086", 0x68F0, GOBJ_ITEM,
     "llITCommonDataBoxEffectDisplayList (itbox.c, runtime-selected)"),
    ("WarkAlt", "MiscData086", 0xA640, GOBJ_ITEM,
     "llITCommonDataWarkDisplayList (itwark.c, runtime-selected)"),
    ("FushigibanaRazorOpa", "MiscDataBank159", 0x28A8, GOBJ_WEAPON,
     "GRYamabukiMap 0x308 FushigibanaRazorWeaponAttributes, DObj 1 list 0"),
    ("FushigibanaRazorXlu", "MiscDataBank159", 0x2998, GOBJ_WEAPON,
     "GRYamabukiMap 0x308 FushigibanaRazorWeaponAttributes, DObj 1 list 1"),
    ("NessSpecial2Effect", "NessSpecial2", 0x08E0, GOBJ_EFFECT,
     "NessSpecial2 root 0x08e0, a Ness effect"),
    ("NessPKFirePillarBase", "NessSpecial3", 0x0870, GOBJ_ITEM,
     "NessSpecial1 0x34 PKFireItemAttributes, DObj 2 (itnesspkfire.c)"),
    ("NessPKFirePillarFlame", "NessSpecial3", 0x0960, GOBJ_ITEM,
     "NessSpecial1 0x34 PKFireItemAttributes, DObj 3 (itnesspkfire.c)"),
    ("BonusTarget", "BonusDataBank150", 0x1048, GOBJ_ITEM,
     "ITBonus1ObjectHeader (file 253) data: DObjDesc 1, DL list 1 (ittarget.c)"),
    ("TaruBomb", "MiscData162", 0x0548, GOBJ_ITEM,
     "GRBonus3File3: Race to the Finish's barrel bomb (ittarubomb.c)"),
    ("TaruBombLink1", "MiscData162", 0x06C8, GOBJ_ITEM,
     "GRBonus3File3 DObjDesc 0x0788 DObj 2: the barrel's DL-link 1 list"),
    ("Bonus2PlatformSmall0", "Bonus2Common", 0x3A60, GOBJ_GROUND_DISPLAY,
     "sc1PBonusStageInitPlatforms: dSC1PBonusStagePlatformDescs Small, DObj 0"),
    ("Bonus2PlatformSmall1", "Bonus2Common", 0x3C10, GOBJ_GROUND_DISPLAY,
     "sc1PBonusStageInitPlatforms: dSC1PBonusStagePlatformDescs Small, DObj 1"),
    ("Bonus2PlatformSmall2", "Bonus2Common", 0x3CC0, GOBJ_GROUND_DISPLAY,
     "sc1PBonusStageInitPlatforms: dSC1PBonusStagePlatformDescs Small, DObj 2"),
    ("Bonus2PlatformMedium0", "Bonus2Common", 0x42B0, GOBJ_GROUND_DISPLAY,
     "sc1PBonusStageInitPlatforms: dSC1PBonusStagePlatformDescs Medium, DObj 0"),
    ("Bonus2PlatformMedium1", "Bonus2Common", 0x4440, GOBJ_GROUND_DISPLAY,
     "sc1PBonusStageInitPlatforms: dSC1PBonusStagePlatformDescs Medium, DObj 1"),
    ("Bonus2PlatformMedium2", "Bonus2Common", 0x44F0, GOBJ_GROUND_DISPLAY,
     "sc1PBonusStageInitPlatforms: dSC1PBonusStagePlatformDescs Medium, DObj 2"),
    ("Bonus2PlatformLarge0", "Bonus2Common", 0x4AE0, GOBJ_GROUND_DISPLAY,
     "sc1PBonusStageInitPlatforms: dSC1PBonusStagePlatformDescs Large, DObj 0"),
    ("Bonus2PlatformLarge1", "Bonus2Common", 0x4C70, GOBJ_GROUND_DISPLAY,
     "sc1PBonusStageInitPlatforms: dSC1PBonusStagePlatformDescs Large, DObj 1"),
    ("Bonus2PlatformLarge2", "Bonus2Common", 0x4D20, GOBJ_GROUND_DISPLAY,
     "sc1PBonusStageInitPlatforms: dSC1PBonusStagePlatformDescs Large, DObj 2"),
    ("Bonus2BoardedSmall0", "Bonus2Common", 0x5210, GOBJ_GROUND_DISPLAY,
     "sc1PBonusStageUpdatePlatformCount: dSC1PBonusStageBoardedPlatformDescs Small, DObj 0"),
    ("Bonus2BoardedSmall1", "Bonus2Common", 0x53C0, GOBJ_GROUND_DISPLAY,
     "sc1PBonusStageUpdatePlatformCount: dSC1PBonusStageBoardedPlatformDescs Small, DObj 1"),
    ("Bonus2BoardedSmall2", "Bonus2Common", 0x5468, GOBJ_GROUND_DISPLAY,
     "sc1PBonusStageUpdatePlatformCount: dSC1PBonusStageBoardedPlatformDescs Small, DObj 2"),
    ("Bonus2BoardedMedium0", "Bonus2Common", 0x5890, GOBJ_GROUND_DISPLAY,
     "sc1PBonusStageUpdatePlatformCount: dSC1PBonusStageBoardedPlatformDescs Medium, DObj 0"),
    ("Bonus2BoardedMedium1", "Bonus2Common", 0x5A20, GOBJ_GROUND_DISPLAY,
     "sc1PBonusStageUpdatePlatformCount: dSC1PBonusStageBoardedPlatformDescs Medium, DObj 1"),
    ("Bonus2BoardedMedium2", "Bonus2Common", 0x5AC8, GOBJ_GROUND_DISPLAY,
     "sc1PBonusStageUpdatePlatformCount: dSC1PBonusStageBoardedPlatformDescs Medium, DObj 2"),
    ("Bonus2BoardedLarge0", "Bonus2Common", 0x5EF0, GOBJ_GROUND_DISPLAY,
     "sc1PBonusStageUpdatePlatformCount: dSC1PBonusStageBoardedPlatformDescs Large, DObj 0"),
    ("Bonus2BoardedLarge1", "Bonus2Common", 0x6080, GOBJ_GROUND_DISPLAY,
     "sc1PBonusStageUpdatePlatformCount: dSC1PBonusStageBoardedPlatformDescs Large, DObj 1"),
    ("Bonus2BoardedLarge2", "Bonus2Common", 0x6128, GOBJ_GROUND_DISPLAY,
     "sc1PBonusStageUpdatePlatformCount: dSC1PBonusStageBoardedPlatformDescs Large, DObj 2"),
)

# Native state effect codes (src/nds/nds_renderer_assets.c) and the executor's
# own ops past them.
STATE_OTHERMODE = 2
STATE_COMBINE = 3
STATE_TEXTURE = 4
STATE_GEOMETRY = 5
STATE_IMAGE = 6
STATE_TILE = 7
STATE_LOAD_TLUT = 8
STATE_LOAD_BLOCK = 9
STATE_TILE_SIZE = 10
STATE_PRIM = 11
STATE_BLEND = 12
STATE_LIGHT_COLOR = 14
STATE_LOAD_TILE = 20
OP_ENV = 0x40
OP_MATERIAL = 0x41
OP_EMIT = 0x42
OP_NAMES = {
    STATE_OTHERMODE: "OTHERMODE", STATE_COMBINE: "COMBINE",
    STATE_TEXTURE: "TEXTURE", STATE_GEOMETRY: "GEOMETRY",
    STATE_IMAGE: "IMAGE", STATE_TILE: "TILE", STATE_LOAD_TLUT: "LOAD_TLUT",
    STATE_LOAD_BLOCK: "LOAD_BLOCK", STATE_TILE_SIZE: "TILE_SIZE",
    STATE_PRIM: "PRIM", STATE_BLEND: "BLEND",
    STATE_LIGHT_COLOR: "LIGHT_COLOR", STATE_LOAD_TILE: "LOAD_TILE",
    OP_ENV: "ENV", OP_MATERIAL: "MATERIAL", OP_EMIT: "EMIT",
}
SYNC_OPS = (0xE6, 0xE7, 0xE8, 0xE9, 0x00)
STATE_OPS = {
    0xE2: STATE_OTHERMODE, 0xE3: STATE_OTHERMODE, 0xFC: STATE_COMBINE,
    0xD7: STATE_TEXTURE, 0xF5: STATE_TILE, 0xF2: STATE_TILE_SIZE,
    0xF3: STATE_LOAD_BLOCK, 0xF4: STATE_LOAD_TILE, 0xFA: STATE_PRIM,
    0xF9: STATE_BLEND,
}
# The emitter's vertex buffer (nds_native_item_wave1_emit.exec.inc).
GROUP_VERTEX_MAX = 24
SEGMENT_E = 0x0E000000
G_MW_LIGHTCOL = 0x0A


class Refused(RuntimeError):
    pass


# Group flags (NDSNativeBakedGroup.flags).
GROUP_I_COVERAGE = 1
G_IM_FMT_I = 4
G_IM_SIZ_4B = 0
G_IM_SIZ_8B = 1
ACMUX_TEXEL0 = 1


def combine_reads_texel0_alpha(w0, w1):
    """TRUE when either cycle's alpha mux selects TEXEL0 alpha and the colour
    is not one of the renderer's own I-tile bakes (the BLENDPE endpoint lerp
    and the flat PRIM rgb, nds_renderer_textures_effects.c), which already
    keep coverage."""
    alpha = ((w0 >> 12) & 7, (w1 >> 12) & 7, (w0 >> 9) & 7, (w1 >> 9) & 7,
             (w1 >> 21) & 7, (w1 >> 3) & 7, (w1 >> 18) & 7, w1 & 7)
    color0 = ((w0 >> 20) & 0xF, (w1 >> 28) & 0xF, (w0 >> 15) & 0x1F, (w1 >> 15) & 7)
    blendpe = color0 == (3, 5, 1, 5)
    prim_rgb = color0 == (15, 15, 31, 3)
    return (ACMUX_TEXEL0 in alpha) and not blendpe and not prim_rgb


def group_flags(ops):
    """{group index: flags} from the state each EMIT runs under.

    An I4/I8 render tile under a combine that reads TEXEL0 alpha asks the
    texture bake for graded intensity coverage: ndsRendererHardwareConvertI
    makes every I texel opaque, which drew Koffing's smog as solid squares."""
    combine = (0, 0)
    tiles = {}
    render_tile = 0
    flags = {}
    for op, w0, w1, arg in ops:
        if op == STATE_COMBINE:
            combine = (w0, w1)
        elif op == STATE_TILE:
            tiles[(w1 >> 24) & 7] = ((w0 >> 21) & 7, (w0 >> 19) & 3)
        elif op == STATE_TEXTURE:
            render_tile = (w0 >> 8) & 7
        elif op == OP_EMIT:
            fmt, siz = tiles.get(render_tile, (None, None))
            if (fmt == G_IM_FMT_I and siz in (G_IM_SIZ_4B, G_IM_SIZ_8B) and
                    combine_reads_texel0_alpha(*combine)):
                flags[arg] = GROUP_I_COVERAGE
    return flags


# A list that draws with vertices an earlier list of the same DObj loaded
# (the source's lists run back to back in one RSP task, so the cache
# carries over): name -> the list whose final cache it starts from.
CACHE_SEEDS = {
    # KamexHydro1 patches the STs of list 0's slots 0-1 (G_MODIFYVTX) and
    # draws them with its own slots 2-3.
    "KamexHydro1": 0xF848,
}
G_MWO_POINT_ST = 0x14


def seed_cache(res, offset, cache, depth=0):
    """The cache a list leaves: its G_VTX loads in order, same-file G_DL
    calls followed."""
    if depth > 4:
        raise Refused(f"seed list at {offset:#x} nests too deep")
    for i in range(512):
        at = offset + i * 8
        w0 = struct.unpack_from(">I", res.payload, at)[0]
        op = w0 >> 24
        if op == 0xDF:
            return
        if op == 0xDE:
            ref = res.pointer_at(at + 4)
            if ref is not None and ref.asset_id == res.file_id:
                seed_cache(res, ref.offset, cache, depth + 1)
                if ((w0 >> 16) & 0xFF) == 1:
                    return
            continue
        if op == 0x01:
            ref = res.pointer_at(at + 4)
            count = (w0 >> 12) & 0xFF
            v0 = ((w0 >> 1) & 0x7F) - count
            for k in range(count):
                cache[v0 + k] = sm.decode_vertex(res, ref.offset + k * 16)
    raise Refused(f"seed list at {offset:#x} has no G_ENDDL")


def compile_root(name, res, root):
    """Walk `root` once. Returns (ops, groups, top_words, material_slots)."""
    ops = []
    groups = []
    cache = [None] * 64
    if name in CACHE_SEEDS:
        seed_cache(res, CACHE_SEEDS[name], cache)
    pending = []
    slots = set()
    top_words = []

    def flush():
        if not pending:
            return
        chunk_tris = []
        chunk_verts = []
        index = {}
        for tri in pending:
            new = [v for v in dict.fromkeys(tri) if v not in index]
            if len(chunk_verts) + len(new) > GROUP_VERTEX_MAX:
                groups.append((tuple(chunk_verts), tuple(chunk_tris)))
                ops.append((OP_EMIT, 0, 0, len(groups) - 1))
                chunk_tris, chunk_verts, index = [], [], {}
                new = list(dict.fromkeys(tri))
            for v in new:
                index[v] = len(chunk_verts)
                chunk_verts.append(v)
            chunk_tris.append(tuple(index[v] for v in tri))
        groups.append((tuple(chunk_verts), tuple(chunk_tris)))
        ops.append((OP_EMIT, 0, 0, len(groups) - 1))
        pending.clear()

    def walk(offset, depth):
        if depth > 4:
            raise Refused(f"{name}: G_DL nesting deeper than 4")
        for i in range(512):
            at = offset + i * 8
            if at + 8 > len(res.payload):
                raise Refused(f"{name}: list at {offset:#x} runs off the file")
            w0, w1 = struct.unpack_from(">II", res.payload, at)
            op = w0 >> 24
            ref = res.pointer_at(at + 4)
            if depth == 0:
                top_words.append((w0, w1))
            if op in SYNC_OPS:
                continue
            if op == 0xDF:
                return
            if op == 0x01:
                count = (w0 >> 12) & 0xFF
                v0 = ((w0 >> 1) & 0x7F) - count
                if ref is None or ref.asset_id != res.file_id:
                    raise Refused(f"{name}: G_VTX at {at:#x} is not file-local")
                if v0 < 0 or v0 + count > 32:
                    raise Refused(f"{name}: G_VTX at {at:#x} leaves the cache")
                for k in range(count):
                    cache[v0 + k] = sm.decode_vertex(res, ref.offset + k * 16)
                continue
            if op == 0x02:
                where = (w0 >> 16) & 0xFF
                slot = (w0 & 0xFFFF) // 2
                if where != G_MWO_POINT_ST or slot >= 32 or cache[slot] is None:
                    raise Refused(f"{name}: G_MODIFYVTX {w0:#010x} at {at:#x}")
                s_new = struct.unpack(">h", struct.pack(">H", w1 >> 16))[0]
                t_new = struct.unpack(">h", struct.pack(">H", w1 & 0xFFFF))[0]
                v = cache[slot]
                cache[slot] = (v[0], v[1], v[2], s_new, t_new, v[5])
                continue
            if op in (0x05, 0x06):
                for tri in sm.decode_triangles(op, w0, w1):
                    verts = tuple(cache[k] for k in tri)
                    if any(v is None for v in verts):
                        raise Refused(f"{name}: triangle at {at:#x} reads an empty slot")
                    pending.append(verts)
                continue
            if op == 0xDE:
                branch = ((w0 >> 16) & 0xFF) == 1
                if ref is None and (w1 & 0xFF000000) == SEGMENT_E and \
                        (w1 & 7) == 0 and not branch:
                    slot = (w1 & 0xFFFFFF) // 8
                    if slot > 1:
                        raise Refused(f"{name}: material slot {slot} at {at:#x}")
                    flush()
                    ops.append((OP_MATERIAL, 0, 0, slot))
                    slots.add(slot)
                    continue
                if ref is None or ref.asset_id != res.file_id:
                    raise Refused(f"{name}: G_DL at {at:#x} leaves the file")
                walk(ref.offset, depth + 1)
                if branch:
                    return
                continue
            flush()
            if op == 0xD9:
                ops.append((STATE_GEOMETRY, w0, w1, 0))
            elif op == 0xFD:
                if ref is None or ref.asset_id != res.file_id:
                    raise Refused(f"{name}: G_SETTIMG at {at:#x} is not file-local")
                ops.append((STATE_IMAGE, w0, ref.offset, 0))
            elif op == 0xF0:
                ops.append((STATE_LOAD_TLUT, w0, w1, 0))
            elif op == 0xFB:
                ops.append((OP_ENV, w0, w1, 0))
            elif op == 0xDB:
                if ((w0 >> 16) & 0xFF) != G_MW_LIGHTCOL:
                    raise Refused(f"{name}: G_MOVEWORD {w0:#010x} at {at:#x}")
                ops.append((STATE_LIGHT_COLOR, w0, w1, 0))
            elif op in STATE_OPS:
                ops.append((STATE_OPS[op], w0, w1, 0))
            else:
                raise Refused(f"{name}: opcode {op:#04x} at {at:#x} is unsupported")
        raise Refused(f"{name}: list at {offset:#x} has no G_ENDDL")

    if name in CACHE_SEEDS:
        # The state the seed list leaves carries over as well: replay its
        # state ops (not its draws or its material) first.
        seed_ops, _, _, _ = compile_root(f"{name}.seed", res,
                                         CACHE_SEEDS[name])
        ops.extend(o for o in seed_ops
                   if o[0] not in (OP_EMIT, OP_MATERIAL))
    walk(root, 0)
    flush()
    if not groups:
        raise Refused(f"{name}: root {root:#x} draws nothing")
    if slots and slots != set(range(max(slots) + 1)):
        raise Refused(f"{name}: material slots {sorted(slots)} are not dense")
    return ops, groups, tuple(top_words), (max(slots) + 1) if slots else 0


def build():
    loaded = {key: sm.load_o2r(REPO, spec) for key, spec in FILES.items()}
    roots = []
    all_ops = []
    all_groups = []
    for name, key, root, gobj_kind, note in ENTRIES:
        res = loaded[key]
        ops, groups, top, slots = compile_root(name, res, root)
        if gobj_kind == GOBJ_WEAPON:
            ops = [(STATE_OTHERMODE, WEAPON_RENDER_MODE[0],
                    WEAPON_RENDER_MODE[1], 0)] + ops
        flags = group_flags(ops)
        group_base = len(all_groups)
        roots.append(dict(
            name=name, asset=res.file_id, root=root, gobj=gobj_kind,
            note=note, first_op=len(all_ops), op_count=len(ops),
            slots=slots, top_words=len(top),
            # The runtime reads images and TLUTs anywhere in the file, so it
            # asks for the whole source payload, as the hand owners' FILE_END
            # asks for their spans.
            end=len(res.payload), last_w0=top[-1][0]))
        for op, w0, w1, arg in ops:
            if op == OP_EMIT:
                arg += group_base
            all_ops.append((op, w0, w1, arg, name))
        all_groups.extend((name, g, flags.get(i, 0))
                          for i, g in enumerate(groups))
    return roots, all_ops, all_groups


def render(roots, ops, groups) -> str:
    lines = [
        "/* Baked item/weapon roots (generated).",
        " * Do not hand-edit; regenerate with generate_nds_native_item_baked.py.",
        " * Sources: SHA-pinned MiscData086 (file 86) and NessSpecial3 (file 336).",
        " * Each op is a native state delta, a live segment-E material or a",
        " * geometry group; see nds_native_item_baked.exec.inc. */",
        "#include <nds/generated/nds_native_item_baked.generated.h>",
        "",
    ]
    for gi, (name, (verts, tris), _flags) in enumerate(groups):
        lines.append(f"/* group {gi}: {name}, {len(verts)} vertices, {len(tris)} triangles */")
        lines.append(f"static const s16 sNdsNativeBakedVerts{gi}[{len(verts) * 5}] =")
        lines.append("{")
        for v in verts:
            lines.append(f"    {v[0]}, {v[1]}, {v[2]}, {v[3]}, {v[4]},")
        lines.append("};")
        lines.append(f"static const u32 sNdsNativeBakedColors{gi}[{len(verts)}] =")
        lines.append("{")
        for v in verts:
            lines.append(f"    0x{v[5]:08x}u,")
        lines.append("};")
        lines.append(f"static const u16 sNdsNativeBakedTris{gi}[{len(tris) * 3}] =")
        lines.append("{")
        for t in tris:
            lines.append(f"    {t[0]}u, {t[1]}u, {t[2]}u,")
        lines.append("};")
        lines.append("")
    lines.append("static const NDSNativeBakedGroup "
                 "sNdsNativeBakedGroups[NDS_NATIVE_BAKED_GROUP_COUNT] =")
    lines.append("{")
    for gi, (name, (verts, tris), gflags) in enumerate(groups):
        lines.append(f"    {{ sNdsNativeBakedVerts{gi}, sNdsNativeBakedColors{gi}, "
                     f"sNdsNativeBakedTris{gi}, {len(verts)}u, {len(tris)}u, {gflags}u }},")
    lines.append("};")
    lines.append("")
    lines.append("static const NDSNativeBakedOp "
                 "sNdsNativeBakedOps[NDS_NATIVE_BAKED_OP_COUNT] =")
    lines.append("{")
    current = None
    for op, w0, w1, arg, name in ops:
        if name != current:
            lines.append(f"    /* {name} */")
            current = name
        lines.append(f"    {{ 0x{w0:08x}u, 0x{w1:08x}u, {arg}u, {op}u, 0u }}, "
                     f"/* {OP_NAMES[op]} */")
    lines.append("};")
    lines.append("")
    lines.append("static const NDSNativeBakedRoot "
                 "sNdsNativeBakedRoots[NDS_NATIVE_BAKED_ROOT_COUNT] =")
    lines.append("{")
    for r in roots:
        lines.append(f"    /* {r['name']}: {r['note']} */")
        lines.append(f"    {{ 0x{r['root']:05x}u, 0x{r['end']:05x}u, "
                     f"0x{r['last_w0']:08x}u, {r['asset']}u, {r['gobj']}u, "
                     f"{r['first_op']}u, {r['op_count']}u, {r['top_words']}u, "
                     f"{r['slots']}u, 0u }},")
    lines.append("};")
    lines.append("")
    return "\n".join(lines)


def render_header(roots, ops, groups) -> str:
    assets = sorted({r["asset"] for r in roots})
    return "\n".join((
        "/* Baked item/weapon roots (generated).",
        " * Do not hand-edit; regenerate with generate_nds_native_item_baked.py. */",
        "#ifndef NDS_NATIVE_ITEM_BAKED_GENERATED_H",
        "#define NDS_NATIVE_ITEM_BAKED_GENERATED_H",
        "",
        f"#define NDS_NATIVE_BAKED_ROOT_COUNT {len(roots)}u",
        f"#define NDS_NATIVE_BAKED_OP_COUNT {len(ops)}u",
        f"#define NDS_NATIVE_BAKED_GROUP_COUNT {len(groups)}u",
        "#define NDS_NATIVE_BAKED_MATERIAL_SLOTS 2u",
        f"#define NDS_NATIVE_BAKED_OP_ENV {OP_ENV}u",
        f"#define NDS_NATIVE_BAKED_OP_MATERIAL {OP_MATERIAL}u",
        f"#define NDS_NATIVE_BAKED_OP_EMIT {OP_EMIT}u",
        "/* The assets any baked root lives in: the adapter's cheap first test. */",
        "#define NDS_NATIVE_BAKED_ASSET_MATCH(asset) \\",
        "    (" + " || ".join(f"((asset) == {a}u)" for a in assets) + ")",
        "",
        "#endif",
        "",
    ))


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--emit", action="store_true")
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--list", action="store_true", help="print each root's ops")
    args = ap.parse_args()
    roots, ops, groups = build()
    packet = render(roots, ops, groups)
    header = render_header(roots, ops, groups)
    if args.list:
        for op, w0, w1, arg, name in ops:
            print(f"{name:22s} {OP_NAMES[op]:11s} {w0:08x} {w1:08x} {arg}")
    if args.emit:
        for path, text in ((OUT, packet), (OUT_HEADER, header)):
            path.parent.mkdir(parents=True, exist_ok=True)
            if (not path.exists()) or path.read_text() != text:
                path.write_text(text, newline="\n")
        print(f"emitted {OUT.relative_to(REPO)} and {OUT_HEADER.relative_to(REPO)}")
    if args.check or not args.emit:
        for path, text in ((OUT, packet), (OUT_HEADER, header)):
            if not path.exists() or path.read_text() != text:
                raise RuntimeError(f"generated artefact stale: {path.relative_to(REPO)}")
    tri_total = sum(len(t) for _, (_, t), _ in groups)
    coverage = sorted({name for name, _, gflags in groups if gflags & GROUP_I_COVERAGE})
    print(f"ITEM_BAKED_NATIVE_OK roots={len(roots)} ops={len(ops)} "
          f"groups={len(groups)} tris={tri_total} i_coverage={','.join(coverage) or '-'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
