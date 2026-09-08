# Handoff

Current: ACTIVE. Contract: docs/reviews/NATIVE_ONLY_IMPLEMENTATION_GOAL.md — every
new ROM, diagnostics and profiling included, must exclude reference renderers and
software scene compositors. Host reference tools are allowed.
**1P campaign remains paused.** Main Menu/VS/VS Options/Option/Backup Clear accepted;
owner symptoms are in docs/BUGS.md and the evidence in docs/p2/BUG_NOTES.md.

## Current checkpoint

Pushed through 01b88a964b7. **`p2_shell_loop` passes** — zero native failures,
35,604 B free floor. Landed today: fast-logic battle draws are bracketed in the
SObj preview frame; the lower HUD no longer rides the text-console flag; a
`pad00 == 0` clause stopped declining three hit-spark materials; particle env
colour reaches the KO pillar; the item manager has its own particle bank instead
of aliasing another effect's; and the Dream Land pin set no longer aborts when a
fighter it names is absent.

## The owner's standing goal: zero native failures, everywhere

`scripts/diagnostics/probe-native-render-batch.ps1` runs cases concurrently on up to
12 runner slots, from `native-stage-battle.json` (300 presents),
`native-stage-long.json` (1,200), `native-fighter-entry.json` (480) and
`native-fighter-long.json` (1,200). **Measure at 1,200** — three stages that pass at
300 fail at match length. Freeze the ROM and ELF into
`builds/resume-20260908/frozen/` for a wave taken beside a build.

A fighter case pokes `gNdsMenuShellCssWalkTargetKind`/`...Kind2` for a mirror match
and asserts the kind committed, zero failures and non-zero owner triangles.

At 1,200 presents, **clean**: Dream Land, Zebes, Yoshi's Island; Mario, Fox, Luigi,
Donkey. **Failing**, by GObj kind (domain 2 is the recorder's, not the object's):

- Ground: Saffron's gate, asset 160 root 0x420, 1,306 in 300 presents.
- Item: Castle's bumper, asset 86 root 0x7558; Mushroom Kingdom asset 155 root
  0xb40, whose scale platforms now draw while this does not.
- Weapon: Sector Z asset 153 root 0x1c50; Samus asset 321 root 0x270.
- Effect: Pikachu asset 85 root 0x440; Captain, Hyrule and Congo with asset
  0xffff and a RAM root, meaning a display list built at runtime.
- Fighter program REJECTED at validate: Yoshi, and Link at code 4, slot 6,
  root 5, observed 0x2828 against expected 0x2630.

Saffron's gate is decoded and patched in BUG_NOTES, including its one new
capability: `DOBJ_FLAG_NOTEXTURE` must become a per-frame hidden mask rather than
grounds to reject the topology. Castle's bumper is blocked on its two source
palettes not sharing packed texels.

## Preserved work and operating rules

Broad unrelated dirty work (1P, tags, pipes, Pakkun, assets, user P3/P4 docs) must
be preserved; do not resume campaign or redo CSS repairs.
**Launch codex as `-m "chatgpt-web/extra-high" -c model_reasoning_effort="xhigh"`;**
the config default gpt-5.6-luna is a spent budget. GLM truncated every run today.
Muse (`swarm-*`, up to 5) and Claude subagents work; launch writers only between
builds, never let one run `make`, one build at a time, no -j/MAKEFLAGS. Never start
a build while a write agent is live — that cost a build today.
CodeGraph first; a restart reads this file and the board, others lookup-only.
Bank verbose output, bounded UTF-8 log reads; start each cycle with verify-all.ps1
-Profile Boundary -List and git status --short. Boundary's `p2_battle_realtime`
and `p2_fourcpu_stress` arms are unmeasured since.
