# P2-2 — Four-Fighter Engine + Standing Stress Gate

Extends the battle engine from 2 to up-to-4 fighters using only existing
content (Mario/Fox mirrors), moves the HUD to the bottom screen, and stands up
the standing P2 stress gate. Pure engine + performance work; zero new assets.

## Scope

2–4 fighters, any human/CPU mix (single console: 1 human + up to 3 CPUs —
this IS single-console VS completeness; >1 human is P3 wireless). Free-for-all
and team battle (team colors, friendly-fire flag), 4-way results ranking,
4-way Sudden Death and ties.

## Work breakdown

1. **Fighter array generalization.** Kill every hardcoded 2-fighter
   assumption: update loop, engagement pairs (6 pairs at 4 fighters — attack
   hitboxes × hurtboxes, grab, reflectors, projectile owners), hit-resolution
   order, respawn slots, scoring, camera targets. Mirror BattleShip's N-player
   semantics (`ft/ftmanager.c`, `gm/`) for resolution order and ties.
2. **Bottom-screen HUD (owner decision).** Damage meters, stock icons, timer,
   portraits for 4 slots render on the sub screen (2D engine/OAM — near-zero
   ARM9 3D cost). Top screen becomes 100% gameplay. P1's 2-slot main-screen
   HUD retires; tick-HUD instrument stays main-screen (instrument only, never
   shipped).
3. **Camera for 3–4 targets.** BattleShip's multi-target framing/bounds
   imported; verify against 4-fighter spread cases (corner camps, vertical
   splits) on Dream Land.
4. **Memory audit + budget law.** Measure the real 4-instance footprint:
   fighter state, battlepack/animation residency, VRAM texture
   residency for 4 fighter texture sets + stage, sound RAM for 4 voice banks.
   Output: the per-fighter/per-stage byte budgets that P2-3/P2-4/P2-5 enforce
   (any 4 + any stage must fit). Arena slack is 16K and heap low-water ~52K at
   2 fighters today — expect this to force reclamation work, which is in
   scope here.
5. **Effect/particle pool policy.** 4 sources of VFX into pools sized for 2.
   Preserve BattleShip's own bounded allocation semantics so effect submits
   (the known tail driver) stay bounded regardless of fighter count.
6. **Stress gate stand-up.** New verifier arm: 4×level-3-CPU (level re-argmaxed
   later per stress-config law), Dream Land, one-minute match, DLDI on, whole
   match. Reported like every gate: tick P50/P95, cadence histogram
   2/3/4/5+, max interval. This arm becomes the standing stress config and
   joins Boundary at phase close. `NDS_R2_BOTH_CPU` generalizes into it.
7. **First mitigation wave.** The initial 4-CPU number will be deep red —
   expected, that is the point. Census by lane, rank levers, land the cheap
   structural ones (stagger pose updates across frames, anim LOD by on-screen
   size, broadphase AABB prefilter before hitbox pairs). Deep levers (30 Hz
   compensated sim) stay parked pending owner approval per sacrifice order.

## Static source-parity audit (2026-08-20)

The implementation audit is intentionally separate from the runtime stress
gate. The owner is doing runtime verification later; none of the observations
below substitutes for that gate.

- **Battle ownership and iteration:** normal VS and Sudden Death use the imported
  BattleShip `scVSBattle` loops. The DS live-fighter registry, per-instance
  renderer caches and match descriptor are now `GMCOMMON_PLAYERS_MAX` wide.
  Native Mario/Fox generated data deliberately remains two rows wide: those
  rows mean *fighter kind* (Mario/Fox), not player instance (P1..P4).
- **Hit/grab/projectile semantics:** `ftmain.c`, `wpmanager.c`, `wpmain.c` and
  `wpprocess.c` remain the source implementations. They walk all live GObjs and
  preserve source team/friendly-fire, owner, `player_num`, stale and reflector
  attribution. The DS `ftParamGetPlayerNumGObj` bridge now performs the source
  linked-list lookup by fighter instance number rather than treating it as a
  0/1 battle slot.
- **Spawn/respawn:** initial spawn is the source four-slot VSBattle loop and
  rebirth uses the source all-fighter halo-slot allocator. The DS map-object
  position shim now also matches the source's missing-spawn behavior: it zeros
  only when the stage has no map objects and otherwise leaves the caller's
  position unchanged if no requested player spawn exists.
- **Normal-match entry:** ordinary VSBattle again leaves `FTDesc.is_skip_entry`
  false exactly like BattleShip, so `ftManagerMakeFighter` starts every fighter
  in Common Entry and `ifCommonEntryFocusThread` advances all live fighters into
  their source appearance sequence. Mario therefore owns the source pipe entry
  and Fox the source Arwing entry even with mirrors in P3/P4. The DS wrapper only
  retries deferred effect descriptors after fighter files become resident. The
  common Entry/Appear bodies are a bounded, line-for-line behavioral extract of
  `ftcommonentry.c:49-268`: P2-2 carries the exact Mario/Fox branches instead of
  importing unsupported Donkey/Samus/Link/etc. entry dependencies. Status IDs,
  120-tic wait, position restore, camera/ghost/shadow state, animation flags and
  Mario-pipe/Fox-Arwing effect choice therefore remain source-owned semantics.
  Sudden Death keeps the source's explicit `is_skip_entry = TRUE` path.
- **Shield-break lifecycle:** the P1 compatibility layer still had a genuine
  source-behavior dead end: six callbacks used by the source common-status table
  (`ShieldBreakFly` through `FuraFura`) were weak no-ops, while the DS entry shim
  zeroed vertical launch speed and substituted generic damage-fall map/physics.
  P2-2 now links the BattleShip `shieldbreakfly/fall/down/stand` and `furafura`
  TUs. The public DS seams delegate to the source bodies, restoring the source
  `attr->shield_break_vel_y`, Fly -> Fall -> Down -> Stand -> FuraFura chain,
  colanim/SFX and breakout timing for every live fighter instance.
