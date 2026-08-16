# Campaign 13 — Finish Draw / Render-Side Fixed Point

> **Planning baseline:** `codex/r2-runtime2` at `a63dd0e4b3af9c6601713f70c179c96c0aa67735` (2026-08-16).
>
> If HEAD has moved when implementation begins, re-run the inventory/measurement steps first and update symbol names/line references rather than blindly applying this document.
>
> **Campaign rule:** optimize toward a DS-native architecture and four-fighter headroom. The current two-fighter P95 gate is a checkpoint, not the architectural finish line. Never bank projected savings; measure the shipping configuration. Prefer same-binary route A/B when practical because this tree is placement-sensitive.

## Objective

Finish draw-side transition to DS-native fixed math in the **current shipping** profile: fixed camera and `GX_COMPOSE=0`.

The board records **≈22,521 tk/fr** of draw-side soft-float remaining after the
camera shipped in Q20.12 (`docs/P1_EXECUTION_BOARD.md` "THE DRAW-SIDE
SOFT-FLOAT SPLIT RE-BASES" note: the old shared/sim/draw split is stale on the
draw row, and the camera chain's 11,504 gross is no longer called). The board
itself marks this figure as needing re-derivation — treat it as an opportunity
estimate, not guaranteed savings, and note the split's basis was the old
`GX_COMPOSE=1` census.

Calibration: inside `FTR` — the run's largest lane — soft float is only
**4,038 tk/fr (1.4%)** (`FTR_LANE.md` phase split). Most of the remaining
draw-side float therefore lives *outside* the fighter draw lane (stage,
particles/VFX, adapters, dispatch); pick targets from the census, not from
fighter-lane intuition.

First realistic target: **10–20K ticks/frame**.

## Target domains

Re-census:

- `guMtx*` construction/composition;
- normalization;
- lighting/vector transforms;
- projection/adapter transforms;
- particle draw transforms;
- renderer float→fixed boundaries;
- values already Q20.12 upstream but routed through float.

## Current repo anchors

- `src/nds/nds_renderer.c`
- `src/port/reloc_backend_renderer_dl.c`
- fixed camera/matrix code
- `scripts/classify-softfloat-caller-phase.py`
- `scripts/census-softfloat-callers.ps1`
- renderer parity/differ tools
- Campaigns 05 and 11

## Phase 0 — Re-derive current draw float lane

**No build is needed for this phase.** The shipping-configuration per-PC
census the board asked for already exists: `v3-c221`
(`build-c221-sitrprof`, `GX_COMPOSE=0`, 1,600 frames — the first per-PC census
ever taken in the shipping configuration; see the board's `SITR_EXCURSION`
entry). Re-derive the draw-side split from that capture before proposing any
slice.

For every draw-side float caller record:

- operation/helper;
- calls/frame;
- owner/subphase;
- input producer representation;
- output consumer representation;
- render-only vs gameplay-visible;
- marginal-80 participation.

Replace stale estimates with this census.

## Phase 1 — Eliminate conversion sandwiches

Highest priority:

`fixed/native producer -> float work -> fixed GX submit`

or

`integer asset -> float normalize/scale -> fixed vertex`.

Move representation boundaries so the **whole path** stays fixed.

Do not optimize one multiply while retaining conversions around it.

## Phase 2 — Matrix chains

For render-only matrix paths:

- keep camera/projection matrices fixed;
- compose fixed local/world matrices where CPU composition still exists;
- feed GX-native values directly.

Coordinate with Campaign 05: do not optimize CPU composition that GX offload will delete.

## Phase 3 — Normalization/lighting

Range-census vectors.

Evaluate:

- fixed dot products;
- fixed length-squared;
- proven fixed sqrt/reciprocal-sqrt;
- AOT unit data for immutable normals;
- AOT-transformed static normals where legal.

Maintain visual shading precision.

## Phase 4 — Projection/adapter boundaries

Adapters existing only because generic N64 structures are float should disappear from Campaign 11's native path.

They may remain in oracle-only generic code.

## Phase 5 — Particles/VFX

Read `docs/OPTIMIZE_LIST.md`'s billboard observation first (owner, 2026-08-06):
on N64 every VFX except the platform is a camera-facing billboard at the
fighter's own Z, always drawn on top. If confirmed against BattleShip, effects
need no per-effect 3D geometry and no depth reasoning — which bounds how much
fixed-point transform work effects should have at all.

For hot P1 effects:

- keep position/scale/rotation fixed where possible;
- bake static quad/UV geometry;
- convert only truly dynamic legacy values;
- submit GX-native values without generic float wrappers.

Do not expand into a whole particle rewrite unless profiling supports it.

## Phase 6 — Retire draw helper families

After each domain conversion, rerun Campaign 06.

Account separately for:

- runtime arithmetic/conversion gain;
- later ITCM helper dividend.

## Verification

- renderer parity corpus;
- fixed-camera pixel/visual comparison;
- matrix/vertex range and saturation counters;
- lighting/color diff;
- VFX tests;
- no gameplay state differences;
- soft-float calls before/after;
- draw/FTR/STG/VFX buckets;
- P50/P95;
- any banked gate claim reports the 2/3/4/5+ VBlank-interval histogram and max
  interval (AGENTS.md device-report law).

## Completion criteria

Native draw keeps Q/native formats from producer through GX for all practical P1 domains. Remaining floats are explicitly justified, and the **current shipping configuration** shows measured reduction.
