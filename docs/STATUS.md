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
path now uses them as the natural fighter-combat gate. They keep the
Pupupu/Dream Land Mario/Fox battle root stable, create Mario/Fox through the
original manager, and drive Wait -> Walk -> Dash -> Run -> RunBrake -> Turn,
Fox Attack11, live hitbox search, Mario damage/recover, and GuardOn/Guard/
GuardOff through imported `ftanim.c`/`ftkey.c`, original status descriptors,
`ftmain.c`, and `gmcollision.c`.

The current public summary is:

```text
ftmanager natural-combat: wait=357/380, walk=8/8,
dash=13/11, run=8/10, attack=22, hitbox=7,
damage=0->4 status=40, guard=3/10/11, updates=471, mask=0xfffff,
hwsubmit=252, hwtri=1152, hwftr=2/582
```

Modes `39/40`, `53/54`, and `161/162` now share this restored natural-combat
coverage. Older gcDrawAll/stage/MP marker stacks and the selected Fox Jab2
synthetic live-hit family remain documented as follow-up work in
`docs/KNOWN_ISSUES.md`.

## Latest Proof

Runtime slice 1 landed full BattleShip `gm/gmcollision.c`, replacing the local
matrix/world-position collision helpers. The shared `FTStruct` source region in
`include/ft/fighter.h` matches BattleShip `fttypes.h` through `display_mode`
(`joints=2280`, callbacks at `2516+`, source region `2896`), with DS/proof
fields moved to the tail extension and compile-time guards freezing layout.

Full BattleShip `ft/ftmain.c` is imported by default; duplicate local
`ftMain*` seams are gone or call the original once. The ladder, boundary,
continuous live-hit verifier, and four-way sharded Regression passed after a
fresh Regression prebuild; details are in `docs/FTSTRUCT_PARITY.md` and
`docs/KNOWN_ISSUES.md`.

Runtime slice 2 graduated the manager/status/animation path. Default builds now
import `ft/ftmanager.c`, the original common/Mario/Fox status descriptor
tables, and live `ftanim.c`/`ftanimend.c`/`ftkey.c`. Mario/Fox are created
through original manager descriptors and status-buffer payload loading. The
natural-combat proof now rebuilds movement, attack, live-hit, damage/recover,
and guard coverage on that runtime for modes `39/40`, `53/54`, and `161/162`.
The remaining stage compat-replay/cliffmotion seams in `ftMainSetStatus` are
scoped away from those proven statuses but still documented as follow-up for
older stage/cliff proofs.

Renderer hardware is now default for all-DL modes `33/34`, stage
draw/collision/floor-follow/floor-edge/MP process/update/sweep/cross/adjust/edge-floor modes `59-78`, and Boundary/Latest pair `161/162`;
global normal builds still default to software preview. Use `-SoftwarePreview`
on those wrappers for comparisons. The current Pupupu
stage-inclusive hardware gate proves matrix, material,
texture, depth/fog/alpha, primitive-Z, and texture-perspective submission with
zero hardware texture rejects. The direct/menu Mario/Fox all-DL hardware
defaults now pass on live manager-created fighters: all 14/18
selected DObjs are clean, hardware submits 284/298 fighter triangles, and the
texture path reports `bind119/upload8/ready119/reject0`. That proof carries
original fighter-part MObjs, the source-equivalent segment `0xE` material
register, RSP vertex/render state, and CI TLUT seeds from the current material
palette. The stage `gcDrawAll`/collision/floor-follow/floor-edge/MP process/update/sweep/cross/adjust/edge-floor hardware defaults now submit the
Pupupu stage and both selected manager-created fighters in one hardware frame
on direct and menu-chain routes: `hwsubmit=252`, `hwtri=1152`,
`hwftr=2/582`, and `bind582/upload66/ready582/reject0`. The active natural-
combat boundary wrappers assert that stage + both-fighter DS 3D replay after
the imported manager combat chain passes. Latest captures include
`artifacts\boundary-combat-hwtri.png`,
`artifacts\stage-floor-follow-hwtri.png`, `artifacts\stage-floor-edge-hwtri.png`, `artifacts\stage-mpprocess-floor-hwtri.png`, `artifacts\stage-mpupdate-floor-hwtri.png`, `artifacts\stage-mpsweep-floor-hwtri.png`, `artifacts\stage-mpcross-floor-hwtri.png`, `artifacts\stage-mpadjust-floor-hwtri.png`, `artifacts\stage-mpedge-floor-hwtri.png`, menu-chain all-DL HW, and
`artifacts\renderer-stage-gcdrawall-hw-fighters.png`.
Full visual fidelity still needs broader source-scene coverage and cutover work.

Latest gameplay proof is now original-manager Mario/Fox combat flow through
natural input: Wait -> movement chain, Fox Attack11, live hitbox search,
Mario damage/recover, and guard. The TaruCannon status `61` setup/physics tick
remains preserved as older regression coverage.

## Current Blocker

The active boundary is still bounded proof scaffolding, not continuous gameplay.
The next useful gameplay work is to migrate the remaining older gcDrawAll/stage/
MP marker families and selected Fox Jab2 modes `159/160` onto natural runtime,
then move toward `battle_playable` camera, HUD, match-flow, and renderer cutover.

- renderer follow-up: broaden source-scene coverage and HW default coverage;
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
