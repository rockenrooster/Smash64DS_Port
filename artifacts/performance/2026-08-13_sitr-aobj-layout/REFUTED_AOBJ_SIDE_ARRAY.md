# REFUTED — the per-`DObj` dense AObj header array, and every layout shape behind it

`SITR_NEXT_CUT.md`'s CORRECTION killed shape 1 ("make the walk contiguous") on
the 36-byte stride and named the §3.7 hot/cold split as *"the shape that
actually addresses the measured cost"*: a dense per-`DObj` side array of kinds,
so *"the kind scan costs one miss instead of five, and the 36-byte payload is
touched only for the 75% of nodes that survive"*.

**That premise is false, and the same profile disproves it.** No build and no
run were spent here either — this is `build-c123-profile`'s whole-match
`BOTH_CPU 1` profile (`../2026-08-12_c123-rebank/profile/`, `regions=1601`),
read per PC across the two functions instead of per function.

Raw: `gcPlayDObjAnimJoint-per-pc.csv`, `ndsR2AnimValueQ-per-pc.csv` (every
executed PC, cycles, executions, cyc/insn, cyc/frame).

## The measured fact: one line fill per node, and it serves the whole record

`gcPlayDObjAnimJoint` @ `0x020013c0`, 604 B: **67,693,940 cycles = 42,282/frame**,
24,391,810 insns, CPI 2.78. 111,944 calls, 572,768 nodes visited (5.12/call,
357.8/frame), 430,723 survivors (75.2%).

| pc | instruction | field | execs | cycles | cyc/insn |
|---|---|---|---:|---:|---:|
| `0x02001484` | `ldrb r5,[r4,#5]` | `kind` | 572,768 | 15,200,176 | **26.54** |
| `0x02001486` | `cmp r5,#0` | (load-use interlock) | 572,775 | 3,745,360 | 6.54 |
| `0x02001488` | `beq` | | 572,767 | 1,214,554 | 2.12 |
| 10 sites | `ldr r4,[r4,#0]` | `next` | 572,767 | 3,953,353 | 6.90 |
| `0x02001496` | `ldr r0,[r4,#12]` | `length` | 422,872 | 3,170,964 | **7.50** |
| `0x020014b0` | `ldrb r3,[r4,#4]` | `track` | 430,723 | 442,816 | **1.03** |

And inside `ndsR2AnimValueQ` (`0x020660f0`, 61,868,677 cyc = **38,644/frame**,
CPI 1.65, 430,671 calls) — every one of its eight loads off the *same*
`AObj *`:

| pc | instruction | field | execs | cyc/insn |
|---|---|---|---:|---:|
| `0x020660f4` | `ldrb r2,[r0,#5]` | `kind` again | 430,671 | **1.00** |
| `0x020660f8` | `ldr lr,[r0,#12]` | `length` | 430,671 | 1.32 |
| `0x02066100` | `ldr ip,[r0,#8]` | `length_invert` | 430,671 | 1.04 |
| `0x02066104` | `ldr r3,[r0,#16]` | `value_base` | 430,671 | 1.61 |
| `0x02066110` | `ldr r4,[r0,#24]` | `rate_base` | 235,824 | 2.38 |
| `0x020661f0` | `ldr r3,[r0,#20]` | `value_target` | 225,700 | 1.62 |
| `0x02066258` | `ldr r0,[r0,#28]` | `rate_target` | 225,700 | 1.89 |
| `0x02066398` | `ldrle r3,[r0,#20]` | `value_target` | 194,847 | 3.71 |

**4,214,372 cycles for ~2.6M executions.** The `ldrb r2,[r0,#5]` at the top of
`ndsR2AnimValueQ` reads the byte the caller's 26.54-cycle `ldrb` read, on the
same node, at **1.00 cyc/insn**.

So the `AObj` record costs **exactly one 32-byte line fill per visited node**,
paid at whichever access touches it first — today the `ldrb` at offset 5 — and
that one fill serves `next`(0), `track`(4), `kind`(5), `length_invert`(8),
`length`(12), `value_base`(16), `value_target`(20), `rate_base`(24),
`rate_target`(28). There is no separable "header miss" to remove. The §3.7
framing — *"the walk reads only 6 bytes of a 36-byte record"* — is true of the
source and false of the cache: those 6 bytes and the 26 bytes the survivors
consume are the same line.

