# Shared plan 04 — Evidence required before a stage is complete

**Status:** plan; extends the P2 stage qualification (`docs/p2/P2-4-stage-production.md`)
and the P4 verification rules. Parent: [master](../New_Stages.md).

## Four evidence levels

| Level | What it proves | What it does not prove |
|---|---|---|
| Source inspected | Named source slices and the census support the card's facts. | Correct conversion or runtime behaviour. |
| Import qualified | The pinned donor resolves to deterministic, classified native-production input (shared plan 01). | A correct running stage. |
| Behaviour qualified | Directed and natural-cycle tests pass in VS and Training. | The gate under every legal roster. |
| Release admitted | Behaviour, native rendering, resources and the standing gate pass, with evidence in the card. | Modes outside the declared tier. |

All seven cards are at the first level after P5.0. Status never jumps from
"selectable" or "draws" to complete.

## Gate A — Export and generator

The staging matches the source lock; the export and the generator run twice
with identical outputs (compare canonical hashes, not timestamps); the pins
hold; every negative control in shared plan 01 fails with its first cause.
Wave 1 stages confirm their existing packet pins are unchanged.

## Gate B — Stage behaviour

- Collision: host parity, then a runtime walk of every surface group: land,
  walk, run and stop on each floor; drop through each pass-through platform;
  grab each ledge from both sides; walls and ceilings where present.
- Blast zones: a KO through each of the four sides; spawn and respawn points;
  the whistle altitude warning.
- Movers (Dinosaur Land): ride each moving group through a full cycle at
  several phases; land, stand, jump off and drop through; an item resting on
  a mover moves with it. Watch the visible carrier and the collision in the
  same frames.
- Hazards (Bowser's Stadium): the natural cycle on its card, with sounds and
  telegraphs, and the hitstun rule checked against a direct calculation for
  two fighters of different gravity.
- Items: a long run with items on spawns by the header's weights; Final
  Destination spawns none.
- CPU: four CPUs play a full timed match; no fighter loops on a stage edge
  or stops acting.

## Gate C — Presentation against the donor

- Reference captures come from the staged Remix ROM in mupen64plus (the
  vanilla tooling: `mupen64plus-ui-console` 2.5 in `D:\Games\N64`, headless
  `--testshots`). Boot a VS battle on the Remix stage id and the stage-select
  page that shows it, poking the scene data as the vanilla cheats do; check
  the poke addresses on the Remix ROM first.
- Compare at matched camera positions: wide at GO, a platform close-up, the
  background, sudden death, and the select's preview. Check materials,
  material animation, fog, lighting and the wallpaper's parallax.
- Music: an owner listening pass against the Remix ROM for the four Remix
  songs, after the T7 bank fix.
- The select: every cell's icon, name, emblem, preview wallpaper and model.
- The native-only law applies: no generic-renderer path, and a stage that
  draws by hiding a layer is not complete.

## Gate D — Lifecycle

Character select, stage select, load, battle, sudden death, Results,
rematch and back to the stage select, which then shows every preview;
Training on the stage; a P3 two-console room on the stage with the sync
check green; two runs of the same seeded match give the same replay digest.
A 30-minute soak cycling every landed stage closes each wave.

## Gate E — Resources and the gate

The card's ledger (shared plan 03), the cast-preset four-CPU run and the
heaviest-roster run, both with items. The final worst-case search (T12)
supplements the per-stage rows and does not replace them.

## The probe (T11)

`scripts/p5/probe_stage.ps1 -Stage <name> [-Roster <preset|heaviest|list>]`
runs, one emulator at a time on its runner slot:

1. Build check: export and generator report 0 failures, pins unchanged, the
   blob registered, the stage's NitroFS files present.
2. Select browse: every cell left to right, 60 tics each, through the
   shell's counters (cursor slot, plaque and preview blits, preview load
   steps and failures, warm-texture releases).
3. Battle: a four-CPU lab match on the stage, items on, 1,960 presented
   frames. Counters: native failures, stage declines, texture misses, heap
   low-water, motion reads after GO, FGM misses, WORK P50/P95/P99 and the
   two-VBlank count, the FPS row's line-buffer and RAM overflow counts, the
   camera status, and the stage's own counters (bomb spawns, explosions and
   respawns; mover rides).
4. The same with the stage's heaviest admitted roster.
5. Training entry on the stage.
6. Captures: wide at GO, platform close-up, background, sudden death, the
   select cell; saved under `artifacts/visibility/` with the probe tag.
7. Output: the witness block pasted into the card.

## Closure record

Each card's closure lists: the source lock and staging manifest hashes, the
export and packet pin hashes, the ROM build ids, the ledger rows, the
capture paths beside their Remix references, the gate rows, the Boundary and
P3 regression results, and any owner-approved delta with its reason.
