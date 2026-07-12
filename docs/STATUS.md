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
large tiles, profiles 0/1 index the first identical TEXEL0/TEXEL1/phase class
through a half-full 1 KiB table, expand forward from each first representative,
then copy repeated rows bottom-to-top.
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
accepted ranges. Each list directly uses the persistent 32-slot input/clip/color/
snapshot planes; validity masks reset compact control state. Null-callback profiles carry only segment-`E` preview resolver state, reset exact traversal guards/hardware totals, and publish fighter triangles once per owner; detailed/profile-2 ledgers remain unchanged. Profile 1 remains `80/1736/344/330` immutable/trusted/validated/adjacent-TRI work. Each run reuses exact material/depth, RGB15, S/T, projected X/Y, and clip Z.
Texture preparation persists through VTX/matrix and invalidates only at exact key mutations. Profiles 0/1 reuse alpha/poly-format only when the blend/combine classifier proves it vertex-independent; profile 2 stays generic.
The animated CI4 palette-pair
LUT remains content-keyed with sixteen exact 4x4 coverage planes. Profiles 0/1
decode the two immutable 32x32 packed source-index planes once, keyed by
pointer/texel count/lane and invalidated before reloc scene storage is reused.
Live tile origins, masks, palette/fraction, phase lookup, and uploads stay
dynamic. Profile 1 proves `2/728` index build/reuse; profile 2 stays bytewise
with index/map `0/0` and `0/0`.

Canonical mode 163 alone keeps `-O2`; larger scripted/lifecycle diagnostics stay
`-Os` and retain `227392` bytes of headroom. Mode 163 compiles the renderer in
ARM state; six measured O3 paths live in ITCM. Aligned VTX payloads decode into
the persistent cache. Light direction and four 128-step RGB tables persist to
exact invalidation; profile 2 omits the 2,096-byte table and runs the independent
exact shade calculation. CI4 maps/class indexing add 1,536 bytes. Profiles 0/1
index exact texture keys through 128 byte slots and compact fingerprints; full
236-byte equality remains the oracle and deletion repairs clusters.
Profile-0 BSS is `1,857,736`; canonical/profile-1/profile-2 ITCM is `20,916/21,376/18,196`, all below 32 KiB. Submission stays `648` raw source-Z, `44`
mixed-matrix, `126` no-Z, `10` range, `1,242` logical divide demand, and
`121/707/121` batches. Signed pre-clamping plus DS `div64` removes the shipping software 64-bit helper. Profile 1 makes `650` cache-miss calls; profile 2 checks
`1,404` evaluations against C with zero mismatch. Final hash lookup is
frame-dependent but conserves active/table/miss results with bounded probes.
Profile-1 median/P95 is draw `2,423,168/2,473,984`, vertex `459,168/460,736`, setup `748,576/799,104`, scan `652,352/653,184`.
Repeated shipping O2 is `2,044,640/2,046,080`; pacing is `12.3fps`.
Forensic oracle remains `2484/0/0`. Capture: `artifacts/visibility/2026-07-12_canonical_fast_121423-0241726-p3736.png`;
shipped SHA-256: `5E502A39FB46000176E2EA3BA54511EEB39AE9068DC666104991BF9AEFE51872`.
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

The canonical frame is still only `12.3fps`, far below the 60 FPS P1 condition.
The O2-equivalent coarse profiler conserves the whole loop and the 128-frame
profile-2 trace is exact. A 5,712-byte/91-op prepared Dream Land layer-0 owner
regressed draw by 12,640 ticks and was reverted; see `docs/PERF_LEDGER.md`.
The next compiler cut must amortize across a materially larger frame slice.
RGBA4 HUD, Whispy face strips, and Mario facing/light A/B remain debt.

## Verification

Final P1Gate/Boundary passed in `195.4s/58.3s`; DevFast and forensic also pass. This is not the five-minute soak; Full Regression stays skipped.

```powershell
.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3
.\scripts\verify-p1-gate.ps1 -DelaySeconds 3
.\scripts\verify-boundary.ps1 -DelaySeconds 3
```
After verified progress, commit, then run `.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean` last.
