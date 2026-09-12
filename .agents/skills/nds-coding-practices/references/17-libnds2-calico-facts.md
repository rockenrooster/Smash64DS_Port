# 17 — Pinned libnds 2.x / Calico traps

These facts target the devkitPro baseline in [SOURCES](SOURCES.md), not every library called libnds. Inspect installed code before changing runtime internals. Hardware rules are in the focused references; this page collects easy-to-misremember API details.

## Build and memory

- Calico `ds9.ld` places ordinary ARM9 content from `0x02001000` and checks its end against `0x02380000`. Physical 4 MiB does not mean all of it is ARM9 heap; upper memory supports ARM7/runtime ownership.
- ARM9 ITCM is 32 KiB and DTCM 16 KiB. The baseline exposes `0x3e80` bytes of DTCM after reserved regions; DTCM data and the default main stack compete. Check the final map and remaining stack, not the section annotation alone.
- `__stacksize__` is a weak `u32` variable, not a function or macro. A request larger than remaining DTCM moves the main stack to heap in this startup path; a small override is not a universal relocation command. C++ overrides need C linkage.
- Old `memCached()`/`memUncached()` recipes and the `0x02400000` alias are not a supported Calico MPU shortcut. Cache/DMA ownership still applies; TCM cannot be made DMA-visible with a flush.
- libdvm replaces the old libfat/libfilesystem backend while retaining compatibility APIs. Do not infer the implementation from a legacy header name or take over ARM7 block-I/O services.

## Threads, IRQs and messages

- `threadPrepare()` takes an **8-byte-aligned stack top**, not base; the supplied storage must cover the required frame/stack. TLS/newlib state needs its own reservation/attachment contract and consumes memory. Do not claim a thread is libc-safe merely because it starts.
- Lower priority number is higher priority. No automatic timeslicing; `threadYield()` only yields to equal-priority runnable threads. Block/sleep to let lower-priority work run.
- IRQs have a separate stack, do not nest in this baseline, and are not thread/TLS context. Use documented nonblocking mailbox/wake operations only; do not allocate, lock, print, or wait there.
- Calico `Mailbox` stores 1–255 **words**, not bytes, and is same-CPU. A nonblocking try-send can fail; a callback-to-mailbox adapter does not make overflow impossible.
- PXI normal values carry 26 bits. Extended sends carry a 16-bit immediate plus 1–32 words. Use application `User0`–`User7` channels without replacing runtime service owners. Callbacks execute in IRQ context. Readiness/receive and send internals can wait; the example's one-outstanding-request assumption is material.
- Timers 2 and 3 are runtime-owned in this baseline. Reserve timers/channels centrally rather than copying legacy examples.

## DMA, GX and video

- `dmaCopy()` is a synchronous DMA3 halfword-copy wrapper with a byte count. `dmaCopyWordsAsynch()` has different width/completion semantics. Verify count granularity, channel limits, pointer domains and capacity in callers.
- `glCallList()` reads a leading payload-word count (header excluded), flushes the payload, waits for DMA activity to stop, submits via DMA0 and waits for that DMA. It does not finish rendering or release texture memory. Invalid/zero counts must be rejected before its debug assertions.
- `glTexImage2D()` uses `GL_TEXTURE_SIZE_ENUM` dimensions. `TEXTURE_SIZE_8` is an enum value, not the integer 8 pixel count. Do not substitute another SDK's pixel-dimension API.
- `GL_RGB` upload explicitly ORs `0x8000` into every direct-color texel. `GL_RGBA` preserves supplied binary alpha. Ordinary indexed palette entries do not carry that alpha bit; see [05](05-vram-memory-and-assets.md).
- `glTexParameter()` replaces parameter bits; preserve color-zero transparency when changing repeat/texture-generation flags. A cached same-name bind may be elided, so raw MMIO changes must invalidate/reconcile the owner state.
- `glColorTableEXT()` is void here. Palette allocation/connection must be verified through supported queries/state, not a fabricated return value.
- `POLY_ALPHA(0)` is wireframe; `POLY_DECAL` does not implement generic texture-alpha cutouts. `GL_BLEND`, `GL_ALPHA_TEST` and `glAlphaFunc()` are global raster settings. The alpha-test register is 5-bit despite the old header comment saying 0–15.
- `DISP3DCNT` bits 12/13 are sticky underflow/overflow status with write-one-to-clear behavior; this is not `GXSTAT` matrix-stack error. Read-modify-write helpers can clear pending evidence. Keep diagnostics and state ownership deliberate.
- Projection/texture matrices have one saved stack level; the model/vector stack has 31 usable levels. Hardware capacities and numerical types are in [07](07-3d-gx.md).
- `glColor3b()` uses 8-bit channel arguments, unlike `RGB15()`'s 5-bit components. Native VRAM/palette/OAM writes must use supported widths; byte-wise C library paths are not guaranteed safe.

Do not duplicate this table into every task plan. Inspect only the relevant implementation, then use the focused reference/example and report what was actually tested.
