# N07 — Stage, effects, particles and UI

> **Revision 2 coverage:** Universal scope: every selectable VS stage must remain complete with any legal four-fighter lineup. Maintain separate GPU/overdraw, geometry/matrix, native-ordering and memory/service leaders instead of assuming the CPU leader stresses all resources. See [16_ALL_ROSTERS_ALL_STAGES.md](16_ALL_ROSTERS_ALL_STAGES.md).

**Target:** remove generic preparation and mixed numeric state without dropping source-visible content. Existing native stage packets, 2D UI work and pose masks are retained. Sources [S11, S17, S21A, S23–S25].

## Static/dynamic partition

Generate a stage draw partition from actual source behavior: immutable geometry/material groups; moving solids; collision/hazard actors; cosmetic animation; camera-facing backgrounds; translucent/no-Z groups. A world-static part still needs camera-dependent state when the camera changes, but it does not need a source DObj ancestry/material rediscovery. Keep hazard gameplay update separate from decorative presentation.

Use a small shared native submission kernel where semantics match. Do not create an all-purpose traversal/configuration object for each fixed quad, effect or label. Bake invariant UV/color/normal/layout decisions in native assets. Batching is allowed only when it preserves visibility, blend/depth and source ordering.

## Depth regression contract

Maintain tests for the impact ring with front and rear portions around a fighter; shield overlap and slice order; Castle roof/front/back geometry; Yoshi stage translucent clouds/platforms; hazard layering; cutout alpha; palette/material changes; and camera clipping. Do not fix a ring that should be depth-tested by pinning it in front. Do not make an absent platform "cheap."

OAM/BG has different priority/blending behavior from depth-tested 3D. Use it for suitable screen-space content. An intersecting world effect usually requires a native 3D depth relationship, not a fixed-priority sprite. Native-only includes legitimate CPU transforms and fixed GX kernels; it does not permit a software scene compositor.

### N07.01 — Compile stage invariants and actor partitions

**Depends on:** N03.05, N02.03, N06.03

**Edit/inspect boundary:** `src/port/renderer_adapter_stage.c`; `src/port/renderer_adapter_matrix.c`; `existing native stage generators`; `src/nds/nds_renderer_native_common.c`.

**Implementation sequence**

1. Derive source stage static/moving/decorative/hazard classes and their transform/material/depth dependencies.
2. Compile static native runs and bound material/resource handles, preserving source draw ordering and geometry closure.
3. Keep dynamic actors and collision data under explicit mutation owners; camera changes update only view/projection/dependent billboards.
4. Remove repeated static DObj traversal, world-matrix construction and generic begin/commit setup for the converted stage partition.
5. Generate stage-specific bounds and resource/depth partitions for every selectable VS stage. Full fighter-stage combinations consume those records; no stage is disallowed because its native hazards or presentation cost more.

**Required tests/evidence:** T-STAGE full geometry/hazards/background, T-XFORM camera changes, T-COLL stage identities and T-DEPTH order; measure static/dynamic costs separately.

**Work or dependency retired:** Per-frame source graph/material work for stage data that did not change.

**Done:** The stage partition executes natively with complete content and a smaller hot preparation path.

**Stop/revert:** Do not classify animated hazards or source-translucent groups as static merely to skip their work.

### N07.02 — Replace remaining native actor setup scaffolding

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

### N07.03 — Convert particle simulation and event state

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

### N07.04 — Build a compact native particle draw batch

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

### N07.05 — Make UI fixed, value-driven and native

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

### N07.06 — Qualify GPU and ordering budgets for the full scene

**Depends on:** N07.02, N07.04, N07.05, N03.09

**Edit/inspect boundary:** `native renderer submission seam`; `source-derived stage/owner manifests`; `existing graphics capture/analysis scripts`.

**Implementation sequence**

1. Collect actual geometry/vertex/polygon, matrix command, texture/palette, blend/depth and FIFO wait populations for complete four-way scenes.
2. Check the configured original-DS hardware limits and current renderer slot conventions; do not infer increased per-buffer capacity from 30 Hz presentation.
3. Test worst overdraw/translucency/particle and hardest stage/fighter/item combinations separately from CPU-only hotspots.
4. Apply only approved existing quality settings; any new reduction needs a measured conflict and owner approval outside the transparent optimization verdict.
5. Search scene-level GPU/overdraw/matrix/FIFO leaders independently of ARM9 and heap leaders. Exercise four-way effect overlap with stage hazards, legal item children and camera extremes; direct rendering is not proof of sufficient geometry/raster capacity.

**Required tests/evidence:** T-GPU capacity and backpressure, T-DEPTH correctness, T-COVER full visible work and T-MEAS actual presentation cadence.

**Work or dependency retired:** Unnecessary graphics commands/overdraw only where equivalent native output is proved.

**Done:** CPU wins do not cause hidden GPU overflow, stalls or content loss; limits and headroom are documented.

**Stop/revert:** A CPU-only pass with GPU failure or absent geometry is RED; do not hide it behind fewer emitted primitives.

### N07.07 — Retire converted stage/particle/UI compatibility paths

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

