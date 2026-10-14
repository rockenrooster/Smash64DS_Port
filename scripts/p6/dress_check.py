#!/usr/bin/env python3
"""P6: Palutena's dress against her legs, every motion (owner playtest
2026-10-14: in EVERY animation the dress surrounds her legs, except at the
slit).

Each motion's clip is posed on the content's own skeleton (coverage.py
Fighter.pose_frames: her rows, translate_scales, the TransN slot); the look
scene's dress and leg vertices ride their rows from the bind pose. The skirt
is measured as cones between rings (each ring's centre and mean radius,
posed): one from its top ring to its hem ring, or, once the skirt hangs from
Peach's dress rows (look_skirt.py), the upper band (the waist ring to the
cut ring on row 1) and the lower band (row 25's ring to the hem on row 26).
A leg vertex between a band's two rings whose distance from its axis passes
the band's radius there is outside the dress by that much. Vertices on the slit's side of the skirt (its angle
within SLIT_HALF_DEG of the slit's direction, in the skirt's own frame) are
reported apart: the slit lets that leg out.

    dress_check.py --staging builds/p4-staging/extra-96621af \\
        --export builds/p4-staging/export-x/palutena \\
        --scene builds/p6-art/palutena/3ds2ds/scene_high/scene.json [--json out.json]

Prints, per motion, the frames and the worst distance a leg vertex reaches
outside the dress (away from the slit, and at it), worst first.
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent / "p4"))
import anim as A  # noqa: E402
import coverage as C  # noqa: E402

DRESS = "DS_dress"
LEGS = "DS_legs"
# Below the hips: the skirt's part of the dress (the bodice is above).
SKIRT_TOP_Y = 335.0
# Leg rows above the foot (the hip and knee rings, thigh and shin lists).
LEG_ROWS = (16, 17, 18, 21, 22, 23)
SLIT_HALF_DEG = 35.0
# Below this many units outside, a vertex is within the cloth's thickness.
TOLERANCE = 4.0


def rings(scene: dict):
    """The skirt's bands as (top ring, bottom ring) vertex index lists, the
    hem ring, and the slit's vertices (the skirt vertices on neither ring of
    a one-band skirt: the slit's top)."""
    verts = scene["vertices"]
    drawn = {i for t in scene["triangles"] for i in t["v"]}
    skirt = [i for i, v in enumerate(verts) if v.get("object") == DRESS and v["model"][1] <= SKIRT_TOP_Y
             and i in drawn]
    on = lambda row: [i for i in skirt if verts[i]["joint"] == row]
    if on(26):
        ys = np.array([verts[i]["model"][1] for i in skirt])
        top = [i for i in skirt if verts[i]["model"][1] > ys.max() - 12.0]
        return [(top, on(1)), (on(25), on(26))], on(26), []
    ys = np.array([verts[i]["model"][1] for i in skirt])
    top_y, hem_y = ys.max(), ys.min()
    top = [i for i in skirt if verts[i]["model"][1] > top_y - 12.0]
    hem = [i for i in skirt if verts[i]["model"][1] < hem_y + 20.0]
    slit = [i for i in skirt if i not in top and i not in hem]
    return [(top, hem)], hem, slit


def apply_scene_skeleton(fighter: C.Fighter, scene: dict) -> None:
    """The rows' rest translations as the look scene places them (what
    model_writer.py writes: a joint's offset from its parent's world), so a
    scene that moved a row (look_skirt.py's pivot) is checked as built."""
    worlds = [np.array(j["world"], dtype=float) for j in scene["joints"]]
    for j, row in enumerate(scene["joints"]):
        if j >= len(fighter.skel.translate):
            break
        p = row["parent"]
        local = worlds[j] if p < 0 else np.linalg.inv(worlds[p]) @ worlds[j]
        fighter.skel.translate[j] = tuple(float(c) for c in local[:3, 3])


def bind_mats(fighter: C.Fighter):
    empty = A.Clip(len(fighter.skel.parents))
    _root, mats = fighter.pose_frames(empty, 0, 0)[0]
    return mats


