# Roadmap

This roadmap tracks milestone status. Keep detailed proof in `docs/STATUS.md`,
`docs/HANDOFF.md`, `docs/DIAGNOSTIC_REFERENCE.md`, and append-only history in
`docs/PORTING.md`.

Status labels:

- Done: verified in current ROM/runtime.
- In Progress: imported or started, but not enough for the next real
  gameplay/menu behavior.
- Deferred: known work, not started.

## Milestones

| # | Milestone | Status | Next Gate |
|---|---|---:|---|
| 1 | Clean devkitPro/libnds NDS project skeleton | Done | Keep `make -j16` green. |
| 2 | Add BattleShip as imported/reference source | Done | Keep `decomp/` read-only; import through `src/import`. |
| 3 | Identify original N64 entry/game-loop path | Done | Maintain `syMainLoop -> syMainThread5 -> scManagerRunLoop` evidence in docs. |
| 4 | DS platform layer like `sm64-nds/src/nds` | In Progress | Continue isolating DS behavior in `src/nds` and `src/port`. |
| 5 | Stub enough libultra/N64 platform functions | In Progress | Replace broad stubs only when the next original boundary needs them. |
| 6 | Boot an `.nds` ROM | Done | Keep melonDS/no$gba smoke paths working. |
| 7 | Run minimal original game-state/update loop | In Progress | Current loop reaches bounded Title setup, bounded VS setup, imported PlayersVS setup, imported Maps setup with Pupupu preview, direct bounded VSBattle setup, direct Pupupu ground object setup, guarded direct/menu-chain Pupupu safe update proofs, bounded asset-backed Mario/Fox model GObj creation, persistent FTStruct-backed Mario/Fox state shells, bounded source-order Mario/Fox init-state proof, bounded original Mario/Fox Wait status/motion setup, one bounded original Wait callback tick, a bounded Wait ground-friction/map pass, display metadata probes, parser-only Mario/Fox display-list scans, decode-only first-DL execute proofs, visible first-DL software draw proofs, visible multi-DL software draw proofs, guarded all-DL software draw proofs, bounded original movement/status proofs through Walk, Dash/Run/RunBrake, Jump/Fall/Landing, selected process/scheduler/controller-source loops, live-input idle, bounded original `gcRunAll`, bounded original `gcDrawAll` Mario/Fox moving-preview proofs, Pupupu stage-inclusive original `gcDrawAll` traversal proofs, direct/menu-chain geometry-backed Pupupu floor-collision proofs, direct/menu-chain continuous geometry-backed Pupupu floor-follow proofs, direct/menu-chain real Pupupu floor-edge / original MP floor-query proofs, direct/menu-chain source-order MP floor-process proofs, direct/menu-chain source-order `mpProcessUpdateMain` floor-loop proofs, direct/menu-chain source-order MP floor-line sweep / second-floor collision proofs, direct/menu-chain live cross-floor proofs, direct/menu-chain source-order MP floor-edge-adjust checks, direct/menu-chain source-order MP edge-under/floor-edge proofs, direct/menu-chain MP wall-blocker proofs, direct/menu-chain MP stale-valid second-floor proofs, direct/menu-chain selected-callback live-stale second-floor proofs, direct/menu-chain selected-callback/root motion-stale second-floor mutation/copyback proofs, direct/menu-chain source-order MP cliff-status branch proofs into original Ottotto/Fall status setup, direct/menu-chain bounded Ottotto/Fall callback-tick proofs, direct/menu-chain Fall physics/map no-collision proofs, direct/menu-chain Fall landing-floor proofs into original LandingLight/Ground, direct/menu-chain CliffCatch -> CliffWait proofs, direct/menu-chain CliffWait -> CliffQuick/AttackQuick setup proofs, direct/menu-chain CliffQuick -> CliffAttackQuick1 -> CliffAttackQuick2 action proofs, direct/menu-chain bounded original CliffAttackQuick2 common2 update/physics/map tick proofs, direct/menu-chain CliffWait -> CliffQuick -> CliffEscapeQuick1 -> CliffEscapeQuick2 action proofs, direct/menu-chain bounded original CliffEscapeQuick2 common2 update/physics/map tick proofs, direct/menu-chain CliffWait climb/drop interrupt proofs, direct/menu-chain CliffQuick -> CliffClimbQuick1 -> CliffClimbQuick2 action proofs, direct/menu-chain bounded original CliffClimbQuick2 common2 update/physics/map tick proofs, direct/menu-chain bounded original CliffClimbQuick2 animation-end handoff into Wait/Ground proofs, and direct/menu-chain bounded original CliffWait timeout into DamageFall/Air plus one DamageFall no-collision callback tick, one positive CliffCatch setup proof, one positive DownBounce setup proof, bounded DownBounce/DownWait/DownStand callback ticks, and a fresh bounded original DownWait interrupt proof covering DownAttackU, DownForwardU, DownBackU, and DownStandU, including DownAttackU attack IDs `53/33/33`, eight guarded stable callback frames plus Wait handoff for the DownAttack/roll branches, bounded roll root movement `+10000/-10000` milli, and guarded callback/Wait handoff for DownStand. |
| 8 | Render a simple placeholder frame | Done | Placeholder remains disabled behind original-preview paths. |
| 8a | Render one original Startup asset | Done | Preserve original `N64Logo` bounded SObj preview. |
| 8b | Render one original Opening Room DObj slice | Done | Preserve bounded Opening Room DObj preview and renderer diagnostics. |
| 8c | Render first post-Opening Room movie scene | Done | Preserve Opening Portraits SObj preview and handoff. |
| 8d | Reach natural opening movie Title boundary | Done | Preserve `verify-title-boundary.ps1` and current Title preview proof. |
| 9 | Replace placeholder rendering with N64 DL-to-DS rendering | In Progress | Continue from the selected bounded Opening Room material/DObj path only when a source boundary needs renderer work. |
| 10 | Load one stage and one fighter | In Progress | Pupupu/Dream Land O2Rs, original Pupupu ground/display GObjs, asset-backed Mario/Fox model GObjs, persistent FTStruct shells, original Wait/Walk/Dash/Run/Jump/Fall/Landing/CliffWait/CliffAttack/CliffEscape/CliffClimb/DamageFall/PassiveStand/Passive/DownBounce/DownWait/DownAttack/DownForward/DownBack/DownStand status proofs, bounded selected scheduler/draw traversal, source-order Pupupu floor/collision proofs, CliffQuick -> CliffAttackQuick1 -> CliffAttackQuick2 action proofs, one bounded original CliffAttackQuick2 common2 update/physics/map tick, CliffQuick -> CliffEscapeQuick1 -> CliffEscapeQuick2 action proof, one bounded original CliffEscapeQuick2 common2 update/physics/map tick, CliffWait climb/drop interrupt proof, CliffQuick -> CliffClimbQuick1 -> CliffClimbQuick2 action proof, one bounded original CliffClimbQuick2 common2 update/physics/map tick, one bounded original CliffClimbQuick2 animation-end handoff into Wait/Ground, one bounded CliffWait timeout into DamageFall/Air proof, one guarded DamageFall no-collision callback tick, one positive DamageFall map-collision branch into CliffCatch/Air setup, positive DamageFall floor-collision branches into bounded PassiveStandF/Ground and Passive/Ground setup plus two guarded stable update/physics/map frames and original anim-end handoff into Wait/Ground, one positive DamageFall map-collision branch into DownBounceU/Ground setup, bounded DownBounce/DownWait/DownStand callback ticks, one fresh bounded DownWait interrupt proof covering DownAttackU, DownForwardU, DownBackU, and DownStandU with DownAttackU attack IDs `53/33/33`, eight guarded stable callback frames plus Wait handoff for DownAttack/DownForward/DownBack, bounded roll root movement `+10000/-10000` milli, guarded callback/Wait handoff for DownStand, and one bounded same-cliff occupancy blocker proof are verified. Next gates are recovery action continuation beyond the bounded stable-frame handoffs, natural ledge occupancy/release/drop/climb behavior, broader PassiveStand/Passive runtime beyond the current two-frame callback proof plus anim-end-to-Wait handoff, and broader collision/runtime work. |
| 11 | Mario on Hyrule Castle moving using original logic | Deferred | Requires original fighter/stage/gameplay source path with adequate DS backend. |
| 12 | Expand menus, fighters, items, audio, full gameplay | Deferred | Start after first playable source-driven slice. |
| 13 | 1:1 full game port | Deferred | Requires broad compatibility, renderer/audio/save/overlay maturity, and behavioral verification. |

