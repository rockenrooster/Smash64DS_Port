# Dinosaur Land — P5 stage card

**Status:** source inspected (P5.0, 2026-10-12).
**Source:** Remix id 0xC1, scope `yoshis_island_melee` (`Stages.asm`:219);
map 0x1247 at offset 0x14 (`Stages.asm`:474, `stage_file_table` row :4309),
geometry 0x1246, wallpaper 0x1245. Port gkind 44. **Wave:** 3, first.
Parent: [master](../New_Stages.md).

## Source observations

- `add_stage(yoshis_island_melee, "Dinosaur Land", ...)`
  (`Stages.asm`:5259): default song `MIDI.id.SMW_ATHLETIC` ("Athletic
  Theme", `midi.asm`:545, master volume 85 at :1663); occasional and rare
  songs Super Mario World Title/Credits and Fortress Boss, unused; class
  BATTLE; series logo Yoshi; hazard type MOVEMENT.
- Ground routine `function.CLONE` (`Stages.asm`:3381, commented "Yoshis
  Island III"): none. The movement is data: layers 0 and 1 carry AnimJoint
  tables, and `grDisplayMakeGeometryLayer` gives layer 1 the
  `mpCollisionPlayYakumonoAnim` process (`grdisplay.c`:206-211, the layer-1
  row of `dGRDisplayDescs`), which moves the five collision groups with the
  geometry. Peach's Castle, Congo Jungle and Saffron City use the same path.
- Camera: Remix routes the match camera to the Mushroom Kingdom routine
  (`Camera.asm`:83).
- `Item.asm`:2695 tests this stage inside Remix's item-container switch,
  outside the profile.
- Select: icon offset 0x37F08; preview zoom 0.3 (`Stages.asm`:3864);
  Peach's Castle's preview camera; the preview shows the first animation
  frame; Training parent Peach's Castle (`Stages.asm`:4089).
- The song uses programs 13, 14, 18, 46 and 60; 46 and 60 are Remix
  instruments.

## Census

Layer 0 eight DObjs (83 triangles), layer 1 ten DObjs (177), both with
AnimJoint, MObjSub and MatAnim tables; layer 3 ten DObjs (79). 339
triangles, 484 vertices, 12 textures (7.1 KB) with 384 B of palettes. Five
collision groups, 16 lines, 32 map objects. Camera bounds
4500/-750/3500/-5000 and blast bounds 7500/-4000/6500/-8000: both lean
left.

## Dependency inventory

- Export and generator donor mode; the generator's AnimJoint and MatAnim
  paths on a donor stage for the first time.
- The generic layer-animation collision path (verify, no new runtime).
- Camera status rows (T9).
- Music: the T7 bank fix and the song's volume override.
- P3 digest: the yakumono loop covers the movers.

## Work sequence

1. Export and generate; register the blob; registry row; select cell row 2,
   column 3.
2. Confirm the movers' collision follows the drawn geometry in the same
   frames.
3. Camera status; framing at the left-leaning bounds.
4. Song; per-gkind rows; probe.

## Directed witnesses

- [ ] Ride each of the five moving groups through a full animation cycle:
      land, stand, jump off, and drop through where the line allows.
- [ ] An item resting on a mover moves with it; a fighter standing on one
      keeps its footing across direction changes.
- [ ] The drawn platforms and their collision agree at several phases
      (capture with the collision overlay in a lab build only).
- [ ] The camera frames the left side as the Remix ROM does at the same
      positions.
- [ ] Material animation on layers 0 and 1 matches the Remix ROM.

## Ledger

| Row | Value |
|---|---|
| Blob slab / body | unmeasured |
| Layer-animation tick cost | unmeasured |
| Textures / palettes | census: 12 textures, 7.1 KB; 384 B palettes |
| Heap low-water, cast preset | unmeasured |
| Heap low-water, heaviest roster | unmeasured |
| Four-CPU WORK P95 / two-VBlank, cast preset | unmeasured |
| Four-CPU WORK P95 / two-VBlank, heaviest roster | unmeasured |

## Closure record

Open.
