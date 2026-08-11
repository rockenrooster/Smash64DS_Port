#!/usr/bin/env python3
"""A host model of the figatree script semantics, for slice 32.

This is the front half of the AOT dense-track generator and the reference the
replay proof compares against. It executes the same fifteen opcodes
`ndsR2FtAnimParseDObjFigatree` executes and records TWO timelines:

  * per-track AObj state after every command, and
  * the **gameplay callbacks** (`func_anim` with tag -2 at Loop, -1 at End).

Both matter. The callbacks were found by reading all fifteen case bodies during
slice 32 step 4, and they are why this bake changes GAMEPLAY rather than
presentation: a baked track that reproduces every AObj field and fires a
callback one frame late is a gameplay bug that field-by-field comparison cannot
see. `scripts/check_ftanim_opcode_surface.py` guards the claim that Loop and End
are the only callback sites and SetFlags the only DObj-flag write.

Semantics transcribed from the port parser, not from the decomp, because the
port is what ships and the two differ (the port range-checks tracks, memoises
the f32 vertex conversion, and since cycle 117 builds its track table lazily).
Opcode field-write map, verbatim from slice 32 step 2:

    SetVal0Rate*   value_base<-value_target, value_target, rate_base<-rate_target,
                   rate_target=0, kind=CUBIC, length_invert=recip, length=segstart
    SetVal*        as above without length_invert
    SetValRate*    as SetVal0Rate plus an explicit rate_target
    SetTargetRate  rate_target only
    SetValAfter*   value_base<-value_target, value_target, kind=STEP,
                   length_invert=frames, length=len_new, rate_target=0
    Event1611      add_length only
    TranslateInterp interpolate <- offset of (event16 + s/2)
    Block/SetFlags conditional anim_wait += payload; SetFlags also writes flags
    Loop           relative jump by s/2, anim_frame=-anim_wait, callback(-2)
    End            tail advance, anim_wait=END, callback(-1), STOP

`*Block` variants additionally advance `anim_wait` by the payload; that is the
only difference between each pair, which is why the parser fallthroughs them.

This module deliberately has NO dependency on the ROM or on decomp: it is
driven by scripts supplied as opcode tuples, so the emitter can be developed and
proven before any asset plumbing exists.
"""

from __future__ import annotations

KIND_NONE, KIND_STEP, KIND_LINEAR, KIND_CUBIC = 0, 5, 6, 7
ANIM_END = "END"
TRACKS = 10                      # nGCAnimTrackJointStart .. JointEnd

# opcode -> (writes_state, consumes_payload, is_block_variant)
OPCODES = {
    "SetVal0RateBlock":  (True,  True,  True),
    "SetVal0Rate":       (True,  True,  False),
    "SetValBlock":       (True,  True,  True),
    "SetVal":            (True,  True,  False),
    "SetValRateBlock":   (True,  True,  True),
    "SetValRate":        (True,  True,  False),
    "SetTargetRate":     (True,  True,  False),
    "SetValAfterBlock":  (True,  True,  True),
    "SetValAfter":       (True,  True,  False),
    "Event1611":         (False, True,  False),
    "SetTranslateInterp": (False, False, False),
    "Block":             (False, True,  False),
    "SetFlags":          (False, True,  False),
    "Loop":              (False, False, False),
    "End":               (False, False, False),
}


class Track:
    """One AObj, in the fields the parser writes."""

    __slots__ = ("kind", "value_base", "value_target", "rate_base",
                 "rate_target", "length", "length_invert", "interpolate")

    def __init__(self) -> None:
        self.kind = KIND_NONE
        self.value_base = 0.0
        self.value_target = 0.0
        self.rate_base = 0.0
        self.rate_target = 0.0
        self.length = 0.0
        self.length_invert = 1.0
        self.interpolate = None

    def snapshot(self) -> tuple:
        return (self.kind, self.value_base, self.value_target, self.rate_base,
                self.rate_target, self.length, self.length_invert,
                self.interpolate)


class Run:
    """The result of executing a script: what a baked track must reproduce."""

    def __init__(self) -> None:
        self.states: list = []      # (pc, opcode, [track snapshots])
        self.callbacks: list = []   # (pc, tag) -- the gameplay-visible part
        self.anim_wait = 0.0
        self.anim_frame = 0.0
        self.flags = 0
        self.stopped = False


