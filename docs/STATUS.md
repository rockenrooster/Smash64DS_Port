# Current Status

This is the short current-truth document for active development. Keep it under 150 lines.
Use `docs/DIAGNOSTIC_REFERENCE.md` for full marker strings; append history in `docs/PORTING.md`.

## Direction

Target remains a full 1:1 playable Nintendo DS source port of BattleShip Smash
64. Gameplay work now moves by runtime-first subsystem slices aimed at
scene-level capability: import coherent original TU groups, prove with the
continuous natural-runtime verifier plus captures, then graduate live.

Keep `decomp/` read-only. Do not hand-author gameplay when BattleShip source can
be ported.

## Current Boundary

The registry decides the active Boundary/Latest set:

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
```

Current Boundary/Latest entries:

```powershell
.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3
.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3
.\scripts\verify-battle-playable-harness.ps1 -DelaySeconds 3
```

Modes `161/162` remain the bounded natural-combat pair. They keep the
Pupupu/Dream Land Mario/Fox battle root stable, create Mario/Fox through the
original manager, and drive Wait -> Walk -> Dash -> Run -> RunBrake -> Turn,
Fox Attack11, live hitbox search, Mario damage/recover, and GuardOn/Guard/
GuardOff through imported `ftanim.c`/`ftkey.c`, original status descriptors,
`ftmain.c`, and `gmcollision.c`.

Mode `163` is the scene-level `battle_playable` anchor. It runs Pupupu Mario/Fox
stock battle with imported camera/Dead/Rebirth live by default, then proves
natural attack/damage, KO/rebirth, normal moves, Mario fireball, Fox blaster,
guard, reflector, grab/throw, Mario/Fox specials, audio asset parsing, one-track
Pupupu BGM playback, and a DS 3D hardware stage + fighter frame.

## Latest Proof

Runtime slice 1 landed full BattleShip `gm/gmcollision.c`, replacing the local
matrix/world-position collision helpers. The shared `FTStruct` source region in
`include/ft/fighter.h` matches BattleShip `fttypes.h` through `display_mode`
(`joints=2280`, callbacks at `2516+`, source region `2896`), with DS/proof
fields moved to the tail extension and compile-time guards freezing layout.

Full BattleShip `ft/ftmain.c` is imported by default; duplicate local
`ftMain*` seams are gone or call the original once. Current layout and coverage
notes are in `docs/FTSTRUCT_PARITY.md` and `docs/KNOWN_ISSUES.md`.

Runtime slice 2 graduated the manager/status/animation path. Default builds now
import `ft/ftmanager.c`, the original common/Mario/Fox status descriptor
tables, and live `ftanim.c`/`ftanimend.c`/`ftkey.c`. Mario/Fox are created
through original manager descriptors and status-buffer payload loading. The
natural-combat proof now rebuilds movement, attack, live-hit, damage/recover,
and guard coverage on that runtime for modes `39/40`, `53/54`, and `161/162`.
The old cliffmotion restore hook is deleted after the direct and menu-chain
cliff-family Regression modes stayed green. The remaining stage compat-replay
seam in `ftMainSetStatus` is still documented as follow-up.

`battle_playable` graduated to default for `gm/gmcamera.c`,
`ftcommondead.c`, `ftcommonrebirth.c`, battle-critical `if/ifcommon.c` HUD,
original `if/ifscreenflash.c`, normal moveset TUs, the weapon manager, Mario
fireball, Fox blaster, the original effect manager, Fox reflector, Mario Super
Jump Punch, Mario Tornado, Fox Fire Fox, original audio asset parsing, and
one-track Pupupu BGM playback. The mode-163 proof reports
`stock8->3`, `falls0->5`, `moveset=0x7ff phase=15`, `tilt=23/17/17`,
`smash=13`, `aerial=19`, `landing=26`, `grab=3/1`, `throw=12/5/175`,
`throwDmg=0->12`, `hud=dmg16/digits0x1060a stock9->4`,
`projectile=... dmg=13`, `reflector=0xff proc=1 vx=49809->-49809`, and
`specials=0xfff phase=7` (`mhi=1/31/0/72/105`, `mlw=1/43/0 dust=1 wait=165`,
`foxhi=1/3/17/20/10/0/61`), `audio=seq47 bank1=1/42/117@32000
bank2=1/1/322@44100 fgm=100/464/695 raw=4422960 resident=0 scratch=64416`,
`bgm=track0 play=1 stop=1 chunks=34 read=2228224 resident=65536`, plus
`hwsubmit=42`, `hwtri=192`, `hwftr=2/582`. FGM/voice playback, original
sequence-player import, and non-critical HUD/SObj/particle perimeter remain
follow-up.

The memory pre-breadth gate has a live VSBattle ledger and scene-owned reloc
cache eviction. Mode `163` reports headroom `237836`, resident reloc `770896`
bytes (`stage=202816`, `fighter=264704`, `if=208672`), stale `0/0`, and source
VSBattle buffers from `scvsbattle.c:31-41`. The BGM stream adds a separate
64 KiB resident buffer while still leaving `172300` bytes above the reserve.

Renderer hardware is now default for all-DL modes `33/34`, stage
draw/collision/floor-follow/floor-edge/MP process/update/sweep/cross/adjust/edge/wall/stale/live-stale/motion-stale/cliff-status/cliff-tick/fall-map/fall-landing/ceiling/ceiling-status/cliff-catch/cliff-wait/cliff-attack/cliff-attack-action/cliff-common2/cliff-escape-action/common2/cliff-climb floor/action/common2/finish/cliff-wait damage/MP Passive modes `59-124`, and Boundary/Latest pair `161/162`;
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
palette. The stage `gcDrawAll`/collision/floor-follow/floor-edge/MP process/update/sweep/cross/adjust/edge/wall/stale/live-stale/motion-stale/cliff-status/cliff-tick/fall-map/fall-landing/ceiling/ceiling-status/cliff-catch/cliff-wait/cliff-attack/cliff-attack-action/cliff-common2/cliff-escape-action/common2/cliff-climb floor/action/common2/finish/cliff-wait damage/MP Passive hardware defaults now use a
stage-side original-manager smoke proof (`mask=0x24f`) and submit the Pupupu
stage plus both selected fighters in one hardware frame on direct and menu-chain
routes: `hwsubmit=42`, `hwtri=192`, `hwftr=2/582`, and
`bind97/upload11/ready97/reject0`. The active natural-combat boundary wrappers
and mode `163` retain the fuller movement/live-hit/combat ownership. Latest
captures include
`artifacts\boundary-combat-hwtri.png`, the stage MP hardware captures through MP Passive,
menu-chain all-DL HW, and `artifacts\renderer-stage-gcdrawall-hw-fighters.png`.
Full visual fidelity still needs broader source-scene coverage and cutover work.

## Current Blocker

The active `161/162` boundary is still bounded proof scaffolding, while
`battle_playable` is the first scene-level unbounded stock/KO anchor.
Legacy bounded modes are migrate-or-delete: when a runtime slice obsoletes a
marker stack, delete its mode/verifier and leave one `[coverage-reduced]`
`KNOWN_ISSUES` line instead of reproducing old markers. Modes `57/58` and
`159/160` have been deleted instead of recreating old synthetic marker stacks.

- renderer follow-up: broaden source-scene coverage and HW default coverage;
- audio/interface follow-up: FGM/voice, original sequence-player playback, and
  the non-critical HUD/SObj and particle perimeter.

## Verification

For normal 30-60 minute work, run
`.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3`.

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