Current addendum: the active direct/menu-chain boundary now proves a bounded
selected live-hit damage lifecycle and damage-status follow-through while
keeping the live battle scene on Pupupu/Dream Land. Modes `161/162` inherit the
current source-order MP, cliff, PassiveStand/Passive recover, wall/rebound,
catch/throw, ledge, Dash-Run damage setup, modes `157/158` damage-recover
proofs, and modes `159/160` live-hit damage-loop proof, then prove selected
Fox Jab2 status `17->52/45`, hitlag `6->0`, callbacks `1/6/1`, source-shaped
search/proc masks, one post-hitlag original damage update tick `2->1`, one
installed original damage physics tick `phys=11500/-1000`, one installed
original damage interrupt tick `interrupt=1`, one installed original damage
map no-collision tick `map=1/1`, one installed original damage map
floor-collision branch `floor=1/1/1/1`, left/right-wall and ceiling
WallDamage short-circuit `wall=0x3ffff`, DamageFall map cliff-catch branch
`cliff=0x3f`, DamageFall map no-collision/floor fallback plus passive
short-circuits `fallmap=0x7ff`, and
repeat gate `1/1 gate=0x3f`. Current marker is
`mpLiveHit=atk=191/166 hitbox=1/j14 dmg=4 second=0/j14/x140 hurt=3/10 hbdmg=0->4/6 eff=0x1ff/0->0 resist=0xfff/7->3 rbreak=0x7f/2->-2/q2 throwAttr=0x1f/1->0 clash=0x3f/24/18 catchStat=0x1f/160000 catchSearch=0xffffffff/s3 shield=4->4/4 shc=0x7fffff/3142 so=155/134 soTick=0x1f/155->154 contact=1 repeat=1 carry=0xf gate=0x3f rehit=5->0 origRehit=hit30/v40000 30->29->0 clear=1/3 ids=0x3 dmg=0->4 hitlag=6 status=17->52/45 hitlag=6->0 callbacks=1/6/1 update=2->1 phys=11500/-1000 interrupt=1 map=1/1 floor=1/1/1/1 wall=0x3ffff cliff=0x3f fallmap=0x7ff finish=57/50 search=0xf repeat=1/1 gate=0x3f`.
Modes `157/158` remain regression coverage for the selected
hit-to-damage-to-recovery aggregate:
`mpDamageRecover=contact=1/1 dmg=0->4 hitlag=6 status=52/45 fall=57/50 ps=1 passive=1 dbounce=1`.
Modes `155/156` remain regression coverage for the bounded PassiveStand/Passive
recover loop:
`mpPassive=ps=73/62->10/4 ticks=6/5/5 passive=81/70->10/4 ticks=6/5/5`.
Modes `153/154` remain regression coverage for the bounded original
Inishie/Mushroom Kingdom scale proof, including the source-created scale
DObj/DL/material/software-preview markers and the bounded
`Wait -> Fall -> Sleep -> Retract -> Wait` scale-state path. Modes `151/152`
remain regression coverage for the platform-speed reader and dynamic
speed-consumer proof. Modes `149/150` remain
regression coverage for the yakumono position primitive. Modes `147/148`
remain regression coverage for the
natural drop-through input gate, modes `145/146` remain regression coverage for
bounded platform status/update-tic behavior, modes `143/144` remain regression
coverage for the active platform predicate, modes `141/142` remain regression
coverage for the inactive platform blocker, modes `139/140` remain regression
coverage for bounded pass-through behavior, modes `137/138` remain regression
coverage for the promoted Hyrule wall-copy proof, and modes `135/136` remain
regression coverage for the promoted Hyrule wall-hit proof.
Full player-driven recovery selection, continuous multi-hitbox activation,
continuous rehit gameplay beyond the selected Link down-air timer window, arbitrary
recovery/damage durations, full hitlag/damage runtime, and broader fighter
gameplay remain deferred.

