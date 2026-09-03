# Handoff

Current: 2026-09-03 — **the ten-fighter ROM reaches gameplay for the first
time** (1,381 presented frames, Kirby vs Fox, was 0). P2-4 is open.

Published `smash64ds.nds` rebuilds green; the board owns its hash.

Landed: P2-3f50 and P2-3f51 closed (board); the pack reservation keeps the
match's fighter cost free, unblocking the ten-flag ROM; dense normals into the
NitroFS owner images (−21,928 B); Yoshi's Island gameplay half behind
`NDS_P2_STAGE_YOSTER`.

Instruments: the arena halt and the fixup recorder flush the data cache and
publish their caller's return address; an unflushed witness reads back as zero
("never happened") because the GDB stub reads behind the data cache.

**Boundary 2026-09-03:** `p2_shell_loop` PASS, `p2_battle_realtime` PASS,
`p2_fourcpu_stress` FAILS on the tick-HUD sampler's 3,600 s ceiling — the
parked P2-2p8 cost, measured, not a regression. Board row has the evidence.

## Next

1. **P2-3f49** is unblocked, not closed: the ten-flag ROM runs with *no*
   animation cache (`gNdsR2AnimCacheArenaReservedBytes` 0), so it streams every
   animation. Static levers bring it back — Mario/Fox owner tables ~64,147 B (a
   frozen combined export, not generator output — that is the work),
   prepared-dense to slot scratch ~65,784 B.
2. **P2-4s1**: Yoster needs its native stage packet (law 8), stage-select art,
   music, asset rows.
3. **P2-3f48**: 3,392 B, and the prerequisite for *all* of P2-5 — every common,
   monster and stage item's art is in that one file. Then Kirby copy and Link
   acceptance; the four-CPU arm needs P2-2p8 un-parked to complete at all.

Owner decision owed: `lbRelocGetForceExternHeapFile:10299` still hands back a
raw heap pointer on a miss instead of failing closed — what made P2-3f51
silent for a whole roster admission.

## Delegation

opencode's paid route hit its monthly cap; the free tier rate-limits above
about three concurrent. Both stall **silently** — live process, no output, no
error. Tell: `~/.local/share/opencode/log/opencode.log`. Agent files need
`mode: all`.

## Active gates

`scripts/verify-all.ps1 -Profile Boundary -List` is the membership authority:
`p2_shell_loop`, `p2_battle_realtime` (mode 163), `p2_fourcpu_stress`.
P1 stays frozen. Owner directive: **no snapshot**; no new worktrees.

## Context discipline

Restart reads this file + `docs/P2_EXECUTION_BOARD.md` only. CodeGraph first,
then bounded reads of the returned seams. `PORTING.md`, `PERF_LEDGER.md`,
`VERIFYING.md`, `KNOWN_ISSUES.md` and the fighter/stage plans are lookup-only.
Bank verbose output to files and bring back status plus the failure window.
One build at a time; never pass `-j` or override `MAKEFLAGS`.

Start of cycle: `scripts/verify-all.ps1 -Profile Boundary -List`, `git status --short`.
