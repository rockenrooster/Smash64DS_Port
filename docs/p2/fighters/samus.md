# Samus — P2-3 fighter 4

Status: integration in progress · Reference: `decomp/BattleShip-main/decomp/src/ft/ftchar/ftsamus/`

The source gameplay slice is now linked and the real shell can select Samus.
This is **not** a completion claim: Charge Shot's source lifecycle,
Bomb/bomb-jump, and the source-specific gameplay audio bank are now accepted,
and the structural move inventory is now source-audited. The controller-driven
common movement/combat, exhaustive ordinary attack/grab/throw inventory,
ledge/down/tech recovery and budget/stress gates are accepted; the remaining
DamageFly directional variants, determinism replay and owner-feel acceptance
still need closure.

## Role

First stored-state projectile fighter: Charge Shot introduces cross-state
persistent charge plus a spawned bomb article, on a heavy-floaty chassis.

## Moveset uniques

- **Charge Shot (B)**: multi-stage charge, storable, cancel/resume, release
  size/speed/damage scale with charge; charge VFX while held.
- **Bombs (Down-B)**: morph-ball drop, timed pop, **bomb-jump must work** —
  the pop boosts Samus; recovery tech players expect.
- **Screw Attack (Up-B)**: multi-hit rise, weak knockback out.
- Heavy but floatiest fall in class; high first jump; long non-tether ledge
  reach (no grapple in 64).

## Assets & audio

Arm-cannon rig (muzzle attach point for charge VFX/shot spawn), 4 costumes,
beam/bomb SFX from source set, announcer clip.

## DS notes / risks

- Charge lifecycle across damage/KO/respawn follows the source, not a blanket
  "persistent" rule. BattleShip installs `ftSamusSpecialNProcDamage` while the
  neutral-special owner is active (it clears `charge_level` and destroys the
  held charge object); stored charge outside that active status survives an
  ordinary hit and the dead state; `ftManagerInitFighter` resets charge/recoil
  when rebirth reconstructs the fighter.
- Bomb article: physics + owner attribution through the projectile seam
  (`wp/`/`it/itfighter` — check where BattleShip keeps it).
- Charge Shot at full size is a big translucent projectile — effect-pool and
  fill-rate check on DS.

### Landed integration evidence — 2026-08-26

- The production manifest stages `SamusMain`, `SamusMainMotion`, `SamusModel`,
  `SamusShieldPose`, all three Samus special files and the 150 local animation
  O2Rs through the same generated fighter pipeline as the earlier P2-3 rows.
- `ftManagerSetupFilesAllKind(nFTKindSamus)` now reaches the physical NitroFS
  asset table. A live load proof showed Main/MainMotion/Model resident before
  fighter construction with zero external relocation failures.
- BattleShip remains the behavior owner: the port imports
  `ftsamusspecialn.c`, `ftsamusspecialhi.c`, `ftsamusspeciallw.c`,
  `wpsamuschargeshot.c`, and `wpsamusbomb.c` rather than reimplementing the
  state machines. The focused CPU proof reaches Charge Shot creation/fire and
  Screw Attack with zero relocation failures.
- Samus RollB exposed a generic AObj16 parser bug rather than a Samus data bug.
  `1014_FTSamusAnimRollB.c` places mixed 32-bit interpolation data between the
  leading joint-pointer table and the first AObj16 script. The DS normalizer now
  lane-swaps only the script suffix named by that source table; the pre-fix
  `syInterpGetFracFrame` fault no longer reproduces.
- The source CSS portrait, generic 3D preview, stock icon/LUTs and announcer are
  live. The scripted source-coordinate tour drops the 1P token on portrait 4
  (Samus), dwells through the selected process, regrabs it, exercises Link as
  the locked negative control, then returns to Mario so the standing battle
  regression remains Mario/Fox.
- `artifacts/verification/2026-08-26_p2-samus-shell-selected.txt`: Samus records
  113 selected-preview frames (`status=65540`, `motion=4`), the real shell
  completes battle -> Results -> rematch back to CSS, the committed/live match
  remains Mario/Fox, FGM misses are zero, and worst recorded scene-arena
  headroom is **273,316 B**.
