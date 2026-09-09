# Handoff

Current: ACTIVE. Contract: docs/reviews/NATIVE_ONLY_IMPLEMENTATION_GOAL.md — every
new ROM, diagnostics and profiling included, must exclude reference renderers and
software scene compositors. Host reference tools are allowed.
**1P campaign remains paused.** Main Menu/VS/VS Options/Option/Backup Clear accepted;
owner symptoms are in docs/BUGS.md and the evidence in docs/p2/BUG_NOTES.md.

## Current checkpoint

Landed 2026-09-09: the fighter angle range reduction, TEN native owners (the
last being the Thunder Jolt *effect*, asset 342 root 0x2170), and the Zebes acid
alpha subdivision. `smash64ds.nds` is rebuilt and its hash re-pinned.

## The owner standing goal: zero native failures, everywhere

`probe-native-render-batch.ps1` runs cases on up to 12 slots. **Measure on
`builds/build-p2-shell/smash64ds-p2-shell-hwtri`** — the root `smash64ds.nds`
is a different configuration and every case times out there with
`transport=failed`, a harness failure not a measurement. `-CasesFile` takes
`native-fighter-long.json` or `native-stage-all.json` under
`scripts/diagnostics/`; both pin 1,200 presents, ~240 s per wave.
**Never build while a wave runs**: make writes the same ROM path the probe
reads, and the cases split across two ROMs with nothing in `summary.json`
saying so — only the per-case `rom_sha256` reveals it.

Measured 2026-09-09 on shell ROM SHA `5203F631`, both waves, every case on that
one ROM: **nine of nine stages AND nine of nine fighters read zero. The goal is
met**, from ~2,629 that morning. Every owner is in BUG_NOTES with its measured
contract; a live-MObj row wants the Pakkun shape, not bake-everything.
**Zero native failures is not zero owner bugs.** Castle's roof and Yoster's
floor are emitted losslessly, pass every static gate, and record no failure --
so they die at draw, and BUG_NOTES names the runtime witnesses to read.

`DIAG_NATIVE` = count, domain, scene, identity, status, root, material, reason;
identity is `(GObj kind << 16) | asset_id` (0x3f2 Ground, 0x3f3 Effect, 0x3f4
Weapon, 0x3f5 Item). **It latches on the FIRST failure and counts every one**,
so closing a row reveals the next. `material 0` is `dobj->mobj == NULL`, NOT
untextured.
`sNdsRendererAdapterItemSubmitHead` is always `0u`: never gate an owner on it.
**`DIAG_OWNERTRI` is indexed by NDSRendererProfileOwner**, not by stage
`owner_spec`; a zero slot is usually an absent fighter. A `DOBJ_FLAG_HIDDEN`
node is skipped with its whole subtree and records nothing.

## Preserved work and operating rules

Preserve broad unrelated dirty work (1P, tags, pipes, assets, P3/P4 docs).
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
