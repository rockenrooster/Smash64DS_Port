# P5 — New Stages: implementation plan

**Revision:** 1 (2026-10-12). Owner request: [Desired_New_Stages.md](Desired_New_Stages.md).
**Live state:** [P5_STATUS.md](P5_STATUS.md). **Resume point:**
[IMPLEMENTATION_HANDOFF.md](IMPLEMENTATION_HANDOFF.md). **Shared rules:**
[01 donor stage admission](shared/01_Donor_Stage_Admission.md),
[02 identity and stage select](shared/02_Identity_and_Stage_Select.md),
[03 DS resources](shared/03_DS_Resources.md),
[04 verification](shared/04_Verification.md). **Cards:** [stages/](stages/).
**Source ledger:** [SOURCES.md](SOURCES.md).

## What P5 delivers

Seven stages from Smash Remix, playable in VS (single console and P3
wireless) and Training with the same polish as the original nine, and a
stage select that uses the screen the way the P4 character select does.

## How this plan is shaped

P4 showed where the time goes (P4 master plan, Revision 3): shared machinery
built once and data-driven, units ordered by measured cost, and one
acceptance command with a fixed checklist per unit. P5 keeps that shape. The
P5.0 census (P5_STATUS, run 2026-10-12 against the staged Remix build) adds
facts that make P5 smaller than P4:

1. **Three of the seven are already native.** Meta Crystal, Duel Zone and
   Final Destination are vanilla 1P venues. Remix plays them in VS from
   files that are byte-identical to vanilla, and those bytes are exactly the
   inputs the port's campaign packets pin (`metal`, `zako`, `last`
   descriptors). They need VS admission, not conversion.
2. **The four Remix stages are ordinary stage files.** Each is three
   self-contained files (map header, geometry, wallpaper) in the vanilla
   formats: CI textures with RGBA16 palettes, one or two combiners, lit
   geometry, no unusual F3DEX2 commands. The existing native stage generator
   can take them once it accepts a donor input in place of BattleShip's
   typed C (shared plan 01).
3. **Remix gives them almost no code.** Three of the four run the generic
   ground path (Remix `function.CLONE`, no ground routine); Bowser's Stadium
   runs Peach's Castle's ground maker with two bombs in place of the
   bumper. The stage-specific work is that hazard, two camera routings and
   the Final Destination VS rules.
4. **The music needs one shared fix.** All four default songs use
   instruments Remix appends to the music bank, which the DS BGM renderer
   does not read today (it falls back to the percussion instrument). The
   same fix corrects five P4 victory songs.

## Product contract

- Original DS envelope: no RAM expansion, no DSi requirement.
- The seven stages:

  | Stage | Remix id | Port gkind | Source |
  |---|---|---|---|
  | Meta Crystal | 0x0D | 13 (`nGRKindMetal`) | vanilla 1P venue, VS as in Remix |
  | Duel Zone | 0x0E | 14 (`nGRKindZako`) | vanilla 1P venue, VS as in Remix |
  | Final Destination | 0x10 | 16 (`nGRKindLast`) | vanilla 1P venue, VS as in Remix |
  | First Destination | 0x2A | 41 (new, P5) | Remix main tree |
  | Bowser's Stadium | 0x43 | 42 (new, P5) | Remix main tree |
  | Hyrule Temple | 0x55 | 43 (new, P5) | Remix main tree |
  | Dinosaur Land | 0xC1 | 44 (new, P5) | Remix main tree (`yoshis_island_melee`) |

  New gkinds continue the source enum after `nGRKindBonusStageEnd` (40) in
  Remix id order. No other Remix stages or layout variants (DL, Omega,
  Remix) are in scope.
- Fidelity: the original SSB64 common rules plus the pinned donor's stage
  data and stage-specific mechanics. Remix's options stay outside the
  profile: hazard and movement switches (hazards are always on, as in the
  original), alternate music, stage layout variants, random-stage lists,
  the item-container switch, the fixed camera and Remix's custom items.
