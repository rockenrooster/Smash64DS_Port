# Campaign 01 — ITCM Reclamation + Hot-Code Repacking

> **Planning baseline:** `codex/r2-runtime2` at `a63dd0e4b3af9c6601713f70c179c96c0aa67735` (2026-08-16).
>
> If HEAD has moved when implementation begins, re-run the inventory/measurement steps first and update symbol names/line references rather than blindly applying this document.
>
> **Campaign rule:** optimize toward a DS-native architecture and four-fighter headroom. The current two-fighter P95 gate is a checkpoint, not the architectural finish line. Never bank projected savings; measure the shipping configuration. Prefer same-binary route A/B when practical because this tree is placement-sensitive.

## Objective

Recover ITCM bytes occupied by code that is dead, diagnostic-only, or mostly cold, then spend the recovered bytes on the highest-value hot code still executing from `.main`.

This is **not** a generic linker reshuffle. The current linker records that the curated 8 KiB `.text.hot` working set is **closed in both directions** after two placement experiments changed neighboring addresses and regressed performance. Do not reorder, add to, or remove from `.text.hot` in this campaign. Note there are **two** curated main-RAM hot-text regions, each with its own ≤8 KiB assert: `.text.hot` (hand-curated update set, `linker/nds_hot_text.ld:179-223`) and `.text.hot.draw` (populated by the generated `nds_task32_draw_hot.inc`, empty on control ROMs, `:227-232`). They are cache-curated main-RAM sections, not ITCM; the inventory must report them separately from ITCM proper and treat both as closed.

The campaign succeeds when ITCM contains a deliberately selected shipping working set, cold tails no longer consume scarce TCM, and every promoted function has a measured net benefit.

## Current repo anchors

- `linker/nds_hot_text.ld`
- `scripts/census-icache-placement.py`
- `scripts/census-icache-temporal.py`
- `scripts/census-fetch-density.py`
- `scripts/placement-layout-model.py`
- `scripts/check-renderer-itcm-placement.ps1`
- `scripts/check-task9-float-itcm.ps1`
- `scripts/compare-elf-sections.py`
- `artifacts/performance/2026-08-16_itcm-census/ITCM_CENSUS.md` — every resident
  censused: 32,180 walked bytes, **14,242 B (44.3%) never execute** in the
  1,600-frame window — but only 2,448 B is wholly-dead blocks; **11,010 B is
  cold *inside* 52 live blocks**, reachable only by source splits (Phase 2),
  never by evicting a symbol.
- `artifacts/performance/2026-08-16_itcm-frsub/ITCM_FRSUB.md` — symbol-level
  "recoverable" figures over-count: the 456 B frsub blob is dead but welded
  into a 684 B input section whose other half is live (a linker cannot split an
  input section). The census's "+1,858 B recoverable by eviction" over-counts
  by 456.
- Known cheap reclamation with its premise already closed (board, `v3-c221`
  shipping census): four port-side production emitters + one invalidate helper
  execute **zero instructions in the shipping configuration** — 688 B + 54 B,
  one-line `NDS_TASK82_ITCM_CODE` removals, none pinned by
  `check-renderer-itcm-placement.ps1`. Still needs its own build + Boundary.
- Current animation hot kernel `ndsR2AnimValueQ` is already an ITCM resident and has been same-binary tested. Do not evict it without new evidence.
- Campaign 06 owns the software-float/helper ITCM dividend (stock member total
  1,952 B plus Task16/r2 bodies; exact occupancy comes from its Phase 0, and
  members free only at input-section granularity). Treat it as a future
  dividend, not space already available.
- Calibrate refill expectations: `FTR_LANE.md` sized the best available ITCM
  tenants at **~3,000–4,000 tk/fr total** for the 908 B pool, using the
  fetch-free bound `cycles − (instructions + dcache + wb + bus + interlock)` —
  NOT the raw `icache_fill` figure. ITCM refill is a rider, not a gate-closer.
- The instrument must fit too: a candidate can fit the ROM that SHIPS and still
  be unpriceable because it does not fit the tick-HUD ROM that MEASURES
  (~220 B free there vs ~2.6 KB on the Boundary ROM; `CAMERA_SHIP.md`).
- `linker/nds_hot_text.ld:180-200` records the Task 94 / E66 prohibition on
  `gcPlayDObjAnimJoint` moves; the board marks it re-testable **by the
  same-binary route method only** (the original verdict was a 128-frame
  cross-build pair). Do not re-open it any other way.

## Hard constraints

1. Do not perturb the existing `.text.hot` member list or order.
2. Do not use instruction count, cycles/instruction, or estimated recoverable stalls as proof of a placement win. This repo has already measured the wrong sign from multiple estimators.
3. Do not leave duplicate hot implementations in the shipping binary after qualification.
4. Do not move profiling/diagnostic code into the shipping resident set.
5. Preserve ARM/Thumb interworking and interrupt safety.
6. Re-run map/disassembly checks after every linker change.