def posed(points: np.ndarray, rows: list[int], mats, inv_bind) -> np.ndarray:
    out = np.empty_like(points)
    for k, (p, r) in enumerate(zip(points, rows)):
        q = mats[r] @ inv_bind[r] @ np.array([p[0], p[1], p[2], 1.0])
        out[k] = q[:3]
    return out


def ring(pts: np.ndarray):
    c = pts.mean(axis=0)
    return c, float(np.linalg.norm(pts - c, axis=1).mean())


def check_frame(dress_pts, bands, leg_pts, slit_dir_local, pelvis=None, upper_rest=None):
    """(worst outside away from the slit, worst at the slit) for one pose.

    One band (the old skirt): a leg vertex between its rings outside it.
    Two (look_skirt.py): a vertex between the lower band's rings outside it;
    one above the lower band's top ring is held to the upper band, measured
    in the pelvis's frame against its rest shape (`upper_rest`: the band's
    axis point, height range and radii there) -- the short upper band shears
    when the torso bends, and a sheared band's own axis says nothing about
    the hips; a vertex below the hem is never outside."""
    worst, worst_slit = 0.0, 0.0
    lower = bands[-1]
    per = _band_out(dress_pts, lower[0], lower[1], leg_pts, slit_dir_local, keep_t=True)
    for k, item in enumerate(per):
        if item is None:
            continue
        t, o, slit = item
        if t > 1.0:
            continue
        if t < 0.0:
            if upper_rest is None or pelvis is None:
                continue
            q = (pelvis @ np.append(leg_pts[k], 1.0))[:3]
            centre, y_lo, y_hi, r_lo, r_hi = upper_rest
            if not y_lo - 10.0 <= q[1] <= y_hi:
                continue
            a = min(max((q[1] - y_lo) / max(y_hi - y_lo, 1.0), 0.0), 1.0)
            o = float(np.hypot(q[0] - centre[0], q[2] - centre[2])) - (r_lo + (r_hi - r_lo) * a)
            slit = False
        if o <= TOLERANCE:
            continue
        if slit:
            worst_slit = max(worst_slit, o)
        else:
            worst = max(worst, o)
    return worst, worst_slit


def _band_out(dress_pts, top, hem, leg_pts, slit_dir_local, keep_t=False):
    """Per leg vertex: None when it is not between the band's rings, else
    (how far outside the band, on the slit's side); with keep_t, every
    vertex as (t along the band, how far outside, on the slit's side)."""
    ct, rt = ring(dress_pts[top])
    ch, rh = ring(dress_pts[hem])
    axis = ch - ct
    length = float(np.linalg.norm(axis))
    if length < 1.0:
        return [None] * len(leg_pts)
    u = axis / length
    # The skirt's own frame: u down its axis, the slit's direction across it.
    s = slit_dir_local - np.dot(slit_dir_local, u) * u
    s = s / max(np.linalg.norm(s), 1e-6)
    out = []
    for p in leg_pts:
        d = p - ct
        t = float(np.dot(d, u)) / length
        if not keep_t and not 0.0 <= t <= 1.0:
            out.append(None)
            continue
        radial = d - np.dot(d, u) * u
        dist = float(np.linalg.norm(radial))
        cosang = float(np.dot(radial / max(dist, 1e-6), s))
        tc = min(max(t, 0.0), 1.0)
        item = (dist - (rt + (rh - rt) * tc), cosang >= np.cos(np.radians(SLIT_HALF_DEG)))
        out.append((t,) + item if keep_t else item)
    return out


