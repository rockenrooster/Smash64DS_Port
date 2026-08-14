# Handoff

Updated: 2026-08-13. **THE GATE IS NOW REPORTED RAW AND NET-OF-APPARATUS** — owner, §8 of
`…/2026-08-13_c-residue/OWNER_DECISIONS.md`: bank **raw 1,210,944 / net ≈1,185,997** against
1,120,380 = **+90,564 raw / +65,617 net**. Apparatus 24,947 (`RESIDUE.md` §5); the instrument is
NOT being slimmed, so every banked figure stays comparable. The old 1,087,296 "canonical both-CPU"
was `BOTH_CPU 0` (`…_c126-armcheck/ARM_MISLABEL.md`) and every `EXHAUSTION.md` ceiling is dead.
**Boundary PASSES (~1,087,600); the stress gate FAILS; the owner's order is bugs first, then P95.**

## R2-07 `BUGS.md` — the owner DECIDED both live rows on 2026-08-13; both await his eye

`docs/BUGS.md` carries the owner's own wording and is the queue — do not reword it. The 2026-08-12
playtest **closed old rows 2 and 3 by verdict** (fire burn, Fox gun); their contracts and pixel proof
stay in `artifacts/bugs/2026-08-12_r2-07-cluster/`. Candidates were `build-c129-foxfire` /
`build-c130-fire-bothcpu`; the fire mechanism cost **+9,856 `WORK-H` P95**, inside that arm's own
9,664 repeat spread (`…/2026-08-12_c130-fire-gate/GATE.md`). The old "all three rows converge on ONE
texture-residency capability" framing was wrong for all three — never reinstate it. **A magenta bar
beside Fox is the BEAM ITSELF**, relocData 316's own RGBA(219,0,134) — not a debug quad, not the gun.
**BOTH ROWS ARE DECIDED IN §1/§2 OF `…/2026-08-13_c-residue/OWNER_DECISIONS.md`; restart facts only:**

- **Whispy — AWAITING OWNER PLAYTEST; the owner chose "re-check after fixes" (2026-08-13).** Blink
  is 6 presented frames, `scale.y` 0.948 -> 0.104 -> 1.0 via XObj kind 28, no blink texture and that
  is source-correct; the shield joint-freeze fix may have been the real symptom. Do not re-derive.
- **Fox — DONE: the owner took the draw-only offset (2026-08-13). AWAITING PLAYTEST.** Beam and
  flash draw **+24 world Y = 3.000 screen px** (`nds_renderer.c:14979`,
  `battleship_lbparticle.c:2571`); gameplay byte-identical — 17 stops x 6 counters, 0 mismatches
  (`…/2026-08-13_c-fox-bore/BORE_OFFSET.md`). It SHIPS in the next published ROM. Every geometric
  quantity was already source-exact (`…_fox-crouch/BEAM_QUAD_ANCHOR.md`); the withheld presentation
  latch stays out of tree (`…_fox-crouch/wip-presentation-latch.patch`), its premise refuted.
- **R2-08 cannot be finished by an agent**: SwitchPlan `:391` needs the owner's recorded retail
  play test, `:385` their visual approval.

## The two arms — same 60 s match; ONLY `BOTH_CPU 1` is R2-07's gate

| arm | role | `WORK-H` P50 | P95 |
|---|---|---:|---:|
| `battle_playable_realtime` (`BOTH_CPU 0`) | shipped, **PASSES** | 899,136 | 1,087,616 |
| **both-CPU (`BOTH_CPU 1`)** | **R2-07 GATE, FAILS** | **924,864** | **1,210,944 raw / ≈1,185,997 net** |

**RE-BANKED 2026-08-13 AFTER THE LEDGER INDEX** (`…/2026-08-13_c-ledger-index/LEDGER_INDEX.md`,
`build-c144-ledgeridx`, 1,600, DLDI ON): VBI **2:1740 3:272 4:13 5+:13 max 26**. **Gap 90,564 raw /
65,617 net.** The anim-joint fix's **+49,216 came back −39,424**, now +9,792. **The shield attach path
paid for a SEARCH, not work**: each attach hit `ndsAObjEvent32FindNormalized`, a linear scan of a
1,177-entry ledger at 8.05 tk/iteration — **5,123 tk/attach, ~88% scan**. An O(1) index over that
ledger (no new key/lifetime, §3.12-clean) took P95 1,250,368 → **1,210,944**, P50 flat, `SINT`
−23,936, **40 frames 3→2**, 12,667 oracle lookups 0 mismatches, A2 falsifier brackets control 2,752.

