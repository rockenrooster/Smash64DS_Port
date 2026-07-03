# Current Status

This is the short current-truth document for active development. Keep it under
150 lines. Use `docs/DIAGNOSTIC_REFERENCE.md` for full marker strings and
`docs/PORTING.md` for append-only increment history.

## Direction

Target remains a full 1:1 playable Nintendo DS source port of BattleShip Smash
64. The process is changing because one-bit proof-mask increments are too
expensive for that goal. Future gameplay work should move by whole original
translation units, or coherent adjacent TU groups, then graduate proven code to
live runtime.

Keep `decomp/` read-only. Do not hand-author gameplay when BattleShip source can
be ported.

## Current Boundary

The registry decides the active Boundary/Latest set:

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
```

Current pair:

```powershell
.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3
.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3
```

Modes `161/162` are still the Boundary/Latest pair, but the default-manager
path now uses them as the natural fighter-motion gate. They keep the
Pupupu/Dream Land Mario/Fox battle root stable, create Mario/Fox through the
original manager, and prove 300+ Wait frames plus one input-driven Wait -> Walk
transition through imported `ftanim.c`/`ftkey.c` runtime.

The current public summary is:

```text
ftmanager natural-motion: wait=300/300, anim=299/299,
walk=8/8, updates=308, mask=0x3ff
```

The older gcRunAll, gcDrawAll/stage/MP, dash, and selected Fox Jab2
live-hit/status marker stacks are now documented as coverage-reduced follow-up work in
`docs/KNOWN_ISSUES.md`.

## Latest Proof

Runtime slice 1 landed the full BattleShip `gm/gmcollision.c` translation unit
through `src/import/battleship_gmcollision.c`. The old project-owned
`gmCollisionGetFighterPartsWorldPosition`, `func_ovl2_800EDA0C`, and
`gmCollisionGetWorldPosition` copies were removed, so matrix/world-position
collision helpers now resolve to the original source. The shared `FTStruct`
source region in `include/ft/fighter.h` now matches BattleShip `fttypes.h`
through `display_mode` (`joints=2280`, callbacks at `2516+`,
source-region size `2896`), with DS/proof-only fields moved to the extension
block at offset `2896` and compile-time guards freezing the layout.

Full BattleShip `ft/ftmain.c` is now imported by default through
`src/import/battleship_ftmain.c`; the duplicate local `ftMain*` seams are gone
or routed through the imported original once. The init/wait/dash-run ladder,
boundary profile, continuous live-hit verifier, and four-way sharded Regression
passed after a fresh Regression prebuild, and the four Regression shards were
rerun green on current `master` after the renderer follow-ups. The only
regression-cycle fix was a proof-recorder correction that preserves the first
selected cross-floor target match so later wall/cliff MP updates cannot erase
the motion-stale evidence. Details are in `docs/FTSTRUCT_PARITY.md` and
`docs/KNOWN_ISSUES.md`.

Runtime slice 2 graduated the manager/status/animation path. Default builds now
import `ft/ftmanager.c`, the original common/Mario/Fox status descriptor
tables, and live `ftanim.c`/`ftanimend.c`/`ftkey.c`. Mario/Fox are created
through original manager descriptors and status-buffer payload loading, and the
natural-motion proof advances Wait animation for 300+ frames with valid joints
before driving Wait -> Walk through the same imported runtime. The refreshed
Regression prebuild plus all four sharded `-NoBuild` Regression runs passed on
this default path. The remaining stage compat-replay/cliffmotion seams in
`ftMainSetStatus` are deliberately kept as documented follow-up because
removing them still needs status-by-status proofs.

Renderer hardware work remains opt-in behind `NDS_RENDERER_HW_TRIANGLES=1`.
It covers the current matrix, material, texture, depth/fog/alpha, primitive-Z,
and texture-perspective proofs for all-DL and Pupupu stage hardware scenes.
Latest captures remain in `artifacts\renderer-*-hw*.png`. Full visual fidelity
still needs remaining combiner/material behavior, broader texture and
primitive-depth source-scene coverage, and renderer cutover work. Default builds
still use the software preview.

Latest gameplay proof is now original-manager Mario/Fox creation plus imported
animation/key runtime for Wait -> Walk. The TaruCannon status `61`
setup/physics tick remains preserved as older regression coverage.

## Current Blocker

The active boundary is still bounded proof scaffolding, not continuous gameplay.
The next useful gameplay work is to rebuild the reduced gcDrawAll/stage/MP,
dash, and live-hit proof coverage on top of the natural original-manager runtime, then move toward
`battle_playable` camera, HUD, match-flow, and renderer cutover work.

- renderer follow-up: finish opt-in hardware combiner/material policy,
  broaden remaining texture-state and no-z/decal/prim-depth source-scene
  coverage after
  the first all-DL CI/TLUT gate plus IA/I decoders, then plan renderer cutover
  work;
- continuous-runtime verifier for unbounded battle frames.

## Runtime Target

Next major gameplay milestone is `battle_playable`: continuous unbounded
VSBattle Mario vs Fox on Pupupu/Dream Land with live input, `gcRunAll` and
`gcDrawAll` every frame, no state restore around proven code, and the DS
hardware renderer path active enough for the scene.

## Verification

For normal 30-60 minute work:

```powershell
.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3
```

Docs-only changes also run `.\scripts\check-docs.ps1`; harness registry/script
changes also run `.\scripts\check-harness-registry.ps1`.

Runtime/subsystem changes that touch shared architecture should graduate to:

```powershell
.\scripts\verify-boundary.ps1 -DelaySeconds 3
```

Run `verify-current` or `verify-regression` only for shared runtime behavior,
common fighter code, scene-manager flow, allocator/linker behavior, harness
registry behavior, or broad renderer changes.

After verified progress, inspect status, optionally commit, then run the Lean
snapshot as the final project command:

```powershell
.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean
```
