# Campaign 12 — Complete Simulation Math Chains → Fixed Point

> **Planning baseline:** `codex/r2-runtime2` at `a63dd0e4b3af9c6601713f70c179c96c0aa67735` (2026-08-16).
>
> If HEAD has moved when implementation begins, re-run the inventory/measurement steps first and update symbol names/line references rather than blindly applying this document.
>
> **Campaign rule:** optimize toward a DS-native architecture and four-fighter headroom. The current two-fighter P95 gate is a checkpoint, not the architectural finish line. Never bank projected savings; measure the shipping configuration. Prefer same-binary route A/B when practical because this tree is placement-sensitive.

## Objective

Convert **complete high-frequency simulation chains** to fixed point so values remain Q-format from producer through intermediates to consumer.

Use the measured **142,786 marginal-80 tk/fr simulation soft-float reservoir**
(`artifacts/performance/2026-08-16_simside-softfloat/SIMSIDE.md`: shared 59,582
/ sim-only 45,539 / sim+dispatch 37,665; 75.9% of cycles are fmul+fadd+fsub)
as a search space, not promised savings. Two caveats travel with that number:

- it was attributed on the `c200-trackprof-off` per-PC capture
  (`GX_COMPOSE=1`-era). No census on disk matches today's shipping config
  (GX compose ON since 2026-08-16 + post-ITCM-repack layout) — Phase 0 takes
  a fresh one (shared with 05/07/11/13);
- the **rate is not asserted**: measured conversion rates in this binary
  disagree by 3× (1.70× camera chain, 2.68× fighter narrow phase, 5.14×
  same-op matrix pair), so the warm-MAC subset is worth 29,437–57,584 tk/fr
  depending on the rate — bank measurements, never the reservoir.

The best-qualified subset is already named (`SIMSIDE.md` §4): **14 functions
≥80% MAC by cycles and entered ≥8×/frame = 71,491 tk/fr** — 5 collision bodies
(50,044), 3 animation bodies (13,904), 5 math leaves (7,541); the two largest
(`func_ovl2_800ED490` 18,759, `gmCollisionGetWorldPosition` 13,091) are pure
100% MAC.

The rule is:

**Q → Q → Q**

not:

**float → Q → float → Q → float**

Start with fighter physics/movement/math chains that run every frame.

## Scope separation

- Campaign 12 owns simulation representation/chains.
- Campaign 06 tracks helper disappearance.
- Campaign 07 handles invariant division.
- Campaign 03 owns animation evaluation.
- Campaign 15 keeps collision numerics unchanged; any numeric collision conversion is a separately proven Campaign 12 chain.

## Phase 0 — Rebuild current simulation float reservoir

Re-derive the census on the **current** shipping configuration (GX compose
ON, post-ITCM-repack). This needs one instrumented build; `v3-c221` no longer
matches what ships. Simulation-side ranks are less GX-sensitive than draw-side
ones, but the marginal-80 mask itself moves when the tail moves — re-mask.

For each simulation caller record:

- helper type;
- calls/frame;
- marginal-80 concentration;
- self ticks;
- parent chain;
- persistent fields read/written;
- downstream consumers;
- branch decisions using result.

Group by **chain**, not function.

Example:

`input/state -> acceleration -> velocity -> position -> map/collision decision -> response`.

## Phase 1 — Choose closed chains

Prioritize chains where:

- ranges can be bounded;
- state persists each frame;
- many operations occur before a legacy boundary;
- outputs are fixed-friendly;
- few external APIs require floats.

Likely classes to re-census:

- horizontal/vertical velocity update;
- gravity/terminal velocity;
- position integration;
- frequent fighter scalar/vector math;
- knockback/movement subchains.

Avoid isolated helpers whose result immediately returns to float-owned state.

Selection heuristics with measured teeth:

- **Edge conversions decide the exchange rate.** An f32↔Q edge conversion
  costs **31–42 cycles** (against `__aeabi_fmul` at 26.5), so the rate is set
  by **conversions per deleted operation** — a property of the chain's
  *signature*: 1.00 conv/op cannot pay, **0.57 breaks even**, and the 5.14×
  prior had conv/op = 0
  (`artifacts/performance/2026-08-16_simmac-exchange/EXCHANGE_LEAF.md`).
  Compute conv/op from the candidate's signature before building; closed
  chains beat wrapped leaves because conv/op falls as the chain lengthens.
  (`2026-08-15_cfx-narrow-exchange/EXCHANGE.md`'s 2.68× was driven by
  `__udivmoddi4` — check the divide/64-bit mix too.)
