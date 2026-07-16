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
419,328/419,392 to 402,560/402,624 and draw P50/P95 from
1,245,024/1,247,616 to 1,230,336/1,232,832. ITCM is 25,384/32,768; the exact
70/686 fast owner and 60/320/306/29/0/0 partition remain unchanged, and the
top-screen A/B is pixel-identical. Next address the measured 53,824-tick local
matrix builder without copying/patching a whole FIFO packet, replacing the exact
shade path, or adding another per-root interpreter.

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

On the current bitmap-OAM configuration it moves stage P50/P95 from
619,744/619,904 to 577,440/577,536, saving 42,304/42,368 ticks. The 500K point
remains the next milestone target, not a discard gate; correct measured gains
accumulate. Incremental no-Z matrix transport remains reverted because it
regressed, and the signed-16 rounding treatment remains reverted because its
visual packet was invalid. M3 remains REWORK.

## M4 — Pre-GO Texture Residency

The current static corpus is the only P1 design:

- 17 source blocks, 22 keys, 21 deduplicated outputs;
- 126,976-byte payload and 131,072 prepared bytes in VRAM A;
- frozen water contributes 36,864 bytes on original runs 42-43;
- scene setup prepares and pins the set before GO;
- gameplay binds resident records only.

BattleShip changes Whispy's mouth and eye material image IDs during the match
(`grpupupu.c:565-623`). Those cosmetic frames are not part of the pinned
22-key corpus. The DS representation therefore reuses the already-resident
pre-GO source image when the primary image pointer is the sole difference in
the exact 59-word key. It never changes gameplay state, animation timing,
geometry, texture dimensions, palette, combine mode, tile state, or sampling.
If any other key field differs, the owner still fails closed. This is the
accepted 90% visual approximation; do not replace it with gameplay conversion.

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
