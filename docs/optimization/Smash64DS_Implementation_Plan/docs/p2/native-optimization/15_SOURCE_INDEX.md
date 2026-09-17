# Source index and evidence scope

Original entries S01–S34 below remain pinned to `75f7f6b4b4864c82c01872d0fd2771d171005272`, the v1 research baseline. Revision 2 rechecked `master` at `e67e5871ba8c4ae972f4826bfeb89757d3686401`; S35–S37 record the refresh. Historical phrases such as “current” or “rechecked” in the retained entries refer to their original preparation, not a new runtime measurement. No new game build/benchmark, all-file exhaustive source audit, or independently reproduced performance result is claimed.

The supplied research document `Smash64DS_Native_Optimization_Plan.md` is supporting analysis. Current product/procedure/residency files override historical experiment-specific restrictions where their scopes differ. Any fresh working-tree divergence requires re-resolving the affected anchors.

## S01 — Current product contract

[`PROJECT_GOAL.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/PROJECT_GOAL.md)

Mechanical equivalence, native-only all-ROM rendering, original-DS performance and permitted implementation changes. Read in prior audit at the unchanged pinned SHA.

## S02 — Current execution and checkpoint

[`docs/P2_EXECUTION_BOARD.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/docs/P2_EXECUTION_BOARD.md)

Existing P2-2p8 ownership, retained work, content gaps and current queue. Cross-check against HANDOFF; not a second live queue.

## S03 — Current build, verification and measurement procedure

[`docs/VERIFYING.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/docs/VERIFYING.md)

Read again for this implementation plan. Defines serial builds/timing, actual targets, native/resource versus product gates, sample coverage and accurate melonDS policy.

## S04 — Final running-joint-mask checkpoint

[`artifacts/performance/2026-09-15_p2-2p8-pose-joint-mask/README.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/artifacts/performance/2026-09-15_p2-2p8-pose-joint-mask/README.md)

Recorded final hard-on WORK-H/cadence/memory, not a fresh benchmark. Distinguish its final hash from same-ROM A/B artifacts.

## S05 — Preceding diagnostic instruction census

[`artifacts/performance/2026-09-15_p2-2p8-pose-track-mask/poseplay-pc.txt`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/artifacts/performance/2026-09-15_p2-2p8-pose-track-mask/poseplay-pc.txt)

Diagnostic section occupancy/cost candidates. Not final shipping truth or proof of global deadness.

## S06 — Deterministic scene residency contract

[`docs/p2/P2-texture-residency.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/docs/p2/P2-texture-residency.md)

Read again. Required set identity, pre-GO admission, zero mandatory motion/texture demand reads, declared BGM and transactional lifetime.

## S07 — Renderer placement/state/packet anchor

[`src/nds/nds_renderer_preamble.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/nds/nds_renderer_preamble.c)

Current shared section macros, native packet state/recorder/patch/submission and global runtime state. Prior source audit; inspect exact function bodies before edits.

## S08 — Actual linked memory policy

[`linker/nds_hot_text.ld`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/linker/nds_hot_text.ld)

Explicit ITCM, .32.o side effects, DTCM stack ceiling and main-RAM hot-text groups. Preserve startup/OS invariants.

## S09 — Native fighter production executor

[`src/nds/nds_renderer_native_fighter_production.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/nds/nds_renderer_native_fighter_production.c)

Preflight before replay, root/foreign table bindings, split/GX paths and production state. Call order is observed; savings require measurement.

## S10 — Shared native preparation

[`src/nds/nds_renderer_native_common.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/nds/nds_renderer_native_common.c)

Source-shaped eligibility and vertex preparation plus shared native stage/fighter work. Reachability is not inferred solely from source presence.

## S11 — Native actor/owner execution

[`src/nds/nds_renderer_native_owners.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/nds/nds_renderer_native_owners.c)

TaruCann configuration/traversal/vertex setup and actor-specific native contracts; a pilot example, not measured dominant CPU cost.

## S12 — Renderer translation-unit organization

[`src/nds/nds_renderer.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/nds/nds_renderer.c)

