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
zero rejects; live-input GX RAM is dynamic near `695/2119`, with geometry
`0x222005`, cycle/render `0x00100000/0xc4112078`, parts `14/18`, and zero oracle mismatches.
Initial diffuse/ambient state comes from first selected source `MObjSub` (`0xffffff00/0x4c4c4c00`); per-part overrides carry and fallback use is zero.

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
Canonical retains `227392` taskman headroom (`161856` after BGM), reloc `681632`
bytes, and stale `0/0`. The forensic gate selects the largest 4 KiB-granular arena (`0x14c000`) and retains `145472` after BGM against 128 KiB.

Canonical realtime + live-input + HW-tri shows recognizable Dream Land; source-
separated Mario/Fox bodies are broadly accepted on DS. Source map-object kinds
`0..3` decode exactly, and the original manager grounds Mario/Fox on lines `3/2`
at X `0/-1397`; `gcAddMObjAll` restores O2R lanes by source provenance.
Canonical observes at least four water/Whispy swaps, no native/failure cases,
and first flags `0x0200 -> 0x006b`. Exact TEXEL0/TEXEL1 CI4 conversion uses S/T
maps, immutable source-index planes, and one RGB15/coverage pair table. Large
tiles index first source-address/phase representatives through a 1 KiB table.
Cold uploads retain full scratch materialization; warm large refreshes emit
unique rows directly into the existing 16 KiB buffer and expand its exact map
on VBlank lines `192..207`. Profile 1 proves reuse and zero fallback; profile 2
checks `18,432/0` exact pair pixels through its independent synchronous path.
Profile 0 omits five generic proof ledgers and retains all `828` triangles: `648`
raw-current, `44` cross-matrix, `126` no-Z, and `10` range; reject stays zero.
Null-callback profiles carry only segment-`E` preview resolver state, reset exact
traversal guards/hardware totals, and publish fighter triangles once per owner;
detailed/profile-2 ledgers remain unchanged. Profile 1 proves topology
`80/1736/344/330`. TRI runs retain exact material/depth, RGB15, S/T, projected
X/Y, and clip Z. Texture preparation survives exact-key mutations; profiles 0/1 reuse alpha/poly-format only when proven vertex-independent.
Prepare/reuse is `98/730`;
batching stays `121/707/121`, and logical divide demand stays `1,242`.
Canonical mode 163 is O2; scripted/lifecycle diagnostics remain Os. Six ARM/O3
paths occupy `20,376/20,376/18,584` ITCM bytes. Exact signed
pre-clamping and libnds `div64` remove the shipping software 64-bit helper;
profile 1 records `650` cached calls, while profile 2 compares `1,404` results
with C. Boundary modes retain exact nonzero clamp counts. Divider evidence adds
no BSS; profile 2 also omits the production 2,096-byte shade table and runs the
independent exact shade path. Profiles 0/1 retain light/table and exact 48-slot
texture-key caches; compact fingerprints still require full 236-byte equality.
Profile-0 BSS is `1,875,504`; prepared context persists by `98/730` epoch.
The 2,916-byte main-RAM K-RAW kernel accelerates `45/540` runs/triangles per
frame with `47/7/0` bounded fallbacks. Same-ROM 128-frame profile-1 draw moves
`2,067,296/2,407,872 -> 1,858,624/2,227,648`; stage/Mario/Fox save
`17,568/98,496/93,248` median ticks. The 32-frame profile-2 dual trace, owner
state/cache, oracle, geometry, and upload sequence compare exactly. An 8 KiB
direct table consumes 330 TRI commands without redecode. A 256-byte exact DObj
index plus affine world product moves O2 draw `2,126,752/2,169,600 ->
2,057,376/2,098,880`. Exact persistent stage worlds reuse `57` stable source nodes; profile 2 shadows `42` selected outputs with zero mismatch/reject/overflow.
Matched cache-off/on stage-world draw is `2,323,008/2,355,712 -> 2,263,616/2,280,512`.
Direct compact CI4 rows move `2,001,600/2,033,664 -> 1,970,304/2,002,880` with identical upload hash and zero fallback.
Canonical draw is `2,199,744/2,212,864` at about `14.0fps`.
Source AObj32 graphs normalize once per reloc generation; fighter AObj16 stays separate and original timing remains live.
The BattleShip ground interrupt chain and source floor/edge callbacks are live
under imported FTMANAGER; manual acceptance is pending. A normals and jump/
special physics remain blockers. Persistent slots retain 44 cross-joint
triangles; no-Z/RSP carry restores the fence and five flower groups (`192 -> 202`).
Masked-clamp, six CI4 stars, and DXT tail stride remain fixed. Debt: Whispy face,
other TEXEL1/fog/color animation, speed, and Mario facing/light A/B.
The scripted target is `smash64ds-battle-playable-fast-hwtri.nds`; shipped is `smash64ds-battle-playable-hwtri.nds`. Canonical alone exposes neutral pad 2. Both melonDS LCDs render; the lower canonical screen is intentionally black except for three visible bootstrap rows.
## Recommended Next Work
1. Manually accept source floor/edge callbacks; then repair live A and jump physics.
2. Build complete-stage direct records that fuse live binding with narrow raw/no-Z kernels; the exact fixed layer-0 schedule saved only `8,448` ticks and is reverted.
3. Defer the rare 4 KiB Whispy miss, then add RGBA4 HUD output.

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

DevFast, forensic, P1Gate, and all Boundary modes `161/162/163` pass. Mode 163 now passes the elevated fighter through line 2 before DashRun; skip Full Regression.
After verified progress, run `.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean` last.
