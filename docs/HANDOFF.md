# Handoff

Current: 2026-09-05 — P2 content work, code-first (nothing built since 09-04,
when `smash64ds.nds` last shipped nine fighters and eight stages and the owner
playtested it). **`docs/BUGS.md` is the driving queue**, governed by
`docs/BUG_FIXING_PROCESS.md`; no row is marked FIXED yet.

**Boundary 2026-09-04, both arms GREEN on the shipped nine-fighter/eight-stage
config:** `p2_shell_loop` (free floor 72,148 B), `p2_battle_realtime`
(frames=212, green 46.6%). `p2_fourcpu_stress` is LIVE, not parked: WORK P95
2,697,209 (2.41x), items were OFF (fixed) — board P2-2. `-List` rules.
**Owner (2026-09-04): no ROM builds/emulator verification until P2 code is 99-100% complete. Source audits continue; commit regularly, push confirmed progress.**

## Next

1. **Land what the in-flight agents return** (see Delegation): the scratchpad logs
   are their reports; verify each claim with the named checker before committing.
2. **RAM is the binding P2 constraint** (P2-3f47, Kirby's halt); direction is an
   offline match-resident pack, runtime paging REFUSED
   (`docs/p2/P2-2-four-fighters.md`, `docs/reviews/Design_DS_fighter_paging.md`;
   Kirby's copy set is carried, hats are a modelpart swap: `p2/fighters/kirby.md`).
3. **The twelve polygons' 23 reloc files are staged nowhere**, so the bridge's
   polygon refusal is correct until they are (ids and rule in
   `docs/p2/P2-3-fighter-production.md`); Master Hand's owner export is the other
   half. Six stage actors still lack a render path; the barrel's slot is the pattern.
4. **The final verification pass gates all of it** (`docs/VERIFYING.md` 4-4e); flip `NDS_P2_1P_GAME ?= 1` only there.

Landed 09-05 (117 commits, pushed; the board carries the detail): stage packets
load from NitroFS as blobs, the 25 bonus boards are registered and admitted,
`ef/efground.c` gives every stage its background actors, all 47 music tracks are
pinned, Screen Adjust and Master Hand are in, and five requested-vs-provided
censuses became `--strict` checkers (reloc symbols, scene kinds, functions,
build flags, audio cues).

Owner decisions owed: `lbRelocGetForceExternHeapFile` raw pointer on a miss; root
P1 ROM is 21.8 MB since 09-04 (pin 12.5 MB); build.ps1 targets `smash64ds`, no P2 pin.

## Delegation

Owner re-authorised agents 09-04: 5 concurrent opencode (4 Muse, 1 GLM via
`-m zai-coding-plan/glm-5.3`) plus Opus subagents; one prompt file and log each.
**In flight at handoff**,
read the logs before touching their files: `bosspins` (Master Hand owner export;
holds both `generate_nds_native_owner*` generators), `fncensus`, the read-only
audits `audit1p`/`auditmodes`/`auditstages`, and three Opus subagents (bonus-scene
reachability, menu art coverage, audio cues -- the last holds
`scripts/sfx/check_audio_cue_census.py` and `docs/VERIFYING.md`). The polygon
staging brief is ready at `scratchpad/prompts/polystage.txt`.

## Context discipline

Restart reads this file + `docs/P2_EXECUTION_BOARD.md` only. CodeGraph first,
then bounded reads of the returned seams. `PORTING.md`, `PERF_LEDGER.md`,
`VERIFYING.md`, `KNOWN_ISSUES.md` and the phase plans are lookup-only.
Bank verbose output; scope git diffs to changed paths. One build at a time; never
pass `-j` or override `MAKEFLAGS`; run a plain `make` before `verify-all.ps1`
if the last build used lab flags. Owner directives: **no snapshot**, no new
worktrees.

Start of cycle: `scripts/verify-all.ps1 -Profile Boundary -List`, `git status --short`.
