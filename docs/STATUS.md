# Current Status

This is the short current-truth document for active development. Keep
`docs/PORTING.md` as append-only history; use this file and `docs/HANDOFF.md`
for planning and handoff.

## Project Hygiene

Harness count is intentional and verifier-driven; do not remove historical
direct/menu-chain gates just to reduce apparent bloat. Generated build
directories, root ROM/ELF outputs, emulator logs, and local emulator payloads
are not source truth. Use the tiered verifier workflow for iteration and the
Lean snapshot exporter for review handoff:

```powershell
.\scripts\verify-dev-fast.ps1
.\scripts\verify-boundary.ps1
.\scripts\verify-current.ps1 -Build -DelaySeconds 3
.\scripts\verify-regression.ps1
.\scripts\clean-generated.ps1 -DryRun
.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean
```

Use `-Mode CodeOnly` for tiny review-only snapshots that intentionally exclude
`decomp/`. Use `-Mode Full` only for explicit debugging/repro needs.
`scripts/New-Smash64DSSnapshot.Legacy.ps1` is the preserved old broad exporter
for fallback/debug reference, not the normal handoff path.
Lean snapshots keep the imported-source context needed for review and build
repro: BattleShip decomp source/includes/symbols/docs/tools, sm64-nds source
and backend reference files, and the top-level build-critical
`decomp/BattleShip-main/BattleShip_o2r` tree. They intentionally exclude
upstream decomp build outputs, baseroms, generated ROM/ELF/NDS files, duplicate
nested O2R copies, and tool caches unless `-IncludeDecompGenerated` is passed
for an explicit debug snapshot.

`verify-dev-fast.ps1` runs the latest direct current-boundary harness only.
`verify-boundary.ps1` runs the latest direct + menu-chain pair.
`verify-current.ps1` is the Latest profile wrapper: runtime + Title + latest
direct/menu-chain pair. Use `-Build` after source changes so the normal runtime
leg cannot sample a stale shared ROM. Shared runtime changes should additionally
run `verify-regression.ps1`. Full remains available as
`verify-all.ps1 -Profile Full`, but it is no longer required after every small
current-boundary task.

Parallel melonDS regression sharding is available but should stay conservative.
Create local runner slots with `.\scripts\New-MelonDSRunnerSlots.ps1 -Count 4`,
prebuild shared outputs with `.\scripts\build-verify-profile.ps1 -Profile Regression`,
then run shards with `verify-all.ps1 -Profile Regression -ShardCount 4
-ShardIndex N -RunnerSlot N -NoBuild`. Runner slots live under
`emulators/melonds-runners/`, are gitignored, and must not be committed.
Default single-runner verification remains the safest path.

Runner-slot proof on 2026-06-28: `New-MelonDSRunnerSlots.ps1 -Count 2 -List`
reported slot0/slot1 with ARM9 ports `3333/3343`, `verify-all.ps1 -Profile
Boundary -ShardCount 2 -ShardIndex 0 -RunnerSlot 0 -NoBuild -DelaySeconds 3`
passed the direct boundary shard, and the matching shard `1` / slot `1`
passed the menu-chain boundary shard. Slot logs were written under
`artifacts/emulator-logs/slot0` and `slot1`; verifier temp directories are
`artifacts/verifier-temp/slot0` and `slot1`.

## Current Boundary

The ROM boots through the original BattleShip startup path, runs the bounded
Opening Room and opening movie sequence, reaches the imported Title scene
boundary, can prove the bounded VS menu chain, and now imports a bounded
original `scvsbattle.c` VSBattle setup slice. The current Title slice loads
original `MNTitle` and
`MNTitleFireAnim` O2R files, creates the original Title actor pair, logo-fire
GObj/display boundary, fire GObj/SObj/process/display boundary, four cameras,
and initial Title vars. It normalizes the 30 original Title fire frame Sprites,
runs one guarded original Title update tick on the natural movie path, and
renders a bounded original Title sprite preview.

This is still a bounded porting boundary, not full Title/menu/gameplay.

Current Latest/Boundary pair:

```powershell
.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3
.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3
```

Latest source-port routing: public `ftCommonDamageGetDamageLevel`,
`ftCommonDamageGetKnockbackAngle`, `ftCommonDamageCheckElementSetColAnim`,
`ftCommonDamageCheckMakeScreenFlash`, `ftCommonDamageSetPublic`,
`ftCommonDamageSetDustEffectInterval`, `ftCommonDamageUpdateDustEffect`,
`ftCommonDamageDecHitStunSetPublic`, `ftCommonDamageUpdateCatchResist`,
`ftCommonDamageCheckCatchResist`, `ftCommonDamageCheckCaptureKeepHold`,
`ftCommonDamageUpdateDamageColAnim`, `ftCommonDamageSetDamageColAnim`,
selected `ftCommonDamageInitDamageVars` /
`ftCommonDamageGotoDamageStatus` setup, selected `ftMainProcParams`
Twister/proc_trap branch, and `ftCommonDamageUpdateMain` now delegate to
imported original BattleShip
`ndsBase*` wrappers; the existing
`DASH_RUN_DAMAGE_LEVELS`, `DASH_RUN_DAMAGE_KNOCKBACK_ANGLE`,
`DASH_RUN_DAMAGE_COLANIM`, `DASH_RUN_DAMAGE_SCREEN_FLASH`,
`DASH_RUN_DAMAGE_PUBLIC`, `DASH_RUN_DAMAGE_DUST`,
`DASH_RUN_DAMAGE_DUST_UPDATE`,
`DASH_RUN_DAMAGE_HITSTUN_PUBLIC`, `DASH_RUN_DAMAGE_UPDATE_CATCH_RESIST`,
`DASH_RUN_DAMAGE_HOLD_RESIST`, `DASH_RUN_DAMAGE_COLANIM_UPDATE`, and
`DASH_RUN_DAMAGE_KIND` proofs now call the public seams with stable
`level=0x1f`, `angle=0x3f`, `colAnim=0x3f`, `flash=0x7f`,
`public=0x3f`, `dust=0xff`, `dustUpdate=0x1f`, `hitPublic=0xf`,
`catchResist=0x1f`, `colAnimUpdate=0x1f`, and `dmgKind=0x7f` markers.
The `dmgKind` high bits prove the bounded Twister status forces
`nFTDamageKindColAnim` before color-animation routing and calls the installed
`proc_trap` in BattleShip source order.
The `DASH_RUN_PROCPARAMS` `ftCommonDamageUpdateMain` branches now call the
public seam and route through the imported original dispatcher for
catch/capture keep-hold, catch/capture zero-knockback, catch/capture
stats/release/no-damage status branches, no-grab/no-capture tail colanim and
damage-status/Sleep branches, plus held-item bypass/resist/drop cases. The
public thrown damage-stat and release-status seams now call their imported
original helpers during this bounded damage setup proof; the broader
lose-grip seam now routes complete seeded catch/capture links through imported
original `ftcommonthrown2.c` release/decide helpers, proving release selection,
default collision projection, invalid-floor `mpCommonSetFighterAir`, and
catch/capture link clear during this proof. Partial seeded links still fall
back to the bounded local guard.
Public `ftCommonDamageCommonProcUpdate`,
`ftCommonDamageAirCommonProcUpdate`, `ftCommonDamageFlyRollUpdateModelPitch`,
and `ftCommonDamageSetStatus` also route through imported original BattleShip
code.

Registry modes `161/162`
(`battle_mariofox_stage_mplivehit_status_loop` and
`menu_chain_mariofox_stage_mplivehit_status_loop`) keep the live VSBattle
roots on Pupupu/Dream Land and inherit the current pass-through/platform,
Inishie scale, passive/recover, wall/rebound, catch/throw, ledge, dash-run
damage setup, and `mpDamageRecover` proofs. They add a bounded selected Fox
Jab2 live-hit lifecycle proof: Attack12 status/motion `191/166`, original
event-backed hitbox metadata, attack-state `Off -> New -> Transfer ->
Interpolate`, selected contact, repeat-hit rejection, damage scheduling, and
damage-recover consumption. The same proof now runs the selected source-shaped
`ftMainSetHitInteractStats` damage/shield/attack record update path plus the
source-order hit-record detect gate,
`ftParamRefreshAttackCollID` clear path, one bounded source-shaped
`ftMainUpdateShieldStatFighter` shield-stat -> GuardSetOff branch, and one
bounded imported original Link down-air rehit-timer seed through
`ftCommonAttackAirLwProcHit` and timer tick window through
`ftCommonAttackAirLwProcUpdate`, now recording that the original refresh call
targets attack-coll slots `0` and `1`. The inherited Dash-Run attack-event
decoder now also records `hitboxIds=0x3`, and the live-hit proof records a
sibling Fox Jab2 hitbox `0` diagnostic contact-gate probe
(`second=0/j14/x140`) plus a bounded source-order Mario hurtbox scan that
proves the global hitstatus and damage-detect gates, temporarily skips slots
`0` and `1` as intangible, tests and misses slot `2`, reaches slot `3`,
and proves Mario's `10` active
damage-coll slots stop at the source-order `None` sentinel. The
same selected slot now feeds a bounded fighter-only `ftMainSearchHitFighter`
branch before the existing source-shaped `ftMainUpdateDamageStatFighter` front
half for attack-record, captured-damage, queue/lag, hitlog, stale-queue, and
hit-SFX table handling, then a bounded fighter-hitlog branch of
`ftMainProcessHitCollisionStatsMain` for damage
angle/element/LR/player/object/joint/index/knockback stats before the existing
percent-damage and hitlag scheduling handoff restores fighter state. The same
restored consume proof now also reruns the unmasked source-order
`ftMainProcSearchHitAll` path against Mario damage-coll slot `0`, proving the
natural first eligible hurtbox can be recorded, queued, consumed into percent,
and scheduled for hitlag without the proof-local slot `0` / `1` intangible
setup used for the selected slot-3 diagnostic; an immediate second unmasked
search is then rejected by the same source-order attack-record gate.
The same selected damage branch now also proves the original
non-normal hitstatus effect/SFX path, damage-resist false-return
effect/SFX path without queueing damage, damage-resist breakthrough
queue/lag path, and source-shaped throw-owner hitlog attribution branch. The
selected shield branch now also proves the source-order
`is_shield` / damage-detect gate, bounded shield sphere contact,
`ftMainUpdateShieldStatFighter` handoff, attack-record clear, adjacent shield-off and damage-detect-off skips,
proof-local state restore, a bounded source-shaped shield-health decrement
and ShieldBreakFly branch from `ftMainProcParams`, a bounded source-shaped
normal shield-contact common tail clear including special/rebound transients
and `hitlag_mul` reset,
a bounded source-shaped shield-heal branch, and a bounded source-shaped
original `ftCommonGuardSetOffProcUpdate` tick proof that stays in GuardSetOff
when Z is held and exits to GuardOff when Z is released, plus a bounded
source-shaped
same-group attack-record carry/clear probe through proof-only attack-coll
slots with state restore, and same-victim shield-contact repeat rejection
through the attack-record gate without new shield collision/stat/effect/
damage changes. The proof also runs a bounded source-shaped
attack-vs-attack clash branch through `ftMainUpdateAttackStatFighter` and
`ftMainSetHitRebound` for both fighters, plus a bounded source-shaped
`ftMainUpdateCatchStatFighter` distance/search tail proof plus a bounded
source-shaped `ftMainSearchFighterCatch` pre-stat gate through record/status/
grabbable/collision filters, same-victim catch-search repeat rejection, a
restored natural slot-0 catch search, and the self, ghost, Boss, same-team,
capture-immune, and global target hitstatus gates before state restore. The
selected
damage scheduling slice remains intentionally selected on Fox Jab2 hitbox `1`.
The inherited Dash-Run damage proof now also records bounded source-shaped
`ftMainProcParams` normal and BatSwing4 attack-damage rumble branches, dust
thresholds, dust update runtime, hitstun/public-knockback decay, damage color-animation routing/update reachability, damage invincibility gate coverage, Smash DI lag-update gates, imported-original `ftCommonDamageCommonProcPhysics` ground/air/clear branches, DamageAir WallDamage map short-circuit coverage, `ftCommonDamageGetKnockbackAngle` Sakurai-angle branch coverage, imported DamageFall interrupt source-order coverage, high-knockback screen flash routing, public knockback reaction, damage voice SFX,
deterministic FlyTop selection, status replacement/electric wrapping, random FlyRoll selection, imported-original common damage callback ground/air update plus common interrupt ground/fall/hammer and AirCommon interrupt routing, imported-original public-wrapper damage-level thresholds, imported-original public-wrapper damage hold/resist gates, public-wrapper `ftCommonDamageUpdateCatchResist` branch coverage, public-wrapper imported-original `ftCommonDamageUpdateMain` catch/capture keep-hold, catch/capture zero-knockback, catch/capture stats/release/no-damage status branches, tail-colanim, damage-status/Sleep branches, held-item bypass/resist/drop branches, imported thrown damage-stat/release-status helper routing, and selected attack-rebound promotion into imported-original ReboundWait effects as `procRumble=0x7f` / `procRebound=0x1f`, `dust=0xff`, `dustUpdate=0x1f`,
`hitPublic=0xf`, `colAnim=0x3f`, `colAnimUpdate=0x1f`, `invGate=0x1f`, `lagUpdate=0x3f`, `commonPhys=0x3f`, `commonCb=0x3fff`, `level=0x1f`, `hold=0xff`, `catchResist=0x1f`, `airMapWall=0x3f`, `angle=0x3f`, `fallInterrupt=0x3f`, `flash=0x7f`, `public=0x3f`, `voice=0xf`, `flytop=0xf`, `replace=0x3f`, `flyroll=0x1f`, `kirbycopy=0x6`, `itemHeavy=0x1f`, `itemBypass=0x1f`, `dmgKind=0x7f`, `sleep=0x7f`, and `loseGrip=0x7b/6/6`,
proving the original normal-hit rumble ID/length calculation, BatSwing4
rumble ID `10` length `0`, dust buckets, dust timer decrement/spawn/reset,
hitstun decrement/public-knockback transfer, Fire/Electric/Freezing/default damage color-animation routing plus wrapper update reachability, `ftCommonDamageCheckSetInvincible` hitlag/flag gates plus timed-invincible true branch, imported-original `ftCommonDamageCommonProcLagUpdate` hitlag/stick/tap gates plus Smash DI root translation, imported-original `ftCommonDamageCommonProcPhysics` ground friction, air friction, air drift/gravity, low-speed throw attack-clear, and restore branches, the DamageAir wall-collision route through original WallDamage helper side effects, reflected knockback/LR, Passive/DownBounce short-circuit, and restore, imported-original `ftCommonDamageGetKnockbackAngle` fixed-angle, air `361`, ground low/high/capped `361` branches, the imported `ftCommonDamageFallProcInterrupt` source-order special-air, attack-air, jump-aerial, and hammer fallback checks, low-knockback screen-flash no-op plus
high-knockback Fire/Electric/Freezing/default screen-flash routing through
imported original `ftCommonDamageCheckMakeScreenFlash`,
75-115 degree public-knockback reduction, force and non-force public-reaction
handoffs, preservation of the separate `damage_kind` dispatch enum across
hit-element setup, hitlag-blocked `ftCommonDamageSetStatus` passive no-op before
zero-hitlag replacement dispatch, hitstun-threshold/
forced damage-voice calls, deterministic FlyTop angle branch into status/motion
`54/47`, replacement status `55` preserved through electric wrapper
status/motion `50/43`, passive dispatch to stored replacement status/motion `55/48`, percent/RNG FlyRoll branch into status/motion
`55/48`, bounded Kirby copy-loss reset from Fox copy `1` to Kirby `8` with
FGM `204`, held-item bypass fallthrough into the normal damage color-animation
tail, FuraSleep status/motion `165/145`, cliff-catch wait setup, FuraSleep
color-animation seam reachability, and proof-local restore.
Modes `161/162` inherit the standalone modes `159/160` live-hit damage-loop
proof and add the selected damage-status follow-through: status `17->52/45`,
hitlag `6->0`, lag callbacks `1/6/1`, original damage update ticks proving
hitstun `2->1`, public knockback release, one installed original damage
status-set knockback-over branch into timed hit-status invincibility, one
original DamageFlyRoll status-set branch into model-pitch update, one
installed original damage physics tick proving selected air-friction/gravity
as `phys=11500/-1000`, one original ground damage-physics branch through
traction friction, one original zero-hitstun air damage-physics branch through
fastfall, one original zero-hitstun air damage-physics branch through drift
and gravity, one original zero-hitstun air damage-physics branch through
air-velocity clamp-decrement, one original lag-update no-op gate proof for
no-hitlag, below-threshold stick, and saturated tap buffers, one original
low-speed throw damage-physics branch that clears
attack collisions, one original DamageFlyRoll damage-physics branch that
updates joint pitch, one original Smash DI lag-update branch that moves the
root and resets tap buffers,
one installed original damage interrupt tick `interrupt=1`, one proof-local
ground DamageCommon interrupt tick through Wait/Ground, one proof-local
air DamageCommon interrupt tick through Fall interrupt, one proof-local ground
hammer DamageCommon interrupt tick through `ftHammerProcInterrupt`, one
proof-local air hammer DamageCommon interrupt tick through
`ftCommonHammerFallProcInterrupt`, one post-expiry installed original
DamageFall interrupt source-order branch through SpecialAir, AttackAir,
JumpAerial, and HammerFall checks, one installed original damage map
no-collision tick `map=1/1`, one installed original damage
map floor-collision branch through passive checks into DownBounce
`floor=1/1/1/1`, one installed original DamageAir no-floor collision
short-circuit, one installed original DamageAir map passive short-circuit
proof through imported PassiveStand and Passive checks, one installed original
damage map left/right-wall and
ceiling WallDamage short-circuit `wall=0x3ffff`, one installed original
DamageFall map no-collision/floor fallback plus proof-local imported-original
DownBounce set-status handoff plus no-cliff-mask and imported
PassiveStand/Passive short-circuits `fallmap=0x7ff`, cliff-catch branch
`cliff=0x3f`,
selected air/fly expiry through imported-original
`ftCommonDamageFallSetStatusFromDamage` into `57/50`, source-shaped
search/proc masks, repeat suppression, and repeat gate `0x3f`.
Current marker:
`mpLiveHit=atk=191/166 hitbox=1/j14 dmg=4 second=0/j14/x140 hurt=3/10 hbdmg=0->4/6 eff=0x1ff/0->0 resist=0xfff/7->3 rbreak=0x7f/2->-2/q2 throwAttr=0x1f/1->0 clash=0x3f/24/18 catchStat=0x1f/160000 catchSearch=0xffffffff/s3 shield=4->4/4 shc=0x7fffff/3142 so=155/134 soTick=0x1f/155->154 contact=1 repeat=1 carry=0xf gate=0x3f rehit=5->0 origRehit=hit30/v40000 30->29->0 clear=1/3 ids=0x3 dmg=0->4 hitlag=6 status=17->52/45 hitlag=6->0 callbacks=1/6/1 update=2->1 phys=11500/-1000 interrupt=1 map=1/1 floor=1/1/1/1 wall=0x3ffff cliff=0x3f fallmap=0x7ff finish=57/50 search=0xf repeat=1/1 gate=0x3f`.
Latest live-hit catch-search hazard increment: modes `161/162` keep
`STAGE_MPLIVEHIT_CATCHSEARCH` mask low bits `0xffffffff`, but the existing
true-return hazard-dispatch bit now also requires TaruCannon kind `3` to route
through `ftMainSetHitHazard` into the source-ordered TaruCannon status setup
shell: status `61`, script `-1`, TaruCannon status-vars reset, barrel GObj
capture, capture immunity, invisible flag, intangible hitstatus, and one
installed original TaruCannon physics callback tick copying the fighter root
position from the barrel root before state restore. Continuous TaruCannon
update/shoot runtime still waits for the Jungle barrel helpers and map
throw-hit data.
Latest live-hit catch-search increment: modes `161/162` now require the
`STAGE_MPLIVEHIT_CATCHSEARCH` secondary skip mask low bits `0x3fff`. The added
bit records the restored source-order self-target rejection branch continuing
to the next fighter in the secondary skip mask before state restore.
Previous live-hit catch-search increment: modes `161/162` required the
`STAGE_MPLIVEHIT_CATCHSEARCH` secondary skip mask low bits `0x1fff`. The added
bits recorded the restored outer target rejection probes for capture immunity,
ghost, Boss, global hitstatus, and same-team targets in the secondary skip mask
before state restore.
Previous live-hit catch-search increment: modes `161/162` required the
`STAGE_MPLIVEHIT_CATCHSEARCH` secondary skip mask low bits `0xff`. The added
bits recorded the restored source-order attack-state-off, Ground/Air mismatch,
and attack-record rejection probes in the secondary skip mask before state
restore.
Previous live-hit catch-search increment: modes `161/162` required the
`STAGE_MPLIVEHIT_CATCHSEARCH` secondary skip mask low bits `0x1f`. The added
bits recorded the existing restored source-order `ftMainSearchFighterCatch`
`None` sentinel break and valid no-collision no-update probes in the secondary
skip mask before state restore.
Previous live-hit catch-search increment: modes `161/162` required the
`STAGE_MPLIVEHIT_CATCHSEARCH` secondary skip mask low bits `0x7`. The added
bit restored Mario/Fox, marked the selected damage-coll slot invincible, reran
BattleShip's source-order `ftMainSearchFighterCatch`, and proved the catch loop
skipped the invincible damage-coll before search target/distance or
attack-record updates.
Previous live-hit hurtbox-damage increment: modes `161/162` require
`STAGE_MPLIVEHIT_HURTBOX_DAMAGE` mask low bits `0x7ffff`, and the inherited
status-loop search marker requires the same damage mask. The added bits
restore Mario/Fox after the natural slot-7 proof, mark earlier Mario
damage-coll slots intangible, rerun BattleShip's source-order
`ftMainProcSearchHitAll`, and prove natural slot-8 and slot-9 damage consume
before restoring state. The public `hurt=3/10` / `hbdmg=0->4/6` summary remains
the selected slot-3 diagnostic.
Previous live-hit hurtbox-damage increment: modes `161/162` require
`STAGE_MPLIVEHIT_HURTBOX_DAMAGE` mask low bits `0x1ffff`. The added bit
restores Mario/Fox after the natural slot-6 proof, marks Mario damage-coll
slots `0` through `6` intangible, reruns BattleShip's source-order
`ftMainProcSearchHitAll`, and proves natural slot-7 damage consume before
restoring state.
Previous live-hit hurtbox-damage increment: modes `161/162` require
`STAGE_MPLIVEHIT_HURTBOX_DAMAGE` mask low bits `0xffff`. The added bit
restores Mario/Fox after the natural slot-5 proof, marks Mario damage-coll
slots `0` through `5` intangible, reruns BattleShip's source-order
`ftMainProcSearchHitAll`, and proves natural slot-6 damage consume before
restoring state.
Previous live-hit hurtbox-damage increment: modes `161/162` require
`STAGE_MPLIVEHIT_HURTBOX_DAMAGE` mask low bits `0x7fff`. The added bit
restores Mario/Fox after the natural slot-4 proof, marks Mario damage-coll
slots `0` through `4` intangible, reruns BattleShip's source-order
`ftMainProcSearchHitAll`, and proves natural slot-5 damage consume before
restoring state.
Previous live-hit hurtbox-damage increment: modes `161/162` require
`STAGE_MPLIVEHIT_HURTBOX_DAMAGE` mask low bits `0x3fff`. The added bit
restores Mario/Fox after the natural slot-2 proof, marks Mario damage-coll
slots `0` through `3` intangible, reruns BattleShip's source-order
`ftMainProcSearchHitAll`, and proves natural slot-4 damage consume before
restoring state.
Previous live-hit hurtbox-damage increment: modes `161/162` require
`STAGE_MPLIVEHIT_HURTBOX_DAMAGE` mask low bits `0x1fff`. The added bit
restores Mario/Fox after the natural slot-1 proof, marks Mario damage-coll
slots `0` and `1` intangible, reruns BattleShip's source-order
`ftMainProcSearchHitAll`, and proves natural slot-2 damage consume before
restoring state.
Previous live-hit hurtbox-damage increment: modes `161/162` require
`STAGE_MPLIVEHIT_HURTBOX_DAMAGE` mask low bits `0xfff`. The added bit
restores Mario/Fox after the natural slot-0 consume/repeat proof, marks Mario
damage-coll slot `0` intangible, reruns source-order `ftMainProcSearchHitAll`,
and proves natural slot-1 damage consume before restoring state.
Previous status-loop callback increment: modes `161/162` now require
`STAGE_MPLIVEHIT_STATUS` and `STAGE_MPLIVEHIT_STATUS_CALLBACK` mask low bits
`0xffffffff`. The added bit reruns installed original
`ftCommonDamageFallProcInterrupt` after DamageFall expiry, proving BattleShip's
source-order SpecialAir, AttackAir, JumpAerial, and HammerFall checks before
restoring state.
Previous status-loop callback increment: modes `161/162` require the same
markers at mask low bits `0x7fffffff`; that bit reruns installed original
`ftCommonDamageFallProcMap` with collision but no `MAP_FLAG_CLIFF_MASK`,
proving the source no-cliff tail runs PassiveStand, Passive, and DownBounce
before restoring state; `fallmap` is now `0x7ff`.
Previous status-loop callback increment: modes `161/162` require the same
markers at mask low bits `0x3fffffff`; that bit reruns installed original
`ftCommonDamageAirCommonProcMap` with collision but no `MAP_FLAG_FLOOR`,
proving the source floor-bit short-circuit skips PassiveStand, Passive, and
DownBounce before restoring state.
Previous status-loop callback increment: modes `161/162` require the same
markers at mask low bits `0x1fffffff`; that bit reruns installed original
`ftCommonDamageFallProcMap` with floor collision, proving imported-original
PassiveStand and Passive true-return short-circuits skip the DownBounce tail
before restoring state.
Previous status-loop callback increment: modes `161/162` require the same
markers at mask low bits `0xfffffff`; that bit reruns installed original
`ftCommonDamageAirCommonProcMap` with floor collision, proving
imported-original PassiveStand and Passive true-return short-circuits skip
later map branches before restoring state.
Earlier status-loop callback increment: modes `161/162` require the same
markers at mask low bits `0x7ffffff`; that bit reruns installed original
`ftCommonDamageCommonProcLagUpdate`, proving the no-hitlag, below-threshold
stick, and saturated tap-buffer no-op gates before restoring state.
Earlier status-loop callback increment: modes `161/162` require the same
markers at mask low bits `0x3ffffff`; that bit reruns installed original
`ftCommonDamageCommonProcPhysics` with zero-hitstun Air kinetics and over-max
horizontal velocity, proving the source air-velocity clamp-decrement branch
before restoring the live-hit state.
Older status-loop callback increment: modes `161/162` require the same
markers at mask low bits `0x1ffffff`; that bit reruns installed original
`ftCommonDamageCommonProcPhysics` with zero-hitstun Air kinetics and horizontal
stick input, proving the source air-drift branch updates X velocity after
gravity before restoring the live-hit state.
Older status-loop callback increment: modes `161/162` require the same
markers at mask low bits `0xffffff`; that bit reruns installed original
`ftCommonDamageCommonProcPhysics` with zero-hitstun Air kinetics, proving the
source `ftPhysicsApplyAirVelDriftFastFall` branch sets fastfall state and
terminal velocity before restoring the live-hit state.
Older status-loop callback increment: modes `161/162` require the same
markers at mask low bits `0x7fffff`; that bit reruns installed original
`ftCommonDamageCommonProcPhysics` with ground kinetics, proving the source
ground-friction branch reduces ground velocity before restoring the live-hit
state.
Older status-loop callback increment: modes `161/162` require the same
markers at mask low bits `0x3fffff`; that bit reruns installed original
`ftCommonDamageCommonProcPhysics` as `DamageFlyRoll`, proving the source
physics branch reaches `ftCommonDamageFlyRollUpdateModelPitch` and updates
joint pitch before restoring the live-hit state.
Older status-loop callback increment: modes `161/162` require the same
markers at mask low bits `0x1fffff`; that bit reruns installed original
`ftCommonDamageCommonProcLagUpdate` with hitlag, stick range, and tap-buffer
state, proving the Smash DI branch moves the root and resets tap buffers
before restoring the live-hit state.
Older status-loop callback increment: modes `161/162` require the same
markers at mask low bits `0xfffff`; that bit reruns installed original
`ftCommonDamageCommonProcPhysics` with a low-speed throw-owned attack coll,
proving the source tail clears attack collisions before restoring the
live-hit state.
Older status-loop callback increment: modes `161/162` require the same
markers at mask low bits `0x7ffff`; that bit reruns installed original
`ftCommonDamageSetStatus` with `DamageFlyRoll`, proving the source branch reaches
`ftCommonDamageFlyRollUpdateModelPitch` and updates joint pitch before
restoring the live-hit state.
Older status-loop callback increment: modes `161/162` require the same
markers at mask low bits `0x3ffff`; that bit reruns installed original
`ftCommonDamageSetStatus` with `is_knockback_over` set, proving the source
branch clears that flag and sets timed hit-status invincibility before
restoring the live-hit state.
Older status-loop callback increment: modes `161/162` require the same
markers at mask low bits `0x1ffff`; that bit seeds `DamageN1` with Air
kinetics, marks hammer held through the existing hammer-check seam, runs
installed original `ftCommonDamageCommonProcInterrupt` with hitstun already
zero, and proves the air hammer branch reaches
`ftCommonHammerFallProcInterrupt` before restoring the live-hit state.
Older status-loop callback increment: modes `161/162` require the same
markers at mask low bits `0xffff`; that bit seeds ground `DamageN1`, marks
hammer held through the existing hammer-check seam, runs installed original
`ftCommonDamageCommonProcInterrupt` with hitstun already zero, and proves the
hammer branch reaches `ftHammerProcInterrupt` before restoring the live-hit
state.
Older status-loop callback increment: modes `161/162` require the same
markers at mask low bits `0x7fff`; that bit seeds `DamageN1` with Air
kinetics, runs installed original `ftCommonDamageCommonProcInterrupt` with
hitstun already zero, and proves the no-hammer air branch reaches
`ftCommonFallProcInterrupt` before restoring the live-hit state.
Older status-loop ground-interrupt increment: modes `161/162` require the same
markers at mask low bits `0x3fff`; that bit seeds ground `DamageN1`, runs
installed original
`ftCommonDamageCommonProcInterrupt` with hitstun already zero, and proves it
clears hitstun state and reaches imported-original Wait/Ground interrupt
handling before restoring the live-hit state.
Older status-loop update increment: modes `161/162` require the same
markers at mask low bits `0x1fff`; that bit seeds ground `DamageN1`, runs
installed original
`ftCommonDamageCommonProcUpdate` at animation end as hitstun reaches zero, and
proves public knockback release through original `mpCommonSetFighterWaitOrFall`
into imported-original Wait/Ground before restoring the live-hit state.
Older status-loop no-expiry increment: modes `161/162` require the same
markers at mask low bits `0xfff`; that bit reruns installed original
`ftCommonDamageAirCommonProcUpdate` with animation still active while hitstun
reaches zero, proving public knockback release without the DamageFall expiry
handoff.
Previous live-hit damage-loop increment: modes `161/162` require
`STAGE_MPLIVEHIT_DAMAGE` mask low bits `0xffffffff`. The added bit marks the
attacking fighter ghost and proves BattleShip's `ftMainProcSearchHitAll`
outer guard exits before clearing the hitlog or running fighter/item/weapon/
ground search and hit-stat processing.
Previous catch-search hazard increment: modes `161/162` require
`STAGE_MPLIVEHIT_CATCHSEARCH` mask low bits `0xffffffff`. The added bit marks
the fighter ghost and proves BattleShip's `ftMainSearchHitHazard` outer guard
exits before wait-timer decrement or obstacle callbacks. The true-return
hazard-dispatch proof now covers Twister setup/tick and TaruCannon setup;
continuous TaruCannon update/shoot runtime remains deferred.
Previous catch-search target-hitstatus increment: modes `161/162` require
`STAGE_MPLIVEHIT_CATCHSEARCH` mask low bits `0x1ffff`. The added bit restores
Mario/Fox, seeds each global target hitstatus field non-normal one at a time,
reruns source-order `ftMainSearchFighterCatch`, and proves the target is
rejected before search target/distance or attack-record update; the local
catch-search early guards now follow BattleShip's ghost, Boss, team,
capture-immune, then hitstatus order.
Previous catch-search ghost/Boss increment: modes `161/162` require
`STAGE_MPLIVEHIT_CATCHSEARCH` mask low bits `0xffff`. The added bits restore
Mario/Fox, seed ghost and Boss targets, rerun source-order
`ftMainSearchFighterCatch`, and prove each target is rejected before search
target/distance or attack-record update.
Previous catch-search team-gate increment: modes `161/162` require
`STAGE_MPLIVEHIT_CATCHSEARCH` mask low bits `0x3fff`. The added bit restores
Mario/Fox, seeds same-team fighters with team battle on and team attack off,
reruns source-order `ftMainSearchFighterCatch`, and proves the target is
rejected before search target/distance or attack-record update.
Previous catch-search capture-immune increment: modes `161/162` require
`STAGE_MPLIVEHIT_CATCHSEARCH` mask low bits `0x1fff`. The added bit restores
Mario/Fox, seeds `capture_immune_mask & catch_mask`, reruns source-order
`ftMainSearchFighterCatch`, and proves the target is rejected before search
target/distance or attack-record update.
Previous catch-search natural-slot increment: modes `161/162` require
`STAGE_MPLIVEHIT_CATCHSEARCH` mask low bits `0xfff`. The added bit restores
the saved Mario/Fox fighter structs, clears the catch attack record, positions
the selected catch hitbox on Mario's natural damage-coll slot `0`, reruns
source-order `ftMainSearchFighterCatch` without proof-local slot-3 masking, and
proves target, distance, and attack-record update before restoring state.
Previous catch-search repeat increment: modes `161/162` require
`STAGE_MPLIVEHIT_CATCHSEARCH` mask low bits `0x7ff`. The added bit keeps the
catch attack record from the first source-order search, reruns
`ftMainSearchFighterCatch`, and proves the same-victim catch repeat is rejected
with no new closest target/distance update.
Previous shield-contact repeat increment: modes `161/162` now require
`STAGE_MPLIVEHIT_SHIELD_CONTACT` mask low bits `0x7fffff`. The added bit keeps
the shield attack record intact, reruns source-order `ftMainSearchHitFighter`,
and proves the same-victim shield repeat is rejected with no new shield
collision, stat, effect, or shield-damage changes.
Previous natural hurtbox repeat increment: modes `161/162` required
`STAGE_MPLIVEHIT_HURTBOX_DAMAGE` mask low bits `0x7ff`. The added bits prove
the restored, unmasked source-order fighter hit search naturally consumes
Mario damage-coll slot `0`, then rejects an immediate repeat search through
the attack-record gate without queue, percent, or hitlog growth. The public
`hurt=3/10` / `hbdmg=0->4/6` summary remains the selected diagnostic.
Previous CliffCatch status increment: modes `161/162` run a proof-local
restored DamageFall cliff-collision seed through imported-original
`ftCommonCliffCatchSetStatus`, verifying CliffCatch status/motion, Air state,
floor-line clear, cliff hold, callback slots, capture immunity, damage
callback, and velocity stop before restoring the current callback proof state.
Previous DownBounce status increment: modes `161/162` run a proof-local
restored DamageFall floor-collision seed through imported-original
`ftCommonDownBounceSetStatus`, verifying DownBounce status/motion, Ground
state, callback slots, attack-buffer clear, and `damage_mul=0.5` before
restoring the current callback proof state.
Previous capture callback increment: public `ftParamStopVoiceRunProcDamage` now
restores BattleShip's `proc_damage` callback tail after the existing capture
voice-stop marker path. The inherited Passive recover CapturePulled proof seeds
a bounded victim callback and now prints `capture=171/150->172/-2
procDamage=1`, proving the callback ran before original CapturePulled status
setup.
Previous CliffWait DownBounce routing increment: the standalone CliffWait
damage proof still keeps the bounded `ftCommonDownBounceSetStatus` branch
responsible for status installation, but it now chooses DownBounceD/U with
BattleShip's original `ftCommonDownBounceCheckUpOrDown` orientation formula
instead of a hard-coded U status. The branch still routes the
ImpactWave/FGM/rumble tail through imported-original
`ftCommonDownBounceUpdateEffects`. Full imported
`ftCommonDownBounceSetStatus` remains deferred for the standalone CliffWait
runtime branch, but modes `161/162` now exercise it under a guarded restored
DamageFall floor-collision seed.
Previous CliffWait DamageFall routing increment: the standalone CliffWait
damage proof now lets public `ftCommonDamageFallSetStatusFromCliffWait` call
imported-original `ftCommonDamageFallSetStatusFromCliffWait` instead of the
previous local DamageFall shell. The `mpCliffWaitDamage=status=85->57
motion=73->50 fallWait=1->0 catch=30` marker remains stable, but the handoff
count now requires the bounded `ftMainSetStatus` DamageFall install and
imported clamp/rumble tail. The bounded PassiveStand/Passive floor branches
still call imported-original checks first and preserve the original
ground/status/velocity-transfer setter sequence through the proof-local status
seam when needed.
Previous WallDamage status routing increment: the standalone passive/recover
WallDamage update proof now lets imported-original `ftCommonWallDamageProcUpdate`
call imported-original `ftCommonDamageFallSetStatusFromDamage` instead of using
the previous local DamageFall shell. The `wallDamage=56/49->57/50` marker
remains stable, but the handoff count now requires the bounded
`ftMainSetStatus` DamageFall install and imported clamp/rumble tail; the public
WallDamage rumble count remains scoped to original setup rumble ID `2`.
Latest selected status-loop increment: the selected post-hitlag damage status
now drives installed imported-original damage physics, interrupt, and map
callbacks after `ftCommonDamageSetStatus`, proving the `DamageFlyN` hitstun
air-physics branch, the ground-friction, zero-hitstun fastfall, and
zero-hitstun air-drift damage-physics branches, a guarded AirCommon ->
DamageFall interrupt handoff, and the AirCommon map no-collision,
floor-collision/DownBounce, and left/right-wall plus ceiling WallDamage
short-circuit paths, then post-expiry
DamageFall map no-collision, floor fallback, and cliff-catch branches while
restoring the proof-local fighter state. Natural continuous damage runtime
remains deferred.
Previous DamageFall status routing increment: the Dash-Run hitstun-expiry
DamageFall handoff now calls imported-original
`ftCommonDamageFallSetStatusFromDamage` and requires the bounded
`ftMainSetStatus` DamageFall install plus the imported clamp/rumble tail before
the existing expiry marker can pass. The marker surface remains stable.
Previous damage callback increment: the Dash-Run common callback marker now
requires `commonCb=0x3fff`, adding the imported-original
`ftCommonDamageCommonProcInterrupt` air/no-hammer dispatch into
`ftCommonFallProcInterrupt` while keeping the earlier ground/air update,
common ground/hammer interrupt, AirCommon interrupt, and restore coverage.
Latest dispatcher increment: the selected slot-3 hurtbox damage consume path
and the Dash-Run proc-params aggregate now enter bounded project-owned
`ftMainProcParams` instead of open-coding the damage/shield/reflect/absorb
tails. The current pair proves the aggregate with `DASH_RUN_PROCPARAMS=0xffffffff`
and `procRumble=0x7f` / `procRebound=0x1f`; real Boss, natural continuous
Twister/trap gameplay, natural continuous rebound, and full unbounded
`ftmain.c` runtime remain deferred.
The selected slot-3 hurtbox damage branch now enters through bounded
`ftMainProcSearchHitAll`, including deferred item/weapon/ground search stubs
and the existing fighter hitlog stats handoff. The selected attack-clash and
shield-contact/stat branches route through bounded `ftMainSearchHitFighter`,
and the selected catch-search proof routes through bounded
`ftMainSearchFighterCatch`, including same-victim catch-search repeat rejection
and restored natural slot-0 / ghost / Boss / same-team / capture-immune /
target-hitstatus gate probes.
Modes `161/162` additionally prove a restored
unmasked slot-0 fighter hit search through the same source-order consume path
and same-victim repeat rejection through the attack-record gate, plus
same-victim shield-contact repeat rejection through the shield attack record.
Modes `157/158` remain regression coverage for the selected
hit-to-damage-to-recovery lifecycle aggregate. Modes `155/156` remain
regression coverage for the PassiveStand/Passive recover aggregate. Modes
`151/152` remain regression coverage for the platform-speed reader and
dynamic floor/ceil/wall consumer path.