- **Camera:** the imported `gmcamera.c` target set is already
  `GMCOMMON_PLAYERS_MAX` wide and uses the source 2/3/4-target zoom/bounds path.
  No parallel two-player camera remains on the shipping battle path.
- **Results:** BattleShip Results owns four-wide presence, fighter, KO/TKO,
  points, place and team-ranking arrays, including Sudden Death tie ordering.
  The DS renderer registers all four Results fighters; its Results observation
  arrays are now four-wide as well.
- **Battle stats, CPU targeting and Sudden Death selection:** the DS
  `ftParamInitPlayerBattleStats` bridge now matches `ftparam.c:158-183`
  field-for-field, including four-opponent KO/damage matrices and stale-move
  state, before adding only the DS live-registry mirror. CPU decision making is
  the imported `ftcomputer.c`: target searches walk the entire fighter GObj list
  and reject same-team targets exactly as the source does; the only P1-specific
  Fox pause branch is an explicit free-play harness override, not the normal
  source CPU path. `scVSBattleSetScoreCheckSuddenDeath` is also the source
  implementation, so FFA and Team ties are collected across all four transfer
  slots before the source recreates only tied fighters for Sudden Death.
- **Grounded damage-velocity / tech state:** a source audit found a real
  gameplay omission in the old compatibility seam: `ftParamVelDamageTransferGround`
  contained only proof counters even though BattleShip calls it from DownBounce,
  Passive and PassiveStand. The DS body now matches `ftparam.c:1426-1448`: on
  ground it seeds `vel_damage_ground` from air-X once, clamps it to +/-250, then
  projects that velocity through the current floor angle. The same audit removed
  the DS-only `fp->damage += damage` mutation from `ftParamUpdateDamage`; source
  gameplay damage is `percent_damage` plus the battle-state totals. BattleShip's
  held-item damage/drop tail is deliberately not claimed here: the P2-2 match is
  still item-off and the current item-manager seam does not yet implement the
  source `itMainSetFighterDrop` lifecycle.
- **Effective hit-status aggregation:** another live `ftparam` shortcut returned
  only `fp->hitstatus`. BattleShip `ftParamGetBestHitStatusPart/All`
  (`ftparam.c:645-689`) first folds the active hurtbox statuses, then applies the
  fighter-wide star and timed-special overrides. The DS seam now follows that
  exact ordering. This matters to source throw/capture and hazard paths and, in
  particular, prevents a respawn/star/timed-invincible fighter from being treated
  as globally Normal merely because its base `hitstatus` is Normal.
- **Hit-status / colour-animation lifecycle:** the same audit found that several
  DS compatibility setters changed collision state without performing the
  colour-animation side effects BattleShip couples to that state. P2-2 now
  restores `ftparam.c:569-642,1247-1323,1716-1740`: whole-fighter and all-part
  status changes select the source Normal/Invincible/Intangible colanim, timed
  invincibility/intangibility selects NoDamage, timed intangible extends rather
  than shortens an existing window, and `ftParamResetStatUpdateColAnim` restores
  live hurtbox, heal, Star, timed no-damage and Hammer overlays after an unlocked
  script ends. The DS table now carries command-for-command native-layout copies
  of BattleShip's common Mario/Fox VS scripts and their exact priorities for CPU
  preview, hit-status, FallSpecial, FastFall, Heal, NoDamage, Rebirth, Hammer and
  Star. Donkey/Samus/Kirby/Ness charge-overlay branches remain intentionally
  absent here because those fighter kinds cannot be created by P2-2; add them
  with those rosters rather than inventing unreachable state now.
- **Facing from centered stick:** BattleShip `ftParamSetStickLR`
  (`ftparam.c:251-254`) treats X=0 as facing right. The old DS helper skipped the
  zero case and could preserve a previous left-facing direction through common
  dash/smash/special setup. The shared helper now follows the source comparison
  exactly, which also covers Mario/Fox special paths that call it directly.
- **Hitlag arithmetic order:** the DS `ftParamGetHitLag` helper had moved
  `hitlag_mul` after the crouch reduction. BattleShip applies the move's hitlag
  multiplier first, stores/truncates it to integer ticks, and only then applies
  the Squat/SquatWait 2/3 reduction (`ftparam.c:1511-1523`). Because both stages
  truncate, the operations are not interchangeable. The DS helper now preserves
  that exact source ordering for every fighter hit and damage-shuffle consumer.
- **Lower-screen state bridge:** imported `ifCommon` remains the gameplay HUD
  authority. The DS sub-screen publication/render bridge now carries active,
  damage, stock, fighter kind and CPU level state for all four source slots
  instead of folding P3/P4 into P2. The source timer, stock and damage display
  GObjs stay live for their source-side state/update semantics but their steady
  display callbacks are routed away from the top screen; countdown/GO and other
  interface GObjs retain the source top-screen route. This fixes the audit catch
  where damage had been drawn on both screens after timer/stock were migrated.
  The sub-OBJ sink also owns Bank I explicitly: libnds `oamInit` enables the sub
  OBJ layer, generated graphics are copied through DMA, and every palette-RAM
  write now uses DMA rather than generic `memcpy` so the DS's dropped 8-bit
  palette writes cannot corrupt timer/portrait/stock colours.
