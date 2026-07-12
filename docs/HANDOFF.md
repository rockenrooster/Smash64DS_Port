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
zero rejects; latest canonical reports `gxram=715/2167`, geometry `0x222005`, cycle/
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
precomputed source-address maps, row-local T addressing, and 17 exact Bayer
phase masks. Profiles 0/1 decode the two immutable 32x32 packed source-index
planes once; live origins/masks/palette/fraction/phase and uploads stay dynamic.
Scene change invalidates pointer keys; profile 1 proves `2/712` build/reuse and
profile 2 remains bytewise `0/0`. The two uploads cover `36,864` bytes with zero
eviction, reject, or oracle drift.
Profile 0 retains all `828` triangles: `648` raw-current, `44` cross-matrix,
`126` no-Z, and `10` range; snapshot/decal/prim/reject stay zero. Profile 1
proves reloc-backed topology `80/1736/344/330`: immutable lists/trusted commands/
dynamic validations/replayed adjacent TRI commands. Stage/fighter dynamic
validation now checks the taskman arena before the loaded-file ledger. Within a TRI
run it reuses exact material/depth, RGB15 color, S/T, projected X/Y, and source
clip Z. Non-TRI commands close GX; texture preparation survives VTX/matrix and
invalidates at exact key mutations. Prepare/reuse is `98/730`; batching remains
`121/707/121`, and divisions remain `1,242`.

Canonical mode 163 is O2; scripted/lifecycle diagnostics remain Os to preserve
their `227392`-byte reserve. Its renderer TU now follows sm64-nds in ARM state;
six measured O3 paths occupy `19,036/19,340/18,216` ITCM bytes in profiles
0/1/2, while normal/legacy builds stay Thumb. The source VTX handler now
uses guarded aligned word loads with bytewise fallback and decodes directly into
the persistent cache. Exact light direction persists until matrix/MOVEMEM
mutation; four 128-step tables key diffuse/ambient RGB while live normal,
direction, alpha, and incomplete-state fallback remain uncached. The new
two-plane cache adds 2,112 profile-0 BSS bytes. Matrix/batch/submission contracts
remain exact. Combined matched P95 draw falls `3,974,720 -> 3,585,792`, DL
`2,820,032 -> 2,430,080`, scan `1,256,576 -> 950,720`, texture
`593,216 -> 509,376`, and setup `1,073,088 -> 988,608`. Profile 0 peaks at
`8.7fps`; final DevFast is `8.6fps`; profile 2 retains `2484/0/0`. Capture:
`artifacts/visibility/2026-07-12_canonical_fast_050724-7066269-p20384_next.png`;
shipped SHA-256: `A617BE4778EF26809769D64CF473ED7C8CF1AA1BA513B39D8AE43B32CD390306`.
Source AObj32 graphs normalize once per reloc generation; fighter AObj16 stays
separate, original timing stays live, and a post-step corrects packed RGBA.
Persistent slots retain 44 cross-joint triangles; phase-aware no-Z restores the
foreground fence. Stage RSP/ST carry restores five flower groups (`192 -> 202`).
Masked-clamp, six CI4 stars, and DXT tail stride remain fixed. Debt: Whispy face,
other TEXEL1/fog/color animation, speed, and Mario facing/light A/B.
The scripted target is `smash64ds-battle-playable-fast-hwtri.nds`; shipped is `smash64ds-battle-playable-hwtri.nds`. Canonical alone exposes neutral pad 2. Both melonDS LCDs render; the lower canonical screen is intentionally black except for three bootstrap rows.

## Recommended Next Work
1. The canonical ROM is still only `8.6fps`, not P1-complete. Profile the
   `0.94M/0.99M` scan/setup buckets and exact TRI-run setup before a packet cache.
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

P1Gate/Boundary passed in `270.0s/132.5s`; DevFast/rebuilt forensic in
`51.9s/50.5s`. This is not the five-minute P1 soak; skip Full Regression.
After verified progress, run `.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean` last.
