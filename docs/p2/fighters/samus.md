# Samus — P2-3 fighter 4

Status: integration in progress · Reference: `decomp/BattleShip-main/decomp/src/ft/ftchar/ftsamus/`

The source gameplay slice is now linked and the real shell can select Samus.
This is **not** a completion claim: Charge Shot's source lifecycle,
Bomb/bomb-jump, and the source-specific gameplay audio bank are now accepted,
and the structural move inventory is now source-audited; the broader scripted
every-state tour plus budget/stress/determinism/owner-feel acceptance still need
closure.

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
determinism replay, budget/stress gate or owner-feel acceptance is complete.

## Acceptance

- [x] Move inventory sweep vs `ftsamus`/`ftdata` data: 206 motion descriptors,
      150 local animation resources, 158 unique NitroFS files and the complete
      11-status Samus table are source-derived and present in the linked image;
      all 53 linked Samus fighter/weapon owners are strong (2026-08-27).
- [x] Charge store/cancel/resume/release and source-defined damage/death/rebirth
      lifecycle matrix equivalent.
- [x] Bomb-jump reproduces through real keyboard input -> DS controller path,
      with source Bomb creation and natural explosion lifecycle.
- [x] CSS selectable with source portrait, live 3D selected preview, stock art,
      source announcer/selected-pose audio, and rematch return path.
- [x] Full source-reachable Samus-specific gameplay audio bank closed; the
      source-unreachable held Charge7 program is explicitly audited, not
      approximated or packed as dead content.
- [ ] Budgets + stress measurement banked; owner feel pass.
