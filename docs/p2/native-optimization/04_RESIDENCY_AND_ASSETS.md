# N02 — Memory recovery and deterministic native asset admission

> **Revision 2 coverage:** Universal scope: every legal four-fighter lineup, including repeated kinds, must be admitted on every selectable VS stage. N02.07 enumerates the static resource universe; N02.06 and N10 qualify target admission. Safe rejection of a legal match is not a passing resource solution. See [16_ALL_ROSTERS_ALL_STAGES.md](16_ALL_ROSTERS_ALL_STAGES.md).

**Purpose:** make precomputation usable on the original DS without replacing CPU work with mid-fight storage stalls. The current residency contract requires no mandatory battle/motion/texture demand reads after GO; BGM is a declared exception with its own service policy. [S06]

## Representation selection before baking

For each asset family compare existing representation, compact native tracks plus constant channels, selected local-matrix precompute, and full samples. Publish ROM bytes, permanent resident bytes, startup scratch, transition peak, per-frame decode/evaluation ticks, and remaining mandatory storage reads. Choose on combined cost, not ROM size alone.

Four fighters × 32 joints × 60 samples × 48-byte matrices is 368,640 bytes for only one second; it is an illustration, not this game's measured bank. Large ROM allowance does not make all those matrices resident. Store reusable local data and dynamic inputs separately. Shared same-kind fighter banks are immutable; per-instance cursors and packet patches remain distinct.

## Minimum bank contents

The generated admission plan must include all source-reachable motion/event variants, model parts, both reachable detail modes, costumes/material frames, copy/morph/entry states, weapons/items/summon children and stage-dependent assets for the selected scene. It need not resident-load the union of every game scene. A mid-fight wave or copy path is not a free loading boundary.

### N02.01 — Inventory simultaneous and transient resource sets

**Depends on:** N00.02, N00.06

**Edit/inspect boundary:** `docs/p2/P2-texture-residency.md`; `docs/p2/P2-1c-vram-map.md`; `src/nds/nds_renderer_assets.c`; `src/nds/nds_ftanim_track.c`; `src/nds/nds_battlepack_anim.c`.

**Implementation sequence**

1. Build exact source-derived required sets per scene/profile: fighter kinds/instances/copy possibilities, stage/hazards, items/children, UI/VFX, motions and audio cue policy.
2. Classify ROM/code, ARM9 heap/arena, ARM7/shared storage, TCM, texture/palette/BG/OAM, graphics scratch and per-instance packet memory separately.
3. Include load→commit transient peaks and retained old-scene lifetimes, libc reserve, stacks, object pools and worst concurrent cue/particle bursts.
4. Compare required/admitted/excluded identities, not equal counts; emit the first failed constraint with exact resource identity.
5. For every legal roster-stage resource case, union immutable banks by identity but charge four mutable instances: clocks, matrices, packet patches, costume state, hit records, CPU state and pending cues. Cover AAAA, AAAB, AABB, AABC and ABCD multiplicities.
6. Compute a source-reachable dependency closure for the selected roster and stage, including Kirby copying opponents and legal held-item/summon/capture children. Record mutual exclusion or concurrent occupancy bounds where they reduce memory; unsupported exclusions do not remove requirements.

**Required tests/evidence:** T-RES set-difference fixtures; missing child and equal-count wrong-resource mutation both fail; runtime admission/pool witnesses from same baseline.

**Work or dependency retired:** Anonymous allocation guesses and implicit post-GO demand assumptions.

**Done:** A source-derived resource model covers the full required catalogue and exposes per-case deficits, including slot-dependent layouts, uncommon states and transient peaks; a pilot allocation is not universal admission proof.

**Stop/revert:** An overfull plan is not solved by marking required content optional or lowering legal pool capacity.

### N02.02 — Recover memory from actually retired representations

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

### N02.03 — Compile compact motion and resource banks

**Depends on:** N02.01, N04.02

**Edit/inspect boundary:** `scripts/fighters/generate_nds_native_owners.py`; `scripts/_paths.py`; `src/nds/nds_ftanim_track.c`; `proposed native bank generator/loader extensions`.

**Implementation sequence**