- **Effect capacity:** the old P1 shell inherited deliberately reduced pools
  (`EFStruct` 12 and particles 48/24/24). That changes source behavior under a
  four-way VFX burst. P2 shell targets now restore BattleShip's 38-effect pool
  and 112/24/80 particle struct/generator/transform capacities, including the
  source `efManagerGetNextStructAlloc` last-four forced-effect reserve. Runtime
  heap headroom and saturation remain measurements for the stress gate. The
  static arena cost is now pinned from the linked stress ELF rather than
  estimated: `EFStruct` is 60 B (`0x8e8 / 38`) so 38 entries are 2,280 B; the
  source particle structs/generators/transforms are 96/92/192 B, so
  112/24/80 consume 28,320 B. Relative to the old 12-effect + 48/24/24 P1
  pools, restoring source capacity reserves **18,456 additional bytes**
  (1,560 effect + 16,896 particle) before any four-fighter instance growth.
  This is a fixed-pool floor for the memory audit, not the final byte budget.
- **BattlePack:** the resident pack is an immutable **Fox animation-kind** pack,
  not storage allocated once per player, so any number of Fox mirrors can share
  it. The separate two-root `nds_ftanim_track.c` cache is a dormant experiment
  (`NDS_R2_FTANIM_TRACK=0` in the shipping shell), not a four-player runtime
  capacity limit.
- **Instance slot vs native owner:** the shipping fighter draw path now keeps
  `fp->nds_slot` as the P1..P4 instance identity while mapping only the generated
  topology/material lookup to the Mario/Fox owner row. The remaining two-wide
  native tables are therefore content-kind tables, not a hidden P1/P2 cap. The
  literal two-slot loops left in the movement/display backends are bounded proof
  harnesses or owner-kind diagnostics and are not on the imported shipping
  VSBattle update/draw path.
- **Transform invalidation cache:** one genuine P1-era two-entry runtime cache
  remained in `reloc_backend_compat_shims.c`. It was correctness-safe because a
  miss rebuilt the authoritative DObj walk, but four live roots could thrash it
  every frame. Its slot count now follows `GMCOMMON_PLAYERS_MAX`, so P3/P4 do
  not pay a DS-only rebuild tax; collision/overflow still fails closed to the
  source-equivalent recursive walk.
- **Team-mode entry audio:** BattleShip `mnPlayersVSFuncStart` announces the mode
  already present in the transfer state on every CSS entry. The native shell now
  uses that same FFA/Team dispatcher on entry instead of hard-coding the FFA
  voice, so Maps -> CSS preserves Team Battle behavior as well as the label,
  teams, costumes and shades.
- **Team appearance rows:** the Mario/Fox `ftParamGetCostumeTeamID` bridge was
  checked directly against BattleShip `dFTParamCostumeIDs`: Mario is
  `{0,3,4}` and Fox is `{1,2,3}` for Red/Blue/Green. Combined with the imported
  four-slot `mnPlayersVSGetShade` allocator, mirrored teammates use source team
  costumes and first-free same-kind/same-team shades rather than slot-index
  colors.
- **Four-CPU stress seed:** `NDS_R2_BOTH_CPU=1` remains the historical two-CPU
  case on the direct-boot R2 targets, so their old measurements are not silently
  redefined. On a P2 menu-shell build it now seeds four level-3 CPUs as
  Mario/Fox/Mario/Fox with BattleShip's reset-team split Red/Red/Blue/Blue. This
  deliberately exercises P3/P4 plus same-kind mirror instances without adding
  content. Use the free-play shell target for the later manual stress pass; the
  scripted shell walk intentionally mutates CSS state as part of its menu test.
  The standing timing arm is separate and deterministic:
  `scripts/verify-p2-four-fighter-stress.ps1` builds
  `smash64ds-p2-fourcpu-tickhud-hwtri`, boots directly into source VSBattle with
  four level-3 CPUs, restores source 38-effect and 112/24/80 particle capacities,
  and reuses the canonical tick-HUD collector plus the independent match-window
  coverage probe. That coverage probe now also refuses the arm unless the source
  VSBattle publications report 0 humans, 4 CPUs, 4 created fighter GObjs and
  active-player mask `0xF`, so a mis-seeded two-fighter build cannot produce a
  plausible four-fighter timing report. The timing run also reads the existing
  arena/general-heap/DObj/effect/particle high- and low-water counters, and only
  after whole-match coverage passes writes `p2-2-fourcpu-memory.json`; allocator
  overflow or objman panic fails the arm, while source-style pool saturation is
  reported rather than silently redefined. **Runtime acceptance passed on
  2026-08-21 and this arm is now the third standing Boundary arm**, alongside
  the P2 shell loop and the unchanged two-fighter realtime regression arm.
  **SUPERSEDED IN ONE RESPECT (board row P2-3r15, 2026-08-25): the arm's roster
  is no longer Mario/Fox mirrors.** `NDS_P2_FOUR_CPU_ROSTER` defaults to 1 on
  this target, so it runs the four LANDED kinds — Mario/Fox/Luigi/Donkey — which
  is what `PROJECT_GOAL.md`'s "measured hardest fighter set" asks for once four
  kinds are landed and fit the shipping configuration (P2-3r13). Everything else
  in this bullet still holds, and `NDS_P2_FOUR_CPU_ROSTER=0` rebuilds the mirror
  arm as the A/B control. **Every timing figure in this document is a mirror
  figure.**
- **Stress target static proof:** the dedicated target now compiles cleanly as
  `smash64ds-p2-fourcpu-tickhud-hwtri`. Its generated config reads
  `NDS_P2_FOUR_CPU_STRESS=1`, `NDS_R2_BOTH_CPU=0`,
  `NDS_R2_SOAK_MATCH_MINUTES=0`, `NDS_R2_EFFECT_POOL=38`, tick HUD on,
  hardware triangles on and renderer mode 9. Disassembly of its actual linked
  `efParticleInitAll` carries literal 112/24/80 capacities, while the linked
  source `ndsBaseEFManagerInitEffects` allocates `0x8e8` bytes, seeds free-count
  38 and links entries at a 60-byte stride. That proves the stress ROM is built
  with BattleShip's source pool depths before the first runtime measurement.

