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

Current Mode 8 is visually correct but over target. The next candidate must
eliminate immutable per-frame command work without copying/patching a whole FIFO
packet or adding another per-root interpreter.

First keep gate:

- save at least 80K combined fighter ticks and reach <=337,472;
- matrix plus lighting <=120K and transport <=145K;
- exact geometry, owner state, texture traffic, screenshot, and reserve;
- no fallback, allocation, or ledger-on-only win.

Final promotion remains 170-250K ledger-off.

## M3 — Complete Dream Land Owner

Target: complete stage rendering in 150-250K ticks.

Retained owner contract:

- eight callbacks, mask 255, 57 DObjs, 42 bindings;
- 54 runs, 202 triangles, 49 epochs, four material commits;
- projected cross-matrix counts 5/10/15;
- source links remain segmented around fighters and weapons;
- Whispy, flowers, draw flags, materials, effects, weapons, depth, and stage
  selection remain live; water pixels alone are frozen.

Current stage-exclusive P50/P95 is 664,544/664,640. The bounded dense-index
prepare-once experiment proved zero conflicts and retained exact semantics:

```text
606 corner references -> 312 dense vertices
408 projected references -> 246 unique
remove 294 repeated attribute preparations
remove 162 repeated transforms and 486 projections
```

It measured 555,584/555,776, only 108,960/108,864 better than A. That missed the
required 164,544 saving and <=500K P50 gate, so the source/checker change was
reverted. Do not retry or widen dense-only preparation reuse. M3 remains REWORK;
select a different measured internal bucket and retain the same gate.

## M4 — Pre-GO Texture Residency

The current static corpus is the only P1 design:

- 17 source blocks, 22 keys, 21 deduplicated outputs;
- 126,976-byte payload and 131,072 prepared bytes in VRAM A;
- frozen water contributes 36,864 bytes on original runs 42-43;
- scene setup prepares and pins the set before GO;
- gameplay binds resident records only.

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
