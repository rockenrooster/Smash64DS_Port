# Handoff

Updated: 2026-08-12. **THE GATE ARM WAS MISLABELLED — read
`artifacts/performance/2026-08-12_c126-armcheck/ARM_MISLABEL.md` FIRST.**
`build-c124-slice48`, the ROM behind the banked “canonical both-CPU” 1,087,296, was
built **`NDS_R2_BOTH_CPU 0`** — a Boundary-arm figure. On the arm R2-07's gate names
(Mario CPU vs Fox CPU, `BOTH_CPU 1`) HEAD measures **1,207,616 — +87,236 OVER**.
**Boundary PASSES (~1,087,600); the R2-07 stress gate FAILS.** Every lane ceiling in
`EXHAUSTION.md` was computed on `BOTH_CPU 0` rows and must be redone. **Owner's order
is bugs first, then P95** — so this is context, not the next task.

## R2-07 `BUGS.md`: rows 2 and 3 are FIXED AND MEASURED, awaiting the owner's playtest

Contracts + evidence: `artifacts/bugs/2026-08-12_r2-07-cluster/CONTRACT.md`. **The old
"all three rows converge on ONE texture-residency capability" framing was WRONG for
ALL THREE** — do not reinstate any part of it. The first two "FIXED" acceptances were
ENGAGEMENT proofs and were withdrawn; **both rows now carry screen-space pixel proof
stacked against a matched control**, so do not re-litigate them — playtest them.
Candidates: `build-c129-foxfire` (Boundary arm, Fox pixels) and `build-c130-fire-bothcpu`
(same source on `BOTH_CPU 1`, flame pixels + gate). Neither is marked FIXED — owner's call.

**Perf:** `…/GATE.md` — `WORK-H` P95 **1,217,472** vs the both-CPU bank 1,207,616, **+9,856,
under this arm's own 9,664 repeat spread**; the mechanism (18 quads, ~12 frames) is ~10x small.

- **Row 3 (Fox gun) — the overlay drew at 0.036 PX**, in the right place, at the right
  depth, 44/44 corners in the viewport. The submit hand-loaded its composed MVP and so
  skipped BOTH halves of `ndsRendererLoadHardwareRawComposedMatrix`: the world-unit pair
  (vertices go in `x16`, the matrix's complete homogeneous row 3 must be
  `>> NDS_RENDERER_HW_WORLD_UNIT_SHIFT`) and the identity GL_PROJECTION, which a
  `NDS_R2_FIGHTER_HW_MTX := 1` target needs because the fighter leaves a real one loaded.
  The source DL also gave **16x32, not 32x16** (CI4 makes both 256 B, so no byte count
  could catch it), TEXEL0 x SHADE, and CLAMP. **A magenta bar beside Fox is
  `NDS_R2_FOX_BLASTER_QUAD`'s debug quad, not the gun.**
- **Row 2 (fire burn) — FlameLR's quad cell was not in the atlas.** Script `0x12` ->
  texture 12, PACKED but sitting in `quads.excluded`, so `ndsParticleQuadFrameFor`
  returned NULL and `battleship_lbparticle.c:3698` drew NOTHING — the burn was exactly
  half-drawn, FlameRandom only. **`QUAD_MEASURED_LIVE` is regraded from a soak's own use
  mask, so after restoring a dead effect, re-grade the atlas before trusting the soak.**
  **The build arm is part of the trigger**: at `BOTH_CPU 0` Mario never burns, so the
  flame probe now reads `nds_build_config.h` and refuses that arm instead of timing out.
- **Row 1 (Whispy face) — the blink animation lasts ONE frame.** `map_gobj[0]->anim_frame`
  hits 1.0 for one sample and is 0 the next; everything around it measured GREEN, and
  **there IS no blink texture** — that entry of `dGRPupupuWhispyEyesAnims` carries a NULL
  material anim (`grpupupu.c:76`) and the six `dGRPupupuWhispyEyesTextures` are the WIND
  cycle on `map_gobj[3]`. Only question left: why does the joint script resolve to one
  frame? Probes: `probe-whispy-eye-texture.ps1`, `probe-whispy-blink-script.ps1`.

