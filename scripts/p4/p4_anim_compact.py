#!/usr/bin/env python3
"""Re-encode a donor fighter animation (FIGATREE, AObjEvent16) without the
keys its own interpolation already implies.

Remix exports its fighters' animations as dense keys: every joint's tracks get
a new linear target every one to three frames (SetValBlock), and constant
channels a new step key (SetValAfterBlock) as often. A key whose value is the
continuation of the line before it, or a step key that steps to the value the
track already holds, changes nothing the source parser computes
(ftAnimParseDObjFigatree, gcGetAObjValue): dropping it and lengthening the
previous segment gives every frame the same value. That is half the keys of a
typical Remix animation, and the figatree heap a fighter reserves for its
largest animation shrinks with it (S15).

The rules, per track, from ftanim.c/objanim.c:

* a linear key k (SetVal/SetValBlock) sets base := the previous target,
  target := v_k, rate := (v_k - base) / p_k, and the value is
  base + rate * (frames since the key); it keeps extrapolating until the
  track's next key;
* a step key (SetValAfter/SetValAfterBlock) holds base until p_k frames have
  passed, then target.

Two consecutive linear keys k, k+1 (k >= 1: key 0's base is the previous
animation's pose) merge into one key at k's time with payload p_k + p_k+1 and
target v_k+1 when key k+1 starts exactly where k ends and their slopes are
equal as rationals. A step key whose value equals the previous key's, read
after that key has stepped, is dropped. Everything else is kept: a stream with
any other command (cubic keys, rates, flags, translate interpolation, a loop
that does not return to the stream's start, a payload of 0, two keys of one
track at one time) is copied unchanged.

The re-encoded stream issues, at each key time, one SetVal or SetValAfter per
(kind, payload) group of tracks, then advances time with a Block (or the last
group's Block form when its payload is exactly the advance), and ends with the
original End or Loop at the original time. compact_file() checks every stream
it changes by simulating both encodings in exact rationals over two passes of
the animation; any difference raises."""
from __future__ import annotations

import struct
from fractions import Fraction

OP_END, OP_BLOCK, OP_SETVALBLOCK, OP_SETVAL = 0, 1, 2, 3
OP_SETVALAFTERBLOCK, OP_SETVALAFTER = 9, 10
OP_LOOP = 13
LINEAR, STEP = "linear", "step"
TRACKS = 10


class CompactError(Exception):
    pass


def _u16(d: bytes, o: int) -> int:
    return struct.unpack_from(">H", d, o)[0]


def _s16(d: bytes, o: int) -> int:
    return struct.unpack_from(">h", d, o)[0]


def decode(d: bytes, start: int):
    """Commands from `start` to End/Loop: (op, flags, payload, values, at,
    next). Returns (cmds, loop_target_or_None, simple): simple is False when the
    stream uses anything compact_stream does not re-encode."""
    o = start
    cmds = []
    simple = True
    for _ in range(1 << 20):
        w = _u16(d, o)
        op, flags, toggle = w >> 11, (w >> 1) & 0x3FF, w & 1
        at = o
        o += 2
        if op == OP_END:
            cmds.append((op, 0, None, (), at, o))
            return cmds, None, simple
        if op == OP_LOOP:
            rel = _s16(d, o)
            o += 2
            cmds.append((op, 0, None, (), at, o))
            # ftanim.c: advance past the opcode, then add s/2 halfwords.
            return cmds, at + 2 + rel, simple
        payload = None
        if op in (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 14) and toggle:
            payload = _u16(d, o)
            o += 2
        n = bin(flags).count("1")
        values = ()
        if op in (2, 3, 6, 7, 8, 9, 10):
            values = tuple(_s16(d, o + 2 * i) for i in range(n))
            o += 2 * n
        elif op in (4, 5):
            values = tuple(_s16(d, o + 2 * i) for i in range(2 * n))
            o += 4 * n
        elif op == 12:
            o += 2
        elif op not in (1, 11, 14):
            raise CompactError(f"unknown event16 op {op} at {at:#x}")
        if op not in (OP_BLOCK, OP_SETVALBLOCK, OP_SETVAL, OP_SETVALAFTERBLOCK, OP_SETVALAFTER):
            simple = False
        if op == OP_SETVALBLOCK and not payload:
            simple = False
        cmds.append((op, flags, payload, values, at, o))
    raise CompactError("stream does not end")


