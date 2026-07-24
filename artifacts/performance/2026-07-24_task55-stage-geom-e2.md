# Task 55 — Stage geometry reduction: E2 measurement + verdict

**Date:** 2026-07-24
**Branch:** `codex/task55-stage-geom-reduction` (E2)
**Parent:** `a463975`
**Outcome:** **STOP — the elision works and is lossless, but ALL is flat.**
The geometry-engine floor is the 606 VERTEX16 vertex transforms, not the
state words or CPU prep. Removing redundant COLOR/TEX_COORD writes shrinks
the replay buffer 9.1% but cannot cut the floor. This completes Task 54's
analysis: the floor is cut ONLY by fewer VERTEX16 commands.

## E1 recap (verified)

- **Override trap avoided:** `builds/build-task55-mode1/nds_build_config.h`
  carries `#define NDS_TASK55_STAGE_GEOM 1`.
- **Byte-identity proven:** default-off published ROM reproduces
  `4D795B4E83B33559...` (Task 53 shipped) byte-for-byte.
- **Elision works at runtime** (GDB probe of `sNdsRendererTask36ReplayOwner`,
  frames 438-440, profile-0 tick-HUD):

| mode | state | word_count | frame_replay | capture_fault |
|---|---|---|---|---|
| mode-0 (elision OFF) | 2 (READY) | 3,916 | 1 | 0 |
| mode-1 (elision ON)  | 2 (READY) | **3,561** | 1 | 0 |

The replay buffer shrank **355 words (9.1%)** — 355 fewer FIFO writes per
frame. Replay stays READY, no fault. (E0 predicted 618 from the profile-1
differ capture; the profile-0 replay reality is 355 — the runtime number is
the ground truth and is what the perf A/B uses.)

## E2 A/B (128 samples, frame 438, same fork/window, deterministic)

A = mode-0 (elision OFF, replay live), B = mode-1 (elision ON). melonDS fork
`DE80E46B…` (models icache/dcache).

| bucket | A P50 | B P50 | Δ P50 | Δ P95 |
|---|---|---|---|---|
| **ALL** | 1,680,128 | 1,680,192 | **+64 (0.00%, flat)** | +192 |
| FTR | 579,264 | 577,472 | −1,792 | +2,560 |
| **STG** | 381,632 | 377,408 | **−4,224** | −5,056 |
| BG | 4,224 | 4,096 | −128 | −64 |
| AUD | 2,496 | 2,304 | −192 | +2,752 |
| HUD | 1,024 | 960 | −64 | +1,792 |
| SRC | 318,336 | 317,760 | −576 | −9,344 |
| MISC | 48,704 | 47,808 | −896 | +832 |
| **OTHR** | 338,432 | 346,048 | **+7,616** | −640 |
| **STG+OTHR** | **720,064** | **723,456** | **+3,392 (~constant)** | |

VBlank histogram (565 presented frames): A = 2:0 / **3:474** / **4:80** /
**5+:12** / max 18; B = 2:0 / **3:478** / **4:76** / **5+:11** / max 18.
slips 0 both. Essentially unchanged.

## What the A/B says — the same invariant Task 53 hit

STG drops 4,224 (the elided FIFO-store time leaves the stage bucket), but
OTHR rises 7,616 and STG+OTHR is ~constant (720,064 → 723,456). **ALL is
flat (+64, 0.00%).** This is the exact pattern Task 53 showed:

| task | what was cut | STG Δ | OTHR Δ | STG+OTHR Δ | ALL Δ |
|---|---|---|---|---|---|
| Task 53 (replay) | stage CPU prep | −187,648 | +174,720 | −12,928 | −128 (flat) |
| **Task 55 (elision)** | **redundant state writes** | **−4,224** | **+7,616** | **+3,392** | **+64 (flat)** |

Both removed real stage work; both left ALL flat; both saw STG+OTHR ~constant.

## The decisive reconciliation — what the floor actually is

