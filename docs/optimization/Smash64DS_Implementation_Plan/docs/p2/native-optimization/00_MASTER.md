# Master implementation plan

**Revision 2 — all legal four-fighter lineups on every selectable VS stage.**

## 1. Required result

Replace the expensive runtime representation and execution paths with compact, source-behavior-equivalent DS-native state and assets. Preserve content; remove work. Native GX/BG/OAM output is required in all target ROMs, including diagnostics. A host oracle may interpret original assets, but no target N64 graphics interpreter or software scene compositor may be introduced.

End conditions are independent:

1. **Performance:** every legal four-fighter lineup on every selectable VS stage must meet the product performance contract; the declared release matrix grades each case independently. Each required source-normal four-CPU stress population meets approximately P95 ≤ 1.12M ticks, using the repository's current exact 1,120,380-tick gate where applicable, and ≥95% two-VBlank presents. All menus retain their 30 Hz requirement. The internal 950,000-tick design objective is headroom, not a new product gate.
2. **Mechanics/content:** complete exercised native output and original mechanics/event ordering are preserved under the permitted numeric representation changes. Existing content bugs remain tracked; no-op or missing content cannot become performance evidence.
3. **Fixed runtime:** no reachable runtime binary32/binary64 arithmetic, including integer IEEE emulation, across shipped DS scenes/services. Constant source bytes alone are not arithmetic, but old float asset formats should be converted host-side where consumed.
4. **Architecture:** no permanent old/new authoritative state mirrors, replay recorders for already-compiled packet families, runtime immutable-asset discovery, or unknown-lifetime caches remain in converted domains.
5. **Resources:** every legal configuration must fit, enter, render and play; safely refusing a legal combination is containment, not acceptance. The original-DS memory envelope, live/transient heap and stack bounds, hardware resource constraints and declared service deadlines hold. Passing static byte sums is not runtime admission proof.

Sources: [S01, S02, S03, S04, S05, S06] in [15_SOURCE_INDEX.md](15_SOURCE_INDEX.md).

### Universal configuration contract

**The completed runtime must support every legal combination of four fighters on every selectable VS stage, including repeated fighter kinds and all legal slot assignments. Every configuration must satisfy the existing performance, native-rendering, mechanics, and resource requirements. A pilot, sampled roster, or measured leader never narrows this requirement. A known failing legal configuration blocks completion.**

Legal costumes, team/control settings, CPU levels, reachable detail modes, motion/model/copy states, items and stage hazards remain supported. This clarification concerns legal VS configurations; it does not introduce arbitrary four-player boss/campaign combinations that the source never permits, or move wireless multiplayer from P3 into P2. Single-console P2 retains one human plus up to three CPUs; four-CPU runs are stress tests.

The full product catalogue and currently implemented catalogue are separate. Unfinished/disabled required fighters or stages remain **BLOCKED product obligations**, never silently disappear from the required set. The source-derived catalogue is versioned and regenerated when content changes.

For twelve fighters and nine VS stages, the base universe is `C(15,4) × 9 = 12,285` unordered roster-stage cases with repetition, and `12^4 × 9 = 186,624` ordered cases. These are base counts, not counts of all costumes, rules, seeds or gameplay histories. Static resource checks enumerate all base/ordered-layout obligations. Release runs cover every base case with a full scored source-normal battle; ordered-slot and variant obligations require their own evidence or an explicit property-scoped equivalence proof. No proof means the obligation stays runnable/open, not waived.

[16_ALL_ROSTERS_ALL_STAGES.md](16_ALL_ROSTERS_ALL_STAGES.md) defines exact coverage sets, permutations, per-case gates, resume/invalidation, additional task cards and negative tests. **Pairwise sampling is a development aid and supplemental interaction technique, not release base-case coverage.** Rare exceptional frames remain allowed under the existing P95/cadence contract; this revision does not require every individual frame to stay below budget.

## 2. Baseline and amount of work to remove

The inspected final running-joint-mask artifact records WORK-H P50/P95 **1,680,384 / 2,389,376**, cadence histogram **103/749/881/240** for 2/3/4/5+ VBlanks, native failure/reject **0/0**, and general-heap low-water **108,096 B**. These are historical measurements at the pinned checkpoint, not newly measured results. Its final four-CPU ROM SHA-256 is `B1C037FB1EF0BB5339EC7E52E0C09D595B49DCEF320C5E20921EE076F813312A`. Do not substitute the same-ROM A/B hash or a published shell hash. [S04]

