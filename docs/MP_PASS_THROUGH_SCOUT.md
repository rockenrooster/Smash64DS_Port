# MP Pass-Through / Platform Scout

Status: pass-through was promoted to bounded direct/menu-chain runtime proof
modes `139/140`; the inactive platform existence scout was promoted in modes
`141/142`; the active yakumono DObj predicate proof was promoted in modes
`143/144`; the bounded platform status/update-tic proof was promoted in modes
`145/146`; and the natural drop-through input gate was promoted in modes
`147/148`; the bounded yakumono position/speed primitive was promoted in modes
`149/150`; the bounded platform speed-reader plus dynamic floor/ceil/wall
collision speed-consumer proof plus one bounded yakumono animation playback
tick and post-animation MP bounds recompute was promoted in modes `151/152`;
and the bounded Inishie/Mushroom Kingdom scale update proof was promoted in
modes `153/154`.

## Scope

This note records the pass-through/map-platform source scout after the Hyrule
wall-copy proof. Floor pass-through behavior is proven without importing moving
platform/yakumono runtime or broadening the live task loop. The platform scout
reaches original `mpCollisionCheckExistPlatformLineID`; modes `141/142`
preserve the no-active-DObj blocker and modes `143/144` prove the minimum
original-compatible active yakumono DObj predicate. Modes `145/146` prove the
same active DObj survives one guarded original-compatible update-tic step
without broadening into real moving-platform motion runtime. Modes `147/148`
then import original `ftcommonpass.c` / `ftcommonsquat.c` enough to prove
down-input Wait -> Squat -> Pass on the same pass-through floor contract.
Modes `149/150` consume that input proof and run source-order
`mpCollisionSetYakumonoPosID` on the same Dream Land yakumono shell. Modes
`151/152` read the resulting speed back through the original-compatible
`mpCollisionGetSpeedLineID` line API, then run one bounded
`mpCollisionCheckFloorLineCollisionDiff` probe through the source-order
active-yakumono coordinate transform. Modes `153/154` inherit that proof,
stage the Inishie map-header dependency, then run one bounded original
`grInishieScaleProcUpdate` tick through a narrow scale-platform proof shell.

The proven target is floor pass-through behavior, not full moving-platform
behavior:

- Prove original `MAP_PROC_TYPE_PASS` routing through a bounded direct/menu
  Mario/Fox harness.
- Prove `MAP_VERTEX_COLL_PASS` is preserved from decoded stage geometry into
  the source-order floor collision tests.
- Keep moving yakumono/platform update runtime deferred until the minimum
  original-compatible motion/update path is proven.

## Original Source Findings

Read-only BattleShip references:

- `decomp/BattleShip-main/decomp/src/mp/mpdef.h:18` defines
  `MAP_VERTEX_COLL_PASS`.
- `decomp/BattleShip-main/decomp/src/mp/mpdef.h:34` defines
  `MAP_PROC_TYPE_PASS`.
- `decomp/BattleShip-main/decomp/src/mp/mpcommon.c:472` begins
  `mpCommonRunFighterSpecialCollisions`.
- `decomp/BattleShip-main/decomp/src/mp/mpcommon.c:510` selects the pass
  branch when `flags & MAP_PROC_TYPE_PASS` and calls
  `mpProcessCheckTestFloorCollisionAdjNew(coll_data, sMPCommonProcPass,
  fighter_gobj)`.
- `decomp/BattleShip-main/decomp/src/mp/mpcommon.c:609` and `:619` expose
  `mpCommonCheckFighterPass` and `mpCommonCheckFighterPassCliff`.
- `decomp/BattleShip-main/decomp/src/mp/mpprocess.c:1995` begins
  `mpProcessCheckTestFloorCollisionAdjNew`.
- `decomp/BattleShip-main/decomp/src/mp/mpprocess.c:2029`, `:2045`, and
  `:2067` accept a pass-through floor when the hit line is not the current
  `ignore_line_id`, then require the optional pass callback to return true.
