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

## Lab smoke — 2026-09-02

- `build-ness-cpu` (`NDS_P2_NESS=1`, proof fighter 11, `NDS_R2_BOTH_CPU=1`):
  level-3 Ness versus level-3 Fox on Dream Land, tick-HUD sampler, 32 samples
  over presented frames 438..469, DLDI on. **No abort, `slips=0`**, VBlank
  intervals 2:66 3:191 4:190 5+:22 (max 17) over 469 presented frames.
- Clean counters: audio 28 play calls, **28 supported**, 0 unsupported / 0
  play-fail / 0 lookup-fail / 0 miss-ring; reloc 0 stream failures, 0 format
  failures; effects 0 range rejects, 0 matrix rejects; weapon and item
  renderer rejected draws 0. All **158** assets his manifest names are packed
  in the lab nitrofs (checked against the build's own directory).
- Two counters are non-zero and have no baseline on this tree, so they are
  recorded rather than claimed clean: `gNdsRelocAssetOpenFailCount=17` (an
  asset id absent from the runtime table, not a missing file -- none are
  missing) and `gNdsFtrPreValidateReject=400` over 469 frames.
- **Not a gate figure.** ALL p50 is 2,237,952 ticks per presented frame on
  this arm, about four VBlanks; the window is 32 samples immediately after
  entry on a both-CPU stress configuration with the tick HUD and DLDI on.
  Cadence for the roster belongs to the Boundary arm and to the P2 stress
  gate, measured separately once row P2-3f49 seats the full roster.
- **Item seam:** admitting Ness moved BattleShip's shared item subsystem off
  Link's flag. `battleship_item_link_core.c` (itmain/itmap/itmanager/
  itprocess/itvisuals, verbatim) now compiles under the derived
  `NDS_P2_ITEM_CORE` -- any fighter with an item article -- and LinkBomb's own
  status dispatch stays behind `NDS_P2_LINK`. PK Fire is its second client.

## Acceptance

- [ ] Move inventory sweep vs `ftness` data.
- [ ] PK Thunder steering + PKT2 launch/armor equivalent (replay-verified).
- [ ] Absorb table correct for every existing projectile.
- [ ] Budgets + stress measurement banked; CSS live; owner feel pass.
