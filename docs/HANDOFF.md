# Handoff

Updated: 2026-08-13. **THE GATE ARM WAS MISLABELLED** (`…/2026-08-12_c126-armcheck/
ARM_MISLABEL.md`): the banked "canonical both-CPU" 1,087,296 was built `BOTH_CPU 0`. On
R2-07's own gate names HEAD measures **1,207,616 — +87,236 OVER**. **Boundary PASSES
(~1,087,600); the R2-07 stress gate FAILS**, every `EXHAUSTION.md` ceiling is dead, and
**the owner's order is bugs first, then P95** — context, not the next task.

## R2-07 `BUGS.md` — the owner playtested; two rows are live and BOTH need HIM

`docs/BUGS.md` carries the owner's own wording and is the queue — do not reword it. The
2026-08-12 playtest **closed old rows 2 and 3 by verdict** (fire burn, Fox gun); their
contracts and pixel proof stay in `artifacts/bugs/2026-08-12_r2-07-cluster/`. Candidates were
`build-c129-foxfire` / `build-c130-fire-bothcpu`; the fire mechanism cost **+9,856 `WORK-H`
P95**, inside that arm's own 9,664 repeat spread (`…/2026-08-12_c130-fire-gate/GATE.md`). The
old "all three rows converge on ONE texture-residency capability" framing was wrong for all
three — never reinstate it. **A magenta bar beside Fox is the BEAM ITSELF**, relocData 316's
own RGBA(219,0,134) — not a debug quad and not the gun.

- **Whispy — ANSWERED; nothing agent-closable remains, do not re-open.** Armed, the blink
  runs **6 presented frames** (`anim_frame` 1,3,5,7,9,11) and the eye's grandchild DObj
  squashes `scale.y` 0.948 -> 0.104 -> 1.0 through XObj **kind 28 = TraRotRpyRSca**.
  **There IS no blink texture and that is source-correct.**
  `artifacts/verification/2026-08-12_whispy-{cadence-armed,channels,xobj-kinds}.txt`, probe
  `scripts/probe-whispy-blink-script.ps1 -Cadence`. Only the owner can name which motion.
- **Fox — every geometric quantity is source-exact; the row is `BLOCKED(decision:)`.**
  Spawn X/Y/Z, radius 20, all 11 hurtbox descriptors, camera, composition (0.004 px),
  attachment, pose phase, baked gun geometry — and, closed 2026-08-13, **the quad's anchor
  and scale**: relocData 316 is `(0,+24,0) (0,-26,0) (-30,-26,0) (-30,+24,0)`, so it
  STRADDLES the projectile's Y, and `scale.y` is never written (`wpfoxblaster.c:44-52`).
  What source *does* specify is a **23.651-unit sag** — shot and flash at world y 223.398,
  bore centre 247.049, so 63.8% of the beam hangs below the barrel **in BattleShip too**.
  Numbers and two priced options: `artifacts/bugs/2026-08-12_fox-crouch/BEAM_QUAD_ANCHOR.md`.
  Moving a source-exact telegraph is the owner's call, never an agent's. The withheld
  presentation latch stays out of tree as `…_fox-crouch/wip-presentation-latch.patch`; its
  "spawns on hidden substep 0" premise is refuted (both shots `sub=1`) and it needs a
  matched/not-matched counter before anyone trusts it.
- **R2-08 cannot be finished by an agent**: SwitchPlan `:391` needs the owner's recorded
  retail play test, `:385` their visual approval.

## The two arms — same 60 s match; ONLY `BOTH_CPU 1` is R2-07's gate

| arm | role | `WORK-H` P50 | P95 |
|---|---|---:|---:|
| `battle_playable_realtime` (`BOTH_CPU 0`) | shipped, **PASSES** | 899,136 | 1,087,616 |
| **both-CPU (`BOTH_CPU 1`)** | **R2-07 GATE, FAILS** | 938,368 | **1,207,616** |

