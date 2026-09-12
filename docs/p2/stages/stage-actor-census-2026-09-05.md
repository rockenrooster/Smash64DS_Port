# Stage Actor Census — September 5 Reference Index

The original delegated census was explicitly low-confidence for unmeasured offsets/sizes and contains now-superseded no-render-path observations. Preserve it as discovery evidence, not an implementation checklist. Current source requirements belong to the stage units; status belongs to the board.

## Actor ownership map

| Source actor | Correct conceptual seam to inspect | Owning unit |
|---|---|---|
| Pupupu face/flowers and wind effects | Source stage actors plus particles; existing Dream Land native control | `../P2-4-stage-production.md` |
| Yoster clouds | Runtime-created ground children, material states and collision on/off | `yoshis-island.md` |
| Castle bumper / Lakitu | GBumper item / separate source effect | `peachs-castle.md` |
| Jungle cannon | Ground obstacle/capture, animated barrel and source fighter launch | `congo-jungle.md` |
| Hyrule tornado | Ground obstacle/capture and particle visual | `hyrule-castle.md` |
| Zebes acid / background effects | Ground damage hazard with parented height / separate effects | `planet-zebes.md` |
| Sector Arwing / lasers | Moving collision actor / source weapon pipelines | `sector-z.md` |
| Yamabuki door / monsters | Moving ground door / five stage-item makers and their children | `saffron-city.md` |
| Inishie scales / POW / plants | Moving collision / item-plus-hazard / items | `mushroom-kingdom.md` |
| Bonus targets/platforms and Race actors | Source bonus rules, Target/GBumper/TaruBomb and corresponding native owners | `bonus-stages.md`, `race-to-the-finish.md` |

## How to use the old census

Retrieve only the actor's creator, display callback, asset root and verified source identities. Recheck any UNVERIFIED/LOW-confidence binding before using it. Distinguish source Gfx commands from words/bytes when pricing display-list data; do not copy old informal size arithmetic into capacity gates.

No proposal to “keep legacy item/effect rendering” survives the all-ROM native-only rule. Conversely, do not rewrite a source gameplay actor simply because its rendering needs a native owner. A source actor can retain its behavior and submit a specialized native visual.

An old “no route” observation does not refute a later committed owner; a later submitted triangle count does not prove current pixels. Establish the immutable build/input baseline, shortest natural trigger and first wrong observable dimension. Use that evidence to update the active unit/board without repeating the whole census.

## Source and retained evidence

Repository/source baseline: `907c46daffbec55477459cc56e83dfc9a417dabb` (September 10, 2026). This revision defines work and acceptance; it does not claim a new build or runtime pass. Current state belongs to `docs/P2_EXECUTION_BOARD.md`; owner symptoms belong to `docs/BUGS.md`.

- `docs/p2/P2-4-stage-production.md`.
- `src/port/reloc_backend_movement.c`.
- `src/port/renderer_adapter_stage.c`.
- `scripts/stages/native_stage_descriptors`.
- `decomp/BattleShip-main/decomp/src/gr/grcommon`.

[Pre-revision document and its source pins](https://github.com/rockenrooster/Smash64DS_Port/blob/907c46daffbec55477459cc56e83dfc9a417dabb/docs/p2/stages/stage-actor-census-2026-09-05.md). The bundle installer preserves that document verbatim under `docs/archive/P2_PLAN_BASELINE_2026-09-10/p2/stages/stage-actor-census-2026-09-05.md`. Use retained investigations only when relevant; superseded diagnoses are not new implementation instructions.