- `decomp/BattleShip-main/decomp/src/mp/mpcollision.c:4201` implements
  `mpCollisionCheckExistPlatformLineID`. Modes `141/142` reach this original
  predicate and preserve the inactive blocker: line `0` maps to yakumono id
  `1`, yakumono count `1`, with a bounded DObj slot present but status off.
  Modes `143/144` install a bounded original-compatible DObj for that id, set
  status on, and prove the predicate active.
- `decomp/BattleShip-main/decomp/src/mp/mpcollision.c` also exposes
  `mpCollisionSetYakumonoOnID` and `mpCollisionAdvanceUpdateTic`. Modes
  `145/146` use project-owned bounded compatibility wrappers for those
  contracts to set the selected yakumono on and advance the update tic
  `0 -> 1` while preserving active predicate state.
- `decomp/BattleShip-main/decomp/src/mp/mpcollision.c:4042` implements
  `mpCollisionSetYakumonoPosID`. Modes `149/150` use the same source-order
  semantics in the project-owned seam: compute `gMPCollisionSpeeds[id]` from
  target minus current DObj translation, then update the DObj translation.
- `decomp/BattleShip-main/decomp/src/mp/mpcollision.c:3125` implements
  `mpCollisionGetSpeedLineID`. Modes `151/152` prove the project-owned seam
  resolves line `0` to yakumono `1` and reads the current speed vector.
- `decomp/BattleShip-main/decomp/src/mp/mpcollision.c:958` begins the
  source-order `mpCollisionCheckFloorLineCollisionDiff` branch that consumes
  `gMPCollisionSpeeds[yakumono_id]` when the selected yakumono DObj is active.
  Modes `151/152` now prove one bounded Dream Land floor probe reaches that
  dynamic branch after the speed reader.
- `decomp/BattleShip-main/decomp/src/mp/mpcollision.c:1916` and `:2422`
  define the source-order left/right wall diff branches that use the same
  active-yakumono local-space transform and speed offset. Modes `151/152` now
  prove one bounded same-yakumono Dream Land wall probe reaches that surface.
- `decomp/BattleShip-main/decomp/src/mp/mpcollision.c:4223` implements
  `mpCollisionGetVertexFlagsLineID`, the narrow accessor needed to expose line
  flags without importing broad platform runtime.
- `decomp/BattleShip-main/decomp/src/gr/grdisplay.c` only attaches
  `mpCollisionPlayYakumonoAnim` to a display layer when the layer descriptor has
  authored DObj or material animation data. The current Pupupu layer-1 probe
  records `stageanim=0/0x91`, so the selected Dream Land platform shell is not
  a real authored layer-1 animation source.

## Runtime Proof

Current maintained harness pair:

```text
battle_mariofox_stage_mpplatform_floor_loop
menu_chain_mariofox_stage_mpplatform_floor_loop
battle_mariofox_stage_mpplatform_active_floor_loop
menu_chain_mariofox_stage_mpplatform_active_floor_loop
battle_mariofox_stage_mpplatform_tick_floor_loop
menu_chain_mariofox_stage_mpplatform_tick_floor_loop
battle_mariofox_stage_mppass_input_loop
menu_chain_mariofox_stage_mppass_input_loop
battle_mariofox_stage_mpplatform_pos_floor_loop
menu_chain_mariofox_stage_mpplatform_pos_floor_loop
battle_mariofox_stage_mpplatform_speed_floor_loop
menu_chain_mariofox_stage_mpplatform_speed_floor_loop
battle_mariofox_stage_inishie_scale_loop
menu_chain_mariofox_stage_inishie_scale_loop
```

Registry modes: `139/140` for pass-through, `141/142` for inactive platform
existence, `143/144` for active platform predicate proof, and `145/146` for
the bounded platform status/update-tic proof, `147/148` for the natural
drop-through input gate, and `149/150` for the yakumono position/speed
primitive, `151/152` for the platform speed-reader proof, and `153/154` for
the Inishie scale update proof.

What the proof records:

