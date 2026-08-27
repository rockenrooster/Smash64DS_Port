# Samus — P2-3 fighter 4

Status: integration in progress · Reference: `decomp/BattleShip-main/decomp/src/ft/ftchar/ftsamus/`

The source gameplay slice is now linked and the real shell can select Samus.
This is **not** a completion claim: Bomb/bomb-jump and the full Charge Shot
persistence matrix still need runtime acceptance, and the rest of Samus's
source-specific gameplay audio bank is still being closed.

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

- Charge persistence across hitstun/KO/respawn per source (players know the
  rules; verify, don't guess).
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

## Acceptance

- [ ] Move inventory sweep vs `ftsamus` data.
- [ ] Charge store/cancel/resume/release matrix equivalent; persistence rules
      verified.
- [ ] Bomb-jump reproduces (scripted input replay).
- [x] CSS selectable with source portrait, live 3D selected preview, stock art,
      source announcer/selected-pose audio, and rematch return path.
- [ ] Full Samus-specific gameplay audio bank closed with no runtime misses.
- [ ] Budgets + stress measurement banked; owner feel pass.