**RE-BANKED ON `BOTH_CPU 1` — `…/2026-08-12_c130-fire-gate/LANES_BOTHCPU.md`** (no build;
`analyze-tick-hud-excursion.ps1 -Ceilings` emits both tables now). `SRC` owns **84.4%** of the
395,863 excursion; `SITR` 83,712 is the largest leaf lane, `SHDT` 51,584 the sharpest presence
(13.17x on a 4,416 median). `SCPU`/`SPRM`/`SPHD` are 2.5–3.7x the Boundary table — the wrong arm ran
ONE CPU. c125's ceilings are dead, and so is `SITR_NEXT_CUT.md`'s cut
(`…/2026-08-13_sitr-aobj-layout/`, no build). **`RESIDUE.md` §0 corrects all sizing. SLICE 50 SPENT §4 row 0 — leg A landed (**195 calls/frame → 8 sweeps a match**, `STG` −11,328 flat, viewport pixel-identical); legs B and C REFUTED, do not re-brief that row.** **B is blocked by RAM**: 828 B against the texture cache `_Static_assert`'s **72 B** of slack. **C does not exist** — slice 1 converted every per-corner writer; the residue is the shared `gl*` → `ndsRendererTask29Gl*` wrappers, which have stage callers, not an `#if`.
**Apparatus is 24,947 not 18,675** (third lane `sNdsEffectPacketArmed`, `#if NDS_TICK_HUD`, 6,272 on EVERY GX command), so the product-side gap is ≈62,300. **Owner package: `…/2026-08-13_c-residue/OWNER_DECISIONS.md`.** **RUNG 2
(quarter-rate particles) REFUTED, no build (`…/2026-08-13_c-particle-rate/`): `MISC` is a DRAW
residual** (`taskman_seam.c:5104`), so its 17,152 never priced the update half — which is `SRC`,
7,364 tk/fr, and pays **−7,493** quarter-rate (ALL particles, both halves, −33,818). **And it shares
ONE LCG with the level-3 AI**, so any cadence change diverges the match: **CHECK EVERY SUB-RATING
FOR `syUtilsRandFloat`.**

**A PROFILE CYCLE IS HALF A TICK.** `ticks/frame = cycles / (2 × regions)`; regions P50 2,240,838
cyc = 2.0001× two VBlanks, so `cycles/2` reproduces the gate arm's `ALL` histogram and every
`SITR`/`AObj`/`AnimValueQ`/`__aeabi_lmul` crumb halves and is gone (`RESIDUE.md` §0).
`analyze-symbol-line-profile.py` reads `regions` and prints its `basis`. Only `FTR`/`STG` are flat
where P95 lives (band min 296,320/171,520), so flat cuts there pay 1:1 and ADD.
**`…/2026-08-13_c-flagsweep/c123-pc-cycles.csv` caches that profile as 98,346 rows — a 79-second
one-pass replacement for the 2.6 GB scan. RANK THE INLINE ATTRIBUTION, NOT ONLY THE SYMBOL
CENSUS: the whole of `RESIDUE.md` §4 row 0 is invisible to a symbol ranking.**

**THE BIGGEST LEVER IS PLACEMENT.** Memory stall is **1,236,685,107 cycles, 33.8% of the match** ≈ 386,000 tk/frame — past `SINT`/`SHDT` by an order, and why slice 48's identical pair differed 94,976.

**SLICE 49 (reclaim dead ITCM) is REFUTED without a build — do not re-open it until the Task 37
port group is understood.** `.itcm` is NOT full (30 of 82 residents never execute, 2,594 B idle),
but the census's 87,033,153 "in reach" stall cycles assume admitting ~3,118 B of mostly PORT
functions, and the PORT bit of `NDS_TASK37_ITCM_LEAVES` is held because *"the owner confirmed the
enabled lab build misbehaves"* — CORRECTNESS. **Eviction alone pays nothing.**

