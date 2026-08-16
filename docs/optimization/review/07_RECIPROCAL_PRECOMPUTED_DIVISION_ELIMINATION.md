# Campaign 07 — Reciprocal / Precomputed Division Elimination

> **Planning baseline:** `codex/r2-runtime2` at `a63dd0e4b3af9c6601713f70c179c96c0aa67735` (2026-08-16).
>
> If HEAD has moved when implementation begins, re-run the inventory/measurement steps first and update symbol names/line references rather than blindly applying this document.
>
> **Campaign rule:** optimize toward a DS-native architecture and four-fighter headroom. The current two-fighter P95 gate is a checkpoint, not the architectural finish line. Never bank projected savings; measure the shipping configuration. Prefer same-binary route A/B when practical because this tree is placement-sensitive.

## Objective

Remove repeated division only where the denominator is constant, invariant for a useful interval, or belongs to a bounded integer domain with a proven exact multiply/shift replacement.

Do **not** blindly rewrite arbitrary IEEE division.

## Phase 0 — Census shipping divides

The surface is already sized: `__aeabi_fdiv` is **10,084 tk/fr over 308,426
calls at 117.9 cycles each — the most expensive helper in the build by 3.2×**
(board, fixed-point census), and `SIMSIDE.md` puts fdiv 13,818 + sqrtf 8,068 =
**21,886 tk/fr** of bit-exact helper-acceleration surface on the marginal-80
(conversion unmeasured). The `v3-c221` shipping-config capture re-derives these
without a build.

From linked ELF/source enumerate:

- software float divides;
- integer divide helpers;
- hardware divider usage if any;
- explicit reciprocals;
- normalization code with repeated divide.

Classify each denominator:

1. build-time constant;
2. asset/motion constant;
3. load/match-time constant;
4. per-state constant for many frames;
5. per-frame shared;
6. truly variable/general IEEE.

Prioritize 1–5.

## Phase 1 — AOT constants

For classes 1–2:

- precompute fixed rational coefficients or exact magic multiplier/shift pairs in host generators;
- store only runtime-needed representation;
- verify all legal inputs against original behavior.

Strong integrations: animation segment durations/scales, asset dimensions, immutable stage math.

Use shifts for power-of-two scales.

## Phase 2 — Load/motion-time reciprocals

For denominators stable during an asset/motion/state:

- compute once outside GO or at state transition;
- store compact coefficient;
- reuse in hot loop.

Do not replace one per-frame divide with another divide plus a store every frame.

Animation phase must preserve exact rounding and avoid accumulated drift.

## Phase 3 — Per-frame shared reciprocals

If many operations divide by the same frame value:

1. prove denominator identity;
2. compute once;
3. share it;
4. ensure reciprocal creation itself is not generic expensive float work.

## Phase 4 — Exact integer multiply/shift

For bounded integer/Q domains:

- derive multiplier/bias/shift with host tooling;
- exhaustively test small domains;
- otherwise prove bounds and test edge neighborhoods;
- preserve signed truncation/rounding exactly.

Never reuse magic constants from a different signedness/rounding rule.

## Phase 5 — Leave true IEEE divides alone

If NaN/Inf/zero/sign/rounding behavior is materially possible, keep IEEE division unless Campaign 12/13 first moves the entire domain to fixed.

## Deliverables

- divide callsite census;
- denominator classification;
- exactness tests for each replacement;
- before/after helper reachability report.

Coordinate with Campaign 06 when final divide callers disappear.

## Verification

- exact host comparison;
- simulation state-hash A/B;
- renderer parity if draw-facing;
- animation traces if motion-facing;
- call counts;
- whole-frame timing;
- any banked gate claim reports the 2/3/4/5+ VBlank-interval histogram and max
  interval (AGENTS.md device-report law);
- code/data footprint.

## Completion criteria

High-frequency repeated/invariant denominator sites are precomputed or proven exact multiply/shift operations, while truly general IEEE divisions remain unchanged and documented.
