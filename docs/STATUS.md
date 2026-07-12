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
Latest canonical reports `gxram=733/2219`, geometry `0x222005`, source cycle/
render `0x00100000/0xc4112078`, parts `14/18`, and zero oracle mismatches.
Source-depth X/Y/Z share one composed clip vertex. Projected no-Z depth has
source-backed background/foreground phases around the first source-Z triangle,
restoring layer-3 foreground fences over layer-1 floor/path.

`gcAddMObjAll` normalizes mixed-width O2R lanes by source provenance. Dream Land
observes at least four water/Whispy swaps and zero native/failure cases. Exact S
maps, row-local T addresses, packed paired CI4 reads, and 17 Bayer phase masks
replace the TEXEL0/TEXEL1 pixel-generic hot path; all other cases retain it. The
two dynamic uploads cover `36,864` bytes with positive direct-pixel/refresh proof
and zero eviction/reject/oracle drift. Pond detail remains accepted.
Fox's DXT tail fix remains; Tyler accepts the water. Persistent stage RSP cache
plus `G_MWO_POINT_ST` restores five flower groups (`192 -> 202` triangles);
Tyler accepts flowers/fences. Forensic lane/oracle proof is `37200/37200`, `2484/0/0`.
The immutable 300x220 wallpaper decodes once; imported `grWallpaperCalcPersp`
still owns its position/scale every tick. HW now composes the proven opaque
source directly into final 256x192 BG2 with the exact old draw-then-nearest map.
Its key covers provenance/epoch, live transform, combine/mapping, and BG2
ownership; no composed gameplay frame is cached. Unsupported layouts retain
the generic path. Canonical proves one `49152`-pixel write per changed frame and
zero staging, BG2 clear/copy, or BG3 full-clear traffic.
Reloc-backed source DLs now expose one immutable byte span; dynamic task-heap
lists keep per-command validation. Profile 1 proves `80` immutable lists,
`1,736` trusted commands, `344` fallback validations, and `330` adjacent TRI
commands replayed through the bounded run path. Each unchanged TRI run also
reuses exact material/depth state, RGB15 colors, scaled S/T, projected X/Y, and
source clip Z. Each non-TRI closes GX and invalidates per-vertex derivatives;
texture preparation persists through VTX/matrix commands and invalidates only
at exact texture/material/depth-key mutations. The animated CI4 palette-pair
LUT is content-keyed and expanded into sixteen exact 4x4 coverage phase planes.
Only `2,048/18,432` pixels were pairable, so the regressive pair branch is gone.

Canonical mode 163 alone keeps `-O2`; the larger scripted/lifecycle diagnostics
stay `-Os` and retain `227392` bytes of headroom. Mode 163 compiles the renderer
TU in ARM state, while four measured O3 loops live in ITCM. One 40-byte exact
context supplies each three-vertex submission instead of restaging 22 arguments
three times; its eight Boolean modes share one flag word. The 8 KiB phase LUT
adds 7,168 net BSS bytes; renderer ITCM is `14,068/14,212/14,128` bytes for
profiles 0/1/2. Profile 2 retains its independent generic/oracle route.
Submission stays `648` raw source-Z, `44` mixed-matrix, `126` no-Z, `10` range,
`1,242` divisions, and `121/707/121` batches. Immutable-packet and redundant
GX-attribute experiments were measured and reverted because neither reduced
the frame. A matched profile-1 A/B cuts draw `4,473,120 -> 4,164,800`, DL
`3,317,280 -> 3,017,888`, texture `908,192 -> 595,424`, and setup
`1,385,856 -> 1,074,912` ticks. Final present is `4,336,864/4,592,960`; scan
is `1,432,864/1,433,856` and vertex `509,760/511,360`. Profile 0 reaches
`7.5fps`; draw is `3,983,296/3,984,640`.
Forensic oracle remains `2484/0/0`. Capture:
`artifacts/visibility/2026-07-12_canonical_fast_030326-3934155-p33036.png`;
shipped SHA-256: `00C9A6167F01E43950098854A56BFB271F1E3A1CEE2741EF7F14D38E24D397FB`.

The memory pre-breadth gate has a live VSBattle ledger and scene-owned reloc cache eviction. Mode `163` reports headroom `227392`, resident reloc `681632`
bytes (`stage=202816`, `fighter=175440`, `if=208672`), stale `0/0`, and source
VSBattle buffers from `scvsbattle.c:31-41`. Audio `.ctl` parsing now peaks at
`16` bytes of scratch. After the 8,704-byte scene matrix cache and 64 KiB BGM,
`161856` bytes remain against the 128 KiB reserve.

## Current Notes

The taskman allocator's `0x140000`/`0x130000` fallbacks preserve the reserve.
Pupupu map objects decode without duplicates/unaligned reads; original-manager
Mario/Fox enter Wait on lines `3/2` at X `0/-1397`.

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

The canonical frame is still only `7.5fps`, far below the 60 FPS P1 condition.
Profile-1 scan/setup remain about `1.43M/1.07M` ticks; next split source matrix/
state commands and VTX lighting. RGBA4 HUD, Whispy face strips, and Mario
facing/light A/B remain debt.

## Verification

P1Gate passed in `189.2s`; Boundary passed in `81.2s`. DevFast passed in
`44.5s` and the forensic oracle in `24.5s`. This is not the five-minute soak;
Full Regression stays skipped.

```powershell
.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3
.\scripts\verify-p1-gate.ps1 -DelaySeconds 3
.\scripts\verify-boundary.ps1 -DelaySeconds 3
```
After verified progress, commit, then run `.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean` last.
