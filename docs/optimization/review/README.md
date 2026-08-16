# Campaign briefs — index, sequencing, shared law

Fifteen campaign specifications distilled from the 2026-08-16 closure planning
pass and **audited 2026-08-16 against the linker, board, Makefile, and
artifacts** (corrections listed at the bottom). They are reference specs, not a
queue: `docs/P1_EXECUTION_BOARD.md` remains the **only** dynamic queue, and a
campaign is worked when the board carries its row. Every brief pins the same
planning baseline (`codex/r2-runtime2` @ `a63dd0e4b3a`) and the same rule:
re-derive measurements at execution time; never bank projections.

## Where the gate stands (at audit time — re-derive from the board)

- Basis `build-c223-ftrmemo` route 1 (shipping config): rank-80 **1,193,408
  raw / 1,168,461 net** against the **1,120,380** gate → **REQUIREMENT
  +48,081** (`artifacts/performance/2026-08-16_ftr-draw-memo/DRAW_MEMO.md`).
- **The gap is excursion, not level**: the median frame passes the gate by
  ~219K; the whole requirement sits in the worst ~80 frames (board,
  `SETUP_SHARE` entry). Size every candidate with the `convert.py`-style
  uniform-D re-rank **at rank-80** before building it; means and medians
  mislead here.
- The gate's second arm is **cadence ≥95% two-VBlank** (owner-decided,
  `plan.md`); banked gate claims report the 2/3/4/5+ VBlank-interval histogram
  and max interval alongside P50/P95 (AGENTS.md).
- Measurement law lives in `docs/VERIFYING.md` + the board's standing rules.
  Shorthand carried by all briefs: same-binary route A/B preferred; cross-build
  placement floor ≥14,080; same-binary paired-median floor ~5,440; the 2^22
  sampler correction (`WRAPFIX`) stays on; 2 profile cycles = 1 project tick.

## Suggested order (dependencies, not a queue)

**Foundation**
- **09** ARM/Thumb partitioning — Phase 1 (ISA ↔ TCM decoupling + shipping
  check) is the **sole owner** of that work item; 01 Phase 3 and 02 Phase 2
  only reference it. Land it before any broad TCM ranking.

**Fully diagnosed, cheap, low-risk**
- **14** Zero in-match card reads — seven known assets, 12,736 at rank-80 on
  the c219 basis, plus the permanent GO-boundary invariant checker.
- **04** Draw-memo completion — Phase 2 (delete the 1,280 B/frame copy) is
  sketched in `DRAW_MEMO.md` §7; part of a known 4,901 tk/fr overhead.
- **15** Dream Land collision tables — extends the proven slices 35–37
  playbook (−10,752 banked, bit-identical by construction).
- **01 Phase 1 only** — dead/zero-instruction ITCM residents (688 B + 54 B
  route with premise already closed).

**Big rocks (each may span many cycles; keep slices small and banked)**
- **03** Compact AOT animation representation — attacks the attach-driven
  re-parse that owns the `SITR` excursion (72,768 ceiling). Carries the
  owner-closed transition-play constraint: the transition-frame play is the
  sole writer of the new status's first pose. **Do not re-propose skipping
  it.**
- **12** Simulation Q chains — 142,786 reservoir (re-derive from `v3-c221`
  first, no build needed); warm-MAC subset 71,491 in 14 functions; collision
  bodies carry a renderer coupling (`parts->mtx_translate`) — read the brief's
  caution before touching them.
- **13** Draw-side fixed point — ≈22,521 remaining per the board, split stale;
  re-derive from `v3-c221` (no build needed).
- **05** GX hierarchy offload — per-joint matrix build is 61,848 tk/fr; feeds
  and is consumed by 11.
- **11** AOT-native renderer (**Task 51**, `NDS_BATTLE_PROFILE=0` seam) — the
  four-fighter-headroom structural change; consumes 04's memo seam and 05's
  ownership data. `FTR` is 291K and 59.3% cache fill: the win is deleting
  resident walk/decode code, not arithmetic.

**Continuous / dividend consumers (late by construction)**
- **06** Soft-float elimination — runs continuously as 03/12/13 close domains;
  helper members free only at **input-section granularity** (see its
  granularity constraint), and the reclaimed bytes feed 01.
- **07** Division/reciprocal — pairs with 03/12 slices; `__aeabi_fdiv` is the
  most expensive helper in the build (10,084 tk/fr).
- **08** Native texture formats, **10** DMA/transfer specialization —
  opportunistic; 10's first rule is *delete the copy* (04 owns the 1,280 B
  copy; do not DMA it).
- **01 (rest) + 02** TCM repacking — mostly consume dividends; final packing
  runs after 06's reclamation. ITCM refill buys ~3–4K tk/fr at best
  (`FTR_LANE.md`) — a rider, not a gate-closer.

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
