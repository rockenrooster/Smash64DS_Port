#!/usr/bin/env python3
"""P6: Palutena's dress against her legs, every motion (owner playtest
2026-10-14: in EVERY animation the dress surrounds her legs).

Each motion's clip is posed on the content's own skeleton (coverage.py
Fighter.pose_frames: her rows, translate_scales, the TransN slot); the look
scene's dress and leg vertices ride their rows from the bind pose. The skirt
hangs from Peach's dress rows (look_skirt.py): the upper band (the waist ring
to the cut ring on row 1) and the lower bands, row 25's ring to the mid ring
on row 26 and on to the hem on row 27 (owner p6s; before the split, one band
to the hem on row 26), each measured about its top ring's row's axis
(chain_outs); an older one-band skirt is one band from its top ring to its
hem.

Her leg mesh is rings (the hip's on rows 16/21, the knee's on 17/22, the
ankle's on 18/23) joined by tubes, so the dress is checked at points along
them (LegSamples), as the owner's 3DS references show it (owner p6j,
2026-10-14: "the slit leg actually never comes out of the dress at all";
"if Palutena takes a step forward where the knee pokes out, the dress should
be stretched over it"):

- the thigh and the knee ("core") are inside the dress: inside the lower
  band and above its hem, or, above the band's top ring, inside the upper
  band (measured in the pelvis's frame against its rest shape: the short
  upper band shears when the torso bends);
- the shin does not come through the lower band's side above the hem; below
  the hem it shows (a raised knee lifts the hem, the shin hangs from under
  it, as on the 3DS);
- the feet are free (the 3DS's show under her dress).

The lower band is measured as drawn (band_outs): the surface from its top
ring to its hem, each ring taken by angle about the axis through their
centres (its axial place and radius in a point's direction), so a hem
stretched, tilted or lifted over a raised knee (skirt_keys.py) is measured as
it is. Both legs count, the slit's too (owner p6j); the slit leg's worst is
reported apart.

    dress_check.py --staging builds/p4-staging/extra-96621af \\
        --export builds/p4-staging/export-x/palutena \\
        --scene builds/p6-art/palutena/3ds2ds/scene_high/scene.json [--json out.json]

Prints, per motion, the frames and the worst distance a leg point reaches
outside the dress (either leg, and the slit leg's), worst first.
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
# Leg rows (the hip, knee and ankle rings).
LEG_ROWS = (16, 17, 18, 21, 22, 23)
# Each leg's rows: hip ring, knee ring, ankle ring (the slit's leg first).
LEG_CHAINS = ((16, 17, 18), (21, 22, 23))
# Points along each thigh (hip ring to knee ring; 1.0 is the knee) and each
# shin (knee ring to ankle ring) with their weights: the thigh's and the
# shin's first (the knee) are core (weight 1.0).
THIGH_AT = (0.25, 0.5, 0.75, 1.0)
SHIN_AT = ((0.15, 1.0), (0.4, 0.5), (0.65, 0.25))
# Below this many units outside, a point is within the cloth's thickness.
TOLERANCE = 4.0
# The band's measure for a point it does not hold to anything (above its top
# ring, or a shin below the hem).
FREE = -1.0e9


def rings(scene: dict):
    """The skirt's bands as (top ring, bottom ring) vertex index lists, the
    hem ring, and the slit's vertices (the skirt vertices on neither ring of
    a one-band skirt: the slit's top). A two-band skirt's slit top (a row 25
    vertex below its ring, look_skirt.py) is on neither band's rings."""
    verts = scene["vertices"]
    drawn = {i for t in scene["triangles"] for i in t["v"]}
    skirt = [i for i, v in enumerate(verts) if v.get("object") == DRESS and v["model"][1] <= SKIRT_TOP_Y
             and i in drawn]
    on = lambda row: [i for i in skirt if verts[i]["joint"] == row]
    if on(27):
        # Three rings (look_skirt.py split_band): row 25's ring, the mid ring
        # on row 26 (with the slit's two edge vertices), the hem on row 27.
        ys = np.array([verts[i]["model"][1] for i in skirt])
        top = [i for i in skirt if verts[i]["model"][1] > ys.max() - 12.0]
        y25 = max(verts[i]["model"][1] for i in on(25))
        ring25 = [i for i in on(25) if verts[i]["model"][1] > y25 - 12.0]
        return [(top, on(1)), (ring25, on(26)), (on(26), on(27))], on(27), []
    if on(26):
        ys = np.array([verts[i]["model"][1] for i in skirt])
        top = [i for i in skirt if verts[i]["model"][1] > ys.max() - 12.0]
        y25 = max(verts[i]["model"][1] for i in on(25))
        ring25 = [i for i in on(25) if verts[i]["model"][1] > y25 - 12.0]
        return [(top, on(1)), (ring25, on(26))], on(26), []
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
        # The scene's tree (model_writer.py writes its parents and makes a
        # drawn row's DObj): Palutena's hem row 27 hangs from row 26.
        fighter.skel.parents[j] = p
        if row.get("has_list") and fighter.rows is not None and j not in fighter.rows:
            fighter.rows = sorted(fighter.rows + [j])


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


