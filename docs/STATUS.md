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

Current Boundary/Latest entries are `battle_mariofox_stage_mplivehit_status_loop`,
`menu_chain_mariofox_stage_mplivehit_status_loop`, and `battle_playable`.

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
Canonical proof reports `gxram=742/2251`, geometry `0x222005`, source cycle/
render `0x00100000/0xc4112078`, parts `14/18`, and zero oracle mismatches.
Source-depth X/Y/Z share one composed clip vertex. Projected no-Z depth has
source-backed background/foreground phases around the first source-Z triangle,
restoring layer-3 foreground fences over layer-1 floor/path.

A source-shaped `gcAddMObjAll` wrapper now normalizes mixed-width O2R lanes by
loaded-file and asset/generation provenance. Canonical Dream Land observes at
least four water/Whispy swaps, zero native/failure cases, and first flags
`0x0200 -> 0x006b`. The renderer resolves the source tile-6/TMEM-0x40 TEXEL1 and
tile-7/TMEM-0 TEXEL0 independently, recognizes exact `G_CC_TEMPLERP`, and
precomposes its CI4 pair with a DS RGBA5551/A1 approximation. Each change builds
all 256 palette-pair RGB/coverage values; addressing and Bayer A1 stay exact. Compatible
animated state refreshes resident VRAM with frame pinning. The 184-frame gate
has positive scene-lifetime refresh, zero eviction/reject/oracle drift, and a
terminal `12/12` matched frame. Pond detail is `46.053%/23px`, versus white
`27.997%/105px`.
Fox's DXT tail fix remains; Tyler accepts the water. Persistent stage RSP cache
plus `G_MWO_POINT_ST` restores five flower groups (`192 -> 202` triangles);
Tyler accepts flowers/fences. Forensic lane/oracle proof is `37200/37200`, `2484/0/0`.
The immutable 300x220 wallpaper decodes once; imported `grWallpaperCalcPersp`
still owns its position/scale every tick. HW now composes the proven opaque
source directly into final 256x192 BG2 with the exact old draw-then-nearest map.
Its key covers provenance/epoch, live transform, combine/mapping, and BG2
ownership; no composed gameplay frame is cached. Unsupported layouts retain
the generic path. Canonical proves `direct67/skip0/change67`, exactly
`67*49152` pixels, and zero staging, BG2 clear/copy, or BG3 full-clear traffic.
The CI4 table first cuts present to `20,285,888`; command-hoisted light and
compile-time profile separation reduce shipped/profile-0 work further. Matrix
loads now use nonzero traversal generations instead of two 64-byte compares.
Hybrid submission sends `648` ordinary source-Z triangles through corrected GX.
Each persistent 32-slot RSP cache owns a bounded 64-entry composed-matrix table
plus per-slot matrix/clip IDs. Profile 0 turns `821` source loads into `282`
lazy transforms and `258` hits; snapshot create/reuse/overflow is `67/7/0`.
The `44` stale-slot triangles are genuinely mixed, so raw-snapshot stays zero;
`126` no-Z and `10` range exceptions also stay projected. Divisions/batches/
loads remain `1,242`, `121/707/121`, and `53`. Profile 2 transforms `821/821`;
oracle `2484/0/0`, device `PosTest 32/0/e1/w0/c0/mw1/drop0`, and depth pass. Eight warm frames improve present median/p95
`15,543,456/15,804,544 -> 13,301,312/13,595,328` (`-14.4%/-14.0%`); pacing is
`25/25 x0.1`. Capture: `artifacts/visibility/2026-07-11_canonical_fast_205024-5917438-p10196.png`.

The memory pre-breadth gate has a live VSBattle ledger and scene-owned reloc cache eviction. Mode `163` reports headroom `236100`, resident reloc `681632`
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
live pads before each update, and has hard GX RAM, display-contract, profile-0,
and screenshot gates; a same-source profile-2 run owns oracle correctness.
DevFast incrementally builds it, runs one capture pass, rotates
`latest.png` to `previous.png`, and requires exact canonical/shipped ROM parity.
The scripted mode-163 ROM remains an internal `-fast-hwtri` target; three
user-facing filenames represent only two unique configurations.
Visible melonDS launch/capture now force the natural equal-size two-screen
layout; the canonical lower screen retains its three bootstrap status rows.

Modes `161/162` remain bounded scaffolding; `battle_playable` is the scene-level
anchor. Obsolete mode/verifier stacks are migrate-or-delete with one
`[coverage-reduced]` line; modes `57/58` and `159/160` are already gone.

Fresh profile 1 is `present=13,301,536/13,563,968` median/p95; DL traversal at
`9,627,840/9,632,832` remains hot. Next: validated immutable-topology packet replay; source RGBA4 HUD is separate debt.

## Verification

Fresh P1Gate/Boundary pass in `281.4s/167.0s`: opening-to-Title, canonical battle, mode-163 combat, and one-minute Results—not the five-minute soak. Full Regression stays skipped.

```powershell
.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3
.\scripts\verify-p1-gate.ps1 -DelaySeconds 3
.\scripts\verify-boundary.ps1 -DelaySeconds 3
```

After verified progress, commit, then run `.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean` last.
