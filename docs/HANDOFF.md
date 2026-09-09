# Handoff

Current: ACTIVE. Contract: docs/reviews/NATIVE_ONLY_IMPLEMENTATION_GOAL.md — every
new ROM, diagnostics and profiling included, must exclude reference renderers and
software scene compositors. Host reference tools are allowed.
**1P campaign remains paused.** Main Menu/VS/VS Options/Option/Backup Clear accepted;
owner symptoms are in docs/BUGS.md and the evidence in docs/p2/BUG_NOTES.md.

## Current checkpoint

Pushed through 8c1c5c67f0b. **Seven of nine stages and five of nine fighters
are clean.** Landed since the last handoff: five native owners -- the runtime
visual templates (four rows at once), the Sector Z Arwing laser, the Peach
Castle bumper, and Link's entry and catch owner programs -- plus the fighter
joint bound, the Saffron gate, and the conditioned alpha promotion.

## The owner standing goal: zero native failures, everywhere

`scripts/diagnostics/probe-native-render-batch.ps1` runs cases concurrently on up to
12 runner slots. **Measure at 1,200** -- stages that pass at 300 fail at match
length. `native-stage-all.json` (nine stages) and `native-fighter-long.json`
(nine fighters) are the two waves; each takes about 210 s at `-MaxParallel 6`.
A fighter case pokes `gNdsMenuShellCssWalkTargetKind`/`...Kind2` for a mirror
match and asserts the kind committed, zero failures and non-zero owner triangles.

Measured 2026-09-09 06:45. **Clean**: Castle, Hyrule, Congo, Zebes, Sector Z,
Yoshi's Island, Dream Land; Mario, Fox, Luigi, Donkey, Captain. **Open**, per
1,200 presents:

- Mushroom Kingdom 105, Item asset 155 root 0x10d0. A SECOND object in the
  Pakkun file; unidentified.
- Saffron 59, Item asset 159 root 0x6a0, the Marumine body.
- Link 2,000, Item asset 353 root 0x16f8. His fighter row is closed; this was
  always happening behind it.
- Yoshi 342, `DIAG_FTCOMPOSE=1035,28,23` -- route 11 mask 4, a rotate Z at or
  past 16 radians refused by the exact angle-to-index path. Source has no such
  bound and rotation is periodic, so the fix is a range reduction.
- Pikachu 123 (MBallRays), Samus 70 (Charge Shot, not the bomb).

Three corrections worth carrying. `material 0` means `dobj->mobj == NULL` and
**not** untextured. The vertex-alpha promotion is load-bearing for 300
triangles; only 17 were defective. And two source palettes CAN share one
resident CI4 image -- the earlier refutation compared resolved texel values,
which must differ, instead of the packed index images, which do not.

## Preserved work and operating rules

Broad unrelated dirty work (1P, tags, pipes, assets, user P3/P4 docs) must be
preserved; do not resume campaign or redo CSS repairs.
**Launch codex as `-m "chatgpt-web/extra-high" -c model_reasoning_effort="xhigh"`,
and pipe the prompt on stdin** — a long prompt as an argument exceeds the command
line. GLM works now (`mode: subagent` silently ran the default agent); **one GLM
at a time**, up to five Muse beside it, no prompt word may start with `-`, and
**stagger launches by about eight seconds** or the losers die with
`database is locked` and a 0-byte log. When a log is empty, read its stderr
first: it separates all five failure modes in one line.
Launch writers only between builds, never let one run `make`, one build at a time,
no -j/MAKEFLAGS. CodeGraph first; a restart reads this file and the board.
Start each cycle with verify-all.ps1 -Profile Boundary -List and git status
--short. Boundary's `p2_battle_realtime` and `p2_fourcpu_stress` arms are still
unmeasured, and the published `smash64ds.nds` is stale.
