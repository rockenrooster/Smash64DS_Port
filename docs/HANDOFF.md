# Handoff

Current: 2026-09-07 — owner order: barrel, Arwing, then geometry/depth/alpha/
particles; DATA children last. **1P paused by owner; do not resume unrequested.**
Owner accepts Main Menu/VS Mode/VS Options/Option/Backup Clear. `docs/BUGS.md` is
the owner's queue (≤20-word statuses); `docs/p2/BUG_NOTES.md` holds the evidence.
Fixed 2026-09-07: Zebes crash (ledger 5,120 x 5 B), acid overdraw, Yoshi→Mario on
START, Hyrule tornado (descriptor read 4 bytes early; damage 14 angle 90),
Mushroom Kingdom music (PCM16 word length, 0 seam miss to present 600), Yoster
cloud A5I3 upload reached. Sound Test/VS Record wired, unverified.

**PAUSED by owner 2026-09-07 ~20:30, mid-Boundary.** Emulators, agents and builds
stopped; nothing mid-write. **Boundary is OWED on HEAD** (two-cycle classifier
fix): killed before either arm reported. Last complete run, one commit earlier:
`p2_shell_loop` GREEN (free floor 35,604 B), `p2_battle_realtime` GREEN
(frames=212), `p2_fourcpu_stress` RED on the standing wander crash (RAM
cliff, board P2-2). Resume: `scripts/verify-all.ps1 -Profile Boundary`.
The standing goal was cleared at the pause; do not resume autonomous work.
**Owner (2026-09-05): complete P2; periodically build `smash64ds.nds`, commit regularly and push confirmed progress. No snapshots. This supersedes the earlier build pause.**

## Next

0. **Theories killed 2026-09-07** (evidence in `docs/p2/BUG_NOTES.md`): barrel
   admits and emits 2 tris/frame yet is invisible (placement); Sector Z makes NO
   Arwing in 1,400 presents and refuses nothing (maker, not flight data); Yoster
   packet covers every source surface (draw-time loss); Castle roof is not the
   near fan; the 20 FPS gap is neither packet nor wallpaper seed.
1. **Stages (2026-09-07):** all eight admit natively (`ab3a8f083e4`, `4099abddd50`); actor arms live (barrel, clouds, gate, acid). Open, evidence in `docs/p2/BUG_NOTES.md`: Congo platform barrel is not the cannon GObj and the native quad is invisible; Hyrule back faces/depth; Castle roof, Yoster floor, Inishie side platforms (packets hold the triangles; runtime declines unmeasured); Saffron wall/door alpha; cloud alpha; Sector Arwing motion; tornado trace (`-TornadoBt`). The intermittent entry-animation ENOENT was a build prune race (fixed). Stage packets now enter with G_CULL_BACK like the RSP baseline (all 40 re-pinned; Boundary due). Ledger high-water 4,035 at 4,096 → 5,120; bake-time event32 pre-normalization would retire it (`agents-0906/event32_prenormalize.final.md`). melonDS host crash 0xc000001d hits ~half the Jungle probe launches; rerun. Unverified WIP in tree: Dokan pipes, OAM tags, Pakkun normalize, tag fixes; the 1P/staffroll Makefile WIP rode into the cull commit unreviewed.
2. **Owner repair queue:** native menus pushed/visually accepted. Link reflection repair `5bc1f461f90` restores full native CSS and passes startup/host tests; evidence: `artifacts/performance/2026-09-06_css-link-reflection/`. Menu coverage repair `0b5cb31ff6d` passes. Yoshi CSS is native (commonpart flags byte lane, raw 0xace0 identity, pre-matrix list fold; `artifacts/performance/2026-09-06_css-yoshi-native/`) and Pikachu's ears draw natively (clamped 12x1 tile padding replicated; `artifacts/performance/2026-09-06_css-pikachu-ears/`); `docs/BUGS.md` has both. Battle/stress acceptance of both is open. Stages next. Public ROM unchanged; 1P paused.
2. **RAM is the binding P2 constraint** (CSS + battle + P2-3f47): offline
   match-resident pack, paging REFUSED (`p2/P2-2-four-fighters.md`,
   `p2/fighters/kirby.md`); compact CSS packages generator done, loader next.
3. **Campaign lab**: Polygons and Master Hand link, donor flags forced in both
   makes; gameplay acceptance open. Hammer/Star arbitration source-correct
   (`ft/ftparam.c:93-155`, 162,732 host cases); ROM acceptance pending.
5. **1P PAUSED by owner.** Pushed through `d155473dd24`; later integration remains local. Campaign reaches Intro and Link/Hyrule play after GO (638 updates), but only 8,356 B remain. Exact lab identity/captures: `builds/resume-20260905/preview-runtime/{intro-capacity-identity.json,first-campaign-combat*}`. NDO3 residency, Intro transient rendering, variant binding/preload and actual fighter-capacity changes are uncommitted. Staffroll-width helper stopped; its partial patch/test must be reviewed before use. Normal ROM remains unchanged. No campaign or P2 acceptance.

Owner decisions owed: `lbRelocGetForceExternHeapFile` raw pointer on a miss; the root P1 ROM is 21.8 MB since 09-04 against a 12.5 MB pin; build.ps1 targets `smash64ds` and there is no P2 output pin.

## Delegation

Owner permits UP TO 4 Muse 1.3 Contributor + 4 GLM 5.3 (`zai-coding-plan`, max).
Launch WRITE agents only between
builds: an agent edit landing mid-verifier failed the 2026-09-07 battle arm. Run emulators serially while shared-DLDI behavior is unresolved.
Main owns integration, campaign state, source review and serialized builds.
New prompts/logs: `builds/resume-20260905/`; older reports live in the Claude
session's external `scratchpad`. The GLM CLI takes `swarm-probe`/`swarm-build`
with an explicit model/variant. OpenCode snapshots are off.

## Context discipline

Restart reads this file + `docs/P2_EXECUTION_BOARD.md` only. CodeGraph first, then
bounded reads of the returned seams; other docs are lookup-only.
Bank verbose output; read active logs with bounded Python UTF-8 seek/read. Scope git diffs. Invoke make from PowerShell (MSYS login resets cwd/PATH). One build at a time; never
pass `-j` or override `MAKEFLAGS`; run a plain `make` before `verify-all.ps1`
if the last build used lab flags. Owner directives: **no snapshot**, no new
worktrees.
PowerShell: pass rg directories plus `-g` filters, not wildcard-containing file paths. Invoke pytest for pytest files; running them as plain Python may execute zero tests.

Start of cycle: `scripts/verify-all.ps1 -Profile Boundary -List`, `git status --short`.
