# Campaign 04 — Fighter Draw-Memo Completion

> **Planning baseline:** `codex/r2-runtime2` at `a63dd0e4b3af9c6601713f70c179c96c0aa67735` (2026-08-16).
>
> If HEAD has moved when implementation begins, re-run the inventory/measurement steps first and update symbol names/line references rather than blindly applying this document.
>
> **Campaign rule:** optimize toward a DS-native architecture and four-fighter headroom. The current two-fighter P95 gate is a checkpoint, not the architectural finish line. Never bank projected savings; measure the shipping configuration. Prefer same-binary route A/B when practical because this tree is placement-sensitive.
>
> **Basis (2026-08-17):** the shipping level is **+26,449** at rank-80 and the fresh per-PC census is `artifacts/performance/2026-08-17_shipping-rebank/v4-c238`. `c200` and `v3-c221` are retired. **Read `SHIPPING_REBANK.md` §7.7 before quoting any figure in this brief** — it lists what the new census contradicts, and mask the census by the GATE's own rank-80 frames.

## Objective

Finish the already-successful fighter draw-reuse memo so the cache-hit path is genuinely cheap.

Current remaining costs include:

- roughly **1,280 B/frame** of local contract copying;
- over-broad keys causing about **96 false misses** in the measured run;
- around **2,924 B** of mutable memo state that may be a DTCM candidate.

## Current measured state

From `artifacts/performance/2026-08-16_ftr-draw-memo/DRAW_MEMO.md` (build
`build-c223-ftrmemo`, same-binary route pair, **`NDS_R2_BOTH_CPU=1` stress arm**
— the gate-measurement configuration, not a casual default run):

- **96.19% hit rate** = `3,765` hits / `3,914` lookups;
- `147` invalidations, `149` fills, `164` bypasses (= the census's 164
  zero-event captures, by construction);
- `96` deliberate false misses (147 invalidations against the census's 51
  contract changes), worth **≈454 tk/fr** total;
- static cost **2,924 B bss + 460 B `.main` code**;
- banked **−12,864 at rank-80**, every geometry counter bit-identical.

The remaining hit-path overhead is **4,901 tk/fr** (gross ceiling at the
measured hit rate 18,565; `FTR` P50 moved 13,664): the 16-word key build and
compare, the 460 B of cold code, and the 1,280 B/frame `memcpy` — in a lane that
is 28.9% D-cache fill.

Keep the memo. Optimize the hit path.

## Current repo anchors

- `src/port/reloc_backend_renderer_dl.c`
- `src/port/diagnostics.c`
- shipping `NDS_R2_FTR_DRAW_MEMO ?= 1`
- `artifacts/performance/2026-08-16_ftr-draw-memo/DRAW_MEMO.md`
- `artifacts/performance/2026-08-16_ftr-capture-memo/CAPTURE_MEMO.md`
- `scripts/analyze-fighter-draw-reconciliation.py`
- `scripts/census-fighter-draw-phases.ps1`
- renderer parity/differ tooling

## Phase 0 — Document the contract/key

For every memo key field answer:

- what authoritative renderer state makes it change?
- is that change already represented by an epoch/generation?
- can it change while display-list pointer stays the same?
- is it correctness-critical or conservative?
- how many misses/invalidations did it cause?

Do not delete key fields until this table exists.

## Phase 1 — Close binding/display-list identity invalidation first

Before weakening the key, create/reuse a compact generation that changes whenever the cached contract is invalid because of:

- display-list identity;
- model-part binding;
- texture/material binding identity;
- generated native-owner identity.

Update the epoch at the authoritative mutation point, not by rescanning fields later.

**This doctrine now has a shipped, owner-confirmed precedent** (2026-08-17,
`artifacts/verification/2026-08-17_grab-throw-world-cache/GRAB_THROW_WORLD_CACHE.md`):
the flattened `ftParamsUpdateFighterPartsTransform` cache was keyed only by
`(root, heap generation)` and survived `ftMainSetStatus` inserting/re-parenting
Mario's joint 28 — frozen world transform, visible grab/back-throw spin bug.
The fix invalidates at the `ftMainSetStatus` **topology-writer seam**, exactly
the bind-where-broken pattern this phase specifies. A key narrower than the
mutation surface is not a theoretical risk in this codebase; it shipped a bug
this week.

This phase closes the memo's one known soundness hole: per-DObj
`dl`/`dls`/`dv`/flags state is **not hashed** (`DRAW_MEMO.md` §6). It is
currently believed unreachable for fighter joints — decomp's `ftparam.c` is not
compiled, the port bodies deliberately leave `joint->dl` alone, and no compiled
`DOBJ_FLAG_HIDDEN` writer touches a fighter joint — but `docs/HANDOFF.md`
records that the display-list-swap hole was **not re-verified** after the c224
cycle. Re-verify the writer census before, and make the invalidation explicit
here rather than relying on the current absence of writers.

Force a binding/display-list change in a test and prove: one invalidation, then clean hits.

## MEASURED 2026-08-17 — take the hit rate first, and Phase 2 cannot be measured alone