Task 54 attributed the ~720K STG+OTHR floor to "the geometry engine draining
the fixed 2,996 words." Tasks 53 and 55 now refine that jointly:

- **Task 53** removed ~187K of stage *CPU prep* (the per-triangle matrix walk /
  vertex-prep in the generic emit). STG dropped, ALL stayed flat.
- **Task 55** removed 355 redundant *state writes* (COLOR/TEX_COORD) from the
  FIFO. STG dropped, ALL stayed flat.

Neither touched the **606 `FIFO_VERTEX16` commands** — the actual vertex
transforms the geometry engine performs. A `GFX_COLOR` / `GFX_TEX_COORD`
write updates a state register; it does NOT trigger a vertex transform. Only
`FIFO_VERTEX16` does. **The ~720K floor is the geometry engine transforming
606 vertices, plus per-triangle setup.** Removing CPU prep or redundant state
words removes their FIFO-store time but the geometry engine still transforms
the same 606 vertices, and the backpressure from that transform dominates.

This is why Task 53's replay win (−187K stage CPU) and Task 55's elision win
(−355 FIFO writes) both redistributed to OTHR instead of dropping ALL: the
geometry-engine vertex-transform drain is the long pole, and it is invariant
to everything except **the VERTEX16 count itself**.

## The only lever that cuts the floor — fewer VERTEX16 commands

- **Stripify** (the one lever that reduces vertex count): ceiling 84 verts
  (5.6%) from E0. Even this may be smaller in practice — the geometry engine
  has per-triangle setup cost too, and a strip doesn't eliminate that. A
  targeted follow-up could prototype GL_TRIANGLE_STRIP conversion for the
  binding-3 run (66 verts / 22 tris in one primitive — the best strip candidate)
  and measure whether the vertex-count cut actually drops ALL. **This is the
  one untested lever remaining.** But its ceiling is small (≤5.6% words) and
  it carries topology-reorder correctness surface.
- **VTX_10**: infeasible (E0 — coordinates out of s10 range).
- **State elision (Task 55)**: works, lossless, but does not cut the floor.

## Correctness (the gates that matter)

- **Elision is lossless by construction:** `GFX_COLOR`/`GFX_TEX_COORD` are
  persistent registers; removing a redundant write leaves the held value
  identical at every vertex → identical render. The replay struct confirms
  state=READY, no fault.
- **State hash:** mode-0 and mode-1 differ only in the draw capture path
  (which runs after all gameplay/sim state is computed). The elision changes
  which redundant register writes happen during the draw; it cannot move the
  sim hash. The existing master state-hash red (Task 45: relocated heap
  pointers, not gameplay) is unrelated and unchanged.
- **Visual A/B:** `artifacts/visibility/task55/task55-A-mode0-baseline.png`
  and `task55-B-mode1-elided.png` (416×664 each). Owner is the oracle; the
  structural lossless argument (identical geometry-engine state per vertex)
  is the stronger proof.

## Verdict — STOP

The elision is a correct, lossless 9.1% replay-buffer reduction, but the perf
gate (real ALL/OTHR reduction) is **not met**: ALL is flat (+64, 0.00%) and
STG+OTHR is ~constant, because the floor is the 606 VERTEX16 vertex
transforms, not the state words this lever removes. Per the spec ("if mode 1
only moves buckets with ALL flat, that is an honest STOP"), this is a STOP.

**This is an informative STOP** — it completes Task 54's "what is the floor"
question with the joint evidence of Tasks 53+55, and it localizes the only
remaining lever precisely: **reduce the VERTEX16 count** (stripify), not the
state words or CPU prep. No ship. The flag (`NDS_TASK55_STAGE_GEOM`, default 0)
stays default-off; the published ROM is unchanged (`4D795B4E`).

## Disposition

**STOP.** Branch `codex/task55-stage-geom-reduction` holds E0+E1+E2. No merge
(STOP outcome). The implementation is correct and retained on the branch as a
checkpoint. Published ROM unchanged. Never push.
