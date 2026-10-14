#!/usr/bin/env python3
"""P6: pose operations for authored clips (anim.py recipes "turn", "blend",
"keys").

A turn is given about the MODEL's axes as the bind pose holds them (x her
left, y up, z forward; her bind pose is a T-pose): turning row r by D makes
its local rotation inv(P) @ D @ P @ local, P the rest world rotation of its
parent -- D applied about the joint as if its parent stood at rest, so a
child's turn composes in its parent's frame the way a pose-mode turn does,
on top of whatever the clip already does ("turn") or of the bind pose
("keys").

- turn: {"row", "deg": [x, y, z], "frames": [first, last] (default all),
  "ease": frames to ramp in and out (default 0)} -- added to the clip;
- blend: {"rows": [...], "amount": 0..1, "toward": "rest" | {"frame": n} |
  {"clip": id, "frame": n}} -- each frame's rotation of each row slerped
  that far toward the bind pose, the clip's own frame n or another clip's
  (a smaller stride: the legs toward standing; the staff arm toward the
  wait's);
- keys: {"length", "loop" (a frame or null), "keys": [{"frame", "rows":
  {row: [x, y, z]}}]} -- a clip from key poses (turns from the bind pose),
  smoothstep slerp between keys; rows no key names stay at rest.
"""
from __future__ import annotations

import numpy as np

import anim as A


def axis_turn(deg) -> np.ndarray:
    """A turn of (x, y, z) degrees about model x, then y, then z."""
    rx, ry, rz = (np.radians(float(d)) for d in deg)
    cx, sx, cy, sy, cz, sz = np.cos(rx), np.sin(rx), np.cos(ry), np.sin(ry), np.cos(rz), np.sin(rz)
    mx = np.array([[1, 0, 0], [0, cx, -sx], [0, sx, cx]])
    my = np.array([[cy, 0, sy], [0, 1, 0], [-sy, 0, cy]])
    mz = np.array([[cz, -sz, 0], [sz, cz, 0], [0, 0, 1]])
    return mz @ my @ mx


def _quat(m):
    t = np.trace(m)
    if t > 0:
        s = np.sqrt(t + 1.0) * 2
        q = [0.25 * s, (m[2, 1] - m[1, 2]) / s, (m[0, 2] - m[2, 0]) / s, (m[1, 0] - m[0, 1]) / s]
    else:
        i = int(np.argmax([m[0, 0], m[1, 1], m[2, 2]]))
        if i == 0:
            s = np.sqrt(1.0 + m[0, 0] - m[1, 1] - m[2, 2]) * 2
            q = [(m[2, 1] - m[1, 2]) / s, 0.25 * s, (m[0, 1] + m[1, 0]) / s, (m[0, 2] + m[2, 0]) / s]
        elif i == 1:
            s = np.sqrt(1.0 + m[1, 1] - m[0, 0] - m[2, 2]) * 2
            q = [(m[0, 2] - m[2, 0]) / s, (m[0, 1] + m[1, 0]) / s, 0.25 * s, (m[1, 2] + m[2, 1]) / s]
        else:
            s = np.sqrt(1.0 + m[2, 2] - m[0, 0] - m[1, 1]) * 2
            q = [(m[1, 0] - m[0, 1]) / s, (m[0, 2] + m[2, 0]) / s, (m[1, 2] + m[2, 1]) / s, 0.25 * s]
    q = np.array(q)
    return q / np.linalg.norm(q)


