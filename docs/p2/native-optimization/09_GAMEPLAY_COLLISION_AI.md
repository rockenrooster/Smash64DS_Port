# N06 — Compact fixed gameplay, collision, AI and scheduling

> **Revision 2 coverage:** Universal scope: all ordered fighter interactions, legal slot/team/control settings and every stage collision/hazard class are supported. Six unordered broad pairs never replace directed hit/grab/reflect outcomes; level-3 CPU profiling is not an AI cost upper bound. See [16_ALL_ROSTERS_ALL_STAGES.md](16_ALL_ROSTERS_ALL_STAGES.md).

**Purpose:** remove source-shaped hot data and repeated work beyond the renderer. Sources [S15, S16, S26, S28]. Original BattleShip code remains the behavior reference, not a writable optimization target. Port-owned replacements/import shims own the DS implementation.

## Migration boundary

Build a field-use closure before replacing `FTStruct`/DObj semantics. A conceptual hot record contains native position/velocity, status/timers, input, facing, active collision volumes and compact attachment handles; cold attributes, descriptions and debug history remain separately referenced. This is not a new general entity framework. Do not duplicate a full FTStruct and copy it every tick.

For the first fixed movement slice, temporarily adapt at a documented phase boundary while unconverted hit/AI code still uses source types. The final fixed gameplay task converts those consumers and deletes the bridge. Never globally redefine `f32` to an integer or write fixed bits into fields still consumed by source floating-point arithmetic.

## Update and ordering rules

Document the current source process order from registration and dispatch, not comments alone. Input/AI, authored events, movement/map collision, hit/catch searches, damage resolution, status changes, spawns/deletes, audio and presentation can observe each other within one logic tick. A shared query snapshot is valid for a **specific source read phase/generation**, not automatically the whole tick. Update or invalidate it after relevant writers.

Compute reusable deterministic facts once per valid phase: positions/bounds, floor-line metadata, candidate masks and source-eligible targets. Never share or eliminate RNG calls merely because two CPUs query similar facts. Source RNG consumption and directed hit/catch resolution order remain exact discrete contracts.

Four fighters provide six unordered broad-phase pairs, but attacks, grabs, shields, ownership and outcomes remain directed. Sort or enumerate admitted candidates in source order; narrow-phase/stateful resolution must not be parallelized or reordered silently.

## Collision representation

Compile static stage line metadata: endpoints, line identity/kind, source flags, adjacency, conservative bounds and necessary constants. Use compact per-region or per-line candidate sets appropriate to actual stage size; a sophisticated BVH is not automatically justified. Moving/hazard collision data has an explicit transform/generation.

Broad-phase rejection must be conservative: outward rounding and motion envelopes cannot reject a collision the source narrow phase would accept. Use exact integer signs/order for topology tests where possible; prove product/sum ranges. Preserve pass-through, ledge, slope, grab/capture and platform-motion semantics. Keep geometric hit tests and gameplay hit priority as different layers.

### N06.01 — Map hot game state and source process order

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

### N06.02 — Convert a complete movement and map-query slice

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

### N06.03 — Compile stage collision metadata and conservative candidates

**Depends on:** N06.01, N04.03, N02.03

**Edit/inspect boundary:** `existing native stage generator`; `existing stage collision wrappers`; `scripts/check-mp-floor-crossing-fixtures.ps1`; `scripts/check-mp-topology-fixtures.ps1`.

**Implementation sequence**

1. Generate native line/adjacency/kind/bounds metadata directly from source stage collision data, preserving line identities and ordering.
2. Choose a small conservative candidate structure from measured query counts and memory cost. Keep an exact ordered scan as the host oracle, not a permanent competing runtime engine.
3. Implement fixed broad/narrow predicates with proved width/rounding and separate moving-platform generations.
4. Test swept movement and near-boundary rejection, including negative coordinates, corners, degenerate/parallel lines and maximum displacement.
5. Cover every selectable stage's collision/hazard classes in the catalogue and use the appropriate stage-local transform. Do not assume Dream Land bounds, static platforms or one global moving-platform state.