Textual includes of implementation fragments; source splitting is not automatically actual module or runtime simplification.

## S13 — Compact pose and event clock

[`src/nds/nds_ft_pose.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/nds/nds_ft_pose.c)

Fixed pose with source fields, binary32 integer clock, timing mismatch history, active masks, required updates and diagnostic state.

## S14 — Current pose interface

[`include/nds/nds_ft_pose.h`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/include/nds/nds_ft_pose.h)

Existing native/source pose boundary; task implementation must inspect all readers before ABI replacement.

## S15 — Status and topology mutation seam

[`src/import/battleship_ftmain.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/import/battleship_ftmain.c)

Read-only reference import plus port wrappers; status can change topology without root/heap-generation identity changes.

## S16 — CPU behavior import and observation

[`src/import/battleship_ftcomputer.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/import/battleship_ftcomputer.c)

CPU processing, source decision engine and telemetry; SCPU is nested and requires positive engagement.

## S17 — Transform adapter and special classes

[`src/port/renderer_adapter_matrix.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/port/renderer_adapter_matrix.c)

World/local/camera and source billboard/orientation-replacement semantics; not all callbacks are ordinary TRS.

## S18 — Row accounting and percentile populations

[`scripts/census-tick-hud-p95-set.py`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/scripts/census-tick-hud-p95-set.py)

WORK-H accounting, SRC/GCRA/SINT/SCPU nesting and separate P95/cadence attribution sets. Audit wording that oversimplifies re-ranking; final metrics use full population.

## S18A — Verified four-fighter script/parameters

[`scripts/verify-p2-four-fighter-stress.ps1`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/scripts/verify-p2-four-fighter-stress.ps1)

Read for this plan. Actual script name, defaults, JsonOut/RowsCsv/CoverageJsonOut/MemoryJsonOut and source item-law checks.

## S18B — Harness membership authority

[`scripts/lib/harness-registry.ps1`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/scripts/lib/harness-registry.ps1)

Read for this plan. Boundary/Latest and p2_fourcpu_stress registry mapping; no invented profile names.

## S19 — Independent native geometry oracle

[`scripts/fighters/check_native_owner_geometry_closure.py`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/scripts/fighters/check_native_owner_geometry_closure.py)

Read again. All landed owners/both detail levels; triangle, dense vertex, matrix routing, facing/winding, strip/BEGIN semantics and source-facing exceptions.

## S20 — Motion/track runtime anchor

[`src/nds/nds_ftanim_track.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/nds/nds_ftanim_track.c)

Existing resident/streaming/motion representations to extend, not replace with an unrelated cache framework.

## S20A — NDO6 and special owner coverage

[`artifacts/visibility/2026-09-13_roster-close-kirby-ness-purin.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/artifacts/visibility/2026-09-13_roster-close-kirby-ness-purin.md)

Existing unlit vertex-color/alpha/class allocation and roster work. Search confirms the retained mechanism; verify exact runtime image before ABI changes.

## S21 — BGM native service

[`src/nds/nds_audio_bgm.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/nds/nds_audio_bgm.c)

Existing service/control/range/refill ownership; actual service threading must be inspected in implementation.

## S21A — Retained BGM direct-range evidence

[`artifacts/performance/2026-09-14_p2-2p8-bgm-direct/README.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/artifacts/performance/2026-09-14_p2-2p8-bgm-direct/README.md)

Already-landed BGM work; direct read remains I/O and cannot be sold as newly implemented.

## S22 — FGM native service

[`src/nds/nds_audio_fgm.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/nds/nds_audio_fgm.c)

Required one-shot/cue/control path and retained direct-range work; final all-runtime numeric closure includes it.

## S23 — Stage adaptation anchor

[`src/port/renderer_adapter_stage.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/port/renderer_adapter_stage.c)

