# Handoff

Updated: 2026-08-12. **THE GATE ARM WAS MISLABELLED — read
`artifacts/performance/2026-08-12_c126-armcheck/ARM_MISLABEL.md` FIRST.**
`build-c124-slice48`, the ROM behind the banked “canonical both-CPU” 1,087,296,
was built **`NDS_R2_BOTH_CPU 0`** — a Boundary-arm figure. On the arm R2-07's gate
names (Mario CPU vs Fox CPU, `BOTH_CPU 1`) HEAD measures **1,207,616 — +87,236
OVER**. **Boundary PASSES (~1,087,600); the R2-07 stress gate FAILS.** Every lane
ceiling in `EXHAUSTION.md` was computed on `BOTH_CPU 0` rows and must be redone.
**Every 128-frame figure is unusable** — use `-Samples 1600`. **Size every change
with a same-binary route; re-bank reports what the ROM measures.**

## R2-07 is the ACTIVE front, not performance — all three `BUGS.md` rows

Contracts + evidence: `artifacts/bugs/2026-08-12_r2-07-cluster/CONTRACT.md`.
**All three rows converge on ONE capability: make additional source textures
resident.** Budget is a non-issue (~1 KB against the 74,496 B the 2026-08-03
`repack_paletted` returned), and `build_runtime_qualified_whispy_record`
(`generate_battle_playable_static_textures.py:901`) already does exactly this for
Whispy's MOUTH — the P2 deferral in `KNOWN_ISSUES:102` was about a GENERAL
dynamic-variant system, not about space.

- **Row 3 (Fox gun) — state half SHIPPED and MEASURED.** The gun is model part 13
  on joint 17; asset already ships (`reloc_extern_data/MiscData315`) and
  `ftmain.c:575` already dispatched the event into a no-op stub. Now
  `Set=On=Reset=18` against 17 blaster spawns. **Remaining is the DRAW only** — it
  cannot ride `SubmitWeaponDObj` (gates on `IsWeaponDisplay`). **Row 3b needs no
  work**: the port `#include`s `ftfoxspecialn.c` verbatim and the source local Y
  offset is `0.0F`, so beam Y *is* joint 17's world Y.
- **Row 2 (fire burn): the Flame lane is DEAD — do NOT touch `P1_PARTICLE_SEAMS`.**
  Script `0x12` is unreachable, and MEASURED 0 requests on BOTH arms including
  `NDS_R2_BOTH_CPU=1` where Mario threw ~5 fireballs. `nEFKind*Flame` comes from a
  fighter's own motion script and neither fighter emits it. The reachable burn is
  **`FireGrind` (script 11, packed)**, whose texture 5 is **frozen at frame 0 of 3**
  (`firegrind_bake.py:16`) — so row 2 is the shared texture-variant fix, not a
  seam-list change. Next: count FireGrind/DamageFire requests.
- **Row 1 (Whispy face)** is NOT a rate problem. Source double-blinks 10 ticks
  apart every 40–309 ticks; the blink image was simply never made resident. The
  `gNdsPupupuUpdateBlinkWait*` globals are **probe** values
  (`ndsGRPupupuRunSafeUpdateProbe` forces `game_status`), not the live path — read
  `gGRCommonStruct.pupupu.whispy_blink_wait` for that.

**R2-08 cannot be finished by an agent**: SwitchPlan `:391` needs the owner's recorded retail play test, `:385` their visual approval.

## The two arms — same 60 s match; ONLY `BOTH_CPU 1` is R2-07's gate

| arm | role | `WORK-H` P50 | P95 |
|---|---|---:|---:|
| Boundary arm (`BOTH_CPU 0`) | shipped config, **PASSES** | 899,136 | 1,087,616 |
| **both-CPU (`BOTH_CPU 1`)** | **R2-07 GATE, FAILS** | 938,368 | **1,207,616** |

**RE-ATTRIBUTION and LANE CEILINGS live in `…/2026-08-12_c125-slice48/` — but they
are BOUNDARY-arm and must be recomputed on `BOTH_CPU 1`.** Headlines, still useful
for direction: `ndsAObjEvent32NormalizeScript` 24,299 on 19/80 is the largest
non-idle non-softfloat row and the latent cliff below; `GCRA` == `SRC` to the tick,
so everything spendable is inside `gcRunAll`.