**Lane-sizing traps, now encoded in `-Ceilings`:** medians do not add (it invented a 110,336 lane in c122); `OTHR` CONTAINS `WAIT`; only `WORK-H` is spendable. A `-Ceilings` ceiling flattens a lane to
its own MEDIAN, so it prices the EXCURSION only — not what deleting the lane pays (`FTR` 8,512 vs
311,744); `RESIDUE.md` §2 has both. **Profile with `NDS_TICK_HUD_DRAW=0` or you profile the
instrument** — the HUD costs ~345,024 tk twice a second on exactly the frames P95 is decided on; the
GATE keeps `DRAW=1`. Soak length is `NDS_R2_SOAK_MATCH_MINUTES`; `Makefile:382` forbids reporting a
both-CPU P95 as Boundary's. **Route to ATTRIBUTE, re-bank to BANK. Collision paid and is BANKED**
(slices 35/36/37, −10,752). Float in `mp*`/`ftMain*`/`ftComputer` is FROZEN; `gmcollision` was UNFROZEN
2026-08-13, but the LINKED ELF says only **37–52%** is reachable (`…_c-collision-seam/`, slice 52).

## What is dead, so nobody re-derives it

- **`SPRM` 13,056, `AUD` 13,824, `BG` 3,968 — CLOSED BY ARITHMETIC 2026-08-13**: each is under 16,000 **deleted entirely**. `SCPU` needs −32.1% and reads 896 on the rank-80 frame. **All 169 `?=` flags audited — ZERO unshipped wins (`…/2026-08-13_c-flagsweep/FLAG_SWEEP.md`).**
- **`FTR` — −93,612 landed (c116); its "0/80, NOT a P95 lever" verdict is BOUNDARY-arm and the
  gate arm's 8,512 is the EXCURSION ceiling, not what deletion pays (311,744).** DS-native AOT
  geometry ships (`NDS_TASK56_FIGHTER_PRIMITIVES ?= 2`) and **SHIPPED BROKEN** — 35.6% of the
  fighter backfacing with Boundary green: **a passing verifier is not visual verification.**
- **Effect DObj submits** — Boundary-only. **Projectiles** · **texture thrash** · **`Find`** ·
  **`Material`** · force-load seam. **`MISC` is the DRAW residual, not "particles"** — and
  particles are FLAT. **The AOT animation bake** (slice 32): SIZE dead.
- **Animation playback ARITHMETIC** (slices 34, 41): idle-joint skip (33), lazy track table (31),
  AObj walk and dispatch all under the floor. **Slice 41 spent the last lever**: 30 Hz poses cost
  **+7,040** *and* diverged the match (damage 130/51 vs 33/65). **Don't blanket-convert
  `ndsBaseGcPlayMObjMatAnim`** — 5 tracks pack 0xRRGGBBAA in f32. STRUCTURAL LAYOUT cuts closed
  2026-08-13; call count is the lever.
- **The 20.12 kernels' ARITHMETIC (slice 42)** — sub-floor and non-additive. **The local-matrix
  memo is dead twice.** **The flower rigid-mask prices +3,200, wrong sign.** **The token→asset_id
  MEMO is dead** (Task 74). **Six more lanes closed by MEASUREMENT** — numbers in
  `…/2026-08-1{1_c122,2_c123}-rebank/SLICE4{5,6,8}.md`: `ndsRelocFinalizeLoadedFile` as the gate;
  anim-cache arena growth (Rejects 0); the `OTHR` ceiling; **BGM sizing**; **every memo is healthy**.

## RAM — price a change before writing it

`check-boot-headroom.ps1 -Build <dir>` after every lab build. Highest `fake_heap_start` proven to
boot **`0x02294804`**, lowest to fail **`0x02294b24`**. **Text counts as much as bss**; a failing
arm reads as a hung emulator. `gSYTaskmanGeneralHeap` free-min **72,188**, floor 32,768.

## Landed slices and the lanes they leave

