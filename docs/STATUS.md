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
Latest canonical reports `gxram=715/2167`, geometry `0x222005`, source cycle/
render `0x00100000/0xc4112078`, parts `14/18`, and zero oracle mismatches.
Source-depth X/Y/Z share one composed clip vertex. Projected no-Z depth has
source-backed background/foreground phases around the first source-Z triangle,
restoring layer-3 foreground fences over layer-1 floor/path.

`gcAddMObjAll` normalizes mixed-width O2R lanes by source provenance. Dream Land
observes at least four water/Whispy swaps and zero native/failure cases. Exact
S/T maps, packed CI4 reads, and 17 Bayer phase masks preserve live water. For
large tiles, profiles 0/1 evaluate only the first identical TEXEL0/TEXEL1/phase
class, expand each row right-to-left, then copy repeated rows bottom-to-top.
Profile 1 proves `545086/2485954` representative/reused pixels; smaller tiles
and profile 2 retain the direct loop. The two uploads remain `36,864` bytes with
zero eviction/reject/oracle drift. Pond detail remains accepted.
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
lists keep per-command validation. Live stage/fighter validators test the
taskman arena before walking the loaded-file ledger, preserving the same
accepted ranges. Profile 1 remains `80` immutable lists, `1,736` trusted
commands, `344` fallback validations, and `330` adjacent TRI commands. Each run reuses exact material/depth state, RGB15 colors, scaled S/T, projected X/Y, and source clip Z. Each non-TRI closes GX and invalidates per-vertex derivatives;
texture preparation persists through VTX/matrix commands and invalidates only
at exact texture/material/depth-key mutations. The animated CI4 palette-pair
LUT remains content-keyed with sixteen exact 4x4 coverage planes. Profiles 0/1
decode the two immutable 32x32 packed source-index planes once, keyed by
pointer/texel count/lane and invalidated before reloc scene storage is reused.
Live tile origins, masks, palette/fraction, phase lookup, and uploads stay
dynamic. Profile 1 proves `2/720` index build/reuse; profile 2 stays bytewise
with index/map `0/0` and `0/0`.

Canonical mode 163 alone keeps `-O2`; the larger scripted/lifecycle diagnostics
stay `-Os` and retain `227392` bytes of headroom. Mode 163 compiles the renderer
TU in ARM state; six measured O3 paths live in ITCM. Aligned VTX payloads use
guarded may-alias word loads with bytewise fallback and decode directly into the
persistent cache. Light direction survives adjacent VTX commands until exact
matrix/MOVEMEM invalidation; four 128-step RGB tables key source diffuse/ambient
colors while normals/direction/alpha remain live. The table occupies 2,096
bytes. CI4 representative maps add 512 bytes; profile-0 BSS is `1,856,200`.
Profile 0/1/2 ITCM is `20,448/20,888/18,216`, below 32 KiB; profile 2 keeps its generic/oracle route.
Submission stays `648` raw source-Z, `44` mixed-matrix, `126` no-Z, `10` range,
`1,242` divisions, and `121/707/121` batches. Representative reuse lowers the
matched profile-1 P95 draw `3,585,792 -> 3,491,392`, DL
`2,430,080 -> 2,350,976`, texture `509,376 -> 439,680`, setup
`988,608 -> 915,968`, and scan `950,720 -> 939,072` ticks. Profile 0's best
benchmark remains `8.7fps`; final DevFast is `8.6fps`.
Forensic oracle remains `2484/0/0`. Capture:
`artifacts/visibility/2026-07-12_canonical_fast_054624-3345757-p21976.png`;
shipped SHA-256: `832566CCEA53F6E6DD7195DDBFCEB0164EDC1F11594D376A5244F42B6A15C76E`.

The memory pre-breadth gate has a live VSBattle ledger and scene-owned reloc cache eviction. Mode `163` reports headroom `227392`, resident reloc `681632`
bytes (`stage=202816`, `fighter=175440`, `if=208672`), stale `0/0`, and source
VSBattle buffers from `scvsbattle.c:31-41`. Audio `.ctl` parsing now peaks at
`16` bytes of scratch. After the 8,704-byte scene matrix cache and 64 KiB BGM,
`161856` bytes remain against the 128 KiB reserve.

## Current Notes

The taskman allocator's `0x140000`/`0x130000` fallbacks preserve the reserve.
Pupupu map objects decode cleanly; Mario/Fox enter Wait on lines `3/2` at X `0/-1397`.

Canonical realtime + live-input + HW-tri has hard GX RAM, display-contract,
profile-0, screenshot, and ROM-parity gates; profile 2 owns oracle correctness.
DevFast builds it, captures once, and rotates `latest.png` to `previous.png`.
The scripted mode-163 ROM remains internal; three user-facing names represent
two configurations. Both melonDS LCDs render; the canonical lower screen is
black except for three bootstrap status rows.

Modes `161/162` remain bounded scaffolding; `battle_playable` is the scene-level
anchor. Obsolete mode/verifier stacks are migrate-or-delete with one
`[coverage-reduced]` line; modes `57/58` and `159/160` are already gone.

The canonical frame is still only `8.6fps`, far below the 60 FPS P1 condition.
Profile-1 scan/setup remain about `0.94M/0.92M` ticks; next profile command-state
dispatch and exact TRI-run setup before considering a larger packet cache.
RGBA4 HUD, Whispy face strips, and Mario facing/light A/B remain debt.

## Verification

P1Gate/Boundary passed in `259.9s/127.8s`; DevFast/rebuilt forensic passed in
`76.9s/49.8s`. This is not the five-minute soak; Full Regression stays skipped.

```powershell
.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3
.\scripts\verify-p1-gate.ps1 -DelaySeconds 3
.\scripts\verify-boundary.ps1 -DelaySeconds 3
```
After verified progress, commit, then run `.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean` last.
