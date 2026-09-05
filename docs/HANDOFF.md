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
**Owner (2026-09-04): no ROM builds/emulator verification until P2 code is 99-100% complete. Source audits continue; commit regularly, push confirmed progress.**

## Next

1. **P2-4n1: fourteen stages runtime-wired, unbuilt** (2026-09-05): the eight
   VS stages and the five 1P arenas pass the checker, the emitter `--check`
   and the DLLink tests; the 25 bonus boards are in flight. Next: native
   actors, then compile + admission at the final pass.
2. **RAM is the binding P2 constraint**, and the plan changed: see
   `docs/p2/P2-2-four-fighters.md` and `docs/reviews/Design_DS_fighter_paging.md`.
   Runtime paging is REFUSED (reloc files hold relocated absolute pointers).
   Direction is an offline-generated match-resident pack; first experiment is
   Kirby's 120,864-byte raw model member. Target 512 KiB net headroom.
3. **P2-3f47** — Kirby halts in `ndsSyMallocOverflowHalt` (his setup leaves
   under 115,440 for Fox). Jigglypuff is now a RE-EVALUATION candidate: both
   his defects closed 2026-09-03 and he presents 76 frames. Ness is unproven
   (his five effect descriptors are in `NDS_EF_ROSTER_DESCS` since 09-04).
4. **P2-6/P2-7 behind `NDS_P2_1P_GAME` (unbuilt, 2026-09-05):** every scene
   is in source and the bridge boots the fight task; the 1P start compiles
   from the ninth overlay patch (signature check out). Collision loads from
   the source's 41-row table; five arena maps staged (o2r wallpaper names
   mislabelled: row by ROM id); bonus reloc rows staged; polygons admitted. Out:
   polygon owner export, boss admission, bonus boards, barrel actor path, packet-load probe. Census: 0.

Held: Congo Jungle and Sector Z music (loop starts near the track midpoint, a
doubled decode). Owner decision owed: `lbRelocGetForceExternHeapFile` returns a
raw heap pointer on a miss instead of failing closed.

## Delegation

Owner re-authorised agents 2026-09-04 evening: 5 concurrent (4 Muse, 1 GLM via
`-m zai-coding-plan/glm-5.3`); launch from prompt files, one per log. Codex's
partial logs: `builds/resume-20260904/`. Verify absence claims against the ELF.

## Context discipline

Restart reads this file + `docs/P2_EXECUTION_BOARD.md` only. CodeGraph first,
then bounded reads of the returned seams. `PORTING.md`, `PERF_LEDGER.md`,
`VERIFYING.md`, `KNOWN_ISSUES.md` and the phase plans are lookup-only.
Bank verbose output; scope git diffs to changed paths (full-tree reads normalize telemetry). One
build at a time; never pass `-j` or override `MAKEFLAGS`, and run a plain
`make` before `verify-all.ps1` if the last build used lab flags — the
generators write shared untracked packs and the verifier tests the default
configuration. Owner directives:
**no snapshot**; no new worktrees.

Start of cycle: `scripts/verify-all.ps1 -Profile Boundary -List`, `git status --short`.