- The CSS-specific source cues are packed, not substituted: announcer 513 and
  the selected-pose BladeDraw 264 were derived from BattleShip's FGM tables.
  The pack is 183 entries / 1,785,424 B with zero exclusions and a 204,800 B
  runtime cache.

### Source-owner acceptance — 2026-08-27

- `1099_FTSamusAnimBomb` exposed the second half of the generic AObj16 boundary
  rule. Its 92-byte joint-pointer table is followed *immediately* by the first
  script, so `script_bytes == table_bytes` is valid. The DS normalizer now
  rejects only `script_bytes < table_bytes`; RollB's larger interpolation gap is
  still preserved by the same source-derived split.
- `artifacts/verification/2026-08-27_p2-samus-owner-entry-charge-bomb.txt` is a
  real-input melonDS proof built with `NDS_P2_SAMUS=1` and
  `NDS_P2_PROOF_FIGHTER0=3`. It reaches BattleShip's
  `efManagerSamusEntryPointMakeEffect`, then stores Charge Shot to level 2,
  cancels at level 3 without losing it, resumes from level 3, releases a level-4
  shot through `wpSamusChargeShotLaunch` (15 damage), and leaves the held-owner
  pointer clear after launch.
- The same run enters source status 229 for Bomb with **zero** fighter-animation
  fallbacks, creates the real `wpSamusBomb` weapon, reaches source aerial Bomb
  status 230 with positive upward bomb-jump velocity, then reaches the natural
  explosion update at source lifetime 6 / size 180.0.
- The focused proof now refuses to run against an accidental Mario/Fox build:
  it verifies the generated build config admits Samus and selects her as P1
  before launching melonDS.
- `artifacts/verification/2026-08-27_p2-samus-charge-lifecycle.txt` closes the
  source-defined Charge Shot lifecycle matrix. The real-input setup stores and
  cancels a shot, then a bounded queued-damage lever is consumed by the normal
  `ftMainProcParams` path: in common Wait, `proc_damage` is null and level 3 is
  preserved through damage; while Charge Shot is active, the same source damage
  path reaches `ftSamusSpecialNProcDamage`, calls the real
  `wpMainDestroyWeapon`, and clears `charge_level` to zero. A fresh stored level
  3 then survives source DeadDown, enters `ftCommonRebirthDownSetStatus` still at
  level 3, and `ftManagerInitFighter` resets `charge_level` and
  `charge_recoil` to zero. The proof deliberately does not use an immediate raw
  GDB read of `charge_gobj` as its null-store oracle because that read can lag
  ARM9 cached stores; it proves the actual destroy call plus zero charge level,
  while the source and compiled instruction stream retain the following null
  store.

### Charge audio closure — 2026-08-27

- BattleShip's held Charge Shot hum is not a flat sample loop. FGM 239..245
  (`Charge0..6`) are infinite UCD programs with changing notes plus a D9 child
  voice (private program 673). The source fighter state supplies the bound the
  audio program itself does not: charge advances once every 20 source updates
  until level 7, and a level-7 Start goes directly to release instead of
  entering the held loop. The DS generator therefore renders only the exact
  gameplay-reachable root prefixes — **409/351/293/235/177/119/61 FGM ticks**
  for levels 0..6 — and asserts every prefix ends before the source program's
  first `jump_loop`. FGM 246 (`Charge7`) is pinned as source-unreachable for
  the held path rather than packed as dead ROM.
- Program 673 stays a separate DS handle instead of being fused into the root.
  That is required by the source pause contract: the root's `set_unk1F 226`
  carries the pause bit while 673's own values do not. Parent/child generation
  links make root stop/recycle safe; a naturally completed child detaches before
  its slot can be reused.
- The BattleShip pause/unpause seams now call the DS FGM backend. Only handles
  carrying the source pause bit freeze their source-duration clock and hardware
  channel; non-pauseable children continue. Resume shifts the root's start/end
  clocks by the paused interval and resumes the same hardware channel.
- `artifacts/verification/2026-08-27_p2-samus-owner-charge-audio.txt` proves the
  real path with melonDS keyboard input: root **239** and child **673** are live
  together; root pauseable=1 / child pauseable=0; source pause reaches
  `soundPause` on the root channel; after 700 ms of real paused run time the
  child has naturally completed and detached while the root remains paused;
  source unpause reaches `soundResume` on that same root channel. The same run
  then completes store/cancel/resume/release, Bomb/bomb-jump/explosion, active
  and stored damage ownership, KO persistence and rebirth reset.
