# Native Renderer Implementation Contract

`docs/P1_EXECUTION_BOARD.md` owns priority and current state;
`docs/PERF_LEDGER.md` owns measurements and rejected experiments. This file owns
only the retained M2-M4 implementation contract.

## Shared Rules

- Preserve BattleShip draw order, selected display lists, material progression,
  matrix ownership, lighting inputs, texture identity, depth, and live state.
- Validate a whole owner before its first GX mutation. Unsupported state falls
  back before partial submission.
- Build immutable topology before gameplay; bind only live matrices, materials,
  colors, lights, and selection state per frame.
- Use fixed-capacity storage. No gameplay heap allocation, conversion, file I/O,
  texture upload preparation, or speculative cache.
- Keep profile 2 as the independent semantic oracle.
- Presentation follows the 90% DS rule; gameplay-relevant geometry and state do
  not. Dream Land water is exact frame 0/fraction 114 on its original triangles.
- Use the eight-frame A/B/A2-if-needed policy in `docs/VERIFYING.md`.

## M2 — Mario/Fox Owner

Target: combined fighter rendering in 170-250K ticks.

Retained production contract:

- 32 roots, 49 epochs, 67 runs, 626 triangles, 541 immutable vertices.
- Mario/Fox ownership remains 320/306 triangles.
- Persistent vertex-cache and cross-matrix semantics remain exact.
- CPU lighting remains source-exact; hardware lighting is not eligible without
  an independent exact oracle.
- Preflight rejects active animlock or unsupported dynamic matrix/material state
  before GX output.

Current Mode 8 is visually correct but over target. Intermediate gains
accumulate; 170-250K is the milestone target, not a per-cut discard gate. Keep
each repeatable ledger-off reduction that preserves exact geometry, owner state,
texture traffic, screenshot, reserve, and conservation without adding fallback
or allocation.

The retained native-fighter ITCM placement moves combined fighter P50/P95 from
419,328/419,392 to 402,560/402,624. Exact power-of-two quantization in the
source-backed local matrix seam then moves it to 398,048/398,144. Hoisting the
raw/textured decisions out of the complete 582-triangle production corner loop
moves it to 397,248/397,312. Aggregating raw/cross accounting once per owner
traversal moves it to 395,264/395,328. AOT-packing the immutable GX `xy/z`
words without growing the 16-byte dense record moves it to 386,880/386,944.
Co-locating the emitted AOT words with prepared color/UV in one power-of-two
record moves it to 384,000/384,000 without changing total fighter-table RAM.
The pre-light-repair post-Task-9 ledger-off sample was 372,096/372,160 and is
no longer current. Splitting the production
emitter by its already-proved raw/cross run class reduces the synchronized
detailed combined-fighter window 433,472/433,536 -> 432,384/432,448 and draw
1,061,888/1,061,952 -> 1,060,928/1,060,992. Raw runs now save two registers
per entry/exit; the rarer cross runs add one, eliminating 190 main-RAM stack
word transfers per frame. Canonical ITCM is 28,052/32,768 after Task 9, the
source-light repair, the raw-corner cut, and the raw texture-class split; the exact 70/686 fast owner and
60/320/306/29/0/0
partition stay unchanged. Full inlining and a shared tail dispatcher are
measured regressions.

The current source-light-exact ledger-off checkpoint is 385,312/388,480.
Capture now resets only live scalar state because every consumed event field
and scratch command is overwritten before use; it no longer clears the
6,240-byte arena twice per frame. Detailed capture moves 47,296/47,360 ->
41,152/41,152, combined fighter 452,640/455,808 -> 446,464/449,600, and draw
1,077,568/1,080,832 -> 1,071,488/1,074,816. The change adds 64 bytes of main
code, no ITCM, and preserves exact pixels, geometry, owner state, texture
traffic, and conservation.

Raw production corners now store only their already-proved 10-bit dense vertex
ID; only the 132 cross-run corners retain a packed matrix slot. This removes
1,746 dynamic masks plus one saved register at each of 54 raw-run entries and
exits. The raw emitter shrinks 0xD0 -> 0xBC bytes, cross remains 0x164, and the
synchronized ledger-off window improves 386,016/389,184 -> 385,088/388,224
without changing pixels, geometry, owner state, or texture traffic.

The two already-specialized raw loops now have separate textured and
untextured callees. The immutable corpus proves 43 untextured and 11 textured
raw calls per frame. The common untextured path saves `r5` and `r6` at entry
and exit, removing 172 main-RAM stack word transfers per frame. The old 0xBC
combined symbol becomes 0x74 textured plus 0x64 untextured symbols; the owner
grows 0xB98 -> 0xB9C and ITCM grows 32 bytes. On the synchronized current-build
window, combined fighter improves 386,624/389,824 -> 385,312/388,480 and draw
1,011,648/1,014,976 -> 1,009,824/1,013,120. All eight paired frames preserve
pixels, geometry, owner state, texture traffic, and conservation.

The generic/fast profile-2 oracle is exact again. F3DEX2 `G_MOVEWORD` uses
index bits 16..23 and offset bits 0..15. The generated owner retains all 148
static fighter `G_MW_LIGHTCOL` commands as 120 compact root preambles plus 28
intra-root epoch-state changes, and validation admits that state effect only
for an exact light-color opcode/index/offset tuple. Fresh frames 180..187 have
zero semantic, owner, or geometry mismatches and 686 triangles in both arms.
Any future M2 change must be another measured production cut from this contract.