At that P95, the deficit is **1,268,996 ticks**, requiring **53.11%** reduction to the exact gate. Halving leaves 1,194,688 ticks, still too slow. A 950,000-tick objective requires **60.24%** reduction. Recompute from the reproduced baseline after adoption; never bank these arithmetic targets as achieved savings.

The design allocation below is an engineering challenge budget, not independently measured percentiles and not a promised decomposition:

| Exclusive work, per presented frame including required logic steps | Proposed ticks |
|---|---:|
| Gameplay, AI, collision, authored gameplay events | 420,000 |
| Pose evaluation and required sockets | 130,000 |
| Fighter draw preparation and native submission | 200,000 |
| Stage, effects and UI presentation | 100,000 |
| Non-overlapped audio/storage/resource service | 60,000 |
| Scheduling, interrupt and remaining work | 40,000 |
| Total | 950,000 |

Only integrated per-frame WORK-H and actual cadence determine performance acceptance. Parent/child buckets overlap; independent P95 deltas do not add. Quantized ALL/presentation intervals are not CPU-work attribution. Confirm timer ticks versus emulator ARM9 cycles before converting units.

## 3. What is already implemented

Retain packet replay, split roots, Link live-texgen patching, direct BGM/FGM range work, resident/early BPS1 directory work, pose active masks and fixed camera/hardware math unless new evidence rejects a route. They are the starting point, not proposed wins. Four-fighter capacity exists in scoped configurations; the challenge is performance plus remaining content completeness. [S02, S04]

Replace the *remaining expense*: immutable preflight on successful reuse, source-state construction for simple actors, repeated transform discovery, dynamic asset work, representation bridges, cold-state traversal, software arithmetic and unbounded source-shaped scheduling. Every task records what is removed and what replaces it.

## 4. Four design decisions to freeze first

**D1 — Single authority.** A native domain publishes native values. An old field may be a temporary adapter output only while unmigrated consumers need it. Record all such consumers and the task that deletes the bridge. No silent permanent mirror.

**D2 — Compile constants; bind lifetimes; update changes.** Host tools own source format interpretation and invariant computation. Load/spawn/topology mutation owns binding and validation. The frame path owns only actual changes and native submission.

**D3 — Event timing and visual interpolation are different contracts.** Do not solve the binary32 animation-clock discrepancy with a global epsilon or an unsupported exactness claim. Exact discrete outcomes and authored event boundaries require source-derived tests; continuous visual pose may use explicit bounded error.

**D4 — Bounded hardware ownership.** One state owner for texture residency, GX submission, math-unit usage and each shared buffer. DMA completion, geometry processing and raster texture lifetime are separate boundaries. No TCM buffers for DMA/ARM7.

Numeric formats and scene budgets become approved contracts after the baseline/range tasks, not guesses distributed through dozens of headers.

## 5. Delivery waves and critical path

| Wave | Packages | Observable exit |
|---|---|---|
| A: establish truth | N00 baseline/gate; N04 numeric inventory; bounded N01 layout cleanup | Reproduced baseline, precise gate, native interfaces and range obligations |
| B: delete preparation | N02 residency recovery; N03 bound renderer/packets; initial N05 pose | A hot converted draw path, measured work removed, safe memory/lifetime |
| C: remove representation bridges | N04 fixed primitives; N05 events/pose; N06 gameplay/collision/AI | Native producer-to-consumer pilot, followed by all required fighter/stage/variant consumers |
| D: expand content surfaces | N07 stages/VFX/UI; N08 audio/control and justified offload | Required resource sets and converted domains cover all reachable shipped scenes |
| E: pack and close | N09 final packing/retirement; N10 release qualification | No-float/runtime-retirement gates and exhaustive declared roster-stage/permutation qualification |

Package dependencies are encoded per task in `tasks.json`. Optional ARM7 work is not a prerequisite for an otherwise passing runtime. Interface design, independent host tests and independent source edits may overlap. Shared generators/builds and timing acceptance may not.

## 6. First implementation commits

These are coherent commit boundaries, not a forced chronology when dependencies change:

1. Adopt the plan and pin baseline identity, source configuration, units and case IDs; do not include runtime changes.
2. Add an explicit product-performance evaluation on existing captured rows; preserve the four-fighter correctness verifier's diagnostic role. Include deliberately failing fixtures.
3. Inventory numeric producers/consumers and exact input-section ownership. Record old bridge consumers and which immutable checks remain on a replay hit.
4. Split only the first measured cold binding/setup tail from an ITCM kernel; retain ISA/optimization settings so placement and codegen can be understood separately.
5. Introduce the smallest bound-draw contract and one simple actor conversion with lifetime tests. This proves the interface, not the campaign speedup.
6. Apply binding separation to the current hot fighter path and measure preflight/replay-hit CPU work before any roster-wide expansion.
7. Extend the existing generator with native packet payloads and typed patch metadata; prove geometry/state equivalence independently before replacing live replay recording.
8. Land range-checked fixed primitives and event-clock differential fixtures, then a complete pose-to-native-draw pilot. Temporary gameplay bridges remain explicitly open.
9. Recover memory by removing retired state and validate the required scene bank; eliminate admitted mandatory motion demand reads rather than creating a larger streaming cache.
10. Convert fixed movement/collision consumers and remove the pilot's gameplay bridge. Reproduce four-way engaged stress and re-price the remaining gap.

Do not spend ten commits only collecting reports. After the bounded baseline and contract work, the next kept runtime commit must remove a named repeated operation/path.

## 7. Decision and stop rules

A task ends with **KEEP**, **REVERT**, or **BLOCKED_WITH_SPECIFIC_CAUSE**; intermediate code may be **IMPLEMENTED_NOT_ACCEPTED**. Never label it FIXED merely because it compiles. A kept enabling interface may have neutral timing if its exact dependency and removal path are documented; it is not credited as a speed win.

Before implementing, measure or bound the cost actually removable by the hypothesis, including its callers. For each candidate compute `old work removed - new patch/copy/validation/service cost`; these estimates rank experiments only. After a whole owner is converted, rerank the complete population. If the credible remaining work-removal envelope is below the deficit, widen the architectural boundary rather than polishing a small leaf indefinitely.

No more than two materially different implementations of the same failed mechanism without a new causal observation. Preserve compact negative evidence. Do not brute-force compiler/linker permutations until a lucky layout wins.

A small delta under the repository's documented cross-build significance floor is not forced into KEEP. Use route engagement, paired changes, same-ROM evidence or a larger integrated slice. No routine long A/B/A cycles; repeated runs are justified by noise or conflicting evidence. [S03]

## 8. Product and numeric permission boundaries

The current project goal explicitly permits different internals and non-bit-identical numeric intermediates while preserving mechanics and feel. Older experiment restrictions are evidence about their own scope, not authority to prohibit the owner's present fixed-runtime request. Conversely, representation permission is not permission to change source reaction timing, skip collision, omit telegraphs, reduce legal pool capacity or silently downgrade art. Keep current quality/rate settings through the primary campaign. Any necessary fidelity/rate compromise needs its measured conflict and owner approval before permanent adoption. [S01]

Do not make retail measurements a dependency. Use repo-local accurate melonDS, interpreter with JIT disabled from boot, as prescribed by the current procedure. Record any newly exercised hardware-model uncertainty rather than tuning emulator timing to make the game pass. [S03]

## 9. Deliverables at campaign closure

A verifier-covered natural-input `smash64ds.nds`; a source-derived catalogue and exact-set coverage ledger with per-case work/cadence/resource verdicts; matching ELF/config/source and generated inputs; final exclusive cost report; cadence/tail report; exact resource-set and lifetime proof; link/disassembly no-float report; converted-domain retirement report; semantic/visual/audio coverage; source manifests; and a one-page remaining-issues statement.

Closure distinguishes scoped current-content progress from full-product qualification. An open required native owner or legal failed/unexecuted/stale case remains a blocker to universal closure. Maintain independent CPU, cadence, memory, VRAM, GPU and audio-service leaders. New content or linked-layout changes invalidate the corresponding evidence; a new observed leader is not a replacement for the declared coverage set. Never claim the full game is performance-complete from Dream Land and one four-kind roster alone.

**Repository refresh:** this revision rechecked `master` at `e67e5871ba8c4ae972f4826bfeb89757d3686401` and the unchanged product gate. Existing numerical evidence above remains pinned to its earlier ROM. Particle-index work and optimization-reference moves landed after that original snapshot; rebase the implementation baseline and inspect the current board rather than replaying old completed work. [S35, S36, S37]
