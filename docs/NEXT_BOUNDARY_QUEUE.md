# Next Boundary Queue

Compact current queue only. Expected harness names are proposed until they are
added to `scripts/lib/harness-registry.ps1`; current registry targets are
`battle_mariofox_stage_mplivehit_status_loop` and
`menu_chain_mariofox_stage_mplivehit_status_loop`.

Completed 2026-07-01: extended the `STAGE_MPLIVEHIT_CATCHSEARCH` secondary
skip mask inside current modes `161/162` from `0x1fff` to `0x3fff`. The added
bit restores Mario/Fox, runs BattleShip's source-order
`ftMainSearchFighterCatch` with attacker before target in the fighter list, and
records self rejection plus next-fighter target catch before state restore.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_STATUS` and
`STAGE_MPLIVEHIT_STATUS_CALLBACK` inside current modes `161/162` from mask
low bits `0x7fffffff` to `0xffffffff`. The added bit reruns installed
original `ftCommonDamageFallProcInterrupt` after DamageFall expiry, proving
BattleShip's source-order SpecialAir, AttackAir, JumpAerial, and HammerFall
checks before restoring state.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_STATUS` and
`STAGE_MPLIVEHIT_STATUS_CALLBACK` inside current modes `161/162` from mask
low bits `0x3fffffff` to `0x7fffffff`. The added bit reruns installed original
`ftCommonDamageFallProcMap` with collision but no `MAP_FLAG_CLIFF_MASK`,
proving the source no-cliff tail runs PassiveStand, Passive, and DownBounce
before restoring state; `fallmap` is now `0x7ff`.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_STATUS` and
`STAGE_MPLIVEHIT_STATUS_CALLBACK` inside current modes `161/162` from mask
low bits `0x1fffffff` to `0x3fffffff`. The added bit reruns installed original
`ftCommonDamageAirCommonProcMap` with collision but no `MAP_FLAG_FLOOR`,
proving the source floor-bit short-circuit skips PassiveStand, Passive, and
DownBounce before restoring state.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_STATUS` and
`STAGE_MPLIVEHIT_STATUS_CALLBACK` inside current modes `161/162` from mask
low bits `0xfffffff` to `0x1fffffff`. The added bit reruns installed original
`ftCommonDamageFallProcMap` with floor collision, proving imported-original
PassiveStand and Passive true-return short-circuits skip the DownBounce tail
before restoring state; `fallmap` is now `0x3ff`.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_STATUS` and
`STAGE_MPLIVEHIT_STATUS_CALLBACK` inside current modes `161/162` from mask
low bits `0x7ffffff` to `0xfffffff`. The added bit reruns installed original
`ftCommonDamageAirCommonProcMap` with floor collision, proving
imported-original PassiveStand and Passive true-return short-circuits skip
later map branches before restoring state.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_STATUS` and
`STAGE_MPLIVEHIT_STATUS_CALLBACK` inside current modes `161/162` from mask
low bits `0x3ffffff` to `0x7ffffff`. The added bit reruns installed original
`ftCommonDamageCommonProcLagUpdate`, proving the no-hitlag, below-threshold
stick, and saturated tap-buffer no-op gates before restoring state.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_STATUS` and
`STAGE_MPLIVEHIT_STATUS_CALLBACK` inside current modes `161/162` from mask
low bits `0x1ffffff` to `0x3ffffff`. The added bit reruns installed original
`ftCommonDamageCommonProcPhysics` with zero-hitstun Air kinetics and
over-max horizontal velocity, proving the source air-velocity clamp-decrement
branch before restoring the live-hit state.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_STATUS` and
`STAGE_MPLIVEHIT_STATUS_CALLBACK` inside current modes `161/162` from mask
low bits `0xffffff` to `0x1ffffff`. The added bit reruns installed original
`ftCommonDamageCommonProcPhysics` with zero-hitstun Air kinetics and
horizontal stick input, proving the source air-drift branch updates X velocity
after gravity before restoring the live-hit state.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_STATUS` and
`STAGE_MPLIVEHIT_STATUS_CALLBACK` inside current modes `161/162` from mask
low bits `0x7fffff` to `0xffffff`. The added bit reruns installed original
`ftCommonDamageCommonProcPhysics` with zero-hitstun Air kinetics, proving the
source `ftPhysicsApplyAirVelDriftFastFall` branch sets fastfall state and
terminal velocity before restoring the live-hit state.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_STATUS` and
`STAGE_MPLIVEHIT_STATUS_CALLBACK` inside current modes `161/162` from mask
low bits `0x3fffff` to `0x7fffff`. The added bit reruns installed original
`ftCommonDamageCommonProcPhysics` with ground kinetics, proving the source
ground-friction branch reduces ground velocity before restoring the live-hit
state.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_STATUS` and
`STAGE_MPLIVEHIT_STATUS_CALLBACK` inside current modes `161/162` from mask
low bits `0x1fffff` to `0x3fffff`. The added bit reruns installed original
`ftCommonDamageCommonProcPhysics` as `DamageFlyRoll`, proving the source
physics branch reaches `ftCommonDamageFlyRollUpdateModelPitch` and updates
joint pitch before restoring the live-hit state.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_STATUS` and
`STAGE_MPLIVEHIT_STATUS_CALLBACK` inside current modes `161/162` from mask
low bits `0xfffff` to `0x1fffff`. The added bit reruns installed original
`ftCommonDamageCommonProcLagUpdate` with hitlag, stick range, and tap-buffer
state, proving the Smash DI branch moves the root and resets tap buffers
before restoring the live-hit state.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_STATUS` and
`STAGE_MPLIVEHIT_STATUS_CALLBACK` inside current modes `161/162` from mask
low bits `0x7ffff` to `0xfffff`. The added bit reruns installed original
`ftCommonDamageCommonProcPhysics` with a low-speed throw-owned attack coll,
proving the source tail clears attack collisions before restoring the
live-hit state.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_STATUS` and
`STAGE_MPLIVEHIT_STATUS_CALLBACK` inside current modes `161/162` from mask
low bits `0x3ffff` to `0x7ffff`. The added bit reruns installed original
`ftCommonDamageSetStatus` with `DamageFlyRoll`, proving the source branch
reaches `ftCommonDamageFlyRollUpdateModelPitch` and updates joint pitch before
restoring the live-hit state.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_STATUS` and
`STAGE_MPLIVEHIT_STATUS_CALLBACK` inside current modes `161/162` from mask
low bits `0x1ffff` to `0x3ffff`. The added bit reruns installed original
`ftCommonDamageSetStatus` with `is_knockback_over` set, proving the source
branch clears that flag and sets timed hit-status invincibility before
restoring the live-hit state.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_STATUS` and
`STAGE_MPLIVEHIT_STATUS_CALLBACK` inside current modes `161/162` from mask
low bits `0xffff` to `0x1ffff`. The added bit seeds `DamageN1` with Air
kinetics, marks hammer held through the existing hammer-check seam, runs
installed original `ftCommonDamageCommonProcInterrupt` with hitstun already
zero, and proves the air hammer branch reaches
`ftCommonHammerFallProcInterrupt` before restoring the live-hit state.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_STATUS` and
`STAGE_MPLIVEHIT_STATUS_CALLBACK` inside current modes `161/162` from mask
low bits `0x7fff` to `0xffff`. The added bit seeds ground `DamageN1`, marks
hammer held through the existing hammer-check seam, runs installed original
`ftCommonDamageCommonProcInterrupt` with hitstun already zero, and proves the
hammer branch reaches `ftHammerProcInterrupt` before restoring the live-hit
state.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_STATUS` and
`STAGE_MPLIVEHIT_STATUS_CALLBACK` inside current modes `161/162` from mask
low bits `0x3fff` to `0x7fff`. The added bit seeds `DamageN1` with Air
kinetics, runs installed original `ftCommonDamageCommonProcInterrupt` with
hitstun already zero, and proves the no-hammer air branch reaches
`ftCommonFallProcInterrupt` before restoring the live-hit state.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_STATUS` and
`STAGE_MPLIVEHIT_STATUS_CALLBACK` inside current modes `161/162` from mask
low bits `0x1fff` to `0x3fff`. The added bit seeds ground `DamageN1`, runs
installed original `ftCommonDamageCommonProcInterrupt` with hitstun already
zero, and proves it clears hitstun state and reaches imported-original
Wait/Ground interrupt handling before restoring the live-hit state.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_STATUS` and
`STAGE_MPLIVEHIT_STATUS_CALLBACK` inside current modes `161/162` from mask
low bits `0xfff` to `0x1fff`. The added bit seeds ground `DamageN1`, runs
installed original `ftCommonDamageCommonProcUpdate` at animation end as
hitstun reaches zero, and proves public knockback release through original
`mpCommonSetFighterWaitOrFall` into imported-original Wait/Ground before
restoring the live-hit state.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_STATUS` and
`STAGE_MPLIVEHIT_STATUS_CALLBACK` inside current modes `161/162` from mask
low bits `0x7ff` to `0xfff`. The added bit reruns installed original
`ftCommonDamageAirCommonProcUpdate` with animation still active while hitstun
reaches zero, proving public knockback release without the DamageFall expiry
handoff.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_DAMAGE` inside current
modes `161/162` from mask low bits `0x7fffffff` to `0xffffffff`. The added
bit marks the attacking fighter ghost and proves BattleShip's
`ftMainProcSearchHitAll` outer guard exits before hitlog clear, hit-search,
or hit-stat processing.

Completed 2026-07-01: strengthened `STAGE_MPLIVEHIT_CATCHSEARCH` inside
current modes `161/162` without changing the public mask low bits
`0xffffffff`. The existing true-return hazard-dispatch bit now also requires
one installed original TaruCannon physics callback tick copying the fighter
root position from the barrel root after source-ordered TaruCannon setup and
before state restore. Continuous TaruCannon update/shoot runtime still waits
for the Jungle barrel helpers and map throw-hit data; marker remains
`catchSearch=0xffffffff/s3`.

