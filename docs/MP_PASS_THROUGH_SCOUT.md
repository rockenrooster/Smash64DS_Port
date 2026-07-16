# MP Pass-Through / Platform Source Reference

Status: the bounded modes `139`-`154` and their direct/menu-chain wrappers were
retired on 2026-07-15. Canonical mode `163` now owns executable P1 platform
semantics through `scripts/verify-battle-playable-platform-semantics.ps1`.

## Scope

This note retains the BattleShip source map for pass-through floors and moving
yakumono. It is not a harness inventory or current progress surface.

## BattleShip Source Findings

- `decomp/BattleShip-main/decomp/src/mp/mpdef.h` defines
  `MAP_VERTEX_COLL_PASS` and `MAP_PROC_TYPE_PASS`.
- `decomp/BattleShip-main/decomp/src/mp/mpcommon.c` routes pass collisions
  through `mpProcessCheckTestFloorCollisionAdjNew` and the optional pass
  callback.
- `decomp/BattleShip-main/decomp/src/mp/mpprocess.c` rejects the current
  `ignore_line_id` and accepts a different pass floor only when its callback
  allows the crossing.
- `decomp/BattleShip-main/decomp/src/mp/mpcollision.c` owns yakumono existence,
  on/off state, position, speed, collision transforms, and bounds updates.
- `decomp/BattleShip-main/decomp/src/gr/grdisplay.c` attaches yakumono animation
  only when the authored layer supplies DObj or material animation data.

## Retained P1 Contract

The natural-runtime verifier exercises all three Dream Land platforms and
requires:

- upward crossing without grounding or clamping;
- strict airborne descent and exact downward plane crossing;
- stable landing on the selected platform line;
- source Wait -> Squat -> Pass initialization;
- same-line pass rejection while `ignore_line_id` is active;
- cleanup of the ignored line and landing on the main floor;
- matching mode-163 runtime state and a screenshot under
  `artifacts/visibility`.

Run it only when platform/collision semantics change:

```powershell
.\scripts\verify-battle-playable-platform-semantics.ps1
```

## Implementation Ownership

Gameplay meaning remains sourced from BattleShip `mpcommon`, `mpprocess`,
`mpcollision`, and `ftcommonpass`. DS compatibility and decoded geometry live
under `src/port`; original runtime imports live under `src/import`. Do not
recreate bounded proof modes for individual branches.

## Deferred

Full authored moving-platform behavior and Mushroom Kingdom/Inishie scale
presentation remain outside the P1 Dream Land requirement. Revisit them for P2
through natural runtime, not by restoring modes `139`-`154`.
