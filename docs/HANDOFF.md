# Handoff

Current: 2026-09-07 — owner order: Zebes crash/loading first, then acid,
tornado, barrel, Arwing motion, then geometry/depth/Saffron wall/alpha/particles;
DATA children last. **1P campaign paused by owner; do not resume unrequested.**
Owner accepts Main Menu/VS Mode/VS Options/Option/Backup Clear. `docs/BUGS.md`
is the owner's live queue (≤20-word statuses), `docs/p2/BUG_NOTES.md` the evidence.
Fixed 2026-09-07: Zebes crash (event32 ledger, now 5,120), acid overdraw (actors
keep G_ZBUFFER), Yoshi→Mario on START. Sound Test/VS Record wired, unverified.

**Boundary 2026-09-07 morning, both arms GREEN:** `p2_shell_loop` (free floor
48,688 B), `p2_battle_realtime` (frames=212). `p2_fourcpu_stress` RED: wander
crash after frame 256, heap floor 15,640 B < 25,600 — RAM cliff, board P2-2.
Boundary is due again on the cull-baseline commit. `-List` rules.
**Owner (2026-09-05): complete P2; periodically build `smash64ds.nds`, commit regularly and push confirmed progress. No snapshots. This supersedes the earlier build pause.**

## Next

1. **Stages (2026-09-07):** all eight admit natively (`ab3a8f083e4`, `4099abddd50`); actor arms live (barrel, clouds, gate, acid). Open, evidence in `docs/p2/BUG_NOTES.md`: Congo platform barrel is not the cannon GObj and the native quad is invisible; Hyrule back faces/depth; Castle roof, Yoster floor, Inishie side platforms (packets hold the triangles; runtime declines unmeasured); Saffron wall/door alpha; cloud alpha; Sector Arwing motion; tornado trace (`-TornadoBt`). The intermittent entry-animation ENOENT was a build prune race (fixed). Stage packets now enter with G_CULL_BACK like the RSP baseline (all 40 re-pinned; Boundary due). Ledger high-water 4,035 at 4,096 → 5,120; bake-time event32 pre-normalization would retire it (`agents-0906/event32_prenormalize.final.md`). melonDS host crash 0xc000001d hits ~half the Jungle probe launches; rerun. Unverified WIP in tree: Dokan pipes, OAM tags, Pakkun normalize, tag fixes; the 1P/staffroll Makefile WIP rode into the cull commit unreviewed.
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