## Active Focus

Project hygiene note: do not remove historical harnesses just to reduce
apparent bloat. Short-term process milestone complete: verification cadence is
now tiered so runtime/playability work can continue without spending every task
on the entire historical harness suite. Use `verify-dev-fast.ps1`,
`verify-boundary.ps1`, `verify-current.ps1`, `verify-regression.ps1`, and Lean
snapshots to keep iteration fast while preserving regression coverage. Feature
work should resume from the bounded CliffWait DamageFall/DownBounce boundary,
which decodes `MPLineInfo` with the original halfword layout, keeps selected
Mario/Fox roots on real Pupupu floor line `3`, routes selected map callbacks
through an original-layout `MPCollData` adapter and a floor-only
`mpCommonRunFighterAllCollisions` callback, proves split-step and
inside/outside/below-floor probes, proves bounded same-line reject,
different-line accept, no-hit miss floor sweeps, and a live P0 `-1 -> 3`
accepted second-floor route, then reaches source-order
`mpProcessRunFloorEdgeAdjust` left/right checks, resolves decoded Pupupu
edge-under wall lines `6/5`, proves the current selected Dream Land floor's
real wall-hit branch is blocked because both wall candidates are edge-under
walls, proves a finalizer-local valid-stale source-order second-floor path
from Dream Land line `1` to line `0`, proves the same valid-stale pair from
the selected P0 callback path with a contained local `MPCollData` pass while
leaving the real Mario/Fox movement loop on line `3/3`, and now mutates the
selected P0 root/collision shell through that same source-order path with
copyback to live floor line `0`, reaches the bounded source-order
`mpCommonProcFighterOnCliffEdge` status branch into original Ottotto and Fall
status setup, then runs one guarded Ottotto update/interrupt/map tick and one
guarded Fall interrupt tick from those created states, proves selected Fall
physics/map no-collision, and proves a bounded Fall/Air crossing into original
LandingLight/Ground through landing-floor setup, then chooses real Pupupu
ceiling line `4`, proves a bounded source-order ceiling test/adjust path
through `mpProcessCheckTestCeilCollisionAdjNew`,
`mpCollisionCheckCeilLineCollisionDiff`, `mpCollisionGetFCCommonCeil`, and
`mpProcessRunCeilCollisionAdjNew`, then routes the selected original
`mpCommonProcFighterCliffFloorCeil` map callback into original
`ftCommonStopCeilSetStatus`, then proves CliffCatch -> CliffWait,
CliffWait -> CliffQuick/AttackQuick setup, CliffQuick ->
CliffAttackQuick1 -> CliffAttackQuick2 action/update on the real Pupupu right
ledge, and one bounded original CliffAttackQuick2 common2 update/physics/map
tick. The current proof preserves status/motion `93/81`, retained
`cliff_id=3`, copied `floor_line_id=3`, reaches guarded anim-end, ground
velocity transfer, and edge-break map seams once, and parks before continuous
CliffAttack runtime, then proves CliffWait -> CliffQuick ->
CliffEscapeQuick1 -> CliffEscapeQuick2 through a Z-button escape selection
with status/motion `85/73 -> 86/74 -> 96/84 -> 97/85`, retained
`cliff_id=3`, copied `floor_line_id=3`, and no damage-fall or unsafe fallback,
then consumes the created CliffEscapeQuick2 state through one bounded original
common2 update/physics/map tick while preserving status/motion `97/85`,
`cliff_id=3`, and `floor_line_id=3`, then proves CliffWait climb/drop
selection and consumes the climb-created CliffQuick state through original
CliffQuick -> CliffClimbQuick1 -> CliffClimbQuick2 setup with retained
`cliff_id=3` and copied `floor_line_id=3`, then one bounded original common2
update/physics/map tick preserving CliffClimbQuick2/Ground `88/76/0`, then a
bounded animation-end handoff into Wait/Ground `10/4/0` with the current
status-reset mask `0x7ffff` proven clear, then a bounded same-cliff occupancy probe blocks a
second matching right-ledge CliffCatch attempt on Pupupu line `3`, then the
current direct/menu-chain DamageFall boundary forces CliffWait fall-wait
timeout into DamageFall/Air `57/50/1`, runs one original no-collision
DamageFall callback tick, runs one positive right-cliff map branch into
bounded original CliffCatch/Air `84/72/1`, and runs one positive floor-collision
branch into bounded original DownBounceU/Ground `68/59/0` setup with side-effect seams
counted, then proves bounded original DownBounce update ticks and one
DownWaitU/Ground `70/-2/0` stable update tick with `stand_wait 180 -> 179`,
then a bounded timeout tick into DownStandU/Ground `72/61/0` with
`damage_mul=1.0` restored. The previous DownWait boundary also proves guarded
original A-button and forward/back stick branches into DownAttackU `80/69`,
DownForwardU `76/65`, and DownBackU `78/67`, verifies DownAttackU attack IDs
`53/33/33`, then eight guarded stable update/physics/map callback frames plus
original animation-end handoff to Wait/Ground `10/4/0` for each attack/roll
branch, then the current direct/menu-chain Turn boundary imports original
`ftcommonturn.c` and proves Wait/Ground `10/4/0` -> Turn/Ground `18/12/0` ->
Wait/Ground `10/4/0` with facing `1 -> -1` and ground velocity
`2500 -> -2500` milli through the original Turn update path.
Arbitrary natural-motion valid-stale/cliff-edge cases
plus full platform/ledge behavior remain deferred until those contracts are
imported.

