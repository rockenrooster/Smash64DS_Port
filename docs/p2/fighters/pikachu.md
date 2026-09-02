# Pikachu — P2-3 fighter 6

Status: source specials, both articles, CSS/HUD surfaces, the native owner and the complete FGM/voice bank admitted behind `NDS_P2_PIKACHU`; tours, Master Ball entry article and owner feel next · Reference: `decomp/BattleShip-main/decomp/src/ft/ftchar/ftpikachu/`

## Role

The terrain-following projectile and the double-teleport recovery — two
mechanics nothing earlier in the order exercises. Small hurtbox extreme.

## Moveset uniques

- **Thunder Jolt (B)**: hops along the ground, follows terrain contours,
  crawls down walls and around ledges, dissipates on time/impact; air
  version arcs then crawls on landing. Terrain-following = per-step collision
  queries — precompute per-stage crawl paths if profiling demands (allowed by
  the optimization doctrine; behavior must stay equivalent).
- **Thunder (Down-B)**: cloud spawns at top, bolt descends to Pikachu, hits
  along its length; self-hit interplay (bolt striking Pikachu has its own
  hit) per source; screen-length article.
- **Quick Attack (Up-B)**: two chained teleport segments with distinct angle
  choice, brief vulnerability rules, no hitbox (in 64 — verify).
- Small, fast, strong edge game; famous u-smash/b-throw KO power.

## Assets & audio

Small model (cheapest draw in roster), 4 costumes (party-hat variants),
voice = actual "Pika" samples (identity-critical), announcer clip.

## DS notes / risks

- Thunder's tall bolt: fill-rate + effect pool; consider 2D-composited bolt
  (billboard doctrine) — visual doctrine allows it if telegraphs stay exact.
- Quick Attack across platforms/walls — teleport collision resolution
  equivalence.
- Thunder Jolt on moving/irregular stages (Congo barrel area, Zebes acid
  slopes) — per-stage crawl verification rows when those stages land.

## Source-derived inventory — 2026-09-01

The production generator derives Pikachu from the same BattleShip tables and
O2R inputs as the landed fighters; no runtime-completion claim is implied yet.

- `dFTPikachuData` pins the source `FTAttributes` block at **0x41c**
  (`243_PikachuMain.c`); `llPikachuMainFileID` is **0xf3**.
- Core closure is PikachuMain/MainMotion/Model/ShieldPose/Special1/2/3 with no
  external dependency file.
- **141** local animation files resolve from **0x7a5..0x831**; the complete
  fighter closure is **150 unique NitroFS files**, including **19**
  item-motion files and **2** Event32 animations (Appear1/Appear2).
- `dFTPikachuSpecialStatusDescs` has **18** entries: AppearR/L, Thunder Jolt
  ground/air, Thunder start/loop/hit/end ×2, and Quick Attack start/zip/end ×2.
  The source table's own comment records that its SpecialHi/SpecialLw attack
  IDs are swapped; that is the game's data and is imported as-is.
- The exact source `PikachuModel` O2R is SHA-256
  `12c543dc39b62b7669cc5453d97af142a1af987c4f5a8098814da214e14da9f1`, file
  id 0x155. `dPikachuMain_setup_parts = {0xFFFFFFC0,0}` is a plain 26-bit
  prefix over 27 raw descriptors (JointTrees High `0x2650` / Low `0x5490`),
  the same shape as Fox: **27 live joints including synthetic TopN** and
  **16 drawable bindings** in both details, GX seed/push/pop **1/8/8**.
- Pikachu is the first owner whose source welds adjacent parts in *both*
  details: eleven logical bindings (0,1,2,3,4,7,8,9,10,12,13) are read across
  six root pairs, so the owner needs **11 cross-matrix stores** (palette slots
  16..26). High closes at **317 triangles** (25 cross runs, **130 restores**);
  Low at **197 triangles** (16 cross runs, **106 restores**).
- Pikachu's head root `0x1c40` is the first owner display list to set the
  combiner `(0xfc121605, 0xff17ffff)` — TEXEL0×SHADE colour with
  TEXEL0α×SHADEα alpha. It is pixel-identical to the textured-lit family
  because SHADEα is the vertex alpha, which is 0xff or the raw 0 the port's
  vertex decode already maps to 0xff (the opaque-surface render mode ignores
  pixel alpha, and family 3 has shipped on that identity since Mario). The
  generator canonicalises the pair through `DIRECT_POLICY_COMBINE_ALIASES`
  and proves the identity per aliased triangle, so the runtime's per-run
  combine validation and two-bit family index are untouched.
