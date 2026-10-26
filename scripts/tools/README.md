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

## Checks

| tool | does |
|---|---|
| `quick_check.ps1 [-Reference TAG] [-Skip host,configs,digests]` | the checks a change passes, in one command: the host tests, the five configs built incrementally into `builds/qc-*` (play = the ship link, walk, gate, profiling gate, wireless lab; each seeded once from the newest build of its kind), and the heavy roster's replay digests on Hyrule Temple, Congo Jungle and Yoshi's Island against a reference gate sweep (`-Reference s82v` = `artifacts/performance/p8-sweep/s82v-<slot>-...csv`; `compare-replay-digest.py --ticks 16`: the same frames, or the same tick stream when a new ROM image only moved presentation timing). Summary: `builds/probes/quick-check.txt`; exit 0 only when every step that ran passed |
| `run_host_tests.py OUT [--workers N]` | every tracked `scripts/**/test_*.py` under pytest, one process a file, N at a time: a line a file and a TOTAL line; exit 0 only with no failure, error or timeout |

## Builds

| tool | builds |
|---|---|
| `build_play_rom.ps1 -Build play-pNN` | the owner's play ROM: free play with every content and P5 (never the root ROM) |
| `build_walk_rom.ps1 -Build walk-uNN` | the play ROM plus the menu walk (`NDS_MENU_WALK=1`), for the audits |
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
| `load_ledger.ps1 -Build [-Seconds] [-Frames]` | S4's load ledger on a walk ROM: per scene change, the VBlanks from the old scene's exit hold to the new scene's first present; `-Frames` adds the VBlanks between frames per scene (menu spikes: frames past two VBlanks; a gdb stop a frame, slow) |
| `profile_set.ps1 -Build -Tag [-Set cast|heavy|mixes|all] [-NoSplit]` | P0's profile census on a profiling gate ROM (`-Extra NDS_CPU_PROFILE=1,NDS_CPU_PROFILE_START=200,NDS_CPU_PROFILE_FRAMES=384`): per-function cycles for the cast, the heavy roster and the mixes, one runner slot a run; `-NoSplit` when the per-frame regions do not fit DTCM |
| `vram_census.ps1 -Build [-Gkinds] [-Kinds] [-Content]` | texture VRAM over a four-CPU match on a gate ROM built with `-Extra NDS_DIAG_COUNTERS=1`: free bytes and the largest free run at GO, live and at their minimum, peak texel and palette bytes (loop V) |
| `ram_ledger.py MAP [--diff OLD_MAP] [--section NAME]` | a build's RAM from its link map: the image, VS battle, 1P battle and menu totals, the TCMs, the image by object and by family; `--diff` the change from an older build, largest first (R0) |

## Readability (Phase C)

| tool | does |
|---|---|
| `outline.py FILE [--all]` | every top-level definition with its size and preprocessor depth, and runs of one prefix: where a file's topics change |
| `split_slices.py [--check] PLAN.json` | cuts a file into topic slices at anchors (brace and preprocessor depth 0, moving up over each anchor's comment) and includes them in order: right after the file in its unity parent, or at the file's end when the plan's parent is the file itself |
| `split_header.py [--check] PLAN.json` | the same for a header: topic headers it includes in place, before its guard's #endif; the proof is every translation unit of a full `make -n -B` listing |
| `split_function.py [--check] PLAN.json` | moves phases of one oversized function body into `.inc` fragments the function includes in place (each fragment between two anchors, at one brace and preprocessor depth, closing all it opens); register them in `source_test_helpers.INLINE_FRAGMENTS` so `source()` reads them where they stand |
| `repoint_split_readers.py FILE_NAME READER.py...` | after a split, points each script that reads the file's text at `source_test_helpers.source()`, which reads it whole (slices in include order, fragments in place) |
| `pp_run.py LISTING OUT_DIR` | preprocesses every compile of a `make -n -B` listing (`-E -P`) into OUT_DIR; a split's proof is the same text before and after |
| `pp_cmp.py BEFORE AFTER` | compares two `pp_run.py` trees: exact, blank-lines-only, or different |
| `obj_cmp.py LISTING BEFORE AFTER WORK` | compiles the units two `pp_run.py` trees differ in, with each unit's flags from the `make -n` listing, and compares their non-debug sections byte for byte (readelf; seconds an object): when preprocessing differs, the code still must not |

| `make_dry_identity.ps1 -Out DIR [-Compare DIR]` | `make -n -B` for the eight configurations (play, walk, wireless, the root make, gate, profiling gate, soak, VRAM lab) into DIR, and with `-Compare` each listing against an earlier run's: the proof that a Makefile change (the P8 G split) moved text without changing one command |

A split also registers in `scripts/menus/source_test_helpers.py` `SPLIT_SOURCES`,
so host tests read the file whole through `source()`.

The Makefile is the root `Makefile` and the topic fragments in `mk/` it
includes in place. A script that reads the Makefile's text reads
`scripts/makefile_parts.py` `makefile_text()` (every fragment where its
include stands); one that edits it writes the part that holds its anchor
(`part_containing()`).