### Runtime acceptance and P2-2 budget law (2026-08-21)

The first complete standing run is accepted, not projected. The exact target was
`smash64ds-p2-fourcpu-tickhud-hwtri`, ROM SHA-256
`C5814DEA590EEBA1525D24F3290CDFBA5C749ACEA931093E4E92AF4C23DBE867`.
`scripts/verify-p2-four-fighter-stress.ps1` sampled **1,972** presented frames
(`2..1973`) while its same-run coverage read spanned frame `1..1973`; the source
clock moved `60 -> 1` seconds (**59/60 = 98.33%**, the verifier's one-second
display-quantization allowance) and both endpoints reported **0 humans, 4 CPUs,
4 fighter GObjs, active mask `0xF`**. There were zero cadence violations. Timing
debt is reported rather than hidden: `ALL` P50/P95 = **1,677,952 / 2,238,464**
ticks, `WORK-H` P50/P95 = **1,482,752 / 1,963,648**, with VBlank intervals
`2:121, 3:1280, 4:507, 5+:65`, max `21`. This is why P2-2's first mitigation was
the source Low-detail native fighter owner rather than a simulation-rate change.
Evidence: `artifacts/verification/2026-08-21_p2-2-fourcpu-boundary-final.log`,
`p2-2-fourcpu-tickhud.json`, and `p2-2-fourcpu-coverage.json`.

**Read with two caveats (2026-09-04).** First, `ALL` is VBlank-quantised — its
2,238,464 is exactly 4 x 559,616 — so it is a presentation-interval readout,
not a CPU cost; `WORK` is the honest column. Second, the `p2-2-fourcpu-*.json`
pointers above were overwritten on 2026-08-27 by a Mario/Fox/Captain/Donkey run,
so the numbers in this paragraph are no longer in the files it names; the
08-21 log is the surviving witness. The current four-distinct-kind figure lives
in `artifacts/performance/2026-09-01_bug-fourcpu-relative-fastpath-full/`
(WORK P50 1,560,672 / P95 2,697,209, 3.29% two-VBlank); do not cite this
section as the present state.

`arenaSearchAllocationFailures` (45 there, 17 on 08-27) is not lost work: it counts the boot-time 4 KiB step-down probes `diagnostics_taskman_heap.c:63` makes on the libnds heap before `calloc` succeeds, so 45 = 184,320 B of arena given up. It rose because the anim-cache reservation grew 163,840 -> 258,048 and the general-heap free floor fell from 325,240 to 32,164 B -- **6,564 B above the 25,600 B verifier floor**. That margin, not the counter, is the four-fighter RAM constraint.

The same accepted run supplies the dynamic memory law. General-heap free
low-water is **40,400 B**, so the standing **25,600 B safety floor has 14,800 B
margin**. DObj active high-water is **209**. BattleShip's source-sized effect
pool peaks at **17/38** active (free-min 21, never entering the last-four forced
reserve). Particle peaks are **33/112 structs, 11/24 generators, 14/80
transforms**, with **0 rejects**. AObj normalized high-water is **2,330**, with
zero normalize failures and zero hash overflow; `syMalloc` overflow and objman
panic are both zero. The native Low-detail plan is live and stable (`build=680`,
`hit=6513`, `verifyMismatch=0`). Evidence:
`artifacts/verification/p2-2-fourcpu-memory.json`.

The byte-budget law is deliberately expressed by **owner**, not by dividing
shared files by four instances. BattleShip loads fighter files by fighter kind;
Mario mirrors share Mario residency and Fox mirrors share Fox residency. A
frame-32 probe on the accepted four-fighter binary therefore records the actual
post-setup arena instead of inventing a per-instance asset charge:

- taskman arena: **1,548,288 B cap**, **1,484,912 B high-water**, **63,376 B
  setup headroom**;
- reloc residency: **681,632 B total**, split **202,816 B stage**, **175,440 B
  fighter-kind**, **208,672 B interface**, **94,704 B other**, with **0 B stale**;
- menu/opening residency in VSBattle: **0 / 0 B**;
- dynamic general-heap law: regardless of how the static owner split changes,
  a legal `4 fighters + stage` configuration must remain **>=25,600 B free for
  the entire match**, not merely at scene setup;
- bounded VFX law: effect/particle capacities remain BattleShip's **38** and
  **112/24/80**. A new fighter/stage/item may increase peak use but may not
  silently shrink those source capacities to make memory fit.

That gives P2-3/P2-4/P2-5 a checkable rule: new **fighter kinds** debit the
fighter-kind residency bucket (mirrors do not), a new **stage** replaces the
stage bucket rather than stacking with Dream Land, and item content debits the
remaining scene/general-heap envelope. Any candidate that makes arena setup
exceed 1,548,288 B, creates stale residency, or drops whole-match general-heap
free space below 25,600 B must reclaim/stream memory before it can join the
content set. The measurement is
`artifacts/verification/p2-2-fourcpu-budget-frame32.txt`; the whole-match dynamic
floor remains the stronger acceptance value above.

**The law fired for the first time on 2026-08-25 (board row P2-3r11).** Four
DISTINCT kinds do not fit this arena. Per-kind main-file trees are Mario
54,048, Fox 119,040, Luigi 57,104, Donkey 79,648
(`include/nds/generated/nds_fighter_production.generated.h`), so
Mario/Fox/Luigi/Donkey debits **309,840 B** against the mirror roster's
173,088 -- exactly the "mirrors do not debit" clause, seen from the other
side -- plus 36,276 B of native-owner image for the two new kinds, plus the
36,864 B the larger ARM9 binary costs the arena 1:1 through
`ndsTaskmanArenaBytes`'s step-down. Total need ~175 KB. Unpaid, it is not a
degraded run but a permanent silent halt: `ftManagerSetupFilesMainKind` for the
fourth kind asked for 77,360 B with 8,300 B free and stopped in
`ndsSyMallocOverflowHalt`.