**R2-08 cannot be finished by an agent**: SwitchPlan `:391` needs the owner's recorded retail play test, `:385` their visual approval.

## The two arms — same 60 s match; ONLY `BOTH_CPU 1` is R2-07's gate

| arm | role | `WORK-H` P50 | P95 |
|---|---|---:|---:|
| Boundary arm (`BOTH_CPU 0`) | shipped config, **PASSES** | 899,136 | 1,087,616 |
| **both-CPU (`BOTH_CPU 1`)** | **R2-07 GATE, FAILS** | 938,368 | **1,207,616** |

**RE-BANKED ON `BOTH_CPU 1` — `…/2026-08-12_c130-fire-gate/LANES_BOTHCPU.md`** (no build;
`analyze-tick-hud-excursion.ps1 -Ceilings` emits both tables now). `SRC` owns **84.4%** of
the 395,863 excursion and its ceiling **207,104 is 2.07x the 100,100 gap — the gate is
reachable inside `SRC` alone**. `SITR` 83,712 is the largest leaf lane, `SHDT` 51,584 the
sharpest presence (13.17x on a 4,416 median). **`SCPU` +252%, `SPRM` +274%, `SPHD` +158%
vs the Boundary table — the wrong arm ran ONE CPU; `FTR` is 8,512, not 0.** `MISC`/`AUD`
shrank and c125's ceilings are dead. **NEXT CUT IS SPECIFIED — `…/SITR_NEXT_CUT.md`.**

**THE BIGGEST LEVER IS PLACEMENT.** Memory stall is **1,236,685,107 cycles,
33.8% of the match** ≈ 386,000 tk/frame — an order of magnitude past `SINT` or
`SHDT`, and why slice 48's behaviourally-identical pair differed 94,976.

**SLICE 49 (reclaim dead ITCM) is REFUTED without a build — do not re-open it until
the Task 37 port group is understood.** `.itcm` is NOT full (30 of 82 residents
never execute, 2,594 B idle), but the census's 87,033,153 stall cycles "in reach"
assumes admitting ~3,118 B of mostly PORT functions, and `NDS_TASK37_ITCM_PORT` is
0 because *"the owner confirmed the enabled lab build misbehaves"* — CORRECTNESS.
**Eviction alone pays nothing**; what is left is `__aeabi_lmul` via the proven
library-member list (86 B ≈ 1,005 tk/frame). ITCM detail in `EXHAUSTION.md`, ceilings dead.

**Lane-sizing traps, now encoded in `-Ceilings`:** medians do not add (that invented a
110,336 lane worth +9,472/row in c122); `OTHR` CONTAINS `WAIT`. Only `WORK-H` is spendable.

**Profile with `NDS_TICK_HUD_DRAW=0` or you profile the instrument** — the HUD
costs ~345,024 tk twice a second on exactly the frames P95 is decided on; the GATE
keeps `DRAW=1`. Soak length is `NDS_R2_SOAK_MATCH_MINUTES`. `Makefile:382` forbids
reporting a both-CPU P95 as Boundary's. **Route to ATTRIBUTE, re-bank to BANK.**
**Collision paid and is BANKED** (slices 35/36/37, −10,752). SIZE IS NOT
PERMISSION: float in `gmcollision`/`mp*`/`ftMain*`/`ftComputer` is FROZEN — exact
memoization/hoisting/reuse/deletion only.

## What is dead, so nobody re-derives it

- **`FTR` — −93,612 landed (c116); its "0/80, NOT a P95 lever" verdict is BOUNDARY-arm
  and the gate arm prices it 8,512.** DS-native AOT geometry ships
  (`NDS_TASK56_FIGHTER_PRIMITIVES ?= 2`) and **SHIPPED BROKEN** — 35.6% of the fighter
  backfacing with Boundary green: **a passing verifier is not visual verification.**
