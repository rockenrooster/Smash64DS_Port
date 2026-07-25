# Plan.md — Stable 30 FPS via Native DS Rendering

> Goal: extract the optimization "wizardry" from three reference projects and
> raise Smash64DS_Port from the current ~13.5–15 FPS retail range to a stable
> 30 FPS, source-faithful in gameplay and ~90% in presentation.

---

## 1. Where We Are Now (honest baseline)

| Metric | Value | Target |
|---|---|---|
| Retail heavy-combat FPS | 13.5–15 | 30 |
| Loop ticks (wholesale) | ~2,240,000 | ≤ 1,118,000 (two VBlanks) |
| VBlanks per update | ~4 (spikes to 5+) | 2 |
| Stage owner (M3) P95 | ~489,000 | 150,000–250,000 |
| Fighter owner (M2) P95 | ~385,000 | 170,000–250,000 |
| `src/nds/nds_renderer.c` | **23,669 lines** | (reference is 1,285) |
| ITCM code in renderer | 2 functions of 32 KiB | hot draw path resident |
| Update ticks (UPD) | ~200,000–380,000 | lower |

**The dominant cost is the draw path** (stage + fighter + wallpaper + effects ≈
1.0–1.2M of the 2.24M loop). Every prior task (21R, 22R, 23R, 26, 27, 29, 34,
36…) has shaved ticks off a **fundamentally too-heavy approach**, and most
runtime cuts were *reverted* because they regressed. M2/M3 remain ~2× over
their milestone targets. We are in steep diminishing returns on the current
architecture.

---

## 2. What the Three References Teach

### `decomp/sm64-nds` — the thin GBI translator (1,285 lines)
A fan port of SM64 to DS that **is** playable. Its renderer is the direct
lesson in "do less CPU work":

- **One tight opcode loop.** `execute()` walks the N64 `Gfx*` display list and
  dispatches each opcode to a tiny handler (`g_vtx`, `g_tri1`, `g_mtx`, …). No
  owner validation, no AOT plans, no generation gates, no profiler.
- **Batched triangles.** `g_tri1`/`g_tri2` append to a 96-entry vertex batch;
  `draw_vertices()` flushes once per material change. One `glBegin`/`glEnd`
  covers many triangles sharing a polygon format.
- **DTCM_BSS for all hot state.** Colors, lights, vertex buffer, texture state,
  geometry mode — everything read in the hot loop lives in DTCM (single-cycle).
- **ITCM_CODE for hot functions.** `draw_vertices`, `g_vtx`, `g_tri1/2`,
  `g_mtx`, `execute` itself are in ITCM.
- **Async texture upload.** `glTexImage2DAsync` queues 128 uploads and
  `glTexSync()` DMA-s them in a batch; textures are cached by address in a
  1024-entry open-addressed map and reused, never re-decoded.
- **Geometry engine does the matrix math.** `glPushMatrix`/`glMultMatrix4x4`
  push work onto the DS GPU matrix stack; the CPU only converts the N64 fixed
  matrix once in `g_mtx`.

**Lesson for us:** our 23K-line owner/AOT layer is doing on the CPU what the
DS geometry engine is designed to do in hardware. We can cut enormous CPU work
by trusting the GPU matrix/pipeline the way sm64-nds does.

### `decomp/sm64ds-decomp` — Nintendo's native wizardry (94% matched)
The *official* SM64DS, built ground-up for DS. This is the real "wizardry":

- **Direct GPU register writes.** Geometry commands stream straight to
  `0x4000440` (the command register); the perspective matrix writes through
  the hardware divider ports `0x4000290`/`0x4000298`. No `gl*` wrapper layer,
  no display-list *interpretation* at all.
- **`Matrix4x3` — 4×3 fixed-point matrices.** Twelve elements, not sixteen.
  Saves 25% of matrix memory and multiply work; the W row is reconstructed by
  the geometry engine.
- **Hardware divider instead of software float.** `cstd::fdiv` / `cstd::ldiv`
  use the DS div unit (`0x4000290`) — the same trick our Task 9 soft-float
  chase was approximating, but Nintendo uses the *actual* hardware unit.
- **Native model format (NSBMD-class).** Models are stored in a DS-native
  binary the geometry engine consumes directly — no per-frame opcode decode.
- **`G3i` command builders** emit fixed command words to the geometry FIFO with
  zero interpretation overhead.

**Lesson for us:** the cheapest triangle is one the CPU barely touches. Where
BattleShip geometry is static (stage, fixed fighter topology), a pre-baked
DS-native command stream replayed with only the live camera/material words
patched in is what Nintendo actually ships.

