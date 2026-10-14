#!/usr/bin/env python3
"""P6: a clip from key poses over base clip frames (anim.py recipe
"keyposes"; the owner's 3DS references, 2026-10-14: the P6 fighters' own
moves are authored as key poses against them and reviewed as sheets).

    {"keyposes": {"skeleton": "roy", "header": "0x...", "length": 51, "first_slot": 0,
     "base": {"clip": "0x...", "frame": 0},
     "keys": [{"frame": 0},
              {"frame": 6, "from": {"clip": ..., "frame": n},
               "trans": {"1": [dx, dy, dz]},
               "turns": {"11": [x, y, z]},
               "aims": [{"row": 11, "to": [x, y, z]}, {"row": 14, "axis": [0, 0, 1], "to": [0, 0, 1]}],
               "ik": [{"chain": [22, 23, 24], "target": "keep", "pole": [0, 0, 1]}],
               "place": [{"row": 14, "at": [x, y, z]}],
               "ease": "smooth" | "linear" | "in" | "out"}, ...]}}

A key starts from a base pose (its "from", else the recipe's "base": a
clip's frame). In order: "trans" offsets rows' translations (model units:
the hips' height for a crouch or a hop); "turns" turn rows about the model's
axes as the base holds their parents (a row turns with everything below
it); "aims" turn a row so its first child (or "child") -- or a local "axis"
of its own -- points along a model direction; "ik" bends a two-bone chain so
its end reaches a target ("keep": where the base pose had it, plus an
optional "offset") with the middle joint toward "pole"; "place" puts a row's
origin at a model position (a held item let go: Ike's thrown Ragnell), its
rotation kept. Between keys every
row's rotation slerps and its translation lerps, eased into the later key.
"""
from __future__ import annotations

import numpy as np

import anim as A
import pose_ops as P


def _ease(t: float, kind: str) -> float:
    if kind == "linear":
        return t
    if kind == "in":
        return t * t
    if kind == "out":
        return 1.0 - (1.0 - t) * (1.0 - t)
    return t * t * (3.0 - 2.0 * t)


def _clip_id(ref) -> int:
    return int(str(ref["clip"]), 0)


def _worlds(skel: A.Skeleton, local, trans):
    rots, pos = [], []
    for i, m in enumerate(local):
        p = skel.parents[i]
        t = np.array(trans[i], dtype=float)
        if p < 0:
            rots.append(m)
            pos.append(t)
        else:
            rots.append(rots[p] @ m)
            pos.append(pos[p] + rots[p] @ t)
    return rots, pos


def _first_child(skel: A.Skeleton, row: int) -> int:
    return next(i for i, p in enumerate(skel.parents) if p == row)


def _turn_between(a, b):
    a = np.asarray(a, dtype=float); b = np.asarray(b, dtype=float)
    a = a / np.linalg.norm(a); b = b / np.linalg.norm(b)
    v = np.cross(a, b)
    s = np.linalg.norm(v)
    c = float(np.dot(a, b))
    if s < 1e-9:
        if c > 0:
            return np.eye(3)
        # Half turn about any axis perpendicular to a.
        axis = np.cross(a, [1.0, 0.0, 0.0])
        if np.linalg.norm(axis) < 1e-6:
            axis = np.cross(a, [0.0, 1.0, 0.0])
        axis /= np.linalg.norm(axis)
        return 2.0 * np.outer(axis, axis) - np.eye(3)
    k = np.array([[0, -v[2], v[1]], [v[2], 0, -v[0]], [-v[1], v[0], 0]])
    return np.eye(3) + k + k @ k * ((1 - c) / (s * s))


def _aim(skel, local, trans, row, to, child=None, axis=None, twist=None):
    rots, pos = _worlds(skel, local, trans)
    if axis is not None:
        d = rots[row] @ np.asarray(axis, dtype=float)
    else:
        c = _first_child(skel, row) if child is None else int(child)
        d = pos[c] - pos[row]
    D = _turn_between(d, to)
    w = D @ rots[row]
    if twist is not None:
        # Turn about the aimed direction so the row's local twist["axis"]
        # lies toward twist["to"] (a blade's flat face to the camera).
        u = np.asarray(to, dtype=float); u = u / np.linalg.norm(u)
        a = w @ np.asarray(twist["axis"], dtype=float)
        b = np.asarray(twist["to"], dtype=float)
        a = a - np.dot(a, u) * u; b = b - np.dot(b, u) * u
        if np.linalg.norm(a) > 1e-6 and np.linalg.norm(b) > 1e-6:
            w = _turn_between(a, b) @ w
    p = skel.parents[row]
    pw = rots[p] if p >= 0 else np.eye(3)
    local[row] = pw.T @ w


