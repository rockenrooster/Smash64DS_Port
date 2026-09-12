# Sources and Version Policy

## Scope and provenance

This is a general Nintendo DS coding skill. Hardware constraints are separated
from SDK-specific defaults. The original pack was dated 2026-08-03; both the
intermediate and final baseline revisions are dated 2026-09-06; this alpha audit is dated 2026-09-11. The following baseline
commits are retained as the versions described by the API notes. Relevant
contracts were checked through source review; this is not an installed SDK
build or a statement that every upstream file has been exhaustively audited.

| Repository | Source baseline |
|---|---|
| devkitPro/libnds | `84e6082ce27c87ed218fb369a9944644aa2243a6` |
| devkitPro/calico | `81b75e314d57ed1784545e28554e567f26f572f1` |
| devkitPro/nds-examples | `f1ba715a451c6407f8b0f805999d0153062ff552` |

Project/installed headers and the matched implementation govern API integration;
hardware documentation governs physical behavior. Do not force a dependency
upgrade just to match these pins. Do not mix startup, services, headers, and
runtime binaries from incompatible generations.

## Primary source map

The compact operational guide is [chapter 17](17-libnds2-calico-facts.md).
These primary sources support it and the deeper task chapters:

| Topic | Pinned source |
|---|---|
| ARM9 DS sections / DTCM reservation | [share/ds9.ld](https://github.com/devkitPro/calico/blob/81b75e314d57ed1784545e28554e567f26f572f1/share/ds9.ld) |
| Main stack selection and startup | [bootstub](https://github.com/devkitPro/calico/blob/81b75e314d57ed1784545e28554e567f26f572f1/source/nds/arm9/bootstub_arm9.s); [startup](https://github.com/devkitPro/calico/blob/81b75e314d57ed1784545e28554e567f26f572f1/source/nds/startup.crt0.c) |
| Thread priorities, yielding, TLS, waits | [include/calico/system/thread.h](https://github.com/devkitPro/calico/blob/81b75e314d57ed1784545e28554e567f26f572f1/include/calico/system/thread.h) |
| IRQ-mode restrictions and wake exceptions | [include/calico/system/irq.h](https://github.com/devkitPro/calico/blob/81b75e314d57ed1784545e28554e567f26f572f1/include/calico/system/irq.h) |
| Mailbox capacity and lifetime | [include/calico/system/mailbox.h](https://github.com/devkitPro/calico/blob/81b75e314d57ed1784545e28554e567f26f572f1/include/calico/system/mailbox.h) |
| Tick units and callback context | [include/calico/system/tick.h](https://github.com/devkitPro/calico/blob/81b75e314d57ed1784545e28554e567f26f572f1/include/calico/system/tick.h) |
| PXI units and registration | [include/calico/nds/pxi.h](https://github.com/devkitPro/calico/blob/81b75e314d57ed1784545e28554e567f26f572f1/include/calico/nds/pxi.h) |
| PXI mailbox-full drops / transport waits | [source/nds/pxi.c](https://github.com/devkitPro/calico/blob/81b75e314d57ed1784545e28554e567f26f572f1/source/nds/pxi.c) |
| Compatible paired ARM9/ARM7 composition | [official PXI example](https://github.com/devkitPro/nds-examples/tree/f1ba715a451c6407f8b0f805999d0153062ff552/pxi) |
| Compiler template flags | [ARM9](https://github.com/devkitPro/nds-examples/blob/f1ba715a451c6407f8b0f805999d0153062ff552/pxi/arm9/Makefile); [ARM7](https://github.com/devkitPro/nds-examples/blob/f1ba715a451c6407f8b0f805999d0153062ff552/pxi/arm7/Makefile) |
| DMA wrapper units / channel use | [include/nds/dma.h](https://github.com/devkitPro/libnds/blob/84e6082ce27c87ed218fb369a9944644aa2243a6/include/nds/dma.h); [underlying DMA helper](https://github.com/devkitPro/calico/blob/81b75e314d57ed1784545e28554e567f26f572f1/include/calico/gba/dma.h) |
| Cache maintenance alignment | [include/nds/arm9/cache.h](https://github.com/devkitPro/libnds/blob/84e6082ce27c87ed218fb369a9944644aa2243a6/include/nds/arm9/cache.h) |
| GX formats, status, stack/list semantics | [include/nds/arm9/videoGL.h](https://github.com/devkitPro/libnds/blob/84e6082ce27c87ed218fb369a9944644aa2243a6/include/nds/arm9/videoGL.h) |
| Texture upload / bank remapping | [source/arm9/videoGL.c](https://github.com/devkitPro/libnds/blob/84e6082ce27c87ed218fb369a9944644aa2243a6/source/arm9/videoGL.c) |
| Native arithmetic and trig | [math.h](https://github.com/devkitPro/libnds/blob/84e6082ce27c87ed218fb369a9944644aa2243a6/include/nds/arm9/math.h); [trig_lut.h](https://github.com/devkitPro/libnds/blob/84e6082ce27c87ed218fb369a9944644aa2243a6/include/nds/arm9/trig_lut.h) |
| BG shadow-state ownership | [include/nds/arm9/background.h](https://github.com/devkitPro/libnds/blob/84e6082ce27c87ed218fb369a9944644aa2243a6/include/nds/arm9/background.h) |
| OAM / sprite ownership and mapping | [include/nds/arm9/sprite.h](https://github.com/devkitPro/libnds/blob/84e6082ce27c87ed218fb369a9944644aa2243a6/include/nds/arm9/sprite.h) |
| Debug console selection | [include/nds/arm9/console.h](https://github.com/devkitPro/libnds/blob/84e6082ce27c87ed218fb369a9944644aa2243a6/include/nds/arm9/console.h) |

The [libnds 2.0.0 migration notes](https://github.com/devkitPro/libnds/releases/tag/v2.0.0)
explain Calico scheduling, timer ownership, IRQ changes, removed memory aliases,
PXI, ARM7 services, and libdvm. Versioned implementation source takes precedence
when a short API description omits blocking, cache work, or channel consumption.

## Hardware sources

[GBATEK](https://problemkaputt.de/gbatek.htm) is the hardware reference for memory,
VRAM banking, CPU access widths, GX commands/capacity, DMA, timers, IPC, and sound.
Useful sections include
[DS DMA](https://problemkaputt.de/gbatek-ds-dma-transfers.htm) and
[VRAM control](https://problemkaputt.de/gbatek-ds-memory-control-vram.htm).
Use the actual ARM946E-S / ARM7TDMI ISA documentation when reviewing instruction
semantics. Hardware constants are not SDK heap or free-TCM budgets.

The prior review also cross-checked retained-geometry behavior in the
[melonDS geometry implementation](https://github.com/melonDS-emu/melonDS/blob/master/src/GPU3D.cpp).
That moving link is an implementation cross-check, not a pinned hardware oracle
or a performance measurement. Do not infer real DMA/cache/storage timing from
unvalidated emulator behavior.

## Maintenance policy

When changing an API-specific recommendation, update its versioned source link,
relevant example, and test contract together. Read implementation effects as
well as declarations. Verify target compilation with real SDK headers when
available; do not use host mocks as ABI or device evidence. Keep expensive model
evaluations and maintenance checks outside the ordinary agent workflow.

DS mode is the primary target. DSi mode, alternate SDKs, custom MPU/linker
layouts, and modified runtimes need their own checks. Exact performance depends
on the workload, storage, target, compiler, and runtime configuration.

Release changes: [CHANGELOG](../CHANGELOG.md).
Executed checks and limitations: [REVIEW_RESULTS](../tests/REVIEW_RESULTS.md).

## Transparency audit additions (2026-09-11)

[videoGL.c](https://github.com/devkitPro/libnds/blob/84e6082ce27c87ed218fb369a9944644aa2243a6/source/arm9/videoGL.c) and the pinned header above establish the upload/API traps. The [melonDS texture lookup and pixel shader](https://github.com/melonDS-emu/melonDS/blob/master/src/GPU3D_Soft.cpp) cross-check format bits, alpha expansion, decal/modulation and overlap handling (moving source inspected on the audit date; not a timing authority). [BlocksDS graphics](https://blocksds.skylyrac.net/tutorial/intermediate/3d_graphics/) and [sprite](https://blocksds.skylyrac.net/tutorial/basic/sprites/) examples corroborate hardware usage but their differing API signatures are not imported into devkitPro code.

The new cutout example was reviewed against pinned declarations; absent real SDK compilation and a native run, it remains an integration smoke-test candidate. Its pixel producer is covered by host tests, not GPU simulation.

## Project map provenance

Repository entry points were checked at Smash64DS_Port commit `5e733919023f9d109773df89d7913589039b79fa` on 2026-09-11. This pins the map review, not SDK upgrades or current task state. Use the installed toolchain and current checkout.
