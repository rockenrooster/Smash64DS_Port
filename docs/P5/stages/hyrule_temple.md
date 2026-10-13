# Hyrule Temple — P5 stage card

**Status:** source inspected (P5.0, 2026-10-12).
**Source:** Remix id 0x55 (`Stages.asm`:111); map 0x981 at offset 0x14
(`Stages.asm`:366, `stage_file_table` row :4201), geometry 0x982, wallpaper
0x983. Port gkind 43. **Wave:** 4 (largest). Parent:
[master](../New_Stages.md).

## Source observations

- `add_stage(htemple, ...)` (`Stages.asm`:5110): default song
  `MIDI.id.HYRULE_TEMPLE` ("Temple Theme (Melee)", `midi.asm`:414);
  occasional and rare songs Great Temple, Dark World and Sky World, unused;
  class BATTLE; series logo Zelda; hazard type NONE. Its item-rate arguments
  are for Remix's own items, outside the profile.
- The header's `bgm_id` is 9 (Hyrule Castle's track); Remix plays the
  `add_stage` default instead. The registry takes the Remix song.
- Ground routine `function.CLONE` (`Stages.asm`:3273): none.
- Camera: Remix routes the match camera to the Mushroom Kingdom routine
  (`Camera.asm`:67; status 3 is `nGMCameraStatusInishie`). The fixed-camera
  coordinates at `Camera.asm`:322 belong to Remix's fixed-camera option,
  outside the profile.
- Select: icon offset 0x217F8; preview zoom 0.2 (`Stages.asm`:3756), the
  smallest of the seven; Peach's Castle's preview camera; Training parent
  Peach's Castle (`Stages.asm`:3981).
- The song uses programs 5, 7, 8, 12, 14, 15, 18, 44, 52 and 60; 44, 52 and
  60 are Remix instruments.

## Census

Layer 0 secondary (ten DObjs, 195 triangles), layer 1 four DObjs (258),
layer 3 ten DObjs (74). No AnimJoint or material tables. 527 triangles,
about 1,215 unique vertices from 4,317 vertex loads, 14 textures (12.8 KB)
with 556 B of palettes. Two collision groups, 35 lines, 48 map objects.
Camera bounds 4000/-7500/9000/-9000 and blast bounds
8200/-10000/13500/-13500, the widest of the sixteen.

## Dependency inventory

- Export and generator donor mode; the largest packet the generator has
  produced (shared plan 03, the workspace maxima).
- Camera status rows (T9) and DS framing at the widest bounds.
- Music: the T7 bank fix.
- CPU: the source AI has no stage-specific branch for this stage; the
  stage's size and multiple levels are the CPU's hardest recovery test in
  P5.
- P3 digest: generic collision loop only.

## Work sequence

1. Export and generate; measure the blob against the slab and body caps and
   the static workspaces; size workspaces per blob if T5 decides so.
2. Register the blob; registry row; select cell row 2, column 2.
3. Camera status; framing check at each bound with four fighters spread out.
4. Song; per-gkind rows; probe with the cast preset, then the heaviest
   roster; T12 ranks it.

## Directed witnesses

- [ ] Every surface group walked, including both collision groups; each
      ledge grabbed from both sides.
- [ ] Camera: four fighters at opposite extremes stay framed as in the Remix
      ROM at the same positions, within the DS aspect.
- [ ] A KO through each of the four distant blast lines.
- [ ] Four CPUs with items for a full timed match: no CPU loops at an edge
      or stops recovering; KOs happen at plausible rates.
- [ ] Geometry per frame at the widest camera stays under the hardware
      lists; the FPS row's line-buffer and RAM overflow counters stay 0.
- [ ] No vanilla stage's heap ledger grows after this stage registers.

## Ledger

| Row | Value |
|---|---|
| Blob slab / body | unmeasured (expected the largest) |
| Static workspace growth | unmeasured |
| Textures / palettes | census: 14 textures, 12.8 KB; 556 B palettes |
| Heap low-water, cast preset | unmeasured |
| Heap low-water, heaviest roster | unmeasured |
| Four-CPU WORK P95 / two-VBlank, cast preset | unmeasured |
| Four-CPU WORK P95 / two-VBlank, heaviest roster | unmeasured |

## Closure record

Open.
