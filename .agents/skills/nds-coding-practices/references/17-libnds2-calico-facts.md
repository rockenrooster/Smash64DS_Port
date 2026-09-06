# libnds 2.x / Calico: Facts for Writing Code

**Scope:** devkitPro libnds `84e6082`, Calico `81b75e3`, nds-examples `f1ba715`,
reviewed 2026-09-06. These are runtime/API facts, not a universal contract for
legacy libnds, BlocksDS, modified linkers, or DSi mode. Installed project headers
and matched source take precedence. See [source pins](SOURCES.md) for full SHAs.
Use this page as a lookup, not required reading for every small task.

## Memory: physical capacity is not free heap

| Item | Reviewed DS-mode behavior |
|---|---|
| Physical main RAM | 4 MiB shared by the system, not a 4 MiB ARM9 heap. |
| Ordinary ARM9 DS sections | `ds9.ld` main region begins at `0x02001000`; a separate assertion limits the ordinary DS sections to end at or below `0x02380000` (nominal 3.5 MiB split). Do not use the script's larger DSi-capable region length as the DS budget. |
| Upper main RAM | Runtime/ARM7-side reservations occupy space outside that ARM9 region; do not reclaim it by changing only one linker assertion. |
| Free application heap | Smaller than the nominal split: inspect the linked sections, startup/runtime reservations, actual heap limits, allocations, and stack overlap. Link success does not prove later dynamic allocations fit. |
| `.rodata` / `const` | Resident linked data still uses RAM. Default main RAM is writable at the MPU level; `const` gives language/optimization semantics, not per-object write protection or cartridge execute-in-place. |
| ITCM / DTCM | Physical capacities are 32 KiB / 16 KiB. Runtime vectors/stacks/reservations reduce what the application can place there. Neither is accessible to DMA. |
| Main ARM9 stack | Defaults to DTCM. The linker exposes `0x3e80` bytes below reserved exception/BIOS space, shared with `.dtcm` and `.dtcm.bss`; this is not 16 KiB of free stack. |
| Old uncached mirrors | `memCached` / `memUncached` were removed. An old `0x02400000` alias is not a supported DS-mode main-RAM view under this MPU layout. Use proper cache maintenance, not invented aliases. |

`u32 __stacksize__ = N;` is a startup override: only when the requested size is
larger than the available DTCM space does startup carve the main stack from the
heap. A small nonzero value does not necessarily relocate it. In C++ define the
symbol with C linkage. Budget that heap cost and verify the actual placement.

**For one DMA upload, prefer an explicitly main-RAM buffer rather than moving
the entire main stack.** An aligned static buffer is suitable only if the actual
linker places it in accessible RAM; alignment alone proves nothing about its
memory domain. CPU halfword uploads are another option for small video edits.
A stack-built `glCallList` buffer has the same DTCM problem because that API uses
DMA. A flush or a synchronous wait cannot make TCM DMA-readable.

