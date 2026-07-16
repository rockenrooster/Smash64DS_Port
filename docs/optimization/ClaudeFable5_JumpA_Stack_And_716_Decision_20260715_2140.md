# Jump-A Stacking Task + 7/16 Decision Memo

**Author:** Claude Fable (planner) — 2026-07-15 21:40 Central
**State at write:** published Boundary ROM `CE922B60…` = **19.7 FPS**.
Jump B (M4 static residency + frozen water) LANDED: 22 keys / 131,072 B pinned
pre-GO, zero post-GO fence over a full one-minute natural match, reserve 163,312 B.
Jump A (M3 stage) PARTIAL: stage 804K→664,544; the dense-index prepare-once cut
(exact, −108,960) was reverted for missing the ≤500K / ≥164,544 gate.

**Why this task exists:** the ≤500K all-or-nothing M3 gate was correct when draw
was >2M; it is now counterproductive. At 19.7 FPS with ~1.5 build days left and
M4 already banked, an *exact* 109K stage cut must not sit on the floor. The new
policy is **bank every exact ≥50K stacked cut**, and reach the stage target by
STACKING three cuts rather than holding out for one 400K architecture.

Reality check that shapes this: only 66 of the stage's 202 triangles are
raw-current (hardware-transform-eligible); 126 are no-Z projected triangles that
pay soft-float per-vertex projection on an FPU-less CPU. The no-Z class is the
dominant stage cost, so the biggest lever is **fixed-point projection**, not
hardware T&L.

---

## TASK 1 — Jump-A stack (paste to codex)

```
/task Jump A (M3 stage) — STACK three exact cuts, bank each. Policy change: the
M3 gate is no longer "<=500K or revert." Bank every semantically-exact cut that
saves >=50K stage P50 ticks; stack them toward stage P50 <=300K (then <=250K).
One ledger row per cut in docs/PERF_LEDGER.md. No whole-owner FIFO copy/patch/DMA
packet and no new per-root interpreter (both proven-dead). Reconcile first per
OPTIMIZATION_ROADMAP R0; preserve the dirty tree.

CUT 1 — re-land the reverted dense-index prepare-once cut.
- It was exact and measured 664,544/664,640 -> 555,584/555,776 (-108,960).
  Evidence: artifacts/visibility/m3-dense-prepare-8frames.json and
  m3-dense-prepare-frame438.png. Reconstruct it (it was hand-reverted, not a git
  revert) as the STACK BASE, not a standalone experiment.
- Gate: profile-2 semantic oracle zero-mismatch, same-ROM A/B/A reproduces the
  -108,960 P50, exact 121/828 ownership + 42F+W / 202F+2W accounting, reserve
  >=128 KiB. KEEP it.

CUT 2 — fixed-point no-Z projection (the dominant stage lever).
- The 126 no-Z projected stage triangles project per-vertex in software float
  (no FPU). The projection divide + clamp lives in the no-Z painter path around
  src/nds/nds_renderer.c:10417-10480 and :10847 (ndsRendererHardwareClampS64ToV16).
- Convert that per-vertex projection to fixed-point (DS hardware DIV register or
  20.12/16.16 integer path), matching the source's own syMatrixF2LFixedW 16.16
  quantization boundary. This is DISPLAY math, not gameplay — drift is tolerable
  IF gated. Read the BattleShip projection source first (cite file:line).
- CRITICAL invariant: preserve the source G_ZBUFFER background/foreground painter
  phases exactly (the fence-over-floor layering counted down/up around the first
  source-Z triangle, nds_renderer.c:10417-10419, :11368). The synthetic no-Z
  painter counter semantics must not change.
- Gate: profile-2 oracle exact OR, if fixed-point drifts sub-pixel, publish max
  per-vertex deviation and hold for Tyler's capture sign-off (pixel goldens are
  his call). Same-ROM A/B/A P50 delta in the ledger. Expected: this is where the
  large stage saving is; target this cut alone >=150K.

CUT 3 (only if 1+2 land with time left) — hardware T&L the 66 raw-current
stage triangles.
- Load the live per-list stage matrix into a GX stack slot and submit the 66
  raw-current triangles as model-space vertices so the geometry engine
  transforms them; freeze their model-space verts/ST/colors. Do NOT touch the
  no-Z/range/cross classes. Stacked cut, not standalone.
- Gate: same as Cut 1; assert zero cross-matrix in this subset; reserve intact.

After the stack: re-profile the whole mode-163 frame (update/audio/HUD/flush/
VBlank/residual), publish stage P50/P95 and whole draw, capture frames 438/439
plus a moving mid-combat frame, run the one-minute natural match to confirm M4
fence still zero and teardown normal. Same-ROM A/B/A, 8-frame falsifier then
32/128 for kept cuts. Land KEEPs; snapshot with New-Smash64DSSnapshot.ps1 -Mode
Lean as the final action.
```

