# Task 53 — Re-activate Task 36 rigid stage replay: E2 measurement

**Date:** 2026-07-24
**Branch:** `codex/task53-replay-arena-fix` (E0/E1 by prior session + E2 here)
**Parent of E2 session:** `f67e571` (this branch, after the E1 build-fix commit)
**Outcome:** **KEEP-candidate — replay re-activates and is bit-exact correct; STG drops
33% but ALL is flat (the saved CPU time redistributes to OTHR).** Default-off, behind
the flag, pending owner visual + device confirmation. Unblocks the Task 52 DMA follow-up
on a now-live replay loop.

## E1 build-fixes (load-bearing gaps found + fixed during E2)

The E0/E1 flag was declared and validated but **never reached the C compiler**, so a
command-line `NDS_TASK53_REPLAY_ARENA_FIX=1` built a TASK53=0 ROM (the override trap the
spec warned about). Verified by building, then fixed (commit `f67e571`):

1. **Makefile config-header emit missing.** The C reads every flag via
   `-include nds_build_config.h` (Makefile:598); TASK53 was never echoed into it, so
   `#if NDS_TASK53_REPLAY_ARENA_FIX` always saw 0. Added the echo (Makefile:1439).
2. **TASK36 cross-check validation mis-ordered.** Ran at Makefile:192, before the
   tick-HUD target block's `override NDS_TASK36_HW_COMPOSE := 2` applied, so a
   command-line TASK53=1 against the TASK36-forcing target was wrongly rejected.
   Moved it beside the Task 44 cross-check (Makefile:554), after every override — the
   established pattern.
3. **Staleness counter declared inside the profile-1 block.** Its use site is gated
   only on TASK53 (no profile gate), so at profile-0 the build failed 'undeclared'.
   Moved the definition (and header extern) to file scope outside profile-1.

Verified after fix: flag-on tick-HUD ROM carries `gNdsRendererTask36ReplayArenaStaleCount`
(@ 0x02194f80) + `ndsRendererTask36ReplayRun` in the ELF; config reads
`#define NDS_TASK53_REPLAY_ARENA_FIX 1`. Default-off published ROM still reproduces
**1818AA77** byte-for-byte.

## E0 safety (confirmed independently here)

- **Baked-address audit:** `ndsRendererTask36ReplayRecord` (nds_renderer.c:4426) writes
  only packed GX opcodes (`opcode << slot*8`) and the GX parameter words (`words[i]`,
  :4483) — matrix cells, vertices, texcoords, colors, polygon params. **No absolute
  heap/arena pointer is captured.** The replay stream has zero arena-layout dependency.
- **Per-frame correctness envelope intact:** `rigid_binding_mask` check (4212/4282),
  config `memcmp` (4254), `ndsRendererTask36ReplayTexturesValid` (4262) all unchanged.
- **Relaxed condition:** admit when `gNdsTaskmanArenaChosenSize >= 0x130000` (the
  allocator floor), regardless of alloc-fails. Legacy strict macro retained for the
  staleness detector.

## E1 probe — replay now admits (the fix works)

`scripts/probe-task52-replay-active.ps1` on the flag-on tick-HUD ROM
(`25b87f8a…`), frames 438–445, reading the always-present internal struct
`sNdsRendererTask36ReplayOwner`:

```
REPLAY_PROBE (flag-ON):  state=2(READY) word_count=3916 frame_capture=0
                         frame_replay=1 capture_fault=0 captured_segment_mask=0xA1
ARENA_GATE (flag-ON):    arena_chosen_size=0x14C000 arena_alloc_fail_count=4
                         rigid_binding_mask=0x00000381c00fffff (matches constant)
                         state=2(READY)
```

vs flag-off (Task 52 E0): `state=3(DISABLED) frame_replay=0 word_count=0` at the same
`arena_chosen_size=0x14C000` / 4 fails. **The relaxed guard admits what the legacy strict
guard blocked, and replay runs.** Memory environment unchanged (same arena size, same
fail count) — the replay buffer is static BSS, not arena-allocated.

## E2 correctness — Task 49 differ (the hard gate): PASSED

STAGE owner, frames 438–445, flag-ON (replay) vs flag-OFF (generic emit), same source
tree, profile-1 differ builds:

| tier | standard | result |
|---|---|---|
| **Tier 1** (non-matrix, bit-exact) | zero tolerance | **2213 entries / 2860 words compared, 2860 matched, 0 divergences → ZERO_DEVIATION** |
| **Tier 2** (matrix effective transform) | screen-space px | **max 0.0 px, mean 0.0 px, all 8 bindings 0.0 → ZERO_DEVIATION** |

The replayed GX stream is **bit-identical** to the live generic emit — same source
geometry, same words, same order. Replay is a pure render-path reorganization; it
changes how the words are submitted (replay buffer vs per-triangle emit), not what is
submitted. A texture-address hazard would have surfaced as a Tier-1 divergence in
TEXIMAGE_PARAM/TEX_COORD; none did. Both captures: 2229 entries, 2996 words, 8 bindings,
0 overflow, 0 fault.

State hash: the differ Tier 1 = 0 (identical GX stream ⇒ identical render ⇒ no gameplay
value moved) is the strongest correctness proof for a render-only change. The pre-existing
master state-hash red (Task 45: relocated heap pointers, not gameplay) is unrelated and
unchanged.

## E2 STG A/B — THE ANSWER (128 samples, frame 438, same ROM/window/input, deterministic)

A = flag-off (generic emit), B = flag-on (replay). Same task53 source tree, melonDS
`DE80E46B…` (repo fork, models icache/dcache). B run twice — **byte-identical between
runs** (deterministic, not noise).

