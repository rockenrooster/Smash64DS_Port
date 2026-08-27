# Source Refactor Plan

**Status:** Core safe implementation complete; dirty-file phases deferred
**Date:** 2026-08-27
**Primary scope:** `src/port/*` and `src/nds/nds_renderer.c`

## Implementation Status

- Phase 1 complete: `reloc_backend_diagnostic_recorders.c` is an ordered
  same-translation-unit aggregator (`4da64a9c795`).
- Phase 2 complete for the proven-retired immediate Mario/Fox proof chain;
  current accepted builds did not link that surface (`767475779cc`). Further
  proof deletion remains evidence-driven rather than a refactor requirement.
- Phase 3 deferred: `reloc_backend_renderer_dl.c` still contains unrelated
  active work, so mechanically splitting it now would fold that work into the
  refactor and violate this plan's isolation rule.
- Phase 4 complete: `nds_renderer.c` is an ordered textual-slice aggregator and
  still produces one executable-equivalent `nds_renderer.o` (`dd340c70da2`).
- Phase 5 remains deliberately opportunistic: no semantic renderer ownership
  change is required merely to complete the structural refactor.
- Phase 6 complete for clean `taskman_seam.c` and `diagnostics.c`
  (`f7b8160d88b`). `nds_menu_shell.c` remains deferred while unrelated active
  roster/UI work is present.

The two deferred files should be revisited only after their active changes are
settled. Do not mechanically redistribute a dirty file across new slices just
to mark a phase complete.

## Purpose

Reduce the size and navigational cost of the largest project-owned source files without changing game behavior, renderer ownership, translation-unit boundaries, or performance-sensitive code generation during the first pass.

The guiding rule is:

> First expose the architecture already present in the code, then improve that architecture.

This is deliberately a two-stage refactor:

1. **Mechanical decomposition** into ordered textual implementation slices while preserving the existing compilation units.
2. **Semantic cleanup** only after the mechanical structure is proven equivalent and obsolete paths are individually shown to be unnecessary.

## Project Constraints

- BattleShip remains the gameplay/source-behavior authority.
- `decomp/` is read-only.
- Preserve Nintendo DS CPU, RAM, VRAM, bandwidth, fixed-point, GX, DMA, and cache-coherency constraints.
- Do not hide shared defects with offsets, one-off constants, or frame-specific exceptions.
- Do not make performance claims from source-only refactoring.
- Builds use the project build flow; do not invoke bare `make` as the normal build entry point.
- Builds already parallelize; do not pass an additional `-j`.
- Keep one build running at a time.
- Current owner directive is **NO SNAPSHOT**. Do not create a snapshot unless that directive changes.
- Preserve all unrelated dirty worktree changes.

## Current Architectural Finding

The `src/port` backend is already organized as an aggregate translation unit rather than a collection of independent object files.

`src/port/scene_backend.c` textually includes project-owned backend slices, including:

```c
#include "diagnostics.c"
#include "taskman_seam.c"
#include "reloc_backend.c"
#include "sprite_preview_backend.c"
#include "opening_movie_backend.c"
#include "title_backend.c"
```

`src/port/reloc_backend.c` then textually includes its own slices, including:

```c
#include "reloc_backend_compat_shims.c"
#include "reloc_backend_assets.c"
#include "reloc_backend_fighter_model.c"
#include "reloc_backend_renderer_dl.c"
#include "reloc_backend_movement.c"
#include "reloc_backend_mp_collision.c"
#include "reloc_backend_cliff_ledge.c"
#include "reloc_backend_diagnostic_recorders.c"
```

All of these therefore compile into the existing `scene_backend.o` translation unit.

That existing pattern is the safest initial refactor mechanism. It preserves:

- static linkage,
- declaration visibility,
- source order,
- inlining opportunities,
- interworking decisions,
- section placement behavior,
- and nearly all code-generation characteristics of the current build.

The initial refactor will **extend this textual-slice model instead of creating new object files**.

## Phase 1: Decompose `reloc_backend_diagnostic_recorders.c`

This is the first implementation target because it is very large, internally mixed, and currently clean while several neighboring files contain unrelated active work.