- Generated pack/checker state is **223 entries / 2,253,212 B**, streaming into
  a **237,568 B (232 KiB)** eight-slot cache. The pack fixture SHA-256 is
  `7153da9f4986c3aac0c206e0f8329e0bc93d45b014ff35e153e72f8b4557b579`;
  all expected entries are covered with zero exclusions. REGION_US audio-ID
  fixtures pass 295 constants and the runtime audio fixtures stay green.

## Move inventory (swept 2026-08-27 against BattleShip)

Samus's ordinary move set is data-driven by the same common fighter machinery
as Mario/Fox; the Samus-specific executable state machine is the 11-entry
special table above it. The production question is therefore not "did we write
a second jab/tilt/smash implementation?" — doing that would fork the source
behavior — but whether **all of Samus's source motion data and every
Samus-specific callback owner reached the DS image without a compatibility stub
silently taking over**.

The source-derived fighter-production manifest answers the resource side:

| Source inventory | Samus |
|---|---:|
| `dFTSamusMotionDescs` entries | **206** |
| unique local animation resources | **150** |
| item-hold/swing/throw motion files already staged for P2-5 | **19** |
| Event32 motion files | **2** |
| complete NitroFS fighter-resource closure | **158** unique files |
| `dFTSamusSpecialStatusDescs` entries | **11** |

`scripts/fighters/fighter_production_manifest.json` derives those files from
BattleShip `ftdata.c`/reloc symbols rather than a hand list. All **150/150**
local animation aliases resolve to source assets. The 158-file NitroFS set is
unique and its source allocation total is **482,736 B**; the eight-resource
core closure accounts for **171,136 B** of that source allocation footprint.
`check-fighter-production-manifest.ps1` regenerates that manifest and its Make
fragment/runtime header, so source-inventory drift is a build-check failure
rather than a documentation mismatch.

The linked Samus proof image independently settles the two tables that matter:

- `dFTSamusMotionDescs` is **0x9a8 = 2,472 B = 206 × 12-byte
  `FTMotionDesc`**, exactly the source `dFTSamusData` count `0xCE`.
- `dFTSamusSpecialStatusDescs` is **0xdc = 220 B = 11 × 20-byte
  `FTStatusDesc`**, i.e. BattleShip's full table, not the inactive 16-entry
  compatibility table in the project header.
- the image contains **53 strong `ftSamus*` / `wpSamus*` text symbols and zero
  weak Samus owners**. The three fighter behavior TUs (`specialn`, `specialhi`,
  `speciallw`) and both weapon TUs (Charge Shot/Bomb) are included directly
  from `decomp/`; the only behavioral adapter in that fighter wrapper is the
  already-documented deterministic third argument for BattleShip's malformed
  two-argument Escape call.

The common motion table covers the source mobility/defense/damage/down/tech/
ledge/grab/throw families and the complete attack inventory. The Samus-specific
attack scripts in `216_SamusMainMotion.c` are present for Jab1/Jab2, Dash
Attack, all five forward-tilt angles, up/down tilt, all five forward-smash
angles, up/down smash, all five aerials, forward/back throw, and the three
special families. Their hitbox, FGM/voice and effect commands remain the
BattleShip motion-program data consumed by the shared imported motion-event
interpreter; there is no DS-side rewritten move table to drift from those
values. Item actions are deliberately **staged but not claimed runtime-tested**
here because items remain P2-5 scope.

Runtime ownership for the Samus-specific branches is already covered by the
focused owner proof: neutral-B start/loop/cancel/resume/release and projectile
ownership, grounded/aerial down-B plus bomb-jump/explosion, and Screw Attack's
source status/audio entry all execute through the strong source owners. This
closes the structural **move inventory sweep** used by the fighter-unit docs;
it does **not** claim the P2-3 plan's broader scripted every-state tour,
determinism replay or owner-feel acceptance is complete.

### Controller-driven common combat tour — 2026-08-27

