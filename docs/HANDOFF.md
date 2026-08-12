# Handoff

Updated: 2026-08-12. **The gate is banked at 1,213,440** (`build-c123-gate`,
slice 45 in). c122 re-banked it at 1,225,280 after slice 43 was WITHDRAWN for the
fighter blink — measured, not derived from the 1,210,560 slice-44 bank, which had
the GX joint compose on; the withdrawal cost P50 +4,864 / P95 +14,720 against
slice 43's claimed −13,632. Slice 45 then took −11,840.
**Every 128-frame figure in the archive is unusable** — use `-Samples 1600`.

## The two baselines — label every figure with its arm AND its coverage

Both arms run the **same 60-second match** (coverage 86.7%), windows ending 43
frames past the buzzer. Slips 0 in every row.

| arm | role | `WORK-H` P50 | P95 | over gate |
|---|---|---:|---:|---:|
| **both-CPU** | **THE GATE** | **938,944** | **1,213,440** | banked c123, slice 45 in |
| **Boundary** mode 163 | shipped configuration | 920,192 | 1,113,408 | re-banked c116, slice 43 IN — stale |

**Gate 1,213,440. Gap 93,060.** Top-80 `WORK-H` frames vs ranks 400–1200, on the
c123 gate rows: `GCRA` +356,544 on 76/80, of which **`SINT` +145,184 on 51/80**
and **`SHDT` +88,416 on 49/80** (4,736 → 93,152, a 19.7x step); `MISC` +19,776,
`SPHD` +10,816. **`FTR` −128 and `STG` +192 at 0/80 presence** — the renderer is
not the tail whatever its size. Capping each lane at its own median (a perfect
fix, so an upper bound): `SINT` −72,448, `SHDT` −37,760, `MISC` −36,352,
`GCRA` remainder −13,376. **No single lane clears the gate; `SINT`+`SHDT` is
−143,488 and does.**

**Two lane-sizing traps, both hit this cycle.** Medians do not add: subtracting
nested buckets' medians from `GCRA`'s median invented a 110,336 "unbucketed"
lane that is really +9,472 on 9/80 when computed per row. And `OTHR` looks like
a −116,800 ceiling but `OTHR = ALL − named` where `named` excludes `WAIT`, so it
CONTAINS the idle wait — `OTHR − WAIT` is flat at 17,600..21,120 and +192 on the
top 80. Only `WORK-H` = `(ALL − WAIT) − HUD` is spendable.

**Profile with `NDS_TICK_HUD_DRAW=0` or you profile the instrument** — the HUD
costs ~345,024 tk twice a second on exactly the frames P95 is decided on
(`nds_platform.c:68`), so `--split-top-frames` otherwise selects HUD-refresh
frames (c122 put `ndsPlatformRenderDebugHud` at 17.5% of premium on 80/80, ~20%
overlap with the real top-80). Neither harness sets it; the GATE keeps `DRAW=1`,
which is how every bank back to slice 44 was measured. The soak's long match is
`NDS_R2_SOAK_MATCH_MINUTES`; `probe-match-window.ps1` reads the timer from the
guest, so a window cannot claim coverage it lacked. `Makefile:382` forbids
reporting a both-CPU P95 as Boundary's; **re-pin `EXPECTED_CENSUS_SHA256` when
coverage changes.** **Route to ATTRIBUTE, re-bank to BANK** — slice 31 read
−7,104 routed, +576 built. **Collision paid and is BANKED** — slices 35/36/37
memoise three `line_id` scans over static geometry, −10,752; their owners are
FLAT (no PC over 3.6%), so the lever was calls, not instructions — the same shape
slice 45 exploited. **Band A (a symbol on ≥40 of 80) = 154,496 tk over 213
symbols**, more than the gap. SIZE IS NOT PERMISSION: float in
`gmcollision`/`mp*`/`ftMain*`/`ftComputer` is FROZEN by the Task 9 hash — exact
memoization/hoisting/reuse/call deletion only.

## What is dead, so nobody re-derives it

- **Effect DObj submits** — Boundary-only: 99.3% of the Boundary excursion, ~12.1%
  of the gate arm's. **Projectiles** · **Particles** · **texture thrash** ·
  **`Find`** · **`Material`** · force-load seam. **`FTR` as the P95
  discriminator** — flat, 0/80 on the c123 tail (NOT "FTR is exhausted"; c110–116
  took 24.3% off it). **The AOT animation bake at 20 B/record** (slice 32): SIZE
  dead, 10,304 B per animation vs 2,310, ×85 = +679,490 B.
- **Animation playback ARITHMETIC** (slices 34, 41): already Requirement 4's
  fixed point at 1.67–1.69 cyc/insn; idle-joint skip (33), lazy track table (31),
  AObj walk and track dispatch all under the floor. **Slice 41 spent the last
  lever**: 30 Hz poses cost P95 **+7,040** *and* the arms diverged into different
  matches (damage 130/51 vs 33/65) — a route A/B cannot price a gameplay change.
  **Slice 39's table is VOID** (threshold ON the quantum). **Don't blanket-convert
  `ndsBaseGcPlayMObjMatAnim`** — 5 tracks pack 0xRRGGBBAA in f32. STRUCTURAL cuts
  in this lane are NOT closed.