The file is approximately 19K lines and is not merely diagnostic code. It currently combines production FTMain behavior, compatibility paths, proof machinery, fighter construction support, and the real fighter display seam.

### Planned textual slices

`reloc_backend_diagnostic_recorders.c` becomes a small ordered aggregator resembling:

```c
#include "reloc_backend_fighter_anim_audit.c"
#include "reloc_backend_ftmain_status_compat.c"
#include "reloc_backend_ftmain_runtime.c"
#include "reloc_backend_ftmain_damage_proofs.c"
#include "reloc_backend_ftmain_collision_proofs.c"
#include "reloc_backend_ftmain_hit_pipeline_proofs.c"
#include "reloc_backend_fighter_proof_chain.c"
#include "reloc_backend_fighter_display_seam.c"
```

Names may be adjusted slightly if implementation reveals a clearer exact owner, but the ownership boundaries should remain the same.

### Mechanical ownership boundaries

Approximate current source regions are shown only as navigation aids. Cuts must land on complete top-level declarations/functions rather than blindly on line numbers.

| Slice | Approx. current region | Responsibility |
| --- | ---: | --- |
| Fighter animation audit | 1-626 | `NDS_FIGHTER_ANIM_AUDIT` implementation and markers |
| FTMain status compatibility | 627-5110 | `ftMainSetStatus`, imported status routing/diagnostics, compatibility harness |
| FTMain runtime/import bridge | 5111-7223 | live/imported FTMain paths, damage/collision runtime, Task108 callback hooks |
| Damage/status proofs | 7224-12627 | damage selection, status behavior, and associated correctness proofs |
| Collision/catch proofs | 12628-16505 | attack/hurtbox/catch/collision proof machinery |
| Hit/damage pipeline proofs | 16506-18322 | hit-record through damage/stats/`ProcParams` proof chain |
| Fighter proof-chain support | 18323-18953 | fallback animation events, immediate proof chain, compatibility fighter construction |
| Fighter display seam | 18954-end | `ftDisplayMainProcDisplay`, native display submission, light stubs/scales |

The proposed split points have been checked at preprocessor depth zero; none requires cutting through an open conditional-compilation block.

### Phase 1 rules

During extraction:

- Preserve exact source order.
- Preserve all symbol names.
- Preserve all storage classes.
- Preserve all preprocessor conditions.
- Preserve all globals in their current relative order.
- Do not move state merely to make a file look cleaner.
- Do not change behavior.
- Do not delete proof code.
- Do not rename functions or variables.
- Do not reformat entire regions.
- Do not convert slices into independent translation units.
- Do not mix semantic cleanup into the extraction commit.

`scene_backend.o` remains the owning object.

### Makefile policy for Phase 1

Do not modify `Makefile` merely to list the new hand-authored nested source slices.

The compiler-generated dependency file for the aggregate translation unit can track ordinary textual `#include` dependencies after the aggregator is compiled. The repository's explicit Makefile prerequisites around generated includes solve a separate generated-file ordering/race problem and are not needed for these normal source slices.

Avoiding a Phase 1 Makefile edit also prevents unnecessary overlap with current unrelated work in that file.

## Phase 1 Verification Gate

The first split is intended to be executable-equivalent.

Use the same build configuration before and after the split and verify, in order:

1. The project builds successfully.
2. `scene_backend.o` retains the expected symbols.
3. Relevant executable and data sections are equivalent.
4. Link-map placement remains equivalent where expected.
5. Final ARM9/ROM output is compared where the build is reproducible enough for that comparison to be meaningful.
6. Run the smallest focused verifier that exercises the affected backend surface.

If raw object hashes differ only because debug/source metadata records the new source filenames, compare the meaningful section hashes and disassembly instead of treating the raw hash difference as a behavioral change.

If executable code and relevant data are proven equivalent, a full synchronized performance campaign is not required for the mechanical extraction itself.

Any unexplained codegen, section, symbol, or behavior difference stops the extraction checkpoint until understood.

## Phase 2: Semantic Cleanup of the Diagnostic/Proof Area

Only after Phase 1 is verified should cleanup begin.

