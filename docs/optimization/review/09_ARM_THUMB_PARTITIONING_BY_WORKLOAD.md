# Campaign 09 — ARM / Thumb Partitioning by Workload

> **Planning baseline:** `codex/r2-runtime2` at `a63dd0e4b3af9c6601713f70c179c96c0aa67735` (2026-08-16).
>
> If HEAD has moved when implementation begins, re-run the inventory/measurement steps first and update symbol names/line references rather than blindly applying this document.
>
> **Campaign rule:** optimize toward a DS-native architecture and four-fighter headroom. The current two-fighter P95 gate is a checkpoint, not the architectural finish line. Never bank projected savings; measure the shipping configuration. Prefer same-binary route A/B when practical because this tree is placement-sensitive.
>
> **Basis (2026-08-17):** the shipping level is **+26,449** at rank-80 and the fresh per-PC census is `artifacts/performance/2026-08-17_shipping-rebank/v4-c238`. `c200` and `v3-c221` are retired. **Read `SHIPPING_REBANK.md` §7.7 before quoting any figure in this brief** — it lists what the new census contradicts, and mask the census by the GATE's own rank-80 frames.

## Objective

Choose ARM or Thumb **per workload**, never by blanket mode switch.

Use ARM where hot kernels materially benefit from instructions/registers such as `SMULL`, `UMULL`, `SMLAL`, or fewer spills/instructions. Use Thumb for cold control/error/rare paths where code density matters more.

## Critical current linker issue

`linker/nds_hot_text.ld` treats `*.32.o` specially:

- text/rodata enters ITCM;
- mutable data/BSS can enter DTCM.

Thus compiling a whole TU as ARM can silently consume both TCMs. Decouple ISA choice from TCM placement before broad partitioning.

## Current repo anchors

- `linker/nds_hot_text.ld`
- `scripts/task81_partition_census.py`
- `scripts/census-fetch-density.py`
- `scripts/census-icache-temporal.py`
- `scripts/check_nds_native_stage_arm.ps1`
- `scripts/check-r2-hwmath-arm-probe.c`
- `scripts/check-r2-hwmath.ps1`

## Phase 0 — ISA/placement census

For every hot function record:

- ARM/Thumb;
- code size;
- object/TU;
- ITCM vs `.main`;
- calls/frame;
- instruction count;
- long multiply/MAC usage;
- spills/reloads;
- veneer/interworking calls;
- literal bytes;
- DTCM globals dragged in by object.

Identify large cold ARM objects containing only one small ARM-worthy kernel.

## Phase 1 — Decouple ISA from TCM

**This campaign is the sole owner of this work item.** Campaigns 01 Phase 3 and
02 Phase 2 reference it and must not implement their own variant; the shipping
check below is specified once, here.

Refactor build/link rules so:

- ARM may live in `.main`;
- only explicit sections enter ITCM;
- ARM compilation does not imply DTCM;
- small ARM kernels may use dedicated TU/per-function section.

Add checks for accidental TCM contributions (report/block any `*.32.o` text,
rodata, data, or BSS landing in a TCM without an explicit section), and prove
the refactor itself is placement-neutral: the shipping map before/after must
show identical TCM occupancy, because this tree is placement-sensitive and the
decoupling must not silently move residents.

## Phase 2 — Split mixed TUs

When one multiply-heavy kernel shares a TU with control helpers:

- isolate kernel;
- compile kernel ARM;
- compile cold control/error helpers Thumb;
- keep ABI boundary narrow;
- avoid repeated interworking inside inner loops.

## Phase 3 — ARM criteria

Use ARM when disassembly/profile proves:

- frequent 32×32→64 multiply;
- MAC chains;
- Thumb register pressure/spills;
- materially lower instruction count;
- loop benefit large enough to offset code size.

“Hot” alone is not enough.

## Phase 4 — Thumb criteria

Use Thumb for:

- low-frequency paths;
- branch/control dominated helpers;
- error/fallback/setup;
- code with little ARM arithmetic benefit;
- code where I-cache/RAM density matters more.

## Phase 5 — Controlled A/B

For hot kernels, route ARM/Thumb alternatives through a selector chosen outside the hot loop when possible.

Measure:

- self/caller ticks;
- spills/load-store;
- I-cache;
- code size;
- ITCM/DTCM use;
- veneer count;
- whole-frame effect.

Reject local wins that cause worse overall placement.

## Integration

- Campaign 03: animation MAC/cubic kernels.
- Campaign 12: simulation Q MAC kernels.
- Campaign 13: matrix/normalization.
- Campaign 01: only measured highest-value code gets ITCM.
- Campaign 02: no ISA-driven DTCM.

## Verification

- disassembly proves intended ISA;
- map proves placement;
- no accidental TCM occupants;
- state/render parity;
- one-minute match;
- kernel and whole-frame timing;
- any banked gate claim reports the 2/3/4/5+ VBlank-interval histogram and max
  interval (AGENTS.md device-report law).

## Completion criteria

Shipping has an explicit workload-based ISA partition, ARM/Thumb is decoupled from TCM placement, arithmetic kernels use ARM only when measured beneficial, and cold paths are compact without correctness loss.
