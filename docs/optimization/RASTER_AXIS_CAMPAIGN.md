# The Raster Axis — campaign plan

> **AMENDED 2026-07-27 by Task 100 — the thesis in §2 is REFUTED.** A quarter of
> the frame's pixels stopped being drawn and `STG` moved **−320** against a
> ≥40,000 kill criterion. §3.1 fires. **Do not build another coverage, fill,
> anti-aliasing or overdraw probe**, and skip Tasks 101–102: they were fork-A
> tasks conditional on a thesis that did not survive.
>
> The refutation has an architectural reason and §2 should have caught it: the DS
> rasterizer consumes the *already-swapped* polygon RAM during scanout, so it is
> structurally incapable of stalling the CPU. §2 conflated FIFO backpressure
> (geometry-side, real, scales with vertices) with rasterization time
> (scanline-side, decoupled). See `ClaudeOpus5_Task100_PixelsAreFree_20260727.md`
> §4.
>
> **The campaign continues on fork B only** — §5 Task 103, "what is the fixed
> cost, if not pixels?" — now carrying a concrete sized hypothesis: ~6,135 ticks
> per stage run × 54 runs. Everything below is preserved as written so the
> reasoning that failed stays legible; read it with this box in force.

**Opened:** 2026-07-27, after Tasks 98 and 99 closed visual approximation on the
payload axis.
**Succeeds:** `COMPILER_FIRST_ARCHITECTURE.md`, whose roadmap is complete and
whose central premise was inverted by Task 81.
**Standing rules apply in full:** `TASK_STANDING_RULES.md` governs how every task
below is run, measured and judged. This document adds nothing to that procedure;
it only says what to point it at.

---

## 1. Why this campaign exists

Two searches are exhausted, both by measurement rather than by argument:

- **Exactness-preserving** — nineteen tasks (78–96). Nothing left above the
  ≥20,000 ticks/frame bar Task 95 set.
- **Visual approximation, payload form** — three tasks (98, 99). Cheaper vertex
  encoding, lower texture resolution and fewer triangles all measured at or near
  zero.

Both searches shared an assumption nobody wrote down: **that the frame's cost is
something the CPU does.** Every instrument this campaign owns measures CPU ticks
attributed to CPU symbols, and every lever proposed in 22 tasks was a change to
what the CPU executes or what it touches.

The record now contains a contradiction that only makes sense if that assumption
is wrong.

### 1.1 The contradiction

Task 54 E0 concluded the stage is **GX-throughput-bound**. Its evidence was
strong: removing 187,648 ticks of stage *CPU* work moved `STG+OTHR` by −1.8%
(732,992 → 720,064) and `ALL` by −128. Work left the CPU and the frame did not
get shorter. The conclusion drawn was that the geometry engine draining its fixed
2,996 words is the floor.

Two later measurements test that conclusion against the two quantities GX
throughput would scale with, and both fail it:

| quantity | change | predicted | **measured** | source |
|---|---|---|---|---|
| command words | −355/frame (−9.1%) | ~−33,700 | **+64** | Task 55 E2 |
| triangles | −101/frame (−50%) | ~−185,000 | **−19,584** | Task 99 |

Neither the command stream nor the vertex count is the currency. Task 99 put a
number on what is left: the stage bucket is **~89% fixed**, ~331,300 of 370,496
ticks that do not care whether any geometry exists at all.

**"GX-throughput-bound" named a bucket, not a mechanism.** The largest single
identified cost in the frame has never been attributed, and it is the same
~331,300 ticks in every task that has looked at it.

### 1.2 The quantity nothing has tested

Halving a mesh's triangles does not shrink its silhouette by one pixel. Same
coverage, fewer corners. That single fact explains both rows of the table above
at once, and **pixels are the one axis 99 tasks have never touched.**

---

## 2. The thesis, stated so it can be killed

> The stage's fixed cost is **rasterization time**, and it reaches the CPU as
> backpressure: a store to a full GX FIFO stalls inline, and that stall is
> charged to whichever bucket the store was issued from.

