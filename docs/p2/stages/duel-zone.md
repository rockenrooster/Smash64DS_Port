# Duel Zone — P2-6 venue (Fighting Polygon Team)

Status: not started · Reference: BattleShip stage data via `docs/DECOMP_MAP.md`.

1P-only: the Polygon wave battle venue ("Battlefield").

## Content inventory

- **Layout**: flat main platform + three floating pass-through platforms
  (the classic triple arrangement — verify offsets from data), no hazards.
- **Set pieces**: abstract void/wireframe-ish backdrop.
- **Music**: Duel Zone theme.
- **Visual treatment**: minimal by design — cheapest visual build in P2-6.

## DS notes / risks

- The venue itself is trivial; the load here is the *fight* — player + 3
  concurrent polygons from a 30-stock wave pool + items. This stage's
  measurement row is really the polygon-wave stress row
  (`fighters/variants.md`).
- Wave spawn/despawn churn on this stage is the heap-watermark test for
  sequential-team machinery.

## Acceptance

- [ ] Collision parity sweep (flat + 3 platforms).
- [ ] Polygon wave battle (3 concurrent, 30 total) correct and within
      stress envelope, heap watermarks flat across waves.
- [ ] Music live; campaign-flow load works; owner visual pass.
