# P2-4 — Complete Stage Presentation, Actors and Moving Collision

Reuse the native stage/blob pipeline and the static collision checker. Finish shared rendering/collision seams, then qualify every stage and profile. Neither a successful packet submission nor static collision parity proves the player-visible stage is complete.

## Scope and ownership

Nine VS venues include the P1 Dream Land control and eight production stages. Existing unit files own the eight; this parent also owns Dream Land regression and its source-selected small profile. P2-6 consumes the separate campaign venues/profiles and 24 bonus boards; a registered packet count is not a count of accepted maps.

Source collision, map objects, bounds, material data, joint animation, hazard logic, background/set pieces and music all belong in the stage inventory. Avoid anatomical/map descriptions guessed from screenshots when the data provides exact geometry.

## Stage and capability map

| Venue | Unit / primary capability |
|---|---|
| Dream Land / Pupupu | Existing P1 stage; wind, eyes/mouth/flowers, approved frozen water; preserve current control |
| Yoshi's Island / Yoster | `stages/yoshis-island.md`; static platforms plus three cloud state/visibility/collision owners; YosterSmall is a distinct campaign profile |
| Peach's Castle / Castle | `stages/peachs-castle.md`; roof/materials, animated platform, item GBumper, Lakitu |
| Congo Jungle / Jungle | `stages/congo-jungle.md`; moving platforms and barrel capture obstacle |
| Hyrule Castle / Hyrule | `stages/hyrule-castle.md`; static towers/terrain and tornado obstacle/particles |
| Planet Zebes / Zebes | `stages/planet-zebes.md`; acid damage hazard and parented draw height |
| Sector Z / Sector | `stages/sector-z.md`; Arwing motion/collision and both weapon pipelines |
| Saffron City / Yamabuki | `stages/saffron-city.md`; moving door/platforms and five stage item monsters/eggs |
| Mushroom Kingdom / Inishie | `stages/mushroom-kingdom.md`; bricks, pipe states, pressure-driven scales, POW/Piranhas, animated background |

The original hazard-order list is not a mandate to redo already imported maps. Source line count describes source size, not measured DS runtime cost or an authorization to reorder owner priorities. Batch only demonstrated shared failures.

## Package: static geometry and materials

**Outcome:** Required main floors/platforms, foreground geometry and background treatment are visible with correct source-derived materials and camera meaning.

Use existing descriptors, compiled native packets and source collision imports. Verify packet/root identity, selected layers, vertex binding/transform, texture frame/UV, palette, final combiner alpha and culling/depth. Do not assume source texel alpha is the final material alpha. Classify absent geometry separately from alpha discard, occlusion, UV slicing or wrong layer selection.

**Proof:** Host collision/source comparison, original-asset reference crops, and natural scene captures of the exact generated corpus. Include a known-present positive control and a deliberate failing input for relevant host validators. Old captures are not qualification for newly regenerated material/texture data. Preserve rejected Castle-roof theories as history; require new contradictory evidence before reopening one.

## Package: moving collision

**Outcome:** Fighters and items interact with visible moving platforms at the source-defined position and phase.

Keep source joint animation, yakumono update tick, displacement, collision enable/disable and rider attachment coherent. Validate map-object counts/indices at conversion or admission; do not enter imported fatal loops on malformed data. Static data parity cannot cover runtime displacement or rider carry.

**Proof:** Ride, land, drop through where allowed, jump/dismount, edge transition and item bounce at multiple phases/directions. Observe both the visible carrier and collision position in the same source-time window. Include cloud disappearance/reappearance and scale load/retract where applicable. A diagnostic collision-line overlay never ships as replacement geometry.

## Package: actor, item, weapon and particle closure

**Outcome:** Every source-created required actor has the right native draw route, asset lifetime and gameplay interaction.

Ground-obstacle/capture (Jungle, Hyrule), ground-hazard/damage (Zebes, POW), item-owned actors (GBumper, plants, Saffron monsters), weapon-owned lasers and particle-only tornado output are different paths. Classification must follow their actual creators and display callbacks. Correct logic with no visible telegraph remains RED.

Preserve original update/timing/attribution while selecting native geometry/OAM/BG/particle representations. A native sprite can implement an appropriate visual if geometry/telegraph and budget requirements are demonstrated; do not impose unsupported “every actor must be a model” rules. Item dependencies close in P2-5, with stage natural-path integration proved here. No damage-only egg substitute or invisible spawn permission.

**Proof:** Complete natural actor cycle—spawn/arrival, activity, interaction, departure/despawn and another cycle—with engaged child effects/items/weapons. Confirm native pixels, collision/damage and sounds together. A missing child returns to the named shared owner.

## Package: venue qualification

For every stage/profile, retain a compact matrix for collision data, runtime movers/hazards, static/background presentation, effects/audio, camera/bounds, resource peak, lifecycle and cadence. Source IDs/profile inputs identify each matrix; no manual duplicate mutable completion table is required.

Capture stage-specific symptom dimensions, not just whole-screen “looks good.” Check a representative return to CSS/another map. A change to shared render data or code rechecks an affected sibling and Dream Land where relevant. Per-landing four-CPU measurement and final measured stress search use the current goal/board policy; owner-deferred performance remains an explicit intermediate result.

## Dream Land and campaign profile guards

Keep wind meaning, animated face/flowers, background movement, source music and the owner-approved frozen water at source frame 0. Do not “fix” that accepted delta. Resolve `PupupuSmall` from its source table/consumer; do not map it to VS Pupupu because the name matches. `YosterSmall` is owned in the Yoshi unit. Campaign arenas and bonus boards use the same admission/lifecycle discipline, with their own bounds and content.

## Exit checklist

- [ ] All eight VS units and Dream Land regressions satisfy source collision and required presentation.
- [ ] Movers, hazards, their children and visible/audio telegraphs pass natural-cycle tests.
- [ ] Every required draw state is native and covered by output evidence, not only submission.
- [ ] All admitted profiles meet resource constraints and scene transitions remain stable.
- [ ] Required stress/cadence measurements, source comparisons and owner approvals are banked.

## Source and retained evidence

Repository/source baseline: `907c46daffbec55477459cc56e83dfc9a417dabb` (September 10, 2026). This revision defines work and acceptance; it does not claim a new build or runtime pass. Current state belongs to `docs/P2_EXECUTION_BOARD.md`; owner symptoms belong to `docs/BUGS.md`.

- `scripts/stages/generate_nds_native_stage.py`.
- `scripts/stages/native_stage_descriptors`.
- `scripts/stages/check_collision_parity.py`.
- `src/port/renderer_adapter_stage.c`.
- `src/port/reloc_backend_mp_collision.c`.
- `decomp/BattleShip-main/decomp/src/gr/grcommon`.
- `decomp/BattleShip-main/decomp/src/mp/mpcollision.c`.

[Pre-revision document and its source pins](https://github.com/rockenrooster/Smash64DS_Port/blob/907c46daffbec55477459cc56e83dfc9a417dabb/docs/p2/P2-4-stage-production.md). The bundle installer preserves that document verbatim under `docs/archive/P2_PLAN_BASELINE_2026-09-10/p2/P2-4-stage-production.md`. Use retained investigations only when relevant; superseded diagnoses are not new implementation instructions.