### `decomp/sm64` — the N64 original
The source of truth for gameplay, geometry, and animation data. We read it to
confirm what a faithful *output* must look like, not as a performance model.

---

## 3. The Architectural Gap (root cause)

```
 sm64-nds:   Gfx* list ──▶ tiny opcode switch ──▶ gl* ──▶ GPU        (1.3K lines)
 Nintendo:   native model ──▶ G3i FIFO ──▶ geometry regs ──▶ GPU     (no decode)
 Smash64DS:  Gfx* list ──▶ owner/AOT/validate/rebuild ──▶ GX ──▶ GPU (23.7K lines)
```

Our renderer rebuilds and re-validates display state **every frame** through a
deep abstraction (profile owners, generation-gated plans, dense-ID packing,
semantic oracles, Task-29 census, benchmark sinks, hardware-compose labs). The
GPU could accept most of that work directly. The update path also pays heavy
main-RAM/float tax that the hardware divider and DTCM placement would remove.

---

## 4. Strategy — Three Phases, Lowest-Risk First

Each phase is independently shippable and verifier-gated. Gameplay stays
bit-exact (verifier-owned); presentation stays ~90% (screenshot + visual
approval gated). Nothing here changes hitboxes, physics, timing, rules, or
state flow.

### Phase 0 — Measure and localize the real hot spots (no behavior change)
**Before rewriting anything, instrument against the references.**

- Profile the current retail-equivalent ROM frame with a phase profiler already
  in-tree; produce an owner-ranked tick breakdown (update / stage / fighter /
  wallpaper / effects / GX-flush / present).
- Diff our hot draw symbols against sm64-nds's `draw_vertices` / `g_mtx` /
  `execute` to quantify how much CPU work we do that the reference does on GPU.
- Confirm the hardware-divider path (`0x4000290`) is unused today and identify
  the `__aeabi_*` float calls still in the update hot path.
- **Gate:** a dated tick-owner table checked into `artifacts/performance`. No
  code change ships in this phase.

### Phase 1 — Thin the draw path (high leverage, low risk)
Attack the ~1.0–1.2M-tick draw cost using sm64-nds's proven techniques, applied
through our **shared seam** (`src/nds/nds_renderer.c`, one-writer surface).

1. **Move hot draw state into DTCM.** Colors, lights, current texture params,
   vertex scratch, geometry-mode word — mirror sm64-nds's `DTCM_BSS` block.
   Single-cycle access replaces main-RAM streaming (the Task-10 1.50× tax lives
   exactly here).
2. **Place the draw hot loop in ITCM.** Only 2 of our renderer functions are in
   ITCM today (28K/32K used). Identify the true top-N draw symbols from Phase 0
   and promote them; keep the cold owner/profiler/diagnostic code in main RAM.
3. **Batch triangles by material.** Replace per-triangle `glBegin`/`glEnd` with
   the sm64-ns run-batch model: accumulate vertices until polygon-format or
   texture changes, then flush once. This is the single biggest sm64-nds win.
4. **Let the geometry engine own the matrix stack.** Stop CPU-composing matrices
   that the GPU can push/multiply. `g_mtx` in sm64-nds does one conversion and
   hands the matrix to `glPushMatrix`/`glMultMatrix4x4`; we currently re-derive
   per binding.
5. **Async/batched texture bind.** Adopt the address-keyed texture cache +
   deferred DMA-sync queue so a resident texture never round-trips.

- **Gate per cut:** synchronized eight-frame A/B, ticks + FPS + screenshot diff
  + reserve + conservation. KEEP only exact-or-approved-pixel cuts that lower
  draw P50/P95. Retail A/B is mandatory for any ITCM/DTCM/cache claim
  (melonDS cannot referee cache placement — Task-10 rule).

### Phase 2 — Native replay for static geometry (the Nintendo trick)
Where geometry and topology are immutable across a battle (stage, fixed fighter
skeletons), stop re-decoding the display list every frame.

- **Pre-bake the immutable GX command stream** for static DObjs/bindings once at
  scene load (the Task-36 Phase-B mechanism already proved 58% word
  conservation on a fixed 4,608-word buffer — generalize it under the seam).
- **Patch only live words per frame:** camera matrices (11 lanes), live
  materials, texture/color/alpha/UV selection. Everything else replays from the
  baked buffer with zero CPU decode.
- **Target the 4×3 matrix representation** where the geometry engine allows,
  cutting matrix traffic and multiply cost toward Nintendo's native footprint.
- Keep the fail-closed validator (a whole owner must validate before first GX
  mutation; unsupported state falls back as a unit) — this is already our
  contract and is sound.

