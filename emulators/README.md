# Emulator Layout

Local emulator binaries and generated configs live here to keep the repo root
clean. Binaries are intentionally ignored by Git.

## melonDS

Place or keep melonDS here:

```text
emulators/melonds/melonDS.exe
```

The melonDS scripts use `emulators/melonds/melonDS.toml` and write stdout/stderr
logs under `artifacts/emulator-logs/`.

This copy is the single source of truth for the whole repo: no system install, no
PATH lookup, and no package-manager build is ever used. `Resolve-MelonDSPath`
rejects any executable outside this directory.

Parallel automation runs from per-slot copies in
`emulators/melonds-runners/slotN/melonDS.exe`. They are copies, not links, so
**replacing `emulators/melonds/melonDS.exe` does not update them.** Refresh after
every swap:

```powershell
.\scripts\New-MelonDSRunnerSlots.ps1 -Count 9 -Force
```

`scripts/check-melonds-policy.ps1` (part of `verify-dev-fast.ps1`) hashes every
slot against the source and fails on drift.

## no$gba

Place the no$gba debugger executable here:

```text
emulators/nogba/NO$GBA.EXE
```

Use `scripts/debug-nogba.ps1` for interactive hardware/debugger work. no$gba is
not an automated verifier in this repo yet; melonDS remains the source of the
GDB-driven runtime checks.
