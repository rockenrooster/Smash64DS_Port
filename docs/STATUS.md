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

Modes `161/162` are the legacy live-hit regression anchor. They keep the
Pupupu/Dream Land Mario/Fox battle root stable and prove a bounded selected Fox
Jab2 live-hit lifecycle through damage-status follow-through. They inherit the
current MP, cliff/ledge, passive/recover, wall/rebound, catch/throw, shield,
damage setup/recover, and TaruCannon hazard setup/physics proofs.

The current public summary is:

```text
status=17->52/45, hitlag=6->0, callbacks=1/6/1,
search=0xf, repeat=1/1, gate=0x3f, catchSearch=0xffffffff/s3
```

Full diagnostic marker strings live in `docs/DIAGNOSTIC_REFERENCE.md`, not here.

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

Renderer hardware work remains opt-in behind `NDS_RENDERER_HW_TRIANGLES=1`.
The hardware path has BattleShip billboard/recalc matrix seeds, Pupupu
stage-inclusive submission, source-shaped material branch packets, CI/IA/I/RGBA
texture upload coverage, recorded material color/alpha, source culling, reset
geometry/filter seeds, sm64-nds-style no-z / decal-depth submission,
F3DEX `G_FOG`/`G_MW_FOG`/`G_SETFOGCOLOR` -> DS fog state, and
`G_SETBLENDCOLOR` alpha / `G_AC_THRESHOLD` -> DS alpha-test threshold state.
Current captures are `artifacts\renderer-stage-gcdrawall-hw-blend-alpha.png`,
`artifacts\renderer-stage-gcdrawall-hw-fogstate.png`, and
`artifacts\battle-mariofox-dl-draw-all-hwtri.png`; the opt-in all-DL verifier
proves `hwdepth=z316/314/proj0/0/decal0/0` and
`hwtex=bind16/upload1/ready16/reject0/fmt0x4/max8x8`, and the opt-in Pupupu
stage gcDrawAll verifier proves a post-proof hardware replay, actual hardware
triangles, and frame flush (`hwsubmit=252`, `hwtri=1152`, `hwflush=1/1`).
The stage hardware replay now seeds the source display-wrapper depth mode and
proves `hwdepth=z456/proj696/decal0` plus source-scene texture activity
`hwtex=bind582/upload66/ready582/reject0/fmt4/max32x32`. Full visual fidelity
still needs remaining combiner/material behavior, broader texture source-scene
coverage, and renderer cutover work. Default builds still use the software
preview.

Latest gameplay proof remains the TaruCannon status `61` setup/physics tick.

## Current Blocker

The active boundary is still bounded proof scaffolding, not continuous gameplay.
The next useful work is not another proof bit. It is one of:

- runtime follow-up: remove the remaining `ftMainSetStatus` compat-replay and
  cliffmotion restore duplicate-behavior seams status-by-status as source
  proofs graduate;
- renderer follow-up: finish opt-in hardware combiner/material policy,
  broaden remaining texture-state and no-z/decal source-scene coverage after
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

Docs-only changes also run:

```powershell
.\scripts\check-docs.ps1
```

Harness registry/script changes also run:

```powershell
.\scripts\check-harness-registry.ps1
```

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