---

## TASK 2 — 7/16 hard feasibility checkpoint (paste to codex)

```
/task 7/16 checkpoint — produce the target decision the board requires, in
writing, before the 7/17 freeze. Do NOT silently redefine P1.

1. Publish phase-specific P95 active ticks on the current published ROM for:
   countdown/GO, early combat, late combat, KO/rebirth, Time Up/Results. Use the
   canonical profile-0 artifact and synchronized windows; fill the P1_EXECUTION_
   BOARD Phase Evidence table (no cross-build recycling — Pending beats a wrong
   number).
2. Compute the ladder against the vblank budgets (1 vblank=560,190; 30fps=
   1,120,380; 20fps=1,680,570). State current per-phase FPS and the gap to
   locked-30 (present <=~1,120K) in EVERY material phase, including interface.
3. Fold in the projected Jump-A stack saving (Task 1 CUT 1+2+3). State whether
   the stack, if fully landed, closes the locked-30 gap in the worst phase.
4. Present exactly one of three, with the number that justifies it:
   (a) 60 FPS still credible — only if a material path converges near 560K in
       every phase (evidence says no; if so, say no explicitly);
   (b) locked-30 "playable preview" — requires the scheduler design (60Hz source
       logic, 29.9Hz present, no catch-up spiral, input/audio policy). BUDGET
       this at ~half a day before the 7/17 freeze;
   (c) stable capped ~20 FPS "playable preview" — if the stage will not crack to
       the ~400K locked-30 needs.
5. Recommend one option with reasoning; Tyler makes the final call. Whatever is
   chosen, restore the strict pacing assertion for that target as default-hard in
   the canonical verifier in the SAME change. Append the decision to the board's
   Dated Gates (July 16) and the ledger.

No new architecture in this task — measurement, arithmetic, and the written
decision only.
```

---

## TASK 3 — Jump C (M2 fighters) — attack the CPU buckets, NEVER a packet

Scope reality first: fighters are ~412K combined (Mario 188K + Fox 224K); the
stage is 664K. Jump C is **lower-leverage than Jump A** and its realistic
outcome is ~412K → ~330K, not the 170–250K milestone. Its value is that the
fighter matrix code (`reloc_backend_renderer_dl.c`) is largely disjoint from the
stage no-Z work (`nds_renderer.c`), so it can run on a **parallel lane** — but
both touch the renderer, so the integration lane must arbitrate shared
`nds_renderer.c` edits. Do NOT let Jump C compete with the locked-30 scheduler
for the last pre-freeze hours; if it's a choice, drop Jump C.