This is not a new mechanism invented for the occasion — it is written in the
ROM's own instrumentation. `src/port/taskman_seam.c:4925`, Task 66:

> *"GX backpressure is not pooled here but distributed through the named buckets
> as memory stall on the write that could not retire."*

The stage replay is a CPU store loop — `GFX_FIFO = words[i]` at
`src/nds/nds_renderer.c:20530`, ~2,996 words per frame, one MMIO store each, into
one register feeding one geometry engine. If the engine is the limiter, the loop
does not cost what its stores cost; it costs what the engine takes to drain them.

**That single change of variable makes every result in §1.1 consistent for the
first time:**

- Removing 355 *words* buys nothing, because the CPU still waits out the same
  total rasterization time — it just waits on a different store.
- Removing 101 *triangles* buys a little (−19,584), because it trims the geometry
  stage without touching coverage.
- Removing 187,648 ticks of *CPU work* buys nothing, because the CPU was never
  the limiter.
- 79.2% "stall" in the texture class and 52–89% across sixteen of seventeen
  classes (Task 81) is partly not memory stall at all — it is the pipe.

**Falsifiable prediction:** a change that reduces screen coverage while holding
word count and triangle count fixed will move `STG` by much more than Task 99's
−19,584. If it does not, the thesis is dead and §6 says what replaces it.

### 2.1 What `WAIT` is, so nobody targets it

`WAIT` is 363,264 ticks P50 and its name invites the wrong conclusion. Per the
same Task 66 comment, **`WAIT` is the span parked in `swiWaitForVBlank`** — it is
the frame's *slack*, not a cost. `WORK = ALL − WAIT` exists precisely so that a
saving smaller than one VBlank shows up somewhere instead of being absorbed by
the wait.

**Do not open a task to reduce `WAIT`.** Reducing it makes the frame worse. The
stall worth hunting is inside `STG`, `FTR` and `SRC`, and per §2 some fraction of
it is the pipe rather than memory.

---

## 3. Kill criterion

This campaign is over when either holds:

1. **Task 100 refutes the thesis** — coverage reduction moves `STG` by less than
   40,000 on a probe that eliminates ≥60% of stage pixels. That is roughly twice
   Task 99's triangle result; below it, pixels are no better a currency than
   words or triangles were, and the raster axis joins the closed list.
2. **The instrument is proven blind and no device path replaces it** — see §4.1.

A single task may still STOP on its own gate without ending the campaign. The
campaign ends on the thesis, not on one arm.

---

## 4. Method

Unchanged from `TASK_STANDING_RULES.md`, with two additions this campaign needs.

- One synchronized 128-sample `-RingDump` per arm, identical ROM/window/runner,
  frames 439–566. **Vary the build, never the run** — run-to-run variance is
  zero, build-to-build placement variance is ±8,000.
- Report **P50 and P95** per bucket. Never headline a mean.
- Device arms report the **2/3/4/5+ VBlank-interval histogram and the max
  interval**. Never min FPS.

### 4.1 New: the instrument-fidelity control, and it runs first

Every number in 99 tasks comes from the repo's melonDS fork. That fork is
**cache-accurate for the ARM9** — which is exactly why the layout tasks (82–95)
were trustworthy, and why their null results can be believed.

It is **not** a cycle-accurate model of the DS rasterizer.

If melonDS charges the 3D engine a cost model rather than simulating fill, then a
coverage change reads as zero on the emulator whether or not it is real on
hardware — and "the stage bucket is 89% fixed" would be a statement about the
emulator, not about the DS. That possibility has never been tested, and it is
load-bearing for this entire campaign.

**Therefore Task 100 is read on both surfaces**, and disagreement is the finding:

| emulator `STG` | device histogram | reading |
|---|---|---|
| moves | moves | thesis supported, instrument sound — proceed |
| flat | **moves** | **instrument is blind to fill.** Campaign continues device-only; every raster task needs a device arm, and Task 54's original conclusion is retroactively suspect |
| moves | flat | emulator is modelling a cost the hardware does not have — stop and fix the instrument before anything else |
| flat | flat | thesis refuted per §3.1 |

