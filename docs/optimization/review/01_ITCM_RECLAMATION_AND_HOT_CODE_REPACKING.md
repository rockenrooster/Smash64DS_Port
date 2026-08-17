# Campaign 01 — ITCM Reclamation + Hot-Code Repacking

> **Planning baseline:** `codex/r2-runtime2` at `a63dd0e4b3af9c6601713f70c179c96c0aa67735` (2026-08-16).
>
> If HEAD has moved when implementation begins, re-run the inventory/measurement steps first and update symbol names/line references rather than blindly applying this document.
>
> **Campaign rule:** optimize toward a DS-native architecture and four-fighter headroom. The current two-fighter P95 gate is a checkpoint, not the architectural finish line. Never bank projected savings; measure the shipping configuration. Prefer same-binary route A/B when practical because this tree is placement-sensitive.
>
> **Basis (2026-08-17):** the shipping level is **+26,449** at rank-80 and the fresh per-PC census is `artifacts/performance/2026-08-17_shipping-rebank/v4-c238`. `c200` and `v3-c221` are retired. **Read `SHIPPING_REBANK.md` §7.7 before quoting any figure in this brief** — it lists what the new census contradicts, and mask the census by the GATE's own rank-80 frames.

## THE RE-KNAPSACK RAN AND LANDED 2026-08-17 — −44,544 at rank-80

`artifacts/performance/2026-08-17_itcm-repack2/ITCM_REPACK2.md`,
`build-c239-itcm-repack2`. **REQUIREMENT +26,449 → −18,095**; rank-80
1,171,776 → 1,127,232; band 41–120 1,175,961.6 → 1,130,086.4; P50 882,336 →
841,024; over gate 106 → 88 of 1,600; cadence 87.44% → **89.06%** (still short of
the ≥95% arm). Paired on presented frame the median is **−41,344** and the
candidate wins **1,521/1,600**. Every retained counter bit-identical; Boundary
green.

**The pool this brief predicted at ≥2,178 B was really 13,188 B, and it was not
where the census pointed.** Corrections, from the linker map at input-section
granularity:

- The census's **"14 never-executed residents, 1,170 B idle"** is mostly
  unusable and it **double-counts**: `__addsf3` 444 + `__aeabi_frsub` 456 +
  `__nds_task16_libgcc_fsub_golden` 448 all name the **same 456 B** at the head
  of `_arm_addsubsf3.itcm.o`, one 684 B input section whose other 228 B is live;
  `__aeabi_ul2f` 188 B sits inside that *live* half; 80 B are exception vectors
  and 52 B cache maintenance, both correctly resident;
  `threadUnblockAllByValue` 312 B is its own section but arrives through the
  `*.32.o` linker rule, so taking it is a linker-script change.
- The real pool was the **generic display-list renderer** — the fallback the
  native stage/fighter owner paths replaced — priced on the **gate's own rank-80
  frames** at 0.0–1.1 tk/byte against a resident table running 20–200:
  `ndsRendererScanList` 5,972 B / 599 tk/fr, `ndsRendererSubmitHardwareTriangle`
  3,204 / 291, `ndsRendererHardwareSubmitVertex` 2,276 / 775,
  `ndsRendererMtxMulAffine20p12` 616 / 676,
  `ndsRendererHardwareLitShadeColorPrepared` 460 / 61,
  `ndsRendererLoadHardwareSplitMatrices` 368 / 8, `…HardwarePolyFmt` 160 / 8,
  `…HardwareTextureSourceBytes` 132 / 5. **They are still emitted and callable;
  only residency changed.**
- **`nds_renderer.o` contributes ONE 16,948-byte `.itcm` input section** — every
  function carrying `section(".itcm")` in that TU merges into it — so eviction
  here is a source attribute, never a linker rule.

**23 admissions, 14,350 B, static ceiling 80,745 gate-80 I-cache-fill tk/fr,
realised 44,544 = 55% of ceiling** (the 2026-08-16 pack realised 46%; the
ceiling is a ranking, never a prediction). `ndsRendererHardwareApplyTextureParams`
— the runner-up that missed by 28 B — is in, as is
`ndsRendererNativeApplyProductionPreamble`, which the GX-compose default flip had
evicted for want of 32 B. `.itcm` is now **32,648 B with 88 free**; `.text.hot`
is **unchanged at 3,984 B**; `.text.hot.draw` grew 4,332 → 4,924 because
`nds_task32_draw_hot.inc` already carried
`*nds_renderer.o(.text.ndsRendererMtxMulAffine20p12)`, a pattern that matched
nothing while that symbol was ITCM-resident.

