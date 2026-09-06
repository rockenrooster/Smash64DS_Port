# Handoff

Current: 2026-09-06 — root `smash64ds.nds` rebuilt and ready for testing.
It passes startup and the Options route; exact identity/evidence are on the board.
Real-items four-CPU execution now clears RAM; cache/performance and campaign CSS remain open. The board is
the dynamic queue; `docs/BUGS.md` retains reproductions and unresolved defects.

**Boundary 2026-09-04, both arms GREEN on the shipped nine-fighter/eight-stage
config:** `p2_shell_loop` (free floor 72,148 B), `p2_battle_realtime`
(frames=212, green 46.6%). `p2_fourcpu_stress` is LIVE, not parked: WORK P95
2,697,209 (2.41x), items were OFF (fixed) — board P2-2. `-List` rules.
**Owner (2026-09-05): complete P2; periodically build `smash64ds.nds`, commit regularly and push confirmed progress. No snapshots. This supersedes the earlier build pause.**

## Next

1. **Real-items four-CPU baseline is live; RAM floor passes.** The 1,972-sample window spawns two items and retains 31,988 B, with no allocator/panic/normalization failures. Cache engagement remains zero and P95 is 2,808,768. Evidence and exact identity are on the board. Commits through `21420ebd843` are pushed. Do not call this full P2 acceptance.
2. **RAM is the binding P2 constraint** (CSS + battle + P2-3f47): an offline
   match-resident pack, runtime paging REFUSED (`p2/P2-2-four-fighters.md`,
   `reviews/Design_DS_fighter_paging.md`; his copy set is carried and his hats are
   a modelpart swap, `p2/fighters/kirby.md`).
3. **Polygons and Master Hand link in the campaign lab**; campaign forces
   donor flags in both recursive and outer make. Gameplay acceptance is open.
   Barrel native draw is committed. Yoster cloud kind-48 camera transforms and
   Hyrule's full particle bank are integrated; visual acceptance remains.
4. **Hammer/Star arbitration is source-correct in code**: the two empty port
   functions now follow BattleShip `ft/ftparam.c:93-155`. Host execution matches
   162,732 source cases, including four fighters and Star warning expiry;
   the audio census is green. ROM playback acceptance remains pending.
5. Menu/cache checkpoint pushed through `f33c5aa039f`; normal human-input ROM published. Startup, Options route/visuals and 23 menu/blitter host cases pass. Results cache fix recovers 258,048 B; full shell lap/rematch passes with 76,580 B floor. Battle arm passes too. Compact preview normalization/native-root binding remains next; source handle/particle setup is being checked before integration.

Owner decisions owed: `lbRelocGetForceExternHeapFile` raw pointer on a miss; the root P1 ROM is 21.8 MB since 09-04 against a 12.5 MB pin; build.ps1 targets `smash64ds` and there is no P2 output pin.

## Delegation

Owner now permits UP TO 4 Muse 1.3 Contributor + 4 GLM 5.3 (`zai-coding-plan`, max),
with idle capacity preferred over duplicate work. Native workers are stopped with work
preserved. Muse owns numeric Main metadata and the preview runtime read set; Main owns integration.
Run emulators serially while shared-DLDI behavior is unresolved.
Main owns integration, campaign state, source review and serialized builds.
New prompts/logs: `builds/resume-20260905/`; older reports are in the Claude
session's external temporary `scratchpad`, not a repo directory. The GLM
CLI uses `swarm-probe`/`swarm-build` with explicit model/variant: `glm-probe`
is subagent-only. OpenCode snapshots are disabled to avoid huge Git scans.

## Context discipline

Restart reads this file + `docs/P2_EXECUTION_BOARD.md` only. CodeGraph first, then
bounded reads of the returned seams. `PORTING.md`, `PERF_LEDGER.md`, `VERIFYING.md`,
`KNOWN_ISSUES.md` and the phase plans are lookup-only.
Bank verbose output; read active logs with bounded Python UTF-8 seek/read. Scope git diffs. Invoke make from PowerShell (MSYS login resets cwd/PATH). One build at a time; never
pass `-j` or override `MAKEFLAGS`; run a plain `make` before `verify-all.ps1`
if the last build used lab flags. Owner directives: **no snapshot**, no new
worktrees.
PowerShell: pass rg directories plus `-g` filters, not wildcard-containing file paths. Invoke pytest for pytest files; running them as plain Python may execute zero tests.

Start of cycle: `scripts/verify-all.ps1 -Profile Boundary -List`, `git status --short`.