Referenced by the current residency contract; implementers inspect actual stage partition and resource owners.

## S24 — Existing native HUD

[`src/nds/nds_battle_hud.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/nds/nds_battle_hud.c)

Native HUD already exists; optimize residual value/layout/update work rather than rediscovering BG/OAM.

## S25 — Particle import/update/draw anchor

[`src/import/battleship_lbparticle.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/import/battleship_lbparticle.c)

Source particle behavior, runtime state and native draw seam; preserve spawn/RNG/lifetime independent of visible output.

## S26 — Object/process import anchor

[`src/import/battleship_sys_objman.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/src/import/battleship_sys_objman.c)

Actual source registration/dispatch semantics must be traced before optional scheduler changes.

## S27 — Hardware math owner

[`include/nds/nds_r2_hwmath_unit.h`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/include/nds/nds_r2_hwmath_unit.h)

Existing divide/sqrt helpers and historical register-user assumptions; re-audit current threads/IRQs before overlap.

## S28 — Original fighter behavior source

[`decomp/BattleShip-main/decomp/src/ft/ftmain.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/decomp/BattleShip-main/decomp/src/ft/ftmain.c)

Strictly read-only specification for events, interactions and update rules; follow into relevant physics/collision/CPU modules for each task.

## S29 — Existing fighter native generator

[`scripts/fighters/generate_nds_native_owners.py`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/scripts/fighters/generate_nds_native_owners.py)

Path confirmed by scripts README and checker import; reuse its source/native representation and generation dependencies.

## S30 — Generator layout/import ownership

[`scripts/README.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/scripts/README.md)

Read/search-established generator area conventions, with scripts/_paths.py managing shared imports. Do not invent paths from old flat layout.

## S31 — SM64DS load-time relocation example

[`decomp/sm64ds-decomp/src/_ZN5Model17UpdateFileOffsetsER8BMD_File.cpp`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/decomp/sm64ds-decomp/src/_ZN5Model17UpdateFileOffsetsER8BMD_File.cpp)

Inspected in research: bind file offsets once. Pattern, not a drop-in Smash replacement.

## S31A — SM64DS fixed arithmetic example

[`decomp/sm64ds-decomp/src/CrossVec3.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/decomp/sm64ds-decomp/src/CrossVec3.c)

Inspected fixed vector products/rounding. Adopt explicit units/range semantics, not assumptions about all reference code.

## S31B — SM64DS fixed camera example

[`decomp/sm64ds-decomp/src/Camera_UpdateMatrices.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/decomp/sm64ds-decomp/src/Camera_UpdateMatrices.c)

Inspected fixed camera/lookup pattern; do not recreate the port’s already-fixed camera unnecessarily.

## S31C — SM64DS animation same-file path

[`decomp/sm64ds-decomp/src/_ZN9ModelAnim7SetAnimEP8BCA_Filei5Fix12IiEj.c`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/decomp/sm64ds-decomp/src/_ZN9ModelAnim7SetAnimEP8BCA_Filei5Fix12IiEj.c)

Inspected source shows speed/flags update without full rebinding when applicable; source-specific semantics still need proof.

## S32 — Build graph and target policy

[`Makefile`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/Makefile)

Large current build/source/generated dependency graph. Changes must be scoped and inspected rather than copying flags from another DS SDK.

## S33 — Existing selected VRAM layout contract

[`docs/p2/P2-1c-vram-map.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/docs/p2/P2-1c-vram-map.md)

Referenced by current residency contract; exact bank claims must be read before remapping, not inferred as free memory.

## S34 — Restart pointer

