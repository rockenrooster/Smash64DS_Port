# Yoshi's Island — P2-4 stage 1 (pipeline prover)

Status: not started · Reference: BattleShip stage data via `docs/DECOMP_MAP.md`
(`gr/` + `mp/` collision; no stage-hazard logic).

## Why first

No hazards at all — the cheapest full pass through the whole stage pipeline
(collision import, geometry build, background, camera, music, SSS entry).

## Content inventory

- **Layout**: sloped/undulating main terrain (no flat ground — tests slope
  collision everywhere), side cloud platforms that act as soft platforms
  (verify exact pass-through/dissipate behavior in source — players expect
  the clouds to support briefly), upper platforms.
- **Hazards**: none.
- **Set pieces**: Super Happy Tree background, Fly Guys/props (background
  only — verify nothing background interacts with gameplay).
- **Camera/blast zones**: from source data.
- **Music**: Yoshi's Island track through the streaming path.
- **Visual treatment**: storybook/crayon look — strong candidate for baked
  vertex colors + 2D BG layers behind low-poly terrain.

## DS notes / risks

- Slope-heavy collision is the real test: every movement state (dash, crawl,
  knockdown slides, item bounces later) on non-flat ground.
- Cloud platform semantics are the one equivalence subtlety — source first.

## Acceptance

- [ ] Collision parity sweep (slopes, clouds, ledges, blast lines) vs
      imported data.
- [ ] Camera bounds equivalent; spawn/respawn points correct.
- [ ] Music + SSS entry live; owner visual pass with screenshot.
- [ ] 4-CPU stress measurement on this stage banked.
