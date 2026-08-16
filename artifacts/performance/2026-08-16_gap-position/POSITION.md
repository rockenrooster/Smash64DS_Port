# The fidelity-neutral inventory is 53,215 tk/fr against a 94,481 gap. It does not close, and no combination of it does.

**Date:** 2026-08-16 · **Branch:** `codex/r2-runtime2` · **HEAD `f644ea15c8b`**
2 lab builds, 2 whole-match gate runs, 0 source changes, no default flipped, no ROM published.
**UNITS: 2 profile cycles = 1 project tick.** Every table states its window.

> **Written for a reader who has not followed this campaign.** Everything needed to act on
> it is here; citations are for checking, not for reading first.

---

## 0. Outcome first

```text
GAP       +94,481 net ticks per presented frame at rank-80.
          build-c206-shipgx0: rank-80 raw 1,239,808 / net 1,214,861 against the
          1,120,380 gate.  This is the SHIPPING renderer (GX_COMPOSE 0), bore 0,
          the one-minute mode-163 match, 1,600 samples, frames 439-2038,
          slips=0.  Every ratio in this document divides by this number.

ANSWER    THE WORK THAT COSTS NO FIDELITY AT ALL SUMS TO 53,215 tk/fr = 0.563x
          OF THE GAP, AT 100% CONVERSION OF EVERY ITEM.  100% is available for
          none of them, two of them are not yet items at all (each needs one
          counter first), and one is measured unable to reach 100%.
          Deleting the whole inventory outright leaves +41,266 still to find.
          THE ENGINEERING LADDER IS EXHAUSTED AS A ROUTE TO THE GATE.

RUNGS     Priced in PROJECT_GOAL.md's own sacrifice order, against +94,481:
            1  audio fidelity          <= 3,040 tk/fr    0.032x   cannot close
            2  visual fidelity         44,476-50,203     0.471x-0.531x  no
            3  gameplay fidelity       55,473-84,451     0.587x-0.894x  no
                                       (a PROJECTION at two measured rates)
            4  original 60 Hz sim      291,488           3.085x   CLOSES ALONE
          Rungs 1+2+all fidelity-neutral together reach 1.03x-1.09x -- i.e.
          they close only if literally everything converts at 100%.

SO        Only rung 4 closes with margin, and it closes with 3x margin, which
          means a PARTIAL cadence change is on the table rather than a blanket
          one.  Section 4 gives three sub-rungs that each close, and states
          exactly which systems must NOT halve.

BLOCKED   BLOCKED(decision: sacrifice order).  This is the owner's call under
          PROJECT_GOAL.md rail 5.  Nothing here is chosen and nothing was
          changed.  Section 5 also folds in the two decisions already with the
          owner, so they are not answered in isolation.
```

---

## 1. The gap, and what it is measured on

| | |
|---|---|
| arm | `builds/build-c206-shipgx0` |
| target | `smash64ds-battle-playable-tickhud-hwtri` (the measurement instrument) |
| config | `NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1`, **`NDS_R2_FIGHTER_GX_COMPOSE 0`**, bore 0, DLDI on |
| match | Boundary `battle_playable_realtime`, mode 163, one minute |
| window | 1,600 samples, frames 439–2038, `-RingDump`, `slips=0` |
| series | `WORK-H`; rank-80 = 80th-largest of the run's own 1,600 rows; net = raw − 24,947 apparatus |
| **rank-80** | **1,239,808 raw / 1,214,861 net** |
| **gate** | 1,120,380 (`PROJECT_GOAL.md`, P95 ≤ 1.12M ARM9 ticks per presented frame) |
| **GAP** | **+94,481** |

**Why this and not the number the board carried until yesterday.** Every bank from
`build-c185` to `build-c203` was built `NDS_R2_FIGHTER_GX_COMPOSE=1` while `Makefile:1545`
pins the published block to `0` — they measured a renderer the user does not run
(`../2026-08-16_gxcompose-bank-basis/BASIS.md`, proven from each build's own generated
header and from `nm` finding 8 `gNdsR2GxCompose*` symbols that exist only inside that
`#if`). The superseded denominators and their conversions:

| superseded | value | multiply by | to get +94,481 basis |
|---|---:|---:|---|
| `c199-bank0` (GX=1, bore 0) | +85,393 | ×0.904 | this document |
| `c200-bank84` (GX=1, bore 84) | +81,297 | ×0.860 | this document |
| `c185` bank | +28,689 | ×0.304 | this document |
| `c170` bank (`MENU.md`) | +32,593 | ×0.345 | this document |

`LADDER.md`, `MENU.md`, `DRAW_FIXEDPOINT.md` and `CAMERA_Q20_12.md` were re-divided in
place on 2026-08-16 and now state the denominator at every table that carries a ratio.

---

## 2. What remains that costs no fidelity at all

"Fidelity-neutral" means the change is invisible and inaudible and does not alter
gameplay: the same picture, the same sounds, the same match. Sizes are rank-80 marginal
ticks per frame from the census basis in `LADDER.md` §0; the conversion column is the
measured rate at which that size becomes rank-80 movement.

| item | tk/fr | conversion | **x of +94,481** | state |
|---|---:|---|---:|---|
| Slice 2 — `gcRunAll` scheduler machinery | 17,786 | 1.00x (flat lane) | **0.188x** | **100% is measured unavailable** — a flat vector still calls each process |
| ~~Texture-bind collapse (103.45 requests → 55.73 binds)~~ | ~~13,868~~ | see below | ~~0.147x~~ | **RE-PRICED 2026-08-16 — the elision half is REFUTED at ZERO** |
| In-match asset I/O — delete all seven load frames | 9,863 | 1.00x (measured as a deletion) | **0.104x** | engineering; 100% = deleting the lane |
| ~~`ndsRendererSyncTextureTile` (72.68 syncs/frame)~~ | ~~8,867~~ | **0.41x measured** | ~~0.094x~~ | **BANKED 2026-08-16 — memo shipped, −3,648** |
| newlib `_svfiprintf_r` shipped residual | 1,526 | — | **0.016x** (ceiling 4,267 = 0.045x) | engineering |
| no-Z bit-identical repeat matrix loads | 1,305 | 1.00x | **0.014x** | engineering |
| ~~**TOTAL at 100% conversion**~~ | ~~**53,215**~~ | | ~~**0.563x**~~ | **superseded — see the correction below** |

