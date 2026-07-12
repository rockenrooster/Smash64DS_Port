# Handoff
Use this file for the active handoff only. Keep it under 150 lines. The harness
registry decides the current Boundary/Latest profile; this file summarizes what
to do next.

## Start Here

1. Check the registry view:

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
```

2. Expected Boundary/Latest entries:

```powershell
.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3
.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3
.\scripts\verify-battle-playable-harness.ps1 -DelaySeconds 3
```

3. Read `docs/STATUS.md` for current truth, `docs/DIAGNOSTIC_REFERENCE.md` for
full marker strings, and `docs/PORTING.md` for history.

## Current Boundary

Modes `161/162` are the bounded natural-combat pair. In the default
original-manager build they prove natural Mario/Fox combat on the Pupupu
Mario/Fox battle root: Wait -> Walk -> Dash -> Run -> RunBrake -> Turn,
Fox Attack11, live hitbox search, Mario damage/recover, and GuardOn/Guard/
GuardOff through imported `ftanim.c`/`ftkey.c`, original status descriptors,
`ftmain.c`, and `gmcollision.c`.

Mode `163` is the scene-level `battle_playable` Boundary/Latest anchor. Its fast
configuration retains the scripted two-human stock chain. Canonical realtime/
live-input presents the source five-minute, items-off Mario human versus Fox
level-3 CPU match. `battle_playable_match_lifecycle` uses a one-minute test
limit and runs the same source timer, Time Up, taskman cleanup, and original
`VSBattle(22) -> VSResults(24)` transition. Imported
`mnvsresults.c`, `lbtransition.c`, `scsubsysfighter.c`, and `scsubsysdata.c`
now own Results by default. The gate reaches tick `120+`, loads all eight files,
creates two fighters and 12 SObjs, and installs source Win/Lose statuses.

Latest renderer detail: BattleShip `ftdisplaymain.c`, `ftdisplaylights.c`, and
`guMtxCatF` are imported. The live fighter path now uses the original display
preamble, light state, visibility flags, and `dl`/ordered-`dls[]` selection;
only those selected DLs cross the narrow DS hardware-submission seam. The
manual all-DObj collector remains only as the software fixture oracle.

The source camera matrix/projection is prepared before fighter visibility
selection. Each selected event retains its matrix/material owner plus per-draw
geometry/prim/env/light and cycle/render state; `dls[0]` uses parent state.
The adapter now preserves the 32-slot input/transformed RSP vertex cache across
the selected part sequence, as BattleShip's common `gSYTaskmanDLHeads[0]`
stream does. All-DL HW output is the full `320/306` Mario/Fox triangle set with
zero rejects; latest canonical reports `gxram=733/2219`, geometry `0x222005`, cycle/
render `0x00100000/0xc4112078`, parts `14/18`, and zero oracle mismatches.
Initial diffuse/ambient state comes from the first selected source `MObjSub`
(`0xffffff00/0x4c4c4c00`); per-part overrides carry and fallback use is zero.

`battle_playable` default: `NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE=1` now links
original `gm/gmcamera.c`, `ftcommondead.c`, `ftcommonrebirth.c`,
battle-critical `if/ifcommon.c` HUD paths, original `if/ifscreenflash.c`, the
normal moveset imports, the weapon manager, Mario fireball, Fox blaster, the
original effect manager, Fox reflector, Mario Super Jump Punch, Mario Tornado,
Fox Fire Fox, `ftcomputer.c`, original audio parsing, and Pupupu BGM playback.
The one-minute lifecycle CPU gate records `7594` original process/target calls,
objective/behavior `0x204/0x9`, `57` input frames, `1668/120` attack frames,
`856` guard frames, `246` status changes, and `81%` maximum Mario damage.
Mode 163 proves stock/KO/rebirth, all normal-move phases, grab/throw damage,
HUD digits/stocks, projectile/reflector/specials, parsed audio, looping BGM, and
`42/202` stage DL/HW triangles plus `2/626` fighter draws/triangles. FGM/voice,
the original sequence player, and non-critical HUD/SObj/particles remain debt.
The ledger retains `227392` headroom, reloc `681632` bytes, stale `0/0`, and
`161856` bytes after BGM against the 128 KiB reserve.

Canonical realtime + live-input + HW-tri shows recognizable Dream Land; source-
separated Mario/Fox bodies are broadly accepted on DS. Source map-object kinds
`0..3` decode exactly, and the original manager grounds Mario/Fox on lines `3/2`
at X `0/-1397`; `gcAddMObjAll` restores O2R lanes by source provenance.
Canonical observes at least four water/Whispy swaps, no native/failure cases,
and first flags `0x0200 -> 0x006b`. Exact TEXEL0/TEXEL1 CI4 conversion now uses
precomputed source-address maps, row-local T addressing, packed paired texels,
and 17 exact Bayer phase masks. Non-CI4 and non-adjacent cases retain the generic
path. The two animated uploads cover `36,864` bytes and positive scene-lifetime
direct-pixel/refresh proof with zero eviction, reject, or oracle drift.
Profile 0 retains all `828` triangles: `648` raw-current, `44` cross-matrix,
`126` no-Z, and `10` range; snapshot/decal/prim/reject stay zero. Profile 1
proves reloc-backed topology `80/1736/344/330`: immutable lists/trusted commands/
dynamic validations/replayed adjacent TRI commands. Within one unchanged TRI
run it reuses exact material/depth, RGB15 color, S/T, projected X/Y, and source
clip Z. Non-TRI commands close GX; texture preparation survives VTX/matrix and
invalidates at exact key mutations. Prepare/reuse is `98/730`; batching remains
`121/707/121`, and divisions remain `1,242`.

Canonical mode 163 is O2; scripted/lifecycle diagnostics remain Os to preserve
their `227392`-byte reserve. Its renderer TU now follows sm64-nds in ARM state;
four measured O3 loops occupy `14,212` ITCM bytes in profile 1 (`14,128`
forensic), while normal/legacy builds stay Thumb. One 40-byte exact context
supplies all three vertex calls instead of restaging 22 arguments three times;
eight Boolean modes share one word. Profiles 0/1 reuse live ordered-list state;
profile 2 remains independent. Runtime omits its 82,176-byte stats array.
Matrix loads remain `53`, batches `121/707/121`, and submissions
`648/44/126/10`. An 8 KiB table resolves all sixteen exact CI4 coverage phases;
the removed pair branch covered only `2,048/18,432` pixels. Matched profile-1
draw falls `4,473,120 -> 4,164,800`, DL `3,317,280 -> 3,017,888`, texture
`908,192 -> 595,424`, and setup `1,385,856 -> 1,074,912` ticks. Profile 0
reaches `7.5fps` at `3,983,296/3,984,640` draw; profile 2 retains oracle
`2484/0/0`. Capture: `artifacts/visibility/2026-07-12_canonical_fast_030326-3934155-p33036.png`;
shipped SHA-256: `00C9A6167F01E43950098854A56BFB271F1E3A1CEE2741EF7F14D38E24D397FB`.
Source AObj32 graphs normalize once per reloc generation; fighter AObj16 stays
separate, original timing stays live, and a post-step corrects packed RGBA.
Persistent slots retain 44 cross-joint triangles; phase-aware no-Z restores the
foreground fence. Stage RSP/ST carry restores five flower groups (`192 -> 202`).
Masked-clamp, six CI4 stars, and DXT tail stride remain fixed. Debt: Whispy face,
other TEXEL1/fog/color animation, speed, and Mario facing/light A/B.
The scripted target is `smash64ds-battle-playable-fast-hwtri.nds`; shipped is `smash64ds-battle-playable-hwtri.nds`. Canonical alone exposes neutral pad 2.

## Recommended Next Work
1. The canonical ROM is still only `7.5fps`, not P1-complete. Profile 1 leaves
   about `1.43M` scan and `1.07M` non-vertex setup ticks. Split source matrix/
   state commands and VTX lighting before another exact runtime cut.
2. Add source RGBA4 interface/HUD output with final-resolution dirty BG3. Keep
   Whispy face strips and Mario facing/light A/B as the remaining visual A/B.
3. Use DevFast while iterating, P1Gate at integrated checkpoints, and Boundary
   once before handoff. Historical one-bit harnesses are localization tools.

Do not restore the rejected five-address load-time `MObjSub` probe; the accepted seam is the generic original attachment boundary and proves live output.
The corrected tile-origin equation is source parity, but its fixed-camera probe changed only `18/49152` pixels; do not cite it as the remaining ribbon fix.
Do not translate N64 `CLAMP` as clamp-only when a nonzero mask owns a smaller physical period; preserve logical tile clamp and masked repeat/mirror separately.
Do not revisit the high-bit fighter branch for current fragments (`0/0` active Mario/Fox descriptors), or queue Dream Land by generic head assumptions (`layer_mask=0`, `42/0` lists, `0/49152` changed pixels).

## Verification

For docs-only edits, run `.\scripts\check-docs.ps1`. Use the visible canonical
loop per meaningful edit and the additive shadow checkpoint for shared-runtime
work:

```powershell
.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3
.\scripts\verify-battle-playable-renderer-forensic.ps1 -DelaySeconds 3
.\scripts\verify-p1-gate.ps1 -DelaySeconds 3
.\scripts\verify-boundary.ps1 -DelaySeconds 3
```

P1Gate passed in `189.2s`; Boundary passed in `81.2s`. DevFast passed in
`44.5s` and the separate forensic oracle in `24.5s`. This is not the
five-minute P1 soak; skip Full Regression.
After verified progress, run `.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean` last.