The previous recover pair proves the bounded original passive input gates:
PassiveStandF, PassiveStandB, neutral-stick no-transition, expired-Z
PassiveStand no-transition, Passive, and expired-Z Passive no-transition. It
now also calls imported original `ftCommonDamageFallProcMap` through a
bounded source-order MP floor collision path through the installed
`FTStruct.proc_map` callback and proves that map callback selects both
PassiveStandF and Passive. A missing DamageFall map callback slot is now
unsafe instead of falling back to a direct base call.
The same recover pair now imports bounded original `ftcommonappeal.c` and
proves a guarded L-button Appeal/Taunt interrupt from Wait reaches
Appeal/Ground status/motion `189/164` with the expected callback shape, ticks
the installed original Appeal interrupt callback once with no catch/guard
branch, then runs the installed `ftAnimEndSetWait` update slot back to
Wait/Ground. It then re-enters Appeal through the same imported original gate,
opens the original `ftCommonAppealProcInterrupt` `flag1` branch, proves the
source-order catch check returns false, and reaches imported GuardOn
status/motion `152/134` through the existing bounded GuardOn compatibility
surface before restoring Wait/Ground.
The same current pair now imports bounded original `ftcommoncatch1.c` and
`ftcommoncatch2.c`. It seeds a Wait/Ground Z-hold plus A-tap catch input,
proves original `ftCommonCatchCheckInterruptCommon` reaches Catch/Ground
status/motion `166/146` through the bounded `ftCommonCatchSetStatus` path,
records the original catch-param setup shape, and runs the installed original
Catch map/update callbacks back to Wait/Ground `10/4`. It then drives the
bounded original CatchPull proc-catch path into CatchPull/Ground `167/147`,
wires `catch_gobj` to the seeded target, records the capture-immune,
catch-swirl, and rumble seams, ticks original CatchPull update into
CatchWait/Ground `168/-2`, and ticks original CatchWait interrupt once with
the throw check stubbed false while `throw_wait` decrements `60->59`.
It now also imports bounded original `ftcommoncapturepulled.c` and
`ftcommoncapturewait.c`, drives the seeded victim through original
CapturePulled/Ground `171/150`, proves the original
`ftParamStopVoiceRunProcDamage` callback tail with `procDamage=1`, ticks
original CapturePulled physics into CaptureWait/Ground `172/-2`, and runs one
installed CaptureWait map callback
through the project-owned floor-projection seam. It now also imports bounded
original `ftcommonthrow.c` and `ftcommonthrown1.c`, drives the seeded catcher
through original `ftCommonThrowCheckInterruptCatchWait` into ThrowF/Ground
`169/148`, then reseeds the same original catch-wait interrupt with stick-left
input to prove ThrowB/Ground `170/149`, queues the seeded victim into
ThrownCommon/Air `186/161`, and records the throw anim-event plus
capture-immune seams. It then ticks the
installed original ThrownCommon update/physics/map callbacks once, preserves
ThrownCommon/Air, and proves the map callback copies the captor floor line
before ticking the original animation-end update branch into immediate
ThrownCommon setup (`throwCb=1/1/1 floor=3 end=1/186/161`). It also ticks the
installed original ThrowF update callback with `flag2` set, proving the real
throw release branch reaches imported original thrown update-stats through
`throwUpdate=169/148 dmg=50->58 script=0`. It now also imports
bounded original `ftcommonthrown2.c`, seeds an original-compatible throw hit
descriptor, and proves `ftCommonThrownReleaseThrownUpdateStats` updates the
victim damage `10->18`, clears capture state, moves the victim to Air, installs
the thrown proc-status callback, and reaches the damage/stat/stale/rumble
compatibility seams, including the damage-init rumble now restored by the
bounded damage setup tail. It then ticks the installed original thrown proc-status
callback once, proving `ftParamSetThrowParams` and script ID `123`, and proves
bounded original damage-release, update-damage-stats, and no-damage-release
helpers. It now also seeds a bounded catcher/victim cleanup case and calls
original `ftCommonThrownDecideDeadResult`, proving the lose-grip path reaches
collision-default, SetAir, two Wait/Fall resolutions, catch/capture pointer
clearing, and final catcher Wait/Ground plus victim Fall/Air cleanup. Item
throw branches, continuous grab/capture/throw runtime, full damage runtime,
and player-driven grab gameplay remain deferred.
The current recover pair now also imports bounded original
`ftcommonwalldamage.c`. It seeds the selected Hyrule wall-hit source
(`floor=5`, `wall=13`, `edge=12`), calls original
`ftCommonWallDamageCheckGoto`, proves reflected wall knockback into
WallDamage status/motion `56/49`, records the effect/quake/rumble/intangible
compatibility seams, then ticks the installed original WallDamage update once
to hand off through imported-original `ftCommonDamageFallSetStatusFromDamage`
into the bounded DamageFall path `57/50`.
The same current pair now imports bounded original `ftcommonrebound.c`. It
seeds original-compatible `attack_rebound`, `hit_lr`, and
`rebound_anim_length`, calls original `ftCommonReboundWaitSetStatus`, proves
ReboundWait/Ground `82/-1`, lets the installed original ReboundWait update
transition into Rebound/Ground `83/71`, then ticks original Rebound once into
the existing Wait/Ground handoff `10/4`.
The current recover pair also now enables the existing bounded original
DownWait proof, so the latest direct/menu-chain boundary continues from the
recovered Wait/Ground state into DownWaitU/Ground, DownStandU, DownAttackU,
DownForwardU, and DownBackU source-order branches and their Wait/Ground
handoffs.
The same current pair now enables the existing bounded original Turn proof:
from recovered Wait/Ground, it drives the original Turn interrupt into
Turn/Ground status/motion `18/12`, proves the installed update callback flips
facing and ground velocity, then returns through the original Wait/Ground
handoff.
It now also enables the existing bounded original face-down DownRecover proof:
from the recovered/Turn-proven Wait boundary, it seeds DownWaitD/Ground,
proves DownStandD, DownAttackD, DownForwardD, and DownBackD source-order
branches, then verifies their original Wait/Ground handoffs.
The same current pair now enables the existing bounded CliffLedge aggregation
proof: same-cliff occupancy blocks a second catch, drop clears cliff hold into
Fall/Air, recatch succeeds after release, and CliffClimbQuick2 finishes through
the original Wait/Ground handoff.
It now also enables the existing bounded CliffLive proof, driving a proof-owned
selected P0 GObj process through original CliffCatch, CliffWait, CliffQuick,
CliffClimbQuick1, CliffClimbQuick2, one guarded common2 update/physics/map
tick, Wait/Ground finish, and a reseeded CliffWait drop into Fall/Air.
The same modes now also enable the existing bounded CliffAttack/Common2
proofs. The delayed aggregate probe reseeds the selected fighter back into the
original-compatible CliffWait state before the A-button CliffAttack interrupt,
then isolates the shared Common2 bridge diagnostics before driving
CliffQuick -> CliffAttackQuick1 -> CliffAttackQuick2 and one guarded Common2
update/physics/map tick.
The same modes now also enable the existing bounded CliffEscape/Common2
proofs. The delayed aggregate probe reseeds the selected fighter back into the
original-compatible CliffWait state before the stick-away CliffEscape
interrupt, then isolates the shared Common2 bridge diagnostics before driving
CliffQuick -> CliffEscapeQuick1 -> CliffEscapeQuick2 and one guarded Common2
update/physics/map tick.
The same modes now also enable the existing bounded Hyrule wall-hit floor
proof. This reuses the source-order MP wall-line/floor-edge scout to prove the
selected wall relationship (`floor=5`, `wall=13`, `edge=12`) and keeps natural
wall-hit/copyback runtime deferred.
They now also enable the existing bounded wall-copy proof for that selected
probe. The proof runs the source-order wall collision copyback path once and
records the expected floor/wall/edge IDs plus the final collision mask.
The same modes now also enable the existing bounded pass-through floor proof:
same-line pass-through collision is rejected through `ignore_line_id`, then a
different-line probe is accepted through the original-compatible pass callback.
They now also enable the existing bounded active platform-floor proof, which
classifies the selected pass-through floor line against the original yakumono
platform predicate, installs the bounded yakumono DObj, and records the active
DObj/status state.
The same modes now also enable the existing bounded platform tick proof, which
routes the active yakumono through one guarded `mpCollisionAdvanceUpdateTic`
step and proves the platform predicate stays active after the tic advances.
They now also enable the existing bounded natural drop-through input proof,
which seeds original-compatible down input on Dream Land line `0`, routes
Wait -> Squat -> Pass through imported original `ftcommonpass.c` /
`ftcommonsquat.c`, records the resulting ignored floor line, then reuses the
same imported Squat helpers to prove SquatWait -> SquatRv -> Wait through the
installed original callback slots.
They now also enable the existing bounded platform-position proof, which calls
`mpCollisionSetYakumonoPosID` for the active Dream Land yakumono and records
the resulting `gMPCollisionSpeeds` delta.
They now also enable the existing bounded platform-speed proof, which calls
`mpCollisionGetSpeedLineID` and runs the bounded dynamic floor, ceiling, wall,
wall-process, animation, bounds, and stage-animation diagnostic slices already
covered by the platform-speed verifier.
They now also enable the existing bounded Inishie/Mushroom Kingdom scale proof,
which stages read-only `StageInishieFile3`, runs source-backed
`grInishieMakeScale` / `grInishieScaleProcUpdate`, and records the source-DL
preview plus scale update threshold diagnostics.
They now also assert the existing bounded Dash-Run attack/guard aggregate
proof and selected hitbox position/range/rectangle/collide/record/hit-log/SFX/stats/proc-params plus damage-status selector/setup/proc-passive/physics/flytop/replacement-electric/flyroll/knockback-invincible/lag-update/hitlag-lifecycle/air-map/interrupt/expiry/fall-physics/fastfall/map-floor-cliff markers inside the current Passive
recover boundary. The current direct and menu-chain pair records
`dashRun=0x3fffffff hitboxPos=0x1ffff hitboxIds=0x3 procRumble=0x7f procRebound=0x1f damageStatus=0x1f damageSetup=0xffffffff dust=0xff dustUpdate=0x1f hitPublic=0xf colAnim=0x3f colAnimUpdate=0x1f invGate=0x1f lagUpdate=0x3f commonPhys=0x3f commonCb=0x3fff level=0x1f hold=0xff catchResist=0x1f airMapWall=0x3f angle=0x3f fallInterrupt=0x3f flash=0x7f public=0x3f voice=0xf flytop=0xf replace=0x3f flyroll=0x1f kirbycopy=0x6 itemHeavy=0x1f itemBypass=0x1f dmgKind=0x7f sleep=0x7f`, proving the older bounded
Attack11/Attack12/Mario Attack13/Fox Attack100, AttackDash, GuardOn/Guard/
GuardOff/GuardSetOff, and EscapeF/EscapeB status/callback/update/Wait-handoff
slices remain live under the latest VSBattle/Pupupu root. The same aggregate
now also proves a bounded original Run -> TurnRun -> Run branch through
imported `ftcommonturnrun.c` (`DASH_RUN_TURNRUN`) and a bounded
BattleShip-compatible `ftcommondamage.c` damage-level/status-table selection,
plus a guarded damage-status install with original-compatible `proc_passive`
ownership, one bounded `ftMainProcUpdateInterrupt`-shaped
`ftCommonDamageCheckSetInvincible` passive dispatch tick, one bounded
`ftCommonDamageSetStatus` electric passive status dispatch tick, one
bounded sleep-element route through `ftCommonDamageGotoDamageStatus` into
FuraSleep status/motion plus `cliffcatch_wait`, imported FuraSleep breakout
setup/update, one A-tap plus stick mash breakout branch, one hitstun-decrement
update tick plus ground DamageCommon expiry into Wait/Ground, one
installed damage physics callback tick, one bounded DamageFlyRoll pitch/update-and-throw-clear physics tail, one bounded knockback-over status/invincibility branch, one bounded hitlag Smash DI lag-update branch, one bounded hitlag lifecycle tick through lag-update and lag-end callbacks, one bounded imported-original DamageAir map callback into the floor/passive/DownBounce seams, one bounded imported-original DamageAir wall-map branch through imported-original map routing, original WallDamage helper side effects, and Passive/DownBounce short-circuit, one imported DamageFall interrupt source-order pass through special-air, attack-air, jump-aerial, and hammer fallback checks, one DamageAir interrupt handoff into
the DamageFall interrupt seam, one hitstun-expiry DamageFall status handoff
through imported-original `ftCommonDamageFallSetStatusFromDamage`, one installed DamageFall air-physics callback tick, one original-shaped
fast-fall branch through `ftPhysicsApplyFastFall`, guarded DamageFall
no-collision, floor-collision, cliff-collision, and hammer-held ground/air
interrupt branch ticks, plus a restored
setup-tail side proof for public knockback, damage color animation selection,
screen flash, rumble, dust effect spawn/interval reset, player-tag wait, and attacker hit
count/knockback, plus the first public-wrapper imported-original `ftCommonDamageUpdateMain`
catch-resist/keep-hold branch that copies damage lag/hitlag to the grabbed
fighter and selects `nFTDamageKindColAnim`, plus the sibling catch-resist
release branch that selects `nFTDamageKindStatus` and reaches the bounded
damage-status install, plus the catch-side non-resist keep-hold branch that
updates grabbed fighter damage stats before the same bounded damage-status
path, plus the first capture keep-hold branch that copies
damage lag/hitlag back to the captured fighter and selects
`nFTDamageKindCatch` on the captor, plus the public-wrapper imported-original
zero-knockback catch branch that reaches the damage color-animation seam, plus
the capture zero-knockback
branch that sets captor hitlag, clears captured fighter tap/release input,
runs `proc_lagstart`, and reaches the same color-animation seam, before
restoring the preview state. It now also covers the capture-side non-resist
keep-hold branch that updates captured fighter damage stats before the same
bounded damage-status path, plus the capture-side keep-hold false branch that
routes through lose-grip into the same bounded damage-status path without
updating thrown damage stats, plus the capture-side keep-hold false
zero-knockback branch that routes the captor through imported original
no-damage release damage-var setup, plus the catch-side non-resist
zero-knockback branch that routes the grabbed fighter through imported
original damage-release setup before clearing the catch link, plus the
no-grab/no-capture tail colanim and damage-status branches, plus the
no-grab/no-capture Sleep-element FuraSleep dispatcher branch.
The floor tick
proves the original map branch reaches the passive checks and DownBounce seam;
the cliff tick proves it reaches the CliffCatch seam without promoting full
DownBounce or CliffCatch runtime here.
The standalone direct/menu-chain Jump-loop proof now also exercises the
original AttackAirLw rehit refresh branch and proves the local
`ftParamRefreshAttackCollID` seam marks attack coll IDs `0/1` new and active,
then clears their original four-slot attack records. It now also proves the
installed original `ftCommonAttackAirProcMap` map branches with `map=0x3ff`,
covering smooth LandingAirN, missing-animation LandingAirNull, skip-landing
Wait, and plain LandingLight handoffs before restoring the proof-local state.
It now also replays the original directional AttackAir selector for F/B/Hi/Lw
and proves `dir=0x1f` before restore, without changing the current live-hit
status boundary.