def _ik(skel, local, trans, chain, target, pole):
    a, b, c = chain
    rots, pos = _worlds(skel, local, trans)
    l1 = np.linalg.norm(pos[b] - pos[a])
    l2 = np.linalg.norm(pos[c] - pos[b])
    to = np.asarray(target, dtype=float) - pos[a]
    d = float(np.clip(np.linalg.norm(to), 1e-3, (l1 + l2) * 0.999))
    u = to / max(np.linalg.norm(to), 1e-6)
    pv = np.asarray(pole, dtype=float)
    v = pv - np.dot(pv, u) * u
    v = v / max(np.linalg.norm(v), 1e-6)
    cos_a = float(np.clip((l1 * l1 + d * d - l2 * l2) / (2 * l1 * d), -1.0, 1.0))
    knee = pos[a] + l1 * (cos_a * u + np.sqrt(max(0.0, 1 - cos_a * cos_a)) * v)
    _aim(skel, local, trans, a, knee - pos[a], child=b)
    _aim(skel, local, trans, b, (pos[a] + u * d) - knee, child=c)


def key_pose(skel: A.Skeleton, clips: dict, key: dict, base: dict, first_slot: int):
    ref = key.get("from", base)
    trans, rot = A.pose(clips[_clip_id(ref)], skel, int(ref.get("frame", 0)), first_slot)
    trans = [list(map(float, t)) for t in trans]
    local = [A.euler(*r) for r in rot]
    base_rots, base_pos = _worlds(skel, local, trans)
    for r, d in (key.get("trans") or {}).items():
        r = int(r)
        trans[r] = [trans[r][k] + float(d[k]) for k in range(3)]
    out = list(local)
    for r, deg in (key.get("turns") or {}).items():
        r = int(r)
        p = skel.parents[r]
        pw = base_rots[p] if p >= 0 else np.eye(3)
        out[r] = pw.T @ P.axis_turn(deg) @ pw @ local[r]
    for aim in key.get("aims") or ():
        _aim(skel, out, trans, int(aim["row"]), aim["to"], aim.get("child"), aim.get("axis"), aim.get("twist"))
    for ik in key.get("ik") or ():
        chain = [int(x) for x in ik["chain"]]
        tgt = ik["target"]
        if tgt == "keep" or isinstance(tgt, dict):
            off = np.asarray((tgt.get("offset", [0, 0, 0]) if isinstance(tgt, dict) else [0, 0, 0]), dtype=float)
            tgt = base_pos[chain[2]] + off
        _ik(skel, out, trans, chain, tgt, ik.get("pole", [0, 0, 1]))
    for pl in key.get("place") or ():
        r = int(pl["row"])
        p = skel.parents[r]
        rots, pos = _worlds(skel, out, trans)
        local = rots[p].T @ (np.asarray(pl["at"], dtype=float) - pos[p])
        trans[r] = [float(v) for v in local]
    return trans, out


def build(skel: A.Skeleton, spec: dict, clips: dict, slots: int) -> A.Clip:
    first_slot = int(spec.get("first_slot", 0))
    length = int(spec["length"])
    keys = sorted(spec["keys"], key=lambda k: int(k["frame"]))
    poses = [(int(k["frame"]), *key_pose(skel, clips, k, spec["base"], first_slot), k.get("ease", "smooth"))
             for k in keys]
    rows = len(skel.parents)
    per_frame_rot = [None] * (length + 1)
    per_frame_tr = [None] * (length + 1)
    for f in range(length + 1):
        if f <= poses[0][0]:
            tr, rt = poses[0][1], poses[0][2]
        elif f >= poses[-1][0]:
            tr, rt = poses[-1][1], poses[-1][2]
        else:
            i = next(i for i in range(1, len(poses)) if poses[i][0] >= f)
            fa, ta, ra, _ea = poses[i - 1]
            fb, tb, rb, eb = poses[i]
            t = _ease((f - fa) / max(fb - fa, 1), eb)
            rt = [P.slerp(ra[r], rb[r], t) for r in range(rows)]
            tr = [[ta[r][k] + (tb[r][k] - ta[r][k]) * t for k in range(3)] for r in range(rows)]
        per_frame_rot[f], per_frame_tr[f] = rt, tr
    clip = A.Clip(slots)
    loop = spec.get("loop")
    for r in range(rows):
        slot = r + first_slot
        if slot >= slots:
            continue
        rest = A.euler(*skel.rotate[r])
        moved = any(np.abs(per_frame_rot[f][r] - rest).max() > 1e-4 for f in range(length + 1))
        tmoved = any(max(abs(per_frame_tr[f][r][k] - skel.translate[r][k]) for k in range(3)) > 1e-3
                     for f in range(length + 1))
        if not (moved or tmoved):
            continue
        st = A.Stream(length, loop)
        prev = tuple(skel.rotate[r])
        eul = []
        for f in range(length + 1):
            prev = A.to_euler(per_frame_rot[f][r], prev)
            eul.append(prev)
        for k in range(3):
            st.tracks[k] = A.Track([round(e[k] * A.ROT_UNIT) for e in eul], 0)
        if tmoved:
            for k in range(3):
                st.tracks[4 + k] = A.Track([round(per_frame_tr[f][r][k] * A.TRANS_UNIT) for f in range(length + 1)], 0)
        clip.streams[slot] = st
    return clip