**What is left of this campaign, in order of measured value:**

1. **Archive members are now the densest unplaced candidates and need a
   build-system change.** `get_fat.isra.0` 352 B / **8,868** gate-80 icf
   (25.2/B), `f_lseek` 664 / 6,366, `tickGetCount` 92 / 3,739, `cpuGetTiming`
   24 / **1,938 (80.8/B)**, `mutexUnlock` 188 / 2,883, `glBindTexture` 92 /
   1,524. `linker/nds_hot_text.ld:113` matches `*.itcm.*` by **filename**, so
   each means extracting a member and renaming it — and the member arrives
   whole, which is why the last two packs excluded them. Only 88 B are free, so
   this needs its own eviction first.
2. The 5,376 B structural renderer decomposition, unchanged and still hard.
3. Census freshness.

## Status — the core of this campaign LANDED 2026-08-16

`artifacts/performance/2026-08-16_itcm-repack/ITCM_REPACK.md` executed Phases
1, 2, 4 and 5 and banked **−28,992 tk/fr at rank-80** (2.06× the cross-build
floor; +48,081 → +19,089 at GX=0):

- **4,108 B** of the 9,484 B cold-inside-hot reserve reclaimed as small exact
  source splits (dead/cold residents, production-run specialization,
  `ndsRendererScanList` branch split, zero-PC arms, UV-miss helper);
- **~4,312 B** of knapsack-ranked port-owned hot leaves admitted (list in the
  artifact; includes `ndsRendererMtxMulAffine20p12`, `ftGetStruct`, `sqrtf`,
  `gcPlayDObjAnimJoint`, `ndsRendererAdapterBuildDObjLocalMatrix`, …);
- final `.itcm = 32,720 B`, **16 B free**;
- the broad one-shot split (`c225`) **regressed and was rejected** — the
  remaining **5,376 B** of cold-inside-hot is compiler-expanded code
  interleaved with executed paths and is **not** a free eviction pool; further
  recovery needs a new structural renderer decomposition, not more `cold`
  attributes;
- `gcPlayDObjAnimJoint` moved to ITCM via a section attribute in the import
  shim (`battleship_sys_objanim.c:596`), so `.text.hot`'s input-section
  pattern for it now matches nothing — the linker's Task 94/E66 comment block
  is historical context, superseded by this measured whole-config win.

## Remaining objective

What is left of this campaign: (a) consume Campaign 06's float-helper dividend
— the repack's own knapsack names the first runner-up tenant
(`ndsRendererHardwareApplyTextureParams`, 180 B / 1,309.5 tk/fr, missed by
28 B) and the ranked candidate list is in the artifact; (b) the 5,376 B
structural decomposition, only if a real decomposition design appears; (c)
keep the ITCM census fresh so future admissions rank on current truth, on the
GX-compose-ON shipping config.

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
- The 688 B + 54 B zero-instruction reclamation route was **consumed** by the
  repack's first step (`c226`, 688 B moved to main). Do not re-derive it.
- Current animation hot kernel `ndsR2AnimValueQ` is already an ITCM resident and has been same-binary tested. Do not evict it without new evidence.
- Campaign 06 owns the software-float/helper ITCM dividend (stock member total
  1,952 B plus Task16/r2 bodies; exact occupancy comes from its Phase 0, and
  members free only at input-section granularity). With 16 B free, that
  dividend is now the **only** meaningful source of admission capacity.
- Refill calibration history, so neither number is misused: `FTR_LANE.md`
  sized the best tenants for the then-908 B pool at ~3,000–4,000 tk/fr; the
  repack then reclaimed 4,108 B first and its ~4.3 KB admission measured
  **−28,992** whole-config. Lesson: the pool size was the binding constraint,
  not the tenant quality — and static I-cache ceilings (the pack's ~62.9K)
  rank tenants but never bank; the measured conversion was 46% of ceiling.
- The instrument must fit too: a candidate can fit the ROM that SHIPS and
  still be unpriceable because it does not fit the tick-HUD ROM that MEASURES
  (`CAMERA_SHIP.md` lesson). Post-repack the measurement binary is at **16 B
  free**, so ANY new admission first needs freed bytes there.

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
