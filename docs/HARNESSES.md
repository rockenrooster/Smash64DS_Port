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
  **That is the SCENE name, and it is not the registry ENTRY name.** The
  registry's `Harness` field names the scene the header and Makefile agree on
  (`nds_scene_harness.h`); its `Name` field names the verifier row. Since P2-1M
  the row that runs mode 163 is called `p2_battle_realtime`, because it reaches
  that scene through the VS shell rather than booting into it — the scene, its
  header constant and its Makefile mapping are all unchanged.
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
