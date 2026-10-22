# scripts/tools -- the live build, probe and audit tools

The tools the P8 loops run (`docs/P8/P8_PLAN.md`), moved here from session
scratchpads so a temp cleanup cannot delete them. PowerShell 7 (`pwsh`); run
builds from PowerShell, not Git Bash (the msys2 `make` path differs).

Every tool dot-sources [`common.ps1`](common.ps1): the repository root, the
devkitPro paths, the ship-layout flag sets, the P4 content -> parent kind
table, and where runs write. Conventions:

- `-Build` names a directory under `builds/` (the ROM and ELF inside it);
  `-Seed` copies an earlier build directory first (mtimes kept) so the first
  build is incremental.
- `-Slot` is a melonDS runner slot (`emulators/melonds-runners/slotN`): one
  probe per slot, start runs at least 30 s apart, and never stop melonDS by
  name (the owner's own `melonDS_3.exe` matches): the tools stop their own
  process or match the slot's path.
- `-Tag` names a run: its gdb command file and log are
  `builds/probes/<Tag>.gdb` / `<Tag>.log` (lines starting `X `/`x ` are the
  probe's records), captures go to `artifacts/visibility/probes/<Tag>/`, and
  audit summaries to `builds/probes/audit-*.txt`.
- Builds are serialised on one named mutex (the lab linker script and UI-kit
  outputs are shared); never edit sources while one runs.

## Builds

| tool | builds |
|---|---|
| `build_play_rom.ps1 -Build play-pNN` | the owner's play ROM: free play with every content and P5 (never the root ROM) |
| `build_walk_rom.ps1 -Build walk-uNN` | the play ROM plus the menu walk (`NDS_P2_MENU_WALK=1`), for the audits |
| `build_gate_rom.ps1 -Build build-sweep-sNN` | THE gate ROM: four-CPU sweep target in the ship layout, with the lab ITCM evictions |
| `build_soak_rom.ps1 -Build build-soak-sNN [-Minutes 5]` | the gate ROM with timed matches, for the soaks |
| `build_sweep_rom.ps1` | the bare four-CPU sweep ROM the gate builds on (not a gate by itself) |

## Probes

| tool | does |
|---|---|
| `boot_check.ps1 -Build` | boots a play/walk ROM: scene reached, storage counters (fail must be 0) |
| `probe_session.ps1` | one gdb session for a whole run: pokes at main, the caller's breakpoints, optional captures |
| `probe_halts.ps1` | halt paths (OOM, aborts, undefined instruction) with a backtrace, until a given scene |
| `probe_campaign.ps1` | one 1P campaign run through the native 1P select (per-scene heap low-water, native failures) |
| `probe_bonus.ps1` | one bonus practice run through the native bonus select |
| `capture_battle.ps1` | four-CPU forced-move schedule with frame captures and gdb lines per frame |
| `capture_slot.ps1` | captures a runner slot's window (from a gdb breakpoint's `shell`) |

## Audits (the S1 matrix, `docs/STABILITY_AUDIT.md`)

| tool | on | covers |
|---|---|---|
| `audit_campaign.ps1` | walk ROM | the 1P campaign for every fighter (12 + Metal Mario + 19 contents) |
| `audit_bonus.ps1` | walk ROM | both bonus practices for every fighter |
| `audit_training.ps1` | walk ROM | Training for every fighter, a stage each, the next fighter as the CPU |
| `audit_vswalk.ps1` | walk ROM | the VS select tour (3D previews), SSS, match, Results, back, `-Laps` times |
| `audit_metal_mario.ps1` | walk ROM | Metal Mario's campaign and both practices |
| `audit_soak.ps1` | soak ROM | four-player all-items matches m1-m8, every fighter once |
| `wireless_lag.ps1 -Build` | wireless lab ROM | two linked consoles (melonDS-mp): scene loads, lockstep gates, desyncs, stalls |

## Measurements

| tool | measures |
|---|---|
| `worst_case_sweep.ps1` | WORK P50/P95 and the two-VBlank share per stage and roster on a gate ROM; the CSV rows are the replay digest (`scripts/compare-replay-digest.py a.csv b.csv`) |
| `heap_census.ps1` | one match's heap allocations, ensured files, stage program load, heap low-water, motion cache and lent spans |