- Attribute audio (`243_PikachuMain.c`): dead voice 550 / DeadSlam FGM 0x126,
  DeadUp 542, Damage 544, Smash 537/538/539, HeavyGet 548, item-throw scales
  0x64/0x64 — asserted by the FTAttributes normalizer when admitted.

## Source-gameplay admission — 2026-09-02

Behind `NDS_P2_PIKACHU=1` (opt-in; not in the shell roster ladder yet):

- `src/import/battleship_pikachu.c` includes the three BattleShip special
  bodies verbatim (`ftpikachuspecialn/hi/lw.c`); `battleship_pikachu_weapons.c`
  includes `wppikachuthunder.c` + `wppikachuthunderjolt.c`. Constants are the
  US values of `ftpikachu.h` / `wpvars.h`; nothing is re-implemented.
- `dFTPikachuSpecialStatusDescs` is promoted wholesale (18 statuses 220..237)
  with the source's own swapped SpecialHi/SpecialLw attack IDs.
- Reloc tokens: Thunder head/trail WPAttributes at PikachuMain 0x0c/0x40 (they
  overlap the file-handle words exactly like Samus's Bomb), Thunder Jolt
  air/ground at PikachuSpecial1 0x00/0x34, ThunderJoltB anim/mat-anim at
  PikachuSpecial3 0x1a20/0x1ae0. All four WPAttributes are normalized and
  pinned to the source literals (Pikachu is the first owner whose weapons carry
  non-zero attack offsets, so the generic zero-offset guard is bypassed per
  struct rather than relaxed).
- Effects: ThunderJolt (Special3), ThunderTrail (Model), ThunderShock (Special2)
  and the shared Master-Ball rays (EFCommonEffects3) join the deferred-desc
  resolver; `NDS_EF_DEFERRED_MAX` 24 -> 28.
- Colour animations 0x38..0x3d transcribed into the DS encoding, plus the
  PlayFGM opcode; SpecialHiStart falls through into SpecialHi's spark loop as
  the source's adjacent arrays do (same rule as Fox's SpecialHiStart).
