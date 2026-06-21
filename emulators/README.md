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

## no$gba

Place the no$gba debugger executable here:

```text
emulators/nogba/NO$GBA.EXE
```

Use `scripts/debug-nogba.ps1` for interactive hardware/debugger work. no$gba is
not an automated verifier in this repo yet; melonDS remains the source of the
GDB-driven runtime checks.
