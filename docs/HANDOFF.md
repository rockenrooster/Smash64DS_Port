# Handoff

Updated: 2026-08-12. **`build-c124-slice48` measures `WORK-H` P95 1,087,296 —
UNDER the 1,120,380 gate — and that is NOT 108,928 of engineering.** The c123 bank
was 1,196,224 (Boundary GREEN); `build-c124-bgmprio-create27` restores that bank's
exact behaviour and already measures **1,101,248**, so **~94,976 is PLACEMENT** and
moves again on any unrelated edit. **The gate is met, not stably met.** Slice 48's
own lever is ~−15,000; slice 45 −11,840, slice 46 −17,216. **Every 128-frame figure
in the archive is unusable** — use `-Samples 1600`.

**THE CROSS-BUILD FLOOR IS NOT ±5,376 ONCE YOU ADD AN OBJECT.** That figure came
from two near-identical HEAD controls. Slice 48's behaviourally-identical pair
differs by **94,976** after adding ~50 bytes and one `aligned(32)` `.data` object.
Not a cache-state story: the FASTER build has more anim-cache misses (14 vs 2) and
more payload reads (135 vs 123). **Size every change with a same-binary route;
re-bank reports what the ROM measures, never what the change was worth.**

## The two baselines — same 60-second match (coverage 86.7%), slips 0 every row

| arm | role | `WORK-H` P50 | P95 |
|---|---|---:|---:|
| **both-CPU** | **THE GATE** | 900,736 | **1,087,296** (c124-slice48, ~95K of it placement) |
| **both-CPU** | prior bank | 938,752 | 1,196,224 (c123-warm) |
| **Boundary** mode 163 | shipped configuration | 920,192 | 1,113,408 (c116 — stale) |

**RE-ATTRIBUTED on the slice-48 ROM** (`…/2026-08-12_c125-slice48/split-top80.txt`,
`NDS_TICK_HUD_DRAW=0`, 1600 frames). Premium/tail frame **1,161,717 → 864,735**.
Top non-idle rows: `__aeabi_fadd` 41,633, `__aeabi_fmul` 36,658, `memcpy` 37,474
(all 80/80), **`ndsAObjEvent32NormalizeScript` 24,299 on 19/80** — 3x its c123 cost,
MORE clustered, and it is the latent cliff below. FAT family ~47,600 but on
**35/80, down from 49/80** — slice 48 moved it off 14 tail frames.

**LANE CEILINGS on that ROM** (`…/EXHAUSTION.md`; cap the lane at its own median
PER ROW, re-take the 80th of 1,600 — baseline 1,089,152 that way): `SRC`/`GCRA`
**133,056**, `SINT` 57,280, `SHDT` 38,912, `MISC` 31,680, `SPHD` 17,152, `AUD`
13,312, `SCPU` 4,288, `STG` 2,048, **`FTR` 0**. **`GCRA` == `SRC` to the tick**, so
all spendable work is inside `gcRunAll`; `FTR` is 0 for the FOURTH straight
measurement at a median of 303,232.

**THE BIGGEST LEVER IS PLACEMENT, and the census sizes it.** Memory stall is
**1,236,685,107 cycles, 33.8% of the match** (`.main` alone 45.6% of its tier)
≈ 386,000 tk/frame — an order of magnitude past `SINT` or `SHDT`, and why slice
48's behaviourally-identical pair differed 94,976.

**SLICE 49 (reclaim dead ITCM) is REFUTED without a build — do not re-open it until
the Task 37 port group is understood.** True: `.itcm` is NOT full, 30 of its 82
residents never execute (2,594 B idle), retiring the board's "ITCM is 99.1% full".
But the census's **87,033,153 non-mem stall cycles in reach** assumes admitting
~3,118 B of mostly PORT functions, and `nds_task37_itcm.h` records that
`NDS_TASK37_ITCM_PORT` is 0 because *"the owner confirmed the enabled lab build
misbehaves"* — a CORRECTNESS refutation, not a performance one; `sqrtf` is ours too
(`src/nds/r2/nds_r2_sqrtf.c`) so it is in that group. **Eviction alone pays
nothing** — ITCM is fixed 32 KB and code never fetched costs nothing. What is left
is `__aeabi_lmul` via the PROVEN library-member list (86 B ≈ 1,005 tk/frame).

