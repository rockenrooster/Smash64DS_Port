# Smash64DS: architecture and completion-speed review

Date: 2026-09-04  
Final review basis: `master`, commit `505234457af0b2f3aff00439605ad6b262ceaede`  
Scope: targeted source, generator, reference-code, build/harness and planning audit. This is not a full line-by-line audit, a new DS build, an emulator benchmark, or a capacity certificate.

## Decision

Keep the behavioral-reference / specialized-DS-runtime architecture. Do not restart the engine. Concentrate work on making the existing fast paths cover the full content set, establishing scene-specific resource bounds, and preventing incomplete or mismatched evidence from directing work.

The important schedule change is to restore the four-fighter memory/performance work as a prerequisite, rather than keep expanding content on a parked scaling instrument. Independent source import and host conversion can continue in parallel; content must not be called accepted merely because it compiles or boots.

Several sophisticated changes already exist, including native fighter owners, generated packets, compact fixed-point pose evaluation, source comparisons, and a compact restart board. These are foundations to preserve, not suggestions to implement again.

## Evidence that changes the plan

### Current capacity and performance status

The current execution board marks the four-fighter phase NOT green. It records an accepted historical ALL P95 of 2,238,464 ticks on Mario/Fox mirrors, a parked four-CPU arm, and failure to fit four distinct kinds. This is not a fresh measurement of the current full-content four-distinct configuration. Do not use the older branch's 8.6M, 7.9M or 2.615M figures as interchangeable current baselines.

The same board records that eight added stages are selectable but do not submit native map geometry. This is a source-backed production-pipeline gap, not eight independent renderer mysteries.