Completed 2026-07-01: strengthened `STAGE_MPLIVEHIT_CATCHSEARCH` inside
current modes `161/162` without changing the public mask low bits
`0xffffffff`. The existing true-return hazard-dispatch bit now also requires
TaruCannon kind `3` through `ftMainSetHitHazard` into the source-ordered
TaruCannon setup shell: status `61`, script `-1`, TaruCannon status-vars reset,
barrel GObj capture, capture immunity, invisible flag, and intangible hitstatus
before state restore. A later strengthened proof adds the installed
TaruCannon physics tick; continuous update/shoot runtime still waits for the
Jungle barrel helpers and map throw-hit data; marker remains
`catchSearch=0xffffffff/s3`.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_CATCHSEARCH` inside current
modes `161/162` from mask low bits `0x7fffffff` to `0xffffffff`. The added
bit marks the fighter ghost and proves BattleShip's `ftMainSearchHitHazard`
outer guard exits before wait-timer decrement or obstacle callbacks; marker is
`catchSearch=0xffffffff/s3`.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_CATCHSEARCH` inside current
modes `161/162` from mask low bits `0x3fffffff` to `0x7fffffff`. The added
bit runs one installed imported-original Twister update/physics callback tick
after the true-return hazard dispatch, proving release-wait advance, bounded
air-velocity update, and root Y-rotation before restoring state. TaruCannon
true-return setup dispatch was completed in a later strengthened proof;
continuous hazard runtime remains deferred; marker is `catchSearch=0x7fffffff/s3`.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_CATCHSEARCH` inside current
modes `161/162` from mask low bits `0x1fffffff` to `0x3fffffff`. The added
bit imports bounded original `ftcommontwister.c`, routes true-return
`nGMHitEnvironmentTwister` callbacks through `ftMainSetHitHazard` /
`ftCommonTwisterSetStatus`, and proves Twister status/motion/callback install,
release wait reset, tornado GObj capture, capture immunity, and wait-timer
decrement before restoring state. TaruCannon true-return setup dispatch was
completed in a later strengthened proof; continuous hazard runtime remains
deferred; marker is `catchSearch=0x3fffffff/s3`.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_CATCHSEARCH` inside current
modes `161/162` from mask low bits `0x7ffffff` to `0x1fffffff`. The added
bits restore the original two-entry ground-obstacle registry, prove
add/full/clear ordering, and prove `ftMainSearchHitHazard` iterates
registered false-return obstacle callbacks in source order while decrementing
Twister/TaruCannon wait timers. True-return trap status dispatch remains
deferred; marker is `catchSearch=0x1fffffff/s3`.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_CATCHSEARCH` inside current
modes `161/162` from mask low bits `0xffffff` to `0x7ffffff`. The added bits
restore BattleShip's `is_catchstatus` flag, route the selected catch search
through bounded `ftMainProcSearchCatch`, prove hazard wait-timer decrement
before the catch-status gate, prove no search/callbacks when the gate is
closed, and prove catch/capture callbacks after a selected target is found
with marker `catchSearch=0x7ffffff/s3`.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_CATCHSEARCH` inside current
modes `161/162` from mask low bits `0x1ffff` to `0xffffff`. The added bits
prove attack-state-off, Ground/Air mismatch, hurt/shield/group attack-record
skips, the hitstatus-none sentinel break, and valid no-collision no-update
with marker `catchSearch=0xffffff/s3`.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_CATCHSEARCH` inside current
modes `161/162` from mask low bits `0xffff` to `0x1ffff`. The added bit
restores Mario/Fox, seeds each global target hitstatus field non-normal one
at a time, reruns source-order `ftMainSearchFighterCatch`, and proves target
rejection with marker `catchSearch=0x1ffff/s3`.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_CATCHSEARCH` inside current
modes `161/162` from mask low bits `0x3fff` to `0xffff`. The added bits
restore Mario/Fox, seed ghost and Boss targets, rerun source-order
`ftMainSearchFighterCatch`, and prove target rejection with marker
`catchSearch=0xffff/s3`.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_CATCHSEARCH` inside current
modes `161/162` from mask low bits `0x1fff` to `0x3fff`. The added bit
restores Mario/Fox, seeds same-team fighters with team battle on and team
attack off, reruns source-order `ftMainSearchFighterCatch`, and proves target
rejection with marker `catchSearch=0x3fff/s3`.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_CATCHSEARCH` inside current
modes `161/162` from mask low bits `0xfff` to `0x1fff`. The added bit restores
Mario/Fox, seeds `capture_immune_mask & catch_mask`, reruns source-order
`ftMainSearchFighterCatch`, and proves target rejection with marker
`catchSearch=0x1fff/s3`.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_CATCHSEARCH` inside current
modes `161/162` from mask low bits `0x7ff` to `0xfff`. The added bit restores
saved Mario/Fox fighter structs, clears the catch record, reruns source-order
`ftMainSearchFighterCatch` against Mario's natural damage-coll slot `0`, and
proves target, distance, and attack-record update with marker
`catchSearch=0xfff/s3`.

Completed 2026-07-01: extended `STAGE_MPLIVEHIT_CATCHSEARCH` inside current
modes `161/162` from mask low bits `0x3ff` to `0x7ff`. The added bit reruns
source-order `ftMainSearchFighterCatch` without clearing the first catch
record and proves same-victim catch repeat rejection with marker
`catchSearch=0x7ff/s3`.

Completed 2026-06-30: routed the public `ftCommonDamageUpdateMain` seam
through imported original BattleShip code, and changed the existing first
catch/keep-hold and zero-knockback catch `DASH_RUN_PROCPARAMS` branch proofs
to call the public dispatcher seam. The aggregate marker remains stable.

Completed 2026-06-30: routed the public
`ftCommonDamageCheckCatchResist` and `ftCommonDamageCheckCaptureKeepHold`
seams through imported original BattleShip code, and changed the existing
`DASH_RUN_DAMAGE_HOLD_RESIST` proof to call the public seams. The marker
remains stable at `hold=0xff`.

Completed 2026-06-30: routed the public
`ftCommonDamageUpdateCatchResist` seam through imported original BattleShip
`ftCommonDamageUpdateCatchResist`, and changed the existing
`DASH_RUN_DAMAGE_UPDATE_CATCH_RESIST` proof to call the public seam. The
marker remains stable at `catchResist=0x1f`.

Completed 2026-06-30: routed the public
`ftCommonDamageGetDamageLevel` and `ftCommonDamageGetKnockbackAngle` seams
through imported original BattleShip `ftCommonDamageGetDamageLevel` /
`ftCommonDamageGetKnockbackAngle`, and changed the existing
`DASH_RUN_DAMAGE_LEVELS` and `DASH_RUN_DAMAGE_KNOCKBACK_ANGLE` proofs to call
the public seams. The markers remain stable at `level=0x1f` and
`angle=0x3f`.

Completed 2026-06-30: routed the public
`ftCommonDamageCheckElementSetColAnim`, `ftCommonDamageCheckMakeScreenFlash`,
and `ftCommonDamageSetPublic` seams through imported original BattleShip
`ftCommonDamageCheckElementSetColAnim`,
`ftCommonDamageCheckMakeScreenFlash`, and `ftCommonDamageSetPublic`, and
changed the existing `DASH_RUN_DAMAGE_COLANIM`,
`DASH_RUN_DAMAGE_SCREEN_FLASH`, and `DASH_RUN_DAMAGE_PUBLIC` proofs to call
the public seams. The markers remain stable at `colAnim=0x3f`,
`flash=0x7f`, and `public=0x3f`.

Completed 2026-06-30: routed the public
`ftCommonDamageUpdateDustEffect` and `ftCommonDamageDecHitStunSetPublic`
seams through imported original BattleShip `ftCommonDamageUpdateDustEffect` /
`ftCommonDamageDecHitStunSetPublic`, and changed the existing
`DASH_RUN_DAMAGE_DUST_UPDATE` and `DASH_RUN_DAMAGE_HITSTUN_PUBLIC` proofs to
call the public seams. The markers remain stable at `dustUpdate=0x1f` and
`hitPublic=0xf`.

Completed 2026-06-30: routed the public
`ftCommonDamageUpdateDamageColAnim` and `ftCommonDamageSetDamageColAnim` seams
through imported original BattleShip `ftCommonDamageUpdateDamageColAnim` /
`ftCommonDamageSetDamageColAnim`, and changed the existing
`DASH_RUN_DAMAGE_COLANIM_UPDATE` proof to call the public seams. The marker
remains stable at `colAnimUpdate=0x1f`.

Completed 2026-06-30: routed the public
`ftCommonDamageFlyRollUpdateModelPitch` seam through imported original
BattleShip `ftCommonDamageFlyRollUpdateModelPitch`, and changed the existing
FlyRoll physics pitch proof to expect original `syUtilsArcTan2` math. The
existing `DASH_RUN_DAMAGE_SETUP` marker remains stable while the pitch/update
tail now reaches original BattleShip code.

Completed 2026-06-30: routed the public `ftCommonDamageSetStatus` seam through
imported original BattleShip `ftCommonDamageSetStatus`. The existing
`DASH_RUN_DAMAGE_SETUP` proof already calls the public seam through the
electric passive status-dispatch tick, so the marker remains stable while the
status handoff now reaches original BattleShip code.

Completed 2026-06-30: routed the public
`ftCommonDamageCheckSetInvincible` seam through imported original BattleShip
`ftCommonDamageCheckSetInvincible`, and changed the invincibility gate proof
to call the public seam instead of the private imported symbol directly. The
diagnostic remains `invGate=0x1f`, now proving the public seam's
imported-original route plus hitlag gate, knockback-over gate,
timed-invincible true branch, original-route bit, and proof-local restore.

Completed 2026-06-30: routed the public
`ftCommonDamageAirCommonProcMap` seam through imported original BattleShip
`ftCommonDamageAirCommonProcMap`, with a narrow public
`ftCommonWallDamageCheckGoto` alias back to the imported WallDamage helper.
The diagnostic now records `airMapWall=0x3f`, proving DamageAir wall-map
collision, WallDamage side effects, Passive/DownBounce short-circuit,
reflected knockback/LR, imported-original AirCommon map routing, and
proof-local restore.

Completed 2026-06-30: routed the public
`ftCommonDamageAirCommonProcInterrupt` seam through imported original
BattleShip `ftCommonDamageAirCommonProcInterrupt`. The diagnostic now records
`commonCb=0x1fff`, proving AirCommon interrupt through imported-original
routing while keeping the DamageFall child seam bounded.

Completed 2026-06-30: routed the public
`ftCommonDamageCommonProcInterrupt` seam through imported original BattleShip
`ftCommonDamageCommonProcInterrupt`. The diagnostic now records
`commonCb=0x1fff`, proving common and AirCommon interrupt branches through
imported-original routing.

Completed 2026-06-30: routed the public
`ftCommonDamageAirCommonProcUpdate` seam through imported original BattleShip
`ftCommonDamageAirCommonProcUpdate`. The diagnostic now records
`commonCb=0x1fff`, proving ground/air update and interrupt branches through
imported-original routing.

Completed 2026-06-30: routed the public
`ftCommonDamageCommonProcUpdate` seam through imported original BattleShip
`ftCommonDamageCommonProcUpdate`. The diagnostic now records
`commonCb=0x1fff`, proving the ground/air update and interrupt branches through
imported-original routing.

Completed 2026-06-30: routed the public
`ftCommonDamageCommonProcPhysics` seam through imported original BattleShip
`ftCommonDamageCommonProcPhysics`. The diagnostic now records
`commonPhys=0x3f`, proving ground friction, air friction, air drift/gravity,
low-speed throw attack-clear, proof-local restore, and imported-original
routing.

Completed 2026-06-30: routed the existing `DASH_RUN_DAMAGE_PUBLIC` proof
through imported original BattleShip `ftCommonDamageSetPublic`, adding a narrow
project-owned `ftParamGetPlayerNumGObj` compatibility seam. The diagnostic now
records `public=0x3f`, proving angle reduction, target public reset, force
handoff, non-force branch, proof-local restore, and imported-original routing.

Completed 2026-06-30: routed the existing
`DASH_RUN_DAMAGE_DUST_UPDATE` proof through imported original BattleShip
`ftCommonDamageUpdateDustEffect`. The diagnostic now records
`dustUpdate=0x1f`, proving nonzero timer decrement without spawning,
zero-cross DustExpandLarge spawn, imported-original interval reset,
proof-local restore, and imported-original routing.

Completed 2026-06-30: routed the existing
`DASH_RUN_DAMAGE_COLANIM_UPDATE` proof through imported original BattleShip
`ftCommonDamageUpdateDamageColAnim` and `ftCommonDamageSetDamageColAnim`.
The diagnostic now records `colAnimUpdate=0x1f`, proving direct wrapper
routing, struct-field wrapper routing, gated no-update behavior, proof-local
restore, and imported-original routing.

Completed 2026-06-30: routed the existing
`DASH_RUN_DAMAGE_HITSTUN_PUBLIC` proof through imported original BattleShip
`ftCommonDamageDecHitStunSetPublic`. The diagnostic now records
`hitPublic=0xf`, proving nonzero hitstun decrement, zero-cross
public-knockback transfer, proof-local restore, and imported-original routing.

Completed 2026-06-30: routed the existing
`DASH_RUN_DAMAGE_COLANIM` proof through imported original BattleShip
`ftCommonDamageCheckElementSetColAnim`. The diagnostic now records
`colAnim=0x3f`, proving Fire/Electric/Freezing/default damage
color-animation routes, proof-local restore, and imported-original routing.

Completed 2026-06-30: routed the existing
`DASH_RUN_DAMAGE_SCREEN_FLASH` proof through imported original BattleShip
`ftCommonDamageCheckMakeScreenFlash`. The diagnostic now records
`flash=0x7f`, proving low-knockback no-op, high-knockback
Fire/Electric/Freezing/default screen-flash routes, and imported-original
routing.

Completed 2026-06-30: routed the existing
`DASH_RUN_DAMAGE_LEVELS` threshold proof through imported original BattleShip
`ftCommonDamageGetDamageLevel`. The diagnostic now records `level=0x1f`,
proving low/mid/high/fly levels at hitstun `0`, `12`, `24`, and `32` plus
imported-original routing.

Completed 2026-06-30: routed the existing
`DASH_RUN_DAMAGE_KNOCKBACK_ANGLE` proof through imported original BattleShip
`ftCommonDamageGetKnockbackAngle`. The diagnostic now records `angle=0x3f`,
proving fixed-angle, air `361`, ground low/high/capped `361`, and
imported-original routing.

Completed 2026-06-30: routed the existing
`DASH_RUN_DAMAGE_DUST` threshold proof through public
`ftCommonDamageSetDustEffectInterval` into imported original BattleShip code.
The diagnostic now records `dust=0xff`, proving the low, mid-low, mid,
mid-high, high, and default air threshold buckets plus public
imported-original routing and proof-local restore.

Completed 2026-06-30: imported original BattleShip `ftcommondamage.c` under
bounded `ndsBase*` names and strengthened the existing
`DASH_RUN_DAMAGE_UPDATE_CATCH_RESIST` diagnostic to `catchResist=0x1f`. The
new high bit proves the zero-knockback color-animation branch through the
compiled original `ftCommonDamageUpdateCatchResist` function while leaving
full DK throw damage runtime deferred behind a compile seam.

Completed 2026-06-30: bounded source-shaped
`ftCommonDamageUpdateCatchResist` branch coverage was added to the inherited
Dash-Run damage proof used by current modes `159/160`. The new
`DASH_RUN_DAMAGE_UPDATE_CATCH_RESIST` diagnostic records `catchResist=0x1f`,
proving zero-knockback and paused low-stack knockback color-animation paths,
the non-resist status-dispatch side seam, public imported-original routing,
and proof-local restore.

Completed 2026-06-30: corrected the project-owned
`ftCommonDamageInitDamageVars` seam so hit-element setup no longer overwrites
`FTStruct.damage_kind`. The new `DASH_RUN_DAMAGE_KIND` diagnostic records
`dmgKind=0x7`, proving before/after preservation and proof-local restore.

Completed 2026-06-30: bounded source-shaped held-item fallthrough coverage was
added to the inherited Dash-Run damage proof used by current modes `159/160`.
The new `DASH_RUN_DAMAGE_ITEM_BYPASS` diagnostic records `itemBypass=0x1f`,
proving light items and heavy items held by non-DK fighters skip the original
heavy-item branch, keep the item attached, reach the normal damage
color-animation tail, and restore proof-local state.

Completed 2026-06-30: explicit bounded source-shaped DK-family heavy-item
branch coverage was added to the inherited Dash-Run damage proof used by
current modes `159/160`. The new `DASH_RUN_DAMAGE_ITEM_HEAVY` diagnostic
records `itemHeavy=0x1f`, proving the heavy-item predicate, catch-resist
return, drop/status return, and proof-local restore.

Completed 2026-06-30: bounded imported-original damage-level threshold coverage
was added to the inherited Dash-Run damage proof used by current modes
`159/160`. The new `DASH_RUN_DAMAGE_LEVELS` diagnostic records `level=0x1f`,
proving `ftCommonDamageGetDamageLevel` low/mid/high/fly routing at hitstun
`0`, `12`, `24`, and `32` plus imported-original routing.

Completed 2026-06-30: bounded source-shaped damage hold/resist gate coverage
was added to the inherited Dash-Run damage proof used by current modes
`159/160`. The new `DASH_RUN_DAMAGE_HOLD_RESIST` diagnostic records
`hold=0xff`, proving Sleep blocks catch-resist, zero knockback resists,
paused low-stack knockback resists, Donkey cargo throw low-level damage
resists, default high knockback does not resist, capture keep-hold true/false
thresholds, and proof-local restore.

Completed 2026-06-30: bounded source-shaped replacement/electric passive
dispatch coverage was strengthened in the inherited Dash-Run damage proof used
by current modes `159/160`. The `DASH_RUN_DAMAGE_REPLACE_ELECTRIC` diagnostic
now records `replace=0x3f`, proving `ftCommonDamageSetStatus` leaves status,
motion, stored replacement status, and animation-event calls unchanged while
`hitlag_tics > 0`, then dispatches to stored replacement status/motion `55/48`
once hitlag is clear.

Completed 2026-06-30: bounded common damage callback stay/expiry coverage was
strengthened in the inherited Dash-Run damage proof used by current modes
`159/160`. The `DASH_RUN_DAMAGE_COMMON_CALLBACKS` diagnostic now records
`commonCb=0x1fff`, proving imported-original ground and air update
stay/expiry routing, imported-original common and AirCommon interrupt routing,
and proof-local restore.

Completed 2026-06-30: bounded source-shaped
`ftCommonDamageInitDamageVars` status replacement plus electric wrapping
coverage was added to the inherited Dash-Run damage proof used by current
modes `159/160`. The new `DASH_RUN_DAMAGE_REPLACE_ELECTRIC` diagnostic records
`replace=0x1f`, proving replacement status `55` is stored, electric wrapper
status/motion `50/43` is installed, the electric passive dispatch reaches
stored replacement status/motion `55/48`, and proof-local fighter state is
restored.

Completed 2026-06-30: bounded source-shaped
`ftCommonDamageInitDamageVars` deterministic FlyTop branch coverage was added
to the inherited Dash-Run damage proof used by current modes `159/160`. The
new `DASH_RUN_DAMAGE_FLYTOP` diagnostic records `flytop=0xf`, proving
airborne high-knockback 90-degree angle-window selection into DamageFlyTop
status/motion `54/47` with proof-local restore.

Completed 2026-06-30: bounded source-shaped
`ftCommonDamageGetKnockbackAngle` Sakurai-angle coverage was added to the
inherited Dash-Run damage proof used by current modes `159/160`. The new
`DASH_RUN_DAMAGE_KNOCKBACK_ANGLE` diagnostic records `angle=0x3f`, proving
fixed-angle, air `361`, ground low-knockback `361`, ground high-knockback
scaled `361`, capped ground `361`, and imported-original routing branches.

Completed 2026-06-30: routed the public
`ftCommonDamageCommonProcLagUpdate` seam through imported original BattleShip
`ftCommonDamageCommonProcLagUpdate`. The diagnostic now records
`lagUpdate=0x3f`, proving the hitlag gate, stick-range gate, tap-buffer gate,
active Smash DI root translation branch, proof-local restore, and
imported-original routing.

Completed 2026-06-30: routed existing `DASH_RUN_DAMAGE_INVINCIBLE` through
imported original BattleShip `ftCommonDamageCheckSetInvincible`. The diagnostic
now records `invGate=0x1f`, proving the hitlag gate, knockback-over flag gate,
timed-invincible true branch, proof-local restore, and imported-original
routing.

Completed 2026-06-30: bounded imported
`ftCommonDamageFallProcInterrupt` source-order coverage was added to the
inherited Dash-Run damage proof used by current modes `159/160`. The new
`DASH_RUN_DAMAGE_FALL_INTERRUPT` diagnostic records `fallInterrupt=0x3f`,
proving the imported interrupt call reaches special-air, attack-air,
jump-aerial, and hammer fallback checks, then restores local state.

Completed 2026-06-30: bounded DamageAir wall-map short-circuit coverage was
added to the inherited Dash-Run damage proof used by current modes `159/160`.
The `DASH_RUN_DAMAGE_AIR_MAP_WALL` diagnostic now records `airMapWall=0x3f`,
proving wall collision, original WallDamage helper side effects,
Passive/DownBounce short-circuit, reflected knockback/LR, imported-original
AirCommon map routing, and proof-local restore.

Completed 2026-06-30: bounded imported-original
`ftCommonDamageCommonProcLagUpdate` gate coverage was added to the inherited
Dash-Run damage proof used by current modes `159/160`. The
`DASH_RUN_DAMAGE_LAGUPDATE` diagnostic records `lagUpdate=0x3f`, proving the
hitlag gate, stick-range gate, tap-buffer gate, active Smash DI root
translation branch, proof-local restore, and imported-original routing.

Completed 2026-06-30: bounded imported-original
`ftCommonDamageCheckSetInvincible` gate coverage was added to the inherited
Dash-Run damage proof used by current modes `159/160`. The
`DASH_RUN_DAMAGE_INVINCIBLE` diagnostic records `invGate=0x1f`, proving the
hitlag gate, knockback-over flag gate, timed-invincibility true branch,
proof-local restore, and imported-original routing.

Completed 2026-06-30: bounded imported-original
`ftCommonDamageUpdateDamageColAnim` / `ftCommonDamageSetDamageColAnim`
coverage was added to the inherited Dash-Run damage proof used by current
modes `159/160`. The `DASH_RUN_DAMAGE_COLANIM_UPDATE` diagnostic records
`colAnimUpdate=0x1f`, proving direct wrapper routing, struct-field wrapper
routing, gated no-update behavior, proof-local restore, and imported-original
routing.

Completed 2026-06-30: bounded imported-original
`ftCommonDamageCheckElementSetColAnim` coverage was added to the inherited
Dash-Run damage proof used by current modes `159/160`. The
`DASH_RUN_DAMAGE_COLANIM` diagnostic records `colAnim=0x3f`, proving the
original Fire/Electric/Freezing/default damage color-animation routes,
proof-local restore, and imported-original routing.

Completed 2026-06-30: bounded imported-original
`ftCommonDamageCheckMakeScreenFlash` coverage was added to the inherited
Dash-Run damage proof used by current modes `159/160`. The new
`DASH_RUN_DAMAGE_SCREEN_FLASH` diagnostic records `flash=0x7f`, proving the
original low-knockback no-op plus high-knockback Fire/Electric/Freezing/default
screen-flash colanim routes plus imported-original routing.

Completed 2026-06-30: bounded imported-original
`ftCommonDamageDecHitStunSetPublic` coverage was added to the inherited
Dash-Run damage proof used by current modes `159/160`. The
`DASH_RUN_DAMAGE_HITSTUN_PUBLIC` diagnostic records `hitPublic=0xf`, proving
the original nonzero hitstun decrement, zero-cross public-knockback transfer,
field-level proof restore, and imported-original routing.

Completed 2026-06-30: bounded imported-original `ftCommonDamageUpdateDustEffect`
runtime coverage was added to the inherited Dash-Run damage proof used by
current modes `159/160`. The new `DASH_RUN_DAMAGE_DUST_UPDATE` diagnostic
records `dustUpdate=0x1f`, proving the original nonzero timer decrement
without spawning, zero-cross DustExpandLarge spawn, interval reset,
field/counter restore, and imported-original routing.

Completed 2026-06-30: bounded imported-original `ftCommonDamageSetPublic` public
reaction coverage was added to the inherited Dash-Run damage proof used by
current modes `159/160`. The `DASH_RUN_DAMAGE_PUBLIC` diagnostic now records
`public=0x3f`, proving the original angle-window public-knockback reduction to
`160000`, target public-knockback reset, very-high attacker force handoff,
default non-forced attacker branch, field-level proof restore, and
imported-original routing.

Completed 2026-06-30: bounded source-shaped `ftMainProcParams` attacker-side
`attack_damage` rumble coverage was added to the inherited Dash-Run damage
proof used by current modes `159/160`. The
`DASH_RUN_PROCPARAMS_RUMBLE` diagnostic now records `procRumble=0x7f` and
derived `procRebound=0x1f`, proving the normal damage-derived rumble branch,
the original BatSwing4 special-case rumble ID `10` length `0` through the
project-owned `ftParamMakeRumble` seam, and the selected attack-rebound
promotion into imported-original ReboundWait effects.

Completed 2026-06-30: bounded public-wrapper
`ftCommonDamageSetDustEffectInterval` threshold coverage was added to the
inherited Dash-Run damage proof used by current modes `159/160`. The
`DASH_RUN_DAMAGE_DUST` diagnostic records `dust=0xff`, proving the original
low, mid-low, mid, mid-high, high, and default air dust interval buckets,
public imported-original routing, and proof-local fighter-state restore.

Completed 2026-06-30: bounded source-shaped `ftCommonDamageInitDamageVars`
Kirby copy-loss coverage was added to the inherited Dash-Run damage proof used
by current modes `159/160`. The new `DASH_RUN_DAMAGE_KIRBYCOPY` diagnostic
records `kirbycopy=0x6`, proving the selected copy ID reset from Fox copy `1`
to Kirby `8` plus FGM `204` without importing full Kirby effect/model-part
runtime.

Completed 2026-06-30: bounded source-shaped `ftCommonDamageInitDamageVars`
random FlyRoll branch coverage was added to the inherited Dash-Run damage
proof used by current modes `159/160`. The new `DASH_RUN_DAMAGE_FLYROLL`
diagnostic records `flyroll=0x1f`, proving the original airborne non-FlyTop
percent/RNG selection branch into DamageFlyRoll status/motion `55/48` with
proof-local RNG and fighter-state restore.

Completed 2026-06-30: bounded source-shaped `ftCommonDamageInitDamageVars`
damage voice SFX branch coverage was added to the inherited Dash-Run damage
proof used by current modes `159/160`. The new `DASH_RUN_DAMAGE_VOICE`
diagnostic records `voice=0xf`, proving the original hitstun-threshold and
forced-call branches through the existing audio stub seam with proof-local
attribute restore.

Completed 2026-06-30: bounded source-shaped `ftCommonDamageUpdateMain`
Sleep-element dispatcher coverage was added to the inherited Dash-Run damage
proof used by current modes `159/160`. The new `DASH_RUN_DAMAGE_SLEEP`
diagnostic records `sleep=0x7f`, proving no catch/capture/item links, Sleep
element, zero knockback, FuraSleep status/motion `165/145`, cliff-catch wait
setup, FuraSleep color-animation seam reachability, and proof-local restore.

Completed 2026-06-30: bounded source-shaped catch-search pre-stat gate
coverage was added to current modes `159/160`. The new
`STAGE_MPLIVEHIT_CATCHSEARCH` diagnostic records `catchSearch=0x3ff/s3`,
proving reset, target/GA gates, hurt-record skip, default-record pass, status
and grabbable skips, selected slot-3 collide, closest target assignment, and
proof-local restore.

Completed 2026-06-30: bounded source-shaped catch-stat distance/search
coverage was added to current modes `159/160`. The new
`STAGE_MPLIVEHIT_CATCHSTAT` diagnostic records
`catchStat=0x1f/160000`, proving the selected hit-interact damage record,
attack-detect clear, closest target distance, search target assignment, and
proof-local restore.

Completed 2026-06-30: bounded source-shaped damage-resist breakthrough
coverage was added to current modes `159/160`. The new
`STAGE_MPLIVEHIT_RESIST_BREAK` diagnostic records
`rbreak=0x7f/2->-2/q2 throwAttr=0x1f/1->0 clash=0x3f/24/18`, proving the selected
`ftMainCheckGetUpdateDamage` branch clears the damage-resist flag when armor
breaks, carries the negative resist remainder into leftover damage, and queues
matching damage/lag before restoring fighter state.

Completed 2026-06-30: bounded source-shaped non-normal hitstatus
effect/SFX coverage was added to current modes `159/160`. The new
`STAGE_MPLIVEHIT_EFFECTONLY` diagnostic records `eff=0x1ff/0->0`, proving the
selected `ftMainUpdateDamageStatFighter` branch inserts the hit record,
updates attacker `attack_damage`, emits the set-off effect and hit SFX seams,
and skips damage queue/percent/hitlog when the selected damage-coll slot is
invincible.

Completed 2026-06-30: bounded source-shaped damage-resist false-return
coverage was added to current modes `159/160`. The new
`STAGE_MPLIVEHIT_RESIST` diagnostic records `resist=0xfff/7->3`, proving the
selected `ftMainCheckGetUpdateDamage` branch decrements damage armor, keeps
the resist flag active, emits the set-off effect and hit SFX seams, and skips
damage queue/percent/hitlog while restoring fighter state.

Completed 2026-06-30: bounded selected shield-contact `hitlag_mul` tail reset
coverage was added to current modes `159/160`. The
`STAGE_MPLIVEHIT_SHIELD_CONTACT` diagnostic now records
`shc=0x7fffff/3142`, proving the selected normal shield-contact tail also
resets `hitlag_mul` to `1.0F` in source order.

Completed 2026-06-30: bounded selected shield-contact special/rebound
transient clear coverage was added to current modes `159/160`. The
`STAGE_MPLIVEHIT_SHIELD_CONTACT` diagnostic now records
`shc=0x7fffff/3142`, proving the selected normal shield-contact tail also
clears reflect, absorb, rebound, and knockback transients in source order.

Completed 2026-06-30: bounded selected normal shield-contact common tail
clear coverage was added to current modes `159/160`. The
`STAGE_MPLIVEHIT_SHIELD_CONTACT` diagnostic now records
`shc=0x7fffff/3142`, proving the non-break GuardSetOff shield branch also runs
the common post-branch tail that clears attack/damage transient fields and
does not set knockback pause.

Completed 2026-06-30: bounded selected ShieldBreakFly hitlag/input/transient
clear tail coverage was added to current modes `159/160`. The
`STAGE_MPLIVEHIT_SHIELD_CONTACT` diagnostic now records
`shc=0x7fffff/3142`, proving the low-shield ShieldBreakFly branch also runs the
common post-shield damage tail: hitlag calculation, input tap/release clear,
lagstart callback, and transient shield-damage clear while keeping
ShieldBreakFly status.

Completed 2026-06-30: bounded selected shield-heal branch coverage was added
to current modes `159/160`. The `STAGE_MPLIVEHIT_SHIELD_CONTACT` diagnostic
now records `shc=0x7fffff/3142`, proving the selected not-shielding,
below-full shield path decrements `shield_heal_wait`, restores shield health
by one, and resets the wait timer to `10` in the original `ftMainProcParams`
order.

Completed 2026-06-30: bounded selected shield-break branch coverage was added
to current modes `159/160`. The `STAGE_MPLIVEHIT_SHIELD_CONTACT` diagnostic
now records `shc=0x7fffff/3142`, proving the selected shield-contact path also
routes the low-shield branch into ShieldBreakFly through the existing
project-owned compatibility seam while restoring proof-local state.

Completed 2026-06-30: bounded selected shield-health decrement coverage was
added to current modes `159/160`. The `STAGE_MPLIVEHIT_SHIELD_CONTACT`
diagnostic now records `shc=0x7fffff/3142`, proving the selected shield-contact
path also subtracts `shield_damage_total` from `shield_health` in the original
`ftMainProcParams` order before the existing GuardSetOff/hitlag clear proof.

Completed 2026-06-30: bounded same-group attack-record carry/clear coverage
was added to current modes `159/160`. The `STAGE_MPLIVEHIT_EVENTS`
diagnostic now records `carry=0xf`, proving source-shaped seed, same-group
copy, no-sibling clear, and proof-local restore for the original
MakeAttackColl attack-record carry branch without adding a new harness.

Completed 2026-06-30: bounded selected hit-record detect-gate coverage was
added to current modes `159/160`. The existing `STAGE_MPLIVEHIT_REHIT`
diagnostic now records `gate=0x3f`, proving the source-order fighter-vs-fighter
branch enables damage detect for an empty/default record and skips records
already marked hurt, shield, or nondefault group before range/hurtbox work.

Completed 2026-06-30: bounded selected live-hit hurtbox global hitstatus gate
coverage was added to current modes `159/160`. The existing
`STAGE_MPLIVEHIT_HURTBOX` proof now tightens mask low bits from `0xff` to
`0xffff`, proving the source-order `special_hitstatus` / `star_hitstatus` /
`hitstatus` non-intangible gate, each intangible skip, and the per-attack
`gFTMainIsDamageDetect[i] == FALSE` skip before the existing slots `0` and
`1` intangible skips, slot `2` tested miss, slot `3` hit, and slot `10`
`None` sentinel proof.

Completed 2026-06-30: bounded original GuardSetOff update-tick coverage was
added to current modes `159/160`. The selected live-hit shield branch now
drives imported original `ftCommonGuardSetOffProcUpdate` through one held-Z
tick that stays in GuardSetOff and decrements `setoff_frames`, then one
released-Z tick that exits to GuardOff, all with proof-local restore. Current
marker includes `soTick=0x1f/155->154`.

Completed 2026-06-30: bounded selected shield-contact branch coverage was
added to the current modes `159/160`. The proof keeps the selected Fox Jab2
live-hit path and adds the source-order shield branch gate through `is_shield`,
damage detect, bounded shield sphere contact, shield-stat handoff, attack
record clear, adjacent shield-off and damage-detect-off skips, and state
restore. Current marker:
`mpLiveHit=atk=191/166 hitbox=1/j14 dmg=4 second=0/j14/x140 hurt=3/10 hbdmg=0->4/6 eff=0x1ff/0->0 resist=0xfff/7->3 rbreak=0x7f/2->-2/q2 throwAttr=0x1f/1->0 clash=0x3f/24/18 catchStat=0x1f/160000 catchSearch=0x3ff/s3 shield=4->4/4 shc=0x7fffff/3142 so=155/134 contact=1 repeat=1 carry=0xf gate=0x3f rehit=5->0 origRehit=hit30/v40000 30->29->0 clear=1/3 ids=0x3 dmg=0->4 hitlag=6`.
Full `gmcollision.c`, full `ftmain.c`, continuous shield runtime beyond the
selected contact/set-off/health-decrement/break/heal/break-clear/tail-clear/special-clear/hitlag-mul-clear proof, and player-driven shield gameplay remain
deferred.

Completed 2026-06-29: bounded selected live-hit damage lifecycle proof shipped
as modes `159/160`. The direct/menu-chain pair keeps the live VSBattle roots on
Pupupu/Dream Land, inherits the modes `157/158` damage-recover aggregate plus
Dash-Run selected contact/damage setup proof, then proves selected Fox Jab2
Attack12 hitbox activation evidence, attack-state `Off -> New -> Transfer ->
Interpolate`, selected contact, repeat-hit rejection, damage scheduling, and
damage-recover consumption, plus the selected shield-stat -> GuardSetOff
branch and sibling hitbox `0` decode/contact-gate probe. Current marker:
`mpLiveHit=atk=191/166 hitbox=1/j14 dmg=4 second=0/j14/x140 shield=4->4/4 so=155/134 contact=1 repeat=1 rehit=5->0 origRehit=hit30/v40000 30->29->0 clear=1/3 dmg=0->4 hitlag=6`.
Full `gmcollision.c`, full `ftmain.c`, multi-slot natural hitbox runtime,
continuous shield runtime beyond the selected set-off branch, continuous rehit
gameplay beyond the selected Link down-air timer window, arbitrary damage-state
duration, complete hitlag/damage runtime, items/weapons, HUD, audio, and
unbounded gameplay scheduling remain deferred.

Completed 2026-06-29: bounded source-shaped hit-to-damage-to-recovery
lifecycle proof shipped as modes `157/158`. The direct/menu-chain pair keeps
the live VSBattle roots on Pupupu/Dream Land, inherits the Passive recover
aggregate plus Dash-Run selected contact/damage setup proof, then proves one
selected contact, damage scheduling, hitlag callback reachability, damage
status setup, DamageFall map recovery selection, PassiveStand/Passive/
DownBounce branch reachability, and final safe floor state. Current marker:
`mpDamageRecover=contact=1/1 dmg=0->4 hitlag=6 status=52/45 fall=57/50 ps=1 passive=1 dbounce=1`.
Full live hitbox collision runtime, full `gmcollision.c`, full `ftmain.c`,
arbitrary damage-state duration, complete hitlag/damage runtime, items/weapons,
HUD, audio, and unbounded gameplay scheduling remain deferred.

Completed 2026-06-28: bounded original ThrownCommon animation-end branch proof
folded into modes `155/156`. It reuses imported `ftcommonthrown1.c`, runs the
installed original `ftCommonThrownProcUpdate` with `anim_frame <= 0`, reaches
original `ftCommonThrownSetStatusImmediate` through the project-owned
`ftMainSetStatus` seam, preserves ThrownCommon/Air, records the animation-event
and capture-immune seams, and reports `throwCb=1/1/1 floor=3 end=1/186/161`.
Continuous thrown animation, release scheduling, item throw branches, and
player-driven grab gameplay remain deferred.

Completed 2026-06-28: bounded original ThrownCommon callback proof folded
into modes `155/156`. It reuses imported `ftcommonthrown1.c`, runs one
installed original ThrownCommon update/physics/map callback slice, preserves
ThrownCommon/Air, copies the captor floor line through the original map
callback, and records `throwCb=1/1/1 floor=3`. Continuous thrown animation,
release scheduling, item throw branches, and player-driven grab gameplay
remain deferred.

Completed 2026-06-28: bounded original Thrown dead-result cleanup proof folded
into modes `155/156`. It reuses imported `ftcommonthrown2.c`, seeds a
catcher/victim relationship, calls original `ftCommonThrownDecideDeadResult`,
proves the original lose-grip path reaches collision-default and SetAir seams,
resolves catcher/victim through Wait/Fall, clears catch/capture pointers, and
records `throwDead=call=1 coll=1 air=1 waitFall=2 status=10/26`. Continuous
death/throw scheduling and player-driven grab gameplay remain deferred.

Completed 2026-06-28: bounded original Thrown proc-status callback tick folded
into modes `155/156`. It reuses the imported `ftcommonthrown2.c` boundary,
ticks the installed original `ftCommonThrownProcStatus` callback once, proves
the project-owned `ftParamSetThrowParams` seam, and records
`throwProc=param=1 script=123`. Continuous thrown/damage status scheduling,
item throw branches, and player-driven grab gameplay remain deferred.

Completed 2026-06-28: bounded original Thrown damage/no-damage release status
proof folded into modes `155/156`. It reuses the imported
`ftcommonthrown2.c` boundary, calls original `ftCommonThrownSetStatusDamageRelease`,
`ftCommonThrownUpdateDamageStats`, and `ftCommonThrownSetStatusNoDamageRelease`
through guarded project-owned wrappers, and proves
`throwReleaseStatus=dmg=20->26 upd=30->36 noDmg=40->40`. Continuous throw
release scheduling, item throw branches, full damage runtime, and
player-driven grab gameplay remain deferred.

Completed 2026-06-26: Downed recovery continuation shipped as modes `129/130`.
It proves face-down DownWaitD into DownStandD, DownAttackD, DownForwardD, and
DownBackD, then returns each branch through the original Wait/Ground handoff.

Completed 2026-06-26: Ledge occupancy/release/drop/climb aggregation shipped as
modes `131/132`. It proves same-cliff occupancy blocks a second catch, drop
clears cliff hold into Fall/Air with `cliffcatch_wait=30`, recatch succeeds
after release, and CliffClimbQuick2 finishes through the original Wait/Ground
handoff.

Completed 2026-06-26: Ledge wait/drop/climb selected live-callback proof
shipped as modes `133/134`. It drives a proof-owned P0 `GObjProcess` through
original CliffCatch -> CliffWait -> CliffQuick -> CliffClimbQuick1 ->
CliffClimbQuick2, one guarded common2 update/physics/map tick, finish into
Wait/Ground, and reseeded CliffWait drop into Fall/Air with source mask
`0xfff`.

Completed 2026-06-26: Pupupu wall-hit geometry scout added to the existing
`mpwall_floor_loop` verifiers. It scans all currently loaded Dream Land floor
lines through the source-order `mpProcessRunFloorEdgeAdjust` path while
restoring older MP-adjust counters, and reports no valid non-edge wall-hit
case: `floors=4`, `walls=8`, `candidates=6`, `hits=0`.

Completed 2026-06-26: Hyrule Castle wall-hit scout added to the same
`mpwall_floor_loop` verifiers. It stages `GRHyruleMap`, `StageCastle`, and
`ExternDataBank113` as isolated scout assets and reports a concrete
source-order wall-hit candidate: floor `5`, wall `13`, edge-under `12`,
left side, adjustment delta `-1600/-388`.

Completed 2026-06-26: Hyrule Castle wall-hit proof promoted as modes
`135/136`. The new direct/menu-chain pair keeps the live battle scene on
Pupupu/Dream Land, inherits cliff-live, preserves the Dream Land wall-blocker
diagnostics, and promotes the isolated Hyrule candidate into a bounded
source-order wall-hit proof without enabling full Hyrule stage runtime.

Completed 2026-06-26: live selected-callback wall-hit copyback proof shipped as
modes `137/138`. The new direct/menu-chain pair consumes the Hyrule wall-hit
and cliff-live proofs, keeps the live scene on Pupupu/Dream Land, installs a
proof-owned P0 process, copies the adjusted Hyrule collision result back into
the selected P0 root/collision shell, and proves P1 root state is unchanged.

Completed 2026-06-27: pass-through floor proof shipped as modes `139/140`.
The direct/menu-chain pair consumes the Hyrule wall-hit and wall-copy proofs,
keeps the live scene on Pupupu/Dream Land, proves Dream Land line `0` carries
`MAP_VERTEX_COLL_PASS` flags `0x4000`, routes through `MAP_PROC_TYPE_PASS`,
rejects same-line `ignore_line_id`, accepts the different-line pass-through
probe through the pass callback gate, and leaves P1 root unchanged.

Completed 2026-06-27: platform floor scout shipped as modes `141/142`. The
direct/menu-chain pair consumes the pass-through proof, calls original
`mpCollisionCheckExistPlatformLineID` for Dream Land line `0`, and records the
inactive blocker. With the current bounded indexed DObj shell, the regression
value is yakumono id `1`, yakumono count `1`, DObj present but status off,
predicate `0`, blocker `0x40`.

Completed 2026-06-27: minimal yakumono DObj activation proof shipped as modes
`143/144`. The direct/menu-chain pair keeps the live battle scene on
Pupupu/Dream Land, preserves the inactive `141/142` blocker as regression
evidence, installs a bounded original-compatible yakumono DObj for Dream Land
line `0`, sets status on, then proves original
`mpCollisionCheckExistPlatformLineID` reports the platform predicate active
with blocker `0`.

Completed 2026-06-27: platform status/update-tic proof shipped as modes
`145/146`. The direct/menu-chain pair consumes the active platform predicate,
routes Dream Land line `0` / yakumono id `1` through the project-owned
`mpCollisionSetYakumonoOnID` helper, runs one guarded original-compatible
`mpCollisionAdvanceUpdateTic` call, and proves the DObj remains active while
the update tic advances `0 -> 1` without unsafe escape.

Completed 2026-06-27: natural drop-through input proof shipped as modes
`147/148`. The direct/menu-chain pair consumes the platform-tick proof, imports
original `ftcommonpass.c` and `ftcommonsquat.c`, seeds original-compatible
down input on Dream Land pass-through line `0`, then proves Wait -> Squat ->
Pass with status `10 -> 28 -> 33`, tap Y `0 -> 254`, ignored line `0`, and
pass wait `3 -> 0`.

Completed 2026-06-27: yakumono position/speed primitive proof shipped as modes
`149/150`. The direct/menu-chain pair consumes the pass-input proof, runs
project-owned `mpCollisionSetYakumonoPosID` with original BattleShip semantics
on Dream Land line `0` / yakumono `1`, and proves target translation,
`gMPCollisionSpeeds` delta `12000/-4000/2000`, status `1 -> 1`, platform
predicate `1`, and no unsafe escape.

Completed 2026-06-27: platform speed-reader plus dynamic floor/ceil/wall collision
speed-consumer proof shipped as modes `151/152`. The direct/menu-chain pair
consumes the position proof, calls the original-compatible
`mpCollisionGetSpeedLineID` reader for Dream Land line `0`, then runs one
bounded `mpCollisionCheckFloorLineCollisionDiff` probe through the original
active-yakumono coordinate transform. It proves the line-to-yakumono lookup
returns yakumono `1`, speed `12000/-4000/2000`, and dynamic marker `dyn=1/2`
with no unsafe escape. It also runs one same-yakumono
`mpCollisionCheckCeilLineCollisionDiff` probe, recorded as `ceil=1/1`.

Completed 2026-06-27: aligned the bounded project-owned ceil same/diff sweep
helper with the same original active-yakumono local-space transform used by
BattleShip `mpCollisionCheckCeilLineCollisionDiff`, then promoted one
same-yakumono Dream Land dynamic ceiling probe into modes `151/152`.

Completed 2026-06-27: promoted one bounded `mpCollisionPlayYakumonoAnim`
slice into modes `151/152` without adding a new harness. The proof seeds one
controlled yakumono DObj animation track set, runs original
`gcParseDObjAnimJoint` / `gcPlayDObjAnimJoint` through the gated compatibility
hook, advances the MP update tic once, and records `anim=1` with speed
`12000/-4000/2000`.

Completed 2026-06-27: added project-owned
`mpCollisionCheckLWallLineCollisionDiff` / `RWall` wrappers to the same
bounded wall sweep helper, using the original active-yakumono local-space
transform. Modes `151/152` now run one controlled same-yakumono Dream Land wall
probe and record `wall=1/1`.

Completed 2026-06-27: routed a bounded original MP wall caller through that
wall diff seam without adding a new harness. Modes `151/152` now run one
first-probe slice of `mpProcessCheckTestL/RWallCollisionAdjNew`, record
`procwall=1/1`, and leave the original edge/ceil/floor follow-up branches
deferred.

Completed 2026-06-27: added original-compatible MP bounds current/diff
recompute after the bounded yakumono animation tick, without adding a new
harness. Modes `151/152` now call `mpCollisionUpdateBoundsCurrent` and
`mpCollisionUpdateBoundsDiff`, record `bounds=1`, and require the
platform-speed proof mask `0x3fff`.

Completed 2026-06-27: added a stage-authored layer-animation diagnostic to the
same modes. The proof records `stageanim=0/0x91`, which means the Dream Land
layer/root and selected yakumono parent are present and the bounded callback
ran once, but the selected Pupupu layer does not expose an authored layer-1
animation table/process.

Completed 2026-06-27: added the first Inishie dependency preflight to the same
modes. The proof stages `GRInishieMap`, resolves `llGRInishieMapMapHeader` at
`0x14`, and reports `inishieAsset=header/geometry nodes=1`. This is not the
scale-platform import yet.

Completed 2026-06-27: Inishie scale moving-yakumono proof shipped as modes
`153/154`. The direct/menu-chain pair keeps the live battle roots on
Pupupu/Dream Land, runs source-backed original `grInishieMakeScale`, then runs
two bounded original `grInishieScaleProcUpdate` ticks. It records line groups
`1/2`, map object kinds `5/6`, altitude `80000 -> 64000`, platform Y
`363000/362000 -> 427000/298000`, second-tick speed `-8000/8000`, a forced
bounded `Wait -> Fall -> Sleep -> Retract -> Wait` threshold path
(`fall=1->2/0`, `step=3->0/0`), and clean source-created DL/material/preview
markers.

Completed 2026-06-27: PassiveStand/Passive recover-loop proof shipped as modes
`155/156`. The direct/menu-chain pair keeps the live battle roots on
Pupupu/Dream Land, inherits the source-order MP floor, cliff, DamageFall, and
previous Passive-loop proofs, then runs five guarded stable update/physics/map
frames for PassiveStandF/Ground, PassiveStandB/Ground, and Passive/Ground
before proving the original `ftAnimEndSetWait` handoff into Wait/Ground.
Current markers are
`mpPassive=ps=73/62->10/4 ticks=6/5/5 passive=81/70->10/4 ticks=6/5/5
branch=0x7f psb=0xf natural=0x7f mapcalls=2`. The pair now also proves the
PassiveStandB/Ground stable-frame and Wait/Ground handoff mask, plus the
imported original DamageFall map callback selecting PassiveStandF and Passive
through the installed `FTStruct.proc_map` callback and bounded source-order MP
floor collision path.

Completed 2026-06-28: bounded Appeal/Taunt status proof stayed within modes
`155/156`. The current direct/menu-chain Passive recovery pair now imports
original `ftcommonappeal.c`, seeds one L-button tap from Wait/Ground, calls the
original Appeal interrupt gate, and proves Appeal/Ground status/motion
`189/164` with callback shape and cleanup diagnostics. This is status-only;
continuous Appeal animation/event/runtime behavior remains deferred.

Completed 2026-06-28: bounded Appeal/Taunt callback/update handoff proof
stayed within modes `155/156`. After the Appeal status proof, the pair now
ticks the installed original Appeal interrupt callback once with no catch/guard
branch, then runs the installed `ftAnimEndSetWait` update slot back to
Wait/Ground. Continuous Appeal animation/event/runtime behavior remains
deferred.

Completed 2026-06-28: bounded Appeal/Taunt catch-fails-then-GuardOn branch
proof stayed within modes `155/156`. After the no-branch Appeal callback and
Wait handoff, the pair re-enters Appeal through the imported original gate,
sets the original `motion_vars.flags.flag1` branch condition, proves the
project-owned catch seam returns false, and routes through imported original
GuardOn setup to status/motion `152/134` before restoring Wait/Ground.
Continuous Appeal and shield runtime remain deferred.

Completed 2026-06-28: latest Passive recover modes now also enable the
existing bounded original DownWait proof. Modes `155/156` continue from the
recovered Wait/Ground boundary into DownWaitU/Ground, DownStandU, DownAttackU,
DownForwardU, and DownBackU source-order branches and their Wait/Ground
handoffs. Continuous downed-action gameplay remains deferred.

Completed 2026-06-28: latest Passive recover modes now also enable the
existing bounded original Turn proof. Modes `155/156` continue from the
recovered Wait/Ground boundary into Turn/Ground `18/12`, prove the installed
original update callback flips facing and ground velocity, then hand off back
to Wait/Ground. Continuous player-driven Turn movement remains deferred.

Completed 2026-06-28: latest Passive recover modes now also enable the
existing bounded original face-down DownRecover proof. Modes `155/156` continue
from the recovered/Turn-proven Wait boundary into DownWaitD/Ground,
DownStandD, DownAttackD, DownForwardD, and DownBackD source-order branches and
their Wait/Ground handoffs. Continuous face-down downed-action runtime remains
deferred.

Completed 2026-06-28: latest Passive recover modes now also enable the
existing bounded CliffLedge aggregation proof. Modes `155/156` prove same-cliff
occupancy blocks a second catch, ledge drop clears cliff hold into Fall/Air
with `cliffcatch_wait=30`, recatch succeeds after release, and
CliffClimbQuick2 finishes through the original Wait/Ground handoff. Continuous
natural ledge runtime remains deferred.

Completed 2026-06-28: latest Passive recover modes now also enable the
existing bounded CliffLive proof. Modes `155/156` drive a proof-owned selected
P0 GObj process through original CliffCatch -> CliffWait -> CliffQuick ->
CliffClimbQuick1 -> CliffClimbQuick2, one guarded common2 update/physics/map
tick, Wait/Ground finish, and a reseeded CliffWait drop into Fall/Air with
source mask `0xfff`. Continuous natural ledge runtime remains deferred.

Completed 2026-06-28: latest Passive recover modes now also enable the
existing bounded Hyrule wall-hit floor proof. Modes `155/156` validate the
selected source-order MP wall-line/floor-edge scout relationship
(`floor=5`, `wall=13`, `edge=12`, `side=0`, `delta=-1600/-388`) while keeping
natural wall collision/copyback runtime deferred.

Completed 2026-06-28: latest Passive recover modes now also enable the
existing bounded wall-copy proof for the selected Hyrule wall-hit probe.
Modes `155/156` prove one source-order wall collision copyback pass and
final collision mask for `floor=5`, `wall=13`, `edge=12`, with natural wall
runtime still deferred.

Completed 2026-06-28: latest Passive recover modes now also enable the
existing bounded pass-through floor proof. Modes `155/156` reject same-line
pass-through collision via `ignore_line_id`, accept the different-line probe
through the pass callback, and record
`mpPass=line=0 flags=0x4000 route=2 same=1 diff=1 cb=1/1/0`. Full
player-driven drop-through/platform gameplay remains deferred.

Completed 2026-06-28: latest Passive recover modes now also enable the
existing bounded platform-floor classification proof. Modes `155/156` check
the selected pass-through line against BattleShip's yakumono platform
predicate and record `mpPlatform=line=0 yak=1 dobj=1 status=0 anim=0
deferred=0x40`. Active/ticking/moving platform runtime remains deferred.

Completed 2026-06-28: latest Passive recover modes now also enable the
existing bounded active platform-floor proof. Modes `155/156` install the
bounded yakumono DObj for Dream Land line `0`, set the original-compatible
status active, and record `mpPlatform=line=0 yak=1 dobj=1 status=1 anim=0
active`. Platform ticking, movement, and speed transfer remain deferred.

Completed 2026-06-28: latest Passive recover modes now also enable the
existing bounded platform-tick proof. Modes `155/156` run one guarded
`mpCollisionAdvanceUpdateTic` step for the active Dream Land yakumono and
record `mpPlatformTick=tic=0->1 setOn=1 advance=1 status=1/1`. Platform
movement and speed transfer remain deferred.

Completed 2026-06-28: latest Passive recover modes now also enable the
existing bounded natural drop-through input proof. Modes `155/156` seed
original-compatible down input on Dream Land pass-through line `0`, route
Wait -> Squat -> Pass through imported original `ftcommonpass.c` /
`ftcommonsquat.c`, and record
`mpPassInput=line=0 flags=0x4000 squat=28 pass=33 ignore=0`. Moving-platform
pass-through and continuous platform gameplay remain deferred.

Completed 2026-06-28: the existing direct/menu-chain pass-input proof now also
reuses imported original `ftcommonsquat.c` to prove SquatWait -> SquatRv ->
Wait through installed original callback slots. The latest Passive recover
modes inherit the upgraded marker
`mpPassInput=line=0 flags=0x4000 squat=28 pass=33 rv=29->30->10 ignore=0`.
Continuous crouch movement and full ground interrupt scanning remain deferred.

Completed 2026-06-28: latest Passive recover modes now also enable the
existing bounded platform-position proof. Modes `155/156` call
`mpCollisionSetYakumonoPosID` for Dream Land line `0` / yakumono `1` and
record `mpPlatformPos=line=0 yak=1 delta=12000/-4000/2000`. Continuous
platform movement and live speed transfer remain deferred.

Completed 2026-06-28: latest Passive recover modes now also enable the
existing bounded platform-speed proof. Modes `155/156` read the active
yakumono speed through `mpCollisionGetSpeedLineID`, then run the bounded
dynamic floor/ceil/wall, wall-process, animation, bounds, and stage-animation
diagnostic slices. Current marker:
`mpPlatformSpeed=line=0 yak=1 read=12000/-4000 dyn=1/2 ceil=1/1 wall=1/1
procwall=1/1 anim=1 bounds=1 stageanim=0/0x91 inishieAsset=header/geometry
nodes=1`. Continuous moving-platform gameplay remains deferred.

Completed 2026-06-28: latest Passive recover modes now also enable the
existing bounded Inishie/Mushroom Kingdom scale proof. Modes `155/156` stage
read-only `StageInishieFile3`, run source-backed `grInishieMakeScale` /
`grInishieScaleProcUpdate`, and record
`inishieScale=ticks=2 lines=1/2 alt=80000->64000
y=363000/362000->427000/298000 speed=-8000/8000 fall=1->2/0
step=3->0/0 sourceDL=0xff cmds=91 tris=20 tex=0x3f preview=0x3f px=432`.
Full Mushroom Kingdom runtime and hardware-backed rendering remain deferred.

Completed 2026-06-28: bounded Guard EscapeF/EscapeB status proof stayed within
the older dash-run modes. The direct/menu-chain Dash -> Run -> RunBrake pair
now imports original `ftcommonescape.c`, reuses the verified GuardOn state,
calls original `ftCommonEscapeCheckInterruptGuard`, and proves EscapeF/EscapeB
status/motion `156/136` and `157/137` with callback mask `0x3ff`, state mask
`0xff`, and `itemthrow_buffer_tics=5`.

Completed 2026-06-28: the same Escape proof now also ticks the installed
original Escape callbacks once per fighter. It proves `tick=0x3ff`:
`ftCommonEscapeProcUpdate` consumes `motion_vars.flags.flag1` and flips LR
while animation time is positive, `ftCommonEscapeProcInterrupt` reaches the
light-throw seam, installed physics/map callbacks reach bounded compatibility
seams, and animation end returns through original `ftCommonWaitSetStatus` to
Wait/Ground. Continuous shield-roll runtime and player-driven guard escape
remain deferred.

Completed 2026-06-28: bounded GuardOn update tick proof stayed within the
older dash-run modes. The pair now runs one installed original
`ftCommonGuardOnProcUpdate` callback with animation time still positive,
keeps GuardOn status/motion `152/134`, and proves shield decay/release counters
advance once through the original shield-var path. Full Guard/GuardOff/SetOff
runtime remains deferred.

Completed 2026-06-28: bounded GuardOn animation-end handoff proof stayed
within the older dash-run modes. The pair now runs a second installed original
`ftCommonGuardOnProcUpdate` callback at animation end, reaches original
`ftCommonGuardSetStatus`, enters Guard status `153` while preserving GuardOn
motion `134`, and installs original Guard update/interrupt/physics/map
callbacks. Full Guard hold, GuardOff, SetOff, shield collision, and
player-driven guard runtime remain deferred.

Completed 2026-06-28: bounded Guard hold update tick proof stayed within the
older dash-run modes. After the GuardOn -> Guard handoff, the pair now runs
one installed original `ftCommonGuardProcUpdate` callback, stays in Guard
status `153`, preserves shield state, and advances shield decay/release
counters again before the existing Guard -> Escape proof. Continuous Guard
hold, GuardOff, SetOff, shield collision, and player-driven guard runtime
remain deferred.

Completed 2026-06-28: bounded Guard release -> GuardOff status proof stayed
within the older dash-run modes. The pair now releases Z during the installed
original `ftCommonGuardProcUpdate` callback, reaches original
`ftCommonGuardOffSetStatus`, enters GuardOff status/motion `154/135`, installs
the original GuardOff update/physics/map callbacks, then restores Guard through
original `ftCommonGuardSetStatus` before the existing Guard -> Escape proof.
GuardOff completion to Wait, SetOff, shield collision, and player-driven guard
runtime remain deferred.

Completed 2026-06-28: bounded GuardOff completion proof stayed within the
older dash-run modes. After the Guard release -> GuardOff status proof, the
pair now runs one installed original `ftCommonGuardOffProcUpdate` callback at
animation end, reaches original `ftCommonWaitSetStatus`, returns to
Wait/Ground, then restores Guard through original `ftCommonGuardSetStatus`
before the existing Guard -> Escape proof. Continuous Guard hold, SetOff,
shield collision, and player-driven guard runtime remain deferred.

Completed 2026-06-28: latest Passive recover modes now also assert the
existing bounded Dash-Run attack/guard aggregate proof. Modes `155/156` record
`dashRun=0x7ffffff`, keeping the older bounded Attack11/Attack12/Mario
Attack13/Fox Attack100, AttackDash, GuardOn/Guard/GuardOff, and EscapeF/
EscapeB status/callback/update slices covered by the latest VSBattle/Pupupu
root. Full attack hitboxes, continuous Attack100/Guard runtime, shield
collision, and player-driven attack/shield gameplay remain deferred.

Completed 2026-06-28: bounded GuardSetOff proof stayed within the older
direct/menu-chain dash-run modes and is inherited by current modes `155/156`.
The proof drives imported original `ftCommonGuardSetOffSetStatus` and the
installed original `ftCommonGuardSetOffProcUpdate`, verifies deterministic
setoff frames/velocity (`20200/-40400`), proves held Z returns to Guard and
released Z returns to GuardOff, then restores Guard for the existing Escape
proof. At that point the aggregate marker became `dashRun=0x1fffffff`.
Continuous
Guard/SetOff/Escape shield runtime, shield collision, and player-driven guard
gameplay remain deferred.

Completed 2026-06-28: bounded original TurnRun proof stayed within the older
direct/menu-chain dash-run modes and is inherited by current modes `155/156`.
The proof imports `ftcommonturnrun.c`, drives Run -> TurnRun -> Run through
the installed original Run interrupt path, verifies TurnRun status/motion
`19/13`, final Run `16/10`, callback/update masks `0xff/0xf`, and LR/ground
velocity flips. The aggregate marker is now `dashRun=0x3fffffff`. Continuous
player-driven TurnRun movement remains deferred.

Completed 2026-06-28: current-safe CliffAttack/Common2 promotion now folds the
existing bounded CliffAttack floor/action/Common2 proofs into modes `155/156`.
The aggregate keeps later wall-copy/platform/Inishie finalizers publishing by
running the cliff finalizers after the current tail, reseeding the delayed
CliffAttack probe back into original-compatible CliffWait, and resetting shared
Common2 bridge diagnostics before the CliffAttack action proof.

Completed 2026-06-28: current-safe CliffEscape/Common2 promotion now folds the
existing bounded CliffEscape action/Common2 proofs into modes `155/156` using
the same delayed-reseed and bridge-isolation pattern. The aggregate now proves
CliffWait -> CliffQuick -> CliffEscapeQuick1 -> CliffEscapeQuick2 plus one
guarded Common2 update/physics/map tick under both the direct and menu-chain
current Passive recover boundaries.

Completed 2026-06-28: bounded attack animation-event decoder proof stayed
within the older direct/menu-chain dash-run modes. The existing
`ftMainPlayAnimEventsAll` seam now installs selected original Mario/Fox
main-motion command extracts, decodes ten original-layout `MakeAttackColl`
commands into the local `FTStruct.attack_colls[0]` / `[1]` shadows, proves Fox
Attack100Start as a real no-hit script, and verifies
`DASH_RUN_ATTACK_EVENT=0x1f/0x3f/0x20/10`.

Completed 2026-06-28: bounded selected command-mutator scan stayed within the
same older dash-run harness pair. The scanner now also applies Mario Jab3's
original `SetAttackCollDamage`, `SetAttackCollSize`, and `ClearAttackCollAll`
command words, then verifies `DASH_RUN_ATTACK_EVENT_CMDS=0xf` for damage,
size, clear, and clear-off state. Full animation command runtime,
staled-damage handling, hitbox activation, hit detection, and hitlag/damage
interaction remain deferred.

Completed 2026-06-28: bounded original WallDamage status/update proof stayed
within current modes `155/156`. The current Passive recover pair now imports
original `ftcommonwalldamage.c`, seeds the selected Hyrule wall-hit source,
routes through original `ftCommonWallDamageCheckGoto`, proves reflected
knockback into WallDamage status/motion `56/49`, records effect/quake/rumble/
intangible compatibility seams, and ticks original
`ftCommonWallDamageProcUpdate` once into DamageFall `57/50`. Natural wall
collision, wall teching, and continuous damage runtime remain deferred.

Completed 2026-06-28: bounded original Catch status proof stayed within
current modes `155/156`. The current Passive recover pair now imports original
`ftcommoncatch1.c`, seeds Wait/Ground Z-hold plus A-tap, proves original
`ftCommonCatchCheckInterruptCommon` accepts the input and reaches
Catch/Ground `166/146`, records catch-param setup, and keeps item throw,
CatchPull, CapturePulled, and throw runtime deferred.

Completed 2026-06-28: bounded original Catch callback handoff proof stayed
within current modes `155/156`. The same pair now runs the installed original
Catch map callback once, then forces the installed original Catch update
callback through `ftAnimEndSetWait` back to Wait/Ground `10/4`, while keeping
CatchPull, CapturePulled, item throw, throw runtime, and player-driven grab
gameplay deferred.

Completed 2026-06-28: bounded original CatchPull/CatchWait proof stayed
within current modes `155/156`. The same pair now imports original
`ftcommoncatch2.c`, drives the bounded original CatchPull proc-catch path into
CatchPull/Ground `167/147`, ticks original CatchPull update into
CatchWait/Ground `168/-2`, and ticks CatchWait interrupt once with the
throw-check seam stubbed false and `throw_wait 60->59`. CapturePulled, throw
execution, item throw branches, continuous grab runtime, and player-driven
grab gameplay remain deferred.

Completed 2026-06-28: bounded original CapturePulled/CaptureWait proof stayed
within current modes `155/156`. The same pair now imports original
`ftcommoncapturepulled.c` and `ftcommoncapturewait.c`, drives the seeded victim
through original CapturePulled/Ground `171/150`, ticks original
CapturePulled physics into CaptureWait/Ground `172/-2`, and runs one installed
CaptureWait map callback through a narrow project-owned
`mpCommonSetFighterProjectFloor` seam. Throw execution, item throw branches,
continuous grab/capture runtime, player-driven grab gameplay, and full
`ftmain.c` remain deferred.

Completed 2026-06-28: bounded original ThrowF/ThrownCommon handoff proof
stayed within current modes `155/156`. The same pair now imports original
`ftcommonthrow.c` and `ftcommonthrown1.c`, drives original
`ftCommonThrowCheckInterruptCatchWait` into ThrowF/Ground `169/148`, queues
the seeded victim into ThrownCommon/Air `186/161`, records the throw
anim-event/capture-immune seams, and verifies the new
`STAGE_MPPASSIVE_THROW` marker through both direct and menu-chain boundary
paths. Throw release/damage runtime, item throw branches, continuous
grab/capture/throw runtime, player-driven grab gameplay, and full `ftmain.c`
remain deferred.

Completed 2026-06-28: bounded original Thrown release/update-stats proof
stayed within current modes `155/156`. The same pair now imports original
`ftcommonthrown2.c`, seeds an original-compatible throw hit descriptor, calls
`ftCommonThrownReleaseThrownUpdateStats`, proves victim damage `10->18`, clears
capture state, forces Air, installs the thrown proc-status callback, and
records the damage/stat/stale/rumble compatibility seams through the new
`STAGE_MPPASSIVE_THROW_RELEASE` marker on both direct and menu-chain boundary
paths. Later bounded release-status and proc-status tick proofs now cover the
next `ftcommonthrown2.c` slices; item throw branches, continuous
grab/capture/throw runtime, full damage runtime, player-driven grab gameplay,
and full `ftmain.c` remain deferred.

Completed 2026-06-28: bounded original ThrowB branch proof stayed within
current modes `155/156`. The same pair now reseeds original
`ftCommonThrowCheckInterruptCatchWait` with stick-left input after the existing
ThrowF proof and reports `throwB=170/149->186/161`. Item throw branches,
continuous grab/capture/throw runtime, full damage runtime, player-driven grab
gameplay, and full `ftmain.c` remain deferred.

Completed 2026-06-28: bounded original ThrowF update release proof stayed
within current modes `155/156`. The same pair now ticks installed original
`ftCommonThrowProcUpdate` with `flag2` set and proves the real release branch
calls imported original thrown update-stats, reporting
`throwUpdate=169/148 dmg=50->58 script=0`. Continuous throw animation
scheduling, item throw branches, full damage runtime, player-driven grab
gameplay, and full `ftmain.c` remain deferred.

| Rank | Candidate | Status | Source Files | Expected Harness Names | Risk | Verification | Owner / Notes |
|---:|---|---|---|---|---|---|---|
| 1 | PassiveStand/Passive recover-loop proof | Completed | `src/import/battleship_ftcommon_passivestand.c`, `src/import/battleship_ftcommon_passive.c`, `src/import/battleship_ftcommon_damagefall.c`; original `ftcommonpassivestand.c`, `ftcommonpassive.c`, `ftcommondamagefall.c` | `battle_mariofox_stage_mppassive_recover_loop`, `menu_chain_mariofox_stage_mppassive_recover_loop` | Medium: longer bounded recovery runtime touches imported update/physics/map callbacks and final Wait handoff. | `verify-boundary.ps1 -DelaySeconds 3` and `verify-current.ps1 -DelaySeconds 3` passed. | Shipped as modes `155/156`; fully live unseeded recovery selection and full fighter gameplay remain deferred. |
| 2 | CliffEscape/Common2 current-safe promotion | Completed | Existing `src/import/battleship_ftcommon_cliffescape.c`; original `ftcommoncliffescape.c` | Folded into `battle_mariofox_stage_mppassive_recover_loop`, `menu_chain_mariofox_stage_mppassive_recover_loop` | Medium: shared selected-fighter state can starve later aggregate finalizers. | Direct/menu current pair and `verify-current.ps1 -DelaySeconds 3` passed. | Shipped with delayed reseed and shared bridge reset; continuous ledge escape runtime remains deferred. |
| 3 | WallDamage current-boundary proof | Completed | `src/import/battleship_ftcommon_walldamage.c`; original `ftcommonwalldamage.c` | Folded into `battle_mariofox_stage_mppassive_recover_loop`, `menu_chain_mariofox_stage_mppassive_recover_loop` | Medium: source status setup reflects velocity, touches damage status vars, and uses effect/quake/rumble/intangible seams. | Direct/menu current pair passed with `wallDamage=56/49->57/50`. | Shipped as bounded status/update proof; natural wall collision/teching remains deferred. |
| 4 | Rebound current-boundary proof | Completed | `src/import/battleship_ftcommon_rebound.c`; original `ftcommonrebound.c` | Folded into `battle_mariofox_stage_mppassive_recover_loop`, `menu_chain_mariofox_stage_mppassive_recover_loop` | Low-medium: compact status pair but adds `hit_lr` and rebound status vars to the fighter shell. | Direct/menu current pair passed with `rebound=82/-1->83/71->10/4`. | Shipped as bounded status/update proof; real attack/shield rebound triggers remain deferred. |
| 5 | Next bounded gameplay spine step | Candidate | TBD after source inspection | TBD | Medium-high until source path is chosen. | Start with direct/menu harness plan, then boundary/current verification. | Keep it narrower than full gameplay, full map collision, full stage runtime, or broad renderer work. |