**P2-3r13 PAID IT, AND NOT OUT OF THIS BUDGET.** The lab arm's
`NDS_R2_BATTLEPACK := 0` plus 32,768 B cache trim (+222,400 B) is withdrawn; the
shipping configuration now hosts four distinct kinds at `NDS_R2_BATTLEPACK 1`
with the full 451,776 B animation reservation. The bytes came from the ARM9
static image and from a reservation the DS renderer does not use: the 185,696 B
title/opening/Castle scene file store left `.bss` for a lazy scene-arena
allocation (`ndsRelocSceneFileBuffer`), `NDS_TASKMAN_ARENA_SIZE` rose
0x17a000 -> 0x1a7000 (+184,320) to spend it, and the VSBattle DL buffers
returned 30,720 B against a measured use of 16 bytes. Accepted run
`artifacts/verification/2026-08-25_p2-3r13-ship4-SUMMARY.md`: 4 fighter GObjs,
mask `0xF`, clock 60 -> 1, **general-heap low-water 49,956 B against the 25,600 B
floor**, arena 1,695,744 (AllocFail 9 -- the binary's 36,864 B is unchanged, not
fixed).

**The per-kind charge itself is unchanged, so this law still governs fighter
#5.** Unique arena bytes per distinct kind: Mario 54,048, Fox 116,752, Luigi
41,552, Donkey 77,360 (four kinds 289,712; mirror roster 170,800). Model files
are 149,616 B of that (51.6%), ShieldPose files 31,920 (11.0%), Fox's
ExternDataBank109 47,120 (16.3%). Every further fighter kind repeats the charge,
so budget the remaining eight against this number rather than against the mirror
arm's, and against today's 24,356 B of margin plus the one measured reclaim left
in the battle arena -- the per-context graphics heap, peak **96 B of 53,248**,
two contexts.

The two-fighter source regression was then re-run through the shipping shell.
Restoring BattleShip Common Entry exposed two verifier assumptions rather than
gameplay defects: Mario's pipe/Fox's Arwing are source link-10 effect DObjs and
therefore contribute exactly **60 submits / 2,400 triangles** to the shared
stage-adapter ledger, while `ftCommonEntrySetStatus` makes fighters invisible
and `ftdisplaymain.c:1087-1092` suppresses them until each fighter's own Appear
sequence exposes it. The verifier now windows CSS lifetime counters at
`scVSBattleStartBattle`, proves the source-effect ledger independently, subtracts
only those owned effect/weapon contributions before enforcing Dream Land's exact
**42-list / 202-triangle** base, and validates admitted Mario/Fox owners at exact
**320 / 306 triangles per submit** without falsely requiring equal visibility
counts during Entry. The focused arm finishes with
`battle_playable Pupupu realtime pacing smoke passed: frames=212` and
`AOBJ32_FAIL=0`; no source behavior was relaxed to make the gate pass. Evidence:
`artifacts/verification/2026-08-21_p2-2-twofighter-boundary-source-entry-final.log`.

### Owner visual residual

Automated/runtime acceptance is green and the four-CPU arm is now in Boundary.
The remaining non-automatable closeout is the owner's visual/play pass for the
four-way camera framing, lower-screen HUD presentation, Team Battle feel and
four-way Results/Sudden Death presentation. The current mid-match emulator
capture (`artifacts/visibility/p2-2-fourcpu-midmatch.png`) visibly contains all
four fighters and four HUD slots, but that is evidence for implementation, not a
claim that the owner has personally accepted the presentation.

## Reference

### Finding (2026-08-20): four-fighter matches lose the native fighter owner path to the source low-detail switch

The 64-frame four-CPU census (`artifacts/performance/2026-08-20_p2-2-fourcpu-profile64/`)
shows `ndsRendererExecuteNativeFighterOwnerProduction` at 0 cycles while the generic
fallback burns the frame (`ndsRendererScanList` 9.50%, `ndsRendererSubmitHardwareTriangle`
7.09%, `ndsRendererHardwareSubmitVertex` 5.25%, `ndsRelocFindLoadedFileContaining` 4.96%).
The two-fighter shipping census (2026-08-17 `shipping-rebank/v4-c238`) shows the exact
inversion, so the fast path itself is healthy.

Root cause, pinned by a GDB probe on both ROMs (slot/size/roots/material counts identical,
return value differs): source `scvsbattle.c:188/:460` sets
`desc.detail = (pl_count + cp_count < 3) ? nFTPartsDetailHigh : nFTPartsDetailLow` —
SSB64's own 3+ fighter policy. Low-detail commonparts put every fighter DL at different
offsets inside the same 0x7510/0x7e50 model files (Mario root 0 lives at 0x3C78, not the
baked 0x1668; Fox at 0x4720, not 0x1970). `ndsRendererValidateNativeFighterOwner` compares
live root offsets against high-detail-baked `sNdsNative{Mario,Fox}Roots`, fails on the
first root every fighter every frame, never bakes a draw plan, and the whole match renders
through the generic interpreter — which the ITCM re-knapsack evicted to main RAM on the
assumption the native path had replaced it. Fix direction: bake the low-detail table set
in `generate_nds_native_owners.py` and key table selection on the fighter's detail level;
that also deletes the per-frame plan-resolve `ndsRelocFindLoadedFileContaining` scans.
The previous session's quoted profile figures (13.1% ValidVertexNoTransform, 12.6%
Figatree) do not appear in the banked census; use the census numbers above.