class Report:
    """The dress check of clips on a content's skeleton and look scene."""

    def __init__(self, fighter: C.Fighter, scene: dict):
        verts = scene["vertices"]
        self.fighter = fighter
        apply_scene_skeleton(fighter, scene)
        bind = bind_mats(fighter)
        self.inv_bind = [np.linalg.inv(m) for m in bind]
        bands, hem, slit = rings(scene)
        dress_idx = sorted({i for band in bands for ring_ in band for i in ring_} | set(slit))
        self.dress_model = np.array([verts[i]["model"] for i in dress_idx])
        self.dress_rows = [verts[i]["joint"] for i in dress_idx]
        remap = {i: k for k, i in enumerate(dress_idx)}
        self.bands = [([remap[i] for i in a], [remap[i] for i in b]) for a, b in bands]
        self.hem_row = verts[hem[0]]["joint"]
        self.hem_c = np.array([verts[i]["model"] for i in hem]).mean(axis=0)
        # The slit faces the +x front (the hem's u 0.65) when no slit vertex says.
        self.slit_c = (np.array([verts[i]["model"] for i in slit]).mean(axis=0) if slit
                       else self.hem_c + np.array([1.0, 0.0, 0.6]))
        legs = [i for i, v in enumerate(verts) if v.get("object") == LEGS and v["joint"] in LEG_ROWS]
        self.leg_model = np.array([verts[i]["model"] for i in legs])
        self.leg_rows = [verts[i]["joint"] for i in legs]
        self.upper_rest = None
        if len(bands) == 2:
            pin = np.linalg.inv(bind[1])
            to_p = lambda i: (pin @ np.append(verts[i]["model"], 1.0))[:3]
            top = np.array([to_p(i) for i in bands[0][0]])
            low = np.array([to_p(i) for i in bands[0][1]])
            centre = np.vstack([top, low]).mean(axis=0)
            rad = lambda pts: float(np.hypot(pts[:, 0] - centre[0], pts[:, 2] - centre[2]).mean())
            self.upper_rest = (centre, float(low[:, 1].mean()), float(top[:, 1].mean()), rad(low), rad(top))

    def clip(self, clip: A.Clip, anim_flags: int):
        """(frames, frames with a leg out, worst out away from the slit,
        worst at the slit)."""
        length = max((st.length for st in clip.streams.values()), default=0)
        frames = self.fighter.pose_frames(clip, anim_flags, length)
        worst, worst_slit, bad = 0.0, 0.0, 0
        inv = self.inv_bind
        for _root, mats in frames:
            dp = posed(self.dress_model, self.dress_rows, mats, inv)
            lp = posed(self.leg_model, self.leg_rows, mats, inv)
            m = mats[self.hem_row] @ inv[self.hem_row]
            sd = (m @ np.append(self.slit_c, 1.0))[:3] - (m @ np.append(self.hem_c, 1.0))[:3]
            w, ws = check_frame(dp, self.bands, lp, sd, np.linalg.inv(mats[1]), self.upper_rest)
            worst, worst_slit = max(worst, w), max(worst_slit, ws)
            bad += 1 if w > 0 else 0
        return length + 1, bad, worst, worst_slit


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--staging", type=Path, required=True)
    ap.add_argument("--export", type=Path, required=True)
    ap.add_argument("--scene", type=Path, required=True)
    ap.add_argument("--json", type=Path)
    ap.add_argument("--motions", help="comma-separated motion indices (default: all)")
    args = ap.parse_args()

    scene = json.loads(args.scene.read_text(encoding="utf-8"))
    fighter = C.load(args.staging, args.export)
    report = Report(fighter, scene)
    wanted = None
    if args.motions:
        wanted = {int(x, 0) for x in args.motions.split(",")}
    seen: dict[tuple, tuple] = {}
    rows_out = []
    for m in fighter.resolved["motions"]:
        if not m["anim_file_id"] or (wanted is not None and m["index"] not in wanted):
            continue
        fid = m["anim_file_id"]
        key = (fid, m["anim_flags"])
        if key not in seen:
            seen[key] = report.clip(A.decode(fighter.rom.o2r(fid)), m["anim_flags"])
        frames_n, bad, worst, worst_slit = seen[key]
        rows_out.append({"motion": m["index"], "clip": f"{fid:#x}", "frames": frames_n,
                         "frames_out": bad, "worst": round(worst, 1), "worst_slit": round(worst_slit, 1)})
    rows_out.sort(key=lambda r: -r["worst"])
    for r in rows_out:
        print(f"motion {r['motion']:3d} clip {r['clip']:>7} frames {r['frames']:3d} "
              f"out {r['frames_out']:3d}  worst {r['worst']:6.1f}  slit {r['worst_slit']:6.1f}")
    clean = sum(1 for r in rows_out if r["worst"] <= 0)
    print(f"dress check: {clean} of {len(rows_out)} motions clean (away from the slit)")
    if args.json:
        args.json.write_text(json.dumps(rows_out, indent=1) + "\n", encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
