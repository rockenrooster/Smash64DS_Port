#!/usr/bin/env python3
"""MF experiment: exact structural view of one BPS1 clip.

A BPS1 clip (generate_battlepack_anim.py:547-583) is
    u32 slot[n]            byte offset of the slot's script from the clip base,
                           0 = NULL slot (joint does not animate)
    run, run, ...          each distinct script once, first-use order, every
                           start 4 B aligned (zero fill)
A run is a native-order AObjEvent16 stream (`opcode = w & 0x1f`,
`flags = (w >> 5) & 0x3ff`, `toggle = w >> 15`; ftanim_reloc_probe.py:262),
walked linearly by the pose parser (`src/nds/nds_ft_pose.c:709-980`) until
End, or until a Loop jumps back. Bytes after that terminator up to the next
run (alignment fill, or source padding) are never read, but are kept here as
`tail` so that parse -> serialise is the identity on every clip.

Command word counts are `ndsRelocAObj16CommandWords` (probe.command_words):
    Loop, TraI         2
    others             1 + toggle + per * popcount(flags)
    per = 1 for ops 2,3,6,7,8,9,10;  2 for ops 4,5;  0 for ops 0,1,11,14
"""

from __future__ import annotations

import struct

OP_END, OP_BLOCK, OP_INTERP, OP_LOOP, OP_SETFLAGS = 0, 1, 12, 13, 14
PER_TRACK = {2: 1, 3: 1, 6: 1, 7: 1, 8: 1, 9: 1, 10: 1, 4: 2, 5: 2}
OP_NAMES = {0: "End", 1: "Block", 2: "SetValBlock", 3: "SetVal",
            4: "SetValRateBlock", 5: "SetValRate", 6: "SetTargetRate",
            7: "SetVal0RateBlock", 8: "SetVal0Rate", 9: "SetValAfterBlock",
            10: "SetValAfter", 11: "Ev1611", 12: "SetTranslateInterp",
            13: "Loop", 14: "SetFlags"}


def slot_table(clip: bytes):
    """-> list of u32 slot words. The table ends where the first run starts
    (runs begin immediately after it, emit_stream_pack:568)."""
    bound = len(clip)
    words = []
    i = 0
    while 4 * i < bound:
        w = struct.unpack_from("<I", clip, 4 * i)[0]
        words.append(w)
        if w != 0 and w < bound:
            bound = w
        i += 1
    if 4 * len(words) != bound and bound != len(clip):
        raise ValueError("slot table does not abut the first run")
    return words


def split_runs(clip: bytes):
    """-> (slots, [(offset, span_bytes)]) with spans covering the clip
    exactly from the end of the table (each span runs to the next run start
    or to the clip end)."""
    slots = slot_table(clip)
    starts = sorted({w for w in slots if w != 0})
    if starts and starts[0] != 4 * len(slots):
        raise ValueError("first run is not at the table end")
    spans = []
    for i, s in enumerate(starts):
        e = starts[i + 1] if i + 1 < len(starts) else len(clip)
        spans.append((s, clip[s:e]))
    return slots, spans


def parse_run(span: bytes):
    """Linear command walk of one run span.

    -> (commands, tail) where each command is a dict
       {op, flags, toggle, payload (u16|None), vals [s16...], jump (s16|None)}
    and `tail` is the span's bytes after the terminator."""
    cmds = []
    pc = 0
    n = len(span)
    while True:
        if pc + 2 > n:
            raise ValueError("run has no terminator")
        w = struct.unpack_from("<H", span, pc)[0]
        op, flags, toggle = w & 0x1F, (w >> 5) & 0x3FF, w >> 15
        cmd = {"op": op, "flags": flags, "toggle": toggle, "word": w,
               "payload": None, "vals": [], "jump": None}
        if op in (OP_LOOP, OP_INTERP):
            cmd["jump"] = struct.unpack_from("<h", span, pc + 2)[0]
            pc += 4
            cmds.append(cmd)
            if op == OP_LOOP:
                break
            continue
        p = pc + 2
        if op == OP_END:
            cmds.append(cmd)
            pc = p
            break
        if toggle:
            cmd["payload"] = struct.unpack_from("<H", span, p)[0]
            p += 2
        per = PER_TRACK.get(op, 0)
        nv = per * bin(flags).count("1")
        if nv:
            cmd["vals"] = list(struct.unpack_from("<%dh" % nv, span, p))
            p += 2 * nv
        cmds.append(cmd)
        pc = p
    return cmds, span[pc:]


def serialise_run(cmds, tail):
    out = bytearray()
    for c in cmds:
        out += struct.pack("<H", c["word"])
        if c["op"] in (OP_LOOP, OP_INTERP):
            out += struct.pack("<h", c["jump"])
            continue
        if c["op"] == OP_END:
            continue
        if c["toggle"]:
            out += struct.pack("<H", c["payload"])
        if c["vals"]:
            out += struct.pack("<%dh" % len(c["vals"]), *c["vals"])
    return bytes(out) + bytes(tail)


def parse_clip(clip: bytes):
    slots, spans = split_runs(clip)
    runs = []
    for off, span in spans:
        cmds, tail = parse_run(span)
        runs.append({"off": off, "cmds": cmds, "tail": bytes(tail)})
    return {"slots": slots, "runs": runs, "size": len(clip)}


def serialise_clip(parsed):
    out = bytearray()
    for w in parsed["slots"]:
        out += struct.pack("<I", w)
    for r in parsed["runs"]:
        if len(out) != r["off"]:
            raise ValueError("run offset drift")
        out += serialise_run(r["cmds"], r["tail"])
    return bytes(out)