Latest proof summary:

```text
mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3
mpCliffWaitDamage=status=85->57 motion=73->50 fallWait=1->0 catch=30
passiveStand=73/62/0->cb1/1->10/4/0
passive=81/70/0->cb1/1->10/4/0
mpPassive=ps=73/62->10/4 ticks=6/5/5 passive=81/70->10/4 ticks=6/5/5 branch=0x7f psb=0xf natural=0x7f mapcalls=2 appeal=189/164 appealGuard=152/134 catch=166/146->10/4 catchPull=167/147->168/-2 tw=60->59 capture=171/150->172/-2 procDamage=1 throw=169/148->186/161 throwB=170/149->186/161 throwCb=1/1/1 floor=3 end=1/186/161 throwUpdate=169/148 dmg=50->58 script=0 throwRelease=dmg=10->18 kb=6600000 script=123 throwReleaseStatus=dmg=20->26 upd=30->36 noDmg=40->40 throwProc=param=1 script=123 throwDead=call=1 coll=1 air=1 waitFall=2 status=10/26 wallDamage=56/49->57/50 rebound=82/-1->83/71->10/4
mpDownWait=status=70/-2->72/61->10/4 attack=80/69->10/4 rolls=76/65->10/4,78/67->10/4
turn=10/4->18/12->10/4 lr=1->-1 vel=2500->-2500
downRecoverD=wait=69/-2 stand=71/60 atk=79/68 roll=75/64,77/66 final=0xf
mpCliffLedge=occ=1 drop=26/20 wait=30 hold=0 recatch=84/72 finish=10/4 line=3
mpCliffLive=wait=85/73 climb=86/74 quick2=88/76 finish=10/4 drop=26/20 src=0xfff
mpCliffAttack=status=85->86 motion=73->74 queue=1 cliff=3 button=0x8000
mpCliffAttackAction=status=86->92->93 motion=74->80->81 cliff=3 floor=3 calls=1/1/1
mpCliffCommon2=status=93->93->93->93 motion=81->81->81->81 cliff=3 floor=3->3
mpCliffEscapeAction=status=85->86->96->97 motion=73->74->84->85 cliff=3 floor=3 calls=1/1/1
mpCliffEscapeCommon2=status=97->97->97->97 motion=85->85->85->85 cliff=3 floor=3->3
mpWallHitFloor=pass floor=5 wall=13 edge=12 side=0 mapNodes=1 delta=-1600/-388
mpWallCopy=pass floor=5 wall=13 edge=12 delta=-1600/-388
mpPass=line=0 flags=0x4000 route=2 same=1 diff=1 cb=1/1/0
mpPlatform=line=0 yak=1 dobj=1 status=1 anim=0 active
mpPlatformTick=tic=0->1 setOn=1 advance=1 status=1/1
mpPassInput=line=0 flags=0x4000 squat=28 pass=33 rv=29->30->10 ignore=0
mpPlatformPos=line=0 yak=1 delta=12000/-4000/2000
mpPlatformSpeed=line=0 yak=1 read=12000/-4000 dyn=1/2 ceil=1/1 wall=1/1 procwall=1/1 anim=1 bounds=1 stageanim=0/0x91 inishieAsset=header/geometry nodes=1
inishieScale=ticks=2 lines=1/2 alt=80000->64000 y=363000/362000->427000/298000 speed=-8000/8000 fall=1->2/0 step=3->0/0 sourceDL=0xff cmds=91 tris=20 tex=0x3f preview=0x3f px=432
dashRun=0x3fffffff hitboxPos=0x1ffff hitboxIds=0x3 procRumble=0x7f procRebound=0x1f damageStatus=0x1f damageSetup=0xffffffff dust=0xff dustUpdate=0x1f hitPublic=0xf colAnim=0x3f colAnimUpdate=0x1f invGate=0x1f lagUpdate=0x3f commonPhys=0x3f commonCb=0x3fff level=0x1f hold=0xff catchResist=0x1f airMapWall=0x3f angle=0x3f fallInterrupt=0x3f flash=0x7f public=0x3f voice=0xf flytop=0xf replace=0x3f flyroll=0x1f kirbycopy=0x6 itemHeavy=0x1f itemBypass=0x1f dmgKind=0x7f sleep=0x7f
```

Supporting proof: the older direct/menu-chain Dash -> Run -> RunBrake modes
now import original `ftcommonturnrun.c`, `ftcommonattack1.c`,
`ftcommonattack100.c`, `ftcommonattackdash.c`, `ftcommonguard1.c`,
`ftcommonguard2.c`, and `ftcommonescape.c`. Before the
Dash path, they prove a bounded A-button tap from Wait goes through original
`ftCommonAttack1CheckInterruptCommon` into Attack11 status/motion `190/165`
for both Mario/Fox, installs the original Attack11 update/interrupt plus
bounded physics/map callbacks, and ticks those callbacks once
(`DASH_RUN_ATTACK11` callback/tick masks `0xff`, wait proc mask `0x3`). They
also prove the original Attack11 follow-up gate reaches Attack12 status/motion
`191/166` with callback mask `0xff` and goto mask `0xf`
(`DASH_RUN_ATTACK12`). They then prove the original Attack12 follow-up gate
reaches Mario's fighter-specific Attack13 status/motion `220/195`, while Fox
correctly remains blocked at Attack12 because the original fighter-kind gate
does not include Fox (`DASH_RUN_ATTACK13` callback mask `0xf`, goto mask
`0x7`). The same installed original Attack12 callbacks then prove Fox's
original rapid-jab gate reaches Attack100Start status/motion `220/195`
(`DASH_RUN_ATTACK100` check/setstatus/ftmain `5/1/1`, callback mask `0xf`,
goto mask `0x3`). The installed original Attack100Start update callback is
then driven through the animation-end handoff and reaches Fox Attack100Loop
status/motion `221/196` with callback mask `0xf` and goto mask `0x3`
(`DASH_RUN_ATTACK100_LOOP`; public wrapper count `0`, `ftMainSetStatus`
count `1`, because the imported TU's internal macro-renamed base status
function is the path being proven). The same marker now also proves one
guarded original Attack100Loop/End update proof (`tick=0xfff`) where A-input
sets loop intent, the update consumes `motion_vars.flags.flag1`, marks
animation end, clears loop intent, remains in Fox Attack100Loop, then a
no-input loop update reaches Fox Attack100End and the installed End update
returns through original `ftAnimEndSetWait` to Wait/Ground.
The dash-run proof now also records `DASH_RUN_ATTACK_ANIM=0x3f`, proving the
original Attack11, Attack12, Mario Attack13, and Fox Attack100Start status
paths reached the project-owned `ftMainPlayAnimEventsAll` seam. AttackDash is
intentionally not part of that marker because original BattleShip
`ftCommonAttackDashSetStatus` does not call `ftMainPlayAnimEventsAll`.
The direct/menu-chain dash-run verifiers now also require
`DASH_RUN_ATTACK_EVENT=0x1f/0x3f/0x20/10`, a bounded original-layout
`MakeAttackColl` event decoder proof on that same seam. It installs a selected
original Mario/Fox main-motion command extract for Attack11, Attack12, Mario
Attack13, and Fox Attack100Start, decodes the BattleShip
`ftMotionCommandMakeAttackCollS*` field layout into the narrow
`FTStruct.attack_colls[0]` / `[1]` shadows, and proves Fox Attack100Start has a
real script with no hitbox (`nohit=0x20`). The current source-order last
decoded hitbox is Fox Jab2 attack ID `1`: damage `4`, size `100`, offset
`0/0/0`, angle `70`, KBG/KBW/BKB `100/0/0`, shield `0`, and
air/ground/rebound flags `0x7`.
The verifiers also require `DASH_RUN_ATTACK_EVENT_CMDS=0xf`, proving the same
bounded scanner reaches Mario Jab3's original `SetAttackCollDamage`,
`SetAttackCollSize`, and `ClearAttackCollAll` command words and that the clear
leaves selected attack-collision state off. The same direct/menu-chain Dash
proof now also requires `DASH_RUN_ATTACK_EVENT_POS` mask `0x1ffff`, proving the
selected decoded Fox Jab2 hitbox can run bounded original `New -> Transfer`,
gated writeback into `FTStruct.attack_colls[1]`, and `Transfer -> Interpolate`
with `pos_prev = old pos_curr` (`state=3`, attack ID `1`, joint ID `14`,
matrix `0/0`), then pass the original broad-phase fighter-range predicate on
current/previous positions and a bounded original-compatible rectangle probe
against the selected Mario descriptor-backed hurtbox, then pass a bounded
`gmCollisionCheckFighterAttackDamageCollide`-style selected attack/damage
collision decision, then run a bounded source-order Mario hurtbox scan that
proves the global hitstatus and damage-detect gates, skips slots `0` and `1`,
tests and misses slot `2`, tests and hits slot `3`, and observes the source-order `None` sentinel after `10`
active slots, then insert Mario into Fox Jab2's
attack-record array using
the source-shaped `ftMainSetHitInteractStats` damage-record behavior, then run
a bounded normal-hit front half of `ftMainUpdateDamageStatFighter` that records
captured damage, attacker `attack_damage`, victim `damage_queue` /
`damage_lag`, the first `FTHitLog` entry, and the player-stat/stale-queue
compatibility seams, then select the original hit-collision FGM table entry,
reach the existing audio stub seam, and run a selected
`ftMainProcessHitCollisionStatsMain` fighter-hitlog stats handoff that fills
victim damage angle/LR/index/player metadata and computed knockback, then run a
source-shaped `ftMainProcParams` damage/hitlag scheduling handoff that consumes
Mario's queued damage, updates percent damage, reaches the damage-status,
damage-shuffle, and rumble seams, computes hitlag, pauses knockback, and clears
the transient damage fields. The same proof now includes the selected
`proc_lagstart` tail in the original post-hitlag position plus Fox's selected
attacker-side `attack_damage` / `proc_hit` and `attack_shield_push` /
`proc_shield` branches, selected victim `shield_damage` -> GuardSetOff,
selected victim shield-break into ShieldBreakFly, reflect-damage break into
ShieldBreakFly, Fox reflector hit, Ness reflector sound, and Ness absorb
branches, plus a bounded BattleShip-compatible
`ftcommondamage.c` damage-level/status-table selector proof for ground, air,
and electric damage status IDs, plus the first bounded source-shaped
`ftCommonDamageUpdateMain` catch-resist/keep-hold branch and sibling
catch-resist release/lose-grip branch, plus the first capture keep-hold
branch, public-wrapper imported-original zero-knockback catch branch,
capture zero-knockback branch, and
catch-side and capture-side non-resist keep-hold stats branches, plus the
capture-side keep-hold false lose-grip and zero-knockback no-damage release
branches, plus the catch-side non-resist zero-knockback damage-release branch
and the no-grab/no-capture tail colanim and damage-status branches, plus the
no-grab/no-capture Sleep-element FuraSleep dispatcher branch, plus the
DK-family heavy-item catch-resist/drop and held-item bypass branches while the
current damage-status runtime remains parked. This is
still a bounded selected-script scan; full
animation command runtime, real stale-queue handling, continuous live hitbox
activation, broader natural multi-slot hurtbox/runtime consumption beyond
the restored slot-0 consume/repeat, natural slot-1/2/4/5/6/7/8/9 consume, and selected slot-3 proofs, continuous rehit gameplay beyond
the selected Link down-air timer window, the remaining continuous
`ftCommonDamageUpdateMain` runtime, full
damage status/runtime, effects, real DS audio, positional audio balance, and
exact imported Fox/Ness special runtimes
remain deferred.
The direct and menu-chain Mario/Fox init harnesses now also prove a bounded
original-compatible `FTDamageColl[11]` shell. The init seam copies every valid
source descriptor slot like original `ftmanager.c`: Mario has `10` active
damage-coll slots and Fox has `11`. Selected slot 0 remains the live-hit
target, with Mario joint `6` half-size `51.5/56.0/47.5` and Fox joint `5`
half-size `51.0/26.0/22.5`.
The same init harnesses attach bounded source-compatible `FTParts` transform
shells for those selected joints and prove matrix/scale availability with
`parts=0x3/0x3/0x3`. This is still prerequisite state only; the remaining
broader natural continuous multi-slot victim runtime remains deferred.
They also prove a bounded Z-hold from Wait through original
`ftCommonGuardOnCheckInterruptCommon` into GuardOn status/motion `152/134`,
with installed callback mask `0xff`, state mask `0xfffffe0f`, GuardOn FGM `13`,
one bounded original GuardOn update tick that preserves GuardOn while
advancing shield decay/release counters, and one animation-end handoff through
original `ftCommonGuardSetStatus` into Guard status `153` with original Guard
callbacks, followed by one bounded original Guard hold update tick that stays
in Guard and advances the shield counters again, plus one release tick through
original `ftCommonGuardOffSetStatus` into GuardOff status/motion `154/135`
and one animation-end GuardOff update through original `ftCommonWaitSetStatus`
back to Wait/Ground before restoring Guard for Escape (`DASH_RUN_GUARD`). This
required
keeping the original `FTAttributes` `shield_size` offset stable after the
local struct extension.
They now also prove a bounded original GuardSetOff branch through imported
`ftCommonGuardSetOffSetStatus` and the installed original
`ftCommonGuardSetOffProcUpdate`: deterministic shield damage `10` produces
`setoff_frames=20200` milli-units and ground velocity `-40400`, held Z returns
to Guard, released Z returns to GuardOff, and the proof restores Guard for the
existing Escape branch (`DASH_RUN_GUARD_SETOFF=4/4 mask=0xfff cb=0xff`).
They also prove a bounded held-stick Guard -> Escape branch through original
`ftCommonEscapeCheckInterruptGuard`, reaching EscapeF/EscapeB status/motion
`156/136` and `157/137` with callback mask `0x3ff`, state mask `0xff`, and
original `itemthrow_buffer_tics` value `5` for both fighters
(`DASH_RUN_ESCAPE`). The same marker now also proves one guarded installed
Escape callback tick per fighter: original `ftCommonEscapeProcUpdate` consumes
`motion_vars.flags.flag1` and flips LR while animation time is positive,
original `ftCommonEscapeProcInterrupt` reaches the light-throw seam, the
installed physics/map callbacks reach bounded compatibility seams, and an
animation-end update returns through original `ftCommonWaitSetStatus` to
Wait/Ground (`tick=0x3ff`, interrupt/physics/map counts `2/2/2`).
They also prove a bounded original Run -> TurnRun -> Run branch. The proof
drives the installed original `ftCommonRunProcInterrupt` through
`ftCommonTurnRunCheckInterruptRun`, reaches TurnRun status/motion `19/13` for
both fighters, verifies installed original TurnRun update/interrupt slots plus
bounded physics/map callbacks (`callback=0xff`), ticks the original update
slot once to flip LR and ground velocity, then ticks animation end back through
original `ftCommonRunSetStatus` to Run/Ground `16/10`
(`DASH_RUN_TURNRUN` update mask `0xf`, ticks `4`).
They then prove the isolated Run A-button tap still goes through the installed
original `ftCommonRunProcInterrupt` route (`runproc=0x3`) into AttackDash
status/motion `192/167` with callback mask `0xff` and tick mask `0x3f`, then
restore Run before continuing the existing run/runbrake movement proof. Current
modes `155/156` now assert the aggregate `DASH_RUN` marker from this older
proof as `dashRun=0x3fffffff procRumble=0x7f procRebound=0x1f dust=0xff dustUpdate=0x1f hitPublic=0xf colAnim=0x3f colAnimUpdate=0x1f invGate=0x1f lagUpdate=0x3f commonPhys=0x3f commonCb=0x3fff level=0x1f hold=0xff catchResist=0x1f airMapWall=0x3f angle=0x3f fallInterrupt=0x3f flash=0x7f public=0x3f voice=0xf flytop=0xf replace=0x3f flyroll=0x1f kirbycopy=0x6 itemHeavy=0x1f itemBypass=0x1f dmgKind=0x7f sleep=0x7f`. This does not import
Attack11/Attack12/Attack13/Attack100Start/AttackDash hitboxes, continuous
Attack100Loop runtime, rapid-jab hitboxes, animation command runtime, item
attack branches, continuous Guard hold, continuous SetOff/Escape shield
runtime beyond the isolated SetOff/Escape callback proofs, continuous TurnRun
runtime beyond the isolated callback/update handoff proof, or full attack
runtime.

Use `-DelaySeconds 3` for `verify-boundary.ps1` and `verify-current.ps1` when
checking the current pair. A 1-second menu-chain capture can halt before the
post-loop finalizer markers are published even though the bounded loop reaches
the proof setup.

The Inishie/Mushroom Kingdom scale proof remains standalone regression
coverage as modes `153/154` and is now also required by current modes
`161/162`. It keeps full Mushroom Kingdom runtime plus real DS hardware
texture/material rendering deferred.
Fighter/stage harness modes `>= 11` use a one-entry opening-action preview
cache to keep menu-chain ROMs under ARM9 memory; normal boot and opening
verification keep the three-entry cache.