def _keys(cmds):
    """Per-track keys [(time, payload, kind, value)] and the End/Loop time."""
    t = 0
    keys: dict[int, list] = {}
    for op, flags, payload, values, _, _ in cmds:
        if op in (OP_END, OP_LOOP):
            return keys, t
        if op == OP_BLOCK:
            t += payload or 0
            continue
        kind = LINEAR if op in (OP_SETVALBLOCK, OP_SETVAL) else STEP
        vi = 0
        for track in range(TRACKS):
            if flags & (1 << track):
                keys.setdefault(track, []).append((t, payload or 0, kind, values[vi]))
                vi += 1
        if op in (OP_SETVALBLOCK, OP_SETVALAFTERBLOCK):
            t += payload or 0
    raise CompactError("no End")


def _reduce(track_keys):
    out = [track_keys[0]]
    for key in track_keys[1:]:
        t, p, kind, v = key
        prev = out[-1]
        if kind == STEP and prev[2] == STEP and v == prev[3] and t >= prev[0] + prev[1]:
            continue
        if (kind == LINEAR and prev[2] == LINEAR and len(out) >= 2 and
                t == prev[0] + prev[1]):
            base = out[-2][3]
            # prev runs base -> prev.v over prev.p; key runs prev.v -> v over p.
            if (prev[3] - base) * p == (v - prev[3]) * prev[1]:
                out[-1] = (prev[0], prev[1] + p, LINEAR, v)
                continue
        out.append(key)
    return out


def _reduce_synced(keys):
    """Drop a key time for every track at once: each track's keys at that time
    must be droppable by _reduce's rules against its kept neighbours, so the
    tracks keep sharing their commands. Returns per-track key lists."""
    times = sorted({k[0] for ks in keys.values() for k in ks})
    by_track = {tr: {k[0]: [] for k in ks} for tr, ks in keys.items()}
    for tr, ks in keys.items():
        for k in ks:
            by_track[tr][k[0]].append(k)
    kept = {tr: [] for tr in keys}
    for t in times:
        trial = {}
        ok = True
        for tr in keys:
            here = by_track[tr].get(t)
            if not here:
                continue
            if len(here) != 1:
                ok = False
                break
            out = list(kept[tr])
            if not out:
                ok = False
                break
            merged = _reduce(out + here)
            if len(merged) != len(out):
                ok = False
                break
            trial[tr] = merged
        if ok and trial:
            for tr, merged in trial.items():
                kept[tr] = merged
        else:
            for tr in keys:
                kept[tr].extend(by_track[tr].get(t, []))
    return kept