class LegSamples:
    """Points along her legs (the leg mesh's tubes between its rings): each
    knee vertex paired with the hip ring's and the ankle ring's vertex
    nearest it in angle about the leg (bind pose), points THIGH_AT along the
    first pair and SHIN_AT along the second."""

    def __init__(self, scene: dict):
        verts = scene["vertices"]
        ids = [i for i, v in enumerate(verts) if v.get("object") == LEGS and v["joint"] in LEG_ROWS]
        self.model = np.array([verts[i]["model"] for i in ids], dtype=float)
        self.rows = [verts[i]["joint"] for i in ids]
        on = lambda row: [k for k, r in enumerate(self.rows) if r == row]
        pa, pb, frac, weight, core, leg = [], [], [], [], [], []
        for n, (hip, knee, ankle) in enumerate(LEG_CHAINS):
            ring_h, ring_k, ring_a = on(hip), on(knee), on(ankle)
            if not (ring_h and ring_k and ring_a):
                continue

            def angle(k, ring_):
                c = self.model[ring_].mean(axis=0)
                v = self.model[k] - c
                return np.arctan2(v[2], v[0])

            def nearest(k, ring_):
                a = angle(k, ring_k)
                return min(ring_, key=lambda j: abs(np.angle(np.exp(1j * (angle(j, ring_) - a)))))
            for k in ring_k:
                h = nearest(k, ring_h)
                for f in THIGH_AT:
                    pa.append(h), pb.append(k), frac.append(f), weight.append(1.0), core.append(True), leg.append(n)
                a = nearest(k, ring_a)
                for f, w in SHIN_AT:
                    pa.append(k), pb.append(a), frac.append(f), weight.append(w), core.append(w >= 1.0), leg.append(n)
        self.a = np.array(pa, dtype=int)
        self.b = np.array(pb, dtype=int)
        self.f = np.array(frac)[:, None]
        self.weight = np.array(weight)
        self.core = np.array(core, dtype=bool)
        self.leg = np.array(leg, dtype=int)
        self.slit = self.leg == 0

    def posed(self, mats, inv_bind) -> np.ndarray:
        """The points in model space for one pose."""
        v = posed(self.model, self.rows, mats, inv_bind)
        return v[self.a] * (1.0 - self.f) + v[self.b] * self.f


def _ring_angles(rel, u, e1, e2):
    a = rel @ u
    radial = rel - np.outer(a, u)
    return np.arctan2(radial @ e2, radial @ e1), a, np.linalg.norm(radial, axis=1)


def _periodic(ang, *vals):
    order = np.argsort(ang)
    ang = ang[order]
    xs = np.concatenate([ang - 2 * np.pi, ang, ang + 2 * np.pi])
    return xs, [np.concatenate([v[order]] * 3) for v in vals]


def band_profile_outs(a, rho, a_top, r_top, a_hem, r_hem, core) -> np.ndarray:
    """band_outs from the points' depths a and radii rho about the axis and
    the rings' depths and radii in their directions."""
    span = np.maximum(a_hem - a_top, 1.0)
    s = (a - a_top) / span
    band = r_top + np.clip(s, 0.0, 1.0) * (r_hem - r_top)
    side = (rho - band) * span / np.hypot(span, r_hem - r_top)
    out = np.where(s > 1.0, np.where(core, np.maximum(side, a - a_hem), FREE), side)
    return np.where(s < 0.0, FREE, out)