**THE BIGGEST LEVER IS PLACEMENT.** Memory stall is **1,236,685,107 cycles,
33.8% of the match** ≈ 386,000 tk/frame — an order of magnitude past `SINT` or
`SHDT`, and why slice 48's behaviourally-identical pair differed 94,976.

**SLICE 49 (reclaim dead ITCM) is REFUTED without a build — do not re-open it until
the Task 37 port group is understood.** `.itcm` is NOT full (30 of 82 residents
never execute, 2,594 B idle), but the census's 87,033,153 stall cycles "in reach"
assumes admitting ~3,118 B of mostly PORT functions, and `NDS_TASK37_ITCM_PORT` is
0 because *"the owner confirmed the enabled lab build misbehaves"* — a CORRECTNESS
refutation. **Eviction alone pays nothing.** What is left is `__aeabi_lmul` via the
proven library-member list (86 B ≈ 1,005 tk/frame). Full detail in `EXHAUSTION.md`.

**Two lane-sizing traps.** Medians do not add (subtracting nested medians from
`GCRA`'s invented a 110,336 lane worth +9,472/row); `OTHR` CONTAINS `WAIT`, so its
−116,800 ceiling is idle time. Only `WORK-H` is spendable.

**Profile with `NDS_TICK_HUD_DRAW=0` or you profile the instrument** — the HUD
costs ~345,024 tk twice a second on exactly the frames P95 is decided on; the GATE
keeps `DRAW=1`. Soak length is `NDS_R2_SOAK_MATCH_MINUTES`. `Makefile:382` forbids
reporting a both-CPU P95 as Boundary's. **Route to ATTRIBUTE, re-bank to BANK.**
**Collision paid and is BANKED** (slices 35/36/37, −10,752). SIZE IS NOT
PERMISSION: float in `gmcollision`/`mp*`/`ftMain*`/`ftComputer` is FROZEN — exact
memoization/hoisting/reuse/deletion only.

## What is dead, so nobody re-derives it

- **`FTR` — −93,612 landed (c116) and it is 0/80 on the tail: NOT a P95 lever.**
  DS-native AOT geometry ships (`NDS_TASK56_FIGHTER_PRIMITIVES ?= 2`) and **it
  SHIPPED BROKEN** — 35.6% of the fighter backfacing. Boundary passed on it: **a
  passing verifier is not visual verification. Hand over a ROM.**
- **Effect DObj submits** — Boundary-only. **Projectiles** · **texture thrash** ·
  **`Find`** · **`Material`** · force-load seam. **`MISC` is PARTICLES and particles
  are FLAT.** **The AOT animation bake** (slice 32): SIZE dead.
- **Animation playback ARITHMETIC** (slices 34, 41): idle-joint skip (33), lazy
  track table (31), AObj walk and dispatch all under the floor. **Slice 41 spent
  the last lever**: 30 Hz poses cost **+7,040** *and* diverged the match (damage
  130/51 vs 33/65). **Don't blanket-convert `ndsBaseGcPlayMObjMatAnim`** — 5 tracks
  pack 0xRRGGBBAA in f32. STRUCTURAL cuts are NOT closed.
- **The 20.12 kernels' ARITHMETIC (slice 42)** — sub-floor and non-additive. **The
  local-matrix memo is dead twice.** **The flower rigid-mask prices +3,200, wrong
  sign.** **The token→asset_id MEMO is dead** (Task 74).
- **Six more lanes closed by MEASUREMENT** — numbers in
  `…/2026-08-1{1_c122,2_c123}-rebank/SLICE4{5,6,8}.md`: `ndsRelocFinalizeLoadedFile`
  as the gate; anim-cache arena growth (Rejects 0); token→asset_id memo; the `OTHR`
  ceiling; **BGM buffer/packet sizing**; **every memo is healthy**.

## RAM — price a change before writing it

`check-boot-headroom.ps1 -Build <dir>` after every lab build. Highest
`fake_heap_start` proven to boot **`0x02294804`**, lowest to fail **`0x02294b24`**.
**Text counts as much as bss**; a failing arm reads as a hung emulator.
**`gSYTaskmanGeneralHeap`** free-min **72,188** (floor 32,768).

## Landed slices and the lanes they leave

**SLICE 43 WITHDRAWN 2026-08-11.** All targets force `NDS_R2_FIGHTER_GX_COMPOSE=0`;
do not re-enable without owner proof. Lead at `nds_platform.c:3197`: the matrix
stack leaks ~3 pushes/frame, wrapping mod 32. That line's `|| NDS_TICK_HUD` is
pinned by `check-gbi-decode-fixtures.ps1:2247`.
**SLICE 46 KEPT — 1,213,440 → 1,196,224** (`…/SLICE46.md`). Warm preload covered
only 57 of the 87 used ids; replaced with the measured 87, 4 per scene update:
**misses 32 → 2**, arena 257,200 → 192,240 (it SHRINKS).

**SLICE 48 KEPT — read its SIZE, not its bank (`…/SLICE48.md`).** The FAT lane is
**BGM**. **`AUD` at 0.2% does NOT clear BGM** — a bucket brackets only its own
thread and the worker ran ABOVE main. Shipped: created at `MAIN_THREAD_PRIO + 1`,
switched to `- 1` once playing (`.data` pokeable). **Deprioritizing during the
MATCH was REFUTED** — same-binary A/B, +8,064 wrong way; creating low is
−13,952..−17,792.

**SLICE 45 KEPT — 1,225,280 → 1,213,440.** `ndsRelocRemoveFighterAObj16StatusAliases`
resolved `ndsRelocAssetIDForToken` for EVERY status node when `addr == data` rejects
almost all in one compare. Resolves 16,002 → 1,143, **−12,160**.

**The fighter LOCAL matrix build is NOT a P95 lane — refuted c122.** Real size but
`FTR` is 0/80 on the tail: a P50 lane. Only shape left is a local-matrix memo,
**DO-NOT-RETRY, killed twice**. **`SINT` is the fighter INTERRUPT proc with `SCPU`
nested, not an animation bucket** — reading it as one mis-attributed an A/B in c119.
**Zero-copy force-load is closed:** `ftmain.c:4623` DISCARDS the return value.

**SLICE 47 REVERTED — the `SHDT` reach bound is DEAD, geometrically.**
**`ReachTests 2,373  WouldSkip 0`** — it never rejects; tightening it needs the
joint position that is the transform being skipped. Carry:
`gmCollisionTestRectangle` also serves item/weapon/ground — **never attribute a
shared leaf's volume to one caller**.

**The collision transform chain is honest work, not redundancy (c123).** Latches
clear once per fighter per frame (`ftmain.c:1847`); hit detection rebuilds lazily
and shares ancestors. 13–18x is a **call-count** ratio, not redundancy — no pass to
delete; the lever is touching fewer parts.

**Do not bring a micro-fix** — R2-06 E11: a load-frame-only ~8,000 cannot be
banked. Clear ~16,000 in one change, or **use the `.data` route on ONE binary**
(only if the change cannot alter gameplay state). **Every change needs an
engagement counter on BOTH sides.** **Slices 45, 46 and 48 were all found by
READING counters the code already kept, on the gate arm, for the first time.**
The Makefile's `?= 0` defaults are not the shipped config (41 overridden);
`.text.hot` is closed both directions. **Latent cliff, unowned:**
`sNdsAObjEvent32NormalizedCount` reads **973 of 1,024** after a minute and overflow
silently **skips the animation attach**. **Boundary for all of it**; a
visible-pixel change needs the owner (`BUGS.md`).

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
- **An arm that cannot produce the event reads 0 either way.** R2-07 row 2: the
  normal arm has Mario as an IDLE HUMAN, so it makes no fire at all — check the
  control differs (`ModelPartOn` 18→5) before believing any zero.

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
never override `MAKEFLAGS`, one build at a time, never build a published target
name for lab work. Preserve mode 163, renderer mode 9, mip 0, static textures,
source countdown, Dream Land water frame 0, Task 16 `1/1/1`. Never edit `decomp/`.
Run `New-Smash64DSSnapshot.ps1` last.
