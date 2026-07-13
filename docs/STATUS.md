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
Latest live-input canonical reports dynamic GX RAM near `695/2119`, geometry
`0x222005`, cycle/render `0x00100000/0xc4112078`, parts `14/18`, and zero oracle mismatches.
Source-depth X/Y/Z share one composed clip vertex. Projected no-Z depth has source-backed background/foreground phases around the first source-Z triangle,
restoring layer-3 foreground fences over layer-1 floor/path.

`gcAddMObjAll` normalizes mixed-width O2R lanes by source provenance. Dream Land
observes at least four water/Whispy swaps and zero native/failure cases. Exact
S/T maps, packed CI4 reads, and one 1 KiB RGB15/coverage table preserve live water.
Large tiles index the first identical TEXEL0/TEXEL1/phase class and expand X.
Cold uploads still materialize all rows in scratch. Warm large refreshes write
each unique row directly into the existing 16 KiB VBlank staging buffer; the
exact row map expands repeats only on lines `192..207`. Matched 128-frame draw
improves `2,001,600/2,033,664 -> 1,970,304/2,002,880`, with identical upload hash
and zero fallback. Profile 2 retains its independent synchronous oracle route
and reports `18,432/0` exact pair-pixel checks.
Fox's DXT tail fix remains; Tyler accepts the water. Persistent stage RSP cache
plus `G_MWO_POINT_ST` restores five flower groups (`192 -> 202` triangles);
Tyler accepts flowers/fences. Forensic lane/oracle proof is `37200/37200`, `2484/0/0`.
The immutable 300x220 wallpaper decodes once; imported `grWallpaperCalcPersp`
still owns its live transform. Final BG2 keeps two exact X/Y maps in the decode
buffer's unused tail, updates only changed rows/columns, and expands full dirty
rows there before cache-flushed DMA. No composed frame is cached or added to
BSS. Same-ROM 128-frame profile-1 draw moves `1926624/1955648 ->
1812256/1900288`; wallpaper moves `344672/348480 -> 237088/340032` while
stage/Mario/Fox stay flat. Profile 2 proves map/pixel `23296/0` and `2555904/0`.
Unsupported/changed ownership redraws fully; staging and BG2/BG3 clear/copy stay zero.
Reloc-backed source DLs now expose one immutable byte span; dynamic task-heap
lists keep per-command validation. Live stage/fighter validators test the
taskman arena before walking the loaded-file ledger, preserving the same
accepted ranges. Each list directly uses the persistent 32-slot input/clip/color/
snapshot planes; validity masks reset compact control state. Null-callback profiles carry only segment-`E` preview resolver state, reset exact traversal guards/hardware totals, and publish fighter triangles once per owner; detailed/profile-2 ledgers remain unchanged. Profile 1 remains `80/1736/344/330` immutable/trusted/validated/adjacent-TRI work. Each run reuses exact material/depth, RGB15, S/T, projected X/Y, and clip Z.
Texture preparation persists through VTX/matrix and invalidates only at exact key mutations. Profiles 0/1 reuse alpha/poly-format only when the blend/combine classifier proves it vertex-independent; profile 2 stays generic.
The animated CI4 palette-pair
LUT remains content-keyed with exact RGB plus one 16-bit 4x4 coverage mask. Profiles 0/1
decode the two immutable 32x32 packed source-index planes once, keyed by
pointer/texel count/lane and invalidated before reloc scene storage is reused.
Live tile origins, masks, palette/fraction, phase lookup, and uploads stay
dynamic. Profile 1 proves `2/728` index build/reuse; profile 2 stays bytewise
with index/map `0/0` and `0/0`.

Canonical mode 163 alone keeps `-O2`; larger scripted/lifecycle diagnostics stay `-Os`; granular 4 KiB arena fallback preserves their reserve. Mode 163 compiles the renderer in
ARM state; six measured O3 paths live in ITCM. Aligned VTX payloads decode into
the persistent cache. Light direction and four 128-step RGB tables persist to
exact invalidation; profile 2 omits the 2,096-byte table and runs the independent
exact shade calculation. CI4 maps/class indexing add 1,536 bytes. Profiles 0/1
index exact texture keys through 128 byte slots and compact fingerprints; full
236-byte equality remains the oracle and deletion repairs clusters.
Profile-0 BSS is `1,875,504`; ITCM is `20,088/20,088/18,584`. Submission stays
`648/44/126/10`, with `1,242` logical divides and `121/707/121` batches. Exact
DS `div64` makes 650 profile-1 calls; profile 2 checks 1,404 with zero mismatch.
The shared all-owner K-RAW kernel executes `45/540` immutable runs/triangles
(`60/246/234` stage/Mario/Fox) and falls back `47/7/0`. Same-ROM 128-frame
profile-1 draw improves `2,067,296/2,407,872 -> 1,858,624/2,227,648`.
Its 8 KiB table consumes 330 TRI commands. A 256-byte DObj index plus exact
affine product moves draw `2,126,752/2,169,600 -> 2,057,376/2,098,880`; profile
2 proves affine and vertex equality. Exact persistent stage worlds reuse `57`
nodes with `42/0` shadow samples/mismatches and zero reject/overflow.
Matched 128-frame draw moves `2,323,008/2,355,712 -> 2,263,616/2,280,512`;
matrix prep saves `67,328/41,088` and stage saves `63,424/82,176` ticks.
The memory pre-breadth gate has a live VSBattle ledger and scene-owned reloc cache eviction. Mode `163` reports headroom `227392`, resident reloc `681632` bytes (`stage=202816`, `fighter=175440`, `if=208672`), stale `0/0`, and source
VSBattle buffers from `scvsbattle.c:31-41`. Audio `.ctl` parsing now peaks at
`16` bytes of scratch. The bounded 48-texture/40-pos-test/832-event caches keep
the larger forensic gate at `202416` bytes before and `136880` after BGM,
retaining the required 128 KiB reserve.

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

Canonical is `14.8fps`; a 128-frame profile-0 sample is `1,690,176/1,867,392` draw ticks. Profile 1 is `1,812,256/1,900,288`, with wallpaper `237,088/340,032`. Current ROM SHA-256 is `5FFA613E500CED28B9630E7F90E30C7A5F129AA3112A49E6BBF8F49D344BFCC5`. The next renderer cut needs a coarse stage-owner kernel. HUD, Whispy face, and Mario facing/light remain debt.
## Verification

DevFast, forensic, P1Gate, and Boundary `161/162/163` pass; Full Regression stays intentionally skipped for fast iteration.

```powershell
.\scripts\verify-dev-fast.ps1 -Build -DelaySeconds 3
.\scripts\verify-p1-gate.ps1 -DelaySeconds 3
.\scripts\verify-boundary.ps1 -DelaySeconds 3
```
After verified progress, commit, then run `.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean` last.
