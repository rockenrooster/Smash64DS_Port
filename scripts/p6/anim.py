#!/usr/bin/env python3
"""P6 A4: fighter animations as joint tracks, and back (docs/P6/P6_PLAN.md A4).

A FIGATREE clip is an array of joint slots, each pointing at an AObjEvent16
stream or at nothing: slot 0 is TransN, 1 XRotN, 2 YRotN, then the model's
JointTree rows in order (lbCommonAddFighterPartsFigatree walks the tree from
TopN's child; ftMainSetStatus resets every row to its bind pose first). A
stream keys up to ten tracks (p4_anim_compact.TRACKS: rotation x, y, z;
translate interpolation; translation x, y, z; scale x, y, z) in event16 raw
units.

decode() samples every stream at the game's rate, one value per frame from
frame 0 to the stream's End (or one period of its Loop), exactly as the
source parser computes them (p4_anim_compact._simulate_cmds). A track whose
first key interpolates from the previous clip's pose keeps that as `lead`:
the frames it blends over (its first key's payload); its values start there.

encode() writes each stream back as a step key at frame 0 (or the lead) and
one linear key a frame, then p4_anim_compact's joint-synchronised fit within
`eps` raw units (ANIM_EPS by default), so a written clip plays the decoded
values within the compactor's tolerance.

retarget() moves a clip onto another humanoid tree by joint roles (ROLES):
each mapped joint takes its source joint's world rotation relative to the
source's bind pose and any translation offset from its bind position.

    anim.py --check builds/p4-staging/extra-96621af builds/p4-staging/export-x
        F2: every clip of Roy, Marth and Peach decodes, re-encodes and
        decodes equal within ANIM_EPS (from two different previous poses)
"""
from __future__ import annotations

import argparse
import json
import math
import struct
import sys
from dataclasses import dataclass, field
from fractions import Fraction
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "p4"))

import p4_anim_compact as C  # noqa: E402

ANIM_EPS = 2                    # generate_p4_fighter.ANIM_EPS
PREVIOUS = (12345, -23456)      # two previous poses: a track that differs depends on it
SLOT_TRANSN, SLOT_XROTN, SLOT_YROTN, SLOT_ROWS = 0, 1, 2, 3


@dataclass
class Track:
    values: list                # raw units per frame (frames lead..length)
    lead: int = 0               # frames blended from the previous pose
    step: object = None         # with a lead: the frame-0 step it blends from
                                # on the first play (a loop that skips the step
                                # blends from the period's end instead)


@dataclass
class Stream:
    length: int                 # frames to the End or the Loop
    loop: int | None            # the frame the Loop returns to, or None
    tracks: dict = field(default_factory=dict)   # track -> Track
    loop_skips_steps: bool = False  # the Loop lands past that frame's step keys

    def value(self, track: int, frame: int):
        t = self.tracks[track]
        return t.values[frame - t.lead] if frame >= t.lead else None


@dataclass
class Clip:
    slots: int                  # joint array length
    streams: dict = field(default_factory=dict)  # slot -> Stream


def _time_of(cmds, at: int) -> int:
    t = 0
    for op, flags, payload, values, offset, nxt in cmds:
        if offset == at:
            return t
        if op in C.EV16_BLOCK_OPS:
            t += payload or 0
    raise C.CompactError(f"no command at {at:#x}")


