# Shared plan 02 — Stage identity and the 6x3 stage select

**Status:** plan. Owner decisions of 2026-10-12 fix the select's grid,
order, names and music. Parent: [master](../New_Stages.md).

## Identity

- The four Remix stages get gkinds 41-44 (First Destination, Bowser's
  Stadium, Hyrule Temple, Dinosaur Land), continuing the source enum after
  `nGRKindBonusStageEnd` (40). They are defined in a port header
  (`include/nds/nds_p5.h`), never in `decomp/`. Remix ids stay in the
  registry for traceability only.
- Meta Crystal (13), Duel Zone (14) and Final Destination (16) keep their
  source gkinds. They become VS and Training stages and stay 1P venues.
- Generated tables from `scripts/p5/stages.json` replace range tests: a
  class per gkind (BATTLE for all seven, as Remix's `class_table`), and
  membership sets for VS, Training and RANDOM. The 1P set is unchanged.
- The save gains no bytes in P5. P7 owns persistence and unlock rules: its
  plan gives each P5 stage a stage-unlock id (0-7) in its own save block,
  separate from the source's `ground_mask`. The registry carries that id;
  it locks nothing until P7 lands.

### Census: the T2 work list

The starting set from the P5.0 read. T2 begins with a grep for `gkind` and
`nGRKind` over `src/` and every compiled source TU, and adds anything this
list misses.

| Site | Today | P5 handling |
|---|---|---|
| `gr/grmainsetup.c:33` | ground makers only for `gkind <= nGRKindBattleEnd` | generated ground-maker rows: Bowser's Stadium's maker; none for the other six |
| `gr/grwallpaper.c:274` | `gkind >= nGRKindBonusStageStart` makes the static wallpaper | class table: BATTLE takes `grWallpaperMakeCommon` (Remix patches this test, `Stages.asm` note on 0x80104C14) |
| `gr/grwallpaper.c:293` | Last runs `sc1PGameBossInitWallpaper` | only on the 1P battle scene (Remix `FD.asm` `fix_`, owner decision 4) |
| `if/ifcommon.c:3317` | bonus HUD for `gkind >= nGRKindBonusStageStart` | class table |
| `ef/efground.c:1580` | ground effects for `gkind <= nGRKindBattleEnd` | none for P5 stages: the source runs them for the nine VS stages only, and Remix's own background animations (`bg_info`) list none of the seven |
| `gm/gmcamera.c:1175` | camera status: Zebes, Inishie, default | generated rows: Inishie status for Hyrule Temple and Dinosaur Land (Remix `Camera.asm`:67, :83) |
| `ft/ftcommon/ftcommondead.c:223-473` | Bonus1 and Bonus2 ranges | P5 gkinds lie outside both; a static assert keeps it so |
| `mn/mnvsmode/mnvsresults.c:205` | `ground_mask \|= (1 << gkind)` into a `u16` | only gkinds 0-8 write (the field means "VS stages played", read through `LBBACKUP_GROUND_MASK_ALL` at `mnvsresults.c:3288` and `sc1pmanager.c:558`); others write nothing until P7 |
| `lb/lbbackup.c:107-113` | resets a saved Inishie cursor while it is locked | a saved cursor that is not admitted or is locked resets to the default |
| `mn/mnmaps/mnmaps.c:1377-1383` | RANDOM rolls `syUtilsRandTimeUCharRange(9)`, not the current stage | roll over the admitted VS set (16), same no-repeat rule |
| `mn/mnplayers/mnplayersvs.c:4500` | stage select off: random over starters (+ Inishie if unlocked) | the admitted VS set |
| `mn/mnmaps/mnmaps.c` per-gkind tables (:594, :806, :883, :949-950, :1020, :1313) | names, emblems, file infos, wallpapers, scales, camera positions for nine | generated rows (T6) |
| `mp/mpcollision.c:3970` | `dMPCollisionGroundFileInfos[gkind]` | rows for 41-44: Remix map file and offset 0x14 |
| `sc/sc1pmode/sc1ptrainingmode.c:659-669` | training wallpaper and fog per gkind | Remix's `background_table` parent (T10) |
| `ft/ftcomputer.c` | branches for Inishie, Yamabuki, Zebes, Hyrule, Yoster | P5 stages take the default AI path; Remix adds no per-stage AI |
| `sc/sccommon/scautodemo.c:554` | attract-demo stage order | unchanged (vanilla) |
| `src/nds/nds_menu_shell_sss.c` | ten slots, `1u << gkind` lock mask, `want <= nGRKindInishie` walk target, slot 6 default | registry-driven cells, membership table, any registered VS gkind as a walk target |
| `src/import/battleship_mnmaps.c` | preview tables indexed by gkind, bounded by `dMNMapsFileInfos` | generated rows (T6) |
| `src/nds/net/nds_net_lobby.c` | stage cursor byte checked against ten slots; descriptor `gkind` byte | slots 0-17; the gkind byte is unchanged; build compatibility includes the registry hash |
| `src/nds/net/nds_net_session.c:793` | per-stage ground state folded into the sync digest | add Bowser's Stadium's bomb slots and timers; movers are covered by the yakumono loop above it |
| port gkind tables (native stage registry and blob loader, stage GX caches, BGM rows, magnifier colours, harness rows) | sized for the existing stages | generated rows |

## The stage select

### Layout (owner decision 1)

In the source's 320x240 frame (the DS shows it at 4/5):

- **Grid:** six columns by three rows of 48x36 cells on a 50x38 pitch from
  x 10, rows at y 16, 54 and 92. The cursor frame keeps the source's
  (-7, -7) offset. Cells are 38x29 DS pixels on a 40-pixel pitch.
- **Lower block:** the STAGE SELECT label (172, 122) and its band
  (160, 128)-(320, 134), the wooden plaque (189, 124), the name plate
  (174-262, 191) with its shadow, and the preview panel (40, 127) move down
  by the grid's growth: about 8 units, so row 2 ends above the label. The 3D
  preview's viewport moves with the panel. Measure the select's viewport
  mapping on a capture before placing the model (the character select's
  lesson: its outer previews stood 14-17 pixels off until the viewport was
  measured).
- The numbers above are the proposal; T6 fixes them by bake and capture. The
  question put to the owner described "header to the top edge, lower block
  about 14 down"; the source's label is part of the lower block, so it
  stays there unless the owner asks to move it.
- **Order** (row-major), Remix's page 1 with the three later-page stages in
  the cells of stages P5 does not carry:

  | | 0 | 1 | 2 | 3 | 4 | 5 |
  |---|---|---|---|---|---|---|
  | row 0 | Peach's Castle | Congo Jungle | Hyrule Castle | Planet Zebes | Mushroom Kingdom | Meta Crystal |
  | row 1 | Yoshi's Island | Dream Land | Sector Z | Saffron City | Duel Zone | Final Destination |
  | row 2 | Bowser's Stadium | First Destination | Hyrule Temple | Dinosaur Land | spare '?' | RANDOM |

### Art

- **Icons:** Remix's STAGE_ICONS (file 0xA04, `icon_offset_table`) for all
  sixteen stages and RANDOM (`icon_offset_random`). They are 40x30 images,
  drawn at 1.2x so a 48x36-unit cell shows them at about their native
  resolution on the DS. The character select uses Remix portraits for every
  cell, originals included; the stage select follows it.
- **Spare cell:** the locked plate the select already draws for locked
  cells.
- **Names (owner decision 2):** the nine vanilla name sprites
  (`llMNMaps<Ground>TextSprite`) on the vanilla plate; the seven new names
  composed from the source's own menu font (the P2-1c kit font) in the same
  black, placed and centred as the vanilla sprites are.
- **Emblems:** series logos Mario Bros. (Meta Crystal), Smash (Duel Zone,
  Final Destination, First Destination), Bowser (Bowser's Stadium), Zelda
  (Hyrule Temple), Yoshi (Dinosaur Land), from Remix's extended
  FTEmblemSprites (file 0x14), as `scripts/p4/p4_css.py` reads it.
- **Preview wallpaper:** each stage's own 300x220 wallpaper at 0.37 in the
  panel, as `mnMapsMakePreviewWallpaper` draws the vanilla ones.

### 3D preview

- The model is the stage's battle packet, made through the existing preview
  path (`ndsMNMapsPreview*`) from the registry's map file and offset
  (Remix `stage_file_table`).
- Scale from Remix's `zoom_table`: Meta Crystal, Duel Zone, Final
  Destination and First Destination 0.5, Bowser's Stadium 0.4, Hyrule Temple
  0.2, Dinosaur Land 0.3. Remix's `position_table` sets no offset for any of
  the seven.
- Camera: every non-vanilla stage uses Peach's Castle's preview camera
  position (Remix `random_fix_2_`, `Stages.asm`:1043-1062), with the vanilla
  target bob. Read Remix's other preview patches as assembled
  (`Stages.asm`:1009-1017, :1064-1200).
- Loading stays stepwise with per-stage release of warm textures. Check the
  eight outside-battle assumptions for each new packet before its first
  probe: the 0x4C camera, the rigid path, billboard scale, scene lights,
  warm uploads, the GX program, the harness buffer and tree sizing
  (P4_STATUS, "The stage select's 3D previews").
- Animated layers show their first animation frame, as `mnMapsMakeLayer`.

### Behaviour

- Rows of six: left and right cycle within the row and skip locked cells;
  up and down change row and refuse a locked destination; the source's
  repeat timing. This generalizes the shell's existing rules unchanged.
- RANDOM picks uniformly among unlocked VS stages other than the current
  one. The character select's no-stage-select pick uses the same set.
- The cursor restores from `maps_vsmode_gkind`; a value that is not
  admitted restores to Dream Land.
- Training's stage select uses the same grid and cells.
- P3: the host sends its cursor slot (0-17) and the guest follows; the host's
  unlock state decides locks, as today.
- The scripted walk keeps Dream Land as its default target and accepts any
  registered VS gkind.

### Memory

Eighteen plaque-front surfaces (one per cell) and seventeen preview
wallpapers, all in NitroFS; the one-surface-blit-per-frame rule stays. The
all-content walk ROM is the memory check (shared plan 03).

## Exit

Every census site reads a generated table; a P5 gkind passes VS setup,
battle, Results, save write and restore and the P3 descriptor; a gkind
outside the registry is refused before the scene starts. The select shows
all 17 cells with their art, every stage's model and wallpaper, RANDOM
reaches all sixteen, and Training and a P3 room select every stage, with
0 failures on the all-content walk ROM.
