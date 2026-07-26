# TASK 40 — Fighter animation audit: every animation used, played, and correct

**STATUS (2026-07-21): COMPLETE / OWNER APPROVED.** Mario 195/195 and Fox
209/209 non-null motions are visually approved. Preserve the existing captures;
do not rerun known-good rows. The audit may disable Fox only while cycling rows;
it restores the published AI-on default before detaching.

**Standing rules apply in full: read `docs/optimization/TASK_STANDING_RULES.md`
first.**

**the owner's report (2026-07-20):** some fighter animations are missing or look
different from the original game. Mario looks fine so far — start the audit with
FOX, then sweep Mario anyway (the harness makes the second pass cheap).

**Sequencing:** do not run concurrently with the active Task 36 session (shared
renderer files). Independent of Tasks 37/38/39.

**Fidelity note — read carefully:** the standing fidelity doctrine's "95% is fine"
clause covers rendering SHORTCUTS (matrix precision, approximated effects). It does
NOT cover animation pose data: animations are data-driven and the source data is
authoritative, so the target here is FAITHFUL, not approximate. Approximation is
allowed only if a specific track feature proves genuinely expensive at runtime —
and then it goes through the owner with an A/B screenshot pair like any approximation.

## Phase 0 — map the pipeline and the seams (report before building anything)

1. Data path: where each fighter's animation data originates (O2R resources →
   `scripts/generate_nds_native_owners.py` and friends → what the runtime loads),
   and which decomp tables define the full motion set per fighter (ftmotion /
   motion-index tables in the imported ft* TUs — cite file:line).
2. Runtime path: the interpreter is imported original code
   (`src/import/battleship_sys_objanim.c`, `gcPlayDObjAnimJoint`) — enumerate every
   PORT-SIDE seam between its joint output and the emitted GX matrices
   (`nds_renderer.c` adapter: BuildDObjWorldMatrix path). Grep the renderer's
   fallback/TODO/unsupported markers (73 hits currently) and list which ones sit on
   the fighter animation path — each is a candidate "silently skipped track
   feature" (interpolation type, scale tracks, per-joint masks, etc.).
3. Deliverable: a one-page map naming every place animation data could be dropped,
   truncated, or decoded differently than the original — this is the checklist the
   audit instruments.

## Phase 1 — coverage + correctness audit

1. **Coverage instrumentation (profile-1 only, zero shipping impact):** counters
   keyed by motion/animation ID recording, per fighter: requested count, data
   resolved vs fallback taken, and any seam-marker hit from the Phase 0 list while
   that animation was active. Profile-0 symbol audit stays clean.
2. **Cycler harness (harness-only, behind a define — NOT a new harness mode if the
   existing Latest boundary pair can host it):** a driver that forces a standing
   fighter through every motion index in the decomp's motion table sequentially,
   holding each for its full duration, and captures a screenshot per animation
   (capture-melonds.ps1, non-clear-pixel assertion). Output: an ordered screenshot
   strip per fighter, filenames = motion ID + decomp symbol name where known.
3. **Automatable checks (no eyeballs needed):**
   - Table completeness: every motion ID in the decomp table has resolvable data
     in our converted assets (missing = finding, no screenshot needed).
   - Duration check: played frame count matches the source data's expected length
     (truncated/misconverted data shows here without visual judgment).
   - Seam hits: any animation whose playback trips a fallback/unsupported marker
     is auto-flagged incorrect-pending-review.
4. **Natural-play pass:** one scripted full match + the existing soak inputs, then
   report which motion IDs were NEVER requested naturally (expected for items-off /
   unreachable states — classify, don't fix).
5. **Deliverable — the coverage table**, per fighter, one row per motion ID:
   `plays-correct (pending the owner) / plays-wrong (seam or duration evidence) /
   missing-data / never-triggered`, plus the screenshot strips for the owner's review.
   **the owner is the correctness oracle for "looks different"** — attach strips, wait
   for his row-by-row verdict before Phase 2 fixes anything visual-only. Rows with
   hard evidence (missing data, duration mismatch, seam hit) may proceed without
   waiting.
6. **Gameplay-adjacency classification (mandatory per bad row):** hurtboxes attach
   to animated joints in this engine. For each wrong/missing animation, check
   whether the affected joints carry hurtbox/hitbox references in the decomp
   tables. Mark each fix `render-only` or `gameplay-adjacent`. Gameplay-adjacent
   fixes change collision outcomes and therefore require the full sharded
   Regression at session end, and get their own commit.

## Phase 2 — implement the fixes, DS-efficient

Order: missing-data rows first (usually pure asset-pipeline fixes), then seam
fixes, then the owner-flagged visual rows.

1. **Missing/misconverted data → fix at BUILD TIME.** Extend the O2R extraction /
   `generate_nds_native_owners.py` conversion so the data ships pre-converted —
   never add a runtime format parser. Cite the source resource (file:line /
   resource name) for every added asset. Regenerate, and keep the existing
   hierarchy/packet checkers green (`check_nds_native_owner_*.py` — extend them to
   cover the new data rather than bypassing them).
2. **Seam fixes → implement the missing track feature faithfully in fixed-point**
   at the existing decode stage (e.g., if an interpolation type or scale track is
   skipped, implement it in 20.12 in the adapter — these are a few ops per track
   per frame, not a perf concern). No new float paths, no per-frame allocation.
3. **Efficiency budget:** animation decode is update-side (×1.73 device class).
   Soft budget for the whole task: ≤ ~10K melonDS ticks added to the update pair
   P50, and the draw owner unchanged. Measure with a synchronized A/B on the
   standard windows; report deltas. If a single fix wants more than that, stop and
   report the option (e.g., build-time flattening of that one animation) instead
   of shipping the cost.
4. **Memory guard:** report the added asset bytes; the published ROM's free reserve
   (last known 166,672 bytes) must not go below ~100K, and the taskman arena must
   still get its full size (assert + report, per the arena lesson).
5. Re-run the Phase 1 audit after fixes: the coverage table must show every
   previously-bad row now `plays-correct (pending the owner)`, and a fresh screenshot
   strip goes to the owner for final approval.

## Gates

- verify-dev-fast + verify-boundary green; full sharded Regression once at session
  end IF any gameplay-adjacent fix or shared/imported TU was touched.
- Coverage table + strips delivered even if Phase 2 is not finished — the audit is
  valuable alone. Checkpoint honestly if the fix list is long; split Phase 2 across
  sessions rather than rushing.
- Perf A/B within budget; profile-0 symbol audit clean; screenshots via
  capture-melonds.ps1 with pixel assertions (you cannot see — never self-approve).

**Stop rules:** (a) an incorrect animation traces to the ENGINE (decomp behavior
itself differs from the real N64) — report it, do not hand-patch gameplay logic;
bit-exactness to the decomp is the contract and upstream divergence is a planner
decision. (b) A fix wants a new runtime interpreter or a float path — stop, report
the build-time alternative. (c) Any fix regresses the update budget past the soft
limit. Separate commits per phase; snapshot at the end.
