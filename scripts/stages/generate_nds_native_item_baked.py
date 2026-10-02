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
# GOBJ_SCENE roots (the 1P ending's room) ship as a NitroFS file the ending
# loads into its own heap (battleship_mvending.c), so neither the resident
# image nor the front-end overlay carries their ~22 KB: static bytes there are
# taken from every scene's arena. Little-endian; see room_binary().
ROOM_MAGIC = 0x31424D52  # 'RMB1'
ROOM_VERSION = 1

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
    "MVCommon": sm.InputSpec(
        "decomp/BattleShip-main/BattleShip_o2r/reloc_movies/MVCommon",
        "f5ed65ebf90e7a933c683d9187d3cbeb1e43f8df88756a9646988a43b7186c0e",
        52, None, None, None),
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
# A scene GObj made with id 0 (gcMakeGObjSPAfter(0, ...)): the 1P ending's
# room objects.
GOBJ_SCENE = 0

# Every weapon draws after wpDisplayDrawNormal (wpdisplay.c:131):
# gDPSetRenderMode(G_RM_AA_XLU_SURF, G_RM_AA_XLU_SURF2), with G_ZBUFFER
# cleared. The port's weapon traversal starts from ndsRendererInitStats
# (render mode 0, opaque) and seeds only the geometry mode, so a weapon list
# that sets no render mode of its own (Koffing's smog, the Ray Gun's shot)
# drew its alpha-faded texture as solid squares. Each weapon root starts
# with the source's render mode; one that sets its own still overrides it.
WEAPON_RENDER_MODE = (0xE200001C, 0x005041C8)

# Board the Platforms' platforms hang under layer-1 yakumono DObjs and draw in
# that layer's gcDrawDObjTreeDLLinksForGObj, which writes each DL link to its
# own display-list head: grDisplayLayer1PriProcDisplay / SecProcDisplay
# (grdisplay.c:86-109) start head 0 with G_RM_AA_ZB_OPA_SURF and head 1 with
# G_RM_AA_ZB_XLU_SURF. Every platform's DObj 0 is DL link 0 (the board) and
# DObjs 1-2 are link 1 (the lights; 136_Bonus2Common.c DObjDLLink tables).
# None of these lists sets a render mode, so each root starts with its head's
# mode -- the lights drew opaque when they inherited the traversal's. The
# board is seeded too: the heads never share state in the source, so a light
# drawn before the next board must not leave it translucent.
GROUND_LINK0_RENDER_MODE = (0xE200001C, 0x00552078)
GROUND_LINK1_RENDER_MODE = (0xE200001C, 0x005049D8)
GROUND_LINK1_ROOTS = frozenset(
    f"Bonus2{kind}{size}{dobj}"
    for kind in ("Platform", "Boarded")
    for size in ("Small", "Medium", "Large")
    for dobj in (1, 2))

