# Yoshi's Island — P2-4 stage 1 (pipeline prover)

Status: static layers in native admission probe 2026-09-07 (see block below) · Reference: BattleShip stage data via `docs/DECOMP_MAP.md`
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

## Native admission status (2026-09-07)

MEASURED. Static layers reach the owner: `summary-a1.txt` reads `stage_reject_reason=6 fail_step=10`, `summary-a4/a5.txt` read `fail_step=0`; shots `artifacts/visibility/2026-09-06_stage-admission-yoster-a{1,2,4}-shot1.png`. Probe `builds/resume-20260905/stage-qa/stage-admission-all.ps1`.
- Gap: the three runtime clouds have no native route — excluded from the descriptor (`scripts/stages/native_stage_descriptors/yoster.py:44-51`); on the snapshot ROM the cloud path rejected 100% (`builds/resume-20260905/stage-qa/QA-RESULTS.txt`: `YosterCloud cb=4504 tri=0 rej=4504`). PROPOSAL in `builds/resume-20260905/agents-0906/stage_actor_admission.final.md:19-20`.
- Byte lanes: `ndsRelocNormalizeGroundDataBounds` layer_mask + fog/emblem (`src/port/reloc_backend_assets.c:8910-8930`); wallpaper Sprite header (see `docs/BUGS.md`).
- The 2026-09-03 rejected-owner section below is record, not current state: the packet since landed.

## A minimal native owner was attempted and rejected (2026-09-03, record)

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
