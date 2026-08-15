# The mechanism has a name, a site and a fix: the resolver asked the expensive question first

**Date:** 2026-08-15 · **Branch:** `codex/r2-runtime2` · **base HEAD `98508e7ad25`**
**Native Battle Kernel slice 1 — what makes the pack path cost +2,261,760 at rank-80.**
Predecessor: `…/2026-08-15_battlepack-isolation/BATTLEPACK_ISOLATION.md`.
**Builds spent: 3** (falsifier, v2 profile arm, seam-fix arm) **+ 1 default-config check.**
Root ROMs unchanged (§8).

---

## 0. Outcome first

```text
VERDICT     A BUG WITH A NAMED SITE.  It is not the pack format, not the RAM,
            not residency, not the raw cache, and not the acquisition count.

THE SITE    ndsRelocResolvePointerFromFileBase, src/port/reloc_backend_assets.c.
            It probes "is `ptr` ALREADY an absolute pointer into a known file?"
            BEFORE it interprets `ptr` as a file-relative offset.  For a clip
            served from the pack that probe can NEVER succeed -- the generator
            emits blob-relative offsets, so `ptr` is a small integer -- and its
            MISS path is ndsRelocFindKnownFileContaining falling through into
            ndsRelocFindStatusNodeContaining over BOTH status buffers, whose
            per-node ndsRelocStatusNodeDataSize runs ndsRelocAssetIDForToken.

THE PRICE   v2 per-PC profile, pack arm, 641 presented frames:
            ndsRelocAssetIDForToken          207,877,919 cyc  99.8% on 69 frames
            ndsRelocFindStatusNodeContaining 113,559,597 cyc 100.0% on 69 frames
            = 250,731 tk/frame mean, 2,329,254 tk on each acquisition frame,
            against the gate arm's measured +2,261,760 at rank-80.  Two
            instruments, two arms, two windows: they agree to 3.0%.

TASK A      ANSWERED.  Pack resident (State READY, 287,904 B, 18 load steps) with
            dispatch OFF (Hits 0 against a control that reads 197):
            rank-80 1,222,464 vs the isolation arm's 3,447,872 -- the cost is
            GONE.  Presence is +36,352 over the no-pack control, 1.6% of the
            effect and inside its own 34 cache rejects.  It is the DISPATCH.

THE FIX     Ask the cheap question first: if `file_base` is in the pack, resolve
            the offset directly and never run the absolute-pointer probe.
            build-c168-packfix-bp1, same arm as C, same 355 acquisitions, same
            damage 0/76, and gNdsRelocResolveOffsetCount 3,629 == 3,629:
            rank-80  3,447,872 -> 1,170,048   (-2,277,824)
            mean     1,196,937 ->   955,581   (-241,356)
            >2M            128 ->         1
            versus the NO-PACK control: P50 -1,568, rank-80 -16,064, mean -4,959,
            over-gate 135 -> 123.  Five statistics, one sign.

AT THE SHIPPING ARENA (build-c169, cache carved to 4,096, Rejects 126) the fix
            still pays -2,011,584 against arm B, but lands +249,792 over the
            no-pack control.  H - G = +265,856 prices the CARVE, now that nothing
            else is in the way.  Section 9's displacement constraint is the
            binding question again: the blob is 1.098x what it evicts.

SLICE 1     BACK ON THE TABLE.  The isolation cycle's "slice 1 does not pay" was
            measured on a build carrying this defect.  The architecture was never
            what cost; it was one ordering in one port-side helper.  -73,659 stays
            retracted -- it was a projection and the measurement is a wash, not a
            win -- but "+2,261,760, refuted as a P95 lever" is now withdrawn.
```

---

## 1. What the 128 frames had in common — and it is not a fighter or a clip

The isolation arm's 128 expensive frames are not clustered: 120 runs, 113 of them
a single frame, spread from presented frame 453 to 1995 with a broad gap
histogram (mode 5–6 frames). Per-frame brackets say why no clustering was ever
going to appear:

| frame | `WORK-H` | `SINT` | `SPHD` | `SCAT` | `SPRM` | `STG` | `MISC` |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 453 | 2,699,584 | **1,835,776** | 89,536 | 1,408 | 2,048 | 172,544 | 118,144 |
| 457 | 2,889,664 | 246,272 | **1,887,040** | 1,216 | 1,792 | 173,056 | 116,864 |
| 909 | 7,245,056 | 2,308,416 | — | 1,344 | 1,856 | **4,367,680** | 81,984 |
| 1632 | 5,010,112 | 473,728 | — | **3,756,864** | — | — | — |
| 1913 | 4,880,192 | 281,216 | — | — | **3,429,888** | — | — |
| 1954 | 5,045,824 | 119,872 | — | 1,408 | 1,856 | 180,224 | **4,347,392** |

The excess lands in **whichever bracket is open**, including the draw side. That
is the signature of a call made from many different procs, and
`gcParseDObjAnimJoint`/`ftAnimParseDObjFigatree` reach the figatree attach from
`gcPlayAnimAll`, `ftParamUpdateAnimKeys`, `ftCommonGuardUpdateJoints`,
`mpCollisionPlayYakumonoAnim` and `gcGetDObjTempAnimTimeMax`. **Chasing a bracket
would have chased the wrong thing.** The common factor is the *acquisition*, not
the proc.

---

## 2. The instrument, and one trap it cost

`build-c167-profile-bp1` — pack + `KEEP_CACHE`, `BOTH_CPU=1`, **`DRAW=0`**,
`NDS_TASK37_PROFILE=1` with per-frame regions, window = presented frames
438..1078 (641 regions, 1.06 GB, 22,197,172 PC rows, 1,864,312,104 cycles).