def _encode(keys, t_end, tail):
    """Words for per-track keys, then `tail` (the End or Loop words). A track
    may have several keys at one time (Remix starts every stream with a
    payload-less step key and a linear key at frame 0): they are emitted in
    their order, one layer per key, and only the last group of a time may be
    the Block form."""
    by_time: dict[int, list] = {}
    for track, ks in keys.items():
        layer_at: dict[int, int] = {}
        for t, p, kind, v in ks:
            layer = layer_at.get(t, 0)
            layer_at[t] = layer + 1
            layers = by_time.setdefault(t, [])
            while len(layers) <= layer:
                layers.append({})
            layers[layer].setdefault((kind, p), {})[track] = v
    times = sorted(by_time)
    words: list[int] = []
    for i, t in enumerate(times):
        advance = (times[i + 1] if i + 1 < len(times) else t_end) - t
        layers = by_time[t]
        for li, groups_map in enumerate(layers):
            groups = sorted(groups_map.items(),
                            key=lambda g: (g[0][1] == advance, g[0][0], g[0][1]))
            for gi, ((kind, p), tracks) in enumerate(groups):
                block = ((li == len(layers) - 1) and (gi == len(groups) - 1) and
                         (p == advance) and advance > 0)
                if kind == LINEAR:
                    op = OP_SETVALBLOCK if block else OP_SETVAL
                else:
                    op = OP_SETVALAFTERBLOCK if block else OP_SETVALAFTER
                flags = 0
                for track in tracks:
                    flags |= 1 << track
                # A step key without a payload steps at once (length_invert 0).
                words.append((op << 11) | (flags << 1) | (1 if p else 0))
                if p:
                    words.append(p)
                for track in sorted(tracks):
                    words.append(tracks[track] & 0xFFFF)
                if block:
                    advance = 0
        if advance > 0:
            words.append((OP_BLOCK << 11) | 1)
            words.append(advance)
    return words + tail


def compact_stream(d: bytes, start: int, eps: int = 0):
    """New halfwords for the stream at `start` and whether they approximate
    (the joint-synchronised fit, _fit_joint, tried when eps > 0), or None to
    keep the stream."""
    cmds, loop_target, simple = decode(d, start)
    if not simple:
        return None
    last = cmds[-1]
    if last[0] == OP_LOOP and loop_target != start:
        return None
    keys, t_end = _keys(cmds)
    # A linear key without a payload keeps the track's old rate; it is only
    # safe where another key of the track follows in the same frame, so the
    # stale rate is never evaluated (the first key of a converted event32
    # stream, then its first segment).
    for ks in keys.values():
        for i, k in enumerate(ks):
            if k[2] == LINEAR and k[1] == 0 and (i + 1 >= len(ks) or ks[i + 1][0] != k[0]):
                return None
    reduced = {track: _reduce(ks) for track, ks in keys.items()}
    synced = _reduce_synced(keys)
    if last[0] == OP_LOOP:
        # Loop back to the start: the offset is filled in by the caller once
        # the stream's length is known (relative to the word after the opcode).
        tail = [OP_LOOP << 11, None]
    else:
        tail = [OP_END << 11]
    for ks in reduced.values():
        if any(k[1] > 0xFFFF for k in ks):
            return None
    words = None
    lossy = False
    candidates = [(reduced, False), (synced, False)]
    if eps > 0:
        fitted = _fit_joint(reduced, eps)
        if fitted is not None:
            candidates.append((fitted, True))
        candidates.append((_fit_tracks(reduced, eps), True))
    for candidate, is_lossy in candidates:
        w = _encode(candidate, t_end, list(tail))
        if words is None or len(w) < len(words):
            words, lossy = w, is_lossy
    if last[0] == OP_LOOP:
        rel = -2 * (len(words) - 1)  # bytes from the halfword after the opcode back to the start
        words[-1] = rel & 0xFFFF
    old_len = (last[5] - start) // 2
    return (words, lossy) if len(words) < old_len else None


def _fit_tracks(keys, eps):
    """_fit_joint for each track on its own: more keys dropped, fewer shared
    commands."""
    out = {}
    for track, ks in keys.items():
        fitted = _fit_joint({track: ks}, eps)
        out[track] = list(ks) if fitted is None else fitted[track]
    return out