The first runtime-equivalence segment now reuses the established mode-163
natural input driver with `NDS_P2_PROOF_FIGHTER0=3`. It never writes a fighter
status. The driver supplies the same controller fields a player/CPU would, and
BattleShip selects every resulting common state. This matters for Samus's grab
in particular: `ftcommoncatch1.c` owns the Z-hold + A-tap Catch interrupt, while
`ftcommonthrow.c` owns forward/back throw selection, the Samus grapple-beam
effect attachment, and victim thrown-status dispatch.

`artifacts/verification/2026-08-27_p2-3f23-samus-common-moveset-gxfix3.log`
passes with `NAT_MOVESET=0x7ff`, phase 15 Done: S3/Hi3/Lw3 17/15/19 frames,
20 active tilt-hitbox frames, S4 26 / hitbox 6, aerial 17 / hitbox 14, landing
16, Catch/CatchWait 3/1, Throw/Thrown/recovery 17/5/131, and throw damage
21->33. The surrounding natural chain also reaches Wait/Walk/Dash/Run/
RunBrake/Turn, live attack/damage/hitlag and guard. Because this focused roster
is Samus/Fox rather than Mario/Fox, Fox correctly owns the auxiliary projectile
and recovery-special phases: blaster kind 1 / mask `0x2`, Fire Fox mask
`0xf80`, and the reflector stage cleanly disables because the projectile actor
is Fox himself.

The tour also caught a DS-only native-render integration omission. Captain and
Samus already had generated hierarchy schedules, binding-parent tables and
cross-palette slots, but the runtime lookup functions stopped at Donkey. Slots
4/5 now dispatch those generated tables. On the identical Samus proof, native
GX compose changes from capture/local/decline **0/0/2** to **32/49/0** while
the source-derived High owner remains exactly **322** triangles; native
production succeeds, generic fallback remains zero, and the bounded final
capture is 63 runs / 628 triangles = Samus 322 + Fox 306. The hierarchy checker
now fails closed if Captain/Samus disappear from any of those runtime lookup
surfaces.

This is deliberately **not** the whole P2-3 every-state claim. The production
gate still needs an exhaustive runtime pass over the full attack variants and
the ledge/tumble families, then CPU-vs-CPU determinism and owner feel.

### Full runtime animation-loader closure — 2026-08-27

The structural inventory is now backed by a runtime loader sweep. Task 40's
profile-1 fighter-animation audit was generalized from Mario/Fox to every landed
fighter using the exact BattleShip `ftdata.c` counts: Mario 204, Fox 219, Donkey
221, Samus 206, Luigi 204 and Captain 214. This also fixes the old 219-row audit
capacity, which was already too small for Donkey. A new data-only mode keeps the
same `ftMainSetStatus` / relocation acquisition path but omits screenshot
handshakes, and the checker now fails closed on unrequested/unresolved non-null
motions, stale-heap fallback, external fixup, invalid figatree or unsafe data.

`artifacts/performance/2026-08-27_p2-3f24-samus-anim-audit-closure-audit.csv`
visits all **206** Samus descriptors: **201/201 non-null requested and resolved,
5 source-null, zero fallback/external-fixup/figatree-invalid/unsafe/timeout**.
The complete ledge animation bank (motions 72..87, CliffCatch through
CliffEscapeSlow2) resolves 16/16, and the DamageFly/down/tech/stun rows resolve
through the same source-ID loader. The historical Task-40 expected-duration
estimator still has documented provisional mismatches; those flags are retained
as diagnostics and are not used here as an equivalence verdict.

This closes a prerequisite, not the state-tour checkbox: the next proof still
has to enter the ledge/tumble and remaining attack variants through gameplay
transitions rather than cycling animation descriptors directly.

### Natural ledge-state tour — 2026-08-28

The ledge half of that runtime-equivalence gate is now closed without status
injection. BattleShip `ftcommoncliffcatchwait.c` is the behavioral authority:
CliffWait A/B selects attack, Z selects escape, inward/upward stick selects
climb, and damage below/at-or-above 100 selects the Quick/Slow family. The
proof-only `NDS_P2_SAMUS_STATE_TOUR` driver does not assign `status_id` or
`motion_id` and never calls `ftMainSetStatus`; `verify-p2-samus-state-tour.ps1`
also rejects those patterns statically before launching the ROM.

