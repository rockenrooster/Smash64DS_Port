# The pack is resident, the marker-2 hang was the arena, and the counter reads 197

**Date:** 2026-08-15 · **Branch:** `codex/r2-runtime2` · **base HEAD `9d660c8e08f`** ·
**landed `3963b8b14ea`**
**Native Battle Kernel slice 1: Task A (residency), Task B (flag-1 Boundary), Task C (engagement).**
Predecessor: `…/2026-08-15_battlepack-pool/BATTLEPACK_POOL.md`.
**Builds spent: 5** (2 lab proof arms, 2 gate arms, 2 Boundary arms — one Boundary build shared).
Root ROMs unchanged (§7).

---

## 0. Outcome first

```text
RESIDENT      the Fox blob is a NitroFS payload streamed into the taskman
              animation arena at scene setup, replacing the 262,144 B raw-file
              cache.  .incbin is gone.
STATIC COST   proven headroom 66,816 (the .rodata arm) -> 354,208, against a
              355,104 flag-0 baseline: +896 B of ARM9 image, not +288,288.
ARENA         gNdsTaskmanArenaChosenSize 0x150000 and AllocFailCount 0 on every
              arm run this cycle, flag 0 and flag 1 alike.
MARKER 2      RESOLVED.  Three attempts timed out there last cycle; both
              attempts this cycle read "GDB marker capture: 27.1s / 26.7s
              elapsed of 120s ceiling (23% / 22% used)".
ENGAGEMENT    gate arm, whole match:  Hits 0 -> 197 with the total acquisition
              count IDENTICAL at 357.  gNdsBattlePackHits has now been read on a
              live ROM.
STILL RED     Boundary at flag 1 fails ONE assert, and it is a different one:
              the lower-screen rolling FPS counter's self-consistency check,
              FPS_HUD=289,14,15,16856768, byte-identical on two runs.  Boundary
              at flag 0 is GREEN on this tree.  NDS_R2_BATTLEPACK stays 0.
NOT FREE      the un-packed fighter loses the raw cache: gNdsR2AnimCacheRejects
              0 -> 126.  Unpriced.  Section 5.
```

---

## 1. Task A — residency, and the 848 bytes that decided its shape

`.incbin` was disqualified by the previous cycle for a measured reason: +288,992 B of ARM9
image drove `gNdsTaskmanArenaChosenSize` `0x150000 → 0x140000` with
`gNdsTaskmanArenaAllocFailCount 16`, because the arena is one `calloc` from the libnds heap
that `fake_heap_start` bounds. The blob is therefore staged into NitroFS and streamed.

### Static cost, measured on three ELFs

`check-boot-headroom.ps1`'s constant: proven headroom = `0x02294804 − fake_heap_start`.

| arm | `fake_heap_start` | proven headroom |
|---|---|---:|
| `build-c162-battlepack` — last cycle, `.incbin` | `0x02284304` | **66,816** |
| `build-c163-battlepack` — this cycle, NitroFS | `0x0223e064` | **354,208** |
| `build-battle-playable-proof-hwtri-harness` — flag-0 baseline (08-15 00:14) | `0x0223dce4` | 355,104 |

**+896 B of image**, and that 896 includes the loader and the five new counters, not the blob.
ROM length 12,244,992 → 12,525,568 (+280,576, the NitroFS payload). `PROJECT_GOAL.md` trades
ROM for runtime without argument.

### The load

One 16,384-byte chunk per `ndsR2AnimCachePreloadStep`, 18 steps for 287,904 bytes
(`gNdsBattlePackLoadSteps` reads **18** on every flag-1 run). It rides that seam rather than
battle start for two reasons the warm walk already paid for: the BGM packet seam bounds the
work at ~186 ms (E4 put 41 asset loads in one call there and Boundary refused the build on
the ADPCM smoke), and the *lazy* reservation is a safety property — taking the arena at the
first scene update means `ftManagerSetupFilesAllKind` has already taken its 116,752 bytes, so
this can never be the allocation that starves battle start.

### Ordering lost by 848 bytes — and this is the finding

The first implementation let the loader be the **first caller** of the bump allocator. It was
not first. Measured, soak `2026-08-15_015724`:

```text
gNdsBattlePackHits                    0
gNdsBattlePackMisses                221     <- every acquisition took the generic path
gNdsR2AnimCacheArenaOverflows         1
gNdsR2AnimCacheArenaOverflowLastSize  287936  <- the pack's request, refused
gNdsR2AnimCacheArenaOverflowLastUsed    3728  <- already taken when it asked
```

3,728 + 287,936 = 291,664 against a 290,816 arena: **short by 848**. Fighter setup stores
3,728 bytes of animation into the arena before the first scene update runs a pack step.

The fix is not a bigger arena, it is a **carve**: the blob owns `[0, RESERVE)` of every arena
generation, taken at reservation time, so nothing can get in front of it.
`NDS_R2_BATTLEPACK_BLOB_BYTES` is generated into `nds_build_config.h` from the blob's own byte
count (`wc -c`), so the reserve cannot drift from the asset; the runtime still refuses a blob
whose self-declared extent does not fit, and degrades to the generic path rather than
truncating.

Arena now `NDS_BATTLEPACK_RESERVE_BYTES (287,936) + 4,096 = 292,032`, measured as
`gNdsR2AnimCacheArenaReservedBytes 292032` with `ReserveFailCount 0`.

### Ownership

The pack lives inside the arena, so it dies with it. `ndsBattlePackDrop` is called from
`ndsR2AnimCacheArenaDropForReset` — the one seam where the heap generation says the block
stopped being ours — and `ndsRelocForceLoadFighterAObj16File` now runs
`ndsR2AnimCacheValidateGeneration()` **before** the pack lookup, because the pack is consulted
ahead of every other guard in that function. `gNdsBattlePackDrops` reads **1** on every run:
the Results-scene rewind. That is also why `gNdsBattlePackState`/`Clips`/`Bytes`/
`ResidentBytes` read 0 at end-of-run — they are read after the drop. `Hits` is the durable
evidence.

### Which fighter, and why

**Fox.** Both need ~559,632 against ~301,564 and do not fit. The tie-breakers, in order:
the Fox blob is the one already generated in-tree and slot-verified at mismatch 0
(sha256 `f6a49219a32583f4…`, re-confirmed this cycle); Fox is the autonomous level-3 CPU on
the Boundary arm, so packing it maximises engagement on the arm that must go green; and it is
the larger blob, which makes it the binding fit test. **The acquisition counts do not differ
materially** — measured below at 197 Fox / 160 Mario on the gate arm — so there was no
asymmetry to override that.

---

## 2. Task B — the marker-2 timeout was the arena, and it is gone

Three Boundary attempts last cycle timed out at marker 2,
`ndsRendererHardwareArmBattleStaticTextures`. Prime suspect (a) was the arena degradation
starving that very preparation. **Proven with the arena globals, not by observing that the red
went away:**

```text
last cycle, .rodata arm   TASKARENA=1310720,16      (0x140000, 16 alloc failures)
this cycle, both runs     gNdsTaskmanArenaChosenSize 1376256, AllocFailCount 0
                          MEMARENA=0,22,2,1376256,1320928,1320928,55328,…
capture cost              27.1 s and 26.7 s of a 120 s ceiling (23% / 22%)
```

All four markers hit, twice. Suspect (b) — a third reader still requiring a registered loaded
file — **did not need to be invoked**: the two seams taught last cycle
(`ndsRelocFindKnownFileContaining`, `ndsRelocPointerIsFighterAObj16`) are sufficient, and the
positive proof is `gNdsRelocResolveOffsetCount` 0 → **3,132** with
`gNdsRelocResolveMisalignCount 0` and `gNdsObjAnimRunawayCount 0`: the resolver is taking the
blob-relative-offset branch thousands of times and refusing none of them.

### What Boundary now fails at flag 1, and it is not ours to hand-wave

```text
Exception: verify-battle-mariofox-gcrunall-loop-harness.ps1:396
  battle_playable lower-screen rolling FPS counter did not sample actual presentation cadence.
  FPS_HUD=289,14,15,16856768          <- byte-identical on two consecutive runs
```

The assert recomputes `fps_x10` from the frame/tick window published beside it:
`floor((15 × 33,513,982 × 10 + 8,428,384) / 16,856,768) = 298`, against a published **289**.