**SLICE 43 WITHDRAWN 2026-08-11.** All targets force `NDS_R2_FIGHTER_GX_COMPOSE=0`; do not re-enable
without owner proof — it measured **−13,632 P95** but the matrix stack leaks ~3 pushes/frame,
wrapping mod 32 (`nds_platform.c:3197`, whose `|| NDS_TICK_HUD` is pinned by
`check-gbi-decode-fixtures.ps1:2247`). **SLICE 46 KEPT — 1,213,440 → 1,196,224** (`…/SLICE46.md`):
warm preload covered 57 of the 87 used ids; the measured 87, 4 per scene update, take **misses 32 →
2** and the arena 257,200 → 192,240 (it SHRINKS).

**SLICE 48 KEPT — read its SIZE, not its bank (`…/SLICE48.md`).** The FAT lane is **BGM**; **`AUD`
at 0.2% does NOT clear it** (a bucket brackets only its own thread). Shipped: worker created at
`MAIN_THREAD_PRIO + 1`, switched to `- 1` once playing; **deprioritizing during the MATCH was
REFUTED** (+8,064 wrong way). **SLICE 45 KEPT — 1,225,280 → 1,213,440** (`…/SLICE45.md`): resolves
16,002 → 1,143, **−12,160**. **The fighter LOCAL matrix build is NOT a P95 lane — refuted c122**;
the local-matrix memo is **DO-NOT-RETRY, killed twice**. **`SINT` is the fighter INTERRUPT proc with
`SCPU` nested, not an animation bucket** — mis-attributed an A/B in c119. **Zero-copy force-load is
closed:** `ftmain.c:4623` DISCARDS the return value.

**`SHDT` IS CLOSED — bar 47,424 tk/fr, not −26.6%: the band is the transform chain, four dirty flags
so nothing to memoise, band-only cuts saturate at 78,016, fixed point only**
(`…/2026-08-13_shdt-{broadphase,band-owner}/`, RESIDUE §4 row 1). **Its file-I/O co-fire is closed
too** (`…/2026-08-13_c-band-io/`): the **SOUND-EFFECT load, not the animation one** (anim now prices
**+0**) — 91 `ndsAudioFgmPlayAtPan` 8-slot misses, **−12,736** (−13,580 worst case), lane saturates
−19,648, rank-80 carries **zero** I/O, residency impossible.
**A GATE LANE IS A MASK FOR THE PROFILE; a MECHANISM NEEDS NO MASK** — sum it over the region axis
(`analyze-io-lane-series.py`) and read its alignment-free **worst-case-pairing** bound first.

**Do not bring a micro-fix** — R2-06 E11: a load-frame-only ~8,000 cannot be banked. Clear ~16,000
in one change, or **use the `.data` route on ONE binary** (only if the change cannot alter gameplay
state). **Every change needs an engagement counter on BOTH sides**; slices 45, 46 and 48 were all
found by READING counters the code already kept, on the gate arm, for the first time. `.text.hot`
is closed both directions.
**R2-07 STRESS GATE PASSED — `…/2026-08-13_c-stress/STRESS_GATE.md`.** 3 successive matches, 5
entries, **Sudden Death UNFORCED** and played to a KO, `NO-FREEZE`, risk counters clean; the
five-minute match ran at 98.7% coverage for `WORK-H` 929,344 / **1,205,760** — **length does not
accumulate cost**. **BOTH ITS ANOMALIES ARE ATTRIBUTED, no build spent
(`…/2026-08-13_c-anim-anomalies/ANOMALIES.md`):**
- **Runaway — FIXED 2026-08-13 (`…/2026-08-13_c-animjoint-fix/ANIMJOINT_FIX.md`). There was no
  missing CLEAR; there was a missing SET.** `lbCommonAddDObjAnimJointAll` (`lb/lbcommon.c:785`) was
  an **empty stub** (`bx lr`), so `ftCommonGuardInitJoints` set `is_anim_joint` while every joint
  still held the GuardOn figatree. Five-minute arm: Figatree misreads **144 → 0** (it was 144 of
  144), runaway **50 → 0**; the runaway counter saw only 2/3 of the class — **48 of 144 decoded to
  a legal opcode and were silent**. **PRICE +49,216 P95** (bank above). Never loosen the bound.
