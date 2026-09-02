# Ness — P2-3 fighter 8

Status: gameplay, native owner, shell surfaces and audio bank admitted behind `NDS_P2_NESS` (roster-close slice, not yet smoked) · Reference: `decomp/BattleShip-main/decomp/src/ft/ftchar/ftness/`

## Role

The most complex recovery in the game (player-steered projectile that turns
into a self-launch) plus the only absorb mechanic. Unlockable (gating P2-7).

## Moveset uniques

- **PK Thunder (Up-B)**: player steers the bolt head continuously; head has
  a hit, tail has hits; striking Ness himself triggers **PKT2** — a big
  armored self-launch used as recovery and as an attack. Steering feel is
  gameplay, not presentation: input→curvature must be equivalent.
- **PK Fire (B)**: bolt that blooms into a damage pillar on contact; angled
  differently in air (verify).
- **PSI Magnet (Down-B)**: absorbs energy projectiles, heals by absorbed
  damage; needs the projectile-classification flag (energy vs physical) to
  exist on every projectile in the game — add the flag at the projectile
  seam now, populate per article.
- **Yo-yo smashes** (up/down): charge-hold behavior at edges (yo-yo dangles)
  per source; famous edge interactions.
- Floaty double jump with momentum carry; strong b-throw.

## Assets & audio

Kid model, cap costumes, PSI VFX set (hexagon sparks), "PK Thunder!" voice
lines, announcer clip.

## DS notes / risks

- PKT2 collides with terrain mid-flight — trajectory + armor + wall-stop
  rules from source; test on every landed stage's underside geometry.
- PSI Magnet's absorb flag audit touches all prior fighters' projectiles —
  schedule as a sweep row, cheap but wide.

## Admission — 2026-09-02 (roster-close slice, `admit_fighter.py`)

- Manifest: 9 core files (`llNessMainFileID` 0xef, MainMotion 0xee, Model
  0x14f, ShieldPose 0x151, Special1 0xf0 = PK Fire item attributes, Special2
  0x160, Special3 0x150), 151 motion files (`FTNessAnim` 0x680..0x716, 5
  event32, 19 item), 160 nitrofs files, `attributes_offset` 0x5bc.
- Gameplay TUs, BattleShip verbatim behind `NDS_P2_NESS`: `battleship_ness.c`
  (ftnessspecialn/hi/lw + the transcribed AppearR/L entry statuses),
  `battleship_ness_weapons.c` (wpnesspkfire + wpnesspkthunder; NessMain
  tokens PK Thunder 0x0c, trail 0x40; PK Fire's spawn weapon 0x00 in
  Special1) and `battleship_ness_items.c` (itnesspkfire, the pillar item,
  attributes token 0x34 in Special1). Status table promoted; kind 11.
- Native owner slot **9**, image slot **7**: NessModel (file 0x14f), High
  JointTree 0x26b0 / Low 0x4fe8 (28 descriptors + sentinel), setup parts
  0xffffffc0, 27 selected parts, 14 drawable roots; hierarchy 1/7/7, **no
  cross bindings**, census High 36/136/37/318 / 954 corners, Low
  40/176/28/199 / 597. Owner inventory green.
- Shell: HUD stock LUTs (texture 0xC088, sprite 0xC188), CSS bake fkind 11
  (portrait, Ness emblem, name text, gate count +1), Selected demo clip 437;
  PlayersVS residency + fkind filter; effect descs on the roster list.
- Audio: 29 own cues + 7 shared (25/27/57/207/220/627/636), bare forks
  rendered (111->105, 124->116, 223->89, 293->287, 304->298, 636->630);
  458 FuraSleep is a 16 kHz body like Yoshi's 596.
  **ACCEPTED DELTA (audio):** 221 NessPKThunderLoop is an infinite
  sequencer; it ships as a lifetime-bounded prefix (`LOOP_PREFIX_CUES`,
  `WPPKTHUNDER_LIFETIME` 160 + `FTNESS_PKTHUNDER_END_DELAY` 30 ticks, loop
  body unrolled by `unroll_loop_prefix_ucd`).

## Acceptance

- [ ] Move inventory sweep vs `ftness` data.
- [ ] PK Thunder steering + PKT2 launch/armor equivalent (replay-verified).
- [ ] Absorb table correct for every existing projectile.
- [ ] Budgets + stress measurement banked; CSS live; owner feel pass.