**Generator work is complete (2026-08-20, same day).**
`generate_nds_native_owners.py` now runs the full pipeline a second time over the
low-detail JointTrees (Mario `dMarioModel_JointTree_0x4590`, Fox
`dFoxModel_JointTree_0x5510` — same descriptor counts, same sentinels) and emits
`...Low` twins of every runtime table: 32 roots / 50 epochs / 53 runs / 393
triangles, 420 dense vertices / 1179 corners, identical light preambles and
cross-binding slots (low shares the high skeleton). All canonical pins are
per-detail (`DETAIL_EXPORT_CARDINALITIES`, `DETAIL_SUBMIT_CLASS_CENSUS`,
`DETAIL_LIGHT_CENSUS`, `DETAIL_GX_PLAN_COUNTS`, `LOW_SOURCE_EXPORT_HASHES`,
`LOW_DIRECT_EPOCH_POLICIES`). The low direct policies are DERIVED by
`derive_direct_epoch_policies()` (combine-pair families + geometry bit 0x400 at
runs time), which reproduces the frozen high table exactly before being trusted.
`sNdsNativeFighterPreparedDenseLow` is deliberately main-RAM, not DTCM (DTCM has
~7 KB free; the low set would not fit). The high output is byte-identical;
the fourcpu ROM rebuilds size-identical because gc-sections drops unreferenced
statics.

**Closed implementation (2026-08-21):** that Low-detail owner slice is now
landed. The generated Low tables are selected by fighter detail, the draw-plan
cache keys detail, and the production path consumes the selected per-detail IR.
The standing stress above is the acceptance test: `gNdsFtrPlanBuild=680`,
`gNdsFtrPlanHit=6513`, `verifyMismatch=0`, with both Mario/Fox hardware owners
nonzero. Mode 7 remains high-detail-only as designed; the shipping mode-9 path
is the one exercised by Boundary.

- `ft/ftmanager.c`, `ft/ftcommon/` for N-fighter iteration and engagement
  order; `gm/` for match rules, teams, results ranking; camera sources for
  multi-target bounds.
- `sm64ds-decomp` for sub-screen 2D HUD layer setup.

## Risks

- Perf remains debt, but it is now measured rather than hypothetical. The
  accepted four-CPU arm is `ALL` P50/P95 1,677,952 / 2,238,464 ticks and
  `WORK-H` 1,482,752 / 1,963,648. The first measured structural mitigation was
  therefore the source-required Low-detail native fighter owner; it removed the
  accidental generic-renderer fallback without changing simulation cadence.
- **RAM is an open P2-2 risk again as of 2026-09-04.** The measurement that
  closed it -- 40,400 B free against the 25,600 B safety floor, 63,376 B of
  frame-32 arena setup headroom, zero stale reloc residency -- was taken on a
  Mario/Fox-class fighter set and is still correct for that set. It does not
  survive the landed roster.
- The per-kind general-heap cost is **one allocation**:
  `ftManagerSetupFilesMainKind` does
  `syTaskmanMalloc(lbRelocGetFileSize(data->file_main_id), 0x10)`
  (decomp `ft/ftmanager.c:285`), and the sizes are the first census column of
  `include/nds/generated/nds_fighter_production.generated.h:9-20`:

  | Mario | Luigi | Purin | Donkey | Ness | Pikachu | Samus | Captain | Link | Fox | Yoshi | Kirby |
  |---|---|---|---|---|---|---|---|---|---|---|---|
  | 54,048 | 57,104 | 72,368 | 79,648 | 79,216 | 80,528 | 85,296 | 102,448 | 107,248 | 119,040 | 146,928 | 204,208 |

  Mario plus Fox is 173,088 B. The four heaviest -- Kirby, Yoshi, Fox, Link --
  are **577,424 B**. (Do not size this from the `reloc_fighters_main` directory
  instead: those on-disk sums rank Yoshi eleventh where the allocation ranks it
  second, so they are not a usable proxy.)
- The overflow is already observed at **two** fighters, not four. A Kirby versus
  Fox lab halts in `ndsSyMallocOverflowHalt` refusing Fox's 115,440 B after
  Kirby's own 204,208 B succeeds (`docs/p2/fighters/kirby.md`, board row
  P2-3f47). Substituting Kirby for Mario adds 150,160 B, and the 40,400 B of
  slack above cannot absorb that one swap -- let alone three more.
- **Kirby's cost is not copy-ability donor data**, which was the obvious guess
  and is wrong. His neutral-B dispatch is a function-pointer list
  (`ftcommonspecialn.c:10-38`), his hats come from the native image slot at
  `ftManagerMakeFighter`, and `dFTKirbyData` leaves special1/3/4 null, so
  `ftManagerSetupFilesMainKind` loads **zero** donor bytes. The 204,208 is his
  own KirbyMain closure -- 144 extern ids against Mario's 48.
- Treat the table as a **lower bound on the problem**: it is the `file_main`
  allocation only, not animation, model or effect residency and not the
  per-fighter runtime structures. It sizes the problem; it does not plan the
  fix. P2-2p8 and P2-3f47 share this root cause and neither closes until the
  resident per-fighter cost is bounded -- by deferral, by a smaller resident
  form, or by reclaiming image bytes, which the arena takes one for one.
- Engagement still has BattleShip's exact O(n²) pair semantics (6 unordered
  fighter pairs at four fighters). The first stress census did not make that
  the highest-ranked safe lever, so P2-2 did **not** insert an ordering-risking
  broadphase merely because it was listed as a candidate. If a later stress
  argmax makes it worth doing, any AABB prefilter must preserve the source pair
  order and exact hit/catch/reflect resolution for admitted pairs.

## Exit criteria

- [x] 4-fighter Mario/Fox mirror engine semantics audited against BattleShip;
      four live fighters are runtime-proven. Results/Sudden-Death presentation
      remains in the owner visual residual above, not an unimplemented engine path.
