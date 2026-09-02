# Yoshi — P2-3 fighter 7

Status: production inventory staged behind `NDS_P2_YOSHI` (files, reloc rows, attributes pin); native owner blocked on the source pre/post DL-pair draw mode (next row) · Reference: `decomp/BattleShip-main/decomp/src/ft/ftchar/ftyoshi/`

## Role

The rules-exception fighter: unique shield, armored double jump, no third
jump. Exercises every "special-case the shared systems" seam.

## Moveset uniques

- **Egg Lay (B)**: swallows the opponent and turns them into an egg —
  opponent-side state (egg struggle/mash-out, damage-in-egg rules). Two-body
  state machine like DK cargo; reuse that seam pattern.
- **Egg Throw (Up-B)**: lobbed egg projectile, arc control; **no recovery
  height** — Yoshi's recovery is his armored double jump instead.
- **Yoshi Bomb (Down-B)**: ground-pound with star spray on landing.
- **Egg Shield**: shield is an egg — does not shrink like normal shields;
  its own poke/break rules (verify exact 64 semantics in source).
- **Double-jump armor**: heavy knockback armor during the second jump — the
  recovery identity; exact armor thresholds from source.

## Assets & audio

Round model, 6 costumes (verify count), Yoshi voice samples, egg VFX,
announcer clip.

## DS notes / risks

- Shield exception must live at the shared shield seam as a declared
  variant, not a Yoshi-local copy of shield code.
- Armor is a knockback-pipeline exception — verify it composes with items
  (Star invincibility, later) and Sudden Death.
- Egg (opponent) state must handle KO-in-egg, timer expiry, thrower KO'd.

## Source-derived inventory — 2026-09-02

- `BOOTSTRAP_FIGHTERS` gains Yoshi: the manifest carries 9 core files
  (`llYoshiMainFileID` 0xf7, Model 0x152, Special2 0x162, Special3 0x153,
  ShieldPose 0x154, MainMotion 0xf6 ...), 142 motion files
  (`FTYoshiAnim` 0x717..0x7a4, 2 event32 motion files, 19 item motion
  files), 151 nitrofs files, `attributes_offset` 1148 (0x47c: 247_YoshiMain.c's
  pre-attributes data ends with the 12-byte skeleton table at 0x470).
- `reloc_backend_assets.c` gains his rows behind `NDS_P2_YOSHI` (anim/core/
  dependency token rows, AObj32 test, payload/alloc size rows, attributes
  offset arm) and the source-literal FTAttributes pin: dead voice 595 /
  DeadSlam FGM 297, DeadUp 588, Damage 590, Smash 584..586, HeavyGet 593,
  item-throw scales 0x64 (REGION_US gmsound.h ordinals; his voice run is
  583 Appeal .. 602 UnkVocalize, announcer 535, crowd 614).
- Owner generator: `P2_O2R_ASSETS` (YoshiModel, file 0x152, sha e2654cbd),
  High JointTree 0x33a0 / Low 0x6948 (28 raw descriptors + sentinel),
  `dYoshiMain_setup_parts` 0xFBFFFFE0 (descriptor 26 omitted -- a bit-walk
  owner like Samus/Link), 27 selected parts + TopN, 18 drawable roots.
- **Blocker, the next row:** `dYoshiMain_commonparts_container` sets flags
  0x01 for both details, so `ftDisplayMainDrawDefault` draws EVERY Yoshi
  joint through case 1: `DObjDesc.dl` targets a `{ dls[0], dls[1] }` pair
  (338_YoshiModel.c:1626, 19 pairs at 0x3308) -- dls[0] draws under the
  parent's matrix before `gcPrepDObjMatrix`, dls[1] after. The decoder
  assumed one DL per joint and walks the pair table as commands ("root
  0x3308 exceeds 255 commands"). Only Yoshi (and NYoshi/NPikachu/Boss,
  P2-6) use the pair form. The generator needs a pair-aware root
  discovery: post-DL as the joint's root, pre-DL as an extra root under the
  parent joint's matrix (a Z-buffered DS makes the draw order irrelevant
  for opaque geometry). Until then he is out of `P2_OWNER_MODEL_CENSUS`.
- Shared seams already live in the port: the egg shield branches in
  `battleship_ftcommon_guard.c` (source guard1/guard2 included), the
  double-jump armor (`ftYoshiJumpAerialProcPhysics`,
  `FTYOSHI_JUMPAERIAL_KNOCKBACK_RESIST` 140 under REGION_US), and Falcon's
  capture TU pattern (`battleship_ftcommon_capturecaptain.c`) for the
  opponent-side `ftcommoncaptureyoshi.c`; `ftCommonCaptureYoshi*` /
  `ftCommonYoshiEgg*` are still `NDS_INACTIVE_STATUS_STUB`s.
- Effects: `efManagerYoshiShieldMakeEffect` and `efManagerEggBreakMakeEffect`
  are DS base effects already; `YoshiEntryEgg` (Special2), `YoshiEggLay`
  (Special3), `YoshiEggEscape` (Model) descs and `efManagerYoshiEggExplode`
  (a weak NULL stub today) are the admission's effect work.
- Audio: 36 source ids (FGM 82/115/130/252..257/297/308, voices 583..602,
  announcer 535 + team 531, crowd 614); Samus/Pikachu bank pattern.

## Acceptance

- [ ] Move inventory sweep vs `ftyoshi` data.
- [ ] Egg Lay two-body matrix (mash-out, KO, edge cases) equivalent.
- [ ] Egg Shield + DJ armor thresholds equivalent.
- [ ] Budgets + stress measurement banked; CSS live; owner feel pass.