**RE-BANKED ON `BOTH_CPU 1` — `…/2026-08-12_c130-fire-gate/LANES_BOTHCPU.md`** (no build;
`analyze-tick-hud-excursion.ps1 -Ceilings` emits both tables now). `SRC` owns **84.4%** of the
395,863 excursion; `SITR` 83,712 is the largest leaf lane, `SHDT` 51,584 the sharpest presence
(13.17x on a 4,416 median). `SCPU`/`SPRM`/`SPHD` are 2.5–3.7x the Boundary table — the wrong
arm ran ONE CPU. c125's ceilings are dead. **`SITR_NEXT_CUT.md`'S CUT IS REFUTED, no build
spent (`…/2026-08-13_sitr-aobj-layout/`)**: ONE line fill per visited `AObj` serves offsets
0–28, so a dense header array only MIGRATES it. **NEXT CUT: `…/2026-08-13_c-residue/
RESIDUE.md` — NO lever predicts ≥16,000, and its §0 corrects every future sizing.**

**A PROFILE CYCLE IS HALF A TICK.** `ticks/frame = cycles / (2 × regions)`: regions P50
2,240,838 cyc = 2.0001× two VBlanks, and `cycles/2` reproduces the gate arm's `ALL` histogram.
Read as 1:1 the no-HUD profile ROM would present at HALF the heavier gate ROM's rate. So the
AObj side array is **−3,379** not −6,758, any-layout ceilings **4,578/5,246**, the `AnimValueQ`
prologue split **1,100–1,350** and `__aeabi_lmul` **~503** — all under the floor, crumbs gone.
`analyze-symbol-line-profile.py` had a THIRD unit (share of a 1,128,000 budget, 1.167× high);
it now reads `regions` and prints its `basis` line. Only `FTR`/`STG` are flat where P95 lives
(band min 296,320/171,520), so flat cuts there pay 1:1 and ADD: 8,000+8,000 = 16,000. `FTR`
−4.8% (14,232 tk/fr) or `SHDT` −26.6% (**3,779**, best leverage 4.2×). **14,691 tk/fr of
`WORK-H` is `cpuGetTiming`+`tickGetCount`** — apparatus the published ROM never runs, so the
product-side gap is ≈72,500, not 87,236.

**THE BIGGEST LEVER IS PLACEMENT.** Memory stall is **1,236,685,107 cycles, 33.8% of the
match** ≈ 386,000 tk/frame (this one WAS converted right) — an order of magnitude past `SINT`
or `SHDT`, and why slice 48's behaviourally-identical pair differed 94,976.

**SLICE 49 (reclaim dead ITCM) is REFUTED without a build — do not re-open it until
the Task 37 port group is understood.** `.itcm` is NOT full (30 of 82 residents never
execute, 2,594 B idle), but the census's 87,033,153 "in reach" stall cycles assume admitting
~3,118 B of mostly PORT functions and `NDS_TASK37_ITCM_PORT` is 0 because *"the owner
confirmed the enabled lab build misbehaves"* — CORRECTNESS. **Eviction alone pays nothing.**

**Lane-sizing traps, now encoded in `-Ceilings`:** medians do not add (that invented a
110,336 lane worth +9,472/row in c122); `OTHR` CONTAINS `WAIT`. Only `WORK-H` is spendable.
A `-Ceilings` ceiling flattens a lane to its own MEDIAN, so it prices the EXCURSION only —
it is not what deleting the lane pays (`FTR` 8,512 vs 311,744). `RESIDUE.md` §2 has both.
**Profile with `NDS_TICK_HUD_DRAW=0` or you profile the instrument** — the HUD
costs ~345,024 tk twice a second on exactly the frames P95 is decided on; the GATE
keeps `DRAW=1`. Soak length is `NDS_R2_SOAK_MATCH_MINUTES`. `Makefile:382` forbids
reporting a both-CPU P95 as Boundary's. **Route to ATTRIBUTE, re-bank to BANK.** **Collision
paid and is BANKED** (slices 35/36/37, −10,752). SIZE IS NOT PERMISSION: float in
`gmcollision`/`mp*`/`ftMain*`/`ftComputer` is FROZEN — exact memo/hoist/reuse/deletion only.

## What is dead, so nobody re-derives it

- **`SPRM` 13,056, `AUD` 13,824, `BG` 3,968 — CLOSED BY ARITHMETIC 2026-08-13**: each is
  under 16,000 **deleted entirely**. `SCPU` needs −32.1% and reads 896 on the rank-80 frame.
