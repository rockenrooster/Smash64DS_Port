---
name: smash64ds-optimize-fixed-math
description: Analyze or optimize arithmetic in Smash64DS_Port using fixed-point, integer, lookup tables, precomputation, quantization, ARM9 hardware divide or square root, and generated math while preserving gameplay behavior. Use for soft-float hotspots, matrix/animation math, conversion helpers, divide/sqrt work, repeated interpolation, or proposals to change numerical representation.
---

# Optimize Fixed-Point and Hardware Math

## Mission

Replace measured expensive arithmetic with the fastest mechanically equivalent DS implementation. Define numerical contracts explicitly; do not turn approximate math into unexplained gameplay drift.

## Read first

Use CodeGraph and inspect:

- the exact BattleShip source behavior;
- compiler disassembly and soft-float call sites;
- existing Task 9/16 ARM9 labs under `scripts/`;
- hardware-divider evidence and current native campaign records;
- renderer matrix/animation paths;
- local DS references for fixed-point conventions and hardware math use.

## Workflow

1. **Profile the operation.**
   - Count calls by frame phase and owner.
   - Identify exact helpers emitted by the compiler.
   - Separate conversion, add/subtract, multiply, compare, divide, sqrt, trig, and interpolation.
   - Confirm whether values are invariant, bounded, repeated, or content-specific.

2. **Write the numerical contract.**
   Record:
   - input range;
   - output range;
   - required sign behavior;
   - overflow/saturation/wrap behavior;
   - rounding mode;
   - zero/divide-by-zero behavior;
   - acceptable error;
   - gameplay quantity affected;
   - whether exact equality is relied upon.

3. **Choose the cheapest transformation.**
   Prefer:
   - compile-time/pre-generated constants;
   - load-time precomputation;
   - content-specific tables;
   - fixed-point with range proof;
   - reciprocal/LUT for bounded repeated divisors;
   - narrow integer representation;
   - hardware divide/sqrt when measured competitive;
   - lower update rate only when the project contract permits and compensation is defined.

4. **Build a host oracle test.**
   - Generate representative and edge-case inputs from source ranges.
   - Compare the candidate against the source/oracle.
   - Report maximum absolute/relative error and mismatches in branch decisions.
   - Include collision, hitbox, animation, camera, and transform edge cases when affected.
   - Do not approve from a small hand-picked set.

5. **Inspect emitted ARM code.**
   - Confirm soft-float helpers disappeared from the intended call sites.
   - Check widening, shifts, sign extension, library calls, spills, and division.
   - Ensure an abstraction did not reintroduce conversion around every use.

6. **Measure in the real frame.**
   A micro-lab establishes instruction/cycle shape; it does not prove end-to-end value. Build same-tree A/B and measure `WORK-H` P50/P95 plus the owning bucket.

## Hardware divide/sqrt rules

Treat the hardware unit as a serialized shared resource:

- check busy/ready semantics through established libnds/local helpers;
- avoid starting and immediately blocking when useful independent work could happen;
- do not issue from interrupt and main code without ownership;
- validate signedness and operand widths;
- compare against constant reciprocal or specialized integer math;
- include call/setup/wait overhead in the measurement.

## Fixed-point checklist

- Prove worst-case intermediate width.
- Use explicit types and shifts.
- Avoid implementation-defined signed overflow or right-shift assumptions.
- Keep units in names or comments.
- Convert at subsystem boundaries, not repeatedly inside loops.
- Precompute scale constants.
- Test negative values, near-zero, maxima, and interpolation endpoints.
- Preserve branch thresholds that affect gameplay.
- For render-only values, use the visual fidelity gate rather than gameplay hashes.

## Exactness and fidelity

Name the guaranteed quantity. Examples:

- exact integer timer progression;
- same collision branch decisions over the tested state space;
- vertex screen error bounded to a stated tolerance;
- same animation frame selection;
- same source event timing.

Do not use an aggregate state hash to imply every numerical quantity is exact.

## Required result

Report:

- operation and call count;
- source range and numerical contract;
- candidate representation;
- oracle test statistics;
- emitted-code change;
- owning-bucket and `WORK-H` P50/P95 delta;
- gameplay/render fidelity gate;
- KEEP, REVERT, STOP, or WIP.