Current source boundary: bounded imported Title setup after the natural opening
movie path, plus guarded dev/test harnesses that can start directly at bounded
original VS Mode, PlayersVS, Maps, VSBattle setup, Pupupu ground object setup,
or Mario/Fox model/struct/init/Wait setup and prove the VS Mode -> PlayersVS -> Maps -> imported
VSBattle setup chain with Pupupu stage-data carry-through, original Dream Land
ground GObj creation, a guarded safe Pupupu update proof, and bounded
asset-backed Mario/Fox model GObj creation plus persistent FTStruct-backed
fighter state, bounded source-order init state, imported original Wait
status/motion setup, one bounded original Wait callback tick, a bounded
Wait ground-friction/map proof, display metadata probes, parser-only first
fighter display-list scans, decode-only first-DL execute proofs, visible
first-DL software draw proofs, visible multi-DL software draw proofs, and
guarded all-DL software draw proofs, the first original Wait -> Walk
input/status/velocity proof, a bounded Walk movement-loop/release-to-Wait
proof, bounded original Dash -> Run -> RunBrake movement proofs, and bounded
original RunBrake -> Wait -> KneeBend -> JumpF airborne movement proofs, plus
bounded original JumpF -> Fall -> LandingLight -> Wait ground-air-ground
proofs, bounded VSBattle update-driven Mario/Fox scheduler-loop proofs,
controller-source moving preview proofs, bounded original `gcRunAll`
  moving-preview proofs, live-input moving-preview idle proofs, bounded
  original `gcDrawAll` moving-preview proofs, Pupupu stage-inclusive original
  `gcDrawAll` traversal proofs, geometry-backed Pupupu floor-collision proofs,
  continuous geometry-backed Pupupu floor-follow proofs, real floor-edge /
  original MP floor-query proofs, source-order MP floor-process proofs,
  source-order `mpProcessUpdateMain` floor-loop proofs, source-order MP
  floor-line sweep / second-floor collision proofs, source-order MP
  cross-floor / live second-floor collision proofs, source-order MP
  floor-edge-adjust checks, source-order MP edge-under/floor-edge proofs,
  source-order MP wall-blocker proofs, source-order MP stale-valid
  second-floor proofs, selected-callback contained live-stale
  second-floor proofs, selected-callback/root motion-stale second-floor
  mutation/copyback proofs, source-order MP cliff-status branch proofs,
  bounded Ottotto/Fall callback-tick proofs, bounded Fall physics/map
  no-collision callback proofs, bounded Fall landing-floor proofs, bounded
  source-order MP ceiling-floor collision/adjust proofs, bounded original
  StopCeil status proofs from the selected ceiling-hit map callback, and
  bounded original CliffCatch status proofs from the selected right-ledge map
  callback, bounded original CliffCatch -> CliffWait proofs with one safe
  CliffWait interrupt tick, bounded original CliffWait ->
  CliffQuick/AttackQuick setup proofs, bounded original CliffQuick ->
  CliffAttackQuick1 -> CliffAttackQuick2 action/update proofs, and bounded
  original CliffAttackQuick2 common2 update/physics/map tick proofs, bounded
  original CliffWait -> CliffQuick -> CliffEscapeQuick1 -> CliffEscapeQuick2
  action proofs, and bounded original CliffEscapeQuick2 common2
  update/physics/map tick proofs.
  See
