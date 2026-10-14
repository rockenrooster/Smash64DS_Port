#!/usr/bin/env python3
"""P6: keys for Palutena's skirt rows, from her legs, for every clip.

Her skirt hangs from Peach's dress rows (look_skirt.py): the lower band from
row 25's ring (row 25 turns about its origin, just under the hips) to the
hem on row 26 (row 25's child, 243 units down its -z axis). Peach's clips
key both rows for her gown; the gown is not hers, so every clip of hers
takes these keys instead (the owner's requirement, playtest 2026-10-14: in
EVERY animation the dress surrounds her legs, except at the slit):

- row 25 turns so its -z axis (the band's axis) points at the middle of her
  knees and ankles, as seen from its origin, drawn GRAVITY of the way toward
  straight down (the cloth hangs), at most MAX_TILT_DEG from straight down
  the pelvis;
- row 26 scales its ring (local x across her, local y front to back, the
  hem's plane) so the band, a cone from row 25's ring to the hem, holds
  every leg vertex between them with MARGIN to spare: an ellipse, wider
  front to back (a stride) than across, each axis capped (a dress, not a
  bell: a pose wider than that is her own clips' to avoid), the leg on the
  slit's side only up to SLIT_CAP;
- both are smoothed over SMOOTH frames.

Owner playtest p6i (2026-10-14): "Dress is not flowing". The band's axis
follows its target (above) through a damped spring in the model's space
(SPRING, DAMPING): it lags her turns and swings and settles after them,
and a looping clip's spring runs three periods so its keys loop. A pose
group's "skirt" op trails it (its axis drawn toward behind her by "trail":
the run, the dash). The flare is worked out on the axis the spring gives,
so the band still holds her legs where it lags.

The keys are computed on the content's own skeleton and look scene
(coverage.py Fighter.pose_frames; the scene's leg and skirt vertices).

    skirt_keys.py --staging ... --export ... --scene <high scene> [--motions 5,23]
        prints the dress check of the motions with these keys
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
import dress_check as D  # noqa: E402

ROW_PELVIS, ROW_UPPER, ROW_HEM = 1, 25, 26
# The legs: thigh rows (their origins are the knees), shin rows (the ankles).
KNEES = (17, 22)
ANKLES = (18, 23)
# The slit's side: the +x leg (rows 15-19).
SLIT_LEG_ROWS = (16, 17, 18)
MAX_TILT_DEG = 75.0
# How far the band's axis is drawn toward straight down (model space).
GRAVITY = 0.1
MARGIN = 6.0
# Ring scale caps: across her (local x), front to back (local y).
MAX_FLARE_X = 1.5
MAX_FLARE_Y = 2.0
SLIT_CAP = 1.2
SMOOTH = 3
# The band's spring (per frame): the pull toward its target and the share of
# its angular speed kept.
SPRING = 0.3
DAMPING = 0.6


def _align(a, b):
    """The smallest rotation taking unit vector a to unit vector b."""
    v = np.cross(a, b)
    c = float(np.dot(a, b))
    if c < -0.9999:
        return np.diag([1.0, -1.0, -1.0])
    k = np.array([[0, -v[2], v[1]], [v[2], 0, -v[0]], [-v[1], v[0], 0]])
    return np.eye(3) + k + k @ k / (1.0 + c)


class Skirt:
    def __init__(self, fighter: C.Fighter, scene: dict):
        self.f = fighter
        verts = scene["vertices"]
        D.apply_scene_skeleton(fighter, scene)
        self.bind = D.bind_mats(fighter)
        self.inv_bind = [np.linalg.inv(m) for m in self.bind]
        bands, _hem, _slit = D.rings(scene)
        if len(bands) != 2:
            raise SystemExit("the scene's skirt is not on rows 25/26 (look_skirt.py)")
        ring25, hem = bands[1]
        self.ring25 = np.array([verts[i]["model"] for i in ring25])
        self.hem = np.array([verts[i]["model"] for i in hem])
        legs = [i for i, v in enumerate(verts) if v.get("object") == D.LEGS and v["joint"] in D.LEG_ROWS]
        self.leg_model = np.array([verts[i]["model"] for i in legs])
        self.leg_rows = [verts[i]["joint"] for i in legs]
        self.leg_slit = np.array([r in SLIT_LEG_ROWS for r in self.leg_rows])
        # Row 25's rest frame in the pelvis's, and the rings in row 25's and
        # row 26's frames (bind pose).
        b1, b25, b26 = self.bind[ROW_PELVIS], self.bind[ROW_UPPER], self.bind[ROW_HEM]
        self.rest25 = np.linalg.inv(b1) @ b25
        self.ring25_local = (np.linalg.inv(b25) @ np.c_[self.ring25, np.ones(len(self.ring25))].T).T[:, :3]
        self.hem_local = (np.linalg.inv(b26) @ np.c_[self.hem, np.ones(len(self.hem))].T).T[:, :3]
        self.r0 = float(np.linalg.norm(self.ring25_local[:, :2] - self.ring25_local[:, :2].mean(0), axis=1).mean())
        self.r1 = float(np.linalg.norm(self.hem_local[:, :2] - self.hem_local[:, :2].mean(0), axis=1).mean())
        self.length = float(np.linalg.norm((np.linalg.inv(b25) @ b26)[:3, 3]))

    def target_axis(self, mats, trail: float = 0.0):
        """The band's target axis (unit, the pelvis's frame) for one posed
        frame: at the knees and ankles, drawn down, trailed, tilt-capped."""
        pelvis = mats[ROW_PELVIS]
        pinv = np.linalg.inv(pelvis)
        to_pelvis = lambda p: (pinv @ np.append(p, 1.0))[:3]
        origin = self.rest25[:3, 3]
        pts = [to_pelvis(mats[r][:3, 3]) for r in KNEES + ANKLES]
        target = np.mean(pts, axis=0) - origin
        n = np.linalg.norm(target)
        rest_axis = self.rest25[:3, :3] @ np.array([0.0, 0.0, -1.0])
        d = target / n if n > 1e-3 else rest_axis
        down = pinv[:3, :3] @ np.array([0.0, -1.0, 0.0])
        d = (1.0 - GRAVITY) * d + GRAVITY * down / max(np.linalg.norm(down), 1e-6)
        if trail:
            back = pinv[:3, :3] @ np.array([0.0, 0.0, -1.0])
            d = d + trail * back / max(np.linalg.norm(back), 1e-6)
        d /= max(np.linalg.norm(d), 1e-6)
        return self.cap_tilt(d)

    def cap_tilt(self, d):
        rest_axis = self.rest25[:3, :3] @ np.array([0.0, 0.0, -1.0])
        ang = np.degrees(np.arccos(np.clip(np.dot(d, rest_axis), -1.0, 1.0)))
        if ang > MAX_TILT_DEG:
            axis = np.cross(rest_axis, d)
            axis /= max(np.linalg.norm(axis), 1e-6)
            t = np.radians(MAX_TILT_DEG)
            d = rest_axis * np.cos(t) + np.cross(axis, rest_axis) * np.sin(t)
        return d

    def frame_keys(self, mats, d=None) -> tuple:
        """Row 25's local rotation matrix (in the pelvis's frame) and row
        26's ring scales (across, front to back) for one posed frame, the
        band along d (the pelvis's frame; its target when None)."""
        pelvis = mats[ROW_PELVIS]
        pinv = np.linalg.inv(pelvis)
        to_pelvis = lambda p: (pinv @ np.append(p, 1.0))[:3]
        origin = self.rest25[:3, 3]
        rest_axis = self.rest25[:3, :3] @ np.array([0.0, 0.0, -1.0])
        if d is None:
            d = self.target_axis(mats)
        rot = _align(rest_axis, d) @ self.rest25[:3, :3]
        # The ring's axes in the pelvis's frame (row 25's local x and y).
        ex, ey = rot[:, 0], rot[:, 1]
        legs = np.array([to_pelvis((mats[r] @ self.inv_bind[r] @ np.append(p, 1.0))[:3])
                         for p, r in zip(self.leg_model, self.leg_rows)])
        rel = legs - origin
        along = rel @ d
        t = along / self.length
        px, py = np.abs(rel @ ex), np.abs(rel @ ey)
        inside = (t > 0.15) & (t <= 1.0)
        def need(coord):
            out = np.ones(len(legs))
            out[inside] = (coord[inside] + MARGIN - self.r0 * (1.0 - t[inside])) / (self.r1 * t[inside])
            return out
        nx, ny = need(px), need(py)
        def flare(n_, cap):
            body = float(n_[inside & ~self.leg_slit].max(initial=1.0))
            slit = float(n_[inside & self.leg_slit].max(initial=1.0))
            return min(max(1.0, body, min(slit, SLIT_CAP)), cap)
        return rot, (flare(nx, MAX_FLARE_X), flare(ny, MAX_FLARE_Y))

    def flow(self, frames, targets, loop) -> list:
        """The band's axis per frame (the pelvis's frame): each frame's
        target followed through the spring in the model's space; a looping
        clip runs three periods and keeps the last."""
        world = [frames[i][1][ROW_PELVIS][:3, :3] @ targets[i] for i in range(len(targets))]
        n = len(world)
        start = loop if (loop is not None and 0 <= loop < n - 1) else None
        order = list(range(n))
        if start is not None:
            period = list(range(start, n))
            order = list(range(n)) + period + period
        s = world[order[0]] / max(np.linalg.norm(world[order[0]]), 1e-6)
        v = np.zeros(3)
        out = [None] * n
        for i in order:
            t = world[i] / max(np.linalg.norm(world[i]), 1e-6)
            v = v * DAMPING + SPRING * np.cross(s, t)
            ang = float(np.linalg.norm(v))
            if ang > 1e-9:
                k = v / ang
                s = s * np.cos(ang) + np.cross(k, s) * np.sin(ang) + k * np.dot(k, s) * (1 - np.cos(ang))
            s = s / max(np.linalg.norm(s), 1e-6)
            out[i] = s
        pel = [frames[i][1][ROW_PELVIS][:3, :3] for i in range(n)]
        local = []
        for i in range(n):
            d = np.linalg.inv(pel[i]) @ out[i]
            local.append(self.cap_tilt(d / max(np.linalg.norm(d), 1e-6)))
        return local

    def clip_keys(self, clip: A.Clip, anim_flags: int, trail: float = 0.0):
        length = max((st.length for st in clip.streams.values()), default=0)
        frames = self.f.pose_frames(clip, anim_flags, length)
        loops = {st.loop for st in clip.streams.values()}
        loop = loops.pop() if len(loops) == 1 else None
        targets = [self.target_axis(mats, trail) for _root, mats in frames]
        axes = self.flow(frames, targets, loop)
        rots, scales = [], []
        for (_root, mats), d in zip(frames, axes):
            r, s = self.frame_keys(mats, d)
            rots.append(r)
            scales.append(s)
        # Smoothing: the axis and the flare, a centred window.
        n = len(rots)
        axes = np.array([r @ np.array([0.0, 0.0, -1.0]) for r in rots])
        sm_axes, sm_scales = [], []
        for i in range(n):
            lo, hi = max(0, i - SMOOTH // 2), min(n, i + SMOOTH // 2 + 1)
            a = axes[lo:hi].mean(axis=0)
            sm_axes.append(a / max(np.linalg.norm(a), 1e-6))
            sm_scales.append((max(x for x, _y in scales[lo:hi]), max(y for _x, y in scales[lo:hi])))
        rest_axis = self.rest25[:3, :3] @ np.array([0.0, 0.0, -1.0])
        out = []
        prev = tuple(self.f.skel.rotate[ROW_UPPER])
        for a, s in zip(sm_axes, sm_scales):
            rot = _align(rest_axis, a) @ self.rest25[:3, :3]
            prev = A.to_euler(rot, prev)
            out.append((prev, s))
        return out

    def apply(self, clip: A.Clip, anim_flags: int, trail: float = 0.0) -> A.Clip:
        """The clip with rows 25 and 26 keyed from her legs (the band
        trailed behind her by `trail`)."""
        keys = self.clip_keys(clip, anim_flags, trail)
        length = len(keys) - 1
        lead_slot = C.first_slot(anim_flags)
        loops = {st.loop for st in clip.streams.values()}
        loop = loops.pop() if len(loops) == 1 else None
        rows = self.f.rows
        slot_of = (lambda row: lead_slot + rows.index(row)) if rows is not None else (lambda row: lead_slot + row)
        # The joint array reaches the skirt's rows (a clip whose array ended
        # before them had none to point at).
        out = A.Clip(max(clip.slots, slot_of(ROW_HEM) + 1))
        out.streams = dict(clip.streams)
        st25 = A.Stream(length, loop)
        for k in range(3):
            st25.tracks[k] = A.Track([round(r[k] * A.ROT_UNIT) for r, _s in keys], 0)
        st26 = A.Stream(length, loop)
        for k in range(2):
            st26.tracks[7 + k] = A.Track([round(s[k] * C.SCALE_UNIT) for _r, s in keys], 0)
        out.streams[slot_of(ROW_UPPER)] = st25
        out.streams[slot_of(ROW_HEM)] = st26
        return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--staging", type=Path, required=True)
    ap.add_argument("--export", type=Path, required=True)
    ap.add_argument("--scene", type=Path, required=True)
    ap.add_argument("--motions")
    ap.add_argument("--json", type=Path)
    args = ap.parse_args()
    scene = json.loads(args.scene.read_text(encoding="utf-8"))
    fighter = C.load(args.staging, args.export)
    sk = Skirt(fighter, scene)
    wanted = {int(x, 0) for x in args.motions.split(",")} if args.motions else None
    report = D.Report(fighter, scene)
    rows = []
    seen = {}
    for m in fighter.resolved["motions"]:
        if not m["anim_file_id"] or (wanted is not None and m["index"] not in wanted):
            continue
        key = (m["anim_file_id"], m["anim_flags"])
        if key not in seen:
            clip = A.decode(fighter.rom.o2r(m["anim_file_id"]))
            keyed = sk.apply(clip, m["anim_flags"])
            seen[key] = report.clip(keyed, m["anim_flags"])
        frames_n, bad, worst, worst_slit = seen[key]
        rows.append({"motion": m["index"], "clip": f"{m['anim_file_id']:#x}", "frames": frames_n,
                     "frames_out": bad, "worst": round(worst, 1), "worst_slit": round(worst_slit, 1)})
    rows.sort(key=lambda r: -r["worst"])
    for r in rows[:30]:
        print(r)
    clean = sum(1 for r in rows if r["worst"] <= 0)
    print(f"skirt keys: {clean} of {len(rows)} motions clean (away from the slit)")
    if args.json:
        args.json.write_text(json.dumps(rows, indent=1) + "\n", encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
