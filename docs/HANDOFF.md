# Handoff

Use this file for the active handoff only. The harness registry decides the
current Boundary/Latest profile; this file summarizes the next action. Use
`docs/STATUS.md` for the current boundary summary, `docs/DIAGNOSTIC_REFERENCE.md`
for marker details, and `docs/PORTING.md` for append-only history.

## Current Agent Start Here

- Current Latest/Boundary pair from `.\scripts\verify-all.ps1 -Profile Boundary -List`: `battle_mariofox_stage_mplivehit_status_loop` + `menu_chain_mariofox_stage_mplivehit_status_loop` (registry modes `161/162`).
- Current proof: bounded direct/menu-chain selected live-hit damage lifecycle aggregate plus selected damage-status follow-through. It keeps the live VSBattle roots stable on Pupupu/Dream Land, inherits the platform-speed/Inishie scale, wall/rebound, catch/throw, ledge, passive/recover, dash-run damage setup, and `mpDamageRecover` proofs, then proves selected Fox Jab2 Attack12 hitbox activation evidence, attack-state `Off -> New -> Transfer -> Interpolate`, selected contact, repeat-hit rejection, bounded source-order Mario hurtbox scan through slot `3` of `10`, selected slot-3 damage record/hitlog/stat/percent/hitlag consumption plus restored unmasked natural slot-0 consume/repeat rejection and natural slot-1/2/4/5/6/7/8/9 consume, shield/contact/rehit side proofs including same-victim shield-contact repeat rejection, and now selected status `17->52/45`, hitlag `6->0`, callbacks `1/6/1`, one post-hitlag original damage update tick `2->1`, one original `ftCommonDamageSetStatus` knockback-over branch into timed hit-status invincibility, one original DamageFlyRoll status-set branch into `ftCommonDamageFlyRollUpdateModelPitch`, one installed original damage physics tick `phys=11500/-1000`, one original ground damage-physics branch through traction friction, one original zero-hitstun air damage-physics branch through fastfall, one original zero-hitstun air damage-physics branch through drift and gravity, one original zero-hitstun air damage-physics branch through air-velocity clamp-decrement, one original low-speed throw damage-physics branch that clears attack collisions, one original DamageFlyRoll damage-physics branch that updates joint pitch, one original Smash DI lag-update branch that moves the root and resets tap buffers, one original lag-update no-op gate proof for no-hitlag, below-threshold stick, and saturated tap buffers, one installed original damage interrupt tick `interrupt=1`, proof-local ground/air DamageCommon interrupt branches through Wait/Fall, proof-local ground/air hammer DamageCommon interrupt branches through Hammer/HammerFall, one post-expiry installed original DamageFall interrupt source-order branch through SpecialAir, AttackAir, JumpAerial, and HammerFall checks, one installed original damage map no-collision tick `map=1/1`, one installed original damage map floor-collision branch through passive checks into DownBounce `floor=1/1/1/1`, installed original DamageAir no-floor and DamageAir/DamageFall passive short-circuit proofs through imported PassiveStand and Passive checks, one installed original damage map left/right-wall and ceiling WallDamage short-circuit `wall=0x3ffff`, post-expiry DamageFall map no-collision/floor fallback plus no-cliff-mask tail and proof-local imported-original DownBounce set-status handoff `fallmap=0x7ff`, cliff-catch branch plus proof-local imported-original CliffCatch set-status handoff `cliff=0x3f`, selected air/fly expiry through imported-original `ftCommonDamageFallSetStatusFromDamage` into `57/50`, source-shaped search/proc masks including natural slot-0 catch-search, self rejection, ghost rejection, Boss rejection, same-team rejection, capture-immune rejection, global target hitstatus rejection, attack-state-off rejection, Ground/Air mismatch rejection, attack-record skip branches, the damage-coll `None` sentinel break, valid no-collision no-update, repeat suppression, bounded false-return ground-obstacle callback iteration, the hazard ghost gate, true-return Twister status dispatch, and gate `0x3f`.
- Latest increment: the selected live-hit catch-search hazard proof keeps public `catchSearch=0xffffffff/s3`, but the existing true-return hazard-dispatch bit now also requires TaruCannon kind `3` through `ftMainSetHitHazard` into the source-ordered TaruCannon setup shell plus one installed original TaruCannon physics tick copying the fighter root position from the barrel root before state restore. Continuous TaruCannon update/shoot runtime still waits for Jungle barrel helpers and map throw-hit data.
- Previous catch-search increment: the selected live-hit catch-search proof now requires `STAGE_MPLIVEHIT_CATCHSEARCH` secondary skip mask low bits `0x3fff`. The added bit records the restored source-order self-target rejection branch continuing to the next fighter in the secondary skip mask before state restore.
- Previous catch-search increment: the selected live-hit catch-search proof required `STAGE_MPLIVEHIT_CATCHSEARCH` secondary skip mask low bits `0xff`. The added bits recorded the restored source-order attack-state-off, Ground/Air mismatch, and attack-record rejection probes in the secondary skip mask before state restore.
- Previous catch-search increment: the selected live-hit catch-search proof required `STAGE_MPLIVEHIT_CATCHSEARCH` secondary skip mask low bits `0x1f`. The added bits recorded the existing restored source-order `ftMainSearchFighterCatch` `None` sentinel break and valid no-collision no-update probes in the secondary skip mask before state restore.
- Previous catch-search increment: the selected live-hit catch-search proof required `STAGE_MPLIVEHIT_CATCHSEARCH` secondary skip mask low bits `0x7`. The added bit restored Mario/Fox, marked the selected damage-coll slot invincible, reran source-order `ftMainSearchFighterCatch`, and proved the catch loop skips the invincible damage-coll before search target/distance or attack-record updates.
- Previous live-hit hurtbox-damage increment: the selected live-hit hurtbox-damage consume proof required `STAGE_MPLIVEHIT_HURTBOX_DAMAGE` mask low bits `0x7ffff`; the status-loop search marker inherited the same damage mask. The added bits restored Mario/Fox after the natural slot-7 proof, marked earlier Mario damage-coll slots intangible, reran source-order `ftMainProcSearchHitAll`, and proved natural slot-8 and slot-9 consume before restoring state.
- Previous live-hit hurtbox-damage increment: the selected live-hit hurtbox-damage consume proof required `STAGE_MPLIVEHIT_HURTBOX_DAMAGE` mask low bits `0x1ffff`. The added bit restored Mario/Fox after the natural slot-6 proof, marked Mario damage-coll slots `0` through `6` intangible, reran source-order `ftMainProcSearchHitAll`, and proved natural slot-7 consume before restoring state.
- Previous live-hit hurtbox-damage increment: the selected live-hit hurtbox-damage consume proof required `STAGE_MPLIVEHIT_HURTBOX_DAMAGE` mask low bits `0xffff`. The added bit restored Mario/Fox after the natural slot-5 proof, marked Mario damage-coll slots `0` through `5` intangible, reran source-order `ftMainProcSearchHitAll`, and proved natural slot-6 consume before restoring state.
- Previous live-hit hurtbox-damage increment: the selected live-hit hurtbox-damage consume proof required `STAGE_MPLIVEHIT_HURTBOX_DAMAGE` mask low bits `0x7fff`. The added bit restored Mario/Fox after the natural slot-4 proof, marked Mario damage-coll slots `0` through `4` intangible, reran source-order `ftMainProcSearchHitAll`, and proved natural slot-5 consume before restoring state.
- Previous live-hit hurtbox-damage increment: the selected live-hit hurtbox-damage consume proof required `STAGE_MPLIVEHIT_HURTBOX_DAMAGE` mask low bits `0x3fff`. The added bit restored Mario/Fox after the natural slot-2 proof, marked Mario damage-coll slots `0` through `3` intangible, reran source-order `ftMainProcSearchHitAll`, and proved natural slot-4 consume before restoring state.
- Previous live-hit hurtbox-damage increment: the selected live-hit hurtbox-damage consume proof required `STAGE_MPLIVEHIT_HURTBOX_DAMAGE` mask low bits `0x1fff`. The added bit restored Mario/Fox after the natural slot-1 proof, marked Mario damage-coll slots `0` and `1` intangible, reran source-order `ftMainProcSearchHitAll`, and proved natural slot-2 consume before restoring state.
- Previous live-hit hurtbox-damage increment: the selected live-hit hurtbox-damage consume proof required `STAGE_MPLIVEHIT_HURTBOX_DAMAGE` mask low bits `0xfff`. The added bit restored Mario/Fox after the natural slot-0 consume/repeat proof, marked Mario damage-coll slot `0` intangible, reran source-order `ftMainProcSearchHitAll`, and proved natural slot-1 consume before restoring state.
- Previous status-loop callback increment: the selected live-hit status-loop proof now requires `STAGE_MPLIVEHIT_STATUS` and `STAGE_MPLIVEHIT_STATUS_CALLBACK` mask low bits `0xffffffff`. The added bit reruns installed original `ftCommonDamageFallProcInterrupt` after DamageFall expiry, proving BattleShip's source-order SpecialAir, AttackAir, JumpAerial, and HammerFall checks before restoring state.
- Previous status-loop callback increment: the selected live-hit status-loop proof requires `STAGE_MPLIVEHIT_STATUS` and `STAGE_MPLIVEHIT_STATUS_CALLBACK` mask low bits `0x7fffffff`. The added bit reruns installed original `ftCommonDamageFallProcMap` with collision but no `MAP_FLAG_CLIFF_MASK`, proving the source no-cliff tail runs PassiveStand, Passive, and DownBounce before restoring state; `fallmap` is now `0x7ff`.
- Previous status-loop callback increment: the selected live-hit status-loop proof requires `STAGE_MPLIVEHIT_STATUS` and `STAGE_MPLIVEHIT_STATUS_CALLBACK` mask low bits `0x3fffffff`. The added bit reruns installed original `ftCommonDamageAirCommonProcMap` with collision but no `MAP_FLAG_FLOOR`, proving the source floor-bit short-circuit skips PassiveStand, Passive, and DownBounce before restoring state.
- Previous status-loop callback increment: the selected live-hit status-loop proof requires `STAGE_MPLIVEHIT_STATUS` and `STAGE_MPLIVEHIT_STATUS_CALLBACK` mask low bits `0x1fffffff`. The added bit reruns installed original `ftCommonDamageFallProcMap` with floor collision, proving imported-original PassiveStand and Passive true-return short-circuits skip the DownBounce tail before restoring state.
- Previous status-loop callback increment: the selected live-hit status-loop proof requires `STAGE_MPLIVEHIT_STATUS` and `STAGE_MPLIVEHIT_STATUS_CALLBACK` mask low bits `0xfffffff`. The added bit reruns installed original `ftCommonDamageAirCommonProcMap` with floor collision, proving imported-original PassiveStand and Passive true-return short-circuits skip later map branches before restoring state.
- Previous status-loop callback increment: the selected live-hit status-loop proof requires `STAGE_MPLIVEHIT_STATUS` and `STAGE_MPLIVEHIT_STATUS_CALLBACK` mask low bits `0x7ffffff`. The added bit reruns installed original `ftCommonDamageCommonProcLagUpdate`, proving the no-hitlag, below-threshold stick, and saturated tap-buffer no-op gates before restoring the live-hit state.
- Earlier status-loop callback increment: the selected live-hit status-loop proof requires `STAGE_MPLIVEHIT_STATUS` and `STAGE_MPLIVEHIT_STATUS_CALLBACK` mask low bits `0x3ffffff`. The added bit reruns installed original `ftCommonDamageCommonProcPhysics` with zero-hitstun Air kinetics and over-max horizontal velocity, proving the source air-velocity clamp-decrement branch before restoring the live-hit state.
- Earlier status-loop callback increment: the selected live-hit status-loop proof requires `STAGE_MPLIVEHIT_STATUS` and `STAGE_MPLIVEHIT_STATUS_CALLBACK` mask low bits `0x1ffffff`. The added bit reruns installed original `ftCommonDamageCommonProcPhysics` with zero-hitstun Air kinetics and horizontal stick input, proving the source air-drift branch updates X velocity after gravity before restoring the live-hit state.
- Older status-loop callback increment: the selected live-hit status-loop proof requires `STAGE_MPLIVEHIT_STATUS` and `STAGE_MPLIVEHIT_STATUS_CALLBACK` mask low bits `0xffffff`. The added bit reruns installed original `ftCommonDamageCommonProcPhysics` with zero-hitstun Air kinetics, proving the source `ftPhysicsApplyAirVelDriftFastFall` branch sets fastfall state and terminal velocity before restoring the live-hit state.
- Older status-loop callback increment: the selected live-hit status-loop proof requires `STAGE_MPLIVEHIT_STATUS` and `STAGE_MPLIVEHIT_STATUS_CALLBACK` mask low bits `0x7fffff`. The added bit reruns installed original `ftCommonDamageCommonProcPhysics` with ground kinetics, proving the source ground-friction branch reduces ground velocity before restoring the live-hit state.
- Older status-loop callback increment: the selected live-hit status-loop proof requires `STAGE_MPLIVEHIT_STATUS` and `STAGE_MPLIVEHIT_STATUS_CALLBACK` mask low bits `0x3fffff`. The added bit reruns installed original `ftCommonDamageCommonProcPhysics` as `DamageFlyRoll`, proving the source physics branch reaches `ftCommonDamageFlyRollUpdateModelPitch` and updates joint pitch before restoring the live-hit state.
- Older status-loop callback increment: the selected live-hit status-loop proof requires `STAGE_MPLIVEHIT_STATUS` and `STAGE_MPLIVEHIT_STATUS_CALLBACK` mask low bits `0x1fffff`. The added bit reruns installed original `ftCommonDamageCommonProcLagUpdate` with hitlag, stick range, and tap-buffer state, proving the Smash DI branch moves the root and resets tap buffers before restoring the live-hit state.
- Earlier status-loop callback increment: the selected live-hit status-loop proof requires `STAGE_MPLIVEHIT_STATUS` and `STAGE_MPLIVEHIT_STATUS_CALLBACK` mask low bits `0xfffff`. The added bit reruns installed original `ftCommonDamageCommonProcPhysics` with a low-speed throw-owned attack coll, proving the source tail clears attack collisions before restoring the live-hit state.
- Older status-loop callback increment: the selected live-hit status-loop proof requires `STAGE_MPLIVEHIT_STATUS` and `STAGE_MPLIVEHIT_STATUS_CALLBACK` mask low bits `0x7ffff`. The added bit reruns installed original `ftCommonDamageSetStatus` with `DamageFlyRoll`, proving the source branch reaches `ftCommonDamageFlyRollUpdateModelPitch` and updates joint pitch before restoring the live-hit state.
- Older status-loop callback increment: the selected live-hit status-loop proof requires `STAGE_MPLIVEHIT_STATUS` and `STAGE_MPLIVEHIT_STATUS_CALLBACK` mask low bits `0x3ffff`. The added bit reruns installed original `ftCommonDamageSetStatus` with `is_knockback_over` set, proving the source branch clears that flag and sets timed hit-status invincibility before restoring the live-hit state.
- Older status-loop callback increment: the selected live-hit status-loop proof requires `STAGE_MPLIVEHIT_STATUS` and `STAGE_MPLIVEHIT_STATUS_CALLBACK` mask low bits `0x1ffff`. The added bit seeds `DamageN1` with Air kinetics, marks hammer held through the existing hammer-check seam, runs installed original `ftCommonDamageCommonProcInterrupt` with hitstun already zero, and proves the air hammer branch reaches `ftCommonHammerFallProcInterrupt` before restoring the live-hit state.
- Older status-loop callback increment: the selected live-hit status-loop proof requires `STAGE_MPLIVEHIT_STATUS` and `STAGE_MPLIVEHIT_STATUS_CALLBACK` mask low bits `0xffff`. The added bit seeds ground `DamageN1`, marks hammer held through the existing hammer-check seam, runs installed original `ftCommonDamageCommonProcInterrupt` with hitstun already zero, and proves the hammer branch reaches `ftHammerProcInterrupt` before restoring the live-hit state.
- Older status-loop callback increment: the selected live-hit status-loop proof requires `STAGE_MPLIVEHIT_STATUS` and `STAGE_MPLIVEHIT_STATUS_CALLBACK` mask low bits `0x7fff`. The added bit seeds `DamageN1` with Air kinetics, runs installed original `ftCommonDamageCommonProcInterrupt` with hitstun already zero, and proves the no-hammer air branch reaches `ftCommonFallProcInterrupt` before restoring the live-hit state.
- Older status-loop ground-interrupt increment: the selected live-hit status-loop proof requires `STAGE_MPLIVEHIT_STATUS` and `STAGE_MPLIVEHIT_STATUS_CALLBACK` mask low bits `0x3fff`. The added bit seeds ground `DamageN1`, runs installed original `ftCommonDamageCommonProcInterrupt` with hitstun already zero, and proves it clears hitstun state and reaches imported-original Wait/Ground interrupt handling before restoring the live-hit state.
- Older status-loop update increment: the selected live-hit status-loop proof requires `STAGE_MPLIVEHIT_STATUS` and `STAGE_MPLIVEHIT_STATUS_CALLBACK` mask low bits `0x1fff`. The added bit seeds ground `DamageN1`, runs installed original `ftCommonDamageCommonProcUpdate` at animation end as hitstun reaches zero, and proves public knockback release through original `mpCommonSetFighterWaitOrFall` into imported-original Wait/Ground before restoring the live-hit state.
- Older status-loop no-expiry increment: the selected live-hit status-loop proof requires `STAGE_MPLIVEHIT_STATUS` and `STAGE_MPLIVEHIT_STATUS_CALLBACK` mask low bits `0xfff`. The added bit reruns installed original `ftCommonDamageAirCommonProcUpdate` with animation still active while hitstun reaches zero, proving public knockback release without the DamageFall expiry handoff.
- Previous live-hit damage-loop increment: the selected live-hit damage-loop proof requires `STAGE_MPLIVEHIT_DAMAGE` mask low bits `0xffffffff`. The added bit marks the attacking fighter ghost and proves BattleShip's `ftMainProcSearchHitAll` outer guard exits before clearing the hitlog or running fighter/item/weapon/ground search and hit-stat processing.
- Latest catch-search hazard increment: the selected live-hit catch-search proof still reports `STAGE_MPLIVEHIT_CATCHSEARCH` mask low bits `0xffffffff`, but the existing true-return hazard-dispatch bit now also requires TaruCannon kind `3` through `ftMainSetHitHazard` into the source-ordered TaruCannon setup shell: status `61`, script `-1`, TaruCannon status-vars reset, barrel GObj capture, capture immunity, invisible flag, intangible hitstatus, and one installed original TaruCannon physics callback tick copying the fighter root position from the barrel root before state restore. Continuous TaruCannon update/shoot runtime still waits for Jungle barrel helpers and map throw-hit data.
- Previous catch-search target-hitstatus increment: the selected live-hit catch-search proof requires `STAGE_MPLIVEHIT_CATCHSEARCH` mask low bits `0x1ffff`. The added bit restores Mario/Fox, seeds each global target hitstatus field non-normal one at a time, reruns source-order `ftMainSearchFighterCatch`, and proves the target is rejected before search target/distance or attack-record update; the local catch-search early guards now follow BattleShip's ghost, Boss, team, capture-immune, then hitstatus order.
- Previous catch-search ghost/Boss increment: the selected live-hit catch-search proof requires `STAGE_MPLIVEHIT_CATCHSEARCH` mask low bits `0xffff`. The added bits restore Mario/Fox, seed ghost and Boss targets, rerun source-order `ftMainSearchFighterCatch`, and prove each target is rejected before search target/distance or attack-record update.
- Previous catch-search team-gate increment: the selected live-hit catch-search proof requires `STAGE_MPLIVEHIT_CATCHSEARCH` mask low bits `0x3fff`. The added bit restores Mario/Fox, seeds same-team fighters with team battle on and team attack off, reruns source-order `ftMainSearchFighterCatch`, and proves the target is rejected before search target/distance or attack-record update.
- Previous catch-search capture-immune increment: the selected live-hit catch-search proof requires `STAGE_MPLIVEHIT_CATCHSEARCH` mask low bits `0x1fff`. The added bit restores Mario/Fox, seeds `capture_immune_mask & catch_mask`, reruns source-order `ftMainSearchFighterCatch`, and proves the target is rejected before search target/distance or attack-record update.
- Previous catch-search natural-slot increment: the selected live-hit catch-search proof requires `STAGE_MPLIVEHIT_CATCHSEARCH` mask low bits `0xfff`. The added bit restores the saved Mario/Fox fighter structs, clears the catch attack record, positions the selected catch hitbox on Mario's natural damage-coll slot `0`, reruns source-order `ftMainSearchFighterCatch` without proof-local slot-3 masking, and proves target, distance, and attack-record update before restoring state.
- Previous catch-search repeat increment: the selected live-hit catch-search proof requires `STAGE_MPLIVEHIT_CATCHSEARCH` mask low bits `0x7ff`. The added bit keeps the catch attack record from the first source-order search intact, reruns `ftMainSearchFighterCatch`, and proves the same-victim catch repeat is rejected with no new closest target/distance update.
- Previous shield-contact repeat increment: the selected live-hit shield-contact proof requires `STAGE_MPLIVEHIT_SHIELD_CONTACT` mask low bits `0x7fffff`. The added bit keeps the shield attack record intact, reruns source-order `ftMainSearchHitFighter`, and proves the same-victim shield repeat is rejected with no new shield collision, stat, effect, or shield-damage changes.
- Previous natural hurtbox repeat increment: the selected live-hit hurtbox-damage consume proof required `STAGE_MPLIVEHIT_HURTBOX_DAMAGE` mask low bits `0x7ff`. The added bit keeps the natural slot-0 attack record intact, reruns unmasked source-order `ftMainProcSearchHitAll`, and proves the same-victim repeat is rejected with no new hitlog, queue growth, or percent growth.
- Previous CliffCatch increment: the selected live-hit status-loop DamageFall cliff branch replays `ftCommonDamageFallProcMap` through a restored cliff-collision seed that reaches imported-original `ftCommonCliffCatchSetStatus` under a proof-local guard. The public marker widened from `cliff=0x1f` to `cliff=0x3f`; the bit proves the imported CliffCatch status/motion, Air state, floor-line clear, cliff hold, callback slots, capture immunity, damage callback, and velocity stop before restoring the DamageFall seed.
- Previous Jump-loop increment: the standalone direct/menu-chain Jump-loop `JUMP_ATTACKAIR` proof keeps the neutral AttackAirN path exact while replaying BattleShip's imported `ftCommonAttackAirCheckInterruptCommon` directional selector for AttackAirF/B/Hi/Lw through a proof-only restored status shell. The marker requires `dir=0x1f` for those four status/motion/attack-ID branches plus the original AttackAirLw proc-hit/rehit callback setup, and `map=0x3ff` for the installed original `ftCommonAttackAirProcMap` landing branches.
- Previous damage-callback increment: restored the original `ftParamStopVoiceRunProcDamage` damage-callback tail for the DS compatibility shell. The helper now calls `FTStruct.proc_damage` after the existing capture voice-stop marker path, and the inherited Passive recover CapturePulled proof seeds a bounded callback and records `procDamage=1` before original CapturePulled status setup.
- Previous CliffWait DownBounce increment: strengthened the standalone CliffWait DamageFall floor-collision DownBounce handoff so the bounded `ftCommonDownBounceSetStatus` branch now chooses DownBounceD/U with BattleShip's original `ftCommonDownBounceCheckUpOrDown` orientation formula before the bounded status install. The same branch still routes its ImpactWave/FGM/rumble tail through imported-original `ftCommonDownBounceUpdateEffects`, clears attack buffer, sets `damage_mul=0.5`, and runs the ground velocity-transfer seam. The selected proof seed still lands on DownBounceU/Ground; full imported `ftCommonDownBounceSetStatus` remains deferred for that standalone runtime branch, but modes `161/162` now exercise it under a guarded restored DamageFall floor-collision seed.
- Previous CliffWait DamageFall routing increment: public `ftCommonDamageFallSetStatusFromCliffWait` now calls imported-original `ftCommonDamageFallSetStatusFromCliffWait`, records success only after the bounded `ftMainSetStatus` DamageFall install plus imported clamp/rumble tail run, and keeps `mpCliffWaitDamage=status=85->57 motion=73->50 fallWait=1->0 catch=30` stable. The bounded PassiveStand/Passive floor branches still give imported-original checks first chance, then preserve the existing original setter sequence through the proof-local status seam when the DS compatibility shell needs it.
- Previous WallDamage routing increment: strengthened the standalone passive/recover WallDamage update handoff so imported-original `ftCommonWallDamageProcUpdate` reaches imported-original `ftCommonDamageFallSetStatusFromDamage` and only records `wallDamage=56/49->57/50` after the bounded `ftMainSetStatus` DamageFall install plus imported clamp/rumble tail run. The public WallDamage rumble count remains scoped to original setup rumble ID `2`; Real Boss, natural continuous Twister/trap gameplay, natural continuous rebound, and full unbounded `ftmain.c` runtime remain deferred.
- Previous DamageFall routing increment: strengthened the Dash-Run hitstun-expiry DamageFall handoff so the existing expiry proof now calls imported-original `ftCommonDamageFallSetStatusFromDamage` and only records success after the bounded `ftMainSetStatus` DamageFall install plus imported clamp/rumble tail run. The marker surface remains stable.
- Previous ftmain search increment: the selected slot-3 hurtbox damage consume proof enters through bounded `ftMainProcSearchHitAll`, which clears hitlogs, calls `ftMainSearchHitFighter`, reaches deferred item/weapon/ground search stubs, and then calls `ftMainProcessHitCollisionStatsMain` when a fighter hitlog exists; modes `161/162` now also replay that fighter-only path unmasked for natural slot `0`. The selected attack-clash proof routes through `ftMainSearchHitFighter` using the original self-before-other fighter-link ordering, the selected shield-contact/stat proof routes through the same hub before `ftMainUpdateShieldStatFighter`, and the selected catch-search proof now reaches bounded `ftMainProcSearchCatch` hazard wait-timer/gate/callback order around `ftMainSearchFighterCatch` before `ftMainUpdateCatchStatFighter`, including repeat rejection, restored natural slot `0`, self rejection, ghost target rejection, Boss target rejection, same-team target rejection, capture-immune target rejection, target hitstatus rejection, Twister dispatch/tick, and TaruCannon setup/physics dispatch. Real weapon/item/ground hitlog branches, continuous Twister/TaruCannon runtime beyond the selected Twister tick and TaruCannon setup/physics, and full unbounded `ftmain.c` remain deferred.
- Current marker: `mpLiveHit=atk=191/166 hitbox=1/j14 dmg=4 second=0/j14/x140 hurt=3/10 hbdmg=0->4/6 eff=0x1ff/0->0 resist=0xfff/7->3 rbreak=0x7f/2->-2/q2 throwAttr=0x1f/1->0 clash=0x3f/24/18 catchStat=0x1f/160000 catchSearch=0xffffffff/s3 shield=4->4/4 shc=0x7fffff/3142 so=155/134 soTick=0x1f/155->154 contact=1 repeat=1 carry=0xf gate=0x3f rehit=5->0 origRehit=hit30/v40000 30->29->0 clear=1/3 ids=0x3 dmg=0->4 hitlag=6 status=17->52/45 hitlag=6->0 callbacks=1/6/1 update=2->1 phys=11500/-1000 interrupt=1 map=1/1 floor=1/1/1/1 wall=0x3ffff cliff=0x3f fallmap=0x7ff finish=57/50 search=0xf repeat=1/1 gate=0x3f`.
- Movement prerequisite repair: the Mario/Fox Jump/Fall/Landing -> process-loop -> scheduler -> preview -> `gcRunAll` proof ladder is green again. The bounded process-loop status helper now treats only JumpAnimEnd-driven `FallAerial` as the intended `Fall` handoff, so scheduler-mode prerequisites no longer reject P1's fall transition while standalone process mode remains unchanged.
- Regression context: modes `159/160` remain standalone selected live-hit damage-loop coverage, modes `155/156` remain the standalone PassiveStand/Passive recovery aggregate, modes `153/154` remain the standalone Inishie scale coverage, and modes `151/152` remain the standalone platform-speed reader/dynamic collision coverage.
- Inherited throw branch addendum: modes `157/158` inherit the bounded original catch-wait ThrowB/Ground `170/149` to ThrownCommon/Air `186/161`, ThrowF update-release, thrown release/status/proc-status, and dead-result cleanup markers from the Passive recover aggregate.
- Inherited cliff/wall/rebound addendum: modes `157/158` inherit the current-safe CliffAttack/Common2, CliffEscape/Common2, Hyrule wall-line/copy, WallDamage, and Rebound isolated proofs from modes `155/156`; WallDamage now hands off through imported-original `ftCommonDamageFallSetStatusFromDamage` while keeping marker `wallDamage=56/49->57/50`. These remain bounded proofs, not natural continuous wall/ledge/rebound gameplay.
- Current pass-through proof: the same modes now also require the existing bounded pass-through floor and natural drop-through input proofs. They reject same-line pass-through collision via `ignore_line_id`, accept the different-line probe through the pass callback, then seed original-compatible down input on Dream Land line `0` and route Wait -> Squat -> Pass through imported original `ftcommonpass.c` / `ftcommonsquat.c`. The same pass-input proof now reuses the imported Squat helpers to prove SquatWait -> SquatRv -> Wait through installed original callback slots, with marker `rv=29->30->10`.
- Current platform proof: the same modes now also require the existing bounded active platform-floor, platform-tick, platform-position, platform-speed, and Inishie scale proofs. They check the selected pass-through line against BattleShip's yakumono platform predicate, install the bounded yakumono DObj, run one guarded `mpCollisionAdvanceUpdateTic` step, call `mpCollisionSetYakumonoPosID`, read speed through `mpCollisionGetSpeedLineID`, run bounded dynamic floor/ceil/wall, wall-process, animation, bounds, and stage-animation diagnostic slices, then stage `StageInishieFile3` and drive source-backed `grInishieMakeScale` / `grInishieScaleProcUpdate` without enabling continuous moving-platform runtime.
- Current dash-run regression guard: the same modes now also assert the existing bounded Dash-Run attack/guard aggregate proof and the selected hitbox position/range/rectangle/collide/record/hit-log/SFX/stats/proc-params plus damage-status selector/setup/proc-passive/physics/flytop/replacement-electric/flyroll/knockback-invincible/lag-update/hitlag-lifecycle/air-map/interrupt/expiry/fall-physics/fastfall/map-floor-cliff markers, printing `dashRun=0x3fffffff hitboxPos=0x1ffff hitboxIds=0x3 procRumble=0x7f procRebound=0x1f damageStatus=0x1f damageSetup=0xffffffff dust=0xff dustUpdate=0x1f hitPublic=0xf colAnim=0x3f colAnimUpdate=0x1f invGate=0x1f lagUpdate=0x3f commonPhys=0x3f commonCb=0x3fff level=0x1f hold=0xff catchResist=0x1f airMapWall=0x3f angle=0x3f fallInterrupt=0x3f flash=0x7f public=0x3f voice=0xf flytop=0xf replace=0x3f flyroll=0x1f kirbycopy=0x6 itemHeavy=0x1f itemBypass=0x1f dmgKind=0x7f sleep=0x7f`. The `procRebound=0x1f` field proves the selected `attack_shield_push` + `attack_rebound` promotion into imported-original ReboundWait status/callback/vector/hitlag/clear effects; the `dmgKind=0x7f` field proves the selected setup enters imported original `ftCommonDamageInitDamageVars`, routes through imported original `ftCommonDamageGotoDamageStatus`, preserves `FTStruct.damage_kind`, and proves the bounded Twister `nFTDamageKindColAnim` plus installed `proc_trap` branch in source order; `dust=0xff` proves threshold buckets through public `ftCommonDamageSetDustEffectInterval` routing into imported original code; the `dustUpdate=0x1f` field proves runtime dust decrement/spawn/reset through imported original `ftCommonDamageUpdateDustEffect`; the `hitPublic=0xf` field proves hitstun decrement/public-knockback transfer through imported original `ftCommonDamageDecHitStunSetPublic`; the `colAnim=0x3f` field proves damage color-animation routing through imported original `ftCommonDamageCheckElementSetColAnim`; the `colAnimUpdate=0x1f` field proves damage color-animation update wrappers through imported original `ftCommonDamageUpdateDamageColAnim` and `ftCommonDamageSetDamageColAnim`; the `invGate=0x1f` field proves knockback invincibility gates through imported original `ftCommonDamageCheckSetInvincible`; the `commonPhys=0x3f` field proves common damage physics branches through imported original `ftCommonDamageCommonProcPhysics`; the `commonCb=0x3fff` field proves common damage ground/air update stay/expiry plus common interrupt ground/fall/hammer and AirCommon interrupt branches through imported original callbacks; the `level=0x1f` field proves damage-level thresholds through imported original `ftCommonDamageGetDamageLevel`; the `angle=0x3f` field proves the Sakurai-angle branches through imported original `ftCommonDamageGetKnockbackAngle`; the `flash=0x7f` field proves screen-flash routing through imported original `ftCommonDamageCheckMakeScreenFlash`; the `public=0x3f` field proves public-reaction routing through imported original `ftCommonDamageSetPublic`; the `catchResist=0x1f` field proves the bounded `ftCommonDamageUpdateCatchResist` branch paths, including one zero-knockback color-animation branch through the imported original function; the `replace=0x3f` field proves the hitlag-blocked passive-dispatch no-op plus zero-hitlag stored replacement dispatch; full item runtime remains deferred.
- Current DamageFall interrupt addendum: the same Dash-Run damage marker now also prints `fallInterrupt=0x3f`, proving bounded imported `ftCommonDamageFallProcInterrupt` source-order child checks and restore. The Attack11 call counters remain additive (`>= 2`) while status/motion IDs and callback masks remain exact.
- Current knockback-angle/FlyTop addendum: the same Dash-Run damage marker now also prints `angle=0x3f` and `flytop=0xf`, proving the imported-original `ftCommonDamageGetKnockbackAngle` fixed-angle/Sakurai-angle branches and the `ftCommonDamageInitDamageVars` deterministic FlyTop angle-window branch before restoring local state.
- Current damage-init addendum: the project-owned `ftCommonDamageInitDamageVars` seam keeps local ownership but routes the selected high-knockback setup call through imported original BattleShip `ftCommonDamageInitDamageVars`. It preserves the previous original-shaped angle-derived `cos/sin` knockback vector routing, source-order ground-to-air conversion, high-damage floor-bounce ImpactWave/QuakeMag0 effect seams, deterministic FlyTop selection, final status replacement/electric wrapping with hitlag-blocked passive-dispatch no-op and zero-hitlag dispatch to the stored replacement, random FlyRoll percent/RNG selection, bounded Kirby copy-loss reset/audio seam, bounded damage dust interval thresholds, bounded dust update runtime, bounded hitstun/public-knockback decay, bounded damage color-animation routing/update wrapper reachability, bounded high-knockback screen-flash routing, bounded public-knockback reaction handling, bounded damage voice SFX threshold/forced-call branches, and explicit preservation of `FTStruct.damage_kind` across hit-element setup.
- Current FuraSleep/addendum: the damage setup side proof also runs imported original `ftCommonFuraSleepProcUpdate` through one A-tap plus stick mash breakout branch before the forced Wait handoff; the procparams side proof now routes the public-wrapper `ftCommonDamageUpdateMain` catch/capture keep-hold, catch/capture zero-knockback, catch/capture stats/release/no-damage, no-grab/no-capture tail-colanim, damage-status/Sleep, held-item bypass/resist/drop branches through the imported original dispatcher. The thrown damage-stat and release-status helper calls inside that dispatcher reach imported originals; complete seeded lose-grip links now route through imported original `ftcommonthrown2.c`, with the bounded local guard only for incomplete proof seeds.
- Supporting proof update: the older direct/menu-chain Dash -> Run -> RunBrake modes now import original `ftcommonturnrun.c`, `ftcommonattack1.c`, `ftcommonattack100.c`, `ftcommonattackdash.c`, `ftcommonguard1.c`, `ftcommonguard2.c`, and `ftcommonescape.c`. They first prove a bounded Wait A-tap through original `ftCommonAttack1CheckInterruptCommon` into Attack11 status/motion `190/165` with callback/tick masks `0xff` and wait-proc mask `0x3`, prove a bounded Z-hold through original `ftCommonGuardOnCheckInterruptCommon` into GuardOn status/motion `152/134` with callback mask `0xff`, state mask `0xfffffe0f`, FGM `13`, one bounded original GuardOn update tick that preserves GuardOn while advancing shield decay/release counters, one animation-end handoff through original `ftCommonGuardSetStatus` into Guard status `153` with original Guard callbacks, one bounded original Guard hold update tick that stays in Guard and advances shield counters again, one release tick through original `ftCommonGuardOffSetStatus` into GuardOff status/motion `154/135`, and one GuardOff animation-end update through original `ftCommonWaitSetStatus` back to Wait/Ground before restoring Guard for Escape. They now also prove a bounded original GuardSetOff branch: `DASH_RUN_GUARD_SETOFF=4/4 mask=0xfff cb=0xff frame=20200 vel=-40400`, with held Z returning to Guard and released Z returning to GuardOff. They then prove original `ftCommonEscapeCheckInterruptGuard` reaches EscapeF/EscapeB status/motion `156/136` and `157/137` with callback mask `0x3ff`, state mask `0xff`, `itemthrow_buffer_tics=5`, and one guarded installed Escape callback tick per fighter (`tick=0x3ff`, interrupt/physics/map counts `2/2/2`) including the original update LR flip and animation-end Wait/Ground handoff. They prove the original Attack11 follow-up gate reaches Attack12 status/motion `191/166` with callback mask `0xff` and goto mask `0xf`, then prove the original Attack12 follow-up gate reaches Mario Attack13 status/motion `220/195` while Fox remains blocked at Attack12 (`191/166`) by the original fighter-kind gate. The same installed original Attack12 callbacks then prove Fox's original rapid-jab gate reaches Attack100Start status/motion `220/195` with callback mask `0xf` and goto mask `0x3`, drive Attack100Start through the animation-end handoff into Fox Attack100Loop status/motion `221/196`, then prove one guarded Loop tick plus a no-input Loop -> Attack100End -> Wait/Ground handoff (`tick=0xfff`). They now also prove `DASH_RUN_ATTACK_ANIM=0x3f` for original Attack11, Attack12, Mario Attack13, and Fox Attack100Start calls into the project-owned `ftMainPlayAnimEventsAll` seam; AttackDash is excluded because original `ftCommonAttackDashSetStatus` does not call that hook. The same seam now scans selected original Mario/Fox main-motion command extracts, decodes ten real `MakeAttackColl` commands into `FTStruct.attack_colls[0]` / `[1]`, verifies `DASH_RUN_ATTACK_EVENT=0x1f/0x3f/0x20/10`, verifies `DASH_RUN_ATTACK_EVENT_CMDS=0xf` for Mario Jab3 damage, size, clear, and clear-off command handling, and records the current source-order last decoded hitbox as Fox Jab2 attack ID `1` (`damage=4`, `size=100`, `offset=0/0/0`, flags `0x7`). They also prove a bounded original Run -> TurnRun -> Run branch through imported `ftcommonturnrun.c`: TurnRun status/motion `19/13`, final Run status/motion `16/10`, callback/update masks `0xff/0xf`, four guarded update ticks total, and LR/ground-velocity flip checks for both fighters. They also prove the existing Run A-tap through installed original `ftCommonRunProcInterrupt` (`runproc=0x3`) into AttackDash status/motion `192/167` with callback mask `0xff` and tick mask `0x3f`. This is a bounded status/callback/update/event-decode proof only; full animation command runtime, staled-damage handling, hitbox activation, hit detection, continuous Attack100Loop runtime, rapid-jab hitboxes, continuous TurnRun runtime beyond the isolated callback/update handoff proof, continuous Guard hold, continuous SetOff/Escape shield runtime beyond isolated callback proofs, item attack branches, and broader attack gameplay remain deferred.
- Supporting hitbox-position proof update: the same direct/menu-chain Dash modes now also verify `DASH_RUN_ATTACK_EVENT_POS` mask `0x1ffff`, proving the selected decoded Fox Jab2 hitbox can run bounded original `New -> Transfer`, gated writeback into `FTStruct.attack_colls[1]`, then `Transfer -> Interpolate` with `pos_prev = old pos_curr`, joint `14`, original `FTAttackMatrix` reset, the original broad-phase fighter-range predicate on current/previous positions, one bounded original-compatible rectangle probe against Mario's selected descriptor-backed hurtbox, one selected `gmCollisionCheckFighterAttackDamageCollide`-style decision, a bounded source-order multi-slot hurtbox scan that proves the global hitstatus and damage-detect gates, skips slots `0` and `1`, tests and misses slot `2`, tests and hits slot `3`, and observes the `None` sentinel after Mario's `10` active slots, source-shaped damage-record insertion into Fox's attack-record array, a bounded normal-hit front half of `ftMainUpdateDamageStatFighter` that records captured damage, attacker `attack_damage`, victim `damage_queue` / `damage_lag`, the first `FTHitLog` entry, the player-stat/stale-queue compatibility seams, the original hit-collision FGM table selection into the existing audio stub seam, a selected `ftMainProcessHitCollisionStatsMain` fighter-hitlog stats handoff that fills victim damage angle/LR/index/player metadata and computed knockback, and a source-shaped `ftMainProcParams` damage/hitlag scheduling handoff that consumes Mario's queued damage, updates percent damage, reaches the damage-status/shuffle/rumble seams, computes hitlag, pauses knockback, calls the selected `proc_lagstart` tail, clears transient damage fields, feeds the source-order slot-3 hurtbox result through the same bounded damage record/hitlog/stat/percent/hitlag path, replays the unmasked natural slot-0 consume path plus immediate attack-record repeat rejection, and now proves natural slot-1/2/4/5/6/7/8/9 consume before restoring both fighter structs. It also proves Fox's attacker-side `attack_damage` / `proc_hit` and `attack_shield_push` / `proc_shield` branches plus Mario's victim `shield_damage` -> GuardSetOff, shield-break -> ShieldBreakFly, reflect-damage break, Fox reflector hit, Ness reflector sound, and Ness absorb branches, proves the BattleShip-compatible `ftcommondamage.c` damage-level/status-table selector, and now runs a guarded damage-status setup/update/physics/interrupt/expiry side proof before restoring status for the wider preview chain. This is a selected live state proof only; full continuous multi-hitbox runtime, broader natural continuous multi-slot hurtbox runtime beyond the restored slot-0 consume/repeat, natural slot-1/2/4/5/6/7/8/9 consume, and selected slot-3 proofs, continuous rehit gameplay beyond the selected Link down-air timer window, full damage status lifecycle, effects, real DS audio, positional audio balance, and exact Fox/Ness special runtimes remain deferred.
- Supporting hurtbox-state proof update: the direct and menu-chain Mario/Fox init harnesses now prove a bounded original-compatible `FTDamageColl[11]` shell plus `FTParts` transform shells for the selected damage-coll joints. The init seam copies every valid source descriptor slot like original `ftmanager.c`: Mario has `10` active damage-coll slots and Fox has `11`. Selected slot 0 remains the live-hit target, with Mario joint `6` half-size `51.5/56.0/47.5` and Fox joint `5` half-size `51.0/26.0/22.5`. The proof remains summarized as `damageColl=0x3/0x3/0x3/0x3` and `parts=0x3/0x3/0x3` for descriptor copy, hit status, joint attachment, FTParts attachment, matrix/world-position consistency, and scale availability. Modes `159/160` prove bounded source-order slot-3 hurtbox damage bookkeeping after one tested miss; modes `161/162` now also prove restored unmasked slot-0 consume/repeat rejection and natural slot-1/2/4/5/6/7/8/9 consume. This is not full hurtbox collision; broader natural continuous multi-slot victim runtime remains deferred.
- Latest markers: `mpLiveHit=atk=191/166 hitbox=1/j14 dmg=4 second=0/j14/x140 hurt=3/10 hbdmg=0->4/6 eff=0x1ff/0->0 resist=0xfff/7->3 rbreak=0x7f/2->-2/q2 throwAttr=0x1f/1->0 clash=0x3f/24/18 catchStat=0x1f/160000 catchSearch=0xffffffff/s3 shield=4->4/4 shc=0x7fffff/3142 so=155/134 soTick=0x1f/155->154 contact=1 repeat=1 carry=0xf gate=0x3f rehit=5->0 origRehit=hit30/v40000 30->29->0 clear=1/3 ids=0x3 dmg=0->4 hitlag=6 status=17->52/45 hitlag=6->0 callbacks=1/6/1 update=2->1 phys=11500/-1000 interrupt=1 map=1/1 floor=1/1/1/1 wall=0x3ffff cliff=0x3f fallmap=0x7ff finish=57/50 search=0xf repeat=1/1 gate=0x3f`, `mpDamageRecover=contact=1/1 dmg=0->4 hitlag=6 status=52/45 fall=57/50 ps=1 passive=1 dbounce=1`, `mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3`, `mpCliffWaitDamage=status=85->57 motion=73->50 fallWait=1->0 catch=30`, `passiveStand=73/62/0->cb1/1->10/4/0`, `passive=81/70/0->cb1/1->10/4/0`, `mpPassive=ps=73/62->10/4 ticks=6/5/5 passive=81/70->10/4 ticks=6/5/5 branch=0x7f psb=0xf natural=0x7f mapcalls=2 appeal=189/164 appealGuard=152/134 catch=166/146->10/4 catchPull=167/147->168/-2 tw=60->59 capture=171/150->172/-2 procDamage=1 throw=169/148->186/161 throwB=170/149->186/161 throwCb=1/1/1 floor=3 end=1/186/161 throwUpdate=169/148 dmg=50->58 script=0 throwRelease=dmg=10->18 kb=6600000 script=123 throwReleaseStatus=dmg=20->26 upd=30->36 noDmg=40->40 throwProc=param=1 script=123 throwDead=call=1 coll=1 air=1 waitFall=2 status=10/26 wallDamage=56/49->57/50 rebound=82/-1->83/71->10/4`, and `dashRun=0x3fffffff procRumble=0x7f procRebound=0x1f dust=0xff dustUpdate=0x1f hitPublic=0xf colAnim=0x3f colAnimUpdate=0x1f invGate=0x1f lagUpdate=0x3f commonPhys=0x3f commonCb=0x3fff level=0x1f hold=0xff catchResist=0x1f airMapWall=0x3f angle=0x3f fallInterrupt=0x3f flash=0x7f public=0x3f voice=0xf flytop=0xf replace=0x3f flyroll=0x1f kirbycopy=0x6 itemHeavy=0x1f itemBypass=0x1f dmgKind=0x7f sleep=0x7f loseGrip=0x7b/6/6`.
- Latest thrown callback marker: `throwCb=1/1/1 floor=3 end=1/186/161`.
- Latest throw update marker: `throwUpdate=169/148 dmg=50->58 script=0`.
- Latest thrown dead-result marker: `throwDead=call=1 coll=1 air=1 waitFall=2 status=10/26`.
- Latest wall-line marker: `mpWallHitFloor=pass floor=5 wall=13 edge=12 side=0 mapNodes=1 delta=-1600/-388`.
- Latest wall-copy marker: `mpWallCopy=pass floor=5 wall=13 edge=12 delta=-1600/-388`.
- Latest pass-through marker: `mpPass=line=0 flags=0x4000 route=2 same=1 diff=1 cb=1/1/0`.
- Latest pass-input marker: `mpPassInput=line=0 flags=0x4000 squat=28 pass=33 rv=29->30->10 ignore=0`.
- Latest platform marker: `mpPlatform=line=0 yak=1 dobj=1 status=1 anim=0 active`.
- Latest platform tick marker: `mpPlatformTick=tic=0->1 setOn=1 advance=1 status=1/1`.
- Latest platform position marker: `mpPlatformPos=line=0 yak=1 delta=12000/-4000/2000`.
- Latest platform speed marker: `mpPlatformSpeed=line=0 yak=1 read=12000/-4000 dyn=1/2 ceil=1/1 wall=1/1 procwall=1/1 anim=1 bounds=1 stageanim=0/0x91 inishieAsset=header/geometry nodes=1`.
- Latest Inishie scale marker: `inishieScale=ticks=2 lines=1/2 alt=80000->64000 y=363000/362000->427000/298000 speed=-8000/8000 fall=1->2/0 step=3->0/0 sourceDL=0xff cmds=91 tris=20 tex=0x3f preview=0x3f px=432`.
- Latest dash-run aggregate marker: `dashRun=0x3fffffff hitboxPos=0x1ffff hitboxIds=0x3 procRumble=0x7f procRebound=0x1f damageStatus=0x1f damageSetup=0xffffffff dust=0xff dustUpdate=0x1f hitPublic=0xf colAnim=0x3f colAnimUpdate=0x1f invGate=0x1f lagUpdate=0x3f commonPhys=0x3f commonCb=0x3fff level=0x1f hold=0xff catchResist=0x1f airMapWall=0x3f angle=0x3f fallInterrupt=0x3f flash=0x7f public=0x3f voice=0xf flytop=0xf replace=0x3f flyroll=0x1f kirbycopy=0x6 itemHeavy=0x1f itemBypass=0x1f dmgKind=0x7f sleep=0x7f`.
- Verifier timing note: use `-DelaySeconds 3` for `verify-boundary.ps1`, `verify-current.ps1`, and direct menu-chain wrapper reruns on the current pair. A 1-second menu-chain capture can stop before post-loop finalizer markers are published.
- Previous boundary: Inishie/Mushroom Kingdom scale modes `153/154` remain standalone regression coverage for the same source-backed scale proof now required by current modes `155/156`.
- Current blocker: this is still a bounded selected live-hit/status proof, not a full player-driven combat subsystem. Full `gmcollision.c`, full `ftmain.c`, continuous multi-hitbox activation, broader natural multi-slot hurtbox/damage runtime beyond the restored slot-0 consume/repeat, natural slot-1/2/4/5/6/7/8/9 consume, and selected slot-3 proofs, broader natural catch-search runtime beyond the restored slot-0, self, ghost, Boss, same-team, capture-immune, target-hitstatus gate, selected hazard ghost-gate proof, and selected Twister true-return/update/physics tick proofs, continuous shield runtime beyond the selected contact/set-off/update-tick/health-decrement/break/heal/break-clear/tail-clear/special-clear/hitlag-mul-clear proof, continuous rehit gameplay beyond the selected Link down-air timer window, arbitrary damage-state duration, full hitlag/damage runtime, specials/items/HUD/audio, and unbounded gameplay scheduling remain deferred.
- Throw cleanup caveat: the current dead-result marker is a bounded lose-grip cleanup proof, not continuous death/throw/damage scheduling.
- Wall-runtime caveat: the current wall-hit, wall-copy, and WallDamage markers are bounded Hyrule probe proofs, not natural live wall collision/copyback/teching gameplay.
- Pass-through caveat: the current `mpPass` / `mpPassInput` markers prove the bounded collision route and one natural down-input drop-through path, not continuous moving-platform pass-through gameplay.
- Platform caveat: the current `mpPlatform` / `mpPlatformTick` / `mpPlatformPos` / `mpPlatformSpeed` / `inishieScale` markers prove active platform primitives and one source-backed Inishie scale slice, not continuous moving platform gameplay.
- Cliff common2 integration caveat: CliffAttack/Common2 and
  CliffEscape/Common2 are now current-safe in modes `155/156` because both
  reseed at the delayed aggregate probe and isolate shared bridge diagnostics.
  Continuous natural ledge attack/escape runtime remains deferred.
