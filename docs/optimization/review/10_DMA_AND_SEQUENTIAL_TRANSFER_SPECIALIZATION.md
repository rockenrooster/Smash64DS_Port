# Campaign 10 — DMA + Sequential-Transfer Specialization

> **Planning baseline:** `codex/r2-runtime2` at `a63dd0e4b3af9c6601713f70c179c96c0aa67735` (2026-08-16).
>
> If HEAD has moved when implementation begins, re-run the inventory/measurement steps first and update symbol names/line references rather than blindly applying this document.
>
> **Campaign rule:** optimize toward a DS-native architecture and four-fighter headroom. The current two-fighter P95 gate is a checkpoint, not the architectural finish line. Never bank projected savings; measure the shipping configuration. Prefer same-binary route A/B when practical because this tree is placement-sensitive.

## Objective

Use DMA only where it removes substantial ARM9 memory traffic or improves large sequential transfers.

Priority:

1. **delete the copy**;
2. make mandatory transfers direct/sequential;
3. benchmark DMA for sufficiently large hardware-visible transfers;
4. leave tiny copies on CPU.

The Campaign 04 ~1,280 B/frame contract copy should be **deleted**, not DMA'd.

The rejected giant fighter FIFO packet must not return as a DMA project.

## Phase 0 — Transfer census

For every significant copy/fill/upload record:

- bytes/invocation;
- invocations/frame;
- source/destination;
- alignment;
- cache state;
- main RAM/VRAM/palette/OAM/GX destination;
- whether CPU consumes result immediately;
- overlap opportunity;
- whether ownership change can delete the copy.

Rank by bytes/frame and CPU ticks, not call count.

## Phase 1 — Delete transfers

Ask:

- can consumer use producer storage directly?
- can ownership move instead of bytes?
- can AOT data load into final storage?
- can a pointer/handle replace a struct clone?
- can staging buffers disappear?

Bank deletions first.

## Phase 2 — Specialize mandatory CPU transfers

For transfers below DMA break-even:

- aligned word/burst loops;
- contiguous layout;
- no per-element conversion;
- combine adjacent ranges where safe.

Campaign 08 should ensure texture bytes are already final format before transfer.

## Phase 3 — Measure DMA threshold on DS

Create a target microbenchmark under gameplay-like memory contention.

Measure CPU vs DMA including setup/sync for a range of sizes for:

- main RAM → VRAM;
- palette/OAM where applicable;
- supported large memory moves;
- GX FIFO stream if the API/hardware path permits.

Derive empirical thresholds; do not use folklore.

## Phase 4 — Candidate integrations

Likely candidates:

- large DS-native texture/palette uploads;
- large VRAM transfers;
- immutable/prebuilt sequential command/data streams from Campaign 11 **only if no per-frame packet copy is added**;
- preparation-time transfers before GO.

Prove source memory is DMA-accessible. Do not assume DTCM is a legal DMA source.

## Phase 5 — Schedule safely

For async DMA:

- define ownership until completion;
- perform required cache clean/invalidate;
- respect VRAM/GX ordering;
- avoid immediate waits when possible;
- instrument WAIT/stall separately.

A “CPU copy” win canceled by equal wait cost is not a win.

## Verification

- byte-identical destination;
- no race/tearing;
- cache coherence;
- correct GX/VRAM ordering;
- CPU and WAIT ticks separately;
- P50/P95;
- any banked gate claim reports the 2/3/4/5+ VBlank-interval histogram and max
  interval (AGENTS.md device-report law);
- transfer-size histogram;
- one-minute match and texture/VFX stress.

## Completion criteria

Every substantial repeated transfer is either deleted, specialized, or DMA'd because end-to-end measurement proves it. Tiny copies remain simple and no giant copied command packet is reintroduced.