- [x] FFA + team battle rules are source-owned and CSS teams toggle live.
- [x] Bottom-screen HUD is shipped for 2–4 slots through the dedicated
      `nds_battle_hud` module; source `ifCommon` state remains authoritative.
- [ ] Owner visual/play pass for four-way camera framing, bottom-screen HUD,
      Team Battle feel, Results and Sudden Death presentation.
- [x] Budget law published (owner-based fighter/stage/item byte envelopes) from
      the measured 4-way audit.
- [x] Stress verifier arm running and reporting; first mitigation wave landed;
      remaining gap attributed by lane on the board.
- [x] 2-fighter Boundary still green (P1 regression guard).
- [x] Full three-arm Boundary closeout rerun is green on 2026-08-21:
      `p2_shell_loop`, `p2_battle_realtime`, and `p2_fourcpu_stress` all pass
      from retained artifacts in one profile invocation. Evidence:
      `artifacts/verification/2026-08-21_p2-2-boundary-closeout-final.log`.

## External design review, 2026-09-04 — `docs/reviews/Design_DS_fighter_paging.md`

The owner had the roster RAM problem reviewed externally. The full text is in
that file; what follows is what it CHANGES, because it replaces the plan rather
than confirming it.

**Runtime demand paging is refused, for a reason I had not weighed.** The reloc
files hold RELOCATED ABSOLUTE POINTERS: the loader resolves internal and
external references into addresses, and the status buffer lets later files
reuse them. Evicting one raw file can therefore leave live pointers to freed
memory in another loaded file, in a DObj or MObj, in a fighter-kind global, in
a live weapon or effect, or in renderer state. The mechanically safe paging
unit is a strongly connected component of the relocation graph plus everything
holding pointers into it, which in practice expands toward the whole closure.
Paging this representation is unsafe by construction, not merely expensive, and
"pick a better page size" does not answer it.

**The hole is bigger than I sized it.** Keeping the 32,768 B floor needs
370,960 B recovered, and that is a lower bound: it ignores per-instance state,
joint/parts allocations, animation scratch, four-player audio pressure, and
items and effects going from two instances to four. Target at least **512 KiB**
of net additional battle headroom.

**The direction instead:** an offline-generated, match-resident pack. At
character-select completion build a manifest from the unique selected kinds,
costumes, stage, item mode and reachable Kirby copy assets; load it completely
before GO; bind once into the fighter-kind tables; never move or evict until
teardown, so hot paths keep today's pointer access. Do NOT load the original
closure and compact it in RAM -- that creates exactly the temporary peak that
already fails. The build pipeline emits the final DS representation directly.

**First experiment, concrete and falsifiable.** Kirby's raw model member is
120,864 B. Build a low-detail battle pack holding only what runtime consumers
actually read -- joint hierarchy, setup descriptors, attachment points, live
material state, collision metadata, selected-costume textures, the existing
native low-detail program -- bypass `KirbyModel` entirely, poison the abandoned
range after setup, then exercise entry, every status, throws, all Copy powers,
Stone, Final Cutter, items, KO, respawn, pause, Results and rematch. Any
post-setup read is a missing field, not a pass. Raw model members for the worst
four total 271,040 B (Kirby 120,864, Link 73,584, Yoshi 44,256, Fox 32,336).

**A standing law once the pack exists:** `asset_reads_after_go == 0`,
`animation_file_loads_after_go == 0`, `mandatory_page_faults == 0`,
`general_heap_low_water >= 32768`. Animation is the one place the source
already behaves like demand loading -- a status change loads the motion into
the figatree heap -- and that is not acceptable as an unpredictable DLDI miss
at 30 Hz. The residency metric is not "data this frame touched" but **all data
reachable with zero reliable prefetch notice**: an input, collision, grab,
throw, reflector, item pickup or damage transition can select a new state
immediately.

**"Any four fit" should be a generated assertion.** There are only
C(12,1)+C(12,2)+C(12,3)+C(12,4) = **793** unique one-to-four fighter-kind sets,
and mirrors add no immutable pack. The build should enumerate all 793 across
stage and item configurations, prove their decoded resident size, and report
the true worst set rather than a hand-picked stress configuration.

**A correction against my own earlier note.** Do not treat the 237,568 B audio
FGM cache as an early concession. This repository has already measured that its
misses can dominate frame time and that the cue working set exceeds the cache
-- it is undersized, not wasteful. Naming it the largest available knob was
wrong.

Presentation cuts come only after compact match packs, duplicate-representation
removal and scene overlays are measured. Ranked: no-fidelity-loss changes first
(drop duplicate N64/DS representations, load only selected kinds and costumes,
source low-detail model for three and four players, scene-exclusive data into
overlays), then representation changes, then memory placement (texture VRAM
residency releasing the main-RAM sources), then presentation-only reductions,
then cosmetic, then content restrictions, and only last anything touching
platform or fighter count.

---

## The four-kind RAM answer, settled by external review (2026-09-04)

Two reviews now govern this. `docs/reviews/Design_DS_fighter_paging.md` refused
runtime paging and prescribed an offline match-resident pack.
`docs/reviews/Review_Deriving_Fighter_Live_After_Setup_Set.md` answers how to
build it, and its first act is to kill the shape we were heading for.

**A model-only pack is arithmetically incapable of closing the hole.** The four
raw models are 271,040 B of the 577,424 B worst-four closure. Deleting all four
*for free* still leaves 306,384 B, which is 130,780 B above the ceiling. The
6-12 KB Kirby model estimate was never sufficient evidence that P2 fits, and it
must not be cited as if it were.

