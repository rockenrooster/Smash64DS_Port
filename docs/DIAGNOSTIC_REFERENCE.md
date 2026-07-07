# Diagnostic Reference

This file preserves the detailed diagnostic-global inventory, marker meanings,
manual GDB notes, and historical verifier details. For day-to-day debugging
workflow, use `docs/GOAL_DEBUGGING.md`.

## Build Environment

Use devkitPro/libnds. On this machine, the known-good PowerShell setup is:

```powershell
$env:DEVKITPRO = 'C:/devkitPro'
$env:DEVKITARM = 'C:/devkitPro/devkitARM'
make -j16
```

Clean build:

```powershell
make clean
make -j16
```

Generated outputs:

- `build/`
- `smash64ds.elf`
- `smash64ds.nds`
- `smash64ds.ds.gba`

Do not edit generated output.

## Emulator Layout

Local emulator binaries and configs live under `emulators/`:

```text
emulators/melonds/melonDS.exe
emulators/melonds/melonDS.toml
emulators/nogba/NO$GBA.EXE
```

Generated emulator stdout/stderr logs are written under
`artifacts/emulator-logs/`. Emulator binaries/configs are ignored by Git; the
repo tracks only scripts and layout docs.

## Runtime Verification

With `emulators/melonds/melonDS.exe` in the workspace:

```powershell
.\scripts\verify-runtime.ps1
.\scripts\verify-opening-skip.ps1
.\scripts\verify-opening-boundary.ps1
.\scripts\verify-title-boundary.ps1
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
.\scripts\verify-battle-mariofox-dash-run-harness.ps1
.\scripts\verify-battle-mariofox-jump-loop-harness.ps1
.\scripts\verify-battle-mariofox-landing-loop-harness.ps1
.\scripts\verify-battle-mariofox-process-loop-harness.ps1
.\scripts\verify-battle-mariofox-scheduler-loop-harness.ps1
.\scripts\verify-battle-mariofox-controller-loop-harness.ps1
.\scripts\verify-battle-mariofox-preview-loop-harness.ps1
.\scripts\verify-battle-mariofox-gcrunall-loop-harness.ps1
.\scripts\verify-battle-mariofox-live-preview-harness.ps1
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
.\scripts\verify-menu-chain-mariofox-dash-run-harness.ps1
.\scripts\verify-menu-chain-mariofox-jump-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-landing-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-process-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-scheduler-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-controller-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-preview-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-gcrunall-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-live-preview-harness.ps1
.\scripts\verify-regression.ps1
```

Use `verify-opening-boundary.ps1` as the quick current Opening Room progress
gate, `verify-title-boundary.ps1` for the natural movie-to-Title speed gate,
`verify-title-harness.ps1` for the direct imported Title boundary without
Opening Room/movie replay, `verify-vs-setup-harness.ps1` for the direct
bounded imported VS Mode setup proof from Title,
`verify-vs-start-transition-harness.ps1` for the bounded original VS Start to
PlayersVS transition proof, `verify-players-vs-setup-harness.ps1` for bounded
imported PlayersVS setup, `verify-maps-setup-harness.ps1` for bounded imported
Maps setup, `verify-menu-chain-vsbattle-harness.ps1` for the guarded VS Mode ->
PlayersVS -> Maps -> imported bounded VSBattle setup proof,
`verify-battle-fd-harness.ps1` for the direct one-Mario bounded VSBattle setup
proof, `verify-battle-pupupu-stage-harness.ps1` for direct Pupupu/Dream Land
stage-data adoption and original ground GObj setup,
`verify-battle-pupupu-update-harness.ps1` for the direct guarded two-tick
original Pupupu update proof, and
`verify-menu-chain-pupupu-update-harness.ps1` for the same update proof after
the VS Mode -> PlayersVS -> Maps -> VSBattle chain,
`verify-battle-mariofox-model-harness.ps1` for the direct asset-backed
Mario/Fox model GObj proof, and
`verify-menu-chain-mariofox-model-harness.ps1` for the same model proof after
the VS Mode -> PlayersVS -> Maps -> VSBattle chain,
`verify-battle-mariofox-struct-harness.ps1` for the direct persistent
FTStruct-backed Mario/Fox state proof, and
`verify-menu-chain-mariofox-struct-harness.ps1` for the same FTStruct proof
after the VS Mode -> PlayersVS -> Maps -> VSBattle chain,
`verify-battle-mariofox-init-harness.ps1` for the direct bounded Mario/Fox
source-order init-state proof, and
`verify-menu-chain-mariofox-init-harness.ps1` for the same init-state proof
after the VS Mode -> PlayersVS -> Maps -> VSBattle chain,
`verify-battle-mariofox-wait-harness.ps1` for the direct imported original
Mario/Fox Wait status/motion setup proof, and
`verify-menu-chain-mariofox-wait-harness.ps1` for the same Wait proof after
the VS Mode -> PlayersVS -> Maps -> VSBattle chain,
`verify-battle-mariofox-wait-tick-harness.ps1` for the direct bounded original
Mario/Fox Wait callback tick proof, and
`verify-menu-chain-mariofox-wait-tick-harness.ps1` for the same Wait callback
tick proof after the VS Mode -> PlayersVS -> Maps -> VSBattle chain,
`verify-battle-mariofox-wait-ground-harness.ps1` for the direct bounded
source-order ground-friction/air-transfer and safe floor-map proof, and
`verify-menu-chain-mariofox-wait-ground-harness.ps1` for the same ground proof
after the VS Mode -> PlayersVS -> Maps -> VSBattle chain, and
`verify-battle-mariofox-display-probe-harness.ps1` for the direct bounded
Mario/Fox display metadata callback probe, and
`verify-menu-chain-mariofox-display-probe-harness.ps1` for the same display
metadata proof after the VS Mode -> PlayersVS -> Maps -> VSBattle chain,
`verify-battle-mariofox-dl-scan-harness.ps1` for the direct parser-only
Mario/Fox display-list scan proof, and
`verify-menu-chain-mariofox-dl-scan-harness.ps1` for the same DL scan proof
after the VS Mode -> PlayersVS -> Maps -> VSBattle chain,
`verify-battle-mariofox-dl-execute-harness.ps1` for the direct decode-only
Mario/Fox display-list execute proof, and
`verify-menu-chain-mariofox-dl-execute-harness.ps1` for the same execute proof
after the VS Mode -> PlayersVS -> Maps -> VSBattle chain,
`verify-battle-mariofox-dl-draw-harness.ps1` for the direct visible first-DL
Mario/Fox software draw proof,
`verify-menu-chain-mariofox-dl-draw-harness.ps1` for the same draw proof after
the VS Mode -> PlayersVS -> Maps -> VSBattle chain, and
`verify-battle-mariofox-dash-run-harness.ps1` /
`verify-menu-chain-mariofox-dash-run-harness.ps1` for the direct and
menu-chain bounded original Dash -> Run -> RunBrake movement proofs. The
Dash/Run verifiers read `DASH_RUN`, `DASH_RUN_STATUS`, `DASH_RUN_CALLS`,
`DASH_RUN_ATTACK11`, `DASH_RUN_ATTACK12`, `DASH_RUN_ATTACK13`,
`DASH_RUN_ATTACK100`, `DASH_RUN_ATTACK100_LOOP`, `DASH_RUN_ATTACK`,
`DASH_RUN_ATTACK_ANIM`, `DASH_RUN_ATTACK_EVENT`, `DASH_RUN_GUARD`,
`DASH_RUN_TURNRUN`, `DASH_RUN_ESCAPE`, `DASH_RUN_PROCPARAMS_RUMBLE`, and
`DASH_RUN_MOVE` GDB marker
groups from
`gNdsFighterMarioFoxDashRun*` and
`gNdsFighterDashRun*` diagnostics. The
`DASH_RUN_ATTACK11` group proves the bounded Wait A-tap branch reaches
original `ftCommonAttack1CheckInterruptCommon`, enters Attack11 status/motion
`190/165`, installs the original Attack11 update/interrupt callback slots,
uses the bounded physics/map callbacks, and ticks those four callback slots
once per fighter (`callback=0xff`, `tick=0xff`, `waitproc=0x3`). The
`DASH_RUN_ATTACK12` group proves the original Attack11 follow-up gate sets
`is_goto_followup`, reaches Attack12 status/motion `191/166`, and installs the
bounded Attack12 callback slots (`callback=0xff`, `goto=0xf`). The
`DASH_RUN_ATTACK13` group proves the original Attack12 follow-up gate reaches
Mario's fighter-specific Attack13 status/motion `220/195`, installs the
Mario Attack13 callback shape, and keeps Fox blocked at Attack12 because the
original `ftCommonAttack13CheckFighterKind` macro excludes Fox
(`callback=0xf`, `goto=0x7`). The
`DASH_RUN_ATTACK100` group proves the installed original Attack12 callbacks
reach original `ftCommonAttack100StartCheckInterruptCommon`, arm Fox's
`is_goto_attack100` gate, and enter Fox Attack100Start status/motion `220/195`
through `ftCommonAttack100StartSetStatus` / `ftMainSetStatus`
(`check/setstatus/ftmain=5/1/1`, `callback=0xf`, `goto=0x3`). The
`DASH_RUN_ATTACK100_LOOP` group proves the installed original
Attack100Start update callback reaches its animation-end handoff into Fox
Attack100Loop status/motion `221/196` with callback mask `0xf` and goto mask
`0x3`; its first counter is expected to be `0` because the imported
translation unit calls the macro-renamed base Loop status function internally,
while the `ftMainSetStatus` count is `1`. Its tick mask is `0xfff` for one
guarded original Attack100Loop/End proof: A-input sets loop intent, update
marks animation end, consumes `motion_vars.flags.flag1`, clears loop intent,
remains in Fox Attack100Loop, then a no-input loop update reaches Fox
Attack100End and the installed End update returns through `ftAnimEndSetWait`
to Wait/Ground. The `DASH_RUN_ATTACK_ANIM` marker is expected to be `0x3f`;
bits cover original Attack11 for both players, Attack12 for both players,
Mario Attack13, and Fox Attack100Start reaching the project-owned
`ftMainPlayAnimEventsAll` seam. AttackDash is intentionally not included
because original `ftCommonAttackDashSetStatus` does not call
`ftMainPlayAnimEventsAll`. The `DASH_RUN_ATTACK_EVENT` marker is expected to
be `0x1f/0x3f/0x20/10`: ten selected original Mario/Fox main-motion
`MakeAttackColl` commands decoded, six selected scripts reached, and Fox
Attack100Start proven as a real no-hit script. The currently last decoded
source-order hitbox is Fox Jab2 attack ID `1` (`damage=4`, `size=100`, `offset=0/0/0`,
`angle=70`, `KBG/KBW/BKB=100/0/0`, `flags=0x7`). This marker does not mean
full animation command runtime, hitbox activation, or hit detection is live.
The companion `DASH_RUN_ATTACK_EVENT_CMDS` marker is expected to be `0xf`:
bit `0x1` means Mario Jab3 `SetAttackCollDamage` was applied, bit `0x2` means
`SetAttackCollSize` was applied, bit `0x4` means `ClearAttackCollAll` was
reached through `ftParamClearAttackCollAll`, and bit `0x8` means the selected
attack-collision state was left off after the clear. This bounded proof uses
the raw command damage value; original stale-damage handling remains deferred.
`DASH_RUN_ATTACK_EVENT_POS` proves the next bounded source-order slice on the
selected Fox Jab2 hitbox: `New -> Transfer`, gated writeback into
`FTStruct.attack_colls[1]`, then `Transfer -> Interpolate` with
`pos_prev = old pos_curr`, then a bounded original-compatible
`gmCollisionCheckAttackInFighterRange` broad-phase probe against the opposing
fighter's original `hit_detect_range`, plus a bounded original-compatible
rectangle probe and selected attack/damage collision decision against the
selected descriptor-backed Mario hurtbox, selected damage-record insertion,
and a bounded normal-hit front-half damage/hit-log bookkeeping step, then
select the original hit-collision FGM table entry, reach the existing audio
stub seam, run a selected fighter-hitlog stats handoff, and run the selected
`ftMainProcParams` damage/hitlag scheduling handoff, including selected
attacker-side `attack_damage` / `proc_hit` and `attack_shield_push` /
`proc_shield` branches plus the selected victim `shield_damage` ->
GuardSetOff, shield-break -> ShieldBreakFly, reflect-damage break, Fox
reflector hit, Ness reflector sound, Ness absorb branches, the first bounded
public-wrapper imported-original `ftCommonDamageUpdateMain`
catch-resist/keep-hold branch, and
the bounded `ftcommondamage.c` damage-status selector/setup/proc-passive
invincible and electric-status dispatch/lag-update/lifecycle side probes.
The marker also appends the attack-event decoder's hitbox-ID mask; the current
proof expects low bits `0x3`, meaning the selected original script scan decoded
at least attack-coll slots `0` and `1`. That is decoder coverage only, not full
continuous multi-hitbox collision runtime.
Current proof expects mask `0x3fffffff`, state `3`, attack ID `1`,
joint ID `14`, and matrix values `0/0`. This is not continuous hitbox
activation, full multi-slot hurtbox collision, continuous rehit gameplay beyond
the selected Link down-air timer window, hitlag,
full damage status lifecycle, or real DS audio yet.
`DASH_RUN_PROCPARAMS` records the selected attacker/victim scheduling handoff:
mask low bits `0xffffffff`, damage before/after, queued damage, queued hitlag
damage, computed victim hitlag, knockback pause flag, and status before/after.
The verifier requires queued damage to increase percent damage, positive
queued lag and hitlag, `is_knockback_paused = TRUE`, unchanged status, and
the first public-wrapper imported-original `ftCommonDamageUpdateMain`
catch-resist/keep-hold branch copying
damage lag/hitlag to the grabbed fighter and selecting `nFTDamageKindColAnim`,
plus the sibling catch-resist release branch selecting `nFTDamageKindStatus`
and routing through the bounded damage-status install, plus the catch-side
non-resist keep-hold branch that updates grabbed fighter damage stats before
the same bounded damage-status path, plus the first
capture keep-hold branch copying damage lag/hitlag back to the captured
fighter and selecting `nFTDamageKindCatch` on the captor, plus the
public-wrapper imported-original zero-knockback catch branch reaching the
damage color-animation seam, plus the
capture zero-knockback branch setting captor hitlag, clearing captured fighter
tap/release input, running `proc_lagstart`, and reaching the same color-animation
seam, plus the capture-side non-resist keep-hold branch that updates captured
fighter damage stats before the same bounded damage-status path, plus the
capture-side keep-hold false branch that routes through lose-grip into the
same bounded damage-status path without updating thrown damage stats, plus the
capture-side keep-hold false zero-knockback branch that routes the captor
through imported original no-damage release damage-var setup, plus the
catch-side non-resist zero-knockback branch that routes the grabbed fighter
through imported original damage-release setup before clearing the catch link,
plus the no-grab/no-capture tail colanim and damage-status branches, plus the
DK-family heavy-item catch-resist/drop, explicit heavy-item branch marker,
and held-item bypass branches, plus the
bounded no-grab/no-capture Sleep-element FuraSleep dispatcher branch.
`DASH_RUN_PROCPARAMS_RUMBLE` records the bounded source-shaped
`ftMainProcParams` attacker-side `attack_damage` rumble proof plus the selected
`attack_shield_push` + `attack_rebound` promotion proof: mask, captured call
count, last rumble ID, and last rumble length. Current proof expects mask bits
`0x7f`, derived `procRebound=0x1f`, at least two captured calls, and final
ID/length `10/0`, proving both the normal damage-derived rumble branch, the
original BatSwing4 special-case branch through the project-owned
`ftParamMakeRumble` seam, and the imported-original ReboundWait
status/callback/vector/hitlag/clear effects.
`DASH_RUN_DAMAGE_SLEEP` records that dispatcher branch: mask, status
before/after, motion after, and color-animation delta. The verifier expects
mask low bits `0x7f`, FuraSleep status/motion `165/145`, nonzero
color-animation delta, cliff-catch wait setup, and proof-local restore.
`DASH_RUN_DAMAGE_DUST` records the bounded public-wrapper
`ftCommonDamageSetDustEffectInterval` threshold proof: mask and packed wait
values. The verifier expects mask low bits `0xff` and packed waits
`0x123580`, proving the low, mid-low, mid, mid-high, high, default air dust
interval buckets, public imported-original routing, and proof-local restore.
`DASH_RUN_DAMAGE_DUST_UPDATE` records the bounded imported-original
`ftCommonDamageUpdateDustEffect` public-wrapper runtime proof: mask,
DustExpandLarge effect count, and post-reset wait. The verifier expects mask
low bits `0x1f`, effect count `1`, and wait `5`, proving a nonzero timer
decrement without spawning, the zero-cross spawn path, interval reset, and
proof-local field/counter restore plus public imported-original routing.
`DASH_RUN_DAMAGE_HITSTUN_PUBLIC` records the bounded imported-original
`ftCommonDamageDecHitStunSetPublic` public-wrapper proof: mask, hitstun after
zero-cross, and public-knockback milli. The verifier expects mask low bits
`0xf`, hitstun `0`, and public knockback `456000`, proving a nonzero hitstun
decrement, zero-cross public-knockback transfer, proof-local field restore,
and public imported-original routing.
`DASH_RUN_DAMAGE_COLANIM` records the bounded imported-original
`ftCommonDamageCheckElementSetColAnim` public-wrapper proof: mask, packed route IDs, and
routed count. The verifier expects mask low bits `0x3f`, packed IDs
`0x0522020e`, and count `4`, proving Fire, Electric, Freezing, default
damage color-animation routes, proof-local restore, and imported-original
routing through the public seam.
`DASH_RUN_DAMAGE_COLANIM_UPDATE` records the bounded imported-original
`ftCommonDamageUpdateDamageColAnim` / `ftCommonDamageSetDamageColAnim` public
wrapper proof: mask, packed direct/set route IDs, and update count. The verifier
expects mask low bits `0x1f`, packed IDs `0x020e`, and count `2`, proving the
direct wrapper, struct-field wrapper, gated no-update path, and proof-local
restore plus imported-original routing.
`DASH_RUN_DAMAGE_INVINCIBLE` records the bounded imported-original
`ftCommonDamageCheckSetInvincible` public-seam gate proof: mask, final
invincible tic count, and final hitstatus. The verifier expects mask low bits
`0x1f`, at least one invincible tic, and hitstatus `2`, proving the hitlag
gate, knockback-over flag gate, timed-invincibility true branch, proof-local
restore, and imported-original routing through the public seam.
`DASH_RUN_DAMAGE_LAGUPDATE` records the bounded imported-original
`ftCommonDamageCommonProcLagUpdate` proof: mask and Smash DI root delta in
milliunits. The verifier expects mask low bits `0x3f`, positive X delta, and
zero Y delta, proving the hitlag gate, stick-range gate, tap-buffer gate,
active Smash DI root translation branch, proof-local restore, and
imported-original routing.
`DASH_RUN_DAMAGE_COMMON_PHYSICS` records the bounded imported-original
`ftCommonDamageCommonProcPhysics` proof: mask, ground-friction X velocity,
air-friction X velocity, air-drift Y velocity, and attack-clear state. The
verifier expects mask low bits `0x3f`, reduced nonzero ground/air-friction X
velocity, negative air-drift Y velocity, cleared attack state `0`, and
proof-local restore plus imported-original routing.
`DASH_RUN_DAMAGE_COMMON_CALLBACKS` records bounded common damage callback
routing: imported-original ground and air update stay/expiry, imported-original
common interrupt ground/fall/hammer branches, imported-original AirCommon
interrupt, and proof-local restore. The verifier expects mask low bits
`0x3fff`.
`DASH_RUN_DAMAGE_LEVELS` records bounded imported-original
`ftCommonDamageGetDamageLevel` public-wrapper threshold routing. The verifier expects mask
low bits `0x1f`, proving low/mid/high/fly levels at hitstun `0`, `12`, `24`,
and `32` plus imported-original routing through the public seam.
`DASH_RUN_DAMAGE_HOLD_RESIST` records bounded public-wrapper
`ftCommonDamageCheckCatchResist` and `ftCommonDamageCheckCaptureKeepHold`
gate coverage. The verifier expects mask low bits `0xff`, proving Sleep
blocks catch-resist, zero knockback and paused low-stack knockback resist,
Donkey cargo throw low-level damage resists, default high knockback does not
resist, capture keep-hold true/false thresholds, public imported-original
routing, and proof-local restore.
`DASH_RUN_DAMAGE_UPDATE_CATCH_RESIST` records bounded public-wrapper
`ftCommonDamageUpdateCatchResist` branch coverage. The verifier expects mask
low bits `0x1f`, proving the zero-knockback color-animation branch through
imported original BattleShip code, paused low-stack color-animation, the
non-resist status-dispatch side seam, and proof-local restore.
`DASH_RUN_DAMAGE_AIR_MAP_WALL` records the bounded DamageAir wall-map
short-circuit proof. The verifier expects mask low bits `0x3f`, proving wall
collision, original WallDamage helper side effects, Passive/DownBounce
short-circuit, reflected knockback/LR, imported-original
`ftCommonDamageAirCommonProcMap` routing, and proof-local restore.
`DASH_RUN_DAMAGE_FALL_INTERRUPT` records the bounded imported
`ftCommonDamageFallProcInterrupt` source-order proof. The verifier expects
mask low bits `0x3f`, proving the imported interrupt call reaches special-air,
attack-air, jump-aerial, and hammer fallback checks, then restores local state.
`DASH_RUN_DAMAGE_SCREEN_FLASH` records the bounded imported-original
`ftCommonDamageCheckMakeScreenFlash` public-wrapper proof: mask, packed routed colanim IDs,
and routed count. The verifier expects mask low bits `0x7f`, packed IDs
`0x3a3d3c3b`, and count `4`, proving the low-knockback no-op plus
high-knockback Fire, Electric, Freezing, and default screen-flash routes plus
imported-original routing through the public seam.
`DASH_RUN_DAMAGE_PUBLIC` records the bounded imported-original
`ftCommonDamageSetPublic` public-wrapper reaction proof: mask, public-knockback milli,
and force-count. The verifier expects mask low bits `0x3f`, knockback
`160000`, and force count `1`, proving the 75-115 degree knockback reduction,
target public-knockback reset, very-high attacker force handoff, default
non-forced attacker branch, proof-local field restore, and imported-original
routing through the public seam.
`DASH_RUN_DAMAGE_VOICE` records the bounded source-shaped
`ftCommonDamageInitDamageVars` damage voice branch: mask, captured call count,
threshold-branch FGM, and forced-call FGM. The verifier expects mask low bits
`0xf`, at least two captured calls, and distinct threshold/forced IDs, proving
both original voice-call gates through the project-owned audio stub seam.
`DASH_RUN_DAMAGE_FLYTOP` records the bounded source-shaped
`ftCommonDamageInitDamageVars` deterministic FlyTop branch: mask, selected
status, selected motion, and angle. The verifier expects mask low bits `0xf`,
DamageFlyTop status/motion `54/47`, and angle `90`, proving the airborne
high-knockback angle-window selection branch and proof-local state restore.
`DASH_RUN_DAMAGE_REPLACE_ELECTRIC` records the bounded source-shaped
`ftCommonDamageInitDamageVars` status replacement plus electric wrapping
branch: mask, installed status, stored replacement status, installed motion,
dispatch status, and dispatch motion. The verifier expects mask low bits
`0x3f`, electric status/motion `50/43`, stored replacement status `55`, and
dispatch status/motion `55/48`, proving the original final replacement
override is preserved through the electric damage wrapper, the selected
passive callback stays blocked while hitlag remains, and zero-hitlag dispatch
reaches the stored replacement before proof-local state restore.
`DASH_RUN_DAMAGE_FLYROLL` records the bounded source-shaped
`ftCommonDamageInitDamageVars` random FlyRoll branch: mask, selected status,
selected motion, and percent. The verifier expects mask low bits `0x1f`,
DamageFlyRoll status/motion `55/48`, and percent at least `100`, proving the
airborne non-FlyTop percent/RNG selection branch and proof-local RNG/state
restore.
`DASH_RUN_DAMAGE_KIRBYCOPY` records the bounded source-shaped
`ftCommonDamageInitDamageVars` Kirby copy-loss branch: mask, copy ID before,
copy ID after, and FGM. The verifier expects mask bits `0x6`, Fox copy `1`
reset to Kirby `8`, and FGM `204`, proving the selected copy-loss reset/audio
seam without importing full Kirby effect/model-part runtime.
`DASH_RUN_DAMAGE_ITEM_HEAVY` records the bounded source-shaped
`ftCommonDamageUpdateMain` DK-family heavy-item branch. The verifier expects
mask bits `0x1f`, proving the heavy-item predicate, catch-resist return,
drop/status return, and proof-local restore.
`DASH_RUN_DAMAGE_ITEM_BYPASS` records the bounded source-shaped
`ftCommonDamageUpdateMain` item-bypass fallthrough proof. The verifier expects
mask bits `0x1f`, proving a light held item and a heavy item held by a non-DK
fighter skip the heavy-item branch, keep the item attached, reach the normal
damage color-animation tail via the public imported-original colanim wrapper
update path, and restore local state.
`DASH_RUN_DAMAGE_KIND` records the bounded source-fidelity guard for
`ftCommonDamageInitDamageVars` plus the selected `ftMainProcParams`
Twister/proc_trap branch. The verifier expects mask bits `0x7f`, proving
`FTStruct.damage_kind` is preserved before/after hit-element setup and after
proof-local restore, imported original init/goto routing is reached, Twister
forces `nFTDamageKindColAnim`, and the installed `proc_trap` callback runs in
BattleShip source order.
`DASH_RUN_DAMAGE_STATUS` records the bounded original-compatible
`ftcommondamage.c` selector side probe: mask low bits `0x1f`, damage level,
damage index, selected ground status, selected air status, and selected
electric status. The proof mirrors BattleShip's damage-level thresholds and
ground/air status tables while requiring the initial selected scheduling path
to leave `status_id` parked.
`DASH_RUN_DAMAGE_KNOCKBACK_ANGLE` records the bounded imported-original
`ftCommonDamageGetKnockbackAngle` public-wrapper branch probe. The verifier expects mask low
bits `0x3f`, proving fixed-angle, air `361`, ground low-knockback `361`,
ground high-knockback scaled `361`, capped ground `361`, and
imported-original routing branches through the public seam.
`DASH_RUN_DAMAGE_SETUP` records the guarded damage-status setup side proof:
mask low bits `0xffffffff`, status before/after, installed motion, installed
ground/air state, hitstun before/after one update tick plus ground
DamageCommon expiry into Wait/Ground, seeded damage velocity,
post-physics velocity, original angle-derived knockback vector routing,
original-compatible `proc_passive`
ownership, a bounded `ftMainProcUpdateInterrupt`-shaped
`ftCommonDamageCheckSetInvincible` passive dispatch tick, a bounded
`ftCommonDamageSetStatus` electric passive status dispatch tick through the
imported-original public seam, a bounded
sleep-element route through `ftCommonDamageGotoDamageStatus` into FuraSleep
status/motion plus `cliffcatch_wait`, imported original
`ftcommonfurasleep.c` breakout timer setup, one original FuraSleep update
tick, one A-tap plus stick mash breakout branch, forced original FuraSleep ->
Wait handoff, and the fighter color-animation seam, a bounded
DamageFlyRoll pitch/update-and-throw-clear physics tail through imported
original `ftCommonDamageFlyRollUpdateModelPitch` and original `syUtilsArcTan2`
math,
a bounded knockback-over status/invincibility branch, a bounded hitlag Smash DI lag-update branch, a bounded hitlag lifecycle tick through lag-update and lag-end callbacks, a bounded imported-original DamageAir map callback
into the floor/passive/DownBounce seams, and the selected DamageAir interrupt handoff
into the DamageFall interrupt seam plus the hitstun-expiry DamageFall status
handoff through imported-original `ftCommonDamageFallSetStatusFromDamage`
plus one installed DamageFall air-physics callback tick, one
original-shaped fast-fall branch through `ftPhysicsApplyFastFall`, guarded
DamageFall no-collision, floor-collision, and cliff-collision map ticks, plus
the restored setup-tail calls for public knockback, damage color animation,
screen flash, rumble, dust effect spawn/interval reset, player-tag wait, and attacker
count/knockback, plus the original damage interrupt hammer-held ground and
air branch routing through project-owned hammer callback stubs.
The proof uses the selected `ftcommondamage.c` status table result, lets the
new damage-status gate call `ftMainSetStatus`, installs original-named damage
callbacks, runs one update tick to decrement hitstun, proves the ground
DamageCommon expiry branch into Wait/Ground, runs one installed physics
callback tick, runs one interrupt handoff tick, runs one expiry update tick
through imported-original `ftCommonDamageFallSetStatusFromDamage` into
DamageFall status setup, runs one installed DamageFall
`ftPhysicsApplyAirVelDriftFastFall` tick, runs one guarded DamageFall map tick
through the safe no-collision branch, then runs one guarded floor-collision branch
that reaches passive checks and the DownBounce seam, then one guarded
cliff-collision branch that reaches the CliffCatch seam, runs the setup-tail
side proof, and restores the preview fighter state before the wider Dash-Run
proof continues.
The `DASH_RUN_GUARD` group proves a bounded Z-hold
from Wait reaches original `ftCommonGuardOnCheckInterruptCommon`, enters
GuardOn status/motion `152/134`, installs the expected update/interrupt/
physics/map callbacks (`callback=0xff`), records state mask `0xfffffe0f`, emits
GuardOn FGM `13`, records guarded shield-effect seam calls, proves one
bounded original GuardOn update tick that preserves GuardOn while advancing
shield decay/release counters, and proves the animation-end handoff through
original `ftCommonGuardSetStatus` into Guard status `153` with original Guard
callbacks. It now also proves one bounded original Guard hold update tick that
stays in Guard and advances shield counters again, plus one release tick
through original `ftCommonGuardOffSetStatus` into GuardOff status/motion
`154/135`, and one GuardOff animation-end update through original
`ftCommonWaitSetStatus` back to Wait/Ground without promoting continuous Guard
hold or full shield runtime. The
`DASH_RUN_GUARD_SETOFF` group proves a bounded original GuardSetOff branch.
It expects setstatus/ftmain counts `4/4`, mask `0xfff`, callback mask `0xff`,
deterministic setoff frames `20200` milli-units, and ground velocity `-40400`.
The mask covers both fighters entering status `155` with the original
`ftCommonGuardSetOffProcUpdate` slot, held Z returning to Guard, released Z
returning to GuardOff, and the proof restoring Guard before the existing
Escape branch. This is still not continuous shield collision or full
player-driven shield runtime. The
`DASH_RUN_ESCAPE` group proves a bounded held-stick branch from Guard reaches
original `ftCommonEscapeCheckInterruptGuard`, enters EscapeF/EscapeB
status/motion `156/136` and `157/137`, installs the expected
update/interrupt/physics/map/status callbacks (`callback=0x3ff`), records
state mask `0xff`, records `itemthrow_buffer_tics=5` for both fighters, and
cleans up the guarded escape seam without promoting full shield-roll runtime.
The
`DASH_RUN_ATTACK` group proves the bounded Run A-tap branch reaches original
`ftCommonAttackDashCheckInterruptCommon` through the installed
`ftCommonRunProcInterrupt` route (`runproc=0x3`), reaches AttackDash
status/motion `192/167`, plus callback mask `0xff` for
`ftAnimEndSetWait` / no interrupt / `ftPhysicsApplyGroundVelTransN` /
`mpCommonSetFighterFallOnEdgeBreak`, and tick mask `0x3f` for one guarded
update/physics/map callback tick per fighter before the harness restores Run,
and
`verify-battle-mariofox-jump-loop-harness.ps1` /
`verify-menu-chain-mariofox-jump-loop-harness.ps1` for the direct and
menu-chain bounded original RunBrake -> Wait -> KneeBend -> JumpF airborne
movement proofs. The Jump-loop verifiers read `JUMP_LOOP`, `JUMP_INPUT`,
`JUMP_STATUS`, `JUMP_GA`, `JUMP_CALLS`, `JUMP_FRAMES`, `JUMP_MOVE`,
`JUMP_VEL`, `JUMP_ATTACKAIR`, `JUMP_DEFER`, and `JUMP_SAFE` marker groups from
`gNdsFighterMarioFoxJumpLoop*` and `gNdsFighterJump*` diagnostics,
`verify-battle-mariofox-landing-loop-harness.ps1` /
`verify-menu-chain-mariofox-landing-loop-harness.ps1` for the direct and
menu-chain bounded original JumpF -> Fall -> LandingLight -> Wait proofs, and
`verify-battle-mariofox-process-loop-harness.ps1` /
`verify-menu-chain-mariofox-process-loop-harness.ps1` for the direct and
menu-chain bounded scripted fighter process-loop proof. The Process-loop
verifiers read `PROC_LOOP`, `PROC_INPUT`, `PROC_STATUS`, `PROC_VISITS`,
`PROC_CALLS`, `PROC_MOVE`, `PROC_VEL`, `PROC_TRANS`, and `PROC_SAFE` marker
groups from `gNdsFighterMarioFoxProcessLoop*` and
`gNdsFighterProcessLoop*` diagnostics, and
`verify-battle-mariofox-scheduler-loop-harness.ps1` /
`verify-menu-chain-mariofox-scheduler-loop-harness.ps1` for the direct and
menu-chain VSBattle update-driven `GObjProcess` scheduler-loop proof. The
Scheduler-loop verifiers read `SCHED_LOOP`, `SCHED_TASKMAN`,
`SCHED_PROCESS`, `SCHED_INPUT`, `SCHED_STATUS`, `SCHED_VISITS`,
`SCHED_CALLS`, `SCHED_MOVE`, `SCHED_TRANS`, and `SCHED_SAFE` marker groups
from `gNdsFighterMarioFoxSchedulerLoop*` and
`gNdsFighterSchedulerLoop*` diagnostics, and
`verify-battle-mariofox-controller-loop-harness.ps1` /
`verify-menu-chain-mariofox-controller-loop-harness.ps1` for the direct and
menu-chain controller-source-driven `GObjProcess` scheduler-loop proof. The
Controller-loop verifiers read `CTRL_LOOP`, `CTRL_BACKEND`, `CTRL_TASKMAN`,
`CTRL_PROCESS`, `CTRL_INPUT`, `CTRL_STATUS`, `CTRL_VISITS`, `CTRL_CALLS`,
`CTRL_MOVE`, `CTRL_TRANS`, and `CTRL_SAFE` marker groups from
`gNdsFighterMarioFoxControllerLoop*`, `gNdsFighterControllerLoop*`, and
`gNdsControllerPlayback*` diagnostics.
`verify-battle-mariofox-preview-loop-harness.ps1` /
`verify-menu-chain-mariofox-preview-loop-harness.ps1` for the direct and
menu-chain moving battle-preview proof. The Preview-loop verifiers read
`PREV_LOOP`, `PREV_BACKEND`, `PREV_TASKMAN`, `PREV_PROCESS`, `PREV_INPUT`,
`PREV_STATUS`, `PREV_CALLS`, `PREV_MOVE`, `PREV_DRAW`, `PREV_SCREEN`,
`PREV_TRANS`, and `PREV_SAFE` marker groups from
`gNdsFighterMarioFoxPreviewLoop*`, `gNdsFighterPreviewLoop*`, and
`gNdsControllerPlayback*` diagnostics. Expected current proof: seven committed
`96x72` preview frames, `14/18` DL-ready Mario/Fox DObj candidates, `582`
total preview pixels, screen X movement `61/-62`, and screen rise `52/52`.
`check-gbi-decode-fixtures.ps1` verifies the shared F3DEX2 decode helpers for
`gSPVertex`, `gSP1Triangle`, and `gSP2Triangles` and scans the active renderer
paths for the old ad hoc VTX/TRI1 snippets. `verify-all.ps1` runs this fixture
before the maintained profile chain. The shared PowerShell helpers live in
`scripts/lib/melonds.ps1` and `scripts/lib/gdb-markers.ps1`.

The script:

1. ensures melonDS has a config under `emulators/melonds/`
2. enables the ARM9 GDB stub (`Enabled = true`, plus the melonDS 1.1
   compatibility key `Enable = true`) without duplicating either key in
   `[Instance0.Gdb]`
3. launches the ROM hidden
4. waits for the melonDS ARM9 listener on port `3333`
5. connects `arm-none-eabi-gdb` to `127.0.0.1:3333`
6. reads diagnostic globals from `smash64ds.elf`
7. restores or removes the temporary melonDS config

The verifier is intentionally stronger than "the ROM opened." It checks that
specific original systems ran.

`verify-opening-skip.ps1` attaches while the movie is advancing, breaks at the
real `mvOpeningRoomFuncRun` callback after tick 9, injects N64 A into the real
BattleShip controller device, and continues to the Title parking call. It
verifies scene `28 -> 1`, object ejection, and the second taskman return.

`verify-title-harness.ps1` builds `TARGET=smash64ds-title
BUILD=build-title-harness NDS_DEV_SCENE_HARNESS=title`, starts the imported
scene manager from `nSCKindTitle` with `scene_prev =
nSCKindOpeningNewcomers`, verifies `gNdsOpeningRoomTickCount == 0`, and checks
the same bounded imported Title markers as the natural path.

`verify-vs-setup-harness.ps1` builds `TARGET=smash64ds-vs-setup
BUILD=build-vs-setup-harness NDS_DEV_SCENE_HARNESS=vs_setup`, starts the
imported scene manager from `nSCKindVSMode` with `scene_prev = nSCKindTitle`,
verifies Opening Room and Title setup did not replay, and checks the bounded
imported VS setup markers.

`verify-vs-start-transition-harness.ps1` builds `TARGET=smash64ds-vs-start
BUILD=build-vs-start-harness NDS_DEV_SCENE_HARNESS=vs_start_transition`,
starts from the same original `mnvsmode.c` setup boundary, advances bounded
original `mnVSModeMain`, injects a synthetic A tap through the original
controller globals, proves original `mnVSModeSaveSettings` and
`syTaskmanSetLoadScene` ran, and verifies the bounded imported PlayersVS
boundary was reached as `scene_curr/scene_prev = 16/9`.

`verify-players-vs-setup-harness.ps1` builds
`TARGET=smash64ds-players-vs BUILD=build-players-vs-setup-harness
NDS_DEV_SCENE_HARNESS=players_setup`, starts from `nSCKindPlayersVS` with
`scene_prev = nSCKindVSMode`, verifies Opening Room/Title/VS transition work
did not replay, and checks the bounded imported PlayersVS setup markers.

`verify-maps-setup-harness.ps1` builds `TARGET=smash64ds-maps
BUILD=build-maps-setup-harness NDS_DEV_SCENE_HARNESS=maps_setup`, starts from
`nSCKindMaps` with `scene_prev = nSCKindPlayersVS`, verifies the seeded
Pupupu/Dream Land cursor, and checks the bounded imported Maps setup markers.

`verify-menu-chain-vsbattle-harness.ps1` builds
`TARGET=smash64ds-menu-chain BUILD=build-menu-chain-vsbattle-harness
NDS_DEV_SCENE_HARNESS=menu_chain_vsbattle`, proves original VS Start ->
PlayersVS, bounded PlayersVS ready/start -> Maps, bounded Maps A-select ->
VSBattle, then verifies the imported bounded VSBattle setup boundary and final
`scene_curr/scene_prev = 22/21`.

`verify-battle-fd-harness.ps1` builds `TARGET=smash64ds-battle-fd
BUILD=build-battle-fd-harness NDS_DEV_SCENE_HARNESS=battle_fd`, starts
directly at `nSCKindVSBattle` from `nSCKindMaps`, seeds one Mario and the
current Final Destination sentinel, verifies the original common battle file
list load, default camera path, manager/interface compatibility stubs, active
fighter descriptor/stub-GObj creation, and one bounded
`scVSBattleFuncUpdate` interface tick.

`verify-battle-pupupu-stage-harness.ps1` builds
`TARGET=smash64ds-battle-pupupu BUILD=build-battle-pupupu-stage-harness
NDS_DEV_SCENE_HARNESS=battle_pupupu_stage`, starts directly at
`nSCKindVSBattle` from `nSCKindMaps`, seeds two players on Pupupu/Dream Land,
verifies real Pupupu `MPGroundData` adoption, and checks the bounded original
Dream Land ground GObj setup markers.

`verify-battle-pupupu-update-harness.ps1` builds
`TARGET=smash64ds-battle-pupupu-update
BUILD=build-battle-pupupu-update-harness
NDS_DEV_SCENE_HARNESS=battle_pupupu_update`, starts from the same direct
Pupupu VSBattle setup boundary, runs two guarded original
`grPupupuProcUpdate` calls in a deterministic safe substate, and verifies zero
wind/push/quake/particle side effects.

`verify-menu-chain-pupupu-update-harness.ps1` builds
`TARGET=smash64ds-menu-chain-pupupu-update
BUILD=build-menu-chain-pupupu-update-harness
NDS_DEV_SCENE_HARNESS=menu_chain_pupupu_update`, proves VS Mode -> PlayersVS
-> Maps -> VSBattle first, then verifies the same guarded Pupupu update
boundary.

`verify-battle-mariofox-init-harness.ps1` builds
`TARGET=smash64ds-battle-mariofox-init
BUILD=build-battle-mariofox-init-harness
NDS_DEV_SCENE_HARNESS=battle_mariofox_init`, starts directly at the Pupupu
VSBattle boundary, creates asset-backed Mario/Fox GObjs with persistent
`FTStruct` shells, runs the bounded source-order init helper, and verifies
damage/shield/GA/floor projection/deferred runtime markers.

With `-ImportBattleShipFTManager`, the same verifier builds
`TARGET=smash64ds-battle-mariofox-init-ftmanager
BUILD=build-battle-mariofox-init-ftmanager-harness
NDS_IMPORT_BATTLESHIP_FTMANAGER=1` and proves the fenced original
`ftmanager.c` Mario/Fox creation path without changing the default manager.

`verify-menu-chain-mariofox-init-harness.ps1` builds
`TARGET=smash64ds-menu-chain-mariofox-init
BUILD=build-menu-chain-mariofox-init-harness
NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_init`, proves VS Mode -> PlayersVS
-> Maps -> VSBattle first, then verifies the same bounded Mario/Fox init-state
boundary.

`verify-battle-mariofox-wait-harness.ps1` builds
`TARGET=smash64ds-battle-mariofox-wait
BUILD=build-battle-mariofox-wait-harness
NDS_DEV_SCENE_HARNESS=battle_mariofox_wait`, starts directly at the Pupupu
VSBattle boundary, imports original `ftcommonwait.c`, runs original
`ftCommonWaitSetStatus` for initialized Mario/Fox structs through a Wait-only
`ftMainSetStatus` seam, and verifies status `10`, motion `4`, animation frame
`0`, speed `1.0`, player tag wait `120`, callback pointer installation, and
zero callback/process/display/gameplay execution.

`verify-menu-chain-mariofox-wait-harness.ps1` builds
`TARGET=smash64ds-menu-chain-mariofox-wait
BUILD=build-menu-chain-mariofox-wait-harness
NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_wait`, proves VS Mode -> PlayersVS
-> Maps -> VSBattle first, then verifies the same imported original Mario/Fox
Wait status/motion setup boundary.

`verify-battle-mariofox-wait-tick-harness.ps1` builds
`TARGET=smash64ds-battle-mariofox-wait-tick
BUILD=build-battle-mariofox-wait-tick-harness
NDS_DEV_SCENE_HARNESS=battle_mariofox_wait_tick`, starts directly at the
Pupupu VSBattle Wait boundary, runs one bounded original
`ftCommonWaitProcInterrupt` tick for Mario and Fox with neutral input, calls
the guarded physics and map callback seams once per fighter, and verifies that
status, motion, ground/air state, root position, ground velocity, and GObj
count remain stable.

`verify-menu-chain-mariofox-wait-tick-harness.ps1` builds
`TARGET=smash64ds-menu-chain-mariofox-wait-tick
BUILD=build-menu-chain-mariofox-wait-tick-harness
NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_wait_tick`, proves VS Mode ->
PlayersVS -> Maps -> VSBattle first, then verifies the same bounded Wait
callback tick boundary.

`verify-battle-mariofox-wait-ground-harness.ps1` builds
`TARGET=smash64ds-battle-mariofox-wait-ground
BUILD=build-battle-mariofox-wait-ground-harness
NDS_DEV_SCENE_HARNESS=battle_mariofox_wait_ground`, starts directly at the
Pupupu VSBattle Wait tick boundary, seeds Mario/Fox ground velocity to `2.0`,
runs the bounded source-order ground-friction/air-transfer helper and safe
floor-map seam, and verifies velocity decrease plus stable fighter state.

`verify-menu-chain-mariofox-wait-ground-harness.ps1` builds
`TARGET=smash64ds-menu-chain-mariofox-wait-ground
BUILD=build-menu-chain-mariofox-wait-ground-harness
NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_wait_ground`, proves VS Mode ->
PlayersVS -> Maps -> VSBattle first, then verifies the same bounded
ground-friction/map boundary.

`verify-battle-mariofox-display-probe-harness.ps1` builds
`TARGET=smash64ds-battle-mariofox-display-probe
BUILD=build-battle-mariofox-display-probe-harness
NDS_DEV_SCENE_HARNESS=battle_mariofox_display_probe`, starts directly at the
Pupupu VSBattle Wait ground boundary, calls the current project-owned
`ftDisplayMainProcDisplay` seam once for Mario and once for Fox under a
DS-owned guard, records DObj/MObj/AObj/display-list metadata, and verifies no
draw, matrix, gameplay, root-position, status, motion, GA, or GObj-count escape.

`verify-menu-chain-mariofox-display-probe-harness.ps1` builds
`TARGET=smash64ds-menu-chain-mariofox-display-probe
BUILD=build-menu-chain-mariofox-display-probe-harness
NDS_DEV_SCENE_HARNESS=menu_chain_mariofox_display_probe`, proves VS Mode ->
PlayersVS -> Maps -> VSBattle first, then verifies the same metadata-only
Mario/Fox display callback boundary.

## Visual melonDS Debugging

Use the visible HUD when you need to watch runtime progress directly:

```powershell
.\scripts\debug-melonds.ps1 -Build
```

The HUD is rendered by `src/nds/nds_platform.c`:

- Top screen: no status rail, progress bars, or debug rectangles. The top
  framebuffer is reserved for original-asset previews so live melonDS output
  does not read as flashing.
- Top previews: converted original startup `N64Logo` Sprite from the bounded
  draw pass, the smaller bounded Opening Room material-candidate DObj
  display-list preview, the original Opening Portraits/name-card SObjs during
  the natural movie path, and the bounded original `MNTitle` sprite composite
  once the Title boundary is reached.
- Moving top-screen markers are disabled. The older compact trace strip, thin
  progress ticks, orange logo-position marker, and status rectangles were
  removed from the top framebuffer because they read as flashing once original
  assets became the visual signal.
- Bottom screen text: live diagnostic globals also checked by
  `scripts/verify-runtime.ps1`, including
  `event=4f524631 tick=280 mask=03` and
  `edata=4f524644 m=0f d=4 dl=3 a=3 op=3` once the first-event probes pass,
  plus `evrun=4f523238 def=01 f=4f524646/...`,
  `s1cam=4f523143 m=1ff vp=1`,
  `ccam=4f524343 m=1f vp=1`,
  `wcam=4f525743 m=1f vp=1`,
  `lcam=4f52434d m=7f a=01 vp=1`,
  `ovl=4f524f43/4f524f45 m=03`,
  `logo=4f524c43/4f524c45 m=03 c=3f`,
  `boss=4f524243/4f524245 m=03 c=3f`,
  `pencil=4f525043 m=3f g=1 d=3 x=6`, and
  `pencil a=0 p=1 dl=1 t=3 r=1` after the original pencils update path,
  followed by `or38=4f523338 d=01`, `or45=4f523435 d=00 su=03`,
  `obj d=0f h=0f o=0f s=0f c=07`, `or50=4f523530 d=01 sp=ff`,
  `or56=4f523536 d=01 e=07`, `s2=1ff a=01 g=2 c=2 x=4`,
  `dlp=4f524450 t=4 p>0 x=3f`, and
  `draw=4f524457 b=3 c=3 d=5` at the Opening Room bounded draw boundary, plus
  compact movie rows such as `mv h=... p=...`,
  `por t=150 d=4f504457 v=4`, `mario t=60 v=... px=...`,
  `name m=fe d=ff c=7`, `fps=60 up=.. dl=.. cv=..`, and
  `ch=... pf=... smp=.. win=..` after the natural movie path reaches the Title
  preview through the paced action bridge. The FPS rows are sampled about once
  per 60 DS VBlanks: `fps` is the ROM-side present rate, `up` is imported
  opening-movie update cadence, `dl` is retained DObj-preview draw cadence,
  `cv` is original-preview commits per sampled second, `ch` is total
  original-preview commits, and `pf` is the paced present-frame count. They do
  not measure host wall-clock emulator slowdown directly.
  The top
  framebuffer is double-buffered for the HUD, the startup logo preview is
  placed from N64 logical coordinates without downscaling the retained copy, the
  Opening Room preview is smaller and moved away from the native logo, the
  legacy DS-native moving probe overlay is disabled, and the bottom console
  redraw is change-driven with milestone-bucketed tick text, a hidden cursor,
  and row-addressed short lines to avoid text wrapping, scrolling, full-screen
  clear flashes, and steady-state pulse from live frame counters on the DS
  bottom screen.

Use this for visual triage; use `verify-runtime.ps1` for pass/fail evidence.

## no$gba Hardware Debugging

Use no$gba when renderer or hardware behavior needs interactive inspection of
DS registers, VRAM, OAM, palettes, backgrounds, timings, or debugger state:

```powershell
.\scripts\debug-nogba.ps1 -Build
```

The launcher defaults to `emulators/nogba/NO$GBA.EXE` and accepts `-NoGba` and
`-Rom` overrides. no$gba is not wired into the automated pass/fail verifier
yet; use it to diagnose hardware/rendering questions, then prove stable runtime
behavior with the melonDS verifier scripts.

The no$gba debugger build can run as one combined debug window or as separate
debugger/emulator windows depending on local settings. This machine is
currently configured for one `No$gba Debugger (Fullversion)` window. Capture all
visible no$gba windows for renderer/hardware handoff evidence:

```powershell
.\scripts\capture-nogba.ps1 -Build -AllWindows
.\scripts\verify-nogba-smoke.ps1 -Build
```

Use `docs/EMULATOR_STRATEGY.md` before renderer work to decide whether the next
test belongs in melonDS, no$gba, or both.

Capture the actual melonDS window to a PNG (default: timestamped file under
`artifacts/`) with:

```powershell
.\scripts\capture-melonds.ps1 -Build
```

For the current natural movie-to-Title visual boundary, use a longer wait:

```powershell
.\scripts\capture-melonds.ps1 -Build -DelaySeconds 145
```

This launches the real ROM, temporarily disables melonDS GDB in the config for
the visible capture attempt, foregrounds melonDS, waits for the live HUD to
reach the requested boundary, captures the emulator
window, and restores the original config. Inspect the PNG when changing HUD
layout or when handoff needs visual evidence; generated captures are ignored by
Git. If melonDS starts hidden/windowless, treat that as an emulator/session
launch issue before changing ROM code. The latest smoke capture after moving
emulators under `emulators/` is `artifacts/melonds-20260620-160953.png`; use a
longer `-DelaySeconds` value when the handoff needs a post-opening Title
capture.
The startup logo can look coarse when melonDS magnifies the native `128x108`
debug copy; that is expected for the diagnostic path and is separate from
renderer-quality work.
Visual capture is currently regression evidence, not the next implementation
focus. Prefer source-code boundary work first; only spend time on visual
debugging when the next original scene/menu import needs a renderer contract or
when a capture is needed to prove a maintained regression.

## Diagnostic Globals

Boot and OS:

- `gNdsBootSelfTestResult`: queue/thread self-test, expected `0x50415353`.
- `gNdsOriginalBootStage`: service-thread and scene bitfield, expected
  `0x53430007`.

Scheduler and frame loop:

- `sSYSchedulerTicCount`: original scheduler VBlank count, must be nonzero.
- `gNdsFrameCounter`: DS frame loop count, must be nonzero.
- `gNdsPerfPresentFps`: ROM-side present rate sampled over about 60 DS
  VBlanks. This is not host wall-clock FPS.
- `gNdsPerfLogicFps`: sampled imported opening-movie update rate. This can be
  `0` at the final runtime verifier sample after the movie has parked at Title.
- `gNdsPerfDLDrawFps`: sampled retained DObj-preview draw rate.
- `gNdsOriginalSpritePreviewCommitCount` /
  `gNdsOriginalDLPreviewCommitCount`: total retained original-preview content
  commits by source.
- `gNdsPerfPreviewCommitFps`: sampled original-preview content commits per
  second.
- `gNdsPerfPreviewCommitCount`: total retained original-preview commits.
- `gNdsPerfSampleCount` / `gNdsPerfSampleWindowTicks`: prove the low-noise
  sampler has published at least one bounded window.

Runtime speed sampling:

- `scripts/verify-runtime.ps1` reports `verifyfps=...` in its final pass line.
  This is a fixed-window verifier average and should not be treated as live
  emulator speed.
- `scripts/sample-runtime-speed.ps1 -DelaySeconds 8` is the quick one-shot
  host-speed probe. Current evidence is `frames=503`, `hostfps=54.28`,
  `romfps=60`, and `room=420` after about nine wall-clock seconds.
- A longer one-shot sample after limiting retained Opening Room DObj work to
  two original draw probes plus retained-preview reuse reached `frames=2257`,
  `hostfps=48.74`, `romfps=60`, `dl=60`, `ch=25`, `room=1320`,
  `rdraw=2/24`, `portraits=150`, `mario=60`, and action progress `4/128`
  after about 46 wall-clock seconds.
- `scripts/verify-opening-movie-speed.ps1` is the maintained full-opening
  host-speed gate. It wraps the sample script with checks for Title marker
  `0x54494457`, Opening Room tick `1320`, all nine action-scene bridge
  boundaries, three cached original action-preview sprites, at least `324`
  paced action-preview frames, and a default hidden-melonDS
  host-speed floor of `30` FPS. Current passing evidence is `frames=3289`,
  `hostfps=40.47`, `romfps=60`, `room=1320`, `rdraw=2/24`,
  `portraits=150`, `mario=60`, `action=9/324`, and
  `title=0x54494457`.
- The FPS/content counters already rejected cadence-only speedups for this
  boundary. `NDS_OPENING_MOVIE_DRAW_INTERVAL=10` missed the Title/action
  verifier window, and `20` reached Title but made the strict Opening Room
  DObj blocker sample unreliable. Keep the verified interval at `30` until
  renderer cost is reduced.
- Avoid repeated GDB polling for progress. In testing, repeated attach/detach
  caused melonDS packet errors and prevented the Title boundary from being
  reached.

Controller:

- `gSYControllerConnectedNum`: original controller discovery count, expected
  `1`.

Opening movie / Opening Portraits:

- `gNdsOpeningMovieRoomHandoffResult`: Opening Room natural movie handoff
  marker, expected `0x4F4D5248`.
- `gNdsOpeningMovieRoomHandoffTick`: current verified natural handoff tick,
  expected `1320`.
- `gNdsOpeningMovieRoomHandoffScene`: requested next scene, expected `29`
  (`nSCKindOpeningPortraits`).
- `gNdsOpeningPortraitsStartResult`: imported Opening Portraits scene start
  marker, expected `0x4F505354`.
- `gNdsOpeningPortraitsFuncStartResult`: imported Opening Portraits task
  `func_start` marker, expected `0x4F504653`.
- `gNdsOpeningPortraitsRelocResult`: portrait O2R relocation/normalization
  marker, expected `0x4F50524C`.
- `gNdsOpeningPortraitsSpriteNormalizeCount` /
  `gNdsOpeningPortraitsSpriteNormalizeFailCount`: current expected pair `9,0`
  for four Set1 cards, four Set2 cards, and the Set1 cover metadata.
- `gNdsOpeningPortraitsDrawResult`: bounded original SObj preview draw marker,
  expected `0x4F504457`.
- `gNdsOpeningPortraitsDrawWidth` / `Height` / `Format` / `Size` /
  `Bitmaps` / `Pixels`: current successful card-draw evidence,
  `300,55,0,2,11` with nonzero accumulated pixels.
- `gNdsOpeningPortraitsNextSceneResult`: original next-scene marker, expected
  `0x4F504E58`.
- `gNdsOpeningPortraitsNextSceneKind`: expected `30`
  (`nSCKindOpeningMario`).
- `gNdsOpeningMarioDrawResult`: bounded original Mario name-card SObj draw
  marker, expected `0x4F4D4457`.
- `gNdsOpeningNameSceneDispatchMask` / `DrawMask`: imported name-card scene
  coverage, expected `0xFE` dispatch and `0xFF` draw for Donkey through
  Pikachu.
- `gNdsOpeningMovieBridgeResult`: bounded action-scene bridge marker,
  expected `0x4F4D4252`.
- `gNdsOpeningMovieBridgeMask`: action-scene bridge coverage for
  `OpeningRun` through `OpeningNewcomers`, expected `0x1FF`.
- `gNdsOpeningMovieTitleResult`: natural Title dispatch marker, expected
  `0x4F4D5449`.
- `gNdsSceneHarnessResult`: dev/test harness marker. Maintained harnesses
  expect `0x4841524E` (`HARN`).
- `gNdsSceneHarnessMode`: build-time harness mode: `0` normal, `1` direct
  Title, `2` bounded VS setup from Title, `3` direct bounded VSBattle setup,
  `4` bounded VS Start to PlayersVS transition from Title, `5` direct
  PlayersVS setup, `6` direct Maps setup, `7` guarded VS Mode -> PlayersVS
  -> Maps -> VSBattle chain, `8` direct Pupupu/Dream Land stage setup, `9`
  direct guarded Pupupu update proof, `10` guarded menu-chain Pupupu update
  proof, `11` direct Mario/Fox model proof, `12` guarded menu-chain
  Mario/Fox model proof, `13` direct Mario/Fox persistent FTStruct proof,
  `14` guarded menu-chain Mario/Fox persistent FTStruct proof, `15` direct
  Mario/Fox source-order init-state proof, `16` guarded menu-chain Mario/Fox
  source-order init-state proof, `17` direct Mario/Fox Wait setup proof, `18`
  guarded menu-chain Mario/Fox Wait setup proof, `19` direct Mario/Fox Wait
  tick proof, `20` guarded menu-chain Mario/Fox Wait tick proof, `21` direct
  Mario/Fox Wait ground-friction/map proof, `22` guarded menu-chain
  Mario/Fox Wait ground-friction/map proof, `23` direct Mario/Fox display
  metadata callback probe, and `24` guarded menu-chain Mario/Fox display
  metadata callback probe, `25` direct Mario/Fox parser-only display-list scan
  proof, and `26` guarded menu-chain Mario/Fox parser-only display-list scan
  proof, `27` direct Mario/Fox decode-only display-list execute proof, and
  `28` guarded menu-chain Mario/Fox decode-only display-list execute proof,
  `29` direct Mario/Fox visible first-DL software draw proof, `30` guarded
  menu-chain Mario/Fox visible first-DL software draw proof, `31` direct
  Mario/Fox visible multi-DL software draw proof, and `32` guarded menu-chain
  Mario/Fox visible multi-DL software draw proof, `33` direct Mario/Fox
  guarded all-DL software draw proof, and `34` guarded menu-chain Mario/Fox
  guarded all-DL software draw proof, `35` direct Mario/Fox original
  Wait -> Walk input/status/velocity proof, `36` guarded menu-chain Mario/Fox
  original Wait -> Walk input/status/velocity proof, `37` direct Mario/Fox
  bounded Walk movement-loop/release-to-Wait proof, and `38` guarded
  menu-chain Mario/Fox bounded Walk movement-loop/release-to-Wait proof, `39`
  direct Mario/Fox bounded Dash -> Run -> RunBrake proof, `40` guarded
  menu-chain Mario/Fox bounded Dash -> Run -> RunBrake proof, `41` direct
  Mario/Fox bounded RunBrake -> Wait -> KneeBend -> JumpF proof, `42` guarded
  menu-chain Mario/Fox bounded RunBrake -> Wait -> KneeBend -> JumpF proof,
  `43` direct Mario/Fox bounded JumpF -> Fall -> LandingLight -> Wait proof,
  `44` guarded menu-chain Mario/Fox bounded JumpF -> Fall -> LandingLight ->
  Wait proof, `45` direct Mario/Fox bounded scripted process-loop proof, and
  `46` guarded menu-chain Mario/Fox bounded scripted process-loop proof, `47`
  direct Mario/Fox VSBattle update-driven scheduler-loop proof, `48`
  guarded menu-chain Mario/Fox VSBattle update-driven scheduler-loop proof,
  `49` direct Mario/Fox controller-source scheduler-loop proof, and `50`
  guarded menu-chain Mario/Fox controller-source scheduler-loop proof, `51`
  direct Mario/Fox moving preview-loop proof, `52` guarded menu-chain
  Mario/Fox moving preview-loop proof, `53` direct Mario/Fox original
  `gcRunAll` moving-preview proof, `54` guarded menu-chain Mario/Fox original
  `gcRunAll` moving-preview proof, `55` direct Mario/Fox live-input
  moving-preview idle proof, `56` guarded menu-chain Mario/Fox live-input
  moving-preview idle proof, `57` direct Mario/Fox bounded original
  `gcDrawAll` moving-preview proof, `58` guarded menu-chain Mario/Fox
  bounded original `gcDrawAll` moving-preview proof, `59` direct Pupupu
  stage-inclusive Mario/Fox bounded original `gcDrawAll` proof, and `60`
  guarded menu-chain Pupupu stage-inclusive Mario/Fox bounded original
  `gcDrawAll` proof, `61` direct Pupupu geometry-backed Mario/Fox
  floor-collision proof, `62` guarded menu-chain Pupupu geometry-backed
  Mario/Fox floor-collision proof, `63` through `84` the maintained
  direct/menu-chain Pupupu floor-follow, floor-edge, MP floor-process,
  MP update-main, MP sweep, MP cross-floor, MP floor-edge-adjust,
  MP edge-under, MP wall-blocker, MP stale-valid, and MP live-stale proof
  pairs, and `85` / `86` the direct/menu-chain MP motion-stale
  selected-callback/root mutation proof pair.
- `gNdsSceneHarnessSceneCurr` / `ScenePrev`: default scene pair preseeded
  before imported `scManagerRunLoop` copies it. Direct Title expects `1/46`
  (`nSCKindTitle` from `nSCKindOpeningNewcomers`).
- `gNdsSceneHarnessReservedMask`: expected `0` for the maintained Title,
  VS setup, direct VSBattle setup, VS Start transition, PlayersVS, Maps,
  menu-chain, Pupupu stage/update, and Mario/Fox model/struct/init/Wait
  harnesses.
- `gNdsTitleOriginalStartResult` / `FuncStartResult`: imported
  `mntitle.c` bounded start markers, expected `0x54495354` and `0x54494653`.
- `gNdsTitleOriginalSetupMask`: current bounded Title setup coverage,
  expected `0xF` for file load, actor creation, camera creation, and var init.
- `gNdsTitleOriginalLoadedFileCount`: original Title file-list load count,
  expected `2` (`MNTitle` and `MNTitleFireAnim`).
- `gNdsTitleOriginalGObjCount` / `CameraCount`: bounded original Title setup
  evidence, expected at least two actor GObjs and exactly four camera GObjs.
- `gNdsTitleOriginalDeferredMask`: explicitly deferred Title branches,
  expected `0x1E` for the remaining logo, labels/Press Start, slash, and
  logo-fire-particle paths after the bounded fire object branch runs.
- `gNdsTitleOriginalLogoFireResult` / `LogoFireMask`: original
  `efParticleInitAll` plus `mnTitleMakeLogoFire` boundary proof, expected
  `TITLE_LOGO_FIRE=0x544c4643,0x3f,1,4,3,1,0`.
- `gNdsTitleFireSpriteNormalizeCount` / `FailCount`: original
  `MNTitleFireAnim` frame Sprite normalization proof, expected `30,0`.
- `gNdsTitleOriginalFireResult` / `FireMask`: original `mnTitleMakeFire`
  object/process/display proof. Natural movie path expects
  `TITLE_FIRE=0x54464952,0xfff,1,2,1,2,786432,0`; the skip-to-Title path
  expects the same marker with shown-fire state
  `TITLE_FIRE=0x54464952,0xfff,1,2,0,2,786432,255`.
- `gNdsTitleOriginalUpdateResult` / `UpdateCount`: bounded original Title
  update proof, expected `0x54495550` and `1` on the natural
  `OpeningNewcomers -> Title` path.
- `gNdsTitleOriginalLayout` / `TransitionTics` /
  `StartActorProcess` / `ProceedScene` / `ProceedWait`: expected
  `0,1,1,0,3` after the one safe update tick. In the skip-to-Title verifier,
  the guarded updater deliberately leaves `TITLE_ORIGINAL_UPDATE=0,0,1,169,0,0,3`
  because that path starts at the later non-opening layout boundary.
- `gNdsTitleRelocResult`: original `MNTitle` O2R load/normalization marker for
  the retained preview, expected `0x5449524C`.
- `gNdsTitlePreviewResult`: bounded Title sprite preview marker, expected
  `0x54495056`.
- `gNdsTitleDrawResult`: at least one Title SObj rendered, expected
  `0x54494457`.
- `gNdsTitleSpriteNormalizeCount` / `FailCount`: current expected pair `10,0`.
- `gNdsTitleDrawVisibleSObjCount` / `RenderableSObjCount` /
  `SObjCount` / `Pixels`: current expected counts `10,10,10` with nonzero
  accumulated pixels.
- `gNdsVSModeOriginalStartResult` / `FuncStartResult`: imported
  `mnvsmode.c` bounded start markers, expected `0x56535354` (`VSST`) and
  `0x56534653` (`VSFS`).
- `gNdsVSModeOriginalRelocResult`: original VS setup file-list load marker,
  expected `0x5653524C` (`VSRL`) after `MNCommon` and `MNVSMode` load.
- `gNdsVSModeOriginalSetupResult`: bounded original VS setup marker, expected
  `0x56535355` (`VSSU`).
- `gNdsVSModeOriginalSetupMask`: current bounded VS setup coverage, expected
  `0x1F` for file load, main GObj, default camera, viewports, and menu SObj
  graph creation.
- `gNdsVSModeOriginalLoadedFileCount`: original VS setup file-list load count,
  expected `2`.
- `gNdsVSModeOriginalGObjCount` / `CameraCount` / `SObjCount`: current
  bounded VS setup object proof, expected at least `8`, at least `1`, and at
  least `20`; current verifier proof is `17,1,28`.
- `gNdsVSModeOriginalButtonMask`: button/value SObj proof, expected `0x3F`
  for VS Start, Rule, Time/Stock, VS Options, Rule value, and Time/Stock value
  object creation.
- `gNdsVSModeOriginalCursorIndex` / `Rule` / `Time` / `Stock`: original VS
  setup state sampled after `mnVSModeFuncStartVars`; current harness proof is
  `0,0,3,2`.
- `gNdsVSModeOriginalDeferredMask`: explicitly deferred VS branches, expected
  `0x7` for `mnVSModeMain` input/update, scene transition, and continuous
  drawing.
- `gNdsVSModeStartTransitionResult`: bounded original VS Start transition
  marker. Expected pass is `0x56535452` (`VSTR`); fail marker is
  `0x5653464C` (`VSFL`).
- `gNdsVSModeStartTransitionMask`: expected `0xFF`. Bit `0` proves the setup
  boundary was complete at entry; bit `1` proves original `mnVSModeMain`
  reached the input-ready tick window; bit `2` proves the synthetic A tap was
  injected through original controller globals; bit `3` proves original
  `mnVSModeMain` changed scene state to `VSMode -> PlayersVS`; bit `4` proves
  original `mnVSModeSaveSettings` stored rule/time/stock; bit `5` proves the
  original start branch set `sMNVSModeExitInterrupt`; bit `6` proves the
  follow-up original tick requested `syTaskmanSetLoadScene`; bit `7` proves
  bounded cleanup ran before returning to the scene manager.
- `gNdsVSModeStartTransitionUpdateCount`: bounded original `mnVSModeMain` call
  count, expected at least `11` for nine idle ticks, one A tap tick, and one
  follow-up load-scene tick.
- `gNdsVSModeStartTransitionInputMask`: synthetic input used for the transition,
  expected `0x8000` (`A_BUTTON`).
- `gNdsVSModeStartTransitionScenePrevBefore` /
  `SceneCurrBefore`: expected `1/9` before the A tap.
- `gNdsVSModeStartTransitionScenePrevAfterTap` /
  `SceneCurrAfterTap`: expected `9/16` after original `mnVSModeMain` accepts
  VS Start.
- `gNdsVSModeStartTransitionScenePrevFinal` /
  `SceneCurrFinal`: expected `9/16` after the follow-up original load-scene
  tick.
- `gNdsVSModeStartTransitionExitInterrupt`: expected `1`, proving the original
  VS exit-interrupt flag was set by the start branch.
- `gNdsVSModeStartTransitionTaskmanStatus`: expected `1`
  (`nSYTaskmanStatusLoadScene`) after the follow-up original tick.
- `gNdsVSModeStartTransitionSavedRule` /
  `SavedTime` / `SavedStock`: expected `1/3/2`, proving the original transfer
  battle settings were saved as time rule, three minutes, two stock.
- `gNdsVSModeStartTransitionButtonMaskAfter`: expected `0x3F`, proving the
  original VS setup button/value SObj proof still survived the transition
  probe.
- `gNdsVSModeStartTransitionCleanupCount`: expected `1`, proving the bounded
  transition probe performed one object cleanup before returning to the scene
  manager.
- `gNdsPlayersVSOriginalStartResult` / `FuncStartResult`: imported
  `mnplayersvs.c` bounded start markers, expected `0x50565354` (`PVST`) and
  `0x50564653` (`PVFS`).
- `gNdsPlayersVSOriginalRelocResult`: original PlayersVS file-list load marker,
  expected `0x5056524C` (`PVRL`) after seven menu files load.
- `gNdsPlayersVSOriginalSetupResult`: bounded original PlayersVS setup marker,
  expected `0x50565355` (`PVSU`).
- `gNdsPlayersVSOriginalSetupMask`: expected `0xFF`. It covers reloc/file
  load, main GObj, default camera, controller-order/vars setup, fighter-manager
  compatibility calls, figatree heap allocation, camera/UI object graph setup,
  and light-parameter setup.
- `gNdsPlayersVSOriginalLoadedFileCount`: expected `7`.
- `gNdsPlayersVSOriginalGObjCount` / `CameraCount` / `SObjCount`: bounded
  PlayersVS object proof; current verifier proof is `sobj=65`.
- `gNdsPlayersVSOriginalControllerOrderMask`,
  `SlotKindMask`, `SlotSelectedMask`, `CursorCount`, `PuckCount`,
  `GateCount`, `PortraitCount`, and `FigatreeHeapCount`: setup-state evidence
  sampled after original PlayersVS init and object creation.
- `gNdsPlayersVSOriginalTime` / `Stock` / `GameRule` / `IsTeam` /
  `IsStageSelect`: expected `3/2/time-rule/0/1` for the seeded VS defaults.
- `gNdsPlayersVSReadyTransitionResult`: bounded original PlayersVS ready/start
  marker. Expected pass is `0x50565452` (`PVTR`); fail marker is
  `0x5056464C` (`PVFL`).
- `gNdsPlayersVSReadyTransitionMask`: expected `0xFF`. It proves setup was
  complete, deterministic two-player selected state was seeded, original
  `mnPlayersVSFuncRun` reached the input-ready/proceed window, synthetic Start
  was injected as `0x1000`, original scene state changed PlayersVS -> Maps,
  the original load-scene request was observed, and bounded cleanup ran.
- `gNdsPlayersVSReadyTransitionScenePrevBefore` /
  `SceneCurrBefore` / `ScenePrevFinal` / `SceneCurrFinal`: expected
  `9/16 -> 16/21` in the menu-chain harness.
- `gNdsPlayersVSReadyTransitionPlayerCount` /
  `CpuCount` / `P0FKind` / `P1FKind` / `StageSelect`: expected at least two
  players, zero CPUs, deterministic Mario/Fox player seeds, and stage select
  enabled for the bounded proof.
- `gNdsMapsOriginalStartResult` / `FuncStartResult`: imported `mnmaps.c`
  bounded start markers, expected `0x4D415053` (`MAPS`) and `0x4D504653`
  (`MPFS`).
- `gNdsMapsOriginalRelocResult`: original Maps file-list load marker, expected
  `0x4D50524C` (`MPRL`) after five files load.
- `gNdsMapsOriginalSetupResult`: bounded original Maps setup marker, expected
  `0x4D505355` (`MPSU`).
- `gNdsMapsOriginalSetupMask`: expected `0xFF`. It covers reloc/file load,
  bounded model-heap proof, main GObj, default camera, vars, camera set,
  wallpaper/plaque/labels/icons/name/cursor SObj graph, and bounded Pupupu
  preview proof.
- `gNdsMapsOriginalLoadedFileCount`: expected `5`.
- `gNdsMapsOriginalCursorSlot` / `GroundKind`: expected `6/6` for the seeded
  Pupupu/Dream Land direct Maps and menu-chain harnesses.
- `gNdsMapsOriginalPreviewDeferred` / `DeferredMask`: expected `0/0x0` for
  the Pupupu direct Maps and menu-chain harnesses.
- `gNdsMapsOriginalPreviewResult`: expected `0x50555056` (`PUPV`) after the
  bounded Pupupu preview path runs.
- `gNdsMapsOriginalPreviewMask`: expected `0xFF`. Bits mean:
  `0` GRPupupuMap loaded, `1` map header/ground data resolved, `2`
  StageDreamLand wallpaper pointer resolved, `3` preview path called, `4`
  preview object proof recorded, `5` model/DObj proof recorded, `6` Pupupu
  preview no longer deferred, `7` diagnostics recorded.
- `gNdsMapsOriginalPreviewGObjCount` / `LayerGObjMask` /
  `WallpaperMade` / `ModelMade` / `DObjCount` / `MObjCount`: bounded Maps
  preview object evidence. Current Pupupu proof expects nonzero GObj/DObj
  counts, wallpaper/model flags set, and a nonzero layer mask.
- `gNdsMapsSelectTransitionResult`: bounded original Maps A-select marker.
  Expected pass is `0x4D53454C` (`MSEL`); fail marker is `0x4D53464C`
  (`MSFL`).
- `gNdsMapsSelectTransitionMask`: expected `0xFF`. It proves setup was
  complete, cursor/ground kind were seeded to Pupupu, original input-ready
  ticks ran, synthetic A was injected as `0x8000`, original scene data saved
  the selected ground kind, scene state changed Maps -> VSBattle, load-scene
  status was observed, and bounded cleanup ran.
- `gNdsMapsSelectTransitionScenePrevBefore` /
  `SceneCurrBefore` / `ScenePrevFinal` / `SceneCurrFinal`: expected
  `16/21 -> 21/22` in the menu-chain harness.
- `gNdsMapsSelectTransitionSelectedSlot` /
  `SelectedGKind`: expected `6/6`.
- `gNdsStagePupupuRelocResult`: expected `0x50555052` (`PUPR`) after
  `GRPupupuMap` and its dependency chain are loaded/fixed enough for Maps or
  VSBattle stage-data use.
- `gNdsStagePupupuRelocAssetMask`: expected low five bits `0x1F`. Bits mean:
  `0` GRPupupuMap, `1` StageDreamLand, `2` ExternDataBank103, `3`
  ExternDataBank104, `4` MiscDataBank152.
- `gNdsStagePupupuRelocDependencyMask`: expected low five bits `0x1F`,
  proving the external dependency chain was traversed.
- `gNdsStagePupupuExternalFixupCount` /
  `ExternalFixupFailCount`: expected at least `9` external fixups and zero
  failures; current full Pupupu dependency proof records `66` fixups.
- `gNdsStagePupupuInternalFixupCount`: internal pointer-chain fixup count for
  staged Pupupu assets.
- `gNdsStagePupupuMapHeaderOffset`: expected `0x14`.
- `gNdsStagePupupuGroundDataPtrReady` / `WallpaperPtrReady` /
  `GeometryPtrReady` / `MapNodesPtrReady`: expected `1` when the Pupupu path
  resolves real `MPGroundData` and preview/stage pointers.
- `gNdsStagePupupuLightAngleXBits` / `LightAngleYBits` / `BGM`: raw stage
  metadata sampled from the resolved Pupupu map data.
- `STAGE_INISHIE_ASSET`: emitted by the platform-speed and Inishie scale
  harness pairs as the dependency preflight for the Inishie/Mushroom Kingdom
  moving-yakumono boundary. Expected marker is
  `0x494e4952,0x14,1,1,1,...`, summarized as
  `inishieAsset=header/geometry nodes=1`. It proves `GRInishieMap` is staged
  and `llGRInishieMapMapHeader` resolves without disturbing the active Pupupu
  proof.
- `STAGE_INISHIE_SCALE_SOURCE`: source-setup diagnostic for current Inishie
  scale boundary builds. Direct and menu-chain source-backed builds currently
  reach step `13`, after original `grInishieMakeScale` returns through the
  offset-compatible setup shim. The second and third fields record active
  GObj/DObj counts sampled before source setup; the last field is the source
  setup readiness mask. Current boundary builds report the original low bits
  plus source bits `0x1ff00`: safe active `G_ENDDL`, native
  ScaleRetract words, raw `155.vpk0.bin` load, raw DObjDesc validation, raw
  map-head-DL validation, raw ScaleRetract validation, native DL scan pass,
  native DObj DL pointers, and native map-head DL pointer. This marker is
  diagnostic context only; the main pass/fail contract remains
  `STAGE_INISHIE_SCALE`.
- `STAGE_INISHIE_SCALE_DISPLAY`: source-display diagnostic emitted with
  `STAGE_INISHIE_SCALE_SOURCE`. For source-backed builds that reach source step
  `13`, expected values are mask `0xff`, count `4`, nonzero command and
  triangle totals, and blocker `0`. The mask proves the two platform DObjs have
  the original `gcDrawDObjDLHead0` display callback, the two string DObjs have
  the original `gcDrawDObjTreeForGObj` display callback, and all four DObj DLs
  scan cleanly through `ndsRendererScanDisplayList`. Current boundary builds
  summarize this as `sourceDL=0xff cmds=91 tris=20`.
- `STAGE_INISHIE_SCALE_MATERIAL`: source-DL material/texture-state diagnostic
  emitted with `STAGE_INISHIE_SCALE_DISPLAY`. Expected current values include
  texture mask `0x3f`, nonzero texture command count, nonzero source texture
  image token, and nonzero tile-size summary. This proves the selected original
  `StageInishieFile3` scale/map-head display-list path exposes `SETCOMBINE`,
  `TEXTURE`, `SETTIMG`, `SETTILE`, `LOADBLOCK`, and `SETTILESIZE` state to the
  DS renderer adapter. It does not yet prove full DS hardware texture upload or
  material application.
- `STAGE_INISHIE_SCALE_PREVIEW`: bounded source-DL visibility diagnostic
  emitted with `STAGE_INISHIE_SCALE_DISPLAY`. Expected current values include
  preview mask `0x3f`, four attempted DObjs, nonzero decoded vertices,
  triangles, valid triangles, pixel count, commit delta `1`, and blocker `0`.
  Current summaries report `preview=0x3f px=432`. The preview is a bounded
  software source-DL proof using decoded original vertices and a fallback
  vertex marker path when the temporary triangle filler produces no coverage;
  full textured DS hardware rendering remains deferred.
- `gNdsSCVSBattleOriginalStartResult` / `FuncStartResult`: imported
  `scvsbattle.c` bounded start markers, expected `0x56425354` (`VBST`) and
  `0x56424653` (`VBFS`).
- `gNdsSCVSBattleOriginalRelocResult`: original common battle file-list load
  marker, expected `0x5642524C` (`VBRL`) after eight interface/common battle
  files load.
- `gNdsSCVSBattleOriginalSetupResult`: bounded original VSBattle setup marker,
  expected `0x56425355` (`VBSU`).
- `gNdsSCVSBattleOriginalSetupMask`: expected `0x6F`. It proves common file
  load, default battle camera path, manager compatibility stubs, active fighter
  descriptor construction, audio/BGM compatibility stubs, and the one bounded
  taskman update tick. Bit `0x10` was the old interface/HUD compatibility-stub
  marker; with imported `if/ifcommon.c` default, BattleShip `scvsbattle.c:204`
  and `scvsbattle.c:212-220` create the original interface path instead.
- `gNdsSCVSBattleOriginalLoadedFileCount`: expected `8`.
- `gNdsSCVSBattleOriginalGObjCount` / `CameraCount` / `MainGObjID`: bounded
  VSBattle setup object evidence after imported `scVSBattleStartBattle`.
- `gNdsSCVSBattleFighterGObjCount` / `ActivePlayerMask`: stub fighter GObjs
  created from the active `SCBattleState` player descriptors. Direct
  `battle_fd` expects `1/0x1`; the menu-chain proof expects at least `2/0x3`.
- `gNdsSCVSBattlePlayerCount` / `CpuCount` / `GameRule` / `Stock` / `GKind`:
  battle-state evidence sampled at the bounded setup boundary. Direct
  `battle_fd` expects `1/0/2/3/16`; the menu-chain proof currently preserves
  the selected Pupupu/Dream Land ground kind `6`.
- `gNdsSCVSBattleOriginalActivePlayerCount` /
  `FighterCreateCount` / `P0FKind` / `P1FKind` / `P0LR` / `P1LR`:
  active fighter descriptor evidence. The Pupupu battle-stage proof expects
  `2/2/0/1/1/-1` for Mario and Fox facing inward.
- `gNdsSCVSBattleScenePrev` / `SceneCurr`: expected `21/22`.
- `gNdsSCVSBattleCompatMask`: low byte compatibility coverage. Expected
  `0xFF` for effects, ground collision, camera, item/weapon manager, fighter
  manager, interface, audio, and rumble stubs.
- `gNdsSCVSBattleCompatCameraMask`: expected low six bits set for the battle,
  player-arrow, magnify, screen-flash, interface, and effect camera stubs.
- `gNdsSCVSBattleCompatInterfaceMask`: interface/HUD setup calls reached as
  bounded compatibility stubs.
- `gNdsSCVSBattleCompatManagerMask`: manager setup calls reached as bounded
  compatibility stubs, including fighter setup and stub fighter creation.
- `gNdsSCVSBattleCompatAudioMask` / `LastAudioVolume` / `LastFGM`: shared
  audio compatibility evidence. FGM/voice calls are diagnostic only; BGM calls
  now route through the one-track Pupupu backend when enabled.
- `BPLAY_PACE`: mode `163` presentation marker. Fast harness builds use
  `NDS_HARNESS_FAST_LOGIC=1` and expect mode `1`, at least 3200 logic frames,
  zero presented/drawn frames, and a nonzero hardware timer. Realtime/manual
  builds use mode `0` and expect one logic update, one scene draw, and one DS
  vblank-presented frame per iteration, with presented and logic rates in
  `59.3..60.3` fps.
- `AUDIO_BGM`: mode `163` BGM backend marker. Expected default proof:
  result `0x42474d31`, low mask bits `0x3`, playing `0` after teardown,
  track `0`, volume `0x7800`, at least one play and stop call, no open/read/
  unsupported-track failures, read bytes at least one 64 KiB chunk, resident
  bytes `65536`, 32 KiB refill size, at least one played chunk, and
  stopped-on-teardown `1`. The hardware-timer rate guard expects at least 3200
  fast-logic frames, streamed rate within `42100..46100` B/s around the PCM16
  mono `44100` B/s target from `scripts/render-audio-bgm-pupupu.py`, safe
  opposite-half writes, and zero unsafe write attempts. Realtime smoke uses the
  same timer-derived byte-rate guard; whole-track wrap is supported and is only
  expected when a run outlasts the 65.5-second rendered track.
- `gNdsSCVSBattleCompatSpawnMask`: deterministic spawn-position queries from
  `mpCollisionGetPlayerMapObjPosition`.
- `gNdsSCVSBattleOriginalUpdateResult` / `UpdateCount`: one bounded
  `scVSBattleFuncUpdate` interface tick, expected `0x56425550` (`VBUP`) and
  `1`.
- `gNdsFighterMarioFoxRelocResult`: expected `0x4654524C` (`FTRL`) after
  `FTManagerCommon`, Mario, Fox, and required fighter dependencies load and
  relocate.
- `gNdsFighterMarioFoxRelocAssetMask`: expected low sixteen bits `0xFFFF` in
  the Mario/Fox model harnesses. The bits cover `FTManagerCommon`, Mario main
  motion/main/special/model/shield-pose files, and Fox special/main
  motion/main/model/shield-pose files.
- `gNdsFighterMarioFoxRelocDependencyMask`: expected low twenty bits set in
  the model harnesses, including the shared external fighter dependency files.
- `gNdsFighterMarioFoxExternalFixupFailCount`: expected `0`.
- `gNdsFighterMarioFoxModelResult`: expected `0x46544D44` (`FTMD`) after the
  bounded Mario/Fox model GObj proof completes.
- `gNdsFighterMarioFoxGObjResult`: expected `0x4654474F` (`FTGO`) after two
  asset-backed fighter GObjs are created.
- `gNdsFighterMarioFoxSetupMask`: expected low twelve bits `0xFFF`. The mask
  proves fighter file readiness, manager allocation, Mario/Fox file and
  attributes readiness, Mario/Fox commonpart descriptor readiness, display
  attachment, process deferral, and Mario/Fox GObj creation.
- `gNdsFighterModelRealGObjCount`: expected `2` in the Mario/Fox model
  harnesses.
- `gNdsFighterModelStubGObjCount`: expected `0` in the Mario/Fox model
  harnesses; setup-only VSBattle harnesses still use stub fighter GObjs.
- `gNdsFighterModelProcessDeferredCount`: expected at least `2`, documenting
  that real fighter processes/gameplay remain parked after model GObj creation.
- `gNdsFighterManagerResult`: expected `0x46544D47` (`FTMG`) only in the
  fenced `-ImportBattleShipFTManager` init proof.
- `gNdsFighterManagerMask`: expected `0xFF`, covering manager extern payloads,
  status-buffer payloads, nonzero figatree heap, both Mario/Fox fighters,
  `dFTManagerDataFiles` ownership, source Entry/Wait status ownership, and
  repeated status-buffer hits.
- `gNdsFighterManagerExternMask` / `StatusBufferMask`: expected `0xF` and
  `0x1FFF`, respectively, for the Mario/Fox manager/common/mainmotion/submotion
  file set loaded through `lbRelocGetStatusBufferFile` semantics.
- `gNdsFighterManagerFighterMask` / `DataMask` / `EntryMask`: expected `0x3`;
  the normal VSBattle descriptors install Entry, not Wait, because source
  `ftdata.c:75-96` leaves `is_skip_entry` false and `ftmanager.c:867-899`
  chooses Entry unless that flag is set.
- `gNdsFighterManagerStatusBufferHitCount` / `FighterCount` /
  `FigatreeHeapSize`: currently `29`, `2`, and `68` in the fenced proof.
- `gNdsFighterModelP0*` / `P1*`: model proof details for Mario/Fox fighter
  kind, GObj ID, top DObj readiness, DObj/MObj/AObj counts, and display
  callback attachment. Current direct proof records `p0DObjs=25` and
  `p1DObjs=27`.
- `gNdsFighterModelLast*`: pointer/stage diagnostics for the last fighter model
  materialization attempt. These stay in the maintained marker set because they
  caught the stale fighter-file-slot bug where a menu-chain scene reset cleared
  the reloc cache without clearing fighter-specific file pointers.
- `gNdsFighterMarioFoxStructResult`: expected `0x46545348` (`FTSH`) after the
  persistent project-owned `FTStruct` shells are attached to the real
  Mario/Fox GObjs.
- `gNdsFighterMarioFoxJointResult`: expected `0x46544A54` (`FTJT`) after the
  bounded joint-table proof records a top joint and at least twenty common
  joints for both fighters.
- `gNdsFighterMarioFoxStateResult`: expected `0x46545354` (`FTST`) after
  descriptor identity, input, and collision pointer state are initialized.
- `gNdsFighterMarioFoxStructMask`: expected low twelve bits `0xFFF`. Bits mean:
  `0` struct pool allocated, `1` both GObjs store `user_data.p`, `2`
  `ftGetStruct` returns pool pointers, `3` descriptor identity copied, `4`
  attribute pointers ready, `5` figatree pointers ready, `6` controller/input
  state initialized, `7` top joints ready, `8` common joint table populated,
  `9` collision translate/LR pointers ready, `10` status/process/display
  runtime remains deferred, and `11` both fighters recorded.
- `gNdsFighterMarioFoxStructPoolUsedMask`: expected `0x3` in the direct and
  menu-chain struct harnesses.
- `gNdsFighterMarioFoxStructCount`: expected `2`.
- `gNdsFighterStructP0*` / `P1*`: persistent struct proof details. Expected
  direct values are Mario `fkind=0 pkind=0 player=0 lr=1 stock=2 detail=1` and
  Fox `fkind=1 pkind=0 player=1 lr=-1 stock=2 detail=1`; some GDB builds print
  Fox LR as raw `0xffffffff`, which the verifier accepts.
- `gNdsFighterStructP0InputMask*` / `P1InputMask*`: expected
  A/B/Z/L masks `0x8000`, `0x4000`, `0x2000`, and `0x20` for both fighters.
- `gNdsFighterStructP0TopJointReady` / `P1TopJointReady`: expected `1`.
- `gNdsFighterStructP0CommonJointCount` / `P1CommonJointCount`: expected at
  least `20`. Current direct proof records Mario `24` and Fox `26`.
- `gNdsFighterStructP0CollTranslateReady` / `P1CollTranslateReady` and
  `P0CollLRReady` / `P1CollLRReady`: expected `1`.
- `gNdsFighterStructProcessAttachCount`,
  `gNdsFighterStructStatusSetCount`, and
  `gNdsFighterStructDisplayProbeCount`: expected `0`, documenting that original
  fighter processes/status/display remain parked during the struct proof.
- `gNdsFighterMarioFoxInitResult`: expected `0x4654494E` (`FTIN`) after the
  bounded source-order Mario/Fox init-state helper completes for both active
  fighters.
- `gNdsFighterMarioFoxCollResult`: expected `0x4654434C` (`FTCL`) after the
  deterministic Pupupu floor-projection/collision contract is recorded.
- `gNdsFighterMarioFoxDeferResult`: expected `0x46544446` (`FTDF`) when the
  init proof explicitly parks fighter status/process/display/gameplay paths.
- `gNdsFighterMarioFoxInitMask`: expected low fourteen bits `0x3FFF`. It
  proves source-order initialization of damage/shield, velocities, root DObj
  transform, attribute collision contracts, floor projection, ground/air state,
  jump counters, attack/damage counters, hitstatus/damage kind, throw/catch/item
  pointer clearing, passive vars, bounded compatibility calls, and both fighter
  records.
- `gNdsFighterMarioFoxInitDeferredMask`: expected `0xFF`, documenting that
  status transitions, process callbacks, input/update loops, physics/gameplay,
  hit/catch/search runtime, shadows, display traversal, and full gameplay stay
  deferred.
- `gNdsFighterMarioFoxInitCount`: expected `2`.
- `gNdsFighterInitP0*` / `P1*`: per-fighter init proof details. Current
  expected values are Mario/Fox fkind `0/1`, damage `0`, shield `55`, GA
  `nMPKineticsGround`, jumps `0`, hitstatus normal, damage kind default,
  motion attack ID `0xFFFFFFFF`, floor attempt/result `1/1`, floor line `0`,
  root Y `0`, root scale X from each fighter's source attributes, passive vars
  seeded deterministically, and throw/catch/item clear markers set.
- `gNdsFighterInitDamageCollMask`,
  `gNdsFighterInitDamageCollNormalMask`,
  `gNdsFighterInitDamageCollJointMask`, and
  `gNdsFighterInitDamageCollHalfSizeMask`: expected low bits `0x3` in the
  direct and menu-chain init harnesses. These prove the bounded
  `FTDamageColl[11]` shell copies every valid source descriptor slot for Mario
  and Fox, normal hit status after `ftParamSetHitStatusPartAll`, and the
  original descriptor copy-and-half-size shape.
- `gNdsFighterInitP0DamageColl*` / `P1DamageColl*`: per-fighter damage-coll
  shell details. Current expected count is `10/11`, with selected slot 0 still
  used by the live-hit proof: Mario
  joint `6`, Fox joint `5`, Mario half-size bits
  `51.5/56.0/47.5` (`0x424e0000`, `0x42600000`, `0x423e0000`), and Fox
  half-size bits `51.0/26.0/22.5`
  (`0x424c0000`, `0x41d00000`, `0x41b40000`).
- `gNdsFighterInitDamageCollPartsMask`,
  `gNdsFighterInitDamageCollMatrixMask`, and
  `gNdsFighterInitDamageCollScaleMask`: expected low bits `0x3` in the direct
  and menu-chain init harnesses. These prove the selected Mario/Fox
  damage-coll joint has a project-owned source-compatible root `FTParts` shell,
  the bounded world-position read matches the stored translate matrix, and the
  scale vector is available for later rectangle tests.
- `gNdsFighterInitP0DamageCollWorld*`,
  `gNdsFighterInitP1DamageCollWorld*`,
  `gNdsFighterInitP0DamageCollScaleXBits`, and
  `gNdsFighterInitP1DamageCollScaleXBits`: per-fighter FTParts transform
  details for the selected shell. Current verifier contract requires nonzero
  scale bits and the low mask bits above; natural multi-slot rectangle
  iteration, victim records, hitlag, and damage are still deferred.
- `gNdsFighterInitPhysicsStopCount`,
  `gNdsFighterInitAttackClearCount`,
  `gNdsFighterInitHitStatusPartCount`, and
  `gNdsFighterInitColAnimResetCount`: expected `2` each in the direct and
  menu-chain init harnesses, proving the bounded compatibility call sites were
  reached once per fighter.
- `gNdsFighterInitProcessAttachCount`,
  `gNdsFighterInitStatusSetCount`, and
  `gNdsFighterInitDisplayProbeCount`: expected `0`, proving the init proof did
  not run fighter processes, status changes, or display traversal.
- `gNdsFighterMarioFoxWaitStatusResult`: expected `0x46545753` (`FTWS`) after
  original `ftCommonWaitSetStatus` runs for both initialized Mario/Fox structs
  through the Wait-only `ftMainSetStatus` seam.
- `gNdsFighterMarioFoxWaitMotionResult`: expected `0x4654574D` (`FTWM`) after
  both fighters record Wait motion `4`, animation frame `0`, and animation
  speed `1.0`.
- `gNdsFighterMarioFoxWaitDeferResult`: expected `0x46545744` (`FTWD`) when
  callback pointers are installed but process/display/gameplay execution
  remains parked.
- `gNdsFighterMarioFoxWaitMask`: expected low twelve bits `0xFFF`. It proves
  model/struct/init prereqs, two original Wait calls, two bounded
  `ftMainSetStatus` calls, status `10`, motion `4`, no attack IDs, animation
  frame/speed, callback pointer readiness, special interrupt, player tag wait
  `120`, main motion file readiness, zero callback execution, and zero
  process/display/gameplay escape.
- `gNdsFighterMarioFoxWaitDeferredMask`: expected `0xFF`.
- `gNdsFighterMarioFoxWaitCount`: expected `2`.
- `gNdsFighterWaitP0*` / `P1*`: per-fighter Wait proof details. Expected
  values include previous status `0xFFFFFFFF`, status `10`, motion `4`, motion
  and status attack IDs `0xFFFFFFFF`, frame bits `0`, speed bits
  `0x3F800000`, special interrupt `1`, player tag wait `120`, Wait callback
  pointer readiness `1/1/1`, and main motion readiness `1`.
- `gNdsFighterWaitOriginalSetStatusCallCount`,
  `gNdsFighterWaitFtMainSetStatusCallCount`,
  `gNdsFighterWaitHammerCheckCount`, and
  `gNdsFighterWaitPlayerTagSetCount`: expected `2` each in the direct and
  menu-chain Wait harnesses.
- `gNdsFighterWaitHammerDeniedCount`,
  `gNdsFighterWaitProcInterruptCallCount`,
  `gNdsFighterWaitProcPhysicsCallCount`,
  `gNdsFighterWaitProcMapCallCount`,
  `gNdsFighterWaitProcessAttachCount`,
  `gNdsFighterWaitDisplayProbeCount`, and
  `gNdsFighterWaitGameplayUpdateCount`: expected `0`, proving the proof
  installed Wait behavior but did not execute the fighter runtime loop.
- `gNdsFighterMarioFoxWaitTickResult`: expected `0x4654544B` (`FTTK`) after
  both initialized fighters run one bounded original
  `ftCommonWaitProcInterrupt` callback tick.
- `gNdsFighterMarioFoxWaitCallbackResult`: expected `0x46544342` (`FTCB`)
  after the bounded callback counts reach two original interrupt callbacks,
  two guarded ground-interrupt checks, two guarded physics callbacks, and two
  guarded map callbacks.
- `gNdsFighterMarioFoxWaitSafeResult`: expected `0x46545346` (`FTSF`) when the
  tick proof records zero status changes, motion changes, ground/air drift,
  root-position drift, GObj-count delta, denied non-Wait status changes,
  process attaches, display probes, or gameplay update escapes.
- `gNdsFighterMarioFoxWaitTickMask`: expected low ten bits `0x3FF`. Bits mean:
  `0` prior Wait setup proof passed, `1` two persistent fighter structs and
  GObjs were valid, `2` both fighters had Wait status before the tick, `3`
  both fighters had Wait motion before the tick, `4` both fighters had the
  expected Wait interrupt/physics/map callback pointers, `5` the original Wait
  interrupt callback and guarded ground interrupt ran once per fighter, `6`
  the guarded physics seam ran once per fighter, `7` the guarded map seam ran
  once per fighter, `8` status/motion remained stable, and `9` ground/air
  state, root position, GObj count, and VSBattle scene boundary remained
  stable.
- `gNdsFighterMarioFoxWaitTickDeferredMask`: expected `0xFF`, documenting that
  real fighter update loops, unbounded physics/map mutation, hit/catch/search
  runtime, shadows, full display traversal, and gameplay remain deferred.
- `gNdsFighterMarioFoxWaitTickCount`: expected `2`.
- `gNdsFighterWaitTickP0*` / `P1*`: per-fighter before/after tick details.
  Current expected values are status `10 -> 10`, motion `4 -> 4`, ground/air
  state `0 -> 0`, ground velocity X unchanged at `0`, Mario root X unchanged
  at `0xC2A00000`, Fox root X unchanged at `0x42A00000`, and both root Y
  values unchanged at `0`.
- `gNdsFighterWaitTickOriginalInterruptCount`,
  `gNdsFighterWaitTickGroundInterruptCheckCount`,
  `gNdsFighterWaitTickPhysicsCallbackCount`, and
  `gNdsFighterWaitTickMapCallbackCount`: expected `2` each in the direct and
  menu-chain Wait tick harnesses.
- `gNdsFighterWaitProcInterruptCallCount`,
  `gNdsFighterWaitProcPhysicsCallCount`, and
  `gNdsFighterWaitProcMapCallCount`: expected `2` each only in the Wait tick
  harness modes. The setup-only Wait harnesses still expect these counters to
  stay `0`.
- `gNdsFighterWaitTickStatusChangeCount`,
  `gNdsFighterWaitTickMotionChangeCount`,
  `gNdsFighterWaitTickGADriftCount`,
  `gNdsFighterWaitTickRootDriftCount`,
  `gNdsFighterWaitTickGObjDelta`,
  `gNdsFighterWaitTickDeniedStatusCount`,
  `gNdsFighterWaitTickProcessAttachCount`,
  `gNdsFighterWaitTickDisplayProbeCount`, and
  `gNdsFighterWaitTickGameplayUpdateCount`: expected `0`.
- `gNdsFighterMarioFoxGroundPhysResult`: expected `0x46544750` (`FTGP`)
  after both fighters run the bounded source-order ground-friction and
  air-transfer proof.
- `gNdsFighterMarioFoxGroundMapResult`: expected `0x4654474D` (`FTGM`)
  after both fighters run the safe floor branch of the map/cliff seam.
- `gNdsFighterMarioFoxGroundSafeResult`: expected `0x46544753` (`FTGS`)
  when the ground proof records zero status, motion, ground/air, root, GObj,
  display, or gameplay drift.
- `gNdsFighterMarioFoxGroundMask`: expected low eleven bits `0x7FF`. Bits
  mean: `0` Wait tick prereq passed, `1` two fighters completed the ground
  proof, `2` physics callbacks ran, `3` both ground velocities decreased, `4`
  friction diagnostics were recorded, `5` map callbacks ran, `6` safe floor
  branch ran, `7` Fall/Ottotto remained denied, `8` fighter state stayed
  stable, `9` display/gameplay stayed parked, and `10` VSBattle remained the
  live scene.
- `gNdsFighterMarioFoxGroundDeferredMask`: expected `0xFF`, documenting that
  real fighter process loops, unbounded map/collision mutation,
  hit/catch/search runtime, shadows, full display traversal, and gameplay
  remain deferred.
- `gNdsFighterMarioFoxGroundCount`: expected `2`.
- `gNdsFighterWaitGroundP0*` / `P1*`: per-fighter ground proof details.
  Current expected values include velocity milli `2000 -> 0`, status `10`,
  motion `4`, ground/air state `0`, unchanged root X/Y bits, populated
  material/traction/friction diagnostics, and air-velocity Y `0`.
- `gNdsFighterWaitGroundPhysicsCallbackCount`,
  `gNdsFighterWaitGroundMapCallbackCount`,
  `gNdsFighterWaitGroundMapCheckCount`, and
  `gNdsFighterWaitGroundMapSafeFloorCount`: expected `2` each in the direct
  and menu-chain Wait ground harnesses.
- `gNdsFighterWaitGroundMapFallDeniedCount`,
  `gNdsFighterWaitGroundMapOttottoDeniedCount`,
  `gNdsFighterWaitGroundStatusChangeCount`,
  `gNdsFighterWaitGroundMotionChangeCount`,
  `gNdsFighterWaitGroundGADriftCount`,
  `gNdsFighterWaitGroundRootDriftCount`,
  `gNdsFighterWaitGroundGObjDelta`,
  `gNdsFighterWaitGroundDisplayProbeCount`, and
  `gNdsFighterWaitGroundGameplayUpdateCount`: expected `0`.
- `gNdsFighterMarioFoxDisplayResult`: expected `0x46544450` (`FTDP`) in
  `battle_mariofox_display_probe` and
  `menu_chain_mariofox_display_probe`.
- `gNdsFighterMarioFoxDisplaySafeResult`: expected `0x46544453` (`FTDS`) when
  the probe reaches the display metadata boundary without draw/matrix/gameplay
  escape.
- `gNdsFighterMarioFoxDisplayMask`: expected low eleven bits `0x7FF`. Bits
  mean: `0` Wait ground prereq passed, `1` exactly two guarded display
  callbacks ran, `2` both persistent fighter structs and GObjs were valid,
  `3` both fighters remained in Wait status `10`, `4` both remained grounded,
  `5` both top joints were present, `6` both DObj trees were non-empty, `7`
  metadata traversal completed for both fighter trees, `8` each fighter had a
  display-list candidate or safe parts-pointer signal, `9` motion/root/GObj
  state stayed stable, and `10` draw/matrix/gameplay counters stayed parked.
- `gNdsFighterMarioFoxDisplayDeferredMask`: expected `0x3F`, documenting that
  full `ftdisplaymain.c`, matrix prep, GBI emission, shadows, afterimages/fog,
  magnify/interface rendering, and continuous fighter draw remain deferred.
- `gNdsFighterMarioFoxDisplayCallbackCount`: expected `2`.
- `gNdsFighterDisplayP0DObjCount` / `P1DObjCount`: expected `25/27`.
- `gNdsFighterDisplayP0MObjCount` / `P1MObjCount` and
  `gNdsFighterDisplayP0AObjCount` / `P1AObjCount`: currently `0/0` for the
  bounded Mario/Fox DObj trees. This is recorded metadata, not a pass failure.
- `gNdsFighterDisplayP0DLReadyCount` / `P1DLReadyCount`: expected `14/18`.
- `gNdsFighterDisplayP0PartsPtrCount` / `P1PartsPtrCount`: current safe
  parts-pointer counts from DObj `user_data.p`; these may be `0` while
  display-list candidates are present.
- `gNdsFighterDisplayP0StatusAfter` / `P1StatusAfter`,
  `P0MotionAfter` / `P1MotionAfter`, and `P0GAAfter` / `P1GAAfter`: expected
  `10/10`, `4/4`, and `0/0`.
- `gNdsFighterDisplayP0RootXBeforeBits` / `P0RootXAfterBits` and
  `P1RootXBeforeBits` / `P1RootXAfterBits`: expected unchanged.
- `gNdsFighterDisplayGObjDelta`, `gNdsFighterDisplayDrawCallCount`,
  `gNdsFighterDisplayMatrixCallCount`, and
  `gNdsFighterDisplayGameplayUpdateCount`: expected `0`.
- `gNdsFighterMarioFoxDLScanResult`: expected `0x46544C50` (`FTLP`) in
  `battle_mariofox_dl_scan` and `menu_chain_mariofox_dl_scan`.
- `gNdsFighterMarioFoxDLScanSafeResult`: expected `0x46544C53` (`FTLS`) when
  the parser-only scan reaches both selected fighter display lists without
  object creation, draw, matrix, gameplay, or root/state escape.
- `gNdsFighterMarioFoxDLScanMask`: expected low eleven bits `0x7FF`. It proves
  the display metadata prerequisite passed, both first DL pointers were found,
  both selected DLs had a recognized owner, both scans ran, parser command
  counts and first opcodes were populated, both blocker values are zero, no
  unsupported commands were encountered, fighter state/root/GObj stayed stable,
  draw/matrix/gameplay counters stayed parked, and the scene remained VSBattle
  from Maps.
- `gNdsFighterMarioFoxDLScanDeferredMask`: expected `0xFF`, documenting that
  full fighter rendering, matrix prep, `gcDrawAll`, broad display traversal,
  texture/material upload, shadows, afterimages, and gameplay remain deferred.
- `gNdsFighterDLScanP0FirstDL` / `P1FirstDL`: selected first display-list
  pointers from the real Mario/Fox DObj trees. Current direct proof is
  `0x22dc548/0x2304f10`; current menu-chain proof is `0x22dcbe8/0x23055b0`.
- `gNdsFighterDLScanP0AssetID` / `P1AssetID`: selected DL ownership. Current
  value `0xFFFFFFFE` means the DL is taskman-arena copied original fighter
  data rather than a pointer still inside a registered loaded O2R file.
- `gNdsFighterDLScanP0CommandCount` / `P1CommandCount`: current proof `59/69`.
- `gNdsFighterDLScanP0Blocker` / `P1Blocker`: current proof `0/0`.
- `gNdsFighterDLScanP0FirstOpcode` / `P1FirstOpcode`,
  `UnsupportedOpcode`, `VertexCount`, `TriangleCount`, `EndCommandCount`,
  `BranchCommandCount`, `SegmentResolveCount`, and `TextureMask`: parser stats
  copied from `NDSRendererStats`; the verifier requires populated command and
  opcode evidence but does not assert exact future values beyond the maintained
  current proof.
- `gNdsFighterDLScanP0UnsupportedCommandCount` /
  `P1UnsupportedCommandCount`: expected `0/0`.
- `gNdsFighterDLScanP0OtherModeCommandCount` /
  `P1OtherModeCommandCount`, `CullCommandCount`, `StateCommandCount`,
  `SkipCommandCount`, `RenderCommandCount`, and `MaxDepthSeen`: renderer
  command-family counters copied from `NDSRendererStats`. The scan verifiers
  expose these as `FTR_DL_SCAN_FAMILIES` and require zero unsupported counts
  plus nonzero state/render evidence.
- `gNdsFighterDLScanGObjDelta`, `DrawCallCount`, `MatrixCallCount`, and
  `GameplayUpdateCount`: expected `0`.
- `gNdsFighterDLScanRangeRejectCount` / `BranchResolveCount`: parser guard
  counters for rejected ranges and segmented branch resolution attempts.
- `gNdsFighterMarioFoxDLExecResult`: expected `0x46544C45` (`FTLE`) in
  `battle_mariofox_dl_execute` and `menu_chain_mariofox_dl_execute`.
- `gNdsFighterMarioFoxDLExecSafeResult`: expected `0x46544C58` (`FTLX`) when
  execute/decode reaches both selected fighter display lists without object
  creation, draw, matrix, gameplay, or root/state escape.
- `gNdsFighterMarioFoxDLExecMask`: expected low eleven bits `0x7FF`. Bits
  mean: `0` prior DL scan proof passed with zero blockers, `1` selected
  Mario/Fox DL pointers were reused, `2` `ndsRendererExecuteDisplayList()` ran
  for both lists, `3` command counts and first opcodes were populated, `4`
  vertex commands and decoded vertices were recorded, `5` triangles were
  recorded, `6` non-degenerate bounds were produced, `7` unsupported counts
  stayed zero, `8` status/motion/ground-air/root/GObj state stayed stable,
  `9` draw/matrix/gameplay counters stayed parked, and `10` scene remained
  VSBattle from Maps.
- `gNdsFighterMarioFoxDLExecDeferredMask`: expected `0xFF`, documenting that
  visible DS fighter rendering, matrix/camera projection, material/texture
  upload, all DL-ready DObjs beyond the first, full `ftdisplaymain.c`,
  continuous draw, shadows/magnify/interface/afterimage/fog, and gameplay
  remain deferred.
- `gNdsFighterMarioFoxDLExecCount`: expected `2`.
- `gNdsFighterDLExecP0CommandCount` / `P1CommandCount`: current proof `59/69`.
- `gNdsFighterDLExecP0VertexCommandCount` /
  `P1VertexCommandCount`, `VertexDecodedCount`, `VertexValidMask`,
  `TriangleCommandCount`, `TriangleCount`, and `TriangleValidCount`: decoded
  geometry evidence from the same selected Mario/Fox DLs. Current proof decodes
  `28/23` vertices and `37/20` triangles.
- `gNdsFighterDLExecP0MinX` / `MaxX` / `MinY` / `MaxY` / `MinZ` / `MaxZ`
  and matching P1 fields: decoded vertex bounds; verifiers require
  non-degenerate bounds for both fighters.
- `gNdsFighterDLExecP0ColorChecksum` / `P1ColorChecksum`: nonzero checksum of
  decoded vertex colors.
- `gNdsFighterDLExecP0UnsupportedOpcode` /
  `P1UnsupportedOpcode` and `UnsupportedCommandCount`: expected `0`.
- `gNdsFighterDLExecP0OtherModeCommandCount` /
  `P1OtherModeCommandCount`, `CullCommandCount`, `StateCommandCount`,
  `SkipCommandCount`, `RenderCommandCount`, `BranchCommandCount`,
  `SegmentResolveCount`, and `TextureMask`: execute command-family counters
  exposed by `FTR_DL_EXEC_FAMILIES`.
- `gNdsFighterDLExecP0StatusAfter` / `P1StatusAfter`,
  `MotionAfter`, `GAAfter`, and root-X before/after bitfields: expected
  `10/10`, `4/4`, `0/0`, and unchanged root values.
- `gNdsFighterDLExecGObjDelta`, `DrawCallCount`, `MatrixCallCount`,
  `GameplayUpdateCount`, `RangeRejectCount`, and `VertexRangeRejectCount`:
  expected `0`, proving the execute/decode proof did not escape into object
  creation, DS drawing, matrix prep, gameplay, or invalid memory reads.
- `gNdsFighterMarioFoxDLDrawResult`: expected `0x46544C44` (`FTLD`) in
  `battle_mariofox_dl_draw` and `menu_chain_mariofox_dl_draw`.
- `gNdsFighterMarioFoxDLDrawSafeResult`: expected `0x46544C57` (`FTLW`) when
  the software draw preview reaches both selected fighter display lists without
  object creation, matrix prep, gameplay, or fighter state/root escape.
- `gNdsFighterMarioFoxDLDrawMask`: expected low eleven bits `0x7FF`. Bits
  mean: `0` prior DL execute/decode proof passed, `1` `96x72` original-DL
  preview buffer acquired, `2` `ndsRendererExecuteDisplayList()` ran once for
  Mario and once for Fox, `3` both callbacks decoded nonzero vertices and
  triangles, `4` both fighters selected usable fallback projection axes and
  bounds, `5` both fighters drew at least one triangle, `6` both fighters
  produced nonzero pixels and color checksums, `7` the retained original-DL
  preview committed once and became ready, `8` unsupported/blocker/
  range-reject state stayed zero, `9` status/motion/ground-air/root/GObj state
  stayed stable, and `10` draw/matrix/gameplay counters stayed parked while
  the scene remained VSBattle from Maps.
- `gNdsFighterMarioFoxDLDrawDeferredMask`: expected `0xFF`, documenting that
  camera-correct battle projection, material/texture upload and sampling, all
  fighter DL-ready DObjs beyond the selected first DL, full `ftdisplaymain.c`,
  matrix prep/draw-MObj traversal, DS hardware polygon submission, continuous
  `gcDrawAll`, gameplay, hit/catch/search, items, HUD, shadows, and magnify
  remain deferred.
- `gNdsFighterMarioFoxDLDrawCount`: expected `2`.
- `gNdsFighterDLDrawPreviewWidth` / `Height` / `Pitch` / `Ready` and
  `PreviewCommitBefore` / `PreviewCommitAfter` / `PreviewCommitDelta`: preview
  surface state. Current proof expects `96x72`, pitch at least `96`, ready
  `1`, and exactly one commit delta.
- `gNdsFighterDLDrawP0Blocker` / `P1Blocker`,
  `P0CommandCount` / `P1CommandCount`, `P0FirstOpcode` / `P1FirstOpcode`,
  `P0UnsupportedOpcode` / `P1UnsupportedOpcode`, and
  `P0UnsupportedCommandCount` / `P1UnsupportedCommandCount`: renderer execution
  stats for the draw probe. Current proof requires blockers and unsupported
  counts/opcodes to stay zero and command counts to remain at least `59/69`.
- `gNdsFighterDLDrawP0VertexDecodedCount` / `P1VertexDecodedCount`,
  `P0TriangleCount` / `P1TriangleCount`,
  `P0TriangleValidCount` / `P1TriangleValidCount`,
  `P0TriangleDrawnCount` / `P1TriangleDrawnCount`,
  `P0RealTriangleDrawnCount` / `P1RealTriangleDrawnCount`,
  `P0MarkerTriangleDrawnCount` / `P1MarkerTriangleDrawnCount`,
  `P0PixelCount` / `P1PixelCount`, and `TotalPixelCount`: decoded and
  software-rasterized geometry evidence. `TriangleDrawnCount` is total visible
  software output, while `RealTriangleDrawnCount` excludes degenerate-marker
  fallback triangles and is the maintained proof counter. Current proof records
  `37/20` represented triangles, `37/18` real drawn triangles, `0/2` marker
  triangles, and `4274/5345` pixels after corrected F3DEX2 decode.
- `gNdsFighterDLDrawP0Axis` / `P1Axis`, `P0Area` / `P1Area`, and the
  `P0/P1MinA/MaxA/MinB/MaxB` fields: fallback projection choice and source
  bounds. Axis values are `0..2` for XY/XZ/YZ. Verifiers now require nonzero
  projected area and real non-degenerate triangle output for both fighters;
  bounded degenerate markers remain visible diagnostics only.
- `gNdsFighterDLDrawP0ScreenMinX` / `MaxX` / `MinY` / `MaxY` and matching P1
  fields: preview-space bounds, expected within `0..95` and `0..71`.
- `gNdsFighterDLDrawP0ColorChecksum` / `P1ColorChecksum`: nonzero checksum of
  colors used by the software preview triangles.
- `gNdsFighterDLDrawP0StatusAfter` / `P1StatusAfter`,
  `MotionAfter`, `GAAfter`, and root-X before/after bitfields: expected
  `10/10`, `4/4`, `0/0`, and unchanged root values.
- `gNdsFighterDLDrawGObjDelta`, `DrawCallCount`, `MatrixCallCount`,
  `GameplayUpdateCount`, `RangeRejectCount`, and `VertexRangeRejectCount`:
  expected `0`, proving the draw proof stayed inside the bounded software
  preview and did not enter object creation, DS draw, matrix prep, gameplay, or
  invalid memory reads.
- `gNdsFighterMarioFoxDLMultiDrawResult`: expected `0x46544D55` (`FTMU`) in
  `battle_mariofox_dl_draw_multi` and `menu_chain_mariofox_dl_draw_multi`.
- `gNdsFighterMarioFoxDLMultiDrawSafeResult`: expected `0x46544D56` (`FTMV`)
  when the multi-DL software preview reaches the first four DL-ready Mario and
  Fox DObjs without object creation, matrix prep, gameplay, or fighter
  state/root escape.
- `gNdsFighterMarioFoxDLMultiDrawMask`: expected low eleven bits `0x7FF`. Bits
  mean: `0` prior first-DL draw proof passed, `1` the multi-DL preview buffer
  was acquired, `2` both fighters were processed, `3` candidate and selected
  DObj counts were populated, `4` all selected DObjs decoded cleanly, `5`
  aggregate geometry exceeded the first-DL-only proof, `6` aggregate pixels met
  or exceeded the first-DL-only proof, `7` the retained original-DL preview
  committed once, `8` unsupported/blocker/range-reject state stayed zero, `9`
  status/motion/ground-air/root/GObj state stayed stable, and `10`
  draw/matrix/gameplay counters stayed parked while the scene remained
  VSBattle from Maps.
- `gNdsFighterMarioFoxDLMultiDrawDeferredMask`: expected `0xFF`, documenting
  that full fighter rendering, camera-correct battle projection,
  material/texture upload and sampling, all remaining DL-ready fighter DObjs,
  full `ftdisplaymain.c`, matrix prep/draw-MObj traversal, DS hardware polygon
  submission, continuous `gcDrawAll`, shadows, magnify/interface rendering,
  gameplay, items, HUD, and audio remain deferred.
- `gNdsFighterMarioFoxDLMultiDrawCount`: expected `2`.
- `gNdsFighterDLMultiDrawPreviewWidth` / `Height` / `Pitch` / `Ready` and
  `PreviewCommitBefore` / `PreviewCommitAfter` / `PreviewCommitDelta`: preview
  surface state. Current proof expects `96x72`, pitch at least `96`, ready
  `1`, and exactly one commit delta.
- `gNdsFighterDLMultiDrawP0CandidateCount` / `P1CandidateCount`: full
  DL-ready DObj census for the current Mario/Fox trees, expected `14/18`.
- `gNdsFighterDLMultiDrawP0SelectedCount` / `P1SelectedCount`,
  `AttemptCount`, `CleanCount`, and `DrawnDObjCount`: first-four selected DObj
  proof. Current maintained pass expects `4/4` for each pair.
- `gNdsFighterDLMultiDrawP0FailedCount` / `P1FailedCount`: expected `0/0`.
- `gNdsFighterDLMultiDrawP0SelectedIndexMask` / `P1SelectedIndexMask`: bitmask
  of depth-first traversal indices selected for drawing, expected nonzero for
  both fighters.
- `gNdsFighterDLMultiDrawP0FirstBlocker` / `P1FirstBlocker`,
  `P0BlockerMask` / `P1BlockerMask`, `P0UnsupportedOpcode` /
  `P1UnsupportedOpcode`, and `P0UnsupportedCommandCount` /
  `P1UnsupportedCommandCount`: expected zero, proving all eight selected DObjs
  decoded through known renderer contracts.
- `gNdsFighterDLMultiDrawP0CommandCount` / `P1CommandCount` and
  `P0FirstOpcode` / `P1FirstOpcode`: aggregate renderer execution stats across
  the first four selected DObjs per fighter. Current proof records nonzero
  first opcodes and command counts above the first-DL `59/69` baseline.
- `gNdsFighterDLMultiDrawP0VertexDecodedCount` / `P1VertexDecodedCount`,
  `P0TriangleCount` / `P1TriangleCount`, `P0TriangleValidCount` /
  `P1TriangleValidCount`, `P0TriangleDrawnCount` / `P1TriangleDrawnCount`,
  `P0RealTriangleDrawnCount` / `P1RealTriangleDrawnCount`,
  `P0MarkerTriangleDrawnCount` / `P1MarkerTriangleDrawnCount`,
  `P0PixelCount` / `P1PixelCount`, and `TotalPixelCount`: aggregate
  multi-DL geometry and software-rasterized pixel evidence. Current maintained
  proof is `87/79` represented triangles, `82/76` real drawn triangles, `1/3`
  marker triangles, and `6190/7026` pixels for Mario/Fox after corrected
  F3DEX2 decode.
- `gNdsFighterDLMultiDrawP0Axis` / `P1Axis`, `P0Area` / `P1Area`, and the
  `P0/P1MinA/MaxA/MinB/MaxB` fields: shared per-fighter fallback projection
  choice and source bounds across all clean selected DObjs. Axis values are
  `0..2` for XY/XZ/YZ. Verifiers require nonzero projected area and real
  non-degenerate triangle growth over the first-DL baseline; markers do not
  satisfy this proof.
- `gNdsFighterDLMultiDrawP0ScreenMinX` / `MaxX` / `MinY` / `MaxY` and matching
  P1 fields: preview-space bounds, expected within `0..95` and `0..71`.
- `gNdsFighterDLMultiDrawP0ColorChecksum` / `P1ColorChecksum`: nonzero
  aggregate checksum of colors used by the software preview triangles.
- `gNdsFighterDLMultiDrawP0StatusAfter` / `P1StatusAfter`,
  `MotionAfter`, `GAAfter`, and root-X before/after bitfields: expected
  `10/10`, `4/4`, `0/0`, and unchanged root values.
- `gNdsFighterDLMultiDrawGObjDelta`, `DrawCallCount`, `MatrixCallCount`,
  `GameplayUpdateCount`, `RangeRejectCount`, and `VertexRangeRejectCount`:
  expected `0`, proving the multi-DL draw proof stayed inside the bounded
  software preview and did not enter object creation, DS draw, matrix prep,
  gameplay, or invalid memory reads.
- `gNdsFighterMarioFoxDLAllDrawResult`: expected `0x4654414C` (`FTAL`) in
  `battle_mariofox_dl_draw_all` and `menu_chain_mariofox_dl_draw_all`.
- `gNdsFighterMarioFoxDLAllDrawSafeResult`: expected `0x46544153` (`FTAS`)
  when the guarded all-DL software preview reaches all current DL-ready Mario
  and Fox DObjs without object creation, matrix prep, gameplay, or fighter
  state/root escape.
- `gNdsFighterMarioFoxDLAllDrawMask`: expected low eleven bits `0x7FF`. The
  bit meanings mirror the multi-DL proof, but the prerequisite is the multi-DL
  proof and the selected set is all current DL-ready DObjs.
- `gNdsFighterMarioFoxDLAllDrawDeferredMask`: expected `0xFF`, documenting
  that real `ftdisplaymain.c` traversal, camera-correct projection, material
  and texture upload/sampling, matrix prep, DS hardware polygon submission,
  continuous `gcDrawAll`, shadows, magnify/interface rendering, gameplay,
  items, HUD, and audio remain deferred.
- `gNdsFighterDLAllDrawDisplayCallbackCount`,
  `P0DisplayCallbackCount`, and `P1DisplayCallbackCount`: expected `2`, `1`,
  and `1`, proving the current guarded `ftDisplayMainProcDisplay` seam ran
  exactly once for Mario and once for Fox.
- `gNdsFighterDLAllDrawP0CandidateCount` / `P1CandidateCount`,
  `SelectedCount`, `AttemptCount`, `CleanCount`, and `DrawnDObjCount`: all-DL
  DObj census and draw proof. Current maintained pass expects
  candidates/selected/attempts `14/18`, Mario `14` clean with `0` failed DObjs,
  and Fox `18` clean with `0` failed DObjs under corrected F3DEX2 decode.
- `gNdsFighterDLAllDrawP0FirstFailed*` /
  `gNdsFighterDLAllDrawP1FirstFailed*`: first non-clean all-DL DObj evidence.
  Current maintained pass expects both failed selected/tree indices to remain
  at the sentinel `4294967295`, with reason, blocker, unsupported opcode,
  unsupported command count, vertex-range reject, and failed geometry counters
  all zero.
- `gNdsFighterDLAllDrawP0CommandCount` / `P1CommandCount`,
  `VertexDecodedCount`, `TriangleCount`, `TriangleValidCount`,
  `TriangleDrawnCount`, `RealTriangleDrawnCount`, `MarkerTriangleDrawnCount`,
  `PixelCount`, and `TotalPixelCount`: aggregate all-DL renderer evidence.
  Current maintained proof is `334/322` represented triangles, `306/290` real
  drawn triangles, `10/24` marker triangles, and `14913/13432` pixels for
  Mario/Fox, exceeding the multi-DL baseline.
- `gNdsFighterDLAllDrawP0Axis` / `P1Axis`, source bounds, screen bounds, color
  checksums, status/motion/GA/root fields, and `GObjDelta` /
  `DrawCallCount` / `MatrixCallCount` / `GameplayUpdateCount` /
  `RangeRejectCount` / `VertexRangeRejectCount`: expected to show usable
  preview bounds, nonzero color checksums, stable fighter state, and zero
  bounded-preview escape counters. Collapsed projected triangles are represented
  by bounded software markers until real fighter matrix/camera projection is
  imported, but those markers are counted separately and do not satisfy the
  geometry proof bits.
- `gNdsFighterMarioFoxWalkInputResult` / `WalkSafeResult`: expected
  `0x4654574b` (`FTWK`) and `0x46545746` (`FTWF`) for the direct and
  menu-chain Wait -> Walk input proofs.
- `gNdsFighterMarioFoxWalkInputMask`: required mask `0x7ff`. Bits mean:
  `0` all-DL prerequisite passed, `1` deterministic stick input seeded and
  forward input valid, `2` original Walk input checks succeeded, `3` original
  Wait interrupt routed through the guarded ground-check seam, `4` original
  Walk status selection produced `12/13`, `5` bounded `ftMainSetStatus`
  accepted WalkMiddle/WalkFast and installed callbacks, `6` one Walk interrupt
  callback stayed in Walk, `7` Walk physics generated nonzero ground velocity,
  `8` safe floor-map pass avoided Fall/Ottotto, `9` GA/root/GObj stayed stable,
  and `10` draw/matrix/display/gameplay/process escapes stayed zero.
- `gNdsFighterMarioFoxWalkDeferredMask`: expected `0xff`. Bits document
  deferred continuous process scheduling, root integration outside the bounded
  movement proofs, turn/jump/squat/attack/special/guard/catch interrupts,
  full collision/ledge handling,
  animation event script execution beyond the counted no-op seam, camera-correct
  battle projection, hit/catch/search/item/weapon/HUD/audio runtime, and full
  imported `ftmain.c` status table/manager loop.
- `FTR_WALK_INPUT`: verifier marker for synthetic stick and selected status.
  Current proof expects absolute stick `40/80`, input success `1/1`, and
  selected status `12/13`.
- `FTR_WALK_STATE`: status/motion/GA before and after. Current proof expects
  Wait `10/10`, motion `4/4`, then WalkMiddle/WalkFast `12/13`, motion `6/7`,
  with GA `0/0`.
- `FTR_WALK_CALLBACKS`: expected Wait interrupt, ground-check, original-check,
  original-success, set-status, `ftMainSetStatus`, animation-event, callback
  readiness, and loop-interrupt counts of `2`; deferred interrupt checks must be
  nonzero and remain no-op.
- `FTR_WALK_PHYS`: expected ground velocity before `0/0`, after greater than
  zero, abs-stick velocity, air-transfer, physics callback, map callback, and
  safe-floor counts of `2`, with Fall/Ottotto counts `0/0`.
- `FTR_WALK_ROOT` and `FTR_WALK_SAFE`: root X/Y bit patterns must remain stable
  and GObj, denied/unexpected status, process, display, gameplay, draw, and
  matrix escape counters must stay zero.
- `gNdsFighterMarioFoxWalkLoopResult` / `WalkLoopSafeResult`: expected
  `0x46574c50` (`FWLP`) and `0x46574c53` (`FWLS`) for the direct and
  menu-chain bounded Walk movement-loop proofs.
- `gNdsFighterMarioFoxWalkLoopMask`: required mask `0x7ff`. Bits mean:
  `0` Walk-input prerequisite passed, `1` held forward stick input seeded,
  `2` four held frames ran for both fighters, `3` original Walk interrupt
  callback stayed bounded, `4` original Walk physics produced expected
  velocity, `5` root X integration consumed `physics.vel_air`, `6` bounded map
  seam stayed safe, `7` neutral release returned both fighters to Wait, `8`
  one Wait friction/map settle frame ran, `9` movement evidence was nonzero and
  directionally valid, and `10` escape counters stayed safe.
- `gNdsFighterMarioFoxWalkLoopDeferredMask`: expected `0xff`, carrying the same
  deferred-scope meaning as the Walk-input proof while adding bounded root
  integration only inside the guarded proof helper.
- `gNdsFighterMarioFoxDashRunResult` / `DashRunSafeResult`: expected
  `0x4644524e` (`FDRN`) and `0x46445253` (`FDRS`) for the direct and
  menu-chain bounded Dash -> Run -> RunBrake proofs.
- `gNdsFighterMarioFoxDashRunMask`: required mask `0x3fffffff` for the current
  maintained dash-run proof. It proves the Walk-loop prerequisite, original
  Dash entry from Wait, Dash -> Run, Run -> RunBrake, source-order
  callback/physics/map calls, velocity reduction, directionally valid root
  movement, zero escape counters, bounded attack status/callback branches, the
  attack animation-event seam, the bounded `MakeAttackColl` event decoder, the
  bounded GuardOn/GuardOff/GuardSetOff status/callback proof, bounded Run ->
  TurnRun -> Run status/update proof, and the bounded
  original Guard EscapeF/EscapeB status/update/Wait-handoff proof.
- `DASH_RUN_TURNRUN`: verifier marker group for the bounded original Run ->
  TurnRun -> Run proof imported from `ftcommonturnrun.c`. Current proof expects
  check/success/setstatus/ftmain counts `2/2/2/2`, TurnRun status/motion
  `19/13` for both fighters, final Run status/motion `16/10`, callback mask
  `0xff`, update mask `0xf`, four guarded update ticks total, and LR plus
  ground velocity negation on the first TurnRun update for both fighters. This
  proves the isolated TurnRun branch and animation-end handoff only, not
  continuous player-driven TurnRun movement.
- `DASH_RUN_STATUS`, `DASH_RUN_CALLS`, and `DASH_RUN_MOVE`: verifier marker
  groups for status/motion IDs, original callback/seam call counts, root
  movement, velocity reduction, and safety counters.
- `DASH_RUN_ATTACK_EVENT`: verifier marker group for the bounded
  original-layout `MakeAttackColl` decoder proof. Current proof expects event
  mask/script/no-hit/count `0x1f/0x3f/0x20/10`: ten selected original
  Mario/Fox main-motion command streams decode hitboxes, all six selected
  scripts are reached, and Fox Attack100Start is a real no-hit script. The
  source-order last decoded hitbox is Fox Jab2 with last player/status `1/191`,
  attack state/id/group/joint `1/1/0/14`, damage/size `4/100`, offset
  `0/0/0`, angle `70`, KBG/KBW/BKB `100/0/0`, shield `0`, and flags `0x7`.
  This is a selected-script scan proof only.
- `DASH_RUN_ATTACK_EVENT_POS`: companion marker for the bounded source-order
  hitbox position and selected contact bookkeeping step. Current proof expects mask `0x1ffff`, state `3`, attack
  ID `1`, joint ID `14`, matrix flag/value `0/0`, and hitbox-ID mask low bits
  `0x3`, proving the selected original script scan decoded at least attack-coll
  slots `0` and `1` while the contact slice stays on Fox Jab2 hitbox `1`. It proves the selected
  decoded Fox Jab2 hitbox can run original `New -> Transfer`, write the
  selected result back into `FTStruct.attack_colls[1]`, then run
  `Transfer -> Interpolate` with `pos_prev = old pos_curr`, then pass the
  original broad-phase fighter-range predicate on both current and previous
  positions, then pass one bounded original-compatible rectangle probe against
  the selected Mario hurtbox and one selected
  `gmCollisionCheckFighterAttackDamageCollide`-style decision, then run
  selected damage-record insertion, normal-hit front-half damage/hit-log
  bookkeeping, selected hit-collision FGM table/SFX seam, and selected
  fighter-hitlog stats handoff, then selected source-shaped `ftMainProcParams`
  damage/hitlag scheduling, including the attacker-side `attack_damage` /
  `proc_hit` and `attack_shield_push` / `proc_shield` branches plus the
  selected victim `shield_damage` -> GuardSetOff, shield-break ->
  ShieldBreakFly, reflect-damage break, Fox reflector hit, Ness reflector
  sound, and Ness absorb branches, without enabling
  continuous attack collision, full multi-slot hurtbox collision, full damage
  status lifecycle, effects, real DS audio, positional audio balance, or exact
  Fox/Ness special runtimes.
- `DASH_RUN_PROCPARAMS`: companion marker for the selected
  `ftMainProcParams` damage/hitlag scheduling handoff. Current proof expects
  mask low bits `0xffffffff`, `damage_after == damage_before + damage_queue`,
  positive damage queue, positive queued hitlag damage, positive computed
  hitlag, `is_knockback_paused = TRUE`, and unchanged status before/after.
  The mask bits cover the selected attacker `attack_damage` / `proc_hit`
  branch, selected attacker `attack_shield_push` / `proc_shield` branch,
  selected victim `shield_damage` -> GuardSetOff branch, selected shield-break
  -> ShieldBreakFly branch, selected reflect-damage break branch, selected Fox
  reflector hit branch, selected Ness reflector sound branch, selected Ness
  absorb branch, victim damage input, percent update, damage-status handoff,
  damage-shuffle setup, rumble handoff, hitlag/pause setup, transient
  damage-field clearing, the selected `proc_lagstart` callback tail, and the
  bounded damage-status selector/setup side probes, plus the first
  public-wrapper imported-original `ftCommonDamageUpdateMain`
  catch-resist/keep-hold branch that copies
  damage lag/hitlag to the grabbed fighter and selects
  `nFTDamageKindColAnim`, plus the sibling catch-resist release branch that
  selects `nFTDamageKindStatus` and reaches the bounded damage-status install,
  plus the catch-side non-resist keep-hold branch that updates grabbed fighter
  damage stats before the same bounded damage-status path,
  plus the first capture keep-hold branch that copies damage lag/hitlag back
  to the captured fighter and selects `nFTDamageKindCatch` on the captor,
  plus the public-wrapper imported-original zero-knockback catch branch that
  reaches the damage color-animation seam without changing fighter status,
  plus the capture zero-knockback branch
  that sets captor hitlag, clears captured fighter tap/release input, runs
  `proc_lagstart`, and reaches the same color-animation seam, plus the
  capture-side non-resist keep-hold branch that updates captured fighter damage
  stats before the same bounded damage-status path, plus the capture-side
  keep-hold false branch that routes through lose-grip into the same bounded
  damage-status path without updating thrown damage stats, plus the
  capture-side keep-hold false zero-knockback branch that routes the captor
  through imported original no-damage release damage-var setup, plus the
  catch-side non-resist zero-knockback branch that routes the grabbed fighter
  through imported original damage-release setup before clearing the catch
  link, plus the no-grab/no-capture tail colanim and damage-status branches,
  plus the DK-family heavy-item catch-resist/drop, explicit heavy-item marker,
  and held-item bypass branches.
- `DASH_RUN_PROCPARAMS_RUMBLE`: companion marker for the bounded
  source-shaped `ftMainProcParams` attacker-side `attack_damage` rumble proof
  plus the selected `attack_shield_push` + `attack_rebound` promotion proof.
  Current proof expects mask bits `0x7f`, derived `procRebound=0x1f`, at least
  two captured calls, and final rumble ID/length `10/0`, proving both the
  normal damage-derived rumble branch, the original BatSwing4 special-case
  branch through the project-owned `ftParamMakeRumble` seam, and the
  imported-original ReboundWait status/callback/vector/hitlag/clear effects.
- `DASH_RUN_DAMAGE_SLEEP`: companion marker for the bounded
  `ftCommonDamageUpdateMain` no-grab/no-capture Sleep-element dispatcher
  branch. Current proof expects mask low bits `0x7f`, FuraSleep
  status/motion `165/145`, nonzero color-animation delta, cliff-catch wait
  setup, and proof-local restore.
- `DASH_RUN_DAMAGE_DUST`: companion marker for the bounded public-wrapper
  `ftCommonDamageSetDustEffectInterval` threshold proof. Current proof expects
  mask low bits `0xff` and packed waits `0x123580`, proving the low, mid-low,
  mid, mid-high, high, default air dust interval buckets, public
  imported-original routing, and proof-local restore.
- `DASH_RUN_DAMAGE_DUST_UPDATE`: companion marker for the bounded
  imported-original `ftCommonDamageUpdateDustEffect` public-wrapper runtime
  proof. Current proof expects mask low bits `0x1f`, DustExpandLarge effect
  count `1`, and post-reset wait `5`, proving the nonzero decrement/no-spawn
  path, zero-cross spawn path, interval reset, proof-local field/counter
  restore, and public imported-original routing.
- `DASH_RUN_DAMAGE_HITSTUN_PUBLIC`: companion marker for the bounded
  imported-original `ftCommonDamageDecHitStunSetPublic` public-wrapper proof.
  Current proof expects mask low bits `0xf`, hitstun `0`, and public knockback
  `456000`, proving nonzero hitstun decrement, zero-cross public-knockback
  transfer, proof-local field restore, and public imported-original routing.
- `DASH_RUN_DAMAGE_COLANIM`: companion marker for the bounded
  imported-original `ftCommonDamageCheckElementSetColAnim` public-wrapper proof. Current proof
  expects mask low bits `0x3f`, packed IDs `0x0522020e`, and routed count `4`,
  proving Fire, Electric, Freezing, default damage color-animation routes,
  proof-local restore, and imported-original routing through the public seam.
- `DASH_RUN_DAMAGE_COLANIM_UPDATE`: companion marker for the bounded
  imported-original `ftCommonDamageUpdateDamageColAnim` /
  `ftCommonDamageSetDamageColAnim` public wrapper proof. Current proof expects mask
  low bits `0x1f`, packed IDs `0x020e`, and update count `2`, proving direct
  wrapper routing, struct-field wrapper routing, gated no-update behavior,
  proof-local restore, and imported-original routing.
- `DASH_RUN_DAMAGE_INVINCIBLE`: companion marker for the bounded
  imported-original `ftCommonDamageCheckSetInvincible` public-seam gate proof.
  Current proof expects mask low bits `0x1f`, at least one invincible tic, and
  hitstatus `2`, proving the hitlag gate, knockback-over flag gate,
  timed-invincibility true branch, proof-local restore, and imported-original
  routing through the public seam.
- `DASH_RUN_DAMAGE_LAGUPDATE`: companion marker for the bounded
  imported-original `ftCommonDamageCommonProcLagUpdate` proof. Current proof
  expects mask low bits `0x3f`, positive X delta, and zero Y delta, proving
  the hitlag gate, stick-range gate, tap-buffer gate, active Smash DI root
  translation branch, proof-local restore, and imported-original routing.
- `DASH_RUN_DAMAGE_COMMON_PHYSICS`: companion marker for the bounded
  imported-original `ftCommonDamageCommonProcPhysics` proof. Current proof
  expects mask low bits `0x3f`, nonzero reduced ground/air-friction X velocities,
  negative air-drift Y velocity, cleared attack state `0`, and proof-local
  restore plus imported-original routing.
- `DASH_RUN_DAMAGE_COMMON_CALLBACKS`: companion marker for bounded common
  damage callback routing: imported-original ground and air update
  stay/expiry, imported-original common interrupt ground/fall/hammer branches,
  imported-original AirCommon interrupt, and proof-local restore. Current proof
  expects mask low bits `0x3fff`.
- `DASH_RUN_DAMAGE_LEVELS`: companion marker for bounded imported-original
  `ftCommonDamageGetDamageLevel` public-wrapper threshold routing. Current proof expects mask
  low bits `0x1f`, proving low/mid/high/fly levels at hitstun `0`, `12`, `24`,
  and `32` plus imported-original routing through the public seam.
- `DASH_RUN_DAMAGE_AIR_MAP_WALL`: companion marker for the bounded DamageAir
  wall-map short-circuit proof. Current proof expects mask low bits `0x3f`,
  proving wall collision, original WallDamage helper side effects,
  Passive/DownBounce short-circuit, reflected knockback/LR, imported-original
  `ftCommonDamageAirCommonProcMap` routing, and proof-local restore.
- `DASH_RUN_DAMAGE_FALL_INTERRUPT`: companion marker for the bounded imported
  `ftCommonDamageFallProcInterrupt` source-order proof. Current proof expects
  mask low bits `0x3f`, proving the imported interrupt call reaches
  special-air, attack-air, jump-aerial, and hammer fallback checks, then
  restores local state.
- `DASH_RUN_DAMAGE_SCREEN_FLASH`: companion marker for the bounded
  imported-original `ftCommonDamageCheckMakeScreenFlash` public-wrapper proof. Current proof
  expects mask low bits `0x7f`, packed IDs `0x3a3d3c3b`, and routed count `4`,
  proving the low-knockback no-op plus high-knockback Fire, Electric,
  Freezing, and default screen-flash routes plus imported-original routing
  through the public seam.
- `DASH_RUN_DAMAGE_PUBLIC`: companion marker for the bounded imported-original
  `ftCommonDamageSetPublic` public-wrapper reaction proof. Current proof expects mask
  low bits `0x3f`, public knockback `160000`, and force count `1`, proving the
  angle-window reduction, target public-knockback reset, very-high attacker
  force handoff, default non-forced attacker branch, proof-local field
  restore, and imported-original routing through the public seam.
- `DASH_RUN_DAMAGE_VOICE`: companion marker for the bounded
  `ftCommonDamageInitDamageVars` damage voice SFX branch. Current proof
  expects mask low bits `0xf`, at least two captured calls, and distinct
  threshold/forced FGM IDs, proving both original voice-call gates through the
  project-owned audio stub seam.
- `DASH_RUN_DAMAGE_FLYROLL`: companion marker for the bounded
  `ftCommonDamageInitDamageVars` random FlyRoll branch. Current proof expects
  mask low bits `0x1f`, DamageFlyRoll status/motion `55/48`, and percent at
  least `100`, proving the airborne non-FlyTop percent/RNG selection branch and
  proof-local RNG/state restore.
- `DASH_RUN_DAMAGE_FLYTOP`: companion marker for the bounded
  `ftCommonDamageInitDamageVars` deterministic FlyTop branch. Current proof
  expects mask low bits `0xf`, DamageFlyTop status/motion `54/47`, and angle
  `90`, proving the airborne high-knockback angle-window selection branch and
  proof-local state restore.
- `DASH_RUN_DAMAGE_REPLACE_ELECTRIC`: companion marker for bounded
  `ftCommonDamageInitDamageVars` final replacement plus electric wrapping.
  Current proof expects mask low bits `0x3f`, electric status/motion `50/43`,
  stored replacement status `55`, and dispatch status/motion `55/48`, proving
  replacement survives the wrapper, hitlag blocks the selected electric passive
  callback, and zero-hitlag dispatch reaches the stored replacement before
  proof-local state restore.
- `DASH_RUN_DAMAGE_KIRBYCOPY`: companion marker for the bounded
  `ftCommonDamageInitDamageVars` Kirby copy-loss branch. Current proof expects
  mask bits `0x6`, copy ID `1->8`, and FGM `204`, proving the selected
  copy-loss reset/audio seam while full Kirby copy effect/model-part runtime
  remains deferred.
- `DASH_RUN_DAMAGE_ITEM_HEAVY`: companion marker for the bounded
  `ftCommonDamageUpdateMain` DK-family heavy-item branch. Current proof
  expects mask bits `0x1f`, proving heavy-item predicate selection,
  catch-resist return, drop/status return, and proof-local restore.
- `DASH_RUN_DAMAGE_ITEM_BYPASS`: companion marker for the bounded
  `ftCommonDamageUpdateMain` held-item fallthrough branch. Current proof
  expects mask bits `0x1f`, proving light-item and heavy-non-DK cases bypass
  the heavy-item damage branch, preserve the held item, reach the normal
  color-animation tail through the public imported-original colanim wrapper
  update path, and restore state.
- `DASH_RUN_DAMAGE_KIND`: companion marker for the bounded
  `ftCommonDamageInitDamageVars` source-fidelity guard and selected
  `ftMainProcParams` Twister/proc_trap branch. Current proof expects mask bits
  `0x7f`, proving hit-element setup preserves the separate
  `FTStruct.damage_kind` dispatch enum before/after the setup call and after
  proof-local restore, imported original init/goto routing is reached, Twister
  forces `nFTDamageKindColAnim`, and installed `proc_trap` runs in source
  order.
- `DASH_RUN_DAMAGE_STATUS`: selected `ftcommondamage.c` damage-status selector
  side probe. Current proof expects mask low bits `0x1f`, damage level `0..3`,
  damage index `0..2`, ground status in the BattleShip damage range `37..56`,
  air status in `46..53`, electric status `49` or `50`, and no status change
  on the initial selected scheduling path.
- `DASH_RUN_DAMAGE_SETUP`: selected `ftcommondamage.c` damage-status setup
  side probe. Current proof expects mask low bits `0xffffffff`, original status
  before, installed damage status `37..56`, installed damage motion `31..49`,
  ground/air state `0..1`, positive hitstun with one update-tick decrement
  plus ground DamageCommon expiry into Wait/Ground, non-zero damage velocity,
  original angle-derived `cos/sin` knockback vector routing with source-order
  ground-to-air conversion, and reduced post-physics X velocity from the
  installed damage physics callback. It also requires original-compatible
  passive status dispatch through imported original `ftCommonDamageSetStatus`,
  `proc_passive` ownership for the installed damage status, one bounded
  `ftMainProcUpdateInterrupt`-shaped `ftCommonDamageCheckSetInvincible`
  passive dispatch tick, one bounded `ftCommonDamageSetStatus` electric
  passive status dispatch tick, the sleep-element route through
  `ftCommonDamageGotoDamageStatus` into FuraSleep status/motion plus
  `cliffcatch_wait`, imported original `ftcommonfurasleep.c` breakout timer
  setup, one original FuraSleep update tick, one A-tap plus stick mash
  breakout branch, forced original FuraSleep -> Wait handoff, and the fighter
  color-animation seam, the selected airborne
  DamageFlyRoll physics tail through imported original
  `ftCommonDamageFlyRollUpdateModelPitch`, attack-collision clear for throw-owned low damage
  velocity, the knockback-over status branch clearing the flag and setting
  timed invincibility, the hitlag Smash DI lag-update branch moving the root
  DObj by the original range multiplier and consuming tap-stick buffers,
  the hitlag lifecycle branch decrementing hitlag, keeping knockback paused
  until the terminal tick, calling lag-end, and resuming animation events,
  DamageAir map path through floor collision, passive checks, and the DownBounce
  handoff seam, the damage interrupt path to clear hitstun and reach the DamageFall interrupt
  seam once, then requires the expiry update path to call imported-original
  `ftCommonDamageFallSetStatusFromDamage` and reach DamageFall status
  setup once, one installed DamageFall air-physics tick, one original-shaped
  fast-fall branch through `ftPhysicsApplyFastFall`, one guarded DamageFall
  no-collision map tick, one guarded floor-collision branch that reaches the
  passive checks plus DownBounce seam, one guarded cliff-collision branch
  that reaches the CliffCatch seam, and the setup-tail side proof for public
knockback, damage color animation, screen flash, rumble, dust effect spawn/interval reset,
  player-tag wait, attacker count/knockback, and the original damage interrupt
  hammer-held ground/air branch routing.
  The proof runs through the gated
  `ftCommonDamageGotoDamageStatus` / `ftMainSetStatus` path, installs
  original-named damage callbacks, ticks the update callback once, proves the
  ground DamageCommon expiry branch into Wait/Ground, ticks the physics
  callback once, ticks the interrupt handoff once, ticks the imported-original
  DamageFall status handoff once, ticks the installed DamageFall air-physics callback
  once, ticks the fast-fall branch once, ticks the DamageFall map callback once through the safe
  no-collision branch, ticks it once more through the floor branch, ticks it a
  third time through the cliff branch, runs the sleep-element
  FuraSleep setup/update/Wait-handoff side proof, runs the setup-tail side proof, and
  restores the preview fighter state before the wider Dash-Run chain continues.
- `DASH_RUN_GUARD`: verifier marker group for the bounded original GuardOn
  proof. Current proof expects GuardOn status/motion `152/134`, callback mask
  `0xff`, state mask `0xfffffe0f`, effect/FGM counts for both fighters, last FGM
  `13`, one original GuardOn update tick that leaves both fighters in GuardOn
  with shield decay/release counters decremented once, and one animation-end
  handoff into Guard status `153` with original Guard callbacks installed,
  followed by one original Guard hold update tick that stays in Guard and one
  release tick into GuardOff status/motion `154/135`, then GuardOff completion
  back to Wait/Ground.
- `DASH_RUN_GUARD_SETOFF`: verifier marker group for the bounded original
  GuardSetOff proof. Current proof expects setstatus/ftmain counts `4/4`,
  mask `0xfff`, callback mask `0xff`, setoff frames `20200`, and ground
  velocity `-40400`. It proves both fighters enter GuardSetOff through
  imported original `ftCommonGuardSetOffSetStatus`, install the original
  `ftCommonGuardSetOffProcUpdate` callback, return to Guard when Z is held,
  return to GuardOff when Z is released, and restore Guard before Escape.
- `DASH_RUN_ESCAPE`: verifier marker group for the bounded original Guard
  EscapeF/EscapeB proof. Current proof expects check/success/setstatus/ftmain
  counts `2/2/2/2`, status/motion `156/136` for P0 and `157/137` for P1,
  item-throw buffers `5/5`, callback mask `0x3ff`, state mask `0xff`,
  tick mask `0x3ff`, and interrupt/physics/map counts `2/2/2`. The tick mask
  proves the installed original update consumes `motion_vars.flags.flag1` and
  flips LR, the installed interrupt reaches the light-throw seam, bounded
  physics/map seams are called, and animation end returns to Wait/Ground.
- `gNdsFighterMarioFoxJumpLoopResult` / `JumpLoopSafeResult`: expected
  `0x464a4d50` (`FJMP`) and `0x464a4d53` (`FJMS`) for the direct and
  menu-chain bounded RunBrake -> Wait -> KneeBend -> JumpF proofs.
- `gNdsFighterMarioFoxJumpLoopMask`: required low bits `0x7ff` for downstream
  consumers and `0xfff` for the standalone direct/menu-chain Jump-loop
  harnesses. It proves the Dash/Run prerequisite, RunBrake -> Wait closeout,
  deterministic jump input, original KneeBend interrupt/check path, bounded
  KneeBend updates, original JumpF status setup, air-state switch, six bounded
  air frames, directional root X movement, rising root Y, and zero escape
  counters. The standalone Jump-loop pair also proves one guarded original
  neutral AttackAirN interrupt/status setup.
- `gNdsFighterMarioFoxJumpLoopDeferredMask`: expected `0xff`, documenting
  deferred attack/special/hammer/aerial/landing/fall/cliff/ceiling branches
  while preserving the bounded ascent proof.
- `JUMP_LOOP`: result, safe result, mask, deferred mask, and fighter count.
  Current standalone Jump-loop proof expects
  `0x464a4d50,0x464a4d53,0xfff,0xff,2`; downstream harnesses accept the
  non-mutating `0x7ff` Jump prerequisite.
- `JUMP_INPUT`: seeded stick/button/input-source proof. Current proof uses
  forward stick `40/-40`, C-button tap/release bits, input source `2`, and no
  shorthop.
- `JUMP_STATUS`: RunBrake start, Wait closeout, KneeBend, and JumpF status /
  motion IDs. Current proof expects RunBrake `17/11`, Wait `10/4`, KneeBend
  `20/14`, and JumpF `22/16` for both fighters.
- `JUMP_GA`: ground/air state across start, Wait, KneeBend, Jump, and
  post-air-loop. Current proof expects ground through KneeBend and air after
  JumpF.
- `JUMP_CALLS` and `JUMP_FRAMES`: original callback/seam call counts and
  bounded frame counts. Current proof records two fighters, bounded KneeBend
  frames, six JumpF air frames each, and original Jump/KneeBend setup calls.
- `JUMP_MOVE` and `JUMP_VEL`: root movement and velocity evidence. Current
  maintained proof records root X `-100800/138900`, root Y
  `395400/468000`, and Y velocity `74300/92000 -> 59900/68000`.
- `JUMP_ATTACKAIR`: bounded original neutral aerial proof on top of the
  standalone JumpF loop. Current proof expects one successful original
  `ftCommonAttackAirCheckInterruptCommon` call, one `ftMainSetStatus` entry,
  status/motion `209/184`, air state `1`, motion/status/stat attack IDs
  `12/9/9`, `tics_since_last_z` reset to `65536`, callback mask `0xf`, plus
  one bounded original Link AttackAirLw rehit tick that refreshes attack coll
  IDs `0/1` through the project-owned `ftParamRefreshAttackCollID`
  compatibility seam. The refresh proof expects count `2`, refresh mask
  `0x3`, refreshed state mask `0x7`, and record-clear mask `0x3` for the
  original four-slot attack record table on both refreshed hitboxes. The
  marker now also prints map-landing mask `0x3ff`, proving the installed
  `ftCommonAttackAirProcMap` callback enters the original landing check and
  covers smooth LandingAirN, missing-animation LandingAirNull, skip-landing
  Wait, and plain LandingLight handoffs before restoring the proof-local
  fighter state. The final direction mask must be `0x1f`, proving the same
  imported original interrupt helper selects AttackAirF/B/Hi/Lw and installs
  the Link down-air proc-hit/rehit callback setup through the bounded status
  shell before restore.
- `JUMP_DEFER` and `JUMP_SAFE`: deferred branch and escape counters. Landing,
  Fall, cliff, ceiling, process attach, display probe, gameplay, draw, matrix,
  denied-status, and unexpected-status counters must stay zero where asserted
  by the verifier.
- `gNdsFighterMarioFoxLandingLoopResult` /
  `gNdsFighterMarioFoxLandingLoopSafeResult`: `0x464c4e44` (`FLND`) and
  `0x464c4e53` (`FLNS`) for the direct and menu-chain bounded
  JumpF -> Fall -> LandingLight -> Wait proofs.
- `gNdsFighterMarioFoxLandingLoopMask`: required low bits `0x7ff`. It proves
  the Jump-loop prerequisite, JumpF start, original Fall status setup, original
  LandingLight status setup, Wait closeout, air/ground state transitions,
  original/seam call counts, bounded Fall and LandingLight frames, floor
  crossing/clamp, movement/velocity evidence, and zero escape counters.
- `gNdsFighterMarioFoxLandingLoopDeferredMask`: expected `0xff`, documenting
  deferred Fall/Landing interrupt branches while preserving the bounded
  ground-air-ground proof.
- `LAND_LOOP`: result, safe result, mask, deferred mask, and fighter count.
  Current maintained proof expects `0x464c4e44,0x464c4e53,0x7ff,0xff,2`.
- `LAND_STATUS`, `LAND_MOTION`, and `LAND_GA`: JumpF start, Fall,
  LandingLight, and Wait status/motion/ground-air states. Current proof expects
  statuses `22,26,31,10`, motions `16,20,25,4`, and air -> air -> ground ->
  ground for both fighters.
- `LAND_CALLS` and `LAND_FRAMES`: original callback/seam call counts and
  bounded frame counts. Current proof records two guarded `ftAnimEndSetFall`
  calls, two original Fall setups, two original LandingLight setups, two
  `ftAnimEndSetWait` closeouts, Fall frames `16/14`, and LandingLight frames
  `5/5`.
- `LAND_MAP`, `LAND_MOVE`, and `LAND_VEL`: floor crossing, floor clamp,
  directional root movement, downward velocity before landing, and bounded
  ground/Wait friction evidence. Current proof clamps both fighters to floor
  `0/0`.
- `LAND_SAFE`: deferred interrupt count, fastfall checks, GObj delta, and
  escape counters. GObj delta, denied/unexpected status, process attach,
  display probe, gameplay, draw, matrix, root-Y drift, and GA drift must remain
  zero.
- `FTR_WALK_LOOP`, `FTR_WALK_LOOP_INPUT`, `FTR_WALK_LOOP_FRAMES`,
  `FTR_WALK_LOOP_STATE`, `FTR_WALK_LOOP_MOVE`, `FTR_WALK_LOOP_VEL`,
  `FTR_WALK_LOOP_RELEASE`, `FTR_WALK_LOOP_MAP`, and `FTR_WALK_LOOP_SAFE`:
  verifier marker family for the movement-loop proof. Current evidence is
  `frames=4/4`, root X delta `48000/-144000`, held velocity `12000/36000`,
  post-settle velocity `6000/28000`, Walk status/motion `12/13` and `6/7`,
  release-to-Wait status/motion `10/10` and `4/4`, stable root Y/GA/GObj
  state, and zero display/draw/matrix/gameplay/process/status escape counters.
- `PLATFORM_DL_PREVIEW`: verifier marker for `gNdsOriginalDLPreviewReady`,
  width, height, commit count, and draw count. The DL draw verifiers require
  ready `1`, dimensions `96x72`, and a commit count at least as large as the
  draw proof's recorded commit-after value.
- `NDSRendererStats` now includes `cull_command_count`,
  `ignored_state_command_count`, `first_othermode_opcode`,
  `first_othermode_w0`, `first_othermode_w1`, `first_cull_w0`, and
  `first_cull_w1`. The renderer records known benign othermode/cull/image/TLUT
  commands as state/skip evidence; unknown/default opcodes still report
  `NDS_RENDERER_BLOCKER_UNSUPPORTED`.
- Verifier marker families:
  `FTR_DL_SCAN_FAMILIES`, `FTR_DL_EXEC`, `FTR_DL_EXEC_COUNTS`,
  `FTR_DL_EXEC_FAMILIES`, `FTR_DL_EXEC_BOUNDS`, `FTR_DL_EXEC_STATE`, and
  `FTR_DL_EXEC_SAFE`, plus `FTR_DL_DRAW`, `FTR_DL_DRAW_PREVIEW`,
  `FTR_DL_DRAW_STATS`, `FTR_DL_DRAW_GEOM`, `FTR_DL_DRAW_RENDER`,
  `FTR_DL_DRAW_AXIS`,
  `FTR_DL_DRAW_SCREEN`, `FTR_DL_DRAW_STATE`, `FTR_DL_DRAW_SAFE`,
  `FTR_DL_MULTI`, `FTR_DL_MULTI_PREVIEW`, `FTR_DL_MULTI_COUNTS`,
  `FTR_DL_MULTI_STATS`, `FTR_DL_MULTI_GEOM`, `FTR_DL_MULTI_RENDER`,
  `FTR_DL_MULTI_AXIS`,
  `FTR_DL_MULTI_SCREEN`, `FTR_DL_MULTI_STATE`, `FTR_DL_MULTI_SAFE`,
  `FTR_DL_ALL`, `FTR_DL_ALL_CALLBACKS`, `FTR_DL_ALL_PREVIEW`,
  `FTR_DL_ALL_COUNTS`, `FTR_DL_ALL_STATS`, `FTR_DL_ALL_GEOM`,
  `FTR_DL_ALL_RENDER`,
  `FTR_DL_ALL_AXIS`, `FTR_DL_ALL_SCREEN`, `FTR_DL_ALL_STATE`,
  `FTR_DL_ALL_SAFE`, `FTR_DL_ALL_FAIL`, `FTR_WALK`, `FTR_WALK_INPUT`,
  `FTR_WALK_STATE`, `FTR_WALK_CALLBACKS`, `FTR_WALK_PHYS`, `FTR_WALK_ROOT`,
  `FTR_WALK_SAFE`, `FTR_WALK_LOOP*`, and `PLATFORM_DL_PREVIEW`.
- `gNdsSCVSBattleStageResult`: expected `0x50555042` (`PUPB`) for the Pupupu
  VSBattle stage-data adoption proof.
- `gNdsSCVSBattleStageMask`: expected `0xFF`. Bits mean: `0` battle gkind is
  Pupupu, `1` GRPupupuMap resolved, `2` `MPGroundData` resolved, `3` geometry
  pointer present, `4` map nodes or map objects present, `5` light angles
  copied, `6` BGM/stage metadata recorded, `7` full collision/yakumono runtime
  explicitly deferred.
- `gNdsSCVSBattleStageGKind`: expected `6` for Pupupu.
- `gNdsSCVSBattleStageGroundDataReady` / `GeometryReady` /
  `MapNodesReady`: expected `1` in `battle_pupupu_stage` and menu-chain
  VSBattle proof.
- `gNdsSCVSBattleStageBGM` / `LightAngleXBits` / `LightAngleYBits`:
  raw stage metadata copied into the battle setup proof.
- `gNdsSCVSBattleStageDeferredMask`: expected low bits `0x3`, documenting
  deferred full collision line processing and yakumono/stage object runtime.
- `gNdsPupupuGroundSetupResult`: expected `0x50554753` (`PUGS`) after the
  project-owned `grCommonSetupInitAll` wrapper enters the Pupupu-only imported
  original path and completes bounded setup.
- `gNdsPupupuGroundDisplayResult`: expected `0x50554744` (`PUGD`) after
  imported original `grDisplayMakeGeometryLayer` creates one or more display
  layer GObjs.
- `gNdsPupupuGroundGObjResult`: expected `0x5055474F` (`PUGO`) after the
  imported original Pupupu map GObj graph is recorded.
- `gNdsPupupuGroundSetupMask`: expected low ten bits `0x3FF`. Bits mean:
  `0` Pupupu VSBattle stage-data precondition valid, `1` wrapper entered the
  Pupupu path, `2` original display-layer setup path ran, `3` display-layer
  GObjs exist, `4` original `grMainSetupMakeGround` dispatch reached Pupupu,
  `5` original `grPupupuMakeGround` created the root stage GObj, `6` original
  `grPupupuInitAll` resolved `map_head`, `7` four Pupupu map GObjs were
  created, `8` DObj/MObj/animation pointer proof recorded, `9` particle bank
  compatibility stub reached.
- `gNdsPupupuGroundDeferredMask`: documents parked runtime behavior. Current
  expected low bits include real particle banks, continuous stage update/draw,
  yakumono runtime, item/effect appear actors, and Whispy/fighter push if that
  runtime path is touched. This is boundary documentation, not a pass/fail
  completion mask.
- `gNdsPupupuGroundLayerGObjCount`: expected at least `1`; current direct and
  menu-chain proofs create `4` original display-layer GObjs.
- `gNdsPupupuGroundLayerGObjMask` / `LayerDObjMask` / `LayerMObjMask` /
  `LayerAnimMask`: pointer-presence proof for original display-layer setup.
- `gNdsPupupuGroundMapGObjCount`: expected `4`.
- `gNdsPupupuGroundMapGObjMask`: expected low four bits `0xF`, proving
  Whispy eyes, Whispy mouth, back flowers, and front flowers GObjs were
  created.
- `gNdsPupupuGroundMapHeadReady`: expected `1`.
- `gNdsPupupuGroundMapHeadOffset`: expected `0x10F0`.
- `gNdsPupupuGroundRootGObjID`: original Pupupu root ground GObj ID.
- `gNdsPupupuGroundWhispyEyesGObjID` /
  `WhispyMouthGObjID` / `FlowersBackGObjID` / `FlowersFrontGObjID`:
  nonzero IDs for the imported original map GObjs.
- `gNdsPupupuGroundParticleBankID`: expected nonzero in the bounded proof;
  the backing particle bank is still a compatibility stub.
- `gNdsPupupuGroundGObjCountBefore` / `GObjCountAfter`: `After` must be
  greater than `Before` in `battle_pupupu_stage` and menu-chain VSBattle.
- `gNdsPupupuGroundDObjCountAfter` / `MObjCountAfter` /
  `AObjCountAfter`: setup-time object-manager evidence. `AObjCountAfter` is a
  pointer-presence boolean because the imported object manager does not export
  an AObj active counter.
- `gNdsPupupuGroundNonPupupuStubCallCount`: expected `0` in Pupupu verifiers,
  proving the original Pupupu dispatch path was used instead of the
  non-Pupupu compatibility stage stubs.
- `NDS_PUPUPU_UPDATE_PASS`: `0x50555550` (`PUUP`), the pass marker for the
  guarded bounded original Pupupu update probe.
- `gNdsPupupuUpdateResult`: expected `NDS_PUPUPU_UPDATE_PASS` in
  `battle_pupupu_update` and `menu_chain_pupupu_update`; expected `0` in the
  setup-only `battle_pupupu_stage` and `menu_chain_vsbattle` verifiers.
- `gNdsPupupuUpdateMask`: expected `0xFF`. Bits mean: `0` prior Pupupu setup
  proof is complete, `1` deterministic safe state was seeded, `2` first update
  moved Whispy from Sleep to Wait, `3` second update stayed in Wait and
  decremented wind wait, `4` four map GObjs survived, `5` guarded side effects
  stayed zero and flowers stayed default, `6` GObj count was unchanged, and
  `7` scene state remained VSBattle from Maps.
- `gNdsPupupuUpdateTickCount`: expected `2` in update harnesses and `0` in
  setup-only Pupupu stage/menu-chain verifiers.
- `gNdsPupupuUpdateGameStatusBefore` / `After`: expected `0 -> 1`
  (`Wait -> Go`) for the guarded update proof.
- `gNdsPupupuUpdateWhispyStatusBefore` / `AfterFirst` / `AfterFinal`:
  expected `0 -> 1 -> 1` for Sleep -> Wait, then Wait.
- `gNdsPupupuUpdateWindWaitBefore` / `AfterFirst` / `AfterFinal`: expected
  `2 -> 2 -> 1`.
- `gNdsPupupuUpdateBlinkWaitBefore` / `AfterFinal`: expected `3 -> 1`.
- `gNdsPupupuUpdateFlowersBackStatusBefore` / `AfterFinal` and
  `gNdsPupupuUpdateFlowersFrontStatusBefore` / `AfterFinal`: expected default
  flower state before and after the guarded proof.
- `gNdsPupupuUpdateMapGObjMaskBefore` / `After`: expected `0xF -> 0xF`.
- `gNdsPupupuUpdateGObjCountBefore` / `After`: expected unchanged.
- `gNdsPupupuUpdateVelPushCount`, `gNdsPupupuUpdateQuakeCount`,
  `gNdsPupupuUpdateParticleScriptCount`, and
  `gNdsPupupuUpdateWindFGMCount`: expected `0`, proving the bounded safe path
  did not enter fighter push, quake, particle script, or Whispy wind FGM side
  effects.
- `gNdsControllerPollCount`: DS SI/controller polls, must be nonzero.
- `gSYControllerMain`: original global controller state after update.

Video:

- `gNdsVideoBootstrapResult`: original video bootstrap result, expected
  `0x56494430`.

NitroFS and O2R payload loading:

- `gNdsRelocAssetInitResult`: NitroFS initialization marker, expected
  `0x4E465349` (`NFSI`).
- `gNdsRelocAssetHeaderReadCount`: O2R header reads. Runtime verification
  expects at least `16`; the current full-opening staged set reads headers for
  startup, Opening Room, Opening Portraits, name cards, action previews, and
  Title resources.
- `gNdsRelocAssetPayloadReadCount`: O2R payload copies. The full runtime
  verifier expects at least `33`; the current Pupupu-stage-enabled runtime
  reads `38`. The early Opening Room skip verifier expects `12` because it
  loads Startup, Opening Room, and bounded Title preview resources.
- `gNdsRelocAssetOpenFailCount`, `gNdsRelocAssetFormatFailCount`, and
  `gNdsRelocAssetShortReadCount`: loader failure counters, expected `0`.

Scene and startup:

- `gNdsSceneBoundaryResult`: scene boundary marker, expected `0x53434E45`.
- `gNdsSceneBoundaryKind`: scene at the current DS parking boundary, expected
  `28` at the bounded Opening Room draw/preview boundary after the tick-560 Scene 2
  camera boundary.
- `gNdsStartupTaskmanResult`: startup reached task-manager start, expected
  `0x53545254`.
- `gNdsStartupTaskmanSceneKind`: startup scene kind, expected `27`.
- `gNdsStartupTaskmanDL0Size`: original startup DL buffer 0 size, expected
  `10240`.
- `gNdsStartupTaskmanDL1Size`: original startup DL buffer 1 size, expected
  `10240`.
- `gNdsStartupTaskmanControllerSet`: startup task setup used original
  `syControllerFuncRead`, expected `1`.
- `gNdsStartupFuncStartResult`: startup `func_start` ran, expected
  `0x46535452`.
- `gNdsStartupSkipAllowWait`: original skip delay after `mnStartupFuncStart`,
  expected `8`.
- `gNdsStartupProceedOpening`: original opening flag after startup init,
  expected `0`.
- `gNdsStartupGObjCreateCount`: startup created actor, default camera,
  wallpaper camera, and wallpaper GObjs through the imported object manager,
  expected `4`.
- `gNdsStartupCameraCreateCount`: startup requested default and wallpaper
  cameras, expected `2`.
- `gNdsStartupRelocInitCount`: startup initialized relocation once, expected
  `1`.
- `gNdsStartupSpriteCreateCount`: startup made one logo sprite object,
  expected `1`.
- `gNdsStartupFadeCreateCount`: startup requested one fade actor, expected `1`.
- `gNdsStartupWallpaperParentValid`: logo SObj parent link points at the
  wallpaper GObj, expected `1`.
- `gNdsStartupLogoPosX` / `gNdsStartupLogoPosY`: original logo position,
  expected `96` and `220`.
- `gNdsStartupLogoFastcopyCleared`: startup cleared `SP_FASTCOPY`, expected
  `1`.
- `gNdsStartupLogoRelocResult`: startup `N64Logo` O2R load/fixup marker,
  expected `0x4C524C43` (`LRLC`).
- `gNdsStartupLogoRelocSize`: startup `N64Logo` payload size, expected
  `29712`.
- `gNdsStartupLogoRelocWordSwapCount`: word byte-swaps for the startup logo
  payload, expected `7428`.
- `gNdsStartupLogoRelocPointerFixupCount`: internal pointer slots patched for
  the startup logo payload, expected `9`.
- `gNdsStartupLogoDrawResult`: bounded startup logo draw marker, expected
  `0x4C445257` (`LDRW`).
- `gNdsStartupLogoDrawBlocker`: startup logo draw blocker enum, expected `0`.
- `gNdsStartupLogoDrawCallbackCount`: number of times `lbCommonDrawSObjAttr`
  ran for the startup logo path, expected at least `1`.
- `gNdsStartupLogoDrawUpdateCount`: bounded update index used for the draw
  sample, expected `17`.
- `gNdsStartupLogoDrawWidth` / `Height`: converted Sprite size, expected
  `128` / `108`.
- `gNdsStartupLogoDrawFormat` / `Size` / `Bitmaps`: Sprite format metadata,
  expected `0` / `2` / `8` for RGBA16 chunks.
- `gNdsStartupLogoDrawPixels`: converted pixel count, expected greater than
  `1000` and currently `13824`.
- `gNdsStartupLogoDrawSObjAttr`: original SObj Sprite attributes sampled from
  the draw callback; the verifier requires `SP_TEXSHUF` (`0x200`) to remain set
  for the startup logo asset.
- `gNdsStartupLogoDrawTexshuf` /
  `gNdsStartupLogoDrawTexshufSamples`: prove the DS startup preview applied the
  inverse odd-row TMEM line swizzle for the original `SP_TEXSHUF` Sprite strips;
  expected `1` and more than `1000` swizzled samples.
- `gNdsStartupActorFuncSet`: actor GObj keeps `mnStartupActorFuncRun`, expected
  `1`.
- `gNdsStartupWallpaperProcessKind`: wallpaper process kind, expected `0`
  (`nGCProcessKindThread`).
- `gNdsStartupWallpaperProcessPriority`: wallpaper process priority, expected
  `1`.
- `gNdsStartupWallpaperDisplaySet`: wallpaper display callback/link/tag
  survived the imported object-manager setup, expected `1`.
- `gNdsStartupWallpaperCameraMaskLow`: wallpaper camera mask low bits, expected
  `1`.
- `gNdsStartupDefaultCameraColor`: default camera fill color, expected `0xFF`.
- `gNdsTaskmanBridgeResult`: startup reached the DS task-loop parking seam,
  expected
  `0x5441534B`.
- `gNdsTaskmanContexts`: original startup task contexts, expected `2`.
- `gNdsTaskmanTaskGfxNum`: original startup task gfx count, expected `1`.
- `gNdsTaskmanGraphicsHeapSize`: per-context graphics heap size, expected
  `10240`.
- `gNdsTaskmanRdpKind`: startup RDP output buffer kind after taskman setup,
  expected `2`.
- `gNdsTaskmanRdpBufferSize`: startup RDP output buffer size, expected `49152`.
- `gNdsStartupTaskmanMallocCount`: real imported `syTaskmanMalloc` calls
  through startup task/object setup, expected `36`.
- `gNdsTaskmanMallocCount`: cumulative imported taskman allocations across
  Startup and Opening Room; it must exceed the startup snapshot.
- `gNdsTaskmanGeneralHeapUsed`: imported taskman general heap usage, expected greater
  than `90000`.
- `gNdsTaskmanDLContextsValid`: display-list buffers allocated for both startup
  contexts, expected `2`.
- `gNdsTaskmanControllerAutoRead`: startup represented BattleShip's controller
  auto-read contract, expected `1`.
- `gNdsTaskmanSceneUpdateSet`, `gNdsTaskmanSceneDrawSet`,
  `gNdsTaskmanLightsSet`: startup task callbacks preserved, expected `1`.
- `gNdsTaskmanLoopReached`: `syTaskmanLoadScene` reached the bounded DS
  `syTaskmanRunTask` seam, expected `1`.
- `gNdsTaskmanBoundedUpdateCount`: bounded original startup updates completed
  inside the DS seam, expected `55`.
- `gNdsTaskmanPostUpdateSkip`: startup actor skip delay after the bounded
  updates, expected `0`.
- `gNdsTaskmanGObjThreadSleeps`: logo GObj thread entered
  `gcSleepCurrentGObjThread`, expected `55`.
- `gNdsTaskmanPostUpdateLogoPosX` / `gNdsTaskmanPostUpdateLogoPosY`: logo SObj
  position after bounded updates, expected `96` and `65`.
- `gNdsTaskmanPostUpdateOpening`: original logo thread set
  `sMNStartupIsProceedOpening`, expected `1`.
- `gNdsTaskmanPostUpdateSceneKind`: requested next scene, expected `28`
  (`nSCKindOpeningRoom`).
- `gNdsTaskmanPostUpdateScenePrev`: previous scene, expected `27`
  (`nSCKindStartup`).
- `gNdsTaskmanPostUpdateStatus`: original taskman status after the scene
  request, expected `1` (`nSYTaskmanStatusLoadScene`).
- `gNdsTaskmanPostUpdateGObjCount`: active GObjs after the original break/eject
  path, expected `0`.
- `gNdsTaskmanPostUpdateFadeCount`: startup fade requests after the logo
  thread runs, expected `2`.
- `gNdsTaskmanCleanupResult`: taskman cleanup completion marker, expected
  `0x434C4E50` (`CLNP`).
- `gNdsTaskmanCleanupQueuesEmpty`: context/reset/game-tic queues drained after
  cleanup, expected `1`.
- `gNdsTaskmanCleanupMode`: original taskman terminal mode after cleanup,
  expected `2`.
- `gNdsTaskmanReturnCount`: bounded taskman seam returned to its caller,
  expected `1`.
- `gNdsOpeningRoomDispatchCount`: original scene manager dispatched
  `mvOpeningRoomStartScene`, expected `1`.
- `gNdsOpeningRoomStartResult`: imported scene entry marker, expected
  `0x4F525354` (`ORST`).
- `gNdsOpeningRoomFuncStartResult`: imported scene `func_start`, expected
  `0x4F524653` (`ORFS`).
- `gNdsOpeningRoomRelocResult`: Opening Room relocation-list marker, expected
  `0x4F52524C` (`ORRL`).
- `gNdsOpeningRoomRelocInitCount`: Opening Room called `lbRelocInitSetup`,
  expected `1`.
- `gNdsOpeningRoomRelocLoadCount`: number of original Opening Room file IDs
  passed through `lbRelocLoadFilesListed`, expected `8`.
- `gNdsOpeningRoomRelocFileMask`: bitmask of resolved Opening Room file IDs,
  expected `0xFF`.
- `gNdsOpeningRoomRelocHeaderMask`: bitmask of validated Opening Room O2R
  headers, expected `0xFF`.
- `gNdsOpeningRoomRelocPayloadMask`: bitmask of copied Opening Room O2R
  payloads, expected `0xFF`.
- `gNdsOpeningRoomRelocContentReady`: payload-byte readiness, expected `1`
  after the NitroFS/O2R load milestone.
- `gNdsOpeningRoomRelocFixupReady`: full mixed-width struct and
  renderer-specific asset fixup readiness, expected `0` until those phases
  exist.
- `gNdsOpeningRoomRelocBytesLoaded`: total copied Opening Room payload bytes,
  expected `329248`.
- `gNdsOpeningRoomRelocLastFileID` / `gNdsOpeningRoomRelocLastSize`: last file
  loaded in the current list, expected `90` / `158928`.
- `gNdsOpeningRoomRelocWordSwapMask`: bitmask of current O2R files whose
  payload words were blanket byte-swapped from N64 big-endian to ARM9 native
  order, expected `0xFF`.
- `gNdsOpeningRoomRelocWordSwapCount`: swapped `u32` words across the current
  eight payloads, expected `82312`.
- `gNdsOpeningRoomRelocWordSwapFailCount`: invalid staged file counter for the
  blanket word-swap pass, expected `0`.
- `gNdsOpeningRoomRelocPointerFixupMask`: bitmask of current O2R files whose
  internal relocation pointer chains were patched, expected `0xFF`.
- `gNdsOpeningRoomRelocPointerFixupCount`: patched internal pointer slots,
  expected `711`.
- `gNdsOpeningRoomRelocPointerFixupFailCount`: malformed chain or unsupported
  external dependency counter, expected `0`.
- `gNdsOpeningRoomRelocSymbolResolveCount`: selected `ll...` offset probes
  plus the real original Scene 1/Scene 2 camera, logo-camera, logo,
  boss-shadow, pencils, Outside, Haze, sunlight, Desk, and spotlight lookups
  resolved through `ndsRelocGetFileData`, expected `38` in the normal no-input
  path.
- `gNdsOpeningRoomRelocSymbolResolveFailCount`: selected symbol probe failures,
  expected `0`.
- `gNdsOpeningRoomRelocSymbolProbeMask`: selected BattleShip symbol probe mask,
  expected `0x7FFFF`.
- `gNdsOpeningRoomRelocLastSymbolOffset`: final normal-path lookup offset,
  expected `0` (`llMVOpeningRoomScene2CamAnimJoint`).
- `gNdsOpeningRoomRelocMObjSubNormalizeCount` /
  `MObjSubNormalizeFailCount` / `MObjSubFirstFlags`: narrow MVCommon
  `MObjSub` mixed-width normalization evidence for the current material probe,
  expected `18`, `0`, and `0x200`.
- `gNdsOpeningRoomRelocMObjSubSourceResult`: ORMT source-scan marker, expected
  `0x4F524D54`.
- `gNdsOpeningRoomRelocMObjSubTextureFlagCount`: normalized records whose
  effective flags include `MOBJ_FLAG_TEXTURE`, expected `0`.
- `gNdsOpeningRoomRelocMObjSubZeroFlagCount`: normalized records that fell
  back to zero flags, expected `0`.
- `gNdsOpeningRoomRelocMObjSubPrimColorCount`: normalized records whose
  effective flags include `MOBJ_FLAG_PRIMCOLOR`, expected `18`.
- `gNdsOpeningRoomRelocMObjSubLightCount`: normalized records whose effective
  flags include the light bit, expected `1`.
- `gNdsOpeningRoomRelocMObjSubFirstTextureOffset` /
  `gNdsOpeningRoomRelocMObjSubFirstTextureFlags`: first texture-bearing source
  offset/flags if one is present, expected `0xFFFFFFFF` and `0`.
- `gNdsOpeningRoomFirstEventResult`: first tick-280 asset-reference probe
  marker, expected `0x4F524631` (`ORF1`).
- `gNdsOpeningRoomFirstEventTick`: original event tick proven by the probe,
  expected `280`.
- `gNdsOpeningRoomFirstEventProbeMask`: pencils DObj/animation readiness mask,
  expected `0x3`.
- `gNdsOpeningRoomFirstEventPencilsDObjOffset`: BattleShip
  `llMVCommonRoomPencilsDObjDesc` offset resolved through the DS backend,
  expected `44728`.
- `gNdsOpeningRoomFirstEventPencilsAnimOffset`: BattleShip
  `llMVCommonRoomPencilsAnimJoint` offset resolved through the DS backend,
  expected `44912`.
- `gNdsOpeningRoomFirstEventDataResult`: first tick-280 pencils data-shape
  marker, expected `0x4F524644` (`ORFD`).
- `gNdsOpeningRoomFirstEventDataMask`: descriptor/table readiness mask,
  expected `0xF`.
- `gNdsOpeningRoomFirstEventPencilsDObjEntries`: resolved pencils DObjDesc
  entries including the terminator, expected `4`.
- `gNdsOpeningRoomFirstEventPencilsDLPtrs`: in-payload display-list pointers
  reachable from the first three DObjDesc entries, expected `3`.
- `gNdsOpeningRoomFirstEventPencilsAnimJoints`: in-payload animation script
  pointers reachable from the animation table, expected `3`.
- `gNdsOpeningRoomFirstEventPencilsAnimFirstOpcode`: first animation command
  opcode, expected `3` (`nGCAnimEvent32SetValBlock`).
- `gNdsOpeningRoomFirstEventRunResult`: tick-280 update callback marker,
  expected `0x4F523238` (`OR28`).
- `gNdsOpeningRoomFirstEventDeferredMask`: deferred first-event bitmask,
  expected `0x01` because fighter creation is the only deferred branch.
- `gNdsOpeningRoomFighterDeferredResult`: deferred fighter boundary marker,
  expected `0x4F524646` (`ORFF`).
- `gNdsOpeningRoomFighterDeferredKind`: fighter kind that would have been
  passed to `mvOpeningRoomMakePulledFighter`, expected to equal
  `gNdsOpeningRoomPulledFighterKind`.
- `gNdsOpeningRoomOverlayCreateResult`: original logo-wallpaper overlay setup
  marker, expected `0x4F524F43` (`OROC`).
- `gNdsOpeningRoomOverlayDisplaySet`: overlay display callback/link readiness,
  expected `1`.
- `gNdsOpeningRoomOverlayAlphaInit`: original overlay alpha initialization,
  expected `255`.
- `gNdsOpeningRoomOverlayCreateGObjCount`: setup-time object count after
  overlay creation, expected `8`.
- `gNdsOpeningRoomOverlayEjectResult`: original overlay ejection marker,
  expected `0x4F524F45` (`OROE`).
- `gNdsOpeningRoomOverlayEjectBeforeGObjCount` /
  `gNdsOpeningRoomOverlayEjectAfterGObjCount`: original
  `gcGetGObjsActiveNum` samples around `gcEjectGObj`, expected `14` / `14`
  because that helper includes free-list objects.
- `gNdsOpeningRoomOverlayEjectUnlinkedMask`: proof that the overlay pointer was
  removed from both original common and display-link lists, expected `0x3`.
- `gNdsOpeningRoomScene1CameraCreateResult`: original
  `mvOpeningRoomMakeScene1Cameras` creation marker, expected `0x4F523143`
  (`OR1C`).
- `gNdsOpeningRoomScene1CameraCreateMask`: readiness mask for the two original
  camera GObjs, two CObjs, four XObjs, display/process/camanim/viewport, and
  DL-buffer checks, expected `0x1FF`.
- `gNdsOpeningRoomScene1CameraCreateGObjCount`: setup-time object count after
  Scene 1 camera creation, expected `4`.
- `gNdsOpeningRoomScene1CameraGObjDelta` / `CObjDelta` / `XObjDelta` /
  `AObjDelta`: original object-manager deltas from Scene 1 camera setup,
  expected `2` / `2` / `4` / `0`.
- `gNdsOpeningRoomScene1CameraDisplaySet` /
  `ProcessSet` / `AnimSet` / `ViewportSet` / `DLBufferSet`: original parked
  display callbacks, process callbacks, camanim attachments, viewport values,
  and DL-buffer flags, expected `1` for each.
- `gNdsOpeningRoomCloseUpOverlayCameraCreateResult`: original
  `mvOpeningRoomMakeCloseUpOverlayCamera` creation marker, expected
  `0x4F524343` (`ORCC`).
- `gNdsOpeningRoomCloseUpOverlayCameraCreateMask`: readiness mask for original
  close-up overlay camera GObj/CObj/no-XObj/display/viewport checks, expected
  `0x1F`.
- `gNdsOpeningRoomCloseUpOverlayCameraCreateGObjCount`: setup-time object count
  after close-up overlay camera creation, expected `5`.
- `gNdsOpeningRoomCloseUpOverlayCameraGObjDelta` / `CObjDelta` / `XObjDelta`:
  original object-manager deltas from close-up overlay camera setup, expected
  `1` / `1` / `0`.
- `gNdsOpeningRoomCloseUpOverlayCameraDisplaySet` /
  `gNdsOpeningRoomCloseUpOverlayCameraViewportSet`: original parked sprite
  camera display callback and viewport checks, expected `1` / `1`.
- `gNdsOpeningRoomWallpaperCameraCreateResult`: original
  `mvOpeningRoomMakeWallpaperCamera` creation marker, expected `0x4F525743`
  (`ORWC`).
- `gNdsOpeningRoomWallpaperCameraCreateMask`: readiness mask for original
  wallpaper-camera GObj/CObj/no-XObj/display/viewport checks, expected `0x1F`.
- `gNdsOpeningRoomWallpaperCameraCreateGObjCount`: setup-time object count
  after wallpaper-camera creation, expected `6`.
- `gNdsOpeningRoomWallpaperCameraGObjDelta` / `CObjDelta` / `XObjDelta`:
  original object-manager deltas from wallpaper-camera setup, expected
  `1` / `1` / `0`.
- `gNdsOpeningRoomWallpaperCameraDisplaySet` /
  `gNdsOpeningRoomWallpaperCameraViewportSet`: original parked sprite camera
  display callback and viewport checks, expected `1` / `1`.
- `gNdsOpeningRoomLogoCameraAssetMask`: logo-camera camanim readiness mask,
  expected `0x1`.
- `gNdsOpeningRoomLogoCameraAnimOffset`: BattleShip
  `llMVOpeningRoomScene1CamAnimJoint` offset, expected `0`.
- `gNdsOpeningRoomLogoCameraCreateResult`: original
  `mvOpeningRoomMakeLogoCamera` creation marker, expected `0x4F52434D`
  (`ORCM`).
- `gNdsOpeningRoomLogoCameraCreateMask`: readiness mask for the original
  camera GObj/CObj/XObj/display/process/camanim/viewport checks, expected
  `0x7F`.
- `gNdsOpeningRoomLogoCameraCreateGObjCount`: setup-time object count after
  logo-camera creation, expected `7`.
- `gNdsOpeningRoomLogoCameraGObjDelta` / `CObjDelta` / `XObjDelta` /
  `AObjDelta`: original object-manager deltas from logo-camera setup, expected
  `1` / `1` / `2` / `0`.
- `gNdsOpeningRoomLogoCameraDisplaySet` /
  `gNdsOpeningRoomLogoCameraProcessSet` /
  `gNdsOpeningRoomLogoCameraAnimSet` /
  `gNdsOpeningRoomLogoCameraViewportSet`: original parked camera display
  callback, process callback, camanim attachment, and viewport checks, expected
  `1` / `1` / `1` / `1`.
- `gNdsOpeningRoomLogoAssetMask`: logo DObj/MObj/MatAnim readiness mask,
  expected `0x7`.
- `gNdsOpeningRoomLogoDObjOffset`: BattleShip
  `llMVCommonRoomLogoDObjDesc` offset, expected `115880`.
- `gNdsOpeningRoomLogoMObjOffset`: BattleShip `llMVCommonRoomLogoMObjSub`
  offset, expected `113760`.
- `gNdsOpeningRoomLogoMatAnimOffset`: BattleShip
  `llMVCommonRoomLogoMatAnimJoint` offset, expected `116012`.
- `gNdsOpeningRoomLogoCreateResult`: original `mvOpeningRoomMakeLogo` creation
  marker, expected `0x4F524C43` (`ORLC`).
- `gNdsOpeningRoomLogoCreateMask`: readiness mask for the original logo
  GObj/DObj/XObj/MObj/display/material-animation checks, expected `0x3F`.
- `gNdsOpeningRoomLogoCreateGObjCount`: setup-time object count after logo
  creation, expected `9`.
- `gNdsOpeningRoomLogoGObjDelta` / `DObjDelta` / `XObjDelta` / `MObjDelta` /
  `AObjDelta`: original object-manager deltas from logo setup, expected
  `1` / `2` / `4` / `1` / `0`.
- `gNdsOpeningRoomLogoDisplaySet` / `LogoMObjSet` /
  `LogoMatAnimSet`: original parked display callback, material object, and
  material animation attachment checks, expected `1` / `1` / `1`.
- `gNdsOpeningRoomLogoEjectResult`: original logo ejection marker, expected
  `0x4F524C45` (`ORLE`).
- `gNdsOpeningRoomLogoEjectBeforeGObjCount` /
  `gNdsOpeningRoomLogoEjectAfterGObjCount`: original
  `gcGetGObjsActiveNum` samples around ejection, expected `14` / `14`.
- `gNdsOpeningRoomLogoEjectUnlinkedMask`: proof that the logo pointer was
  removed from both original common and display-link lists, expected `0x3`.
- `gNdsOpeningRoomBossShadowAssetMask`: boss-shadow display-list and animation
  readiness mask, expected `0x3`.
- `gNdsOpeningRoomBossShadowDisplayListOffset`: BattleShip
  `llMVCommonRoomBossShadowDisplayList` offset, expected `128912`.
- `gNdsOpeningRoomBossShadowAnimOffset`: BattleShip
  `llMVCommonRoomBossShadowAnimJoint` offset, expected `129316`.
- `gNdsOpeningRoomBossShadowCreateResult`: original
  `mvOpeningRoomMakeBossShadow` creation marker, expected `0x4F524243`
  (`ORBC`).
- `gNdsOpeningRoomBossShadowCreateMask`: readiness mask for the original
  boss-shadow GObj/DObj/XObj/process/display/animation checks, expected
  `0x3F`.
- `gNdsOpeningRoomBossShadowCreateGObjCount`: setup-time object count after
  boss-shadow creation, expected `10`.
- `gNdsOpeningRoomBossShadowGObjDelta` / `DObjDelta` / `XObjDelta` /
  `AObjDelta`: original object-manager deltas from setup, expected
  `1` / `1` / `1` / `0`.
- `gNdsOpeningRoomBossShadowProcessSet` /
  `gNdsOpeningRoomBossShadowDisplaySet` /
  `gNdsOpeningRoomBossShadowAnimSet`: original process, parked display
  callback, and animation attachment checks, expected `1` / `1` / `1`.
- `gNdsOpeningRoomBossShadowEjectResult`: original boss-shadow ejection
  marker, expected `0x4F524245` (`ORBE`).
- `gNdsOpeningRoomBossShadowEjectBeforeGObjCount` /
  `gNdsOpeningRoomBossShadowEjectAfterGObjCount`: original
  `gcGetGObjsActiveNum` samples around ejection, expected `14` / `14`.
- `gNdsOpeningRoomBossShadowEjectUnlinkedMask`: proof that the boss-shadow
  pointer was removed from both original common and display-link lists,
  expected `0x3`.
- `gNdsOpeningRoomPencilsCreateResult`: original
  `mvOpeningRoomMakePencils` creation marker from inside the tick-280 update,
  expected `0x4F525043` (`ORPC`).
- `gNdsOpeningRoomPencilsCreateMask`: readiness mask for the original pencils
  GObj/DObj/XObj/process/display/animation-root checks, expected `0x3F`.
- `gNdsOpeningRoomPencilsGObjDelta`: original object-manager common GObj
  increase from the pencils call, expected `1`.
- `gNdsOpeningRoomPencilsDObjDelta`: DObj increase from
  `gcSetupCommonDObjs`, expected `3`.
- `gNdsOpeningRoomPencilsXObjDelta`: XObj increase from original transform
  setup, expected `6`.
- `gNdsOpeningRoomPencilsAObjDelta`: AObj increase during the first
  `gcPlayAnimAll`, expected `0` for this first `SetValBlock` slice.
- `gNdsOpeningRoomPencilsProcessSet`: original common update process attached,
  expected `1`.
- `gNdsOpeningRoomPencilsDisplaySet`: parked `gcDrawDObjTreeForGObj` display
  callback attached, expected `1`.
- `gNdsOpeningRoomPencilsDObjTreeCount`: DObj tree nodes walked by original
  `gcGetTreeDObjNext`, expected `3`.
- `gNdsOpeningRoomPencilsAnimRootCount`: root animation marker count, expected
  `1`.
- `gNdsOpeningRoomTick380DeferredResult`: tick-380 deferred branch marker,
  expected `0x4F523338` (`OR38`).
- `gNdsOpeningRoomTick380DeferredMask`: deferred fighter-status/rotation mask,
  expected `0x01`.
- `gNdsOpeningRoomTick450RunResult`: tick-450 update marker, expected
  `0x4F523435` (`OR45`).
- `gNdsOpeningRoomTick450DeferredMask`: no remaining tick-450 deferred branch,
  expected `0x00`.
- `gNdsOpeningRoomOutsideAssetMask`: Outside display-list readiness mask,
  expected `0x1`.
- `gNdsOpeningRoomOutsideDisplayListOffset`: BattleShip
  `llMVCommonRoomOutsideDisplayList` offset, expected `147968`.
- `gNdsOpeningRoomOutsideCreateResult`: wrapper-created Outside object marker,
  expected `0x4F524F55` (`OROU`).
- `gNdsOpeningRoomOutsideCreateMask`: readiness mask for the Outside
  GObj/DObj/XObj/display-link checks, expected `0x0F`.
- `gNdsOpeningRoomOutsideCreateGObjCount`: setup-time object count after
  Outside creation, expected `11`.
- `gNdsOpeningRoomOutsideGObjDelta` / `DObjDelta` / `XObjDelta`: object-manager
  deltas from Outside setup, expected `1` / `1` / `1`.
- `gNdsOpeningRoomOutsideDisplaySet`: original display callback/link readiness
  for display link 6, expected `1`.
- `gNdsOpeningRoomHazeAssetMask`: Haze display-list readiness mask, expected
  `0x1`.
- `gNdsOpeningRoomHazeDisplayListOffset`: BattleShip
  `llMVCommonRoomHazeDisplayList` offset, expected `39160`.
- `gNdsOpeningRoomHazeCreateResult`: wrapper-created Haze object marker,
  expected `0x4F52485A` (`ORHZ`).
- `gNdsOpeningRoomHazeCreateMask`: readiness mask for the Haze
  GObj/DObj/XObj/display-link checks, expected `0x0F`.
- `gNdsOpeningRoomHazeCreateGObjCount`: setup-time object count after Haze
  creation, expected `12`.
- `gNdsOpeningRoomHazeGObjDelta` / `DObjDelta` / `XObjDelta`: object-manager
  deltas from Haze setup, expected `1` / `1` / `1`.
- `gNdsOpeningRoomHazeDisplaySet`: original display callback/link readiness
  for display link 6, expected `1`.
- `gNdsOpeningRoomSunlightAssetMask`: sunlight display-list readiness mask,
  expected `0x1`.
- `gNdsOpeningRoomSunlightDisplayListOffset`: BattleShip
  `llMVCommonRoomSunlightDisplayList` offset, expected `149256`.
- `gNdsOpeningRoomSunlightCreateResult`: wrapper-created sunlight object marker,
  expected `0x4F525343` (`ORSC`).
- `gNdsOpeningRoomSunlightCreateMask`: readiness mask for the sunlight
  GObj/DObj/XObj/display-link checks, expected `0x0F`.
- `gNdsOpeningRoomSunlightCreateGObjCount`: setup-time object count after
  sunlight creation, expected `13`.
- `gNdsOpeningRoomSunlightGObjDelta` / `DObjDelta` / `XObjDelta`: object-manager
  deltas from sunlight setup, expected `1` / `1` / `1`.
- `gNdsOpeningRoomSunlightDisplaySet`: original display callback/link readiness
  for display link 6, expected `1`.
- `gNdsOpeningRoomSunlightEjectResult`: sunlight ejection marker, expected
  `0x4F525345` (`ORSE`).
- `gNdsOpeningRoomSunlightEjectBeforeGObjCount` /
  `gNdsOpeningRoomSunlightEjectAfterGObjCount`: original
  `gcGetGObjsActiveNum` samples around ejection, expected `14` / `14`.
- `gNdsOpeningRoomSunlightEjectUnlinkedMask`: proof that the sunlight pointer
  was removed from both original common and display-link lists, expected `0x3`.
- `gNdsOpeningRoomCloseUpOverlayCreateResult`: original
  `mvOpeningRoomMakeCloseUpOverlay` creation marker from inside the tick-450
  update, expected `0x4F52434F` (`ORCO`).
- `gNdsOpeningRoomCloseUpOverlayCreateMask`: readiness mask for the close-up
  overlay GObj/display-link/alpha checks, expected `0x07`.
- `gNdsOpeningRoomCloseUpOverlayCreateTick`: original actor tick at creation,
  expected `450`.
- `gNdsOpeningRoomCloseUpOverlayCreateGObjCount`: original
  `gcGetGObjsActiveNum` sample after close-up overlay creation, expected `14`.
- `gNdsOpeningRoomCloseUpOverlayGObjDelta`: original object-manager common GObj
  increase from the close-up overlay call, expected `1`.
- `gNdsOpeningRoomCloseUpOverlayDisplaySet`: original display callback/link
  readiness for display link 26, expected `1`.
- `gNdsOpeningRoomCloseUpOverlayAlphaInit`: original close-up overlay alpha
  initialization, expected `0`.
- `gNdsOpeningRoomTick500RunResult`: tick-500 update marker, expected
  `0x4F523530` (`OR50`).
- `gNdsOpeningRoomTick500DeferredMask`: deferred pulled-fighter display-link
  movement mask, expected `0x01`.
- `gNdsOpeningRoomSpotlightAssetMask`: spotlight display-list/MObj/material
  animation asset readiness mask, expected `0x07`.
- `gNdsOpeningRoomSpotlightDisplayListOffset`: BattleShip
  `llMVCommonRoomSpotlightDisplayList` offset, expected `142872`.
- `gNdsOpeningRoomSpotlightMObjOffset`: BattleShip
  `llMVCommonRoomSpotlightMObjSub` offset, expected `142480`.
- `gNdsOpeningRoomSpotlightMatAnimOffset`: BattleShip
  `llMVCommonRoomSpotlightMatAnimJoint` offset, expected `143120`.
- `gNdsOpeningRoomSpotlightCreateResult`: original
  `mvOpeningRoomMakeSpotlight` creation marker from tick 500, expected
  `0x4F52534C` (`ORSL`).
- `gNdsOpeningRoomSpotlightCreateMask`: readiness mask for spotlight
  GObj/DObj/XObj/MObj/display/process/material-animation/position checks,
  expected `0xFF`.
- `gNdsOpeningRoomSpotlightCreateTick`: original actor tick at creation,
  expected `500`.
- `gNdsOpeningRoomSpotlightCreateGObjCount`: original
  `gcGetGObjsActiveNum` sample after spotlight creation, expected `14`.
- `gNdsOpeningRoomSpotlightGObjDelta` / `DObjDelta` / `XObjDelta` /
  `MObjDelta` / `AObjDelta`: object-manager deltas for spotlight creation,
  expected `1`, `1`, `1`, `2`, and `0`.
- `gNdsOpeningRoomSpotlightDisplaySet`, `ProcessSet`, `MObjSet`,
  `MatAnimSet`, `PositionSet`: spotlight object link checks, expected `1`.
- `gNdsOpeningRoomTick560RunResult`: tick-560 update marker, expected
  `0x4F523536` (`OR56`).
- `gNdsOpeningRoomTick560DeferredMask`: deferred Boss fighter status mask,
  expected `0x01`.
- `gNdsOpeningRoomScene2CameraAssetMask`: Scene 2 camanim readiness mask,
  expected `0x01`.
- `gNdsOpeningRoomScene2CameraAnimOffset`: BattleShip
  `llMVOpeningRoomScene2CamAnimJoint` offset, expected `0`.
- `gNdsOpeningRoomScene2CameraEjectResult` / `EjectMask`: original Scene 1
  camera ejection marker and readiness mask, expected `0x4F523245` / `0x07`.
- `gNdsOpeningRoomScene2CameraCreateResult` / `CreateMask`: original Scene 2
  camera creation marker and readiness mask, expected `0x4F523243` / `0x1FF`.
- `gNdsOpeningRoomScene2CameraGObjDelta` / `CObjDelta` / `XObjDelta` /
  `AObjDelta`: object-manager deltas after Scene 1 camera ejection, expected
  `2`, `2`, `4`, and `0`.
- `gNdsOpeningRoomScene2CameraDisplaySet`, `ProcessSet`, `AnimSet`,
  `ViewportSet`, `DLBufferSet`: Scene 2 camera link checks, expected `1`.
- `gNdsOpeningRoomUpdateResult`: first imported actor update, expected
  `0x4F525550` (`ORUP`).
- `gNdsOpeningRoomTickCount`: actor time at the normal no-input boundary,
  expected `560`.
- `gNdsOpeningRoomPreAssetResult`: tick-279 pre-event marker, expected
  `0x4F525041` (`ORPA`).
- `gNdsOpeningRoomControllerCheckCount`: original shared controller gate calls
  on ticks 10-560, expected `551`.
- `gNdsOpeningRoomPulledFighterKind` / `DroppedFighterKind`: distinct members
  of the original `{Mario,Fox,Donkey,Samus,Link,Yoshi,Kirby,Pikachu}` set.
- `gNdsOpeningRoomSkipToTitleCount`: `0` in normal verification and `1` in the
  skip-path verifier.
- `gNdsOpeningRoomGObjCount` / `gNdsOpeningRoomCameraCount`: post-`func_start`
  original object-manager counts, expected `13` / `6`.
- Opening Room task setup diagnostics preserve DL sizes `12000` / `4096`,
  graphics heap `32768`, and RDP output buffer `49152`.
- `gNdsOpeningRoomDrawResult`: bounded Opening Room draw marker, expected
  `0x4F524457` (`ORDW`).
- `gNdsOpeningRoomDrawBlocker`: current full-renderer blocker reached by the
  bounded Opening Room draw path, expected `3` for general DObj display-list
  translation. The separate preview path is tracked by
  `gNdsOpeningRoomDLPreviewBlocker`.
- `gNdsOpeningRoomDrawTickCount`: actor tick when the latest actual draw probe
  ran, expected `1320` for the final Opening Room handoff draw.
- `gNdsOpeningRoomDrawFrameCount`: taskman frame counter when the draw probe
  ran, expected `1`; this proves the probe is still bounded and not the
  continuous draw loop.
- `gNdsOpeningRoomDrawProbeCount`: number of actual original `scene_draw`
  probes run for the retained Opening Room preview, expected `2`.
- `gNdsOpeningRoomDrawReuseCount`: number of repeated movie presentations that
  reused the retained Opening Room preview instead of rerunning `gcDrawAll`;
  the 45-second speed sample currently reports `24`.
- `gNdsOpeningRoomDrawCameraCallbackCount`: original `func_80017EC0` camera
  display callbacks reached by the probes, expected greater than `0`.
- `gNdsOpeningRoomDrawDisplayCallbackCount`: camera-captured display callbacks
  reached by the probes, expected greater than `0`.
- `gNdsOpeningRoomDrawDObjCallbackCount`: DObj display callbacks reached by the
  probes, expected greater than `0`.
- `gNdsOpeningRoomDrawFirstCameraMaskLow` / `FirstCameraPriority`: first camera
  evidence, currently `0x40` / `80`.
- `gNdsOpeningRoomDrawFirstCameraFlags`: active first draw camera CObj flags,
  currently `0x4` (`COBJ_FLAG_DLBUFFERS`) for the Scene 2 main camera.
- `gNdsOpeningRoomDrawFirstCameraXObjCount` / `XObjKind0` / `XObjKind1`:
  active first draw camera matrix evidence, currently `2,3,8`
  (`nGCMatrixKindPerspFastF` plus the Scene 2 camera-vector kind).
- `gNdsOpeningRoomDrawFirstCameraViewportScaleX` /
  `ViewportScaleY` / `ViewportTransX` / `ViewportTransY`: original viewport
  values sampled from the active draw CObj, expected `600,440,640,480`.
- `gNdsRdpDefaultViewportSetCount`: number of original default viewport
  initializations performed by the DS RDP shim, expected greater than `0`.
- `gNdsRdpDefaultViewportScaleX` / `ScaleY` / `TransX` / `TransY` /
  `ScaleZ` / `TransZ`: last default viewport values written by
  `syRdpSetDefaultViewport`, expected `640,480,640,480,511,511` for the
  current 320x240 BattleShip video setup.
- `gNdsOpeningRoomDrawFirstCameraNear100` / `Far100` / `FovY100`: active
  perspective values in centi-units. The verifier requires positive near/fovy
  and `far > near` because the bounded DObj preview now draws through this
  active original camera state.
- `gNdsOpeningRoomDrawFirstCameraEyeX100` / `EyeY100` / `EyeZ100` and
  `AtX100` / `AtY100` / `AtZ100`: active CObj eye/at vectors in centi-units.
  These are sampled as the source for the bounded camera-projected preview.
- `gNdsOpeningRoomDrawFirstObjectDLLink` / `FirstObjectID` /
  `FirstObjectKind`: first captured display object evidence, currently display
  link `6`, object ID `0`, object kind `1` (`nGCCommonKindDObj`).
- `gNdsOpeningRoomDrawFirstCallback`: first DObj display callback marker,
  currently `0x444C4E4B` (`gcDrawDObjDLLinksForGObj`).
- `gNdsOpeningRoomDrawFirstDObjDL`: first DObj display-list pointer, expected
  nonzero.
- `gNdsOpeningRoomDrawFirstDObjMeta`: DObj readiness bits, currently `0x11`
  for display-list pointer plus XObj.
- `gNdsOpeningRoomDrawMaterialCandidateResult`: marker proving the bounded
  imported draw traversal found a material-bearing DObj candidate, expected
  `0x4F524D43` (`ORMC`) once melonDS/GDB verification is available.
- `gNdsOpeningRoomDrawMaterialCandidateCount`: number of material-bearing DObj
  candidates observed by the draw probe. The verifier requires at least `1`.
- `gNdsOpeningRoomDrawMaterialCandidateCameraMaskLow` /
  `CameraPriority`: camera evidence for the first material-bearing candidate.
  The verifier checks for a nonzero mask and a real priority.
- `gNdsOpeningRoomDrawMaterialCandidateObjectDLLink` /
  `ObjectID` / `ObjectKind`: display object evidence for the first
  material-bearing candidate. Object kind must be `1` (`DObj`).
- `gNdsOpeningRoomDrawMaterialCandidateCallback`: original display callback
  marker for the material-bearing candidate. Valid values are the existing DObj
  draw callbacks; this is callback-aware so `dobj->dl` and `dobj->dl_link`
  paths are not conflated.
- `gNdsOpeningRoomDrawMaterialCandidateDObjDL` /
  `DObjMeta`: selected material-bearing candidate display-list pointer and
  DObj readiness bits. The verifier checks for a nonzero DL pointer and meta
  bits proving both display-list and MObj are present.
- `gNdsOpeningRoomDrawMaterialCandidateMObjCount`: first material-bearing
  candidate MObj chain count. The verifier requires at least `1`.
- `gNdsOpeningRoomDrawMaterialCandidateMObjFlags` /
  `MObjEffectiveFlags` / `MObjMask`: raw first-MObj flags, original fallback
  flags after the `gcDrawMObjForDObj` material contract, and a presence mask.
  The current verifier requires raw flags `0x200` for the corrected first
  MVCommon material candidate, DObj/MObj presence, and non-sentinel effective
  flags; individual texture/tile/load-block branch checks are conditional on
  the recorded material mask.
- `gNdsOpeningRoomDrawMaterialCandidateMObjTextureCurr` /
  `TextureNext` / `PaletteIndex` / `Lfrac100`: current/next texture IDs,
  palette index, and interpolation fraction for the first candidate MObj.
- `gNdsOpeningRoomDrawMaterialCandidateMObjFormat` / `Size` /
  `BlockFormat` / `BlockSize`: texture format metadata resolved from the first
  candidate MObj. The verifier checks that the primary format/size are not the
  reset sentinel.
- `gNdsOpeningRoomDrawMaterialCandidateMObjTileWidth` / `TileHeight` /
  `ScrollWidth` / `ScrollHeight`: texture tile and scroll dimensions recorded
  before any renderer branch is emitted.
- `gNdsOpeningRoomDrawMaterialCandidateMObjScaleS100` / `ScaleT100` /
  `TranslateS100` / `TranslateT100`: S/T scale and translation sampled from the
  first candidate MObj in centi-units.
- `gNdsOpeningRoomDrawMaterialCandidateMObjSpriteArray` /
  `PaletteArray`: raw first-MObj sprite/palette array pointers. These remain
  useful source-side diagnostics, but the corrected current first material
  candidate is prim-color only and does not emit a material `SETTIMG` command.
- `gNdsOpeningRoomDrawMaterialCandidateMObjSpriteCurr` / `SpriteNext` /
  `PalettePtr`: safe pointer evidence for the first candidate MObj's sprite and
  palette records. These are diagnostics only; they do not mean texture upload
  or material branch rendering exists.
- `gNdsOpeningRoomDrawTextureMaterialResult`: marker for the first
  texture-capable material source found by the bounded imported draw traversal.
  It remains `0` for the current verified slice; the pass marker is
  `0x4F525458` (`ORTX`) when an effective `MOBJ_FLAG_TEXTURE` material is found.
- `gNdsOpeningRoomDrawTextureMaterialCandidateCount` /
  `MObjCount` / `SpriteArrayCount` / `SpriteCurrCount` / `SpriteNextCount`:
  source-side texture-material scan counts. Current expected values are all
  `0`, proving the selected bounded material path exposes no texture-bearing
  `MObj` source yet.
- `gNdsOpeningRoomDrawTextureMaterialObjectDLLink` / `ObjectID` /
  `ObjectKind` / `Callback`, `DObjDL` / `DObjMeta`, `MObjFlags` /
  `MObjEffectiveFlags` / `MObjMask`, `MObjTextureCurr` / `MObjTextureNext`, and
  `MObjSpriteArray` / `MObjSpriteCurr` / `MObjSpriteNext`: captured evidence for
  the first future `ORTX` candidate. Current reset/sentinel values are expected
  while `ORTX` result is `0`.
- `gNdsOpeningRoomDrawMaterialBranchResult`: marker proving the DS backend
  mirrored the original `gcDrawMObjForDObj` branch-list command-family
  decisions for the first material-bearing candidate, expected `0x4F524D42`
  (`ORMB`).
- `gNdsOpeningRoomDrawMaterialBranchMObjCount`: MObj count used for the original
  branch table. The verifier requires it to match the candidate MObj count;
  current value is `2`.
- `gNdsOpeningRoomDrawMaterialBranchSegmentCommands`: original
  `gSPSegment(..., 0xE, gSYTaskmanGraphicsHeap.ptr)` command count. Expected
  `1`.
- `gNdsOpeningRoomDrawMaterialBranchTableCommands`: original
  `gSPBranchList(&new_dl[i], branch_dl)` table command count. Expected to match
  the MObj count; current value is `2`.
- `gNdsOpeningRoomDrawMaterialBranchGeneratedCommands`: total commands that the
  branch stream would generate after the table, excluding actual `Gfx`
  allocation. Current value is `4`.
- `gNdsOpeningRoomDrawMaterialBranchFirstMask`: command-family mask for the
  first MObj branch. The verifier requires segment/table/end evidence and
  conditionally checks texture/tile/scroll/load-block families when the material
  mask says they are present. Current value is `0x4023`, proving the corrected
  prim-color-only source material shape.
- `gNdsOpeningRoomDrawMaterialBranchFirstGeneratedCommands`: first-MObj branch
  stream command count. Current value is `2`.
- `gNdsOpeningRoomDrawMaterialBranchFirstTextureScaleS` /
  `FirstTextureScaleT`: original `gSPTexture` scale values when the first MObj
  emits a texture command. Current candidate first-MObj does not emit texture,
  so both are `0`.
- `gNdsOpeningRoomDrawMaterialBranchFirstTileUls` / `FirstTileUlt` /
  `FirstTileLrs` / `FirstTileLrt`: original `gDPSetTileSize` values when the
  first MObj emits tile-size setup. Current values are all `0`.
- `gNdsOpeningRoomDrawMaterialBranchFirstScrollUls` / `FirstScrollUlt` /
  `FirstScrollLrs` / `FirstScrollLrt`: original scroll tile-size values when
  emitted. Current values are all `0`.
- `gNdsOpeningRoomDrawMaterialBranchFirstLoadBlockTexels` /
  `FirstLoadBlockDxt`: original `gDPLoadBlock` texel/DXT values when the first
  MObj emits a load-block path. Current values are `0,0`.
- `gNdsOpeningRoomDrawMaterialEmitResult`: marker proving the DS backend
  emitted a detached original-shaped material branch table/stream into
  `gSYTaskmanGraphicsHeap`, expected `0x4F524D45` (`ORME`).
- `gNdsOpeningRoomDrawMaterialEmitBlocker`: blocker enum for the detached
  material emission path. Expected `0`.
- `gNdsOpeningRoomDrawMaterialEmitUnsupportedMask`: command-family mask that
  still prevents detached emission. Expected `0` for the current candidate.
- `gNdsOpeningRoomDrawMaterialEmitMObjCount` /
  `TableCommands` / `GeneratedCommands`: emitted MObj/table/branch-stream
  counts. Current values are `2`, `2`, and `4`.
- `gNdsOpeningRoomDrawMaterialEmitHeapStart` /
  `BranchStart` / `HeapAfter` / `HeapBytes`: graphics heap allocation evidence.
  Current byte count is `48`.
- `gNdsOpeningRoomDrawMaterialEmitFirstTableOp`: first table opcode, expected
  `0xDE` (`G_DL`/branch-list).
- `gNdsOpeningRoomDrawMaterialEmitFirstBranchOp0` /
  `FirstBranchOp1` / `FirstBranchOp2`: first emitted branch opcodes, expected
  `0xFA`, `0xDF`, `0x00`.
- `gNdsOpeningRoomDrawMaterialEmitFirstBranchW0_*` /
  `FirstBranchW1_*`: raw first-branch command words. The current first command
  is `G_SETPRIMCOLOR`, followed by `G_ENDDL`; the visible bounded preview now
  consumes this emitted branch state for the selected material candidate.
- `gNdsOpeningRoomDLPreviewMaterialBranchResult`: marker proving the bounded
  DL preview consumed the detached material branch table and both generated
  branch streams as preview state, expected `0x4F524D50` (`ORMP`).
- `gNdsOpeningRoomDLPreviewMaterialBranchBlocker`: preview-state blocker enum,
  expected `0`.
- `gNdsOpeningRoomDLPreviewMaterialBranchCommandCount` /
  `PrimCount` / `EndCount`: parsed detached branch-table stream counts,
  expected `4`, `2`, and `2`.
- `gNdsOpeningRoomDLPreviewMaterialBranchFirstOp` /
  `SecondOp` / `UnsupportedOp`: first parsed material branch opcodes, expected
  `0xFA`, `0xDF`, and `0`.
- `gNdsOpeningRoomDLPreviewMaterialBranchPrimColor` /
  `PrimLod` / `PrimM`: first parsed `G_SETPRIMCOLOR` state. The verifier
  requires the prim color to match the detached `ORME` branch and the current
  `lod/m` fields to be `0,0`.
- `gNdsOpeningRoomMaterialDLProbeResult`: marker for the material-bearing
  candidate's own display-list shape probe. It remains `0` until the candidate
  list can be consumed without unsupported commands; the pass marker is
  `0x4F524D44` (`ORMD`).
- `gNdsOpeningRoomMaterialDLProbeBlocker`: blocker enum for that non-visual
  material-candidate DL probe. Current expected value is `3`
  (`UNSUPPORTED`), proving the probe reached a real list but stopped at a
  command the bounded renderer does not execute yet.
- `gNdsOpeningRoomMaterialDLProbeFirstDL`: nonzero original display-list
  pointer selected from the first material-bearing DObj candidate.
- `gNdsOpeningRoomMaterialDLProbeCommandCount` /
  `VertexCount` / `TriangleCount`: current list shape is 29 commands, four
  vertices, and four triangles.
- `gNdsOpeningRoomMaterialDLProbeFirstOpcode` /
  `UnsupportedOpcode`: current first opcode is `0xE7`; first unsupported opcode
  is `0xDE` (`G_DL`), making nested display-list branch handling the next
  renderer blocker for this material candidate.
- `gNdsOpeningRoomMaterialDLProbeVertexCommandCount` /
  `TriangleCommandCount` / `SyncCommandCount` / `EndCommandCount` /
  `BranchCommandCount` / `OtherModeCommandCount` /
  `UnsupportedCommandCount`: current command-family counts are `2`, `2`, `7`,
  `1`, `2`, `0`, and `2`.
- `gNdsOpeningRoomMaterialDLExpandResult`: marker proving the bounded
  branch-expanded material-candidate display-list probe passed through
  `src/nds/nds_renderer.c`, expected `0x4F524D58` (`ORMX`).
- `gNdsOpeningRoomMaterialDLExpandBlocker`: blocker enum for the expanded
  probe, expected `0`.
- `gNdsOpeningRoomMaterialDLExpandFirstDL`: same first material-candidate
  display-list pointer sampled by `ORMD`, expected nonzero.
- `gNdsOpeningRoomMaterialDLExpandFirstBranchDL`: first raw branch target seen
  while expanding the material-candidate list. Current expected value is
  `0x0E000000`, matching the original segment-`E` material branch table
  contract from `gcDrawMObjForDObj`.
- `gNdsOpeningRoomMaterialDLExpandFirstResolvedBranchDL`: native pointer after
  resolving the first segment-`E` branch through the emitted `ORME`
  `gSYTaskmanGraphicsHeap` table, expected nonzero.
- `gNdsOpeningRoomMaterialDLExpandCommandCount` /
  `VertexCount` / `TriangleCount` / `UnsupportedCommandCount`: expanded shape,
  expected `42`, `4`, `4`, and `0`.
- `gNdsOpeningRoomMaterialDLExpandFirstOpcode` /
  `UnsupportedOpcode`: expanded first opcode and first unsupported opcode,
  expected `0xE7` and `0`.
- `gNdsOpeningRoomMaterialDLExpandVertexCommandCount` /
  `TriangleCommandCount` / `SyncCommandCount` / `EndCommandCount` /
  `BranchCommandCount` / `BranchCallCount` / `BranchJumpCount` /
  `SegmentResolveCount` / `OtherModeCommandCount`: current command-family
  counts are `2`, `2`, `7`, `6`, `5`, `5`, `0`, `2`, and `0`.
- `gNdsOpeningRoomMaterialDLExpandColorCommandCount` /
  `MaxDepth`: current values are `5` color-state commands and branch depth `2`.
- `gNdsOpeningRoomDLPreviewResult`: bounded Opening Room DObj display-list
  preview marker, expected `0x4F524450` (`ORDP`). The visible preview now
  receives commands from `ndsRendererExecuteDisplayList`; the scene bridge
  still owns the Smash-specific vertex/texture/camera decode.
- `gNdsOpeningRoomDLPreviewBlocker`: preview blocker enum, expected `0`.
- `gNdsOpeningRoomDLPreviewCommandCount`: parsed first-DL command count,
  expected `42` for the branch-expanded material candidate.
- `gNdsOpeningRoomDLPreviewVertexCount`: decoded vertex count, expected at
  `4`.
- `gNdsOpeningRoomDLPreviewTriangleCount`: rasterized triangle count, expected
  `4`.
- `gNdsOpeningRoomDLPreviewPixelCount`: written preview pixels, expected
  greater than `0`; exact count can change as the bounded diagnostic
  presentation is tightened.
- `gNdsOpeningRoomDLPreviewFirstOpcode`: first command opcode, expected
  `0xE7`.
- `gNdsOpeningRoomDLPreviewUnsupportedOpcode`: unsupported command seen by the
  preview interpreter, expected `0`.
- `gNdsOpeningRoomDLPreviewVertexCommandCount`: number of `G_VTX` commands in
  the parsed branch-expanded list, expected `2`.
- `gNdsOpeningRoomDLPreviewTriangleCommandCount`: number of `G_TRI1`/`G_TRI2`
  commands in the parsed branch-expanded list, expected `2`.
- `gNdsOpeningRoomDLPreviewSyncCommandCount`: number of RDP sync commands in
  the parsed branch-expanded list, expected `7`.
- `gNdsOpeningRoomDLPreviewEndCommandCount`: number of `G_ENDDL` commands in
  the parsed branch-expanded list, expected `6`.
- `gNdsOpeningRoomDLPreviewBranchCommandCount`: number of `G_DL` or
  `G_CULLDL` commands seen by the bounded preview, expected `5`.
- `gNdsOpeningRoomDLPreviewBranchCallCount` /
  `BranchJumpCount` / `SegmentResolveCount`: branch execution shape for the
  selected material candidate, expected `5`, `0`, and `2`.
- `gNdsOpeningRoomDLPreviewColorCommandCount` /
  `PrimColor`: material color-state evidence from the branch-expanded stream,
  expected `5` and `0xFFFFFFFF`.
- `gNdsOpeningRoomDLPreviewOtherModeCommandCount`: number of `G_SETOTHERMODE`
  or raw RDP set-othermode commands seen by the bounded preview, expected `0`.
- `gNdsOpeningRoomDLPreviewUnsupportedCommandCount`: total unsupported command
  count seen by the bounded preview, expected `0`.
- `gNdsOpeningRoomDLPreviewRendererTextureMask` /
  `TextureImage` / `TextureFormat` / `TextureSize` /
  `TextureImageWidth`: the renderer adapter's own `G_SETCOMBINE`,
  `G_SETTIMG`, `G_SETTILE`, `G_SETTILESIZE`, `G_TEXTURE`, and `G_LOADBLOCK`
  state decode for the visible `ORDP` path. Current expected values are mask
  `0x3F`, nonzero image pointer, format `4`, size `2`, and image width field
  `1`.
- `gNdsOpeningRoomDLPreviewRendererTextureLoadTexels` /
  `TextureSetTileCount` / `TextureCommandCount`: adapter-owned load/tile/
  texture-command counts, currently `512`, `4`, and `1`.
- `gNdsOpeningRoomDLPreviewRendererTextureStateFlags`: adapter-owned
  `G_TEXTURE` state flags, currently `0x0F`.
- `gNdsOpeningRoomDLPreviewRendererTextureTileWidth` /
  `TextureTileHeight` / `TextureRenderTile` / `TextureRenderTileLine` /
  `TextureRenderTileFlags`: adapter-owned tile decode for the bounded
  preview, currently `32`, `32`, render tile `0`, line `4`, and flags
  `0xB7`.
- `gNdsOpeningRoomDLPreviewRendererTextureLoadBlockLrs` /
  `TextureLoadBlockDxt`: adapter-owned first load-block fields, currently
  `511` and `512`.
- `gNdsOpeningRoomDLPreviewFirstDL`: first parsed `DObjDLLink->dl` pointer,
  expected nonzero.
- `gNdsOpeningRoomDLPreviewTransformMask`: sampled original DObj/XObj
  transform evidence, expected `0x1F` for DObj, translate, rotate, scale, and
  XObj.
- `gNdsOpeningRoomDLPreviewXObjCount`: first previewed DObj XObj count,
  expected at least `1`.
- `gNdsOpeningRoomDLPreviewFirstXObjKind`: first previewed XObj kind, expected
  `28` (`nGCMatrixKindTraRotRpyRSca`).
- `gNdsOpeningRoomDLPreviewTranslateX100` / `TranslateY100` /
  `TranslateZ100`: sampled DObj translation in centi-units.
- `gNdsOpeningRoomDLPreviewRotateX100` / `RotateY100` / `RotateZ100`: sampled
  DObj RPY rotation in centi-radians, currently `0,0,0` for the first previewed
  object.
- `gNdsOpeningRoomDLPreviewScaleX100` / `ScaleY100` / `ScaleZ100`: sampled
  DObj scale in centi-units, expected `117,100,117` for the selected
  material candidate.
- `gNdsOpeningRoomDLPreviewMinX` / `MaxX` / `MinY` / `MaxY`: transformed
  preview bounds; the verifier requires nonzero width and height.
- `gNdsOpeningRoomDLPreviewProjectionMask`: bounded camera-projection evidence,
  expected `0x1F` for active camera, viewport, perspective, look-at, and
  projected vertices. The current selected material primitive is too small for
  the earlier camera-projected triangle path and uses the bounded retained
  preview fallback for visible pixels.
- `gNdsOpeningRoomDLPreviewProjectionMode`: projection draw mode, expected `2`
  for the current material-candidate preview.
- `gNdsOpeningRoomDLPreviewProjectionBlocker`: projection blocker enum,
  expected `6`.
- `gNdsOpeningRoomDLPreviewProjectedVertexCount` /
  `ProjectedTriangleCount`: camera-projected slice evidence, currently `4/0`.
- `gNdsOpeningRoomDLPreviewProjectedMinX` / `ProjectedMaxX` /
  `ProjectedMinY` / `ProjectedMaxY`: projected preview-space bounds,
  currently `3,31,-23,-14`.
- `gNdsOpeningRoomDLPreviewProjectedMinDepth100` /
  `ProjectedMaxDepth100`: projected camera depth range in centi-units,
  currently `470900..565500`.
- `gNdsOpeningRoomDLPreviewGeometryCommandCount`: number of
  `G_GEOMETRYMODE` commands consumed from the first bounded list, currently
  `2`.
- `gNdsOpeningRoomDLPreviewGeometryClearMask` /
  `GeometrySetMask`: last raw geometry-mode command masks, currently
  `0xD9FFFFFF` and `0x20000`.
- `gNdsOpeningRoomDLPreviewGeometryFinalMode`: final N64 geometry-mode state
  for the selected bounded list, currently `0x20000`.
- `gNdsOpeningRoomDLPreviewGeometryFlags`: DS diagnostic decode of the final
  geometry-mode state, currently `0x21`.
- `gNdsOpeningRoomDLPreviewGeometryPositiveWinding` /
  `GeometryNegativeWinding` / `GeometryZeroArea` /
  `GeometryDrawnTriangles`: projected triangle winding evidence for the
  drawn slice, currently `0/0/4/4`.
- `gNdsOpeningRoomDLPreviewTextureMask`: first preview DL texture setup
  evidence, expected `0x3F` for set-combine, tile, texture enable, tile-size,
  texture-image, and load-block commands.
- `gNdsOpeningRoomDLPreviewTextureImage`: first `G_SETTIMG` image pointer,
  expected nonzero.
- `gNdsOpeningRoomDLPreviewTextureFormat` / `TextureSize`: first texture image
  format/size, currently `4`/`2` for this material-candidate command state.
- `gNdsOpeningRoomDLPreviewTextureImageWidth`: first texture-image width field,
  currently `1` because this list uses a load-block image setup.
- `gNdsOpeningRoomDLPreviewTextureTileWidth` /
  `TextureTileHeight`: decoded first render tile size, expected `16x32`.
- `gNdsOpeningRoomDLPreviewTextureLoadTexels`: decoded first load-block texel
  count, expected `256`.
- `gNdsOpeningRoomDLPreviewTextureLoadBlockTile` /
  `TextureLoadBlockUls` / `TextureLoadBlockUlt` /
  `TextureLoadBlockLrs` / `TextureLoadBlockDxt`: raw first `G_LOADBLOCK`
  command fields, currently `7,0,0,255,1024`.
- `gNdsOpeningRoomDLPreviewTextureTileSizeTile` /
  `TextureTileSizeUls` / `TextureTileSizeUlt` /
  `TextureTileSizeLrs` / `TextureTileSizeLrt`: raw first
  `G_SETTILESIZE` command fields, currently `0,0,0,60,124`.
- `gNdsOpeningRoomDLPreviewTextureTexelWidth` /
  `TextureTexelHeight`: derived physical texture layout for this bounded
  material-candidate preview, currently `0x0` because no resolved material
  texture source is uploaded yet.
- `gNdsOpeningRoomDLPreviewTextureSamplePixels`: number of preview pixel writes
  that sampled the bounded original texture, expected `0` for this
  prim-color-only material candidate.
- `gNdsOpeningRoomDLPreviewTextureSetTileCount`: number of `G_SETTILE`
  commands in the first parsed list, expected `4`.
- `gNdsOpeningRoomDLPreviewTextureCombineW0` /
  `TextureCombineW1`: first set-combine command words, expected
  `0xFC6F96DF,0xFF2E7F3F`.
- `gNdsOpeningRoomDLPreviewTextureCombineMode`: decoded bounded combiner mode,
  expected `0` for the current material-candidate command state.
- `gNdsOpeningRoomDLPreviewTextureCombineFlags`: decoded bounded combiner
  evidence, expected `0x1` for set-combine seen.
- `gNdsOpeningRoomDLPreviewTextureModulatedPixels`: number of preview pixel
  writes that applied decoded shade modulation to sampled texture texels,
  expected `0` for this material-candidate preview.
- `gNdsOpeningRoomDLPreviewTextureRenderTile`: first render tile selected by
  `G_SETTILE`, currently `0`.
- `gNdsOpeningRoomDLPreviewTextureRenderTileLine` /
  `TextureRenderTileTmem` / `TextureRenderTilePalette`: decoded render-tile
  line/TMEM/palette fields, currently `4/0/0`.
- `gNdsOpeningRoomDLPreviewTextureRenderTileCms` /
  `TextureRenderTileCmt`: decoded S/T address mode bits, currently `2/2`.
- `gNdsOpeningRoomDLPreviewTextureRenderTileMasks` /
  `TextureRenderTileMaskt`: decoded S/T mask widths, currently `5/5`.
- `gNdsOpeningRoomDLPreviewTextureRenderTileShifts` /
  `TextureRenderTileShiftt`: decoded S/T shifts, currently `0/0`.
- `gNdsOpeningRoomDLPreviewTextureRenderTileFlags`: DS diagnostic decode of
  the render/load tile state, currently `0xB7` for render seen, load seen,
  S/T clamped, and S/T masked.
- `gNdsOpeningRoomDLPreviewTextureCommandCount`: number of `G_TEXTURE`
  commands consumed from the first parsed list, currently `1`.
- `gNdsOpeningRoomDLPreviewTextureScaleS` /
  `TextureScaleT`: decoded `G_TEXTURE` S/T scale values, currently
  `65535/65535`.
- `gNdsOpeningRoomDLPreviewTextureLevel` /
  `TextureTile` / `TextureOn` / `TextureXParam`: decoded `G_TEXTURE` level,
  tile, on/off, and xparam fields, currently `0/0/1/0`.
- `gNdsOpeningRoomDLPreviewTextureStateFlags`: DS diagnostic decode of
  `G_TEXTURE` state, currently `0x0F` for seen, on, nonzero S scale, and
  nonzero T scale.
- `gNdsOpeningRoomDLPreviewMaterialCount`: number of original `MObj` nodes
  attached to the selected preview DObj. The verifier expects `2` for the
  selected material-bearing candidate.
- `gNdsOpeningRoomDLPreviewMaterialFlags` /
  `MaterialEffectiveFlags`: raw first-MObj flags and the effective flags
  BattleShip's original `gcDrawMObjForDObj` would use after applying its
  default `TEXTURE | 0x20 | ALPHA` fallback. Expected `0x200/0x200`.
- `gNdsOpeningRoomDLPreviewMaterialMask`: material diagnostic readiness bits.
  Expected `0x403` for the current material-bearing candidate.
- `gNdsOpeningRoomDLPreviewMaterialTextureCurr` /
  `MaterialTextureNext` / `MaterialPaletteIndex` /
  `MaterialLfrac100`: first-MObj animation/material IDs when a material chain
  exists. Expected `0,0,0,0` for the current candidate.
- `gNdsOpeningRoomDLPreviewMaterialFormat` / `MaterialSize` /
  `MaterialBlockFormat` / `MaterialBlockSize`: first-MObj texture format
  fields when a material chain exists. Expected `4,2,4,1`.
- `gNdsOpeningRoomDLPreviewMaterialTileWidth` / `MaterialTileHeight` /
  `MaterialScrollWidth` / `MaterialScrollHeight`: first-MObj tile dimensions
  when a material chain exists. Expected `16,32,16,32`.
- `gNdsOpeningRoomDLPreviewMaterialScaleS100` /
  `MaterialScaleT100` / `MaterialTranslateS100` /
  `MaterialTranslateT100`: first-MObj texture scale/translation in centi-units
  when a material chain exists. Expected `100,100,0,0`.
- `gNdsOpeningRoomDLPreviewMaterialSpriteCurr` /
  `MaterialSpriteNext` / `MaterialPalettePtr`: resolved first-MObj sprite and
  palette pointers when original material flags require those arrays. Expected
  `0` for the current first DObj.
- `gNdsOriginalSpritePreviewDisplayWidth` /
  `DisplayHeight`: retained startup logo presentation size. Expected `128`
  and `108`, proving the melonDS visual debug copy is native-size rather than
  the older downscaled DS-logical copy.
- `gNdsOriginalDLPreviewReady` / `Width` / `Height`: retained platform
  presentation state for the top-screen preview, expected `1`, `96`, and `72`.
- `gNdsOriginalDLPreviewCommitCount` / `DrawCount`: retained platform
  presentation counters; both must be at least `1` so parsing cannot pass
  without the visible preview being committed and drawn.

## Failure Triage

If build fails:

1. Identify the first missing symbol/type from the compiler output.
2. Inspect the original BattleShip source and header that define it.
3. Add the smallest compatibility declaration in `include/`.
4. Add a DS backend stub or implementation in `src/port` or `src/nds`.
5. Do not edit `decomp/`; keep hooks in project-owned wrappers/shims.
6. Do not edit generated build output.
7. Do not rewrite original gameplay or scene logic to avoid the missing symbol.

If runtime verification fails:

1. Read the full GDB output printed by the script.
2. Determine the earliest failed marker in boot order.
3. Inspect the source that should set that marker.
4. Check whether a thread is parked, a queue is empty/full, or a service stub is
   returning too early.
5. Add a targeted diagnostic global if the boundary is ambiguous.

If the emulator does not start:

1. Confirm `smash64ds.nds` and `smash64ds.elf` exist.
2. Confirm `emulators/melonds/melonDS.exe` exists or pass `-MelonDS`.
3. If melonDS stays alive but has no visible/capturable window and no ARM9 GDB
   listener on `3333`, first inspect `[Instance0.Gdb]` in
   `emulators/melonds/melonDS.toml` for duplicate `Enable` / `Enabled` keys,
   then test another known-good ROM such as devkitPro's
   `Simple_Tri.nds`. If that also starts hidden/windowless, treat it as a
   melonDS/session launch issue rather than a Smash64DS runtime regression.
4. Delete stale `emulators/melonds/melonDS.toml` only if it is local generated
   config and not a user-edited file.
5. Re-run `make clean; make -j16`.

## Useful Manual GDB Commands

After launching melonDS with GDB enabled:

```powershell
C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe .\smash64ds.elf
```

Inside GDB:

```gdb
target remote 127.0.0.1:3333
p/x gNdsOriginalBootStage
p/x gNdsSceneBoundaryResult
p gNdsStartupTaskmanSceneKind
p/x gNdsStartupLogoRelocResult
p gNdsStartupLogoRelocSize
p gNdsStartupLogoRelocWordSwapCount
p gNdsStartupLogoRelocPointerFixupCount
p/x gNdsStartupLogoDrawResult
p gNdsStartupLogoDrawBlocker
p gNdsStartupLogoDrawCallbackCount
p gNdsStartupLogoDrawUpdateCount
p gNdsStartupLogoDrawWidth
p gNdsStartupLogoDrawHeight
p gNdsStartupLogoDrawBitmaps
p gNdsStartupLogoDrawPixels
p/x gNdsTaskmanCleanupResult
p gNdsOpeningRoomDispatchCount
p/x gNdsOpeningRoomRelocResult
p/x gNdsOpeningRoomRelocFileMask
p/x gNdsOpeningRoomRelocHeaderMask
p/x gNdsOpeningRoomRelocPayloadMask
p gNdsOpeningRoomRelocBytesLoaded
p gNdsOpeningRoomRelocFixupReady
p gNdsOpeningRoomRelocSymbolResolveCount
p/x gNdsOpeningRoomRelocSymbolProbeMask
p gNdsOpeningRoomRelocMObjSubNormalizeCount
p gNdsOpeningRoomRelocMObjSubNormalizeFailCount
p/x gNdsOpeningRoomRelocMObjSubFirstFlags
p/x gNdsOpeningRoomRelocMObjSubSourceResult
p gNdsOpeningRoomRelocMObjSubTextureFlagCount
p gNdsOpeningRoomRelocMObjSubZeroFlagCount
p gNdsOpeningRoomRelocMObjSubPrimColorCount
p gNdsOpeningRoomRelocMObjSubLightCount
p/x gNdsOpeningRoomRelocMObjSubFirstTextureOffset
p/x gNdsOpeningRoomRelocMObjSubFirstTextureFlags
p/x gNdsOpeningRoomFirstEventResult
p gNdsOpeningRoomFirstEventTick
p/x gNdsOpeningRoomFirstEventProbeMask
p gNdsOpeningRoomFirstEventPencilsDObjOffset
p gNdsOpeningRoomFirstEventPencilsAnimOffset
p/x gNdsOpeningRoomFirstEventDataResult
p/x gNdsOpeningRoomFirstEventDataMask
p gNdsOpeningRoomFirstEventPencilsDObjEntries
p gNdsOpeningRoomFirstEventPencilsDLPtrs
p gNdsOpeningRoomFirstEventPencilsAnimJoints
p gNdsOpeningRoomFirstEventPencilsAnimFirstOpcode
p/x gNdsOpeningRoomFirstEventRunResult
p/x gNdsOpeningRoomFirstEventDeferredMask
p/x gNdsOpeningRoomFighterDeferredResult
p gNdsOpeningRoomFighterDeferredKind
p/x gNdsOpeningRoomOverlayCreateResult
p/x gNdsOpeningRoomOverlayEjectResult
p/x gNdsOpeningRoomOverlayEjectUnlinkedMask
p/x gNdsOpeningRoomScene1CameraCreateResult
p/x gNdsOpeningRoomScene1CameraCreateMask
p gNdsOpeningRoomScene1CameraCreateGObjCount
p gNdsOpeningRoomScene1CameraGObjDelta
p gNdsOpeningRoomScene1CameraCObjDelta
p gNdsOpeningRoomScene1CameraXObjDelta
p gNdsOpeningRoomScene1CameraAObjDelta
p gNdsOpeningRoomScene1CameraDisplaySet
p gNdsOpeningRoomScene1CameraProcessSet
p gNdsOpeningRoomScene1CameraAnimSet
p gNdsOpeningRoomScene1CameraViewportSet
p gNdsOpeningRoomScene1CameraDLBufferSet
p/x gNdsOpeningRoomCloseUpOverlayCameraCreateResult
p/x gNdsOpeningRoomCloseUpOverlayCameraCreateMask
p gNdsOpeningRoomCloseUpOverlayCameraCreateGObjCount
p gNdsOpeningRoomCloseUpOverlayCameraGObjDelta
p gNdsOpeningRoomCloseUpOverlayCameraCObjDelta
p gNdsOpeningRoomCloseUpOverlayCameraXObjDelta
p gNdsOpeningRoomCloseUpOverlayCameraDisplaySet
p gNdsOpeningRoomCloseUpOverlayCameraViewportSet
p/x gNdsOpeningRoomLogoCameraAssetMask
p gNdsOpeningRoomLogoCameraAnimOffset
p/x gNdsOpeningRoomLogoCameraCreateResult
p/x gNdsOpeningRoomLogoCameraCreateMask
p gNdsOpeningRoomLogoCameraCreateGObjCount
p gNdsOpeningRoomLogoCameraGObjDelta
p gNdsOpeningRoomLogoCameraCObjDelta
p gNdsOpeningRoomLogoCameraXObjDelta
p gNdsOpeningRoomLogoCameraAObjDelta
p gNdsOpeningRoomLogoCameraViewportSet
p/x gNdsOpeningRoomLogoAssetMask
p gNdsOpeningRoomLogoDObjOffset
p gNdsOpeningRoomLogoMObjOffset
p gNdsOpeningRoomLogoMatAnimOffset
p/x gNdsOpeningRoomLogoCreateResult
p/x gNdsOpeningRoomLogoCreateMask
p/x gNdsOpeningRoomLogoEjectResult
p/x gNdsOpeningRoomLogoEjectUnlinkedMask
p/x gNdsOpeningRoomBossShadowAssetMask
p gNdsOpeningRoomBossShadowDisplayListOffset
p gNdsOpeningRoomBossShadowAnimOffset
p/x gNdsOpeningRoomBossShadowCreateResult
p/x gNdsOpeningRoomBossShadowCreateMask
p/x gNdsOpeningRoomBossShadowEjectResult
p/x gNdsOpeningRoomBossShadowEjectUnlinkedMask
p/x gNdsOpeningRoomPencilsCreateResult
p/x gNdsOpeningRoomPencilsCreateMask
p/x gNdsOpeningRoomTick380DeferredResult
p/x gNdsOpeningRoomTick380DeferredMask
p/x gNdsOpeningRoomTick450RunResult
p/x gNdsOpeningRoomTick450DeferredMask
p/x gNdsOpeningRoomOutsideAssetMask
p gNdsOpeningRoomOutsideDisplayListOffset
p/x gNdsOpeningRoomOutsideCreateResult
p/x gNdsOpeningRoomOutsideCreateMask
p gNdsOpeningRoomOutsideCreateGObjCount
p gNdsOpeningRoomOutsideGObjDelta
p gNdsOpeningRoomOutsideDObjDelta
p gNdsOpeningRoomOutsideXObjDelta
p gNdsOpeningRoomOutsideDisplaySet
p/x gNdsOpeningRoomHazeAssetMask
p gNdsOpeningRoomHazeDisplayListOffset
p/x gNdsOpeningRoomHazeCreateResult
p/x gNdsOpeningRoomHazeCreateMask
p gNdsOpeningRoomHazeCreateGObjCount
p gNdsOpeningRoomHazeGObjDelta
p gNdsOpeningRoomHazeDObjDelta
p gNdsOpeningRoomHazeXObjDelta
p gNdsOpeningRoomHazeDisplaySet
p/x gNdsOpeningRoomSunlightAssetMask
p gNdsOpeningRoomSunlightDisplayListOffset
p/x gNdsOpeningRoomSunlightCreateResult
p/x gNdsOpeningRoomSunlightCreateMask
p gNdsOpeningRoomSunlightCreateGObjCount
p gNdsOpeningRoomSunlightGObjDelta
p gNdsOpeningRoomSunlightDObjDelta
p gNdsOpeningRoomSunlightXObjDelta
p gNdsOpeningRoomSunlightDisplaySet
p/x gNdsOpeningRoomSunlightEjectResult
p gNdsOpeningRoomSunlightEjectBeforeGObjCount
p gNdsOpeningRoomSunlightEjectAfterGObjCount
p/x gNdsOpeningRoomSunlightEjectUnlinkedMask
p/x gNdsOpeningRoomCloseUpOverlayCreateResult
p/x gNdsOpeningRoomCloseUpOverlayCreateMask
p gNdsOpeningRoomCloseUpOverlayCreateTick
p gNdsOpeningRoomCloseUpOverlayCreateGObjCount
p gNdsOpeningRoomCloseUpOverlayGObjDelta
p gNdsOpeningRoomCloseUpOverlayDisplaySet
p gNdsOpeningRoomCloseUpOverlayAlphaInit
p/x gNdsOpeningRoomTick500RunResult
p/x gNdsOpeningRoomTick500DeferredMask
p/x gNdsOpeningRoomSpotlightAssetMask
p gNdsOpeningRoomSpotlightDisplayListOffset
p gNdsOpeningRoomSpotlightMObjOffset
p gNdsOpeningRoomSpotlightMatAnimOffset
p/x gNdsOpeningRoomSpotlightCreateResult
p/x gNdsOpeningRoomSpotlightCreateMask
p gNdsOpeningRoomSpotlightCreateTick
p gNdsOpeningRoomSpotlightCreateGObjCount
p gNdsOpeningRoomSpotlightGObjDelta
p gNdsOpeningRoomSpotlightDObjDelta
p gNdsOpeningRoomSpotlightXObjDelta
p gNdsOpeningRoomSpotlightMObjDelta
p gNdsOpeningRoomSpotlightAObjDelta
p gNdsOpeningRoomSpotlightDisplaySet
p gNdsOpeningRoomSpotlightProcessSet
p gNdsOpeningRoomSpotlightMObjSet
p gNdsOpeningRoomSpotlightMatAnimSet
p gNdsOpeningRoomSpotlightPositionSet
p/x gNdsOpeningRoomDrawResult
p gNdsOpeningRoomDrawBlocker
p gNdsOpeningRoomDrawTickCount
p gNdsOpeningRoomDrawFrameCount
p gNdsOpeningRoomDrawCameraCallbackCount
p gNdsOpeningRoomDrawDisplayCallbackCount
p gNdsOpeningRoomDrawDObjCallbackCount
p/x gNdsOpeningRoomDrawFirstCameraMaskLow
p gNdsOpeningRoomDrawFirstCameraPriority
p/x gNdsOpeningRoomDrawFirstCameraFlags
p gNdsOpeningRoomDrawFirstCameraXObjCount
p gNdsOpeningRoomDrawFirstCameraXObjKind0
p gNdsOpeningRoomDrawFirstCameraXObjKind1
p gNdsOpeningRoomDrawFirstCameraViewportScaleX
p gNdsOpeningRoomDrawFirstCameraViewportScaleY
p gNdsOpeningRoomDrawFirstCameraViewportTransX
p gNdsOpeningRoomDrawFirstCameraViewportTransY
p gNdsRdpDefaultViewportSetCount
p gNdsRdpDefaultViewportScaleX
p gNdsRdpDefaultViewportScaleY
p gNdsRdpDefaultViewportTransX
p gNdsRdpDefaultViewportTransY
p gNdsRdpDefaultViewportScaleZ
p gNdsRdpDefaultViewportTransZ
p gNdsOpeningRoomDrawFirstCameraNear100
p gNdsOpeningRoomDrawFirstCameraFar100
p gNdsOpeningRoomDrawFirstCameraFovY100
p gNdsOpeningRoomDrawFirstCameraEyeX100
p gNdsOpeningRoomDrawFirstCameraEyeY100
p gNdsOpeningRoomDrawFirstCameraEyeZ100
p gNdsOpeningRoomDrawFirstCameraAtX100
p gNdsOpeningRoomDrawFirstCameraAtY100
p gNdsOpeningRoomDrawFirstCameraAtZ100
p gNdsOpeningRoomDrawFirstObjectDLLink
p gNdsOpeningRoomDrawFirstObjectID
p gNdsOpeningRoomDrawFirstObjectKind
p/x gNdsOpeningRoomDrawFirstCallback
p/x gNdsOpeningRoomDrawFirstDObjDL
p/x gNdsOpeningRoomDrawFirstDObjMeta
p/x gNdsOpeningRoomDLPreviewResult
p gNdsOpeningRoomDLPreviewBlocker
p gNdsOpeningRoomDLPreviewCommandCount
p gNdsOpeningRoomDLPreviewVertexCount
p gNdsOpeningRoomDLPreviewTriangleCount
p gNdsOpeningRoomDLPreviewPixelCount
p/x gNdsOpeningRoomDLPreviewFirstOpcode
p/x gNdsOpeningRoomDLPreviewUnsupportedOpcode
p gNdsOpeningRoomDLPreviewUnsupportedCommandCount
p gNdsOpeningRoomDLPreviewVertexCommandCount
p gNdsOpeningRoomDLPreviewTriangleCommandCount
p gNdsOpeningRoomDLPreviewSyncCommandCount
p gNdsOpeningRoomDLPreviewEndCommandCount
p gNdsOpeningRoomDLPreviewBranchCommandCount
p gNdsOpeningRoomDLPreviewBranchCallCount
p gNdsOpeningRoomDLPreviewBranchJumpCount
p gNdsOpeningRoomDLPreviewSegmentResolveCount
p gNdsOpeningRoomDLPreviewColorCommandCount
p/x gNdsOpeningRoomDLPreviewPrimColor
p gNdsOpeningRoomDLPreviewOtherModeCommandCount
p/x gNdsOpeningRoomDLPreviewFirstDL
p/x gNdsOpeningRoomDLPreviewTransformMask
p gNdsOpeningRoomDLPreviewXObjCount
p gNdsOpeningRoomDLPreviewFirstXObjKind
p gNdsOpeningRoomDLPreviewTranslateX100
p gNdsOpeningRoomDLPreviewTranslateY100
p gNdsOpeningRoomDLPreviewTranslateZ100
p gNdsOpeningRoomDLPreviewRotateX100
p gNdsOpeningRoomDLPreviewRotateY100
p gNdsOpeningRoomDLPreviewRotateZ100
p gNdsOpeningRoomDLPreviewScaleX100
p gNdsOpeningRoomDLPreviewScaleY100
p gNdsOpeningRoomDLPreviewScaleZ100
p gNdsOpeningRoomDLPreviewMinX
p gNdsOpeningRoomDLPreviewMaxX
p gNdsOpeningRoomDLPreviewMinY
p gNdsOpeningRoomDLPreviewMaxY
p/x gNdsOpeningRoomDLPreviewProjectionMask
p gNdsOpeningRoomDLPreviewProjectionMode
p gNdsOpeningRoomDLPreviewProjectionBlocker
p gNdsOpeningRoomDLPreviewProjectedVertexCount
p gNdsOpeningRoomDLPreviewProjectedTriangleCount
p gNdsOpeningRoomDLPreviewProjectedMinX
p gNdsOpeningRoomDLPreviewProjectedMaxX
p gNdsOpeningRoomDLPreviewProjectedMinY
p gNdsOpeningRoomDLPreviewProjectedMaxY
p gNdsOpeningRoomDLPreviewProjectedMinDepth100
p gNdsOpeningRoomDLPreviewProjectedMaxDepth100
p/x gNdsOpeningRoomDLPreviewTextureMask
p/x gNdsOpeningRoomDLPreviewTextureImage
p gNdsOpeningRoomDLPreviewTextureFormat
p gNdsOpeningRoomDLPreviewTextureSize
p gNdsOpeningRoomDLPreviewTextureImageWidth
p gNdsOpeningRoomDLPreviewTextureTileWidth
p gNdsOpeningRoomDLPreviewTextureTileHeight
p gNdsOpeningRoomDLPreviewTextureLoadTexels
p gNdsOpeningRoomDLPreviewTextureLoadBlockTile
p gNdsOpeningRoomDLPreviewTextureLoadBlockUls
p gNdsOpeningRoomDLPreviewTextureLoadBlockUlt
p gNdsOpeningRoomDLPreviewTextureLoadBlockLrs
p gNdsOpeningRoomDLPreviewTextureLoadBlockDxt
p gNdsOpeningRoomDLPreviewTextureTileSizeTile
p gNdsOpeningRoomDLPreviewTextureTileSizeUls
p gNdsOpeningRoomDLPreviewTextureTileSizeUlt
p gNdsOpeningRoomDLPreviewTextureTileSizeLrs
p gNdsOpeningRoomDLPreviewTextureTileSizeLrt
p gNdsOpeningRoomDLPreviewTextureTexelWidth
p gNdsOpeningRoomDLPreviewTextureTexelHeight
p gNdsOpeningRoomDLPreviewTextureSamplePixels
p gNdsOpeningRoomDLPreviewTextureSetTileCount
p/x gNdsOpeningRoomDLPreviewTextureCombineW0
p/x gNdsOpeningRoomDLPreviewTextureCombineW1
p gNdsOpeningRoomDLPreviewTextureRenderTile
p gNdsOpeningRoomDLPreviewTextureRenderTileLine
p gNdsOpeningRoomDLPreviewTextureRenderTileTmem
p gNdsOpeningRoomDLPreviewTextureRenderTilePalette
p gNdsOpeningRoomDLPreviewTextureRenderTileCms
p gNdsOpeningRoomDLPreviewTextureRenderTileCmt
p gNdsOpeningRoomDLPreviewTextureRenderTileMasks
p gNdsOpeningRoomDLPreviewTextureRenderTileMaskt
p gNdsOpeningRoomDLPreviewTextureRenderTileShifts
p gNdsOpeningRoomDLPreviewTextureRenderTileShiftt
p/x gNdsOpeningRoomDLPreviewTextureRenderTileFlags
p gNdsOpeningRoomDLPreviewTextureCommandCount
p gNdsOpeningRoomDLPreviewTextureScaleS
p gNdsOpeningRoomDLPreviewTextureScaleT
p gNdsOpeningRoomDLPreviewTextureLevel
p gNdsOpeningRoomDLPreviewTextureTile
p gNdsOpeningRoomDLPreviewTextureOn
p gNdsOpeningRoomDLPreviewTextureXParam
p/x gNdsOpeningRoomDLPreviewTextureStateFlags
p/x gNdsOpeningRoomDLPreviewRendererTextureMask
p/x gNdsOpeningRoomDLPreviewRendererTextureImage
p gNdsOpeningRoomDLPreviewRendererTextureFormat
p gNdsOpeningRoomDLPreviewRendererTextureSize
p gNdsOpeningRoomDLPreviewRendererTextureImageWidth
p gNdsOpeningRoomDLPreviewRendererTextureLoadTexels
p gNdsOpeningRoomDLPreviewRendererTextureSetTileCount
p gNdsOpeningRoomDLPreviewRendererTextureCommandCount
p/x gNdsOpeningRoomDLPreviewRendererTextureStateFlags
p gNdsOpeningRoomDLPreviewRendererTextureTileWidth
p gNdsOpeningRoomDLPreviewRendererTextureTileHeight
p gNdsOpeningRoomDLPreviewRendererTextureRenderTile
p gNdsOpeningRoomDLPreviewRendererTextureRenderTileLine
p/x gNdsOpeningRoomDLPreviewRendererTextureRenderTileFlags
p gNdsOpeningRoomDLPreviewRendererTextureLoadBlockLrs
p gNdsOpeningRoomDLPreviewRendererTextureLoadBlockDxt
p gNdsOriginalDLPreviewReady
p gNdsOriginalDLPreviewWidth
p gNdsOriginalDLPreviewHeight
p gNdsOriginalSpritePreviewCommitCount
p gNdsOriginalDLPreviewCommitCount
p gNdsOriginalDLPreviewDrawCount
p gNdsPerfPresentFps
p gNdsPerfLogicFps
p gNdsPerfDLDrawFps
p gNdsPerfPreviewCommitFps
p gNdsPerfPreviewCommitCount
p gNdsPerfSampleCount
p gNdsPerfSampleWindowTicks
p/x gNdsOpeningRoomUpdateResult
p gNdsOpeningRoomTickCount
p/x gNdsOpeningRoomPreAssetResult
p gNdsOpeningRoomControllerCheckCount
p sSYSchedulerTicCount
p gSYControllerMain
detach
quit
```

Prefer adding checks to `scripts/verify-runtime.ps1` once a diagnostic becomes
part of the expected port contract.

## Mario/Fox gcRunAll Loop Markers

Use these verifiers for the current selected-fighter object-manager boundary:

```powershell
.\scripts\verify-battle-mariofox-gcrunall-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-gcrunall-loop-harness.ps1
```

Marker groups:

- `GCRUNALL_LOOP`: result, safe result, proof mask, deferred mask, fighter
  completion count, frame cap, and update cap.
- `GCRUNALL_TASKMAN`: prepared flag, bounded taskman updates,
  VSBattle-update calls, base original update calls, original `gcRunAll()`
  call count, and total bounded taskman updates.
- `GCRUNALL_RUN`: previous proof-owned process pauses, non-target GObj visits,
  non-target process pauses, target process preservation, and GObj count
  before/after.
- `GCRUNALL_PROCESS`: original `gcAddGObjProcess` attach counts and selected
  Mario/Fox callback counts reached through `gcRunAll`.
- `GCRUNALL_INPUT`: deterministic controller playback and original
  controller-read/global-update bridge evidence; direct FTStruct script writes
  must remain zero.
- `GCRUNALL_STATUS`, `GCRUNALL_VISITS`, `GCRUNALL_CALLS`, `GCRUNALL_MOVE`,
  and `GCRUNALL_TRANS`: Wait/Walk/Dash/Run/RunBrake/KneeBend/JumpF/Fall/
  LandingLight/Wait movement state, transition, callback, root-motion, and
  floor-return evidence.
- `GCRUNALL_DRAW` and `GCRUNALL_SCREEN`: bounded `96x72` all-DL software
  preview evidence entered through guarded `ftDisplayMainProcDisplay`.
- `GCRUNALL_SAFE`: GObj delta, unexpected/denied status, process-escape,
  display-probe, gameplay, draw, matrix, root-Y drift, and GA drift counters.

Current proof:

```text
Battle Mario/Fox gcRunAll-loop harness passed: scene=22/21 gcRunAll=30 callbacks=30/30 draws=11 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1
Menu-chain Mario/Fox gcRunAll-loop harness passed: scene=22/21 gcRunAll=30 callbacks=30/30 draws=11 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1
```

This proves only selected Mario/Fox callbacks reached through bounded original
`gcRunAll()` after pausing previous proof-owned and non-target object
processes. It does not prove unpaused full-scene `gcRunAll`, continuous
taskman scheduling, arbitrary fighter processes, full collision, hardware
fighter rendering, HUD, audio, or full imported `ftmain.c`.

## Mario/Fox Live Preview Markers

Use these verifiers for the current live controller source boundary:

```powershell
.\scripts\verify-battle-mariofox-live-preview-harness.ps1
.\scripts\verify-menu-chain-mariofox-live-preview-harness.ps1
```

Marker groups:

- `LIVE_LOOP`: result, safe result, proof mask, frame target, frame count, and
  selected Mario/Fox completion counts.
- `LIVE_BACKEND`: live controller read count, connected mask, P0 button/stick
  sample, and controller-map count from `osContGetReadData`.
- `LIVE_TASKMAN`: prepared flag, bounded taskman update counts, base original
  VSBattle update calls, and original `gcRunAll()` call count.
- `LIVE_RUN`: proof-owned process pauses, non-target process pauses, target
  process preservation, and GObj count stability.
- `LIVE_INPUT`: original controller read/global-update evidence and DS-owned
  bridge counts. Direct FTStruct script writes must remain zero in automated
  idle proofs.
- `LIVE_STATUS`, `LIVE_CALLS`, `LIVE_MOVE`, and `LIVE_SAFE`: final Wait/Ground
  idle state, selected callback visits, root/floor stability, and escape/drift
  guards.
- `LIVE_DRAW`: bounded moving-preview draw evidence copied from the maintained
  all-DL software preview helper while staying inside the selected fighter
  display boundary.

Current proof:

```text
Battle Mario/Fox live-preview harness passed: scene=22/21 liveReads=60 frames=60/60 draws=16 pixels=1410 idle=Wait/Ground directInput=0 safe=1
Menu-chain Mario/Fox live-preview harness passed: scene=22/21 liveReads=60 frames=60/60 draws=16 pixels=1410 idle=Wait/Ground directInput=0 safe=1
```

Build a longer manual live-preview ROM with:

```powershell
make TARGET=smash64ds-live-preview BUILD=build-live-preview NDS_DEV_SCENE_HARNESS=battle_mariofox_live_preview NDS_DEV_LIVE_INPUT_PREVIEW=1 -j16
```

The dev build extends the frame cap for manual P0 input checks. It is not an
automated pass condition and does not make the battle loop fully playable.

## Mario/Fox gcDrawAll Loop Markers

Use these verifiers for the current selected-fighter object-manager draw
boundary:

```powershell
.\scripts\verify-battle-mariofox-gcdrawall-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-gcdrawall-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-gcdrawall-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-gcdrawall-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-collision-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-collision-loop-harness.ps1
.\scripts\verify-battle-mariofox-stage-floor-edge-loop-harness.ps1
.\scripts\verify-menu-chain-mariofox-stage-floor-edge-loop-harness.ps1
```

Marker groups:

- `GCDRAWALL_LOOP`: result, safe result, proof mask, deferred mask, selected
  fighter completion count, frame cap, and update cap. Current pass values are
  `0x46444150`, `0x46444153`, mask `0x1fff`, deferred mask `0xff`, and count
  `2`.
- `GCDRAWALL_TASKMAN`: prepared flag, bounded taskman updates, VSBattle update
  counts, base original update counts, original `gcRunAll()` call count,
  original `gcDrawAll()` keyframe count, battle camera callback count, and total
  bounded taskman updates.
- `GCDRAWALL_RUN`: previous proof-owned process pauses, non-target GObj visits,
  non-target process pauses, target process preservation, and GObj count
  before/after.
- `GCDRAWALL_PROCESS`: original `gcAddGObjProcess` attach counts and selected
  Mario/Fox process callback counts reached through original `gcRunAll`.
- `GCDRAWALL_INPUT`: deterministic playback, original controller read/global
  update, DS-owned `gSYControllerDevices` to `FTStruct` bridge evidence, button
  masks, dash tap counts, jump tap counts, and direct FTStruct script writes.
  Direct FTStruct writes must stay zero.
- `GCDRAWALL_STATUS`: start/final status, motion, ground-air state, status visit
  masks, and transition masks. Current required masks are status `0x3ff` and
  transition `0x7ff` for both fighters.
- `GCDRAWALL_DRAW`: retained `96x72` original-DL preview state, commit delta,
  draw keyframe count, selected display callback counts, captured display
  counts, non-target display callback escape count, DL-ready DObj census,
  drawn DObj count, pixel count, and color checksums.
- `GCDRAWALL_SCREEN` and `GCDRAWALL_MOVE`: projected screen movement, jump rise,
  root movement, root rise, floor clamp, direction, and final floor evidence.
- `GCDRAWALL_TRANS`: Fall/Landing/Ground/Air/Wait/RunBrake/Jump/Landing end
  transition counters and deferred interrupt checks.
- `GCDRAWALL_SAFE`: GObj delta, unexpected/denied status, process-attach escape,
  display-probe, gameplay, draw, matrix, root-Y drift, and GA drift counters.
  All current safety values must remain zero.
- `PLATFORM_DL_PREVIEW`: retained preview surface readiness, size, and commit
  count after the `gcDrawAll` keyframes.
- `STAGE_GCDRAWALL`: stage-inclusive result, safe result, proof mask, deferred
  mask, and selected fighter completion count. Current pass values are
  `0x46534744`, `0x46534753`, mask `0xfff`, deferred mask `0xff`, and count
  `2`.
- `STAGE_GCDRAWALL_CAPTURE`: original `gcDrawAll` call count, battle camera
  callback count, Pupupu display-layer capture mask, Pupupu map-GObj capture
  mask, captured stage display count, and selected fighter display callback
  count. Current layer/map capture masks are both `0xf`.
- `STAGE_GCDRAWALL_DOBJ`: stage DObj draw bridge callback count, bridge kind
  mask, layer/map DObj masks, and layer/map DL-ready masks. Current DObj and
  DL-ready masks are complete for the selected Pupupu layer/map GObjs.
- `STAGE_GCDRAWALL_SAFE`: prepared/base-result flags, manual stage-display
  call count, unexpected scene count, non-stage capture count, and GObj count
  delta. Manual calls, unexpected scene escapes, and GObj delta must stay zero.
- `STAGE_GCDRAWALL_PIXELS`: retained preview commit delta, total selected
  fighter preview pixel count, and stage draw compatibility mask.
- `STAGE_GCDRAWALL_HW`: opt-in hardware replay DObj submit count, hardware
  triangle count, z-buffered triangle count, projected-depth triangle count,
  decal-depth triangle count, texture bind count, texture upload count, ready
  texture bind count, unsupported texture reject count, texture format mask,
  and max ready texture dimensions. Current direct-stage hardware pass values
  are `252`, `1152`, `456`, `696`, `0`,
  `618`, `72`, `618`, `0`, `0x4`, and `32x32`.
- `STAGE_COLLISION`: geometry-backed floor-collision result, safe result, proof
  mask, deferred mask, and selected fighter count. Current pass values are
  `0x4653434c`, `0x46534353`, mask `0xffff`, deferred mask `0xff`, and count
  `2`.
- `STAGE_COLLISION_GEOM`: prepared flag, base stage draw flag, ground-data
  ready flag, geometry ready flag, yakumono count, map-object count, floor-line
  count, and total decoded line count. Current Pupupu proof expects one
  yakumono, 42 map objects, four floor lines, and seven total decoded lines.
- `STAGE_COLLISION_PROJECT`: total project calls, geometry-backed project calls,
  legacy-flat fallback count, no-geometry count, out-of-range line count, bad
  vertex count, division-guard count, and total probe count.
  Current proof expects zero legacy flat fallback and zero unsafe fallback after
  the geometry path is prepared.
- `STAGE_COLLISION_PROBES`: total probe count, hit count, miss count, offstage
  miss count, below-floor miss count, P0/P1 project counts, and P0/P1 hit
  counts. Current proof expects three hits and two deliberate misses.
- `STAGE_COLLISION_P0` / `STAGE_COLLISION_P1`: final real floor line ID,
  floor distance, floor flags, floor angle, root/floor Y samples, edge sample,
  and final floor-OK flag.
- `STAGE_COLLISION_EDGE`: final left/right edge samples for both resolved floor
  lines.
- `STAGE_COLLISION_KIND`: decoded floor group ID/count, floor line min/max
  exclusive range, final P0/P1 line kind, final P0/P1 line IDs, final P0/P1
  line-is-floor flags, non-floor candidate count, yakumono DObj deferred count,
  and unsafe yakumono DObj index guard count. Current proof resolves Mario/Fox
  to decoded floor line IDs `0/0` inside floor range `[0,4)` and requires zero
  non-floor candidates.
- `STAGE_COLLISION_SAFE`: final P0/P1 floor OK flags, GObj delta, unexpected
  scene count, unexpected status count, and unsafe fallback count. All safety
  escape counters must remain zero.
- `STAGE_FLOOR_FOLLOW`: continuous floor-follow result, safe result, proof
  mask, deferred mask, and selected fighter count. Current pass values are
  `0x46464c50`, `0x46464c53`, mask `0xffff`, deferred mask `0xff`, and count
  `2`.
- `STAGE_FLOOR_FOLLOW_SETUP`: prepared flag, base stage draw flag, base
  collision flag, geometry-ready flag, initial seed/adopt counts, final
  re-center count, and final adopt count. The floor-follow proof expects the
  base draw/collision geometry path to be ready and the final re-center/adopt
  counts to stay zero.
- `STAGE_FLOOR_FOLLOW_UPDATES`: total map update calls, project calls, hit
  count, miss count, no-geometry count, non-floor count, clamp count, and
  P0/P1 update counts. Current proof expects 18/18 P0/P1 updates and hits.
- `STAGE_FLOOR_FOLLOW_P0` / `STAGE_FLOOR_FOLLOW_P1`: resolved real floor line
  ID, line kind, line-is-floor flag, root delta X, final root Y, floor Y,
  floor OK flag, visit mask, final status, and final ground/air state.
- `STAGE_FLOOR_FOLLOW_DRIFT`: P0/P1 final post-clamp floor drift, max drift,
  and P0/P1 hit counts. Current proof expects zero final drift for both
  selected fighters.
- `STAGE_FLOOR_EDGE`: floor-edge result, safe result, proof mask, deferred
  mask, and selected fighter count. Current pass values are `0x4653454c`,
  `0x46534553`, mask `0xffff`, deferred mask `0xff`, and count `2`.
- `STAGE_FLOOR_EDGE_LINE`: prepared flag, base floor-follow flag, selected
  real floor line ID, line kind, line-is-floor flag, floor width, floor range,
  and total floor line count. Current proof selects decoded line ID `3`.
- `STAGE_FLOOR_EDGE_P0` / `STAGE_FLOOR_EDGE_P1`: seeded root X, final root X,
  floor Y, distance from the relevant edge, root delta, floor OK flag, visit
  mask, final status, and final ground/air state.
- `STAGE_FLOOR_EDGE_PROBES`: inside-left hit, inside-right hit, outside-left
  miss, outside-right miss, and floor common-query hit/miss counts.
- `STAGE_FLOOR_EDGE_QUERIES`: line-type query count, vertex-position query
  count, floor common-query count, left/right edge-under query counts, and
  deferred edge-under count. Edge-under helpers intentionally return `-1`
  until wall/ledge/platform contracts are imported.
- `STAGE_FLOOR_EDGE_UPDATES`: inherited floor-follow update and hit counts for
  P0/P1, plus post-clamp drift counters for the edge proof.
- `STAGE_FLOOR_EDGE_SAFE`: final floor OK flags, GObj delta, unexpected scene
  count, unexpected status count, unsafe fallback count, outside-query failure
  count, and edge-under non-deferred count. All current safety escape counters
  must remain zero.
- `STAGE_MPPROCESS_FLOOR`: source-order MP floor-process result, safe result,
  proof mask, deferred mask, and selected fighter count. Current pass values
  are `0x46534d50`, `0x46534d53`, mask `0xffff`, deferred mask `0xff`, and
  count `2`.
- `STAGE_MPPROCESS_FLOOR_ADAPTER`: prepared flag, base floor-edge seen flag,
  original-layout `MPCollData` adapter build count, adapter copyback count,
  and LR fallback count. Current proof records `36/36` build/copyback passes
  and zero LR fallbacks.
- `STAGE_MPPROCESS_FLOOR_CALLS`: `mpProcessSetCollProjectFloorID` call/hit/
  miss counts, `mpProcessCheckTestFloorCollisionNew` call/hit/miss counts,
  edge-branch count, set-project fallback count, and local-probe
  `mpProcessSetLandingFloor` / `mpProcessSetCollideFloor` call counts.
- `STAGE_MPPROCESS_FLOOR_FC`: positive, negative, and zero signed-distance
  counts from `mpCollisionGetFCCommonFloor`. Positive distance proves the
  bounded helper no longer rejects an object below the selected floor.
- `STAGE_MPPROCESS_FLOOR_PROBES`: local inside probe count/hit count, outside
  probe count/miss count, below-floor probe count, and below-floor positive
  distance count.
- `STAGE_MPPROCESS_FLOOR_P0` / `STAGE_MPPROCESS_FLOOR_P1`: live per-fighter
  update count, hit count, miss count, final line ID, final line-is-floor flag,
  final `mask_stat`, final floor distance in milli-units, and final root Y in
  milli-units. Current proof resolves both fighters to line ID `3` with
  `MAP_FLAG_FLOOR` set.
- `STAGE_MPPROCESS_FLOOR_SAFE`: no-final-recenter evidence, unexpected scene
  count, unexpected status count, and unsafe count. Current proof expects
  `1,0,0,0`.
- `STAGE_MPUPDATE_FLOOR`: source-order MP update-main floor result, safe
  result, proof mask, deferred mask, and selected fighter count. Current pass
  values are `0x46554d50`, `0x46554d53`, mask `0xffff`, deferred mask `0xff`,
  and count `2`.
- `STAGE_MPUPDATE_FLOOR_ADAPTER`: prepared flag, base MP-process proof seen
  flag, original-layout `MPCollData` adapter build count, and adapter copyback
  count.
- `STAGE_MPUPDATE_FLOOR_UPDATE`: `mpProcessUpdateMain` call count, TRUE/FALSE
  return counts, total step count, max step count, split count, cap count,
  translate-reset count, and proc-coll call count.
- `STAGE_MPUPDATE_FLOOR_COLL`: bounded
  `mpCommonRunFighterAllCollisions` call count, floor hit/miss counts,
  cliff-edge/stop-edge/default branch counts, and deferred wall, ceiling,
  floor-edge-adjust, and second-floor-test counts.
- `STAGE_MPUPDATE_FLOOR_CHECKS`: bounded `mpCommonCheckFighterOnFloor` and
  `mpCommonCheckFighterOnCliffEdge` call/hit/miss counts. The current moving
  proof routes selected Mario/Fox map callbacks through the cliff-edge wrapper
  and keeps floor misses at zero.
- `STAGE_MPUPDATE_FLOOR_PROBES`: standalone inside, outside, below-floor, and
  split-step probe counts/results for the update-main stepping path.
- `STAGE_MPUPDATE_FLOOR_P0` / `STAGE_MPUPDATE_FLOOR_P1`: per-fighter update,
  hit, miss, `pos_diff.x`, root X before/final, root Y final, final line ID,
  final `mask_stat`, and final floor-OK flag. Current proof resolves both
  fighters to line ID `3` with nonzero `pos_diff.x` and `MAP_FLAG_FLOOR` set.
- `STAGE_MPUPDATE_FLOOR_SAFE`: no-final-recenter evidence, Fall/Ottotto denial
  counters, unexpected scene/status counters, and unsafe count. Current proof
  expects all safety escape counters to stay zero.
- `STAGE_MPSWEEP_FLOOR`: source-order MP floor-line sweep result, safe
  result, proof mask, deferred mask, and selected fighter count. Current pass
  values are `0x46535750`, `0x46535753`, mask `0xffff`, deferred mask `0xff`,
  and count `2`.
- `STAGE_MPSWEEP_FLOOR_SETUP`: prepared flag, base MP update-main proof seen
  flag, sweep standalone probe count, and old second-floor deferred count.
  Current proof expects the old deferred count to stay zero in modes `71/72`.
- `STAGE_MPSWEEP_FLOOR_CHECK`: `mpProcessCheckTestFloorCollision` call, hit,
  miss, final accepted line, and previous line values. Current proof records
  an accepted different-line probe from line `3` to line `0`.
- `STAGE_MPSWEEP_FLOOR_SWEEP`: same-line and different-line helper call/hit
  counts plus visited/candidate masks. Same-line hits are rejected by the
  second-floor test; the different-line helper proves the bounded alternate
  floor path.
- `STAGE_MPSWEEP_FLOOR_SECOND`: selected-callback second-floor branch call,
  hit, miss, landing-floor, floor-edge-adjust, and collision-end-clear counts.
  Current moving Mario/Fox callbacks call the branch and reject same-line or
  no-new-floor cases while standalone probes prove the accepted different-line
  branch.
- `STAGE_MPSWEEP_FLOOR_PROBES`: same-line reject, different-line accept,
  no-hit miss, old-deferred-zero, second-floor-called, landing-bookkeeping, and
  floor-edge-adjust bookkeeping evidence. The source-order floor-edge-adjust
  checks are covered by the `STAGE_MPADJUST_FLOOR*` markers.
- `STAGE_MPSWEEP_FLOOR_P0` / `STAGE_MPSWEEP_FLOOR_P1`: per-fighter final line
  ID, final floor-OK flag, final `mask_stat`, second-floor call count, and
  second-floor miss count. Current proof keeps both fighters on line ID `3`
  with `MAP_FLAG_FLOOR` set.
- `STAGE_MPCROSS_FLOOR`: source-order MP cross-floor result, safe result,
  proof mask, deferred mask, and selected fighter count. Current pass values
  are `0x46435250`, `0x46435253`, mask `0xffff`, deferred mask `0xff`, and
  count `2`.
- `STAGE_MPCROSS_FLOOR_SETUP`: prepared flag, base MP sweep proof seen flag,
  prime attempt/hit/miss counts, source line ID, target line ID, target
  X/Y milli diagnostics, and unsafe count. Current proof primes P0 with source
  line `-1` and targets decoded Pupupu line `3`.
- `STAGE_MPCROSS_FLOOR_LIVE`: live selected-callback second-floor call, hit,
  miss, accepted-new-line, landing-floor, floor-edge-adjust, and
  collision-end-clear counts. Current proof records 17 live hits and 17
  accepted-new-line events.
- `STAGE_MPCROSS_FLOOR_P0`: P0 live cross hit count, final line ID,
  target-line match count, and final floor-OK flag. Current proof expects a
  live `-1 -> 3` path with final line `3`.
- `STAGE_MPCROSS_FLOOR_P1`: P1 live cross hit count, final line ID, and final
  floor-OK flag. Current proof keeps P1 on line `3` as the retained-floor
  control case.
- `STAGE_MPADJUST_FLOOR`: source-order MP floor-edge-adjust result, safe
  result, proof mask, deferred mask, and selected fighter count. Current pass
  values are `0x46454150`, `0x46454153`, mask `0xffff`, deferred mask `0xff`,
  and count `2`.
- `STAGE_MPADJUST_FLOOR_SETUP`: prepared flag, base MP cross-floor proof seen
  flag, `mpProcessRunFloorEdgeAdjust` call count, and unsafe count. Current
  proof records `17` live calls with unsafe count `0`.
- `STAGE_MPADJUST_FLOOR_CHECK`: left/right
  `mpProcessCheckFloorEdgeCollisionL/R` call/hit/miss counts. Current proof
  records `17/17` left/right calls, zero hits, and `17/17` misses for the
  floor-only slice.
- `STAGE_MPADJUST_FLOOR_WALL`: left/right
  `mpCollisionCheckL/RWallLineCollisionSame` call/hit/miss counts. Current
  proof records `17/17` wall sweep calls, zero wall hits, and `17/17` misses.
- `STAGE_MPADJUST_FLOOR_EDGE`: left/right edge-under call counts, edge-under
  deferral count, left/right adjust call counts, and no-adjust count. Current
  proof records `17/17` edge-under calls, `34` deferrals, zero left/right
  adjust calls, and `17` no-adjust outcomes.
- `STAGE_MPADJUST_FLOOR_P0P1`: P0/P1 adjust-run counts, P0/P1 adjust-local
  final line IDs, and P0/P1 adjust-local floor-OK flags. Current proof expects
  P0 to run the live adjust branch and end on line `3`; P1 is intentionally a
  retained-floor control and is verified through `STAGE_MPUPDATE_FLOOR_P1` as
  maintained on line `3`.
- `STAGE_MPEDGE_FLOOR`: source-order MP edge-under/floor-edge result, safe
  result, proof mask, deferred mask, and selected fighter count. Current pass
  values are `0x46454750`, `0x46454753`, mask `0xffff`, deferred mask `0xff`,
  and count `2`.
- `STAGE_MPEDGE_FLOOR_SETUP`: prepared flag, base MPAdjust proof seen flag,
  selected floor line ID, selected floor validity, and unsafe count. Current
  proof expects selected floor line `3`, a valid decoded floor, and unsafe
  count `0`.
- `STAGE_MPEDGE_FLOOR_EDGE`: left/right edge-under call, hit, and miss counts.
  Current proof records `18/18` left/right calls, nonzero left/right hits, and
  zero misses while keeping MPAdjust edge-under deferral at `0`.
- `STAGE_MPEDGE_FLOOR_LINE`: resolved left/right edge-under line IDs and line
  kinds. Current proof resolves wall line IDs `6/5` with wall kinds `3/2` from
  decoded Pupupu geometry adjacency.
- `STAGE_MPWALL_FLOOR`: source-order MP wall-blocker result, safe result,
  proof mask, deferred mask, and selected fighter count. Current pass values
  are `0x46574c50`, `0x46574c53`, mask `0xffff`, deferred mask `0xff`, and
  count `2`.
- `STAGE_MPWALL_FLOOR_SETUP`: prepared flag, base MPEdge proof seen flag,
  candidate probe count, probe-hit count, probe-miss count, and unsafe count.
  Current proof expects prepared/base seen, nonzero candidate probes, zero
  wall hits, nonzero misses, and unsafe count `0`.
- `STAGE_MPWALL_FLOOR_LINE`: selected floor line ID, candidate wall line ID,
  wall kind, edge-under line ID, side, check-hit count, adjust-call count, and
  final-floor-OK flag. Current proof records floor line `3`, wall line equal
  to edge-under line (`6` on the left or `5` on the right), expected wall kind
  `3/2`, zero check hits, zero adjust calls, and final-floor-OK `0`.
- `STAGE_MPWALL_FLOOR_POS`: start X/Y, final X/Y, and delta X/Y in
  milli-units for the wall-hit adjustment branch. Current proof expects all
  values to remain zero because original `mpProcessCheckFloorEdgeCollisionL/R`
  rejects the edge-under wall candidate before adjustment.
- `STAGE_MPWALL_HIT_SCOUT`: geometry-wide source-order wall-hit scout counts
  for the currently loaded map: run count, floor tests, wall tests, non-edge
  wall candidates, source-order hits, misses, and unsafe count. Current Dream
  Land proof records one run, `4` floor tests, `8` wall tests, `6` non-edge
  candidates, zero hits, nonzero misses, and unsafe count `0`.
- `STAGE_MPWALL_HIT_SCOUT_LINE`: wall-hit scout floor line, wall line,
  edge-under line, side, wall kind, and final-floor-OK flag. Current Dream
  Land proof records `-1/-1/-1` and final-floor-OK `0` because no source-order
  non-edge wall-hit adjustment case exists in the loaded Pupupu geometry.
- `STAGE_MPWALL_HIT_SCOUT_POS`: wall-hit scout start X/Y, final X/Y, and
  delta X/Y in milli-units. Current Dream Land proof keeps all values at zero
  because the geometry-wide scout found no adjustment hit.
- `STAGE_MPWALL_HYRULE_SCOUT`: isolated Hyrule Castle source-order wall-hit
  scout relocation/counter marker: reloc result, ground-data ready,
  geometry-ready, map-node ready, run count, floor tests, wall tests,
  non-edge candidates, hits, misses, and unsafe count. Current proof expects
  reloc result `0x4859524c`, ready flags, one run, nonzero tests/candidates,
  at least one hit, and unsafe count `0`.
- `STAGE_MPWALL_HYRULE_SCOUT_LINE`: Hyrule scout floor line, wall line,
  edge-under line, side, wall kind, and final-floor-OK flag. Current proof
  records floor `5`, wall `13`, edge-under `12`, left side `0`, and
  final-floor-OK `1`.
- `STAGE_MPWALL_HYRULE_SCOUT_POS`: Hyrule scout start X/Y, final X/Y, and
  delta X/Y in milli-units. Current proof records a source-order wall-hit
  adjustment delta of `-1600/-388`.
- `STAGE_MPWALLHIT_FLOOR`: promoted Hyrule wall-hit proof result, safe result,
  proof mask, deferred mask, and hit count. Current pass values are
  `0x46574850`, `0x46574853`, mask low bits `0x3ff`, deferred mask `0xff`,
  and count greater than zero.
- `STAGE_MPWALLHIT_FLOOR_SETUP`: prepared flag, Dream Land wall-blocker/base
  evidence flag, Hyrule reloc result, Hyrule ground-data ready flag,
  geometry-ready flag, map-node ready flag, and unsafe count. Current proof
  expects prepared/base evidence, reloc result `0x4859524c`, ready flags, and
  unsafe count `0`.
- `STAGE_MPWALLHIT_FLOOR_COUNT`: run count, floor tests, wall tests,
  non-edge candidates, hit count, and miss count copied from the isolated
  Hyrule source-order proof window. Current proof expects one run, nonzero
  tests/candidates, and at least one hit.
- `STAGE_MPWALLHIT_FLOOR_LINE`: promoted Hyrule candidate floor line, wall
  line, edge-under line, side, wall kind, and final-floor-OK flag. Current
  proof records floor `5`, wall `13`, edge-under `12`, side `0`, wall kind
  `3`, and final-floor-OK `1`.
- `STAGE_MPWALLHIT_FLOOR_POS`: promoted Hyrule candidate start X/Y, final X/Y,
  and delta X/Y in milli-units. Current proof records delta `-1600/-388`.
- `STAGE_MPWALLCOPY_FLOOR`: selected-callback wall-copy proof result, safe
  result, proof mask, deferred mask, and copyback count. Current pass values
  are `0x46574350`, `0x46574353`, mask low bits `0x3ff`, deferred mask
  `0xff`, and count `1`.
- `STAGE_MPWALLCOPY_BASE`: wall-copy prepared flag, base Hyrule wall-hit proof
  seen flag, proof-owned process attach count, process-run count, callback
  count, and copyback count. Current proof expects each value to be `1`.
- `STAGE_MPWALLCOPY_SRC`: copied Hyrule source candidate floor line, wall line,
  edge-under line, and side. Current proof records floor `5`, wall `13`,
  edge-under `12`, side `0`.
- `STAGE_MPWALLCOPY_POS`: wall-copy start X/Y, final X/Y, and delta X/Y in
  milli-units. Current proof records the same source-order adjustment delta as
  the Hyrule wall-hit proof: `-1600/-388`.
- `STAGE_MPWALLCOPY_STATE`: P0 final-floor-OK flag, P0 final collision mask,
  P0 final ground/air state, unsafe count, and P1 root before/after X/Y
  markers. Current proof expects P0 floor OK, `MAP_FLAG_FLOOR` in the final
  mask, `nMPKineticsGround`, unsafe count `0`, and unchanged P1 root
  coordinates.
- `STAGE_MPPASS_FLOOR`: selected-callback pass-through floor proof result,
  safe result, proof mask, deferred mask, and accepted pass count. Current pass
  values are `0x46505050`, `0x46505053`, mask low bits `0x7ff`, deferred mask
  `0xff`, and count `1`.
- `STAGE_MPPASS_BASE`: pass-through prepared flag, base wall-copy proof seen
  flag, candidate scan count, candidate count, no-candidate blocker, and unsafe
  count. Current proof expects prepared/base flags, nonzero scan count, one
  selected candidate, no blocker, and unsafe count `0`.
- `STAGE_MPPASS_LINE`: selected pass-through floor line, raw vertex flags, and
  `MAP_VERTEX_COLL_PASS` presence flag. Current proof records Dream Land line
  `0`, flags `0x4000`, and pass flag `1`.
- `STAGE_MPPASS_ROUTE`: `MAP_PROC_TYPE_PASS` route count, same-line
  `ignore_line_id` rejection count, different-line acceptance count, pass
  callback count, pass callback allow count, and deny count. Current proof
  records `2/1/1/1/1/0`.
- `STAGE_MPPASS_PROCESS`: proof-owned process attach count, GObj process run
  count, and selected callback count. Current proof expects `1/1/1`.
- `STAGE_MPPASS_P1`: P1 root before/after X/Y and unchanged flag. Current
  proof expects before/after coordinates to match and unchanged flag `1`.
- `STAGE_MPPLATFORM_FLOOR`: selected-callback platform floor result, safe
  result, proof mask, deferred mask, and active platform count. Modes
  `141/142` keep the inactive blocker regression with count `0` and blocker
  `0x40`. Modes `143/144` are the active predicate regression and expect
  `0x46504c50`, `0x46504c53`, mask low bits `0xff`, deferred mask `0xff`,
  and active platform count `1`.
- `STAGE_MPPLATFORM_BASE`: platform scout prepared flag, base pass-through
  proof seen flag, platform probe count, and unsafe count. Current proof
  expects `1/1/1/0`.
- `STAGE_MPPLATFORM_LINE`: selected platform probe line, raw vertex flags,
  `MAP_VERTEX_COLL_PASS` presence flag, yakumono id, and yakumono count.
  Current proof records Dream Land line `0`, flags `0x4000`, pass flag `1`,
  yakumono id `1`, and yakumono count `1`.
- `STAGE_MPPLATFORM_DOBJ`: yakumono DObj-present flag, DObj status, DObj anim
  present flag, original platform predicate result, and blocker mask. The old
  inactive scout now records `1/0/0/0/0x40`: the bounded DObj slot exists, but
  its status is off. The active predicate regression records
  `1/1/0/1/0`, meaning the selected yakumono DObj exists, is on, and the
  original predicate accepts the line.
- `STAGE_MPPLATFORM_TICK`: selected-callback platform status/update-tic
  result, safe result, proof mask, deferred mask, and advance count. Modes
  `145/146` remain regression coverage and expect `0x46505450`,
  `0x46505453`, mask low bits `0xff`, deferred mask `0xff`, and count `1`.
- `STAGE_MPPLATFORM_TICK_STEP`: tick proof prepared flag, base active proof
  seen flag, `mpCollisionSetYakumonoOnID` count, guarded
  `mpCollisionAdvanceUpdateTic` count, unsafe count, update tic before/after,
  DObj-present flag, DObj status before/after, and platform predicate after.
  Current proof expects `1/1/1/1/0`, tic `0 -> 1`, DObj present `1`, status
  `1 -> 1`, and predicate `1`.
- `STAGE_MPPASS_INPUT`: natural drop-through input result, safe result, proof
  mask, deferred mask, and selected fighter count. Modes `147/148` remain the
  regression proof and expect `0x46504950`, `0x46504953`, mask
  low bits `0x7ff`, deferred mask `0x7ff`, and count `1`.
- `STAGE_MPPASS_INPUT_SETUP`: pass-input prepared flag, base platform-tick
  proof seen flag, original pass-input check call/success counts, Squat
  status-set count, installed Squat proc-interrupt call count, Squat
  countdown-to-Pass count, Pass status-set count, `mpCommonSetFighterAir`
  count, air-velocity clamp count, and unsafe count. Current proof expects
  `1/1/1/1/1/3/1/1/1/1/0`.
- `STAGE_MPPASS_INPUT_STATE`: selected pass-through line, raw line flags,
  seeded stick Y, tap Y before/after, Wait/Squat/Pass status IDs, final
  ground-air state, final ignored line, and pass wait before/after. Current
  proof records Dream Land line `0`, flags `0x4000`, stick Y `<= -53`, tap Y
  `0 -> 254`, status `10 -> 28 -> 33`, ground-air `1`, ignored line `0`, and
  pass wait `3 -> 0`.
- `STAGE_MPPASS_INPUT_SQUATRV`: bounded crouch-release companion proof using
  the same imported BattleShip Squat helpers. It records SquatWait set count,
  SquatWait interrupt tick count, SquatRv set count, SquatRv animation-end tick
  count, SquatWait/SquatRv/final status IDs, callback mask, and final
  ground-air state. Current proof expects `1/1/1/1`, status
  `29 -> 30 -> 10`, callback mask low bits `0xf`, and final ground state `0`.
- `STAGE_MPPLATFORM_POS`: source-order yakumono position result, safe result,
  proof mask, deferred mask, and set-position count. Modes `149/150` are
  maintained regression coverage and expect `0x4650504f`, `0x46505053`, mask
  low bits `0xff`, deferred mask `0xff`, and count `1`.
- `STAGE_MPPLATFORM_POS_SETUP`: prepared flag, base pass-input proof seen flag,
  `mpCollisionSetYakumonoPosID` count, unsafe count, selected line, yakumono id,
  DObj-present flag, status before/after, and platform predicate after. Current
  proof records Dream Land line `0`, yakumono `1`, DObj present `1`, status
  `1 -> 1`, predicate `1`, and unsafe count `0`.
- `STAGE_MPPLATFORM_POS_VEC`: DObj before target/after coordinates and
  `gMPCollisionSpeeds` delta in milli-units. Current proof records after
  matching target and speed delta `12000/-4000/2000`, matching original
  `mpCollisionSetYakumonoPosID` semantics.
- `STAGE_MPPLATFORM_SPEED`: source-order platform-speed reader result, safe
  result, proof mask, deferred mask, and `mpCollisionGetSpeedLineID` count.
  Modes `151/152` are maintained regression coverage and the inherited
  dependency for the current Inishie scale proof. They expect `0x46505350`,
  `0x46505353`, mask low bits `0x3fff`, deferred mask `0xff`, and count `1`.
- `STAGE_MPPLATFORM_SPEED_SETUP`: prepared flag, base position proof seen flag,
  speed-reader count, unsafe count, selected line, yakumono id, DObj status,
  and platform predicate. Current proof records Dream Land line `0`, yakumono
  `1`, status `1`, predicate `1`, and unsafe count `0`.
- `STAGE_MPPLATFORM_SPEED_VEC`: expected speed from the previous position
  proof and the value read through `mpCollisionGetSpeedLineID`, in milli-units.
  Current proof records `12000/-4000/2000` for both vectors.
- `STAGE_MPPLATFORM_SPEED_DYNAMIC`: dynamic floor diff branch count, controlled
  probe count, hit count, line ID, yakumono ID, speed X/Y, and hit X/Y in
  milli-units. Current proof records one controlled probe on Dream Land line
  `0` / yakumono `1`, speed `12000/-4000`, and summary `dyn=1/2`.
- `STAGE_MPPLATFORM_SPEED_DYNAMIC_CEIL`: dynamic ceiling diff probe count, hit
  count, selected ceiling line, yakumono ID, and hit X/Y in milli-units.
  Current proof records one same-yakumono Dream Land ceiling probe and summary
  `ceil=1/1`.
- `STAGE_MPPLATFORM_SPEED_DYNAMIC_WALL`: dynamic wall diff probe count, hit
  count, selected wall line, yakumono ID, wall kind, and hit X/Y in milli-units.
  Current proof records one same-yakumono Dream Land wall probe through the
  project-owned `mpCollisionCheckLWallLineCollisionDiff` / `RWall` seam and
  summary `wall=1/1`.
- `STAGE_MPPLATFORM_SPEED_PROCESS_WALL`: bounded MP process wall probe count,
  hit count, selected wall line, wall kind, current wall mask, and multi-wall
  collision count. Current proof records one first-probe slice through
  `mpProcessCheckTestL/RWallCollisionAdjNew` with summary `procwall=1/1`.
- `STAGE_MPPLATFORM_SPEED_ANIM`: bounded yakumono animation playback count, MP
  update tic before/after, DObj status before/after, and speed X/Y/Z in
  milli-units. Current proof records one gated `mpCollisionPlayYakumonoAnim`
  call through original object-animation helpers, tic `+1`, status `2 -> 2`,
  speed `12000/-4000/2000`, and summary `anim=1`.
- `STAGE_MPPLATFORM_SPEED_BOUNDS`: post-animation MP bounds recompute count
  plus current-vs-start diff top/bottom/right/left in milli-units. Current
  proof calls `mpCollisionUpdateBoundsCurrent` and `mpCollisionUpdateBoundsDiff`
  after the gated yakumono animation tick and records summary `bounds=1` with
  at least one nonzero diff field.
- `STAGE_MPPLATFORM_SPEED_STAGE_ANIM`: stage-authored layer-animation
  diagnostic mask, DObj animation count, MObj/material animation count, and
  layer callback count. Current proof records `stageanim=0/0x91`: the Dream
  Land layer GObj/root and selected yakumono parent are present and the bounded
  callback ran once, but there is no authored Pupupu layer-1 animation table or
  `mpCollisionPlayYakumonoAnim` process on that layer. This marker is a precise
  blocker diagnostic, not an extra pass-mask bit; the platform-speed proof mask
  remains `0x3fff`.
- `STAGE_INISHIE_SCALE`: bounded Inishie/Mushroom Kingdom scale update result,
  safe result, proof mask, deferred mask, and set-position count. Modes
  `153/154` are regression coverage and expect `0x46495350`,
  `0x46495353`, mask low bits `0x7ff`, deferred mask `0xff`, and count `4`.
- `STAGE_INISHIE_SCALE_SETUP`: prepared flag, base platform-speed proof seen
  flag, shell-create count, update count, `mpCollisionSetYakumonoPosID` count,
  `mpCollisionSetYakumonoOnID` count, unsafe count, DObj mask, line mask,
  set-position mask, and set-on mask. Current proof records
  `1/1/1/2/4/2/0/0xf/0x3/0x3/0x3`.
- `STAGE_INISHIE_SCALE_LINES`: left/right line IDs, left/right map object
  kinds, and scale status before/after. Current proof records line groups
  `1/2`, map object kinds `5/6`, and status `0 -> 0`.
- `STAGE_INISHIE_SCALE_ALT`: altitude before/after, left/right base Y,
  left/right Y before, left/right Y after, and left/right speed Y in
  milli-units. Current proof records altitude `80000 -> 64000`, Y
  `363000/362000 -> 427000/298000`, and second-tick speeds `-8000/8000`.
- `STAGE_INISHIE_SCALE_FALL`: forced threshold diagnostic. Current proof
  records four fall-phase position writes, two sparkle calls, status `1 -> 2`,
  altitude after Wait `1100000`, Fall/Sleep acceleration `0`, final Fall Y
  `1460000/-741000`, and speed `-3000/-3000`.
- `STAGE_INISHIE_SCALE_STEP`: forced Sleep/Retract diagnostic. Current proof
  records four retract-phase position writes, position mask `0x3`, two
  platform re-enable calls, re-enable mask `0x3`, wait `1 -> 0`, status
  `3 -> 0`, retracted altitude `0`, restored platform Y `363000/362000`, and
  final retract speeds `-1097000/1103000`.
- `STAGE_MPSTALE_FLOOR`: source-order MP stale-valid second-floor result,
  safe result, proof mask, deferred mask, and selected fighter count. Current
  pass values are `0x46535450`, `0x46535453`, mask `0xffff`, deferred mask
  `0xff`, and count `2`.
- `STAGE_MPSTALE_FLOOR_SETUP`: prepared flag, base MPWall proof seen flag,
  source-pair search attempt/hit/miss counts, stale line ID, target line ID,
  target X/Y in milli-units, and unsafe count. Current proof records stale
  floor line `1`, target floor line `0`, target position `-285000/1542000`,
  at least one hit, zero local search misses, and unsafe count `0`.
- `STAGE_MPSTALE_FLOOR_LIVE`: source-order second-floor call/hit/miss counts,
  accepted-new-line count, landing-floor count, floor-edge-adjust count, and
  collision-end-clear count for the stale proof. Current proof records one
  accepted valid-stale second-floor path through the finalizer-local
  `MPCollData` probe. The local valid-stale path is a hit path; miss evidence
  is still supplied by the maintained `STAGE_MPSWEEP_FLOOR_SECOND` no-hit
  marker.
- `STAGE_MPSTALE_FLOOR_P0P1`: stale-proof P0 final line ID, retained P1 final
  line ID, P0 target-line-match count, and P0/P1 final floor-OK flags. Current
  proof records P0 on target line `0` and retained live P1 on line `3`.
- `STAGE_MPLIVESTALE_FLOOR`: selected-callback live-stale second-floor result,
  safe result, proof mask, deferred mask, and selected fighter count. Current
  pass values are `0x464c5350`, `0x464c5353`, mask `0xffff`, deferred mask
  `0xff`, and count `2`.
- `STAGE_MPLIVESTALE_FLOOR_SETUP`: prepared flag, base stale proof seen flag,
  source-pair search attempt/hit/miss counts, stale line ID, target line ID,
  target X/Y in milli-units, selected-callback probe count, and unsafe count.
  Current proof records stale floor line `1`, target floor line `0`, target
  position `-285000/1542000`, one selected P0 callback-local probe, zero search
  misses, and unsafe count `0`.
- `STAGE_MPLIVESTALE_FLOOR_LIVE`: selected-callback local source-order
  second-floor call/hit/miss counts, accepted-new-line count, landing-floor
  count, floor-edge-adjust count, and collision-end-clear count. The proof
  runs a contained local `MPCollData` pass through `mpProcessUpdateMain`,
  `mpCommonRunFighterAllCollisions`, and `mpProcessCheckTestFloorCollision`
  so the accepted stale pair is proven without moving the real Mario/Fox roots.
- `STAGE_MPLIVESTALE_FLOOR_P0P1`: live-stale P0 final line ID, retained P1
  final line ID, P0 target-line-match count, and P0/P1 final floor-OK flags.
  Current proof records P0 on target line `0` and retained live P1 on line `3`.
- `STAGE_MPMOTIONSTALE_FLOOR`: selected-callback/root motion-stale
  second-floor result, safe result, proof mask, deferred mask, and selected
  fighter count. Current pass values are `0x464d5350`, `0x464d5353`, mask
  `0xffff`, deferred mask `0xff`, and count `2`.
- `STAGE_MPMOTIONSTALE_FLOOR_SETUP`: prepared flag, base live-stale proof seen
  flag, source-pair search attempt/hit/miss counts, stale line ID, target line
  ID, target X/Y in milli-units, selected P0 mutation count, source-order
  update hit count, target-line-match count, and unsafe count. Current proof
  records stale floor line `1`, target floor line `0`, target position
  `-285000/1542000`, one mutation, one update hit, one target match, and
  unsafe count `0`.
- `STAGE_MPMOTIONSTALE_FLOOR_POS`: P0 previous X/Y, target X/Y, and final X/Y
  in milli-units. Current proof records the source-order copyback
  `-293000/1574000 -> -285000/1542000`.
- `STAGE_MPMOTIONSTALE_FLOOR_P0P1`: motion-stale P0 final line ID, retained P1
  final line ID, and P0/P1 final floor-OK flags. Current proof records P0 on
  target line `0` and P1 on line `3`.
- `STAGE_MPCLIFFSTATUS_FLOOR`: source-order MP cliff-status floor result, safe
  result, proof mask, deferred mask, and selected fighter count. Current pass
  values are `0x46435350`, `0x46435353`, mask `0x3ff`, deferred mask `0xff`,
  and count `2`.
- `STAGE_MPCLIFFSTATUS_FLOOR_SETUP`: prepared flag, base motion-stale proof
  seen flag, `mpCommonProcFighterOnCliffEdge` probe count, false/true
  `mpCommonCheckFighterOnCliffEdge` counts, floor-edge branch count, Fall
  branch count, and unsafe count. Current proof records two false cliff-edge
  checks, one Ottotto branch, one Fall branch, and zero unsafe count.
- `STAGE_MPCLIFFSTATUS_FLOOR_STATUS`: Ottotto status call count, Fall status
  call count, accepted status-set count, and air-state set count. Current proof
  records `1/1/2/1`.
- `STAGE_MPCLIFFSTATUS_FLOOR_P0P1`: P0 status, motion, ground/air state, final
  line, and floor-edge mask, followed by P1 status, motion, ground/air state,
  final line, and floor-edge mask. Current proof records P0 Ottotto
  `36/30/0` with a nonzero floor-edge mask and P1 Fall `26/20/1` with no
  floor-edge mask.
- `STAGE_MPCLIFFTICK_FLOOR`: source-order MP cliff-tick floor result, safe
  result, proof mask, deferred mask, and selected fighter count. Current pass
  values are `0x46435450`, `0x46435453`, mask `0xfff`, deferred mask `0xff`,
  and count `2`.
- `STAGE_MPCLIFFTICK_FLOOR_CALLS`: prepared flag, base cliff-status proof seen
  flag, Ottotto update/interrupt/map call counts, Ottotto anim-end check
  count, Ottotto floor check/hit counts, Fall interrupt count, and Fall
  special-air/attack-air/jump-aerial interrupt check counts. Current proof
  records `1` for every call/check field.
- `STAGE_MPCLIFFTICK_FLOOR_STATUS`: P0 before/after status, motion,
  ground/air state, and final line, followed by P1 before/after status,
  motion, ground/air state, and final line, then guarded status-set fallback,
  OttottoWait fallback, and unsafe counts. Current proof records P0
  `36/30/0 -> 36/30/0` on line `0`, P1 `26/20/1 -> 26/20/1` on line `3`,
  and all fallback/unsafe counts as `0`.
- `STAGE_MPFALLMAP_FLOOR`: source-order MP Fall-map floor result, safe result,
  proof mask, deferred mask, and selected fighter count. Current pass values
  are `0x46464d50`, `0x46464d53`, mask `0x3ff`, deferred mask `0xff`, and
  count `1`.
- `STAGE_MPFALLMAP_FLOOR_CALLS`: prepared flag, base cliff-tick proof seen
  flag, Fall physics callback count, fast-fall check count, gravity call count,
  air-drift call count, air-friction call count, bounded integration count,
  Fall map callback count, and guarded no-collision map result count. Current
  proof records `1` for every field.
- `STAGE_MPFALLMAP_FLOOR_STATUS`: P1 before/after status, motion, ground/air
  state, line before/after, root-Y before/after in milli-units, velocity-Y
  before/after in milli-units, and unsafe count. Current proof records P1
  Fall `26/20/1 -> 26/20/1` on line `3`, root-Y `200000 -> 194000`, a more
  negative Y velocity, and unsafe count `0`.
- `STAGE_MPFALLLAND_FLOOR`: source-order MP Fall-landing floor result, safe
  result, proof mask, deferred mask, and selected fighter count. Current pass
  values are `0x464c4e50`, `0x464c4e53`, mask `0x7ff`, deferred mask `0xff`,
  and count `1`.
- `STAGE_MPFALLLAND_FLOOR_CALLS`: prepared flag, base Fall-map proof seen
  flag, Fall physics callback count, fast-fall check count, gravity call
  count, air-drift call count, air-friction call count, bounded integration
  count, Fall map callback count, floor-collision count,
  `mpProcessSetLandingFloor` count, landing/wait branch count,
  `ftCommonLandingSetStatus` count, optional
  `ftCommonLandingSetStatusParam` wrapper count, and `ftMainSetStatus` count.
  Current proof records `1` for every required source boundary and `0` for the
  optional param-wrapper count because the imported route can reach the base
  landing status path directly.
- `STAGE_MPFALLLAND_FLOOR_STATUS`: P1 before/after status, motion, ground/air
  state, line before/after, floor/root-Y before/after in milli-units,
  velocity-Y before/after in milli-units, `mpCommonSetFighterGround` count,
  unsafe count, and landing/wait branch count. Current proof records P1
  Fall/Air `26/20/1 -> 31/25/0`, line `3 -> 3`, root-Y `4000 -> 0`,
  velocity-Y `-8000 -> 0`, set-ground `1`, and unsafe count `0`.
- `STAGE_MPCEIL_FLOOR`: source-order MP ceiling-floor result, safe result,
  proof mask, deferred mask, and selected fighter count. Current pass values
  are `0x46434550`, `0x46434553`, mask `0x7f`, deferred mask `0xff`, and
  count `1`.
- `STAGE_MPCEIL_FLOOR_SETUP`: prepared flag, base Fall-landing proof seen
  flag, decoded ceiling line count, selected ceiling line ID, selected line
  kind, and unsafe count. Current proof selects real Pupupu ceiling line `4`,
  kind `1`, and unsafe count `0`.
- `STAGE_MPCEIL_FLOOR_CHECK`: ceiling test call/hit/miss counts,
  same-line ceiling sweep call/hit/miss counts, and different-line ceiling
  sweep call/hit/miss counts. Current proof records one test hit and one
  different-line sweep hit.
- `STAGE_MPCEIL_FLOOR_QUERY`: ceiling sweep visit/candidate counts,
  `mpCollisionGetFCCommonCeil` call/hit/miss counts,
  `mpProcessRunCeilCollisionAdjNew` count, and current/stat ceiling mask
  counters. Current proof records nonzero visits/candidates, two
  `FCCommonCeil` hits, one adjust call, and one current/stat ceiling marker.
- `STAGE_MPCEIL_FLOOR_POS`: root-Y before check, after check, and after
  adjust, previous/target top-Y, selected ceiling Y, and ceiling distance in
  milli-units. Current proof records root-Y `-1464000 -> -1472000`, target
  top crossing above ceiling Y `-1072000`, and distance `-8000`.
- `STAGE_MPCEILSTATUS_FLOOR`: ceiling-status result, safe result, proof mask,
  deferred mask, and selected fighter count. Current pass values are
  `0x46435350`, `0x46435353`, mask `0x1ff`, deferred mask `0xff`, and count
  `1`.
- `STAGE_MPCEILSTATUS_FLOOR_CALLS`: prepared flag, base ceiling-floor proof
  seen flag, selected map callback count, ceiling-heavy check count,
  special-collision callback count, ceiling collision count, ceiling adjust
  count, ceiling-heavy mask count, `ftCommonStopCeilSetStatus` count, and
  unsafe count. Current proof records `1` for each required source boundary
  and unsafe count `0`.
- `STAGE_MPCEILSTATUS_FLOOR_STATUS`: selected ceiling line ID, selected line
  kind, before/after status, before/after motion, before/after ground-air
  state, guarded `ftMainSetStatus` count, and animation-event callback count.
  Current proof records real Pupupu ceiling line `4`, kind `1`, Fall/Air
  `26/20/1 -> 66/57/0`, and one guarded status/anim callback.
- `STAGE_MPCEILSTATUS_FLOOR_POS`: root-Y before/after, vertical velocity
  before/after, current collision mask, and status collision mask. Current
  proof records root-Y adjustment `-1464000 -> -1472000`, vertical velocity
  `40000 -> 0`, current mask `0x4400`, and status mask `0x400`.
- `STAGE_MPCLIFFCATCH_FLOOR`: cliff-catch result, safe result, proof mask,
  deferred mask, and selected fighter count. Current pass values are
  `0x46434350`, `0x46434353`, mask `0xfff`, deferred mask `0xff`, and count
  `1`.
- `STAGE_MPCLIFFCATCH_FLOOR_CALLS`: prepared flag, base ceiling-status proof
  seen flag, selected map callback count, ceiling-heavy check count,
  special-collision callback count, left/right cliff-test counts, left/right
  cliff-hit counts, landing-param count, `ftCommonCliffCatchSetStatus` count,
  `ftMainSetStatus` count, animation-event callback count, stop-velocity
  count, and unsafe count. Current proof records two selected map/collision
  passes: one right-cliff hit that reaches the original CliffCatch status path,
  and one same-cliff/same-LR occupancy block that does not call status setup
  again. Landing/status remain one call each, and unsafe count remains `0`.
- `STAGE_MPCLIFFCATCH_FLOOR_EFFECTS`: guarded flash-effect and capture-immune
  counts reached by the imported CliffCatch status path. Current proof records
  `1/1`.
- `STAGE_MPCLIFFCATCH_FLOOR_STATUS`: selected line, selected side, before/after
  status, before/after motion, before/after ground-air state, cliff-hold flag,
  stop-velocity count, physics-stop count, copied cliff ID, and LR before.
  Current proof records real Pupupu line `3`, right side `1`, Fall/Air
  `26/20/1 -> 84/72/1`, cliff hold `1`, `cliff_id=3`, and LR `-1`.
- `STAGE_MPCLIFFCATCH_FLOOR_POS`: selected ledge X/Y, root X/Y before and
  after, current collision mask, and status collision mask. Current proof
  records ledge X `2318000`, root placement
  `2518000,-408000 -> 2318000,-408000`, and right-cliff masks
  `0x2000/0x2000`.
- `STAGE_MPCLIFFCATCH_FLOOR_OCC`: occupancy probe count, occupancy block
  count, holder/probe cliff IDs, holder/probe LR, probe status/motion/ground-air
  after the blocked attempt, probe cliff-hold flag after the blocked attempt,
  CliffCatch status-set delta, and landing-param delta. Current proof records
  one probe, one block, holder/probe line `3/3`, holder/probe LR `-1/-1`,
  probe remains Fall/Air `26/20/1`, probe cliff-hold remains `0`, and both
  status-set and landing-param deltas remain `0`.
- `STAGE_MPCLIFFWAIT_FLOOR`: CliffWait result, safe result, proof mask,
  deferred mask, and selected fighter count. Current pass values are
  `0x46435750`, `0x46435753`, mask `0x7ff`, deferred mask `0xff`, and count
  `1`.
- `STAGE_MPCLIFFWAIT_FLOOR_CALLS`: prepared flag, base CliffCatch proof seen
  flag, original CliffCatch update count, anim-end check count, anim-end set
  status count, `ftCommonCliffWaitSetStatus` count, guarded `ftMainSetStatus`
  count, player-tag wait count, capture-immune count, CliffWait interrupt
  count, attack check count, escape check count, climb-or-fall check count,
  and unsafe count. Current proof records one call for every required
  source-order boundary and unsafe count `0`.
- `STAGE_MPCLIFFWAIT_FLOOR_STATUS`: before/update/interrupt status, motion,
  and ground-air state; cliff-hold and allow-interrupt flags; retained cliff
  ID and LR; fall-wait after update, before interrupt, and after interrupt;
  player-tag wait, capture mask, proc-damage setup, damage-fall call count,
  and previous CliffCatch cliff ID. Current proof records CliffCatch/Air
  `84/72/1 ->` CliffWait/Ground `85/73/0`, retained `cliff_id=3`, LR `-1`,
  player-tag wait `120`, nonzero capture mask, proc-damage setup, fall-wait
  `1080 -> 1079`, and no damage-fall call.
- `STAGE_MPCLIFFWAIT_DAMAGE`: CliffWait timeout/DamageFall result, safe
  result, proof mask, deferred mask, and selected fighter count. Current pass
  values are `0x46445750`, `0x46445753`, mask `0x1ffffff`, deferred mask
  `0xff`, and count `1`.
- `STAGE_MPCLIFFWAIT_DAMAGE_CALLS`: prepared flag, base CliffWait proof seen
  flag, original CliffWait interrupt call count, original attack check count,
  original escape check count, original climb/fall check count,
  `ftCommonDamageFallSetStatusFromCliffWait` call count, guarded
  `ftMainSetStatus` count, air-velocity clamp/rumble seam count,
  collision-default placement count, and unsafe count. Current proof records
  imported-original `ftCommonDamageFallSetStatusFromCliffWait` reaching the
  bounded DamageFall status install and imported clamp/rumble tail, one call
  for every required source-order boundary, and unsafe count `0`.
- `STAGE_MPCLIFFWAIT_DAMAGE_STATUS`: before/after status, motion, and
  ground-air state. Current proof records CliffWait/Ground `85/73/0 ->`
  DamageFall/Air `57/50/1`.
- `STAGE_MPCLIFFWAIT_DAMAGE_FLAGS`: fall-wait before/after, cliffcatch wait
  after, cliff ID before/after, floor line after, cliff-hold before/after,
  proc-damage set flag after DamageFall setup, `tics_since_last_z` after, and
  installed callback flag. Current proof
  records fall-wait `1 -> 0`, `cliffcatch_wait=30`, retained
  `cliff_id=3`/`floor_line_id=3`, cliff hold `1 -> 0`, `proc_damage=0`, and
  `tics_since_last_z=65536`.
- `STAGE_MPCLIFFWAIT_DAMAGE_POS`: root X/Y before, target X/Y, and root X/Y
  after the status placement seam. Current proof records the root moving to
  the deterministic cliff-damage placement target.
- `STAGE_MPCLIFFWAIT_DAMAGE_TICK`: original DamageFall interrupt, special-air,
  aerial-attack, aerial-jump, HammerFall, physics, map, cliff-check,
  no-collision, passive-stand, passive, and down-bounce counts, followed by
  DamageFall velocity-Y before/after and status/motion/ground-air after the
  tick. Current proof records one interrupt and one physics tick, five guarded
  map/cliff-check calls, one no-collision branch, four positive collision
  branches, all four interrupt subchecks, three passive-stand checks, two
  passive checks, one down-bounce setup path, first-tick status/motion/GA still
  `57/50/1`, and velocity-Y `0 -> -4000`.
- `STAGE_MPCLIFFWAIT_DAMAGE_COLLISION`: positive DamageFall map-collision
  DownBounce branch diagnostics. Current proof records four collision hits
  across the CliffCatch, PassiveStand, Passive, and DownBounce positive passes,
  one bounded
  DownBounce `ftMainSetStatus` call selected through BattleShip's original
  DownBounceD/U orientation formula, one ground placement, one
  imported-original DownBounce update-effects tail producing one ImpactWave
  effect, one FGM, one rumble, one velocity-transfer seam, final
  DownBounceU/Ground `68/59/0`, attack buffer `0`, `damage_mul=500` milli,
  installed callback flag `1`, effect kind `22`, nonzero FGM, and rumble ID
  `4`.
- `STAGE_MPCLIFFWAIT_DAMAGE_CLIFFCATCH`: positive DamageFall cliff-collision
  branch diagnostics. Current proof records one original
  `ftCommonCliffCatchSetStatus` path through the bounded `ftMainSetStatus`
  seam, one ground/air placement pair, one animation-event call, one velocity
  stop, one ledge flash effect, one capture-immune setup, final
  CliffCatch/Air `84/72/1`, cliff hold restored to `1`, proc-damage and
  CliffCatch/Common callbacks installed, retained `cliff_id=3`,
  `floor_line_id=-1`, right-cliff masks `0x2000`, and capture mask `0x4`.
- `STAGE_MPCLIFFWAIT_DAMAGE_PASSIVESTAND`: positive DamageFall floor-collision
  PassiveStand branch diagnostics. Current proof records one bounded
  PassiveStand `ftMainSetStatus` call, one original ground-placement call, one
  velocity-transfer seam, PassiveStandF/Ground `73/62/0`, seeded stick X `-20`
  with LR `-1`, and installed `ftAnimEndSetWait`,
  `ftPhysicsApplyGroundVelTransN`, and edge-break map callbacks.
- `STAGE_MPCLIFFWAIT_DAMAGE_PASSIVESTAND_TICK`: guarded PassiveStand
  animation-end update handoff diagnostics. Current proof records one
  `ftAnimEndSetWait` tick, one bounded Wait `ftMainSetStatus` call, one
  player-tag wait call, PassiveStandF/Ground `73/62/0 ->` Wait/Ground
  `10/4/0`, `playertag_wait=120`, and valid Wait callbacks.
- `STAGE_MPCLIFFWAIT_DAMAGE_PASSIVESTAND_CALLBACKS`: guarded PassiveStand
  physics/map callback diagnostics before the animation-end handoff. Current
  proof records one `ftPhysicsApplyGroundVelTransN` tick, one
  `mpCommonSetFighterFallOnEdgeBreak` tick, retained PassiveStandF/Ground
  `73/62/0`, and unchanged PassiveStand callbacks.
- `STAGE_MPCLIFFWAIT_DAMAGE_PASSIVE`: positive DamageFall floor-collision
  Passive branch diagnostics. Current proof records one bounded Passive
  `ftMainSetStatus` call, one original ground-placement call, one
  velocity-transfer seam, Passive/Ground `81/70/0`, and installed
  `ftAnimEndSetWait`, `ftPhysicsApplyGroundVelFriction`, and ground-break map
  callbacks.
- `STAGE_MPCLIFFWAIT_DAMAGE_PASSIVE_TICK`: guarded Passive animation-end
  update handoff diagnostics. Current proof records one `ftAnimEndSetWait`
  tick, one bounded Wait `ftMainSetStatus` call, one player-tag wait call,
  Passive/Ground `81/70/0 ->` Wait/Ground `10/4/0`, `playertag_wait=120`,
  and valid Wait callbacks.
- `STAGE_MPCLIFFWAIT_DAMAGE_PASSIVE_CALLBACKS`: guarded Passive physics/map
  callback diagnostics before the animation-end handoff. Current proof records
  one bounded `ftPhysicsApplyGroundVelFriction` tick, one
  `mpCommonSetFighterFallOnGroundBreak` tick, retained Passive/Ground
  `81/70/0`, and unchanged Passive callbacks.
- `STAGE_MPPASSIVE`: direct/menu Passive-loop result, safe result, proof mask,
  deferred mask, and selected fighter count. Current pass values are
  `0x46505350`, `0x46505353`, mask `0x1ffff`, deferred mask `0xff`, and count
  `1` for the recover-loop modes.
- `STAGE_MPPASSIVE_BRANCH`: recover-loop-only passive input branch matrix.
  Current modes `155/156` require mask `0x7f`: bit 0 proves buffered-Z
  PassiveStandF, bit 1 proves buffered-Z PassiveStandB, bit 2 proves neutral
  stick does not enter PassiveStand, bit 3 proves expired-Z PassiveStand does
  not transition, bit 4 proves buffered-Z Passive, bit 5 proves expired-Z
  Passive does not transition, and bit 6 proves the diagnostic branch seam
  cleaned up.
- `STAGE_MPPASSIVE_PASSIVESTANDB`: recover-loop-only PassiveStandB runtime
  and final handoff mask. Current modes `155/156` require mask `0xf`: bit 0
  proves original PassiveStandB/Ground setup from the buffered-Z branch, bit 1
  proves five guarded stable update-then-physics/map frames, bit 2 proves the
  original `ftAnimEndSetWait` handoff into Wait/Ground with valid Wait
  callbacks, and bit 3 proves the PassiveStandB guard cleaned up.
- Passive recover-loop stable frames run source-order update before
  physics/map, matching BattleShip's `ftMainProcUpdateInterrupt` process before
  `ftMainProcPhysicsMap`.
- `STAGE_MPPASSIVE_NATURALMAP`: recover-loop-only imported DamageFall map
  callback proof. Current modes `155/156` require mask `0x7f`: bit 0 proves
  imported original `ftCommonDamageFallProcMap` selects PassiveStandF/Ground,
  bit 1 proves the same map callback selects Passive/Ground when PassiveStand
  does not take the input, bit 2 proves the map-callback guard cleaned up,
  bit 3 proves the PassiveStandF branch used the bounded source-order MP floor
  collision path, bit 4 proves the Passive branch used that same floor
  collision path, and bits 5-6 prove both branches entered through the
  installed `FTStruct.proc_map == ftCommonDamageFallProcMap` callback before
  the wrapper dispatched to the imported original base. Missing callback slots
  are unsafe; the proof no longer falls back to a direct base call. The
  companion `STAGE_MPPASSIVE_NATURALMAP_CALLS` marker must be `2` in current
  modes `155/156`.
- `STAGE_MPPASSIVE_APPEAL`: recover-loop-only bounded Appeal/Taunt callback
  proof. Current modes `155/156` require mask `0x7f`: bit 0 proves the
  imported original `ftCommonAppealCheckInterruptCommon` accepted the seeded
  L-button tap, bit 1 proves Appeal/Ground status/motion `189/164`, bit 2
  proves the expected callback shape, bit 3 proves the bounded `ftMainSetStatus`
  seam ran once, bit 4 proves the Appeal guard cleaned up, bit 5 proves the
  installed original Appeal interrupt callback was invoked with no catch/guard
  branch while staying in Appeal/Ground, and bit 6 proves the installed
  `ftAnimEndSetWait` update slot returns to Wait/Ground. The marker also
  records check count, set-status count, status, motion, ground-air, callback
  flag, button tap, and L-mask values. Current recover modes expect the global
  check and set-status counts to be `2` because the companion GuardOn branch
  proof re-enters Appeal through the same imported original gate.
- `STAGE_MPPASSIVE_APPEAL_GUARD`: recover-loop-only bounded source-order
  Appeal/Taunt catch-fails-then-GuardOn branch proof. Current modes `155/156`
  require mask `0x7f`: bit 0 proves the proof re-entered Appeal/Ground through
  imported original `ftCommonAppealCheckInterruptCommon`, bit 1 proves the
  original Appeal interrupt called the project-owned
  `ftCommonCatchCheckInterruptCommon` seam and it returned false, bit 2 proves
  the imported original GuardOn check and bounded GuardOn status seam both ran
  once, bit 3 proves GuardOn/Ground status/motion `152/134`, bit 4 proves the
  installed GuardOn callback shape, bit 5 proves core shield state was
  initialized while Z was held, and bit 6 proves the Appeal GuardOn active flag
  cleaned up. The marker also records catch, guard-check, set-status counts,
  status, motion, callback flag, shield flag, Z-hold, and Z-mask values.
- `STAGE_MPPASSIVE_CATCH`: recover-loop-only bounded original Catch status
  and callback-handoff proof. Current modes `155/156` require mask `0x7ff`:
  bit 0 proves a
  Wait/Ground Z-hold plus A-tap entered imported original
  `ftCommonCatchCheckInterruptCommon`, bit 1 proves the original check
  accepted the input, bit 2 proves bounded `ftCommonCatchSetStatus` ran once,
  bit 3 proves Catch/Ground status/motion `166/146`, bit 4 proves the expected
  Catch callback shape, bit 5 proves `ftParamSetCatchParams` populated the
  catch/capture callback parameters, bit 6 proves the active guard cleaned up,
  bit 7 proves the installed Catch map/update callback handoff also passed,
  bit 8 proves the bounded CatchPull/CatchWait marker passed, bit 9 proves
  the bounded CapturePulled/CaptureWait marker passed, and bit 10 proves the
  bounded ThrowF/ThrownCommon marker passed. The marker also records the one
  item-throw check returning false, zero light-throw status calls, and zero
  CatchPull/CapturePulled deferred callback calls for the initial Catch setup
  slice.
- `STAGE_MPPASSIVE_CATCH_CALLBACKS`: companion marker for the bounded Catch
  callback-handoff proof. Current modes `155/156` require callback mask
  `0x7f`: bit 0 proves one installed original Catch map tick ran, bit 1
  proves the map tick reached the bounded `mpCommonCheckFighterOnEdge` seam
  once, bit 2 proves the map callback preserved Catch/Ground `166/146`, bit 3
  proves one installed original Catch update tick ran, bit 4 proves that
  update tick reached Wait/Ground `10/4`, bit 5 proves the expected Wait
  callback shape after the handoff, and bit 6 proves callback active guards
  cleaned up. The marker also records map/update tick counts and the
  status/motion/ground-air values after map and update.
- `STAGE_MPPASSIVE_CATCH_PULL`: recover-loop-only bounded original CatchPull
  to CatchWait proof. Current modes `155/156` require mask `0xff`: bit 0
  proves a seeded target/search GObj was available, bit 1 proves original
  `ftCommonCatchPullProcCatch` ran and installed CatchPull/Ground
  status/motion `167/147`, bit 2 proves `catch_gobj` was wired to the seeded
  target, bit 3 proves the capture-immune, catch-swirl, and rumble seams ran
  with counts `2/1/1`, bit 4 proves original CatchPull update reached
  CatchWait/Ground `168/-2`, bit 5 proves the target capture flag was set,
  bit 6 proves one CatchWait interrupt tick called the throw-check seam and
  decremented `throw_wait` `60->59`, and bit 7 proves the active guards
  cleaned up. The marker also records proc-catch, set-status, update,
  interrupt, throw-check, effect/rumble, status/motion/ground-air, target, and
  unsafe counters.
- `STAGE_MPPASSIVE_CAPTURE`: recover-loop-only bounded original
  CapturePulled to CaptureWait proof. Current modes `155/156` require mask
  `0x3f`: bit 0 proves original `ftCommonCapturePulledProcCapture` ran and
  installed CapturePulled/Ground status/motion `171/150`, bit 1 proves the
  victim captured the seeded fighter GObj and flipped LR, bit 2 proves voice
  stop, velocity stop, and capture-immune seams ran with counts `1/1/2`, bit 3
  proves one original CapturePulled physics tick reached CaptureWait/Ground
  `172/-2`, bit 4 proves one installed CaptureWait map tick preserved ground
  state, and bit 5 proves the capture active guards cleaned up. The marker also
  records proc-capture, the seeded `proc_damage` callback count, set-status,
  physics/map tick counts, status/motion/ground-air values, victim LR,
  jumps-used, root position in milli-units, and unsafe count. Current proof
  requires `proc_damage=1`, proving `ftParamStopVoiceRunProcDamage` ran the
  BattleShip damage-callback tail before CapturePulled status setup.
- `STAGE_MPPASSIVE_THROW`: recover-loop-only bounded original ThrowF/ThrowB
  to ThrownCommon handoff proof. Current modes `155/156` require mask
  `0x3ff`:
  bit 0 proves original `ftCommonThrowCheckInterruptCatchWait` accepted the
  seeded throw input, bit 1 proves bounded `ftCommonThrowSetStatus` installed
  ThrowF/Ground status/motion `169/148`, bit 2 proves the ThrowF callback
  shape was installed, bit 3 proves the seeded victim reached ThrownCommon/Air
  status/motion `186/161` with one jump consumed, bit 4 proves the throw
  anim-event and capture-immune seams ran with counts `2/2`, bit 5 proves the
  target still points back at the catcher GObj, and bit 6 proves the active
  guard cleaned up. Bit 7 proves a seeded stick-left CatchWait branch reaches
  ThrowB/Ground status/motion `170/149`, bit 8 proves the same seeded victim
  reaches ThrownCommon/Air from ThrowB, and bit 9 proves both original throw
  checks ran with total counts `2/2/2/4/4` for throw-check, catcher
  set-status, target set-status, animation-event, and capture-immune seams.
  The marker also records throw-check, catcher set-status,
  target set-status, anim-event, capture-immune, catcher/target status-motion/
  ground-air, callback, target jump, target capture-GObj, and unsafe counts.
  `STAGE_MPPASSIVE_THROW_B` records the B-branch result, catcher
  status/motion/ground-air/callback shape, and target status/motion/ground-air/
  jump/capture-GObj shape.
  The separate `STAGE_MPPASSIVE_THROW_CALLBACK` marker proves bounded
  ThrownCommon callback ticks; this marker is only the original status handoff
  proof.
- `STAGE_MPPASSIVE_THROW_CALLBACK`: recover-loop-only bounded original
  `ftCommonThrownProcUpdate`, `ftCommonThrownProcPhysics`, and
  `ftCommonThrownProcMap` callback proof imported from `ftcommonthrown1.c`.
  Current modes `155/156` require mask `0x1ff`: bit 0 proves the installed
  ThrownCommon update/physics/map callback slots point at the imported original
  callbacks, bit 1 proves one original update tick preserved ThrownCommon/Air
  status/motion/ground-air `186/161/1`, bit 2 proves one original physics tick
  ran, bit 3 proves one original map tick copied the captor floor line, bit 4
  proves the victim still points at the captor GObj, and bit 5 proves the
  active guard cleaned up with unsafe count zero. Bit 6 proves a second
  animation-end update tick reached original `ftCommonThrownSetStatusImmediate`
  and preserved ThrownCommon/Air `186/161/1`, bit 7 proves that immediate path
  reached the animation-event and capture-immune seams, and bit 8 proves the
  ThrownCommon callback slots remained installed after the immediate handoff.
  This is a bounded callback-reach, floor-copy, and animation-end branch proof,
  not continuous thrown animation, release scheduling, item throw handling, or
  player-driven grab gameplay.
- `STAGE_MPPASSIVE_THROW_UPDATE`: recover-loop-only bounded original
  `ftCommonThrowProcUpdate` `flag2` release proof imported from
  `ftcommonthrow.c`. Current modes `155/156` require mask `0x7f`: bit 0 proves
  the installed ThrowF callback and seeded capture relation are present, bit 1
  proves one original Throw update tick called the thrown update-stats release
  path once, bit 2 proves the catcher stayed ThrowF/Ground while clearing
  `catch_gobj`, `flag2`, and capture immunity, bit 3 proves the victim damage
  went `50->58` and capture was cleared while staying Air, bit 4 proves script
  ID `0` and the original `flag2 == 1` LR inversion, bit 5 proves the thrown
  proc-status callback was installed, and bit 6 proves the active guard cleaned
  up with unsafe count zero. This is a bounded callback-release proof, not
  continuous throw animation scheduling or player-driven grab gameplay.
- `STAGE_MPPASSIVE_THROW_RELEASE`: recover-loop-only bounded original
  `ftCommonThrownReleaseThrownUpdateStats` proof imported from
  `ftcommonthrown2.c`. Current modes `155/156` require mask `0xff`: bit 0
  proves the public release seam called imported original update-stats once,
  bit 1 proves `ftCommonDamageInitDamageVars` and 1P damage stats seams ran,
  bit 2 proves damage update, player battle stats, and stale queue seams ran,
  bit 3 proves three source-compatible rumble requests were made, including
  the damage-init rumble plus the two thrown-release rumbles, bit 4 proves the
  victim capture pointer was cleared and victim ground-air changed to Air,
  bit 5 proves original thrown proc-status was installed with script ID `123`,
  bit 6 proves damage went `10->18` from seeded throw damage `8`, and bit 7
  proves hitstatus normalized, knockback was positive, LR matched, the active
  guard cleaned up, and unsafe stayed zero. The marker records counts,
  damage-before/after, init damage, knockback milli (`6600000` in the current
  proof), LR, and script ID.
- `STAGE_MPPASSIVE_THROW_RELEASE_STATUS`: recover-loop-only bounded original
  `ftCommonThrownSetStatusDamageRelease`,
  `ftCommonThrownUpdateDamageStats`, and
  `ftCommonThrownSetStatusNoDamageRelease` proof imported from
  `ftcommonthrown2.c`. Current modes `155/156` require mask `0x1f`: bit 0
  proves damage release ran once and updates damage `20->26`, bit 1 proves it
  clears capture, normalizes hitstatus, and sets a nonzero release LR, bit 2
  proves update-damage-stats ran once and updates damage `30->36`, bit 3
  proves no-damage release ran once, keeps damage `40->40`, and queues zero
  damage, and bit 4 proves the active guard cleaned up with unsafe count zero.
  This is not continuous throw release/damage runtime: item throw branches,
  hitlag/full damage status runtime, stale queue internals, and continuous
  scheduling remain deferred.
- `STAGE_MPPASSIVE_THROW_PROC_STATUS`: recover-loop-only bounded original
  `ftCommonThrownProcStatus` callback proof imported from
  `ftcommonthrown2.c`. Current modes `155/156` require mask `0x3f`: bit 0
  proves the installed proc-status slot still points to the imported original
  callback, bit 1 proves the seeded capture GObj is ready, bit 2 proves one
  guarded proc-status tick was attempted, bit 3 proves original code reached
  the project-owned `ftParamSetThrowParams` seam and set `throw_gobj`, bit 4
  proves the original callback wrote script ID `123`, and bit 5 proves guard
  cleanup with unsafe count zero. This is still a bounded callback tick, not
  continuous thrown/damage status scheduling.
- `STAGE_MPPASSIVE_THROW_DEAD_RESULT`: recover-loop-only bounded original
  `ftCommonThrownDecideDeadResult` cleanup proof imported from
  `ftcommonthrown2.c`. Current modes `155/156` require mask `0x7f`: bit 0
  proves the guarded original dead-result call ran once, bit 1 proves the
  original lose-grip/release path reached `mpCommonRunFighterCollisionDefault`,
  bit 2 proves release moved the victim through `mpCommonSetFighterAir`, bit 3
  proves both fighters reached `mpCommonSetFighterWaitOrFall`, bit 4 proves
  catcher `catch_gobj` and victim `capture_gobj` were cleared, bit 5 proves
  final Wait/Ground `10/0` for the catcher and Fall/Air `26/1` for the victim,
  and bit 6 proves guard cleanup with unsafe count zero. This is still a
  bounded cleanup proof, not continuous death/throw/damage scheduling.
- `STAGE_MPPASSIVE_WALLDAMAGE`: recover-loop-only bounded original
  WallDamage status/update proof. Current modes `155/156` require mask
  `0x1ff`: bit 0 proves the selected Hyrule wall-hit/wall-copy source was
  available, bit 1 proves original `ftCommonWallDamageCheckGoto` accepted the
  seeded left-wall collision, bit 2 proves WallDamage/Ground status and motion
  `56/49`, bit 3 proves the bounded `ftMainSetStatus` seam installed the
  expected original callback shape, bit 4 proves effect/quake/rumble/timed
  intangible seams ran once, bit 5 proves the reflected knockback/facing and
  `15` intangible ticks, bit 6 proves source floor/wall IDs `5/13`, bit 7
  proves one installed original WallDamage update tick handed off into
  DamageFall/Air `57/50`, and bit 8 proves the WallDamage active guard cleaned
  up. The bit-7 handoff now requires imported-original
  `ftCommonDamageFallSetStatusFromDamage` to call the bounded `ftMainSetStatus`
  DamageFall install and imported clamp/rumble tail; the public rumble counter
  remains the original WallDamage setup rumble ID `2`. The companion
  `STAGE_MPPASSIVE_WALLDAMAGE_STATE` marker records
  `56/49/0 -> 57/50/1`, hitstun `1 -> 0`, and intangible `15`.
  `STAGE_MPPASSIVE_WALLDAMAGE_VEC` records the negative incoming X velocity,
  positive reflected damage X velocity, positive knockback, LR `-1`, and
  source floor/wall IDs `5/13`.
- `STAGE_MPPASSIVE_REBOUND`: recover-loop-only bounded original
  ReboundWait/Rebound proof. Current modes `155/156` require mask `0x7f`:
  bit 0 proves original `ftCommonReboundWaitSetStatus` reached
  ReboundWait/Ground `82/-1`, bit 1 proves the expected ReboundWait callback
  slots were installed, bit 2 proves original rebound math from
  `rebound_anim_length / attack_rebound` and `hit_lr` produced timer `6000`,
  anim speed `3000`, and ground X velocity `-12000` in milli-units, bit 3
  proves the installed original ReboundWait update transitioned into
  Rebound/Ground `83/71`, bit 4 proves one original Rebound update reached
  Wait/Ground `10/4`, bit 5 proves the active guards cleaned up, and bit 6
  proves no unsafe count was raised. `STAGE_MPPASSIVE_REBOUND_STATE` records
  `82/-1 -> 83/71 -> 10/4`; `STAGE_MPPASSIVE_REBOUND_VEC` records velocity,
  anim speed, timer before/final, LR, and hit LR.
- `STAGE_MPPASSIVE_SETUP`: prepared flag, base CliffWait damage proof seen
  flag, and unsafe count. Current proof records prepared `1`, base proof
  seen `1`, and unsafe count `0`.
- `STAGE_MPPASSIVE_PASSIVESTAND`: bounded PassiveStandF/Ground runtime
  diagnostics. Current proof records setup/stable/final status/motion/GA
  `73/62/0 -> 73/62/0 -> 10/4/0`, one PassiveStand `ftMainSetStatus` seam,
  one ground-placement call, one velocity-transfer seam, installed callback
  flag `1`, guarded update/physics/map tick counts, one Wait
  `ftMainSetStatus` handoff, `playertag_wait=120`, and valid final Wait
  callbacks. The original Passive-loop modes record `3/2/2` with two stable
  frames; recover-loop modes `155/156` record `6/5/5` with five stable frames.
- `STAGE_MPPASSIVE_PASSIVE`: bounded Passive/Ground runtime diagnostics.
  Current proof records setup/stable/final status/motion/GA
  `81/70/0 -> 81/70/0 -> 10/4/0`, one Passive `ftMainSetStatus` seam, one
  ground-placement call, one velocity-transfer seam, installed callback flag
  `1`, guarded update/physics/map tick counts, one Wait `ftMainSetStatus`
  handoff, `playertag_wait=120`, and valid final Wait callbacks. The original
  Passive-loop modes record `3/2/2` with two stable frames; recover-loop modes
  `155/156` record `6/5/5` with five stable frames.
- `STAGE_MPPASSIVE_FINAL`: final Wait/Ground status, motion, ground-air, and
  player-tag state after the PassiveStand and Passive branches complete. Current
  proof records both branches ending in Wait/Ground `10/4/0` with
  `playertag_wait=120`.

## Stage MP Damage Recover Loop Markers

The `battle_mariofox_stage_mpdamage_recover_loop` and
`menu_chain_mariofox_stage_mpdamage_recover_loop` verifiers are regression
coverage for modes `157/158`. They consume the PassiveStand/Passive recover aggregate
and the Dash-Run selected contact/damage setup proof, then add a bounded
source-shaped selected hit-to-damage-to-recovery lifecycle proof. The marker
group is emitted only when the verifier passes
`-RequireStageMPDamageRecoverLoop`.

- `STAGE_MPDAMAGE_RECOVER`: top-level direct/menu damage-recover result, safe
  result, proof mask, deferred mask, and selected fighter count. A passing
  proof reports `0x46445243`, `0x46445253`, mask low bits `0xffff`,
  deferred mask `0xff`, and count `1`.
- `STAGE_MPDAMAGE_RECOVER_SETUP`: base proof reachability plus selected
  contact/proc-params setup counts. A passing proof has prepared,
  PassiveRecover base, Dash damage base, contact seed/decision/hit, and
  proc-params call/hit counts all nonzero.
- `STAGE_MPDAMAGE_RECOVER_CONTACT`: selected attacker/victim slots, selected
  contact decision/hit counts, proc-params call/hit counts, and lag-start /
  lag-update / lag-end callback reachability.
- `STAGE_MPDAMAGE_RECOVER_DAMAGE`: victim percent, damage queue, hitlag,
  hitstun, knockback, angle, element, and damage-index evidence. The current
  summary records `dmg=0->4` and `hitlag=6`.
- `STAGE_MPDAMAGE_RECOVER_STATUS`: expected/actual ground and air damage
  status/motion plus the DamageFall handoff. Current summary records
  `status=52/45` and `fall=57/50`.
- `STAGE_MPDAMAGE_RECOVER_CALLBACKS`: bounded damage update/status,
  DamageCommon, and DamageAir callback counts; all fields must be nonzero.
- `STAGE_MPDAMAGE_RECOVER_GROUND`: ground probe, wait handoff, DamageAir map,
  DamageFall setup, and DamageFall map reachability.
- `STAGE_MPDAMAGE_RECOVER_BRANCHES`: PassiveStand, Passive, and DownBounce
  branch reachability. Current summary records `ps=1 passive=1 dbounce=1`.
- `STAGE_MPDAMAGE_RECOVER_VEL`: selected victim damage velocities and root
  position evidence; at least one damage velocity component must be nonzero.
- `STAGE_MPDAMAGE_RECOVER_SAFE`: final floor-line and safety counters. A
  passing proof has both selected fighters on valid floor lines, no final
  recenter, no unexpected scene, and no unsafe count.

This proves one bounded selected contact-to-recovery lifecycle. It does not
enable full live hitbox collision runtime, full `gmcollision.c`, full
`ftmain.c`, arbitrary damage-state duration, complete hitlag/damage runtime,
items/weapons, HUD, audio, or unbounded gameplay scheduling.

## Stage MP Live-Hit Damage Loop Markers

The `battle_mariofox_stage_mplivehit_damage_loop` and
`menu_chain_mariofox_stage_mplivehit_damage_loop` verifiers are standalone
regression coverage for modes `159/160`. The current Boundary/Latest pair is
`battle_mariofox_stage_mplivehit_status_loop` and
`menu_chain_mariofox_stage_mplivehit_status_loop` for modes `161/162`, which
consume this damage-loop proof and add bounded selected damage-status
follow-through. The marker group is emitted only when the verifier passes
`-RequireStageMPLiveHitDamageLoop`.

- `STAGE_MPLIVEHIT_DAMAGE`: top-level live-hit result, safe result, proof mask,
  deferred mask, and count. A passing proof reports `0x464c4844`,
  `0x464c4853`, mask low bits `0xffffffff`, deferred mask `0x1`, and count
  `1`.
- `STAGE_MPLIVEHIT_SETUP`: prepared flag, damage-recover base, Dash damage
  base, attacker/victim slots, proof-local state save/restore counts, and the
  explicit full-collision-deferred count.
- `STAGE_MPLIVEHIT_ATTACK`: selected Attack12 status/motion/ground-air and
  installed callback mask. Current proof records `191/166/0` and callback mask
  `0xff`.
- `STAGE_MPLIVEHIT_EVENTS`: source-backed animation-event parse count,
  selected script count, make-attack count, command mask, and source-shaped
  same-group attack-record carry/clear mask. Current proof records carry mask
  `0xf` for seed, copy, clear, and restore.
- `STAGE_MPLIVEHIT_ATTACKDATA`: selected hitbox metadata from the decoded
  original motion command. Current proof records Fox Jab2 attack ID `1`, joint
  `14`, damage `4`, and nonzero flags.
- `STAGE_MPLIVEHIT_SECONDARY`: sibling Fox Jab2 hitbox metadata and bounded
  contact-gate probe. Current proof records attack ID `0`, joint `14`, damage
  `4`, normalized radius `100`, forward offset `140`, angle `70`, flags
  `0x7`, and mask low bits `0x7f` for decode, metadata, local
  New/Transfer/Interpolate, range, rectangle, and collide gates. This is
  diagnostic coverage only; the selected damage scheduling path remains
  hitbox `1`.
- `STAGE_MPLIVEHIT_HURTBOX`: bounded source-order Mario hurtbox scan. The
  proof first checks the original global `special_hitstatus` /
  `star_hitstatus` / `hitstatus` gate, including each intangible skip case,
  and the per-attack `gFTMainIsDamageDetect[i] == FALSE` skip. It then
  restores detection, temporarily marks damage-coll slots `0` and `1`
  intangible, and follows the original `ftmain.c` loop shape: skip intangible
  slots, test one normal non-`None` miss, continue, test the next normal slot,
  break on first hit, and observe the first `None` sentinel. Current proof
  records mask low bits `0x1ffff`, active count `10`, sentinel slot `10`, two
  skips, two tests, one hit, first hit slot `3`, a nonzero joint, and normal
  hitstatus `0`.
- `STAGE_MPLIVEHIT_HURTBOX_DAMAGE`: bounded source-order damage consumption.
  It feeds the first selected hurtbox-scan hit slot into the selected damage
  record, hitlog, stats, percent-damage, and hitlag scheduling handoff, then
  restores both fighter structs and reruns the same fighter-only search path
  unmasked against natural damage-coll slot `0`. It then reruns once more
  without clearing the attack record to prove source-order same-victim repeat
  rejection, then restores again and proves natural slot-1, slot-2, slot-4,
  slot-5, slot-6, slot-7, slot-8, and slot-9 consume by marking earlier slots intangible. Current proof records mask low bits
  `0x7ffff`; printed slot
  `3`, joint, queued damage `0 -> 4`, percent damage `0 -> 4`, and hitlag `6`
  remain the selected slot-3 diagnostic, while the added bits prove natural
  slot-0 consume, rejected repeat search, and natural slot-1/2/4/5/6/7/8/9 consume.
- `STAGE_MPLIVEHIT_EFFECTONLY`: bounded source-shaped
  `ftMainUpdateDamageStatFighter` non-normal damage-coll branch. It marks the
  selected slot invincible, proves attack-record insertion and attacker
  `attack_damage`, skips damage queue/percent/hitlog, records one set-off
  effect seam and one hit SFX seam, then restores both fighter structs.
  Current proof records mask low bits `0x1ff`, status `2`, damage `4`,
  queue `0 -> 0`, percent `0 -> 0`, unchanged hitlog, effect count `1`,
  SFX count `1`, and attacker damage `4`.
- `STAGE_MPLIVEHIT_RESIST`: bounded source-shaped
  `ftMainCheckGetUpdateDamage` damage-resist false-return branch. It seeds
  damage resist above the selected Fox Jab2 damage, proves the resist value is
  decremented without queue/percent/hitlog changes, records the same set-off
  effect and hit SFX seams, then restores both fighter structs. Current proof
  records mask low bits `0xfff`, damage `4`, resist `7 -> 3`, flag still set,
  unchanged queue/percent/hitlog, effect count `1`, SFX count `1`, and attacker
  damage `4`.
- `STAGE_MPLIVEHIT_RESIST_BREAK`: bounded source-shaped
  `ftMainCheckGetUpdateDamage` damage-resist breakthrough branch. It seeds
  damage resist below the selected Fox Jab2 damage, proves the helper returns
  true, clears the resist flag, leaves a negative resist remainder, and queues
  leftover damage/lag. Current proof records mask low bits `0x7f`, resist
  `2 -> -2`, flag `0`, leftover damage `2`, queue `2`, and lag `2`.
- `STAGE_MPLIVEHIT_THROWATTR`: bounded source-shaped
  `ftMainUpdateDamageStatFighter` throw-owner attribution branch. It seeds the
  selected Fox attacker with original-compatible `throw_gobj`/owner fields,
  then proves the hitlog attacker player/player-num use the throw owner rather
  than the direct attacker before restoring both fighter structs. Current proof
  records mask low bits `0x1f`, direct attacker `1`, throw owner `0`, and a
  hitlog owner match.
- `STAGE_MPLIVEHIT_ATTACK_CLASH`: bounded source-shaped
  `ftMainUpdateAttackStatFighter` / `ftMainSetHitRebound` branch. It seeds one
  active attack coll on each selected fighter, proves both hit-interact attack
  records store the opposing group, clears attack/damage detect in the same
  ignore-flag order as the original, applies the US rebound formula, records
  facing signs, counts both setoff-effect seams, and restores state. Current
  proof records mask low bits `0x3f`, groups `4/2`, push `24/18`, rebound
  milli `42880/33160`, LR `-1/+1`, and effect count `2`.
- `STAGE_MPLIVEHIT_CATCHSTAT`: bounded source-shaped tail of
  `ftMainUpdateCatchStatFighter`. It calls the project-owned
  `ftMainSetHitInteractStats` seam for the selected attack group, proves the
  selected hurt interaction record and attack-detect clear, computes the
  source-order absolute X distance between the fighters, assigns the closer
  `search_gobj`, then restores fighter/root/detect state. Current proof
  records mask low bits `0x1f` and `160000` milli-units.
- `STAGE_MPLIVEHIT_CATCHSEARCH`: bounded source-shaped
  `ftMainSearchFighterCatch` pre-stat gate. It resets search target/distance,
  proves normal target and ground-air gates, exercises a hurt-record skip,
  proves the default-record pass, skips status-disabled and non-grabbable
  damage-coll slots, collides with selected slot `3`, assigns the same closest
  target/distance, reruns the source-order search and rejects the same-victim
  catch through the attack-record gate, then restores saved fighter structs and
  proves a natural slot-0 catch search plus the self-before-target, ghost, Boss, same-team,
  capture-immune, global target hitstatus, attack-state-off, Ground/Air
  mismatch, attack-record skip, hitstatus-none sentinel, invincible
  damage-coll skip, and valid no-collision no-update gates, then routes through
  bounded `ftMainProcSearchCatch` to prove
  hazard wait-timer decrement before the catch-status gate, closed-gate
  no-search/no-callback behavior, and catch/capture callbacks after a selected
  target is found. It also proves the original two-entry ground-obstacle
  registry add/full/clear ordering and false-return callback iteration, proves
  the hazard ghost gate exits before wait-timer decrement or callbacks, then
  routes one true-return Twister obstacle through imported original
  `ftCommonTwisterSetStatus` to prove Twister status/motion/callback install,
  release wait reset, tornado GObj capture, capture immunity, and wait-timer
  decrement, then runs one installed imported-original Twister update/physics
  callback tick to prove release-wait advance, bounded air velocity, and root
  Y-rotation, then routes one true-return TaruCannon obstacle through
  `ftMainSetHitHazard` into the source-ordered TaruCannon setup shell to prove
  status `61`, script `-1`, TaruCannon vars reset, barrel GObj capture,
  capture immunity, invisible flag, intangible hitstatus, and one installed
  original TaruCannon physics callback tick copying the fighter root position
  from the barrel root before restoring state. Current proof records mask low
  bits `0xffffffff`, skip mask `0x3fff`, slot `3`, and `160000` milli-units.
  Continuous TaruCannon update/shoot runtime remains deferred until the Jungle
  barrel helpers and map throw-hit data are local.
- `STAGE_MPLIVEHIT_POS`: selected hitbox position plus attack-state lifecycle:
  `Off -> New -> Transfer -> Interpolate`, with the proof-local matrix-reset
  marker set.
- `STAGE_MPLIVEHIT_COLL`: bounded selected range, rectangle, contact,
  damage-record insertion, immediate repeat-hit probe/reject, and writeback
  markers.
- `STAGE_MPLIVEHIT_REHIT`: bounded source-shaped hit-interact record and
  refresh-clear proof. It calls the project-owned `ftMainSetHitInteractStats`
  compatibility seam for damage, shield, and attack-group updates, proves the
  source-order record gate that enables damage detect only for an empty/default
  record and skips hurt, shield, and nondefault group records, then proves
  `ftParamRefreshAttackCollID` clears the selected Attack12 record and its
  seeded rehit timer. Current summary records `gate=0x3f rehit=5->0 clear=1`.
- `STAGE_MPLIVEHIT_SHIELD`: bounded source-shaped
  `ftMainUpdateShieldStatFighter` side proof. It records attacker shield push,
  victim shield damage before/after/total, source-facing `shield_lr`,
  source-player diagnostic, one set-off effect size, and the GuardSetOff
  status/motion/hitlag/clear mask. Current summary records
  `shield=4->4/4 so=155/134`.
- `STAGE_MPLIVEHIT_SHIELD_SETOFF_TICK`: bounded imported original
  `ftCommonGuardSetOffProcUpdate` tick proof inside the selected live-hit
  shield branch. It records mask `0x1f`, held-Z status `155`, released-Z
  status `154`, and held-Z frame decrement to `1000` milli-units. Current
  summary records `soTick=0x1f/155->154`.
- `STAGE_MPLIVEHIT_SHIELD_CONTACT`: bounded source-order shield-contact gate
  proof matching the selected `ftmain.c` branch shape. It records the
  proof-local contact mask, attack ID, damage-detect flag, bounded shield
  collision count, hit count, and shield angle in milli-radians. Current
  summary records `shc=0x7fffff/3142`, proving `is_shield`, damage detect,
  active attack, shield joint/sphere, shield-stat handoff, shield-health
  decrement, ShieldBreakFly branch plus its hitlag/input/transient-clear tail,
  normal shield-contact common tail clear including special/rebound
  transients and `hitlag_mul` reset, and shield-heal branch in the
  `ftMainProcParams` order,
  record clear, state restore, the adjacent
  `is_shield == false` skip path that leaves
  damage detect available for the later hurtbox branch, and the sibling
  `gFTMainIsDamageDetect == false` shield-contact skip path, plus immediate
  same-victim shield repeat rejection with no collision/stat/effect/
  shield-damage changes.
- `STAGE_MPLIVEHIT_ORIG_REHIT_HIT`: bounded imported original
  `ftCommonAttackAirLwProcHit` seed branch. It temporarily uses Link down-air,
  dirties attack IDs `0/1`, calls the original hit callback, and proves the
  callback clears active attacks, clears fastfall, applies vertical bounce
  velocity `40000` milli-units, enters AttackAirLw status/motion `213/188`,
  resets animation frame to `35000`, and seeds the 30-tick timer.
- `STAGE_MPLIVEHIT_ORIG_REHIT`: bounded imported original
  `ftCommonAttackAirLwProcUpdate` rehit-timer window. It temporarily uses the
  Link down-air branch on the selected live shell after the original hit seed,
  proves the original `30 -> 29 -> ... -> 1` countdown does not refresh or
  clear records early, then `1 -> 0` refreshes attack IDs `0/1` to `New`,
  clears both attack records, leaves `is_attack_active` set, and restores the
  proof-local state. Current summary records
  `origRehit=hit30/v40000 30->29->0 clear=1/3 ids=0x3`.
- `STAGE_MPLIVEHIT_DAMAGESTATE`: victim damage before/after, hitlag, hitstun,
  knockback, and damage velocity evidence. Current summary records
  `dmg=0->4 hitlag=6`.
- `STAGE_MPLIVEHIT_PROC`: selected hit-log, SFX, stats, proc-params, and
  lag-start/update/end markers.
- `STAGE_MPLIVEHIT_STATUS`: status-loop result, safe result, proof mask,
  deferred mask, and count. Modes `161/162` require pass values
  `0x464c5450` / `0x464c5453`, mask `0xffffffff`, deferred mask `0x1`, and count
  `1`.
- `STAGE_MPLIVEHIT_STATUS_SETUP`: inherited damage-loop proof, proof-local
  state save/restore slots, and Attack12 seed evidence.
- `STAGE_MPLIVEHIT_STATUS_SEARCH`: bounded source-shaped search evidence.
  Current proof records search mask `0xf` plus nonzero hurtbox, hit-collision,
  damage-record, and hit-interact clear counts.
- `STAGE_MPLIVEHIT_STATUS_PROC`: bounded source-shaped proc evidence. Current
  proof records proc mask `0x1f`, status `17->52`, motion `45`, damage
  increase, and lag-start reachability.
- `STAGE_MPLIVEHIT_STATUS_HITLAG`: selected hitlag lifecycle. Current proof
  records hitlag `6->0` with lag callbacks `1/6/1`.
- `STAGE_MPLIVEHIT_STATUS_CALLBACK`: post-hitlag installed imported-original
  damage callback ticks. Current proof records callback mask low bits `0xffffffff`,
  the selected damage status matching `STAGE_MPLIVEHIT_STATUS_PROC`, hitstun
  `2->1`, an active-animation no-expiry tick that releases public knockback
  without entering DamageFall, one `ftCommonDamageSetStatus` knockback-over
  branch that clears the flag and sets timed hit-status invincibility, one
  `ftCommonDamageSetStatus` DamageFlyRoll branch that reaches
  `ftCommonDamageFlyRollUpdateModelPitch` and updates joint pitch, one
  `ftCommonDamageCommonProcPhysics` ground-friction branch that reduces ground
  velocity while preserving ground damage status, one zero-hitstun
  `ftCommonDamageCommonProcPhysics` Air branch through
  `ftPhysicsApplyAirVelDriftFastFall` that sets fastfall state and terminal
  velocity, one zero-hitstun `ftCommonDamageCommonProcPhysics` Air branch
  through drift/gravity that updates X velocity without fastfall, one
  zero-hitstun `ftCommonDamageCommonProcPhysics` Air branch through
  air-velocity clamp-decrement, one
  `ftCommonDamageCommonProcPhysics` low-speed throw branch that clears attack
  collisions, one `ftCommonDamageCommonProcPhysics` DamageFlyRoll branch that
  updates joint pitch, one `ftCommonDamageCommonProcLagUpdate` Smash DI branch that
  moves the root and resets tap buffers, one `ftCommonDamageCommonProcLagUpdate`
  no-op gate proof for no-hitlag, below-threshold stick, and saturated tap
  buffers, one `ftCommonDamageAirCommonProcMap` collision/no-floor
  short-circuit that skips PassiveStand, Passive, and DownBounce, one
  `ftCommonDamageAirCommonProcMap` floor-collision gate proof and one
  `ftCommonDamageFallProcMap` floor-collision gate proof through
  imported-original PassiveStand and Passive true-return short-circuits, one
  ground animation-expiry
  `ftCommonDamageCommonProcUpdate` tick that releases public knockback through
  original `mpCommonSetFighterWaitOrFall` into Wait/Ground, one installed
  ground `ftCommonDamageCommonProcInterrupt` tick that clears hitstun state and
  reaches imported-original Wait/Ground interrupt handling, one air
  `ftCommonDamageCommonProcInterrupt` tick that reaches the no-hammer
  `ftCommonFallProcInterrupt` branch, one ground hammer
  `ftCommonDamageCommonProcInterrupt` tick that reaches
  `ftHammerProcInterrupt`, one air hammer `ftCommonDamageCommonProcInterrupt`
  tick that reaches `ftCommonHammerFallProcInterrupt`, one installed
  original damage physics tick reducing selected air
  velocity to `11500/-1000` through air friction/gravity, one installed
  original damage interrupt tick recorded as `interrupt=1`, one installed
  original damage map no-collision tick recorded as `map=1/1`, one installed
  original damage map floor-collision branch recorded as `floor=1/1/1/1`
  for collision, PassiveStand check, Passive check, and DownBounce handoff,
  one installed original damage map wall/ceiling-collision branch recorded as
  `wall=0x3ffff`: low six bits cover left-wall WallDamage helper,
  short-circuit, knockback, and restore; middle six bits cover ceiling; high
  six bits cover right-wall; one post-expiry DamageFall map cliff-catch
  branch recorded as `cliff=0x3f` for cliff collision, count-only
  CliffCatch seam, passive/DownBounce skip, preserved DamageFall state,
  source route, and a proof-local imported-original
  `ftCommonCliffCatchSetStatus` side proof; and
  one post-expiry DamageFall map no-collision/floor fallback proof recorded
  as `fallmap=0x7ff` for no-collision, floor collision, PassiveStand/Passive
  checks, count-only DownBounce handoff, collision without a cliff mask,
  imported PassiveStand/Passive short-circuits, preserved DamageFall state,
  source route, and a proof-local imported-original `ftCommonDownBounceSetStatus`
  floor-collision side proof; one post-expiry `ftCommonDamageFallProcInterrupt`
  source-order proof through SpecialAir, AttackAir, JumpAerial, and HammerFall
  checks; selected expiry status/motion `57/50`; and public knockback release
  `23000`.
- `STAGE_MPLIVEHIT_STATUS_REPEAT`: immediate repeat-hit probe/reject and repeat
  gate evidence. Current proof records `1/1 gate=0x3f`.
- `STAGE_MPLIVEHIT_STATUS_SAFE`: final floor-line and safety counters for the
  status-loop pair.
- `STAGE_MPLIVEHIT_RECOVER`: consumption of the damage-recover aggregate,
  damage status/motion, and PassiveStand/Passive/DownBounce branch evidence.
- `STAGE_MPLIVEHIT_SAFE`: final floor-line and safety counters. A passing proof
  has both selected fighters on valid floor lines, no final recenter, no
  unexpected scene/status, and no unsafe count.

Modes `161/162` prove one bounded selected live-hit-to-damage lifecycle plus a
selected damage-status follow-through (`status=17->52/45`, `hitlag=6->0`,
callbacks `1/6/1`, `update=2->1`, `phys=11500/-1000`, `finish=57/50`,
`search=0xf`, `repeat=1/1 gate=0x3f`) plus the
non-normal hitstatus and damage-resist effect/SFX no-damage branches, a sibling
Fox Jab2 hitbox decode/contact-gate probe, bounded source-order Mario hurtbox
scan with selected slot-3 damage bookkeeping after one tested miss plus a
restored unmasked natural slot-0 consume/repeat reject and natural
slot-1/2/4/5/6/7/8/9 consume, and one
selected shield-contact
gate. It does not enable broad `gmcollision.c`, full `ftmain.c`, continuous
multi-hitbox runtime, broader natural continuous multi-slot victim/damage runtime,
continuous shield runtime beyond the selected contact/set-off/update-tick/health-decrement/break/heal/break-clear/tail-clear/special-clear/hitlag-mul-clear branch,
continuous rehit gameplay beyond the selected Link down-air timer window,
arbitrary damage-state duration, complete hitlag/damage runtime, items/weapons,
HUD, audio, or unbounded gameplay scheduling.

## Battle Memory Ledger Markers

Mode `163` prints the battle memory ledger through the existing
`battle_playable` verifier. `MEMARENA` records result marker, scene, reloc
generation, taskman arena capacity, current/high-water arena use, arena
headroom, VSBattle DL bytes, graphics bytes, RDP bytes, and
`gFTManagerFigatreeHeapSize`. `MEMRELOC` records loaded reloc file count,
resident bytes, stage/fighter/interface/menu/opening/other bytes, and stale
file/byte counts. `MEMEVICT` records the last scene-generation eviction
file/byte count. The current passing proof reports `head=235220`,
`reloc=618448`, `stage=202816`, `fighter=206960`, `if=208672`,
`stale=0/0`, and direct-route `evict=0/0`. The source-sized VSBattle taskman
buffer assertions come from `scvsbattle.c:31-41`; the original taskman heap
setup/allocator path is `sys/taskman.c:267-383`.

## Projectile Proof Markers

Mode `163` prints `PROJECTILE` by default. It records result, mask, selected
actor slot, selected weapon kind, B-press frames, special-status frames,
special motion, accessory frames, motion `flag0` frames, spawn calls,
successful spawns, update/map/hit destroy counts, resident weapon frames, max
weapon count, weapon-kind mask, attack-state mask, max damage, max lifetime,
and map mask. With Fox reflector default-on, the proof drives Mario fireball
into Fox shine and also prints `REFLECTOR`.

Current default pass:

- Mario fireball reflected by Fox:
  `projectile=actor0/kind0 b=1 status=23 accessory=23 flag0=0 spawn=1 ok=1 destroy=0/0/0 weaponFrames=46 max=1 kindMask=0x1 attackMask=0xc dmg=13 life=140 map=0x401`.
- Reflector:
  `reflector=0xff fox1 proj0 shine=9/14/9 reflect=23 lr=-1 clear=1688 proc=1 vx=49809->-49809 owner=1 attrs=ref1/abs1/shield1/count1/dmg13/size100000 delta=-3053077/-3424614 special=350000/50`.

The source path is original common B dispatch in
`ftcommonspecialn.c:88`-`:101`, motion-event `flag0` in `ftmain.c:624`,
accessory/update execution in `ftmain.c:1855`-`:1857`, Mario spawn at
`ftmariospecialn.c:17`-`:53`, Fox spawn at `ftfoxspecialn.c:11`-`:25`,
weapon creation/attack metadata in `wpmanager.c:87`-`:104` and
`:191`-`:236`, and weapon processes at `wpmanager.c:304`-`:306`. Mario's
immediate hit-destroy path follows `wpmariofireball.c:126`-`:131` and
`wpprocess.c:469`-`:474`.
The reflector path follows `ftfoxspeciallw.c:110`, `:171`, and `:212` for
`is_reflect`, `ftmain.c:3342`-`:3350` and `:3973` for weapon special-collision
resolution, `ftmain.c:4010` for `reflect_lr` clear, and `wpprocess.c:522`-`:573`
plus `wpmain.c:103`-`:109` for projectile owner/direction transfer.

## Natural Moveset Marker

Mode `163` prints `NAT_MOVESET` when the default original normal-moveset path
is active. The required mask is now `0x7ff`: tilts S3/Hi3/Lw3, tilt hitbox,
charged S4 and hitbox, aerial, landing, Catch/CatchWait, ThrowF/ThrowB,
ThrownCommon, and throw recovery. The current battle-playable proof reports
`moveset=0x7ff phase=15`, `grab=3/1`, `throw=12/5/11`, and
`throwDmg=0->12`.

Catch input and status entry follow `ftcommoncatch1.c:111-136`; CatchWait
throw dispatch follows `ftcommoncatch2.c:57-76`; ThrowF/ThrowB selection and
victim thrown setup follow `ftcommonthrow.c:56-126`; throw damage updates
follow `ftcommonthrown2.c:129-180`; the source descriptor table wiring is
`ftcommonstatus.h:3347-3430`.

- `STAGE_MPDOWNWAIT`: direct/menu DownWait-loop result, safe result, proof
  mask, deferred mask, and selected fighter count. Current pass values are
  `0x46445749`, `0x46445755`, mask `0x3ffff`, deferred mask `0x3ffff`, and
  count `1`.
- `STAGE_MPDOWNWAIT_SETUP`: prepared flag, base Passive-loop proof seen flag,
  DownWait setup wrapper count, DownWait `ftMainSetStatus` seam count,
  capture-immune count, DownWait status/motion/ground-air, stand wait,
  capture mask, damage multiplier in milli-units, and installed callback flag.
  Current proof records DownWaitU/Ground `70/-2/0`, `stand_wait=180`,
  capture mask `0x33`, damage multiplier `500`, and valid DownWait callbacks.
- `STAGE_MPDOWNWAIT_INTERRUPT`: original DownWait interrupt source-order
  diagnostics: interrupt call, DownAttack check, forward/back check, DownStand
  check, DownStand status request, DownStand `ftMainSetStatus`, packed source
  order, and unsafe count. Current proof records one call for each check/status
  step, source order `0x12345`, and unsafe count `0`.
- `STAGE_MPDOWNWAIT_INPUT`: seeded DownWait interrupt input. Current proof
  records stick `0,80`, no button tap, and nonzero Z mask while the stand-up
  branch is accepted through the stick-angle path.
- `STAGE_MPDOWNWAIT_STATUS`: before/after DownWait interrupt status/motion/
  ground-air, stand wait, wakeup flag, damage multiplier, and installed
  DownStand callback flag. Current proof records
  DownWaitU/Ground `70/-2/0` -> DownStandU/Ground `72/61/0`, `flag1`
  `1 -> 0`, damage multiplier `1000`, and valid DownStand callbacks.
- `STAGE_MPDOWNWAIT_DOWNSTAND_TICK`: guarded DownStand interrupt and callback
  diagnostics: original DownStand interrupt tick count, KneeBend/Pass/Dokan
  check counts reached through the imported DownStand interrupt, post-interrupt
  wakeup flag, update/physics/map/stable-frame counts, stable
  status/motion/ground-air, Wait `ftMainSetStatus` and player-tag counts,
  final status/motion/ground-air, final player-tag wait, and final Wait
  callback flag. Current proof records one imported DownStand interrupt tick,
  eight KneeBend/Pass/Dokan checks each, eight stable DownStandU/Ground
  `72/61/0` callback ticks, then Wait/Ground `10/4/0` with
  `playertag_wait=120`.
- `STAGE_MPDOWNWAIT_ATTACK`: guarded A-button branch diagnostics for the same
  original DownWait interrupt callback: interrupt call count, original
  DownAttack check count, bounded DownAttack status request count,
  DownAttack `ftMainSetStatus` seam count, animation-event call count, packed
  source order, button tap, A/B masks, before status/motion/ground-air, after
  status/motion/ground-air, motion attack ID, status attack ID, stat attack ID,
  and installed callback flag. Current proof records
  source order `0x1234` and DownWaitU/Ground `70/-2/0` -> DownAttackU/Ground
  `80/69/0` with attack IDs `53/33/33` and valid DownAttack callbacks.
- `STAGE_MPDOWNWAIT_ATTACK_TICK`: guarded DownAttack callback/final-handoff
  diagnostics: update, physics, map, stable-frame counts, stable
  status/motion/ground-air, Wait `ftMainSetStatus` and player-tag counts,
  final status/motion/ground-air, final player-tag wait, and final Wait
  callback flag. Current proof records DownAttackU/Ground `80/69/0` for
  eight stable update/physics/map callback frames plus the final animation-end
  update, then Wait/Ground `10/4/0` with `playertag_wait=120`.
- `STAGE_MPDOWNWAIT_ROLL`: guarded forward/back stick branch diagnostics for
  the same original DownWait interrupt callback: interrupt call count, original
  DownAttack check count, original forward/back check count, bounded roll
  status request count, roll `ftMainSetStatus` seam count, animation-event
  call count, and packed source order for forward/back probes. Current proof
  records two roll probes, two original attack checks, two original
  forward/back checks, two status requests, two `ftMainSetStatus` seams, two
  animation-event calls, and source order `0x12345` for both directions.
- `STAGE_MPDOWNWAIT_ROLL_STATUS`: forward/back stick input and after-state
  diagnostics for the guarded roll probes. Current proof records forward stick
  `80,0` -> DownForwardU/Ground `76/65/0` and back stick `-80,0` ->
  DownBackU/Ground `78/67/0`, with `is_jostle_ignore=1` and valid roll
  callbacks for both branches.
- `STAGE_MPDOWNWAIT_ROLL_FORWARD_TICK`: guarded DownForward callback/final
  handoff diagnostics with the same field order as
  `STAGE_MPDOWNWAIT_ATTACK_TICK`. Current proof records DownForwardU/Ground
  `76/65/0` for eight stable callback ticks, then Wait/Ground `10/4/0` with
  `playertag_wait=120`.
- `STAGE_MPDOWNWAIT_ROLL_BACK_TICK`: guarded DownBack callback/final handoff
  diagnostics with the same field order as `STAGE_MPDOWNWAIT_ATTACK_TICK`.
  Current proof records DownBackU/Ground `78/67/0` for eight stable callback
  ticks, then Wait/Ground `10/4/0` with `playertag_wait=120`.
- `STAGE_MPDOWNWAIT_ROLL_MOVE`: bounded roll root-motion diagnostics:
  forward root X before/after/delta, back root X before/after/delta, and
  direction mask. Current proof records forward/back deltas
  `+10000/-10000` milli and mask `0x3`, proving both roll callback paths move
  in their expected directions before the final Wait handoff.
- `STAGE_TURN`: direct/menu Turn-loop result, safe result, proof mask,
  deferred mask, and selected fighter count. Current pass values are
  `0x46545552`, `0x46545553`, mask `0x1ff`, deferred mask `0xff`, and count
  `1`.
- `STAGE_TURN_CALLS`: bounded source-order Turn proof counts: prepared flag,
  base DownWait-loop proof seen flag, original Turn check, successful check,
  Turn status request, bounded `ftMainSetStatus`, animation-event seam,
  update tick, final update tick, physics tick, map tick, and unsafe count.
  Current proof records all required counts as `1` and unsafe count `0`.
- `STAGE_TURN_INPUT`: seeded Turn input and before-state diagnostics. Current
  proof records stick X `-80`, Wait/Ground `10/4/0`, facing `1`, positive
  ground velocity `2500` milli, and valid Turn callbacks after setup.
- `STAGE_TURN_SETUP`: state after original `ftCommonTurnCheckInterruptCommon`
  accepts the seeded input and reaches bounded original Turn status setup.
  Current proof records Turn/Ground `18/12/0`, facing still `1`, velocity still
  positive, `is_allow_turn_direction=0`, `is_disable_sa_interrupts=0`, button
  mask `0`, `lr_dash=0`, `lr_turn=-1`, and `attacks4_buffer=256`.
- `STAGE_TURN_UPDATE`: state after one guarded original `ftCommonTurnProcUpdate`
  tick with `motion_vars.flags.flag1` armed. Current proof records Turn/Ground
  `18/12/0`, facing `-1`, ground velocity `-2500` milli, flag cleared, and
  Turn direction / special-attack interrupt flags set to `1`.
- `STAGE_TURN_FINAL`: state after a second guarded Turn update with
  `anim_frame=0`. Current proof records original Wait/Ground handoff `10/4/0`,
  facing `-1`, nonpositive retained ground velocity, one Wait status handoff,
  one player-tag wait call, `playertag_wait=120`, and valid Wait callbacks.
- `STAGE_MPCLIFFWAIT_DAMAGE_DOWNBOUNCE`: bounded original DownBounce update
  diagnostics. Current proof records two guarded DownBounce update ticks, one
  original DownBounce attack check, one original forward/back check, an
  A-button tap that sets `attack_buffer=60`, and a first tick that remains
  DownBounceU/Ground `68/59/0`.
- `STAGE_MPCLIFFWAIT_DAMAGE_DOWNWAIT`: bounded original DownWait setup/update
  diagnostics. Current proof records one DownWait `ftMainSetStatus` seam, one
  capture-immune setup, DownWaitU/Ground `70/-2/0`, `stand_wait=180`, capture
  mask `0x33`, `damage_mul=500` milli, installed DownWait callbacks, one
  original DownWait update tick with `stand_wait 180 -> 179`, zero DownStand
  status calls, and stable DownWaitU/Ground `70/-2/0`.
- `STAGE_MPCLIFFWAIT_DAMAGE_DOWNSTAND`: bounded original DownWait timeout into
  DownStand diagnostics. Current proof records one original DownWait
  stand-status request, one bounded DownStand `ftMainSetStatus` seam,
  DownStandU/Ground `72/61/0`, wakeup flag `1 -> 0`, installed DownStand
  callbacks, cleared `proc_damage`, restored `damage_mul=1000` milli, and
  `stand_wait` consumed to `0`.
- `STAGE_MPCLIFFATTACK_FLOOR`: CliffAttack result, safe result, proof mask,
  deferred mask, and selected fighter count. Current pass values are
  `0x46434150`, `0x46434153`, mask `0x7ff`, deferred mask `0xff`, and count
  `1`.
- `STAGE_MPCLIFFATTACK_FLOOR_CALLS`: prepared flag, base CliffWait proof seen
  flag, CliffWait interrupt call count, original cliff attack check count,
  original cliff escape check count, original cliff climb-or-fall check count,
  guarded CliffQuick/CliffSlow status-set count, and animation-event count.
  Current proof records one interrupt, one attack check, one status set, one
  animation-event call, and zero escape/climb-or-fall calls.
- `STAGE_MPCLIFFATTACK_FLOOR_STATUS`: before/after status, motion, and
  ground-air state; retained before/after cliff ID; queued cliff-motion status
  ID and cliff ID; cliff-hold and allow-interrupt flags; injected button tap,
  A-button mask, fall-wait before/after, and damage-fall count. Current proof
  records CliffWait/Ground `85/73/0 ->` CliffQuick/Ground `86/74/0`,
  retained `cliff_id=3`, queued `AttackQuick`, A-button tap `0x8000`, and no
  damage-fall call.
- `STAGE_MPCLIFFATTACK_FLOOR_SAFE`: unsafe count, B-button mask, proc-damage
  setup flag after the interrupt, and the previous CliffWait fall-wait value.
  Current proof records unsafe count `0`, nonzero B mask, proc-damage setup,
  and unchanged fall-wait across the guarded attack interrupt.
- `STAGE_MPCLIFFATTACK_ACTION`: CliffAttack action result, safe result, proof
  mask, deferred mask, and selected fighter count. Current pass values are
  `0x46434155`, `0x46434156`, mask `0xfff`, deferred mask `0xff`, and count
  `1`.
- `STAGE_MPCLIFFATTACK_ACTION_CALLS`: prepared flag, base CliffAttack setup
  proof seen flag, original CliffQuick update count, guarded
  CliffAttackQuick1 status-set count, original CliffAttackQuick1 update count,
  anim-end check count, guarded CliffAttackQuick2 status-set count, original
  common2 collision-data update count, original common2 status-var init count,
  and unsafe count. Current proof records one call for every required
  source-order boundary and unsafe count `0`.
- `STAGE_MPCLIFFATTACK_ACTION_STATUS`: before/after Quick1/after Quick2
  status, motion, and ground-air state; retained cliff and floor IDs; queued
  cliff-motion status; cliff-hold and proc-damage flags; Quick2 update/map
  callback flags; and jostle-ignore setup. Current proof records
  CliffQuick/Ground `86/74/0 ->` CliffAttackQuick1/Ground `92/80/0 ->`
  CliffAttackQuick2/Ground `93/81/0`, retained `cliff_id=3`, and
  copied `floor_line_id=3`.
- `STAGE_MPCLIFFATTACK_ACTION_FLAGS`: cliff-hold after Quick1, cliff-hold
  after Quick2, proc-damage flag, Quick2 update callback flag, Quick2 map
  callback flag, and jostle-ignore flag. The current verifier requires
  Quick1 to retain cliff hold, Quick2 to clear cliff hold through the bounded
  original-compatible status reset, and the selected Quick2 callbacks to be
  installed.
- `STAGE_MPCLIFFATTACK_ACTION_ROOT`: common2 bridge call count, guard pass
  count, guard reject count, queued cliff-motion status ID, LR, cliff ID, root
  X/Y before, root X/Y after, expected root X/Y, after-call floor distance,
  and root-position OK flag. The current verifier requires one guarded bridge
  pass, zero rejects, status ID `1`, a valid cliff ID, a changed root, after
  root equal to expected root within one milli-unit, floor distance near zero,
  and root-position OK `1`.
- `STAGE_MPCLIFFCOMMON2`: CliffCommon2 result, safe result, proof mask,
  deferred mask, and selected fighter count. Current pass values are
  `0x46433250`, `0x46433253`, mask `0x1ff`, deferred mask `0xff`, and count
  `1`.
- `STAGE_MPCLIFFCOMMON2_CALLS`: prepared flag, base CliffAttack action proof
  seen flag, original common2 update count, anim-end check count,
  Wait-or-Fall fallback count, original common2 physics count, guarded ground
  velocity transfer count, original common2 map count, guarded edge-break map
  count, and unsafe count. Current proof records one update, anim-end check,
  physics, ground transfer, map, and edge-break guard, zero Wait/Fall fallback,
  and unsafe count `0`.
- `STAGE_MPCLIFFCOMMON2_STATUS`: before/update/physics/map status, motion, and
  ground-air state. Current proof records CliffAttackQuick2/Ground
  `93/81/0` preserved across update, physics, and map callbacks.
- `STAGE_MPCLIFFCOMMON2_FLAGS`: before/after cliff ID, before/after floor line,
  update/physics/map proc pointer flags, and after-map cliff-hold. Current
  proof requires `cliff_id=3`, `floor_line_id=3 -> 3`, all selected Quick2
  callbacks to match the imported original function pointers, and cliff hold
  to remain clear after the common2 map tick.
- `STAGE_MPCLIFFESCAPE_ACTION`: CliffEscape action result, safe result, proof
  mask, deferred mask, and selected fighter count. Current pass values are
  `0x46434550`, `0x46434553`, mask `0x3fff`, deferred mask `0xff`, and count
  `1`.
- `STAGE_MPCLIFFESCAPE_ACTION_CALLS`: prepared flag, base CliffWait proof seen
  flag, original CliffWait interrupt count, original attack check count,
  original escape check count, original climb/fall check count, guarded
  CliffQuick status-set count, animation-event count, original CliffQuick
  update count, guarded CliffEscapeQuick1 status-set count, original
  CliffEscapeQuick1 update count, anim-end check count, guarded
  CliffEscapeQuick2 status-set count, original common2 status-var init count,
  and unsafe count. Current proof records one call for every required
  source-order boundary except climb/fall, which stays `0`; unsafe count also
  stays `0`.
- `STAGE_MPCLIFFESCAPE_ACTION_STATUS`: before/after interrupt/after Quick1/
  after Quick2 status, motion, and ground-air state. Current proof records
  CliffWait/Ground `85/73/0 ->` CliffQuick/Ground `86/74/0 ->`
  CliffEscapeQuick1/Ground `96/84/0 ->` CliffEscapeQuick2/Ground `97/85/0`.
- `STAGE_MPCLIFFESCAPE_ACTION_LEDGE`: retained cliff IDs through interrupt,
  Quick1, and Quick2; floor line after Quick2; queued cliff-motion status ID
  and cliff ID. Current proof requires retained `cliff_id=3`, copied
  `floor_line_id=3`, and queued EscapeQuick status ID `2`.
- `STAGE_MPCLIFFESCAPE_ACTION_FLAGS`: interrupt cliff-hold, Quick1 cliff-hold,
  Quick2 cliff-hold, allow-interrupt before/after, proc-damage at interrupt
  and Quick1, proc-update at interrupt and Quick1, Quick2 proc-map,
  jostle-ignore, fall-wait before/after interrupt, damage-fall count,
  injected Z-button tap, Z-button mask, and A-button mask. Current proof
  requires the Z tap to match the Z mask, differ from the A mask, keep
  fall-wait nonzero, avoid damage-fall, and clear cliff hold after Quick2.
- `STAGE_MPCLIFFESCAPE_ACTION_ROOT`: same common2 bridge/root contract as
  `STAGE_MPCLIFFATTACK_ACTION_ROOT`, but with queued cliff-motion status ID
  `2` for EscapeQuick.
- `STAGE_MPCLIFFESCAPE_COMMON2`: CliffEscape Common2 result, safe result,
  proof mask, deferred mask, and selected fighter count. Current pass values
  are `0x46453250`, `0x46453253`, and count `1`.
- `STAGE_MPCLIFFESCAPE_COMMON2_CALLS`: prepared flag, base CliffEscape action
  proof seen flag, original common2 update count, anim-end check count,
  Wait-or-Fall fallback count, original common2 physics count, guarded ground
  velocity transfer count, original common2 map count, guarded edge-break map
  count, and unsafe count. Current proof records one update, physics, and map
  call; fallback and unsafe counts stay `0`.
- `STAGE_MPCLIFFESCAPE_COMMON2_STATUS`: before/after update/after physics/after
  map status, motion, and ground-air state. Current proof records
  CliffEscapeQuick2/Ground `97/85/0` preserved across update, physics, and map
  callbacks.
- `STAGE_MPCLIFFESCAPE_COMMON2_FLAGS`: before/after cliff ID, before/after
  floor line, update/physics/map proc pointer flags, and after-map cliff-hold.
  Current proof requires retained `cliff_id=3`, `floor_line_id=3 -> 3`, all
  selected Quick2 callbacks to match the imported original function pointers,
  and cliff hold to remain clear after the common2 map tick.
- `STAGE_MPCLIFFCLIMB_FLOOR`: CliffClimb floor result, safe result, proof mask,
  deferred mask, and selected fighter count. Current pass values are
  `0x46434c50`, `0x46434c53`, mask `0x7fff`, deferred mask `0xff`, and count
  `1`.
- `STAGE_MPCLIFFCLIMB_FLOOR_CALLS`: prepared flag, base CliffWait proof seen
  flag, original CliffWait interrupt count, original attack check count,
  original escape check count, original climb/fall check count, guarded
  CliffQuick status-set count, guarded Fall status-set count, animation-event
  count, and unsafe count. Current proof records two interrupt/check passes:
  one climb branch and one drop branch.
- `STAGE_MPCLIFFCLIMB_FLOOR_BASE`: before status, motion, ground-air state,
  cliff ID, facing, fall-wait, and allow-interrupt flag. Current proof starts
  from CliffWait/Ground `85/73/0` on retained Pupupu `cliff_id=3` with LR
  `-1`.
- `STAGE_MPCLIFFCLIMB_FLOOR_CLIMB`: climb stick X/Y, after status, motion,
  ground-air state, after cliff ID, queued cliff-motion status ID, queued cliff
  ID, cliff-hold flag, proc-damage flag, allow-interrupt flag, and fall-wait.
  Current proof requires upward stick input, CliffQuick/Ground `86/74/0`,
  retained `cliff_id=3`, queued climb metadata, and unchanged fall-wait.
- `STAGE_MPCLIFFCLIMB_FLOOR_DROP`: drop stick X/Y, after status, motion,
  ground-air state, after cliff ID, after fall-wait, after `cliffcatch_wait`,
  cliff-hold flag, proc-callback setup flag, and damage-fall count. Current
  proof requires away-stick input, Fall/Air `26/20/1`, retained `cliff_id=3`,
  `cliffcatch_wait=30`, callback setup, zero damage-fall, and cliff hold
  cleared by the bounded status reset.
- `STAGE_MPCLIFFCLIMB_FLOOR_RECATCH`: recatch probe count, recatch map
  callback count, ceil-heavy/cliff check count, special-collision count,
  left/right cliff test counts, left/right cliff hit counts, landing-param
  count, CliffCatch status count, `ftMainSetStatus` count, and occupancy-block
  count. Current proof requires one bounded recatch pass, one right-cliff hit,
  one landing/status handoff, and zero occupancy block.
- `STAGE_MPCLIFFCLIMB_FLOOR_RECATCH_STATUS`: former holder cliff-hold flag,
  former holder status/motion/ground-air/cliff ID, recatcher before
  status/motion/ground-air, recatcher after status/motion/ground-air,
  recatcher cliff-hold flag, recatcher cliff ID, recatcher facing, and current
  / status collision masks. Current proof records the dropped former holder as
  Fall/Air `26/20/1` with hold `0`, then the recatcher as
  CliffCatch/Air `84/72/1` with hold `1` on the same Pupupu right ledge.
- `STAGE_MPCLIFFCLIMB_ACTION`: CliffClimb action result, safe result, proof
  mask, deferred mask, and selected fighter count. Current pass values are
  `0x46434c41`, `0x46434c55`, mask `0xfff`, deferred mask `0xff`, and count
  `1`.
- `STAGE_MPCLIFFCLIMB_ACTION_CALLS`: prepared flag, base CliffClimb floor proof
  seen flag, original CliffQuick update count, guarded CliffClimbQuick1 status
  count, original CliffClimbQuick1 update count, guarded anim-end check count,
  guarded CliffClimbQuick2 status count, common2 collision-data update count,
  common2 init-vars count, and unsafe count. Current proof requires one each
  for Quick update, Quick1 setup/update, anim-end, Quick2 setup, common2
  update, and common2 init; unsafe stays `0`.
- `STAGE_MPCLIFFCLIMB_ACTION_STATUS`: before, after Quick1, and after Quick2
  status/motion/ground-air triples, before/after cliff IDs, final floor line,
  queued climb status ID, and queued cliff ID. Current proof records
  `86/74/0 -> 87/75/0 -> 88/76/0` on retained `cliff_id=3`, final
  `floor_line_id=3`, queued climb status `0`, and queued cliff `3`.
- `STAGE_MPCLIFFCLIMB_ACTION_FLAGS`: cliff-hold flag after Quick1, cliff-hold
  flag after Quick2, proc-damage flag after Quick1, proc-update flag after
  Quick1, proc-map flag after Quick2, and jostle-ignore flag after Quick2.
  Current proof requires Quick1 cliff hold to be `1`, Quick2 cliff hold to be
  `0`, and the proc/jostle flags to be installed.
- `STAGE_MPCLIFFCLIMB_ACTION_ROOT`: same common2 bridge/root contract as
  `STAGE_MPCLIFFATTACK_ACTION_ROOT`, but with queued cliff-motion status ID
  `0` for ClimbQuick.
- `STAGE_MPCLIFFCLIMB_COMMON2`: CliffClimb common2 result, safe result, proof
  mask, deferred mask, and selected fighter count. Current pass values are
  `0x464c3250`, `0x464c3253`, mask `0x1ff`, deferred mask `0xff`, and count
  `1`.
- `STAGE_MPCLIFFCLIMB_COMMON2_CALLS`: prepared flag, base CliffClimb action
  proof seen flag, original common2 update count, guarded anim-end check count,
  guarded Wait/Fall fallback count, original common2 physics count, guarded
  ground velocity transfer count, original common2 map count, guarded
  ground-break map callback count, and unsafe count. Current proof requires
  one update, one anim-end check, zero Wait/Fall fallback, one physics tick,
  one ground velocity transfer, one map tick, one ground-break callback, and
  zero unsafe count.
- `STAGE_MPCLIFFCLIMB_COMMON2_STATUS`: before, after update, after physics,
  and after map status/motion/ground-air triples. Current proof requires
  CliffClimbQuick2/Ground `88/76/0` to remain stable through all three
  callbacks.
- `STAGE_MPCLIFFCLIMB_COMMON2_FLAGS`: before cliff ID, before floor line,
  after-map cliff ID, after-map floor line, proc-update/proc-physics/proc-map
  flags, and after-map cliff-hold. Current proof requires retained
  `cliff_id=3`, retained `floor_line_id=3`, all three callback pointers set,
  and cliff hold to remain clear after the common2 map tick.
- `STAGE_MPCLIFFCLIMB_FINISH`: CliffClimb finish result, safe result, proof
  mask, deferred mask, and selected fighter count. Current pass values are
  `0x46434650`, `0x46434653`, mask `0x3ff`, deferred mask `0xff`, and count
  `1`.
- `STAGE_MPCLIFFCLIMB_FINISH_CALLS`: prepared flag, base CliffClimb common2
  proof seen flag, original common2 update count, guarded anim-end check
  count, guarded Wait/Fall fallback count, bounded Wait status-set count,
  player-tag wait count, special interrupt restore count, and unsafe count.
  Current proof requires one update, one anim-end check, one Wait/Fall
  fallback, one Wait status-set, one player-tag wait, one special-interrupt
  restore, and zero unsafe count.
- `STAGE_MPCLIFFCLIMB_FINISH_STATUS`: before and after status/motion/
  ground-air triples, before/after cliff IDs, and before/after floor lines.
  Current proof records CliffClimbQuick2/Ground `88/76/0 ->` Wait/Ground
  `10/4/0`, retained `cliff_id=3`, and stable `floor_line_id=3`.
- `STAGE_MPCLIFFCLIMB_FINISH_FLAGS`: before/after cliff ID, before/after
  floor line, after-Wait cliff-hold flag, original common2 update proc flag,
  bounded Wait proc-state flag, special-interrupt restored flag, after-Wait
  player-tag wait, after-Wait jostle-ignore flag, and common status reset mask.
  Current proof requires retained `cliff_id=3`, stable `floor_line_id=3`,
  cliff hold clear, jostle ignore clear, both proc flags set, restored special
  interrupt, `playertag_wait=120`, and `(reset_mask & 0x7ffff) == 0x7ffff`.
  That mask proves the current project-owned `ftMainSetStatus` reset cleared
  reflect, absorb, shield, fastfall, invisible, shadow hide, player-tag hide,
  ghost, hitstun, cliff hold, jostle ignore, damage player, ignored line,
  capture immune mask, camera zoom range, shuffle tics, knockback resistance,
  stacked knockback, and damage multiplier.

Current proof:

```text
Battle Mario/Fox gcDrawAll-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1
Menu-chain Mario/Fox gcDrawAll-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1
Battle Mario/Fox stage gcDrawAll-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0
Menu-chain Mario/Fox stage gcDrawAll-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0
Battle Mario/Fox stage collision-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=0/0 floorRange=0-4 floorLines=4 probes=3/2
Menu-chain Mario/Fox stage collision-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=0/0 floorRange=0-4 floorLines=4 probes=3/2
Battle Mario/Fox stage floor-follow-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=0/0 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=0/0 updates=18/18 drift=0/0 visits=0x1/0x1
Menu-chain Mario/Fox stage floor-follow-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=0/0 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=0/0 updates=18/18 drift=0/0 visits=0x1/0x1
Battle Mario/Fox stage floor-edge-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=118750/42000 delta=137250/214000 probes=2/2 queries=6/4
Menu-chain Mario/Fox stage floor-edge-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=118750/42000 delta=137250/214000 probes=2/2 queries=6/4
Battle Mario/Fox stage MP floor-process-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=118750/42000 delta=137250/214000 probes=2/2 queries=50/46 mpProcessFloor=test=38/2 project=0/2 probes=1/2 below=2 p0line=3 p1line=3 fc=2/1/39
Menu-chain Mario/Fox stage MP floor-process-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=118750/42000 delta=137250/214000 probes=2/2 queries=50/46 mpProcessFloor=test=38/2 project=0/2 probes=1/2 below=2 p0line=3 p1line=3 fc=2/1/39
Battle Mario/Fox stage mpProcessUpdateMain floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=90/85 mpProcessFloor=test=42/3 project=0/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/36/42 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000
Menu-chain Mario/Fox stage mpProcessUpdateMain floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=90/85 mpProcessFloor=test=42/3 project=0/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/36/42 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000
Battle Mario/Fox stage MP sweep floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=90/85 mpProcessFloor=test=42/3 project=0/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/36/42 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000 mpSweepFloor=check=1/36 same=35/1 diff=1 line=3->0 second=34/34 p0line=3 p1line=3
Menu-chain Mario/Fox stage MP sweep floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=90/85 mpProcessFloor=test=42/3 project=0/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/36/42 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000 mpSweepFloor=check=1/36 same=35/1 diff=1 line=3->0 second=34/34 p0line=3 p1line=3
Battle Mario/Fox Stage MP cross-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=138/133 mpProcessFloor=test=25/20 project=17/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/83/43 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000 mpSweepFloor=check=18/19 same=18/1 diff=18 line=3->0 second=34/17 p0line=3 p1line=3 mpCrossFloor=line=-1->3 live=17/17 accepted=17 p0line=3 p1line=3
Menu-chain Mario/Fox Stage MP cross-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=138/133 mpProcessFloor=test=25/20 project=17/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/83/43 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000 mpSweepFloor=check=18/19 same=18/1 diff=18 line=3->0 second=34/17 p0line=3 p1line=3 mpCrossFloor=line=-1->3 live=17/17 accepted=17 p0line=3 p1line=3
Battle Mario/Fox Stage MP adjust-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=138/133 mpProcessFloor=test=25/20 project=17/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/83/43 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000 mpSweepFloor=check=18/19 same=18/1 diff=18 line=3->0 second=34/17 p0line=3 p1line=3 mpCrossFloor=line=-1->3 live=17/17 accepted=17 p0line=3 p1line=3 mpAdjustFloor=run=17 check=17/17 wallMiss=17/17 edge=17/17 noAdjust=17 p0line=3 p1maintained=3
Menu-chain Mario/Fox Stage MP adjust-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 stageCollision=line=3/3 floorRange=0-4 floorLines=4 probes=3/2 floorFollow=line=3/3 updates=18/18 drift=0/0 visits=0x8/0x8 floorEdge=line=3 width=4636000 dist=97000/97000 delta=159000/159000 probes=2/2 queries=138/133 mpProcessFloor=test=25/20 project=17/3 probes=1/2 below=2 p0line=3 p1line=3 fc=3/83/43 mpUpdateFloor=steps=39/2 split=1 probes=1/1/1 p0line=3 p1line=3 posdiff=-1000/1000 mpSweepFloor=check=18/19 same=18/1 diff=18 line=3->0 second=34/17 p0line=3 p1line=3 mpCrossFloor=line=-1->3 live=17/17 accepted=17 p0line=3 p1line=3 mpAdjustFloor=run=17 check=17/17 wallMiss=17/17 edge=17/17 noAdjust=17 p0line=3 p1maintained=3
Battle Mario/Fox Stage MP edge-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpAdjustFloor=run=17 checkHit=0/0 checkMiss=17/17 wallHit=0/0 wallMiss=17/17 edge=18/18 adjust=0/0 noAdjust=17 mpEdgeFloor=edge=6/5 kind=3/2 calls=18/18
Menu-chain Mario/Fox Stage MP edge-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpAdjustFloor=run=17 checkHit=0/0 checkMiss=17/17 wallHit=0/0 wallMiss=17/17 edge=18/18 adjust=0/0 noAdjust=17 mpEdgeFloor=edge=6/5 kind=3/2 calls=18/18
Battle Mario/Fox Stage MP wall-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpAdjustFloor=run=17 checkHit=0/0 checkMiss=17/17 wallHit=0/0 wallMiss=17/17 edge=18/18 adjust=0/0 noAdjust=17 mpEdgeFloor=edge=6/5 kind=3/2 calls=18/18 mpWallFloor=blocked floor=3 edgeWall=5 kind=2 side=1 candidates=2 miss=1 mpWallHitScout=none floors=4 walls=8 candidates=6 hyruleWallHit=hit floor=5 wall=13 edge=12 side=0 delta=-1600/-388
Menu-chain Mario/Fox Stage MP wall-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 final=Wait/Ground safe=1 mpAdjustFloor=run=17 checkHit=0/0 checkMiss=17/17 wallHit=0/0 wallMiss=17/17 edge=18/18 adjust=0/0 noAdjust=17 mpEdgeFloor=edge=6/5 kind=3/2 calls=18/18 mpWallFloor=blocked floor=3 edgeWall=5 kind=2 side=1 candidates=2 miss=1 mpWallHitScout=none floors=4 walls=8 candidates=6 hyruleWallHit=hit floor=5 wall=13 edge=12 side=0 delta=-1600/-388
Battle Mario/Fox Stage MP stale-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpWallFloor=blocked floor=3 edgeWall=5 kind=2 side=1 candidates=2 miss=1 mpStaleFloor=line=1->0 live=18/0 accepted=1 p0line=0 p1line=3
Menu-chain Mario/Fox Stage MP stale-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpWallFloor=blocked floor=3 edgeWall=5 kind=2 side=1 candidates=2 miss=1 mpStaleFloor=line=1->0 live=18/0 accepted=1 p0line=0 p1line=3
Battle Mario/Fox Stage MP live-stale-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpWallFloor=blocked floor=3 edgeWall=5 kind=2 side=1 candidates=2 miss=1 mpStaleFloor=line=1->0 live=1/0 accepted=1 p0line=0 p1line=3 mpLiveStaleFloor=line=1->0 selected=1 live=1/0 accepted=1 p0line=0 p1line=3
Menu-chain Mario/Fox Stage MP live-stale-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=61/-62 screen-rise=52/52 final=Wait/Ground safe=1 mpWallFloor=blocked floor=3 edgeWall=5 kind=2 side=1 candidates=2 miss=1 mpStaleFloor=line=1->0 live=1/0 accepted=1 p0line=0 p1line=3 mpLiveStaleFloor=line=1->0 selected=1 live=1/0 accepted=1 p0line=0 p1line=3
Battle Mario/Fox Stage MP motion-stale-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000
Menu-chain Mario/Fox Stage MP motion-stale-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000
Battle Mario/Fox Stage MP cliff-status-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3
Menu-chain Mario/Fox Stage MP cliff-status-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3
Battle Mario/Fox Stage MP cliff-tick-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3
Menu-chain Mario/Fox Stage MP cliff-tick-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3
Battle Mario/Fox Stage MP Fall-map-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3 mpFallMap=phys=1 fast=1 grav=1 map=1 noColl=1 y=200000->194000
Menu-chain Mario/Fox Stage MP Fall-map-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3 mpFallMap=phys=1 fast=1 grav=1 map=1 noColl=1 y=200000->194000
Battle Mario/Fox Stage MP Fall-landing-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3 mpFallMap=phys=1 fast=1 grav=1 map=1 noColl=1 y=200000->194000 mpFallLand=phys=1 map=1 floor=1 land=1/0 status=26->31/25 y=4000->0 velY=-8000->-12000
Menu-chain Mario/Fox Stage MP Fall-landing-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3 mpFallMap=phys=1 fast=1 grav=1 map=1 noColl=1 y=200000->194000 mpFallLand=phys=1 map=1 floor=1 land=1/0 status=26->31/25 y=4000->0 velY=-8000->-12000
Battle Mario/Fox Stage MP Ceiling-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3 mpFallMap=phys=1 fast=1 grav=1 map=1 noColl=1 y=200000->194000 mpFallLand=phys=1 map=1 floor=1 land=1/0 status=26->31/25 y=4000->0 velY=-8000->-12000 mpCeilFloor=line=4 kind=1 check=1/1 diff=1/1 fc=2/2 y=-1464000->-1472000 ceil=-1072000 dist=-8000
Menu-chain Mario/Fox Stage MP Ceiling-floor-loop harness passed: scene=22/21 gcRunAll=33 gcDrawAll=7 display=42/42 draws=7 pixels=930 visits=0x3ff/0x3ff transitions=0x7ff/0x7ff screen-dx=-22/-62 screen-rise=52/52 final=Wait/Ground safe=1 stageCapture=0xf/0xf stageDObj=0xf/0xf stageDL=0xf/0xf stagePixels=930 compat=0x0 mpMotionStaleFloor=line=1->0 mutation=1 update=1 match=1 p0line=0 p1line=3 pos=-293000,1574000->-285000,1542000 mpCliffStatus=ottotto=36/30 fall=26/20 branches=1/1 air=1 lines=0/3 mpCliffTick=ottTick=1/1/1 fallInt=1 airChecks=1/1/1 lines=0/3 mpFallMap=phys=1 fast=1 grav=1 map=1 noColl=1 y=200000->194000 mpFallLand=phys=1 map=1 floor=1 land=1/0 status=26->31/25 y=4000->0 velY=-8000->-12000 mpCeilFloor=line=4 kind=1 check=1/1 diff=1/1 fc=2/2 y=-1464000->-1472000 ceil=-1072000 dist=-8000
```

This proves only selected Mario/Fox display callbacks reached through bounded
original `gcDrawAll()` after masking/guarding non-target display callbacks. It
also proves the selected Pupupu display-layer and map GObjs are captured by the
same camera path and reach DS-owned DObj draw bridges, and that the opt-in
stage-collision modes project the proof-owned fighters against real Pupupu
floor geometry instead of the older flat compatibility seam. The collision
proof re-centers the proof-owned roots from decoded Pupupu floor endpoints
before the final projection sample. The floor-follow modes then remove that
final re-center/adopt shortcut and prove selected Mario/Fox roots are projected
and clamped to the real decoded Pupupu floor during the bounded movement slice.
The floor-edge modes then seed the fighters near the widest decoded floor line,
prove inside/outside floor queries through original-compatible MP helpers, and
record edge-under calls as deferred instead of faking ledge/wall behavior. The
MP floor-process modes then run a bounded source-order floor-only
`mpprocess.c` slice through an `MPCollData` adapter and copy floor results back
to the selected `FTStruct` collision shells. The MP update-main modes build on
that proof by routing selected Mario/Fox map callbacks through bounded
source-order `mpProcessUpdateMain`, preserving the original reset-to-previous
position and substep callback order, and proving a floor-only
`mpCommonRunFighterAllCollisions` callback path. The MP sweep modes replace
the previous second-floor-test deferral with bounded same-line/different-line
floor sweep helpers and `mpProcessCheckTestFloorCollision`; the MP cross modes
then prove a live selected-callback P0 `-1 -> 3` accepted second-floor path
while keeping P1 as a retained-floor control. The MP adjust modes then prove
source-order `mpProcessRunFloorEdgeAdjust` reaches left/right floor-edge
checks and wall-line sweep misses while the old floor-edge-adjust deferred
counter stays zero. The MP edge modes resolve decoded Pupupu edge-under wall
lines `6/5`, and the MP wall modes prove the current selected Dream Land floor
has no usable real wall-hit adjustment because both side-wall candidates are
those same edge-under lines rejected by original
`mpProcessCheckFloorEdgeCollisionL/R`. The same MP wall modes now also stage an
isolated Hyrule Castle scout and identify a concrete source-order hit candidate
at floor `5`, wall `13`, edge-under `12`, side `0`, with delta
`-1600/-388`. The MP wall-hit modes `135/136` promote that Hyrule candidate
into a direct/menu-chain proof while still inheriting the cliff-live proof and
keeping the live scene on Pupupu. The MP wall-copy modes `137/138` remain
regression coverage: they consume the wall-hit proof, run a proof-owned
selected P0 process, copy the adjusted Hyrule result back into the live P0
root/collision shell, and prove P1 remains unchanged. The MP pass-through
modes `139/140` are the current direct/menu-chain boundary: they consume
wall-copy, prove Dream Land line `0` carries `MAP_VERTEX_COLL_PASS` flags
`0x4000`, route through `MAP_PROC_TYPE_PASS`, reject same-line
`ignore_line_id`, accept the different-line probe through the pass callback
gate, and keep P1 unchanged. The MP stale modes add a
finalizer-local valid-stale second-floor source-order probe from Dream Land
line `1` to line `0`, accepting the target floor and reaching landing-floor,
floor-edge-adjust, and collision-end-clear evidence without moving the live
wall/edge proof roots. The MP live-stale modes then run the same valid-stale
pair from the selected P0 callback path as a contained local source-order
`MPCollData` pass, preserving the real Mario/Fox movement loop on decoded
floor line `3/3`. The MP motion-stale modes then seed the same valid-stale
pair into the selected P0 root/collision shell and copy target line `0` back
to live state. The MP cliff-status modes then import original
`ftcommonottotto.c` and prove a
bounded source-order `mpCommonProcFighterOnCliffEdge` branch: P0 with
`MAP_FLAG_FLOOREDGE` enters Ottotto, while P1 without that flag enters Fall.
The MP cliff-tick modes then consume those created states under guards: P0
runs one original Ottotto update/interrupt/map tick and remains
Ottotto/Ground on line `0`, while P1 runs one original Fall interrupt tick,
reaches the three guarded air interrupt checks, and remains Fall/Air on line
`3`.
The MP Fall-map modes then consume that P1 Fall state under guards: P1 runs
the selected status-table original Fall physics callback, executes one bounded
airborne integration step, reaches the selected Fall map callback through a
guarded no-collision branch, and remains Fall/Air on line `3`.
This still does not prove
unpaused full-scene draw traversal, DS hardware polygon rendering,
camera-correct battle matrices, full fighter display, full map collision
processing, real wall-hit floor-edge adjustment on a non-edge wall candidate,
natural-motion landing, natural-motion cliff/offstage detection, continuous
Fall/Ottotto runtime,
platforms, ledges, walls, ceilings, cliffcatch, attacks, specials, guard,
catch, items, HUD, audio, or full imported `ftmain.c`.

## Stage MP DownRecover Loop Markers

The `battle_mariofox_stage_mpdownrecover_loop` and
`menu_chain_mariofox_stage_mpdownrecover_loop` verifiers extend the Turn-loop
boundary with a bounded face-down downed recovery proof. The marker group is
emitted only when the verifier passes `-RequireStageMPDownRecoverLoop`.

- `STAGE_MPDOWNRECOVER` records the top-level result, safe result, pass mask,
  deferred mask, and proof count. A passing proof reports
  `0x46445250`, `0x46445253`, mask `0xff`, deferred mask `0xff`, and count
  `1`.
- `STAGE_MPDOWNRECOVER_SETUP` records DownWaitD setup: prepared/base-Turn
  seen, DownWait set-status counts, capture-immune count, status `69`, motion
  `-2`, ground state, stand-wait `180`, callback readiness, and unsafe count.
- `STAGE_MPDOWNRECOVER_DOWNSTAND` records the original DownWait interrupt path
  into DownStandD. It expects one interrupt/check/status path, source-order
  marker `0x12345`, status/motion `71/60`, ground state, and valid callbacks.
- `STAGE_MPDOWNRECOVER_ATTACK` records the original DownAttackD branch. It
  expects source-order marker `0x1234`, status/motion `79/68`, ground state,
  DownAttackD attack IDs `52/32/32`, and valid callbacks.
- `STAGE_MPDOWNRECOVER_ROLL` records the original DownForwardD and DownBackD
  branches. It expects two interrupts/checks/status paths, source-order marker
  `0x12345` for both branches, forward status/motion `75/64`, back
  status/motion `77/66`, ground state, and valid callbacks.
- `STAGE_MPDOWNRECOVER_FINAL` records the four original animation-end Wait
  handoffs. It expects four Wait status sets, four player-tag waits, final
  status/motion/callback masks `0xf`, and unsafe count `0`.

This proves only the bounded D-state recovery branch selection and Wait/Ground
handoffs. It does not prove continuous downed action runtime, hitboxes,
invulnerability timing, real opponent interactions, full damage state, or
unbounded gameplay scheduling.

## Stage MP Cliff-Ledge Loop Markers

The `battle_mariofox_stage_mpcliffledge_loop` and
`menu_chain_mariofox_stage_mpcliffledge_loop` verifiers aggregate the existing
bounded cliff-catch, CliffWait, CliffClimb, CliffClimbQuick2 finish, and
DownRecover proofs. The marker group is emitted only when the verifier passes
`-RequireStageMPCliffLedgeLoop`.

- `STAGE_MPCLIFFLEDGE` records the top-level result, safe result, pass mask,
  deferred mask, and proof count. A passing proof reports `0x464c4750`,
  `0x464c4753`, mask `0x3ff`, deferred mask `0xff`, and count `1`.
- `STAGE_MPCLIFFLEDGE_BASE` records prepared/base proof state plus same-cliff
  occupancy evidence. A passing proof has DownRecover, CliffCatch, CliffClimb,
  and CliffClimbFinish bases all seen, occupancy block count `1`, and matching
  holder/probe cliff IDs on Pupupu line `3`.
- `STAGE_MPCLIFFLEDGE_STATE` records the aggregate state transitions: drop
  reaches Fall/Air `26/20/1`, `cliffcatch_wait=30`, cliff hold cleared,
  recatch reaches CliffCatch/Air `84/72/1` with cliff hold restored and no
  occupancy block, and the climb finish reaches Wait/Ground `10/4/0`.
- `STAGE_MPCLIFFLEDGE_LINES` records line continuity across drop, recatch, and
  climb finish. The current proof expects line `3/3/3`.

This proves a bounded source-order ledge spine aggregation. It does not prove
unbounded ledge runtime, real opponent ledge contention over many frames,
ledge invulnerability/timing, broad platform/ledge contracts, attacks from
ledge, or full gameplay scheduling.

## Stage MP Cliff-Live Loop Markers

The `battle_mariofox_stage_mpclifflive_loop` and
`menu_chain_mariofox_stage_mpclifflive_loop` verifiers extend the aggregate
cliff-ledge proof with a selected proof-owned P0 `GObjProcess` path. The marker
group is emitted only when the verifier passes `-RequireStageMPCliffLiveLoop`.

- `STAGE_MPCLIFFLIVE` records the top-level result, safe result, pass mask,
  deferred mask, and proof count. A passing proof reports `0x464c5650`,
  `0x464c5653`, mask `0x3ff`, deferred mask `0xff`, and count `1`.
- `STAGE_MPCLIFFLIVE_BASE` records that the aggregate cliff-ledge base passed,
  the live process attached, `gcRunGObjProcess` ran, selected callbacks ran,
  and source mask `0xfff` was reached.
- `STAGE_MPCLIFFLIVE_COUNTS` records one original callback step for Wait
  update, climb interrupt, drop interrupt, Quick update, Quick1 update,
  common2 update, common2 physics, common2 map, and finish update.
- `STAGE_MPCLIFFLIVE_STATUS` records the live status chain:
  CliffCatch/Air `84/72/1`, CliffWait/Ground `85/73/0`, queued climb kind `0`,
  fall wait `1080`, cliff line `3`, CliffQuick/Ground `86/74/0`,
  CliffClimbQuick1 `87/75`, and CliffClimbQuick2 `88/76` on floor line `3`.
- `STAGE_MPCLIFFLIVE_FINAL` records common2 staying `88/76/0`, the original
  common2 finish handoff to Wait/Ground `10/4/0`, and the later drop into
  Fall/Air `26/20/1`.
- `STAGE_MPCLIFFLIVE_DROP` records `cliffcatch_wait=30`, cliff hold cleared,
  and unsafe count `0`.

This proves the ledge wait/drop/climb path through a selected live callback
shell. It still does not unbound the full task loop, natural multiplayer ledge
arbitration, attacks, items, HUD, audio, or full gameplay.

## Stage Inishie Scale Loop Markers

The `battle_mariofox_stage_inishie_scale_loop` and
`menu_chain_mariofox_stage_inishie_scale_loop` verifiers are standalone
regression coverage. They keep the live VSBattle roots on Pupupu/Dream Land,
stage the read-only Inishie/Mushroom Kingdom scale payload, run source-backed
original `grInishieMakeScale`, then run bounded original
`grInishieScaleProcUpdate` calls. The marker group is emitted when the verifier
passes `-RequireStageInishieScaleLoop`.

- `STAGE_INISHIE_SCALE` records the top-level result, safe result, pass mask,
  source mask, and proof count. A passing current proof reports mask
  `0x7ff`, source mask `0xff`, and count `4`.
- `STAGE_INISHIE_SCALE_SETUP` records source setup reachability, scale DObj and
  collision-yakumono setup, line groups `1/2`, map object kinds `5/6`, and
  callback/DObj readiness masks.
- `STAGE_INISHIE_SCALE_SOURCE`, `STAGE_INISHIE_SCALE_DISPLAY`,
  `STAGE_INISHIE_SCALE_MATERIAL`, and `STAGE_INISHIE_SCALE_PREVIEW` prove the
  staged `StageInishieFile3` raw payload, native DL/Vtx conversion, clean
  source-created display-list scan (`sourceDL=0xff cmds=91 tris=20`), texture
  command state (`tex=0x3f`), and bounded software preview (`preview=0x3f`,
  `px=432`).
- `STAGE_INISHIE_SCALE_LINES` records scale line groups `1/2`, map object kinds
  `5/6`, and the main two-tick state values.
- `STAGE_INISHIE_SCALE_ALT` records the normal bounded two-tick Wait update:
  altitude `80000 -> 64000`, platform Y
  `363000/362000 -> 427000/298000`, and second-tick speeds `-8000/8000`.
- `STAGE_INISHIE_SCALE_FALL` records the forced threshold path. The current
  passing proof reports four fall-phase `mpCollisionSetYakumonoPosID` writes,
  two sparkle calls, status `1 -> 2`, altitude after Wait `1100000`,
  acceleration after Fall/Sleep `0`, final platform Y `1460000/-741000`, and
  speeds `-3000/-3000`.
- `STAGE_INISHIE_SCALE_STEP` records the bounded Sleep/Retract path. The
  current passing proof reports four retract-phase position writes, two
  platform re-enable calls, wait `1 -> 0`, status `3 -> 0`, retracted altitude
  `0`, restored platform Y `363000/362000`, and final retract speeds
  `-1097000/1103000`.

This proves bounded original scale setup/update and one forced
`Wait -> Fall -> Sleep -> Retract -> Wait` threshold path. It does not prove
continuous Mushroom Kingdom runtime, Pakkun, Power Block, item/monster
behavior, hardware texture upload/material application, or unbounded stage
scheduling.

## Debugging Principle

A passing probe proves only the boundary it checks. Do not use a narrow marker
as proof of a broad milestone. The startup object and taskman diagnostics now
prove imported `sys/taskman.c`, `sys/objman.c`, and `sys/objhelper.c` created
the initial actor/camera/wallpaper/logo relationships, startup heap/buffer
setup, bounded `gcRunAll` updates through the startup Opening Room request, and
the original taskman break/eject and cleanup paths. They also prove the original
scene-manager switch dispatched `mvOpeningRoomStartScene`, and the imported
Opening Room relocation setup, file-ID list, NitroFS O2R payload copying,
blanket word byte-swap, internal pointer-chain relocation, selected
symbol-offset probes, first tick-280 pencils asset-reference probes,
Scene 1/logo-camera camanim probes, logo, Outside, Haze, and sunlight asset-reference probes,
first pencils descriptor/animation table shape, tick-280 deferred fighter marker, original
`mvOpeningRoomMakeScene1Cameras`, original
`mvOpeningRoomMakeCloseUpOverlayCamera`, original
`mvOpeningRoomMakeWallpaperCamera`, original
`mvOpeningRoomMakeLogoCamera`, original `mvOpeningRoomMakeLogo`, original
`mvOpeningRoomMakePencils` object creation from inside `mvOpeningRoomFuncRun`,
original Scene 1 cameras, close-up overlay camera, wallpaper-camera,
logo-camera, logo, overlay, boss-shadow, Outside, Haze, and sunlight setup/ejection, and the
actor/camera/Scene1-camera/close-up-camera/wallpaper-camera/logo-camera/overlay/logo/boss-shadow/Outside/Haze/sunlight
slice reached and passed the tick-280 pencils object boundary. They also prove
the tick-380 fighter-status/rotation branch is explicitly deferred, the
tick-450 close-up overlay object is created through original object-manager
state and real sunlight ejection leaves original lists, the tick-500 spotlight
object is created through original object-manager/material-animation state
while pulled-fighter display-link movement is explicitly deferred, and the
tick-560 Scene 2 camera transition runs while Boss fighter status is explicitly
deferred. The bounded Opening Room draw marker proves the next renderer
boundary reaches original `gcDrawAll`, original `func_80017EC0`, the narrow DS
camera capture bridge, original DObj display callbacks, and the first
material-bearing DObj candidate, then proves that candidate's branch-expanded
display list can be parsed and rasterized by a deliberately narrow DS preview
path.
The skip verifier additionally proves imported
shared controller logic and the original Title transition. They do not prove
external relocation dependency recursion, texture/display-list fixups,
fighter/effect object initialization, pulled-fighter execution, remaining
room-object events, later camera/fighter-status events, continuous draw looping,
the continuous draw half of taskman, full `lbcommon` rendering, or DS
display-list translator work. The visual HUD now proves one bounded original
`N64Logo` Sprite preview path and one bounded material-bearing Opening Room
DObj display-list preview path. The runtime verifier also checks that
material-bearing candidate's first-MObj diagnostic contract, detached
original-shaped prim-color branch emission, branch-expanded `G_DL` walk, and
live bounded preview consumption for that selected path. The new `ORTX` marker
proves the current selected material path has no texture-bearing original
`MObj` source yet. It still does not prove the general original renderer, broad
material/combiner mapping, texture upload, z-buffering, or continuous draw
semantics.