def _fit_joint(keys, eps):
    """Joint-synchronised linear keys within `eps` raw units of every frame
    of the exact curve (rotation 1/512 rad, translation 1/4 unit a unit).
    Keeps each track's leading payload-less step keys, its first segment when
    no such key gives the curve's start, and its last two knots (the final
    segment extrapolates to the animation's end exactly as before). Returns
    per-track keys, or None when a track mixes linear keys with later step
    keys or its linear keys are not one contiguous run from frame 0."""
    lead: dict[int, list] = {}
    curves: dict[int, list] = {}
    other: dict[int, list] = {}
    for track, ks in keys.items():
        i = 0
        while i < len(ks) and ks[i][0] == 0 and ks[i][1] == 0:
            i += 1
        lead[track] = ks[:i]
        rest = ks[i:]
        if not any(k[2] == LINEAR for k in rest):
            other[track] = rest
            continue
        if any(k[2] != LINEAR for k in rest) or rest[0][0] != 0:
            return None
        t = 0
        knots = []
        for k in rest:
            if k[0] != t or k[1] <= 0:
                return None
            t += k[1]
            knots.append((t, k[3]))
        curves[track] = knots
    if not curves:
        return None
    forced = set()
    start = 0
    starts = {}
    for track, knots in curves.items():
        if not lead[track]:
            # The first segment's base is the previous animation's pose.
            return None
        starts[track] = Fraction(lead[track][-1][3])
        if len(knots) >= 2:
            forced.add(knots[-2][0])
        forced.add(knots[-1][0])
    candidates = sorted({t for knots in curves.values() for t, _ in knots})
    end_time = max(knots[-1][0] for knots in curves.values())
    if any(knots[-1][0] != end_time for knots in curves.values()):
        return None

    # Each track's exact value at every frame up to its last knot.
    frames: dict[int, list] = {}
    for track, knots in curves.items():
        vals = []
        prev_t, prev_v = 0, starts[track]
        for t, v in knots:
            for f in range(prev_t, t):
                vals.append(Fraction(prev_v) + Fraction(v - prev_v) * (f - prev_t) / (t - prev_t))
            prev_t, prev_v = t, Fraction(v)
        vals.append(Fraction(knots[-1][1]))
        frames[track] = vals

    def fits(a, b):
        for vals in frames.values():
            qa, qb = round(vals[a]), round(vals[b])
            for f in range(a, b + 1):
                if abs(vals[f] - (qa + Fraction(qb - qa) * (f - a) / (b - a))) > eps:
                    return False
        return True

    kept = [start]
    while kept[-1] < end_time:
        a = kept[-1]
        best = None
        for b in candidates:
            if b <= a:
                continue
            if not fits(a, b):
                break
            best = b
            if b in forced:
                break
        if best is None:
            return None
        kept.append(best)
    out = {}
    for track in keys:
        if track not in curves:
            out[track] = list(lead[track]) + list(other.get(track, []))
            continue
        vals = frames[track]
        out[track] = list(lead[track]) + [(a, b - a, LINEAR, round(vals[b]))
                                          for a, b in zip(kept, kept[1:])]
    return out


def _simulate(d: bytes, start: int, frames: int):
    """Exact per-frame track values of the source parser at speed 1."""
    cmds, loop_target, _ = decode(d, start)
    index = {c[4]: i for i, c in enumerate(cmds)}
    return _simulate_cmds(cmds, index.get(loop_target), frames)


CUBIC = "cubic"
OP_SETVALRATEBLOCK, OP_SETVALRATE, OP_SETTARGETRATE = 4, 5, 6
OP_SETVAL0RATEBLOCK, OP_SETVAL0RATE, OP_1611, OP_SETFLAGS = 7, 8, 11, 14


def _rate_ratio(track: int) -> Fraction:
    """An event16 rate unit in value units per frame (ftAnimGetTargetValue's
    rate fraction over its value fraction)."""
    if track <= 3:
        return Fraction(1)        # rotation, translation interpolation
    if track <= 6:
        return Fraction(1, 8)     # translation: 1/32 over 1/4
    return Fraction(1, 2)         # scale: 1/8192 over 1/4096


