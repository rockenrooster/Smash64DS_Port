# Handoff

Updated: 2026-08-12. **The gate is banked at 1,196,224** (`build-c123-warm`,
slices 45+46 in, Boundary GREEN). c122 re-banked at 1,225,280 after slice 43 was
WITHDRAWN for the fighter blink — measured, not derived from the 1,210,560
slice-44 bank. Slice 45 took −11,840, slice 46 −17,216. **Gap 76,804.**
**Every 128-frame figure in the archive is unusable** — use `-Samples 1600`.

## The two baselines — label every figure with its arm AND its coverage

Both arms run the **same 60-second match** (coverage 86.7%), windows ending 43
frames past the buzzer. Slips 0 in every row.

| arm | role | `WORK-H` P50 | P95 | over gate |
|---|---|---:|---:|---:|
| **both-CPU** | **THE GATE** | **938,752** | **1,196,224** | banked c123-warm, slices 45+46 |
| **Boundary** mode 163 | shipped configuration | 920,192 | 1,113,408 | re-banked c116, slice 43 IN — stale |

**Gate 1,196,224. Gap 76,804.** Top-80 `WORK-H` frames vs ranks 400–1200:
**`SHDT` +102,816 on 57/80** (4,544 → 107,360, a 23.6x step) and **`SINT`
+108,384 on 47/80**; `SPHD` +17,024 on 28/80, `MISC` +16,864. **`FTR` −736 and
`STG` +608 at 0/80** — the renderer is not the tail whatever its size. Ceilings
(cap the lane at its median per row, re-take the 80th): `SINT` −56,512, `SHDT`
−38,912, `MISC` −29,248. **No single lane clears it; `SINT`+`SHDT` does.** Slice
46 moved 36,800 out of `SINT` and `SHDT` took the lead — ownership really moves,
re-attribute after every KEEP. **`SHDT` is smaller than its bucket**:
`--pc-detail` reads the chain walk at ~18 calls a hot frame but 0.03% of the
window, so deleting hurtbox transforms reaches ~30% of it — a reach bound is
worth ~15–20K, not the ceiling.

