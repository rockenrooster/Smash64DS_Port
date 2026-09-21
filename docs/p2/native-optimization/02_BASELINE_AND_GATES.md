# N00 — Baseline, opportunity ledger and explicit gates

> **Revision 2 coverage:** The fixed current four-kind run is the initial regression/attribution anchor only. N00.06 defines the full required roster-stage universe early; N00.04 grades each scenario independently. Exhaustive runs belong to release qualification, not every edit. See [16_ALL_ROSTERS_ALL_STAGES.md](16_ALL_ROSTERS_ALL_STAGES.md).

Execution uses `13_AGENT_EXECUTION.md`: N00 outcomes may already be satisfied by
mapped valid evidence. These cards are not mandatory rework at each restart or a
reason to postpone an independently safe runtime slice until every global report
exists. Preserve actual safety prerequisites and explicit acceptance debt.

This is bounded enabling work. Reuse existing scripts and identities; do not build a second profiler or spend the campaign repeatedly re-establishing the same baseline. Sources [S01–S05, S18].

### N00.01 — Freeze inputs and resolve active policy

**Depends on:** None

**Edit/inspect boundary:** `PROJECT_GOAL.md`; `docs/VERIFYING.md`; `docs/HANDOFF.md`; `docs/P2_EXECUTION_BOARD.md`; `scripts/lib/harness-registry.ps1`.

**Implementation sequence**

1. Confirm the working branch/commit and compare with the pinned snapshot. Record relevant dirty overlays without modifying unrelated files; inspect the latest board checkpoint rather than selecting the oldest green artifact.
2. Reuse the matching baseline manifest and verify its source/generated assets, compiler/linker/library, ROM/ELF/config, emulator, save/DLDI, seed/input and runner identity. Fill missing or invalidated fields once; do not create a new manifest merely for a restarted context.
3. Classify exact, bounded-error and approval-required changes using C8. Resolve obsolete bit-exact-only experiment comments against the current product goal; preserve required discrete mechanics.
4. List baseline known bugs and missing native owners by scenario. Do not use missing output to claim a completed full-content performance result.
5. Adopt the owner clarification: every legal four-fighter lineup, repeated kinds included, on every selectable VS stage is in scope. Record full product-required and currently implemented content sets separately; a disabled unfinished fighter remains a product gap.
6. Retain the original 75f7f6b research evidence as historical. This revision rechecked master at e67e5871ba8c4ae972f4826bfeb89757d3686401; inspect the newer board, particle-quad-index work and moved optimization references before implementation. Do not re-credit retained optimizations.

**Required tests/evidence:** T-MEAS identity tests; parse manifest; independently verify ROM/ELF/config pairing and non-fast-logic runtime settings.

**Work or dependency retired:** Ambiguous baselines and repeated rediscovery; no runtime saving claimed.

**Done:** One reproducible identity and explicit policy/coverage scope are linked from P2-2p8.

**Stop/revert:** Missing derived inputs or a mismatched ELF block the measurement; do not silently substitute another ROM.

### N00.02 — Reproduce work, cadence and resource populations

**Depends on:** N00.01

**Edit/inspect boundary:** `scripts/verify-p2-four-fighter-stress.ps1`; `scripts/verify-battle-playable-realtime-harness.ps1`; `scripts/census-tick-hud-p95-set.py`.

**Implementation sequence**

1. Reuse valid matching source-normal baseline rows and same-run coverage/resource evidence first. Reproduce only for a named identity/population invalidator or missing proof. When a run is needed, use the current verified defaults and ring collector; the historical defaults are frame-1 identity and 1,972 samples at frames 2–1973, not an invariant source-minute after cadence changes. Use unique outputs.
2. Collect runtime roster/mask, gameplay clock, item override defaults, positive native output, pool/heap/resource witnesses and timing in the same run.
3. Calculate WORK-H from each row, verify accounting identities, and report P50/P95/P99/max, FPS, 2/3/4/5+ VBlank counts and consecutive late presents. Keep cold transition/lifecycle windows separate and test them separately.
4. Run the natural two-fighter regression using the required integrated profile when appropriate. Preserve current quality/audio/configuration; do not disable features for the baseline.

**Required tests/evidence:** T-MEAS populations and T-COVER engagement; retain raw rows and hashes. Confirm cycles/ticks ratio from timer and emulator instrumentation, not a remembered frequency.

**Work or dependency retired:** Invalid profiling populations; no runtime saving claimed.

**Done:** Comparable baseline rows with exact scope and independently reported correctness/performance verdicts.

**Stop/revert:** Stop on timing corruption, unexplained missing frames or unengaged fighters; investigate that cause before attribution.

### N00.03 — Assign exclusive costs and concrete deletion candidates

**Depends on:** N00.02

**Edit/inspect boundary:** `scripts/census-tick-hud-p95-set.py`; `scripts/task37_softfloat_callers.py`; `src/nds/nds_renderer_native_fighter_production.c`; `src/port/renderer_adapter_matrix.c`.