def decode_stream(body: bytes, start: int) -> Stream:
    cmds, loop_target, _ = C.decode(body, start)
    index = {c[4]: i for i, c in enumerate(cmds)}
    length = C._t_end(cmds)
    loop = None if loop_target is None else _time_of(cmds, loop_target)
    rows = [C._simulate_cmds(cmds, index.get(loop_target), 2 * length + 1, previous=p) for p in PREVIOUS]
    stream = Stream(length, loop)
    before = []
    if loop is not None:
        # Commands at the loop's frame before its target: the restart skips
        # them (Remix loops past a clip's opening step keys).
        before = [c for c in cmds[:index[loop_target]] if _time_of(cmds, c[4]) == loop]
        if before:
            if any(c[0] not in (C.OP_SETVALAFTER, C.OP_SETVALAFTERBLOCK) or c[2] for c in before):
                raise C.CompactError(f"stream {start:#x}: loop skips more than step keys")
            stream.loop_skips_steps = True
    for track in range(C.TRACKS):
        if all(r[track] is None for r in rows[0][:length + 1]):
            continue
        lead = 0
        step = None
        stepped = [c for c in before if c[1] & (1 << track)]
        if stepped and loop == 0 and any(rows[0][f][track] != rows[0][length + f][track] for f in range(length + 1)):
            # The loop skips this track's step: its first segment blends from
            # the step on the first play and from the period's end after.
            c = stepped[0]
            step = c[3][bin(c[1] & ((1 << track) - 1)).count("1")]
            first = next(c for c in cmds[index[loop_target]:] if c[0] not in (C.OP_END, C.OP_LOOP, C.OP_BLOCK)
                         and c[1] & (1 << track))
            if first[0] not in (C.OP_SETVALBLOCK, C.OP_SETVAL) or not first[2] or _time_of(cmds, first[4]) != 0:
                raise C.CompactError(f"stream {start:#x} track {track}: looped lead of op {first[0]}")
            stream.tracks[track] = Track([rows[0][f][track] for f in range(first[2], length + 1)], first[2], step)
            continue
        if any(a[track] != b[track] for a, b in zip(rows[0][:length + 1], rows[1][:length + 1])):
            first = next(c for c in cmds if c[0] not in (C.OP_END, C.OP_LOOP, C.OP_BLOCK) and c[1] & (1 << track))
            if first[0] not in (C.OP_SETVALBLOCK, C.OP_SETVAL) or not first[2]:
                raise C.CompactError(f"stream {start:#x} track {track}: lead of op {first[0]}")
            lead = first[2]
            if any(a[track] != b[track] for a, b in zip(rows[0][lead:length + 1], rows[1][lead:length + 1])):
                raise C.CompactError(f"stream {start:#x} track {track}: depends past its lead")
        stream.tracks[track] = Track([rows[0][f][track] for f in range(lead, length + 1)], lead)
    return stream


