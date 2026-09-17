# The DTCM falsifier passes, and the budget kills the lane anyway

The stall-class sizing (`…_p2-2p8-stall-budget/STALL_CLASS_SIZING.md`) named one
candidate worth a build — put the flattened parts-walk cache in DTCM and widen
it — and one cheap falsifier to run first: **move the table into DTCM at its
shipped four slots, change nothing else, and read STG alone.** If STG does not
improve, this table was never the evictor that lost N05.03, widening cannot pay,
and the lane dies for one build instead of four.

## The falsifier passes

`sNdsFtPartsFlat` moved to `.dtcm.bss` at four slots, nothing else changed.
Every divergence witness identical to the control —
`gNdsFighterDLAllDrawP0/P1HardwareTriangleCount` 349,031 / 333,618,
`gNdsFtrPlanHit` 6,217, `gNdsGCDrawsActiveMax` 203 — so this is the same
workload.

| P50 | control (`.main.bss`) | DTCM, 4 slots | delta |
|---|---:|---:|---:|
| **STG** | 344,768 | 336,832 | **-7,936** |
| SRC | 555,584 | 550,592 | -4,992 |
| FTR | 355,712 | 355,904 | +192 |
| **WORK-H** | 1,588,928 | 1,578,752 | **-10,176** |

STG improved, so the table **was** contributing to the stage's eviction, and
DTCM removes that contribution. The mechanism is confirmed: DTCM is addressable
zero-wait memory, not a cache, so unlike the 16-slot main-RAM table that lost
N05.03 (+51,520 STG) it can neither evict the stage nor be evicted.

**But -10,176 is below the 14,080-tick cross-build significance floor.** Two
buckets moved the right way by 12,928 combined, which is more than noise-shaped,
and it is still not a bankable win by this repo's own standard.

## The budget kills the widening, and the sizing had it wrong

The sizing document reported **5,704 free DTCM bytes**, taken from
`dtcm LENGTH = 0x3e80` in `C:/devkitPro/calico/lib/ds9.ld`. That is the region,
not the budget. This repo has its own ceiling:

```
linker/nds_hot_text.ld:171
  ASSERT( __dtcm_bss_end <= 0x02ff3000,
    "Error: DTCM data reaches into the boot stack's measured low-water mark" )
```

DTCM data grows up from `0x02ff0000` and the boot stack grows down from
`__sp_usr = 0x02ff3e80`; the region LENGTH covers **both**, so the linker alone
cannot catch a collision. The comment records the measurement behind the number:
DTCM dumped at match frame 900 on 2026-07-28 put the stack's deepest reach at
`0x02ff3340`, and `0x02ff3000` keeps 832 bytes of headroom under it.

So the real ceiling is **12,288 bytes of DTCM data**:

| | bytes |
|---|---:|
| ceiling (`0x02ff3000 - 0x02ff0000`) | 12,288 |
| used before the move (`.dtcm` 0x2220 + `.dtcm.bss` 0x618) | 10,296 |
| **actually free** | **1,992** |
| free after moving the 4-slot table | 408 |
| further bytes 16 slots x 48 would need | **1,680** |

The widening never fit. The sizing's "best candidate" of 8 slots at 3,168 bytes
did not fit either — it was 1,176 bytes over budget before it was proposed.

And 8 slots is the wrong target regardless: measured in main RAM it removed no
conflicts at all (15,177 against the 4-slot 15,490), because the working set is
between 9 and 16 roots. The only slot count that helps is the one that does not
fit.

## Disposition: reverted, not kept

The placement is reverted. It buys an unproven 2% of the gap and costs **1,584
of the 1,992 usable bytes**, which would leave 408 and block every future DTCM
candidate. Keeping an under-floor result that consumes 80% of a scarce budget is
how a campaign accumulates unbankable cost.

Verified reverted: the rebuilt ELF differs from the pre-DTCM control by **6
bytes**, all inside the embedded git stamp, and `.dtcm` is back to 0x2220.

## What this closes

The stall class is now measured end to end rather than argued:

| | result |
|---|---|
| is the stall data or fetch? | **data, 3.6:1** — 560,739 against 155,651 |
| can DTCM hold anything that pays? | **no** — 1,992 usable bytes, and the break-even is ~13 refetches per resident line per frame |
| does placement cure N05.03's failure? | **yes, and it is worth -10,176** — below the significance floor |
| can the lane be finished? | **no** — the only slot count that reduces conflicts needs 1,680 bytes that do not exist |
| does `FTParts` packing help? | **no** — every hot field is already in line 0 |
| does `DObj` packing help? | **8.1% ceiling, blocked** — declared in pristine `decomp/`, no port shadow |

The one number that survives for future work: **the frame moves ~816 KB of data
through a 4 KB data cache every frame, and 39.5% of the data stall is renderer
streaming.** That bucket is 254,344 tk/fr, it is the largest single owner of
stall, and nothing in this campaign has attacked it. It is a working-set
**volume** problem — which is the one thing neither placement nor packing can
change.