- Next best work: choose the next bounded source-driven gameplay boundary after the selected live-hit proof. Good candidates are another selected attack's original rehit/window behavior, broader natural multi-slot victim runtime beyond the restored slot-0 consume/repeat, natural slot-1/2/4/5/6/7/8/9 consume, selected slot-3 bookkeeping proofs, TaruCannon update/shoot runtime once the original Jungle barrel helpers and map throw-hit data exist locally, continuous shield runtime beyond the selected contact/set-off/update-tick/health-decrement/break/heal/break-clear/tail-clear/special-clear/hitlag-mul-clear proof, or the next small `ftcommondamage.c` runtime callback not already covered. Keep it restored after proof and avoid broad `ftmain.c` / `gmcollision.c` / item runtime imports.
- Build note: fighter/stage harness modes `>= 11` now reserve one opening-action preview cache entry instead of three so menu-chain harness ROMs stay under the DS ARM9 memory limit. Normal boot/opening builds keep the three-entry cache.
- Build note: modes `159/160/161/162` apply scoped `-Os` after their harness defines because the live-hit boundary ROMs are ARM9-size-tight. If a stale build directory predates that flag, force that target once with `make -B` or use a clean build.
- Shortest safe verifier sequence for docs-only edits: `.\scripts\check-harness-registry.ps1`; for boundary code use `.\scripts\verify-dev-fast.ps1 -DelaySeconds 3`, `.\scripts\verify-boundary.ps1 -DelaySeconds 3`, then `.\scripts\verify-current.ps1 -Build -DelaySeconds 3` after source changes that can affect the shared normal runtime ROM.
- Files most likely to matter: `Makefile`, `include/gr/ground.h`, `include/ef/effect.h`, `include/reloc_data.h`, `src/import/battleship_grinishie_scale.c`, `src/import/battleship_grpupupu_ground.c`, `src/port/scene_harness.c`, `src/port/taskman_seam.c`, `src/port/reloc_backend.c`, `include/nds/nds_scene_harness.h`, `include/nds/nds_startup.h`, and focused `include/mp/map.h` additions.
- Read first: `docs/MP_PASS_THROUGH_SCOUT.md`, then original BattleShip `decomp/BattleShip-main/decomp/src/gr/grcommon/grinishie.c`, `decomp/BattleShip-main/decomp/src/mp/mpcollision.c`, `decomp/BattleShip-main/decomp/src/mp/mpprocess.c`, and `decomp/sm64-nds` only for DS backend architecture choices.
- Avoid broad-importing: full `ftmain.c`, all renderer/object-display code such as `sys/objdisplay.c`, broad stage/audio/item/HUD systems, or generated/decomp outputs.
- Known blockers: unpaused full-scene scheduling/draw, full map collision, natural ledge/platform/wall runtime, attacks/specials/full shield runtime/catch/items/HUD/audio, and full DS hardware renderer remain deferred.

