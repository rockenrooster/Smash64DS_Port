# The next gate-lane cut, specified — `gcPlayDObjAnimJoint`'s AObj walk

Follows `LANES_BOTHCPU.md`, which named `SITR` (ceiling **83,712**, excursion
156,723, 46.9% of `SRC`) as the largest leaf lane on the corrected arm. This
takes it to the point `HANDOFF.md`'s standing rule requires before any edit —
*"`--pc-detail` BEFORE designing — no build, and it routinely names a different
lever than the source reads like"* — and it did exactly that here.

**No build and no run were spent.** `build-c123-profile` is already
`NDS_R2_BOTH_CPU 1` **and** `NDS_TICK_HUD_DRAW 0`, i.e. a correct gate-arm
profile ROM, and its whole-match profile (`regions=1601`) was already on disk at
`../2026-08-12_c123-rebank/profile/`. Every figure below comes off it.

## Where `SITR` actually goes

`SITR` is `ftMainProcUpdateInterrupt` minus the nested CPU AI
(`reloc_backend_diagnostic_recorders.c:5733`). Subtree attribution
(`analyze-subtree-attribution.py`, static call graph, shared symbols charged to
nobody):

| root | direct child | exclusive cycles | % non-idle |
|---|---|---:|---:|
| `ftMainProcUpdateInterrupt` | **`ftMainPlayAnim`** | 261,125,561 | **8.44%** |
| | `ftComputerProcessAll` (= `SCPU`, subtracted out of `SITR`) | 186,007,800 | 6.01% |
| | everything else | < 6,000,000 | < 0.2% |
| `ftMainPlayAnim` | **`ftParamUpdateAnimKeys`** | 560,487,699 | **18.11%** |
| `ftParamUpdateAnimKeys` | **`gcPlayDObjAnimJoint`** | 168,588,034 | **5.45%** |
| | `ndsR2FtAnimParseDObjFigatree` | 70,551,124 | 2.28% |
| | *SHARED, 18 symbols* | 282,630,486 | 9.13% |

The shared pool is the soft-float helpers plus `ndsR2AnimValueQ`, and
`analyze-leaf-helper-attribution.py` says it has **no single owner**: the largest
caller is `ndsBaseGcPlayMObjMatAnim` at 0.53% of work, and the whole soft-float
surface spreads over 295 functions (matrices 1.65%, other 1.61%, gameplay 1.44%,
collision 1.28%, animation evaluation 1.18%). That is consistent with the
animation *arithmetic* being spent, which `HANDOFF.md` already records.

## What `--pc-detail` says, and it is not arithmetic

`gcPlayDObjAnimJoint`, 604 bytes, `.text.hot`, 67,693,940 cycles = **1.77% of
the window** over 185 distinct PCs:

| pc | cycles | % fn | insns | cyc/insn | instruction |
|---|---:|---:|---:|---:|---|
| `0x02001484` | **15,200,176** | **22.5** | 572,768 | **26.54** | `ldrb r5, [r4, #5]` |
| `0x02001486` | 3,745,360 | 5.5 | 572,775 | 6.54 | `cmp r5, #0` |
| `0x0200143a` | 1,862,871 | 2.8 | 142,044 | 13.11 | `ldr r4, [r4, #0]` |

Read against `objanim.c:772`, `r4` is the `AObj *` and the loop is

```c
aobj = dobj->aobj;
while (aobj != NULL) {
    if (aobj->kind != nGCAnimKindNone) { ... }
    aobj = aobj->next;          /* ldr r4, [r4, #0] */
}
```

so `ldrb r5, [r4, #5]` is **`aobj->kind`** — a one-byte load — and `ldr r4,
[r4, #0]` is `aobj->next`. **26.54 cycles for a byte load is a cache miss on
essentially every execution**; a hit is 1–3. The following `cmp` costs another
6.54 waiting on `r5`. Together **18.9M cycles, 28% of the function**, on reading
one byte per node.

**The counts are exact, from the same table.** Function entry
(`push {r4,r5,r6,r7,lr}`) executes **111,944** times a match and the kind load
**572,768**, so the walk is **5.12 AObj nodes per call**, and 430,723 of them
(75%) survive the kind test to reach the track switch and the 430,671 calls to
`ndsR2AnimValueQ`.

