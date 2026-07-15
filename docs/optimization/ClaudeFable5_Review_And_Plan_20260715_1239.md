# Big-Jumps Review & Plan — 17fps lab / ~15fps canonical → target

**Author:** Claude Fable (planner) — 2026-07-15 12:39 Central
**Audience:** codex + Tyler. Review + plan only; no implementation.
**Grounded in:** OPTIMIZATION_ROADMAP.md, P1_EXECUTION_BOARD.md,
NATIVE_RENDERER_PLAN.md, PERF_LEDGER.md, and live reads of
`reloc_backend_renderer_dl.c` (matrix path) + `nds_platform.c` (BG/VRAM).
**Deadline:** 7/19; hard feasibility checkpoint 7/16; freeze 7/17. ~2 build
days remain. This plan is ruthlessly prioritized for that.

---

## Part 1 — Review: why some jumps stuck and most didn't

I read every rejected experiment. There is one pattern that explains all of
them, and it is the most important thing in this document:

### The jumps that STUCK all moved a subsystem onto a dedicated DS hardware block

| Win | Before | After | Mechanism |
|---|---:|---:|---|
| M1 wallpaper (Cut G) | 237,000 | 1,856 | 2D **affine BG** engine samples it for free |
| Lower HUD off top screen | foreground 788,160 | 0 | moved to **bottom screen** text |
| Native countdown / GO | foreground 1,863,232 | 10,336 | 2D **OAM sprite** engine |
| KRAW-SHARED kernel | — | −208,672 | branchless **specialized run kernel** |

Three of four are "hand the work to a fixed-function 2D unit that costs zero
CPU per frame." The fourth (KRAW) is the only 3D win, and it worked by running
a **straight-line specialized kernel** over an immutable run — not by caching
output.

### The jumps that FAILED all tried to cache-and-replay a 3D GX command packet

| Reject | Result | Why it died |
|---|---:|---|
| M2 per-joint GX hierarchy | +saved 13,472 (missed 80K) | matrix fell 18K but **DL transport rose 33K** (84 matrix restores) |
| M2 whole-owner FIFO copy/patch/DMA | **+124,288 regression** | copy + word-patch + cache-flush + DMA cost **more** than the CPU work it replaced |
| M2 split projection/modelview | +36,200 | too small |
| M2 GX palette + parallel CPU geom | +48,100 | kept the CPU work it was meant to remove |
| M4 2–4 slot output LRU | ~0 hits | reuse distance 216; nothing to cache |
| M4 on-demand NitroFS stream | draw P95 +21,376 | I/O on the critical path |

**Root cause, stated plainly:** T2B already proved GX *transport* is ≤100K and
that **CPU policy/derivation is the wall.** Every failed M2 packet then added
*more* transport (copy/patch/flush/DMA) on top of the derivation instead of
removing the derivation. You cannot out-cache dynamic output; the packet
machinery's own overhead is larger than the win.

### Two hard walls the experiments have now proven — stop pushing on them

1. **Hardware lighting is dead.** DS one-light shading misses 102 of 413 exact
   RGB15 cases across 16/18 bindings. Lighting is a **permanent CPU sidecar.**
   No more attempts to move it to GX.
2. **GX hardware matrices lose on deep hierarchies.** The fighter joint tree
   needs 84 matrix restores; the transport of those matrix commands exceeds the
   CPU-projection it saves. So hardware T&L is **wrong for fighters** — but that
   verdict does **not** transfer to the stage (see §2.1).

### Honest 60fps reachability call (you asked me to use my judgment)

Draw is 1,575K; active (present) ~1,993K; one VBlank = 560K. Even if M2 hits
250K (−162K), M3 hits 250K (−554K), and M4 removes 189K, draw lands ~570K and
**active lands near ~1,000K — that is locked-30 territory, not 60.** Reaching
one VBlank additionally requires halving update/audio/flush, which is
behavior-sacred imported code you can only touch through the toolchain.

**My call: 60fps is not reachable by 7/19. The honest, achievable deliverable
is a stable, verifier-clean, locked-30 "playable preview."** That was always
the documented floor. Build the plan below to *maximize* the cut (every tick
helps input latency and headroom), but make the 7/16 checkpoint decision with
locked-30 as the expected outcome, not a fallback you're ashamed of. Say this
to Tyler now, not on the 18th.

---

## Part 2 — The plan: three big jumps, in leverage order, each engineered to
## avoid the failure pattern

Design law for all three (this is the whole lesson): **do not build another
record/replay packet.** Either (a) offload to a fixed-function unit, or (b) run
a branchless specialized kernel over immutable runs, or (c) attack the
derivation cost directly in fixed-point. Nothing copies-then-patches a big GX
stream.

### JUMP A (do first, highest leverage) — M3 stage via single-matrix hardware T&L

