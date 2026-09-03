# Yoshi's Island — P2-4 stage 1 (pipeline prover)

Status: not started · Reference: BattleShip stage data via `docs/DECOMP_MAP.md`
(`gr/` + `mp/` collision; no stage-hazard logic).

## Why first

No hazards at all — the cheapest full pass through the whole stage pipeline
(collision import, geometry build, background, camera, music, SSS entry).

## Content inventory

- **Layout**: sloped/undulating main terrain (no flat ground — tests slope
  collision everywhere), side cloud platforms that act as soft platforms
  (verify exact pass-through/dissipate behavior in source — players expect
  the clouds to support briefly), upper platforms.
- **Hazards**: none.
- **Set pieces**: Super Happy Tree background, Fly Guys/props (background
  only — verify nothing background interacts with gameplay).
- **Camera/blast zones**: from source data.
- **Music**: Yoshi's Island track through the streaming path.
- **Visual treatment**: storybook/crayon look — strong candidate for baked
  vertex colors + 2D BG layers behind low-poly terrain.

## DS notes / risks

- Slope-heavy collision is the real test: every movement state (dash, crawl,
  knockdown slides, item bounces later) on non-flat ground.
- Cloud platform semantics are the one equivalence subtlety — source first.

## Acceptance

- [ ] Collision parity sweep (slopes, clouds, ledges, blast lines) vs
      imported data.
- [ ] Camera bounds equivalent; spawn/respawn points correct.
- [ ] Music + SSS entry live; owner visual pass with screenshot.
- [ ] 4-CPU stress measurement on this stage banked.

## A minimal native owner was attempted and rejected (2026-09-03)

A delegated pass added roughly 295 lines to
`scripts/stages/generate_nds_native_stage.py` emitting a Yoster owner with the
correct static layer topology and **zero bindings, runs, epochs or materials**,
whose own comment said Yoster "keeps rendering through the existing generic
DObj path until the full bake lands" and marked its per-layer display link and
callback values UNVERIFIED placeholders that "must NOT be read as source fact".

It was reverted rather than landed. Law 8 forbids a completed unit from drawing
through the generic renderer, and an owner that admits topology while the stage
still draws generically satisfies the letter of having an owner and none of the
point. Invented link and callback constants in a generator are worse than none:
they read as source-derived to the next person.

The reason it reached for a minimal owner is real and is the actual difficulty:
**Yoster's runtime topology is dynamic.** `grYosterInitAll`
(`gryoster.c:199-257`) builds three cloud GObjs at runtime from `map_nodes`,
each with three child DObjs (`:237-245`), so the static relocData files do not
describe the live owner the way Dream Land's eight static owners do. Dream
Land's bake could walk static display lists; Yoster's cannot, without modelling
what `grYosterInitAll` constructs.

So the native packet for this stage is not a transcription job. It needs a
decision first: either the generator learns to model the three runtime clouds
from `map_nodes` and emit them as static owners, or the clouds keep a
stage-specific runtime path and only the four static display layers are baked —
and that second option has to be measured against law 8 rather than assumed.
