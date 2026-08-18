# P2-4 — Stage Production (pipeline + the remaining 8 VS stages)

Same industrialization as P2-3, for stages. Dream Land (P1) is the exemplar;
every stage must meet the Stage Completeness Standard in `PROJECT_GOAL.md`.
Highly stage-specific renderers are explicitly fine.

## Pipeline generalization (first slice)

1. **Collision import**: ground/wall/ceiling/platform geometry, ledge grab
   points, pass-through flags, blast zones, spawn/respawn points from
   BattleShip map data (`gr/`, `mp/` and per-stage data via `DECOMP_MAP.md`).
2. **Geometry/visual build**: DS-budget stage mesh, baked lighting/vertex
   colors, background treatment (3D, 2D BG layers, or hybrid per stage —
   cheapest recognizable wins), within the P2-2 per-stage budget.
3. **Hazard seam**: stage hazards are stage-owned update hooks with
   mechanically equivalent behavior from BattleShip (`gr/grcommon/` + per-stage
   logic) — no generic hazard interpreter.
4. **Camera/bounds + music** per stage; SSS art (map icon, name).
5. **Acceptance harness**: collision parity sweep (probe walk of surfaces,
   ledges, blast lines vs imported data), hazard behavior checks, 4-CPU
   stress measurement on the stage.

## Stage order (owner-ratified, hazard complexity ascending)

| # | Stage | File | Hazards / notable |
|---|---|---|---|
| 1 | Yoshi's Island | `stages/yoshis-island.md` | No hazards; cloud platforms — cheapest full pipeline pass |
| 2 | Peach's Castle | `stages/peachs-castle.md` | Bumper, sliding platform |
| 3 | Congo Jungle | `stages/congo-jungle.md` | Barrel cannon underneath |
| 4 | Hyrule Castle | `stages/hyrule-castle.md` | Tornado; large multi-terrain layout |
| 5 | Planet Zebes | `stages/planet-zebes.md` | Rising/falling acid |
| 6 | Sector Z | `stages/sector-z.md` | Arwing strafing runs; biggest stage — perf risk |
| 7 | Saffron City | `stages/saffron-city.md` | Pokémon door spawns; rooftop gaps |
| 8 | Mushroom Kingdom | `stages/mushroom-kingdom.md` | Warp pipes, POW, Piranhas, walk-off — most bespoke systems |

1P-only venues (`final-destination`, `meta-crystal`, `duel-zone`,
`race-to-the-finish`) and the 24 bonus boards (`stages/bonus-stages.md`) reuse
this pipeline but are P2-6 scope.

## Standing rules for every stage row

- Inspect the stage's BattleShip data/logic before building; hazards are
  gameplay (mechanical equivalence), backgrounds are presentation (visual
  doctrine applies — timebox exactness, record deltas with screenshots).
- Whispy precedent: P1's Dream Land wind implementation is the reference for
  how a hazard integrates with fighter physics.
- Each stage lands fully (collision, hazards, music, SSS entry) before the
  next starts; each closes with a 4-CPU stress measurement on it, and the
  hardest-stage argmax updates the standing stress config.
- Music per stage through the existing streaming path; sound-RAM budget per
  P2-2.

## Exit criteria

- [ ] Pipeline reproducible from BattleShip data by `make`.
- [ ] All 8 VS stages landed per unit DoD; SSS fully populated (Mushroom
      Kingdom unlock-gated in P2-7, selectable in dev builds).
- [ ] Collision parity sweeps green on every stage.
- [ ] Stress config re-argmaxed including stages; board updated.