**The memo's hit rate fell 96.19% → 88.20% when the grab-transform repair
landed** (`SHIPPING_REBANK.md` §4): Hits 3,765 → 3,452, Fills 149 → 462,
Invalidations 147 → 111, ReplayEvents 60,462 → 55,292, Boundary 3,914 on both.
Lost hits == gained fills == **313**, and 5,170/313 = 16.52 replay events per
lost hit against the census's 16.08 per capture. The cause is
`ndsFighterRendererInvalidateStatusCachesOnSetStatus`
(`src/port/reloc_backend_renderer_dl.c:14125`, added by `14977e0ab8c`, called
from `ftMainSetStatus`) doing `sNdsFtrDrawMemo[slot].valid = 0u`; a `valid == 0`
slot returns before the key compare, which is why the *key-mismatch* counter
fell while Fills rose. Worth **~1,888 tk/fr match-average** (313 × ~9,650 ticks
per walk — an estimate) and more at rank-80: `battleship_ftMainSetStatus` is
**4.03×** and `ftDisplayMainDrawDefault` **3.51×** concentrated on the gate's
own rank-80 frames. **The invalidation is correctness-motivated and the repair
is owner-confirmed — do not delete it. Make it conditional on the topology
actually changing.** This is now the largest item in this campaign.

**Phase 2 is sized and it is below every floor this instrument has.** From the
linked ELF: `sNdsFtrDrawMemo` is `0xa90 = 2,704 B` for two slots,
`sNdsFighterDisplayContractPreambles` `0x300 = 768 B` for 32 entries
(24 B/preamble), so a slot event is 16 B, and at the measured 16.08 events per
capture over two fighters the hit-path copy is `2 × 16 × (16 + 24) =`
**1,280 B/frame** — confirming `DRAW_MEMO.md` §7. The linked `memcpy` is the
**170-byte ITCM word-copy** (`01fff6ec T memcpy`), not decomp's byte loop, so
the deletion is order **500–1,500 ticks/frame: 4–10× below the same-binary
paired-median floor (~5,440) and ~10–28× below the cross-build rank-80 floor
(≥14,080)**. **Do not run a whole-match A/B for it.** Ship it inside a larger
Campaign 04/11 slice, or price it with a per-unit constant probe
(`-PerStopGlobals` window sums).

## Phase 2 — Delete the 1,280 B/frame copy

`DRAW_MEMO.md` §7 already sketched the mechanism: the submit path is per-slot
and runs immediately after that slot's capture, so the consumer can read the
slot cache directly instead of `memcpy`ing events+preambles back into
`sNdsFighterDisplayContract`. Not built, not sized — size it before banking.

On hit, return/retain a stable pointer or compact handle to cached state rather than cloning a large local contract.

Rules:

1. cached storage remains valid for the full draw;
2. cached fields are read-only;
3. truly dynamic fields are outside the cached contract or supplied separately;
4. do not copy the whole object elsewhere to “solve” aliasing;
5. copy only a tiny mutable subset if absolutely required.

Add a bytes-copied counter and require this hit-path copy to reach zero.

## Phase 3 — Separate cached and dynamic inputs

Cached/epoch-stable examples:

- topology/primitive references;
- binding IDs;
- static material order;
- native-owner metadata.

Dynamic examples:

- matrices;
- visibility;
- colors/tints;
- dynamic material/texture epochs;
- transient fighter flags.

This becomes the seam for Campaign 11.

## Phase 4 — Attribute false misses, then decide

The ROI cap is known: the 96 false misses are worth **≈454 tk/fr** total, and
`DRAW_MEMO.md` explicitly warns that the key is *strictly stronger* than the
contract hash by design (it also carries `fp->lr`, `shade`, `costume`,
`detail_curr`, `is_modelpart_modify`, `colanim.skeleton_id`, `fp->attr`, tree
root pointer) and that weakening it "is not obviously the right trade". A false
miss costs one walk and cannot cost correctness; a stale hit can.

So: first add the per-word miss counter (one instrumented build; `fp->lr` on
turnaround is the named suspect). Only if one word owns most of the 96 **and**
its removal is provably epoch-covered by Phase 1:

1. remove it in a lab route;
2. vary the underlying state;
3. compare cached result with generic recomputation;
4. retain removal only with exact parity.

Do not chase an unsafe 100% hit rate; ≈454 tk/fr is the entire prize and
abandoning this phase is an acceptable outcome.

## Phase 5 — Test DTCM

After final compaction, Campaign 02 should A/B main RAM vs DTCM for the ~2.9 KB state.

Measure lookup/hit/consumer cost, whole FTR/WORK-H, and opportunity cost to other DTCM residents.

Keep it in main RAM if DTCM benefit/byte is weak.

## Phase 6 — Native-renderer API

Expose a narrow stable interface conceptually returning a cached/native draw contract plus small dynamic state.

Do not make Campaign 11 rebuild the generic contract after a cache hit.

## Verification

- cached vs uncached render parity;
- forced binding/display-list invalidation;
- owner hierarchy checks;
- Mario/Fox visual parity;
- hit/miss/invalidation counters;
- copied bytes/frame;
- one-minute match;
- FTR/WORK-H timing;
- any banked gate claim reports the 2/3/4/5+ VBlank-interval histogram and max
  interval alongside P50/P95 (AGENTS.md device-report law; plan.md's ≥95%
  two-VBlank cadence is the gate's cadence arm);
- no gameplay-state change.

## Completion criteria

1. Cache hits copy zero 1,280 B contracts.
2. Binding/display-list invalidation is explicit and proven.
3. False misses are reduced without stale reuse.
4. Renderer consumes cached state directly.
5. DTCM placement is measured, not assumed.
6. Shipping memo remains a net win.
