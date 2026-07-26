# Task 75 E0 — Preloading every fighter animation costs 711 KiB

**Date:** 2026-07-26
**Status:** E0 sizing. Static, no emulator run. Owner has authorised the RAM
trade ("unused RAM is wasted RAM").

Task 73 established that the on-demand animation load cannot be skipped — nothing
is redundantly re-fetched (`animResident:0`) — only moved. Moving it means holding
animations resident, so the first question is what that costs.

## The number

`builds/*/nitrofs/reloc/reloc_animations` is **301 files, 728,064 bytes
(711.0 KiB)** — and the whole directory is Mario and Fox: 143 Mario, 158 Fox.
There is no larger set to subset out of. Preloading *everything* both fighters
can animate is 711 KiB.

Relocated size will exceed file size by alignment and fixups; budget ~800 KiB.

## The room

Loaded sections of the battle ROM (debug sections excluded — they are not
resident):

| section | bytes |
|---|---|
| `.main` | 804,440 |
| `.main.bss` | 1,701,936 |
| `.main.rw` | 133,012 |
| `.itcm`, `.text.hot`, `.text.hot.draw`, `.dldi` | 60,640 |
| **total** | **2,700,028 (2.58 MiB of 4 MiB)** |

**~1.42 MiB free** before the runtime heap. 800 KiB fits with roughly 600 KiB
spare.

## What it buys

Task 71 priced the whole on-demand-load path at roughly 170,000 ticks of P95;
Task 72 has already taken 79,488 of that by removing a duplicate open. The
remainder — the surviving directory walk, the cartridge read, the `memcpy`, the
byte swap and the relocation — is what preloading removes from gameplay
entirely, because it all happens once at match start instead of inside the frame
that needs the move.

## What has to be settled during implementation

The load returns a pointer the caller then uses:
`lbRelocGetForceExternHeapFile` returns `(file != NULL) ? file : heap`. Today
`file == heap`, the caller-provided slot. A preloaded copy lives at a fixed
address instead, so the change hinges on whether callers tolerate a returned
pointer that is not the heap they passed in. That is the first thing to check,
not the last — if any caller assumes equality, the design becomes "copy the
resident copy into the slot", which keeps the `memcpy` and saves only the open,
read, swap and relocate.

Relocation is per-destination-address, so a preloaded buffer must be finalized
once for the address it will permanently occupy, and `ndsRelocRegisterLoadedFile`
must record it there.

Second open question: whether preloading at match start pushes a visible hitch
into the match-start transition. 711 KiB of cartridge read is not free; it is
merely somewhere the player is already waiting.
