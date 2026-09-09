# Handoff

Current: ACTIVE. Contract: docs/reviews/NATIVE_ONLY_IMPLEMENTATION_GOAL.md — every
new ROM, diagnostics and profiling included, must exclude reference renderers and
software scene compositors. Host reference tools are allowed.
**1P campaign remains paused.** Main Menu/VS/VS Options/Option/Backup Clear accepted;
owner symptoms are in docs/BUGS.md and the evidence in docs/p2/BUG_NOTES.md.

## Current checkpoint

Pushed through ded7923f394. **Eight of nine stages and seven of nine fighters
are clean.** Landed since the last handoff: the fighter angle range reduction
(Yoshi 342 to 0) and four native owners — Link's Bomb (2,000 to 144), the
Saffron Marumine (59 to 0), the Poke Ball entry rays (Pikachu 123 to 23) and
the Mushroom Kingdom POW block.

## The owner standing goal: zero native failures, everywhere

`scripts/diagnostics/probe-native-render-batch.ps1` runs cases concurrently on up to
12 runner slots. **Measure on `builds/build-p2-shell/smash64ds-p2-shell-hwtri`** —
the root `smash64ds.nds` is a different configuration and every case times out
on it with `transport=failed`, which is a harness failure, not a measurement.
`-CasesFile` takes `scripts/diagnostics/native-fighter-long.json` (nine
fighters) or `native-stage-all.json` (nine stages); both pin 1,200 presents in
the case file, and each wave takes about 240 s at `-MaxParallel 6`.

Measured 2026-09-09 08:05. **Clean**: Castle, Hyrule, Congo, Zebes, Sector Z,
Yoshi's Island, Dream Land, Saffron; Mario, Fox, Luigi, Donkey, Captain, Samus,
Yoshi. **Open**, per 1,200 presents, all three of them newly unmasked rather
than regressions:

- Link 144, Effect asset 85 root 0x2ef0, **material non-NULL** — the first open
  row with a live MObj, so it needs the Pakkun shape, not the Bomb shape.
- Pikachu 23, Weapon asset 342 root 0x270, material 0.
- Mushroom Kingdom 105, Item asset 155 root 0x10d0 — the POW block; its owner
  is landed but was not yet in the measured ROM.

`DIAG_NATIVE` prints count, domain, scene, identity, status, root, material,
reason. Identity is `(GObj kind << 16) | asset_id`; kind 0x3f2 Ground, 0x3f3
Effect, 0x3f4 Weapon, 0x3f5 Item. **The record latches identity on the FIRST
failure and counts every one**, so closing a row reveals the next — a non-zero
count afterwards is not a failed fix until you check the identity.

Corrections worth carrying. `material 0` means `dobj->mobj == NULL` and **not**
untextured. Two source palettes CAN share one resident CI4 image. And
`sNdsRendererAdapterItemSubmitHead` is written `0u` twice and never advanced, so
it reads 0 for every list of every item: never gate an owner on it.

## Preserved work and operating rules

Broad unrelated dirty work (1P, tags, pipes, assets, user P3/P4 docs) must be
preserved; do not resume campaign or redo CSS repairs.
**Codex: `-m "chatgpt-web/extra-high" -c model_reasoning_effort="xhigh"`, prompt on
stdin.** Its stdout stays 0 bytes until it finishes — read stderr for progress.
opencode: one GLM slot per agent file, up to five Muse, **stagger 8 s**, and
**never reuse an agent name** — a zombie holds the old log open and the relaunch
cannot even create it, which stderr says in one line. Launchers are in the
session scratchpad.
Launch writers only between builds, never let one run `make`, one build at a time,
no -j/MAKEFLAGS. CodeGraph first; a restart reads this file and the board.
Start each cycle with verify-all.ps1 -Profile Boundary -List and git status
--short. Boundary's `p2_battle_realtime` and `p2_fourcpu_stress` arms are still
unmeasured, and the published `smash64ds.nds` is stale.