The large proof surface includes old Mario/Fox and scene-harness paths that appear to be largely verification-oriented, but none should be deleted merely because it looks old.

For each candidate removal:

1. Find all references/callers.
2. Determine whether it is reachable from current production paths.
3. Determine whether it is reachable from the current verifier/harness registry.
4. Check Boundary/Latest verification needs.
5. Remove one coherent obsolete surface at a time.
6. Run the focused verifier.
7. Run Boundary when the removal affects broadly shared behavior or proof coverage.

This phase is where names, ownership comments, local interfaces, and genuinely obsolete infrastructure may be cleaned up.

Do not combine large deletion work with the original mechanical split.

## Phase 3: Decompose `reloc_backend_renderer_dl.c`

This file is another major monolith, but it currently contains unrelated active work. Do not make it the first target and do not fold those existing changes into the refactor.

Once the active work is settled, keep it inside `scene_backend.o` and decompose it textually by owner.

Provisional structure:

```text
reloc_backend_renderer_dl.c       (aggregator)
  renderer_adapter_matrix.c
  renderer_adapter_legacy_dl_probes.c
  renderer_adapter_stage.c
  renderer_adapter_legacy_raster_probes.c
  renderer_adapter_fighter.c
```

### Intended ownership

**Matrix/common adapter**

- fixed matrix conversion,
- DObj/world caches,
- camera conversion,
- GX-slot hierarchy preparation,
- shared native-owner transform preparation.

**Legacy DL probes**

- old DL scan/execute/draw proof paths,
- owner hashing,
- depth/snapshot diagnostics.

**Stage adapter**

- stage traversal,
- stage material conversion,
- native stage topology,
- native stage owner preparation,
- stage/effect submission surfaces.

**Legacy raster probes**

- software DL/raster proof paths,
- old fighter proof/raster surfaces that are not the live native owner.

**Fighter adapter**

- fighter display contract,
- draw memo,
- native fighter inputs/topology,
- native fighter production submission.

### Current stage-owner performance lead

Do not casually refactor or reorder the current native-stage preparation seam before its reject behavior is understood.

The current performance investigation has identified native-stage owner preparation reject reason 6 as the next lead. The adapter reaches `ndsRendererPrepareNativeStageOwner(...)`; failure marks the stage topology invalid and records reject reason 6.

This is active performance evidence, not cleanup noise.

Mechanical slicing may expose the owner more clearly, but semantic changes to that path require their own correctness and performance work.

## Phase 4: Decompose `src/nds/nds_renderer.c`

`nds_renderer.c` is approximately 39K lines and is currently compiled as one `nds_renderer.o`.

The first renderer refactor should preserve that object boundary as well. Convert the file into an ordered aggregate of textual implementation slices rather than immediately creating independent translation units.

Provisional source-order architecture:

```text
nds_renderer.c                          (aggregator / public owner)
  nds_renderer_preamble.c
  nds_renderer_assets.c
  nds_renderer_dl_core.c
  nds_renderer_textures_effects.c
  nds_renderer_native_common.c
  nds_renderer_native_fighter.c
  nds_renderer_native_stage.c
  nds_renderer_native_fighter_production.c
  nds_renderer_dispatch_profile.c
```

These names are conceptual. Exact boundaries should be anchored by symbols and current source order during implementation.

### Renderer ownership rule

The DS renderer owns DS execution/hardware behavior. The `reloc_backend_renderer_dl` side owns BattleShip-to-DS adaptation.

Do not create a generic abstraction layer between them merely for architectural symmetry.

### Fighter packet ownership invariant

Keep the following mechanism under one coherent owner:

```text
fighter packet recording
+ packet storage
+ packet patching
+ cache publication
+ DMA submission
+ DMA completion
+ invalidation/release
= one ownership unit
```

This is a producer/consumer/lifetime/cache-coherency contract, not a set of unrelated helpers.

The packet's backing storage must remain valid until asynchronous hardware consumption is complete, and reuse must not occur before the completion wait.

### GX writer synchronization invariant

Any GX writer outside the renderer must continue to honor the fighter-packet DMA wait rail before writing GX state/FIFO.

