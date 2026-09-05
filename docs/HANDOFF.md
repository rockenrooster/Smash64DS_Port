# Handoff

Current: 2026-09-05 — P2 content work, code-first (nothing built since 09-04,
when `smash64ds.nds` last shipped nine fighters and eight stages and the owner
playtested it). **`docs/BUGS.md` is the driving queue**, governed by
`docs/BUG_FIXING_PROCESS.md`; no row is marked FIXED yet.

**Boundary 2026-09-04, both arms GREEN on the shipped nine-fighter/eight-stage
config:** `p2_shell_loop` (free floor 72,148 B), `p2_battle_realtime`
(frames=212, green 46.6%). `p2_fourcpu_stress` is LIVE, not parked: WORK P95
2,697,209 (2.41x), items were OFF (fixed) — board P2-2. `-List` rules.
**Owner (2026-09-05): complete P2; periodically build `smash64ds.nds`, commit regularly and push confirmed progress. No snapshots. This supersedes the earlier build pause.**

## Next

1. **Land what the in-flight agents return** (Delegation): their scratchpad logs are the reports; verify each claim with the named checker before committing.
2. **RAM is the binding P2 constraint** (P2-3f47, Kirby's halt): an offline
   match-resident pack, runtime paging REFUSED (`p2/P2-2-four-fighters.md`,
   `reviews/Design_DS_fighter_paging.md`; his copy set is carried and his hats are
   a modelpart swap, `p2/fighters/kirby.md`).
3. **The twelve polygons' 23 reloc files are staged nowhere**, so the bridge's
   polygon refusal is correct until they are (ids in `p2/P2-3-fighter-production.md`);
   Master Hand's owner export is the other half. Six stage actors still lack a
   render path; the Jungle barrel's actor slot is the pattern.
4. **Hammer/Star arbitration is source-correct in code**: the two empty port
   functions now follow BattleShip `ft/ftparam.c:93-155`. Host execution matches
   162,732 source cases, including four fighters and Star warning expiry;
   the audio census is green. ROM playback acceptance remains pending.
5. **The final verification pass gates all of it** (`docs/VERIFYING.md` 4-4e); flip `NDS_P2_1P_GAME ?= 1` only there.

Landed 09-05 (119 commits, pushed; the board carries the detail): stage packets load
from NitroFS as blobs, the 25 bonus boards are registered and admitted, `efground.c`
gives every stage its background actors, all 47 music tracks are pinned, Screen Adjust
and Master Hand are in, six censuses became `--strict` checkers.

Owner decisions owed: `lbRelocGetForceExternHeapFile` raw pointer on a miss; the root P1 ROM is 21.8 MB since 09-04 against a 12.5 MB pin; build.ps1 targets `smash64ds` and there is no P2 output pin.

## Delegation

Owner now requests 4 Muse 1.3 Contributor + 1 GLM 5.3 (`zai-coding-plan`, max)
and 3 native Codex GPT-6 Astra workers. Current scopes: polygon asset staging,
menu audit completion, inherited item review, Master Hand export review,
the inherited function census, campaign entry, memory estimator and real barrel
packet execution. Main owns item music, integration and serialized builds.
New prompts/logs: `builds/resume-20260905/`; older reports are in the Claude
session's external temporary `scratchpad`, not a repo directory. The GLM
CLI uses `swarm-probe -m zai-coding-plan/glm-5.3 --variant max`: its `glm-probe`
definition is subagent-only and otherwise silently falls back to default.

## Context discipline

Restart reads this file + `docs/P2_EXECUTION_BOARD.md` only. CodeGraph first, then
bounded reads of the returned seams. `PORTING.md`, `PERF_LEDGER.md`, `VERIFYING.md`,
`KNOWN_ISSUES.md` and the phase plans are lookup-only.
Bank verbose output; scope git diffs to changed paths. One build at a time; never
pass `-j` or override `MAKEFLAGS`; run a plain `make` before `verify-all.ps1`
if the last build used lab flags. Owner directives: **no snapshot**, no new
worktrees.

Start of cycle: `scripts/verify-all.ps1 -Profile Boundary -List`, `git status --short`.
