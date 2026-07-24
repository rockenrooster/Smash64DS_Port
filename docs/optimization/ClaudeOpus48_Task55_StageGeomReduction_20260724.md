# TASK 55 — Stage geometry reduction (cut the ~720K GX floor itself)

**Standing rules apply in full: read `docs/optimization/TASK_STANDING_RULES.md`
first.**

Branch: `codex/task55-stage-geom-reduction` — **branch from current master**
(has Task 53 replay live + Task 54's E0 STOP doc). Record the parent SHA. This is
the lever Tasks 51→54 all pointed at. Do **not** touch the Task 51/53 flags or the
Task 54 branch.

New selector `NDS_TASK55_STAGE_GEOM ?= 0`:

- **0** = current shipping emit (Task 53 replay, `FIFO_VERTEX16` 2-word verts,
  `GL_TRIANGLE` lists) — the baseline/control.
- **1** = the highest-value word reduction E0 identifies (VTX_10 packing and/or
  stripify), applied **at capture time** so the replay `owner->words[]` buffer is
  simply smaller — no per-frame cost added.

Default 0 ⇒ published ROM byte-identical to Task 53's shipped ROM.

Campaign: `docs/optimization/PROFILE0_NATIVE_CAMPAIGN.md` §5. Stage line: Task 51
(matrix bake) KILL → Task 52 (stage DMA) STOP → Task 53 (re-activate replay)
SHIPPED → Task 54 (DMA overlap) **STOP at E0** → **this task** attacks the floor
Task 54 proved is irreducible any other way.

## The finding this acts on (Task 54 E0)

Task 54 proved the stage's real frame cost — **STG+OTHR ≈ 720K ticks** — is
**GX-throughput-bound** on the **fixed 2,996 words** the 8 rigid Dream Land layer0
bindings emit. It is invariant to *who* issues the stores (CPU loop or DMA) and to
how much CPU prep you remove (the Task 53 A/B removed 187,648 ticks of stage CPU
work and STG+OTHR moved only −12,928). **The only lever left is sending the
geometry engine fewer words.** Task 51's differ already told us where the words
are: **~81% vertex, ~0.4% matrix** — vertex emit dominates, so vertex-word
reduction is where the floor lives.

## Two concrete reductions (both hit the dominant cost)

1. **VTX_10 packing.** The stage emits `FIFO_VERTEX16` = **2 words/vertex**
   (`nds_renderer.c:1210-1211` writes `GFX_VERTEX16 = xy; GFX_VERTEX16 = z;`;
   opcode table `4424-4425` = `FIFO_VERTEX16`, param_count 2). `VTX_10`
   (`FIFO_VERTEX10`) packs x/y/z into **1 word** at 10-bit signed precision.
   Ceiling: up to ~½ of the vertex words, i.e. a large fraction of the 2,996.
   **Cost: 10-bit vs 16-bit position precision** — a fidelity tradeoff E0 must
   size before committing (see trap).
2. **Stripify.** The stage draws `glBegin(GL_TRIANGLE)` **triangle lists**
   (`nds_renderer.c:20254` and the native emit path) = **3 verts/triangle**.
   `GL_TRIANGLE_STRIP` needs **N+2** verts for N triangles where the topology has
   shared-edge runs. Ceiling depends on how strippable the rigid static topology
   is — E0 measures it. **No precision loss** — this is the safe lever if the
   topology cooperates.

## Where the reduction lives (CodeGraph orientation — verify in E0)

The vertex format and triangle-list emission are governed by an **offline
generator + companion-checker toolchain**, not (only) the runtime C hot path.
Start here, not in `glVertex3v16`:

- `scripts/generate_nds_native_owners.py:1587` `pack_fifo_vertex16(x,y,z)` hard-codes
  the **2-word VERTEX16** encoding. VTX_10 = a `pack_fifo_vertex10` sibling here.
- `:360` `FIFO_PARAMETER_COUNTS` = the shared opcode→param-count table
  (`FIFO_VERTEX16=0x23`→2). Add `FIFO_VERTEX10=0x24`→1 here; the **companion
  checker (`check_nds_native_owner_packet.py` / `check_nds_native_stage.py`)
  decodes every generated payload before it can reach the ARM build** (`:344`) —
  it must learn the new opcode or it will (correctly) reject the packet.
- `:1781` `FIFO_BEGIN` is emitted as **`GL_TRIANGLE`** (lists). Stripify
  restructures the per-corner loop (`:1806-1843`) to `GL_TRIANGLE_STRIP`.
- `:1939-1948` **hard census asserts** (`expected_triangles = 320/306`, corners,
  store/restore) reject any geometry change until deliberately updated — a safety
  net; update them *with cited justification*, never silence them.
- `generate_nds_native_stage.py:3185` `stage_vertex_coordinate_shift` already fits
  x/y/z to ±2048 (12-bit) — the precision headroom E0 sizes is partly precomputed.

**Note:** `build_packed_fifo_owner_plan` above is the **fighter** (mario/fox) path;
the **stage** is `generate_nds_native_stage.py` (`GeneratedStageProgram`,
`DenseVertex`, `StageRun`). E0 step 0 resolves which locus actually produces the
shipping stage's 2,996 words.

## Fidelity posture (READ THIS — it changes what the differ means here)

This is a **render-side** change. Per the standing render-fidelity doctrine
(pixel-exactness was dropped for render-side work; **the owner is the visual
oracle**; gameplay stays bit-exact):

- **The Task 49 differ Tier 1 WILL fail by construction** — the words change on
  purpose. That is the **goal, not a regression.** Do **not** auto-revert on a
  Tier-1 delta here, and do not "fix" it back to bit-exact.
- **Differ Tier 2 becomes a FIDELITY METER, not a 0-gate:** report the
  screen-space px deviation the packing introduces (VTX_10 quantization will show
  as a small nonzero px; stripify alone should be ~0.0). This is a *number to
  judge*, not a pass/fail at 0.
- **The true gate is the owner visual A/B.** You (Codex) cannot see — capture
  `artifacts/visibility/` A/B screenshots and hand them to the owner.
- **State hash must stay EXACT.** A vertex-emit-format change is render-only; if
  it perturbs the simulation state hash, something is wrong — STOP and diagnose.

## Stages

### E0 — characterize the stream and compute the achievable floor (measure first)

The discipline that saved Tasks 51/52/54: prove the win exists before building it.

0. **Locate the stage emit locus (do this first — it decides E1's file).** Is the
   shipping stage's 2,996-word stream **generator-baked** (a `GeneratedStageProgram`
   instruction stream emitted offline, like the fighter packet) or **Task-36
   runtime-captured** (`sNdsRendererTask36ReplayOwner.words[]` filled from the live
   `glVertex3v16`/`GFX_VERTEX16` emit at `nds_renderer.c:20254`+)? Trace where the
   words the replay loop blasts originate. If generator-baked → E1 edits the Python
   generator + companion checker + census asserts. If runtime-captured → E1 changes
   the C emit format (`glVertex3v16`→packed, `glBegin(GL_TRIANGLE)`→strip) upstream
   of capture. Report the locus with `file:line` before proceeding.
1. **Break the 2,996 words down by GX command class** for the 8 rigid layer0
   bindings (reuse the Task 49 differ / Task 51 class histogram, or dump
   `owner->words[]` and classify). Report the exact count of: vertex
   (`FIFO_VERTEX16`) words, and everything else (matrix/color/texcoord/poly-attr/
   begin). Confirm the ~81% vertex figure at head resolution.
2. **Compute the theoretical word floor under each lever:**
   - **VTX_10:** `floor_v10 = total − vertex_word_count/2` (2-word→1-word). Report
     the % word cut.
   - **Stripify:** count triangles per binding and estimate strip vertex count
     (needs shared-edge topology analysis of the incoming DL). Report the % word
     cut and whether the topology actually strips (soup vs runs).
3. **Translate word-cut → ALL win.** Task 54 established words ∝ drain time and
   STG+OTHR (~720K) dominates ALL and is throughput-bound. A W% word cut ≈ W% off
   the ~720K floor ≈ the ALL-level win. State the predicted ALL delta for each
   lever.
4. **Size the VTX_10 precision cost.** Read the stage model-space extent (the
   coordinate range feeding `GFX_VERTEX16`) and compute the 10-bit quantization
   step in that space. If a step is a large fraction of a visible feature (thin
   platform lips, tree silhouettes), VTX_10 is risky — prefer stripify, or scope
   VTX_10 only to bindings where the step is sub-pixel.
5. **STOP at E0 if the achievable cut is small** (e.g. < ~10% words after ruling
   out precision-unsafe bindings) **or if neither lever is safe** (topology won't
   strip AND VTX_10 quantization is visible). Report the breakdown and the reason.
   Otherwise pick the **highest-value safe lever** for E1 and say why.

### E1 — implement the chosen reduction behind `NDS_TASK55_STAGE_GEOM`

- Apply the reduction at the E0-identified locus so the **replay loop is unchanged**
  and no per-frame cost is added; the emitted `owner->words[]` buffer is just
  shorter. Verify the one-time `DC_FlushRange` at capture still covers the (now
  smaller) buffer.
  - **If generator-baked:** add `pack_fifo_vertex10` + `FIFO_VERTEX10=0x24` to
    `FIFO_PARAMETER_COUNTS`, teach the companion checker to decode it, and update
    the census asserts (`expected_triangles`/corners) with cited justification —
    the checker + asserts are the correctness net, do not bypass them.
  - **If runtime-captured:** change the C emit upstream of capture
    (`glVertex3v16`→packed vertex, `glBegin(GL_TRIANGLE)`→`GL_TRIANGLE_STRIP`),
    behind the flag, leaving mode 0 byte-identical.
- Mode 1 = the E0-chosen lever. If E0 finds **both** safe and additive, they may
  compose, but land them as separate commits so the A/B can attribute each.
- Whole-owner, deterministic; no per-binding runtime branching in the hot path.
- Cite `file:line` (BattleShip decomp `decomp/`, read-only, algorithms only) for
  any vertex-format / strip-construction reference.

### E2 — fidelity (owner is the oracle) then perf

1. **Owner visual A/B is the gate.** `artifacts/visibility/` screenshots, mode 0
   vs mode 1, across normal camera / wide zoom / pause orbit / large flat surfaces
   / stage silhouette — the views where 10-bit quantization or a strip seam would
   show. Fighter damage-shield-death-rebirth for one match to confirm nothing
   downstream moved. **You cannot see — the owner judges.**
2. **Differ as fidelity meter:** report Tier 2 screen-space px deviation (expect ~0
   for stripify, small nonzero for VTX_10). **Tier 1 nonzero is expected — not a
   fail.** **State hash EXACT is a hard gate.**
3. **Fork A/B, modes 0 vs 1**, ≥ 128 samples, same ROM/window/input. Report
   P50/P95/mean/max and the 2/3/4/5+ VBlank histogram for ALL, STG, FTR, SRC,
   OTHR, MISC. **The answer:** does STG+OTHR (and hence ALL) drop by ≈ the word-cut
   fraction E0 predicted? The fork is retail-accurate within 5% and **is** the
   referee — a separate device run is only needed if the ALL delta is itself
   under ~5% (below the fork's error bar), in which case queue a device A/B in
   `builds/device-queue/` before any ship.

## Gates

- **Fidelity:** owner visual A/B **approved**; state hash **EXACT**; zero GX fault /
  FIFO lockup across all windows. (Tier-1 differ delta is expected and not a gate.)
- **Perf:** mode 1 must show a real STG+OTHR / ALL reduction close to the E0
  prediction. If the measured ALL win is far below the word-cut prediction, STOP
  and explain (the floor model was wrong somewhere) rather than shipping a fidelity
  cost for no gain.
- **Ship:** KEEP behind the flag; flipping the default is the owner's call (like
  Task 53), **and here it additionally requires the owner's visual sign-off**
  because it is a deliberate fidelity tradeoff. Default 0 ⇒ Task 53's published ROM
  byte-identical.
- `.\scripts\verify-dev-fast.ps1` (bar the known pacing red — attribute, don't
  chase) then `.\scripts\verify-boundary.ps1`.

## Traps

- **The differ is NOT the gate here.** Tier 1 nonzero is the whole point; do not
  revert on it, do not weaken the differ, do not chase bit-exactness. The owner's
  eye + state-hash-EXACT are the gates.
- **VTX_10 precision.** 10-bit signed = coarse. E0's model-space-extent check is
  load-bearing; do not blanket-convert without it. When in doubt, stripify first
  (lossless) and treat VTX_10 as the optional second commit.
- **State-hash perturbation.** If the emit change moves the simulation hash, you
  changed more than render — STOP.
- **Override trap:** thread `NDS_TASK55_STAGE_GEOM` into the tick-HUD measurement
  target or `=1` is silently ignored (Task 53 hit exactly this — the flag never
  reached the C compiler). Prove the built ROM took mode 1 (preproc/objdump/boot
  marker) before trusting numbers.
- **Generator/checker must move together.** A new vertex opcode or strip topology
  that the generator emits but the companion checker can't decode = a (correct)
  rejected packet; a census-assert left stale = a build failure. Update generator +
  checker + asserts in one commit. This is the toolchain protecting you, not
  fighting you — never silence a checker to make a packet pass.
- **One writer** on `src/nds/nds_renderer.c` (and, if generator-baked, on the
  `generate_nds_native_*.py` / `check_nds_native_*.py` pair).
- **Capacity:** a *smaller* buffer is fine, but confirm the capture path's
  bounds/asserts don't assume the old word count.

## Constraints

- `decomp/` read-only (`decomp/sm64-nds/src/nds/nds_renderer.c` = DS GX vertex/DL
  reference, algorithms only).
- Long builds detached; build through `C:/devkitPro/msys2/usr/bin/bash.exe -lc '…'`.
- Time-box open-ended work ~10 runs / ~1 hour, then checkpoint and report. Cite
  `file:line` for every behavior claim. **Never push.**

## Deliverables

- Separate commits: (1) E0 stream breakdown + floor computation + precision sizing,
  (2) the chosen reduction behind the flag, (3) optional second lever if additive,
  (4) differ-meter + fork A/B + visual + docs.
- The selector; E0 word-class histogram + predicted ALL delta per lever; differ
  Tier-2 px meter; state-hash result; modes 0/1 A/B P50/P95 + VBlank dist;
  `artifacts/visibility/` screenshots.
- A clear **MERGE / KEEP-candidate (owner visual pending) / STOP** verdict with the
  measured ALL delta.
- Results section here; `PERF_LEDGER.md` entry; `HANDOFF.md`/`PORTING.md` notes.
- `.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean` as the final action.

## Final response (what to return)

1. Exact files changed. 2. E0: word-class histogram, floor per lever, VTX_10
precision verdict, chosen lever + why (or STOP + reason). 3. Differ Tier-2 px meter
+ state-hash result. 4. Modes 0/1 A/B — ALL/STG/OTHR P50/P95 + VBlank distribution
vs the E0 prediction. 5. Owner-visual A/B screenshots captured (owner judges).
6. GX/FIFO fault check. 7. MERGE / KEEP-candidate (owner visual pending) / STOP with
the ALL delta.

---

## Result (2026-07-24): E0 — chosen lever is redundant state-write elision (NOT VTX_10/stripify)

Branch `codex/task55-stage-geom-reduction`, parent `a463975`. E0 census of the
2,996-word replay stream (Task 49 differ capture, frame 445, STAGE owner, 8
rigid bindings). Full analysis:
`artifacts/performance/2026-07-24_task55-stage-geom-e0.md`.

### The two named levers are not the highest-value safe cut

- **VTX_10: INFEASIBLE.** Stage model-space coords reach ±30,272; s10 range is
  ±511. 91% of X and 100% of Z would clip. Not usable for stage-scale geometry.
- **Stripify: 5.6% (168 words).** 606 verts / 427 unique (29.5% dup); only
  42/148 adjacent tri-pairs share an edge. Below the 10% gate; topology-limited.

### The real lever: redundant COLOR/TEX_COORD elision — LOSSLESS 20.6%

`GFX_COLOR` / `GFX_TEX_COORD` are persistent state registers; a vertex uses the
held value until it's rewritten. `ndsRendererNativeStageEmitNoZVertex`
(`nds_renderer.c:20448`) writes color **unconditionally per vertex**; no reset
exists anywhere (frame-wide grep empty; `EndBatch` at :13351 only touches alpha
test). Measured across the stream:

| state | writes | runs | redundant | % |
|---|---|---|---|---|
| COLOR | 606 | 50 | 556 | 91.7% |
| TEX_COORD | 591 | 529 | 62 | 10.5% |
| **total** | 1,197 | | **618** | |

**618 words = 20.6% of the 2,996-word stream, elided losslessly at capture.**
Predicted ALL delta ≈ ~148K ticks off ~720K STG+OTHR ≈ **~8.8% off ALL P50**.

### E0 verdict: PROCEED — mode 1 = redundant state-write elision at capture

Clears the 10% gate by 2× and is lossless. Implementation seam:
`ndsRendererTask36ReplayCapture` (`nds_renderer.c:~4450`) skips recording a
COLOR/TEX_COORD word whose value equals the last recorded one (per-owner
`last_color`/`last_texcoord`); replay loop unchanged, buffer just shorter.
Stripify deferred as an optional additive 2nd commit (lossless +5.6%) but not
the primary lever. VTX_10 dropped.

---

## Result (2026-07-24): E2 — STOP. Elision works + is lossless, but ALL flat.

Branch `codex/task55-stage-geom-reduction`. Elision implemented behind
`NDS_TASK55_STAGE_GEOM` (commit `c6a6228`), override-trap proven avoided
(config header carries the flag), byte-identity proven (default-off ROM =
`4D795B4E`). Runtime-proven: replay buffer 3,916 → 3,561 words (−355, −9.1%),
state=READY, no fault. Full E2 evidence:
`artifacts/performance/2026-07-24_task55-stage-geom-e2.md`.

### A/B (128 samples, frame 438, deterministic fork)

| bucket | A (off) P50 | B (on) P50 | Δ |
|---|---|---|---|
| **ALL** | 1,680,128 | 1,680,192 | **+64 (flat)** |
| **STG** | 381,632 | 377,408 | **−4,224** |
| **OTHR** | 338,432 | 346,048 | **+7,616** |
| STG+OTHR | 720,064 | 723,456 | +3,392 (~constant) |

VBlank: A 3:474/4:80/5+:12, B 3:478/4:76/5+:11, max 18, slips 0 — unchanged.

**The same invariant Task 53 hit:** STG drops (stage work removed), OTHR rises
(redistributed backpressure), STG+OTHR ~constant, ALL flat.

### Why — the floor is VERTEX16 transforms, not state words

Task 53 removed stage CPU prep (−187K STG); Task 55 removed redundant state
writes (−355 FIFO words); BOTH left ALL flat. Neither touched the **606
`FIFO_VERTEX16` commands** — the actual vertex transforms the geometry engine
performs. `GFX_COLOR`/`GFX_TEX_COORD` writes update a state register but do
not trigger a vertex transform. **The ~720K floor is the geometry engine
transforming 606 vertices + per-triangle setup.** Removing CPU prep or state
words removes their FIFO-store time, but the geometry engine still transforms
the same 606 vertices, and that drain dominates.

This completes Task 54's "what is the floor" question: the floor is the
VERTEX16 count, invariant to everything except **fewer VERTEX16 commands**.

### Verdict: STOP

Elision is correct + lossless + 9.1% buffer cut, but the perf gate is not met
(ALL flat). Per the spec's honest-STOP clause, this is a STOP. No ship, no
merge; flag stays default-off, published ROM unchanged (`4D795B4E`).

**Only remaining lever (untested): stripify** — reduces the VERTEX16 count
itself (ceiling 84 verts / 5.6% from E0). A targeted follow-up could prototype
GL_TRIANGLE_STRIP for the binding-3 run (best candidate: 66 verts / 22 tris in
one primitive) and measure whether the vertex-count cut actually drops ALL.
Carries topology-reorder correctness surface; ceiling is small.

---

## Owner visual A/B follow-up (2026-07-24, post-STOP)

The prior "lossless by construction / Tier-2 0.0 px" claim was scoped to a
**single static-frame** Task 49 differ capture (frame 438, replay buffer
drained pre-emptively for byte-comparison). It demonstrated summed-state
equivalence at one drained moment, not motion correctness through the
capture window.

The owner (the visual oracle per AGENTS.md) subsequently observed mode 0 vs
mode 1 in normal-battle play and reported that **some surfaces are
pulsating in color** in mode 1. This is a meaningful fidelity finding —
the elision is **not actually lossless across animation frames**, and the
static-frame differ did not referee it. Likely mechanism: `GFX_COLOR` /
`GFX_TEX_COORD` register state carries across frame boundaries, and a
downstream consumer for which write-count timing matters (vs summed value)
sees a frame-rate-sensitive delta when intermediate writes are elided.

**Disposition unchanged — STOP.** This is one more reason the elision does
not ship (in addition to the perf-gate fail already documented): the owner
A/B found a real visual delta. Flag stays default-off; published ROM stays
byte-identical to Task 53's `4D795B4E…`. Implementation is retained on the
branch as a checkpoint with negative-evidence value for future Tasks.

**Negative-evidence note for future elision candidates:** "lossless by
construction" is a *necessary-but-not-sufficient* claim. Cross-frame
held-state and any frame-rate-sensitive downstream must be verified, not
assumed. A robust differ for future state-write-elision candidates should
sample multiple frames and compare held register state at start-of-frame,
not only the drained word stream at one moment.
