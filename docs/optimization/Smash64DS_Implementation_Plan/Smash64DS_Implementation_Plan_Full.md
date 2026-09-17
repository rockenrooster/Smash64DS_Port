# Smash64DS — Exhaustively Detailed Native-Runtime Implementation Plan

**September 15, 2026 · 75 tasks · 11 packages · pinned master 75f7f6b4b4864c82c01872d0fd2771d171005272**

This reader edition is generated from the split implementation package. All runtime work remains planned; no game build or benchmark is claimed. The split documents, task graph, evidence templates, validator and additive patch are supplied separately.

## Reader navigation

- [README.md](#doc-readme)

- [00_MASTER.md](#doc-00-master)

- [01_CONTRACTS.md](#doc-01-contracts)

- [02_BASELINE_AND_GATES.md](#doc-02-baseline-and-gates)

- [03_TCM_AND_BUILD.md](#doc-03-tcm-and-build)

- [04_RESIDENCY_AND_ASSETS.md](#doc-04-residency-and-assets)

- [05_BOUND_RENDERER.md](#doc-05-bound-renderer)

- [06_PACKET_COMPILER.md](#doc-06-packet-compiler)

- [07_FIXED_NUMERICS.md](#doc-07-fixed-numerics)

- [08_EVENTS_AND_POSE.md](#doc-08-events-and-pose)

- [09_GAMEPLAY_COLLISION_AI.md](#doc-09-gameplay-collision-ai)

- [10_STAGE_VFX_UI.md](#doc-10-stage-vfx-ui)

- [11_AUDIO_AND_OFFLOAD.md](#doc-11-audio-and-offload)

- [12_FINAL_PACK_AND_RETIREMENT.md](#doc-12-final-pack-and-retirement)

- [13_AGENT_EXECUTION.md](#doc-13-agent-execution)

- [14_VALIDATION.md](#doc-14-validation)

- [15_SOURCE_INDEX.md](#doc-15-source-index)


---


<a id="doc-readme"></a>

## P2-2p8 native-runtime implementation campaign

**Plan date:** September 15, 2026. **Read-only planning baseline:** `75f7f6b4b4864c82c01872d0fd2771d171005272` on `master`.

This is an implementation specification, not an implemented optimization or a new benchmark. All task states begin **PLANNED**. Proposed paths, APIs, formats and test IDs are explicitly new work; existing anchors refer to the pinned repository. No ROM was built and no GitHub branch was modified while preparing this package.

### Start and authority

Read [00_MASTER.md](#doc-00-master), then the assigned package, relevant contracts in [01_CONTRACTS.md](#doc-01-contracts), and its tests in [14_VALIDATION.md](#doc-14-validation). Do not reread the whole research report on every restart.

`PROJECT_GOAL.md` remains the product authority. `docs/VERIFYING.md` remains the execution procedure. `docs/P2_EXECUTION_BOARD.md` remains the **only live queue**. Register this campaign beneath existing **P2-2p8**, not as a second competing milestone. The task graph here is a static implementation breakdown; record current status only in the existing board/evidence, or generate a view from it. Do not manually maintain two status ledgers.

The owner's requested endpoint is a smaller, fixed-point DS runtime, substantially lower CPU time, native rendering in every ROM, and sufficient headroom for four engaged players/CPUs. Runtime includes menus, audio control, transitions and rare game states, not just the measured battle loop. ROM-derived source assets, source data and reference code remain read-only; build tools may use floating point. Integer code that emulates binary32 arithmetic is not fixed point.

### Documents

| File | Purpose |
|---|---|
| [00_MASTER.md](#doc-00-master) | Decisions, sequence, budgets, first commits and integration rules |
| [01_CONTRACTS.md](#doc-01-contracts) | Native numeric, ownership, mutation, resource and packet interfaces |
| [02_BASELINE_AND_GATES.md](#doc-02-baseline-and-gates) | Reproducible measurement; explicit product gate |
| [03_TCM_AND_BUILD.md](#doc-03-tcm-and-build) | Bounded early ITCM reclamation, build/telemetry cleanup |
| [04_RESIDENCY_AND_ASSETS.md](#doc-04-residency-and-assets) | Memory recovery and deterministic pre-GO asset admission |
| [05_BOUND_RENDERER.md](#doc-05-bound-renderer) | Binding and dynamic-update separation; eliminate source-shaped prep |
| [06_PACKET_COMPILER.md](#doc-06-packet-compiler) | Generated native GX streams, patching and hardware lifetime |
| [07_FIXED_NUMERICS.md](#doc-07-fixed-numerics) | Numeric graph, fixed primitives, constants and no-float closure |
| [08_EVENTS_AND_POSE.md](#doc-08-events-and-pose) | Event timing, compact pose, required sockets and transform reuse |
| [09_GAMEPLAY_COLLISION_AI.md](#doc-09-gameplay-collision-ai) | Fixed gameplay, collision, shared query facts and ordering |
| [10_STAGE_VFX_UI.md](#doc-10-stage-vfx-ui) | Static/dynamic stage split, particles and native UI |
| [11_AUDIO_AND_OFFLOAD.md](#doc-11-audio-and-offload) | Audio/storage bounds and justified DMA/ARM7/math offload |
| [12_FINAL_PACK_AND_RETIREMENT.md](#doc-12-final-pack-and-retirement) | Whole-runtime cleanup, TCM tuning and release closure |
| [13_AGENT_EXECUTION.md](#doc-13-agent-execution) | Task handoffs, source ownership, safe integration and example commands |
| [14_VALIDATION.md](#doc-14-validation) | Concrete regression, negative, coverage and acceptance tests |
| [15_SOURCE_INDEX.md](#doc-15-source-index) | Immutable evidence anchors; what is known versus proposed |
| `tasks.json`, `tasks.csv` | Static dependency graph and implementation register |
| `templates/` | Unmeasured baseline, experiment and admission templates |

### Install without overwriting active work

These files are additive under `docs/p2/native-optimization/`; the supporting plan validator is under `planning-tools/`. Copy into a clean review branch or apply the supplied additive patch after `git apply --check`. Refuse collisions rather than overwrite an existing campaign. Inspect `git status --short` first. The package does not edit `decomp/`, existing generated outputs, existing P2 plans, or the root ROM.

Add a link from the existing P2-2p8 board row and one pointer from the handoff when adopting the campaign. Keep the original research document as supporting analysis, not as a second execution queue.

A standalone combined Markdown edition is also provided for reading. Edit the split source documents and regenerate the combined view; do not maintain both by hand.



---


<a id="doc-00-master"></a>

## Master implementation plan

### 1. Required result

Replace the expensive runtime representation and execution paths with compact, source-behavior-equivalent DS-native state and assets. Preserve content; remove work. Native GX/BG/OAM output is required in all target ROMs, including diagnostics. A host oracle may interpret original assets, but no target N64 graphics interpreter or software scene compositor may be introduced.

End conditions are independent:

1. **Performance:** the standing source-normal four-CPU stress population meets approximately P95 ≤ 1.12M ticks, using the repository's current exact 1,120,380-tick gate where applicable, and ≥95% two-VBlank presents. All menus retain their 30 Hz requirement. The internal 950,000-tick design objective is headroom, not a new product gate.
2. **Mechanics/content:** complete exercised native output and original mechanics/event ordering are preserved under the permitted numeric representation changes. Existing content bugs remain tracked; no-op or missing content cannot become performance evidence.
3. **Fixed runtime:** no reachable runtime binary32/binary64 arithmetic, including integer IEEE emulation, across shipped DS scenes/services. Constant source bytes alone are not arithmetic, but old float asset formats should be converted host-side where consumed.
4. **Architecture:** no permanent old/new authoritative state mirrors, replay recorders for already-compiled packet families, runtime immutable-asset discovery, or unknown-lifetime caches remain in converted domains.
5. **Resources:** the original-DS memory envelope, live/transient heap and stack bounds, hardware resource constraints and declared service deadlines hold. Passing static byte sums is not runtime admission proof.

Sources: [S01, S02, S03, S04, S05, S06] in [15_SOURCE_INDEX.md](#doc-15-source-index).

### 2. Baseline and amount of work to remove

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

### 3. What is already implemented

Retain packet replay, split roots, Link live-texgen patching, direct BGM/FGM range work, resident/early BPS1 directory work, pose active masks and fixed camera/hardware math unless new evidence rejects a route. They are the starting point, not proposed wins. Four-fighter capacity exists in scoped configurations; the challenge is performance plus remaining content completeness. [S02, S04]

Replace the *remaining expense*: immutable preflight on successful reuse, source-state construction for simple actors, repeated transform discovery, dynamic asset work, representation bridges, cold-state traversal, software arithmetic and unbounded source-shaped scheduling. Every task records what is removed and what replaces it.

### 4. Four design decisions to freeze first

**D1 — Single authority.** A native domain publishes native values. An old field may be a temporary adapter output only while unmigrated consumers need it. Record all such consumers and the task that deletes the bridge. No silent permanent mirror.

**D2 — Compile constants; bind lifetimes; update changes.** Host tools own source format interpretation and invariant computation. Load/spawn/topology mutation owns binding and validation. The frame path owns only actual changes and native submission.

**D3 — Event timing and visual interpolation are different contracts.** Do not solve the binary32 animation-clock discrepancy with a global epsilon or an unsupported exactness claim. Exact discrete outcomes and authored event boundaries require source-derived tests; continuous visual pose may use explicit bounded error.

**D4 — Bounded hardware ownership.** One state owner for texture residency, GX submission, math-unit usage and each shared buffer. DMA completion, geometry processing and raster texture lifetime are separate boundaries. No TCM buffers for DMA/ARM7.

Numeric formats and scene budgets become approved contracts after the baseline/range tasks, not guesses distributed through dozens of headers.

### 5. Delivery waves and critical path

| Wave | Packages | Observable exit |
|---|---|---|
| A: establish truth | N00 baseline/gate; N04 numeric inventory; bounded N01 layout cleanup | Reproduced baseline, precise gate, native interfaces and range obligations |
| B: delete preparation | N02 residency recovery; N03 bound renderer/packets; initial N05 pose | A hot converted draw path, measured work removed, safe memory/lifetime |
| C: remove representation bridges | N04 fixed primitives; N05 events/pose; N06 gameplay/collision/AI | Native producer-to-consumer paths across the active four-fighter configuration |
| D: expand content surfaces | N07 stages/VFX/UI; N08 audio/control and justified offload | Required resource sets and converted domains cover all reachable shipped scenes |
| E: pack and close | N09 final packing/retirement; N10 release qualification | No-float/runtime-retirement gates and full four-CPU product acceptance |

Package dependencies are encoded per task in `tasks.json`. Optional ARM7 work is not a prerequisite for an otherwise passing runtime. Interface design, independent host tests and independent source edits may overlap. Shared generators/builds and timing acceptance may not.

### 6. First implementation commits

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

### 7. Decision and stop rules

A task ends with **KEEP**, **REVERT**, or **BLOCKED_WITH_SPECIFIC_CAUSE**; intermediate code may be **IMPLEMENTED_NOT_ACCEPTED**. Never label it FIXED merely because it compiles. A kept enabling interface may have neutral timing if its exact dependency and removal path are documented; it is not credited as a speed win.

Before implementing, measure or bound the cost actually removable by the hypothesis, including its callers. For each candidate compute `old work removed - new patch/copy/validation/service cost`; these estimates rank experiments only. After a whole owner is converted, rerank the complete population. If the credible remaining work-removal envelope is below the deficit, widen the architectural boundary rather than polishing a small leaf indefinitely.

No more than two materially different implementations of the same failed mechanism without a new causal observation. Preserve compact negative evidence. Do not brute-force compiler/linker permutations until a lucky layout wins.

A small delta under the repository's documented cross-build significance floor is not forced into KEEP. Use route engagement, paired changes, same-ROM evidence or a larger integrated slice. No routine long A/B/A cycles; repeated runs are justified by noise or conflicting evidence. [S03]

### 8. Product and numeric permission boundaries

The current project goal explicitly permits different internals and non-bit-identical numeric intermediates while preserving mechanics and feel. Older experiment restrictions are evidence about their own scope, not authority to prohibit the owner's present fixed-runtime request. Conversely, representation permission is not permission to change source reaction timing, skip collision, omit telegraphs, reduce legal pool capacity or silently downgrade art. Keep current quality/rate settings through the primary campaign. Any necessary fidelity/rate compromise needs its measured conflict and owner approval before permanent adoption. [S01]

Do not make retail measurements a dependency. Use repo-local accurate melonDS, interpreter with JIT disabled from boot, as prescribed by the current procedure. Record any newly exercised hardware-model uncertainty rather than tuning emulator timing to make the game pass. [S03]

### 9. Deliverables at campaign closure

A verifier-covered natural-input `smash64ds.nds`; matching ELF/config/source and generated inputs; final exclusive cost report; cadence/tail report; exact resource-set and lifetime proof; link/disassembly no-float report; converted-domain retirement report; semantic/visual/audio coverage; source manifests; and a one-page remaining-issues statement.

Closure distinguishes current shipped content from unimplemented P2 content. An open required native owner remains a content blocker. When additional content lands, its new measured argmax becomes the standing stress case. Never claim the full game is performance-complete from Dream Land and one four-kind roster alone.



---


<a id="doc-01-contracts"></a>

## Shared implementation contracts

These are **proposed native interfaces and rules to implement**, not a claim that the current tree already exposes these APIs. Keep public names consistent with existing ownership where possible. Add only interfaces that have both a real producer and consumer. [S07–S17, H01–H06]

### C1. Domain ownership and minimal interfaces

| Boundary | Producer | Consumer | Native payload | Not allowed in the hot consumer |
|---|---|---|---|---|
| Source assets → native bank | Existing host generators | Scene admission | Versioned little-endian tables, offsets, fixed constants, native GX words | N64 command decoding, endian conversion, source pointer relocation |
| Admission → bound instance | Scene/spawn/topology owner | Native draw/pose | Validated table pointers and resource handles | File opens, format discovery, general texture lookup |
| Simulation → presentation | Source-ordered fixed game loop | Pose/draw | Position/facing, pose index/phase, material/visibility generations | Reinterpretation of fixed words as floats |
| Pose → gameplay sockets | Native pose evaluator | Hit/grab/weapon logic | Required fixed local/world matrices or socket vectors | GPU matrix readback; full visual hierarchy rebuild |
| Pose → GX | Native pose evaluator | Packet patcher | Affine matrices and typed state patches | Repeated float conversion and all-purpose traversal state |
| Service → game | Audio/storage/ARM7 owner | Game thread | Bounded completion records | Hidden blocking joins or stale shared pointers |

A temporary migration shim may present legacy arguments to unmigrated code, but the dependency register must name each reader/writer and the task that removes it. Do not add temporary conversions to the final hot ABI. Existing source-compatible `FTStruct`/DObj fields cannot simply be reinterpreted as fixed values while old arithmetic still reads them.

### C2. Numeric representation

Start from semantic units, not an arbitrary universal Q format. The range census publishes min/max, required slack, rounding, product widths and overflow proof for each field. Candidate defaults below must pass that census:

| Quantity | Candidate native representation | Obligation |
|---|---|---|
| World position / ordinary translation | Signed 32-bit integer with 12 fractional bits | In source units this spans −524,288 through 524,287.999755859375; prove every live state and intermediate fits |
| Velocity / acceleration | Signed 32-bit, 12 or 16 fractional bits selected for full chain | Preserve terminal velocity, damping and event thresholds; fractional choice is per chain |
| Matrix coefficients | Signed 32-bit, 12 fractional bits; affine 12 words where applicable | Matrix orientation/layout, translation scaling and narrowing proven against consumers |
| Unit directions / normals | Signed bounded fixed representation appropriate to each consumer | Quantize to GX encoding only at packet generation/patch seam; don't propagate hardware vertex limits into game physics |
| Angles | Unsigned turn representation or signed wrapped delta, often 16-bit | Explicit wrap, shortest-path interpolation, source coordinate handedness |
| Percent/status/timers/masks | Native bounded integers | Do not convert integer semantics into fractional arithmetic |
| Clip phase / time | Integer event cursor/deadline plus qualified fixed/rational phase | Authored event boundaries, remainders, hitlag, speed changes and joint-lane order |
| Texture coordinate | Signed fixed coordinate native to emitted GX command | Scale/bias/half-texel policy and source texgen semantics preserved |

For a 32-bit signed value with F fractional bits, document the representable real interval explicitly. `Q20.12` naming is ambiguous about whether the sign bit is counted; headers should state width and fraction bits, not rely on that shorthand alone.

Each primitive defines inputs, arithmetic result, rounding and failure domain. Use multiplication, not left-shifting negative signed integers, when forming scaled intermediates in portable C. Prove sums of products fit signed 64-bit before accumulating. A squared-distance expression with three axes requires a bound on the *sum*, not just each square. Do not use float to implement a fixed primitive.

Round-to-nearest, floor, truncation-toward-zero and saturation are distinct operations. Name them or attach the rule to the consumer. Avoid global saturation: it can conceal corrupted physics and turn overflow into a plausible but wrong state. Unsupported input at admission fails with identity; a runtime arithmetic contract violation latches a diagnostic and follows the established safe error route, never wraps silently.

Host tests should use independent high-precision integer/rational math and the source oracle. Do not declare a candidate correct by comparing it to the same helper compiled twice. The target toolchain's C/ABI choices are pinned, and critical kernels receive ARM9 compile/disassembly tests.

### C3. Bound draw description

A compact conceptual descriptor, **not a frozen on-disk C layout**:

```c
/* Proposed API contract. Final sizes/order come from range and bank census. */
typedef struct NativeBoundDraw {
    const uint32_t *geometry_words;   /* DMA-visible admitted memory */
    uint32_t geometry_word_count;
    const void *patch_table;          /* typed fixed-width records */
    uint32_t patch_count;
    uint32_t residency_generation;
    uint32_t topology_generation;
    uint16_t model_variant;
    uint16_t draw_class;
} NativeBoundDraw;
```

At binding, resolve run topology, transform class, native material words, valid resource views, source draw order and patch offsets. In steady state, read a small descriptor and apply only dynamic fields. A runtime binder is permitted for a genuine topology change, but it must not perform mandatory source-file demand reads after GO or rebuild unrelated instances.

A descriptor does not authorize arbitrary writes into a packet. Admission proves each patch's command, word span, count, alignment, source index and bounds. Mutable counts are rechecked when they can change. Do not trust a stale descriptor just because its pointer is non-null.

### C4. Mutation and invalidation table

| Mutation | Invalidate/update | Must not invalidate merely because of this mutation |
|---|---|---|
| New scene / arena lifetime | All scene-backed handles and instance bindings | System-owned persistent services with independently valid lifetime |
| Same-address memory reuse | Lifetime generation and all references to retired allocation | Unrelated live allocations |
| Fighter status with topology mutation | Parent/child binding, required-joint closure, affected packet variant | All four fighters' immutable geometry |
| Kirby copy / costume or selected model part | Affected resource view and model/attachment binding | Unchanged stage and other fighters |
| Samus morph or entry model | Selected native variant and required sockets | Whole bank relocation |
| Clip attach / detach / seek | Event cursor, affected pose tracks and dirty descendants | Texture residency |
| Speed change or hitlag | Timing phase/deadline and dependent pose values | Immutable animation bank |
| World transform / facing | World/socket transforms and affected native matrices | Local clip data |
| Camera change | View/projection/billboard/texgen dependencies | Model-local pose and topology |
| Palette/material frame | Native material/palette selection or pre-admitted updates | Joint hierarchy |
| Texture bank remap at legal boundary | Resource handles and GPU-use generation | Current-epoch handles before retirement has completed |

Use mutation-owned generations/dirty masks. Enumerate every writer before replacing existing hashes/invalidation; the current `ftMainSetStatus` wrapper documents topology changes that leave root/heap identity unchanged. [S15]

Use 32-bit generations unless a proven bounded lifetime justifies smaller values. On wrap, rebuild/invalidate at a quiescent boundary; never let an old handle become valid again accidentally. Publish a new generation only after the associated data is complete. A single game-thread owner does not need an atomic operation for every field; IPC publication does need explicit visibility and ordering.

### C5. Packet and graphics lifetime

A packet buffer has a state machine:

```text
FREE → CPU_WRITING → READY → DMA_READING → REUSABLE
```

Only CPU_WRITING may be patched. Flush written source cache lines and complete required write-buffer synchronization before READY is submitted. DMA_READING cannot be modified, freed, recycled or used as conversion scratch. A CPU submission route has its own completion condition but the same no-overlap rule. The exact SDK operation and DMA channel owner must be read from the pinned toolchain, not invented from another SDK's API.

Textures/palettes have a separate lifetime:

```text
ADMITTED → REFERENCED_BY_SUBMISSION → IN_FLIGHT_RENDER_USE → RETIRABLE
```

DMA completing its read of command words is **not** texture retirement. Establish the renderer's actual geometry/raster/flip boundary before uploading over texture memory. Keep resident battle resources stable through the locked epoch.

Packets must begin with sufficient state or an explicitly validated preamble contract; no dependence on a prior fighter's lucky material/matrix mode. Matrix stack/store/restore slots are a finite hardware resource. The current generator checker uses current-matrix sentinel 31 and stored slots 0–30: preserve the existing convention or change generator, patcher and checker together. [S19]

Patching constants for four instances requires per-instance mutable storage or serialized safe scratch ownership; never mutate one shared template while another instance's DMA reads it. Hardware FIFO words remain native DS commands, not a new CPU-interpreted N64 program.

### C6. Native asset bank format

Extend an existing native bank where possible. Introduce a version only when its representation changes. Proposed header fields are: magic/version, header bytes, total bytes, source-content hash, converter/schema hash, table directory, resource profile ID and payload integrity value. Individual tables carry kind/count/offset/stride/alignment. Wire formats are explicitly little-endian, offsets are relative to the bank, and no host pointer/padding is serialized.

Validation order: fixed header → size limits → table-directory bounds → offset/alignment/count multiplication → referenced spans → resource/command domain → semantic identities → publish runtime pointers. Bounds use subtraction-based checks that cannot wrap. Checksums catch corruption but cannot prove semantic coverage; compare required identities, not only hashes or equal counts.

Bank lifetime allows immutable sharing across same-kind fighters. Instance event cursors, visibility, costume state and patch buffers are never shared mutable state. Record output ROM bytes, permanent resident bytes, load scratch, transition peak and unavoidable streaming bytes separately.

### C7. Admission and memory

Before GO, atomically admit the required fighter/motion/material/effect/item/stage set for the selected legal scene. Required post-GO motion/texture demand reads are zero. Streaming BGM is a distinct declared service with reserved buffers/deadlines. One-shot cues need a separately qualified resident/deadline-safe policy. [S06]

Budget **transient peak**, not just final footprint. A transactional bank switch that retains old resources while building new ones can OOM even when each final scene fits. Pre-reserve the combined scratch or plan an explicitly quiescent retirement boundary before loading. Preserve UI responsiveness during loading; loading work is cheaper than battle work, not permission to freeze the shell indefinitely.

Memory refusal states why the scene cannot be admitted. Refusal is containment, not a completed port. Do not disable a fighter/effect to make admission succeed. CSS preview banks are distinct from a battle's full gameplay banks and should not demand residency of the entire roster simultaneously.

DTCM is CPU-local. The current qualified data ceiling is `0x02ff3000`; re-prove stack boundaries before raising it. ARM7/DMA shared data stays in legally visible non-TCM storage. [S08, H02]

### C8. Numeric and semantic comparison classes

**Class E (exact):** pure refactor/placement, resource identity, source event order, integer counters with gameplay meaning, RNG draw order, packed geometry/source topology and unchanged native command semantics.

**Class B (bounded continuous error):** fields whose representation intentionally changes. Report max absolute error, boundary distance, overflow margin, cumulative drift and downstream decisions. Tolerance must be derived from quantization and the source domain, not a blanket pixel/world-unit epsilon.

**Class A (approval required):** different quality, temporal update rate, simplified game rules or removed content. Keep out of the primary transparent optimization path until approved with evidence.

A B-class field crossing an E-class decision boundary needs explicit source-case proof/correction. Do not accept different hit/ledge/status timing because the position difference is numerically small. Whole-match state hashes may differ after permitted continuous differences; use reproducible short state fixtures plus long natural behavior/engagement coverage instead of weakening every assertion.



---


<a id="doc-02-baseline-and-gates"></a>

## N00 — Baseline, opportunity ledger and explicit gates

This is bounded enabling work. Reuse existing scripts and identities; do not build a second profiler or spend the campaign repeatedly re-establishing the same baseline. Sources [S01–S05, S18].

#### N00.01 — Freeze inputs and resolve active policy

**Depends on:** Campaign adoption and current-source identity

**Edit/inspect boundary:** `PROJECT_GOAL.md`; `docs/VERIFYING.md`; `docs/HANDOFF.md`; `docs/P2_EXECUTION_BOARD.md`; `scripts/lib/harness-registry.ps1`.

**Implementation sequence**

1. Confirm the working branch/commit and compare with the pinned snapshot. Record relevant dirty overlays without modifying unrelated files; inspect the latest board checkpoint rather than selecting the oldest green artifact.
2. Record source, generated assets, compiler/linker/library versions, ROM/ELF/config hashes, emulator hash/config, save/DLDI state, seed/input and runner identity in the baseline manifest.
3. Classify exact, bounded-error and approval-required changes using C8. Resolve obsolete bit-exact-only experiment comments against the current product goal; preserve required discrete mechanics.
4. List baseline known bugs and missing native owners by scenario. Do not use missing output to claim a completed full-content performance result.

**Required tests/evidence:** T-MEAS identity tests; parse manifest; independently verify ROM/ELF/config pairing and non-fast-logic runtime settings.

**Work or dependency retired:** Ambiguous baselines and repeated rediscovery; no runtime saving claimed.

**Done:** One reproducible identity and explicit policy/coverage scope are linked from P2-2p8.

**Stop/revert:** Missing derived inputs or a mismatched ELF block the measurement; do not silently substitute another ROM.

#### N00.02 — Reproduce work, cadence and resource populations

**Depends on:** N00.01

**Edit/inspect boundary:** `scripts/verify-p2-four-fighter-stress.ps1`; `scripts/verify-battle-playable-realtime-harness.ps1`; `scripts/census-tick-hud-p95-set.py`.

**Implementation sequence**

1. Run the current source-normal four-kind stress with existing defaults: frame-1 identity and frames 2–1973 for 1,972 populated timing samples. Use unique output paths and the existing ring collector.
2. Collect runtime roster/mask, gameplay clock, item override defaults, positive native output, pool/heap/resource witnesses and timing in the same run.
3. Calculate WORK-H from each row, verify accounting identities, and report P50/P95/P99/max, FPS, 2/3/4/5+ VBlank counts and consecutive late presents. Keep cold transition/lifecycle windows separate and test them separately.
4. Run the natural two-fighter regression using the required integrated profile when appropriate. Preserve current quality/audio/configuration; do not disable features for the baseline.

**Required tests/evidence:** T-MEAS populations and T-COVER engagement; retain raw rows and hashes. Confirm cycles/ticks ratio from timer and emulator instrumentation, not a remembered frequency.

**Work or dependency retired:** Invalid profiling populations; no runtime saving claimed.

**Done:** Comparable baseline rows with exact scope and independently reported correctness/performance verdicts.

**Stop/revert:** Stop on timing corruption, unexplained missing frames or unengaged fighters; investigate that cause before attribution.

#### N00.03 — Assign exclusive costs and concrete deletion candidates

**Depends on:** N00.02

**Edit/inspect boundary:** `scripts/census-tick-hud-p95-set.py`; `scripts/task37_softfloat_callers.py`; `src/nds/nds_renderer_native_fighter_production.c`; `src/port/renderer_adapter_matrix.c`.

**Implementation sequence**

1. Build one exclusive owner table from existing buckets and per-PC/caller data. Separate SRC/GCRA/SINT/SCPU nesting, intended idle, blocked service, GX backpressure and instrumentation.
2. Price successful packet replay preflight, packet miss/recording, static stage preparation, pose evaluation/publication, software float plus conversion callers, memory copying and filesystem events.
3. For each high-value candidate record exact repeated work, frequency, immutable/dynamic inputs, native replacement cost, memory cost and the test that proves it was removed.
4. Rank against both the P95-tail and cheapest-late-frame populations, then choose structural tasks that can plausibly address the measured deficit. Treat diagnostic work suppression only as an unqualified upper bound.

**Required tests/evidence:** T-MEAS accounting; sums of exclusive frame rows reconcile; no parent-plus-child or sum-of-percentiles budget.

**Work or dependency retired:** Unpriced optimization guesses and duplicate attribution.

**Done:** A bounded deletion ledger names the next highest-impact executable slices.

**Stop/revert:** If removable costs cannot explain the gap, widen the owner boundary; do not keep surveying individual tiny symbols.

#### N00.04 — Implement a real product-performance verdict

**Depends on:** N00.02

**Edit/inspect boundary:** `scripts/verify-p2-four-fighter-stress.ps1`; `scripts/lib/harness-registry.ps1`; `scripts/census-tick-hud-p95-set.py`; `proposed scripts/check-p2-native-performance.py`.

**Implementation sequence**

1. Add a pure host evaluator for existing row/coverage artifacts, or extend the existing evaluator without changing the meaning of its prior reports. Publish explicit correctness, coverage, work-gate and cadence-gate verdicts.
2. Adopt the existing percentile convention consistently and store the rank/population. The 1,600-frame rank-80 sizing convention is not hardcoded into the 1,972-sample run.
3. Make a required product mode fail for P95 above the configured gate, two-VBlank cadence below 95%, missing coverage or invalid identities. Keep diagnostic candidate runs able to report RED without pretending implementation failed to execute.
4. Add final shipping-configuration evaluation and prevent an aggregate green capacity message from being used as performance acceptance.

**Required tests/evidence:** T-MEAS negative fixtures: over-budget-only, cadence-only, omitted slow frames, duplicate/malformed rows, missing identity, unengaged CPU, missing required owner; all fail the appropriate verdict.

**Work or dependency retired:** The acceptance gap where correctness-only passes are read as FPS closure.

**Done:** A deliberately slow valid ROM/report is explicitly performance RED; a valid fixture at the documented limits grades consistently.

**Stop/revert:** Do not tighten the product contract silently or exempt known slow frames. Fix evaluator definitions before using it to rank changes.

#### N00.05 — Freeze semantic and visual regression fixtures

**Depends on:** N00.01

**Edit/inspect boundary:** `scripts/fighters/test_pose_clock_differential.py`; `scripts/fighters/check_native_owner_geometry_closure.py`; `docs/p2/BUG_NOTES.md`; `docs/BUGS.md`; `proposed native optimization fixture manifest`.

**Implementation sequence**

1. Select controlled state fixtures for movement, hitlag, grabs/throws, platform boundaries, copy/morph/entry states, depth-intersecting effects and scene transitions.
2. Derive expected outcomes from the relevant BattleShip source and source assets; use independent native geometry closure rather than candidate output as the oracle.
3. Specify tolerance class per field, exact event ticks/order, resource identities, expected visible owners and audio cue timing.
4. Add explicit negative cases for no-op rendering and equal-count wrong-resource substitution. Fixtures should run cheaply before full-match qualification.

**Required tests/evidence:** T-NUM, T-CLOCK, T-GEOM, T-LIFE and T-COVER fixture sanity; every expected route has a positive witness.

**Work or dependency retired:** Vague correctness claims and tests that only prove zero error counters.

**Done:** Known tricky states have reproducible triggers and defined outcomes, independent of the implementation being replaced.

**Stop/revert:** An untriggered CPU move is not evidence; use source-controller playback or a clearly labeled deterministic state fixture.



---


<a id="doc-03-tcm-and-build"></a>

## N01 — ITCM/DTCM policy, build boundaries and early debloating

**Purpose:** create room for a smaller runtime without confusing source formatting with actual linked/executed savings. This package is bounded early work; final packing is N09.

The pinned linker groups `*.32.o` text/rodata into ITCM and data/BSS into DTCM. Renderer attributes also create shared input sections. The prior diagnostic 4,650-byte zero-execution population is not proof of dead code; interrupts, startup, another fighter, an uncommon move and packet misses must be covered. Current DTCM data is constrained to `0x02ff3000`. [S07, S08, S05]

**Explicit rule:** retain exception vectors, startup/load metadata, ARM/Thumb interworking and ABI-required system code. Do not exchange correctness for a prettier ITCM total.

#### N01.01 — Produce byte-accurate linked ownership

**Depends on:** N00.02

**Edit/inspect boundary:** `linker/nds_hot_text.ld`; `scripts/check-renderer-itcm-placement.ps1`; `scripts/check-task20-dtcm-layout.ps1`; `scripts/compare-elf-sections.py`.

**Implementation sequence**

1. Enumerate each ITCM/DTCM output byte by input section, object/member, symbols/aliases, literal pools, alignment and veneers from the actual matching ELF/map.
2. Classify explicit placement versus filename wildcard; track .text.hot and .text.hot.draw separately as main-RAM placement, not additional hardware memory.
3. Classify CPU-only state versus DMA/IPC/graphics-facing data and include stack reserve/low-water obligations.
4. Join execution/caller coverage as a candidate ranking only. Preserve a separate category for necessary unobserved cold/system paths.

**Required tests/evidence:** T-TCM inventory reconciliation: union of non-overlapping ranges equals section size; aliases never double-count; unnamed bytes are explained.

**Work or dependency retired:** Opaque group placement and invalid recoverable-byte claims.

**Done:** Every byte has an owner/class and each proposed eviction identifies its actual input-section granularity.

**Stop/revert:** Do not use a different debug ELF or sum alias sizes; fix identity/granularity before placement changes.

#### N01.02 — Decouple instruction mode from residency

**Depends on:** N01.01

**Edit/inspect boundary:** `linker/nds_hot_text.ld`; `Makefile`; `src/nds/nds_renderer_preamble.c`; `include/nds/nds_task37_itcm.h`.

**Implementation sequence**

1. Create explicit kernel section naming, e.g. .itcm.nds.<kernel>, independent of target("arm")/target("thumb") and optimization level. Only port-owned selected kernels use it.
2. Replace accidental .32.o placement with an explicit retained-system allowlist plus deliberate kernel/data sections. Inspect library/OS archive members first; do not globally evict all .32.o code/data.
3. Keep output load/start/end symbols and startup copy semantics valid, including reserved vectors and contiguous LMA assumptions.
4. Update placement checkers to inspect real sections rather than obsolete filenames, and fail on new accidental placement. Confirm the linked instruction bodies where placement-only equivalence is claimed.

**Required tests/evidence:** T-TCM boot/IRQ/context/interworking and link-map checks; T-LIFE shell→battle→results; final hard-on timing comparison.

**Work or dependency retired:** Automatic code/data residency solely because an object was compiled ARM.

**Done:** ISA can change without moving unrelated globals; intended current residents remain explicit and layout effects are measured.

**Stop/revert:** Revert the policy slice on stack/IRQ/boot regression; do not paper over it by raising a memory region.

#### N01.03 — Separate shipping observation from safety

**Depends on:** N01.01, N00.05

**Edit/inspect boundary:** `src/nds/nds_ft_pose.c`; `src/nds/nds_renderer_native_owners.c`; `src/import/battleship_ftcomputer.c`; `Makefile`; `scripts/verify-p2-four-fighter-stress.ps1`.

**Implementation sequence**

1. Inventory volatile counters, witness arrays, formatting, hashing, detailed timing and oracle state that remain in shipping code. Classify each as safety, product state, coverage witness or debug-only.
2. Keep bounded first-failure safety records and necessary admission/range checks. Compile detailed per-joint/per-vertex observers out of low-instrumentation shipping where not required for correctness.
3. Provide a compact versioned publication block for verifier-required observations instead of pinning every historical diagnostic global into every target. Update harness readers and missing-symbol behavior together.
4. Compare instrumented and shipping build identities; do not credit visible-HUD removal against WORK-H twice. Remove no source gameplay work in this commit.

**Required tests/evidence:** T-MEAS equivalent workload and T-COVER positive engagement with lean witnesses; no missing-symbol waiver; disassembly confirms observer instructions are absent where disabled.

**Work or dependency retired:** Unnecessary shipping observer loads/stores and obsolete debug state, not safety checks.

**Done:** Shipping and evidence targets have explicit observation contracts and final shipping timing remains independently measured.

**Stop/revert:** A verifier that can no longer prove four engaged fighters blocks the change; restore a compact witness, not all historic telemetry.

#### N01.04 — Split the first measured cold renderer tails

**Depends on:** N00.03, N01.02

**Edit/inspect boundary:** `src/nds/nds_renderer_native_common.c`; `src/nds/nds_renderer_native_owners.c`; `src/nds/nds_renderer_native_fighter_production.c`.

**Implementation sequence**

1. Choose a live large ITCM body whose cold binding/error/setup paths are identified, such as native stage begin/commit responsibilities. Record common-path inputs and immutable invariants.
2. Extract the cold responsibility into a noinline main-RAM function without adding a common-case call. Keep hot ARM/Thumb and optimization settings unchanged initially.
3. Move invariant processing to existing admission/spawn/status boundaries only after enumerating all mutations that can invalidate it.
4. Check emitted hot bytes including literals/veneer costs; measure the reclaimed-space baseline before filling it with gameplay code.

**Required tests/evidence:** T-TCM exact linked shrinkage; T-GEOM and T-LIFE uncommon routes; full-frame A/B rather than instruction-count prediction.

**Work or dependency retired:** Cold setup/error code carried inside a hot input section and proven repeated immutable work.

**Done:** A specific kernel is smaller and retains required behavior; reclaimed bytes and net time are recorded separately.

**Stop/revert:** Reject a split whose hot call/branch/cache penalty outweighs its benefit; do not repeat broad attribute-only splitting.

#### N01.05 — Remove dead build variants and create real module boundaries

**Depends on:** N01.03, N03.03

**Edit/inspect boundary:** `src/nds/nds_renderer.c`; `src/nds/nds_renderer_preamble.c`; `Makefile`; `src/nds/nds_renderer_dispatch_profile.c`.

**Implementation sequence**

1. For the converted native owner, separate cold binder, immutable generated data and small emitter into real compilation units where it reduces coupling. Explicitly own the remaining mutable GX state.
2. Remove obsolete successful-experiment switches and losing implementations for that qualified owner; retain a host oracle and a versioned native diagnostic route only while a current experiment needs it.
3. Update Makefile source membership and generator dependencies. A .c textual include converted to a real TU must not leave duplicate definitions or rely on formerly private statics.
4. Keep unconverted required content paths until their own replacement passes; source-file size alone does not establish a safe deletion.

**Required tests/evidence:** T-BUILD target/configuration matrix, symbol uniqueness, native-only linking and generator-staleness checks; compare real text/data bytes.

**Work or dependency retired:** Retired routes, accidental duplicate object inclusion and module-global coupling in completed slices.

**Done:** The converted path has one shipping implementation with narrow inputs and no stale build switch resurrecting a forbidden route.

**Stop/revert:** Do not refactor every file simultaneously or remove a reachable sibling based on one-owner coverage.

#### N01.06 — Use reclaimed ITCM only for measured winners

**Depends on:** N01.04

**Edit/inspect boundary:** `linker/nds_hot_text.ld`; `include/nds/nds_task37_itcm.h`; `scripts/census-icache-placement.py`.

**Implementation sequence**

1. Rerank current unplaced small gameplay/pose/native kernels against the post-shrink ELF and current frame populations.
2. Test explicit placement with unchanged operation/ISA where possible; count displaced bytes, literal pools, interworking and added calls.
3. Keep individually proven winners, then test the combined pack. Reserve instrument feasibility but do not preserve unused space for its own sake.
4. Leave final repacking to N09 after structural work. Record enabling gains honestly; this task does not claim to solve the million-tick deficit.

**Required tests/evidence:** T-TCM placement A/B and final hard-on whole-frame validation; startup and all supported scene boundaries.

**Work or dependency retired:** Avoidable instruction-fetch stalls for the selected remaining kernels.

**Done:** New residents improve measured whole-frame behavior and fit the actual byte/stack budget.

**Stop/revert:** A prediction based only on stall density is not acceptance; revert a combined pack that loses.



---


<a id="doc-04-residency-and-assets"></a>

## N02 — Memory recovery and deterministic native asset admission

**Purpose:** make precomputation usable on the original DS without replacing CPU work with mid-fight storage stalls. The current residency contract requires no mandatory battle/motion/texture demand reads after GO; BGM is a declared exception with its own service policy. [S06]

### Representation selection before baking

For each asset family compare existing representation, compact native tracks plus constant channels, selected local-matrix precompute, and full samples. Publish ROM bytes, permanent resident bytes, startup scratch, transition peak, per-frame decode/evaluation ticks, and remaining mandatory storage reads. Choose on combined cost, not ROM size alone.

Four fighters × 32 joints × 60 samples × 48-byte matrices is 368,640 bytes for only one second; it is an illustration, not this game's measured bank. Large ROM allowance does not make all those matrices resident. Store reusable local data and dynamic inputs separately. Shared same-kind fighter banks are immutable; per-instance cursors and packet patches remain distinct.

### Minimum bank contents

The generated admission plan must include all source-reachable motion/event variants, model parts, both reachable detail modes, costumes/material frames, copy/morph/entry states, weapons/items/summon children and stage-dependent assets for the selected scene. It need not resident-load the union of every game scene. A mid-fight wave or copy path is not a free loading boundary.

#### N02.01 — Inventory simultaneous and transient resource sets

**Depends on:** N00.02

**Edit/inspect boundary:** `docs/p2/P2-texture-residency.md`; `docs/p2/P2-1c-vram-map.md`; `src/nds/nds_renderer_assets.c`; `src/nds/nds_ftanim_track.c`; `src/nds/nds_battlepack_anim.c`.

**Implementation sequence**

1. Build exact source-derived required sets per scene/profile: fighter kinds/instances/copy possibilities, stage/hazards, items/children, UI/VFX, motions and audio cue policy.
2. Classify ROM/code, ARM9 heap/arena, ARM7/shared storage, TCM, texture/palette/BG/OAM, graphics scratch and per-instance packet memory separately.
3. Include load→commit transient peaks and retained old-scene lifetimes, libc reserve, stacks, object pools and worst concurrent cue/particle bursts.
4. Compare required/admitted/excluded identities, not equal counts; emit the first failed constraint with exact resource identity.

**Required tests/evidence:** T-RES set-difference fixtures; missing child and equal-count wrong-resource mutation both fail; runtime admission/pool witnesses from same baseline.

**Work or dependency retired:** Anonymous allocation guesses and implicit post-GO demand assumptions.

**Done:** Each selected scene has a concrete bank/heap plan and known deficits, including uncommon states.

**Stop/revert:** An overfull plan is not solved by marking required content optional or lowering legal pool capacity.

#### N02.02 — Recover memory from actually retired representations

**Depends on:** N02.01, N01.03

**Edit/inspect boundary:** `src/nds/nds_renderer_preamble.c`; `src/nds/nds_renderer_assets.c`; `src/nds/nds_ft_pose.c`; `src/port/renderer_adapter_matrix.c`; `Makefile`.

**Implementation sequence**

1. Identify duplicate source/native buffers, retired recorder storage, repeated matrix copies, diagnostic arrays and scene-global reservations with live owners.
2. For each candidate prove no remaining consumer across active and cold scene states. Reclaim per-scene storage only after the prior owner is quiescent.
3. Where consumer retirement depends on N03/N05, reserve the reclamation task but do not free early. Split independently safe removals from dependent removals.
4. Price memory released to the actual match arena and runtime low-water, not merely total ELF BSS shrink. Avoid retaining a second copy for convenience.

**Required tests/evidence:** T-RES allocation/lifetime tests, cold first-use and repeated transitions; same-run heap/graphics/pool bounds; no stale handles.

**Work or dependency retired:** Duplicate or dead state proven unused; precise bytes credited only after runtime availability is confirmed.

**Done:** Recovered bytes are available to the intended bank at its allocation boundary, with safety reserves preserved.

**Stop/revert:** Keep a necessary buffer until its consumer retires; moving it to another hidden heap is not reclamation.

#### N02.03 — Compile compact motion and resource banks

**Depends on:** N02.01, N04.02

**Edit/inspect boundary:** `scripts/fighters/generate_nds_native_owners.py`; `scripts/_paths.py`; `src/nds/nds_ftanim_track.c`; `proposed native bank generator/loader extensions`.

**Implementation sequence**

1. Extend existing generators with explicit version/hash/table metadata and fixed numeric payloads. Fold constants and deduplicate identical immutable channels without aliasing mutable state.
2. Choose compact keys/coefficient tables and selected precomputed local transforms from measured bank/evaluator tradeoffs. Include event metadata but keep gameplay event semantics independently validated.
3. Validate offset/count/stride/alignment/integrity in host output and target admission. Generated outputs have deterministic ordering and no timestamps/host addresses.
4. Update generator inputs, Makefile dependencies and staleness checks in the same commit; keep source references read-only.

**Required tests/evidence:** T-BANK round trip/corruption/version/overflow tests; independent source-derived motion/geometry/resource completeness; deterministic regeneration.

**Work or dependency retired:** Runtime source format decoding, invariant coefficient work and redundant immutable payloads for converted banks.

**Done:** Banks are smaller/resident enough and directly consumable with a documented version transition.

**Stop/revert:** Reject a representation that fits ROM but cannot meet runtime resident/transient limits or event fidelity.

#### N02.04 — Implement transactional admission and locked epochs

**Depends on:** N02.02, N02.03

**Edit/inspect boundary:** `src/nds/nds_renderer_assets.c`; `src/nds/nds_ftanim_track.c`; `src/nds/nds_battlepack_anim.c`; `src/port/renderer_adapter_stage.c`.

**Implementation sequence**

1. Validate the complete new scene plan and reserve all permanent/transient resources before publishing any live handles.
2. Prepare/upload at the legal boundary; bind all native owners; commit one generation only after full success. On failure clean partial allocations and restore the prior valid scene or established failure UI.
3. Lock the battle epoch: no mandatory motion/texture create/convert/evict/demand-read work after GO. Pre-admitted palette/material animation remains legal.
4. Resolve scene transitions and copy/morph/wave changes from admitted data, using C4 mutation ownership and C5 texture/packet lifetime rather than a gameplay-time LRU.

**Required tests/evidence:** T-RES failure injection at every admission phase; T-LIFE cancelled loads, rematch, same-address arena reuse; class-specific post-GO counters must remain zero.

**Work or dependency retired:** Mandatory demand reads and repeated resource discovery in converted locked scenes.

**Done:** Complete native content is admitted atomically and stays valid through all qualified battle states.

**Stop/revert:** Fail closed at admission on missing resources, but leave product coverage OPEN; containment is not a successful port.

#### N02.05 — Separate CSS, battle and transition residency

**Depends on:** N02.04, N03.02

**Edit/inspect boundary:** `src/nds/nds_menu_shell_css.c`; `src/nds/nds_renderer_assets.c`; `src/nds/nds_menu_shell_router.c`; `docs/p2/P2-1c-vram-map.md`.

**Implementation sequence**

1. Use compact preview-specific banks for visible/selectable CSS instances without reserving every fighter gameplay closure simultaneously.
2. Keep selected-pose and first-use animation resident/prepared before selection presentation; bind preview state separately from battle topology.
3. Plan explicit bank retirement/handoff for CSS→stage select→battle→results→CSS and 1P introductions/wave replacements.
4. Keep transition presentation responsive under its own cadence budget and avoid state leakage from a previous scene.

**Required tests/evidence:** T-UI fast selection/selected poses for all roster entries, T-LIFE repeated menu/match loops, T-RES transient peaks and cancellation.

**Work or dependency retired:** Unnecessary whole-roster gameplay residency in previews and accidental cross-scene reservations.

**Done:** Preview and battle resource sets are independently complete, bounded and correctly retired.

**Stop/revert:** Do not hide unresident fighters or reuse the last rendered preview while claiming live output.

#### N02.06 — Qualify no-demand-read and bank capacity closure

**Depends on:** N02.04, N02.05, N03.10, N05.08

**Edit/inspect boundary:** `scripts/verify-p2-four-fighter-stress.ps1`; `docs/p2/P2-texture-residency.md`; `proposed admission manifest artifacts`.

**Implementation sequence**

1. Exercise cold motion changes, copy/morph/entry, rare item/summon children, simultaneous KO/respawn and long matches under the selected bank.
2. Classify every storage event by client and source. A direct NitroROM read remains a read; a cache miss remains a mandatory dependency unless proved optional/declared.
3. Validate independent VRAM placement/palette/view/atlas constraints and RAM peaks, not a single total-memory number.
4. Publish the scene closure and repeat for the measured worst CPU and worst memory configurations, which may differ.

**Required tests/evidence:** T-RES zero mandatory post-GO motion/texture reads, exact required sets, no safety reserve breach; T-AUDIO declared services have no deadline loss.

**Work or dependency retired:** Residual mandatory storage dependency in qualified battle banks.

**Done:** A recorded resource-class report proves the locked admission contract for each accepted scene profile.

**Stop/revert:** No blanket zero-I/O claim when BGM streams; no accepted scene closure with an unclassified storage client.



---


<a id="doc-05-bound-renderer"></a>

## N03A — Bound native renderer and mutation-owned preparation

**Target:** a successful draw/replay consumes validated native bindings, not source-shaped state reconstruction. Sources [S07, S09, S10, S11, S12, S15, S17].

### Current seams and replacement direction

`ndsRendererExecuteNativeFighterOwnerProduction` selects tables and calls `ndsRendererNativePreflightProductionOwner` before `ndsFighterPacketTryReplay`. Measure the preflight internals before deleting them: call order alone does not prove the price or that every check repeats. The native TaruCann path is a concrete small-actor pilot; it is not a substitute for measuring a hot four-fighter path.

| Current responsibility | New owner/boundary | Steady-state residue |
|---|---|---|
| Asset pointer/range, table version and run topology validation | Scene bank admission | Generation identity only where live mutation can invalidate it |
| Root/model-part/foreign-table resolution | Spawn or actual topology/model change | Bound native root pointer/index |
| Texture/material immutable translation | Admitted resource binding | Native selected handle/words |
| Parent chain/type discovery | Topology bind | Dense parent index / fixed transform class |
| Per-frame matrix/material hash over whole objects | Mutation owner | Narrow generation/dirty set |
| Constant vertex/UV/normal conversion | Host generator | Native command payload |
| Dynamic pose/world/camera/billboard work | Native presentation preparation | Changed native matrices or texcoords |
| Fault reporting/accounting teardown | Cold failure helper | Bounded failure branch, not a large hot body |

The migration must preserve source-default state, unlit per-vertex color, alpha zero behavior, copied Kirby table ownership, split roots and source depth/order classes. Do not generalize from opaque Mario idle.

### Minimal runtime execution shape

```text
bind_on_real_change(instance, admitted_bank, topology, resource_epoch)
prepare_changed_inputs(instance, fixed_pose, world, camera, materials)
submit_bound_draw(instance)
```

These are semantic operations, not mandated function names. `bind_on_real_change` may be split across existing scene/status callbacks. It must not poll the source graph every frame to decide whether something changed. Preserve current native execution for unmigrated owners; retire it per owner only after replacement coverage.

#### N03.01 — Partition preflight into immutable and live checks

**Depends on:** N00.03, N00.05, N02.01

**Edit/inspect boundary:** `src/nds/nds_renderer_native_fighter_production.c`; `src/nds/nds_renderer_assets.c`; `src/nds/nds_renderer_native_common.c`.

**Implementation sequence**

1. Walk each preflight operation and record its inputs, mutation owner, lifetime and failure output. Classify bank-immutable, instance-topology, material/residency or genuinely per-draw.
2. Price successful replay hits separately from misses/first use and capture which validation entries actually run.
3. Specify the bound descriptor from C3 using existing native tables wherever possible. Preserve copied/foreign-root ownership and distinguish slot, fighter kind and instance identity.
4. Add tests for every moved validation condition before relocating its execution boundary.

**Required tests/evidence:** T-BIND corrupt bank/foreign root/stale epoch/wrong model variant tests; replay-hit counters and measured perflight cost; no candidate branch changes yet.

**Work or dependency retired:** Only immutable checks demonstrated to repeat are marked for removal; no speculative bulk validation deletion.

**Done:** A validation/mutation matrix states exactly what remains on the hot path and why.

**Stop/revert:** Any unowned mutation remains conservatively checked until its writer is identified; do not substitute an optimistic pointer comparison.

#### N03.02 — Implement binding and explicit invalidation

**Depends on:** N03.01, N04.02

**Edit/inspect boundary:** `src/port/renderer_adapter_matrix.c`; `src/import/battleship_ftmain.c`; `src/nds/nds_renderer_assets.c`; `proposed include/nds/nds_native_binding.h`.

**Implementation sequence**

1. Add small bound-instance records or extend current records; keep immutable bank references distinct from per-instance mutation state.
2. Hook genuine scene, spawn, status topology, model-part, copy/morph, costume and resource-epoch changes using C4. Verify all current invalidation callers before narrowing them.
3. Build parent/root/material bindings once per relevant change and publish only after all referenced resources and patch metadata are valid.
4. Expose fixed native transforms through a narrow API. During migration explicitly mark any legacy producer bridge and its cost; no new floating conversions inside the emitter.

**Required tests/evidence:** T-BIND same-root reparent, same-address reuse, four identical fighters with different costumes, Kirby copy, Samus morph, rapid status changes and generation wrap.

**Work or dependency retired:** Repeated immutable root/material/parent discovery for bound instances.

**Done:** All bound fields have a mutation owner; tests invalidate the right scope without invalidating unrelated instances.

**Stop/revert:** A stale attachment or valid-looking wrong-generation handle blocks the migration; restore conservative ownership until fixed.

#### N03.03 — Prove the contract on one small actor

**Depends on:** N03.02

**Edit/inspect boundary:** `src/nds/nds_renderer_native_owners.c`; `src/port/renderer_adapter_matrix.c`; `existing TaruCann generated tables`.

**Implementation sequence**

1. Convert TaruCann or an equally small currently admitted native actor to bind invariant vertices, material, texture view and transform class at admission/spawn.
2. Remove the per-submit construction of generic traversal/configuration/vertex arrays whose contents are now bound or generated.
3. Keep its dynamic world motion and source camera/projection semantics; prove the translation-unit scale and near-plane treatment independently.
4. Preserve actor visibility, winding, alpha, depth and required lifetime. Keep the shared executor small rather than adding a second actor-specific generic framework.

**Required tests/evidence:** T-BIND actor spawn/despawn/bank change; T-GEOM source corner order/material/depth plus visible moving barrel fixture; measure whole actor CPU/scratch.

**Work or dependency retired:** Generic state construction and immutable conversions on every pilot actor draw.

**Done:** The native actor is complete with a smaller hot path and no extra live mirror; pilot interface proof is separate from campaign speed claims.

**Stop/revert:** Do not expand actor coverage until the contract is correct; do not claim significant four-CPU savings from an actor absent in that baseline.

#### N03.04 — Move a hot fighter to the bound hit path

**Depends on:** N03.02, N03.03

**Edit/inspect boundary:** `src/nds/nds_renderer_native_fighter_production.c`; `src/nds/nds_renderer_preamble.c`; `src/port/renderer_adapter_matrix.c`.

**Implementation sequence**

1. Select a currently expensive admitted fighter/variant from the reproduced four-kind profile, not an easy but unrepresentative idle-only path.
2. Route a valid bound replay hit past retired immutable preflight work while preserving live generation/count/capacity checks. Reuse existing packet correctness for split roots and texgen.
3. Factor model-local, world, camera and material updates so camera-only motion does not trigger pose/topology rebuilds; directly reference stable matrices instead of copying full workspaces.
4. Test the full selected move/entry/damage/capture lifecycle, measure hit/miss behavior and delete the completed owner’s old binding path after acceptance.

**Required tests/evidence:** T-BIND and T-GEOM full pilot state tour; T-MEAS hit/miss/patch/copy bytes plus same-ROM and hard-on timing; four-slot positive output.

**Work or dependency retired:** Repeated successful-hit immutable preflight and source-shaped matrix/material setup for the pilot fighter.

**Done:** Whole-owner cost improves in the real stress configuration with stable memory and complete native output.

**Stop/revert:** Neutral micro-cost with a large new resident record is not enough; redesign the bound-state boundary before roster rollout.

#### N03.05 — Specialize transform and render classes

**Depends on:** N03.04, N04.03

**Edit/inspect boundary:** `src/port/renderer_adapter_matrix.c`; `src/nds/nds_renderer_native_common.c`; `src/nds/nds_renderer_native_owners.c`.

**Implementation sequence**

1. Classify all reachable matrix callbacks from source and current adapters: affine TRS, source orientation replacement, billboard, camera-dependent special cases, cross-root binding and procedural attachments.
2. Generate/bind the class once and use a small native fixed kernel per actual class. Preserve scale accumulation, coordinate handedness, translation retention and projection semantics.
3. Separate render classes for ordinary depth-tested, translucent, painter/no-Z and exceptional depth behavior; preserve source ordering inside non-commutative groups.
4. Use CPU matrices only where gameplay or unsupported GX semantics need them; never build a full CPU hierarchy solely to discard it after equivalent GX composition.

**Required tests/evidence:** T-XFORM class-by-class oracle, degenerate axes, reflection/negative scale and near-plane cases; T-DEPTH shield/ring/stage ordering; required sockets remain current.

**Work or dependency retired:** Runtime callback/type discovery and generic whole-state branches inside converted emitters.

**Done:** Every reachable transform/render class is native and bounded; unsupported required classes are explicit blockers, not silent fallback.

**Stop/revert:** Do not collapse all classes to generic TRS or globally reorder translucency for material batching.



---


<a id="doc-06-packet-compiler"></a>

## N03B — Ahead-of-time native GX packet compiler

**Target:** replace live recording/translation for qualified owners with directly executable native DS command templates plus typed dynamic patches. The existing native owner generator and independent geometry closure checker are the starting point, not throwaway work. [S19, S29, S07, S09]

### Packet format and algorithm

The host compiler takes source-validated native runs/epochs and emits DS command words, with an independent decoder verifying the result. Pack command opcodes/parameters according to the DS FIFO format, including zero-parameter commands and grouped opcode words. Do not treat a list of raw register values as a valid packed list by coincidence. Retain correct triangle/strip orientation and the runtime BEGIN policy.

Each packet directory entry contains a variant ID, command span, required matrix slots, native state preamble and typed patch spans. Proposed patch kinds: affine matrix payload, projection/view block, polygon attributes, admitted texture view, palette base/selection, per-instance color/alpha and live texcoord. Validate opcode/span alignment and index bounds at admission. Add a new kind only for a real producer; no generic runtime expression evaluator.

Constant commands are shared. Dynamic model variants are a bounded generated set keyed by actual topology/visibility semantics, not pose × camera × costume × every material-frame Cartesian products. Use skip spans or selected subpackets where they preserve valid primitive/matrix state. Opaque-only reordering requires equivalence proof; translucent and no-Z ordering stays explicit.

For each alternative record: resident command bytes, patch metadata, per-instance scratch, flush bytes, DMA/CPU submission setup, blocked time, matrix words, polygon/vertex use and new failure surface. Lower CPU time that exhausts geometry capacity or worsens FIFO/raster completion is not a successful packet conversion. Thirty-Hz presentation does not double per-submission geometry storage.

### Patch rules

A matrix change touches only its admitted payload spans. A camera change also touches dependent billboard/texgen spans, not every local pose channel. Visibility variants keep stack/store/restore balanced. Link’s live texgen patch and NDO6 unlit per-vertex colors must remain represented; mask packed class/alpha/flag bits rather than using the whole byte as a class. [S19, S20A]

The `READY` publication in C5 is the only route to submission. Avoid full packet copies on every frame unless measured memory/transport tradeoffs require them. Never patch a shared or in-flight buffer. CPU/GX source geometry stays DMA-visible, not in TCM.

#### N03.06 — Extend the generator with native packet payloads

**Depends on:** N03.01, N04.02, N00.05

**Edit/inspect boundary:** `scripts/fighters/generate_nds_native_owners.py`; `scripts/fighters/check_native_owner_geometry_closure.py`; `src/nds/nds_renderer_assets.c`; `proposed generated native packet sections`.

**Implementation sequence**

1. Add a native command emitter on the already source-validated run/epoch representation. Preserve original material/depth/transform dependencies and explicit unlit vertex color.
2. Generate bounded variant directories and typed patch spans with exact source indices. Reuse existing source identities and schema hashes; increment the native ABI only when necessary.
3. Build a separate decoder/checker that expands output to oriented triangles, material states and matrix routes and compares to the independent source path, not the emitter’s intermediate objects.
4. Emit command/resource/patch size reports and deterministic hashes for every admitted owner and both reachable detail settings.

**Required tests/evidence:** T-GEOM all six existing closures plus T-PACKET opcode/parameter/patch-boundary tests and corrupted span negatives.

**Work or dependency retired:** First-use native packet recording and invariant command translation for packet-qualified variants.

**Done:** Generated bytes and patch metadata have independent semantic and structural proof across the emitted owner set.

**Stop/revert:** A self-consistent truncated packet must fail source closure; do not equate a valid buffer with complete geometry.

#### N03.07 — Implement small typed patch kernels

**Depends on:** N03.06, N03.05

**Edit/inspect boundary:** `src/nds/nds_renderer_preamble.c`; `src/nds/nds_renderer_native_fighter_production.c`; `proposed native packet patcher`.

**Implementation sequence**

1. Implement fixed-width, contiguous patch loops using validated descriptors; do not branch through a large generic expression engine per vertex.
2. Use generation/dirty dependencies to patch matrices, material words, visibility and texgen only when their inputs change.
3. Keep the packet buffer state machine explicit and reserve per-instance ownership. Align shared/writeback ranges safely and count bytes copied/flushed.
4. Retain the existing Link texgen math until an independently verified native-hardware substitution is faster; absence of a named library helper is not proof of equivalent texgen.

**Required tests/evidence:** T-PACKET only-dynamic-word mutation, adjacent-line canaries, same-kind four-instance isolation, rapid camera/costume changes, split-root and Kirby foreign-bank cases.

**Work or dependency retired:** Per-frame invariant command construction, unnecessarily broad dirty patching and avoidable full packet copies.

**Done:** Patch work is bounded by actual changed inputs and cannot write outside validated spans or into an in-flight buffer.

**Stop/revert:** Any stale frame, cross-instance color leak or command-state dependency is a correctness failure even when geometry counts match.

#### N03.08 — Qualify native packet transport and GX ownership

**Depends on:** N03.07

**Edit/inspect boundary:** `src/nds/nds_renderer_preamble.c`; `src/nds/nds_renderer_dispatch_profile.c`; `existing GX/DMA submission seam`.

**Implementation sequence**

1. Identify the actual toolchain/DMA channel owner, FIFO submission API, cache maintenance and completion route in the pinned build.
2. Compare packed CPU submission and DMA transport on representative small and large packet sizes including setup, flush, wait and bus contention, not just copy-loop time.
3. Prove matrix-store/restore scheduling and resource lifetime across all four fighter packets plus stage/effects. Ensure previous asynchronous work cannot leak state into the next owner.
4. Keep the measured winner by packet/work class only when a class distinction is justified; otherwise retain one simple transport route. Do not add an unbounded transport scheduler.

**Required tests/evidence:** T-PACKET in-flight overwrite/early-free negatives, T-TCM no inaccessible source buffers, T-GPU capacity/backpressure and T-MEAS wall-time.

**Work or dependency retired:** Actual non-overlapped CPU submission work; no credit for hidden waits moved to another bracket.

**Done:** Whole-frame transport improves or the simpler equivalent route is retained, with valid hardware completion and no audio/service starvation.

**Stop/revert:** DMA-only microbench wins that regress total time are reverted; do not assume DMA and ARM9 main-memory work overlap freely.

#### N03.09 — Expand packets across native owners and special states

**Depends on:** N03.08

**Edit/inspect boundary:** `scripts/fighters/generate_nds_native_owners.py`; `src/nds/nds_renderer_native_fighter_production.c`; `src/nds/nds_renderer_assets.c`; `source-derived owner/variant manifest`.

**Implementation sequence**

1. Convert by measured hot owner groups, including Donkey/Samus/Link/Kirby stress states, then remaining roster and duplicated-kind combinations.
2. Include entry, damage, hidden/replacement parts, morph, copy hats, throws, reflected projectiles and both reachable detail configurations.
3. For each group run independent source geometry/state closure and a source-controller state tour before deleting its recording route.
4. Track required native gaps explicitly. New generated code should share small hardware kernels and immutable data, not duplicate a full renderer per move.

**Required tests/evidence:** T-COVER owner/variant/child coverage plus T-GEOM, T-DEPTH and T-LIFE; runtime draws prove native commands executed, not only lookup success.

**Work or dependency retired:** Live recording/immutable preparation across the converted owner set.

**Done:** Every converted required variant has complete native output and a tested packet/lifetime path.

**Stop/revert:** One successful idle or CPU state does not close a fighter; missing special states block owner retirement.

#### N03.10 — Retire replaced renderer state and price the integrated path

**Depends on:** N03.09, N01.05

**Edit/inspect boundary:** `src/nds/nds_renderer_preamble.c`; `src/nds/nds_renderer_native_common.c`; `src/nds/nds_renderer_native_fighter_production.c`; `Makefile`.

**Implementation sequence**

1. Remove obsolete recorder objects, source-shaped state, old binding branches and packet-copy buffers only for fully qualified converted families.
2. Return recovered memory to the admitted scene bank and update explicit ITCM/DTCM ownership. Verify no other owner relied on the retired static/global.
3. Remove experimental route toggles from shipping, regenerate assets and build the final hard-on target.
4. Run the integrated four-way stress and natural shell regression; record per-owner and whole-frame differences plus residual costs. Feed newly freed memory into N02 rather than retaining stale reserves.

**Required tests/evidence:** T-RETIRE symbol/caller/data reachability, T-RES runtime low-water and T-MEAS integrated re-ranked results; T-GEOM required content unchanged.

**Work or dependency retired:** Completed renderer migration’s old preparation/recording infrastructure and redundant storage.

**Done:** One authoritative native route remains for converted content, measured in the real shipped shape.

**Stop/revert:** Retain no permanent old/new route pair solely for convenience; unconverted dependencies remain explicit and separate.



---


<a id="doc-07-fixed-numerics"></a>

## N04 — Whole-domain fixed numerics

**Endpoint:** fixed/integer arithmetic through producers, state, consumers and target-side services. Moving IEEE operations into integer helper code or renaming `f32` does not satisfy it. The inspected pose clock currently uses integer binary32 arithmetic, and fixed pose values coexist with source float fields. [S13, S14, S27]

### Numeric-domain register to create

Each row must contain: source field/function; semantic unit; current storage/type; all writers/readers; min/max from source plus observed range; unobserved legal cases; selected fixed format; intermediate width; rounding; overflow rule; exact downstream decisions; conversion sites; cold/interrupt/indirect callers; and the task that deletes the legacy storage. Observed min/max alone is not a proof of all source-legal values.

Use separate chains for world motion, affine matrices, camera/projection, collision predicates, damage/knockback, AI query values, particle state, material/texture animation, audio controls and menu transitions. Establish an explicit conversion seam between different **fixed** formats. Do not choose a new format independently in each leaf.

### Kernel requirements

Prefer compile-time constant folding/reciprocals and source-normalized integer fields. Use hardware divide/sqrt only for remaining required operations and respect the existing shared math-unit owner. Avoid a universal generic fixed library that adds type dispatch or saturated arithmetic to every operation. A few typed C/static-inline primitives plus carefully justified ARM kernels are sufficient.

Keep debug overflow assertions outside the release hot path when input bounds prove safety; retain unavoidable runtime guards at actual variable-domain seams. No unchecked 64-bit multiply-of-64-bit intermediates. Inspect ARM/Thumb instruction selection and helper/veneer costs. Exact same-function placement proof is distinct from approximate numeric replacement proof.

#### N04.01 — Build the producer-consumer and helper graph

**Depends on:** N00.01, N00.03

**Edit/inspect boundary:** `scripts/census-softfloat-callers.ps1`; `scripts/task37_softfloat_callers.py`; `include/nds/nds_f32_exact.h`; `src/import`; `src/nds`; `src/port`.

**Implementation sequence**

1. Inventory runtime floating arithmetic using types, preprocessed compilation units, compiler lowering and link/caller data. Include arithmetic, compares, conversions, libm, doubles/varargs and custom mantissa/exponent routines.
2. Start at all shipped scene entry points, callbacks, interrupts, service threads and startup/transition roots. Resolve function-pointer and weak-symbol targets conservatively.
3. Map helpers to their callers/domains and exact linked input sections. Include parent bridge work rather than only helper self-time.
4. Publish a per-domain field/use graph and list the smallest producer-to-consumer closure that removes a real chain.

**Required tests/evidence:** T-FLOAT audit fixtures: hidden typedef, constant-only literal, indirect libm call, integer IEEE implementation and float formatting are classified correctly.

**Work or dependency retired:** Unknown float families and leaf-only conversion proposals; no speed saving yet.

**Done:** Every known target arithmetic family and bridge has an owner and migration path.

**Stop/revert:** Unknown indirect targets remain blockers; do not declare zero runtime float from grep or the lack of FPU instructions.

#### N04.02 — Freeze units, ranges and numeric ABI

**Depends on:** N04.01, N00.05

**Edit/inspect boundary:** `proposed include/nds/nds_native_numeric.h`; `include/nds/nds_anim_fixed.h`; `include/nds/nds_r2_camera_fixed.h`; `source field-use register`.

**Implementation sequence**

1. For each high-priority chain derive source-legal input bounds and worst intermediate products, sums, shifts and denominators. Add dynamic scene/fighter extremes and rare procedural states.
2. Choose fixed widths/fraction bits from that proof and C2 candidate formats; pin coordinate handedness, world-to-GX scale, angle wrap and matrix layout.
3. Define named rounding/narrowing operations and class E/B comparison policy. Decide which constants/tables are generated host-side.
4. Publish one shared header/contract owned by the integrator. Record each remaining legacy API boundary and prevent independent agents from choosing incompatible representations.

**Required tests/evidence:** T-NUM extremes/rounding/overflow and compile-time sizeof/alignment checks; test values immediately on both sides of gameplay decision boundaries.

**Work or dependency retired:** Inconsistent numeric conventions and permanent float sandwiches.

**Done:** Chosen native types have explicit range/rounding proofs and agreed producer/consumer semantics.

**Stop/revert:** An unbounded source field requires a new representation or explicit admitted-domain proof, not guessed saturation.

#### N04.03 — Implement and qualify the small primitive set

**Depends on:** N04.02

**Edit/inspect boundary:** `include/nds/nds_r2_hwmath_unit.h`; `proposed native numeric primitives`; `existing fixed matrix/vector kernels`.

**Implementation sequence**

1. Implement only required multiply-accumulate, rounded shifts, vector operations, fixed ratio, angle lookup and normalization primitives with documented domains.
2. Use wide intermediates where proved necessary, reciprocal/lookup precompute for invariant operands, and eliminate unnecessary normalization/division before accelerating it.
3. Compile with the pinned ARM9 toolchain; inspect generated ARM/Thumb code, library calls, stack pressure and interworking. Compare a C reference and a target kernel independently.
4. Integrate overflow/corruption diagnostics at contract boundaries; do not add a global runtime saturation framework.

**Required tests/evidence:** T-NUM independently generated arithmetic corpus, exhaustive reduced domains, randomized legal extremes, divide-by-zero negatives and sanitizer/UB checks on the host.

**Work or dependency retired:** Software-float operations and redundant divides inside the selected fixed primitives.

**Done:** Primitives meet documented numeric semantics and target codegen obligations; speed is measured with their real callers.

**Stop/revert:** A primitive faster alone but slower after conversions/64-bit helpers is not adopted into the full chain.

#### N04.04 — Close residual camera, vector and transform chains

**Depends on:** N04.03

**Edit/inspect boundary:** `src/import/battleship_gmcamera.c`; `src/port/renderer_adapter_matrix.c`; `include/nds/nds_r2_camera_fixed.h`; `include/nds/nds_r2_hwmath_unit.h`.

**Implementation sequence**

1. Keep the already-fixed camera route; identify remaining source float publication, billboard/look-at, projection, material direction and conversion consumers.
2. Carry fixed values through those consumers and specialize affine versus perspective operations correctly. Avoid full 4×4 work for a proved affine-only chain.
3. Generate invariant trigonometric/reciprocal data where useful without baking live camera or gameplay-dependent state into giant tables.
4. Remove obsolete conversion wrappers only after all consumers of that chain use the native ABI.

**Required tests/evidence:** T-XFORM camera boundary/near-plane/degenerate-up/negative-scale and T-DEPTH regressions; same source-controlled camera trajectories.

**Work or dependency retired:** Residual mixed-representation camera/transform work, not the fixed route that already exists.

**Done:** Converted camera/transform producers and consumers are native end to end with no extra mirror.

**Stop/revert:** A numerical error that changes clipping/attachment/depth behavior must be corrected before acceptance.

#### N04.05 — Migrate constants and target asset numerics

**Depends on:** N04.02, N02.03

**Edit/inspect boundary:** `scripts/fighters/generate_nds_native_owners.py`; `existing stage/material/audio generators`; `Makefile`; `proposed numeric asset schema`.

**Implementation sequence**

1. Convert target-consumed float constants and source numeric payloads into the chosen fixed formats in host tooling; preserve original asset provenance and source values for oracle use.
2. Fail generation on overflow, undefined rounding, missing type metadata or an unsupported required asset rather than silently truncating.
3. Update bank hashes/versions and loader validation together. A bank schema change cannot be applied to an old resident image.
4. Check emitted code/data for runtime initializer/conversion routines that should have become constants.

**Required tests/evidence:** T-BANK deterministic generation, version mismatch and numeric boundary fixtures; target link audit finds no runtime constant-conversion initializer in converted banks.

**Work or dependency retired:** Load-time and per-use floating numeric conversion of host-convertible target assets.

**Done:** Fixed constants/data arrive ready for native runtime consumption across converted domains.

**Stop/revert:** Do not edit extracted source or reference decomp to make the converter accept invalid inputs.

#### N04.06 — Build a no-float regression gate

**Depends on:** N04.01, N04.03

**Edit/inspect boundary:** `proposed scripts/check-native-runtime-numerics.py`; `Makefile`; `scripts/verify-all.ps1`; `include/nds/nds_f32_exact.h`.

**Implementation sequence**

1. Combine source/type and compiler-output checks with ELF symbol/caller analysis; cover libgcc/libm helpers, custom IEEE code, weak aliases and indirect roots.
2. Initially report by converted domain with an explicit temporary migration allowlist. Each entry names consumer, reason and removal task; new entries require review.
3. At final closure reject all reachable target floating arithmetic in shipped configurations, not merely the profiled path. Keep host generators/oracles outside the target gate.
4. Include independent negative fixtures that introduce one prohibited arithmetic path without using a literal float keyword or a familiar helper name.

**Required tests/evidence:** T-FLOAT hidden typedef/varargs/indirect/custom IEEE/source-include/constant-folding fixtures; build graph proves host references are not linked into ROM.

**Work or dependency retired:** Future reintroduction of float and false success based only on helper-symbol disappearance.

**Done:** The checker distinguishes static data from arithmetic and detects all exercised negative patterns; domain migration is auditable.

**Stop/revert:** A finite trace or blacklist alone is insufficient proof; unresolved roots keep closure open.



---


<a id="doc-08-events-and-pose"></a>

## N05 — Event clock, compact animation and required pose

**Target:** source-correct discrete animation/gameplay events and compact fixed pose values, without per-joint binary32 emulation, repeated source parsing or permanent fixed→float→fixed publication. Sources [S13, S14, S15, S20].

### Event clock implementation decision

Do not assume one clock per fighter until the source has proved it. Joint scripts can have independent waits/loops, and the current engine publishes the last source-ordered writer to the GObj clock. Bind-time metadata must preserve event lanes and publication order where these are semantically observable.

Primary representation: an integer logic-tick stamp, explicit clip/event cursor, qualified fixed phase and optional rational remainder for ratio-derived speeds. For finite admitted constant-speed segments, generate source-derived integer event deadlines/transition metadata host-side. When a source operation changes speed or seeks, re-anchor according to the current logical state; do not recompute from an idealized absolute fraction and discard a behaviorally important accumulated remainder.

A concrete decision sequence is mandatory: enumerate actual speed constructors → reconstruct source event traces → test fixed phase plus residual → use generated deadline corrections for finite source regimes where needed → separately prove dynamic speed-change/rebound/interrupt cases. A generated schedule is not sufficient for an unrestricted input domain. If a case cannot yet be proven, it remains unmigrated and the **final no-float gate stays open**; do not retain integer IEEE emulation as the claimed endpoint.

Ordinary Q12 cannot represent 1/3 exactly. Nor does merely raising precision guarantee repeated-IEEE boundary identity. The existing source records real landing/animation boundary mismatches. Preserve those cases in the oracle rather than treating the historical comment as permission for a global epsilon. [S13]

### Pose representation

Compile per-clip static channels, active tracks, track-to-joint mapping, interpolation coefficients, transform class and source event dependencies. Use compact native values and indices, not a second linked AObj-like graph. Keep dynamic procedural channels explicit. Constant values need no per-frame evaluation or copy unless a consumer requires publication.

At topology/consumer change derive required-joint closure: gameplay root motion, hurt/hit volumes, grab/throw, weapon/item/attachment sockets and their ancestors. Update this closure at the existing required logic cadence. Visual-only evaluation may follow the existing sanctioned presentation cadence, but this campaign does not lower gameplay rates. Invisible fighters can still have gameplay-active joints.

Record local pose generation and world transform generation separately. Camera-only changes should not invalidate local pose. GPU matrices must not become the authoritative gameplay representation, and there is no GPU-readback loop to recover hitboxes.

#### N05.01 — Derive event lanes and timing semantics

**Depends on:** N04.02, N00.05

**Edit/inspect boundary:** `src/nds/nds_ft_pose.c`; `src/import/battleship_ftanim.c`; `src/import/battleship_ftmain.c`; `scripts/fighters/test_pose_clock_differential.py`.

**Implementation sequence**

1. Enumerate wait/frame/speed constructors and every writer/reader, including source sentinels, joint lanes, GObj last-writer publication and script event order.
2. Extract real constant, ratio-derived and dynamic speed cases with attach/seek/start-frame/loop/rebound/landing semantics.
3. Build source-driven traces with exact event tick/order and discrete status outcomes, not only pose-coordinate comparisons.
4. Specify fixed clock/remainder/deadline rules from those cases, including overflow/wrap and invalid speed handling.

**Required tests/evidence:** T-CLOCK retained Q12 mismatch examples, actual landing/rebound cases, speed transitions, pauses/hitlag and distinct joint lanes.

**Work or dependency retired:** Unsupported assumptions about one global exact fixed clock.

**Done:** Every admitted speed/event regime has an explicit source-derived behavior contract and planned native representation.

**Stop/revert:** Do not remove the existing clock until its semantic replacements pass; a rational clock is not automatically source-equivalent.

#### N05.02 — Implement fixed event stepping and deadline corrections

**Depends on:** N05.01, N04.03

**Edit/inspect boundary:** `src/nds/nds_ft_pose.c`; `include/nds/nds_f32_exact.h`; `proposed native event metadata/compiler`.

**Implementation sequence**

1. Implement integer event state and qualified phase/remainder update with a bounded number of source-ordered events per tick.
2. Generate deadline/transition metadata for proved finite source regimes; handle dynamic speed changes by the specified re-anchor rule, not a global epsilon.
3. Preserve hitlag freezing, multiple events in one tick, loops/end/changed/null states, interruptions and the correct GObj publication lane.
4. Compare events against the independent source oracle and isolate each mismatch to source regime and boundary before changing the rule.

**Required tests/evidence:** T-CLOCK exhaustive admitted finite regimes plus generated dynamic traces; exact event/order/status match, bounded runtime work and no integer overflow.

**Work or dependency retired:** Per-joint integer IEEE add/sub and repeated source wait arithmetic in qualified regimes.

**Done:** Native stepping passes the admitted semantic corpus and target-cost check; unresolved regimes remain explicitly open.

**Stop/revert:** Do not pad every duration by one tick or silently keep binary32 under a new name. Failure requires case-specific redesign.

#### N05.03 — Compile compact fixed tracks and constant channels

**Depends on:** N02.03, N04.03, N05.01

**Edit/inspect boundary:** `src/nds/nds_ftanim_track.c`; `src/nds/nds_ft_pose.c`; `include/nds/nds_anim_fixed.h`; `existing fighter motion generator`.

**Implementation sequence**

1. Compile static channels, active track maps, interpolation coefficients and interpolation classes using source values and approved quantization.
2. Generate bounded indices into resident data; use constant/linear/cubic-specific kernels without scanning inactive track classes.
3. Keep event metadata separate from visual samples. Procedural state, live facing/scale and attachments are runtime inputs, not baked as final world coordinates.
4. Compare compact coefficient/keys versus selective sampled locals for both memory and integrated CPU cost before choosing by family.

**Required tests/evidence:** T-POSE constant endpoints, interpolation extrema, per-track error/range proof, malformed spans and bank determinism; no event timing changes.

**Work or dependency retired:** Repeated source track interpretation, invariant coefficient generation and inactive/constant work beyond the existing masks.

**Done:** A compact resident representation replaces a named track path with measured whole-domain benefit.

**Stop/revert:** Do not claim the already-landed live/run masks again; a larger bake that increases mandatory reads is rejected.

#### N05.04 — Create topology-owned required-joint closures

**Depends on:** N03.02, N05.03

**Edit/inspect boundary:** `src/nds/nds_ft_pose.c`; `src/import/battleship_ftmain.c`; `src/port/renderer_adapter_matrix.c`; `gameplay joint/socket consumers`.

**Implementation sequence**

1. Enumerate gameplay consumers of each joint and their ancestor dependencies, including hurtboxes, attacks, captures, items and procedural attachments.
2. Build a bounded dense closure on bind/topology/consumer change and maintain source ordering where it matters.
3. Use explicit counts/bitsets sized for the actually admitted hierarchy; a future wide hierarchy requires a generated complete plan, not silently truncated 64-bit masks.
4. Keep visual-only work separate while preserving existing logic/event rates and gameplay-active invisible/offscreen joints.

**Required tests/evidence:** T-POSE ancestor closure, hidden active attacks, dynamically enabled heavy-item joint, copy/morph and wide-hierarchy negative fixtures.

**Work or dependency retired:** Repeated dependency discovery and unnecessary full hierarchy processing for gameplay consumers.

**Done:** Every required socket is current at its consumption tick with no dependence on whether the fighter was drawn.

**Stop/revert:** Missing one ancestor or treating visibility as gameplay inactivity blocks the change.

#### N05.05 — Make native pose authoritative through the draw pilot

**Depends on:** N05.02, N05.03, N05.04, N03.07

**Edit/inspect boundary:** `src/nds/nds_ft_pose.c`; `include/nds/nds_ft_pose.h`; `src/port/renderer_adapter_matrix.c`; `src/nds/nds_renderer_native_fighter_production.c`.

**Implementation sequence**

1. Publish native local pose/matrices directly to the bound draw/patch API; delete fixed→DObj-float→renderer-fixed conversions in that chain.
2. Distinguish local pose, world and camera-dependent generations, directly reference stable data and avoid per-draw whole-matrix workspace copies.
3. For unmigrated gameplay consumers, publish a temporary one-way legacy bridge once at the required logic boundary; name its deletion dependency and measure its cost.
4. Check the full pilot move/entry/capture/damage lifecycle and four-instance behavior before expanding the representation.

**Required tests/evidence:** T-POSE/T-XFORM/T-GEOM complete pilot states; audit shows converted draw never reads legacy float pose; T-MEAS includes temporary bridge cost.

**Work or dependency retired:** Pose-to-render conversion sandwiches, duplicated visual pose authority and redundant copied transforms.

**Done:** Native pose drives the full pilot render chain with a net integrated improvement; legacy gameplay bridge is explicitly not final closure.

**Stop/revert:** A native evaluator with all old publication and rebuild work still running is not a completed pose slice.

#### N05.06 — Share local/world transform results correctly

**Depends on:** N05.05

**Edit/inspect boundary:** `src/port/renderer_adapter_matrix.c`; `src/nds/nds_ft_pose.c`; `gameplay attachment and matrix consumers`.

**Implementation sequence**

1. Use one native local result per changed joint and one required world/socket result per consumer epoch.
2. Compose only required CPU ancestor paths; use generated/bound GX matrix routes for visual-only transforms when that eliminates rather than duplicates CPU work.
3. Preserve source special transform classes and dynamic scale/facing/attachment semantics; distinguish affine model work from camera projection.
4. Replace whole-tree per-tick invalidation with precise mutation-owned dirty descendants after all writers are accounted for.

**Required tests/evidence:** T-XFORM CPU socket/GX visual correspondence, camera-only/world-only changes, procedural attachments and negative scales; matrix count/bytes witnesses.

**Work or dependency retired:** Duplicated CPU/GX hierarchy composition and whole-tree invalidation for unaffected work.

**Done:** Transform counts scale with actual changed/required nodes and no gameplay result waits on GX readback.

**Stop/revert:** A faster visual hierarchy with stale gameplay sockets is rejected; keep special cases native rather than forcing generic TRS.

#### N05.07 — Expand pose and event conversion across content

**Depends on:** N05.06

**Edit/inspect boundary:** `src/nds/nds_ft_pose.c`; `src/import/battleship_ftanim.c`; `fighter motion/event banks`; `source-derived pose coverage manifest`.

**Implementation sequence**

1. Convert the current stress roster, duplicated fighter kinds, all remaining fighters, copy variants and non-battle previews by explicit family coverage.
2. Include animation end, landing speed, hitlag, simultaneous status changes, entry, KO, respawn, grabs/throws and uncommon procedural tracks.
3. Keep the independent source oracle host-side and use source-controller tours for states ordinary CPU behavior does not trigger.
4. Update domain helper/bridge inventory and compact bank capacity after each qualified family; do not accumulate permanent per-fighter compatibility engines.

**Required tests/evidence:** T-CLOCK/T-POSE complete admitted regime matrix; T-COVER real event/variant engagement; T-RES fit and no mandatory motion reads for claimed locked profiles.

**Work or dependency retired:** Remaining converted-family source pose/event parsing and representation conversion.

**Done:** All claimed families have semantic/runtime coverage; unimplemented content remains explicitly open.

**Stop/revert:** A zero mismatch count over zero comparisons or untriggered state does not close a family.

#### N05.08 — Remove legacy pose and clock authority

**Depends on:** N05.07, N06.04

**Edit/inspect boundary:** `src/nds/nds_ft_pose.c`; `include/nds/nds_ft_pose.h`; `include/nds/nds_f32_exact.h`; `src/import/battleship_ftanim.c`.

**Implementation sequence**

1. Confirm all required gameplay, render, preview and event consumers of the converted scene/domain use native state.
2. Remove one-way float publication bridges, old event-clock representations, shadow AObj/DObj state and obsolete parser helpers for those consumers.
3. Keep only necessary source compatibility outside the fully converted scope and label it a remaining runtime-float dependency until N09 closure.
4. Reclaim retired memory and rerun semantic, resource and whole-frame tests on the final hard-on build.

**Required tests/evidence:** T-FLOAT domain closure, T-RETIRE reader/caller proof, T-CLOCK and T-LIFE state/cadence coverage; memory availability confirmed at admission.

**Work or dependency retired:** Permanent duplicate pose/event authority and integer binary32 clock emulation in completed domains.

**Done:** Native pose/event producers and all consumers agree on one representation, with the old bridge physically gone.

**Stop/revert:** Do not delete fields while a source-imported callback still consumes their float ABI; move that consumer first.



---


<a id="doc-09-gameplay-collision-ai"></a>

## N06 — Compact fixed gameplay, collision, AI and scheduling

**Purpose:** remove source-shaped hot data and repeated work beyond the renderer. Sources [S15, S16, S26, S28]. Original BattleShip code remains the behavior reference, not a writable optimization target. Port-owned replacements/import shims own the DS implementation.

### Migration boundary

Build a field-use closure before replacing `FTStruct`/DObj semantics. A conceptual hot record contains native position/velocity, status/timers, input, facing, active collision volumes and compact attachment handles; cold attributes, descriptions and debug history remain separately referenced. This is not a new general entity framework. Do not duplicate a full FTStruct and copy it every tick.

For the first fixed movement slice, temporarily adapt at a documented phase boundary while unconverted hit/AI code still uses source types. The final fixed gameplay task converts those consumers and deletes the bridge. Never globally redefine `f32` to an integer or write fixed bits into fields still consumed by source floating-point arithmetic.

### Update and ordering rules

Document the current source process order from registration and dispatch, not comments alone. Input/AI, authored events, movement/map collision, hit/catch searches, damage resolution, status changes, spawns/deletes, audio and presentation can observe each other within one logic tick. A shared query snapshot is valid for a **specific source read phase/generation**, not automatically the whole tick. Update or invalidate it after relevant writers.

Compute reusable deterministic facts once per valid phase: positions/bounds, floor-line metadata, candidate masks and source-eligible targets. Never share or eliminate RNG calls merely because two CPUs query similar facts. Source RNG consumption and directed hit/catch resolution order remain exact discrete contracts.

Four fighters provide six unordered broad-phase pairs, but attacks, grabs, shields, ownership and outcomes remain directed. Sort or enumerate admitted candidates in source order; narrow-phase/stateful resolution must not be parallelized or reordered silently.

### Collision representation

Compile static stage line metadata: endpoints, line identity/kind, source flags, adjacency, conservative bounds and necessary constants. Use compact per-region or per-line candidate sets appropriate to actual stage size; a sophisticated BVH is not automatically justified. Moving/hazard collision data has an explicit transform/generation.

Broad-phase rejection must be conservative: outward rounding and motion envelopes cannot reject a collision the source narrow phase would accept. Use exact integer signs/order for topology tests where possible; prove product/sum ranges. Preserve pass-through, ledge, slope, grab/capture and platform-motion semantics. Keep geometric hit tests and gameplay hit priority as different layers.

#### N06.01 — Map hot game state and source process order

**Depends on:** N04.02, N00.05

**Edit/inspect boundary:** `src/import/battleship_ftmain.c`; `src/import/battleship_ftcomputer.c`; `src/import/battleship_sys_objman.c`; `decomp/BattleShip-main/decomp/src/ft/ftmain.c`.

**Implementation sequence**

1. Enumerate hot fields and all readers/writers across source-imported gameplay, pose, collision, AI, weapons/items and render bindings.
2. Trace actual process registration/dispatch order and same-tick mutation visibility, including spawns, deletes, hitlag, state transitions and RNG.
3. Define a compact fixed state record and phase-level accessors only for fields in a complete planned chain; pin layout/range and source semantics.
4. List temporary adapters and which downstream task removes each; retain the source code as an independent read-only oracle.

**Required tests/evidence:** T-ORDER source process/observable event traces and T-NUM field-use closure; callback and weak-symbol routes resolved.

**Work or dependency retired:** Repeated cold-state access and unclear ordering/ABI assumptions; no early wholesale struct replacement.

**Done:** A minimal hot-state schema and exact source-observation schedule are approved before runtime migration.

**Stop/revert:** Do not assume all consumers can use one beginning-of-tick snapshot; missing writers block narrowing invalidation.

#### N06.02 — Convert a complete movement and map-query slice

**Depends on:** N06.01, N04.03, N05.02

**Edit/inspect boundary:** `src/import/battleship_ftmain.c`; `port-owned fighter physics/status replacements`; `existing stage collision wrappers`.

**Implementation sequence**

1. Implement fixed input-to-velocity-to-position-to-stage-query-to-state-publication for a representative admitted fighter; include ground and air paths.
2. Preserve source acceleration, friction, terminal speed, gravity, jumps, landing, hitlag and status-entry/exit timing with source-derived constants.
3. Use explicit one-way adapters at phase boundaries only while unconverted consumers remain; record their cost and avoid conversions inside inner math loops.
4. Exercise slopes, one-way platforms, ledges, knockback movement, captures and bounds in short controlled fixtures before full matches.

**Required tests/evidence:** T-PHYS velocities/trajectories/drift/thresholds and T-COLL map boundaries; exact discrete transitions, bounded continuous errors; overflow-free target corpus.

**Work or dependency retired:** Float operations and pointer-heavy state traversal in the complete movement/map-query slice.

**Done:** The slice is native from its selected producer to publication and improves total caller cost with bridge cost included.

**Stop/revert:** A visual approximation that changes land/ledge timing or a globally saturated overflow result is rejected.

#### N06.03 — Compile stage collision metadata and conservative candidates

**Depends on:** N06.01, N04.03, N02.03

**Edit/inspect boundary:** `existing native stage generator`; `existing stage collision wrappers`; `scripts/check-mp-floor-crossing-fixtures.ps1`; `scripts/check-mp-topology-fixtures.ps1`.

**Implementation sequence**

1. Generate native line/adjacency/kind/bounds metadata directly from source stage collision data, preserving line identities and ordering.
2. Choose a small conservative candidate structure from measured query counts and memory cost. Keep an exact ordered scan as the host oracle, not a permanent competing runtime engine.
3. Implement fixed broad/narrow predicates with proved width/rounding and separate moving-platform generations.
4. Test swept movement and near-boundary rejection, including negative coordinates, corners, degenerate/parallel lines and maximum displacement.

**Required tests/evidence:** T-COLL source-equivalent candidate superset and resolved contact; missed-collision negative mutation must fail; existing topology/floor fixtures plus moving hazards.

**Work or dependency retired:** Repeated static line property calculation and unnecessary narrow queries for rejected geometry.

**Done:** Generated metadata is complete and broad phase never excludes a source-valid contact in admitted domains.

**Stop/revert:** Do not adopt a tree/bin structure whose setup/cache cost exceeds the small ordered scan or whose rounding drops contacts.

#### N06.04 — Convert interaction consumers and close gameplay pose bridges

**Depends on:** N06.02, N06.03, N05.06

**Edit/inspect boundary:** `src/import/battleship_ftmain.c`; `port-owned hit/catch/damage/knockback consumers`; `gameplay attachment and socket consumers`.

**Implementation sequence**

1. Convert hit/hurt/catch volumes, grabs/throws, projectile/item attachment and damage/knockback inputs to fixed native pose/game state.
2. Preserve directed ownership, teams/self exclusions, stale move behavior, source hit priority, invulnerability, hitlag and status/RNG order.
3. Use conservative six-pair broad-phase facts for four fighters where valid, but run directed/stateful resolution in source order and at the source observation phase.
4. Remove legacy pose publication for consumers now native and expose remaining non-battle/rare consumers to the retirement register.

**Required tests/evidence:** T-HIT simultaneous/directed hits, shields, reflects, grabs/releases, invulnerability, throws and stale-move tests; T-ORDER exact event/RNG traces.

**Work or dependency retired:** Pose-to-game float bridges and repeated broad facts in converted interactions.

**Done:** Gameplay consumers use native sockets/state and the converted domain no longer requires its legacy float pose authority.

**Stop/revert:** Shared-pair optimization cannot merge directed outcomes or reuse a snapshot after a stateful hit/status mutation.

#### N06.05 — Share AI query facts without changing CPU decisions

**Depends on:** N06.04

**Edit/inspect boundary:** `src/import/battleship_ftcomputer.c`; `port-owned native AI query layer`; `existing stage floor/target query helpers`.

**Implementation sequence**

1. Price SCPU exclusively and identify repeated deterministic facts: target position/bounds, source eligibility, floor relationships and hazard visibility.
2. Build compact phase/generation-owned facts shared by relevant CPU readers; refresh after any source-visible writer, not merely once per presented frame.
3. Convert distances/angles/comparisons through full fixed query chains while preserving selected CPU levels, reaction delays and RNG call count/order.
4. Keep source decision rules and callback sequencing; do not use lower-frequency decisions or an ARM7 one-frame-late result as an invisible optimization.

**Required tests/evidence:** T-AI all admitted CPU levels, recovery/attack/target switching, items and teams; T-ORDER exact RNG; positive button/movement/action engagement.

**Work or dependency retired:** Repeated deterministic AI fact calculation and associated float/conversion chains.

**Done:** Source-controller decisions match the specified discrete behavior and CPU cost is measured separately from its parent bucket.

**Stop/revert:** Four active GObjs with idle/no-op AI do not prove CPU engagement. Reject stale-phase shared facts even if average behavior looks similar.

#### N06.06 — Replace hot object traversal with bounded active sets

**Depends on:** N06.01, N06.04

**Edit/inspect boundary:** `src/import/battleship_sys_objman.c`; `port-owned fighter/item/weapon/effect active lists`; `source process registration shims`.

**Implementation sequence**

1. Identify repeated hot list walks/pointer chasing and actual active capacities; keep source legal capacity and same-tick insert/remove semantics.
2. Use dense handles/indices for phase-specific active sets without moving or duplicating source-visible objects unexpectedly.
3. Generate stable dispatch/status mappings where this removes a real interpreter path; share small kernels rather than generating a huge function per move/frame.
4. Keep stable iteration/resolution order and protect handle reuse with generations. A callback changing status/process ownership must update the next phase correctly.

**Required tests/evidence:** T-ORDER spawn/delete during iteration, callback replacement, simultaneous items/effects and handle reuse; T-RES capacity refusal tests.

**Work or dependency retired:** Measured hot pointer/list discovery and unnecessary generic process overhead.

**Done:** Active-set traversal is smaller/faster with identical ordering/lifetime semantics and no legal content capacity loss.

**Stop/revert:** Do not introduce an ECS/framework or swap-delete that changes gameplay processing order just to simplify code.

#### N06.07 — Evaluate scheduler flattening only when priced

**Depends on:** N06.06, N06.05

**Edit/inspect boundary:** `src/import/battleship_sys_objman.c`; `source process registration shims`; `existing taskman update seam`.

**Implementation sequence**

1. Use the measured remaining scheduler cost and proven phase trace to decide whether direct typed phases would remove meaningful work.
2. If justified, replace only the bounded hot domain with explicit source-ordered loops and bounded deferred operations where the source already defers them.
3. Preserve immediate operations when required; do not turn all spawns/deletes into next-tick queues. Keep pause, hitlag, state transition and RNG behavior.
4. If not justified, record the rejected mechanism and retain the simpler current dispatch. Scheduler rewrite is not compulsory for fixed-point closure.

**Required tests/evidence:** T-ORDER exact source process/event trace, recursive/same-tick mutations and mixed actor kinds; whole-frame timing with no omitted callbacks.

**Work or dependency retired:** Only measured removable scheduling overhead; optional task may close as REJECTED_NOT_NEEDED with evidence.

**Done:** A retained scheduler is faster and mechanically equivalent, or a documented decision avoids an unhelpful rewrite.

**Stop/revert:** Any event/order mismatch or negligible removable cost ends this route; do not reengineer all scene management.

#### N06.08 — Expand fixed gameplay to all shipped domains

**Depends on:** N06.04, N06.05, N06.06

**Edit/inspect boundary:** `src/import`; `port-owned native gameplay modules`; `source-derived gameplay coverage manifest`; `proposed runtime numeric gate`.

**Implementation sequence**

1. Convert remaining fighter specials, weapons, items, stage hazards, damage/knockback, cameras and mode-specific gameplay consumers by source-use closure.
2. Include non-VS and rare scene/state entry points reachable in shipped builds; distinguish currently unimplemented content from converted accepted content.
3. Remove completed-domain legacy structs/bridges/functions and update imports/build membership without editing reference source.
4. Run deterministic short semantic fixtures and long natural stress; permitted continuous changes can alter later workload, so also use controlled workload tours for timing attribution.

**Required tests/evidence:** T-PHYS/T-COLL/T-HIT/T-AI/T-ORDER breadth, T-FLOAT domain audit, T-RES peaks and T-COVER actual content.

**Work or dependency retired:** Remaining gameplay float chains, old hot-state authority and conversion shims in claimed converted scope.

**Done:** Every shipped gameplay arithmetic root has a native implementation or remains an explicit final-closure blocker.

**Stop/revert:** Do not call a domain fixed because its typical state avoids an unmigrated special/cold callback.



---


<a id="doc-10-stage-vfx-ui"></a>

## N07 — Stage, effects, particles and UI

**Target:** remove generic preparation and mixed numeric state without dropping source-visible content. Existing native stage packets, 2D UI work and pose masks are retained. Sources [S11, S17, S21A, S23–S25].

### Static/dynamic partition

Generate a stage draw partition from actual source behavior: immutable geometry/material groups; moving solids; collision/hazard actors; cosmetic animation; camera-facing backgrounds; translucent/no-Z groups. A world-static part still needs camera-dependent state when the camera changes, but it does not need a source DObj ancestry/material rediscovery. Keep hazard gameplay update separate from decorative presentation.

Use a small shared native submission kernel where semantics match. Do not create an all-purpose traversal/configuration object for each fixed quad, effect or label. Bake invariant UV/color/normal/layout decisions in native assets. Batching is allowed only when it preserves visibility, blend/depth and source ordering.

### Depth regression contract

Maintain tests for the impact ring with front and rear portions around a fighter; shield overlap and slice order; Castle roof/front/back geometry; Yoshi stage translucent clouds/platforms; hazard layering; cutout alpha; palette/material changes; and camera clipping. Do not fix a ring that should be depth-tested by pinning it in front. Do not make an absent platform "cheap."

OAM/BG has different priority/blending behavior from depth-tested 3D. Use it for suitable screen-space content. An intersecting world effect usually requires a native 3D depth relationship, not a fixed-priority sprite. Native-only includes legitimate CPU transforms and fixed GX kernels; it does not permit a software scene compositor.

#### N07.01 — Compile stage invariants and actor partitions

**Depends on:** N03.05, N02.03, N06.03

**Edit/inspect boundary:** `src/port/renderer_adapter_stage.c`; `src/port/renderer_adapter_matrix.c`; `existing native stage generators`; `src/nds/nds_renderer_native_common.c`.

**Implementation sequence**

1. Derive source stage static/moving/decorative/hazard classes and their transform/material/depth dependencies.
2. Compile static native runs and bound material/resource handles, preserving source draw ordering and geometry closure.
3. Keep dynamic actors and collision data under explicit mutation owners; camera changes update only view/projection/dependent billboards.
4. Remove repeated static DObj traversal, world-matrix construction and generic begin/commit setup for the converted stage partition.

**Required tests/evidence:** T-STAGE full geometry/hazards/background, T-XFORM camera changes, T-COLL stage identities and T-DEPTH order; measure static/dynamic costs separately.

**Work or dependency retired:** Per-frame source graph/material work for stage data that did not change.

**Done:** The stage partition executes natively with complete content and a smaller hot preparation path.

**Stop/revert:** Do not classify animated hazards or source-translucent groups as static merely to skip their work.

#### N07.02 — Replace remaining native actor setup scaffolding

**Depends on:** N07.01, N03.03, N03.08

**Edit/inspect boundary:** `src/nds/nds_renderer_native_owners.c`; `src/port/renderer_adapter_matrix.c`; `generated native actor/effect executors`.

**Implementation sequence**

1. Apply the proven bound-actor contract to admitted bumpers, clouds, barrels and effect owners by their actual transform/material class.
2. Generate constant vertices/UVs and native preambles; bind textures and hierarchy once at admission/spawn.
3. Keep procedural movement, billboard semantics, material animation and lifetime updates as explicit small inputs.
4. Retire generic local traversal/state arrays per converted actor and share only truly identical fixed kernels.

**Required tests/evidence:** T-BIND spawn/despawn and source actor lifecycle; T-STAGE/T-DEPTH visible moving output; capacity and resource epochs.

**Work or dependency retired:** Repeated configuration and immutable vertex/material setup for simple native actors.

**Done:** Each converted actor has a documented compact input contract and complete lifecycle proof.

**Stop/revert:** Do not replace a missing required actor with an opaque quad or return success without geometry.

#### N07.03 — Convert particle simulation and event state

**Depends on:** N04.03, N06.01

**Edit/inspect boundary:** `src/import/battleship_lbparticle.c`; `source particle/emitter definitions`; `existing particle asset generators`.

**Implementation sequence**

1. Map particle/emitter numeric fields, spawn/RNG calls, lifetime, acceleration, collision and event consumers.
2. Convert complete particle state/update chains to fixed values with source-derived constants; keep spawn law and random draw order.
3. Compile fixed emitter/curve coefficients and constant behavior metadata host-side where source rules permit it.
4. Keep existing simulation cadence and legal pool capacity. Decorative presentation policy is separate from emitter/gameplay event state.

**Required tests/evidence:** T-PARTICLE deterministic emit/spawn/lifetime/RNG traces, extreme velocities and pool pressure; T-NUM error/overflow and T-COVER visible populations.

**Work or dependency retired:** Particle software float, source numeric decoding and repeated invariant emitter work.

**Done:** Fixed particles preserve required event/state behavior with no reduced live population masquerading as optimization.

**Stop/revert:** Offscreen or low-alpha status is not automatic permission to skip RNG or emitter state.

#### N07.04 — Build a compact native particle draw batch

**Depends on:** N07.03, N03.08, N04.04

**Edit/inspect boundary:** `src/import/battleship_lbparticle.c`; `src/nds/nds_renderer_textures_effects.c`; `src/nds/nds_renderer_native_common.c`.

**Implementation sequence**

1. Consume fixed particle state directly, reuse the actual camera basis at its valid generation and prepare compact native quads/instances.
2. Batch only compatible texture/material/depth classes without reversing required transparency or source ordering.
3. Keep intersecting effects in the appropriate depth-tested native path; use sprite/BG presentation only for proven suitable screen-space effects.
4. Count emitted work, patch/copy/flush bytes and CPU time; remove the converted generic per-particle setup path.

**Required tests/evidence:** T-PARTICLE mixed materials/alpha/lifetime, T-DEPTH ring/shield and overlapping particles, T-GPU capacity/backpressure, T-RES atlas completeness.

**Work or dependency retired:** Repeated camera basis, generic quad setup and conversion bridges beyond existing caches.

**Done:** Batch preparation is smaller/faster and every required particle/telegraph remains represented correctly.

**Stop/revert:** A low particle count or missing depth intersection invalidates the result even if the frame time improves.

#### N07.05 — Make UI fixed, value-driven and native

**Depends on:** N04.03, N02.05

**Edit/inspect boundary:** `src/nds/nds_battle_hud.c`; `src/nds/nds_ifcommon_oam.c`; `src/nds/nds_menu_shell_css.c`; `src/nds/nds_menu_shell_core.c`.

**Implementation sequence**

1. Retain existing BG/OAM/native surfaces and identify residual generic geometry, float tween/layout and per-frame formatting work.
2. Precompute static layout/glyph/native tile descriptors; update digits, selection, health/stock/time and menu surfaces only when values or their animation phase change.
3. Convert UI interpolation, preview transforms, scroll and transition controls to fixed/native integer values through their full consumers.
4. Keep touch/input response, four-slot HUD, CSS selected poses and options/data/results surfaces complete; share immutable glyph assets without sharing mutable state.

**Required tests/evidence:** T-UI all screens/options/fast selection/held inputs/four slots and 30 Hz cadence; T-FLOAT non-battle arithmetic; T-RES bank/OBJ/BG transitions.

**Work or dependency retired:** Repeated unchanged formatting/layout and residual mixed-representation UI preparation, not already-existing 2D optimizations.

**Done:** All claimed UI surfaces are native/fixed with responsive source-consistent presentation.

**Stop/revert:** Do not retain a retired software text slab or stale prior-screen image as an optimization fallback.

#### N07.06 — Qualify GPU and ordering budgets for the full scene

**Depends on:** N07.02, N07.04, N07.05, N03.09

**Edit/inspect boundary:** `native renderer submission seam`; `source-derived stage/owner manifests`; `existing graphics capture/analysis scripts`.

**Implementation sequence**

1. Collect actual geometry/vertex/polygon, matrix command, texture/palette, blend/depth and FIFO wait populations for complete four-way scenes.
2. Check the configured original-DS hardware limits and current renderer slot conventions; do not infer increased per-buffer capacity from 30 Hz presentation.
3. Test worst overdraw/translucency/particle and hardest stage/fighter/item combinations separately from CPU-only hotspots.
4. Apply only approved existing quality settings; any new reduction needs a measured conflict and owner approval outside the transparent optimization verdict.

**Required tests/evidence:** T-GPU capacity and backpressure, T-DEPTH correctness, T-COVER full visible work and T-MEAS actual presentation cadence.

**Work or dependency retired:** Unnecessary graphics commands/overdraw only where equivalent native output is proved.

**Done:** CPU wins do not cause hidden GPU overflow, stalls or content loss; limits and headroom are documented.

**Stop/revert:** A CPU-only pass with GPU failure or absent geometry is RED; do not hide it behind fewer emitted primitives.

#### N07.07 — Retire converted stage/particle/UI compatibility paths

**Depends on:** N07.06

**Edit/inspect boundary:** `src/nds/nds_renderer_textures_effects.c`; `src/port/renderer_adapter_stage.c`; `src/import/battleship_lbparticle.c`; `menu/scene native source membership`.

**Implementation sequence**

1. Remove completed-domain traversal workspaces, source arithmetic, stale software surfaces and duplicate emission paths.
2. Keep remaining required native owners until their source/output coverage closes and identify those dependencies explicitly.
3. Return freed RAM/TCM reservations and regenerate the exact scene-resource manifest.
4. Run shell/battle/results/CSS and mode-specific transitions on the final hard-on configuration with semantic/audio/visual evidence.

**Required tests/evidence:** T-RETIRE/T-FLOAT reachability, T-LIFE transition loops, T-RES low-water and T-UI/T-STAGE/T-PARTICLE coverage.

**Work or dependency retired:** Retired stage, particle and UI compatibility scaffolding and old float consumers.

**Done:** Converted surfaces have one live native implementation and no hidden software composition route.

**Stop/revert:** Do not use a broad file deletion to remove an untested required child state.



---


<a id="doc-11-audio-and-offload"></a>

## N08 — Audio, storage service and measured CPU offload

**Purpose:** reduce non-overlapped work and deadline tails without moving them into an unmeasured worker. Preserve the existing direct BGM/FGM paths; the plan does not rediscover them. [S21, S22, S27, H03–H06]

### Priority

Host precomputation and native fixed-function execution come first. An ARM7 job is optional and requires measured independent work and service slack. Read the actual SDK/Calico service ownership: storage/audio work may already execute partly on ARM7. Do not propose moving the same service there a second time.

Offload benefit is the critical-path work removed minus publication/copies/cache maintenance, queueing, non-overlapped completion, contention and added memory. Measure whole-frame work and cadence, not only ARM9 function self-time. Keep accurate melonDS policy; newly used bus/concurrency behavior needs model validation, not an emulator tweak to improve the result.

### Audio rules

Compile source audio into the existing supported native playback representation host-side. Convert event/pitch/pan/volume/control arithmetic to native fixed/integer chains; this does not authorize missing voice/SFX/crowd/announcer cues or fewer simultaneous audible sources. BGM has reserved buffers/service deadlines. Required one-shot cues need a distinct resident or proved deadline-safe policy. Direct file/range reads are still I/O.

### ARM7 protocol, only if a job qualifies

A descriptor contains protocol version, request ID, scene generation, job kind, validated input/output ranges/counts, deadline and bounded result/error state. One owner writes each buffer state. Enqueue only complete cache-visible input; worker output publication and ARM9 invalidation obey the chosen memory protocol. Separate control/data cache lines where they have different writers. `volatile` alone is not coherency.

States are FREE→QUEUED→RUNNING→DONE→RECLAIMED, with explicit cancellation acknowledgement/quarantine. Timeout cannot free memory while a late worker may still write. Scene changes retire generations only after all old writers are acknowledged or safely isolated. No unbounded queue and no unchecked pointer-to-object protocol.

Audio/input/storage priorities remain protected. Same-tick AI/collision is not a first offload candidate: waiting can erase the gain, and delayed results can change mechanics. A native worker failure may trigger a defined safe error or an already-proved native CPU execution before the deadline, but not missing work or an N64 graphics fallback.

#### N08.01 — Attribute remaining service tails and ownership

**Depends on:** N00.03, N02.01

**Edit/inspect boundary:** `src/nds/nds_audio_bgm.c`; `src/nds/nds_audio_fgm.c`; `src/nds/nds_audio_assets.c`; `actual SDK/Calico ARM7 service configuration`.

**Implementation sequence**

1. Identify each service thread/core, storage client, synchronization point and buffer owner in the current build.
2. Classify BGM refill, FGM startup, remaining motion demand and other storage events by bytes, event frequency, blocked work and deadlines.
3. Price event-window totals instead of medians that discard infrequent refills; separate unused idle from blocking service.
4. Choose the next service mechanism only from remaining measured cost, preserving the direct-read optimizations already retained.

**Required tests/evidence:** T-AUDIO event/underrun/late-cue witness and T-MEAS exclusive service attribution; no double-counting background work.

**Work or dependency retired:** Unclassified storage clients and repeated ownership guesses; no speed credit yet.

**Done:** All recurring services have a declared owner, budget/deadline and measured interference.

**Stop/revert:** Do not claim zero I/O from zero libfat calls or assume ARM7 has spare capacity without measurement.

#### N08.02 — Convert audio control and eliminate unnecessary decode work

**Depends on:** N08.01, N04.03, N04.05

**Edit/inspect boundary:** `src/nds/nds_audio_bgm.c`; `src/nds/nds_audio_fgm.c`; `src/nds/nds_audio_assets.c`; `existing host SFX/BGM generators`.

**Implementation sequence**

1. Convert pitch/rate, envelope/volume, pan and event/control arithmetic through fixed/native integer consumers.
2. Move invariant source-format conversion or sample preparation to host generators using the current supported playback API/format.
3. Keep cue identity, duration, loop/seam behavior, voice priority and audible simultaneous events correct under four-way bursts.
4. Remove completed-domain float/libm paths and repeated runtime setup that the native asset/control representation replaces.

**Required tests/evidence:** T-AUDIO source cue events, seam/finite-track completion, pitch/pan/volume bounds, burst priorities and T-FLOAT audio roots.

**Work or dependency retired:** Audio-control float and host-convertible runtime decode/setup, not audio content.

**Done:** Native control and assets meet audible/timing contracts without missing cues or added service stalls.

**Stop/revert:** Audio quality/rate/channel reductions require approval and cannot be counted as transparent code savings.

#### N08.03 — Bound buffers, prefetch and storage scheduling

**Depends on:** N08.02, N02.04

**Edit/inspect boundary:** `src/nds/nds_audio_bgm.c`; `src/nds/nds_audio_fgm.c`; `scene admission and storage service seam`.

**Implementation sequence**

1. Reserve BGM and required cue buffers with explicit lifetime and worst-case service interference.
2. Choose resident or deadline-safe one-shot policy from measured latency and legal cue bursts; do not copy the BGM streaming exemption to every cue.
3. Prioritize/coalesce declared range requests only where order/cursor/loop semantics permit, and avoid filesystem discovery inside playback starts.
4. Prove fallback/error handling does not duplicate playback, reuse a live buffer or silently lose a required cue.

**Required tests/evidence:** T-AUDIO delayed/short/failed read injection, simultaneous cues, loop boundaries, cancellation and source event order; T-RES reserves and zero undeclared demand.

**Work or dependency retired:** Unnecessary per-event allocation/discovery and avoidable non-overlapped service tails.

**Done:** Declared services meet deadlines under the hardest complete scene with explicit memory cost.

**Stop/revert:** A bigger buffer that causes battle admission OOM or a rare missing sound is not a win.

#### N08.04 — Overlap hardware math only under proven ownership

**Depends on:** N04.03, N08.01

**Edit/inspect boundary:** `include/nds/nds_r2_hwmath_unit.h`; `src/import/battleship_gmcamera.c`; `actual IRQ/thread math-unit users`.

**Implementation sequence**

1. Inventory every divide/sqrt register writer in the current linked binary including library, thread and IRQ paths; discard old no-preemption assumptions unless re-proven.
2. For a measured remaining independent operation, split start/consume and schedule useful native work between them without allowing another writer to overwrite the result.
3. Define ownership/save-restore or bounded critical section at the shared-unit boundary; do not add broad interrupt masking at every call.
4. Compare with the already-existing synchronous native helper, including ownership overhead and interrupt/service effects.

**Required tests/evidence:** T-HWMATH overlapping/cancelled/preempted operations and exact domain results; T-AUDIO IRQ deadline integrity; whole-frame timing.

**Work or dependency retired:** Only actual exposed wait latency for a qualified math chain.

**Done:** A retained asynchronous schedule improves total cost with correct cross-user ownership, or the synchronous route remains.

**Stop/revert:** Do not count hardware math as newly implemented or adopt overlap on a stale register-writer audit.

#### N08.05 — Select and price one ARM7 candidate

**Depends on:** N08.01, N08.03

**Edit/inspect boundary:** `actual ARM7 build/service entry points`; `existing shared-buffer/IPC APIs`; `proposed bounded worker job descriptor`.

**Implementation sequence**

1. Select one coarse independent job with measured critical-path cost and demonstrable ARM7 service slack, such as preparation of non-immediate service data.
2. Compute input/output bytes, copy/cache cost, queue latency, deadline, cancellation and resident-memory budget before writing the worker.
3. Prototype an equivalent native CPU control and one bounded worker implementation without displacing SDK audio/input/storage services.
4. If no job qualifies, close this optional task as NOT_SELECTED with evidence; do not manufacture an offload requirement.

**Required tests/evidence:** T-IPC protocol schema/ranges/generations and baseline slack; T-MEAS end-to-end work/deadline evidence.

**Work or dependency retired:** A named non-overlapped job only if measured benefit exceeds communication/contention costs.

**Done:** One candidate has a defensible protocol and total-time case, or ARM7 offload is explicitly not needed.

**Stop/revert:** No same-tick AI/collision offload with new latency; no second ARM7 storage service that duplicates an existing one.

#### N08.06 — Prove worker coherency, cancellation and net gain

**Depends on:** N08.05

**Edit/inspect boundary:** `bounded worker/IPC implementation if selected`; `shared-buffer allocator/publication seam`; `actual ARM7 service scheduling`.

**Implementation sequence**

1. Implement C7/C5-style explicit ownership with completed cache-visible inputs and uniquely owned output buffers outside TCM.
2. Handle queue-full, timeout, late completion, scene-generation retirement and cancellation acknowledgement without freeing a possible live writer.
3. Inject ordering/delay/failure races and prove services remain responsive; a late result cannot change another scene’s state.
4. Measure CPU work, total frame/cadence, deadlines and memory versus the control. Retain only a whole-system winner; otherwise remove the worker/queue.

**Required tests/evidence:** T-IPC stale reply, same-address reuse, cancellation race, full queue and double publication; T-AUDIO deadlines; serial uncontaminated timing.

**Work or dependency retired:** Actual critical-path ARM9 work after all worker overheads, or removal of a failed optional prototype.

**Done:** A selected offload is safe and faster end to end; an unselected/failed route leaves no permanent framework.

**Stop/revert:** Never report a faster ARM9 bracket while the join, DMA contention or audio underrun moved elsewhere.

#### N08.07 — Close audio/service runtime numerics and reachability

**Depends on:** N08.03, N04.06

**Edit/inspect boundary:** `src/nds/nds_audio_bgm.c`; `src/nds/nds_audio_fgm.c`; `actual ARM7/shared runtime sources`; `proposed no-float gate`.

**Implementation sequence**

1. Audit both processor/service build roots and all cold error/transition/control paths for remaining floating arithmetic.
2. Remove completed-domain native/legacy duplicate service state and experimental routes not retained.
3. Preserve platform startup/shutdown/return-to-loader and established service lifecycle while narrowing the arithmetic/runtime surface.
4. Publish declared service classes, deadline evidence and the final fixed-runtime service result with matching build identity.

**Required tests/evidence:** T-FLOAT both target cores/services, T-AUDIO cue/loop/shutdown and T-LIFE transitions; no unknown arithmetic roots.

**Work or dependency retired:** Remaining reachable audio/service float and retired worker/control scaffolding.

**Done:** Services satisfy the owner’s runtime-fixed requirement, independently of battle-loop arithmetic.

**Stop/revert:** Do not exempt cold audio/control code from the all-runtime endpoint or alter library internals without a reproducible build.



---


<a id="doc-12-final-pack-and-retirement"></a>

## N09 — Final retirement, hot-data/code placement and kernel tuning

Do this on the smaller integrated runtime, not on a snapshot full of temporary old/new routes. Preserve qualified system residents and the current DTCM stack ceiling. Source-level line count, comments and total repository size are not runtime metrics. Report linked code/data and executed work. [S07, S08, S03, H01, H02]

### Packing policy

**DTCM:** compact CPU-only mutable state whose repeated accesses justify its bytes. Measure candidate records individually; do not mirror state permanently between main RAM and DTCM. Immutable bulk geometry and DMA/ARM7 buffers remain outside. Include alignment and current stack/interrupt low-water, not only data symbols.

**ITCM:** small dense kernels with measured whole-frame benefit, explicit unique sections and controlled literals/veneers. Binding, failure diagnostics, file access and source interpretation do not belong merely because their caller is hot. Renderer code does not receive an arbitrary entitlement to fill the region; neither does game code. The measured smaller kernel set decides.

**Main RAM/cache:** .text.hot and .text.hot.draw are placement groups in the same physical instruction-cache competition, not independent caches. Re-evaluate layout after code deletion; preserve old comments as historical evidence rather than permanent prohibitions on a new architecture.

**Bare metal:** inspect fixed arithmetic, packet patching, active-list and small matrix kernels before considering assembly. First remove operations and unnecessary reads. Test ARM/Thumb, modest per-kernel optimization levels, inlining and load/store scheduling on the actual toolchain. No global -O3/LTO/fast-math lottery. Source-compatible libraries may be rebuilt only with pinned reproducible options and ABI/regression proof.

#### N09.01 — Close the whole-runtime float and retirement ledger

**Depends on:** N03.10, N05.08, N06.08, N07.07, N08.07, N04.04, N04.05, N04.06

**Edit/inspect boundary:** `proposed runtime numeric gate`; `Makefile`; `src/import`; `src/nds`; `src/port`; `both target-core linked artifacts`.

**Implementation sequence**

1. Run the complete source/type/lowering/link/caller audit on all shipped scene/service roots and resolve every temporary numeric allowlist entry.
2. Remove remaining duplicate authority, float conversion shims, integer IEEE arithmetic, obsolete graphics recorders and unreachable experimental helpers.
3. Keep source data/oracles host-side and source behavior references read-only; verify target object membership prevents their accidental inclusion.
4. Reconcile removal at input-section granularity and eliminate cold float formatting/transition/math-library roots as well as hot battle calls.

**Required tests/evidence:** T-FLOAT zero unresolved runtime arithmetic roots; T-RETIRE bridge/caller/data absence; weak/indirect/varargs/custom-IEEE negative fixtures.

**Work or dependency retired:** All remaining runtime floating arithmetic and completed-domain compatibility dependencies.

**Done:** The owner’s fixed-runtime endpoint is structurally proved across shipped configurations, not inferred from one trace.

**Stop/revert:** Any unresolved callback or cold path keeps this task OPEN; do not rename helpers or suppress checker findings.

#### N09.02 — Pack compact CPU-hot state into DTCM

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

#### N09.03 — Repack ITCM around final kernels

**Depends on:** N09.02, N01.06

**Edit/inspect boundary:** `linker/nds_hot_text.ld`; `include/nds/nds_task37_itcm.h`; `native emit/pose/collision kernels`; `placement checker scripts`.

**Implementation sequence**

1. Rebuild the byte/caller/execution census after removing helper families and old renderer state; do not reuse a historical tenant ranking.
2. Remove placement from necessary cold code, shrink cold-inside-hot tails where still present and rank current gameplay/pose/emit kernels.
3. Test placement-only candidates and combined working sets with full literal/veneer/alias accounting.
4. Update placement assertions and record current code bytes by subsystem without treating a predetermined renderer/game split as a performance law.

**Required tests/evidence:** T-TCM linked/kernel identity, startup/IRQ/interworking and whole-frame profiling in final configuration.

**Work or dependency retired:** Avoidable instruction fetch stalls and remaining cold code occupying final ITCM.

**Done:** ITCM residents are explicitly justified by current measured benefit and all bytes fit.

**Stop/revert:** Do not evict an interrupt/startup/rare-required path as dead because of a short trace.

#### N09.04 — Tune only remaining measured kernels

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

#### N09.05 — Remove experimental routes and rebuild shipping shape

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

#### N09.06 — Publish the final architecture and resource ownership map

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



---


<a id="doc-13-agent-execution"></a>

## Agent execution and integration contract

### 1. The unit of work

Use one task ID from `tasks.json` under existing P2-2p8. Read the current board and handoff, the task card, its named source, shared contracts and relevant test sections. Do not restart implemented optimizations or reread every historical campaign on every run. Source files/functions are pinned anchors; re-resolve after rebasing.

Each assigned task must have a specific behavior/operation to remove. “Optimize the renderer” is not a task. A task is also not complete when it only adds infrastructure that every future agent must maintain. Enabling work must name its immediate consumer and eventual deletion of the old path.

### 2. Source ownership and concurrency

| Lane | Owns | May work in parallel with | Must serialize |
|---|---|---|---|
| Integrator | Shared ABI, linker, Makefile, generated outputs, current baseline and board | Read-only review/host fixture authoring | Every shared generated-output update and target build |
| Renderer | Bound descriptors, native packet/actor emitters and renderer state | Disjoint numeric fixtures, gameplay source analysis | Other edits to included renderer fragments/shared statics |
| Numeric/pose | Numeric kernels, event clocks, native pose contracts | Disjoint generator test authoring | Shared numeric header, pose consumer ABI changes |
| Gameplay | Physics/collision/AI/phase-owned query state | Disjoint stage/UI work | Shared FTStruct/consumer migration and scheduler changes |
| Content/services | Stage/VFX/UI or audio/offload, one claimed source slice | Independent host tests | Resource manifest/bank handoff and service ownership |
| Reviewer | Independent source/geometry/numeric/correctness proof | All disjoint source implementation | Final acceptance writes and timing runs |

Do not force a fixed number of agents when their edit sets overlap. Additional helpers can inspect or write disjoint host fixtures, not repeatedly touch the same giant translation unit. The project procedure allows isolated correctness runners but requires one build at a time and no concurrent timing/exact visual acceptance. Preserve that policy. [S03]

The current board reports many pre-existing dirty auxiliary worktrees. Inspect existing workspace ownership before creating another. Never use `git reset --hard`, `git clean`, broad deletion of `decomp/`/`artifacts/`, or replacement of owner inputs to make a check green.

### 3. Task handoff template

```text
Task: Nxx.yy — exact title
Parent: P2-2p8
Baseline: commit + intended dirty overlay + ROM/ELF/config/asset hashes
Dependencies: IDs and accepted evidence paths
Allowed edits: concrete files/symbols and one owner for shared interfaces
Work to remove: repeated operations + current measured/bounded cost
New runtime cost: patches/copies/service/memory introduced
Contracts: numeric class, mutation owners, event order, lifetimes
Tests: relevant fixture IDs + existing or task-created invocation
Limits: original-DS RAM/TCM/transient resources; no required content loss
First falsifier: cheapest test that would invalidate the mechanism
Finish: code + producers + outputs + tests + final hard-on evidence
Stop: exact correctness/performance/resource condition causing revert
Status return: KEEP / REVERT / BLOCKED_WITH_SPECIFIC_CAUSE
```

The task-specific cards already fill most of these fields. Add current hashes and measured costs; do not invent projected savings or report task metadata as runtime evidence.

### 4. Per-task execution loop

Read current implementation and identify already-landed parts. Run the cheapest relevant source/host check. Freeze inputs and coordinate the edit boundary. Implement the smallest complete vertical slice and its negative tests. Build once through the integrator. Run a focused source-controlled target case with positive engagement. Decide KEEP/REVERT; for a kept batch collect the widest relevant integrated proof, then rebuild/qualify the final hard-on shipping shape when due. Retire the losing route and update the existing board row/evidence.

Do not perform a long unchanging suite after every line edit. Do not skip required integrated/final coverage merely because a host test passed. Use the documented eight-frame synchronized comparison for early iteration; long-run P95 and cadence require whole-match evidence. Respect the documented cross-build noise/significance handling rather than forcing tiny deltas into a victory. [S03]

If a checker fails because of an existing owner overlay or infrastructure issue, preserve it and report the exact blocker plus focused evidence. Do not mark the umbrella green, waive native/content checks, delete the owner's input, or repeatedly rebuild until a noisy run passes.

### 5. Commands and build discipline

The pinned procedure uses PowerShell 7 and configured devkitPro/devkitARM. Verify script parameter blocks before copying historical commands. Existing startup commands:

```powershell
git status --short
.\scripts\verify-all.ps1 -Profile Boundary -List
```

On a prepared host, the P2 build entry is `make TARGET=smash64ds`. For an unprepared host, `build.ps1 -Rom <owner-provided baserom path>` is the documented acquisition/extraction entry, with prerequisites checked first. This package does not contain or supply a copyrighted game ROM or generated ROM assets.

Never pass `-j`, override `-Jobs`, or change `MAKEFLAGS`; the existing Makefile owns build parallelism. Shared generated files make parallel builds unsafe even in separate BUILD directories. Do not use a custom BUILD with a publish target as though it could not overwrite the root ROM. Lab targets and matching per-build ROM/ELF pairs are for experiments. [S03]

Boundary is three named runtime entries; Latest adds the normal runtime entry. Choose the widest relevant profile for a batch instead of stacking every profile. A green Boundary does not rebuild the root shipping ROM automatically. `-NoBuild` requires validated matching artifacts. Do not change the established profiles to hide a slow product result; N00.04 adds explicit performance evaluation.

Use repo-local accurate melonDS and JIT-disabled interpreter from boot; keep the owner's manual instance untouched. Timing runs are serialized. Disposable per-run save/DLDI/storage paths and coherent guest observation/publication are part of the identity, not optional bookkeeping.

### 6. Progress and evidence format

Keep the dynamic board concise. A kept work item can read:

```text
N03.04 KEEP — replay-hit immutable preflight retired; native/state guards pass.
Evidence: <path>; ROM <hash>; WORK-H/cadence <measured values>.
Remaining: <specific unconverted family or final acceptance gap>.
```

Do not use **FIXED** without required coverage. A report should distinguish source presence, host checks, target engagement, integrated correctness, product performance, resource closure and published artifact. Store detailed source reasoning and logs under the existing evidence owners; do not paste them into every code comment and board row.

### 7. Safely adopting or revising this plan

The package is additive and was not pushed to GitHub. Review the proposed shared interfaces and first task at the active branch before committing the plan. If a newer commit already implements a task, link its equivalent evidence and move to its remaining obligation rather than reimplementing it.

Update static dependencies when a real ownership requirement changes. Do not create a new parallel campaign simply because task IDs are inconvenient. Keep the plan validator/combined view generated from one source set; the existing P2 board remains the live queue.



---


<a id="doc-14-validation"></a>

## Validation specification and final qualification

**Test IDs below are requirements to implement or map onto existing tests.** They are not claims that these tests were run while writing the plan. Existing entry points are identified where verified. New host checks/fixtures remain task deliverables.

### 1. Evidence layers

| Layer | What it proves | What it does not prove |
|---|---|---|
| Static source/type/link | Ownership, numeric reachability, target membership, section/resource bounds | Runtime engagement, mechanics, pixels or FPS |
| Independent host source oracle | Geometry/event/predicate semantics and known numeric domains | Target timing, cache/IPC coherency or complete admitted content |
| Focused target fixture | One actual runtime mechanism with positive witnesses | Whole-match P95, global roster completeness or lifecycle stability |
| Source-normal whole match | Real workload, cadence and same-run resource/engagement evidence | Untriggered moves/children, Time Up/Results/rematch or every content combination |
| Final hard-on natural-input build | Shipping configuration and integration | Unmeasured future content or changed SDK/emulator |

Use the least expensive layer capable of falsifying a change, then collect the widest relevant integrated evidence for a kept batch. Never replace an independent semantic oracle with the candidate's own generator output. [S03, S19]

### 2. Detailed fixture catalog

#### T-MEAS — Measurement and product gate

- **M01 identity:** deliberately combine ROM A with ELF/config B; fail before collecting or interpreting samples. Save/debug storage and emulator policy changes must change identity.
- **M02 accounting:** verify WORK-H equals WORK−HUD per frame and reconciles the documented exclusive sum. SRC/GCRA/SINT/SCPU cannot be counted together as separate owners.
- **M03 rank:** use small hand-checkable fixtures and the real sample population to validate the current percentile convention. Do not hardcode the old rank-80 index into all runs.
- **M04 product:** fixtures that fail only P95, only cadence, both, or neither must yield four independent correctness/coverage/work/cadence fields. A correctness-only green must not become product green.
- **M05 population:** missing slow frames, malformed/duplicate payloads, ring wrap corruption, wrong frame ranges, repeated labels without justified collector semantics and zero samples must fail or be explicitly invalid, never disappear from percentiles.
- **M06 time units:** compare timer configuration, cpuGetTiming behavior and emulator cycle counters. Store the conversion and tested calibration; do not confuse two-VBlank timer ticks with raw ARM9 instruction cycles.
- **M07 apparatus:** visible HUD subtraction is per row; any remaining instrumentation/layout effect is independently described. Remove no task CPU work from the metric twice.
- **M08 controlled versus natural:** exact workload comparisons use the same source-controlled states/input/seed. Once allowed continuous numeric changes diverge long natural matches, use natural runs for outcome/coverage and controlled tours for causal timing. Report both scopes.
- **M09 tails:** store P50/P95/P99/max, cadence histogram, maximum interval and consecutive late-frame runs. A diagnostic attribution exclusion never becomes a gate-population exclusion.
- **M10 negative engagement:** disable one CPU action source or omit a required owner in a fixture and prove coverage/product acceptance fails even with lower ticks.

Existing anchors: `scripts/census-tick-hud-p95-set.py`, `scripts/verify-p2-four-fighter-stress.ps1`, existing ring/capture scripts. New evaluator is N00.04. The current 1,972-sample stress clock 60→1 does not test match completion. [S03, S18]

#### T-NUM — Native arithmetic

- **N01 limits:** min/max valid fields, maximum source scale/translation/speed, negative coordinates and values one unit inside/outside admitted ranges.
- **N02 rounding:** positive and negative halves, exact multiples, values adjacent to zero, floor versus truncation and chained accumulation.
- **N03 widths:** every multiply/accumulate/shift path, sum-of-squares bounds, intermediate cancellation and narrowing. Host sanitizers/checked reference must identify overflow and undefined shifts.
- **N04 divides:** positive/negative numerator and denominator, large valid widths, tiny nonzero denominator, denominator zero failure, quotient/remainder sign and constant-reciprocal equivalence.
- **N05 vectors/angles:** zero vector, near-zero direction, opposite/up-degenerate vectors, full-turn wrap, shortest signed angular delta and negative scale.
- **N06 drift:** source-controlled long integration with error envelopes, plus short exact-discrete boundary tests. Tolerance is per field/domain, never a blanket epsilon.
- **N07 independent oracle:** compare target output against independently computed high-precision/reference values, not only identical C compiled on two hosts.
- **N08 code generation:** ARM9 ABI, alignment, register clobbers for assembly, helper/veneer calls, unexpected doubles and pinned compiler flags.

#### T-FLOAT — Whole-runtime no-float proof

Inject and detect: float hidden behind typedef; operator in a source-included TU; double promotion from a literal/varargs; libm through a function pointer; weak alias to a float routine; custom integer mantissa/exponent arithmetic; runtime `%f` formatting; a cold menu/transition helper; an ARM7 service callback; and an inline conversion that does not call libgcc.

Constant-only data or host-generator arithmetic is classified separately. A target build can have no FPU instructions and still perform extensive software float. Success requires the type/lowering/call/link analyses and root coverage to agree. Unknown callbacks and unresolved library roots remain blockers. A finite dynamic trace cannot establish global absence by itself. N04.06 defines the gate; N09.01 closes its temporary allowlist.

#### T-CLOCK — Event timing

- **C01 speeds:** every source speed constructor and its admitted legal domain; known 1/3 and 16/3 counterexamples plus source landing/rebound cases, dyadic controls and large/small waits.
- **C02 segment edges:** exact boundary, one tick before/after, multiple events on a tick, zero wait, end/changed/null and looping.
- **C03 discontinuities:** attach at nonzero frame, seek, speed change before/on/after an event, status interrupt/re-entry and animation replacement on the same source tick.
- **C04 hitlag/pause:** state freezes and resumes at the specified source boundary; no catch-up double event or off-by-one landing.
- **C05 lanes:** different per-joint waits/loops, last-writer GObj publication and source script order.
- **C06 dynamic ratios:** rebound and other live parameter-derived speed changes; finite generated deadlines do not exempt dynamic cases.
- **C07 overflow:** long duration and tick/phase/generation wrap policy; corrupted event metadata is rejected at admission.
- **C08 behavioral outcomes:** attack/weapon spawn, landing/end/status and audio events occur at required ticks/order, not merely close pose coordinates.

Existing source tests: `scripts/fighters/test_pose_clock_differential.py`; the current generic pose oracle can inform cases but binary32 field equality is not automatically the final native representation contract. Keep the oracle independent and host-side where it interprets source formats. [S13]

#### T-POSE / T-XFORM — Pose, matrices and required sockets

- Constant/linear/cubic channels at endpoints and interior extrema; quantization bounds and scale/rotation interpolation.
- Different clips sharing constant channels without sharing mutable cursor/state.
- Required-joint closure with ancestors, hidden but gameplay-active parts, held items, throws, copy hats, Samus morph, selected CSS poses and procedural attachments.
- Same root pointer after reparenting, same arena address with new lifetime, topology generation change and a hierarchy exceeding the declared bitset capacity.
- Camera-only, world-only, local-pose-only and material-only mutations update precisely their dependent outputs.
- Source transform classes including billboard/orientation replacement, translation retention, scale accumulation, negative scale/reflection, degenerate camera axes and perspective clipping.
- CPU gameplay socket and native visual transform agree within their approved numeric contract; no gameplay matrix depends on having rendered the fighter.
- Counts prove avoided local/world evaluations and matrix copies; copied bytes are not counted as eliminated just because the copy moved elsewhere.

#### T-BIND / T-BANK — Bindings and native formats

- Header/ABI/schema/source hash mismatch; truncated header/table/payload; overlapping or misaligned spans; integer-overflowed count×stride; invalid pointer-relative offset.
- Wrong owner/model variant, copied foreign root, stale resource epoch, same-address allocator reuse and wrapped generation handling.
- Invalid texture view/palette/material dependency; equal-count but wrong identity; missing child or cold state.
- Four same-kind fighters with distinct costumes, materials, animation clocks and patch contents must not mutate shared immutable bank data.
- Partial admission failure at each step must not publish a mixed-epoch scene; cancellation and scene return preserve correct retirement.
- Fast valid binding does not perform source file reads, original graphics command decoding or immutable graph rediscovery in the qualified hot draw.
- Repeated deterministic regeneration produces identical output/hashes; source identity and source-derived completeness are independently checked.

#### T-GEOM / T-PACKET — Native commands and topology

Keep all six existing independent closure dimensions: source triangle completeness/order; vertex-cache-to-dense identity; matrix routing; authored facing; winding; primitive-strip/BEGIN expansion. Add packet command/parameter decode and dynamic patch proof. [S19]

Negative fixtures: drop the second G_TRI2 triangle; truncate the last triangle; alias a overwritten cache slot; reverse one strip; route a vertex to a wrong matrix; read the NDO6 alpha/flag byte as an unmasked class; remove unlit color; misalign a patch; write one word past its span; omit an initial material/matrix state; use a foreign bank under the wrong owner; and hide a required root while preserving total triangle count elsewhere. Each must fail an appropriate independent checker.

Runtime tests exercise split roots, palette/visibility changes, Link texgen, Kirby foreign roots, high/low details and previous-owner state contamination. Patch only declared dynamic words. Test a camera change during four-instance submission and an attempted write/free while DMA reads the packet. Completion and buffer-retirement witnesses belong to the actual transport owner.

#### T-DEPTH / T-GPU — Native presentation and hardware limits

Test an impact ring partly in front of and partly behind a fighter; shield overlap/slice ordering; Castle roof and occluded geometry; stage translucent clouds and cutouts; acid/hazard layering; effects at near-plane limits; palette animations; and mixed translucent particles crossing characters. Source depth semantics, not aggregate nonblack pixels, define success.

Measure primitive/vertex/matrix use, FIFO waits, texture/palette/atlas placements and render completion for complete scenes. Do not assume 30 Hz increases per-buffer geometry capacity. A packet with fewer CPU instructions but excess commands, matrix-store collisions or raster stalls is rejected. An opaque substitute, missing background/platform or zero-alpha required effect is incomplete content, not reduced cost.

#### T-PHYS / T-COLL / T-HIT — Gameplay mechanics

- Ground/air movement, acceleration/friction/terminal speed, jump trajectories, gravity, land/ledge timing, knockback, hitstun/hitlag and source integer timers.
- Slopes, platform corners/edges, pass-through from both directions, moving platforms, ledge approaches, high-speed swept crossings, degenerate/parallel lines, negative coordinates and blast-zone bounds.
- Conservative broad-phase supersets versus source ordered narrow results; outward rounding never drops a source-valid collision.
- Four-way simultaneous attacks, directed grabs, shields/reflects, teams/self exclusions, invulnerability, stale moves, throw/release/item sockets and weapon ownership.
- A stateful hit or status change invalidates phase-dependent shared facts before the next reader; six unordered broad pairs do not erase directed outcomes.
- Corrupt or out-of-domain numeric inputs trigger established safe failure, not silent wrapping/saturation into plausible gameplay.

Reuse existing floor/topology/hitstatus fixtures and extend from actual BattleShip behavior. Do not require random long-match bitwise identity after explicitly allowed continuous numeric differences; do require exact known discrete boundary outcomes.

#### T-AI / T-ORDER — CPU and scheduler

- Every admitted CPU level, target switch, recovery, movement, defense, attack and item decision with source controller/RNG timing.
- Four CPUs are positively engaged: movement/actions/targets/attacks, not merely four live objects. Some states require human/source-controller tours rather than an ordinary CPU trigger.
- Same-tick create/delete/status/process changes, callback replacement, mixed actor types and deterministic iteration order.
- Stable handles reject old-generation reuse; a dense-list optimization cannot reorder hit/RNG outcomes through swap-delete.
- Shared query facts are keyed to the source-observation phase; test a writer between two readers that would invalidate a whole-tick snapshot.
- If optional scheduler flattening is not implemented, record that decision rather than weakening the order tests.

#### T-PARTICLE / T-STAGE / T-UI

Particle spawn/emitter/RNG/lifetime and pool pressure remain source-equivalent; hidden/transparent/offscreen does not automatically mean inactive. Mixed atlas/material/alpha/depth effects test both simulation and drawing. Static stages retain moving hazards/collision/backgrounds and source animation inputs. UI tests cover four-slot HUD values, quick CSS selection/selected poses, previews, stage select, options/backup clear/data/results and every shipped transition. Prove 30 Hz cadence and native fixed arithmetic outside battle. A stale screenshot or retired software text buffer fails.

#### T-RES / T-LIFE — Memory, storage and lifetime

- Exact required/admitted/excluded sets and all format/bank/palette/view/atlas constraints.
- Minimum free heap plus maximum transient/graphics/packet/thread/boot stack use; code-region limits must reflect original DS, not a broad DSi-capable address range.
- Repeated CSS→battle→results→CSS, rematch, sudden death, pause, KO/respawn, copy/morph, entry, campaign/other shipped modes and allocation-address reuse.
- Inject failure during allocation, relocation, upload, bind, publication and cancellation. No leaked temporary bank or partially valid scene.
- Zero mandatory motion/texture post-GO reads for locked profiles. Direct NitroROM reads count. BGM is separately declared; required one-shot service has its own proof.
- Long-soak late states and cue bursts, plus independent memory-hardest and CPU-hardest rosters.
- Reclaimed memory is available at the actual allocation boundary; an ELF BSS reduction alone is insufficient.

#### T-AUDIO / T-HWMATH / T-IPC

Source cue identity/timing, finite-track ending, loop seams, pitch/pan/volume bounds and simultaneous required cues. Inject late, short, failed reads; preserve playback cursor and do not double-play a fallback. Worker tests include full queue, incomplete publication, late completion, cancellation race, stale scene generation, same-address reuse and attempted buffer reclamation with an outstanding writer. Math tests include every current shared-unit writer and interruption/overlap. Confirm no audio/input/service starvation and whole-frame net gain for retained offload.

An unselected ARM7 experiment is not a missing required feature. A selected one must satisfy all relevant tests. No TCM pointers cross into DMA or ARM7 requests; volatile is not cache coherency.

#### T-TCM / T-BUILD / T-RETIRE / T-DOC

Reconcile every section byte, alias/literal/veneer/alignment and startup LMA assumption. Test IRQ/boot/context/interworking and stack limits. Build all currently shipped configurations after source membership/schema changes; generator-staleness and untracked-derived-input checks remain enforced. Confirm retired functions/data/bridges are unreachable/absent and no build flag revives a forbidden graphics route. Final source/config/ROM/ELF/evidence links must agree. Plan link/dependency validation is useful, but it is not a substitute for any game test.

### 3. T-COVER — Coverage matrix construction

Use individual coverage for every currently landed fighter, stage, item/child and reachable scene. Use mechanism partitions to exercise special cases: same-kind sharing, distinct-kind memory pressure, copy/foreign bank, morph/model parts, texgen, unlit color, depth-intersecting VFX, translucent/no-Z stages, moving hazards, many cues and late state transitions. Then execute pairwise/targeted four-way combinations plus measured leaders. Record the exact search set and its limitations; do not claim an exhaustive global argmax from a sampled matrix.

Re-run the measured leaders whenever converted code/content affects their mechanism, and rescreen newly landed content before adopting the next leader. Full-content acceptance requires missing native owners to be implemented in their existing P2 packages; this optimization campaign does not waive those tasks.

### 4. Existing commands versus proposed commands

The following existing command shape is verified from the pinned script parameter block and procedure; substitute a genuinely idle runner slot and unique output directory. This is execution guidance, **not a report that it was run here**:

```powershell
## Repository root, PowerShell 7; no parallel builds or concurrent timing.
$case = 'artifacts/performance/native-opt/baseline'
New-Item -ItemType Directory -Force $case | Out-Null
.\scripts\verify-p2-four-fighter-stress.ps1 `
  -RunnerSlot 2 `
  -JsonOut "$case/timing.json" `
  -RowsCsv "$case/rows.csv" `
  -CoverageJsonOut "$case/coverage.json" `
  -MemoryJsonOut "$case/memory.json"
```

Require the script to complete successfully and inspect its terminating errors and final verdicts. Preserve full OS-level logs for umbrella execution as `VERIFYING.md` requires. Runner slot 2 is an example, not a reservation. Omit `-NoBuild` unless ROM/ELF/config freshness is proved.

Other existing entry points:

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
python .\scripts\fighters\check_native_owner_geometry_closure.py
.\scripts\check-generator-staleness.ps1
.\scripts\check-melonds-policy.ps1
```

New numeric/product/bank/packet fixtures described here must be implemented under their tasks before adding invocation examples to the operational runbook. Never invent a passing command for an uncreated checker.

### 5. Final release tasks

#### N10.01 — Define the full qualification matrix

**Depends on:** N00.05, N02.01

**Edit/inspect boundary:** `source-derived fighter/stage/item/scene manifest`; `scripts/lib/harness-registry.ps1`; `proposed qualification scenario manifest`.

**Implementation sequence**

1. Enumerate current shipped content/configuration families and their distinctive mechanisms, including two detail settings when reachable, copy/morph and mode transitions.
2. Build deterministic short fixtures for every semantic/graphics mechanism and source-normal full-match scenarios for the current measured cost leaders.
3. Use all stages/owners in coverage, duplicated and mixed four-fighter sets, items/hazards, CPU levels and memory extremes; record an explicit coverage algorithm rather than claiming every combination was exhaustively run.
4. Define requalification triggers when code, generated data, roster, stage, item, SDK or emulator changes.

**Required tests/evidence:** T-COVER manifest-set audit and negative missing mechanism; scope distinguishes all individual coverage from sampled interaction combinations.

**Work or dependency retired:** Unknown release scope and stale guessed hardest-stage assumptions.

**Done:** The matrix is finite, source-derived, reproducible and captures all required mechanisms with explicit interaction sampling limits.

**Stop/revert:** Do not call a guessed roster/stage the global argmax; name the measured search population.

#### N10.02 — Run controlled and natural four-way performance qualification

**Depends on:** N10.01, N09.05, N00.04

**Edit/inspect boundary:** `scripts/verify-p2-four-fighter-stress.ps1`; `product-performance evaluator`; `final shipping and matching profiling builds`.

**Implementation sequence**

1. Run controlled source-state/input tours for comparable workload attribution after numeric changes and natural source-normal matches for real behavior.
2. Rederive measured cost leaders over current landed content, test mixed/duplicate four-way configurations and one-human/three-CPU play as well as four CPUs.
3. Grade work and cadence separately on valid full populations and report P99/max/consecutive misses, resource/engagement and clock coverage.
4. Publish deficit and next highest-impact remaining owner when RED; do not rescue the result by dropping samples or disabling content.

**Required tests/evidence:** T-MEAS product work/cadence limits plus T-COVER positive four-way interaction; final ROM identity and current source item law.

**Work or dependency retired:** No runtime work by itself; this proves or rejects the integrated performance endpoint.

**Done:** The standing complete supported stress configuration passes both required gates, with search/coverage scope recorded.

**Stop/revert:** Below-target FPS despite a good isolated kernel keeps performance OPEN.

#### N10.03 — Qualify lifecycle, memory and service stability

**Depends on:** N09.05, N02.06, N08.07

**Edit/inspect boundary:** `shell/lifecycle verification scripts`; `final bank/service owners`; `source-controlled transition tours`.

**Implementation sequence**

1. Test CSS→battle→results→CSS, rematch, sudden death, pause, KO/respawn, copy/morph, intro and shipped mode transitions.
2. Collect allocation/stack/TCM/graphics/packet/IPC and audio service evidence; test interrupted admission/cancellation and repeated arena address reuse.
3. Use long natural soaks for leak and rare deadline behavior only after focused tests pass; keep timing measurements isolated from concurrent correctness runners.
4. Confirm no mandatory locked-epoch motion/texture reads and no undeclared service clients appear late in a match.

**Required tests/evidence:** T-LIFE/T-RES/T-AUDIO/T-IPC applicable tests, full coverage beyond the stress script’s 60→1 clock window.

**Work or dependency retired:** No runtime work by itself; validates long-lived ownership and resource closure.

**Done:** No leaks/stale handles/late writers/underruns or unqualified scene gaps remain in the claimed shipped scope.

**Stop/revert:** The 1,972-frame battle script does not cover results/rematch; missing lifecycle coverage blocks this task.

#### N10.04 — Qualify visual, mechanical and numeric completeness

**Depends on:** N09.05, N10.01

**Edit/inspect boundary:** `independent geometry/source oracles`; `semantic fixture suite`; `proposed whole-runtime no-float checker`.

**Implementation sequence**

1. Run exact discrete gameplay/event/order tests, bounded continuous error reports and native geometry/material/transform tests across the qualification matrix.
2. Capture tricky visual states with expected-owner/resource identities and actual pixels; preserve all required content regardless of aggregate error counters.
3. Run the no-float audit on every shipped scene/service root, including menus and cold callbacks, with no temporary migration allowance.
4. Record unresolved pre-existing content defects separately; they cannot be ignored when claiming full-game completeness.

**Required tests/evidence:** T-NUM/T-CLOCK/T-PHYS/T-COLL/T-HIT/T-AI/T-ORDER/T-GEOM/T-DEPTH/T-UI/T-FLOAT.

**Work or dependency retired:** No runtime work by itself; proves representation and fidelity closure.

**Done:** Claimed shipped scope has complete native mechanics/presentation and fixed runtime without disguised IEEE helpers.

**Stop/revert:** A matching sampled state hash alone or zero no-op error counters are insufficient.

#### N10.05 — Reproduce the release from declared inputs

**Depends on:** N10.02, N10.03, N10.04, N09.06

**Edit/inspect boundary:** `Makefile`; `build.ps1`; `scripts/verify-all.ps1`; `declared source/generated asset prerequisites`.

**Implementation sequence**

1. Rebuild the natural-input P2 target from recorded source and declared ignored derived inputs without relying on unrelated dirty files.
2. Run generator-staleness, native-only, runtime numerics, ABI/architecture and the widest relevant integration profile; inspect all final verdicts.
3. Pin final ROM/ELF/config/generator/emulator/verifier identities and attach timing, content, memory and service coverage.
4. Confirm the published target is smash64ds.nds, not a fast-logic/tick-hud/forced-item lab target or the frozen P1 artifact.

**Required tests/evidence:** T-BUILD clean reproducibility and exact final identity; no broken umbrella bypassed as GREEN.

**Work or dependency retired:** Accidental undeclared dependencies and false publication acceptance.

**Done:** A reproducible release artifact is covered by all required final gates.

**Stop/revert:** Compile success or a prior green hash does not qualify a changed ROM.

#### N10.06 — Close or report the precise remaining gap

**Depends on:** N10.05

**Edit/inspect boundary:** `docs/P2_EXECUTION_BOARD.md`; `docs/HANDOFF.md`; `docs/PERF_LEDGER.md`; `final campaign evidence`.

**Implementation sequence**

1. Publish measured before/after work, reduction percentage, cadence, tail, memory and explicit content/numeric scope.
2. Close P2-2p8 performance only when its standing gates pass; keep other P2 content rows independent.
3. List retired work categories and retained architecture, not total lines rewritten or projected savings.
4. If later content changes the argmax or reintroduces a dependency, reopen the existing row with the new specific deficit and violated mechanism.

**Required tests/evidence:** T-DOC final report agrees with artifacts and product gates; no unsupported all-content or speedup claims.

**Work or dependency retired:** Ambiguous completion claims and duplicated status tracking.

**Done:** The board accurately distinguishes completed performance, fixed-runtime closure and any remaining full-game work.

**Stop/revert:** No measured pass means no closure; preserve the recoverable checkpoint and next concrete owner instead.



---


<a id="doc-15-source-index"></a>

## Source index and evidence scope

All repository anchors below refer to `75f7f6b4b4864c82c01872d0fd2771d171005272`. `master` was rechecked through GitHub and still pointed to this commit when this implementation plan was prepared. Sources read in the prior research are distinguished from sources rechecked now. No new game build/benchmark, all-file exhaustive source audit, or independently reproduced performance result is claimed.

The supplied research document `Smash64DS_Native_Optimization_Plan.md` is supporting analysis. Current product/procedure/residency files override historical experiment-specific restrictions where their scopes differ. Any fresh working-tree divergence requires re-resolving the affected anchors.

### S01 — Current product contract

[`PROJECT_GOAL.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/PROJECT_GOAL.md)

Mechanical equivalence, native-only all-ROM rendering, original-DS performance and permitted implementation changes. Read in prior audit at the unchanged pinned SHA.

### S02 — Current execution and checkpoint

[`docs/P2_EXECUTION_BOARD.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/docs/P2_EXECUTION_BOARD.md)

Existing P2-2p8 ownership, retained work, content gaps and current queue. Cross-check against HANDOFF; not a second live queue.

### S03 — Current build, verification and measurement procedure

[`docs/VERIFYING.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/docs/VERIFYING.md)

Read again for this implementation plan. Defines serial builds/timing, actual targets, native/resource versus product gates, sample coverage and accurate melonDS policy.

### S04 — Final running-joint-mask checkpoint

[`artifacts/performance/2026-09-15_p2-2p8-pose-joint-mask/README.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/artifacts/performance/2026-09-15_p2-2p8-pose-joint-mask/README.md)

Recorded final hard-on WORK-H/cadence/memory, not a fresh benchmark. Distinguish its final hash from same-ROM A/B artifacts.

### S05 — Preceding diagnostic instruction census

[`artifacts/performance/2026-09-15_p2-2p8-pose-track-mask/poseplay-pc.txt`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/artifacts/performance/2026-09-15_p2-2p8-pose-track-mask/poseplay-pc.txt)

Diagnostic section occupancy/cost candidates. Not final shipping truth or proof of global deadness.

### S06 — Deterministic scene residency contract

[`docs/p2/P2-texture-residency.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/docs/p2/P2-texture-residency.md)

Read again. Required set identity, pre-GO admission, zero mandatory motion/texture demand reads, declared BGM and transactional lifetime.

### S07 — Renderer placement/state/packet anchor

[`src/nds/nds_renderer_preamble.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/nds/nds_renderer_preamble.c)

Current shared section macros, native packet state/recorder/patch/submission and global runtime state. Prior source audit; inspect exact function bodies before edits.

### S08 — Actual linked memory policy

[`linker/nds_hot_text.ld`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/linker/nds_hot_text.ld)

Explicit ITCM, .32.o side effects, DTCM stack ceiling and main-RAM hot-text groups. Preserve startup/OS invariants.

### S09 — Native fighter production executor

[`src/nds/nds_renderer_native_fighter_production.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/nds/nds_renderer_native_fighter_production.c)

Preflight before replay, root/foreign table bindings, split/GX paths and production state. Call order is observed; savings require measurement.

### S10 — Shared native preparation

[`src/nds/nds_renderer_native_common.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/nds/nds_renderer_native_common.c)

Source-shaped eligibility and vertex preparation plus shared native stage/fighter work. Reachability is not inferred solely from source presence.

### S11 — Native actor/owner execution

[`src/nds/nds_renderer_native_owners.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/nds/nds_renderer_native_owners.c)

TaruCann configuration/traversal/vertex setup and actor-specific native contracts; a pilot example, not measured dominant CPU cost.

### S12 — Renderer translation-unit organization

[`src/nds/nds_renderer.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/nds/nds_renderer.c)

Textual includes of implementation fragments; source splitting is not automatically actual module or runtime simplification.

### S13 — Compact pose and event clock

[`src/nds/nds_ft_pose.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/nds/nds_ft_pose.c)

Fixed pose with source fields, binary32 integer clock, timing mismatch history, active masks, required updates and diagnostic state.

### S14 — Current pose interface

[`include/nds/nds_ft_pose.h`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/include/nds/nds_ft_pose.h)

Existing native/source pose boundary; task implementation must inspect all readers before ABI replacement.

### S15 — Status and topology mutation seam

[`src/import/battleship_ftmain.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/import/battleship_ftmain.c)

Read-only reference import plus port wrappers; status can change topology without root/heap-generation identity changes.

### S16 — CPU behavior import and observation

[`src/import/battleship_ftcomputer.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/import/battleship_ftcomputer.c)

CPU processing, source decision engine and telemetry; SCPU is nested and requires positive engagement.

### S17 — Transform adapter and special classes

[`src/port/renderer_adapter_matrix.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/port/renderer_adapter_matrix.c)

World/local/camera and source billboard/orientation-replacement semantics; not all callbacks are ordinary TRS.

### S18 — Row accounting and percentile populations

[`scripts/census-tick-hud-p95-set.py`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/scripts/census-tick-hud-p95-set.py)

WORK-H accounting, SRC/GCRA/SINT/SCPU nesting and separate P95/cadence attribution sets. Audit wording that oversimplifies re-ranking; final metrics use full population.

### S18A — Verified four-fighter script/parameters

[`scripts/verify-p2-four-fighter-stress.ps1`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/scripts/verify-p2-four-fighter-stress.ps1)

Read for this plan. Actual script name, defaults, JsonOut/RowsCsv/CoverageJsonOut/MemoryJsonOut and source item-law checks.

### S18B — Harness membership authority

[`scripts/lib/harness-registry.ps1`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/scripts/lib/harness-registry.ps1)

Read for this plan. Boundary/Latest and p2_fourcpu_stress registry mapping; no invented profile names.

### S19 — Independent native geometry oracle

[`scripts/fighters/check_native_owner_geometry_closure.py`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/scripts/fighters/check_native_owner_geometry_closure.py)

Read again. All landed owners/both detail levels; triangle, dense vertex, matrix routing, facing/winding, strip/BEGIN semantics and source-facing exceptions.

### S20 — Motion/track runtime anchor

[`src/nds/nds_ftanim_track.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/nds/nds_ftanim_track.c)

Existing resident/streaming/motion representations to extend, not replace with an unrelated cache framework.

### S20A — NDO6 and special owner coverage

[`artifacts/visibility/2026-09-13_roster-close-kirby-ness-purin.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/artifacts/visibility/2026-09-13_roster-close-kirby-ness-purin.md)

Existing unlit vertex-color/alpha/class allocation and roster work. Search confirms the retained mechanism; verify exact runtime image before ABI changes.

### S21 — BGM native service

[`src/nds/nds_audio_bgm.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/nds/nds_audio_bgm.c)

Existing service/control/range/refill ownership; actual service threading must be inspected in implementation.

### S21A — Retained BGM direct-range evidence

[`artifacts/performance/2026-09-14_p2-2p8-bgm-direct/README.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/artifacts/performance/2026-09-14_p2-2p8-bgm-direct/README.md)

Already-landed BGM work; direct read remains I/O and cannot be sold as newly implemented.

### S22 — FGM native service

[`src/nds/nds_audio_fgm.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/nds/nds_audio_fgm.c)

Required one-shot/cue/control path and retained direct-range work; final all-runtime numeric closure includes it.

### S23 — Stage adaptation anchor

[`src/port/renderer_adapter_stage.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/port/renderer_adapter_stage.c)

Referenced by the current residency contract; implementers inspect actual stage partition and resource owners.

### S24 — Existing native HUD

[`src/nds/nds_battle_hud.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/nds/nds_battle_hud.c)

Native HUD already exists; optimize residual value/layout/update work rather than rediscovering BG/OAM.

### S25 — Particle import/update/draw anchor

[`src/import/battleship_lbparticle.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/import/battleship_lbparticle.c)

Source particle behavior, runtime state and native draw seam; preserve spawn/RNG/lifetime independent of visible output.

### S26 — Object/process import anchor

[`src/import/battleship_sys_objman.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/import/battleship_sys_objman.c)

Actual source registration/dispatch semantics must be traced before optional scheduler changes.

### S27 — Hardware math owner

[`include/nds/nds_r2_hwmath_unit.h`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/include/nds/nds_r2_hwmath_unit.h)

Existing divide/sqrt helpers and historical register-user assumptions; re-audit current threads/IRQs before overlap.

### S28 — Original fighter behavior source

[`decomp/BattleShip-main/decomp/src/ft/ftmain.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/decomp/BattleShip-main/decomp/src/ft/ftmain.c)

Strictly read-only specification for events, interactions and update rules; follow into relevant physics/collision/CPU modules for each task.

### S29 — Existing fighter native generator

[`scripts/fighters/generate_nds_native_owners.py`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/scripts/fighters/generate_nds_native_owners.py)

Path confirmed by scripts README and checker import; reuse its source/native representation and generation dependencies.

### S30 — Generator layout/import ownership

[`scripts/README.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/scripts/README.md)

Read/search-established generator area conventions, with scripts/_paths.py managing shared imports. Do not invent paths from old flat layout.

### S31 — SM64DS load-time relocation example

[`decomp/sm64ds-decomp/src/_ZN5Model17UpdateFileOffsetsER8BMD_File.cpp`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/decomp/sm64ds-decomp/src/_ZN5Model17UpdateFileOffsetsER8BMD_File.cpp)

Inspected in research: bind file offsets once. Pattern, not a drop-in Smash replacement.

### S31A — SM64DS fixed arithmetic example

[`decomp/sm64ds-decomp/src/CrossVec3.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/decomp/sm64ds-decomp/src/CrossVec3.c)

Inspected fixed vector products/rounding. Adopt explicit units/range semantics, not assumptions about all reference code.

### S31B — SM64DS fixed camera example

[`decomp/sm64ds-decomp/src/Camera_UpdateMatrices.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/decomp/sm64ds-decomp/src/Camera_UpdateMatrices.c)

Inspected fixed camera/lookup pattern; do not recreate the port’s already-fixed camera unnecessarily.

### S31C — SM64DS animation same-file path

[`decomp/sm64ds-decomp/src/_ZN9ModelAnim7SetAnimEP8BCA_Filei5Fix12IiEj.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/decomp/sm64ds-decomp/src/_ZN9ModelAnim7SetAnimEP8BCA_Filei5Fix12IiEj.c)

Inspected source shows speed/flags update without full rebinding when applicable; source-specific semantics still need proof.

### S32 — Build graph and target policy

[`Makefile`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/Makefile)

Large current build/source/generated dependency graph. Changes must be scoped and inspected rather than copying flags from another DS SDK.

### S33 — Existing selected VRAM layout contract

[`docs/p2/P2-1c-vram-map.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/docs/p2/P2-1c-vram-map.md)

Referenced by current residency contract; exact bank claims must be read before remapping, not inferred as free memory.

### S34 — Restart pointer

[`docs/HANDOFF.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/docs/HANDOFF.md)

Current short restart boundary, not an additional queue. Pin linked artifact identity and reconcile board differences.

### H01 — BlocksDS optimizing code

[BlocksDS optimizing code](https://blocksds.skylyrac.net/tutorial/advanced/optimizing_code/)

Rechecked for this plan: ARM/Thumb tradeoffs, long multiply, TCM, cache and copying. Hardware background; the project uses its pinned devkitPro/Calico toolchain, so APIs/options must be checked there.

### H02 — BlocksDS TCM and cache

[BlocksDS TCM and cache](https://blocksds.skylyrac.net/tutorial/intermediate/tcm_and_cache/)

Rechecked: CPU-local fast memory and cache behavior. Current project linker/stack proof is the actual allocation authority.

### H03 — BlocksDS DMA

[BlocksDS DMA](https://blocksds.skylyrac.net/tutorial/intermediate/dma/)

Rechecked: DMA cannot access TCM/cache, source visibility, contention and completion. Not a promise of free concurrent main-memory execution.

### H04 — BlocksDS ARM7

[BlocksDS ARM7](https://blocksds.skylyrac.net/tutorial/intermediate/using_the_arm7/)

Rechecked: service ownership and cross-core design. Inspect the actual current ARM7 image before proposing new work.

### H05 — libnds hardware math reference

[libnds hardware math reference](https://blocksds.skylyrac.net/libnds/math_8h.html)

Rechecked: hardware math API/background; shared-unit ownership must match the pinned binary.

### H06 — libnds native 3D interface reference

[libnds native 3D interface reference](https://blocksds.skylyrac.net/libnds/videoGL_8h.html)

Reference carried from the research source index. Confirm exact declarations/command semantics in the installed SDK before implementation; not freshly re-fetched here.

### Not available as proof in this package

There is no current locally built ROM/ELF, compiler run, emulator run, new pixel/audio capture or target benchmark here. The package consists of implementation specifications, source links, a static task graph, evidence templates and a host validator for the plan itself. Connected GitHub reads and the supplied pinned research provide the source evidence; runtime acceptance still requires executing the specified tests.

Existing source locations are anchors, not permission to edit reference code. Descriptors, new checkers and new generated formats labeled proposed are implementation work still to do. Quoted historical numbers remain attached to their original artifact/configuration; planned allocations and targets remain estimates/objectives.



---


## Appendices supplied in the repository package

`tasks.json` and `tasks.csv` contain the static implementation register. `dependency-order.txt` provides one valid task order; `dependency-graph.mmd` is the generated dependency diagram. The `templates/` directory contains unmeasured baseline, experiment, admission, numeric-domain and deletion-ledger templates. `planning-tools/` contains a host plan validator and ten tests; its checks are not game verification.