```
/task Jump C (M2 fighters) — reduce the matrix + lighting CPU buckets DIRECTLY.
Forbidden (all proven dead, do not revive): whole-owner FIFO copy/patch/DMA
packet (+124K), per-joint GX hierarchy matrices (84 restores, +33K transport),
DS hardware lighting (misses 102/413 RGB15 cases), any new per-root interpreter.
This task touches ONLY the exact CPU derivation, keeps the existing Mode-8
production owner, and preserves the 32-root/49-epoch/67-run/626-triangle/
44-cross-matrix contract. Reconcile first (OPTIMIZATION_ROADMAP R0); coordinate
any nds_renderer.c edit with the stage lane. Keep the active-animlock / dynamic
-matrix-state fail-closed preflight (commit 6d6fba9685): census zero, reject
before the first GX write.

CUT 1 — fighter local-matrix construction (measured ~72,896 ticks).
- The per-joint locals are built in software float and quantized to 16.16 via
  syMatrixF2LFixedW: syMatrixTraRotRpyRSca / syMatrixRotPyrR / syMatrixRotRpyR /
  syMatrixTraRotPyrRSca at src/port/reloc_backend_renderer_dl.c:333,504-519,
  526,544,595,764,887; syMatrixF2LFixedW at :489,:589. The ARM9 has no FPU.
- PROFILE the bucket first (R2): identify the dominant soft-float op. If the
  imported matrix builders call libm sinf/cosf for rotation, route them through
  the BattleShip source's own u16-angle sin/cos tables (sins/coss) — that is
  BOTH faster AND more source-faithful than libm. Read the BattleShip source for
  the exact table + matrix functions before changing anything (cite file:line);
  do not hand-author trig.
- Where the source already quantizes to 16.16, do the arithmetic in fixed-point
  rather than float. This is display math (the RSP sees the fixed result anyway)
  — sub-LSB drift is acceptable if gated.
- Gate: profile-2 matrix oracle exact, OR publish max per-element deviation vs
  the float path and hold for Tyler capture sign-off. Same-ROM A/B/A P50 delta
  in the ledger. Bank if >=40K exact. Do not regress the fixed-W quantization the
  RSP contract depends on.

CUT 2 — fighter lighting (measured ~79,648 ticks) — MEASURE BEFORE TOUCHING.
- This path already has a prepared per-part light direction and a 2,096-byte
  shade LUT cache: ndsRendererHardwareLitShadeColorLut (nds_renderer.c:2041),
  prepared_light_direction (:2105), the LUT cache (:2004-2010,:2401,:2406). It
  may already be near its floor.
- Only optimize if profiling shows real per-vertex residual work BEYOND the LUT
  lookup (e.g. a per-vertex normal transform that could be hoisted to per-part,
  or a cache-miss rate worth reducing). If the bucket is already a bare lookup,
  record that and STOP — do not manufacture a cut. Lighting stays CPU and stays
  source-exact (profile-2 oracle governs).

CUT 3 (only if time) — extend the KRAW shared kernel to more fighter raw runs
than it already covers (246 Mario + 234 Fox today). Stacked, not a rewrite.

Gates: matrix+light combined <=120K (from ~152K); each kept cut is exact and
>=40K; profile-2 semantic + owner entry/exit + vertex-cache + screenshot all
agree; reserve >=128 KiB; no per-frame allocation. Same-ROM A/B/A, 8-frame
falsifier then 32/128 for kept cuts. Ledger row per cut. Snapshot last.
```

## TASK 4 — TWO DEPTH BUGS (fix BEFORE Jump A CUT 2; these are correctness, not perf)

Tyler reported two visible defects. Both live in the CPU-projected depth path
that Jump A CUT 2 rewrites, so they must be fixed FIRST, as their own
pixel/oracle-gated commits (one variable each), and THEN CUT 2's fixed-point
conversion sits on the corrected path. **Amendment to Task 1 CUT 2:** its
"preserve the source painter phases exactly" becomes **"match the SOURCE depth
ordering"** — the current phases are partly WRONG (bug 1), so preserving them
would preserve the bug.

### Bug 1 — stage triangles overlap the floor top-grass and side bushes (normal play)

Diagnosis: the port routes every stage triangle through the DS hardware depth
buffer, but N64 stage decals (grass tops, bushes, fences) are *no-Z painter*
primitives. The port emulates their order with a synthetic monotonic depth
counter and packs it into the SAME v16 depth channel as the real
perspective-Z geometry:
- classification/band assignment: `src/nds/nds_renderer.c:11330-11340`
  (`source_zbuffered` → projected_z=0 uses composed clip Z; else →
  `ndsRendererHardwareNextProjectedDepth()` synthetic painter value);
- the two depth writers: perspective-Z `ndsRendererHardwareClipVertexDepth`
  (~:10406) vs synthetic-Z `ndsRendererHardwareClipVertexNdcDepth` (:10426);
- context flags ZBUFFERED/DECAL_DEPTH/PRIM_DEPTH/SOURCE_CLIP_DEPTH at :11369-11376.

