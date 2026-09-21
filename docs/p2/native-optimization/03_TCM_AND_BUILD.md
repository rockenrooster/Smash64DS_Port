# N01 — ITCM/DTCM policy, build boundaries and early debloating

> **Revision 2 coverage:** Universal scope: code/data placement and deadness must consider every required fighter, stage and rare owner. Zero execution on the pilot roster is not global unreachability; a memory/layout win cannot be accepted by hiding another legal configuration. See [16_ALL_ROSTERS_ALL_STAGES.md](16_ALL_ROSTERS_ALL_STAGES.md).

**Purpose:** create room for a smaller runtime without confusing source formatting with actual linked/executed savings. This package is bounded early work; final packing is N09.

The pinned linker groups `*.32.o` text/rodata into ITCM and data/BSS into DTCM. Renderer attributes also create shared input sections. The prior diagnostic 4,650-byte zero-execution population is not proof of dead code; interrupts, startup, another fighter, an uncommon move and packet misses must be covered. Current DTCM data is constrained to `0x02ff3000`. [S07, S08, S05]

**Explicit rule:** retain exception vectors, startup/load metadata, ARM/Thumb interworking and ABI-required system code. Do not exchange correctness for a prettier ITCM total.

## Build iteration for these tasks

`../../VERIFYING.md` owns producer/configuration and incremental-build procedure.
Keep baseline artifacts, reuse a matching warm BUILD directory, and serialize
shared producers. A target name does not prove effective stage flags. Prepare
matching producer data/stamps before switching lab/shipping consumers; repair a
reproduced dependency/order defect instead of repeating full fresh shell builds.
No generated-file hand edits, missing-stage workaround or weakened stale check.

N01 is not a separate campaign of compiler permutations. Apply its needed kernel
split/module/producer fix with the selected runtime consumer. An ITCM space saving
is enabling capacity until the complete retained batch has a measured CPU result.
Relevant system/boot/IRQ/interworking tests remain due; reused semantic proof cannot
excuse new linked-layout timing. Final full-runtime repacking stays in N09.

### N01.01 — Produce byte-accurate linked ownership

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

### N01.02 — Decouple instruction mode from residency

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

### N01.03 — Separate shipping observation from safety

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

### N01.04 — Split the first measured cold renderer tails

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

### N01.05 — Remove dead build variants and create real module boundaries

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

### N01.06 — Use reclaimed ITCM only for measured winners

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
