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

# E1 — Design constraints found, and why implementation stopped here

Three facts, established by reading the call path rather than assuming it.

## 1. The return value is discarded

`decomp/BattleShip-main/decomp/src/ft/ftmain.c:4623`:

```c
lbRelocGetForceExternHeapFile(motion_desc->anim_file_id, (void*) fp->figatree_heap);
fp->figatree = fp->figatree_heap;
```

The caller throws the returned pointer away and uses the slot it passed in, so
the data must physically land at `fp->figatree_heap`. `decomp/` is read-only, so
this cannot be changed. **The "return a resident pointer" design is unavailable**
and a copy always remains. E0 flagged this as the question that decides the size
of the win; the answer is the pessimistic one.

## 2. But a copy is enough, and the helper already exists

`ndsRelocCopyLoadedFileToHeap` memcpys and then walks the payload rebasing every
word that points inside the source range to the destination range. A relocated
resident copy can therefore be stamped into `figatree_heap` and stay valid.

So preloading still removes the NitroFS walk, the cartridge read, the byte swap,
the relocation and the token lookup from the frame — everything except one
RAM-to-RAM copy, which is cheaper than the cart-backed copy it replaces.

## 3. The blocker: registration capacity and renderer aliasing

`NDS_RELOC_LOADED_FILE_CAPACITY` is **96**. There are **301** animations.
Preloaded copies cannot all be registered in `sNdsRelocLoadedFiles`, and
relocation is only expressible against a registered `NDSRelocLoadedFile`.

Registering them would also be wrong even if it fit: the renderer resolves
fighter display lists through `ndsRelocFindLoadedFileContaining`
(`reloc_backend_renderer_dl.c:11617`, inside the native-owner validation Task 69
mapped), and an arena copy containing the same payload could capture that
lookup instead of the live heap slot. That is a correctness hazard in the exact
path whose fallbacks Task 69 spent a session counting.

## Two viable designs, neither of them small

**A. Arena with private relocation.** Keep preloaded copies outside
`sNdsRelocLoadedFiles` entirely and give the arena its own minimal
relocate-in-place, so nothing the renderer queries can alias them. Needs the
fixup logic factored out of the registered-file path.

**B. Raw arena, relocate on use.** Store post-byte-swap, pre-fixup payloads and
run the existing finalize into `figatree_heap` after the copy. Keeps
`ndsRelocFinalizeLoadedFile` in the frame — Task 71 measured that at 28,241
ticks/frame — so it wins less, but touches far less machinery.

B is the safer first cut and still removes the walk, the cart read and the byte
swap. A is the full win.

## Status

Stopped before writing either. Both need the fixup path factored or duplicated,
and landing a half-validated change in the animation load path — with a
correctness hazard sitting in the renderer's pointer resolution — is not worth
the risk of finishing it in a hurry. The sizing (E0) and these constraints are
the deliverable; implementation is a clean-context task.
