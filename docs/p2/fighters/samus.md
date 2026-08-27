# Samus — P2-3 fighter 4

Status: integration in progress · Reference: `decomp/BattleShip-main/decomp/src/ft/ftchar/ftsamus/`

The source gameplay slice is now linked and the real shell can select Samus.
This is **not** a completion claim: Charge Shot's source lifecycle and
Bomb/bomb-jump are runtime-accepted, but the rest of Samus's source-specific
gameplay audio bank, move inventory, and budget/stress acceptance still need
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

## Acceptance

- [ ] Move inventory sweep vs `ftsamus` data.
- [x] Charge store/cancel/resume/release and source-defined damage/death/rebirth
      lifecycle matrix equivalent.
- [x] Bomb-jump reproduces through real keyboard input -> DS controller path,
      with source Bomb creation and natural explosion lifecycle.
- [x] CSS selectable with source portrait, live 3D selected preview, stock art,
      source announcer/selected-pose audio, and rematch return path.
- [ ] Full Samus-specific gameplay audio bank closed with no runtime misses.
- [ ] Budgets + stress measurement banked; owner feel pass.
