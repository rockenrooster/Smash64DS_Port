#!/usr/bin/env python3
"""Bake every real Mario/Fox animation and prove the records reproduce it.

Slice 32. `ftanim_bake.py` proves the bake on 20,000 SYNTHETIC scripts, which
establishes the resolver but says nothing about the shipped content: the
synthetic driver collapses every per-track value into one `payload`, and real
figatree commands carry a separate TARGET word per selected track (two for
`SetValRate{,Block}`, which is 69.6% of the bank). This runs the same resolver
over the real thing.

Three layers have to line up for this to pass, and each was wrong at some point
in cycle 117:

  * `ftanim_reloc_probe.py` reads the o2r bank the way the ROM does -- including
    the MSB-first command bit order that eight earlier readings missed.
  * `ftanim_script_model.run_commands` executes those commands with per-track
    targets and the real `-anim_wait - anim_speed` segment start.
  * `ftanim_bake.bake_run` resolves the run to control + write records, and
    `replay` reconstructs the timeline from records ALONE -- no opcode, no flags
    mask. That is what makes this a test rather than a tautology.

Comparison is by `struct.pack` bit pattern, not `==`, because step 7 saw
`length` move `0.0 -> -0.0` and IEEE calls those equal while the runtime's
`nds_fcmp.h` zero predicates do not.

A failure here means the dense-track format cannot represent some real
animation, which is exactly what has to be known before an emitter is written
against it.
"""

from __future__ import annotations

import importlib.util
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))


def _load(name):
    spec = importlib.util.spec_from_file_location(
        name, ROOT / "scripts" / ("%s.py" % name))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def main() -> int:
    probe = _load("ftanim_reloc_probe")
    if not probe.BANK.is_dir():
        print("SKIP: %s absent -- run scripts/fetch-battleship-reference.ps1"
              % probe.BANK)
        return 0

    model = _load("ftanim_script_model")
    bake = _load("ftanim_bake")

    scripts = commands = records = 0
    mismatches = []
    empty = 0
    for path in sorted(probe.BANK.iterdir()):
        if not path.name.startswith(("FTMarioAnim", "FTFoxAnim")):
            continue
        for cmds in probe.scripts_in(path):
            scripts += 1
            commands += len(cmds)
            run = model.run_commands(cmds)
            if not run.states:
                empty += 1
                continue
            baked = bake.bake_run(run)
            records += len(baked["writes"])

            rep_states, rep_cb, rep_wait, rep_flags = bake.replay(baked)
            want = [tuple(tuple(bake._bits(f) for f in snap) for snap in st)
                    for _pc, _op, st in run.states]
            got = [tuple(tuple(bake._bits(f) for f in (snap or ()))
                         for snap in st) for st in rep_states]
            if (got != want or rep_cb != list(run.callbacks)
                    or bake._bits(rep_wait) != bake._bits(run.anim_wait)
                    or rep_flags != run.flags):
                mismatches.append(path.name)
                if len(mismatches) >= 5:
                    break
        if len(mismatches) >= 5:
            break

    print("scripts baked   : %d" % scripts)
    print("commands        : %d" % commands)
    print("write records   : %d" % records)
    print("scripts with no state (pure Block/Loop) : %d" % empty)
    print("mismatches      : %d" % len(mismatches))
    if mismatches:
        print("\nfirst failures:", ", ".join(mismatches))
        print("FAIL: the baked records do not reproduce the interpreted "
              "timeline for real content. The dense format cannot represent "
              "these animations as written -- fix that before the emitter.")
        return 1
    if scripts == 0:
        print("FAIL: no scripts were read; the bank or the reader moved.")
        return 1
    print("\nFTANIM_REAL_BAKE=PASS  every script in the shipped Mario/Fox bank "
          "resolves to control + write records that reproduce its per-track "
          "state timeline and its callback timeline, compared as bit patterns, "
          "with no opcode and no flags mask at replay time")
    return 0


if __name__ == "__main__":
    sys.exit(main())