> ### CORRECTION, 2026-08-16 — the two counter-gated rows had their counter, and it had already been read
>
> `../2026-08-16_tilesync-memo/TILESYNC.md`. `NDS_TASK107_RENDER_STATE_CENSUS` publishes
> both counters and was **run on 2026-08-15** (`../2026-08-15_renderer-state-redundancy/`).
> The 22,735 tk/fr this table carried as *unsized volume* resolves as follows, and it is
> **10,849 tk/fr smaller than the inventory assumed**:
>
> | was | now | how |
> |---|---|---|
> | `ndsRendererSyncTextureTile` 8,867, unmeasured | **−3,648 BANKED** | 62.12% of 144,105 calls provably redundant (89,511 skips, this cycle's own counters, identical on both route arms); an exact serial memo ships in `build-c217-tilesync-ship`, measured on a zero-noise same-binary route, 87.1% of 1,600 frames improve, complement-controlled `WORK-H −3,648 / WAIT +3,648 / ALL +0` |
> | texture-bind collapse 13,868, unmeasured | **0 as an elision item** | the 2026-08-15 census measured **zero** exact locally-redundant bind issues; 46.05% of requests are already elided; the 26,769 revisits need draw REORDERING, ceiling **2,484** |
> | — | **+5,754 as a PLACEMENT item** | 45% of the bind pair's 13,868 is `icache_fill` on **360 bytes** (14.19 and 21.22 tk/fr per byte). Ceiling, unbuilt; 268 B of it is port-reachable and ITCM has 512 B free |
>
> **Corrected fidelity-neutral inventory: 30,480 available today + 2,484 reordering
> ceiling + 5,754 placement ceiling = 38,718**, against a gap that is now **+73,425**
> (`build-c217-tilesync-ship`) — **0.527x**, from four items none above 0.24x. The ratio
> barely moves because the gap moved too; what moved is that **10,497 tk/fr of it was
> never there.**

**Verified against the gate series rather than summed on paper.** Deleting 53,215 from
every one of `build-c206-shipgx0`'s 1,600 frames and re-ranking gives rank-80 1,186,593 =
**gap +41,266**. (Calibration: a flat cut of 94,481 lands the gap at exactly 0, and a flat
cut of 22,608 moves rank-80 by exactly 22,608 — the re-rank is exact.)

**The subset available today** — no owner decision, no pending measurement, no new counter
— is **30,480 tk/fr = 0.323x**, four items none larger than 0.19x. It leaves **+64,001**.

### 2.1 What is closed, and why it is zero rather than small

Each of these was a live candidate within the last two weeks and each is now measured shut.
They are listed so nobody re-prices them.

| lane | why it is zero | evidence |
|---|---|---|
| **Animation representation** (lane size 33,951) | converts **~1%**: both sides of the exchange are ~3,800 tk/fr and cancel; **−319 tk/fr** linear to 100% coverage | v3 capture took the FETCH branch, dense side 72.4% icache+dcache fill; the ON arm executes 664,438 *fewer* instructions |
| **devkitARM/devkitPro toolchain bump** | **no-op, three independent ways**: r67 is the newest release (nothing to bump to); the six soft-float objects are hand-written `ieee754-sf.S` assembly; and what is left from libgcc is byte-identical across `-march` levels while what varies was already replaced by this repo | `36f908dcbcc` |
| **ITCM golden reclaim** | 632 B is dead code but only **56 B** is cleanly droppable, and dropping it buys **no ticks** — it is space, against a 916 B overflow it cannot cover | `BASIS.md` §6 |
| **`GX_COMPOSE`** | the −17,152 is **retired**; two independent build pairs now agree it deletes nothing resolvable | §3 below |
| **Fighter narrow-phase fixed-point collision** | route measured closed at an exchange rate of 2.68 | `EXCHANGE.md` |
| **`__aeabi_fadd` micro-optimisation** | the function is **flat**: top PC 4.63% over 81 PCs, 96.22% issue stall. There is no instruction to delete; the only lever is the call count, which is a different task | `BASIS.md` §7 |

---

## 3. The `GX_COMPOSE` transfer, settled — the stage never paid

The open question was whether the flag's `FTR −8,448 / STG +6,592` was a real cost moving
onto the stage, a tick-HUD accounting artefact, or something else. **It is none of the
first two: the stage's work is bit-identical, and the STG rise does not reproduce.**

**Two builds, one config line apart** (`diff` of the generated headers is exactly
`NDS_R2_FIGHTER_GX_COMPOSE 0` → `1`; `nm` finds 0 vs 8 `gNdsR2GxCompose*` symbols — a
negative control that cannot be faked), both carrying `NDS_TASK103_STAGE_RUN_PHASE=1`,
which partitions the whole `STG` bucket at its four accumulation sites. The partition is
complete: the four sites sum to **223,978 tk/fr** against the bucket's own mean of
**223,084** — 0.4% apart.

**Every stage work counter is identical to the unit, over 2,038 presented frames:**

| counter | GX=0 | GX=1 | per frame |
|---|---:|---:|---:|
| `gNdsTask103WordCount` (GX command words the stage pushes) | 7,972,976 | **7,972,976** | 3,912.2 |
| `gNdsTask103RunCount` | 67,188 | **67,188** | 33.0 |
| `gNdsTask103CommitCount` | 16,296 | **16,296** | 8.00 |
| `gNdsTask103DisplayCount` | 55,890 | **55,890** | 27.4 |
| `gNdsTask103PrepareCount` / `FinishCount` / `TraversalCount` | 2,038 / 2,037 / 8 | **identical** | |

and `gNdsTask103PushTicks` — the span that actually writes those words — differs by
**+9.7 tk/fr, 0.09%**.

**And the STG rise inverts under the instrument.** Paired per-frame medians, 1,600 frames:

| pair | ΔFTR | ΔSTG | ΔMISC | Δ draw side | Δ WORK-H | Δ rank-80 |
|---|---:|---:|---:|---:|---:|---:|
| `c206→c207` (uninstrumented) | **−8,192** | **+6,656** | +2,368 | +832 | +4,544 | −7,040 |
| `c208→c209` (Task 103 instrumented) | **−8,576** | **−1,632** | −1,088 | −11,392 | −13,248 | −8,256 |

- **The fighter saving reproduces** on both pairs (−8,192 on 1,599/1,600 frames; −8,576 on
  1,597/1,600). **The stage rise does not.**
- **It is not a quantisation artefact.** `[[all-is-a-quantized-gate]]`'s failure mode needs
  a floored row that reads 0 and dumps its ticks into a neighbour. Across 3,200 sampled
  frames on the two uninstrumented arms there is **not one zero row in any bucket**;
  `STG`'s minimum is 171,776 / 178,048 and the sampling granularity is 64 ticks — 0.036% of
  the bucket. Both spans are plain `cpuGetTiming()` deltas at named seams
  (`reloc_backend_movement.c` ×4, `reloc_backend_renderer_dl.c` ×2).
- **What is left is either cross-build placement in that one pair, or a GX queue effect
  that the instrument's own ~43,000 tk/fr of added stage work absorbs. This cycle cannot
  separate those two**, and does not need to: under either reading there is no stage work
  to recover. Stated as a bound, not a mechanism.
- **Scale check for the placement reading:** on the uninstrumented pair the *simulation*
  buckets, which this flag cannot touch, move **+3,456 (`SRC`)** — that is this pair's own
  placement term, measured, and it is half the size of the STG rise.

**Consequence for the gate: the flag deletes nothing.** Whole draw side +832 paired median
on one pair and −11,392 on the other; rank-80 −7,040 and −8,256, both inside the ≥14,080
cross-build floor; P50 **disagrees in sign between the two pairs**. The −17,152 stays
retired and no ratio in this document owes anything to it.

> **Instrument trap, recorded so it is not re-paid.** `gNdsTask103BeginMtxTicks` and
> `gNdsTask103BeginMtxCount` are declared `[4]` (`nds_renderer.c:29634-29635`). Read as
> scalars through the harness's `-ExtraGlobals` they returned **their own addresses** —
> `35,173,948` = `0x0218b63c`, exactly the `nm` value — with no error. Both reads were
> discarded. A counter that looks like data and is an address is the shape that gets
> published.

---

## 4. Rung 4, re-derived on this tree: the 60 Hz simulation is worth 3.085x the gap

The recorded figure for compensated 30 Hz was **119,744 tk/fr** (Task 106), which
`MENU.md` §4.3 correctly notes no census could confirm or refute. It can be derived
directly from the gate series instead, and it is **2.4× larger than recorded**.

**The derivation, and why it is sound.** `taskman_seam.c:4273-4275` states it outright:

> "`SRC` is exactly this many logical updates per presented frame — `ndsRunMarioFoxProofUpdate`
> is the only writer of `gNdsTickHudSourceTicks`, and it brackets `scVSBattleFuncUpdate`."

The loop runs **2.000** logical updates per presented frame. So `SRC` is exactly the two
60 Hz updates, and running one instead of two deletes about half of it. Delete half of
`SRC` from every one of `build-c206-shipgx0`'s own 1,600 rows and re-rank:

| intervention | new rank-80 raw | moved | new gap | closes? |
|---|---:|---:|---:|:--:|
| *calibration:* flat cut of 94,481 | 1,145,327 | 94,481 | **+0** | — |
| *calibration:* flat cut of 22,608 | 1,217,200 | 22,608 | +71,873 | — |
| **`SRC` half — the whole simulation at 30 Hz** | **948,320** | **291,488** | **−197,007** | **YES (3.085x)** |
| `GCRA` half — `gcRunAll` bodies only | 950,944 | 288,864 | −194,383 | YES (3.057x) |
| `GCRA` half, **`SCPU` and `SINT` held at 60 Hz** | 1,118,560 | 121,248 | −26,767 | YES (1.283x) |
| `SINT` half only | 1,107,744 | 132,064 | −37,583 | YES (1.398x) |
| `SCPU` half only — the level-3 AI at 30 Hz | 1,212,320 | 27,488 | +66,993 | no (0.291x) |

**Only 16.4% of `SRC` has to go to close the gate** — about a third of one of the two
updates.

**Why it beats its own bucket size.** `SRC` concentrates **2.09×** on the gate population:
P50 332,928, top-80 median 695,872. The frames that set rank-80 are simulation-heavy
frames (the hit-detection sub-bucket `SHDT` reads 4,160 at P50 and 169,440 at the top-80
median). Halving a bucket that is concentrated *where the percentile lives* removes more
from the tail than from the middle — the exact inverse of the failure mode that has killed
several levers here.

**Three caveats, stated rather than discovered:**

1. **"Half of `SRC` per frame" is an approximation.** The two updates are not equal cost,
   and an event (a hit, a KO) lands in one of them, not both. The error is largest on
   exactly the tail frames being ranked. **The rung survives it**: it closes with 3.085×
   margin, so it tolerates a 3× error in this assumption before the conclusion changes.
2. **No compensation cost is included.** A compensated 30 Hz adds work back (§4.1). None of
   it is measured. This is a ceiling on the rung, not a prediction of an implementation.
3. **`NDS_TASK106_UPDATES_PER_PRESENT=1` exists as a build flag but is not a candidate** —
   the source says so at the same site: uncompensated, the match simply advances at half
   speed. It prices the rung; it does not implement it.

### 4.1 What "compensated" would have to mean, concretely

**Halve** — this is where the 291,488 is: the `gcRunAll` process bodies. Fighter physics
integration, animation advance, camera, particle and effect update, stage update.

**Must NOT halve**, each for a named reason:

| system | why it cannot halve | consequence if it does |
|---|---|---|
| **Input sampling** | latency doubles 16.7 → 33.3 ms, and single-frame input windows stop being expressible | the game stops feeling like SSB64 — this is the requirement `PROJECT_GOAL.md` protects by name |
| **Hitbox / hurtbox resolution** | SSB64 hitboxes live as few as 2 frames and travel fast | fast hitboxes tunnel through hurtboxes. Needs swept or sub-stepped collision — which **adds cost back**, and collision is the *concentrated* half of the soft-float class, i.e. the expensive half to keep |
| **RNG sequence** | `syUtilsRandFloat` is **one LCG shared across 135 draw sites** — 65 in the level-3 AI, 44 in `efmanager.c`, 26 in `lbparticle.c` | halving the update halves the draw count and desynchronises the AI's decisions from the source. **This is the known-fatal one** and must be designed against first |
| **Frame-counted gameplay state** | hitstun, shieldstun, stale-move queue, invulnerability, ledge cooldown, respawn timers, the match timer are integer frame counters in the source | at 30 Hz each must advance by 2, so every odd-length window rounds — a 5-frame hitstun becomes 4 or 6 |

**Known prior failure:** `plan.md` §3 item 8 records that blanket 30 Hz poses **regressed
and diverged**. Any proposal has to be per-system, which is precisely what the sub-rungs in
the table above make possible: `GCRA` half with `SCPU` and `SINT` held at 60 Hz still
closes at 1.283×, and that variant leaves the AI and the interrupt/physics half untouched.

> **One documentation defect found in passing.** `taskman_seam.c:4277` says this rung is
> "third in the sacrifice order, above gameplay fidelity and frame rate". `PROJECT_GOAL.md`
> puts it **fourth** — below gameplay fidelity and above stable 30 FPS. Both clauses of the
> comment are wrong. Not edited this cycle (no source change was in scope).

---

## 5. The other three rungs, and the two decisions already with the owner

### Rung 1 — audio fidelity: ≤ 3,040 tk/fr, 0.032x. It cannot close 4% of the gap.

`AUD` is one call to `ndsAudioBackendUpdate()` per presented frame
(`taskman_seam.c:4487-4495`). On the shipping arm it is **P50 2,880, top-80 median
3,040 = 0.032x**. The reason it is so small is structural: **the gate is an ARM9 metric and
DS audio mixing is the ARM7's job**, so audio fidelity is close to free on the number being
gated. `PROJECT_GOAL.md` ranks it first to sacrifice; it is also the rung with least to
give.

*Stated as measured-today, not as a ceiling:* the milestone's full audio requirement
(voices, announcer, crowd) is not all present yet, and BGM streaming I/O may sit in the
asset-I/O lane rather than in `AUD`.

### Rung 2 — visual fidelity: 44,476–50,203 tk/fr, 0.471x–0.531x. Does not close.

| item | tk/fr | **x of +94,481** | what the owner would be accepting |
|---|---:|---:|---|
| Stage no-Z band | 22,608 | **0.239x** | the depth-disabled stage scenery band. Flat at exactly 1.00x, so it converts 1:1 |
| — sub-rung `NDS_DREAMLAND_CARD_CULL` | ~4,600 | 0.049x | 20.6% of stage scenery instead of the whole band |
| Particle draw kernels | 12,595 | **0.133x** | fewer/cheaper particles. **This is the floor and the honest number**; an older −30,676 is retracted and must not be shown |
| Draw-side fixed point | 9,273–15,000 | **0.098x–0.159x** | draw-side precision (see below) |
| **total** | **44,476–50,203** | **0.471x–0.531x** | |

**Both visible rungs together no longer close the gap.** Two banks ago they did,
comfortably. What the owner is being asked for has changed from "accept one visible change
and we are done" to "accept every visible change and be 47–53% of the way".

### Rung 3 — gameplay fidelity: a projected 55,473–84,451, 0.587x–0.894x. Does not close.

The soft-float class is **168,060 tk/fr** at rank-80. **80.2% of it — 134,720 tk/fr — is
simulation or shared** (`shared` 57,521, `sim-only` 39,537, `sim+dispatch` 37,662),
classified from the linked ELF rather than from symbol names. Converting it to fixed point
changes physics, collision, RNG, animation and status timing numerically, which is a
gameplay-fidelity change.

Projected at the two exchange rates that have actually been measured in this binary:

```text
at 1.70x (camera chain, measured in situ)   134,720 x (1 - 1/1.70) = 55,473   0.587x
at 2.68x (collision narrow phase, measured) 134,720 x (1 - 1/2.68) = 84,451   0.894x
```

**This is a projection, not a measurement** — both rates were measured on different symbol
sets, and the lane's transcendental half is known to convert far worse than its arithmetic
half. Neither end closes alone. Its failure mode is also the worst on the board: a
numerically different collision still looks like a match.

### The two live decisions, and how they interact with the rungs

**(a) `BLOCKED(decision: draw-side precision)` — this IS rung 2.** It is not a separate
question.
- **Yes** → 9,273–15,000 tk/fr (0.098x–0.159x) enters rung 2, of which the camera chain is
  already built and measured at **−4,736 paired median = 0.050x**. And if the answer is a
  general budget rather than a one-off, the particle/quad category (9,015 tk/fr gross, and
  multiply-accumulate shaped, which converts *better* than the camera chain did) is the
  next and better candidate.
- **No** → the whole 34,178 tk/fr draw-side soft-float lane closes, and rung 2 drops to
  **35,203 = 0.373x**.
- **The number the owner should have in front of them:** the camera arm's pixel delta is
  **6.5350% / 3.6325%** of the top screen at two simulation-clock locks — against a
  **same-build adjacent-present floor of 35.2217% / 37.7983%** on the same crop. That is,
  the change is *smaller than the difference between two consecutive frames of the
  unmodified build*. Structurally identical picture, speckle over textured surfaces. A
  playable both-arms ROM exists: `builds/build-c205-camtoggle/smash64ds-battle-playable-proof-hwtri.nds`,
  **SELECT** (Right Shift in the repo melonDS) flips arms mid-match, indicator on the
  bottom screen.

**(b) `BLOCKED(decision: GX_COMPOSE default)` — this is NOT a rung and has no performance
content left.** §3 settles it: two independent build pairs, the flag deletes nothing
resolvable. Its only remaining consequence was basis hygiene — whether the campaign's
numbers are measured on the renderer that ships — **and this cycle removed that by
re-quoting every ranking document at `GX_COMPOSE 0`.** Flipping the default to 1 would
cost the 0.0358–0.1742% pixel delta the owner already accepted and buy nothing measurable;
leaving it at 0 costs nothing and is now consistent everywhere. **It no longer blocks
anything.**

---

## 6. The arithmetic, in one place

Against **+94,481**, with no double counting (each row is a distinct symbol set) and at
**100% conversion**, which is not on offer for any row:

```text
fidelity-neutral, available today                30,480   0.323x
fidelity-neutral, gated on one counter each      22,735   0.241x   SUPERSEDED 2026-08-16
                                                -------
FIDELITY-NEUTRAL TOTAL                           53,215   0.563x   -> +41,266 remains

  CORRECTED 2026-08-16 (section 2) -- the counter existed and had been read:
    available today                              30,480
    BANKED this cycle, tile-sync memo              3,648   shipped and measured
    bind revisit reordering, ceiling               2,484   unbuilt
    bind text ITCM placement, ceiling              5,754   unbuilt, 3,802 port-reachable
                                                -------
  CORRECTED FIDELITY-NEUTRAL TOTAL               42,366   -- 10,849 of the 53,215 was
                                                             never there

rung 1  audio fidelity                            3,040   0.032x
rung 2  visual fidelity                     44,476-50,203 0.471x-0.531x
                                                -------
NEUTRAL + RUNGS 1 AND 2                    100,731-106,458 1.066x-1.127x

rung 3  gameplay fidelity (projected)      55,473-84,451  0.587x-0.894x
rung 4  original 60 Hz simulation               291,488   3.085x
```

**Read that top block first.** Everything that costs nothing, plus the entire audio rung,
plus every visible change on the board, reaches the gate only if all fourteen items convert
at 100% — including two that no one has yet written a counter for, one that is measured
unable to reach 100%, and three that need owner acceptance of a permanent visual loss.

**Rung 4 closes on its own with 3× margin, and it is the only thing on this board that
does.** That margin is the useful part: it means the question is not "60 Hz or 30 Hz" but
"which subset of the simulation moves to 30 Hz", and §4 shows subsets that close while
leaving the AI and the interrupt/physics half at 60 Hz.

---

## 7. What this cycle did NOT do

- **Nothing was chosen.** No default flipped, no cadence changed, no source edited. The
  sacrifice-order call is the owner's under `PROJECT_GOAL.md` rail 5 and is not made here.
- **No compensation cost was measured.** §4's 291,488 is a ceiling from an exact re-rank of
  measured rows; an implementation adds work back and none of that is priced.
- **The two counter-gated items still have no counter** — `ndsRendererSyncTextureTile` and
  the texture-bind collapse remain 0.241x of *unsized* volume, not 0.241x of proven waste.
- **The `GX_COMPOSE` STG residual was bounded, not mechanised.** §3 shows the stage does no
  extra work and that the rise does not reproduce; it does not separate cross-build
  placement from a GX queue effect, and says so.
- **No pixel capture was taken**, no ROM published, both root ROMs byte-unchanged.

## 8. Reproduction

```powershell
# the two arms, one config line apart; build back to back with no repo write between
# them (the git dirty-count is baked into the binary)
make TARGET=smash64ds-battle-playable-tickhud-hwtri BUILD=build-c208-t103-gx0 `
    NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1 `
    NDS_TASK103_STAGE_RUN_PHASE=1
make TARGET=smash64ds-battle-playable-tickhud-hwtri BUILD=build-c209-t103-gx1 `
    NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1 `
    NDS_TASK103_STAGE_RUN_PHASE=1 NDS_R2_FIGHTER_GX_COMPOSE_LAB=1
# pwsh 7 only
pwsh -File scripts\sample-tick-hud-buckets.ps1 -Build build-c208-t103-gx0 -NoBuild `
    -RingDump -Samples 1600 -StartFrame 438 -TimeoutSeconds 3600 `
    -ExtraGlobals <the 18 gNdsTask103* scalars + 4 invariants> `
    -RowsCsv ...\c208-t103-gx0-rows.csv -JsonOut ...\c208-t103-gx0.json
```

Sections 2, 4 and 6 need **no build at all** — they are exact arithmetic over
`../2026-08-16_gxcompose-bank-basis/c206-shipgx0-rows.csv`, which is already on disk.