- **The 20.12 kernels' ARITHMETIC (slice 42)** — sub-floor and non-additive.
  **The local-matrix memo is dead twice** — E8's +16,301 key cost, payload 302
  tk/call since `MTX_DIRECT`. **The flower rigid-mask prices at +3,200, wrong
  sign**. **The token→asset_id MEMO is dead** (Task 74): bss tables lose to a
  resident chain of link-time immediates — slice 45 deleted the CALLS instead.
- **`ndsRelocFinalizeLoadedFile` as the gate** (R2-06 E8's headline claim):
  refuted by E8's own `NDS_R2_RELOC_FIXUP_TIMING` split — one setup call is 88%
  of the total, recurring finalize is ~4,604 tk/call. Neither repair it proposed
  is worth building.
- **Growing the anim-cache arena.** GATE arm: Rejects 0, Overflows 0, 262,144
  reserved / 257,200 used, 351 of 383 hits, heap low-water 70,776. The
  zero-reject invariant already holds; there is nothing to buy.

## RAM: both budgets are near their floor — price a change before writing it

- **Static/boot.** `check-boot-headroom.ps1 -Build <dir>` after every lab build.
  Highest `fake_heap_start` proven to boot **`0x02294804`**, lowest proven to
  fail **`0x02294b24`**. **Text counts as much as bss**; a failing arm reads as a
  hung emulator. **`gSYTaskmanGeneralHeap`** free-min is **70,776** on the c123
  gate (−169,152 B static, arena at `0x150000`), i.e. 38,008 above the 32,768
  floor. Coupled: freeing `.bss` enlarges it.

## FTR: −93,612 landed (c116); it is now 0/80 presence and NOT a P95 lever

**Banked `FTR` mean 291,896** vs a 385,508 baseline (−24.3%), on the SHIPPED
strips. Keep these three warnings; stop re-deriving the rest.

- **DS-native AOT geometry ships** (`NDS_TASK56_FIGHTER_PRIMITIVES ?= 2`): 1,878
  `GL_TRIANGLES` corners → 1,014 strip corners, P50 −10,112 / P95 −14,144. **It
  SHIPPED BROKEN** — an undirected edge made 35.6% of the fighter BACKFACING and
  `BEGIN_VTXS` on group TYPE change welded adjacent strips into one vertex list.
  Boundary passed on the broken build: **a passing verifier is not visual
  verification. Hand the owner a ROM.**
- **The emit stalls per VERTEX, not per word** — 40.5 cycles a corner, ~28 of it
  the GX write. A baked/DMA'd GX stream and `VTX_10` are **refuted**: they trade
  words, not vertices. Cold bytes in an *entered* body cost **+14,963**.
- **Compiling the frame-summary counters out is refuted** (FTR −7,378): it breaks
  the gate — the loop harness asserts batch/texture accounting off those globals.
  **Tick factor 0.4993 tk/cyc** from `ALL` vs total cycles, never from the FTR sum.

## Landed slices and the lanes they leave

**SLICE 43 WITHDRAWN 2026-08-11.** Owner retest still blinked after the
parent-slot union repair, so the earlier "fixed" claim was false. All targets
force `NDS_R2_FIGHTER_GX_COMPOSE=0`; do not re-enable without owner proof. Lead
at `nds_platform.c:3197`: the matrix stack leaks ~3 pushes/frame, wrapping mod 32.
**SLICE 45 KEPT — 1,225,280 → 1,213,440.** Same-binary A/B
(`gNdsR2RelocAliasRoute`): `ndsRelocRemoveFighterAObj16StatusAliases` resolved
`ndsRelocAssetIDForToken` for EVERY status node when `addr == data` rejects
almost all in one compare. Resolves 16,002 → 1,143 of 16,067 visits, **P95
−12,160, P50 +256**. Exact — the resolver is a pure function of a link-time
ADDRESS. Evidence `artifacts/performance/2026-08-11_c122-rebank/SLICE45.md`.

**SLICE 44 KEPT — 1,244,480 → 1,210,560** (superseded above).
`NDS_R2_STAGE_VALIDATE_STRIDE=8` strides the stage's 42-binding revalidation,
**−17,088 / −35,904**. Round-robin, NOT "sweep every 8th frame" — that shape
makes 12.5% of frames expensive and P95 lands on one. Demotion is one-way.

**The fighter LOCAL matrix build is NOT a P95 lane — refuted c122.** Real size
(~24,314 tk/frame, 80/80) and memory-bound, but `FTR` is 0/80 on the tail: it is
a P50 lane. Its only remaining shape is a local-matrix memo, **DO-NOT-RETRY,
killed twice**; the Task 91 comment at `reloc_backend_renderer_dl.c:1790` argues
for it anyway and is not an invitation.

**`SINT` is the fighter INTERRUPT proc with `SCPU` nested inside it, not an
animation bucket** — reading it as one mis-attributed an A/B in c119. It is the
**biggest remaining lane: +145,184 on 51/80, ceiling −72,448.** Its old split
(`ftMainPlayAnim` +60,559, `ftComputerProcessAll` +24,386 = map collision, not
AI) predates slices 44/45 — re-split before designing. **Zero-copy force-load is
closed:** `ftmain.c:4623` **discards the return value** and sets
`fp->figatree = fp->figatree_heap` itself.

**Fighter animation is fixed point — Requirement 4 shipped** (slice 25), P50
−23,360 / P95 −37,504; what is LEFT is ~70,000 cyc/frame of memory stall. All
`gNdsR2AnimCutRoute` bits ship ON — no default-off win hides there.

**Do not bring a micro-fix** — R2-06 E11: a load-frame-only ~8,000 cannot be
banked, relinking moves the tail more than the saving. Clear ~16,000 in one
change, or **use the `.data` route on ONE binary** (only if the change cannot
alter gameplay state). **Every change needs an engagement counter on BOTH
sides** — c110 read FTR −13,587 off a skip it could not prove fired.

**Do not re-derive these.** The Makefile's `?= 0` defaults are not the shipped
config (41 overridden). `.text.hot` is closed both directions
(`linker/nds_hot_text.ld:179-201`), **3.30 cyc/insn, worse than `.main`**;
census C/D rank cost, never predict placement. **Latent cliff, unowned:**
`sNdsAObjEvent32NormalizedCount` reads **973 of 1,024** after a minute; overflow
silently **skips the animation attach**. **The load-frame exclusion is REFUTED**
(`SRC > 2x median` is circular for SRC, swings the gap 3.08x). **Boundary for
all of it**; a change altering a visible pixel needs the owner (`BUGS.md`).

## Measurement rules that change your FIRST action — board owns the rest

- **The sampler is bit-deterministic — never repeat a run.** Same ROM twice gives
  byte-identical buckets (slice 45's control reproduced the c122 bank to 2,176).
  So the 14,080 cross-build figure is **placement, not noise**; anything under it
  needs the `.data` route. Use `-Samples 1600 -RingDump`. A duplicate frame LABEL
  at a ring-stop seam is warned, not failed (slice 41); IDENTICAL payload is a
  stale read and always fatal, as is a duplicate away from a seam.
- **Judge on `WORK-H`**; buckets locate, they never decide (floor ≥8,544).
  **`ALL` is VBlank-quantized** — it hid a +52,928 once and the strips entirely.
  **1.85 cycles of `FTR` mean per byte of added ARM text** — beat your footprint.
- **A census row in tk/fr sizes a P50 lever, NOT a P95 one.** Slices 36/37 had
  equal mean self cost and P95 wins **2.45x apart**; slice 44 is the same law
  sign-flipped (mean −10,838, P95 −35,904). **Presence is the tell**: slice 45
  moved P95 −12,160 and P50 +256 off 71/80 presence.
- **A route A/B is only valid for a change that cannot alter gameplay state**
  (slice 41). It deletes the ±8,544 floor by holding the binary fixed, but still
  assumes both arms walk the same trajectory. Read an end-of-match gameplay
  counter from the SAME run first.
- **`--pc-detail` BEFORE designing the fix — no build, and it routinely names a
  different lever than the source reads like.** Slice 44's guard looked like a
  compare; four cold `ldr`s were 39% of it. Same call answered c108/c110/c116.
  **Resolve line numbers against the build's `NDS_TASK10_GIT_SHORT`.**
- **A lane's SIZE is not its CEILING, and medians do not add.** Cap the bucket at
  its median per row, re-take the 80th of 1,600 — that is the most the lane can
  ever pay. Subtracting medians instead invented a 110,336 lane that was 9,472.

## Restart surface — parked items live on the board's **Parked** list

`AGENTS.md` owns the start-of-cycle commands. `docs/P1_EXECUTION_BOARD.md` is the
only dynamic queue (history in `docs/optimization/archive/`);
`Smash64DS_Runtime2_SwitchPlan.md` is the charter; `docs/BUGS.md` carries the
owner's verdicts — preserve their wording. A clean checkout builds through
`build.ps1`, not bare `make`: four of six `.inc` are gitignored and
**`build.ps1`'s generator is not run by `make`, so it can rot**. `make p1-tick`
builds the measuring ROM, `make p1` the published pair; bare `make` builds the P2
ROM P1 does not ship. Never pass `-j`, never override `MAKEFLAGS`, one build at a
time, never build a published target name for lab work. Preserve mode 163,
renderer mode 9, mip 0, static textures, source countdown, Dream Land water frame
0, Task 16 `1/1/1`. Never edit `decomp/`. Run `New-Smash64DSSnapshot.ps1` last.
