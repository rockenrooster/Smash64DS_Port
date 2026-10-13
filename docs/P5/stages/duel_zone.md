# Duel Zone — P5 stage card

**Status:** source inspected (P5.0, 2026-10-12).
**Source:** Remix id 0x0E (`Stages.asm`:36); map 0x10C (GRZakoMap, 268) at
offset 0x14, geometry 0x74 (StageBattlefieldFile2, 116), wallpaper 0x61.
Port gkind 14 (`nGRKindZako`). **Wave:** 1. Parent:
[master](../New_Stages.md).

## Source observations

- Remix plays the vanilla files: 0x10C, 0x74 and 0x61 are byte-identical to
  vanilla and are the inputs the port's `zako` descriptor pins.
- No ground routine (`OS.NULL`, `Stages.asm`:3201); hazard type NONE
  (`Stages.asm`:4648); series logo Smash (`Stages.asm`:4552).
- Music: the header's BGM 36 (Remix `BGM.stage.DUEL_ZONE`, the Fighting
  Polygon theme, `BGM.asm`:1467); no Remix override for the main stage.
- Items: the header's weights (`268_GRZakoMap.c`).
- The header's team bounds belong to the 1P Polygon fight; VS uses the
  normal bounds.
- Select: icon offset 0x8838; preview zoom 0.5 (`Stages.asm`:3685);
  Peach's Castle's preview camera; Training parent Sector Z
  (`Stages.asm`:3910).
- Camera: default status.

## Census

One layer (layer 1: two DObjs, 138 triangles), no material tables; 237
vertices, 10 textures (10.5 KB); one collision group of 15 lines, 18 map
objects. Camera bounds 3000/-2000/4000/-4000, the tightest of the sixteen;
blast bounds 9000/-4200/9000/-9000.

## Dependency inventory

- Packet, collision and wallpaper: existing (campaign), qualified in the
  Polygon team fight.
- 1P role: the Fighting Polygon Team venue; unchanged.
- P3 digest: generic collision loop only.

## Work sequence

1. Registry row; VS and Training membership; select cell row 1, column 4.
2. Per-gkind rows: Sector Z's Training wallpaper and magnifier colour.
3. Probe in VS with four fighters and items.

## Directed witnesses

- [ ] Drop through every pass-through platform the collision marks, and
      grab each ledge from both sides.
- [ ] Four CPUs with items under the tight camera bounds: framing holds all
      four fighters; KO through each blast side.
- [ ] The campaign's Polygon team fight is unchanged (regression).
- [ ] The select shows its icon, composed name, Smash emblem, wallpaper and
      model.

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