## The lever, and what it is not

It is **not** the compare and **not** the interpolation arithmetic — the
campaign has already spent that lane twice (slices 34, 41), and this is the same
shape as slice 44, where a guard *looked* like a compare and four cold `ldr`s
were 39% of it. The lever is **the pointer chase**: five `AObj` nodes per call,
each first touch a miss, 111,944 calls a match.

Two candidate shapes, neither yet designed:

1. **Make the walk contiguous.** `AObj` is a linked list per `DObj`; five nodes
   in one cache line would pay the miss once instead of five times. The AOT dense
   bank already exists (`FTANIM_DENSE_BANK=OK`, 20-byte records, 2.78 MB, verified
   bit-exact) — the open question is whether the runtime can consume it here
   rather than at parse time only.
2. **Stop entering with nothing to do.** 25% of visited nodes fail the kind test,
   which is a cache line touched to learn a byte is zero. A per-`DObj` summary of
   which kinds are live would skip them without the load.

**Size the change before writing it.** `HANDOFF.md`: clear ~16,000 in one change
or use the `.data` route on one binary, because a load-frame-only ~8,000 cannot
be banked. At 1601 regions the kind load alone is ~9,500 cycles/frame and the
whole function ~42,300, so shape 1 is plausibly in range and shape 2 is not on
its own.

**And price it against the floor.** This arm's repeat spread is 9,664 and the
cross-build placement floor is ±8,544 (`GATE.md`), so a candidate that measures
under ~10,000 has not been shown to do anything.

---

## CORRECTION 2026-08-13 — shape 1's premise is arithmetically impossible

**Do not spend a build on "make the walk contiguous" as written above.**

`ptype /o AObj` on `build-c142-crouchprobe`'s ELF (authoritative, not inferred):

```
/*  0 | 4 */  AObj *next;
/*  4 | 1 */  u8 track;
/*  5 | 1 */  u8 kind;        <- the ldrb r5,[r4,#5] in the table above
/* XXX 2-byte hole */
/*  8 .. 35 */ 7 x 4 bytes (length_invert, length, value_base,
                            value_target, rate_base, rate_target, interpolate)
   total size (bytes): 36
```

The ARM9 D-cache line is **32 bytes** (`P1_EXECUTION_BOARD.md:1764`, and
`:4437` already records "each 36-byte node straddles two 32-byte lines").

Shape 1 claims "five nodes in one cache line would pay the miss once instead of
five times". At a 36-byte stride the five walked headers sit at byte offsets
0, 36, 72, 108, 144 — i.e. in **five different 32-byte lines even when the nodes
are perfectly contiguous**. Five nodes cannot occupy one line; 5 x 36 = 180 B is
5.6 lines. Contiguity would buy same-page locality and nothing else, and
ARM946E-S has **no hardware data prefetcher** to turn the regular stride into
prefetch hits. The AObj pool is *already* one contiguous
`syTaskmanMalloc` block (`battleship_sys_objman.c:119`), so the only thing
"make it contiguous" can still change is free-list ORDER within that block —
which does not reduce the line count.

**The shape that actually addresses the measured cost** is the plan's own
§3.7 hot/cold split: the walk reads only `next` (offset 0) and `kind`
(offset 5), 6 bytes of a 36-byte record. A dense per-`DObj` side array of kinds
puts all five in ONE line, so the kind scan costs one miss instead of five, and
the 36-byte payload is touched only for the 75% of nodes that survive the test
(that part is real work and must stay).

**Size it honestly before writing it.** From the same table at 1601 regions:
kind load 15,200,176 cyc = **~9,494/frame**; the dependent `cmp` 3,745,360 =
**~2,340/frame**; `aobj->next` 1,862,871 = **~1,164/frame**. Best case if the
header scan became free: **~13,000/frame**. That is above this arm's ±8,544
placement floor but **below `HANDOFF.md`'s ~16,000 "can actually be banked"
bar**, so it is a pairing candidate, not a standalone gate-closer — and it ADDS
data while the ROM sits 1.4–2.2 KB from the boot cliff (board G2), which must be
priced first. Shape 2 (~9,500/frame, the kind load alone) is smaller still and
the note above already rejects it as insufficient alone; the two overlap almost
entirely and must not be added together as if independent.
