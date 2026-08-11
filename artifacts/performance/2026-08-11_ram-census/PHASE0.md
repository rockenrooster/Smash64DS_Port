# RAM recovery — Phase 0 baseline (re-banked at HEAD)

Plan: `docs/RAM_RECOVERY_PLAN.md`. HEAD `1ce4c5f4dee`, shipping P1 configuration
rebuilt from source (the root ELF was stale by two slices — it predated 36/37 —
and the plan forbids banking off a stale ELF).

## 0.1 Static image — `smash64ds-battle-playable-hwtri.elf`

| section | addr | bytes |
|---|---|---:|
| `.main` | 0x020038e8 | 903,336 |
| `.main.rw` | 0x020e0190 | 136,996 |
| `.main.bss` | 0x021018e0 | **1,599,280** |
| `.itcm` | 0x01ff8020 | 29,800 |
| `.dtcm` | 0x02ff0000 | 8,800 |
| `.dtcm.bss` | 0x02ff2260 | 152 |
| **main total** | | **2,639,612** |

`fake_heap_start` 0x02288004 · `__heap_start_ntr` **0x02288010**

`.main.bss` is 1,536 B larger than the pre-rebuild census (1,597,744): that is
slices 36/37's memo tables, and it is why the rebuild was required.

## 0.2 Runtime state — one-minute Boundary match, 1600 frames from 438

Read with `-ExtraGlobals` on `builds/build-tick-hud-buckets`. Artifact:
`phase0-runtime.json`. **Shipping arm (NOT `NDS_R2_BOTH_CPU`).**

| counter | plan snapshot | MEASURED |
|---|---:|---:|
| `gNdsTaskmanArenaChosenSize` | 1,245,184 | **1,257,472** |
| `gNdsTaskmanArenaAllocFailCount` | — | **29** |
| `gNdsTaskmanGeneralHeapFreeMin` | 42,136 | **15,120** |
| `gNdsR2AnimCacheArenaReservedBytes` | 200,704 | 200,704 |
| `gNdsR2AnimCacheArenaUsedBytes` | 200,400 | 200,384 |
| `gNdsR2AnimCacheArenaOverflows` | 38 | **11** |
| `gNdsR2AnimCacheRejects` | 38 | **11** |
| `gNdsR2AnimWarmLoaded` | 83 | 83 |
| `gNdsOriginalDLPreviewCommitCount` | — | **0** |

### Two corrections that change the plan's arithmetic

**1. The 32 KiB safety floor is ALREADY BREACHED at baseline.**
`gNdsTaskmanGeneralHeapFreeMin` is **15,120**, not 42,136 — **17,648 B BELOW**
the plan's non-negotiable 32,768 floor. So "discretionary low-water slack
~9,368 B" is wrong; slack is **negative**. Nothing may grow until RAM is
recovered, and recovery is now required merely to restore invariant #3 rather
than to fund a bigger cache. Every "spend the slack" option in the plan's
starting snapshot is off the table.

**2. Reject count is arm-dependent — 11 on Boundary, 38 on both-CPU.**
The 38 that motivated this campaign came from the `NDS_R2_BOTH_CPU` stress arm.
Boundary — the canonical Mario-human-vs-level-3-Fox configuration — refuses
**11**. `ANIM_REQUIRED_BYTES` must therefore be derived per arm and the shipping
figure quoted against Boundary. Both are still non-zero, so the defect stands;
only its size changes.

The arena also settled at 1,257,472 after **29** failed probes, i.e. the
allocator is already probing down to what main RAM leaves it. That is the
mechanism by which recovered static bytes convert to arena bytes — and it
confirms the conversion is real rather than assumed.

## 0.3 `ANIM_REQUIRED_BYTES` — NOT yet derived

