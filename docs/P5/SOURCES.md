# P5 source and evidence ledger

## Pins

P5 uses the P4 source lock ([docs/P4/source-lock.json](../P4/source-lock.json)):
`JSsixtyfour/smashremix` at `5e04fe7fcd023cd43c71f25f89bb6e810d254d55`.
All seven stages come from the main Remix tree; EXTRA is not needed. The
staged build is `builds/p4-staging/remix-5e04fe7` (made by
`scripts/p4/stage_remix.py`); it and everything read from it stay local.

## Remix witnesses (main tree at the lock)

| Id | File and lines | What it establishes |
|---|---|---|
| R1 | `src/Stages.asm`:20-247 | stage ids (Meta Crystal 0x0D, Duel Zone 0x0E, Final Destination 0x10, First Destination 0x2A, Bowser's Stadium 0x43, Hyrule Temple 0x55, Dinosaur Land 0xC1) |
| R2 | `src/Stages.asm`:252-273 | header offsets (`type`): 0x14, Final Destination 0x00 |
| R3 | `src/Stages.asm`:277-497 | map header file ids |
| R4 | `src/Stages.asm`:499-512, :3185-3405 | ground routine constants and `function_table` |
| R5 | `src/Stages.asm`:538-545, :1349-1380, :2843 | the select's 6x3 grid of 40x30 icons, cursor placement, icon grid draw |
| R6 | `src/Stages.asm`:956-1006, :2851 | the vanilla select parts Remix hides; Remix's name string |
| R7 | `src/Stages.asm`:1009-1200 | preview camera, random fixes, zoom and position, header and type patches |
| R8 | `src/Stages.asm`:2638-2700 | `training_id_fix_`: the `background_table` parent for Training |
| R9 | `src/Stages.asm`:2952-2971 | `stage_table_normal`, page 1 |
| R10 | `src/Stages.asm`:3407-3630, :3670, :3891, :3895, :4114, :4333 | icon offsets, `zoom_table`, `position_table`, `background_table`, `stage_file_table`, `class_table` |
| R11 | `src/Stages.asm`:4443, :4536, :4584, :4632, :4681 | name strings, series logos, tournament hazard modes, hazard types, default-music table |
| R12 | `src/Stages.asm`:4777-4853, :5056, :5085, :5110, :5259 | `add_stage` and the four Remix stages' rows |
| R13 | `src/Hazards.asm`:537-597, :938-1060, :1305-1550 | Bowser's Stadium: bumper replacement, `add_item_`, respawn, flash, explosion, air collision, hitstun |
| R14 | `src/File.asm`:364-389 | the bomb model, header and animation files |
| R15 | `src/Camera.asm`:55-101, :310-330 | Mushroom Kingdom camera routing; the fixed-camera option |
| R16 | `src/FD.asm`:1-136 | Final Destination in VS: the boss backdrop gate and the music switch |
| R17 | `src/Item.asm`:2649-2705 | the item-container option (outside the profile) |
| R18 | `src/BGM.asm`:225-260, :1449-1470 | alternate song pick (option); stage BGM ids |
| R19 | `src/midi.asm`:69-110, :366, :386, :414, :545, :760-960, :1490-1750 | song insertion, the four default songs, the instrument bank extension, per-song overrides |

## BattleShip witnesses

`gr/grdef.h` (stage enum), `gr/grdisplay.c`:182-219 (layer maker and
layer-1 collision process), `gr/grwallpaper.c`:266-302 (wallpaper kind),
`gr/grmainsetup.c`:31-49 (ground makers), `gr/grcommon/grcastle.c`
(Peach's Castle maker), `mp/mpcollision.c`:26-50 and :3965-3980 (ground
file table), `gm/gmcamera.c`:1175-1190 and `gm/gmdef.h`:147-158 (camera
status), `it/itmanager.c`:646 (item weights gate), `mn/mnmaps/mnmaps.c`
(layout :355-460, icons and cursor :530-560 and :845-851, random
:1377-1383, preview :883-1022 and :1313), `mn/mnplayers/mnplayersvs.c`:4500,
`mn/mnvsmode/mnvsresults.c`:205 and :3288, `lb/lbbackup.c`:107-113,
`lb/lbtypes.h`:282, `ef/efground.c`:1580, `if/ifcommon.c`:3317,
`ft/ftcommon/ftcommondead.c`:223-473, `ft/ftcomputer.c` (stage branches),
`sc/sc1pmode/sc1ptrainingmode.c`:659-669, `sc/sc1pmode/sc1pgameboss.c`,
`relocData/266_GRLastMap.c`, `268_GRZakoMap.c`, `269_GRMetalMap.c`.

## Port files read for the plan

`scripts/stages/generate_nds_native_stage.py` and the `metal`, `zako`,
`last` descriptors; `src/nds/nds_menu_shell_sss.c`;
`src/import/battleship_mnmaps.c`; `src/nds/nds_native_stage_select.inc`;
`src/nds/nds_native_stage_blob.c`; the blob maxima header;
`src/nds/nds_renderer_assets.c`; `src/nds/nds_reloc_assets.c`;
`scripts/sfx/bgm/render-audio-bgm.py`; `scripts/p4/remix_rom.py`,
`stage_remix.py`, `p4_audio.py`, `p4_css.py`; `src/nds/net/nds_net_lobby.c`,
`nds_net_session.c`; `src/import/battleship_sc1pgameboss.c`.

## What P5.0 did not establish

No export, generator run, build or emulator run was made for P5, and no
Remix ROM reference capture. Triangle and vertex counts are census
estimates. Memory, maxima growth, geometry per frame and the gate are
unmeasured. The consumers of the bomb animation files are unconfirmed. The
gkind census is a starting list. A located line range is a witness for the
fact beside it, not proof that every dependency of the stage is known.