def _simulate_cmds(cmds, loop_index, frames: int):
    """Per-frame track values, in event16 value units, of the source parser
    at speed 1 (ftAnimParseDObjFigatree + gcPlayDObjAnimJoint), exactly.
    Rates in the commands are raw event16 rate units."""
    # base, target, kind, length offset (frame the length was 0),
    # length_invert, linear rate, rate_base, rate_target
    state = {track: [Fraction(12345), Fraction(12345), None, 0, 0, Fraction(0),
                     Fraction(0), Fraction(0)] for track in range(TRACKS)}
    pc = 0
    wait = 0
    out = []
    ended = None
    for f in range(frames):
        if f > 0:
            wait -= 1
        spins = 0
        while wait <= 0:
            spins += 1
            if spins > 4 * len(cmds):
                raise CompactError("stream loops without advancing time")
            op, flags, payload, values, at, nxt = cmds[pc]
            p = payload or 0
            if op == OP_END:
                # gcPlayDObjAnimJoint stops advancing the tracks' length.
                wait = 1 << 30
                ended = f
                break
            if op == OP_LOOP:
                pc = loop_index
                continue
            if op in (OP_BLOCK, OP_SETFLAGS):
                wait += p
                pc += 1
                continue
            vi = 0
            for track in range(TRACKS):
                if not flags & (1 << track):
                    continue
                s = state[track]
                if op == OP_1611:
                    s[3] -= p
                    continue
                if op == OP_SETTARGETRATE:
                    s[7] = Fraction(values[vi]) * _rate_ratio(track)
                    vi += 1
                    continue
                s[0] = s[1]
                s[1] = Fraction(values[vi])
                vi += 1
                s[3] = f
                if op in (OP_SETVALBLOCK, OP_SETVAL):
                    s[2] = LINEAR
                    if p:
                        s[5] = (s[1] - s[0]) / p
                    s[7] = Fraction(0)
                elif op in (OP_SETVALAFTERBLOCK, OP_SETVALAFTER):
                    s[2] = STEP
                    s[4] = Fraction(p)
                    s[7] = Fraction(0)
                else:
                    s[2] = CUBIC
                    s[6] = s[7]
                    if op in (OP_SETVALRATEBLOCK, OP_SETVALRATE):
                        s[7] = Fraction(values[vi]) * _rate_ratio(track)
                        vi += 1
                    else:
                        s[7] = Fraction(0)
                    if p:
                        s[4] = Fraction(1, p)
            if op in (OP_SETVALBLOCK, OP_SETVALAFTERBLOCK, OP_SETVALRATEBLOCK,
                      OP_SETVAL0RATEBLOCK):
                wait += p
            pc += 1
        at_f = f if ended is None else ended
        row = []
        for track in range(TRACKS):
            base, target, kind, t0, li, rate, rb, rt = state[track]
            length = at_f - t0
            if kind == LINEAR:
                row.append(base + rate * length)
            elif kind == STEP:
                row.append(target if length >= li else base)
            elif kind == CUBIC:
                f16, f12 = li * li, Fraction(length) * length
                f18 = li * f12
                f14 = length * f12 * f16
                f20 = 2 * f14 * li
                f22 = 3 * f12 * f16
                f24 = f14 - f18
                row.append(base * ((f20 - f22) + 1) + target * (f22 - f20) +
                           rb * ((f24 - f18) + length) + rt * f24)
            else:
                row.append(None)
        out.append(tuple(row))
    return out


def _stream_frames(d: bytes, start: int) -> int:
    cmds, _, _ = decode(d, start)
    _, t_end = _keys(cmds)
    return 2 * t_end + 2