Deferred: the per-request classification the plan asks for (asset id, requested
vs aligned bytes, resident hit / first miss / rejection / repeat-of-rejected,
first frame) needs instrumentation that does not exist yet. Do not carry the old
82 KiB estimate into a cache constant — with Boundary at 11 rejects rather than
38, that number is very likely too large.

## Phase 1 proof — the DL preview pair is dead in this configuration

Three independent lines, all agreeing:

1. **Reader condition.** The only code that reads `sOriginalDLDisplayPreview`
   or the preview width/height statics is `nds_platform.c:1877-1980`, entirely
   inside `#if !NDS_RENDERER_HW_TRIANGLES`. P1 ships hardware triangles, so the
   buffers are **write-only** — filled by the opening room, displayed by nobody.
2. **Linked-ELF attribution.** Source has seven call sites for
   `ndsPlatformBeginOriginalDLPreview`; exactly **one** survives into the
   shipped battle ELF (`ndsOpeningRoomRenderDLPreview`). An exhaustive scan over
   every branch mnemonic confirms it, and the only functions holding an address
   inside either array are the three API entry points — so nothing was inlined
   and no generic helper receives the pointer.
3. **Runtime, lifetime-wide.** `gNdsOriginalDLPreviewCommitCount` = **0** across
   the whole one-minute match. A cumulative counter reading zero at the end is
   lifetime-wide proof, which is what the plan asked for instead of a ready flag
   sampled at one instant.

---

# Phase 1 RESULT — DL preview pair removed

Implementation: storage, the display-preview builder, and the three API bodies
follow `#if !NDS_RENDERER_HW_TRIANGLES` — the **same condition already on their
only reader**. `ndsPlatformBeginOriginalDLPreview` returns NULL on hwtri, which
is not a new failure mode: `ndsOpeningRoomRenderDLPreview` already handles NULL
by setting `NDS_OPENING_ROOM_DL_PREVIEW_BLOCKER_NO_PIXELS` and returning.

## Static (shipping P1 ROM)

| metric | before | after | delta |
|---|---:|---:|---:|
| `.main.bss` | 1,599,280 | 1,577,648 | **−21,632** |
| `.main` | 903,336 | 902,944 | −392 |
| total | 2,639,612 | 2,617,588 | **−22,024** |
| `__heap_start_ntr` | 0x02288010 | 0x02282a10 | −22,016 |

Clears the plan's KEEP gate (>=20 KiB) with 2,024 B to spare.

## Runtime (one-minute Boundary, 1600 frames from 438)

| counter | before | after | delta |
|---|---:|---:|---:|
| `gNdsTaskmanArenaChosenSize` | 1,257,472 | 1,277,952 | **+20,480** |
| `gNdsTaskmanArenaAllocFailCount` | 29 | 24 | −5 |
| `gNdsTaskmanGeneralHeapFreeMin` | 15,120 | **35,324** | **+20,204** |
| `gNdsR2AnimCacheArenaUsedBytes` | 200,384 | 200,384 | 0 |
| `gNdsR2AnimCacheRejects` | 11 | 15 | **+4** |

**The static→arena conversion is real, not inferred.** The allocator climbed a
rung (+20,480) and needed five fewer probes. This is the mechanism the whole
campaign depends on, now demonstrated rather than assumed.

**Invariant #3 is restored.** Heap low-water 15,120 → 35,324, i.e. from 17,648
BELOW the 32,768 floor to 2,556 above it. Phase 1 alone converts an
already-violated safety invariant back into a satisfied one.

## Open: rejects 11 -> 15

Flagged rather than waved away — the plan's revert rules list increased post-GO
file reads as a stop condition. It cannot be a behavioural consequence of the
change: `gNdsOriginalDLPreviewCommitCount` was **0** before the edit, so the
removed path never executed at runtime in this ROM. That leaves binary layout or
harness run-to-run variance. Being decided by re-running the IDENTICAL binary
(`phase1-repeat.json`); a differing repeat means the counter is not stable enough
to gate on, an identical repeat means +4 is real and needs a cause.
