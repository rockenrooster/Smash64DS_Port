# Task 51 — Dream Land native stage: STG KILL (does not merge)

**Date:** 2026-07-23
**Branch:** `codex/task51-dreamland-native` (5 commits, E0–E1b complete + differ/STG measured)
**Outcome:** **KILL — STG budget not met.** The path is mechanically correct and
the differ passes for what draws, but the 16 non-rigid bindings Task 51 targets
do not draw in the battle_playable scene, so there is no STG cost to recover.
Recorded per the spec's kill criterion ("KILL: > 200,000 after ONE measured
attempt — record the number and STOP").

## The measurement (the gate)

Tick-HUD STG bucket, 64 samples, frame 438, profile-0 tickhud target,
NDS_TASK51_STAGE_NATIVE 0 vs 1, identical window:

| side | STG P50 | STG P95 | STG max | ROM (tickhud) |
|---|---|---|---|---|
| A: native OFF | **569,216** | 574,208 | 584,512 | `B07E384F…` |
| B: native ON  | **587,968** | 595,008 | 597,504 | `24B7A6E9…` |

- **Target:** STG P50 ≤ 120,000. **Kill line:** > 200,000.
- **Result:** 587,968 ≫ 200,000. **KILL.** The native path is ~18,752 ticks
  *worse*, not better — it adds the baked-matrix table + wrapper bookkeeping
  without converting any binding that actually draws.

## Root cause (why there is no win to recover)

The Task 49 differ captured the STAGE owner GX stream over frames 438–445,
600–607, and 1200–1207 (native on). In every window:

- **Only 8 bindings draw** (indices 0–7, all `layer0`, all rigid per
  `NDS_RENDERER_TASK36_RIGID_BINDING_MASK`).
- **Class 22 (MATRIX_MULT4x3) = 0** — the Task 51 path never fired. The 8
  drawing bindings are rigid, so the `if (!IsRigid)` Task 51 branch skips them;
  they take the existing `LoadNoZMatrix` (LOAD4X4) path in both A and B.
- **The 16 non-rigid bindings Task 51 targets (20–29, 33–38 — `map0–3`, `layer1`)
  do not submit any GX commands in the battle_playable scene.** They are
  conditionally-excluded stage elements (platform/map parts that this scene
  state does not draw). Their world matrices are constant (Task 48 proved it),
  but constant-and-undrawn costs zero either way.

The campaign's STG 597,632 baseline is dominated by the 8 rigid `layer0`
bindings drawing through `LoadNoZMatrix` — and Task 36 (`NDS_TASK36_HW_COMPOSE==2`,
already shipping) is the path that owns the rigid subset. Task 51's
generalization to the non-rigid subset reaches bindings that don't draw.

## What IS proven (the path is correct, just not exercised)

- **Differ: Tier 1 = 0 divergences, Tier 2 = 0.0 screen-px (ZERO_DEVIATION).**
  oracle (native off) vs native on, STAGE owner, frames 438–445: 2213 entries /
  2860 words compared, all matched; 8 bindings, max screen-px 0.0. The
  host-side bake is bit-exact with the runtime for the bindings that draw.
- **Default-off byte-identity holds:** published ROM `1818AA77…` reproduced
  byte-for-byte from a fresh dir with all Task 51 code present (table + runtime
  compile out at `NDS_TASK51_STAGE_NATIVE=0`).
- **Override trap handled:** `NDS_TASK51_STAGE_NATIVE=1` resolves correctly in
  the differ, tickhud, and published targets (verified via `nds_build_config.h`).
- **Build is clean** at both default and flag-on.

## Build environment note

Git Bash's direct `make` hits the documented `/opt/devkitpro` recursive sub-make
path quirk. **Build through the devkitPro msys2 bash instead:**
`C:/devkitPro/msys2/usr/bin/bash.exe -lc 'cd repo && make TARGET=... BUILD=... -j16'`.
This is the working invocation that reproduces `1818AA77`.

## Disposition

STOP per the kill criterion. The branch is the checkpoint: 5 commits (spec +
matrix-math foundation + E0 generator bake + E1a/E1c flag+enum + E1b runtime).
Nothing merges — the published ROM stays `1818AA77…` (default-off,
`NDS_TASK51_STAGE_NATIVE` is not in any target block). The differ result
(ZERO_DEVIATION) and the matrix-math foundation are retained on the branch as
recoverable work should a later task find a scene state where the non-rigid
bindings actually draw.

The decisive question for any revisit: **find a scene/match state where
bindings 20–29 / 33–38 submit GX, or confirm they are structurally undrawn in
battle_playable.** If the latter, the non-rigid subset is not a real STG cost
and Task 51's premise (that it captures a real prize) does not hold for this
scene.