[`docs/HANDOFF.md`](https://github.com/rockenrooster/Smash64DS_Port/blob/75f7f6b4b4864c82c01872d0fd2771d171005272/docs/HANDOFF.md)

Current short restart boundary, not an additional queue. Pin linked artifact identity and reconcile board differences.

## H01 — BlocksDS optimizing code

[BlocksDS optimizing code](https://blocksds.skylyrac.net/tutorial/advanced/optimizing_code/)

Rechecked for this plan: ARM/Thumb tradeoffs, long multiply, TCM, cache and copying. Hardware background; the project uses its pinned devkitPro/Calico toolchain, so APIs/options must be checked there.

## H02 — BlocksDS TCM and cache

[BlocksDS TCM and cache](https://blocksds.skylyrac.net/tutorial/intermediate/tcm_and_cache/)

Rechecked: CPU-local fast memory and cache behavior. Current project linker/stack proof is the actual allocation authority.

## H03 — BlocksDS DMA

[BlocksDS DMA](https://blocksds.skylyrac.net/tutorial/intermediate/dma/)

Rechecked: DMA cannot access TCM/cache, source visibility, contention and completion. Not a promise of free concurrent main-memory execution.

## H04 — BlocksDS ARM7

[BlocksDS ARM7](https://blocksds.skylyrac.net/tutorial/intermediate/using_the_arm7/)

Rechecked: service ownership and cross-core design. Inspect the actual current ARM7 image before proposing new work.

## H05 — libnds hardware math reference

[libnds hardware math reference](https://blocksds.skylyrac.net/libnds/math_8h.html)

Rechecked: hardware math API/background; shared-unit ownership must match the pinned binary.

## H06 — libnds native 3D interface reference

[libnds native 3D interface reference](https://blocksds.skylyrac.net/libnds/videoGL_8h.html)

Reference carried from the research source index. Confirm exact declarations/command semantics in the installed SDK before implementation; not freshly re-fetched here.

## Not available as proof in this package

There is no current locally built ROM/ELF, compiler run, emulator run, new pixel/audio capture or target benchmark here. The package consists of implementation specifications, source links, a static task graph, evidence templates and a host validator for the plan itself. Connected GitHub reads and the supplied pinned research provide the source evidence; runtime acceptance still requires executing the specified tests.

Existing source locations are anchors, not permission to edit reference code. Descriptors, new checkers and new generated formats labeled proposed are implementation work still to do. Quoted historical numbers remain attached to their original artifact/configuration; planned allocations and targets remain estimates/objectives.

## S35 — Revision-2 repository refresh

[Commit e67e5871ba8c4ae972f4826bfeb89757d3686401](https://github.com/rockenrooster/Smash64DS_Port/commit/e67e5871ba8c4ae972f4826bfeb89757d3686401)

The GitHub branch read during revision 2 returned this September 15 commit. This is a metadata/source refresh, not a new local build. The original task source anchors and recorded timing remain explicitly historical.

## S36 — Product contract rechecked for universal-scope revision

[`PROJECT_GOAL.md` at revision recheck](https://github.com/rockenrooster/Smash64DS_Port/blob/e67e5871ba8c4ae972f4826bfeb89757d3686401/PROJECT_GOAL.md)

Read the current P2 gate during revision 2. It still requires the measured hardest four-CPU fighter/stage workload with all items, P95 about 1.12M and at least 95% two-VBlank cadence, with rare overruns allowed. The owner clarification extends explicit implementation/qualification scope to all legal lineups and stages without removing that allowance or changing P3 wireless scope.

## S37 — Changes since the original planning snapshot

[Compared baseline to rechecked master](https://github.com/rockenrooster/Smash64DS_Port/compare/75f7f6b4b4864c82c01872d0fd2771d171005272...e67e5871ba8c4ae972f4826bfeb89757d3686401)

The connector comparison reported two commits ahead. Its changed-file list includes particle-quad-index implementation/evidence, FGM coverage-check changes, board/handoff updates and archived optimization-reference moves. The repository also now carries the earlier research under `docs/optimization/README.md` and `docs/optimization/Smash64DS_Native_Optimization_Plan.md`. The README introduction was read during this revision; it is supporting analysis, not a competing execution queue. No new timing result is asserted from this changed-file list.
