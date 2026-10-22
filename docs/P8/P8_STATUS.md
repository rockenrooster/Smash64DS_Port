# P8 status -- stability, RAM, CPU and readability

Live state for [the P8 plan](P8_PLAN.md). One row per item; the cursor is the
first row that is not done.

## Cursor

**Next action:** H-1 -- copy the tools S1 and the gate need into
`scripts/tools/` (gate and play ROM builds, the walk/soak/wireless drivers,
the worst-case sweep, the digest compare, the heap census) with a README; then
build the S1 ROMs on HEAD and start S1 in the background slots while Phase C
goes on in the foreground.

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
| H-1 tools S1 and the gate need, into `scripts/tools/` | next | sources: session scratchpads `31e5c78d-...` and `4aa2b87d-...` under `%TEMP%\claude\D--Stuff-DevFolder-Smash64DS-Port\` |
| S1 regression net (full matrix) on HEAD | open | ROMs: walk, soak, wireless lab, ship layout |
| C split the 13 port files over 4,000 lines | in progress | `nds_renderer_native_common.c` done (`7f0dca28600`); next `reloc_backend_assets.c` |
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

- 2026-10-22 -- owner: CPU hunting beside the RAM hunting (plan section 5, loop P).
- 2026-10-22 -- owner answers Q1-Q3; I: `AGENTS.md` and `docs/HANDOFF.md` route to P8.