**Attribution, sharper than the prior record.** 289 with 15 frames requires a tick window of
≈17,394,800; 16,856,768 is published beside it. Both are valid windows (the sample threshold is
`BUS_CLOCK/2 = 16,756,991`), so **`X10` lags `FrameWindow`/`TickWindow` by exactly one sample**.
It is not a wrong `BUS_CLOCK`: solving for the constant that makes 289 consistent with
(15, 16,856,768) gives 32.42–32.53 M, which is no clock this machine has.

What was ruled out this cycle, cheaply:
- **A second writer.** `gNdsBattlePlayableHudFpsX10` and `…TickWindow` have exactly one
  non-reset writer each (`nds_platform.c:2371`, `:2374`), inside one `REG_IME = 0` block.
- **A cache-line straddle making a gdb read see half-flushed memory.** All four globals sit in
  one 32-byte line on this arm (`0x02104f40..0x02104f4c`, offsets 0/4/8/12).

This is the R2-04 E2 assert, recorded as *intermittent and unexplained* with prior sightings of
exactly this shape (`299,14,15,17421760`; `290,…,15,17485504`) — **at flag 0**. What is new is
that the flag-1 arm reproduces it **deterministically**, and a harness assert that names the
same four values twice is a correctness signal, not noise. Whether the pack's cadence causes
the tear or merely exposes it is unresolved. `NDS_R204_FPSHUD_SHADOW` exists for exactly this
question and was not spent.

**Boundary at flag 0 is GREEN on this tree** (`Boundary verification profile passed.`,
exit 0) — so the source changes did not disturb the shipping default.

---

## 3. Task C — engagement on a live ROM, with a control that reads 0

Gate arm `NDS_R2_BOTH_CPU=1`, target `smash64ds-battle-playable-proof-hwtri`, canonical
one-minute match, **2,043 presented frames on both arms**, DLDI on, `NO-FREEZE`.
Builds `build-c163-gate-bp0` / `build-c163-gate-bp1`; logs beside this file.

| counter | flag 0 | flag 1 |
|---|---:|---:|
| `gNdsBattlePackHits` | **0** | **197** |
| `gNdsBattlePackMisses` | 357 | 160 |
| **total acquisitions** | **357** | **357** |
| `gNdsBattlePackLoadSteps` | (absent) | 18 |
| `gNdsBattlePackLoadFails` | (absent) | 0 |
| `gNdsBattlePackDrops` | (absent) | 1 |
| `gNdsR2AnimCacheHits` | 338 | 30 |
| `gNdsR2AnimCacheFills` | 19 | 4 |
| `gNdsR2AnimCacheRejects` | 0 | 126 |
| `gNdsR2AnimCacheArenaReservedBytes` | 262,144 | 292,032 |
| `gNdsR2AnimCacheArenaReserveFailCount` | 0 | 0 |
| `gNdsTaskmanArenaChosenSize` | 1,376,256 | 1,376,256 |
| `gNdsTaskmanArenaAllocFailCount` | 0 | 0 |
| `gNdsObjAnimRunawayCount` | 0 | 0 |
| `gNdsRelocResolveMisalignCount` | 0 | 0 |

**The totals are identical.** 357 acquisitions on both arms: the deletion changes what an
acquisition costs, not how many happen. 197 of 357 (55.2%) are served from the pack, and the
flag-0 arm's `gNdsBattlePackHits 0` is a control that *could* have been non-zero — the counter
is compiled in on both arms and `Misses` proves the site executes 357 times there.

A second control fell out of the failed first attempt (§1): same flag-1 ROM, pack **not**
resident, Boundary-style arm — `Hits 0 / Misses 221`; with the carve, `Hits 169 / Misses 52`.
Same binary configuration, same match, only the reserve differs.

### Per-fighter acquisition split, measured

| arm | packed (Fox) | generic (Mario) | pack share |
|---|---:|---:|---:|
| gate, both CPU | 197 | 160 | 55.2% |
| Boundary-style, human Mario | 169 | 52 | 76.5% |

### The K0 after-GO assertions — what is and is not proven

On a pack hit, `ndsRelocForceLoadFighterAObj16File` returns before the `memcpy`, the alias
strip, `ndsRelocRegisterLoadedFile`, the whole `ndsRelocFinalizeLoadedFile` chain (internal
fixups, AObj16 normalization, attribute/weapon normalization, external fixups, sprite pass) and
`ndsRelocAssetGetPath`. All seven K0 counters are therefore zero **on that path by
construction**, in the same function, and `gNdsBattlePackHits 197` counts exactly the times it
was taken.

