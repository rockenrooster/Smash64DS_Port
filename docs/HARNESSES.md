# Harnesses

```text
HARNESS_INDEX_SOURCE: scripts/lib/harness-registry.ps1
```

The registry is the authority. Do not hand-maintain a second harness list.

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
.\scripts\verify-all.ps1 -Profile Latest -List
.\scripts\check-harness-registry.ps1
```

Only Latest and Boundary remain. Use an unregistered focused checker directly
when it covers a narrower risk.

## What “Mode” Means

- Harness mode `163` selects the canonical `battle_playable_realtime` scene.
- Renderer modes are internal implementation selectors: generic control `0`,
  AOT fighters `8`, and complete-stage owner `9`.
- Harness modes `1`-`162` are retired and absent from the executable fleet.

## Naming

- `check-*`: host/static invariant.
- `verify-*`: executable ROM/runtime check.
- `battle_playable_*`: scene-level natural-runtime capability.
- `*-lab`: non-published experiment under `builds/`.

Add a harness only for a new scene-level capability. Otherwise extend mode 163,
an existing focused checker, or natural Boundary coverage. Registry edits must
pass `check-harness-registry.ps1`.
