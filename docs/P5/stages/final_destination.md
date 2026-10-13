# Final Destination — P5 stage card

**Status:** source inspected (P5.0, 2026-10-12).
**Source:** Remix id 0x10 (`Stages.asm`:38); map 0x10A (GRLastMap, 266) at
offset 0x00 (Remix `type.FINAL_DESTINATION`, `Stages.asm`:269), geometry
0x72 (StageLastFile2, 114), wallpaper 0x60. Port gkind 16 (`nGRKindLast`).
**Wave:** 1. Parent: [master](../New_Stages.md).

## Source observations

- Remix plays the vanilla files: 0x10A, 0x72 and 0x60 are byte-identical to
  vanilla and are the inputs the port's `last` descriptor pins.
- VS backdrop (owner decision 4): `grWallpaperMakeDecideKind`'s Last case
  calls `sc1PGameBossInitWallpaper` before `grWallpaperMakeCommon`
  (`grwallpaper.c:293`). Remix's `FD.asm` `fix_` (patch at 0x80104C84)
  makes that call only on the 1P battle screen, so VS shows FD's 300x220
  wallpaper with the common parallax and no boss effects. The boss camera
  and boss effects are made by the 1P scene only (`sc1pgameboss.c`).
- Music: the header's BGM 24 (Master Hand intro). Remix's `FD.asm`
  `music_fix_` counts frames from the screen's first frame and, at 0x348
  (840) frames, if the intro is still the current track, plays BGM 25 (the
  Final Destination theme) and makes it the track a Star or Hammer returns
  to.
- No items: the header's `item_weights` is NULL, and Remix keeps the
  source's gate (`itmanager.c:646`; in the assembled ROM, `lw a3, 0x84(t7)`
  and `beq a3, zero` at 0x8016EC78-0x8016EC80). With items on, nothing
  spawns.
- No ground routine (`OS.NULL`, `Stages.asm`:3203); hazard type NONE
  (`Stages.asm`:4650); series logo Smash (`Stages.asm`:4554).
- Select: icon offset 0x91F8; preview zoom 0.5 (`Stages.asm`:3687);
  Peach's Castle's preview camera; Training parent Sector Z
  (`Stages.asm`:3912).
- Camera: default status.

## Census

One layer (layer 1: five DObjs, 198 triangles) with AnimJoint and MatAnim
tables: the existing packet carries 13 material events. 355 vertices, 7
textures (7.1 KB); one collision group of 7 lines, 7 map objects. Camera
bounds 5000/-2400/5500/-5500; blast bounds 9000/-4200/11000/-11000. The
geometry file also holds the boss backdrop's effect tables (above 0x86D8),
which stay outside the packet and unused in VS.

## Dependency inventory

- Packet, collision and wallpaper: existing (campaign), qualified in the
  Master Hand fight.
- Wallpaper route: the 1P-only boss backdrop gate (T8).
- Music: the timed switch (T7), both vanilla tracks already in the BGM set.
- 1P role: the Master Hand venue; unchanged.

## Work sequence

1. Registry row; VS and Training membership; select cell row 1, column 5.
2. The wallpaper gate: VS takes the common wallpaper (Training takes its
   own wallpaper first, `grwallpaper.c:269`, with Sector Z as the parent).
3. The music switch at 840 frames, and its Star/Hammer resume track.
4. Per-gkind rows; probe in VS with four fighters and items on.

## Directed witnesses

- [ ] VS, items on at the highest rate, a full timed match: 0 items spawn.
- [ ] The intro plays from the battle scene's first frame and switches to
      the Final Destination theme at 840 frames; a Star picked up after the switch
      returns to it.
- [ ] The wallpaper is static with the common parallax; no boss effect
      GObj exists in VS.
- [ ] The platform's joint and material animation run as in the campaign.
- [ ] The Master Hand fight is unchanged, backdrop and camera included
      (regression).

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