- **AObj cliff = CAPACITY, not a leak — FIXED.** Four zero-growth stops against reuse firing 16-19/stop kills the leak theory; the shipping 1-minute arm already stood at **889/1,024**, and a LEDGER cannot be evicted (the repack has no spare bit) so capacity is the lever: **`NDS_AOBJ_EVENT32_NORMALIZED_MAX` 1024 → 2048**, +8,192 B bss, headroom **167,936**; corpus proven **1,019** by `gNdsAObjEvent32NormalizedHighWater` on a re-run five-minute match.
**START PAUSES THE MATCH** and the old whole-window freeze hash could not see it — the watch hashes the TOP band and `-PressStartOnResults` presses only on a detected Results screen.

## Measurement rules that change your FIRST action — board owns the rest

- **The sampler is bit-deterministic — never repeat a run.** Same ROM twice gives byte-identical
  buckets, so ANY cross-build delta is placement. `-Samples 1600 -RingDump`. A duplicate frame
  LABEL at a ring seam is warned; IDENTICAL payload is a stale read and always fatal, as is one
  away from a seam.
- **Judge on `WORK-H`**; buckets locate, they never decide (floor ≥8,544). **`ALL` is
  VBlank-quantized** — it hid a +52,928 once. **1.85 cycles of `FTR` mean per byte of added ARM
  text.** **A bucket only sees its OWN thread** (slice 48).
- **A census row in tk/fr sizes a P50 lever, NOT a P95 one.** Slices 36/37 had equal mean self
  cost and P95 wins **2.45x apart**. **Presence is the tell** — and a lane that is BIMODAL at the
  percentile returns less than its mean (`RESIDUE.md` §1).
- **A route A/B is valid only for a change that cannot alter gameplay state
  (slice 41) AND only if the poke lands before the value is READ.** `-SetGlobals`
  fires at the first frame-complete marker; record what was actually applied or
  the control is the candidate relabelled (slice 48 got 1,102,208 on both arms).
- **Per-line/per-PC attribution BEFORE designing — no build, and it routinely names a different
  lever than the source reads like.** Slice 44's guard looked like a compare but four cold `ldr`s
  were 39%; 85.5% of `ndsAObjEvent32NormalizeScript` is two pointer scans; the whole of RESIDUE §4
  row 0 is inlined helpers a symbol census cannot see.
- **An arm that cannot produce the event reads 0 either way** — check the control differs first.
  **And a zero one level DOWNSTREAM of a rejected request reads exactly like a dead lane**: row
  2's four zeroes were correct readings of a request killed by a NULL script pointer upstream,
  and each was read as "nothing asks for this".

## Restart surface — parked items live on the board's **Parked** list

**PUSH IS UNBLOCKED — `a4100b7`, 2026-08-13**, the owner's "scrub the 16" (`OWNER_DECISIONS.md` §9):
16 Cargo blobs under `decomp/BattleShip-main/decomp/tools/` baked the build machine's user directory.
Untracked + gitignored, byte-identical on disk; they **stay in pushed HISTORY — the owner accepted.**
Scan is `git grep -l -i -e <owner-given-name> HEAD`, now **17 → 1**: the survivor, sm64's IDO
`usr/lib/copt`, is a FALSE POSITIVE. `AGENTS.md` still miscalls `/decomp/` gitignored — 26,260 tracked.

`AGENTS.md` owns the start-of-cycle commands; `docs/P1_EXECUTION_BOARD.md` is the only dynamic
queue; `docs/BUGS.md` carries the owner's verdicts — preserve their wording. A clean checkout builds
through `build.ps1`, not bare `make`: four of six `.inc` are gitignored and **`build.ps1`'s
generator is not run by `make`**. `make p1-tick` builds the measuring ROM, `make p1` the published
pair. Never pass `-j`, never override `MAKEFLAGS`, one build at a time, never build a published
target name for lab work. Preserve mode 163, renderer mode 9, mip 0, static textures, source
countdown, Dream Land water frame 0, Task 16 `1/1/1`. Never edit `decomp/`. Run
`New-Smash64DSSnapshot.ps1` last.