def compact_file(body: bytes, slots: dict[int, int], eps: int = 0):
    """Re-encode every stream of a FIGATREE body. `slots` maps each pointer
    slot (byte offset in the joint array) to its stream's offset. A stream
    the exact rules re-encode must simulate to the same values on every
    frame; one the eps fit re-encodes, to within eps plus the half unit a
    rounded knot adds. Returns (new_body, new_slots), or None when nothing
    got smaller."""
    if not slots:
        return None
    array_end = min(slots.values())
    if max(slots) + 4 > array_end:
        raise CompactError("joint pointers overlap the streams")
    # Only files that are the joint array and its streams: any other bytes
    # (data a translate-interpolation command points at) would not move.
    covered = [(0, array_end)]
    for target in set(slots.values()):
        cmds, _, _ = decode(body, target)
        if any(c[0] == 12 for c in cmds):
            return None
        covered.append((target, cmds[-1][5]))
    end = 0
    for lo, hi in sorted(covered):
        if lo > ((end + 3) & ~3):
            return None
        end = max(end, hi)
    # Past the last stream only alignment padding (O2R bodies are 16-byte
    # multiples).
    if any(body[((end + 3) & ~3):]):
        return None
    new_streams: dict[int, bytes] = {}
    changed = False
    for target in sorted(set(slots.values())):
        result = compact_stream(body, target, eps)
        cmds, _, _ = decode(body, target)
        old = body[target:cmds[-1][5]]
        if result is None:
            new_streams[target] = old
            continue
        words, lossy = result
        data = b"".join(struct.pack(">H", w) for w in words)
        frames = _stream_frames(body, target)
        before = _simulate(body, target, frames)
        scratch = bytes(data) + b"\0\0"
        after = _simulate(scratch, 0, frames)
        if not lossy and before != after:
            raise CompactError(f"stream {target:#x}: re-encoding changes its values")
        if lossy:
            bound = Fraction(2 * eps + 1, 2)
            for row_before, row_after in zip(before, after):
                for vb, va in zip(row_before, row_after):
                    if (vb is None) != (va is None) or (vb is not None and abs(vb - va) > bound):
                        raise CompactError(f"stream {target:#x}: the fit leaves its tolerance")
        new_streams[target] = data
        changed = True
    if not changed:
        return None
    out = bytearray(body[:array_end])
    for i in range(0, array_end, 4):
        if i in slots:
            out[i:i + 4] = b"\0\0\0\0"
    placed = {}
    for target in sorted(new_streams):
        while len(out) % 4:
            out += b"\0"
        placed[target] = len(out)
        out += new_streams[target]
    while len(out) % 4:
        out += b"\0"
    if len(out) >= len(body):
        return None
    return bytes(out), {slot: placed[t] for slot, t in slots.items()}