## Fast Commands

```powershell
.\scripts\verify-dev-fast.ps1
.\scripts\verify-boundary.ps1
.\scripts\verify-current.ps1
.\scripts\verify-regression.ps1
.\scripts\verify-battle-mariofox-gcdrawall-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-gcdrawall-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-gcdrawall-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-gcdrawall-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-collision-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-collision-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-floor-follow-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-floor-follow-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-floor-edge-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-floor-edge-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpprocess-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpprocess-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpupdate-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpupdate-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpsweep-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpsweep-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpcross-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpcross-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpadjust-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpadjust-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpedge-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpedge-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpwall-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpwall-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpstale-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpstale-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mplivestale-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mplivestale-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpmotionstale-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpmotionstale-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpcliffstatus-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpcliffstatus-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpclifftick-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpclifftick-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpfallmap-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpfallmap-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpfallland-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpfallland-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpceil-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpceil-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpceilstatus-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpceilstatus-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpcliffcatch-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpcliffcatch-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpcliffwait-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpcliffwait-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpcliffattack-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpcliffattack-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpcliffattack-action-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpcliffattack-action-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpcliffcommon2-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpcliffcommon2-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpcliffescape-action-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpcliffescape-action-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpcliffescape-common2-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpcliffescape-common2-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpcliffclimb-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpcliffclimb-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpcliffclimb-action-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpcliffclimb-action-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpcliffclimb-common2-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpcliffclimb-common2-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpcliffclimb-finish-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpcliffclimb-finish-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpcliffwait-damage-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpcliffwait-damage-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mppassive-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mppassive-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3
.\scripts\verify-menu-chain-mariofox-stage-mppassive-recover-loop-harness.ps1 -DelaySeconds 3
.\scripts\verify-battle-mariofox-stage-mpdownwait-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpdownwait-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-turn-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-turn-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpdownrecover-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpdownrecover-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpcliffledge-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpcliffledge-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpclifflive-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpclifflive-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpwallhit-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpwallhit-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpwallcopy-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpwallcopy-floor-loop-harness.ps1
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
.\scripts\verify-battle-mariofox-stage-mpplatform-pos-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpplatform-pos-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpplatform-speed-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpplatform-speed-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-inishie-scale-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-inishie-scale-loop-harness.ps1
.\scripts\check-harness-registry.ps1
.\scripts\clean-generated.ps1 -DryRun
.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean
```

Normal current-boundary handoff should use `verify-boundary.ps1` plus
`verify-current.ps1`. Shared/runtime-heavy changes should also run
`verify-regression.ps1`. `verify-current.ps1` is the Latest profile wrapper, so
do not also run `verify-all.ps1 -Profile Latest` unless profile plumbing is the
thing being tested. Full is reserved for major snapshots, wide refactors,
Makefile/source-list/header ABI changes, taskman/object-manager/controller/
reloc/display shared changes, verifier registry/checker work, or explicit
reviewer request.

Parallel regression sharding now uses isolated local melonDS runner slots:

```powershell
.\scripts\New-MelonDSRunnerSlots.ps1 -Count 4
.\scripts\build-verify-profile.ps1 -Profile Regression
.\scripts\verify-all.ps1 -Profile Regression -ShardCount 4 -ShardIndex 0 -RunnerSlot 0 -NoBuild
.\scripts\verify-all.ps1 -Profile Regression -ShardCount 4 -ShardIndex 1 -RunnerSlot 1 -NoBuild
.\scripts\verify-all.ps1 -Profile Regression -ShardCount 4 -ShardIndex 2 -RunnerSlot 2 -NoBuild
.\scripts\verify-all.ps1 -Profile Regression -ShardCount 4 -ShardIndex 3 -RunnerSlot 3 -NoBuild
```

Do not run parallel shards that rebuild into the same output dirs. Use prebuild
+ `-NoBuild`. Runner slots and shard logs are local/generated.

Latest runner-slot proof: after `.\scripts\build-verify-profile.ps1 -Profile
Boundary`, the two Boundary shards passed concurrently with `-NoBuild`:
shard `0` on slot `0` / ARM9 `3333` and shard `1` on slot `1` / ARM9 `3343`.
Slot logs landed under `artifacts/emulator-logs/slot0` and `slot1`; the
slot-specific verifier temp roots are `artifacts/verifier-temp/slot0` and
`slot1`.

Snapshot policy:

- `.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean` is the default handoff
  snapshot. It excludes generated build products, root ROM/ELF outputs,
  emulator logs/configs, local emulator payloads, artifacts, upstream decomp
  build output, decomp baseroms/generated binaries, duplicate nested O2R
  copies, and decomp tool caches by default.
- Lean still includes build-critical decomp context: imported BattleShip
  source/includes/symbols/docs/tools, sm64-nds source/backend reference files,
  and `decomp/BattleShip-main/BattleShip_o2r` for NitroFS reloc assets.
- Use `.\scripts\check-snapshot-build-context.ps1 -Mode Lean -Source .` or the
  same script with `-Archive <zip>` to prove Lean kept imported decomp include
  targets and Makefile-referenced O2R assets.
- `-Mode CodeOnly` is for tiny review-only handoffs and excludes `decomp/`.
- `-Mode Full` is only for explicit debugging/repro needs.
- `-IncludeDecompGenerated` is a Lean-only debug escape hatch for upstream
  generated decomp payloads; do not use it for normal handoff.
- `scripts/New-Smash64DSSnapshot.Legacy.ps1` is the preserved old broad
  exporter for fallback/debug reference, not the normal path.

## Current State