Each of six scenarios begins only after the established controller-driven
common tour has reached `NAT_MOVESET=0x7ff`. From source Wait, ordinary right
input runs Samus off Dream Land and the source map path selects Fall. Only then
does guest code establish the cache-coherent geometry/damage/facing
precondition for one descending right-cliff sweep. The normal DS map update and
BattleShip collision path must select CliffCatch and CliffWait; controller input
then selects Quick Attack/Escape/Climb or Slow Attack/Escape/Climb. The guest
never primes the cliff/action status being claimed.

`artifacts/verification/2026-08-28_p2-3f25-samus-natural-ledge-tour.txt` banks
the permanent read-only-GDB proof: **6/6 scenarios**, exactly **12** guest
precondition stages (two per scenario), full source-state mask **`0x1ffff`**,
prerequisite **`NAT_MOVESET=0x7ff`**, and **0 stalls**. The mask covers Fall,
CliffCatch, CliffWait, CliffQuick/CliffSlow and both stage-1/stage-2 states for
all six attack/escape/climb families. The exact proof ROM SHA-256 is
`A125C7E3DFD2C57E1B0F65D497994830C3F9E2274334748FBEBD1F228BE7ACB7`.

This closes ledge visitation only. DamageFly/tumble, down/bounce/getup and
Passive/tech visitation still need their own source-selected gameplay tour, and
the remaining attack variants still need explicit runtime coverage.

### Natural tumble/down/tech recovery tour — 2026-08-28

The recovery half of the tumble gate is now source-selected from a real hit.
BattleShip `ftcommondamage.c` is the damage-state authority; `ftcommondamagefall.c`
owns landing selection, `ftcommonpassive.c` / `ftcommonpassivestand.c` own the
three tech branches, and `ftcommondownwaitbounce.c` plus the down-state helpers
own prone recovery. The proof-only `NDS_P2_SAMUS_TUMBLE_TOUR` arm is mutually
exclusive with the ledge tour and starts only after the established common
controller tour reaches `NAT_MOVESET=0x7ff`.

Every one of **11** scenarios first stages Samus and Fox on Dream Land, then
uses ordinary Fox Up+A input. The real fighter attack collision must put Samus
into a DamageFly-family source status; only after BattleShip advances that hit to
`DamageFall` may guest code establish the descending floor/orientation
precondition. No status/motion assignment, `ftMainSetStatus`, damage-status
setter, or fighter-damage injection is permitted; the permanent verifier rejects
those patterns before booting the ROM.

The three tech scenarios use BattleShip's buffered-Z law to select Passive,
PassiveStandF and PassiveStandB. The remaining eight scenarios intentionally
arrive without a tech buffer, so BattleShip selects DownBounceD/U and then
DownWaitD/U; controller input from DownWait selects Stand, Forward, Back or
Attack for each prone orientation. One proof bug was source-ordering, not game
behavior: DownBounce changes to DownWait at the end of `gcRunAll`, while the next
controller frame is applied before the proof recorder runs. A one-update
observation latch now lets the recorder see DownWait before issuing the same
source getup input.

`artifacts/verification/2026-08-28_p2-3f26-samus-tumble-recovery-tour.txt`
banks the permanent read-only-GDB result: **11/11 scenarios**, **11** real Fox
hits, exactly **22** guest precondition stages, full recovery mask
**`0x1ffff`**, DamageFly entry mask **`0x8` = DamageFlyTop**, prerequisite
**`NAT_MOVESET=0x7ff`**, and **0 stalls**. The exact proof ROM SHA-256 is
`B8D1D023B8C7EE0680EF34A2BE79F4BCF51E7E2C276F8D23BA36DA27EEBBC94C`.

This closes down/tech recovery and proves a real tumble entry. It does **not**
claim DamageFlyHi/Lw/N/Roll have all been visited; those directional damage
variants remain part of the outstanding exhaustive attack/tumble inventory.

### Exhaustive ordinary attack/grab/throw tour — 2026-08-28

