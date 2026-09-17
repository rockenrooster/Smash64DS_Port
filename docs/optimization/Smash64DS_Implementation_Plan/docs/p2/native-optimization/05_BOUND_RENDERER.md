# N03A — Bound native renderer and mutation-owned preparation

> **Revision 2 coverage:** Universal scope: the simple actor and hot-fighter pilots establish the interface and cost, not a supported-roster allowlist. N03.11 proves independent same-kind instances and explicit slot-sensitive behavior before the renderer family is closed. See [16_ALL_ROSTERS_ALL_STAGES.md](16_ALL_ROSTERS_ALL_STAGES.md).

**Target:** a successful draw/replay consumes validated native bindings, not source-shaped state reconstruction. Sources [S07, S09, S10, S11, S12, S15, S17].

## Current seams and replacement direction

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

## Minimal runtime execution shape

```text
bind_on_real_change(instance, admitted_bank, topology, resource_epoch)
prepare_changed_inputs(instance, fixed_pose, world, camera, materials)
submit_bound_draw(instance)
```

These are semantic operations, not mandated function names. `bind_on_real_change` may be split across existing scene/status callbacks. It must not poll the source graph every frame to decide whether something changed. Preserve current native execution for unmigrated owners; retire it per owner only after replacement coverage.

### N03.01 — Partition preflight into immutable and live checks

**Depends on:** N00.03, N00.05, N02.01

**Edit/inspect boundary:** `src/nds/nds_renderer_native_fighter_production.c`; `src/nds/nds_renderer_assets.c`; `src/nds/nds_renderer_native_common.c`.

**Implementation sequence**

1. Walk each preflight operation and record its inputs, mutation owner, lifetime and failure output. Classify bank-immutable, instance-topology, material/residency or genuinely per-draw.
2. Price successful replay hits separately from misses/first use and capture which validation entries actually run.
3. Specify the bound descriptor from C3 using existing native tables wherever possible. Preserve copied/foreign-root ownership and distinguish slot, fighter kind and instance identity.
4. Add tests for every moved validation condition before relocating its execution boundary.
5. Classify invariant checks over all generated owners, stages and variants. Do not bake a small pilot hierarchy limit, one texture format, or a particular roster into the bound ABI.

**Required tests/evidence:** T-BIND corrupt bank/foreign root/stale epoch/wrong model variant tests; replay-hit counters and measured perflight cost; no candidate branch changes yet.

**Work or dependency retired:** Only immutable checks demonstrated to repeat are marked for removal; no speculative bulk validation deletion.

**Done:** A validation/mutation matrix states exactly what remains on the hot path and why.

**Stop/revert:** Any unowned mutation remains conservatively checked until its writer is identified; do not substitute an optimistic pointer comparison.

### N03.02 — Implement binding and explicit invalidation

**Depends on:** N03.01, N04.02

**Edit/inspect boundary:** `src/port/renderer_adapter_matrix.c`; `src/import/battleship_ftmain.c`; `src/nds/nds_renderer_assets.c`; `proposed include/nds/nds_native_binding.h`.

**Implementation sequence**

1. Add small bound-instance records or extend current records; keep immutable bank references distinct from per-instance mutation state.
2. Hook genuine scene, spawn, status topology, model-part, copy/morph, costume and resource-epoch changes using C4. Verify all current invalidation callers before narrowing them.
3. Build parent/root/material bindings once per relevant change and publish only after all referenced resources and patch metadata are valid.
4. Expose fixed native transforms through a narrow API. During migration explicitly mark any legacy producer bridge and its cost; no new floating conversions inside the emitter.
5. Key mutable bindings by stable instance identity plus generation, not fighter kind alone. Same-kind instances may share read-only packets/banks but must not share writable patches, pose cursors, materials, light state or copy ownership.

**Required tests/evidence:** T-BIND same-root reparent, same-address reuse, four identical fighters with different costumes, Kirby copy, Samus morph, rapid status changes and generation wrap.

**Work or dependency retired:** Repeated immutable root/material/parent discovery for bound instances.

**Done:** All bound fields have a mutation owner; tests invalidate the right scope without invalidating unrelated instances.

**Stop/revert:** A stale attachment or valid-looking wrong-generation handle blocks the migration; restore conservative ownership until fixed.

### N03.03 — Prove the contract on one small actor

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

### N03.04 — Move a hot fighter to the bound hit path

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

### N03.05 — Specialize transform and render classes

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

