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

The current Boundary and Latest playable-spine set is:

```text
battle_mariofox_stage_mplivehit_status_loop      mode 161
menu_chain_mariofox_stage_mplivehit_status_loop  mode 162
battle_playable                                  mode 163
```

These modes keep the live battle scene on Pupupu/Dream Land, create Mario/Fox
through the imported original manager, and drive Wait -> Walk -> Dash -> Run ->
RunBrake -> Turn, Fox Attack11, live hitbox search, Mario damage/recover, and
GuardOn/Guard/GuardOff through imported `ftanim.c`/`ftkey.c`, original status
descriptors, `ftmain.c`, and `gmcollision.c`.

They now build and verify DS 3D hardware submission by default:
`hwsubmit=252`, `hwtri=1152`, `hwftr=2/582`, and
`hwtex=bind582/upload66/ready582/reject0/fmt4/max32x32`. Pass
`-SoftwarePreview` to the wrappers only for comparison runs. Use
`-DelaySeconds 3` for the current Boundary/Latest profiles so the menu-chain
finalizer markers are captured.

Mode `163` is the scene-level `battle_playable` anchor. It reuses the gcRunAll
natural-combat verifier path, adds stock KO -> Rebirth -> Wait assertions, and
requires a hardware-triangle stage + fighter frame.
`battle_playable_match_lifecycle` is a registry alias for the same scene mode,
not a new gameplay mode. Its compile-distinct verifier target uses the source
CPU/live setup and a one-minute harness timer to prove the original
timer-expiry/end transition to VS Results.

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

- `Latest`: runtime, Title, the current direct/menu boundary pair, and
  `battle_playable`.
- `BoundaryDirect`: current direct boundary only.
- `Boundary`: current direct/menu boundary pair plus `battle_playable`.
- `P1Gate`: additive four-leg shadow checkpoint: compact opening smoke,
  canonical realtime `FastIteration`, supplemental deterministic mode-163
  battle playback, and the one-minute mode-163 lifecycle/Results verifier.
- `Regression`: historical playable-spine coverage plus the current boundary.
- `RegressionCore`: runtime/title, canonical realtime presentation, the
  one-minute lifecycle, one cliff proof, and the direct/menu MP floor pair.
- `Full`: all registered verifiers.

`P1Gate` does not change Boundary, Regression, or Full profile membership, and
a pass is neither P1 completion nor the required five-minute soak. The legacy
harness fleet remains available for diagnosis; this additive profile does not
delete or graduate its unique assertions.

Use `docs/VERIFYING.md` for when to run each profile.

## Generator Direction

Manual copy/paste is now a risk. The next tooling improvement should be a
harness-pair generator that updates the registry, Makefile mapping, mode define,
wrapper scripts, docs index, and proof-template text in one controlled path.
Until then, registry checks are the required guardrail.
