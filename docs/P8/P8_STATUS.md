# P8 status -- stability, RAM, CPU and readability

Live state for [the P8 plan](P8_PLAN.md). One row per item; the cursor is the
first row that is not done.

## Cursor

**Next action:** build the S1 ROMs on HEAD with `scripts/tools/` (walk, soak,
wireless lab, gate) and start S1 in the background slots; Phase C goes on in
the foreground between builds (C3: `nds_renderer_textures_effects.c`).

**Before the first edit:** when this plan was written (2026-10-22 08:20) a
session was running the owner's goal "finish Todo list: readability, RAM
hunting, crash hunting": it committed `7f0dca28600` at 08:18, had a `make`
running, a Phase C2 plan ready for `reloc_backend_assets.c` and its own map
census (`map_census.py`, memory `ram-hunt-census-2026-10-22`). One integrator
only: this goal continues that work, it does not run beside it. Check for a
running build and that session's uncommitted work first. The worktree started
with 56 dirty paths (owner docs, test scripts, a few sources): leave them,
commit with explicit paths only.

## Baseline (2026-10-22)

| what | identity | numbers |
|---|---|---|
| HEAD | `7f0dca28600` | readability A, B-1..B-4b and C's first split landed |
| ship play ROM | `builds/play-p7z` | `.main` 1,865,184; `.main.rw` 175,580; `.main.bss` 875,208 (image 2,915,972); overlays 423,040; ITCM 32,728 / 32,768; DTCM 12,244 / 16,384. Battle-resident: R0 computes it |
| gate ROM, worst-case search | `build-sweep-s56` | cast <= 1,250,000 / 95% on 16 of 16; heavy roster over 1,250,000 / 90% on 11 of 16 (Hyrule Temple 1,609,344, 80.7%); mixes 24 of 64 over; lowest heap 25,604 |
| last gate ROM | `build-sweep-s61` | digests identical to `s60` on three configs |
| stability | `docs/STABILITY_AUDIT.md`, 2026-10-20 | every mode on every fighter, 0 halts (before ~47,000 lines of A/B deletions) |

## Items

| item | state | evidence / next |
|---|---|---|
| Q1-Q3 owner decisions (plan section 7) | answered 2026-10-22 | Q1 yes (AGENTS.md and HANDOFF.md route here); Q2 yes (R10 if R1-R9 leave the heavy mixes short); Q3 P7 only as bugs |
| H-1 tools S1 and the gate need, into `scripts/tools/` | done | 22 files, `scripts/tools/README.md`: play/walk/gate/soak builds, the probe session and halt probes, the six audit drivers, the wireless lag driver, the worst-case sweep, the heap census, the boot check; no session paths (`common.ps1`); `boot_check.ps1` boots `play-ram1` clean |
| S1 regression net (full matrix) on HEAD | open | ROMs: walk, soak, wireless lab, ship layout |
| C split the 13 port files over 4,000 lines | in progress | `nds_renderer_native_common.c` (`7f0dca28600`) and `reloc_backend_assets.c` (C2) done; next `nds_renderer_textures_effects.c` |
| S3 static analysis (`-fanalyzer`, allocations, stacks) | open | foreground work while S1 runs |
| D purpose headers and invariant comments | open | after C |
| E dormant verifier framework, legacy fighter executor | open | owner OK (`OWNER_QUESTIONS.md` 6) |
| F history names | open | |
| G Makefile cleanup and split | open | proof: `make -n` identity |
| H-2 the rest of the tools, `quick_check.ps1` | open | |
| I docs routing, P7 close-out, archive | in progress | `AGENTS.md` and `HANDOFF.md` route to P8 (2026-10-22); next `docs/README.md`'s table (dirty: merge) and `P7_STATUS.md`'s rows against their evidence |
| S2 fault injection (squeeze, slow storage, long soak, laps, fuzz) | open | lab flags only |
| S4 stalls (load ledger, storage waits, menu spikes) | open | |
| S5 freeze report gaps | open | menus, loads, before the first frame; identity fields |
| S6 open bug rows | open | Yoshi-win Results freeze, Master Hand stiff, FD backgrounds, Palutena's Results dress, wireless start on a slow card |
| R0 RAM ledger and buffer peaks | open | after D (owner order) |
| R1-R9 RAM levers | open | ranked in plan section 4; R4's second printf engine already gone (`41636f74b0f`, 7 KB) |
| R10 per-fighter code overlays | allowed (owner Q2) | only if R1-R9 leave the heavy mixes short |
| P0 CPU ledger (band vs median census) | open | profiling only: may run in the background before D |
| P1-P11 CPU levers | open | ranked in plan section 5; after D, alternate with R by gain over risk |
| P12 lower update rates | gated | owner's yes, only after P1-P11 |
| Spends (R's bytes into stage programs, lean spares, the motion arena) | open | Hyrule Temple's whole program with the heavy roster and the mixes first |
| Final: play ROM, S1 matrix, worst-case search on the final HEAD | open | |

## Log

Newest first. One line a batch: date, item, commit, proof, numbers.

- 2026-10-22 -- H-1: the S1 and gate tools in `scripts/tools/` (22 files, README); parse-clean,
  `boot_check.ps1` on `play-ram1`: scene 1, 0 storage failures.
- 2026-10-22 -- C2: `reloc_backend_assets.c` (18,246 lines) into 8 slices of the
  `scene_backend` unit (1,269 to 3,545 lines); `scene_backend.o` preprocess-identical in the
  five configs; the 14 affected test files fail exactly as on HEAD (18 pre-existing), the
  four script tests pass; one stale Phase A check fixed (`test_results_demo_submotion_routes.py`).
- 2026-10-22 -- R4 part: newlib's stream printf engine out of the link (`41636f74b0f`),
  `.main` -7,168 B on `play-ram1`; boots clean.

- 2026-10-22 -- owner: CPU hunting beside the RAM hunting (plan section 5, loop P).
- 2026-10-22 -- owner answers Q1-Q3; I: `AGENTS.md` and `docs/HANDOFF.md` route to P8.