The ROM boots through original BattleShip startup, bounded Opening Room,
Opening Portraits, Opening Mario, the imported fighter name-card scenes, the
bounded action-scene bridge, imported bounded Title setup, a direct bounded VS
Mode setup harness, a bounded original VS Start -> PlayersVS transition
harness, bounded imported PlayersVS setup, bounded imported Maps setup, a
direct bounded `battle_fd` VSBattle setup harness, a direct bounded
`battle_pupupu_stage` ground-object harness, a direct bounded
`battle_pupupu_update` safe stage-update harness, and guarded menu-chain proofs
to the same imported VSBattle setup/update boundaries with Pupupu stage
adoption, bounded original Dream Land ground object setup, and asset-backed
Mario/Fox fighter model GObj creation with persistent project-owned
`FTStruct` state shells attached to those fighter GObjs plus a bounded
source-order Mario/Fox init-state proof, bounded Wait callback/ground proofs,
direct plus menu-chain Mario/Fox display metadata probes, and direct plus
menu-chain Mario/Fox display-list scan, execute/decode, visible first-DL
software draw proofs, visible multi-DL software draw proofs for the first four
DL-ready DObjs per fighter, and guarded all-DL software draw proofs through the
current `ftDisplayMainProcDisplay` seam for all current Mario/Fox DL-ready
DObjs, followed by direct and menu-chain original Wait -> Walk input/status/
velocity proofs through imported `ftcommonwalk.c`, and direct plus menu-chain
bounded Walk movement-loop/release-to-Wait proofs, then direct plus
menu-chain original Dash -> Run -> RunBrake movement proofs through imported
`ftcommondash.c`, `ftcommonrun.c`, and `ftcommonrunbrake.c`, followed by
direct and menu-chain original RunBrake -> Wait -> KneeBend -> JumpF airborne
movement proofs through imported `ftcommonkneebend.c` and `ftcommonjump.c`,
direct and menu-chain original JumpF -> Fall -> LandingLight -> Wait proofs
through imported `ftcommonfall.c` and `ftcommonlanding.c`, and direct plus
menu-chain bounded scripted fighter process-loop proofs that drive the proven
Walk, Dash/Run/RunBrake, and Jump/Fall/Landing paths through one shared
source-order frame loop and deterministic controller-input bridge, followed by
direct plus menu-chain VSBattle update-driven Mario/Fox `GObjProcess`
scheduler-loop proofs that attach selected fighter processes with original
`gcAddGObjProcess`, invoke them through `gcRunGObjProcess`, and run from the
bounded `scVSBattleFuncUpdate` taskman update path, followed by direct plus
menu-chain controller-source moving preview proofs, the first bounded original
object-manager `gcRunAll` moving-preview proofs, direct plus menu-chain
live-input Mario/Fox moving-preview idle proofs, direct plus menu-chain
bounded original `gcDrawAll` Mario/Fox moving-preview proofs, direct plus
menu-chain Pupupu stage-inclusive original `gcDrawAll` traversal proofs,
direct plus menu-chain geometry-backed Pupupu floor-collision proofs,
direct plus menu-chain continuous geometry-backed Pupupu floor-follow proofs,
direct plus menu-chain real Pupupu floor-edge / original MP floor-query proofs,
direct plus menu-chain source-order MP floor-process proofs, direct plus
menu-chain source-order `mpProcessUpdateMain` floor-loop proofs, direct plus
menu-chain source-order MP floor-line sweep / second-floor collision proofs,
direct plus menu-chain live selected-callback MP cross-floor proofs, direct
plus menu-chain source-order MP floor-edge-adjust proofs, direct plus
menu-chain source-order MP edge-under/floor-edge proofs, direct plus
menu-chain MP wall-blocker proofs, direct plus menu-chain MP stale-valid
second-floor proofs, direct plus menu-chain selected-callback live-stale
second-floor proofs, and direct plus menu-chain selected-callback/root
motion-stale second-floor proofs, direct plus menu-chain source-order MP
cliff-status branch proofs, direct plus menu-chain bounded Ottotto/Fall
callback-tick proofs, direct plus menu-chain bounded Fall physics/map callback
proofs, direct plus menu-chain bounded Fall landing-floor proofs, direct plus
menu-chain source-order MP ceiling-floor collision/adjust proofs, direct plus
menu-chain MP ceiling-hit StopCeil status proofs, direct plus menu-chain
MP cliff-catch status proofs, direct plus menu-chain original
CliffCatch -> CliffWait proofs, direct plus menu-chain original
CliffWait -> CliffQuick/AttackQuick setup proofs, and direct plus menu-chain
original CliffQuick -> CliffAttackQuick1 -> CliffAttackQuick2 action/update
proofs, followed by direct plus menu-chain bounded original
CliffAttackQuick2 common2 update/physics/map tick proofs, direct plus
menu-chain original CliffWait Z-button escape action proofs through
CliffEscapeQuick2, and direct plus menu-chain bounded original
CliffEscapeQuick2 common2 update/physics/map tick proofs, followed by direct
plus menu-chain bounded original CliffWait climb/fall interrupt proofs, then
direct plus menu-chain bounded original CliffQuick -> CliffClimbQuick1 ->
CliffClimbQuick2 action proofs, direct plus menu-chain bounded original
CliffClimbQuick2 common2 update/physics/map tick proofs, and direct plus
menu-chain bounded original CliffClimbQuick2 finish handoff proofs into
Wait/Ground, followed by direct plus menu-chain bounded original CliffWait
timeout proofs into DamageFall/Air plus one guarded original DamageFall
interrupt/physics/map no-collision tick, one positive right-cliff map branch
into bounded original CliffCatch setup, and one positive floor-collision branch
into bounded original DownBounce setup, then bounded original DownBounce,
DownWait, and DownStand timeout update callback ticks. The stage MP
Passive-loop harnesses consume the verified PassiveStand/Passive setup states,
run two guarded stable update/physics/map frames for PassiveStandF/Ground
`73/62/0` and Passive/Ground `81/70/0`, then prove the original animation-end
handoff into Wait/Ground `10/4/0`. The stage MP DownWait-loop
harnesses consume that Passive-loop proof, prime a fresh DownWaitU/Ground
state, and prove original DownWait interrupt source order `0x12345` into
DownAttackU `80/69`, DownForwardU `76/65`, DownBackU `78/67`, and
DownStandU/Ground `72/61/0` with `flag1` cleared and `damage_mul=1.0`; the
DownAttackU branch proves attack IDs `53/33/33`, and the attack/roll branches
now run eight guarded stable callback frames, with the roll branches proving
bounded root motion `+10000/-10000` milli, before proving the original
`ftAnimEndCheckSetStatus` handoff back to Wait/Ground `10/4/0`. The current
stage MP Turn-loop harnesses consume that DownWait-loop proof, import original
`ftcommonturn.c`, and prove a fresh Wait/Ground fighter accepts original
Turn input, reaches Turn `18/12`, flips facing `1 -> -1` and ground velocity
`2500 -> -2500` milli in `ftCommonTurnProcUpdate`, runs guarded physics/map
callbacks, then returns through the original animation-end path to
Wait/Ground `10/4/0`. The stage MP proof chain
starts from the verified
stage-inclusive `gcDrawAll` endpoint, re-enable deterministic playback for a
bounded moving slice, advance selected Mario/Fox callbacks through original
`gcRunAll`, and render moving all-DL software-preview keyframes by calling
original `gcDrawAll()` rather than manually invoking
`ftDisplayMainProcDisplay`. Non-target display callbacks are masked/guarded,
selected Mario/Fox display callbacks are reached through the object-manager
draw traversal, selected Pupupu layer/map display GObjs are captured through
the same camera path and reach the DS-owned DObj draw bridges, modes `63/64`
project floor checks against real Pupupu `MPGroundData` / `MPGeometryData`
during the bounded update path, modes `65/66` seed both fighters near the
widest decoded floor line edges and prove original-compatible MP floor query
helpers, modes `67/68` run a project-owned source-order `mpprocess.c` floor
slice through an original-layout `MPCollData` adapter, modes `69/70` route
selected Mario/Fox map callbacks through bounded source-order
`mpProcessUpdateMain`, modes `71/72` replace the previously deferred
second-floor branch with bounded `mpProcessCheckTestFloorCollision` plus
same-line and different-line floor sweep helpers, and modes `73/74` prove the
accepted second-floor path through the live selected Mario/Fox callback route,
while modes `75/76` replace the previous floor-edge-adjust deferral with
bounded source-order `mpProcessRunFloorEdgeAdjust` left/right checks, and
modes `77/78` replace the previous edge-under lookup deferral with decoded
Pupupu geometry adjacency to wall line IDs `6/5`, while modes `79/80` prove
the current selected Dream Land floor's real wall-hit adjustment is blocked
because the only left/right wall candidates are the same edge-under wall lines
that original `mpProcessCheckFloorEdgeCollisionL/R` rejects, and modes
`81/82` add a finalizer-local valid-stale second-floor probe from Dream Land
floor line `1` to line `0` through source-order
`mpProcessCheckTestFloorCollision`, and modes `83/84` run that same valid-stale
pair from the selected P0 callback path as a contained local `MPCollData`
source-order pass through `mpProcessUpdateMain`,
`mpCommonRunFighterAllCollisions`, and
`mpProcessCheckTestFloorCollision`, while modes `85/86` seed that same pair
into the selected P0 root/collision shell, run the selected callback through
the same source-order path, accept Dream Land floor line `1 -> 0`, and copy the
target floor back to the live fighter state while P1 remains grounded on line
`3`, and modes `87/88` import original `ftcommonottotto.c` and prove the
bounded source-order `mpCommonProcFighterOnCliffEdge` status branch: P0 with
`MAP_FLAG_FLOOREDGE` reaches original Ottotto status/motion `36/30`, while P1
without that flag reaches original Fall status/motion `26/20` and air state,
and modes `89/90` consume those statuses without opening gameplay scheduling:
P0 runs one guarded original `ftCommonOttottoProcUpdate`,
`ftCommonOttottoProcInterrupt`, and `ftCommonOttottoProcMap` tick while
remaining Ottotto/Ground on floor line `0`, and P1 runs one guarded original
`ftCommonFallProcInterrupt` tick, reaches the three original air interrupt
checks, and remains Fall/Air on floor line `3`, and modes `91/92` consume that
P1 Fall state, call the selected status-table original
`ftPhysicsApplyAirVelDriftFastFall` path, record guarded fast-fall, gravity,
air-drift, and air-friction seams, integrate one bounded airborne step, and
call the selected original `mpCommonProcFighterCliffFloorCeil` map callback
through a guarded no-collision branch while P1 stays Fall/Air on floor line
`3`, and modes `93/94` then seed P1 just above decoded Pupupu floor line `3`,
cross the floor through the selected original map callback route, call
source-order landing-floor setup, reach original LandingLight status/motion
`31/25`, switch to Ground, and clamp vertical velocity from `-8000` to `0`,
modes `95/96` choose real Pupupu ceiling line `4`, run
`mpProcessCheckTestCeilCollisionAdjNew` through the bounded different-line
ceiling sweep and `mpCollisionGetFCCommonCeil`, then run
`mpProcessRunCeilCollisionAdjNew` and prove current/stat ceiling collision
markers plus deterministic upward-probe adjustment, and modes `97/98` route
P1's selected original `mpCommonProcFighterCliffFloorCeil` map callback
through bounded `mpProcessUpdateMain`, run the same ceiling-heavy
collision/adjust path, import original `ftcommonstopceil.c`, reach
`ftCommonStopCeilSetStatus`, and prove Fall/Air `26/20/1` becomes
StopCeil/Ground `66/57/0`, and modes `99/100` route that same selected map
callback through the source-order cliff-test path, import original
`ftcommoncliffcatchwait.c`, reach `ftCommonCliffCatchSetStatus`, and prove
Fall/Air `26/20/1` becomes CliffCatch/Air `84/72/1` on real Pupupu line `3`,
then run a bounded same-cliff/same-LR occupancy probe that seeds P0 as the
line `3` holder and proves P1 is blocked before a second status setup. Modes
`101/102` start from that CliffCatch proof, call original
`ftCommonCliffCatchProcUpdate`, reach `ftAnimEndCheckSetStatus` and original
`ftCommonCliffWaitSetStatus`, prove CliffWait/Ground `85/73/0`, then run one
guarded original `ftCommonCliffWaitProcInterrupt` tick that decrements
fall-wait `1080 -> 1079` while attack/escape/climb-or-fall checks stay
guarded false, and modes `103/104` then import original
`ftcommoncliffattack.c`, `ftcommoncliffclimb.c`, and
`ftcommoncliffescape.c`, inject an A-button tap from the CliffWait state,
call original `ftCommonCliffAttackCheckInterruptCommon`, and prove
CliffQuick/Ground `86/74/0` with queued AttackQuick metadata on retained
Pupupu `cliff_id=3`, while modes `105/106` call original
`ftCommonCliffQuickProcUpdate`, reach CliffAttackQuick1/Ground `92/80/0`,
call original `ftCommonCliffAttackQuick1ProcUpdate`, reach the guarded
original `ftAnimEndCheckSetStatus` branch into
`ftCommonCliffAttackQuick2SetStatus`, and prove CliffAttackQuick2/Ground
`93/81/0`. The imported common2 collision-data helper is reached through a
project-owned temporary `MPCollData` bridge, and the wrapper copies the
resulting ledge floor fields back into the live `FTCollisionData` shell, so
the action proof records `floor_line_id=3` with the retained `cliff_id=3`.
The bridge now rejects invalid queued cliff-motion status IDs before indexing
BattleShip's five-entry `cliff_status_ga` table and verifies the root snap
against the expected Pupupu ledge edge for CliffClimbQuick2,
CliffAttackQuick2, and CliffEscapeQuick2 action handoffs. Modes `107/108`
then consume that created Quick2 state through one bounded
original `ftCommonCliffCommon2ProcUpdate`,
`ftCommonCliffCommon2ProcPhysics`, and
`ftCommonCliffAttackEscape2ProcMap` tick. The common2 proof keeps
CliffAttackQuick2/Ground `93/81/0`, retained `cliff_id=3`, and copied
`floor_line_id=3`, reaches one guarded anim-end check without falling back to
Wait/Fall, one guarded ground velocity transfer, and one guarded edge-break map
callback, then parks before continuous CliffAttack runtime.
Modes `109/110` then reuse the CliffWait state, inject a Z-button tap, prove
the original attack check is reached and skipped, prove original escape check
selection, and advance through original CliffQuick -> CliffEscapeQuick1 ->
CliffEscapeQuick2 action setup. The proof records CliffWait/Ground
`85/73/0 ->` CliffQuick/Ground `86/74/0 ->` CliffEscapeQuick1/Ground
`96/84/0 ->` CliffEscapeQuick2/Ground `97/85/0`, retained `cliff_id=3`,
copied `floor_line_id=3`, one guarded Quick update, one Quick1 update, one
Quick2 setup, Z tap distinct from A tap, and no damage-fall or unsafe fallback.
Modes `111/112` then consume that created CliffEscapeQuick2/Ground state
through one bounded original `ftCommonCliffCommon2ProcUpdate`,
`ftCommonCliffCommon2ProcPhysics`, and
`ftCommonCliffAttackEscape2ProcMap` tick. The proof keeps
CliffEscapeQuick2/Ground `97/85/0`, retained `cliff_id=3`, and copied
`floor_line_id=3`, reaches one guarded anim-end check without falling back to
Wait/Fall, one guarded ground velocity transfer, and one guarded edge-break map
callback, then parks before continuous CliffEscape runtime.
Modes `113/114` then return to the verified CliffWait/Ground state on real
Pupupu `cliff_id=3`, call original `ftCommonCliffWaitProcInterrupt`, prove the
guarded attack and escape checks are reached and rejected for the seeded
inputs, then route through original
`ftCommonCliffClimbOrFallCheckInterruptCommon`. The climb branch reaches
CliffQuick/Ground `86/74/0` with queued climb metadata and retained
`cliff_id=3`; the drop branch reaches Fall/Air `26/20/1` with
`cliffcatch_wait=30`, retained cliff metadata, and no damage-fall or unsafe
fallback, then proves a bounded same-ledge recatch by a second selected fighter:
the dropped former holder has `is_cliff_hold=0`, the recatcher enters
CliffCatch/Air `84/72/1` on the same Pupupu right ledge, and the occupancy
block count remains zero. It then parks before continuous ledge climb/drop
action runtime.
Modes `115/116` then consume the climb-created CliffQuick/Ground state, call
original `ftCommonCliffQuickProcUpdate`, reach original
`ftCommonCliffClimbQuick1SetStatus`, run original
`ftCommonCliffClimbQuick1ProcUpdate`, and reach the guarded anim-end path for
`ftCommonCliffClimbQuick2SetStatus`. The proof records
CliffQuick/Ground `86/74/0 ->` CliffClimbQuick1/Ground `87/75/0 ->`
CliffClimbQuick2/Ground `88/76/0`, retained `cliff_id=3`, copied
`floor_line_id=3`, one guarded common2 collision-data update, one guarded
common2 init-vars call, and zero unsafe fallback. It parks before continuous
ledge climb runtime.
Modes `117/118` then consume that created CliffClimbQuick2/Ground state through
one bounded original `ftCommonCliffCommon2ProcUpdate`,
`ftCommonCliffCommon2ProcPhysics`, and `ftCommonCliffClimbCommon2ProcMap` tick.
The proof keeps CliffClimbQuick2/Ground `88/76/0`, retained `cliff_id=3`, and
copied `floor_line_id=3`, reaches one guarded anim-end check without falling
back to Wait/Fall, one guarded ground velocity transfer, and one guarded
ground-break map callback, then parks before continuous ledge climb runtime.
Modes `119/120` then consume the same created CliffClimbQuick2/Ground state
through the bounded original common2 animation-end handoff. The proof reaches
`mpCommonSetFighterWaitOrFall`, calls the bounded Wait `ftMainSetStatus` path,
and records CliffClimbQuick2/Ground `88/76/0 ->` Wait/Ground `10/4/0`,
retained `cliff_id=3`, stable `floor_line_id=3`, `playertag_wait=120`, and a
restored special interrupt. The bounded status seam now applies the current
project-owned original-compatible common reset, and the finish diagnostics
require reset mask `0x7ffff` after the Wait handoff: reflect, absorb, shield,
fastfall, invisible, shadow hide, player-tag hide, ghost, hitstun, cliff hold,
jostle ignore, damage player, ignored line, capture immune mask, camera zoom
range, shuffle tics, knockback resistance, stacked knockback, and damage
multiplier reset. The climb
drop, Quick2, common2, and finish diagnostics require stale hold/jostle state
to be clear after status changes. The `ftCommonCliffCommon2UpdateCollData`
fallback now routes through the same DS common2 bridge as the guarded action
branches, keeping the queued status-ID range guard and root snap diagnostics on
every current entry point before calling the imported base helper. The common2
root/status guard and same-cliff occupancy blocker are still bounded harness
coverage; broader natural ledge occupancy/release remains deferred.
The inherited moving/floor slice still proves both fighters reach
Wait/Ground/Floor on decoded real floor line ID `3` inside floor range `[0,4)`
before the later cliff-status/ceiling/cliff-catch finalizer probes, with
nonzero P0/P1 `pos_diff.x`, 39
update-main steps, split-step standalone probe evidence, floor callback hits,
positive signed below-floor distance evidence, inside hit and outside miss
probes, local
`mpProcessSetLandingFloor` / `mpProcessSetCollideFloor` probe calls, old
second-floor deferred counts held at zero, standalone same-line reject and
different-line accept evidence, a live P0 `-1 -> 3` second-floor acceptance
through `mpProcessCheckTestFloorCollision`, 17 live floor-edge-adjust calls,
17/17 left/right floor-edge checks, 17/17 wall-line sweep misses in the
MPAdjust proof, 18/18 left/right edge-under calls in the MP edge-floor proof,
resolved adjacent wall lines `6/5` with wall kinds `3/2`, MPAdjust edge-under
deferred count `0` in modes `77/78`, two MP wall-blocker candidates, zero
wall hits, nonzero wall-blocker miss evidence, old floor-edge-adjust deferred
count `0`, the valid-stale local probe source/target `1 -> 0` at `x=-285`,
`y=1542`, one accepted stale-floor second-floor result, landing-floor,
floor-edge-adjust, collision-end clear evidence, zero final recenter/adopt
counts, one selected live-stale callback proof, isolated probe/search
edge-under diagnostics, one selected motion-stale P0 mutation/copyback proof,
zero post-clamp drift, and stable safety counters. The cliff-status finalizer
then records two source-order `mpCommonProcFighterOnCliffEdge` probes, one
Ottotto branch, one Fall branch, two accepted status sets, one air-state set,
P0 final Ottotto `36/30` with a nonzero floor-edge mask, P1 final Fall
`26/20` with no floor-edge mask, and zero unsafe count. The cliff-tick
finalizer records Ottotto update/interrupt/map `1/1/1`, Fall interrupt `1`,
air checks `1/1/1`, no guarded status-set fallback, no OttottoWait fallback,
and zero unsafe count. The Fall-map finalizer records the Fall physics
callback, fast-fall check, gravity, air drift, air friction, one bounded
integration, one Fall map callback, one no-collision map result, root-Y
`200000 -> 194000`, and zero unsafe count. The Fall-landing finalizer records
the Fall physics callback, one bounded integration that crosses the decoded
floor, one map-floor collision, one landing-floor setup call, one LandingLight
status-set path, Ground conversion, root-Y clamp `4000 -> 0`, velocity-Y clamp
`-8000 -> 0`, and zero unsafe count. The ceiling-floor finalizer records
selected ceiling line `4`, one bounded ceiling test hit, one different-line
ceiling sweep hit, two `mpCollisionGetFCCommonCeil` hits, one adjust call,
current/stat ceiling mask evidence, root-Y adjustment
`-1464000 -> -1472000`, ceiling Y `-1072000`, and zero unsafe count. The
ceiling-status finalizer records the selected original map callback, one
ceiling-heavy check, one special-collision callback, one ceiling hit, one
adjust call, one `MAP_FLAG_CEILHEAVY` marker, one
`ftCommonStopCeilSetStatus` call, one guarded `ftMainSetStatus` call, one
animation-event callback, Fall/Air `26/20/1 -> 66/57/0`, vertical velocity
`40000 -> 0`, mask evidence `0x4400/0x400`, and zero unsafe count. The
cliff-catch finalizer records two selected map callbacks, two ceiling-heavy
checks, two special-collision callbacks, two left/right cliff-test passes, one
normal right-cliff hit, one same-cliff occupancy block, one landing-param
setup, one `ftCommonCliffCatchSetStatus` call, guarded status/animation
callbacks, stop-velocity and physics-stop calls, flash and capture-immune side
effects, Fall/Air `26/20/1 -> 84/72/1`, cliff hold `1`, copied `cliff_id=3`,
root placement on ledge X `2318000`, right-cliff mask evidence
`0x2000/0x2000`, `occ=1/0`, and zero unsafe count.
The CliffWait finalizer records the base CliffCatch proof, one original
CliffCatch update, one anim-end status check/set, one
`ftCommonCliffWaitSetStatus`, one guarded `ftMainSetStatus`, one player-tag
wait setup, one capture-immune setup, one CliffWait interrupt, guarded
attack/escape/climb-or-fall checks, CliffCatch/Air `84/72/1 ->`
CliffWait/Ground `85/73/0`, retained `cliff_id=3`, LR `-1`, nonzero
capture mask, proc-damage setup, fall-wait `1080 -> 1079`, no damage-fall
call, and zero unsafe count.
The older modes `61/62` remain the final-sample floor projection proofs and
still re-center the proof-owned roots from decoded Pupupu floor endpoints
before their collision sample.

The current Title boundary loads original `MNTitle` and `MNTitleFireAnim`,
creates the original actor/logo-fire/fire/camera/vars boundaries, normalizes
the 30 original Title fire-frame Sprites, runs one guarded original Title update
tick on the natural movie path, and renders a bounded original Title sprite
preview.