`docs/STATUS.md` for the exact current state.

Recommended next milestone:

1. Choose the next bounded source-driven gameplay or stage-runtime slice after
   the PassiveStand/Passive recover-loop proof. Keep the same bounded verifier
   discipline and do
   not promote full Mushroom Kingdom runtime, Pakkun, Power Block,
   item/monster behavior, broad map collision, or a full renderer rewrite.
2. Inspect BattleShip source and docs before changing code.
3. Use `decomp/sm64-nds` only for DS backend architecture comparison.
4. Add minimal compatibility shims in project-owned code.
5. Prove the new boundary with the narrowest verifier.

Avoid for the next milestone:

- broad fighter/stage/action-scene imports
- full title/menu rewrite
- full audio import
- full renderer rewrite
- visual polish unless it blocks the selected original source boundary

## Renderer Track

The renderer remains in progress. The current proven path is a bounded Opening
Room material/DObj preview plus a bounded Inishie scale source-DL
material/visibility proof through the DS renderer adapter, not a general
hardware renderer. Continue renderer work only from a selected original asset
or source boundary, and keep diagnostics in `docs/DIAGNOSTIC_REFERENCE.md`.

Do not import all of `sys/objdisplay.c` as one step. That crosses matrix,
camera, framebuffer/depth, sprite, and broad GBI contracts before the DS
translator is ready.