- **`FTR` — −93,612 landed (c116); its "0/80, NOT a P95 lever" verdict is BOUNDARY-arm and
  the gate arm's 8,512 is the EXCURSION ceiling, not what deletion pays (311,744).**
  DS-native AOT geometry ships (`NDS_TASK56_FIGHTER_PRIMITIVES ?= 2`) and **SHIPPED
  BROKEN** — 35.6% of the fighter backfacing with Boundary green: **a passing verifier is
  not visual verification.**
- **Effect DObj submits** — Boundary-only. **Projectiles** · **texture thrash** ·
  **`Find`** · **`Material`** · force-load seam. **`MISC` is PARTICLES and particles
  are FLAT.** **The AOT animation bake** (slice 32): SIZE dead.
- **Animation playback ARITHMETIC** (slices 34, 41): idle-joint skip (33), lazy track
  table (31), AObj walk and dispatch all under the floor. **Slice 41 spent the last
  lever**: 30 Hz poses cost **+7,040** *and* diverged the match (damage 130/51 vs
  33/65). **Don't blanket-convert `ndsBaseGcPlayMObjMatAnim`** — 5 tracks pack
  0xRRGGBBAA in f32. STRUCTURAL LAYOUT cuts closed 2026-08-13; call count is the lever.
- **The 20.12 kernels' ARITHMETIC (slice 42)** — sub-floor and non-additive. **The
  local-matrix memo is dead twice.** **The flower rigid-mask prices +3,200, wrong sign.**
  **The token→asset_id MEMO is dead** (Task 74). **Six more lanes closed by MEASUREMENT** —
  numbers in `…/2026-08-1{1_c122,2_c123}-rebank/SLICE4{5,6,8}.md`: `ndsRelocFinalizeLoadedFile`
  as the gate; anim-cache arena growth (Rejects 0); the `OTHR` ceiling; **BGM buffer/packet
  sizing**; **every memo is healthy**.

## RAM — price a change before writing it

`check-boot-headroom.ps1 -Build <dir>` after every lab build. Highest `fake_heap_start` proven
to boot **`0x02294804`**, lowest to fail **`0x02294b24`**. **Text counts as much as bss**; a
failing arm reads as a hung emulator. `gSYTaskmanGeneralHeap` free-min **72,188**, floor 32,768.

## Landed slices and the lanes they leave

**SLICE 43 WITHDRAWN 2026-08-11.** All targets force `NDS_R2_FIGHTER_GX_COMPOSE=0`; do not
re-enable without owner proof — it measured **−13,632 P95** but the matrix stack leaks ~3
pushes/frame, wrapping mod 32 (`nds_platform.c:3197`, whose `|| NDS_TICK_HUD` is pinned by
`check-gbi-decode-fixtures.ps1:2247`). **SLICE 46 KEPT — 1,213,440 → 1,196,224**
(`…/SLICE46.md`): warm preload covered 57 of the 87 used ids; the measured 87, 4 per scene
update, take **misses 32 → 2** and the arena 257,200 → 192,240 (it SHRINKS).

**SLICE 48 KEPT — read its SIZE, not its bank (`…/SLICE48.md`).** The FAT lane is **BGM**.
**`AUD` at 0.2% does NOT clear BGM** — a bucket brackets only its own thread and the worker
ran ABOVE main. Shipped: created at `MAIN_THREAD_PRIO + 1`, switched to `- 1` once playing
(`.data` pokeable). **Deprioritizing during the MATCH was REFUTED** — same-binary A/B,
+8,064 wrong way; creating low is −13,952..−17,792. **SLICE 45 KEPT — 1,225,280 →
1,213,440**: `ndsRelocRemoveFighterAObj16StatusAliases` resolved `ndsRelocAssetIDForToken`
for EVERY status node when `addr == data` rejects almost all in one compare. Resolves
16,002 → 1,143, **−12,160**.
**The fighter LOCAL matrix build is NOT a P95 lane — refuted c122.** Only shape left is a
local-matrix memo, **DO-NOT-RETRY, killed twice**. **`SINT` is the fighter INTERRUPT proc
with `SCPU` nested, not an animation bucket** — reading it as one mis-attributed an A/B in
c119. **Zero-copy force-load is closed:** `ftmain.c:4623` DISCARDS the return value.

