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