The ordinary attack half of the every-state gate is now closed through source
controller transitions rather than status cycling. The proof-only
`NDS_P2_SAMUS_ATTACK_TOUR` arm covers **23 scenarios / 24 status bits**: Jab1/2,
Dash Attack, all five S3 angles, Hi3/Lw3, all five S4 angles, Hi4/Lw4, all five
aerials, Catch/CatchPull/CatchWait, and forward/back throw. The input driver
never assigns `status_id`/`motion_id` and never calls `ftMainSetStatus`; the
permanent verifier rejects those patterns before booting the ROM.

BattleShip remains the authority. The common attack interrupt code selects the
ground/aerial variants from controller stick/button history, `ftcommoncatch1.c`
installs Catch from real Z+A input, `ftcommoncatch2.c` installs CatchPull only
after the real catch collision finds Fox, and `ftcommonthrow.c` selects F/B
throw and victim thrown-status dispatch. A tiny post-`ftMainSetStatus` observer
is proof-only and read-only; it exists because CatchPull can begin and finish
inside one `gcRunAll`, before the once-per-update sampler can see it.

The tour exposed two shared compact-pose semantic bugs. First, the DS track had
stored BattleShip `AObj.length_invert` and its linear rate in the same word.
Those are distinct source fields and zero-payload commands intentionally leave
the untouched one unchanged; Samus Catch contains such a command. The compact
track is therefore 24 bytes now and preserves both fields independently.

Second, and gameplay-critical, fighter pose ownership was too broad. BattleShip
attaches a fighter figatree by walking the TopN hierarchy, but
`ftParamUpdateAnimKeys` later plays the complete indexed `fp->joints[]` table.
Samus's grapple joint **36** is a live indexed joint outside the compact Catch
hierarchy. The old DS path saw the hierarchy as compact-owned and skipped
generic animation for joint 36, so the grab collision never followed the source
grapple pose. `NdsFtPose` now records exactly which source joint IDs were bound;
only those joints skip generic playback. Omitted/alternate indexed joints keep
BattleShip's AObj path. This is a shared engine fix and applies to every fighter.

`artifacts/verification/2026-08-28_p2-3f27-samus-exhaustive-attack-throw-tour.txt`
reports **23/23 scenarios**, full status mask **`0xffffff`**, exactly **23**
scenario stages, Catch/CatchPull/CatchWait mask **`0x7`**, **70** sampled Catch
frames, two active grapple collision slots (`0x3`) both attached to source
joint 36, prerequisite **`NAT_MOVESET=0x7ff`**, and **0 stalls / 0 pose-track
overflow**. The exact proof ROM SHA-256 is
`D02B03D465326B110827CF484B94350205F6E3E7BFEB3665BFE78D76C8808CB4`.

The compact-player shadow oracle independently reaches **43,146** comparisons
with **zero transform mismatches**, zero runaway, saturation, overflow or
BindFull. Its stricter all-field lane still records 297 animation-clock
`-0.0`/`+0.0` bit mismatches; those are retained as exact diagnostics and no
longer hide the transform verdict. Artifact:
`artifacts/verification/2026-08-28_p2-3f27-samus-pose-oracle-final.txt`.

Because the source-faithful track representation grew from 20 to 24 bytes, the
standing four-kind stress was rebuilt rather than reusing P2-3f22's ledger.
Samus/Fox/Captain/Donkey again draw all four slots (`0xF`) for the accepted
one-minute window, clock **60->1**, with general-heap low-water **207,044 B**
(**181,444 B above** the 25,600 B floor), pose binds/full **714/0**, graphics
heap overflow/no-room **0/0**, native-plan build/hit/mismatch **630/6,661/0**,
and zero hard allocator/object/AObj failures. ROM SHA-256
`FA1F83F6AEC09BE65347A433EE9313B0676838DA2EEEE54E922D60E1EE105F3C`;
artifacts are `2026-08-28_p2-3f27-fourcpu-{buckets,coverage,memory}.json` and
the matching rows CSV.

### Complete DamageFly family — 2026-08-28