def _mat(q):
    w, x, y, z = q / np.linalg.norm(q)
    return np.array([[1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
                     [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
                     [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)]])


def slerp(a, b, t):
    qa, qb = _quat(a), _quat(b)
    if np.dot(qa, qb) < 0:
        qb = -qb
    d = float(np.clip(np.dot(qa, qb), -1.0, 1.0))
    if d > 0.9995:
        q = qa + (qb - qa) * t
    else:
        th = np.arccos(d)
        q = (np.sin((1 - t) * th) * qa + np.sin(t * th) * qb) / np.sin(th)
    return _mat(q)


def _rest_worlds(skel: A.Skeleton):
    out = []
    for i, r in enumerate(skel.rotate):
        m = A.euler(*r)
        p = skel.parents[i]
        out.append(m if p < 0 else out[p] @ m)
    return out


def _length(clip: A.Clip) -> int:
    return max((st.length for st in clip.streams.values()), default=0)


def _rotations(clip: A.Clip, skel: A.Skeleton, row: int, first_slot: int, length: int):
    """Row's local rotation matrix on every frame of the clip."""
    out = []
    for f in range(length + 1):
        _t, rot = A.pose(clip, skel, f, first_slot)
        out.append(A.euler(*rot[row]))
    return out


def _write_row(clip: A.Clip, skel: A.Skeleton, row: int, first_slot: int, mats, length: int):
    """The row's stream replaced by these per-frame rotations (translations
    and scale tracks kept)."""
    slot = row + first_slot
    old = clip.streams.get(slot)
    loop = old.loop if old is not None else (
        next(iter({st.loop for st in clip.streams.values()}), None) if clip.streams else None)
    st = A.Stream(length, loop)
    if old is not None:
        for k, tr in old.tracks.items():
            if k > 2:
                st.tracks[k] = tr
    prev = tuple(skel.rotate[row])
    eulers = []
    for m in mats:
        prev = A.to_euler(m, prev)
        eulers.append(prev)
    for k in range(3):
        st.tracks[k] = A.Track([round(e[k] * A.ROT_UNIT) for e in eulers], 0)
    clip.streams[slot] = st
    if clip.slots <= slot:
        clip.slots = slot + 1


def turn(clip: A.Clip, skel: A.Skeleton, row: int, deg, frames=None, ease: int = 0,
         first_slot: int = 0) -> A.Clip:
    length = _length(clip)
    worlds = _rest_worlds(skel)
    p = skel.parents[row]
    P = worlds[p] if p >= 0 else np.eye(3)
    D = axis_turn(deg)
    first, last = (0, length) if frames is None else (int(frames[0]), int(frames[1]))
    mats = _rotations(clip, skel, row, first_slot, length)
    out = []
    for f, m in enumerate(mats):
        if f < first or f > last:
            out.append(m)
            continue
        w = 1.0
        if ease:
            w = min(1.0, (f - first + 1) / ease, (last - f + 1) / ease)
            w = w * w * (3 - 2 * w)
        d = slerp(np.eye(3), D, w)
        out.append(P.T @ d @ P @ m)
    _write_row(clip, skel, row, first_slot, out, length)
    return clip


def blend(clip: A.Clip, skel: A.Skeleton, rows, amount: float, toward="rest", first_slot: int = 0,
          toward_clip: A.Clip | None = None) -> A.Clip:
    """`toward`: "rest", {"frame": n} (the clip's own frame n) or, with
    `toward_clip`, {"clip": id, "frame": n} (another clip's frame n: Peach's
    wait holding the staff upright)."""
    length = _length(clip)
    for row in rows:
        mats = _rotations(clip, skel, row, first_slot, length)
        if toward == "rest":
            target = [A.euler(*skel.rotate[row])] * len(mats)
        elif toward_clip is not None:
            _t, rot = A.pose(toward_clip, skel, int(toward["frame"]), first_slot)
            target = [A.euler(*rot[row])] * len(mats)
        else:
            target = [mats[int(toward["frame"])]] * len(mats)
        _write_row(clip, skel, row, first_slot,
                   [slerp(m, t, float(amount)) for m, t in zip(mats, target)], length)
    return clip


def keys(skel: A.Skeleton, spec: dict, slots: int) -> A.Clip:
    length = int(spec["length"])
    loop = spec.get("loop")
    worlds = _rest_worlds(skel)
    clip = A.Clip(slots)
    ks = sorted(spec["keys"], key=lambda k: int(k["frame"]))
    rows = sorted({int(r) for k in ks for r in k["rows"]})
    for row in rows:
        p = skel.parents[row]
        P = worlds[p] if p >= 0 else np.eye(3)
        rest = A.euler(*skel.rotate[row])
        pts = [(int(k["frame"]), axis_turn(k["rows"].get(str(row), k["rows"].get(row, [0, 0, 0]))))
               for k in ks]
        mats = []
        for f in range(length + 1):
            if f <= pts[0][0]:
                d = pts[0][1]
            elif f >= pts[-1][0]:
                d = pts[-1][1]
            else:
                i = next(i for i in range(1, len(pts)) if pts[i][0] >= f)
                (fa, da), (fb, db) = pts[i - 1], pts[i]
                t = (f - fa) / max(fb - fa, 1)
                d = slerp(da, db, t * t * (3 - 2 * t))
            mats.append(P.T @ d @ P @ rest)
        st = A.Stream(length, loop)
        prev = tuple(skel.rotate[row])
        eulers = []
        for m in mats:
            prev = A.to_euler(m, prev)
            eulers.append(prev)
        for k in range(3):
            st.tracks[k] = A.Track([round(e[k] * A.ROT_UNIT) for e in eulers], 0)
        clip.streams[row] = st
    return clip
