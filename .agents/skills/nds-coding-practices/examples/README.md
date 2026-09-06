# Reusable Nintendo DS Patterns

Use the smallest pattern that fits. The main examples target Calico-based
devkitPro libnds 2.x; the consuming project's installed headers and owners remain
authoritative. Do not transplant these calls into a different SDK generation.

| File | Role |
|---|---|
| `frame_loop.c` | Standalone loop shape: input once, update/prepare/submit/commit separation. Rendering callbacks are intentionally placeholders. |
| `sprite_oam.c` | Standalone sprite demo: OBJ VRAM, explicit halfword writes, shadow OAM, bounded commit. |
| `tiled_background.c` | Standalone BG demo: non-overlapping tile/map intervals, halfword writes, wrap-bounded scroll. |
| `gx_frame.c` | Standalone GX triangle: setup-time projection, explicit draw state, balanced matrices, one finalization. |
| `dma_cache.c` | Reusable ARM9 helpers, **not a main program**: checked async publication and owned inbound cache lines. |
| `video_copy16.h` | Small CPU uploads with explicit volatile halfword stores and zero-count no-op. |
| `fixed_math.h` | Portable arithmetic reference with explicit negative rounding; not the default hot ARM9 divide implementation. |
| `irq_handoff.c` | Standalone same-CPU IRQ snapshot with a short critical-section copy, not a cross-CPU protocol. |
| `irq_worker.c` | Standalone Calico IRQ-to-worker demo: coalescing mailbox, lower-priority blocking worker, TLS, stop/join. |
| `pxi/arm9.c`, `pxi/arm7_service.c` | Matching request/reply **components**, preserving an existing compatible core. Read [`pxi/README.md`](pxi/README.md) before integration. |
| `pxi/protocol.h`, `pxi/demo.h` | Small value protocol and component declarations. |
| `shared_mailbox.h` | Advanced layout-only example for raw shared memory. Not a native Calico `Mailbox` and not a complete transport. Prefer PXI for small messages. |

Standalone demos each own their display/IRQ resources. Build one at a time;
do not link their `main` functions together. Replace hard-coded banks, channels,
and initialization with existing project owners when integrating. Do not replace
a full ARM7 service core with the PXI component file.

DMA helpers leave actual allocation capacity, memory placement, physical range
non-overlap, and exclusive channel ownership to the caller. Their zero/alignment/
count guards do not prove those facts. The default Calico main stack is in DTCM;
never use a stack DMA buffer merely because these C guards pass.

The IRQ worker's one-slot queue coalesces wake hints by design. It is not suitable
for lossless input events or buffer-completion records without a different queue
policy. The PXI example permits one request in flight; a full default PXI mailbox
can lose data, and synchronous request/reply has no timeout argument.

## Validation

Run `python3 tests/run_host_tests.py` for pure logic/layout and mocked DMA/cache
call contracts. `tests/run_clang_codegen.py` inspects portable helpers with a
freestanding ARM target when Clang is available; it is not an SDK build.
`tests/run_target_checks.py --out /path/to/outputs` compiles examples and explicit
helper probes against an installed current devkitPro SDK. That script is
compile-only: complete NDS linking, paired-CPU startup, visual tests, actual DMA/
cache behavior, and timing still need the application's normal build/run path.

See `../tests/REVIEW_RESULTS.md` for what was actually run for this release.
Maintenance tests and evaluation cases are not required context for every task.
