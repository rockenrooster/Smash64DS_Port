# Campaign briefs — index, sequencing, shared law

Fifteen campaign specifications distilled from the 2026-08-16 closure planning
pass and **audited 2026-08-16 against the linker, board, Makefile, and
artifacts** (corrections listed at the bottom). They are reference specs, not a
queue: `docs/P1_EXECUTION_BOARD.md` remains the **only** dynamic queue, and a
campaign is worked when the board carries its row. Every brief pins the same
planning baseline (`codex/r2-runtime2` @ `a63dd0e4b3a`) and the same rule:
re-derive measurements at execution time; never bank projections.

## Where the gate stands (updated 2026-08-17 — re-derive from the board)

- **Last measured level: +19,089 net at rank-80, at GX compose OFF** — the
  ITCM repack banked **−28,992** on top of the c223 memo basis (rank-80
  1,164,416 raw / 1,139,469 net, P50 885,440, 95/1600 over gate;
  `artifacts/performance/2026-08-16_itcm-repack/ITCM_REPACK.md`).
- **The shipping configuration then changed and its level is UNMEASURED**:
  owner policy 2026-08-16 promotes accepted optimisations to shipping
  defaults, so `NDS_R2_FIGHTER_GX_COMPOSE ?= 1` now ships (the accepted c185
  bank). The board's own warning: re-bank before quoting a new gap; do **not**
  arithmetically add the old GX A/B price.
- **Step 0 for any campaign work now: re-bank the shipping basis and take a
  fresh per-PC census on it.** Every existing census (`c200` GX=1-era,
  `v3-c221` GX=0) mismatches the current shipping config — briefs 05/07/11/12/13
  size against attributions that predate GX compose ON and the ITCM repack.
- **The gap was excursion, not level** (median frame passed by ~219K on the
  c220 basis; board `SETUP_SHARE` entry). Re-verify that shape on the new
  basis, then size every candidate with the `convert.py`-style uniform-D
  re-rank **at rank-80**; means and medians mislead here.
- The gate's second arm is **cadence ≥95% two-VBlank** (owner-decided,
  `plan.md`) and it is still open: the c185 GX-compose DRAW=0 sibling read
  90.731%. Banked gate claims report the 2/3/4/5+ VBlank-interval histogram
  and max interval alongside P50/P95 (AGENTS.md).
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

**Step 0 — re-base (blocks all sizing)**
- Re-bank the shipping level (GX compose ON) with a whole-match run + cadence
  histogram, and take a fresh per-PC census on that config. Cheap, no design
  risk, and every campaign below sizes against it.

**Foundation**
- **09** ARM/Thumb partitioning — Phase 1 (ISA ↔ TCM decoupling + shipping
  check) is the **sole owner** of that work item; 01 Phase 3 and 02 Phase 2
  only reference it. Still open; the repack worked within the old rules.

**Fully diagnosed, cheap, low-risk**
- **04** Draw-memo completion — Phase 2 (delete the 1,280 B/frame copy) is
  sketched in `DRAW_MEMO.md` §7; part of a known 4,901 tk/fr overhead.
- **15** Dream Land collision tables — extends the proven slices 35–37
  playbook (−10,752 banked, bit-identical by construction).
- **14 residual** — make the GO/pre-BGM invariant fail-closed and extend it
  past animation (texture conversion, reloc normalization).

**Big rocks (each may span many cycles; keep slices small and banked)**
- **03** Compact AOT animation representation — attacks the attach-driven
  re-parse that owns the `SITR` excursion (72,768 ceiling on the c220 basis).
  Carries the owner-closed transition-play constraint: the transition-frame
  play is the sole writer of the new status's first pose. **Do not re-propose
  skipping it.**
- **12** Simulation Q chains — 142,786 reservoir (re-attribute on the new
  shipping census first); warm-MAC subset 71,491 in 14 functions; collision
  bodies carry a renderer coupling (`parts->mtx_translate`) — read the brief's
  caution before touching them.
- **13** Draw-side fixed point — ≈22,521 board estimate, stale twice over;
  re-derive from the new shipping census. Coordinate with 05's one live seam
  (`ndsRendererAdapterBuildDObjLocalMatrix` f32 boundary).
- **05 residual** — joint-consumer classification, AOT folding, fixed-native
  local matrices into the now-shipping GX compose; feeds 11.
- **11** AOT-native renderer (**Task 51**, `NDS_BATTLE_PROFILE=0` seam) — the
  four-fighter-headroom structural change; consumes 04's memo seam and 05's
  ownership data. The GX=0-era attribution said `FTR` was 59.3% cache fill:
  the win is deleting resident walk/decode code, not arithmetic — re-derive
  the split on the new census.

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