## Phase 0 — Freeze a shipping ITCM census

Create a machine-readable report from the shipping ELF containing **every byte in ITCM**, grouped by input section/object/symbol.

For each resident record:

- symbol and object;
- start/end/size;
- ARM or Thumb;
- calls/executions per gameplay frame;
- active-frame count;
- marginal-80 activity if available;
- caller set;
- whether it is lab/diagnostic only;
- whether it contains cold error/assert/fallback tails;
- literal-pool/rodata bytes pulled in with it;
- whether it landed in ITCM only because it is a `*.32.o`.

Also report total ITCM used/free and largest contiguous free range.

**Deliverable:** an ITCM inventory Markdown plus JSON/CSV.

## Phase 1 — Reclaim unquestionably dead/non-shipping residents

Start with zero-risk reclamation:

1. Compare shipping ELF against tick-HUD/lab ELFs.
2. Find resident code whose only callers are compiled-out diagnostics, probes, obsolete routes, or dead fallbacks.
3. Prove link reachability; do not infer deadness from one runtime trace.
4. Remove the placement attribute/linker rule or eliminate the dead shipping object.
5. Verify the shipping ELF no longer contains it.
6. Run boundary and one-minute correctness verification.

Do **not** refill the bytes in the same first experiment. Establish a clean reclaimed-space baseline first.

## Phase 2 — Split cold tails out of hot residents

Use per-PC and branch-frequency evidence to find large resident functions containing:

- failure/error handling;
- rare format fallbacks;
- diagnostics/asserts;
- cold switch arms;
- one-time setup embedded in per-frame code.

For one candidate at a time:

1. Extract the cold path into `noinline` cold code in `.main`.
2. Keep the common path straight-line.
3. Avoid adding a call/veneer on the common path.
4. Confirm the linked hot body actually shrank.
5. Verify the extracted tail is rare across a full match.
6. Measure the resulting shipping layout.

Reject a split if the hot-path branch/call cost is larger than the TCM benefit.

## Phase 3 — Decouple ARM mode from automatic TCM residency

**Campaign 09 Phase 1 owns this work item** (including the shipping check for
accidental `*.32.o` placement). This campaign consumes the result — do not
implement it twice. Sequence: 09 Phase 1 lands before this campaign's Phase 4
ranking, so refill candidates are ranked on a map where ISA no longer implies
residency.

## Phase 4 — Rank `.main` refill candidates

Re-run the I-cache placement/temporal census against the post-reclamation shipping binary.

Exclude:

- the closed `.text.hot` set;
- cold/rare functions;
- callers whose apparent cost is mostly child cost;
- functions too large to fit sensibly;
- candidates with destructive temporal overlap unless tested as a combination.

For each shortlist candidate record size including literals, execution density, marginal-80 participation, temporal overlap, veneer/interworking changes, and exact displaced bytes.

The ranking is a shortlist only.

## Phase 5 — Qualify promotions experimentally

Prefer a **one-binary dual-copy route**:

- keep byte-identical `.main` and ITCM copies temporarily;
- choose target through one route word before measurement;
- confirm identical call counts and outputs/state;
- compare paired frames;
- delete the losing copy afterward.

If same-binary routing is impossible, use tightly matched builds and explicitly respect known placement noise.

Promote one function at a time. Only after individual winners are known should combined packing be measured, because ITCM interactions are non-additive.

## Phase 6 — Spend Campaign 06's float dividend last

When Campaign 06 proves software-float/helper routines unreachable:

1. remove their ITCM sections;
2. measure the empty-space baseline;
3. re-run candidate ranking on the new map;
4. refill only with newly measured winners.

Do not assume a candidate ranked before the helper removal remains best after addresses move.

## Verification

For every banked change:

- post-link ITCM occupancy check;
- `verify-boundary.ps1`;
- one-minute Mario/Fox match;
- state/render checks for touched subsystem;
- corrected WORK-H P50/P90/P95/rank-80/top-1%;
- FTR/SRC/STG/MISC/OTHR movement;
- ITCM used/free bytes;
- any banked gate claim reports the 2/3/4/5+ VBlank-interval histogram and max
  interval (AGENTS.md device-report law);
- no root-ROM contamination by a lab build.

## Keep / kill gates

**KEEP** only when the resident change is repeatably faster or enables a larger measured packing win with no correctness loss.

**KILL** when:

- gain is inside noise but consumes meaningful ITCM;
- neighboring owners regress because of address movement;
- veneers/calls erase the gain;
- `.text.hot` ordering changes;
- the “cold” tail is actually common.

## Completion criteria

1. Every shipping ITCM resident has an explicit reason to be there.
2. Dead/diagnostic residents are gone.
3. Profitable cold-tail splits are complete.
4. Campaign 06's unreachable float code is reclaimed.
5. Recovered bytes are refilled only with measured winners.
6. A final map documents exact ownership/free space.
7. The combined shipping layout wins, not merely a microbenchmark.
