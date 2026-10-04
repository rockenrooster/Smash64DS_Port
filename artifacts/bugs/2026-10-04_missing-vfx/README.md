# Missing VFX (owner r75) — 2026-10-04

Owner: "Getting missing VFX again ... Kirby vs Fox on Peach's Castle, items very
high, only fire flowers." Sector Z row: 4 level-9 CPUs (Ness/Kirby/Fox/Yoshi),
all items very high.

Probe: scratchpad `vsfx.ps1` (VS walk ROM, battle descriptor poked at
`scVSBattleStartScene` with word read-modify-writes; STAT every 300 presented
frames: arena, GObj cap, effect-struct pool, particle pools, reject count,
quad-miss count and mask, native failures; RING = the particle reject ring).

## Before (walk-all2, r75 code)

| run | rejects | quad misses | ring |
|---|---|---|---|
| vsfx01 Kirby/Fox Castle, fire flowers | 11 | 29 (texture 28) | — |
| sz4p01 Sector Z 4P all items | 56 | 392 (textures 28, 35, 36) | bank 12 script 12 x38 (Kirby inhale, bank empty), bank 2 script 0x69 x14 (item swirl, reason 4 unreachable), bank 5 script 3 x3 (Yoshi egg, bank empty) |

Heap, GObj latch, effect floor and particle pools were healthy in both; the
effect-struct pool touched 0 once in the 4P all-items match (source depth 38,
same as the N64).

## Causes

1. Atlas textures 28/31/35/36 held out by name (P1 ruling): DamageElectric,
   the children of half of DamageNormalLight's variants, Purin's notes.
2. Five public efmanager makers not in the reach seeds (ItemSpawnSwirl,
   Ripple, KirbyStar, Psionic, DustCollide).
3. Kirby's (particles_unk0) and Yoshi's (particles_unk2) fighter banks were
   never packed, so they registered empty.
4. Yoster's vapor drew common texture 0 (no stride).

## After (walk-all3)

| run | rejects | quad misses | native failures |
|---|---|---|---|
| vsfx03 Kirby/Fox Castle, fire flowers, 2 min + sudden death | 0 | 0 | 0 |
| sz4p02 Sector Z 4P all items, 2 min + sudden death | 0 | 0 | 0 |

The generator now refuses to bake an atlas that excludes a packed texture or a
reach seed list that misses a public maker.