Do not weaken or obscure that rule during refactoring.

### Native stage ownership invariant

Keep native-stage preparation, execution state, topology/config/material validation, reuse decisions, and associated failure publication cohesive until the current rejection path is understood and deliberately redesigned.

## Phase 5: Renderer Semantic Cleanup

Only after the renderer source is mechanically navigable should ownership cleanup begin.

Possible later work includes:

- deleting proven-obsolete software DL/raster proof surfaces,
- narrowing oversized internal interfaces,
- reducing duplicated native-owner preparation,
- clarifying stage versus fighter adapter ownership,
- simplifying legacy harness-only state,
- considering genuinely independent translation units where there is a real ownership boundary and measured benefit.

Independent translation units are **not** a goal by themselves. They may change:

- inlining,
- ARM/Thumb interworking,
- register allocation/spills,
- I-cache behavior,
- code footprint,
- linker placement,
- ITCM/DTCM fit,
- helper-call generation.

Any such conversion needs codegen and performance evidence rather than being treated as a cosmetic cleanup.

## Phase 6: Medium Monoliths

After the two major renderer/backend areas are structurally improved, revisit medium-sized owners.

### `taskman_seam.c`

Possible ownership slices:

- taskman lifecycle,
- R2 battle host,
- scene-harness compatibility,
- legacy proof runners,
- shell transitions.

### `diagnostics.c`

Split only by actual diagnostic ownership/lifetime, not by arbitrary size.

### `nds_menu_shell.c`

Possible ownership slices:

- menu shell/router,
- VS mode/rules,
- character select screen,
- stage select screen.

This file currently participates in active roster/UI work, so defer it until that work is settled.

## Verification Policy by Change Type

### Mechanical same-translation-unit extraction

```text
build
-> object/section equivalence
-> symbol/map equivalence
-> focused verifier
```

No performance claim is made and no full performance campaign is required if executable equivalence is proven.

### Semantic cleanup or deletion

```text
focused verifier
-> Boundary when shared behavior or proof coverage warrants it
```

The burden is to prove the removed path is obsolete, not merely unused in one test.

### Hot renderer / GX / DMA / TCM / translation-unit change

```text
map + disassembly/codegen inspection
-> focused correctness verification
-> synchronized performance A/B
-> Boundary
```

Performance comparisons must use representative synchronized workloads and the project's authoritative profiling environment.

## Commit Strategy

Keep structural and semantic changes separable and reviewable.

Recommended checkpoints:

1. Split fighter animation/status portions of diagnostic recorders.
2. Split FTMain runtime/proof portions.
3. Split fighter display/proof-chain tail.
4. Clean narrow interfaces/comments exposed by the split.
5. Delete individually proven obsolete proof infrastructure.
6. Mechanically split renderer adapter after its active work settles.
7. Mechanically split `nds_renderer.c` while preserving `nds_renderer.o`.
8. Perform separately verified semantic renderer ownership cleanup.

Commit verified retained checkpoints regularly. Push only after confirmed progress. Do not create a snapshot under the current owner directive.

## Explicit Non-Goals During the Initial Refactor

Do **not**:

- bulk-rename symbols while splitting files,
- run whole-file formatting over moved code,
- reorder globals for aesthetics,
- convert the new slices into separate `.o` files,
- add a generic renderer abstraction framework,
- alter runtime behavior during the mechanical pass,
- edit `decomp/`,
- fold unrelated dirty renderer/menu/asset/roster work into the refactor,
- delete old proof infrastructure before reachability and verifier need are established,
- move hot functions between sections/translation units without codegen review,
- change GX/DMA ownership as incidental cleanup,
- claim a performance improvement without measurement.

## Next Deferred Work

No additional structural work should be forced into the current dirty files.
When their active changes are settled:

1. mechanically split `src/port/reloc_backend_renderer_dl.c` while preserving
   `scene_backend.o`, then prove object/code equivalence;
2. mechanically split `src/nds/nds_menu_shell.c` only if it is still large
   enough to justify the change after the active roster/UI work lands;
3. perform further semantic cleanup only where current-build reachability and
   verifier coverage prove an old path obsolete.