The `vs_setup` harness now enters `nSCKindVSMode` from `nSCKindTitle` and
runs imported `mnvsmode.c` setup far enough to load original `MNCommon` and
`MNVSMode`, create the original main GObj, default camera, viewports,
background, menu-name, VS Start, Rule, Time/Stock, VS Options, value, arrow,
and subtitle SObj graph, then parks at the taskman loop boundary.
`NDS_DEV_SCENE_HARNESS=vs_start_transition` starts at the same boundary, runs
the setup proof, advances original `mnVSModeMain` through the input-ready gate,
injects a synthetic A tap on VS Start, proves original `mnVSModeSaveSettings`,
observes `scene_prev = nSCKindVSMode` and `scene_curr = nSCKindPlayersVS`,
observes the original load-scene request, and then reaches the bounded imported
PlayersVS setup boundary.
`NDS_DEV_SCENE_HARNESS=players_setup` starts directly at
`nSCKindPlayersVS` from `nSCKindVSMode`, imports `mnplayersvs.c` through a
DS-owned wrapper, loads the seven original PlayersVS menu files, creates the
original main GObj/default camera/camera set/menu object graph, initializes
original PlayersVS vars and slot state, and parks before continuous
character-select input/drawing.
`NDS_DEV_SCENE_HARNESS=maps_setup` starts directly at `nSCKindMaps` from
`nSCKindPlayersVS`, imports `mnmaps.c` through a DS-owned wrapper, loads the
five original Maps files, creates the original stage-select SObj graph, starts
from seeded Pupupu/Dream Land, loads `GRPupupuMap` plus `StageDreamLand`,
`ExternDataBank104`, `ExternDataBank103`, and `MiscDataBank152`, applies
external relocation fixups, runs the bounded original Pupupu preview path, and
no longer marks Pupupu preview as deferred.
`NDS_DEV_SCENE_HARNESS=menu_chain_vsbattle` starts at VS Mode, proves original
VS Start -> PlayersVS, injects a deterministic two-player PlayersVS ready/start
state, proves original PlayersVS -> Maps, injects a synthetic Maps A-select on
Pupupu, proves original Maps scene-data saving, and parks at
`scene_curr/scene_prev = 22/21` after imported bounded VSBattle setup with real
Pupupu `MPGroundData` adopted and the bounded original Pupupu ground object
graph created.
`NDS_DEV_SCENE_HARNESS=battle_fd` starts directly at `nSCKindVSBattle` from
`nSCKindMaps`, seeds one Mario using stock rules and `nGRKindLast` as the
current Final Destination sentinel, imports `scvsbattle.c` /
`scvsbattlefiles.c`, runs original `scVSBattleStartBattle` through common
battle file loading, default camera creation, manager/interface/audio
compatibility stubs, active fighter descriptor construction, stub fighter GObj
creation, and one bounded `scVSBattleFuncUpdate` interface tick.
`NDS_DEV_SCENE_HARNESS=battle_pupupu_stage` starts directly at
`nSCKindVSBattle` from `nSCKindMaps`, seeds two human players on Pupupu/Dream
Land, resolves real `MPGroundData` from `GRPupupuMap`, records geometry,
map-node, light-angle, and BGM metadata, enters the Pupupu-only imported
original ground setup path, runs original `grDisplayMakeGeometryLayer`,
`grMainSetupMakeGround`, `grPupupuMakeGround`, and `grPupupuInitAll`, creates
four display-layer GObjs plus the original Whispy eyes, Whispy mouth, back
flowers, and front flowers map GObjs, records DObj/MObj/animation diagnostics,
creates two stub fighter GObjs from original descriptors, and parks before
continuous stage update/draw, Whispy wind, yakumono runtime, item/effect
runtime, full collision/stage runtime, or gameplay.
`NDS_DEV_SCENE_HARNESS=battle_pupupu_update` starts from the same direct
Pupupu VSBattle setup, then calls original `grPupupuProcUpdate` twice in a
deterministic safe substate. It proves Whispy Sleep -> Wait and one Wait
countdown tick, keeps flowers in their default state, preserves map GObjs and
object counts, records zero wind/push/quake/particle side effects, and parks at
the VSBattle boundary before continuous stage runtime.
`NDS_DEV_SCENE_HARNESS=menu_chain_pupupu_update` proves the same bounded
Pupupu update after VS Mode -> PlayersVS -> Maps -> VSBattle.
`NDS_DEV_SCENE_HARNESS=battle_mariofox_model` starts directly at the Pupupu
VSBattle boundary, loads `FTManagerCommon`, Mario, Fox, and required external
fighter O2R dependencies, applies the internal/external relocation path, and
creates real asset-backed Mario/Fox fighter GObjs with top/model/commonpart
DObj trees instead of the setup-only stub fighter GObjs.
`NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_model` proves the same Mario/Fox
model boundary after the guarded VS Mode -> PlayersVS -> Maps -> VSBattle
chain.
`NDS_DEV_SCENE_HARNESS=battle_mariofox_struct` starts directly at the same
Pupupu VSBattle boundary and extends the model proof by allocating persistent
`FTStruct` shells for Mario and Fox from a bounded project-owned pool. Each
shell is stored in `fighter_gobj->user_data.p`, is returned by `ftGetStruct`,
records original `FTDesc` identity, `FTAttributes` and figatree pointers,
input masks, collision pointer contracts, and a deterministic top/common joint
table from the real DObj tree.
`NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_struct` proves the same persistent
FTStruct-backed fighter state after the guarded VS Mode -> PlayersVS -> Maps ->
VSBattle chain.
`NDS_DEV_SCENE_HARNESS=battle_mariofox_init` starts directly at the same
Pupupu VSBattle boundary and runs a bounded project-owned helper in original
`ftManagerInitFighter` source order for P0 Mario and P1 Fox. It preserves the
model and persistent `FTStruct` proofs, initializes damage, shield, velocity,
attack/damage counters, hitstatus, throw/catch/item pointers, passive state,
root DObj position/scale, `FTAttributes` map/cliff collision contracts, and a
deterministic Pupupu floor projection. It also proves bounded
physics/attack-collision/hitstatus/colanim compatibility calls while keeping
fighter status/process/display/gameplay parked.
`NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_init` proves the same init-state
boundary after the guarded VS Mode -> PlayersVS -> Maps -> VSBattle chain.
`NDS_DEV_SCENE_HARNESS=battle_mariofox_wait` imports original
`ftcommonwait.c` and runs original `ftCommonWaitSetStatus` for initialized P0
Mario and P1 Fox after the direct Pupupu VSBattle setup. The project-owned
`ftMainSetStatus` seam is Wait-only for this harness, records status `10`,
motion `4`, animation frame `0`, speed `1.0`, special interrupt `TRUE`, player
tag wait `120`, and the Wait interrupt/physics/map callback pointers, but does
not execute those callbacks or any fighter process/gameplay loop.
`NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_wait` proves the same original Wait
status/motion setup after the guarded VS Mode -> PlayersVS -> Maps ->
VSBattle chain.
`NDS_DEV_SCENE_HARNESS=battle_mariofox_wait_tick` starts from the same direct
Wait boundary, runs one bounded original `ftCommonWaitProcInterrupt` tick for
Mario and Fox with neutral input, then calls the guarded project-owned physics
and map callback seams once per fighter. It proves status, motion,
ground/air state, root position, ground velocity, and GObj count remain stable
without starting a fighter process loop.
`NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_wait_tick` proves the same bounded
Wait callback tick after the guarded VS Mode -> PlayersVS -> Maps -> VSBattle
chain.
`NDS_DEV_SCENE_HARNESS=battle_mariofox_wait_ground` starts from the same
direct Wait tick boundary, seeds ground velocity `2.0` for Mario and Fox, runs
a bounded source-order ground-friction/air-transfer helper mirroring the
original `ftphysics.c` path, and runs the safe floor branch of the
`mpCommonProcFighterOnCliffEdge` map seam. It proves friction/map callbacks
ran twice, ground velocity decreased, status/motion/ground-air/root/GObj state
stayed stable, and no Fall/Ottotto/process/display/gameplay escape occurred.
`NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_wait_ground` proves the same bounded
ground-friction/map pass after the guarded VS Mode -> PlayersVS -> Maps ->
VSBattle chain. The older Wait tick harnesses intentionally keep their no-op
physics/map seam behavior.
`NDS_DEV_SCENE_HARNESS=battle_mariofox_display_probe` starts from the same
direct Wait ground boundary and calls the project-owned
`ftDisplayMainProcDisplay` seam once for Mario and once for Fox under a
DS-owned guard. It records metadata from the real fighter DObj trees without
rendering: Mario/Fox DObj counts `25/27`, MObj counts `0/0`, AObj counts
`0/0`, display-list candidate counts `14/18`, stable status `10`, motion `4`,
GA `0`, root X positions, GObj count, and zero draw/matrix/gameplay counters.
`NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_display_probe` proves the same
metadata-only display boundary after the guarded VS Mode -> PlayersVS -> Maps
-> VSBattle chain, including the same DObj/display-list candidate counts and
the same no-draw/no-matrix/no-gameplay/no-root/no-GObj-escape contract.
`NDS_DEV_SCENE_HARNESS=battle_mariofox_dl_scan` starts from the same direct
display metadata boundary, selects the first display-list-bearing DObj for
Mario and Fox, and scans each selected original display list with
`ndsRendererScanDisplayList()` only. The selected DLs are taskman-arena copied
original fighter data, reported with ownership sentinel `0xfffffffe`, not
registered O2R file pointers. Current direct proof records command counts
`59/69`, renderer blocker `0/0`, zero unsupported opcodes/commands, and
nonzero vertex/triangle command families while preserving status `10`, motion
`4`, GA `0`, root X, GObj count, and zero draw/matrix/gameplay counters.
`NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_dl_scan` proves the same parser-only
DL scan after the full bounded VS Mode -> PlayersVS -> Maps -> VSBattle chain.
`NDS_DEV_SCENE_HARNESS=battle_mariofox_dl_execute` starts from the same direct
DL scan boundary and runs `ndsRendererExecuteDisplayList()` once for the
selected Mario DL and once for the selected Fox DL. It decodes real `G_VTX`,
`G_TRI1`, and `G_TRI2` payloads, records command-family/bounds/color
diagnostics, and proves `59/69` commands, `28/23` decoded vertices, `37/20`
triangles, zero unsupported commands, and no status/motion/root/GObj/draw/
matrix/gameplay escape.
`NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_dl_execute` proves the same
execute/decode boundary after the full bounded VS Mode -> PlayersVS -> Maps ->
VSBattle chain.
`NDS_DEV_SCENE_HARNESS=battle_mariofox_dl_draw` starts from the same direct DL
execute/decode boundary and reuses the selected real Mario/Fox first display
lists for a bounded software draw preview. It decodes each DL through
`ndsRendererExecuteDisplayList()`, projects the collected triangles into
deterministic side-by-side boxes in the existing `96x72` original-DL preview
surface, commits that retained preview once, and proves nonzero pixels for both
fighters while preserving status `10`, motion `4`, GA `0`, root X, GObj count,
and zero draw/matrix/gameplay escape counters.
`NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_dl_draw` proves the same visible
first-DL software preview after the full bounded VS Mode -> PlayersVS -> Maps
-> VSBattle chain.
`NDS_DEV_SCENE_HARNESS=battle_mariofox_dl_draw_multi` starts from the same
direct first-DL draw boundary, censuses all DL-ready Mario/Fox fighter DObjs
(`14/18`), selects the first four DL-ready DObjs per fighter in deterministic
depth-first order, executes/decodes all eight selected original display lists
through `ndsRendererExecuteDisplayList()`, and draws them into the same
retained `96x72` original-DL preview surface with one shared projection
axis/bounds set per fighter. It proves `6190/7026` pixels, `87/79` triangles
after corrected F3DEX2 command decoding, `82/76` real drawn triangles, `1/3`
bounded marker triangles, four clean selected DObjs per fighter,
stable status `10`, motion `4`, GA `0`, root X and GObj count, and zero
draw/matrix/gameplay/range-reject escape.
`NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_dl_draw_multi` proves the same
visible multi-DL software preview after the full bounded VS Mode -> PlayersVS
-> Maps -> VSBattle chain.
`NDS_DEV_SCENE_HARNESS=battle_mariofox_dl_draw_all` starts from the same
direct multi-DL draw boundary, calls the current guarded
`ftDisplayMainProcDisplay` seam exactly once for Mario and once for Fox,
censuses all DL-ready fighter DObjs (`14/18`), selects all 32 current
display-list-bearing DObjs, executes them through
`ndsRendererExecuteDisplayList()`, and draws all selected DObjs into the
retained `96x72` original-DL software preview. Current proof records
`14913/13432` pixels, `334/322` represented triangles, `306/290` real drawn
triangles, and `10/24` bounded marker triangles for Mario/Fox. The F3DEX2
decoder now keeps all selected Mario/Fox DObjs clean (`clean=14/18`,
`failed=0/0`). `FTR_DL_ALL_FAIL` is now required to stay fully clear with
sentinel failed indices and no failure reason, renderer blocker, unsupported
opcode/command, or vertex-range reject. Stable status `10`, motion `4`, GA `0`,
root X, GObj count, and zero draw/matrix/gameplay/range-reject escape counters
remain required. Collapsed projected triangles use bounded software markers
until real fighter matrix/camera projection is imported, but marker triangles
are now counted separately and do not satisfy geometry proof bits.
`NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_dl_draw_all` proves the same guarded
all-DL draw boundary after the full bounded VS Mode -> PlayersVS -> Maps ->
VSBattle chain.
`NDS_DEV_SCENE_HARNESS=battle_mariofox_walk_input` starts from that same direct
Pupupu VSBattle boundary and extends the guarded all-DL proof with imported
original `ftcommonwalk.c`. It seeds deterministic forward stick input, enters
through original `ftCommonWaitProcInterrupt`, routes the guarded ground-check
seam into original `ftCommonWalkCheckInterruptCommon`, proves Mario selects
WalkMiddle (`status 12`, `motion 6`) and Fox selects WalkFast (`status 13`,
`motion 7`), then runs one bounded Walk interrupt callback, one source-order
Walk ground-velocity physics pass, and one safe floor-map pass. Root position,
GA, GObj count, and draw/matrix/display/gameplay escape counters stay stable.
`NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_walk_input` proves the same
original Wait -> Walk boundary after the full bounded VS Mode -> PlayersVS ->
Maps -> VSBattle chain.
`NDS_DEV_SCENE_HARNESS=battle_mariofox_walk_loop` starts from the same direct
Walk-input boundary and runs four bounded source-order Walk frames for Mario
and Fox with held forward stick input. It calls original Walk interrupt and
physics callbacks each frame, integrates `fp->physics.vel_air` into the
top/root DObj, runs the bounded safe floor-map seam, releases stick input to
neutral, proves original `ftCommonWalkProcInterrupt` returns both fighters to
Wait through `ftCommonWaitCheckInterruptCommon`, and runs one bounded Wait
friction/map settle frame. Current proof records root deltas
`48000/-144000`, velocity reduction `12000/36000 -> 6000/28000`, stable root
Y/GA/GObj state, and zero display/draw/matrix/gameplay/process/status escape.
`NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_walk_loop` proves the same movement
loop after the full bounded VS Mode -> PlayersVS -> Maps -> VSBattle chain.
`NDS_DEV_SCENE_HARNESS=battle_mariofox_dash_run` starts from the same direct
Walk-loop boundary, seeds deterministic dash input, enters original Dash from
Wait, crosses the bounded Dash -> Run threshold through original Dash
interrupt logic, runs held Run frames, releases to neutral, proves original
Run -> RunBrake, and parks after bounded RunBrake physics/map ticks.
`NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_dash_run` proves the same
Dash -> Run -> RunBrake movement path after the full bounded VS Mode ->
PlayersVS -> Maps -> VSBattle chain.
`NDS_DEV_SCENE_HARNESS=battle_mariofox_jump_loop` starts from the same direct
Dash/Run proof endpoint, closes RunBrake back to Wait through the guarded
original-compatible `ftAnimEndSetWait` path, enters original KneeBend through
`ftCommonWaitProcInterrupt`, advances bounded KneeBend updates until original
`ftCommonJumpSetStatus`, then proves JumpF airborne movement for six bounded
frames. Current proof records JumpF status/motion `22/16`, air state `1/1`,
root movement `100800/-138900` X and `395400/468000` Y milli-units, and Y
velocity decay `74300/92000 -> 59900/68000` while draw, matrix, display,
process, gameplay, denied-status, unexpected-status, landing-status, and
fall-status counters remain zero.
`NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_jump_loop` proves the same
RunBrake -> Wait -> KneeBend -> JumpF boundary after the full bounded VS Mode
-> PlayersVS -> Maps -> VSBattle chain.
`NDS_DEV_SCENE_HARNESS=battle_mariofox_scheduler_loop` starts from the same
direct VSBattle Mario/Fox process-loop boundary, attaches one selected
Mario/Fox `GObjProcess` callback per fighter through original
`gcAddGObjProcess`, invokes those callbacks with `gcRunGObjProcess`, routes the
proof through the wrapped `scVSBattleFuncUpdate`, and runs a capped VSBattle
taskman update loop until both fighters complete the proven Walk, Dash/Run/
RunBrake, and Jump/Fall/Landing paths.
`NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_scheduler_loop` proves the same
scheduler-facing boundary after the full bounded VS Mode -> PlayersVS -> Maps
-> VSBattle chain.
`NDS_DEV_SCENE_HARNESS=battle_mariofox_controller_loop` starts from the same
direct VSBattle scheduler-loop endpoint, enables deterministic controller
playback, feeds `OSContPad` data through `osContGetReadData`, runs original
`syControllerReadDeviceData` and `syControllerUpdateGlobalData`, bridges
`gSYControllerDevices` to `FTStruct` input through DS-owned code, and proves
the same Mario/Fox movement contract through original `gcRunGObjProcess`.
`NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_controller_loop` proves the same
controller-source boundary after the full bounded VS Mode -> PlayersVS -> Maps
-> VSBattle chain.

This is not full Title/VS/menu import. Full Title input, animated logo,
labels/Press Start, slash, logo-fire particles, audio, continuous title draw,
full VS Mode navigation/rule editing/options transition, continuous VS menu
drawing, full interactive PlayersVS cursor/puck selection, Maps preview model
rendering fidelity, full fighter/stage logic, full collision line processing,
Whispy wind/fighter push, yakumono/stage object runtime, items/weapons runtime,
interface rendering, audio backend, full fighter process/status/gameplay, real
fighter display rendering beyond the bounded software preview,
camera-correct battle projection, material/texture upload, exact original
fighter joint-ID mapping,
fighter/stage-heavy action scene internals, and gameplay remain deferred.

A project-owned NDS dev/test scene harness is now available for faster
boundary iteration. Default builds are unchanged. `NDS_DEV_SCENE_HARNESS=title`
mutates only the original-compatible `dSCManagerDefaultSceneData` before the
imported `scManagerRunLoop` copies it, entering `nSCKindTitle` from
`nSCKindOpeningNewcomers` and then running the same bounded imported
`mnTitleStartScene` path. `NDS_DEV_SCENE_HARNESS=vs_setup` enters the bounded
imported `mnVSModeStartScene` setup proof from Title.
`NDS_DEV_SCENE_HARNESS=vs_start_transition` enters the same VS setup proof and
then runs the bounded original VS Start transition probe.
`NDS_DEV_SCENE_HARNESS=players_setup` enters bounded imported PlayersVS setup.
`NDS_DEV_SCENE_HARNESS=maps_setup` enters bounded imported Maps setup.
`NDS_DEV_SCENE_HARNESS=menu_chain_vsbattle` proves the VS Mode -> PlayersVS ->
Maps -> imported bounded VSBattle setup chain.
`NDS_DEV_SCENE_HARNESS=battle_fd` enters the same imported bounded VSBattle
setup directly.
`NDS_DEV_SCENE_HARNESS=battle_pupupu_stage` enters the same imported bounded
VSBattle setup directly with two players and real Pupupu stage-data adoption.
`NDS_DEV_SCENE_HARNESS=battle_pupupu_update` and
`NDS_DEV_SCENE_HARNESS=menu_chain_pupupu_update` add the guarded two-tick
original Pupupu update proof on top of the direct and menu-chain setup paths.
`NDS_DEV_SCENE_HARNESS=battle_mariofox_model` and
`NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_model` replace the setup-only stub
fighter GObjs with bounded asset-backed Mario/Fox model GObjs on the direct and
menu-chain VSBattle paths. `NDS_DEV_SCENE_HARNESS=battle_mariofox_struct` and
`NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_struct` attach the persistent
project-owned `FTStruct` shell and joint-table proof to those same direct and
menu-chain paths. `NDS_DEV_SCENE_HARNESS=battle_mariofox_init` and
`NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_init` add the bounded
source-order fighter init-state proof on top of those struct-backed paths.
`NDS_DEV_SCENE_HARNESS=battle_mariofox_wait` and
`NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_wait` add the bounded original
Mario/Fox Wait status and motion setup proof on top of those initialized
fighter paths. `NDS_DEV_SCENE_HARNESS=battle_mariofox_wait_tick` and
`NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_wait_tick` add one bounded original
Wait interrupt callback tick plus guarded physics/map seam calls on top of
those Wait-state fighter paths. `NDS_DEV_SCENE_HARNESS=battle_mariofox_wait_ground`
and `NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_wait_ground` add the bounded
source-order ground-friction/air-transfer and safe floor-map proof without
starting fighter process/gameplay loops.
`NDS_DEV_SCENE_HARNESS=battle_mariofox_display_probe` adds the direct metadata
display callback probe without rendering fighters.
`NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_display_probe` adds the same guarded
display metadata proof after the full bounded menu chain.
`NDS_DEV_SCENE_HARNESS=battle_mariofox_dl_scan` and
`NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_dl_scan` add parser-only fighter
display-list scans after the direct and menu-chain display metadata proofs.
`NDS_DEV_SCENE_HARNESS=battle_mariofox_dl_execute` and
`NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_dl_execute` add decode-only
execution of the same selected Mario/Fox display lists.
`NDS_DEV_SCENE_HARNESS=battle_mariofox_dl_draw` and
`NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_dl_draw` add bounded software
preview drawing of those same selected first DLs. They prove visible retained
preview pixels for both fighters, but still do not implement full fighter
rendering.
`NDS_DEV_SCENE_HARNESS=battle_mariofox_dash_run` and
`NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_dash_run` add the bounded original
Dash -> Run -> RunBrake movement proof on top of the Walk-loop boundary.
`NDS_DEV_SCENE_HARNESS=battle_mariofox_jump_loop` and
`NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_jump_loop` add the bounded original
RunBrake -> Wait -> KneeBend -> JumpF airborne movement proof on top of the
Dash/Run boundary. Those two standalone Jump-loop harnesses now also seed one
guarded neutral-air input after the six bounded JumpF air frames and prove
original
`ftCommonAttackAirCheckInterruptCommon` reaches AttackAirN/Air
status/motion `209/184` through the bounded `ftMainSetStatus` seam without
starting continuous aerial attack runtime. They also run one bounded original
Link AttackAirLw rehit tick and prove the project-owned
`ftParamRefreshAttackCollID` seam refreshes attack coll IDs `0/1` as new and
active, then clears their original four-slot attack records. They now also
tick the installed original AttackAir map callback into the original landing
branches and prove `map=0x3ff` for smooth LandingAirN, missing-animation
LandingAirNull, skip-landing Wait, and plain LandingLight handoffs. They now
also replay the original directional selector for AttackAirF/B/Hi/Lw and prove
`dir=0x1f` before restoring the proof-local state. Downstream
landing/process/current boundary harnesses still consume the non-mutating
JumpF handoff and only require the older Jump-loop `0x7ff` prerequisite mask.
`NDS_DEV_SCENE_HARNESS=battle_mariofox_landing_loop` and
`NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_landing_loop` add the bounded
original JumpF -> Fall -> LandingLight -> Wait proof on top of the Jump-loop
boundary. They import `ftcommonfall.c` and `ftcommonlanding.c`, enter Fall
through guarded `ftAnimEndSetFall`, run bounded Fall physics/map frames,
detect Pupupu floor crossing, call original `ftCommonLandingSetStatus`, clamp
root Y to the recorded floor, close LandingLight through `ftAnimEndSetWait`,
and park at VSBattle before continuous fighter scheduling.

## Latest Proof

Known-good PowerShell environment:

```powershell
$env:DEVKITPRO = 'C:/devkitPro'
$env:DEVKITARM = 'C:/devkitPro/devkitARM'
```

Latest maintained regression chain:

```powershell
make -j16
.\scripts\check-gbi-decode-fixtures.ps1
.\scripts\verify-runtime.ps1
.\scripts\verify-opening-skip.ps1
.\scripts\verify-title-boundary.ps1
.\scripts\verify-regression.ps1
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
.\scripts\verify-battle-mariofox-live-preview-harness.ps1
.\scripts\verify-battle-mariofox-gcdrawall-loop-harness.ps1
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
.\scripts\verify-menu-chain-mariofox-live-preview-harness.ps1
.\scripts\verify-menu-chain-mariofox-gcdrawall-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-gcdrawall-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-collision-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mplivestale-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mplivestale-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpmotionstale-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpmotionstale-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpcliffstatus-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpcliffstatus-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpclifftick-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpclifftick-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpfallmap-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpfallmap-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpfallland-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpfallland-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpceil-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpceil-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpceilstatus-floor-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpceilstatus-floor-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpcliffclimb-action-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpcliffclimb-action-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpcliffclimb-common2-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpcliffclimb-common2-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpcliffclimb-finish-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpcliffclimb-finish-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpcliffwait-damage-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpcliffwait-damage-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mppassive-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mppassive-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-mpdownwait-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-mpdownwait-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-turn-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-turn-loop-harness.ps1
```

Latest post-maintenance results:

```text
verify-runtime.ps1 -> Runtime verification passed (401 frames, fps=60/up=0/dl=60, cv=0/ch=32, verifyfps=2.93)
verify-opening-boundary.ps1 -> frames=504 hostfps=54.30 room=420
verify-opening-skip.ps1 -> Opening Room skip verification passed (tick 10 -> Title)
verify-title-boundary.ps1 -> frames=3292 hostfps=40.52 room=1320 action=9/324 title=0x54494457
verify-title-harness.ps1 -> Title harness passed: scene=1/46 room=0 title=0x54494457
verify-vs-setup-harness.ps1 -> VS setup harness passed: scene=9/1 setup=0x1f files=2 sobj=28 buttons=0x3f
verify-vs-start-transition-harness.ps1 -> VS Start transition harness passed: scene=16/9 trans=0x56535452 mask=0xff saved=1/3/2
verify-players-vs-setup-harness.ps1 -> PlayersVS setup harness passed: files=7 mask=0xff sobj=65 slots=2/4/4
verify-maps-setup-harness.ps1 -> Maps setup harness passed: scene=21/16 setup=0xff files=5 preview=0xff assets=0x1f slot=6 gkind=6
verify-battle-fd-harness.ps1 -> Battle FD harness passed: files=8 players=1/0 fighters=1 gkind=16 mask=0x7f
verify-battle-pupupu-stage-harness.ps1 -> Battle Pupupu stage harness passed: scene=22/21 gkind=6 stage=0xff ground=0x3ff layers=4 mapGObjs=4 players=2 fighters=2
verify-battle-pupupu-update-harness.ps1 -> Battle Pupupu update harness passed: scene=22/21 update=0xff ticks=2 whispy=0->1 windWait=2->1 sidefx=0/0/0/0
verify-battle-mariofox-model-harness.ps1 -> Battle Mario/Fox model harness passed: assets=0xffff, setup=0xfff, realGObjs=2, p0DObjs=25, p1DObjs=27
verify-battle-mariofox-struct-harness.ps1 -> Battle Mario/Fox struct harness passed: scene=22/21 struct=0xfff pool=0x3 p0Joints=24 p1Joints=26 model=0xfff
verify-battle-mariofox-init-harness.ps1 -> Battle Mario/Fox init harness passed: scene=22/21 init=0x3fff p0GA=0 p1GA=0 floor=1/1 calls=2/2/2/2
verify-battle-mariofox-wait-harness.ps1 -> Battle Mario/Fox Wait harness passed: waitMask=0xfff count=2 status=10/10 motion=4/4 callbacks=0
verify-battle-mariofox-wait-tick-harness.ps1 -> Battle Mario/Fox Wait tick harness passed: scene=22/21 tick=0x3ff callbacks=2/2/2 stable=1 final=22
verify-battle-mariofox-wait-ground-harness.ps1 -> Battle Mario/Fox Wait ground harness passed: scene=22/21 ground=0x7ff vel=2000->0/2000->0 map=2/2 stable=1
verify-battle-mariofox-display-probe-harness.ps1 -> Battle Mario/Fox display probe harness passed: scene=22/21 display=0x7ff dobj=25/27 mobj=0/0 ready=14/18 stable=1
verify-battle-mariofox-dl-scan-harness.ps1 -> Battle Mario/Fox DL scan harness passed: scene=22/21 dl=0x22dc548/0x2304f10 asset=4294967294/4294967294 commands=59/69 blocker=0/0 safe=1
verify-battle-mariofox-dl-execute-harness.ps1 -> Battle Mario/Fox DL execute harness passed: commands=59/69 verts=28/23 tris=37/20 safe=1
verify-battle-mariofox-dl-draw-harness.ps1 -> Battle Mario/Fox DL draw harness passed: scene=22/21 pixels=4274/5345 tris=37/20 real=37/18 marker=0/2 preview=96x72 commit=1 safe=1
verify-battle-mariofox-dl-draw-multi-harness.ps1 -> Battle Mario/Fox multi-DL draw harness passed: scene=22/21 candidates=14/18 selected=4/4 pixels=6190/7026 tris=87/79 real=82/76 marker=1/3 clean=4/4 preview=96x72 safe=1
verify-battle-mariofox-dl-draw-all-harness.ps1 -> Battle Mario/Fox all-DL draw harness passed: scene=22/21 callbacks=2 candidates=14/18 selected=14/18 pixels=14913/13432 tris=334/322 real=306/290 marker=10/24 clean=14/18 failed=0/0 preview=96x72 safe=1
verify-battle-mariofox-walk-input-harness.ps1 -> Battle Mario/Fox Walk input harness passed: scene=22/21 status=12/13 motion=6/7 stick=40/80 vel=12000/36000 callbacks=2 safe=1
verify-battle-mariofox-walk-loop-harness.ps1 -> Battle Mario/Fox Walk loop harness passed: scene=22/21 frames=4/4 root-dx=48000/-144000 release=Wait vel=12000/36000->6000/28000 safe=1
verify-battle-mariofox-dash-run-harness.ps1 -> Battle Mario/Fox Dash-Run harness passed: wait->attack11=190/165 cb=0xff tick=0xff waitproc=0x3; guard=152/134 cb=0xff state=0xfffffe0f fgm=13; escape=156/136 cb=0x3ff state=0xff; attack11->attack12=191/166 cb=0xff goto=0xf; attack12->attack13=220/195 foxblock=191/166 cb=0xf goto=0x7; fox attack100start=220/195 cb=0xf goto=0x3; fox attack100loop=221/196 cb=0xf goto=0x3 tick=0xfff; anim=0x3f; dash->run->runbrake + attackdash=192/167 cb=0xff tick=0x3f runproc=0x3 root-dx=301575/-418500 vel=51200/71000->47450/66000
verify-battle-mariofox-jump-loop-harness.ps1 -> Battle Mario/Fox Jump-loop harness passed: jump dx=100800/138900 dy=395400/468000 vy=74300/92000->59900/68000 attackAir=209/184 map=0x3ff dir=0x1f
verify-battle-mariofox-landing-loop-harness.ps1 -> Battle Mario/Fox Landing-loop harness passed: scene=22/21 fall=26/26 landing=31/31 wait=10/10 frames=16/14 floor=0/0 safe=1
verify-battle-mariofox-process-loop-harness.ps1 -> Battle Mario/Fox process-loop harness passed: scene=22/21 frames=30/28 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff root-dx=137250/-214000 rise=74300/92000 final=Wait/Ground safe=1
verify-battle-mariofox-scheduler-loop-harness.ps1 -> Battle Mario/Fox scheduler-loop harness passed: scene=22/21 updates=30 callbacks=30/30 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff root-dx=137250/-214000 rise=74300/92000 final=Wait/Ground safe=1
verify-battle-mariofox-controller-loop-harness.ps1 -> Battle Mario/Fox controller-loop harness passed: scene=22/21 reads=30 updates=30 callbacks=30/30 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff root-dx=137250/-214000 rise=74300/92000 final=Wait/Ground safe=1
verify-battle-mariofox-preview-loop-harness.ps1 -> Battle Mario/Fox preview-loop harness passed: scene=22/21 drawFrames=7 callbacks=14 pixels=582 screenDx=61/-62 screenRise=52/52 rootDx=137250/-214000 final=Wait/Ground safe=1
verify-battle-mariofox-gcrunall-loop-harness.ps1 -> Battle Mario/Fox gcRunAll-loop harness passed: scene=22/21 gcRunAll=30 callbacks=30/30 draws=11 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1
verify-battle-mariofox-live-preview-harness.ps1 -> Battle Mario/Fox live-preview harness passed: scene=22/21 liveReads=60 frames=60/60 draws=16 pixels=1410 idle=Wait/Ground directInput=0 safe=1
verify-battle-mariofox-gcdrawall-loop-harness.ps1 -> Battle Mario/Fox gcDrawAll-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1
verify-battle-mariofox-stage-gcdrawall-loop-harness.ps1 -> Battle Mario/Fox stage gcDrawAll-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0
verify-battle-mariofox-stage-collision-loop-harness.ps1 -> Battle Mario/Fox stage collision-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=0/0 floorRange=0-4 floorLines=4 probes=3/2
verify-battle-mariofox-stage-floor-edge-loop-harness.ps1 -> Battle Mario/Fox stage floor-edge-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=118750/42000 delta=137250/214000 probes=2/2 queries=6/4
verify-battle-mariofox-stage-mpprocess-floor-loop-harness.ps1 -> Battle Mario/Fox stage MP floor-process-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=118750/42000 delta=137250/214000 probes=2/2 queries=50/46 mpProcessFloor=test=38/2 project=0/2 probes=1/2 below=2 p0line=3 p1line=3 fc=2/1/39
verify-battle-mariofox-stage-mpupdate-floor-loop-harness.ps1 -> Battle Mario/Fox stage mpProcessUpdateMain floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=90/85 mpProcessFloor=test=42/3 project=0/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/36/42 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000
verify-battle-mariofox-stage-mpsweep-floor-loop-harness.ps1 -> Battle Mario/Fox stage MP sweep floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=90/85 mpProcessFloor=test=42/3 project=0/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/36/42 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000 mpSweepFloor=check=1/36 same=35/1 diff=1 line=3->0 second=34/34 p0line=3 p1line=3
verify-battle-mariofox-stage-mpcross-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP cross-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=138/133 mpProcessFloor=test=25/20 project=17/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/83/43 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000 mpSweepFloor=check=18/19 same=18/1 diff=18 line=3->0 second=34/17 p0line=3 p1line=3 mpCrossFloor=line=-1->3 live=17/17 accepted=17 p0line=3 p1line=3
verify-battle-mariofox-stage-mpadjust-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP adjust-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=138/133 mpProcessFloor=test=25/20 project=17/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/83/43 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000 mpSweepFloor=check=18/19 same=18/1 diff=18 line=3->0 second=34/17 p0line=3 p1line=3 mpCrossFloor=line=-1->3 live=17/17 accepted=17 p0line=3 p1line=3 mpAdjustFloor=run=17 check=17/17 wallMiss=17/17 edge=17/17 noAdjust=17 p0line=3 p1maintained=3
verify-battle-mariofox-stage-mpedge-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP edge-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=138/133 mpProcessFloor=test=25/20 project=17/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/83/43 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000 mpSweepFloor=check=18/19 same=18/1 diff=18 line=3->0 second=34/17 p0line=3 p1line=3 mpCrossFloor=line=-1->3 live=17/17 accepted=17 p0line=3 p1line=3 mpAdjustFloor=run=17 checkHit=0/0 checkMiss=17/17 wallHit=0/0 wallMiss=17/17 edge=18/18 adjust=0/0 noAdjust=17 p0line=3 p1maintained=3 mpEdgeFloor=edge=6/5 kind=3/2 calls=18/18
verify-battle-mariofox-stage-mpwall-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP wall-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpAdjustFloor=run=17 checkHit=0/0 checkMiss=17/17 wallHit=0/0 wallMiss=17/17 edge=18/18 adjust=0/0 noAdjust=17 mpEdgeFloor=edge=6/5 kind=3/2 calls=18/18 mpWallFloor=blocked floor=3 edgeWall=5 kind=2 side=1 candidates=2 miss=1 mpWallHitScout=none floors=4 walls=8 candidates=6 hyruleWallHit=hit floor=5 wall=13 edge=12 side=0 delta=-1600/-388
verify-battle-mariofox-stage-mpstale-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP stale-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpWallFloor=blocked floor=3 edgeWall=5 kind=2 side=1 candidates=2 miss=1 mpStaleFloor=line=1->0 live=18/0 accepted=1 p0line=0 p1line=3
verify-battle-mariofox-stage-mplivestale-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP live-stale-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpWallFloor=blocked floor=3 edgeWall=5 kind=2 side=1 candidates=2 miss=1 mpStaleFloor=line=1->0 live=1/0 accepted=1 p0line=0 p1line=3 mpLiveStaleFloor=line=1->0 selected=1 live=1/0 accepted=1 p0line=0 p1line=3
verify-battle-mariofox-stage-mpmotionstale-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP motion-stale-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000
verify-battle-mariofox-stage-mpcliffstatus-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP cliff-status-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3
verify-battle-mariofox-stage-mpclifftick-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP cliff-tick-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3
verify-battle-mariofox-stage-mpfallmap-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP Fall-map-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3 mpFallMap=phys=1 fast=1 grav=1 map=1 noColl=1 y=200000->194000
verify-battle-mariofox-stage-mpceilstatus-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP Ceiling-status floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCeilFloor=line=4 kind=1 check=2/2 diff=2/2 fc=4/4 y=-1464000->-1472000 ceil=-1072000 dist=-8000 mpCeilStatus=line=4 status=26->66 motion=20->57 ga=1->0 velY=40000->0 masks=0x4400/0x400
verify-battle-mariofox-stage-mpcliffcatch-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP Cliff-catch floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3 mpFallMap=phys=1 fast=1 grav=1 map=1 noColl=1 y=200000->194000 mpFallLand=phys=1 map=1 floor=1 land=1/0 status=26->31/25 y=4000->0 velY=-8000->-12000 mpCeilFloor=line=4 kind=1 check=4/2 diff=4/2 fc=4/4 y=-1464000->-1472000 ceil=-1072000 dist=-8000 mpCeilStatus=line=4 status=26->66 motion=20->57 ga=1->0 velY=40000->0 masks=0x4400/0x400 mpCliffCatch=line=3 side=1 status=26->84 motion=20->72 occ=1/0 ledge=2318000/0 root=2518000,-408000->2318000,-408000 masks=0x2000/0x2000
verify-battle-mariofox-stage-mpcliffwait-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP Cliff-wait floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3 mpFallMap=phys=1 fast=1 grav=1 map=1 noColl=1 y=200000->194000 mpFallLand=phys=1 map=1 floor=1 land=1/0 status=26->31/25 y=4000->0 velY=-8000->-12000 mpCeilFloor=line=4 kind=1 check=4/2 diff=4/2 fc=4/4 y=-1464000->-1472000 ceil=-1072000 dist=-8000 mpCeilStatus=line=4 status=26->66 motion=20->57 ga=1->0 velY=40000->0 masks=0x4400/0x400 mpCliffCatch=line=3 side=1 status=26->84 motion=20->72 occ=1/0 ledge=2318000/0 root=2518000,-408000->2318000,-408000 masks=0x2000/0x2000 mpCliffWait=status=84->85->85 motion=72->73->73 fallWait=1080->1079 cliff=3 guards=1/1/1
verify-battle-mariofox-stage-mpcliffattack-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP Cliff-attack floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffWait=status=84->85->85 motion=72->73->73 fallWait=1080->1079 cliff=3 guards=1/1/1 mpCliffAttack=status=85->86 motion=73->74 queue=1 cliff=3 button=0x8000
verify-battle-mariofox-stage-mpcliffattack-action-loop-harness.ps1 -> Battle Mario/Fox Stage MP Cliff-attack action-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffAttack=status=85->86 motion=73->74 queue=1 cliff=3 button=0x8000 mpCliffAttackAction=status=86->92->93 motion=74->80->81 cliff=3 floor=3 calls=1/1/1
verify-battle-mariofox-stage-mpcliffcommon2-loop-harness.ps1 -> Battle Mario/Fox Stage MP CliffCommon2-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffAttackAction=status=86->92->93 motion=74->80->81 cliff=3 floor=3 calls=1/1/1 mpCliffCommon2=status=93->93->93->93 motion=81->81->81->81 cliff=3 floor=3->3 calls=1/1/1
verify-battle-mariofox-stage-mpcliffescape-action-loop-harness.ps1 -> Battle Mario/Fox Stage MP Cliff-escape action-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffEscapeAction=status=85->86->96->97 motion=73->74->84->85 cliff=3 floor=3 calls=1/1/1
verify-battle-mariofox-stage-mpcliffescape-common2-loop-harness.ps1 -> Battle Mario/Fox Stage MP CliffEscape Common2-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffEscapeAction=status=85->86->96->97 motion=73->74->84->85 cliff=3 floor=3 calls=1/1/1 mpCliffEscapeCommon2=status=97->97->97->97 motion=85->85->85->85 cliff=3 floor=3->3 calls=1/1/1
verify-battle-mariofox-stage-mpcliffclimb-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP Cliff-climb floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffClimb=climbStatus=86/74 dropStatus=26/20 cliff=3 sticks=0,80/80,0 calls=2/2 recatch=26->84 hold=0/1 block=0
verify-battle-mariofox-stage-mpcliffclimb-action-loop-harness.ps1 -> Battle Mario/Fox Stage MP Cliff-climb action-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffClimb=climbStatus=86/74 dropStatus=26/20 cliff=3 sticks=0,80/80,0 calls=2/2 mpCliffClimbAction=status=86->87->88 motion=74->75->76 cliff=3 floor=3 calls=1/1/1
verify-battle-mariofox-stage-mpcliffclimb-common2-loop-harness.ps1 -> Battle Mario/Fox Stage MP Cliff-climb common2-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffClimb=climbStatus=86/74 dropStatus=26/20 cliff=3 sticks=0,80/80,0 calls=2/2 mpCliffClimbAction=status=86->87->88 motion=74->75->76 cliff=3 floor=3 calls=1/1/1 mpCliffClimbCommon2=status=88->88->88->88 motion=76->76->76->76 cliff=3 floor=3->3 calls=1/1/1
verify-battle-mariofox-stage-mpcliffclimb-finish-loop-harness.ps1 -> Battle Mario/Fox Stage MP Cliff-climb finish-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffClimbFinish=status=88->10 motion=76->4 cliff=3 floor=3->3 reset=0/0/0x7ffff calls=1/1/1
verify-battle-mariofox-stage-mpcliffwait-damage-loop-harness.ps1 -> Battle Mario/Fox Stage MP Cliff-wait damage-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffWaitDamage=status=85->57 motion=73->50 fallWait=1->0 catch=30 tics=65536 hold=1->0 procDmg=0 dmgTick=1/1/5 velY=0->-4000 cliffCatch=84/72/1 hold=1 passiveStand=73/62/0->cb1/1->10/4/0 passive=81/70/0->cb1/1->10/4/0 downBounce=68/59/0 dbuf=60 downWait=70/-2 wait=180->179 downStand=72/61 flag1=1->0 dmgMul=1000 coll=4
verify-menu-chain-vsbattle-harness.ps1 -> Menu-chain VSBattle harness passed: VS->PV mask=0xff, PV->Maps mask=0xff, Maps preview=0xff, Maps->VSBattle mask=0xff, stage=0xff, ground=0x3ff, final=22/21
verify-menu-chain-pupupu-update-harness.ps1 -> Menu-chain Pupupu update harness passed: VS->PV mask=0xff, PV->Maps mask=0xff, Maps->VSBattle mask=0xff, ground=0x3ff, update=0xff, final=22/21
verify-menu-chain-mariofox-model-harness.ps1 -> Menu-chain Mario/Fox model harness passed: chain masks=0xff/0xff/0xff, assets=0xffff, setup=0xfff, realGObjs=2
verify-menu-chain-mariofox-struct-harness.ps1 -> Menu-chain Mario/Fox struct harness passed: chain masks=0xff/0xff/0xff, model=0xfff, struct=0xfff, final=22/21
verify-menu-chain-mariofox-init-harness.ps1 -> Menu-chain Mario/Fox init harness passed: chain masks=0xff/0xff/0xff, model=0xfff, struct=0xfff, init=0x3fff, final=22/21
verify-menu-chain-mariofox-wait-harness.ps1 -> Menu-chain Mario/Fox Wait harness passed: chain final=22/21 waitMask=0xfff count=2 status=10/10 motion=4/4 callbacks=0
verify-menu-chain-mariofox-wait-tick-harness.ps1 -> Menu-chain Mario/Fox Wait tick harness passed: chain final=22/21 wait=0xfff tick=0x3ff callbacks=2/2/2 stable=1
verify-menu-chain-mariofox-wait-ground-harness.ps1 -> Menu-chain Mario/Fox Wait ground harness passed: chain final=22/21 wait=0xfff tick=0x3ff ground=0x7ff map=2/2 stable=1
verify-menu-chain-mariofox-display-probe-harness.ps1 -> Menu-chain Mario/Fox display probe harness passed: chain final=22/21 display=0x7ff dobj=25/27 ready=14/18 stable=1
verify-menu-chain-mariofox-dl-scan-harness.ps1 -> Menu-chain Mario/Fox DL scan harness passed: chain final=22/21 dl=0x22dcbe8/0x23055b0 asset=4294967294/4294967294 commands=59/69 blocker=0/0 safe=1
verify-menu-chain-mariofox-dl-execute-harness.ps1 -> Menu-chain Mario/Fox DL execute harness passed: chain final=22/21 commands=59/69 verts=28/23 tris=37/20 safe=1
verify-menu-chain-mariofox-dl-draw-harness.ps1 -> Menu-chain Mario/Fox DL draw harness passed: chain final=22/21 pixels=4274/5345 tris=37/20 real=37/18 marker=0/2 preview=96x72 commit=1 safe=1
verify-menu-chain-mariofox-dash-run-harness.ps1 -> Menu-chain Mario/Fox Dash-Run harness passed: chain final=22/21 wait->attack11=190/165 cb=0xff tick=0xff waitproc=0x3; guard=152/134 cb=0xff state=0xfffffe0f fgm=13; escape=156/136 cb=0x3ff state=0xff; attack11->attack12=191/166 cb=0xff goto=0xf; attack12->attack13=220/195 foxblock=191/166 cb=0xf goto=0x7; fox attack100start=220/195 cb=0xf goto=0x3; fox attack100loop=221/196 cb=0xf goto=0x3 tick=0xfff; anim=0x3f; dash->run->runbrake + attackdash=192/167 cb=0xff tick=0x3f runproc=0x3 root-dx=301575/-418500 vel=51200/71000->47450/66000
verify-menu-chain-mariofox-stage-mpcliffattack-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP Cliff-attack floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffWait=status=84->85->85 motion=72->73->73 fallWait=1080->1079 cliff=3 guards=1/1/1 mpCliffAttack=status=85->86 motion=73->74 queue=1 cliff=3 button=0x8000
verify-menu-chain-mariofox-stage-mpcliffattack-action-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP Cliff-attack action-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffAttack=status=85->86 motion=73->74 queue=1 cliff=3 button=0x8000 mpCliffAttackAction=status=86->92->93 motion=74->80->81 cliff=3 floor=3 calls=1/1/1
verify-menu-chain-mariofox-stage-mpcliffcommon2-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP CliffCommon2-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffAttackAction=status=86->92->93 motion=74->80->81 cliff=3 floor=3 calls=1/1/1 mpCliffCommon2=status=93->93->93->93 motion=81->81->81->81 cliff=3 floor=3->3 calls=1/1/1
verify-menu-chain-mariofox-stage-mpcliffescape-action-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP Cliff-escape action-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffEscapeAction=status=85->86->96->97 motion=73->74->84->85 cliff=3 floor=3 calls=1/1/1
verify-menu-chain-mariofox-stage-mpcliffescape-common2-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP CliffEscape Common2-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffEscapeAction=status=85->86->96->97 motion=73->74->84->85 cliff=3 floor=3 calls=1/1/1 mpCliffEscapeCommon2=status=97->97->97->97 motion=85->85->85->85 cliff=3 floor=3->3 calls=1/1/1
verify-menu-chain-mariofox-stage-mpcliffclimb-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP Cliff-climb floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffClimb=climbStatus=86/74 dropStatus=26/20 cliff=3 sticks=0,80/80,0 calls=2/2 recatch=26->84 hold=0/1 block=0
verify-menu-chain-mariofox-stage-mpcliffclimb-action-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP Cliff-climb action-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffClimb=climbStatus=86/74 dropStatus=26/20 cliff=3 sticks=0,80/80,0 calls=2/2 mpCliffClimbAction=status=86->87->88 motion=74->75->76 cliff=3 floor=3 calls=1/1/1
verify-menu-chain-mariofox-stage-mpcliffclimb-common2-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP Cliff-climb common2-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffClimb=climbStatus=86/74 dropStatus=26/20 cliff=3 sticks=0,80/80,0 calls=2/2 mpCliffClimbAction=status=86->87->88 motion=74->75->76 cliff=3 floor=3 calls=1/1/1 mpCliffClimbCommon2=status=88->88->88->88 motion=76->76->76->76 cliff=3 floor=3->3 calls=1/1/1
verify-menu-chain-mariofox-stage-mpcliffclimb-finish-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP Cliff-climb finish-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffClimbFinish=status=88->10 motion=76->4 cliff=3 floor=3->3 reset=0/0/0x7ffff calls=1/1/1
verify-menu-chain-mariofox-stage-mpcliffwait-damage-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP Cliff-wait damage-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCliffWaitDamage=status=85->57 motion=73->50 fallWait=1->0 catch=30 tics=65536 hold=1->0 procDmg=0 dmgTick=1/1/5 velY=0->-4000 cliffCatch=84/72/1 hold=1 passiveStand=73/62/0->cb1/1->10/4/0 passive=81/70/0->cb1/1->10/4/0 downBounce=68/59/0 dbuf=60 downWait=70/-2 wait=180->179 downStand=72/61 flag1=1->0 dmgMul=1000 coll=4
verify-battle-mariofox-stage-mppassive-loop-harness.ps1 -> Battle Mario/Fox Stage MP Passive-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 passiveStand=73/62/0->cb1/1->10/4/0 passive=81/70/0->cb1/1->10/4/0 mpPassive=ps=73/62->10/4 ticks=3/2/2 passive=81/70->10/4 ticks=3/2/2
verify-menu-chain-mariofox-stage-mppassive-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP Passive-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 passiveStand=73/62/0->cb1/1->10/4/0 passive=81/70/0->cb1/1->10/4/0 mpPassive=ps=73/62->10/4 ticks=3/2/2 passive=81/70->10/4 ticks=3/2/2
verify-battle-mariofox-stage-mpdownwait-loop-harness.ps1 -> Battle Mario/Fox Stage MP DownWait-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpPassive=ps=73/62->10/4 ticks=3/2/2 passive=81/70->10/4 ticks=3/2/2 mpDownWait=status=70/-2->72/61->10/4 attack=80/69->10/4 rolls=76/65->10/4,78/67->10/4 ticks=9/9/9 rollDelta=10000/-10000 downStandTicks=9/8/8 source=0x12345 checks=1/1/1 dsChecks=8/8/8 input=0,80 flag1=1->0/1
verify-menu-chain-mariofox-stage-mpdownwait-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP DownWait-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpPassive=ps=73/62->10/4 ticks=3/2/2 passive=81/70->10/4 ticks=3/2/2 mpDownWait=status=70/-2->72/61->10/4 attack=80/69->10/4 rolls=76/65->10/4,78/67->10/4 ticks=9/9/9 rollDelta=10000/-10000 downStandTicks=9/8/8 source=0x12345 checks=1/1/1 dsChecks=8/8/8 input=0,80 flag1=1->0/1
verify-battle-mariofox-stage-turn-loop-harness.ps1 -> Battle Mario/Fox Stage Turn-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 turn=10/4->18/12->10/4 lr=1->-1 vel=2500->-2500 calls=1/1/1
verify-menu-chain-mariofox-stage-turn-loop-harness.ps1 -> Menu-chain Mario/Fox Stage Turn-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 turn=10/4->18/12->10/4 lr=1->-1 vel=2500->-2500 calls=1/1/1
verify-battle-mariofox-stage-mpdownrecover-loop-harness.ps1 -> Battle Mario/Fox Stage MP DownRecover-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 downRecoverD=wait=69/-2 stand=71/60 atk=79/68 roll=75/64,77/66 final=0xf
verify-menu-chain-mariofox-stage-mpdownrecover-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP DownRecover-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 downRecoverD=wait=69/-2 stand=71/60 atk=79/68 roll=75/64,77/66 final=0xf
verify-battle-mariofox-stage-mpcliffledge-loop-harness.ps1 -> Battle Mario/Fox Stage MP cliff-ledge loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpCliffLedge=occ=1 drop=26/20 wait=30 hold=0 recatch=84/72 finish=10/4 line=3
verify-menu-chain-mariofox-stage-mpcliffledge-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP cliff-ledge loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpCliffLedge=occ=1 drop=26/20 wait=30 hold=0 recatch=84/72 finish=10/4 line=3
verify-battle-mariofox-stage-mpclifflive-loop-harness.ps1 -> Battle Mario/Fox Stage MP cliff-live loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpCliffLive=wait=85/73 climb=86/74 quick2=88/76 finish=10/4 drop=26/20 src=0xfff
verify-menu-chain-mariofox-stage-mpclifflive-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP cliff-live loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpCliffLive=wait=85/73 climb=86/74 quick2=88/76 finish=10/4 drop=26/20 src=0xfff
verify-battle-mariofox-stage-mpwallhit-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP wall-hit floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpWallHitFloor=pass floor=5 wall=13 edge=12 side=0 mapNodes=1 delta=-1600/-388 mpCliffLive=wait=85/73 climb=86/74 quick2=88/76 finish=10/4 drop=26/20 src=0xfff
verify-menu-chain-mariofox-stage-mpwallhit-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP wall-hit floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpWallHitFloor=pass floor=5 wall=13 edge=12 side=0 mapNodes=1 delta=-1600/-388 mpCliffLive=wait=85/73 climb=86/74 quick2=88/76 finish=10/4 drop=26/20 src=0xfff
verify-battle-mariofox-stage-mpwallcopy-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP wall-copy floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpWallHitFloor=pass floor=5 wall=13 edge=12 side=0 mapNodes=1 delta=-1600/-388 mpWallCopy=pass floor=5 wall=13 edge=12 delta=-1600/-388 mpCliffLive=wait=85/73 climb=86/74 quick2=88/76 finish=10/4 drop=26/20 src=0xfff
verify-menu-chain-mariofox-stage-mpwallcopy-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP wall-copy floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpWallHitFloor=pass floor=5 wall=13 edge=12 side=0 mapNodes=1 delta=-1600/-388 mpWallCopy=pass floor=5 wall=13 edge=12 delta=-1600/-388 mpCliffLive=wait=85/73 climb=86/74 quick2=88/76 finish=10/4 drop=26/20 src=0xfff
verify-battle-mariofox-stage-mppass-floor-loop-harness.ps1 -> Battle Mario/Fox Stage MP pass-through floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpWallHitFloor=pass floor=5 wall=13 edge=12 side=0 mapNodes=1 delta=-1600/-388 mpWallCopy=pass floor=5 wall=13 edge=12 delta=-1600/-388 mpPass=line=0 flags=0x4000 route=2 same=1 diff=1 cb=1/1/0
verify-menu-chain-mariofox-stage-mppass-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP pass-through floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpWallHitFloor=pass floor=5 wall=13 edge=12 side=0 mapNodes=1 delta=-1600/-388 mpWallCopy=pass floor=5 wall=13 edge=12 delta=-1600/-388 mpPass=line=0 flags=0x4000 route=2 same=1 diff=1 cb=1/1/0
verify-current.ps1 -> Latest verification profile passed.
verify-menu-chain-mariofox-dl-draw-multi-harness.ps1 -> Menu-chain Mario/Fox multi-DL draw harness passed: chain final=22/21 candidates=14/18 selected=4/4 pixels=6190/7026 tris=87/79 real=82/76 marker=1/3 clean=4/4 preview=96x72 safe=1
verify-menu-chain-mariofox-dl-draw-all-harness.ps1 -> Menu-chain Mario/Fox all-DL draw harness passed: chain final=22/21 callbacks=2 candidates=14/18 selected=14/18 pixels=14913/13432 tris=334/322 real=306/290 marker=10/24 clean=14/18 failed=0/0 preview=96x72 safe=1
verify-menu-chain-mariofox-walk-input-harness.ps1 -> Menu-chain Mario/Fox Walk input harness passed: chain final=22/21 status=12/13 motion=6/7 stick=40/80 vel=12000/36000 callbacks=2 safe=1
verify-menu-chain-mariofox-walk-loop-harness.ps1 -> Menu-chain Mario/Fox Walk loop harness passed: chain final=22/21 frames=4/4 root-dx=48000/-144000 release=Wait vel=12000/36000->6000/28000 safe=1
verify-menu-chain-mariofox-jump-loop-harness.ps1 -> Menu-chain Mario/Fox Jump-loop harness passed: jump dx=100800/138900 dy=395400/468000 vy=74300/92000->59900/68000 attackAir=209/184 map=0x3ff dir=0x1f
verify-menu-chain-mariofox-landing-loop-harness.ps1 -> Menu-chain Mario/Fox Landing-loop harness passed: scene=22/21 fall=26/26 landing=31/31 wait=10/10 frames=16/14 floor=0/0 safe=1
verify-menu-chain-mariofox-process-loop-harness.ps1 -> Menu-chain Mario/Fox process-loop harness passed: scene=22/21 frames=30/28 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff root-dx=137250/-214000 rise=74300/92000 final=Wait/Ground safe=1
verify-menu-chain-mariofox-scheduler-loop-harness.ps1 -> Menu-chain Mario/Fox scheduler-loop harness passed: scene=22/21 updates=30 callbacks=30/30 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff root-dx=137250/-214000 rise=74300/92000 final=Wait/Ground safe=1
verify-menu-chain-mariofox-controller-loop-harness.ps1 -> Menu-chain Mario/Fox controller-loop harness passed: scene=22/21 reads=30 updates=30 callbacks=30/30 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff root-dx=137250/-214000 rise=74300/92000 final=Wait/Ground safe=1
verify-menu-chain-mariofox-preview-loop-harness.ps1 -> Menu-chain Mario/Fox preview-loop harness passed: scene=22/21 drawFrames=7 callbacks=14 pixels=582 screenDx=61/-62 screenRise=52/52 rootDx=137250/-214000 final=Wait/Ground safe=1
verify-menu-chain-mariofox-gcrunall-loop-harness.ps1 -> Menu-chain Mario/Fox gcRunAll-loop harness passed: scene=22/21 gcRunAll=30 callbacks=30/30 draws=11 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1
verify-menu-chain-mariofox-live-preview-harness.ps1 -> Menu-chain Mario/Fox live-preview harness passed: scene=22/21 liveReads=60 frames=60/60 draws=16 pixels=1410 idle=Wait/Ground directInput=0 safe=1
verify-menu-chain-mariofox-gcdrawall-loop-harness.ps1 -> Menu-chain Mario/Fox gcDrawAll-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1
verify-menu-chain-mariofox-stage-gcdrawall-loop-harness.ps1 -> Menu-chain Mario/Fox stage gcDrawAll-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0
verify-menu-chain-mariofox-stage-collision-loop-harness.ps1 -> Menu-chain Mario/Fox stage collision-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=0/0 floorRange=0-4 floorLines=4 probes=3/2
verify-menu-chain-mariofox-stage-floor-edge-loop-harness.ps1 -> Menu-chain Mario/Fox stage floor-edge-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=118750/42000 delta=137250/214000 probes=2/2 queries=6/4
verify-menu-chain-mariofox-stage-mpprocess-floor-loop-harness.ps1 -> Menu-chain Mario/Fox stage MP floor-process-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=118750/42000 delta=137250/214000 probes=2/2 queries=50/46 mpProcessFloor=test=38/2 project=0/2 probes=1/2 below=2 p0line=3 p1line=3 fc=2/1/39
verify-menu-chain-mariofox-stage-mpupdate-floor-loop-harness.ps1 -> Menu-chain Mario/Fox stage mpProcessUpdateMain floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=90/85 mpProcessFloor=test=42/3 project=0/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/36/42 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000
verify-menu-chain-mariofox-stage-mpsweep-floor-loop-harness.ps1 -> Menu-chain Mario/Fox stage MP sweep floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=90/85 mpProcessFloor=test=42/3 project=0/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/36/42 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000 mpSweepFloor=check=1/36 same=35/1 diff=1 line=3->0 second=34/34 p0line=3 p1line=3
verify-menu-chain-mariofox-stage-mpcross-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP cross-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=138/133 mpProcessFloor=test=25/20 project=17/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/83/43 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000 mpSweepFloor=check=18/19 same=18/1 diff=18 line=3->0 second=34/17 p0line=3 p1line=3 mpCrossFloor=line=-1->3 live=17/17 accepted=17 p0line=3 p1line=3
verify-menu-chain-mariofox-stage-mpadjust-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP adjust-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=138/133 mpProcessFloor=test=25/20 project=17/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/83/43 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000 mpSweepFloor=check=18/19 same=18/1 diff=18 line=3->0 second=34/17 p0line=3 p1line=3 mpCrossFloor=line=-1->3 live=17/17 accepted=17 p0line=3 p1line=3 mpAdjustFloor=run=17 check=17/17 wallMiss=17/17 edge=17/17 noAdjust=17 p0line=3 p1maintained=3
verify-menu-chain-mariofox-stage-mpedge-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP edge-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=138/133 mpProcessFloor=test=25/20 project=17/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/83/43 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000 mpSweepFloor=check=18/19 same=18/1 diff=18 line=3->0 second=34/17 p0line=3 p1line=3 mpCrossFloor=line=-1->3 live=17/17 accepted=17 p0line=3 p1line=3 mpAdjustFloor=run=17 checkHit=0/0 checkMiss=17/17 wallHit=0/0 wallMiss=17/17 edge=18/18 adjust=0/0 noAdjust=17 p0line=3 p1maintained=3 mpEdgeFloor=edge=6/5 kind=3/2 calls=18/18
verify-menu-chain-mariofox-stage-mpwall-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP wall-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpAdjustFloor=run=17 checkHit=0/0 checkMiss=17/17 wallHit=0/0 wallMiss=17/17 edge=18/18 adjust=0/0 noAdjust=17 mpEdgeFloor=edge=6/5 kind=3/2 calls=18/18 mpWallFloor=blocked floor=3 edgeWall=5 kind=2 side=1 candidates=2 miss=1 mpWallHitScout=none floors=4 walls=8 candidates=6 hyruleWallHit=hit floor=5 wall=13 edge=12 side=0 delta=-1600/-388
verify-menu-chain-mariofox-stage-mpstale-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP stale-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpWallFloor=blocked floor=3 edgeWall=5 kind=2 side=1 candidates=2 miss=1 mpStaleFloor=line=1->0 live=18/0 accepted=1 p0line=0 p1line=3
verify-menu-chain-mariofox-stage-mplivestale-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP live-stale-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpWallFloor=blocked floor=3 edgeWall=5 kind=2 side=1 candidates=2 miss=1 mpStaleFloor=line=1->0 live=1/0 accepted=1 p0line=0 p1line=3 mpLiveStaleFloor=line=1->0 selected=1 live=1/0 accepted=1 p0line=0 p1line=3
verify-menu-chain-mariofox-stage-mpmotionstale-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP motion-stale-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000
verify-menu-chain-mariofox-stage-mpcliffstatus-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP cliff-status-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3
verify-menu-chain-mariofox-stage-mpclifftick-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP cliff-tick-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3
verify-menu-chain-mariofox-stage-mpfallmap-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP Fall-map-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3 mpFallMap=phys=1 fast=1 grav=1 map=1 noColl=1 y=200000->194000
verify-menu-chain-mariofox-stage-mpceilstatus-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP Ceiling-status floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpCeilFloor=line=4 kind=1 check=2/2 diff=2/2 fc=4/4 y=-1464000->-1472000 ceil=-1072000 dist=-8000 mpCeilStatus=line=4 status=26->66 motion=20->57 ga=1->0 velY=40000->0 masks=0x4400/0x400
verify-menu-chain-mariofox-stage-mpcliffcatch-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP Cliff-catch floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3 mpFallMap=phys=1 fast=1 grav=1 map=1 noColl=1 y=200000->194000 mpFallLand=phys=1 map=1 floor=1 land=1/0 status=26->31/25 y=4000->0 velY=-8000->-12000 mpCeilFloor=line=4 kind=1 check=4/2 diff=4/2 fc=4/4 y=-1464000->-1472000 ceil=-1072000 dist=-8000 mpCeilStatus=line=4 status=26->66 motion=20->57 ga=1->0 velY=40000->0 masks=0x4400/0x400 mpCliffCatch=line=3 side=1 status=26->84 motion=20->72 occ=1/0 ledge=2318000/0 root=2518000,-408000->2318000,-408000 masks=0x2000/0x2000
verify-menu-chain-mariofox-stage-mpcliffwait-floor-loop-harness.ps1 -> Menu-chain Mario/Fox Stage MP Cliff-wait floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3 mpFallMap=phys=1 fast=1 grav=1 map=1 noColl=1 y=200000->194000 mpFallLand=phys=1 map=1 floor=1 land=1/0 status=26->31/25 y=4000->0 velY=-8000->-12000 mpCeilFloor=line=4 kind=1 check=4/2 diff=4/2 fc=4/4 y=-1464000->-1472000 ceil=-1072000 dist=-8000 mpCeilStatus=line=4 status=26->66 motion=20->57 ga=1->0 velY=40000->0 masks=0x4400/0x400 mpCliffCatch=line=3 side=1 status=26->84 motion=20->72 occ=1/0 ledge=2318000/0 root=2518000,-408000->2318000,-408000 masks=0x2000/0x2000 mpCliffWait=status=84->85->85 motion=72->73->73 fallWait=1080->1079 cliff=3 guards=1/1/1
verify-all.ps1 -Profile Latest -> Latest verification profile passed.
verify-regression.ps1 -> Regression verification profile passed.
```

