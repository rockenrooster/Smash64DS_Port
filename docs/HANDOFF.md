# Handoff

Current: 2026-09-04 — **`smash64ds.nds` now SHIPS the landed content and the
owner has playtested it.** Nine fighters (rung 7 of `NDS_P2_SHELL_ROSTER`),
all eight opt-in stages, the item core they derive, and the VS Options and
Item Switch screens. **`docs/BUGS.md` is the driving queue** and
`docs/BUG_FIXING_PROCESS.md` governs it: every row carries a status, none is
marked FIXED yet.

**Boundary 2026-09-04, both arms GREEN on the shipped nine-fighter/eight-stage
config:** `p2_shell_loop` (free floor 72,148 B), `p2_battle_realtime`
(frames=212, green 46.6%). `p2_fourcpu_stress` is LIVE, not parked: WORK P95
2,697,209 (2.41x), items were OFF (fixed) — board P2-2. `-List` rules.
**Owner (2026-09-04): no emulator runs until the code is complete.**

## Next

1. **Work `docs/BUGS.md`.** Owner playtest rows, biggest first. Two causes are
   fixed and need his eye: all eight backgrounds (the battle wallpaper cache
   keyed on Dream Land's asset id) and Link's CSS preview (he was the one
   landed fighter missing from the per-kind filter). Probes are out on Sector
   Z's geometry and BGM, and which of Peach's/Hyrule's wallpaper rows is
   wrong. Congo barrel law/offsets/RNG match decomp verbatim (HANDOFF 09-04).
2. **RAM is the binding P2 constraint**, and the plan changed: see
   `docs/p2/P2-2-four-fighters.md` and `docs/reviews/Design_DS_fighter_paging.md`.
   Runtime paging is REFUSED (reloc files hold relocated absolute pointers).
   Direction is an offline-generated match-resident pack; first experiment is
   Kirby's 120,864-byte raw model member. Target 512 KiB net headroom.
3. **P2-3f47** — Kirby halts in `ndsSyMallocOverflowHalt` (his setup leaves
   under 115,440 for Fox). Jigglypuff is now a RE-EVALUATION candidate: both
   his defects closed 2026-09-03 and he presents 76 frames. Ness is unproven,
   and five of his effect descriptors are absent from `NDS_EF_ROSTER_DESCS`.
4. **P2-6** ladder tables build behind `NDS_P2_1P_GAME`; the stage-clear bonus
   table waits on 58 `llSC1PStageClear*` manifest rows.

Held: Congo Jungle and Sector Z music (loop starts near the track midpoint, a
doubled decode). Owner decision owed: `lbRelocGetForceExternHeapFile` returns a
raw heap pointer on a miss instead of failing closed.

## Delegation

OpenCode is the active skill. `opencode run --agent swarm-build|swarm-probe
--variant Xhigh --auto`; permissions are tool-enforced, so prompts carry scope,
not rules. Build agents write only their own new files and REPORT
Makefile/header deltas. Redirect every agent's output to its own file.
Verify every stub/absence claim against the linked ELF with `nm`.

## Context discipline

Restart reads this file + `docs/P2_EXECUTION_BOARD.md` only. CodeGraph first,
then bounded reads of the returned seams. `PORTING.md`, `PERF_LEDGER.md`,
`VERIFYING.md`, `KNOWN_ISSUES.md` and the phase plans are lookup-only.
Bank verbose output to files; bring back status plus the failure window. One
build at a time; never pass `-j` or override `MAKEFLAGS`, and run a plain
`make` before `verify-all.ps1` if the last build used lab flags — the
generators write shared untracked packs and the verifier tests the default
configuration. Owner directives:
**no snapshot**; no new worktrees.

Start of cycle: `scripts/verify-all.ps1 -Profile Boundary -List`, `git status --short`.
