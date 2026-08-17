# Campaign briefs — index, sequencing, shared law

Fifteen campaign specifications distilled from the 2026-08-16 closure planning
pass and **audited 2026-08-16 against the linker, board, Makefile, and
artifacts** (corrections listed at the bottom). They are reference specs, not a
queue: `docs/P1_EXECUTION_BOARD.md` remains the **only** dynamic queue, and a
campaign is worked when the board carries its row. Every brief pins the same
planning baseline (`codex/r2-runtime2` @ `a63dd0e4b3a`) and the same rule:
re-derive measurements at execution time; never bank projections.

## Where the gate stands (STEP 0 IS DONE — measured 2026-08-17)

- **The shipping level is `+26,449` at rank-80** — rank-80 **1,171,776 raw /
  1,146,829 net**, band 41–120 1,175,961.6, P50 882,336, P90 1,070,848, top-1%
  1,455,232, 106/1600 over gate, WRAPFIX corrected 0 rows
  (`artifacts/performance/2026-08-17_shipping-rebank/SHIPPING_REBANK.md`,
  `build-c237-shipbank`, GX compose ON, `NDS_R2_BOTH_CPU=1`, DLDI ON, 1,600
  samples, frames 440–2039). **Size every candidate against +26,449.**
- **The old +19,089 is not a comparison, it is a different run.** c234 → c237 is
  +7,360 at rank-80, inside the ≥14,080 cross-build floor, and the frame-joined
  views disagree in sign (whole-match paired median −4,032, inside the ~5,700
  P50 floor). The two bases are not distinguishable; do not report a regression
  and do not arithmetically add the old GX A/B price.
- **Cadence does NOT clear its arm: 87.44% two-VBlank** —
  `2:1783 3:245 4:8 5+:3`, max interval 18 over 2,039 presented frames,
  `slips=0`. The previous basis read 87.79%; the 90.731% on record is the c185
  **DRAW=0** sibling and is a different instrument. The ≥95% arm (owner-decided,
  `plan.md`) remains open. Banked gate claims still report the histogram and max
  interval alongside P50/P95 (AGENTS.md).
- **The fresh per-PC census is on disk**:
  `artifacts/performance/2026-08-17_shipping-rebank/v4-c238/` (54,913,786 PC
  rows, shipping config), with the gate-mask reductions beside it. **Use it, do
  not take another** — `c200` (GX=1-era) and `v3-c221` (GX=0) are both retired.
  §7.7 of `SHIPPING_REBANK.md` lists what it contradicts in 01/05/07/11/12/13/14;
  the largest are `ndsRendererMtxMulAffine20p12` **18,549 → 384 tk/fr** (GX
  compose retired it; its 616 B of ITCM is now waste) and the card filesystem at
  **9.07× concentration** on the gate's own rank-80 frames.
- **Mask the census by the GATE's frames, not by the profile's own axis.**
  `total_cycles - halt_wait` includes the tick-HUD's console render, which
  `WORK-H` subtracts: under the profile's own marginal-80,
  `ndsPlatformRenderDebugHud` is the largest named owner at 8.8%; under the
  gate's own rank-80 frames it is 0.9%. `region = frame - 440` (derived
  empirically; the harness banner says 439).
- **The gap is still excursion, not level, and it grew**: the sixteen lane
  medians sum to 875,904 against a raw gate of 1,145,327, so **the median frame
  passes by 269,423** (218,767 on the c220 basis, 265,263 on c234). Size every
  candidate with the `convert.py`-style uniform-D re-rank **at rank-80** — it
  converts at ratio 1.000 from D=5,440 to D=100,000 — and never with a
  clip-to-median excess.
- Measurement law lives in `docs/VERIFYING.md` + the board's standing rules.
  Shorthand carried by all briefs: same-binary route A/B preferred; cross-build
  placement floor ≥14,080; same-binary paired-median floor ~5,440; the 2^22
  sampler correction (`WRAPFIX`) stays on; 2 profile cycles = 1 project tick.

## Landed since the briefs were written (2026-08-16 evening → 2026-08-17)

- **Campaign 01 was substantially EXECUTED** by the ITCM repack (−28,992
  banked): 4,108 B of the cold-inside-hot reserve source-split in small exact
  pieces, ~4,312 B of knapsack-ranked hot leaves admitted, `.itcm` now
  32,720 B with **16 B free**. Its brief now documents the residual only.
- **Campaign 05's core mechanism SHIPPED**: `NDS_R2_FIGHTER_GX_COMPOSE=1` is
  the published default (owner-accepted c185 bank; matrix-stack leak fixed).
  Its brief is now a finish-the-campaign spec, not a start-one.
