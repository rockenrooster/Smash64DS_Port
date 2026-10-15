#!/usr/bin/env python3
"""P6: keys for Palutena's skirt rows, from her legs, for every clip.

Her skirt hangs from Peach's dress rows (look_skirt.py): the lower band from
row 25's ring (row 25 turns about its origin, just under the hips) to the
hem on row 26 (row 25's child, 243 units down its -z axis). Peach's clips
key both rows for her gown; the gown is not hers, so every clip of hers
takes these keys instead (the owner's requirement, playtest 2026-10-14: in
EVERY animation the dress surrounds her legs):

- row 25 turns so its -z axis (the band's axis) points at the middle of her
  knees and ankles, as seen from its origin, drawn GRAVITY of the way toward
  straight down (the cloth hangs), at most MAX_TILT_DEG from straight down
  the pelvis, and turns about that axis so its local y lies along her legs'
  spread (one leg's knee and ankle from the other's, across the axis);
- row 26 shapes the hem: its ring scaled across (sx) and along the spread
  (sy), tilted about the across axis (one end of the spread lifted toward
  the waist, the other let down) and moved along the spread (dy) and up the
  band (dz), so the band (dress_check.band_outs: the surface from row 25's
  ring to the hem) holds her thighs and knees inside it, above the hem, and
  keeps her shins from coming through its side, with MARGIN to spare, the
  hem as near its rest shape as that allows. Owner p6j (2026-10-14), from
  the 3DS: "the slit leg actually never comes out of the dress at all" (both
  legs are held) and "if Palutena takes a step forward where the knee pokes
  out, the dress should be stretched over it": a knee raised ahead lifts
  the hem's front over it (the shin hangs from under it, as the 3DS's
  does), the back hanging behind the other leg; a stride stretches it along
  the stride. Each frame's hem is the least costly (HEM_SCALE a unit of
  each change from rest, OUT_UNIT a unit of a point's distance outside,
  TEMPORAL a share of the change from the frame before, by HEM_STEP), found
  by a Nelder-Mead search within HEM_LO..HEM_HI from the frame before's.

Owner playtest p6i (2026-10-14): "Dress is not flowing". The band's axis
follows its target (above) through a damped spring in the model's space
(SPRING, DAMPING): it lags her turns and swings (at most MAX_LAG_DEG) and
settles after them, and a looping clip's spring runs three periods so its
keys loop. A pose
group's "skirt" op trails it (its axis drawn toward behind her by "trail":
the run, the dash). The hem is solved on the axis the spring gives, so the
band still holds her legs where it lags.

Where no hem can hold a leg (a clip made for another fighter: a lunge or a
kick past the dress), the leg is held: its thigh and shin turned toward
their rest (straight down) as far as the dress then holds it (HOLD_STEPS of
the way, tested on the frame alone), eased in and out over the frames
around (HOLD_RAMP a frame), and the clip solved again from the held legs,
up to HOLD_PASSES times while a frame is still out (owner p6j: on the 3DS
her leg never leaves her dress).

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
# The legs: shin rows (their origins are the knees), foot rows (the ankles).
KNEES = (17, 22)
ANKLES = (18, 23)
MAX_TILT_DEG = 75.0
# How far the band's axis is drawn toward straight down (model space).
GRAVITY = 0.1
MARGIN = 6.0
# The ring's turn about the band's axis toward her legs' spread: at most
# MAX_TWIST_DEG, eased in from SPREAD_MIN to SPREAD_FULL units of spread.
MAX_TWIST_DEG = 60.0
SPREAD_MIN, SPREAD_FULL = 30.0, 120.0
# The hem's shape (row 26 in row 25's frame): scale across, scale along the
# spread, tilt (radians; + lifts the +y end), move along the spread, move up
# the band. Its limits, the change that costs a unit, the change from the
# frame before that costs TEMPORAL; a point OUT_UNIT outside costs a unit
# (times its weight); the hem keeps MIN_SPAN below row 25's ring each way.
HEM_REST = np.array([1.0, 1.0, 0.0, 0.0, 0.0])
HEM_LO = np.array([0.9, 0.9, np.radians(-50.0), -100.0, -10.0])
HEM_HI = np.array([1.45, 1.75, np.radians(50.0), 100.0, 140.0])
HEM_SCALE = np.array([0.4, 0.6, np.radians(45.0), 90.0, 150.0])
HEM_STEP = np.array([0.08, 0.12, np.radians(6.0), 15.0, 15.0])
TEMPORAL = 0.3
OUT_UNIT = 2.5
MIN_SPAN = 40.0
# The cloth's length: each panel (row 25's ring vertex to its hem vertex)
# shortened by PANEL_SHORTEN or stretched by PANEL_STRETCH costs PANEL_W
# over the panels (a hem lifted all round is a shorter dress, not one held
# up by a knee).
PANEL_SHORTEN, PANEL_STRETCH, PANEL_W = 35.0, 60.0, 3.0
SEARCH_EVALS = 260
# Holding a leg the hem cannot: its thigh and shin turned these shares of
# the way to their rest, the least that clears (a frame's share less
# HOLD_RAMP each frame away, around it).
HOLD_ROWS = ((16, 17), (21, 22))
HOLD_STEPS = (0.15, 0.3, 0.45, 0.6, 0.75, 0.9, 1.0)
HOLD_RAMP = 0.2
HOLD_PASSES = 4
SMOOTH = 3
# The band's spring (per frame): the pull toward its target and the share of
# its angular speed kept.
SPRING = 0.3
DAMPING = 0.6
# The most the band's axis lags its target (a tumble's spin would leave her
# legs behind the dress).
MAX_LAG_DEG = 25.0


def _align(a, b):
    """The smallest rotation taking unit vector a to unit vector b."""
    v = np.cross(a, b)
    c = float(np.dot(a, b))
    if c < -0.9999:
        return np.diag([1.0, -1.0, -1.0])
    k = np.array([[0, -v[2], v[1]], [v[2], 0, -v[0]], [-v[1], v[0], 0]])
    return np.eye(3) + k + k @ k / (1.0 + c)


def _turn(axis, ang):
    """The rotation by ang (radians) about a unit axis."""
    k = np.array([[0, -axis[2], axis[1]], [axis[2], 0, -axis[0]], [-axis[1], axis[0], 0]])
    return np.eye(3) + np.sin(ang) * k + (1.0 - np.cos(ang)) * (k @ k)


def _toward(R, Rb, a):
    """Rotation R turned share a of the way to Rb."""
    Q = Rb @ R.T
    ang = float(np.arccos(np.clip((np.trace(Q) - 1.0) / 2.0, -1.0, 1.0)))
    if ang < 1e-6:
        return R
    axis = np.array([Q[2, 1] - Q[1, 2], Q[0, 2] - Q[2, 0], Q[1, 0] - Q[0, 1]])
    n = np.linalg.norm(axis)
    if n < 1e-9:
        return R
    return _turn(axis / n, a * ang) @ R


def _search(f, x0, lo, hi, step=0.5, evals=SEARCH_EVALS, tol=1e-3):
    """Nelder-Mead over a box (points clipped into it): (best x, its cost)."""
    n = len(x0)
    clip = lambda x: np.minimum(np.maximum(x, lo), hi)
    pts = [clip(np.array(x0, dtype=float))]
    for i in range(n):
        x = pts[0].copy()
        x[i] = x[i] + step if x[i] + step <= hi[i] else x[i] - step
        pts.append(clip(x))
    vals = [f(x) for x in pts]
    used = len(vals)
    while used < evals:
        order = np.argsort(vals, kind="stable")
        pts = [pts[i] for i in order]
        vals = [vals[i] for i in order]
        if vals[-1] - vals[0] < tol:
            break
        centre = np.mean(pts[:-1], axis=0)
        xr = clip(centre + (centre - pts[-1]))
        fr = f(xr)
        used += 1
        if fr < vals[0]:
            xe = clip(centre + 2.0 * (centre - pts[-1]))
            fe = f(xe)
            used += 1
            pts[-1], vals[-1] = (xe, fe) if fe < fr else (xr, fr)
        elif fr < vals[-2]:
            pts[-1], vals[-1] = xr, fr
        else:
            xc = clip(centre + 0.5 * ((xr if fr < vals[-1] else pts[-1]) - centre))
            fc = f(xc)
            used += 1
            if fc < min(fr, vals[-1]):
                pts[-1], vals[-1] = xc, fc
            else:
                for i in range(1, len(pts)):
                    pts[i] = clip(pts[0] + 0.5 * (pts[i] - pts[0]))
                    vals[i] = f(pts[i])
                used += n
    i = int(np.argmin(vals))
    return pts[i], vals[i]


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
        self.legs = D.LegSamples(scene)
        # Row 25's rest frame in the pelvis's, and the rings in row 25's and
        # row 26's frames (bind pose).
        b1, b25, b26 = self.bind[ROW_PELVIS], self.bind[ROW_UPPER], self.bind[ROW_HEM]
        self.rest25 = np.linalg.inv(b1) @ b25
        ring_model = np.array([verts[i]["model"] for i in ring25])
        hem_model = np.array([verts[i]["model"] for i in hem])
        self.ring25_local = (np.linalg.inv(b25) @ np.c_[ring_model, np.ones(len(ring_model))].T).T[:, :3]
        self.hem_local = (np.linalg.inv(b26) @ np.c_[hem_model, np.ones(len(hem_model))].T).T[:, :3]
        self.hem_rest_t = np.array(self.f.skel.translate[ROW_HEM], dtype=float)
        self.top_z = float(self.ring25_local[:, 2].mean())
        self.panel_rest = np.linalg.norm(self.hem_points(HEM_REST) - self.ring25_local, axis=1)
        # The band is measured about row 25's own axis (its -z through its
        # ring's centre, dress_check.band_outs): the ring's profile is fixed.
        self.axis_xy = self.ring25_local[:, :2].mean(axis=0)
        self.top = D._periodic(*self._profile(self.ring25_local))
        self.upper_rest = D.upper_rest_of(scene, self.bind)

    # ------------------------------------------------------------ the axis

    def rest_axis(self):
        return self.rest25[:3, :3] @ np.array([0.0, 0.0, -1.0])

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
        d = target / n if n > 1e-3 else self.rest_axis()
        down = pinv[:3, :3] @ np.array([0.0, -1.0, 0.0])
        d = (1.0 - GRAVITY) * d + GRAVITY * down / max(np.linalg.norm(down), 1e-6)
        if trail:
            back = pinv[:3, :3] @ np.array([0.0, 0.0, -1.0])
            d = d + trail * back / max(np.linalg.norm(back), 1e-6)
        d /= max(np.linalg.norm(d), 1e-6)
        return self.cap_tilt(d)

    def cap_tilt(self, d):
        rest_axis = self.rest_axis()
        ang = np.degrees(np.arccos(np.clip(np.dot(d, rest_axis), -1.0, 1.0)))
        if ang > MAX_TILT_DEG:
            axis = np.cross(rest_axis, d)
            axis /= max(np.linalg.norm(axis), 1e-6)
            t = np.radians(MAX_TILT_DEG)
            d = rest_axis * np.cos(t) + np.cross(axis, rest_axis) * np.sin(t)
        return d

    def spread_twist(self, mats, d, rot) -> float:
        """The turn (radians) about the band's axis d putting the ring's
        local y (rot's) along her legs' spread across the axis: the one
        leg's knee and ankle from the other's (the pelvis's frame), eased in
        from SPREAD_MIN to SPREAD_FULL units of spread, at most
        MAX_TWIST_DEG either way (the nearer end of the spread)."""
        pinv = np.linalg.inv(mats[ROW_PELVIS])
        to_pelvis = lambda p: (pinv @ np.append(p, 1.0))[:3]
        left = np.mean([to_pelvis(mats[r][:3, 3]) for r in (KNEES[0], ANKLES[0])], axis=0)
        right = np.mean([to_pelvis(mats[r][:3, 3]) for r in (KNEES[1], ANKLES[1])], axis=0)
        v = left - right
        v = v - np.dot(v, d) * d
        size = float(np.linalg.norm(v))
        w = min(max((size - SPREAD_MIN) / (SPREAD_FULL - SPREAD_MIN), 0.0), 1.0)
        if w <= 0.0:
            return 0.0
        v = v / size
        ey = rot[:, 1]
        ang = float(np.arctan2(np.dot(np.cross(ey, v), d), np.dot(ey, v)))
        if ang > np.pi / 2:
            ang -= np.pi
        elif ang < -np.pi / 2:
            ang += np.pi
        lim = np.radians(MAX_TWIST_DEG)
        return w * min(max(ang, -lim), lim)

    def band_rot(self, d, twist):
        """Row 25's local rotation (the pelvis's frame): its -z along d,
        turned about it by twist."""
        return _turn(d, twist) @ _align(self.rest_axis(), d) @ self.rest25[:3, :3]

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
            lag = float(np.arccos(np.clip(np.dot(s, t), -1.0, 1.0)))
            if lag > np.radians(MAX_LAG_DEG):
                k = np.cross(s, t)
                k = k / max(np.linalg.norm(k), 1e-9)
                s = _turn(k, lag - np.radians(MAX_LAG_DEG)) @ s
                v = v * 0.5
            out[i] = s
        pel = [frames[i][1][ROW_PELVIS][:3, :3] for i in range(n)]
        local = []
        for i in range(n):
            d = np.linalg.inv(pel[i]) @ out[i]
            local.append(self.cap_tilt(d / max(np.linalg.norm(d), 1e-6)))
        return local

    # ------------------------------------------------------------- the hem

    def hem_points(self, p) -> np.ndarray:
        """The hem ring in row 25's frame for hem shape p."""
        sx, sy, tilt, dy, dz = p
        h = self.hem_local
        x, y, z = h[:, 0] * sx, h[:, 1] * sy, h[:, 2]
        c, s = np.cos(tilt), np.sin(tilt)
        t = self.hem_rest_t
        return np.c_[x + t[0], y * c - z * s + t[1] + dy, y * s + z * c + t[2] + dz]

    def legs_local(self, mats, rot) -> np.ndarray:
        """Her leg points in row 25's frame (rot its rotation in the
        pelvis's)."""
        m25 = np.eye(4)
        m25[:3, :3] = rot
        m25[:3, 3] = self.rest25[:3, 3]
        inv = np.linalg.inv(mats[ROW_PELVIS] @ m25)
        pts = self.legs.posed(mats, self.inv_bind)
        return (inv @ np.c_[pts, np.ones(len(pts))].T).T[:, :3]

    def _profile(self, pts):
        """Points (row 25's frame) by angle about the band's axis: (angle,
        depth down the axis, radius)."""
        rel = pts[:, :2] - self.axis_xy
        return np.arctan2(rel[:, 1], rel[:, 0]), -pts[:, 2], np.hypot(rel[:, 0], rel[:, 1])

    def frame_legs(self, legs):
        """One frame's leg points for the hem search: their angle, depth and
        radius about the axis, and the ring's depth and radius there."""
        ang, depth, rho = self._profile(legs)
        xt, (at, rt) = self.top
        return ang, depth, rho, np.interp(ang, xt, at), np.interp(ang, xt, rt)

    def outs(self, p, fl) -> np.ndarray:
        """Per leg point, how far outside the band with hem shape p
        (dress_check.band_outs about row 25's axis)."""
        ang, depth, rho, a_top, r_top = fl
        xh, (ah, rh) = D._periodic(*self._profile(self.hem_points(p)))
        return D.band_profile_outs(depth, rho, a_top, r_top, np.interp(ang, xh, ah),
                                   np.interp(ang, xh, rh), self.legs.core)

    def full_outs(self, p, legs, rot) -> np.ndarray:
        """outs, with the core above the band's top ring held to the upper
        band (dress_check.Report.frame_outs)."""
        fl = self.frame_legs(legs)
        out = self.outs(p, fl)
        if self.upper_rest is None:
            return out
        above = (fl[1] < fl[3]) & self.legs.core
        if not above.any():
            return out
        pel = legs @ rot.T + self.rest25[:3, 3]
        return np.where(above, D.upper_outs(pel, self.upper_rest), out)

    def cost(self, p, fl, prev) -> float:
        hem = self.hem_points(p)
        # The axis inside the hem (its angles all round it).
        h_ang = np.sort(np.arctan2(hem[:, 1] - self.axis_xy[1], hem[:, 0] - self.axis_xy[0]))
        if float(np.max(np.diff(np.append(h_ang, h_ang[0] + 2 * np.pi)))) >= np.pi * 0.9:
            return 1e6
        o = np.maximum(self.outs(p, fl) + MARGIN, 0.0) / OUT_UNIT
        c = float(np.sum(((p - HEM_REST) / HEM_SCALE) ** 2)) + float(np.sum(self.legs.weight * o * o))
        if prev is not None:
            c += TEMPORAL * float(np.sum(((p - prev) / HEM_STEP) ** 2))
        panel = np.linalg.norm(hem - self.ring25_local, axis=1) - self.panel_rest
        c += PANEL_W * float(np.mean((np.minimum(panel, 0.0) / PANEL_SHORTEN) ** 2 +
                                     (np.maximum(panel, 0.0) / PANEL_STRETCH) ** 2))
        lift = hem[:, 2] - (self.top_z - MIN_SPAN)
        return c + float(np.sum(np.maximum(lift, 0.0) ** 2)) / 25.0

    def solve_hem(self, legs, prev):
        """The hem shape for one frame's legs (row 25's frame), searched
        from the frame before's (prev) and from rest."""
        fl = self.frame_legs(legs)
        lo, hi = (HEM_LO - HEM_REST) / HEM_SCALE, (HEM_HI - HEM_REST) / HEM_SCALE
        f = lambda x: self.cost(HEM_REST + x * HEM_SCALE, fl, prev)
        starts = [(np.zeros(5), 0.5)] if prev is None else [((prev - HEM_REST) / HEM_SCALE, 0.2),
                                                            (np.zeros(5), 0.5)]
        best = None
        for x0, step in starts:
            if best is not None and f(x0) >= best[1]:
                continue
            x, v = _search(f, x0, lo, hi, step)
            if best is None or v < best[1]:
                best = (x, v)
        return HEM_REST + best[0] * HEM_SCALE

    # ------------------------------------------------------------ the clip

    def clip_keys(self, clip: A.Clip, anim_flags: int, trail: float = 0.0):
        """Per frame: row 25's rotation (Euler) and the hem's shape."""
        return self._solve(clip, anim_flags, trail)["keys"]

    def _solve(self, clip: A.Clip, anim_flags: int, trail: float = 0.0) -> dict:
        length = max((st.length for st in clip.streams.values()), default=0)
        frames = self.f.pose_frames(clip, anim_flags, length)
        loops = {st.loop for st in clip.streams.values()}
        loop = loops.pop() if len(loops) == 1 else None
        targets = [self.target_axis(mats, trail) for _root, mats in frames]
        axes = self.flow(frames, targets, loop)
        rest_rot = lambda d: _align(self.rest_axis(), d) @ self.rest25[:3, :3]
        twists = [self.spread_twist(mats, d, rest_rot(d)) for (_root, mats), d in zip(frames, axes)]
        # Smoothing: the axis and the twist, a centred window.
        n = len(axes)
        rots = []
        for i in range(n):
            lo, hi = max(0, i - SMOOTH // 2), min(n, i + SMOOTH // 2 + 1)
            a = np.mean(axes[lo:hi], axis=0)
            rots.append(self.band_rot(a / max(np.linalg.norm(a), 1e-6), float(np.mean(twists[lo:hi]))))
        legs = [self.legs_local(mats, rot) for (_root, mats), rot in zip(frames, rots)]
        # The hem, frame by frame from the one before (a loop's period twice,
        # so its first frame follows its last).
        order = list(range(n))
        if loop is not None and 0 <= loop < n - 1:
            order += list(range(loop, n))
        hems = [None] * n
        prev = None
        for i in order:
            prev = self.solve_hem(legs[i], prev)
            hems[i] = prev
        out = []
        prev_e = tuple(self.f.skel.rotate[ROW_UPPER])
        for rot, hem in zip(rots, hems):
            prev_e = A.to_euler(rot, prev_e)
            out.append((prev_e, hem))
        return {"keys": out, "frames": frames, "rots": rots, "legs": legs, "hems": hems, "length": length}

    # ------------------------------------------------------------ the hold

    def _held_mats(self, mats, leg, a):
        """mats with one leg's thigh and shin turned share a of the way to
        their rest (its foot following)."""
        m = list(mats)
        for row in HOLD_ROWS[leg] + (ANKLES[leg],):
            p = self.f.skel.parents[row]
            local = np.linalg.inv(mats[p]) @ mats[row]
            if row in HOLD_ROWS[leg]:
                M = local[:3, :3]
                sc = np.linalg.norm(M, axis=0)
                local = local.copy()
                local[:3, :3] = _toward(M / sc, A.euler(*self.f.skel.rotate[row]), a) * sc
            m[row] = m[p] @ local
        return m

    def hold_need(self, solved, frames0, held) -> np.ndarray:
        """Per leg and frame, the share of the way to rest the leg needs:
        where the solved clip still has it out, the least of HOLD_STEPS past
        its share held now (`held`) that clears it, tested on that frame of
        the clip as it came (frames0) alone; elsewhere its share now."""
        rots, legs, hems = solved["rots"], solved["legs"], solved["hems"]
        need = held.copy()
        for i in range(len(rots)):
            out = self.full_outs(hems[i], legs[i], rots[i])
            for leg in (0, 1):
                mask = self.legs.leg == leg
                if float(out[mask].max(initial=-1.0)) <= D.TOLERANCE:
                    continue
                steps = [a for a in HOLD_STEPS if a > held[leg, i] + 1e-6]
                if not steps:
                    continue
                # The least step that clears, by halving (a leg turned
                # further toward rest is no further out).
                lo, hi = 0, len(steps) - 1
                while lo < hi:
                    mid = (lo + hi) // 2
                    l2 = self.legs_local(self._held_mats(frames0[i][1], leg, steps[mid]), rots[i])
                    o2 = self.full_outs(self.solve_hem(l2, hems[i]), l2, rots[i])
                    if float(o2[mask].max(initial=-1.0)) <= D.TOLERANCE:
                        hi = mid
                    else:
                        lo = mid + 1
                need[leg, i] = steps[lo]
        return need

    @staticmethod
    def _ease(need) -> np.ndarray:
        """Each frame's share, eased in and out over the frames around."""
        held = np.zeros_like(need)
        cols = np.nonzero(need.any(axis=0))[0]
        for i in range(need.shape[1]):
            for j in cols:
                held[:, i] = np.maximum(held[:, i], need[:, j] - HOLD_RAMP * abs(i - j))
        return np.clip(held, 0.0, 1.0)

    def _slot_of(self, anim_flags):
        lead_slot = C.first_slot(anim_flags)
        rows = self.f.rows
        return (lambda row: lead_slot + rows.index(row)) if rows is not None else (lambda row: lead_slot + row)

    def hold(self, clip: A.Clip, anim_flags: int, frames, held) -> A.Clip:
        """The clip (posed as frames) with each leg's thigh and shin turned
        `held` (per leg and frame) of the way to their rest."""
        length = len(frames) - 1
        slot_of = self._slot_of(anim_flags)
        out = A.Clip(clip.slots)
        out.streams = dict(clip.streams)
        for leg in (0, 1):
            if not held[leg].any():
                continue
            for row in HOLD_ROWS[leg]:
                slot = slot_of(row)
                st = clip.streams.get(slot)
                if st is None or not any(k in st.tracks for k in range(3)):
                    continue   # not turned: at its rest already
                if st.length < length and st.loop is not None:
                    continue
                rest = A.euler(*self.f.skel.rotate[row])
                p = self.f.skel.parents[row]
                new = A.Stream(length, st.loop, dict(st.tracks), st.loop_skips_steps)
                if st.length < length:
                    for k, t in st.tracks.items():
                        new.tracks[k] = A.Track(list(t.values) + [t.values[-1]] * (length - st.length),
                                                t.lead, t.step)
                cols = {k: (list(new.tracks[k].values), new.tracks[k].lead) if k in new.tracks
                        else ([round(self.f.skel.rotate[row][k] * A.ROT_UNIT)] * (length + 1), 0)
                        for k in range(3)}
                for f in range(length + 1):
                    a = float(held[leg, f])
                    if a <= 0.0:
                        continue
                    mats = frames[f][1]
                    local = np.linalg.inv(mats[p]) @ mats[row]
                    M = local[:3, :3]
                    R = M / np.linalg.norm(M, axis=0)
                    near = tuple((cols[k][0][f - cols[k][1]] if f >= cols[k][1] else cols[k][0][0]) / A.ROT_UNIT
                                 for k in range(3))
                    e = A.to_euler(_toward(R, rest, a), near)
                    for k in range(3):
                        vals, lead = cols[k]
                        if f >= lead:
                            vals[f - lead] = round(e[k] * A.ROT_UNIT)
                for k in range(3):
                    vals, lead = cols[k]
                    step = new.tracks[k].step if k in new.tracks else None
                    new.tracks[k] = A.Track(vals, lead, step)
                out.streams[slot] = new
        return out

    def apply(self, clip: A.Clip, anim_flags: int, trail: float = 0.0) -> A.Clip:
        """The clip with rows 25 and 26 keyed from her legs (the band
        trailed behind her by `trail`), her legs held where the dress cannot
        hold them."""
        solved = self._solve(clip, anim_flags, trail)
        frames0 = solved["frames"]
        held = np.zeros((2, len(frames0)))
        source = clip
        for _ in range(HOLD_PASSES):
            need = self.hold_need(solved, frames0, held)
            if np.array_equal(need, held):
                break
            held = self._ease(need)
            clip = self.hold(source, anim_flags, frames0, held)
            solved = self._solve(clip, anim_flags, trail)
        keys = solved["keys"]
        length = len(keys) - 1
        loops = {st.loop for st in clip.streams.values()}
        loop = loops.pop() if len(loops) == 1 else None
        slot_of = self._slot_of(anim_flags)
        # The joint array reaches the skirt's rows (a clip whose array ended
        # before them had none to point at).
        out = A.Clip(max(clip.slots, slot_of(ROW_HEM) + 1))
        out.streams = dict(clip.streams)
        st25 = A.Stream(length, loop)
        for k in range(3):
            st25.tracks[k] = A.Track([round(r[k] * A.ROT_UNIT) for r, _h in keys], 0)
        st26 = A.Stream(length, loop)
        t = self.hem_rest_t
        st26.tracks[0] = A.Track([round(h[2] * A.ROT_UNIT) for _r, h in keys], 0)
        st26.tracks[5] = A.Track([round((t[1] + h[3]) * A.TRANS_UNIT) for _r, h in keys], 0)
        st26.tracks[6] = A.Track([round((t[2] + h[4]) * A.TRANS_UNIT) for _r, h in keys], 0)
        for k in range(2):
            st26.tracks[7 + k] = A.Track([round(h[k] * C.SCALE_UNIT) for _r, h in keys], 0)
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
    print(f"skirt keys: {clean} of {len(rows)} motions clean (both legs)")
    if args.json:
        args.json.write_text(json.dumps(rows, indent=1) + "\n", encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
