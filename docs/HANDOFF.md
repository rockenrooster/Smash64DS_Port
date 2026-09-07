# Handoff

Current: 2026-09-06 — owner reports supersede earlier menu/stage acceptance claims.
Owner visually accepts VS Options, Option and Backup Clear. Prioritize CSS/stage repairs; retain cadence/save validation as open.
**Owner paused 1P campaign development. Do not resume it until requested.**
VS Options round trip is repaired; CSS Link/Yoshi/Pikachu and all eight non-Dream-Land stages need repair. Full observations are in `docs/BUGS.md`. Main Menu/VS Mode are the accepted menu references. The board is
the dynamic queue; `docs/BUGS.md` retains reproductions and unresolved defects.

**Boundary 2026-09-04, both arms GREEN on the shipped nine-fighter/eight-stage
config:** `p2_shell_loop` (free floor 72,148 B), `p2_battle_realtime`
(frames=212, green 46.6%). `p2_fourcpu_stress` is LIVE, not parked: WORK P95
2,697,209 (2.41x), items were OFF (fixed) — board P2-2. `-List` rules.
**Owner (2026-09-05): complete P2; periodically build `smash64ds.nds`, commit regularly and push confirmed progress. No snapshots. This supersedes the earlier build pause.**

## Next

1. **Stages admit natively since `ab3a8f083e4` (2026-09-07):** every blob stage had declined with reason 6 because the blob maxima header was never generated (Makefile dependency on `nds_renderer_assets.o`, an object no rule builds; `__has_include` hid it) and the blob's segment-0 flag ran Dream Land's certificate against every stage. All eight admit since `4099abddd50` (warm uploads at owner prepare + TMEM-load image + env colour; 29.9 FPS entry frames). Open: Zebes data-abort at ~56 battle frames (Mario Appear event32 bind, odd anim_joint pointer), Sector Z force-status overflow, acid scroll re-upload churn — see BUGS.md. Also landed `08ce35de928`: Congo platforms animate (AObjEvent32 plan cap 640), descriptor asset preload, pause decal eject. Uncommitted in tree, unverified: Dokan whole-file import (pipes), player tags via OAM (tags not visible in probes; `oam recognized=1`), Pakkun MObjSub normalize wrapper, tag review fixes. CSS lazy-load patch reversed out (hangs in the shell walk; arena-exhaustion halt most likely, `builds/resume-20260905/agents-0906/css_arena_budget.final.md`). Probe: `builds/resume-20260905/stage-qa/stage-admission-all.ps1`.
2. **Owner repair queue:** native menus pushed/visually accepted. Link reflection repair `5bc1f461f90` restores full native CSS and passes startup/host tests; evidence: `artifacts/performance/2026-09-06_css-link-reflection/`. Menu coverage repair `0b5cb31ff6d` passes. Yoshi CSS is native (commonpart flags byte lane, raw 0xace0 identity, pre-matrix list fold; `artifacts/performance/2026-09-06_css-yoshi-native/`) and Pikachu's ears draw natively (clamped 12x1 tile padding replicated; `artifacts/performance/2026-09-06_css-pikachu-ears/`); `docs/BUGS.md` has both. Battle/stress acceptance of both is open. Stages next. Public ROM unchanged; 1P paused.
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
5. **1P PAUSED by owner.** Pushed through `d155473dd24`; later integration remains local. Campaign reaches Intro and Link/Hyrule play after GO (638 updates), but only 8,356 B remain. Exact lab identity/captures: `builds/resume-20260905/preview-runtime/{intro-capacity-identity.json,first-campaign-combat*}`. NDO3 residency, Intro transient rendering, variant binding/preload and actual fighter-capacity changes are uncommitted. Staffroll-width helper stopped; its partial patch/test must be reviewed before use. Normal ROM remains unchanged. No campaign or P2 acceptance.

Owner decisions owed: `lbRelocGetForceExternHeapFile` raw pointer on a miss; the root P1 ROM is 21.8 MB since 09-04 against a 12.5 MB pin; build.ps1 targets `smash64ds` and there is no P2 output pin.

## Delegation

Owner now permits UP TO 4 Muse 1.3 Contributor + 4 GLM 5.3 (`zai-coding-plan`, max),
with idle capacity preferred over duplicate work. Native workers are stopped with work
preserved. Preview source-generation, reader, native-address and host-test reviews are complete. Main owns integration and current runtime failures.
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