def run_script(script, is_anim_root=True, anim_wait=0.0):
    """Execute `script`, a list of (opcode, flags_mask, payload, s) tuples.

    `flags_mask` selects which of the 10 tracks the command applies to, exactly
    like the parser's `flags >> 1` loop, which STOPS at the first zero mask
    rather than scanning all ten -- so a mask of 0b101 touches tracks 0 and 2,
    and a mask of 0 touches none.
    """
    tracks = [Track() for _ in range(TRACKS)]
    run = Run()
    run.anim_wait = anim_wait
    pc = 0
    guard = 0

    while pc < len(script):
        guard += 1
        if guard > 100000:
            raise RuntimeError("script did not terminate; a Loop has no End")
        op, mask, payload, s = script[pc]
        if op not in OPCODES:
            raise KeyError("unknown opcode %r -- the surface is closed at 15, "
                           "see check_ftanim_opcode_surface.py" % op)
        writes, _consumes, is_block = OPCODES[op]

        if op == "End":
            run.anim_frame = run.anim_wait
            run.anim_wait = ANIM_END
            if is_anim_root:
                run.callbacks.append((pc, -1))
            run.stopped = True
            break

        if op == "Loop":
            run.anim_frame = -run.anim_wait
            if is_anim_root:
                run.callbacks.append((pc, -2))
            pc += max(1, s // 2)
            continue

        if op in ("Block", "SetFlags"):
            if op == "SetFlags":
                run.flags = mask
            if payload:
                run.anim_wait += payload
            pc += 1
            run.states.append((pc, op, [t.snapshot() for t in tracks]))
            continue

        if op == "SetTranslateInterp":
            tracks[TRACKS - 1].interpolate = ("offset", pc + s // 2)
            pc += 1
            run.states.append((pc, op, [t.snapshot() for t in tracks]))
            continue

        if writes or op == "Event1611":
            m = mask
            for i in range(TRACKS):
                if m == 0:
                    break               # the parser's early exit, not a scan
                if m & 1:
                    _apply(tracks[i], op, payload, run)
                m >>= 1
            if is_block:
                run.anim_wait += payload

        pc += 1
        run.states.append((pc, op, [t.snapshot() for t in tracks]))

    return run


def _apply(t: Track, op: str, payload: float, run: Run) -> None:
    if op == "SetTargetRate":
        t.rate_target = payload
        return
    if op == "Event1611":
        t.length += payload         # ndsR2AnimAddLength
        return
    if op.startswith("SetValAfter"):
        t.value_base = t.value_target
        t.value_target = payload
        t.kind = KIND_STEP
        t.length_invert = payload    # frames slot
        t.length = run.anim_frame
        t.rate_target = 0.0
        return
    # the three cubic-writing families
    t.value_base = t.value_target
    t.value_target = payload
    t.rate_base = t.rate_target
    t.rate_target = payload if op.startswith("SetValRate") else 0.0
    t.kind = KIND_CUBIC
    t.length = run.anim_frame
    if op.startswith("SetVal0Rate") or op.startswith("SetValRate"):
        if payload:
            t.length_invert = 1.0 / payload


# --------------------------------------------------------------------------
# Real content.
#
# `run_script` above is driven by synthetic tuples in which one `payload` stands
# in for every per-track value, which is what the 20,000-script round-trip in
# `ftanim_bake.py` proves against. Real figatree commands do not work that way:
# a command carries an optional payload AND a separate TARGET word for each
# selected track, two of them for `SetValRate{,Block}`. `run_commands` consumes
# the structured commands `ftanim_reloc_probe.decode_script()` produces, so the
# generator runs the same semantics over the shipped bank.
#
# It is a second entry point rather than a change to `run_script` on purpose:
# the synthetic proof is the only thing keeping the bake honest right now, and
# rewriting the function it validates would invalidate it.
#
# Opcode numbers are the AObjEvent16Kind ordinals (objdef.h:169-187).
OP = {0: "End", 1: "Block", 2: "SetValBlock", 3: "SetVal",
      4: "SetValRateBlock", 5: "SetValRate", 6: "SetTargetRate",
      7: "SetVal0RateBlock", 8: "SetVal0Rate", 9: "SetValAfterBlock",
      10: "SetValAfter", 11: "Event1611", 12: "SetTranslateInterp",
      13: "Loop", 14: "SetFlags"}
BLOCK_OPS = {2, 4, 7, 9}            # the *Block halves advance anim_wait


def run_commands(cmds, is_anim_root=True, anim_speed=1.0, anim_wait=0.0):
    """Execute decoded real commands, recording the same two timelines.

    `anim_speed` is a DObj field rather than script data, so it is a parameter;
    `length` is written as `-anim_wait - anim_speed`, the segment start the
    parser computes in `ndsR2AnimSegmentStart`.
    """
    tracks = [Track() for _ in range(TRACKS)]
    run = Run()
    run.anim_wait = anim_wait
    # Per-command `anim_wait` AFTER the command, parallel to `run.states`.
    #
    # Without this the bake is not runnable. `states` carries what each command
    # writes and `callbacks` carries what it signals, but the *timing* -- the
    # `anim_wait += payload` that decides which frame the next command lands on
    # -- lived only in the running total, so a baked script reproduced every
    # AObj field and had no idea when to apply them. The runtime bind consumes
    # this as its control stream.
    run.waits = []

    for cmd in cmds:
        op = cmd["op"]
        if cmd.get("cyclic"):
            break                    # the script loops; the frame budget ends it
        payload = cmd["payload"] or 0
        if op == 0:                                             # End
            run.anim_frame = run.anim_wait
            run.anim_wait = ANIM_END
            if is_anim_root:
                run.callbacks.append((cmd["pc"], -1))
            run.stopped = True
            break
        if op == 13:                                            # Loop
            run.anim_frame = -run.anim_wait
            if is_anim_root:
                run.callbacks.append((cmd["pc"], -2))
            continue
        if op == 12:                                            # TranslateInterp
            tracks[TRACKS - 1].interpolate = ("offset", cmd["jump"])
        elif op == 14:                                          # SetFlags
            run.flags = cmd["flags"]
            run.anim_wait += payload
        elif op == 1:                                           # Block
            run.anim_wait += payload
        else:
            seg = -run.anim_wait - anim_speed
            for bit, vals in cmd["targets"]:
                _apply_real(tracks[bit], op, payload, vals, seg)
            if op in BLOCK_OPS:
                run.anim_wait += payload
        run.states.append((cmd["pc"], OP[op], [t.snapshot() for t in tracks]))
        run.waits.append(run.anim_wait)

    return run


def _apply_real(t, op, payload, vals, seg):
    """Apply one command to one track.

    Every value is coerced to float because these are the `f32` fields of an
    AObj. The decoder hands back s16 ints, and leaving them as ints made the
    real-content bake report false mismatches: Python calls `0 == 0.0` true, so
    the resolver emitted no write record, while the bit-pattern comparison the
    proof runs on distinguishes an int `0` from a float `0.0`. Storing the wrong
    Python type in a model of an f32 field is simply a bug -- found by running
    the bake on the shipped bank, which the synthetic scripts could not surface
    because they only ever supply floats.
    """
    payload = float(payload)
    if op == 6:                                     # SetTargetRate
        t.rate_target = float(vals[0])
        return
    if op == 11:                                    # Event1611
        t.length += payload
        return
    if op in (9, 10):                               # SetValAfter{,Block}
        t.value_base = t.value_target
        t.value_target = float(vals[0])
        t.kind = KIND_STEP
        t.length_invert = payload                   # a FRAME COUNT here
        t.length = seg
        t.rate_target = 0.0
        return
    t.value_base = t.value_target
    t.value_target = float(vals[0])
    if op in (2, 3):                                # SetVal{,Block} -> Linear
        t.kind = KIND_LINEAR
        if payload:
            t.rate_base = (t.value_target - t.value_base) / payload
        t.length = seg
        t.rate_target = 0.0
        return
    t.rate_base = t.rate_target                     # the cubic families
    t.rate_target = float(vals[1]) if op in (4, 5) else 0.0
    t.kind = KIND_CUBIC
    if payload:
        t.length_invert = 1.0 / payload
    t.length = seg


if __name__ == "__main__":
    demo = [
        ("SetVal0Rate", 0b101, 4.0, 0),
        ("Block", 0, 2.0, 0),
        ("SetValAfter", 0b1, 3.0, 0),
        ("SetTranslateInterp", 0, 0.0, 4),
        ("End", 0, 0.0, 0),
    ]
    r = run_script(demo)
    print("commands executed :", len(r.states))
    print("callbacks         :", r.callbacks)
    print("anim_wait          :", r.anim_wait)
    print("track0 final       :", r.states[-1][2][0])
    assert r.callbacks == [(4, -1)], r.callbacks
    assert r.stopped
    print("FTANIM_SCRIPT_MODEL=OK  both timelines produced")
