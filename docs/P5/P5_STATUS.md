# P5 status — Smash Remix stages on the DS

Owner of the live P5 state. Plans live beside it ([master](New_Stages.md),
[handoff](IMPLEMENTATION_HANDOFF.md), [shared plans](shared/),
[cards](stages/), [sources](SOURCES.md)). The master plan's Revision 1
(2026-10-12) orders the work as Board 1 (shared machinery T1-T12) and
Board 2 (stage waves); item status is tracked here under those ids.

## P5.0 census (2026-10-12) — done

Tool: `scripts/p5/stage_census.py` (read-only), run against
`builds/p4-staging/remix-5e04fe7`, the P4.0 staging at the P4 source lock
(`ssb64asm.z64`, 5,455 files) and its vanilla ROM (`roms/ssb.rom`, 2,132
files). Its output is the master plan's measured table.

1. **The 1P venues are vanilla bytes.** Files 0x10D, 0x75, 0x62 (Meta
   Crystal), 0x10C, 0x74, 0x61 (Duel Zone) and 0x10A, 0x72, 0x60 (Final
   Destination) are identical in the Remix and vanilla ROMs, and their
   payload hashes are the ones the `metal`, `zako` and `last` descriptors
   pin (for example map 0x10A `61098ffa...`, geometry 0x72 `8da67fd1...`).
2. **The Remix stages are three files each, with no outside dependency:**
   First Destination 0x877 (map), 0x878 (geometry), 0x879 (wallpaper);
   Bowser's Stadium 0x8CA, 0x8CB, 0x8CC; Hyrule Temple 0x981, 0x982, 0x983;
   Dinosaur Land 0x1247 (map), 0x1246 (geometry), 0x1245 (wallpaper). Every
   wallpaper is a 300x220 sprite.
3. **Header offsets** (Remix `type`): 0x14 for every P5 stage except Final
   Destination (0x00).
4. **Formats:** CI textures (loaded as 16-bit) with RGBA16 palettes, one or
   two combiners per stage, lit geometry, and no display-list command
   outside the set the vanilla stages use.
5. **Ground code** (Remix `function_table`): none for the 1P venues;
   `function.CLONE` (none) for First Destination, Hyrule Temple and
   Dinosaur Land; Peach's Castle's maker with bombs for Bowser's Stadium.
6. **Final Destination has no items.** Its header's `item_weights` is NULL,
   and Remix keeps the source's gate (`lw a3, 0x84(t7)`, `beq a3, zero` at
   0x8016EC78-0x8016EC80 in the assembled ROM).
7. **The songs need Remix's instruments.** Programs past the vanilla bank's
   42 (Remix appends from 0x2A): First Destination 44, 45, 52; Koopa's
   Theme 49; Temple Theme 44, 52, 60; Athletic Theme 46, 60. The DS BGM
   renderer reads only the vanilla bank and resolves a program past it to
   the percussion instrument (`render-audio-bgm.py`, `resolve_instrument`),
   so such a song renders with wrong instruments and no error. Five P4
   victory songs already ship this way: Wolf (48), Bowser (51), Crash (50,
   66), Lanky (68), Sheik (52). T7 fixes both.
8. **Camera:** Remix sends Hyrule Temple and Dinosaur Land to the Mushroom
   Kingdom camera status (`Camera.asm`:67, :83); the other five use the
   default.
9. **Stage select:** Remix's grid is six columns by three rows of 40x30
   icons per page (`Stages.asm`:538-545), page 1 in the order the owner
   chose (`Stages.asm`:2952-2971); its icons are STAGE_ICONS (file 0xA04,
   40x30 sprites). Remix hides the vanilla plate and names and draws names
   in its own font at (230, 209) (`Stages.asm`:976, :1002, :2851); P5
   keeps the vanilla plate instead (owner decision 2). Non-vanilla stages
   use Peach's Castle's preview camera (`Stages.asm`:1043-1062); zoom 0.5
   for Meta Crystal, Duel Zone, Final Destination and First Destination,
   0.4 Bowser's Stadium, 0.2 Hyrule Temple, 0.3 Dinosaur Land.
10. **Training parents** (Remix `background_table`): Sector Z for Meta
    Crystal, Duel Zone, Final Destination and Bowser's Stadium; Peach's
    Castle for First Destination, Hyrule Temple and Dinosaur Land.
11. **Remix code that tests these stages:** First Destination none outside
    the stage tables; Bowser's Stadium only `Hazards.asm` (the bombs);
    Hyrule Temple `Camera.asm` (routing, and the fixed-camera option);
    Dinosaur Land `Camera.asm` and `Item.asm` (the container option); Final
    Destination `FD.asm`. The AI, spawn and blast-zone files test none of
    them.
12. **The gkind census** has a starting list (shared plan 02).