- **Gate:** stage owner P95 must drop toward 150–250K; fighter toward 170–250K.
  Pixels exactly `0/49,152` for the replayed segments (Task-36 already proves
  this is achievable). One retail lifecycle confirms pacing.

### Phase 3 — Kill the update-path float tax
The UPD owner (~200–380K) still pays for software `__aeabi` float that Nintendo
never pays.

- Route the hot fixed-point divides through the **DS hardware divider**
  (`0x4000290`/`0x4000298`), exactly as `cstd::fdiv`/`cstd::ldiv` do in
  sm64ds-decomp — the principled version of what Task 9/16 were hand-emulating.
- Replace the remaining software float compares/converts in the update seam
  with the retained Task-9/16 exact leafs where they already proved neutral.
- Confirm DTCM placement of update-coroutine state (the Task-31 census found
  five concurrent 16 KiB coroutines; revisit placement now that draw is cheaper).

- **Gate:** UPD P50/P95 cut with exact state-hash identity (the Task-9/16
  six-field verifier). Retail A/B for the divider swap (hardware-unit behavior,
  melonDS-blind).

---

## 5. What This Plan Is NOT

- **Not a gameplay rewrite.** BattleShip source stays authoritative for rules,
  collision, physics, state, camera meaning, and flow. DS-native work lives in
  `src/nds`/`src/port`; ABI in `include`; imports in `src/import`.
- **Not pixel-exact presentation.** ~90% likeness under the fidelity budget;
  each rendering approximation gets one measured attempt, owner visual approval,
  and an `artifacts/visibility` screenshot before KEEP.
- **Not speculative abstraction.** We add no new selectors, caches, profiler
  modes, or tooling without a measured tick win. The *existing* 23K-line layer
  is the thing to *reduce*, not extend.
- **Not emulator-only promotion.** ITCM/DTCM/divider/cache claims require retail
  proof (Task-10 calibration: melonDS has no dcache model). CPU-work-removal
  claims may KEEP on typed A/B behind their flag until the next device checkpoint.

---

## 6. Rules of Engagement (from AGENTS.md)

- `decomp/*` is read-only. Inspect BattleShip before behavior changes; inspect
  `sm64-nds`/`sm64ds-decomp` before backend architecture changes.
- One-writer on `src/nds/nds_renderer.c` throughout.
- Fix root causes at the shared seam — no symptom-hiding offsets, frame checks,
  duplicated state, or asset-specific hacks.
- Preserve user dirty-tree work; prefer focused edits over whole-file replacement.
- Each phase: smallest focused checker while editing, one widest verifier
  (Boundary for battle-only, Current if shared startup changes) at checkpoint,
  then `New-Smash64DSSnapshot.ps1 -Mode Lean` as the final action.
- Performance evidence: ticks, FPS, dated screenshot, screenshot analysis, plus
  the 2/3/4/5+ VBlank histogram and max interval on device (never min FPS).

---

## 7. Success Criteria

1. Retail one-minute battle runs at a stable **2 VBlanks per presentation**
   (2/3/4/5+ histogram dominated by the 2-bucket, zero pacing slips) — i.e. a
   real ~30 FPS.
2. Stage owner ≤ 250K P95; fighter owner ≤ 250K P95; loop ≤ 1,118,000 ticks.
3. Net reserve ≥ 128 KiB; exact 4,084 updates / 2,042 presents; one clean
   teardown; synchronized top-screen delta `0/49,152` for faithful segments.
4. All P1 acceptance rows green; owner visual approval on presentation.
5. `smash64ds.nds` + `smash64ds-battle-playable-hwtri.nds` are the only
  published ROMs, verifier-covered.

---

## 8. Sequencing Checklist

- [ ] Phase 0: dated owner-ranked tick table + hardware-divider/float audit.
- [ ] Phase 1.1: DTCM hot-state block (retail A/B).
- [ ] Phase 1.2: ITCM draw hot functions (retail A/B).
- [ ] Phase 1.3: material-batched triangle flush (synchronized A/B).
- [ ] Phase 1.4: GPU-owned matrix stack (synchronized A/B).
- [ ] Phase 1.5: address-keyed async texture bind (synchronized A/B).
- [ ] Phase 2.1: generalized immutable GX replay buffer.
- [ ] Phase 2.2: live-word-only per-frame patch.
- [ ] Phase 2.3: 4×3 matrix representation where eligible.
- [ ] Phase 3.1: hardware-divider route for hot divides (retail A/B + state hash).
- [ ] Phase 3.2: residual float leafs in update seam.
- [ ] Final: full lifecycle verifier, retail histogram, snapshot.
