# Handoff

Current: ACTIVE. Contract: docs/reviews/NATIVE_ONLY_IMPLEMENTATION_GOAL.md — every
new ROM, diagnostics and profiling included, must exclude reference renderers and
software scene compositors. Host reference tools are allowed.
**1P campaign remains paused.** Main Menu/VS/VS Options/Option/Backup Clear accepted;
owner symptoms are in docs/BUGS.md and the evidence in docs/p2/BUG_NOTES.md.

## Current checkpoint

Pushed through a1257a4c9bc. Landed tonight: the fighter joint bound moved from 27
to 40 and took **Yoshi from 8,360 native rejects to 350**; Saffron's gate is a
native surface and its 1,306-per-300 row is closed; the zero-alpha promotion is
conditioned at the triangle; every fighter has its own costume rows again.

## The owner's standing goal: zero native failures, everywhere

`scripts/diagnostics/probe-native-render-batch.ps1` runs cases concurrently on up to
12 runner slots. **Measure at 1,200** — stages that pass at 300 fail at match
length, and Mushroom Kingdom is the proof. `native-stage-all.json` (nine stages)
and `native-fighter-long.json` (nine fighters) are the two waves; each takes about
210 s at `-MaxParallel 6`. Freeze the ROM and ELF into `builds/resume-20260908/frozen/`
for a wave taken beside a build. A fighter case pokes
`gNdsMenuShellCssWalkTargetKind`/`...Kind2` for a mirror match and asserts the kind
committed, zero failures and non-zero owner triangles.

Measured 2026-09-08 22:20 on one build. **Clean**: Dream Land, Zebes, Yoshi's
Island; Mario, Fox, Luigi, Donkey. **Failing**, per 1,200 presents:

- Castle 1,260, Item asset 86 root 0x7558, the bumper. Owner reviewed UNSAFE:
  it cannot tell `nITKindGBumper` from `nITKindNBumper`, there are four guard
  sites and not three, and the generator must assert the CI4 palette indices.
- Mushroom Kingdom 161, Congo 56, Hyrule 54, Captain 46 — **one row, four
  scenes**: the seven runtime visual templates. Asset 0xffff and a RAM root are
  CORRECT for that class; the lists are built into the taskman arena.
- Sector Z 110, Weapon asset 153 root 0x1c50. Generator and executor landed.
- Saffron 59, Item asset 159 root 0x6a0, the Marumine body. Identified, no owner.
- Link 3,007, entry-pose topology shift; codex is baking a second owner program.
- Pikachu 137 (MBallRays), Samus 91 (Charge Shot, not the bomb).
- Yoshi 350, `DIAG_FTCOMPOSE=5,28,23` — a uniform zero accumulated scale, which
  is how source collapses a subtree, being declined as a corrupt chain.

Two corrections worth carrying: `material 0` in a failure record means
`dobj->mobj == NULL` and **not** untextured — the Sector laser is fully textured.
And the vertex-alpha promotion is load-bearing for 300 triangles; only 17 on
Saffron and Zebes were defective. Yoshi's Island transparency is texel-side.

## Preserved work and operating rules

Broad unrelated dirty work (1P, tags, pipes, assets, user P3/P4 docs) must be
preserved; do not resume campaign or redo CSS repairs.
**Launch codex as `-m "chatgpt-web/extra-high" -c model_reasoning_effort="xhigh"`,
and pipe the prompt on stdin** — a long prompt as an argument exceeds the command
line. GLM works now (`mode: subagent` silently ran the default agent); **one GLM
at a time**, up to five Muse beside it, and no prompt word may start with `-`.
Launch writers only between builds, never let one run `make`, one build at a time,
no -j/MAKEFLAGS. CodeGraph first; a restart reads this file and the board.
Start each cycle with verify-all.ps1 -Profile Boundary -List and git status
--short. Boundary's `p2_battle_realtime` and `p2_fourcpu_stress` arms are still
unmeasured, and the published `smash64ds.nds` is stale.