## Important Local Boundaries

- `decomp/` is read-only reference material.
- `src/import` owns wrappers for original BattleShip translation units.
- `src/nds` owns DS hardware integration.
- `src/port` owns compatibility/backend seams.
- `include` owns compatibility declarations.
- Do not edit generated build output or emulator logs/configs by hand.

`src/port/scene_backend.c` is intentionally a thin include orchestrator over:

- `src/port/diagnostics.c`
- `src/port/taskman_seam.c`
- `src/port/reloc_backend.c`
- `src/port/sprite_preview_backend.c`
- `src/port/opening_movie_backend.c`
- `src/port/title_backend.c`

The dev/test harness lives in:

- `include/nds/nds_scene_harness.h`
- `src/port/scene_harness.c`
- `src/import/battleship_scmanager.c`

Build-time harness modes are selected with `NDS_DEV_SCENE_HARNESS=normal`,
`title`, `vs_setup`, `vs_start_transition`, `players_setup`, `maps_setup`,
`menu_chain_vsbattle`, `battle_fd`, `battle_pupupu_stage`,
`battle_pupupu_update`, `menu_chain_pupupu_update`,
`battle_mariofox_model`, `menu_chain_mariofox_model`,
`battle_mariofox_struct`, `menu_chain_mariofox_struct`,
`battle_mariofox_init`, `menu_chain_mariofox_init`,
`battle_mariofox_wait`, `menu_chain_mariofox_wait`,
`battle_mariofox_wait_tick`, `menu_chain_mariofox_wait_tick`,
`battle_mariofox_wait_ground`, `menu_chain_mariofox_wait_ground`,
`battle_mariofox_display_probe`, `menu_chain_mariofox_display_probe`,
`battle_mariofox_dl_scan`, `menu_chain_mariofox_dl_scan`,
`battle_mariofox_dl_execute`, `menu_chain_mariofox_dl_execute`,
`battle_mariofox_dl_draw`, `menu_chain_mariofox_dl_draw`,
`battle_mariofox_dl_draw_multi`, `menu_chain_mariofox_dl_draw_multi`,
`battle_mariofox_dl_draw_all`, `menu_chain_mariofox_dl_draw_all`,
`battle_mariofox_walk_input`, `menu_chain_mariofox_walk_input`,
`battle_mariofox_walk_loop`, `menu_chain_mariofox_walk_loop`,
`battle_mariofox_dash_run`, `menu_chain_mariofox_dash_run`,
`battle_mariofox_jump_loop`, `menu_chain_mariofox_jump_loop`,
`battle_mariofox_landing_loop`, `menu_chain_mariofox_landing_loop`,
`battle_mariofox_process_loop`, `menu_chain_mariofox_process_loop`,
`battle_mariofox_scheduler_loop`, `menu_chain_mariofox_scheduler_loop`,
`battle_mariofox_controller_loop`, `menu_chain_mariofox_controller_loop`, or
the current Mario/Fox stage direct/menu-chain modes through
`battle_mariofox_stage_mpcliffclimb_floor_loop` and
`menu_chain_mariofox_stage_mpcliffclimb_floor_loop`. Keep normal builds on the
natural path.

Do not list those slices separately in `Makefile` `CFILES` until a later ABI
cleanup adds explicit narrow shared headers.

## Verifier Entry Points

- `scripts/verify-opening-boundary.ps1`: fast Opening Room progress gate.
- `scripts/verify-runtime.ps1`: full marker contract through current runtime
  boundary.