**SLICE 47 REVERTED — the `SHDT` reach bound is DEAD, geometrically. `ReachTests 2,373
WouldSkip 0`** — it never rejects, and tightening it needs the joint position that IS the
transform being skipped. Carry: `gmCollisionTestRectangle` also serves item/weapon/ground —
**never attribute a shared leaf to one caller**. **The collision transform chain is honest
work, not redundancy (c123)**: latches clear once per fighter per frame (`ftmain.c:1847`),
hit detection rebuilds lazily and shares ancestors, and 13–18x is a **call-count** ratio —
the lever is touching fewer parts. The LANE is still the best leverage in the table
(`RESIDUE.md` §4): −26.6% = 16,000 P95 for 3,779 tk/fr, the cheapest in the residue.

**Do not bring a micro-fix** — R2-06 E11: a load-frame-only ~8,000 cannot be banked. Clear
~16,000 in one change, or **use the `.data` route on ONE binary** (only if the change cannot
alter gameplay state). **Every change needs an engagement counter on BOTH sides**; slices
45, 46 and 48 were all found by READING counters the code already kept, on the gate arm, for
the first time. The Makefile's `?= 0` defaults are not the shipped config (41 overridden);
`.text.hot` is closed both directions. **Latent cliff, unowned:**
`sNdsAObjEvent32NormalizedCount` reads **973 of 1,024** after a minute and overflow silently
**skips the animation attach**. **Boundary for all of it**; a visible-pixel change needs the
owner (`BUGS.md`).

## Measurement rules that change your FIRST action — board owns the rest

- **The sampler is bit-deterministic — never repeat a run.** Same ROM twice gives
  byte-identical buckets, so ANY cross-build delta is placement. `-Samples 1600 -RingDump`.
  A duplicate frame LABEL at a ring seam is warned; IDENTICAL payload is a stale read and
  always fatal, as is one away from a seam.
- **Judge on `WORK-H`**; buckets locate, they never decide (floor ≥8,544). **`ALL`
  is VBlank-quantized** — it hid a +52,928 once. **1.85 cycles of `FTR` mean per
  byte of added ARM text.** **A bucket only sees its OWN thread** (slice 48).
- **A census row in tk/fr sizes a P50 lever, NOT a P95 one.** Slices 36/37 had equal mean
  self cost and P95 wins **2.45x apart**. **Presence is the tell** — and a lane that is
  BIMODAL at the percentile returns less than its mean (`RESIDUE.md` §1).
- **A route A/B is valid only for a change that cannot alter gameplay state
  (slice 41) AND only if the poke lands before the value is READ.** `-SetGlobals`
  fires at the first frame-complete marker; record what was actually applied or
  the control is the candidate relabelled (slice 48 got 1,102,208 on both arms).
- **`--pc-detail` BEFORE designing — no build, and it routinely names a different
  lever than the source reads like.** Slice 44's guard looked like a compare but four cold
  `ldr`s were 39%; 85.5% of `ndsAObjEvent32NormalizeScript` is two pointer scans.
- **An arm that cannot produce the event reads 0 either way** — check the control differs
  first. **And a zero one level DOWNSTREAM of a rejected request reads exactly like a dead
  lane**: row 2's four zeroes were correct readings of a request killed by a NULL script
  pointer upstream, and each was read as "nothing asks for this".

## Restart surface — parked items live on the board's **Parked** list

**DO NOT PUSH (found 2026-08-12; owner said ignore for now).** The owner-given-name scan is
RED: 16 tracked `decomp/` Rust build artifacts embed the build machine's user directory —
a doc/reality mismatch, since `AGENTS.md` says `/decomp/` is gitignored and it is NOT
(`git ls-files decomp` reads 26,276). Local commits are safe; only the push is blocked.
Owner's call: untrack `decomp/` or scrub those 16.

`AGENTS.md` owns the start-of-cycle commands; `docs/P1_EXECUTION_BOARD.md` is the only
dynamic queue; `docs/BUGS.md` carries the owner's verdicts — preserve their wording. A clean
checkout builds through `build.ps1`, not bare `make`: four of six `.inc` are gitignored and
**`build.ps1`'s generator is not run by `make`**. `make p1-tick` builds the measuring ROM,
`make p1` the published pair. Never pass `-j`, never override `MAKEFLAGS`, one build at a
time, never build a published target name for lab work. Preserve mode 163, renderer mode 9,
mip 0, static textures, source countdown, Dream Land water frame 0, Task 16 `1/1/1`. Never
edit `decomp/`. Run `New-Smash64DSSnapshot.ps1` last.