- The live battle scene stays on Pupupu/Dream Land.
- The proof consumes the maintained Hyrule wall-hit and wall-copy boundaries.
- Dream Land floor line `0` is selected as the pass-through floor candidate.
- The candidate preserves `MAP_VERTEX_COLL_PASS` as raw flags `0x4000`.
- `MAP_PROC_TYPE_PASS` routing is reached twice.
- Same-line `ignore_line_id` rejection is proven once.
- Different-line pass-through acceptance is proven once.
- The optional pass callback gate is called once and allows once:
  `cb=1/1/0`.
- Original `mpCollisionCheckExistPlatformLineID` is called for the selected
  line.
- The inactive platform scout records yakumono id/count `1/1`, DObj present
  with status off, predicate `0`, and blocker `0x40`.
- The active platform proof records the same line/id, DObj present, status
  on, predicate `1`, and blocker `0`.
- The status/update-tic proof records `mpCollisionSetYakumonoOnID` once, one
  guarded `mpCollisionAdvanceUpdateTic`, update tic `0 -> 1`, DObj status
  `1 -> 1`, predicate `1`, and unsafe count `0`.
- The pass-input proof records original-compatible stick Y `<= -53`, tap Y
  `0 -> 254`, Wait -> Squat -> Pass status `10 -> 28 -> 33`, ignored line `0`,
  pass wait `3 -> 0`, and unsafe count `0`.
- The position/speed proof records Dream Land line `0`, yakumono `1`, DObj
  status `1 -> 1`, platform predicate `1`, and
  `gMPCollisionSpeeds` delta `12000/-4000/2000`.
- The speed-reader proof records the same line/yakumono and reads
  `12000/-4000/2000` through `mpCollisionGetSpeedLineID`.
- The dynamic floor-collision probe records the same line/yakumono, one
  active-yakumono branch, one controlled probe, and two hit records
  (`dyn=1/2`) while preserving the same speed vector.
- The dynamic ceiling probe records one same-yakumono
  `mpCollisionCheckCeilLineCollisionDiff` probe and hit (`ceil=1/1`).
- The dynamic wall probe records one same-yakumono
  `mpCollisionCheckLWallLineCollisionDiff` / `RWall` probe and hit
  (`wall=1/1`).
- The bounded MP process wall probe routes one first-probe slice of original
  `mpProcessCheckTestL/RWallCollisionAdjNew` through the same wall-diff seam
  (`procwall=1/1`).
- The bounded animation probe seeds one controlled AObj track set, calls gated
  `mpCollisionPlayYakumonoAnim`, runs original `gcParseDObjAnimJoint` /
  `gcPlayDObjAnimJoint` helpers, advances the MP update tic once, and records
  the same `12000/-4000/2000` speed vector (`anim=1`).
- The bounds probe calls original-compatible `mpCollisionUpdateBoundsCurrent`
  and `mpCollisionUpdateBoundsDiff` after that animation tick, then records
  one recompute and nonzero current-vs-start diff (`bounds=1`).
- The stage-authored layer-animation diagnostic records `stageanim=0/0x91`.
  The Dream Land layer/root is present and the bounded callback ran once, but
  the selected line `0` / yakumono `1` path has no authored Pupupu layer-1
  animation table/process.
- The Inishie scale proof records line groups `1/2`, map object kinds `5/6`,
  altitude `80000 -> 72000`, platform Y `363000/362000 -> 435000/290000`,
  and speed `72000/-72000` after one bounded original
  `grInishieScaleProcUpdate` tick.
- P1 root state remains unchanged.

Verifier commands:

```powershell
.\scripts\verify-battle-mariofox-stage-mppass-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mppass-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpplatform-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpplatform-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpplatform-active-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpplatform-active-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpplatform-tick-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpplatform-tick-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mppass-input-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mppass-input-loop-harness.ps1
```

Current proof summary:

```text
mpPass=line=0 flags=0x4000 route=2 same=1 diff=1 cb=1/1/0
mpPlatform=line=0 yak=1 dobj=1 status=0 anim=0 deferred=0x40
mpPlatform=line=0 yak=1 dobj=1 status=1 anim=0 active
mpPlatformTick=tic=0->1 setOn=1 advance=1 status=1/1
mpPassInput=line=0 flags=0x4000 squat=28 pass=33 ignore=0
mpPlatformPos=line=0 yak=1 delta=12000/-4000/2000
mpPlatformSpeed=line=0 yak=1 read=12000/-4000 dyn=1/2 ceil=1/1 wall=1/1 procwall=1/1 anim=1 bounds=1 stageanim=0/0x91
inishieScale=lines=1/2 alt=80000->72000 y=363000/362000->435000/290000 speed=72000/-72000
```

## Implementation Notes

- `include/mp/map.h` now defines project-owned `MAP_VERTEX_COLL_PASS` and the
  narrow vertex-flags accessor declaration.
- `src/port/reloc_backend.c` uses bounded source-compatible helpers to scan the
  decoded floor geometry, route the selected fighter through an
  original-compatible pass-through floor collision gate, and apply the
  source-order active-yakumono local-space transform for floor, ceiling, and
  wall diff sweeps.
- The platform-speed `mpCollisionPlayYakumonoAnim` proof is gated to modes
  `151/152`.
  Outside that proof it preserves the prior deferred behavior. The bounded
  proof uses seeded linear tracks to exercise original object-animation
  playback, then calls original-compatible MP bounds current/diff recompute.
  The current Dream Land platform-layer diagnostic records `stageanim=0/0x91`,
  so real authored yakumono animation scripts and full moving-platform runtime
  remain deferred until another original stage/yakumono source path proves a
  narrow route.
- The current `mpProcessCheckTestL/RWallCollisionAdjNew` proof is a bounded
  first-probe slice. The original edge/ceil/floor follow-up branches remain
  deferred until a verifier needs them.
- The proof deliberately uses the current decoded Dream Land geometry for the
  pass-through candidate. Hyrule geometry remains isolated to the wall-hit and
  wall-copy proof roots.
- The shared GDB verifier now has an explicit `-RequireStageMPPassFloor`
  switch. This avoids PowerShell partial-parameter binding to
  `-RequireStageMPPassiveLoop`.

## Inishie Scale Result

Completed source scout:

- Inishie/Mushroom Kingdom scales were the next moving-yakumono boundary.
  `grInishieScaleProcUpdate` updates two platform DObj Y positions and calls
  `mpCollisionSetYakumonoPosID` for both scale line groups every update. It is
  a real moving-platform path and avoids Yamabuki's item/monster path and
  Sector's Arwing/weapon path.
- Modes `151/152` included the first dependency preflight for that route:
  `GRInishieMap` is staged, `llGRInishieMapMapHeader` resolves at `0x14`, and
  the verifier reports `inishieAsset=header/geometry nodes=1`.
- Modes `153/154` now create the bounded scale-platform/string/collision DObj
  proof shell and run one original `grInishieScaleProcUpdate` tick. The proof
  keeps the live battle roots on Pupupu/Dream Land; it does not start the full
  Mushroom Kingdom stage runtime.
- Yoster clouds are real yakumono motion, but their update depends on fighter
  stand checks, material animation, vapor particles, and cloud state timers.
  Useful later, not the shortest first moving-platform proof.
- Yamabuki gates are real yakumono motion, but the open path is tied to
  monster/item spawning and gate animation. Defer until item/HUD-adjacent stage
  runtime is less risky.
- Sector Arwing collision is real yakumono motion, but it is tied to Arwing
  animation, weapon/laser state, and an external Fox special model file. Defer
  until broad stage runtime is ready.

The next platform-related code step should be:

- replace more of the proof shell with original `grInishieMakeScale` setup by
  staging only the required scale DObj/model data behind
  `llGRInishieMapScaleDObjDesc` and `llGRInishieMapMapHead`, then keep the
  same bounded update proof.

Do not turn this into a DS-native fake platform implementation.

## Deferred

- Full moving platform collision/motion and stage-authored platform animation
  scripts.
- Full natural player-driven drop-through runtime beyond the bounded
  Wait -> Squat -> Pass input gate.
- Full imported `mpcommon.c`, `mpprocess.c`, or `mpcollision.c`.