- **The collision bodies carry a renderer coupling.** The board's
  `SETUP_SHARE` entry: hit detection drives the per-joint compose (22.77
  entries/engaged frame vs 0.76 control), and the renderer *consumes*
  `parts->mtx_translate` from that chain — so a Q conversion of
  `func_ovl2_800ED490` moves **drawn geometry** as well as hit results. Such a
  chain needs Tier A/B proof covering both consumers, and the board records the
  fixed-point route there as already declined once. Prefer chains without a
  cross-subsystem consumer for the first slices.
- **Size on the tail, not the mean.** The median frame passes the gate by
  ~219K (`SETUP_SHARE`); the whole requirement is excursion in the worst ~80
  frames. Size every candidate with the `convert.py`-style uniform-D re-rank
  at rank-80 before building.

## Phase 2 — Range census and Q design

Capture live and theoretical bounds.

For every field/intermediate specify:

- signedness;
- integer/fractional bits;
- maximum product;
- accumulator width;
- overflow/saturation policy;
- rounding.

Use 64-bit intermediates where needed. Campaign 09 can retain ARM for MAC-heavy kernels if measured.

Do not choose one Q format for every domain by habit.

## Phase 3 — Host oracle

Run old float and candidate fixed chain on recorded inputs.

Check:

- numeric error;
- zero/sign crossings;
- threshold comparisons;
- status transitions;
- collision query inputs;
- trigger inputs;
- cumulative drift.

Where exact decision equivalence matters, prove fixed error cannot cross decision threshold or choose a representation that reproduces the decision.

## Phase 4 — Same-binary shadow

Before replacement, compute fixed result in shadow while float remains authoritative.

Record:

- maximum error;
- decision mismatches;
- saturation/overflow;
- divergence frames.

Any unexplained gameplay decision mismatch blocks promotion.

The shadow harness is **lab-build-only and is removed at promotion** — the
repo forbids permanent proof-only branch reruns and seed/restore wrappers
(AGENTS.md "graduate imported subsystems live"). A shadow ROM must never be
read for ticks.

## Phase 5 — Replace the **whole state chain**

When proven:

1. make Q state authoritative;
2. remove conversions inside chain;
3. make hot consumers read Q directly;
4. convert only at true legacy boundaries;
5. delete float mirror when no meaningful consumer remains.

Do not maintain two authoritative states indefinitely.

## Phase 6 — Roll out chain by chain

Each accepted chain gets:

- isolated slice;
- same-binary A/B if possible;
- targeted behavior tests;
- one-minute match;
- measured bank.

Re-run soft-float census after every banked chain.

## Phase 7 — Close helper families

Coordinate with Campaign 06 when a chain removes last callers of a software-float helper.

## Fidelity ladder

### Tier A — exact/decision-equivalent

Preferred.

### Tier B — bounded difference proven not to cross decisions

Allowed only with documented proof.

### Tier C — behavior trade

Not part of this campaign without explicit owner authorization.

## Verification per chain

- range census;
- host oracle;
- shadow comparison;
- zero unexplained decision mismatches;
- state hash/proof;
- relevant attacks/recovery/ledge/collision tests;
- one-minute match;
- soft-float calls before/after;
- P50/P95/marginal-80;
- any banked gate claim reports the 2/3/4/5+ VBlank-interval histogram and max
  interval (AGENTS.md device-report law);
- code/data footprint;
- saturation count zero unless semantically intended.

## Scaling criterion

Report per-fighter cost where possible. Estimate four-fighter impact from measured per-fighter invocation/cost, not by blindly doubling whole-frame savings.

## Completion criteria

High-frequency Mario/Fox simulation domains are fixed-native end-to-end wherever fidelity permits, persistent state no longer bounces between float and Q, helper families shrink materially, and every chain has behavior proof plus shipping timing.
