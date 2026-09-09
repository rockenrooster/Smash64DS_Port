# Handoff

Current: ACTIVE. Contract: docs/reviews/NATIVE_ONLY_IMPLEMENTATION_GOAL.md — every
new ROM, diagnostics and profiling included, must exclude reference renderers and
software scene compositors. Host reference tools are allowed.
**1P campaign remains paused.** Main Menu/VS/VS Options/Option/Backup Clear accepted;
owner symptoms are in docs/BUGS.md and the evidence in docs/p2/BUG_NOTES.md.

## Current checkpoint

Pushed through 9eb17e2f3fb. **All nine stages and seven of nine fighters read
zero native failures.** Landed today: the fighter angle range reduction (Yoshi
342 to 0) and five native owners — Link's Bomb (2,000 to 144), Saffron's
Marumine (59 to 0), the Poke Ball entry rays (Pikachu 123 to 23) and the
Mushroom Kingdom POW block (105 to 0). `smash64ds.nds` republished and re-pinned.

## The owner standing goal: zero native failures, everywhere

`probe-native-render-batch.ps1` runs cases on up to 12 slots. **Measure on
`builds/build-p2-shell/smash64ds-p2-shell-hwtri`** — the root `smash64ds.nds` is
a different configuration and every case times out on it with
`transport=failed`, a harness failure not a measurement. `-CasesFile` takes
`scripts/diagnostics/native-fighter-long.json` or `native-stage-all.json`; both
pin 1,200 presents, ~240 s per wave at `-MaxParallel 6`.

Measured 2026-09-09. All nine stages: `native failures=0`. Two fighter rows
remain, and **both carry a non-NULL material**, so both want the live-MObj
Pakkun shape, not the Bomb shape: Link 144 at Effect asset 85 root 0x2ef0, and
Pikachu 22 at Weapon asset 342 root 0x1660. Expect a row per ROOT, not per
file: each file holds siblings and closing one reveals the next.

**Zero native failures does NOT mean a stage is right**: Saffron's gate draws 0
triangles AND rejects 0, so the instrument cannot see it. See BUGS.md.

`DIAG_NATIVE` = count, domain, scene, identity, status, root, material, reason.
Identity is `(GObj kind << 16) | asset_id`; 0x3f2 Ground, 0x3f3 Effect, 0x3f4
Weapon, 0x3f5 Item. **It latches identity on the FIRST failure and counts every
one**, so closing a row reveals the next: a non-zero count is not a failed fix
until you read the identity.

Corrections. `material 0` means `dobj->mobj == NULL`, **not** untextured. Two
source palettes CAN share one resident CI4 image. `sNdsRendererAdapterItemSubmitHead`
is written `0u` twice and never advanced, so it reads 0 for every list of every
item: never gate an owner on it.

## Preserved work and operating rules

Broad unrelated dirty work (1P, tags, pipes, assets, user P3/P4 docs) must be
preserved; do not resume campaign or redo CSS repairs.
**Codex: `-m "chatgpt-web/extra-high" -c model_reasoning_effort="xhigh"`, prompt on
stdin**; its stdout stays 0 bytes until it finishes, so read stderr for progress.
opencode: **stagger 8 s** and **never reuse an agent name** — a zombie holds the
old log and the relaunch cannot create it, which stderr says in one line.
**`check-docs.ps1` caps this file at 60 lines, not the 200 AGENTS.md states.**
Launch writers only between builds, never let one run `make`, one build at a
time, no -j/MAKEFLAGS. CodeGraph first; a restart reads this file and the board,
others lookup-only. Bank verbose output, bounded UTF-8 log reads; start each
cycle with verify-all.ps1 -Profile Boundary -List and git status --short.
Boundary's `p2_battle_realtime` and `p2_fourcpu_stress` arms remain unmeasured;
`check-docs.ps1` runs first and fails the whole profile before any arm does.
