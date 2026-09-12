# Shared plan 03 — Admit the selected match into the original DS envelope

**Status:** proposed extension; no new resource measurements. Parent: [master](../New_Characters.md).

## Principle: resource fragments, not eighteen permanently resident fighters

Use the P2 match-resident pack and deterministic texture-placement direction. Verify its implementation state at kickoff; a recommendation in a review is not a landed feature. P4 supplies source-derived fragments to that system, not a parallel cache or gameplay-time fighter pager.

For a fixed match roster/rules/stage, take the transitive union of base resources, selected fighters, ordinary items, their articles and reachable copy abilities. Deduplicate immutable content by verified identity, keeping relocation/material-view semantics where needed. Count per-instance state separately.

```text
resident match cost = fixed code/data + unique immutable dependency union
                    + live fighter state + source-bounded gameplay pools
                    + renderer/audio/network/stacks + safety reserve
loading peak       = maximum simultaneous live allocations across scene epochs
```

Do not add every category blindly if one ledger category already owns its allocation. Include alignment, allocator metadata, compressed/decompressed overlap, old preview state, new match data and temporary native preparation. Prove lifetime peaks, not just free bytes after setup.

The whole roster still grows executable code, read-only tables, static data and menu metadata. Report `.text`, `.rodata`, `.data`, `.bss`, shared struct-size changes and maps by owner. Removing unselected models from RAM does not remove their statically linked code.

## Independent constraints

| Domain | Required evidence |
|---|---|
| Main RAM | Actual complete-match resident ledger, loading peaks, stack/pool high-water and required reserve. |
| Texture storage | Available bank map; format-aware texel and auxiliary placement, not only total bytes. |
| Palette/view resources | Palette bytes/bases/alignment, texture-view slot count, all material alternatives. |
| Geometry | Complete-scene generated polygons and vertices, matrix/command limits, hardware overflow diagnostics. |
| GPU work | Transparency/overdraw and render/FIFO completion under actual scene/camera stress. |
| CPU | Whole-frame P50/P95/P99, cadence distribution and subsystem contributions. |
| I/O/audio | Pre-GO resource availability, preview/read behavior, sustained audio and intentional streaming lanes. |

The DS geometry constraints include a 2,048-polygon list and 6,144-vertex capacity. These are independent of whether a character's source model seems small, and 30-FPS presentation does not double one submitted scene's hardware capacity. Measure the generated primitive representation, not an N64 triangle estimate. See [primary graphics documentation](https://blocksds.skylyrac.net/tutorial/intermediate/3d_graphics/).

Required fighter/model/texture/copy-ability assets must not fault in during combat. This does not ban the project's intentionally supported BGM streaming; keep its bandwidth and buffers in the measured ledger. No texture miss may invalidate an entire unrelated native stage or silently remove mandatory content.

## Gameplay object bounds come from semantics

For each article record creator, minimum emission interval, lifetime/expiry, source explicit caps, owner changes, destruction interactions and whether it outlives an action or stock. Derive possible overlap, including source-legal reflected/transferred/recreated objects. A simple lifetime/rate bound is a starting bound, not a proof when an article can be prolonged or handed off.

Banjo's forward/backward egg lifetime constants differ; Snake's configuration names five resource families but no live caps. Four mirrors and three Kirbys plus a donor exercise different pressure. Do not replace source behavior with an arbitrary tiny pool or assume all effect objects are cosmetic. When a source itself refuses creation at a cap, reproduce that rule rather than treating it as unbounded growth.

Separate required gameplay objects from optional cosmetic emitters. Optimize representation, reuse and scheduling before proposing a smaller gameplay population. Any forced omission follows PROJECT_GOAL and requires the existing owner approval policy.

## Efficient DS representation

Favor source-derived compact pose/animation streams, fixed-point or quantized representations with validated error bounds, precompiled GX commands, stable texture handles, prepared material state and event-driven part changes. Reuse the current native generators. Do not import the donor DObj/display-list interpreter as the permanent shipping fallback.

A visual pose updated at 30 Hz is not automatically a valid 60-Hz hitbox, grab or projectile origin. Compute the necessary gameplay joints/trajectories at their required event times, through a small specialized path or validated baked data. Test source animation speed changes and attachment dependencies explicitly.

Start from converted source art. Low-poly substitution, texture reduction, sprite effects, fewer particles or cheaper lighting are responses to measured conflicts—not automatic defaults for a new character. Preserve readable identity and mechanics, record the actual conflict, and obtain required approval for permanent compromises.

## Executable residency: conditional, not a new framework by default

First measure projected full-roster linked costs. Share equivalent native helpers and avoid wholesale duplication of generic tables. Use dead stripping where it genuinely removes unreachable code; selectable-but-unselected callbacks are not necessarily dead to the linker.

Only if measured executable/data growth still prevents the required roster should a separate design test match-boundary code overlays. Reference the existing sm64-nds and sm64ds-decomp overlay approaches; account for all selected fighters and copy callbacks, relocation, shared imports, instruction-cache maintenance and active pointer lifetimes. Never evict a callable routine or load code in response to a combat input. Overlay work is not a prerequisite just because 18 fighters are planned.

## Combination coverage without combinatorial ROM bloat

With 30 admitted selections, exactly four slots allowing repeats yield C(33,4) = 40,920 unordered multisets. Offline resource enumeration is practical; it is not an instruction to generate 40,920 payload packs or run that many full emulator matches.

Enumerate unordered multisets for order-independent immutable unions, plus multiplicity-sensitive mutable/pool bounds. Cover slot/owner ordering separately in runtime tests. Enumerate costumes and item/stage profiles, or group them only where a verified resource-equivalence bound justifies it. A representative costume sample cannot prove every palette placement fits.

Keep separate adversaries for distinct-asset RAM, mirror article pressure, Kirby copies, geometry, palette/views, transparency and CPU. No single per-fighter size ranking proves all axes. Host-side analysis may use conservative bounds and refinement; runtime match setup consumes compact fragments and a deterministic plan rather than scanning all roster combinations.

## Admission and failure

Fail before GO with the exact first unclassified/missing resource or failed constraint. Release admission requires the proposed any-4 scope; silently rejecting a previously promised combination at runtime is not a substitute for its proof. A failed unadmitted development candidate may be blocked safely, while its card remains open.

Use the current measured reserve and PROJECT_GOAL gates, not old P2 provisional byte ceilings. Read-only source size is not loaded DS size. Report unknown as unknown. Resource safety, gameplay correctness and P95/cadence qualification are separate checks; rare permitted CPU overruns never authorize memory/GPU overflow.