Sources: [execution board](https://github.com/rockenrooster/Smash64DS_Port/blob/505234457af0b2f3aff00439605ad6b262ceaede/docs/P2_EXECUTION_BOARD.md), [goal](https://github.com/rockenrooster/Smash64DS_Port/blob/505234457af0b2f3aff00439605ad6b262ceaede/PROJECT_GOAL.md).

### A current bug-row contradicts the current code

`docs/BUGS.md` says the Results fighter draw lacks the `no_oracle` save/restore bracket. At this same commit, `renderer_adapter_fighter.c` already contains it around `ndsFighterMarioFoxDLAllDrawForSlot`:

```c
saved_no_oracle = ndsRendererHardwareNoOracleEnabled();
ndsRendererHardwareSetNoOracle(TRUE);
/* fighter draw */
ndsRendererHardwareSetNoOracle(saved_no_oracle);
```

The bracket is conditional on build flags. Its presence does not prove that the shipped Results scene renders correctly or uses the intended route. It does disprove the instruction to add an absent bracket at that location. The next action is to verify the compiled configuration and runtime route, not apply a duplicate patch.

Sources: [bug row](https://github.com/rockenrooster/Smash64DS_Port/blob/505234457af0b2f3aff00439605ad6b262ceaede/docs/BUGS.md), [draw implementation](https://github.com/rockenrooster/Smash64DS_Port/blob/505234457af0b2f3aff00439605ad6b262ceaede/src/port/renderer_adapter_fighter.c#L4340-L4460).

### A clock-exactness premise has a host counterexample

The pose implementation claims Q12 preserves event boundaries for source speeds including small-integer ratios. The original parser subtracts a float32 speed from its float32 wait and advances when the wait is nonpositive. The new path rounds the speed to Q12 and subtracts that integer.

For wait 1 and speed 1/3:

- float32 crosses after 3 subtractions;
- the Q12 speed is 1365;
- after 3 subtractions, the Q12 wait is `4096 - 3 * 1365 = 1`;
- the Q12 path crosses on tick 4.

The supplied script reproduces this, also reproduces a wait-16 / speed-16/3 case, and verifies a 1/2-speed control. It uses only the Python standard library.

This refutes the general exactness argument. It does NOT establish that a specific live move uses that exact wait/speed pair or has a gameplay-visible defect. Some states have independent timers. Rebound does establish that ratio-derived speeds are relevant enough to audit.

Keep fixed-point pose values. Add a control-clock differential test over actual legal speeds, waits, initial frames, speed changes, hitlag, attach/end/loop paths and held-body updates. Compare event boundaries and published GObj time, not merely pose error. Do not add a global epsilon or assume mathematically exact rational time automatically matches the float source.

Sources: [pose engine](https://github.com/rockenrooster/Smash64DS_Port/blob/505234457af0b2f3aff00439605ad6b262ceaede/src/nds/nds_ft_pose.c), [source parser](https://github.com/rockenrooster/Smash64DS_Port/blob/505234457af0b2f3aff00439605ad6b262ceaede/decomp/BattleShip-main/decomp/src/ft/ftanim.c), [source rebound](https://github.com/rockenrooster/Smash64DS_Port/blob/505234457af0b2f3aff00439605ad6b262ceaede/decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommonrebound.c).

### The field certificate is not a transitive consumer proof

The native-stage generator extracts named C function bodies and scans their pointer-field syntax. The adapter's active-stage accessor comment explicitly describes putting descriptor arrow reads in untracked helpers. These mechanisms can be useful local change detectors. They do not establish a whole-program set of raw asset consumers.

Before using an expanded certificate to justify freeing original data, cover reachable callees, approved opaque boundaries, aliases and callbacks, or enforce a narrower access API that the battle build cannot bypass. A generated field classification is not itself semantic completeness.

Add negative tests: a newly introduced raw read directly, through a helper, through an alias and through a registered callback must either enter the reviewed dependency model or fail the check. Do not claim these mutation tests were run by this review.

Sources: [stage generator](https://github.com/rockenrooster/Smash64DS_Port/blob/505234457af0b2f3aff00439605ad6b262ceaede/scripts/stages/generate_nds_native_stage.py), [adapter descriptor/accessors](https://github.com/rockenrooster/Smash64DS_Port/blob/505234457af0b2f3aff00439605ad6b262ceaede/src/port/renderer_adapter_matrix.c#L472-L565), [existing independent review](https://github.com/rockenrooster/Smash64DS_Port/blob/505234457af0b2f3aff00439605ad6b262ceaede/docs/reviews/Independent_Review_P2_Residency_and_Four_Fighter_Plans.md).

## Recommended implementation sequence

The following are proposed tasks, not claims that implementation or measurement has been completed.

### Task 0 — trustworthy artifacts and acceptance state

Extend the existing build configuration, telemetry and verifier records. Avoid a separate new dashboard/framework.

Each measurement must identify the exact ROM/ELF pair, effective flags, generated assets, emulator configuration, scenario and input/seed. A parked arm is NOT RUN; an intentionally non-gating performance observation is not a performance PASS. Keep functional, presentation, residency and timing verdicts separate.

Correct the Results row only after distinguishing source presence from compiled and executed route. Retain the frozen P1 artifact as a regression reference; do not assume P2 inherits its performance simply by using the same two fighter kinds.

The current registry explicitly says four-CPU timing is debt rather than a passing timing gate. Keep that distinction in every generated summary.

Acceptance: a run cannot silently consume another configuration's ELF/ROM or report overall completion with mandatory gates parked.

Sources: [registry](https://github.com/rockenrooster/Smash64DS_Port/blob/505234457af0b2f3aff00439605ad6b262ceaede/scripts/lib/harness-registry.ps1), [verifier](https://github.com/rockenrooster/Smash64DS_Port/blob/505234457af0b2f3aff00439605ad6b262ceaede/scripts/verify-all.ps1), [Makefile](https://github.com/rockenrooster/Smash64DS_Port/blob/505234457af0b2f3aff00439605ad6b262ceaede/Makefile).

### Task 1 — capacity estimator before a complete new pack loader

Use one asset identity and dependency vocabulary, but keep storage, main RAM, texture bytes, palette placement, software handles and geometry capacity as independent budgets.

Extend the existing fighter manifest tools. Initially permit conservative intervals: known retained bytes, known removable bytes, replacement bytes, and unresolved contributions. An unknown must not become zero. Include the exact linked-image and per-instance costs for the tested build.

Account for the peak over scene phases, including old/new scene overlap, staging, decompression/binding scratch, audio buffers, all object pools and final resident data. An end-state pack fitting is insufficient if construction cannot fit.

Retain whole small, fully described semantic tables. Concentrate reduction on redundant N64 geometry, unselected presentation resources, padding and setup-only representations. Late reset/reconstruction inputs are live: current mutable state is not a substitute for authoritative defaults.

Deduplicate immutable atoms, not mutable fighter instances or palettes merely equal at load time. Enumerate distinct-kind unions plus the legal costume, copy-capability, team, item, stage and instance-state variations. Recompute the worst set after transformation.

Do not promote 512 KiB recovery or the earlier conditional 175,604-byte allowance to established capacity facts. First produce a same-build ledger and candidate bound. Integrate a representative hard fighter only after the estimate justifies it.

Acceptance: every uncertainty is named; setup and steady-state peaks fit or explicitly fail; no mandatory combat asset is fetched on demand after GO. Scheduled BGM streaming is a separate policy, not a contradiction hidden in the counters.

Sources: [live-set review](https://github.com/rockenrooster/Smash64DS_Port/blob/505234457af0b2f3aff00439605ad6b262ceaede/docs/reviews/Review_Deriving_Fighter_Live_After_Setup_Set.md), [amendments](https://github.com/rockenrooster/Smash64DS_Port/blob/505234457af0b2f3aff00439605ad6b262ceaede/docs/reviews/Independent_Review_P2_Residency_and_Four_Fighter_Plans.md), [current handoff](https://github.com/rockenrooster/Smash64DS_Port/blob/505234457af0b2f3aff00439605ad6b262ceaede/docs/HANDOFF.md).

### Task 2 — finish the stage compiler, then generate stages in batches

The adapter has eight table entries pointing to Dream Land and also returns Dream Land for out-of-range kinds. A selectable stage must instead resolve its own generated identity, topology, assets, material programs and packet.

Finish descriptor threading, per-stage symbol/data isolation, and generated workspace bounds. Prove Dream Land remains unchanged where intended. Finish one contrasting stage through generator, load, native draw, hazards and scene transition; then expand the same pipeline to the remaining stages.

Prefer selected-stage data in a scene-owned native blob rather than permanently linking every stage's geometry into the ARM9 image. Keep the small registry resident. Missing mandatory descriptors fail before entering the scene; they must not masquerade as Dream Land.

Share immutable converted geometry with SSS preview production where appropriate, while giving preview and battle distinct instances, cameras, dependencies and lifetimes. Separate stage core geometry from dynamic actors and particles so an effect failure cannot revoke the complete stage fast path.

Acceptance: selected kind equals generated descriptor/asset identity; meaningful native geometry submits; required actor and hazard states are exercised; mandatory fallback is absent. Collision and a successful boot alone do not close the task.

Source: [actual adapter](https://github.com/rockenrooster/Smash64DS_Port/blob/505234457af0b2f3aff00439605ad6b262ceaede/src/port/renderer_adapter_matrix.c#L472-L565).

### Task 3 — one effect-residency and native-display completion effort

Treat the fifth particle sheet, wider fighter-script reachability, missing stock-effect scripts and relevant item effects as one shared resource-planning problem, not separate local patches.

The current bug analysis identifies an 8,192-byte fifth sheet, a second shared-capacity assertion, generated palette offsets/sizes and downstream allocation failures. A sheet-count edit alone is not a complete fix. Validate actual VRAM placement and the entire scene lifecycle, including Results.

Require exact mandatory resource IDs, not only admitted counts or total bytes. Add narrow owner-level route/failure reporting through existing shipping telemetry. Keep first inner failure information instead of only outer reason 6.

Finish native display in an order guided by actual command/tick contribution. Keep gameplay-relevant source weapon/item/effect objects and replace their display work with compact native records and packets. Do not sort translucent effects across source ordering constraints without a proof of equivalence.

Shield is a separate policy choice: a native path exists, but changing the owner-approved model route needs a representative remeasurement and a new owner ruling. Do not silently flip it.

Acceptance: every enabled required script has its dependencies; texture/view/palette limits are satisfied; mandatory generic fallback and silent missing draws are separately detected; a local optional effect cannot demote the stage.

Sources: [bug analysis](https://github.com/rockenrooster/Smash64DS_Port/blob/505234457af0b2f3aff00439605ad6b262ceaede/docs/BUGS.md), [VRAM review](https://github.com/rockenrooster/Smash64DS_Port/blob/505234457af0b2f3aff00439605ad6b262ceaede/docs/reviews/Review_DS_Texture_VRAM_Residency.md).

### Task 4 — bounded CSS representation and storage qualification

Keep selection immediate. Do not put a synchronous full battle-closure setup into hover handling.

First estimate exact compact preview-only data, including the preview animations and costumes that can really be requested. If all twelve descriptions fit, preload them at a menu boundary and keep only visible live instances/textures. Otherwise use bounded, latest-selection-wins loading of the same representation, with cancellation generations and no queue of obsolete cursor positions.

Reuse the indexed bank reader and validation with the battle pack, but not its lifetime or gameplay roots. Qualify the actual file-backed NitroFS/DLDI route. An archive inside NitroFS does not automatically eliminate outer-ROM seeks or audio contention. Measure returned bytes/progress and active operations rather than infer throughput from a timeout.

The existing CSS review identifies the roughly 154 KiB compressed menu-music container as a possible RAM-backed source. Treat that as a measured menu-budget trade, not a blanket instruction to retain all audio.

Acceptance: UI interaction remains responsive, stale jobs cannot publish into a new selection, audio deadlines are accounted for, and the storage backend is actually the one used for the intended launch route.

Source: [CSS/storage review](https://github.com/rockenrooster/Smash64DS_Port/blob/505234457af0b2f3aff00439605ad6b262ceaede/docs/reviews/Review_CSS_Lazy_Load_and_NitroFS.md).

### Task 5 — optimize the clean runtime, preserving discrete combat semantics

Only after residency and native routes are known should a new cost census decide the remaining architecture.

Preferred direction: two slim 60 Hz combat substeps inside one 30 Hz presentation. Preserve source status/event/collision/RNG order; move only proven presentation work to the presentation phase. Do not turn two source integrations into one naive dt=2 update.

Generate a gameplay-joint dependency closure including ancestors, hit/hurt boxes, throws, shields/reflectors, attachments, held items, copy capabilities and late topology changes. Reuse transforms only at the same source tick, phase and coordinate space. A final render matrix is not automatically the matrix collision needed earlier.

The original hit search is directed and order-sensitive. Do not substitute a six-unordered-pair schedule just because four fighters have six unordered pairs.

Price changes on per-frame traces. Do not add independent subsystem P95s or treat a historical average saving as guaranteed P95 recovery. Use same-binary controlled routes where useful, then qualify the final shipping binary.

Acceptance: mechanically equivalent event/state behavior and the project's actual P95/cadence gates on a representative current stress population.

Sources: [four-fighter proposal](https://github.com/rockenrooster/Smash64DS_Port/blob/505234457af0b2f3aff00439605ad6b262ceaede/docs/reviews/4Fighter_optimization.md), [ordering and proof corrections](https://github.com/rockenrooster/Smash64DS_Port/blob/505234457af0b2f3aff00439605ad6b262ceaede/docs/reviews/Independent_Review_P2_Residency_and_Four_Fighter_Plans.md).

## Workflow changes that reduce completion time

### Eliminate cross-configuration artifact contamination

The Makefile documents shared generated paths outside BUILD and shared published ELF/NDS output hazards. A warning to rebuild after a lab run is not a durable correctness boundary.

Keep one build at a time until configuration-dependent generated outputs and ELF/NDS pairs are isolated. Prefer a per-configuration generated directory and immutable build artifact. Publish by promoting the tested pair, rather than let experiments overwrite the owner's root ROM. Keep Make's existing internal parallelism; do not add competing top-level makes.

Include generator inputs and configuration in cache keys. Write generated outputs atomically. Compare clean and incremental outputs for equivalent inputs and investigate unexpected differences.

### Parallelize bounded products, not shared integration

Use workers for independent descriptors, source inventories, schema families, host tests and specific reproductions. Keep one integrator owning shared Makefile, headers, registries and publication. This works within the current no-new-worktrees constraint.

A useful assignment states the exact input commit, production seam, expected artifact, failure case and stop condition. Require absence claims to be checked against preprocessing/linkage or the actual ELF where available; a filename guess is not evidence.

### Use a small test ladder

Run cheap schema, generation, arithmetic, decoding and negative tests after local edits. Use focused production-route smoke tests after a subsystem change. Run the broader Boundary and representative full-match measurements at coherent integration checkpoints and before acceptance/publication.

Do not permanently replace the full-match gate with a short diagnostic sample. Conversely, do not pay the full shell traversal cost for every arithmetic hypothesis. The existing harness registry should remain the entry point rather than multiplying proof-only modes.

### Keep the current board small; strengthen its evidence model

The September 2 compact-board cleanup already exists. Preserve it. Add explicit status dimensions and exact evidence identities instead of writing another large narrative status document. A source change is implemented; it is not automatically compiled, exercised, visible, mechanically correct or performant.

## Conditional second-line memory lever: scene overlays

If the same-build ledger shows substantial menu/campaign-only code or data occupying battle RAM, price scene-lifetime overlays. Start with data; executable overlays require callback, IRQ, return-address and cache-lifetime discipline. Do not page live combat code or assets in response to a move.

The checked-in sm64-nds reference loads selected level data into an overlay region; the SM64DS reference tracks loaded overlay regions and checks overlap. Borrow the lifetime organization, not an assumption that its exact implementation or capacity transfers to Smash64DS. No overlay saving was measured in this review.

Sources: [sm64-nds level overlay](https://github.com/rockenrooster/Smash64DS_Port/blob/505234457af0b2f3aff00439605ad6b262ceaede/decomp/sm64-nds/src/nds/nds_overlay.c), [SM64DS overlay regions](https://github.com/rockenrooster/Smash64DS_Port/blob/505234457af0b2f3aff00439605ad6b262ceaede/decomp/sm64ds-decomp/src/LoadOverlay.c).

## What not to spend the next cycle on

Do not restart in another language or engine, add a general battle pager/LRU, assume more threads or more agents solve shared-state integration, repeat already-landed fixed-point/packet optimizations, or import the remainder of the campaign before the four-fighter resource and timing foundation is credible.

The best completion metric is the number of configurations that are simultaneously complete, resident, native where required, behaviorally sound and within budget—not the number of imported files or green smoke tests.

## Included experiment

`repro_pose_clock_boundary.py` — standard-library-only host counterexample.  
`pose_clock_results.json` — output from the script executed during this review.

No repository files were modified, no changes were pushed, and no DS performance improvement is claimed as measured here.