def _band_terms(top: np.ndarray, bottom: np.ndarray, pts: np.ndarray, origin: np.ndarray, u: np.ndarray,
                across: np.ndarray):
    """One band's measure of the points (band_outs): their place down it
    (0 at its top ring, 1 at its bottom ring), their distance outside its
    side, and how far below its bottom ring they are."""
    e1 = across - np.dot(across, u) * u
    if np.linalg.norm(e1) < 1e-3:
        e1 = np.cross(u, [0.0, 1.0, 0.0])
    e1 = e1 / np.linalg.norm(e1)
    e2 = np.cross(u, e1)
    xt, (at, rt) = _periodic(*_ring_angles(top - origin, u, e1, e2))
    xh, (ah, rh) = _periodic(*_ring_angles(bottom - origin, u, e1, e2))
    ang, a, rho = _ring_angles(pts - origin, u, e1, e2)
    a_top, r_top = np.interp(ang, xt, at), np.interp(ang, xt, rt)
    a_hem, r_hem = np.interp(ang, xh, ah), np.interp(ang, xh, rh)
    span = np.maximum(a_hem - a_top, 1.0)
    s = (a - a_top) / span
    band = r_top + np.clip(s, 0.0, 1.0) * (r_hem - r_top)
    return s, (rho - band) * span / np.hypot(span, r_hem - r_top), a - a_hem


def chain_outs(rings_: list, axes: list, across: np.ndarray, pts: np.ndarray, core: np.ndarray):
    """band_outs over a chain of bands (ring i to ring i + 1, about the axis
    axes[i] through ring i's centre): a point is measured on the first band
    it is not below; above the first ring it is FREE, below the last (the
    hem) a core point is out by how far below. Also the points' place down
    the first band (negative: above it)."""
    out = np.full(len(pts), FREE)
    done = np.zeros(len(pts), dtype=bool)
    n = len(rings_) - 1
    s0 = None
    for b in range(n):
        s, side, below = _band_terms(rings_[b], rings_[b + 1], pts, rings_[b].mean(axis=0), axes[b], across)
        if b == 0:
            s0 = s
            done |= s < 0.0
        held = ~done & (s <= 1.0)
        out = np.where(held, side, out)
        done |= held
        if b == n - 1:
            past = ~done
            out = np.where(past, np.where(core, np.maximum(side, below), FREE), out)
    return out, s0


def band_outs(top: np.ndarray, hem: np.ndarray, pts: np.ndarray, core: np.ndarray,
              origin: np.ndarray, u: np.ndarray, across: np.ndarray) -> np.ndarray:
    """Per point: how far outside the lower band it is (signed: negative
    inside), FREE where the band holds it to nothing (above its top ring:
    the upper band's; a shin below the hem). The band is the surface from
    its top ring to its hem ring, each ring taken by angle about its axis
    (the line through origin along u, down the band: row 25's -z through its
    ring's centre) -- its depth down the axis and radius in the point's
    direction, interpolated between its vertices -- the distance taken
    across the band's slope; a core point below the hem is outside by how
    far below it is."""
    e1 = across - np.dot(across, u) * u
    if np.linalg.norm(e1) < 1e-3:
        e1 = np.cross(u, [0.0, 1.0, 0.0])
    e1 = e1 / np.linalg.norm(e1)
    e2 = np.cross(u, e1)
    xt, (at, rt) = _periodic(*_ring_angles(top - origin, u, e1, e2))
    xh, (ah, rh) = _periodic(*_ring_angles(hem - origin, u, e1, e2))
    ang, a, rho = _ring_angles(pts - origin, u, e1, e2)
    return band_profile_outs(a, rho, np.interp(ang, xt, at), np.interp(ang, xt, rt),
                             np.interp(ang, xh, ah), np.interp(ang, xh, rh), core)


