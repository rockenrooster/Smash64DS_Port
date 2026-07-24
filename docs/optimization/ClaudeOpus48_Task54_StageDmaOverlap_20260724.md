# TASK 54 — Stage replay DMA + CPU overlap (reclaim the Task 53 OTHR backpressure)

**Standing rules apply in full: read `docs/optimization/TASK_STANDING_RULES.md`
first.**

Branch: `codex/task54-stage-dma-overlap` — **branch from current master** (has
Task 53: replay is live). Record the parent SHA. This is the DMA follow-up that
Task 52 scouted and Task 53 unblocked. Do **not** touch the Task 51 or Task 53
flags.

New mode selector `NDS_TASK54_STAGE_DMA_MODE ?= 0`:

- **0** = current shipping CPU FIFO replay (Task 53, default) — the baseline/control.
- **1** = DMA_FIFO transport with an **immediate** completion barrier — control
  (isolates "DMA vs CPU writes" with no overlap; expected ~neutral).
- **2** = DMA_FIFO transport with a **deferred** barrier at the latest proven-safe
  point, allowing genuine CPU overlap of the stage FIFO drain — **the lever**.

Default 0 ⇒ published ROM unchanged (Task 53's shipped ROM).

Campaign: `docs/optimization/PROFILE0_NATIVE_CAMPAIGN.md` §5. Stage line: Task 51
(matrix bake) KILL → Task 52 (stage DMA) STOP → Task 53 (re-activate replay)
SHIPPED, loop now live → **this task** reclaims the backpressure.

## The finding this acts on (Task 53 E2, fork-measured, retail-accurate within 5%)

Re-activating replay dropped **STG P50 33%** (569,280 → 381,632) but **ALL P50
stayed flat** (−128) — the saving redistributed to **OTHR (+174,720)**. The
stage-owner CPU work shrank, but the DS geometry engine still has to process the
same 2,996 words, and that backpressure wait moved out of STG into OTHR (the CPU
stalls on GX completion before the next frame phase). **The cost is now
GX-throughput-bound.**

## Primary objective

Convert the OTHR backpressure into overlapped time: hand the stage FIFO feed to
DMA and let the CPU do independent non-GX work while the GX drains the stage, so
the wait is reclaimed at the **frame (ALL) level** — not just moved between
buckets. Target is **ALL/OTHR, not STG** (STG is already won). Honest: this only
pays off if such overlappable work exists — E0 gates it.

## Stages

### E0 — is there anything to overlap? (measure before you build)

The discipline that saved Tasks 51/52/53: prove the premise before implementing.

1. **Characterize the post-stage frame flow.** After the STAGE owner (0) submits,
   what does the ARM9 do before the next unavoidable GX/FIFO write? Owners MARIO
   (1) / FOX (2) follow — split their work into **non-GX CPU prep** (matrix / anim
   / DL build) vs **GX emit** (FIFO writes). Only non-GX work can overlap a DMA
   FIFO drain; the moment the CPU touches `GFX_FIFO` it must sync (two writers to
   the FIFO register = corruption).
2. **Measure the reclaimable window:** how much non-GX CPU work sits between stage
   submit and the first post-stage FIFO write, and how long does the stage GX
   drain take? Reclaimable ≈ `min(non-GX prep, stage drain)`. Use the M3 phase
   profiler or a targeted tick split.
3. **STOP at E0 if the overlap window is small.** If the CPU's next act after stage
   submit is GX emit that hits the same FIFO (nothing non-GX to run during the
   drain), mode 2 has nothing to reclaim and the OTHR is unavoidable FIFO
   serialization. Report the split and STOP. In that case the real lever is
   **geometry reduction** (fewer words for the GX: VTX_10, stripify, cull) — note
   it as the recommended alternative task; do not pivot to it here.

### E1 — mode 1 (control) then mode 2 (the lever), behind `NDS_TASK54_STAGE_DMA_MODE`

- **Mode 1:** replace the CPU loop (`ndsRendererTask36ReplayRun`,
  `src/nds/nds_renderer.c:~20273`) with a DMA_FIFO transfer of `owner->words` on a
  **dedicated channel** (channels 1/2 are free per Task 52 E0; channel 0 is texture
  staging, 3 is mid-frame fill — do not collide), **immediate** barrier. Proves DMA
  transport correctness in isolation.
- **Mode 2:** same DMA, but **defer the barrier** — do not wait after the stage
  DMA; let the CPU proceed to the E0-identified non-GX work; insert the
  DMA-completion sync at the latest proven-safe point, i.e. immediately before the
  first post-stage `GFX_FIFO` write. The overlapped work **must** be non-FIFO and
  independent of the stage GX result.
- **No per-frame `DC_FlushRange`** (`owner->words` is immutable, flushed once at
  capture — verify).
- Reuse Task 36's owner buffer and admission; whole-owner, no per-binding fallback.

### E2 — prove correctness, then measure

1. **Task 49 differ, STAGE owner, mode 2 vs mode 0:** Tier 1 **must be 0**
   (identical words), Tier 2 0.0 px. **BUT the differ cannot catch a FIFO collision
   or a mis-timed barrier** — the load-bearing checks are: owner visual A/B
   (`artifacts/visibility/`, you cannot see) across normal camera / wide zoom /
   pause orbit / fighter damage-shield-death-rebirth; zero GX fault / FIFO lockup /
   DMA collision; **state hash EXACT**.
2. **Fork A/B, modes 0 vs 1 vs 2**, ≥ 128 samples, same ROM/window/input. Report
   P50/P95/mean/max and the 2/3/4/5+ VBlank histogram for ALL, STG, FTR, SRC, OTHR,
   MISC. **The answer:** does mode 2 drop ALL/OTHR (not just move buckets)? Mode 1
   is the control — if mode 1 is neutral and mode 2 wins, the win is the overlap,
   not the DMA.
3. **The fork is retail-accurate within 5%** — it **is** the referee here; no
   separate device run is required **unless** the ALL delta is itself under ~5%
   (below the fork's error bar), in which case queue a device A/B in
   `builds/device-queue/` before shipping.

## Gates

- **Correctness:** differ Tier 1 = 0, Tier 2 0.0 px; state hash EXACT; owner visual
  approved; zero FIFO/DMA fault across all windows.
- **Perf:** mode 2 must show a real ALL/OTHR reduction (report the delta). If mode 2
  only moves buckets with ALL flat, that is an **honest STOP** — the backpressure is
  unavoidable and geometry reduction is the next lever. Do not force it.
- **Ship:** KEEP behind the flag; flipping the default is the owner's call (like Task
  53). Default 0 ⇒ Task 53's published ROM byte-identical.
- `.\scripts\verify-dev-fast.ps1` (bar the known pacing red) then
  `.\scripts\verify-boundary.ps1`.

## Traps

- **Two writers to `GFX_FIFO` (CPU + DMA) = corruption.** Mode 2's whole correctness
  is: overlap only non-FIFO work; sync DMA-done before the first CPU FIFO write.
- **Override trap:** thread `NDS_TASK54_STAGE_DMA_MODE` into the tick-HUD measurement
  target or `=2` is silently ignored; prove the built ROM took mode 2
  (preproc/objdump/boot marker) before trusting numbers. **Task 53 hit exactly this**
  — the flag never reached the C compiler.
- **One writer** on `src/nds/nds_renderer.c`.
- The differ green light is necessary but **not** sufficient — it cannot see a timing
  race.

## Constraints

- `decomp/` read-only (`decomp/sm64-nds/src/nds/nds_renderer.c` = DS FIFO/DMA
  reference, algorithms only).
- Long builds detached; build through `C:/devkitPro/msys2/usr/bin/bash.exe -lc '…'`.
- Time-box open-ended debugging ~10 runs / ~1 hour, then checkpoint and report. Cite
  `file:line` for every behavior claim. **Never push.**

## Deliverables

- Separate commits: (1) E0 overlap-window analysis, (2) mode 1 DMA transport, (3)
  mode 2 deferred barrier + reorder, (4) differ + A/B + visual + docs.
- The mode selector; E0 reclaimable-window estimate; differ certificate; modes 0/1/2
  A/B P50/P95 + VBlank dist; `artifacts/visibility/` screenshots.
- A clear **MERGE / KEEP-candidate / STOP** verdict with the ALL delta.
- Results section here; `PERF_LEDGER.md` entry; `HANDOFF.md`/`PORTING.md` notes.
- `.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean` as the final action.

## Final response (what to return)

1. Exact files changed. 2. E0 overlap-window verdict (non-GX prep available, stage
drain duration, reclaimable estimate). 3. GX differ (Tier 1/Tier 2) + state-hash
result. 4. Modes 0/1/2 A/B — ALL/STG/OTHR P50/P95 + VBlank distribution. 5. Owner
visual result. 6. FIFO/DMA fault check. 7. MERGE / KEEP-candidate / STOP decision
with the ALL delta and whether geometry reduction is the recommended next lever.

---

## Result (2026-07-24): STOP at E0 — no overlap window worth a DMA

Branch `codex/task54-stage-dma-overlap`, parent `482eb57` (Task 53 shipped,
replay live). **STOP at E0.** No DMA implementation admitted. Geometry reduction
is the recommended next lever. Full analysis:
`artifacts/performance/2026-07-24_task54-stage-dma-e0.md`.

### The decisive hardware facts (libnds, read-only)

- `GFX_FIFO` (`*(vu32*)0x04000400`, video.h:728) is **one** register into
  **one** command pipe feeding **one** geometry engine. Stage and fighters both
  write it, in strict arrival order.
- `glFlush` is **non-blocking** (`videoGL.h:724`: `GFX_FLUSH = mode;`). A
  frame-wide grep for `GFX_STATUS`/`GFX_BUSY`/any FIFO-empty poll across `src/`
  returns **nothing** — no explicit geometry-wait exists anywhere. Backpressure
  is implicit: a CPU store to a full pipe stalls inline until one slot drains.

### The decisive measurement (Task 53 A/B, already in tree, 128 samples)

Switching generic-emit → replay removed **187,648 ticks of stage CPU work** from
STG. STG+OTHR moved **732,992 → 720,064** (~constant, −1.8%). ALL was flat
(1,680,256 → 1,680,128). B run1 = B run2 byte-identical (deterministic).

**STG+OTHR is invariant to removing 187K of CPU work ⇒ the stage's frame cost is
dominated by the geometry engine draining its fixed 2,996 words, which is
GX-throughput-bound and identical regardless of who issues the stores.**

### Why mode 2 cannot reclaim it

- Mode 2's only overlappable work is the fighter's **non-GX** prep
  (`ndsFighterDisplayContractCapture`, counter `m2_contract_capture_ticks`) during
  the stage drain.
- The stage drain has **no slack** — it is the ~720K bottleneck, fully utilized.
- The fighter's **GX** writes append behind the stage drain in the same pipe;
  they cannot run during it.
- ⇒ maximum ALL win bounded to a few percent, **against** a deferred-barrier
  reorder crossing the stage→fighter owner boundary into a shared FIFO — the
  exact FIFO-collision corruption trap #1 warns about, and the differ cannot
  referee a timing race (trap #4).

### Recommended next lever (not pursued here)

Geometry reduction cuts the ~720K GX floor itself: packed `VTX_10`/`VTX_XY`
vertex formats, stripify the rigid static topology, cull off-screen bindings.
This is the lever DMA structurally cannot reach.

### Disposition

**STOP at E0.** No `NDS_TASK54_STAGE_DMA_MODE` runtime path added, no DMA
channel selected, published ROM unchanged. Branch holds the E0 analysis. Never
push.