**Required tests/evidence:** T-COLL source-equivalent candidate superset and resolved contact; missed-collision negative mutation must fail; existing topology/floor fixtures plus moving hazards.

**Work or dependency retired:** Repeated static line property calculation and unnecessary narrow queries for rejected geometry.

**Done:** Generated metadata is complete and broad phase never excludes a source-valid contact in admitted domains.

**Stop/revert:** Do not adopt a tree/bin structure whose setup/cache cost exceeds the small ordered scan or whose rounding drops contacts.

### N06.04 — Convert interaction consumers and close gameplay pose bridges

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

### N06.05 — Share AI query facts without changing CPU decisions

**Depends on:** N06.04

**Edit/inspect boundary:** `src/import/battleship_ftcomputer.c`; `port-owned native AI query layer`; `existing stage floor/target query helpers`.

**Implementation sequence**

1. Price SCPU exclusively and identify repeated deterministic facts: target position/bounds, source eligibility, floor relationships and hazard visibility.
2. Build compact phase/generation-owned facts shared by relevant CPU readers; refresh after any source-visible writer, not merely once per presented frame.
3. Convert distances/angles/comparisons through full fixed query chains while preserving selected CPU levels, reaction delays and RNG call count/order.
4. Keep source decision rules and callback sequencing; do not use lower-frequency decisions or an ARM7 one-frame-late result as an invisible optimization.
5. Use stable slot/instance identities for targets. Test equal-kind opponents, teams, each selectable CPU level and source-ordered target changes. Do not assume level 3, level 9, or four CPUs is automatically the cost leader for every behavior.

**Required tests/evidence:** T-AI all admitted CPU levels, recovery/attack/target switching, items and teams; T-ORDER exact RNG; positive button/movement/action engagement.

**Work or dependency retired:** Repeated deterministic AI fact calculation and associated float/conversion chains.

**Done:** Source-controller decisions match the specified discrete behavior and CPU cost is measured separately from its parent bucket.

**Stop/revert:** Four active GObjs with idle/no-op AI do not prove CPU engagement. Reject stale-phase shared facts even if average behavior looks similar.

### N06.06 — Replace hot object traversal with bounded active sets

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

### N06.07 — Evaluate scheduler flattening only when priced

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

### N06.08 — Expand fixed gameplay to all shipped domains

**Depends on:** N06.04, N06.05, N06.06

**Edit/inspect boundary:** `src/import`; `port-owned native gameplay modules`; `source-derived gameplay coverage manifest`; `proposed runtime numeric gate`.

**Implementation sequence**

1. Convert remaining fighter specials, weapons, items, stage hazards, damage/knockback, cameras and mode-specific gameplay consumers by source-use closure.
2. Include non-VS and rare scene/state entry points reachable in shipped builds; distinguish currently unimplemented content from converted accepted content.
3. Remove completed-domain legacy structs/bridges/functions and update imports/build membership without editing reference source.
4. Run deterministic short semantic fixtures and long natural stress; permitted continuous changes can alter later workload, so also use controlled workload tours for timing attribution.
5. Cover every legal ordered fighter pairing within four-way fixtures, all selectable stages, repeated fighter kinds, source-legal team/self exclusions and human-plus-CPU controls. Reuse conservative pair facts but never erase directed hit/grab/reflect ownership.

**Required tests/evidence:** T-PHYS/T-COLL/T-HIT/T-AI/T-ORDER breadth, T-FLOAT domain audit, T-RES peaks and T-COVER actual content.

**Work or dependency retired:** Remaining gameplay float chains, old hot-state authority and conversion shims in claimed converted scope.

**Done:** Every shipped gameplay arithmetic root has a native implementation or remains an explicit final-closure blocker.

**Stop/revert:** Do not call a domain fixed because its typical state avoids an unmigrated special/cold callback.

