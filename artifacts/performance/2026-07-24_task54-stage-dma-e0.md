# Task 54 — Stage replay DMA + CPU overlap: E0 overlap-window analysis

**Date:** 2026-07-24
**Branch:** `codex/task54-stage-dma-overlap` (E0 only)
**Parent:** `482eb57` (master — Task 53 shipped, replay live, default-on)
**Outcome:** **STOP at E0.** There is no safe, material overlap window for a
deferred-barrier DMA. The OTHR backpressure Task 53 measured is unavoidable
FIFO serialization against a single geometry engine that is throughput-bound on
the stage's fixed word count. DMA mode 2 cannot reclaim it. **Geometry reduction
is the recommended next lever.** No DMA implementation is admitted.

## E0 charter (spec §E0)

> Measure before you build. Is there anything to overlap? If the CPU's next act
> after stage submit is GX emit that hits the same FIFO, mode 2 has nothing to
> reclaim and the OTHR is unavoidable FIFO serialization. STOP and recommend
> geometry reduction.

## 1. Hardware facts that settle the question (libnds, read-only reference)

- `GFX_FIFO` is a **single** memory-mapped register: `*(vu32*)0x04000400`
  (`C:/devkitPro/libnds/include/nds/arm9/video.h:728`). Every geometry command —
  matrix, vertex, color, texcoord, begin, polygon attribute — is a store to this
  one register, into one command pipe feeding **one** geometry engine.
- `glFlush` is **non-blocking**: `void glFlush(u32 mode) { asm volatile("" ::: "memory"); GFX_FLUSH = mode; }`
  (`videoGL.h:724`). `GFX_FLUSH` (`0x04000540`) signals "render the buffered
  commands"; it does **not** wait for the geometry engine to finish.
- `GFX_BUSY = GFX_STATUS & BIT(27)` (`video.h:773`) is the geometry-engine busy
  bit. A frame-wide grep for `GFX_STATUS` / `GFX_BUSY` / any FIFO-empty / busy
  poll across `src/` returns **nothing** — there is **no explicit geometry-wait
  anywhere in the frame loop.** Backpressure is entirely implicit: when the
  command pipe is full, the CPU store instruction to `GFX_FIFO` stalls inline
  until one slot drains. It does not stall until the frame completes — only until
  the bounded pipe buffer has room.

## 2. The post-stage frame flow (where mode 2 would have to overlap)

`ndsStageGCDrawAllLoopSubmitHardwareFrame` (`src/port/reloc_backend_movement.c`)
runs the stage owner bracketed by STG ticks, then the frame continues:

```
PrepareNativeStageOwner        ┐ STG bucket  (reloc_backend_movement.c:13701)
gcDrawAll()                    │  ── per-display CommitNativeStageDisplay
                                 │     feeds Task 36 replay or generic emit
FinishNativeStageOwner         ┘  (reloc_backend_movement.c:13738)
ndsFighterDisplayContractSubmitStageFighters()   ← first act after stage finish
                                                   (reloc_backend_movement.c:13756)
  └ per fighter: ndsFighterDisplayContractCapture  (CPU prep, non-FIFO)
                 ndsFighterMarioFoxDLAllDrawForSlot (FIFO writes, same GFX_FIFO)
```

The replay loop itself (`src/nds/nds_renderer.c:20314-20317`):
```c
words = &owner->words[run->word_offset];
for (i = 0u; i < run->word_count; i++) { GFX_FIFO = words[i]; }
```
It returns immediately after the blast — **no inline completion wait.**

## 3. The fatal structural constraint (spec trap #1, confirmed in code)

Stage and fighters draw on the **same single `GFX_FIFO`** into the **same one
geometry engine**, processed **strictly in arrival order**. Consequence:

- Mode 2 can DMA the stage words into the pipe and free the CPU to run the
  fighter's **non-GX** prep (`ndsFighterDisplayContractCapture`: matrix/anim/DL
  capture, counter `m2_contract_capture_ticks`) during the stage drain.
- But the fighter's **GX** work (its own FIFO writes) **appends behind** the
  still-draining stage words in the same pipe. It cannot run during the stage
  drain. It cannot overtake it. The geometry engine processes the stage first,
  then the fighter, in one serial stream.
- Therefore mode 2's reclaimable work is **at most the fighter's non-GX prep**,
  and only the fraction of it that is currently serialized *after* stage submit
  rather than before it. The stage GX work is irreducible.

## 4. Magnitude — bounded by the Task 53 A/B (already measured, 128 samples)

The Task 53 switch from generic-emit (A) to replay (B) is exactly the
"remove stage CPU prep, keep the same stage words" probe. Its result directly
bounds what mode 2 could ever reclaim:

| bucket (P50, ticks) | generic-emit (A) | replay (B = Task 54 mode 0) | Δ |
|---|---|---|---|
| STG | 569,280 | 381,632 | **−187,648** (stage CPU prep removed) |
| OTHR | 163,712 | 338,432 | **+174,720** (that prep's time → pipe-full stalls) |
| **STG+OTHR** | **732,992** | **720,064** | **−12,928 (~constant)** |
| FTR | 576,384 | 579,264 | +2,880 |
| ALL | 1,680,256 | 1,680,128 | **−128 (flat)** |

**Decisive signal: STG+OTHR is ~constant** across a change that removed 187,648
ticks of stage CPU work. That sum is the stage's total cost to the frame — and
it is dominated by the geometry engine draining the stage's fixed 2,996 words,
which is identical in both modes. Replay moved the cost from STG into OTHR; it
did not remove it, because the drain is GX-throughput-bound, not CPU-issue-bound.

The MISC bucket (which absorbs flush ticks) is also flat (48,448 → 48,704) and
small — `glFlush` is non-blocking and cheap, so the geometry drain does not land
in MISC either; it is distributed as pipe-full store stalls attributed to
whatever bucket the CPU is executing when each stall hits.

## 5. What mode 1 and mode 2 would measure (predicted, from the above)

- **Mode 1 (DMA transport, immediate barrier):** the DMA blasts the 2,996 stage
  words into the pipe faster than the CPU loop, then waits for DMA-done. The
  geometry engine still drains the same 2,996 words at the same throughput.
  Expected: ~neutral on ALL (the pipe-full stall is merely moved from the CPU
  store loop to the DMA-completion wait — the spec's own "mode 1 expected
  ~neutral"). This is the correct control.
- **Mode 2 (DMA, deferred barrier):** the DMA blasts the words; the CPU runs
  fighter non-GX prep during the drain; the barrier is inserted before the first
  fighter `GFX_FIFO` write. **The maximum ALL win is bounded by
  `min(fighter non-GX prep, stage drain)` — and the stage drain has no slack
  (it is the ~720K bottleneck, fully utilized).** The fighter non-GX prep is a
  fraction of FTR's 579,264 (most of FTR is the fighter's own GX writes, which
  cannot overlap). Realistic ceiling: a few percent of ALL, **against** a
  deferred-barrier reorder that crosses the stage→fighter owner boundary and
  risks the exact FIFO-collision corruption trap #1 warns about.

## 6. The reframe of Task 53's "ALL flat"

Task 53's OTHR +174,720 was attributed to "GX-backpressure redistribution." The
hardware facts here sharpen that: it is **pipe-full store-stall relocation**.
Under generic-emit, the per-triangle CPU work (matrix walk, vertex prep at
`nds_renderer.c:21374-21416`) is long enough that the pipe drains as the CPU
computes — few stalls land, STG=569K. Under replay, the CPU blasts all words
with ~zero inter-store work, so the pipe fills immediately and every store after
the first ~pipe-depth stalls. Those stalls land partly in STG and partly in
OTHR (after the loop returns), but STG+OTHR is invariant because **the geometry
engine's total work on the fixed stage words is invariant.** DMA substitutes one
issuer (DMA) for another (CPU store loop) into the same pipe; it cannot change
the drain time.

## 7. STOP conditions (spec §E0) — evaluated

1. **Removable CPU transport cost (the FIFO replay loop).** Not zero (unlike
   Task 52, the loop now runs), but the loop's CPU cost is already near its floor
   — a tight `for(i) GFX_FIFO = words[i]` with no per-word CPU work. The
   elapsed cost the loop reports is dominated by the pipe-full stall, which is
   the geometry drain, which DMA cannot remove.
2. **Useful safe overlap interval.** The only candidate (fighter non-GX prep)
   exists but is small relative to the GX-bound floor, and the stage drain has
   no slack to absorb it productively — the geometry engine is busy the entire
   drain. The expected ALL win is bounded to a few percent and carries a
   correctness hazard (deferred barrier across owner boundary into a shared
   pipe) that the differ cannot referee (trap #4).
3. **Most elapsed time is unavoidable GX backpressure.** Confirmed: STG+OTHR
   ~720K is invariant to CPU-work removal and is GX-throughput-bound on the
   fixed 2,996 stage words.

All three lean STOP. The first two are not trivially zero (the loop runs, prep
exists), so this is a **judgment STOP, not a trivial STOP** — but the
cost/benefit is adverse: a deferred-barrier DMA across a shared FIFO for an
expected few-percent ALL win, with a timing-race correctness risk the differ
cannot catch.

## 8. Recommended next lever: geometry reduction (do not pivot here)

The ~720K STG+OTHR floor is the geometry engine processing **2,996 words** for
8 rigid Dream Land layer0 bindings. The only way to lower it is to send the
geometry engine **fewer words**:

- **VTX_10 / VTX_XY** packed vertex formats (fewer words per vertex than
  `FIFO_VERTEX16`'s 2-word form) where Z precision allows.
- **Stripify** the rigid static topology (one `BEGIN` covers a strip; the
  shared-vertex `VERTEX_DIFF` form cuts words).
- **Cull** bindings/frusta that never contribute on-screen.

This cuts the GX floor itself — the lever DMA cannot reach. It is the
recommended next task; this task does not pivot to it.

## Disposition

**STOP at E0.** No DMA implementation admitted. No `NDS_TASK54_STAGE_DMA_MODE`
runtime path added. No DMA channel selected. Published ROM unchanged (Task 53's
shipped ROM). The branch holds this E0 analysis as the deliverable.

Branch `codex/task54-stage-dma-overlap` holds this file. Parent `482eb57`.
Never push.

## Build environment note

Git Bash direct `make` hits the documented `/opt/devkitpro` recursive sub-make
path quirk; build through `C:/devkitPro/msys2/usr/bin/bash.exe -lc '…'` when a
build is needed. E0 here required no new build — the conclusion follows from
hardware semantics (libnds, read-only) and the Task 53 A/B already in tree.
