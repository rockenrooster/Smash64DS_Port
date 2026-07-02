# Handoff

Use this file for the active handoff only. Keep it under 150 lines. The harness
registry decides the current Boundary/Latest profile; this file summarizes what
to do next.

## Start Here

1. Check the registry view:

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
```

2. Current pair is expected to be:

```powershell
.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3
.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3
```

3. Read `docs/STATUS.md` for current truth, `docs/DIAGNOSTIC_REFERENCE.md` for
full marker strings, and `docs/PORTING.md` for history.

## Current Boundary

Modes `161/162` are the active legacy regression anchor. They prove selected
Fox Jab2 live-hit damage lifecycle plus selected damage-status follow-through
on the Pupupu Mario/Fox battle root. The current short marker summary is:

```text
status=17->52/45, hitlag=6->0, callbacks=1/6/1,
search=0xf, repeat=1/1, gate=0x3f, catchSearch=0xffffffff/s3
```

Latest verified source-backed detail: TaruCannon kind `3` now routes through
the source-ordered setup shell, installs status `61`, and runs one original
physics tick copying fighter root position from the barrel root. Continuous
TaruCannon update/shoot runtime still waits for Jungle barrel helpers and map
throw-hit data.

Latest renderer detail: `src/nds/nds_renderer.c` has opt-in DS 3D hardware
submission behind `NDS_RENDERER_HW_TRIANGLES=1`, now fed by source-shaped DObj
and camera matrix seeds from `src/port/reloc_backend_renderer_dl.c`. Stage 3b
adds billboard-kind `33-40` and recalc-kind `41-50` seed coverage, consistent
textured/untextured `v16` world scaling, stage DObj submission from the
existing Pupupu `gcDrawAll` traversal, and renderer support for emitted
BattleShip `gSPMvpRecalc` / `G_MW_MATRIX` display-list streams. It also seeds
fighter-parts matrix kind `0x4B`, BattleShip battle-camera matrix kind `0x4C`,
converts cached fighter-parts `Mtx44f` seeds with fixed-W semantics matching
`syMatrixF2LFixedW`, and composes selected DObj parent chains from root to child
before the camera modelview. The hardware texture path now records
`LOADTLUT` state and converts CI4/CI8 texels through TLUT palettes in addition
to RGBA16/I16 uploads. Captures:
`artifacts\renderer-chain-hw-battle.png` and
`artifacts\renderer-stage-gcdrawall-hw.png`. The stage-inclusive capture shows
the Pupupu platform plus fighter geometry using the captured BattleShip camera
GObj and `0x4C` matrix seed; the all-DL battle capture restores the same battle
camera around its direct display calls, so it no longer depends on the bounded
hardware fallback camera. It is still flat-shaded because these proof paths
submit raw DObj DLs without preceding material DL emission.
Hardware all-DL stats on the opt-in build were
`HW=316/314 oracle=316/314 rejects=18/8 seeds=14/18` and
`MW=0/0/0/0`, so this direct all-DL scene is using per-DObj matrix seeds rather
than emitted matrix-word streams. The next renderer pass should route material
display lists into opt-in hardware draw, prove textured CI/TLUT output, then
finish combiner/material/depth state and renderer cutover. Default builds still
use the software preview.

Latest runtime detail: `gm/gmcollision.c` is now imported as a whole BattleShip
TU via `src/import/battleship_gmcollision.c`, replacing the local
matrix/world-position helper copies. A full `ft/ftmain.c` wrapper was tested
but rejected before landing because the current narrow item/weapon/effect,
audio, and ground headers cannot expose that TU without a broader
compatibility-header split.

## Process Change

Do not continue one-bit proof-mask increments. Future gameplay slices should
import one whole BattleShip TU, or a coherent adjacent group of TUs, prove it at
the boundary, then graduate it to live runtime by removing the guarded seam.

New harness modes are exceptional. Fold new proofs into the current pair unless
the work reaches a scene-level boundary such as `battle_playable` or
`results_screen`.

## Recommended Next Work

1. Renderer follow-up: route material display lists into opt-in hardware draw,
   then prove textured CI/TLUT output, combiner/material/depth state, and
   renderer cutover.
2. Runtime slice 1 follow-up: split/expand the item, weapon, effect, audio, and
   ground compatibility headers enough for full `ft/ftmain.c`, then replace
   the remaining local `ftMain*` seams and add the continuous-runtime verifier.
3. Broaden hardware texture/combiner support after material DL emission is
   wired into the hardware path.

## Verification

For docs-only edits:

```powershell
.\scripts\check-docs.ps1
.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3
```

For mechanical split chunks:

```powershell
.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3
.\scripts\verify-boundary.ps1 -DelaySeconds 3
```

For shared runtime/common fighter/renderer/harness registry changes, add the
smallest broader check that matches the touched area:

```powershell
.\scripts\verify-current.ps1 -Build -DelaySeconds 3
.\scripts\verify-regression.ps1
.\scripts\check-harness-registry.ps1
.\scripts\check-gbi-decode-fixtures.ps1
```

After verified progress, inspect status, optionally commit, then run snapshot
as the last project command:

```powershell
.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean
```

## Avoid

- one-bit proof-mask increments;
- per-branch seed/restore proofs when the whole function can run naturally;
- permanent state restore around already-proven code;
- new harness modes for individual feature bits;
- broad gameplay rewrites outside original BattleShip source;
- broad compatibility headers to make imports easier;
- editing `decomp/`, generated outputs, emulator configs, logs, or runner slots.
