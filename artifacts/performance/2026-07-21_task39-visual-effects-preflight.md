# Task 39 visual-effects preflight

Verdict: **STOP_BEFORE_PHASE_C**. The task's structural map is contradicted by
the current tree, so its stop rule applies before instrumentation or fixes.

## Contradiction

The planner says shield has no DS-specific code and uses the untouched generic
DObj/material path. Current code instead weak-routes
`efManagerShieldMakeEffect` directly to
`ndsEFManagerMakeVisualEffect(nNDSVisualEffectShield, ...)` in
`src/port/reloc_backend_compat_shims.c:1619-1631`.

That route is explicitly DS-owned:

- the imported wrapper renames the original shield symbol at
  `src/import/battleship_efmanager.c:149,184`;
- the DS visual template table includes shield at `:212` and constructs the
  template around `:385`;
- shield/reflector select `nNDSVisualTemplateShield` at `:421-423`;
- the instance attaches to the fighter Y-rotation joint at `:674-676`.

The original BattleShip shield implementation remains the comparison source at
`decomp/BattleShip-main/decomp/src/ef/efmanager.c:4095-4148`, but it is not the
live public symbol on this port.

## Decision

The reported wrong shield can therefore be a substitute-template or adapter
fidelity issue, not the planner's claimed generic-material-only issue. Per Task
39's first stop rule, no per-entry counters, OAM/VRAM work, flags, screenshots,
or visual changes were added. Resume only after the architecture map is corrected
and the owner returns the required marked contact sheet; then census the actual
original/substitute/particle owners before choosing a fix lane.
