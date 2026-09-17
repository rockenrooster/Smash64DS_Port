# N09 — Final retirement, hot-data/code placement and kernel tuning

> **Revision 2 coverage:** Universal scope: final no-float, reachability and TCM retirement cover all required content, not merely the pilot ROM call graph. Final layout changes conservatively stale timing evidence; N10 requalifies the declared case universe. See [16_ALL_ROSTERS_ALL_STAGES.md](16_ALL_ROSTERS_ALL_STAGES.md).

Do this on the smaller integrated runtime, not on a snapshot full of temporary old/new routes. Preserve qualified system residents and the current DTCM stack ceiling. Source-level line count, comments and total repository size are not runtime metrics. Report linked code/data and executed work. [S07, S08, S03, H01, H02]

## Packing policy

**DTCM:** compact CPU-only mutable state whose repeated accesses justify its bytes. Measure candidate records individually; do not mirror state permanently between main RAM and DTCM. Immutable bulk geometry and DMA/ARM7 buffers remain outside. Include alignment and current stack/interrupt low-water, not only data symbols.

**ITCM:** small dense kernels with measured whole-frame benefit, explicit unique sections and controlled literals/veneers. Binding, failure diagnostics, file access and source interpretation do not belong merely because their caller is hot. Renderer code does not receive an arbitrary entitlement to fill the region; neither does game code. The measured smaller kernel set decides.

**Main RAM/cache:** .text.hot and .text.hot.draw are placement groups in the same physical instruction-cache competition, not independent caches. Re-evaluate layout after code deletion; preserve old comments as historical evidence rather than permanent prohibitions on a new architecture.

**Bare metal:** inspect fixed arithmetic, packet patching, active-list and small matrix kernels before considering assembly. First remove operations and unnecessary reads. Test ARM/Thumb, modest per-kernel optimization levels, inlining and load/store scheduling on the actual toolchain. No global -O3/LTO/fast-math lottery. Source-compatible libraries may be rebuilt only with pinned reproducible options and ABI/regression proof.

### N09.01 — Close the whole-runtime float and retirement ledger

**Depends on:** N03.10, N05.08, N06.08, N07.07, N08.07, N04.04, N04.05, N04.06

**Edit/inspect boundary:** `proposed runtime numeric gate`; `Makefile`; `src/import`; `src/nds`; `src/port`; `both target-core linked artifacts`.

**Implementation sequence**

1. Run the complete source/type/lowering/link/caller audit on all shipped scene/service roots and resolve every temporary numeric allowlist entry.
2. Remove remaining duplicate authority, float conversion shims, integer IEEE arithmetic, obsolete graphics recorders and unreachable experimental helpers.
3. Keep source data/oracles host-side and source behavior references read-only; verify target object membership prevents their accidental inclusion.
4. Reconcile removal at input-section granularity and eliminate cold float formatting/transition/math-library roots as well as hot battle calls.
5. Prove retirement across the entire product-required content catalogue, including cold and unimplemented-required consumers. An unfinished legal owner prevents a universal retirement/completeness claim; shipped pilot reachability may be reported separately.

**Required tests/evidence:** T-FLOAT zero unresolved runtime arithmetic roots; T-RETIRE bridge/caller/data absence; weak/indirect/varargs/custom-IEEE negative fixtures.

**Work or dependency retired:** All remaining runtime floating arithmetic and completed-domain compatibility dependencies.

**Done:** The owner’s fixed-runtime endpoint is structurally proved across shipped configurations, not inferred from one trace.

**Stop/revert:** Any unresolved callback or cold path keeps this task OPEN; do not rename helpers or suppress checker findings.

### N09.02 — Pack compact CPU-hot state into DTCM

**Depends on:** N09.01, N02.06, N01.02

**Edit/inspect boundary:** `linker/nds_hot_text.ld`; `compact native game/pose/binding records`; `scripts/check-task20-dtcm-layout.ps1`.

**Implementation sequence**

1. Rank native mutable records by repeated CPU accesses and measured main-RAM cost per byte. Split cold fields before placement.
2. Test same-content single-authority location changes; update one base pointer at initialization rather than branch per access or synchronize two copies.
3. Account for alignment, runtime stacks, interrupts, boot and all scene transitions within the qualified ceiling.
4. Test combined packing and keep only whole-frame winners. Exclude every DMA/IPC/resource buffer requiring external visibility.

**Required tests/evidence:** T-TCM no-inaccessible-buffer and stack-canary/low-water tests; T-MEAS individual and combined placement; T-LIFE startup/transitions.

**Work or dependency retired:** Avoidable data-cache/memory stalls on the final compact CPU working set.

**Done:** The measured DTCM map improves integrated cost within current stack and ownership proof.

**Stop/revert:** No ceiling increase without new stack proof; no permanent DTCM/main-RAM state mirror.

### N09.03 — Repack ITCM around final kernels

