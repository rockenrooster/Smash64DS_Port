# First Destination — P5 stage card

**Status:** landed in VS (play-p5a, 2026-10-13); owner playtest of the
header fix pending.
**Source:** Remix id 0x2A (`Stages.asm`:68); map 0x877 at offset 0x14
(`Stages.asm`:323, `stage_file_table` row :4158), geometry 0x878, wallpaper
0x879. Port gkind 41. **Wave:** 2 (the donor-path prover). Parent:
[master](../New_Stages.md).

## Source observations

- `add_stage(first_destination, ...)` (`Stages.asm`:5056): default song
  `MIDI.id.FIRST_DESTINATION` ("Final Destination (Melee)", `midi.asm`:366);
  occasional and rare songs Multi-Man Melee 2 and Credits (Brawl), unused
  (owner decision 3); class BATTLE; series logo Smash; hazard type NONE.
- Ground routine `function.CLONE` (`Stages.asm`:3230): none.
- No Remix code tests its id outside the stage tables (P5.0 search of
  `Camera.asm`, `Hazards.asm`, `Item.asm`, `AI.asm`, `Spawn.asm`,
  `Blastzone.asm`).
- Select: icon offset 0xB8F8; preview zoom 0.5 (`Stages.asm`:3713);
  Peach's Castle's preview camera; Training parent Peach's Castle
  (`Stages.asm`:3938).
- The song uses programs 1, 5, 7, 10, 17, 18, 30, 34, 37, 40, 44, 45 and 52;
  44, 45 and 52 are instruments Remix appends to the bank.

## Census

Three files, no outside dependencies. Layer 0 is secondary (two DObjs, 119
triangles, opaque and translucent DLLink lists); layer 1 four DObjs (18),
layer 2 two (16), layer 3 two with no triangles. No AnimJoint or material
tables. 153 triangles, 362 vertices, 13 textures (3.4 KB) with 858 B of
palettes; lit geometry. One collision group of 3 lines, 26 map objects.
Camera bounds 5000/-2400/5000/-5000; blast bounds 7000/-4200/9221/-9219
(the top blast line is lower than any vanilla stage's).

## Dependency inventory

- Export and generator donor mode (T3, T4): this stage proves them.
- Music: the T7 bank fix (Remix instruments).
- Camera default; items from the header's weights; no hazards.
- P3 digest: generic collision loop only.

## Work sequence

1. Export, generate twice, pin the counts, pass collision parity and the
   negative controls.
2. Register the blob; the lab ROM draws it natively.
3. Registry row, select cell row 2, column 1; per-gkind rows (Peach's
   Castle Training parent).
4. Render the song with the Remix bank; probe.

## Directed witnesses

- [ ] Export and generator outputs identical across two runs; the pins hold;
      each negative control stops with its first cause.
- [ ] The translucent parts of layer 0 draw with their source alpha against
      the wallpaper and the fighters.
- [ ] A KO through the low top blast line at 7000.
- [ ] The song's instruments match the Remix ROM by ear; 0 out-of-bank
      programs in the render log.
- [ ] Reference captures from the Remix ROM match at wide, close and
      background views.

## Ledger

| Row | Value |
|---|---|
| Blob slab / body | slab 9,780; blob 10,322; GX template 15,948 (10 DObjs, 6 bindings, 38 runs, 366 of 1,120 dense vertices) |
| Collision maps | compact map + geometry in `reloc/p4` (wallpaper container out of the tree) |
| Wallpaper | converted from 0x879 (`native_wallpaper_p5_firstdest.bin`) |
| Textures / palettes | census: 13 textures, 3.4 KB; 858 B palettes |
| Heap low-water, cast preset | 69,236 (Mario, Fox, DK, Samus; 900 frames) |
| Heap low-water, heaviest roster | unmeasured |
| Four-CPU WORK P95 / two-VBlank, cast preset | unmeasured |
| Four-CPU WORK P95 / two-VBlank, heaviest roster | unmeasured |

## Closure record

Open. 2026-10-13: the owner's first playtest found no structure, no 3D
preview and instant KOs. The map's MPGroundData had no row in the ground-map
normalizer (`sNdsRelocGroundMapAssets`), so it stayed in ROM word order:
layer_mask read 0 (the adapter's topology step declined layer 0's Sec
callback, mask 0x10) and the blast bounds were scrambled. The rows now come
from the runtime header; the probe shows the preview, the desk, layer_mask 1
and bounds -5000/5000/-2400/5000.
