---
name: smash64ds-perf-experiment
description: Plan and run one attributable Nintendo DS performance experiment in Smash64DS_Port. Use for FPS/tick optimization, profiling, comparing two implementations, testing an architectural cut, or deciding KEEP/REWORK/REVERT. Requires a measured exclusive bucket, ledger search, same-ROM comparison when practical, a short first falsifier, phase-aware P50/P95, and a compact evidence packet. Do not use for unmeasured cleanup or feature work.
---

# Objective

Reach a trustworthy decision with the fewest builds and emulator windows while
preserving BattleShip behavior, renderer contracts, and the live P1 workload.

# 1. Reconcile before editing

1. Read `AGENTS.md`.
2. Read the current P1 row and the narrow technical contract for the target.
3. Search `docs/PERF_LEDGER.md` and live source for the hypothesis, mechanism,
   symbol, and likely ancestor experiments.
4. Inspect `git status --short`, `git diff --stat`, and focused diffs.
5. Confirm lane ownership, build directory, target/profile/mode, runner slot, and
   current ROM/ELF identity.
6. Treat `decomp/` as read-only. Read BattleShip source before changing behavior,
   update order, display selection, material semantics, or gameplay arithmetic.

Do not duplicate a rejected experiment under a new name. State the material
cost-model difference before retrying an idea.

# 2. Define one falsifiable hypothesis

Before code, write:

- exact active phase and exclusive counter;
- current median/P95 and sample/window identity;
- maximum possible whole-frame saving if the bucket vanished;
- one causal hypothesis;
- one attributable runtime boundary;
- first falsifier;
- live KEEP / REWORK / REVERT thresholds;
- correctness/state/capture invariants;
- maximum permitted ROM, ITCM/DTCM, BSS, stack, reserve, VRAM, or audio impact.

Do not bundle independent changes. A structural owner cut may combine tightly
coupled mechanics only when they share one causal A/B boundary.

# 3. Capture a trustworthy baseline

Record:

- commit plus dirty-tree identity;
- effective compiler/linker flags;
- target, build directory, profile, runtime selector, and scene/match phase;
- ROM and ELF SHA-256;
- relevant map/section sizes;
- active ticks separately from VBlank wait;
- whole-frame and exclusive owner/sub-bucket P50/P95;
- conservation error where counters are intended to add up;
- semantic, state, cache, triangle, texture, upload, memory, and audio counters
  touched by the experiment;
- synchronized screenshot or other fidelity evidence when visible output can
  change.

Never compare figures from different builds, phases, instrumentation levels, or
workloads as an A/B decision.

# 4. Use the fastest valid experiment ladder

1. Host/static/disassembly smoke when applicable.
2. One incremental profile-1 build for the source revision.
3. Eight warm synchronized frames for crash/counter/falsifier evidence.
4. Stop immediately on visual/state/counter corruption or a missed structural
   threshold.
5. For a plausible candidate, run synchronized 32-frame A/B/A in the same ROM
   when practical, selecting only at a frame or complete-owner boundary.
6. Build profile 2 only after the candidate meets the live material-win gate.
7. Use 128-frame profile-1 and exact forensic distributions only for a KEEP
   decision or when the live plan explicitly requires them.
8. Build profile 0 and widen to project gates only during promotion.

Runtime selectors are laboratory tools. Do not branch on them inside pixel,
vertex, triangle, joint, sample, or other hot inner loops. Remove or compile them
out before an accepted checkpoint unless QA still requires the selector.

# 5. Inspect the right artifact

For CPU/code-generation hypotheses, inspect the actual object and ELF map before
editing. For memory/transfer hypotheses, measure bytes, changed bytes, lifetime,
and wait. For renderer hypotheses, prove which generic work disappears. For
audio, split decode/mix/copy/wait and preserve cadence.

Do not start with:

- global compiler-flag sweeps;
- speculative cache, TCM, DMA, allocator, or ARM7 moves;
- approximate gameplay math;
- another opcode/table executor that retains generic helpers;
- a partial render that wins by dropping output;
- instrumentation changes counted as production savings.

# 6. Decide and return evidence

Use `references/performance-evidence-packet.md`.

KEEP only when the intended exclusive bucket and relevant whole-frame/phase
metrics improve beyond noise and every fidelity/state/resource signal agrees.
REWORK only when one named residual wall can plausibly cross the live gate.
Otherwise REVERT the runtime candidate while preserving generally useful exact
fixtures or measurement infrastructure.

Workers return evidence to integration; they do not rewrite central truth docs.