**The expensive frames reproduce on the `DRAW=0` arm, at the same game moments.**
Regions over 4.0M cycles map to presented frames 452, 456, 464, 471, 476, 482,
488, 518, 520, 540, 542, 549, 569, 571 … — the gate arm's 453, 457, 465, 472,
477, 483, 489, 519, 521, 541, 543, 550, 570, 572 … offset by exactly one
(`region r = presented frame 438 + r` against the sampler's 439-based window).
**The tick-HUD draw instrument is not involved in this at all.**

69 of 641 regions (10.8%) hold 520,985,694 cycles = **27.9%** of the window.
Work excess on them, idle removed: **5,067,262 cycles = 2,533,631 ticks each**.

> **THE TRAP, AND IT IS NOW STRUCTURAL.** This capture is **v2, not v3**:
> `-RunnerSlot` silently overrides `-MelonDS`, because
> `Resolve-MelonDSRunnerSlot` (`scripts/lib/melonds.ps1:542`) ignores the
> parameter entirely for slot ≥ 0 and returns
> `emulators\melonds-runners\slotN\melonDS.exe`. Asking for
> `emulators\melonds-attributor` *with* a slot therefore produced a capture with
> **no stall columns at all** while every banner still said "census", and the
> failure surfaced downstream as `KeyError: 'halt_wait'`.
> `run-task37-profile-census.ps1` now **throws** on that invocation, and
> `census-marginal-frame-owners.py` names the real fault instead of raising a
> KeyError. **The verdict does not turn on it** — cycles and instructions are
> enough to name a function that is 138x its control — but the stall-class split
> of those cycles was not taken and is not claimed.

---

## 3. The mechanism, per PC, masked to the frames that carry it

Mask = the 69 regions over 4.0M cycles (the two-VBlank quantum is 2,240,760, so
this mask is not sorting rounding noise; the ranked frames are 1.8x–6.0x it).

| symbol | all cycles | masked cycles | masked / all | tk/frame | masked insns |
|---|---:|---:|---:|---:|---:|
| **`ndsRelocAssetIDForToken`** | 207,877,919 | 207,366,743 | **99.8%** | 162,151 | 144,533,495 |
| **`ndsRelocFindStatusNodeContaining`** | 113,559,597 | 113,559,597 | **100.0%** | 88,580 | 81,785,164 |
| `ndsRelocFindLoadedFileContaining` | 4,168,059 | 2,209,792 | 53.0% | 3,251 | 1,136,968 |
| `lbCommonAddFighterPartsFigatree` | 868,118 | 628,395 | 72.4% | 677 | 88,256 |
| `ndsBattlePackContains` | 730,443 | 518,231 | 70.9% | 570 | 135,964 |
| `ndsRelocFindKnownFileContaining` | 500,842 | 397,122 | 79.3% | 391 | 90,852 |
| `ndsRelocResolvePointerFromFileBase` | 433,186 | 293,067 | 67.7% | 338 | 89,808 |
| *every other symbol* | — | — | **≈10.8%** | — | — |

10.8% is the base rate (69/641). **Every other function in the binary sits at
it.** Only these are concentrated. `armWaitForIrq` reads 16.2% — idle, and the
expensive frames idle less, which is the arithmetic complement.

The three concentrated functions hold **323,136,132 cycles on the masked frames
against a total work excess there of 349,641,078 — 92.4%.**

`ndsRelocStatusNodeDataSize`, `ndsRelocFindLoadedFileByAsset`,
`ndsRelocFindLoadedFileByData` and `ndsRelocPointerRangeInBuffer` never appear:
they are inlined into the two owners above, which is exactly the case the
campaign's "rank the inline attribution" rule exists for — a symbol census here
is *not* misleading only because the inlining folds INTO the owner rather than
out of it.

### 3.1 The same two functions on the control

`build-c159-profile-bothcpu`, v3, `BOTH_CPU=1`/`DRAW=0`, 1,601 regions —
the pack-OFF arm, from `…/2026-08-14_runtime2-p95-closure/gate-p95-pc.csv`:

| symbol | control cyc/frame | control tk/frame | pack tk/frame | ratio |
|---|---:|---:|---:|---:|
| `ndsRelocAssetIDForToken` | 2,352 | 1,176 | **162,151** | **138x** |
| `ndsRelocFindStatusNodeContaining` | 0 (absent) | 0 | **88,580** | — |
| `ndsRelocFindLoadedFileContaining` | 4,004 | 2,002 | 3,251 | 1.62x |

### 3.2 Why, in source

`ndsRelocResolvePointerFromFileBase` (`src/port/reloc_backend_assets.c`) ran, in
this order:

1. `ndsRelocFindKnownFileContaining(ptr, size, …)` — *"is `ptr` already
   absolute?"*. On a pack slot word this **always misses**, and a miss is:
   `ndsBattlePackContains` (O(1), FALSE) → `ndsRelocFindLoadedFileContaining`
   (scan) → `ndsRelocFindStatusNodeContaining(force buffer)` →
   `ndsRelocFindStatusNodeContaining(status buffer)`.
2. `ndsRelocFindKnownFileContaining(file_base, 1u, &base, &data_size)` — *"which
   file owns this?"*. On a pack clip this hits `ndsBattlePackContains`
   immediately and is **O(1)**.

`ndsRelocFindStatusNodeContaining` is a linear scan whose body is not cheap:

```c
for (i = 0; i < count; i++) {
    size_t data_size = ndsRelocStatusNodeDataSize(&nodes[i]);   /* <-- */
    …
}
```

and `ndsRelocStatusNodeDataSize` calls `ndsRelocFindLoadedFileByData`, then
`ndsRelocAssetIDForToken(node->id)`, then `ndsRelocFindLoadedFileByAsset` twice.
`ndsRelocAssetIDForToken` is a ~300-compare token chain whose **full miss also
walks the 143-entry Mario and 158-entry Fox pointer arrays** — the function's own
header block already records 1,003 cycles / 550 instructions per call and
"a full miss walks all 143 + 158 entries".

**So one figatree slot costs two complete status-buffer scans, and every node
visited in them runs that chain.** Measured, that is ~136 node visits per slot
(144,533,495 insns ÷ 550 insns/call ÷ 3,629 slot resolves ≈ 136) at ~1,003 cycles
each ≈ **136,000 cycles ≈ 68,000 ticks per slot**, and a fighter action change
resolves ~18 slots (3,629 resolves ÷ 197 pack acquisitions = 18.4).

> **This is what `gNdsRelocResolveOffsetCount` 0 → 3,629 was pointing at, and why
> the previous cycle was right to refuse to divide by it.** The division demanded
> ~104,000 ticks per resolve and was rejected as "not a plausible table-walk
> price". It is not a table walk. The measured price is ~68,000 ticks per
> resolve, and the reason it is that large is that each one drags 136 status-node
> visits behind it.

---

## 4. Task A — the slice-51 falsifier

`build-c166-nodispatch-bp1`: identical to the isolation arm except
`NDS_R2_BATTLEPACK_DISPATCH=0`, a `volatile u32` initialiser that makes
`ndsBattlePackFindFigatree` answer NULL. Runtime-gated rather than `#if`'d so
both arms carry the same instructions at the same addresses.

**Residency is unchanged and dispatch is proven dead against a control that is
non-zero:**

| counter | A control | C isolation | **F falsifier** |
|---|---:|---:|---:|
| `gNdsBattlePackState` | 0 | 1 (READY) | **1 (READY)** |
| `gNdsBattlePackBytes` | 0 | 287,904 | **287,904** |
| `gNdsBattlePackLoadSteps` | 0 | 18 | **18** |
| `gNdsBattlePackHits` | 0 | **197** | **0** |
| `gNdsBattlePackMisses` | 355 | 158 | 355 |
| total acquisitions | 355 | 355 | **355** |
| `gNdsRelocResolveOffsetCount` | 0 | 3,629 | **0** |
| `gNdsR2AnimCacheRejects` | 0 | 0 | 34 |
| `gNdsTaskmanArenaChosenSize` / `AllocFail` | 1,376,256 / 0 | 1,548,288 / 0 | **1,548,288 / 0** |
| `ReserveFailCount` · general-heap free-min | 0 · — | 0 · 52,864 | **0 · 52,864** |
| damage P0 / P1 | 0 / 76 | 0 / 76 | **0 / 76** |

Gate-arm figures, all four arms on one basis — `smash64ds-battle-playable-tickhud-hwtri`,
`BOTH_CPU=1`, `DRAW=1`, DLDI on, 1,600 samples, rank-80 of 1,600, apparatus 24,947:

| arm | P50 | P90 | **rank-80 raw / net** | top-1% | max | mean | >gate | >2M |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| **A** control, no pack | 940,416 | 1,097,920 | **1,186,112 / 1,161,165** | 1,570,944 | 2,300,928 | 960,540 | 135 | 2 |
| **B** pack, cache deleted | 939,712 | 1,540,032 | **3,447,488 / 3,422,541** | 6,118,208 | 7,252,800 | 1,219,250 | 271 | 130 |
| **C** pack + cache (isolation) | 940,128 | 1,216,832 | **3,447,872 / 3,422,925** | 6,175,104 | 7,245,056 | 1,196,937 | 218 | 128 |
| **F** pack resident, dispatch OFF | 941,600 | 1,120,576 | **1,222,464 / 1,197,517** | 1,612,480 | 5,216,320 | 976,696 | 160 | 7 |

```text
F - C   rank-80 -2,225,408   mean -220,241   >2M 128 -> 7
F - A   rank-80    +36,352   mean  +16,156   P50 +1,184   >2M 2 -> 7
```

**Presence-only cost is +36,352 at rank-80 — 1.6% of the effect — and the
falsifier carries 34 cache rejects the control does not** (with dispatch off the
163,840 B cache has to serve both fighters again), which is where its 7 remaining
over-2M frames and its `max` of 5,216,320 come from. Residency, streaming, the
carve and the arena are **not** the cost. **The dispatch is.**

Window is 440..2038 on this arm (the sampler warned about 5 label collisions at
ring seams out of 1,600; no payload repeats, so the iterations are distinct and
the percentiles stand — only the frame ids at those seams do not).

---

## 5. The fix, at the owning seam

`ndsRelocResolvePointerFromFileBase` now asks the **cheap** question first: if
`file_base` is inside the pack, resolve `ptr` as a blob-relative offset directly
and never run the absolute-pointer probe.

It cannot change a result. `data_size` is the blob extent (287,904 B) and every
DS address is ≥ 0x02000000, so a word that genuinely *is* an absolute pointer
fails the bound, falls through, and takes the original path — where
`ndsBattlePackContains` answers it O(1) anyway. `gNdsRelocResolveOffsetCount` and
`gNdsRelocResolveMisalignCount` keep their meanings and are the engagement check:
the fix must resolve *the same* slots, only faster.

The block is `#if NDS_R2_BATTLEPACK` rather than left to the compiled-out stub,
because there is no LTO here and a cross-TU call is not free at flag 0.

### Measured — `build-c168-packfix-bp1`, same basis as §4

| arm | P50 | P90 | **rank-80 raw / net** | top-1% | max | mean | >gate | >2M |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| **A** control, no pack | 940,416 | 1,097,920 | **1,186,112 / 1,161,165** | 1,570,944 | 2,300,928 | 960,540 | 135 | 2 |
| **C** pack, defect present | 940,128 | 1,216,832 | **3,447,872 / 3,422,925** | 6,175,104 | 7,245,056 | 1,196,937 | 218 | 128 |
| **G** pack, defect fixed | 938,848 | 1,086,336 | **1,170,048 / 1,145,101** | 1,510,784 | 2,182,016 | 955,581 | 123 | 1 |

```text
G - C   P50 -1,280   rank-80 -2,277,824   mean -241,356   over-gate -95   >2M  128 -> 1
G - A   P50 -1,568   rank-80    -16,064   mean   -4,959   over-gate -12   >2M    2 -> 1
```

**Engagement is identical to arm C, which is what makes this a cost delta and not
a different program:** `Hits` 197, `Misses` 158, 355 acquisitions, `LoadSteps`
18, `AnimCacheHits` 149, `Fills` 9, `Rejects` **0**, arena 1,548,288 /
`AllocFail` 0 / `ReserveFail` 0, general-heap free-min 52,864, damage **0 / 76**.
And the decisive one: **`gNdsRelocResolveOffsetCount` 3,629 = 3,629.** The fix
resolves exactly the same slots; only the path to them changed.
`gNdsRelocResolveMisalignCount` 0 and `gNdsObjAnimRunawayCount` 0 on both arms.

`SINT` max falls 6,454,592 → **810,176**; `WORK-H` max 7,245,056 → **2,182,016**.

> **HOW FAR TO PUSH THE `G - A` CLAIM.** −16,064 at rank-80 is close to the
> ≥14,080 cross-build floor and must not be banked as a P95 win on its own. What
> the five statistics jointly support is the weaker and sufficient claim: **with
> the defect fixed the resident pack is no worse than the no-pack control, and
> every statistic leans slightly its way** — P50 −1,568, mean −4,959, over-gate
> 135 → 123, >2M 2 → 1, max −118,912, all the same sign. `VERIFYING.md`'s rule
> (rank on P50 / mean / over-gate, not rank-80 alone) is satisfied.
> **The +2,261,760 is gone, and that part is not marginal at all.**

**This arm is still not shippable**: `KEEP_CACHE=1` grows `NDS_TASKMAN_ARENA_SIZE`
to 0x17A000 and Boundary pins `gNdsTaskmanArenaChosenSize == 1376256`. It
isolates the *defect*, not the product.

### 5.1 The fix at the SHIPPING arena — and the cache deletion becomes the whole residual

`build-c169-packfix-noarena-bp1` = `NDS_R2_BATTLEPACK=1` alone: arena
**1,376,256** (`AllocFail` 0, the Boundary-pinned value), general-heap free-min
**40,576** against the 32,768 floor, so the blob's carve leaves the raw cache at
4,096 B — `Rejects` **126**, `AnimCacheHits` 30, `Fills` 2, exactly arm B's cache
state. `Hits` 197 / 355 acquisitions / damage **0 / 76** /
`gNdsRelocResolveOffsetCount` **3,629**.

| arm | pack | defect | cache | arena | P50 | **rank-80 raw / net** | max | mean | >gate | >2M |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---:|
| **A** | no | — | 262,144 | 1,376,256 | 940,416 | **1,186,112 / 1,161,165** | 2,300,928 | 960,540 | 135 | 2 |
| **B** | yes | present | 4,096 | 1,376,256 | 939,712 | **3,447,488 / 3,422,541** | 7,252,800 | 1,219,250 | 271 | 130 |
| **C** | yes | present | 163,840 | 1,548,288 | 940,128 | **3,447,872 / 3,422,925** | 7,245,056 | 1,196,937 | 218 | 128 |
| **F** | resident, no dispatch | n/a | 163,840 | 1,548,288 | 941,600 | **1,222,464 / 1,197,517** | 5,216,320 | 976,696 | 160 | 7 |
| **G** | yes | **fixed** | 163,840 | 1,548,288 | 938,848 | **1,170,048 / 1,145,101** | 2,182,016 | 955,581 | 123 | 1 |
| **H** | yes | **fixed** | 4,096 | **1,376,256** | 938,784 | **1,435,904 / 1,410,957** | 6,401,536 | 988,452 | 190 | 8 |

```text
H - B   P50   -928   rank-80 -2,011,584   mean -230,798   >gate  -81   >2M 130 -> 8
H - A   P50 -1,632   rank-80   +249,792   mean  +27,912   >gate  +55   >2M   2 -> 8
H - G   P50    -64   rank-80   +265,856   mean  +32,871   >gate  +67   >2M   1 -> 8
```

**`H − G` is the price of the carve, and it is now measurable because nothing
else is in the way.** The two arms differ only in arena size and cache size; the
defect is fixed in both. **+265,856 at rank-80, +32,871 mean, >2M 1 → 8.**

> **THE NUANCE, STATED CAREFULLY BECAUSE IT LOOKS LIKE A REVERSAL AND IS NOT.**
> Phase 8 blamed the cache deletion for the +2,261,376. The isolation arm refuted
> that, correctly: arms B and C differ 40x in cache and landed 384 ticks apart,
> so the cache could not be what cost *then*. It was a **passenger while the
> defect dominated**. With the defect removed the passenger is the whole
> remaining fare: **the cache deletion prices at +265,856**, and it is what keeps
> the shipping-arena pack 249,792 above the no-pack control.
>
> **`§9`'s design constraint is therefore exactly right and is now the binding
> question**, where the isolation cycle had (reasonably, on its evidence) set it
> aside: *a resident pack must be smaller than the storage it displaces.* At
> 287,904 B against 262,144 B the Fox blob is 1.098x its own displacement. Close
> that and the shipping arm becomes arm G.

---

## 6. What this cycle did NOT do

- **No v3 stall capture.** §2 records why and what it costs: the split of those
  cycles across `issue` / `icache_fill` / `dcache_fill` was not taken. The
  function-level attribution did not need it.
- **No Boundary run.** §7 proves zero shipped bytes changed by ELF section
  compare, which is the method the previous cycle established.
- **No default flip.** `NDS_R2_BATTLEPACK`, `NDS_R2_BATTLEPACK_KEEP_CACHE` and
  `NDS_R2_BATTLEPACK_DISPATCH` are 0 / 0 / 1 and the first two stay 0. The
  `KEEP_CACHE` arm still grows `NDS_TASKMAN_ARENA_SIZE`, and Boundary pins
  `gNdsTaskmanArenaChosenSize == 1376256`, so it remains structurally unable to
  pass Boundary — it is a lab arm, not a candidate.
- **Task B (the pack's own RAM footprint) untouched**, per the brief.
- **No re-run of the isolation arm.** A and C are quoted from
  `…/2026-08-15_battlepack-{gate,isolation}/`, re-ranked here from their own row
  CSVs so all four arms share one definition of rank-80.

---

## 7. Shipped bytes — proven inert, byte-for-byte

`build-c168-default-check`: this tree, **defaults** (`NDS_R2_BATTLEPACK 0`),
`BOTH_CPU=1`, against `build-c165-default-check` (the isolation cycle's tree) and
`build-c164-gate-bp0` (HEAD `79a9447fd6d`, where Boundary is GREEN at flag 0 and
flag 1).

```text
                           text     data        bss     total   fake_heap_start
build-c164-gate-bp0     985,468  148,288  1,307,016  2,440,772      0x022463c4
build-c165-default-check 985,468  148,288  1,307,016  2,440,772      0x022463c4
build-c168-default-check 985,468  148,288  1,307,016  2,440,772      0x022463c4
```

Sizes are the weak form. **Section CONTENT hashes**, `c168-default-check` against
`c165-default-check`:

```text
.text.hot       4,588 B   2F26D3E2455A889D == 2F26D3E2455A889D
.text.hot.draw  5,268 B   C490FF0B06E03F83 == C490FF0B06E03F83
.itcm          32,152 B   C1FE67EB8272578A == C1FE67EB8272578A
.dtcm           8,800 B   E4E358EDFB0435A1 == E4E358EDFB0435A1
.main         925,600 B   03BC293BA2C56717 != 57A49E443B84C4B9
```

`.main` differs in **7 bytes at one offset, 0x0c8fc0**: `3139502` → `98508e7`.
That is the embedded git short hash, not code. **Zero instructions changed at the
shipping default**, so Boundary's state on `79a9447fd6d` is undisturbed and was
not re-run — the same method and the same conclusion as the previous cycle, at a
strictly stronger standard (that cycle compared sizes only, which would not have
caught a same-size codegen change).

> Worth keeping: a section-size compare is NOT a proof of inertness, and the
> git-hash string means a section-hash compare will always show one 7-byte run.
> Diff the bytes and read the run.

---

## 8. Root ROMs

Unchanged across the cycle — no published target was built.

```text
smash64ds.nds                          54c07fac80c50418949908701f7c2bdbf27512c5f96ac09086fabbb0df6ac68a
smash64ds-battle-playable-hwtri.nds    2015fbd1f68b81c03626d8c6d473c8bcbcf527a3a26dfe86ff19bd74ecbb1360
```

## 9. Reproduction

```powershell
# the falsifier
make TARGET=smash64ds-battle-playable-tickhud-hwtri BUILD=build-c166-nodispatch-bp1 `
     NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1 `
     NDS_R2_BATTLEPACK_DISPATCH=0

# the profile arm -- NOTE: no -RunnerSlot, or the slot silently wins over -MelonDS
pwsh -File scripts\run-task37-profile-census.ps1 `
     -MelonDS emulators\melonds-attributor\melonDS.exe `
     -Build build-c167-profile-bp1 -StartFrame 438 -Frames 640 `
     -MakeFlags NDS_R2_BOTH_CPU=1,NDS_R2_BATTLEPACK=1,NDS_R2_BATTLEPACK_KEEP_CACHE=1,NDS_TICK_HUD_DRAW=0 `
     -OutDir artifacts\performance\2026-08-15_battlepack-mechanism\v3-pack-arm

# every arm: 5-minute soak BEFORE the gate run (it grows the arena), then
python scripts/census-tick-hud-p95-set.py --rows <rows.csv> --apparatus 24947
```