**Two lane-sizing traps, both hit this cycle.** Medians do not add (subtracting
nested medians from `GCRA`'s invented a 110,336 lane that is +9,472 per row), and
`OTHR` CONTAINS `WAIT`, so its apparent −116,800 ceiling is idle time —
`OTHR − WAIT` is +192 on the top 80. Only `WORK-H` is spendable.

**Profile with `NDS_TICK_HUD_DRAW=0` or you profile the instrument** — the HUD
costs ~345,024 tk twice a second on exactly the frames P95 is decided on
(`nds_platform.c:68`), so `--split-top-frames` otherwise selects HUD-refresh
frames (c122 put `ndsPlatformRenderDebugHud` at 17.5% of premium on 80/80, ~20%
overlap with the real top-80). Neither harness sets it; the GATE keeps `DRAW=1`,
which is how every bank back to slice 44 was measured. The soak's long match is
`NDS_R2_SOAK_MATCH_MINUTES`; `probe-match-window.ps1` reads the timer from the
guest. `Makefile:382` forbids reporting a both-CPU P95 as Boundary's; **re-pin
`EXPECTED_CENSUS_SHA256` when coverage changes.** **Route to ATTRIBUTE, re-bank
to BANK** — slice 31 read −7,104 routed, +576 built. **Collision paid and is
BANKED** (slices 35/36/37, −10,752); their owners are FLAT, so the lever was
calls not instructions — the shape slices 45/46 reused. SIZE IS NOT PERMISSION:
float in `gmcollision`/`mp*`/`ftMain*`/`ftComputer` is FROZEN by the Task 9 hash
— exact memoization/hoisting/reuse/call deletion only.

## What is dead, so nobody re-derives it

- **Effect DObj submits** — Boundary-only (99.3% there, ~12.1% of the gate arm).
  **Projectiles** · **texture thrash** · **`Find`** · **`Material`** ·
  force-load seam. **`FTR` as the P95 discriminator** — flat, 0/80. **`MISC` is
  PARTICLES and particles are FLAT** — 53,982 tk/frame particle draw, 52% of
  `MISC`, against a tail premium of only +16,864 on 26/80. **The AOT animation
  bake** (slice 32): SIZE dead.
- **Animation playback ARITHMETIC** (slices 34, 41): Requirement 4's fixed point;
  idle-joint skip (33), lazy track table (31), AObj walk and dispatch all under
  the floor. **Slice 41 spent the last lever**: 30 Hz poses cost **+7,040** *and*
  diverged the match (damage 130/51 vs 33/65). **Slice 39's table is VOID.**
  **Don't blanket-convert `ndsBaseGcPlayMObjMatAnim`** — 5 tracks pack
  0xRRGGBBAA in f32. STRUCTURAL cuts here are NOT closed.
- **The 20.12 kernels' ARITHMETIC (slice 42)** — sub-floor and non-additive.
  **The local-matrix memo is dead twice.** **The flower rigid-mask prices at
  +3,200, wrong sign.** **The token→asset_id MEMO is dead** (Task 74): bss tables
  lose to a resident chain of link-time immediates — slice 45 deleted the CALLS.
- **Five lanes closed by MEASUREMENT this cycle** — full numbers in
  `artifacts/performance/2026-08-1{1_c122,2_c123}-rebank/SLICE4{5,6}.md`:
  `ndsRelocFinalizeLoadedFile` as the gate (refuted by R2-06 E8's OWN phase
  split — one setup call is 88% of the total); growing the anim-cache arena
  (Rejects 0, and slice 46 dropped it to 192,240 of 262,144); the token→asset_id
  memo; the `OTHR` ceiling; and **every memo in the ROM is healthy**, so nothing
  banked is silently degrading.

## RAM: both budgets are near their floor — price a change before writing it

- **Static/boot.** `check-boot-headroom.ps1 -Build <dir>` after every lab build.
  Highest `fake_heap_start` proven to boot **`0x02294804`**, lowest proven to
  fail **`0x02294b24`**. **Text counts as much as bss**; a failing arm reads as a
  hung emulator. **`gSYTaskmanGeneralHeap`** free-min **70,776**, 38,008 above
  the floor. Freeing `.bss` enlarges it.

## FTR: −93,612 landed (c116); now 0/80 presence and NOT a P95 lever

**Banked `FTR` mean 291,896** vs 385,508 (−24.3%). One warning survives:

- **DS-native AOT geometry ships** (`NDS_TASK56_FIGHTER_PRIMITIVES ?= 2`), P50
  −10,112 / P95 −14,144. **It SHIPPED BROKEN** — an undirected edge made 35.6%
  of the fighter BACKFACING and `BEGIN_VTXS` on group TYPE change welded strips
  into one vertex list. Boundary passed on the broken build: **a passing verifier
  is not visual verification. Hand the owner a ROM.**

## Landed slices and the lanes they leave

**SLICE 43 WITHDRAWN 2026-08-11.** All targets force
`NDS_R2_FIGHTER_GX_COMPOSE=0`; do not re-enable without owner proof. Lead at
`nds_platform.c:3197`: the matrix stack leaks ~3 pushes/frame, wrapping mod 32.
**That line's `|| NDS_TICK_HUD` is pinned by
`check-gbi-decode-fixtures.ps1:2247`** — Boundary went RED for a cycle because
the guard moved and the assertion did not.
**SLICE 46 KEPT — 1,213,440 → 1,196,224.** The anim warm preload never finished
(`gNdsR2AnimWarmLoaded` 83 of 85) AND warmed the wrong assets: `gNdsR204AnimSeen`
gives 87 used ids of which the 85-entry list covered only **57** — 30 used ids
streamed mid-match while 28 warmed ids were never used. List replaced with the
measured 87, `gNdsR2AnimWarmStep` (4) per scene update: **misses 32 → 2**, warm
85/85, **arena 257,200 → 192,240** (it SHRINKS — no RAM budget), Rejects 0, heap
70,776, BGM playing, VBlank 4x 31→10. Split −8,448 list / −8,768 stepping.
**The bound on stepping is the BGM packet seam, not load time** — E4 prices one
load at >4.5 ms against ~186 ms. `…/2026-08-12_c123-rebank/SLICE46.md`.

**SLICE 45 KEPT — 1,225,280 → 1,213,440.** Same-binary A/B:
`ndsRelocRemoveFighterAObj16StatusAliases` resolved `ndsRelocAssetIDForToken` for
EVERY status node when `addr == data` rejects almost all in one compare.
Resolves 16,002 → 1,143, **P95 −12,160**. Exact — the resolver is a pure
function of a link-time ADDRESS.

**The fighter LOCAL matrix build is NOT a P95 lane — refuted c122.** Real size
(~24,314 tk/frame, 80/80) and memory-bound, but `FTR` is 0/80 on the tail: it is
a P50 lane. Its only remaining shape is a local-matrix memo, **DO-NOT-RETRY,
killed twice**; the Task 91 comment at `reloc_backend_renderer_dl.c:1790` argues
for it anyway and is not an invitation.

**`SINT` is the fighter INTERRUPT proc with `SCPU` nested inside it, not an
animation bucket** — reading it as one mis-attributed an A/B in c119. Now
+108,384 on 47/80, ceiling −56,512. **Zero-copy force-load is closed:**
`ftmain.c:4623` **discards the return value** and sets
`fp->figatree = fp->figatree_heap`.

**NEXT: the `SHDT` reach bound, sized on the post-slice-46 profile.** Collision
soft float is **36,462 tk/frame of tail premium** (`func_ovl2_800ED490` 15,464,
`gmCollisionGetWorldPosition` 9,288, `SetInvertMatrix` 5,510, `TestRectangle`
fdiv 3,159, `TransformMatrixAll` 3,041), ~48,000 with the functions' own
instructions — **47% of `SHDT`**, and the calls cluster 13–18x on hot frames.
`TestRectangle` does 3 divides a call, so 56.5 fdiv/hot frame decodes to **~19
rectangle tests**. Rejecting half is ~24,000. Design: AOT `maxreach` per hurtbox
joint (Σ|rest translate| up the chain), a **fail-closed** runtime assert that
each joint's local translate equals its rest value, and skip the world-transform
build when `dist(attack, victim root) > attack_r + hurt_r + maxreach`. Sound
because cycle 106 recorded that fighter animation moves ROTATIONS, not joints
(`reloc_backend_compat_shims.c:1504`) — the assert is what stops that being
faith. Needs a `battleship_` interpose on
`gmCollisionCheckFighterAttackDamageCollide`; no `decomp/` edit.

**Do not bring a micro-fix** — R2-06 E11: a load-frame-only ~8,000 cannot be
banked. Clear ~16,000 in one change, or **use the `.data` route on ONE binary**
(only if the change cannot alter gameplay state). **Every change needs an
engagement counter on BOTH sides.** **Slices 45 and 46 were both found by READING
counters the code already kept, on the gate arm, for the first time.**

**Do not re-derive these.** The Makefile's `?= 0` defaults are not the shipped
config (41 overridden). `.text.hot` is closed both directions, **3.30 cyc/insn,
worse than `.main`**; census C/D rank cost, never predict placement. **Latent
cliff, unowned:** `sNdsAObjEvent32NormalizedCount` reads **973 of 1,024** after a
minute; overflow silently **skips the animation attach** — and
`ndsAObjEvent32NormalizeScript` now shows on 24/80 tail frames. **The load-frame
exclusion is REFUTED.** **Boundary for all of it**; a visible-pixel change needs
the owner (`BUGS.md`).

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

**DO NOT PUSH (found 2026-08-12).** The rail is "the owner's given name must not
appear in tracked files; scan before pushing" and the scan is RED: 16 tracked
files carry it, all `decomp/` Rust build artifacts embedding the build machine's
user directory. Root cause is a doc/reality mismatch — `AGENTS.md` says
`/decomp/` is gitignored and it is NOT (`git ls-files decomp` reads **26,276**,
`git check-ignore` matches none). Local commits are safe; only the push is
blocked. Owner's call: untrack `decomp/` or scrub those 16.

`AGENTS.md` owns the start-of-cycle commands; `docs/P1_EXECUTION_BOARD.md` is the
only dynamic queue; `docs/BUGS.md` carries the owner's verdicts — preserve their
wording. A clean checkout builds through `build.ps1`, not bare `make`: four of
six `.inc` are gitignored and **`build.ps1`'s generator is not run by `make`, so
it can rot**. `make p1-tick` builds the measuring ROM, `make p1` the published
pair. Never pass `-j`, never override `MAKEFLAGS`, one build at a time, never
build a published target name for lab work. Preserve mode 163, renderer mode 9,
mip 0, static textures, source countdown, Dream Land water frame 0, Task 16
`1/1/1`. Never edit `decomp/`. Run `New-Smash64DSSnapshot.ps1` last.
