# Meta Crystal — P5 stage card

**Status:** source inspected (P5.0, 2026-10-12).
**Source:** Remix id 0x0D (`Stages.asm`:35); map 0x10D (GRMetalMap, 269) at
offset 0x14, geometry 0x75 (StageMetalFile2, 117), wallpaper 0x62.
Port gkind 13 (`nGRKindMetal`). **Wave:** 1. Parent:
[master](../New_Stages.md).

## Source observations

- Remix plays the vanilla files: 0x10D, 0x75 and 0x62 in the staged Remix
  ROM are byte-identical to vanilla, and their payload hashes are the
  inputs the port's `metal` descriptor pins
  (`scripts/stages/native_stage_descriptors/metal.py`).
- No ground routine: Remix's `function_table` holds `OS.NULL` for ids
  0x09-0x28 (`Stages.asm`:3196-3227), and the source makes ground actors
  only for the nine VS stages (`grmainsetup.c:33`).
- Hazard type NONE (`Stages.asm`:4647); series logo Mario Bros.
  (`Stages.asm`:4551).
- Music: the header's BGM 37 (Remix `BGM.stage.META_CRYSTAL`, the Metal
  Mario theme, `BGM.asm`:1469). Remix sets no default or alternate song for
  the main Meta Crystal; its alternates belong to the DL and "Meta Crystent"
  variants, which are out of scope.
- Items: the header's weights (`269_GRMetalMap.c`, the non-JP row).
- Select: icon offset 0x7E78 in STAGE_ICONS; preview zoom 0.5
  (`Stages.asm`:3684); Peach's Castle's preview camera (non-vanilla rule);
  Training parent Sector Z (`Stages.asm`:3909).
- Camera: default status.

## Census

Four layers of two DObjs each (48, 72, 7 and 26 triangles), all with
MObjSub and MatAnim tables: the existing packet carries 13 material events.
153 triangles, 362 vertices, 8 textures (2.4 KB), one collision group of 7
lines, 17 map objects. Camera bounds 5000/-2400/5000/-5000; blast bounds
9000/-4200/10000/-10000.

## Dependency inventory

- Packet, collision and wallpaper: existing (campaign), qualified in a
  two-fighter 1P fight only.
- 1P role: the Metal Mario venue; unchanged.
- P3 digest: no ground state beyond the generic collision loop.

## Work sequence

1. Registry row (T1); VS and Training membership (T2); select cell row 0,
   column 5 (T6).
2. Per-gkind rows (T10): Training wallpaper and magnifier colour from Sector
   Z; no ground effects.
3. Probe (T11) in VS with four fighters and items: the packet's first
   four-fighter, item-dense workload.

## Directed witnesses

- [ ] A four-CPU VS match with items: all four layers draw natively with
      their material animation, 0 declines.
- [ ] A KO through each blast side; spawns and respawns at the header's
      points.
- [ ] The campaign's Metal Mario fight is unchanged (regression).
- [ ] The select shows the Remix icon, the composed name, the Mario Bros.
      emblem, the wallpaper at 0.37 and the model at 0.5.

## Ledger

| Row | Value |
|---|---|
| Blob slab / body | existing packet; record from the registry |
| Heap low-water, cast preset | unmeasured |
| Heap low-water, heaviest roster | unmeasured |
| Four-CPU WORK P95 / two-VBlank, cast preset | unmeasured |
| Four-CPU WORK P95 / two-VBlank, heaviest roster | unmeasured |

## Closure record

Open.