- gmsound: FGM 79/112/125/225..232/294/305 and voices 536..550 declared.
- Entry: Appear pair + flag1 rays. **Recorded delta:** the thrown Master Ball
  (`dEFManagerMBallThrownEffectDesc`) draws from ITCommonData, which this ROM
  does not link (item common data is P2-5's); Pikachu enters with the rays but
  without the ball until that file is admitted.
- CSS: portrait (in-progress `?` plate), gate name/emblem (Pocket Monsters),
  Selected clip 476; HUD: stock icon (five source LUTs) and portrait. The HUD
  portrait palette band is now per PLAYER (5..8) with stocks at 9..12, because
  an eighth per-kind portrait palette did not fit the sixteen sub-OBJ slots.
- `renderer_adapter_fighter.c` draws a fighter only through its native owner
  slot, so the first admission (before the owner) fought, took damage and
  showed on the HUD with no model on screen -- verified by a human-idle lab
  with Pikachu at (472, 0) beside Fox at (614, 0) and nothing at his position.

## Native owner — 2026-09-02

- `P2_RUNTIME_OWNERS` gains `("pikachu", "NDS_P2_PIKACHU")`; native owner
  slot **7**, image slot **5** (`nitro:/fighters/pikachu_{high,low}.bin`,
  High 27 arrays / 5,414 elements, Low 3,534). Every runtime seam that named
  Link's slot 6 now names Pikachu's 7: owner tables, image path/size/verify,
  dense normals, joint schedule/binding tables, cross palette slots, adapter
  owner/model-id/profile-owner, fighter-manager and CSS-preview image
  residency.
- His eleven cross stores reach palette slot 26; the adapter takes the real
  union of an owner's cross slots and allocates parent slots downward from 30
  around it, so no constant moved.
- Native-owner checkers: geometry closure, weld consistency and matrix
  precision PASS with Pikachu in the owner list; the hierarchy checker and
  `generate_nds_native_owners.py --check` still hit the standing
  `hierarchy_locals` falsifier from P2-3f33 (main tree too).
- Both-CPU tickhud lab with the owner (3,600 frames, clock 3,208 at the last sample): no `__excpt_entry`, reloc symbol-resolve/fixup and weapon spawn failures 0, native plan build/hit/verify-mismatch 158/1,574/0, validate rejects 0/0. The packet layer re-recorded every frame with faults (1,732 records / 1,732 faults / 0 hits) -- the same pre-existing HEAD residue the 2026-09-01 Mario probe showed on this worktree, not owner-specific. Human-idle lab: Pikachu drawn standing beside Fox; shell lab: Pikachu drawn in the CSS 1P preview and in the following match.
- Smoke (both-CPU tickhud lab, 3,600 frames): no CPU abort, no reloc symbol
  resolve or fixup failures, no weapon spawn failures; Pikachu's own level-3 AI
  reached Thunder Jolt ground/air (222/223) and Thunder's air self-hit (230).
60-frame census over the same run: statuses 221 (AppearL) 2, 222 (Thunder Jolt ground) 4, 223 (Thunder Jolt air) 1, 230/231 (Thunder air hit/end) 1/1, one KO at 101% and respawn; Quick Attack (232..237) was not sampled by the level-3 AI in this run

## Audio bank — 2026-09-02

- `render-audio-fgm-phase-pack.py` gains `PIKACHU_AUDIO`: 34 cues through the
  same source-program AOT renderer Samus's bank uses -- FGM 79, 112, 125,
  225..229, 231, 232, 294, 305, the four shared cues his motion scripts are
  the first to request (90, 101, 139 MBallOpen, 637), voices 536..551,
  announcer 507 and crowd 611. Bare `fork_voice` roots render their target
  program (112->105, 125->116, 294->287, 305->298, 90->86, 101->94, 637->630);
  232 Thunder fuses forks 674/675 and 139 fuses 682. Pack 223 -> 257 entries (258 with 230 below),
  2,671,080 bytes; DeadUp 542 (55,204 IMA bytes) fits the 60 KiB slot, so the
  237,568-byte cache does not move. `check-audio-fgm-phase-pack.ps1` PASS.
- **Electric2-5 (226..229)** drive pitch with n_env.c modulator shape 8 and
  volume with shape 4 -- the engine's random sample-and-hold family
  (`randFloat1`/`randFloat2`, n_env.c:3993-4011 spawn, :4126-4190 tick). The
  renderer now carries those shapes with the source's own two LCGs
  (`seed * 0x343FD + 0x269EC3`, static seed 1) as ONE fixed realization per
  cue, declared `random_modulator_fixed_realization` in the entry's
  `runtime_fidelity_debt` (kept through the full-program render by
  `PERSISTENT_FIDELITY_DEBT`). Accepted delta, sacrifice order 1.
- Those four sit near the source Nyquist (sample 12 at +1190 cents with a
  +/-2500-cent modulator clamped to +1200) and encoded at 13.1-13.3 dB IMA
  SNR, under the pack's 14 dB floor. `FULL_PROGRAM_AOT_OUTPUT_RATE_HZ` renders
  them at 64 kHz (DS `frequency` u16 holds it); SNR 15.8-16.2 dB, +~21 KiB ROM.
- **230 `nSYAudioFGMPikachuElectricLoop`** (the grounded crawl, an infinite
  `mark_loop`/`jump_loop` sequencer `wpPikachuThunderJoltGroundMakeWeapon`
  starts on every ground segment) ships as a source-lifetime-bounded prefix
  like Samus's Charge hums: each ground segment inherits the previous
  segment's remaining `lifetime`, the air spawn sets `WPPIKACHUJOLT_LIFETIME`
  (100, REGION_US) and `wpMainStopFGM` ends the voice at weapon death, so one
  play never outlives 100 game ticks -- 293 FGM ticks with the one-tick
  margin, a prefix of the 460-tick first pass (60 intro + 400-tick loop
  note). 64 kHz like Electric2-5 (19.1 dB SNR, 53,916 IMA bytes, fits the
  60 KiB slot), `ds_pause_with_game`. The one extension it does not carry is
  a reflector re-arming the lifetime: a reflected crawl goes quiet after the
  prefix, declared `gameplay_lifetime_bounded_prefix`. Pack 258 entries,
  2,725,028 bytes; checker PASS. `build_pikachu_jolt_loop_selector` pins the
  wpvars.h define and the three Jolt source lines it relies on.
- **Runtime smoke (both-CPU lab, 3,600 ticks, gdb counters):** pack loaded,
  258 supported, 177/177 play calls supported, 0 unsupported, 0 play
  failures, miss ring empty (the 257-entry ROM had logged exactly two misses,
  both id 230, the grounded crawl). Same run: Thunder spawned at t=191 and
  self-hit the airborne Pikachu at t=205; four Thunder Jolts thrown, two of
  which landed and spawned a floor segment (`from_type 0 -> 0`, lifetime 97/98
  inherited). No floor->wall transition and no Quick Attack was exercised by
  the level-3 CPU in that minute; the crawl-edge and QA tours stay open.
- gmsound.h gains the four shared ids (`nSYAudioFGMInflateJump2` 90,
  `nSYAudioFGMInflateJump7` 101, `nSYAudioFGMMBallOpen` 139,
  `nSYAudioFGMCharacterUnkZip8` 637); `ndsAudioFgmIDIsIncluded` lists the 34.

## Specials tour — 2026-09-02

gdb breakpoints on the source entry points during both-CPU lab matches
(build-pikachu-cpu); positions read at frame boundaries from the weapon list.

- **Thunder Jolt crawl, Dream Land edge.** A landing jolt was moved to
  x=-2200 on the main floor (nothing inside a map proc was poked). Per-frame:
  floor crawl at 55/tick to the edge (x=-2318, y=0), hand-off to the left
  slope (line kind 3, `lr=3`) at t+2, then straight down the slope 55/tick
  with x following the (-2318,0)->(-1972,-1072) line, gone at the underside
  with 75 ticks of lifetime left -- the source `ProcMap` has no ceiling case,
  so past the wall's lower edge the crawl ends. Floor edge hand-off, wall
  crawl and end all match the source flow. Natural CPU jolts landing
  mid-stage crawled and hit Fox.
- **Thunder:** spawned at t=191 with Pikachu airborne, self-hit (air) at
  t=205; a grounded Thunder later self-hit at t+15. Head/trail articles alive.
- **Quick Attack:** never used by the level-3 CPU in four one-minute matches
  (no `ftPikachuSpecialHiStartSetStatus` hit); the zip rules are the source's
  verbatim `ftpikachuspecialhi.c`. Needs a human-input tour.
- Dream Land collision as the port builds it (7 lines): three platforms,
  floor (-2318..2318, y 0), ceiling (y -1072), right slope kind 2, left slope
  kind 3; edge topology floor<->slopes<->ceiling.
- Probe hygiene: fields written by the function you broke on read stale
  through the gdb stub (dirty dcache); read at the next frame boundary instead.
  `gGCCommonLinks[5]` is the weapon list (4 is items).

## Quick Attack tour — 2026-09-02

- `build-pikachu-human` (proof fighter 0 = Pikachu human, no both-CPU) vs
  level-3 Fox, DS keys poked into the platform's latched held-keys word from
  gdb (the Yoshi tour's method). Ground: UP+B at t=150 -> SpecialHiStart
  t=152, twenty-tick start, zip at t=172 in the stick direction held at
  that moment (RIGHT: +330/tick along the floor, 1,650 in five ticks), End
  t=177; the sub-zip check at t=185 with the stick moved to UP took the
  second zip (`ftPikachuSpecialHiCheckGotoSubZip` angle-diff rule). Air:
  UP+B at t=411 during a jump -> AirHiStart with the 0.8 gravity clamp, zip
  t=432 (RIGHT, 330/tick), End t=437, second zip after a direction change
  at t=445, then FallSpecial. Source-equivalent; no abort.
- Method caveat: the poke lands after `sHeldKeys` is latched, so an
  asynchronous pad poll can read the real (neutral) keys on some ticks --
  the fighter's stick showed 80/0 jitter. Poke before the latch (line 545)
  next time; the zips still resolved because the direction is read on one
  tick.

## Acceptance

- [x] Move inventory sweep vs `ftpikachu` data (P2-3f34).
- [x] Thunder Jolt crawl paths equivalent on Dream Land (2026-09-02); each landed stage as it lands.
- [x] Thunder bolt/self-hit semantics equivalent (air + ground self-hit observed, 2026-09-02).
- [x] Quick Attack segment/angle rules equivalent (ground + air, sub-zip; 2026-09-02).
- [ ] Budgets + stress measurement banked; CSS live; owner feel pass.