def upper_rest_of(scene: dict, bind) -> tuple | None:
    """The upper band's rest shape in the pelvis's frame (its axis point,
    height range and radii there), or None for a one-band skirt."""
    verts = scene["vertices"]
    bands, _hem, _slit = rings(scene)
    if len(bands) < 2:
        return None
    pin = np.linalg.inv(bind[1])
    to_p = lambda i: (pin @ np.append(verts[i]["model"], 1.0))[:3]
    top = np.array([to_p(i) for i in bands[0][0]])
    low = np.array([to_p(i) for i in bands[0][1]])
    centre = np.vstack([top, low]).mean(axis=0)
    rad = lambda pts: float(np.hypot(pts[:, 0] - centre[0], pts[:, 2] - centre[2]).mean())
    return centre, float(low[:, 1].mean()), float(top[:, 1].mean()), rad(low), rad(top)


def upper_outs(pts_pelvis: np.ndarray, upper_rest) -> np.ndarray:
    """Per point (the pelvis's frame): how far outside the upper band's rest
    shape (its axis point, height range and radii there), FREE off its
    height range."""
    centre, y_lo, y_hi, r_lo, r_hi = upper_rest
    y = pts_pelvis[:, 1]
    a = np.clip((y - y_lo) / max(y_hi - y_lo, 1.0), 0.0, 1.0)
    o = np.hypot(pts_pelvis[:, 0] - centre[0], pts_pelvis[:, 2] - centre[2]) - (r_lo + (r_hi - r_lo) * a)
    return np.where((y >= y_lo - 10.0) & (y <= y_hi), o, FREE)


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
        # The lower bands' rings (row 25's to the hem) and the rows whose -z
        # is each band's axis.
        self.chain = [self.bands[1][0]] + [b[1] for b in self.bands[1:]] if len(bands) > 1 else None
        self.axis_rows = [verts[a[0]]["joint"] for a, _b in bands[1:]] if len(bands) > 1 else None
        self.legs = LegSamples(scene)
        self.upper_rest = upper_rest_of(scene, bind)

    def frame_outs(self, mats) -> np.ndarray:
        """Per leg point, how far outside the dress (FREE: nothing holds it)."""
        dp = posed(self.dress_model, self.dress_rows, mats, self.inv_bind)
        lp = self.legs.posed(mats, self.inv_bind)
        top, hem = self.bands[-1]
        if self.upper_rest is not None:
            # Each lower band's axis: its top ring's row's -z (row 25's, the
            # mid ring's row 26) through the ring's centre.
            axes = [-mats[r][:3, 2] / np.linalg.norm(mats[r][:3, 2]) for r in self.axis_rows]
            out, s0 = chain_outs([dp[r] for r in self.chain], axes, mats[25][:3, 0], lp, self.legs.core)
            pin = np.linalg.inv(mats[1])
            q = (pin @ np.c_[lp, np.ones(len(lp))].T).T[:, :3]
            up = upper_outs(q, self.upper_rest)
            # Above the lower band's top: the upper band holds the core.
            above = (s0 < 0.0) & self.legs.core
            return np.where(above, up, out)
        origin = dp[top].mean(axis=0)
        u = dp[hem].mean(axis=0) - origin
        u = u / max(np.linalg.norm(u), 1e-6)
        return band_outs(dp[top], dp[hem], lp, self.legs.core, origin, u, mats[1][:3, 0])

    def clip(self, clip: A.Clip, anim_flags: int):
        """(frames, frames with a leg out, worst out of either leg, worst of
        the slit leg)."""
        length = max((st.length for st in clip.streams.values()), default=0)
        frames = self.fighter.pose_frames(clip, anim_flags, length)
        worst, worst_slit, bad = 0.0, 0.0, 0
        for _root, mats in frames:
            out = self.frame_outs(mats)
            w = float(out.max(initial=0.0))
            ws = float(out[self.legs.slit].max(initial=0.0))
            w = w if w > TOLERANCE else 0.0
            ws = ws if ws > TOLERANCE else 0.0
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
              f"out {r['frames_out']:3d}  worst {r['worst']:6.1f}  slit leg {r['worst_slit']:6.1f}")
    clean = sum(1 for r in rows_out if r["worst"] <= 0)
    print(f"dress check: {clean} of {len(rows_out)} motions clean (both legs)")
    if args.json:
        args.json.write_text(json.dumps(rows_out, indent=1) + "\n", encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
