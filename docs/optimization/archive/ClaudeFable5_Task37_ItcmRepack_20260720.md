# TASK 37 — ITCM repack: shrink residents, evict cold code, add census toppers

**Standing rules apply in full: read `docs/optimization/TASK_STANDING_RULES.md`
first.**

**SEQUENCING — hard constraint:** do NOT run this concurrently with Task 36 (same
linker script and renderer hot set), and do NOT select new residents from any PC
census taken before Task 36's outcome landed: Task 36 Phase A removes the rigid-stage
CPU matrix compose, which cools `ndsRendererMtxMul20p12` and friends. Re-census on
the post-Task-36 renderer before choosing what to add.

## Context (verify numbers against the current .map, don't trust them blindly)

- ITCM usable ≈ 32,736 bytes (region 0x7fe0 at linker/nds_hot_text.ld:18, vectors
  excluded); last known occupancy 28,820 bytes → ~3.9K free.
- Current residents: renderer kernels (raw emit, production shade, fighter executor,
  submit-vertex) + Task 9/16 libgcc float members (extracted + renamed to `.itcm`
  via objcopy, Makefile ~:1043-1071; per-member list `NDS_TASK9_FLOAT_ITCM_MEMBERS`
  ~:345).
- Placement mechanisms already in the build: `*(.itcm .itcm.*)` input collection in
  the `.itcm` output section (nds_hot_text.ld:111-122), which precedes `.main` —
  so per-function lines like `*battleship_ftmain.o(.text.<symbol>)` added INSIDE
  the `.itcm` section place imported-TU functions with zero source edits (same
  first-match trick as `.text.hot`).
- Device calibration (Task 10): ITCM ×1.03, main-RAM streaming ×1.50, update ×1.73,
  draw ×1.51. melonDS models the ITCM-vs-main-RAM bus difference (it has no cache
  model but DOES charge main-RAM waitstates), so ITCM moves are
  **melonDS-sufficient class with an UNDERSTATED win**: melonDS A/B is the referee,
  and the calibration-predicted device delta (≈ melonDS delta × class multiplier)
  is reported alongside. No per-task device run; queue the pair.
- Graveyard fences: NO Thumb conversions anywhere (Task 12 retail REVERT — and
  Thumb inside ITCM raises executed instruction count for zero fetch benefit).
  DTCM is out of scope (Task 31 census closed it: five concurrent 16KiB coroutines).

## Phase A — inventory and heat audit (no behavior change)

1. From the current post-Task-36 build's .map: exact table of every ITCM symbol —
   name, bytes, source (project TU vs libgcc member), plus padding/veneer waste.
   Cross-check total against the region.
2. Fresh GDB PC census (Task 32 methodology, both draw and update windows, ~900+
   samples over ~30 s of scripted combat) INCLUDING ITCM address ranges, so the
   same table gains a samples column. Deliverable: ranked samples-per-byte for
   (a) current ITCM residents and (b) the top ~25 main-RAM functions.
3. Flag eviction candidates: any resident with ~0 samples across both windows
   (whole libgcc members nothing calls anymore count — check each member's entry
   points against the census, not just the file).

## Phase B — shrink the residents

1. Recompile PROJECT-SIDE ITCM residents at `-Os` via per-object CFLAGS overrides
   (never the libgcc members — prebuilt/hand-tuned). Rationale: inside ITCM there
   is no fetch penalty for smaller code; -O3 size bloat buys little there.
2. Accept/reject PER RESIDENT with a melonDS synchronized A/B on the typed owner
   counters: accept the -Os variant if the affected owner's P50 regression is
   ≤ ~2K ticks (the reclaimed bytes buy more elsewhere); otherwise keep -O2/-O3
   for that resident and note it.
3. Apply Phase A evictions (drop cold residents back to main RAM — into
   `.text.hot`/`.text.hot.draw` if they still sample warm there, plain `.main` if
   truly cold).
4. Report the reclaimed budget in bytes.

## Phase C — pack new residents

1. Greedy-pack from the Phase A ranking: value = samples/byte × class multiplier
   (update-path candidates weighted ×1.73, draw-path ×1.51). Expected shape: the
   small hot draw helpers (post-36 survivors among `MtxLoadN64ToDS20p12`,
   `FindDObjWorldMatrix`, `SyncTextureTile`, `ApplyStateSpan`, ...) and the
   hottest Task 17 update functions (`ftMainProcPhysicsMap*`, `gcRunGObjProcess`,
   search-hit kernels) — but let the fresh census decide, not this list.
2. Mechanism: per-function input-section lines inside the `.itcm` output section
   for imported TUs; `__attribute__((section(".itcm.<name>")))` or object renames
   for project TUs — whichever each case needs; zero source edits to decomp/.
3. **De-duplication is mandatory:** a symbol promoted to ITCM must be REMOVED from
   `.text.hot` / `.text.hot.draw` in the same change (first-match would silently
   satisfy the hot-text line anyway — remove the stale lines and re-verify both
   sections' sizes and asserts; `scripts/check-task32-draw-hot-text.ps1` must be
   updated to the new expected set, with the change cited in the report).
4. Fill to ≥95% of the region but keep the link hard-fail as the guard (region
   overflow = link error = the feature cannot silently misplace). Leave the final
   layout table in the report.
5. Everything behind `NDS_TASK37_ITCM_REPACK` (default 0; A/B pair buildable).
   Placement engagement proof is STRUCTURAL: the .map and the link asserts — no
   runtime counter is possible or needed; note this in the report instead of
   inventing one.

## Gates

- Fixtures + verify-dev-fast + verify-boundary green; full-match soak clean
  (results screen included).
- melonDS synchronized A/B (standard windows): whole-loop and typed owner counters
  must improve or hold; report exact deltas AND the calibration-predicted device
  delta per bucket. KEEP on melonDS evidence per the standing device-economy rule;
  build the retail A/B pair into `builds/device-queue/` for the next batched
  checkpoint.
- Pixel-exactness applies (this is placement, not approximation): synchronized
  frame 607 must be 0 changed pixels vs control.

**Kill criteria:** total calibration-predicted device win < ~10K ticks (not worth
the layout churn — revert placement, keep the Phase A audit as an artifact), or any
melonDS regression that per-resident -Os rejection can't localize, or hot-text
checker/link asserts that can't be reconciled → checkpoint on WIP branch, report,
stop. Separate commits per phase; snapshot at the end.