A guarded NDS dev/test scene harness now exists for faster source-boundary
iteration. Normal builds still follow the natural startup/opening/movie path.
The Title harness build mutates the original-compatible
`dSCManagerDefaultSceneData` before entering imported `scManagerRunLoop`, so it
starts at `nSCKindTitle` with `scene_prev = nSCKindOpeningNewcomers` and still
runs the imported bounded `mnTitleStartScene` path. A VS setup target is wired
to `nSCKindVSMode` with `scene_prev = nSCKindTitle`. That target now imports
`mnvsmode.c` enough to run bounded original `mnVSModeStartScene` /
`mnVSModeFuncStart` setup, load the original `MNCommon` and `MNVSMode` O2R
files, create the original main GObj, default camera, viewports, VS menu
button/value/background/menu-name/subtitle SObj object graph, then park before
`mnVSModeMain` input/update transitions and continuous drawing.
The `vs_start_transition` harness enters the same VS setup boundary, advances
original `mnVSModeMain` through a bounded no-input gate, injects a synthetic
A-button tap on the original VS Start cursor, records original
`mnVSModeSaveSettings`, observes `scene_prev = nSCKindVSMode` and
`scene_curr = nSCKindPlayersVS`, reaches the original load-scene request, and
then parks at the bounded imported PlayersVS setup boundary.
The `players_setup` harness imports `mnplayersvs.c` through
`src/import/battleship_mnplayersvs.c`, loads the seven original PlayersVS menu
files, creates the original main GObj/default camera/camera set/menu object
graph, initializes original PlayersVS vars and slot state, then parks before
continuous character-select input/drawing.
The `maps_setup` harness imports `mnmaps.c` through
`src/import/battleship_mnmaps.c`, loads the five original Maps files, creates
the original main GObj/default camera/camera set/stage-select SObj graph,
starts from the seeded Pupupu/Dream Land cursor, loads `GRPupupuMap` and its
external O2R dependency chain, applies external fixups, runs the bounded
original Pupupu preview path, records real preview object/stage-pointer
diagnostics, and no longer marks Pupupu preview as deferred.
The `menu_chain_vsbattle` harness starts at VS Mode, runs the original VS Start
transition, enters bounded PlayersVS setup, injects a deterministic two-player
ready/start state through original PlayersVS update logic, enters bounded Maps
setup, injects a synthetic A-button stage select on Pupupu, observes original
Maps scene-data saving, reaches `scene_prev = nSCKindMaps` and
`scene_curr = nSCKindVSBattle`, then reaches the same imported bounded
VSBattle setup boundary with the selected Pupupu stage data adopted.
The `battle_fd` harness now starts directly at `nSCKindVSBattle` from
`nSCKindMaps`, seeds one Mario with stock rules and `nGRKindLast` as the
current Final Destination sentinel, loads the original/common battle file list,
creates the default battle camera path through compatibility stubs, builds
fighter descriptors from `SCBattleState`, creates stub fighter GObjs for active
players, reaches interface/HUD setup stubs, proves one bounded
`scVSBattleFuncUpdate` interface tick, and parks before real gameplay/update/draw.
The `battle_pupupu_stage` harness starts directly at `nSCKindVSBattle` from
`nSCKindMaps`, seeds two human players on Pupupu/Dream Land, resolves real
`MPGroundData` from `GRPupupuMap`, records geometry/map-node/light/BGM
metadata, enters the project-owned `grCommonSetupInitAll` wrapper's
Pupupu-only original path, runs imported original `grDisplayMakeGeometryLayer`,
`grMainSetupMakeGround`, `grPupupuMakeGround`, and `grPupupuInitAll`, creates
four original display-layer GObjs plus the original Whispy eyes, Whispy mouth,
back flowers, and front flowers map GObjs, records DObj/MObj/animation setup
diagnostics, creates stub fighter GObjs from original descriptors, proves one
bounded VSBattle interface tick, and parks before continuous stage update/draw,
Whispy wind, yakumono runtime, items/effects runtime, full fighter logic, or
gameplay.
The `battle_pupupu_update` harness starts from the same direct Pupupu VSBattle
setup boundary, then runs two guarded original `grPupupuProcUpdate` calls in a
deterministic safe substate. It proves the Sleep -> Wait transition and one
Wait countdown tick while preserving the four original map GObjs, object count,
default flower state, and zero wind/push/quake/particle side effects. The
`menu_chain_pupupu_update` harness proves the same bounded update after the
natural VS Mode -> PlayersVS -> Maps -> VSBattle chain.
The `battle_mariofox_model` harness starts directly at the same bounded Pupupu
VSBattle setup boundary, loads `FTManagerCommon`, Mario, Fox, and the required
external fighter O2R dependencies, applies internal/external fixups, and
replaces the stub Mario/Fox fighter GObjs with asset-backed fighter GObjs that
own real top/model/commonpart DObj trees. The `menu_chain_mariofox_model`
harness proves the same asset-backed Mario/Fox model boundary after the guarded
VS Mode -> PlayersVS -> Maps -> VSBattle chain. Both model harnesses park before
real fighter status/process/gameplay execution and before full fighter display
rendering.
The `battle_mariofox_struct` harness extends the direct model proof by attaching
persistent project-owned `FTStruct` shells to the real Mario/Fox fighter GObjs.
The shells are allocated from a small bounded pool, stored in `GObj.user_data.p`,
returned by `ftGetStruct`, seeded from the original `FTDesc` identity fields,
and populated with the original asset pointers, deterministic input masks,
collision pointer contracts, and a bounded top/common joint table derived from
the DObj tree. The `menu_chain_mariofox_struct` harness proves the same
FTStruct-backed state after the guarded VS Mode -> PlayersVS -> Maps ->
VSBattle path. These harnesses still park before original fighter status,
process, physics, hit/catch, shadow, gameplay, and full display execution.
The `battle_mariofox_init` harness starts from the same direct Pupupu VSBattle
boundary and runs a bounded project-owned helper in original
`ftManagerInitFighter` source order for P0 Mario and P1 Fox. It preserves the
model and persistent `FTStruct` proofs, initializes damage, shield, velocity,
attack/damage counters, hitstatus, throw/catch/item pointers, Mario/Fox passive
state, root DObj position/scale, `FTAttributes` collision contracts, and a
deterministic Pupupu floor-projection/ground-state contract. The
`menu_chain_mariofox_init` harness proves the same init-state boundary after
VS Mode -> PlayersVS -> Maps -> VSBattle. These init harnesses still do not run
original status transitions, process callbacks, fighter input, physics/update
loops, hit/catch/search runtime, shadows, full display traversal, or gameplay.
The `battle_mariofox_wait` harness extends the direct initialized Mario/Fox
boundary by importing original `ftcommonwait.c` through
`src/import/battleship_ftcommon_wait.c` and running original
`ftCommonWaitSetStatus` for both persistent fighter structs. A bounded
project-owned `ftMainSetStatus` seam accepts only `nFTCommonStatusWait` in the
Wait harnesses, installs the original Wait status contract, records status
`10`, motion `4`, animation frame `0`, animation speed `1.0`, special
interrupt `TRUE`, player tag wait `120`, and the expected Wait interrupt,
physics, and map callback pointers without executing them. The
`menu_chain_mariofox_wait` harness proves the same Wait-status/motion setup
after VS Mode -> PlayersVS -> Maps -> VSBattle. These Wait harnesses still park
before fighter process callbacks, input/update loops, physics execution,
hit/catch/search runtime, shadows, full display traversal, or gameplay.
The `battle_mariofox_wait_tick` harness starts from that same direct Wait
boundary and runs one bounded original `ftCommonWaitProcInterrupt` callback
tick for each initialized fighter, then calls the guarded project-owned
physics and map callback seams once per fighter. It proves neutral input does
not change status, motion, ground/air state, root position, ground velocity, or
GObj count, and it records the callback counts without starting the fighter
process loop. The `menu_chain_mariofox_wait_tick` harness proves the same
bounded tick after VS Mode -> PlayersVS -> Maps -> VSBattle. These tick
harnesses still park before real fighter update loops, unbounded physics/map
mutation, hit/catch/search runtime, shadows, full display traversal, and
gameplay.
The `battle_mariofox_wait_ground` harness starts from the same direct Wait
tick boundary, then runs a second controlled Mario/Fox ground pass. It seeds
deterministic ground velocity `2.0`, mirrors original
`ftPhysicsSetGroundVelFriction` and the safe subset of
`ftPhysicsSetGroundVelTransferAir` in source order, and runs the safe floor
branch of `mpCommonProcFighterOnCliffEdge`. It proves both fighters reach
friction/map diagnostics, velocity decreases to a stable floor state,
status/motion/ground-air/root/GObj counts stay stable, and no
Fall/Ottotto/process/display/gameplay escape occurs. The
`menu_chain_mariofox_wait_ground` harness proves the same bounded
ground-friction/map pass after VS Mode -> PlayersVS -> Maps -> VSBattle. The
older Wait tick harnesses keep their no-op physics/map seam behavior.
The `battle_mariofox_display_probe` harness starts from the same direct Wait
ground boundary and calls the current project-owned `ftDisplayMainProcDisplay`
stub exactly once for Mario and once for Fox under a DS-owned guard. It walks
the real fighter DObj trees as metadata only, records Mario/Fox DObj counts
`25/27`, MObj/AObj counts `0/0`, display-list candidate counts `14/18`, and
proves status, motion, ground-air state, root position, GObj count, draw,
matrix, and gameplay counters stay stable. It does not render fighters or
enter full fighter display traversal.
The `menu_chain_mariofox_display_probe` harness proves the same guarded
Mario/Fox display metadata boundary after the bounded original VS Mode ->
PlayersVS -> Maps -> VSBattle chain. It preserves the direct display proof's
DObj/display-readiness metadata contract while proving the full menu-chain
transition path still reaches Pupupu VSBattle with stable Wait/ground/display
state and no renderer, matrix, gameplay, root, or GObj escape.
The `battle_mariofox_dl_scan` harness extends the direct display metadata proof
by selecting the first display-list-bearing DObj for Mario and Fox and scanning
those original DLs through the existing `ndsRendererScanDisplayList()` parser
only. The selected DLs are taskman-arena copies of original fighter display
data, reported with ownership sentinel `0xfffffffe`, and now scan to command
counts `59/69` with renderer blocker `0/0`, zero unsupported opcodes, nonzero
vertex/triangle command families, and stable state. No fighter rendering,
matrix prep, `gcDrawAll`, continuous draw, or gameplay loop is entered. The
`menu_chain_mariofox_dl_scan` harness proves the same parser-only DL scan after
the bounded VS Mode -> PlayersVS -> Maps -> VSBattle chain.
The `battle_mariofox_dl_execute` and `menu_chain_mariofox_dl_execute` harnesses
extend that same direct/menu-chain chain by calling
`ndsRendererExecuteDisplayList()` once for the selected Mario DL and once for
the selected Fox DL. The execute callback decodes real `G_VTX`, `G_TRI1`, and
`G_TRI2` payloads, records command-family, bounds, color, and safety
diagnostics, and currently proves `59/69` commands, `28/23` decoded vertices,
and `37/20` triangles with zero unsupported commands. This is decode-only
proof; it still does not make fighters visible or enter full fighter display
traversal.
The `battle_mariofox_dl_draw` and `menu_chain_mariofox_dl_draw` harnesses
extend the same direct/menu-chain proof by reusing those selected real
Mario/Fox first display lists, decoding them again through
`ndsRendererExecuteDisplayList()`, normalizing the decoded triangles into
deterministic side-by-side `96x72` software preview boxes, and committing the
existing original-DL preview surface. Current proof draws nonzero software
pixels for both fighters (`4274/5345`) with `37/20` represented triangles,
`37/18` real drawn triangles, and `0/2` bounded marker triangles after
corrected F3DEX2 command decoding, zero unsupported commands, stable fighter
state/root/GObj counts, and zero draw/matrix/gameplay escape counters. This is
a bounded first-DL software preview, not full fighter rendering.
The `battle_mariofox_dl_draw_multi` and
`menu_chain_mariofox_dl_draw_multi` harnesses extend that visible baseline by
censusing all DL-ready Mario/Fox fighter DObjs (`14/18`), selecting the first
four DL-ready DObjs per fighter in deterministic depth-first order, executing
and decoding all eight selected original display lists through
`ndsRendererExecuteDisplayList()`, and drawing them into the same retained
`96x72` original-DL software preview surface with one shared projection
axis/bounds set per fighter. Current proof draws `6190/7026` pixels,
`87/79` represented triangles, `82/76` real drawn triangles, and `1/3`
bounded marker triangles for Mario/Fox after corrected F3DEX2 command decoding,
keeps all eight selected DObjs clean, and
preserves status `10`, motion `4`, GA `0`, root X, GObj count, and zero
draw/matrix/gameplay/range-reject escape counters. The renderer adapter now
recognizes the selected path's `G_MODIFYVTX` (`0x02`) command as a bounded
skipped state command for this preview.
The `battle_mariofox_dl_draw_all` and
`menu_chain_mariofox_dl_draw_all` harnesses extend the same direct/menu-chain
proof through the current guarded `ftDisplayMainProcDisplay` seam. They census
all DL-ready Mario/Fox fighter DObjs (`14/18`), select and execute all 32
original DObj display lists through `ndsRendererExecuteDisplayList()`, draw all
selected DObjs into the retained `96x72` original-DL software preview, and prove
`14913/13432` pixels, `334/322` represented triangles, `306/290` real drawn
triangles, and `10/24` bounded marker triangles for Mario/Fox. The F3DEX2
decoder now keeps all selected Mario/Fox DObjs clean (`clean=14/18`,
`failed=0/0`), and the maintained `FTR_DL_ALL_FAIL` marker is required to stay
fully clear with sentinel failed indices and no failure reason, renderer
blocker, unsupported opcode/command, or vertex-range reject. Stable status
`10`, motion `4`, GA `0`, root X, GObj count, and zero
draw/matrix/gameplay/range-reject escape counters are still required.
Collapsed projected triangles are represented by bounded software markers until
real fighter matrix/camera projection is imported, but marker triangles are
counted separately and no longer satisfy geometry proof bits.
The `battle_mariofox_walk_input` and
`menu_chain_mariofox_walk_input` harnesses extend that guarded all-DL boundary
with the first original input-driven fighter movement proof. They import
original `ftcommonwalk.c`, seed deterministic forward stick input, enter
through original `ftCommonWaitProcInterrupt`, route the guarded
`ftCommonGroundCheckInterrupt` seam into original
`ftCommonWalkCheckInterruptCommon`, and prove Mario transitions to
WalkMiddle (`status 12`, `motion 6`) while Fox transitions to WalkFast
(`status 13`, `motion 7`). They then run one bounded Walk interrupt callback,
one source-order Walk ground-velocity physics pass, and one safe floor-map
pass while preserving ground state, root position, GObj count, and zero
draw/matrix/display/gameplay escape.
The `battle_mariofox_walk_loop` and
`menu_chain_mariofox_walk_loop` harnesses extend the Walk-input proof with the
first bounded source-order movement loop. They run four held Walk frames for
Mario and Fox, call original `ftCommonWalkProcInterrupt` and
`ftCommonWalkProcPhysics` each frame, integrate `fp->physics.vel_air` into the
top/root DObj, and run the bounded safe floor-map seam. They then release stick
input to neutral, prove original `ftCommonWalkProcInterrupt` returns both
fighters to Wait through `ftCommonWaitCheckInterruptCommon`, and run one
bounded Wait friction/map settle frame. Current proof records root movement
`48000/-144000` milli-units in facing direction, velocity reduction
`12000/36000 -> 6000/28000`, stable root Y/GA/GObj state, and zero draw,
matrix, display, gameplay, process, denied-status, unexpected-status, or
full-renderer escape.
The `battle_mariofox_dash_run` and `menu_chain_mariofox_dash_run` harnesses
extend the Walk-loop proof by importing original `ftcommondash.c`,
`ftcommonrun.c`, and `ftcommonrunbrake.c`. They start from initialized
Mario/Fox Wait state, seed deterministic dash input through the original Wait
interrupt path, prove original Dash (`status 15`, `motion 9`), force the
bounded Dash -> Run threshold through original Dash interrupt logic, run four
held Run frames, release to neutral, prove original Run -> RunBrake
(`status 17`, `motion 11`), run two RunBrake frames, and park before
continuous fighter scheduling or gameplay. Current proof records root movement
`301575/-418500` milli-units in facing direction, velocity reduction
`51200/71000 -> 47450/66000`, stable root Y/GA/GObj state, and zero draw,
matrix, display, gameplay, process, denied-status, unexpected-status, or
full-renderer escape.
The `battle_mariofox_jump_loop` and
`menu_chain_mariofox_jump_loop` harnesses extend the Dash/Run proof with the
first bounded original ground-to-air movement path. They close the RunBrake
endpoint back to Wait through a guarded original-compatible
`ftAnimEndSetWait` / `ftCommonWaitSetStatus` path, import original
`ftcommonkneebend.c` and `ftcommonjump.c`, seed deterministic C-button jump
input with forward stick, enter KneeBend through original
`ftCommonWaitProcInterrupt` and `ftCommonKneeBendCheckInterruptCommon`, advance
bounded KneeBend updates until original `ftCommonJumpSetStatus`, and prove
JumpF (`status 22`, `motion 16`) with air state, upward velocity, six bounded
airborne frames, root X/Y movement, and zero draw, matrix, display, gameplay,
process, denied-status, unexpected-status, landing-status, or fall-status
escape.
The `battle_mariofox_landing_loop` and
`menu_chain_mariofox_landing_loop` harnesses extend that JumpF airborne proof
with the first complete bounded ground-air-ground loop. They import original
`ftcommonfall.c` and `ftcommonlanding.c`, enter Fall through guarded
`ftAnimEndSetFall`, prove Fall (`status 26`, `motion 20`), run bounded Fall
interrupt/air-physics/map frames, detect the Pupupu floor crossing in the
DS-owned map seam, call original `ftCommonLandingSetStatus`, prove
LandingLight (`status 31`, `motion 25`), switch back to ground through
`mpCommonSetFighterGround`, close LandingLight to Wait through
`ftAnimEndSetWait`, and run one post-landing Wait friction/map settle frame.
Current proof records Fall frames `16/14`, floor clamp `0/0`, stable GObj/GA/
root-Y state, source-order signed ground-X transfer, preserved negative
airborne Y velocity after the ground switch, and zero LandingHeavy,
FallAerial, JumpAerial, cliff, ceiling,
draw, matrix, display, gameplay, process, denied-status, or unexpected-status
escape.
The `battle_mariofox_process_loop` and
`menu_chain_mariofox_process_loop` harnesses extend that ground-air-ground
proof with one bounded scripted source-order fighter frame driver. They drive
both Mario and Fox through Wait -> Walk -> Wait, Wait -> Dash -> Run ->
RunBrake -> Wait, and Wait -> KneeBend -> JumpF -> Fall -> LandingLight ->
Wait using deterministic synthetic controller state while recording callback,
status, transition, visit, movement, and safety diagnostics.
The `battle_mariofox_scheduler_loop` and
`menu_chain_mariofox_scheduler_loop` harnesses then attach selected Mario/Fox
`GObjProcess` callbacks with original `gcAddGObjProcess`, invoke them through
`gcRunGObjProcess`, and run the bounded proof from the wrapped
`scVSBattleFuncUpdate` path under a capped VSBattle taskman update loop. The
current proof records 30 bounded VSBattle updates, 30 callbacks per fighter,
all expected movement visits/transitions, stable final Wait/Ground state, and
zero scheduler safety escapes.
The `battle_mariofox_controller_loop` and
`menu_chain_mariofox_controller_loop` harnesses extend that spine with the
first controller-source-driven proof. They start from the verified
scheduler-loop endpoint, enable deterministic DS controller playback, feed only
`OSContPad` data through `osContGetReadData`, run original
`syControllerReadDeviceData` and `syControllerUpdateGlobalData`, bridge
`gSYControllerDevices` into `FTStruct` input through DS-owned code, and drive
the same selected Mario/Fox `GObjProcess` callbacks through `gcRunGObjProcess`
from bounded VSBattle taskman updates. The current proof records 30
proof-local controller reads/updates, 30 callbacks per fighter, the same
`0x3ff` status and `0x7ff` transition masks, root-X movement, root-Y
rise/floor return, final Wait/Ground state, and zero direct-FTStruct script
input or safety escapes.
The `battle_mariofox_preview_loop` and
`menu_chain_mariofox_preview_loop` harnesses add guarded moving software
preview frames through `ftDisplayMainProcDisplay`. The
`battle_mariofox_stage_mpstale_floor_loop` and
`menu_chain_mariofox_stage_mpstale_floor_loop` harnesses remain maintained
finalizer-local regression evidence. They build on the MP wall-blocker proof,
keep the floor-only
`mpCommonRunFighterAllCollisions` slice bounded, keep the live selected
callback cross-floor proof as `-1 -> 3`, and add a finalizer-local
source-order `MPCollData` probe for a valid stale Dream Land floor pair
`1 -> 0` at `x=-285`, `y=1542`. That probe reaches
`mpProcessUpdateMain -> mpCommonRunFighterAllCollisions`, records the first
`mpProcessCheckTestFloorCollisionNew` miss evidence from the sweep proof, then
accepts the target floor through original/source-order
`mpProcessCheckTestFloorCollision`, calls `mpProcessSetLandingFloor`, reaches
`mpProcessRunFloorEdgeAdjust`, clears collision-end state, and parks without
moving the previous wall/edge proof-owned live roots. The maintained
live-stale harnesses then trigger that same valid-stale pair from the selected
P0 callback path through a contained local `MPCollData` source-order pass,
reach `mpProcessUpdateMain -> mpCommonRunFighterAllCollisions ->
mpProcessCheckTestFloorCollision`, accept target line `0`, call
`mpProcessSetLandingFloor`, reach `mpProcessRunFloorEdgeAdjust`, clear
collision-end state, and park while leaving the real Mario/Fox movement loop
on decoded floor line `3/3`. The previous ceiling-floor harnesses
`battle_mariofox_stage_mpceil_floor_loop` and
`menu_chain_mariofox_stage_mpceil_floor_loop` remain regression coverage for
the bounded source-order ceiling test/adjust path against real Pupupu ceiling
line `4`. The previous ceiling-status harnesses
`battle_mariofox_stage_mpceilstatus_floor_loop` and
`menu_chain_mariofox_stage_mpceilstatus_floor_loop` remain regression coverage
for the selected ceiling-heavy branch into original `ftCommonStopCeilSetStatus`.
The former wall-copy boundary,
`battle_mariofox_stage_mpwallcopy_floor_loop` and
`menu_chain_mariofox_stage_mpwallcopy_floor_loop`, remains regression coverage.
Those modes keep the live
VSBattle scene on Pupupu/Dream Land, inherit the current Hyrule wall-hit and
cliff-live proofs, locally install isolated Hyrule Castle geometry, and run a
proof-owned selected P0 `GObjProcess` that copies the source-order wall-hit
adjustment back into the live fighter root/collision shell. The proof validates
Hyrule floor `5`, wall `13`, edge-under `12`, left side `0`, records adjustment
delta `-1600/-388`, restores the Pupupu/Dream Land collision globals, proves
P1 root state is unchanged, and keeps Mario/Fox final safety at Wait/Ground.
The previous wall-hit modes remain regression coverage for the isolated Hyrule
source-order wall-hit floor-edge-adjust proof. The previous cliff-live modes remain
regression coverage for bounded original CliffCatch -> CliffWait -> CliffQuick
-> CliffClimbQuick1 -> CliffClimbQuick2, one guarded common2
update/physics/map tick, finish into Wait/Ground, and reseeded CliffWait drop
into Fall/Air with source mask `0xfff`. The previous cliff-ledge modes remain
regression coverage for
same-cliff occupancy blocking, drop/release into Fall/Air, post-release
recatch, and original CliffClimbQuick2 finish into Wait/Ground. The
previous DownRecover modes remain regression coverage for the face-down
DownWaitD/Ground shell and original `ftCommonDownWaitProcInterrupt` routing
through bounded DownStandD, DownAttackD, DownForwardD, and DownBackD branches,
including final Wait masks `0xf` and no unsafe escape. The previous Turn-loop
modes remain regression coverage for
Wait `10/4` -> Turn `18/12` -> Wait `10/4`, facing `1 -> -1`, ground velocity
`2500 -> -2500` milli, source-order check/setup/update counts `1/1/1`, and no
unsafe escape. The previous DownWait-loop modes remain regression coverage for
DownAttack/DownForward/DownBack/DownStand recovery branches, eight guarded
stable callback frames, bounded roll root movement `+10000/-10000` milli, and
Wait/Ground handoffs. The previous Passive-loop modes remain regression coverage
for two stable PassiveStand/Passive callback frames plus the original Wait
handoff. The previous CliffWait damage
modes build on the earlier ceiling-status, cliff-catch, CliffWait, and
CliffAttack setup proofs: the CliffCatch base still routes P1's selected original
`mpCommonProcFighterCliffFloorCeil` map callback through bounded
`mpProcessUpdateMain`, runs source-order left/right cliff tests, selects the
real right Pupupu ledge on line `3`, imports original
`ftcommoncliffcatchwait.c`, reaches `ftCommonCliffCatchSetStatus`, and proves
Fall/Air `26/20/1 -> CliffCatch/Air 84/72/1` with cliff hold set,
`cliff_id=3`, LR `-1`, root placement on the ledge, and `MAP_FLAG_RCLIFF`
evidence `0x2000`. It now also runs a second bounded same-cliff/same-LR
occupancy probe with P0 already holding Pupupu line `3`, proves P1's matching
right-ledge CliffCatch attempt is blocked before a second status setup, and
records `occ=1/0` for one occupancy block and zero extra status-set delta.
The CliffWait regression boundary starts from that state,
calls original `ftCommonCliffCatchProcUpdate`, reaches
`ftAnimEndCheckSetStatus` and original `ftCommonCliffWaitSetStatus`, proves
CliffWait/Ground `85/73/0`, then runs one guarded original
`ftCommonCliffWaitProcInterrupt` tick. It records fall-wait `1080 -> 1079`,
player tag wait `120`, nonzero capture immunity, proc-damage setup, and no
damage-fall call. The CliffAttack setup regression imports original
`ftcommoncliffattack.c`, `ftcommoncliffclimb.c`, and
`ftcommoncliffescape.c`, injects a synthetic A-button tap through original
`ftCommonCliffWaitProcInterrupt`, lets original
`ftCommonCliffAttackCheckInterruptCommon` choose the AttackQuick path, and
proves CliffQuick/Ground `86/74/0` with queued cliff motion metadata
`AttackQuick` on retained `cliff_id=3`. The previous CliffAttack action
boundary then calls
original `ftCommonCliffQuickProcUpdate`, reaches CliffAttackQuick1/Ground
`92/80/0`, calls original `ftCommonCliffAttackQuick1ProcUpdate`, reaches the
original `ftAnimEndCheckSetStatus` branch into
`ftCommonCliffAttackQuick2SetStatus`, and proves CliffAttackQuick2/Ground
`93/81/0` with one guarded call each through the Quick update, Quick1 set
status, Quick1 update, anim-end, Quick2 set status, and original common2
status helper wrappers. It also proves retained `cliff_id=3`, copied
`floor_line_id=3`, cliff-hold, proc-damage, Quick2 map callback, and
jostle-ignore setup. The project-owned wrapper now bridges the imported
`ftCommonCliffCommon2UpdateCollData` helper through a temporary original
`MPCollData` view, calls the original helper, and copies the resulting ledge
floor fields back into the live `FTCollisionData` shell. That bridge now
guards the queued cliff-motion status ID against the BattleShip
`cliff_status_ga` layout and records a root before/after/expected snap check
for CliffClimbQuick2, CliffAttackQuick2, and CliffEscapeQuick2 action handoffs.
The previous common2 boundary then consumes the created CliffAttackQuick2/Ground state through one
bounded original `ftCommonCliffCommon2ProcUpdate`,
`ftCommonCliffCommon2ProcPhysics`, and
`ftCommonCliffAttackEscape2ProcMap` tick. It proves Quick2/Ground
`93/81/0` stays stable, retained `cliff_id=3` and copied `floor_line_id=3`
survive the update/physics/map calls, the guarded anim-end check does not fall
through to Wait/Fall, ground velocity transfer is reached once, the map edge
break callback is reached once under guard, and no unsafe escape occurred.
The previous CliffEscape action boundary imports original
`ftcommoncliffescape.c`, starts from the CliffWait state, injects a synthetic
Z-button tap, proves the original attack check is reached and skipped, proves
the original escape check accepts, and reaches CliffQuick/Ground `86/74/0`
with queued EscapeQuick metadata on retained `cliff_id=3`. It then calls
original `ftCommonCliffQuickProcUpdate`, reaches CliffEscapeQuick1/Ground
`96/84/0`, calls original `ftCommonCliffEscapeQuick1ProcUpdate`, reaches the
original anim-end branch into `ftCommonCliffEscapeQuick2SetStatus`, and proves
CliffEscapeQuick2/Ground `97/85/0` with copied `floor_line_id=3`, cliff hold,
proc-damage, proc-update/proc-map, jostle-ignore setup, and no damage-fall or
unsafe fallback. The current CliffEscape common2 boundary then consumes the
created CliffEscapeQuick2/Ground state through one bounded original
`ftCommonCliffCommon2ProcUpdate`, `ftCommonCliffCommon2ProcPhysics`, and
`ftCommonCliffAttackEscape2ProcMap` tick. It proves Quick2/Ground `97/85/0`
stays stable, retained `cliff_id=3` and copied `floor_line_id=3` survive the
update/physics/map calls, the guarded anim-end check does not fall through to
Wait/Fall, ground velocity transfer is reached once, the map edge-break
callback is reached once under guard, and no unsafe escape occurred. The
previous CliffClimb floor boundary returns to the verified CliffWait/Ground
state on real Pupupu `cliff_id=3`, calls original
`ftCommonCliffWaitProcInterrupt`, reaches the original attack and escape
checks and proves they are rejected for the seeded inputs, then routes through
original `ftCommonCliffClimbOrFallCheckInterruptCommon`. It proves the upward
stick branch reaches CliffQuick/Ground `86/74/0` with queued climb metadata
and retained `cliff_id=3`, while the away-stick branch reaches Fall/Air
`26/20/1` with `cliffcatch_wait=30`, retained cliff metadata, and no
damage-fall or unsafe fallback. The floor boundary also proves a bounded
release-then-recatch handoff: the former ledge holder has `is_cliff_hold=0`
after the Fall branch, and a second selected fighter reaches the same Pupupu
right ledge through the source-order CliffCatch map/status path with no
same-cliff occupancy block. The previous CliffClimb action boundary then
consumes the climb-created CliffQuick/Ground state, calls original
`ftCommonCliffQuickProcUpdate`, reaches original
`ftCommonCliffClimbQuick1SetStatus`, calls original
`ftCommonCliffClimbQuick1ProcUpdate`, reaches the guarded
`ftAnimEndCheckSetStatus` path for `ftCommonCliffClimbQuick2SetStatus`, and
proves CliffClimbQuick2/Ground `88/76/0` on retained `cliff_id=3` with copied
`floor_line_id=3`. The previous CliffClimb common2 boundary then consumes that
created CliffClimbQuick2/Ground state through one bounded original
`ftCommonCliffCommon2ProcUpdate`, `ftCommonCliffCommon2ProcPhysics`, and
`ftCommonCliffClimbCommon2ProcMap` tick. It proves Quick2/Ground `88/76/0`
stays stable, retained `cliff_id=3` and copied `floor_line_id=3` survive the
update/physics/map calls, the guarded anim-end check does not fall through to
Wait/Fall, ground velocity transfer is reached once, the map ground-break
callback is reached once under guard, and no unsafe escape occurred. The
previous CliffClimb finish boundary consumes that common2-created
CliffClimbQuick2/Ground state through original
`ftCommonCliffCommon2ProcUpdate`, forces the guarded animation-end handoff
through `mpCommonSetFighterWaitOrFall`, reaches the bounded Wait
`ftMainSetStatus` path, and proves CliffClimbQuick2/Ground `88/76/0 ->`
Wait/Ground `10/4/0` with retained `cliff_id=3`, retained
`floor_line_id=3`, `playertag_wait=120`, restored special interrupt, and no
unsafe escape. The bounded `ftMainSetStatus` seam now applies the current
project-owned original-compatible common reset and the finish proof reports
the reset mask. The current CliffWait timeout/DamageFall boundary then returns
to the verified CliffWait/Ground state on real Pupupu `cliff_id=3`, sets
`fall_wait=1`, runs original `ftCommonCliffWaitProcInterrupt`, reaches the
guarded attack, escape, and climb/fall checks in source order, takes original
`ftCommonCliffWaitCheckFall`, calls the bounded
`ftCommonDamageFallSetStatusFromCliffWait` seam, and proves CliffWait/Ground
`85/73/0 ->` DamageFall/Air `57/50/1`. It also proves `fall_wait 1 -> 0`,
`cliffcatch_wait=30`, `tics_since_last_z=65536`, retained
`cliff_id=3`/`floor_line_id=3`, original-compatible status reset clears
`is_cliff_hold`, installs the original DamageFall callback pointers, and runs
one guarded original DamageFall interrupt/physics/map tick. That tick reaches
the original special-air, aerial-attack, aerial-jump, and HammerFall interrupt
checks, applies air gravity through `ftPhysicsApplyAirVelDriftFastFall`
(`velY 0 -> -4000` in milli-units), calls original
`ftCommonDamageFallProcMap`, and proves the no-collision
`mpCommonCheckFighterCliff == FALSE` branch without passive/down-bounce
escape. It then reseeds the same guarded DamageFall map path for a positive
right-cliff collision and routes through the original
`mpCommonCheckFighterCliff` / `ftCommonCliffCatchSetStatus` branch, proving
CliffCatch/Air `84/72/1`, restored `is_cliff_hold`, installed cliff damage and
CliffCatch/Common callbacks, `floor_line_id=-1`, right-cliff masks `0x2000`,
and capture mask `0x4`. The next two guarded positive floor-collision passes
import bounded original `ftcommonpassivestand.c` and `ftcommonpassive.c`,
prove the buffered-stick PassiveStandF/Ground setup `73/62/0`, then prove the
buffered-neutral Passive/Ground setup `81/70/0`. Both pass through the
original DamageFall map callback, original passive checks, original
ground-placement call, bounded `ftMainSetStatus` contract, and the
velocity-transfer seam, then run one guarded original physics/map callback
pair and one original `ftAnimEndSetWait` handoff into the bounded Wait seam.
The new Passive-loop boundary consumes the same verified passive setup states
in separate direct/menu harnesses, runs two guarded stable update/physics/map
frames for PassiveStandF/Ground and Passive/Ground, then forces the original
animation-end update handoff into Wait/Ground. The handoff records
PassiveStand `73/62/0 -> cb1/1 -> 10/4/0` and Passive
`81/70/0 -> cb1/1 -> 10/4/0`, stable callback counts `3/2/2` per branch,
one Wait `ftMainSetStatus` call per branch, `playertag_wait=120`, and valid
Wait callbacks while keeping the common status reset active. A final guarded
positive floor-collision pass imports
bounded original `ftcommondownwaitbounce.c` update-effects tail, falls through
the passive checks, calls `ftCommonDownBounceSetStatus`, and proves
DownBounceU/Ground `68/59/0` with ground placement, bounded status install,
imported ImpactWave/FGM/rumble effects, velocity-transfer, cleared attack
buffer, and `damage_mul=0.5` diagnostics.
The same bounded proof now runs original
DownBounce update callbacks twice: an A-button tap fills `attack_buffer=60`
while the fighter stays DownBounceU/Ground, then the animation-end branch
calls original DownBounce attack and forward/back checks, reaches the bounded
DownWaitU setup path with motion/script sentinel `-2`, capture immune mask
`0x33`, `damage_mul=0.5`, and `stand_wait=180`. It then runs one original
DownWait update tick to prove `stand_wait 180 -> 179` without falling through
to DownStand, followed by a bounded timeout tick that reaches original
DownStandU/Ground `72/61/0`, clears the wakeup flag, keeps `proc_damage`
cleared, and restores `damage_mul=1.0` through the bounded common status reset.
The previous DownWait-loop boundary consumes the Passive-loop proof, primes a
fresh DownBounceU/Ground shell, routes original `ftCommonDownWaitSetStatus`
through the project-owned bounded `ftMainSetStatus` seam, then calls the
installed original DownWait interrupt callback. It proves source order
`0x12345`: DownAttack check, forward/back check, DownStand check, bounded
DownStand status request, and bounded DownStand `ftMainSetStatus`. The proof
also runs guarded original DownWait recovery-input branches: A-button reaches
DownAttackU `80/69`, forward stick reaches DownForwardU `76/65`, and back
stick reaches DownBackU `78/67`, with the original set-status and animation
event calls observed. The DownAttack set-status branch now also proves original
attack IDs `motion=53`, `status=33`, and `stat=33` for DownAttackU. Each
attack/roll recovery branch now runs eight guarded stable update/physics/map
callback frames, with the roll branches proving bounded root motion
`+10000/-10000` milli through the proof-owned ground-velocity transfer seam,
then
proves the original `ftAnimEndCheckSetStatus` handoff back to Wait/Ground
`10/4/0` with `playertag_wait=120` and valid Wait callbacks. The standing
branch records DownWaitU/Ground
`70/-2/0`, `stand_wait=180`, capture mask `0x33`, `damage_mul=0.5`, input
stick `0,80`, and final DownStandU/Ground `72/61/0` with `flag1` cleared,
eight guarded stable DownStand callback frames, and `damage_mul=1.0`.
The current DamageFall proof
mask is `0x1ffffff`; the Dash-Run procparams proof mask is `0xffffffff`; the
CliffClimb finish handoff still requires common-reset
mask `0x7ffff`, covering reflect, absorb, shield, fastfall, invisible,
shadow hide, player-tag hide, ghost, hitstun, cliff hold, jostle ignore,
damage player, ignored line, capture immune mask, camera zoom range, shuffle
tics, knockback resistance, stacked knockback, and damage multiplier reset.
CliffClimb drop,
release-then-recatch, Quick2, common2, and finish diagnostics assert stale
ledge hold/jostle state is cleared after status changes. Every current
`ftCommonCliffCommon2UpdateCollData` wrapper entry now routes through the
DS common2 bridge, so the fallback path receives the same queued-status range
guard and root snap diagnostics as the guarded action proofs. The import keeps BattleShip's common2 helpers under
`ndsBase*` names, while the bounded animation-end seam invokes the same
source-order Quick2 setup through project-owned common2 wrappers so the live
`FTCollisionData` shell receives the original ledge-floor update safely. The
current bridge contract is still a bounded harness seam, not permission to open
unbounded natural ledge runtime. Full
attack hitbox/damage logic, natural ledge occupancy/release/drop/climb behavior,
continuous PassiveStand/Passive action runtime beyond the current bounded
two-frame stable callback proof plus anim-end-to-Wait handoff,
natural-motion ceiling gameplay, wall hits, and platform behavior remain
deferred. The previous
motion-stale harnesses seed that same valid-stale pair into the selected P0
root/collision shell, run the selected callback through source-order
`mpProcessUpdateMain` and `mpCommonRunFighterAllCollisions`, accept Dream Land
floor line `1 -> 0` through `mpProcessCheckTestFloorCollision`, copy the
target floor back to the live fighter state, keep P1 grounded on line `3`, and
remain regression coverage. The previous cliff-status harnesses import
original `ftcommonottotto.c` and prove the bounded source-order
`mpCommonProcFighterOnCliffEdge` status branch after that motion-stale floor
proof: a probe with `MAP_FLAG_FLOOREDGE` reaches original Ottotto
status/motion `36/30` for P0, while a probe without that flag reaches original
Fall status/motion `26/20` and air state for P1. The previous cliff-tick
harnesses then consume those created statuses without opening gameplay
scheduling: P0 runs one guarded original `ftCommonOttottoProcUpdate`,
`ftCommonOttottoProcInterrupt`, and `ftCommonOttottoProcMap` tick and remains
Ottotto/Ground on floor line `0`; P1 runs one guarded original
`ftCommonFallProcInterrupt` tick, reaches the three original air interrupt
checks, and remains Fall/Air on floor line `3`. The previous Fall-map
harnesses consume that P1 Fall state, call the selected status-table original
`ftPhysicsApplyAirVelDriftFastFall` path, record guarded fast-fall, gravity,
air-drift, and air-friction seams, integrate one bounded airborne step, then
call the selected original `mpCommonProcFighterCliffFloorCeil` map callback
through a guarded no-collision branch. P1 remains Fall/Air on floor line `3`
with decreasing root-Y and velocity-Y. The previous Fall-landing harnesses
consume that Fall-map proof, seed P1 just above decoded Pupupu floor line `3`,
cross the floor, call source-order landing-floor setup, reach original
LandingLight status/motion `31/25`, switch to Ground, and clamp vertical
velocity from `-8000` to `0`. Continuous Fall/Ottotto runtime, arbitrary live
cliff/offstage transitions, ledges/cliffcatch, full natural-motion ceiling
hits, platform pass-through, and full gameplay remain deferred. The
previous
`stage_mpwall_floor_loop` harnesses remain as the honest wall-blocker proof:
the selected Dream Land main floor's left/right side-wall candidates are the
same edge-under wall lines `6/5` that original
`mpProcessCheckFloorEdgeCollisionL/R` rejects. The same wall-floor verifiers
now also stage an isolated Hyrule Castle map scout and have found a concrete
source-order non-edge wall-hit candidate: Hyrule floor `5`, wall `13`,
edge-under `12`, left side, with adjustment delta `-1600/-388` in
milli-units. The previous
`stage_mpadjust_floor_loop` harnesses remain as the bounded floor-edge-adjust
wall-miss proof, `stage_mpcross_floor_loop` remains as the live accepted
second-floor proof, `stage_mpsweep_floor_loop` remains as the bounded
same-line reject, different-line standalone accept, and no-hit miss floor-sweep
proof, `stage_mpupdate_floor_loop` remains as the source-order update-main
floor proof, `stage_mpprocess_floor_loop` remains as the source-order
floor-test proof, `stage_floor_edge_loop` remains as the selected floor-edge
query proof, `stage_floor_follow_loop` remains as the continuous
selected-fighter floor-follow proof, and `stage_collision_loop` remains as the
final-sample floor projection proof.