## What the side array would therefore actually buy

Removing the header load from the node does not delete the fill. For the 75.2%
that survive it **migrates** it to the first payload load (`ldr r0,[r4,#12]`,
7.50 cyc/insn today, would become ~26.5). Only the 24.8% that fail the test are
saved, and the side record costs a new fill of its own.

| | cycles/match |
|---|---:|
| removed: `ldrb` kind + `cmp` + `beq` | −20,160,090 |
| removed: `ldr` next ×10 sites | −3,953,353 |
| added: side-record line fill, 1 per call (111,944 × 26.5) | +2,966,516 |
| added: warm scan of 5.12 entries/call (4 cyc/node) | +2,291,072 |
| added: fill migrates onto `length` (422,872 × 26.5, was 3,170,964) | +8,035,144 |
| **net** | **−10,820,711** |

**−6,758 cycles/frame** at 1601 regions — and that is the *best* case, charging
nothing for maintenance.

Against the campaign's own bars: cross-build placement floor **±8,544**, this
arm's repeat spread **9,664**, `HANDOFF.md`'s bankable bar **~16,000**. The best
case is **0.79x the floor** and **0.42x the bar**. It cannot be measured, let
alone banked.

Maintenance, uncharged above and strictly a subtraction: `aobj->kind` is
**written 76,577 times a match (47.8/frame)** — 72,969 of them at four sites in
`ndsR2FtAnimParseDObjFigatree` (`0x0208a0e6/194/278/33e`), the rest in
`gcParseDObjAnimJoint` and `gcAddAObjForDObj` — and each would have to write the
side array too, on top of `gcAddDObjAnimJoint` clearing every kind in a chain and
an overflow path for chains longer than one record.

## The general bound — this closes the lane, not just the shape

Line-fill cost in the walk today (cycles minus one issue cycle per insn):
14,627,408 (kind) + 3,172,585 (interlock) + 3,380,586 (next) + 2,748,092
(length) = **23,928,671 = 14,946 cyc/frame**.

Floor for *any* representation, assuming perfect packing, zero maintenance and
zero added instructions — 430,723 surviving nodes must still be read:

| record | bytes/node | line fills | cyc/frame | ceiling |
|---|---:|---:|---:|---:|
| 26 B actually consumed | 26 | 349,962 | 5,791 | **9,155** |
| FTANIM dense bank record | 20 | 269,202 | 4,455 | **10,491** |

**No layout change to this walk can reach 16,000/frame** — not a side array, not
a hot/cold split, not a contiguous repack, and **not consuming the FTANIM dense
bank at play time**. That answers `SITR_NEXT_CUT.md`'s standing question on cost
grounds, independent of whether the runtime *could* consume the bank: at 20-byte
records its unattainable ceiling is 10,491/frame, and the realizable side-array
shape is 6,758. `HANDOFF.md` already carries *"the AOT animation bake (slice 32):
SIZE dead"*; this is the arithmetic behind why, measured at the consumer.

## What is still in `SITR`, sized

`ndsR2AnimValueQ` is 38,644 cyc/frame at **CPI 1.65 — issue-bound, not
memory-bound**, so its lever is instructions, not layout. Its two largest PCs
are its own frame:

- `0x020660f0` `push {r4,r5,r6,r7,r8,r9,sl,fp,lr}` — 3,890,792 cyc, 9.03 cyc/insn
- `0x02066330` `pop {r4,r5,r6,r7,r8,r9,sl,fp,pc}` — 5,753,204 cyc, 13.36 cyc/insn

**9,643,996 cycles = 6,022/frame, 15.6% of the function**, saving nine registers
on every one of 430,671 calls. Only the Cubic arm (225,700 calls) needs that
pressure; the other 204,971 — Step's compare-and-select and Linear's single
SMLAL — pay a Cubic-sized frame for it. Splitting the arms into their own
`noinline` bodies is layout-free and bounded, but sizes at ~2,200-2,700/frame:
a pairing candidate, not a gate-closer, and it must not be started as one.

Per `HANDOFF.md`'s *"a flat function's only lever is not entering it"*, what is
left in this lane is **call count**: 358 node visits and 269 value evaluations
per frame. Nothing about how they are laid out.
