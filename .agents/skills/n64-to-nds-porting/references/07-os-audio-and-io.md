# 07 — Replace source services at their purpose

| Source dependency | Target boundary |
|---|---|
| SP/DP tasks and completion | Native render/audio owners with explicit release points. |
| VI/swap | Canonical simulation clock plus target presentation. |
| PI reads/DMA | Asset IDs and the real DS load/stream stack. |
| Threads/message queues | Existing native workers or bounded state machines preserving order. |
| Cache/physical-address macros | Reevaluate actual target producers/consumers; no mechanical rename. |
| Controllers/accessories/saves | Normalized input and explicitly supported persistence/accessory behavior. |

ARM7 is not a replacement RSP. Preserve the selected runtime's services and offload only when compute/communication/cache/lifetime economics justify it. A queue may encode required deferred order even in a nominally single-threaded port; simplify machinery without deleting that behavior. Never wait in IRQ context for work requiring that IRQ or a blocked owner.

## Audio conversion

Preserve instruments, note/tempo timing, pitch, envelope/pan, voice priorities/stealing, loop state, cancellation and sample lifetime. Decode source compressed samples with the actual predictor/codebook/loop metadata, then encode a supported target format. N64 ADPCM and DS ADPCM are not interchangeable streams.

Name loop/length units: source samples, decoded samples, encoded bytes, blocks or target API units. Re-encoding can move legal boundaries; verify loop sound and pitch rather than applying a guessed byte ratio. Shared immutable samples do not merge distinct voice/envelope state.

Prerendered music is appropriate only when it preserves required dynamic layers, tempo and transitions. Streaming trades CPU for bandwidth and buffers; muting or dropping required cues is not native-port completion. Use the companion for actual sound/PXI/IRQ/cache contracts.

## Saves and shutdown

Use a versioned, bounded, explicit-endian save format rather than raw structs or source pointers. Preserve progress/unlocks/settings and distinguish unsupported accessories from corruption. Translate the payload instead of recreating a controller-pak filesystem unless needed. Keep previous valid data until a supported commit succeeds; desktop rename assumptions are not a DS durability guarantee.

Scene teardown needs GX, texture, audio, DMA, workers and file-request completion, not only source simulation quiescence. Advance generations, detach/cancel or drain, then release after external consumers are safe. A late completion must not install state in a new scene. [Sources](SOURCES.md).