- **Campaign 14's core LANDED**: battle prep + stepped animation preload now
  drain at the pre-BGM seam; a 1,600-frame run shows **0 post-start animation
  cache misses and 0 post-start payload reads** (`BUG_CLOSURE.md`). Residual:
  fail-closed counters beyond animation + RAM pricing.
- **Grab/back-throw spin bug fixed** (owner-confirmed): a port cache keyed
  `(root, heap generation)` survived a topology write; the fix invalidates at
  the `ftMainSetStatus` topology-writer seam — living precedent for Campaign
  04 Phase 1's epoch doctrine and a seam Campaign 03 must preserve.

## Suggested order (dependencies, not a queue)

**Step 0 — re-base (DONE 2026-08-17, `SHIPPING_REBANK.md`)**
- Level `+26,449`, cadence 87.44%, census `v4-c238`. Nothing below needs to
  re-measure the basis; read §7.7 before quoting any pre-2026-08-17 attribution.

**Foundation**
- **09** ARM/Thumb partitioning — Phase 1 (ISA ↔ TCM decoupling + shipping
  check) is the **sole owner** of that work item; 01 Phase 3 and 02 Phase 2
  only reference it. Still open; the repack worked within the old rules.

**Fully diagnosed, cheap, low-risk**
- **14 residual — PROMOTED, it is now the best-shaped item on the board.** The
  card filesystem is **9.07× concentrated on the gate's own rank-80 frames**:
  `get_fat` 1,424 → 12,907 tk/fr, `f_lseek` 886 → 8,073, `f_read` 239 → 2,928 =
  **23,908 tk/fr on exactly the 80 frames that set the gate, 90% of the whole
  requirement**. `ndsRelocGetFileData` is flat (conc 1.04) and genuinely quiet,
  so this is a *different* reader than the one the campaign closed. Fail-closed
  invariant first, then find that reader.
- **01/06 — ITCM has ≥2,178 B of measured eviction pool, not 16 B.**
  `ndsRendererMtxMulAffine20p12` fell **18,549 → 384 tk/fr** (52.94 → 1.19
  calls/frame) when GX compose shipped, and still rents 616 B at 1,993.8
  cyc/byte; `ndsRendererLoadHardwareSplitMatrices` 392 B at 170.3; and the
  census names **14 never-executed residents, 1,170 B idle**. The repack's own
  named runner-up missed by 28 B. Re-run the knapsack on `v4-c238`.
- **04** Draw-memo completion — Phase 2's 1,280 B/frame copy is now **sized at
  ~500–1,500 tk/fr and is BELOW every floor this instrument has**; it must ride
  with a larger slice (`SHIPPING_REBANK.md` §6). What Phase 2 should take first
  is new: the grab-fix seam blows the memo **313 times a match**
  (hit rate 96.19% → 88.20%), and those frames are 3.5–4.0× concentrated on the
  gate's own rank-80 frames (§4).
- **15** Dream Land collision tables — extends the proven slices 35–37
  playbook (−10,752 banked, bit-identical by construction).

**Big rocks (each may span many cycles; keep slices small and banked)**
- **03** Compact AOT animation representation — attacks the attach-driven
  re-parse that owns the `SITR` excursion (72,768 ceiling on the c220 basis).
  Carries the owner-closed transition-play constraint: the transition-frame
  play is the sole writer of the new status's first pose. **Do not re-propose
  skipping it.**
- **12** Simulation Q chains — re-measured: the reservoir is **147,180 tk/fr**
  caller-attributed on the gate-80 mask, of which the strictly simulation
  subsystems are **87,085** (`SIMSIDE.md`'s 83,204, agreeing to 4.5%). The size
  held; the **ratio did not** — "1.511× the requirement" was against +94,481, and
  against +26,449 it is **5.57×**. Collision bodies carry a renderer coupling
  (`parts->mtx_translate`) — read the brief's caution before touching them.
- **13** Draw-side fixed point — measured: renderer + particles **11,859 tk/fr**
  on the gate-80 mask, plus 16,447 of matrices/transform shared with the camera.
  Always label the mask. Coordinate with 05's one live seam
  (`ndsRendererAdapterBuildDObjLocalMatrix`, now **4,829 whole / 5,142 gate-80,
  conc 1.06**).
- **05 residual** — joint-consumer classification, AOT folding, fixed-native
  local matrices into the now-shipping GX compose (`Roots 62,952`, `Declines 0`,
  `LoadHardwareGxComposedMatrices` 13,319 / 14,082, conc 1.06); feeds 11.
- **11** AOT-native renderer (**Task 51**, `NDS_BATTLE_PROFILE=0` seam) — the
  four-fighter-headroom structural change; consumes 04's memo seam and 05's
  ownership data. Cache fill still beats arithmetic on the shipping census
  (`icache_fill` 32.0% + `dcache_fill` 25.9% = 57.9% on the gate's own frames,
  `issue` 28.8%), so the win is still deleting resident walk/decode code — but
  **`FTR` is no longer flat**: conc 1.05 with 12,928 of excess at rank-80, and
  the per-phase split in the brief predates GX compose (§7.7).

**Continuous / dividend consumers**
- **06** Soft-float elimination — **urgency went UP**: ITCM has 16 B free and
  the repack's knapsack already names the first runner-up tenant that missed
  by 28 B, so every input-section 06 frees has an immediate measured consumer.
  Helper members free only at **input-section granularity**.
- **07** Division/reciprocal — pairs with 03/12 slices; `__aeabi_fdiv` was the
  most expensive helper in the build (10,084 tk/fr, pre-repack attribution).
- **08** Native texture formats, **10** DMA/transfer specialization —
  opportunistic; 10's first rule is *delete the copy* (04 owns the 1,280 B
  copy; do not DMA it).
