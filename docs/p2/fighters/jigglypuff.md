# Jigglypuff — P2-3 fighter 9

Status: gameplay, native owner, shell surfaces and audio bank admitted behind `NDS_P2_PURIN` (roster-close slice, not yet smoked) · Reference: `decomp/BattleShip-main/decomp/src/ft/ftchar/ftpurin/`
(`purin` = Jigglypuff's Japanese name — the decomp uses it throughout.)

## Role

Multi-jump aerial drift specialist; mechanically small after Ness/Yoshi, so a
quick close-out before Kirby. Unlockable (gating P2-7).

## Moveset uniques

- **Five midair jumps** — shares the multi-jump machinery Kirby will reuse;
  land it here first.
- **Pound (B)**: horizontal drift attack, air-stall/recovery tool.
- **Sing (Up-B)**: radius sleep on grounded opponents, no recovery
  properties; sleep state (opponent-side) lands here and is reused by items
  later if applicable.
- **Rest (Down-B)**: instant sleep with a 1-frame-active point-blank KO
  hitbox — frame-1 exactness matters; this move is a community benchmark.
- Lightest fighter, slowest fall, strongest air drift; shield-break launch
  (instant upward KO on shield break — verify the special-case in source).

## Assets & audio

Tiny model (cheapest in roster), 4 costumes (flower/bow/hat), Jigglypuff
voice samples, sleep VFX ("Zzz"), announcer clip.

## DS notes / risks

- Rest's frame-1 hitbox and Sing's wake rules are exactness-sensitive; take
  them from frame data directly.
- Sleep (opponent state) interacts with damage/mash-wake scaling — shared
  seam, will be reused (Mr. Saturn? no — but Sing + future items), keep it
  in `ftcommon` equivalent.

## Admission — 2026-09-02 (roster-close slice, `admit_fighter.py`)

- Manifest: core files `llPurinMainFileID` 0xe9, MainMotion 0xe8, Model
  0x14a, ShieldPose 0x14b, Special2 0x15f (Sing effect); 144 motion files
  (0x4ef..0x5e7, 2 event32, 19 item), 150 nitrofs files,
  `attributes_offset` 0x474. **The corpus labels her animations
  `FTKirbyCopyAnim000..058` (1445..1503) then `FTPurinAnim000..007`
  (1504..1511)**: the manifest gained two-segment stems (`O2R_ANIM_STEMS`,
  `_ANIM_SPLIT_ID` / `_ANIM_PATH_STEM2`) and the reloc anim arm follows them.
- Gameplay TU `battleship_purin.c` (ftpurinspecialn/hi/lw: Pound, Sing,
  Rest) behind `NDS_P2_PURIN`; status table promoted; kind 10. The victim
  half of Sing is the shared `ftCommonSleepProc*` (inactive stub now
  `#if !NDS_P2_PURIN`).
- Native owner slot **10**, image slot **8**: PurinModel (0x14a), JointTree
  High 0x2028 / Low 0x40a0 (27 descriptors), setup parts 0xeff9ff80, 23
  parts, 7 drawable roots; hierarchy 1/4/4, 4 cross bindings (slots 16..19,
  both details), 38 per-corner restores; census High 21/58/44/319 / 957
  corners, Low 21/44/31/200 / 600.
- Shell: HUD stock LUTs (texture 0x7A88, sprite 0x7BB0, five palettes),
  CSS bake fkind 10 (portrait, Pokémon emblem, name text
  `llMNPlayersCommonJigglypuffTextSprite`), Selected demo clip 470.
- Audio: 23 own cues + 5 shared (91/102/233/234/638); 569 FuraSleep is a
  16 kHz body like Yoshi's 596.

## Acceptance

- [ ] Move inventory sweep vs `ftpurin` data.
- [ ] Rest frame-1 KO reproduces (replay-verified); Sing radius/duration
      equivalent.
- [ ] Multi-jump count/decay equivalent.
- [ ] Budgets + stress measurement banked; CSS live; owner feel pass.
