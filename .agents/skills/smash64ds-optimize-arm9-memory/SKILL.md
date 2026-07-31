---
name: smash64ds-optimize-arm9-memory
description: Analyze or optimize ARM9 execution and memory behavior in Smash64DS_Port, including instruction and data cache locality, ITCM/DTCM, hot functions, stack use, alignment, memcpy/memset, compiler code generation, soft-float helpers, memory stalls, linker sections, and placement-sensitive performance. Use for CPU-bound hot paths, census results, disassembly work, cache-sensitive regressions, TCM proposals, or low-level ARM9 optimization.
---

# Optimize ARM9 Memory

## Mission

Remove ARM9 work first; improve locality only when the mechanism is measured. Preserve mechanically equivalent gameplay and the repository's evidence rules.

## Read first

Read `AGENTS.md`, `PROJECT_GOAL.md`, the current board/handoff, `docs/optimization/TASK_STANDING_RULES.md`, and `docs/VERIFYING.md`.

Use CodeGraph before grep/read. Inspect:

- `Makefile` for active optimization and lab flags;
- `linker/` and the generated map for section placement;
- `include/nds/nds_task37_itcm.h`, `linker/nds_hot_text.ld`, and current census tooling;
- `scripts/task*_arm9_lab/` for established micro-lab patterns;
- `artifacts/task37-census/` and current permanent performance evidence;
- comparable ARM9 organization in `decomp/sm64-nds` and `decomp/sm64ds-decomp`.

## Project-specific constraint

The existing placement campaign is exhausted. Tasks 87, 88, 89, and 94 regressed, and the standing rules forbid another placement move without a new measured mechanism. Do not rank a symbol by hotness and move it to TCM as the experiment. First prove why its current fetch/data behavior is the limiting cost and account for the code/data displaced or re-addressed.

## Workflow

1. **Find the exact hot path.**
   - Use the steady-state per-PC census, typed tick buckets, call paths, and disassembly.
   - Distinguish own instructions from callees such as soft-float or library copies.
   - Separate per-update, per-owner draw, and occasional tail work.

2. **Classify the cost.**
   - avoidable algorithmic work;
   - repeated loads/stores or large structure traffic;
   - compiler-generated `memcpy`/`memset`;
   - unaligned or narrow accesses;
   - soft-float/library helper calls;
   - instruction locality;
   - data locality/cache-line churn;
   - stack spills;
   - branch/call overhead;
   - ROM/main-RAM access pattern.

3. **Prefer transformations in this order.**
   - delete or hoist work;
   - precompute/generate;
   - specialize;
   - shrink the live working set;
   - change data layout;
   - inline tiny fixed-size copies/loads when disassembly proves helper calls;
   - replace expensive arithmetic through the fixed-math skill;
   - only then consider TCM or section placement with a new mechanism.

4. **Inspect generated code.**
   - Build the exact A/B configuration.
   - Use `objdump`, map files, symbol sizes, and call-site inspection.
   - Confirm the intended instruction shape actually exists.
   - Verify that a macro or attribute did not silently change unrelated placement.

5. **Make one controlled experiment.**
   - Same source tree, flag off versus flag on.
   - Keep the flag default-off until the task's rules permit promotion.
   - Add only the smallest counter needed to prove engagement.
   - Remove temporary high-frequency diagnostics before measured builds.

6. **Measure correctly.**
   - Search on `WORK-H` P50.
   - Gate on `WORK-H` P95.
   - Report spread and per-frame sign consistency.
   - `ALL` is only the VBlank-boundary confirmation.
   - Same-ROM repeat runs are deterministic; vary the build, not the run.

## ARM9 review checklist

Check:

- fixed-size structure copies that GCC lowers to helper calls;
- 2- or 4-byte `memcpy` accessors;
- repeated zeroing/copying of cold fields;
- pointer chasing and sparse indices in inner loops;
- large per-frame temporary structs;
- accidental volatile traffic;
- cache-line-sized data touched once per owner;
- hot loops reading ROM or uncached/shared regions;
- soft-float conversions and comparisons;
- division/modulo by non-constant values;
- function-call boundaries that block useful constant propagation;
- over-instrumented profile builds used as performance candidates.

Do not assume `inline`, `restrict`, `const`, `hot`, or section attributes help. Confirm the emitted code and A/B.

## Device classification

Cache locality, TCM residency, and placement are device-only class unless the custom accuracy emulator has an explicitly validated model for the exact mechanism. Queue an A/B pair for the batched device checkpoint; do not ask for one retail run per idea.

CPU work removal visible in typed buckets can be melonDS-sufficient when no device-divergent fallback exists.

## Correctness gates

- Use the smallest focused checker while editing.
- Run the widest relevant verifier once for a kept checkpoint.
- If imported/shared translation units change, follow the standing-rule regression policy.
- Preserve state, collision, RNG, camera meaning, and gameplay timing.
- A faster build with unexplained state differences is a failure.

## Required result

Report:

- symbol/call path and owner bucket;
- baseline instruction/data mechanism;
- emitted-code evidence;
- bytes/instructions/accesses removed or working-set change;
- `WORK-H` P50/P95 deltas and frame-sign distribution;
- code/data placement side effects;
- verifier result;
- melonDS-sufficient or queued device-only verdict;
- KEEP, REVERT, STOP, or WIP.