Two candidate faults (profile-2 trace records `projected_z` per triangle —
use it): (a) a grass/bush/fence DObj is **misclassified** (wrong zbuffered/
decal flag vs its source G_RM/G_ZBUFFER render mode), landing in the wrong
band; (b) the synthetic painter band **collides** with the real perspective-Z
range at the live camera distance. Also suspect a **regression** from the M3
stage-owner reorder (ef65ef541c) changing callback/draw order feeding the
painter counter — bisect against a pre-M3-owner ROM.

Fix method: read the BattleShip Pupupu stage source render modes for the
offending DObjs (grass top, side bushes, fences — start at the stage file group
around `104_StagePupupuFile2` / `dStagePupupu`, cite exact file:line), confirm
each maps to the correct port classification, and guarantee the synthetic
painter band is kept strictly outside the real-Z range for all camera
distances in the match envelope. Gate: capture shows correct occlusion (grass/
bushes occlude what they should); profile-2 per-triangle depth ORDER matches
source order; **not** perf-gated. Land as its own commit.

### Bug 2 — pause-orbit geometry corruption blocks the screen at wide angles

This is the board's open P2 "pause-orbit geometry containment" and the
historically-demoted near-plane finding — demoted only because the *front-facing
canonical* scene never crosses the near plane. Pause-orbit does.

Diagnosis (confirmed in code): the perspective divide
`ndsRendererHardwareProjectToV16` (`src/nds/nds_renderer.c:4340-4404`) has **no
near-plane clip**. When the camera orbits so a vertex crosses behind the near
plane, `vtx->w` goes to ~0 or negative; the function either drops the vertex
(`vtx->w == 0` early return at :10401/:10437) or pre-clamps X/Y to ±32767
(:4362-4380), flinging it to a screen corner. A triangle with one vertex behind
the camera becomes a giant stretched primitive spanning the screen. The N64 and
the DS geometry engine both clip against the near plane in clip space; this port
CPU-projects and submits post-divide screen coords, bypassing that clipper.

Fix options (pick by remaining time):
- **Correct:** near-plane clip each triangle in clip space (W = near) BEFORE the
  CPU perspective divide, emitting 0/1/2 output triangles. Bigger but right.
- **Interim (demo-safe):** if any vertex has `w <= near_epsilon`, DROP the whole
  triangle instead of saturating — geometry vanishes near the camera rather than
  exploding across the screen. Cheap, and acceptable for a demo where pause-orbit
  is incidental. Add a counter.
- **Cheapest mitigation:** if the pause camera orbit range is port-controlled,
  clamp it to the clean envelope (board says front/±16.8° clean, ±33.6° breaks)
  — but only if that does not diverge from source pause-camera behavior.

Gate: pause-orbit capture at the previously-breaking angles shows no
screen-blocking corruption; front-facing normal play is byte-identical
(profile-2 exact — the fix must not touch in-envelope frames). Keep the board's
deterministic pause-angle symptom gate. Land as its own commit, separate from
Bug 1 and from CUT 2.

### Sequencing

1. Bug 1 fix (own commit, pixel+order gated).
2. Bug 2 fix (own commit, pause-angle + front-facing-exact gated).
3. THEN Jump A CUT 2 fixed-point projection on the corrected path (perf-gated,
   same-ROM A/B/A). Do not A/B a perf cut against a buggy baseline.

## Planner notes (not for the /task blocks)

- **Order:** Task 1 CUT 1 (re-land, ~1h) → CUT 2 (fixed-point no-Z, the real
  win, most of a day) → Task 2 checkpoint → CUT 3 only if time. If CUT 2 lands
  ~150–200K, stage reaches ~350–400K and locked-30 becomes arithmetically
  plausible; if CUT 2 stalls, option (c) capped-20 is the honest deliverable.
- **Do not** let the fixed-point projection touch gameplay collision/position
  math — it is renderer-display-only. The stage owner reads live camera/DObj
  transforms; only the final screen-projection arithmetic changes.
- **Frozen water** is a visible compromise riding on the published ROM; Tyler
  must sign off the static-water capture regardless of the fps outcome.
- **Locked-30 scheduler** is itself pre-freeze work — if option (b), it competes
  with CUT 3 for the same hours. Prefer banking locked-30 over chasing CUT 3.
```