- **01 residual + 02** — 01 is reduced to consuming 06's dividend, the 5,376 B
  structural renderer decomposition (hard; the broad c225 split regressed and
  was rejected), and census freshness. 02 (DTCM) is untouched by the repack
  and stands as written.

## Cross-campaign ownership (single writers)

| Item | Owner | Everyone else |
|---|---|---|
| ISA ↔ TCM decoupling + `*.32.o` shipping check | 09 Phase 1 | 01/02 reference it |
| 1,280 B/frame contract copy | 04 Phase 2 (delete) | 10 must not DMA it |
| Memo → renderer seam | 04 Phase 6 defines, 11 consumes | — |
| Fighter hierarchy ownership data | 05 generates, 11 owns at end state | — |
| Transition-frame animation play | **CLOSED — REJECTED by owner** (`VERDICT.md`) | binds 03, 12, and any attach-lane idea |
| Giant copied FIFO packet (+124K) | forbidden everywhere | 11 kill-condition, 10 explicit |
| `.text.hot` / `.text.hot.draw` membership | closed both directions (linker) | 01 inventories, never edits |
| Collision numerics | frozen — exact moves only | 15 structural only; numeric change = 12-proven chain |
| Helper-family removal + ITCM byte reclamation | 06 | 12/13/03 report last-caller closures to it |

## Owner inputs already encoded

- `PROJECT_GOAL.md`: "RAM resources may HAVE to be reclaimed or shuffled" —
  authorizes the 01/02 class of work.
- `docs/OPTIMIZE_LIST.md` billboard observation (2026-08-06) — read before any
  effect/texture work (08, 13 Phase 5); confirm against BattleShip.
- `docs/OPTIMIZE_LIST.md` "30hz Animations" — a sacrifice-order trade needing
  its own owner decision; 03 stays exact-60 Hz and must not preclude it.

## Status update applied 2026-08-17

After commits `3b8273d1470` (ITCM repack bank), `a507a0e6e25` (GX compose
ships ON), `14977e0ab8c` (grab-transform + presentation fixes): README
re-based to the +19,089 GX=0 level with the shipping level marked unmeasured;
01/05/14 re-scoped to residuals; 12/13/07/11 census pointers corrected from
`v3-c221` (now config-mismatched) to a fresh shipping census; 04 gained the
grab-cache precedent; 03 gained the `ftMainSetStatus` invalidation-seam
requirement; 06 gained the named runner-up ITCM tenant.

## Audit corrections applied 2026-08-16

- **04**: hit counters corrected to **3,765 / 3,914** (the earlier 3,493/3,623
  pair matched no artifact — 3,493 is an unrelated `NativeEligible` figure from
  archived 2026-07-28 docs); evidence labeled as the `NDS_R2_BOTH_CPU=1` stress
  arm; false-miss ROI (≈454 tk/fr) and `DRAW_MEMO.md`'s "not obviously the
  right trade" warning carried into Phase 4; display-list-swap hole noted.
- **03**: unrecorded joint-delta figures replaced with the `VERDICT.md`
  mechanism and the owner's verbatim rejection; `ftMainRunUpdateColAnim`
  correction carried; 30 Hz reconciliation added.
- **13**: "22.5K" sourced to the board's ≈22,521 with its staleness warning;
  Phase 0 now targets the existing `v3-c221` census (no build).
- **06**: exact stock-member table (1,952 B) and the input-section granularity
  constraint added (456 B dead welded to 228 B live; no partial trims).
- **15**: −10,752 cited to board slices 35–37; `ndsMPFindLineEndpoints` figure
  pinned to its c117 profile source (5,861 tk/fr).
- All briefs: cadence-histogram reporting line added to verification.