## M3 — Complete Dream Land Owner

Target: complete stage rendering in 150-250K ticks.

Retained owner contract:

- eight callbacks, mask 255, 57 DObjs, 42 bindings;
- 54 runs, 202 triangles, 49 epochs, four material commits;
- projected cross-matrix counts 5/10/15;
- source links remain segmented around fighters and weapons;
- Whispy, flowers, draw flags, material state, effects, weapons, depth, and
  stage selection remain live. Water pixels are frozen at source frame 0, and
  an unprepared post-GO Whispy material image reuses its first resident source
  frame only when every non-image word of the complete renderer key matches.

The retained no-Z codegen correction and dense-index prepare-once path both
keep exact pixels and semantics. The dense contract is:

```text
606 corner references -> 312 dense vertices
408 projected references -> 246 unique
remove 294 repeated attribute preparations
remove 162 repeated transforms
```

The current bitmap-OAM path also AOT-packs the immutable GX coordinate shift in
the existing cache byte, removing the per-triangle search without growing the
12,663-byte packet. Its same-ROM A/B moves stage P50/P95 from
578,272/578,560 to 556,256/556,352. Routing the 118/146 exact zero-shift matrix
loads through the existing raw builder then moves stage to 541,952/542,272 and
draw to 962,816/962,880. Exact bounded `s16` vertex rounding removes the generic
64-bit shift sequences and moves the current same-ROM stage window from
545,440/545,536 to 536,032/536,256 with a 0/49,152 native-pixel delta. The 500K
point remains the next milestone target, not a discard gate; correct measured
gains accumulate.

Task 23R Phase 0 closes the consumed-field surface around Task 26. Its generated
checker binds 588 pointer-field accesses in 36 production closures, including
live texture-cache records and every callback-visible output base. Task 26 then
retains one exact fixed generated program for segment 0 / `layer0`: 21 DObjs,
bindings 0–19, runs 0–25, 54 triangles, 22 epochs, 108 dense vertices, 123 state
effects, and 90 synchronization effects. It consumes generation-validated
asset bases, 42 per-frame composed matrices, four per-frame material snapshots,
and current renderer config without a runtime opcode scan, packet copy/patch,
per-frame list construction, sorting, or second topology cache. Existing
prepared storage, matrix and near-plane work, material/texture/color/alpha/UV
selection, validation, commit loop, GX emitters, and fail-closed fallback before
GX remain live.

The current/generated CPU trace matches all 2,775 words and 26 rows over eight
frames; forced live mutation records inject/mismatch/revalidate `1/1/1` before
GX. Five Task-25R-control phases save 3,424–3,616 stage P50 ticks without a
relevant P95 loss, and synchronized pixels remain `0/49,152`. A hardware-style
melonDS pair saves 10,240/10,368 stage P50/P95 ticks; the user's single retail
A/B saves 21,568 draw ticks while loop remains in the same VBlank bucket. That
retail observation is not a repeatability claim. Because the generated working
set cannot be widened under the current no-repeat device constraint, controlled
Task 26 expansion stops while the exact smaller win remains banked. Task 23R
Phase 1 may now remeasure only residual work and may retain a cache only at
least 20% exact complete-key hits with key cost below half the avoided work.

Incremental no-Z matrix transport remains reverted because it regressed. M3
remains REWORK after the retained Task 26 segment-0 slice.

## M4 — Pre-GO Texture Residency

The current static corpus is the only P1 design:

- 19 source blocks, 24 keys, 23 deduplicated outputs;
- 132,096-byte payload and 136,192 prepared bytes across VRAM A+B;
- frozen water contributes 36,864 bytes on original runs 42-43;
- scene setup prepares and pins the set before GO;
- gameplay binds resident records only.

BattleShip changes Whispy's mouth and eye material image IDs during the match
(`grpupupu.c:565-623`), and late Fox material progression selects a second
runtime-observed source image. Both exact source assets are now generated and
pinned as keys 152 and 313 before GO. The exact-key checker reports 24 hits, 23
deduplicated outputs, 1,344 classified field misses, three explicit misses, and
six invalid-key falsifiers. Gameplay performs no conversion, decode,
allocation, GL create/upload, fallback, refresh, or fence violation across the
complete one-minute lifecycle; one teardown releases both banks. Do not restore
the former primary-image approximation or any gameplay conversion path.

The animated tiled-water implementation, asset, generator, draw path, residency
path, checks, and build selector are deleted. Do not recreate them.

M4 promotion requires one natural GO-to-battle-teardown run with zero conversion,
decode, allocation, file I/O, GL create/upload/delete, eviction, refresh,
manifest fallback, or fence violation; exactly one teardown; and at least 128 KiB
reserve. Results may prewarm at its own setup boundary.

## Integrated Promotion

After an owner clears its local gate, profile the whole frame: update, audio,
input/HUD, renderer owners, texture work, flush, VBlank wait, and residuals.
Keep only changes whose ticks/FPS, semantic counters, runtime state, memory, and
screenshot analysis agree. Compilation alone proves nothing.