1. Extend existing generators with explicit version/hash/table metadata and fixed numeric payloads. Fold constants and deduplicate identical immutable channels without aliasing mutable state.
2. Choose compact keys/coefficient tables and selected precomputed local transforms from measured bank/evaluator tradeoffs. Include event metadata but keep gameplay event semantics independently validated.
3. Validate offset/count/stride/alignment/integrity in host output and target admission. Generated outputs have deterministic ordering and no timestamps/host addresses.
4. Update generator inputs, Makefile dependencies and staleness checks in the same commit; keep source references read-only.
5. Factor reusable fighter/stage/resource banks instead of generating a full payload for every roster-stage combination. Emit deterministic per-case admission metadata from shared records; the combinatorial test catalogue must not become a combinatorial ROM bank.
6. Publish worst per-instance patch/scratch/matrix/animation capacities over all required owners and legal variants, not constants observed only for Mario/Fox or the current stress roster.

**Required tests/evidence:** T-BANK round trip/corruption/version/overflow tests; independent source-derived motion/geometry/resource completeness; deterministic regeneration.

**Work or dependency retired:** Runtime source format decoding, invariant coefficient work and redundant immutable payloads for converted banks.

**Done:** Banks are smaller/resident enough and directly consumable with a documented version transition.

**Stop/revert:** Reject a representation that fits ROM but cannot meet runtime resident/transient limits or event fidelity.

### N02.04 — Implement transactional admission and locked epochs

**Depends on:** N02.02, N02.03

**Edit/inspect boundary:** `src/nds/nds_renderer_assets.c`; `src/nds/nds_ftanim_track.c`; `src/nds/nds_battlepack_anim.c`; `src/port/renderer_adapter_stage.c`.

**Implementation sequence**

1. Validate the complete new scene plan and reserve all permanent/transient resources before publishing any live handles.
2. Prepare/upload at the legal boundary; bind all native owners; commit one generation only after full success. On failure clean partial allocations and restore the prior valid scene or established failure UI.
3. Lock the battle epoch: no mandatory motion/texture create/convert/evict/demand-read work after GO. Pre-admitted palette/material animation remains legal.
4. Resolve scene transitions and copy/morph/wave changes from admitted data, using C4 mutation ownership and C5 texture/packet lifetime rather than a gameplay-time LRU.
5. A source-legal profile that cannot be admitted remains a product failure even when rejection is safe. Never make it illegal in the selector, force an unapproved quality downgrade, omit required assets or defer demand reads until after GO to make the solver pass.

**Required tests/evidence:** T-RES failure injection at every admission phase; T-LIFE cancelled loads, rematch, same-address arena reuse; class-specific post-GO counters must remain zero.

**Work or dependency retired:** Mandatory demand reads and repeated resource discovery in converted locked scenes.

**Done:** Complete native content is admitted atomically and stays valid through all qualified battle states.

**Stop/revert:** Fail closed at admission on missing resources, but leave product coverage OPEN; containment is not a successful port.

### N02.05 — Separate CSS, battle and transition residency

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

### N02.06 — Qualify no-demand-read and bank capacity closure

**Depends on:** N02.04, N02.05, N03.10, N05.08, N02.07, N10.07

**Edit/inspect boundary:** `scripts/verify-p2-four-fighter-stress.ps1`; `docs/p2/P2-texture-residency.md`; `proposed admission manifest artifacts`.

**Implementation sequence**

1. Exercise cold motion changes, copy/morph/entry, rare item/summon children, simultaneous KO/respawn and long matches under the selected bank.
2. Classify every storage event by client and source. A direct NitroROM read remains a read; a cache miss remains a mandatory dependency unless proved optional/declared.
3. Validate independent VRAM placement/palette/view/atlas constraints and RAM peaks, not a single total-memory number.
4. Publish the scene closure and repeat for the measured worst CPU and worst memory configurations, which may differ.
5. Reconcile N02.07 static results with N10 per-case runtime admission. Distinguish an exact structural reuse certificate from an untested case; host byte sums alone never count as target startup or post-GO proof.

**Required tests/evidence:** T-RES zero mandatory post-GO motion/texture reads, exact required sets, no safety reserve breach; T-AUDIO declared services have no deadline loss.

**Work or dependency retired:** Residual mandatory storage dependency in qualified battle banks.

**Done:** All required roster-stage capacity cases pass the generated static constraints and their assigned runtime admission/lifetime obligations; every locked profile has complete required resources, no undeclared demand work and no known legal admission failure.

**Stop/revert:** No blanket zero-I/O claim when BGM streams; no accepted scene closure with an unclassified storage client.