def o2r_with_body(blob: bytes, body: bytes, slots: dict[int, int]) -> bytes:
    """An O2R container (remix_rom.Rom.o2r's layout) with a new body whose
    internal pointer slots are `slots` (offset -> target); no extern slots."""
    header = blob[:0x40]
    file_id, intern, extern, n = struct.unpack_from("<IHHI", blob, 0x40)
    if extern != 0xFFFF or n != 0:
        raise CompactError("animation file has extern slots")
    words = bytearray(body)
    order = sorted(slots)
    for i, slot in enumerate(order):
        nxt = (order[i + 1] // 4) if i + 1 < len(order) else 0xFFFF
        target = slots[slot]
        if target % 4 or target // 4 > 0xFFFF:
            raise CompactError(f"slot {slot:#x}: target {target:#x} not encodable")
        struct.pack_into(">I", words, slot, (nxt << 16) | (target // 4))
    head = (order[0] // 4) if order else 0xFFFF
    out = bytearray(header)
    out += struct.pack("<IHHI", file_id, head, 0xFFFF, 0)
    out += struct.pack("<I", len(words))
    out += words
    return bytes(out)


def _o2r_body(blob: bytes):
    file_id, intern, extern, n = struct.unpack_from("<IHHI", blob, 0x40)
    if extern != 0xFFFF or n != 0:
        return None, None
    off = 0x4C + 2 * n
    size = struct.unpack_from("<I", blob, off)[0]
    body = blob[off + 4:off + 4 + size]
    slots = {}
    w = intern
    while w != 0xFFFF:
        val = struct.unpack_from(">I", body, w * 4)[0]
        slots[w * 4] = (val & 0xFFFF) * 4
        w = val >> 16
    return body, slots


def compact_o2r(blob: bytes, eps: int = 0):
    """The compacted O2R for an animation container, or None to keep it."""
    body, slots = _o2r_body(blob)
    if body is None:
        return None
    result = compact_file(body, slots, eps)
    if result is None:
        return None
    new_body, new_slots = result
    return o2r_with_body(blob, new_body, new_slots)


# ---------------------------------------------------------------------------
# AObjEvent32 joint animations (FTANIM_FLAG_ANIMJOINT: Remix's entries) as
# FIGATREE. gcParseDObjAnimJoint and ftAnimParseDObjFigatree keep the same
# AObj state per command; event32 carries f32 values and a 15-bit payload in
# the command word, event16 s16 values scaled per track
# (ftAnimGetTargetValue) and a payload halfword. Only the commands
# _simulate models convert; a stream with any other (jumps, interpolation
# pointers, callbacks) keeps the whole file event32.

EV32_TO_EV16 = {2: OP_BLOCK, 3: OP_SETVALBLOCK, 4: OP_SETVAL,
                5: OP_SETVALRATEBLOCK, 6: OP_SETVALRATE, 7: OP_SETTARGETRATE,
                8: OP_SETVAL0RATEBLOCK, 9: OP_SETVAL0RATE,
                10: OP_SETVALAFTERBLOCK, 11: OP_SETVALAFTER, 12: OP_1611,
                15: OP_SETFLAGS}
EV16_BLOCK_OPS = (OP_BLOCK, OP_SETVALBLOCK, OP_SETVALRATEBLOCK, OP_SETVAL0RATEBLOCK,
                  OP_SETVALAFTERBLOCK, OP_SETFLAGS)


def _ev16_scale(track: int) -> int:
    """1 / ftAnimGetTargetValue's value fraction for a track index."""
    if track <= 2:
        return 512        # rotation
    if track == 3:
        return 16384      # translation interpolation
    if track <= 6:
        return 4          # translation
    return 4096           # scale


def _ev16_rate_scale(track: int) -> int:
    """1 / ftAnimGetTargetValue's rate fraction for a track index."""
    if track <= 2:
        return 512
    if track == 3:
        return 16384
    if track <= 6:
        return 32
    return 8192


def _t_end(cmds) -> int:
    t = 0
    for op, flags, payload, values, _, _ in cmds:
        if op in (OP_END, OP_LOOP):
            return t
        if op in EV16_BLOCK_OPS:
            t += payload or 0
    raise CompactError("no End")


def _decode32(body: bytes, start: int, pointers: dict[int, int]):
    """An event32 stream as _simulate commands with exact values in event16
    units, plus whether it loops to its start; None when unconvertible."""
    o = start
    cmds = []
    for _ in range(1 << 20):
        w = struct.unpack_from(">I", body, o)[0]
        op, flags, payload = w >> 25, (w >> 15) & 0x3FF, w & 0x7FFF
        at = o
        o += 4
        if op == 0:
            cmds.append((OP_END, 0, None, (), at, o))
            return cmds, False
        if op == 14:  # SetAnim: the loop, when it names this stream's start
            if pointers.get(o) != start:
                return None
            o += 4
            cmds.append((OP_LOOP, 0, None, (), at, o))
            return cmds, True
        if op not in EV32_TO_EV16:
            return None
        op16 = EV32_TO_EV16[op]
        if op16 == OP_BLOCK:
            cmds.append((OP_BLOCK, 0, payload, (), at, o))
            continue
        if op16 in (OP_1611, OP_SETFLAGS):
            # 1611 lengthens its tracks; SetFlags writes DObj flags (kept
            # verbatim, not tracks) and waits.
            cmds.append((op16, flags, payload, (), at, o))
            continue
        if op16 == OP_SETTARGETRATE:
            payload = 0  # the source reads no payload for it
        values = []
        for track in range(TRACKS):
            if not flags & (1 << track):
                continue
            kinds = (("value",) if op16 not in (OP_SETVALRATEBLOCK, OP_SETVALRATE, OP_SETTARGETRATE)
                     else ("value", "rate") if op16 != OP_SETTARGETRATE else ("rate",))
            for kind in kinds:
                f = struct.unpack_from(">f", body, o)[0]
                o += 4
                if f != f or abs(f) == float("inf"):
                    return None
                scale = _ev16_scale(track) if kind == "value" else _ev16_rate_scale(track)
                values.append(Fraction(f) * scale)
        cmds.append((op16, flags, payload, tuple(values), at, o))
    return None


def convert32_body(body: bytes, slots: dict[int, int]):
    """An event32 joint-animation body as an equivalent FIGATREE body:
    (body, slots), or None. Every value rounds to its event16 unit, and every
    frame of the result is checked against the event32 stream to within half
    a unit."""
    if not slots:
        return None
    array_end = min(t for s, t in slots.items() if s < min(slots.values()))
    joints = {s: t for s, t in slots.items() if s < array_end}
    pointers = {s: t for s, t in slots.items() if s >= array_end}
    streams = {}
    for target in sorted(set(joints.values())):
        decoded = _decode32(body, target, pointers)
        if decoded is None:
            return None
        cmds, loops = decoded
        words = []
        for op, flags, payload, values, _, _ in cmds:
            if op in (OP_END, OP_LOOP):
                break
            if op == OP_BLOCK:
                words += [(OP_BLOCK << 11) | (1 if payload else 0)] + ([payload] if payload else [])
                continue
            words.append((op << 11) | (flags << 1) | (1 if payload else 0))
            if payload:
                words.append(payload)
            for v in values:
                q = round(v)
                if not -0x8000 <= q <= 0x7FFF:
                    return None
                words.append(q & 0xFFFF)
        if loops:
            words += [OP_LOOP << 11, 0]
            words[-1] = (-2 * (len(words) - 1)) & 0xFFFF
        else:
            words.append(OP_END << 11)
        data = b"".join(struct.pack(">H", x) for x in words)
        frames = 2 * _t_end(cmds) + 2
        exact = _simulate_cmds(cmds, 0, frames)
        quantized = _simulate(data + b"\0\0", 0, frames)
        # Half a unit for each rounded value; a rounded rate can add up to
        # half a rate unit a frame inside a cubic segment. One unit (1/512
        # rad, 1/4 unit of translation) bounds both.
        for row_e, row_q in zip(exact, quantized):
            for ve, vq in zip(row_e, row_q):
                if (ve is None) != (vq is None) or (
                        ve is not None and abs(ve - vq) > 1 + Fraction(1, 1 << 20)):
                    return None
        streams[target] = data
    out = bytearray(body[:array_end])
    for i in range(0, array_end, 4):
        if i in joints:
            out[i:i + 4] = b"\0\0\0\0"
    placed = {}
    for target in sorted(streams):
        while len(out) % 4:
            out += b"\0"
        placed[target] = len(out)
        out += streams[target]
    while len(out) % 16:
        out += b"\0"
    return bytes(out), {s: placed[t] for s, t in joints.items()}


def convert32_o2r(blob: bytes, eps: int = 0):
    """An event32 joint-animation container as a (compacted) FIGATREE
    container, or None to keep it event32."""
    body, slots = _o2r_body(blob)
    if body is None:
        return None
    converted = convert32_body(body, slots)
    if converted is None:
        return None
    new_body, new_slots = converted
    compacted = compact_file(new_body, new_slots, eps)
    if compacted is not None:
        new_body, new_slots = compacted
    return o2r_with_body(blob, new_body, new_slots)