**Not proven: the measured, GO-gated, per-fighter zero.** The ROM carries no counter that
splits animation I/O by fighter or by before/after GO, so the phase-7 assertion as §K1 words it
has not been taken. That needs a counter set and a build; it is next cycle's, and it should be
built as a *fail-closed* assertion rather than a printout.

---

## 4. What this cycle did NOT do

- **No gate measurement.** The **−73,659 at rank-80 remains a projection** on the profile arm.
  Phase 8 is untouched, deliberately.
- **No phase-6 oracle** (Task D). Phase 5 still introduces no new evaluator: the same generic
  parser runs the same bytes from a different address. The oracle belongs with the evaluator
  slice, and its design constraint is already recorded — the phase-4 test scored mismatch 0
  while shipping the wrong bit order **because both arms shared one decoder**, so any oracle
  must first name every component the two arms share and state which defect classes that
  sharing makes invisible.
- **The FPS-HUD tear is not root-caused.** Two hypotheses killed, mechanism open.
- **No per-fighter or after-GO I/O counters.**
- **`gNdsTaskmanGeneralHeapFreeMin` was not captured**; it is not in the soak's global list, and
  the "general heap free bytes" the soak prints is the Results scene, not the battle low-water.
  The arena arithmetic is therefore checked by `ReserveFailCount 0` and `AllocFailCount 0`
  rather than by the low-water directly. Add it to the list next cycle.

---

## 5. The trade this landed, stated plainly

Only one fighter fits, so the other loses the raw-file cache:

```text
gNdsR2AnimCacheRejects       0 -> 126     (gate arm, whole match)
gNdsR2AnimCacheHits        338 ->  30
```

Every one of those 126 is an acquisition that streams from ROM where it used to hit a cache.
It is a **performance** outcome and never a correctness one — every failure path in this
subsystem degrades to the on-demand load — but it is real and it is unpriced.

**A wall-clock signal exists and it points the wrong way.** Proof arm, same target, same
1-minute match, 2,043 presented frames both, only the reserve fix differing:

```text
gNdsBattlePlayablePacingVBlanks   4,274  (pack NOT resident)  ->  4,805  (resident)
present-interval buckets [4]/[5]      5 / 12                  ->      42 / 108
```

+12.4% battle wall time. Candidate owners: the 18 streamed chunks (bounded, ~18 frames), the
126 uncached loads, and cross-build placement. **Not attributed.** This is the first thing
phase 8 must resolve, and if it holds, the answer is not to shrink the pack — it is to grow
`NDS_TASKMAN_ARENA_SIZE` out of the 146,560 B `RAM_RECOVERY_PLAN` Phase 2 recovered so **both**
fighters can be resident and neither loses its cache.

`BLOCKED(decision: …)` is **not** raised for this: which pool the pack lands in is engineering,
not a fidelity trade, and the flag stays default 0 until it is measured.

---

## 6. Reproduction

```powershell
# lab arm
$env:NDS_R2_BATTLEPACK='1'
make TARGET=smash64ds-battle-playable-proof-hwtri BUILD=build-c163-battlepack

# engagement, both arms (drop the env var for the control)
.\scripts\soak-freeze-watch.ps1 -Build build-c163-gate-bp1 `
    -Target smash64ds-battle-playable-proof-hwtri -BothCpu $true `
    -MatchMinutes 0 -MinutesToRun 2.5

# Boundary.  verify-all.ps1 writes child output with [Console]::Out.Write, so
# NO PowerShell redirection captures it -- use an OS-level redirect, and pwsh,
# because `powershell` is 5.1 and the repo's scripts use PS7 syntax.
cmd /c "pwsh -NoProfile -ExecutionPolicy Bypass -File scripts\verify-all.ps1 -Profile Boundary > %TEMP%\b.log 2>&1"
```

## 7. Root ROMs

Unchanged across the cycle — no published target was built.

```text
smash64ds.nds                          54c07fac80c50418949908701f7c2bdbf27512c5f96ac09086fabbb0df6ac68a
smash64ds-battle-playable-hwtri.nds    2015fbd1f68b81c03626d8c6d473c8bcbcf527a3a26dfe86ff19bd74ecbb1360
```
