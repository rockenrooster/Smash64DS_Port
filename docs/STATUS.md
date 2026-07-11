# Current Status
This is the short current-truth document. Use `docs/DIAGNOSTIC_REFERENCE.md`
for full marker strings; append history in `docs/PORTING.md`.
## Direction
Target remains a full 1:1 playable Nintendo DS source port of BattleShip Smash
64. Gameplay work now moves by runtime-first subsystem slices aimed at
scene-level capability: import coherent original TU groups, prove with the
continuous natural-runtime verifier plus captures, then graduate live.

Keep `decomp/` read-only. Do not hand-author gameplay when BattleShip source can
be ported.
## Current Boundary
The registry decides the active Boundary/Latest set:

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
```

Current Boundary/Latest entries:

```powershell
.\scripts\verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3
.\scripts\verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1 -DelaySeconds 3
.\scripts\verify-battle-playable-harness.ps1 -DelaySeconds 3
```

Modes `161/162` remain the bounded natural-combat pair. They keep the
Pupupu/Dream Land Mario/Fox battle root stable, create Mario/Fox through the
original manager, and drive Wait -> Walk -> Dash -> Run -> RunBrake -> Turn,
Fox Attack11, live hitbox search, Mario damage/recover, and GuardOn/Guard/
GuardOff through imported `ftanim.c`/`ftkey.c`, original status descriptors,
`ftmain.c`, and `gmcollision.c`.

Mode `163` has three verifier-covered configurations. The fast harness keeps
its scripted two-human stock chain; canonical realtime/live-input presents the
source five-minute, items-off Mario human versus Fox level-3 CPU match. The
fast lifecycle configuration uses a one-minute test limit, then runs the same
source Time Up and VSBattle-to-VSResults transition. Together they cover
natural combat/KO/rebirth, normal moves, specials, audio, and a DS 3D frame.

## Latest Proof

The original fighter runtime is live: `gmcollision.c`, `ftmain.c`,
`ftmanager.c`, `ftcomputer.c`, animation/key, common/Mario/Fox statuses, normal
moves, weapons, effects, and specials run through imported BattleShip code.
The lifecycle gate records source CPU process/target frames, A/B/Z inputs,
live hitboxes, guard, and positive Mario damage. Original
`ifcommon.c:2472-2537,3144-3152,3342-3345` consumes all `3600` one-minute test
ticks and requests `LoadScene`; imported
`scvsbattle.c:513-560` returns through taskman cleanup and changes scene
`VSBattle(22) -> VSResults(24)`. Imported `mnvsresults.c`, `lbtransition.c`,
and the source subsystem fighter/data support now run Results by default. The
gate reports tick `120+`, all eight files, two fighters, 12 SObjs, and source
Win/Lose statuses installed through original `ftMainSetStatus`.

The fighter renderer imports BattleShip `ftdisplaymain.c`, `ftdisplaylights.c`,
and `guMtxCatF`. Its display preamble, lighting state, visibility flags, and
single-`dl`/ordered-`dls[]` selection run live; only selected lists cross the DS
submission seam. The manual all-DObj collector remains a software fixture.
Imported `sys/objanim.c` now receives source-shaped DObj/MObj/CObj AObj32
graphs: the wrapper repacks only MSB-first commands once per reloc generation,
follows branches, preserves payloads/pointers, and excludes fighter AObj16.
Original timing stays live; a post-step corrects N64-endian packed RGBA.
Selected events retain source matrix/material, geometry/prim/env/light, and
cycle/render state; pre-matrix `dls[0]` keeps parent state as in
`ftdisplaymain.c:789-805,883-899`. The DS bridge also carries the RSP input and
transformed vertex cache across those per-part lists, matching BattleShip's
single `gSYTaskmanDLHeads[0]` stream. Exact Mario cross-joint fixtures now pass,
all-DL HW triangles rise from `284/298` to `320/306`, and rejects fall to zero.
Fighter playback seeds its initial light pair from the first selected source
`MObjSub` (`0xffffff00/0x4c4c4c00`); overrides carry and fallback use is zero.
Canonical proof reports `gxram=730/2214`, geometry `0x222005`, source cycle/
render `0x00100000/0xc4112078`, parts `14/18`, and zero oracle mismatches.
Source-depth X/Y/Z share one composed clip vertex. Projected no-Z depth has
source-backed background/foreground phases around the first source-Z triangle,
restoring layer-3 foreground fences over layer-1 floor/path.

A source-shaped `gcAddMObjAll` wrapper now normalizes mixed-width O2R lanes by
loaded-file and asset/generation provenance. Canonical Dream Land observes at
least four water/Whispy swaps, zero native/failure cases, and first flags
`0x0200 -> 0x006b`. The renderer resolves the source tile-6/TMEM-0x40 TEXEL1 and
tile-7/TMEM-0 TEXEL0 independently, recognizes exact `G_CC_TEMPLERP`, and
precomposes its CI4 pair with a DS RGBA5551/A1 approximation. Compatible
animated state refreshes resident VRAM with frame pinning. The 184-frame gate
has positive scene-lifetime refresh, zero eviction/reject/oracle drift, and a
terminal `12/12` matched frame. Pond detail is `46.053%/23px`, versus white
`27.997%/105px`.
Fox's DXT tail fix remains; Tyler accepts the water. Persistent stage RSP cache
plus `G_MWO_POINT_ST` restores five flower groups and adds ten source triangles
(`192 -> 202`); Tyler accepts both flowers and foreground fences. Texture
conversion retains exact `37200/37200` lane observations and oracle `2403/0/0`.
The immutable 300x220 wallpaper now decodes once into the retained HW buffer;
source position/scale stay live and composed content is never cached. Proof is
`build1/hit44/fast45/fallback0/opaque66000`; present fell
`34,839,424 -> 24,764,160` ticks (`-28.9%`). Same-state GX triangle batching
now spans only adjacent TRI1/TRI2 commands and closes at every other opcode or
list exit. Canonical proves `begin103/reuse725/end103` with all `828` triangles
and oracle results unchanged; present fell again to `24,238,464` (`-2.1%`),
draw to `23,877,568` (`-0.8%`), while pacing remains `13/13 x0.1`. The dated
capture is `artifacts/visibility/2026-07-11_canonical_fast_154544-4627518-p18756.png`.

The memory pre-breadth gate has a live VSBattle ledger and scene-owned reloc
cache eviction. Mode `163` reports headroom `237948`, resident reloc `681632`
bytes (`stage=202816`, `fighter=175440`, `if=208672`), stale `0/0`, and source
VSBattle buffers from `scvsbattle.c:31-41`. Audio `.ctl` parsing now peaks at
`16` bytes of scratch. The separate 64 KiB BGM buffer leaves `172412` bytes
against the 128 KiB reserve.

## Current Notes

The taskman allocator now tries `0x140000` and `0x130000` before its legacy
1 MiB fallback, preventing source-display builds from overflowing after a
failed `0x150000` allocation while preserving the 128 KiB reserve.
Pupupu map-object kinds `0..3` now decode as `(0,6)`, `(-1397,906)`,
`(1,1545)`, and `(1421,909)` with no duplicates or unaligned reads. Mario and
Fox enter Wait grounded on lines `3/2` at X `0/-1397`.

Canonical realtime + live-input + HW-tri renders through `gcDrawAll`, polls
live pads before each update, and has hard GX RAM, oracle, display-contract,
and screenshot gates. DevFast incrementally builds it, runs one capture pass, rotates
`latest.png` to `previous.png`, and requires exact canonical/shipped ROM parity.
The scripted mode-163 ROM remains an internal `-fast-hwtri` target; three
user-facing filenames represent only two unique configurations.

The active `161/162` boundary is still bounded proof scaffolding, while
`battle_playable` is the scene-level battle anchor.
Legacy bounded modes are migrate-or-delete: obsolete mode/verifier stacks get
deleted with one `[coverage-reduced]` `KNOWN_ISSUES` line. Modes `57/58` and
`159/160` have already been deleted.

Next P1 work is Whispy and Mario light A/B, then measured renderer work,
phase/shifts, fog, and gameplay-critical FGM/voice.
## Verification

All four `P1Gate` legs pass in `278.9s`: compact opening-to-Title, canonical
live battle/capture, supplemental mode-163 combat, and one-minute Results.
Fresh Boundary passes in `152.2s`. This is not the five-minute P1 soak; Full
Regression was skipped for Tyler's fast cadence.

```powershell
.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3
.\scripts\verify-p1-gate.ps1 -DelaySeconds 3
.\scripts\verify-boundary.ps1 -DelaySeconds 3
```

After verified progress, commit if requested, then run the Lean snapshot last:

```powershell
.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean
```