This is the control that Task 45's post-mortem said should have run first and did
not. It runs first here.

### 4.2 Probes are allowed to look broken

Task 99 established the pattern and it worked: a probe that is visually wrong by
construction is legitimate when its job is to produce a slope, not a candidate.
Probes are flag-guarded, default 0, and **reverted before handoff** — they never
reach a published ROM. A probe is not a fidelity trade and does not need the
owner's visual approval; a *candidate* does.

---

## 5. The task list

### Task 100 — The coverage probe (**the decisive one; run this first**)

Reduce stage screen coverage while holding word count and triangle count fixed.

| arm | build | what it isolates |
|---|---|---|
| **A** | baseline | control |
| **B** | stage geometry scaled to ~¼ screen area | pure coverage — identical words, identical triangles, ¼ the pixels |
| **C** | `glEnable(GL_ANTIALIAS)` removed at `src/nds/nds_platform.c:315` | DS hardware AA is an edge-pixel rasterizer cost. One bit, no geometry change, no re-addressing collateral |

Arm B is the experiment; arm C rides along free and is independently interesting
(§5.1). Both are read on emulator **and** device per §4.1.

**Gate:** §3.1 — B must move `STG` by ≥40,000 or the campaign closes.

**Note on arm B's construction:** scale in the projection or model matrix, not by
regenerating geometry. Regenerating changes the word stream, which reintroduces
the variable being held fixed, and per Task 99 arm C any change to the stage run
set disarms the Task 36 replay and costs ~100,000 ticks — which would swamp the
signal completely. **Whatever arm B does, the replay capture must stay valid.**

### Task 101 — Fork A: translucency and polygon-ID census

Runs only if Task 100 supports the thesis.

`glEnable(GL_BLEND)` is set globally at `src/nds/nds_platform.c:317` and
`POLY_ALPHA` appears at five sites. Translucent polygons take the DS's slower
rasterizer path and interact with polygon ID for sorting. Census which stage and
fighter polygons are actually translucent, and how many distinct polygon IDs are
in flight.

**Anchor before proposing anything:** this task produces counts, not a change.
A lever comes out of it only if some quantity it counts can be tied to ticks by
Task 100's slope.

### Task 102 — Fork A: off-screen binding cull

Task 54 E0 recommended this and it was never built. It is the one candidate that
satisfies **both** the refined operation rule from Task 99 §4 (it removes runs,
binds and draws — not items from inside them) **and** the coverage thesis (it
removes pixels).

**Hard constraint, learned expensively:** Task 99 arm C culled 27 of 54 stage
runs and `STG` went *up* 109,888 because the capture-once replay was disarmed.
Any cull must either preserve the replay capture or carry its own re-capture, and
must be measured against that risk explicitly. Do not repeat arm C.

### Task 103 — Fork B: what is the fixed cost, if not pixels? **← THE LIVE TASK**

Task 100 refuted the thesis, so this is what the campaign is now. Coverage,
words, triangles and "more CPU instructions" are all ruled out, which leaves
per-operation scaffolding — the currency Task 99 §4 named and no task has
isolated:

```
331,296 fixed ticks / 54 stage runs  =  ~6,135 ticks per run
of which ~1,621 x 25 binds = 40,525 is texture bind (Task 98 §3)
```

Suspects, in order: per-run begin/end batch scaffolding
(`ndsRendererNativeStageBeginRun` / `ndsRendererHardwareEndBatch`), matrix-mode
and generation bookkeeping, and GX state writes issued once per run.

**The instrument is the whole difficulty.** Task 99 arm C varied run count by
culling and got **+109,888**, because changing the run set disarms the Task 36
capture-once replay and hands back the ~100,000 that Task 53 won. Any design here
must vary the per-run cost *without* invalidating the capture — for example by
timing the scaffolding directly with a phase counter rather than by removing
runs, which is the approach Task 91 E1 used successfully on the fighter draw.

**Gate:** ≥20,000 ticks/frame attributable and removable, per the Task 95 bar.