def decode(blob: bytes) -> Clip:
    body, slots = C._o2r_body(blob)
    if body is None:
        raise C.CompactError("animation file has extern slots")
    clip = Clip(min(slots.values()) // 4 if slots else 0)
    for slot_off, target in slots.items():
        clip.streams[slot_off // 4] = decode_stream(body, target)
    return clip


def encode_stream(stream: Stream, eps: int = ANIM_EPS) -> list[int]:
    keys = {}
    for track, t in stream.tracks.items():
        vals = [round(v) for v in t.values]
        if any(not -0x8000 <= v <= 0x7FFF for v in vals):
            raise C.CompactError(f"track {track}: value out of range")
        if t.lead:
            ks = [(0, t.lead, C.LINEAR, vals[0])]
        else:
            ks = [(0, 0, C.STEP, vals[0])]
        base = t.lead
        ks += [(base + i, 1, C.LINEAR, vals[i + 1]) for i in range(len(vals) - 1)]
        keys[track] = ks
    if stream.loop and not stream.loop_skips_steps:
        # A loop back to a later frame restarts there with a step to that
        # frame's value (the fit keeps such tracks dense).
        for track, ks in keys.items():
            t = stream.tracks[track]
            if stream.loop >= t.lead:
                at = next(i for i, k in enumerate(ks) if k[0] == stream.loop and k[1] == 1)
                ks.insert(at, (stream.loop, 0, C.STEP, round(t.values[stream.loop - t.lead])))
    fitted = C._fit_joint(keys, eps) if eps > 0 and stream.length > 0 else None
    chosen = fitted if fitted is not None else keys
    for track, t in stream.tracks.items():
        if t.step is not None:
            chosen[track] = [(0, 0, C.STEP, round(t.step))] + list(chosen[track])
    # The frame-0 step keys go first, in one command, so a loop that skips
    # them lands right after it.
    steps = {tr: ks[0][3] for tr, ks in chosen.items() if ks[0][:3] == (0, 0, C.STEP)}
    rest = {tr: (ks[1:] if tr in steps else ks) for tr, ks in chosen.items()}
    rest = {tr: ks for tr, ks in rest.items() if ks}
    head = []
    if steps:
        head = [(C.OP_SETVALAFTER << 11) | (sum(1 << tr for tr in steps) << 1)]
        head += [steps[tr] & 0xFFFF for tr in sorted(steps)]
    tail = [C.OP_END << 11] if stream.loop is None else [C.OP_LOOP << 11, 0]
    if rest:
        if min(k[0] for ks in rest.values() for k in ks) != 0:
            raise C.CompactError("keys start after frame 0")
        words = head + C._encode(rest, stream.length, tail)
    else:
        words = head + ([(C.OP_BLOCK << 11) | 1, stream.length] if stream.length else []) + tail
    if stream.loop is None:
        return words
    target = len(head) if stream.loop_skips_steps else 0
    if stream.loop:
        data = b"".join(struct.pack(">H", w) for w in words)
        cmds, _, _ = C.decode(data, 0)
        t = 0
        for op, flags, payload, values, offset, nxt in cmds:
            if t == stream.loop and op not in (C.OP_BLOCK,):
                target = offset // 2
                break
            if op in C.EV16_BLOCK_OPS:
                t += payload or 0
        else:
            raise C.CompactError("loop frame not found")
    words[-1] = (2 * target - 2 * (len(words) - 1)) & 0xFFFF
    return words


def encode(clip: Clip, header_blob: bytes, eps: int = ANIM_EPS) -> bytes:
    """An O2R container for the clip (header and file id from header_blob)."""
    body = bytearray(4 * clip.slots)
    slots = {}
    for slot in sorted(clip.streams):
        words = encode_stream(clip.streams[slot], eps)
        while len(body) % 4:
            body += b"\0"
        slots[4 * slot] = len(body)
        body += b"".join(struct.pack(">H", w) for w in words)
    while len(body) % 16:
        body += b"\0"
    return C.o2r_with_body(header_blob, bytes(body), slots)


# ---------------------------------------------------------------------------
# Poses and retargeting. A joint's local transform is translate, rotate
# (Rz * Ry * Rx, column vectors: syMatrixRotRpyR) and scale; tracks 0-2 are
# its rotation in 1/512 rad, 4-6 its translation in 1/4 unit.

ROT_UNIT, TRANS_UNIT = 512.0, 4.0


def euler(rx: float, ry: float, rz: float):
    import numpy as np
    cx, sx, cy, sy, cz, sz = (np.cos(rx), np.sin(rx), np.cos(ry), np.sin(ry), np.cos(rz), np.sin(rz))
    return np.array([[cz * cy, cz * sy * sx - sz * cx, cz * sy * cx + sz * sx],
                     [sz * cy, sz * sy * sx + cz * cx, sz * sy * cx - cz * sx],
                     [-sy, cy * sx, cy * cx]])


def to_euler(m, near=(0.0, 0.0, 0.0)) -> tuple:
    """Angles of a rotation (euler's order), of its two solutions the one
    nearer `near`, each unwrapped to within pi of it."""
    import math
    sy = max(-1.0, min(1.0, -m[2][0]))
    ry = math.asin(sy)
    if abs(math.cos(ry)) < 1e-6:
        candidates = [(0.0, ry, math.atan2(-m[0][1], m[1][1]))]
    else:
        rx = math.atan2(m[2][1], m[2][2])
        rz = math.atan2(m[1][0], m[0][0])
        candidates = [(rx, ry, rz), (rx + math.pi, math.pi - ry, rz + math.pi)]

    def unwrap(a, b):
        return a + 2 * math.pi * round((b - a) / (2 * math.pi))
    best = None
    for c in candidates:
        c = tuple(unwrap(a, b) for a, b in zip(c, near))
        d = sum((a - b) ** 2 for a, b in zip(c, near))
        if best is None or d < best[0]:
            best = (d, c)
    return best[1]


@dataclass
class Skeleton:
    parents: list
    translate: list
    rotate: list

    @classmethod
    def from_model(cls, fm, detail: str = "high") -> "Skeleton":
        joints = fm.trees[detail][:-1]
        return cls(list(fm.parents(detail)), [tuple(j.translate) for j in joints],
                   [tuple(j.rotate) for j in joints])

    def world(self, rotations):
        import numpy as np
        out = []
        for i, r in enumerate(rotations):
            m = euler(*r)
            out.append(m if self.parents[i] < 0 else out[self.parents[i]] @ m)
        return out

    def positions(self, rotations, translations=None):
        """World joint positions of a pose (model units)."""
        import numpy as np
        rots, pos = [], []
        for i, r in enumerate(rotations):
            t = np.array(translations[i] if translations else self.translate[i])
            m = euler(*r)
            p = self.parents[i]
            if p < 0:
                rots.append(m)
                pos.append(t)
            else:
                rots.append(rots[p] @ m)
                pos.append(pos[p] + rots[p] @ t)
        return pos


def pose(clip: Clip, skel: Skeleton, frame: int, first_slot: int = 0):
    """Each row's (translate, rotate) at a frame: animated tracks over the
    bind pose (a track before its lead holds its first value; a stream past
    its end holds its last)."""
    rot = [list(r) for r in skel.rotate]
    trans = [list(t) for t in skel.translate]
    for slot, st in clip.streams.items():
        row = slot - first_slot
        if not 0 <= row < len(rot):
            continue
        f = min(frame, st.length)
        for track, t in st.tracks.items():
            v = t.values[max(f - t.lead, 0)]
            if track <= 2:
                rot[row][track] = float(v) / ROT_UNIT
            elif 4 <= track <= 6:
                trans[row][track - 4] = float(v) / TRANS_UNIT
    return trans, rot


def retarget(clip: Clip, src: Skeleton, dst: Skeleton, roles: dict, src_first_slot: int = 0,
             dst_slots: int | None = None, height_scale: float = 1.0, item=None) -> Clip:
    """The clip on another skeleton. `roles` maps source rows to target
    rows; a mapped target joint takes its source joint's world rotation
    relative to the source's bind pose, applied to its own bind pose (so
    differently oriented joints still move alike); unmapped target joints
    keep their bind pose. A mapped joint whose source joint's translation is
    animated moves by that joint's offset from its bind position (carried
    through the parents' bind frames), times height_scale.

    `item` (target row, source axis, target axis; each axis in its row's
    local frame) turns a held item: the target's item axis follows the
    source's (Palutena's staff where Marth's sword points) by a constant
    local turn of the target row, which then always plays."""
    import numpy as np
    length = max((st.length for st in clip.streams.values()), default=0)
    lead = max((t.lead for st in clip.streams.values() for t in st.tracks.values()), default=0)
    loops = {st.loop for st in clip.streams.values()}
    loop = loops.pop() if len(loops) == 1 else None
    src_rest = src.world(src.rotate)
    dst_rest = dst.world(dst.rotate)
    inverse = {d: s for s, d in roles.items()}
    animated_src = {slot - src_first_slot for slot, st in clip.streams.items()
                    if any(tr <= 2 for tr in st.tracks)}
    moving = {d: s for d, s in inverse.items()
              if any(4 <= tr <= 6 for tr in clip.streams.get(s + src_first_slot, Stream(0, None)).tracks)}
    rows_out = sorted(d for d, s in inverse.items() if s in animated_src or
                      any(a in animated_src for a in _ancestors(src, s)))
    turn = {}
    if item is not None:
        row, src_axis, dst_axis = item
        if row not in inverse:
            raise ValueError(f"item row {row} has no source joint")
        s_row = inverse[row]
        want = dst_rest[row].T @ src_rest[s_row] @ np.asarray(src_axis, dtype=float)
        turn[row] = _turn_between(np.asarray(dst_axis, dtype=float), want)
        if row not in rows_out:
            rows_out = sorted(rows_out + [row])
    series = {d: [] for d in rows_out}
    moves = {d: [] for d in moving}
    prev = {d: tuple(dst.rotate[d]) for d in rows_out}
    for f in range(length + 1):
        s_trans, s_rot = pose(clip, src, f, src_first_slot)
        s_world = src.world([tuple(r) for r in s_rot])
        d_world = []
        for i in range(len(dst.parents)):
            p = dst.parents[i]
            if i in inverse:
                s = inverse[i]
                w = s_world[s] @ src_rest[s].T @ dst_rest[i]
                if i in turn:
                    w = w @ turn[i]
            else:
                w = (d_world[p] if p >= 0 else np.eye(3)) @ euler(*dst.rotate[i])
            d_world.append(w)
        for d in rows_out:
            p = dst.parents[d]
            local = (d_world[p].T if p >= 0 else np.eye(3)) @ d_world[d]
            prev[d] = to_euler(local, prev[d])
            series[d].append(prev[d])
        for d, s in moving.items():
            off = np.array(s_trans[s]) - np.array(src.translate[s])
            ps, pd = src.parents[s], dst.parents[d]
            if ps >= 0:
                off = src_rest[ps] @ off
            if pd >= 0:
                off = dst_rest[pd].T @ off
            moves[d].append(tuple(np.array(dst.translate[d]) + off * height_scale))
    out = Clip(dst_slots if dst_slots is not None else len(dst.parents) - 1)
    for d in rows_out:
        st = Stream(length, loop)
        for k in range(3):
            vals = [round(series[d][f][k] * ROT_UNIT) for f in range(lead, length + 1)]
            rest = round(dst.rotate[d][k] * ROT_UNIT)
            if d not in turn and all(abs(v - rest) <= ANIM_EPS for v in vals):
                continue
            st.tracks[k] = Track(vals, lead)
        if st.tracks:
            out.streams[d] = st
    for d, ts in moves.items():
        st = out.streams.setdefault(d, Stream(length, loop))
        for k in range(3):
            vals = [round(ts[f][k] * TRANS_UNIT) for f in range(lead, length + 1)]
            if all(abs(v - round(dst.translate[d][k] * TRANS_UNIT)) <= ANIM_EPS for v in vals) and d != 0:
                continue
            st.tracks[4 + k] = Track(vals, lead)
    return out


def world_frames(clip: Clip, skel: Skeleton, first_slot: int = 0, frames=None) -> list:
    """Each frame's joint world matrices (4x4 rows, model space): the
    playback a review renders (A6)."""
    import numpy as np
    length = max((st.length for st in clip.streams.values()), default=0)
    out = []
    for f in (frames if frames is not None else range(length + 1)):
        trans, rot = pose(clip, skel, f, first_slot)
        mats = []
        for i, p in enumerate(skel.parents):
            m = np.eye(4)
            m[:3, :3] = euler(*rot[i])
            m[:3, 3] = trans[i]
            mats.append(m if p < 0 else mats[p] @ m)
        out.append([m.tolist() for m in mats])
    return out


def retime(clip: Clip, factor: float) -> Clip:
    """The clip played `factor` times as long (0.5: twice as fast): every
    track resampled at the new frames by linear interpolation of its
    per-frame values; leads and loop frames scale with it."""
    out = Clip(clip.slots)
    for slot, st in clip.streams.items():
        length = max(1, round(st.length * factor)) if st.length else 0
        loop = None if st.loop is None else round(st.loop * factor)
        new = Stream(length, loop, loop_skips_steps=st.loop_skips_steps)
        for track, t in st.tracks.items():
            lead = round(t.lead * factor)
            vals = []
            for g in range(lead, length + 1):
                x = min(g / factor if factor else 0.0, st.length) - t.lead
                x = max(0.0, min(x, len(t.values) - 1))
                i = int(x)
                frac = x - i
                a = float(t.values[i])
                b = float(t.values[min(i + 1, len(t.values) - 1)])
                vals.append(a + (b - a) * frac)
            new.tracks[track] = Track(vals, lead, t.step)
        out.streams[slot] = new
    return out


def spin(clip: Clip, slot: int, rest: tuple, axis, degrees: float, first: int, last: int) -> Clip:
    """The clip with one row turned about `axis` (in its parent's frame) by
    `degrees` over frames first..last, eased in and out, the whole turn kept
    after: a flip (Aether's apex: the root row, 360 about x, she faces +z).
    The row's three rotation tracks are rewritten one key a frame (a track
    the clip leaves alone from the bind pose `rest`, radians); its other
    tracks stay, held at their last value where the turn runs longer."""
    import numpy as np
    st = clip.streams.get(slot)
    length = max(last, st.length if st is not None else 0)

    def angle(track: int, f: int) -> float:
        if st is None or track not in st.tracks:
            return float(rest[track])
        t = st.tracks[track]
        f = min(f, st.length)
        return float(t.values[max(f - t.lead, 0)]) / ROT_UNIT

    a = np.asarray(axis, dtype=float)
    a = a / np.linalg.norm(a)
    k = np.array([[0.0, -a[2], a[1]], [a[2], 0.0, -a[0]], [-a[1], a[0], 0.0]])
    new = Stream(length, None)
    if st is not None:
        for track, t in st.tracks.items():
            if track > 2:
                vals = list(t.values) + [t.values[-1]] * max(0, length + 1 - t.lead - len(t.values))
                new.tracks[track] = Track(vals, t.lead, t.step)
    rot = {0: [], 1: [], 2: []}
    near = (angle(0, 0), angle(1, 0), angle(2, 0))
    for f in range(length + 1):
        u = 0.0 if f <= first else 1.0 if f >= last else (f - first) / float(last - first)
        u = u * u * (3.0 - 2.0 * u)
        th = math.radians(degrees) * u
        turn = np.eye(3) + math.sin(th) * k + (1.0 - math.cos(th)) * (k @ k)
        near = to_euler(turn @ euler(angle(0, f), angle(1, f), angle(2, f)), near)
        for i in range(3):
            rot[i].append(int(round(near[i] * ROT_UNIT)))
    for i in range(3):
        new.tracks[i] = Track(rot[i], 0, None)
    out = Clip(clip.slots)
    out.streams = dict(clip.streams)
    out.streams[slot] = new
    return out


def splice(clip: Clip, first: int, last: int, hold: int = 0, strip_transn: bool = False) -> Clip:
    """Frames first..last of the clip as a clip of its own, then the last
    frame held `hold` frames more (a pose: Quick Draw's stance from a
    frame of a swing). strip_transn drops a TransN clip's leading travel
    slot (the status that plays the splice moves the fighter itself), so
    the rows start at slot 0. A track before its lead takes its first
    value; a stream past its end holds its last."""
    if not 0 <= first <= last:
        raise ValueError(f"splice frames {first}..{last}")
    out = Clip(clip.slots - 1 if strip_transn else clip.slots)
    length = last - first + hold
    for slot, st in clip.streams.items():
        if strip_transn:
            if slot == 0:
                continue
            slot -= 1
        new = Stream(length, None)
        for track, t in st.tracks.items():
            vals = []
            for g in range(length + 1):
                f = min(first + g, last, st.length)
                vals.append(t.values[max(f - t.lead, 0)])
            new.tracks[track] = Track(vals, 0, None)
        out.streams[slot] = new
    return out


def _turn_between(a, b):
    """The smallest rotation (3x3) taking direction a to direction b."""
    import numpy as np
    a = a / np.linalg.norm(a)
    b = b / np.linalg.norm(b)
    v = np.cross(a, b)
    s = np.linalg.norm(v)
    c = float(np.dot(a, b))
    if s < 1e-9:
        return np.eye(3) if c > 0 else -np.eye(3)
    k = np.array([[0, -v[2], v[1]], [v[2], 0, -v[0]], [-v[1], v[0], 0]])
    return np.eye(3) + k + k @ k * ((1 - c) / (s * s))


def _ancestors(skel: Skeleton, row: int) -> list:
    out = []
    while skel.parents[row] >= 0:
        row = skel.parents[row]
        out.append(row)
    return out


# Joint roles between the two humanoid trees the P6 bases use. Rows the
# target has and the source does not (Roy's cape, shoulder plates, the
# second hand item) keep their bind pose.
ROLES = {
    ("captain", "roy"): {0: 0, 1: 1, 2: 2, 3: 3, 4: 4, 5: 5, 6: 6, 7: 8, 8: 9, 9: 10, 10: 11,
                         11: 12, 12: 13, 13: 14, 15: 21, 16: 22, 17: 23, 18: 24, 19: 25,
                         20: 26, 21: 27, 22: 28, 23: 29, 24: 30},
    # Mario's tree: the arms 3-6 and 9-12 (13 his held item), the neck 7 and
    # head 8, the legs 14-18 and 19-23 (14 and 19 the hip frames).
    ("mario", "roy"): {0: 0, 1: 1, 2: 2, 3: 3, 4: 4, 5: 5, 6: 6, 7: 8, 8: 9, 9: 10, 10: 11, 11: 12,
                       12: 13, 13: 14, 14: 21, 15: 22, 16: 23, 17: 24, 18: 25, 19: 26, 20: 27,
                       21: 28, 22: 29, 23: 30},
    # Link's tree: the left arm 3-7 (7 the shield), the right arm 8-11 with
    # his sword on 15, the neck 18 and head 19, the legs 21-25 and 26-30.
    ("link", "roy"): {0: 0, 1: 1, 2: 2, 3: 3, 4: 4, 5: 5, 6: 6, 7: 7, 8: 10, 9: 11, 10: 12, 11: 13,
                      15: 14, 18: 8, 19: 9, 21: 21, 22: 22, 23: 23, 24: 24, 25: 25, 26: 26, 27: 27,
                      28: 28, 29: 29, 30: 30},
    # Peach's tree (Palutena's): the left arm 3-6, the neck 7 and head 8, the
    # right arm 10-13 with the held point 14 (her staff), the legs 15-19 and
    # 20-24, the dress 25/26 (never driven: the look does not draw it).
    ("mario", "peach"): {0: 0, 1: 1, 2: 2, 3: 3, 4: 4, 5: 5, 6: 6, 7: 7, 8: 8, 9: 10, 10: 11, 11: 12,
                         12: 13, 13: 14, 14: 15, 15: 16, 16: 17, 17: 18, 18: 19, 19: 20, 20: 21, 21: 22,
                         22: 23, 23: 24},
    # Ness's: Mario's layout with a second hip frame a side (19, 25: unmapped).
    ("ness", "peach"): {0: 0, 1: 1, 2: 2, 3: 3, 4: 4, 5: 5, 6: 6, 7: 7, 8: 8, 9: 10, 10: 11, 11: 12,
                        12: 13, 13: 14, 14: 15, 15: 16, 16: 17, 17: 18, 18: 19, 20: 20, 21: 21, 22: 22,
                        23: 23, 24: 24},
    ("captain", "peach"): {0: 0, 1: 1, 2: 2, 3: 3, 4: 4, 5: 5, 6: 6, 7: 7, 8: 8, 9: 10, 10: 11, 11: 12,
                           12: 13, 13: 14, 15: 15, 16: 16, 17: 17, 18: 18, 19: 19, 20: 20, 21: 21,
                           22: 22, 23: 23, 24: 24},
    ("link", "peach"): {0: 0, 1: 1, 2: 2, 3: 3, 4: 4, 5: 5, 6: 6, 8: 10, 9: 11, 10: 12, 11: 13, 15: 14,
                        18: 7, 19: 8, 21: 15, 22: 16, 23: 17, 24: 18, 25: 19, 26: 20, 27: 21, 28: 22,
                        29: 23, 30: 24},
    # Marth's: the left arm 3-6 (7 its held point), neck 8 and head 9, the
    # right arm 10-13 with the sword 14, the cape 15-18 and pads 19/20
    # (unmapped), the legs 21-25 and 26-30.
    ("marth", "peach"): {0: 0, 1: 1, 2: 2, 3: 3, 4: 4, 5: 5, 6: 6, 8: 7, 9: 8, 10: 10, 11: 11, 12: 12,
                         13: 13, 14: 14, 21: 15, 22: 16, 23: 17, 24: 18, 25: 19, 26: 20, 27: 21, 28: 22,
                         29: 23, 30: 24},
    # Peach's own clips with a held item turned (her pan's axis to the staff's).
    ("peach", "peach"): {r: r for r in range(27)},
}


# Skeletons by name: (main file, model file, FTAttributes offset) in the
# staged reference ROM (resolved.json descriptors; Captain is Roy's setup
# parent, so Roy's parent_descriptor names him).
SKELETONS = {"captain": (236, 332, 1160), "roy": (0x142A, 0x142B, 1236),
             "mario": (0xCB, 0x128, 1064), "link": (0xE1, 0x144, 0x708),
             "ness": (0xEF, 0x14F, 0x5BC), "marth": (0xCCA, 0xCCB, 0x524),
             "peach": (0x136E, 0x136F, 0x59C)}


def skeleton(rom, name: str, detail: str = "high") -> Skeleton:
    import n64_model as N
    main, model, attr = SKELETONS[name]
    return Skeleton.from_model(N.FighterModel(N.RelocFile.read(rom.o2r(main)),
                                              N.RelocFile.read(rom.o2r(model)), attr), detail)


def with_transn_slot(clip: Clip) -> Clip:
    """The clip with an empty TransN slot before its rows, for a motion row
    with the TransN flag (the Appear statuses read the TransN joint, which
    only such a row makes): the joint stays at rest, the fighter at its
    entry position."""
    out = Clip(clip.slots + 1)
    for slot, st in clip.streams.items():
        out.streams[slot + 1] = st
    return out


def build(recipe: dict, rom) -> bytes:
    """A clip from its recipe (art/p6/<name>/anims/*.json): a source clip
    (file id in the reference ROM), optionally retargeted ("retarget":
    {"from", "to", "height_scale", "first_slot": a TransN clip's 1, "item":
    {"row", "from_axis", "to_axis"}}), spliced ("frames": [first, last],
    "hold": frames the last is held, "strip_transn": drop a TransN clip's
    travel slot), retimed ("retime": the length factor), turned ("spin": a
    list of {"row", "axis", "degrees", "frames": [first, last]}, spin(); a
    clip not retargeted names its tree in "skeleton") and given an empty
    TransN slot ("transn_slot": true, with_transn_slot); posed
    ("ops": a list of pose_ops.py operations, in order, on the clip's own
    tree -- "blend" rows toward the bind pose or a frame of another clip,
    "turn" a row about the model's axes; or, with no "source", "keys": a
    clip from key poses); the O2R container keeps the source's header
    (p6_author.py gives it its own id; a keyed clip takes "header": a
    clip of the same tree's id for its header)."""
    import pose_ops
    if "keys" in recipe:
        blob = rom.o2r(int(str(recipe["header"]), 0))
        skel = skeleton(rom, recipe["skeleton"])
        clip = pose_ops.keys(skel, recipe["keys"], len(skel.parents) - 1)
    else:
        blob = rom.o2r(int(str(recipe["source"]), 0))
        clip = decode(blob)
    rt = recipe.get("retarget")
    if rt:
        src, dst = skeleton(rom, rt["from"]), skeleton(rom, rt["to"])
        item = rt.get("item")
        clip = retarget(clip, src, dst, ROLES[(rt["from"], rt["to"])],
                        src_first_slot=int(rt.get("first_slot", 0)),
                        dst_slots=len(dst.parents) - 1, height_scale=float(rt.get("height_scale", 1.0)),
                        item=(int(item["row"]), item["from_axis"], item["to_axis"]) if item else None)
    if "frames" in recipe:
        first, last = (int(x) for x in recipe["frames"])
        clip = splice(clip, first, last, int(recipe.get("hold", 0)), bool(recipe.get("strip_transn")))
    if "retime" in recipe:
        clip = retime(clip, float(recipe["retime"]))
    for sp in recipe.get("spin", ()):
        # The bind pose of the clip's own tree (the retarget's target, else
        # the recipe's "skeleton").
        name = rt["to"] if rt else recipe["skeleton"]
        row = int(sp["row"])
        rest = skeleton(rom, name).rotate[row]
        clip = spin(clip, row + int(sp.get("first_slot", 0)), rest, sp["axis"], float(sp["degrees"]),
                    int(sp["frames"][0]), int(sp["frames"][1]))
    if recipe.get("ops"):
        tree = skeleton(rom, rt["to"] if rt else recipe["skeleton"])
        for op in recipe["ops"]:
            kind = op["op"]
            if kind == "blend":
                toward = op.get("toward", "rest")
                other = (decode(rom.o2r(int(str(toward["clip"]), 0)))
                         if isinstance(toward, dict) and "clip" in toward else None)
                clip = pose_ops.blend(clip, tree, [int(r) for r in op["rows"]], float(op["amount"]),
                                      toward, int(op.get("first_slot", 0)), toward_clip=other)
            elif kind == "turn":
                clip = pose_ops.turn(clip, tree, int(op["row"]), op["deg"], op.get("frames"),
                                     int(op.get("ease", 0)), int(op.get("first_slot", 0)))
            else:
                raise ValueError(f"recipe op {kind!r}")
    if recipe.get("transn_slot"):
        clip = with_transn_slot(clip)
    return encode(clip, blob, int(recipe.get("eps", ANIM_EPS)))


def compare(old: bytes, new: bytes, eps: int = ANIM_EPS) -> list[str]:
    """Differences beyond eps (plus the half unit rounding adds) between two
    clips, simulated over two periods from both previous poses."""
    ob, os_ = C._o2r_body(old)
    nb, ns = C._o2r_body(new)
    bound = Fraction(2 * eps + 1, 2)
    errors = []
    if set(os_) != set(ns):
        return [f"slots differ: {sorted(os_)} vs {sorted(ns)}"]
    for slot, target in sorted(os_.items()):
        oc, ol, _ = C.decode(ob, target)
        nc, nl, _ = C.decode(nb, ns[slot])
        frames = 2 * C._t_end(oc) + 3
        oi = {c[4]: i for i, c in enumerate(oc)}
        ni = {c[4]: i for i, c in enumerate(nc)}
        for prev in PREVIOUS:
            a = C._simulate_cmds(oc, oi.get(ol), frames, previous=prev)
            b = C._simulate_cmds(nc, ni.get(nl), frames, previous=prev)
            for f, (ra, rb) in enumerate(zip(a, b)):
                for track, (va, vb) in enumerate(zip(ra, rb)):
                    if (va is None) != (vb is None) or (va is not None and abs(va - vb) > bound):
                        errors.append(f"slot {slot // 4} frame {f} track {track}: {va} vs {vb}")
                        break
                else:
                    continue
                break
    return errors


def check(staging: Path, export_root: Path, fighters=("roy", "marth", "peach")) -> int:
    import remix_rom as R
    rom = R.Rom(staging / "ssb64asm.z64")
    clips = streams = before = after = 0
    bad = []
    for name in fighters:
        res = json.loads((export_root / name / "resolved.json").read_text(encoding="utf-8"))
        ids = sorted({m["anim_file_id"] for m in res["motions"] + res["menu_motions"] if m["anim_file_id"]})
        for fid in ids:
            blob = rom.o2r(fid)
            new = blob
            try:
                clip = decode(blob)
                streams += len(clip.streams)
                new = encode(clip, blob)
                errors = compare(blob, new)
            except C.CompactError as e:
                errors = [str(e)]
            clips += 1
            before += len(blob)
            after += len(new) if not errors else len(blob)
            if errors:
                bad.append((name, fid, errors[:2]))
    for name, fid, errs in bad[:20]:
        print(f"  {name} {fid:#x}: {errs}")
    print(f"F2: {clips} clips, {streams} streams, {len(bad)} differ; "
          f"bytes {before} -> {after} (re-encoded)")
    return 1 if bad else 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--check", nargs=2, type=Path, metavar=("STAGING", "EXPORT_ROOT"))
    ap.add_argument("--build", type=Path, metavar="RECIPE", help="build a clip from its recipe")
    ap.add_argument("--staging", type=Path, help="the P4 staging directory (ssb64asm.z64)")
    ap.add_argument("--out", type=Path, help="the built clip (O2R container, under builds/)")
    args = ap.parse_args()
    if args.check:
        sys.path.insert(0, str(HERE.parent))
        return check(*args.check)
    if args.build:
        sys.path.insert(0, str(HERE.parent))
        import remix_rom as R
        recipe = json.loads(args.build.read_text(encoding="utf-8"))
        blob = build(recipe, R.Rom(args.staging / "ssb64asm.z64"))
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_bytes(blob)
        print(f"anim: {args.build.name} -> {args.out} ({len(blob)} bytes)")
        return 0
    ap.print_help()
    return 0


if __name__ == "__main__":
    sys.exit(main())
