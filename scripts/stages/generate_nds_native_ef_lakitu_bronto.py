#!/usr/bin/env python3
"""Lakitu + Bronto Burt efground native-actor packets: decode, emit, check.

Source: ExternDataBank106 (Castle geometry, file 106) for Lakitu plus
106_StageCastleFile2.c and ef/efground.c:27-106 (dEFGroundCastleEffectDescs);
ExternDataBank104 (Dream Land geometry, file 104) for Bronto Burt plus
104_StagePupupuFile2.c and efground.c:671-749 (dEFGroundPupupuEffectDescs).

Why this pair: smallest complete efground actors after measuring. Both are
single-quad-per-drawable textured birds with fully static DLs in their stage
geometry banks (no segment-E material program dependence at draw time beyond
Bronto's one live sprite-frame image):

- Lakitu: 6 joints (A root + e0 + e1 intermediate + 3 kind72 drawables),
  3 texture epochs
  (palette+image each), 12 verts, 6 tris. Joint-animated (TRAX/TRAY flight
  sweep + SCA flap, scripts 0xBB0/0xC14 R and 0xD00/0xD40 L). No MObj.
- Bronto: 3 joints (A root + e0 intermediate + 1 kind72 drawable), 1 palette epoch + 3 live
  sprite-frame images (MObjSub_0x3208 spritelink), 4 verts, 2 tris.
  The source's material animation tables leave the MObj-bearing node empty;
  texture_id_curr remains 0. Scripts 0x3448/0x36E8 animate DObj translation.

Modes (host-only, never builds, never touches shared outputs):
  --check        decode + verify committed packets/headers, print census
  --emit-packet  write src/nds/generated/nds_native_actor_ef_{lakitu,bronto}.generated.inc
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(REPO / "scripts"))
import _paths  # noqa: F401,E402
sys.path.insert(0, str(REPO / "scripts" / "stages"))
import generate_nds_native_stage as sm  # noqa: E402
from native_stage_descriptors.castle import DESCRIPTOR as CASTLE_DESC  # noqa: E402
from native_stage_descriptors.dreamland import DESCRIPTOR as DREAMLAND_DESC  # noqa: E402

CASTLE_TYPED = REPO / "decomp/BattleShip-main/decomp/src/relocData/106_StageCastleFile2.c"
PUPUPU_TYPED = REPO / "decomp/BattleShip-main/decomp/src/relocData/104_StagePupupuFile2.c"
LOGIC = REPO / "decomp/BattleShip-main/decomp/src/ef/efground.c"
LAKITU_PACKET = REPO / "src/nds/generated/nds_native_actor_ef_lakitu.generated.inc"
BRONTO_PACKET = REPO / "src/nds/generated/nds_native_actor_ef_bronto.generated.inc"
LAKITU_HEADER = REPO / "include/nds/nds_native_actor_ef_lakitu.h"
BRONTO_HEADER = REPO / "include/nds/nds_native_actor_ef_bronto.h"

# ---------------------------------------------------------------- Lakitu pins

# DL offsets (file 106) and word counts, from 106_StageCastleFile2.c.
LAK_DLS = {
    "prelude": (0x3F20, 12),   # state only + G_DL call into sub
    "sub": (0x3F80, 15),       # joint2's quad (called by prelude)
    "quad1": (0x3FF8, 15),     # joint3's quad
    "quad2": (0x4070, 21),     # joint4's quad + tail state restore
}
LAK_EXPECTED_OPS = {
    "prelude": (0xE7, 0xE3, 0xE2, 0xE2, 0xFC, 0xF9, 0xE8, 0xF5,
                0xD7, 0xD9, 0xDE, 0xDF),
    "sub": (0xE7, 0xF5, 0xF5, 0xFD, 0xE6, 0xF0, 0xE7, 0xF2,
            0xFD, 0xE6, 0xF3, 0xE7, 0x01, 0x06, 0xDF),
    "quad1": (0xE7, 0xF5, 0xF5, 0xFD, 0xE6, 0xF0, 0xE7, 0xF2,
              0xFD, 0xE6, 0xF3, 0xE7, 0x01, 0x06, 0xDF),
    "quad2": (0xE7, 0xF5, 0xF5, 0xFD, 0xE6, 0xF0, 0xE7, 0xF2,
              0xFD, 0xE6, 0xF3, 0xE7, 0x01, 0x06, 0xE7,
              0xE7, 0xD9, 0xE3, 0xE2, 0xE2, 0xDF),
}
# Per-run VTX pool offsets and texture epochs (palette, image, sizes).
# Joints: 0=A (tk1 root, gcAddDObjForGObj), 1=e0 (desc entry 0, NULL dl),
# 2=e1 (entry 1, NULL dl, live C14 scale flap), 3=e2 (entry 2, DL_0x3F20
# prelude calling the sub quad), 4=e3 (entry 3, DL_0x3FF8), 5=e4 (entry 4,
# gap DL at file 0x4070). e1 is a live NULL-dl intermediate (C14 SCAX/SCAY
# 1.0<->1.1): dropping it would lose e2's world scale, so all 6 stay live.
LAK_RUNS = (
    {"joint": 3, "dl": "sub", "vtx": 0x3E60,
     "pal": (0x36E8, 32), "img": (0x3DA0, 192)},
    {"joint": 4, "dl": "quad1", "vtx": 0x3EA0,
     "pal": (0x36C0, 32), "img": (0x3B98, 520)},
    {"joint": 5, "dl": "quad2", "vtx": 0x3EE0,
     "pal": (0x3698, 32), "img": (0x3710, 1160)},
)
LAK_PARENTS = (31, 0, 1, 2, 1, 1)
LAK_XOBJ_COUNTS = (1, 1, 1, 2, 2, 2)
# 28 = TraRotRpyRSca (A root, efground desc tk1), 27 = TraRotRpyR (children,
# desc tk2), 72 = 0x48 billboard (efground.c:1336, lr +/-1 arm, func_ovl0_800CAB48).
# The lr +/-3 arm (efground.c:1334, 0x2E = kind46, objdisplay.c:960) reuses
# the SAME DLs/geometry, so the packet below is class-independent; only the
# live XObj kind (72 vs 46) and the billboard rows differ at draw time.
LAK_XOBJ_KINDS = (28, 27, 27, 27, 72, 27, 72, 27, 72)
LAK_BINDINGS = (0, 1, 2, 3, 4, 5)

# ---------------------------------------------------------------- Bronto pins

BRONTO_DL_OFF, BRONTO_DL_WORDS = 0x32C8, 30
BRONTO_EXPECTED_OPS = (
    0xE7, 0xE3, 0xE2, 0xE2, 0xFC, 0xF9, 0xE8, 0xF5,
    0xF5, 0xF5, 0xFD, 0xE6, 0xF0, 0xE7, 0xD7, 0xF2,
    0xDE, 0xE6, 0xF3, 0xE7, 0xD9, 0x01, 0x06, 0xE7,
    0xE7, 0xD9, 0xE3, 0xE2, 0xE2, 0xDF,
)
BRONTO_HOOK_IDX = 16
BRONTO_PAL = (0x2D38, 32)
# MObjSub_0x3208 spritelink_0x31F8 order: {0x306C, 0x2EE4, 0x2D5C}.
BRONTO_FRAMES = ((0x306C, 388), (0x2EE4, 392), (0x2D5C, 392))
# Hook emits gDPSetTextureImage(CI, 16b, 1, sprites[curr]) per
# objdisplay.c:1421-1430 (flags ALPHA only); w0 matches the taru render-image
# precedent 0xFD500000.
BRONTO_HOOK_W0 = 0xFD500000
# Joints: 0=A (tk1 root, gcAddDObjForGObj, live spawn pose), 1=e0 (desc
# entry 0, NULL dl, carries the single MObj with source frame 0),
# 2=e1 (entry 1, DL_0x32C8, no MObj: the MObjSub table mobjlink_0x31F4 has
# exactly one entry, pairing node 0 of the e0 walk). e0 is a live NULL-dl
# intermediate: dropping it would lose e1's world offset (-2113, 32.8, 0).
BRONTO_PARENTS = (31, 0, 1)
BRONTO_XOBJ_COUNTS = (1, 1, 2)
BRONTO_XOBJ_KINDS = (28, 27, 27, 72)
BRONTO_BINDINGS = (0, 1, 2)

FAILURES: list[str] = []


def fail(msg: str) -> None:
    FAILURES.append(msg)


def require(cond: bool, msg: str) -> None:
    if not cond:
        fail(msg)


def load_bank(desc, key):
    entry = desc.o2r_inputs[key]
    spec = sm.InputSpec(entry["path"], entry["sha256"], entry.get("file_id"),
                        entry.get("internal_fixups"), entry.get("external_fixups"),
                        entry.get("payload_sha256"))
    return sm.load_o2r(REPO, spec)


def words_at(payload, off, cnt):
    return [struct.unpack_from(">II", payload, off + i * 8) for i in range(cnt)]


def walk_geometry(res, payload, off, cnt, tag, want_vtx, want_tris):
    """Shared VTX/TRI2 walk: one cache, dense verts, exact corners."""
    raw = words_at(payload, off, cnt)
    slots: dict[int, int] = {}
    dense_by_src: dict[int, int] = {}
    dense: list[tuple] = []
    tris: list[tuple[int, int, int]] = []
    vtx_loads: list[tuple[int, int, int]] = []
    for idx, (w0, w1) in enumerate(raw):
        op = w0 >> 24
        if op == sm.OP_VTX:
            count = (w0 >> 12) & 0xFF
            v0 = ((w0 >> 1) & 0x7F) - count
            ref = res.pointer_at(off + idx * 8 + 4)
            require(ref is not None and ref.asset_id == res.file_id,
                    f"{tag} word {idx}: VTX source outside bank")
            if ref is None:
                continue
            for s in range(count):
                v = sm.decode_vertex(res, ref.offset + s * 16)
                key = ref.offset + s * 16
                if key not in dense_by_src:
                    dense_by_src[key] = len(dense)
                    dense.append(v)
                slots[v0 + s] = dense_by_src[key]
            vtx_loads.append((count, v0, ref.offset))
        elif op in (sm.OP_TRI1, sm.OP_TRI2):
            for corner in sm.decode_triangles(op, w0, w1):
                try:
                    tris.append(tuple(slots[s] for s in corner))
                except KeyError:
                    fail(f"{tag} word {idx}: triangle uses unloaded cache slot")
    require(vtx_loads == want_vtx, f"{tag} VTX census changed: {vtx_loads}")
    require(len(tris) == want_tris, f"{tag} tri count changed: {len(tris)}")
    return raw, dense, tris, vtx_loads


def decode_lakitu():
    res = load_bank(CASTLE_DESC, "stage_geometry")
    payload = res.payload
    decoded = {}
    for name, (off, cnt) in LAK_DLS.items():
        raw = words_at(payload, off, cnt)
        ops = tuple(w0 >> 24 for w0, _ in raw)
        require(ops == LAK_EXPECTED_OPS[name],
                f"lakitu {name} op sequence changed: {[hex(o) for o in ops]}")
        decoded[name] = raw
    require(decoded["prelude"][10][0] >> 24 == 0xDE, "prelude word 10 must be G_DL")
    require(decoded["prelude"][11][0] >> 24 == 0xDF, "prelude must end EndDL")
    # Prelude G_DL target is the sub quad DL (relocated; same-bank check).
    # (Target address check happens against the typed macro below.)
    for name, run in (("sub", LAK_RUNS[0]), ("quad1", LAK_RUNS[1]),
                      ("quad2", LAK_RUNS[2])):
        raw = decoded[name]
        vtx_idx = 12
        require(raw[vtx_idx][0] >> 24 == 0x01, f"{name} word 12 must be VTX")
        ref = res.pointer_at(LAK_DLS[name][0] + vtx_idx * 8 + 4)
        require(ref is not None and ref.asset_id == 106
                and ref.offset == run["vtx"],
                f"{name} VTX pool must be bank offset {run['vtx']:#x}")
        img_idx = 8
        img_ref = res.pointer_at(LAK_DLS[name][0] + img_idx * 8 + 4)
        require(img_ref is not None and img_ref.asset_id == 106
                and img_ref.offset == run["img"][0],
                f"{name} render image must be bank offset {run['img'][0]:#x}")
        pal_idx = 3
        pal_ref = res.pointer_at(LAK_DLS[name][0] + pal_idx * 8 + 4)
        pal_off = {"sub": 0x36E8, "quad1": 0x36C0, "quad2": 0x3698}[name]
        require(pal_ref is not None and pal_ref.asset_id == 106
                and pal_ref.offset == pal_off,
                f"{name} palette must be bank offset {pal_off:#x}")
    # Geometry walk per quad DL (single 4-vert pool, one TRI2 = 2 tris).
    quads = {}
    for name, run in (("sub", LAK_RUNS[0]), ("quad1", LAK_RUNS[1]),
                      ("quad2", LAK_RUNS[2])):
        off, cnt = LAK_DLS[name]
        _raw, dense, tris, loads = walk_geometry(
            res, payload, off, cnt, f"lakitu-{name}",
            [(4, 0, run["vtx"])], 2)
        require(len(dense) == 4, f"lakitu-{name} must have 4 verts")
        quads[name] = (dense, tris)
    # Quad1/quad2/sub TRI2 corners are exact per-DL (winding differs on quad2).
    require(quads["sub"][1] == [(3, 2, 1), (0, 3, 1)], "sub tris changed")
    require(quads["quad1"][1] == [(3, 2, 1), (0, 3, 1)], "quad1 tris changed")
    require(quads["quad2"][1] == [(3, 2, 1), (2, 0, 1)], "quad2 tris changed")
    # Typed-source pins: DObjDesc ids/translates, anim script census, DL macros.
    CASTLE_DL_DIR = (REPO / "decomp/BattleShip-main/decomp/build/us/src"
                     / "relocData/StageCastleFile2")
    dl_text = (CASTLE_DL_DIR / "DL_0x3F20.dl.inc.c").read_text()
    dl_text += (CASTLE_DL_DIR / "DL_0x3F80.dl.inc.c").read_text()
    dl_text += (CASTLE_DL_DIR / "DL_0x3FF8.dl.inc.c").read_text()
    dl_text += (CASTLE_DL_DIR / "gap_0x3684_sub_0x9EC.dl.inc.c").read_text()
    for tok in ("gsSPDisplayList((Gfx *)dStageCastleFile2_DL_0x3F80)",
                "gsSPVertex((Vtx *)dStageCastleFile2_gap_0x3684_sub_0x7DC, 4, 0)",
                "gsSPVertex((Vtx *)dStageCastleFile2_gap_0x3684_sub_0x81C, 4, 0)",
                "gsSPVertex((Vtx *)dStageCastleFile2_gap_0x3684_sub_0x85C, 4, 0)",
                "gsSP2Triangles(3, 2, 1, 0, 0, 3, 1, 0)",
                "gsSP2Triangles(3, 2, 1, 0, 2, 0, 1, 0)"):
        require(tok in dl_text, f"StageCastleFile2 DL inc missing {tok!r}")
    text = CASTLE_TYPED.read_text()
    for tok in ("{ 0, (void*)0x00000000, { -3000.0f, 0.0f, 0.0f }",
                "{ 1, (void*)0x00000000, { 30.0f, 0.0f, -30.0f }",
                "{ 0x4002, (void*)dStageCastleFile2_DL_0x3F20",
                "{ 0x4001, (void*)dStageCastleFile2_DL_0x3FF8",
                "{ 0x4001, (void*)dStageCastleFile2_gap_0x3684_sub_0x9EC",
                "{ 18, (void*)0x00000000",
                "dStageCastleFile2_gap_0x3684_sub_0x9EC_post[6]",
                "dStageCastleFile2_gap_0x3684_sub_0x9EC_post_joint[5]",
                "u32 dStageCastleFile2_gap_0x3684_sub_0xBB0[24]",
                "u32 dStageCastleFile2_gap_0x3684_sub_0xC14[54]",
                "u32 dStageCastleFile2_gap_0x3684_sub_0xD00[16]",
                "u32 dStageCastleFile2_gap_0x3684_sub_0xD40[84]",
                "u8 dStageCastleFile2_Tex_0x3710[1160]",
                "u8 dStageCastleFile2_Tex_0x3B98[520]",
                "u8 dStageCastleFile2_Tex_0x3DA0[192]"):
        require(tok in text, f"106_StageCastleFile2.c missing {tok!r}")
    logic = LOGIC.read_text()
    for tok in ("dEFGroundCastleEffectDescs",
                "&llGRCastleMapLakituDObjDesc",
                "&llGRCastleMapLakituRAnimJoint",
                "&llGRCastleMapLakituLAnimJoint",
                "efGroundCommonProcUpdate",
                "efGroundSetupEffectDObjs",
                "lr_bool",
                "gcAddXObjForDObjFixed(current_dobj, 0x2E, 0)",
                "gcAddXObjForDObjFixed(current_dobj, 0x48, 0)",
                "nGCMatrixKindTraRotRpyRSca",
                "nGCMatrixKindTraRotRpyR"):
        require(tok in logic, f"efground.c missing {tok!r}")
    return {"quads": quads, "words": decoded}


def decode_bronto():
    res = load_bank(DREAMLAND_DESC, "stage_geometry")
    payload = res.payload
    raw = words_at(payload, BRONTO_DL_OFF, BRONTO_DL_WORDS)
    ops = tuple(w0 >> 24 for w0, _ in raw)
    require(ops == BRONTO_EXPECTED_OPS,
            f"bronto op sequence changed: {[hex(o) for o in ops]}")
    require(raw[BRONTO_HOOK_IDX][0] >> 24 == 0xDE,
            "word 16 must be the material hook")
    require(raw[BRONTO_DL_WORDS - 1][0] >> 24 == 0xDF, "last word must be EndDL")
    _raw, dense, tris, loads = walk_geometry(
        res, payload, BRONTO_DL_OFF, BRONTO_DL_WORDS, "bronto",
        [(4, 0, 0x3288)], 2)
    require(len(dense) == 4, f"bronto verts changed: {len(dense)}")
    require(tris == [(3, 2, 1), (0, 3, 1)], f"bronto tris changed: {tris}")
    pal_ref = res.pointer_at(BRONTO_DL_OFF + 10 * 8 + 4)
    require(pal_ref is not None and pal_ref.asset_id == 104
            and pal_ref.offset == BRONTO_PAL[0],
            "bronto palette must be bank offset 0x2D38")
    # Exactly one static SETTIMG (the palette load); the render image comes
    # from the MObj hook, never from a second static word.
    settimgs = [i for i, (w0, _w1) in enumerate(raw) if (w0 >> 24) == 0xFD]
    require(settimgs == [10], f"bronto static SETTIMG census changed: {settimgs}")
    # Render tile (third SETTILE, word 9) is CI4; tile extent pinned.
    w9 = raw[9]
    fmt, siz = (w9[0] >> 21) & 7, (w9[0] >> 19) & 3
    require((fmt, siz) == (2, 0), f"bronto render tile not CI4: fmt={fmt} siz={siz}")
    require(raw[15][1] == 0x0007C05C,
            f"bronto tile extent changed: 0x{raw[15][1]:08x}")
    text = PUPUPU_TYPED.read_text()
    for tok in ("DObjDesc dStagePupupuFile2_Layer3Anim_DObjDesc_0x33B8[3]",
                "{ 16385, (void *)dStagePupupuFile2_Layer3Anim_DL_0x32C8",
                "MObjSub dStagePupupuFile2_Layer3Anim_MObjSub_0x3208[1]",
                "dStagePupupuFile2_Layer3Anim_spritelink_0x31F8[4]",
                "dStagePupupuFile2_Tex_0x2D5C[392]",
                "dStagePupupuFile2_Tex_0x2EE4[392]",
                "dStagePupupuFile2_Tex_0x306C[388]",
                "u32 dStagePupupuFile2_Layer3Anim_MatAnim_0x3448[36]",
                "u32 dStagePupupuFile2_Layer3Anim_MatAnim_0x34E8[124]",
                "u32 dStagePupupuFile2_Layer3Anim_MatAnim_0x36E8[46]",
                "u32 dStagePupupuFile2_Layer3Anim_MatAnim_0x37A8[109]",
                "Gfx dStagePupupuFile2_Layer3Anim_DL_0x32C8[30]",
                "Vtx dStagePupupuFile2_Layer3Anim_Vtx_0x3288[4]",
                "0x0001,\n\t\tG_IM_FMT_CI, G_IM_SIZ_4b,"):
        require(tok in text, f"104_StagePupupuFile2.c missing {tok!r}")
    logic = LOGIC.read_text()
    for tok in ("&llGRPupupuMapBrontoDObjDesc",
                "&llGRPupupuMapBrontoMObjSub",
                "&llGRPupupuMapBrontoLAnimJoint",
                "&llGRPupupuMapBrontoRAnimJoint",
                "gcAddXObjForDObjFixed(current_dobj, 0x2E, 0)",
                "gcAddXObjForDObjFixed(current_dobj, 0x48, 0)",
                "lr_bool",
                "dEFGroundPupupuEffectDescs"):
        require(tok in logic, f"efground.c missing {tok!r}")
    return {"words": raw, "dense": dense, "tris": tris}


def state_stmt(op, w0, w1, asset_off=None):
    h = lambda v: f"0x{v:08x}u"  # noqa: E731
    if op in (0xE7, 0xE6, 0xE8, 0xDF):
        return None  # pipe/load/tile syncs + EndDL: no DS state
    if op in (0xE3, 0xE2):
        return f"ndsRendererRecordOtherMode(stats, 0x{op:02x}u, {h(w0)}, {h(w1)});"
    if op == 0xFC:
        return f"ndsRendererRecordSetCombine(stats, {h(w0)}, {h(w1)});"
    if op == 0xFB:
        return f"stats->env_color = {h(w1)};"
    if op == 0xF9:
        return f"stats->blend_color = {h(w1)};"
    if op == 0xF5:
        return f"ndsRendererRecordSetTile(stats, {h(w0)}, {h(w1)});"
    if op == 0xD7:
        return f"ndsRendererRecordTextureState(stats, {h(w0)}, {h(w1)});"
    if op == 0xF2:
        return f"ndsRendererRecordSetTileSize(stats, {h(w0)}, {h(w1)});"
    if op == 0xFD:
        return (f"ndsRendererRecordSetImage(stats, {h(w0)}, "
                f"(u32)(uintptr_t)(asset_base + 0x{asset_off:04x}u));")
    if op == 0xF0:
        return f"ndsRendererRecordLoadTlut(stats, {h(w1)});"
    if op == 0xF3:
        return f"ndsRendererRecordLoadBlock(stats, {h(w0)}, {h(w1)});"
    if op == 0xD9:
        return (f"stats->geometry_mode = (stats->geometry_mode & {h(w0)}) | {h(w1)};")
    fail(f"unsupported state opcode 0x{op:02x}")
    return None


def emit_state_block(L, words, tex_offs):
    for w0, w1 in words:
        op = w0 >> 24
        off = tex_offs.get((w0, w1))
        s = state_stmt(op, w0, w1, asset_off=off)
        if s is not None:
            L.append(f"    {s}")


def render_lakitu(p):
    L = []
    A = L.append
    A("/* Castle Lakitu efground native actor packet (generated).")
    A(" *")
    A(" * Source: ExternDataBank106 payload (castle.py stage_geometry pins)")
    A(" * plus relocData/106_StageCastleFile2.c and ef/efground.c:27-106.")
    A(" * 6 live joints (A root + e0/e1 intermediates + 3 kind72 drawables), 3")
    A(" * texture epochs, 12 verts, 6 tris. e2's state prelude (DL_0x3F20) calls")
    A(" * the sub quad DL_0x3F80 inline; quad1/quad2 follow in draw order.")
    A(" * Do not hand-edit: regenerate with")
    A(" *   python scripts/stages/generate_nds_native_ef_lakitu_bronto.py --emit-packet")
    A(" */")
    A("static const u8 sNdsNativeActorEfLakituJointParents[6] =")
    A("{")
    for v in LAK_PARENTS:
        A(f"    {v}u,")
    A("};")
    A("")
    A("static const u8 sNdsNativeActorEfLakituJointXObjCounts[6] =")
    A("{")
    for v in LAK_XOBJ_COUNTS:
        A(f"    {v}u,")
    A("};")
    A("")
    A("static const u8 sNdsNativeActorEfLakituJointXObjKinds[9] =")
    A("{")
    for v in LAK_XOBJ_KINDS:
        A(f"    {v}u,")
    A("};")
    A("")
    A("static const u8 sNdsNativeActorEfLakituBindings[6] =")
    A("{")
    for v in LAK_BINDINGS:
        A(f"    {v}u,")
    A("};")
    A("")
    A("static const u16 sNdsNativeActorEfLakituRuns[9] =")
    A("{")
    for i, r in enumerate(LAK_RUNS):
        A(f"    {r['joint']}u, {i * 2}u, 2u,")
    A("};")
    A("")
    # Shared dense corners: per-run vert bases 0/4/8, exact per-DL windings.
    corners = [3, 2, 1, 0, 3, 1,
               7, 6, 5, 4, 7, 5,
               11, 10, 9, 10, 8, 9]
    dense = []
    for name in ("sub", "quad1", "quad2"):
        dense.extend(p["quads"][name][0])
    require(len(dense) == 12, f"lakitu dense verts changed: {len(dense)}")
    for d, t in zip(dense, [(180, -191, -30, 1024, 0),
                            (-120, -191, -30, 0, 0),
                            (-120, 19, -30, 0, 768),
                            (180, 19, -30, 1024, 768),
                            (120, 189, 0, 1024, 1024),
                            (120, -51, 0, 1024, 0),
                            (-120, -51, 0, 0, 0),
                            (-120, 189, 0, 0, 1024),
                            (-330, 210, 30, 0, 1536),
                            (-330, -150, 30, 0, 0),
                            (-30, 210, 30, 1280, 1536),
                            (-30, -150, 30, 1280, 0)]):
        require(tuple(d[:5]) == t, f"lakitu vert changed: {d[:5]} != {t}")
        require(d[5] == 0xFFFFFFFF, f"lakitu vert color changed: {hex(d[5])}")
    A("static const u16 sNdsNativeActorEfLakituTriIndices[18] =")
    A("{")
    for d in corners:
        A(f"    {d}u,")
    A("};")
    A("")
    A("static const s16 sNdsNativeActorEfLakituVerts[60] =")
    A("{")
    for v in dense:
        A(f"    {v[0]}, {v[1]}, {v[2]}, {v[3]}, {v[4]},")
    A("};")
    A("")
    A("static const u32 sNdsNativeActorEfLakituVertColors[12] =")
    A("{")
    for v in dense:
        A(f"    0x{v[5]:08x}u,")
    A("};")
    A("")
    A("static const u32 sNdsNativeActorEfLakituTextureEpochs[12] =")
    A("{")
    for r in LAK_RUNS:
        A(f"    0x{r['pal'][0]:08x}u, {r['pal'][1]}u, "
          f"0x{r['img'][0]:08x}u, {r['img'][1]}u,")
    A("};")
    A("")
    prelude = p["words"]["prelude"][:10]
    sub_ep = [w for w in p["words"]["sub"][:12] if (w[0] >> 24) not in (0xE7, 0xE6, 0xE8)]
    q1_ep = [w for w in p["words"]["quad1"][:12] if (w[0] >> 24) not in (0xE7, 0xE6, 0xE8)]
    q2_ep = [w for w in p["words"]["quad2"][:12] if (w[0] >> 24) not in (0xE7, 0xE6, 0xE8)]
    finish = [w for w in p["words"]["quad2"][14:] if (w[0] >> 24) not in (0xE7, 0xE6, 0xE8, 0xDF)]
    require(len(finish) == 4, f"lakitu finish words changed: {len(finish)}")
    # Map each epoch SETTIMG word to its image/palette offset explicitly.
    A("static void ndsNativeActorEfLakituSetupPrelude(")
    A("    NDSRendererStats *stats, const u8 *asset_base)")
    A("{")
    A("    (void)asset_base;")
    emit_state_block(L, [w for w in prelude if (w[0] >> 24) not in (0xE7, 0xE6, 0xE8, 0xDF, 0xDE)], {})
    A("}")
    for i, (ep, r) in enumerate(zip((sub_ep, q1_ep, q2_ep), LAK_RUNS)):
        A("")
        A(f"static void ndsNativeActorEfLakituSetupEpoch{i}(")
        A("    NDSRendererStats *stats, const u8 *asset_base)")
        A("{")
        A("    (void)asset_base;")
        fd_seen = 0
        for w0, w1 in ep:
            op = w0 >> 24
            if op == 0xFD:
                # First SETTIMG in the epoch head is the palette load
                # (word 3), the second is the render image (word 8).
                off = r["pal"][0] if fd_seen == 0 else r["img"][0]
                fd_seen += 1
                s = state_stmt(op, w0, w1, asset_off=off)
            else:
                s = state_stmt(op, w0, w1)
            if s is not None:
                A(f"    {s}")
        require(fd_seen == 2, f"lakitu epoch{i} SETTIMG census changed")
        A("}")
    A("")
    A("static void ndsNativeActorEfLakituFinishState(")
    A("    NDSRendererStats *stats, const u8 *asset_base)")
    A("{")
    A("    (void)asset_base;")
    emit_state_block(L, finish, {})
    A("}")
    slab = (6 + 6 + 9 + 6 + 9 * 2 + 18 * 2 + 12 * 14 + 12 * 4
            + (len(prelude) + len(sub_ep) + len(q1_ep) + len(q2_ep)
               + len(finish)) * 8)
    A("")
    A(f"/* counts: joints=6 drawables=3 runs=3 triangles=6 verts=12 slab={slab} */")
    return "\n".join(L) + "\n", slab


def render_bronto(p):
    L = []
    A = L.append
    A("/* Dream Land Bronto Burt efground native actor packet (generated).")
    A(" *")
    A(" * Source: ExternDataBank104 payload (dreamland.py stage_geometry pins)")
    A(" * plus relocData/104_StagePupupuFile2.c and ef/efground.c:671-749.")
    A(" * 3 live joints (A root + e0 intermediate + kind72 drawable e1),")
    A(" * 1 palette epoch, 3 live")
    A(" * sprite-frame images (MObjSub_0x3208 spritelink, TEXID flap), 4")
    A(" * verts, 2 tris. The word-16 material hook is replaced by the live")
    A(" * frame image; every other word replays verbatim.")
    A(" * Do not hand-edit: regenerate with")
    A(" *   python scripts/stages/generate_nds_native_ef_lakitu_bronto.py --emit-packet")
    A(" */")
    A("static const u8 sNdsNativeActorEfBrontoJointParents[3] =")
    A("{")
    for v in BRONTO_PARENTS:
        A(f"    {v}u,")
    A("};")
    A("")
    A("static const u8 sNdsNativeActorEfBrontoJointXObjCounts[3] =")
    A("{")
    for v in BRONTO_XOBJ_COUNTS:
        A(f"    {v}u,")
    A("};")
    A("")
    A("static const u8 sNdsNativeActorEfBrontoJointXObjKinds[4] =")
    A("{")
    for v in BRONTO_XOBJ_KINDS:
        A(f"    {v}u,")
    A("};")
    A("")
    A("static const u8 sNdsNativeActorEfBrontoBindings[3] =")
    A("{")
    for v in BRONTO_BINDINGS:
        A(f"    {v}u,")
    A("};")
    A("")
    A("static const u16 sNdsNativeActorEfBrontoRuns[3] =")
    A("{")
    A("    2u, 0u, 2u,")
    A("};")
    A("")
    corners = [d for t in p["tris"] for d in t]
    require(corners == [3, 2, 1, 0, 3, 1], f"bronto corners changed: {corners}")
    A("static const u16 sNdsNativeActorEfBrontoTriIndices[6] =")
    A("{")
    for d in corners:
        A(f"    {d}u,")
    A("};")
    A("")
    for d, t in zip(p["dense"], [(150, -120, 0, 1024, 0),
                                 (-150, -120, 0, 0, 0),
                                 (-150, 120, 0, 0, 768),
                                 (150, 120, 0, 1024, 768)]):
        require(tuple(d[:5]) == t, f"bronto vert changed: {d[:5]} != {t}")
        require(d[5] == 0xFFFFFFFF, f"bronto vert color changed: {hex(d[5])}")
    A("static const s16 sNdsNativeActorEfBrontoVerts[20] =")
    A("{")
    for v in p["dense"]:
        A(f"    {v[0]}, {v[1]}, {v[2]}, {v[3]}, {v[4]},")
    A("};")
    A("")
    A("static const u32 sNdsNativeActorEfBrontoVertColors[4] =")
    A("{")
    for v in p["dense"]:
        A(f"    0x{v[5]:08x}u,")
    A("};")
    A("")
    A("static const u32 sNdsNativeActorEfBrontoTextureEpoch[8] =")
    A("{")
    A(f"    0x{BRONTO_PAL[0]:08x}u, {BRONTO_PAL[1]}u,")
    for off, size in BRONTO_FRAMES:
        A(f"    0x{off:08x}u, {size}u,")
    A("};")
    A("")
    raw = p["words"]
    setup_a = [w for w in raw[:BRONTO_HOOK_IDX]
               if (w[0] >> 24) not in (0xE7, 0xE6, 0xE8)]
    # Words 17..20: load sync (skip), LoadBlock, pipe sync (skip), then the
    # geometry-mode set that arms the quad (mirrors the taru setup tail).
    setup_b = [w for w in raw[BRONTO_HOOK_IDX + 1:BRONTO_HOOK_IDX + 5]
               if (w[0] >> 24) not in (0xE7, 0xE6, 0xE8)]
    # Words 23..28: syncs (skip), geometry restore, othermode restore.
    finish = [w for w in raw[23:29]
              if (w[0] >> 24) not in (0xE7, 0xE6, 0xE8, 0xDF)]
    require(len(setup_a) == 12, f"bronto setup_a changed: {len(setup_a)}")
    require(len(setup_b) == 2, f"bronto setup_b changed: {len(setup_b)}")
    require(len(finish) == 4, f"bronto finish changed: {len(finish)}")
    A("static void ndsNativeActorEfBrontoSetupState(")
    A("    NDSRendererStats *stats, const u8 *asset_base)")
    A("{")
    A("    (void)asset_base;")
    for w0, w1 in setup_a:
        if (w0 >> 24) == 0xFD:
            s = state_stmt(w0 >> 24, w0, w1, asset_off=BRONTO_PAL[0])
        else:
            s = state_stmt(w0 >> 24, w0, w1)
        if s is not None:
            A(f"    {s}")
    A("}")
    A("")
    A("static void ndsNativeActorEfBrontoSetupTail(")
    A("    NDSRendererStats *stats, const u8 *asset_base)")
    A("{")
    A("    (void)asset_base;")
    emit_state_block(L, setup_b, {})
    A("}")
    A("")
    A("static void ndsNativeActorEfBrontoFinishState(")
    A("    NDSRendererStats *stats, const u8 *asset_base)")
    A("{")
    A("    (void)asset_base;")
    emit_state_block(L, finish, {})
    A("}")
    slab = (3 + 3 + 4 + 3 + 3 * 2 + 6 * 2 + 4 * 14 + 8 * 4
            + (len(setup_a) + len(setup_b) + len(finish)) * 8)
    A("")
    A(f"/* counts: joints=3 drawables=1 runs=1 triangles=2 verts=4 slab={slab} */")
    return "\n".join(L) + "\n", slab, setup_b


LAKITU_HEADER_WANTS = {
    "NDS_NATIVE_ACTOR_EF_LAKITU_JOINT_COUNT": 6,
    "NDS_NATIVE_ACTOR_EF_LAKITU_DRAWABLE_COUNT": 3,
    "NDS_NATIVE_ACTOR_EF_LAKITU_RUN_COUNT": 3,
    "NDS_NATIVE_ACTOR_EF_LAKITU_TRIANGLE_COUNT": 6,
    "NDS_NATIVE_ACTOR_EF_LAKITU_VERT_COUNT": 12,
    "NDS_NATIVE_ACTOR_EF_LAKITU_CORNER_COUNT": 18,
    "NDS_NATIVE_ACTOR_EF_LAKITU_STATE_PRELUDE": 8,
    "NDS_NATIVE_ACTOR_EF_LAKITU_STATE_EPOCH": 7,
    "NDS_NATIVE_ACTOR_EF_LAKITU_STATE_FINISH": 4,
    "NDS_NATIVE_ACTOR_EF_LAKITU_XOBJ_KIND_TRA_ROTRPYRSCA": 28,
    "NDS_NATIVE_ACTOR_EF_LAKITU_XOBJ_KIND_TRA_ROTRPYR": 27,
    "NDS_NATIVE_ACTOR_EF_LAKITU_XOBJ_KIND_BILLBOARD": 72,
    "NDS_NATIVE_ACTOR_EF_LAKITU_XOBJ_KIND_BILLBOARD_46": 46,
}

BRONTO_HEADER_WANTS = {
    "NDS_NATIVE_ACTOR_EF_BRONTO_JOINT_COUNT": 3,
    "NDS_NATIVE_ACTOR_EF_BRONTO_DRAWABLE_COUNT": 1,
    "NDS_NATIVE_ACTOR_EF_BRONTO_RUN_COUNT": 1,
    "NDS_NATIVE_ACTOR_EF_BRONTO_TRIANGLE_COUNT": 2,
    "NDS_NATIVE_ACTOR_EF_BRONTO_VERT_COUNT": 4,
    "NDS_NATIVE_ACTOR_EF_BRONTO_CORNER_COUNT": 6,
    "NDS_NATIVE_ACTOR_EF_BRONTO_FRAME_COUNT": 3,
    "NDS_NATIVE_ACTOR_EF_BRONTO_STATE_SETUP_A": 12,
    "NDS_NATIVE_ACTOR_EF_BRONTO_STATE_SETUP_B": 2,
    "NDS_NATIVE_ACTOR_EF_BRONTO_STATE_FINISH": 4,
    "NDS_NATIVE_ACTOR_EF_BRONTO_XOBJ_KIND_TRA_ROTRPYRSCA": 28,
    "NDS_NATIVE_ACTOR_EF_BRONTO_XOBJ_KIND_TRA_ROTRPYR": 27,
    "NDS_NATIVE_ACTOR_EF_BRONTO_XOBJ_KIND_BILLBOARD": 72,
    "NDS_NATIVE_ACTOR_EF_BRONTO_XOBJ_KIND_BILLBOARD_46": 46,
}


def check_header(path, wants):
    import re
    text = path.read_text()
    for name, want in wants.items():
        m = re.search(rf"#define\s+{name}\s+(0x[0-9a-fA-F]+|\d+)u?", text)
        require(m is not None and int(m.group(1), 0) == want,
                f"header: {name} must be {want}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--emit-packet", action="store_true")
    args = ap.parse_args()
    lak = decode_lakitu()
    bro = decode_bronto()
    lak_text, lak_slab = render_lakitu(lak)
    bro_text, bro_slab, _setup_b = render_bronto(bro)
    print(f"ef-lakitu: joints=6 drawables=3 dl_words=12+15+15+21 src_verts=12 "
          f"tris=6 runs=3 epochs=3 slab={lak_slab}")
    print(f"ef-bronto: joints=3 drawables=1 dl_words=30 src_verts=4 tris=2 "
          f"runs=1 frames=3 slab={bro_slab}")
    if args.emit_packet:
        LAKITU_PACKET.parent.mkdir(parents=True, exist_ok=True)
        LAKITU_PACKET.write_text(lak_text)
        BRONTO_PACKET.write_text(bro_text)
        print(f"emitted {LAKITU_PACKET}")
        print(f"emitted {BRONTO_PACKET}")
        return 0 if not FAILURES else 1
    if LAKITU_PACKET.is_file():
        require(LAKITU_PACKET.read_text() == lak_text,
                "lakitu packet drifted: re-run --emit-packet + review")
    else:
        fail("lakitu packet absent: run --emit-packet once, then --check")
    if BRONTO_PACKET.is_file():
        require(BRONTO_PACKET.read_text() == bro_text,
                "bronto packet drifted: re-run --emit-packet + review")
    else:
        fail("bronto packet absent: run --emit-packet once, then --check")
    if LAKITU_HEADER.is_file():
        check_header(LAKITU_HEADER, LAKITU_HEADER_WANTS)
    else:
        fail("lakitu header absent")
    if BRONTO_HEADER.is_file():
        check_header(BRONTO_HEADER, BRONTO_HEADER_WANTS)
    else:
        fail("bronto header absent")
    if FAILURES:
        print("FAIL:")
        for f in FAILURES:
            print(f"  - {f}")
        return 1
    print("ef-lakitu-bronto --check: green (host/source only, no ROM)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
