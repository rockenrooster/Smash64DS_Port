# Shared plan 03 — Admit each stage into the DS envelope

**Status:** plan; census sizes only, no runtime measurement yet. Parent:
[master](../New_Stages.md).

## Principle: the stage is part of the selected match

A stage competes with the fighters, items and effects of the match it is in.
Its resident cost is the packet blob, the compact collision maps, the
wallpaper, its textures and palettes, any hazard assets and the BGM stream
buffers. All of it is ready before GO; nothing loads in response to combat.
The owner chose lower content fidelity over a character-select budget
(2026-10-08), so a stage may not make a legal roster unplayable: every P5
stage must hold the heaviest admitted roster at the heap floor.

```text
stage resident cost = blob slab + blob body + collision maps + wallpaper
                    + stage textures and palettes + hazard files
                    + stage share of the static workspaces
```

## Evidence each card records

| Domain | Evidence |
|---|---|
| Main RAM | blob slab and body bytes, collision bytes, wallpaper bytes, hazard files; heap low-water with the cast preset and with the heaviest admitted roster, beside Dream Land and Sector Z with the same rosters |
| Static workspaces | the stage's counts against `nds_native_stage_blob_maxima.generated.h` (segment, run and dense-vertex workspaces are sized by the maximum over every registered stage, `nds_native_stage_select.inc`) and against the blob slab and body caps (`nds_native_stage_blob.c`) |
| Texture VRAM and palettes | the stage's static texture set and palettes placed with four fighters, items and entry effects (P4 found entry-texture VRAM exhaustion with four Crashes) |
| Geometry | polygons and vertices per frame with four fighters at the widest camera, against the 2,048-polygon and 6,144-vertex hardware lists; the battle FPS row's line-buffer underflow and RAM overflow counters stay 0 |
| CPU | four-CPU WORK P50/P95/P99 and the two-VBlank count; the stage draw's share against Dream Land's |
| I/O and audio | the song's stream rate and buffers (the existing BGM path); hazard sounds in the FGM pack; the select's preview load steps |
| ROM | file sizes; ROM is a cheap resource (PROJECT_GOAL), recorded, not budgeted |

## The workspace maxima

The runtime keeps static workspaces sized by the largest registered stage.
Census: Hyrule Temple has about 1,215 unique vertices against today's
maximum of 863 dense vertices, and about 527 source triangles against 451
for the largest registered packet (Jigglypuff's Board the Platforms board),
so registering it would grow those workspaces for every stage, vanilla ones
included. T5 decides by
measurement: if the growth costs a vanilla stage's heap or a P4 roster's
floor, size those workspaces per blob (allocated with the stage's match
resources from the blob's own counts) instead of statically. The blob slab
cap (`NDS_NATIVE_STAGE_BLOB_MAX_SLAB_BYTES`, 21,573 B today) and body cap
rise only with the measured largest blob.

## Hyrule Temple

- 527 source triangles, ~1,215 unique vertices, 14 textures (12.8 KB): the
  largest P5 packet.
- Widest camera of the sixteen (bounds 4000/-7500/9000/-9000): more stage on
  screen, fighters drawn smaller. Check the fighter LOD rules (owner D14) at
  that zoom; the gate's four-fighter configuration on this stage is the
  likely new argmax (T12).
- If it misses the gate after exact work, the first lever is the
  owner-approved stage background LOD (2026-10-04) on far layers. Anything
  else needs owner approval with measurements.

## Bowser's Stadium's bombs

- At most two bomb items live (one per side) plus their explosions; Remix
  recreates a side only after its bomb is gone (300-frame wait).
- Files: Remix 0x9B1 (model), 0x9B2 (header with the hitbox; its word 0xA8
  names the model) and 0x9C8-0x9CA (ground, air and land animations,
  `File.asm`:364-389). They load with the stage before GO, through the P4
  article path, not at first spawn.

## Dinosaur Land

Two animated layers and five collision groups update every tick through the
generic layer-animation path; measure that path's tick cost beside Peach's
Castle and Congo Jungle, the vanilla stages that use it.

## The stage select

One stage model is resident at a time and released when the cursor moves;
warm textures are released per stage. Measure the select on the all-content
walk ROM (`TARGET=smash64ds NDS_P2_MENU_WALK=1`): the lab-config menu ROM
has about 245 KB more free memory and hid a 2026-10-07 regression where
every preview went dark. Eighteen plaque surfaces and seventeen preview
wallpapers live in NitroFS and cost no resident memory.

## Failure policy

Fail before GO with the first missing resource or failed limit. A missing
texture must not drop a layer silently or invalidate the stage packet; a
rejected stage is a failed admission, not a finished port.

## Exit

Every card carries its ledger rows. T5 is done when each P5 stage with its
heaviest admitted roster keeps heap low-water >= 25,600 B and 0 texture
misses, no vanilla stage's ledger grows, and the select opens on the
all-content walk ROM with every preview.
