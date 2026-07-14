# Harnesses

<!-- HARNESS_INDEX_SOURCE: scripts/lib/harness-registry.ps1 -->

The authoritative harness index is `scripts/lib/harness-registry.ps1`. Do not
maintain a second hand-written list of all modes in this doc. Generate the
current rows with:

```powershell
.\scripts\verify-all.ps1 -Profile Full -List
.\scripts\verify-all.ps1 -Profile Boundary -List
.\scripts\verify-all.ps1 -Profile Latest -List
```

`scripts/check-harness-registry.ps1` validates that the registry, Makefile
mappings, mode defines, wrapper scripts, and profile plans stay in sync.

## Current Boundary

The current Boundary playable-spine set is:

```text
battle_playable_realtime  mode 163
```

This scene-level owner runs the canonical one-minute Mario-human/Fox-CPU match
with exact Wait-to-GO control/timer behavior, source CPU/input, retained affine
BG2 wallpaper, and live hardware stage/fighter submission. Modes `161/162`
remain registered for diagnosis but are no longer Boundary/Latest members:
their bounded input driver assumes pre-GO movement, which exact BattleShip
control locking correctly prevents. Do not add a synthetic unlock to restore
their marker stack; the coverage reduction is recorded in KNOWN_ISSUES.

`battle_playable_match_lifecycle` is a registry alias for the same scene mode,
not a new gameplay mode. Its compile-distinct verifier target uses the source
CPU/live setup and a one-minute harness timer to prove the original
timer-expiry/end transition to VS Results. Run the full natural state/memory
gate with `scripts/verify-battle-playable-one-minute-match.ps1`; it reuses mode
163 and does not add a harness or proof-mask mode.

## Naming Rules

- Direct battle harnesses start with `battle_`.
- Menu-chain harnesses start with `menu_chain_`.
- Mario/Fox fighter proof pairs use `battle_mariofox_*` and
  `menu_chain_mariofox_*`.
- Stage/MP proof pairs include `stage_` and the narrow source boundary, such as
  `stage_mpcross_floor_loop`.
- A new direct Mario/Fox harness normally needs a matching menu-chain harness.
  Document any exception in `docs/HANDOFF.md` and `docs/KNOWN_ISSUES.md`.

## Adding A Harness Pair

For a normal direct/menu-chain pair:

1. Inspect the relevant original BattleShip source under `decomp/`.
2. Add or extend the narrow source import in `src/import`.
3. Add mode defines in `include/nds/nds_scene_harness.h`.
4. Add diagnostics in `include/nds/nds_startup.h` and `src/port/diagnostics.c`.
5. Wire harness seeding in `src/port/scene_harness.c`.
6. Wire taskman/scene finalization in `src/port/taskman_seam.c`.
7. Add Makefile `NDS_DEV_SCENE_HARNESS` mappings.
8. Add verifier wrapper scripts.
9. Add registry records and profile placement in
   `scripts/lib/harness-registry.ps1`.
10. Update `scripts/check-harness-registry.ps1` expected profile plans.
11. Update `docs/STATUS.md`, `docs/HANDOFF.md`,
    `docs/DIAGNOSTIC_REFERENCE.md`, and append `docs/PORTING.md`.
12. Run `check-harness-registry.ps1`, the new direct/menu verifiers,
    `verify-boundary.ps1`, and `verify-current.ps1`.

## Profiles

- `Latest`: runtime, Title, and canonical `battle_playable_realtime`.
- `LatestFast`: Title plus canonical `battle_playable_realtime`.
- `BoundaryDirect` / `Boundary`: canonical `battle_playable_realtime`.
- `P1Gate`: additive four-leg shadow checkpoint: compact opening smoke,
  canonical realtime `FastIteration`, supplemental deterministic mode-163
  battle playback, and the one-minute mode-163 lifecycle/Results verifier.
- `Regression`: historical playable-spine coverage plus the current boundary.
- `RegressionCore`: runtime/title, canonical realtime presentation, the
  one-minute lifecycle, one cliff proof, and the direct/menu MP floor pair.
- `Full`: all registered verifiers.

`P1Gate` does not change Boundary, Regression, or Full profile membership, and
a pass is neither P1 completion nor the required one-minute full-match soak. The legacy
harness fleet remains available for diagnosis; this additive profile does not
delete or graduate its unique assertions.

Use `docs/VERIFYING.md` for when to run each profile.

## Generator Direction

Manual copy/paste is now a risk. The next tooling improvement should be a
harness-pair generator that updates the registry, Makefile mapping, mode define,
wrapper scripts, docs index, and proof-template text in one controlled path.
Until then, registry checks are the required guardrail.