The last four unvisited directional tumble states are now closed with a
separate controller-only proof arm, `NDS_P2_SAMUS_DAMAGEFLY_TOUR`. The source
rule is exact: `ftMainSearchHitFighter` scans Samus's `damage_colls[]` in array
order and stores the first colliding hurtbox's `placement` as `damage_index`.
`ftcommondamage.c` maps level-3 placement **0/1/2** to
DamageFlyLw/N/Hi, then overrides the result to DamageFlyTop for launch angles
strictly between 70 and 110 degrees, or to DamageFlyRoll at >=100% when the
source `syUtilsRandFloat() < 0.5` branch succeeds.

The proof follows those laws rather than writing their outputs. For Hi, Fox
performs a real C-button jump and starts N-air only after natural jump physics
has put him above Samus, so the first source hurtbox intersected is placement-2
head instead of the earlier placement-1 torso. Neutral F-tilt naturally reaches
placement 1 for N. Down-smash reaches placement-0 leg geometry for Lw. Up-smash
supplies the source 80-degree Top override. Roll repeats a real non-vertical
F-tilt from the 100% precondition until BattleShip's own 50% branch fires. The
permanent verifier statically rejects status/motion assignment, direct damage
injection, Damage status setters, and any proof-side RNG call.

Two consecutive runs report the identical cache-coherent terminal:
`SAMUS_DAMAGEFLY_TOUR=5,6,15,1,0x1f,0xf,0x120a,5,1,3,1,109,0,8,1,10,0x7ff,0,0`.
That is all five family states (`0x1f`), all four source attacker paths (`0xf`),
Hi/N/Lw placements **2/1/0**, five real hits, one source Roll attempt, three
361-degree hits (head N-air, N F-tilt, Roll F-tilt), one 80-degree Top hit,
Roll selected at 109% after source hit damage, zero mismatches, prerequisite
`NAT_MOVESET=0x7ff`, zero stalls and zero pose-track overflow. Proof ROM SHA-256
is `D97E71A7E823AF4E8D76758534337EC2BA2E6EB6DD1C1B43D4870815061C871B`;
artifacts are `2026-08-28_p2-3f28-samus-damagefly-tour.txt` and the independent
repeat `2026-08-28_p2-3f28-samus-damagefly-tour-repeat.txt`.

## Acceptance

- [x] Move inventory sweep vs `ftsamus`/`ftdata` data: 206 motion descriptors,
      150 local animation resources, 158 unique NitroFS files and the complete
      11-status Samus table are source-derived and present in the linked image;
      all 53 linked Samus fighter/weapon owners are strong (2026-08-27).
- [x] Charge store/cancel/resume/release and source-defined damage/death/rebirth
      lifecycle matrix equivalent.
- [x] Bomb-jump reproduces through real keyboard input -> DS controller path,
      with source Bomb creation and natural explosion lifecycle.
- [x] CSS selectable with source portrait, source fighter-name + Metroid-series
      emblem gate art, live 3D selected preview, stock art, source
      announcer/selected-pose audio, and rematch return path.
- [x] Full source-reachable Samus-specific gameplay audio bank closed; the
      source-unreachable held Charge7 program is explicitly audited, not
      approximated or packed as dead content.
- [x] Native-owner budget + one-minute six-kind stress measurement banked.
- [x] Controller-driven common movement/combat/grab/throw segment reaches
      `NAT_MOVESET=0x7ff` with source-owned transitions and no native GX fallback.
- [x] Full runtime animation-loader closure: 201/201 non-null Samus motions
      resolve with zero silent fallback or loader-safety failures.
- [x] Natural ledge-state tour: source-selected Fall/CliffCatch/CliffWait plus
      all quick/slow attack, escape and climb stages; mask `0x1ffff`, zero stalls.
- [x] Natural tumble/down/tech recovery tour: 11/11 real-hit scenarios cover
      DamageFall, tech F/N/B, DownBounce/DownWait and every U/D getup branch;
      recovery mask `0x1ffff`, zero stalls.
- [x] Exhaustive ordinary attack/grab/throw tour: 23/23 scenarios, all 24
      source attack/throw status bits (`0xffffff`), Catch chain `0x7`, zero
      stalls; shared compact-pose ownership is source-joint exact.
- [x] Complete DamageFly family: Hi/N/Lw/Top/Roll through real source attacks,
      exact placement 2/1/0 selection and BattleShip-owned Top/Roll overrides.
- [ ] CPU determinism replay; owner feel pass.
