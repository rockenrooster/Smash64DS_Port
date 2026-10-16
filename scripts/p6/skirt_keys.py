#!/usr/bin/env python3
"""P6: keys for Palutena's dress rows, from a cloth model, for every clip.

Her skirt (look_skirt.py) is three rings: row 25's ring at her hips (row 25
turns about its origin, just under the hips), the mid ring on row 26 about
her knees (row 25's child) and the hem on row 27 (row 26's child); the band
from each ring to the next is drawn between them. Every clip of hers takes
these rows' keys (the owner's requirement, playtest 2026-10-14: in EVERY
animation the dress surrounds her legs).

Owner p6s (2026-10-15): "Skirt/hem issues, skirt behaves like a paper cone,
instead of a heavy, flowy dress"; the hem "flat against the ground" when she
kneels or crouches, "more flat relative to the horizontal plane" when she
runs; no twitching or spinning (the forward tilt, the wait, the run). The
keys come from a cloth model baked over the clip (Skirt.cloth):

- one chain a sample of the rings (look_skirt.SAMPLES' u): from its point on
  row 25's ring (carried by the pelvis) to its mid ring point and its hem
  point, two segments no longer than their rest by STRETCH, nor shorter
  than SQUASH of it;
- each chain hangs: its segments point the way the dress's rest shape about
  the pelvis does, drawn toward straight down (HANG near the hips, HANG_HEM
  below); in a clip made from Ultimate's motion its points are drawn
  GUIDE_W of the way to where Ultimate's own skirt chains put them
  (ultimate.py's guide: the chains' third joints for the mid ring, their
  ends for the hem, moved from their rest as Ultimate's are);
- each point follows that target through a damped spring (SPRING_MID,
  SPRING_HEM: the hem lags more, the cloth's weight), then the constraints
  hold (FEEDBACK of their move kept as speed): the lengths, the rings'
  edges (RING_STRETCH), her legs -- each thigh and shin a capsule of
  LEG_RADIUS of the leg mesh's radius plus MARGIN, pushing the points, the chains' segments
  and the rings' edges out from her middle (never to the nearer side: a
  kick would pass the dress under the thigh), so a raised knee stretches
  the dress over it (owner p6j) -- the hem's points drawn LEVEL with each
  other, and, on a clip with her feet on a floor, the points kept on it
  (FLOOR_Y) and spread out from her: kneeling, the hem lies on the ground;
- a looping clip runs three periods and keeps the last, so its keys loop.
  (A cloth driven by velocity from the constraints' moves blew up where a
  crouch put the rest dress under the floor.)

Each frame's rows are then fitted to the cloth's points (the pelvis's
frame): row 25's turn by the least-squares rotation of the chains about its
origin (its twist about the band held to MAX_TWIST_DEG), row 26's and row
27's translation, turn and scale across (x, y) by least squares on their
rings' samples, from the frame before's (TEMPORAL) and near their rest
(REST). The rows' keys are put on shared knots, straight between them
(KNOT_EPS), so the clip compacts small. Her legs are as the clip has them
(the old hem solver turned a leg it could not hold toward its rest).

Row 27 was Peach's held joint (the grab's and heavy items': the scene's
"joint_roles" moved it to row 28): a clip that installs its hidden part keys
it one slot later.

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

ROW_PELVIS, ROW_UPPER, ROW_MID, ROW_HEM = 1, 25, 26, 27
# Her hair panels' row (look_blend.py: the head's child) and the bodice they
# hung from: a clip that does not turn the hair itself (all but the Ultimate
# ones, ultimate.py) keeps it on the bodice as the look had it.
ROW_HAIR, ROW_HAIR_FROM, ROW_HEAD = 9, 2, 8
# The legs: (hip, knee, ankle) rows of each (dress_check.LEG_CHAINS; the
# thigh row's origin is the hip, the shin's the knee, the foot's the ankle).
LEGS = ((16, 17, 18), (21, 22, 23))
# The cloth (model units, per frame of the clip):
# Each chain hangs from its point on row 25's ring: its segments point the
# way the dress's rest shape about the pelvis does, turned back toward
# hanging as they were by HANG (the upper band, near the hips) and HANG_HEM
# (the lower) of the pelvis's tilt from upright.
HANG, HANG_HEM = 0.4, 0.65
# A guided clip: the points drawn this share of the way to where Ultimate's
# chains put them.
GUIDE_W = 0.65
# Each point follows its target through a damped spring (per frame): the
# hem lags more than the mid ring (the cloth's weight).
SPRING_MID, SPRING_HEM = 0.30, 0.16
DAMP = 0.75                 # share of the critical damping
SUB = 2                     # spring steps a frame
STRETCH = 0.06              # a segment no longer than its rest by this share
SQUASH = 0.8                # nor shorter than this share (on a floor it spreads)
RING_STRETCH = (0.7, 1.5)   # a ring's edges within these shares of their rest
LEVEL = 0.35                # the hem's points drawn this share to their mean height
FLOOR_Y = 3.0               # the hem's least height on a floor (the mid ring's: + 8)
# Her legs' capsules: LEG_RADIUS of the leg mesh's radius, and the cloth
# kept MARGIN off them (owner p6t, "the natural pose ... more like a hoop
# dress": the look's A-line passes within a few units of her knees, and a
# margin pushed it out into a bulge there standing).
LEG_RADIUS = 0.85
MARGIN = 0.0
FEEDBACK = 0.3              # share of a constraint's move the spring keeps
ITER = 3                    # constraint passes a frame
# The fit:
MAX_TILT_DEG = 80.0         # row 25's band axis from its rest
MAX_TWIST_DEG = 20.0        # row 25's turn about the band
REST = 0.02                 # a fit's pull to the row's rest (per unit of FIT_SCALE)
TEMPORAL = 0.05             # a fit's pull to the frame before's
FIT_SCALE = np.array([60.0, 60.0, 60.0, 0.5, 0.5, 0.5, 0.3, 0.3])
FIT_LO = np.array([-400.0, -400.0, -400.0, -3.2, -3.2, -3.2, 0.75, 0.75])
FIT_HI = np.array([400.0, 400.0, 400.0, 3.2, 3.2, 3.2, 1.6, 1.6])
# The keys: each row's tracks put on shared knots, straight between them,
# within these raw units (turn 512 a radian: about half a degree;
# translation 4 a unit: 1.5 units; scale 4096: 0.012), so the clip compacts
# to a few keys a row (the figatree heap: p4_anim_compact.py).
KNOT_EPS = {"rot": 4, "trans": 6, "scale": 48}


def _window(n: int, i: int, half: int, loop) -> list:
    """A centred window's frames about i: wrapped through a loop's period
    (frames loop..n-1, its last frame its first), clamped otherwise."""
    if loop is not None and 0 <= loop < n - 1:
        period = n - 1 - loop
        out = []
        for d in range(-half, half + 1):
            k = i + d
            if k > n - 1:
                k = loop + (k - loop) % period
            elif k < loop:
                k = (n - 1) - (loop - k) % period if i >= loop else max(k, 0)
            out.append(k)
        return out
    return [min(max(i + d, 0), n - 1) for d in range(-half, half + 1)]


def _turn(axis, ang):
    """The rotation by ang (radians) about a unit axis."""
    k = np.array([[0, -axis[2], axis[1]], [axis[2], 0, -axis[0]], [-axis[1], axis[0], 0]])
    return np.eye(3) + np.sin(ang) * k + (1.0 - np.cos(ang)) * (k @ k)


def _kabsch(a: np.ndarray, b: np.ndarray, w: np.ndarray) -> np.ndarray:
    """The rotation taking points a (about the origin) nearest to b."""
    h = (a * w[:, None]).T @ b
    u, _s, vt = np.linalg.svd(h)
    d = np.sign(np.linalg.det(vt.T @ u.T))
    return vt.T @ np.diag([1.0, 1.0, d]) @ u.T


def _swing_twist(q: np.ndarray, axis: np.ndarray):
    """q as a swing (taking the axis where q does) after a twist about it:
    (swing, twist angle)."""
    to = q @ axis
    v = np.cross(axis, to)
    s, c = np.linalg.norm(v), float(np.dot(axis, to))
    swing = np.eye(3) if s < 1e-9 else _turn(v / s, np.arctan2(s, c))
    twist = swing.T @ q
    # twist turns about the axis: its angle from a vector across it.
    e = np.cross(axis, [1.0, 0.0, 0.0])
    if np.linalg.norm(e) < 1e-3:
        e = np.cross(axis, [0.0, 0.0, 1.0])
    e /= np.linalg.norm(e)
    f = twist @ e
    return swing, float(np.arctan2(np.dot(np.cross(e, f), axis), np.dot(e, f)))


def _local(x) -> np.ndarray:
    """A row's local matrix from fit parameters (translation, euler turn,
    scale across): syMatrixTraRotRpyRSca, scale z 1."""
    m = np.eye(4)
    m[:3, :3] = A.euler(x[3], x[4], x[5]) @ np.diag([x[6], x[7], 1.0])
    m[:3, 3] = x[:3]
    return m


def _fit(base: np.ndarray, pts: np.ndarray, targets: np.ndarray, x0: np.ndarray, rest: np.ndarray,
         prev: np.ndarray | None, iters: int = 12) -> np.ndarray:
    """A row's parameters (_local) putting its points (row-local) nearest
    the targets under the parent's matrix base: damped Gauss-Newton with a
    numeric Jacobian, clipped to FIT_LO..FIT_HI."""
    hp = np.c_[pts, np.ones(len(pts))]

    def resid(x):
        r = [((base @ _local(x) @ hp.T).T[:, :3] - targets).ravel()]
        r.append(np.sqrt(REST * len(pts)) * (x - rest) / FIT_SCALE * 10.0)
        if prev is not None:
            r.append(np.sqrt(TEMPORAL * len(pts)) * (x - prev) / FIT_SCALE * 10.0)
        return np.concatenate(r)

    x = np.clip(np.array(x0, dtype=float), FIT_LO, FIT_HI)
    lam = 1e-3
    r = resid(x)
    cost = float(r @ r)
    for _ in range(iters):
        jac = np.empty((len(r), len(x)))
        for k in range(len(x)):
            h = 1e-4 * FIT_SCALE[k]
            xp = x.copy()
            xp[k] += h
            jac[:, k] = (resid(xp) - r) / h
        g = jac.T @ r
        hmat = jac.T @ jac
        while True:
            step = np.linalg.solve(hmat + lam * np.diag(np.diag(hmat) + 1e-9), -g)
            xn = np.clip(x + step, FIT_LO, FIT_HI)
            rn = resid(xn)
            cn = float(rn @ rn)
            if cn < cost:
                x, r, cost = xn, rn, cn
                lam = max(lam * 0.3, 1e-7)
                break
            lam *= 10.0
            if lam > 1e6:
                return x
        if float(np.abs(step).max()) < 1e-5:
            break
    return x


def _closest_on_segments(p0, p1, q0, q1):
    """Closest points of segments p0-p1 and q0-q1: (s, t) along each."""
    d1, d2, r = p1 - p0, q1 - q0, p0 - q0
    a, e, f = float(d1 @ d1), float(d2 @ d2), float(d2 @ r)
    if a < 1e-9 and e < 1e-9:
        return 0.0, 0.0
    if a < 1e-9:
        return 0.0, min(max(f / e, 0.0), 1.0)
    c = float(d1 @ r)
    if e < 1e-9:
        return min(max(-c / a, 0.0), 1.0), 0.0
    b = float(d1 @ d2)
    den = a * e - b * b
    s = min(max((b * f - c * e) / den, 0.0), 1.0) if den > 1e-9 else 0.0
    t = (b * s + f) / e
    if t < 0.0:
        t, s = 0.0, min(max(-c / a, 0.0), 1.0)
    elif t > 1.0:
        t, s = 1.0, min(max((b - c) / a, 0.0), 1.0)
    return s, t


def _knots(st: A.Stream, loop) -> None:
    """The stream's tracks (all from frame 0, every frame) put on shared
    knots, straight between them, each within its KNOT_EPS (a loop's start
    and the last frame are knots)."""
    tracks = sorted(st.tracks)
    if not tracks:
        return
    vals = {k: np.array(st.tracks[k].values, dtype=float) for k in tracks}
    eps = {k: KNOT_EPS["rot"] if k < 3 else KNOT_EPS["trans"] if k < 7 else KNOT_EPS["scale"] for k in tracks}
    n = len(vals[tracks[0]])
    fixed = {0, n - 1} | ({loop} if loop is not None and 0 <= loop < n else set())
    knots = [0]
    a = 0
    while a < n - 1:
        b = a + 1
        while b + 1 < n and b not in fixed:
            c = b + 1
            ok = True
            for k in tracks:
                v = vals[k]
                f = np.arange(a + 1, c)
                line = v[a] + (v[c] - v[a]) * (f - a) / (c - a)
                if np.any(np.abs(v[a + 1:c] - line) > eps[k]):
                    ok = False
                    break
            if not ok:
                break
            b = c
        knots.append(b)
        a = b
    for k in tracks:
        v = vals[k]
        new = v.copy()
        for a, b in zip(knots, knots[1:]):
            f = np.arange(a, b + 1)
            new[a:b + 1] = v[a] + (v[b] - v[a]) * (f - a) / (b - a)
        st.tracks[k] = A.Track([int(round(x)) for x in new], 0)


def _exit_along(q: np.ndarray, o: np.ndarray, a, b, ra: float, rb: float, steps: int = 12) -> np.ndarray:
    """Per point (rows of q): how far along its unit direction (rows of o)
    it must move to leave the capsule a-b (radii ra at a, rb at b); 0 if
    outside."""
    ab = b - a
    l2 = max(float(ab @ ab), 1e-9)
    reach = 2.5 * max(ra, rb)
    ts = np.linspace(0.0, reach, steps + 1)
    pts = q[:, None, :] + o[:, None, :] * ts[None, :, None]
    t = np.clip(((pts - a) @ ab) / l2, 0.0, 1.0)
    c = a + t[..., None] * ab
    inside = np.linalg.norm(pts - c, axis=2) < ra + t * (rb - ra)
    first_out = np.where(inside.all(axis=1), steps, np.argmin(inside, axis=1))
    out = ts[np.minimum(first_out, steps)]
    return np.where(inside[:, 0], out, 0.0)


class Skirt:
    def __init__(self, fighter: C.Fighter, scene: dict):
        self.f = fighter
        verts = scene["vertices"]
        D.apply_scene_skeleton(fighter, scene)
        self.bind = D.bind_mats(fighter)
        self.inv_bind = [np.linalg.inv(m) for m in self.bind]
        bands, hem, _slit = D.rings(scene)
        if len(bands) != 3:
            raise SystemExit("the scene's skirt is not on rows 25/26/27 (look_skirt.py split_band)")
        uv = lambda i: float(verts[i]["uv"][0])
        ring25 = sorted(bands[1][0], key=uv)
        # The hem's samples at the ring's u (the slit's own two hem vertices
        # ride the fit).
        us25 = [uv(i) for i in ring25]
        hem = sorted((i for i in hem if any(abs(uv(i) - u) < 1e-3 for u in us25)), key=uv)
        mid_all = bands[2][0]
        hem_u = [uv(i) for i in hem]
        # The mid ring's samples on the band's sides (one per hem sample, at
        # its u; the slit's two edge vertices are carried by the fit).
        mid = []
        for u in hem_u:
            mid.append(min(mid_all, key=lambda i: (abs(uv(i) - u), -abs(verts[i]["model"][1]))))
        self.hem_u = hem_u
        b1 = self.bind[ROW_PELVIS]
        pin = np.linalg.inv(b1)
        to_pelvis = lambda i: (pin @ np.append(verts[i]["model"], 1.0))[:3]
        # Rest points in the pelvis's frame (a chain per sample; u 1 is u 0).
        self.top_p = np.array([to_pelvis(i) for i in ring25])
        self.mid_p = np.array([to_pelvis(i) for i in mid])
        self.hem_p = np.array([to_pelvis(i) for i in hem])
        if len(self.top_p) != len(self.hem_p):
            raise SystemExit("row 25's ring and the hem differ in samples")
        self.chains = len(hem) - 1 if abs(hem_u[-1] - 1.0) < 1e-3 and abs(hem_u[0]) < 1e-3 else len(hem)
        self.len1 = np.linalg.norm(self.mid_p - self.top_p, axis=1)[:self.chains]
        self.len2 = np.linalg.norm(self.hem_p - self.mid_p, axis=1)[:self.chains]
        ring_len = lambda p: np.linalg.norm(np.roll(p[:self.chains], -1, axis=0) - p[:self.chains], axis=1)
        self.edge_mid, self.edge_hem = ring_len(self.mid_p), ring_len(self.hem_p)
        # The rows' rest: row 25's in the pelvis's frame, 26's in 25's, 27's
        # in 26's (bind), and each ring's points in its own row's frame.
        self.rest25 = pin @ self.bind[ROW_UPPER]
        self.rest26 = np.linalg.inv(self.bind[ROW_UPPER]) @ self.bind[ROW_MID]
        self.rest27 = np.linalg.inv(self.bind[ROW_MID]) @ self.bind[ROW_HEM]
        loc = lambda row, ids: np.array([(self.inv_bind[row] @ np.append(verts[i]["model"], 1.0))[:3] for i in ids])
        self.mid_local = loc(ROW_MID, mid)
        self.hem_local = loc(ROW_HEM, hem)
        self.x26_rest = np.r_[self.rest26[:3, 3], 0.0, 0.0, 0.0, 1.0, 1.0]
        self.x27_rest = np.r_[self.rest27[:3, 3], 0.0, 0.0, 0.0, 1.0, 1.0]
        # Each chain's azimuth about the hem's centre (a guide's moves are
        # given by direction, ultimate.py).
        c = self.hem_p.mean(axis=0)
        self.az_hem = np.arctan2(self.hem_p[:, 0] - c[0], self.hem_p[:, 2] - c[2])
        c = self.mid_p.mean(axis=0)
        self.az_mid = np.arctan2(self.mid_p[:, 0] - c[0], self.mid_p[:, 2] - c[2])
        # The legs' capsules' radii (hip, knee, ankle) from the leg mesh's
        # rings about each leg's line (bind pose).
        self.radii = []
        for hip, knee, ankle in LEGS:
            pts = {r: np.array([v["model"] for v in verts if v.get("object") == D.LEGS and v["joint"] == r])
                   for r in (hip, knee, ankle)}
            ph, pk, pa = (self.bind[r][:3, 3] for r in (hip, knee, ankle))

            def radius(ring, a, b):
                if not len(ring):
                    return 30.0
                d = (b - a) / np.linalg.norm(b - a)
                rel = ring - a
                return float(np.linalg.norm(rel - np.outer(rel @ d, d), axis=1).mean())
            self.radii.append((radius(pts[hip], ph, pk), radius(pts[knee], ph, pk), radius(pts[ankle], pk, pa)))
        self.hair_row = any(v.get("object") == "DS_hair_panels" and v["joint"] == ROW_HAIR for v in verts)
        self.roles = scene.get("joint_roles") or []
        self.rest_axis = self.rest25[:3, :3] @ np.array([0.0, 0.0, -1.0])
        self.pelvis_bind_inv = np.linalg.inv(self.bind[ROW_PELVIS][:3, :3])

    # ------------------------------------------------------------ the guide

    def _guide_points(self, guide: dict, key: str, rest: np.ndarray, az: np.ndarray, n: int) -> list:
        """A guide's points per frame (the pelvis's frame): the rest points
        moved as the source's chains move in each direction about them."""
        gaz = np.asarray(guide["mid_azimuths" if key == "mid" else "azimuths"], dtype=float)
        rows = guide[key]
        out = []
        for i in range(n):
            d = np.asarray(rows[min(i, len(rows) - 1)], dtype=float)
            xs, cols = D._periodic(gaz, d[:, 0], d[:, 1], d[:, 2])
            move = np.stack([np.interp(az, xs, col) for col in cols], axis=1)
            out.append(rest + move)
        return out

    # ------------------------------------------------------------ the cloth

    def cloth(self, frames, loop, guide=None, trail: float = 0.0, floor: bool = True):
        """The cloth's mid and hem points per frame (model space)."""
        n = len(frames)
        k = self.chains
        pel = [frames[i][1][ROW_PELVIS] for i in range(n)]
        to_model = lambda m, p: (m @ np.c_[p, np.ones(len(p))].T).T[:, :3]
        g_mid = g_hem = None
        if guide is not None and guide.get("mid"):
            g_mid = self._guide_points(guide, "mid", self.mid_p, self.az_mid, n)
        if guide is not None and guide.get("hem"):
            g_hem = self._guide_points(guide, "hem", self.hem_p, self.az_hem, n)
        order = list(range(n))
        if loop is not None and 0 <= loop < n - 1:
            period = list(range(loop + 1, n))
            order = list(range(n)) + period + period
        down = np.array([0.0, -1.0, 0.0])
        unit = lambda v: v / np.maximum(np.linalg.norm(v, axis=1), 1e-6)[:, None]
        p1 = p2 = v1 = v2 = None
        out1, out2 = [None] * n, [None] * n
        for i in order:
            m = pel[i]
            mats = frames[i][1]
            anchor = to_model(m, self.top_p[:k])
            rest1 = to_model(m, self.mid_p[:k])
            rest2 = to_model(m, self.hem_p[:k])
            air = np.zeros(3)
            if trail:
                fwd = m[:3, 2] / max(np.linalg.norm(m[:3, 2]), 1e-6)
                air = -trail * fwd
            # Gravity acts on her tilt: standing, the dress is the look's
            # A-line (owner p6t: "the natural pose ... more like a hoop
            # dress"); as her hips turn from upright, each segment turns
            # HANG (HANG_HEM) of the way back toward hanging as it did.
            pd = m[:3, :3] @ self.pelvis_bind_inv @ down
            pd = pd / max(np.linalg.norm(pd), 1e-6)
            tilt_axis = np.cross(pd, down)
            tilt = float(np.arctan2(np.linalg.norm(tilt_axis), np.dot(pd, down)))
            ax = tilt_axis / max(np.linalg.norm(tilt_axis), 1e-9)
            back1 = _turn(ax, HANG * tilt) if tilt > 1e-4 else np.eye(3)
            back2 = _turn(ax, HANG_HEM * tilt) if tilt > 1e-4 else np.eye(3)
            d1 = unit(unit(rest1 - anchor) @ back1.T + air * HANG)
            t1 = anchor + d1 * self.len1[:, None]
            d2 = unit(unit(rest2 - rest1) @ back2.T + air * HANG_HEM)
            t2 = t1 + d2 * self.len2[:, None]
            if g_mid is not None:
                t1 = t1 + (to_model(m, g_mid[i][:k]) - t1) * GUIDE_W
            if g_hem is not None:
                t2 = t2 + (to_model(m, g_hem[i][:k]) - t2) * GUIDE_W
            if p1 is None:
                p1, p2 = t1.copy(), t2.copy()
                v1, v2 = np.zeros_like(p1), np.zeros_like(p2)
            else:
                for _ in range(SUB):
                    for p, v, t, kk in ((p1, v1, t1, SPRING_MID), (p2, v2, t2, SPRING_HEM)):
                        kk = kk / (SUB * SUB)
                        c = 2.0 * np.sqrt(kk) * DAMP
                        v += kk * (t - p) - c * v
                        p += v
            caps = []
            for (hip, knee, ankle), (rh, rk, ra) in zip(LEGS, self.radii):
                ph, pk, pa = mats[hip][:3, 3], mats[knee][:3, 3], mats[ankle][:3, 3]
                rh, rk, ra = rh * LEG_RADIUS + MARGIN, rk * LEG_RADIUS + MARGIN, ra * LEG_RADIUS + MARGIN
                caps.append((ph, pk, rh, rk))
                caps.append((pk, pa, rk, ra))
            q1, q2 = p1.copy(), p2.copy()
            centre = m[:3, 3]
            # Each chain's outward direction: from the pelvis's vertical axis
            # through its anchor, across (the pelvis's frame).
            o = self.top_p[:k].copy()
            o[:, 1] = 0.0
            o = o / np.maximum(np.linalg.norm(o, axis=1), 1e-6)[:, None]
            self._outward = o @ m[:3, :3].T
            for _ in range(ITER):
                self._constrain(anchor, q1, q2, caps, floor, centre)
            v1 += (q1 - p1) * FEEDBACK
            v2 += (q2 - p2) * FEEDBACK
            p1, p2 = q1, q2
            out1[i], out2[i] = p1.copy(), p2.copy()
        return out1, out2

    def _constrain(self, anchor, q1, q2, caps, floor, centre):
        """One pass of the cloth's constraints, in place."""
        k = self.chains
        # Lengths: the mid point from its anchor, the hem from the mid.
        for base, q, rest in ((anchor, q1, self.len1), (q1, q2, self.len2)):
            d = q - base
            ln = np.linalg.norm(d, axis=1)
            want = np.clip(ln, rest * SQUASH, rest * (1.0 + STRETCH))
            q[:] = base + d * (want / np.maximum(ln, 1e-6))[:, None]
        # The rings' edges: neither collapsed nor torn.
        for q, rest in ((q1, self.edge_mid), (q2, self.edge_hem)):
            d = np.roll(q, -1, axis=0) - q
            ln = np.linalg.norm(d, axis=1)
            want = np.clip(ln, rest * RING_STRETCH[0], rest * RING_STRETCH[1])
            corr = d * ((ln - want) / np.maximum(ln, 1e-6))[:, None] * 0.5
            q += corr
            q -= np.roll(corr, 1, axis=0)
        # Her legs push the cloth out from her middle (the dress around her
        # legs, never through them: a kick lifts the dress over the thigh):
        # each point, each chain's segments (their lower end moved) and each
        # ring's edges (both ends) out of the legs' capsules along their
        # chain's outward direction.
        out_dir = self._outward
        for a, b, ra, rb in caps:
            for q in (q1, q2):
                q += out_dir * _exit_along(q, out_dir, a, b, ra, rb)[:, None]
            for upper, lower in ((anchor, q1), (q1, q2)):
                for j in range(k):
                    s, t = _closest_on_segments(upper[j], lower[j], a, b)
                    pc = upper[j] + (lower[j] - upper[j]) * s
                    need = _exit_along(pc[None], out_dir[j][None], a, b, ra, rb)[0]
                    if need > 0.0:
                        lower[j] += out_dir[j] * need / max(s, 0.35)
            for q in (q1, q2):
                for j in range(k):
                    jn = (j + 1) % k
                    s, t = _closest_on_segments(q[j], q[jn], a, b)
                    pc = q[j] + (q[jn] - q[j]) * s
                    o = out_dir[j] + out_dir[jn]
                    o = o / max(np.linalg.norm(o), 1e-6)
                    need = _exit_along(pc[None], o[None], a, b, ra, rb)[0]
                    if need > 0.0:
                        w = need / ((1.0 - s) ** 2 + s ** 2)
                        q[j] += o * w * (1.0 - s)
                        q[jn] += o * w * s
        # The hem level, and on a floor: a point under it is lifted onto it
        # and spread out from her (the cloth lies on the ground).
        y = q2[:, 1]
        q2[:, 1] = y + LEVEL * (y.mean() - y)
        if floor:
            for q, base, rest, y0 in ((q1, anchor, self.len1, FLOOR_Y + 8.0), (q2, q1, self.len2, FLOOR_Y)):
                low = q[:, 1] < y0
                if not low.any():
                    continue
                q[low, 1] = y0
                dy = np.abs(base[:, 1] - y0)
                reach = np.sqrt(np.maximum((rest * 0.95) ** 2 - dy ** 2, 0.0))
                have = np.linalg.norm((q - base)[:, [0, 2]], axis=1)
                grow = np.where(low, np.maximum(reach - have, 0.0), 0.0)
                if grow.any():
                    flat = q - centre
                    flat[:, 1] = 0.0
                    flat = flat / np.maximum(np.linalg.norm(flat, axis=1), 1e-6)[:, None]
                    q += flat * grow[:, None]

    # ------------------------------------------------------------- the fit

    def fit(self, frames, mids, hems):
        """Per frame: row 25's local rotation, row 26's and row 27's fit
        parameters."""
        n = len(frames)
        k = self.chains
        full = lambda pts: np.vstack([pts, pts[:len(self.hem_p) - k]])   # u 1 is u 0
        origin = self.rest25[:3, 3]
        out = []
        x26 = self.x26_rest.copy()
        x27 = self.x27_rest.copy()
        prev26 = prev27 = None
        rest_down = self.rest25[:3, :3]
        a_rest = np.vstack([self.mid_p[:k], self.hem_p[:k]]) - origin
        w = np.r_[np.full(k, 1.0), np.full(k, 0.5)]
        for i in range(n):
            pel = frames[i][1][ROW_PELVIS]
            pin = np.linalg.inv(pel)
            to_p = lambda p: (pin @ np.c_[p, np.ones(len(p))].T).T[:, :3]
            m_p, h_p = to_p(mids[i]), to_p(hems[i])
            q = _kabsch(a_rest, np.vstack([m_p, h_p]) - origin, w)
            swing, twist = _swing_twist(q, self.rest_axis)
            lim = np.radians(MAX_TWIST_DEG)
            twist = min(max(twist, -lim), lim)
            # Tilt capped: the swing's angle at most MAX_TILT_DEG.
            to = swing @ self.rest_axis
            ang = float(np.arccos(np.clip(np.dot(to, self.rest_axis), -1.0, 1.0)))
            if ang > np.radians(MAX_TILT_DEG):
                v = np.cross(self.rest_axis, to)
                swing = _turn(v / max(np.linalg.norm(v), 1e-9), np.radians(MAX_TILT_DEG))
            rot = swing @ _turn(self.rest_axis, twist) @ rest_down
            m25 = np.eye(4)
            m25[:3, :3] = rot
            m25[:3, 3] = origin
            x26 = _fit(m25, self.mid_local, full(m_p), x26, self.x26_rest, prev26)
            m26 = m25 @ _local(x26)
            x27 = _fit(m26, self.hem_local, full(h_p), x27, self.x27_rest, prev27)
            prev26, prev27 = x26.copy(), x27.copy()
            out.append((rot, x26.copy(), x27.copy()))
        return out

    # ------------------------------------------------------------ the clip

    def _solve(self, clip: A.Clip, anim_flags: int, trail: float = 0.0, guide=None):
        length = max((st.length for st in clip.streams.values()), default=0)
        frames = self.f.pose_frames(clip, anim_flags, length)
        loops = {st.loop for st in clip.streams.values()}
        loop = loops.pop() if len(loops) == 1 else None
        if guide is not None and "floor" in guide:
            floor = bool(guide["floor"])
        else:
            # Her feet on a floor: an ankle at standing height most frames.
            low = [min(m[r][:3, 3][1] for r in (LEGS[0][2], LEGS[1][2])) for _root, m in frames]
            floor = float(np.mean(np.array(low) < 60.0)) > 0.5
        mids, hems = self.cloth(frames, loop, guide, trail, floor)
        fits = self.fit(frames, mids, hems)
        return {"frames": frames, "fits": fits, "loop": loop, "length": length, "floor": floor,
                "mids": mids, "hems": hems}

    def _slot_of(self, anim_flags):
        lead_slot = C.first_slot(anim_flags)
        rows = self.f.rows
        return (lambda row: lead_slot + rows.index(row)) if rows is not None else (lambda row: lead_slot + row)

    def apply(self, clip: A.Clip, anim_flags: int, trail: float = 0.0, guide=None) -> A.Clip:
        """The clip with rows 25, 26 and 27 keyed from the cloth (the air
        moving back past her by `trail` units a frame; a guide's chains:
        Ultimate's skirt)."""
        solved = self._solve(clip, anim_flags, trail, guide)
        fits, loop, length = solved["fits"], solved["loop"], solved["length"]
        slot_of = self._slot_of(anim_flags)
        # The joint array reaches the skirt's rows (a clip whose array ended
        # before them had none to point at).
        out = A.Clip(max(clip.slots, slot_of(ROW_HEM) + 1))
        out.streams = dict(clip.streams)
        # A row's role moved (look_skirt.py: the held joint, row 27, to row
        # 28): a clip that installs its hidden part keyed that DObj after the
        # rows the fighter makes (it hangs last from row 0), the hem's slot
        # now; its keys move one past the made rows.
        for role in self.roles:
            mask = 0
            for part in role.get("hidden_parts", ()):
                mask |= 1 << (31 - int(part))
            lead = C.first_slot(anim_flags)
            new_slot = lead + len(self.f.rows)
            if anim_flags & mask and new_slot not in clip.streams:
                # (A clip made on the new tree keys it there already.)
                st = clip.streams.get(lead + int(role["from"]))
                if st is not None:
                    out.streams[new_slot] = st
                    out.slots = max(out.slots, new_slot + 1)
        prev_e = tuple(self.f.skel.rotate[ROW_UPPER])
        eul25 = []
        for rot, _x26, _x27 in fits:
            prev_e = A.to_euler(rot, prev_e)
            eul25.append(prev_e)
        st25 = A.Stream(length, loop)
        for k in range(3):
            st25.tracks[k] = A.Track([round(e[k] * A.ROT_UNIT) for e in eul25], 0)
        _knots(st25, loop)
        out.streams[slot_of(ROW_UPPER)] = st25
        for row, idx in ((ROW_MID, 1), (ROW_HEM, 2)):
            st = A.Stream(length, loop)
            xs = [f[idx] for f in fits]
            # Euler angles unwrapped frame to frame.
            prev = np.zeros(3)
            angs = []
            for x in xs:
                e = A.to_euler(A.euler(*x[3:6]), tuple(prev))
                prev = np.array(e)
                angs.append(e)
            for k in range(3):
                st.tracks[k] = A.Track([round(a[k] * A.ROT_UNIT) for a in angs], 0)
                st.tracks[4 + k] = A.Track([round(x[k] * A.TRANS_UNIT) for x in xs], 0)
            for k in range(2):
                st.tracks[7 + k] = A.Track([round(x[6 + k] * C.SCALE_UNIT) for x in xs], 0)
            _knots(st, loop)
            out.streams[slot_of(row)] = st
        if self.hair_row and guide is None:
            out.streams[slot_of(ROW_HAIR)] = self._hair_on_bodice(solved["frames"], loop,
                                                                   clip.streams.get(slot_of(ROW_HAIR)))
        return out

    def _hair_on_bodice(self, frames, loop, old):
        """Row 9's keys holding the hair to the bodice (its bind relation),
        any translation keys the clip had kept."""
        rot = lambda m: np.linalg.svd(m[:3, :3])[0] @ np.linalg.svd(m[:3, :3])[2]
        rel = rot(self.bind[ROW_HAIR_FROM]).T @ rot(self.bind[ROW_HAIR])
        prev = tuple(self.f.skel.rotate[ROW_HAIR])
        eul = []
        for _root, mats in frames:
            local = rot(mats[ROW_HEAD]).T @ rot(mats[ROW_HAIR_FROM]) @ rel
            prev = A.to_euler(local, prev)
            eul.append(prev)
        st = A.Stream(len(frames) - 1, loop)
        for k in range(3):
            st.tracks[k] = A.Track([round(e[k] * A.ROT_UNIT) for e in eul], 0)
        if old is not None:
            for k, t in old.tracks.items():
                if k >= 3:
                    st.tracks[k] = t
        return st


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
