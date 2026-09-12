# Sector Z — Arwing Orientation, Collision and Both Laser Routes

Stage completion contract over the existing native packet and source behavior. Current symptoms, candidate identity and closure state belong to `docs/BUGS.md` and the execution board.

## Preserve and reuse

Keep the source Arwing state/animation/weapon implementations and native hull/actor work. Source line count is not a DS cost measurement or permission to override current priority. The owner now reports visible but misoriented Arwings and visible collision lines; do not treat that as simply a missing ship.

## Completion packages

**Hull and camera.** Qualify source main geometry/materials, platforms/ledges, source background and camera/bounds at large fighter separation. Use existing native packets/collision data; do not introduce culling assumptions before pricing visibility and cost.

**Arwing orientation and motion.** Preserve source flight animation, custom transform/region selection and source facing along the intended path. Inspect parent/world/camera transform order when the ship faces the backdrop rather than left/right. A hardcoded 90-degree correction is not justified unless it is the actual source→DS coordinate transform for all relevant states.

**Moving collision.** Source Arwing rideable collision is enabled only in its declared near/active states and follows its true transform/offset. Test arrival, active/ride, departure and rider separation. Diagnostic collision lines are not required game art and must not remain visible in the published scene.

**Laser pipelines.** Source 2D and aimed 3D laser paths have separate creators/attributes/callbacks. Both must draw correctly, move/hit/map-interact, respect actual reflect/absorb/credit rules and release required children. Verify asset-base resolution and source transform conventions; a generic weapon manager link alone does not prove native laser pixels.

**Audio/decorations.** Preserve source ship/laser cues, music and required background effects. Test actual output while actor/weapon workload is active.

## Dependencies and lifetime

Arwing is a stage/collision actor; lasers are source weapons. Native weapon/effect capacity and required assets depend on the shared P2-5/P2-2/texture contracts. The US transform variant must match the source configuration; do not combine constants from multiple regions. P2-6 Fox encounter uses the same venue with campaign setup.

## Natural-path proof

Natural source flight cycle with both direction/orientation cases, active collision/rider checks, engaged 2D and 3D laser outcomes and visible impacts, then departure/another arrival. Compare output plus actual source matrices/collision state and no-debug-line production capture. Measure real workload rather than infer cost from source file length.

Static collision parity covers source data, not moving collision or required visible pixels. Use `../P2-4-stage-production.md` for shared material/actor/scene/stress requirements. New texture/material corpus inputs require current captures for affected output; old packet admission cannot replace them.

- [ ] Source collision/map objects/bounds and spawn points match the selected profile.
- [ ] Every required static and dynamic visual, telegraph and audio element is present natively.
- [ ] Source movers/hazards and their children pass the specified natural-cycle interactions.
- [ ] Entry/exit resource ownership, actual resource/cadence/stress gates and required owner review pass.

## Source and retained evidence

Repository/source baseline: `907c46daffbec55477459cc56e83dfc9a417dabb` (September 10, 2026). This revision defines work and acceptance; it does not claim a new build or runtime pass. Current state belongs to `docs/P2_EXECUTION_BOARD.md`; owner symptoms belong to `docs/BUGS.md`.

- `decomp/BattleShip-main/decomp/src/gr/grcommon/grsector.c`.
- `docs/p2/P2-4-stage-production.md`.
- `decomp/BattleShip-main/decomp/src/gr/grcommon/grsector.h`.
- `decomp/BattleShip-main/decomp/src/relocData/262_GRSectorMap.c`.
- `decomp/BattleShip-main/decomp/src/relocData/109_StageSectorFile2.c`.

[Pre-revision document and its source pins](https://github.com/rockenrooster/Smash64DS_Port/blob/907c46daffbec55477459cc56e83dfc9a417dabb/docs/p2/stages/sector-z.md). The bundle installer preserves that document verbatim under `docs/archive/P2_PLAN_BASELINE_2026-09-10/p2/stages/sector-z.md`. Use retained investigations only when relevant; superseded diagnoses are not new implementation instructions.