- Modes: VS with any 1-4 fighter selection (P4 contents included), items,
  teams and the P3 wireless room; Training. The 1P campaign keeps Meta
  Crystal, Duel Zone and Final Destination as its venues, unchanged.
- Unlocks: all seven are open in P5. Unlock rules and their save block
  belong to P7 (`docs/P7/P7_Implementation_Plan.md` gives each P5 stage a
  stage-unlock id, not its gkind); the registry carries that id, and no P5
  stage writes a save bit out of range (the source's `ground_mask` stays the
  source's).
- Performance: the standing gate on every P5 stage. Four-CPU WORK
  P95 <= 1,120,000 and >= 95% of frames within two VBlanks, items on, for
  the cast preset and for the stage's heaviest admitted roster. Replay
  digest identical, 0 native failures, 0 motion reads after GO, heap
  low-water >= 25,600 B.
- Residency: everything a stage needs (packet, collision, wallpaper, hazard
  assets, music stream) is resident or streaming before GO; nothing faults
  in during combat. The stage select loads previews in steps, as today.

## Owner decisions (2026-10-12, asked with this plan)

1. **Stage select grid:** 6x3 with full-size cells. Rows 0-1 are Remix's
   stage-select page 1 exactly; row 2 is Bowser's Stadium, First
   Destination, Hyrule Temple, Dinosaur Land, a spare '?' cell and RANDOM
   (Remix's cells, with the three later-page stages in the cells of stages
   P5 does not carry). Icons at the vanilla 48x36 size spanning the width.
2. **Stage names:** keep the vanilla plate and the nine vanilla name
   sprites; the seven new names are composed from the source's own menu
   font in the same black.
3. **Music:** the default track only (no Remix occasional or rare songs).
4. **Final Destination backdrop in VS:** follow Remix: the static wallpaper,
   without the 1P boss effects.

## Measured stage cost (census of 2026-10-12)

Read from the staged Remix ROM by `scripts/p5/stage_census.py`, which walks
each map header's layers, display lists and collision. Triangle and vertex counts are planning estimates
(unique vertex loads); the generator's pinned counts are authoritative once
they exist. Layers: DObjs/triangles per layer 0-3; A = joint animation,
M = material animation, S = secondary (opaque + translucent DLLink) layer.

| Stage | Files (map/geo/wall) | Layers | Tris | Verts | Textures | Collision groups/lines/map objects | Camera bounds T/B/R/L | Blast bounds T/B/R/L |
|---|---|---|---:|---:|---|---|---|---|
| vanilla nine | 3-5 files | | 136-299 | 250-583 | 3-29 KB | 1-4 / 5-19 / 26-42 | | |
| Meta Crystal | 0x10D/0x75/0x62 | 2/48M 2/72M 2/7M 2/26M | 153 | 362 | 8, 2.4 KB | 1/7/17 | 5000/-2400/5000/-5000 | 9000/-4200/10000/-10000 |
| Duel Zone | 0x10C/0x74/0x61 | - 2/138 - - | 138 | 237 | 10, 10.5 KB | 1/15/18 | 3000/-2000/4000/-4000 | 9000/-4200/9000/-9000 |
| Final Destination | 0x10A/0x72/0x60 | - 5/198AM - - | 198 | 355 | 7, 7.1 KB | 1/7/7 | 5000/-2400/5500/-5500 | 9000/-4200/11000/-11000 |
| First Destination | 0x877/0x878/0x879 | 2/119S 4/18 2/16 2/0 | 153 | 362 | 13, 3.4 KB | 1/3/26 | 5000/-2400/5000/-5000 | 7000/-4200/9221/-9219 |
| Bowser's Stadium | 0x8CA/0x8CB/0x8CC | 4/22S 4/26 - 10/0 | 48 | 113 | 3, 4.0 KB | 1/3/23 | 6000/-2100/6000/-6000 | 8000/-5000/9450/-9450 |
| Hyrule Temple | 0x981/0x982/0x983 | 10/195S 4/258 - 10/74 | 527 | 1,215 | 14, 12.8 KB | 2/35/48 | 4000/-7500/9000/-9000 | 8200/-10000/13500/-13500 |
| Dinosaur Land | 0x1247/0x1246/0x1245 | 8/83AM 10/177AM - 10/79 | 339 | 484 | 12, 7.1 KB | 5/16/32 | 4500/-750/3500/-5000 | 7500/-4000/6500/-8000 |

What each stage adds beyond its packet:

| Stage | Ground code | Hazards / movers | Camera | Items | Music (BGM id) |
|---|---|---|---|---|---|
| Meta Crystal | none | none | default | header weights | vanilla 37 (Metal Mario) |
| Duel Zone | none | none | default | header weights | vanilla 36 (Fighting Polygon) |
| Final Destination | none; no 1P boss backdrop in VS | none | default | none (NULL weights) | vanilla 24 intro, then 25 at 840 frames |
| First Destination | none | none | default | header weights | Remix "Final Destination (Melee)" |
| Bowser's Stadium | Peach's Castle maker with bombs | two respawning bombs | default | header weights | Remix "Koopa's Theme" |
| Hyrule Temple | none | none | Mushroom Kingdom routine | header weights | Remix "Temple Theme (Melee)" |
| Dinosaur Land | none | animated layers 0-1, five collision groups | Mushroom Kingdom routine | header weights | Remix "Athletic Theme" |

Hyrule Temple is the outlier: 3x Dream Land's triangles, 2.1x Sector Z's
vertices, the widest camera and blast bounds of all sixteen. It is the
likely new argmax of the standing gate and the stage that raises the native
packet maxima (shared plan 03).

## Board 1 — shared machinery (each item done once, for all seven)

Build each item generically for every stage that uses it. Where an item can
only be proven by a stage, its first consumer in wave order proves it.
Status lives in P5_STATUS.

| # | Item | Done when |
|---|---|---|
| T1 | **One stage list drives everything.** `scripts/p5/stages.json` holds one row per P5 stage: name, Remix scope and id, port gkind, Remix map/geometry/wallpaper file ids, header offset (Remix `type`), class, stage-select cell, icon offset, preview zoom and position, training-wallpaper parent, camera routine, music, item rule, hazard owner, P7 stage-unlock id. `NDS_P5_STAGES` (rules in `scripts/p5/p5_rules.mk`, the inner make, per the P4 lesson) and a generated X-macro header feed every shared site: the ground file table, ground-maker table, wallpaper kind, camera status, the mnMaps file/scale/camera/wallpaper rows, the training and magnifier rows, the select's cells, BGM rows and reloc asset rows. Per-stage inputs: the row and, where needed, `src/port/nds_p5_<name>.c`. | First Destination builds from its row with no hand-written per-stage block in shared code; adding another row needs no shared-code edit. |
| T2 | **Identity seams.** Replace gkind range tests and fixed-size tables with generated class and membership tables, as Remix's `class_table` does (BATTLE, RTTF, BTP, BTT). The census in shared plan 02 is the work list: ground makers (`grmainsetup.c:33`), wallpaper kind (`grwallpaper.c:274`, :293), bonus HUD (`ifcommon.c:3317`), ground effects (`efground.c:1580`), camera status (`gmcamera.c:1175`), the save's stage mask (`mnvsresults.c:205`, a `u16` written with `1 << gkind`), save restore (`lbbackup.c:107`), the select's lock mask (`1u << gkind` in a `u32`), both random picks (`mnmaps.c:1383`, `mnplayersvs.c:4500`), harness walk targets, the P3 descriptor and cursor, and every port table indexed by gkind. | Every census site reads a generated table; a P5 gkind passes VS setup, battle, Results, save write and restore and the P3 descriptor; a gkind outside the registry is refused before the scene starts. |
| T3 | **Donor stage export.** `scripts/p5/stage_export.py` reads the staged Remix ROM through `scripts/p4/remix_rom.py` and writes, under the build tree, O2R containers for each stage's three files and a `stage.json`: the resolved map header (layers, DObjDesc arrays, DLLink arrays, AnimJoint, MObjSub and MatAnim tables, collision, map objects, item weights, bounds, light angle, fog, emblem colours, whistle altitude) and the Remix table rows (`add_stage`, zoom, position, background parent, icon offset, class, ground function, hazard type, series logo), cross-checked against the assembled bytes. | The four Remix stages export with 0 failures, twice bit-identically; the negative controls in shared plan 01 fail with the first cause. |
| T4 | **Native stage generator donor mode.** `generate_nds_native_stage.py` takes a descriptor generated from `stage.json` (owner specs from the header's layers, links 4/6/13/17, pri/sec from `layer_mask`, material sources from the MObjSub tables plus the fail-closed segment-E walk, the header offset given directly) instead of BattleShip typed C and reloc symbols. Expected counts are pinned after first generation; drift falsifies. Collision (`generate_compact_ground_maps.py`, `check_collision_parity.py`), wallpaper (`generate_native_wallpapers.py`), GX compile and the blob registry take the same donor inputs. | First Destination's packet, collision and wallpaper regenerate bit-identically, host collision parity passes, and the lab ROM draws the stage natively with 0 declines. |
| T5 | **Residency and memory.** Every P5 packet is a NitroFS blob (no `LINKED` macro). Find which buffers the packet maxima size (`nds_native_stage_blob_maxima.generated.h`); Hyrule Temple's ~1,215 unique vertices exceed today's 863-vertex maximum, so a static maxima-sized buffer would grow for every stage: size those per blob instead. Keep a per-stage heap ledger beside Dream Land and Sector Z with the same rosters. | Each P5 stage with its heaviest admitted roster keeps heap low-water >= 25,600 B and 0 texture misses; no vanilla stage's ledger grows; the select opens on the all-content walk ROM with every preview. |
| T6 | **The 6x3 stage select** (owner decisions 1-2). `scripts/p5/p5_sss.py` owns GRID, SCREEN and LAYOUT for the kit bake and the shell, as `scripts/p4/p4_css.py` does for the character select: Remix's STAGE_ICONS (file 0xA04, 40x30) for all sixteen cells plus RANDOM, as the CSS uses Remix portraits for every cell; the spare cell's locked plate; names, emblems, preview wallpapers, the 3D preview through Remix's preview tables; navigation, both randoms, cursor restore, Training's select and the P3 guest cursor. Layout and preview rules in shared plan 02. | All 17 cells show their art; a browse of every cell shows its model and wallpaper; RANDOM reaches all sixteen; Training and a P3 room select every stage; 0 failures on the all-content walk ROM. |
| T7 | **Stage music.** Generalize the P4 song path (`scripts/p4/p4_audio.py` `build_bgm`, `scripts/sfx/bgm/render-audio-bgm.py`): read the music bank from the staged ROM, where Remix appends its instruments (`midi.asm`, `instrument_count` from 0x2A), apply Remix's per-song overrides (master volume, bend range, priority), and fail on a program outside the bank instead of resolving it to the percussion instrument. Append rows the way `nds_p4_bgm.generated.inc` does. Final Destination plays the Master Hand intro (24), then the Final Destination theme (25) at 840 frames (`FD.asm` `music_fix_`). | The four Remix songs render with 0 out-of-bank programs and correct loops, the five affected P4 victory songs re-render, and an owner listening pass against the Remix ROM finds no wrong instrument. |
| T8 | **Hazards and movers.** Bowser's Stadium's bombs: the vanilla TaruBomb item (native since the Race to the Finish work) with Remix's model, header and animation files drawn through the P4 item-article path (`scripts/p4/p4_articles.py`), and Remix's spawn, hover, respawn, flash, explosion, sound and hitstun rules ported in `src/port/nds_p5_bowserb.c`. Dinosaur Land's movers: layers 0-1 AnimJoints through the generic layer-animation path that drives Peach's Castle, Congo Jungle and Saffron City's collision. Final Destination's VS wallpaper: `grWallpaperMakeDecideKind`'s Last case runs the boss backdrop only in the 1P scene. | The natural-cycle witnesses on the Bowser's Stadium, Dinosaur Land and Final Destination cards pass. |
| T9 | **Camera and bounds.** Hyrule Temple and Dinosaur Land take the Mushroom Kingdom camera status (Remix `Camera.asm`); the others keep the default. Bounds, spawns and whistle altitude come from each header. Check DS framing at the widest bounds (Hyrule Temple). | Camera extremes per stage match the Remix ROM's framing at matched positions within the DS aspect. |
| T10 | **Per-gkind presentation and rule rows.** Training wallpaper and magnifying-glass colour through Remix's `background_table` parent (Sector Z for Meta Crystal, Duel Zone, Final Destination and Bowser's Stadium; Peach's Castle for First Destination, Hyrule Temple and Dinosaur Land); no ground effects; item weights from each header (Final Destination has none); fog, light angle and emblem colours from each header; pause camera and sudden death on the stage's bounds. | Every row exists for every P5 gkind; Training opens each stage with its parent's wallpaper. |
| T11 | **Acceptance probe.** `scripts/p5/probe_stage.ps1 -Stage <name>`, in the repository (P4's S10 never became one command): build check, a browse of all 17 select cells, a four-CPU lab match on the stage with items (cast preset, then the heaviest admitted P4 roster), the counters and captures in shared plan 04, Training entry, and the card's witness block. | First Destination passes it and its block is in its card. |
| T12 | **Worst-case search.** The standing gate is the measured argmax over landed content. Rank the seven by stage-only cost (cast preset, four CPUs, items), then run the heaviest stage with the heaviest rosters. | The gate configuration is re-derived and recorded; every P5 stage passes with its own heaviest admitted roster. |

Order inside Board 1: T1 and T2 first (identity), then T6 with locked cells
for stages not landed yet, so the select is visible from Wave 1. T10 and T11
land with Wave 1. T3, T4 and T7 land with Wave 2. T8 and T9 land with
Wave 3. T5 is measured every wave and finished in Wave 4. T12 runs at the
end of each wave and once more at the end. T3 export work can run while a
build is in progress.

## Board 2 — stages by measured cost

Each stage is finished before the next starts.

| Wave | Stages | What the wave proves |
|---|---|---|
| 1 | Meta Crystal, Duel Zone, Final Destination | VS admission of venues that already have native packets: the registry, the identity seams, the 6x3 select, the per-gkind rows, the probe; Final Destination's VS wallpaper, music and no-item rules |
| 2 | First Destination | The donor path end to end: export, generator donor mode, collision, wallpaper, Remix music with Remix instruments. The smallest custom stage, no hazards or movers, with a secondary (translucent) layer 0 |
| 3 | Dinosaur Land, then Bowser's Stadium | Animated layers driving collision, the Mushroom Kingdom camera, asymmetric bounds; a stage hazard through the P4 article path |
| 4 | Hyrule Temple | The largest geometry and widest bounds: maxima, memory, camera framing and the gate's argmax |

## Per-stage checklist (the definition of done)

A stage is accepted when every line holds. The T11 probe checks lines 1-2,
8 and 11 and captures the evidence for lines 3-7.

1. Export and generator: 0 failures, counts pinned, regenerates
   bit-identically, host collision parity passes (Wave 1: the existing
   packet pins stay unchanged).
2. Native draw: every layer native, 0 declines, 0 native failures;
   materials, material animation, fog and lighting match the Remix ROM
   reference captures at matched camera positions.
3. Collision, pass-through platforms, ledges, blast zones, camera bounds,
   spawn points and whistle altitude as the header says; movers carry
   riders.
4. Hazards mechanically equivalent, with their telegraphs and sounds.
5. Background: the wallpaper with the source's parallax.
6. Music: the default track, its loop and the BGM restart path.
7. Stage select: icon, name, emblem, preview wallpaper, 3D preview,
   navigation, both randoms; Training's select.
8. Items per the header's weights (none on Final Destination); stage items
   interact with the stage's collision.
9. CPU: four CPUs play a full timed match and recover to the stage (the AI
   has no stage-specific branch for any of the seven; the source's branches
   test vanilla gkinds only).
10. Lifecycle: character select, stage select, load, battle, sudden death,
    Results, rematch and back; a P3 two-console match; the replay digest is
    identical across runs.
11. Performance and memory: the standing gate for the cast preset and the
    stage's heaviest admitted roster; heap low-water >= 25,600 B; 0 motion
    reads after GO.
12. Card updated: the probe's witness block, approved deltas, P3 status.

## Working rules for speed

- Iterate on the lab ROM. Build the menu and playable ROMs at each stage
  checkpoint and before handing a ROM to the owner. Stage-select bugs only
  show on a menu ROM, and the memory check uses the all-content walk ROM
  (`TARGET=smash64ds NDS_P2_MENU_WALK=1`, the playable ROM's flags).
- Generated output is never hand-edited. Per-stage facts go in
  `scripts/p5/stages.json` or the stage's C file; rules go in the
  generators.
- Read Remix's asm as assembled, next to the BattleShip source of the
  vanilla routine it patches, and port what it does, not what its comments
  say.
- The donor build stays in ignored staging and ROM-derived output stays
  under `builds/`; the repository is public.
- When a build is running, do export, census or docs work. Never edit
  source then.
- Ask the owner only about very noticeable visual approximations and
  contract changes. A measured budget conflict on a stage may use the
  owner-approved stage background LOD (2026-10-04) before anything else.
- Probe one run at a time per runner slot; reference captures come from the
  staged Remix ROM in mupen64plus (shared plan 04).

## Draft goal text (for when P5 becomes the active goal)

```text
/goal implement P5 as fast as possible against PROJECT_GOAL.md and .\docs\P5\*.
Done when:
All seven P5 stages (Meta Crystal, Duel Zone, Final Destination, First Destination,
Bowser's Stadium, Hyrule Temple, Dinosaur Land) pass the per-stage checklist with the
same polish as the original nine.
Stage select: the owner's 6x3 grid, 3D previews for all sixteen stages, RANDOM over
every unlocked VS stage, Training's select.
Four-CPU WORK P95 <= 1,120,000 with >= 95% of frames within two VBlanks on every P5
stage with items, for the cast preset and each stage's heaviest admitted roster; the
worst-case search re-derived.
No P3 or P4 regressions. Replay digest identical, 0 native failures, 0 motion reads
after GO, heap low-water >= 25,600.

Guardrails:
decomp/ is read-only, and game rules stay source code.
Native-only ROMs; measure the config that ships; keep the instrument out of the gate.
ROM-derived files stay under builds/.
Never edit source while a build is running.
Ask only about very noticeable visual approximations or a contract change.
Keep docs updated.
```

## What P5 reuses

- The P4.0 donor staging (`scripts/p4/stage_remix.py`, `stage_extra.py`) and
  ROM reader (`scripts/p4/remix_rom.py`), at the P4 source lock.
- The P2 native stage pipeline: `generate_nds_native_stage.py`, blob
  residency, `generate_compact_ground_maps.py`, `check_collision_parity.py`,
  `generate_native_wallpapers.py`, `compile_nds_stage_gx.py` and the runtime
  stage owner.
- The stage select's 3D preview (P4_STATUS, "The stage select's 3D
  previews", 2026-10-12) and its eight outside-battle lessons.
- The P4 item-article path (S6) and the existing native TaruBomb item.
- The P4 song rows (`nds_p4_bgm.generated.inc`) and the P4 CSS layout
  pattern (`scripts/p4/p4_css.py`).
- The campaign packets for Meta Crystal, Duel Zone and Final Destination.

P5 starts when the owner makes it the active goal (P4 is active today).
Its heavy-roster gate rows depend on P4's P95 pass; the cast-preset rows do
not.
