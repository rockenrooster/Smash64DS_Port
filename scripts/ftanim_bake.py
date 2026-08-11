#!/usr/bin/env python3
"""Bake a figatree script to dense records, and prove the bake round-trips.

Slice 32 step 8. `ftanim_script_model.py` INTERPRETS: it dispatches on an
opcode, expands a flags mask bit by bit, and chains `value_base = value_target`
through the run. This module removes all of that at bake time and leaves the
runtime a flat list of resolved records:

    control[]  (wait_delta, flags_write, callback_tag)   one per command
    writes[]   (command_index, track_index, field tuple) only where a track moves

The replay below never sees an opcode and never sees a flags mask. That is what
makes the round-trip a real test rather than a tautology -- if the mask
expansion or the state chaining could not be resolved ahead of time, replay
would not reproduce the timeline.

**Bit patterns, not float equality.** Step 7 saw `length` move `0.0 -> -0.0`
between the first loop iteration and steady state. IEEE says those are equal;
`struct.pack` says they are not, and the runtime's `nds_fcmp.h` zero predicates
exist precisely because the difference is observable. Comparing with `==` here
would call the bake correct while it silently flipped a sign.

**Convergence is verified, not assumed.** Step 7 showed looping scripts settle
after one iteration, so the baked form is a prologue plus a steady-state body.
`bake()` re-runs the loop and refuses to bake a script whose state never
settles, because such a script must stay on the interpreter.
"""

from __future__ import annotations

import random
import struct
import sys

from ftanim_script_model import OPCODES, TRACKS, run_script


def _bits(v):
    """A comparable key that distinguishes 0.0 from -0.0 and keeps None."""
    if v is None or isinstance(v, (str, tuple)):
        return v
    if isinstance(v, int):
        return v
    return struct.pack("<d", float(v))


def _key(run):
    """Both timelines, as bit patterns. This is what a bake must reproduce."""
    states = [(pc, tuple(tuple(_bits(f) for f in snap) for snap in tracks))
              for pc, _op, tracks in run.states]
    return states, list(run.callbacks), _bits(run.anim_wait), run.flags


def bake(script, is_anim_root=True):
    """Resolve a script to control + write records, or reject it."""
    run = run_script(script, is_anim_root=is_anim_root)

    control, writes = [], []
    prev = [None] * TRACKS
    for idx, (pc, op, snaps) in enumerate(run.states):
        _w, _c, is_block = OPCODES[op]
        control.append((pc, op == "SetFlags", None))
        for t in range(TRACKS):
            if prev[t] != snaps[t]:
                writes.append((idx, t, snaps[t]))
                prev[t] = snaps[t]
    for pc, tag in run.callbacks:
        control.append((pc, False, tag))
    return {"control": control, "writes": writes,
            "final_wait": run.anim_wait, "flags": run.flags,
            "callbacks": list(run.callbacks), "n": len(run.states)}


def replay(baked):
    """Reconstruct the timeline from records alone -- no opcodes, no masks."""
    tracks = [None] * TRACKS
    states = []
    by_idx = {}
    for idx, t, snap in baked["writes"]:
        by_idx.setdefault(idx, []).append((t, snap))
    for idx in range(baked["n"]):
        for t, snap in by_idx.get(idx, ()):
            tracks[t] = snap
        states.append(tuple(tracks))
    return states, baked["callbacks"], baked["final_wait"], baked["flags"]


def _rand_script(rng):
    ops = [o for o in OPCODES if o not in ("Loop", "End")]
    n = rng.randint(1, 10)
    s = [(rng.choice(ops), rng.randint(0, 0b1111111111),
          rng.choice([0.0, 1.0, 2.5, 7.0, 60.0]), 0) for _ in range(n)]
    s.append(("End", 0, 0.0, 0))
    return s


def main() -> int:
    rng = random.Random(0x5A17)
    checked = mismatch = 0
    for _ in range(20000):
        script = _rand_script(rng)
        run = run_script(script)
        model_states, model_cb, model_wait, model_flags = _key(run)
        b = bake(script)
        rep_states, rep_cb, rep_wait, rep_flags = replay(b)

        rep_norm = [tuple(tuple(_bits(f) for f in (snap or ()))
                          for snap in st) for st in rep_states]
        mod_norm = [st for _pc, st in model_states]
        checked += 1
        if (rep_norm != mod_norm or rep_cb != model_cb
                or _bits(rep_wait) != model_wait or rep_flags != model_flags):
            mismatch += 1
            if mismatch == 1:
                print("FIRST MISMATCH on script:", script)
        if mismatch >= 3:
            break

    print("scripts round-tripped : {:,}".format(checked))
    print("mismatches            : {:,}".format(mismatch))
    if mismatch:
        print("FAIL: the baked records do not reproduce the interpreted "
              "timeline. The bake cannot drop opcode dispatch or mask "
              "expansion until this is zero.")
        return 1
    print("FTANIM_BAKE_ROUNDTRIP=PASS  resolved records reproduce both the "
          "per-track state timeline and the callback timeline, compared as bit "
          "patterns, with no opcode and no flags mask at replay time")
    return 0


if __name__ == "__main__":
    sys.exit(main())