**Why this is the top pick:** stage is now the biggest owner (**804,032**, 51%
of draw — bigger than both fighters combined) and it is the one 3D owner that
is *structurally suited to the winning pattern* yet has **no production attempt
yet** (only a host packet, production_linked=0).

**Why hardware T&L wins here where it lost for fighters:** I verified the
matrix path — the stage uses the **live matrix per list with zero cross-matrix
triangles** (NATIVE_RENDERER_PLAN §M3), i.e. a *shallow* transform, ~42 lists,
no 84-restore joint hierarchy. Loading the stage's matrix into the GX stack and
submitting model-space vertices costs a handful of matrix loads, not 84. The
transport math that killed the fighter packet **inverts** for the stage.

**What is immutable vs live (this decides what freezes):**
- Immutable per frame: topology, the 66 raw + static no-Z vertex positions in
  model space, static ST, static vertex colors (stage is textured, largely
  unlit — confirm the unlit assumption in the census).
- Live per frame: the camera/world matrix (one, or one-per-list), water ST
  scroll, Whispy blink/flower animation selection, segment-E material/FRAC.

**Construction (the anti-packet shape):**
1. Freeze static model-space verts/ST/colors into rodata/setup slab (≤16 KiB,
   no new BSS/heap — the M3 contract already says this).
2. Per frame: load the live camera matrix into a GX stack slot; stream the
   frozen static runs through **KRAW-style branchless kernels** (extend what
   already worked), letting the **geometry engine do the transform+projection.**
   This removes the per-triangle CPU projection/divide for static geometry —
   the dominant derivation cost — without any copy/patch/DMA.
3. The 126 no-Z triangles: replicate the source's background/foreground painter
   layering with **GX polygon depth attributes / draw order**, not CPU
   projection. This is the one genuinely hard part; prototype it against the
   profile-2 oracle first (host fixture already exists).
4. Water ST + Whispy/flower are the **only** live patches, bounded to a few
   dozen words — patch those slots, everything else is frozen.
5. Whole-owner preflight validates callback/DObj/DL identity + material shadow
   before the first GX write; any mismatch → generic owner fallback for the
   whole frame (the contract already specifies this).

**Falsification gate (already set, keep it):** first paired A/B/A saves ≥300K
stage ticks, reaches ≤500K, P95 improved, exact census/order/hashes/screenshots.
Final target 150–250K. **Budget:** 1.5 days. If the no-Z depth layering fights
back past a half-day, land the 66-raw + static no-Z-on-cold-path subset (still
a large win) and defer the hard no-Z class.

Expected: stage 804K → ≤300K first cut; draw 1,575K → ~1,070K; ~17 → ~22fps.

### JUMP B (do in parallel, low renderer risk) — M4 static texture prewarm

**Why now:** removes the ~189K conversion wall, and — critically — **it
unblocks Jump A** by making texture bindings frozen constants instead of
per-frame conversions. It is mostly a memory-lifetime problem, not renderer
surgery, so it can run on a second lane without touching the stage code.

**Do exactly the M4-static-256 checkpoint, nothing more ambitious:**
1. After battle setup returns, enumerate every static texture/palette key
   (Dream Land static geometry, Whispy roots, Mario/Fox, fireball, reflector).
   Census is nearly closed: 16 blocks / 20 keys / 90,112 dedup bytes.
2. Convert once, pin to VRAM with stable keys, upload **before GO.**
3. Arm a violation fence at the source Wait→GO transition: latch any post-GO
   conversion/upload/alloc/eviction/I/O. Allow only the 2 existing water
   refreshes. Prove zero through Time Up/Results.
4. Publish RAM/VRAM/reserve — must hold ≥128 KiB reserve.

**Water stays a separate representation problem — and here is the specific
mechanism (this is the one place I disagree with "convert every phase"):** the
Dream Land water is a **palette animation over immutable indices** (STATUS: two
immutable 32×32 index planes decode once; only palette/fraction/phase advance).
So each phase collapses to a **palette**, not a new texture. The DS-native fix
is: upload the index texture once, precompute the small fixed set of phase
palettes at setup, and per frame **swap the palette-base register** (32 bytes,
or a register field) — not reconvert. The just-committed RGB256 tiled-AOT water
packets (c85bac721e/cb742db044) are the raw material; drive them toward
palette-swap residency, not per-phase upload. Target post-GO water work ≈ a
register write.

**Gate:** zero post-GO conversion/upload counters; water still animates
correctly (profile-2 pair-pixel oracle 18,432/0-class); reserve ≥128 KiB.
**Budget:** 1 day, parallel lane.

Expected: −189K conversion + frozen bindings for Jump A. draw → ~880K after A+B.

### JUMP C (only if A landed early) — M2 fighters, but attack derivation, NOT a packet

