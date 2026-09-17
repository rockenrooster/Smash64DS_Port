# Shared implementation contracts

These are **proposed native interfaces and rules to implement**, not a claim that the current tree already exposes these APIs. Keep public names consistent with existing ownership where possible. Add only interfaces that have both a real producer and consumer. [S07–S17, H01–H06]

## C1. Domain ownership and minimal interfaces

| Boundary | Producer | Consumer | Native payload | Not allowed in the hot consumer |
|---|---|---|---|---|
| Source assets → native bank | Existing host generators | Scene admission | Versioned little-endian tables, offsets, fixed constants, native GX words | N64 command decoding, endian conversion, source pointer relocation |
| Admission → bound instance | Scene/spawn/topology owner | Native draw/pose | Validated table pointers and resource handles | File opens, format discovery, general texture lookup |
| Simulation → presentation | Source-ordered fixed game loop | Pose/draw | Position/facing, pose index/phase, material/visibility generations | Reinterpretation of fixed words as floats |
| Pose → gameplay sockets | Native pose evaluator | Hit/grab/weapon logic | Required fixed local/world matrices or socket vectors | GPU matrix readback; full visual hierarchy rebuild |
| Pose → GX | Native pose evaluator | Packet patcher | Affine matrices and typed state patches | Repeated float conversion and all-purpose traversal state |
| Service → game | Audio/storage/ARM7 owner | Game thread | Bounded completion records | Hidden blocking joins or stale shared pointers |

A temporary migration shim may present legacy arguments to unmigrated code, but the dependency register must name each reader/writer and the task that removes it. Do not add temporary conversions to the final hot ABI. Existing source-compatible `FTStruct`/DObj fields cannot simply be reinterpreted as fixed values while old arithmetic still reads them.

## C2. Numeric representation

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

## C3. Bound draw description

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

## C4. Mutation and invalidation table

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

## C5. Packet and graphics lifetime

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

## C6. Native asset bank format

Extend an existing native bank where possible. Introduce a version only when its representation changes. Proposed header fields are: magic/version, header bytes, total bytes, source-content hash, converter/schema hash, table directory, resource profile ID and payload integrity value. Individual tables carry kind/count/offset/stride/alignment. Wire formats are explicitly little-endian, offsets are relative to the bank, and no host pointer/padding is serialized.

Validation order: fixed header → size limits → table-directory bounds → offset/alignment/count multiplication → referenced spans → resource/command domain → semantic identities → publish runtime pointers. Bounds use subtraction-based checks that cannot wrap. Checksums catch corruption but cannot prove semantic coverage; compare required identities, not only hashes or equal counts.

Bank lifetime allows immutable sharing across same-kind fighters. Instance event cursors, visibility, costume state and patch buffers are never shared mutable state. Record output ROM bytes, permanent resident bytes, load scratch, transition peak and unavoidable streaming bytes separately.

## C7. Admission and memory

Before GO, atomically admit the required fighter/motion/material/effect/item/stage set for the selected legal scene. Required post-GO motion/texture demand reads are zero. Streaming BGM is a distinct declared service with reserved buffers/deadlines. One-shot cues need a separately qualified resident/deadline-safe policy. [S06]

Budget **transient peak**, not just final footprint. A transactional bank switch that retains old resources while building new ones can OOM even when each final scene fits. Pre-reserve the combined scratch or plan an explicitly quiescent retirement boundary before loading. Preserve UI responsiveness during loading; loading work is cheaper than battle work, not permission to freeze the shell indefinitely.

Memory refusal states why the scene cannot be admitted. Refusal is containment, not a completed port. Do not disable a fighter/effect to make admission succeed. CSS preview banks are distinct from a battle's full gameplay banks and should not demand residency of the entire roster simultaneously.

DTCM is CPU-local. The current qualified data ceiling is `0x02ff3000`; re-prove stack boundaries before raising it. ARM7/DMA shared data stays in legally visible non-TCM storage. [S08, H02]

## C8. Numeric and semantic comparison classes

**Class E (exact):** pure refactor/placement, resource identity, source event order, integer counters with gameplay meaning, RNG draw order, packed geometry/source topology and unchanged native command semantics.

**Class B (bounded continuous error):** fields whose representation intentionally changes. Report max absolute error, boundary distance, overflow margin, cumulative drift and downstream decisions. Tolerance must be derived from quantization and the source domain, not a blanket pixel/world-unit epsilon.

**Class A (approval required):** different quality, temporal update rate, simplified game rules or removed content. Keep out of the primary transparent optimization path until approved with evidence.

A B-class field crossing an E-class decision boundary needs explicit source-case proof/correction. Do not accept different hit/ledge/status timing because the position difference is numerically small. Whole-match state hashes may differ after permitted continuous differences; use reproducible short state fixtures plus long natural behavior/engagement coverage instead of weakening every assertion.

## C9. Universal roster-stage ownership and acceptance

The universal contract in [00_MASTER.md](00_MASTER.md) applies to every native interface. Roster/stage-specific specialization is allowed; roster/stage-specific exclusions are not. Shared functions and compile-time capacities must be valid for the complete required owner/variant set, with source-derived bounds rather than pilot counts.

An instance key includes stable slot/object identity and generation. Fighter kind identifies immutable bank content, not mutable pose, material, visibility, packet, hit, CPU target or attachment state. A four-copy lineup owns four independent mutable instances even when all static geometry is shared. A single scratch block is legal only with proven serialized lifetime and no retained pointer into overwritten data.

Admission validates all constraints over each legal case, including legal simultaneous children/copies and temporary transitions. A source-backed bound or mutually exclusive resource class can reduce a conservative union. A guessed absence from one CPU run cannot. Refusal on true resource failure is safe containment, but the legal case stays RED until its representation/allocation supports it.

Equivalence is property-specific: identical resource unions may justify reusing a static solver result; identical rendered geometry does not prove source-ordered hit priority or equal CPU/cadence. Every reused ordered-slot result names the exact covered cases, proof scope and dependency hashes. No implied slot symmetry. See [16_ALL_ROSTERS_ALL_STAGES.md](16_ALL_ROSTERS_ALL_STAGES.md).

Performance and cadence are graded per scenario and required run, then combined with logical AND. Unfinished content, missing cases, stale identities and discovered legal failures remain explicit. Finite validation histories are evidence for the declared matrix, never a proof of every possible gameplay sequence.