| bucket | A P50 | B P50 | Δ P50 | A P95 | B P95 | Δ P95 |
|---|---|---|---|---|---|---|
| ALL | 1,680,256 | 1,680,128 | **−128** | 2,241,024 | 2,240,576 | −448 |
| FTR | 576,384 | 579,264 | +2,880 | 1,013,760 | 1,014,528 | +768 |
| **STG** | **569,280** | **381,632** | **−187,648 (−33.0%)** | 575,744 | 388,672 | **−187,072 (−32.5%)** |
| BG | 4,160 | 4,224 | +64 | 4,224 | 4,288 | +64 |
| AUD | 2,368 | 2,496 | +128 | 64,960 | 63,232 | −1,728 |
| HUD | 960 | 1,024 | +64 | 316,160 | 320,704 | +4,544 |
| SRC | 317,248 | 318,336 | +1,088 | 950,016 | 953,408 | +3,392 |
| MISC | 48,448 | 48,704 | +256 | 158,528 | 157,632 | −896 |
| **OTHR** | **163,712** | **338,432** | **+174,720** | 432,768 | 537,216 | +104,448 |

VBlank-interval histogram (565 presented frames):

| | A (flag-off) | B (flag-on) |
|---|---|---|
| 2-VBlank | 0 | 0 |
| 3-VBlank | 426 | **474** |
| 4-VBlank | 122 | **80** |
| 5+-VBlank | 17 | **12** |
| max interval | 18 | 18 |
| slips | 0 | 0 |

### What the A/B says

**STG drops 187,648 ticks (33%)** — replay skips the per-triangle geometry walk as
predicted; the stage-owner CPU work shrinks substantially.

**But ALL P50 is essentially unchanged (−128).** The saved STG CPU time is consumed by
work outside the named buckets: **OTHR rises +174,720**, nearly offsetting the STG gain.
OTHR = `all − named` (taskman_seam.c:4919), the residual. The stage owner got faster, but
the total frame loop did not.

**The most likely cause is GX-backpressure redistribution**, not a defect: replay submits
the same 2996 words to the FIFO with less CPU prep, so STG *CPU* time drops, but the GX
geometry engine still has to process them, and that stall now manifests outside the
stage-owner measurement window (waiting on GX completion before the next frame-phase
operation). This is the same finding Task 52's spec anticipated for DMA — "DMA cannot
remove GX backpressure; it may merely move the same wait" — except here it's replay, not
DMA, and the wait moved from STG into OTHR.

**There is a real pacing improvement at the tail:** the VBlank histogram shifts toward
faster frames (3-VBlank share 426→474, 4-VBlank 122→80, 5+ 17→12). The median frame time
is flat, but fewer frames spill into the 4/5+-VBlank buckets. That is the signal a device
A/B would confirm — melonDS cannot referee pacing near bucket edges authoritatively
(device-only class, per the standing rules).

### Verdict on the win

This is the nuanced outcome between the spec's two poles:
- It is NOT "replay activates but STG doesn't drop" (STG clearly drops 33%).
- It is NOT a clean ALL-level win (ALL P50 is flat; the work redistributes to OTHR).
- It IS a **real STG-owner win with a pacing-tail improvement**, gated on device
  confirmation for the ALL-level claim. The Task 52 DMA follow-up is unblocked — there is
  now a live replay loop whose CPU transport is the next lever, and the OTHR redistribution
  is exactly the GX-backpressure cost DMA-mode-2 overlap would target.

## E2 memory — unchanged (verified)

- `gNdsTaskmanArenaChosenSize` = `0x14C000`, `gNdsTaskmanArenaAllocFailCount` = 4 on
  **both** flag-off and flag-on (probe above) — identical. The replay buffer
  `sNdsRendererTask36ReplayOwner.words[]` is static BSS (resident whether or not replay
  is active; only the admission guard changes), so re-activating replay consumes **no
  additional heap** and cannot erode the post-BGM reserve.
- ELF symbol delta: +1 symbol (`gNdsRendererTask36ReplayArenaStaleCount`, 4 bytes BSS).
  Later BSS symbols shift by ~0x60 (the counter + alignment). No code-size growth in the
  hot path (the relaxed guard is a macro swap, same instruction count at TASK53=0).

## E2 visual — A/B screenshots captured (owner is the oracle)

`artifacts/visibility/task53/task53-A-flagoff-genericemit.png` and
`task53-B-flagon-replay.png` (synchronized pair, 70776 / 70125 bytes). The differ's
ZERO_DEVIATION (2860/2860 words bit-identical) is the stronger proof that the rendered
output is identical; the screenshots are for the owner's eyeball confirmation. **Owner
visual approval is still required before any ship** (agents cannot self-approve visuals).

## Build environment note

Git Bash direct `make` hits the `/opt/devkitpro` recursive sub-make quirk. Build through
the devkitPro msys2 bash:
`C:/devkitPro/msys2/usr/bin/bash.exe -lc 'cd repo && make TARGET=... BUILD=... -j16'`.

## Disposition

**KEEP-candidate, default-off, behind `NDS_TASK53_REPLAY_ARENA_FIX`.** Not overridden in
any published or tick-HUD target block (stays default-off until owner approves + device
confirms). Published ROM stays `1818AA77…`. The flag is the ship mechanism (canonical
Task 37/32/49 path). The Task 52 DMA follow-up is unblocked on a now-live replay loop;
the OTHR redistribution measured here is the GX-backpressure cost DMA overlap would target.

Branch `codex/task53-replay-arena-fix` holds the E2 evidence (this file, the A/B JSONs,
the differ captures, the screenshots). ROMs: A (flag-off tickhud) `0a75008a…`, B (flag-on
tickhud) `25b87f8a…`, differ-A, differ-B. Never push.
