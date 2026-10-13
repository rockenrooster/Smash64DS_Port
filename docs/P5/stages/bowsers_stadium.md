# Bowser's Stadium — P5 stage card

**Status:** landed in VS with its bombs (play-p5a, 2026-10-13); owner
playtest pending.
**Source:** Remix id 0x43 (`Stages.asm`:93); map 0x8CA at offset 0x14
(`Stages.asm`:348, `stage_file_table` row :4183), geometry 0x8CB, wallpaper
0x8CC. Port gkind 42. **Wave:** 3, after Dinosaur Land. Parent:
[master](../New_Stages.md).

## Source observations

- `add_stage(bowserb, ...)` (`Stages.asm`:5085): default song
  `MIDI.id.BOWSERBOSS` ("Koopa's Theme", `midi.asm`:386, with a bend-range
  override at :1559 and master volume 88 at :1742); occasional and rare
  songs Koopa's Road and Ultimate Koopa, unused; class BATTLE; series logo
  Bowser; hazard type HAZARDS.
- Ground routine `function.PEACHS_CASTLE` (`Stages.asm`:3255), Peach's
  Castle's maker at 0x8010B4AC. Remix's `peach_bumper_`
  (`Hazards.asm`:537, patch at 0x8010B4B4) replaces its `grCastleInitAll`
  call: on this stage there is no bumper and no map-node animation; with
  hazards on (always, in the profile) it adds two bombs.
- The bombs (`Hazards.asm`):
  - `add_item_` (:938) loads Remix's bomb header (file 0x9B2, a modified
    Race to the Finish header with the hitbox; its word 0xA8 names the model
    file 0x9B1) and makes item kind 0x1A, the source's
    `nITKindTaruBomb`, through the stage item maker at 0x8016EA78. Each bomb
    hovers: its bottom ECB is set to -416.
  - Positions x +2990 (right, unique id 0) and x -2990 (left, id 1),
    y 512 (`Hazards.asm`:575-592; the same positions on respawn, :1350-1368).
  - `replenish_bowser_bombs_` (:1309): while a side's bomb exists and has
    not exploded, the timer holds at 300 frames (0x3C * 5); after it
    explodes or is destroyed, the timer counts down and the side's bomb is
    remade at 0.
  - A remade bomb (:1370-1382) starts with its hitbox off and a 120-frame
    flash (`BOWSER_BOMB_FLASH_TIME`, :1305): `bowser_bomb_flash_main` (:1392)
    toggles drawing every two frames, then turns the hitbox on and plays FGM
    0xC2. Its item id word is overwritten to 0x11A.
  - `bowser_bomb_explosion_` (:1420): on this stage the explosion sets the
    respawn flag, plays FGM 0x01 and skips the shrapnel effect.
  - `bowser_bomb_air_collision_` (:1457): the air-collision sound is FGM
    0xC2 instead of 0x47.
  - `bomb_hitstun_` (:1481): a fighter hit by a Race to the Finish bomb or a
    Bowser bomb on this stage takes hitstun times 9 / (gravity + 4), its own
    gravity attribute.
- Remix files: 0x9B1 (model), 0x9B2 (header), 0x9C8-0x9CA (ground, air and
  land animations), `File.asm`:364-389. The export confirms who reads the
  three animation files.
- Select: icon offset 0x1ACB8; preview zoom 0.4 (`Stages.asm`:3738);
  Peach's Castle's preview camera; Training parent Sector Z
  (`Stages.asm`:3963).

## Census

Layer 0 secondary (four DObjs, 22 triangles), layer 1 four DObjs (26),
layer 3 ten DObjs with no triangles. 48 triangles, 113 vertices, 3 textures
(4.0 KB) with 530 B of palettes. One collision group of 3 lines, 23 map
objects. Camera bounds 6000/-2100/6000/-6000; blast bounds
8000/-5000/9450/-9450.

## Dependency inventory

- Export and generator donor mode (from Wave 2).
- The native TaruBomb item (P2, Race to the Finish) and the P4 item-article
  path (S6) for Remix's model, header and animations.