**Two lane-sizing traps.** Medians do not add (subtracting nested medians from
`GCRA`'s invented a 110,336 lane worth +9,472/row); `OTHR` CONTAINS `WAIT`, so its
−116,800 ceiling is idle time. Only `WORK-H` is spendable.

**Profile with `NDS_TICK_HUD_DRAW=0` or you profile the instrument** — the HUD
costs ~345,024 tk twice a second on exactly the frames P95 is decided on. The GATE
keeps `DRAW=1`. Soak length is `NDS_R2_SOAK_MATCH_MINUTES`. `Makefile:382` forbids
reporting a both-CPU P95 as Boundary's; **re-pin `EXPECTED_CENSUS_SHA256` when
coverage changes.** **Route to ATTRIBUTE, re-bank to BANK.** **Collision paid and
is BANKED** (slices 35/36/37, −10,752); their owners are FLAT, so the lever was
calls not instructions. SIZE IS NOT PERMISSION: float in `gmcollision`/`mp*`/
`ftMain*`/`ftComputer` is FROZEN — exact memoization/hoisting/reuse/deletion only.

## What is dead, so nobody re-derives it

- **`FTR` — −93,612 landed (c116), mean 291,896 vs 385,508, and it is 0/80 on the
  tail: NOT a P95 lever.** DS-native AOT geometry ships (`NDS_TASK56_FIGHTER_PRIMITIVES ?= 2`)
  and **it SHIPPED BROKEN** — 35.6% of the fighter backfacing, `BEGIN_VTXS` on
  group TYPE change welding strips. Boundary passed on it: **a passing verifier is
  not visual verification. Hand over a ROM.**
- **Effect DObj submits** — Boundary-only. **Projectiles** · **texture thrash** ·
  **`Find`** · **`Material`** · force-load seam. **`FTR` as the P95 discriminator**
  — flat, 0/80. **`MISC` is PARTICLES and particles are FLAT.** **The AOT animation
  bake** (slice 32): SIZE dead.
- **Animation playback ARITHMETIC** (slices 34, 41): idle-joint skip (33), lazy
  track table (31), AObj walk and dispatch all under the floor. **Slice 41 spent
  the last lever**: 30 Hz poses cost **+7,040** *and* diverged the match (damage
  130/51 vs 33/65). **Slice 39's table is VOID.** **Don't blanket-convert
  `ndsBaseGcPlayMObjMatAnim`** — 5 tracks pack 0xRRGGBBAA in f32. STRUCTURAL cuts
  are NOT closed.
- **The 20.12 kernels' ARITHMETIC (slice 42)** — sub-floor and non-additive. **The
  local-matrix memo is dead twice.** **The flower rigid-mask prices at +3,200,
  wrong sign.** **The token→asset_id MEMO is dead** (Task 74) — slice 45 deleted
  the CALLS; `--pc-detail` says the two pointer SCANS are 64% of it, not the chain.
- **Six lanes closed by MEASUREMENT this cycle** — numbers in
  `artifacts/performance/2026-08-1{1_c122,2_c123}-rebank/SLICE4{5,6,8}.md`:
  `ndsRelocFinalizeLoadedFile` as the gate (refuted by R2-06 E8's OWN phase split
  — one setup call is 88% of the total); growing the anim-cache arena (Rejects 0,
  and slice 46 dropped it to 192,240 of 262,144); the token→asset_id memo; the
  `OTHR` ceiling; **BGM buffer/packet sizing** (`PACKET_BYTES` is a max bound, not
  the packet size, and the loop is already byte-addressed — see
  `RAM_RECOVERY_PLAN.md`); and **every memo in the ROM is healthy**.

## RAM — price a change before writing it

`check-boot-headroom.ps1 -Build <dir>` after every lab build. Highest
`fake_heap_start` proven to boot **`0x02294804`**, lowest to fail **`0x02294b24`**.
**Text counts as much as bss**; a failing arm reads as a hung emulator.
**`gSYTaskmanGeneralHeap`** free-min **72,188**, above the 32,768 floor.

## Landed slices and the lanes they leave

**SLICE 43 WITHDRAWN 2026-08-11.** All targets force `NDS_R2_FIGHTER_GX_COMPOSE=0`;
do not re-enable without owner proof. Lead at `nds_platform.c:3197`: the matrix
stack leaks ~3 pushes/frame, wrapping mod 32. That line's `|| NDS_TICK_HUD` is
pinned by `check-gbi-decode-fixtures.ps1:2247` — Boundary went RED because the
guard moved, not the assertion.
**SLICE 46 KEPT — 1,213,440 → 1,196,224** (`…/SLICE46.md`). The warm preload never
finished (83 of 85) and covered only 57 of the 87 used ids; replaced with the
measured 87, 4 per scene update: **misses 32 → 2**, arena 257,200 → 192,240 (it
SHRINKS). **Stepping is bounded by the BGM packet seam**, not load time.

**SLICE 48 KEPT — read its SIZE, not its bank (`…/SLICE48.md`).** The FAT lane is
**BGM**: `RefillCount` 104 == `WorkerWakeCount` 104, and the anim cache cannot own
it (2 misses since slice 46; 85 of 123 payload reads precede frame 438). **`AUD` at
0.2% does NOT clear BGM** — a bucket brackets only its own thread and the worker
ran ABOVE main. Shipped: created at `MAIN_THREAD_PRIO + 1`, switched to `- 1` once
playing (`gNdsAudioBgmWorkerPrio` / `…RunPrio` / `…PrioApplied`, `.data` pokeable).
**Deprioritizing during the MATCH was REFUTED** — same-binary A/B, +8,064 wrong
way; creating low is −13,952..−17,792 and moved the lane off 14 tail frames.

**SLICE 45 KEPT — 1,225,280 → 1,213,440.** Same-binary A/B:
`ndsRelocRemoveFighterAObj16StatusAliases` resolved `ndsRelocAssetIDForToken` for
EVERY status node when `addr == data` rejects almost all in one compare. Resolves
16,002 → 1,143, **−12,160**. Exact — a pure function of a link-time ADDRESS.

**The fighter LOCAL matrix build is NOT a P95 lane — refuted c122.** Real size
(~24,314 tk/frame, 80/80) and memory-bound, but `FTR` is 0/80 on the tail: a P50
lane. Only shape left is a local-matrix memo, **DO-NOT-RETRY, killed twice**.

**`SINT` is the fighter INTERRUPT proc with `SCPU` nested, not an animation
bucket** — reading it as one mis-attributed an A/B in c119. Ceiling −56,512.
**Zero-copy force-load is closed:** `ftmain.c:4623` DISCARDS the return value.

**SLICE 47 REVERTED — the `SHDT` reach bound is DEAD, geometrically.** Shipped
MEASURE-ONLY: **`ReachTests 2,373  WouldSkip 0  Unsound 0`** — it never rejects. The
bound sums |translate| up the parent chain, so for a hand it is most of the
fighter's extent, and `gmCollisionCheckFighterInFighterRange` has already put the
attacker inside that radius; tightening it needs the joint's position, which is the
transform being skipped. Carry: `gmCollisionTestRectangle` also serves
item/weapon/ground — **never attribute a shared leaf's volume to one caller**.

**The collision transform chain is honest work, not redundancy (c123).** Latches
clear once per fighter per frame in `ftParamsUpdateFighterPartsTransformAll`
(`ftmain.c:1847`), then hit detection rebuilds lazily and shares ancestors.
`func_ovl2_800ED490` runs 17.1 composes on a tail frame vs 1.34 on control — a
13–18x **call-count** ratio. No pass to delete; the lever is touching fewer parts.

**Do not bring a micro-fix** — R2-06 E11: a load-frame-only ~8,000 cannot be
banked. Clear ~16,000 in one change, or **use the `.data` route on ONE binary**
(only if the change cannot alter gameplay state). **Every change needs an
engagement counter on BOTH sides.** **Slices 45, 46 and 48 were all found by
READING counters the code already kept, on the gate arm, for the first time.**
The Makefile's `?= 0` defaults are not the shipped config (41 overridden).
`.text.hot` is closed both directions, **3.30 cyc/insn, worse than `.main`**.
**Latent cliff, unowned:** `sNdsAObjEvent32NormalizedCount` reads **973 of 1,024**
after a minute; overflow silently **skips the animation attach**, and
`ndsAObjEvent32NormalizeScript` shows on 24/80 tail frames. **The load-frame
exclusion is REFUTED.** **Boundary for all of it**; a visible-pixel change needs
the owner (`BUGS.md`).

## Measurement rules that change your FIRST action — board owns the rest

- **The sampler is bit-deterministic — never repeat a run.** Same ROM twice gives
  byte-identical buckets, so ANY cross-build delta is placement. `-Samples 1600
  -RingDump`. A duplicate frame LABEL at a ring seam is warned; IDENTICAL payload
  is a stale read and always fatal, as is one away from a seam.
- **Judge on `WORK-H`**; buckets locate, they never decide (floor ≥8,544).
  **`ALL` is VBlank-quantized** — it hid a +52,928 once. **1.85 cycles of `FTR`
  mean per byte of added ARM text.** **A bucket only sees its OWN thread**: `AUD`
  reads 0.2% while the BGM worker's read lands wherever main was (slice 48).
- **A census row in tk/fr sizes a P50 lever, NOT a P95 one.** Slices 36/37 had
  equal mean self cost and P95 wins **2.45x apart**. **Presence is the tell.**
- **A route A/B is valid only for a change that cannot alter gameplay state
  (slice 41) AND only if the poke lands before the value is READ.** `-SetGlobals`
  fires at the first frame-complete marker; record what was actually applied or
  the control is the candidate relabelled (slice 48 got 1,102,208 on both arms).
- **`--pc-detail` BEFORE designing — no build, and it routinely names a different
  lever than the source reads like.** Slice 44's guard looked like a compare, but
  four cold `ldr`s were 39%; the resolver's if-chain runs 1,309 times a match to
  its scans' 219,115; 85.5% of `ndsAObjEvent32NormalizeScript` is two pointer
  scans, not its logic.

## Restart surface — parked items live on the board's **Parked** list

**DO NOT PUSH (found 2026-08-12; owner said ignore for now).** The scan for the
owner's given name in tracked files is RED: 16 tracked `decomp/` Rust build
artifacts embed the build machine's user directory. Root cause is a doc/reality
mismatch — `AGENTS.md` says `/decomp/` is gitignored and it is NOT (`git ls-files
decomp` reads 26,276). Local commits are safe; only the push is blocked. Owner's
call: untrack `decomp/` or scrub those 16.

`AGENTS.md` owns the start-of-cycle commands; `docs/P1_EXECUTION_BOARD.md` is the
only dynamic queue; `docs/BUGS.md` carries the owner's verdicts — preserve their
wording. A clean checkout builds through `build.ps1`, not bare `make`: four of six
`.inc` are gitignored and **`build.ps1`'s generator is not run by `make`**. `make
p1-tick` builds the measuring ROM, `make p1` the published pair. Never pass `-j`,
never override `MAKEFLAGS`, one build at a time, never build a published target
name for lab work. Preserve mode 163, renderer mode 9, mip 0, static textures,
source countdown, Dream Land water frame 0, Task 16 `1/1/1`. Never edit `decomp/`.
Run `New-Smash64DSSnapshot.ps1` last.