- **Effect DObj submits** — Boundary-only. **Projectiles** · **texture thrash** ·
  **`Find`** · **`Material`** · force-load seam. **`MISC` is PARTICLES and particles
  are FLAT.** **The AOT animation bake** (slice 32): SIZE dead.
- **Animation playback ARITHMETIC** (slices 34, 41): idle-joint skip (33), lazy track
  table (31), AObj walk and dispatch all under the floor. **Slice 41 spent the last
  lever**: 30 Hz poses cost **+7,040** *and* diverged the match (damage 130/51 vs
  33/65). **Don't blanket-convert `ndsBaseGcPlayMObjMatAnim`** — 5 tracks pack
  0xRRGGBBAA in f32. STRUCTURAL cuts are NOT closed.
- **The 20.12 kernels' ARITHMETIC (slice 42)** — sub-floor and non-additive. **The
  local-matrix memo is dead twice.** **The flower rigid-mask prices +3,200, wrong
  sign.** **The token→asset_id MEMO is dead** (Task 74). **Six more lanes closed by
  MEASUREMENT** — numbers in `…/2026-08-1{1_c122,2_c123}-rebank/SLICE4{5,6,8}.md`:
  `ndsRelocFinalizeLoadedFile` as the gate; anim-cache arena growth (Rejects 0); the
  `OTHR` ceiling; **BGM buffer/packet sizing**; **every memo is healthy**.

## RAM — price a change before writing it

`check-boot-headroom.ps1 -Build <dir>` after every lab build. Highest
`fake_heap_start` proven to boot **`0x02294804`**, lowest to fail **`0x02294b24`**.
**Text counts as much as bss**; a failing arm reads as a hung emulator.
`gSYTaskmanGeneralHeap` free-min **72,188** (floor 32,768).

## Landed slices and the lanes they leave

**SLICE 43 WITHDRAWN 2026-08-11.** All targets force `NDS_R2_FIGHTER_GX_COMPOSE=0`;
do not re-enable without owner proof. Lead at `nds_platform.c:3197`: the matrix
stack leaks ~3 pushes/frame, wrapping mod 32. That line's `|| NDS_TICK_HUD` is
pinned by `check-gbi-decode-fixtures.ps1:2247`.
**SLICE 46 KEPT — 1,213,440 → 1,196,224** (`…/SLICE46.md`). Warm preload covered only
57 of the 87 used ids; replaced with the measured 87, 4 per scene update: **misses
32 → 2**, arena 257,200 → 192,240 (it SHRINKS).

**SLICE 48 KEPT — read its SIZE, not its bank (`…/SLICE48.md`).** The FAT lane is
**BGM**. **`AUD` at 0.2% does NOT clear BGM** — a bucket brackets only its own thread
and the worker ran ABOVE main. Shipped: created at `MAIN_THREAD_PRIO + 1`, switched to
`- 1` once playing (`.data` pokeable). **Deprioritizing during the MATCH was REFUTED**
— same-binary A/B, +8,064 wrong way; creating low is −13,952..−17,792.

**SLICE 45 KEPT — 1,225,280 → 1,213,440.** `ndsRelocRemoveFighterAObj16StatusAliases`
resolved `ndsRelocAssetIDForToken` for EVERY status node when `addr == data` rejects
almost all in one compare. Resolves 16,002 → 1,143, **−12,160**.
**The fighter LOCAL matrix build is NOT a P95 lane — refuted c122.** Real size but
`FTR` is 0/80 on the tail: a P50 lane. Only shape left is a local-matrix memo,
**DO-NOT-RETRY, killed twice**. **`SINT` is the fighter INTERRUPT proc with `SCPU`
nested, not an animation bucket** — reading it as one mis-attributed an A/B in c119.
**Zero-copy force-load is closed:** `ftmain.c:4623` DISCARDS the return value.

**SLICE 47 REVERTED — the `SHDT` reach bound is DEAD, geometrically.**
**`ReachTests 2,373  WouldSkip 0`** — it never rejects, and tightening it needs the
joint position that IS the transform being skipped. Carry: `gmCollisionTestRectangle`
also serves item/weapon/ground — **never attribute a shared leaf to one caller**.

