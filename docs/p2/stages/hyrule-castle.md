# Hyrule Castle — Static Terrain and Tornado Capture

Stage completion contract over the existing native packet and source behavior. Current symptoms, candidate identity and closure state belong to `docs/BUGS.md` and the execution board.

## Preserve and reuse

Keep source terrain/map-object import, native layers, stage setup and the source tornado state machine. The first natural 1P fight consumes this venue. Do not schedule a wholly new terrain or generic hazard system because an old census called the tornado unrendered.

## Completion packages

**Static layout and view.** Qualify source floor/ramps/towers/roof surfaces, ledges/gaps, materials/background and camera/bounds without inventing geometry from a sketch. Check a wide separated-fighter view and source foreground layers; static collision and visible material output are distinct proofs.

**Tornado behavior and telegraph.** Preserve source Sleep/Wait/Summon/Move/Turn/Stop/Subside transitions, source position selection and steering behavior. Its ground-obstacle/capture state and particle visual must agree in world space and source time. A invisible damage/capture volume is unacceptable even when logic is source-faithful.

**Fighter interaction.** Natural approach/contact must produce the correct capture/damage/launch, input effects and exit/cooldown behavior as source specifies. Test source interaction during turning/stopping, eligible/ineligible cases and source cleanup if the fight ends mid-cycle.

**Import safety.** The source rejects a zero or excessive Twister map-object count with a fatal loop. Validate the source allowed count/index domain before entry; do not modify gameplay to conceal malformed map data.

## Dependencies and lifetime

Tornado is an obstacle with particle/native effect output, not the same damage-hazard path as acid. Its exact particle assets and source telegraph are mandatory atlas roots. Hyrule static/actor content must be available to the P2-6 Link encounter; correct VS selection alone does not qualify 1P setup.

## Natural-path proof

Host collision/map-object constraints, an engaged full tornado appearance/contact/exit cycle with visible particles and source result, wide camera, actual music, and natural first-1P-fight use. Prove the dynamic cycle rather than rely on a snapshot during source tornado sleep.

Static collision parity covers source data, not moving collision or required visible pixels. Use `../P2-4-stage-production.md` for shared material/actor/scene/stress requirements. New texture/material corpus inputs require current captures for affected output; old packet admission cannot replace them.

- [ ] Source collision/map objects/bounds and spawn points match the selected profile.
- [ ] Every required static and dynamic visual, telegraph and audio element is present natively.
- [ ] Source movers/hazards and their children pass the specified natural-cycle interactions.
- [ ] Entry/exit resource ownership, actual resource/cadence/stress gates and required owner review pass.

## Source and retained evidence

Repository/source baseline: `907c46daffbec55477459cc56e83dfc9a417dabb` (September 10, 2026). This revision defines work and acceptance; it does not claim a new build or runtime pass. Current state belongs to `docs/P2_EXECUTION_BOARD.md`; owner symptoms belong to `docs/BUGS.md`.

- `decomp/BattleShip-main/decomp/src/gr/grcommon/grhyrule.c`.
- `docs/p2/P2-4-stage-production.md`.
- `decomp/BattleShip-main/decomp/src/relocData/113_StageHyruleFile2.c`.
- `decomp/BattleShip-main/decomp/src/ft/ftmain.c: ground obstacle dispatch`.
- `docs/p2/P2-6-one-player.md`.

[Pre-revision document and its source pins](https://github.com/rockenrooster/Smash64DS_Port/blob/907c46daffbec55477459cc56e83dfc9a417dabb/docs/p2/stages/hyrule-castle.md). The bundle installer preserves that document verbatim under `docs/archive/P2_PLAN_BASELINE_2026-09-10/p2/stages/hyrule-castle.md`. Use retained investigations only when relevant; superseded diagnoses are not new implementation instructions.