## Next Boundary

The current playability-facing boundary is the direct/menu-chain selected
live-hit status-loop proof, registry modes `161/162`
(`battle_mariofox_stage_mplivehit_status_loop` and
`menu_chain_mariofox_stage_mplivehit_status_loop`). Treat
`.\scripts\verify-all.ps1 -Profile Boundary -List` as authoritative if this
summary ever drifts.

Good next slices are still bounded, source-backed, and restored after proof:
another selected attack's original rehit/window behavior, broader natural
multi-slot victim runtime beyond the restored slot-0 consume/repeat, natural
slot-1/2/4/5/6/7/8/9 consume follow-through, selected slot-3 bookkeeping
proofs, TaruCannon update/shoot runtime after the Jungle barrel helpers and
map throw-hit data exist locally, continuous shield runtime beyond the selected
contact/set-off/update-tick/health-decrement/break/heal/break-clear/tail-clear/
special-clear/hitlag-mul-clear proof, or the next small `ftcommondamage.c`
runtime callback not already covered.

Keep continuous player-driven gameplay, full map collision, full stage runtime,
item/weapon systems, audio, continuous full-scene draw, full fighter rendering,
and full renderer rewrites bounded or deferred.

## Known Blockers

- CliffAttack/Common2 and CliffEscape/Common2 are current-safe in modes
  `155/156` through delayed aggregate reseeding and isolated shared Common2
  bridge diagnostics. Continuous natural ledge attack/escape runtime remains
  deferred.
- Full Title input, animated logo, labels/Press Start, slash, logo-fire
  particles, audio, and continuous title drawing remain deferred.
- Full VS Mode navigation/rule editing/options transition, audio, and
  continuous menu drawing remain deferred.
- PlayersVS setup is imported, but full interactive cursor/puck selection,
  fighter actor runtime/rendering, and continuous character-select drawing
  remain deferred. The ready transition uses deterministic two-player state
  injection to prove original proceed behavior.
- Maps setup now proves the Pupupu/Dream Land preview path and real stage
  asset dependency chain, but full preview rendering fidelity and continuous
  Maps drawing remain deferred.
- VSBattle setup is imported only to the bounded setup/update/model proof.
  Stub fighter GObjs remain for older setup-only harnesses, while the
  Mario/Fox model/struct/init harnesses create asset-backed fighter GObjs from
  original descriptors, attach persistent project-owned `FTStruct` shells, and
  initialize a bounded original-source-order runtime state shell.
  Pupupu `MPGroundData` is adopted, and bounded original Pupupu ground object
  setup creates the display-layer and Whispy/flowers map GObjs. A guarded
  two-tick `grPupupuProcUpdate` proof covers only the safe Sleep -> Wait / wait
  countdown path. The Mario/Fox Wait ground proof covers only source-order
  ground friction, air-velocity transfer, and the safe floor branch of the
  cliff-map seam. The Mario/Fox Walk input proof covers one original Wait ->
  Walk transition, one Walk interrupt callback, one velocity-generation pass,
  and one safe map pass. The Walk-loop proof adds only four synthetic held Walk
  frames, root integration, neutral release back to Wait, and one Wait
  friction/map settle frame. The Dash/Run proof adds only a synthetic-input
  original Dash -> Run -> RunBrake path with bounded physics/map ticks. The
  Landing-loop proof adds a bounded original JumpF -> Fall -> LandingLight ->
  Wait path. The Process-loop proof adds a shared bounded source-order frame
  driver and deterministic scripted controller-input bridge that proves
  Wait -> Walk -> Wait, Wait -> Dash -> Run -> RunBrake -> Wait, and Wait ->
  KneeBend -> JumpF -> Fall -> LandingLight -> Wait for both Mario and Fox;
  the Scheduler-loop proof attaches selected Mario/Fox `GObjProcess` callbacks
  with original `gcAddGObjProcess`, invokes them through `gcRunGObjProcess`
  from bounded `scVSBattleFuncUpdate` taskman updates, and proves the same
  movement contract through that scheduler-facing path. The controller-loop
  proof routes deterministic `OSContPad` playback through original controller
  read/global-update code before the same process callbacks run. The new
  preview-loop proof then reuses that controller-source path and samples seven
  guarded `ftDisplayMainProcDisplay` frames into the bounded `96x72` software
  preview while Mario/Fox roots move. The `gcRunAll`-loop proof pauses previous
  proof-owned and non-target object processes, attaches selected Mario/Fox
  callbacks through original `gcAddGObjProcess`, and proves those callbacks are
  reached through bounded original `gcRunAll()` calls. The live-preview proof
  swaps the deterministic playback source for live DS controller reads through
  the original controller read/global-update bridge and proves a 60-frame
  neutral idle moving-preview boundary. The `gcDrawAll`-loop proof re-enables
  deterministic playback, advances the selected callbacks through original
  `gcRunAll`, and reaches Mario/Fox `ftDisplayMainProcDisplay` callbacks
  through bounded original `gcDrawAll()` keyframes while masking non-target
  display callbacks. The stage-inclusive `gcDrawAll` proof keeps that same
  object-manager draw traversal and additionally proves selected Pupupu display
  layer and map GObjs are captured by the camera path and reach the
  project-owned `gcDrawDObjTree*` bridges with complete DObj/DL-ready masks.
  The stage-collision proof keeps the older flat floor seam for older harnesses
  but, in modes `61/62`, projects Mario/Fox against real Pupupu geometry,
  adopts the current real floor under each proof-owned fighter, and records
  real line IDs, edges, floor flags, angles, probe hits/misses, and final
  Wait/Ground/Floor state. The stage-floor-follow proof builds on that
  geometry path in modes `63/64`, disables the final re-center/adopt shortcut,
  projects each selected proof-owned fighter against the real Pupupu floor
  during the bounded movement slice, clamps roots to decoded floor Y, records
  per-player update/hit counts, and proves zero final post-clamp floor drift.
  The floor-edge proof builds on that geometry path in modes `65/66`, keeps
  the no-final-recenter behavior, seeds the selected proof-owned roots near
  opposite ends of the widest decoded Pupupu floor, proves inside/outside
  `mpCollisionGetFCCommonFloor` queries, records line-type and vertex-position
  helper use, and deliberately records edge-under helper calls as deferred
  `-1` results until real wall/ledge/platform contracts are imported.
  Unpaused full-scene `gcRunAll`/`gcDrawAll`, continuous
  scheduling, arbitrary live input combinations,
  squat/attack/special/guard/catch
  paths, full collision,
  hit/catch/search, items, HUD, audio, and full imported `ftmain.c` remain
  deferred. The
  Mario/Fox display probe covers only metadata
  collection
  through the guarded display callback and records zero MObj/AObj counts on the
  current bounded DObj trees; exact original fighter joint-ID mapping,
  continuous stage
  update/draw, full collision line processing, Whispy wind, yakumono/stage
  object runtime, fighter process/gameplay, fighter display traversal,
  items/weapons
  runtime, audio backend, HUD rendering, full fighter display rendering, and
  gameplay remain deferred.
- The Mario/Fox DL draw proofs now cover the selected first-DL path, the first
  four DL-ready DObjs per fighter, and all current DL-ready Mario/Fox DObjs
  through a guarded `ftDisplayMainProcDisplay` seam into a bounded `96x72`
  software preview. Full fighter rendering still needs camera-correct battle
  projection, real matrix prep, material/texture upload, full
  `ftdisplaymain.c` traversal, shadows, magnify/interface rendering,
  continuous draw, and a DS hardware polygon path.
- Fighter/stage-heavy opening action scenes are still bounded bridge stubs in
  original order.
- Opening Room DObj rendering is still a bounded preview path, not a general
  display-list-to-DS hardware renderer.
- no$gba has only launch/window smoke automation. Use it interactively for
  VRAM/OAM/register/timing questions only when melonDS cannot answer them.
- Large maintenance files are being split for velocity:
  `src/port/scene_backend.c` and `scripts/verify-runtime.ps1`.

## Verifier Commands

Use PowerShell with devkitPro variables set:

```powershell
$env:DEVKITPRO = 'C:/devkitPro'
$env:DEVKITARM = 'C:/devkitPro/devkitARM'
make -j16
.\scripts\check-gbi-decode-fixtures.ps1
.\scripts\verify-runtime.ps1
.\scripts\verify-opening-skip.ps1
.\scripts\verify-opening-movie-speed.ps1
.\scripts\verify-title-harness.ps1
.\scripts\verify-vs-setup-harness.ps1
.\scripts\verify-vs-start-transition-harness.ps1
.\scripts\verify-players-vs-setup-harness.ps1
.\scripts\verify-maps-setup-harness.ps1
.\scripts\verify-battle-fd-harness.ps1
.\scripts\verify-battle-pupupu-stage-harness.ps1
.\scripts\verify-battle-pupupu-update-harness.ps1
.\scripts\verify-battle-mariofox-model-harness.ps1
.\scripts\verify-battle-mariofox-struct-harness.ps1
.\scripts\verify-battle-mariofox-init-harness.ps1
.\scripts\verify-battle-mariofox-wait-harness.ps1
.\scripts\verify-battle-mariofox-wait-tick-harness.ps1
.\scripts\verify-battle-mariofox-wait-ground-harness.ps1
.\scripts\verify-battle-mariofox-display-probe-harness.ps1
.\scripts\verify-battle-mariofox-dl-scan-harness.ps1
.\scripts\verify-battle-mariofox-dl-execute-harness.ps1
.\scripts\verify-battle-mariofox-dl-draw-harness.ps1
.\scripts\verify-battle-mariofox-dl-draw-multi-harness.ps1
.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1
.\scripts\verify-battle-mariofox-walk-input-harness.ps1
.\scripts\verify-battle-mariofox-walk-loop-harness.ps1
.\scripts\verify-battle-mariofox-dash-run-harness.ps1
.\scripts\verify-battle-mariofox-jump-loop-harness.ps1
.\scripts\verify-battle-mariofox-landing-loop-harness.ps1
.\scripts\verify-battle-mariofox-process-loop-harness.ps1
.\scripts\verify-battle-mariofox-scheduler-loop-harness.ps1
.\scripts\verify-battle-mariofox-controller-loop-harness.ps1
.\scripts\verify-battle-mariofox-preview-loop-harness.ps1
.\scripts\verify-battle-mariofox-gcrunall-loop-harness.ps1
.\scripts\verify-menu-chain-vsbattle-harness.ps1
.\scripts\verify-menu-chain-pupupu-update-harness.ps1
.\scripts\verify-menu-chain-mariofox-model-harness.ps1
.\scripts\verify-menu-chain-mariofox-struct-harness.ps1
.\scripts\verify-menu-chain-mariofox-init-harness.ps1
.\scripts\verify-menu-chain-mariofox-wait-harness.ps1
.\scripts\verify-menu-chain-mariofox-wait-tick-harness.ps1
.\scripts\verify-menu-chain-mariofox-wait-ground-harness.ps1
.\scripts\verify-menu-chain-mariofox-display-probe-harness.ps1
.\scripts\verify-menu-chain-mariofox-dl-scan-harness.ps1
.\scripts\verify-menu-chain-mariofox-dl-execute-harness.ps1
.\scripts\verify-menu-chain-mariofox-dl-draw-harness.ps1
.\scripts\verify-menu-chain-mariofox-dl-draw-multi-harness.ps1
.\scripts\verify-menu-chain-mariofox-dl-draw-all-harness.ps1
.\scripts\verify-menu-chain-mariofox-walk-input-harness.ps1
.\scripts\verify-menu-chain-mariofox-walk-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-dash-run-harness.ps1
.\scripts\verify-menu-chain-mariofox-jump-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-landing-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-process-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-scheduler-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-controller-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-preview-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-gcrunall-loop-harness.ps1
```

For a clean regression after header, Makefile, imported source, or linker-visible
symbol changes:

```powershell
make clean
make -j16
.\scripts\check-gbi-decode-fixtures.ps1
.\scripts\verify-regression.ps1
```

Build a direct Title harness ROM without replacing the normal output:

```powershell
make TARGET=smash64ds-title BUILD=build-title-harness NDS_DEV_SCENE_HARNESS=title -j16
.\scripts\verify-title-harness.ps1
```

Build a direct VS setup harness ROM without replacing the normal output:

```powershell
make TARGET=smash64ds-vs-setup BUILD=build-vs-setup-harness NDS_DEV_SCENE_HARNESS=vs_setup -j16
.\scripts\verify-vs-setup-harness.ps1
```

Build a direct VS Start transition harness ROM without replacing the normal
output:

```powershell
make TARGET=smash64ds-vs-start BUILD=build-vs-start-harness NDS_DEV_SCENE_HARNESS=vs_start_transition -j16
.\scripts\verify-vs-start-transition-harness.ps1
```

Build the direct PlayersVS, Maps, and menu-chain harness ROMs without replacing
the normal output:

```powershell
make TARGET=smash64ds-players-vs BUILD=build-players-vs-setup-harness NDS_DEV_SCENE_HARNESS=players_setup -j16
.\scripts\verify-players-vs-setup-harness.ps1
make TARGET=smash64ds-maps BUILD=build-maps-setup-harness NDS_DEV_SCENE_HARNESS=maps_setup -j16
.\scripts\verify-maps-setup-harness.ps1
make TARGET=smash64ds-battle-fd BUILD=build-battle-fd-harness NDS_DEV_SCENE_HARNESS=battle_fd -j16
.\scripts\verify-battle-fd-harness.ps1
make TARGET=smash64ds-battle-pupupu BUILD=build-battle-pupupu-stage-harness NDS_DEV_SCENE_HARNESS=battle_pupupu_stage -j16
.\scripts\verify-battle-pupupu-stage-harness.ps1
make TARGET=smash64ds-battle-pupupu-update BUILD=build-battle-pupupu-update-harness NDS_DEV_SCENE_HARNESS=battle_pupupu_update -j16
.\scripts\verify-battle-pupupu-update-harness.ps1
make TARGET=smash64ds-battle-mariofox-model BUILD=build-battle-mariofox-model-harness NDS_DEV_SCENE_HARNESS=battle_mariofox_model -j16
.\scripts\verify-battle-mariofox-model-harness.ps1
make TARGET=smash64ds-battle-mariofox-struct BUILD=build-battle-mariofox-struct-harness NDS_DEV_SCENE_HARNESS=battle_mariofox_struct -j16
.\scripts\verify-battle-mariofox-struct-harness.ps1
make TARGET=smash64ds-battle-mariofox-init BUILD=build-battle-mariofox-init-harness NDS_DEV_SCENE_HARNESS=battle_mariofox_init -j16
.\scripts\verify-battle-mariofox-init-harness.ps1
make TARGET=smash64ds-battle-mariofox-wait BUILD=build-battle-mariofox-wait-harness NDS_DEV_SCENE_HARNESS=battle_mariofox_wait -j16
.\scripts\verify-battle-mariofox-wait-harness.ps1
make TARGET=smash64ds-battle-mariofox-wait-tick BUILD=build-battle-mariofox-wait-tick-harness NDS_DEV_SCENE_HARNESS=battle_mariofox_wait_tick -j16
.\scripts\verify-battle-mariofox-wait-tick-harness.ps1
make TARGET=smash64ds-battle-mariofox-wait-ground BUILD=build-battle-mariofox-wait-ground-harness NDS_DEV_SCENE_HARNESS=battle_mariofox_wait_ground -j16
.\scripts\verify-battle-mariofox-wait-ground-harness.ps1
make TARGET=smash64ds-battle-mariofox-display-probe BUILD=build-battle-mariofox-display-probe-harness NDS_DEV_SCENE_HARNESS=battle_mariofox_display_probe -j16
.\scripts\verify-battle-mariofox-display-probe-harness.ps1
make TARGET=smash64ds-battle-mariofox-dl-scan BUILD=build-battle-mariofox-dl-scan-harness NDS_DEV_SCENE_HARNESS=battle_mariofox_dl_scan -j16
.\scripts\verify-battle-mariofox-dl-scan-harness.ps1
make TARGET=smash64ds-battle-mariofox-dl-execute BUILD=build-battle-mariofox-dl-execute-harness NDS_DEV_SCENE_HARNESS=battle_mariofox_dl_execute -j16
.\scripts\verify-battle-mariofox-dl-execute-harness.ps1
make TARGET=smash64ds-battle-mariofox-dl-draw BUILD=build-battle-mariofox-dl-draw-harness NDS_DEV_SCENE_HARNESS=battle_mariofox_dl_draw -j16
.\scripts\verify-battle-mariofox-dl-draw-harness.ps1
make TARGET=smash64ds-battle-mariofox-dl-draw-multi BUILD=build-battle-mariofox-dl-draw-multi-harness NDS_DEV_SCENE_HARNESS=battle_mariofox_dl_draw_multi -j16
.\scripts\verify-battle-mariofox-dl-draw-multi-harness.ps1
make TARGET=smash64ds-battle-mariofox-dl-draw-all BUILD=build-battle-mariofox-dl-draw-all-harness NDS_DEV_SCENE_HARNESS=battle_mariofox_dl_draw_all -j16
.\scripts\verify-battle-mariofox-dl-draw-all-harness.ps1
make TARGET=smash64ds-battle-mariofox-walk-loop BUILD=build-battle-mariofox-walk-loop-harness NDS_DEV_SCENE_HARNESS=battle_mariofox_walk_loop -j16
.\scripts\verify-battle-mariofox-walk-loop-harness.ps1
make TARGET=smash64ds-battle-mariofox-dash-run BUILD=build-battle-mariofox-dash-run-harness NDS_DEV_SCENE_HARNESS=battle_mariofox_dash_run -j16
.\scripts\verify-battle-mariofox-dash-run-harness.ps1
make TARGET=smash64ds-battle-mariofox-jump-loop BUILD=build-battle-mariofox-jump-loop-harness NDS_DEV_SCENE_HARNESS=battle_mariofox_jump_loop -j16
.\scripts\verify-battle-mariofox-jump-loop-harness.ps1
make TARGET=smash64ds-menu-chain BUILD=build-menu-chain-vsbattle-harness NDS_DEV_SCENE_HARNESS=menu_chain_vsbattle -j16
.\scripts\verify-menu-chain-vsbattle-harness.ps1
make TARGET=smash64ds-menu-chain-pupupu-update BUILD=build-menu-chain-pupupu-update-harness NDS_DEV_SCENE_HARNESS=menu_chain_pupupu_update -j16
.\scripts\verify-menu-chain-pupupu-update-harness.ps1
make TARGET=smash64ds-menu-chain-mariofox-model BUILD=build-menu-chain-mariofox-model-harness NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_model -j16
.\scripts\verify-menu-chain-mariofox-model-harness.ps1
make TARGET=smash64ds-menu-chain-mariofox-struct BUILD=build-menu-chain-mariofox-struct-harness NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_struct -j16
.\scripts\verify-menu-chain-mariofox-struct-harness.ps1
make TARGET=smash64ds-menu-chain-mariofox-init BUILD=build-menu-chain-mariofox-init-harness NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_init -j16
.\scripts\verify-menu-chain-mariofox-init-harness.ps1
make TARGET=smash64ds-menu-chain-mariofox-wait BUILD=build-menu-chain-mariofox-wait-harness NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_wait -j16
.\scripts\verify-menu-chain-mariofox-wait-harness.ps1
make TARGET=smash64ds-menu-chain-mariofox-wait-tick BUILD=build-menu-chain-mariofox-wait-tick-harness NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_wait_tick -j16
.\scripts\verify-menu-chain-mariofox-wait-tick-harness.ps1
make TARGET=smash64ds-menu-chain-mariofox-wait-ground BUILD=build-menu-chain-mariofox-wait-ground-harness NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_wait_ground -j16
.\scripts\verify-menu-chain-mariofox-wait-ground-harness.ps1
make TARGET=smash64ds-menu-chain-mariofox-display-probe BUILD=build-menu-chain-mariofox-display-probe-harness NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_display_probe -j16
.\scripts\verify-menu-chain-mariofox-display-probe-harness.ps1
make TARGET=smash64ds-menu-chain-mariofox-dl-scan BUILD=build-menu-chain-mariofox-dl-scan-harness NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_dl_scan -j16
.\scripts\verify-menu-chain-mariofox-dl-scan-harness.ps1
make TARGET=smash64ds-menu-chain-mariofox-dl-execute BUILD=build-menu-chain-mariofox-dl-execute-harness NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_dl_execute -j16
.\scripts\verify-menu-chain-mariofox-dl-execute-harness.ps1
make TARGET=smash64ds-menu-chain-mariofox-dl-draw BUILD=build-menu-chain-mariofox-dl-draw-harness NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_dl_draw -j16
.\scripts\verify-menu-chain-mariofox-dl-draw-harness.ps1
make TARGET=smash64ds-menu-chain-mariofox-dl-draw-multi BUILD=build-menu-chain-mariofox-dl-draw-multi-harness NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_dl_draw_multi -j16
.\scripts\verify-menu-chain-mariofox-dl-draw-multi-harness.ps1
make TARGET=smash64ds-menu-chain-mariofox-dl-draw-all BUILD=build-menu-chain-mariofox-dl-draw-all-harness NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_dl_draw_all -j16
.\scripts\verify-menu-chain-mariofox-dl-draw-all-harness.ps1
make TARGET=smash64ds-menu-chain-mariofox-walk-loop BUILD=build-menu-chain-mariofox-walk-loop-harness NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_walk_loop -j16
.\scripts\verify-menu-chain-mariofox-walk-loop-harness.ps1
make TARGET=smash64ds-menu-chain-mariofox-dash-run BUILD=build-menu-chain-mariofox-dash-run-harness NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_dash_run -j16
.\scripts\verify-menu-chain-mariofox-dash-run-harness.ps1
make TARGET=smash64ds-menu-chain-mariofox-jump-loop BUILD=build-menu-chain-mariofox-jump-loop-harness NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_jump_loop -j16
.\scripts\verify-menu-chain-mariofox-jump-loop-harness.ps1
```

## Latest Proof

Latest verified state after the direct and menu-chain Turn-loop proofs:

```text
make -j16
scripts/check-gbi-decode-fixtures.ps1 -> GBI decode fixtures passed: F3DEX2 VTX/TRI1/TRI2 packing and source snippets verified.
scripts/verify-vs-setup-harness.ps1 -> VS setup harness passed: scene=9/1 setup=0x1f files=2 sobj=28 buttons=0x3f
scripts/verify-vs-start-transition-harness.ps1 -> VS Start transition harness passed: scene=16/9 trans=0x56535452 mask=0xff saved=1/3/2
scripts/verify-players-vs-setup-harness.ps1 -> PlayersVS setup harness passed: files=7 mask=0xff sobj=65 slots=2/4/4
scripts/verify-maps-setup-harness.ps1 -> Maps setup harness passed: scene=21/16 setup=0xff files=5 preview=0xff assets=0x1f slot=6 gkind=6
scripts/verify-battle-fd-harness.ps1 -> Battle FD harness passed: files=8 players=1/0 fighters=1 gkind=16 mask=0x7f
scripts/verify-battle-pupupu-stage-harness.ps1 -> Battle Pupupu stage harness passed: scene=22/21 gkind=6 stage=0xff ground=0x3ff layers=4 mapGObjs=4 players=2 fighters=2
scripts/verify-battle-pupupu-update-harness.ps1 -> Battle Pupupu update harness passed: scene=22/21 update=0xff ticks=2 whispy=0->1 windWait=2->1 sidefx=0/0/0/0
scripts/verify-battle-mariofox-model-harness.ps1 -> Battle Mario/Fox model harness passed: assets=0xffff, setup=0xfff, realGObjs=2, p0DObjs=25, p1DObjs=27
scripts/verify-battle-mariofox-struct-harness.ps1 -> Battle Mario/Fox struct harness passed: scene=22/21 struct=0xfff pool=0x3 p0Joints=24 p1Joints=26 model=0xfff
scripts/verify-battle-mariofox-init-harness.ps1 -> Battle Mario/Fox init harness passed: scene=22/21 init=0x3fff p0GA=0 p1GA=0 floor=1/1 calls=2/2/2/2
scripts/verify-battle-mariofox-wait-harness.ps1 -> Battle Mario/Fox Wait harness passed: waitMask=0xfff count=2 status=10/10 motion=4/4 callbacks=0
scripts/verify-battle-mariofox-wait-tick-harness.ps1 -> Battle Mario/Fox Wait tick harness passed: scene=22/21 tick=0x3ff callbacks=2/2/2 stable=1 final=22
scripts/verify-battle-mariofox-wait-ground-harness.ps1 -> Battle Mario/Fox Wait ground harness passed: scene=22/21 ground=0x7ff vel=2000->0/2000->0 map=2/2 stable=1
scripts/verify-battle-mariofox-display-probe-harness.ps1 -> Battle Mario/Fox display probe harness passed: scene=22/21 display=0x7ff dobj=25/27 mobj=0/0 ready=14/18 stable=1
scripts/verify-battle-mariofox-dl-scan-harness.ps1 -> Battle Mario/Fox DL scan harness passed: scene=22/21 dl=0x22dc548/0x2304f10 asset=4294967294/4294967294 commands=59/69 blocker=0/0 safe=1
scripts/verify-battle-mariofox-dl-execute-harness.ps1 -> Battle Mario/Fox DL execute harness passed: commands=59/69 verts=28/23 tris=37/20 safe=1
scripts/verify-battle-mariofox-dl-draw-harness.ps1 -> Battle Mario/Fox DL draw harness passed: scene=22/21 pixels=4274/5345 tris=37/20 real=37/18 marker=0/2 preview=96x72 commit=1 safe=1
scripts/verify-battle-mariofox-dl-draw-multi-harness.ps1 -> Battle Mario/Fox multi-DL draw harness passed: scene=22/21 candidates=14/18 selected=4/4 pixels=6190/7026 tris=87/79 real=82/76 marker=1/3 clean=4/4 preview=96x72 safe=1
scripts/verify-battle-mariofox-dl-draw-all-harness.ps1 -> Battle Mario/Fox all-DL draw harness passed: scene=22/21 callbacks=2 candidates=14/18 selected=14/18 pixels=14913/13432 tris=334/322 real=306/290 marker=10/24 clean=14/18 failed=0/0 preview=96x72 safe=1
scripts/verify-battle-mariofox-walk-input-harness.ps1 -> Battle Mario/Fox Walk input harness passed: scene=22/21 status=12/13 motion=6/7 stick=40/80 vel=12000/36000 callbacks=2 safe=1
scripts/verify-battle-mariofox-walk-loop-harness.ps1 -> Battle Mario/Fox Walk loop harness passed: scene=22/21 frames=4/4 root-dx=48000/-144000 release=Wait vel=12000/36000->6000/28000 safe=1
scripts/verify-battle-mariofox-dash-run-harness.ps1 -> Battle Mario/Fox Dash-Run harness passed: wait->attack11=190/165 cb=0xff tick=0xff waitproc=0x3; guard=152/134 cb=0xff state=0xfffffe0f fgm=13; escape=156/136 cb=0x3ff state=0xff; attack11->attack12=191/166 cb=0xff goto=0xf; attack12->attack13=220/195 foxblock=191/166 cb=0xf goto=0x7; fox attack100start=220/195 cb=0xf goto=0x3; fox attack100loop=221/196 cb=0xf goto=0x3 tick=0xfff; anim=0x3f; dash->run->runbrake + attackdash=192/167 cb=0xff tick=0x3f runproc=0x3 root-dx=301575/-418500 vel=51200/71000->47450/66000
scripts/verify-battle-mariofox-jump-loop-harness.ps1 -> Battle Mario/Fox Jump-loop harness passed: jump dx=100800/138900 dy=395400/468000 vy=74300/92000->59900/68000 attackAir=209/184 map=0x3ff dir=0x1f
scripts/verify-battle-mariofox-landing-loop-harness.ps1 -> Battle Mario/Fox Landing-loop harness passed: scene=22/21 fall=26/26 landing=31/31 wait=10/10 frames=16/14 floor=0/0 safe=1
scripts/verify-battle-mariofox-process-loop-harness.ps1 -> Battle Mario/Fox process-loop harness passed: scene=22/21 frames=30/28 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff root-dx=137250/-214000 rise=74300/92000 final=Wait/Ground safe=1
scripts/verify-battle-mariofox-scheduler-loop-harness.ps1 -> Battle Mario/Fox scheduler-loop harness passed: scene=22/21 updates=30 callbacks=30/30 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff root-dx=137250/-214000 rise=74300/92000 final=Wait/Ground safe=1
scripts/verify-battle-mariofox-controller-loop-harness.ps1 -> Battle Mario/Fox controller-loop harness passed: scene=22/21 reads=30 updates=30 callbacks=30/30 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff root-dx=137250/-214000 rise=74300/92000 final=Wait/Ground safe=1
scripts/verify-battle-mariofox-preview-loop-harness.ps1 -> Battle Mario/Fox preview-loop harness passed: scene=22/21 drawFrames=7 callbacks=14 pixels=582 screenDx=61/-62 screenRise=52/52 rootDx=137250/-214000 final=Wait/Ground safe=1
scripts/verify-battle-mariofox-gcrunall-loop-harness.ps1 -> Battle Mario/Fox gcRunAll-loop harness passed: scene=22/21 gcRunAll=30 callbacks=30/30 draws=11 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1
scripts/verify-battle-mariofox-live-preview-harness.ps1 -> Battle Mario/Fox live-preview harness passed: scene=22/21 liveReads=60 frames=60/60 draws=16 pixels=1410 idle=Wait/Ground directInput=0 safe=1
scripts/verify-battle-mariofox-gcdrawall-loop-harness.ps1 -> Battle Mario/Fox gcDrawAll-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1
scripts/verify-battle-mariofox-stage-gcdrawall-loop-harness.ps1 -> Battle Mario/Fox stage gcDrawAll-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0
scripts/verify-battle-mariofox-stage-collision-loop-harness.ps1 -> Battle Mario/Fox stage collision-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=0/0 floorRange=0-4 floorLines=4 probes=3/2
scripts/verify-battle-mariofox-stage-floor-follow-loop-harness.ps1 -> Battle Mario/Fox stage floor-follow-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=0/0 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=0/0 updates=18/18 drift=0/0 visits=0x1/0x1
scripts/verify-battle-mariofox-stage-floor-edge-loop-harness.ps1 -> Battle Mario/Fox stage floor-edge-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=118750/42000 delta=137250/214000 probes=2/2 queries=6/4
scripts/verify-battle-mariofox-stage-mpprocess-floor-loop-harness.ps1 -> Battle Mario/Fox stage MP floor-process-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=118750/42000 delta=137250/214000 probes=2/2 queries=50/46 mpProcessFloor=test=38/2 project=0/2 probes=1/2 below=2 p0line=3 p1line=3 fc=2/1/39
scripts/verify-battle-mariofox-stage-mpupdate-floor-loop-harness.ps1 -> Battle Mario/Fox stage mpProcessUpdateMain floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=90/85 mpProcessFloor=test=42/3 project=0/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/36/42 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000
scripts/verify-battle-mariofox-stage-mpsweep-floor-loop-harness.ps1 -> Battle Mario/Fox stage MP sweep floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=90/85 mpProcessFloor=test=42/3 project=0/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/36/42 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000 mpSweepFloor=check=1/36 same=35/1 diff=1 line=3->0 second=34/34 p0line=3 p1line=3
scripts/verify-battle-mariofox-stage-mpcross-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP cross-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=138/133 mpProcessFloor=test=25/20 project=17/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/83/43 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000 mpSweepFloor=check=18/19 same=18/1 diff=18 line=3->0 second=34/17 p0line=3 p1line=3 mpCrossFloor=line=-1->3 live=17/17 accepted=17 p0line=3 p1line=3
scripts/verify-battle-mariofox-stage-mpadjust-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP adjust-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=138/133 mpProcessFloor=test=25/20 project=17/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/83/43 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000 mpSweepFloor=check=18/19 same=18/1 diff=18 line=3->0 second=34/17 p0line=3 p1line=3 mpCrossFloor=line=-1->3 live=17/17 accepted=17 p0line=3 p1line=3 mpAdjustFloor=run=17 check=17/17 wallMiss=17/17 edge=17/17 noAdjust=17 p0line=3 p1maintained=3
scripts/verify-battle-mariofox-stage-mpedge-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP edge-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=138/133 mpProcessFloor=test=25/20 project=17/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/83/43 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000 mpSweepFloor=check=18/19 same=18/1 diff=18 line=3->0 second=34/17 p0line=3 p1line=3 mpCrossFloor=line=-1->3 live=17/17 accepted=17 p0line=3 p1line=3 mpAdjustFloor=run=17 checkHit=0/0 checkMiss=17/17 wallHit=0/0 wallMiss=17/17 edge=18/18 adjust=0/0 noAdjust=17 p0line=3 p1maintained=3 mpEdgeFloor=edge=6/5 kind=3/2 calls=18/18
scripts/verify-battle-mariofox-stage-mpwall-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP wall-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpAdjustFloor=run=17 checkHit=0/0 checkMiss=17/17 wallHit=0/0 wallMiss=17/17 edge=18/18 adjust=0/0 noAdjust=17 mpEdgeFloor=edge=6/5 kind=3/2 calls=18/18 mpWallFloor=blocked floor=3 edgeWall=5 kind=2 side=1 candidates=2 miss=1 mpWallHitScout=none floors=4 walls=8 candidates=6 hyruleWallHit=hit floor=5 wall=13 edge=12 side=0 delta=-1600/-388
scripts/verify-battle-mariofox-stage-mpstale-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP stale-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpWallFloor=blocked floor=3 edgeWall=5 kind=2 side=1 candidates=2 miss=1 mpStaleFloor=line=1->0 live=18/0 accepted=1 p0line=0 p1line=3
scripts/verify-battle-mariofox-stage-mplivestale-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP live-stale-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpWallFloor=blocked floor=3 edgeWall=5 kind=2 side=1 candidates=2 miss=1 mpStaleFloor=line=1->0 live=1/0 accepted=1 p0line=0 p1line=3 mpLiveStaleFloor=line=1->0 selected=1 live=1/0 accepted=1 p0line=0 p1line=3
scripts/verify-battle-mariofox-stage-mpmotionstale-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP motion-stale-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000
scripts/verify-battle-mariofox-stage-mpcliffstatus-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP cliff-status-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3
scripts/verify-battle-mariofox-stage-mpclifftick-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP cliff-tick-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3
scripts/verify-battle-mariofox-stage-mpfallmap-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP Fall-map-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3 mpFallMap=phys=1 fast=1 grav=1 map=1 noColl=1 y=200000->194000
scripts/verify-battle-mariofox-stage-mpfallland-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP Fall-landing-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3 mpFallMap=phys=1 fast=1 grav=1 map=1 noColl=1 y=200000->194000 mpFallLand=phys=1 map=1 floor=1 land=1/0 status=26->31/25 y=4000->0 velY=-8000->-12000
scripts/verify-battle-mariofox-stage-mpceil-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP Ceiling-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3 mpFallMap=phys=1 fast=1 grav=1 map=1 noColl=1 y=200000->194000 mpFallLand=phys=1 map=1 floor=1 land=1/0 status=26->31/25 y=4000->0 velY=-8000->-12000 mpCeilFloor=line=4 kind=1 check=1/1 diff=1/1 fc=2/2 y=-1464000->-1472000 ceil=-1072000 dist=-8000
scripts/verify-battle-mariofox-stage-mpceilstatus-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP Ceiling-status floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCeilFloor=line=4 kind=1 check=2/2 diff=2/2 fc=4/4 y=-1464000->-1472000 ceil=-1072000 dist=-8000 mpCeilStatus=line=4 status=26->66 motion=20->57 ga=1->0 velY=40000->0 masks=0x4400/0x400
scripts/verify-battle-mariofox-stage-mpcliffcatch-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP Cliff-catch floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3 mpFallMap=phys=1 fast=1 grav=1 map=1 noColl=1 y=200000->194000 mpFallLand=phys=1 map=1 floor=1 land=1/0 status=26->31/25 y=4000->0 velY=-8000->-12000 mpCeilFloor=line=4 kind=1 check=4/2 diff=4/2 fc=4/4 y=-1464000->-1472000 ceil=-1072000 dist=-8000 mpCeilStatus=line=4 status=26->66 motion=20->57 ga=1->0 velY=40000->0 masks=0x4400/0x400 mpCliffCatch=line=3 side=1 status=26->84 motion=20->72 occ=1/0 ledge=2318000/0 root=2518000,-408000->2318000,-408000 masks=0x2000/0x2000
scripts/verify-battle-mariofox-stage-mpcliffwait-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP Cliff-wait floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3 mpFallMap=phys=1 fast=1 grav=1 map=1 noColl=1 y=200000->194000 mpFallLand=phys=1 map=1 floor=1 land=1/0 status=26->31/25 y=4000->0 velY=-8000->-12000 mpCeilFloor=line=4 kind=1 check=4/2 diff=4/2 fc=4/4 y=-1464000->-1472000 ceil=-1072000 dist=-8000 mpCeilStatus=line=4 status=26->66 motion=20->57 ga=1->0 velY=40000->0 masks=0x4400/0x400 mpCliffCatch=line=3 side=1 status=26->84 motion=20->72 occ=1/0 ledge=2318000/0 root=2518000,-408000->2318000,-408000 masks=0x2000/0x2000 mpCliffWait=status=84->85->85 motion=72->73->73 fallWait=1080->1079 cliff=3 guards=1/1/1
scripts/verify-battle-mariofox-stage-mpcliffattack-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP Cliff-attack floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffWait=status=84->85->85 motion=72->73->73 fallWait=1080->1079 cliff=3 guards=1/1/1 mpCliffAttack=status=85->86 motion=73->74 queue=1 cliff=3 button=0x8000
scripts/verify-battle-mariofox-stage-mpcliffattack-action-loop-harness.ps1 -> Battle Mario/Fox Stage MP Cliff-attack action-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffAttack=status=85->86 motion=73->74 queue=1 cliff=3 button=0x8000 mpCliffAttackAction=status=86->92->93 motion=74->80->81 cliff=3 floor=3 calls=1/1/1
scripts/verify-battle-mariofox-stage-mpcliffcommon2-loop-harness.ps1 -> Battle Mario/Fox Stage MP CliffCommon2-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffAttackAction=status=86->92->93 motion=74->80->81 cliff=3 floor=3 calls=1/1/1 mpCliffCommon2=status=93->93->93->93 motion=81->81->81->81 cliff=3 floor=3->3 calls=1/1/1
scripts/verify-battle-mariofox-stage-mpcliffescape-action-loop-harness.ps1 -> Battle Mario/Fox Stage MP Cliff-escape action-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffEscapeAction=status=85->86->96->97 motion=73->74->84->85 cliff=3 floor=3 calls=1/1/1
scripts/verify-battle-mariofox-stage-mpcliffescape-common2-loop-harness.ps1 -> Battle Mario/Fox Stage MP CliffEscape Common2-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffEscapeAction=status=85->86->96->97 motion=73->74->84->85 cliff=3 floor=3 calls=1/1/1 mpCliffEscapeCommon2=status=97->97->97->97 motion=85->85->85->85 cliff=3 floor=3->3 calls=1/1/1
scripts/verify-battle-mariofox-stage-mpcliffclimb-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP Cliff-climb floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffClimb=climbStatus=86/74 dropStatus=26/20 cliff=3 sticks=0,80/80,0 calls=2/2
scripts/verify-battle-mariofox-stage-mpcliffclimb-action-loop-harness.ps1 -> Battle Mario/Fox Stage MP Cliff-climb action-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffClimb=climbStatus=86/74 dropStatus=26/20 cliff=3 sticks=0,80/80,0 calls=2/2 mpCliffClimbAction=status=86->87->88 motion=74->75->76 cliff=3 floor=3 calls=1/1/1
scripts/verify-battle-mariofox-stage-mpcliffclimb-common2-loop-harness.ps1 -> Battle Mario/Fox Stage MP Cliff-climb common2-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffClimb=climbStatus=86/74 dropStatus=26/20 cliff=3 sticks=0,80/80,0 calls=2/2 mpCliffClimbAction=status=86->87->88 motion=74->75->76 cliff=3 floor=3 calls=1/1/1 mpCliffClimbCommon2=status=88->88->88->88 motion=76->76->76->76 cliff=3 floor=3->3 calls=1/1/1
scripts/verify-battle-mariofox-stage-mpcliffclimb-finish-loop-harness.ps1 -> Battle Mario/Fox Stage MP Cliff-climb finish-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffClimbFinish=status=88->10 motion=76->4 cliff=3 floor=3->3 reset=0/0/0x7ffff calls=1/1/1
scripts/verify-battle-mariofox-stage-mpcliffwait-damage-loop-harness.ps1 -> Battle Mario/Fox Stage MP Cliff-wait damage-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffWaitDamage=status=85->57 motion=73->50 fallWait=1->0 catch=30 tics=65536 hold=1->0 procDmg=0 dmgTick=1/1/5 velY=0->-4000 cliffCatch=84/72/1 hold=1 passiveStand=73/62/0->cb1/1->10/4/0 passive=81/70/0->cb1/1->10/4/0 downBounce=68/59/0 dbuf=60 downWait=70/-2 wait=180->179 downStand=72/61 flag1=1->0 dmgMul=1000 coll=4
scripts/verify-menu-chain-vsbattle-harness.ps1 -> Menu-chain VSBattle harness passed: VS->PV mask=0xff, PV->Maps mask=0xff, Maps preview=0xff, Maps->VSBattle mask=0xff, stage=0xff, ground=0x3ff, final=22/21
scripts/verify-menu-chain-pupupu-update-harness.ps1 -> Menu-chain Pupupu update harness passed: VS->PV mask=0xff, PV->Maps mask=0xff, Maps->VSBattle mask=0xff, ground=0x3ff, update=0xff, final=22/21
scripts/verify-menu-chain-mariofox-model-harness.ps1 -> Menu-chain Mario/Fox model harness passed: chain masks=0xff/0xff/0xff, assets=0xffff, setup=0xfff, realGObjs=2
scripts/verify-menu-chain-mariofox-struct-harness.ps1 -> Menu-chain Mario/Fox struct harness passed: chain masks=0xff/0xff/0xff, model=0xfff, struct=0xfff, final=22/21
scripts/verify-menu-chain-mariofox-init-harness.ps1 -> Menu-chain Mario/Fox init harness passed: chain masks=0xff/0xff/0xff, model=0xfff, struct=0xfff, init=0x3fff, final=22/21
scripts/verify-menu-chain-mariofox-wait-harness.ps1 -> Menu-chain Mario/Fox Wait harness passed: chain final=22/21 waitMask=0xfff count=2 status=10/10 motion=4/4 callbacks=0
scripts/verify-menu-chain-mariofox-wait-tick-harness.ps1 -> Menu-chain Mario/Fox Wait tick harness passed: chain final=22/21 wait=0xfff tick=0x3ff callbacks=2/2/2 stable=1
scripts/verify-menu-chain-mariofox-wait-ground-harness.ps1 -> Menu-chain Mario/Fox Wait ground harness passed: chain final=22/21 wait=0xfff tick=0x3ff ground=0x7ff map=2/2 stable=1
scripts/verify-menu-chain-mariofox-display-probe-harness.ps1 -> Menu-chain Mario/Fox display probe harness passed: chain final=22/21 display=0x7ff dobj=25/27 ready=14/18 stable=1
scripts/verify-menu-chain-mariofox-dl-scan-harness.ps1 -> Menu-chain Mario/Fox DL scan harness passed: chain final=22/21 dl=0x22dcbe8/0x23055b0 asset=4294967294/4294967294 commands=59/69 blocker=0/0 safe=1
scripts/verify-menu-chain-mariofox-dl-execute-harness.ps1 -> Menu-chain Mario/Fox DL execute harness passed: chain final=22/21 commands=59/69 verts=28/23 tris=37/20 safe=1
scripts/verify-menu-chain-mariofox-dl-draw-harness.ps1 -> Menu-chain Mario/Fox DL draw harness passed: chain final=22/21 pixels=4274/5345 tris=37/20 real=37/18 marker=0/2 preview=96x72 commit=1 safe=1
scripts/verify-menu-chain-mariofox-dl-draw-multi-harness.ps1 -> Menu-chain Mario/Fox multi-DL draw harness passed: chain final=22/21 candidates=14/18 selected=4/4 pixels=6190/7026 tris=87/79 real=82/76 marker=1/3 clean=4/4 preview=96x72 safe=1
scripts/verify-menu-chain-mariofox-dl-draw-all-harness.ps1 -> Menu-chain Mario/Fox all-DL draw harness passed: chain final=22/21 callbacks=2 candidates=14/18 selected=14/18 pixels=14913/13432 tris=334/322 real=306/290 marker=10/24 clean=14/18 failed=0/0 preview=96x72 safe=1
scripts/verify-menu-chain-mariofox-walk-input-harness.ps1 -> Menu-chain Mario/Fox Walk input harness passed: chain final=22/21 status=12/13 motion=6/7 stick=40/80 vel=12000/36000 callbacks=2 safe=1
scripts/verify-menu-chain-mariofox-walk-loop-harness.ps1 -> Menu-chain Mario/Fox Walk loop harness passed: chain final=22/21 frames=4/4 root-dx=48000/-144000 release=Wait vel=12000/36000->6000/28000 safe=1
scripts/verify-menu-chain-mariofox-dash-run-harness.ps1 -> Menu-chain Mario/Fox Dash-Run harness passed: chain final=22/21 wait->attack11=190/165 cb=0xff tick=0xff waitproc=0x3; guard=152/134 cb=0xff state=0xfffffe0f fgm=13; escape=156/136 cb=0x3ff state=0xff; attack11->attack12=191/166 cb=0xff goto=0xf; attack12->attack13=220/195 foxblock=191/166 cb=0xf goto=0x7; fox attack100start=220/195 cb=0xf goto=0x3; fox attack100loop=221/196 cb=0xf goto=0x3 tick=0xfff; anim=0x3f; dash->run->runbrake + attackdash=192/167 cb=0xff tick=0x3f runproc=0x3 root-dx=301575/-418500 vel=51200/71000->47450/66000
scripts/verify-menu-chain-mariofox-jump-loop-harness.ps1 -> Menu-chain Mario/Fox Jump-loop harness passed: jump dx=100800/138900 dy=395400/468000 vy=74300/92000->59900/68000 attackAir=209/184 map=0x3ff dir=0x1f
scripts/verify-menu-chain-mariofox-landing-loop-harness.ps1 -> Menu-chain Mario/Fox Landing-loop harness passed: scene=22/21 fall=26/26 landing=31/31 wait=10/10 frames=16/14 floor=0/0 safe=1
scripts/verify-menu-chain-mariofox-process-loop-harness.ps1 -> Menu-chain Mario/Fox process-loop harness passed: scene=22/21 frames=30/28 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff root-dx=137250/-214000 rise=74300/92000 final=Wait/Ground safe=1
scripts/verify-menu-chain-mariofox-scheduler-loop-harness.ps1 -> Menu-chain Mario/Fox scheduler-loop harness passed: scene=22/21 updates=30 callbacks=30/30 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff root-dx=137250/-214000 rise=74300/92000 final=Wait/Ground safe=1
scripts/verify-menu-chain-mariofox-controller-loop-harness.ps1 -> Menu-chain Mario/Fox controller-loop harness passed: scene=22/21 reads=30 updates=30 callbacks=30/30 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff root-dx=137250/-214000 rise=74300/92000 final=Wait/Ground safe=1
scripts/verify-menu-chain-mariofox-preview-loop-harness.ps1 -> Menu-chain Mario/Fox preview-loop harness passed: scene=22/21 drawFrames=7 callbacks=14 pixels=582 screenDx=61/-62 screenRise=52/52 rootDx=137250/-214000 final=Wait/Ground safe=1
scripts/verify-menu-chain-mariofox-gcrunall-loop-harness.ps1 -> Menu-chain Mario/Fox gcRunAll-loop harness passed: scene=22/21 gcRunAll=30 callbacks=30/30 draws=11 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1
scripts/verify-menu-chain-mariofox-live-preview-harness.ps1 -> Menu-chain Mario/Fox live-preview harness passed: scene=22/21 liveReads=60 frames=60/60 draws=16 pixels=1410 idle=Wait/Ground directInput=0 safe=1
scripts/verify-menu-chain-mariofox-gcdrawall-loop-harness.ps1 -> Menu-chain Mario/Fox gcDrawAll-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1
scripts/verify-menu-chain-mariofox-stage-gcdrawall-loop-harness.ps1 -> Menu-chain Mario/Fox stage gcDrawAll-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0
scripts/verify-menu-chain-mariofox-stage-collision-loop-harness.ps1 -> Menu-chain Mario/Fox stage collision-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=0/0 floorRange=0-4 floorLines=4 probes=3/2
scripts/verify-menu-chain-mariofox-stage-floor-follow-loop-harness.ps1 -> Menu-chain Mario/Fox stage floor-follow-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=0/0 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=0/0 updates=18/18 drift=0/0 visits=0x1/0x1
scripts/verify-menu-chain-mariofox-stage-floor-edge-loop-harness.ps1 -> Menu-chain Mario/Fox stage floor-edge-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=118750/42000 delta=137250/214000 probes=2/2 queries=6/4
scripts/verify-menu-chain-mariofox-stage-mpprocess-floor-loop-harness.ps1 -> Menu-chain Mario/Fox stage MP floor-process-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=118750/42000 delta=137250/214000 probes=2/2 queries=50/46 mpProcessFloor=test=38/2 project=0/2 probes=1/2 below=2 p0line=3 p1line=3 fc=2/1/39
scripts/verify-menu-chain-mariofox-stage-mpupdate-floor-loop-harness.ps1 -> Menu-chain Mario/Fox stage mpProcessUpdateMain floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=90/85 mpProcessFloor=test=42/3 project=0/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/36/42 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000
scripts/verify-menu-chain-mariofox-stage-mpsweep-floor-loop-harness.ps1 -> Menu-chain Mario/Fox stage MP sweep floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=90/85 mpProcessFloor=test=42/3 project=0/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/36/42 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000 mpSweepFloor=check=1/36 same=35/1 diff=1 line=3->0 second=34/34 p0line=3 p1line=3
scripts/verify-menu-chain-mariofox-stage-mpcross-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP cross-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=138/133 mpProcessFloor=test=25/20 project=17/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/83/43 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000 mpSweepFloor=check=18/19 same=18/1 diff=18 line=3->0 second=34/17 p0line=3 p1line=3 mpCrossFloor=line=-1->3 live=17/17 accepted=17 p0line=3 p1line=3
scripts/verify-menu-chain-mariofox-stage-mpadjust-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP adjust-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=138/133 mpProcessFloor=test=25/20 project=17/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/83/43 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000 mpSweepFloor=check=18/19 same=18/1 diff=18 line=3->0 second=34/17 p0line=3 p1line=3 mpCrossFloor=line=-1->3 live=17/17 accepted=17 p0line=3 p1line=3 mpAdjustFloor=run=17 check=17/17 wallMiss=17/17 edge=17/17 noAdjust=17 p0line=3 p1maintained=3
scripts/verify-menu-chain-mariofox-stage-mpedge-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP edge-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=138/133 mpProcessFloor=test=25/20 project=17/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/83/43 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000 mpSweepFloor=check=18/19 same=18/1 diff=18 line=3->0 second=34/17 p0line=3 p1line=3 mpCrossFloor=line=-1->3 live=17/17 accepted=17 p0line=3 p1line=3 mpAdjustFloor=run=17 checkHit=0/0 checkMiss=17/17 wallHit=0/0 wallMiss=17/17 edge=18/18 adjust=0/0 noAdjust=17 p0line=3 p1maintained=3 mpEdgeFloor=edge=6/5 kind=3/2 calls=18/18
scripts/verify-menu-chain-mariofox-stage-mpwall-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP wall-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpAdjustFloor=run=17 checkHit=0/0 checkMiss=17/17 wallHit=0/0 wallMiss=17/17 edge=18/18 adjust=0/0 noAdjust=17 mpEdgeFloor=edge=6/5 kind=3/2 calls=18/18 mpWallFloor=blocked floor=3 edgeWall=5 kind=2 side=1 candidates=2 miss=1 mpWallHitScout=none floors=4 walls=8 candidates=6 hyruleWallHit=hit floor=5 wall=13 edge=12 side=0 delta=-1600/-388
scripts/verify-menu-chain-mariofox-stage-mpstale-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP stale-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpWallFloor=blocked floor=3 edgeWall=5 kind=2 side=1 candidates=2 miss=1 mpStaleFloor=line=1->0 live=18/0 accepted=1 p0line=0 p1line=3
scripts/verify-menu-chain-mariofox-stage-mplivestale-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP live-stale-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpWallFloor=blocked floor=3 edgeWall=5 kind=2 side=1 candidates=2 miss=1 mpStaleFloor=line=1->0 live=1/0 accepted=1 p0line=0 p1line=3 mpLiveStaleFloor=line=1->0 selected=1 live=1/0 accepted=1 p0line=0 p1line=3
scripts/verify-menu-chain-mariofox-stage-mpmotionstale-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP motion-stale-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000
scripts/verify-menu-chain-mariofox-stage-mpcliffstatus-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP cliff-status-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3
scripts/verify-menu-chain-mariofox-stage-mpclifftick-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP cliff-tick-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3
scripts/verify-menu-chain-mariofox-stage-mpfallmap-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP Fall-map-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3 mpFallMap=phys=1 fast=1 grav=1 map=1 noColl=1 y=200000->194000
scripts/verify-menu-chain-mariofox-stage-mpfallland-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP Fall-landing-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3 mpFallMap=phys=1 fast=1 grav=1 map=1 noColl=1 y=200000->194000 mpFallLand=phys=1 map=1 floor=1 land=1/0 status=26->31/25 y=4000->0 velY=-8000->-12000
scripts/verify-menu-chain-mariofox-stage-mpceil-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP Ceiling-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3 mpFallMap=phys=1 fast=1 grav=1 map=1 noColl=1 y=200000->194000 mpFallLand=phys=1 map=1 floor=1 land=1/0 status=26->31/25 y=4000->0 velY=-8000->-12000 mpCeilFloor=line=4 kind=1 check=1/1 diff=1/1 fc=2/2 y=-1464000->-1472000 ceil=-1072000 dist=-8000
scripts/verify-menu-chain-mariofox-stage-mpceilstatus-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP Ceiling-status floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCeilFloor=line=4 kind=1 check=2/2 diff=2/2 fc=4/4 y=-1464000->-1472000 ceil=-1072000 dist=-8000 mpCeilStatus=line=4 status=26->66 motion=20->57 ga=1->0 velY=40000->0 masks=0x4400/0x400
scripts/verify-menu-chain-mariofox-stage-mpcliffcatch-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP Cliff-catch floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3 mpFallMap=phys=1 fast=1 grav=1 map=1 noColl=1 y=200000->194000 mpFallLand=phys=1 map=1 floor=1 land=1/0 status=26->31/25 y=4000->0 velY=-8000->-12000 mpCeilFloor=line=4 kind=1 check=4/2 diff=4/2 fc=4/4 y=-1464000->-1472000 ceil=-1072000 dist=-8000 mpCeilStatus=line=4 status=26->66 motion=20->57 ga=1->0 velY=40000->0 masks=0x4400/0x400 mpCliffCatch=line=3 side=1 status=26->84 motion=20->72 occ=1/0 ledge=2318000/0 root=2518000,-408000->2318000,-408000 masks=0x2000/0x2000
scripts/verify-menu-chain-mariofox-stage-mpcliffwait-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP Cliff-wait floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3 mpFallMap=phys=1 fast=1 grav=1 map=1 noColl=1 y=200000->194000 mpFallLand=phys=1 map=1 floor=1 land=1/0 status=26->31/25 y=4000->0 velY=-8000->-12000 mpCeilFloor=line=4 kind=1 check=4/2 diff=4/2 fc=4/4 y=-1464000->-1472000 ceil=-1072000 dist=-8000 mpCeilStatus=line=4 status=26->66 motion=20->57 ga=1->0 velY=40000->0 masks=0x4400/0x400 mpCliffCatch=line=3 side=1 status=26->84 motion=20->72 occ=1/0 ledge=2318000/0 root=2518000,-408000->2318000,-408000 masks=0x2000/0x2000 mpCliffWait=status=84->85->85 motion=72->73->73 fallWait=1080->1079 cliff=3 guards=1/1/1
scripts/verify-menu-chain-mariofox-stage-mpcliffattack-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP Cliff-attack floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffWait=status=84->85->85 motion=72->73->73 fallWait=1080->1079 cliff=3 guards=1/1/1 mpCliffAttack=status=85->86 motion=73->74 queue=1 cliff=3 button=0x8000
scripts/verify-menu-chain-mariofox-stage-mpcliffattack-action-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP Cliff-attack action-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffAttack=status=85->86 motion=73->74 queue=1 cliff=3 button=0x8000 mpCliffAttackAction=status=86->92->93 motion=74->80->81 cliff=3 floor=3 calls=1/1/1
scripts/verify-menu-chain-mariofox-stage-mpcliffcommon2-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP CliffCommon2-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffAttackAction=status=86->92->93 motion=74->80->81 cliff=3 floor=3 calls=1/1/1 mpCliffCommon2=status=93->93->93->93 motion=81->81->81->81 cliff=3 floor=3->3 calls=1/1/1
scripts/verify-menu-chain-mariofox-stage-mpcliffescape-action-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP Cliff-escape action-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffEscapeAction=status=85->86->96->97 motion=73->74->84->85 cliff=3 floor=3 calls=1/1/1
scripts/verify-menu-chain-mariofox-stage-mpcliffescape-common2-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP CliffEscape Common2-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffEscapeAction=status=85->86->96->97 motion=73->74->84->85 cliff=3 floor=3 calls=1/1/1 mpCliffEscapeCommon2=status=97->97->97->97 motion=85->85->85->85 cliff=3 floor=3->3 calls=1/1/1
scripts/verify-menu-chain-mariofox-stage-mpcliffclimb-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP Cliff-climb floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffClimb=climbStatus=86/74 dropStatus=26/20 cliff=3 sticks=0,80/80,0 calls=2/2
scripts/verify-menu-chain-mariofox-stage-mpcliffclimb-action-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP Cliff-climb action-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffClimb=climbStatus=86/74 dropStatus=26/20 cliff=3 sticks=0,80/80,0 calls=2/2 mpCliffClimbAction=status=86->87->88 motion=74->75->76 cliff=3 floor=3 calls=1/1/1
scripts/verify-menu-chain-mariofox-stage-mpcliffclimb-common2-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP Cliff-climb common2-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffClimb=climbStatus=86/74 dropStatus=26/20 cliff=3 sticks=0,80/80,0 calls=2/2 mpCliffClimbAction=status=86->87->88 motion=74->75->76 cliff=3 floor=3 calls=1/1/1 mpCliffClimbCommon2=status=88->88->88->88 motion=76->76->76->76 cliff=3 floor=3->3 calls=1/1/1
scripts/verify-menu-chain-mariofox-stage-mpcliffclimb-finish-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP Cliff-climb finish-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffClimbFinish=status=88->10 motion=76->4 cliff=3 floor=3->3 reset=0/0/0x7ffff calls=1/1/1
scripts/verify-menu-chain-mariofox-stage-mpcliffwait-damage-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP Cliff-wait damage-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffWaitDamage=status=85->57 motion=73->50 fallWait=1->0 catch=30 tics=65536 hold=1->0 procDmg=0 dmgTick=1/1/5 velY=0->-4000 cliffCatch=84/72/1 hold=1 passiveStand=73/62/0->cb1/1->10/4/0 passive=81/70/0->cb1/1->10/4/0 downBounce=68/59/0 dbuf=60 downWait=70/-2 wait=180->179 downStand=72/61 flag1=1->0 dmgMul=1000 coll=4
scripts/verify-battle-mariofox-stage-mppassive-loop-harness.ps1 -> Battle Mario/Fox Stage MP Passive-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 passiveStand=73/62/0->cb1/1->10/4/0 passive=81/70/0->cb1/1->10/4/0 mpPassive=ps=73/62->10/4 ticks=3/2/2 passive=81/70->10/4 ticks=3/2/2
scripts/verify-menu-chain-mariofox-stage-mppassive-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP Passive-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 passiveStand=73/62/0->cb1/1->10/4/0 passive=81/70/0->cb1/1->10/4/0 mpPassive=ps=73/62->10/4 ticks=3/2/2 passive=81/70->10/4 ticks=3/2/2
scripts/verify-battle-mariofox-stage-mpdownwait-loop-harness.ps1 -> Battle Mario/Fox Stage MP DownWait-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpPassive=ps=73/62->10/4 ticks=3/2/2 passive=81/70->10/4 ticks=3/2/2 mpDownWait=status=70/-2->72/61->10/4 attack=80/69->10/4 rolls=76/65->10/4,78/67->10/4 ticks=9/9/9 rollDelta=10000/-10000 downStandTicks=9/8/8 source=0x12345 checks=1/1/1 dsChecks=8/8/8 input=0,80 flag1=1->0/1
scripts/verify-menu-chain-mariofox-stage-mpdownwait-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP DownWait-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpPassive=ps=73/62->10/4 ticks=3/2/2 passive=81/70->10/4 ticks=3/2/2 mpDownWait=status=70/-2->72/61->10/4 attack=80/69->10/4 rolls=76/65->10/4,78/67->10/4 ticks=9/9/9 rollDelta=10000/-10000 downStandTicks=9/8/8 source=0x12345 checks=1/1/1 dsChecks=8/8/8 input=0,80 flag1=1->0/1
scripts/verify-battle-mariofox-stage-turn-loop-harness.ps1 -> Battle Mario/Fox Stage Turn-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 turn=10/4->18/12->10/4 lr=1->-1 vel=2500->-2500 calls=1/1/1
scripts/verify-menu-chain-mariofox-stage-turn-loop-harness.ps1 -> Menu-chain Mario/Fox Stage Turn-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 turn=10/4->18/12->10/4 lr=1->-1 vel=2500->-2500 calls=1/1/1
scripts/verify-battle-mariofox-stage-mpdownrecover-loop-harness.ps1 -> Battle Mario/Fox Stage MP DownRecover-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 downRecoverD=wait=69/-2 stand=71/60 atk=79/68 roll=75/64,77/66 final=0xf
scripts/verify-menu-chain-mariofox-stage-mpdownrecover-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP DownRecover-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 downRecoverD=wait=69/-2 stand=71/60 atk=79/68 roll=75/64,77/66 final=0xf
scripts/verify-battle-mariofox-stage-mpcliffledge-loop-harness.ps1 -> Battle Mario/Fox Stage MP cliff-ledge loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpCliffLedge=occ=1 drop=26/20 wait=30 hold=0 recatch=84/72 finish=10/4 line=3
scripts/verify-menu-chain-mariofox-stage-mpcliffledge-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP cliff-ledge loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpCliffLedge=occ=1 drop=26/20 wait=30 hold=0 recatch=84/72 finish=10/4 line=3
scripts/verify-battle-mariofox-stage-mpclifflive-loop-harness.ps1 -> Battle Mario/Fox Stage MP cliff-live loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpCliffLive=wait=85/73 climb=86/74 quick2=88/76 finish=10/4 drop=26/20 src=0xfff
scripts/verify-menu-chain-mariofox-stage-mpclifflive-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP cliff-live loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpCliffLive=wait=85/73 climb=86/74 quick2=88/76 finish=10/4 drop=26/20 src=0xfff
scripts/verify-battle-mariofox-stage-mpwallhit-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP wall-hit floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpWallHitFloor=pass floor=5 wall=13 edge=12 side=0 mapNodes=1 delta=-1600/-388 mpCliffLive=wait=85/73 climb=86/74 quick2=88/76 finish=10/4 drop=26/20 src=0xfff
scripts/verify-menu-chain-mariofox-stage-mpwallhit-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP wall-hit floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpWallHitFloor=pass floor=5 wall=13 edge=12 side=0 mapNodes=1 delta=-1600/-388 mpCliffLive=wait=85/73 climb=86/74 quick2=88/76 finish=10/4 drop=26/20 src=0xfff
scripts/verify-battle-mariofox-stage-mpwallcopy-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP wall-copy floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpWallHitFloor=pass floor=5 wall=13 edge=12 side=0 mapNodes=1 delta=-1600/-388 mpWallCopy=pass floor=5 wall=13 edge=12 delta=-1600/-388 mpCliffLive=wait=85/73 climb=86/74 quick2=88/76 finish=10/4 drop=26/20 src=0xfff
scripts/verify-menu-chain-mariofox-stage-mpwallcopy-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP wall-copy floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpWallHitFloor=pass floor=5 wall=13 edge=12 side=0 mapNodes=1 delta=-1600/-388 mpWallCopy=pass floor=5 wall=13 edge=12 delta=-1600/-388 mpCliffLive=wait=85/73 climb=86/74 quick2=88/76 finish=10/4 drop=26/20 src=0xfff
scripts/verify-battle-mariofox-stage-mppass-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP pass-through floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpWallHitFloor=pass floor=5 wall=13 edge=12 side=0 mapNodes=1 delta=-1600/-388 mpWallCopy=pass floor=5 wall=13 edge=12 delta=-1600/-388 mpPass=line=0 flags=0x4000 route=2 same=1 diff=1 cb=1/1/0
scripts/verify-menu-chain-mariofox-stage-mppass-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP pass-through floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpWallHitFloor=pass floor=5 wall=13 edge=12 side=0 mapNodes=1 delta=-1600/-388 mpWallCopy=pass floor=5 wall=13 edge=12 delta=-1600/-388 mpPass=line=0 flags=0x4000 route=2 same=1 diff=1 cb=1/1/0
scripts/verify-battle-mariofox-stage-mpplatform-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP platform floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpPass=line=0 flags=0x4000 route=2 same=1 diff=1 cb=1/1/0 mpPlatform=line=0 yak=1 dobj=1 status=0 anim=0 deferred=0x40
scripts/verify-menu-chain-mariofox-stage-mpplatform-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP platform floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpPass=line=0 flags=0x4000 route=2 same=1 diff=1 cb=1/1/0 mpPlatform=line=0 yak=1 dobj=1 status=0 anim=0 deferred=0x40
scripts/verify-battle-mariofox-stage-mpplatform-active-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP platform active floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpPass=line=0 flags=0x4000 route=2 same=1 diff=1 cb=1/1/0 mpPlatform=line=0 yak=1 dobj=1 status=1 anim=0 active
scripts/verify-menu-chain-mariofox-stage-mpplatform-active-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP platform active floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpPass=line=0 flags=0x4000 route=2 same=1 diff=1 cb=1/1/0 mpPlatform=line=0 yak=1 dobj=1 status=1 anim=0 active
scripts/verify-battle-mariofox-stage-mpplatform-pos-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP platform-position floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpPass=line=0 flags=0x4000 route=2 same=1 diff=1 cb=1/1/0 mpPlatform=line=0 yak=1 dobj=1 status=1 anim=0 active mpPlatformTick=tic=0->1 setOn=1 advance=1 status=1/1 mpPassInput=line=0 flags=0x4000 squat=28 pass=33 ignore=0 mpPlatformPos=line=0 yak=1 delta=12000/-4000/2000
scripts/verify-menu-chain-mariofox-stage-mpplatform-pos-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP platform-position floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpPass=line=0 flags=0x4000 route=2 same=1 diff=1 cb=1/1/0 mpPlatform=line=0 yak=1 dobj=1 status=1 anim=0 active mpPlatformTick=tic=0->1 setOn=1 advance=1 status=1/1 mpPassInput=line=0 flags=0x4000 squat=28 pass=33 ignore=0 mpPlatformPos=line=0 yak=1 delta=12000/-4000/2000
scripts/verify-battle-mariofox-stage-mpplatform-speed-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP platform-speed floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpPass=line=0 flags=0x4000 route=2 same=1 diff=1 cb=1/1/0 mpPlatform=line=0 yak=1 dobj=1 status=1 anim=0 active mpPlatformTick=tic=0->1 setOn=1 advance=1 status=1/1 mpPassInput=line=0 flags=0x4000 squat=28 pass=33 ignore=0 mpPlatformPos=line=0 yak=1 delta=12000/-4000/2000 mpPlatformSpeed=line=0 yak=1 read=12000/-4000 dyn=1/2 ceil=1/1 wall=1/1 procwall=1/1 anim=1 bounds=1 stageanim=0/0x91
scripts/verify-menu-chain-mariofox-stage-mpplatform-speed-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP platform-speed floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpPass=line=0 flags=0x4000 route=2 same=1 diff=1 cb=1/1/0 mpPlatform=line=0 yak=1 dobj=1 status=1 anim=0 active mpPlatformTick=tic=0->1 setOn=1 advance=1 status=1/1 mpPassInput=line=0 flags=0x4000 squat=28 pass=33 ignore=0 mpPlatformPos=line=0 yak=1 delta=12000/-4000/2000 mpPlatformSpeed=line=0 yak=1 read=12000/-4000 dyn=1/2 ceil=1/1 wall=1/1 procwall=1/1 anim=1 bounds=1 stageanim=0/0x91
scripts/verify-battle-mariofox-stage-inishie-scale-loop-harness.ps1 -> Battle Mario/Fox Stage Inishie scale-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpPass=line=0 flags=0x4000 route=2 same=1 diff=1 cb=1/1/0 mpPlatform=line=0 yak=1 dobj=1 status=1 anim=0 active mpPlatformTick=tic=0->1 setOn=1 advance=1 status=1/1 mpPassInput=line=0 flags=0x4000 squat=28 pass=33 ignore=0 mpPlatformPos=line=0 yak=1 delta=12000/-4000/2000 mpPlatformSpeed=line=0 yak=1 read=12000/-4000 dyn=1/2 ceil=1/1 wall=1/1 procwall=1/1 anim=1 bounds=1 stageanim=0/0x91 inishieAsset=header/geometry nodes=1 inishieScale=ticks=2 lines=1/2 alt=80000->64000 y=363000/362000->427000/298000 speed=-8000/8000 fall=1->2/0 step=3->0/0 sourceDL=0xff cmds=91 tris=20 tex=0x3f preview=0x3f px=432
scripts/verify-menu-chain-mariofox-stage-inishie-scale-loop-harness.ps1 -> Menu-chain Mario/Fox Stage Inishie scale-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpPass=line=0 flags=0x4000 route=2 same=1 diff=1 cb=1/1/0 mpPlatform=line=0 yak=1 dobj=1 status=1 anim=0 active mpPlatformTick=tic=0->1 setOn=1 advance=1 status=1/1 mpPassInput=line=0 flags=0x4000 squat=28 pass=33 ignore=0 mpPlatformPos=line=0 yak=1 delta=12000/-4000/2000 mpPlatformSpeed=line=0 yak=1 read=12000/-4000 dyn=1/2 ceil=1/1 wall=1/1 procwall=1/1 anim=1 bounds=1 stageanim=0/0x91 inishieAsset=header/geometry nodes=1 inishieScale=ticks=2 lines=1/2 alt=80000->64000 y=363000/362000->427000/298000 speed=-8000/8000 fall=1->2/0 step=3->0/0 sourceDL=0xff cmds=91 tris=20 tex=0x3f preview=0x3f px=432
scripts/verify-runtime.ps1 -> Runtime verification passed (401 frames, fps=60/up=0/dl=60, cv=0/ch=32, verifyfps=2.93)
scripts/verify-opening-boundary.ps1 -> frames=504 hostfps=54.30 room=420
scripts/verify-opening-skip.ps1 -> Opening Room skip verification passed (tick 10 -> Title)
scripts/verify-title-boundary.ps1 -> frames=3292 hostfps=40.52 room=1320 action=9/324 title=0x54494457
scripts/verify-current.ps1 -> Latest verification profile passed.
scripts/verify-title-harness.ps1 -> Title harness passed: scene=1/46 room=0 title=0x54494457
scripts/verify-regression.ps1 -> Regression verification profile passed.
```