# The ending's room camera (func_80017EC0 -> func_80016338, objdisplay.c:
# 2680-2684) starts display heads 0 and 2 at G_RM_AA_ZB_OPA_SURF and heads 1
# and 3 at G_RM_AA_ZB_XLU_SURF; the background's DL-link-1 lists draw on head 1
# and set no mode of their own, the rest draw on head 0.
SCENE_LINK1_ROOTS = frozenset((
    "MVCommonRoomBackground07398",
    "MVCommonRoomBackground074A0",
    "MVCommonRoomBackground075A0",
    "MVCommonRoomBackground076D8",
    "MVCommonRoomBackground077C0",
    "MVCommonRoomBackground07840",
    "MVCommonRoomBackground07928",
    "MVCommonRoomBackground079D0",
    "MVCommonRoomBackground07A78",
    "MVCommonRoomBackground07B60",
))

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
    # P2-6: the 1P ending's room (mvending.c:158-236; the opening room
    # draws the same MVCommon lists). Every DObj list of the background
    # (gcDrawDObjTreeDLLinksForGObj), desk, books, pencils and lamp
    # (gcDrawDObjTreeForGObj) and the tissues (gcDrawDObjDLHead0), each a
    # GObj the scene makes with id 0.
    ("MVCommonRoomBackground05A18", "MVCommon", 0x05A18, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x7ef0, DL link 0"),
    ("MVCommonRoomBackground05AC8", "MVCommon", 0x05AC8, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x7f1c, DL link 0"),
    ("MVCommonRoomBackground05BB0", "MVCommon", 0x05BB0, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x7f48, DL link 0"),
    ("MVCommonRoomBackground05CA0", "MVCommon", 0x05CA0, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x7f74, DL link 0"),
    ("MVCommonRoomBackground05D68", "MVCommon", 0x05D68, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x7fa0, DL link 0"),
    ("MVCommonRoomBackground05DE8", "MVCommon", 0x05DE8, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x7fcc, DL link 0"),
    ("MVCommonRoomBackground05EA8", "MVCommon", 0x05EA8, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x7ff8, DL link 0"),
    ("MVCommonRoomBackground05F70", "MVCommon", 0x05F70, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x807c, DL link 0"),
    ("MVCommonRoomBackground06058", "MVCommon", 0x06058, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x80a8, DL link 0"),
    ("MVCommonRoomBackground06130", "MVCommon", 0x06130, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x80d4, DL link 0"),
    ("MVCommonRoomBackground06210", "MVCommon", 0x06210, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x8100, DL link 0"),
    ("MVCommonRoomBackground062D8", "MVCommon", 0x062D8, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x8158, DL link 0"),
    ("MVCommonRoomBackground063F8", "MVCommon", 0x063F8, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x8184, DL link 0"),
    ("MVCommonRoomBackground07398", "MVCommon", 0x07398, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x81dc, DL link 1"),
    ("MVCommonRoomBackground064C0", "MVCommon", 0x064C0, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x8208, DL link 0"),
    ("MVCommonRoomBackground074A0", "MVCommon", 0x074A0, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x8208, DL link 1"),
    ("MVCommonRoomBackground075A0", "MVCommon", 0x075A0, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x8234, DL link 1"),
    ("MVCommonRoomBackground076D8", "MVCommon", 0x076D8, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x828c, DL link 1"),
    ("MVCommonRoomBackground06590", "MVCommon", 0x06590, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x82b8, DL link 0"),
    ("MVCommonRoomBackground06680", "MVCommon", 0x06680, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x82e4, DL link 0"),
    ("MVCommonRoomBackground06708", "MVCommon", 0x06708, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x8310, DL link 0"),
    ("MVCommonRoomBackground067E8", "MVCommon", 0x067E8, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x833c, DL link 0"),
    ("MVCommonRoomBackground068B8", "MVCommon", 0x068B8, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x8368, DL link 0"),
    ("MVCommonRoomBackground077C0", "MVCommon", 0x077C0, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x83c0, DL link 1"),
    ("MVCommonRoomBackground069C0", "MVCommon", 0x069C0, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x8444, DL link 0"),
    ("MVCommonRoomBackground06AB0", "MVCommon", 0x06AB0, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x8470, DL link 0"),
    ("MVCommonRoomBackground06CE0", "MVCommon", 0x06CE0, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x84c8, DL link 0"),
    ("MVCommonRoomBackground06DD0", "MVCommon", 0x06DD0, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x84f4, DL link 0"),
    ("MVCommonRoomBackground07840", "MVCommon", 0x07840, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x84f4, DL link 1"),
    ("MVCommonRoomBackground06EC0", "MVCommon", 0x06EC0, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x8520, DL link 0"),
    ("MVCommonRoomBackground07928", "MVCommon", 0x07928, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x85a4, DL link 1"),
    ("MVCommonRoomBackground079D0", "MVCommon", 0x079D0, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x85d0, DL link 1"),
    ("MVCommonRoomBackground07A78", "MVCommon", 0x07A78, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x8628, DL link 1"),
    ("MVCommonRoomBackground07B60", "MVCommon", 0x07B60, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x8654, DL link 1"),
    ("MVCommonRoomBackground06FD8", "MVCommon", 0x06FD8, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x8680, DL link 0"),
    ("MVCommonRoomBackground070C8", "MVCommon", 0x070C8, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x86ac, DL link 0"),
    ("MVCommonRoomBackground071B8", "MVCommon", 0x071B8, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x86d8, DL link 0"),
    ("MVCommonRoomBackground072A8", "MVCommon", 0x072A8, GOBJ_SCENE,
     "llMVCommonRoomBackgroundDObjDesc DObj @0x8704, DL link 0"),
    ("MVCommonRoomDesk08CA0", "MVCommon", 0x08CA0, GOBJ_SCENE,
     "llMVCommonRoomDeskDObjDesc DObj @0x8e24"),
    ("MVCommonRoomBooks0A3F0", "MVCommon", 0x0A3F0, GOBJ_SCENE,
     "llMVCommonRoomBooksDObjDesc DObj @0xa724"),
    ("MVCommonRoomBooks0A5E8", "MVCommon", 0x0A5E8, GOBJ_SCENE,
     "llMVCommonRoomBooksDObjDesc DObj @0xa750"),
    ("MVCommonRoomPencils0ACF0", "MVCommon", 0x0ACF0, GOBJ_SCENE,
     "llMVCommonRoomPencilsDObjDesc DObj @0xaeb8"),
    ("MVCommonRoomPencils0ADD0", "MVCommon", 0x0ADD0, GOBJ_SCENE,
     "llMVCommonRoomPencilsDObjDesc DObj @0xaee4"),
    ("MVCommonRoomPencils0AE68", "MVCommon", 0x0AE68, GOBJ_SCENE,
     "llMVCommonRoomPencilsDObjDesc DObj @0xaf10"),
    ("MVCommonRoomLamp0BC28", "MVCommon", 0x0BC28, GOBJ_SCENE,
     "llMVCommonRoomLampDObjDesc DObj @0xbdec"),
    ("MVCommonRoomLamp0BD00", "MVCommon", 0x0BD00, GOBJ_SCENE,
     "llMVCommonRoomLampDObjDesc DObj @0xbe18"),
    ("MVCommonRoomLamp0BD40", "MVCommon", 0x0BD40, GOBJ_SCENE,
     "llMVCommonRoomLampDObjDesc DObj @0xbe44"),
    ("MVCommonRoomTissues0C690", "MVCommon", 0x0C690, GOBJ_SCENE,
     "llMVCommonRoomTissuesDisplayList (gcDrawDObjDLHead0)"),
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
    keep coverage -- except the lerp whose alpha is TEXEL0 * PRIM: its bake
    keeps coverage on I4 only, so an I8 tile asks (Board the Platforms'
    lights drew as solid discs)."""
    alpha = ((w0 >> 12) & 7, (w1 >> 12) & 7, (w0 >> 9) & 7, (w1 >> 9) & 7,
             (w1 >> 21) & 7, (w1 >> 3) & 7, (w1 >> 18) & 7, w1 & 7)
    color0 = ((w0 >> 20) & 0xF, (w1 >> 28) & 0xF, (w0 >> 15) & 0x1F, (w1 >> 15) & 7)
    blendpe = color0 == (3, 5, 1, 5)
    blendpe_prim_alpha = blendpe and alpha[:4] == (ACMUX_TEXEL0, 7, 3, 7)
    prim_rgb = color0 == (15, 15, 31, 3)
    return ((ACMUX_TEXEL0 in alpha) and (not blendpe or blendpe_prim_alpha)
            and not prim_rgb)


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
    # A boarded platform's second light sets no combine of its own: both
    # lights are DL link 1, so the source draws list 2 right after list 1 on
    # the same display-list head and it inherits list 1's state.
    "Bonus2BoardedSmall2": 0x53C0,
    "Bonus2BoardedMedium2": 0x5A20,
    "Bonus2BoardedLarge2": 0x6080,
    # The ending's props draw their DObj lists back to back on display head
    # 0, and the later lists set only what changes: the second book's
    # covers, the pencils' second and third lists and the lamp's head use
    # the TLUT mode, texture and unlit, flat geometry their tree's earlier
    # lists left. Seeds replay in order.
    "MVCommonRoomBooks0A5E8": (0xA3F0,),
    "MVCommonRoomPencils0ADD0": (0xACF0,),
    "MVCommonRoomPencils0AE68": (0xACF0, 0xADD0),
    "MVCommonRoomLamp0BD00": (0xBC28,),
    "MVCommonRoomLamp0BD40": (0xBC28, 0xBD00),
}
G_MWO_POINT_ST = 0x14
# A TLUT load one entry short of what its texture indexes: the books' CI8
# cover loads 255 colours (gDPLoadTLUT count 255) and 4 of its texels use
# index 255, which the RDP reads from whatever TMEM held. The DS refuses a
# palette shorter than its texels (BAD_TLUT) and drew the books untextured,
# so this load takes 256 entries -- the file's next word, 0x0000.
# (file, list offset of the G_LOADTLUT) -> colours.
TLUT_COUNT_FIXES = {
    (52, 0x0A588): 256,
    (52, 0x0A680): 256,
}


def cache_seeds(name):
    """The seed lists of `name`, in draw order."""
    seeds = CACHE_SEEDS.get(name, ())
    return (seeds,) if isinstance(seeds, int) else tuple(seeds)


# Baked lighting for the room. The room movies' pre-render functions
# (mvEndingFuncLights, mvOpeningRoomFuncLights) set G_LIGHTING on every display
# head (syRdpResetSettings runs them after the reset list) and aim light 1 with
# ftDisplayLightsDrawReflect, so a room list that draws with G_LIGHTING still
# set is lit by the RSP: its colour bytes are normals. A battle's baked roots
# are lit by the executor itself (the battle submit's geometry carries
# G_LIGHTING and the stage's light: Board the Platforms' platforms), but the
# room submit does not light, so the light is folded into the colours here, as
# the stage generator does for map lists (bake_source_lighting): ambient +
# diffuse * max(0, N.L), with the lists' own gSPLightColor words and L carried
# into the DObj's frame.
G_LIGHTING = 0x20000
# scSubsysFighterSetLightParams(45.0F, 45.0F, ...) in mvEndingFuncStart
# (mvending.c:546) and mvOpeningRoomFuncStart (mvopeningroom.c:1355).
LIGHT_ANGLE_ROOM = (45.0, 45.0)
# Battle roots whose light is fixed are baked too, for speed: the executor
# lights every vertex of a lit root every frame (Board the Platforms' platforms
# were most of its 205K-cycle baked emit). The light is light_angle (20, 45)
# from every bonus2 map header (the 12 boards share it); the platform DObjs
# are unrotated with uniform scale. Such a root clears G_LIGHTING before its
# first op and sets it again after its last, so the executor draws the baked
# colours and every later root still finds the battle's G_LIGHTING.
LIGHT_ANGLE_BONUS2 = (20.0, 45.0)
BATTLE_LIGHT_BAKES = {
    f"Bonus2{kind}{size}0": LIGHT_ANGLE_BONUS2
    for kind in ("Platform", "Boarded")
    for size in ("Small", "Medium", "Large")
}
# The executor puts a vertex in v16 as value << 4 (1/256 of a source unit per
# hardware unit, NDS_RENDERER_HW_WORLD_UNIT_SHIFT 8), clamped to +-2047
# source units. A root whose geometry reaches past that (the ending room's
# walls, up to 7,658) stores its vertices shifted right and the executor
# scales the modelview's axes back (NDSNativeBakedRoot.vertex_shift); the
# rounding is the native stage path's (ndsRendererNativeStageVertexShift).
V16_SOURCE_MAX = 2047


def shift_round(value: int, shift: int) -> int:
    if shift == 0:
        return value
    magnitude = (abs(value) + (1 << (shift - 1))) >> shift
    return -magnitude if value < 0 else magnitude


def vertex_shift_for(groups) -> int:
    peak = max((abs(c) for verts, _ in groups for v in verts for c in v[:3]),
               default=0)
    shift = 0
    while shift_round(peak, shift) > V16_SOURCE_MAX:
        shift += 1
        if shift > 7:
            raise Refused(f"geometry reaches {peak}: no vertex shift fits")
    return shift


GEOMETRY_CLEAR_LIGHTING = (STATE_GEOMETRY, 0xD9000000 | (~G_LIGHTING & 0xFFFFFF), 0, 0)
GEOMETRY_SET_LIGHTING = (STATE_GEOMETRY, 0xD9FFFFFF, G_LIGHTING, 0)
# The MVCommon room's DObjDesc tables (mvending.c:158-221), searched for the
# DObj that draws a room root, so its light rides the DObj's rest pose.
ROOM_DOBJ_TABLES = (0x07E98, 0x08DF8, 0x0A6F8, 0x0AEB8, 0x0BDC0)
DOBJ_DESC_BYTES = 0x2C
DOBJ_DESC_END = 18


def _rpy_rows(rotate, scale):
    """syMatrixTraRotRpyRSca's 3x3 (row vectors: rows are the DObj's axes)."""
    import math
    r, p, y = rotate
    sr, cr = math.sin(r), math.cos(r)
    sp, cp = math.sin(p), math.cos(p)
    sy, cy = math.sin(y), math.cos(y)
    return (
        tuple(c * scale[0] for c in (cp * cy, cp * sy, -sp)),
        tuple(c * scale[1] for c in (sr * sp * cy - cr * sy,
                                     sr * sp * sy + cr * cy, sr * cp)),
        tuple(c * scale[2] for c in (cr * sp * cy + sr * sy,
                                     cr * sp * sy - sr * cy, cr * cp)),
    )


def _mul_rows(a, b):
    return tuple(tuple(sum(a[i][k] * b[k][j] for k in range(3))
                       for j in range(3)) for i in range(3))


def room_dobj_rows(res, root):
    """The world axes of the room DObj whose list (or DL link) is `root`:
    gcSetupCommonDObjs makes desc id N a child of the latest id N-1."""
    for table in ROOM_DOBJ_TABLES:
        chain = {}
        offset = table
        while True:
            depth = struct.unpack_from(">i", res.payload, offset)[0]
            if depth == DOBJ_DESC_END:
                break
            rotate = struct.unpack_from(">3f", res.payload, offset + 0x14)
            scale = struct.unpack_from(">3f", res.payload, offset + 0x20)
            local = _rpy_rows(rotate, scale)
            world = local if depth == 0 else _mul_rows(local, chain[depth - 1])
            chain[depth] = world
            ref = res.pointer_at(offset + 4)
            if ref is not None and ref.asset_id == res.file_id:
                lists = [ref.offset]
                if table == ROOM_DOBJ_TABLES[0]:
                    # The background draws DL links (gcDrawDObjTreeDLLinks).
                    lists = []
                    link = ref.offset
                    while struct.unpack_from(">i", res.payload, link)[0] != 4:
                        target = res.pointer_at(link + 4)
                        if target is not None:
                            lists.append(target.offset)
                        link += 8
                if root in lists:
                    return world
            offset += DOBJ_DESC_BYTES
    return None


def object_light(angle, rows=None):
    """Light 1 (ftDisplayLightsDrawReflect's s8 words) in a DObj's frame."""
    import math
    light = sm.reflect_light_direction(*angle)
    norm = math.sqrt(sum(c * c for c in light)) or 1.0
    world = tuple(c / norm for c in light)
    if rows is None:
        return world
    projected = []
    for row in rows:
        length = math.sqrt(sum(c * c for c in row)) or 1.0
        projected.append(sum(row[k] * world[k] for k in range(3)) / length)
    plen = math.sqrt(sum(c * c for c in projected)) or 1.0
    return tuple(c / plen for c in projected)


def lit_rgba(rgba, light, diffuse, ambient):
    """The RSP's colour for a lit vertex whose colour bytes are its normal."""
    normal = []
    for shift in (24, 16, 8):
        byte = (rgba >> shift) & 0xFF
        normal.append((byte - 256 if byte > 127 else byte) / 128.0)
    intensity = max(0.0, sum(normal[k] * light[k] for k in range(3)))
    rgb = 0
    for shift in (16, 8, 0):
        channel = (((ambient >> shift) & 0xFF)
                   + intensity * ((diffuse >> shift) & 0xFF))
        rgb = (rgb << 8) | min(255, int(channel))
    return (rgb << 8) | (rgba & 0xFF)


def end_light_state(res, offset, state, depth=0):
    """Run a list's G_LIGHTING and gSPLightColor words over `state`
    (lit, diffuse, ambient): what a following list inherits."""
    if depth > 4:
        raise Refused(f"seed list at {offset:#x} nests too deep")
    for i in range(512):
        at = offset + i * 8
        w0, w1 = struct.unpack_from(">II", res.payload, at)
        op = w0 >> 24
        if op == 0xDF:
            return state
        if op == 0xD9:
            state = (((state[0] and (w0 & G_LIGHTING) != 0)
                      or (w1 & G_LIGHTING) != 0), state[1], state[2])
        elif op == 0xDB and ((w0 >> 16) & 0xFF) == G_MW_LIGHTCOL:
            if (w0 & 0xFFFF) == 0x00:
                state = (state[0], w1 >> 8, state[2])
            elif (w0 & 0xFFFF) == 0x18:
                state = (state[0], state[1], w1 >> 8)
        elif op == 0xDE:
            ref = res.pointer_at(at + 4)
            if ref is not None and ref.asset_id == res.file_id:
                state = end_light_state(res, ref.offset, state, depth + 1)
                if ((w0 >> 16) & 0xFF) == 1:
                    return state
    raise Refused(f"seed list at {offset:#x} has no G_ENDDL")


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


def compile_root(name, res, root, light=None, refuse_unbaked_lit=False):
    """Walk `root` once. Returns (ops, groups, top_words, material_slots).

    `light` (a unit vector in the DObj's frame) bakes the RSP's lighting
    into the triangles drawn with G_LIGHTING set; without it those keep
    their colour bytes, or refuse when `refuse_unbaked_lit`."""
    ops = []
    groups = []
    cache = [None] * 64
    # (G_LIGHTING, light 1 colour, light 2 colour) as the root starts: the
    # pre-render function's G_LIGHTING, then what its seed lists leave.
    lighting = (True, None, None)
    for seed in cache_seeds(name):
        seed_cache(res, seed, cache)
        lighting = end_light_state(res, seed, lighting)
    lighting = list(lighting)
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
                    if lighting[0] and light is not None:
                        if lighting[1] is None or lighting[2] is None:
                            raise Refused(f"{name}: lit triangle at {at:#x} "
                                          "before both light colours")
                        verts = tuple(v[:5] + (lit_rgba(v[5], light, lighting[1],
                                                        lighting[2]),)
                                      for v in verts)
                    elif lighting[0] and refuse_unbaked_lit:
                        raise Refused(f"{name}: lit triangle at {at:#x} has no "
                                      "light bake")
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
                lighting[0] = ((lighting[0] and (w0 & G_LIGHTING) != 0)
                               or (w1 & G_LIGHTING) != 0)
                ops.append((STATE_GEOMETRY, w0, w1, 0))
            elif op == 0xFD:
                if ref is None or ref.asset_id != res.file_id:
                    raise Refused(f"{name}: G_SETTIMG at {at:#x} is not file-local")
                ops.append((STATE_IMAGE, w0, ref.offset, 0))
            elif op == 0xF0:
                colours = TLUT_COUNT_FIXES.get((res.file_id, at))
                if colours is not None:
                    w1 = (w1 & ~(0x3FF << 14)) | ((colours - 1) << 14)
                ops.append((STATE_LOAD_TLUT, w0, w1, 0))
            elif op == 0xFB:
                ops.append((OP_ENV, w0, w1, 0))
            elif op == 0xDB:
                if ((w0 >> 16) & 0xFF) != G_MW_LIGHTCOL:
                    raise Refused(f"{name}: G_MOVEWORD {w0:#010x} at {at:#x}")
                if (w0 & 0xFFFF) == 0x00:
                    lighting[1] = w1 >> 8
                elif (w0 & 0xFFFF) == 0x18:
                    lighting[2] = w1 >> 8
                ops.append((STATE_LIGHT_COLOR, w0, w1, 0))
            elif op in STATE_OPS:
                ops.append((STATE_OPS[op], w0, w1, 0))
            else:
                raise Refused(f"{name}: opcode {op:#04x} at {at:#x} is unsupported")
        raise Refused(f"{name}: list at {offset:#x} has no G_ENDDL")

    for index, seed in enumerate(cache_seeds(name)):
        # The state the seed lists leave carries over as well: replay their
        # state ops (not their draws or their materials) first, in order.
        seed_ops, _, _, _ = compile_root(f"{name}.seed{index}", res, seed)
        ops.extend(o for o in seed_ops
                   if o[0] not in (OP_EMIT, OP_MATERIAL))
    walk(root, 0)
    flush()
    if light is not None:
        # Baked: the executor must never light these triangles again, so no
        # geometry word (its own or a seed's) sets or clears G_LIGHTING.
        ops = [(op, w0 | G_LIGHTING, w1 & ~G_LIGHTING, arg)
               if op == STATE_GEOMETRY else (op, w0, w1, arg)
               for op, w0, w1, arg in ops]
    if not groups:
        raise Refused(f"{name}: root {root:#x} draws nothing")
    if slots and slots != set(range(max(slots) + 1)):
        raise Refused(f"{name}: material slots {sorted(slots)} are not dense")
    return ops, groups, tuple(top_words), (max(slots) + 1) if slots else 0


def build(scene: bool = False):
    """The resident table (scene=False) or the overlay room table (True)."""
    loaded = {key: sm.load_o2r(REPO, spec) for key, spec in FILES.items()}
    roots = []
    all_ops = []
    all_groups = []
    for name, key, root, gobj_kind, note in ENTRIES:
        if (gobj_kind == GOBJ_SCENE) != scene:
            continue
        res = loaded[key]
        light = None
        if gobj_kind == GOBJ_SCENE:
            light = object_light(LIGHT_ANGLE_ROOM, room_dobj_rows(res, root))
        elif name in BATTLE_LIGHT_BAKES:
            light = object_light(BATTLE_LIGHT_BAKES[name])
        ops, groups, top, slots = compile_root(
            name, res, root, light, refuse_unbaked_lit=(gobj_kind == GOBJ_SCENE))
        if name in BATTLE_LIGHT_BAKES:
            ops = [GEOMETRY_CLEAR_LIGHTING] + ops + [GEOMETRY_SET_LIGHTING]
        shift = vertex_shift_for(groups)
        if shift:
            groups = [(tuple(tuple(shift_round(c, shift) for c in v[:3]) + v[3:]
                             for v in verts), tris)
                      for verts, tris in groups]
        if gobj_kind == GOBJ_WEAPON:
            ops = [(STATE_OTHERMODE, WEAPON_RENDER_MODE[0],
                    WEAPON_RENDER_MODE[1], 0)] + ops
        elif gobj_kind == GOBJ_GROUND_DISPLAY:
            mode = (GROUND_LINK1_RENDER_MODE if name in GROUND_LINK1_ROOTS
                    else GROUND_LINK0_RENDER_MODE)
            ops = [(STATE_OTHERMODE, mode[0], mode[1], 0)] + ops
        elif gobj_kind == GOBJ_SCENE:
            mode = (GROUND_LINK1_RENDER_MODE if name in SCENE_LINK1_ROOTS
                    else GROUND_LINK0_RENDER_MODE)
            ops = [(STATE_OTHERMODE, mode[0], mode[1], 0)] + ops
        flags = group_flags(ops)
        group_base = len(all_groups)
        roots.append(dict(
            name=name, asset=res.file_id, root=root, gobj=gobj_kind,
            note=note, first_op=len(all_ops), op_count=len(ops),
            slots=slots, top_words=len(top), shift=shift,
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


def render(roots, ops, groups, prefix="sNdsNativeBaked",
           counts="NDS_NATIVE_BAKED", storage="static ") -> str:
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
        lines.append(f"static const s16 {prefix}Verts{gi}[{len(verts) * 5}] =")
        lines.append("{")
        for v in verts:
            lines.append(f"    {v[0]}, {v[1]}, {v[2]}, {v[3]}, {v[4]},")
        lines.append("};")
        lines.append(f"static const u32 {prefix}Colors{gi}[{len(verts)}] =")
        lines.append("{")
        for v in verts:
            lines.append(f"    0x{v[5]:08x}u,")
        lines.append("};")
        lines.append(f"static const u16 {prefix}Tris{gi}[{len(tris) * 3}] =")
        lines.append("{")
        for t in tris:
            lines.append(f"    {t[0]}u, {t[1]}u, {t[2]}u,")
        lines.append("};")
        lines.append("")
    lines.append(f"{storage}const NDSNativeBakedGroup "
                 f"{prefix}Groups[{counts}_GROUP_COUNT] =")
    lines.append("{")
    for gi, (name, (verts, tris), gflags) in enumerate(groups):
        lines.append(f"    {{ {prefix}Verts{gi}, {prefix}Colors{gi}, "
                     f"{prefix}Tris{gi}, {len(verts)}u, {len(tris)}u, {gflags}u }},")
    lines.append("};")
    lines.append("")
    lines.append(f"{storage}const NDSNativeBakedOp "
                 f"{prefix}Ops[{counts}_OP_COUNT] =")
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
    lines.append(f"{storage}const NDSNativeBakedRoot "
                 f"{prefix}Roots[{counts}_ROOT_COUNT] =")
    lines.append("{")
    for r in roots:
        lines.append(f"    /* {r['name']}: {r['note']} */")
        lines.append(f"    {{ 0x{r['root']:05x}u, 0x{r['end']:05x}u, "
                     f"0x{r['last_w0']:08x}u, {r['asset']}u, {r['gobj']}u, "
                     f"{r['first_op']}u, {r['op_count']}u, {r['top_words']}u, "
                     f"{r['slots']}u, {r['shift']}u }},")
    lines.append("};")
    lines.append("")
    return "\n".join(lines)


def fnv1a(data: bytes) -> int:
    value = 2166136261
    for byte in data:
        value = ((value ^ byte) * 16777619) & 0xFFFFFFFF
    return value


def room_binary(roots, ops, groups) -> bytes:
    """The room table as the ending loads it: a 32-byte header (magic,
    version, root/op/group counts, data bytes, FNV-1a of the body, body
    bytes), then the roots, ops and groups exactly as their C structs
    (include/nds/nds_native_baked_types.h) lay out, a group's three pointers
    stored as data offsets the loader rebases, then the data: every colour
    (u32), every vertex (5 x s16), every triangle (3 x u16)."""
    colors = bytearray()
    verts = bytearray()
    tris = bytearray()
    rows = []
    for _name, (vs, ts), gflags in groups:
        rows.append((len(verts), len(colors), len(tris), len(vs), len(ts), gflags))
        for v in vs:
            colors += struct.pack("<I", v[5])
            verts += struct.pack("<5h", *v[:5])
        for t in ts:
            tris += struct.pack("<3H", *t)
    vert_base = len(colors)
    tri_base = vert_base + len(verts)
    data = bytes(colors) + bytes(verts) + bytes(tris)
    data += bytes(-len(data) % 4)
    body = bytearray()
    for r in roots:
        body += struct.pack("<IIIHHHHHBB", r["root"], r["end"], r["last_w0"],
                            r["asset"], r["gobj"], r["first_op"], r["op_count"],
                            r["top_words"], r["slots"], r["shift"])
    for op, w0, w1, arg, _name in ops:
        body += struct.pack("<IIHBB", w0, w1, arg, op, 0)
    for vert_off, color_off, tri_off, nverts, ntris, gflags in rows:
        if nverts > 0xFF or ntris > 0xFF:
            raise Refused("room group exceeds the u8 counts")
        body += struct.pack("<IIIBBH", vert_base + vert_off, color_off,
                            tri_base + tri_off, nverts, ntris, gflags)
    body += data
    header = struct.pack("<8I", ROOM_MAGIC, ROOM_VERSION, len(roots), len(ops),
                         len(groups), len(data), fnv1a(bytes(body)), len(body))
    return header + bytes(body)


def render_header(roots, ops, groups, room=None) -> str:
    assets = sorted({r["asset"] for r in roots})
    room_roots, room_ops, room_groups = room if room is not None else ([], [], [])
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
        "/* The ending's room table (nitro:/movies/room_baked.bin). */",
        f"#define NDS_NATIVE_BAKED_ROOM_MAGIC 0x{ROOM_MAGIC:08x}u",
        f"#define NDS_NATIVE_BAKED_ROOM_VERSION {ROOM_VERSION}u",
        "#define NDS_NATIVE_BAKED_ROOM_ASSET_MATCH(asset) \\",
        "    (" + (" || ".join(f"((asset) == {a}u)" for a in sorted({r['asset'] for r in room_roots})) or "0") + ")",
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
    ap.add_argument("--room-out", type=Path,
                    help="write the ending's room table (NitroFS) here")
    args = ap.parse_args()
    roots, ops, groups = build()
    room_roots, room_ops, room_groups = build(scene=True)
    if args.room_out is not None:
        blob = room_binary(room_roots, room_ops, room_groups)
        args.room_out.parent.mkdir(parents=True, exist_ok=True)
        if (not args.room_out.exists()) or args.room_out.read_bytes() != blob:
            args.room_out.write_bytes(blob)
        print(f"ROOM_BAKED_OK roots={len(room_roots)} ops={len(room_ops)} "
              f"groups={len(room_groups)} bytes={len(blob)}")
        if not (args.emit or args.check):
            return 0
    packet = render(roots, ops, groups)
    header = render_header(roots, ops, groups,
                           (room_roots, room_ops, room_groups))
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
    print(f"ITEM_BAKED_NATIVE_OK roots={len(roots)}+{len(room_roots)} ops={len(ops)}+{len(room_ops)} "
          f"groups={len(groups)} tris={tri_total} i_coverage={','.join(coverage) or '-'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
