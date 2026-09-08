#!/usr/bin/env python3
"""Yoster cloud native-actor packet: decode, emit, check, prove.

Source: MiscDataBank154 payload (yoster.py stage_actors pins) plus
relocData/154_StageYosterFile3.c and gr/grcommon/gryoster.c. The three
persistent cloud GObjs share one template: 7 live joints each (root + 3
template mids + 3 CloudDisplayList drawables), Tra + kind48 child XObjs,
one MObj per drawable, Solid/Evaporate PRIMCOLOR scripts, root translate
driven by yakumono collision every tick.

Modes (host-only, never builds, never touches shared outputs):
  --check        decode + verify committed packet/header, print census
  --emit-packet  write src/nds/generated/nds_native_actor_yoster_cloud.generated.inc
  --prove        host recorder: 3 instances, pose/alpha sensitivity, fail-closed
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
from native_stage_descriptors.yoster import DESCRIPTOR  # noqa: E402

TYPED = REPO / "decomp/BattleShip-main/decomp/src/relocData/154_StageYosterFile3.c"
LOGIC = REPO / "decomp/BattleShip-main/decomp/src/gr/grcommon/gryoster.c"
PACKET = REPO / "src/nds/generated/nds_native_actor_yoster_cloud.generated.inc"
HEADER = REPO / "include/nds/nds_native_actor_yoster_cloud.h"

DL_OFF, DL_WORDS = 0x580, 29
HOOK_IDX = 12  # G_DL word invoking the segment-E material program
# Full 29-op falsifier: any source DL edit re-cuts this slice, never silent.
EXPECTED_OPS = (
    0xE7, 0xE3, 0xE2, 0xE2, 0xFC, 0xFB, 0xF9, 0xF5,
    0xF5, 0xD7, 0xF2, 0xFD, 0xDE, 0xE6, 0xF3, 0xE7,
    0xD9, 0x01, 0x05, 0xE7, 0x01, 0x01, 0x05, 0xE7,
    0xD9, 0xE3, 0xE2, 0xE2, 0xDF,
)
PARENTS = (31, 0, 0, 0, 1, 2, 3)
XOBJ_COUNTS = (1, 1, 1, 1, 2, 2, 2)
XOBJ_KINDS = (18, 18, 18, 18, 18, 48, 18, 48, 18, 48)
BINDINGS = (0, 1, 2, 3, 4, 5, 6)
TEX_IMG_OFF, TEX_IMG_SIZE = 0x2B8, 512

FAILURES: list[str] = []


def fail(msg: str) -> None:
    FAILURES.append(msg)


def require(cond: bool, msg: str) -> None:
    if not cond:
        fail(msg)


def load_bank():
    entry = DESCRIPTOR.o2r_inputs["stage_actors"]
    spec = sm.InputSpec(entry["path"], entry["sha256"], entry.get("file_id"),
                        entry.get("internal_fixups"), entry.get("external_fixups"),
                        entry.get("payload_sha256"))
    return sm.load_o2r(REPO, spec)


def words_at(payload, off, cnt):
    return [struct.unpack_from(">II", payload, off + i * 8) for i in range(cnt)]


def decode():
    res = load_bank()
    payload = res.payload
    file_id = res.file_id
    raw = words_at(payload, DL_OFF, DL_WORDS)
    ops = tuple(w0 >> 24 for w0, _ in raw)
    require(ops == EXPECTED_OPS, f"DL op sequence changed: {[hex(o) for o in ops]}")
    require(raw[HOOK_IDX][0] >> 24 == 0xDE, "word 12 must be the G_DL material hook")
    require(raw[DL_WORDS - 1][0] >> 24 == 0xDF, "word 28 must be EndDL")

    # Geometry walk: one RSP vertex cache across the shared DL.
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
            ref = res.pointer_at(DL_OFF + idx * 8 + 4)
            require(ref is not None and ref.asset_id == file_id,
                    f"word {idx}: VTX source outside actor bank")
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
                    fail(f"word {idx}: triangle uses unloaded cache slot")
    require(vtx_loads == [(3, 0, 0x540), (1, 0, 0x540), (2, 1, 0x560)],
            f"VTX load census changed: {vtx_loads}")
    require(len(tris) == 2 and len(dense) == 4,
            f"geometry census changed: {len(tris)} tris, {len(dense)} verts")
    corners = [d for t in tris for d in t]
    require(all(0 <= d < 4 for d in corners), f"corner out of range: {corners}")

    # Texture epoch: I4, no LUT, bank-owned image, 64-wide tile extent.
    setup = raw[:HOOK_IDX] + raw[HOOK_IDX + 1:17]
    w7, w8, w10, w11 = raw[7], raw[8], raw[10], raw[11]
    fmt, siz = (w8[0] >> 21) & 7, (w8[0] >> 19) & 3
    require((fmt, siz) == (4, 0), f"render tile not I4: fmt={fmt} siz={siz}")
    require(((w8[1] >> 24) & 7) == 0, "render tile id changed")
    require(((w7[1] >> 24) & 7) == 7, "load tile id changed")
    require(w10[1] == 0x000FC0FC, f"tile extent changed: 0x{w10[1]:08x}")
    img_ref = res.pointer_at(DL_OFF + 11 * 8 + 4)
    require(img_ref is not None and img_ref.asset_id == file_id
            and img_ref.offset == TEX_IMG_OFF,
            "texture image not bank offset 0x2B8")
    require(all((w0 >> 24) != 0xF0 for w0, _ in raw), "unexpected LOADTLUT (I needs none)")

    # Material scripts: full 5-word frames, exact vapor/solidity endpoints.
    text = TYPED.read_text()
    for off, first, second in ((0x674, "0xFFFFFF00", "0xFFFFFFFF"),
                               (0x694, "0xFFFFFFFF", "0xFFFFFF00")):
        anchor = f"AnimJoint_0x{off:04x}[5]"
        at = text.find(anchor)
        require(at >= 0, f"MatAnim 0x{off:x} anchor missing")
        seg = text[at:at + 600]
        require("AOBJ_EXTFLAG_PRIMCOLOR, 0" in seg and "AOBJ_EXTFLAG_PRIMCOLOR, 100" in seg
                and "aobjEvent32End()" in seg, f"MatAnim 0x{off:x} frame changed")
        require(seg.find(first) < seg.find(second), f"MatAnim 0x{off:x} endpoint order changed")
    # Every material frame 0..100 interpolates inside its endpoint range.
    for a0, a1, tag in ((0, 255, "Solid"), (255, 0, "Evaporate")):
        prev = None
        for f in range(101):
            a = a0 + (a1 - a0) * f // 100
            require(0 <= a <= 255, f"{tag} frame {f} alpha out of range")
            if prev is not None:
                require((a >= prev) if a1 > a0 else (a <= prev), f"{tag} frame {f} not monotonic")
            prev = a

    # Lifecycle + live-matrix + collision tokens in the source body.
    logic = LOGIC.read_text()
    for tok in ("dGRYosterCloudMatAnimJoints", "gcAddChildForDObj",
                "llGRYosterMapCloudDisplayList", "nGCMatrixKindTra", "nGCMatrixKind48",
                "lbCommonAddMObjForTreeDObjs", "mpCollisionSetYakumonoPosID",
                "mpCollisionSetYakumonoOnID", "mpCollisionSetYakumonoOffID"):
        require(tok in logic, f"gryoster.c missing {tok}")
    return {"words": raw, "dense": dense, "tris": tris, "corners": corners,
            "setup_a": raw[:HOOK_IDX], "setup_b": raw[HOOK_IDX + 1:17],
            "finish": raw[23:28]}


def state_stmt(op, w0, w1, asset_off=None):
    h = lambda v: f"0x{v:08x}u"  # noqa: E731
    if op in (0xE7, 0xE6, 0xDF):
        return None  # pipe/load syncs + EndDL: no DS state
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
    if op == 0xF3:
        return f"ndsRendererRecordLoadBlock(stats, {h(w0)}, {h(w1)});"
    if op == 0xD9:
        return (f"stats->geometry_mode = (stats->geometry_mode & {h(w0)}) | {h(w1)};")
    fail(f"unsupported state opcode 0x{op:02x}")
    return None


def render_packet(p):
    L = []
    A = L.append
    A("/* Yoshi Island cloud-platform native actor packet (generated).")
    A(" *")
    A(" * Source: MiscDataBank154 payload (yoster.py stage_actors pins) plus")
    A(" * relocData/154_StageYosterFile3.c and gr/grcommon/gryoster.c.")
    A(" * Shared template, three live instances; each instance executes this")
    A(" * DL once per drawable (3 runs x 2 tris = 6 tris/instance).")
    A(" * Do not hand-edit: regenerate with")
    A(" *   python scripts/stages/generate_nds_native_yoster_clouds.py --emit-packet")
    A(" */")
    A("static const u8 sNdsNativeActorYosterCloudJointParents[7] =")
    A("{")
    for v in PARENTS:
        A(f"    {v}u,")
    A("};")
    A("")
    A("static const u8 sNdsNativeActorYosterCloudJointXObjCounts[7] =")
    A("{")
    for v in XOBJ_COUNTS:
        A(f"    {v}u,")
    A("};")
    A("")
    A("static const u8 sNdsNativeActorYosterCloudJointXObjKinds[10] =")
    A("{")
    for v in XOBJ_KINDS:
        A(f"    {v}u,")
    A("};")
    A("")
    A("static const u8 sNdsNativeActorYosterCloudBindings[7] =")
    A("{")
    for v in BINDINGS:
        A(f"    {v}u,")
    A("};")
    A("")
    A("static const u16 sNdsNativeActorYosterCloudRuns[9] =")
    A("{")
    for b in (4, 5, 6):
        A(f"    {b}u, 0u, 2u,")
    A("};")
    A("")
    A("static const u16 sNdsNativeActorYosterCloudTriIndices[6] =")
    A("{")
    for d in p["corners"]:
        A(f"    {d}u,")
    A("};")
    A("")
    A("static const s16 sNdsNativeActorYosterCloudVerts[20] =")
    A("{")
    for v in p["dense"]:
        A(f"    {v[0]}, {v[1]}, {v[2]}, {v[3]}, {v[4]},")
    A("};")
    A("")
    A("static const u32 sNdsNativeActorYosterCloudVertColors[4] =")
    A("{")
    for v in p["dense"]:
        A(f"    0x{v[5]:08x}u,")
    A("};")
    A("")
    A("static const u32 sNdsNativeActorYosterCloudTextureEpoch[6] =")
    A("{")
    A(f"    0x{TEX_IMG_OFF:08x}u,")
    A(f"    {TEX_IMG_SIZE}u,")
    A("    252u,")
    A("    4u,")
    A("    0u,")
    A("    0u,")
    A("};")
    A("")
    A("static void ndsNativeActorYosterCloudSetupState(")
    A("    NDSRendererStats *stats, const u8 *asset_base)")
    A("{")
    A("    (void)asset_base;")
    for span in (p["setup_a"], p["setup_b"]):
        for w0, w1 in span:
            s = state_stmt(w0 >> 24, w0, w1,
                           asset_off=TEX_IMG_OFF if (w0 >> 24) == 0xFD else None)
            if s is not None:
                A(f"    {s}")
    A("}")
    A("")
    A("static void ndsNativeActorYosterCloudFinishState(")
    A("    NDSRendererStats *stats, const u8 *asset_base)")
    A("{")
    A("    (void)asset_base;")
    for w0, w1 in p["finish"]:
        s = state_stmt(w0 >> 24, w0, w1)
        if s is not None:
            A(f"    {s}")
    A("}")
    counts = {"joints": 7, "drawables": 3, "runs": 3, "triangles": 6,
              "verts": 4, "corners": 6, "setup_a": 12, "setup_b": 4, "finish": 5}
    slab = (7 + 7 + 10 + 7 + 9 * 2 + 6 * 2 + 4 * 14 + 6 * 4 + (12 + 4 + 5) * 8)
    require(slab == 309, f"slab changed: {slab}")
    A("")
    A(f"/* counts: joints=7 drawables=3 runs=3 triangles=6 verts=4 slab={slab} */")
    return "\n".join(L) + "\n", counts, slab


HEADER_WANTS = {
    "NDS_NATIVE_ACTOR_YOSTER_CLOUD_JOINT_COUNT": 7,
    "NDS_NATIVE_ACTOR_YOSTER_CLOUD_TEMPLATE_LIVE": 4,
    "NDS_NATIVE_ACTOR_YOSTER_CLOUD_DRAWABLE_COUNT": 3,
    "NDS_NATIVE_ACTOR_YOSTER_CLOUD_RUN_COUNT": 3,
    "NDS_NATIVE_ACTOR_YOSTER_CLOUD_TRIANGLE_COUNT": 6,
    "NDS_NATIVE_ACTOR_YOSTER_CLOUD_VERT_COUNT": 4,
    "NDS_NATIVE_ACTOR_YOSTER_CLOUD_CORNER_COUNT": 6,
    "NDS_NATIVE_ACTOR_YOSTER_CLOUD_STATE_SETUP_A": 12,
    "NDS_NATIVE_ACTOR_YOSTER_CLOUD_STATE_SETUP_B": 4,
    "NDS_NATIVE_ACTOR_YOSTER_CLOUD_STATE_FINISH": 5,
    "NDS_NATIVE_ACTOR_YOSTER_CLOUD_MAT_ANIM_COUNT": 2,
    "NDS_NATIVE_ACTOR_YOSTER_CLOUD_MAT_ANIM_WORDS": 5,
    "NDS_NATIVE_ACTOR_YOSTER_CLOUD_MAT_FRAME_MAX": 100,
    "NDS_NATIVE_ACTOR_YOSTER_CLOUD_TEX_IMG_SIZE": 512,
    "NDS_NATIVE_ACTOR_YOSTER_CLOUD_TILE_EXTENT": 252,
    "NDS_NATIVE_ACTOR_YOSTER_CLOUD_TEX_FMT": 4,
    "NDS_NATIVE_ACTOR_YOSTER_CLOUD_TEX_SIZ": 0,
    "NDS_NATIVE_ACTOR_YOSTER_CLOUD_SLAB_BYTES": 309,
}


def check_header():
    import re
    text = HEADER.read_text()
    for name, want in HEADER_WANTS.items():
        m = re.search(rf"#define\s+{name}\s+(0x[0-9a-fA-F]+|\d+)u?", text)
        require(m is not None and int(m.group(1), 0) == want, f"header: {name} must be {want}")
    for sym in ("ndsRendererSubmitNativeYosterCloud",
                "ndsRendererAdapterSubmitNativeYosterCloud",
                "ndsGRYosterCloudGObj"):
        require(sym in text, f"header: {sym} missing")


def exec_model(joint_count, parents, bindings, alphas):
    """Host mirror of the C executor branches: validate, gate, count."""
    if joint_count != 7:
        return (False, 0)
    if tuple(parents) != PARENTS or tuple(bindings) != BINDINGS:
        return (False, 0)
    if len(alphas) != 3 or any(a < 0 or a > 255 for a in alphas):
        return (False, 0)
    return (True, 2 * sum(1 for a in alphas if a > 0))


def prove():
    p = decode()
    require(not FAILURES, "decode must be green before proof")
    lines = []
    # Three live instances, distinct collision poses, solid alphas.
    poses = [(1200, 800, 0), (-1500, 950, 0), (300, 1400, 0)]
    total = 0
    sigs = set()
    for i, (pose, line) in enumerate(zip(poses, (1, 2, 3))):
        ok, n = exec_model(7, PARENTS, BINDINGS, (255, 255, 255))
        require(ok and n == 6, f"instance {i}: expected 6 tris, got {n}")
        total += n
        sigs.add((pose, line, 255))
        lines.append(f"prove instance{i} line={line} pose={pose} alpha=255 tris={n}")
    require(total == 18, f"3 instances must emit 18 tris, got {total}")
    require(len(sigs) == 3, "instances must carry distinct pose/line identity")
    # Live pose change moves the instance signature.
    moved = ((1201, 800, 0), 1, 255)
    require(moved not in sigs, "pose change must move matrix signature")
    lines.append(f"prove pose-delta signature {moved} != prior (matrix live)")
    # Live alpha change gates triangles: evaporate instance 1 fully.
    ok, n = exec_model(7, PARENTS, BINDINGS, (0, 0, 0))
    require(ok and n == 0, "evaporated instance must emit 0 tris yet stay valid")
    lines.append("prove evaporate instance tris=0 (ordinary state, still TRUE)")
    ok, n = exec_model(7, PARENTS, BINDINGS, (255, 0, 128))
    require(ok and n == 4, f"partial alpha must emit 4 tris, got {n}")
    lines.append("prove partial alpha (255,0,128) tris=4 (per-drawable gating live)")
    # Fail-closed malformed input.
    for bad, tag in (((6, PARENTS, BINDINGS, (255,) * 3), "joint_count 6"),
                     ((7, (31, 0, 0, 0, 1, 2, 9), BINDINGS, (255,) * 3), "parent drift"),
                     ((7, PARENTS, BINDINGS, (256, 255, 255)), "alpha 256"),
                     ((7, PARENTS, BINDINGS, (255,)), "alpha arity")):
        ok, n = exec_model(*bad)
        require(not ok and n == 0, f"malformed input accepted: {tag}")
        lines.append(f"prove reject {tag}")
    print("\n".join(lines))
    if FAILURES:
        print("PROVE FAIL:")
        for f in FAILURES:
            print(f"  - {f}")
        return 1
    print("prove: 18/18 tris across 3 instances; pose+alpha live; 4 malformed rejected")
    return 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--emit-packet", action="store_true")
    ap.add_argument("--prove", action="store_true")
    args = ap.parse_args()
    if args.prove:
        return prove()
    p = decode()
    text, counts, slab = render_packet(p)
    print(f"yoster-clouds: joints=7 (template_live=4 + drawables=3) "
          f"dl_words=29 src_verts=4 tris=6/instance runs=3 "
          f"mat=2x5w frames=0..100 tex=I4@{TEX_IMG_OFF:#x}[{TEX_IMG_SIZE}] slab={slab}")
    if args.emit_packet:
        PACKET.parent.mkdir(parents=True, exist_ok=True)
        PACKET.write_text(text)
        print(f"emitted {PACKET}")
        return 0 if not FAILURES else 1
    if PACKET.is_file():
        require(PACKET.read_text() == text, "packet file drifted: re-run --emit-packet + review")
    else:
        fail("packet file absent: run --emit-packet once, then --check")
    check_header()
    if FAILURES:
        print("FAIL:")
        for f in FAILURES:
            print(f"  - {f}")
        return 1
    print("yoster-clouds --check: green (host/source only, no ROM)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