Additional current-boundary proof on 2026-06-27: direct and menu-chain Inishie
scale-loop ROMs built with `NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP=1` both
passed their `-NoBuild` verifiers, proving the guarded original
`grInishieMakeScale` setup path through the offset-compatible shim. A later
same-day pass replaced the shim's zero DL word with `G_ENDDL` and copied the
original ScaleRetract anim words from typed `StageInishieFile3`; the direct and
menu-chain then-opt-in verifiers still passed.
Another same-day pass staged `155.vpk0.bin` into then-opt-in NitroFS builds,
validated the raw `5136` byte `StageInishieFile3` payload, converted the
scale DObjDesc scalar fields from the raw big-endian data, and kept active DL
pointers bounded at `G_ENDDL`; direct and menu-chain then-opt-in verifiers still
passed.
The follow-up pass converts the narrow scale DL/Vtx slices to native word
order, patches DObj/map-head DL pointers, and tightens the source marker to
require bits `8..16`; direct and menu-chain then-opt-in verifiers still passed.
The current-boundary promotion pass made that source-backed setup the default
for modes `153/154` only, moved the wrappers/registry entries to separate
`*-source-harness` build directories, and verified the regular
`verify-boundary.ps1` and `verify-current.ps1` flows. Both current summaries
now include `sourceDL=0xff cmds=91 tris=20 tex=0x3f preview=0x3f px=432`.

`NDS_DEV_SCENE_HARNESS=battle_mariofox_gcdrawall_loop` and
`menu_chain_mariofox_gcdrawall_loop` extend the live-input idle and bounded
`gcRunAll` moving-preview boundaries with the first bounded original
`gcDrawAll` Mario/Fox display traversal proof. The harnesses start from the
verified live-preview endpoint, re-enable deterministic controller playback for
a bounded moving slice, advance selected Mario/Fox callbacks through original
`gcRunAll`, and render moving all-DL software-preview keyframes by calling
original `gcDrawAll` rather than manually invoking
`ftDisplayMainProcDisplay`. Non-target display callbacks are masked/guarded,
while selected Mario/Fox display callbacks are reached through the
object-manager draw traversal. Current proof renders seven `96x72` keyframes,
records 42/42 Mario/Fox display callbacks, nonzero preview pixels, screen-X
movement, screen-Y jump rise/floor return, complete status/transition masks,
and final Wait/Ground/Floor state. Full-scene unmasked draw traversal,
hardware polygon rendering, camera-correct matrices, full collision, attacks,
specials, guard, catch, items, HUD, audio, and full imported `ftmain.c` remain
deferred.

`NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_gcdrawall_loop` and
`menu_chain_mariofox_stage_gcdrawall_loop` extend that selected-fighter draw
proof without broadening gameplay. They run the same bounded moving Mario/Fox
slice on Pupupu and record that original `gcDrawAll -> func_80017EC0 ->
gcCaptureCameraGObj` captures all four original Pupupu display-layer GObjs and
all four Pupupu map GObjs. Their imported stage display callbacks reach the
DS-owned DObj draw bridges, producing complete layer/map DObj and DL-ready
masks (`0xf/0xf`) with no manual stage display calls and no unexpected scene
escape. They do not unmask arbitrary full-scene draw traversal or prove real
hardware polygon rendering, camera-correct matrices, Whispy wind, yakumono
runtime, collision lines, items, HUD, or audio.

`NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_collision_loop` and
`menu_chain_mariofox_stage_collision_loop` extend the stage-inclusive draw
proof with the first real Pupupu floor projection boundary. In those modes only,
`mpCollisionCheckProjectFloor` scans the loaded `MPGeometryData` floor lines in
source order, records geometry/probe diagnostics, and keeps older harnesses on
their previous flat-floor compatibility seam. The current proof resolves real
decoded floor line IDs `0/0` inside floor range `[0,4)`, floor line count `4`,
total line count `7`, three successful floor probes, one offstage miss, one
below-floor miss, valid floor edges, and final Wait/Ground/Floor state for both
fighters. The proof-owned roots are re-centered from decoded floor endpoints
before the collision projection sample; full map collision processing,
platforms, ledges, continuous floor following, slopes beyond the current
projection sample, cliff logic, items, HUD, audio, and full gameplay remain
deferred.

`NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_floor_follow_loop` and
`menu_chain_mariofox_stage_floor_follow_loop` extend the stage-collision proof
with continuous selected-fighter floor following for the bounded moving slice.
In those modes only, the proof-owned map seam projects each selected Mario/Fox
root against the real decoded Pupupu floor during the update path, clamps root
Y to the projected floor, preserves final Wait/Ground/Floor state, records
18/18 per-player updates and hits, and asserts zero post-clamp drift. This is
still not full BattleShip map collision: platform pass-through, ledges,
ceilings, walls, slopes beyond the selected decoded floor, cliffcatch,
arbitrary live gameplay, items, HUD, audio, and full `ftmain.c` remain
deferred.

`NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_floor_edge_loop` and
`menu_chain_mariofox_stage_floor_edge_loop` extend the continuous floor-follow
proof with the first real Pupupu floor-edge and original-compatible MP
floor-query evidence. In those modes only, the proof selects the widest decoded
Pupupu floor line, seeds Mario/Fox near opposite floor edges, keeps the
bounded movement slice on that line, proves inside floor-query hits and
outside misses, records signed distance-to-edge/delta diagnostics, and reaches
`mpCollisionGetLineTypeID`, `mpCollisionGetVertexPositionID`, and
`mpCollisionGetFCCommonFloor` through project-owned compatibility helpers.
`mpCollisionGetEdgeUnderLLineID` and `mpCollisionGetEdgeUnderRLineID` are
called and counted but intentionally return `-1` until real wall/ledge/platform
contracts are imported. This is still not full BattleShip map collision:
platform pass-through, ledges, wall edge-under resolution, ceilings, walls,
cliffcatch, arbitrary live gameplay, items, HUD, audio, and full `ftmain.c`
remain deferred.

`NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpprocess_floor_loop` and
`menu_chain_mariofox_stage_mpprocess_floor_loop` extend the floor-edge proof
with the first source-order MP floor-process slice for selected Mario/Fox
fighters. In those modes only, a project-owned original-layout `MPCollData`
adapter is built from the bounded `FTStruct` collision shell, runs
`mpProcessCheckTestFloorCollisionNew` after the existing floor-follow update,
copies back the floor fields, and proves signed floor distances above, below,
and on the selected decoded Pupupu floor. The direct and menu-chain proofs
record 38 floor-process hits, two deliberate misses, two project-floor misses,
one inside hit probe, two outside miss probes, positive below-floor distance
evidence, local `mpProcessSetLandingFloor` / `mpProcessSetCollideFloor` probe
calls, 18/18 live Mario/Fox update hits, final line ID `3/3`, final
`MAP_FLAG_FLOOR`, and no scene/status/unsafe escapes. This is still not full
BattleShip map collision: platform pass-through, ledges, wall edge-under
resolution, ceilings, walls, cliffcatch, arbitrary live gameplay, items, HUD,
audio, and full `ftmain.c` remain deferred.

`NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpadjust_floor_loop` and
`menu_chain_mariofox_stage_mpadjust_floor_loop` extend the live cross-floor
proof with the first bounded source-order floor-edge-adjust pass. The proof
keeps the same bounded floor-only `mpCommonRunFighterAllCollisions` slice,
primes P0 with current floor line `-1`, lets source-order
`mpProcessCheckTestFloorCollisionNew` project against decoded Pupupu geometry,
proves `mpProcessCheckTestFloorCollision` accepts real floor line `3`, then
calls source-order `mpProcessRunFloorEdgeAdjust`. The current floor-only slice
reaches `mpProcessCheckFloorEdgeCollisionL/R`,
`mpCollisionGetEdgeUnderL/RLineID`, and
`mpCollisionCheckL/RWallLineCollisionSame`; both wall sweeps safely miss, so
the left/right floor-edge adjust branches remain deferred. Direct and
menu-chain proofs record 17 live adjust calls, 17/17 left/right floor-edge
checks, 17/17 wall misses, 17/17 edge-under deferred calls, old
floor-edge-adjust deferred count `0`, P0 final line `3`, P1 maintained line
`3`, no unsafe count, no final recenter/adopt, and preserved
Wait/Ground/Floor state. This is still not full BattleShip map collision:
real wall-hit floor-edge adjustment, live selected-callback
stale-valid-floor crossing,
platform pass-through, ledges, wall edge-under resolution, ceilings, walls,
cliffcatch, arbitrary live gameplay, items, HUD, audio, and full `ftmain.c`
remain deferred.

`NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpedge_floor_loop` and
`menu_chain_mariofox_stage_mpedge_floor_loop` extend the floor-edge-adjust
proof by replacing the previously deferred `mpCollisionGetEdgeUnderL/RLineID`
path with bounded decoded Pupupu geometry adjacency. The proof resolves the
selected floor line's adjacent left/right wall line IDs as `6/5`, records wall
kinds `3/2`, keeps MPAdjust edge-under deferred count at `0`, still reaches
the left/right floor-edge checks and wall sweeps through the selected live
Mario/Fox callback route, and preserves the same final line `3/3`,
Wait/Ground/Floor state, and safety counters. The older `mpadjust` modes
remain in regression to prove the explicitly deferred edge-under behavior.
This is still not full BattleShip map collision: real wall-hit adjustment,
live selected-callback stale-valid-floor crossing, platform pass-through,
ledges, ceilings, walls, cliffcatch, arbitrary live gameplay, items, HUD,
audio, and full `ftmain.c` remain deferred.

`NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpwall_floor_loop` and
`menu_chain_mariofox_stage_mpwall_floor_loop` extend the edge-under proof by
running a bounded source-order wall-candidate sweep for the current selected
Dream Land main floor. The proof records two side-wall candidates and confirms
both are the same edge-under walls `6/5`; original
`mpProcessCheckFloorEdgeCollisionL/R` requires the wall candidate to differ
from the edge-under line, so the real wall-hit floor-edge-adjust branch is
blocked for this geometry slice. The proof records zero wall hits, nonzero
miss evidence, zero adjust calls, zero position deltas, and stable
Wait/Ground/Floor state. The verifier now also runs an isolated
geometry-wide wall-hit scout over all
currently loaded Dream Land floor lines while restoring the older MP-adjust
counters. Both direct and menu-chain routes report `mpWallHitScout=none` with
`floors=4`, `walls=8`, `candidates=6`, and zero hits. This makes the Pupupu
wall-hit blocker stage-wide for the currently staged map asset, not just a
line `3` local probe. The same verifier now stages `GRHyruleMap`,
`StageCastle`, and `ExternDataBank113` only for an isolated Hyrule scout, using
existing relocation scratch storage so the bounded battle task heap is not
consumed by the scout dependencies. That Hyrule scout passes relocation marker
`0x4859524c` and reports a real source-order wall-hit candidate:
`hyruleWallHit=hit floor=5 wall=13 edge=12 side=0 delta=-1600/-388`. Modes
`135/136` promote that concrete candidate into a direct plus menu-chain
bounded source-order wall-hit floor-edge-adjust proof while keeping the live
scene on Pupupu. Modes `137/138` now consume that proof, attach a proof-owned
selected P0 process, copy the adjusted Hyrule collision result back into the
live P0 root/collision shell, and prove P1 root state is unchanged.

`NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpstale_floor_loop` and
`menu_chain_mariofox_stage_mpstale_floor_loop` extend the wall-blocker proof
with a bounded valid-stale second-floor path. The live selected-callback
cross-floor proof remains the existing `-1 -> 3` route, and the new proof runs
a finalizer-local source-order `MPCollData` probe instead of moving the live
P0/P1 roots. The selected stale pair is Dream Land floor line `1 -> 0` at
`x=-285`, `y=1542`; it reaches `mpProcessCheckTestFloorCollision`, accepts
the new floor, calls `mpProcessSetLandingFloor`, reaches
`mpProcessRunFloorEdgeAdjust`, clears collision-end state, and keeps the old
deferred count at zero. These modes remain in regression as the finalizer-local
valid-stale source-order proof.

`NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mplivestale_floor_loop` and
`menu_chain_mariofox_stage_mplivestale_floor_loop` remain regression coverage
for the contained local selected-callback valid-stale probe. The
`NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpfallland_floor_loop` and
`menu_chain_mariofox_stage_mpfallland_floor_loop` modes remain regression
coverage for Fall landing-floor setup. The
`NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpceil_floor_loop` and
`menu_chain_mariofox_stage_mpceil_floor_loop` modes `95/96` remain regression
coverage for the bounded source-order ceiling test/adjust path against real
Pupupu ceiling line `4`. The
`NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpceilstatus_floor_loop` and
`menu_chain_mariofox_stage_mpceilstatus_floor_loop` modes `97/98` remain
regression coverage for the
selected original `mpCommonProcFighterCliffFloorCeil` map callback reaches the
ceil-heavy collision/adjust path and original `ftCommonStopCeilSetStatus`
through imported `ftcommonstopceil.c`. Modes `99/100` remain regression
coverage for the selected right-cliff test and original
`ftCommonCliffCatchSetStatus` proof on real Pupupu line `3`. Modes `101/102`
remain regression coverage for imported `ftcommoncliffcatchwait.c`: they prove
CliffCatch update reaches original `ftCommonCliffWaitSetStatus`, CliffWait
motion/status `85/73`, ground state, retained `cliff_id=3`, player tag wait,
capture immunity, proc-damage setup, and one guarded CliffWait interrupt tick
with fall-wait `1080 -> 1079`. Modes `103/104` remain regression coverage for
the CliffAttack setup proof: they import original cliff attack/climb/escape
helpers, inject an A-button tap from that CliffWait state, call original
`ftCommonCliffAttackCheckInterruptCommon`, and prove CliffQuick motion/status
`86/74` with queued AttackQuick metadata on retained Pupupu `cliff_id=3`. The
previous active stage-MP boundary was
`NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpcliffattack_action_loop` and
`menu_chain_mariofox_stage_mpcliffattack_action_loop`; modes `105/106` then
call original `ftCommonCliffQuickProcUpdate`, reach CliffAttackQuick1
`92/80`, run original `ftCommonCliffAttackQuick1ProcUpdate`, and reach
CliffAttackQuick2 `93/81` through the guarded original anim-end branch. The
original common2 collision-data helper is now reached through a narrow
project-owned `MPCollData` bridge, and the wrapper copies the ledge floor
result back to the live `FTCollisionData` shell as `floor_line_id=3`. The
previous active stage-MP boundary was
`NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpcliffcommon2_loop` and
`menu_chain_mariofox_stage_mpcliffcommon2_loop`; modes `107/108` consume that
created CliffAttackQuick2/Ground state through one bounded original common2
update/physics/map tick and preserve status/motion `93/81`, `cliff_id=3`, and
`floor_line_id=3`. The previous active stage-MP boundary was
`NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpcliffescape_action_loop` and
`menu_chain_mariofox_stage_mpcliffescape_action_loop`; modes `109/110` inject
a Z-button tap from CliffWait, prove original escape selection, and advance
CliffQuick -> CliffEscapeQuick1 -> CliffEscapeQuick2 with status/motion
`85/73 -> 86/74 -> 96/84 -> 97/85`.
The previous active stage-MP boundary was
`NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpcliffclimb_floor_loop` and
`menu_chain_mariofox_stage_mpcliffclimb_floor_loop`; modes `113/114` consume
the verified CliffWait/Ground ledge state, reject the guarded attack/escape
checks for the seeded inputs, and prove original climb/fall interrupt branching
to CliffQuick/Ground `86/74` and Fall/Air `26/20` while retaining
`cliff_id=3`. The previous active stage-MP boundary was
`NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpcliffclimb_action_loop` and
`menu_chain_mariofox_stage_mpcliffclimb_action_loop`; modes `115/116` consume
that climb-created CliffQuick/Ground state and prove original
CliffQuick -> CliffClimbQuick1 -> CliffClimbQuick2 action setup with
status/motion `86/74 -> 87/75 -> 88/76`, retained `cliff_id=3`, copied
`floor_line_id=3`, and one guarded common2 collision/init pass. The previous
active stage-MP boundary was
`NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpcliffclimb_common2_loop` and
`menu_chain_mariofox_stage_mpcliffclimb_common2_loop`; modes `117/118`
consume that created CliffClimbQuick2/Ground state through one bounded original
common2 update/physics/map tick while preserving status/motion `88/76`,
`cliff_id=3`, and `floor_line_id=3`. The previous active stage-MP boundary was
`NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_mpcliffclimb_finish_loop` and
`menu_chain_mariofox_stage_mpcliffclimb_finish_loop`; modes `119/120` consume
the same CliffClimbQuick2/Ground state through the bounded original common2
animation-end handoff into Wait/Ground while preserving `cliff_id=3` and
`floor_line_id=3`. A previous active stage-MP boundary was
`NDS_DEV_SCENE_HARNESS=battle_mariofox_stage_inishie_scale_loop` and
`menu_chain_mariofox_stage_inishie_scale_loop`; modes `153/154`
consume the cliff-live, Hyrule wall-hit, wall-copy, pass-through, inactive
platform, active platform, platform-tick, pass-input, position, platform-speed,
dynamic floor/ceil/wall, process-wall, yakumono animation, bounds, and Inishie
map-header proofs, keep the live scene on Pupupu, then run two bounded
original `grInishieScaleProcUpdate` ticks through a narrow Inishie
scale-platform proof shell.
The proof records Dream Land line `0`, yakumono `1`, delta/readback
`12000/-4000/2000`, dynamic markers `dyn=1/2`, `ceil=1/1`, `wall=1/1`,
`anim=1`, bounds marker `bounds=1`, stage-authored animation blocker marker
`stageanim=0/0x91`, Inishie line groups `1/2`, map object kinds `5/6`,
altitude `80000->64000`, platform Y `363000/362000->427000/298000`,
second-tick speed `-8000/8000`, DObj status `1`, platform predicate `1`, and no unsafe
escape. Modes `153/154` remain regression coverage for the Inishie
scale-platform proof; the current Latest/Boundary targets are modes `161/162`.
Modes `151/152` remain regression coverage for the platform speed-reader,
dynamic surface-consumer, yakumono animation, bounds, and Inishie map-header
preflight proof. Modes `149/150` remain
regression coverage for the yakumono
position primitive. Modes `147/148` remain regression coverage
for the natural drop-through input gate. Modes `145/146` remain regression
coverage for platform status and update-tic behavior. Modes `143/144` remain
regression coverage for the active
platform predicate,
modes `141/142` remain regression coverage for the inactive platform blocker,
modes `139/140` remain regression coverage for the bounded pass-through floor
contract, modes `137/138` remain regression coverage for the promoted Hyrule
wall-copy proof, and modes `135/136` remain regression coverage for the
promoted Hyrule wall-hit proof. Modes `133/134`
remain regression coverage for the aggregate cliff-ledge proof, selected P0
live `GObjProcess` CliffCatch/CliffWait/CliffClimbQuick2 common2 path,
Wait/Ground finish, and CliffWait drop into Fall/Air. Modes `131/132` remain
regression coverage for same-cliff occupancy blocking, drop/release into
Fall/Air, post-release recatch, and original CliffClimbQuick2 finish into
Wait/Ground. Modes
`129/130` remain regression coverage for the bounded face-down DownRecover
branch set.
Modes `127/128` remain regression coverage for the bounded original Wait ->
Turn interrupt/status/update path, facing/ground-velocity flip, and Wait/Ground
handoff. Modes `125/126` remain regression coverage
for the bounded original DownWait interrupt branches,
DownAttackU/DownForwardU/DownBackU/DownStandU handoffs, eight guarded stable
callback frames, roll root movement `+10000/-10000` milli, and DownAttackU
attack IDs `53/33/33`. Modes `123/124` remain regression coverage
for the verified CliffWait damage PassiveStand/Passive setup branches, two
guarded stable update/physics/map frames, and the original animation-end
handoff into Wait/Ground.
Modes `121/122` remain regression coverage for the CliffWait timeout,
DamageFall, CliffCatch, PassiveStand/Passive setup, DownBounce, DownWait, and
DownStand proof chain.
Modes `85/86` remain regression
coverage for the selected P0 root/collision motion-stale proof:
they reuse the Dream Land valid-stale pair `1 -> 0`, seed it into P0 state
before the selected map callback runs, route through source-order
`mpProcessUpdateMain`, `mpCommonRunFighterAllCollisions`, and
`mpProcessCheckTestFloorCollision`, then copy accepted target floor line `0`
back to the live P0 root/collision state. Modes `87/88` then import original
`ftcommonottotto.c` and prove the bounded source-order
`mpCommonProcFighterOnCliffEdge` status branch: P0 with `MAP_FLAG_FLOOREDGE`
enters original Ottotto status/motion `36/30`, while P1 without that flag
enters original Fall status/motion `26/20` and air state. Modes `89/90` then
run one guarded imported Ottotto update/interrupt/map callback tick for P0 and
one imported Fall interrupt tick for P1. Modes `91/92` now consume that P1
Fall state, run one guarded original Fall physics callback, perform one bounded
airborne integration step, call the selected original Fall map callback through
the guarded no-collision branch, and remain regression coverage. Modes `93/94`
then seed P1 just above decoded Pupupu floor line `3`, cross the floor through
the selected original map callback route, call landing-floor setup, reach
original LandingLight status/motion `31/25`, switch to Ground, clamp
vertical velocity to zero, and remain regression coverage.

`NDS_DEV_SCENE_HARNESS=battle_mariofox_live_preview` and
`menu_chain_mariofox_live_preview` extend the bounded original `gcRunAll`
Mario/Fox moving-preview proof with the first live DS-controller source
boundary. The automated proof disables deterministic playback for the live
phase, uses the live `osContGetReadData` path, runs original
`syControllerReadDeviceData` / `syControllerUpdateGlobalData`, copies
`gSYControllerDevices` into `FTStruct` input through a DS-owned bridge, runs
selected Mario/Fox callbacks through original `gcRunAll`, and proves a
60-frame neutral idle slice keeps both fighters in Wait/Ground/Floor while
still drawing guarded all-DL preview keyframes. Use
`NDS_DEV_LIVE_INPUT_PREVIEW=1` with the direct harness for the manual 3600-frame
P0 live-input dev build.

`src/port/scene_backend.c` is now a thin include orchestrator over
`diagnostics.c`, `taskman_seam.c`, `reloc_backend.c`,
`sprite_preview_backend.c`, `opening_movie_backend.c`, and `title_backend.c`.
This is the mechanical split stage; those slices intentionally keep the old
single-translation-unit static linkage.

## Do-Not-Touch Constraints

- Treat all of `decomp/` as read-only reference material.
- Do not edit generated build output, generated ROM/ELF artifacts, or generated
  emulator logs/configs except through build and verification commands.
- Do not rewrite Smash gameplay or menus by hand when original BattleShip code
  exists.
- Do not import broad renderer/fighter/stage/audio systems just to satisfy a
  narrow boundary.
- Keep DS-specific behavior in `src/nds` or `src/port`, imports in
  `src/import`, and compatibility declarations in `include`.