## Owner decisions (2026-10-12)

Asked with the plan; the owner chose every recommended option:

1. Stage select: 6x3, full-size cells, Remix page-1 order, row 2 Bowser's
   Stadium, First Destination, Hyrule Temple, Dinosaur Land, spare '?',
   RANDOM.
2. New stage names: the source's menu font on the vanilla plate.
3. Music: the default track only.
4. Final Destination in VS: the static wallpaper, as Remix.

## Board 1 status (master plan Revision 1)

| Item | State |
|---|---|
| T1 stage registry | landed for Wave 1: `scripts/p5/stages.json` (seven rows), `scripts/p5/p5_stages.py` (`--header` X-macro rows in the build tree, read by `include/nds/nds_p5.h`; `--sss` select spec), `scripts/p5/p5_rules.mk` (inner make), `NDS_P5` flag |
| T2 identity seams | Wave 1 sites done: select lock and both randoms (offered list), walk target and rows, preview file/scale/camera rows, Final Destination's boss wallpaper and song switch; Results' `u16` stage mask is harmless for 13/14/16 and needs a guard before 41-44 |
| T3 donor stage export | `scripts/p5/stage_export.py` writes the four Remix stages' O2R closures and `stage.json` (resolved header, layers, Remix rows); twice bit-identical (`--verify-repeat`) |
| T4 generator donor mode | `scripts/p5/stage_generate.py` derives the descriptor from `stage.json`, learns pins (`scripts/p5/pins/`), and emits include, blob and GX template through the unchanged generator; runtime registration open |
| T5 residency and memory | open; Hyrule Temple exceeds today's 863-vertex workspace maximum (census) |
| T6 6x3 stage select | landed: kit bake on the 6x3 grid (Remix icons for every cell, lower block +8 rows, menu-font names 1:1 on the vanilla plate, Remix logos, venue preview wallpapers), shell grid, generated plaque/preview tables, 3D preview rows (Remix zoom, Peach's Castle camera, lifted with the panel); full browse and Training select open |
| T7 stage music | open; the bank defect also affects five P4 victory songs |
| T8 hazards and movers | open |
| T9 camera and bounds | open |
| T10 per-gkind rows | open |
| T11 acceptance probe | open |
| T12 worst-case search | open |

## Board 2 status

| Wave | Stage | Card | State |
|---|---|---|---|
| 1 | Meta Crystal | [card](stages/meta_crystal.md) | landed in VS (play-p5a); owner playtest passed 2026-10-13 |
| 1 | Duel Zone | [card](stages/duel_zone.md) | landed in VS (play-p5a); owner playtest passed 2026-10-13 |
| 1 | Final Destination | [card](stages/final_destination.md) | landed in VS (play-p5a): static wallpaper, Master Hand's entrance then Final Destination at 840 updates; owner playtest passed 2026-10-13 |
| 2 | First Destination | [card](stages/first_destination.md) | source inspected |
| 3 | Dinosaur Land | [card](stages/dinosaur_land.md) | source inspected |
| 3 | Bowser's Stadium | [card](stages/bowsers_stadium.md) | source inspected |
| 4 | Hyrule Temple | [card](stages/hyrule_temple.md) | source inspected |

## Wave 1 record (2026-10-13)

Build: `make TARGET=smash64ds` with the eighteen `NDS_P4_<NAME>=1` flags and
`NDS_P5=1` (play-p5a). Probes (`shipbattle.ps1`, four level-9 CPUs, 900
presented frames):

| Stage | Roster | Heap low-water | Native failures | Song |
|---|---|---|---|---|
| Final Destination | Mario, Fox, DK, Samus | 48,840 | 0 | 25 after 840 updates |
| Meta Crystal | Mario, Fox, DK, Samus | 54,784 | 0 | 37 |
| Duel Zone | Dedede, Sonic, Lanky, Kirby-parent content 11 | 56,960 | 0 | 36 |

The select's cursor, plaque and preview were captured on each cell's slot
(5, 10, 11). One Duel Zone probe went back to the character select at the
screenshot frame and did not reproduce in two reruns of the same command.

## Risks and open items

- Hyrule Temple's size: the workspace maxima, the blob caps and the heap
  floor with heavy P4 rosters (T5); the widest camera on the DS (T9).
- The T7 renderer change touches the shared BGM path: start that build
  alone, as for any shared P2 generator (P4 S2 note).
- P4's P95 pass is not closed; the heavy-roster gate rows wait on it, the
  cast-preset rows do not.
- The select's STAGE SELECT label stays in the lower block in the proposal;
  the owner can move it to the top in the same bake.
- The Remix ROM reference boot (scene-data pokes) is unverified on the Remix
  build.
- The consumers of Remix's bomb animation files 0x9C8-0x9CA are to be
  confirmed by the export.
