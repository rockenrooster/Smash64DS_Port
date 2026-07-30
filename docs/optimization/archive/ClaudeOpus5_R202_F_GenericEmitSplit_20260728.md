# R2-02 F — generic emit, split four ways, and three dead ends closed

**Date:** 2026-07-28
**Phase:** R2-02 follow-up (the board's "generic emit is 67,126 ticks/frame" row)
**Verdict:** no cut shipped. The target moved, and three candidate cuts are
refuted with numbers rather than opinion.

## 1. Why this was opened

The board named `generic emit` at 67,126 ticks/frame — 21 runs carrying 103
triangles — and E7 had already split it once, flagging segment 4 as "the
*largest* remaining stage lever, worth ~19,000". This series asks the question
E7 left: **what are 21 runs actually buying, and can the count come down?**

## 2. F0 — runs do not merge

The first hypothesis was that consecutive runs carry identical state, so
`BeginRun`'s fixed sequence is re-issued for nothing and runs could batch.

Per frame, over 20 adjacent-run comparisons:

| field | repeats predecessor |
|---|---|
| whole state | **1.0** |
| texture (name + params + textured) | 2.0 |
| poly format | 5.0 |
| alpha test + ref | 8.0 |
| matrix binding | 5.0 |

**Refuted.** One merge per frame is worth ~1,200 ticks. The generic path
genuinely rebinds a different texture on 18 of 21 runs; the run count reflects
real material changes, not sloppy ordering.

## 3. F1 — the target is not segment 4

Per-segment split of the same bucket (the counters already existed and had
never been read):

| seg | owner | ticks | runs | tris | ticks/run | ticks/tri |
|---|---|---|---|---|---|---|
| 1 | actor | 6,110 | 1.0 | 4 | 6,110 | 1,528 |
| 2 | actor | 10,138 | 4.0 | 8 | 2,534 | 1,267 |
| 3 | actor | 9,926 | 4.0 | 6 | 2,482 | 1,654 |
| **4** | **layer1** | **22,843** | 6.0 | **76** | 3,807 | **301** |
| 6 | actor | 17,824 | 6.0 | 9 | 2,971 | 1,980 |
| | | **66,841** | 21.0 | 103 | | |

Segments 1/2/3/6 are the actor owners — Whispy's eyes and mouth, both flower
beds. Together they are **43,998 ticks/frame for 21 triangles**: nearly double
segment 4, on 28% of its geometry, at 2,095 ticks per triangle.

**The board's "segment 4 is the largest remaining stage lever" is superseded.**
It was true of the only split that had been taken; it is not true of the frame.
R2-02's plan already named the actors — *"DYNAMIC VISUAL -> Whispy + flowers:
small specialized update+draw path"* — and that path was never built. R2-02 E3
and E4 instead tried to route the actors through the *replay*, which is a
different mechanism, and were refuted twice.

## 4. F2 — Task 51 is structurally unreachable, not merely unexercised

`NDS_TASK51_STAGE_NATIVE` exists to do exactly what the actors need: non-rigid
bindings submit a generator-baked constant world via `MTX_MULT4x3` instead of a
per-frame CPU compose. It was killed on 2026-07-23 for one stated reason — *"the
16 non-rigid bindings Task 51 targets do not draw in the battle_playable
scene"* — and the kill note names its own revisit condition: *"find a
scene/match state where bindings 20–29 / 33–38 submit GX."*

Those are precisely the actor segments' bindings, and **they draw now**: 15 runs
and 21 triangles per frame. So the condition is satisfied. The flag was turned
on and measured:

| counter | value |
|---|---|
| `EmitNoZTriangle` triangles taking the Task 51 path | **0.0 / frame** |
| triangles falling through to the CPU path | 27.0 / frame |
| ticks spent in `Task51EnsureWorld` **failing** | 1,634 / frame |
| generic emit, flag off → on | 71,649 → **76,403** (+4,754) |

`ndsRendererNativeStageTask51EnsureWorld` rejects on its second gate,
`task36_segment_active == FALSE`. The baked table covers all 42 bindings, so
that is not the problem; the problem is that **only a rigid binding ever opens
the Task 36 segment bracket** (`nds_renderer.c`, `BeginRun`), and an actor
segment has no rigid binding. Task 51 was built to piggyback on a bracket that
nothing in its own target opens.

This is a *different* reason than the one recorded in 2026-07-23. That note
would have sent the next reader down the same three builds. Pinned here: the
revisit condition is now met and the answer is still no, because reviving Task
51 requires giving the actor segments their own bracket first — at which point
the specialized path in §3 is the cheaper thing to build.

## 5. F3 — where `BeginRun` actually goes

Bracketing `BeginRun`'s four sections separately, per frame:

| section | ticks | scope |
|---|---|---|
| matrix — rigid Task 36 | 0 | 0.0 generic runs |
| matrix — raw composed | 1,460 | 3.0 runs @ 486 |
| matrix — projected range | 1,185 | 3.0 runs @ 395 |
| matrix — generic load (the actors) | 9,255 | 15.0 runs @ 617 |
| `EndBatch` | 2,804 | all 54 runs |
| **texture bind** | **21,978** | all 54 runs @ 407 |
| alpha test + fog + poly format | 5,472 | all 54 runs @ 101 |

The texture bind is the largest single item in `BeginRun` — comparable to all of
segment 4 — and it is paid by the **replay** runs too, the path four previous
tasks have already optimised.

It is not a missing guard. `ndsRendererHardwareBindTextureName` early-outs on
`sNdsRendererHardwareBoundTextureName`, and
`ndsRendererHardwareApplyTextureParams` early-outs on the GX state shadow. With
18 of 21 generic runs genuinely changing texture (§2), most of these binds are
real work that any implementation would still owe the hardware.

## 6. What this leaves

The stage's remaining cost is **per-run fixed overhead with no single dominant
redundancy**. Merging is dead, Task 51 is dead, the texture binds are guarded and
mostly genuine. One real lever survives, and it is the one R2-02 planned and
never built: stop routing 21 actor triangles through discover → prepare →
`BeginRun` → generic emit, and give them a specialized update+draw path.

Sizing it honestly: the texture binds those 15 runs owe (~6,100 at 407/run) and
the vertex writes are not recoverable, so the ceiling is roughly **30,000**, not
44,000. That is generator plus runtime work, and it should be gated on a
screenshot against the control arm — E3 and E4 both destroyed these exact
segments while reporting a saving, and E3's KEEP was written from the candidate
arm alone.

## 7. Instrumentation

The surviving counters live under the existing default-off
`NDS_TASK103_STAGE_RUN_PHASE`, alongside the Task 103 family:
`gNdsTask103BeginMtx{Ticks,Count}[4]`, `gNdsTask103NoZPath[3]`,
`gNdsTask103NoZ{World,Proj}Ticks`, `gNdsTask103Begin{EndBatch,Tex,Tail}Ticks`.
The per-segment counters (`gNdsTask103GenericSeg{Ticks,Runs,Tris}[]`) already
existed and had never been read — §3 is the first time they were.

**§2's counters have been removed, and the removal is the point.** They read six
`prepared_run` fields inside `ndsRendererCommitNativeStageSegment`, whose
consumed-field closure the stage falsifier polices — and the falsifier failed
the next Boundary run over it. A default-off `#if` does not hide the reads,
because the check is static analysis of the source. Classifying them to make it
pass would have asserted an immutability §2 itself disproved (18 of 21 runs
rebind a texture). The question was answered once and does not need re-asking,
so the probe is gone and the answer stays here.

That failure is also a process finding: this document's commit was made without
running the widest relevant verifier, on the reasoning that default-off
instrumentation cannot change a shipping build. It cannot change the binary, but
it can change what a *source-level* checker sees.

## 8. For the standing rules

`gNdsTask103NoZWorldTicks` read back as `0xEA800009` — an ARM branch opcode —
on the flag-off arm, because the symbol was never emitted and gdb resolved a
neighbouring address. A counter inside a compiled-out block does not read as
zero; it reads as whatever is at that address. **A null result must prove the
instrument was compiled in**, not merely that it printed something falsy. This
extends the rule E5 added rather than replacing it.