- `src/port/nds_p5_bowserb.c`: the stage's ground maker, the two bomb slots,
  respawn, flash, sounds, hitstun rule.
- P3 digest: the two slots' item references, timers and flash counters
  (`nds_net_session.c:793` arm).
- Music: the T7 bank fix (program 49 is a Remix instrument) and the
  song's overrides.

## Work sequence

1. Export and generate; register the blob; registry row; select cell row 2,
   column 0.
2. Route Remix's bomb files through the article path, loaded with the stage
   before GO.
3. Port the ground maker and the bomb rules from the asm, read as assembled.
4. Fold the bomb state into the P3 sync digest and the replay digest.
5. Song, per-gkind rows (Sector Z Training parent), probe.

## Landing notes (2026-10-13)

- `src/port/nds_p5_stages.c` ports `peach_bumper_`'s Bowser's Stadium arm,
  `add_item_`, `replenish_bowser_bombs_` and `bowser_bomb_flash_main` as
  assembled; `ndsP5StageMakeGround` runs where the source's ground dispatch
  does (`grMainSetupMakeGround`, renamed under P5 with a wrapper). The
  source's TaruBomb code runs the bombs; Remix's patches to it are hooks in
  `battleship_item_tarubomb.c` (explosion: FGM 0x01, respawn flag, no
  shrapnel; landing: FGM 0xC2), the item destroy (a destroyed bomb leaves
  its side), `ftMainProcessHitCollisionStatsMain`'s head (the first hit
  log's attacker per port) and `ftCommonDamageInitDamageVars`' hitstun
  (times 9 / (gravity + 4) when that attacker is a bomb).
- Read as assembled and kept: a remade bomb's main routine is the flash and
  then none, so it hangs where it was made; the first two fall to the
  hover. Remix renumbers a remade bomb 0x11A; only its own hitstun test reads
  that (and takes 0x1A too), so the port keeps the source kind -- 0x11A would
  index the item tables past their end.
- The bomb draws natively from Remix's model: `scripts/p5/p5_articles.py`
  resolves the header's ITAttributes (0xA8) to the model's DObjDLLink lists
  (three roots) and adds them to the P4 article rows as a stage "content"
  (64 + stage index) that is in the match only on its stage.
- The Race bomb's code sat in the frontend overlay's 1P head, which VS
  battles borrow (the first probe aborted in `itTaruBombFallProcUpdate`);
  P5 builds link the overlay script without its rows, so the object is
  resident (1,636 bytes).
- The P3 digest folds each side's timer, flash and flags.

## Directed witnesses

- [ ] Both bombs hover at x +-2990, y 512 from the first frame of the match.
- [ ] A hit bomb explodes with FGM 0x01 and no shrapnel; its side's timer
      runs 300 frames, then a new bomb appears.
- [ ] The new bomb flashes for 120 frames with no hitbox (a fighter passes
      through it), then plays FGM 0xC2 and becomes solid.
- [ ] A bomb that falls plays FGM 0xC2 on collision.
- [ ] Hitstun from a bomb hit equals 9 / (gravity + 4) times the normal
      value for two fighters of different gravity.
- [ ] Four CPUs with items: bombs, items and fighters interact; the P3 sync
      check stays green through a respawn.

## Ledger

| Row | Value |
|---|---|
| Blob slab / body | slab 3,736; blob 4,790; GX template 3,752 (18 DObjs, 14 bindings, 9 runs) |
| Bomb files | 0x9B2 header 272 B, 0x9B1 model 7,176 B, loaded with the ground (battle heap) |
| Textures / palettes | census: 3 textures, 4.0 KB; 530 B palettes |
| Heap low-water, cast preset | 59,944 (Mario, Fox, DK, Samus; 1,800 frames, two explosions and respawns a side) |
| Heap low-water, heaviest roster | unmeasured |
| Four-CPU WORK P95 / two-VBlank, cast preset | unmeasured |
| Four-CPU WORK P95 / two-VBlank, heaviest roster | unmeasured |

## Closure record

Open.