## Mid-Term

1. Expand original Title/menu flow behind DS shims.
2. Import enough task/object/display contracts for one gameplay scene boundary.
3. Continue DS-compatible relocation, overlay, and asset streaming strategy
   from the proven Pupupu dependency chain.
4. Start from asset-backed Mario/Fox fighter GObjs with persistent FTStruct
   shells, bounded init state, original Wait status/motion setup, bounded
   ground-friction/map proof, display metadata probes, parser-only DL scans,
   decode-only first-DL execute proofs, visible first-DL software draw proofs,
   visible multi-DL software draw proofs, guarded all-DL software draw proofs,
   original Wait -> Walk input/status/velocity proof, bounded Walk
   movement-loop/release-to-Wait proof, bounded original Dash -> Run ->
   RunBrake movement proofs, bounded original RunBrake -> Wait ->
   KneeBend -> JumpF airborne movement proofs, bounded original JumpF -> Fall
   -> LandingLight -> Wait proofs, the bounded scripted process-loop proof, and
   the bounded VSBattle update-driven scheduler-loop proof, then prove the next
   bounded original controller-input, collision/map, scheduler, or wider
   rendered display path.
5. Replace audio stubs with a DS sequence/sample backend.
6. Add persistent save-data support.

## Long-Term

1. Expand fighters, stages, items, effects, menus, and battle modes.
2. Improve renderer fidelity and performance.
3. Add asset conversion/cache tooling where runtime conversion is too expensive.
4. Verify behavior against original formulas and state transitions.
5. Optimize DS memory layout, overlays, and data streaming after original paths
   are proven.

## Non-Goals

- Do not hand-author Smash-like physics or attacks.
- Do not approximate moves, hitboxes, hurtboxes, or knockback when original code
  exists.
- Do not replace menus with DS-native rewrites because they are easier.
- Do not edit generated build output directories.
- Do not optimize around temporary probes before proving original code paths.