### Task 104 — The MPU and cache-attribute audit (independent of the fork)

**There is no CP15 or MPU configuration anywhere in `src/`.** Cacheability,
bufferability, and therefore write-back versus write-through policy per region
are whatever libnds's default `setupMPU` provides, and in 99 tasks nobody has
read them back.

With 52–89% stall frame-wide this is the last *global* stall lever that is a
configuration rather than a layout change — so it carries none of the
re-addressing collateral that closed Tasks 87–89, 94 and 95.

It may well be empty; libnds defaults are usually sane. The cost of finding out
is one GDB read of the CP15 region registers, which is why it is on the list at
all. **Read first, propose second** — this is a census, not a change.

### Task 105 — Candidate assembly and the fidelity budget

Runs only if 101–104 produce something. Whatever survives becomes a *candidate*
rather than a probe: default-on in a lab build, synchronized screenshot diffs
against baseline, `artifacts/visibility` entry, and **the owner's visual
approval** before it goes near a published ROM. AA removal (Task 100 arm C) is
the likely first item here, and jaggier edges is a real visual change that is the
owner's call, not mine.

---

## 6. Honest sizing

`WORK-H` P95 is **1,761,664** against a **1,120,000** gate — **641,664 over**.

`STG` is 370,496 P50 in total. **Even deleting the stage entirely does not close
the gap.** No task in this list can finish the job alone, and any write-up
claiming otherwise is wrong.

What this campaign can realistically do is decide whether the raster axis has any
levers at all. If it does, the same mechanism applies to the fighter path, which
is the larger bucket (`fighter: native production` alone is 255,061 ticks/frame,
17.6% of work, per Task 81) — and *that* is where a gap-closing number would have
to come from. Task 100 is worth running because it is the cheapest possible test
of a mechanism that would reframe the fighter path too, not because the stage is
where the win is.

If the thesis dies at Task 100 on a sound instrument, then three independent
axes — CPU work, payload size, and coverage — have all been measured to have no
lever above the bar. At that point the honest conclusion is that **30 FPS on this
content is a scope decision rather than an optimization problem**, and the
sacrifice order in `PROJECT_GOAL.md` moves past visual fidelity to the 60 Hz
simulation. That is the owner's call and this document does not pre-empt it.

---

## 7. Not in scope

- **Reducing `WAIT`.** §2.1. It is slack.
- **Another payload reduction.** Task 99 §4's rule stands: a change pays only if
  it removes a run, bind, draw or dispatch, never if it removes items from inside
  one. Coverage is on the list because it is not a payload.
- **Restarting anything in `COMPILER_FIRST_ARCHITECTURE.md`.** Its roadmap is
  complete, item 78 is closed three separate ways, and its close-out says so.
- **Re-litigating soft-float.** Task 92 measured the renderer-eligible fraction at
  ~20,000 — at the bar, behind a fidelity gate, for 2.5× the noise floor.
- **Layout and placement.** Closed by Tasks 87, 88, 89, 94 and 95. Task 104 is a
  configuration audit, not a placement change, and must not become one.

---

## 8. Rules this campaign inherits, and must not relearn

Each of these cost a task to earn. They are in `TASK_STANDING_RULES.md`; they are
repeated here as pointers because this campaign is unusually exposed to all five.

1. **Find the tick anchor before proposing a lever.** A reduction ratio is not a
   saving. (Task 98 §7)
2. **Do not size a per-item lever by dividing a bucket total.** It gave 1,832
   ticks/triangle against a measured 194 — 9.4× wrong, in the familiar direction
   of attributing fixed overhead to the per-item quantity. (Task 99 §2)
3. **Work that moves between buckets while the total grows is pipeline
   substitution, not a saving.** Discard it rather than report it. (Tasks 84,
   96 E0, 99 arm C)
4. **Restrict every metric to the domain that changed.** A full-mesh IoU of
   0.9976 once hid a 70%-wrong island. (Task 90-era)
5. **Vary the build, never the run.** Run-to-run variance is zero; ±8,000 is
   build placement.