**Depends on:** N09.02, N01.06

**Edit/inspect boundary:** `linker/nds_hot_text.ld`; `include/nds/nds_task37_itcm.h`; `native emit/pose/collision kernels`; `placement checker scripts`.

**Implementation sequence**

1. Rebuild the byte/caller/execution census after removing helper families and old renderer state; do not reuse a historical tenant ranking.
2. Remove placement from necessary cold code, shrink cold-inside-hot tails where still present and rank current gameplay/pose/emit kernels.
3. Test placement-only candidates and combined working sets with full literal/veneer/alias accounting.
4. Update placement assertions and record current code bytes by subsystem without treating a predetermined renderer/game split as a performance law.
5. Rank resident kernels on a measured suite containing all relevant mechanism/cost leaders, not one roster. Preserve measured legal-case performance in the final packing and invalidate affected timing evidence when linked layout changes.

**Required tests/evidence:** T-TCM linked/kernel identity, startup/IRQ/interworking and whole-frame profiling in final configuration.

**Work or dependency retired:** Avoidable instruction fetch stalls and remaining cold code occupying final ITCM.

**Done:** ITCM residents are explicitly justified by current measured benefit and all bytes fit.

**Stop/revert:** Do not evict an interrupt/startup/rare-required path as dead because of a short trace.

### N09.04 — Tune only remaining measured kernels

**Depends on:** N09.03

**Edit/inspect boundary:** `final native numeric/matrix/packet/active-list kernels`; `Makefile per-object compiler settings`.

**Implementation sequence**

1. Inspect the highest remaining exclusive kernels for redundant loads, spills, unnecessary 64-bit divides, missed constant folding, wide structures and call/veneer overhead.
2. Test one causal codegen change at a time: smaller affine form, load reuse, ARM/Thumb choice, optimization level or controlled inlining.
3. Write small assembly only when C output leaves a specific measurable gap; define ABI, clobbers, alignment, overflow and caller obligations and retain an independent host reference.
4. Remeasure the whole configuration after each retained batch; larger unrolling/code size can defeat cache/ITCM gains.

**Required tests/evidence:** T-NUM exact/bounded operation corpus, ARM9 codegen/ABI tests, T-MEAS net whole-frame gain and T-TCM fit.

**Work or dependency retired:** Remaining avoidable native instructions/memory stalls, not source-level complexity for its own sake.

**Done:** Every kept tuning change has a mechanism, tests and integrated evidence.

**Stop/revert:** Reject a microbenchmark win with worse total time or code-size pressure; do not use fast-math to fake fixed-point migration.

### N09.05 — Remove experimental routes and rebuild shipping shape

**Depends on:** N09.04

**Edit/inspect boundary:** `Makefile`; `native runtime configuration headers`; `scripts/verify-all.ps1`; `generated asset outputs`.

**Implementation sequence**

1. Remove losing/native A/B routes, route selectors, debug-only witnesses and migration allowlists from the shipping target.
2. Regenerate assets from pinned input and build source-normal natural-input smash64ds with no fast logic, scripted walk or demand-exclusion flags.
3. Keep matched ROM/ELF/config hashes and rerun integrated relevant verification plus product performance evaluation.
4. Compare final hard-on results to qualified experimental results; report placement/instrument differences instead of assuming they match.

**Required tests/evidence:** T-BUILD reproducible clean-input build, T-FLOAT final roots, T-MEAS final shipping scope and T-COVER natural input.

**Work or dependency retired:** Temporary dual implementations, experiment knobs and measurement-only dependencies from final runtime.

**Done:** The final binary, not just a routed experimental build, meets its current acceptance gates.

**Stop/revert:** A routed A/B success does not permit publishing an unmeasured hard-on ROM.

### N09.06 — Publish the final architecture and resource ownership map

**Depends on:** N09.05

**Edit/inspect boundary:** `docs/P2_EXECUTION_BOARD.md`; `docs/HANDOFF.md`; `docs/PERF_LEDGER.md`; `native source/interface comments`.

**Implementation sequence**

1. Update existing owners with concise final API/units/lifetime and generated-data descriptions; archive obsolete experiment detail rather than retaining contradictory runtime comments.
2. Publish exact final RAM/TCM/code/bank/transient budgets and the before/after work-deletion ledger.
3. Keep one authoritative queue and short restart pointer; documentation describes actual retained source, not planned but unlanded work.
4. Remove stale instructions that pin retired renderer or float-helper dependencies into future builds.

**Required tests/evidence:** T-DOC links/source/task references and final artifact identities; implementation and comments agree.

**Work or dependency retired:** Stale policy and restart ambiguity that could reintroduce retired paths.

**Done:** A new agent can identify current owners, constraints and acceptance without reading the full historical campaign.

**Stop/revert:** Do not label an unmet performance/content/numeric gate FIXED in the handoff.

