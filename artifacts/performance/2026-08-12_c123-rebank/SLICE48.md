# Slice 48 — the BGM worker's priority, and a 94,976-tick placement lesson

Cycle 124, on the c123 bank (`build-c123-warm`, `WORK-H` P50 938,752 / P95
**1,196,224**, 1600 samples from frame 438, `-RingDump`, DLDI ON,
`NDS_R2_BOTH_CPU=1`). **Gap to 1,120,380: 76,804.**

This slice found a lever worth roughly **−15,000**, refuted its own headline
hypothesis, and — more valuably — showed that a cross-build comparison on this
ROM can carry a **94,976-tick placement term**, 18x the recorded ±5,376 floor.

**Read this before quoting the shipped number.** `build-c124-slice48` measures
`WORK-H` P95 **1,087,296**, which is under the 1,120,380 gate. Do not read that
as 108,928 of engineering: `create27` restores the c123 bank's exact behaviour
and already measures 1,101,248, so **~94,976 of the gap is a lucky link and will
evaporate on the next unrelated edit**. The gate is not stably met.

## How the lane was found, and the bucket trap underneath it

The c123 top-80 attribution put a FAT family on **49 of the 80 costliest
frames**: `armCopyMem32` 27,331 + `get_fat` 16,418 + `f_lseek` 10,375 +
`_dvmDiscCacheReadWrite` 4,552 = **58,676 cyc/frame ≈ 29,338 tk**, 96%/41%
memory-stalled. Real cartridge reads inside gameplay frames.

Two candidates could own it, and counters separate them cleanly on the gate arm
(no build, `-ExtraGlobals`):

```
gNdsAudioBgmRefillCount      104     gNdsAudioBgmWorkerWakeCount  104
gNdsR2AnimCacheHits          381     Misses  2   Fills  2   Rejects 0
gNdsR2AnimWarmLoaded          85     WarmFailed 0   ArenaUsed 192,240 of 262,144
gNdsRelocAssetPayloadReadCount 123   HeapDecline 0   HeapFreeMin 70,776
```

Slice 46 took anim-cache misses **32 → 2**, and 85 of the 123 payload reads are
the warm preload, which finishes around frame 22 — before the window opens at
438. So at most ~38 asset reads are in-window against **104 BGM refills**, and
`RefillCount == WorkerWakeCount` exactly, pinning every refill to the worker
thread. **The lane is BGM.**

**The trap, and it cost a published conclusion.** An earlier pass killed this
lane by reading `AUD` — P50 2,496, P95 6,976, share 0.2% — as BGM's total cost.
That inference is invalid. **A tick-HUD bucket only sees its own thread.** `AUD`
brackets the main thread's `ndsAudioBgmUpdate`; the worker ran at
`MAIN_THREAD_PRIO - 1`, *above* main, and calico is explicit that "higher
priority threads are always guaranteed to preempt the current thread". Its
cycles therefore land in whatever bucket main happened to be inside — `SRC`,
`MISC`, anywhere but `AUD`.

## The hypothesis, and its refutation

If the worker preempts the gameplay frame, moving it BELOW main should push the
read into the VBlank idle (`WAIT` P50 207,104 ticks) and take ~29,338 tk off the
tail. The seam budget is ~186 ms against a 16.7 ms frame, so the read has ~11
VBlank windows to finish — safe by 11x.

Route-gated on `.data aligned(32)` so both arms come from one binary. **Two
harness failures had to be cleared first:**

1. **The first control was the candidate relabelled.** `-SetGlobals` fires at the
   first frame-complete marker, but the worker's priority is consumed once, at
   thread creation, when BGM starts — earlier. Poking 27 set the global and
   changed nothing: both arms returned `WORK-H` P95 **1,102,208 to the byte**.
   `gNdsAudioBgmWorkerPrioApplied`, added for exactly this, read 29 while
   `gNdsAudioBgmWorkerPrio` read 27. A `.data` route is only an A/B if the value
   is **re-read** after the poke lands; `ndsAudioBgmApplyWorkerPrio` now does.
2. **Comparing the fixed control against the old candidate is a cross-build
   comparison** — the thing the route exists to eliminate. Both arms must be the
   same binary.

With both fixed, on ONE binary (`build-c124-bgmprio2`), arms by `-SetGlobals`:

| arm | prio in window | `PrioApplied` | Refills | SeamMiss | P50 | P95 |
|---|---|---:|---:|---:|---:|---:|
| control (preempting) | 27 | 27 | 99 | 0 | 897,088 | **1,083,456** |
| candidate (below main) | 29 | 29 | 98 | 0 | 896,128 | **1,091,520** |
| delta | | | | | −960 | **+8,064** |

**Refuted.** Deprioritizing the refill *during the match* costs 8,064. The read
is not removed from the tail by scheduling it later; it is simply done later.

## The 94,976 that was not a win

Both new builds beat the bank by ~100K in `SRC` (452,736 / 456,896 vs 553,344,
and ~50K at P50), which reads like a huge win and is not one. The two arms above
share one property the bank does not: the worker is **created** at 29, because a
poke can never precede creation.

`build-c124-bgmprio-create27` isolates that. It flips **only the initializer**
back to `MAIN_THREAD_PRIO - 1`, restoring the bank's exact behaviour. The two
ROMs differ in **41 bytes** — the `.data` word plus build stamps — so placement
is identical:

| build | creation | in-window | P95 | placement |
|---|---|---|---:|---|
| `build-c123-warm` (bank) | 27 | 27 | **1,196,224** | A |
| `build-c124-bgmprio-create27` | 27 | 27 | **1,101,248** | B |
| `build-c124-bgmprio2` (cand) | 29 | 29 | 1,091,520 | B |
| `build-c124-bgmprio2` (ctl) | 29 | 27 | **1,083,456** | B |
| `build-c124-slice48` (**ships**) | 29 | 27 | **1,087,296** | C |

**`create27` reproduces the bank's behaviour and measures 94,976 less.** Same
priorities, same `SeamMiss` 0, ~50 bytes of added code. It is not a cache-state
story either — the faster build has **more** anim-cache misses (14 vs 2) and
**more** payload reads (135 vs 123). The plausible cause is the added
`aligned(32)` `.data` object shifting every later `.data` object's cache-line
mapping.

**So ~94,976 of the apparent 112,768 is placement, and must not be counted as
headroom against the gate.** Published as a re-bank this slice would have read
−112,768; its actual lever is −17,792.

## What ships, and why the split

The two effects pull opposite ways:

- creating the worker **below** main is worth **−13,952 to −17,792** — two
  independent pairs against `create27` (→ 1,083,456 and → 1,087,296), same sign
  and magnitude. **Cross-build, so weakly held**: the placement term measured
  immediately above is up to 94,976, which swamps it. Take it as "the right sign,
  probably worth ~15K", not as a banked figure.
- running it **below** main during the match **costs +8,064** — this one is
  same-binary and solid.

Setup is where the anim warm preload and the asset loads run, and a
higher-priority cartridge reader interleaving with them is the only difference
between those arms. So the shipped configuration is **created low, run high**:

- `gNdsAudioBgmWorkerPrio` = `MAIN_THREAD_PRIO + 1` (creation)
- `gNdsAudioBgmWorkerRunPrio` = `MAIN_THREAD_PRIO - 1` (applied once BGM updates)

Both are `.data aligned(32)` and pokeable, and `gNdsAudioBgmWorkerPrioApplied`
reports what was actually applied, so the next agent can re-run this A/B without
a build. No audio fidelity is involved at any point: the same bytes are read at
the same rate, and `SeamMiss`/`Overrun`/`UnsafeWrite` are 0 in every arm above.

## Rules this slice earned

1. **A tick-HUD bucket only sees its own thread.** A preempting worker's cost
   lands in the interrupted thread's bucket. `AUD` at 0.2% did not mean BGM was
   cheap.
2. **A `.data` route is only an A/B if the value is re-read after the poke.**
   `-SetGlobals` fires at the first frame-complete marker; anything consumed at
   init needs re-application and an `…Applied` readback to prove it.
3. **The ±5,376 cross-build floor bounds a near-identical pair only.** Once a
   change adds an object — especially an over-aligned one — budget the placement
   term in the tens of thousands. Size with a route; re-bank only reports what
   the ROM measures.

## Files

- `bgm.{json,log}`, `cache-now.{json,log}` — the counters that identified the lane.
- `prio-cand.{json,log}`, `prio-ctl.{json,log}` — the invalid first A/B (`PrioApplied` 29 on both).
- `prio2-ctl.{json,log}`, `prio2-cand.{json,log}` — the valid same-binary A/B.
- `create27.{json,log}` — the placement isolation.
- `slice48-ship.{json,log}` — the shipped configuration.