- `scripts/verify-opening-skip.ps1`: callback-time input injection and
  skip-to-Title proof.
- `scripts/verify-title-boundary.ps1`: natural movie-to-Title speed gate.
- `scripts/verify-title-harness.ps1`: direct Title scene harness gate without
  replaying Opening Room or the opening movie.
- `scripts/verify-vs-setup-harness.ps1`: direct VS Mode setup harness gate
  from Title without replaying Opening Room, the opening movie, or Title setup.
- `scripts/verify-vs-start-transition-harness.ps1`: direct VS Start transition
  harness gate proving original `mnVSModeMain` changes scene state to
  PlayersVS and then reaches the bounded imported PlayersVS boundary.
- `scripts/verify-players-vs-setup-harness.ps1`: direct PlayersVS setup gate.
- `scripts/verify-maps-setup-harness.ps1`: direct Maps setup gate.
- `scripts/verify-battle-fd-harness.ps1`: direct bounded VSBattle setup gate.
- `scripts/verify-battle-pupupu-stage-harness.ps1`: direct bounded VSBattle
  gate for Pupupu `MPGroundData` adoption plus original Dream Land ground GObj
  creation.
- `scripts/verify-battle-pupupu-update-harness.ps1`: direct bounded VSBattle
  gate plus two guarded original `grPupupuProcUpdate` ticks in a safe substate.
- `scripts/verify-battle-mariofox-model-harness.ps1`: direct bounded VSBattle
  gate plus asset-backed Mario/Fox fighter model GObj creation.
- `scripts/verify-battle-mariofox-struct-harness.ps1`: direct bounded
  VSBattle gate plus persistent project-owned `FTStruct` shells attached to
  the Mario/Fox fighter GObjs.
- `scripts/verify-battle-mariofox-init-harness.ps1`: direct bounded VSBattle
  gate plus bounded source-order Mario/Fox fighter init-state proof.
- `scripts/verify-battle-mariofox-wait-harness.ps1`: direct bounded VSBattle
  gate plus imported original Mario/Fox `ftCommonWaitSetStatus` proof through a
  Wait-only `ftMainSetStatus` seam.
- `scripts/verify-battle-mariofox-wait-tick-harness.ps1`: direct bounded
  VSBattle gate plus one bounded original Mario/Fox Wait interrupt callback
  tick and guarded physics/map seam calls.
- `scripts/verify-battle-mariofox-wait-ground-harness.ps1`: direct bounded
  VSBattle gate plus source-order Mario/Fox ground friction, air-transfer, and
  safe floor-map seam proof.
- `scripts/verify-battle-mariofox-jump-loop-harness.ps1`: direct bounded
  VSBattle gate plus original RunBrake -> Wait -> KneeBend -> JumpF airborne
  movement proof.
- `scripts/verify-menu-chain-vsbattle-harness.ps1`: full guarded VS Mode ->
  PlayersVS -> Maps -> imported bounded VSBattle setup plus Pupupu stage-data
  carry-through and original ground GObj creation gate.
- `scripts/verify-menu-chain-pupupu-update-harness.ps1`: full guarded VS Mode
  -> PlayersVS -> Maps -> imported bounded VSBattle setup plus the same safe
  Pupupu update proof.
- `scripts/verify-menu-chain-mariofox-model-harness.ps1`: full guarded VS Mode
  -> PlayersVS -> Maps -> imported bounded VSBattle setup plus the same
  asset-backed Mario/Fox fighter model GObj proof.
- `scripts/verify-menu-chain-mariofox-struct-harness.ps1`: full guarded VS
  Mode -> PlayersVS -> Maps -> imported bounded VSBattle setup plus the same
  persistent `FTStruct` shell proof.
- `scripts/verify-menu-chain-mariofox-init-harness.ps1`: full guarded VS Mode
  -> PlayersVS -> Maps -> imported bounded VSBattle setup plus the same
  bounded source-order Mario/Fox init-state proof.
- `scripts/verify-menu-chain-mariofox-wait-harness.ps1`: full guarded VS Mode
  -> PlayersVS -> Maps -> imported bounded VSBattle setup plus the same
  imported original Mario/Fox Wait status/motion setup proof.
- `scripts/verify-menu-chain-mariofox-wait-tick-harness.ps1`: full guarded VS
  Mode -> PlayersVS -> Maps -> imported bounded VSBattle setup plus the same
  bounded original Wait callback tick proof.
- `scripts/verify-menu-chain-mariofox-wait-ground-harness.ps1`: full guarded VS
  Mode -> PlayersVS -> Maps -> imported bounded VSBattle setup plus the same
  source-order ground-friction/map proof.
- `scripts/verify-menu-chain-mariofox-jump-loop-harness.ps1`: full guarded VS
  Mode -> PlayersVS -> Maps -> imported bounded VSBattle setup plus the same
  original RunBrake -> Wait -> KneeBend -> JumpF airborne movement proof.
- `scripts/verify-battle-mariofox-scheduler-loop-harness.ps1`: direct bounded
  VSBattle gate plus selected Mario/Fox `GObjProcess` callbacks attached with
  `gcAddGObjProcess`, invoked by `gcRunGObjProcess`, and driven through bounded
  `scVSBattleFuncUpdate` taskman updates.
- `scripts/verify-menu-chain-mariofox-scheduler-loop-harness.ps1`: full
  guarded VS Mode -> PlayersVS -> Maps -> imported bounded VSBattle setup plus
  the same scheduler-facing Mario/Fox proof.
- `scripts/verify-menu-chain-mariofox-display-probe-harness.ps1`: full guarded
  VS Mode -> PlayersVS -> Maps -> imported bounded VSBattle setup plus the
  same metadata-only Mario/Fox display callback proof as the direct display
  harness.
- `scripts/verify-battle-mariofox-dl-scan-harness.ps1`: direct bounded
  VSBattle gate plus parser-only scans of the first selected Mario/Fox fighter
  display lists.
- `scripts/verify-battle-mariofox-dl-execute-harness.ps1`: direct bounded
  VSBattle gate plus decode-only execution of the selected Mario/Fox fighter
  display lists.
- `scripts/verify-battle-mariofox-dl-draw-harness.ps1`: direct bounded
  VSBattle gate plus visible software preview drawing of the selected
  Mario/Fox first display lists.
- `scripts/verify-battle-mariofox-dl-draw-multi-harness.ps1`: direct bounded
  VSBattle gate plus visible software preview drawing of the first four
  DL-ready Mario/Fox DObjs per fighter.
- `scripts/verify-menu-chain-mariofox-dl-scan-harness.ps1`: full guarded VS
  Mode -> PlayersVS -> Maps -> imported bounded VSBattle setup plus the same
  parser-only Mario/Fox DL scan proof.
- `scripts/verify-menu-chain-mariofox-dl-execute-harness.ps1`: full guarded VS
  Mode -> PlayersVS -> Maps -> imported bounded VSBattle setup plus the same
  decode-only Mario/Fox DL execute proof.
- `scripts/verify-menu-chain-mariofox-dl-draw-harness.ps1`: full guarded VS
  Mode -> PlayersVS -> Maps -> imported bounded VSBattle setup plus the same
  visible Mario/Fox first-DL software draw proof.
- `scripts/verify-menu-chain-mariofox-dl-draw-multi-harness.ps1`: full
  guarded VS Mode -> PlayersVS -> Maps -> imported bounded VSBattle setup plus
  the same visible Mario/Fox multi-DL software draw proof.
- `scripts/verify-battle-mariofox-live-preview-harness.ps1`: direct bounded
  selected Mario/Fox `gcRunAll` moving-preview idle proof using live DS
  controller reads through the original controller read/global-update bridge.
- `scripts/verify-menu-chain-mariofox-live-preview-harness.ps1`: full guarded
  VS Mode -> PlayersVS -> Maps -> imported bounded VSBattle setup plus the
  same live-input moving-preview idle proof.
- `scripts/verify-battle-mariofox-stage-mpcliffstatus-floor-loop-harness.ps1`:
  direct bounded stage MP proof that imports original `ftcommonottotto.c` and
  proves source-order `mpCommonProcFighterOnCliffEdge` branches to Ottotto
  from `MAP_FLAG_FLOOREDGE` and Fall without that flag.
- `scripts/verify-menu-chain-mariofox-stage-mpcliffstatus-floor-loop-harness.ps1`:
  full guarded VS Mode -> PlayersVS -> Maps -> VSBattle path plus the same
  bounded MP cliff-status branch proof.
- `scripts/verify-battle-mariofox-stage-mpclifftick-floor-loop-harness.ps1`:
  direct bounded stage MP proof that runs one guarded original Ottotto
  update/interrupt/map tick and one guarded original Fall interrupt tick from
  the created cliff-status states.
- `scripts/verify-menu-chain-mariofox-stage-mpclifftick-floor-loop-harness.ps1`:
  full guarded VS Mode -> PlayersVS -> Maps -> VSBattle path plus the same
  bounded Ottotto/Fall callback-tick proof.
- `scripts/verify-battle-mariofox-stage-mpfallmap-floor-loop-harness.ps1`:
  direct bounded stage MP proof that consumes the P1 Fall state, runs the
  selected original Fall physics callback, integrates one bounded airborne
  step, and reaches the selected Fall map callback through a guarded
  no-collision branch.
- `scripts/verify-menu-chain-mariofox-stage-mpfallmap-floor-loop-harness.ps1`:
  full guarded VS Mode -> PlayersVS -> Maps -> VSBattle path plus the same
  bounded Fall physics/map callback proof.
- `scripts/verify-battle-mariofox-stage-mpfallland-floor-loop-harness.ps1`:
  direct bounded stage MP proof that consumes the Fall-map boundary, crosses
  decoded Pupupu floor line `3`, calls landing-floor setup, reaches original
  LandingLight, switches to Ground, and preserves the source-order airborne Y
  diagnostic.
- `scripts/verify-menu-chain-mariofox-stage-mpfallland-floor-loop-harness.ps1`:
  full guarded VS Mode -> PlayersVS -> Maps -> VSBattle path plus the same
  bounded Fall landing-floor proof.
- `scripts/verify-battle-mariofox-stage-mpceil-floor-loop-harness.ps1`:
  direct bounded stage MP proof that chooses real Pupupu ceiling line `4` and
  proves the bounded source-order ceiling test/adjust path through
  `mpProcessCheckTestCeilCollisionAdjNew` and
  `mpProcessRunCeilCollisionAdjNew`.
- `scripts/verify-menu-chain-mariofox-stage-mpceil-floor-loop-harness.ps1`:
  full guarded VS Mode -> PlayersVS -> Maps -> VSBattle path plus the same
  bounded ceiling-floor collision/adjust proof.
- `scripts/verify-battle-mariofox-stage-mpceilstatus-floor-loop-harness.ps1`:
  direct bounded stage MP proof that imports original `ftcommonstopceil.c` and
  proves the selected original `mpCommonProcFighterCliffFloorCeil` map callback
  reaches ceiling-heavy collision/adjust plus `ftCommonStopCeilSetStatus`.
- `scripts/verify-menu-chain-mariofox-stage-mpceilstatus-floor-loop-harness.ps1`:
  full guarded VS Mode -> PlayersVS -> Maps -> VSBattle path plus the same
  bounded ceiling-hit StopCeil status proof.
- `scripts/verify-battle-mariofox-stage-mpcliffcatch-floor-loop-harness.ps1`:
  direct bounded stage MP proof that imports original
  `ftcommoncliffcatchwait.c` and proves the selected original
  `mpCommonProcFighterCliffFloorCeil` map callback reaches the right-cliff
  test plus `ftCommonCliffCatchSetStatus`.
- `scripts/verify-menu-chain-mariofox-stage-mpcliffcatch-floor-loop-harness.ps1`:
  full guarded VS Mode -> PlayersVS -> Maps -> VSBattle path plus the same
  bounded CliffCatch status proof on real Pupupu line `3`.
- `scripts/verify-battle-mariofox-stage-mpcliffwait-floor-loop-harness.ps1`:
  direct bounded stage MP proof that starts from CliffCatch, calls original
  `ftCommonCliffCatchProcUpdate`, reaches original `ftCommonCliffWaitSetStatus`,
  and runs one guarded `ftCommonCliffWaitProcInterrupt` tick.
- `scripts/verify-menu-chain-mariofox-stage-mpcliffwait-floor-loop-harness.ps1`:
  full guarded VS Mode -> PlayersVS -> Maps -> VSBattle path plus the same
  bounded CliffCatch -> CliffWait proof on real Pupupu line `3`.
- `scripts/verify-battle-mariofox-stage-mpcliffattack-floor-loop-harness.ps1`:
  direct bounded stage MP proof that starts from CliffWait, injects an A-button
  tap, calls original `ftCommonCliffAttackCheckInterruptCommon`, and reaches
  CliffQuick/AttackQuick setup.
- `scripts/verify-menu-chain-mariofox-stage-mpcliffattack-floor-loop-harness.ps1`:
  full guarded VS Mode -> PlayersVS -> Maps -> VSBattle path plus the same
  bounded CliffWait -> CliffQuick/AttackQuick proof on real Pupupu line `3`.
- `scripts/verify-battle-mariofox-stage-mpcliffattack-action-loop-harness.ps1`:
  direct bounded stage MP proof that advances original CliffQuick through
  `ftCommonCliffQuickProcUpdate`, original CliffAttackQuick1 update, and the
  guarded Quick2 status branch.
- `scripts/verify-menu-chain-mariofox-stage-mpcliffattack-action-loop-harness.ps1`:
  full guarded VS Mode -> PlayersVS -> Maps -> VSBattle path plus the same
  CliffQuick -> CliffAttackQuick1 -> CliffAttackQuick2 action proof.
- `scripts/verify-battle-mariofox-stage-mpcliffcommon2-loop-harness.ps1`:
  direct bounded stage MP proof that consumes the created CliffAttackQuick2
  state through one original common2 update, physics, and map tick.
- `scripts/verify-menu-chain-mariofox-stage-mpcliffcommon2-loop-harness.ps1`:
  full guarded VS Mode -> PlayersVS -> Maps -> VSBattle path plus the same
  CliffAttackQuick2 common2 tick proof.
- `scripts/verify-battle-mariofox-stage-mpcliffescape-action-loop-harness.ps1`:
  direct bounded stage MP proof that starts from CliffWait, injects a Z-button
  tap, selects original CliffEscape, and advances CliffQuick through
  CliffEscapeQuick1 into CliffEscapeQuick2 setup.
- `scripts/verify-menu-chain-mariofox-stage-mpcliffescape-action-loop-harness.ps1`:
  full guarded VS Mode -> PlayersVS -> Maps -> VSBattle path plus the same
  CliffWait -> CliffEscapeQuick2 action proof.
- `scripts/verify-battle-mariofox-stage-mpcliffescape-common2-loop-harness.ps1`:
  direct bounded stage MP proof that consumes the created CliffEscapeQuick2
  state through one original common2 update, physics, and map tick.
- `scripts/verify-menu-chain-mariofox-stage-mpcliffescape-common2-loop-harness.ps1`:
  full guarded VS Mode -> PlayersVS -> Maps -> VSBattle path plus the same
  CliffEscapeQuick2 common2 tick proof.
- `scripts/verify-battle-mariofox-stage-mpcliffclimb-floor-loop-harness.ps1`:
  direct bounded stage MP proof that starts from CliffWait, reaches the
  original attack/escape/climb-fall interrupt chain, and proves climb to
  CliffQuick plus drop to Fall on real Pupupu `cliff_id=3`.
- `scripts/verify-menu-chain-mariofox-stage-mpcliffclimb-floor-loop-harness.ps1`:
  full guarded VS Mode -> PlayersVS -> Maps -> VSBattle path plus the same
  bounded CliffWait climb/fall interrupt proof.
- `scripts/verify-battle-mariofox-stage-mpcliffclimb-action-loop-harness.ps1`:
  direct bounded stage MP proof that consumes the climb-created CliffQuick
  state and advances through original CliffClimbQuick1 and CliffClimbQuick2
  setup on real Pupupu `cliff_id=3`.
- `scripts/verify-menu-chain-mariofox-stage-mpcliffclimb-action-loop-harness.ps1`:
  full guarded VS Mode -> PlayersVS -> Maps -> VSBattle path plus the same
  bounded CliffQuick -> CliffClimbQuick1 -> CliffClimbQuick2 proof.
- `scripts/verify-battle-mariofox-stage-mpcliffclimb-common2-loop-harness.ps1`:
  direct bounded stage MP proof that consumes the created CliffClimbQuick2
  state through one original common2 update, physics, and map tick.
- `scripts/verify-menu-chain-mariofox-stage-mpcliffclimb-common2-loop-harness.ps1`:
  full guarded VS Mode -> PlayersVS -> Maps -> VSBattle path plus the same
  CliffClimbQuick2 common2 tick proof.
- `scripts/verify-battle-mariofox-stage-mpcliffclimb-finish-loop-harness.ps1`:
  direct bounded stage MP proof that consumes the CliffClimbQuick2 common2
  state through the original animation-end handoff into Wait/Ground.
- `scripts/verify-menu-chain-mariofox-stage-mpcliffclimb-finish-loop-harness.ps1`:
  full guarded VS Mode -> PlayersVS -> Maps -> VSBattle path plus the same
  CliffClimbQuick2 finish handoff proof.
- `scripts/verify-battle-mariofox-stage-mpcliffwait-damage-loop-harness.ps1`:
  direct bounded stage MP proof that consumes the verified CliffWait/Ground
  state, forces the original fall-wait timeout branch, and reaches
  DamageFall/Air while clearing cliff hold, then proves one original
  DamageFall no-collision map tick, one positive right-cliff map branch into
  CliffCatch/Air `84/72/1`, one positive floor-collision branch into
  DownBounceU/Ground setup, two bounded original DownBounce updates, one
  bounded original DownWait stable update tick, and one bounded timeout tick
  into DownStandU/Ground with stale `proc_damage` cleared and `damage_mul`
  restored to `1.0`.
- `scripts/verify-menu-chain-mariofox-stage-mpcliffwait-damage-loop-harness.ps1`:
  full guarded VS Mode -> PlayersVS -> Maps -> VSBattle path plus the same
  CliffWait timeout -> DamageFall/DownBounce/DownWait/DownStand proof.
- `scripts/verify-battle-mariofox-stage-mppassive-loop-harness.ps1`:
  direct bounded stage MP proof that consumes the verified PassiveStand and
  Passive setup states, runs two guarded stable update/physics/map frames for
  each branch, then proves the original animation-end handoff into Wait/Ground.
- `scripts/verify-menu-chain-mariofox-stage-mppassive-loop-harness.ps1`:
  full guarded VS Mode -> PlayersVS -> Maps -> VSBattle path plus the same
  PassiveStand/Passive stable callback and Wait handoff proof.
- `scripts/verify-battle-mariofox-stage-mpdownwait-loop-harness.ps1`:
  direct bounded stage MP proof that consumes the Passive-loop proof, runs a
  bounded original DownWait interrupt branch set, and proves source order
  `0x12345` into DownStandU/Ground plus guarded original DownAttackU,
  DownForwardU, and DownBackU setup branches, DownAttackU attack IDs
  `53/33/33`, eight guarded stable callback frames for each attack/roll
  branch, bounded roll root movement `+10000/-10000` milli, one guarded
  original DownStand interrupt tick through KneeBend/Pass/Dokan checks,
  guarded DownStand callback frames, and
  animation-end handoff back to Wait/Ground.
- `scripts/verify-menu-chain-mariofox-stage-mpdownwait-loop-harness.ps1`:
  full guarded VS Mode -> PlayersVS -> Maps -> VSBattle path plus the same
  DownWait interrupt branch proof.
- `scripts/verify-all.ps1`: maintained regression chain.

Shared verifier helpers:

- `scripts/lib/melonds.ps1`
- `scripts/lib/gdb-markers.ps1`

## Emulator Policy

Use melonDS for automated pass/fail verification. Use no$gba only when melonDS
cannot answer a DS hardware question such as VRAM, OAM, palettes, BG/3D
registers, DMA, or timing. no$gba currently has smoke/window-capture automation
only, not runtime-global verification.

## Next Best Work

1. Use the bounded Mario/Fox MP DownWait interrupt proof,
   PassiveStand/Passive two-frame callback proof, CliffWait timeout/DamageFall/
   CliffCatch/DownBounce/DownWait/DownStand proof, and same-cliff occupancy
   blocker to choose the next narrow original fighter/stage spine boundary.
   Good candidates are bounded post-record hit interaction/status
   bookkeeping after the new selected damage-record insertion,
   continuous DownAttack/DownForward/DownBack/DownStand
   recovery action runtime after the proven bounded stable-frame Wait
   handoffs, broader
   natural ledge
   occupancy/release/drop/climb behavior, dedicated ledge
   drop/climb action follow-through, broader PassiveStand/Passive runtime
   beyond the current two stable callback frames and anim-end-to-Wait handoff, a
   real wall-hit floor-edge adjustment on a different geometry case,
   platform/ledge/wall/ceiling contracts, or a small original fighter-map
   callback step without unbounding gameplay.
2. Inspect the relevant BattleShip source and headers first.
3. Inspect `decomp/sm64-nds` only for DS backend architecture patterns.
4. Add project-owned shims in `src/port`, `src/nds`, or `include`.
5. Keep the source boundary bounded; do not import broad fighter/stage/audio or
   full renderer systems unless that exact boundary requires them.
6. Extend the smallest verifier that proves the new boundary.
7. Update `docs/STATUS.md`, this handoff, and `docs/PORTING.md`.

## Reference Docs

- `docs/STATUS.md`: short current-truth summary.
- `docs/ROADMAP.md`: milestone status and next gates.
- `docs/KNOWN_ISSUES.md`: stubs, warnings, and risks.
- `docs/GOAL_DEBUGGING.md`: short debug workflow.
- `docs/DIAGNOSTIC_REFERENCE.md`: detailed marker inventory.
- `docs/ARCHITECTURE.md`: subsystem boundaries.
- `docs/DECOMP_MAP.md`: read-only upstream context map.
- `docs/EMULATOR_STRATEGY.md`: melonDS/no$gba choice.
- `docs/PORTING.md`: append-only history.