**Implementation sequence**

1. Reuse or derive one bounded exclusive owner table from applicable buckets and per-PC/caller data. Refresh only stale/missing attribution needed to choose the next complete work-removal slice. Separate SRC/GCRA/SINT/SCPU nesting, intended idle, blocked service, GX backpressure and instrumentation.
2. Inspect landed packet-precheck and fixed-particle checkpoints before pricing remaining replay/miss/record work, static stage preparation, pose/transform/publication, soft-float/conversions, copies and filesystem events. Do not redo a retired mechanism or assume an old profile describes the current path.
3. For each high-value candidate record exact repeated work, frequency, immutable/dynamic inputs, native replacement cost, memory cost and the test that proves it was removed.
4. Rank against both the P95-tail and cheapest-late-frame populations, then choose structural tasks that can plausibly address the measured deficit. Treat diagnostic work suppression only as an unqualified upper bound.

**Required tests/evidence:** T-MEAS accounting; sums of exclusive frame rows reconcile; no parent-plus-child or sum-of-percentiles budget.

**Work or dependency retired:** Unpriced optimization guesses and duplicate attribution.

**Done:** A bounded deletion ledger names the next highest-impact executable slices.

**Stop/revert:** If removable costs cannot explain the gap, widen the owner boundary; do not keep surveying individual tiny symbols.

### N00.04 — Implement a real product-performance verdict

**Depends on:** N00.02

**Edit/inspect boundary:** `scripts/verify-p2-four-fighter-stress.ps1`; `scripts/lib/harness-registry.ps1`; `scripts/census-tick-hud-p95-set.py`; `proposed scripts/check-p2-native-performance.py`.

**Implementation sequence**

1. Add a pure host evaluator for existing row/coverage artifacts, or extend the existing evaluator without changing the meaning of its prior reports. Publish explicit correctness, coverage, work-gate and cadence-gate verdicts.
2. Adopt the existing percentile convention consistently and store the rank/population. The 1,600-frame rank-80 sizing convention is not hardcoded into the 1,972-sample run.
3. Make a required product mode fail for P95 above the configured gate, two-VBlank cadence below 95%, missing coverage or invalid identities. Keep diagnostic candidate runs able to report RED without pretending implementation failed to execute.
4. Add final shipping-configuration evaluation and prevent an aggregate green capacity message from being used as performance acceptance.
5. Grade work, cadence, coverage, native output and resources PER scenario and PER required run. A pooled good percentile across other rosters cannot mask a slow legal lineup. Feed explicit case verdicts to N10.08.
6. Add fixtures where only one of many cases fails, or the same fast result is reused under a different roster, stage, slot order, CPU profile or artifact identity. Missing, stale, skipped and infrastructure-invalid evidence must not count as PASS.

**Required tests/evidence:** T-MEAS negative fixtures: over-budget-only, cadence-only, omitted slow frames, duplicate/malformed rows, missing identity, unengaged CPU, missing required owner; all fail the appropriate verdict.

**Work or dependency retired:** The acceptance gap where correctness-only passes are read as FPS closure.

**Done:** A deliberately slow valid ROM/report is explicitly performance RED; a valid fixture at the documented limits grades consistently.

**Stop/revert:** Do not tighten the product contract silently or exempt known slow frames. Fix evaluator definitions before using it to rank changes.

### N00.05 — Freeze semantic and visual regression fixtures

**Depends on:** N00.01, N00.06

**Edit/inspect boundary:** `scripts/fighters/test_pose_clock_differential.py`; `scripts/fighters/check_native_owner_geometry_closure.py`; `docs/p2/BUG_NOTES.md`; `docs/BUGS.md`; `proposed native optimization fixture manifest`.

**Implementation sequence**

1. Select controlled state fixtures for movement, hitlag, grabs/throws, platform boundaries, copy/morph/entry states, depth-intersecting effects and scene transitions.
2. Derive expected outcomes from the relevant BattleShip source and source assets; use independent native geometry closure rather than candidate output as the oracle.
3. Specify tolerance class per field, exact event ticks/order, resource identities, expected visible owners and audio cue timing.
4. Add explicit negative cases for no-op rendering and equal-count wrong-resource substitution. Fixtures should run cheaply before full-match qualification.
5. Select discriminating fixtures from the source-derived legal catalogue. Include all five duplicate multiplicity classes, directed fighter interactions and slot-specific behavior; reserve exhaustive release runs for N10, not every edit.

**Required tests/evidence:** T-NUM, T-CLOCK, T-GEOM, T-LIFE and T-COVER fixture sanity; every expected route has a positive witness.

**Work or dependency retired:** Vague correctness claims and tests that only prove zero error counters.

**Done:** Known tricky states have reproducible triggers and defined outcomes, independent of the implementation being replaced.

**Stop/revert:** An untriggered CPU move is not evidence; use source-controller playback or a clearly labeled deterministic state fixture.