**The budget equation, which supersedes the "404,336 B shortfall" framing.**
That framing double-counted: it ignored the 173,088 B the shipped pair already
spends, which the pack reclaims. With `W` the whole four-kind resident
replacement, `D_other` other four-player growth and `D_binder` net linked growth
from the binder:

```
F_new = 72,148 + 173,088 - 36,864 - W - D_other - D_binder
      = 208,372 - W - D_other - D_binder
```

For a 32,768 B floor, **`W <= 175,604 - D_other - D_binder`**. That is an
optimistic upper bound, not a target; the required reduction from 577,424 is
**69.6%** before any other growth.

**The unit of thought changes from bytes to semantics.** Do not hunt a minimal
byte subset that can live behind the existing raw-file pointer ABI — that
preserves exactly the wild-pointer behaviour that made completeness unprovable.
Compile the closure offline into a closed, typed, semantic post-setup ABI and
migrate battle code onto generated accessors, so an unconverted raw-file access
is a **link failure** rather than an untested rare path. Poisoning and play
scripts become secondary falsifiers, not the proof.

**Cross-fighter references stop being a special case.** Kirby's copy-hat rows
pointing at `dLinkBoomerangModel_Joint_0x00F8_DisplayList` and `dFoxUnknown_DL`
are handled by canonical atom identity plus set union: Kirby's capability
manifest and Link's base manifest name the same atom, so it is stored once
whether Kirby, Link, or both need it. There are 495 four-kind payload
combinations but only **793 proof cases** (the 1-through-4-kind sets:
12 + 66 + 220 + 495), enumerated offline for sizing only. No 495 packs.

**The next step is a census-only estimator, not the runtime and not a poison
harness.** It indexes every typed source object, resolves explicit and
schema-defined implicit edges, classifies post-setup consumers, translates raw
presentation data to the native representation, enumerates exact
selected-costume texel and palette spans, separates VRAM-uploaded from
main-RAM-retained bytes, adds native-owner and pack-overhead bytes, unions
cross-fighter dependencies by atom ID, enumerates all 793 sets, and reports the
worst set, worst costume combination, worst feature profile and an exact byte
ledger. Its gate:

| Result | Meaning |
| --- | --- |
| unclassified atoms or consumers | STOP — no size verdict is valid |
| worst pack > `175,604 - D_other - D_binder` | RED — the pack alone cannot hold a 32 KiB floor |
| worst pack ~150-175 KiB | YELLOW — fit depends on explicit secondary recovery |
| worst pack <= ~150-160 KiB | GREEN enough to build the runtime proof |

The 150-160 KiB band is provisional; replace it with an exact threshold measured
from a four-slot pack-disabled skeleton build that isolates `D_other` and
`D_binder`.

**Layout** keeps `SIM_HOT`, `RENDER_HOT` and resident `WARM_COLD`, and adds a
distinct `SETUP_UPLOAD_TRANSIENT` section plus separate scene/UI lifetimes.

**If the estimator is yellow or red**, the levers in order are: direct
selected-texture upload to VRAM retaining only handles; splitting battle from
CSS/entry/Results lifetimes (scene transitions are legal load boundaries, and
this is not gameplay-time paging); enforcing low-detail-only battle residency;
recovering battle static address space; and compacting the motion/event
representation.

### Per-frame lane coverage, audited 2026-09-04

The review asks for a 17-lane per-presented-frame census before any simulation
rate is chosen. Ten of those lanes already exist; six do not. Audited read-only,
so nobody re-derives it:

**Already per-frame.** Source update A and B are exposed exactly as the review
guessed — `gNdsRendererProfileSourceUpdate1Ticks` / `2Ticks`, published per
presented frame at `src/port/taskman_seam_battle_host.c:1194-1195` under
**`NDS_RENDERER_PROFILE_LEVEL >= 1`** (not `NDS_TICK_HUD`), read through the
`-PerFrameGlobals` GDB path rather than a ring column. Also per-frame, via the
tick-HUD ring: fighter simulation (`GCRA`), AI (`SCPU`), physics and map
collision (`SPHD`+`SPHC`), status/interrupt (`SINT`), stage draw (`STG`),
fighter draw (`FTR`), HUD and background (`BG`+`HUD`).

**Missing, with the single bracket site each would need:**

| Lane | State | Bracket |
| --- | --- | --- |
| animation events | no counter at all | `ftMainPlayAnimEventsAll` (`src/import/battleship_ftmain.c:152`) |
| gameplay-joint pose | count only, no timing | `ndsFtPoseUpdate` (`include/nds/nds_ft_pose.h:230`) |
| weapon draw | cumulative only | `gNdsMiscWeaponDrawTicks` writer, `src/port/reloc_backend_movement.c:13007` |
| effect draw | cumulative only | `gNdsMiscEffectDrawTicks` writer, `:13015` |
| particle draw | cumulative only | `gNdsMiscParticleDrawTicks` writer, `src/import/battleship_lbparticle.c:4266` |
| GX FIFO wait | folded into `MISC` | flush span in `ndsPlatformEndFrame`, `src/nds/nds_platform.c:3727-3745` |
| asset I/O | count only, flag-gated | the load call site, not the `NDS_TASK75_MARK_ASSET_LOAD()` count macro |

Hit/catch/reflect is **partial**: hit (`SHDT`), catch (`SCAT`) and params
(`SPRM`) each have a per-frame counter, but reflect has no symbol; its sub-span
is inside `ftMainProcParams`
(`src/port/reloc_backend_ftmain_runtime.c:582`).

Extraction is `scripts/sample-tick-hud-buckets.ps1`. Prefer `-RingDump`, which
takes one GDB stop and dumps `sBattleTickHudRing` whole (`:540-543`), stitching
every `RingStopStride=96`; the per-frame path costs one stop per presented
frame, and `-PerFrameGlobals` is incompatible with `-RingDump` (`:270-275`).