Sources: Calico [`ds9.ld`](https://github.com/devkitPro/calico/blob/81b75e314d57ed1784545e28554e567f26f572f1/share/ds9.ld),
[`bootstub_arm9.s`](https://github.com/devkitPro/calico/blob/81b75e314d57ed1784545e28554e567f26f572f1/source/nds/arm9/bootstub_arm9.s),
[`startup.crt0.c`](https://github.com/devkitPro/calico/blob/81b75e314d57ed1784545e28554e567f26f572f1/source/nds/startup.crt0.c),
and the [libnds 2.0 migration notes](https://github.com/devkitPro/libnds/releases/tag/v2.0.0).

## Threads, waits, IRQs, and timers

| Need / trap | Correct current-runtime choice |
|---|---|
| Main loop | `while (pmMainLoop())` preserves runtime sleep/exit handling. |
| Frame wait | `swiWaitForVBlank()` aliases the Calico thread wait; `threadWaitForVBlank()` waits for the next VBlank. It blocks, rather than creating spare CPU time by spinning. |
| Scheduling | Priority-preemptive, no timeslicing. Lower numeric priority means higher scheduling priority. `MAIN_THREAD_PRIO` is the default reference point. |
| Idle worker | Block on `mailboxRecv`, a supported wait primitive, or a timer. `threadYield()` shares with equal-priority threads, not lower-priority workers. |
| New thread | `threadPrepare` takes an **8-byte-aligned stack top**, not the buffer base. Keep thread/stack objects alive until `threadJoin` completes. |
| libc / TLS in a worker | Attach storage with `threadAttachLocalStorage` before starting a native thread that calls libc or accesses TLS. With `NULL`, storage is carved from that worker's stack; budget `threadGetLocalStorageSize()`. Do not blindly transplant ARM9 TLS assumptions to ARM7. |
| Portable threading | POSIX/C/C++ threading interfaces are supported; native Calico threads are not the only option. Threads still consume stacks and CPU time. |
| ISR / tick / PXI callback | IRQ mode, separate stack, no nesting. No libc, TLS access, ordinary thread calls, blocking, or allocation. `threadUnblock*` and `mailboxTrySend` are documented wake-up exceptions. |
| LCD IRQ setup | Registration, interrupt-controller enable, and LCD source enable are separate. Use the supported LCD helpers; do not overwrite an existing handler casually. |
| Profiling / periodic work | Timers **2 and 3 belong to Calico**. `tickGetCount()` returns ticks at `TICK_FREQ`, not ARM9 CPU cycles. Use tick/thread timer facilities; reserve 0/1 only after checking their owners. Tick callbacks themselves still run in IRQ context. |

A native `Mailbox` holds same-CPU thread messages; it is not itself a shared
ARM9/ARM7 memory protocol. `mailboxPrepare` takes a **word count** (not bytes);
its storage must survive the mailbox. Choose a capacity fitting its `u8` count
field (1..255), not an arbitrarily large queue. `mailboxTrySend` can fail when
full: specify coalescing, backpressure, or an error policy. Never spin at higher
priority waiting for a lower-priority consumer to make space.

Examples: [`irq_handoff.c`](../examples/irq_handoff.c) for a small protected
same-CPU snapshot; [`irq_worker.c`](../examples/irq_worker.c) for a documented
IRQ-to-thread wake-up with shutdown. A thread waiting directly for VBlank is
simpler when no custom ISR capture is needed.

Sources: Calico [`thread.h`](https://github.com/devkitPro/calico/blob/81b75e314d57ed1784545e28554e567f26f572f1/include/calico/system/thread.h),
[`irq.h`](https://github.com/devkitPro/calico/blob/81b75e314d57ed1784545e28554e567f26f572f1/include/calico/system/irq.h),
[`mailbox.h`](https://github.com/devkitPro/calico/blob/81b75e314d57ed1784545e28554e567f26f572f1/include/calico/system/mailbox.h),
[`tick.h`](https://github.com/devkitPro/calico/blob/81b75e314d57ed1784545e28554e567f26f572f1/include/calico/system/tick.h).

## PXI replaces the legacy custom FIFO API

Do not emit old `fifoSendValue32` calls into a current Calico project. Reserve one
of `PxiChannel_User0` through `PxiChannel_User7`; leave system channels alone.

| Operation | API and important unit/context |
|---|---|
| Wait for peer registration | `pxiWaitRemote(channel)`; startup wait, not a per-frame poll. |
| Simple send | `pxiSend(channel, immediate)`; **26-bit** immediate, not arbitrary 32-bit data. Not waiting for a reply does not promise wait-free FIFO transmission. |
| Request/reply | `pxiSendAndReceive(channel, immediate)` waits for `pxiReply`; it has no timeout argument. Keep one request owner per channel and a peer guaranteed to reply. |
| Receive on a worker | `mailboxPrepare` + `pxiSetMailbox`; worker blocks on `mailboxRecv` and responds with `pxiReply`. |
| Receive in a callback | `pxiSetHandler`; callback is IRQ context, not a worker. |
| Larger messages | `pxiSendWithData`: 16-bit immediate and 1..32 additional **words**. Read the peer's framing path before extending a simple-message protocol. |

**Queue-full trap:** the reviewed `pxiSetMailbox` adapter calls
`mailboxTrySend` and ignores failure. A full mailbox can silently drop words.
Use one outstanding request for a tiny control protocol, or explicit capacity/
credits for bursts and extended messages. The transport does not make an
undersized application mailbox lossless. Source:
[`pxi.c`](https://github.com/devkitPro/calico/blob/81b75e314d57ed1784545e28554e567f26f572f1/source/nds/pxi.c).

A transmitted address is not proof that either CPU can safely access the
payload. Shared pointers still need mapping, bounds, cache visibility, and
lifetime. Prefer small values or validated handles. The bounded echo/stop pair
in [`examples/pxi/README.md`](../examples/pxi/README.md) deliberately uses only
simple values; it does not require a shared-memory arena or replace ARM7 services.

Sources: Calico [`pxi.h`](https://github.com/devkitPro/calico/blob/81b75e314d57ed1784545e28554e567f26f572f1/include/calico/nds/pxi.h),
[official PXI example](https://github.com/devkitPro/nds-examples/tree/f1ba715a451c6407f8b0f805999d0153062ff552/pxi).

## API side effects: units, hidden work, and completion

| API | Reviewed effect |
|---|---|
| `dmaCopy` | DMA3, halfword copy, byte-count argument, waits for transfer. Validate even byte count, zero and hardware limit; caller handles cache publication. |
| `dmaCopyWordsAsynch` | Explicit channel and byte count; word alignment/count, accessible buffers, cache publication, channel ownership, and last-use lifetime remain caller responsibilities. |
| `DC_InvalidateRange` | Requires owned full 32-byte cache lines (base and length aligned). Dirty data can be discarded. Arbitrary outward rounding can corrupt neighbors. |
| `glCallList` | First word is payload length in **words**, excluding itself. Flushes payload, waits for all four DMA channels to be idle, uses DMA0, then waits for DMA0. Transfer completion is not render/texture completion. |
| `glFlush` | Submits the swap/finalization command; not a full GPU fence. FIFO backpressure can stall writes. |
| `glTexImage2D` with data | Upload implementation temporarily maps A-D for LCD access and restores mapping. A null data pointer requests allocation without upload. Do not remap while affected rendering/capture/DMA users are active. |

Texture-mode VRAM is not CPU-writable through the texture mapping. Prefer
setup/loading uploads or a bounded, proven-safe upload window. Do not turn this
into a universal ban on dynamic textures; synchronize the actual bank users.

GX status: polygon/vertex overflow is `GFX_CONTROL & GL_POLY_OVERFLOW`
(`DISP3DCNT` bit 13), not `GXSTAT`. Bit 12 reports render underflow. Acknowledge
write-one-to-clear flags deliberately. `glEnd` is a dummy command; omit
`GFX_END` in packed lists. Projection/texture have one stack level each;
position/vector have 31 usable stack levels, with existing renderer reservations
reducing application nesting.

Sources: libnds [`dma.h`](https://github.com/devkitPro/libnds/blob/84e6082ce27c87ed218fb369a9944644aa2243a6/include/nds/dma.h),
[`cache.h`](https://github.com/devkitPro/libnds/blob/84e6082ce27c87ed218fb369a9944644aa2243a6/include/nds/arm9/cache.h),
[`videoGL.h`](https://github.com/devkitPro/libnds/blob/84e6082ce27c87ed218fb369a9944644aa2243a6/include/nds/arm9/videoGL.h),
[`videoGL.c`](https://github.com/devkitPro/libnds/blob/84e6082ce27c87ed218fb369a9944644aa2243a6/source/arm9/videoGL.c).

## Native number formats and math

The `f32` naming convention uses `s32`/`int` values; do not assume there is a
C typedef named `f32`. Fractional counts below are explicit to avoid Q-notation
ambiguity. See [`10-fixed-point-and-hardware-math.md`](10-fixed-point-and-hardware-math.md).

| Boundary | Storage / scale | Helpers or packing |
|---|---|---|
| Scalar / matrix f32 | signed 32-bit, 12 fraction bits, divide raw by 4096 | `inttof32`, `mulf32`, `divf32`, `sqrtf32` |
| `v16` vertices | signed 16-bit, 12 fraction bits; range -8..32767/4096 | `inttov16`, `glVertex3v16`, `VERTEX_PACK` |
| `t16` texture coordinates | signed 16-bit, 4 fraction bits; range -2048..32767/16 texels | `inttot16`, `TEXTURE_PACK` |
| `v10` normals | packed signed **10-bit**, **9 fraction bits**, divide raw by **512**; -1..511/512 | `f32tov10`, `NORMAL_PACK`; header's “.10” comment does not mean ten fractional bits |
| Native angles | 32768 units per turn; wrap to 0..32767 | `DEGREES_IN_CIRCLE`, `degreesToAngle` |
| Trig result | 12 fraction bits | `sinLerp`, `cosLerp`; distinguish binary-angle APIs from degree/radian wrappers |

`mulf32` shifts its signed wide product, giving floor-like rounding on the target
for negative fractions; the portable reference helper in this pack truncates
toward zero. `divf32` uses the ARM9 hardware divider in 64/32 mode; generic
`int64_t / variable` can call `__aeabi_ldivmod`. Neither is a license to ignore
zero divisors, result range, negative-shift language rules, or overlapping math
unit users. Match semantics before replacing a helper.

Sources: libnds [`math.h`](https://github.com/devkitPro/libnds/blob/84e6082ce27c87ed218fb369a9944644aa2243a6/include/nds/arm9/math.h),
[`trig_lut.h`](https://github.com/devkitPro/libnds/blob/84e6082ce27c87ed218fb369a9944644aa2243a6/include/nds/arm9/trig_lut.h),
and `videoGL.h` above.

## Build defaults and practical debugging

The reviewed official templates use ARM9 ARM code with
`-march=armv5te -mtune=arm946e-s -O2` and ARM7 Thumb with
`-march=armv4t -mtune=arm7tdmi -mthumb -Os`. Retain working project overrides;
measure changes rather than assuming one mode is always faster.

For a development build, install `defaultExceptionHandler()` for exception
register dumps. `consoleDebugInit(DebugDevice_NOCASH)` selects emulator debug
output; `nocashMessage("literal")` emits a message without changing the visible
screen. Keep diagnostics out of IRQs and measured hot paths. Availability and
display depend on the chosen emulator; logging is not timing validation.

```sh
# Unresolved helpers in a real, exercised object (a first check only):
arm-none-eabi-nm -u hot.o | grep __aeabi_
# Linked image may already have resolved those symbols:
arm-none-eabi-nm -S --size-sort app.elf
arm-none-eabi-objdump -dr app.elf
```

Exercise header helpers with nonconstant arguments: a `.c` file that never
includes/calls them cannot validate their generated path. An empty optimized
object is not a performance result. See `tests/README.md` for optional checks.

Current storage stack: libdvm replaces libfat/libfilesystem with compatibility
interfaces; block-device work is on ARM7 while filesystem processing is on ARM9.
Use the project's selected filesystem interfaces instead of adding a second stack.

Sources: [official ARM9 Makefile](https://github.com/devkitPro/nds-examples/blob/f1ba715a451c6407f8b0f805999d0153062ff552/pxi/arm9/Makefile),
[ARM7 Makefile](https://github.com/devkitPro/nds-examples/blob/f1ba715a451c6407f8b0f805999d0153062ff552/pxi/arm7/Makefile),
libnds [`console.h`](https://github.com/devkitPro/libnds/blob/84e6082ce27c87ed218fb369a9944644aa2243a6/include/nds/arm9/console.h),
and the migration notes linked above.
