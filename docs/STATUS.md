# Current Status
This is the short current-truth document. Use `docs/DIAGNOSTIC_REFERENCE.md`
for full marker strings; append history in `docs/PORTING.md`.
## Direction
Target remains a full 1:1 playable Nintendo DS source port of BattleShip Smash
64. Import coherent original TU groups, prove them in continuous natural runtime
plus captures, then graduate them live.

Keep `decomp/` read-only; do not hand-author gameplay when source can be ported.
## Current Boundary
The registry decides the active Boundary/Latest set:

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
```

Current Boundary/Latest entries are `battle_mariofox_stage_mplivehit_status_loop`,
`menu_chain_mariofox_stage_mplivehit_status_loop`, and `battle_playable`.

Modes `161/162` remain the bounded natural-combat pair. They use the original
manager and drive movement, Fox Attack11, live hitbox search, Mario damage/
recover, and guard through imported animation/key, status, `ftmain.c`, and
`gmcollision.c`.

Mode `163` has three verifier-covered configurations. The fast harness keeps
its scripted two-human stock chain; canonical realtime/live-input presents the
source five-minute, items-off Mario human versus Fox level-3 CPU match. The
fast lifecycle configuration uses a one-minute test limit, then runs the same
source Time Up and VSBattle-to-VSResults transition. Together they cover
natural combat/KO/rebirth, normal moves, specials, audio, and a DS 3D frame.

## Latest Proof

The imported fighter runtime owns collision, main/manager/CPU, animation/key,
statuses, normal moves, weapons, effects, and specials. The lifecycle gate
records source CPU/input/combat, consumes all `3600` test ticks, returns through
taskman cleanup, and changes `VSBattle(22) -> VSResults(24)`. Imported Results
loads all eight files, two fighters, 12 SObjs, and source Win/Lose statuses.

The renderer imports BattleShip display/light/matrix source and preserves live
camera, selected DL order, RSP vertex carry, materials, lighting, and depth.
Profile 2 owns exact oracle coverage; production Mode 8 owns the accepted native
fighter path. Geometry remains `828 = 202+320+306` source triangles with exact
`648/44/126/10` classes and zero oracle mismatch. Detailed optimization history
and counters live in `docs/PERF_LEDGER.md`.

The mode-163 runtime links original camera, death/rebirth, HUD, normal moves,
weapons, effects, CPU, audio parsing, and Pupupu BGM. Its lifecycle proof reaches
source expiry, taskman cleanup, and Results while preserving the 128 KiB reserve.
The canonical configuration remains the five-minute live-input P1 match.

The coarse target intrinsically forces realtime presentation, live input, HW
triangles, profile 1, and `NDS_RENDERER_FAST_RUN_DEFAULT=8`. A plain target
build reproduces ROM `DC2871F3...52E4E3AD` (12,036,096 bytes). Eight exact
Mode-8 frames retain all 828 triangles with `70/686` runs, `60+320+306` owners,
and `29/0/0` fallbacks at about 15.4 FPS. The exact-ROM paired capture passes
visibility, motion, named-region, and texture-detail gates. A same-ROM no-build
run passes original Fox CPU, natural BGM/refills, reserve, and ITCM checks.

The `39CD1397...B508CEF` cleanup ROM was built with live input disabled by an
under-specified manual invocation and is rejected. Its PNG files were complete;
the apparent partial second images came from a multi-image inspection view that
displayed unchanged regions as black. The user's separate report of no audio or
stage draw on the exact `DC287...` ROM remains pending manual-environment retest,
so hardware/manual acceptance is not claimed.

Working-looking emulator captures exist for Mode 0, Mode 8, and canonical
HW-tri only. There is no per-mode capture set for Modes 1-7, and screenshots do
not prove hardware boot or audio.

Cut F is rejected and removed. Its first cache window reached
`467,968/468,992` draw with zero stage/wallpaper CPU time, but the seed omitted
wallpaper pixels and the live source camera exceeded cache bounds after 232
frames, forcing full generic fallback. The large short-window saving does not
justify further work on an invalid presentation architecture.

A separate per-callback typed stage executor is rejected and removed. Its
candidate stage time was about `877,248` ticks versus `873,344` generic, with
799/828 fast triangles and one fallback. It saved essentially zero stage time;
the whole-draw difference was the retained fighter owner.

Full-source scanline/HBlank wallpaper is also rejected and removed. Same-ROM
draw saved `239,392` ticks and wallpaper saved `240,032`, below the 300K gate;
its source upload also worsened P95. Exact `DC287...` Mode 8 is restored.

## Current Notes

The taskman allocator now probes 4 KiB pages from `0x150000` through `0x130000`
before its smaller fallbacks, avoiding the old 64 KiB capacity cliff.
Source ground/floor/edge callbacks are live; manual acceptance is pending. Live input now dispatches A normals, first jump, and double jump.
The full Mario battle-animation bank (`499..641`) resolves to staged BattleShip O2R; compact path lookup avoids 143 redundant ARM9 records and retains scripted headroom `198416` (`132880` after BGM).
Live checks load normalized assets `606/509/511` and advance AObj16 joints. Mario Up-B now restores exact BattleShip TransN facing/rotation/axis motion and rising PROJECT/descending PASS map semantics; near-wall/floor joins and ceiling-edge adjustment remain incomplete.

Canonical realtime + live-input + HW-tri has hard GX RAM, display-contract,
profile-0, screenshot, and ROM-parity gates; profile 2 owns oracle correctness.
DevFast builds it, captures once, and rotates `latest.png` to `previous.png`.
The scripted mode-163 ROM remains internal; three user-facing names represent
two configurations. Both melonDS LCDs render; the canonical lower screen is
black except for three bootstrap status rows.

Modes `161/162` remain bounded scaffolding; `battle_playable` is the scene-level
anchor. Obsolete mode/verifier stacks are migrate-or-delete with one
`[coverage-reduced]` line; modes `57/58` and `159/160` are already gone.

The last decision baseline was `14.9fps` with `1,856,864/1,888,960` draw; its profile-1 stage/draw were `771,680/801,344` and `1,790,368/1,879,872`. Current canonical/shipped parity is 12,033,024 bytes at SHA-256 `99FF3D2DB2A51B341548821BFECE87A22AA2614D8A0AD1B1B3F4E050C901BE30`.

The serialized title/opening store still saves `176,000` BSS bytes; mode 9 again
reported mode-8 accounting and no timing, so its `18,676`-byte workspace is gone.

Cut D is rejected and its census code is removed. A 600-frame run found 322
complete keys, 206 final outputs, reuse distance exactly 216, 76.17% changed
frames, and 3,739,648 bytes for full residency; a 2-4-slot LRU cannot hit.
ARM7 overlap would require a second source/render prepass because the exact
recipe first exists at its dependent bind, leaving only 77,376 ticks of total
prepass/IPC/wait budget to clear the 120K gate. The 176,736-tick matrix aggregate
also cannot supply the requested hundreds-of-thousands jump.

Preserve the exact hardened Mode-8 baseline and retest the user's manual
environment before changing ROM code for that report. The next performance
architecture must be distinct from mode 9, Cut D, Cut F, and the typed
stage executor, with at least 300K credible exclusive saving before implementation.
Stop after one build/60 minutes without it; do not return to prepared VMs,
record executors, matrix-only, affine/scanline wallpaper, or local no-Z/raw work.
HUD, weapon geometry, platform crossing, Whispy face, and Mario facing/light
remain presentation debt.
## Verification

This checkpoint passes the exact coarse Mode-8 benchmark, paired capture gates,
realtime CPU/audio/memory verifier, all four static checks, and a fresh DevFast
canonical build/capture/parity run. The prior broad checkpoint passed forensic,
P1Gate, and Boundary `161/162/163`; rerun those after further shared-runtime
work. Full Regression stays skipped for minimum-calendar work.

```powershell
.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3
.\scripts\verify-p1-gate.ps1 -DelaySeconds 3
.\scripts\verify-boundary.ps1 -DelaySeconds 3
```
After verified progress, run `.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean` last.