**The collision transform chain is honest work, not redundancy (c123).** Latches
clear once per fighter per frame (`ftmain.c:1847`); hit detection rebuilds lazily and
shares ancestors. 13–18x is a **call-count** ratio — the lever is touching fewer parts.

**Do not bring a micro-fix** — R2-06 E11: a load-frame-only ~8,000 cannot be banked.
Clear ~16,000 in one change, or **use the `.data` route on ONE binary** (only if the
change cannot alter gameplay state). **Every change needs an engagement counter on
BOTH sides**; slices 45, 46 and 48 were all found by READING counters the code
already kept, on the gate arm, for the first time. The Makefile's `?= 0` defaults are
not the shipped config (41 overridden); `.text.hot` is closed both directions.
**Latent cliff, unowned:** `sNdsAObjEvent32NormalizedCount` reads **973 of 1,024**
after a minute and overflow silently **skips the animation attach**. **Boundary for
all of it**; a visible-pixel change needs the owner (`BUGS.md`).

## Measurement rules that change your FIRST action — board owns the rest

- **The sampler is bit-deterministic — never repeat a run.** Same ROM twice gives
  byte-identical buckets, so ANY cross-build delta is placement. `-Samples 1600
  -RingDump`. A duplicate frame LABEL at a ring seam is warned; IDENTICAL payload is
  a stale read and always fatal, as is one away from a seam.
- **Judge on `WORK-H`**; buckets locate, they never decide (floor ≥8,544). **`ALL`
  is VBlank-quantized** — it hid a +52,928 once. **1.85 cycles of `FTR` mean per
  byte of added ARM text.** **A bucket only sees its OWN thread** (slice 48).
- **A census row in tk/fr sizes a P50 lever, NOT a P95 one.** Slices 36/37 had
  equal mean self cost and P95 wins **2.45x apart**. **Presence is the tell.**
- **A route A/B is valid only for a change that cannot alter gameplay state
  (slice 41) AND only if the poke lands before the value is READ.** `-SetGlobals`
  fires at the first frame-complete marker; record what was actually applied or
  the control is the candidate relabelled (slice 48 got 1,102,208 on both arms).
- **`--pc-detail` BEFORE designing — no build, and it routinely names a different
  lever than the source reads like.** Slice 44's guard looked like a compare but
  four cold `ldr`s were 39%; 85.5% of `ndsAObjEvent32NormalizeScript` is two
  pointer scans, not its logic.
- **An arm that cannot produce the event reads 0 either way** — check the control
  differs first. **And a zero one level DOWNSTREAM of a rejected request reads exactly
  like a dead lane**: row 2's four zeroes were correct readings of a request killed by
  a NULL script pointer upstream, and each was read as "nothing asks for this".

## Restart surface — parked items live on the board's **Parked** list

**DO NOT PUSH (found 2026-08-12; owner said ignore for now).** The owner-given-name
scan is RED: 16 tracked `decomp/` Rust build artifacts embed the build machine's
user directory. Root cause is a doc/reality mismatch — `AGENTS.md` says `/decomp/`
is gitignored and it is NOT (`git ls-files decomp` reads 26,276). Local commits are
safe; only the push is blocked. Owner's call: untrack `decomp/` or scrub those 16.

`AGENTS.md` owns the start-of-cycle commands; `docs/P1_EXECUTION_BOARD.md` is the
only dynamic queue; `docs/BUGS.md` carries the owner's verdicts — preserve their
wording. A clean checkout builds through `build.ps1`, not bare `make`: four of six
`.inc` are gitignored and **`build.ps1`'s generator is not run by `make`**. `make
p1-tick` builds the measuring ROM, `make p1` the published pair. Never pass `-j`,
never override `MAKEFLAGS`, one build at a time, never build a published target name
for lab work. Preserve mode 163, renderer mode 9, mip 0, static textures, source
countdown, Dream Land water frame 0, Task 16 `1/1/1`. Never edit `decomp/`. Run
`New-Smash64DSSnapshot.ps1` last.