**Stop building fighter packets.** Every one has failed. The residual fighter
cost is two CPU buckets the packet experiments never isolated because they were
busy caching output:

- **Matrix build ≈ 72,896 ticks — this is soft-float, and that's the
  opportunity.** I verified the path: `ndsRendererAdapterBuildDObjFallbackMtx`
  → `syMatrixTraRotRpyRSca` (float) → `syMatrixF2LFixedW` (float→16.16). The
  concat/build runs in **software floating point on a CPU with no FPU** (~1,100
  ticks per 4×4). The N64 source itself quantizes to 16.16 fixed at the RSP
  boundary, so doing the **concat in fixed-point 20.12/16.16 is arguably *more*
  faithful, not less** — and ARM integer MUL is ~20× cheaper than the soft-float
  path. Nobody has tried converting the concat itself to fixed-point; the failed
  experiments moved matrices to the GX stack (transport lost) or cached them.
  This is the single underexplored fighter lever. Display-only math → drift is
  pixel-gate/profile-2 tolerable.
- **Lighting ≈ 79,648 ticks — permanent CPU, but hoist harder.** Precompute the
  part-space light direction **once per part** (not per vertex), leaving a bare
  normal·light → 128-step RGB table lookup per vertex. Some hoisting landed
  already; measure whether per-vertex still does a transform.

Plus: extend KRAW coverage to more fighter raw runs (it already carries 246
Mario + 234 Fox of the 626).

**Reality:** fighters are the hard owner (deep hierarchy, dynamic lighting).
Realistic outcome is 412K → ~300K, not 250K. **Do not burn the freeze on
chasing 250K here** — the matrix-fixed-point cut is worth ~50K and is the whole
ask for Jump C. If A is late, skip C entirely.

**Gate:** matrix bucket ≤~20K after fixed-point (from 73K); profile-2 light +
matrix oracle exact-or-pixel-approved; same-ROM A/B ≥80K only if bundling
matrix+lighting+KRAW; otherwise land the matrix cut alone as its own row.

---

## Part 3 — Schedule & the 7/16 decision

| When | Lane 1 (renderer) | Lane 2 (memory) | Exit |
|---|---|---|---|
| 7/15 | Jump A: freeze static stage + single-matrix HW T&L for the 66 raw + static no-Z | Jump B: static prewarm + fence | A first cut ≥300K saved; B fence zero post-GO |
| 7/16 AM | Jump A: no-Z depth-layer class (or defer to cold path) + water palette-swap | finish B | stage ≤300–350K; draw ≤~900K |
| **7/16 PM** | **Hard checkpoint (below)** | | **explicit target decision** |
| 7/16 PM | Jump C matrix-fixed-point *iff* A/B green early | | fighter matrix ≤20K |
| 7/17 | Freeze; re-profile whole frame; lock the pacing decision | | |
| 7/18 | Locked-30 soak + Full Regression + dated capture | | |
| 7/19 | RC fixes + honest red-row report | | |

**The 7/16 checkpoint decision (make it explicitly, in writing):**
- If active ≤ ~1,050K after A+B: **enable locked-30, label "playable preview
  @30fps," and stop chasing 60.** Locked-30 needs the scheduler design the
  roadmap already specifies (60Hz source logic, 29.9Hz present, no catch-up
  spiral) — that is itself ~half a day; account for it.
- If active is materially below that and 60 looks live: keep pushing, but do
  not claim P1 complete below real-time without Tyler's explicit approval.
- Either way: **do not silently redefine the target.** Present the number.

## Part 4 — What NOT to do (so the last two days aren't burned re-litigating)

- No more whole-owner FIFO/copy/patch/DMA packets (M2 −124K, per-joint −33K
  transport). Proven dead.
- No hardware lighting (102/413 miss). Proven dead.
- No GX hardware matrices for the fighter *hierarchy* (84 restores). Dead for
  fighters; fine for the shallow stage.
- No output-cache LRU or on-demand texture I/O on the critical path (M4). Dead.
- No per-phase water reconvert — palette-swap the immutable indices instead.
- No new harness modes, one-bit proof masks, or synthetic reruns (standing
  rule).

## Part 5 — One-line summary for Tyler

The recent big wins all came from handing work to fixed-function DS hardware or
branchless kernels; every failed attempt tried to cache-and-replay a 3D command
packet, whose copy/patch/DMA overhead is bigger than the CPU work it replaces.
So: do the **stage** next with single-matrix hardware transform (it's the
biggest owner and the only 3D one that fits the winning pattern), prewarm
textures + palette-swap the water off the critical path in parallel, and touch
fighters only via fixed-point matrix math — not another packet. Expect a
strong, honest **locked-30** for the demo; 60 isn't in the two days left.
