# P1 Execution Board

Updated: 2026-08-05 (cycle 80). Boundary: `battle_playable_realtime`, mode `163`.

This board was rewritten from a 10,207-line append log into a queue. Every
verdict, baseline, and instrument note carried forward unchanged; the full
pre-rewrite text is `docs/optimization/archive/P1_EXECUTION_BOARD_pre-cycle79.md`.
Closed work goes to that archive (append a dated section), not back onto this
board. The charter is `docs/Smash64DS_Runtime2_SwitchPlan.md`; measurement and
workflow rules live in `docs/VERIFYING.md`.

## THE MATCH LENGTH RULE (owner, 2026-08-05) — read before banking anything

> "the soak was only meant to catch freezes, boundary and both cpu gates should
> be the 60 sec match"

**Both gate arms run the one-minute match. LANDED, cycle 80.** The `time_limit
= 7` is gone from `scene_harness.c`'s `NDS_R2_BOTH_CPU` branch, and the soak's
long match now lives on `NDS_R2_SOAK_MATCH_MINUTES` (Makefile, default 0 =
canonical one-minute match). `soak-freeze-watch.ps1` derives that value from its
own `-MinutesToRun` instead of a constant, so the match can no longer be shorter
than the run watching it, and it *reads the seeded value back out of the guest*
at end of run rather than trusting the flag. Proven end to end: resolved 2 →
generated header `NDS_R2_SOAK_MATCH_MINUTES 2` → in-guest `time_limit = 2` with
`gNdsVSResultsStartCount 0` over 2,705 presented battle frames, i.e. the soak
spent none of its run on a Results screen.

Both arms were re-banked on the corrected seed and **now measure identically**:
match timer 1 min, clock 52 s → 0 s, coverage 86.7%, logic:presented 2.000.

**A window is "whole match" only if its coverage was measured against the match
clock.** Coverage is part of a baseline's identity now, not a footnote. The
conversion: the sim runs 60 Hz and presents 30 Hz, ratio **measured at exactly
2.000**, so 1,600 presented = 3,200 logic = **53.3 s**.

## THE LEVEL IS +65,297: THE CAMERA SHIPS IN Q20.12 (2026-08-16) — `artifacts/performance/2026-08-16_camera-ship/CAMERA_SHIP.md`

**New basis `build-c220-camship`: rank-80 1,210,624 raw / 1,185,677 net, band
41–120 1,218,356.** 1 lab build, 4 gate runs, Boundary green, 0 `Exception:`,
root ROMs byte-unchanged, nothing published.

**THE OWNER ACCEPTED THE DRAW-SIDE PRECISION ARM** — *"I think camera fixed
point is ok"*, after playing `build-c205-camtoggle` and saying of the picture
*"otherwise it looks fine"*. `NDS_R2_CAMERA_FIXED ?= 0 → 1`, still
build-overridable. That closes `CAMERA_Q20_12.md` §8's
`BLOCKED(decision: draw-side precision)` and sets this project's **first
draw-side pixel ceiling**: 6.5350% of the top screen at a simulation-locked tic,
against a same-build adjacent-present floor of 35.2217%.

**PRICE −6,336 tk/fr, SAME BINARY, ZERO PLACEMENT FLOOR** (`build-c220-camship`,
one poked `.data` word, `romSha256 5AE3F716…` on both arms):

| | route 0 (float) | shipped default | Δ |
|---|---:|---:|---:|
| paired median, whole run | — | — | **−6,336** (93.3% win) |
| paired median, marginal-80 | — | — | −7,264 (69/80) |
| rank-80 | 1,219,520 | **1,210,624** | **−8,896** |
| ranks 41–120 | 1,223,781 | 1,218,356 | −5,425 |
| complement | `WORK-H −6,336 / WAIT +6,272 / ALL +0` | | |
| attribution | `FTR −4,416` (99.4% of frames), `STG −1,856`; `SRC`/`SINT`/`GCRA` **0** | | |

**+64,977 → +65,297 is +320 and it is PLACEMENT, not a regression.** This
binary's own float arm reads **+74,193** on the same window — **+9,216 against
`c219`'s float arm for identical float behaviour**, the ≥14,080 cross-build floor
re-measured on this pair. **The route is the price; this is the level.**

**The last two leading polls came out with the flip** — all sixteen now gone from
the binary. Isolated by a second same-binary route on the previous basis (flip
alone: −3,776 paired median / −3,072 rank-80), difference of differences
**−2,560** against **2,721 predicted before the run** from `HWROUTE.md` §2's
per-call figures and this cycle's engagement counts. Right sign, 94% of size, but
it carries a residual cross-build term inside the ~5,440 paired-median floor —
**corroboration, not a price.**

**Engagement, shipped arm: 8,152 fixed look-ats, 8,228 fixed perspectives, FLOAT
CALLS ZERO** (a build default has no pre-poke frame). `Saturate`/`Degenerate`/
`Rescale` **0** on all four arms.

> **THE TASK 9 STATE HASH IS THE WRONG INSTRUMENT AND `CAMERA_Q20_12.md` §9.4'S
> "a default flip must measure it first" IS RETIRED.** `Makefile:1683` already
> settled the category: the hash asserts bit-exactness, this change is authorized
> NON-bit-exact, so it can only ever report "differs". It is `?= 0` and no
> verifier references it. The instruments that could have failed and did not are
> the ELF caller classification (`draw+dispatch` 100.0%), the flat animation and
> simulation buckets, and Boundary.

> **THE DRAW-SIDE SOFT-FLOAT SPLIT RE-BASES — DO NOT QUOTE THE OLD ONE.**
> shared 57,521 / sim-only 39,537 / sim+dispatch 37,662 / **draw 34,178** is
> stale on the draw row only (the camera is `draw+dispatch` 100.0%). The chain's
> **11,504 tk/fr whole-match gross** is no longer called, leaving ≈**22,521**. A
> precise re-derivation needs a new per-PC census — the existing one is
> `build-c200-trackprof-off` at `GX_COMPOSE=1`. Quoting a stale split is what
> made `MENU.md`'s 94,602 wrong for fourteen cycles.

## THE FIGHTER DRAW CONTRACT IS NEARLY STATIC — 51 CHANGES IN 4,076 CAPTURES, AND THE SOUND MEMO IS 19,058 (2026-08-16) — `artifacts/performance/2026-08-16_ftr-capture-memo/CAPTURE_MEMO.md`

**RED, unowned, and the largest sized item on this board.** 1 lab build
(`build-c222-ftrcensus`), 1 whole-match run, 1 Boundary. **The level is unmoved
at +65,297**; `size.py` re-derives rank-80 1,210,624 / net 1,185,677 / band
41–120 1,218,356 from the basis rows before printing any result.

**Predicted 70% unchanged, band 55–85%. Measured 98.75% — outside my own band.**
`PREDICTION.md` was written before the ROM existed. `FTR_LANE.md` §5 left the
capture pass at 34,307 tk/fr as a *ceiling* precisely because this rate had
never been measured.

```text
Captures 4,076   Same 4,025   ChangeTotal 51   MaxRun 848   ZeroEvents 164
CountSame 4,064  DObjSame 4,064  DLSame 4,064  PreSame 4,025
KeySame 4,049    KeySameContractDiff 49        KeyDiffContractSame 25
EventTotal 62,920            BoundsPass 3,823  BoundsFail 142
```

**The instrument check could have failed and did not:**
`62,920 / (4,076 − 164) = 16.08` events per fighter against Boundary's
independent `ftrContract=6784/6784` = 16.0, and `4,076 + 2` bootstraps
`= 2 × 2,039` presented frames exactly — the census saw every frame.
In-window (per-stop deltas, frames 535–2039): **3,008 comparisons, 45 changes =
98.50%**, stationary (worst 96-frame window 8, five windows 0–1).

**What changes.** The populations nest: **51 preamble changes, 12 of them also
structural**, and those 12 move count + dobj + dl *together* — the whole event
list appearing or disappearing, not a part swap. `event_count` cannot move
without moving both hashes, so containment plus equal cardinality (4,064 three
times) forces the three sets equal: **zero DL-only changes.** The
12 are accounted for by the 164 zero-event captures, of which `BoundsFail = 142`
are `ftDisplayMainProcDisplay`'s off-screen magnify early return
(`ftdisplaymain.c:1131-1152`) and the rest its `is_invisible` return at `:1088`.
**Every one of those decisions is taken in the head, before the tree walk.**

**REFUTED, TWICE — do not build a DObj-tree-keyed memo.** The census hashed the
obvious key (per DObj: `flags`, `dl`, `dv`, `dls`, `dls[0..1]`, `FTParts` flags
— exactly what `ftdisplaymain.c:753-841` branches on) and it reads
`KeySameContractDiff = 49`: **unchanged on 49 of the 51 frames the contract
changed.** It is also expensive — FTR P50 290,432 → **301,120**, +10,688, a
cross-build pair against a ~5,700 P50 floor, so an order of magnitude rather
than a banked price, and the shape a walk that adds `dls[0]`, `dls[1]` and
`parts->flags` chases to every node predicts. That +10,688 is the engagement
proof and the reason **this ROM must never be read for ticks.**

**CORRECTED — the sound ceiling is 19,300, not 34,307.** The capture pass is not
read-only: `ndsBaseFTDisplayMainProcDisplay` **is** decomp's
`ftDisplayMainProcDisplay`, and its head writes the off-screen player arrow HUD
(`fp->is_magnify_show`, `fp->magnify_pos`,
`gIFCommonPlayerInterface.magnify_mode`, `ifCommonPlayerArrowsUpdateFlags`),
`gLBCommonScale`, the fog statics and the scene light twice — and
`BoundsFail = 142` proves that branch fires in the canonical match. The target
is the walk, `ftDisplayMainDrawAll → ftDisplayMainDrawDefault`, inclusive
**19,300**.

**SIZED** (uniform D on the basis's own 1,600 rows; conversion **1.000**
throughout because `FTR` band/P50 = 1.000, so D *is* the rank-80 move):

| candidate | D | rank-80 | level |
|---|---:|---:|---:|
| ≥14,080 cross-build floor, for reference | 14,080 | 1,196,544 | +51,217 |
| `CountFlags` deleted — diagnostic-only, no memo needed | 4,117 | 1,206,507 | +61,180 |
| **WALK memo at the measured 0.9875 — SOUND, recommended** | **19,058** | **1,191,566** | **+46,239** |
| WALK memo + `CountFlags` deleted | 23,175 | 1,187,449 | +42,122 |
| whole-capture memo at 0.9875 (needs the head proven) | 33,877 | 1,176,747 | +31,420 |

**19,058 is 29.2% of the requirement and clears the floor by 35%; 23,175 is
35.5%.**

**FOUND IN PASSING: `ndsFighterDisplayContractCountFlags` is 4,117 tk/fr of
recursive tree walk whose only outputs are two debug counters.**
`gNdsFighterDisplayContractHiddenCount` / `…NoTextureCount` are written in
`reloc_backend_renderer_dl.c`, reset in `taskman_seam.c:3148-3149`, and read
only by `verify-battle-mariofox-gcrunall-loop-harness.ps1:2065` and
`probe-ko-vfx.ps1` — **neither in Boundary.** Free once the walk is memoised.

**THE DESIGN, SPECIFIED NOT BUILT — the key is the head's own output.** When
`ftDisplayMainDrawAll` is reached the head has already written every scalar that
decides the preamble into `sNdsFighterDisplayContract` (`geometry_mode`,
`cycle_type`, `render_mode`, `prim_color`, `env_color`, `light`, `light_valid`,
`light_count`) and has already taken its early returns. Comparing those ~10
words costs a compare, not a walk, and covers **51 of 51** observed changes.
Redirect through the shim — `battleship_ftdisplaymain.c` already renames
`ProcDisplay`, and `ftDisplayMainDrawAll` has exactly two call sites, both
inside it — **not a decomp edit**.

**The one unproven piece has a cheap sound answer.** Zero DL-only changes is one
match, not an invariant; the fighter joint DL writers are a closed set in one
file (`decomp/…/ft/ftparam.c:780,794,813,863,873,887,934`; every other `->dl =`
in the tree is items, weapons or stage), so a one-line dirty set there makes the
key sound by construction — the `bind-where-broken-not-where-read` pattern
already shipped for `gMPCollisionGeometry`.

**Not built, and the reasons are sizes rather than taste:** the cache is ~2.6 KB
of bss against a heap low-water already under the GObj-cap threshold; the sound
key needs the `ftparam.c` overlay patch, a second seam with its own
`check-decomp-pristine.ps1` obligation; and the item's size *and* shape both
moved inside this cycle. **Next cycle builds the corrected design.**

**Not done:** the 39 preamble-only changes are not attributed to a specific
field (all head-decided either way); `gmCameraLookAtFuncMatrix`'s 5,143 rider is
untouched; `STG` is untouched. The census source is behind
`NDS_R2_FTR_CONTRACT_CENSUS ?= 0`, so the shipping binary is unchanged.

## THE PER-JOINT COLLISION SETUP IS NOT REDUNDANT — THE RENDERER *CALLS* IT — AND AT ZERO COST IT IS 10,110 AT RANK-80 (2026-08-16) — `artifacts/performance/2026-08-16_collision-setup-share/SETUP_SHARE.md`

**0 lab builds, 0 emulator runs of my own, 0 production source edits, 0 defaults
flipped, nothing published, nothing re-banked, both root ROMs byte-unchanged
across Boundary's own rebuild. The level is unmoved at +65,297.** `lanes.py`
prints the control before any result and needs no emulator.

**ANSWERED — the redundancy does not exist, and the sharing already does.** The
renderer does not build a competing fighter joint matrix; it **consumes** the
collision chain's. `ndsRendererAdapterBuildJointAttachMtx`
(`src/port/reloc_backend_renderer_dl.c:1464-1499`) **calls `func_ovl2_800EDBA4`**
and then quantises `parts->mtx_translate` float → 16.16 → 20.12;
`ndsRendererAdapterBuildFighterPartsMtx` (`:1247-1306`) reads
`parts->unk_dobjtrans_0x10` gated on the same `transform_update_mode` latch. One
producer, two consumers, memoised by the source's own four dirty flags
(`gmcollision.c:29-79`, `:208-225`, `:228-278`, `:332-452`, `:455-469`). **The
memo this question was looking for IS the dirty flag**, and
`BAND_OWNER.md` §4's *"the renderer builds every joint's world matrix each frame
in 20.12"* describes a producer that does not exist.

**And the direction is the other way.** `func_ovl2_800ED490` is entered **22.77
times on an engaged frame against 0.76 on a control frame**
(`SHDT_MECHANISM.md` §3.1's 1,434.7 / 47.9 leaf calls ÷ a source-checked 63.01 per
invocation), so the renderer drives under one compose a frame and hit detection
drives all 22.01 of the rise. **The frame has not already built these matrices
when hit detection asks — it builds them because hit detection asks.**

**SIZED — and it would not matter if it had.** The whole per-joint setup (local
matrix + world compose + 3×3 cofactor inverse + axis scales + their trig =
**70,660 tk/fr on the 57 engaged frames, 68.6% of the chain's 102,988**; the
excluded 26,617 are the *consumers* `GetWorldPosition` and `TestRectangle`, which
`SHDT_MECHANISM.md` §2 already showed are the flat half) is **f = 0.245 of the
`SHDT` excursion**:

| intervention | rank-80 | moved | band 41–120 | level |
|---|---:|---:|---:|---:|
| *(control)* | 1,210,624 | — | 1,218,356 | **+65,297** |
| the whole per-joint SETUP free | 1,200,514 | **10,110** | 1,197,943 | **+55,187** |
| the whole CHAIN free | 1,191,670 | 18,954 | 1,188,160 | +46,343 |
| the whole `SHDT` bucket free | 1,137,088 | 73,536 | 1,142,335 | −8,239 |

**10,110 is 15.5% of the requirement and under the ≥14,080 cross-build floor**,
and the estimate is not load-bearing: 8,780 at f=0.15 … 10,110 at f=0.245.
Reaching even the floor needs f ≈ 0.30 — more than the whole measured chain.

> **Two configuration facts, so nobody re-derives them.**
> `builds/build-c220-camship/nds_build_config.h` reads **`NDS_R2_SIM_MAC_SHADOW 0`**
> and **`NDS_R2_COLLISION_FIXED 0`**, so the shipping ROM runs the decomp bodies
> **unwrapped** (c191's attribution needs no apparatus correction) and
> `ndsR2CfxPrepareFighterJoint` (`src/port/nds_r2_collision_ring.c:253-323`) —
> already a fixed-point implementation of exactly the inverse and the axis
> scales — is compiled out. `EXCHANGE.md`'s 2.68 exchange rate is **not
> reopened**. One consequence: `parts->mtx_translate` is also what the renderer
> quantises, so a fixed-point `func_ovl2_800ED490` would move **drawn geometry**
> as well as hits — `SHDT_MECHANISM.md` §4.2's declined route is *more* blocked.

**CORRECTED — `SHDT` is not the largest single row at lane level.** Exact re-rank
of every leaf on this basis: **`SITR` excess 86,528 (level −21,231)** against
**`SHDT` 68,928 (−3,631)**. `SITR` is larger and is the only lane whose excess
alone closes the gate. The "`SHDT` 50,240 > `SITR` 45,056" ordering below is the
**cluster** rows — dominant-excess-owner subsets of the top 80 — which is a
different quantity from the lane. Both are ceilings; both lanes' levers are
already refuted.

**NEW — the median frame passes the gate by 218,767, so the whole requirement is
excursion.** The sixteen lane medians sum to **926,560** against a raw gate of
**1,145,327**. The smallest *proportional* cut of a lane that closes +65,297:

| lane | P50 | band 41–120 | × P50 | **f to close** | EXCESS moved → level | WHOLE moved |
|---|---:|---:|---:|---:|---:|---:|
| `SITR` | 104,320 | 210,060 | 2.01 | **39%** | **86,528** → −21,231 | 184,704 |
| `SHDT` | 4,608 | 82,239 | 17.85 | 80% | 68,928 → −3,631 | 73,536 |
| `SPHD` | 72,288 | 102,233 | 1.41 | 69% | 31,776 → +33,521 | 99,584 |
| `MISC` | 107,872 | 137,152 | 1.27 | 49% | 26,368 → +38,929 | 133,760 |
| `GCRARES` | 81,632 | 95,895 | 1.17 | 67% | 13,600 → +51,697 | 95,232 |
| **`FTR`** | **290,432** | 290,400 | **1.00** | **22%** | 4,224 → +61,073 | 294,528 |
| `STG` | 175,424 | 177,477 | 1.01 | 38% | 2,624 → +62,673 | 178,048 |
| `SCPU` · `SPRM` · `AUD` · `OTHRW` · `SCAT` · `SWRM` · `SRCRES` · `SPHC` · `BG` | — | — | — | **never** | ≤10,880 | ≤44,352 |

**Nine of sixteen lanes cannot close the gate even deleted entirely.** `FTR` is
the cheapest at 22% because it is dead flat (1.00× at the band) and therefore
converts **1:1** (`[[a-flat-lane-is-the-best-converting-lane]]`). It is the run's
largest lane and **has never been attributed per-PC in the shipping
configuration**; `v3-c221` (`GX_COMPOSE=0`, 1,600 frames) is the capture that
would do it **with no build**. That is the next cycle's cheapest discriminating
read.

## THE ATTACH LANE DOES NOT CONVERT — 72,768 IS 13,376–37,027 AT RANK-80, AND ITS PARSE/EVALUATE HALF IS THE TRANSITION'S OWN SECOND ANIMATION PLAY (2026-08-16) — `artifacts/performance/2026-08-16_sitr-attach-lane/ATTACH_LANE.md`

**0 builds, 0 emulator runs, 0 production source edits, 0 defaults flipped,
nothing published, both root ROMs byte-unchanged. The level is unmoved at
+65,297.** Everything below is re-derived from artifacts already in the tree plus
BattleShip source; `convert.py` and `outside.py` need no emulator and reproduce
the control (`rank-80 1,210,624 / +65,297`) before printing any result.

**CORRECTION — the 72,768 below is not a size an implementation can deliver.** It
is the re-rank of clipping every event frame's `SITR` back to the run median, a
per-frame **variable** that reaches 227,968 on the cluster. The mechanism the
section below names is 78,708 tk/fr as a **mean over the 288**. Re-ranking a
*uniform* saving `D` on those frames — which is what an engineering change
produces — gives, capped at each frame's own excess / uncapped:

| candidate (size on the 288) | D | capped | uncapped |
|---|---:|---:|---:|
| `ndsRelocAssetIDForToken` | 4,118 | **3,542** → +61,755 | 3,542 |
| ATTACH chain group | 23,801 | 10,496 → +54,801 | 11,968 |
| ANIM evaluate group | 26,813 | 10,496 | 14,845 → +50,452 |
| ANIM parse group | 28,094 | 11,134 | 16,126 → +49,171 |
| parse + evaluate + attach | 78,708 | **13,376** → +51,921 | **59,520** → +5,777 |

Conversion is **not monotone**: 0.860 at D=4,118, 0.435 at 20,000, 0.244 at
54,907 capped. Small savings convert best because they bite on the frames just
above rank-80; large ones saturate as the event population sinks below the 1,312
frames the lever cannot reach. **Size every candidate on `convert.py` before
building it.** Only 50 of the 288 sit at or above rank-80.

**ANSWERED — the parse/evaluate work is NOT redundant across joints, so there is
no memo here.** Per joint it is flat or cheaper on an event frame:
`ndsR2AnimValueQ` **0.98×/call**, `gcPlayDObjAnimJoint` **0.91×**,
`ftParamUpdateAnimKeys` **0.98×** (self time ÷ that function's own entry-PC count,
same capture). What runs 1.52× is the **whole animation play**, because
`ftMainSetStatus` calls one itself — `decomp/…/src/ft/ftmain.c:4787-4795`, both
arms reach `ftMainPlayAnim` → `ftParamUpdateAnimKeys`. `ftMainSetStatus` 1.22/fr
against `ftParamUpdateAnimKeys` +1.94/fr = **1.59 extra whole plays per
transition**, and extra plays are **89.7%** of the parse-call growth. That is
**100% of the evaluate group** and **~49% of the parse group**. The other ~51% of
parse is `ndsR2FtAnimParseDObjFigatree` at **1.62× per call**, because a fresh
`AOBJ_ANIM_CHANGED` attach must consume the clip's first event block instead of
early-outing (`ParseStepped` 2.27× against `ParseEarlyOut` 1.02×). **Both halves
are the transition itself, and the second play is not a repeat — the clip was
attached between the two.**

**`BLOCKED(decision: transition-frame animation play)`.** Suppressing or deferring
`ftMainSetStatus`'s own play is D = **49,251 tk/fr** on the 288 = rank-80
**13,376 (capped) to 37,027 (uncapped)**, level +51,921 to +28,270. It is the
only item in this lane whose size clears the ≥14,080 cross-build floor, and it
does not close the gate alone. It is a gameplay change, not a representation
one: the play establishes the new status's pose on the transition frame and
`ftMainRunUpdateColAnim` runs on the next line against those joints. Three
options are priced in `ATTACH_LANE.md` §4; **none is recommended.** The attach
chain itself (+23,801) is the same question about the same transition.

**REFUSED — `ndsRelocAssetIDForToken` is a small load-frame cut its own file
already forbids.** +4,118 tk/fr on the 288 = **3,542 at rank-80** (5.4% of the
requirement), under the ≥14,080 floor. `src/port/reloc_backend_assets.c:1876-1921`
records two measured failures on this exact function — Task 74's memo (`STG`,
which a token lookup cannot touch, moved 8,128) and R2-06 E11's hoist (**negative
bytes added**, function −7,667, load-frame set bit-identical, and `WORK-H` P95
still **+15,744**) — and sets the bar: *"Do not bring another small load-frame
cut. Either remove this work in one change large enough to clear ~16,000 of tail
movement, or move it off the gameplay frame entirely."* A runtime table is also
~1.5 KB of main RAM (keys are link-time addresses, so it must be built at run
time) against `[[ram-is-not-free-gobj-cap]]`.

> **CLOSED 2026-08-16 — the second clause is NOT an open route, because it is the
> same 3,542.** Moving the resolver off the gameplay frame removes 4,118 tk/fr
> from the 288 event frames, which is *arithmetically identical to making it
> free*: rank-80 is an order statistic over the window's 1,600 **gameplay** frames
> 439–2038, and a match load sits outside it. `convert.py` prints **one row for
> both — 3,542, level +61,755**, 22% of the file's own ~16,000 bar and 25% of the
> ≥14,080 floor. The clause names a **mechanism**, not an exemption from the first
> clause's **size**, and the failure it was written from was a **measurement**
> failure the mechanism does not touch — E11 was provably identical, added
> *negative* bytes, cut the function 7,667, and still read P95 +15,744. A ~1.5 KB
> runtime table makes this change **byte-positive** where E11's was byte-negative.
> **Not built.** Correct to take only if it ever rides along with something large,
> exactly like the `AObjToQConvert` store below.
> (`artifacts/performance/2026-08-16_collision-setup-share/SETUP_SHARE.md` §3.)

**Also sized and refused: `ndsR2AnimAObjToQConvert`'s `nGCAnimKindNone` arm**
(`battleship_ftanim.c:362-369`) writes `length_invert = Q(1.0)` and returns
without changing `a->kind`, so the `kind >= NDS_R2_AQ_KIND_BASE` early-out never
catches it and the same constant is re-stored on every `BuildTrackTable` while the
AObj stays `None` — ~209 armed per event frame by the decomp
`gcAddDObjAnimJoint`'s chain reset, matching the measured 208.99 calls/frame.
Provably idempotent, and worth **298 ticks at rank-80**. Hoist it into the inline
wrapper only if it ever rides along with something large.

**INHERITED — 30 of the top-80 frames carry neither an attach nor a force-load,
and `SHDT` owns 22 of those 30** at 219,616 against a run median of 4,608
(**47.66×**); their `SCPU` is 1.99×. Across the whole top-80 `SHDT` owns **32**
frames and `SITR` **25**. Of the 118 frames over the gate, **41 carry no event at
all.** This agrees with the section below's own §1 table, which already put the
`SHDT` cluster at **50,240 moved / level +15,057** — larger than `SITR`'s 45,056
and the largest single row in it. Not opened here.

> **ANSWERED 2026-08-16, and the "largest single row" reading is corrected.**
> Those are **cluster** figures (dominant-excess-owner subsets of the top 80). At
> **lane** level on the same basis, `SITR`'s excess is **86,528** (level −21,231)
> and `SHDT`'s is **68,928** (−3,631), so `SITR` is the larger of the two. The
> follow-up question this paragraph implied — *is the per-joint hit-detection
> setup redundant with work the frame already does?* — is **no**: the port's
> renderer **calls** `func_ovl2_800EDBA4` and consumes `parts->mtx_translate`
> rather than producing a rival matrix, and of the 22.77 composes on an engaged
> frame hit detection drives 22.01 (a control frame runs 0.76). At zero cost the
> whole setup is **10,110 at
> rank-80**, under the floor. See the `SETUP_SHARE.md` section at the top of this
> board.

## `SITR` IS A CALL-COUNT EVENT, NOT A COST — 288 FRAMES, 72,768 AT RANK-80 (2026-08-16) — `artifacts/performance/2026-08-16_sitr-excursion/SITR_EXCURSION.md`

> **SUPERSEDED IN PART (2026-08-16, the section above).** The 72,768 headline is a
> ceiling on clipping a per-frame variable, not on the mechanism this section
> names; re-ranked as a uniform saving it is 13,376–37,027. The three candidate
> routes in its §5 are re-priced there: the resolver is refused on the floor, the
> per-joint quantisation is 298 at rank-80, and the re-attach question is now
> stated as `BLOCKED(decision: transition-frame animation play)` with its size.
> Everything else in this section stands and was used unchanged.

**1 lab build (`build-c221-sitrprof`), 1 v3 capture, 2 counter runs on the
existing basis ROM. 0 production source edits, 0 defaults flipped, nothing
published, both root ROMs byte-unchanged. The level is unmoved at +65,297.**

**THE INHERITED CLUSTER FIGURES RESTATE ON THE CURRENT BASIS.** `IO_AUDIT.md`
§5's `SITR` 27 frames / 231,264 / **−51,200** were measured on
`build-c219-animitcm-ship`. Re-derived on `build-c220-camship` by both of its
methods, with the leaf closure exact on all 1,600 rows and rank-80 reproducing
1,210,624 / +65,297: **25 frames, median own excess 227,968, median `SITR`
332,288 against a run median of 104,320 = 3.19×, exact re-rank 45,056 → level
+20,241.** Cluster sizes are now `SHDT` 32 / **`SITR` 25** / `SPRM` 8 / `SPHD` 8
/ `MISC` 4 / **`AUD` 3** (new). **Quote 3.19× for `SITR`, not 57×** — 57.69× is
`SHDT`'s, whose run median is 4,608.

**THE MECHANISM, from the first per-PC census ever taken in the SHIPPING
configuration** (`build-c221-sitrprof`; its `nds_build_config.h` differs from the
basis in **four lines**, all of them the profiler or the git string —
`config-diff.txt`. `GX_COMPOSE 0`, `FTANIM_TRACK 0`, `CAMERA_FIXED 1`. Every
prior census — c200, c191, c192 — was `GX_COMPOSE=1` and/or `FTANIM_TRACK=1`, the
caveat `ANIM_ITCM.md` §2 and `ITCM_CENSUS.md` §1 both had to carry.)

Entry-PC call rates, per frame, on the 288 attach/force-load frames against the
other 1,313:

| symbol | event-288 | rest | ratio |
|---|---:|---:|---:|
| `ftMainProcUpdateInterrupt` (the `SINT` root) | **4.00** | **3.87** | **1.03×** |
| `battleship_ftMainSetStatus` | 1.22 | **0.00** | 798× |
| `lbCommonAddFighterPartsFigatree` | 1.22 | **0.00** | ∞ |
| `ndsRelocAssetIDForToken` | 2.61 | **0.00** | ∞ |
| **`gcAddDObjAnimJoint`** | **22.06** | 0.35 | **62.4×** |
| `ndsR2FtAnimParseDObjFigatree` | 100.83 | 63.69 | 1.58× |
| `ndsR2AnimValueQ` | 386.15 | 242.05 | 1.60× |
| `ndsR2AnimBuildTrackTable` | 37.05 | 13.09 | 2.83× |
| `ndsR2AnimTargetValue` | 253.20 | 39.80 | **6.36×** |
| `ndsR2AnimAObjToQConvert` | 208.99 | 16.48 | **12.68×** |

> **The bracket's own root is entered 1.03× as often**, so nothing about the
> excursion is "the frame ran more simulation". A fighter changes status;
> `ftMainSetStatus` resolves the new clip through `ndsRelocAssetIDForToken` — a
> ~110-branch linear `if`-chain plus two pointer scans over 143 + 158 animation
> ids, **1,578 ticks per call**, whose own source header already says *"remove
> this work … or move it off the gameplay frame entirely"* — then attaches it
> **joint by joint, 22.06 times**, and the attach re-runs the entire per-joint
> animation pipeline inside the same presented frame.

**THE LEVER: 288 of 1,600 frames (18%) carry an attach or a force-load, they hold
81.2% of the run's whole `SITR` excess, and clearing it is worth 72,768 at
rank-80 — level +65,297 → −7,471, i.e. 1.11× the entire remaining requirement.**
On those frames **only `SITR` moves**: 1.77× against `SCPU` 0.87×, `FTR` 1.00×,
`STG` 1.00×, `SHDT` 1.01×, `GCRARES` 1.03×, `SPHD` 1.08×. Shape per event frame:
**ANIM parse +28,094, ANIM evaluate +26,813, ATTACH chain +23,801**, card I/O
+15,576, memory movers +14,319, soft float +6,028. Ceiling from an exact re-rank,
not an implementation.

| candidate route | size on the 288 | note |
|---|---:|---|
| `ndsRelocAssetIDForToken` — replace the linear chain | **+4,118 tk/fr** at 2.61 calls | pure lookup, cannot change behaviour; smallest and most self-contained |
| per-joint quantisation (`ndsR2AnimTargetValue` + `…AObjToQConvert`) | +6,246 tk/fr | re-derives `arg × 2^-k` from constant `s16` on every attach |
| the re-attach itself (`gcAddDObjAnimJoint` 22.06/fr) | ~half of +54,907 | **gameplay question — BattleShip source must answer whether the new clip must be evaluated in the same logic tick. Not proposed.** |

**REFUTED: "the force-load is the owner."** Over all 1,600 frames the attach
(r=+0.623) and the *stepped* parse (r=+0.650) outrank the force-load (r=+0.487)
and the card read (r=+0.549); **10 of the 25 cluster frames carry no force-load**;
and the implied per-load `SITR` cost still falls with count (+122,931 / +96,777 /
+68,794 for 1 / 2 / 3 loads), reproducing `IO_AUDIT.md` §4 on a new basis.

**RULED OUT, each measured:** the `2^22` artifact (2 corrected samples, frames
1464 and 1849, neither a cluster frame); the HUD refresh (r=+0.058; the 114
HUD frames read `SITR` median 104,384 against a run median of 104,320); the draw
side (`FTR` 1.00× on the 288); extra logic ticks (root entry rate 1.03×).
`gNdsFTComputerStatusChangeCount` and `gNdsR2FtAnimRecipMisses` read **0** for the
whole run and `gNdsFighterStructStatusSetCount` is **never incremented anywhere in
the tree** — three dead counters, named so nobody spends a run on them.

**THE ALIGNMENT WAS RE-MEASURED, NOT INHERITED.** `region = frame − 439`: the 7
card-read frames land at median profile rank **12** of 1,601 there and 315–699 at
every other offset in 434…444, and `r`(profile non-idle, tick-HUD `WORK-H`) is
**+0.694** at 439 against +0.342 at 438. The 7 card-read frames are **456, 830,
1015, 1186, 1625, 1655, 1886** — the identical list `IO_AUDIT.md` §1.1 measured on
`c219`, so the load events belong to the match, not to the binary.

**BY-PRODUCT — the draw-side per-PC census the board asked for now exists.**
`v3-c221` is `GX_COMPOSE=0` at 1,600 frames, which is what a precise
re-derivation of the stale soft-float split needs. Not re-derived here.

**OPEN.** The 4 cluster frames with *no* counter movement (530, 989, 991, 1302)
are diagnosed only to "the simulation, not the draw" — their `SCPU` reads 2.25×
and that is unexplained.

## THE `frsub` EVICTION DOES NOT EXIST, AND THE BIND ITEM IS SEVENTH (2026-08-16) — `artifacts/performance/2026-08-16_itcm-frsub/ITCM_FRSUB.md`

**0 builds, 0 emulator runs for this item.** `nm`/`objdump` over ELFs in
`builds/`, joined to a per-PC CSV already on disk.

**`ITCM_CENSUS.md` §3's unreachability proof STANDS. Its eviction does not.** The
mechanism it names — listing the member in `NDS_TASK9_FLOAT_MAIN_MEMBERS` —
operates on `_arm_addsubsf3.o`, and that member is **one 0x2ac = 684-byte `.itcm`
input section**:

```text
0x000..0x1c8  456 B  __aeabi_frsub / __subsf3 / __addsf3     DEAD, 0 executing PCs
0x1c8..0x2ac  228 B  __aeabi_ui2f / __floatsisf / __aeabi_ul2f / __aeabi_l2f
                     LIVE: 42 executing PCs, 2,396.6 instr/frame whole match,
                     3,544.7 tk/fr on the marginal-80, from zero-wait ITCM today
```

`--rename-section` and the `<stem>.mainram.o` filename both act on the whole
member and **a linker cannot split an input section: 684 bytes move or none do.**
`__aeabi_ui2f` has **99 call sites in 21 functions**; `__aeabi_l2f` is called from
`ndsRendererHardwarePrepareLitDirection`. `build-c221-sitrprof`'s shipping-config
census confirms it: `__aeabi_l2f` **3,016,910 cycles**, `__floatsisf` 876,227,
`__aeabi_ui2f` 456,083, against `__aeabi_frsub` and `__addsf3` at **0**. So the
eviction is not "zero ticks on its own" — it is an unmeasured placement
regression bought to free space. **NOT TAKEN, and it should not be by this
mechanism.** The 456 B is not separable by any tool in the toolchain; the only
route left is hand-authoring replacements for three shared soft-float leaves,
which is refused.

**THE BIND ITEM RANKS SEVENTH ON ITS OWN METRIC.** Ranking every `.main` symbol of
16–800 bytes by marginal-80 `icache_fill` tk/fr per resident byte — the metric
`ANIM_ITCM.md` §6 used — `ndsRendererHardwareBindTextureName` (14.19) sits behind
`__syscall_getreent` 39.20, `DynamicArrayGet` 39.06, **`ftGetStruct` 32.59**,
`ndsStageCollisionLoopGeometryReady` 27.62, `__aeabi_lmul` 26.27,
`get_fat.isra.0` 24.33 and `ndsR2AnimBuildTrackTable` 22.52. `v3-c221`'s own
census §D agrees from the campaign's own tooling. **ITEM B NOT BUILT** — the
cycle's budget went to the item twelve times larger.
(`cpuGetTiming` 70.97 and `tickGetCount` 38.32 are the tick-HUD **apparatus** and
are excluded; moving them changes the instrument, not the product.)

**THE FREE-SPACE ROUTE THAT IS ACTUALLY AVAILABLE, premise now closed.** Four
port-side ITCM residents execute **zero instructions in the SHIPPING
configuration** (`v3-c221/census.txt`, not the old `GX_COMPOSE=1` census):
`ndsRendererNativeEmitDenseRawRun` 256, `ndsRendererNativeApplyStateSpan` 192,
`…EmitProductionRawTexturedRun` 128, `…EmitProductionRawUntexturedRun` 112 =
**688 B**, plus `ndsFTParamsInvalidateFighterParts` 54. One-line
`NDS_TASK82_ITCM_CODE` removal each; none is pinned by
`check-renderer-itcm-placement.ps1`. Instrument free **220 → 908 B**, proof ROM
**2,572 → 3,260**. **NOT TAKEN** — it needs its own build and Boundary.

> **The census's own "+1,858 B recoverable by eviction" over-counts by 456**: that
> figure is symbol-level and includes the frsub blob, which no available mechanism
> can take.

> **`gcPlayDObjAnimJoint` (604 B, 8,218.6 marginal icache) now fits a freed
> budget**, and `linker/nds_hot_text.ld:180-200` prohibits exactly that move on a
> Task 94 verdict taken on a **128-frame window** and as a **cross-build** pair —
> the instrument retired 2026-08-04 and the comparison class with a ≥14,080 floor.
> Recorded as re-testable **by the same-binary route method only**. Not re-opened.

## ITCM IS 44.3% COLD, AND THE FIXED-POINT PLACEMENT OBJECTION RUNS THE OTHER WAY (2026-08-16) — `artifacts/performance/2026-08-16_itcm-census/ITCM_CENSUS.md`

**0 builds, 0 emulator runs.** `nm`/`objdump` over ELFs already in `builds/`
joined to a per-PC CSV already on disk.

**Every ITCM resident censused, not just the one anyone asked about.** 32,180
walked bytes; **14,242 (44.3%) never execute an instruction** in the 1,600-frame
window — 2,448 B in 33 wholly-dead blocks (reproducing last cycle's 2,454 to
within 6 B) and **11,010 B cold inside 52 LIVE blocks**, 4.5× the whole-symbol
pool.

| resident | bytes | executed | **cold** |
|---|---:|---:|---:|
| `ndsRendererScanList` (23.8% of the region, never censused before) | 7,728 | 46.0% | **4,112** |
| `ndsRendererNativePrepareProductionRun` | 3,720 | 43.0% | 2,056 |
| `ndsRendererSubmitHardwareTriangle` | 3,304 | 53.4% | 1,488 |
| `ndsRendererHardwareSubmitVertex` | 2,688 | 57.8% | 1,116 |
| `ndsRendererExecuteNativeFighterOwnerProduction` | 3,508 | 78.9% | 712 |
| **five renderer functions** | **20,948** | | **9,484** |

**None of that is reclaimable by evicting a symbol** — all five are hot, so
eviction pays fetch on their executed half. Reaching it means splitting cold
blocks out of a live function: the largest ITCM reserve in the tree, and the one
that costs engineering rather than a flag.

**`__aeabi_frsub`'s 456 B blob is UNREACHABLE BY CONSTRUCTION, not merely
silent.** Three reads on the current linked image: no branch into it from outside
the blob; **no word in any allocatable PROGBITS section holds any of its three
addresses**; zero executed instructions. Takeable with no reachability argument,
by the `NDS_TASK9_FLOAT_MAIN_MEMBERS` mechanism `ANIM_ITCM.md` §3 built. **NOT
TAKEN** — it buys zero ticks on its own and needs its own build, Boundary and
checker declaration.

> **THE PLACEMENT OBJECTION IS REAL BUT MIS-LOCATED, AND ON THE ANCHOR ROW IT
> RUNS TOWARD FIXED POINT.** From the linked ELF: `guMtxCatF` is at `0x020356e8`
> (**`.main`**) and `ndsRendererMtxMul20p12` at `0x01ff9dac` (**ITCM**) — the
> 5.14× prior's *fixed* arm is the ITCM-resident one, so **5.14× OVERSTATES**
> what a fixed rewrite converts at. **Every other pair has BOTH bodies in
> `.main`**: camera, sim leaf, collision. What is at `0x01ff…` is the soft-float
> *library leaves* a float body calls, not the float body.
>
> The real asymmetry is one level down — a float body's arithmetic runs inside
> ITCM leaves at zero fetch, a fixed body inlines it into fetch-charged `.main`
> bytes — and **it is already measured with a sign, twice, both against fixed**:
> `CAMERA_Q20_12.md` §6 (+3,032 B of inlined leaves turned −4,736 into +1,600)
> and the collision ring's `K-ICACHE` null. **NO CLOSED LANE REOPENS.**

**Budget: 220 B free on the INSTRUMENT, 2,572 B on the PROOF ROM; 676 / 3,028
after the frsub eviction.** A particle/quad fixed kernel needs 1,100–1,500 B, so
the warm re-test **does not fit on the instrument** — the only target that
produces a tick number. **Record that as structural: a placement candidate can
fit the ROM that ships and still be unpriceable.** `ANIM_ITCM.md` §6's Item B
(268 B) sits exactly in that gap and **fits after the eviction**.

## `SHDT` IS CLOSED: FREE HIT DETECTION CLEARS +64,977 BY 2,543 (2026-08-16) — `artifacts/performance/2026-08-16_shdt-mechanism/SHDT_MECHANISM.md`

**0 builds, 0 emulator runs, 1 harness change (the 2^22 filter, built and
verified). Boundary green, 0 `Exception:`. Root ROMs byte-unchanged.**

**Do not brief `SHDT` as an 88%-of-the-gap lever again.** Exact re-rank of
`build-c219-animitcm-ship`'s own 1,600 corrected rows (band 41–120 beside
rank-80, because rank-80 sits on a steep slope):

| intervention | rank-80 | moved | band 41–120 | gap |
|---|---:|---:|---:|---:|
| *(control)* | 1,210,304 | — | 1,217,946 | **+64,977** |
| delete `func_ovl2_800ED490` (self+leaf) everywhere | 1,203,600 | 6,704 | 1,212,715 | +58,273 |
| delete the whole collision chain's **excursion** | 1,186,630 | 23,674 | 1,197,387 | +41,303 |
| delete the whole collision chain **everywhere** | 1,166,216 | 44,088 | 1,172,913 | +20,889 |
| `SHDT` down to its own P50, every frame | 1,147,264 | 63,040 | 1,149,093 | **+1,937** |
| **the WHOLE `SHDT` bucket deleted, every frame** | **1,142,784** | **67,520** | **1,144,552** | **−2,543** |

**Hit detection running at literally zero cost clears the requirement by 3.8% of
it; reduced to its own median it does not clear it at all.**

**Mechanism, measured not read.** 57 of 1,600 frames run ≥1 (attack collision ×
hurtbox) test; their work premium is **+268,255 tk/fr**. The victim's **~14**
hurtbox joints get a world matrix, a 3×3 cofactor inverse and axis scales built
**once each**, then one rectangle test per pair. **The cost is per FRAME, not per
pair** — natural experiment on `build-c191-sitr-profile-c185`: frames with 44
pairs run **exactly 2.00×** the `TestRectangle` of frames with 22, while
`SetInvertMatrix` 0.98×, `EDBA4` 0.98×, `ED490` 0.97×, `__aeabi_fadd` 1.04×,
`__aeabi_fmul` 0.98× and `sqrtf` 0.98× stay **flat** and the frame gets **10,004
ticks cheaper**. OLS over the 57: **−440 tk/pair, R² = 0.003**; implied per-pair
collapses 327,966 → 5,229 across the count buckets (63×).

**Three routes closed by that one table.** The source's latch is already optimal
(nothing to memoise); the pair test is nearly free; a per-hurtbox broad phase
removes the flat half — a second, independent refutation of slice 47.

**Owner.** The fighter collision transform chain, **+102,988 tk/fr = 38.4%** of
the engaged premium (self 35,277 + leaf 67,712, `--attribute-leaves` over 8,746
`bl` sites). Largest item `func_ovl2_800ED490`, the 3×4 affine multiply: **63.0
soft-float calls per invocation** — exactly `gmcollision.c`'s 36 `fmul` + 27
`fadd` — at **32.7 ARM9 cycles per helper call**. **93.5% is genuine marginal
work; the cache-displacement reading is 4.5%.** Mask controls: random-88 negative
control −19,189 with **no** collision symbol; the cost-ranked mask overlaps at
37/88 and reproduces the asset-load ranking instead.

**The one sub-lane whose refutation does NOT transfer, and it is still declined.**
`EXCHANGE.md`'s 2.68 exchange rate was driven by `__udivmoddi4` at 4.0 sixty-four-bit
divides per entry; **an affine multiply has no divide.** But it changes the joint
matrices that decide hits (gameplay fidelity, rung 3), it is worth **6,704 =
0.103× of the requirement even free**, and `HWROUTE.md` §7 prices a byte in that
chain at 3.61 tk/fr. **Named, sized, not recommended — no owner decision asked.**

**Banked: the 2^22 filter.** `sample-tick-hud-buckets.ps1` now detects
`ALL >= (1<<22)` and subtracts it from `ALL` and every bucket at or above it, with
a new trailing **`WRAPFIX`** CSV column (last, so no column moves) and a warning
naming each frame against the run's corrected `ALL` median. Verified: all five
known-bad rows reproduce `IO_AUDIT.md` §2 **exactly**; frame 1357 corrects **6**
buckets = `SCPU`+`SINT`+`GCRA`+`SRC`+`WORK`+`ALL`, the parent chain §2 Proof 3
names, and the count was not used to build the filter; largest clean `ALL` is
3,358,080, a **25% margin** under the threshold. **Trigger still unproven.**

**Next largest un-diagnosed item: the `SITR` cluster** (27 frames, −51,200).

---

## THE CARD *IS* READ IN-MATCH — 7 times — AND THE INSTRUMENT INFLATES 5 FRAMES BY 2^22 (2026-08-16) — `artifacts/performance/2026-08-16_match-io-audit/IO_AUDIT.md`

**0 builds, 0 source changes, 3 whole-match runs on the existing
`build-c219-animitcm-ship` ROM.** Boundary green, 0 `Exception:`, root ROMs
byte-unchanged. Owner's question — *"does the match start before everything has
loaded?"* — answered below.

**1. Yes, seven times, and every one is a top-twelve frame.**
`gNdsRelocAssetPayloadReadCount` **101 → 108**, `HeaderReadCount` 1,593 → 1,600,
`ShortReadCount` **0**. Each read is one animation-cache **miss**
(`Misses`/`Fills` 3 → 10). **Cycle 105's mechanism is closed, not improved**:
arena `Overflows 0 / Rejects 0`, `UsedBytes` 415,984 of `ReservedBytes` 451,776,
`WarmFailed 0` — against that cycle's `142/142` at a full arena. Of the **349**
in-match animation acquisitions, **215 (61.6%) never reach the load path** (battle
pack), 127 hit the RAM cache, **7 read the card**. Frames **456, 830, 1015, 1186,
1625, 1655, 1886** = ranks **4, 1, 7, 10, 12, 8, 9** of 1,600. Priced against a
*matched* control (a frame that force-loads and does no I/O): **+593,728 median /
+536,988 mean**, 490,476 of it in `SITR`. **Worth 12,736 at rank-80** —
`POSITION.md`'s "seven load frames | 9,863" is the same lane, correctly counted.

**2. `cpuGetTiming()` reports a span exactly 2^22 = 4,194,304 ticks too large, in
8 of the last 13 whole-match runs.** Proven three ways: subtracting 2^22 lands
`ALL` within **−192/+256/+192/+128** of the run's own median on four of five
frames; the inflated `ALL` is **9.505 VBlanks** where every clean `ALL` is a whole
number; and it lands in whichever span was open (`FTR`, `SCPU`, `SPHD`). Mechanism
from the linked image: `cpuGetTiming` is `((u32)(tickGetCount()-start)) << 6`
(`0x020be710`) and `tickGetCount` credits **one** pending 16-bit timer overflow via
the heuristic `((timer ^ 0x8000) >> 15) & (IF >> 5)`; when that fails on a span's
*start* read the span is one overflow long — 2^16 × 64 = **2^22**.

> **THE REQUIREMENT IS +64,977, NOT +71,569.** Corrected, `build-c219-animitcm-ship`
> reads rank-80 **1,210,304 raw / 1,185,357 net**; band 41–120 =
> 1,323,072 / 1,300,480 / 1,266,240 / 1,225,152 / **1,210,304** / 1,188,480 /
> 1,166,144 / 1,154,944 / 1,140,608. Per-run correction ranges **−1,536 … −6,592**
> and is **asymmetric** (it can only inflate), so it adds build-dependent noise on
> top of the ≥14,080 placement floor. **Nothing was rebanked** — both numbers stand
> until the sampler filters it. **The filter is one condition, `ALL >= (1<<22)`,
> complete by construction, and it is NOT built.**

**3. `-PerFrameGlobals` emits a TORN bucket row and must not be used for bucket
values.** `ALL` comes from iteration *f*, every other bucket from *f+1*, so
**1,526 of 1,600 rows violate `ALL == WORK + WAIT`** — an identity the ring path
satisfies on all 1,600. Its counters are sound at ring offset **+1** (association
+212,620 there, −6,751 at 0, −882 at −1). Cycle 105's spike probe used this path.

**4. The force-load path is LIVE and it is the largest identified item — but the
"~228,600 per cache hit" price is REFUTED.** 134 force-loads on 116 frames.
`WORK-H` mean lift **+193,677 / +179,545 / +274,542** for 1 / 2 / 3 loads on the
frame, i.e. implied per-load **193,677 → 89,773 → 91,514**: it does not scale, so
it is not a per-event price. The count-linear part is **`SITR`** (+87,805 /
+193,086 / +205,572 → **~68,500–96,500 per load**), which **vindicates cycle 105's
`SINT` attribution**. `SPHD`/`SHDT`/`SPRM` also jump on single-load frames and
none of them scale — that is the state change, not the load. Exact re-rank:
deleting the premium on the 109 non-I/O force-load frames is **52,736 = 81% of
+64,977**.

**5. The over-gate set is FIVE populations, not one, and two methods agree**
(dominant-excess-owner labelling; Ward on excess-share vectors, best k=5,
silhouette 0.458). Leaf closure asserted **exact** — `leafsum == WORK-H` on all
1,600 rows, no clamping.

**Read the last column as it is headed: it deletes EVERY leaf's excess on those
frames, not the named cluster's.** Deleting only `SHDT`'s excess on its own 33
frames is **−50,752 (gap +14,225)**, not −57,152 (2026-08-16, `SHDT_MECHANISM.md`
§6).

| cluster | n | median own excess | withFL | withRead | delete ALL its frames' excess → rank-80 |
|---|---:|---:|---:|---:|---:|
| **`SHDT`** live hitbox hit detection | 33 | 259,776 | 7 | 0 | **−57,152** → gap +7,825 (`SHDT` alone: −50,752) |
| **`SITR`** interrupt/state proc | 27 | 231,264 | 16 | 6 | **−51,200** → gap +13,777 |
| `SPHD` physics map default | 8 | 215,136 | 5 | 0 | −14,464 |
| `SPRM` params/anim interpreter | 7 | 298,496 | 7 | 1 | −12,736 |
| `MISC` frames 450–453,455 | 5 | 215,680 | 0 | 0 | −6,016 |

They are **disjoint**: `SHDT` frames carry 14,112 of `SITR` excess (median
102,944) and `SITR` frames carry 2,176 of `SHDT` excess (median 4,608). 94.8%
median of an over-gate frame's excess is inside `SRC`; only 450/451/452 are
majority outside.

> **`SHDT` has a run P50 of 4,608 and a mean of 14,544, and on its own 33 cluster
> frames it runs 56× the run P50.** Every lane sizing done from a mean or a median
> self time has read it as noise; it owns **41% of the over-gate frames**. This is
> the concrete answer to "is the marginal-80 one population": **no**, and the
> worst-blurred lane is the one with the smallest median.
>
> **Two corrections, 2026-08-16 (`SHDT_MECHANISM.md` §6).** *mean ÷ P50 is 3.2×,
> not 56×* — the 56.4 is the cluster's excess ÷ the run P50, a different quantity.
> And *"88% of the remaining gap" is a ceiling on a deletion nobody can perform*:
> the whole `SHDT` bucket deleted from every frame moves rank-80 by **67,520**
> against a +64,977 requirement, so **the lane at zero cost clears it by 2,543 and
> nothing short of zero clears it at all.**

**Open, unexplained, not this cycle's:** the load-frame population moves
**±100,000–150,000** between two *same-binary* arms (camera pair: 830 −124,160,
1500 −142,016, 747 −103,360, 1975 +152,448, 1447 +129,280) against a whole-run
paired median of −4,768. Not the 2^22 artifact — neither camera arm carries one.

**`CAMERA_Q20_12.md` §3.2 is corrected in place**: ranks 10 and 20 are **different
frames** in the two arms (rank-10 = control 1655 vs candidate 1975; top-20 sets
overlap 19/20 but only 9/20 share a rank), so +30,336/+20,224 are rank-permutation
artifacts, not cartridge cost, and "not reproducible between two emulator
sessions" is false — three sessions on one ROM gave **byte-identical** 1,600-row
CSVs. **The banked −4,736 is untouched.**

**Harness note, cost 20 minutes:** `verify-all.ps1`'s pre-flight throws
*"Recursive make is unusable … `NDS_RECURSIVE_MAKE=FAIL:127`"* when launched from a
Git-Bash-spawned shell and passes under PowerShell. The message blames the
toolchain; the toolchain is fine. Run the harnesses from PowerShell.

---

## THE LEVEL IS +71,569 (2026-08-16) — `artifacts/performance/2026-08-16_anim-itcm/ANIM_ITCM.md`

**New basis `build-c219-animitcm-ship`: rank-80 1,216,896 raw / 1,191,949 net.**
Same target, config and window as every basis before it.

**`ndsR2AnimValueQ` IS IN ITCM.** The campaign's largest single fidelity-neutral
item was a placement question, and it is answered on a **same-binary route** —
two byte-identical copies of one body (**257 words, 0 differing words**) at two
addresses, one `.data` word choosing the `bl`. Engagement equality
`gNdsR2CubicEvals = 290,076` on **both** arms; complement `WORK-H −3,840 /
WAIT +3,840 / ALL +0`.

```text
whole-run paired median   -3,840  (85.8% of 1,600 frames)   BANKED
marginal-80 median        -6,432  (73 of 80)
rank-80                  -14,208      ranks 41-120 band    -15,572
```

Every one of 14 sampled ranks improves, rank 1 through rank 1200. The percentile
readings are 2.2–2.4× the paired median because the saving **clusters on
expensive frames** — a heavy frame plays more Q AObj nodes. Bank the −3,840; the
cross-build −1,856 is deep inside the ≥14,080 floor and its band moved the other
way, so **+71,569 is a LEVEL, not a bank.**

**ITCM WAS NOT FULL — IT HELD 2,454 DEAD BYTES.** The per-PC census that priced
the kernel also prices every ITCM resident, and 33 non-overlapping blocks execute
**zero** instructions across the whole 1,600-frame window. 736 B taken:
`_arm_cmpsf2.o` + `_arm_unordsf2.o` (332 B — the port defines the `fcmp` helpers
itself, so libgcc's renamed goldens have no caller **by construction**) and
`ndsRendererHardwareGetLightShadeLut` (404 B — the miss-path LUT *builder*).
Instrument ITCM free **512 → 220**; proof ROM **2,572**. `itcm-census.txt` lists
the 1,718 B left.

**Two traps this cost, both now structural.** (1) `linker/nds_hot_text.ld:113`
matches `*.itcm.*` by **filename**, so dropping `--rename-section` freed nothing
until the object was also renamed `<stem>.mainram.o` — the first link overflowed
by exactly the 332 B that were supposed to have moved. (2)
`check-task9-float-itcm.ps1` asserted a **placement policy** where it meant to
assert the Phase-2 **rename**; it now reads each member's placement from the
build's own emitted object name, still throws on a mismatch (proven three ways,
`ANIM_ITCM.md` §8), and its new fail-closed guard caught a stale-object hazard
the Makefile recipe now cleans.

| row | now |
|---|---|
| `ndsR2AnimValueQ` fetch (21,719) | **BANKED −3,840**, shipped in ITCM. §2 corrects the "a kernel is fetched whole by construction" premise: **162 of 257 slots and 23 of 33 lines** execute; 320 B are never fetched |
| bind placement (`ndsRendererHardwareBindTextureName`) | **STILL UNBUILT, and now blocked twice**: 268 B wanted against **220 B free**, and its 3,802 ceiling is **inside** the ≥14,080 cross-build floor, so it needs its own **route**, not just its own build. Next 240 B come from the two raw-run emitters |

---

## SUPERSEDED — the level was +73,425 (2026-08-16) — `artifacts/performance/2026-08-16_tilesync-memo/TILESYNC.md`

**New basis `build-c217-tilesync-ship`: rank-80 1,218,752 raw / 1,193,805 net.**
Same target, config and window as every basis before it.

**Bank only −3,648 of the −8,640.** The tile-sync memo is measured on a
same-binary zero-noise route at **−3,648 paired median, 87.1% of 1,600 frames
improving**, complement-controlled (`WORK-H −3,648 / WAIT +3,648 / ALL +0`). The
shipping build reads −8,640 at rank-80 and −9,216 paired against
`build-c215-hwmath-ship`, but rank-80's cross-build floor is ≥14,080 and this
pair's own placement term is visible as `SRC +3,136` — a bucket the memo cannot
touch. **+73,425 is a LEVEL, not a bank.**

**A second, independent equality control.** Boundary's own proof ROM carries the
memo and its 212-frame realtime smoke reads `binds=54 vtx=2484 tri=828` and
`ftrTri=132712/p067840/p164872/own424` **identical** to `boundary-c206` and
`-c209b`, for `ticks` 294,482,496 → 294,363,520 = **−118,976**. Same geometry,
same binds, same triangles, measurably less time.

**THE TWO COUNTER-GATED ROWS HAD THEIR COUNTER, AND IT HAD BEEN READ ON
2026-08-15.** `NDS_TASK107_RENDER_STATE_CENSUS` (`Makefile:250`) publishes both,
and `artifacts/performance/2026-08-15_renderer-state-redundancy/` is its run.
Three documents since then said "needs one counter first". Before writing an
instrument, `ls artifacts/performance/`. That census then stopped on a
**16,000-tick package floor that `AGENTS.md` now explicitly disavows** ("keep
every repeatable correctness-preserving gain"), which is why a real 62%-redundant
lane sat unbanked for a day.

| row | was | now |
|---|---|---|
| `ndsRendererSyncTextureTile` | 8,867, "needs a counter" | **BANKED −3,648.** 89,511 of 144,105 calls (62.12%) provably redundant; an exact serial memo ships |
| texture-bind collapse | 13,868, "needs a counter" | **0 as an elision item** — the census measured **zero** exact locally-redundant issues; 46.05% are already elided; the 26,769 revisits need draw REORDERING, ceiling 2,484 |
| — | — | **NEW: +5,754 as a PLACEMENT item.** 45% of the bind pair's cost is `icache_fill` on **360 B** (14.19 and 21.22 tk/fr per byte). 268 B is port-reachable (`ndsRendererHardwareBindTextureName` is `static`); `glBindTexture` is libnds. Ceiling, unbuilt, **one build** |

`POSITION.md`'s fidelity-neutral inventory is corrected in place: 53,215 →
**42,366**, i.e. **10,849 tk/fr of it was never there.**

### SUPERSEDED — the gap was +82,065 (2026-08-16, `HWROUTE.md`)

**Two bit-identical codegen changes shipped, and the basis was
`build-c215-hwmath-ship`: rank-80 1,227,392 raw / 1,202,445 net = GAP +82,065.**
Same target, config and window as the `c206-shipgx0` basis it replaced.

| item | change | state |
|---|---|---|
| **A** | `nds_r2_sqrtf.o` built `-marm`, so `nds_r2_sqrtf.h:73`'s 48-bit `root*root` is one `UMULL` instead of `bl __aeabi_lmul` | **LANDED** |
| **B** | the leading `DIVCNT`/`SQRTCNT` busy poll deleted from every executing site — `sqrtf` and nine renderer sites, via `ndsR2HwMathDiv64`/`ndsR2HwMathSqrt64`. **16 leading polls before, 2 after**; **the last 2 came out 2026-08-16 with the `NDS_R2_CAMERA_FIXED` flip that made them execute — all sixteen are now gone** | **LANDED** |

```text
same-binary route, one ROM (sha BEBC5801...), four arms, readback==requested on each
  rank-80         arm0 1,241,536 -> arm3 1,229,824      -11,712
  ranks 41-120 band mean (reorder-robust tail)          -14,125
  paired median, whole run                               -5,184   79.9% of frames win
the shipping build, cross-build on the same window       -12,416  band -12,144
.main                          931,088 -> 930,872 B      -216 B   NEGATIVE
```

**The ladder now reads 0.695x of +94,481** (12,416 banked + `POSITION.md`'s untouched
53,215 inventory = 65,631), leaving **+28,850** unaccounted. It read 0.611x.

**Method result worth reusing: this instrument is deterministic.** Two repeat runs
reproduced their 1,600-row CSVs **byte for byte** (identical sha256, VBI histogram
included). On a same-binary route with a fixed input script the same-arm repeat floor is
**exactly zero**, so every tick between arms is the change — and a repeat of an unchanged
arm buys nothing and should not be spent.

**Read the concentration, not just the headline.** The change removes ~5,200 tk/fr from
the median frame and ~13,000–14,000 from the gate population, because the `sqrtf` lane is
**1.76x denser on the marginal-80 frames** (80.26 calls/frame there against 45.72
whole-match, from the entry PC). rank-80 sits on a steep local slope — rank 60 −26,880,
rank 80 −11,712, rank 100 −5,824 — so quote the 41–120 band, not one order statistic.
For the same reason **arm 2's rank-80 of +3,072 is not a regression**: its paired median
is −768 and its band is −1,571.

**BLOCKED(decision: `sqrtf` IME mask).** `nds_r2_hwmath_unit.h`'s ELF survey shows this
binary has no interrupt-context user of either unit, so the reachability the mask guards
does not currently exist. Priced: **698 tk/fr** for its three I/O accesses, **1,294** with
their register setup. Removing a safety property is the owner's call, not an optimisation.

## THE DS DIVIDE AND ROOT UNITS ARE PRICED (2026-08-16) — `artifacts/performance/2026-08-16_hwmath-units/HWMATH.md`

**The three never-defined hooks are defined.** `NDS_R2_CFX_DIV64`,
`NDS_R2_CFX_ISQRT64` **and** `NDS_R2_COLLISION_DIV64` (`nds_r2_collision_mtx.h:361`,
which nobody had counted and which sits inside `ndsR2CollisionFixedInvertF32`, a live
ring entry point) now reach the ARM9 coprocessors. +44 B of text; 0 libgcc 64-bit divides
left in the object against 6 in the control; graded 0 mismatches over 65,536 operands per
class on four builds. `NDS_R2_CFX_HWMATH` follows `NDS_R2_COLLISION_FIXED`, which is 0
everywhere, so no shipped byte moves.

**Measured per-operation prices** (in situ, 4,096 iterations, loop and operand generator
subtracted, `build-c213-hwmath4`):

| operation | software | DS unit (leading poll / SM64DS form) | ratio |
|---|---:|---:|---|
| 64-bit divide | 292.5 tk | **106.0 / 65.0** | 2.76x / 4.50x |
| 64-bit isqrt | 294.9 tk | **55.5 / 35.5** | 5.32x / 8.31x |
| f32 divide | `__aeabi_fdiv` **71.5 tk** | 137.6 / 97.6 | **0.52x / 0.73x — REFUTED** |
| `sqrtf` | shipped Thumb **153.0 tk** | ARM state **90.5–96.5** | **1.59x–1.69x** |

**Three things this settles, and one it opens.**

1. **The ring stays closed.** The hooks take its exchange rate from 2.68 to **2.19–2.08**
   (arithmetic on `EXCHANGE.md` §3.1's own measured `__udivmoddi4` row, not a new A/B).
   Still a loss, and `EXCHANGE.md` §0.3's 15,217 tk/fr ceiling is **0.161x of +94,481 at
   an exchange rate of zero.** Do not re-price this lane again.
2. **Column N is 0.050x, not 0.315x.** Its `sqrtf` half is **already shipped** —
   `NDS_R2_FIXED_SQRT ?= 1` and it reads 1 in `build-c200-trackprof-off`,
   `build-c206-shipgx0` and `build-c209-simmac2` — so `SIMSIDE.md` §6's 144.62 tk/call
   *is* the hardware implementation. Its `fdiv` half is refused on price by a floor
   argument: the unit alone costs 65.0 tk against a 71.5 tk routine that does the whole
   job. **`SIMSIDE.md` §6's 29,786 tk/fr surface should be read as 4,300–4,750 measured.**
3. **LANDED 2026-08-16 — `nds_r2_sqrtf.o` is built `-marm`.** The 4,300–4,750 estimate
   here was **2.8x low**: measured on a same-binary route it is worth −3,840 tk/fr at the
   median frame and −8,960 on the marginal-80 population, because that estimate was priced
   on the whole-match `sqrtf` rate and the gate population carries 1.76x of it. See the
   section above.
4. **LANDED 2026-08-16 — the leading poll is gone from every executing site.** Its rate
   was counted from the entry PCs before a byte was written (`[[entry-pc-gives-exact-call-counts]]`):
   **2,491.3 tk/fr** at marginal-80, of which `sqrtf` alone is 1,845.8. The outlined
   libnds `div64` executes **zero** times in this match, so nothing had to be done about
   libnds. **DONE 2026-08-16:** `battleship_gmcamera.c`'s two came out the moment the
   owner's decision made them execute; sized at **2,721 tk/fr** before the run from the
   engagement counts (56.2 divides + 12.0 roots a frame), difference of differences
   **−2,560**. The binary now has **no leading poll anywhere**.

**Chain map (item C).** C1 collision cluster 50,044 tk/fr — built, conv/op ~0.02, still
2.68, killed by 3,228 B at **0.97 entries/frame**. C3 trig leaves 4,519 tk/fr are C2's
interior and already implemented in `nds_r2_collision_fixed.h`.

**C2 animation→joint-matrix is CLOSED, 2026-08-16. The byte ledger is written and the
answer is NO by 3x** (`../2026-08-16_tilesync-memo/TILESYNC.md` §5). Do not re-open it.

**Two facts kill it, and neither needs a build.**

1. **Most of C2 is already converted.** `Makefile:1680` is
   `override NDS_R2_CUBIC_FIXED := 1` and `NDS_R2_ANIM_CUT_ROUTE ?= 0` folds
   `NDS_R2_ANIM_CUT_ON(bit)` to a constant 1, so `ndsR2FtAnimParseDObjFigatree` is the
   **Q parser in every shipped ROM** with its float arms dead-coded, its evaluator is
   `ndsR2AnimValueQ`, and `ndsR2AQStore` is a bit-pun so the representation costs zero
   storage. The 20,357 "prize" is not un-converted arithmetic — it is what **survives**:
   the f32 fields the decomp ABI fixes (`anim_wait`, `anim_speed`, `anim_frame`,
   `aobj->length`, the `Vec3f` joint outputs). That is the representation boundary, which
   `EXCHANGE_LEAF.md` already measured at R 0.83x–1.00x. `gmCollisionTransformMatrixAll`
   makes it exact: 21.7 ops/entry against **21 conversions — conv/op 0.97**, the same
   reading `gmCollisionGetWorldPosition` returned at 18/18. Blast radius if the ABI moves
   instead: **1,403 exact `->anim_wait`/`->anim_speed`/`->anim_frame` accesses in 117
   files**, and `Makefile:2302` makes every one a caller rewrite.

2. **`HWROUTE.md` §7's 3.61 tk/fr per byte is an AVERAGE, and it understates an added hot
   kernel by 4.6x–5.9x.** Measured on the same census: `ndsR2AnimValueQ` 1,028 B at 370.6
   entries/frame pays **21,719 tk/fr of `icache_fill` = 21.13 tk/fr per byte**;
   `lbCommonSin` 18.66; `lbCommonCos` 16.74. A large caller fetches only its executed
   path; a kernel is fetched whole on every call, by construction.

```text
                                     HWROUTE section 7   MEASURED
whole 20,357 prize spent by                +5,640 B       963 B
half of it by                              +2,820 B       482 B
collision ring added                        3,228 B  = 3.4x the WHOLE budget
camera inlining added                       3,032 B  = 3.1x the WHOLE budget
```

Its two standing limits both survive and both got worse: the chain's own `issue` cost is
**7,924 of 37,854 (20.9%)**, and `dcache_fill` (10,817) exceeds it, so it is memory-bound
on both sides and a Q26 joint matrix is the same 32 bits as the `f32` it replaces.

**CLOSED 2026-08-16 — the kernel is in ITCM, banked −3,840, see the level header
and `artifacts/performance/2026-08-16_anim-itcm/ANIM_ITCM.md`. The "fetched whole
by construction" mechanism below is REFUTED by its own census (162 of 257 slots
execute); the 21.13 tk/fr per byte price stands.**

**WHAT THE LEDGER UNCOVERED, AND IT IS UNOWNED: `ndsR2AnimValueQ` pays 21,719 tk/fr of
PURE INSTRUCTION FETCH** for 1,028 bytes at 370.6 entries a frame — **0.296x of +73,425**,
fidelity-neutral by construction (the arithmetic does not change), and priced by no
document on this board. Add `lbCommonSin`+`lbCommonCos` (120 B, 2,116 tk/fr of fetch) and
the animation chain's *fetch* bill is **23,835 tk/fr against an arithmetic prize of 20,357
that does not convert.** **CLOSED for `ndsR2AnimValueQ` 2026-08-16** — it is in ITCM and
banked −3,840. The instrument's free ITCM is now **220 B** (not 512), and
`ITCM_CENSUS.md` prices what is left: **456 B provably unreachable**, 788 B silent,
468 B of crash/kernel paths to leave alone, and **9,484 B cold inside five live
renderer functions** that only a source split can reach.

**One defect found and fixed at its seam.** `int32_t` is `long` on devkitARM, so an
`(int32_t *)` cast onto an `int *` in the bench's wrapper was a strict-aliasing violation;
GCC folded the remainder to its initialiser and deleted the sticky term, producing 110
wrong IEEE quotients in 65,536 — deterministic, in one of three builds of the same source.
No pointer cast is left in the chain, so drift is now a `-Werror` error. Nothing shipped
carried it. `HWMATH.md` §7.

## THE BANK — RE-ESTABLISHED ON THE REPAIRED TREE AT THE SHIPPING DEFAULT (2026-08-15, `build-c200-bank84`)

`artifacts/performance/2026-08-15_ftanim-full-coverage/REBANK.md`. c185's
configuration on the repaired tree: `NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1
NDS_R2_BATTLEPACK_KEEP_CACHE=1 NDS_R2_FIGHTER_GX_COMPOSE_LAB=1`, DLDI on, mode
163 one-minute, 1,600 samples, `-RingDump`, frames 439–2038, `slips=0`.

| | value |
|---|---:|
| `WORK-H` P50 | 942,656 |
| P90 | 1,123,328 |
| **rank-80 raw** | **1,226,624** |
| **rank-80 net of apparatus 24,947** | **1,201,677** |
| **gap to 1,120,380** | **+81,297** |
| top-1% · max | 1,546,304 · 5,090,560 |
| trimmed mean (drop top 8) | 963,510 |
| over-gate | 166/1600 |
| VBI 2/3/4/5+ · max · total | 1711/303/16/8 · 19 · 2038 |

> ### BASIS CORRECTION, 2026-08-16 — "at the shipping default" was about the BORE, not the renderer
>
> `artifacts/performance/2026-08-16_gxcompose-bank-basis/BASIS.md`.
> **This bank, `c199-bank0` and every bank since `c185` were built
> `NDS_R2_FIGHTER_GX_COMPOSE=1` while the ROM ships `0`** — proven from
> `builds/build-c199-bank0/nds_build_config.h` and from `nm` finding the 8
> `gNdsR2GxCompose*` symbols in its ELF that exist only inside that `#if`
> (0 on every GX=0 ELF). `Makefile:766` is `?= 0`; the published block
> (`Makefile:1545`) pins **0 unconditionally**. `c170` and everything before it
> was GX=0, so the basis changed at `c185`.
>
> **The shipping level, measured fresh at HEAD `b1339828070`:**
> `build-c206-shipgx0` (GX=0) rank-80 **1,239,808 raw / 1,214,861 net =
> GAP +94,481**. Its one-config-line sibling `build-c207-gx1` reads
> 1,232,768 / 1,207,821 (+87,441) and lands 2,048 from `c199-bank0`, so the
> banked configuration replicates. All seventeen invariants identical on all
> three arms, `slips=0`, GX engagement complete on the GX=1 arm
> (`Declines=0`, `Captures=Roots=62,920`).
>
> **The 9,088 between the two configurations is inside the ≥14,080 cross-build
> floor**, so the bank is not *provably* optimistic — but it is measured on a
> renderer the user does not run. **Quote `+94,481` and cite
> `build-c206-shipgx0`**; every "x of the gap" ratio multiplies by **0.904**
> and no conclusion in `LADDER.md`, `MENU.md` or `CAMERA_Q20_12.md` changes
> sign.
>
> **RE-DIVISION DONE 2026-08-16.** `LADDER.md`, `MENU.md`, `DRAW_FIXEDPOINT.md`
> and `CAMERA_Q20_12.md` are re-quoted **in place**, and every table that
> carries a ratio now states `+94,481` next to it so the denominator can no
> longer be separated from the number. Two rows moved for reasons other than
> the denominator: the animation-representation mechanism (33,951) is struck
> as **closed at ~1% conversion**, and `LADDER.md` §4's "0.357x → 0.659x"
> amendment is **withdrawn** on both halves (the 5.14x rate was refuted at
> 1.70x, and the lane was never engineering-available — it is
> `BLOCKED(decision: draw-side precision)`).
>
> **`GX_COMPOSE`'s `−17,152` is RETIRED, and the "transfer" is REFUTED too.**
> The same-HEAD pair reads rank-80 **+7,040** and P50 **−4,288** — inside both
> floors and disagreeing in sign. The −17,152 came from `build-c184-gxc-a/-b`
> at a 1,189,312 level, i.e. **before the segment repair `64c41c361a7`**.
> **The `FTR −8,192 / STG +6,656` was taken to a second pair with
> `NDS_TASK103_STAGE_RUN_PHASE=1` and the stage half did not survive**
> (`artifacts/performance/2026-08-16_gx-transfer-locate/`, 2 builds, 2 gate
> runs): every stage *work* counter is **bit-identical to the unit** —
> `WordCount` 7,972,976, `RunCount` 67,188, `CommitCount` 16,296,
> `DisplayCount` 55,890 — `PushTicks` differs 0.09%, and ΔSTG inverts to
> **−1,632**. The fighter saving reproduces on both pairs; the stage rise does
> not. Not a quantisation artefact either: **zero floored rows in any bucket**
> over 3,200 sampled frames, granularity 64 ticks. What is left is cross-build
> placement or a GX queue effect the instrument absorbs — bounded, not
> mechanised. **BLOCKED(decision: `GX_COMPOSE` default) NO LONGER BLOCKS
> ANYTHING**: it has no performance content, and the basis-consistency half is
> discharged by the re-division above.
>
> ### THE ASSEMBLED POSITION — `BLOCKED(decision: sacrifice order)`, with the owner
>
> `artifacts/performance/2026-08-16_gap-position/POSITION.md`. **Everything
> that costs no fidelity at all sums to 53,215 tk/fr = 0.563x of +94,481 at
> 100% conversion, and 100% is available for none of it** — two items need a
> counter before they are items, one is measured unable to reach 100%. Deleting
> the whole inventory leaves **+41,266**. Priced in `PROJECT_GOAL.md`'s own
> order: rung 1 audio **≤3,040 = 0.032x** (the gate is an ARM9 metric; DS
> mixing is the ARM7's job); rung 2 visual **44,476–50,203 = 0.471x–0.531x**;
> rung 3 gameplay **55,473–84,451 = 0.587x–0.894x, projected**; rung 4 the
> 60 Hz simulation **291,488 = 3.085x, and it is the only thing on the board
> that closes alone.** That last figure is **re-derived on this tree** — halve
> `SRC` (which `taskman_seam.c:4273-4275` states is exactly the two logical
> updates a presented frame runs) across `build-c206-shipgx0`'s own 1,600 rows
> and re-rank — and it is **2.4x the recorded Task 106 figure of 119,744**,
> because `SRC` concentrates **2.09x on the gate population**. Only **16.4% of
> `SRC`** is needed to close, so partial-cadence variants close too: `SINT`
> half alone is 1.398x, and `GCRA` half with the AI and the interrupt/physics
> half **held at 60 Hz** is 1.283x. `POSITION.md` §4.1 names what must not
> halve (input sampling, hitbox resolution, the single shared `syUtilsRandFloat`
> LCG behind 135 draw sites, and every integer frame counter).

**`+28,689` IS DEAD AND EVERY LEVER PRICED AGAINST IT MUST BE RE-READ.** The
c185 bank (1,174,016 raw / 1,149,069 net) measured a match the shipped
segment-phase parser defect made *cheaper* (`64c41c361a7`); the repaired match
costs **56,704 more raw ticks at rank-80**. Nothing regressed — the requirement
is simply three times larger than the `MENU.md` ladder was sized against.

**Basis, because two conventions are in circulation:** rank-80 is recomputed
from the 1,600 per-frame rows in the run's own JSON (it reproduces
`DENSE_RUNTIME.md`'s table exactly). The harness banner's `p95` column uses a
different rank convention and is **not** the banked figure.

**Two independent arms bracket it:** `build-c193-segfix` (earlier HEAD)
1,228,608 and `build-c200-bank84` 1,226,624 — 1,984 apart, inside the ≥14,080
cross-build P95 floor; P50 256 apart inside ~5,700. A third arm at bore 0
(`build-c199-bank0`) reads 1,230,720, so the whole bore spread is 4,096 at
rank-80 — inside the same floor. **The shipping bore is now 0, so a future
re-bank belongs on the bore-0 arm; until then quote +81,297 and carry ±4,096 of
bore basis with it — the bore is not a performance question either way.**
Invariants on all three:
P1Damage 76 · spark 16 · shield 480 · AObj high-water 774 · packHits 257 ·
runaway 0 · CaptureOutcome 2 · SegmentMask 161. Arena: ChosenSize 1,548,288,
AllocFail 0, heap free-min 53,136 against the 32,768 reserve.

**Full-coverage dense animation is REFUSED on a measured sizing failure.** The
track pack can only *coexist* with `battlepack_fox.bin` (three consumers need
the o2r stream: the bind's asset-id resolution, the fail-open generic parser
that Mario uses entirely, and the oracle's reference cursor), and coexistence
needs +287,082 B against +16,384 B of grantable arena — **short by 270,698 B**.

**But the v3 was taken and the lane is closed on its own merits**
(`…/2026-08-15_ftanim-dispatch-attribution/RESULT.md`). It took the **FETCH**
branch — the dense side is 72.4% icache+dcache fill — and refuted the ISSUE
branch three placement-immune ways. **Both sides of the exchange are ~3,800 tk/fr
and they cancel**: the measured whole-match named exchange at 23.25% parse-call
coverage is **−74 tk/fr**, linear to 100% **−319 tk/fr**, i.e. order 10² tk/fr
against a +81,297 gap. **33,951 was the lane's SIZE; the representation converts
~1% of it.** So **the full-coverage arena arm no longer needs building** — the
270,698 B shortfall stops mattering. An earlier "+69.4 tk per exchanged call,
~1.59x" is **retracted**: the exchange is exactly 1:1 and the dense call costs
208.0 tk against 215.5 (0.965x); it was a residual divided by a count.

**TASK 4 — DRAW-SIDE FIXED POINT: the arithmetic gate is PASSED, and the collision
lane's exchange rate is falsified at 5.14x** (`…/2026-08-15_drawside-softfloat/DRAW_FIXEDPOINT.md`;
**0 builds, 0 emulator runs** — priced from the `c200-trackprof-off` v3 already on disk,
marginal-80 threshold 1,224,970, 1,654 from the bank's rank-80).

Soft float is **168,060 tk/fr** at rank-80. Split by phase from the **linked ELF** (reverse
call graph pruned to the 1,363 of 3,803 symbols that executed; roots labelled by callback
*role* before module prefix): shared 57,521 · sim-only 39,537 · sim+dispatch 37,662 ·
**draw+dispatch 17,407 · draw-only 13,231** · unresolved 2,702. With draw-side `sqrtf`
(3,540) the convertible lane is **34,178 tk/fr = 0.420x of +81,297**, over 2,271.9
calls/frame and 872 sites. **Draw-side concentration is 1.12x against the class's 2.11x** —
flat, so it converts 1:1 against a level gate.

**The deciding measurement, free:** the same operation exists in this binary in both forms.
Float 4×4 concat `guMtxCatF` **2,921 tk/call**; port 20.12 concat `ndsRendererMtxMul20p12`
**568.40 tk/call**. Same mask, same build, both live — **5.14x**. Both forms' multiplies
run at 2.00 cycles, so the whole difference is unpack/normalise/round. The float library is
**ITCM-resident with 2,976 B free**, which is why the ring lost and why this need not:
55% of the lane converts to inline integer ops *smaller* than the `bl` they delete.
**Conservative ceiling 24,564 tk/fr (lowest of three routes) = 1.54x the 16,000 floor.**

**No implementation, no build, no A/B, no pixel pair — and that is stated, not discovered.**
No sub-package clears 16K alone. Next step is a **falsifier**, not the package: one build,
one same-binary route (explicit `.data` attribute on the route word), one gate run, camera
chain only — 15,812 tk/fr gross, 2,764 B, 181 sites. **SUPERSEDED 2026-08-16:**
`NDS_R2_CFX_DIV64`/`ISQRT64` are now DEFINED and the units are priced (2.76x–4.50x on the
divide, 5.32x–8.31x on the root) — see the hardware-units section at the top of this board;
`__udivmoddi4` already runs 11.70×/frame at 2,909 tk/fr and nothing may add to it.

> **CORRECTION OF A RETRACTION, 2026-08-15.** An earlier revision of this
> section asserted that the owner's approval of bore 0 had been **fabricated by
> the agent** and restored the shipping default to 84 (`88abf259bda`,
> `9b25d4e1095`). **That assertion was wrong: both owner quotes are genuine**,
> relayed accurately; the fabrication conclusion was inferred from a working tree
> that contradicted them, not from the owner. **The owner has since settled it
> directly — *"bore should be zero, no offset, not needed anymore"* — and the
> shipping default is 0**, with `docs/BUGS.md`'s Fox row closed rather than
> `BLOCKED`. The bank above is unaffected (bore spread 4,096, inside the floor).
> `REBANK.md` §2 carries the full account. **What stays retracted is a different
> number**: this section's "+69.4 tk per exchanged call, ~1.59x" — a genuine
> residual-÷-count error, refuted by the v3 at exactly 1:1 / 0.965x.

**TASK 5 — THE CAMERA CHAIN IS BUILT, MEASURED, AND THE 5.14x PRIOR IS REFUTED IN SITU AT
1.70x** (`…/2026-08-16_camera-fixedpoint/CAMERA_Q20_12.md`; **3 lab builds, 4 gate runs**,
two same-binary `.data`-route A/Bs, 0 published bytes moved, route ships at **0**).

Two of the three camera producers converted to Q20.12 — the game camera
(`gmCameraLookAtFuncMatrix`, 2.000 entries/fr) and the renderer adapter's
(`syMatrixLookAtReflect` 2.000/fr + `syMatrixPerspFast` 2.138/fr) — sharing one look-at and
one perspective kernel. **`syMatrixF2L` is deleted, not converted, for all 6.138 of its
entries a frame.** The particle camera (`syMatrixLookAtF`, `syMatrixOrthoF`, `guMtxCatF`)
is a different kernel set and was left alone. Divides and roots go to the DS hardware units
at `0x04000280`/`0x040002B0`; **nothing was added to `__udivmoddi4`**.

```text
v1 build-c201-camfix, same-binary route, floor zero, WORK-H, 1,600 frames, slips=0
  paired median  -4,736   mean -4,728   improved 1,439/1,600
  P50 -4,160 · P90 -6,016 · trimmed -4,573 · top-80 paired median -5,568
  rank-80 point estimate -1,408  <- NOISE, see below
  FTR -3,264 · STG -1,664 (sum -4,928); no other bucket moves at the median
```

**The rank-80 point estimate is not the result.** Rank-by-rank the delta is −7,808 /
−6,016 / −4,160 / −3,904 / −4,160 / −5,248 / −4,992 at ranks 40/160/320/640/800/1200/1600 —
a **level**, as a per-frame-constant workload must be. Ranks 10 and 20 read **+30,336** and
**+20,224** from cartridge-read frames that do not reproduce between two emulator sessions
of a match whose seventeen invariants are bit-identical. Rank-80 sits inside that band.
**Quote the paired per-frame median.**

**THE RATE.** Gross deleted 11,657 tk/fr marginal-80 / 11,504 whole match (rates from
`DRAW_FIXEDPOINT.md`, call counts confirmed by this cycle's own engagement counters).
`R = gross/(gross − net)` → **1.700x whole match, 1.915x on the gate population, against
the 5.14x prior.** It is an **upper** bound: the numerator carries only the soft-float
library bill, and the float form's self time — which the prior's 2,921 tk *did* include —
can only drive R toward 1. **Reasons, named:** the prior is a pure multiply-accumulate
pair, while a look-at needs **3 roots and 9 divides** per entry and a perspective 5 more,
and those convert badly; and the float library is ITCM-resident while the replacement
cannot be.

**ITCM IS NOT AVAILABLE, and the 2,976 B free is the wrong ROM.** `.itcm` on the
measurement instrument (`…-tickhud-hwtri`) is **32,188/32,768 — 580 B free**; the 2,976 is
Boundary's manifest for the **proof** ROM. Two link attempts overflowed `itcm` by **916 B**
(kernels only) and **1,452 B** (with leaves). Structurally, a `.data` route needs both arms
resident, so **a same-binary route can never test ITCM residency for a replacement of an
ITCM-resident library**; that needs a compile-time pair whose ≥14,080 rank-80 floor is 3x
the effect.

**THE CYCLE'S MOST USEFUL SURPRISE: inlining the leaves INVERTED the win.** v2
(`build-c202-camfix2`) made every leaf `always_inline` — 42 calls per entry became 12 — and
the paired median went **−4,736 → +1,600**. Across the two same-binary pairs and the
measured cross-build control term (+5,440 paired median on two builds whose route-0 path is
identical), the fixed kernel became **+11,776 tk/fr more expensive for +3,032 B**, ~30
cycles per added line per entry — inside the measured 23–51 cycle `icache_fill` band.
**On a kernel entered a handful of times a frame, inlining is a cost.** `K-ICACHE`'s
mechanism, at 8.138 entries/frame instead of 0.97, strong enough to flip a sign.

**Two build traps, both reproduced and both fixed before any number was believed:** the
kernels first compiled to **Thumb** (18 `bl __aeabi_lmul` in one look-at), and even in ARM
state `noinline` plus mismatched `target` attributes kept 42 calls per entry, because GCC
will not inline across differing target attributes.

**Engagement and control.** Candidate 8,148 fixed look-ats / 8,224 fixed perspectives
against 2 and 2 float (one pre-poke frame); control 0 fixed, 4,076/4,152 float. **Saturate
0, degenerate 0, rescale 0.** All eight new or routed symbols classify **`draw+dispatch`,
100.0%** from the linked ELF; `gGMCameraMatrix`'s four readers, from the image's literal
pools, are all display/present callbacks. **All seventeen match invariants are bit-identical
across all four runs and equal to `build-c199-bank0`'s.**

### THE CAMERA ARM'S PRESENTED CADENCE — MEASURED 2026-08-16, AND THE ARM IS INNOCENT

`artifacts/performance/2026-08-16_camera-cadence/CADENCE.md`. Nine runs on
`builds/build-c205-camtoggle` — **the toggle ROM the owner played**, not the tick-HUD
instrument. Arm selected at frame 1, two gdb stops, 2,043 presented frames each; both arms
reproduce **bit-identically** on a repeat run.

| arm | iv 2 | iv 3 | iv 4 | iv 5+ | max | VBlanks | P50 | P95 | P99 | violations |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| FLOAT (shipping) | 1,953 | 72 | 5 | 13 | 26 | 4,277 | 2 | 2 | 3 | 0 |
| FIXED Q20.12 | **1,956** | **69** | 5 | 13 | 26 | **4,273** | 2 | 2 | 3 | 0 |

**The fixed arm is 3 frames BETTER and 4 VBlanks shorter over the match**, and the per-frame
instrument reproduces the same margin independently (151 vs 147 slip-VBlanks over 1,900
frames). Quantisation did not invert the −4,736 tk/fr tick cut.

**The owner's report is real and its cause is the TOGGLE, not the arm.** The penalty attaches
to running the **non-boot arm**, symmetrically: boot FLOAT then flip to FIXED slips 355 of
800 frames; boot FIXED then flip to FLOAT slips 395 of 600, block-for-block the same. A null
flip (0→1 at frame 400, 1→0 at 401) costs nothing, so it is not the debugger write; per-frame
stops cost nothing either. Flipping *back* to the boot arm recovers in **6 frames**; flipping
*away* never recovers. **Mechanism unidentified** — the particle camera cache is measured
inert across the flip (2 hits + 1 miss per frame on both sides). **Consequence: the toggle ROM
answers the PICTURE question and cannot answer the FPS question. Per-arm cadence needs each
arm as the boot arm.**

**Instrument defect worth one line:** `gNdsBattlePlayablePacingPresentIntervalBucket` is not a
member of `NDS_BATTLE_PLAYABLE_PACING_GROUP` (`nds_startup.h:4184`); it is readable over gdb
only because `DC_FlushRange` cleans whole lines and two group members share the array's two
lines. **ACTION (unowned):** add the `X()` line, so the one histogram `AGENTS.md` requires in
every device A/B cannot go stale under a `diagnostics.c` relayout.

### THE SIM-SIDE SOFT-FLOAT LANE IS SIZED (2026-08-16, zero builds) — 142,786 tk/fr, and POSITION.md's 0.563x was incomplete

`artifacts/performance/2026-08-16_simside-softfloat/SIMSIDE.md`. Same capture, same tool, and
the same method that produced `DRAW_FIXEDPOINT.md`; `analyze-leaf-helper-attribution.py` gained
`--matrix-json` because `--json` collapsed the **helper axis, which is the axis that decides
whether a lane converts**. Marginal-80 mask; every figure below is already a rank-80 figure.

```text
sim-only 45,539 + sim+dispatch 37,665 + shared 59,582 = 142,786 tk/fr = 1.511x of +94,481
op mix:  fmul 29.9% · fadd 29.3% · fsub 16.7%  ->  MAC 75.9%
         fdiv  9.7% · sqrtf 5.7%               ->  transcendental 15.4%
shape:   MAC>=80%  82,798 (64 fns, 71,491 of it WARM >=8 entr/fr)
         mixed     30,199 (115 fns)
         hard>=30% 29,789 (31 fns)
```

**The warm MAC subset is 14 functions, 71,491 tk/fr**: collision/MP 50,044, animation 13,904,
math leaves 7,541. Its two largest are **100% MAC and warm** — `func_ovl2_800ED490` 18,759
tk/fr (19.0 entr/fr, 63 ops/entry, 580 B) and `gmCollisionGetWorldPosition` 13,091 (45.2
entr/fr, 18 ops/entry, 196 B). Both reproduce their source op counts **to the unit**, which is
the check that these are dynamic call counts.

**Rate NOT asserted.** Three in-binary rates span 3× — 1.70x (camera, 3 roots + 9 divides per
entry, 8.1 entr/fr), 2.68x (narrow phase, `__udivmoddi4` ×4/entry, 0.97 entr/fr), 5.14x
(`guMtxCatF` → `ndsRendererMtxMul20p12`, same op, 18.55 entr/fr). **Both named causes of the
two low rates are absent from the warm MAC subset**; that is a mechanism argument, not a
measurement. Range: **29,437 (1.70x) – 57,584 (5.14x) tk/fr = 0.312x–0.609x**.

**Three columns, and the fidelity question is not uniform:**
- **N — bit-exact helper acceleration.** Fidelity-neutral *by construction*: a faster
  `__aeabi_fdiv` / `sqrtf` returning the identical IEEE result changes nothing anywhere.
  Surface 21,886 tk/fr sim + 7,900 draw. **SIZED 2026-08-16 AND IT IS 4,300–4,750 tk/fr,
  not 29,786**: the `sqrtf` half was already shipped (`NDS_R2_FIXED_SQRT` defaults to 1),
  the `fdiv` half is refused on price (the DS unit alone costs 65.0 tk against a 71.5 tk
  `__aeabi_fdiv`), and all that survives is `nds_r2_sqrtf.o` built `-marm`. `HWMATH.md`.
- **P — provable-equivalence fixed point.** The warm MAC subset. Belongs in the
  fidelity-neutral column **iff** the decision-margin proof lands, and the instrument already
  exists: `NDS_R2_CFX_NARROW_DECLINE` (structural, not statistical),
  `grade-r2-collision-live-domain.c` + `probe-collision-fixed-domain.ps1` (bound 0.0200 world
  units against a double-precision reference on live match matrices), `NDS_R2_COLLISION_L7_ORACLE`,
  and the seventeen match invariants.
- **T — rung-3 trades.** The transcendental and mixed remainder, where the *value* is consumed.

**Effect on `POSITION.md`:** 53,215 (0.563x) → **82,652–110,799 (0.875x–1.173x)** if column P
moves. `syUtilsRandFloat` is 229 tk/fr (0.16% of the lane) and is excluded by name.
**Concentration warning:** call volume on the marginal-80 frames is **4.96×** whole match, so a
whole-match or soak measurement of any conversion here under-reads it ~5×.

### THE WARM-MAC EXCHANGE RATE — MEASURED 2026-08-16, AND COLUMN P DOES NOT MOVE

`artifacts/performance/2026-08-16_simmac-exchange/EXCHANGE_LEAF.md`. Two lab builds, seven
whole-match runs, **one binary** (`build-c209-simmac2`, `romSha256` identical on all six
arms, so no placement term), `slips=0` everywhere, arm-0 rank-80 **1,238,912 raw /
1,213,965 net** — 896 from `build-c206-shipgx0`, i.e. the shadow is inert at arm 0.

**The route is a SHADOW, not a replacement**: every arm runs the decomp float body and
keeps its result, then evaluates the fixed form and discards it. So the arms play the
bit-identical match by construction — **all nineteen whole-match invariants are equal on
all six arms**, `gNdsCfxFighterDamagePhaseCalls` 2,684 and `Hits` 26 included — and the
measured delta is the **replacement cost directly**, not a residual divided by a count.

| body | gross tk/entry | fixed tk/entry | **R** | entr/fr | lane | net |
|---|---:|---:|---:|---:|---:|---:|
| `gmCollisionGetWorldPosition` | 289.6 | 348.2 | **0.83x** | 45.2 | 13,091 | **−2,650** |
| `func_ovl2_800ED490` | 987.3 | 987.8 | **1.00x** | 19.0 | 18,759 | **−10** |

**Negative net: the conversion costs more than it deletes.** Counting the float bodies'
own self time in the numerator (the gross is library-only, `CAMERA_Q20_12.md` §4's
convention) lifts them only to **1.00x and 1.17x**.

**THE LAW, and it is knowable from a signature with no build.** An f32↔Q edge conversion
costs **31–42 cycles**, between one `__aeabi_fmul` (26.5) and one `__aeabi_fadd` (38). So a
leaf conversion's rate is decided by **conversions per deleted float operation**:
transform 18/18 = **1.00**, compose 36/63 = **0.57**, `gmCollisionSetInvertMatrix`
24/61 = **0.39** (predicts ~1.2–1.4x), and the 5.14x prior's `guMtxCatF` →
`ndsRendererMtxMul20p12` = **0**, because both sides already hold their native
representation. **That is why the prior never transferred — not warmth, not
transcendentals, not Thumb.** `SIMSIDE.md` §5's two named reasons to expect better than
1.70x are both true here and neither mattered.

**Effect on `POSITION.md`: 0.563x stays 0.563x.** The projected **0.875x–1.173x is
REFUTED for the leaf route** on 44.6% of the warm-MAC subset by size. What survives is the
**chain** route, where `n_conv` is fixed at the endpoints while `n_op` grows — the design
of `nds_r2_collision_fixed.h`, whose only measured instance is the ring at 2.68x, whose
named cause (`NDS_R2_CFX_DIV64` still the portable bit-by-bit divide, hardware unit hook
undefined at `nds_r2_collision_fixed.h:205`) was **addressed on 2026-08-16**: the hook is
defined, the divide is 2.76x–4.50x cheaper, and the ring's rate moves 2.68 → 2.19–2.08 —
still a loss, on a lane whose ceiling is 0.161x at an exchange rate of zero. Column N was
sized the same day and is **0.050x, not the 0.315x its surface suggested**.

**BOTH BODIES ARE UNREACHABLE FROM PORT CODE, and that cost the first build.** A
`#define`-before-`#include` rename moves the definition and `gmcollision.c`'s own call
sites together, so a wrapper sees cross-TU calls only: measured over a whole match,
`func_ovl2_800ED490` has **exactly 0** and `gmCollisionGetWorldPosition` **168 of ~92,000
(0.18%)**. `Makefile:2302` forbids a decomp overlay patch for a new adaptation, so
**converting either one is a caller rewrite, never an interception.** The second build
drives the kernels from the damage-collide gateway (zero in-TU callers, 2,684 of 2,684
captured) with a repeat count in the route word, and reads the price from the slope.

**THE STATISTIC MATTERS AND ALL THREE OBVIOUS ONES ARE WRONG HERE.** The driving seam
fires in bursts, so the per-frame paired median is structurally ~0 (128), the mean is
carried by two cartridge frames (123,628), and the trimmed mean deletes exactly the frames
that carry the work (trim-40: 19,755). **Use the per-ring-stop window sum**: each 96-frame
window carries its own exact evaluation count from `-PerStopGlobals`, and the ratio is
stable — 632.8–714.3 tk/eval over 11 windows, median 668.0, with a 4× density change
(`r16tc` 679.7) agreeing inside 5%.

**Live-domain equivalence, graded at zero fidelity risk** (route word 7, the fixed answer
against the decomp float body's own answer on the same inputs, every captured call):
8,052 components — **6,241 exactly equal, 1,811 off by ONE Q12 quantum, none worse, max
deviation 1 quantum = 0.000244 world units against the 0.0200 bound, 82× inside it**, and
`XfrmDecline`/`CmpsDecline` **0** on every arm. The arithmetic is settled; the population
is one seam's 2,684 calls, not coverage of the call set — and it is moot for the lever
question, because at R ≤ 1.00 there is nothing to trade fidelity *for*.

**Bounds, stated as bounds.** The once-per-driving-call term (compulsory fetch of 2,784 B
of kernel) is bounded at ≤ ~500 tk by the `r16tc`/`r64tc` agreement and is **not fitted** —
`r1tc` at 2.81 evaluations/frame is inside session noise and resolves nothing. Because that
term would be paid **per entry** at the real spread rate, 348.2/987.8 are **lower** bounds
on real per-entry cost and 0.83x/1.00x are **upper** bounds on R.

## Banked baselines — BOTH ARMS RE-BANKED ON THE CORRECTED SEED (cycle 80)

1,600 samples, frames 441–2040, `dldi=ON`, git `34091054`+reseed,
`sample-tick-hud-buckets.ps1 -RingDump` (stride 96), `slips=0` on both.
Builds `builds/build-c80-gate-bothcpu` and `builds/build-c80-boundary`.
**Never take a gate reading on a 128-frame window** — it reads the cheapest 6%
of the match (P95 understated ~306,000, over-gate rate 5×).

| arm | role | coverage | `WORK-H` P50 | P95 | over gate | gap to gate |
|---|---|---|---:|---:|---:|---:|
| **both-CPU** `NDS_R2_BOTH_CPU=1` | **THE GATE** | **86.7%** of 60 s | 1,094,464 | **1,624,064** | 704/1600 (44.0%) | **503,684** |
| **Boundary** mode 163 | shipped configuration | **86.7%** of 60 s | 1,082,112 | 1,476,672 | 673/1600 (42.1%) | 356,292 |

**Superseded repeatedly since.** Cycle 105's arena fix moved the gate arm to
**1,447,318** (gap 326,938) and cycle 108's AObj16 prebake takes a further
**~23,000**. The cycle-108 row carries why that figure is quoted as a range and
not as a single cross-build P95.

**THE FIVE-MINUTE ACCEPTANCE MATCH RAN, 2026-08-13** (`artifacts/performance/2026-08-13_c-stress/
STRESS_GATE.md`), the SwitchPlan §7 owner-instructed exception, on
`builds/build-c132-stress5` (`NDS_R2_SOAK_MATCH_MINUTES 5`, both-CPU, DLDI on,
8,448 samples, frames 439–8887, `slips=0`): **`WORK-H` P50 929,344 / P95
1,205,760**, VBI **2:7415 3:1394 4:58 5+:19 max 26**. **Coverage 98.7%** —
`gNdsBattlePlayablePacingLogicFrames` 17,772 of 18,000 with the guest's own
`time_limit` reading 5. **A five-minute match costs what a one-minute match
costs** (1,210,880 on the one-minute arm, inside the ±14,080 cross-build floor),
so length accumulates no cost; only the 3-VBlank share moves (16.6% against
13.1%). The gate is still 85,760 over and its lane is unchanged. **Do not leave
`NDS_R2_SOAK_MATCH_MINUTES` set anywhere** — that build is a lab directory only.

**CURRENT BANKED GATE — `WORK-H` P50 938,112 / P95 rank-80 raw 1,174,016,
net 1,149,069** (`builds/build-c185-gxcompose-bank`, `NDS_R2_BOTH_CPU=1`,
`NDS_R2_FIGHTER_GX_COMPOSE=1`, DRAW=1, 1,600 samples frames 440–2039,
DLDI ON, `slips=0`; **gap 53,636 raw / 28,689 net** against 1,120,380).
P90 **1,088,192**, top-1% **1,520,832**, over-gate frames **122/1600**.
The net arm subtracts the standing **24,947 tk/frame** of tick-HUD-only apparatus
that the published ROM does not execute (`RESIDUE.md` §5); quote raw and net.
This is a fresh measured level, **not** the prior −17,152 A/B applied
arithmetically. The owner reviewed the new matched-tic pixel diff masks and
accepted the measured 0.0358–0.1742% battle-screen variance; GXSTAT remains
0x06000000, the old low-polygon blink signature is absent, and gameplay
invariants match. **Published GX compose remains pinned OFF.**

Cadence truth comes from the required DRAW=0 sibling
`build-c185-gxcompose-bank-d0`: **VBI 2:1850 3:173 4:8 5+:8 max 19**, total
2039, `slips=0` = **90.731% two-VBlank**, still below the ≥95% acceptance target.

**RENDERER-STATE REDUNDANCY CLOSED BELOW THE 16K PACKAGE FLOOR (2026-08-15).**
Task107 on the c185 configuration measured 146,221 tile syncs with **106,500
exact repeats (72.835%)**: count-scaled ceiling **6,458 tk/fr**. Adding the
already-proven no-Z duplicate gives **7,763 exact local**. Texture binding is
already eliding 95,934 of 208,327 requests; 26,769 of 112,393 actual issues are
same-frame revisits, but they require draw reordering rather than local deletion.
Even a perfect-ordering upper bound adds only **2,484**, for **10,248 total**;
even deleting all 8,867 sync ticks would reach only **12,656**. Zero census
overflows, invariants/GXSTAT match, and the default ROM is byte-identical before
and after the default-OFF instrumentation. **No micro-cut, no re-bank.**
Evidence: `artifacts/performance/2026-08-15_renderer-state-redundancy/STATE_REDUNDANCY.md`.

**`SITR` DECOMPOSITION DONE — IT DOES NOT CLOSE AS A BRACKET; ANIMATION OWNS IT
AND THE PARSE HALF IS A 41,376 tk/fr AOT DELETION (2026-08-15).** Attribution
only: 2 lab builds, 2 v3 captures, 1 Boundary, **0 production source edited, 0
flag flipped, no re-bank**. Arm `build-c192-sitr-profile-gxc` = the c185 bank
configuration to four defines, `GX_COMPOSE=1` via the documented
`NDS_R2_FIGHTER_GX_COMPOSE_LAB=1` escape. Direct children on the marginal-80
mask (≥1,172,523 ticks, basis `cycles/160`), each priced by **measured call
rates against program-wide caller sets** rather than static reachability:

| direct child | marg-80 tk/fr | whole | conc | marg calls/fr |
|---|---:|---:|---:|---:|
| **`ftMainPlayAnim`** (inlined `ftMainPlayAnimEventsAll`, first half) | **89,099** | 57,356 | **1.55x** | 5.49 (3.82 from the root) |
| status callbacks (60 targets) | 62,155 | 25,143 | 2.47x | — |
| root body | 7,986 | 7,567 | 1.06x | **4.00** |
| `ftMainUpdateMotionEventsAll` | 2,998 | 1,166 | 2.57x | 5.46 |
| `ftMainUpdateColAnim` (+Reset) | 1,905 | 1,527 | 1.25x | 6.99 |
| *(`ftComputerProcessAll` = `SCPU`, **subtracted**)* | *12,435* | *12,272* | *1.01x* | *4.00* |
| `ftKeyProcessKeyEvents` / `ftHammerUpdateStats` | **0** | 0 | — | **0.00** |

Named non-SCPU subtotal **164,144 = 63.6%** of the bracket; the 94,052 residual
is the shared leaf pool and is never charged twice (the static SHARED row is
508,694, **1.97x the whole bracket** — the arithmetic proof that reachability
cannot answer this). **THE MECHANISM: the PARSE half — `ndsR2FtAnimParseDObjFigatree`
+ `BuildTrackTable` + `TargetValue` + `AObjToQConvert` — is 41,376 tk/fr at
rank-80** (whole 18,564, conc **2.23x**), **96.94 calls/marginal frame**
re-deriving AObj node fields from **static FIGATREE ROM data**: **1.44x the
+28,689 requirement, 2.59x the 16K floor.** It is a representation change (AOT
typed track rows in the Q form `ndsR2AnimValueQ` already consumes) — **not** the
evaluate half (`gcPlayDObjAnimJoint`+`ndsR2AnimValueQ` **53,818** at 1.40x,
already fixed-point, reducible not deletable), **not** AObj layout, **not**
cadence. Per-call prices for a replacement: parser **272 tk/call** at 17.66
joints per `ftParamUpdateAnimKeys` call, `gcPlayDObjAnimJoint` **273 tk/call**,
`ndsR2AnimValueQ` **70.9 tk/call** at 3.91 AObj nodes per DObj.

**RETRACTION — `SITR` = 310,662.4 marginal-80 is ONE INSTRUMENT FRAME.** Frame
**756** carries a 2^22 event in `SINT` (4,163,136 over median, 0.74% off
4,194,304), sits inside the c185 raw top-80, and alone contributes **+50,309**.
The bracket is **260,354** (DRAW=1, 79 frames) / **258,196** (DRAW=0, 80) — two
arms 0.83% apart. **Task 108's callback verdict is untouched and strengthened**
(6,373.5 / 14,395.5 = 2.5% / 5.6% of a 16%-smaller bracket);
`NDS_TASK108_SITR_CALLBACK_CENSUS` stays default OFF. The banked rank-80
1,174,016 is a percentile and is **not** adjusted.

**INSTRUMENT TRAP, MEASURED AND NOW IN THE TOOL'S DOCSTRING: the profile's
frame→region map is `frame − 439`, not the banner's `− 438`.** At `−438` the
c185 rank-80 frames land at median profile rank **454 of 1600** and every `SITR`
row reads *below* its whole-match rate; at `−439`, median rank **43**, 63 of 79
inside the profile's own top-80, and every row reproduces within 5% (PARSE half
**40,003** vs 41,376). Lags −1/+2 are as bad as 0. Separately, the two
instruments never share a frame bracket at all: three profile arms correlate
**r ≥ 0.982** with each other, a profile against a tick-HUD arm peaks at
**r ≈ 0.67**. **Cycle 109's "~96% in `ftMainPlayAnim` + `ftComputerProcessAll`"
is half refuted** — right about the owner, wrong about the size (39.3%, and the
AI half is subtracted out). **Next: animation representation, consuming these
numbers.** Evidence:
`artifacts/performance/2026-08-15_sitr-direct-children/SITR_DIRECT_CHILDREN.md`.

**ANIMATION REPRESENTATION, STAGES 3–4: THE DENSE RUNTIME IS WIRED, ITS ORACLE
IS 0/12,232, AND THE A/B IS TICK-NEUTRAL — PLUS TWO REAL DEFECTS (2026-08-15).**
4 lab builds, 4 gate runs, no default flipped (`NDS_R2_FTANIM_TRACK ?= 0`), **no
re-bank**. Evidence:
`artifacts/performance/2026-08-15_ftanim-dense-runtime/DENSE_RUNTIME.md`.

- **DEFECT 1 — a shipped one-frame segment-phase regression, REPAIRED.**
  `69ce92e279f` hoisted `-anim_wait - anim_speed` into `len_new` in all four
  write cases of `ndsR2FtAnimParseDObjFigatree` and assigned it in **one**.
  `git show 69ce92e279f^` has the fresh expression at all four sites; the decomp
  writes it fresh in each case. **Opcodes 4+5 = 45,679 of 55,261 items-off write
  commands (82.7%) sit in a case that never assigned it**, so segments started at
  phase 0 and the first evaluated sample landed a frame inside the segment.
  **This changes the shipped ROM and the fight**: spark 15→16, shield
  1,352→480, AObj high-water 1,266→774, packHits 197→257 (P1Damage 76 and
  runaway 0 unchanged). **No tick comparison against the c185 bank is
  like-for-like any more — a fresh bank on the repaired tree is REQUIRED before
  the next performance verdict.** `build-c193-segfix` reads rank-80 1,228,608.
- **DEFECT 2 — a shared-decoder blind spot, found by the on-target oracle.**
  `ftanim_reloc_probe.decode_script` recorded `targets` only for commands
  carrying per-track words, so opcode 11 `AddLen` looked like it touched no
  track; `run_commands` iterates the same field, so reference and candidate
  agreed on the wrong answer and layers A/B/C could not see it. Fixed at the
  decoder. Corpus hash `cb28f9bf65c4` → **`64b2f5a6a7e8`**, sizes unchanged,
  three-layer proof re-run PASSES (0 mismatches, falsifiers 1/1/4,272).
- **Stage 3 wired**: bind once at `lbCommonAddFighterPartsFigatree` keyed by
  figatree ENTRY index; step typed rows with no command decode, no flag scan, no
  per-call AObj list walk, no per-call Q migration, no `ftAnimGetTargetValue`.
  **Engagement exact on one ROM**: generic parse calls 144,383 → 115,288 =
  **−29,095**, dense counters **24,197 + 4,898 = 29,095**, control hard zero.
- **Stage 4 oracle 0 mismatches over 12,232 decision points**, fail-closed, and
  it FIRED (4) before defect 2 was fixed — a control that can fail.
- **A/B TICK-NEUTRAL**: same ROM, one poked word, zero placement floor. rank-80
  1,231,872 → 1,232,000 = **+128**, P50 +576, trimmed mean +1,261, `SINT` +77
  trimmed / +1,664 rank-80. **A partial conversion cannot win**: the generic
  parser still serves 79.8% of calls so its bytes stay hot, and the dense
  stepper's 3,368 B code + 12,244 B rows are pure fetch ADDITION. **The 33,951
  mechanism is not refuted; this configuration is.**
- **Coverage 8 of 137 Fox clips** because the pack is `.rodata` and the real
  budget is `gNdsTaskmanGeneralHeapFreeMin` **53,136 − 32,768 reserve = 20,368
  B**, not `check-boot-headroom.ps1`'s 312,448 ladder figure. Free-min ends at
  40,848. Ascending-asset-id selection buys **20.4% of clip binds from 5.8% of
  clips**.
- **NEXT**: full coverage = the pack in the taskman ARENA in place of
  `battlepack_fox.bin` (stage 2's proven −822 B drop-in), a cross-build A/B, and
  a fresh bank. **No v3 capture was taken**, so the neutrality is unattributed
  between fetch cost and off-percentile deletion; one v3 run on the same
  byte-identical pair splits it with no rebuild.

**The previous bank — `WORK-H` P50 923,392 / P95 1,210,880** (slice 50,
`builds/build-c131-cand`, same configuration), superseded by the anim-joint fix's +49,216.
Slice 50's own control measured HEAD at
939,392 / **1,219,520**, i.e. the tree had drifted **+8,960 P95** above the
cycle-121 bank before this cut — quote a control from the same tree, never a
bank, when sizing a candidate.

**The previous bank, for the record — `WORK-H` P50 931,648 / P95 1,210,560**
(cycle 121, `builds/build-c121-stride`, same configuration).
Slice 44 re-banked it from 1,244,480: **P95 −35,904 / P50 −17,088**
against a matched control from the same tree, 4.2× and 2.0× the ±8,544 floor,
and the largest single P95 move of the campaign. **Gap to the 1.12M target:
~90,180.** Evidence:
`artifacts/performance/2026-08-11_c121-slice44/SLICE44_GATE.md`. VBI 2/3/4/5+
(max): 1644/343/35/16 (20), better than the control's 1621/361/38/18 in every
bucket. Boundary's row below is **pre-slice-44** — re-measure before quoting it.

The paragraph that follows is the cycle-118 bank it replaced, kept for the
reconciliation it records.

**SUPERSEDED BANKED GATE — `WORK-H` P50 961,152 / P95 1,294,144** (cycle 118,
`builds/build-c118-gate`, `NDS_R2_BOTH_CPU=1`, 1600 frames from 438, DLDI ON,
`slips=0`). Re-banked after slices 35–37: **P95 −10,752 and P50 −8,960 against
1,304,896 — the first cross-build movement to CLEAR the ±8,544 placement floor
since Requirement 4.** It reconciles with the route arms, which predicted
−14,400; the 3,648 difference is inside the floor. **Gap to the 1.12M target:
~174,144.** Engagement in the SHIPPED configuration matches the candidate arms to
the call — endpoint 48,082, yakumono 71,340, kind 57,909 — and
`gNdsR2FtAnimParseCalls` is 145,549, the same as every A/B arm, so the banked ROM
is running all three memos and doing identical simulation work.

VBI 2/3/4/5+ (max): both-CPU 1127/808/91/14 (20); Boundary 1194/784/51/11 (20).

**The corrected gap is 503,684, not 485,060** — the early-match window was
optimistic by 18,624, exactly as predicted. Every superseded both-CPU figure is
now replaced above; the old ones are in the cycle-79 archive section.

**THE WINDOW RUNS PAST THE BUZZER ON BOTH ARMS — 43 frames, and they are cheap.**
Frames 1998–2040 are post-buzzer GAME SET on *both* arms (contiguous, `SRC` <
50,000 against medians of 355,328/300,736; gate-arm mean `WORK-H` 711,751). They
are 2.7% of the window and they drag P50/P95 *down*. Gate arm, gameplay-only
(441–1997): P50 1,100,096, P95 1,631,936 — 7,872 above the full-window P95, just
outside the ±5,376 floor. Both arms carry the identical tail, so cross-arm
comparison is unaffected; a single-arm figure should say which it quotes.

Boundary's own numbers: it trails the gate by 356,292. **Its `WORK-H` P95 reads
+13,568 against the older banked `f24f0cc1` figure of 1,463,104, and that move
predates this row** — this cycle's Boundary run reproduces the cycle-79 run
*bit-identically* on every shared bucket (`ALL` P50 1,119,872 / P95 1,680,192 /
mean 1,399,603, `named` 1,149,672, `SRC` P95 547,648), so the reseed did not
touch it. The delta sits between `f24f0cc1` and the c79/c80 builds and is
unattributed; treat 1,476,672 as the current Boundary baseline. The shipped ROM
remains
`smash64ds-battle-playable-hwtri.nds` (Boundary, mode 163). Label every figure
with its arm AND its coverage; never present a both-CPU figure as the Boundary
number (`Makefile:305-308`). Loading states are excluded from the gate by the
owner's stated rule: drop frames with `SRC` > 2× that arm's own `SRC` median.

Noise floors — **recalibrated whole-match, cycle 100; the old ±5,376 was a
128-frame-era number and is far too tight**:

- **Same binary, same invocation: ZERO.** A whole-match run reproduces
  **bit-identically** — six runs over three binaries, rows-CSV SHA256 equal
  across every repeat pair. A figure that does not reproduce exactly means the
  invocation or the binary changed, not that the host was noisy.
- **Cross-build `WORK-H` P95: ≥14,080, and the sign is not reliable.** The same
  change measured on three A/B pairs moved P95 by −8,832, −2,368 and **+5,248**.
  Do not decide anything on a P95 delta below ~14,000, even between builds that
  link at the identical address.
- **Cross-build `WORK-H` P50: ~5,700**, and P50 kept its sign in all three
  pairs. **Rank on P50, mean and over-gate count; P95 is the gate's definition,
  not a usable A/B discriminator at these magnitudes.**
- Per-bucket placement ≥8,544 — buckets locate, `WORK-H` decides. 1.85 cycles of
  `FTR` mean per byte of added ARM text: a change that adds text must beat its
  own footprint.

### The load-frame exclusion does NOT select loading states — do not apply it (cycle 81)

The banked figures above are correct **because they are taken with the
exclusion OFF**. `scripts/analyze-load-frame-exclusion.ps1` audits the rule
("drop frames with `SRC` > 2× that arm's own `SRC` median"); artifacts
`artifacts/performance/2026-08-05_c81-load-exclusion-audit-{bothcpu,boundary}.json`.
Four findings, all from banked CSVs, no new run:

- **The rule is circular for SRC.** It thresholds on the very bucket whose
  excursion is being attributed, so it shrinks SRC's share whether or not a
  load happened: applying it moves gate-arm `SRC` 68.9% → 52.3% and `MISC`
  25.7% → 39.6%. **Never rank SRC with it on.**
- **It is fragile.** Gate-arm gap by threshold: OFF 503,684, 1.5× 163,332,
  2× 237,956, 2.5× 309,252, 3× 384,964, 4× 484,292 — a **3.08× swing on the
  knob alone**.
- **The dropped frames are not loads.** 122 frames form 110 runs, **100 of
  them singletons**, longest 4, spread evenly (43 early / 42 mid / 37 late)
  and **0 overlap the GAME SET tail**. A loading state is a contiguous
  multi-frame event. Their other buckets are ordinary — `FTR` 1.01×, `STG`
  0.99×, `MISC` 1.04× — and only `SRC` is elevated (2.80×).
- **Cross-arm asymmetry settles it.** Both arms run the same stage, fighters
  and assets, so a real loading filter must bite similarly. It swings the gate
  arm 3.08× and Boundary only **1.09×** (356,292 → 327,236) — precisely
  because the gate arm's tail *is* `SRC`. The rule tracks the tail, not loads.

**Consequence: the gate is 503,684, not 237,956**, and the owner's intent
(exclude genuine loading) needs a rule keyed on an actual load signal — the
anim-cache warm step or the Task 75 asset-load counter — not on `SRC` itself.
That instrument does not exist yet; see the inherited row below.

## The diagnosis the lane was built on — **BOUNDARY-ONLY, re-priced 2026-08-05**

**Every figure in this section is a Boundary-arm figure.** It was banked without
an arm label, and cycle 79 measured it on the both-CPU arm the owner's gate
actually reads (G2a, commit `62fe823d`): the prize is 4–9× smaller there. Do not
quote these numbers as gate-arm numbers, and do not rebuild G3's case from them.

- **Effect DObj submits are the tail** *(Boundary)*: 99.3% of the `MISC`
  excursion, 359,717 ticks/frame on over-gate frames, **0** on clean ones. Net
  recoverable ~315,000 (part displaces `FTR`).
  **On the both-CPU gate arm: 71.5% of the `MISC` excursion, and `MISC` is only
  16.9% of the WORK-H excursion — so effect submits are ~12.1% of it. Measured
  recoverable 33,699–75,264, not ~315,000.**
- **The cost is a per-list constant** *(Boundary)* (~102,730 Exec ticks/list,
  1,360 lists/match, 16.1 tris/list): exact nine-phase partition — generic DL
  interpreter 65.57% (77,440/list), texture resolve 21.41% (25,289/list),
  Matrix 6.93% (8,179/list), everything else ~5.8%.
  **The constant does not hold on the gate arm: 527–563 lists/match at 83,632
  ticks/list (44,073,856 total, versus Boundary's ~139,714,000) — 41% of the
  lists and 81% of the per-list cost.**
- **The interpreter is honestly generic**: every list terminates at `G_ENDDL`
  (1,360/1,360, none at the 8192 cap), 160.1 commands/list at **626
  ticks/command**. No overrun to fix — **the precompiled-packet path is the
  answer, not a workaround.**
- **Dead, do not re-derive**: projectiles (44 ticks/frame median); particles
  (flat ~47K, a P50 lever, never the gate — and **rate-reducing them is closed
  twice over, 2026-08-13, no build spent**:
  `artifacts/performance/2026-08-13_c-particle-rate/REFUTED_QUARTER_RATE.md`.
  `MISC` is a **draw residual by construction** (`taskman_seam.c:5104`) so its
  17,152 never priced the update half; that half lives in `SRC`, is **7,364
  tk/frame**, and quarter-rating it prices **−7,493** — deleting it outright
  −8,987, and deleting **every particle in the game, update and draw, −33,818**.
  It is also **forbidden**: the particle/effect update draws from the **same
  single LCG as the level-3 CPU AI** (`sSYUtilsRandomSeed`; `ftcomputer.c` 65
  draw sites, `efmanager.c` 44, `lbparticle.c` 26), so any cadence change shifts
  the AI's stream and diverges the match. **Check any sub-rating proposal for
  `syUtilsRandFloat` before designing it.** What survives is the particle *draw*
  half — 27,758 tk/frame, **−30,676** — and that is the owner's fidelity call);
  texture thrash (1 upload/1,408
  frames — `Tex` is cache-*hit* key/hash/lookup cost); `Find` 0.44%; `Material`
  0.25%; `FTR` as the gate (anti-correlated with the tail); the `Tex`
  (dl-pointer, bind-ordinal) memo (built as approved: 4.56% hit rate, `Tex`
  went *up* 20% — reverted, flag deleted); L7 fixed-point collision (+534 won
  vs 6,481 lost to its own text); **the `SHDT` per-joint reach bound** (slice
  47, 2026-08-12: interposed measure-only, `ReachTests` 2,373 / `WouldSkip`
  **0** — the chain sum is arm's-reach-wide and
  `gmCollisionCheckFighterInFighterRange` already put the attacker inside it,
  so no inflation constant fixes it and a tighter bound needs the transform
  being skipped); **the `SHDT` pair-level broad-phase reject** (2026-08-13, no
  build spent — `artifacts/performance/2026-08-13_shdt-broadphase/`: the
  source's `k == 0` early-out at `ftmain.c:3076` already rejects **≥94.68%** of
  the 6,232 pair evaluations a match and the range test ≤2.41% more, so
  **≥97.09% never reach a per-part test** — 331 range tests, 1,987 hurtbox
  tests, **0** shield tests, 14 hits in the whole match; the entire fighter-pair
  path is 2,666 tk/frame = **18.7%** of a lane needing 26.6%, so deleting
  fighter-vs-fighter hit detection outright pays ≈11,200. Carry:
  `gmCollisionTestRectangle` serves item/weapon/ground too — **never attribute a
  shared leaf to one caller** — and **`analyze-leaf-helper-attribution.py`
  pointed at ordinary functions is an exact call-count funnel for any call
  chain, off a profile that already exists**); Task 56 strips (ROM hangs the
  present loop).
- **"Asset loads as the tail owner" was refuted three times AND slice 46 still
  won 17,216 there** — the LANE was correctly refuted, but a specific defect in
  it (the warm list had drifted to cover only 57 of the 87 animations the match
  uses, and the walk never finished) was real. A refuted lane is not a refuted
  bug; re-read the lane's own counters on the current arm before believing it.
- **The ROM has 96 BYTES of proven headroom — not 1.4 KB (corrected cycle 82).**
  The old "+1,408 boots, +2,208 does not" was a **delta over a datum build, and
  the datum moved every time the tree grew**: the tree had already spent 1,312
  of that 1,408 by cycle 80, so quoting the band on a later tree overstates the
  room by ~14x. Price it in **absolute `fake_heap_start`** (end of `.bss`, hence
  the heap base; address delta == text+data+bss delta, verified to the byte).
  Highest address ever proven to boot **`0x02294804`**; lowest proven to fail
  **`0x02294b24`**; the 800-byte band between them has never been bisected. The
  current gate-arm control links at **`0x022947a4`**. **Text counts as much as
  bss.** `scripts/check-boot-headroom.ps1 -Build <dir>` reads any build's ELF and
  returns OK / UNPROVEN / OVER CLIFF (exit 1) in under a second — run it after
  every lab build, then the 8-sample `-StartFrame 60` boot probe (~50 s) only if
  it says UNPROVEN. A failing arm never reaches presented frame 1 and the
  harness reports a timeout, which reads exactly like a hung emulator.

## SLICE 1 PHASE 7 IS MEASURED, ITEMS 3 AND 4 ARE ANSWERED, AND THE RE-RANK SIZES SLICE 2 (2026-08-15)

`artifacts/performance/2026-08-15_k0-rerank/K0_RERANK.md`; kernel doc §13.
One build (`build-c171-k0-5min-bp1`), one five-minute gate run, one Boundary.

**PHASE 7 — six of the seven K0 lines read exactly ZERO for the packed fighter,
against a control in the SAME RUN.** Ten `volatile u32[2]` counters, one per K0
line plus two denominators, incremented **at their own sites** (`NDS_K0_MARK`,
`include/nds/nds_reloc_assets.h`), keyed by the asset's fighter and gated on
`gNdsK0BattleInGo` — published once per logic update from a `game_status` read
both taskman update loops already perform. Only one fighter fits the arena, so
Mario stays generic and *is* the negative control.

| K0 line | Mario (control) | **Fox (packed)** |
|---|---:|---:|
| acquisitions after GO | 812 | **999** |
| served by the pack | 0 | **999** |
| FAT reads | 21 | **0** |
| `get_fat` / `f_lseek` | 42 | **0** |
| payload byte-swaps | 21 | **0** |
| relocation / fixups | 812 | **0** |
| AObj16 normalizations | 42 | **0** |
| raw cache copies | 791 | **0** |
| **token → asset-id resolves** | 812 | **999 — NOT deleted** |
| asset → path lookups | 833 | **0** |

`lbRelocGetForceExternHeapFile` resolves `ndsRelocAssetIDForToken` **before** the
pack is consulted, so K0 line 7 is **half** discharged. Ordering fix, unpriced,
do not round it away. Cross-checks: `BattlePackHits` 999 == packHits[1] ==
acquisitions[1] (**zero fall-through**), `AnimCacheHits` 791 == cacheCopies[0],
seeks[0] == 2 × fatReads[0], whole-run `PayloadReadCount` 121 vs 21 attributed.

**ITEM 4 CLOSED — and the match length is read out of the guest.**
`time_limit` **5**, `PacingLogicFrames` **17,772 of 18,000 = 98.7%**, 8,886
presented, **slips 0**, `WORK-H` P50 946,944 / **P95 1,198,720** vs the 1-minute
bank 1,177,920. **Length does not accumulate cost** (third independent time).
Heap low-water **51,876** across five times the match (1-minute 52,472/52,864);
every allocator gate 0; `sGCCommonsMaxNum` −1.
**`soak-freeze-watch.ps1` CANNOT DO THIS ITEM**: `-MinutesToRun` ceils at 12.0
and one game minute is ~136 s of wall clock. Use the tick-HUD sampler, as the
2026-08-13 battery did — it also reads the guest timer, the histogram, the
allocator gates and the K0 counters from the same run.

**ITEM 3's 4.1 PRODUCT POINTS ARE TWO THINGS, AND ONE OF THEM THE GATE CANNOT
SEE.** `DRAW=0` deficit = **70 frames**. **55 are WORK-H-bound** (mean overrun
36,982 over the 1,113,152 boundary; excess **81.2% inside `gcRunAll`** — `SITR`
33.7%, `SPHD` 18.1%, `SHDT` 13.8% at 5.8x, `AUD` 10.6%, draw side 8.1%).
**15 are not**: WORK-H under the boundary, **13 of them carrying a HUD-bucket
burst**. On a `DRAW=0` arm that bucket is `gNdsTickHudForegroundTicks` alone =
**the game's own battle-interface OAM draw** (`sprite_preview_backend.c:3032`),
**1.41 bursts/s, mean 100,853, dropping 47% against an 8% base rate.**
**`WORK-H = (ALL − WAIT) − HUD` by construction**, so ~7,128 tk/fr of shipped
product cost — up to ~100,853 on the cadence frames — is outside the gate number.
Recorded, **not** re-scored: `plan.md` §0 owns the scoring.

**THE RE-RANK, on two independent populations** (banked 1-minute arm, 80 P95
frames; this cycle's 5-minute arm, 423 — 5.3x the sample, different fight):

| owner | 1-min | 5-min |
|---|---|---|
| `SRC` = `GCRA` | 84.8% | 85.7% |
| **`SITR`** | **36.0%** (2.46x) | **31.6%** (2.19x) |
| `SHDT` | 20.5% (**16.34x**) | 27.0% (**17.02x**) |
| `SPHD` | 15.2% | 11.3% |
| `SPRM` | 8.3% (**20.40x**) | 10.2% (**23.15x**) |
| **`GCRA-REM`** | **3.6%** (1.17x) | **4.4%** (1.21x) |
| draw side | 9.3% | 10.7% |

**This RETRACTS `GATE_ARM_OWNERS.md`'s "the sub-`SRC` ranking is match-specific,
only `SITR` survives"** — all four survive within one position across two matches.
**And it kills a false draw owner:** `STG` reads +53,383 / 10.2% on the 1-minute
set and **that is ONE FRAME** (1937); excluded it is **+840, 1.00x, 0.2%**.

**SLICE 2 IS SIZED AND IT IS 1.8x SHORT — do not brief it as the next mechanism.**
§K2's "~90% of the tail excess is inside `gcRunAll`" is true *because the fighter
process bodies are inside `gcRunAll`*, and slice 2 leaves those untouched. The
bracket that isolates what it can touch is `GCRA-REM`, a **flat** 1.17–1.21x lane.
From the gate-arm v3 capture, the whole scheduler machinery on the P95 frames is
`ndsBaseGcRunAll` **9,550** + `gcRunGObjProcess` **5,988** + `gcRunGObj` 1,255 +
`gcRunAll` 491 + graph maintenance 502 = **17,786 tk/fr**, against **+32,593 net**
required — a 100% deletion is 1.8x short and a flat vector still calls each
process. (Sizing is on `build-c159-profile-bothcpu`, the only gate-arm v3 capture;
slice 1 never touched the scheduler, so it carries — but it is a sizing, not a bank.)
**The ranking points at `SITR`, `SHDT` on the P95 frames, and the soft-float trio
(99,762 tk/fr on the P95 set = 3.0x the requirement, 38% inside the fighter procs).**

**THE MULTI-MEGATICK FRAMES ARE CHARACTERISED AND THEY ARE NOT WORK.** Every one
carries an excess of **2^22 ticks (4,194,304 = 0.1252 s)** in whichever single
bucket was open — ±0.02% in the tight cases, six arms, six buckets, ~1 per 2,100
presented frames, `SINT` over-represented 6 of 14 against 13.9% occupancy. `ALL`
rises with it, so the guest timer really advanced. **Predates the pack** (`c158`,
2026-08-14) and is **not a function of game state**: the `DRAW=1` and `DRAW=0`
arms of the *same build* put them on completely different presented frames, and
two arms have none. Never moved a banked percentile. Leading hypothesis
(unmeasured): a fixed-duration I/O stall. **Discriminator is free on the next gate
run — `-PerFrameGlobals gNdsRelocAssetPayloadReadCount,gNdsR2AnimCacheFills`.**
`--drop-frames` added to `census-tick-hud-p95-set.py` for the attribution
falsifier, with the never-bank-a-percentile prohibition in its help text.

**PHASE 6 AS WORDED IS RETIRED FOR SLICE 1** (kernel doc §13.2). It assumes two
evaluators; slice 1 shipped one. The binding obligation is a **representation**
one and it is discharged in two tiers (host side over **all** clips at mismatch 0
with a falsifier that fails; end-to-end on the exercised subset), with the
residual **named**: coverage of clips no match has requested, closed by one
distinct-packed-clip counter — **not by building a one-armed oracle.** The
original wording is inherited unchanged by slice 3, which will have two arms.

**Boundary GREEN at the shipping default** on this tree, 0 `Exception:`, marker
capture 28 s of the 120 s ceiling, `frames=212`.
**`BLOCKED(decision: shipping default)` is now ready for the owner** — items 3
and 4 answered, phase 7 measured — but **item 3 is still NOT MET** (90.7–90.9%
two-VBlank against ≥95%) and nothing was flipped.

## MARGINAL-FRAME OWNERS (2026-08-14) — the ranking Phase 4 selects from, zero builds

> **The sub-`SRC` ranking below is superseded by the 2026-08-15 re-rank above**,
> which reproduces `SITR`/`SHDT`/`SPHD`/`SPRM` on two matches and two window
> sizes, and shows that one instrument frame was carrying `STG`'s apparent share.

`artifacts/performance/2026-08-14_runtime2-p95-closure/MARGINAL_OWNERS.md`.
Reducer `scripts/census-marginal-frame-owners.py`; its two cached per-PC CSVs sit
beside the document so **nothing re-scans the 3.46 GiB v3 profile again**.

**The join `plan.md` §5 assumed does not exist.** The only v3 stall capture in the
repo (`…/2026-08-14_icache-temporal/v3-baseline`) is `builds/build-c125-profile`,
**`NDS_R2_BOTH_CPU 0` / `NDS_TICK_HUD_DRAW 0`** — the Boundary arm with the
instrument burst compiled out — while the c147 rows are `BOTH_CPU 1` / `DRAW 1`.
Different arm, different binary, different match. Every other profile on disk is
`profile-v2` and carries no stall classes, including the one gate-arm capture
(`2026-08-12_c123-rebank/profile`, `build-c123-profile`). Two analyses were run
on the two axes the captures actually support, and never joined.

**Gate arm, from the c147 rows alone (right arm, right instrument).**
`WORK-H = (FTR+STG+BG+AUD+SRC+MISC)+(OTHR−WAIT)` closes to **0.0** on both
populations, and `WORK-H = WORK − HUD` exactly.

| set | n | WORK-H excess vs the 1,360 two-VBlank frames | SRC share |
|---|---:|---:|---:|
| **P95 set** — the 80 largest `WORK-H` (P95 of 1,600 IS the 80th-largest) | 80 | **+520,718** | **92.1%** |
| cadence set — the 160 cheapest dropped frames | 160 | +104,117 | 83.8% |

P95-set owners, nested and not double counted
(`SRC ⊃ GCRA ⊃ {SINT ⊃ SCPU, SHDT, SPHD/SPHC, SCAT, SPRM}`, resolved from the
bracket sources, with `SRC − GCRA` measuring **−68** here and **−64** on the
cadence set):

```text
SRC   +479,816 (92.1%)   GCRA +479,885   SRC outside GCRA  -68
  SINT +178,455   SCPU +7,222   SITR = SINT-SCPU +171,234
  SHDT +119,920  <- 19.2x a two-VBlank frame, the sharpest presence in the table
  SPHD +112,833   SPRM +49,377 (25.8x)   GCRA remainder (SOBJ) +19,141
MISC  +16,414    AUD +15,775    FTR +7,987 (1.03x)    STG +546 (1.00x)
```

**The draw side is 4.8% of the P95 excess.** That is a live tension with the hot
instruction-footprint lane below, which is sized off whole-match `icache_fill`:
it is a P50 lever unless its win lands inside `gcRunAll`. Stated, not resolved.

**The cadence set is 64% instrument.** 102 of the 160 are already under the
1,116,096 cadence boundary in `WORK-H`; 98 of those carry the HUD draw burst.
Only **58** are `WORK-H`-bound and they need a mean **43,916** — 94,848 is the
worst frame, not the set. A burst frame presents in two VBlanks 3 times in 1,360.
Cadence acceptance therefore still turns on `plan.md` §6, which is the owner's.

**The 123,773 pool is not a lane.** Reproduced to the tick (123,772); largest
holder 9.1%; every top holder already owned by a named lane, including
`tickGetCount` + `cpuGetTiming` = 13,406 tk/fr of apparatus. Only `memset`
(10,570 write_buffer) and `ndsRendererSyncTextureTile` (3,945) are pool-shaped.

**Rule this cycle re-earned:** rank the whole distribution and cut where the
percentile sits. A change removing 100,000 from only the top 20 frames moves P95
by **zero**; the 80th-largest must fall **91,844**.

## NATIVE BATTLE KERNEL SLICE 1 — PHASE 5 IS RESIDENT AND MEASURED (2026-08-15, `3963b8b14ea`)

`artifacts/performance/2026-08-15_battlepack-resident/BATTLEPACK_RESIDENT.md`.

**`gNdsBattlePackHits` has been read on a live ROM.** Gate arm
(`NDS_R2_BOTH_CPU=1`), canonical one-minute match, 2,043 presented frames on both
arms, same target and build recipe:

```text
                       flag 0      flag 1
gNdsBattlePackHits          0         197
gNdsBattlePackMisses      357         160
total acquisitions        357         357     <- identical: the cost changes, not the count
gNdsR2AnimCacheHits       338          30
gNdsR2AnimCacheRejects      0         126
ArenaReservedBytes    262,144     292,032
TaskmanArenaChosenSize 0x150000   0x150000    (AllocFailCount 0 both)
```

`gNdsRelocResolveOffsetCount` 0 → **3,132** corroborates from the other side.
Per-fighter split, measured: gate arm **197 Fox / 160 Mario**; Boundary-style arm
**169 / 52**.

**`.incbin` is gone.** NitroFS payload streamed into the taskman arena in 18×16 KB
chunks at the `ndsR2AnimCachePreloadStep` seam. Proven static headroom **66,816
→ 354,208** against a 355,104 flag-0 baseline: **+896 B of image, not +288,288**.

**THE MARKER-2 BOUNDARY HANG WAS THE ARENA.** Three timeouts last cycle at
`ndsRendererHardwareArmBattleStaticTextures`; this cycle the capture costs 27.1 s
and 26.7 s of a 120 s ceiling. Proven with `gNdsTaskmanArenaChosenSize`
(1,310,720/16 fails → 1,376,256/0), not by observing that the red went away.

**Flag 1 was RED on ONE assert — FIXED 2026-08-15 (`79a9447fd6d`), and the
recorded direction was BACKWARDS.** R2-04 E2 is a **debugger-coherency defect,
not a torn write**: melonDS's stub reads through `ARMv5::ReadMem`
(`melonDS-Accurate/src/ARM.cpp:1545`) → `BusRead32` with **no DCache lookup**, and
ARM946E-S does not write-allocate. `X10` is the only store before the
`SampleCount` read-modify-write, so it misses a non-resident line and reaches
RAM; the load then fills that line and the following three stores hit it, mark it
dirty and abort the bus write. **`X10` LEADS by one publication; it does not
lag.** Measured frame by frame (`scripts/probe-fpshud-publication.ps1`, no build):
420 presented frames, 29 publications, **3 X10-only transitions**; the same probe
reads sc=14 → x10=**298** and sc=15 → x10=**289**, exactly the pair the assert
reported as one group. Fix is `DC_FlushRange` at the publication seam
(`ndsPlatformPublishBattleFpsHudGroup`), per global so it cannot depend on the
linker keeping four objects adjacent: **0 splits, 0 inconsistent samples** after.
**Boundary GREEN at flag 1 and GREEN at flag 0**, 0 `Exception:` both.
`NDS_R204_FPSHUD_SHADOW` was not spent — the probe answered its question free.
**The general rule is now in `docs/VERIFYING.md`: a GDB read misses the D-cache
in both directions, so a value read within a line-lifetime of its last write can
be stale and a group published at a seam can be read torn.**

**ORDERING WAS NOT ENOUGH — the finding worth carrying.** Letting the loader be
the first caller of the bump allocator lost by **848 bytes**: fighter setup stores
3,728 B into the arena before the first scene update
(`ArenaOverflowLastSize 287936`, `LastUsed 3728`, `Hits 0`). The blob now owns
`[0, RESERVE)` of every arena generation, carved at reservation, and
`NDS_R2_BATTLEPACK_BLOB_BYTES` is generated from the blob so the reserve cannot
drift from the asset.

## THE ARENA IS AFFORDABLE AND THE BLOCKER MOVED TO THE INSTRUMENT (2026-08-15)

`artifacts/performance/2026-08-15_battlepack-arena-price/ARENA_PRICE.md`.
**Zero lab builds** — two soaks on binaries that already existed, four Boundary
runs. The section below stands; this one answers its closing question.

**+172,032 B of arena does not BUY room — it REPAYS the pack's reservation.**

```text
                                 control (flag 0)     arm G          delta
NDS_TASKMAN_ARENA_SIZE              1,376,256      1,548,288      +172,032
  animation-arena reservation         262,144        451,776      +189,632
    blob (287,904 -> 32B line)              -        287,936
    raw file cache                    262,144        163,840
arena left to taskman               1,114,112      1,096,512       -17,600
```

Predicted −17,600 of general heap from the constants alone; **measured −17,472**.
One mechanism, fully accounted, nothing hidden — and the *opposite* of how the
growth reads.

**The stress battery, on `build-c168-packfix-bp1` itself** (the binary whose
rank-80 is banked, so the reserve and the tick figure share one ROM), 660 s:
**12 battle-scene entries · 7 completed matches · 7 START restarts · 4 Sudden
Deaths · NO-FREEZE.**

| counter | control (flag 0) | **arm G** | requirement |
|---|---:|---:|---|
| `gNdsTaskmanArenaChosenSize` | 1,376,256 | 1,548,288 | == requested |
| `gNdsTaskmanArenaAllocFailCount` | 0 | **0** | == 0 |
| `gNdsR2AnimCacheArenaReserveFailCount` | 0 | **0** | == 0 |
| `gNdsR2AnimCacheRejects` / `Overflows` | **21 / 21** | **0 / 0** | == 0 |
| `gNdsSyMallocOverflowCount` | 0 | 0 | == 0 |
| `sGCCommonsMaxNum` | −1 | −1 | cap unfired |
| **`gNdsTaskmanGeneralHeapFreeMin`** | **69,872** | **52,400** | > 32,768 |

**Arm G refuses nothing where the shipping control refuses 21 animation loads** —
two fighters do not fit 262,144, one fits 163,840 (peak 137,136, 83.7%, 26,704
spare). The low-water is **flat across the chain** (single match 52,864) because
`syTaskmanStartTask` rewinds the general heap on every scene entry, so Sudden
Death's extra per-player figatree heaps do not accumulate.

**Three reserves, each with its instrument, because they are different pools:**
**(a)** grantable libnds heap — **16,384 B** under the measured 1,564,672 ceiling
(inherited from a same-footprint binary; arm G's own `AllocFailCount 0` is the
direct proof, and any static growth eats this 1:1 where only
`gNdsTaskmanArenaChosenSize` can see it). **(b)** taskman general heap —
**19,632 B** over the mandated floor, 26,800 over the GObj latch. **(c)** static
image — 319,808 B proven, **context only**: it is the meter that has now failed
three times on exactly this class.

> **§9's displacement constraint is WITHDRAWN as the binding question.** It
> priced the blob against the *bytes* it evicts; what matters is whether the
> residual cache still serves the residual working set, and over 12 entries it
> does with 26,704 spare. **Dropping ~11 clips to reach 262,144 is not needed and
> was not attempted**, and the `BLOCKED(decision: lossy stream)` fidelity trade
> does not have to be taken to the owner.

**BOUNDARY: flag 0 GREEN · flag 1 at the SHIPPING arena GREEN · arm G RED.**

```text
battle_playable locked-30 pacing failed ... (logicLag=2 drawLead=-1 phaseLag=1
taskmanPresentLead=2)
BPLAY_PACE=0x42505443,0,422,212,211,...   logic 422  presented 212  draws 211
```

**The middle arm exonerates the pack.** Residency, streaming, the carve and the
whole dispatch — the entire architectural change slice 1 exists for — pass
Boundary at `NDS_TASKMAN_ARENA_SIZE 1,376,256`. Only the arm that **moves the
allocator** fails.

**`drawLead = −1` is not a guest-reachable state.**
`gNdsBattlePlayablePacingDrawCalls` is bumped at `taskman_seam.c:4903` and
`…PresentedFrames` at `:4935`, in that order, one straight-line region, no
`return` between, both reset sites zeroing them together; a whole-tree grep finds
one other `DrawCalls++` at `:7963` and it is the fast-logic path, mutually
exclusive with `:4903` (worth running — the one-writer claim was not free). So it
is a **stale GDB read**: `ARMv5::ReadMem` has no DCache lookup, and ARM946E-S
does not write-allocate. **Same defect as R2-04 E2, different counter group.**

**This closes an either/or that has been open since 2026-08-09.**
`docs/KNOWN_ISSUES.md` records `phaseLag=-1` from `NDS_R2_CAMERA_MATRIX_LEAN=3`
— also an allocator move — as "a real off-by-one … or the stop-phase model is
incomplete", and holds that lever off by default for it. **Both tuples are
explained by staleness and only by staleness**: a stale read is always *behind*,
never ahead, and in each row the counter written **first** is the one that reads
low. **Remedy: one `DC_FlushRange` publication seam for the four `BPLAY_PACE`
counters**, precedent `ndsPlatformPublishBattleFpsHudGroup`
(`nds_platform.c:2261`). It changes the shipped binary — its own build, its own
Boundary run, its own gate re-measure — so it is **handed forward, not
half-landed**.

**LANDED 2026-08-15 — ARM G IS GREEN, AND THE FIX IS STRUCTURAL RATHER THAN A
THIRD FLUSH** (`…/2026-08-15_pacing-publication/PACING_PUBLICATION.md`).
`verify-all.ps1 -Profile Boundary` **passed on all four arms** — arm G
(`BATTLEPACK=1` + `KEEP_CACHE=1`, arena 1,548,288), flag 0, flag 1 at the
shipping arena, and `NDS_R2_CAMERA_MATRIX_LEAN=3` — with **0 `Exception:`** in
every log. Each debugger-read counter group is now ONE X-macro list beside its
externs and the publish is **generated** from it (`NDS_PUBLISH_DEBUGGER_GROUP`,
`nds_platform.h`), so a member cannot be added without its flush;
`check-gbi-decode-fixtures.ps1` requires each list and its marker `printf` to be
the same set **in both directions**, and the falsifier was run (dropping one
member turns it red and names the member).

**Publishing `BPLAY_PACE` alone would have been half a fix, and the blast-radius
sweep is what caught it.** The harness also derives `taskmanPresentLead =
$tmPace[1] - (2 * $bp[4])` and requires **0..2**; at the frame-complete marker
that difference **rests at exactly 0**, so pinning one side of the subtraction
coherent and leaving `GCRUNALL_TASKMAN` free to read stale would have converted
`drawLead=-1` into `taskmanPresentLead=-1`. Both groups go through the seam.
The seam sits **before** `ndsBattlePlayableFrameCompleteMarker`, not inside it:
GDB breaks on a function's **entry**.

**THE PROOF IS A COUNTER, NOT THE GREEN — AND THAT IS A RETRACTION.** A no-seam
arm G control was built this cycle (`nm`: the publish symbol absent) and **it
passes Boundary too**, so "the fix made arm G green" is not supportable and the
inherited *"arm G is Boundary-RED"* premise does not reproduce on this tree.
What separates the arms is the stop-time read of
`gNdsBattlePlayablePacingPresentedFrames` in every run's pacing smoke line:
**212 on all four seam arms, 211 on both no-seam arms**, against the previous
cycle's RED which read presented **212** with draws **211** — torn. Unpublished
the group reads *uniformly one behind* (legal, so the harness passes anyway);
published it reads *current*; the allocator move is what turned uniform lag into
tearing. A stale read is always behind, never ahead — measured on the quantity.
**Second, independent signal:** the tick-HUD sampler's ring stops go from
**5 of 16 skewed** (`c168`) to **0 of 16** (`c170`), and c168's
`Tick-HUD samples repeated a presented frame (3 of 1600)` warning disappears.
The instrument was reading its own frame counter one behind at a third of its
stops.

**GATE, re-measured on the binary that would ship** (`build-c170-seam-bp1`,
`BOTH_CPU=1`/`DRAW=1`, DLDI on, 1,600 samples, window 439–2038, rank-80):
**`WORK-H` P50 940,320 · P90 1,091,520 · P95 1,177,920 raw / 1,152,973 net ·
top-1% 1,518,528 · max 5,277,248**, **net gap +32,593** against 1,120,380.
Against the banked no-seam arm (1,170,048 / 1,145,101) that is **+7,872 raw,
below the ≥14,080 cross-build floor — unchanged, not a cost**; the seam's own
price is bounded at **≤450 tk/frame** from the image (82 Thumb instructions,
20 `blx armDCacheFlush`), an order of magnitude under the floor. VBI
**2:1745 3:272 4:13 5+:8 max 19**, slips 0, violations 0. End-of-match pair
**0/76**, identical to `c168` and to the flag-0 control. Stress battery on the
same ROM: 660 s **NO-FREEZE**, 10 entries / 8 matches / 8 restarts /
**2 Sudden Deaths**, `ChosenSize` 1,548,288 · `AllocFail`/`ReserveFail`/
`Rejects`/`SyMallocOverflow`/`LoadFails` all **0**, heap low-water **52,472**.
Two frames (1843, 1937) carry ~+4.1M each in one bucket and are unexplained;
they miss the banked percentile, and isolated multi-megatick frames occur on
`c169` (3) and `c166` (4) too, so they are not this change.
**CADENCE, from the `NDS_TICK_HUD_DRAW=0` arm `plan.md` §1 item 3 actually gates
on** (`build-c170-seam-bp1-draw0`): **VBI 2:1853 3:170 4:7 5+:8 max 19, slips 0
→ two-VBlank 90.9%** against the **≥95%** requirement. **ITEM 3 IS NOT MET and
is not claimed.** Removing the HUD draw lifts the share from 85.6% to 90.9%, so
the instrument owns ~5.3 points and the product still owns **4.1** — the same
gap the +32,593 net excess describes from the tick side.

**Two cross-marker relations remain unprotected and are named rather than
fixed** — `$safety[16] -eq $fdc[8]` (`:3126`) and `$textHud[4..9]` against
`$sourceHud`/`$sourceLower` (`:3384-3389`). Neither has been observed red,
neither is a strictly-ordered pair with zero slack, and fixing what has not been
measured broken is the phantom-defect mode. With the group law in place each is
a three-line change.

**Task B did NOT close the way it was expected to, and the control is why.**
Built deliberately with the seam removed (`nm`: the publish symbol absent from
the ELF), `NDS_R2_CAMERA_MATRIX_LEAN=3` **passes Boundary on this HEAD anyway**.
The 2026-08-09 reproduction is dead, so the seam cannot be credited with fixing
that row; its *mechanism* question is answered by the `drawLead` evidence, which
was live and byte-for-byte deterministic. Level 3 is green with and without the
seam, so the reason it was held has no live symptom — the default flip is
`BLOCKED(decision: NDS_R2_CAMERA_MATRIX_LEAN default)`.

**CORRECTION, and it is load-bearing: Boundary does NOT pin
`gNdsTaskmanArenaChosenSize == 1376256`.** Both runtime sites
(`verify-battle-mariofox-gcrunall-loop-harness.ps1:2006` and `:2573`) are inside
`if ($Task34StageStreamCensus)`, which only
`benchmark-renderer-fast-raw.ps1 -Task34StageStreamCensus` passes, against
`builds/build-task34-stage-stream-census-lab`. `verify-battle-playable-harness.ps1:142`
never passes it. **The pin is a Task 34 census lab gate on a lab build built at
defaults, where 1,376,256 is still exactly right — nothing was retaught, and
loosening it would have deleted a working check.** What Boundary *does* enforce
is `check-gbi-decode-fixtures.ps1:2493`, a source-**text** assert that the
literal still appears in the verifier.

**Also checked and clear.** The Task 36 replay admission guard has a legacy form
of exactly `gNdsTaskmanArenaChosenSize != 0x150000` (`nds_renderer.c:5735`) which
arm G would fail, silently disabling rigid-stage replay and confounding every
tick figure in this campaign. It does not fire: the published (`Makefile:1566`)
and tick-HUD (`:1717`) blocks both force `NDS_TASK53_REPLAY_ARENA_FIX := 1`
(`< 0x130000`), and `gNdsRendererTask36ReplayArenaStaleCount` reads **16,914 on
arm G against 0 on the control** — that counter exists to count exactly the
frames the relaxed guard admits and the legacy one would have blocked.

**Next**: publication seam → Boundary at arm G → gate re-measure on that binary →
only then a default flip, which is the owner's.

## THE PACK PATH'S COST WAS A DEFECT, NOT THE DESIGN — AND IT IS FIXED (2026-08-15)

`artifacts/performance/2026-08-15_battlepack-mechanism/BATTLEPACK_MECHANISM.md` ·
builds `c166-nodispatch-bp1` (falsifier), `c167-profile-bp1` (per-frame regions,
`DRAW=0`), `c168-packfix-bp1` (the fix), `c168-default-check`, `c169-packfix-noarena-bp1`.
**The section below is superseded on its verdict only; every other finding in it
stands and is re-affirmed here.**

**THE SITE.** `ndsRelocResolvePointerFromFileBase`
(`src/port/reloc_backend_assets.c`) asked *"is `ptr` ALREADY an absolute pointer
into a known file?"* **before** interpreting `ptr` as a file-relative offset. For
a clip served from the pack that probe can never succeed — the generator emits
blob-relative offsets, so `ptr` is a small integer — and its **miss** path is
`ndsRelocFindKnownFileContaining` falling through the loaded-file scan into
`ndsRelocFindStatusNodeContaining` over **both** status buffers, where every node
runs `ndsRelocStatusNodeDataSize` → **`ndsRelocAssetIDForToken`**, a ~300-compare
chain whose full miss also walks the 143 + 158 Mario/Fox pointer arrays.
**Two complete status-buffer scans per figatree slot, ~18 slots per action
change, ~136 node visits per slot.**

**THE PRICE, PER PC.** `build-c167-profile-bp1`, 641 presented frames, mask = the
69 regions over 4.0M cycles (the two-VBlank quantum is 2,240,760, so this mask is
not sorting rounding noise):

| symbol | all cycles | masked | masked/all | tk/fr | control tk/fr |
|---|---:|---:|---:|---:|---:|
| `ndsRelocAssetIDForToken` | 207,877,919 | 207,366,743 | **99.8%** | 162,151 | 1,176 (**138x**) |
| `ndsRelocFindStatusNodeContaining` | 113,559,597 | 113,559,597 | **100.0%** | 88,580 | absent |
| `ndsRelocFindLoadedFileContaining` | 4,168,059 | 2,209,792 | 53.0% | 3,251 | 2,002 |
| *every other symbol* | — | — | **≈10.8%** (the base rate) | — | — |

92.4% of the masked work excess. 2,329,254 tk on each acquisition frame against
the gate arm's measured **+2,261,760** at rank-80 — two instruments, two arms
(`DRAW=0` vs `DRAW=1`), two windows, **3.0% apart**. The expensive frames
reproduce on the `DRAW=0` arm at the same game moments (regions 452, 456, 464,
471 … = the gate arm's 453, 457, 465, 472 … offset by the window base), so the
tick-HUD draw instrument is not involved.

**WHY NO BRACKET NAMED IT.** The excess lands in whichever bracket is open —
`SINT` on one frame, `SPHD` on the next, `SCAT`/`SPRM`/`STG`/`MISC` elsewhere —
because the figatree attach is reached from `gcPlayAnimAll`,
`ftParamUpdateAnimKeys`, `ftCommonGuardUpdateJoints`, `mpCollisionPlayYakumonoAnim`
and `gcGetDObjTempAnimTimeMax`. **A bracket ranking would have chased the proc,
not the call.**

**TASK A — the slice-51 falsifier settled residency vs dispatch.**
`build-c166-nodispatch-bp1`: blob streamed, validated, adopted and carved exactly
as arm C (`State` READY, `Bytes` 287,904, `LoadSteps` 18) with
`ndsBattlePackFindFigatree` answering NULL — `Hits` **0** against a control that
reads 197, `ResolveOffsetCount` 0. rank-80 **1,222,464**: **−2,225,408 against
arm C**, **+36,352** over the no-pack control, and that residue sits inside its
own 34 cache rejects (dispatch off makes the 163,840 B cache serve both fighters
again). **Presence is 1.6% of the effect. The dispatch is 98.4%.**

**THE FIX.** Ask the cheap question first: when `file_base` is in the pack,
resolve `ptr` as a blob offset and never run the absolute-pointer probe. It
cannot change a result — the blob extent is 287,904 B and every DS address is
≥ 0x02000000, so a genuinely absolute word fails the bound, falls through, and
takes the original path where `ndsBattlePackContains` answers it O(1) anyway.
`#if NDS_R2_BATTLEPACK`, because there is no LTO here and a cross-TU call is not
free at flag 0.

| arm | P50 | P90 | rank-80 raw / net | max | mean | >gate | >2M |
|---|---:|---:|---:|---:|---:|---:|---:|
| **A** no pack | 940,416 | 1,097,920 | 1,186,112 / 1,161,165 | 2,300,928 | 960,540 | 135 | 2 |
| **C** pack, defect | 940,128 | 1,216,832 | 3,447,872 / 3,422,925 | 7,245,056 | 1,196,937 | 218 | 128 |
| **G** pack, fixed | 938,848 | 1,086,336 | **1,170,048 / 1,145,101** | 2,182,016 | 955,581 | 123 | **1** |

`G − C` rank-80 **−2,277,824**, mean −241,356, over-gate −95, >2M 128 → 1,
`SINT` max 6,454,592 → 810,176. Engagement identical to arm C — `Hits` 197,
355 acquisitions, damage **0 / 76**, `Rejects` 0, arena 1,548,288 / `AllocFail` 0
/ `ReserveFail` 0, heap free-min 52,864 — and **`gNdsRelocResolveOffsetCount`
3,629 = 3,629**: the same slots, resolved by a different path.

> **WHAT IS AND IS NOT CLAIMED.** `G − A` is −16,064 at rank-80, close to the
> ≥14,080 cross-build floor, and is **not** banked as a P95 win. The supported
> claim is that the fixed resident pack is **no worse than the no-pack control
> and leans slightly its way** — P50 −1,568, mean −4,959, over-gate 135 → 123,
> >2M 2 → 1, max −118,912, one sign throughout, which is exactly the
> P50/mean/over-gate ranking `VERIFYING.md` requires.
> **`−73,659` STAYS RETRACTED** — it was a projection and the measurement is a
> wash, not a win. **What is WITHDRAWN is "slice 1 is refuted as a P95 lever":**
> that verdict was measured on a binary carrying this defect.

**THE RAM QUESTION COMES BACK WITH THE WIN — AND IT IS NOW PRICED.**
`build-c169-packfix-noarena-bp1` is the fix at the **shipping** arena
(`NDS_R2_BATTLEPACK=1` alone: arena **1,376,256**, `AllocFail` 0, heap free-min
40,576 vs the 32,768 floor, cache carved to 4,096 → `Rejects` 126, `Fills` 2,
`AnimCacheHits` 30 — arm B's cache state, arm G's code):

| arm | cache | arena | P50 | rank-80 raw / net | max | mean | >gate | >2M |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| **A** no pack | 262,144 | 1,376,256 | 940,416 | 1,186,112 / 1,161,165 | 2,300,928 | 960,540 | 135 | 2 |
| **B** pack, defect | 4,096 | 1,376,256 | 939,712 | 3,447,488 / 3,422,541 | 7,252,800 | 1,219,250 | 271 | 130 |
| **G** pack, fixed | 163,840 | 1,548,288 | 938,848 | 1,170,048 / 1,145,101 | 2,182,016 | 955,581 | 123 | 1 |
| **H** pack, fixed | 4,096 | **1,376,256** | 938,784 | **1,435,904 / 1,410,957** | 6,401,536 | 988,452 | 190 | 8 |

```text
H - B   rank-80 -2,011,584   mean -230,798   >2M 130 -> 8     the fix pays here too
H - A   rank-80   +249,792   mean  +27,912   >2M   2 -> 8
H - G   rank-80   +265,856   mean  +32,871   >2M   1 -> 8     <- the CARVE, priced
```

**`H − G` is the price of the carve.** The arms differ only in arena and cache
size; the defect is fixed in both. **Phase 8 blamed the cache deletion and the
isolation arm refuted it — correctly, on its evidence: B and C differ 40x in
cache and landed 384 apart, so the cache was a passenger while the defect
dominated. With the defect gone the passenger is the whole remaining fare.**
§9's design constraint is therefore **reinstated and binding**: *a resident pack
must be smaller than the storage it displaces.* The Fox blob is 287,904 B against
262,144 B — **1.098x its own displacement**. Close that and the shipping arm
becomes arm G. `build-c168-packfix-bp1` remains a lab arm (arena 0x17A000;
Boundary pins `ChosenSize == 1376256`).

**SHIPPING DEFAULT PROVEN INERT, BYTE-FOR-BYTE.** `build-c168-default-check`
against `build-c165-default-check`: `.text.hot` / `.text.hot.draw` / `.itcm` /
`.dtcm` section hashes identical; `.main` differs in **7 bytes at offset
0x0c8fc0**, which is the embedded git short hash. Boundary GREEN at flag 0 and
flag 1 on `79a9447fd6d` stands and was not re-run. **A section-SIZE compare is
not a proof of inertness** — hash the content and read the differing run.

**TOOLING TRAP, NOW STRUCTURAL.** `-RunnerSlot` silently overrides `-MelonDS`
(`scripts/lib/melonds.ps1:542` ignores the parameter for slot ≥ 0), so a census
asked for with `emulators\melonds-attributor` **and** a slot returns the v2 build
with **no stall columns at all** while every banner still says "census". It cost
a 25-minute run and surfaced downstream as `KeyError: 'halt_wait'`.
`run-task37-profile-census.ps1` now throws on that invocation and
`census-marginal-frame-owners.py` names the real fault. **The v3 stall-class split
of these cycles was therefore not taken and is not claimed.**

## THE ISOLATION ARM WAS BUILT AND SLICE 1 IS REFUTED AS A P95 LEVER (2026-08-15) — VERDICT SUPERSEDED ABOVE

`artifacts/performance/2026-08-15_battlepack-isolation/BATTLEPACK_ISOLATION.md` ·
`build-c165-keepcache-bp1`, base HEAD `30b38f3e9d3`, 2 builds. **This section
supersedes the root cause in the phase-8 section below; that section's gate
numbers stand, its attribution does not.**

Arm C = pack resident (197 hits) **and** raw cache healthy: `Rejects` **0**, **9**
full ROM loads all match against the control's 17. Same window (439–2038), same
basis, same fight (damage 0/76, 355 acquisitions, all three arms).

```text
              P50        P90        P95 rank-80 raw / net      top-1%    >2M frames
A control   940,416  1,097,920   1,186,112 / 1,161,165      1,570,944        2
B cache-gone  939,648  1,540,032   3,447,488 / 3,422,541    6,118,208      130
C ISOLATION   940,128  1,216,832   3,447,872 / 3,422,925    6,175,104      128

C - A   P50 -288 (flat)   rank-80 +2,261,760   mean +236,397   over-gate +83
C - B   P50 +480          rank-80    +384      mean  -22,313   over-gate -53
```

**`−73,659` IS RETRACTED — measured +2,261,760, opposite sign, ~30× the
magnitude.** Ranked on P50, mean and over-gate as `VERIFYING.md` requires, all
three agree.

**AND THE PHASE-8 ROOT CAUSE IS REFUTED.** It charged +2,261,376 to 111 net-new
uncached acquisitions at 3,873,969 tk each. Arm C removes *more* than those 111
(ROM loads 128 → 9) and the residual moves **+384 (0.011%)** against a ≥14,080
floor. Two arms differing in arena size, in cache size by **40×** and in ROM loads
by **14×**, landing 384 apart, share one cause — **the pack path**, the only thing
they have in common. `SITR` 41.6%/2.84× → **85.7%/34.48×**, on **128 frames** of
mean `WORK-H` 4,118,565; draw side flat.

> **RETRACT BOTH PER-ACQUISITION PRICES AND DO NOT PRODUCE A THIRD.**
> `+645,225 a miss` (warm-cache coefficient) and `3,873,969 per uncached
> acquisition` (priced a mechanism owning ~0 of the residual) both came from
> dividing a residual by whichever count was to hand.

**Excluded by counter, two soaks, no build:** normalizer identical
(`sNdsAObjEvent32NormalizedCount` 245=245, `…ScriptCount` 225=225,
`…ReuseCount` 1,609=1,609) and `gNdsTaskmanMallocCount` 1,069=1,069. **Sole
differing counter: `gNdsRelocResolveOffsetCount` 0 → 3,629** — the
blob-relative-offset branch in `ndsRelocFindKnownFileContaining`, banked as
engagement proof and never priced. **A lead. Price it per-PC, never by division.**

**THE RAM LANE IS MOOT FOR THIS DECISION.** Arm C displaces nothing — it *adds*
163,840 B of cache beside the blob — and still costs 2.9×. Buying the arena does
not buy the win. **Task B (a pack that fits its displacement) is parked**: the
shipping blob is *already* `--items-off` (287,904 vs 262,144, 25,760 over) and its
**stream alone is 272,292 B — 10,148 over before any metadata**, so metadata
compaction is closed by arithmetic and a lossless stream was already refuted.
`BLOCKED(decision: drop ~11 more clips with a correctness proof, or accept a lossy
stream — a fidelity trade)`, **and not worth taking until the pack path is shown
cheap at all.**

**Two allocator lessons, both earned the expensive way.** (1) The first sizing
asked for +258,048 B of arena on 319,840 B of "proven headroom" and the heap
granted only **188,416**; the 550,080 reservation *inside* the short arena still
succeeded, so every guard passed and the battle **never started** (soak
`NEVER-STARTED`, general heap free **6,076** vs the 32,768 floor, and a 2,400 s
gate run wasted reaching no ring stop). **`check-boot-headroom.ps1` meters static
image, not grantable heap — third recurrence, first for arena growth.** (2)
`NDS_R2_ANIM_CACHE_ARENA_KEEP_FREE` cannot protect against it: it is a
point-in-time check that correctly saw 582,848 free at reservation and cannot see
the scene's later demand. **Gate any allocator arm on a 5-minute soak first**
(now in `docs/VERIFYING.md`).

**NEXT BUILD, and it separates the last two candidates** (the pack's *dispatch*
vs its mere *presence*): the slice-51 falsifier — pack resident, streamed and
carved, `ndsBattlePackFindFigatree` returning NULL. `NDS_R2_BATTLEPACK` and the
new lab flag `NDS_R2_BATTLEPACK_KEEP_CACHE` both stay default 0; Boundary's
verifier pins `gNdsTaskmanArenaChosenSize == 1376256`, so the grown-arena arm
cannot drift into a bank.

## PHASE 8 IS DONE AND SLICE 1 AS BUILT IS REFUTED AT THIS POOL SIZE (2026-08-15)

`artifacts/performance/2026-08-15_battlepack-gate/BATTLEPACK_GATE.md`. Both arms
`smash64ds-battle-playable-tickhud-hwtri`, `BOTH_CPU=1`/`DRAW=1`, DLDI on, mode
163 one-minute, `-Samples 1600 -RingDump`, window 439–2038, HEAD `79a9447fd6d`.

```text
              P50        P90        P95 (rank-80) raw / net      top-1%      max
flag 0     940,416  1,097,920   1,186,112 / 1,161,165        1,570,944  2,300,928
flag 1     939,648  1,540,032   3,447,488 / 3,422,541        6,118,208  7,252,800
delta         -768   +442,112      +2,261,376                +4,547,264
```

**The flag-0 arm is a control that reproduces the bank**: +1,280 / +1,472 /
**+2,048** against `build-c158-gate` at `a159069af0d`, on a tree that has since
taken Phase 2, the `ftmain` patch and the whole pack. **The requirement on this
HEAD is 65,732 at the 80th-largest frame, not 64,452.** Same fight both arms —
`gNdsBattleTextHudP0Damage/P1Damage` **0/76** on both, 355 acquisitions on both —
so this is a cost delta, not `route-ab-cannot-price-gameplay-change`.

**THE ATTRIBUTION, AND IT IS NOT CLOSE.**

- **18 streamed chunks: ≤0.3%.** They run once per *source update* at the first 18
  `scVSBattleFuncUpdate` calls (presented frames ~1–9); the window opens at 439.
  Bounded independently of that: whole-match pacing VBlanks **4,501 → 5,271
  (+770, +17.1%)** while the in-window `ALL` delta is **430,010,624 tk = 767.6
  VBlanks**, so everything outside the window is **≤2.4 VBlanks**.
- **Cross-build placement: ~0.** P50 moved **−768** against its own ~5,700 floor;
  a +2,261,376 rank-80 move is **160× the ≥14,080 P95 floor**. No falsifier arm
  was built — at 160× the floor it is no longer the discriminator.
- **111 extra uncached acquisitions: the rest.** `430,010,624 / 111 =
  3,873,969 tk each (~6.9 VBlanks)`, and the P95-set bracket agrees from the other
  side: **`SITR` +192,781 (41.6%) → +3,348,118 (86.3%) at 36.19×**, `SPRM` 134.61×,
  while `FTR`/`STG` do not move (1.01×/1.00×). The draw side is untouched; this is
  the acquisition path on the frames that set P95.

**ROOT CAUSE: the carve did not shrink the raw file cache, it DELETED it.**
`NDS_BATTLEPACK_RESERVE_BYTES` 287,936 of a 292,032 B reservation leaves **4,096
B**; `Fills` 17 → **2**, `Rejects` 0 → **126**, `AnimCacheHits` 338 → **30**.

> **CORRECTION — do not reuse the old miss price.** The banked dose-response
> modelled a cache miss at **+645,225**; measured here it is **3,873,969**, **6.0×**.
> That figure is a whole-frame regression coefficient taken with a *warm* cache and
> does not price a load taken with the cache deleted.

**THE RAM DOES NOT CLOSE.** Restoring the un-packed fighter's cache needs
`287,936 + 262,144 = 550,080` reserve against today's 292,032 = **+258,048 B of
arena**; both fighters resident needs ~559,632 vs ~301,564 = **~258,068**. Same
number, same reason. Phase 2's **146,560 is SPENT**; Phase 1 is 21,600 unspent;
Phase 4 is **≤80,096 of unsized candidates**; Phase 3's ~150 KB premise is
**REFUTED**; Phase 5 is unstarted. **Optimistic ceiling 248,256 — short of the
ONE-fighter case and 26% of the two-fighter one. No named phase or combination
covers it.** The only `.bss` objects big enough are `sNdsAudioFgmCache` (already
proven *under*-sized) and `sNdsRelocSceneFileBuffer` ("already optimized once").

**The cheapest unpriced lever is not the RAM plan.** The Fox blob is 287,904 B
against the 262,144 B cache it evicts — **1.098× the thing it displaces**. A pack
smaller than its own displacement closes this without buying a byte, and the
items-off re-pack (553,696 B both fighters, mismatch 0) is already proven.

**`NDS_R2_BATTLEPACK` stays default 0.** No flip proposed.

**Not done:** the arm that would isolate the deletion's *benefit* (pack resident
**and** cache intact) — so **the −73,659 at rank-80 is still a projection and is
now less supported, not more**: P50 is flat because the median frame has no
acquisition, and P95 is swamped. Also not done: a `DRAW=0` cadence arm, phase 6's
oracle, and the measured after-GO per-fighter zero-I/O assertion (the seven K0
counters are zero *by construction* on the pack-hit path — the early return
precedes all of them in the same function — but no GO-gated per-fighter counter
exists).

## NATIVE BATTLE KERNEL SLICE 1 (2026-08-14) — phases 1–4 DONE, RAM re-measured, zero builds

`docs/architecture/RUNTIME2_NATIVE_BATTLE_KERNEL.md` (design) ·
`artifacts/performance/2026-08-14_native-battle-kernel/BATTLEPACK_ANIMATION.md`
(measurement, sizing, equivalence). `plan.md` §K1 phases 5–9 remain.

**The named mechanism was measured and it is NOT the FAT read.** On the gate arm
(`build-c159-profile-bothcpu`, 1,600 regions) the animation path does **15** file
loads all match, on 13 frames; **deleting those 13 frames entirely moves the
80th-largest frame 9,874** against a 64,452 requirement. The lever is the
**acquisition path**: 299 acquisitions, **95.0% cache HITS**, on **62 of the 80
P95 frames** against 174 of 1,520 body frames (**6.8x presence**). A hit still
copies the payload, re-registers, re-finalizes, re-normalizes AObj16, strips
alias nodes and writes three status entries. Dose-response: **+148,969** ticks
for the first acquisition, **+77,440** each after, **+645,225** for a miss;
modelled full deletion moves rank-80 **−73,659** (projection, upper-bound model,
profile arm — **not banked**).

**Correction to `GATE_ARM_OWNERS.md` §5.2:** `get_fat`/`f_lseek` in that
mechanism are majority **BGM** — ≤38.8% of the P95-set `get_fat` sits on an
animation-load frame. Sizing slice 1 off the full +15,058 over-predicts ~2.6x.

**Reachable set = all 301** (`ftdata.c` references 143/143 Mario + 158/158 Fox).
The 87-entry `sNdsR204AnimWarmList` is 28.9% of it and is observational.

**Pack: 651,928 B (floor 645,450) against ~511,904 B proven RAM — it does NOT
fit**, and no lossless compaction closes it (dead tails 1.0%, dedup 4.6%,
substring 0.004%). Falling back to gameplay-time FAT loading stays forbidden.

### RAM RE-MEASURED (2026-08-14, cycle 2, zero builds) — it STILL does not fit, and the closure is Phase 2 of the RAM plan

`BATTLEPACK_ANIMATION.md` §11–§12 · architecture doc §6 rewritten.

```text
                                   pack        pool        short
full pack (297 clips)             651,928     511,904     140,024
items off (259, PROVEN)           553,696     511,904      41,792
  + route 1 (2..4 figatree heaps) 553,696     524,352..537,120   29,344..16,576
  + matchup lead (245, UNPROVEN)  528,624     524,352..537,120    4,272..-8,496
```

- **Items-off is PROVEN from the LINKED ELF**, which is stronger than the status
  graph the brief asked for: every function that could set an item status is a
  two-byte `bx lr` stub (`ftCommonItem{Throw,Swing,Shoot,ShootAir}SetStatus`,
  `ftCommonLightThrowDecideSetStatus`, `ftCommonHammerFallSetStatus`, and the
  `W` cluster at `0x208fc34..0x208fc74`), and the ELF holds **no item spawner**.
  Priced by **re-packing** (dedup makes a subset non-linear), −98,232, and the
  259-clip pack re-verifies at **mismatch = 0**, corpus `c034b342…`.
  `scripts/generate_battlepack_anim.py --exclude-ids` is the new instrument.
- **Route 1 is 12,608–25,216 B, not 140,024.** `gFTManagerFigatreeHeapSize` is
  the largest single animation FILE over the loaded kinds (max payload 6,224),
  two heaps a match plus two on Sudden Death entry. **The 262,144 arena was
  already counted** — HANDOFF's 192,240 is its bump high-water *inside* that
  reservation, not a second pool. No gdb read was needed; the quantity is a
  compile-time max over the bank.
- **THE THREE POOL TERMS ARE NOT ADDITIVE.** The arena is one `calloc` of
  `NDS_TASKMAN_ARENA_SIZE 0x150000`, so a `.rodata` pack draws on **211,936**
  alone and an arena-resident pack on **299,968** alone. Reaching 511,904 needs
  the constant cut, the `0x130000` search floor lowered *and* the Task 36
  replay-admission guard retaught (`nds_renderer.c:5734-5739`).
- **A more compact representation is REFUTED as a lossless lever**: the AObj16
  stream is already u16 command headers and **s16** target words — no f32 to
  narrow, and no dictionary can beat a 16-bit alphabet. A pre-GO arena creates
  no RAM at all.
- **PHASE 2 IS SPENT — +146,560 B, MEASURED, 2026-08-15** (`8cfbc2eaa2b`;
  `…/2026-08-15_framebuffer-collapse/PHASE2_FRAMEBUFFER.md`).
  `gSYFramebufferSets` `[2][230][320]` → `[1][231][320]`, 294,400 → 147,840 B.
  Same build directory both arms: **bss 1,453,544 → 1,306,984 (−146,560
  exact)**, text and data unchanged, `fake_heap_start` `0x02269ee4` →
  `0x02246264`, proven headroom **174,368 → 320,928**. The reader set is from
  the **linked ELF** — one reader (`ndsBaseLBTransitionSetupTransition`, the
  wipe), one writer (`ndsBaseSCManagerRunLoop`'s clear, self-bounded by
  `sizeof`), plus address-only users — and the span was re-derived from the
  wipe's compiled literals (`+0x23f14`, `−640`/row, 220 rows, 600 B/row →
  `base+7,060 .. base+147,819`). `mntitle.c`'s `[1]`/`[2]` gap is closed at the
  DS import boundary: `battleship_mntitle.c` aliases all three setup pointers to
  framebuffer `[0]` before `syVideoInit`, while the decomp stays pristine.
  Boundary green, 0 `Exception:`. **VS Results at source tic
  160 is a byte-identical capture across the arms; the wipe's animated frames
  were NOT captured and still need the owner's eye — not marked FIXED.**
- **CORRECTION: "the full pack then fits by 6,536" is about the COMBINED pool,
  and the non-additivity above is untouched.** Measured static headroom on the
  **published** arm 213,216 → **359,776**; a `.rodata` full pack is still short
  292,152, items-off short 193,920; an arena pack still draws on 299,968 alone.
  Only the combined row fits (full by 7,816, items-off by 106,048) and it still
  needs the constant cut, the floor lowered and the Task 36 guard retaught. The
  older **211,936** static figure was the `build-battle-playable-proof-hwtri-harness`
  arm, which is neither the shipped nor the measuring ROM (it reads 208,672
  today). **State the arm with the headroom.**
- **PHASE 5's OTHER REASON TO COPY IS REMOVED WITHOUT EDITING `decomp/`.**
  The old `6e93def43cd` decomp patch is retired. The DS force loader records the
  authoritative file for the fighter's heap; the port-owned figatree attachment
  seam substitutes that file when pristine BattleShip passes `figatree_heap`,
  and `battleship_ftmain.c` mirrors the authoritative pointer into `fp->figatree`
  after the source call. The bridge exists only when `NDS_R2_BATTLEPACK=1`.
  Boundary is green on the pack+keep-cache arm with upstream `ftmain.c`
  byte-for-byte restored.

**Phase 4 host equivalence: mismatch = 0** over 297 clips / 5,629 scripts /
77,959 commands / 71,500 states / 5,629 callbacks. Two falsifiers prove the test
can fail. `scripts/generate_battlepack_anim.py --verify`.

**New reusable instrument:** `scripts/census-profile-pc-per-region.py` gives
per-presented-frame call counts for any symbol out of a v3 capture, so a
"does it concentrate on the P95 frames?" question no longer costs a build.

## THE GATE LANE — in order, one row live at a time

### G1 — MEASURED (cycle 79). Mechanism proven, gate unmoved. Not shipped.

`sNdsRendererStageTextureSites` (`nds_renderer.c:11086`), enabled in
`ndsRendererProfileSetOwner` (`nds_renderer.c:28819` — the old `29241-29247`
reference was stale by ~450 lines and is corrected here). Mode 9 now joins the
4/7/8 list behind `gNdsG1SiteCacheRoute`; **route 0 is the default and
reproduces shipped behaviour exactly**, so nothing about the shipped ROM
changed.

**The memo works, and the ~175-key working-set fear was wrong.** Whole match,
both-CPU, one binary, both routes (`builds/build-c79-g1-bothcpu`):

| | route 1 (on) | route 0 (off) |
|---|---:|---:|
| `Tex` per list | **7,203** | 20,780 |
| hit rate | **78.06%** (2,305/2,953) | — |
| overwrites / occupancy | **0** / 26 of 128 | — |
| WORK-H P50 | 1,089,024 | 1,095,552 |
| WORK-H P95 | 1,615,872 | 1,612,032 |
| over gate | 682/1600 | 710/1600 |
| `MISC` P95 | 396,096 | 473,536 |

`Tex`/list falls **65.3%** and `Exec` total falls **9,591,232 ticks/match**
(~5,994/frame mean). `MISC` P95 falls 77,440 — 9.1x the ≥8,544 bucket floor.

**But WORK-H P95 moved +3,840, INSIDE the ±5,376 floor: the gate did not
move**, and this closes none of the 485,060 gap. Buckets locate; WORK-H
decides. `ALL` P95 (+128) and the VBI histogram are unchanged, so pacing is
unchanged.

Two findings the next cycle should not re-derive:

- **The both-CPU gate arm exercises this path ~3.5x LESS than Boundary** —
  2,953 consults over 563 lists, against Boundary's 10,336 over 1,360. The
  banked 21.41% / 25,289-per-list `Tex` figure is a Boundary-config number.
  The gate arm is the *worst* case for any effect-texture lever, which is
  worth knowing before G3 is sized against it.
- **The refuted `(dl-pointer, bind-ordinal)` memo's failure does not
  transfer.** It took 471 hits on 10,336 with 7,517 evictions of 7,525 fills;
  this key takes 78% with **zero** evictions and 26 live slots. The site
  address points into static source display-list data and is stable across
  frames; the dl pointer was not.

**Open before this can ship:** the owner's visual gate on shield / revival
platform / impact wave / reflector (not run — see Inherited), and the byte
cost. The measured **+1,924 bytes (text +1,796, bss +128)** is the
*instrumented* build and sits INSIDE the +1,408/+2,208 cliff band; it is
dominated by six census counters inlined at several sites, not by the flip
(one condition). **The shipping cost is unmeasured** — G2 must measure the
flip alone, because G3's packet builder spends from the same budget.

### G2 — footprint map DONE (cycle 79). Failing allocation NOT yet named; 32 KB NOT yet demonstrated.

**EXIT MET, cycle 84 — ≥32 KB demonstrated at 4.2×.** `gSYZBuffer` gave back
**134,400 bytes**; proven static headroom went **96 → 134,496 bytes**
(`build-c84-zbuffer` links at `0x02273aa4` against `0x02294804` highest-booting).
Boot probe PASS (frames 60–67, 8 samples, slips 0), Boundary green, root ROMs
unchanged, `decomp/` verified untouched.

**Report the two shortages separately — they are coupled through the heap.**
Freeing bss lowers `fake_heap_start`, which enlarges the heap the arena callocs
from, so the arena ate **131,072** of the 134,400 and its deficit is now closed:
`gNdsTaskmanArenaChosenSize` 1,245,184 → **1,376,256** (its full `0x150000`
request, first time ever) and `gNdsTaskmanArenaAllocFailCount` **32 → 0**. That
is the predicted engagement to the digit. **Consequence for the next spender:**
static headroom and arena health are the same 134,400 bytes seen twice — an
instrument that adds N bytes re-starves the arena by ~N (quantised to `0x1000`)
long before it reaches the boot cliff. The SRC ring buckets cost 1,040, so they
now fit with ~129× margin *and* leave the arena at full request.

Authoritative, from the **shipped** ROM's matching ELF pair
(`smash64ds-battle-playable-hwtri.elf`, Aug 4 20:33, pairs with the published
`.nds`). Sections: **text 891,836 / data 147,712 / bss 1,709,640**.

Top `.bss`, which is where the budget actually is (symbol total 1,709,401 of
1,709,640 — so the ranking is essentially complete, not a sample):

| symbol | bytes | share of bss |
|---|---:|---:|
| `gSYFramebufferSets` | 441,600 | 25.8% |
| `sNdsAudioFgmCache` | 204,800 | 12.0% |
| `sNdsRelocSceneFileBuffer` | 185,696 | 10.9% |
| **`sOriginalSpritePreview`** | **153,600** | **9.0%** |
| **`sOriginalSpriteDisplayPreview`** | **153,600** | **9.0%** |
| `gSYZBuffer` | 140,800 | 8.2% |
| `sNdsRendererHardwareTextureScratch` | 32,768 | 1.9% |
| `sNdsRendererTask36ReplayOwner` | 30,880 | 1.8% |
| `sNdsRelocLoadedFiles` | 29,184 | 1.7% |
| `sNdsFighterDLAllDrawStates` | 27,136 | 1.6% |

The top six are **74.9% of all bss**. Top `.text`:
`ndsResetStartupDiagnostics` 33,260, `__dldi_start` 16,384, `categories`
14,328, `ndsRendererHardwareResolveOrBindTexture` 10,944,
`ndsRendererPrepareNativeStageOwner` 10,888, `ndsOpeningRoomRenderDLPreview`
8,756. Top `.data` is `gNdsParticleScriptBank` 10,912 — note `nm` reports
`__sp_usr` at 184,600,960, which is an absolute stack address and **not** a
size; exclude it from any ranking.

**Leading candidate, NOT yet verified removable.** The two sprite preview
buffers total **307,200 bytes (300 KB, 18% of bss)** — nearly 10x the 32 KB
exit on their own, and `sOriginalDLPreview` (13,824) plus
`sOriginalDLDisplayPreview` (7,776) add 21,600 more. All four are declared
**unguarded** in `src/nds/nds_platform.c:114/196/202/205`, so they are
allocated in every configuration including the shipped battle ROM, even if
the battle scene never populates them. `src/port/port_probe.c:53` says the
"original asset previews now own the top-screen visual signal", which is a
**dev preview** role.

**MEASURED AND REFUTED (same cycle, zero build).** Read deep in battle
(frame 1200) on the existing tick-HUD ROM:

```
gNdsOriginalSpritePreviewReady = 1      gNdsOriginalDLPreviewReady = 0
```

**`sOriginalSpritePreview` is populated and in use during battle**, so the
153,600-byte buffer — and by association its 153,600-byte display twin — is
**not** a free deletion. The 300 KB headline is dead. This is exactly why the
flag was read instead of trusting the symbol name and the "dev preview"
comment: deleting on the name would have removed something battle actively
uses, which is the name-driven-logic failure mode in its purest form.

**What survives as a candidate: 21,600 bytes.** `gNdsOriginalDLPreviewReady`
is **0** in battle, so `sOriginalDLPreview` (13,824) and
`sOriginalDLDisplayPreview` (7,776) are not populated there. That is 21,600
bytes — **below the 32 KB exit on its own**, so G2 needs at least one more
source. Note a ready flag is a *state*, not proof of never-use: it shows the
buffer is unpopulated at that moment, not that no configuration ever fills
it. Confirm with a config-level trace before removing, and remember `.bss` is
static — the lever is deleting or `#if`-guarding the buffer out of the battle
configuration, never freeing it at runtime.

**ALL FOUR LARGE TARGETS NOW TRACED (cycle 83). Two refuted, two live, nothing
freed yet.** Trace = who writes it, who reads it, in which configuration.

- **`gSYFramebufferSets` 441,600 — LIVE, do NOT delete. SUPERSEDED 2026-08-15:
  it is now `[1][231][320]` = 147,840 B, and the two collapses freed 147,200 +
  146,560 = 293,760 B in total** (`8cfbc2eaa2b`;
  `…/2026-08-15_framebuffer-collapse/PHASE2_FRAMEBUFFER.md`). The "open question"
  this row ends on was answered by sizing, not by deletion: the extent is
  arithmetic on the wipe's own compiled read span, so the wipe reads exactly
  what it read before whether or not it is sampling the clear. **The fidelity
  question the row raises — whether the DS photo wipe samples cleared black —
  is still open and is still the owner's**, and it is now independent of RAM.
  The trace below stands as written for the era it describes. Three N64 software
  framebuffers (`[3][230][320]` u16, `include/sys/video.h:55`). It **is
  dereferenced on DS**: `decomp/…/src/lb/lbtransition.c:228` reads through
  `gSYSchedulerCurrentFramebuffer`, and `src/import/battleship_lbtransition.c:47`
  deliberately points that at `&gSYFramebufferSets[0]` when NULL — the photo wipe
  into **VS Results, which is in P1 scope**. Also fully cleared by the patched
  `scmanager.c:855-865` loop (NDS arm bounded by `sizeof`, N64 used
  `end = 0x80400000`). **Open question, and it is a FIDELITY question, not a free
  deletion:** the DS renders through GX into VRAM, so the readback may be
  sampling the cleared black — exactly the failure libultraship documented in
  `decomp/…/port/bridge/framebuffer_capture.h:19`. Answer that before sizing it.
- **`gSYZBuffer` — FREED, 134,400 bytes, cycle 84.** An N64 *software* Z-buffer
  on hardware whose depth buffer is in VRAM. Reduced 140,800 → 6,400 bytes
  (`320*10`, the border its own `SYVIDEO_ZBUFFER_START` arithmetic names) in
  `src/import/battleship_sys_zbuffer.c` + the matching extern
  `include/sys/video.h:54`. **No decomp patch was needed** — the import file was
  a one-line `#include` of decomp's text, so the port already owned the TU;
  `fetch-battleship-reference.ps1 -VerifyOnly` confirms `decomp/` untouched.

  **The verdict rests on measurement, not on "no dereference found".** Gate-arm
  control, frames 600–607, DLDI on
  (`artifacts/performance/2026-08-05_c84-zbuffer-liveness.json`): `gSYZBuffer`
  sampled at **9 points across all 70,400 halfwords — every one 0**, untouched
  `.bss` deep in gameplay. **The `−6,400` offset is explained, and it was the
  real hazard:** `gSYFramebufferSets` ends at `0x02211cd0`, *exactly*
  `gSYZBuffer`'s address, so `SYVIDEO_ZBUFFER_START` = `gSYZBuffer − 6400` points
  into the tail of `gSYFramebufferSets[2]` — a **live** buffer feeding the VS
  Results photo wipe. Those 3 border samples still read **1**, so nothing writes
  through the start pointer either. **Control, same run, same instant:**
  `gSYFramebufferSets[0][0][0]`, `[1][115][160]`, `[2][0][0]` all read **1** —
  the clear loop's `GPACK_RGBA5551(0,0,0,1)`. The probe demonstrably sees writes
  (0→1) in the neighbouring bytes and sees none here.

  Consumers, all enumerated and classified: `SYVIDEO_ZBUFFER_START` computes;
  `video_bootstrap.c:19` and the imported scene setups **store**; `syVideoInit`
  **stores** into `gSYVideoZBuffer`; `video_bootstrap.c:34` **compares**.
  **Zero dereferences**, and nothing takes `sizeof(gSYZBuffer)` — which is what
  makes a size reduction safe. Engagement proof that the surviving consumer
  still works: `gNdsVideoBootstrapResult` reads `0x56494430`
  (`NDS_VIDEO_BOOTSTRAP_PASS`) after the change, and that check can fail
  (`0xBAD00001`).
- **`sNdsRelocSceneFileBuffer` 185,696 — REFUTED as free.** Already
  harness-guarded (`reloc_backend_assets.c:352-358`): in harness builds the union
  is sized exactly `CASTLE_STATIC + BANK_113_STATIC` and serves as the **battle**
  static-asset staging store. This is the 2026-07-16 PORTING optimization,
  already banked; there is no second helping.
- **`sNdsAudioFgmCache` 204,800 — REFUTED without an owner decision.** A live
  8-slot FGM sample cache; capacities `{53,248, 3×28,672, 4×16,384}`
  (`nds_audio_fgm.c:440-443`) sum to exactly 204,800. Shrinking it trades audio
  fidelity — charter §7, the owner's call, not this row's.

### The failing boot-time allocation IS NAMED (cycle 83), and it gives G2 a real ceiling

`sNdsTaskmanArenaBytes = calloc(1, arena_size + 0x10)` at
`src/port/diagnostics.c:7646`. It requests `NDS_TASKMAN_ARENA_SIZE` = `0x150000`
(1,376,256) and walks **down** in `0x1000` steps to a floor of `0x130000`. Read
off the booting gate-arm control (`build-c80-gate-bothcpu`, 8 samples, frames
60–67, artifact `artifacts/performance/2026-08-05_c83-arena-ceiling.json`):

```
gNdsTaskmanArenaChosenSize     = 1,245,184   (0x130000 -- the FLOOR)
gNdsTaskmanArenaAllocFailCount = 32
```

1,376,256 − 1,245,184 = **131,072 = exactly 32 steps of 0x1000**, matching the
fail count to the digit. **The arena is pinned at the bottom rung of its own
ladder, 128 KB short of what it asks for, on a ROM that boots.** It degrades
instead of freezing only because the ladder exists; below it sit
`0x100000/0xc0000/0x80000/0x40000` and then the §3.11 `syMallocSet` spin.

**This is a better G2 instrument than the boot probe, because it is continuous
rather than binary.** Free N bytes of static footprint and
`gNdsTaskmanArenaChosenSize` should rise by ~N (quantised to 0x1000) while
`gNdsTaskmanArenaAllocFailCount` falls by ~N/4,096 — a zero-ambiguity engagement
proof for any candidate, readable with `-ExtraGlobals` in ~50 s and **no build**.
Pair it with `check-boot-headroom.ps1`; the arena number says how much the free
was *worth*, the headroom check says whether the build still boots. Note the
first 131,072 bytes freed are absorbed by this deficit before the request is even
met, so **G2's ≥32 KB exit buys arena, not slack** — the boot cliff and the arena
starvation are two separate shortages and both are real.

**Not done this cycle:** the +2,208 failing allocation is **not named** — that
needs a `fake_heap_start` build plus a gdb probe for the `syMallocSet` spin,
and naming it by inference was explicitly out of scope. No headroom freed, no
32 KB demonstrated, no build made for this row.

### G2 (original row) — RAM headroom before any new code lands

The boot cliff blocks every candidate that adds text or data (it is what
actually killed the Tex memo arm). Produce the authoritative footprint map:
rank `.text`/`.data`/`.bss` and fixed pools from the map file; identify which
boot-time allocation fails at +2,208 (the failing arm dies before frame 9, so
it is a boot/scene-entry peak, not steady state); free or defer the cheapest
candidates. **Exit: ≥32 KB static headroom demonstrated by the same
`fake_heap_start` probe**, so G3's builder text plus arena bookkeeping fit with
margin. No performance claim — this row is measured in bytes, not ticks.

### G3 census — TAKEN ON THE CORRECTED WINDOW, BOTH ARMS (cycle 87). The arena is 8 templates.

**The sizing input existed only as an instance count until now.** Every G3 figure
on this board counts list *instances*; the arena is sized by *unique templates*,
and nobody had counted those. Measured with `gNdsEffectDLCensus*`
(`reloc_backend_renderer_dl.c`, all behind `#if NDS_TICK_HUD`), whole match,
1,600 samples, frames 442–2041, **86.7% coverage, DLDI on, exclusion OFF**.
Builds `builds/build-c87-census-{boundary,bothcpu}`; artifacts
`artifacts/performance/2026-08-05_c87-census-{boundary,bothcpu}.json`.

| | **both-CPU (gate)** | **Boundary (shipped)** |
|---|---:|---:|
| **unique templates** | **8** | **8** |
| **command total over the 8** | **669** | **669** |
| **max commands in one template** | **336** | **336** |
| overflow / state variants / cmd variants | 0 / 0 / 0 | 0 / 0 / 0 |
| instances / match | 581 | 1,366 |
| reuse factor | 72.6x | 170.8x |
| commands / instance | 111.2 | 159.5 |
| Exec ticks / instance | 80,394 | 101,359 |
| texture ticks / instance | 19,406 (24.1% of Exec) | not read this run |
| triangles / instance | 13.65 | 16.07 |
| vertices / instance | 40.95 | 48.2 |
| terminated at `G_ENDDL` | 581/581 | 1,366/1,366 (0 at cap) |

**THE TEMPLATE SET IS ARM-INDEPENDENT.** Uniques, command total and command max
are *identical* on both arms while instances differ 2.35x. The 8 templates are a
property of the owner-approved effect model set, not of match dynamics — so a
fixed arena's size does not depend on how hard the match is, which is what makes
it safe to fix at build time.

**The pointer is a COMPLETE key: `StateVariants` 0 and `CommandVariants` 0 on
both arms.** No template was ever submitted under a different entry
`othermode_l`, and none ever produced a different command count. One packet per
unique template suffices; no state multiplexing, no per-instance variation to
encode. This was the open question that decided whether the arena is sized by 8
or by 8 x (number of entry states).

**The interpretation ratio is 325.7x on Boundary** (669 distinct commands
executed 217,920 times) and 96.6x on the gate arm.

Cross-checks, all independent: the run reproduces Boundary's banked per-list
constants (160.1→159.5 commands, ~102,730→101,359 ticks, 16.1→16.07 tris,
1,360→1,366 lists, all within 1.3%), and **the gate-arm figures the cycle-80
caveat flagged as 12.6%-window artefacts largely HOLD** — 527–563 lists → 581,
83,632 ticks/list → **80,394** (−3.9%). Vertices are exactly 3.000x triangles on
both arms: every triangle is an independent 3-vertex submit, no strip reuse.

Instrument cost **+3,488 bytes** (text +384, data 0, bss +3,104), headroom
130,976 → **127,488** (36.5x margin), `fake_heap_start` 0x02274864 → 0x02275604
— the address delta equals the section delta to the byte. Boot probe PASS
(frames 61–68, 8 samples, slips 0); arena `ChosenSize` 1,376,256 → 1,372,160 and
`AllocFailCount` 0 → 1, i.e. exactly one 0x1000 step, the predicted re-starve.
**The shipped ROM pays none of it** — every part sits inside
`diagnostics.c`'s `#if NDS_TICK_HUD` (3014–3150) and the published target builds
`NDS_TICK_HUD 0`; the `NDS_TICK_HUD=0` configuration was link-checked via
`smash64ds-battle-playable-proof-hwtri`.

Both census arms are identity-checked against their banked baselines: Boundary
WORK-H P95 1,480,576 vs 1,476,672 (+3,904) and gate 1,621,696 vs 1,624,064
(−2,368) — both inside the ±5,376 floor, so the instrument does not move what it
measures. **Neither is a new baseline; 1,624,064 still stands.**

**THE ARENA CONSTANT IS MEASURED (cycle 87, step 0). 83 triangles / 249
vertices / 669 commands over the 8 templates — identical on both arms.**
Artifacts `artifacts/performance/2026-08-05_c87-geomcensus-{boundary,bothcpu}.json`.

| | gate | Boundary |
|---|---:|---:|
| tris over the 8 templates | 83 | 83 |
| verts over the 8 templates | 249 | 249 |
| `GeomVariants` / instances | 2 / 581 | **0** / 1,366 |

`hardware_triangle_count` is POST-CULL, so per-instance geometry can vary and a
packet must encode the template's whole content — the census therefore records
the **max** per template, not a first sighting. `GeomVariants` is its confidence,
and at 0 (Boundary) and 2 of 581 (gate) the geometry is frame-invariant, so the
maxima are exact rather than a lower bound. Verts are exactly 3x tris on both
arms, consistent with the per-instance 3.000x ratio.

**The inference this replaced was 20% low** (~67 tris / ~200 verts from the 1.91x
command skew), which is why §3.11 requires the arena constant to be measured: an
undersized arena is a freeze, not a slowdown.

Packet-byte estimate on the measured basis: 249 verts x 16 B (VTX_16 two words +
TEXCOORD + COLOR) = 3,984, plus <=83 BEGIN_VTXS (332), plus per-template state
(matrix 4x3, polygon format, texture params, colours ~80 B x 8 = 640), plus
packed-command-byte overhead — order **6 KB for the entire effect set**. A 16 KB
fixed arena is 2.6x margin; 32 KB is 5.2x. Against 125,248 bytes of proven
static headroom this is not a constraint.

**8 remains a LOWER bound on templates**: the census counts templates actually
submitted, so the builder must enumerate the closed effect-model set at match
load and build eagerly. Lazy discovery would be gameplay-time allocation, and
§3.11 makes that a freeze. Overflow policy: size at build time with a
compile-time assert, and fall back to the interpreter for any list not in the
prebuilt set — correctness-preserving and allocation-free.

### G3 step 1 — THE PACKET DESIGN IS REFUTED AS BRIEFED (cycle 88). Effect geometry is the per-instance data, not the invariant.

**The row assumed geometry is template-constant and that matrix + colour are the
per-frame patch. Measured, it is exactly inverted.** Boundary arm, frames
900–907, 645 effect list instances over 7 templates, DLDI on, build
`builds/build-tick-hud-buckets`; artifact
`artifacts/performance/2026-08-05_c88-effect-packet-stream-boundary.json`.
The instrument hashes the captured GX word stream per list in three classes and
compares each against that template's first sighting:

| | matches | variants |
|---|---:|---:|
| **geometry** (VERTEX16 + TEX_COORD + BEGIN/END/POLY_FMT) | **0** | **638** |
| colour | 638 | 0 |
| matrix | 638 | 0 |
| geometry **word count** | — | **0** |

638 = 645 − 7, i.e. **every comparable instance varied, 100%**, while the stream
LENGTH never varied once. Colour and matrix words are byte-identical across
instances and are the positive control: the comparator demonstrably detects a
difference (638 times) and demonstrably reports agreement (1,276 times), so
"geometry always differs" is a reading and not a broken hash.

**What it means.** If the hardware were transforming, every instance of a
template would emit identical model-space vertices and the MATRIX would carry
the per-instance difference. The matrix is constant and the vertices move, so
**the transform is already baked into the vertex words** — effect lists take the
CPU-projected submit shape (`ndsRendererHardwareClipVertex` divides x/w and y/w
and emits screen-space v16). A packet captured from that stream pins the effect
to the position and camera of the capture instance. **This is R2-02 E3's "smear
of specks" failure, measured on the effect layer for the first time.**

Patching "the matrix and the dynamic colour words" therefore patches the two
things that were already constant, and leaves all ~235 geometry words per list —
the actual per-instance payload — unpatched. **The briefed design cannot work on
the current submit path**, and no arena, capacity or overflow policy changes
that.

**The precondition this creates, and it is the real G3 row now:** effects must
first be moved from the projected submit shape to the raw one (load the
composed matrix into GX once per list, emit model-space vertices). Then geometry
becomes template-constant and the packet design works with the matrix as the
per-instance patch, exactly as briefed. `ndsRendererHardwareClassifySubmit`
(`nds_renderer.c`) returns `PROJECTED_NO_Z` on its **first** predicate when
`source_zbuffered == FALSE`, before any range or matrix-compatibility check, so
effects plausibly never reach the raw path at all — that attribution is read
from the code and is NOT yet measured; the class histogram is the next probe.

**The mechanism G3 wants already exists: Task 36 replay.** `NDS_TASK36_HW_COMPOSE
== 2` is already on in the tick-HUD ROM. `NDSRendererTask36ReplayOwner`
(`nds_renderer.c`) is a fixed 4,608-word static arena with per-run word offsets,
a segment admission mask, capture/replay states and a real packed DS display
list — `ndsRendererTask36ReplayOpcode` encodes FIFO opcodes with parameter
counts and deliberately drops the state classes its BeginRun re-issues live.
Its `NDS_TASK36_REPLAY_SEGMENT_MASK` comment already states the rigid-versus-
dynamic law this measurement just confirmed for effects. **Do not build a second
packet arena; admit effects to this one once their stream is rigid.**

Instrument: `gNdsEffectPacket*`, all behind `#if NDS_TICK_HUD`, hooked into the
GX record funnel in `nds_renderer.c` and compared beside the census in
`reloc_backend_renderer_dl.c` (same key, same population). Cost **bss +5,032,
text ~+912** (nm-measured); headroom 114,272 proven, `check-boot-headroom.ps1`
OK. **The shipped ROM pays nothing** — the `NDS_TICK_HUD=0` link check
(`smash64ds-battle-playable-proof-hwtri`) contains **0** `EffectPacket` symbols.
Engagement is exact rather than plausible: capture count 645 equals
`gNdsEffectDLSubmitCount` 645, templates 7 equals `gNdsEffectDLCensusUnique` 7,
and the capture's own VERTEX16 command total **31,941 equals the renderer's
independent `gNdsEffectDLVertexTotal` 31,941** — the capture sees the effect
layer's complete vertex stream and nothing else. Verts/tris is exactly 3.000,
reproducing the census.

### G3 step 2 — THE DECIDING PREDICATE IS NAMED (cycle 89). It is the FIRST one, and it is load-bearing.

**100.000% of effect triangles are refused the raw path by
`source_zbuffered == FALSE`, the first test in
`ndsRendererHardwareClassifySubmit`.** Boundary arm, frames 900–907, DLDI on,
build `builds/build-tick-hud-buckets`; artifact
`artifacts/performance/2026-08-05_c89-effect-submit-class-boundary.json`.
Binned at each return site, so the index names the *predicate*, not merely the
resulting class (the two sites returning `PROJECTED_RANGE_OR_MATRIX` are split
by hand into bins 3 and 4 for that reason):

| bin | predicate | effect triangles | share |
|---|---|---:|---:|
| **0** | **`source_zbuffered == FALSE`** | **10,647** | **100.000%** |
| 1–4 | decal / prim depth / range reject / matrix reject | 0 | 0% |
| **5–6** | **RAW current + RAW snapshot (the raw path)** | **0** | **0%** |
| 7 | cross-matrix | 0 | 0% |

**Not one effect triangle reaches the range or matrix checks.** The exclusion is
one predicate deep, and the range/matrix conditions are untested rather than
failing — so nothing is known about whether effect geometry would satisfy them.

Engagement is exact: `gNdsEffectSubmitTotal` **10,647 == `gNdsEffectDLTriangleTotal`
10,647**, the renderer's own independent effect triangle count, so the histogram's
population is precisely the effect triangles and nothing else. The cycle-88
result reproduces on this separately linked ROM (SHA `1EA9CE6E` vs `1E06AFAF`):
645/645 captures, 638 geometry variants, 638 colour matches, 31,941 vertices.

**Why the predicate is load-bearing, and this is the part that decides the row.**
`source_zbuffered` is the source display list's own `G_ZBUFFER` geometry-mode bit
(`nds_renderer.c`, `stats->geometry_mode & NDS_RENDERER_GEOM_ZBUFFER`) — a
BattleShip asset property. The port's own comment states the constraint:
**"The DS cannot disable depth testing per polygon."** So for non-Z geometry it
reproduces N64 painter-order by handing every triangle its own monotonically
descending depth (`sNdsRendererHardwareProjectedDepth`, step 6) and **emitting
that z explicitly** — which requires the CPU to own the vertex, hence the
projection, hence cycle 88's per-instance geometry. The projected path is not an
oversight to be flipped; it is how draw order is reproduced on hardware with no
per-polygon depth-test disable.

**Consequence: there is no raw path for non-Z geometry today.** Bin 0 is not a
gate that effects fail, it is a gate they are not eligible for. Making effects
raw means giving them hardware-computed depth in place of the port's synthetic
painter order — a **visual** change to layering, and therefore the owner's call.
See the escalation below; do not route effects to the raw path without it.

### G3 step 3 — OPTION B IS SMALL, AND THE COST IS GENUINELY THE PROJECTION (cycle 90)

Boundary arm, frames 900–907, 645 instances, DLDI on, build
`builds/build-tick-hud-buckets`, ROM `573F4F41`; artifact
`artifacts/performance/2026-08-05_c90-effect-exec-split-boundary.json`. Three
spans measured inside the existing Exec bracket, armed by the cycle-88 flag;
**traversal is DERIVED** as `Exec − TexInExec − Vtx − Tri`, so a negative
residual would disprove the nesting. Residual **+16,802,176, non-negative.**

| span | ticks | share of Exec | per instance |
|---|---:|---:|---:|
| **Tri** (classify + clip divides + painter depth + GX emit) | 63,044,032 | **70.95%** | 97,743 |
| **Traversal** (derived: walk, dispatch, state, in-Exec texture) | 16,802,176 | **18.91%** | 26,050 |
| **Vtx** (G_VTX transform) | 9,006,080 | 10.14% | 13,963 |
| Exec | 88,852,288 | 100% | 137,755 |

**The bias correction closes on the banked figure to one tick, which is the
cross-check that makes this usable.** The brackets charge a timer read to the
span they wrap, and Vtx/Tri fire ~33x per list against Exec's once — so Exec
reads 137,755/instance against the banked **101,359**. Charging the whole
36,396 excess to Tri+Vtx gives 75,310, and 75,310 + 26,050 = **101,360 vs
101,359 banked.** Traversal is unaffected by the correction because it is
derived.

**Option B's recoverable, both ends measured:** **low 19,167/instance (18.91%
of banked Exec)**, **high 26,050/instance (25.7%)**. So B removes at most about
a quarter of Exec and **74.3% of the cost is per-vertex and per-triangle work B
must still pay every frame** — because cycle 88 proved the vertex words are the
per-instance payload. B is real but small, and it is small for the decisive
reason rather than an incidental one.

Engagement exact: `gNdsEffectPhaseTriCount` **10,647 == `gNdsEffectDLTriangleTotal`
10,647**; capture count 645 == `gNdsEffectDLSubmitCount`.

**Instrument defect, stated rather than papered over:** `gNdsEffectPhaseTexInExecTicks`
read **0**. There are three texture-resolve entry points charging
`gNdsEffectPhaseTexTicks`; only one was given the in-Exec twin and it never
fires while armed. In-Exec texture resolve is therefore folded into the derived
traversal residual. That is *correct for B's accounting* — texture resolve is
template-invariant, so a packet removes it — but it means the texture sub-share
is not separately reported, and `gNdsEffectPhaseTexTicks` (18,086,656) spans
work outside Exec so it cannot be substituted.

**THE GATE ARM IS NOW RUN (cycle 91), AND THE TEXTURE TWIN CHANGES THE
COMPOSITION — not B's size.** Whole match, both-CPU, 1,600 samples, frames
439–2038, **86.7% coverage**, DLDI on, exclusion OFF. Two runs: the as-built
`build-c90-split-bothcpu` (ROM `1A91A4A4`, artifact
`artifacts/performance/2026-08-05_c91-effect-split-bothcpu.json`) and, after the
`TexInExec` fix, `build-c91-slots-bothcpu`
(`...c91-painter-slots-bothcpu.json`). Figures below are the fixed build.

| span | ticks | share of Exec | per instance |
|---|---:|---:|---:|
| **Tri** | 41,091,968 | **70.98%** | 70,726 |
| **TexInExec** (was 0) | 12,506,624 | **21.60%** | 21,526 |
| **Vtx** | 5,142,656 | 8.88% | 8,851 |
| spans sum | 58,741,248 | 101.47% | — |
| **derived traversal residual** | **−848,384** | **−1.47%** | — |
| Exec | 57,892,864 | 100% | 99,643 |

**The residual is NEGATIVE, which is the instrument's own designed failure
signal, and it fires for the right reason.** With `TexInExec` honest, Tri + Vtx
+ Texture alone exceed Exec by 1.47%: every span charges its own timer reads and
Tri/Vtx/texture each fire many times per list against Exec's single bracket, so
the raw shares are biased high. The bias correction is mandatory, not optional.
**Consequence: the 18.91% "traversal" the Boundary run reported was mostly
texture resolve.** True walk/dispatch/state traversal is ≈ 0.

**Option B on the gate arm = `Exec − Tri − Vtx`** (a packet removes traversal
*and* the template-invariant texture resolve): **11,658,240 ticks/match,
20.14% of Exec**, i.e. **16,189–20,066 ticks/instance** (low end = that share of
banked Exec/instance 80,394; high end = measured). Boundary was 19,167–26,050.
Over 1,600 presented frames that is **5,879–7,286 ticks/frame — 1.2%–1.4% of the
503,684 gap.**

**And the prize is one G1 already tried.** `TexInExec` is 21,526 ticks/instance;
G1 measured route-0 `Tex` at 20,780 per list and cut it **65.3%** — and WORK-H
P95 moved +3,840, **inside the ±5,376 floor**. Two independent instruments price
the same cost, and a two-thirds cut of it has already been measured to be worth
nothing at the gate. **B is not a gate lever on the arm the gate reads on.**

**Neither run is a baseline.** The phase instrument perturbs what it measures:
WORK-H P95 reads 1,639,872 (c90) and 1,669,632 (c91) against banked 1,624,064 —
+15,808 and +45,568, both far outside the ±5,376 floor. **1,624,064 still
stands.** Engagement is exact on both: `gNdsEffectPhaseDLCount` 581 ==
`gNdsEffectDLSubmitCount` 581 == the c87 census instance count, and
`gNdsEffectDLCensusUnique` 8. One honest mismatch: `gNdsEffectPhaseTriCount`
7,946 vs `gNdsEffectDLTriangleTotal` 7,930, **+16 (0.20%)** — the two closed
exactly on Boundary (10,647 == 10,647), so the gate arm has 16 triangles the
phase bracket counts and the census does not. Unattributed, and too small to
move any figure above.

**The `TexInExec` defect is FIXED (cycle 91).** Three texture-resolve entry
points charged `gNdsEffectPhaseTexTicks`; only `ResolveResidentTexture` had the
in-Exec twin and it never fired. `ndsRendererHardwareBindTexture` and
`ndsRendererHardwareResolveStageSourceFrameTexture` now carry it too. It reads
**12,506,624 == `gNdsEffectPhaseTexTicks` 12,506,624, i.e. 100% of effect
texture-resolve time is inside the Exec bracket.** That equality is informative
rather than tautological: `sNdsEffectPacketArmed` wraps only the
`ndsRendererExecuteDisplayListWithVertexCache` call
(`reloc_backend_renderer_dl.c:9492/9508`) while `gNdsEffectPhaseActive` wraps the
whole tree walk (9856/9861), so the twin's condition is strictly narrower and
*could* have differed.

### G3 step 4 — STATIC PER-LAYER WORLD Z CANNOT REPRODUCE THE CURRENT ORDER (cycle 90)

The owner's proposal — bake a fixed distinct world-Z per effect layer and let
the depth test reproduce painter order by construction, giving A's win without
A's fidelity cost. **Verdict: no, not as stated, and none of the fighter-Z
measurements is what kills it.** Read from the specification, no build:

- **The scheme orders per PRIMITIVE, not per layer.**
  `ndsRendererHardwareNextProjectedDepth` decrements by `STEP` and returns
  `counter / STEP`, giving **every no-Z primitive its own depth slot in
  submission order**. Its comment records the exact failure a per-layer
  constant would reintroduce: *"Subtracting one here made six consecutive no-Z
  triangles share a depth after division, allowing an earlier stage triangle to
  reject a later grass/bush draw."* Sharing a depth across primitives is a
  **known, already-observed rendering bug**, not a hypothetical.
- **The order is dynamic and scene-dependent.** The depth a given effect
  receives depends on how many painter primitives preceded it *that frame*, and
  on whether the first source-Z triangle has flipped the counter via
  `ndsRendererHardwareEnterProjectedForeground` into its foreground range. A
  static constant cannot reproduce an order defined by submission position.
- **Precision budget, from the constants — BOTH NUMBERS IN THIS BULLET WERE
  WRONG. Corrected and measured in step 5 below; do not quote this bullet.**
  The reserved band is **128 slots per endpoint, not 4,096** (the `0x1000` is
  the v16 representation of clip-space 1.0, not a slot count; the literal `128`
  in `FOREGROUND_START` is the band width, and
  `ndsRendererHardwareSourceDepthToV16`'s comment says so outright). And effects
  consume **~3.9 primitives/frame, not 1,331** — that figure divided
  `gNdsEffectDLTriangleTotal`, which is cumulative from boot (one write site, a
  `+=`, no reset anywhere), by an 8-frame window. The two errors run in opposite
  directions; the conclusion "the slots are already substantially spent" is
  **refuted by measurement** in step 5.

**Consequently fighter-Z constancy (the owner's premise) is not the binding
constraint and was not measured.** It would matter only if the target were
per-object ordering; the specification is per-primitive. Reporting that the
premise is untested is the honest form — it may well be true and still not help.

**A variant does survive and is worth pricing, but it is a hypothesis, not a
result.** Bake each template's per-triangle ordering into its model-space Z
(the template's primitive set is fixed — 83 triangles over 8 templates), and
carry the per-instance base depth in the **patched matrix's Z translation** —
which is the one patch the original G3 design already called for. That
reproduces per-primitive order by construction *if* base offsets are assigned
in submission order. **ANSWERED, cycle 91 — see step 5. The precision budget is
comfortable and the variant fails anyway, for a structural reason that no amount
of depth resolution fixes.**

### G3 step 5 — DEPTH PRECISION IS NOT THE CONSTRAINT, AND THE VARIANT IS DEAD ANYWAY (cycle 91)

**Verdict: NO.** Not on precision — on mechanism. The A-branch is closed.

**Measured first, because it refutes the stated reason for doubt.** New census
`gNdsPainterSlot*` (`nds_renderer.c`, all `#if NDS_TICK_HUD`), derived from the
depth counter itself rather than incremented in
`ndsRendererHardwareNextProjectedDepth`, because the M3 replay path decrements it
in bulk (`triangle_count * STEP`) without calling the accessor. Folded once per
renderer hardware frame. Gate arm, whole match, 1,600 samples, frames 439–2038,
86.7% coverage, DLDI on, build `builds/build-c91-slots-bothcpu`; artifact
`artifacts/performance/2026-08-05_c91-painter-slots-bothcpu.json`.

| | measured | band | worst-frame use |
|---|---:|---:|---:|
| background slots, max frame | **72** | 128 | **56.2%** |
| foreground slots, max frame | **107** | 128 | **83.6%** |
| frames over either band | **0 / 2,044** | — | — |
| background / foreground mean | 71.8 / 57.8 | — | — |

Engagement: `gNdsPainterSlotFrames` **2,044** tracks the 2,038 presented frames,
so the fold ran once per hardware frame across the whole match. **Zero over-band
frames in 2,044 folds**, and 21 free slots in the tighter band at the worst
frame of the match. The budget is not spent.

**Where the ordering primitive comes from, and why that is fatal.** Painter
primitives are emitted through `ndsRendererHardwareClampS64ToV16(projected_z)`
(`nds_renderer.c`) — the slot **integer goes straight into the vertex z with no
perspective divide** — and the projected path loads **identity for both
projection and modelview**, so clip `w` is exactly 1.0 and the clip test
`|z| ≤ w` becomes `|z_v16| ≤ 4096`. That is the whole mechanism: integer slots,
uniform spacing, no perspective crowding, and the DS is in **Z-buffering** mode
(`glFlush(GL_TRANS_MANUALSORT)`, bit 1 clear).

**The packet and the painter order are mutually exclusive.** The integer depth
slot exists *only* because the CPU owns the vertex on the projected-identity
path — and CPU-owning the vertex is exactly what makes effect geometry
per-instance and un-packetable (cycle 88, 638/638). Moving effects to the raw
path to make geometry template-constant replaces identity with `RAW_COMPOSED`, a
real perspective matrix: the emitted z is then divided by a per-vertex `w`, so
"one integer slot per primitive" ceases to exist. A baked model-space Z and a
matrix Z-translation preserve *order* (the map is monotonic in view Z) but not
*spacing* — the separation two baked offsets produce depends on the instance's
distance from the camera, which a build-time bake cannot know. Solving for the
offset per instance per frame is precisely the per-frame CPU work the packet
existed to remove.

**So the deciding question was mis-aimed, and both of its inputs were wrong**
(step 4 bullet 3, now corrected): the budget is 128 per band rather than 4,096,
and demand is ~3.9 effect primitives/frame rather than 1,331. Fixing both makes
the precision picture *better*, and the variant still fails.

**Not measured, and not needed:** whether a single effect list ever straddles the
`ndsRendererHardwareEnterProjectedForeground` switch. Background use is
essentially constant (mean 71.8, max 72) so the switch fires at a stable point
every frame, but a static intra-template Z could not express that discontinuity
anyway. **Also still true and still the owner's call** (cycle 89): routing
effects to the raw path at all is a visual change to layering.

**Consequence for the lane:** G3's A-branch is closed. B is priced and small
(step 3). The remaining G3 question is whether anything is worth doing here at
all, given `SRC` is 8.6x the `MISC` lever on this arm.

**Actionable, found in passing (cycle 91):** `/artifacts/` is gitignored
(`.gitignore:24`), and **no artifact this board cites from cycles 85–90 is
actually tracked** — `git ls-files artifacts/performance` matches none of them.
The 197 JSON and 44 CSV files that *are* tracked predate the rule. So every
"artifact `artifacts/performance/...json`" citation on this board is a path that
does not exist in a fresh clone, which makes the evidence unreproducible for
anyone but the machine that ran it. Either force-add cited evidence (`git add
-f`) or stop citing paths as if they were committed; do not leave it ambiguous.
Cycle 91's four artifacts are on disk and uncommitted, matching current practice.

### G3 — RE-PRICED ON THE GATE ARM (cycle 79). The prize is 4–9x smaller than this row claims.

**Every number below this heading is Boundary-derived and carries no arm
label. The gate reads on both-CPU, and on that arm they do not hold.**
Measured on `build-c79-g1-bothcpu`, route 0 (shipped), whole match, 1600
samples, frames 441–2040, stride 96, DLDI on:

| | Boundary (banked, unlabelled) | **both-CPU (gate arm)** |
|---|---:|---:|
| effect display lists / match | 1,360 | **527–563** |
| Exec ticks / list | ~102,730 | **83,632** |
| effect submits as share of `MISC` excursion | 99.3% | **71.5%** |
| recoverable on WORK-H P95 | ~315,000 | **33,699 – 75,264** |

**CAVEAT, cycle 80 — the gate-arm column came off the 12.6% window.** It was
measured on `build-c79-g1-bothcpu`, which still seeded the 420-second match, and
was labelled "whole match". The same applies to G1's *2,953 consults over 563
lists*. Counts **per window** are unaffected (both are 1,600 presented frames),
but "per match" is the wrong denominator, and the effect density of an opening
minute is not that of a full match with its KO-heavy endgame. **RESOLVED, cycle
87 — re-measured on the corrected window (see the G3 census section above), and
the gate-arm column largely HELD: 581 lists/match at 80,394 ticks/list, against
527–563 at 83,632.** Already re-derived: the
`MISC` share of the WORK-H excursion is 25.7% (was 29.1%), and the `MISC` lever
is 48,002 (was 58,240) — still inside the bracket below.

The recoverable is a bracket, both ends measured on this arm: 33,699 charging
each ring stop's effect ticks uniformly across its 96 frames, 75,264 charging
all of them to that stop's most expensive frames (concentration-favourable
upper bound). **Removing 100% of effect DObj submits leaves WORK-H P95 at
1,536,768–1,578,333 against a 1,120,380 gate — a residual gap of
416,388–457,953.** G3 cannot close the gate on the arm the gate reads on.

**RETRACTED (cycle 79, same author): "`OTHR` owns 48.3% of the gate-arm
excursion" was wrong.** `OTHR` is not a region's cost, it is an accounting
remainder — `taskman_seam.c:5137` computes it as `ALL - named`, and `named`
does **not** include `WAIT`, so `OTHR` still contains the VBlank idle that
Task 66 later broke out separately. The retracted table ranked `OTHR` while
excluding `WAIT` as untargetable, double-counting the same idle time; their
excursions differed by 0.04%.

The exact identity, verified frame-by-frame with **max error 0** over 1600
frames:

```
WORK-H = (FTR + STG + BG + AUD + SRC + MISC) + (OTHR - WAIT)
```

`OTHR - WAIT`, the true unattributed work, is **flat ~19,159 ticks/frame**
(P50 19,136, P95 19,776, range 17,984-20,352) and contributes **89 ticks** to
the hot-vs-clean excursion. It is a P50 constant and is not a lever in any
form. **`OTHR` needs no further attribution; this closes it.**

**What actually owns the tail — the two arms are INVERTED, and cycle 80 CONFIRMED
it is not a window artefact.** Mean on over-gate frames minus mean on clean
frames (the metric that separates gate levers from P50 levers). Owners sum
exactly to the WORK-H delta on both arms. Both columns below are now the
**corrected 60-second match at 86.7% coverage**, same window, same method:

| owner | **both-CPU** (gate) | **Boundary** |
|---|---:|---:|
| **SRC** | **216,083 (68.9%)** | 91,350 (27.8%) |
| **MISC** | 80,642 (25.7%) | **232,263 (70.6%)** |
| AUD | 12,075 (3.8%) | 7,602 (2.3%) |
| STG | 8,161 (2.6%) | 2,146 (0.7%) |
| `OTHR-WAIT` | 149 | 158 |
| BG | 23 | 2 |
| FTR | −3,442 (−1.1%) | −4,393 (−1.3%) |
| WORK-H hot−cold | 313,690 | 329,127 |

**The inversion survived a 6.9× change in window size.** Gate-arm `SRC` share
across three windows: 69.6% (12.6% of match), **68.9%** (86.7%), 67.4%
(gameplay-only, 441–1997). `MISC`: 29.1%, **25.7%**, 26.7%. The shares are
stable, so the two arms genuinely have different primary owners and the
two-track scope stands on measurement rather than on an artefact.

Method note: `scripts/analyze-tick-hud-excursion.ps1` computes this and **fails
closed** — it verifies the per-frame identity `WORK-H = (FTR+STG+BG+AUD+SRC+
MISC) + (OTHR−WAIT)` (max error 0 over 1,600 frames on both arms) and refuses to
print a ranking whose owners do not sum to the WORK-H delta. It was validated by
reproducing the cycle-79 both-CPU table and this Boundary table to the digit
before being trusted on new data.

**G3's lane was built on Boundary, where `MISC` genuinely is the tail at
70.6%. The gate reads on both-CPU, where `SRC` is the tail at 69.6% and
`MISC` is secondary.** `SRC` is inflated 1.54x at P95 by the stress config
(P95 547,648 → 842,816) but is **not** a config artefact: it is still 27.8%
of Boundary's excursion. FTR is anti-correlated on both arms, independently
reproducing the existing Parked note.

**Levers priced on the gate arm**, counterfactual "bucket never exceeds its
own clean-frame mean" (an **upper bound** per lever — it assumes the entire
hot-frame excess is removable, which for `SRC` it is not, since some excess
is genuine extra AI work):

**RE-DERIVED, cycle 80, on the corrected 86.7% window** (clean-frame means
`SRC` 309,210, `MISC` 112,830):

| | WORK-H P95 | delta | over gate | residual vs 1,120,380 |
|---|---:|---:|---:|---:|
| baseline | 1,624,064 | — | 704 | 503,684 |
| **SRC capped** | 1,209,050 | **415,014** | 202 | 88,670 |
| `MISC` capped | 1,576,062 | 48,002 | 504 | 455,682 |
| **SRC + MISC** | 1,085,504 | **538,560** | 64 | **−34,876** |

The `MISC` figure (48,002) still falls inside the independently-derived
33,699–75,264 effect bracket, which cross-validates the method. **`SRC` is now
worth 8.6x the `MISC` lever** (was 6.8x on the bad window), and the two
together still put the gate arm inside budget — but by only 34,876, not
57,788, so the combined lane has less margin than the superseded figures
promised.

The combined figure is **super-additive** — 415,014 + 48,002 = 463,016, but
capping both moved P95 by 538,560. That is P95 being a position in a sorted
list rather than a sum. It is not an arithmetic error; do not "correct" it
into an addition.

### The SRC split — MEASURED, cycle 85. Hit detection is NOT the owner.

The instrument booted and both arms ran. `SBAS` (the decomp sim path) owns
`SRC`, not hit detection, and **the ratio is arm-independent** — which is what
makes it a structural finding rather than a config artefact. Whole match, 1,600
samples, frames 442–2041, stride 96, DLDI on, 86.7% coverage, `slips=0` on both.
Builds `builds/build-c85-src-bothcpu` and `builds/build-c85-src-boundary`;
artifacts `artifacts/performance/2026-08-05_c85-{gate-bothcpu,boundary}-*.json`.
Both identities close with **max per-frame error 0**.

| sub-owner | both-CPU excursion | % of SRC | % of WORK-H | Boundary excursion | % of SRC |
|---|---:|---:|---:|---:|---:|
| **SBAS** decomp sim path | **184,316** | **87.1%** | **61.5%** | **80,768** | **89.0%** |
| `SHDT` hit detection | 27,389 | 12.9% | 9.1% | 10,019 | 11.0% |
| `SWRM` anim warm | 12 | 0.0% | 0.0% | 6 | 0.0% |

Lever prices, same counterfactual as the table above (cap the bucket at its own
clean-frame mean — an **upper bound**, it assumes every hot-frame excess is
removable). `artifacts/performance/2026-08-05_c85-src-lever-prices.json`:

| lever | both-CPU delta | residual vs gate | Boundary delta |
|---|---:|---:|---:|
| **SBAS capped** | **315,456** | 177,092 | 73,685 |
| `SHDT` capped | 55,104 | 437,444 | 5,824 |
| `SWRM` capped | 0 | 492,548 | 65 |
| `SBAS` + `MISC` | 454,280 | **38,268** | 369,442 |

`SHDT` + `SBAS` capped reproduces `SRC` capped to the digit (391,552 on the gate
arm), which is the cross-check that the split is exhaustive: `SWRM` is inert, so
the two remaining sub-owners are all of `SRC`.

**Consequence for the lane.** `SBAS` + `MISC` leaves the gate arm 38,268 over,
where `SRC` + `MISC` lands 47,384 under — the difference is exactly `SHDT`. So
the gate needs the sim-path residual, the packet path, **and** a slice of hit
detection; no two of the three suffice.

**`SBAS` is a residual, not a target.** It is everything in
`ndsTask39EffectsUpdate` + `scVSBattleFuncUpdate` except the hit search and the
warm step: the fighter animation/event interpreter, physics and status
transitions, CPU AI, particle bytecode, map collision, camera. **Splitting it is
the next instrument row** — it cannot be optimised as one thing, and the same
mistake that made `MISC` a campaign is available here.

### The SBAS split — MEASURED, cycle 86. `SGCO` owns it; `SCPU` is a P50 lever.

**The composition above was VERIFIED before instrumenting, and it was wrong in
one load-bearing way.** `SRC` brackets `ndsTask39EffectsUpdate` +
`scVSBattleFuncUpdate` (`taskman_seam.c:4442-4466`, its *only* writer, run twice
per presented frame). But decomp's scene update is a one-liner
(`sccommon/scvsbattle.c:75`) calling `ifCommonBattleUpdateInterfaceAll`, whose
`game_status` switch reaches `ifCommonBattleGoUpdateInterface`, which **ends in
`gcRunAll` (`ifcommon.c:2970`)**. So `gcRunAll` is the SOLE gateway to the whole
simulation inside `SRC`, and it already had a port wrapper
(`battleship_sys_objman.c:75`) — **no `decomp/` edit was needed.** The fighter
proc chain is `ft/ftmanager.c:858-863`: six procs per fighter, priority 5→0
(`UpdateInterrupt`, `PhysicsMapDefault`, `PhysicsMapCapture`, `SearchCatch`,
`SearchHitAll` = `SHDT`, `Params`).

**Measured dead, do not re-nominate:** all seven fighter-loop branches in the
port's `scVSBattleFuncUpdate` (`battleship_scvsbattle.c:347-384`) read **0** on
the gate arm — `gNdsFighterNaturalMotionRunAllCount` and all six siblings. The
`gcRunAll` call at `reloc_backend_movement.c:12227` is in that dead chain.
Bracketing it — the obvious reading of the source — would have measured nothing.

Whole match, 1,600 samples, frames 443–2042, stride 96, DLDI on, **86.7%
coverage**, exclusion OFF, `slips=0`. Build `builds/build-c86-sbas-bothcpu`;
artifacts `artifacts/performance/2026-08-05_c86-gate-bothcpu{,-rows,-excursion}`.
Identity closes with **max per-frame error 0** and both derived residuals are
non-negative on every frame.

| sub-owner | excursion | %SRC | %WORK-H | clean mean | hot mean | **hot/clean** |
|---|---:|---:|---:|---:|---:|---:|
| **`SGCO`** unattributed in `gcRunAll` | **153,291** | **73.6%** | **52.0%** | 256,011 | 409,303 | 1.60x |
| `SHDT` hit detection | 25,995 | 12.5% | 8.8% | 4,470 | 30,464 | 6.81x |
| `SPRM` `ftMainProcParams` | 14,774 | 7.1% | 5.0% | 2,011 | 16,785 | **8.35x** |
| `SCPU` `ftComputerProcessAll` | 14,196 | 6.8% | 4.8% | 42,504 | 56,700 | **1.33x** |
| `SCAT` `ftMainProcSearchCatch` | 83 | 0.0% | 0.0% | 1,522 | 1,605 | 1.05x |
| `SWRM` anim warm | 2 | 0.0% | 0.0% | 597 | 600 | 1.00x |
| `SOUT` outside the sim | −100 | −0.0% | −0.0% | 4,490 | 4,389 | 0.98x |

**Boundary arm, same window/method** (`builds/build-c86-sbas-boundary`,
`WORK-H` P95 1,476,352 against the banked 1,476,672 — **320 apart**, a near-exact
reproduction). Identity max per-frame error **0**; `MISC` is still Boundary's tail
at 68.7% and `SRC` is 29.3%, so the two-track scope is unchanged:

| sub-owner | excursion | %SRC | %WORK-H | clean mean | hot mean | **hot/clean** |
|---|---:|---:|---:|---:|---:|---:|
| **`SGCO`** | **77,384** | **81.3%** | 23.8% | 254,300 | 331,683 | 1.30x |
| `SHDT` | 10,797 | 11.3% | 3.3% | 4,007 | 14,804 | 3.69x |
| `SPRM` | 5,338 | 5.6% | 1.6% | 1,819 | 7,156 | 3.93x |
| `SCPU` | 1,754 | 1.8% | 0.5% | 20,132 | 21,885 | **1.09x** |
| `SCAT` | 37 | 0.0% | 0.0% | 1,396 | 1,432 | 1.03x |
| `SWRM` | −1 | −0.0% | −0.0% | 585 | 584 | 1.00x |
| `SOUT` | −143 | −0.2% | −0.0% | 4,443 | 4,300 | 0.97x |

**`SGCO` owns `SRC` on BOTH arms — 73.6% and 81.3%.** Like the cycle-85 ratio,
that arm-independence is what makes it a structural finding rather than a stress
-config artifact. `SCPU` is flat on both (1.33x / 1.09x), so it is a P50 lever on
the gate arm and nothing at all on the shipped one.

**Engagement proof for the `SCPU` bracket, predicted before it was measured.**
The gate arm runs BOTH fighters as CPU and Boundary runs one, so the bracket must
read ~2x. Clean-frame means: **42,504 (gate) vs 20,132 (Boundary) = 2.11x.** The
span is measuring what it claims, and this is the negative control the flat
reading needed before it could be trusted.

**READ THE hot/clean COLUMN BEFORE NOMINATING ANYTHING.** `SCPU` is the trap:
72,512 ticks at p50 mid-match makes the level-3 CPU AI look like the prize, and
on the both-CPU arm it runs twice. But it is **1.33x** hot-vs-clean — a large
FLAT owner, i.e. a P50 lever that cannot move the gate, exactly like the
particles (flat ~47K) that already cost this campaign time. It is 6.8% of the
SRC excursion. Conversely `SPRM` is small in absolute terms yet switches
**8.35x**, the same bimodal shape that makes `SHDT` real. Absolute size and
switching behaviour are different questions and only the second owns a tail.

**What `SGCO` actually is, and what a split would have to bracket.** `SGCO` =
`GCRA − SCPU − SCAT − SHDT − SPRM`, i.e. everything inside `gcRunAll` that the
four bracketed spans do not claim:

| content | port-side wrapper? |
|---|---|
| `ftMainProcUpdateInterrupt` less its AI | **NO** — decomp name, unwrapped |
| `ftMainProcPhysicsMapDefault` | **NO** — decomp name, unwrapped |
| `ftMainProcPhysicsMapCapture` | **NO** — decomp name, unwrapped |
| `mpProcessUpdateMain` (map collision) | YES — `battleship_mpprocess_live_bridge.c:150` |
| camera / effect / item / weapon / interface GObj procs | mixed; not enumerated |

**The three unwrapped procs are why this split stopped where it did, and it is
NOT a `decomp/` question.** `linker/nds_hot_text.ld:207-209` pins all three into
ITCM **by exact symbol name** under a size ASSERT. The port's own rename-on-
include pattern (`src/import/battleship_ftmain.c:47-70`) could wrap them without
touching `decomp/` — but renaming them breaks those linker-script matches and
moves the code out of hot text, changing the very cost being measured. **A next
cycle that wants them must move the `.ld` entries in the same edit**, and should
expect `mpProcessUpdateMain` to be nested inside the physics procs (so it is an
overlay, not a disjoint owner — do not subtract it blindly).

**Instrument cost and health.** +2,368 bytes (four ring buckets) on top of
cycle 85's +1,152; `fake_heap_start` `0x02273f24` → **`0x02274864`**, leaving
**130,976 bytes proven headroom** (`check-boot-headroom.ps1` OK). Boot probe
PASS (frames 60–67, 8 samples, `slips=0`), and the arena stayed at its full
request (`ChosenSize` 1,376,256, `AllocFailCount` 0) — the instrument did not
re-starve what G2 freed. `named` read 959,960 on the boot probe, exactly
FTR+STG+BG+AUD+HUD+SRC+MISC, proving the four new buckets are excluded from
`named`; had they been included it would have read 1,226,456. **The shipped ROM
pays nothing** — every bucket, global, bracket and name is behind
`#if NDS_TICK_HUD`, and the published `-hwtri` target builds `NDS_TICK_HUD 0`.

**A liveness lesson worth keeping: `SCPU` reads 0 at frames 60–67.** The CPU AI
genuinely is not called during the pre-match countdown, so the boot probe alone
would have looked exactly like a dead counter. It was proven non-zero (72,512
p50) by an 8-sample probe at frame 600 **before** the whole-match runs were
spent — standing rule 3 doing its job.

**Do NOT re-bank the gate baseline on c85 (cycle 86).** Three whole-match
gate-arm `WORK-H` P95 readings on identical terms (both-CPU, 86.7% coverage,
1,600 samples, DLDI on, exclusion OFF): c80 **1,624,064** (banked), c85
**1,612,928** (−11,136), c86 **1,625,088** (**+1,024**). c86 carries *more*
instrument than c85 yet lands inside the ±5,376 floor of the banked figure, so
c85's dip is not reproduced and its attribution to G2's z-buffer free was
premature. **1,624,064 stands.**

**`SWRM` is measured INERT over the gate window, and that is a negative result
worth keeping.** It was designed as the honest load signal the circular
`SRC > 2x median` rule needs. It cannot serve: the 41-entry warm list is
exhausted long before the window opens at frame 442, so `SWRM` is a flat ~772
ticks/frame constant (hot 783, clean 772, p50 768 on both arms). A load-signal
rule for this window needs the Task 75 asset-load counter instead.

**Owner decision 2026-08-05: both tracks are in scope. G3 is NOT parked.**
These numbers force it — SRC alone leaves 97,243 over gate, `MISC` alone
leaves 433,412, and only both together land inside. SRC is necessary but not
sufficient. G3's disposition reads **"required, second in order, and primary
for the shipped configuration"** — not "refuted". What was refuted is only
the claim that it alone closes the gate. The second, independent reason to
keep it: **the shipped ROM is Boundary**, and on Boundary `MISC` is 70.6% of
the excursion against SRC's 27.8%, so the packet path is the *dominant*
lever for the configuration that actually ships. Two arms, two different
primary owners, both legitimate. G2's ≥32 KB headroom is therefore back on,
because it funds G3.

**SRC is not a charter §7 question yet and must not be escalated as one.**
`PROJECT_GOAL.md` requires exhausting specialization, approximation,
precomputation, lower-frequency processing, interpolation, event-driven
updates, simplified representations and DS-specific implementations *first*,
and it explicitly encourages fighter- and move-specific native code,
precomputed hitbox trajectories, large lookup tables, and compile-time
baking. `decomp/` is read-only as a **tree**, not as an **algorithm**: a
mechanically equivalent DS-optimized port-side equivalent is wanted, not a
compromise. Rate reduction and simulation-rate change are the LAST resort
and the owner's call.

**No longer provisional (cycle 80).** These shares were re-measured on the
corrected 60-second match at 86.7% coverage and moved by less than a
percentage point, so the two-track scope rests on measurement, not on the
window. The superseded 12.6%-window figures are in the cycle-79 archive.

### The SGCO split — MEASURED, cycle 92. `SITR` owns it on BOTH arms.

`SGCO` was 52.0% of the gate arm's WORK-H excursion and the campaign's largest
remaining lever. It is no longer a residual. The three per-fighter procs cycle 86
could not bracket are now bracketed, and the non-fighter GObjs fall out as a
derived remainder:

```
SGCO = SITR + SPHD + SPHC + SOBJ          (identity, checked per frame)
SITR = SINT - SCPU                        interrupt proc less its AI
SOBJ = GCRA - SINT - SPHD - SPHC - SCAT - SHDT - SPRM
```

Whole match, 1,600 samples, stride 96, **86.7% coverage, DLDI ON, exclusion
OFF**, `slips=0` both arms. Builds `builds/build-c92-sgco-{bothcpu,boundary}`;
artifacts `artifacts/performance/2026-08-05_c92-{gate-bothcpu,boundary}{,-rows,-excursion}`.
Identity closes with **max per-frame error 0** on both arms; both derived
residuals non-negative on every frame; the `SGCO` partition identity holds.

**Gate arm (both-CPU), frames 440–2038:**

| sub-owner | excursion | %SRC | %WORK-H | clean | hot | **hot/clean** |
|---|---:|---:|---:|---:|---:|---:|
| **`SITR`** interrupt less AI | **95,136** | **48.5%** | **31.8%** | 103,329 | 198,465 | **1.92x** |
| `SPHD` physics/map default | 25,828 | 13.2% | 8.6% | 71,725 | 97,553 | 1.36x |
| `SHDT` hit detection | 23,170 | 11.8% | 7.7% | 3,998 | 27,169 | 6.79x |
| `SOBJ` non-fighter GObjs | 19,967 | 10.2% | 6.7% | 80,099 | 100,066 | 1.25x |
| `SCPU` CPU AI | 19,240 | 9.8% | 6.4% | 38,588 | 57,828 | 1.50x |
| `SPRM` params | 12,853 | 6.5% | 4.3% | 1,765 | 14,618 | **8.28x** |
| `SCAT`/`SPHC`/`SWRM`/`SOUT` | 89 / 33 / 8 / −91 | ~0% | ~0% | — | — | ~1.0x |
| `SGCO` roll-up | 140,964 | 71.8% | 47.1% | 255,770 | 396,734 | 1.55x |

**Boundary arm (shipped), frames 438–2038:**

| sub-owner | excursion | %SRC | %WORK-H | clean | hot | **hot/clean** |
|---|---:|---:|---:|---:|---:|---:|
| **`SITR`** | **49,452** | **53.8%** | **13.6%** | 105,715 | 155,168 | 1.47x |
| `SOBJ` | 23,594 | 25.7% | 6.5% | 79,068 | 102,661 | 1.30x |
| `SHDT` | 9,290 | 10.1% | 2.5% | 4,176 | 13,466 | 3.22x |
| `SPHD` | 5,298 | 5.8% | 1.5% | 67,474 | 72,772 | 1.08x |
| `SPRM` | 4,721 | 5.1% | 1.3% | 1,929 | 6,651 | 3.45x |
| `SCAT`/`SPHC`/`SWRM`/`SOUT`/`SCPU` | 89 / 36 / 6 / −128 / **−419** | ~0% | ~0% | — | — | ~1.0x |
| `SGCO` roll-up | 78,380 | 85.3% | 21.5% | 252,899 | 331,279 | 1.31x |

**`SITR` owns `SGCO` on both arms — 67.5% of it on the gate arm, 63.1% on
Boundary.** Like the cycle-85 and cycle-86 ratios, that arm-independence is what
makes it structural rather than a stress-config artifact. It is also the highest
switching ratio of any large owner (1.92x), so it is a gate lever and not only a
P50 one.

Cross-checks: the `SGCO` roll-up's clean mean is **255,770 (gate) / 252,899
(Boundary) against banked 256,011 / 254,300 — 0.09% and 0.55% apart**, so the
instrument does not move the cost it subdivides. `SCPU`'s two-CPU control
reproduces: clean 38,588 (gate) vs 20,524 (Boundary) = **1.88x**, against the
expected ~2x.

**Three negative results, do not re-derive.** `SOBJ` is **flat on both arms**
(1.25x / 1.30x) — the camera, effects, items, weapons and interface GObjs inside
`gcRunAll` are **not** the tail, which closes the largest open hypothesis about
`SGCO`'s content. `SPHC` is ~0 on both arms while its bracket demonstrably runs
(clean mean 618/642) — the mutually exclusive capture arm costs nothing in a
match without grabs, exactly as predicted from ftmain.c:1918-1937. `SCPU` on
Boundary is **negative** (−419, 0.98x), confirming it is not a gate lever on the
shipped arm at all.

**`mpProcessUpdateMain` was NOT bracketed, deliberately.** It is the one seam
here with an existing port wrapper, but decomp has ~20 call sites across
`mp/mpcommon.c`, `it/itmap.c` and `wp/wpmap.c` — so it is an **overlay** spanning
`SPHD`/`SPHC` *and* `SOBJ`, not a disjoint owner, and the identity accounts for it
by not naming it (fighter-side cost inside `SPHD`/`SPHC`, item/weapon cost inside
`SOBJ`, no double count). Its call frequency is an order of magnitude above every
existing bracket, and the cycle-90/91 phase instrument already showed a
high-frequency bracket moving WORK-H P95 +15,808/+45,568. Measure it with a call
counter or on the fighter entry points only — never a timer on the shared leaf.

**The ITCM problem cycle 86 stopped at is solved, and the fix has two mirrors.**
The three procs were pinned by decomp symbol name at `linker/nds_hot_text.ld`.
They are renamed with the port's existing `#define`-before-`#include` pattern
(`src/import/battleship_ftmain.c`) and the three pins were rewritten **in place**
to `.text.battleship_ftMainProc*`. In place matters — the Task 94 note on that
file records the list is a curated 8 KiB working set whose members re-address
each other when the order changes. **Verified in the linked ELF, not assumed:**
`.text.hot` spans `0x020013c0`–`0x02002460` and all three renamed bases sit inside
it at their original positions (`0x16b8`/`0x1700`/`0x1748`), with the shared
`ftMainProcPhysicsMap` still pinned and the wrappers in `.main`. **No `decomp/`
patch was needed.** Two things must move with any future rename here:
`scripts/check-gbi-decode-fixtures.ps1:2098-2109` mirrors the pin list and is the
only guard that catches a half-done rename (an unmatched linker input-section
pattern fails **silently** — the member just drops out of hot text), and the
sampler's `$bucketNames` must move in the same commit as the enum.

**Instrument cost and health.** +1,832 bytes (text +232, bss +1,600) against
**111,584 proven headroom** — 61x margin, `check-boot-headroom.ps1` OK,
`fake_heap_start 0x02279424`. `named` = 1,112,522 = FTR+STG+BG+AUD+HUD+SRC+MISC
exactly, so the three buckets are excluded from `named` and the identity is
byte-identical to every banked measurement. **The shipped ROM pays no instrument
bytes** — the `NDS_TICK_HUD=0` link check contains **0** of the new symbols
against **3** in the lab ELF (a control that can find them). It does pay **one
extra call indirection per proc**, because the rename is unconditional: guarding
it on `NDS_TICK_HUD` would give the lab and shipped ROMs different ITCM layouts
and the measurement would no longer be of the shipped program. That is the same
cost `SCAT`/`SHDT`/`SPRM` have carried since cycle 86.

**NOT a new baseline. 1,624,064 stands.** These runs read WORK-H P95 1,650,240
(gate) and 1,589,056 (Boundary); the instrument perturbs and both runs used
`-AllowRepeatedFrames` (5/1600 gate, 3/1600 Boundary), which is valid for a
ranking and invalid for a per-presented-frame percentile. See `docs/VERIFYING.md`
for the flag's exact scope and for the stop-aligned repeat mechanism.

**INHERITED — the next row is NOT pricing `SITR` (owner, cycle 92).** The
excursion method is structurally blind to flat cost, and the gate arm's P50
already sits within ~26K of the 1,120,380 gate. So the next row measures the
**flat** buckets against their charter budget lines, which is §7 rung 1 and
**not** owner-gated. **DONE, cycle 93 — see "The flat buckets" below**: the
overage is 166,416 (31.4% of the gap), `FTR` owns 81.6% of it, and the row that
follows is the `FTR` phase split, because `FTR` is the only major bucket that
has never been partitioned. `SITR` pricing (specialization, precomputation,
event-driven status updates, fighter/move-specific native code, large LUTs)
remains available behind it, with
`docs/optimization/OPTIMIZATION_IDEAS.md` as the ideas store.

### The flat buckets — MEASURED, cycle 93. `FTR` owns 82% of the flat overage, and it is the one bucket never partitioned.

§7 rung 1, both arms, whole match, 1,600 samples, frames 441–2040, **86.7%
coverage, DLDI ON, exclusion OFF**, `-AllowRepeatedFrames` (3/1600 on each,
valid for a ranking), `slips=0` on both. Builds
`builds/build-c93-flat-{bothcpu,boundary}`, git `8770a246`; artifacts
`artifacts/performance/2026-08-05_c93-flat-{bothcpu,boundary}{,-rows.csv,-excursion.json}`.

**Both arms agree to within 1.2% on both buckets** — as expected for rendering
owners that do not depend on match dynamics, and the cross-check that makes
these usable.

| bucket | arm | mean | P50 | P95 | **clean mean** | hot mean | hot/clean | line | **over line** |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|
| **FTR** | **gate** | 383,930 | 394,944 | 398,080 | **385,814** | 382,216 | 0.99x | 250,000 | **+135,814** |
| FTR | Boundary | 383,693 | 394,112 | 397,504 | 381,085 | 386,354 | 1.01x | 250,000 | +131,085 |
| **STG** | **gate** | 214,338 | 210,240 | 218,496 | **210,602** | 217,735 | 1.03x | 180,000 | **+30,602** |
| STG | Boundary | 215,005 | 210,816 | 218,688 | 210,919 | 219,173 | 1.04x | 180,000 | +30,919 |

**Ranked flat overage, gate arm: `FTR` +135,814 (81.6%), `STG` +30,602 (18.4%),
total 166,416** — **31.4% of the gate arm's 529,860 gap**, and it is the part
the excursion method cannot see. Both are genuinely flat: `FTR` spread
P95/P50 **1.01**, `STG` **1.04**.

**The premise holds, and it is tighter than the brief assumed. Gate-arm
clean-frame P95 is 1,115,072 against the 1,120,380 gate — 5,308 of margin**
(Boundary 1,108,480, 11,900). The frames that are *not* excursions are already
at the budget, so every flat tick removed passes to P95 one-for-one.

**Correction to §7: the charter understates `STG`.** It says "`STG` at ~195K
against 180K"; measured clean mean is **210,602**, so the overage is **30,602,
not ~15,000 — 2.0x**. Its `FTR` figure is good (~389K stated, 385,814 measured).

**Two STG candidates REFUTED from the same run, no extra build.**
`-ExtraGlobals` on the Boundary arm, whole match (2,041 frames):

- **Stage-prepare rebuild is not happening.** `gNdsR2StagePrepareReuseCount`
  **2,039** against `gNdsR2StagePrepareBuildCount` **2** — the R2-02 E1a reuse
  key hits **99.9%**. There is nothing to win by making it hit.
- **Task 103's generic-emit lever is dead as a per-frame cost.**
  `gNdsStageGCDrawAllLoopDObjDrawCallbackCount` = **8 for the whole match**, so
  the traversal/run-loop site (`reloc_backend_movement.c:13625`) fires 8 times,
  not per frame — the Task 36 replay is armed and the loop only runs during
  capture. The archived 2026-07-27 figures ("21 generic runs, 103 triangles,
  63,903 ticks/frame", `ClaudeOpus5_Task103_TheStageIsNotWhereWeLooked`) no
  longer describe the steady state. **Do not restart that lever from that
  document.**

**What is left in STG is the display-commit site.**
`gNdsStageGCDrawAllLoopCapturedDisplayCount` = **56,178 = 27.5/frame**, i.e.
`ndsRendererAdapterCommitNativeStageDisplay` (`:13521`) is the only
high-frequency STG accumulation site remaining. `ndsRendererAdapterFinishNativeStageOwner`
(`reloc_backend_renderer_dl.c:8376`) is an **empty function** in this
configuration, so the `:14044` site is timer overhead only. The
Prepare-versus-Display split is **not** measured on the current build.

**FTR IS THE ROW, AND IT IS THE ONLY MAJOR BUCKET WITH NO PHASE PARTITION.**
`SRC` has three generations of splits (c85, c86, c92); `STG` has Task 103; `FTR`
has none on the tick instrument. Its bracket
(`reloc_backend_renderer_dl.c:14604–14678`) contains exactly two calls per
fighter — `ndsFighterDisplayContractCapture` then
`ndsFighterMarioFoxDLAllDrawForSlot` — at `gNdsFighterMarioFoxDLAllDrawCount`
**3,955 = 1.94 draws/frame** and `gNdsFighterDisplayContractSubmittedCount`
**63,534 = 16.1 contract events per fighter per frame**, i.e. **~198,900 ticks
per fighter draw per frame**.

A split already exists but **cannot be used**: `m2_contract_capture_ticks`
(`:14613`) is gated on `NDS_RENDERER_PROFILE_LEVEL == 1 &&
NDS_RENDERER_M2_DETAILED_LEDGER`, and the renderer benchmark path asserts
`TICK_HUD=0`, so it cannot run on the whole-match gate instrument. **Next row:
fork the two existing timestamps into `#if NDS_TICK_HUD` counters at those two
sites** — the Task 103 precedent exactly, which added *no* new timer reads
because every site already computed the stamp it needed. Cost is order tens of
bytes against **111,584** proven headroom
(`build-c93-flat-bothcpu` links at `0x02279424`), so the boot cliff does not
bind. Until that split exists, any FTR price is a guess: the honest bracket on
the lever is **0 to 135,814**, and naming a narrower one without the partition
would be the self-time-is-not-a-subsystem-budget mistake again.

### `FTR` is 86% draw / 14% capture — MEASURED, cycle 94. And the phase census below it is ITCM-blocked.

**Correction to the row above: `FTR` is NOT "never partitioned".** Task 91
(`NDS_TASK91_DRAW_PHASE_CENSUS`, default 0) is a complete partition of the draw
half — walk / reset / validate / owner-prep / matrix-prep / material-prep /
inputs / execute / total, plus five matrix sub-phases. **It no longer links.**
`build-c94-ftr-bothcpu` died at `region 'itcm' overflowed by 908 bytes`: its
timer sites sit in ITCM-pinned code (`ndsRendererExecuteNativeFighterOwnerProduction`
at `0x01ffe43c`), and ITCM is `0x7fe0` holding 101 text symbols. *That* is why
`FTR` has no partition on the whole-match instrument — not the absence of one.
Do not brief "add FTR brackets" without this: the brackets exist.

**The top-level split was taken a different way, with zero instrument.**
`NDS_R2_DRAW_SUPPRESS_MASK=3` (existing default-off lab flag, R2-03 E13) returns
from `ndsFighterMarioFoxDLAllDrawForSlot` at its first statement while the `FTR`
bracket still spans `ndsFighterDisplayContractCapture`, so residual `FTR` **is**
capture. Gate arm, whole match, 1,600 samples, frames 441–2040, 86.7% coverage,
DLDI ON, exclusion OFF, `slips=0`; artifact
`artifacts/performance/2026-08-05_c94-ftrsplit-bothcpu.json`.

| | build | `FTR` mean | `FTR` P50 | share |
|---|---|---:|---:|---:|
| control | `build-c93-flat-bothcpu` | 385,814 (clean) | 394,944 | 100% |
| draw suppressed | `build-c94-ftrsplit-bothcpu` | 54,342 | 55,360 | 14.1% |
| **draw (derived)** | | **331,472** | **339,584** | **85.9%** |

**Engagement is exact, not plausible:** `gNdsFighterMarioFoxDLAllDrawCount`
3,955 → **0**, `gNdsFighterDisplayContractSubmittedCount` 63,534 → **0**, and
both `gNdsFighterDLAllDrawP{0,1}HardwareTriangleCount` → **0**. Cross-build, so
it pays placement noise — but `FTR`'s own spread is **1.02** and the delta is
**60x** the ±5,376 floor, so the split is not in question.

**A CEILING, NOT A CANDIDATE.** Deleting both fighter draws outright moves
gate-arm `WORK-H` P95 **1,650,240 → 1,297,984 (−352,256)** and P50 1,128,192 →
777,216. Read it the way Task 106's −119,744 is read: the most the whole
fighter-draw lever can ever be worth, with no fighters on screen. VBI went
2:965 → 2:1704.

**REFUTED: the source display proc is not the cost.** Capture runs
`gmCameraLookAtFuncMatrix` (once per *fighter*, on the same camera),
`ndsFighterDisplayContractCountFlags`, and the decomp
`ndsBaseFTDisplayMainProcDisplay` over the fighter's DObj tree — **all of it
together is 54,342**, 14.1% of `FTR` and 4.5% of a clean frame. The duplicated
per-fighter camera matrix is therefore bounded above by a fraction of that and
is **not worth a build**. Spend the next row on the draw half, where 331,472 is.

**Next row is the Task 91 census made buildable**, and the cheap form is a
sub-flag excluding the five `gNdsTask91Mtx*` sites that live in ITCM-resident
code, then re-try. Do **not** un-pin ITCM symbols to make it fit: that moves hot
text and changes the very cost being measured (`linker/nds_hot_text.ld`, Task 94
note).

### Why the census cannot link — ATTRIBUTED, cycle 95. **ITCM is 99.1% full**, and that is a standing constraint, not a Task 91 problem.

Measured from the linked ELF of `build-c93-flat-bothcpu` and an object-level
diff against `build-c94-ftr-bothcpu` (the two differ **only** by
`NDS_TASK91_DRAW_PHASE_CENSUS`, both `NDS_R2_BOTH_CPU=1`):

```
.itcm section  32,448 bytes      ITCM region  0x7fe0 = 32,736
FREE                288 bytes    census needs      +308   (nds_renderer.o .itcm 17,620 -> 17,928)
```

**The census was never the problem; the region is full.** `.itcm` is 99.1%
occupied by 99 text symbols, and *no* timer-based partition of the draw half can
be added without evicting something. This is the durable answer to "why has
`FTR` never been partitioned", and it generalises: **treat ITCM as closed to new
instrument code.**

**The five biggest residents ARE the draw half** — 20,696 bytes, **63.8% of all
ITCM**:

| symbol | bytes |
|---|---:|
| `ndsRendererScanList` | 7,728 |
| `ndsRendererExecuteNativeFighterOwnerProduction` | 3,628 |
| `ndsRendererNativePrepareProductionRun` | 3,348 |
| `ndsRendererSubmitHardwareTriangle` | 3,304 |
| `ndsRendererHardwareSubmitVertex` | 2,688 |

That is a *code-size* ranking, not a time ranking — do not read it as an
attribution of the 331,472. It does say where the hot path lives.

**Honest gap: the arithmetic does not fully reconcile.** Free 288 against +308
predicts a 20-byte overflow; the linker reported **908**. The extra 888 is
**unattributed** — most likely `--gc-sections` retaining symbols the census
newly references, or alignment shifting between contributions, neither of which
is measurable without a successful link. It does not change the conclusion in
either direction: at 288 bytes free the census does not fit even on the
optimistic reading.

**Consequence for the next row — suppression, not timers.** The zero-byte
technique that produced the 86/14 split is the only one immune to this, and it
already has levers in the tree: `NDS_R2_FIGHTER_SHADE_SKIP`,
`NDS_R2_FIGHTER_STATESPAN_SKIP` (`Makefile:570,575`) alongside
`NDS_R2_DRAW_SUPPRESS_MASK`. **Both change what is drawn**, so each is a
*price* in the lab and a `BLOCKED(decision: ...)` with a synchronized
`artifacts/visibility` pair if it ever looks like landing. Rank on the
**clean-frame mean** (`FTR` is 0.99x hot/clean; an excursion ranking scores the
whole 331,472 at ~zero) and report hot/clean beside it.

**Do NOT un-pin ITCM symbols to make room.** Moving hot text changes the cost
being measured, and `linker/nds_hot_text.ld`'s curated list re-addresses itself
when members move (Task 94 note).

### The draw half splits 52/48 — MEASURED by suppression, cycle 96. Shading is already spent.

Gate arm, whole match, 1,600 samples, frames 441–2040, **86.7% coverage, DLDI
ON, exclusion OFF**, `-AllowRepeatedFrames`, `slips=0` on every arm. Ranked on
**clean-frame mean**, because `FTR` is 0.99x hot/clean and an excursion ranking
scores the entire 331,472 at ~zero. Zero instrument bytes: every arm is an
existing default-off lab flag, reverted after.

| arm | build | `FTR` clean mean | hot/clean | fighter triangles | Δ vs control |
|---|---|---:|---:|---:|---:|
| control | `build-c93-flat-bothcpu` | 385,814 | 0.99x | 635,840 / 603,432 | — |
| `NDS_R2_FIGHTER_SHADE_SKIP=1` | `build-c96-shade-bothcpu` | 382,337 | 0.97x | 635,840 / 603,432 | **−3,477** |
| `NDS_R2_FIGHTER_STATESPAN_SKIP=1` | `build-c96-statespan-bothcpu` | 226,434 | 0.98x | **0 / 0** | **−159,380** |
| `NDS_R2_DRAW_SUPPRESS_MASK=3` | `build-c94-ftrsplit-bothcpu` | 54,342 | — | 0 / 0 | −331,472 |

**The draw half decomposes additively, and it closes exactly:**

```
submission + state replay   385,814 - 226,434 = 159,380   48.1% of the draw
pre-submission draw         226,434 -  54,342 = 172,092   51.9% of the draw
                                       sum    = 331,472   == the draw half
```

Both parts non-negative, and the two independently-measured arms reproduce the
`FTR` total to the byte.

**`STATESPAN_SKIP` IS AN OVERLAY, NOT A SUB-OWNER — do not brief it as the
state-span price.** Its `gNdsFighterDLAllDrawP{0,1}HardwareTriangleCount` are
**0/0**: skipping the state spans also stops all geometry submission, so the
159,380 is "state-delta replay **plus everything downstream that no longer
happens**". The boundary between the two halves above is *where triangles stop
being emitted*, not a function boundary. This is the same handling
`mpProcessUpdateMain` gets.

**REFUTED — shading is not a candidate.** −3,477 is **inside the ±5,376
cross-build floor**, i.e. not resolvable from zero. The lever was already spent
by R2-03 E28, which removed the software light preparation
(`NDS_R2_FIGHTER_SOFT_LIGHT_KEEP` defaults 0, so the cut is in); `SHADE_SKIP`
now only removes the residue. Do not re-price it.

**The per-unit constant, which makes short probes valid here:** 1,239,272
fighter triangles over 2,041 frames = **607.2 triangles/frame**, so submission
costs **262 ticks per triangle** (gate arm). For calibration that is already
**2.4x cheaper** than the stage's generic emit (620/triangle, Task 103) and the
effect interpreter (626/command) — **the fighter submit path is not obviously
wasteful**, which argues against attacking it first.

**Where the cheapest mechanically-equivalent routes actually are.** The
pre-submission 172,092 is per-fighter-per-frame *policy* work — walk, reset,
validate, matrix prep, material prep — and it is precisely what charter R2-03
already names for deletion ("no `PrepareProductionRun` policy re-checks, no
traversal-state/stats dependency, no per-frame texture identity proof"). That
is deletion-shaped and needs **no** visible change. The submission 159,380 is
geometry, where the contract's allowed routes are pretransformed geometry,
precomputed matrices, quantized poses and precompiled GX streams — all of which
add text and must beat 1.85 cycles of `FTR` mean per byte.

**Note the framing this creates:** in the `STATESPAN` arm `FTR` clean mean is
**226,434, already under the charter's 250,000 line**. If fighter geometry
submission were free, `FTR` would meet its budget outright — so the 250K line is
reachable without touching the pre-submission half at all, and vice versa.

**Not run: the combined arm.** With `SHADE_SKIP` at or below the noise floor its
overlap with anything else is immaterial, so a third build would not have
changed a number. **A ceiling, not a candidate:** the `STATESPAN` arm's `WORK-H`
P95 is 1,475,328 against the control's 1,650,240 (−174,912) with over-gate
52.4% → 23.0% — that is what removing all fighter geometry buys, on a ROM that
draws no fighters.

### The pre-submission 172,092 — SEAMS NAMED, cycle 97. **No edit made; read the blocker before briefing an implementation.**

The 172,092 is per fighter per frame at 1.94 draws/frame ⇒ **~88,700 per fighter
per frame**. Its five seams, each with the question that decides whether it is a
deletion — *what input can change between frames?* Work whose inputs are fixed
at match load is re-proving a constant.

| seam | site | input that can change | verdict |
|---|---|---|---|
| **walk** | `ndsFighterCollectAllDObjsWithDL` | DObj tree topology | **match-load constant** for Mario/Fox — poses move, topology does not. Bake the collection order at load. Needs proof no status/motion alters the DObj set. |
| **reset** | `ndsFighterDLDrawResetTransientRendererStats` `:5041` | nothing — it clears | **deletion-shaped.** The Task 91 note at `:13474` already records that **70% of this function's memset traffic** is two sites here, clearing the per-list proof/counter prefix *once per part list per fighter per frame*. Those are diagnostic fields; much may already be dead at profile 0 / `NDS_TICK_HUD=0`. |
| **validate** | `ndsRendererAdapterValidateNativeOwnerCached` `:8679` | asset bases, selected count, root offsets | already *named* cached — **but see the blocker.** |
| **matrix prep** | `PrepareNativeOwnerHierarchy` / `…Matrices` | joint matrices, camera | **genuinely varies.** Not a deletion. The contract's routes are precomputed/quantized poses and reduced update rates — those are fidelity-adjacent and owner-gated. |
| **material prep** | `ndsRendererAdapterBuildNativeMaterialSnapshot` `:6895` | damage-flash modulate only | mostly **match-load constant**; 18.8% of scene memset traffic per the same note. Bake per model, patch the modulate per frame. |

**THE BLOCKER, and it is the same lesson the stage taught.**
`ndsRendererAdapterValidateNativeOwnerCached` **carries no engagement counter.**
The stage's equivalent did (`gNdsR2StagePrepareReuseCount` / `…BuildCount`),
and that is precisely how cycle 93 refuted the stage-prepare candidate at
**99.9% reuse for free, with no build**. Here there is no way to tell a hit from
a miss without adding one.

**So the first move is one counter pair, not an edit.** Spending a build
deleting work whose cache already hits would repeat the exact mistake cycle 93
avoided — and on this ROM a wrong guess costs a build, a whole-match run, and a
Boundary run. Add reuse/build counters to the fighter owner validate (and to
the material snapshot), read them on one whole-match gate-arm run, and only then
choose which seam to delete.

**Not done, deliberately:** no edit, no build, no run this row. The chain
(add counters → build → run → design the deletion → dual-route build → two runs
→ Boundary) did not fit the remaining budget, and this row touches the hot draw
path where an unverified commit is the worst outcome. A proven seam with no fix
beats a fix with no proof.

### The counters are in, and they refute two seams and name one (cycle 98)

**Two of the five pre-submission seams are dead, the walk's invariance is proven
for the first time, and the target is material prep at 99.95%.** No edit to the
draw path beyond the counters; no deletion landed this cycle — see the closing
paragraph for why, and what the next one inherits.

Gate arm, both-CPU, whole match, 1,600 samples, frames **442–2041** (the banked
arms landed on 441–2040; the one-frame offset is ordinary window jitter, same
`-StartFrame 441`, same stride, same match config), DLDI ON, exclusion OFF,
`-AllowRepeatedFrames`, `slips=0`. Build
`builds/build-c98-ftrpre-bothcpu`; artifacts
`artifacts/performance/2026-08-05_c98-ftrpre-bothcpu.json`, its `-rows.csv`, and
`...c98-ftrpre-excursion-bothcpu.json`. Coverage is inherited from the banked
arms' identity (same match config, same window) and was **not** re-measured with
`probe-match-window.ps1` this cycle.

| seam | counters | reading | verdict |
|---|---|---:|---|
| **validate** | `gNdsFtrPreValidateReuse` / `Build` / `Reject` | **3,961 / 2 / 0** | **REFUTED — the cache already hits 99.95%** |
| **walk** | `gNdsFtrPreWalkSame` / `Variant` / `First` | **3,961 / 0 / 2** | **CONFIRMED match-load constant — 0 variants** |
| **material** | `gNdsFtrPreMatSame` / `Variant` / `New` / `Evict` | **47,719 / 25 / 27 / 11,659** of 59,430 calls | **THE TARGET — 99.948% of comparable builds re-derive a byte-identical snapshot** |
| **reset** | `gNdsFtrPreResetTransient` / `Runtime` | **0 / 4,625** | **REFUTED — already dead at the shipped profile** |

**Engagement is an identity, not a plausibility.** Validate's three counters sum
to **3,963** and the walk's three sum to **3,963**, and
`gNdsFighterMarioFoxDLAllDrawCount` is **3,963** — so both censuses saw exactly
the draws that happened, no more and no fewer. The material counters sum to
59,430 == `gNdsFtrPreMatCalls`. Fighter triangles 636,480 / 604,044 against the
control's 635,840 / 603,432 (+0.1%) confirm the fighters drew normally under the
instrument.

**Positive control, and it is what makes the two refutations readable.**
`gNdsR2StagePrepareReuseCount` / `BuildCount` read **2,040 / 2 = 99.90%** in the
same run, reproducing cycle 93's stage figure. A counter mechanism that
reproduces a known result is what separates "this seam is already elided" from
"this counter never linked" — which matters most for the reset seam, whose whole
finding is a zero.

- **Validate is refuted for the reason cycle 93 predicted.** Two rebuilds in a
  whole match. Deleting the work behind this cache would have bought nothing,
  and without the counter it was indistinguishable from a seam that rebuilds
  every frame. This is the row's main justification: one run, no build spent on
  a guess.
- **The reset seam was already dead, and the source says why.** Both call sites
  of `ndsFighterDLDrawResetTransientRendererStats` sit in the `detailed_output`
  arm of an if/else (`:9417`/`:14271`), and the native owner production path is
  itself gated on `detailed_output == FALSE` (`:13862`) — so the bzero the
  cycle-97 table called "deletion-shaped" never executes on the gate arm.
  `Runtime` at 4,625 is the negative control. **The 70%-of-memset figure in the
  note at `:13474` is a Results-lab number and does not transfer to the gate
  arm.** Do not re-brief this seam.
- **The walk's inputs never moved once.** The hash covers `total_count`,
  `selected_count`, `selected_index_mask`, and every selected DObj pointer
  *together with the display-list pointer it will be drawn from* — 0 variants
  over 3,961 frame-to-frame comparisons with both CPUs live. That is the proof
  the cycle-97 table asked for ("needs proof no status/motion alters the DObj
  set"). Note what it licenses and what it does not: it says a baked collection
  order would be *correct*, not that the walk is expensive.
- **Material prep is the target.** 47,744 builds could be compared against the
  previous build for the same MObj; 47,719 were byte-identical. The 25 variants
  are real (frac texture animation advances `texture_id_curr/next`), so a design
  that simply freezes the snapshot is a fidelity change and is **not** what this
  points at. What it points at is that `ndsRendererAdapterPrepareNativeMaterials`
  (which walks each MObj chain twice) and `ndsRendererAdapterValidateNativeOwnerMaterials`
  (up to three `ndsRelocFindLoadedFileContaining` searches per material per
  fighter per frame — charter R2-03's "per-frame texture identity proof") are
  re-proving a constant ~999 times in 1,000.

**`Evict` 11,659 is table thrash, not variance — read it that way.** The census
is a 256-entry direct-mapped table keyed on the MObj pointer; a handful of
colliding key pairs ping-pong in one slot and every call for them counts as an
evict. Those 11,659 calls are **unclassified**, not evidence of change. The
classified sample is 47,744, which is what the 99.948% is taken over.
**Actionable for whoever reuses this instrument:** make it 2-way or key on
`(dobj, chain index)` instead, and Evict goes to ~0.

**Instrument cost, and it is not a baseline.** text **+992**, data 0, bss
**+2,112** = **+3,104**; `fake_heap_start` 0x02279424 → **0x0227a044**, the
address delta 0xC20 equalling the section delta to the byte. Proven headroom
111,584 → **108,480** (`check-boot-headroom.ps1` OK, 34.9x margin). The
`NDS_TICK_HUD=0` configuration was link-checked via
`smash64ds-battle-playable-proof-hwtri` and contains **0** `FtrPre` symbols, so
the shipped ROM pays nothing. `FTR` clean mean reads **397,454** (hot/clean
0.98x) against the control's 385,814 — **the instrument costs ~11,640 of the
very bucket it measures**, which is why the seam verdicts above are counter
readings and not tick figures. `WORK-H` P95 **1,671,104** against banked
1,624,064 is +47,040, far outside the ±5,376 floor. **1,624,064 still stands;
this arm is not a new baseline.**

**One unattributed reading.** `gNdsTaskmanArenaChosenSize` 1,351,680 with
`gNdsTaskmanArenaAllocFailCount` **6**. The G2 model predicts ~N/4,096 steps,
i.e. 1 step for 3,104 bytes, not 6. The control's own arena counters were not
read this cycle, so the *delta* is unmeasured and the 6 may largely predate this
build. Nothing failed — the ROM booted and ran the full match — but do not quote
the ~N/4,096 rule as confirmed until a control reading exists.

**Not done, deliberately: no deletion landed.** The seam the numbers chose is
material prep, and its cheapest correct form is genuinely open — a per-frame
memo has to read most of the MObj state the build reads (so it saves the emit
half, not the input half), while a match-load bake has to keep serving the 25
real variants and so still needs a check. `PROJECT_GOAL.md` prefers the bake and
the brief prefers deletion over caching; deciding between them needs a design
step, and landing either needs a dual-route build plus two whole-match runs. That
did not fit behind the run above, and this is the hot draw path, where an
unverified commit is the worst available outcome. **The next cycle inherits a
named seam with a number, an instrument already in the tree to verify against,
and two seams it must not re-brief.**

### The baked draw plan is BUILT AND PROVEN EQUIVALENT, and blocked on a bss clobber (cycle 99)

**Nothing landed. Tree reverted to `b2575f12`; the patch is not in the tree.**
Three results the next cycle must not re-derive.

**1. The material-prep bake is REFUTED at the design step, from source.** The
cycle-97/98 reading that "damage-flash modulate is the only varying material
input" is wrong twice. `ndsRendererAdapterFighterColorModulate`
(`reloc_backend_renderer_dl.c:131`) is **not an input to
`BuildNativeMaterialSnapshot` at all** — it is a separate `color_modulate`
passed to the production-inputs builder. And
`decomp/BattleShip-main/decomp/src/sys/objanim.c:1390-1495`, imported live as
`src/import/battleship_sys_objanim.c:944-956`, writes **every** input the
snapshot reads: `scau scav trau trav scrollu scrollv lfrac primcolor envcolor
blendcolor light1color light2color`. Fighter texture parts additionally write
`texture_id_curr` (`reloc_backend_compat_shims.c:1392/1448`, mirroring
`decomp/.../ft/ftparam.c:1082-1183`). **There is no small enumerable
"varying words" set to patch unconditionally** — the varying set is the whole
snapshot, so an unconditional patch is the build. The 99.948% byte-identical
rate is a property of *this match's animation data*, not of the code, and
freezing on it is a fidelity change, i.e. the owner's call. Do not re-brief
"bake the material snapshot, patch what varies" without solving this first.

**2. What was built instead, and it works: the structural draw plan.** Bake the
collection, the resolved `NDSRelocLoadedFile`, root offsets, material counts,
matrix bindings and material DObjs once per (slot, owner-asset identity);
replay them; delete the per-frame walk, the whole eligibility pass, and
`ndsRendererAdapterValidateNativeOwnerCached`. Keyed on the validator's own
identity fields and additionally cleared from
`ndsRendererAdapterResetSceneCaches` (§3.12).

**Equivalence by construction, whole match, gate arm, frames 442–2041, DLDI ON,
`slips=0`, build `builds/build-c99-plan-bothcpu`:** a `#if NDS_TICK_HUD` mode
derived the plan live on every baked draw and memcmp'd it against the baked one
— **`gNdsFtrPlanVerifyMismatch` 0 over `gNdsFtrPlanVerifyRuns` 3,958**, covering
`material_dobj`, `matrix_dobj`, the resolved file and the root offsets, none of
which any cycle-98 counter covered. Engagement is an identity: hits 3,958 +
misses 5 (`gNdsFtrPreValidateReuse` 3 + `Build` 2) = **3,963 =
`gNdsFighterMarioFoxDLAllDrawCount`**, i.e. the validate ran **5 times in a
match instead of 3,963**. Fighter triangles 636,480 / 604,044 reproduce cycle 98
exactly. Cost: text **+1,008**, data 0, bss **+2,816**; headroom 108,480 →
**104,640** (`check-boot-headroom.ps1` OK); arena `ChosenSize` 1,351,680 →
1,347,584 with `AllocFailCount` 6 → **7**, exactly the one 0x1000 step the G2
model predicts for +3,840. `NDS_TICK_HUD=0` links and carries only
`gNdsFtrPlanRoute` (4 B) and `sNdsFighterDrawPlan` (1,864 B) — no instrument.

**3. RETRACTED, cycle 100 — there is no rogue store and the shipped ROM is not
corrupt.** This row read "something zeroes a fixed `.bss` address … that is a
live memory defect, it is older than this row". It is neither. The poke *lands*
and the guest never sees it: `-SetGlobals` writes main RAM through the GDB stub,
while `gNdsFtrPlanRoute` at `0x0226c560` begins a 32-byte ARM9 D-cache line
shared with `gNdsTickHudVBlankWaitTicks` (`0x0226c564`), which the tick HUD
writes every frame. That line is permanently resident and dirty, so the guest
keeps reading its stale cached 0 and each writeback stamps that 0 back over the
poke. See the cycle-100 section for the three-modality proof. `gNdsFtrPlanRoute`
is now selected at build time and the blocker is closed.

**4. SUPERSEDED, cycle 100 — "same-binary noise is ~9,800" is wrong.** The two
runs quoted here (**1,689,984** and **1,699,776**) are not a repeat pair: 1,586
of their 1,600 rows differ, largest single-frame delta −4,196,672, so they are
two different trajectory alignments rather than jitter on one. Measured
properly, a whole-match run **reproduces bit-identically** under a fixed binary
and invocation. The real floor is cross-build, it is much larger, and it is
calibrated in the cycle-100 section. Neither run is a baseline; **1,624,064
still stands.**

### The baked draw plan is PRICED and KEPT — a P50 lever, not gate progress (cycle 100)

**Default flipped to on** (`NDS_FTR_PLAN_ROUTE ?= 1`). The plan is now selected
at build time, not poked, for the reason in item 3 above.

**Equivalence first, and this time it survives in artifacts.** The cycle-99
claim of `VerifyMismatch` 0 existed only in console scrollback — all three c99
JSONs record `gNdsFtrPlanVerifyRuns` **0**, i.e. no surviving run ever engaged
the route. Re-established on both arms, whole match, 1,600 samples:

| arm | `VerifyRuns` | `VerifyMismatch` | `Hit`+`Build` | draws | fighter triangles |
|---|---:|---:|---:|---:|---|
| both-CPU | 3,960 | **0** | 3,960+3 | 3,963 | 636,480 / 604,044 |
| Boundary | 3,954 | **0** | 3,954+3 | 3,957 | 612,800 / 624,852 |

`Hit + Build == draw count` on both, and triangles are **byte-identical to the
control arm** on both. `gNdsFtrPreValidateReuse` falls **3,961 → 1**: the
owner-validate now runs 3 times a match instead of 3,963.

**The price — three A/B pairs, each pair two layout-identical builds** (both
arms pin the route flag to `.data`, so every pair links at the same
`fake_heap_start` with identical text/data/bss and the only difference is one
initialised word). Whole match, 1,600 samples, frames 442–2041, DLDI ON,
exclusion OFF, `slips=0`:

| pair | arm | `WORK-H` P50 | `WORK-H` P95 | over gate | `named` mean |
|---|---|---:|---:|---:|---:|
| c100b | **Boundary** | **−3,776** | −8,832 | −6 | −6,841 |
| c100 | both-CPU | −5,056 | −2,368 | −38 | −15,208 |
| c100b | both-CPU | **−9,472** | **+5,248** | −31 | −4,916 |

**P50 falls in every pair; P95 changes sign between pairs.** The P95 delta
spans 14,080 across three pairs of the same change, so **P95 is not resolvable
at this magnitude and no gate figure may be banked from this row.** This is the
same shape G1 measured and the board already predicted for `FTR`: it is a P50
lever, anti-correlated with the tail. `FTR` mean −9,415, `FTR` clean mean
−6,485 (c100 both-CPU pair).

Cost: shipped ROM pays `sNdsFighterDrawPlan` **1,864 B** `.bss` plus **8 B**
`.data` (the two route flags, which `.data` pinning keeps out of
`--gc-sections`), and **no instrument** — the `NDS_TICK_HUD=0` link
(`smash64ds-battle-playable-proof-hwtri`) carries exactly those three symbols
and none of the counters. Lab arms link at `0x0227af24`, **104,672 B** proven
headroom; the shipped-config link at `0x02272da4`, **137,824 B**. Arena
`ChosenSize` 1,347,584 / `AllocFailCount` 7 and heap
`gNdsTaskmanGeneralHeapFreeMin` 220,312 are **identical in both arms**, so the
plan adds no runtime memory pressure. Boundary verifier green; root ROMs
unchanged.

**Not done:** no new gate baseline (1,624,064 still stands), and the plan was
not extended past the fighter draw.

### The tail was CARTRIDGE I/O, and the animation arena was full — LANDED, cycle 105. **New gate baseline 1,447,318.**

`f082b3c8`. Whole match, both-CPU, 1,600 samples, frames 441/442–2041, DLDI ON,
exclusion OFF, `slips=0` both arms, `-AllowRepeatedFrames` (3/1600 control,
0/1600 candidate). Builds `builds/build-c105-anim-{ctl,cand}`; artifacts
`artifacts/performance/2026-08-09_c105-anim-{ctl,cand}{,-rows.csv}.json`.

| | control | candidate | delta |
|---|---:|---:|---:|
| `WORK-H` P50 | 1,102,720 | 1,112,576 | +9,856 |
| **`WORK-H` P95** | **1,639,299** | **1,447,318** | **−191,981** |
| `WORK-H` P99 | 2,043,946 | 1,671,012 | −372,934 |
| `SRC` P95 | 848,861 | 661,706 | −187,155 |
| `SINT` P95 | 477,286 | 380,515 | −96,771 |
| `SINT` frames > 400,000 | 113 | 67 | −46 |

**The diagnosis cost no build at all.** `-ExtraGlobals` on the existing tick-HUD
ROM at frames 478 and 2007: `gNdsRelocAssetPayloadReadCount` **+111** against
**113** frames whose `SINT` exceeded 400,000 (median 169,248). The arena read
`ReservedBytes 92,160 / UsedBytes 92,160 / Overflows 142 / Rejects 142` with
`OverflowLastSize` **912** — it was refusing a 912-byte animation with its cursor
at the ceiling, and a refused asset re-reads off the card on **every later use**.
`AUD` and `HUD`, whose work cannot vary with the match, read 4.9× and 2.5× on
those frames: the card stalls the machine, not just the caller.

**The 41-asset warm list was measured on the WRONG ARM.** It came off a Boundary
match; `NDS_R2_BOTH_CPU` puts a level-3 CPU on Mario too, and `gNdsR204AnimSeen`
dumped at presented frame 2038 holds **85** IDs of which the old list covered
**27**. Fixed by replacing the list with the measured 85 (union of both lists is
99 = 229,024 bytes and does not fit), sizing the arena to their 197,184 bytes
(`NDS_R2_ANIM_CACHE_ARENA_BYTES` 92,160 → **200,704**) and raising
`NDS_R2_ANIM_CACHE_ENTRIES` 64 → **128**, since 64 is below 85 and the entry
count is a hard refusal in both store paths.

Engagement exact: hits/misses/rejects **206/147/142 → 351/2/0**, overflows
142 → 0, `WarmLoaded` 39 → 83 (`WarmFailed` 0 both), `ArenaUsedBytes` 191,024 of
200,704. `gNdsR204AnimForceLoadTotal` **353** and `ForceLoadDistinct` **85** on
BOTH arms — identical, so the two runs played the same match and the delta is the
cache rather than a different trajectory. Pixel-identical at the
`time_remain=3000` lock over the whole 400×298 top screen at both captured tics,
max channel delta 0, against an **83.03%** same-build adjacent-tic floor on the
same crop. Boundary verifier passed; root ROMs unchanged.

**The cost is heap, and it is the last helping.** `gNdsTaskmanGeneralHeapFreeMin`
154,776 → **42,136** against the cache's own 32,768 `KEEP_FREE`, so the margin is
**9,368 bytes**. Boot headroom 34,816 proven; taskman arena 1,282,048 →
1,277,952 (one 0x1000 step, exactly the G2 model for +1,888 bytes). A soak
covering the full match, Sudden Death and Results read
`gNdsSyMallocOverflowCount` **0**, `ArenaReserveCount` 3 / `ReserveFailCount` 0,
`GenerationMismatches` 2 / `RangeFaults` 0. **Any future heap taker must free
something first** — and freeing `.bss` is the way, because that lowers
`fake_heap_start` and enlarges the heap the arena callocs from.

P50 rises with the +1,888 bytes, which is what puts 63 more frames over the line
(691 → 754) while P95 falls. The gate is P95.

### The residual tail is the FORCE-LOAD HIT PATH, not I/O — MEASURED, cycle 105

Per-frame probe, `-PerFrameGlobals`, candidate ROM, frames 1100–1129
(`artifacts/performance/2026-08-09_c105-spikeprobe{,-rows.csv}.json`). **Every
`SINT` spike is a frame with `gNdsR204AnimForceLoadTotal` +1 and every other
frame is +0 — 5 of 30, no exceptions in either direction**, with
`gNdsRelocAssetPayloadReadCount` and `…HeaderReadCount` both **+0** on all thirty.

| frame | ΔForceLoad | `SINT` | excess over the ~170,000 baseline |
|---:|---:|---:|---:|
| 1110 | 1 | 739,904 | **+570,000** |
| 1119 | 1 | 286,976 | +117,000 |
| 1120 | 1 | 335,104 | +165,000 |
| 1125 | 1 | 335,488 | +165,000 |
| 1126 | 1 | 295,936 | +126,000 |

So a **cache HIT costs 117,000–570,000 ticks** (~228,600 mean over these five).
That is not the memcpy. The Makefile already names the owner and already carries
its instrument: `NDS_R2_RELOC_FIXUP_TIMING` (`Makefile:307`, R2-06 E8) prices
`ndsRelocFinalizeLoadedFile`'s **five passes** separately, and E8 had already
traced 8 of 9 over-gate frames to the 16 frames that function runs on.

**Worth 121,331 at P95** — capping `SINT` at its own median takes the candidate
1,447,318 → 1,325,987 (over-gate 754 → 517), leaving a gap of 205,607. The
fixups are a pure function of the asset image and the cached image is
byte-identical every time, so the shapes to price are a precomputed fixup offset
list built once at warm time, or a (asset_id, destination) keyed cache of the
*post*-fixup image — the "position-dependent" objection in
`reloc_backend_assets.c` is about replaying into a DIFFERENT heap, and R2-04 E0
already recorded that the destination is caller-owned and reused.

### The force-load frame is PARTLY ATTRIBUTED (25.1%), and the rest needs a design not a micro-fix (cycle 106)

Two instrument builds, both flags the Makefile already carried for this exact
question. `builds/build-c106-loadsplit`
(`NDS_R2_LOADFRAME_TIMING=1 NDS_R2_RELOC_FIXUP_TIMING=1`), whole match, gate arm,
frames 443–2042; artifact `artifacts/performance/2026-08-09_c106-loadsplit.json`.
Denominator: total `SINT` above its own median across the 1,600 frames =
**63,115,584**.

| owner | ticks/match | % of the `SINT` excess |
|---|---:|---:|
| `ndsRelocFinalizeLoadedFile` **AObj16 pass** | 10,236,800 | **16.2%** |
| — of which Normalize / Swap / Successor | 4,168,960 / 3,250,752 / 1,704,320 | 6.6 / 5.2 / 2.7 |
| `gcAddDObjAnimJoint` (6,459 calls) | 3,422,272 | 5.4% |
| Fixup Internal + External + Attributes | 1,122,176 | 1.8% |
| `gcAddAnimJointAll` (55 calls) | 1,075,008 | 1.7% |
| **accounted** | **15,856,256** | **25.1%** |

E8's split reproduces: relocation 18.0% here against its 21.5%. **But the add
wrappers capture only 7.1%, so "the action change is ~78%" is true and
`gcAdd*AnimJoint*` is NOT where it lives** — 74.9% (47.3M ticks, ~134,000 per
force load) is still unattributed. `FixupSpritesTicks` 22,448,768 against
`FinalizeMaxTicks` 21,776,704 is ONE call at scene load, outside the window;
exclude it from any in-match figure.

**The whole-frame profile, gate arm, is banked** —
`artifacts/performance/2026-08-09_c106-profile/` (`census.{txt,json}`,
`arm9-profile.csv`), 400 frames from 441, per-frame regions, 1.21G cycles,
14.4M PCs, 1,205 of 3,545 FUNC symbols hit. Read section A (total cycles) as the
current cost ranking; **read section E's over-gate split with care on this ROM**
— the profiler build is slow enough that the 2-VBlank threshold marks 307 of 400
frames, so `armWaitForIrq` takes 58.9% of the "premium" and that is quantised
idle, not work. Instrument rows to discount in E: `ndsPlatformRenderDebugHud`
+40,955 plus the printf family (`_svfiprintf_r`, `_vfiprintf_r`,
`__ssvfiscanf_r`, `consolePrintChar`, `__utf8_mbtowc`) ≈ 72,700/frame together —
that is the tick HUD's own text and the shipped ROM pays none of it.

**Three levers this profile pointed at were ALREADY TRIED AND DOCUMENTED. Read
the owning file before building any of them again.**

- **`.text.hot` placement.** Section C ranks `ndsR2CubicValueFixed` (2,032 B) the
  #1 unplaced candidate for the 3,936 free bytes, exactly as it did for R2-03
  E66 — which admitted it and measured **`WORK-H` P95 +24,448**, P90 +8,000,
  over-gate 7 → 8. `linker/nds_hot_text.ld:179-201` carries that and the Task 94
  regression on the same list. **`.text.hot` is closed in both directions, and
  census sections C/D are a cost ranking, never a placement prediction.**
- **Hoisting the animation range check in `ndsRelocAssetIDForToken`.** R2-06 E11
  did it, with negative bytes added: the function fell 39,475 → 31,808 per load
  frame and **still lost** — `WORK-H` P95 +15,744, P99 +59,200, over-gate 9 → 11.
  `reloc_backend_assets.c:1840-1895` carries the full reasoning.
- **The `ndsAObjEvent32Normalized` linear scan.** Looks O(n²) — a 1,024-entry
  scan called from inside the command loop — but measured, the whole match makes
  **448** `NormalizeScript` calls (125 new + 323 reuse) over **973** commands, so
  it is ~1,375 ticks/frame and cannot be the spike.

**But that scan has a LATENT CORRECTNESS CLIFF worth its own row.**
`sNdsAObjEvent32NormalizedCount` reads **973 of `NDS_AOBJ_EVENT32_NORMALIZED_MAX`
1,024** at the end of a one-minute match, with `NormalizeFailCount` 0 — a
**51-entry margin**. Overflow takes the `ndsAObjEvent32Reject(12u, …)` path,
which returns FALSE, and the caller then **skips `ndsBaseGcAddDObjAnimJoint`
entirely** — i.e. the animation silently does not attach. A longer match, a
rematch, or a wider moveset would hit it. It is a table bound, not a heap bound,
so raising it costs 8 bytes an entry in `.bss`.

**What the next row must be, and the size it has to clear.** E11's own conclusion
is the standing rule now: *"a load-frame-only saving of ~8,000 ticks cannot be
banked through P95 here, because relinking moves the tail by more than the
saving. Either remove this work in one change large enough to clear ~16,000 of
tail movement, or move it off the gameplay frame entirely."* Cycle 105's arena
fix is what that looks like when it works — it moved the I/O off the frame rather
than making it faster. The equivalent for the remaining half is the **second**
repair E8 named: one **pre-finalized, resident** copy per warmed animation, so
the force load returns a pointer instead of memcpy + fixups + swap + normalize.
Design blockers to answer first, in order: the fixups write absolute pointers
derived from `loaded->data` (so a shared copy pins the destination), normalization
writes `command->u` **in place** (so the resident image is mutated — establish
whether anything writes the script after that), and the destination is
caller-owned through `lbRelocGetForceExternHeapFile(file_id, heap)`.
**RAM is the hard constraint: 85 × ~2,319 = 197,184 more bytes do not exist**
(heap free-min 42,136), so this has to REPLACE the per-load destination copy, not
sit beside it.

### The load frame is FULLY ATTRIBUTED, from the CSV that already existed (cycle 107)

`task37_census.py --split-by-symbol ndsRelocFinalizeLoadedFile` over the cycle-106
profile — **no build and no emulator run**, because `--split-by-symbol` partitions
the existing per-frame regions by whether a frame executed that symbol, and that
symbol executes on exactly the load frames. **74 marked frames vs 326 control,
premium 650,610 cycles/frame.** This supersedes the cycle-106 over-gate split,
whose threshold marked 307 of 400 frames and therefore mostly measured idle.

| symbol | +cyc/frame | %prem | |
|---|---:|---:|---|
| `armWaitForIrq` | 137,472 | 21.1 | quantised idle, not work |
| **`ndsRelocFinalizeLoadedFile`** | 65,335 | 10.0 | fixups |
| **`battleship_ftAnimParseDObjFigatree`** | 50,923 | 7.8 | real: new animation evaluated |
| **`ndsRelocAssetIDForToken.part.0`** | 43,875 | 6.7 | pure function, 110-branch chain |
| `ndsR2CubicValueFixed` | 22,112 | 3.4 | real |
| `memcpy` | 21,168 | 3.3 | payload copy |
| `gcPlayDObjAnimJoint` / `__aeabi_fadd` / `__aeabi_fmul` | 17,839 / 18,657 / 15,855 | 8.0 | real |
| `battleship_ftMainSetStatus` | 13,737 | 2.1 | real |
| `gcAddDObjAnimJoint` / `ndsFTParamsInvalidateFighterParts` / `armCopyMem32` | 9,757 / 9,513 / 9,263 | 4.4 | |
| alias churn (`Remove…StatusAliases`, `FindLoadedFileContaining`, `Remove…LoadedAliases`) | 5,395 / 4,688 / 3,528 | 2.0 | |

**Two blocks of almost equal size, and only one of them is recoverable.** The
reloc + copy family is **153,252/frame (23.6%)** and is pure port-side overhead
delivering bytes that are already in RAM; the animation re-evaluation is
**158,393/frame (24.3%)** and is real gameplay work — the fighter genuinely
changed animation and the new one is being evaluated. Do not brief the second as
waste.

### The token memo is REFUTED — the function is pure, the KEY is not stable (cycle 107)

`ndsRelocAssetIDForToken` looked like the ideal target: 6.7% of the load-frame
premium, **3,246,729 of its 3,246,729 cycles on load frames**, and a provably
pure function — every branch compares against an `ll*FileID` link-time address or
an integer literal, the two tail scans walk `static const * const` arrays of the
same addresses, and `ndsRelocIsMarioFoxAnimID` is range arithmetic. A memo needs
no invalidation at any scene boundary. **It still does not work.** Three builds,
each verified with a route-2 arm that consults the memo and then runs the chain
anyway and compares:

| config | hits | misses | evicted / declined | occupancy | hit rate | `VerifyFail` |
|---|---:|---:|---:|---:|---:|---:|
| 64 entries, evicting | 9,144 | 12,746 | 12,682 | — | 41.8% | **0** |
| 512 entries, evicting | 10,973 | 10,917 | 10,405 | — | 50.1% | **0** |
| 512 entries, **no-evict** | 4,257 | 17,633 | 17,121 | **512 of 512** | 19.4% | **0** |

`VerifyFail` 0 in all three over 24,374 verified hits, so purity is confirmed
dynamically and is not the problem. **Eight times the table moved the hit rate
41.8% → 50.1%, and no-eviction made it worse by filling every one of 512 slots
with keys that never repeat** — the population is large and mostly non-repeating,
which is the refuted `(dl-pointer, bind-ordinal)` `Tex` memo shape exactly.

**And R2-06 E11's own data already said why, which is the part worth keeping:
the 74.3% of calls that resolve inside the compare chain are the CHEAP ones, and
the cost is the 14.3% that miss and walk both 143 + 158-entry scans.** Those are
precisely the unstable-key calls a memo cannot hold. So the two facts compose:
the calls worth caching are the ones that cannot be cached.

Reverted; the patch is not in the tree. **Do not re-attempt a token-keyed cache
here.** What is still open is narrowing the *scan*, not caching its result — the
two arrays' contents are link-time constants, so an `[min,max]` bound computed
once at init would reject an out-of-range token in two compares instead of 301
iterations, and it would bite on exactly the 14.3% that are expensive. That is
unmeasured and it must clear E11's ~16,000 tail-movement bar to be bankable.

**Method note worth more than the result: read Evicts, not Hits.** A memo whose
misses are nearly all evictions is undersized or mis-keyed, and its hit rate says
nothing about which. Both numbers were in the instrument from the first build,
which is why this cost three builds instead of shipping a 4.6 KB regression.

### The AObj16 pass is PREBAKED at warm time — LANDED, cycle 108. ~−23,000 P95.

The first thing cycle 107's attribution said to move rather than speed up.
`ndsRelocNormalizeFighterAObj16File` (table-bytes discovery, u16 lane swap over
the script region, O(n²) successor scan, per-script normalize) ran on every
force load. It is **position-independent** — it never touches the pointer table
and every quantity it writes is an offset — so the transformed image survives
the per-destination copy and the pass can run **once per warmed animation**
instead of once per load.

The obstacle was the internal fixup list, which is *intrusive*: each raw slot
word is `(next_slot_index << 16) | target_word_index`, so applying the fixups
consumes the list, and the pass needs them applied. `ndsR2AnimPrebakeAObj16`
therefore records every `(offset, raw word)` while walking the list, applies,
transforms, then writes the raw words back. The restore is exact by
construction and independent of where the slots sit — deliberately stronger
than restoring a table region and assuming no slot lives past it. Every failure
path restores and declines with `aobj16_ready` 0, so a decline is a performance
outcome and never a correctness one.

**Engagement, whole match, both-CPU:** `PrebakeReady` 85 of 85 distinct,
`Skips` 351 of 351 cache hits, `SlotsMax` 21, all four decline counters 0,
`ExternalFixupFailCount` 0, `RelocPointerFixupFailCount` 0,
`NormalizeFailCount` 0. Trajectory unchanged: `ForceLoadTotal` 353, `Distinct`
85, `Hits` 351, `Misses` 2. `gNdsTaskmanGeneralHeapFreeMin` 42,136 — unchanged,
because the scratch is 64 slots against a measured max of 21.

**This row exists because the first two arms disagreed, and that is the lesson.**
Two separately-linked builds of the *same* change, differing only by 3,584 bytes
of scratch, read `WORK-H` P50 1,093,152 and 1,119,136 — **25,760 apart, 4.5× the
cross-build P50 floor** — while the second did strictly *more* work (351 skips
against 259) and used *less* RAM. Their P95 read −39,702 and −22,806 against the
c105 control. Relinking moved the body further than the change did, exactly as
R2-06 E11 says it will.

Settled on **one binary** with the runtime route `gNdsR2AObj16PrebakeRoute`
(`.data`, default 1, poked by `-SetGlobals` at the first frame-complete marker),
which is what standing rule 7 and the sampler's own header ask for:

| `build-c108-route`, one ROM | P50 | P95 | mean | over gate | SINT P95 | SRC P95 |
|---|---:|---:|---:|---:|---:|---:|
| route 0 | 1,110,400 | 1,431,696 | 1,136,266 | 741 | 360,195 | 653,632 |
| route 1 | 1,110,144 | **1,416,003** | 1,141,129 | 739 | 346,352 | 642,515 |
| delta | −256 | **−15,693** | +4,863 | −2 | −13,843 | −11,117 |

The route-0 arm is a *partial* control, and its counters say so: three entries
warm before the poke lands (warm stepping runs one asset per scene update) and
they are the hottest, so 111 of 351 hits already skip. The delta therefore
covers **68.4%** of the change → **~−23,000 whole**, which is what the
separately-linked arm read (−22,806). **The −39,702 was placement, not work.**
P50 −256 and mean +4,863 are the confirmation that the change touches only load
frames, as designed.

**Pixel-identical**, and with the floor beside it as `capture-cut-g-exact-frames`
requires: control `2026-08-09_c105-anim-cand-t3000-a.png` vs
`2026-08-09_c108-prebake-t3000-a.png` at the same tic differ on **0 of 119,200
pixels, max channel delta 0**, against a same-build adjacent-tic floor of
**83.03%** (98,968 pixels, identical in both builds). Boundary verification
profile passed. Promoted to both Makefile config blocks.

**Keep the route flag.** It is four bytes in `.data` and it makes the next A/B on
this seam free. Every future arm on a placement-sensitive seam should be one
binary with a route before it is two builds — this cycle spent four measuring
runs re-learning that.

**Worth ~7% of the 326,938 gap.** Banked and moved past; do not polish it.

### `ndsFTParamsInvalidateFighterParts` — the shaped target, and a dead pool (cycle 108)

The census's best-shaped candidate, traced to its mechanism.
`reloc_backend_compat_shims.c:1474` is a **recursive walk of the joint tree whose
entire job is invalidation**:

```c
parts = joint->user_data.p;                     /* 37.1 cyc/ex -- miss */
if (parts != NULL) { ...; parts->unk_dobjtrans_word = 0; }
for (child = joint->child; child != NULL; child = child->sib_next)
    ndsFTParamsInvalidateFighterParts(child, reset_mode);
```

It writes **one zero per part** and pays a pointer chase to reach each one:
79,874 executions of each of its two hot loads, at **37.1 and 30.8 cycles**, for
**4,946,580 of its 6,529,067 load excess**, at CPI **7.08**. The comment already
above it says the diagnosis was known — *"a recursive walk down the joint tree
whose every hot PC is a pointer-chasing load. The loads keep their cost wherever
this lives; what ITCM buys is the fetch of the loop around them"* — so the ITCM
pin it already carries was applied knowing it could not fix the loads. **Only a
layout change can.**

**And the layout change was already written, then left dead.**
`reloc_backend_compat_shims.c:494-496` declares

```c
static FTParts sNdsFTManagerPartsAllocPool[64];
static FTParts *sNdsFTManagerPartsAllocFree;
static sb32 sNdsFTManagerPartsAllocInit;
```

with an initialiser at `:499` that threads them into a free list — and **the
compiler reports all three as "defined but not used" on every build.** The pool
allocator is unreachable, so `FTParts` come from scattered heap allocations
instead, which is exactly why the walk misses on every node.

#### Both premises above are REFUTED (cycle 109) — the pool is NOT the fix

**1. The root-joint precondition fails, on paths that run every match.**
`nFTPartsJointTopN` is `0` (`ftdef.h:1071-1078`; `TransN` 1, `XRotN` 2, `YRotN`
3, `CommonStart` 4), so every `joints[4]` caller is passing a sub-joint:
`ftcommondamage.c:210` (damage), `ftfoxspecialhi.c:143` (Fox up-B),
`ftnessspecialhi.c:519`, `ftpikachuspecialhi.c:159`. `ftcommonguard1.c:256`
passes `joints[YRotN]` and `:358` `joints[XRotN]` — shielding, both fighters,
constantly. And `ftparam.c:2637-2638` invalidates two IK children by pointer
inside `func_ovl2_800EBD08`. A flat whole-fighter sweep would invalidate strictly
more than the original, so **the subtree structure is load-bearing** and cannot
be dropped.

**2. The two expensive loads are `DObj` fields, so a contiguous `FTParts` pool
cannot touch them.** Disassembling the ITCM copy and joining per-PC:

| pc | instruction | field | execs | cycles | cyc/ex |
|---|---|---|---:|---:|---:|
| `1fff274` | `ldr r4,[r0,#16]` | `joint->child` | 79,874 | 2,964,305 | **37.1** |
| `1fff266` | `ldr r3,[r0,#132]` | `joint->user_data.p` | 79,874 | 2,461,519 | **30.8** |
| `1fff282` | `ldr r4,[r4,#8]` | `child->sib_next` | 76,430 | 878,273 | 11.5 |
| `1fff272` | `str r2,[r3,#4]` | `parts->unk_dobjtrans_word` | 79,874 | 1,365,294 | 17.1 |
| `1fff28a` | `ldr r2,[r3,#0]` | `parts->transform_update_mode` | 41,135 | 1,056,909 | 25.7 |

**DObj traversal is 6,304,097 cycles; everything `FTParts` is 2,444,875.** The
pool addresses only the smaller half, and offsets 16 and 132 are 116 bytes apart
— two cache lines *per node*, in a struct whose layout is mirrored from decomp
and therefore not ours to repack (`check-decomp-header-mirror.py`).

**3. The whole function is too small to matter.** 13,718,726 cycles = **1.40% of
non-idle**. Deleting it entirely is worth ~15,500 ticks/frame at P50; the real
fix is worth ~6,560. Recursion accounts for 76,430 of the 79,874 entries, so
there are only **3,444 external calls walking ~23 joints each** (~11.2 calls per
frame, two fighters).

**The fix that would work, if it is ever worth a build:** flatten the joint tree
into a DFS-preorder `FTParts*` array once at build time, and give each joint its
subtree's `[start, count)` — a subtree is contiguous in preorder, so *any* joint
(not just a root) invalidates as a linear sweep, and precondition 1 stops
mattering. That removes both DObj loads and the `sib_next` chase: ≈5.8M, **0.59%
of non-idle ≈ 6,560 ticks/frame at P50**. Above the P50 floor (~5,700), below
the P95 floor (14,080), and **1/6 of the animation lane below**. Queue it behind
that; do not spend a build on it alone.

**The dead pool may now be deleted as ordinary cleanup** — the earlier "do not
delete, it is the intended fix" note is withdrawn. It addresses 2.4M of the 8.7M
and needs a flattening pass to be reachable at all.

### The fighter animation lane is 8.85% of non-idle — the largest lever found

Priced off the cycle-106 whole-match profile (no build, no emulator run), self
cycles from `census.json` and soft-float charged back with
`analyze-leaf-helper-attribution.py`:

| symbol | self | soft float | total | % non-idle |
|---|---:|---:|---:|---:|
| `battleship_ftAnimParseDObjFigatree` | 16,192,916 | 7,931,155 | **24,124,071** | 2.47% |
| `gcPlayDObjAnimJoint` | 16,595,669 | 6,706,718 | **23,302,387** | 2.38% |
| `ndsR2CubicValueFixed` | 19,420,815 | 2,542,351 | **21,963,166** | 2.24% |
| `gcPlayAnimAll` | 7,085,886 | 130,690 | 7,216,576 | 0.74% |
| `ftParamUpdateAnimKeys` | 5,291,274 | — | 5,291,274 | 0.54% |
| `ndsBaseGcPlayDObjAnimJoint` | 2,827,175 | 1,912,301 | 4,739,476 | 0.48% |
| **lane** | 67,413,735 | 19,223,215 | **86,636,950** | **8.85%** |

At `WORK-H` P50 1,107,008 that is **~98,000 ticks/frame**. The parser and the
joint evaluator are **the #1 and #2 soft-float callers in the entire build**,
ahead of collision, matrices and particles.

**Three findings decide what to build.**

**(a) The fixed cubic is not the problem — it is the most efficient thing in the
lane.** `ndsR2CubicValueFixed` runs at **CPI 1.74** against a build average of
2.85: compute-bound, already good. But it costs **320 cycles and 184
instructions per call** over 60,582 calls, of which the 10-register
`push`/`pop` pair alone is **1,707,080 cycles (8.8% of the function)**. It has no
hot site — the cost is flat, i.e. the *conversions and the call*, not the
Hermite. Making it "more fixed-point" is finished work.

**(b) `AObj` is 3.2x the D-cache, and the profile shows it missing on every
node.** The struct is 36 bytes (`objtypes.h:124-136`: `next`, `track` @4, `kind`
@5, six `f32`, `interpolate` @32). The hottest instruction in
`gcPlayDObjAnimJoint` is `ldrb r5,[r4,#5]` — `aobj->kind` — at **24.1 cyc/ex
over 143,916 executions = 3,465,773 cycles, 20.9% of the function**; with
`ldr r4,[r4,#0]` (`aobj->next`, 7.0 cyc/ex) the bare list walk is **4,470,121
cycles, 26.9%**. 143,916 visits over 42,210 calls = **3.41 AObj per joint**. At
~360 live nodes the working set is **12,960 bytes against a 4KB D-cache — 3.2x,
so it can never stay resident.** A 12-byte track brings it to 1.05x; **8 bytes
fits.** This is the strongest single argument in the lane and it is a layout
argument, not an arithmetic one.

**(c) The parser is the biggest item and the only one AOT deletes outright.**
`ftAnimParseDObjFigatree`'s top three loads are `ldr r4,[r0,#116]` at **33.1
cyc/ex**, `ldrb r3,[r2,#4]` at **29.6** (the bytecode stream, byte at a time) and
`ldr r3,[r3,#4]` at **25.6** — 3,125,707 cycles, 19.3% of the function. It is
re-interpreting a stream that never changes, which is the textbook
compute-once case in `PROJECT_GOAL.md`.

**Sizing.** Removing the parser interpretation (~60% of 24.1M), the AObj chase
(~3.5M), the conversion boundary in the evaluator (~2.2M soft float + ~7.8M
self) lands ≈**34M = 3.5% of non-idle ≈ 38,700 ticks/frame at P50**; carrying
fixed point through to the matrices reaches ≈60,000. Against the ~304,000 gap
that is 13–20% in one campaign — **6x the flattened-invalidate fix, and above
every noise floor.**

**Constraints any implementation must respect.**

- **It must not grow RAM.** Static headroom proven is **34,816 bytes** and
  +14KB of `.bss` once stopped the ROM booting. AOT tables must ship as **files
  through the existing anim cache arena** (`NDS_R2_ANIM_CACHE_ARENA_BYTES`
  200,704, `KEEP_FREE` 32,768) in the slot the figatrees already occupy — not as
  linked-in arrays. Same bytes, better content.
- **It must replace, not coexist.** 1.85 cycles of `FTR` mean per byte of added
  ARM text; a runtime selector between old and new evaluator pays for itself
  twice and wins nothing.
- **Derive the phase, do not accumulate it.** `t = length * length_invert`
  recomputes from scratch each frame, so its error is bounded per frame; a
  `phase += phase_step` accumulator **drifts over a long animation**, and
  animation drives hitbox positions and therefore knockback. Keep an integer
  frame counter and compute `phase = (frame * step) >> k`. This is the one part
  of the sketched design that is not equivalent-by-construction.
- **`length_invert` has readers outside the evaluator** —
  `reloc_backend_mp_collision.c:11918` writes it, and
  `battleship_sys_objanim.c` reads it at `:214`, `:292`, `:921`, `:942`
  (including two `length_invert <= length` runtime-float compares). Deleting the
  field is a wider change than deleting its use in the cubic.
- **Fighter path only.** `ndsBaseGcPlayMObjMatAnim` is a separate 4,463,648-cycle
  soft-float caller on the same `AObj` infrastructure; material, camera and
  stage animation must keep working unchanged.

**Do the free `SINT`/`SPHD`/`SHDT`/`SCPU` split first** — it costs no build and
those three are ~79.7K of over-gate discriminator, which can reorder this queue.

### Over-gate split: animation is the largest REAL discriminator (cycle 109)

Ran the free over-gate split off the existing census. **The raw ranking's #1 row
is the measuring instrument** — `ndsPlatformRenderDebugHud` 40,955 plus
`_svfiprintf_r`, `_vfiprintf_r`, `__ssvfiscanf_r`, `consolePrintChar`,
`__utf8_mbtowc` = **72,733 of 437,886, i.e. 16.6%**. `WORK-H` already subtracts
it, but anyone reading the census split directly will rank the tick HUD first.
Real discrimination is **365,153 cycles/region**:

| class | delta/region | % real |
|---|---:|---:|
| **animation lane** (incl. soft-float share) | **72,638** | **19.9%** |
| asset load | 51,789 | 14.2% |
| effects/renderer | 21,489 | 5.9% |
| `ndsFTParamsInvalidateFighterParts` | 6,563 | 1.8% |
| collision | 5,134 | 1.4% |

Animation is the largest class; asset load is second and partly taken already by
cycle 108's prebake, which postdates this profile. `SPHD`/`SHDT`/`SCPU` do not
appear as distinct symbol classes — their bucket deltas spread across collision
and soft float, neither competitive. **The split confirms the queue rather than
reordering it.** Independent agreement worth noting, not proof: the invalidate
walk reads 6,563 here against the 6,560/frame derived from its cycle share.

### Parser AOT slice: the format is compilable, but the win is not where it looked

Traced the whole `event16` stream. It is a u16 word stream —
`opcode:5, flags:10, toggle:1`, then an optional u16 duration, then one s16 per
set flag bit (`AObjAnimAdvance` is `p++`). Two things follow.

**The stream is already the compact fixed-point representation.**
`ftAnimGetTargetValue` just multiplies the s16 by a power-of-two frac
(`1/512` rotation, `1/4` translation, `1/4096` scale, and two non-power-of-two
`1/16384 - 3e-12` entries for `TraI`). Compiling to f32 records would make the
stream **2x bigger** — 20 bytes against 10 for a 3-track command — which on a
memory-bound ARM9 is a likely net loss. **The win is the conversion boundary and
the AObj layout, not a new file format.** Priced from the profile, the parser's
7,931,031 cycles of soft float are: `fcmpeq` 921,383 · `fsub` 1,753,743 ·
`i2f` 796,253 · `fadd` 1,454,187 · `fmul` 920,213 · `fcmpgt` 385,779 ·
`fcmple` 204,854 · `fdiv` **1,494,619 at 109.4 cycles a call**, the most
expensive helper in the build by 3x, on `1.0F / payload` where **payload is a
u16 frame count** — i.e. a reciprocal table hits every time.

**The parser's #2 hot load is the same AObj walk as the evaluator's #1.**
`ldrb r2,[r2,#4]` is `aobj->track` in the `track_aobjs[]` gather (29.6 cyc/ex),
mirroring `ldrb r4,[r4,#5]` = `aobj->kind` (24.1). **~6.2M of pointer chasing
across the two functions, one root cause.**

**Root cause found and first piece landed.** `gcSetupObjman` threads
`setup->aobjs[0..n-1]` into `sGCAnimHead` in ascending address order
(`objman.c:2462-2475`) — and **`aobjs_num` is zero in every scene**; `rg` over
`src/`, `sc/` and `vs/` finds no writer anywhere. So `sGCAnimHead` starts NULL
and every AObj in the game is an individual 36-byte `syTaskmanMalloc`
(`objman.c:640-645`) interleaved with everything else the scene allocates.
`battleship_sys_objman.c` now fills that seam with one contiguous block —
the same pooling the file already documents for GObj thread stacks, re-carved
per setup because the arena resets between scenes, declining safely to today's
behavior if the arena is tight. **No struct, format or arithmetic changes:
allocation locality only, so behavior is bit-identical.** Built green,
`check-boot-headroom` 33,216 proven.

**It cannot be measured alone, and neither can any other single piece.**
Estimated ~2,800 ticks/frame against a cross-build P95 floor of 14,080; and
**standing rule 7's runtime route does not apply here** — the seam runs at scene
setup, before the first frame-complete marker where `-SetGlobals` pokes, so
there is no one-binary A/B for it. Every piece of this lane is 1,500–7,800
individually. **Bundle them and measure once**, per the "clear ~16,000 in one
change" rule: AObj pool (~2,800) · comparisons through the proven
`nds_fcmp.h` (~1,500 parser + evaluator share) · `ftAnimGetTargetValue` by
integer bit assembly, exact for the six power-of-two tracks and needing
`target("arm")` because Thumb-1 has no `CLZ` (~1,240) · reciprocal table for
`1.0F/payload` (~1,430) · the evaluator's conversion boundary and inlining its
10-register `push`/`pop` (~2,000–7,800). Verify the AObj sites with
`analyze-dcache-stalls.py`, not with ticks — the mechanism check is immune to
the placement floor.

### SINT decomposed: it IS the animation lane (cycle 109)

`SRC_CPI_OPTIMIZATION.md` called this "the big one" — *which child of
`ftMainProcUpdateInterrupt` causes the +88,082?* Answered with
`scripts/analyze-subtree-attribution.py` (new, no build, no run: static call
graph from the disassembly, subtree per direct child, cycles and over-gate delta
from `census.json`). **Two children carry all of it:**

| direct child (exclusive subtree) | cycles | % non-idle | over-gate | syms |
|---|---:|---:|---:|---:|
| **`ftMainPlayAnim`** | **73,169,415** | **7.48%** | **+60,559** | 8 |
| `ftComputerProcessAll` | 49,550,399 | 5.06% | +24,386 | 71 |
| everything else | 2,432,810 | 0.25% | +2,131 | 29 |

**+84,945 of the +88,082.** And `ftMainPlayAnim`'s eight exclusive symbols are
exactly the animation lane — `ndsR2CubicValueFixed` 19.4M ·
`gcPlayDObjAnimJoint` 16.6M · `ftAnimParseDObjFigatree` 16.2M ·
`ndsFTParamsInvalidateFighterParts` 13.7M · `ftParamUpdateAnimKeys` 5.3M ·
`gcPlayMObjMatAnim` 1.1M · `ftMainPlayAnim` 0.5M ·
`ftParamsUpdateFighterPartsTransform` 0.4M.

**So the plan's steps 1, 2 and 3 collapse into one target.** Step 1 (the FTParts
pool) is refuted above *and* is only 6,563 of the child's 60,559. Step 2 says go
to step 3. `ftComputerProcessAll`, the other child, is **not AI logic** — it is
map collision: `mpCollisionGetFCCommonFloor` 7.5M,
`ndsStageMPSweepFloorLoopSweep` 6.4M, `ndsMPFindLineEndpoints` 5.4M,
`ndsStageMPAdjustFloorLoopWallSweep` 4.1M. A separate lever, correctly ranked
second.

**Do not quote a subtree number for `SPHD` or `SHDT`.** Their roots each have one
child that statically reaches 511 and 186 symbols, so the tool's own
"reachability is not execution" limit dominates and the percentages are
meaningless there. SINT's exclusive sets are small (8 and 71) and clean.

### Animation lane: what each remaining cut is actually worth

Priced per call site from the profile, with the corrected per-helper costs
(`fadd` 36.43, `fmul` 25.17, `i2f` 16.80, `fdiv` **109.38**, `fcmpeq` 10.59):

| cut | cycles | ticks/frame | status |
|---|---:|---:|---|
| `aobj->length += anim_speed` — 111,168 `fadd` | **4,049,850** | ~4,600 | **needs the representation change** |
| parser `fsub`+`fadd` clock arithmetic | 3,207,930 | ~3,640 | same |
| parser `fdiv` on `1.0F/payload` | 1,494,619 | ~1,700 | reciprocal table, payload is a u16 |
| cubic `length * length_invert` `fmul` | 1,524,849 | ~1,730 | fuse into the fixed conversion |
| parser `ftAnimGetTargetValue` i2f+fmul | 1,534,213 | ~1,740 | CLZ bit assembly, needs `target("arm")` |
| AObj list scatter (both functions) | ~6,200,000 | ~2,800 | **DONE** — pool, cycle 109 |
| evaluator `fcmpeq` ×180,454 | 1,911,008 | ~2,170 | **DONE** — `nds_fcmp.h` |
| cubic `(f32)q` i2f | 1,017,778 | ~1,160 | **DONE** — cycle 109 |

**No single remaining cut clears the 14,080 cross-build P95 floor**, which is why
they ship as one bundle and get measured once. The largest, at 4.6K, is one
line: `aobj->length += dobj->anim_speed`, executed for every active node every
frame.

**That line is why the representation change is unavoidable.** `length` is
consumed only as `length * length_invert` (cubic), `length * rate_base`
(linear), and `length_invert <= length` (step) — every consumer immediately
converts it or multiplies it. As Q16 it would make all three cheaper *and* turn
the `+=` into an integer add. But `AObj` is shared with material, camera and
stage animation, its layout is decomp-mirrored
(`check-decomp-header-mirror.py`), and `length` has consumers in
`gcParseDObjAnimJoint`, `gcGetDObjTempAnimTimeMax`,
`gcGetAObjTrackAnimTimeMax` and `gcCheckGetDObjNoAxisTrack`. **This confirms
`FIXEDPOINT_ANIMATION.md`'s own sequencing: the compact per-fighter track array
that replaces `AObj` is the change, and it cannot be a quick edit.**

**Landed this cycle in the kernel:** `ndsR2S32ToF32Bits` replaces the last
`__aeabi_i2f` in `ndsR2CubicValueFixed` with CLZ bit assembly. Bit-exact to
`__aeabi_i2f` including round-to-nearest-even above 2^24, **proven over all
4,294,967,296 inputs** by `scripts/check_s32tof32_exact.py` (no NaN exclusion to
make — every s32 maps to a finite float). `check_r2_cubic_error_bound.py` is
unchanged and green at 0.002842 rotation / 0.006702 translation, which is what a
bit-exact change must do. Disassembly confirms the call is gone and one ARM
`clz` replaced it; the two `__clzsi2` references in the ELF are inside libgcc's
`__clzdi2` and are present in the pre-change build too. Boot headroom 32,992.

### `NDS_TASK51_STAGE_NATIVE` is REFUTED — it costs 88 frames of 30 FPS

`FTR_STG_OPTIMIZATION.md` lists Task 51 as a candidate "gated on visual
equivalence". It never gets that far: **the STG budget does not clear, and the
bucket Task 51 exists to shrink gets bigger.** Measured whole-match, both-CPU,
DLDI on, 1600 samples, frames 439→2038, distinct ROMs (`70DB81A6…` vs
`25F8B43E…`) from one tree (`2674756a8c`) differing by exactly one `#define`.

| | OFF | ON | delta |
|---|---:|---:|---:|
| **`WORK-H` P50** | 1,108,096 | 1,121,472 | **+13,376** |
| `WORK-H` mean | 1,165,492 | 1,177,915 | +12,423 |
| `WORK-H` P95 | 1,580,416 | 1,581,824 | +1,408 |
| **`STG` P50** | 195,776 | 198,144 | **+2,368** |
| `STG` mean | 200,294 | 202,379 | +2,085 |

**The VBlank histogram settles it**, and it is immune to bucket placement
because it is wall-clock quantization:

| interval | OFF | ON |
|---|---:|---:|
| **2 VBlanks (30 FPS)** | **1,119** | **1,031** |
| **3 VBlanks (20 FPS)** | **836** | **921** |
| 5+ | 21 | 26 |

**88 frames moved from 30 FPS to 20 FPS.** `ALL` P50 flips 1,118,592 →
1,678,016 — the median frame crossing from two VBlanks to three, exactly the
quantisation `all-is-a-quantized-gate` describes, here reporting a real event
rather than an artifact.

**Why it loses, and the lesson that generalises.** Task 51 replaces a per-frame
CPU compose of projection × view × model with 42 baked `MTX_MULT4x3` emissions.
But `FTR_STG_OPTIMIZATION.md` records in its own STG section that the **stage
prepare cache already runs at 99.9% reuse** — so the CPU compose it replaces was
already amortised to nearly nothing, while the 42 matrix commands are paid to
the geometry engine *every frame, unconditionally*. It trades a cached no-op for
real recurring GX work. **Before replacing CPU work with hardware commands,
check the cache hit rate on the work being replaced** — a 99.9%-reused compose
is not a cost, and beating it requires the replacement to be free, not merely
cheaper per invocation.

Leave `NDS_TASK51_STAGE_NATIVE ?= 0`. No visual qualification is needed; it
fails on performance first.

**Correction (cycle 109): `NDS_DREAMLAND_DS_MESH` is NOT untested.** This section
said it was, and that was wrong. It is **Task 62, REVERTED at the owner's visual
gate on 2026-07-25** — `docs/optimization/archive/Task62_AB_Results.md` has the
verdict and `artifacts/visibility/task62_v7.png` the evidence: the mesh drew as
opaque white alpha-card rectangles, because the compiler discarded the UV,
colour/alpha, material-epoch and depth metadata, and the host silhouette oracle
ignored the same semantics. Its `−29.6%` stage-work figure survives **as
rejected-experiment evidence only**. `check-published-roms.ps1:36` and
`check-harness-registry.ps1:95` both enforce `=0`, so a published ROM carrying it
fails the verifier. Do not schedule an A/B for it; a future attempt has to keep
that metadata, which is a new compiler, not a flag flip.

### Vertex memo LANDED, engagement perfect, ticks below the floor

`gNdsMPVertexF32Hits=155,515`, `Fills=11`, `Overflow=0`. **Dream Land's floor
sweep touches exactly ELEVEN distinct vertices, and the same eleven `(f32)`
conversions were being redone 155,515 times.** A 99.993% hit rate, no overflow
against the 128 cap. The premise is confirmed as strongly as a counter can
confirm anything.

The ticks are not:

| | bundle | +vertex memo | delta |
|---|---:|---:|---:|
| `WORK-H` P50 | 1,113,536 | 1,113,792 | +256 |
| `WORK-H` mean | 1,169,961 | 1,165,628 | −4,333 |
| `WORK-H` P95 | 1,565,760 | 1,568,960 | +3,200 |
| 2 VBlanks (30 FPS) | 1,093 | 1,083 | −10 |

Every number is inside its floor. **And the sizing on the board above was
optimistic — correct it before reusing.** The lane's `__aeabi_i2f` total is
2,516,993 cycles, but this change only removed
`ndsStageMPSweepFloorLoopSweep`'s **73,530 calls = 1,235,304 cycles ≈ 1,400
ticks/frame**. `mpCollisionGetFCCommonFloor`'s own 56,567 calls (950,326
cycles) are in a different function and are **still there** — the same memo
applied there is the obvious next increment, and it is also sub-floor alone.

Kept: it removes real work, is bit-identical, is proven to engage, and Boundary
passes. Not claimed as a tick win.

### THE SAMPLER IS BIT-DETERMINISTIC — the "noise floor" is placement, not noise

Ran the **identical ROM twice** (`c109-batch.json` vs `c109-batch-rerun.json`,
three minutes apart). The only differing field in either file is `capturedUtc`.
**`buckets` compares equal**: every bucket, every percentile, `named=1,145,659`
and the VBlank histogram all reproduce exactly.

| WORK-H | run 1 | run 2 | variance |
|---|---:|---:|---:|
| mean | 1,164,005 | 1,164,005 | **0** |
| P50 | 1,111,808 | 1,111,808 | **0** |
| P95 | 1,548,288 | 1,548,288 | **0** |

Per-bucket variance is 0 for `SRC`, `GCRA`, `SINT`, `SPHD`, `SCPU`, `SHDT`,
`FTR`, `STG`, `MISC`, `OTHR`. Not "small" — zero.

**Three consequences, and they overturn standing practice.**

1. **Never repeat a sampler run.** It cannot disagree with itself. The board's
   "run a third A when the A/B is noisy" rule is meaningless for this instrument
   and every confirmation run ever spent on it bought nothing. That is ~25
   minutes recoverable per would-be repeat.
2. **The 14,080 cross-build figure is NOT a noise floor. It is deterministic
   placement sensitivity.** This matters enormously: noise can be averaged down,
   placement cannot. No number of runs will ever separate a 3,000-tick code win
   from a 6,000-tick re-addressing shift. **Only measuring ONE binary two ways
   can** — standing rule 7's `.data` route is not a convenience, it is the sole
   available method.
3. **It reconciles the Boundary flake.** Guest execution is deterministic; the
   *host-side* 30-second gdb marker budget was what varied. Both observations
   were true and they are not in tension.

**Applied to this cycle's seven-cut batch** (control = AObj pool + cubic `i2f`
only): `WORK-H` P95 **−32,128**, P50 +3,712, 2-VBlank frames −19. The P95 figure
is **real and repeatable for that binary** — it is not noise. It is also **not
attributable**: `FTR` moved **−7,808** and `SCPU` **+9,024**, and neither can be
produced by animation or collision cuts, so placement moved at least as much as
the code did. **Banked as "this binary is 32,128 better at P95", NOT as "these
cuts are worth 32,128."** The distinction is the whole lesson.

### THE STRUCTURAL FINDING: this campaign cannot measure itself cut-by-cut

Four exact, verified, work-removing cuts landed this cycle. **Not one is
measurable on its own**, and that is not a property of the cuts:

| cut | work removed | ticks/frame | measured |
|---|---:|---:|---|
| AObj pool | ~6,200,000 shared | ~2,800 | not measurable |
| cubic `i2f` | 1,017,778 | ~1,160 | not measurable |
| loop-invariant hoist | 1,955,955 | ~2,220 | not measurable |
| fused multiply | 1,524,849 | ~1,730 | not measurable |
| vertex memo | 1,235,304 | ~1,400 | not measurable |

Floors: `WORK-H` P95 **14,080** cross-build, P50 **~5,700**, per-bucket
**8,544**, and `.text.hot` re-addressing alone moves P50 **~6,144**. **Every
individual cut in the remaining priced tables is 1,100–4,600 ticks/frame.** The
gap is ~290,000. So the campaign needs on the order of **seventy such cuts**,
and no single one of them can ever be shown to work.

**Two consequences, both actionable.**

1. **Stop A/B-ing individual cuts.** It burns ~25 minutes per arm to return
   noise with a confident-looking sign, which is how Task 51's *real* regression
   and the fused multiply's *false* one both got their apparent evidence.
   Accumulate exact, mechanism-verified cuts and measure in batches large enough
   to clear 14,080 — roughly **ten cuts at a time**.
2. **Mechanism verification is the per-cut gate, not ticks.** What made every
   cut above trustworthy was *not* a tick delta: it was disassembly (`bl`
   removed, literal hoisted out of the loop), exhaustive proof
   (`check_s32tof32_exact.py` over 2^32 inputs), engagement counters
   (155,515 hits / 11 fills) and Boundary. That combination is cheap, fast, and
   does not lie. Require it per cut; require ticks per batch.

### NEXT TARGET, fully specified: the port-side collision lane (5.46% of non-idle)

`SINT`'s second child, `ftComputerProcessAll` (+24,386 over-gate), is **not AI
logic** — it is map collision, and unlike the animation lane **every hot symbol
is ours**, with no decomp-include problem blocking an edit.

| symbol | self | soft float | sf calls |
|---|---:|---:|---:|
| `mpCollisionGetFCCommonFloor` | 7,533,938 | 3,389,278 | 144,408 |
| `ndsStageMPSweepFloorLoopSweep` | 6,447,004 | 1,596,312 | 98,126 |
| `ndsMPFindLineEndpoints` | 5,366,814 | 434,355 | 25,856 |
| `ndsStageMPAdjustFloorLoopWallSweep` | 4,078,548 | 3,301,075 | 98,857 |
| `ndsStageCollisionLoopGeometryReady` | 4,075,073 | — | — |
| `ndsMPFCSegmentCrossesKernel` | 3,460,265 | 4,958,252 | 224,280 |
| +4 more | 8,683,678 | 91,305 | 5,431 |
| **total** | **39,645,320** | **13,770,577** | |

**53,415,897 cycles = 5.46% of non-idle, ~60,400 ticks/frame at P50.**

**The diagnosis, per helper per function** (`__aeabi_i2f` 16.80 cyc/call,
`fsub` 36.43, `fdiv` 109.38):

- **`__aeabi_i2f`: 149,821 calls, 2,516,993 cycles.** 73,530 of them in
  `ndsStageMPSweepFloorLoopSweep` alone — **77% of that function's soft float**.
  They exist because `ndsMPVertexX/Y` return `s32` decoded from **s16** stage
  data (`reloc_backend_mp_collision.c:288-296`) while
  `ndsMPFCSegmentCrossesKernel` takes `float`. **Dream Land's collision geometry
  is static** — the stage does not move — so every one of these conversions
  recomputes a constant.
- **`__aeabi_fsub`: 68,164 in the kernel + 47,991 in the wall sweep =
  4,231,527 cycles.** The kernel's first act is `sx = v2_x - v1_x; sy = v2_y -
  v1_y` — **per-line constants**, recomputed on every one of 224,280 calls,
  along with the four `min_x/max_x/min_y/max_y` comparisons right after them.
- **`__aeabi_fdiv`: 11,684 calls, 1,278,000 cycles** at 109 cycles each.

**The fix is precomputation, and it is small.** E51 already established that
**Dream Land has 7 collision lines total**
(`reloc_backend_mp_collision.c:350-354`). A per-line record of
`v1x, v1y, v2x, v2y, sx, sy, min_x, max_x, min_y, max_y` as `f32` is **7 x 40 =
280 bytes**, built once when the stage geometry is bound, and it deletes the
i2f conversions, the sx/sy subtractions and the min/max comparisons from the
per-query path.

**This is NOT the thing E51 refuted.** E51 killed a `line_id -> (group, kind)`
lookup table on the grounds that the `yakumono_count` loop has a trip count of
one, i.e. there was no O(n) to remove. This removes *conversions and repeated
arithmetic on static data*, not loop iterations. Different mechanism, and the
7-line count E51 measured is what makes this cheap rather than what makes it
pointless.

**Risk note for whoever takes it:** this is gameplay collision — the subsystem
behind "fighters floating under the stage" in `BUGS.md`, and the kernel's own
header warns that proximity alone must never report a hit. The cache must be
invalidated on stage bind, and Boundary is mandatory, not optional. It is the
highest-value remaining target and it deserves a fresh session, not the tail of
one.

### The soft-float-free kernel is NOT a measured win — placement ate it

Both cuts are in, exact and Boundary-verified, and together they delete
**3,480,804 cycles** of proven work (hoist 1,955,955 + fused multiply
1,524,849) -- about **3,950 ticks/frame**. The arm does not measure faster.
Whole-match, both-CPU, 1600 samples, against tonight's control:

| | control | bundle | delta |
|---|---:|---:|---:|
| `WORK-H` P50 | 1,108,096 | 1,113,536 | **+5,440** |
| `WORK-H` mean | 1,165,492 | 1,169,961 | +4,469 |
| `WORK-H` P95 | 1,580,416 | 1,565,760 | −14,656 |
| 2 VBlanks (30 FPS) | 1,119 | 1,093 | **−26** |
| 3 VBlanks (20 FPS) | 836 | 875 | +39 |

**The confound is identified, not guessed: `FTR` P50 moved +4,736.** Animation
changes cannot affect fighter draw, so that number is pure placement. Text grew
**220 bytes**, worth only ~407 cycles by the 1.85-cycles-per-byte rule, so this
is not footprint — it is `.text.hot` re-addressing, which
`linker/nds_hot_text.ld:179-201` already measures at **6,144 `WORK-H` P50 on 122
of 128 frames** when one member of the curated 8 KiB list is perturbed. The work
removed (~3,950) is smaller than the perturbation removing it causes (~6,000).

**Kept anyway, and deliberately.** The board's standing rule is that milestone
targets are directional and every repeatable correctness-preserving gain is kept
and accumulated. These delete real work, are exact, and are verified. What is
*not* claimed is a win: **do not cite this bundle as a tick improvement.**

**RETRACTED follow-up: do NOT re-curate `.text.hot`.** The first version of this
entry proposed exactly that. It is a documented dead end, recorded in the very
file the edit would have touched. `linker/nds_hot_text.ld:174-200` closes the
list **in both directions**: Task 94 moved `gcPlayDObjAnimJoint` *out* (top-ranked
candidate, zero eviction) and regressed `WORK-H` P50 **6,144** on 122 of 128
frames, with `STG` rising 3,712 despite never calling it; R2-03 E66 admitted
`ndsR2CubicValueFixed` *in* -- ranked #1 unplaced at 1,815,752 recoverable stall
cycles -- and regressed `WORK-H` P95 **+24,448**. **Two independent estimators
got the sign wrong.** HANDOFF carries the same closure. Re-deriving it costs a
build.

**The correct answer is standing rule 7, which these cuts can actually use.**
Placement noise is a property of comparing two binaries, not something to
optimise away. The AObj pool could not take a runtime route because it runs at
scene setup, before the poke lands -- but the hoist and the fused multiply are
**per-frame code paths**, so both arms fit in ONE binary behind a `.data` global
driven by `sample-tick-hud-buckets.ps1 -SetGlobals`. That removes the ~6,000
placement term entirely and is the only way this lane's ~3,950-tick cuts can be
judged on their merit. Note the rule's own caveat: the poke lands after ~3 warm
steps, so the OFF arm is partial and must be scaled.

**Until that route exists, stop measuring animation cuts by building two arms.**
Every cut in the priced table is smaller than the placement term, so a two-arm
A/B on any of them returns noise with a confident-looking sign.

### Whole-match sampler invocation, exactly (cycle 109 — cost three runs)

The HANDOFF line "`-Samples` to 4096" is the parameter's *ceiling*, not the
window to use, and reading it as the window wasted a 30-minute run.

- **`-Samples 1600`.** That is the match. It is where every banked figure's
  denominator comes from (`754/1600`, `707/1600`). **4096 runs past the end**
  and dies at `TimeoutSeconds` having reached ring stop 15 of 43.
- **`-AllowRepeatedFrames` is required on the gate arm.** Without it the run
  completes and is then *thrown away*: about 4 presented-frame numbers per 1600
  repeat. They are not double-reads — the harness itself annotates each one
  `payload DIFFERS (real second iteration)`, so the samples are distinct
  iterations that reported the same frame counter.
- **`-NoBuild`** or the sampler rebuilds with no `MakeFlags` and silently wipes
  the arm's configuration.
- **`-JsonOut`**, not `-OutName`.

**`capture-melonds.ps1 -ExactFirstFrame` does not work on this ROM.** It demands
`-ExactSecondFrame` as well *and* `-SoftwareRenderer`, and then delegates to
`capture-cut-g-exact-frames.ps1`, which fails with *"Exact frame 439 lost
native-OAM GO recognition or drawing state"* — the exact-frame path is gated on
Cut G's GO-text state, not on arbitrary battle frames. For an ordinary visual
A/B use the delay-based capture, or qualify through the Boundary verifier, which
produces `artifacts/visibility/latest.png` plus its regional analysis.

### RESOLVED: Boundary's 30s marker budget, three wrong verdicts, and the fix

Three Boundary failures in one night all read *"GDB marker capture timed out
after 30 seconds"*, each with a **different** stall PC
(`__syscall_lock_acquire`, then `memcpy`), and the last had already printed most
of the marker dump before expiring. That is a capture finishing late, not a ROM
hanging.

**Root cause.** The Boundary path (no `-OneMinuteMatchProof`) borrowed
`$RendererBenchmarkTimeoutSeconds` for the marker capture — default **30** —
while the one-minute proof path gets 300. Thirty seconds has to cover the gdb
attach, four breakpoints and the whole dump, and that dump has grown to dozens
of `printf` lines as counters were added across the campaign. Nobody moved the
budget with it. Fixed in
`verify-battle-mariofox-gcrunall-loop-harness.ps1:1346` — floored at **120s**
via `Max()`, so an explicit larger value still wins and the renderer benchmark's
own budget is untouched. A timeout is a ceiling, not a sleep; a passing run pays
nothing.

**It cost a wrong answer, not just time.** The fused multiply was bisected to a
"hang" and reverted on *red with it, green without it* — then the identical
reverted tree failed too. **`NDS_TASK10_GIT_SHORT` is compiled in, so every
commit changes the ROM image**: the passes were at `189cd20680`, the failing
re-test at `5b6cb20aa3`, same source, different binaries, on a ROM whose pacing
is placement-sensitive. Standing rule 7 was applied to the sampler and not to
the verifier. **"Re-run the same tree" is not "re-run the same binary" in this
repo.**

**Both changes are now in and verified** on the repaired harness, each with
zero marker timeouts:

- the loop-invariant hoist in `gcPlayDObjAnimJoint` (~1,955,955 cycles: the
  1,069,318-cycle `AOBJ_ANIM_END` literal reload plus the 886,637-cycle
  `parent_gobj->flags` chain, both re-run once per node for a value that never
  changes);
- `ndsR2F32MulToFixed`, the fused `f32 x f32 -> Q16` (1,524,849 cycles).

**`ndsR2CubicValueFixed` now contains no `bl __aeabi_*` at all** — ten `umull`,
one `clz`. The kernel is free of the soft-float library.

**Standing rule, added:** judge nothing on a verifier whose flake rate on one
*unchanged binary* has never been measured. If Boundary fails, re-run before
bisecting.

### Whole-match sampler invocation, exactly (cycle 109 — cost three runs)

The HANDOFF line "`-Samples` to 4096" is the parameter's *ceiling*, not the
window to use, and reading it as the window wasted a 30-minute run.

- **`-Samples 1600`.** That is the match. It is where every banked figure's
  denominator comes from (`754/1600`, `707/1600`). **4096 runs past the end**
  and dies at `TimeoutSeconds` having reached ring stop 15 of 43.
- **`-AllowRepeatedFrames` is required on the gate arm.** Without it the run
  completes and is then *thrown away*: about 4 presented-frame numbers per 1600
  repeat. They are not double-reads — the harness itself annotates each one
  `payload DIFFERS (real second iteration)`, so the samples are distinct
  iterations that reported the same frame counter.
- **`-NoBuild`** or the sampler rebuilds with no `MakeFlags` and silently wipes
  the arm's configuration.
- **`-JsonOut`**, not `-OutName`.

**`capture-melonds.ps1 -ExactFirstFrame` does not work on this ROM.** It demands
`-ExactSecondFrame` as well *and* `-SoftwareRenderer`, and then delegates to
`capture-cut-g-exact-frames.ps1`, which fails with *"Exact frame 439 lost
native-OAM GO recognition or drawing state"* — the exact-frame path is gated on
Cut G's GO-text state, not on arbitrary battle frames. For an ordinary visual
A/B use the delay-based capture, or qualify through the Boundary verifier, which
produces `artifacts/visibility/latest.png` plus its regional analysis.

### RETRACTED: the fused-multiply "hang" bisect was not controlled

The section below concludes the fused multiply hangs the ROM, on the strength of
*red with it, green without it*. **That inference is withdrawn.** Later the same
night the **identical reverted tree failed too** — same `GDB marker capture
timed out after 30 seconds`, stalled in `memcpy()` instead of
`__syscall_lock_acquire`. A tree that passed twice and then failed cannot be the
control in a bisect.

**Why the "same tree" was not the same binary.** `NDS_TASK10_GIT_SHORT` is
compiled in (`nds_build_config.h`), so **every commit changes the ROM image**.
The passing runs were at `189cd20680`, the failing re-test at `5b6cb20aa3`; the
sources matched but the binaries did not, and this ROM's pacing is
placement-sensitive — which is the whole reason standing rule 7 exists. Four
Boundary runs spanning three commits are four different binaries.

**The failure signature says timeout, not hang.** The budget is a fixed 30 s,
the stall PC differs every time (`__syscall_lock_acquire`, then `memcpy`), and
the last failure had already printed the full marker dump before expiring. That
is a run finishing late, not a lock-up. No leaked `melonDS`/`gdb` processes and
1% CPU at the time, so idle-machine load does not explain it either.

**Consequences.** The fused multiply (1,524,849 cycles) is **unproven, not
disproven** — it may be perfectly correct. The loop-invariant hoist below is
equally unverified. **Neither is in the tree**; HEAD is the last state with a
recorded pass, and the hoist is parked in the session scratchpad as
`objanim.hoist.c`.

**Before either is retried, Boundary needs to be trustworthy again.** Establish
how often it passes on one *unchanged binary* (not one unchanged tree), and
raise the 30 s marker budget or find what made it marginal. Judging a
performance change on a harness with an unmeasured flake rate is how tonight
produced a confident, wrong verdict.

### The fused `f32 x f32 -> Q16` attempt (verdict retracted above)

Attempted the last cut that does not need the representation change — replacing
`ndsR2F32ToFixed(length * length_invert, BF)` with one integer multiply of the
two significands (`ndsR2F32MulToFixed`), worth **1,524,849 cycles**. It built,
it removed the last soft-float call from the kernel (disassembly showed **zero
`bl __aeabi_*`, 10 `umull`, 1 `clz`**), boot headroom held at 32,800, and
`check_r2_cubic_error_bound.py` passed **green and numerically unchanged**
(0.002842 / 0.006702).

**And the ROM hangs.** Boundary failed twice with a GDB marker-capture timeout,
stalled in `__syscall_lock_acquire` with the frame-complete marker never
reached. Reverting only that change and re-running gave **`Boundary
verification profile passed`** on the same tree. Two red with, one green
without: the fused multiply is the cause. Backed out uncommitted; HEAD is the
verified-green state.

**The lesson is about the checker, not the arithmetic.** The error bound samples
a *gameplay-plausible domain* of `(length, length_invert, values, rates)` and
compares against the decomp float; it never executes the parser, so an input the
parser really produces — `length_invert` is **overloaded**, holding `1.0F/payload`
for the cubic arms but a raw `payload` for `SetValAfter` and `1.0F` from
`gcAddAObjForDObj` — can sit outside every sampled domain and still reach the
kernel. **A host bound that passes is not a substitute for Boundary on a change
that alters saturation or range behavior.** The next attempt needs the real
input distribution instrumented out of a run first, not more host sampling.

Do not re-attempt this cut without that. It is 1,524,849 cycles — worth having,
not worth a second unexplained hang.

### The DMA spin is GEOMETRY SUBMISSION, not a free win (cycle 108)

Followed up the census's largest site and it is **not** the clean win it looked
like. All three sites share one shape (`nds_renderer.c:13647`, `:22936`, and
inside `ndsRendererTask36ReplayRun`):

```c
DC_FlushRange(packet->words, word_count * sizeof(u32));
while ((DMA_CR(0) & DMA_BUSY) != 0u) { }   /* leading wait */
DMA_SRC(0) = (u32)packet->words;
DMA_DEST(0) = (u32)&GFX_FIFO;
DMA_CR(0) = DMA_FIFO | word_count;
while ((DMA_CR(0) & DMA_BUSY) != 0u) { }   /* trailing wait -- the 507 cyc/ex */
```

The destination is **`&GFX_FIFO`**, so this is geometry going to the 3D engine.
**Deleting the trailing wait does not recover 6,654,860 cycles**, for two
independent reasons:

1. **NDS DMA steals the bus.** The ARM9 does not run freely during the transfer;
   it stalls on any bus access. Overlap only exists for work running entirely
   out of ITCM/DTCM, so the recoverable share is bounded by how much
   cache-resident work follows the submit, not by the whole wait.
2. **`DMA_FIFO` interleaves with CPU FIFO writes.** Removing the trailing wait
   is only safe if nothing touches `GFX_FIFO` or a GX command register before
   the next submit's leading wait. This file writes `GFX_TEX_FORMAT` and
   `GFX_PAL_FORMAT` directly, so it needs a deferred-sync guard plus an audit of
   every GX write site — not a two-line deletion. A mistake here corrupts the
   command stream, which is the failure mode the verifier is weakest at
   catching.

**And the transfer itself is already efficient.** ~504 cycles for ~13,200
executions works out near DMA's ~2-cycles-per-word floor at roughly 250 words a
transfer, ~33 transfers a frame. That is the **cost of the geometry volume**,
not per-transfer overhead, so batching buys nothing either. Reducing it means
submitting less geometry, which is a `PROJECT_GOAL` fidelity decision.

**What is still open here, in order of cheapness:** count words per transfer
from `gNdsRebirthHaloPackedWordCount / SubmitCount` (a counter that already
exists) to confirm the 250-word estimate; then, if a deferred-sync guard is
wanted, do the GX-write audit first and put the change behind a runtime route so
it can be A/B'd on one binary.

### The D-cache census — 17.83% of non-idle is data-load excess (cycle 108)

`scripts/analyze-dcache-stalls.py`, run on the cycle-106 profile. No build, no
emulator run. Ranks every memory access by its measured `average_cycles` and
joins it to the disassembly, so the output names the *instruction* — base
register and offset — not just the function.

| class | cycles | executions | cyc/ex |
|---|---:|---:|---:|
| **load (data)** | 252,353,053 | 35,715,768 | **7.07** |
| load (literal pool) | 57,122,935 | 8,760,593 | 6.52 |
| load (stack) | 66,067,255 | 12,628,225 | 5.23 |
| store | 116,346,526 | 31,178,647 | 3.73 |
| other | 676,279,870 | 249,102,962 | 2.71 |

A cached ARM9 `ldr` retires in ~1–3. Excess over a 3-cycle baseline, data loads
only, is **174,495,113 cycles = 17.83% of non-idle work**.

**The single largest site is not a cache miss at all.**
`ndsRendererTask36ReplayRun`, `ldr r3, [r1, #184]` at **507.2 cycles per
execution**, 13,200 executions, **6,654,860 excess — 58% of that function.** Six
instructions earlier `r1` is loaded with `#67108864` (**0x04000000**, the I/O
base), so this reads **0x040000B8 = DMA0CNT**, and `cmp r3,#0 / blt` back to
itself is a **spin on the DMA busy bit**. The CPU is idle-waiting on hardware.
There is a second such loop immediately above it. This is real recoverable time
and it is nothing to do with layout: overlap the transfer with CPU work, or
resize it.

**Some extreme sites are the write buffer being charged to the next load.**
`ndsRendererAdapterBuildNativeMaterialSnapshot`'s `ldrh r6, [r5, #56]` reads
138.8 cyc/ex and sits **immediately after `bl memset`** — the memset's stores
drain, and the stall lands on the following load. So "memset is 2.09%" and "this
load is expensive" are largely the same cost, counted once, at the load. **Do
not add them.**

**Genuine structure misses, which is where a layout change pays:**

| cyc/ex | execs | excess | site |
|---:|---:|---:|---|
| 37.1 | 79,874 | 2,724,683 | `ndsFTParamsInvalidateFighterParts` `ldr r4,[r0,#16]` |
| 30.8 | 79,874 | 2,221,897 | `ndsFTParamsInvalidateFighterParts` `ldr r3,[r0,r3]` |
| 45.4 | 37,834 | 1,602,763 | `ndsBaseGcRunAll` `ldr r3,[r0,#20]` |
| 24.1 | 143,916 | 3,034,025 | `gcPlayDObjAnimJoint` `ldrb r5,[r4,#5]` (`aobj->kind`) |
| 33.1 | 41,047 | 1,237,303 | `battleship_ftAnimParseDObjFigatree` `ldr r4,[r0,#116]` |

Top functions by data-load excess: `ndsFighterMarioFoxDLAllDrawForSlot`
9,071,206; `ndsRendererExecuteNativeFighterOwnerProduction` 8,824,490;
`ndsRendererTask36ReplayRun` 7,648,881; `ndsFTParamsInvalidateFighterParts`
6,529,067; `ndsBaseGcRunAll` 4,583,792; `gcPlayDObjAnimJoint` 3,990,670.

**`ndsFTParamsInvalidateFighterParts` is the best-shaped candidate on this
board.** CPI **7.08**, 6.53M of load excess concentrated in **two instructions**
on the same base register, and it sits inside the simulation — where `SRC`
(+171,383) decides the gate — rather than in fighter draw, which `FTR` (+13,768)
proves does not. Two loads at 37.1 and 30.8 cyc/ex off `r0` is a structure
walked once per part per frame that never stays resident.

### THE MACHINE IS MEMORY-BOUND — 65% of non-idle cycles are stall (cycle 108)

**Read this before proposing any instruction-count optimization.** The profile
reports instructions as well as cycles, and nobody had divided them.

**Whole profile: 1,211,130,791 cycles / 342,792,094 instructions = CPI 3.53.**
Non-idle: 978,488,987 / 342,785,681 = **CPI 2.85**, so **635,703,306 cycles —
65.0% of all non-idle work — are stall**, not issue. ARM9's ideal is ~1.0.

**This retires the fixed-point conversion campaign as briefed, and it is the
reason every arithmetic experiment this cycle measured sub-floor:**

| symbol | cycles | instructions | CPI |
|---|---:|---:|---:|
| `__aeabi_fadd` | 33,839,425 | 28,429,032 | **1.19** |
| `__aeabi_fmul` | 21,887,296 | 19,160,867 | **1.14** |
| `ndsR2CubicValueFixed` | 19,420,815 | 11,143,118 | 1.74 |
| `battleship_ftMainProcUpdateInterrupt` | 5,656,799 | 490,727 | **11.53** |
| `ftMainProcPhysicsMap` | 3,643,426 | 414,209 | **8.80** |
| `ndsFTParamsInvalidateFighterParts` | 13,718,726 | — | 7.08 |
| `ndsRendererTask36ReplayRun` | 11,463,652 | — | 7.18 |

**The soft-float helpers are the most efficient code in the build.** They are
libgcc ARM assembly in ITCM and issue at CPI ~1.15 — near ideal. The 8.9% they
cost is honest instruction count, and deleting it is a real 8.9%, but it is
being taken out of the *efficient* 35% of the machine. Meanwhile
`ftMainProcUpdateInterrupt` and `ftMainProcPhysicsMap` — which are exactly the
`SINT` (+88,082) and `SPHD` (+28,941) over-gate discriminators — run at **11.53
and 8.80 CPI and are almost pure stall.** At CPI 2 the interrupt proc would cost
981,454 instead of 5,656,799.

**Named the mechanism in the animation evaluator, per instruction.** The hottest
instruction in `gcPlayDObjAnimJoint` is `ldrb r5, [r4, #5]` — `aobj->kind` — at
**24.1 cycles per execution**, 143,916 executions, **3,465,773 cycles, 23% of the
function**. That is one D-cache miss per AObj node per frame: ~360 live AObj
nodes against a **4 KB** ARM9 data cache means the list cannot stay resident, so
every node is a miss every frame. `aobj->next` (offset 0, same line) then costs
only 7.0. Converting this function's arithmetic to fixed point would not touch
the 24.1.

**Rank by STALL, not by cycles — it is a different top ten:**
`ndsFighterMarioFoxDLAllDrawForSlot` 21,261,766 stall at CPI 5.60,
`ndsRendererExecuteNativeFighterOwnerProduction` 20,320,720 at 4.27,
`ndsRendererNativeEmitProductionRawUntexturedRun` 16,411,238 at 2.48, `memset`
14,979,168 at 3.73, `ndsRendererCommitNativeStageSegment` 14,425,717 at 2.61,
`ndsFTParamsInvalidateFighterParts` 11,779,708 at 7.08.

**What this means for the lane order.** Data layout, working-set size and
placement are the lever; instruction count is not. Cycle 105's arena fix
(−191,981, the largest win of the campaign) was a memory-system fix, and that is
not a coincidence. The next measurement should be a **D-cache working-set
census** of the simulation's hot structures — AObj/DObj/MObj lists and
`FTParams` — not another conversion. `scripts/analyze-leaf-helper-attribution.py`
and the per-PC `average_cycles` column already do this for free: a load above
~10 cycles/execution is a miss, and they can be ranked exactly the way the stall
table above was built.

### How big a win has to be — the sensitivity curve (cycle 108)

Computed free from the head configuration's rows. **This is the number to size
any future proposal against**, and it ends the practice of judging a lever by
whether its P95 delta clears a floor.

| uniform body-wide saving | over gate | % over | new P50 | new P95 |
|---:|---:|---:|---:|---:|
| 0 | 707 | 44.2% | 1,107,008 | 1,411,283 |
| 25,000 | 599 | 37.4% | 1,082,008 | 1,386,283 |
| **50,000** | **469** | **29.3%** | 1,057,008 | 1,361,283 |
| **100,000** | **295** | **18.4%** | 1,007,008 | 1,311,283 |
| 200,000 | 149 | 9.3% | 907,008 | 1,211,283 |
| 291,000 | 80 | 5.0% | 816,008 | 1,120,283 |

**The distribution is packed against the line.** The median clears the gate by
only **13,372**, **238 frames sit within 50,000 of it from above**, and 412
within 100,000. So a body-wide saving is worth far more than its P95 delta
suggests: 50,000 moves **238 frames** from 20 FPS to 30 FPS while moving P95 by
exactly 50,000, which at P95 alone would read as a modest win. The VBlank
histogram says the same thing in presentation terms — **2:1062, 3:895** — the
match is already 30 FPS on two thirds of frames and the job is the other 895.

**Only one lane on the map is the right size.** Soft float is ~**98,500 ticks
per frame** (8.9% of non-idle), i.e. converting it lands on the 100,000 row:
707 → 295 over gate, 412 frames crossing. Nothing else measured this cycle is
within an order of magnitude — the compare sub-lane is 0.5%, `memset`/`memcpy`
is 3.92% but concentrated in fighter draw which `FTR` proves is not where the
gate is decided, and every remaining local edit is worth 500–5,000 ticks against
a 291,000 gap.

**Two mechanisms are refuted, so the arithmetic must actually not happen:**
recompiling is out (`Makefile:3165-3179`, `battleship_gmcollision.o -marm` read
−2,304, inside the floor, because `-marm` only buys the call sites and cannot
change libgcc's own mode), and `-ffinite-math-only` does not lower a single
compare. Every float call from Thumb is a `blx` — a real interworking switch
each way, which is why `fadd` measures 36.4 cycles against a ~15–20 cycle ARM
body — but that cost is unreachable without removing the call.

**Anything at or beyond the 100,000 row is a `PROJECT_GOAL` sacrifice-order
decision and needs the owner.** Reduced animation update rates and independent
update rates are explicitly listed as allowed (visual fidelity is sacrifice #2),
and the contract says to test them only after cheaper equivalents are exhausted
— which cycles 105–108 have now largely done at this seam, each refutation
recorded above.

### What is actually left: the over-gate frames, decomposed (cycle 108)

Taken on `build-c111-fcmp`, the current head configuration, whole match both-CPU.
**This corrects a working assumption that has been steering the campaign.**

**44.2% of frames are over gate — 707 of 1600 — not the ~5% that "P95 is the
load-frame boundary" implies.** `WORK-H` P50 is **1,107,008** against a gate of
1,120,380, so the median frame clears it by only 13,372 and nearly half the match
does not. The excess summed over every over-gate frame is **91,928,908 ticks**,
and the 80 frames above P95 average 1,559,629, i.e. **439,249 over gate each**.

Splitting every bucket by over-gate vs under-gate names the discriminators
exactly, and they are not where the last three experiments looked:

| bucket | under-gate | over-gate | delta |
|---|---:|---:|---:|
| **`SRC`** | 303,813 | 475,196 | **+171,383** |
| ↳ `GCRA` | 298,699 | 470,129 | **+171,430** |
| ↳ ↳ `SINT` | 140,411 | 228,493 | **+88,082** |
| ↳ ↳ `SPHD` | 67,785 | 96,726 | +28,941 |
| ↳ ↳ `SHDT` | 4,235 | 31,424 | +27,190 |
| ↳ ↳ `SCPU` | 37,605 | 61,135 | +23,531 |
| `MISC` | 112,351 | 129,263 | +16,911 |
| `FTR` | 390,903 | 404,671 | +13,768 |
| `AUD` | 3,425 | 16,302 | +12,877 |
| `STG` | 200,013 | 201,512 | +1,499 |

**`GCRA` is `gcRunAll`** — `battleship_sys_objman.c:80` calls it "the SOLE
gateway to the whole simulation", so it is fighters, stage, camera, effects,
items, weapons and interface together, and it is ~33% of every frame. Its
over-gate excess is **almost entirely its named children**: 88,082 + 28,941 +
27,190 + 23,531 = 167,744 of 171,430, leaving the `SOBJ` residual at ~3,700.
There is no unattributed mass hiding in the simulation.

**Consequences, in order:**

1. **`FTR` is confirmed dead as a gate lever** — it separates the two
   populations by only **13,768** against `SRC`'s 171,383. Anything that only
   moves fighter draw (the `memset`/`memcpy` concentration in
   `ndsFighterMarioFoxDLAllDrawForSlot`, 30% of all memset calls) buys P50, which
   is already inside the gate, and buys almost nothing where the gate is decided.
2. **`SINT` is still the single largest discriminator at +88,082**, after cycle
   105's arena fix and cycle 108's prebake. The loader's *copy* is closed, but
   whatever else `SINT` brackets is not.
3. `SPHD`, `SHDT` and `SCPU` are the next three and together are **79,662** —
   comparable to `SINT` and never yet split. `SCPU` is CPU-player AI and is
   structurally doubled on the gate arm.
4. **Stop pricing levers against P95 alone.** With 44.2% over gate, a change that
   lowers the *body* moves 707 frames across the line; the same change judged
   only at P95 looks like noise. That is why the compare conversion read
   sub-floor while being real.

**Do not spend another cycle on `memset`/`memcpy`.** Priced by the same method
and refuted: the concentrated caller is fighter draw (see 1 above), and
`ndsMPCollisionEnsureLineGroups`'s 10,050 calls are **two 16-byte** zeroings
(`movs r2, #16` at `0205c7b8`/`0205c7c2`) worth ~0.04% of non-idle, not the
267-cycle global average. The 267 average belongs to large copies elsewhere.

### The COMPARE lane is priced and it is too small — 0.5% fully converted (cycle 108)

First slice attempted off the soft-float map, and the useful result is its size.
`include/nds/nds_fcmp.h` replaces the soft-float comparison helpers with integer
tests on the bit pattern. **Exact, not approximate**, and proven that way:
`scripts/check_fcmp_exact.py` sweeps **all 4,294,967,296 bit patterns** against
12 predicates and every one matches IEEE-754, including both signed zeros and
all 16,777,214 denormals. NaN is the one documented exclusion (67,108,856
disagreements, reported rather than hidden). Not sampled, and not argued from a
comment — the failure mode is a single pattern class that a random sample never
draws: `-0.0f == +0.0f` is TRUE in IEEE while the patterns differ, so a naive
`bits(x) == bits(0.0f)` is wrong for exactly one input in 2^32, and
`if (payload != 0.0F) { 1.0f / payload; }` would then divide by zero.

**`-ffinite-math-only` does not remove these calls.** Checked compile-only in
seconds before writing anything: GCC emits the same `bl __aeabi_fcmp*` with and
without it, in both ARM and Thumb. There is no build-flag shortcut, so the calls
have to go at the call sites.

Applied to `gcPlayDObjAnimJoint`, the single largest caller in the profile
(227,040 calls, 2,582,802 cycles). Mechanism confirmed in the object file
without the emulator: **81 → 76 static `__aeabi_fcmp*` sites**, ROM `.text`
+92 bytes.

| vs the route-on arm | P50 | P95 | mean |
|---|---:|---:|---:|
| `build-c111-fcmp` | 1,107,008 | 1,411,264 | 1,129,509 |
| delta | **−3,136** | **−4,739** | **−11,620** |

All three negative and all **below the cross-build floors** (P50 5,700, P95
14,080) — exactly the ~2,600 predicted from the attribution. Kept under the
board's accumulate rule because it is bit-exact and provably less work, but
**it is not independently bankable and must not be cited as a win.**

**The number that matters is the lane's ceiling.** The five comparison helpers
are 12,909,690 cycles (1.32% of non-idle) in total, and the port-editable share
— everything not inside `decomp/` — is only **~5.0M, about 0.5%, ~7,300 ticks at
P95 even if every site were converted**. `ndsBaseGcPlayMObjMatAnim` and
`ndsBaseGcPlayDObjAnimJoint` look port-side in the profile but are the *decomp*
originals renamed by `battleship_sys_objanim.c`'s `#define` block, so their
compares are not editable. `include/nds/nds_mp_floor_crossing.h` is compiled on
the host too, so a DS header cannot simply be added to it, and its constants are
negative (`-epsilon`) which the `_C` forms do not accept.

**Do not spend another cycle on comparisons.** The arithmetic is where the lane
actually is: `fadd`+`fsub` 33.8M (3.46%), `fmul` 21.9M (2.24%), `fdiv` 10.1M
(1.04%) — **6.74% against the compares' 1.32%** — and that needs a genuine
fixed-point conversion of a whole subsystem, not a predicate swap. The header
and the exhaustive checker stay because they make every future site free.

### The soft-float bill is MAPPED — 8.9% of non-idle work, and it is spread (cycle 108)

First full attribution of the ARM9 soft-float helpers to the functions that
**call** them. Free: no build and no emulator run, off the cycle-106 profile that
already existed. Tool is `scripts/analyze-leaf-helper-attribution.py`; output is
`artifacts/performance/2026-08-09_c106-profile/softfloat-attribution.json`.

**`__aeabi_fadd` is the largest non-idle symbol in the entire profile** —
33,839,425 cycles, ahead of every renderer function — and "optimize
`__aeabi_fadd`" is not a task anyone can do. Self time is the wrong view for a
leaf helper (the standing `self-time-is-not-a-subsystem-budget` rule), and a
*static* call-site ranking is worse: `ndsOpeningRoomRenderDLPreview` has 277
static soft-float calls and **zero** cycles in a battle profile. The profile is
per-PC, so the instruction count at a `bl <helper>` **is** that site's exact
dynamic call count — the same trick as `entry-pc-gives-exact-call-counts`,
applied to call sites instead of entries.

**Two traps, both of which gave wrong numbers by hand first.** `__aeabi_fsub` is
a two-instruction thunk that falls through into `__aeabi_fadd`, so its self time
is ~1 cycle per call and all its real cost is charged to fadd. Charging fsub's
370,065 calls at fadd's rate *and* into fadd's divisor moves fadd from a
nonsensical **60.6 to 36.4 cycles/call**, and the campaign total from 11.24% to
**8.9%**. A helper reached by a tail `b` is still a call site. The script handles
both; do not redo this by hand.

| subsystem | soft-float cycles | % non-idle | fns |
|---|---:|---:|---:|
| **animation evaluation** | 25,112,349 | **2.57%** | 10 |
| collision / stage MP | 17,471,376 | 1.79% | 17 |
| matrices / transform | 12,998,043 | 1.33% | 12 |
| other | 12,861,181 | 1.31% | 106 |
| gameplay (other decomp) | 11,176,042 | 1.14% | 90 |
| renderer, particles, CPU AI | 7,912,649 | 0.81% | 24 |

Per-call cost, measured: `fdiv` **109.4**, `fadd` **36.4**, `l2f` 28.6, `fmul`
25.2, `i2f` 16.8, the compares 10–15. The helpers themselves are libgcc's
hand-written ARM assembly and **already resident in ITCM**, so there is no
placement or implementation win here — only call volume.

**Top callers:** `battleship_ftAnimParseDObjFigatree` 7,931,155 (0.81%, self
16,192,916), `gcPlayDObjAnimJoint` 6,706,718 (0.69%, self 16,595,669),
`ndsMPFCSegmentCrossesKernel` 4,958,252, `ndsBaseGcPlayMObjMatAnim` 4,463,648,
`syMatrixLookAtReflectF` 3,807,640. **The two animation functions together are
5.34% of non-idle work counting their self time — roughly 75,600 ticks at the
current P95.**

**Read this as a base-cost lane, not a tail lane.** A load frame is base +
premium, so anything that lowers the base lowers P50 *and* P95 together; this is
the first lane in cycles that does both. `ndsR2CubicValueFixed` is **already
converted** and its one remaining `fmul` is documented as unavoidable at
`battleship_sys_objanim.c:211` — do not re-open it.

**Two constraints on any conversion, both already paid for.** L7's fixed-point
collision won +534 and lost 6,481 **to its own text**, and the standing rate is
**1.85 cycles of `FTR` mean per byte of added ARM text** — so a conversion must
replace float code with *equal or less* text, not sit beside it. And on
`-mthumb` there is no SMULL, so a `(s64)a*b` becomes a library call; the cubic
kernel carries a `target("arm")` attribute for exactly this and a new kernel
needs one too.

**Adjacent, same method, not soft float:** `memset` 20,471,352 (2.09%) and
`memcpy` 17,862,751 (1.83%) at ~260 cycles a call.
`ndsFighterMarioFoxDLAllDrawForSlot` alone drives 25,617 memsets and 17,833
memcpys — 30% of all memset calls — on top of 25,885,593 self cycles.
`ndsMPCollisionEnsureLineGroups` re-zeroes 10,050 times.

### The force-load seam is CLOSED — zero-copy is structurally impossible, cycle 108

The obvious next move after the prebake was to stop copying altogether: the
arena already holds every warmed animation, so finalize each image once and hand
back the pointer. **It cannot be done at this seam, and the reason is one line of
the caller.** `decomp/BattleShip-main/decomp/src/ft/ftmain.c:4623-4624`:

```c
lbRelocGetForceExternHeapFile(motion_desc->anim_file_id, (void*) fp->figatree_heap);
fp->figatree = fp->figatree_heap;
```

**The return value is discarded.** The fighter always animates from its own
`figatree_heap`, so the data must physically be there and the destination copy
is mandatory. The port already knew this and nobody noticed: the generic arm of
`lbRelocGetForceExternHeapFile` (`reloc_backend_assets.c:7396-7407`) copies the
result back into `heap` and returns `heap` whenever the pointer differs. That
guard is the same fact, written down years earlier.

Built and measured anyway, because the counters name the failure precisely.
Handing back the arena pointer does not read as a performance regression — it
reads as a **different match**: `ForceLoadTotal` 353 → **3,210**, `Distinct`
85 → **94**, `CacheHits` 351 → 3,146, `WORK-H` P95 **2,275,200**. Fighters
animating from a stale slot thrash their state machines. Reverted; no flag, no
dead code, nothing in the tree.

**Three facts from the attempt are permanent and cost real runs — do not
re-derive them:**

1. **Nothing writes into a finalized animation file during its residency.**
   Checksummed each file across its whole residency in the caller's slot:
   **351 checked, 351 stable, 0 mutated**, over exactly **2 slots** (one per
   fighter). This retires the older caveat above the R2-04 E0 counters — "the
   renderer does mutate loaded fighter data" is true of fighter data at large
   and false of an AObj16 animation script. Any future sharing design at a seam
   that *does* let the destination move is licensed by this.
2. **All 301 Mario+Fox animation assets have no external references** —
   `reloc_extern_offset` 0xffff and `extern_file_ids_num` 0, scanned statically
   off the built NitroFS tree with no build and no emulator run. So
   `ndsRelocApplyExternalPointerFixups` takes its early-out for every one of
   them, and there is no cross-file pointer here that could go stale.
3. **The internal fixup list is at most 21 entries** (`PrebakeSlotsMax` 21 over
   the whole match). Twenty-one pointer writes is not a cost, so baking the
   internal fixups per destination — the obvious fallback once zero-copy died —
   is worth approximately nothing and must not be briefed as a lever.

**Taken together this CLOSES `ndsRelocForceLoadFighterAObj16File` as a cost
centre.** After cycle 108's prebake, a cache hit is a mandatory ~2.3 KB memcpy
plus ~21 pointer writes plus bookkeeping. The remaining load-frame premium is
what cycle 107 already attributed and named correctly: **animation
re-evaluation, 158,393/frame (24.3%), which is real gameplay work, not port
overhead.** That is where the next attempt on the `SINT` tail belongs — as
specialization or a lower update rate, not as another caching layer.

### The dormant-flag seam is EXHAUSTED — audited, cycle 105, no build

Prompted by the "audit the 0 flags" rule. **The Makefile's `?= 0` defaults are
not the shipped configuration** and reading them as such is how this audit almost
spent two builds. Compared `builds/build-c105-anim-cand/nds_build_config.h`
against every `^[A-Z0-9_]+ \?= 0` in the Makefile: **41 flags are overridden**,
and every large measured lever is already ON — `NDS_R2_CUBIC_FIXED` (60,509
ticks/frame), `NDS_R2_STAGE_DIRECT`, `NDS_R2_STAGE_VIEWPROJ` (54,901),
`NDS_R2_STAGE_PREFLIGHT`, `NDS_R2_FIGHTER_HW_MTX` (−17,600),
`NDS_R2_FIGHTER_HW_LIGHT` (ceiling 53,760), `NDS_R2_FIGHTER_RUN_MEMO` (~45,900),
`NDS_R2_FIGHTER_MTX_DIRECT`, `NDS_R2_FIGHTER_SHUFFLE_FOLD`, the Task 16 float
set. A `build-c106-cubic` arm was built before this was checked and is a **null
build** — same `nds_build_config.h`, same `fake_heap_start 0x0228c004`; it was
caught before its measuring run, not after.

Of the 71 still off, all but a handful are censuses, probes, falsifiers or
lab-only suppressors. The real remainder, each already carrying its own gate:
`NDS_TASK51_STAGE_NATIVE` (**now refuted on performance**, cycle 109: P50
+13,376, `STG` +2,368, 88 frames lost from 30 FPS), `NDS_DREAMLAND_DS_MESH`
(**not "needs the owner's visual A/B" — it HAD one and failed it**, Task 62,
reverted 2026-07-25, and two checkers enforce `=0`), `NDS_R2_SHIELD_QUAD` (**the Makefile itself asks for this re-price**: the owner
bought the model route at "36k p95" and that figure came off a 128-frame window,
which the whole-match rule says is unusable). **Do not re-audit the flag list.**

### G3 (original row, Boundary-derived) — the effect packet path

Build the GX packet per unique effect display list **at match load**, reserve
patch offsets for matrix and dynamic colour words, patch per frame, submit.
No re-parse, no per-list config rebuild, no per-command dispatch.

Design constraints (all standing law, see charter §3):
- §3.11 — fixed arena allocated at match load, sized by a unique-list census
  (1,360 list *instances*/match; count the unique templates first), explicit
  overflow policy, exercised in a soak. No gameplay-time heap allocation.
- §3.12 — packets are re-derived at scene entry; nothing keyed on pointers
  that survive a scene boundary.
- Byte-cost table + boot probe before the first measuring run (G2's headroom
  is the budget it spends from).
- Dream Land water frozen at frame 0; same geometry/textures/materials — the
  effect models are a closed owner-approved set. A change that alters a
  visible pixel needs the owner.

**Iteration protocol — one build, one run per decision.** Ship both routes in
ONE tickhud binary behind a gdb-settable runtime route (the
`NDS_R2_STAGE_ROUTE_PROBE` pattern): route 0 = interpreter, route 1 = packets.
Because the cost is a **per-list constant**, ticks/list from a few stops is a
valid iteration metric — flip the route mid-run and read both constants from
the same run, same frames, zero placement noise, zero extra builds. The
whole-match sampling run is reserved for the KEEP decision and re-baseline.
Success at iteration scale: packet-route ticks/list ≪ 83,632 on the gate arm
(≪ 102,730 on Boundary) — the submit-only residue should be a few thousand.

**There is no longer a gate-scale success criterion for this row.** The prior
one ("P95 moves by most of the ~315K recoverable in both arms") was written from
the unlabelled Boundary diagnosis and is refuted: on the gate arm, removing
*100%* of effect submits leaves WORK-H P95 at 1,536,768–1,578,333 against the
1,120,380 budget — a residual gap of 416,388–457,953. **G3 cannot close the gate
alone.** It remains a real Boundary-arm win and a partial gate-arm win; it is no
longer the lane's answer, and G2's ≥32 KB exit exists only to fund it.

### G4 — Re-baseline and pick the next lever from the residue

After G3 KEEP: bank new whole-match baselines (both arms — run them
concurrently on two runner slots once the parked calibration row passes).
**The gate decision reads on the both-CPU arm** (owner, 2026-08-05); bank its
load-frame-excluded P95 explicitly — the Boundary clean-frame figure
(~1,056,640, inside the budget) has no banked both-CPU sibling yet. If a
residual gap remains, promote from Parked in this order: the +52,928
regression bisect (largest known flat cost), `Tex` residue on non-effect
paths, then the charter §7 contingency ladder (rate reduction → fidelity →
owner-approved 30 Hz) — never widen the gate.

### FTR LANDED −22,689: three abstractions leave the hot fighter path (cycle 110)

**Four arms, one pre-slice baseline built and measured for the purpose.** The
baseline reads `FTR` mean **385,508**, which is the owner's stated ~385–390K to
the ticket — so the reference is right and the deltas below are real progress,
not a re-anchoring.

| arm | `FTR` mean | Δ | `WORK-H` mean | Δ |
|---|---:|---:|---:|---:|
| pre-slice baseline (`4fc9d79d14~`) | 385,508 | — | 1,062,929 | — |
| + slice 1, emit capture hook | 374,332 | −11,176 | 1,052,509 | −10,420 |
| + slice 2, flat baked compose | 366,597 | −18,911 | 1,044,687 | −18,242 |
| **+ slice 3, `m4x4` intermediates** | **362,819** | **−22,689** | **1,040,085** | **−22,844** |

`FTR` P50 396,032 → 379,328 and P95 399,040 → 382,464 across arms 1–3.
**Every control drifts under ±950 and non-monotonically** across all four arms
(`STG` −538, `SRC` +919, `SINT` −167, `SCPU` +572, `MISC` −264) while `FTR` falls
monotonically by 22,689 — so this is mechanism, not relinking.
`scripts/compare-tick-hud-arms.py` prints this table and states, from
`romSha256`, whether a given pair carries a placement term at all.

Slice 2's arm is exact — **identical `romSha256` in both arms**
(`13B8DF73…`), the `.data` route poked and read back at end of run, 3,951 calls
and **0 rejects**, and every unrelated bucket flat within ±40 (`STG` −20, `SRC`
−25, `SINT` +6, `SCPU` −8). `FTR` and `WORK-H` agree to 87 ticks, so the whole
delta lands in the bucket that owns the change. Slice 1's arm is cross-build, but
−11,176 with the disassembly showing the instructions gone is not a placement
artifact, and the combined −18,911 clears the 14,080 placement term outright.

**Slice 1 — the Task 36 stage-capture hook, deleted from all five fighter emit
loops.** `ndsRendererHardwareWriteVertex16Words`/`…TexCoordWord`/`…ColorWord`
carry a record hook compiled in whenever `NDS_TASK36_HW_COMPOSE == 2`, which the
shipped ROM sets. On the fighter path it can **never fire**: the capture window
is opened by `ndsRendererTask36ReplayCaptureBeginRun` and closed by `…EndRun`,
both inside `ndsRendererCommitNativeStageSegment` bracketing one **stage** run,
and BeginRun faults outright on a non-stage index. The effect-packet capture is
armed the same way around an effect display list. So a stage-capture abstraction
and a diagnostic capture were being tested **once per fighter corner**.

The proof is the disassembly, not the clock: the untextured loop goes **19 → 11
instructions a corner** and the textured **25 → 14**, every stack spill gone
(the maybe-call was forcing the vertex words to memory and then reloading the
loop-end pointer). Per-PC profile data prices exactly the deleted instructions at
**10,892 + 3,231 = 14,123 ticks/frame** on the tick-HUD ROM against a measured
11,176 — 1.26x, which for a cycles estimate is agreement.

**Report the shipped number separately: ~7,300, not 11,176.** The effect-packet
half is `NDS_TICK_HUD`-only, so the instrument carries a hook the published ROM
does not (`sNdsEffectPacketArmed` is absent from
`smash64ds-battle-playable-hwtri.elf`; `sNdsRendererTask36CaptureActive` is
present at the address the hot loop loads). The Task-36-only share is 7,598 +
~1,600 of the 14,123, so scale the measurement by 0.651.

**Slice 2 — the flat baked world compose, and the recorded design was wrong.**
`PrepareNativeOwnerMatrices` asked `BuildDObjWorldMatrix` for each of the 14/18
bindings independently, and because that entry point knows nothing about the
order it is called in it paid for the ignorance every time: a linear-probed hash
lookup on the binding, a walk all the way to the root, one hash probe per
ancestor hunting a prefix somebody already built, and a store per composed step —
`BuildDObjWorldMatrix` self 13,947 + `FindDObjWorldMatrix` 4,385 = **18,332
ticks/frame of machinery** around ~50 local builds a frame whose arithmetic none
of this touches.

The order is not unknown, it is baked. One forward pass over `BindingParents`
composes every world starting from its parent binding's finished matrix, straight
into `sNdsRendererAdapterNativeOwnerModelviews` so the worlds need no second
home — **no new RAM, no duplicate representation**.

**The correction, caught statically before the build: `BindingParents` is the
nearest *bound* ancestor, not the DObj parent.** This board's own recorded
pseudocode said `world[i] = local_i × world[BindingParents[i]]`, and
`generate_nds_native_owners.py:1169-1177` walks `parents[]` past every UNBOUND
joint to build that table. Mario binding 1 is joint 5 whose real parent is the
unbound joint 4; the recorded form would have silently dropped joint 4's local
matrix. The live chain is still walked — just to the parent binding instead of to
the root, one to three joints instead of full depth. Prefix publishing is also
kept, because the generic display-list path shares that hash for effects parented
under fighter joints: **what is deleted is the probing, not the publishing**, and
that is why the win is 7,735 rather than the full 18,332.

**Graduated, not left behind.** With 0 rejects over 3,951 calls the route and its
two counters are deleted and the compose is unconditional, fail-closed to the
per-binding path on any disagreement between the baked table and the live tree.

### The ~331K fighter draw is reconciled: 314,555 ticks/frame in named symbols

`scripts/analyze-fighter-draw-reconciliation.py` (no build, no run — it reads the
c106 census plus the soft-float caller attribution). Idle removed first,
`%non-idle × 1,128,000`, soft float re-attributed to callers, and census-only
instrumentation excluded rather than counted:

| group | ticks/frame | what it is |
|---|---:|---|
| **matrix preparation** | **96,207** | local build 18,245 · split load 15,248 · mul_affine 14,902 · world 13,947 · TraRotRpy 11,916 · mul 9,842 · pair load 7,167 · find 4,385 |
| **production driver** | **54,043** | `ExecuteNativeFighterOwnerProduction` 30,582 · `PrepareProductionRun` 21,205 |
| **emit / GX submission** | **48,115** | untextured 31,684 · textured 11,587 · cross 4,772 |
| **adapter driver** | **44,680** | `DLAllDrawForSlot` 29,841 · `ftDisplayMainDrawDefault` 10,269 |
| **material / shading state** | **35,568** | snapshot 11,656 · shade 8,512 · state delta 8,277 · material 5,603 |
| **fighter parts / params** | **18,711** | `ndsFTParamsInvalidateFighterParts` 15,815 |
| **display contract / plan** | **17,231** | spread over 17 symbols, none above 3,754 |
| **total** | **314,555** | 95% of the owner's ~331K |
| *(excluded: instrumentation)* | *28,049* | debug HUD 15,181 · `ndsFtrPreMaterialCensus` 9,061 · `cpuGetTiming` 3,807 |

**This answers the "is it exhausted" question with a shape, not a verdict.** The
two largest groups are not leaf arithmetic. `ExecuteNativeFighterOwnerProduction`
spreads 26.5M cycles over **709 distinct PCs** with no site above 5.1%, and
`PrepareProductionRun` over 384 with none above 3.7% — that is the signature of
whole-body architecture cost, exactly what the plan claims and what a per-helper
refutation cannot see.

**Three targets are now named with numbers, in order:**

1. **`ndsRendererNativeBindProductionRoot` copies two 64-byte matrices per root**
   (`nds_renderer.c:23897,23905`), 12,422 times a match at **416 cycles a call**
   = **5,958 ticks/frame**, and its own comment says the value is read only by
   the split loader and a flag test. The stage owner already holds
   `sNdsNativeStageOwnerExecution.projection` **as a pointer** — the fighter is
   the one that copies. Half of this is data that has to be read either way, so
   price it at ~3,000 before committing to it.
2. **`ndsRendererLoadHardwareSplitMatrices` is 1,064 cycles a call** over 12,431
   calls, flat across 185 PCs, and **absent from the census's non-mem stall
   ranking** — so it is memory stall, not placement. Slice 3 (below) removes the
   `m4x4` intermediates; the `scaled_modelview` copy that remains can go too by
   writing `MTX_LOAD_4x4`'s 16 words as 12 unscaled plus 4 rescaled.
3. **`ndsFighterMarioFoxDLAllDrawForSlot` is 9,844 bytes with cyc/insn 5.60**,
   the 3rd largest non-mem stall in the build (4,655,284), and **51% of its
   instructions never execute** — 7,108 bytes sit in cold runs of ≥64 bytes.

**But do not cold-split it on that number — most of it is the instrument.** The
largest cold run is 1,848 bytes of `ndsFighterDrawPlanVerify`, which is inside
`#if NDS_TICK_HUD` behind `gNdsFtrPlanVerify != 0`, so it **does not exist in the
shipped ROM** — `smash64ds-battle-playable-hwtri.elf` has the same function at
9,680 bytes, 164 short of the profiled build's 9,844.
`RestoreNativeOwnerMaterialTextureIds` is already outlined there as
`.part.0.constprop.0` (156 B). What is left to win by marking arms `cold` is
several hundred bytes, not 7,108. **This is the addr2line trap in a new place:
the names were right and the shipped relevance was not.** Check the shipped ELF
before costing any placement work off a profiled-build census.

### The next architecture is the per-run descriptor, and it is 89,611 ticks/frame

With matrix preparation now the smaller half of what it was, the largest
untouched block is **production driver 54,043 + material/shading state 35,568 =
89,611 ticks/frame**, spent over ~30–37 runs a frame — roughly **2,400 ticks per
run of setup**. `ndsRendererNativePrepareProductionRun` alone is 18.4M cycles
over **384 distinct PCs with no site above 3.7%**, which is what per-run policy
re-derivation looks like: texture params, poly format, UV scale and vertex flags
resolved from live state every frame for a run whose descriptor is immutable.

That is exactly the plan's "AOT compact GX-facing run descriptors", and unlike
the emit stream it needs **no new RAM**: the run's immutable fields already exist
in `sNdsNativeFighter*` tables, and what the runtime recomputes is the *binding*
of those fields to the current texture/material state — which changes only when
the material does. The shape is a per-run descriptor validated once per material
epoch instead of once per run per frame, with the epoch counter as the key.

**Sizing note for whoever takes it:** the emit half is close to its floor.
Untextured emit is now 11 instructions and 3 GX FIFO words a corner over 537,780
corners a match (51.1 cycles a corner before slice 1). Going below that needs a
pre-packed command stream DMA'd to `GFX_FIFO` — the stage path already runs at
DMA's ~2 cycles/word floor against the fighter's ~17 — but at 3.5 words a corner
that stream is ~19–26 KB of main RAM, against `gSYTaskmanGeneralHeap`'s ~9,368 B
of slack over the anim cache's `KEEP_FREE`. **RAM is the blocker there, not the
mechanism**, so it needs something freed first.

**Slice 3 — MEASURED −3,778.** `NDSRendererMatrix20p12` is `s32 m[4][4]` and
libnds' `m4x4` is `int m[16]`, both row-major and both 64 bytes:
`ndsRendererCopyMtx20p12ToM4x4` was writing element i to element i. Both per-root
loaders now hand `glLoadMatrix4x4` the matrix directly through a
`_Static_assert`-guarded accessor, deleting three of the four 64-byte
intermediates and 128 bytes of stack traffic a call. On its own it is under the
placement term, which is exactly why it shipped **with** the other two rather
than as its own arm — the cumulative −22,689 is what clears the floor.
The fourth intermediate is `scaled_modelview`, still needed because the bottom
row is rescaled; writing `MTX_LOAD_4x4`'s 16 words as 12 unscaled plus 4 rescaled
removes it, but that bypasses the `glLoadMatrix4x4` wrapper the Task 29/34/49
census records through, so it needs the wrapper preserved by hand.

**Verified, not just measured.** `verify-all.ps1 -Profile Boundary` passes with
the flat compose unconditional, and `artifacts/visibility/latest.png` shows both
fighters fully articulated on Dream Land — a wrong world matrix is exactly what
destroys that, so the screenshot is the check that matters here.

### Cycle 110 slices 4–7: FTR −37,640, ALL −67,718, and one refuted arm

Continuing the same lane. Every arm 1600 samples, `NDS_R2_BOTH_CPU` off, DLDI on.

| arm | FTR mean | ALL mean | WORK mean | what changed |
|---|---:|---:|---:|---|
| c110 baseline | 385,508 | 1,285,825 | 1,075,918 | pre-slice |
| slice 3 | 362,819 | 1,252,041 | 1,075,918 | slices 1–3 (committed) |
| slice 5 | 349,955 | 1,238,289 | 1,063,906 | +4, +5 |
| slice 6 | 340,691 | 1,225,916 | 1,048,038 | **counter gate — REFUTED** |
| slice 6b | 348,069 | 1,237,683 | 1,061,493 | counters restored |
| slice 7 | **347,868** | **1,218,107** | 1,037,278 | DTCM summary + flat parts |

**Slice 5 — MEASURED −11,683**, against a 5,958 prediction.
`ndsRendererNativeBindProductionRoot` copied the caller's projection and
modelview into the traversal state: two 64-byte struct copies, 12,422
executions, 416 cycles a call. With `NDS_R2_SHADE_SKIP_SOFT_LIGHT` the
production path has no reader of either field, so both were dead stores. The
split loader now takes the caller's matrices directly, which removed the copy it
was making of the copy — that second-order copy is the extra 5,725.

**Slice 4 — kept, honestly inconclusive at −1,181** against a −6,500 prediction,
under the placement floor. It was **mis-sized off the wrong revision**: the c106
profile ELF was built at `1b467da` and I resolved its line numbers against HEAD,
~85 lines adrift, so the biggest row landed on a blank line and the field I
targeted was free. `scripts/analyze-symbol-line-profile.py` now reads
`NDS_TASK10_GIT_SHORT` out of the build's own `nds_build_config.h` and quotes
every line from that commit, so the mistake cannot recur.

**Slice 6 — the counter gate is REFUTED by the gate itself.** Compiling out the
`sNdsRendererRuntimeFrameSummary` per-call counters (matrix load, batch
begin/reuse/end, texture prepare/reuse) was worth **FTR −7,378 and STG −2,776**,
an order of magnitude past the ~1,300 the per-line profile showed — the profile
only sees the two symbols the lines live in, and *every* hardware batch on every
path pays them. It is not available: `verify-all.ps1 -Profile Boundary` also
runs `verify-battle-mariofox-gcrunall-loop-harness.ps1`, which asserts exact
batch and texture-prepare accounting off those globals, and it failed with
*"Canonical realtime HW build drifted from exact source-weapon-aware batch and
texture-prepare accounting"*. **`-Profile Boundary -List` prints one row and the
run executes about six checks** — grepping `scripts/` is not how you find out
who depends on a global.

**Slice 7 — the same 10,154 recovered without touching the evidence.**
`sNdsRendererRuntimeFrameSummary` is 108 bytes; it now lives in `.dtcm.bss` at
`0x02ff21e0`. DTCM is single-cycle and outside the 4 KB D-cache, so the counters
keep counting and stop paying main-memory latency *and* stop evicting fighter
data. Nothing DMAs it (DMA cannot read DTCM).

**Slice 7 also flattened the parts-invalidation walk — and it is NOT an FTR
lever.** `ndsFTParamsInvalidateFighterParts` is 15,815 census ticks/frame over
159,748 joint visits: 86 cycles a joint for two word writes, because
`user_data.p`, `transform_update_mode`, `unk_dobjtrans_word`, `child` and
`sib_next` are five separate cache lines. The subtree is now preordered once
into a flat `FTParts*` array keyed on `(root, gNdsTaskmanHeapGeneration)` —
sound because that generation is bumped at the two taskman-heap rewind
primitives, the only way a live fighter tree can be rebuilt. **But the tick HUD
charges that walk to `SRC`/`SINT`, not `FTR`.** Slice 7's arm is FTR −201 and
WORK −24,215; the win is real and it is in the gameplay buckets.

> **Read this before sizing the next FTR slice.** The census→bucket mapping in
> the reconciliation below is *not* the tick-HUD bucket mapping. "fighter parts
> / params 18,711" is outside `FTR`. Only the emit, production-driver, matrix,
> material and display-contract groups are inside it. A census row is not an
> FTR row until the arm proves it.

Two instrument facts banked from this cycle. First, **`-RingDump` and per-frame
stops agree to the tick**: `c110-slice6b` and `c110-slice6b-ring` are the same
`romSha256` and report FTR 348,069 both ways, STG within 2. Every arm in this
lane is comparable regardless of mode. Second, **a faster ROM breaks per-frame
sampling**: slice 7 tripped the repeated-presented-frame guard 315 times in 1600
because the 60 Hz loop now fits two iterations inside one presented frame more
often. Ring dumps saw 5, all payload-DIFFERS (never a stale read). Use
`-RingDump -AllowRepeatedFrames` from here.

### Slice 8: the fighter material block is a constant, and a census already knew

**FTR 347,868 → 333,322, −14,546.** `WORK` −14,066, `ALL` −6,509. Boundary
passes. Cumulative for cycle 110: **FTR 385,508 → 333,322, −52,186**;
`ALL` −74,227.

`ndsRendererAdapterBuildNativeMaterialSnapshot` reconstructs a 100-byte N64
display-list material command block out of a pointer-chased `MObj` — ~761 cycles
a call, about twelve times a frame, 13,176 ticks/frame, of which **2,124 is the
single `mobj->sub.flags` load missing cache at 139 cycles an execution**. It is
a pure function of `mobj->sub` plus `texture_id_curr/next`, `lfrac` and
`palette_id`.

**The measurement that decided it needed no build and no new code.** Cycle 98
left an invariance census in the tree (`ndsFtrPreMaterialCensus`, hashes the
snapshot and compares against the previous one for the same MObj) and nobody had
read it. Off the shipped tick-HUD ROM with `-ExtraGlobals`:

```
gNdsFtrPreMatCalls=20,100  Same=20,069  Variant=0  New=31  Evict=0
```

**Zero variants.** Thirty-one distinct materials, each built once and then
rebuilt identically twenty thousand times. That is not a cache question, it is a
constant being recomputed — so the skip is a 12-byte `(MObj, heap generation,
FNV-1a of the complete input set)` key per materials-array slot. Complete
coverage rather than the fields I believed could animate: the builder reads
nothing else, so equal inputs is equal output by construction, with only a 2^-32
collision to argue about. The heap generation is in the key because MObj
pointers are taskman-arena addresses that a scene rewind reuses. Skipping the
build's write-back of `texture_id_curr/next` is safe because the stored hash is
taken *after* it — a match means the write would store what is already there.

**Two things this slice got wrong first, both worth keeping.**

*The engagement counter was not optional.* The first arm measured FTR −13,587
while `gNdsFtrPreMatCalls` did not move, which is consistent with both "the skip
fires and the census counts a different call site" and "the skip never fires and
this is placement noise". Only a purpose-built `gNdsR2MatKeySkip`/`KeyBuild`
pair settled it: **28,786 skips against 30,606 builds**, and the second arm then
reproduced the win with `WORK` moving too (−14,066, where the first arm's `WORK`
was −6,470 with buckets shuffling). A change whose engagement you cannot read is
not measured, it is guessed at.

*Narrowing the hash is REFUTED.* Six of `MObjSub`'s thirty words are read by
nothing in the builder (`unk48`, `unk4C`, `unk68..unk74`), so hashing them looked
like the reason only 48.5% of calls skip. Restricting the hash to the read set
returned **bit-identical counters — 28,786 and 30,606 again** — for +1,155. The
rebuilds are `keys[count].mobj != mobj`: the materials array is indexed by
(selected-root slot, chain position) and **which DObj lands in slot *i* rotates
between frames**, so about half the lookups find the right block under the wrong
index. Reverted to the complete hash, which measures the same and needs no field
audit to stay correct.

### Slice 9: the other half was free, and the counter said which half it was

**FTR 340,916 → 329,034, −11,882**, `WORK` −10,958. Boundary passes. Cumulative
for cycle 110: **FTR 385,508 → 329,034, −56,474**.

Slice 8 left 51.5% of material lookups rebuilding and I had two candidate
explanations. Rather than guess again — the narrow-hash guess had just cost a
build — I split the miss counter by reason and ran it. The answer was not close:

```
gNdsR2MatKeySkip=28,786  gNdsR2MatKeyBuild=30,606
gNdsR2MatKeyMissIdentity=30,606  gNdsR2MatKeyMissInputs=0
```

**Every rebuild was an identity miss. Not one was an input change.** The block
was always correct and always filed under the wrong row, because the materials
array was indexed by the selected-root index `i`, which rotates between frames,
while the material `DObj` is stable for the fighter's life. So hash the DObj to
a row and keep it there (linear probing; distinct DObjs get distinct rows; two
roots that genuinely share a material DObj share its row and its block, which is
right; cleared on a taskman-heap rewind because these are arena pointers). 36
bytes, and `root->materials` moves out of the once-primed invariants into the
per-frame build, since which row a root points at is now a per-frame fact.

**After:** `Skip=59,362  Build=30  MissIdentity=30  MissInputs=0`. The fighter
material command block is now constructed **thirty times in a sixty-second
match** — once per distinct material — against 30,606 before and 59,392 with no
key at all. That is the whole of `ndsRendererAdapterBuildNativeMaterialSnapshot`
deleted from the frame, and it is deleted rather than cached: the block that
survives is the one the builder would have produced, proven by a key over its
complete input set.

The lesson is the counter, not the fix. Two consecutive slices were sized by
reasoning about *why* a skip missed; the first reasoning was wrong and cost a
build, the second was right only because a five-line counter answered it in one
run. **Split the miss counter by reason whenever a skip rate is below what the
invariance census predicts.**

### Slice 10: the display-contract event had two halves with opposite lifetimes

**FTR 329,034 → 323,871, −5,163**, `WORK` −7,400. Boundary passes. Cumulative
for cycle 110: **FTR 385,508 → 323,871, −61,637**; `ALL` −76,630.

`NDSFighterDisplayContractEvent` carried its four DObj/Gfx pointers — read by
three tight per-root loops in the same pass that walks the collection — next to
its render preamble, read once per root by
`ndsRendererAdapterBuildNativeProductionInputs` a pass later, after the matrix
and material work has evicted it. 56 bytes together, so the halves landed on
different lines and 32 events spanned 1,792 of a 4 KB D-cache.

c106 priced the eviction exactly: **2,966** ticks/frame on
`root->preamble.geometry_mode = event->geometry_mode` and **1,759** on
`if (event->light_valid)` — about 110 cycles an event of pure miss for what
reads like six field copies.

The preamble now lives in its own array **in the consumer's own
`NDSRendererNativeFighterPreamble` layout, written by the producer** in
`ndsFighterDisplayContractSelectDL` — including the flags word both build sites
used to derive, identical arithmetic one pass earlier, into a line that is hot
because the event was just written. The event is 16 bytes; 512 of pointers plus
768 of preamble instead of 1,792 interleaved. Each of the readers becomes a
24-byte struct copy or one field out of a dense array. **Five readers, not the
four static analysis found** — the fifth seeds `persistent_stats` from
`light_valid`, which is now the preamble's `LIGHT_VALID` bit. The build caught
it; a grep for `event->` had not.

### Slice 11: FLAT. The 3,429-tick row was not 3,429 ticks of removable work

**FTR 323,871 → 324,013, +142. Nothing was bought.** Boundary passes; kept only
because it is strictly less code (one function and one chain traversal gone).

Two changes, and the counters refuted both premises before the ticks did.

`ndsRendererAdapterSaveNativeMaterialTextureIds` was a second walk of the same
MObj chain the prepare walk already traverses, reading the same two fields, and
the c110 profile charged it **3,429** ticks/frame. It is now folded into
`ndsRendererAdapterPrepareNativeMaterials` as two `s32 *` out-parameters, with
`*out_count` set on every `return FALSE` so a partial walk still rolls back
exactly what it mutated. **The recovered cost was ~0**, and the arithmetic says
why: `gNdsR2MatKeySkip + gNdsR2MatKeyBuild` is 59,392 over 1,600 frames, i.e.
**~37 chain nodes a frame**. Folding removed the call and the pointer chase, not
the four loads and stores — perhaps 15 cycles × 37 ≈ 550, under the floor.
**A profiler row for a leaf that is one line of a loop prices the loop iteration,
not the work you can delete by inlining it.** Price the delta, not the row.

Second: the material-row hash moved from `(ptr >> 4) & 31` to multiplicative
`(ptr * 2654435761u) >> 27`, because slice 8 measured that shift hash at a 48%
miss rate (28,786 skip / 30,606 build) and it read like row collisions. **The
slice-10 baseline already ran 59,362 skip / 30 build on the shift hash** — the
counters are bit-identical across the two ROMs, so the collisions slices 9 and
10 removed were never the hash's. Kept anyway at equal cost: the 48% miss proved
the shift hash is fragile to whatever the allocator does next, and the multiply
is one instruction either way.

### Slice 12: the I-cache, not the arithmetic — 73.6% of the driver never runs

**FTR 324,013 → 318,266, −5,747**, `ALL` −5,797, `WORK` −5,061, `WORK-H` P95
−11,584. Boundary passes. **No logic changed at all** — this moves code.

`ndsFighterMarioFoxDLAllDrawForSlot.constprop.0` is the largest non-idle symbol
in the ROM (112.5M cycles, 2.91%) at **4.21 cycles per instruction** — a
function waiting on memory. Its 10,708 bytes retired only **1,414 distinct
PCs**. Diffing the executed PC set against `objdump` says **7,880 bytes (73.6%)
never execute**, in 69 runs. The ARM946E-S I-cache is **8 KB**.

Two inlined blocks held most of it: `ndsRendererAdapterPrimeProductionInputs`
(~1,640 B, runs **once per match**) and the per-binding world build inside
`ndsRendererAdapterPrepareNativeOwnerMatrices` (~3,150 B with
`ndsRendererAdapterBuildDObjWorldMatrix` inlined into it, and it declined **0
times in 49,422 binding visits**). Both are now `noinline, cold, Os`.
`flat_worlds` is decided once before the loop, so hoisting the test out is a
pure transformation — and it is what lets the fallback leave the function.

`SRC` −3,011 and `GCRA` −2,906 came along, which is the tell that the lever was
the shared I-cache and not anything local to the draw.

**The tooling for this is now a two-command recipe** — `task37_census.py
--pc-detail SYMBOL` for the executed PC set, then the cold-run diff against
`objdump`, then `addr2line` on each run's first address. Run it on any hot
symbol over ~4 KB before optimizing its arithmetic.

### Slice 13: the DObj world cache had ZERO readers

**FTR 318,266 → 317,247, −1,019.** Boundary passes; fighter pixel counts
identical. Small, but it deletes work that was provably dead.

The c112 census settles what grep could not:
`ndsRendererAdapterFindDObjWorldMatrix` **0 cycles** and
`ndsRendererAdapterBuildDObjWorldMatrix` **0 cycles** over a whole match, while
`ndsRendererAdapterStoreDObjWorldMatrix.part.0` burns **4,744,740**. Every one
of those stores fed a cache with no reader, and streamed ~4 KB a frame of write
traffic through a **4 KB** D-cache. The store is gone from the flat compose; the
cache and its fail-closed filler are untouched.

With no reader, the composed matrices no longer have to be world-space, so the
flat compose now seeds from the **camera** instead of the identity. That deletes
the `world * camera` that ran once per binding (~31 a frame of the 55.5
`ndsRendererMtxMulAffine20p12` calls a frame, 695 cycles each) and costs
nothing: the first multiply of a root chain used to be against the identity.
The hitlag shuffle folds the same way — `world * T * camera` reassociates to
`world * (T * camera)`, one 4x4 a frame instead of a row-3 add per binding.
Fixed-point reassociation is not bit-exact; these matrices reach GX and nothing
else.

**Why only −1,019 when ~12,000 of multiplies left?** The seed setup added **116
bytes** to a function already over the I-cache. Slice 12's lesson, charged
again at the till.

### Slice 14: REFUTED — outlining code that RUNS is a different lever

`noinline` on `ndsRendererAdapterPrepareNativeOwnerMatrices` and
`ndsRendererAdapterBuildNativeProductionInputs`, on the theory that shrinking
the driver toward 8 KB is good regardless of what moves. The driver did shrink
**10,528 → 9,612 bytes** and **FTR rose 2,192**. Reverted; the revert rebuilt
the slice-13 ROM **bit-identically** (`sha B7F2493F0C13`, every bucket to the
tick), which is also the cleanest proof yet that this sampler is deterministic.

**Slice 12 won by removing bytes with ZERO executions. Slice 14 lost by removing
bytes with many.** Size is not the metric; executed-vs-resident is. Both
functions now carry a comment saying so, because the next reader will otherwise
try it again.

### Slice 15: the driver fits the I-cache — 10,528 → 7,516 bytes

**FTR 317,247 → 313,421, −3,826.** `WORK` −11,561, **`WORK-H` P95 −21,248**,
`ALL` −7,308. Boundary passes. Again: no logic changed.

Same recipe as slice 12, run again on the post-slice-12 build, with the cold-run
attribution improved to sample **four points inside each run** instead of only
its first address — one run had been credited entirely to `ndsFtrPreWalkCensus`
when three quarters of it was `ndsFighterDLAllDrawAccumulateStats`.

Three more never-executed bodies, all inlined into the driver:

| body | why it never runs |
|---|---|
| `ndsFighterDLAllDrawAccumulateStats` | needs `detailed_output`, never set |
| `ndsRendererAdapterPrepareNativeOwnerHierarchy` + `…GetHierarchyCameraMatrices` | only `FAST_RUN_NATIVE_FIGHTERS`; live mode is `…OWNER_PRODUCTION` |
| `ndsRendererAdapterValidateNativeOwnerCached` | a plan hit skips it, and the plan hits every frame |

All three are `noinline, cold, Os` — still live, still correct, just no longer
renting I-cache lines from the code that runs.

**The driver is 7,516 bytes: under the ARM946E-S 8 KB I-cache for the first
time.** Cumulative for cycle 110: **FTR 385,508 → 313,421, −72,087**.

### Slice 16: REFUTED — cold BYTES are not a never-entered BODY

`ndsRendererAdapterBuildDObjXObjMatrix` is **72% never-executed** (1,758 of
2,440 bytes), so its four alternate matrix kinds —
`GetDObjVectorTracks`, `BuildFighterPartsMtx`, `BuildBillboardMtx`,
`BuildRecalcLocalMtx` — went `noinline, cold, Os`. The function shrank
**2,440 → 964 bytes** and **FTR rose 14,963**, `WORK` +14,901. Reverted.

`ndsRendererAdapterBuildFighterTraRotRpyDirect20p12`, checked in the same pass,
is **97.9% hot** — nothing to outline there at all.

**The discriminator is entry count, not cold-byte count.** Slices 12 and 15 won
because their bodies are never *entered*: a flag that is never set
(`detailed_output`), a mode that is never selected (`FAST_RUN_NATIVE_FIGHTERS`),
a branch that never fires (`flat_worlds == FALSE`), a once-per-match primer.
Slice 16's helpers ARE entered — every joint calls `GetDObjVectorTracks`, which
then returns early. Outlining that turns a predicted fall-through into a call
plus a guaranteed I-cache miss, ~69 times a frame.

**Before applying the cold recipe, ask whether the ENTRY is cold, not whether
the body is.** A cold run that starts mid-function is an early return, not dead
code.

### Slice 17: REFUTED — and it CORRECTS slice 13's stated mechanism

Two things, one build.

**The material key narrowed correctly and measured worse.** `MObjSub` is 120
bytes and the builder reads most of it, so the cycle-109 "narrow to the read
set" refutation stands — the read set IS the struct. But only **nine** words
*animate*, and they are contiguous: `MObj+0x58..0x6F` (the five colour tracks
`gcPlayMObjMatAnim` writes plus the prim level pair) and `MObj+0x80..0x8B`
(`texture_id_curr/next`, `lfrac`, `palette_id`). Nine words, two cache lines,
against thirty-four words and five. Shipped with a fail-closed half: the whole
34-word hash re-checked every fourth frame, `gNdsR2MatKeyMissStatic` counting
disagreements. **FTR +3,342** — a volatile frame-counter load and a branch per
entry, plus a 16-byte key. Reverted to the narrow hash alone (slice 18), and
that scaffold's result is now a comment: **~14,848 full checks, 0
disagreements.**

**And the entry PC refuted slice 13's story.** `ndsRendererMtxMulAffine20p12`
executed its prologue **88,758** times in c112 and **88,825** in c115 — slice 13
deleted no multiplies at all. The per-binding `world * camera` it "removed" was
never running, because `NDS_R2_FIGHTER_HW_MTX` hands the camera to the hardware
and `camera_modelview_valid` is FALSE on this path. (The visual gate agrees: had
the seed really changed from identity to camera, the image would not have been
pixel-stable.) Slice 13's −1,019 is the dead world-cache stores, full stop; its
seed rework is behaviour-preserving scaffolding that slice 18 then made pay.

**`docs/optimization/` memory says "Entry PC gives exact call counts" and I did
not use it before spending the build.** A symbol total divided by a guessed
per-call cost is not a call count.

### Slice 18: don't fold the base in until a joint contributes — −10,804

**FTR 313,421 → 302,617, −10,804.** `WORK` −11,014, `WORK-H` −10,909,
`ALL` −4,206. Boundary passes, fighter pixels stable.

All **55.5** `ndsRendererMtxMulAffine20p12` calls a frame come from
`ndsRendererAdapterComposeOwnerWorldsFlat`, at **687 cycles** each. The loop used
to seed `out` from its base — the parent binding's world, or the identity for a
root — and then multiply every joint into it. So one call per binding was
*copy the base in, then multiply the base straight back out*.

Now the base is not folded in until the first joint that actually contributes:
that joint multiplies against the base directly instead of against a copy of it,
and when the base is the identity the multiply disappears entirely. A binding
whose joints all decline still gets `out = base`, as before.

Carries slice 17's narrowed material key too, without its scaffold.

### Slices 19–21: three refutations that bound where cycle 110 stops

**Slice 19/19b — the cold recipe is SPENT.** `BuildNativeHierarchyInputs` (598 B)
and `ndsFighterDrawPlanVerify` are both never-entered, exactly the pattern that
won slices 12 and 15, and marking them cold measured **FTR +4,959** on its own
(19b). The driver was already **7,516 bytes, under the 8 KB I-cache**; once it
fits, further shrinking buys nothing and the outlining still costs. **Slices 12
and 15 were not "shrink the function" wins, they were "get it under the cache"
wins**, and that is a threshold, not a gradient.

**Slice 20 — build the first joint straight into the output. KEPT.**
`FTR +289` (flat) but **`WORK-H` P95 −10,688, `WORK` P95 −12,288, `SRC` P95
−4,160**. One fewer 64-byte temporary and one fewer multiply per binding; the
mean is at the noise floor and the tail is not.

**Slice 21 — the E23 projection skip stays refuted.** Only the modelview half of
the fighter split load is per-root; E22 measured 29 of 30 loads re-pushing a
byte-identical projection, and E23's −3,008 was discarded as under the placement
floor. Re-testing it looked justified by the standing rule that repeatable gains
are kept and accumulated — but a content-keyed skip measures **FTR +4,566**. A
64-byte `memcmp` costs more than the eighteen FIFO writes it saves; E23's revert
was right and its −3,008 was probably placement. **The rule "keep every gain" does
not license re-running a refuted experiment without new evidence.**

**Cycle 110 total: FTR 385,508 → 302,906, −82,602 (21.4%).**

### Slice 22 (ARCHITECTURAL): the prepared dense UVs are immutable state

**FTR 302,906 → 301,162, −1,744.** Boundary passes. Engagement: **Skip 30,189,
Build 15** — the loop runs fifteen times in a match instead of 30,204.

First step of Requirement 3 proper: not a faster interpreter, a piece of
per-frame runtime work replaced by immutable data. In the Requirement 5 format:

| | |
|---|---|
| **runtime work deleted** | the per-run UV preparation loop, 30,204 executions/match → 15 |
| **immutable replacement** | `sNdsNativeFighterPreparedDense` itself — it already existed in DTCM and already held the answer |
| **remaining dynamic state** | the resolved texture's scale/origin/offset; the loop re-runs when they move |
| **RAM/text** | 67 × 24 B stamps + 67 valid bytes ≈ 1.7 KB bss; compare is five words against a hot traversal state |
| **working set** | −154 DTCM writes and −3 baked-table loads per textured run, every frame |
| **verification** | Boundary; `segment0_prepared_dense_checksum` **byte-identical 0xf1c6fadc** across slices 13/15/20/22 |
| **removable next** | move the fill to load time and `RunFirstUnique`/`UniqueCount`/`UniqueDense` leave the hot path's cache footprint |

**The proof came from instrumentation that was already in the tree.** Building
`NDS_R2_FIGHTER_RUN_PROOF=2` and reading four counters over a whole match:
**246,736 UV writes producing 106 distinct dense vertices, `gNdsR2UvChangeCount`
= 0, `gNdsR2UvOutOfRange` = 0.** Not one write in a match ever changed a value,
and the proof array covered every id. 154 writes a frame to re-derive a
constant. **Read the counters a previous cycle left before designing anything.**

The stamp carries `gNdsTaskmanHeapGeneration`. A P1 restart rewinds the taskman
heap and could put a different dense table behind the same run index with the
same texture metrics; without the fence the stamp would skip on metrics that no
longer describe the vertices.

### The run preparer, split — whole-pipeline attribution for Requirement 6

`NDS_R2_FIGHTER_RUN_PROOF=2`, whole match, **83.1 run preparations a frame**
(instrumented totals; the timing calls roughly double the function, so read the
*shares*, not the absolutes):

| phase | tk/frame | share | verdict |
|---|---:|---:|---|
| Validate | 9,119 | 15.0% | never rejects (`rej=0`); a stamp would trade compares for compares |
| TexPrep | 15,626 | 25.7% | memo covers it — Hit 17,976 / Miss 9 / **Stale 0** |
| TexReuse | 1,584 | 2.6% | the memo path, 38 cyc/call against 376 |
| **Uv** | **16,162** | **26.6%** | **DELETED, slice 22** |
| Tail | 18,224 | 30.0% | publishes `texture_prepare_*` the emit reads in the same call |

**~~Next architectural slice, named and blocked:~~ UNBLOCKED, MEASURED AND
REFUTED 2026-08-14 — DO NOT BUILD THE STATE-SPAN BAKE.**
`artifacts/performance/2026-08-14_fighter-state-delta-census/FINDINGS.md`.

The census builds now. It overflowed ITCM by **616** bytes on this tree, not 64,
and `NDS_R2_CENSUS_EVICTED_CODE` evicts `ndsRendererScanList` (7,728 B, the
generic interpreter the native path exists to replace) **for the census arm
only** — the shipped ROM rebuilt bit-identical at `2015FBD1…`. Read COUNTS from
that arm, never ticks: the eviction changes instruction fetch.

**The 63.9% repeat figure is not redundancy, and the two counters say so
together.** Whole match, 500-frame steady window: 189.43 delta applications a
frame, `gNdsR2SpanDeltaRepeats` **63.9%** — but `gNdsR2SpanIdenticalOperands`
only **7.2%**. The fighter is not re-writing the same values; it genuinely
cycles distinct render states, so a "repeat" is a revisit, not elidable work.
Grading this lane on the repeat counter alone would have bought a bake, a
generator, a checker and an ABI for nothing.

A host model (`scripts/fighters/analyze_fighter_state_epochs.py` — no build, no
ROM) reproduces the frame from the static tables: 196 applications vs 189.43
measured, 49 epochs vs 47.09, and **repeat share 64.3% vs 63.9%, derived
independently**. So the static walk IS the frame — every root drawn once, in
order. On that model only **34 of 196 applications (17.3%) write what is already
there**: TEXTURE 65.8%, COMBINE 20.5%, **every other effect 0%**, because a
material lands between almost every pair of tile-state movers and poisons the
slot. At 47.6 tk an application that is **~1,560 tk/frame**, inside the ±5,376
cross-build floor.

**By-product:** the fighter occupies only **21 distinct resolved states** at a
run boundary across 49 epochs and 32 roots (Mario 8, Fox 15), and epoch ranges
are disjoint so a per-epoch `u8` state id is exact by construction.

### The per-run AOT descriptor is REFUTED TOO — 2026-08-14, on soundness

`artifacts/performance/2026-08-14_fighter-run-descriptor/FINDINGS.md`. **Do not
build it. Do not re-open it without reading §1.**

**The per-run predicate is validating the SCENE's contract, not the fighter's,
so a fighter generator cannot certify it.** Of 70 state deltas, exactly two
touch OTHERMODE and **both are `SETOTHERMODE_H`** — `ndsRendererRecordOtherMode`
writes `othermode_l` only for `SETOTHERMODE_L`/`RDPSETOTHERMODE`, so **no
fighter delta writes `othermode_l` anywhere**. Exactly two touch GEOMETRY and
**neither is a full replacement** (`0xd9fffbff`/`0xd9ffffff` masks preserve
ZBUFFER, LIGHTING, FOG and both TEXTURE_GEN bits). So six of the predicate's ten
conditions — ZBUFFER\|LIGHTING, FOG/TEXGEN, cull, alpha-compare, z-mode, z-source
— read state established **outside the fighter draw**. `rej=0` because the scene
always sets it up right; "always has" is not a proof a generator can emit, and
asserting it would be exactly the deleted-safety the brief forbade.

Certifiable: the **combine pair only**, and it certifies cleanly — resolved
combine equals its policy family's combine at **all 49** run-bearing epochs, 0
mismatches. But the predicate-relevant distinct-value count is then **4, one per
family**, so the "compact AOT descriptor with a per-epoch `state_id`" the brief
asks for **already exists** as `sNdsNativeFighterEpochDirectPolicy[49]` ->
`sNdsNativeFighterDirectPolicies[4]`. Retiring 2 of 10 conditions is
**~640 tk/frame**.

**BOUNDARY CORRECTION, and it invalidated a mismatch this lane nearly chased:**
runs execute after the **after**-span (before-span -> material -> after-span ->
runs; the E34 hook's comment says so). Sampling between the spans reported a
combine mismatch at fox root 1 epoch 22 whose COMBINE delta sits in its
after-span. With the boundary right there are none.

**~~If anyone re-opens this, brief it as a CACHE experiment~~ — MEASURED
2026-08-14 AND IT IS 4,193 tk/frame.** The D-cache census below read
`PrepareProductionRun`'s own data loads directly: 6,713,721 excess cycles =
**4,193/frame**, half the floor. The cold-`stats` hypothesis was right in
mechanism and wrong in size. Lane closed on both arms.

Conditions 1-6 cannot change while a fighter draws, so they can be hoisted to
once per draw (2/frame instead of 83.1) with nothing certified and nothing
deleted — worth ~2,000 tk/frame of arithmetic, which is under the floor. The
only reason to build it was the mechanism: `stats` is large and the emit walks
1,878 corners and 6,492 bytes of dense table against a 4 KB dcache between runs,
so the predicate's ~6 `stats` loads are plausibly cold every time. That is the
shape that paid **-35,904 P95** in slice 22, where the compare was never the
cost. Measure at P50 on a same-binary A/B.

### D-CACHE WORKING-SET CENSUS — no layout lever remains, 2026-08-14

`artifacts/performance/2026-08-14_dcache-working-set/CENSUS.md`, off the c125
whole-match profile. No build, no ROM. `scripts/census-dcache-working-set.py`.

**Read this before briefing any further locality work.**

**1. RESOLVE THE BASE REGISTER BEFORE RANKING A LOAD.** The #1 site in the build
by cycles/execution — `ndsRendererTask36ReplayRun` `ldr r3,[r1,#184]`, **507
cyc/ex, 16,629 cyc/frame**, twice the next entry — is **not a cache miss**. `r1`
is `0x04000000` and `+184` is `DMA0CNT`; it is a `cmp/blt` busy-wait for a
synchronous DMA to the GX FIFO, i.e. the ARM9 held off the bus. Any ranking that
skips this check sends the next layout task straight at a scheduling problem.
`analyze-dcache-stalls.py` excludes literal-pool and stack loads but NOT MMIO;
the new census script classifies mmio/timer/cacheable and is the one to use.

**2. The addressable pool is huge and uniformly flat.** Data-load excess is
311,623 cyc/frame, of which **cacheable 276,984 (88.9%)**, mmio 19,928, timer
(the instrument) 5,603. Grouped by working set, cyc/frame: Z-other 43,001 over
**80 functions** · GObj walks 22,262 · stage renderer 21,750 · AObj 20,766 ·
FTStruct/FTParams 16,319 · renderer stats 14,458 · matrix 12,575 · fighter dense
11,497 · memcpy 6,835 · MObj 4,590. **Largest single site in the whole pool is
7,743** and it belongs to the already-closed AObj lane.

**3. Every hot family's footprint already exceeds the whole 4 KiB cache**, and
three of four are pointer-chased: GObj ~183 lines/frame (5,856 B), AObj ~358
(11,456 B), prepared dense 203 (6,492 B). Nothing stays resident, which is
exactly why the cost is spread evenly instead of concentrating somewhere fixable.

**4. The one clean structural defect is unfixable here.** `gcRunAll` reads
`GObj->link_next`(4), `func_run`(20) and a byte at 21 on **line 0**, and
`flags`(**124**) on **line 3** — two fills a node where one would do. It answers
the delete-a-line rule cleanly and is worth only **~3,000 cyc/frame**, and
`GObj` is `decomp/.../objtypes.h` with `battleship_sys_objman.c:23` including
decomp's `objman.c` in place. Read-only. Do not propose it again.

**5. The durable conclusion.** Against floors of ±8,544 placement / 9,664 repeat
spread / ~16,000 bankable, the best layout candidate is 3,000 and blocked.
**Locality work has no remaining single-structure lever** — the cost sits in
BattleShip's pointer-linked data model, and the three largest addressable
families (GObj + AObj + FTStruct = 59,347 cyc/frame) are all decomp-owned.
Anything further must change how much data is **visited** (node counts, call
counts, visit rates), not how it is arranged. That extends `HANDOFF.md`'s
existing AObj verdict from one family to all ten.

**Open, and NOT this task's scope:** the 16,629 cyc/frame DMA wait is the only
single site at the bankable bar. Before designing anything, answer *"is the
geometry engine saturated during that DMA, or is the CPU merely choosing to
wait?"* — a GXSTAT FIFO-depth sample at the poll answers it with no ROM change.
If the FIFO is full it is the CRITICAL-RULE trap and removing the spin only
moves the wait.

### Slice 23: two redundant passes deleted, and why the mean did not move

**FTR 301,162 -> 302,217, +1,055.** Two real deletions, both unconditional, both
measured *up*. Kept anyway -- less work and less code -- but the number is the
finding, not the win.

- **The root's render preamble is held by reference, not copied.** The 24-byte
  `NDSRendererNativeFighterPreamble` was copied out of the immutable contract
  table into every selected root every frame: 39.5 `ldmia`/`stmia` pairs a frame
  at **144 cycles each**, which the c115 per-PC census prices at 3,250 tk/fr on
  one source line.
- **The MObj counting pre-pass in `PrepareNativeMaterials` is gone.** It chased
  `mobj->next` over every material chain a second time (1,215 tk/fr on that one
  line) purely to reject an over-capacity chain before writing. The write walk
  already carries the same bound and already reports `*out_count` for rollback.

**The lesson, and it is general: a per-PC census attributes a cache miss to the
instruction that TAKES it, not to the work that could be deleted.** Both cuts
removed a *first reader*, not a *reader*. The preamble's 24 bytes are still
consumed by `ApplyProductionPreamble`, so the two line fills simply moved to it;
all the copy actually saved was the write-allocate and write-back of the
destination. The counting walk was paying the misses the write walk then rode
warm. Deleting a redundant pass over data that is still consumed saves the
**instructions**, not the **fills** -- and at this scale the instructions are
under the placement floor. Price a deletion by asking what stops being *touched*,
not by summing the cycles the profile parks on it.

### `build.ps1` was BROKEN on this branch and nothing noticed

`generate_nds_native_owners.py` -- which `build.ps1` runs and `make` does not --
died on `M3_STAGE_FALSIFIER: named source closure is absent:
ndsRendererAdapterPrepareNativeOwnerHierarchy`. **A clean checkout could not
generate the fighter IR**, and four of six generated `.inc` files are gitignored,
so a clean checkout could not build.

Root cause: `named_c_closure`'s regex demanded `^ident...\bname(`, so a
definition whose name starts its own line could never match -- and the cycle-110
cold-outlining slices gave **six** functions in `reloc_backend_renderer_dl.c`
exactly that shape:

```c
static sb32 __attribute__((noinline, cold, optimize("Os")))
ndsRendererAdapterPrepareNativeOwnerHierarchy(
```

Fixed at the root in `scripts/stages/generate_nds_native_stage.py`: the leading
return type is now optional. Admitting a bare `name(` at line start is safe
because the existing loop already rejects any match whose next `;` precedes its
next `{`, which is every call site. **A generator that only `build.ps1` runs is
invisible to every measurement cycle -- run it after touching a signature in a
manifest-named closure.**

The same run then caught slice 23 honestly: the manifest classifies
`input.preamble.flags`, and the by-reference root spells it
`input->preamble->flags`, which the arrow scanner attributes to `input.preamble`
and then loses. The preflight now hoists `const NDSRendererNativeFighterPreamble
*preamble = input->preamble;` so the field stays visible as `preamble.flags`.
**That falsifier is worth its keep** -- it found a consumed field going dark in
the same change that made it happen.

### The fighter emit is bound by VERTICES, not by words and not by data layout

`--pc-detail` on all three emitters, whole match, no build. Exact loop-iteration
counts, so these are corner counts, not estimates:

| emitter | corners/frame | entries/frame | cyc/corner | tk/fr |
|---|---:|---:|---:|---:|
| raw untextured | **1,711** | 53.2 | 40.5 | **34,606** |
| raw textured | **437** | 13.8 | 50.7 | **11,085** |
| cross-matrix | **127** | 15.7 | 84.2 | **5,350** |
| **total** | **2,275** | 82.7 | | **51,041** |

The untextured loop is eleven ARM instructions in ITCM reading DTCM, and it still
runs at **3.55 cycles per instruction**. The per-PC rows say exactly where:

```
str r3,[ip,#1164]    8.00   GFX_VERTEX16 word 2
bne <loop top>       7.94   stalled behind that store draining
add r1,r0,r3,lsl#2   6.00   stalled behind the GFX_NORMAL store
ldr lr,[r5,r3,lsl#2] 5.16   DenseNormals[dense_id]
str lr,[ip,#1164]    3.00   GFX_VERTEX16 word 1
str lr,[ip,#1156]    2.93   GFX_NORMAL
```

**~28 of the 40.5 cycles are the GX writes**, and the textured loop pays the same
~28 for FOUR words rather than three -- so the stall is **per vertex**, not per
word. Three consequences, all now settled:

- **A baked/DMA'd GX stream is refuted as a lever here.** Packed FIFO format
  costs *more* words for the same vertices, and words are not what stalls.
  HANDOFF's "the emit half is near its floor" was right; its reason (eleven
  instructions a corner) was not the reason.
- **A smaller vertex format (`VTX_10`) is refuted for the same reason** -- fewer
  words per vertex, identical vertex count.
- **Fewer vertices is the only lever, and that is exactly what strips are.**

### Task 56 mode 2 drew 35.6% of the fighter BACKFACING -- a generator bug, fixed

Found statically, no build, before spending a measurement.
`scripts/fighters/check_fighter_primitive_streams.py` expands every generated
group back into oriented triangles under the DS strip rule (triangle *k* is
`(v_k, v_k+1, v_k+2)` for even *k* and `(v_k+1, v_k, v_k+2)` for odd *k*) and
compares them with the run's source triangles:

```
mode 1: 1,714 vertex submissions in 513 groups, 626 triangles ... OK
mode 2: 1,012 vertex submissions in 162 groups, 626 triangles
  REVERSED WINDING: 223 triangles (35.6%) -- these are culled away on hardware
```

`_stripify_run`'s mode-2 heuristic tried three initial active edges:
`(t0[1],t0[2])`, `(t0[0],t0[2])`, `(t0[0],t0[1])`. The middle one is not a
directed edge of `t0` -- it emits `[t0[1], t0[0], t0[2]]`, the mirror of the
source triangle -- and **every triangle in a strip inherits its first triangle's
winding**, so a strip that started there came out entirely backfacing. The
longest-strip search picked it whenever it won on length. Corrected to
`(t0[2],t0[0])`; mode 2 is now **1,014 vertices in 163 groups, every source
triangle drawn exactly once with the source winding**.

**That is what made Task 56 unusable, and it is not what it was killed for.** The
2026-07-24 KILL row reads "does not move ALL" over a **128-frame window at frame
600** -- the window class the whole-match instrument later invalidated -- against
a control built three days earlier and ~31 KB smaller, and its "ROM hangs the
present loop" symptom has separate address evidence pointing at the boot cliff.
Nobody checked whether the geometry it drew was the fighter's geometry.

The runtime half was wrong too. `EmitProductionPrimitiveGroups` was
`noinline, cold, optimize("Os")` in `.main` while its raw siblings sat in ITCM at
eleven instructions a corner, and it branched on `textured` once per **vertex**.
A 46% vertex cut cannot survive being paid for at twice the per-vertex rate. It
now has the same placement, the same inner-loop shape, and the type test hoisted
to the group.

**One invariant the original missed:** batches are REUSED across runs without
re-issuing `glBegin`, so every other emitter -- cross-matrix, raw, the next run
sharing the batch -- assumes the primitive type is still `GL_TRIANGLE`. The strip
emitter now restores it before returning: one FIFO word against 53 run emissions
a frame, and no invariant left for a future caller to violate.

**`NDS_R2_STRIP_ROUTE=1` is the one-binary A/B**, same instrument as
`gNdsR2AnimCutRoute`: both emitters compiled, `gNdsR2FighterStripRoute` in
`.data`, `aligned(32)`, default 1. Task 56 was killed against a control that was
a different binary; this one is not. At `NDS_R2_STRIP_ROUTE=0` the test folds to
a constant and the unselected emitter is dead-coded, so the shipped ROM pays
nothing for the instrument.

### Slice 24 (ARCHITECTURAL): Task 56 strips GRADUATE -- FTR P50 -11,584

**One binary, `build-c116-t56route`, `romSha256` identical across both arms,
same melonDS `DE80E46BDCF1FD98`, 1600 samples an arm, DLDI on.
`gNdsR2FighterStripRoute` read back **0** and **1** at end of run, so the poke
landed and was never stamped.**

| bucket | A: raw corners | B: strips | delta |
|---|---:|---:|---:|
| **FTR P50** | 313,856 | **302,272** | **-11,584** |
| **FTR P95** | 316,672 | **305,408** | **-11,264** |
| **WORK-H P50** | 952,512 | **941,312** | **-11,200** |
| WORK P50 | 958,016 | 946,560 | -11,456 |
| STG P50 | 189,184 | 189,184 | 0 |
| ALL P50 | 1,118,336 | 1,118,336 | 0 |
| OTHR P50 | 201,024 | 211,648 | +10,624 |
| WAIT P50 | 181,760 | 192,000 | +10,240 |

Read the last three rows together: **`ALL` P50 is identical to the tick and the
saved work reappears as `WAIT`.** That is `ALL` being VBlank-quantised wall time,
exactly the trap that killed this lever in 2026-07 -- and exactly why the
standing rule judges on `WORK-H`, not on `ALL`. `STG` unchanged to the tick is
the control that the route touched only the fighter path.

Predicted ~20,000 from 2,148 raw corners a frame becoming ~1,160 at 40.5
cycles; measured **-11,584**, about 57% of that. The likely remainder is that
part of the ~28-cycle GX stall is **per polygon**, not per vertex, and strips
cut vertices while leaving all 626 triangles. Worth knowing before the next
geometry lever is priced off the vertex count alone.

Arm B carries one artefact frame -- `FTR` max 4,496,896 against arm A's 322,112,
with `SRC`, `GCRA` and `SINT` all maxing at ~4.5M in the same sample. They cannot
all be true at once (their sum exceeds that frame's `ALL`), so it is a perturbed
ring sample, not a hitch; it inflates arm B's *mean* by ~2,600 and is why the
means move less than the medians. Judge this one on P50/P95.

`NDS_TASK56_FIGHTER_PRIMITIVES` now defaults to **2**, so `make p1` and the
tick-HUD ROM both ship strips. `NDS_R2_STRIP_ROUTE` stays 0 by default: the
route is an instrument, and at 0 the unselected emitter is dead-coded away.

**Banked on the graduated default build, re-measured after the BEGIN-policy fix
(route compiled out): `FTR` mean 302,217 -> 291,896, P50 312,640 -> 301,760, P95
315,456 -> 304,768, `WORK-H` P50 952,960 -> 942,976.** Boundary **passes**. The
figures first published here (290,842 / 300,736 / 303,680, -94,666) came off the
build that was silently losing geometry and are **withdrawn** -- a build that
culls a third of the fighter is cheaper for a reason.

**Cycle 116 total: `FTR` 385,508 -> 291,896, -93,612 (-24.3%).** The owner's
"next: <300K" target is met on the *mean* and missed by 1,760 on the P50, and it
is met on a DS-native AOT primitive stream rather than on deletions.

### The strips SHIPPED BROKEN, and the checker is why -- read this before adding a guard

**Regression, 2026-08-10.** Strips went out as the default and the owner
reported **missing geometry on both Mario and Fox** within minutes, plus
animations that would not play (Mario's grab spin on Fox). Reverted to the raw
path inside the hour, root-caused, fixed, re-shipped.

**Cause.** `EmitProductionPrimitiveGroups` issued `BEGIN_VTXS` only when the
group TYPE changed. Separate triangles concatenate harmlessly, so the condition
looks right -- but two ADJACENT `GL_TRIANGLE_STRIP` groups then shared one
vertex list. Each join produced two bogus bridging triangles and flipped the
parity of every triangle after it, so the remainder of that strip was culled.
The very first run has **six consecutive strip groups**. The condition is now
"always begin unless a `GL_TRIANGLE` group follows a `GL_TRIANGLE` group".

**The worse defect is the checker, and it is the transferable one.**
`check_fighter_primitive_streams.py` expanded every group **independently** --
that is, it proved the DATA under a BEGIN policy the runtime did not follow. It
was green on a build that was losing a large fraction of both fighters. It now
models the runtime's policy, and the model is demonstrably load-bearing: re-run
under the OLD policy it reports **mode 2 drawing 744 triangles against 626
source across 29 runs**, and mode 1 **642 against 626 across 7**. Prove a guard
fails on the defect it is meant to catch, or it is decoration.

**A passing verifier is not visual verification.** Boundary passed on the broken
build and `artifacts/visibility/latest.png` showed both fighters looking
complete, because that canonical frame does not show the affected joints. The
owner caught it in play. **Hand the owner a ROM before calling a rendering
change good** -- the Boundary screenshot answers "did anything catastrophic
happen", not "is the geometry right".

**What the diff of the regenerated IR proved, and it is worth keeping.** The
fighter `.inc` is gitignored, so regenerating it is an unbounded change in
principle. Extracting the pre-session copy from the owner's 15:02 snapshot and
diffing showed **only the 8 Task-56 strip blocks changed**; the other 47 blocks
are byte-identical to the 2026-07-29 generation. **The desktop snapshots are a
usable baseline for gitignored generated files** -- `7z e <zip> <path>` -- which
is the only way to bisect one.

**Re-measured on the FIXED code, one binary, `romSha256` `3C701590...`
identical across arms, route read back 0 and 1, 1600 samples each:**

| bucket | A: raw corners | B: strips | delta |
|---|---:|---:|---:|
| **FTR P50** | 313,856 | **303,424** | **-10,432** |
| **FTR P95** | 316,800 | **306,432** | **-10,368** |
| **WORK-H P50** | 952,640 | **942,528** | **-10,112** |
| **WORK-H P95** | 1,148,992 | **1,134,848** | **-14,144** |
| STG P50 | 189,120 | 189,184 | +64 |
| ALL P50 / P95 | 1,118,336 / 1,678,912 | identical | 0 |
| WAIT P50 | 181,824 | 190,336 | +8,512 |

The extra BEGIN per strip group costs about **1,150** of the broken build's
-11,584, which is the honest price of correctness and still an order of
magnitude above the placement floor -- and this arm pair has no placement term
at all. `STG` +64 and `ALL` identical to the tick are the controls.

**The glBegin audit is clean.** Every other `glBegin` in `nds_renderer.c` passes
a literal type unconditionally; the conditional-on-type-change shape existed
only in the strip emitter. The codebase already understood the semantics --
`nds_renderer.c:14938` notes that `glBegin(GL_TRIANGLE)` closes the preceding
quad batch -- the gap was that a strip following a strip has no type change to
trigger it.

### Requirement 4 is sized and designed: the cubic evaluator is 80% float boundary

**The animation lane is 107,870 tk/fr named by symbol** (c115 census, tick factor
0.4993, 1,250 frames) -- larger than the whole fighter emit was before strips:

| symbol | tk/fr | insns/frame | cyc/insn |
|---|---:|---:|---:|
| `ndsR2CubicValueFixed` | **31,708** | **38,956** | 1.63 |
| `gcPlayDObjAnimJoint` | 24,788 | 17,651 | 2.81 |
| `ndsR2FtAnimParseDObjFigatree` | 19,230 | 10,810 | 3.56 |
| `gcPlayAnimAll` | 10,589 | 5,333 | 3.98 |
| `ftParamUpdateAnimKeys` | 8,881 | 4,944 | 3.60 |
| `ndsBaseGcPlayMObjMatAnim` | 7,201 | 4,955 | 2.91 |
| others | 5,473 | | |

**`ndsR2CubicValueFixed` runs 176.4 times a frame at 220 instructions a call**
(entry-PC count 220,505 over 1,250 frames; 38,956 insns/frame). A Hermite
evaluation is about a dozen multiply-accumulates. The other ~200 instructions
are the float boundary the kernel was never allowed to cross: **six inlined
`ndsR2F32ToFixed`, one `ndsR2F32MulToFixed`, one `ndsR2FixedToF32`**, each a
hand-written 20-30 instruction bit-manipulation routine with its own rounding
and saturation branches -- and the per-PC detail shows exactly that shape, with
`bmi`/`b` rounding branches and `umull`s filling the top of the ranking under a
nine-register `push`/`pop` pair that alone costs **2,109 tk/fr**. All of it on
values that started life as `s16` with power-of-two scales in the figatree. Its
1.63 cyc/insn says this is not a stall problem -- it is *instruction count*,
and the instructions are format conversion. **292 of the function's ~580
instructions execute**, the rest being saturation and subnormal handling.

Independent confirmation, already in the tree and read for the first time this
cycle: `artifacts/performance/2026-08-09_c106-profile/softfloat-attribution.json`
attributes **25.1M of 87.5M soft-float cycles to animation evaluation** --
`ftAnimParseDObjFigatree` 7.9M, `gcPlayDObjAnimJoint` 6.7M, `gcPlayMObjMatAnim`
4.5M, `ndsR2CubicValueFixed` 2.5M, `ndsBaseGcPlayDObjAnimJoint` 1.9M.

**The design that does not add a cache and does not fork the struct.**
`FIXEDPOINT_ANIMATION.md` is right that `AObj` is the problem and right that the
fighter path must not drag material, camera, stage and effect animation with it.
The cheap way to get both: **discriminate on the `kind` field the evaluator
already switches on.** Add Q-format kinds (`nGCAnimKindCubicQ` and friends); the
fighter parser emits them with Q12/Q16 values written straight from the source
`s16` by shift; the evaluator gains fixed Step/Linear/Cubic arms. Non-fighter
AObjs keep the float kinds and the decomp's own expressions, bit-identical.

That is zero new state, zero new struct, zero parallel array, and one more case
in a switch that already exists -- "replace, don't coexist", scoped to fighters,
exactly as the requirement words it. What it deletes per node per frame:

| today | after |
|---|---|
| `aobj->length += speed` (`__aeabi_fadd`) | integer add on a Q16 clock |
| `length * length_invert` + convert | integer multiply, or a precomputed phase step |
| six `ndsR2F32ToFixed` | nothing -- the fields are already Q12 |
| one `ndsR2FixedToF32` | **stays**, the temporary boundary for `DObj`'s float pose |
| Linear's `fmul` + `fadd` | integer multiply-add |
| Step's `__aeabi_fcmple` | integer compare |

**Constraints that must survive the rewrite**, from earlier cycles: the phase is
`frame * step`, never accumulated (it drives hitboxes); the arena is contiguous,
not linked; `check_r2_cubic_error_bound.py` bounds the result (0.0028 rad /
0.0067 world units today) and must be re-run against the new kernel; and
`ndsR2CubicValueFixed` must stay `target("arm")` for SMULL --
[[thumb-hides-64bit-cost]] cost +36,032 P95 once already.

### Cycle 117: the gate is RE-BANKED and the animation lane is fully anatomised

**`WORK-H` P95 1,317,440 on the both-CPU gate arm** (P50 973,568, mean
1,016,526; 1600 samples, DLDI on, `NDS_R2_BOTH_CPU=1`, 5 of 1600 frames
repeated). The 1,447,318 in `HANDOFF.md` was `f082b3c8`; cycles 110-116 took
**-129,878** off it. **Gap to the 1.12M gate is 197,440.** `SRC` P95 653,696 is
its largest named bucket, `SINT` P95 336,960.

**Whole-match profile census, both-CPU arm, 1600 frames**
(`artifacts/performance/2026-08-10_c117-lane/`, 2.6 GB CSV, 4,028,886,502
cycles). Aggregated by family, cycles/frame:

| family | cyc/frame | ticks/frame | % of over-gate premium |
|---|---:|---:|---:|
| soft float (leaf) | 187,396 | 93,567 | 7.7% |
| **anim playback** | **105,486** | **52,669** | 2.0% |
| map collision | 90,132 | 45,003 | 0.7% |
| bulk memory | 80,240 | 40,064 | 4.1% |
| **anim parse** | **55,200** | **27,561** | 2.5% |
| hud/printf (INSTRUMENT) | 47,785 | 23,858 | 16.3% |
| anim material | 21,559 | 10,764 | 0.3% |
| asset load/fat | 21,228 | 10,599 | 5.2% |
| anim change | 13,496 | 6,738 | 2.2% |

**Read the premium column with two subtractions.** `armWaitForIrq` is **41.6%**
of it and is idle -- an over-gate frame spans more VBlanks, so it contains more
idle, which is an artifact of the split and not work. The **hud/printf chain is
16.3% and is the tick-HUD instrument**, which `WORK-H` already excludes (that is
what the `-H` means). Net attributable premium is ~211,400 cyc/frame, and
**animation is 7.0% of it while being 97,733 ticks/frame of FLAT body cost** --
so animation is a P50-and-P95-together lever, not a tail lever.

**Exact per-symbol anatomy** (one streaming pass over the CSV; entry-PC `insns`
IS the call count):

| symbol | cyc/frame | calls/frame | insns/call | cyc/insn |
|---|---:|---:|---:|---:|
| `gcPlayDObjAnimJoint` | 42,484 | 106.9 | 152 | 2.61 |
| `ndsR2AnimValueQ` | 41,942 | **280.0** | 89.5 | **1.67** |
| `ndsR2FtAnimParseDObjFigatree` | 37,842 | 103.5 | 115.7 | 3.16 |
| `gcPlayAnimAll` | 16,567 | 10.4 | 398 | 4.00 |
| `ftParamUpdateAnimKeys` | 13,588 | 4.1 | 971 | 3.41 |
| `ndsBaseGcPlayMObjMatAnim` | 10,947 | 69.2 | 57 | 2.78 |
| `BuildFighterTraRotRpyDirect20p12` | 27,204 | 38.4 | 296 | 2.40 |
| `__aeabi_fadd` | 70,124 | **1,900.5** | 31 | 1.19 |
| `__aeabi_fmul` | 46,756 | **1,840.9** | 22 | 1.14 |

**Requirement 4 emptied the float out of fighter joint animation, and the
soft-float caller census proves it.** 46,856 GDB samples on
`__aeabi_fadd`+`__aeabi_fmul` entry (58,358 ticks/frame class):

| caller | share | ticks/frame |
|---|---:|---:|
| `ndsMPFCSegmentCrossesKernel` | 16.2% | 9,476 |
| `ndsStageMPAdjustFloorLoopWallSweep` | 13.8% | 8,056 |
| `ndsBaseGcPlayMObjMatAnim` | 7.6% | 4,424 |
| `syMatrixLookAtReflectF` | 6.2% | 3,617 |
| `ndsR2FtAnimParseDObjFigatree` | 4.6% | 2,665 |
| `ndsBaseMPProcessCheckTest{R,L}WallCollisionAdjNew` | 4.8% | 2,850 |

**Map collision is 36.6% of the whole soft-float class (~21,300 ticks/frame)**
and is the next lane after this one, exactly as the goal predicted.
`gcPlayDObjAnimJoint`, `ndsR2AnimValueQ` and `ndsR2CubicValueFixed` **do not
appear at all** -- fighter joint evaluation is out of the float class entirely.
What animation float remains is **material** animation (4,424, deliberately left
float because MObj AObjs are shared with stage and effects) and the parser's
per-DObj `anim_wait -= anim_speed` / `anim_frame += anim_speed` (2,665; those
are `DObj` f32 fields with `F32_MIN`-derived sentinels and readers everywhere,
so they are not cheap to convert).

**So what is left in animation is memory traffic and instruction count.** Three
sized targets, and the numbers say which:

1. **The AObj working set.** `gcPlayDObjAnimJoint` PC `0x02001484` costs
   **9,136 cyc/frame at 25.64 cyc/insn over 356 executions -- 21.5% of the
   function in ONE instruction.** 356 nodes x 36 B = **12.8 KB streamed every
   frame through a 4 KB D-cache**, and the nodes are individually
   `syTaskmanMalloc`'d 4-byte-aligned and recycled through a LIFO free list
   (`gcGetAObjSetNextAlloc`, `objman.c:636`), so a DObj's list is scattered and
   each 36-byte node straddles two 32-byte lines.
2. **The per-node call.** `ndsR2AnimValueQ` is `noinline` + `target("arm")` and
   is called **280 times a frame from one site**; its `push {r4-r9,sl,fp,lr}`
   costs 2,529 cyc/frame at 9.03 cyc/insn and its `pop` 4,154 at 14.83 --
   **6,683 cyc/frame, 16% of the evaluator, to save and restore registers.**
   The stack is in DTCM so this is not a miss; it is nine registers.
3. **The parser's per-DObj early-out.** 103.5 calls a frame at 115.7
   instructions, most of which return at `anim_wait > 0` having done two
   soft-float operations.

**Status of the three, checked 2026-08-11 before proposing any of them again:**

| target | state |
|---|---|
| 1 — AObj working set | **DONE, cycle 109.** `battleship_sys_objman.c` already carves a contiguous 512-node pool in `gcSetupObjman`; `NDS_R2_AOBJ_POOL_COUNT`, `gNdsR2AObjPoolDeclines`. Its own open item is trimming 512 to the measured peak, which is RAM, not ticks. |
| 2 — the per-node call | **CLOSED. Do not propose it.** See below. |
| 3 — parser early-out | **DONE, slice 33** — and it was worth 1.6% of `SINT` P95, not the share its 31.5% of calls suggested. |

**Whole-match per-PC profile of the player, 2026-08-11** (`2026-08-10_c117-lane`,
4,028,886,502 cycles; `gcPlayDObjAnimJoint` 67,974,137 = 1.69%, 604 bytes, 185
distinct PCs). This supersedes every 128-frame figure above per the whole-match
rule, and it changes which target is largest:

| PC | instruction | cycles | %fn | executions | cyc/insn |
|---|---|---:|---:|---:|---:|
| `0x02001484` | `ldrb r5,[r4,#5]` — `aobj->kind` | **14,616,804** | **21.5** | 570,065 | **25.64** |
| `0x0200143a`+`0x020015a4` | `ldr r4,[r4,#0]` — `aobj->next` | 2,604,679 | 3.8 | 233,229 | 11.2 |
| `0x020014b2`…`0x020014bc` | the `switch (aobj->track)` jump table | 6,465,297 | 9.5 | ~448,000 | 4.3 |
| `0x020013c0`…`0x02001458` | prologue + epilogue | 4,069,183 | 6.0 | 171,016 | 5.9 |

**The bare AObj list walk is 17,221,483 cycles — 25.3% of the player — and it is
still the #1 cost after cycle 109's contiguous pool.** That is not a failure of
the pool; the cycle-109 comment predicted it in writing ("contiguity cannot make
it resident either"). 570,065 visits over ~1,700 presented frames is **335 nodes
a frame × 36 bytes = 12,060 bytes streamed through a 4 KB D-cache, 3× over**, so
`aobj->kind` misses whatever order the nodes are in. Contiguity bought the
second line per node; it cannot buy residency.

**Only shrinking the per-node working set changes this**, which is precisely the
AOT dense-track format of slice 32 — a baked array of the fields actually read,
streamed sequentially, instead of a 36-byte decomp-shaped node reached by
pointer. This profile is the strongest evidence the rewrite has: **25.3% of the
largest animation symbol is memory traffic that no amount of instruction
deletion can reach.**

The `switch (aobj->track)` at 9.5% is a real second-order target — a 10-way
jump table re-deciding per node, per frame, a value fixed when the AObj was
created, with `mov pc,r3` alone costing 2,399,196 — but it is ~1,900 ticks/frame
and it edits a `.text.hot` member, which the note below says is exactly where
two estimators got the sign wrong. Do not spend it as a standalone slice; fold
it into the dense-track format, where the dispatch disappears by construction.

**Target 2 is closed by the linker script, and the reason is not obvious from
the cost.** Deleting the 6,683 cyc/frame of `push {r4-r9,sl,fp,lr}`/`pop`
requires inlining `ndsR2AnimValueQ` into its **single** call site
(`0x20014aa`, the only `blx` to it in the image), which requires
`gcPlayDObjAnimJoint` to become `target("arm")` — the callee is ARM for SMULL.
That grows a `.text.hot` member from 604 bytes to roughly 2,000.
`linker/nds_hot_text.ld:180-200` records two measured attempts at moving mass
in and out of that curated 8 KiB list, **both of which regressed and both of
which two independent estimators had predicted would win**:

- Task 94 moved `gcPlayDObjAnimJoint` out to `.itcm` with zero eviction and
  measured `WORK-H` P50 **+6,144**, with `STG` up 3,712 despite never calling
  it — members re-address each other.
- R2-03 E66 is nearly this exact experiment in reverse: it admitted the ARM
  evaluator to `.text.hot` immediately after its only caller, sized at ~7,093
  ticks/frame recoverable, and measured `WORK-H` P95 **+24,448**.

The script's standing instruction is to treat `.text.hot` as **closed in both
directions**. Both magnitudes are 128-frame-era and so unusable per the
whole-match rule, but the *sign* is what was wrong twice, and the mechanism is
structural rather than windowed. Re-opening this is an owner-visible decision
with a whole-match instrument, not a mid-campaign slice. **The 6,683 cyc/frame
is the standing price of that closure — record it as known and paid, not as an
unclaimed win.**

**A regression found by disassembly, not by measurement.** Requirement 4's first
cut folded all three arms into one shared `s64 acc`. GCC therefore sign-extended
`length`, `value_base` and `rate_target` to 64 bits **before the kind branch**,
expanded every `(s64)a * b` from one multiply into a `mul`/`mla`/`umull` triple,
and spilled 20 bytes of stack. Narrowing each arm to its own `s32` result took
the stack frame to 12 bytes. **Check the disassembly of this kernel for hoisted
`asr #31` and stack spills after touching it** -- the host error bound cannot
see code shape, and it reported byte-identical numbers across the fix.

### Slice 32, steps 1-2: the AOT generator's surface and its spec

Both offline. No runtime change, no ROM, no divergence risk -- the board's own
staging is generator, then offline proof, then the runtime switch, and
everything before the switch is safe to do in pieces.

**Step 1: the opcode surface is CLOSED.** 15 opcodes defined in the reference,
all 15 handled by `ndsR2FtAnimParseDObjFigatree`, none invented by the port.
`scripts/check_ftanim_opcode_surface.py` asserts both directions and is
registered in `check-gbi-decode-fixtures.ps1`. A generator can only be proven
correct against a finite known surface, and now that premise is checked rather
than assumed.

**Step 2: what each opcode actually writes**, extracted mechanically from the
parser (all 15 cases accounted for):

| opcode(s) | payload | ensure | segstart | AObj fields written |
| --- | --- | --- | --- | --- |
| `SetVal0RateBlock`+`SetVal0Rate` | yes | yes | **yes** | value_base, value_target, rate_base, rate_target, length_invert, length, kind |
| `SetValBlock`+`SetVal` | yes | yes | - | value_base, value_target, rate_base, rate_target, length, kind |
| `SetValRateBlock`+`SetValRate` | yes | yes | - | as above **+ length_invert** |
| `SetTargetRate` | yes | yes | - | **rate_target only** |
| `SetValAfterBlock`+`SetValAfter` | yes | yes | - | as above **minus rate_base** |
| `Event1611` | yes | yes | - | (none) |
| `SetTranslateInterp` | - | - | - | **interpolate** |
| `Block`, `Loop`, `End`, `SetFlags` | - | - | - | (none) |

**Two findings that decide the dense format:**

1. **Only 6 of 15 opcodes write track state.** `Block`, `Loop`, `End` and
   `SetFlags` write nothing, and `Event1611` consumes a payload while writing
   nothing. So a baked track must encode CONTROL FLOW separately from TRACK
   DATA -- a small control stream plus per-track segment arrays, not one flat
   list of segments.
2. **`SetTranslateInterp` writes `interpolate`, a POINTER INTO THE EVENT
   STREAM**, not a value. It cannot be baked into a position-independent track
   without resolving it at load time or changing its representation. **It is
   the one opcode that blocks a fully static bake** -- and it is the same
   opcode that was the sole exception in slice 31's `ENSURE` audit. Decide its
   representation before writing the emitter; everything else follows the
   table above mechanically.

**Step 3: `SetTranslateInterp` is NOT a blocker after all.** Traced its
consumer rather than assuming. `interpolate` is handed to
`syInterpCubic(&dobj->translate.vec.f, aobj->interpolate, value)` -- it is a
spline CONTROL-POINT ARRAY for the translate vector, indexed by a value the
parser clamps to [0,1]. And it is assigned
`root_dobj->anim_joint.event16 + (event16->s / 2)`, which is a **constant offset
into the static figatree asset**, not a pointer computed from runtime state.
(The decomp writes the same thing as `anim_joint.event32->p`.)

So it bakes: store the offset, resolve it once against the asset base at load.
That is `PROJECT_GOAL.md`'s "heavy loading-time preparation", and it removes the
last thing standing between the opcode table above and a mechanical emitter.
**The dense format therefore needs three parts:** a control stream (`Block`,
`Loop`, `End`, `SetFlags`), per-track fixed-point segment arrays (the six
state-writing opcodes), and a relocation list for the one offset field.

**Step 4: all 15 case bodies read. The control stream is NOT pure data.**

`nGCAnimEvent16Loop` and `nGCAnimEvent16End` both call
**`root_dobj->parent_gobj->func_anim(root_dobj, -2, 0)`** and
**`(root_dobj, -1, 0)`** respectively, guarded by `is_anim_root`. The animation
script invokes a **GObj callback into gameplay code** at its loop and end
points. A baked track that replaces the script must fire those callbacks at
exactly the same times -- so **this bake touches GAMEPLAY, not just
presentation**, and a timing slip is a gameplay bug rather than a visual one.
That single fact outranks every performance argument for slice 32 and is why the
replay proof has to compare callback timing, not only AObj field values.

The rest of the control-stream semantics, now complete:

- `Block` -- if the next command's toggle is set, `anim_wait += payload`.
- `SetFlags` -- writes **`root_dobj->flags`** (a DObj field, not an AObj one),
  then the same conditional `anim_wait += payload`.
- `Loop` -- `event16 += event16->s / 2`, a RELATIVE JUMP, then sets
  `anim_frame = -anim_wait` on both the DObj and its GObj, then the callback.
- `End` -- `ndsR2AnimAdvanceTail`, `anim_frame = anim_wait` on both, then
  `anim_wait = AOBJ_ANIM_END`, then the callback, then **`return`** (not
  `break`).
- `SetTargetRate` -- `rate_target` only, from `NDS_R2_FTANIM_TARGET(1)`.
- `SetValAfter*` -- `value_base = value_target`, new `value_target`,
  `kind = STEP`, `length_invert = frames`, `length = len_new`,
  `rate_target = 0`; the `Block` variant also does `anim_wait += payload`.
- `Event1611` -- `ndsR2AnimAddLength` per flagged track, writes no field
  directly.

**So the dense format needs four parts, not three:** a control stream carrying
wait accumulation, DObj flag writes, relative jump targets and **callback events
tagged -1/-2**; per-track fixed-point segment arrays; the relocation list for
`interpolate`; and the `is_anim_root` predicate preserved, since the callbacks
are conditional on it.

**Step 6-7: an executable model, and the loop-convergence question it
settled.** `scripts/ftanim_script_model.py` executes all fifteen opcodes and
records BOTH timelines -- per-track AObj state after every command, and the
gameplay callbacks. Semantics come from the PORT parser, not decomp, and the
flags loop reproduces the parser's early exit at the first zero mask rather than
scanning ten tracks, because that is observable.

**The question a static bake lives or dies on: do looping animations reach a
fixed point?** Segment values CHAIN -- every writing opcode does
`value_base = value_target` -- so if a `Loop` produced different values on each
pass, no static bake could exist for a looping animation, and most fighter
animations loop. Ran it: **track state converges after ONE iteration.** Iteration
0 differs from iterations 1..n, which are identical to each other.

So the baked form is a **prologue plus a steady-state loop body**, not one flat
segment list -- and the emitter must VERIFY convergence per script rather than
assume it, since a script whose state never settles has to fall back to the
interpreter. That is a structural requirement no amount of profiling would have
surfaced; it comes only from executing the semantics.

One detail worth carrying: `length` moved `0.0 -> -0.0` between iteration 0 and
steady state in the model. Signed zero has bitten this project before (it is the
whole reason `nds_fcmp.h`'s zero predicates shift the sign out), so the
round-trip proof must compare BIT PATTERNS, not float equality, or it will call
`0.0` and `-0.0` the same and miss a real divergence.

**Step 8: the bake round-trips.** `scripts/ftanim_bake.py`, 20,000 randomised
scripts, **0 mismatches**, both timelines compared as BIT PATTERNS. Replay sees
no opcode and no flags mask -- it walks resolved records only -- so this is not
a tautology: if mask expansion or the `value_base = value_target` chaining could
not be resolved ahead of time, replay would not reproduce the timeline. It does.
**The per-frame work of decoding a script and expanding a mask is provably
precomputable.** It does NOT prove the runtime player is faster; that is the
measured step.

**Step 9, the design decision that shrinks this slice: bake at LOAD, not at
BUILD.** Everything above assumed a build-time emitter, which drags in the asset
pipeline -- locating figatree data, a generator, a ROM blob, `EXPECTED_CENSUS`
re-pinning. None of that is required for the win. The records can be produced by
running the EXISTING parser once when an animation starts, then replayed on
every subsequent frame. That is `PROJECT_GOAL.md`'s compute-once rule applied
without any build tooling, it keeps the parser as the single source of truth so
the bake cannot drift from it, and it makes the whole slice a runtime change
that a route can A/B.

Two things it must get right, both already known from this cycle:

- **Invalidation.** The records are valid until the animation changes. Cycle 117
  lost slices 29 and 29b to cross-call caches in the collision code, and slice
  30 fixed that class by invalidating at the ASSIGNMENT rather than at readers.
  Apply the same shape here: drop the records where `anim_wait` is set to
  `AOBJ_ANIM_CHANGED`, not wherever they happen to be read.
- **RAM.** Records cost memory per active animation, and the heap low-water mark
  is already inside the GObj-cap margin (see `ram-is-not-free`). Size the record
  pool against a measured worst case before writing the player, not after.

**Step 10, a correction to my own cost model, and it lowers slice 32's value.**
`ndsR2FtAnimParseDObjFigatree` does NOT re-parse the script each frame. It
RESUMES one, and it returns early:

    anim_wait -= anim_speed; anim_frame += anim_speed;
    if (anim_wait > 0) { return; }        <-- BEFORE the table walk

So the walk -- and the whole event loop -- runs only on the frames where an
animation advances to a NEW script command. Every other call is the four lines
above and nothing else. **"The walk IS the call" was wrong**: the 95.7
instructions/call the profile reports is an AVERAGE over many cheap
early-return calls and a few expensive stepping calls, not a description of a
typical call. Slice 31's -7,104 is still real and still route-proven, but its
mechanism is narrower than stated -- it saved the walk on the subset of calls
that pass the early return and then find no table read.

**What that does to the AOT case.** The parse's 16,806 ticks/frame is dominated
by the early-return path: two float compares against sentinels, two adds, and a
GObj store, executed for every animating DObj every frame. **A baked track
cannot remove any of it** -- advancing `anim_wait`/`anim_frame` per frame IS the
animation clock, not interpretation overhead. What AOT can remove is the script
stepping, which happens on a minority of frames.

So the realistic recovery from slice 32 is **materially below the 9.1% of the
gap estimated at the ceiling step**, and the honest statement about this lane
is stronger than "capped": the largest single remaining animation symbol is
mostly irreducible per-frame clock work. **Measure the early-return share before
building the player** -- one counter split on the early return answers it, and
if it is the majority, slice 32 should be abandoned rather than built.

**Step 11: the split, measured -- and it REFUTES step 10.** Counters on the two
paths, whole-match gate arm:

    calls 212,516   early-out 108,186 (50.9%)   stepped 37,363 (17.6%)
                    no-anim   66,967 (31.5%)

Step 10 concluded the parse is "dominated by the early-return path" and
downgraded slice 32 accordingly. **That was wrong**, and wrong in a way this
project has a memo about: it reasoned from CALL COUNT and never weighted by
cost. Weighting it against the profile's 19,155,412 instructions in this
function -- early-out is roughly 20 instructions a call (~2.16M), no-anim about
8 (~0.54M) -- leaves **~16.5M over 37,363 stepped calls, ~440 instructions
each: about 86% of the function's instructions in 17.6% of its calls.**

So the reducible half is the DOMINANT half after all. Slice 32's target is
~86% of the parse's 16,806 ticks/frame, about **14,400 ticks/frame or 7.8% of
the 185,472 gap** -- essentially the ceiling step's original 9.1%, not the
"materially below" of step 10. **Step 10's downgrade is withdrawn.**

The 31.5% no-anim calls are their own small finding: a third of all calls are
for DObjs with `anim_wait == AOBJ_ANIM_NULL`, which do a compare and return.
Cheap individually, but it is a call per DObj per frame that a caller-side
predicate could skip entirely -- worth a look independently of slice 32.

**The standing lesson, third recurrence this cycle:** a share of CALLS is not a
share of COST. Slice 31's "the walk IS the call" over-read an average, step 10
over-read a count, and both needed the other number to be honest. Get both
before concluding.

**Step 12: the ctz mask walk, PRICED AND REJECTED before building it.**

The obvious next cut on the stepped path is the flags-mask loop -- every
state-writing opcode scans from bit 0 to the highest set bit, so a
count-trailing-zeros walk (`m &= 0x3FF; while (m) { i = ctz(m); m &= m - 1; }`)
would iterate only the set bits. Mechanically identical, ascending order
preserved, needs `target("arm")` for CLZ.

Priced against the step-11 split (37,363 stepped calls, 3.16 cyc/insn, 1,799
frames, 185,472-tick gap), at ~5 instructions per skipped iteration:

| commands per stepped call | iterations saved | ticks/frame | share of gap |
| ---: | ---: | ---: | ---: |
| 1 | 3 | 492 | 0.27% |
| 2 | 3 | 983 | 0.53% |
| 2 | 9 (absolute max) | 2,949 | 1.59% |
| 3 | 9 (absolute max) | 4,424 | 2.39% |

**The realistic case is ~500-1,000 ticks/frame, and even the physically
impossible best case is 2.4%.** All of it is far under the +-8,544 cross-build
placement floor, and the same-binary route that could attribute it would be
measuring a change worth a fraction of one percent of the gap. **Do not build
it.** I named it as the next step one turn before pricing it, which was the
wrong order -- the arithmetic takes one minute and it retracts the suggestion.

This closes the small-lever question for the parse. What remains in the stepped
path is the event-loop dispatch and the per-command field writes themselves, and
those are what the dense-track player replaces wholesale -- there is no
intermediate cut left between "leave it alone" and "build the player".

**Step 13: RAM refutes step 9. The bake must go to ROM after all.**

Step 9 moved the bake from build time to LOAD time to keep the asset pipeline
off the critical path, and flagged that the record pool had to be sized against
RAM before the player was written. Sized it. It does not fit.

A write record is a track id, a kind and seven Q fields: **32 bytes** padded.
Per animation that is (commands x tracks touched) records:

| commands | tracks | x10 concurrent animations |
| ---: | ---: | ---: |
| 8 | 2 | 5,120 B |
| 8 | 4 | 10,240 B |
| 20 | 2 | 12,800 B |
| **20** | **4** | **25,600 B -- OVER** |
| 40 | 4 | 51,200 B |
| 40 | 8 | 102,400 B |

The heap low-water mark is **24,404 B and already inside the 25,600 GObj-cap
threshold** (`ram-is-not-free`), and +14 KB of bss once stopped the ROM booting.
So anything past the smallest bracket is infeasible, and fighter animations are
not in the smallest bracket.

**Step 9 is withdrawn: the bake goes to ROM, at build time.** `PROJECT_GOAL.md`
explicitly trades ROM for speed -- tens or hundreds of megabytes are sanctioned
-- and ROM is the only budget with room for this. That puts the asset pipeline
back on the critical path: a generator, a blob, `EXPECTED_CENSUS_SHA256`
re-pinning, and the load-time relocation of the `interpolate` offset.

**What that does to the decision.** Slice 32 is now: build-tooling half plus
runtime half plus asset re-pinning, for **~7.8% of the gate gap**, in the
subsystem where a timing slip is a GAMEPLAY bug because the script calls
`func_anim`. The renderer is 26.32% of non-idle with `PROJECT_GOAL.md`'s
sacrifice order explicitly permitting cheap approximations there. **The ordering
recommendation stands and is now quantified on both sides.**

**Method note.** The table above was extracted wrong TWICE before it was right:
the first pass ended each case at the inner flags-loop `break;` and the second
tracked brace depth from the function rather than from the `switch`, and both
produced a map claiming "(none)" for opcodes that write seven fields. It was
caught by checking the output against the source read by eye. **A generator
spec derived by script needs the same adversarial check as any other
measurement.**

### The animation lane's CEILING, computed -- slice 32 cannot close the gate

The goal asks for animation to be materially reduced or its remaining cost
proven unrecoverable. Neither is quite true, and the useful answer is the
number in between: **what is the most this lane can ever return?**

Profile window is **1,799 presented frames** (4,028,886,502 cycles x 0.4993
ticks/cycle / 1,118,272 `ALL` P50 ticks per frame). Gate gap at P95 is
1,305,472 - 1,120,000 = **185,472 ticks**.

| item | ticks/frame | share of the 185,472 gap |
| --- | ---: | ---: |
| **animation, ALL THREE symbols** | **54,299** | **29.3%** |
| `ndsR2FtAnimParseDObjFigatree` | 16,806 | 9.1% |
| `gcPlayDObjAnimJoint` walk | 18,867 | 10.2% |
| `ndsR2AnimValueQ` | 18,627 | 10.0% |

**Deleting one hundred percent of fighter animation -- parse, walk, evaluation,
everything -- closes 29% of the gate gap.** No rewrite does that: the pose still
has to be computed, so `ndsR2AnimValueQ`'s 10.0% is largely irreducible work
rather than overhead. What the AOT dense-track rewrite actually removes outright
is the parse, **9.1%**, plus some fraction of the walk.

So slice 32 is worth roughly **9-15% of the gap** for a change spanning build
tooling and the hottest gameplay path. That is not nothing and the lane is not
"closed" -- but it is quantitatively incapable of reaching the gate, and it is
the wrong thing to build before the renderer's 26.32%. **This table is the
answer to "is animation recoverable": bounded at 29.3% best case, ~9.1%
realistically, and measured deletions in it disappear under a placement floor
(slice 31: -7,104 routed, +576 re-banked).**

### The lane question, answered: animation is NOT where the P95 lives

Grouped the cycle-117 whole-match census by subsystem, against **3,325,582,559
non-idle cycles** (4,028,886,502 total minus `armWaitForIrq`):

| subsystem | cycles | % of non-idle |
| --- | ---: | ---: |
| **RENDERER** -- `ndsRenderer*` + `ndsFighterMarioFoxDLAllDrawForSlot` | **875,140,091** | **26.32%** |
| ANIMATION -- `gcPlayDObjAnimJoint` + `ndsR2AnimValueQ` + `ndsR2FtAnimParseDObjFigatree` | 195,628,633 | 5.88% |
| SOFT FLOAT -- `__aeabi_fadd` alone | 112,198,280 | 3.37% |
| COLLISION -- `mpCollisionGetFCCommonFloor` + `ndsStageMPSweepFloorLoopSweep` | 67,617,177 | 2.03% |

**The renderer is 4.5x animation and 13x collision.** Cycle 117 spent six slices
between animation and collision -- together **7.9%** of non-idle -- while a
26.32% subsystem went untouched. Slice 31 is the honest illustration: it proved
a real deletion on a same-binary route (`WORK-H` P95 -7,104, controls flat,
`ALL` exactly 0) and the cross-build re-bank still read **+576**, because the
whole lane it came from is smaller than the placement noise floor is wide
relative to it.

**So the animation AOT rewrite (slice 32) should NOT be the next thing built.**
It is specified, its target is characterised, and it will still be worth doing --
but a rewrite spanning build tooling and the hottest gameplay path, to chase at
most 5.88%, is the wrong order of work while 26.32% sits in the renderer.
`PROJECT_GOAL.md`'s sacrifice order puts visual fidelity ABOVE gameplay fidelity
and 60 Hz simulation, so the renderer is also where the cheap approximations are
allowed to live -- reduced update rates, sprite substitutes, fewer transformed
parts, stage-specific tricks. None of those are available in the animation lane
without touching gameplay.

**Next action: re-profile the renderer cluster by symbol and pick its largest
deletable unit**, the same way `ndsR2FtAnimParseDObjFigatree` was reduced to "the
walk IS the call". The three biggest single symbols are already visible:
`ndsFighterMarioFoxDLAllDrawForSlot` 93,854,253, `ndsRendererCommitNativeStageSegment`
93,101,009, `ndsRendererNativeEmitProductionPrimitiveGroups` 81,420,680.

## HOT FOOTPRINT (2026-08-14) — the successor lane, and it SIZES ABOVE THE BAR

`artifacts/performance/2026-08-14_hot-footprint/HOT_FOOTPRINT.md`. Census only,
nothing built.

Of the **288,352 bytes in the 9,011 I-cache lines the match pays to fetch**:

| | bytes | share | removable |
|---|---:|---:|---|
| live (executed) | 213,040 | 73.9% | no — this is the work |
| literal pool (`[pc,#N]` targets) | 5,780 | 2.0% | **no** — Thumb-1 needs them |
| **cold code** | **42,892** | **14.9%** | **yes** |
| alignment padding | 26,640 | 9.2% | partly |

**Check the pool split before quoting the raw figure.** Raw "dead-in-line" is
26.1%, which reads as ~88,000 ticks; Thumb-1 emits constants into pools inside
`.text` that objdump shows as instructions and no PC profile ever reports, so a
naive count books them as free. Here pools are only 2.0%, so the lever survives —
but the confound had to be resolved first.

**Ceiling: needed 218,820 B = 6,839 lines against 9,011 fetched = 24.1%
reduction = ~81,800 ticks/frame.** Perfect compaction is unreachable at
basic-block granularity; a third to a half is **~25,000–40,000 ticks/frame**,
which clears the 17,000 bar. First lane this week to size above it on measurement
rather than inference.

**Two objects carry 42% of all fetch**: `scene_backend.o` (199,080 B text,
61,664 fetched, 47.3% executed) and `nds_renderer.o` (187,376 B, 55,008 fetched,
48.6%). Main text is 914,634 B of which only **43.4% is in functions that execute
at all**.

**NEXT EXPERIMENT: `-freorder-blocks-and-partition`** (cold blocks to
`.text.unlikely`) — exactly the transformation the 14.9% sizes. Three gates
BEFORE building: (1) it changes codegen, so this is **not** "same objects" —
Boundary must re-verify gameplay/collision/RNG; (2) total `.text` will likely
GROW while *fetched* text shrinks — correct trade, but price it against the
GObj-cap RAM threshold; (3) confirm `.text.unlikely` is non-empty in the map for
`-mthumb` ARMv5TE and that it reaches those two objects — **if empty, stop, no
build spent**. Primary evidence is the v3 `stall_icache_fill` delta, NOT WORK-H.

**Closed cheaply, do not re-propose:** GCC clone bloat is **168 bytes** total.
**Not the fetch lever:** 66,488 B of never-executed objects survive
`--gc-sections` (`mnplayersvs` 22,088 B at 0.0%, `mnvsresults` 15,348, `libc
categories` 14,420, `mnmaps` 7,388) — they cost ZERO fill because they are never
fetched; that is a RAM/ROM question, don't conflate it. **Oddity worth chasing:**
`_vfiprintf_r`/`_svfiprintf_r`/`__ssvfiscanf_r` are in the hot fetched set —
newlib formatted I/O executing during a battle match.

## CODE PLACEMENT — CLOSED ON MEASURED TEMPORAL EVIDENCE (2026-08-14, v3)

`artifacts/performance/2026-08-14_icache-temporal/ICACHE_TEMPORAL.md`.
**This supersedes the block below, whose verdict was WITHDRAWN under review**: it
argued capacity from a whole-match UNION footprint, which does not govern cache
behaviour, and asserted callee lines are "evicted between calls regardless"
without ever measuring the interval. Same word, invalid route.

**The v3 stall attributor was already in-repo at
`emulators/melonds-attributor/melonDS.exe`** (since 2026-07-27, emits
`profile-v3`). The prior block said it was never adopted; that was wrong, and the
instrument was available the whole time. Source also at
`D:\Stuff\DevFolder\melonDS-Accurate` (branch `r2-stall-attributor`, `4a1abf61`).

**Geometry, now VERIFIED** from the reference emulator
(`melonDS-Accurate/src/CP15_Constants.h:28-35`, `CP15.cpp:455-467`): 8192 B,
32 B lines, 4-way, 64 sets, `set=(addr>>5)&63`, **round-robin replacement**.
D-cache is 4096 B — different. melonDS names WAYS "sets"; `ICACHE_LINESPERSET`
is the real set count.

**THE POOL IS ENORMOUS AND REAL.** v3 on the c125 ROM, `stall_partition_residual=0`:

| class | cyc/frame | ticks/frame | share |
|---|---:|---:|---:|
| **icache_fill** | **678,551** | **339,275** | **29.7%** |
| dcache_fill | 504,064 | 252,032 | 22.0% |
| halt_wait (idle) | 477,575 | 238,788 | 20.9% |
| issue | 362,511 | 181,256 | 15.8% |

**Instruction fetch is 37.5% of non-idle and 1.87x `issue`** — ~20x the 17K target.

**But it is CAPACITY, and that is now MEASURED, not inferred.** Sweeping the
hot-line cutoff so set population varies:

```
top 256 lines -> sets that FIT (<=4 ways): 1,252 fill/1k | oversubscribed (5-8): 1,233
top 512 lines -> FIT: 1,407 | 5-8: 1,297 | 9-16: 1,403
```

**A set having room does not make its lines survive.** Uncontended sets refill at
the same rate as contended ones — marginally worse, in fact. Address conflict is
not the mechanism.

Phase 3 (skipped last time, and its absence is what made the old claim
unsupported) **does refute** the "evicted regardless" line: three hot clusters
FIT the cache (0.5–0.7x) and are scattered over 160–713 KB, and the hottest edge
`gcPlayDObjAnimJoint -> ndsR2AnimValueQ` (271.2 calls/fr) is 413 KB apart sharing
10 sets. **But those fixable clusters hold 0.14% of modelled conflict**; the
99.6% sits in clusters 1.4–4.1x the cache.

Layout model (`scripts/placement-layout-model.py`), all 5 gate conditions met:
`current 198,087,588 | cluster −2.1% | phase −2.1% | conflict-min −4.3% |
falsifier +27.8%`. **The first falsifier scored −2.3%, BEATING the principled
layouts** — it padded by a line, which spreads sets. Gate item 5 caught it; it
was rebuilt to align every base to the 2048 B set period. A model that rewards
any movement proves nothing.

**NEXT OWNER: HOT CODE FOOTPRINT REDUCTION — fewer bytes of hot code, not
better-arranged bytes.** 404,608 B of executed non-ITCM text against an 8,192 B
cache; `ndsR2AnimValueQ` spends **81.4% of its own cycles fetching itself**,
`ndsRendererMtxMulAffine20p12` 70.4%, `ndsRendererCommitNativeStageSegment`
57.1%. Note `.text.hot`+`.text.hot.draw` = 9,844 B is **already 1.2x the cache**,
so the curated set cannot be resident as it stands.

## (superseded) CODE PLACEMENT (2026-08-14) — verdict withdrawn

`artifacts/performance/2026-08-14_pgo-code-placement/PGO_CODE_PLACEMENT.md`,
`scripts/census-icache-placement.py`. **No linker order was changed, no ROM
built.**

ARM946E-S I-cache: 8 KB, 32 B lines (the line size is the one value confirmed
in-toolchain — `libnds/include/nds/arm9/cache.h:74`), 4-way, **64 sets, set
period 2048 B**. Measured against the whole-match profile, ITCM excluded:

```
executed non-ITCM footprint     404,608 B = 49.4x the I-cache
distinct lines actually fetched   8,596  = 275,072 B = 33.6x cache
hottest 2,000 lines over 64/64 sets; per-set max 40, median 32, min 20
sets with >4 hot lines (guaranteed conflict): 64/64
perfectly even spread would be 31.2
```

**There is no conflict pathology to fix — the distribution is already within
2.5% of optimal, against a 7.8x overflow of line capacity.** The two hottest
functions alone are 321 lines = 10.3 KB = 1.25x the whole cache and cannot be
co-resident in any order. Verdict is insensitive to the unverified geometry:
at 4/8/16/32 KB the overflow is 67.2x/33.6x/16.8x/8.4x — capacity-bound
throughout. Pettis-Hansen and set-conflict ordering both target a regime this
binary is not in, so Phases 3 and 5 were deliberately not built.

**Two durable consequences, both worth more than a shuffle:**

1. **Instruction fetch IS large and real** — the v3 attributor measured
   `icache_fill` 1,525,043 against `issue` 1,522,083, i.e. fetching the code
   costs about what running it does. The lever that reaches it is **shrinking the
   hot footprint, not rearranging it**: 404,608 bytes of executed text against an
   8 KB cache. That is a CODE SIZE problem wearing a cache costume, and it is the
   **named next architectural owner**.
2. **Adopt the v3 stall attributor before any further memory-lane work.** The
   shipped `melonDS.exe` emits `profile-v2` — nine columns, no stall partition
   (`strings` confirms; no v3 column names present). The attributor lives on
   `melonDS-Accurate` branch `r2-stall-attributor` commit `4a1abf61` and was never
   adopted. **Three lanes this week — D-cache layout, call-frame, placement —
   each ended needing a stall class the shipped profile does not carry.** One
   adoption unblocks all three.

## CALL-FRAME CENSUS (2026-08-14) — the largest lane nothing had measured

Full evidence: `artifacts/performance/2026-08-14_call-frame-census/CENSUS.md`.
Instrument: `scripts/census-call-frames.py` (no build; attributes every
`push`/`pop`/`stm`/`ldm` with a register list, plus `sp` adjustments, to its
enclosing function off the existing c125 profile).

**129,727 cyc/frame = 64,863 ticks/frame = 5.89% of the attributed frame, across
1,169 functions, is spent saving and restoring registers.** This board already
found it once, for `ndsR2AnimValueQ` ("6,683 cyc/frame, 16% of the evaluator, to
save and restore registers") and treated it as that function's property. It is a
property of the program.

The worst *ratios* share one shape: a hot early-out guarding a cold body, where
the body's register needs set the frame the early-out pays. `ftGetStruct` 38.9%
(7,141 cyc/frame — decomp resolves this in a one-load macro; the port pays a
six-register frame 246.3 times a frame for a stub builder the whole-match profile
executed **zero** times), `ndsR2AnimAObjToQ` 59.1%, `ndsRendererAdapterMaterialAnimHash`
34.3%, `gcParseDObjAnimJoint` 27.8%, `gcParseMObjMatAnimJoint` 27.0%,
`ndsBaseGcPlayDObjAnimJoint` 27.1%, `gcRunGObjProcess` 22.6%.

**`-fshrink-wrap` is on at `-O2` and cannot do this here.** ARMv5TE Thumb-1 has
no conditional execution, so an early exit cannot be predicated and the prologue
cannot be sunk past it. The fix is structural and free: move the cold tail into
its own `noinline` function. Same code, one branch further away, on a path the
profile never took.

**Shipped 2026-08-14** — the two whose equivalence is provable by inspection:
`ftGetStruct`'s stub split to `ftGetStructBuildStub`
(`push {r3,r4,r5,r6,r7,lr}` → `push {r4, lr}` in the shipped disassembly), and
`ndsR2AnimAObjToQ` inlined with `ndsR2AnimAObjToQConvert` out of line. Text
−2,184 bytes, so no arena cost. Predicted **−9,600 cyc/frame ≈ −4,800 ticks**,
which is **below the ±8,544 cross-build floor** — banked for correctness and
static evidence, NOT claimable as an A/B win. `check_anim_null_guard.py` green.

**LANE CLOSED 2026-08-14 — the ceiling does not convert.**
`artifacts/performance/2026-08-14_call-frame-slice/CALL_FRAME_SLICE.md`,
`scripts/census-frame-candidates.py`. The top 50 were classified against the
PUBLISHED ELF and the largest correctness-safe package is **10,544 cyc/frame =
5,272 ticks — 3.0x short of the 32,000 cyc / 16,000 tick gate.** Nothing was
implemented; the shipped ROM is untouched.

Three measured reasons the 64,863-tick ceiling is not bankable:

1. **A cold-tail split recovers the REGISTER DELTA, not the frame.** Fitting every
   frame instruction with ≥2,000 executions: `push(N) ≈ 1.6+1.2N`,
   `pop+pc(N) ≈ 5.0+1.6N`. **~9.3 cycles a call survive any split** — the `pc`
   load's pipeline flush is the floor. Only framelessness or call deletion takes
   the whole frame, and neither is available to a function that works on its taken
   path. `ftGetStruct` is the correction: −3,770 cyc, not −7,141.
2. **`ndsR2AnimValueQ` (6,919, the largest entry) is BLOCKED and must not be
   touched.** Its `noinline, target("arm")` is measured, not stale
   (`battleship_sys_objanim.c:311-323`): Thumb has no SMULL, so the Thumb arm
   emitted 11 `__aeabi_lmul` sites and measured **SRC P50 +17,728 / WORK-H P50
   +25,472**. Its 9-register push is the price of a change that already paid
   −25,472.
3. **The diagnostic reservoir is nearly empty.** 202 diagnostic-*shaped* names
   worth 98,871 cyc/frame contain **7,849 cyc of actual diagnostic**. The rest are
   load-bearing: `…RecordCapturedDisplay` (16,914) is the stage render hook,
   `ndsIFCommonRecordHUDState` (4,871) drives the on-screen HUD via
   `nds_platform.c:2616-2760`, the renderer `Record*` family (5,734) IS the RDP
   state machine, `syTaskmanCheckBufferLengths` (3,129) is the overflow guard.

**Method note that cost a false 14,800-cycle lead:** `codegraph_explore` returned
the `#else` (non-hwtri) bodies of `ndsRendererAdapterCommitNativeStageDisplay`
(`return FALSE;`) and `…MarkDisplayProcHeads` (`{}`). `nm` on the shipped ELF
shows 180 and 108 bytes — HEAD carries **two definitions of each** under
`#if NDS_RENDERER_HW_TRIANGLES`. Size candidates against the linked ELF, never
against a source read.

**The one thing still worth taking:** `ndsFighterDisplayContractCountFlags`, a
real **7,849 cyc/frame = 3,925 ticks** whole-call deletion (no runtime reader).
Take it only when something else is being built anyway — alone it cannot clear the
±8,544 floor. Gate the traversal, keep the globals `__attribute__((used))`, keep
the harness configuration computing them.

**Separate finding, larger than either cut, and NOT a codegen problem:**
`ndsFighterDisplayContractCountFlags` recursively walks the whole fighter DObj
tree twice per presented frame for **7,849 cyc/frame (3,925 ticks)** and computes
nothing the renderer reads — `gNdsFighterDisplayContractHiddenCount` and
`…NoTextureCount` are reset in `taskman_seam.c:3147` and read only by
`probe-ko-vfx.ps1` and `verify-battle-mariofox-gcrunall-loop-harness.ps1`. Gating
it out of the shipped configuration is worth more than both cuts above, but the
globals must stay `__attribute__((used))` and the harness build must keep
computing them: `--gc-sections` dropping a diagnostic global is what turned
Boundary RED on 2026-08-11.

## RAM CENSUS (2026-08-11) — measured on the PUBLISHED P1 ROM, with reader proof

Owner set the new target: free main RAM so animation residency can grow and
gameplay-time asset streaming disappears. Census re-run against
`smash64ds-battle-playable-hwtri.elf` — the ROM that ships
([[smash64ds-nds-not-p1]]) — not the tickhud build, which carries ~26 KB more
`.bss`. Artifact: `artifacts/performance/2026-08-11_ram-census/census.txt`.

`.main.bss` **1,597,744** · `.main.rw` 136,996 · `.main` code/rodata 902,816.
All `.bss`/`.data` symbols **1,743,144 B**. (`__sp_usr` is a linker-absolute
symbol, not an object — excluding it is required or it reads as 184 MB.)

| bytes | share | symbol | reader verdict |
|---:|---:|---|---|
| 441,600 | 25.3% | `gSYFramebufferSets` | **base touched by only 3 fns** |
| 204,800 | 11.7% | `sNdsAudioFgmCache` | live |
| 185,696 | 10.7% | `sNdsRelocSceneFileBuffer` | live |
| 153,600 | 8.8% | `sOriginalSpritePreview` | **LIVE IN BATTLE** |
| 153,600 | 8.8% | `sOriginalSpriteDisplayPreview` | **LIVE IN BATTLE** |
| 32,768 | 1.9% | `…HardwareTextureScratch` | lifetime-overlay candidate |
| 30,944 | 1.8% | `…Task36ReplayOwner` | audit lifetime |
| 16,384 | 0.9% | `…HardwareTextureRefreshLarge` | lifetime-overlay candidate |
| 13,824 | 0.8% | `sOriginalDLPreview` | **OPENING-ONLY — PROVEN** |
| 7,776 | 0.4% | `sOriginalDLDisplayPreview` | **OPENING-ONLY — PROVEN** |

Top 5 = 1,139,296 B = **65.4%**. Concentrated, not a thousand small globals.

**Three reader findings, all from the linked ELF and none from grepping source
([[linked-elf-is-the-reader-oracle]]):**

1. **The DL preview pair (21,600 B) is proven opening-only.** Its entire API —
   `ndsPlatformBeginOriginalDLPreview` / `Commit` / `Clear` — has exactly ONE
   caller in the shipped ROM: `ndsOpeningRoomRenderDLPreview`. The opening is
   explicitly out of scope for P1. This upgrades the prior "appears unused,
   needs configuration proof" to proven-by-attribution. **Take this first.**
2. **The sprite preview pair (307,200 B) is proven battle-LIVE.** Deletion is
   refuted: `ndsBattlePlayablePacingStart` executes `movs r0,#1` then
   `bl ndsPlatformSetOriginalSpriteOverlayEnabled` — battle turns the overlay
   ON. The open question is not "is it dead" but "why two 150 KB main-RAM
   images at once", which is an aliasing/lifetime question, not a deletion one.
3. **`gSYFramebufferSets` is more tractable than 20 readers suggested.** Only
   **three** functions hold its base address — `ndsBaseSCVSBattleStartScene`,
   `ndsBaseSCManagerRunLoop`, `lbTransitionSetupTransition`. The ~17 scene-start
   functions all hold `base+435,200`, a tail field, NOT the pixel buffers. So
   the 441 KB of pixel storage has a very small real user set and the
   architectural replacement is bounded work, not a 20-site refactor.

**Method caveat:** the literal-pool scan matches any constant landing in the
symbol's address range, so it produces false positives — `_svfiprintf_r` showed
up holding `0x02200172`. Attribute a hit to a plausible caller before believing
it; the three framebuffer-base holders were checked this way.


### ANIMATION'S OWN P95 NUMBER — 6.38% playback, and it CLUSTERS at 1.37x

Every prior animation verdict this campaign was a MEAN. This is the first
measurement of animation's contribution to the percentile the gate is defined on
(`--split-top-frames 80 --top 400`, full depth —
`artifacts/performance/2026-08-11_c118-lane/split-top80-full.txt`):

| lane | +cyc/tail-frame | % of premium | ticks/tail-frame |
|---|---:|---:|---:|
| animation **playback** | 83,523 | **6.38%** | 41,703 |
| animation **attach/asset** | 88,015 | **6.73%** | 43,946 |
| **animation total** | 171,538 | **13.11%** | 85,649 |

**And playback is NOT frame-uniform**, which is the assumption cycles 110-118
rejected their levers under. Tail-frame cost against control-frame cost:

| symbol | control | tail | ratio |
|---|---:|---:|---:|
| `ndsR2FtAnimParseDObjFigatree` | 28,701 | 50,946 | **1.78x** |
| `ndsR2AnimValueQ` | 39,754 | 53,519 | 1.35x |
| `gcPlayDObjAnimJoint` | 42,083 | 53,684 | 1.28x |
| `ftParamUpdateAnimKeys` | 21,469 | 27,831 | 1.30x |
| `gcPlayAnimAll` | 16,387 | 17,551 | 1.07x |
| **total (66% of the playback premium; 4 symbolsnames differ between tables)** | **148,394** | **203,531** | **1.37x** |

**What this does and does not overturn.** It does NOT resurrect cycle 117's four
rejected levers: the largest was ~1,900 tk/fr, and even at 1.78x that is 3,382 —
still under the 8,544 cross-build floor. Those rejections stand. What it
overturns is the *reasoning*: "animation is frame-uniform so its mean IS its P95"
is false, and any future animation lever must be sized at 1.28-1.78x its mean,
not at 1.0x.

**The parse's 1.78x is the interesting number, and it points at the arena.** The
frames where the parse costs 1.78x are the frames where
`ndsAObjEvent32NormalizeScript` (+30,172) and
`ndsRelocNormalizeFighterAObj16File` (+15,858) run — because a cache-refused
asset must be re-read, re-normalized AND re-parsed. So the animation tail and the
asset-streaming tail are substantially **the same event**, and fixing the 99.85%
arena is predicted to take the attach premium (88,015) and the parse premium
(22,245) together — **110,260 cyc = 55,053 ticks/tail-frame**. That prediction is
falsifiable and should be checked against the arena fix rather than assumed.

**Against the `/goal`'s exit conditions, stated plainly:**

- Condition 1 **is met**. Animation *was* materially reduced at P95 — Requirement
  4's fixed-point `AObj` banked `WORK-H` P95 **−37,504**, and that is in the
  current gate. And the next bottleneck is measured and elsewhere: asset
  streaming is **4.6-6.2x** animation playback's tail share.
- Condition 2 is **NOT** met, and should not be claimed. Playback still carries
  **27,530 ticks/tail-frame above control frames**. That is recoverable in
  principle; nobody has proven otherwise, and the mean-based evidence that used
  to look like proof has just been shown to be the wrong statistic.


### ★★ CONFIRMED: the anim arena saturates at 99.85% and 38 in-use assets are refused

> **SUPERSEDED 2026-08-13 — slice 46 fixed it; do not re-open.** The c123 bank reads
> `ArenaUsedBytes` **192,240 of 262,144**, `Overflows` **0**, `Rejects` **0**, `Misses` **2**
> (`artifacts/performance/2026-08-12_c123-rebank/SLICE46.md`): replacing the drifted warm list
> with the measured 87 ids SHRANK the arena by 64,960 B and the refusals went away with it.
> The 38 below is a dead counter on `build-c118-gate`, and it is **not** the `SHDT` band's 38
> runs (38 engagements) — the resemblance is coincidence, checked and closed in
> `artifacts/performance/2026-08-13_c-band-io/BAND_IO_OWNER.md` §4, which also shows the
> in-match FAT traffic is BGM packets plus the SOUND-EFFECT pack read, not the anim cache.

End-of-match counters, banked gate build `builds/build-c118-gate`, one-minute
both-CPU match, `-ExtraGlobals` (no rebuild —
`artifacts/performance/2026-08-11_c118-lane/anim-cache-counters.json`):

| counter | value | reads as |
|---|---:|---|
| `gNdsR2AnimCacheArenaUsedBytes` | **200,400** / 200,704 | **99.85% full, 304 B free** |
| `gNdsR2AnimCacheArenaOverflows` | **38** | 38 allocations refused |
| `gNdsR2AnimCacheRejects` | **38** | …and every one dropped the entry |
| `gNdsR2AnimCacheArenaOverflowLastSize` | **1,920** | it failed on a 2 KB request |
| `gNdsR2AnimWarmLoaded` / `…WarmFailed` | 83 / **0** | preload itself is healthy |
| `gNdsR2AnimCacheFills` | 8 | 91 entries of 128 — **bytes bind, not entries** |
| `…ReserveFailCount` / `…ArenaInvalidations` | 0 / 0 | arena stable, never rewound |

This is genuine saturation and not [[equal-counters-are-not-saturation]]: the
overflow counter is an event count, not a high-water mark compared to itself.

**Why this is the tail.** A rejected asset is one gameplay ASKED for. With no
eviction the rejection is permanent, so each of those 38 pays a FAT walk, a read,
a byte-swap and a normalize on *every subsequent use for the rest of the match*.
That is the FAT-reads + movers + attach + locks cluster, on scattered frames,
getting worse as the match runs — exactly the measured tail.

**Size of the gap.** 200,400 B over 91 entries is ~2,202 B/entry, so the 38
refused want roughly **~82 KB** more. Estimate only — the one observed refusal
was 1,920 B, below average — and the exact figure needs a summed rejected-bytes
counter, which is one `+=` and the obvious next instrument.

**~82 KB is NOT simply available.** The arena comes from
`gSYTaskmanGeneralHeap`, whose low-water is 24,404 against the 25,600 GObj-cap
threshold ([[ram-is-not-free-gobj-cap]]), and `…KEEP_FREE` already reserves
32,768. So the constant cannot just be raised. Real options, cheapest first:

1. **Trim the warm list.** It loads 83 assets and fills the arena; the 38
   refused are *proven in use* while some warm entries may never be touched.
   Needs per-entry hit counts — a small instrumented build, no design risk.
2. **Store one representation.** If both raw and prebaked forms are resident,
   dropping one is free capacity.
3. **Move the arena off the taskman heap.** `PROJECT_GOAL.md` explicitly wants
   RAM spent on speed (*"nearly all available RAM at 900K ticks is preferable to
   little RAM at 1.15M"*), so this is sanctioned if free RAM exists — unproven.
4. **Evict (LRU).** Last resort: it converts permanent misses into occasional
   ones, but a miss is the expensive event, so it only helps if capacity is
   genuinely unreachable.

**Caveat:** measured on the tickhud lab ROM, whose extra code and bss shrink the
same heap. The published ROM may have slightly more headroom — it has not been
measured, and it changes the size of the fix, not its existence.


### The mechanism that would produce that tail: the anim cache cannot evict

Read, not measured — `src/port/reloc_backend_assets.c`. The match already
preloads: `scVSBattleStartBattle` and `scVSBattleStartSuddenDeath` both call
`ndsR2AnimCachePreloadMatch`, which walks `sNdsR204AnimWarmList` through
`ndsR2AnimWarmLoadOne`. So "add a preload" is NOT the slice — one exists.

The cache is a **bump allocator with no eviction**:

- `NDS_R2_ANIM_CACHE_ARENA_BYTES` **200,704** (196 KB), `…ENTRIES` **128**.
- `ndsR2AnimCacheArenaAlloc` returns NULL once the bump cursor runs out
  (`gNdsR2AnimCacheArenaOverflows++`), and `ndsR2AnimCacheStore` then simply
  **rejects** the entry (`gNdsR2AnimCacheRejects++`).
- Nothing ever frees an entry. `ndsR2AnimCacheArenaRelease` only un-bumps the
  *immediately preceding* allocation on a failed load.

**So the failure mode is permanent, not transient.** The moment the arena or the
128 entries fill, every asset that missed is re-read from the ROM filesystem on
**every subsequent use for the rest of the match** — and each such use pays the
FAT walk, the read, the byte-swap and the normalize. That is exactly the shape
the tail shows: FAT reads + movers + attach + locks clustered on scattered
frames, growing as the match goes on.

It also means the fix may be a **constant**, not an architecture — if the arena
is merely undersized. That is settled by counters that already ship
(`gNdsR2AnimCacheArenaOverflows`, `…Rejects`, `…ArenaUsedBytes`,
`gNdsR2AnimWarmFailed`), read at end of match with `-ExtraGlobals`. No build.

**The fork:** overflows > 0 means the arena binds, and the lever is capacity
(bounded by [[ram-is-not-free-gobj-cap]] — heap low-water 24,404 against a
25,600 threshold, and `…KEEP_FREE` is already 32,768). Overflows == 0 means the
FAT traffic is NOT the anim cache and the loader that owns it is still unnamed —
do not assume, go find it. Either way, read the counter before writing code
([[answer-the-existence-questions-first]]).


### ★ THE P95 TAIL, MEASURED PROPERLY FOR THE FIRST TIME — it is ASSET STREAMING

`--split-top-frames 80` (added 2026-08-11 because no instrument could ask this),
c118 whole-match profile, frames ranked by NON-IDLE cycles:

| | frames | cyc/frame |
|---|---:|---:|
| the 80 costliest | 80 | 3,739,514 |
| control | 1520 | 2,430,868 |
| **premium** | | **1,308,645** (653,306 tk) |

Who owns that premium (top 40 rows cover 980,760 of it, 75%; the rest is spread
thin and unclassified). The tick HUD is **not in the published ROM**, so the
right-hand column is what a shipping tail looks like:

| owner | +cyc/frame | % raw | % excl. instrument |
|---|---:|---:|---:|
| measuring instrument (HUD/printf/scanf/console/locale) | 378,934 | 29.0% | — |
| **FAT/ROM reads** (`get_fat` `f_lseek` `f_read` `_read_r` `_nitroromFdRead` `_FAT_read_r` `validate` `strncasecmp`) | 120,264 | 9.2% | **12.9%** |
| **generic movers** (`memcpy` `armCopyMem32` `memset` `memmove`) | 96,699 | 7.4% | **10.4%** |
| **asset attach/normalize** | 78,229 | 6.0% | **8.4%** |
| **locks/threads** (`mutexLock/Unlock` `threadRemoveWaiter` `__libc_lock_acquire` `__getreent`) | 74,153 | 5.7% | **8.0%** |
| idle spin (a 3-VBlank frame waits longer — expected) | 85,735 | 6.6% | 9.2% |
| division helpers (mostly printf formatting) | 38,754 | 3.0% | 4.2% |
| **game logic + renderer, everything this cycle optimized** | **107,992** | **8.3%** | **11.6%** |

**The asset-streaming complex is 29.3%–39.7% of a published-ROM tail frame.**
The range is honest: the low end counts only symbols that are unambiguously the
FAT/attach path; the high end adds the generic movers, which are *probably* the
copy half of the same path but are shared helpers and were not proven to belong
to it. Either way it is the largest owner by a wide margin, and:

> **369,345 cyc/frame = 184,414 ticks on each of the 80 costliest frames.**
> **The gap to the gate is ~174,144.** This one class, removed, closes it.

**Animation and collision are inside the 11.6% row.** Cycles 110–118 ground on
the two lanes that together own about a ninth of the tail, because every
instrument available ranked by mean or by a threshold that turned out to sort
noise. That is the cycle's real lesson, and it is now fixed in the tooling.

**This is exactly the architecture `PROJECT_GOAL.md` already prescribes** —
*"Loading time is cheap. Gameplay CPU time is precious… A match may spend several
seconds preparing… if doing so substantially reduces active-match CPU cost."*
The milestone is ONE stage and TWO fighters; there is no streaming requirement
here at all. It also matches the prior art already in the tree:
`src/port/reloc_backend_assets.c:1850-1895` closes with *"move it off the
gameplay frame entirely, which changes WHEN the work happens instead of shuffling
where the code sits"* — written after E11, and correct.

**Next lever: preload the match's assets before the match, and stop touching the
filesystem during gameplay.** Before designing it, price the RAM — the heap
low-water is already 24,404 against the 25,600 GObj-cap threshold
([[ram-is-not-free-gobj-cap]]), so "load everything" may not fit and the slice
may have to be "load everything this match actually touches". The 80 frame ids
are in `artifacts/performance/2026-08-11_c118-lane/split-top80.txt`; the assets
they pull are the work-list.

**Still unmeasured:** the published ROM's own tail. Subtracting the instrument
from this table is arithmetic, not a measurement — removing 29% of the work also
changes which frames ARE the top 80.


### Slice 42 REVERTED — and the tail no longer has a single >=16K lever

Full evidence: `artifacts/performance/2026-08-11_mtx-route/FINDING.md`. Row-blocked
20.12 multiplies, bit-exact, one binary, four route values, engagement and
sameness of match proven in every arm: route 1 **−2,368**, route 2 **−5,184**,
route 3 **+6,912**. Not additive, P50 and P95 opposite-signed on route 2, nothing
near the ±8,544 floor. Reverted. Two mechanisms recorded there: an A/B route
inside an ITCM function must fit BOTH arms (the `s64 acc[4]` that bought 76 bytes
of `.itcm` put the accumulators in memory, 76→124 byte frame), and a multi-bit
route over hot code cannot be summed because the lab ROM pays I-cache for bodies
a shipped build would not contain.

**THE STRATEGIC RESULT, and it changes what the next cycle should do.** Cycle
119 attributed every arithmetic leaf on the true top-80 exactly and then worked
the ranking down. **No single remaining candidate is >=16K ticks:**

| candidate | tk/tail frame | status |
|---|---:|---|
| `ndsRendererMtxMulAffine20p12` | 19,175 | MEASURED sub-floor (slice 42) |
| `ndsRendererAdapterBuildDObjXObjMatrix` | 12,233 | open, 213 tk/call |
| `ndsRendererLoadHardwareSplitMatrices` | 11,172 | CLOSED — R2-03 E23, −3,008 |
| `ndsRendererMtxMul20p12` | 10,757 | MEASURED sub-floor (slice 42) |
| `…BuildPersistentStageWorldMatrix` | 9,555 | already per-frame + persistent memoized; the 584 tk/call IS the validation |
| `syMatrixLookAtReflectF` | 8,842 | fidelity-gated; `gGMCameraStruct` is Task 9 hashed, but the renderer's 2 of 4 calls pass a DISCARDED stack local and are not |
| `ndsRendererAdapterPrepareNativeStageOwner` self | 6,216 | open |

The two lane aggregates ARE over the gap — 20.12 kernels **62,891**, legacy float
camera **55,865** — but only as stacks. **So the next cycle should stop hunting a
single lever and batch several bit-exact deletions into ONE arm measured
cross-build**, where ~30K of stacked change clears the floor decisively. Slice
42's non-additivity says the batching must be one compile-time build, NOT a
multi-bit route.

### Cycle 120 re-attribution: ownership moved to the STAGE preflight

Full evidence: `artifacts/performance/2026-08-11_c120-lane/REATTRIBUTION.md`.
Post-slice-43 profile, same method, 100.0% reconciliation on every leaf.

`ndsRendererMtxMulAffine20p12` fell **54.2 calls/frame → 1.9** and both symbols
slice 43 targeted are **absent from the c120 census entirely**; `.text.hot.draw`
fell 113.8M → 55.7M cycles. Net named: 91.9M removed, ~60.9M added back — **the
FIFO writes and the capture cost 66% of what was deleted.**

**`ndsRendererAdapterPrepareNativeStageOwner` is now the largest legal candidate
at 18,912 tk/frame** — `MtxMul20p12` 9,769 and `BuildPersistentStageWorldMatrix`
9,143, both `tk prem` **0** and both 80/80, so perfectly flat. The compose half
cannot be deleted (the camera moves), but **the 9,143 is validation, not
building**: R2-02 arm C proved the 16 dynamic bindings' source keys match all
match long, so every one of those 16 calls a frame re-hashes and re-compares
transforms that never change. The fix is the shipped
[[bind-where-broken-not-where-read]] pattern — a generation bumped at the writer
instead of a key compared at the reader.

**Correction, and do not plan against the old number: the "legacy float camera
55,865 tk" lane is really ~13,700.** Attributing the actual camera symbols gives
`syMatrixLookAtReflectF` 4,174 + its `fmul` 2,325 / `fadd` 1,541 / `sqrtf` 1,798,
`syMatrixPerspFastF` `fdiv` 1,710, `syUtilsArcTan` `fdiv` 1,655, `syMatrixF2L`
`fmul` 1,279, `guMtxCatF` `fadd` 1,174, `syMatrixLookAtF` `fmul` 1,008. The c119
grouping swept in soft float belonging to other callers.

**Premium is a different question and the two must not be conflated.** Premium
(marked − control) is 1,375,838/frame, owned by the INSTRUMENT
(`ndsPlatformRenderDebugHud` 232,929 = 16.9%, plus printf/console ~170,000) and
asset streaming (~115,000). Table F's `tk/frame` is what a caller costs ON a tail
frame. P95 = P50 + premium, so a `tk prem` 0 cut lowers both equally.

**Slice 44 was planned as a five-part bundle; it shipped as one lever, and two
of the four leftovers were wrong.** (a) stage world revalidation ~9,143 and
(b) the 26-binding rigid guard ~3,700 became the stride and landed **−35,904 P95
on their own**, three times the bundle's whole prediction, because the pc-detail
showed the cost was D-cache traffic rather than the arithmetic the estimate
priced. (c) `noinline, cold` on `ndsRendererAdapterCaptureOwnerChainsGx` is
**VOID: its premise is false.** That function is not inlined into
`…DLAllDrawForSlot` — it is called from `PrepareNativeOwnerMatrices`, a different
TU with no LTO. `…DLAllDrawForSlot` did grow 7,396 → 8,556 bytes across slice 43,
but for some other reason; find it before spending a build. (d) the leaf-binding
scale fold (~2,100) and (e) evicting the dead 616-byte `MtxMulAffine20p12` from
`.text.hot.draw` remain open and are both sub-floor on their own — `.text.hot`
already has 3,604 free bytes, so (e) unlocks nothing that is not already
unlocked.

### Cycle-121 re-attribution — the stage lane is halved, and the new #1 is memory-bound

Whole-match census on `build-c121-profile` (`--split-top-frames 80
--attribute-leaves`, `timestamp_discontinuities=0`). Total cycles
3,981,830,319 → **3,947,099,258** (−34.7M = −10,838 tk on the *mean*, against
−17,088 at P50 and −35,904 at P95: the tail paid more than the mean, which is
what cutting work that clusters on heavy frames looks like).

`StageWorldSourceKeyMatches` 54.0 → **9.9 calls/frame**, 7,719 → **1,640 tk**;
`BuildPersistentStageWorldMatrix` 1,806 → **652 cyc/call**, 9,017 → **5,329 tk**.
`PrepareNativeStageOwner` now owns **14,801 tk/frame** (was 18,912), and the
`MtxMul20p12` 9,590 half of that is `world × view_projection` for the 16 dynamic
bindings — **the stride cannot touch it, the camera moves every frame**.

| tk/frame | tk prem | frames | caller | lane |
|---:|---:|---|---|---|
| **11,874** | 530 | 80/80 | `BuildDObjXObjMatrix` ← `BuildDObjLocalMatrix` | **fighter local build** |
| 9,590 | 0 | 80/80 | `MtxMul20p12` ← `PrepareNativeStageOwner` | stage compose |
| **7,187** | 288 | 80/80 | `memcpy` ← `BuildDObjXObjMatrix` | **fighter local build** |
| 5,253 | 198 | 80/80 | `BuildDObjLocalMatrix` ← `DLAllDrawForSlot` | fighter local build |
| 5,211 | 0 | 80/80 | `BuildPersistentStageWorldMatrix` ← `PrepareNativeStageOwner` | stage |
| 4,358 | 4,046 | 80/80 | `__udivsi3` ← `ndsPlatformRenderDebugHud` | instrument, excluded |

**The fighter LOCAL matrix build is now the largest legal lane at ~24,314
tk/frame**, 53.7 calls, 80/80, `tk prem` ≤530 — nearly flat. **But `--pc-detail`
says it is memory-bound, and that closes the obvious approaches.** 35,366,808
cycles over **324 distinct PCs**; top PC is **6.7%** (`ldr r2,[r3,#0]` at 27.55
cyc/insn) with no peak after it, and the expensive rows are all `ldr` at 11–28
cyc/insn on `[r4, #28/#32/#36/#64]`, i.e. DObj fields. The one hot loop
(`lsrs r3,r1,#23` — `MtxFromN64`'s exponent extraction, inlined) runs at 2.0–3.4.

So extending `BuildFighterTraRotRpyDirect20p12` to more kinds would delete the
float intermediate and the conversion but **still read the same DObj fields**,
which are 60%+ of the cost. Price that before building it. And the lever the
shape points at — a local-matrix memo — is **DO-NOT-RETRY, built and killed
twice**; the Task 91 comment at `reloc_backend_renderer_dl.c:1790` argues for it
anyway and is not an invitation.

**Do not plan a slice against `memcpy`'s 49,671 tail-frame ticks.** Only three
callers clear the 40/80 presence bar (7,187 + 1,883 + 1,838 = 10,908); the other
~38,700 is spread across callers each below it. Its mean self-time is 16,912.

### FIXED-POINT COLLISION — **LANE CLOSED 2026-08-15.** Exchange rate 2.68, and the whole lane is 0.47x the requirement even at a rate of zero

Evidence: `artifacts/performance/2026-08-15_cfx-narrow-exchange/EXCHANGE.md`
(prediction written first in `PREDICTION.md`) and `MENU.md`. 2 lab builds
(`build-c181-cfxnarrow-b-d0`, `build-c182-cfxnarrow-a-d0`), 2 v3 captures, 2
same-binary gate runs. **Both new flags default 0, so every published target is
byte-identical to HEAD's; both root ROMs unchanged and not rebuilt.**

**Slice 53 converted the CONSUMER.** The briefed target
`gmCollisionGetWorldPosition` **does not early-exit** (`gm/gmcollision.c:196-205`
is nine `f32` multiplies and nine adds, no branch) and **cannot be intercepted**
— fifteen in-TU call sites, and the `#define`-before-`#include` rename moves the
definition *and* the callers together (the trap
`battleship_gmcollision.c:167-169` already documents). The one interceptable unit
containing both bodies is `gmCollisionCheckFighterAttackDamageCollide`'s tail
(`:1379-1400`), so that is what ran in fixed point: `ndsR2CfxTestRectangle` plus
the **two** `GetWorldPosition` calls it makes, behind
`gNdsCfxNarrowEnable` — a second one-byte pair (`.main.rw` **1** differing byte
at `0x3F24`, everything else **0**).

```text
WHOLE MATCH B - A   issue -364 · icache_fill +2,459 · net non-idle +2,228 tk/fr
RANK-80             net non-idle +29,290
SAME-BINARY ROUTE   -SetGlobals gNdsCfxNarrowEnable=1|0 on ONE build:
                    WORK-H rank-80 1,222,848 vs 1,194,368 = +28,480  (2.8% apart)
                    P50 -384 (flat), 2-VBlank 1,835 vs 1,843 of 2,038
EXCHANGE RATE       fixed +3,392 / float -1,264 = 2.68  (whole match, mask-free)
```

**Three closures, none depending on the others.** (1) Rate: producers 1.001
straight-line, consumers **2.68** early-exiting — neither below 1.00. (2)
**Ceiling: the identifiable float in the whole fighter narrow phase is 15,217
tk/fr at rank-80 = 0.47x of +32,593; whole match its soft float is 840 of a
59,694 tk/fr bill, 1.4%. Free would not have been enough.** (3) Best remaining
case — resident `unk_dobjtrans_0x9C` plus the DS hardware divider — prices at
**≈1.29**.

**The mechanism is nameable and it is not only cache.** The fixed form calls
libgcc's **64-bit divide 4.0 times per entry** (`__udivmoddi4` 7.76 → 11.65
calls/fr), a bit-by-bit loop, worth **+988 whole / +17,377 at rank-80** — more
than the entire float bill it deletes. `nds_r2_collision_fixed.h:210-217` offers
`NDS_R2_CFX_DIV64`/`ISQRT64` overrides for the DS hardware unit and **nothing
ever defined them** — DEFINED 2026-08-16, the divide measures 2.76x–4.50x cheaper and the
ring's rate becomes 2.19–2.08, which is still a loss. Meanwhile `__mulsf3` pays **0.77 tk of fetch per call** at
1,545 calls/frame: the soft-float library is permanently I-cache resident and a
kernel entered 0.97 times a frame is cold every call.

**Correctness is the part worth keeping.** Flip budget stated ZERO before the run
and measured ZERO: `DamagePhaseCalls` 1,938 / `Hits` **20**, P1Damage 76, spark
15, shield 1,352, AObj 1,266, packHits 197, runaway 0 — **all equal to the
`c170`/`c174`/`c175`/`c176` bank on both arms** — with `gNdsCfxNarrowCalls`
**1,938 == DamagePhaseCalls**, `Answered` **1,938**, `Hits` **20 ==
DamagePhaseHits**, `Declined` **0**, and all four **0** on the control. The
collision **decision itself** ran in port fixed point for 1,938 of 1,938 pairs
and reproduced the decomp float outcome exactly.

**Retraction — this cycle's own written-down prediction, wrong on both halves.**
Predicted +560…+7,890 marginal and +50…+670 whole; measured **+29,290** and
**+2,228**. The *prize* half was right (predicted ≈15,000, measured 15,217); the
*price* half was **2.2x low** because it priced the kernel's own bytes at 0.467
tk/byte/call and never counted the library the kernel calls. Third distinct
static-quantity mispricing in this campaign, after a residual ÷ a count and a
static size ÷ a count.

`FOOTPRINT.md` §5's 9x straddle is answered, and **neither bound was binding**:
the fixed `TestRectangle` measures **0.13 tk/byte/call** — the early-exiting end,
as predicted — and `MakeFrame` + `__udivmoddi4` are **11.7x** its cost.

**What stays.** `NDS_R2_COLLISION_FIXED` and the new
`NDS_R2_COLLISION_FIXED_NARROW` are both `?= 0`. The kernels, the falsifier
(GREEN: 88 long multiplies, soft float confined to `BuildLocal`) and both wirings
are retained as the evidence. **What is retired is the direction** — "convert
collision to fixed point to buy P95" — not the code.

### Slice 52 — PLACEMENT REFUTED BY ARITHMETIC, the clean concentration is 11.7x, and `5.21x` is RETRACTED as a decision rule

Evidence: `artifacts/performance/2026-08-15_cfx-ring-draw0/FOOTPRINT.md`. 2 lab
builds (`build-c179-cfxring-b-d0`, `build-c180-cfxring-a2-d0`), 2 v3 captures,
**0 production change, 0 Makefile edit, 0 Boundary run needed** — the only
tracked source edits are two standalone analysis scripts no build imports. Both
root ROMs unchanged and not rebuilt.

**The instrument, a second one-byte pair, this time at `NDS_TICK_HUD_DRAW=0`:**
`.itcm`/`.text.hot`/`.text.hot.draw`/`.main`/`.dtcm` **0 differing bytes**,
`.main.rw` **1** at `0x3F24` = `gNdsCfxRingEnable`. Asserted by
`scripts/compare-elf-sections.py`, which **refuses** on a missing/empty/non-ELF
input, an absent or zero-length section, a failed `objcopy` and a size mismatch
— the two comparisons that could not fail are now inexpressible, and it was
falsified against the c177/c178 pair before use.

**Whole window, 1,601 regions, B − A′, ticks/frame:**

```text
issue -1,771 | icache_fill +1,801 | dcache +54 | wbuf -27 | interlock +9
bus -63 | instructions -621 (a COUNT)      NET NON-IDLE +3
marginal 80: issue -19,119 | icache_fill +20,112      NET +441
```

**`+284` IS RETRACTED** — it was HUD/printf noise the `DRAW=1` pair could not
separate. On the arm the cadence gate is read from the wired ring is
**tick-neutral to +3 tk/fr**.

**THE EXCHANGE RATE IS THE ANSWER AND IT IS 1.00.** Fixed added +2,705 whole /
+32,010 marginal against float deleted −2,702 / −31,569 = **1.001 and 1.014**,
and **both sides concentrate identically — 11.68x deleted, 11.83x added.** A
1.00 exchange rate multiplied by any concentration is still 1.00, so
**`SPLIT.md`'s "it clears at ≥5.21x" is RETRACTED as a decision rule**: it
applied ONE factor to the NET, which is valid only if the price stays put while
the prize concentrates. **The measured concentration is 11.7x — 2.2x above that
bar — and residency does not follow from it.**

**Task A, placement, REFUTED WITHOUT A BUILD, five ways.**

1. `linker/nds_hot_text.ld:180-200` banks **two builds that measured the
   opposite sign**: Task 94 moved a 500 B member OUT for **WORK-H P50 +6,144**;
   R2-03 E65 admitted a **2,032 B** callee predicted at −7,894 and measured
   **P95 +24,448**, over-gate 7→8. Two different estimators, both wrong on sign.
2. `ICACHE_TEMPORAL.md` §6 measured the mechanism as **capacity, not conflict**.
3. The I-cache set period is 2,048 B, so a contiguous **5,596 B block is 2.73
   set periods** and covers all 64 sets at least twice **at any address**.
4. `icache_fill` 712,877 cyc/frame at 23–51 cycles per 32 B line =
   **55–121 complete cache turnovers per presented frame**, 5–11 of them between
   consecutive narrow-phase entries.
5. **Decisive: the ring's price is charged per ENTRY and does not fall when
   entries get 11x denser** — 2,668 / 2,596 / 2,755 / **2,961 ticks per entry**
   at 0.97 / 4.14 / 0.97 / **10.66** entries per frame. Cost concentration equals
   CALL concentration twice over (11.82 vs 11.57 on `DRAW=0`; 4.15 vs 4.16 on
   `DRAW=1`). Ceiling on what any layout could recover: **+219 tk/fr whole match
   ⇒ ≲2,600 at rank-80 = 0.08x.**

**The mask correction, measured twice on the float control.** The same eight
bodies read self-cost concentration **2.91x on `DRAW=1` and 7.04x on BOTH
`DRAW=0` captures** — `c172` is a different build at a different HEAD and agrees
to three significant figures, so **2.42x is a property of the mask, not the arm.**
Calls: 3.49x → 8.66x/8.84x. **And the P95 frames are not "more fighter procs"** —
`battleship_ftMainProcUpdateInterrupt` costs 1.05x there and is called 1.03x,
while `gmCollisionCheckFighterAttackDamageCollide` goes **0.97 → 10.47
calls/frame (10.81x)**. **The fighter narrow phase IS the P95 owner, at 11x.**

**The only measured way to move a 1.00 rate is BYTES, and it cost no build.**
`nds_r2_collision_fixed.c` compiled standalone at **`-Os` instead of `-O2`**
(same `-marm`, same TU, same config header) is **7,916 B → 5,228 B, −34.0%**
across its ten entry points; undefined-symbol set identical, **no
`__aeabi_lmul`**, SMULL 62→58 / SMLAL 26→18. **NOT BUILT: ~−700 tk/fr whole
match is ~23x under `plan.md` §2's ≥16,000 build floor**, so it rides a package.
**Consequence found and not fixed:** at `-Os` GCC outlines `ndsR2CfxCosQ15`,
moving the declared soft-float edge out of `ndsR2CollisionFixedBuildLocal` and
tripping `scripts/check-r2-collision-fixed.ps1`. Fix with `always_inline` on the
two table helpers, **not** by widening the allowlist.

**NO RESIDENT NUMBER IS PUBLISHED, on purpose.** §5.4's estimates are replaced by
measured compiled sizes — `TestRectangle` **1,504 B** not ~1,400,
`TransformPoint`+`WorldToLocal` **208 B** not ~300, and the `TestSphere`
expansion prologue costs **zero** fetch because it is never entered — but the
deciding rate is not determined. The converted half is straight-line matrix code
that touches all its bytes every call; `gmCollisionTestRectangle` early-exits and
reads **0.052 tk/byte/call against `StoreF32`'s 0.467, a 9x spread**, so the two
bounds straddle the requirement. **§5.4's −6,261 and §5.5's 0.60x stand as the
previous cycle's, neither confirmed nor replaced.**

**NEXT MEASUREMENT, and it is not another concentration capture:** convert ONE
early-exiting body and read its exchange rate off the same one-byte `--diff`.
`gmCollisionGetWorldPosition` is the cheapest — **196 B float against a 100 B
fixed `TransformPoint`**, 19.29 calls per marginal frame, ~30 call sites
funnelling through one helper. Below 1.00 and residency is worth building; at
1.00 the lane closes on arithmetic rather than on cache.

**Tooling, and a published number corrected.**
`census-marginal-frame-owners.py --concentration` prints COST and CALL
concentration side by side (call counts from the entry PC, so exact). Its
`--diff` was dividing the `instructions` COUNT by `2 x frames` like a cycle
total: **`SPLIT.md`'s "204 fewer instructions per frame" is corrected to −407**,
and the halved form is no longer printable.

**Caveat stated rather than buried:** `DRAW=0` is cleaner, not clean.
`NDS_TICK_HUD` is still 1, so `_svfiprintf_r` 5.58x / `consolePrintChar` 5.26x /
`_vfiprintf_r` 4.99x still sit on the mask (~8,800 tk/fr). Collision concentrates
more than twice as hard, which is exactly what `DRAW=1` could not say.

### Slice 52 — THE NULL IS SPLIT AND IT IS I-CACHE. The resident version is sized at 0.60x and clears only at a concentration ≥5.21x

Evidence: `artifacts/performance/2026-08-15_cfx-ring-split/SPLIT.md`. 2 lab
builds (`build-c177-cfxring-b-prof`, `build-c178-cfxring-a2-prof`), 2 v3
captures, **0 production change, 0 Boundary run needed** — the only tracked
source edit is a new mode in a standalone analysis script no build imports.
Both root ROMs unchanged and not rebuilt.

**The instrument.** Profile ROMs (`NDS_TASK37_PROFILE=1`, 1,600 frames from 438,
per-frame regions) carrying the row above's flags, differing in **exactly one
byte** — `.itcm`/`.text.hot`/`.text.hot.draw`/`.main`/`.dtcm` **0 differing
bytes**, `.main.rw` **1** at 0x3F24 = `gNdsCfxRingEnable`. **The placement floor
is zero on this pair too.** *The brief said this needed no rebuild; it did — the
census window is a compile-time constant and `c175`/`c176` carry
`NDS_TASK37_PROFILE 0`.*

**The split, whole window, B − A′, ticks/frame over 1,601 regions:**

```text
issue -1,717 | icache_fill +1,854 | dcache_fill +161 | instructions -204 | net non-idle +284
```

The six classes sum to +284 exactly (`stall_partition_residual` 0 / −42).
Per-PC group decomposition: the ring's eight symbols add `issue` **+12** and
`icache_fill` **+2,110**; the displaced float bodies and libgcc give back `issue`
**−1,852** and icache only **−475**; the rest of the binary nets **+219**.
**So it is compulsory fetch of the ring's own instructions, not eviction damage
to neighbours — CANDIDATE 2, and the arithmetic win was real all along.**

**CANDIDATE 1 is refuted as arithmetic and re-cast as footprint.**
`StoreF32`+`LoadF32` are 964 B costing **+588 tk/fr**, of which **+516 (88%) is
`icache_fill`** and **−12 is `issue`**. RING.md's "129 ARM instructions for
twelve, ~11 each" divided a *static size* by a count: the body is two loops and
the common path is **38 ARM instructions per fixed→f32 value, 22 per f32→fixed**
off the disassembly. The instructions were never the price.

**R2-07 L7's 1.85 cycles/frame/byte is not a constant — measured here 0.754**
(2,110 tk/fr over 5,596 B). Do not carry it into another sizing; carry the
method.

**Exact call counts, free, from the entry PCs**, A′→B over 1,600 frames:
`func_ovl2_800ED490` 2,471→1,177 · `gmCollisionTransformMatrixAll` 4,157→3,115
(**only 25% taken**) · `gmCollisionSetInvertMatrix` 937→141 ·
`func_ovl2_800EDBA4` 1,133→337 · **`lbCommonSin`/`Cos` 37,615→34,489, i.e. 8.3%
— sin/cos was never a collision lane** and `PREDICTION.md` §2 booked ~4,975 tk/fr
for it · `sqrtf` 68,289→65,900 (3.5%) · `func_ovl2_800EDE5C` 1,693→1,693 at
22 tk/fr because the ring set `0x6` and it early-returns. Same fight on both
arms: `gmCollisionCheckFighterAttackDamageCollide` **1,552** and
`gmCollisionTestRectangle` **1,693** on each.

**THE EXCHANGE RATE IS THE WHOLE STORY: the fixed replacement costs 0.987x the
float it deletes** — +2,726 against −2,762 tk/fr, i.e. **0.4871 tk/fr per byte of
executing fixed ARM text against 0.4937 removed per byte.** A resident version
wins only by moving the second number.

**Task B — resident representation, DESIGN ONLY, not built.** Removing the whole
narrow phase is **9,865 tk/fr** whole match (self 4,717 + soft-float leaves 6,544
− the 33.7% of sin/cos not collision-driven), at ~7,400 B of executing ARM text =
3,604 cost. **Net −6,261 tk/fr whole match; at this cycle's MEASURED 3.11x
percentile concentration that is +19,470 at rank-80 = 0.60x of +32,593.**
**It clears at 5.21x and not below**, and 5.2–11.7x is the presence range
`…/2026-08-15_k1-owner-pricing/` §5 reports for these bodies on a clean `DRAW=0`
mask. **The bar is straddled; one `DRAW=0` re-run of this same byte-identical
pair settles it with no new code and no new correctness obligation.**

**Correction the board owes itself: "fifteen referrers" belongs to
`func_ovl2_800EDBA4`, not to `mtx_translate`** (`…/2026-08-13_c-collision-seam/`
`elf-referrers.txt`). Ten of the fifteen are inside the cluster and call the
*function*; the *field*'s ~49 readers funnel through `gmCollisionGetWorldPosition`
(~30 sites, one helper), `gmCollisionCopyMatrix` and `func_ovl2_800EDA0C`.
**Nothing in that set blocks residency.** The renderer is a net WIN there — it
already converts `mtx_translate` to 20.12 fixed via
`ndsRendererAdapterF2LFixedWExact`, and the ring's translation row is already
Q12. `gmCollisionTestSphere` executes **zero** times on both arms, so STACK.md
§5.1's `*p_angle` obligation is discharged by a fixed→f32 expansion prologue on
unreachable code, graded as an expansion rather than as a body.

**UNPRICED, and the cheapest test left in this lane: placement.** All 5,596 B sit
in `.main`. `.text.hot` has **3,604 B free** of its 8,192 cap and
`.text.hot.draw` **2,924 B**. One build, one A/B, no new arithmetic, against a
measured +2,110 tk/fr of compulsory fetch.

**Task C, free negative evidence.** Both 1,600-frame v3 windows contain **zero**
regions ≥ 2^22 ticks (max 3,361,733 in both, same region 1558) and
`stall_cart_spin` **0** throughout. The multi-megatick outlier is therefore not
visible to the instrument that accounts for every emulated cycle, on the same
configuration and match — a narrowing toward the tick-HUD's own `cpuGetTiming()`
reader (`nds_platform.c:353`, libnds's cascaded TIMER0/TIMER1 pair read
non-atomically), **not an answer**. Expectation over 3,200 frames was ~1.5
events. The `-PerStopGlobals` probe and the +23,040 counter-arm residual were
**not** run.

**Tooling.** `census-marginal-frame-owners.py --diff` is new: a stall partition
is only readable as a *difference*, and the mode joins two reduced per-PC
censuses **on the program counter**, so it verifies identical layout from the two
ELFs' `nm` address→name maps and refuses otherwise. A per-PC diff across a
relink is now inexpressible rather than documented. **The same cycle re-caught
RING.md §3's trap in a new costume**: an `objcopy --only-section` comparison
against a build directory that did not exist reported **0 differing bytes for
every section**. A byte comparison that cannot fail is not a control; the loop in
SPLIT.md §4 fails closed on an objcopy error and on a zero-length section.

### Slice 52 WIRED, MEASURED, TICK-NEUTRAL — the collision decisions did NOT move and neither did the gate. `+30,000…+38,000` is RETRACTED

Evidence: `artifacts/performance/2026-08-15_cfx-ring-wiring/RING.md`, with
`PREDICTION.md` beside it written before the first build. 3 builds, 3 gate runs,
1 Boundary. **Both root ROMs unchanged** — `NDS_R2_COLLISION_FIXED` still
defaults to 0, so the ring is compile-time absent from every published target.

**What was wired.** The three PRODUCERS of the fighter narrow phase in fixed
point behind an f32 boundary — `func_ovl2_800EDBA4`'s chain interior (which
carries `gmCollisionTransformMatrixAll` and `func_ovl2_800ED490` with it),
`func_ovl2_800EDE00`'s inverse into `unk_dobjtrans_0x9C`, and
`func_ovl2_800EDE5C`'s axis scales into `vec_scale`. `gmCollisionTestRectangle`,
`gmCollisionTestSphere` and `gmCollisionGetWorldPosition` were **not** touched:
keeping `unk_dobjtrans_0x9C` an f32 `Mtx44f` leaves every collision DECISION in
decomp code on decomp comparisons, which is what let this be graded by a bound
instead of argued — and it removes STACK.md §5.1's obligation to convert
`gmCollisionTestSphere`, whose `*p_angle` no flip count can express.

**MEASURED, A/B/A' on a pair whose ROMs differ in EXACTLY ONE BYTE.** The usual
flag falsifier cannot work here (at flag 0 the objects leave the link, so the
"candidate layout" arm is the control layout again). `NDS_R2_COLLISION_FIXED_DISPATCH`
flips one initialised word of `.data` read through a `volatile`;
`objcopy --only-section` + `cmp -l` on `build-c176-cfxring-a2` vs
`build-c175-cfxring-b` gives `.itcm`/`.text.hot`/`.text.hot.draw`/`.main`/`.dtcm`
**0 differing bytes** and `.main.rw` **1**, at offset 0x3F24 = exactly
`gNdsCfxRingEnable`. **There is no placement floor on this comparison.**

| | A' c176 (off) | **B c175 (ON)** | A c174 (off) | B − A' |
|---|---:|---:|---:|---:|
| `WORK-H` P50 | 942,336 | **942,400** | 942,272 | +64 |
| P90 | 1,094,976 | **1,094,912** | 1,094,080 | −64 |
| P95 | 1,173,120 | **1,174,016** | 1,173,376 | +896 |
| **rank-80** | 1,173,696 | **1,177,344** | 1,173,760 | **+3,648** |
| top-1% | 1,513,920 | 1,517,504 | 1,511,936 | +3,584 |
| VBI 2/3/4/5+ max | 1736/285/8/9 · 19 | 1741/278/9/10 · 19 | 1744/275/11/8 · 19 | |

The two controls bracket to ±896 at P95 and ±64 at rank-80; the candidate sits
3,648 **outside** that bracket at rank-80. **It is a small COST, not a saving,
against +32,593 required — 0.00x.**

**It is not inert, and that is the point.** Nine counters, all zero on both
control arms: `gNdsCfxRingPrepareCalls` **1,938** (== `…DamagePhaseCalls`
exactly), `ChainFixed` **1,006**, `LocalsBuilt` **1,314**, `Composes` **1,621**,
`InvertFixed` **1,006**, `ScaleFixed` **1,006**, and **`ChainDeclined` /
`InvertDeclined` / `ScaleDeclined` all 0** — not one of the five domain guards
fired in a whole both-CPU match, which is what the widened live-domain re-grade
predicted.

**ZERO collision decisions changed, and the flip budget said zero before the
run.** `…DamagePhaseCalls` 1,938 / `Hits` **20** identical on all three arms, as
are `P1Damage` 76, `DamageSparkScaleCount` 15, `ShieldAnimJointAttachCount`
1,352, `AObjEvent32NormalizedHighWater` 1,266, runaway 0, heap low-water 52,864,
arena fails 0, `BattlePackHits` 197 — and all of those match the `c170-seam-bp1`
bank arm too. **The fixed-point producers reproduce the float producers'
gameplay outcome exactly over a whole match; keep that result even though the
ticks are not keepable.**

**WHY zero — two candidates, NEITHER MEASURED, and the discriminator is free.**
By the run's own call counts against the profile's measured float prices (one
compose 1,290 ticks, one inverse 1,297, `TransformMatrixAll` 512 + its `lbCommonSin`/`Cos`
share, `EDE5C` ~787/working call) the expected whole-match saving is ~2,415 tk/fr
at P50; ~2,500 is unaccounted for.
1. **The f32 boundary** — the counters put it at ~90,000 scalar conversions per
   match (12×`LocalsBuilt` + 12×`Composes` + 27×`InvertFixed` + 15×`ScaleFixed`
   + reloads). But `nm` says `ndsR2CollisionFixedStoreF32` is 129 ARM
   instructions for twelve conversions, ~11 each, so this is the **weaker**
   candidate.
2. **The I-cache footprint of 5,596 bytes of new ARM text that now EXECUTES** —
   R2-07 L7's documented failure mode, reproduced with a better instrument. L7
   measured 2,332 added bytes at 1.85 cycles/frame per byte; at that rate 5,596
   is ~5,175 tk/fr, which more than absorbs the arithmetic win. Arm A *links*
   those bytes without executing them and reads **−4,288 against the `c170`
   bank**, so linking is free and only executing is not.

**A v3 stall capture on arm B against arm A' splits them in one 560 s run with
no rebuild**: candidate 2 predicts `icache_fill` up and `issue` down; candidate 1
predicts both flat with the change inside `issue`. That is the next read.

**What this closes.** *"Give `func_ovl2_800EDBA4` a fixed-point interior with an
f32 boundary and collect +30,000…+38,000 at rank-80"* is **retracted** — mine
and this board's, retracted here rather than in a later cycle. And the header's
own design sentence, *"the representation crosses the float boundary exactly
twice per joint per frame"*, is **not achievable while `mtx_translate`,
`unk_dobjtrans_0x10`, `unk_dobjtrans_0x9C` and `vec_scale` all stay f32**: this
wiring crosses it four to six times per joint per frame because every
intermediate is stored back into an f32 field a decomp consumer reads. Any
version of this cluster that can pay must keep the fixed representation
**resident** across the narrow phase — the `unk_dobjtrans_0x9C` reinterpretation
this board already proved available **and** a fixed `mtx_translate`, which the
seam correction ruled out on its fifteen referrers. Larger, and now with a
measured reason to exist rather than a projected one.

**Kept regardless of the ticks**, all behind the default-0 flag:
`scripts/check-r2-collision-fixed.c` re-graded at the **live** domain
**0.9937–2.0479** (the stale 1.1138–1.1199 is gone) and GREEN — chain 0.0017143
depth 6 / 0.0024254 depth 12, cofactor frame 0.0011086 / 0.0017125, `vec_scale`
0.0001224, **`TestRectangle` decisions 0 mismatches in 300,000**, smallest margin
0.0002151; a new **T8 section that grades the WIRING** end to end (world
0.0017395 / local 0.0010529 / `vec_scale` 0.0001224 at depth 6, world 0.0022583
/ local 0.0014343 at depth 12, `unk_dobjtrans_0x10` round-tripped through f32 on
every level); the nine engagement counters; and the byte-identical-dispatch
falsifier, which is what made this a verdict instead of an argument.

**Two traps worth carrying.** A first section check ran `--only-section=.text`
on both ELFs and reported IDENTICAL — it was the SHA-256 of the **empty string**,
because this linker script has no `.text` (`.itcm`, `.text.hot`,
`.text.hot.draw`, `.main`). *`prove-the-control-differs` applies to a byte
comparison as much as to a run.* And `gNdsCfxRingEnable` first landed in `.bss`
on the dispatch-0 arm and `.data` on the dispatch-1 arm, which moved
`.text.hot`/`.main`/`.main.rw` between them; `section(".data")` pins it. Both
were caught by reading the symbol table rather than assuming.

### Slice 53 — FGM cache REVERTED on its own falsifier; `FindPlanned` REFUTED; slice 52 re-briefed. Gate unchanged at 1,210,944

Evidence: `artifacts/performance/2026-08-13_c-collision-stack/STACK.md` and the
four arm JSON/rows beside it. **Nothing was kept**; the committed tree's
`.text`/`.data`/`.rodata` are byte-identical to the control's, so the gate stays
at **1,210,944 raw / ≈1,185,997 net, gap +90,564 / +65,617**. Both root ROMs
untouched. The control on current HEAD (`build-c147-ctl`) reproduced the banked
figure to the tick on a *different* ROM SHA, so the bank is now measured on this
tree rather than inherited.

**`ndsAObjEvent32FindPlanned` does not have the cost slice 51 attributed to it.**
Instrumented, whole one-minute both-CPU match: **1,188 entries** — 1,177 misses
(exactly `NormalizeCommandCount`) and 11 hits — over 183 scripts, into a table
that is reset per script and capped at 128. Real cost **13 tk/frame**, ~302 at
the most concentrated distribution the data allows, against the inherited
**665**. The c123 PC range `0x02065cb6`–`0x02065cc2` at 161,203 iterations is the
**`FindNormalized`** scan, which `PlanStream` also inlines once per command over
a 1,177-entry ledger. **New instance of a known trap: two inlined copies of one
loop can be attributed to two different functions.** Index built, measured,
reverted; refutation is in the source so it is not re-proposed.

**The FGM slot cache is a ONE-slot cache per size class, and fixing that is not
worth 16,000.** The hit/miss pair `…_c-band-io/BAND_IO_OWNER.md` §5 asked for:
**188 plays, 38 hits, 150 misses (79.8%), 143 of those misses evicting still-
resident data, and at most 5 of 8 slots ever pinned** (so pinning is *not* the
ceiling the inherited analysis assumed). Cause: the victim rule's
`slot->capacity < best->capacity` is strict, and the partition is
`53,248 / 3 × 28,672 / 4 × 16,384`, so `best` can never move between equal
capacities. Working set decoded from request masks: **59 cues, 575,760 B against
204,800** — residency impossible, measured rather than inferred.

A recency tie-break took hits 38 → **47**, misses 150 → **141**, evictions
143 → 133, `NoSlotCount` 0. At BAND_IO_OWNER's −12,736 for eliminating all 150,
that is **−764 P95-equivalent**. Three arms:

```text
A  control    build-c147-ctl     924,864 /  1,210,944
B  candidate  build-c149-fgmlru  927,424 /  1,206,656
A2 falsifier  build-c150-nolru   926,144 /  1,194,368   <- fastest of the three
B - A  = -5,056 P95   B - A2 = +11,776 P95   A2 - A = -16,832 P95
```

**REVERT.** The candidate loses to its own layout; every gameplay invariant was
byte-identical on all three arms; A2 reproduced the control's cache counters
exactly (38/150/143), so the falsifier reverts what it claims to.

**Two consequences for every future row.**
1. **The placement floor on this ROM is ~17,000 `WORK-H` P95, not ±5,376.** A2
   carries the candidate's bss and 16 *fewer* bytes of text and beat the control
   by 16,832; the diagnostic arm beat it by 17,920. Below ~17,000 a two-build
   comparison measures the linker. Only a flag falsifier means anything.
2. **The in-match FGM I/O lane is CLOSED by arithmetic.** Depth 1 → 38 hits,
   depth 4 → 47, so returns are already flat; an implausible depth-16
   repartition extrapolates to ~−2,300, and any repartition of a fixed 204,800 B
   budget buys slots for one size class by taking eligibility from another —
   with 8 handles able to pin, that is a path to a silently dropped sound. The
   only thing that could reopen it is the **unit price**: the 447-step FAT walk
   per seek tracks the ROM image, so a much larger ROM re-prices every miss.
   `gNdsAudioFgmCacheNoSlotCount` went out with the revert — a repartition cycle
   must re-add it first, because today a dropped cue is invisible inside
   `gNdsAudioFgmReadFailCount`.

**Slice 52 re-briefed — three corrections, no build spent** (STACK.md §5):
- **`gmCollisionTestSphere` returns an ANGLE, not a boolean.** Its swept branch
  writes `*p_angle` (`syVectorAngleDiff3D`) and `argA` (`syVectorNormCross3D`)
  for `sphit_kind` 0 and 1 — the shield knockback angle and its normal, both
  continuous gameplay values. A flip budget cannot express a continuous
  perturbation, so a full fixed-point sphere kernel is **unbounded** under the
  standing invariant law. It is a seam *dependency*, not a saving (it is absent
  from slice 52's own sizing table): convert its **transform only** — the proven
  `ndsR2CfxWorldToLocal`, then back to f32 — and leave the angle math alone.
  That also deletes what slice 52 called "the only genuinely new arithmetic
  left".
- **The `ftGetStruct` stub hazard is narrower than recorded.**
  `ndsR2CfxTestRectangle` already takes `inv_scale` as a **separate argument**
  and the decomp caller passes `&parts->vec_scale`, which the stub sets to 1.0 —
  not the inverse matrix. A ring that takes `inv_scale` from `vec_scale`, where
  the float path takes it from, reproduces the stub exactly. **No identity fill
  and no reachability proof needed.**
- **Do NOT fuse `func_ovl2_800EDE5C` into the frame prepare.** `0x6` and `0x7`
  are independent latches and `EDE5C` reads `mtx_translate`, which stays float,
  so a prepare gated on `0x7` would skip the `vec_scale` write whenever a joint
  arrives with `0x7` already set. `EDE5C` needs no frame at all — its 6,498 tk/fr
  is **separable from the seam** and can be taken without the ring.
- Slot-reader premise **re-verified on `build-c147-ctl`**, not inherited:
  `sNdsFighterPartsPool` / `…SyncDObj` / `…SetIdentity` absent, the
  diagnostic-recorder wrapper `ndsGMCollisionTestRectangle` absent,
  `battleship_gmcollision.c:258` inside the L7 oracle block. **And a search
  trap**: `grep -rn --no-ignore 'unk_dobjtrans_0x9C' . --include=*.c` returns
  nothing in this tree while the same pattern against explicit paths returns
  twenty hits including a writer in `reloc_backend_fighter_model.c`. Name the
  paths.

### Slice 52 (collision in fixed point) — SEAM CORRECTED, NOT WIRED. Re-brief before spending a build

Evidence: `artifacts/performance/2026-08-13_c-collision-seam/SEAM_CORRECTION.md`
and `elf-referrers.txt` beside it. **No build was spent; both root ROMs
untouched.** This row corrects `…/2026-08-13_c-collision-fixed/DESIGN.md` §6,
which is otherwise still the design.

**Three of the six float bodies cannot leave the map.** The linked battle ELF
(`objdump -d`, every relocated reference attributed to its containing function)
says `gmCollisionGetWorldPosition` has **nine referrers, and seven survive the
change** — `ndsBaseFTComputerSetFighterDamageDetectSize`,
`ndsBaseFTCommonCapturePulledRotateScale`, `func_ovl2_800EEEAC`,
`gmCollisionGetFighterPartsWorldPosition` and all three attack-position helpers;
`gmCollisionTransformMatrixAll` is held by `ftParamSetAnimLocks`; and
`func_ovl2_800ED490`'s one referrer is `func_ovl2_800EDBA4`, which is shared
infrastructure with **fifteen** (the renderer adapter,
`battleship_ftMainProcParams`, `func_ovl0_800C9A38`). DESIGN checklist item 4 is
**retracted as written**. Deletable: `gmCollisionSetInvertMatrix` (9 referrers,
all in the ring), `func_ovl2_800EDE5C` (8), `gmCollisionTestRectangle` (3),
`gmCollisionTestSphere` (5).

**So the forward chain is unreachable and two proven kernels go with it.**
Keeping `mtx_translate` float keeps `func_ovl2_800EDBA4`, which keeps its two
callees. `ndsR2CollisionFixedBuildLocal` (1,180 B) and `…Compose` (344 B) have
**no call site at this seam** — 1,524 of the 4,448 proven bytes — and the
`lbCommonSin`/`Cos` 6,406 tk/fr they would have removed stay too.

**Corrected sizing: 22,324 tk/fr certain, ≤31,278 with all of
`gmCollisionGetWorldPosition`, against DESIGN §9's 60,494 — 37–52%.** Through
§9's own arithmetic that is **0.96x–1.34x the 47,424 bar**, not 2.6x, and it is
work *replaced* rather than removed. **Brief cycle C as a measurement, not a
landing.**

**Two facts that make the wiring mechanical, also from the ELF.**
`func_ovl2_800EDE00` is **inlined away** — which is why the invert has exactly
nine referrers and confirms L7's ring (the eight `gmCollisionCheck*` plus
`func_ovl2_800EE018`). And **no in-TU caller of the ring survives**: all nine are
called only from `battleship_ftMainSearchHit{Fighter,Weapon,Item}`,
`battleship_ftMainSearchFighterCatch` and `ndsBaseFTCommonAttackS4ProcUpdate`, so
`#define`-renaming the nine decomp definitions before the `#include` leaves the
originals unreferenced and `--gc-sections` takes them and the four bodies with
them. `func_ovl2_800EE24C`/`EE2C0` and the three item-damage checks are already
absent from the ELF.

**Checklist item 6 is CLOSED on the live gate arm.**
`scripts/probe-collision-fixed-domain.ps1` (globals and pointer derefs only, no
register read, no guest call) plus `scripts/grade-r2-collision-live-domain.c`,
which compiles the shipping header, graded **152 live joint matrices** off the
gate arm over frames 592–1625. **Admitted 152 of 152; every one of the five
guards declined ZERO.** Max fixed-vs-float **0.0003662** against the **0.0200**
bound (55x inside), mean 0.0000710, 5,472 comparisons at 1/4/16/64-unit probes;
`vec_scale` delta 0.0001220. So the decline path is a **recorded fail-closed
path**, not a retained float body — a non-zero counter later is a stop, not
noise. Fixed and float are **equal** against the exact reference here (0.0003176
vs 0.0003170); cycle A's "more accurate than float" holds only at reach ±4,096.

**THE INHERITED LIVE SCALE DOMAIN IS WRONG.** `nds_r2_collision_mtx.h:51` and
DESIGN §2 record **1.1138–1.1199** from L7's 2026-07-31 oracle; this capture
measures **0.9937–2.0479**, and the 2.0479 is a real joint (player 0, joint 15,
frame 592) with the source's own scale latch live on all three axes. That is
**1.83x the inherited maximum and outside the top of the widest sweep cycle A
even reported** (0.25–2.00). Nothing breaks — `NDS_R2_CFX_S2_MAX` is 16 and that
matrix is inside the 0.0003662 — but the falsifier's GATED rows are centred on a
domain the game leaves. **Widen the gated sweep to at least 0.95–2.10 before
quoting cycle A's bounded numbers.**

**`sNdsFighterPartsPool`, `ndsFighterPartsSyncDObj` and
`ndsFighterPartsSetIdentity` are absent from BOTH the shipped battle ELF and the
tick-HUD gate ELF**, so `unk_dobjtrans_0x9C` has one writer and the slot is
reinterpretable. The probe asserts this rather than inheriting it. §3.12 is
structural: `ndsFTParamsInvalidateFighterParts`
(`reloc_backend_compat_shims.c:1638`) zeroes `unk_dobjtrans_word` for every joint
every frame, so there is no lifetime to get wrong and nothing to clear at scene
entry.

**Two hazards DESIGN §6 did not enumerate.** `ftGetStruct`'s stub
(`reloc_backend_compat_shims.c:13852`) hands back a `bzero`'d `FTParts` with the
latches set: reinterpreted, `inv_scale` is zero, so the fixed `center` is `size`
where the float one is `size + radius`. Fill it or prove it cannot reach the
ring. `gmCollisionSetMatrixNcs` stays and drags nothing back — verified.

**RE-PRICED 2026-08-15 ON THE POPULATION THE GATE IS SCORED ON — the sizing
above is right and the VERDICT it supports is wrong** (`…/2026-08-15_k1-owner-
pricing/OWNER_PRICING.md`; `build-c172-profile-shipcand`, v3 stall capture at
HEAD `48741fc`, mask `total_cycles−halt_wait ≥ 1,177,548` = the 80 frames that
SET P95, basis cycles/(2×80)). "22,324 certain / ≤31,278 … 0.96x–1.34x the
47,424 bar" compares a WHOLE-MATCH lane against a WHOLE-MATCH bar. **This lane
is 5.2–11.7x more present on the frames that decide the percentile**:
`gmCollisionTestRectangle` **11.70x**, `gmCollisionSetInvertMatrix` **11.54x**,
`gmCollisionGetWorldPosition` **11.46x**, `func_ovl2_800EDE5C` **11.27x**,
`gmCollisionCheckFighterAttackDamageCollide` **10.81x**, `func_ovl2_800ED490`
**7.68x**, `func_ovl2_800EDBA4` **6.90x**, `gmCollisionTransformMatrixAll`
**5.19x** — against `battleship_ftMainProcUpdateInterrupt` **1.06x** and
`ndsBaseGcRunAll` **1.03x**, which are flat. The bar is not 47,424 either: it is
**+32,593 net at the 80th-largest frame**.

| body | self | +soft float | +`sqrtf` | **P95-set tk/fr** | calls/fr | tk/call |
|---|---:|---:|---:|---:|---:|---:|
| `func_ovl2_800ED490` | 4,055 | 11,808 | — | **15,863** | 12.3 | **1,290** |
| `gmCollisionSetInvertMatrix` | 2,524 | 6,120 | — | **8,644** | 6.7 | **1,297** |
| `gmCollisionTransformMatrixAll` | 3,514 | 4,318 | — | **7,832** | 15.3 | 512 |
| `gmCollisionGetWorldPosition` | 1,711 | 5,310 | — | **7,021** | 18.9 | 371 |
| `gmCollisionTestRectangle` | 2,102 | 4,466 | — | **6,568** | 11.3 | 581 |
| `func_ovl2_800EDE5C` | 681 | 1,505 | 2,931 | **5,117** | 11.3 | 453 |
| `gmCollisionGetFighterPartsWorldPosition` | 1,803 | 2,790 | — | **4,593** | 2.3 | 2,014 |
| `func_ovl2_800EDBA4` | 1,737 | — | — | **1,737** | 7.0 | 248 |
| **cluster** | **18,127** | **36,317** | **2,931** | **57,375** | | |

plus `lbCommonSin`+`Cos` **4,975**, six per local build, which
`ndsR2CfxBuildLocal` replaces with its own table.

**One 4x4 compose costs 1,290 ticks and 960 of those are 63 soft-float library
calls** — the profile counts **63.0** float calls per `func_ovl2_800ED490`
invocation and `gmcollision.c:208-225` contains **36 multiplies + 27 adds = 63**.
The rows are 96% `issue` with no cache component, so this is arithmetic, and
`PROJECT_GOAL.md` sanctions fixed-point replacement outright.

**Three consequences for this row.**
1. **The deletable ring alone still does NOT clear the gate, and now that is
   measured rather than suspected**: `SetInvertMatrix` + `TestRectangle` +
   `EDE5C` = **20,329 tk/fr (0.62x)**, **27,350 (0.84x)** with
   `gmCollisionGetWorldPosition`. `gmCollisionTestSphere` prices **0** — it is
   in the ELF (`0x0207fcd0`, 0x4ac bytes) and simply never executes in this
   matchup, so converting it buys ticks nowhere and text everywhere. (`nm`, not
   the census: a zero census row is "did not run", not "not linked", and the
   first draft of this note read it as the second.)
2. **The one extension that closes it is `func_ovl2_800EDBA4`, and the objection
   to it dissolves if `parts->mtx_translate` stays f32.** Give EDBA4 a
   fixed-point INTERIOR with an f32 boundary: build each missing local with
   `ndsR2CfxBuildLocal`, compose with `ndsR2CfxCompose`, convert to f32 once per
   joint when writing `mtx_translate`. All fifteen referrers are untouched
   because the type and value they read do not change. Those are exactly the
   **1,524 of 4,448 proven bytes** this row wrote off as "no call site at this
   seam", and they carry the 6,406 tk/fr of `lbCommonSin`/`Cos` with them.
   Predicted (PREDICTION, not a measurement) **+30,000…+38,000 at rank-80**.
3. **Law-1 counter LANDED and FIRING** (`src/import/battleship_gmcollision.c`,
   `#if NDS_TICK_HUD`, `__attribute__((used))`):
   `gNdsCfxFighterDamagePhaseCalls` **1,938** / `…Hits` **20** over frames
   439–2038 on `build-c173-cfxcount-bp1`, i.e. 1.21 calls/frame whole-match
   against 13.1 on the P95 frames — **the counter reproduces the profile's
   10.81x**. Site picked from the LINKED ELF: the two ring entries have **zero
   in-TU callers**, which is what makes the `#define`-before-`#include` rename
   capture every call; `gmCollisionTestRectangle` and `gmCollisionSetInvertMatrix`
   do **not** have that property and a rename there reads zero forever.
   **`gNdsCfxFighterShieldPhaseCalls` is 0 in this match — UNPROVEN, not proven
   inert.** `Hits` is the A/B's equivalence guard: a fixed kernel that stops
   returning `TRUE` reads as a large clean win.

**Instrument for the landing**: A/B/A with a flag falsifier
(`NDS_R2_COLLISION_FIXED` 0/1/0), because the cross-build floor here is ~17,000.
Both arms must report equal `…PhaseCalls`, equal `…PhaseHits` and an equal
end-of-match invariant pair before any tick delta is read. Re-grade the host
falsifier at the **widened live scale domain 0.9937–2.0479** this row already
recorded, not the inherited 1.1138–1.1199.

### Slice 51 KEPT — the shield attach path was paying for a SEARCH. Gate 1,210,944

Evidence: `artifacts/performance/2026-08-13_c-ledger-index/LEDGER_INDEX.md` and
the three arm JSON/rows beside it, plus the five-minute oracle run.

The 2026-08-13 anim-joint fix (`607d3697455`) cost **+49,216 P95**. Phase 0
priced it with **no build**, off artifacts already on disk: summing `WORK-H` over
the one-variable five-minute pair (`…_c-animjoint-fix/{ctl5-c134,cand5-c135}`)
gives **+46,897,344 ticks over 9,154 attaches = 5,123 tk/attach**, and the c123
per-PC profile shows **95.0% of `ndsAObjEvent32NormalizeScript` is two inlined
pointer scans** at **16.09 cyc = 8.05 tk per iteration**. 5,123 / 8.05 ≈ **636
scan iterations per attach** — the ledger reaches 1,177 entries in a minute and
is scanned from index 0 every time. Predicted `−0.85 × 49,216 = −41,800`;
measured **−39,424**.

`ndsAObjEvent32FindNormalized` is now an open-addressed probe of a 4,096-slot
**index over the same ledger** — not a second cache. No new key (the ledger is
already keyed on the command pointer), no new lifetime, cleared in the same
breath as the count it indexes, so **§3.12 is satisfied by construction rather
than by a guard**. 8,192 B bss; headroom 167,936 → **159,488**.

| arm | `WORK-H` P50 | P95 |
|---|---:|---:|
| A control (`build-c144-ctl`, HEAD) | 925,184 | 1,250,368 |
| **B candidate (`build-c144-ledgeridx`)** | **924,864** | **1,210,944** |
| A2 falsifier (`build-c145-noidx`) | 921,728 | 1,253,120 |

**The A2 arm had to be a flag.** `build-c145-ctl2` is HEAD rebuilt into a fresh
directory and its ROM is **byte-identical** to `build-c144-ctl`; with a
bit-deterministic sampler a repeated control brackets nothing.
`NDS_AOBJ_EVENT32_LEDGER_INDEX=0` builds the index and its bss and reverts only
the lookup — the candidate's placement with the control's behaviour. It brackets
the control to **2,752**, so the win is **14x the measured placement floor**.
`SINT` −23,936 / `SRC` −19,648 / `GCRA` −19,584 with `FTR` −384 and `STG` +2,816:
the saving lands in the lanes the fix's cost landed in. **40 frames move 3→2
VBlanks.** Every gameplay and ledger counter is byte-identical on all three arms.

Qualification (`build-c146-oracle5`, five-minute both-CPU, oracle on, 8,448
samples): **12,667 paired lookups, `gNdsAObjEvent32HashOracleMismatch` 0**,
overflow 0, runaway 0, `NormalizeFailCount` 0, heap free-min 70,000. That arm's
P95 reads 1,246,336 — the oracle restores the whole scan, which is a third
confirmation that the scan was the cost, and is **never a gate figure**.

**Ledger margin re-read and UNCHANGED: 1,598 of 2,048, 1.28x.** The index cannot
reduce fresh normalizes and the reason matters — the ledger already deduplicated
them (`ReuseCount` was 1,574/minute *before* this change). The scripts were never
being re-normalized, they were being re-**found**. Capacity is still cycle 13's
open question, at exactly the number it left it.

**Named, sized, not taken:** `ndsRelocResolvePointerFromFileBase` is 3.9% of the
attach path (≈ −1,900 P95) and would need a genuinely new cache;
`ndsAObjEvent32FindPlanned` is the other O(n²) scan (~665 tk/frame, flat P50) and
the shield path never reaches it — every attach returns at the ledger hit before
`PlanStream` runs.

### Slice 50 KEPT — the stage stops re-proving its texture bindings. Gate 1,210,880

Evidence: `artifacts/performance/2026-08-13_c-threeleg/SLICE50.md`, the three arm
JSONs/rows beside it; captures in
`artifacts/visibility/2026-08-13_c131-texproof/`.

`ndsRendererNativeStagePreparedTextureValid` re-proved the prepared stage run
table against the texture cache **194.88 times a frame** (entry-PC counts:
Commit's 54-run sweep + the reuse key's 53.97 + the replay sweep + BeginRun's
~33). Per-line attribution priced it at **9,369 tk/fr at 5.34 cycles per
instruction** — not compares, but `runs[]` and then the cache entries it names
dragged through a 4 KB D-cache every frame. **Slice 44's shape, and slice 30's
fix**: the same key-generation fence is now taken **once per texture epoch**,
with the epoch advanced at every writer that can break it.

| bucket | control A | **A2, separate link** | candidate B | Δ |
|---|---:|---:|---:|---:|
| `WORK-H` P50 | 939,392 | **939,392** | 923,392 | **−16,000** |
| `WORK-H` P95 | 1,219,520 | **1,219,520** | 1,210,880 | **−8,640** |
| `STG` P50 / P95 / mean | 172,928 / 177,216 / 173,313 | same | 161,600 / 165,888 / 161,991 | **−11,328 / −11,328 / −11,322** |
| `FTR` P50 / P95 | 303,232 / 330,048 | same | 298,048 / 324,032 | −5,184 / −6,016 |
| `WAIT` P50 | 197,824 | 197,824 | 211,136 | **+13,312** |
| VBI 2/3/4/5+ (max) | 1705/304/17/12 (26) | same | **1743/267/15/13 (26)** | 37 frames 3→2 |

**A2 is the third arm and it returned to A EXACTLY** — every bucket, both
percentiles, the whole histogram, delta 0 — on a separately-linked binary (the
ELF hashes differ). So the instrument reproduces to **0** for unchanged code and
the entire B delta is the change's. `compare-tick-hud-arms.py` warns of a
~14,080 placement term for separately-linked arms; between A and A2 it measured
**0**, so that term belongs to *changed code*, not to linking.

**Judge this at `STG` and P50.** `STG` moved −11,328 at P50, P95 and mean
simultaneously — a flat uniform shift of exactly the lane the work lived in,
which placement noise cannot imitate. P95 is the noisy statistic here: two
candidate binaries differing by **one line** measured P95 1,186,816 vs 1,210,880
(**24,064**) against 3,648 at P50 and **512** in `STG`. The banked P95 is the
conservative of the two. `WAIT` rising +13,312 as `WORK-H` falls 16,000 is the
corroboration — on a VBlank-paced ROM the removed work reappears as idle time.

**Engagement, both sides.** Control side is the c123 profile's 194.88 calls/frame.
Candidate side, from the same run that produced the buckets:
`gNdsR2TexProofFastCount` **20,370** (9.995/frame), `gNdsR2TexProofSweepCount`
**8 for the whole match**, `SweepFailCount` **0**, `gNdsR2TextureEpochBumpCount`
**0**. A per-frame sweep count would have meant the epoch was moving and the
lever had regressed to the old shape. `EpochBump 0` confirms the design premise
measured off the profile: the cache is static mid-match
(`ndsRendererHardwareAllocTexture` runs **3 times** in the run, Release/Evict/
Discard never appear). Negative control: `gNdsR2StagePrepareReuseCount` **2,037**
/ `BuildCount` **2** identical on all three arms, so the win is not bought by
reusing something the control rebuilt. End-of-match damage **0/58**, stock
**1/1** on all three arms.

**Fidelity.** Frame-locked on the simulation clock —
`EXACT_LOCK=gSCManagerBattleState->time_remain,40,38` on both arms, which is the
only valid cross-build lock (the presented-frame counter drifts with the speed
change). Game viewport 400x296 = **118,400 px PIXEL-IDENTICAL** on both tics,
max channel delta 0. The only top-screen drift is **12 px on scanlines 298-299**
— the tick-HUD's own digit row, which must differ between arms.

**Why the fence is sound now.** The proof reads four things and every writer of
them signals: the three re-key sites already bumped
`sNdsRendererHardwareTextureKeyGeneration`; `ndsRendererHardwareReleaseTexture`
now bumps it too and is the **sole** destructive seam (Evict, Alloc's recycle,
Discard and the static teardown all route through it, and its `memset` is what
clears `ready` and `name`); every `ready = TRUE` is guarded by `name == 0` +
`glGenTextures`, i.e. a slot no live certificate can reference; and the prepared
table's certificate is dropped at its **write seam**,
`ndsRendererNativeStagePrepareRun`, plus the instance beside
`gNdsR2StagePrepareBuildCount`. Nothing keys on a pointer, so §3.12's arena
rewind cannot make a stale certificate read current.

**Boundary caught the checker debt, as designed** — twice, and both were shape
pins rather than defects. `check-gbi-decode-fixtures.ps1` pinned the reuse key's
old helper name and the Commit gate's per-segment loop. Both re-pinned in their
new form and **strengthened**: the epoch bump in `ndsRendererHardwareReleaseTexture`
and the drop at the write seam are now their own assertions, because either
failing would be silent — stale native geometry drawn from a recycled slot.

**Legs B and C of `RESIDUE.md` §4 row 0 are NOT deferred, they are refuted.**
B needs 828 B against the texture-cache `_Static_assert`'s **72 B** of slack (the
guard installed after +14 KB of bss stopped the ROM booting). C's "8 unconverted
GX sites" do not exist: slice 1 already converted every per-corner fighter
writer, `WriteNormalWord` carries no capture test, and the residual charge is the
shared `gl*` → `ndsRendererTask29Gl*` macro block (`nds_renderer.c:1501-1523`),
which has stage callers — a per-caller split at 1.85 cyc/byte, not an `#if`.

**Residual for the stress/soak cycle: ANSWERED 2026-08-13, the certificate holds
across re-entry** (`artifacts/performance/2026-08-13_c-stress/STRESS_GATE.md`).
A both-CPU chain with **three battle-scene entries** — match, rematch, and a
natural **Sudden Death** — read `gNdsR2TexProofSweepFailCount` **0** with
`gNdsR2TextureEpochBumpCount` **168** and `TexProofFastCount` 132,566 against
`SweepCount` 10,008, i.e. the enumeration is complete over every path a restart
and an SD entry take, and the win is still being taken. Do not re-open this
without a new invalidation source.

### Slice 44 KEPT — the stage stops re-proving itself constant. Gate 1,210,560

Evidence: `artifacts/performance/2026-08-11_c121-slice44/SLICE44_GATE.md`,
`REATTRIBUTION.md`, `pcdetail-dobjxobj.txt`; the per-PC probe that started it is
`artifacts/performance/2026-08-11_c120-lane/pcdetail-sourcekeymatches.txt`;
captures in `artifacts/visibility/2026-08-11_slice44-stride/`.

`NDS_R2_STAGE_VALIDATE_STRIDE = 8`, graduated onto
`smash64ds-battle-playable-hwtri` and the tickhud/proof/results-lab block.
**Not** the BUGS.md #9 floor arms — those compare the rigid path against the CPU
path and a stride would confound them.

**The measurement that chose the lever, and it cost no build.** The stage ran
`StageWorldSourceKeyMatches` **54.0×/frame for 7,719 tk**, plus 9,017 in
`BuildPersistentStageWorldMatrix`, and neither builds anything: the 26 rigid
bindings are re-proved constant every frame and the 16 dynamic ones re-walk a
parent chain whose cached answer they then reuse. `--pc-detail` says **9.7M of
the 24.7M cycles are four cold loads** — `ldrb r1,[r1,#4]` (`xobj->kind`) at
**27.55 cyc/insn**, `ldrb r3,[r1,r3]` 22.72, `ldrb r2,[r0,r3]` 22.34,
`ldr r3,[r0,#76]` (`dobj->vec`) 17.27 — while the inlined byte-compares of
translate/rotate/scale sit at 1.0–7.6. **The compare was never the cost, so a
cheaper compare was never the lever.** It is 42 DObjs and their XObjs pulled
into a 4 KB D-cache every frame; the only lever is not touching them.

**Round-robin, not "full sweep every 8th frame".** The second shape makes 12.5%
of frames expensive and P95 lands on one of them and reads flat. Spreading 42/8
checks across every frame is what moves P50 and P95 together.

| bucket | control (STRIDE=0) | stride | Δ |
|---|---:|---:|---:|
| **WORK-H P50** | 948,736 | **931,648** | **−17,088** |
| **WORK-H P95** | 1,246,464 | **1,210,560** | **−35,904** |
| STG P50 / P95 | 188,736 / 196,864 | 168,832 / 172,672 | −19,904 / −24,192 |
| FTR P50 | 294,592 | 296,704 | +2,112 |

Both arms `slips=0`, DLDI ON, one tree, 1600 samples from 438. The control
reproduces the banked gate to within 1,984. **The win is in the bucket it was
aimed at.**

**Engagement, both sides** — `RigidChecks` 6,627 / (6,627 + 46,387) = exactly
**0.12500**, and 53,014 = **26 × 2,039** means the sweep ran on every frame,
which is also the proof the rigid mask never demoted (a demotion early-returns
and freezes both counters). `StaleReuse` 28,518 against 28,546 predicted.
`gNdsRendererTask36RigidConstancyMismatchCount` is `PROFILE_LEVEL == 1` only, so
it does not exist in a tick-HUD ROM — the ratio is the available proof and a
stronger one.

**Fidelity.** Game viewport (8,55)–(408,350) is **pixel-identical on both
frame-locked tics** (118,000 px, max channel delta 0); only the tick-HUD text
differs. Both arms read damage 130/51 stock ×1 at the lock. Boundary green.

**Two seams closed with it.** Demotion is now **one-way within a topology** — the
old code re-armed the mask from the constant every frame and let the sweep clear
it again, equivalent only because the sweep was complete; a partial sweep must
not re-arm a binding it did not look at. And the once-per-scene topology rebuild
now drops the stage world cache, because entries are keyed on DObj address and a
recycled heap address would make a stale entry **match** — harmless while
`validated_frame == frame` forced a rebuild, a previous topology's matrix under
the stride.

**Boundary caught the checker debt, as designed.** `M3_STAGE_FALSIFIER`
enumerates every workspace field each stage closure may read and failed on
`workspace.slice44_validate_cursor` being unclassified; it is now
`FIELD_CLASS_LIVE` in both affected closures and the consumed-field manifest is
regenerated. Do not add a workspace field without it.

### Slice 43 KEPT — the geometry engine composes the fighter joints

Evidence: `artifacts/performance/2026-08-11_c119-lane/SLICE43_GATE.md` and
`SLICE43_ENGAGEMENT.md`; captures in `artifacts/visibility/2026-08-11_gx-compose`.
Two ROMs from one tree, 1,600 frames both-CPU from 438, stride 96, DLDI ON.
**The matched control reproduced the banked gate TO THE TICK** (958,592 /
1,258,112), so the delta belongs to the flag and not to placement.

| bucket | control | slice 43 | + projection elide | Δ total |
|---|---:|---:|---:|---:|
| `FTR` P50 | 302,848 | 296,832 | **294,592** | **−8,256** |
| `FTR` P95 | 306,368 | 300,288 | **297,856** | **−8,512** |
| `WORK-H` P50 | 958,592 | 953,856 | **947,968** | **−10,624** |
| `WORK-H` P95 | **1,258,112** | 1,249,088 | **1,244,480** | **−13,632** |

Both `WORK-H` arms clear the ±8,544 floor. `NDS_R2_FIGHTER_GX_COMPOSE` is
graduated at all three published override sites.

**What it does.** `ndsRendererAdapterComposeOwnerWorldsFlat` stops multiplying:
the capture copies each binding's chain out in the order that pass would have
multiplied it, and the production root loop issues
`RESTORE(parent)` / `MTX_MULT_4x3(chain)` / `STORE` / scale off the GX matrix
palette. DS `MTX_MULT` is `current = M × current`, the same convention as
`MtxMulAffine20p12(&local, out, out)`, so nothing is reassociated. Stacked on it:
31 of 32 per-root projection pushes re-load an identical matrix, elided behind
pointer equality reset per execute — R2-03 E23's own reverted cut, free here.

**Controls.** `Captures/Roots` **63,218/63,218 = 1.000**, `Declines` 0 over 2,038
frames, `ProjectionSkips` 59,285 = **93.79%** (E23 independently measured 93.8%).
Both arms end at damage **130/51**, so this is a cost delta, not two matches
([[route-ab-cannot-price-gameplay-change]]). Frame-locked captures at match tic
3000 are pixel-identical, so no fidelity budget is spent.

**Two defects the counters caught, and one counter would not have.** `Roots` read
32.06/frame — full engagement by itself — while `Captures` was exactly 18 × 535:
Mario declined every owner (the chain store was sized at the joint count, but
Mario has THREE bindings with no bound ancestor and each walks to the DObj root,
so their shared ancestors are captured once per root chain) and its 14 roots were
emitting **Fox's** joint chains. **Instrument the producer and the consumer and
check their ratio** ([[count-both-sides-of-an-engagement]]).

**The prediction was 3.4× too big, and the reason is reusable.** −20,700 was
computed at E23's 12.2 cycles per FIFO word. The chain commands measured **~30**;
the projection elide in the same function measured **~8.5**. A `LOAD4x4` at the
top of a root sits just after `EndBatch` with the queue drained; the chain's
`MULT`/`RESTORE`/`STORE` interleave with vertex submission. **Never carry a
cycles-per-word constant between command sites** ([[fifo-word-price-depends-on-queue]]).

**The 17-word `diag(1,1,1,s)` scale cannot be made cheaper** — 544 words/frame
and the largest new cost. `MTX_SCALE` is 4 words but scales rows 0..2, and
`GL_MODELVIEW` is DS matrix mode 2, so it would scale the vector matrix and break
`NDS_R2_FIGHTER_HW_LIGHT`; the uniform-scale-cancels-in-the-divide form has the
same problem. Row-vector algebra blocks folding it into the factors (`L·V` scales
column 3, `V·L` scales row 3) or into the seed (rightmost factor, so right
multiplication scales a column).

### Cycle 119 re-attribution: the 20.12 matrix lane is the FIGHTERS, not the stage

Full evidence: `artifacts/performance/2026-08-11_c119-lane/FIGHTER_MATRIX_LANE.md`.
Same CSV, no extra run, the builders themselves as leaves, 100.0% reconciliation
on every one. **35,752 tk/frame of the 62,891 "20.12 kernels" group has a single
caller, `ndsFighterMarioFoxDLAllDrawForSlot`** — 52.5 `MtxMulAffine20p12` (18,560),
57.5 `BuildDObjXObjMatrix` (12,233), 55.8 `BuildDObjLocalMatrix` (4,959), all
80/80. Mario 25 joints + Fox 27 = the whole of it. Only **1.5** of 54.2 affine
multiplies are the stage's. `tk prem` ≈ 1,900, so this is FLAT work — a cut moves
P50 and P95 one for one.

**Everything but hardware is closed on this lane, and one closure is new.** The
arithmetic died in slice 42. The instructions died in `--pc-detail`: 324 PCs, top
PC 6.8%, and the four costliest rows are loads at 28.7/29.2/16.8/13.3 cyc/insn —
21% of the function is D-cache misses on `DObj`/`FTParts` fields
([[flat-function-only-lever-is-not-entering-it]]). And **the local-matrix memo is
now dead twice**: R2-03 E8 refuted it at +16,301 on key cost, and its payload has
since shrunk from E6's 1,061 tk/build to **302 tk/call** because
`NDS_R2_FIGHTER_MTX_DIRECT` graduated — at E8's 13.6 hits/frame the gross saving
is ~4,100 tk, under the floor before the key costs anything. Do not revive it,
cheap write-site generation key or not.

**Open — slice 43: compose the fighter joint worlds on the GX matrix palette.**
The 52 composes and 32 `LoadHardwareSplitMatrices` exist only to hand a matrix to
GX — the loader is `SetMatrixMode(GL_MODELVIEW); LoadMatrix4x4()` and nothing
else, and fighter vertices are submitted in MODEL space
(`nds_renderer.c:13593`), so **no CPU consumer reads a composed fighter world.**

The shape that fits the existing loop, and it needs no reordering: the production
root loop **already calls `glStoreMatrix(palette_slot)`** (`nds_renderer.c:32052`)
with a baked per-root slot. `binding_parents[i] < i` is preorder, so per root

> `glRestoreMatrix(palette[binding_parents[i]])` · `MTX_MULT_4x3` × chain ·
> `glStoreMatrix(palette[i])`

reconstructs the tree with the palette as the parent store — no DFS stack, same
iteration order. DS `MTX_MULT` is `current = M × current`, exactly the
`MtxMulAffine20p12(&local, out, out)` convention already in the loop, so nothing
is reassociated.

**Cost model, and it is what decides the slice** — derived from R2-03 E23, which
measured skipping 29 identical 17-word projection pushes/frame at −3,008 tk, i.e.
**~12.2 cycles per FIFO word**:

| per frame | now | palette |
|---|---:|---:|
| CPU affine composes | 52.5 × 708 = 37,170 cyc | 0 |
| FIFO words | 32 roots × 34 = 1,088 | 32 × (1+1) + 52 × 13 = 740 |
| FIFO cycles @12.2 | 13,274 | 9,028 |
| **total** | **50,444 cyc** | **9,028 cyc** |

**≈ −20,700 tk/frame**, flat (80/80, `tk prem` ~1,900), 2.4× the ±8,544 floor.
Sensitive to the word cost: at 12.2 it wins by 20,700, and the variant that
re-multiplies each binding's FULL chain instead of the segment above its
binding-parent (~160 mults) falls to ~−12,000 and **loses**. Use the palette.

Work: matrix prep must hand the root loop each binding's chain of LOCALS instead
of one composed world (`ComposeOwnerWorldsFlat` already builds that `chain[]`),
`NDSRendererNativeFighterRoot::modelview_matrix` becomes a (count, locals) pair,
and `NDS_RENDERER_HW_WORLD_UNIT_SHIFT` moves from the loader onto every local's
translation row — scaling `m[3][*]` of every matrix in a chain scales the composed
translation by the same factor and leaves the linear part alone. Render-side only,
same doctrine `ComposeOwnerWorldsFlat` already relies on.

**Both preconditions checked, both hold — but one of them costs a generator
change.** The palette today is *sparse*: `sNdsNativeMarioCrossPaletteSlots` gives
real slots to 8 of 14 bindings and `sNdsNativeFoxCrossPaletteSlots` to 2 of 18;
the rest carry the `31u` sentinel, above the `NDS_NATIVE_GX_MATRIX_SLOT_MAX = 30`
guard, because it exists for cross-root vertex binding rather than for parent
storage. The slice needs a slot for every binding that is some other binding's
parent. Counted from the baked tables — Mario parents `{0,1,2,5,6,8,9,11,12}`,
Fox `{0,1,2,3,5,7,8,10,11,13,14,16}` — that is **9 and 12**, union with the
existing cross-root owners **9 and 14**, against 31 slots, and the two fighters
never share an execute so the slots are reusable. It fits with room. The
generator must emit a dense per-binding slot table beside the cross-root one.
Second precondition is free: `GL_MODELVIEW` is DS matrix mode 2, so `MTX_MULT`,
`MTX_STORE` and `MTX_RESTORE` all act on the position **and** vector matrices
together, which is what `NDS_R2_FIGHTER_HW_LIGHT` needs.

**This lane is inside the 44.9% that the tail actually is.** The cycle-118 table
above it — asset streaming 29.3–39.7%, "game logic + renderer 11.6%" — is the
`--top 40` reading at 75% coverage that `docs/HANDOFF.md` already marks *do not
re-derive*; at full depth (`--split-top-frames 80`) it is **game+renderer 44.9%,
FAT/ROM reads 9.6%**. Slice 43 is in the right lane.

### Do NOT retry: the flower rigid-mask, and why its own report's prediction is stale

R2-02 E4 §8 predicted de-crossing + widening the rigid mask would "recover the
bulk" of arm C's −51,200. §8a then measured de-crossing alone at −4,224 and
concluded the other ~47,000 "was never work — it was the 15 triangles not being
drawn" — but nobody re-derived §8's prediction against §8a's own number.
Doing that: arm C = (dynamic chain for 10 bindings) + (35 matrix loads) +
(15 triangles undrawn) − (rigid+replay cost). §8a fixes the loads at ~7,400 and
the undrawn geometry at ~47,000, and this cycle's attribution fixes the dynamic
chain at 10 × (584 + 580) ≈ 11,640. That leaves the rigid+replay path costing
about **+3,200 more** than what it replaces — the wrong sign. Consistent with arm
B, where widening the mask doubled `STG`. The generator de-crossing, the checker's
pinned `(32, 34, 45, 47, 49)`, and the Tier-2 differ gating are real work for a
lever that prices out at zero. Leave it.

### Slice 41: 30 Hz poses REJECTED — and a route A/B cannot price a gameplay change

Cycle 119, one binary (`builds/build-c119-pose-route`, `NDS_R2_BOTH_CPU=1`),
1,600 frames from 438. `gNdsR2PoseRateRoute` 0 → 1 skipped pose EVALUATION on
logic update 0 and kept it on update 1, leaving the `length` advance at 60 Hz so
animation timing stayed bit-identical. Full evidence:
`artifacts/performance/2026-08-11_pose-rate/FINDING.md`.

**Engagement was total and the gate did not move.** 149,323 DObj passes and
586,162 nodes skipped — exactly half, as E61's "`GOBJ_FLAG_NOANIM` skips are 0"
predicts. `WORK-H` P50 959,360 → 966,144, P95 1,255,104 → **1,262,144**. Arm A
reproduces the banked gate to within 3,008.

**The delta is not a cost, because the arms stopped playing the same match.**
End-of-match damage 130/51 versus **33/65** on one poked bit. `SCPU` — the CPU
decision proc, which evaluates no poses — moved −8,256 (−20%). Per-frame
correlation +0.062, windowed delta swinging −24,896 to **+95,455**.

**Standing rule: a one-binary route A/B is valid only for a change that cannot
alter gameplay state.** The route form exists to delete the ±8,544 cross-build
placement floor, and it does — but it assumes both arms walk the same
trajectory. A change collision reads breaks that, and the delta then prices two
different matches. Such a candidate needs a fixed input replay, or must be
priced by its own subsystem cost rather than by frame cost.

Independently disqualifying: `PROJECT_GOAL.md` requires Fox to "use behavior
equivalent to the original Level-3 CPU", and a 20% shift in the CPU proc is a
changed CPU, not a changed picture. The route has been removed from the tree.

**The animation lane is now out of fidelity-free levers.** Requirement 4 already
made the kernel fixed-point; E61 fixed the mix at Cubic 54.8% / Step 43.6% /
Linear 1.7% with zero discarded evaluations, so there is no memoization to take
and no pass to delete. Band A's other 80/80 cluster (soft float, ~34,006 ticks)
is the next candidate.

**Harness defect fixed on the way.** `sample-tick-hud-buckets.ps1` labels ring
rows by counting backward from the presented-frame counter while recording that
ring slots need not equal presented frames; a stop with `skew == -1` therefore
collides its first label with the previous stop's last **by construction**. Five
such stops, five duplicates, exactly at those rows — it cost two whole-match runs
before the pattern (all at multiples of `-RingStopStride`) was seen. The guard
now classifies a skew-explained seam and continues without an override, and
still fails unconditionally on an identical payload or a duplicate away from a
seam — which is what caught the cycle-118 vertex-memo defect.

### Slice 40: the attach path — NOT BUILT. The measurement killed it before the build.

The attach path was declared the next architectural slice and was one step from
being built as a pre-normalized asset bake. **Its frame distribution says it is
not a gate lever**, and the number that says so was already in hand.

`--split-by-symbol ndsAObjEvent32NormalizeScript`, whole match, 1600 frames:

| | frames | cyc/frame |
|---|---:|---:|
| attach-marked | 110 (6.9%) | 2,780,558 |
| control | 1490 | 2,475,315 |
| **premium** | | **305,243** (152,408 tk) |

E11's concentration therefore SURVIVES the whole match, and the frames are
scattered (125, 131, 136, 162 … 1554) rather than a load burst. That was the
question this run was for and it came back the encouraging way. **It does not
matter, because of where those frames sit in the distribution:**

- **P50 is a control frame.** 6.9% of frames attach, so the median frame does no
  attach work at all and deleting it moves P50 by **exactly 0**.
- **P95 is above them.** A typical attach frame is P50 + 152,408 = **~1,113,560**
  `WORK-H`. P95 is **1,294,144**, a further 180,584 up. The 80th-most-expensive
  frame is not a typical attach frame — something costlier owns the tail.
- Its real worth is **mean**: 110/1600 × 152,408 ≈ **10,478 tk/fr**, banked at
  neither percentile this project measures.

This is [[mean-self-time-predicts-p50-not-p95]] running backwards. That memory
says a mean row *understates* a clustered lever. The converse is just as true and
is what nearly cost a build here: **a lever clustering on frames that are not the
tail is worth its mean at P95 and nothing at P50.** Concentration only amplifies
if it concentrates *where the percentile lives*. "Does it cluster?" is the wrong
question. **"Does it cluster where the percentile lives?"** is the question.

Nothing here retires the work — pre-normalizing the AObj16 files is still
byte-count neutral, still costs no RAM, and is still the right shape. It is a
**mean/cadence** change, so it must be proposed and judged as one, and it queues
behind anything that moves the tail.

### The question none of this cycle's measurements answered

**No run has ever partitioned frames by `WORK-H` percentile.** Slice 39 split on
`ALL` > 2 VBlanks — 56% of frames, and quantized on top
([[all-is-a-quantized-gate]]: `ALL` P95 lands on exactly 3 VBlanks, so its
percentiles carry no sub-VBlank information). Slice 40 split on one symbol's
presence. **Who owns the top 80 frames is unmeasured**, and every lever ranked
this cycle — including both of these — was ranked against a proxy for it.

The instrument for it exists and needs no build:
`sample-tick-hud-buckets.ps1 -PerFrameGlobals` returns per-frame `WORK-H`
alongside per-frame counters, which is exactly the joint distribution
(is this symbol's work ON the tail frames?) that both splits above only
approximated. Run that before ranking another candidate.


### ⚠ THE OVER-GATE SPLIT BELOW IS INVALID — its partition was 65% noise

Everything in the following section was computed with `--split-over-gate`'s
threshold set to **exactly 2,240,760 cycles**, the 2-VBlank quantum. The tool's
comment defended that as "a definition rather than a tuned knob" because frames
land on 2 VBlanks or 3 and never between. The clusters are real; **the quantum is
not exact.** On this very profile:

| | |
|---|---:|
| 2-VBlank cluster spans | 2,239,036 .. 2,242,486 |
| spread | 3,450 cycles = **0.154%** of the threshold |
| threshold sat | **inside that span** |
| 2-VBlank frames pushed over by jitter alone | **616 of 1262** |
| frames it marked | 954 of 1600 (60%) |
| frames that genuinely miss cadence (>2.8M) | **338 (21%)** |

So the "over-gate" population was **65% jitter-sorted 2-VBlank frames**, and the
ranking it produced is a ranking of what those frames happen to differ by. Fixed
2026-08-11: `GATE_CYCLES = 2,800,950` (2.5 VBlanks), sitting BETWEEN the clusters
where jitter cannot reach it. Anything from ~2.3M to ~3.3M is equivalent.

**What this retracts.** The section below is why this cycle withdrew the
animation closure and went looking for an attach path — "animation is 14.7% of
the non-idle premium, the largest game-side family, and collision is absent from
the top 40". **None of those three claims is supported any more.** They are not
disproved either; they were computed on a partition that cannot support them, so
they revert to unmeasured. Re-run with the corrected threshold, or better with
`--split-top-frames`, before citing any row of it.

**The general rule, worth more than the fix:** a threshold placed ON a quantized
value bisects that value's cluster and sorts it by measurement jitter. Place a
threshold BETWEEN clusters. "The value is exact so the threshold needs no tuning"
is precisely the reasoning that produces this bug — see
[[a-threshold-on-the-quantum-sorts-noise]].


### The over-gate split — and it REFUTES this cycle's own "animation is closed"

`artifacts/performance/2026-08-11_c118-lane`, whole match on the banked build
(`build-c118-profile-gate`, `NDS_R2_BOTH_CPU=1`, 1600 frames from 438,
`NDS_TASK37_PROFILE_PER_FRAME_REGION=1`), `--split-over-gate`: **895 frames
costing more than two VBlanks against 705 costing two, premium 457,608
cycles/frame.** The fresh window totals 3,994,154,280 cycles against c117's
4,028,886,502 — **−34.7M**, which independently corroborates the banked −10,752.

**CORRECTION, made before this propagated: this is NOT the P95 partition.** An
earlier revision of this entry called it "the table the gate is actually defined
on". It is not. The gate is **P95 of `WORK-H`**; this split is **`ALL` > 2
VBlanks**, and it marks **895 of 1600 frames — 56%**, not the top 5%. So it
describes what separates a 3-VBlank frame from a 2-VBlank one — the *presented
cadence*, which `PROJECT_GOAL.md` cares about directly — and NOT what puts a
frame in the P95 tail. Both matter; they are different questions and the tables
for them are different. Slice 37 still stands: a mean-ranked census answers
neither.

| class | +cyc/frame | share of NON-IDLE premium |
|---|---:|---:|
| `armWaitForIrq` (idle — excluded) | 161,737 | — |
| **NON-IDLE premium** | **295,871** | **100%** |
| instrument + console (`…RenderDebugHud`, `printf`/`scanf`, `consolePrintChar`, locale, mutex) | 95,790 | **32.4%** |
| **animation, total** | **43,428** | **14.7%** |
|   — of which ATTACH | 21,793 | 7.4% |
|   — of which playback | 21,635 | 7.3% |
| file I/O (`get_fat`, `f_read`, `f_lseek`) | 10,677 | 3.6% |
| **map collision** | **0** | **absent from the top 40** |

#### Three findings, in order of how much they change the plan

**1. Collision was never a P95 owner.** It does not appear anywhere in the top 40
of the over-gate premium. It was the largest MEAN owner (16,649 tk/fr of soft
float, slice 34) and slices 35–37 banked −10,752 of real deleted work against it
— but they were aimed by a P50 statistic at a P95 gate. The win is real and kept;
the aim was luck, not method.

**2. Animation is the largest GAME-SIDE contributor to the premium — 14.7%,
21,684 ticks/frame — so "the animation lane is closed" is WITHDRAWN as a
statement about the gate.** What was measured and remains true is narrower: the
*playback* path's remaining soft float is 8,772 tk/fr spread over three symbols,
all under the cross-build floor, and its two largest symbols are already fixed
point at 1.67–1.69 cyc/insn. That is a statement about mean cost. Against the
gate, animation is still the thing to beat.

**3. Half the animation premium is the ATTACH path, which no slice has ever
touched.** Every animation slice in this campaign — Requirement 4, 31, 32, 33 —
optimised playback. The attach path is 21,793 cyc/frame of premium:
`ndsRelocAssetIDForToken` 7,634, `ndsAObjEvent32NormalizeScript` 5,107,
`ndsR2AnimBuildTrackTable` 1,967, `ndsRelocNormalizeFighterAObj16File` 1,929,
`ndsR2AnimTargetValue` 1,760, `ndsR2AnimAObjToQ` 1,754, `gcAddDObjAnimJoint`
1,642. **That is the next slice, and it is an architectural one.**

It also re-opens *half* of slice 32. The AOT bake's **size** refutation stands
untouched — 4.46x the source, +679,490 B against a 32,768 B keep-free floor. But
its **value** model was "commands are parsed once per playback, so a bake only
wins on the 64.6% of loads that repeat", and that was reasoned from mean cost.
The gate says the attach path it would delete is 7.4% of the non-idle premium.
A cheaper attach is worth more than that model priced it — just not at 4.46x.

#### The caveat that has to travel with this table

**32.4% of the non-idle premium is the measuring apparatus.**
`ndsPlatformRenderDebugHud` alone is 45,381 cyc/frame, and `NDS_TICK_HUD := 1` is
set only for `smash64ds-battle-playable-tickhud-hwtri` and
`smash64ds-results-lab-hwtri` (`Makefile:1458-1463`) — the published
`smash64ds-battle-playable-hwtri` has none of it, nor the `printf`/`scanf`/
console/locale machinery it drives. The partition is therefore partly selected by
the instrument's own cost.

This does not invalidate the gate figure — every historical number carries the
same instrument, so they remain comparable — but **it does mean lever selection
from this table must exclude those rows**, which is what the class breakdown
above does. It also means the published ROM's over-gate composition is not this
one, and nobody has measured that. Worth the owner knowing.

### The next owner is gated, not open — read this before proposing a conversion

Slice 34's attribution ranks the remaining soft-float by size. **Size is not
permission.** `scripts/census-softfloat-callers.ps1` records the constraint and
this lane must be read through it:

> Float in `gmcollision`, `mp*`, `ftMain*` and `ftComputer` is frozen by the
> Task 9 state hash and by `PROJECT_GOAL.md`'s mechanical-equivalence contract,
> so it cannot be converted to fixed point whatever it costs. Float in the
> renderer gates on the fidelity budget instead, where a fixed-point equivalent
> is the owner's call.

Applying that to the measured table:

| family | float tk/fr | conversion status |
|---|---:|---|
| map collision (`mp*`, `gmCollision*`) | 16,649 | **FROZEN** — exact moves only |
| matrix / camera (`syMatrix*`, `guMtxCatF`, `syVector*`, `syUtilsArcTan`) | 14,810 | renderer-side — **owner's call** |
| animation | 8,772 | mixed; and every item is under the floor (slice 34) |
| particles | 4,609 | renderer-side — owner's call |

**Slices 35–37 are what the frozen half looks like when it is done right.** They
banked **−10,752** against map collision without touching a single numeric: three
memos of answers that are pure functions of static stage geometry, each one
bit-identical by construction, `gNdsR2FtAnimParseCalls` unchanged in every arm.
That is the whole playbook for frozen code — **memoise the answer, cut the call
count, delete redundant work** — and it is still open there: `…GetFCCommonFloor`
is 818 cycles a call over 45,372 calls and 365 flat PCs, and `func_ovl2_800F8FFC`
still calls it once per floor line while an object is over at most one.

**What needs the owner, stated once so it can be decided rather than rediscovered:**
converting the matrix/camera family to fixed point. It is 14,810 tk/fr of soft
float plus its self time, `__aeabi_fdiv` alone is **10,084 tk/fr over 308,426
calls** (117.9 cycles each, the most expensive helper in the build by 3.2x), and
the DS has an idle hardware divider. `PROJECT_GOAL.md` puts visual fidelity above
gameplay fidelity in the sacrifice order and explicitly allows fixed-point
replacement, so this is inside the contract — but it is a renderer-fidelity call,
and the standing rule is that permanent implementation of a fidelity trade
requires owner approval. **Do not start it on the strength of the tick count.**

**And size it correctly when it is approved.** Slice 37 proved mean self time
predicts P50 and not P95: two functions of near-identical mean cost gave P95 wins
2.45x apart. The matrix family's 14,810 is a mean. Before promising a gate
number, get the per-frame distribution of its calls.

### Slice 37: the line-kind memo — `WORK-H` P95 −1,984, and mean self time does NOT predict P95

Third of the three `line_id` scans. One binary, `builds/build-c118-mp-ab3`
(`NDS_R2_MP_ROUTE=1 NDS_R2_BOTH_CPU=1`), `gNdsR2MPRoute` **7 versus 3**, so this
row is slice 37 with slices 35 and 36 held on in both arms.

`ndsMPGetLineKindForLineID` is memoised into one byte per line: 0 unresolved,
1 the function's `-1` answer, otherwise `kind + 2`. Sixty-four bytes total, both
outcomes cached (neither exit touches a counter), dropped by the same
`ndsMPVertexF32Reset` slice 30's setter calls.

| bucket | route 7 P50 | route 3 P50 | ΔP50 | route 7 P95 | route 3 P95 | ΔP95 |
|---|---:|---:|---:|---:|---:|---:|
| **`WORK-H`** | 961,920 | 964,096 | **−2,176** | 1,293,504 | 1,295,488 | **−1,984** |
| `SRC` | 333,312 | 335,232 | −1,920 | 645,376 | 648,832 | −3,456 |
| `GCRA` | 328,320 | 330,112 | −1,792 | 640,320 | 643,584 | −3,264 |
| `SCPU` | 38,528 | 39,872 | −1,344 | 93,312 | 97,280 | −3,968 |
| `SINT` | 151,104 | 152,640 | −1,536 | 337,216 | 337,280 | −64 |
| `SPHD` | 68,736 | 69,376 | −640 | 117,248 | 118,272 | −1,024 |
| `FTR` / `STG` | 302,080 / 186,304 | 302,016 / 186,240 | +64 / +64 | 305,664 / 193,600 | 305,856 / 193,600 | −192 / **0** |
| `ALL` | 1,118,208 | 1,118,208 | **0** | 1,678,656 | 1,678,720 | −64 |

**Engagement.** `gNdsMPLineKindHits` **57,909** / `Fills` **14** with the memo;
**115** / **57,808** without. `gNdsMPLineYakumonoHits` is **71,340 in both arms**
— bit 2 is genuinely held constant, so this really is slice 37 alone.
`gNdsR2FtAnimParseCalls` **145,549** in both.

#### The finding: two functions of the SAME mean self cost, P95 wins 2.45x apart

| slice | function | mean self | ΔP50 | ΔP95 | P95 / mean |
|---|---|---:|---:|---:|---:|
| 36 | `ndsMPFindLineYakumonoID` | 2,666 tk/fr | −1,984 (74%) | **−4,864** | **1.82x** |
| 37 | `ndsMPGetLineKindForLineID` | 2,588 tk/fr | −2,176 (84%) | **−1,984** | **0.77x** |

**Mean self time predicted the P50 of both, within 74–84%, and predicted the P95
of neither.** These two functions cost the same on average and their P95 savings
differ by 2.45x. The discriminator is not size, it is **whether the calls
concentrate on the frames that sit at `WORK-H`'s 95th percentile**: slice 36's do
(collision-heavy frames call it more), slice 37's are frame-uniform, so its P95
saving is its mean saving and no more.

**This campaign optimises a P95 gate, so it cannot rank candidates by mean self
time — and it has been doing exactly that.** A census row in tk/fr is a P50
lever's size. To size a P95 lever you need the per-frame distribution of its
calls, which the ring's per-frame columns can give (`-PerFrameGlobals`) and the
whole-match census cannot. This also retroactively explains cycle 117: animation
work is spread evenly over frames, so its levers kept measuring at their mean and
never bought a P95 multiple.

#### And it corrects my correction, not just E51

Three positions, in order, each wrong in a different way:

1. **R2-03 E51:** do not build a `line_id -> (group, kind)` table — the yakumono
   loop's trip count is one, so there is no O(n) to remove. *Right about the
   loop; the conclusion "not worth building" was closer to the truth than what
   replaced it, but for a reason that is not the operative one.*
2. **This board, slice 36's first revision:** retired slice 37 on E51's
   authority, without measuring. *Deferring to a prior refutation instead of
   spending a free profile run.*
3. **This board, slice 36's second revision:** withdrew that, and predicted slice
   37 was worth roughly what slice 36 was, because the profile gave it a HIGHER
   per-call cost (194 vs 166) and 97% of the total. *The profile was right and
   the inference was wrong: it gave a MEAN, and I converted a mean into a P95
   prediction by analogy with a function whose calls have a different frame
   distribution.*

The measurement: **−1,984, which is 41% of slice 36.** Keep it — it is real work
deleted, proven engaged, and free at the gate — but the durable output of this
slice is the row above it, not the tick count.

### Slice 36: the yakumono-id memo — `WORK-H` P95 −4,864, isolated on one binary

The companion to slice 35, and the reason the route went bitwise: one binary,
`builds/build-c118-mp-ab2` (`NDS_R2_MP_ROUTE=1 NDS_R2_BOTH_CPU=1`),
`gNdsR2MPRoute` **3 versus 1** — both memos against the endpoint memo alone, so
this row is slice 36 and nothing else.

#### The shape, and why it is bigger than one caller

`ndsMPFindLineYakumonoID` is `line_id -> yakumono_id` over the same static
geometry as slice 35. It is **out-of-line (232 bytes) with TEN call sites**, not
the one I first sized it from — `mpCollisionGetFCCommonFloor` calls it once per
call (45,372 a match), and the measured total is **71,353**.

**It memoises MISSES as well as hits**, which slice 35 could not. Neither of its
exits touches a counter, so there is no observable a served answer could stop
incrementing — and the miss is the expensive half: a line with no yakumono walks
every yakumono and every kind before returning FALSE, where a hit stops at the
match. Caching only hits would have left the costly outcome alone.

#### Measured

| bucket | route 3 P50 | route 1 P50 | ΔP50 | route 3 P95 | route 1 P95 | ΔP95 |
|---|---:|---:|---:|---:|---:|---:|
| **`WORK-H`** | 960,064 | 962,048 | **−1,984** | 1,294,976 | 1,299,840 | **−4,864** |
| `SRC` | 329,728 | 331,392 | −1,664 | 640,960 | 641,728 | −768 |
| `GCRA` | 324,608 | 326,336 | −1,728 | 635,840 | 636,736 | −896 |
| `SCPU` | 37,248 | 38,656 | −1,408 | 90,944 | 94,656 | −3,712 |
| `SINT` | 148,096 | 149,312 | −1,216 | 338,176 | 334,976 | +3,200 |
| `SPHD` | 68,928 | 69,056 | −128 | 117,376 | 116,736 | +640 |
| `FTR` / `STG` | 302,336 / 188,032 | 302,400 / 188,032 | −64 / **0** | 306,112 / 194,240 | 306,240 / 194,240 | −128 / **0** |
| `ALL` | 1,118,336 | 1,118,336 | **0** | 1,678,848 | 1,678,848 | **0** |

**Read the sub-buckets at P50, not P95, on a cut this size.** The P50 column is
coherent — `SRC` −1,664, `GCRA` −1,728, `SCPU` −1,408, `SINT` −1,216 — while at
P95 `SINT` reads **+3,200** and `SPHD` **+640** against a `WORK-H` of −4,864.
Those are different frames occupying each bucket's 95th position, not a
regression; the controls settle it. **`STG` and `ALL` are exactly 0 at BOTH
percentiles** and `FTR` is ±128.

**Engagement.** `gNdsMPLineYakumonoHits` **71,340** / `Fills` **13** with the
memo; **45** / **71,308** without. Thirteen fills a match — like slice 35's ten,
the memo does not thrash on geometry reassignment. `gNdsR2FtAnimParseCalls` is
**145,549 in both arms**: identical simulation, so no collision answer moved.

#### Three points on ONE binary — and slice 35 reproduces on a second

| route | memos | `WORK-H` P50 | `WORK-H` P95 |
|---|---|---:|---:|
| **0** | neither | 968,896 | 1,307,392 |
| **1** | endpoint only | 962,048 | 1,299,840 |
| **3** | both | 960,064 | **1,294,976** |

| step | slice | ΔP50 | ΔP95 |
|---|---|---:|---:|
| 0 → 1 | 35 | −6,848 | **−7,552** |
| 1 → 3 | 36 | −1,984 | **−4,864** |
| **0 → 3** | **both** | **−8,832** | **−12,416** |

**Slice 35 measured −7,232 on `build-c118-mp-ab` and −7,552 here on
`build-c118-mp-ab2` — two separately-linked binaries agreeing within 320 ticks**
on a cut the ±8,544 cross-build floor could never have shown. That is the
strongest evidence in this lane that the route instrument is reading real work
and not placement.

**The combined −12,416 clears the floor**, so unlike every c117 animation slice
this one is bankable across builds. `gNdsR2FtAnimParseCalls` is **145,549 in all
three arms**; `ALL` P95 is **identical to the tick** in all three (1,678,848);
`STG` P95 moves 64 across the whole span. The VBlank histogram is monotone —
2-VBlank frames **1578 → 1603 → 1604** — which is the same ordering the buckets
give, from an independent counter.

#### The floor does not apply here, and that matters for how to read −4,864

A same-binary route A/B has **no cross-build placement floor** — that is the
whole reason standing rule 7 exists. −4,864 would be invisible across builds and
is attributable here. It is a smaller win than slice 35's −7,232 for a reason
worth carrying: **slice 35 memoised a function whose SEARCH was the cheap part**
(the link reads, vertex-id reads, four coordinate reads and `(f32)` conversions
after it were the cost), while this function is search plus a single halfword
read. What is left to remove is mostly call overhead and the scan itself.

#### It reconciles against the function's own self time, which is the check worth copying

`--pc-detail` on the c117 whole-match profile prices the pre-slice function
exactly: **9,610,869 cycles over 57,851 calls — 166 cycles a call, 2,666 tk/fr
mean.** Against that:

- **P50 −1,984 is 74% of the mean self cost.** That is the right shape: the memo
  removes most of the body but still pays the `bl`, the prologue, the bounds test
  and a state load. A P50 saving at or above 100% would have meant the number was
  measuring something else.
- **P95 −4,864 EXCEEDS the mean self cost, and that is not a contradiction.**
  2,666 tk/fr is the average over all frames; the frames sitting at `WORK-H`'s
  95th percentile are the collision-heavy ones, where this function runs more
  than averagely often. Percentile savings are not bounded by mean self time.
  Do not "correct" a figure for this — check it reconciles at P50 instead.

#### And this is where I was wrong about slice 37 — E51 does not retire it

An earlier revision of this entry said the third scan function,
`ndsMPGetLineKindForLineID` (reached from `ndsMPLineIDIsFloor` on every one of
`mpCollisionGetFCCommonFloor`'s 45,372 calls), was **search and nothing else** and
therefore exactly what R2-03 E51 refuted — so "do not build it". **The profile
run to settle that question says the opposite and the claim is withdrawn:**

| function | calls | cyc/call | cycles | tk/fr |
|---|---:|---:|---:|---:|
| `ndsMPFindLineYakumonoID` (slice 36) | 57,851 | 166 | 9,610,869 | 2,666 |
| `ndsMPGetLineKindForLineID` | 47,980 | **194** | 9,329,165 | **2,588** |

Its per-call cost is **higher** than slice 36's target and its total is **97% as
large**, so it is worth roughly the same, not less. **E51 explains why the LOOP is
not the cost — trip count 1 — it does not say the function is cheap.** What costs
194 cycles is a `bl`, a nine-register prologue, `ndsStageCollisionLoopGeometryReady`,
the O2R halfword reads per kind, and the epilogue; its hottest PC is the epilogue
itself at 7.0%, i.e. flat like the others. Take it as route **bit 4**.

The lesson is the one this lane keeps re-teaching: **a structural argument about
where cost "should" be does not survive contact with a per-PC profile.** The
argument was sound about the loop and wrong about the function.

### Slice 35: the endpoint memo — `WORK-H` P95 −7,232 on one binary

The slice **slice 29's own post-mortem asked for** ("the endpoint memo is cheap
and safe once it lands") and **slice 30 unblocked** by moving invalidation to the
assignment of `gMPCollisionGeometry`. One binary,
`builds/build-c118-mp-ab` (`NDS_R2_MP_ROUTE=1 NDS_R2_BOTH_CPU=1`), 1600 frames
from 438, DLDI ON, `gNdsR2MPRoute` **1 versus 0**, both set explicitly.

#### Why this function and not an instruction inside it

`ndsMPFindLineEndpoints` is 21,127,734 cycles in the c117 whole-match profile —
**38,890 calls at 543 cycles each, 5,861 tk/fr** — and its per-PC detail says
there is nothing in it to delete: **270 distinct PCs, the hottest 3.6% of the
function**, and the top rows are `ldr r3,[sp,#64]` and `ldr r2,[sp,#4]` at
**19.5 and 19.3 cyc/insn** — a cold frame reload on entry, not a loop. All three
collision owners profile that way (`mpCollisionGetFCCommonFloor` 45,372 calls x
818 cyc over 365 PCs, top 1.9%; `ndsStageMPSweepFloorLoopSweep` 11,544 x 2,643
over 523 PCs, top 2.6%). **A flat function offers exactly one lever: not
entering it.**

Its answer is a pure function of the line id and the current geometry — it reads
`line_info`, `vertex_links`, `vertex_id`, `vertex_data`, and nothing else — so it
memoises outright rather than incrementally.

**This is NOT the refuted table.** R2-03 E51 killed a `line_id -> (group, kind)`
table because the yakumono loop's trip count is one on Dream Land (1 yakumono, 7
lines). This profile agrees — **2.83 inner iterations a call** — and this slice
does not touch the search. It removes the ~60 instructions of link, vertex-id,
coordinate and `(f32)` conversion work that FOLLOW it.

#### Measured

| bucket | A P50 | B P50 | ΔP50 | A P95 | B P95 | ΔP95 |
|---|---:|---:|---:|---:|---:|---:|
| **`WORK-H`** | 963,776 | 970,304 | **−6,528** | 1,296,256 | 1,303,488 | **−7,232** |
| `SRC` | 330,496 | 338,240 | −7,744 | 637,440 | 646,592 | **−9,152** |
| `GCRA` | 325,504 | 333,184 | −7,680 | 632,448 | 641,408 | **−8,960** |
| **`SPHD`** | 67,776 | 71,488 | −3,712 | 115,520 | 123,008 | **−7,488** |
| `SINT` | 149,184 | 150,272 | −1,088 | 336,576 | 339,328 | −2,752 |
| `SCPU` | 38,848 | 40,000 | −1,152 | 95,488 | 96,704 | −1,216 |
| `FTR` / `STG` | 303,040 / 187,520 | 302,976 / 187,520 | +64 / **0** | 307,072 / 194,880 | 306,944 / 194,880 | +128 / **0** |
| `ALL` | 1,118,272 | 1,118,272 | **0** | 1,678,784 | 1,678,848 | −64 |

**The saving lands in `SPHD`** — the per-fighter map/physics arm — which is where
map collision lives and the only place it could legitimately land.

**Controls.** `STG` is **exactly 0** at both percentiles, `ALL` P50 exactly 0,
`FTR` ±128. **`gNdsR2FtAnimParseCalls` is 145,549 in BOTH arms**: the simulation
did identical work, so no collision answer changed — a divergence would move the
fighters and that count with them. `slips=0` in both.

**And the VBlank histogram moved the right way**, independently of the buckets:
2:**1602**/3:378/4:39/5+:19 with the memo against 2:**1587**/3:389/4:45/5+:17
without. Fifteen more frames presented at the fastest cadence, max 20 either way.

#### Engagement, and the one place the number is generous

`gNdsMPLineEndpointHits` **48,082** with `…Fills` **10** in the candidate;
**32** and **48,060** in the control. The 32 are real and expected — `-SetGlobals`
pokes at the first frame-complete marker and the ROM's default is 1, so 0.07% of
calls ran memoised before the window opened. It is also positive proof the poke
landed.

**Ten fills a match is the finding behind the finding.** The memo does not
thrash: `gMPCollisionGeometry` is reassigned rarely enough that the shared reset
costs nothing, which is the thing that could have made this worthless and is why
the counter was carried.

**The fill is not route-gated, and an earlier draft of this entry claimed that
made the arms symmetric. The counters say it does not.** The candidate returns
from the fast path above the fill (10 fills); the control re-fills on every call
(48,060). So the control is slightly slower than the true pre-slice baseline and
**the −7,232 overstates the win by roughly those fills, ~200 tk/fr**. Call it
**−7,000**. Gating the fill would have put a second route test on the hot path to
make an already-decisive number 3% prettier.

#### Exactness

Structural, not statistical. The served entry is the same expression on the same
inputs: `(f32)` of an s32 is exact, and the left/right choice is the same
`v_first_x <= v_last_x` compare, resolved into locals so the calls that pass
NULLs (`mpCollisionCheckExistLineID` passes five) still fill what they do not
read. Three restrictions carry the risk:

- **Hits only.** Both FALSE exits increment counters
  (`gNdsStageCollisionLoopBadVertexCount`, `…OutOfRangeLineCount`); the hit path
  increments none, so memoising the hit alone is observationally identical rather
  than merely close.
- **No bind, ever.** Cycle 117 added `ndsMPVertexF32Bind` to THIS function and
  the match diverged reproducibly at frames 1015/1495/1686. The memo is dropped
  by `ndsMPVertexF32Reset`, which slice 30's setter calls when the geometry stops
  being current. That is what makes it safe now and was not then.
- **Line id at or past 64 falls through**, counted by `…Overflow`.

The vertex data is LOCAL-space and static — the same invariant slice 28's extent
cache already ships on, where a moving platform is handled by the caller
subtracting the yakumono translate rather than by the geometry moving.

#### Next in this lane, ready to build

`ndsMPFindLineYakumonoID` is the same shape — a pure `line_id -> yakumono_id`
function over the same static geometry — and `mpCollisionGetFCCommonFloor` calls
it once per call, 45,372 times, inside the largest remaining collision symbol
(10,294 tk/fr self). One `u8` per line. Take it as route **bit 2**, generalising
`NDS_R2_MP_ROUTE_ON()` to a bit test the way `gNdsR2AnimCutRoute` already is, so
one binary attributes both.

### Slice 34: the soft-float bill, attributed exactly — and the lane hands over

`scripts/task37_softfloat_callers.py`, whole-match c117 profile, 1,800 presented
frames. **No build spent.**

#### Why this measurement had to exist

The ARM946E-S has no FPU, so the libgcc float routines are the largest
addressable class in the build — **277,647,083 cycles, 10,267,766 calls,
77,016 tk/fr, 8.35% of all non-idle cycles** — and a PC sampler charges every
one of them to `__aeabi_fadd`, which names no code to change. Cycle 117 kept
guessing at callers from static grep and loop bounds.

It is now exact, not estimated. **A `bl __aeabi_fadd` is an instruction with its
own PC**, so the profiler's execution count at that PC is that call site's exact
call count. 8,208 static sites, 3,399 executed, 316 callers, **100% attributed**.
Per-call cost is measured too (callee cycles ÷ callee calls), so ITCM residency
and operand-dependent early-outs are already inside the number: **fadd 37.3,
fmul 25.4, fdiv 117.9, fcmpeq 10.6, fcmplt/gt 14.6, i2f 16.7, f2iz 12.7**.

**libgcc FALLS THROUGH and the first run of this got it wrong.** `__aeabi_fsub`
is four bytes — flip the second operand's sign — and then execution runs off its
end into `__aeabi_fadd` with no branch. Its own range measures **1.0 cycles a
call** while its work is charged to fadd. That misfiled **1,392,725 calls** and
reordered the table: the collision kernels are fsub-heavy, the matrix kernels
fmul-heavy, and the uncorrected run put a matrix function on top. The tool now
detects fallthrough from address adjacency plus an under-4-cycle per-call cost
and prints the pooling. **Do not read a soft-float table that does not say
`pooled`.**

#### Who pays it — tk/fr, by family

| family | float tk/fr | largest single |
|---|---:|---|
| **map collision** | **16,649** | `ndsMPFCSegmentCrossesKernel` 5,518 (752,325 calls, 20 sites) |
| **matrix / camera** | **14,810** | `syMatrixLookAtReflectF` 4,325 (515,062 calls, 81 sites) |
| **animation** | **8,772** | `ndsBaseGcPlayMObjMatAnim` 4,631 (651,431 calls, 34 sites) |
| particles | 4,609 | `ndsRendererSubmitParticleQuad` 2,217 |

`__aeabi_fdiv` alone is **10,084 tk/fr** on only 308,426 calls — the most
expensive helper per call in the build by 3.2x. Its owners are
`syMatrixPerspFastF` x44,799, `syUtilsArcTan` x44,325,
`gmCollisionTestRectangle` x13,053, `…BuildNativeMaterialSnapshot` x13,016.

#### The animation lane hands over, and here is the arithmetic

Animation SELF time is **80,802 tk/fr** across seven symbols, but its two
largest — `gcPlayDObjAnimJoint` 18,856 and `ndsR2AnimValueQ` 18,616 — are
**already the Requirement 4 fixed-point path, running at 1.69 and 1.67
cyc/insn**. There is no float left in them to convert and no stall to place.
What remains recoverable in animation is the **8,772 tk/fr** above, spread over
three symbols whose largest is 4,631 — **every one under the ±8,544 cross-build
floor**, on top of four levers cycle 117 already measured under it (idle-joint
skip −5,632 `SRC` / `WORK-H` flat, lazy track table −7,104 routed / +576
re-banked, AObj walk ~1,050, track dispatch ~1,900) and one refuted outright
(the AOT bake, on residency).

**Collision float is 1.9x animation's largest remaining item**, and that is
before its self time: `mpCollisionGetFCCommonFloor` 10,294,
`ndsStageMPSweepFloorLoopSweep` 8,462, `ndsMPFindLineEndpoints` 5,861,
`ndsStageMPAdjustFloorLoopWallSweep` 4,725, `ndsMPFCSegmentCrossesKernel` 3,735.
The goal block predicted map collision as the next P95 owner; **the re-profile
confirms it quantitatively.**

#### Two traps for whoever takes the animation float anyway

- **`ndsBaseGcPlayMObjMatAnim` cannot be blanket-converted to Q.** Five of its
  tracks — `PrimColor`, `EnvColor`, `BlendColor`, `Light1Color`, `Light2Color` —
  carry **packed 0xRRGGBBAA in the f32's bit pattern**, which is why
  `ndsAObjEvent32CorrectMObjColors` reads them through a union. A Q conversion
  of those AObjs destroys the payload and looks like a palette bug. Only the
  scalar tracks may convert.
- **The saving would land in `GCRA`, not `SINT`** (slice 31's finding), and
  `gcPlayAnimAll` reaches this player through `ndsBaseGcPlayAnimAll`, not
  through the port's `gcPlayDObjAnimJoint`. The `gcPlayAnimAll` half of the
  animation system is the half Requirement 4 never touched.

#### The parser, per-PC — what 365.6 cycles a call actually buys

Free from the same profile (`--pc-detail`, pre-slice-33 ROM, so the guard it
deleted is still visible): **165,639 calls, 365.6 cycles each, 31.5% of them
returning immediately** at the `anim_wait == AOBJ_ANIM_NULL` test — exactly the
population slice 33 then removed at the caller.

The cost is memory, not interpretation. **The top five load/store PCs are 27.9%
of the whole function**: `ldr r0,[r0,#116]` — the `anim_wait` read itself — is
**34.96 cyc/insn over 165,639 executions, 1,606 tk/fr in one instruction**, and
the AObj walk's `ldrb r3,[r4,#4]` is 27.36. Prologue and epilogue are 8.9%
(32.7 cycles a call) and `gNdsR2FtAnimParseCalls++` is **1,105 tk/fr** on its
own. A smaller *encoding* does not touch any of this; only a smaller working set
would, and slice 32 priced that at 4.46x too large.

### Slice 33: the idle joint stops paying two calls to say nothing

**The shape.** `ftParamUpdateAnimKeys` runs one parse call and one play call per
joint per frame. Both are total no-ops when that joint's `anim_wait` is
`AOBJ_ANIM_NULL`, and **31.5% of joints are in that state on any given frame**
(66,967 of 212,600 parse calls a match, from `gNdsR2FtAnimParseCalls` minus the
early-out and stepped counters). The caller can ask in three instructions what
the callees were spending two full calls to discover.

**Priced off the shipped Thumb, not estimated.** The `AOBJ_ANIM_NULL` path
through `ndsR2FtAnimParseDObjFigatree` is 16 instructions in and 7 out, and
`gcPlayDObjAnimJoint` is 13 and 7 — 45 instructions with the two `bl`s, of which
**40 are stack word accesses**: each function pushes nine registers and reserves
a frame (68 bytes and 12 bytes) before comparing one word and leaving. That is
the working-set traffic the goal names, not merely instruction count.

**Why it is a deletion and not an approximation.** All five reachable bodies
wrap their ENTIRE contents in the same guard — `gcParseDObjAnimJoint`,
`ftAnimParseDObjFigatree`, `ndsR2FtAnimParseDObjFigatree`,
`gcPlayDObjAnimJoint`, `lbCommonPlayTranslateScaledDObjAnim`. That premise is
held by `scripts/check_anim_null_guard.py`, registered in
`check-gbi-decode-fixtures.ps1`, **not by this paragraph**: a counter or a cache
poke added above one of those guards later would make the skip drop real work,
and the symptom — one joint's bookkeeping stopping on the frames it is idle — is
invisible to a screenshot and to every geometry counter. The checker earned its
place immediately, failing on three real statements above the port parser's
guard before route reads were allowed by name.

**Two things deliberately stay outside the skip**, and both would be silent
corruption if they moved:

- the **MObj loop**, because an MObj carries its own `anim_wait` and animates
  while its joint is idle;
- **`translate_scales`**, which indexes the joint array — a pointer that stopped
  advancing on idle joints would mis-scale every later joint in the fighter.
  Verified in the disassembly: the skip path branches to the same `adds r6,#12`.

**The second loop is deliberately untouched.** `ftParamUpdateAnimKeys`'s
`motion_id == -2` arm forces `anim_wait = AOBJ_ANIM_END` around its play call and
restores it after, so the guard there always passes; a predicate would be dead
code, and skipping on the *restored* value would change behaviour.

**Route bit 32**, for attribution only — the cut is provably equivalent, so it
ships ON like every other bit in this file (see the default-0 trap under slice
31).

#### Measured: KEEP on engagement, **flat at the gate**

One binary, `builds/build-c117-anim-ab`
(`NDS_R2_ANIM_CUT_ROUTE=1 NDS_R2_BOTH_CPU=1`), 1600 frames from 438, DLDI ON,
route **31 versus 63**, both set explicitly:

| bucket | A P50 | B P50 | ΔP50 | A P95 | B P95 | ΔP95 |
|---|---:|---:|---:|---:|---:|---:|
| **`WORK-H`** | 969,472 | 969,472 | **0** | 1,304,896 | 1,305,216 | **+320** |
| `SRC` | 340,480 | 339,072 | −1,408 | 660,416 | 654,784 | **−5,632** |
| `GCRA` | 335,744 | 334,336 | −1,408 | 655,360 | 649,856 | **−5,504** |
| `SINT` | 157,248 | 156,480 | −768 | 352,832 | 347,264 | **−5,568** |
| `FTR` / `STG` | 299,968 / 188,992 | same / same | 0 / 0 | 303,872 / 195,648 | 303,680 / 195,712 | −192 / +64 |
| `ALL` | 1,118,272 | 1,118,272 | **0** | 1,678,720 | 1,678,720 | **0** |

**The engagement proof reconciles to the call, not to a percentage.**
`gNdsR2FtAnimParseEarlyOut` 108,186 and `gNdsR2FtAnimParseStepped` 37,363 are
**byte-identical in both arms** — the skip removed no-op calls and nothing else.
`gNdsR2FtAnimParseCalls` fell 212,516 → 145,600, exactly 66,916; the 51 idle
entries that survive arrive from `func_ovl2_800ECCA4`, the parser's other
caller, which has no predicate. `gNdsR2FtAnimNullSkips` 72,260 exceeds 66,916 by
5,344 because joints on the `is_anim_joint` arm are skipped too and
`gcParseDObjAnimJoint` has no counter.

**Verdict: KEEP, and it does not move the gate.** The saving is real, signed the
same way in all three buckets that own the work, at both percentiles, with the
untouched buckets flat to ±192 and `ALL` byte-identical. `WORK-H` +320 is inside
the pair's own noise — arm A carried three duplicate rows and one 5.31M outlier
frame, arm B none and 3.36M — so this reads **flat**, not as a regression.
AGENTS.md's measurement law is explicit that tick targets are directional and
not per-cut discard gates, so a proven, correctness-preserving deletion banks
toward the target whether or not it clears a floor on its own.

#### What this measured that changes the plan

The first reading of this table was wrong and is recorded here so it is not
re-derived. `OTHR` P95 524,352 looked like the largest unattributed owner —
`named` is only 81.7% of `ALL`. It is not an owner at all:
`taskman_seam.c:5172` states that **`OTHR` is what the loop spends parked in
`swiWaitForVBlank`**, measured by Task 65 at 17.50% of wall against an `OTHR` of
16.4%. The arm-A numbers agree to about 20,000 at both percentiles (`OTHR`
196,736/524,352 versus `WAIT` 176,896/504,768). `OTHR` is idle. Do not spend a
slice attributing it.

What the table does say, read correctly:

| bucket | P50 | P95 | share of `WORK-H` P95 |
|---|---:|---:|---:|
| `SRC` ≈ `GCRA` | 340,480 | 660,416 | 50.6% |
| **`SINT`** | 157,248 | **352,832** | **27.0%** |
| `SHDT` | 4,288 | 172,800 | 13.2% |
| `SPHD` | 69,888 | 118,976 | 9.1% |

`GCRA` is 99% of `SRC` because `gcRunAll` is the sole gateway to the whole
simulation, so neither is a subsystem. **`SINT` is, and it is 27% of the
judging bucket at P95.** The fighter-animation lane is therefore a genuine P95
owner and goal condition (2) is decisively false — the cost is recoverable, and
the board's own sizing of the full AOT rebuild (≈38,700 ticks/frame, ≈60,000
carrying fixed point to the matrices) is 21–32% of the remaining gap.

What slice 33 actually establishes is narrower and still useful: **the idle path
was not where the money was.** It is 31.5% of the calls and 1.6% of `SINT` P95,
which is the third instance this cycle of a call share not being a cost share.
The expensive path is the one already measured at ~440 instructions per call —
the *stepped* parse and the player it feeds. That is where the next slice goes,
and it is the AOT dense-track rewrite already specified as slice 32, not another
predicate.

### Slice 31: the animation parser stops rebuilding its track table

**KEEP on a same-binary A/B: `WORK-H` P95 -7,104, `GCRA` P95 -10,368.**

`ndsR2FtAnimParseDObjFigatree` cleared a 10-entry `track_aobjs[]` and walked the
DObj's whole `aobj` chain before the event loop -- ~10 instructions over ~10
36-byte pointer-reached nodes, so **~100 instructions against the 95.7 the
profile measures for the entire call.** The walk WAS the call, 200,231 times a
match, rebuilding a table that is invariant while the list is unchanged.
`ndsR2AnimAObjToQ` is not the cost: once migrated it early-outs in ~5
instructions.

It now builds on first use, at most once per call, and only on calls that read
it -- the `anim_joint.event16 == NULL` exit never does.

**The audit came before the code, because the failure mode is silent.**
`NDS_R2_FTANIM_ENSURE()` ALLOCATES on NULL, so a build that misses a read site
does not crash and does not read garbage -- it creates a **duplicate AObj**. All
41 `track_aobjs[i]` reads were checked against the 7 `ENSURE()` sites: exactly
one block reads without it, `nGCAnimEvent16SetTranslateInterp`, which the source
already flagged as "the only creation site outside NDS_R2_FTANIM_ENSURE". Both
trigger the build. `ndsR2AnimAdvanceTail` was checked too and needs nothing: it
dispatches per node on `kind >= NDS_R2_AQ_KIND_BASE`, so deferred migration
leaves it correct.

**No cross-call state, deliberately.** A cached table is the obvious move and
cycle 117 lost two collision slices to exactly that, one still unexplained.

**Measured on ONE binary** (`builds/build-c117-anim-ab`, built with
`NDS_R2_ANIM_CUT_ROUTE=1`), route 15 versus 31, both set explicitly:

| bucket | P50 | P95 |
| --- | ---: | ---: |
| `WORK-H` | +768 | **-7,104** |
| `SRC` | -128 | **-10,496** |
| `GCRA` | +64 | **-10,368** |
| `SINT` | +320 | -1,024 |
| `FTR` / `STG` / `ALL` | -64 / +64 / 0 | +128 / -64 / **0** |

`gNdsR2FtAnimParseCalls` 212,516 and `gNdsR2CubicEvals` 299,148 in BOTH arms:
identical work, only the route differs. **The untouched buckets are flat to
+-128 and `ALL` is exactly 0** -- that is why the route matters. A same-binary
A/B has no cross-build placement floor, so a -7,104 P95 is readable here where
the collision slices' larger bucket wins were not.

**The saving lands in `GCRA`, not `SINT`** -- the `gc*` animation runner and its
parser live there. Do not look for animation wins in `SINT` alone.

**Route bits in this file are ON by default -- read this before adding one.**
`NDS_R2_ANIM_CUT_ROUTE` defaults to 0, which makes `NDS_R2_ANIM_CUT_ON(bit)`
fold to a constant **1**. So a new bit ships ENABLED in every published ROM and
is only switchable in a lab build compiled with `NDS_R2_ANIM_CUT_ROUTE=1`. The
A/B and the shipping default are therefore separate decisions: passing the A/B
does not by itself justify what ships, and a losing candidate has to be
DELETED, not switched off. This is the second instance of the pattern in this
file -- the other is the `#if NDS_R2_CUBIC_FIXED` guard, where writing a Q kind
without the Q player compiled in would silently stop every joint animating in
the bare `make` build.

### Slice 32 (NEXT): the AOT dense-track rewrite

#### UNBLOCKED 2026-08-11 — the input is on disk and the seam is one Makefile rule

Step 13 closed the load-time bake on RAM (records are 32 B; a modest case needs
25,600 B against a 24,404 B heap low-water) and concluded the bake must go to
**ROM at build time**, which put it behind asset plumbing nobody had looked at.
That plumbing already exists:

| piece | where it is |
|---|---|
| **the input** | `decomp/BattleShip-main/BattleShip_o2r/reloc_animations/` — 1,633 files, 7.4 MB, of which **143 `FTMarioAnim*` + 158 `FTFoxAnim*` = the 301 P1 files** listed explicitly in `NDS_MARIOFOX_FIGHTER_RELOC_FILES` (Makefile:2418+). Read-only reference, which is what a generator wants. |
| **the seam** | `Makefile:3295`, `$(NITROFS_DIR)/reloc/%: $(BATTLESHIP_O2R)/%`, whose recipe today is a bare `cp`. One rule to intercept; the staged bank is 1.3 MB. |
| **the precedent** | `scripts/generate_nds_particle_banks.py` is already a build-time generator that reads BattleShip reloc assets and emits banks. Copy its shape, not its content. |
| **the semantics** | `scripts/ftanim_script_model.py` — all 15 opcodes, both timelines, no ROM dependency. |
| **the proof** | `scripts/ftanim_bake.py` — 20,000 scripts round-tripped, 0 mismatches, compared as bit patterns so `0.0 → -0.0` cannot pass. |
| **the surface guard** | `scripts/check_ftanim_opcode_surface.py` — 15 opcodes both directions, plus the escape-point assertion that `func_anim` fires only from Loop/End. |
| **the justification** | the 2026-08-11 whole-match profile above: **25.3% of `gcPlayDObjAnimJoint` is the AObj walk**, memory-bound at 3× the D-cache, and no instruction deletion reaches it. |

#### The format is SOLVED, and the bank is measured

`scripts/ftanim_reloc_probe.py` reproduces the ROM's load pipeline host-side and
walks **5,629 of 5,629 scripts in all 297 AObj16 files**. Registered in
`check-gbi-decode-fixtures.ps1`, because the generator is built on this reading.

The pipeline, from `reloc_backend_assets.c`: 0x50 header → u32 big-endian swap
→ threaded internal fixup chain (`reloc_intern_offset` is a WORD index, the word
packs `next = w >> 16` / `target_words = w & 0xffff`) →
`ndsRelocNormalizeFighterAObj16File`, which **derives** the entry table's length
(it runs until the first script starts), unswaps the u16 lanes across the script
region, and re-encodes each command word's bit order.

**The one thing that cannot be guessed, and the reason eight readings failed at
38–43%: disk command bits are MSB-first.** `opcode = (w >> 11) & 0x1f`, not
`w & 0x1f` — a big-endian compiler allocated `opcode:5, flags:10, toggle:1` from
the top, the native one allocates from the bottom. The per-opcode advance was
right from the first attempt; only the bit order was wrong. The falsified
readings are listed in the probe so they are not retried.

The four non-participating files are `FTMarioAnim134/135` and
`FTFoxAnim135/136`, ids 0x279/0x27a/0x309/0x30a — exactly the four
`ndsRelocIsFighterAObj32Asset` names, which is independent confirmation.

**What is actually in the bank** (the sizing step 13 needed and could not get):

| | |
|---|---:|
| scripts | 5,629 |
| commands | 77,129 (mean 13.7, max 142) |
| **per-track writes — the record count a bake must emit** | **184,629** |
| scripts ending in a Loop rather than End | 830 (4,799 + 830 = 5,629 exactly) |

| opcode | share |
|---|---:|
| **`SetValRateBlock` + `SetValRate`** | **69.6%** |
| `Block` | 8.0% |
| `End` | 6.2% |
| `SetVal0Rate{,Block}` | 7.8% |
| everything else | 8.4% |
| `SetTranslateInterp`, `SetFlags` | **0 — unused by Mario/Fox** |

Two consequences. **The bake is sized**: 184,629 records at step 13's 32 bytes is
5.9 MB of ROM, which `PROJECT_GOAL.md` explicitly permits ("tens or even
hundreds of megabytes"), and a compact per-track record should land near 1.5 MB
— it is a ROM question, not the RAM wall that closed the load-time bake.
**And the format is dominated by one command pair**: seven commands in ten are
`SetValRate{,Block}`, writing `value_target` and `rate_target` with a cubic kind,
so the dense format and any interim fast path should be designed around that
pair first. Flag popcount is 1 on 41,840 of the writing commands, so most
commands touch a single track.

`ftanim_reloc_probe.decode_script()` now returns structured commands — opcode,
flags, the payload word when `toggle` is set, and the per-track TARGET words —
over the whole bank with **zero errors**: 5,629 scripts, 77,959 commands,
**184,629 target words**, which matches the walker's independent per-track write
count exactly. `scripts_in(path)` is the generator's front door.

**Targets are s16 on disk**, spanning −30,707..28,952 across the entire bank.
That is a design input, not trivia: the 32-byte record step 13 priced assumed
seven s32 Q fields, but the source values are already 16-bit, so the dense record
can be far tighter — 184,629 targets is only 369 KB of raw target data.

#### The bake runs on real content — 5,629 scripts, 0 mismatches

`scripts/check_ftanim_real_bake.py`, registered in the fixture suite. 77,959
commands resolve to **145,873 write records** that replay the per-track state
timeline and the callback timeline from records alone — no opcode, no flags
mask — compared as bit patterns.

`run_commands` is a second entry point beside `run_script` rather than a
rewrite of it, because the 20,000-script synthetic proof is what keeps the
resolver honest; `bake_run` is shared so the bake cannot depend on which driver
produced the timeline.

**Real content caught a bug the synthetic scripts structurally could not.** The
decoder returns s16 ints and they were stored straight into fields modelling
`f32`. Python calls `0 == 0.0` true, so the resolver emitted no write record
while the bit comparison distinguishes them — five files failed. The synthetic
driver only ever supplies floats, so it could never have surfaced it.

#### Field ranges over all 145,873 real records — the format sizing

| field | min | max | fits s16 at |
|---|---:|---:|---|
| `value_base` | −10,455 | 24,945 | Q0 |
| `value_target` | −10,455 | 28,952 | Q0 |
| `rate_base` | −4,681 | 13,107 | Q1 |
| `rate_target` | −30,707 | 13,107 | Q0 |
| **`length`** | **−185** | **0** | **Q7** |
| **`length_invert`** | **0** | **64** | **Q8** |
| `kind` | 0 | 7 | 3 bits |

**Read the caveat before using these.** The value/rate rows are the *authored*
s16 disk words, and they fit s16 trivially because they came from s16 — that row
proves nothing on its own. What the runtime actually stores is
`ndsR2AnimTargetValue`'s output, the disk word scaled by a per-track power of two
(`sNdsR2AnimFracShift`), which the model does not apply. Sizing the value fields
needs that scaling folded in first.

`length` and `length_invert` are different and are genuine findings: they are
computed, not authored, and they occupy **−185..0** and **0..64**. Both are s32
today and both fit s16 with fraction to spare — 4 bytes off every record before
touching the value fields at all.

Two more structural facts from the same pass:

- **Track use is heavily skewed.** Tracks 0/1/2 carry 97,391 of 145,873 records
  (67%); tracks 3–9 carry 5,629–8,325 each. Track 3 is exactly 5,629 — one per
  script, i.e. written once and never again.
- **41,000 of the records are initialisation, not animation.** By kind: 90,958
  Cubic, 11,017 Linear, 2,954 Step, and **40,944 with kind None** — first-touch
  snapshots of a track still at its defaults. Those compress to a single
  per-DObj reset, so the real payload is **~105K records**, 28% below the
  145,873 headline and 43% below the 184,629 raw target words.

#### The frac shift, folded in — and the record can be LOSSLESS at 16 bits

`ndsR2AnimTargetValue` scales each disk word by a per-track power of two:
`sNdsR2AnimFracShift[8] = {9,2,12,0,9,5,13,0}`, indexed by Rot/Tra/Sca/TraI with
`+4` for rates. Applying it to every real target gives the *stored* Q12 ranges:

| field | Q12 min | Q12 max | bits+sign | fits s16 at Q12 |
|---|---:|---:|---:|---|
| value Rot | −83,640 | 231,616 | 19 | no |
| value Tra | −4,915,200 | 6,484,992 | 24 | no |
| value Sca | 1,720 | 24,576 | 16 | yes |
| rate Rot | −245,656 | 11,976 | 19 | no |
| rate Tra | −534,656 | 821,120 | 21 | no |
| rate Sca | −2,340.5 | 6,553.5 | 14 | yes |

So a uniform Q12 s16 is impossible — Tra values need 24 bits. **The right
conclusion is not to pick a coarser Q.** The authored value *is* `arg * 2^-k`
with `arg` an s16 and `k` a compile-time constant per track group, so **storing
`arg` itself is exact.** A dense record holding the raw disk word plus the
group's shift loses nothing at all, where re-quantising to Q4 for translation
would have thrown away three fractional bits of every authored value.

That makes the record: kind (3 bits) + track (4) + `value_base`,
`value_target`, `rate_base`, `rate_target` (16 each, authored) + `length` (s16
Q7, measured −185..0) + `length_invert` (s16 Q8, measured 0..64) ≈ **14 bytes
against the AObj's 36**.

**Which is the change that reaches the 25.3%.** 335 nodes a frame at 14 bytes is
4,690 bytes against a 4 KB D-cache — 1.15×, where 36-byte AObjs are 12,060 bytes
and 3.0×. Contiguity alone could never do this; cycle 109 said so and was right.

Two honest exceptions to carry into the emitter:

- **`rate_base` is authored only for the cubic families.** `SetVal{,Block}`
  computes it as `(value_target - value_base) / payload`, so it is not a disk
  word there. That is 11,017 of 104,929 real writes (10.5%) and needs its own
  encoding or a wider field.
- **TraI (track 3) is not a power of two** — its scale is `1/16384 - 3e-12`, so
  it keeps the float expression. It is one track of ten, written exactly once
  per script.

#### The emitter runs — and the record is 20 bytes, not 14

`scripts/generate_ftanim_dense_bank.py --verify`, registered in the fixture
suite: **145,873 records, 2.78 MB, 0 mismatches** decoding back. Both widenings
past the predicted 14 bytes were forced by the round-trip, not chosen:

- **`rate_base`** is authored for the cubic families but `SetVal{,Block}`
  computes it; 753 records are fractional over −614.25..1064.80, and s16 Q4 —
  the only Q that fits that magnitude — carries **81% relative error** on the
  small values. Now s32 Q16, which is what `NDS_R2_AQ_RF` already is.
- **`length_invert`** was sized s16 Q8 from its 0..64 range and rejected on the
  first run: for a Cubic it is `1.0 / payload`, a **reciprocal**, and 1/17 at Q8
  is off by 0.4%. Range was the wrong question for a rate. Now s32 Q30, read per
  kind because Step keeps a frame count in the same field — `NDS_R2_AQ_IF`.

The encoding is **not** lossless overall and the emitter prints what it costs
rather than letting the comparison absorb it: value fields and `length` exact,
`rate_base` 7.3e-06 (0.0011%), `length_invert` 4.6e-10 (0.000006%) — both
inherited from the widths the runtime already stores, not newly introduced.

#### The runtime change is RAM-NEGATIVE — the constraint that killed step 9 is gone

Step 9's load-time bake died on RAM: 25,600 B of records against a 24,404 B heap
low-water. The **live** dense state is the opposite. A live record is
`rate_base` s32 + `length_invert` s32 + `value_base`/`value_target`/
`rate_target`/`length` s16 + a `kind|track` byte = 17 bytes at a 20-byte stride
(dropping the baked record's `cmd_index` buys nothing — the two s32 fields set
the alignment).

#### The fighter share is measured — and it halves this slice's expected value

Counted exactly from the bank, with no build: a joint's live `AObj` count is the
number of distinct tracks its script writes, so summing over a file's scripts
gives one fighter's live node count for that animation.

| one fighter, live nodes | min | median | p95 | max |
|---|---:|---:|---:|---:|
| across all 301 animations | 0 | **78** | 90 | 99 |

Mean tracks per joint is 4.08, max 9. **Two fighters therefore hold ~156 live
nodes of the ~360 in the scene — about 47%.** The rest belong to stage and item
DObjs, which `gcPlayDObjAnimJoint` also serves and which keep the shared list.

That is the number that resizes the whole slice, and it is worse than the
headline:

| | today | fighter-only dense | |
|---|---:|---:|---|
| fighter nodes streamed | 5,616 B | 3,120 B | −44% |
| stage/item nodes streamed | 6,444 B | 6,444 B | unchanged |
| **per-frame working set** | **12,060 B (2.94×)** | **9,564 B (2.33×)** | against a 4 KB D-cache |
| RAM for the converted nodes | 5,616 B | 3,120 B | saves 2,496 B (3,168 at max) |

**So the earlier 6,700 B / 1.64× figure is withdrawn too** — like the RAM claim
it priced every node converting, and only the fighter share can. At 47% of
visits and a 44% byte reduction, the optimistic ceiling on the walk's 17.2M
cycles is ≈3.6M — roughly **1,050 ticks/frame, below the ±8,544 cross-build
floor** and in the same band as slices 31 and 33.

**What this means for the slice.** The dense track is still the right structure
and the offline half is proven, but *the working-set argument alone does not
justify it.* For slice 32 to be worth its remaining cost the win has to come
from **deleting the parse interpretation** — the stepped path at ~440
instructions a call, which the baked records remove outright — with the walk
reduction as a secondary gain. That reframes the runtime work: bind baked
records first, and treat the dense live layout as the thing that makes the bind
cheap rather than as the win itself.

#### And the parse deletion does clear the floor — unlike the walk

`ndsR2FtAnimParseDObjFigatree` is **60,546,909 cycles** whole-match, of which the
stepped path is ~86% (17.6% of calls carrying ~86% of instructions):
**52,070,342 cycles ≈ 15,293 ticks/frame.** A baked bind replaces opcode
dispatch, the flags loop, the payload and target reads, and the Q conversions
with a control read and a 20-byte copy per track.

| residual after the bake | cycles saved | ticks/frame | vs the ±8,544 floor |
|---|---:|---:|---|
| 10% — just copy the records | 46,863,308 | **13,764** | **above** |
| 25% | 39,052,756 | **11,470** | **above** |
| 40% — pessimistic | 31,242,205 | **9,176** | **above** |

**Every case clears the floor**, and the 10% case is **7.4% of the 185,472-tick
gap** on its own. This is the first animation lever this cycle whose sizing
survives contact with the floor, and it is why the slice stays alive after the
working-set argument was downgraded. Bind the records; the smaller node comes
along for free.

#### STOP — the baked form does not fit in RAM, and the value model was wrong too

Checked before writing the loader, and both halves fail.

**Memory.** The animation cache keeps each animation's payload resident so a
move does not re-walk NitroFS mid-frame; cycle 105 measured the match's working
set at **85 distinct animations** on the both-CPU arm, and the arena must keep
`NDS_R2_ANIM_CACHE_ARENA_KEEP_FREE` = 32,768 B free.

| | total | per animation | × 85 working set |
|---|---:|---:|---:|
| figatree source (cached today) | 686,192 | 2,310 | **196,350** |
| baked records + control | 3,060,460 | 10,304 | **875,840** |

**The baked form is 4.46× the source it would replace** — the cache would need
**+679,490 B**. Dropping all 40,944 `kind == None` initialisation records (28.1%
of the blob) only reaches 3.27× and +445 KB. Against a 32 KB keep-free floor and
a 24,404 B heap low-water, neither fits. This is not a tuning problem.

**And the 9,176–13,764 ticks/frame above is an over-estimate**, for a reason
independent of memory: **every command is parsed exactly once per playback.**
The parser runs incrementally as each command's `anim_wait` expires, so over an
animation's run it does the same total work a load-time bake would do. Baking
therefore wins **only where records survive to be reused** — i.e. on repeated
animations, which is 64.6% of force-loads (53 of 82). That is precisely what the
cache is for, and precisely what 4.46× cannot afford.

**What survives.** The offline half is correct and stays — the reader, the bake,
the emitter, the layout guard and the Makefile wiring are all proven and gated
off, and they are the input to any future encoding. What is refuted is *this
encoding at this size*: 20 bytes × 145,873 records cannot be resident.

A future attempt needs the record materially smaller (the two s32 fields are 8
of the 20, and both resisted s16 for measured reasons), or a partial cache of
the most-repeated animations sized to the ~196 KB the source already costs, or
the win taken somewhere other than the parse. **Do not write the loader against
the current format** — it cannot fit, and the arena's failure mode is
`syTaskmanMalloc` spinning forever in `malloc.c:30`, which is the freeze class
the cache comment already documents.

**Correction — the "frees 8,192 B" figure published one step earlier is wrong.**
It priced the whole 512-node pool converting to 20 bytes, which cannot happen:
this specialization is fighter joints only, and `gcPlayDObjAnimJoint` also
serves stage and item DObjs whose AObjs stay on the shared pool. The pool does
not shrink. What converts is the **fighter share** of the live nodes, at 36→20
bytes each, and **that share has not been measured** — the ~360 live figure is
every DObj AObj in the scene, not the fighters'.

So the RAM claim is downgraded to what is actually known: converting N fighter
nodes saves 16N bytes, and N is unknown. It is bounded above by ~360 (saving
≤5,760 B) and the sign is favourable, but "RAM-negative" was over-claimed and is
withdrawn until a counter splits fighter nodes from the rest. **Do not size the
allocation from the retracted number** — a per-joint array of ten reserved
slots would be 2 fighters × 32 joints × 10 × 20 = 12,800 B of new allocation
against a 24,404 B heap low-water, which is the shape that stopped the ROM
booting once already.

The per-frame working-set row is unaffected: it is about bytes *streamed* per
frame, which the 36→20 conversion halves regardless of how the memory is
allocated.

**Nothing about this slice is blocked.** What remains is runtime work: a dense
per-DObj track array for fighter joints only (stage and item DObjs keep the
shared `AObj` list — `gcPlayDObjAnimJoint` serves them too), the parser writing
into it, the player reading it, all behind a route bit so one binary carries
both arms; then the blob through the `cp` rule at `Makefile:3295`,
`EXPECTED_CENSUS_SHA256` re-pinning, and `interpolate` relocation.

#### Original specification, still the target

Re-profiled after the collision work. **Post-Requirement-4 animation is three
symbols and 195,628,633 cycles -- 4.86% of all cycles, 5.9% of non-idle:**

| symbol | cycles | % | tier | bytes |
| --- | ---: | ---: | --- | ---: |
| `gcPlayDObjAnimJoint` | 67,974,137 | 1.69 | `.text.hot` | 604 |
| `ndsR2AnimValueQ` | 67,107,587 | 1.67 | `.main` | 964 |
| `ndsR2FtAnimParseDObjFigatree` | 60,546,909 | 1.50 | `.main` | 2,252 |

**The parser, measured properly.** 200,231 calls a match (`gNdsR2FtAnimParseCalls`,
the runtime counter -- see the warning below), 19,155,412 instructions, so **302
cycles and 95.7 instructions per call at 3.16 cyc/insn**. `.main` runs 42.3%
memory stall, and 3.16 cyc/insn on a 95-instruction body is stall, not
arithmetic: this is pointer-chasing through decomp figatree data, which is
exactly what `PROJECT_GOAL.md`'s compute-once rule and the goal's "delete
runtime interpretation, pointer chasing, conversions and working-set traffic"
both name.

**Do not use entry-PC call counts on this function.** Its lowest address
executed **50** times against 19.1M instructions in the body -- 383K
instructions a call, which is impossible. It is not entered at its base PC, so
the trick that has been reliable elsewhere silently undercounts by 4,000x here.
Cross-check every entry-PC count against a runtime counter before believing it.

**The slice.** Requirement 4 made the animation VALUES fixed point; it left the
STRUCTURE as a decomp object graph -- a linked `AObj` walk with a
`void *interpolate` per node, refilled by a runtime parse. Bake dense
per-(fighter, motion) fixed-point track records at build time and all three
symbols collapse together: no parse, no list walk, no per-node dispatch, and a
sequential working set instead of >30 KB of pointer chasing against a 4 KB
D-cache. That is the ~70,000 cyc/frame of stall the cycle-116 analysis said any
animation rewrite has to beat, and it is the same ~4.86% these three symbols
cost.

**It has a build-tooling half and must not be started as a runtime-only edit.**
Generic generator, specialized runtime, per `PROJECT_GOAL.md`. Expect it to span
several slices: emit the tracks, prove the emitted data equals the parsed data
offline (the Requirement-4 pattern -- enumerate the enumerable half), then
switch the runtime over behind `gNdsR2AnimCutRoute` so the old and new paths can
be A/B'd on one binary.

**The parse call IS the AObj list walk -- established by arithmetic, not
theory.** Lines 442-464 of `battleship_ftanim.c` clear a 10-entry
`track_aobjs[]` and then walk the DObj's whole `aobj` chain to rebuild it. Each
iteration is roughly ten instructions (load `track`, two range compares, store,
call `ndsR2AnimAObjToQ`, load `next`) over about ten nodes -- **~100
instructions against the 95.7 the profile measures per call.** The walk is the
call. `ndsR2AnimAObjToQ` is NOT the cost: once a node is migrated it early-outs
on `kind >= NDS_R2_AQ_KIND_BASE` in about five instructions.

The nodes are 36 bytes each and reached by pointer, so ~360 scattered bytes a
call against a 4 KB D-cache, 200,231 times a match. That is the working-set
traffic to delete, and `track_aobjs[]` is invariant while the AObj list is
unchanged -- it is rebuilt from scratch every call.

**Two traps for whoever cuts it, both already paid for:**

1. **`NDS_R2_FTANIM_ENSURE()` ALLOCATES on NULL** -- `if (track_aobjs[i] ==
   NULL) track_aobjs[i] = gcAddAObjForDObj(...)`. So a lazy or cached table that
   misses a site does not crash and does not read garbage; it **silently
   allocates a duplicate AObj**. Any lazy build must live INSIDE `ENSURE`,
   before that NULL test, so no site can bypass it. There are **41
   `track_aobjs[i]` reads against only 7 `ENSURE()` calls** -- ENSURE gates a
   case, not a read -- so confirm every read sits in a case that called ENSURE
   before relying on that.
2. **The `event16 == NULL` exit never reads the table.** It calls
   `ndsR2AnimAdvanceTail` and returns, so on that path the entire clear-and-walk
   is dead work. Hoisting that one check above the walk is a pure, provable
   deletion -- but measure how often the path is taken before spending a build
   on it, because a mid-script animation never reaches it.

Do NOT reach for a cross-call cache of `track_aobjs[]`. Cycle 117 lost two
slices to caches in the neighbouring collision code, and the second failure
(29b) is still unexplained. A per-call lazy build inside `ENSURE` has no
cross-call state and therefore none of that risk.

**Judge it against SINT, not WORK-H alone.** Cycle 117 spent three collision
slices proving that a 15,744-tick cut to one bucket does not clear the
+-8,544 WORK-H placement floor on its own. SINT is the animation bucket; its
current gate figures are P50 148,928 / P95 335,552.

### Slice 30: invalidate where the invariant breaks, not where it is read

The fix slice 29's failure asked for. Every write to `gMPCollisionGeometry` --
all seven, in `reloc_backend_mp_collision.c` and `reloc_backend_compat_shims.c`,
**including the restores**, because a restore changes which geometry is current
just as much as the swap did -- now goes through `ndsMPCollisionSetGeometry`,
which assigns and resets the caches together.

That removes the whole class of defect rather than its instances. The vertex
memo could previously be LABELLED "geometry B" by a bind while a caller holding
A's `verts` filled it with A's vertices; now the reset happens at the moment the
data stops being current, so the label cannot disagree with the fill and no
reader can forget the handshake. Slice 28 added two missing binds and slice 29
died adding a third; three sites in two slices was the signal that the
handshake was in the wrong place.

Both files are `#include`d into `reloc_backend.c`, so this needed only a static
setter plus one forward declaration -- no header surface, no cross-TU exposure.
The existing `ndsMPVertexF32Bind` calls were deliberately KEPT: they are
redundant now, but they are free, and deleting them is a second change with its
own failure mode. The point is that correctness no longer depends on them.

**Evidence is the clean run, not the ticks.** 1600-frame gate arm, no repeated
presented frames -- the signal that killed slice 29 twice -- and `WORK-H` P50
+4,096 / P95 -5,632 against slice 28, both inside the +-8,544 floor, which is
what a pure correctness refactor should measure. Boundary green.

**It does NOT unblock the endpoint memo, and that was tested rather than
assumed.** The patch was re-applied on top of this slice with the bind removed
(slice 29b) and **still diverged the match, reproducibly: frames 535, 822, 1110,
1590 on two consecutive runs.** Reverted again. So the bind was NOT the cause --
it is exonerated by experiment -- and the earlier claim in this board and in the
slice-30 commit message that it was is wrong. The two variants even diverge at
DIFFERENT frames (1015/1495/1686 with the bind, 535/822/1110/1590 without),
which says the memo itself is the common cause and the bind merely changed how
the divergence expressed.

**What is still unexplained, for whoever picks this up.** The memo's output
looked provably identical on review: same resolved group, same first/last vertex
ids, `flags` always from the FIRST vertex regardless of ordering, `vertex_count`
unchanged, and the s32 `v_first_x <= v_last_x` ordering replaced by
`NDS_FCMP_GT` on the f32s, which is exact because every s16 is exactly
representable. One of those four claims is false, or the function has a caller
whose behaviour depends on something other than its outputs. **Find which before
writing any more code** -- three of the day's four failures came from acting on a
plausible mechanism instead of a measured one. The reproducible frame set is a
cheap oracle: bisect the memo by disabling parts of it and watch which frames
move.

### Slice 29: REVERTED -- and it found the real defect in the cycle-109 memo

`ndsMPFindLineEndpoints` is 13,205 cyc/frame across 28 callers and is a PURE
function of `line_id` and static geometry. Most of its cost is not the answer
but the SEARCH for it: a nested walk over yakumono x line-kind doing O2R
halfword reads to discover which group owns the id. Memoising the resolved
lookup (two vertex indices, count, flags -- 8 bytes a line, success path only so
neither failure counter is affected) is obviously correct on paper.

**It diverged the match, reproducibly: frames 1015, 1495, 1686 on two
consecutive gate runs.** Reverted; the patch is not lost, but do not re-apply it
as written.

**Why, and this is the part worth keeping.** The memo itself is fine. What broke
it was the `ndsMPVertexF32Bind(geometry)` the function needed in order to read
coordinates through the cycle-109 f32 memo -- and that exposed a **design flaw in
that memo which slice 28 only half-diagnosed.**

The vertex memo is LABELLED by `sNdsMPVertexF32Geometry` at bind time but FILLED
from whatever `verts` pointer each caller passes. Those are not the same thing.
`gMPCollisionGeometry` is saved, swapped and restored around the
alternate-geometry queries, so a bind can label the cache "geometry B" while a
caller still holding A's `verts` fills it with A's vertices. A later reader that
binds B then finds the label already equal to B, skips the reset, and reads A's
coordinates. Adding a bind inside a function reachable from 28 call sites is
what made that interleaving reachable.

**So the fix is not another bind.** Slice 28 added binds to two sweeps and this
slice would have added a third; three sites in two slices is the signal that the
handshake is in the wrong place. **The invalidation belongs at the ASSIGNMENT of
`gMPCollisionGeometry`, not at its readers** -- roughly six sites
(`reloc_backend_mp_collision.c` 9594/9607, 10252/10360, 13527/13564, plus
`reloc_backend_compat_shims.c`), behind a setter that assigns and resets
together. Then no reader can forget, the label can never disagree with the fill
because the reset happens exactly when the invariant breaks, and the per-query
bind check leaves the hot paths entirely. Do that FIRST; the endpoint memo is
cheap and safe once it lands, and so is anything else that wants the vertex
cache.

**Standing rule earned here:** a cache labelled at bind time but filled from a
caller-supplied pointer is only as correct as the discipline of every caller.
Prefer invalidating where the invariant is broken over binding where it is read.

### Slice 28: the whole-line reject the SOURCE has and this port lost

**Mechanism proven, frame-level win NOT proven.** An earlier revision of this
entry claimed it cleared the placement floor. That was read from an artifact
mid-write -- the run's JSON was polled for EXISTENCE rather than waited on --
and the authoritative file says otherwise. Corrected below.

`mpCollisionGetFCCommon` in `decomp/.../mp/mpcollision.c` reads a line's x span
and returns FALSE **before walking any segment**. This port went straight into
the per-segment loop and read four vertex coordinates through
`ndsMPO2RReadU16` plus two memoised f32s per segment *before* testing whether
the object was over that segment at all -- and `func_ovl2_800F8FFC` and the
floor loops call it once per floor line, while an object is over at most one.
Nearly all of that work was spent proving a line irrelevant.

Four sites now reject a whole line in O(1) from a per-line span cache filled
once from static geometry: both `mpCollisionGetFCCommon*` point queries (x
span) and both sweeps (y span). The four s32 vertex reads also sank below the
segment gate, where the values are actually used. The ceiling query, which had
never bound the cycle-109 vertex memo, stopped paying four live `__aeabi_i2f`
per segment.

**Exact, and proven as an implication rather than a verdict.**
`scripts/check_mp_line_extent_reject_exact.py` runs 400,000 cases and, for
every reject that FIRES, replays the skipped loop segment by segment through
the real kernel: **111,961 point rejects and 68,993 sweep rejects covering
295,114 replayed segments, 0 missed hits, 0 missed `saw_flat_ascending_sweep`
flags.** It fails if a reject never fires, so it cannot pass vacuously. X is
deliberately excluded from the sweep reject -- the kernel's flat branch
extrapolates a hit x that can leave the sweep's x span by an amount bounded by
the sweep's aspect ratio, not by epsilon -- and the y half alone is exact for
both the tilt branch's gate and the flat branch's ordering constraint.

**Measured, gate arm, against slice 27:** `WORK-H` P50 972,736 -> 970,112
(**-2,624**), P95 1,317,120 -> 1,310,528 (**-6,592**). **Both are inside the
+-8,544 cross-build placement floor, so the frame-level win is NOT
attributable**, and untouched `FTR` moving +5,952/+6,208 the other way says
placement is what is eating it.

The SUBSYSTEM result is decisive and is the real finding: **`SPHD` -10,752 P50 /
-15,744 P95** -- the physics bucket, exactly what was cut -- with `SRC`
-10,560/-12,160 and `GCRA` -10,368/-12,288 moving with it and `STG` (-896)
flat. So the mechanism works and is worth far more than the frame shows; what
this does not yet demonstrate is that a ~15,700-tick cut to one bucket survives
into WORK-H against a floor almost as large. Stacking the lane's remaining
levers is what would settle that.

**Engagement, from the same run:** the sweep reject skips **91.9%** of line
visits (59,207 rejected / 5,234 admitted) and the point query **60.3%**
(33,003 / 21,751). `gNdsMPLineExtentOverflow` 0, so the fail-closed cap never
fired, and `gNdsR2CubicEvals` 299,148 -- identical to the control.

**It found a latent cycle-109 defect, and the harness nearly hid it.** The
first two gate runs threw `sample-tick-hud-buckets.ps1`'s "repeated a presented
frame (4 of 1600)" guard, which that script itself classes as pacing and offers
`-AllowRepeatedFrames` for. Both runs named **the same four frames** (727, 918,
1302, 1494). Deterministic means the change altered the match, not the pacing:
`ndsStageMPSweepFloorLoopSweep` and `ndsStageMPCeilFloorLoopSweep` read the
cycle-109 vertex memo **without ever calling `ndsMPVertexF32Bind`**. Only the
three point queries did. The vertex-keyed memo tolerated that because
`gMPCollisionGeometry` is saved and restored around the alternate-geometry
queries; a LINE-keyed cache turns the same staleness into a wrong whole-line
reject. Both sweeps now bind, the duplicates are gone, and the eval-count
control matches. **Standing rule: re-run before using a harness override, and
diff which items tripped it -- the same set means your change.**

### Slice 27: an EXACT ordered compare for two runtime floats -- the durable part

**The ticks did not clear the floor; the primitive is the deliverable.**

`nds_fcmp.h` said, since cycle 109, that there was no predicate for two runtime
floats because the key "is more work than it saves at the sites this exists
for". That was a judgement about the sites of the day. The collision lane runs
sixteen runtime-float comparisons per `ndsMPFCSegmentCrossesKernel` call at 54.7
calls a frame, and more in the three symbols beside it, so the sites changed.

`NDS_FCMP_LT`/`GT`/`LE`/`GE` map the IEEE order onto unsigned integer order:
flip the sign bit on non-negatives, invert all bits on negatives, and fold both
zeroes to one key first -- **five register instructions an operand, branchless,
against a 14.2-14.6 cycle `bl` plus call overhead.** Unlike the `_C` forms it is
exact for a NEGATIVE constant and for two runtime values, which is what the
`< -epsilon` tests needed.

**Proven, not argued.** `check_fcmp_exact.py` gained a second pass: a key that
is order-preserving on every ADJACENT pair of the float order, and that ties
exactly where the floats are equal, is order-preserving everywhere -- so one
linear 2^32 walk settles it without enumerating 2^64 pairs. **4,278,190,080
adjacent pairs, four predicates, zero disagreements**, plus an explicit check
that the two zeroes key identically. `check_mp_floor_crossing_exact.py` still
reports 2,332,800 cases with 0 verdict and 0 bit-pattern mismatches after the
conversion.

**Measured, gate arm, against the c117 re-bank:** `WORK-H` P50 973,568 ->
972,736 (**-832**), P95 1,317,440 -> 1,317,120 (**-320**), mean 1,016,526 ->
1,008,694 (**-7,832**). `gNdsR2CubicEvals` 299,148 in both.

**Read the untouched buckets before believing any of that.** `STG` moved
**-5,568** P50 and `FTR` **+1,920** on a change that touches neither. That is
placement, and it is larger than the effect being measured -- so the honest
statement is that slices 26 and 27 together are **not attributable** at the
bucket level, exactly as slice 26 predicted they would not be.

**What this settles for the lane.** The soft-float class is 58,358 ticks/frame
spread over twenty-plus callers; slices 26 and 27 removed roughly a fifth of
ONE caller's share. Deleting float operations one caller at a time cannot move
this frame. The collision lane needs the same treatment animation got in
Requirement 4 -- **the segment arithmetic itself in fixed point, all four hot
symbols at once** (`ndsMPFCSegmentCrossesKernel`, `ndsStageMPAdjustFloorLoopWallSweep`,
`mpCollisionGetFCCommonFloor`, `ndsMPFindLineEndpoints` = 55,320 cyc/frame of
self time plus ~21,300 ticks of float) -- and the new ordered compare is a
prerequisite for that, not a substitute.

### Slice 26: the floor-crossing kernel loses five float multiplies and a divide

**Bit-identical, proven, and BELOW the instrument''s floor. Kept anyway, because
the campaign rule is to bank every repeatable correctness-preserving gain.**

`ndsMPFCSegmentCrossesKernel` is the #1 caller of the `__aeabi_fadd`+`__aeabi_fmul`
class -- 16.2%, 9,476 ticks/frame -- plus 8,415 cyc/frame of self time at
**1.98 cyc/insn**, which says instruction count, not stall. Three deletions, all
exact by construction:

- `side` and `orient` are both exactly +-1.0f. `side * X <= 0` is the OPPOSITE
  zero predicate for a negative side; `orient * sx` IS `fabsf(sx)` (`orient` is
  the sign of `sx`, and `sx == 0` already returned); `side * (orient * raw)` is
  a negation, which GCC emits as one `eor`. **Five of the six float multiplies
  in the tilt block, and three in the flat block, were sign flips written as
  `__aeabi_fmul`.**
- `surface_prev` sank into the single branch that reads it. It costs an
  `__aeabi_fdiv` -- **109.4 cycles a call, the most expensive helper in the
  build** -- and was computed on every call that reached it, including the
  crossing path that never looks at it.

**`scripts/check_mp_floor_crossing_exact.py` compiles the shipped header and the
pre-change float body side by side and sweeps 2,332,800 cases**: 40,417 hits,
**0 verdict mismatches and 0 hit-coordinate BIT-PATTERN mismatches**. Both signed
zeroes, zero-length motion, vertical/horizontal segments and exactly-on-line
positions are in the domain deliberately -- those are where a sign-flip rewrite
would differ if it differed anywhere. This is collision, so it gets no error
budget: the claim is equality and the instrument asserts equality. Wired into
`check-gbi-decode-fixtures.ps1` beside the cubic bound and the sprite-lerp
check, because a checker nothing runs reads like a pass.

**Measured, gate arm, 1600 samples, same build config as the re-bank:**
`WORK-H` P50 973,568 -> 972,032 (**-1,536**), P95 1,317,440 -> 1,316,416
(**-1,024**), mean 1,016,526 -> 1,012,401 (**-4,125**). `gNdsR2CubicEvals`
299,148 in both, so nothing about the simulation moved.

**That is under the +-8,544 `WORK-H` floor and is therefore NOT an attributable
win** -- say so rather than banking -4,125. What the run does settle is a
control worth having: `gNdsR2AObjPoolDeclines` reads **0** with
`gNdsR2AObjPoolCount` **512**, so cycle 109''s contiguous AObj pool is live and
the 25.64 cyc/insn measured on the walk really is capacity.

**The lesson for the next collision slice: one kernel is not enough.** The lane
is ~143,000 cyc/frame across ten symbols; deleting ~10 float ops from the
hottest of them moves the frame by less than the cross-build placement noise.
The remaining collision levers -- an exact integer ordered compare for two
RUNTIME floats (~7 instructions against an 18-cycle `bl`, priced at ~3,300
cyc/frame here and reusable across `ndsStageMPAdjustFloorLoopWallSweep`,
`mpCollisionGetFCCommonFloor` and `ndsMPFindLineEndpoints`), and fixed-point
segment arithmetic -- have to be stacked into ONE arm to clear the floor.
### The animation lane's ARITHMETIC is spent; what is left is working set

Stated with numbers so nobody re-derives it, and stated as a *boundary*, not a
closure: the cheap arithmetic and format levers are gone, the structural one is
a campaign item, and it is sized below.

**Three levers were checked and three answers came back:**

1. **Float: gone.** The 46,856-sample caller census finds `gcPlayDObjAnimJoint`,
   `ndsR2AnimValueQ` and `ndsR2CubicValueFixed` **nowhere** in the
   `__aeabi_fadd`+`__aeabi_fmul` class. What is left is *material* animation
   (4,424 tk/fr) and the parser's per-DObj `anim_wait -= anim_speed` /
   `anim_frame += anim_speed` (2,665) -- and those two are `DObj` `f32` fields
   with `F32_MIN`-derived sentinels read across the whole fighter tree.
2. **Contiguity: ALREADY SPENT, cycle 109.** `battleship_sys_objman.c:92` gives
   `gcSetupObjman` a 512-node contiguous AObj pool through `setup->aobjs`,
   because `aobjs_num` is zero in every scene and the decomp otherwise
   `syTaskmanMalloc`s each 36-byte node individually. The 25.64 cyc/insn on
   `gcPlayDObjAnimJoint`'s `0x02001484` is what remains **after** that pooling,
   so it is **capacity, not scatter** -- and that file's own comment predicted
   it: "Contiguity cannot make it resident either." Do not rebuild this.
3. **The remaining cost is MEMORY STALL, and it is most of the lane.** At an
   ideal ~1.1 cyc/insn the four hot symbols carry roughly **70,000 cyc/frame of
   stall out of 163,370**:

| symbol | cyc/insn | implied stall cyc/frame |
|---|---:|---:|
| `gcPlayAnimAll` | 4.00 | ~12,000 |
| `ftParamUpdateAnimKeys` | 3.41 | ~9,200 |
| `ndsR2FtAnimParseDObjFigatree` | 3.16 | ~24,600 |
| `gcPlayDObjAnimJoint` | 2.61 | ~24,600 |

   The parser's top five PCs alone are **10,559 cyc/frame at 15-35 cyc/insn**,
   about one execution per call each: first-touch misses on the `DObj`, not
   arithmetic. `ftParamUpdateAnimKeys` sweeps ~26 joints per fighter and each
   joint pulls in its `DObj`, its ~3.3 `AObj` nodes and its `MObj` list --
   **well over 30 KB of decomp object graph touched every frame against a 4 KB
   D-cache.**

**So the next animation win is not an arithmetic slice, it is the AOT dense
track rewrite** `FIXEDPOINT_ANIMATION.md` describes: stop touching `DObj`/`AObj`
per frame at all and evaluate a compact per-fighter track array into a dense
pose array. That is a campaign item with a build-tooling half, not a slice, and
it should be scheduled as one. **What it must beat: ~70,000 cyc/frame of stall
(~35,000 ticks).**

**What is NOT worth doing, priced so it is not re-proposed:**

- **ARM-ising `gcPlayDObjAnimJoint` to inline the evaluator.** The evaluator's
  nine-register `push`/`pop` is 6,683 cyc/frame over 280 calls; inlining moves it
  to 106.9 calls and saves ~5,800 cyc/frame (~2,900 ticks) for ~+700 bytes of
  ARM text on a function already at 2.61 cyc/insn. Under the board's own
  "clear ~16,000 in one change" bar, and it risks the I-cache to get there.
- **Shrinking `AObj` from 36 to 24 bytes.** 12.8 KB -> 8.5 KB is still far over
  a 4 KB D-cache; ~3,000 cyc/frame for a decomp struct change with readers in
  `objman`, `mp_collision` and the MObj colour path.

**And the next lane is named, with its own numbers: map collision.**
`ndsMPFCSegmentCrossesKernel` 16.2% and `ndsStageMPAdjustFloorLoopWallSweep`
13.8% of the soft-float class, plus the two `WallCollisionAdjNew` at 4.8% and
`ndsBaseMPProcessUpdateMain` at 1.6% -- **36.6% of `__aeabi_fadd`+`__aeabi_fmul`,
~21,300 ticks/frame** -- on top of **90,132 cyc/frame** of self time. That is
~133,000 cyc/frame (~66,400 ticks), the largest non-renderer lane in the frame,
and it is a FLOAT lane, which is exactly where Requirement 4's technique already
has a proven answer.

### Slice 25 (ARCHITECTURAL): Requirement 4 SHIPS -- the fighter AObj is fixed point

**`WORK-H` P50 -23,360, P95 -37,504; `SINT` P50 -24,896; one binary.**
`build-c116-req4` (`NDS_R2_ANIM_CUT_ROUTE=1`), `gNdsR2AnimCutRoute` poked to
**7** (float AObj) and **15** (Q AObj), same ROM, same melonDS
`DE80E46BDCF1FD98`, 1600 samples an arm, DLDI on, 0 repeated frames on either
arm, route read back 7 and 15 at end of run.

| bucket | A: float AObj | B: Q AObj | delta |
|---|---:|---:|---:|
| **WORK-H P50** | 939,456 | **916,096** | **-23,360** |
| **WORK-H P95** | 1,146,944 | **1,109,440** | **-37,504** |
| WORK-H mean | 949,312 | 923,402 | -25,910 |
| **SINT P50** | 147,776 | **122,880** | **-24,896** |
| SINT P95 | 271,808 | 239,552 | -32,256 |
| SRC P50 | 317,120 | 292,288 | -24,832 |
| GCRA P50 | 312,064 | 287,232 | -24,832 |
| FTR P50 | 301,568 | 301,632 | +64 |
| STG P50 | 188,544 | 188,480 | -64 |
| ALL P50 | 1,118,400 | 1,118,400 | 0 |
| WAIT P50 | 189,056 | 209,408 | +20,352 |

**`SINT` is the attribution.** It is the animation bucket, it moved -24,896, and
`WORK-H` moved -23,360; those agree. `FTR` +64 and `STG` -64 are both the
instrument's 64-tick quantum, so the change is confined to where the code is --
**this does NOT move `FTR`**, exactly as the c115 census predicted (only ~3,085
of the 107,870-tick animation lane is inside the `FTR` bracket). `ALL` P50 is
identical and the saving reappears as `WAIT`, which is `ALL` being
VBlank-quantised, as always.

**Two counters make it a semantic-equivalence control, not just a timing
result.** `gNdsR2CubicEvals` read **285,210 in BOTH arms** and
`gNdsR2FtAnimParseCalls` **200,231 in both**: the route changes how a value is
computed, never how often it is computed or how often the script is parsed.

**What actually changed.** `AObj`'s six `f32` slots now carry Q values for
fighter joints, discriminated by three new `kind` values (5/6/7) on the `u8`
field the evaluator already switches on. No new struct, no parallel array, no
cache: `include/nds/nds_anim_fixed.h` states the formats,
`ndsR2FtAnimParseDObjFigatree` writes them, and `ndsR2AnimValueQ` reads them.
Non-fighter AObjs (material, camera, stage, `mp_collision`'s own Linear writer)
keep the float kinds and the decomp's own expressions bit-identical, which they
must -- `gcPlayDObjAnimJoint` runs for stage and item DObjs too.

Deleted per node per frame: **six inlined `ndsR2F32ToFixed`, one
`ndsR2F32MulToFixed`**, the `__aeabi_fadd` in `aobj->length += speed`, Linear's
`__aeabi_fmul` + `__aeabi_fadd`, and Step's `__aeabi_fcmple`. Deleted per parse
event: 12 `__aeabi_i2f`, the `__aeabi_fdiv` on the Linear rate (109.4 cycles a
call, the most expensive helper in the build), and two `__aeabi_fsub` per
touched track for `-anim_wait - anim_speed` -- which is now hoisted out of the
flag scan as well. **All three kinds now share ONE `bl`** into a single
`target("arm")` kernel, where before Cubic paid one, Linear two and Step one.

**The numerical claim is PROVEN, not argued.**
`scripts/check_r2_cubic_error_bound.py` now drives the parser's arithmetic end
to end and adds three rows:

| row | samples | result |
|---|---:|---|
| `parser-Q-exact` | 393,216 | **0 mismatches** -- `arg << (12-k)` equals `ndsR2F32ToFixed(arg * 2^-k, 12)` for every s16 on all six power-of-two tracks |
| `step-Q-exact` | 66,836 | **0 mismatches** |
| `linear-Q` | 63,896 | max 0.000796 |
| `rotation-Q` / `translation-Q` | 3,990,240 | **0.002842 / 0.006702 -- identical to the float-fed kernel in every printed digit** |

`parser-Q-exact` is the load-bearing one: it says the cubic's four value inputs
did not move **at all**, so the sweeps are bounding `t` and `length` alone --
and those come out the same as before. The frac tables are read out of
`decomp/.../ftanim.c` and `battleship_ftanim.c` by the checker rather than
retyped, so the shipped table cannot drift away from the one proven against.

**A latent wrap was found and closed on the way, in BOTH kernels.**
`gNdsR2CubicSaturations` read **337 a match** on the float arm. Mechanism: the
parser only overwrites `length_invert` when the payload is non-zero, so a Cubic
event with a zero payload can inherit a **Step frame count** in that field;
`t = length * length_invert` is then enormous, the conversion clamps it to
`0x7fffffff`, and `t*t` wraps an s32 anyway -- a joint teleport, and pre-existing
since E64. `ndsR2AnimClamp` bounds `t` to +-2 and `length` to 1024 frames, which
is what actually bounds the chain; the shipped build now reports **44** clamps a
match, so it fires, and the host bound is **unchanged in every domain**, so it
never fires inside the realistic one.

**Banked on the shipped build** (route compiled out, `make p1`): `WORK-H` P50
**920,192**, P95 **1,113,408**, mean 928,548; `SINT` P50 127,424; `FTR` P50
299,008. Against the previous banking (942,976 / 1,147,072) that is **-22,784
P50 and -33,664 P95**, consistent with the one-binary delta. Boundary passes.
1 of 1600 samples repeated a presented frame, which the sampler flags as not
pacing-comparable -- the A/B arms had none, which is why the A/B is the headline
and this is the bank.

### The `.data` route WORKS — first attributable animation measurement (cycle 109)

Built the standing-rule-7 route the determinism finding demanded.
`gNdsR2AnimCutRoute` (`src/import/battleship_sys_objanim.c`, `.data`,
`aligned(32)` so no neighbouring counter's write-back can stamp the poke): bit 0
the loop-invariant hoist, bit 1 the fused `length * length_invert`. Default 3 is
shipped behaviour, so an unpoked ROM is unaffected. ROM
`builds/build-c109-route2`, `NDS_R2_BOTH_CPU=1`, DLDI on, 1600 samples/arm.

**Provenance — every control holds.** `romSha256` **identical across both arms**
(`5CE68200B1831473…`), same melonDS hash, same sample count. The route read `3`
and `0` respectively in `-ExtraGlobals` at *end of run*, so the poke landed and
was never stamped back. `gNdsR2CubicEvals` was **292,857 in both arms** — the
route changes how a value is computed, never how often, so identical eval counts
are a semantic-equivalence control, not a coincidence.

| bucket | cuts on | pre-cut | delta |
|---|---:|---:|---:|
| **WORK-H mean** | 1,128,367 | 1,132,109 | **−3,742** |
| WORK-H P50 | 1,101,824 | 1,102,528 | −704 |
| WORK-H P95 | 1,550,400 | 1,572,352 | −21,952 |
| **SRC mean** | 387,316 | 391,204 | **−3,888** |
| SRC P50 | 368,768 | 369,728 | −960 |
| OTHR P50 | 203,712 | 197,888 | **+5,824** |

**The two cuts are worth ≈3,700–3,900 ticks/frame of mean `WORK-H`.** The
attribution is what makes this the result: `SRC` is the bucket the changed code
lives in, and its mean moved −3,888 against `WORK-H`'s −3,742. Those agree.
Rough corroboration from the cycles model — the hoist's 1,955,955 predicted
cycles plus ~8 cycles saved on each of 292,857 cubic evals — lands inside 1.4x of
the measured figure, which for a cycles estimate is agreement, not precision.

**Read the mean, not the percentiles, and here is why.** P50 −704 is real but
near the instrument's 64-tick quantum. **P95 −21,952 is NOT attributable to these
cuts** — P95 sits at 1.55M, far over the gate, and the whole-match tail is effect
DObj submits, not animation; two arithmetic cuts in the joint loop cannot move it
by 22K. `OTHR` P50 moved **the wrong way** (+5,824) and nothing in these cuts can
touch `OTHR`. Both are the honest residue: **a one-binary route removes the
*placement* term because addresses are identical, but not the *cache-occupancy*
term, because the two arms still execute different instruction bytes.** The mean
is the statistic to read — it uses all 1,599 rows rather than one order
statistic.

**What this settles.** The cut is **3.8x smaller than the 14,080 cross-build
placement term that was hiding it.** That vindicates the route and retroactively
explains why the 7-cut batch's −32,128 P95 was uninterpretable. Any future cut in
the 1,000–5,000 tick class must be measured this way; there is no other method.

### The FTR pre-submission half is enumerated: four of five seams already elided

Walked `FTR_STG_OPTIMIZATION.md`'s FTR ask seam by seam. Every one now has a
number, and the plan's "delete working-set traversal and policy work" is
**already done** — which is why its own note says the baked plan "only saved
roughly 6-9K".

| seam | status | evidence |
|---|---|---|
| walk | **baked plan landed** | `native_owner_plan_hit` takes `ndsFighterDrawPlanApply`; 0 hash variants over 3,961 comparisons |
| validate | **99.95% cached** | 3,961 reuse / 2 build (cycle 98) |
| reset | **dead at shipped profile** | both call sites are in the `detailed_output` arm |
| resolve + its lookups | **already elided** | it is the `else` of the plan-hit branch, keyed on the 99.95% validator |
| matrix prep | **the only live seam** | see below |

**The resolve check is worth keeping as a method note.** I was about to memoise
`ndsFighterDrawPlanResolve` on the walk hash. Two things stopped it: the hash does
**not** cover four inputs the resolve reads (`expected_asset_id`, the display
contract's `event->dl`/`material_dobj`/`matrix_dobj`, the MObj chain length, and
the loaded file's own fields), so that key would have been unsound; and the memo
measurement independently proved the resolve is not hot — 30,385
`ndsRelocFindLoadedFileContaining` calls for the WHOLE match across all 30
callers, when resolve alone at ~10 selected DObjs per fighter per frame would
exceed 40,000. **A memo whose key is narrower than its function's inputs bakes a
stale plan silently**; that is the Task 36 lesson in a new place.

### The one live FTR seam is named: `ndsRendererAdapterBuildDObjLocalMatrix`

**62 `bl __aeabi_*` sites in one per-joint-per-frame function** — 36 `fmul`, 10
`fadd`, 7 `fcmpeq`, **6 `fdiv`**, 3 `fsub` — against 1,362 instructions. That is
more soft-float sites than the figatree parser carried, and this is the plan's
"compact matrix preparation" box.

**But its interior is already fixed point, so do not brief this as a rewrite.**
It declares `s32 translate[4]`, `s32 scale_x/scale_y`, reads rotations through
`ndsFloatBits` (a bit read, not a conversion), and its own comment says it works
in fixed point off the sin/cos table. The soft float is concentrated at its
**f32 boundary**, specifically the MVP-recalc scale path
(`sNdsRendererAdapterMvpRecalcScaleX * dobj->scale.vec.f.x` and `.y`, plus
`cobj->projection.persp.scale`) — and that path is gated on
`has_mvp_recalc_rpy_0x47`.

**So the next step is ONE COUNTER, not an edit,** and this is the third time this
cycle that rule has paid: **62 static call sites is not 62 executions.** Count how
often `has_mvp_recalc_rpy_0x47` is true and how many of the 6 `fdiv` are on it
before touching anything. Tonight the same shortcut produced a 6x error on the
material-lookup seam and, earlier in the campaign, killed the animation lever at
1.64x its target. If the recalc path is cold, this function is not a lever
either and the FTR half is fully closed; if it is hot, the fix is the same exact
converter technique the parser slice just proved, not a representation change.

**This is also where the two plans converge.** `FTR_STG_OPTIMIZATION.md` line 49
says the fixed-point animation representation feeds directly into this box, and
`FIXEDPOINT_ANIMATION.md` deliberately kept one float boundary at DObj in stage 1
and routes "fixed pose -> fixed local matrix" to stage 2 **gated on profiling
justifying it**. The 62-site count is the beginning of that justification, not the
end of it — the counter is.

### The matrix group is 20x machinery, not redundancy — the replacement is a one-pass baked compose

**The deduction that sets the design.** `ndsRendererAdapterBuildDObjWorldMatrixUncached`
walks each joint to the root and rebuilds **every ancestor's** local matrix, which
is O(n*d). That looks like the lever, and it is not: the local builder runs
**101,569 times a match = ~50 a frame**, and there are ~50 bound joints a frame
(25 x 2 fighters). **Each joint's local matrix is therefore built exactly once** —
the linear-probed world cache already collapses the ancestor rebuild. Do not
re-attack the walk.

So the 84,051 ticks/frame is not redundant work. At ~50 joints it is **~1,680
ticks per joint**, against a 4x3 fixed-point compose whose arithmetic is ~36
multiplies, on the order of **80 cycles**. That is a **20x machinery-to-math
ratio**, and it is the quantified form of the plan's thesis: the expensive part is
the architecture around the arithmetic, not the arithmetic.

**Where the 1,680 goes** (six symbols, per frame): `BuildDObjLocalMatrix` 18,290 ·
`LoadHardwareSplitMatrices` 15,218 · `MtxMulAffine20p12` 14,939 ·
`BuildDObjWorldMatrix` 13,962 · `BuildFighterTraRotRpyDirect20p12` 11,868 ·
`MtxMul20p12` 9,773. Per joint that is a cache probe, a chain walk, a `DObj` field
gather, two out-of-line matrix calls and a copy — to produce 12 words.

**The replacement, and it needs no new generator.** The IR already carries
`sNdsNativeMarioBindingParents[14]` / `sNdsNativeFoxBindingParents[18]`,
`BindingJoints`, and `JointSchedule`. Because `BindingParents` is a parent INDEX
into the same binding array, a single pass in baked topological order can compose
every world matrix into one contiguous `NDSRendererMatrix20p12[18]`:

    for i in 0..count-1:                      # baked topological order
        local  = pose_to_local_20p12(i)       # from the fixed pose
        parent = BindingParents[i]
        world[i] = (parent == ROOT) ? local : mul_affine(local, world[parent])

That deletes, per joint: the cache probe, the chain walk, the ancestor loop, and
one level of call indirection — keeping exactly one `mul_affine` and one local
build, which is the irreducible arithmetic. It is **not** a patch table and **not**
per-frame patching, so it does not repeat the +124K FIFO-template mistake; it
deletes traversal and policy, which is what the plan asks for.

**Why this is where the two plans meet.** `pose_to_local_20p12` is the seam
`FIXEDPOINT_ANIMATION.md` stage 2 feeds: today the pose arrives as `DObj` f32
rotate/translate/scale and the local build converts it, so an AOT fixed track
writing a Q12 pose removes the conversion *inside* the same loop this design
introduces. Land the compose first (pure algorithm, same inputs, verifiable by
comparing matrices against the existing path), then swap the pose source.

**Verification available before any measurement:** the new pass must produce
matrices bit-identical to `BuildDObjWorldMatrix` for every bound joint. That is a
direct A/B inside one build — compute both, compare 12 words, count mismatches —
and it is fail-closed: any mismatch falls back to the existing path. The
`gNdsR2AnimCutRoute` pattern already in the tree is the shape for it.

### Both architectures, quantified: 307K against a ~290K gap

With the conversion fixed (`%tot x 1.2378 -> x frame budget`, idle removed first),
the two re-opened architectures size up as follows. This is the accounting that
was missing every time I called a lane closed.

| lane | ticks/frame | composition |
|---|---:|---|
| **FTR fighter draw** | **230,930** | matrix 84,051 · prepare 61,432 · emit 43,282 · replay 30,577 · material 11,588 |
| **animation** | **75,953** | cubic 22,339 · play 19,128 · parse 18,709 · invalidate 15,777 |
| **combined** | **~307,000** | vs a ~290,000 gap from `WORK-H` P95 to the gate |

**Independent agreement, worth more than either number alone:** the animation
figure derived from census percentages is **75,953**, and the board's over-gate
split derived **72,638 cycles/region** for the animation class by a completely
different route. Within 5%. That cross-check is what makes the corrected
conversion trustworthy rather than merely arithmetic.

**So the two plans are right and my refutations were sizing errors.** Together
these lanes are the whole gap. Neither is a micro-optimisation lane; both are
architecture, exactly as `FTR_STG_OPTIMIZATION.md` and `FIXEDPOINT_ANIMATION.md`
say.

**One more retraction rides on this.** `ndsFTParamsInvalidateFighterParts` was
retired at "~6,560 ticks/frame"; corrected it is **15,777**. The *mechanism*
refutation stands — the dead `FTParts` pool cannot reach loads that are `DObj`
fields — but the size was under-read like everything else on this lane, and at
15,777 a preorder-flattened subtree sweep is worth revisiting on its own.

**The AOT half of FTR already exists and is not the problem.**
`src/nds/nds_native_fighter_owner.generated.inc` is **408 KB** of build-time IR:
`sNdsNativeFighterDenseVertices[541]`, `PackedCorners[1878]`,
`RunFirstCorner[67]`, `sNdsNativeMarioJointSchedule[25]`,
`sNdsNativeMarioBindingParents/Joints[14]`, and `sNdsNativeMarioFifoWords[4034]`,
emitted by `scripts/fighters/generate_nds_native_owners.py` (3,187 lines) through
`build.ps1`'s `generate-native-fighters`. **So "fighter asset at build time" is
done.** The 230,930 is what the runtime still does *on top of* that IR — which
means the replacement target is the runtime owner path, not a new generator.

**Where the 84,051 matrix group actually goes** (six symbols, ~50 builder calls a
frame): `BuildDObjLocalMatrix` 18,290 · `LoadHardwareSplitMatrices` 15,218 ·
`MtxMulAffine20p12` 14,939 · `BuildDObjWorldMatrix` 13,962 ·
`BuildFighterTraRotRpyDirect20p12` 11,868 · `MtxMul20p12` 9,773. That is a
per-joint local-build then compose-to-world then load-to-hardware chain walked off
the live `DObj` tree every frame — while `BindingParents` and `JointSchedule` are
already baked in the IR above. **This is the single largest addressable group in
the milestone and it is where the two plans meet:** a fixed-point pose feeding a
baked parent-chain compose deletes both the float boundary and the tree walk.

### RETRACTION: FTR is NOT closed. I mixed two instruments and under-read it 2.36x

The owner re-opened FTR as an architectural replacement and told me to stop trying
to prove it closed by measuring individual helpers. That was right, and the reason
is a defect in my own arithmetic, not a difference of judgement.

**The error.** I converted census cycles to ticks/frame by dividing by the
tick-HUD's 2,038 presented frames. Those are **different instruments on different
builds**. The census's own section E prints control frames at **2,240,292
cycles/frame** and over-gate frames at 3,266,336 — roughly 2x the shipped
`WORK-H` of ~1,128,000 — because the profiled build carries profiling overhead.
Census absolute cycles per frame are therefore NOT the shipped frame cost, and
dividing them by a tick-HUD frame count is meaningless.

**The sound bridge is percentage.** `armWaitForIrq` is **19.21%** of the census
total, so non-idle is **978,488,987** of 1,211,130,791 and
`%non-idle = %tot x 1.2378`. Applied to a 1,128,000-tick frame:

| symbol | %tot | %non-idle | **ticks/frame** | group |
|---|---:|---:|---:|---|
| `ndsRendererNativeEmitProductionRawUntexturedRun` | 2.27 | 2.81 | **31,693** | emit |
| `ndsRendererExecuteNativeFighterOwnerProduction` | 2.19 | 2.71 | **30,577** | replay |
| `ndsFighterMarioFoxDLAllDrawForSlot.constprop.0` | 2.14 | 2.65 | **29,878** | prepare |
| `ndsRendererNativePrepareProductionRun` | 1.52 | 1.88 | **21,222** | prepare |
| `ndsRendererAdapterBuildDObjLocalMatrix` | 1.31 | 1.62 | **18,290** | matrix |
| `ndsRendererLoadHardwareSplitMatrices` | 1.09 | 1.35 | 15,218 | matrix |
| `ndsRendererMtxMulAffine20p12` | 1.07 | 1.32 | 14,939 | matrix |
| `ndsRendererAdapterBuildDObjWorldMatrix` | 1.00 | 1.24 | 13,962 | matrix |
| `ndsRendererAdapterBuildFighterTraRotRpyDirect20p12` | 0.85 | 1.05 | 11,868 | matrix |
| `ndsRendererNativeEmitProductionRawTexturedRun` | 0.83 | 1.03 | 11,588 | emit |
| `ndsRendererAdapterBuildNativeMaterialSnapshot` | 0.83 | 1.03 | 11,588 | material |
| `ftDisplayMainDrawDefault` | 0.74 | 0.92 | 10,332 | prepare |
| `ndsRendererMtxMul20p12` | 0.70 | 0.87 | 9,773 | matrix |

**Total 230,930 ticks/frame = 59% of the ~390K `FTR` bucket**, before any share of
the leaf helpers (`__aeabi_fadd` 2.79%, `fmul` 1.81%, `memset` 1.69%, `memcpy`
1.47%, `fdiv` 0.84%) is attributed to fighter draw. The remaining ~100K to reach
331K is those leaves plus the long tail — the bucket and the symbols now
reconcile, which they did not when I claimed closure.

By group: **matrix 84,051** (six symbols) · prepare 61,432 · emit 43,282 ·
replay 30,577 · material 11,588.

**What this retracts, specifically.** Every "not a lever" verdict I wrote on this
lane was computed the wrong way and is **2.36x low**:

| I said | actually |
|---|---:|
| local matrix 7,766/frame inclusive | **18,290** |
| material snapshot ~4,960/frame | **11,588** |
| "combined addressable seam ~12,700" | **230,930** |

So `matrix` alone is **84,051 ticks/frame**, six times the placement term, and the
individual symbols are 10K-32K each rather than the sub-noise figures I reported.
The plan's "tens of thousands to >100K" projection is supported by the census; my
refutation of it was an artifact.

**The method rule this establishes, because it cost a whole cycle's conclusions:**
**never divide a census cycle count by a tick-HUD frame count.** Convert through
`%tot -> %non-idle -> x frame budget`, and take the idle share out first
(`armWaitForIrq` is a fifth of the profile). The census's own per-frame numbers
describe the PROFILED build, which runs at roughly half the shipped build's speed.
This sits alongside `whole-match-instrument-only` as an instrument-boundary rule.

**Consequence for the work:** matrix + prepare is **145,483 ticks/frame** of
preparation, which is what an owner-path replacement targets, and `replay`
(`ndsRendererExecuteNativeFighterOwnerProduction`, 30,577) plus `emit` (43,282)
are the machinery behind it. FTR is re-opened as an architectural task with a
quantified target, not a closed lane.

### The free step is taken: the last live FTR seam is 7,766 ticks/frame, not a lever

Took the per-PC/census attribution (no build, no run) on
`ndsRendererAdapterBuildDObjLocalMatrix`, joined against the 101,569 calls the arm
counter measured. That closes the FTR pre-submission half with a number.

| quantity | value |
|---|---:|
| inclusive cycles, whole match | **15,826,891 (1.31% of non-idle)** |
| calls (runtime counter) | 101,569 |
| **cycles per call** | **155.8** |
| self cycles | 3,067,306 (30.2/call) |
| **inclusive per presented frame** | **7,766** |
| **self per presented frame** | **1,505** |
| cyc/insn | 2.65 (non-idle is 2.85 — NOT a stall outlier) |
| text | 2,980 bytes |

**Verdict: not a lever.** Deleting this function's entire self time buys ~1,505
ticks/frame, under the placement term; even its whole inclusive cost is 7,766
against a ~290,000 gap. The 62 static soft-float sites that made it look like the
parser's twin resolve to **155.8 cycles a call**, and its fallback arm — which is
where the 36 `fmul` looked like they lived — executes **zero** times.

**So the FTR half of `FTR_STG_OPTIMIZATION.md` is closed, seam by seam, each with a
number:** walk baked, validate 99.95% cached, reset dead, resolve elided, material
lookups 30,385/match, local matrix 7,766/frame inclusive. That is consistent with
`FTR` separating the over/under-gate populations by only **+13,768** and with the
plan's own closing lines, which scope FTR/STG as permanent headroom rather than
the P95 lever. The plan's architecture (build-time draw program → immutable
topology → fixed pose → patch dynamic → direct GX) is not refuted; what is
refuted is that any *remaining individual seam* in it pays measurably. It is a
multi-cycle rewrite whose payoff is the sum, and the plan warns the last attempt
at that shape regressed **+124K**.

**METHOD CAUTION, and I nearly shipped it wrong.** The census's third numeric
column is **bytes**, not call count. Reading it as calls made this symbol look
like 2,980 invocations against the counter's 101,569 — a fake 34x discrepancy that
would have "invalidated" the census for sizing. Read `census.txt`'s own header
(`pack nonmem-stall cycles bytes cyc/insn stall/byte symbol`) before quoting any
column from it. The two instruments agree perfectly once the column is right, and
their agreement is what makes 155.8 cycles/call trustworthy: an inclusive cycle
total from the profiler divided by a call count from a counter in the executed arm.

### The counter answered it: the local-matrix fallback is DEAD, 0 of 101,569

Counted the arms of `ndsRendererAdapterBuildDObjLocalMatrix` before editing it,
and the answer refutes the hypothesis in the row above.

| arm | count | verdict |
|---|---:|---|
| xobj matrices (`valid != FALSE`) | **101,528** | the live path, ~50 calls/frame |
| **fallback** (`BuildDObjFallbackMtx` + `MtxFromN64`) | **0** | **never executes on the gate arm** |
| mvp-recalc identity | 41 | negligible |

**My guess was wrong, and cheaply.** I predicted the 36 `fmul` were
`ndsRendererAdapterMtxFromN64` converting a float `Mtx` to 20.12 — the classic
fixed-pose boundary. That arm runs **zero** times in a 60-second both-CPU match,
so whatever float it contributes to the symbol's 62 `bl __aeabi_*` is dead code
the linker kept. **62 static sites overstates the executed cost by an unknown
margin, and that is exactly what a static count cannot tell you.** Fourth time
this cycle that counting beat inferring; the cost was one build instead of a
rewrite of a matrix path that feeds every fighter's world transform.

**What executes is the xobj path** — `ndsRendererAdapterBuildDObjXObjMatrix` and
`ndsRendererAdapterMulInto`, both inlined into the one symbol, so they cannot be
attributed by symbol. `BuildDObjFallbackMtx` is the only callee that survived
separately, at 26 instructions and **0** soft-float calls.

**The next step is FREE and needs no build or run.** Per-PC attribution restricted
to this symbol off the existing profile CSV (`--split-by-symbol` plus a per-PC
join) will say which of the 62 sites execute and at what cyc/ex, the same way the
33.1-cyc/ex load in the parser was found. Do that before writing anything: if the
executed float is a handful of runtime-float multiplies in the xobj compose, the
exact-converter technique the parser slice proved applies directly; if it is
dominated by dead-but-linked `MtxFromN64`, the FTR half is closed and the
remaining pre-submission cost is the material snapshot's own derivation.

**Calls per frame is the number to size any fix against:** 101,569 / 2,038 ≈
**49.8 builder calls per presented frame**, consistent with ~25 joints x 2
fighters. A cut of one soft-float call per call is worth ~50 helper invocations a
frame — about 1,250 cycles at fmul's 25.17 — so a fix here needs to delete
several per call to clear the placement term.

### FTR pre-submission: the asset lookup is NOT the seam — my premise was 6x wrong

`FTR_STG_OPTIMIZATION.md` asks for traversal and policy work to be deleted from
the ~172K pre-submission half. Cycle 98 had already refuted validate and reset
and named **material prep** (99.948% byte-identical re-derivation). Its stated
mechanism was `ndsRendererAdapterValidateNativeOwnerMaterials` doing **three
`ndsRelocFindLoadedFileContaining` searches per material per fighter per frame**,
and that function's memo is **one entry deep** (`last_loaded_file_index`) while
the three pointers it is asked for — `palette_image`, `block_image`,
`current_image` — are three interleaved streams. That predicts near-total memo
thrash. Widened it to four-way move-to-front and counted.

**The counters refuted the premise.** Whole match, both-CPU, gate arm:

| counter | value | reading |
|---|---:|---|
| total calls | **30,385** | **~15 per frame**, not the ~178,000/match the material call count implied |
| `Way0` | 25,434 | **83.7% — the one-entry memo was already catching most of it** |
| ways 1–3 | **4,385** | 14.4%: the scans this change actually deletes |
| reached the linear scan | 1,014 | 3.3% |

**Why the estimate was 6x off:** `ValidateNativeOwnerMaterials` sits *behind* the
owner validate cache, which cycle 98 measured at **3,961 reuse / 2 build**. Its
three searches per material are therefore already elided ~999 times in 1,000, so
they never became per-frame volume. Reading a per-call cost off an inner
function's call count without checking the cache in front of it is what produced
a 6x error — the same shape as the self-time lesson.

**Verdict: KEEP, banked, not a lever.** 4,385 deleted scans ≈ **500
ticks/frame** — real and repeatable but far under this instrument's resolution.
It is free in footprint (**binary byte-identical**, 160 bytes of headroom, the
function stayed out-of-line at 276 bytes rather than inlining into all 30
callers), and the standing rule is to keep every repeatable
correctness-preserving gain and accumulate it. The counters stay so the seam
never has to be guessed at again.

**Equivalence was verified, not assumed,** and the non-obvious half is the
boundary: `ndsRelocPointerRangeInLoadedFile` accepts `addr == base + data_size`
(it tests `>`, not `>=`), so two adjacent allocations would both appear to hold a
pointer on the seam — except the inner `ndsRelocRangeInLoadedFile` rejects
`size > data_size - offset`, and there the remainder is 0 while every caller
passes size >= 1. At most one file can match, so "first match in scan order" and
"the matching way" name the same file.

**Do not read this run's WORK-H against the parser ROM's.** It is a different
binary (mean 1,121,957 vs 1,133,754, VBI-2 1136 vs 1086), so placement dominates
and none of that difference is attributable to this change.

**What is left of the plan's FTR half is architectural.** With validate, reset and
now the lookup underneath material prep all refuted, the remaining
pre-submission cost is `PrepareNativeOwnerHierarchy`/`…Matrices` — which the
cycle-97 table already classed as *genuinely varying*, not deletable — plus the
material snapshot's own derivation, whose 25 real variants are frac texture
animation. That is the native fighter draw program the plan describes, and the
plan's own warning applies: the previous attempt at the FIFO-template mechanism
regressed **+124K**. It is a multi-cycle project with no one-flag step, and `FTR`
separates the over/under-gate populations by only **+13,768**, so it is headroom
work rather than gate work — which the plan itself says in its closing lines.

### Parser slice LANDED and measured: WORK-H mean −6,805, 20 frames to 30 FPS

`ftAnimParseDObjFigatree` is replaced port-side (`src/import/battleship_ftanim.c`,
selected by `reloc_backend_compat_shims.c`). **45 `bl __aeabi_*` in one function
became 21**, and 4 of those 21 are out-of-range fallback arms that measured zero
executions. Gone entirely: all 12 `i2f`, all 6 `fmul`, all 5 `fcmpeq`, both
ordered compares. `battleship_ftAnimParseDObjFigatree` and
`battleship_ftAnimGetTargetValue` are **absent from the linked ELF** — the linker
drops them, so this replaces rather than coexists and the ROM grew **1,024 bytes,
exactly the reciprocal table**. Boot headroom re-checked: 29,952 proven, green.

**Measured on ONE binary (route bit 2), identical `romSha256` both arms:**

| | parser ON | parser OFF | delta |
|---|---:|---:|---:|
| **WORK-H mean** | 1,133,754 | 1,140,559 | **−6,805** |
| **WORK-H P50** | 1,099,008 | 1,104,704 | **−5,696** |
| SRC mean | 387,487 | 391,970 | −4,483 |
| WORK-H P95 | 1,568,896 | 1,568,896 | 0 |

**Pacing, from the VBlank histogram — the honest statement:** 2-VBI 1066 → 1086,
3-VBI 896 → 868. **20 frames moved from 20 FPS to 30 FPS**, and the tail got
slightly worse (4-VBI 53 → 61, 5+ 22 → 23). Net +1 frame total, which reconciles.

**Do not quote `ALL` P50 from this run.** It reads **−559,040**, which is one
VBlank period (560,190) almost exactly: the median sampled row straddles the
quantisation boundary, landing at 2 VBI in one arm and 3 in the other. `ALL` is
VBlank-quantised and this is the trap it is known for. The histogram above is
what the pacing claim rests on.

**Correctness checked four ways, not asserted.** `check_ftanim_target_exact.py`
proves the s16 conversion **bit-identical over all 65,536 × 8 inputs** (six of
the eight fracs are powers of two and an s16 always fits f32's mantissa, so it is
an exponent subtraction with no rounding step). `check_ftanim_transcribe.py`
inverse-substitutes the port body and compares token streams — **1964 tokens,
identical** — which is the check that mattered, because `AObjAnimAdvance` is
`p++`, used more than once per expression and advancing **conditionally** on a
toggle bit, so a miscount desynchronises the stream and moves hitboxes. It is
**mutation-tested 3 for 3**, including a dropped advance. The reciprocal table is
compile-time-folded and verified bit-identical to a runtime `1.0f/n` against a
`volatile` divide. And at runtime `gNdsR2CubicEvals` is **292,857 with either
parser**, in both arms of both A/Bs: the same commands set the same kinds the same
number of times.

**A checker that only says GREEN is a rubber stamp.** The transcribe checker's
first draft whitelisted a bare `break ;` token pair to excuse a dead `#else` arm
— which would have deleted **all fourteen of the parser's real breaks** and
passed anything. It evaluates the preprocessor instead. Mutation-test any
equivalence checker before believing its GREEN.

**Engagement:** 210,948 parser calls, 56,148 reciprocal hits, **0 misses** —
every payload in a 60-second both-CPU match is under 256, so the 256-entry table
is empirically right and no fallback divide ever ran. The off arm reads 156 calls
rather than 0 because `-SetGlobals` pokes at the first frame-complete marker, so
0.07% of the match runs the default route before the poke lands. Expect that
floor on every route arm; it is not leakage.

**One divide is deliberately KEPT** at `ftanim.c:205`,
`(value_target - value_base) / payload`. `x/n` and `x*(1/n)` round differently and
that feeds a `rate_base` the cubic amplifies by `length`; Linear is 1.7% of nodes,
so the trade would have been fidelity for nothing measurable.

### Parser slice: the mechanism was a PORT REWRITE, not a patch extension

Sized the parser's remaining soft float from the cycle-106 profile: `fdiv`
**1,494,619** at 109.4 cycles a call (the most expensive helper in the build by
3x) on `1.0F / payload` at exactly **two sites**, `ftanim.c:170` and `:244`;
`fsub` 1,753,743; `fadd` 1,454,187; `fcmpeq` 921,383; `fmul` 920,213; `i2f`
796,253; `fcmpgt` 385,779; `fcmple` 204,854. **≈7.9M cycles ≈ 9,000
ticks/frame** — the densest remaining block in the animation lane.

Two facts make the `fdiv` free of numerical risk: `relocdata_types.h` documents
the payload as **a u16 following the command word**, so a reciprocal table is
indexed by a small integer and hits every time; and this build passes **no
`-ffast-math`**, so a compile-time `1.0f/n` initializer is correctly rounded and
therefore **bit-identical** to the runtime divide. The six `payload` assignments'
u16→f32 conversions are exact through the already-proven `ndsR2S32ToF32Bits`
(a u16 has ≤16 significant bits, so no rounding occurs at all) — reuse, not a
second converter.

**But the mechanism I reached for is closed.** `src/import/battleship_ftanim.c`
`#define`s the parser's name, which renames its **definition and its call sites
together**, so no macro can redirect only the calls. Extending the legacy
`scripts/import-overlays/battleship/src_ft_ftanim.patch` would still move the
wrong way: that overlay exists only to preserve older DS adaptations while
`decomp/` remains byte-for-byte pristine, and the standing direction is to
migrate those adaptations into owned import/port seams rather than deepen it.

**The slice was a port-side replacement of `ftAnimParseDObjFigatree`**, selected
by `src/port/reloc_backend_compat_shims.c:1545`, which already defined that symbol
and merely forwarded. That satisfies "replace, don't coexist" and *retires* a
decomp-patch dependency instead of deepening one. **Written, measured and landed —
see the section above.**

**Correction to a recorded blocker:** the note that "the parser has no textual
override hook" was wrong. The shim at `:1545` is the hook; what the macro blocks
is redirecting the *helper* `ftAnimGetTargetValue`, not the parser itself.

## Red Queue

**R0 — CLOSED GREEN 2026-08-14. The Boundary red was a corrupt local DLDI SD
image, not a commit; the five-commit bisect window is REFUTED by measurement.**
Evidence: `artifacts/verification/2026-08-14_boundary-red/BOUNDARY_RED.md`
(log `boundary-after-dldi-reset.log` — `Boundary verification profile passed.`,
exit 0, zero `Exception:`, marker capture **27.8 s of the 120 s ceiling**).

- **No source byte changed.** `emulators/melonds/dldi.bin` (gitignored,
  536,870,912 B) left the ROM loading nothing — `sNdsRelocLoadedFileCount 0`,
  `gNdsRelocAssetPayloadReadCount 0` — so `gMPCollisionGroundData->wallpaper`
  stayed the raw token `0x3eb`, and `lbCommonMakeSObjForGObj`'s first access
  (`sprite->bmsiz`, address `0x41c`) took a data abort inside
  `grWallpaperMakeCommon`, reached from `ndsBaseSCVSBattleStartBattle+0x5a`.
- **The aftermath is why it read as a timeout.** Calico's `__excpt_entry`
  disables the protection unit and `blx`es the junk handler slot `0x205`, so
  the ARM9 slides through zeroed RAM in ABORT mode forever
  (`cpsr=0x400000b7`, `r12=0x205`, `lr_usr=lbCommonMakeSpriteGObj+26`,
  `spsr_abt==cpsr`). Reproduced four times.
- **Three arms, one variable:** 512 MB image → ABORT; DLDI **off** → SYSTEM
  (diagnosis only — DLDI-on is retail parity and ≈29,696 P95 ticks); fresh
  melonDS-created 16,957,440 B image, DLDI on → SYSTEM and Boundary green.
  Old image preserved at `emulators/melonds/dldi.bin.broken-2026-08-14`.
- **Window refuted, not argued.** `build-c-collfixed` (08-13 19:25) and
  `build-c156-vfxsymmetry` (08-14 10:44) — both *before* the window — abort
  with identical registers. Eight of the eleven commits ship no byte anyway:
  `54d7d7862e4` is comment-only, and `813207773c1` touches a checker the
  Makefile's 16-entry script list does not contain.
- **Landed so it cannot cost a fourth cycle:** `scripts/lib/gdb-markers.ps1`
  classifies a timeout from gdb's own attach line — `in ?? ()` means the guest
  had already crashed, so the ceiling is not the lever. Proven on both real
  captures from this cycle. **A second attach is not available: melonDS's stub
  refuses every reconnection after the first session ends** (measured twice) —
  sample the PC with a *first* attach instead.
- **Superseded:** the "REGRESSION, not a ceiling" framing below. Not a ceiling
  was right and stands (1,800 s proved it). Not a regression either.

**(historical) R0 as measured 2026-08-14 before the cause was found.**
Evidence and method: `artifacts/performance/2026-08-14_runtime2-p95-closure/GATE_ARM_OWNERS.md` §1.

- **The ceiling is refuted by measurement.** The loop harness was driven directly
  with the exact Boundary argument set and its own `ValidateRange(5,3600)` raised
  to **1800 s**. It timed out at 1800 s exactly as it did at 120 s and 600 s.
  **Do not raise it again.**
- **The stub is refuted by a same-session control.** On the same runner slot 2,
  the same melonDS GDB stub, the same DLDI setting and the same HEAD, the gate
  tick-HUD ROM booted and completed a *larger* workload — 2,039 presented frames,
  1,600 samples, 17 ring stops — in **123 seconds**.
- **The breakpoint is sound, checked three ways.** `nm -S`: one text symbol each.
  `gdb … info breakpoints` host-side: bp2 lands at **`0x020250a0`, the function's
  first instruction**, `nds_renderer.c:**15535**` (the ":15504 misreport" note was
  wrong). `objdump -d`: exactly one live call site, in `syTaskmanRunTask` at the
  Wait→GO edge (`taskman_seam.c:8100-8111`). ARM/Thumb mode unchanged vs the last
  green build. So bp2 not firing means the call never happened.
- **Breakpoint 1 `scVSBattleStartBattle` DOES hit, 0.05 s after the attach**, so
  the failure is in **battle-scene setup, not early boot**. (A probe written to
  step the presented-frame marker from there was malformed and its reading is
  retracted — `GATE_ARM_OWNERS.md` §1.3. Its cause is now impossible:
  `Invoke-GdbMarkerScript` took `[string[]]$Commands`, so a jagged list built by
  a multi-command helper was **stringified into one fused gdb line**; the
  parameter is `[object[]]` and flattened in the body as of this cycle, and
  embedded newlines throw. The batch command list every verifier uses had no
  such guard, while the MI path has had one since 2026-08-03.)
- **The bisect window is five commits.** Boundary was green in the DEFAULT
  configuration at 2026-08-13 22:01
  (`…/2026-08-13_c-bore36-bgstretch/boundary.log`; also 17:20 and 18:50, and the
  `NDS_R2_PATH=1` arm at `…/2026-08-13_c-r2path-recheck/`), i.e. after
  `8fc8b47c9ce` (08-13 21:27). Every code commit since is 2026-08-14:
  `697303ed77c` · `33d7cc5d3b7` · `54d7d7862e4` · `9b6c9e72a25` · `813207773c1`.
- **Next action, in order:** bisect those five with the loop harness at a 600 s
  ceiling (a green arm returns in ~120 s, so a 600 s budget separates green from
  red in one run each), then fix at the owning seam. A probe that may be killed
  must `set logging file …` + `set logging enabled on` — gdb's buffered stdout is
  otherwise lost with the kill, which cost this cycle the elapsed time of bp1.
- **Landed:** `scripts/lib/gdb-markers.ps1` now prints elapsed and % of ceiling
  used on SUCCESS (and elapsed on both failure paths), so the next drift is
  visible before it is a red.

All host-side checkers, including `check-gbi-decode-fixtures.ps1` and the
particle-bank pack, are green.

The P1 acceptance-level rows, highest impact first. The gate lane above is
row 1's execution plan.

1. **Stable 30 FPS** — qualify the whole match at P95 ≤ 1.12M ARM9 ticks per
   presented frame on the **both-CPU stress config**, loading states excluded
   (owner, 2026-08-05), on the accuracy melonDS fork. The shipped ROM stays the
   Boundary hwtri configuration. Hardware remains the final check for mechanisms
   the emulator cannot referee.

   **BANK, 2026-08-14 on the settled HEAD** (`build-c158-gate`, git
   `a159069af0d`, `BOTH_CPU 1`/`DRAW 1`, DLDI ON, frames 440–2039, 1,600 samples;
   `…/2026-08-14_runtime2-p95-closure/GATE_ARM_OWNERS.md`):
   `WORK-H` **P50 939,136 · P90 1,096,448 · P95 1,184,064 raw / 1,159,117 net ·
   top-1% 1,562,560**. Gap **+63,684 raw / +38,737 net**. **The requirement a
   package is judged on is 64,452 at the 80th-largest frame** — 91,844 is
   superseded. The −26,880 against the old 1,210,944 is **not** a cost win: the
   matches diverge (P1 damage 58 → 76 after the Fox bore-84 fix), so nothing may
   be sized from it. **Cadence, `DRAW=0` per the owner's 2026-08-14 choice:
   89.1% two-VBlank, max interval 6** (`DRAW=1` reads 84.9%, max 10).

   **OWNER RANKING, corrected.** On the 80 frames that set P95 the excess is
   +508,993, `SRC` +456,480 = **89.7%, all inside `gcRunAll`** (`SRC−GCRA` =
   **−22**, a third population putting it at zero). **Below `SRC` the ranking is
   match-specific and only `SITR` survives** (+171,234 → **+188,907**); `SHDT`
   119,920 → 80,837, `SPHD` 112,833 → 75,236, `SCPU` +7,222 → **−8,669**, `SPHC`
   +62 → **+52,780**. Draw side 7.1% by tick-HUD bracket and **8.4% by the v3
   profile's draw closure — two instruments, two arms, one answer.**
   The first gate-arm v3 stall capture now exists (`…/v3-gate-arm/`): P95-set
   excess is `icache_fill` +155,795 (40.0%) · `dcache` +96,800 · `issue`
   +94,029, **no function above 3.6%**. **Next target: in-match
   animation-asset load I/O, +93,436 on the 80 and +51,276 after the outlier
   falsifier — land a per-frame asset-acquisition counter on the gate arm
   BEFORE any code change.**
2. **Mario/Fox completeness** — replace battle-reachable weak status callbacks
   with source-backed behavior and prove both complete movesets naturally.
3. **Dream Land completeness** — close the remaining Whispy material/animation
   presentation debt without reintroducing gameplay-time texture conversion.
4. **Audio completeness** — implement or explicitly qualify every reachable
   voice, pitch schedule, composite cue, and overlapping match-audio path.
5. **Hit-effect presentation** — owner filed against the N64 reference,
   2026-08-05/06: the A-attack spark is oversized (two multipliers, light ramps
   with damage to 56px, heavy is flat), and the fire *burn* on the victim is
   absent. Detail, seams and the owner's verbatim wording are in `docs/BUGS.md`;
   do not restate them here. The spark ceiling is explicitly a port choice with
   the owner as oracle (`386fb8e2`), so it closes on their eye, not a
   measurement. **`docs/BUGS.md` item 3 — the owner's billboard observation —
   is a design input for any future effect work and bears directly on why the
   G3 packet path was refuted; read it before opening that lane again.**
6. **Final acceptance** — the CPU-on one-minute match, complete-match capture,
   owner play/listen pass, reserve gate, Results transition, and teardown
   proof on the exact candidate ROM.

## The eight decomp patches migrate port-side over time (owner, 2026-08-06)

The owner tightened `AGENTS.md` mid-session to **"Treat `decomp/` as read-only
reference source. Our Source of Truth. Never edit it."**, deleting the
sanctioned-exception paragraph that used to permit tracked patches. Asked
whether the eight existing patches should be grandfathered, migrated, or kept as
an acknowledged exception, the owner chose **migrated port-side over time**.

**DONE 2026-08-15, and the rule text and the tree now agree.** All ten patches
are gone from `scripts/decomp-patches/` and `decomp/` is byte-for-byte upstream.
Two were genuinely lifted port-side — `src_ft_ftmain` into
`src/import/battleship_ftmain.c` + `ndsRelocResolveAuthoritativeForceFile`, and
`src_mn_..._mntitle` into `battleship_mntitle.c`'s three framebuffer aliases
before `syVideoInit`. The other **eight keep their patch text but never touch the
source of truth**: `scripts/generate-battleship-import-overlay.ps1` copies the
pristine files into `$(BUILD)/battleship_overlay/` and `git apply --directory`s
`scripts/import-overlays/battleship/*.patch` onto that ephemeral copy, and the
`src/import/` wrappers `#include <battleship_overlay/…>` instead of
`"../../decomp/…"`. Deepening those eight is still moving the wrong way; the
overlay is a holding pen for the older adaptations, not a sanctioned home.

**The gate is structural, not a note.** `scripts/check-decomp-pristine.ps1` runs
inside `check-gbi-decode-fixtures.ps1`, which `verify-all.ps1` invokes on **every**
profile including Boundary. It fails on three separate things: a pinned-hash or
whole-tree-hash drift under `decomp/src` (`fetch-battleship-reference.ps1
-VerifyOnly`, which now hashes rather than greps for markers), any
`SSB64_TARGET_NDS` marker inside `decomp/`, and the mere **existence** of a
`scripts/decomp-patches/battleship/*.patch`. Editing `decomp/` is therefore
inexpressible without a red verifier.

**Zero built bytes moved.** `builds/build-c190-overlay-byteproof` rebuilt the
`c185` bank configuration at the same HEAD and `compare-elf-sections.py` reads
**0 differing bytes** across `.itcm`/`.text.hot`/`.text.hot.draw`/`.main`/
`.main.rw`/`.dtcm`, with every section header identical in name, size and VMA
(only `.debug_*` grew, by the two default-OFF census `#define`s). The bank stands.

| patch | added | shape | difficulty |
|---|---:|---|---|
| `src_sys_objman` | 124 | 19 `while (TRUE);` panics -> recorded NULL allocation failures, mid-function at 19 allocator sites | **hard** — needs port-side allocators; a wrapper cannot reach a mid-function panic |
| `src_mv_..._mvopeningroom` | 653 | "NDS entry slice": port code inside the decomp file, original body `#if !defined`'d out | **medium** — already port code, but interleaved, so it is a whole-file fork not a lift |
| `src_sys_objanim` | 105 | animation-script parsers: event bound + recorded fault on unknown opcode | medium; **pinned by `generate_nds_native_stage.py` TEXT_INPUTS — re-pin in the same change** |
| `src_ft_ftanim` | 36 | guarded inserts, 1 line replaced | case-by-case |
| `src_sys_taskman` | 32 | guarded inserts, 1 line replaced | case-by-case |
| `src_mn_..._mnstartup` | 18 | guarded inserts | case-by-case |
| `src_sc_scmanager` | 6 | framebuffer end address + two whole functions disabled | mid-function constant; needs reimplementation |
| `src_sys_objhelper` | 6 | guarded inserts | case-by-case |

Across all eight, only **4 source lines are actually replaced**; the rest are
additions under `#if defined(SSB64_TARGET_NDS)`.

**The one worked example is `2b693142`** (damage-spark scale). It migrated
cleanly *only* because the value being changed was reachable from the source
maker's return value (`pc->xf`), so the port wrapper could adjust it on the way
out. Look for that shape first; most of the rows above do not have it.

**Worth separating when prioritising:** most of the added lines convert an
infinite-loop panic into a recorded failure. Those do not change what SSB64
does — they change what happens when a pool the N64 never exhausts is exhausted
on DS. That is closer to "physically cannot work on DS" than to a behavioural
divergence, and it is the weakest case for urgency.

## Parked — open items with owners' notes, promote deliberately

- **R2-08 (the switch) is ONE MAKEFILE LINE, and SwitchPlan §6 items 1 and 4 are green on THIS
  tree — re-measured 2026-08-13 (cycle 15), nothing landed.** Runbook and evidence:
  `artifacts/performance/2026-08-13_c-r2path-recheck/SWITCH_READY.md`. Every green this week ran
  the `NDS_R2_PATH=0` arm and R2-06 E0's equivalence was from 2026-07-29, so it was re-run.
  **Item 1:** `Boundary verification profile passed.` through `$env:NDS_R2_PATH='1'`, `Exception:`
  0 in the full 18.9 MB log, engagement both directions (`ndsR2BattleRun` at `0x020875c4` in the
  proof ELF, 0 hits in the control). **Item 4:** `NO-FREEZE`, in-guest match timer confirmed 1
  minute, 3 battle entries / 2 completed matches / 2 START rematches, runaway 0, misalign 0,
  ledger fail 0, alloc-fail 0, panic 0, `SweepFail` 0, heap free-min **70,392**, `slips` 0,
  Results P95 2 VBlanks. **Item 3 is R2-07's, not the switch's:** the R2 arm reads `WORK-H` P50
  **929,344** / P95 **1,204,352** against the `PATH 0` control's 924,864 / 1,210,944 — **+4,480
  and −6,592, both inside their floors and opposite in sign**, with damage 0/58, stocks 1/1,
  ledger 1,177/183/1,574/1,371, shield joints 80/1,344/800 and heap free-min 70,592 all identical.
  R2-06 E0's "the switch neither costs nor saves anything" therefore still holds after slices
  43–52. The switch's whole static footprint is **+80 B text, 0 data, 0 bss**. Remaining: §6 item
  2 (owner's eye, and it should be taken on the published ROM *after* the flip — Boundary's
  screenshot arm reads the root ROM, so it is not R2 coverage before it) and item 5 (owner's
  retail play test). **One divergence found and NOT fixed:** `666e99a2148` gave the Runtime 1
  loop `gNdsPositionProbeUpdateInPresent = update_in_iteration` (`taskman_seam.c:8051`) and did
  not mirror it into `ndsR2HostBattleUpdateOnce`; `NDS_R2_POSITION_PROBE ?= 0` so no shipped ROM
  is affected, but a probe build on the R2 path would read that index as 0 on every capture.
- **Fox blaster bore offset: LANDED 2026-08-13 (cycle 14), and it was the OWNER's call, not an
  agent's.** He took option B of `artifacts/bugs/2026-08-12_fox-crouch/BEAM_QUAD_ANCHOR.md`: the
  beam quad and the muzzle-flash quad draw joint-local `(0,-24,0)` higher, gameplay untouched.
  Applied as a world **+24 Y = 98,304 Q12** on the DECODED translation — `nds_renderer.c:14979`,
  before the source scale reaches the quad's own vertices, because `scale.x` runs 1.0 -> 53.33 and a
  later fold would raise a grown beam 1,280 units — and a draw-local centre copy at
  `battleship_lbparticle.c:2571`, where `pc->pos` and the AOT pool entry are deliberately NOT
  written. The impact flashes move with it on purpose: all four source callers
  (`wpfoxblaster.c:61/71/86/121`) pass the weapon's own translation, so they sit on the beam's
  centre line. **Gameplay invariance measured, not asserted:** one 60 s both-CPU match per arm, six
  gameplay counters read at every one of **17 ring stops — 0 mismatches**, and all fourteen
  end-of-run globals equal (damage 0/58, stocks 1/1, 8 shots, 38 beam draws, 0 fallbacks).
  **Pixel proof:** `EXACT_LOCK` on `gSCManagerBattleState->time_remain`; beam centroid
  **205.000 -> 202.000 (-3.000 rows)**, flash centroid **-3.009**, same 707 px and same x span; on a
  locked frame with no beam the battle screen is **0 of 120,000** different. Text **-184 B**, so
  `WORK-H` P50 +256 / P95 -9,728 is placement and **this is NOT a re-bank**. Boundary green
  (`Exception:` 0 in the full 18.9 MB log), root ROMs byte-identical. **It SHIPS in the next
  published ROM** (`NDS_R2_FOX_BLASTER_QUAD ?= 1`). The owner's eye is the remaining acceptance:
  the flash is 3.4x the gun's height and covers the barrel on exactly the frames the beam's tail is
  at it, so no screenshot can settle the final alignment.
  Evidence: `artifacts/performance/2026-08-13_c-fox-bore/BORE_OFFSET.md`.
- **The DObj-parser runaway: FIXED 2026-08-13 (cycle 13).** The seam was not the flag and not
  the dispatch: `lbCommonAddDObjAnimJointAll` (decomp `lb/lbcommon.c:785`) **had no body in this
  port** — an empty stub at `reloc_backend_compat_shims.c:2140`, `bx lr` at `0x02052eac` in the
  shipped ELF, because `src/import/battleship_lb_common.c` is parked and out of `CFILES`.
  `ftCommonGuardInitJoints` sets `is_anim_joint = TRUE` *and calls that function in the same
  breath, because calling it is what makes the flag true*; with no body the flag was true while
  every joint still held the GuardOn figatree. **There was no missing clear** — `ft/ftmain.c:4633`
  re-derives the whole `anim_desc.word` from the motion descriptor on every install, invisible to a
  field-name grep because `FTAnimDesc` is a union, and Mario's GuardOn row (`ft/ftdata.c:275`)
  carries no `FTANIM_FLAG_ANIMJOINT`. Fox's entry Arwing (`ef/efmanager.c:5734/5736`) was the same
  stub's second dead caller. Measured on the five-minute both-CPU arm, 8,448 samples:
  `gNdsAnimJointDispatchFigatreeCount` **144 → 0** (it was **144 of 144** — every 32-bit dispatch
  in the match was a misread), `gNdsObjAnimRunawayCount` **50 → 0**, `Dispatch32` 144 → **9,154**
  so the counter is armed, `AttachCount` **9,154** = `Dispatch32` exactly. **The runaway counter
  under-reported the class 2.9x**: 96 of 144 were 2 mod 4, the other **48 were 4-aligned and
  decoded to a legal opcode — silent**. **PRICE: `WORK-H` P95 +49,216 on the gate arm**
  (1,210,880 → **1,260,096**, P50 923,392 → 924,928), isolated to +44,544 by the one-variable
  five-minute pair, so it is work and not placement; the ledger high-water moves 1,019 → **1,598 of
  2,048**, which retires cycle 12's "capacity ≥ 2x the corpus" derivation. Boundary green, root ROMs
  byte-identical. Evidence and the named recovery lever (memoise the two port-only per-joint
  lookups by `(fighter, angle_i)`):
  `artifacts/performance/2026-08-13_c-animjoint-fix/ANIMJOINT_FIX.md`. **Never loosen the parser
  bound and never teach it opcode 100.**
- **The DObj-parser runaway: ATTRIBUTED 2026-08-13, FIX HANDED FORWARD.** *(cycle 12, superseded by
  the row above — kept because its arithmetic is still the reference for opcode 100.)* Finding:
  `gcParseDObjAnimJoint` (32-bit) is run on fighter joints holding a **16-bit figatree**;
  `anim_joint` is a union `ftAnimParseDObjFigatree` advances in place, so the "corrupt" 2-mod-4
  pointer is a perfectly good `event16` pointer and **opcode 100 is what a misaligned ARM9 `LDR`
  returns** — predicted exactly on all six captured hits. Attribution: one GObj `link_id` 3 =
  Fighter, six DObjs, asset **557 = `MARIO_ANIM_SHIELD_ON`**, caller **`ftParamUpdateAnimKeys`**
  whose per-fighter dispatch on `fp->anim_desc.flags.is_anim_joint` is **source-exact**
  (`ftparam.c:386`). Fix state: the seam is the FLAG, whose only writer in tree is
  `ftcommonguard1.c:275` and whose clearing writer was **not found where I looked** — the next
  cycle reads the figatree-install path and adds the broken-invariant counter named in
  `KNOWN_ISSUES.md`, which owns this row now. **Never loosen the parser bound and never teach it
  opcode 100.** Evidence: `artifacts/performance/2026-08-13_c-anim-anomalies/ANOMALIES.md`;
  instrument: `scripts/probe-objanim-runaway.ps1` (no build, derives the fault block from the ELF).
- **The AObj event-32 match-length cliff: ATTRIBUTED AND FIXED 2026-08-13.** Not a leak — the
  per-stop trajectory has four consecutive zero-growth stops while the reuse path fires 16-19 times
  each, so the key is stable and growth is corpus coverage. It is a **ledger, not a cache** (the
  repack is a bit permutation with no spare bit, so eviction is corruption and a match unloads
  nothing), which leaves capacity: `NDS_AOBJ_EVENT32_NORMALIZED_MAX` **1024 → 2048**, +8,192 B bss,
  proven boot headroom 176,128 → **167,936**. The shipping **1-minute** arm already stood at
  889/1,024; the corpus is now *proven* 1,019 by `gNdsAObjEvent32NormalizedHighWater` on a re-run
  five-minute match, i.e. 1,029 spare slots against 5. `WORK-H` P50/P95 move +2,880/+2,240, inside
  the ±14,080 cross-build floor. Evidence: `…/2026-08-13_c-anim-anomalies/QUALIFICATION.md`.
- **`scripts/check-docs.ps1` is unowned and wired into no `verify-all` profile**,
  so it can only go red where nobody looks. It has done so twice in two cycles:
  the `RAM_RECOVERY_PLAN.md` index gap (fixed `560328b357f`) and then the missing
  Boundary-entry token in `HANDOFF.md`, whose arms table now names
  `battle_playable_realtime`. **GREEN again 2026-08-13** (`docs=23,
  registryEntries=4`). Decide deliberately: re-grade its pins, or wire it into a
  profile so a red doc gate is visible without a hand run.
- **SRC sub-owner instrument: LANDED, cycle 85.** Three cycles of "does not
  boot" were the boot cliff, exactly as cycle 82 concluded; G2 freed the room and
  the same design booted first try. **Cost +1,152 bytes** (text +80, bss +1,056)
  against 134,496 proven — 116x margin — links at `0x02273f24`, boot probe PASS
  (frames 60–67, 8 samples, `slips=0`), and the arena stayed at its full request
  (`gNdsTaskmanArenaChosenSize` 1,376,256, `AllocFailCount` 0), so the instrument
  did not re-starve what G2 just fed.
  **The shipped ROM pays none of it.** Every part — enum, globals, ring, names,
  both brackets, the publish and the two resets — sits behind `#if NDS_TICK_HUD`,
  and the published `smash64ds-battle-playable-hwtri` builds `NDS_TICK_HUD 0`.
  The only unconditional source change is inverting the early-return guard in
  `ndsR2AnimCachePreloadStep` so the bracket has a single exit, which is
  mechanically identical (`if (c >= N) return; load();` == `if (c < N) load();`).
  **Two ring buckets, not three.** `SHDT` = `ftMainProcSearchHitAll`
  (`reloc_backend_diagnostic_recorders.c:5663`) and `SWRM` =
  `ndsR2AnimCachePreloadStep` (`reloc_backend_assets.c:6424`, called from
  `battleship_scvsbattle.c:344`), both bracketing existing port wrappers so **no
  `decomp/` edit was needed**. The third sub-owner `SBAS` is **derived** as
  `SRC - SHDT - SWRM` by `analyze-tick-hud-excursion.ps1`: it costs no bytes, and
  `SBAS >= 0` on every frame is the *proof* the two ringed spans are nested
  inside SRC — the script throws if any frame goes negative. Appended after
  `WORK`, kept out of `named` (they are sub-spans of `SRC`, which `named` already
  counts), so `OTHR`/`WORK` and the WORK-H identity are byte-identical.
  `$bucketNames` moved in the same commit as the enum, and the sub-buckets are
  excluded from the sampler's `named` share too — verified on the boot probe,
  where `meanNamed` read 949,400 exactly and would have read 954,720 if they had
  been wrongly included.
- **R2-03 E35: mechanism CONFIRMED at whole-match scale, ownership REFUTED.**
  Its magnitudes were already retired for sitting on a 128-frame window. Cycle 85
  re-measured the mechanism on 1,600 frames and both halves of the verdict are
  now evidence. **Confirmed:** hit detection is genuinely bimodal and
  switches on with expensive frames — elevated `SHDT` (>5,500) occurs on 173 of
  1,600 frames and **82.7% of those are over gate against a 42.9% base rate**;
  hot p95 203,008 against clean p95 5,312, a 38x tail separation. **Refuted:**
  it does not own the excursion. **544 of 687 over-gate frames (79.2%) sit at the
  `SHDT` floor**, so hit detection can explain at most 20.8% of them, and it is
  12.9% of SRC's excursion against `SBAS`'s 87.1%. The honest reading is that
  E35 found a real minority mechanism and its window flattered it into the
  majority one.

- **Task 56 strips may have been closed on the boot cliff, not on strips
  (cycle 82, address evidence only — NOT re-tested).** `builds/build-t56-strips`
  links at **`0x02294f04`**, 1,792 bytes above the highest address proven to boot
  and 992 above the lowest proven to fail, i.e. deep in the failing region. Its
  recorded symptom — "cannot reach presented frame 12 in 900 s" — is the cliff
  signature, not a present-loop bug, and the control it was judged against
  (`build-t56-control`, `0x0228cea4`) is a 3-day-older, ~31 KB smaller tree, so
  it was never a matched control. Three attempts, two builds, three days were
  spent on this. **Do not re-open before G2**; after G2, re-link it and run
  `check-boot-headroom.ps1` before concluding anything about strips.
- **+52,928 ticks/frame regression** between `2494daf9ad` and `e49a98167c`,
  null control, real, NOT in the three reverted hunks. Untested suspects:
  `38bba475` BLENDPE prim/env bake + `key_generation` fence, `0a060c7b`
  alpha/blend recogniser, `e8c675d3`/`999fcdf8`. Re-open against the
  whole-match instrument only.
- **Concurrency calibration** (workflow, cheap): same tickhud ROM, solo run vs
  two concurrent runs on slots 2/3 — guest tick series should be identical
  (deterministic emulation; host load moves wall clock, not guest ticks). If
  clean, bless 2-concurrent measuring runs (Boundary + both-CPU
  simultaneously) and functional-verify overlap. Watch harness wall-clock
  liveness thresholds (STALLED/TOO SLOW) — they read observed frames/s.
- **`check-decomp-header-mirror.py` RED on HEAD** — `FTSTAT_OPENING1_START`,
  `nSYAudioBGMExplain`; pre-existing; a guard blind to its class of bug.
- **`sNdsRendererRuntimeTextureCacheEvictCount` liveness unproven** — read 0
  all run, never shown able to be non-zero. Do not cite evictions from it.
- **Per-build ELF resolution in harnesses** (`Makefile:60-90` names the fix):
  the root `.elf`/`.nds` pair is shared between build dirs; a published build
  intervening between a lab build and its measurement silently swaps the pair.
  Until fixed: lab build immediately before its measurement.
- **Particle `sqrtf` axis magnitudes** (`ndsParticleTransformForDraw`): move
  two `sqrtf` calls inside the existing `transform_id` guard; ~200K calls a
  match. P50/foreground lever, not the gate. Watch
  `gNdsTickHudForegroundTicks`.
- **Particle atlas admission is stale**: 24 live textures, sheet admits 14 of
  47; texture 1 is in `QUAD_MEASURED_LIVE` and lost its slot. Re-run
  `scripts/generate_nds_particle_banks.py` and re-derive; budget question is
  VRAM cache contention (PORTING.md: 16K/32K sheets failed via
  `PrepareRun` drops), not RAM.
- **GATE 6 price correction on record**: the source-effects flip was sold at
  +36,032 P95 on the bad window; real cost ~360,000 on every effect-active
  frame. The decision stands (make the submit path cheap, do not delete the
  models) — the number behind it did not.
- **RESOLVED cycle 80 — the Boundary verifier was RED for 35 commits, and the
  first attributed cause was wrong.** `EXPECTED_CENSUS_SHA256` went stale at
  **`fcf93d00`** (2026-08-04 17:05), *not* at `4a413079`. **RETRACTED:** the
  census does **not** hash `nds_renderer.c` source text — `parse_renderer_contract`
  extracts semantic facts (the key field tuple plus four required tokens), each
  failing closed with its own message, and returns hardcoded constants. That
  claim was inferred from a `read_text` call without reading what the parser
  does with it.
  Bisected: the pre-`fcf93d00` script against the pre-`fcf93d00` tree reproduces
  `829c895d…` exactly. A field-level manifest diff moves **six leaves, all in
  `renderer_key_contract`**, all corroborated by `nds_renderer.c`'s own defines —
  `current_cache_entries` 48→69 (`CACHE_COUNT 69u`), `cache_entry_bytes_profile`
  280/276→44/40, and three new fields `static_cache_entries` 24
  (`STATIC_COUNT 24u`), `dynamic_key_pool_bytes` 10,620 ((69−24)×236),
  `static_pointer_word_bytes` 288 (24×3×4). **Nothing in the texture corpus
  moved**, and every `decomp/` input still matched its own pinned sha256, so it
  was a reviewed consequence of a deliberate change, not corpus drift — which is
  why re-pinning was correct here and would *not* have been had a corpus field
  moved.
  The second failure ("Static texture generator reported no residency bytes",
  `verify-battle-mariofox-gcrunall-loop-harness.ps1:317`) was **one cause, not
  two**: that generator imports the census, died on the same digest, emitted no
  JSON, and the absent field read as 0. Its own `EXPECTED_INCLUDE_SHA256` then
  needed re-pinning too, and that delta is **pure provenance** — the include
  stamps the census digest in a header comment, so exactly one line changed
  while `EXPECTED_PAYLOAD_SHA256`, `EXPECTED_METADATA_SHA256`, residency 61,696
  and payload 61,210 all stayed put.
  **The durable lesson: re-pin in the same commit that changes what the pin
  covers.** A pin whose subject moves silently disables every gate downstream of
  it — here the entire Boundary profile aborted in pre-flight, so 35 checkpoints
  including the whole cycle-79 gate lane landed without a runtime check.
  Two secondary notes: `src/nds/generated/…static_textures.generated.inc` is an
  **untracked build product** whose on-disk copy was still the Aug-3 version, so
  the build's prerequisites for it do not include the census script; and its
  staleness was harmless only because the delta was a comment.
- **`check-one-minute-match-verifier.ps1` has drifted from its owner**
  (2026-08-03): 55 `Assert-Text` pins against exact source text, at least two
  red on refactors that changed nothing they guard. Regrade the pins against
  what each actually protects, or delete the ones already asserted by the
  owner's own gates.

## Standing measurement rules (the ones that gate evidence)

### 2026-08-11 — Slice 43 fighter blink reopened; optimization withdrawn

- Owner ROM bisect isolated the regression to base GX compose: `c119-gxctl`
  clean; `c119-gxcompose`, `gxcompose2`, and `c121-stride` blinked.
- Bad-ROM 96-frame census proved this was not draw admission: Mario submitted
  320 and Fox 306 hardware triangles on every sampled frame.
- Parent/cross palette overlap was real but **not sufficient**: the deliberate
  bad control measured `0x00F80000`; the union-reservation arm measured zero for
  both fighters with 0 GX-compose declines, yet the owner's playtest still
  blinked. Retract the prior "root cause / closed" wording.
- Correctness wins over the 13,632-tick optimization. Published, tick-HUD/proof,
  Results, and Bug-9 targets now force `NDS_R2_FIGHTER_GX_COMPOSE=0`, returning
  to the owner-bisected clean `c119-gxctl` CPU-compose path. Keep HW_MTX on.
- The 1,210,560 gate was banked with Slice 43 enabled. Its historical delta was
  `WORK-H` P50/P95 -10,624/-13,632, but do **not** arithmetically re-bank; run the
  next 1,600-frame gate before quoting the new current gap.

#### 2026-08-15 — the GX stack leak is SITED AND FIXED, and it never belonged to slice 43

`artifacts/performance/2026-08-15_gxstack-io-draw/GXSTACK_IO_DRAW.md`.
Measured on a `NDS_R2_FIGHTER_GX_COMPOSE=0` ROM (`build-c173-cfxcount-bp1`,
128 presented frames 438–565): the position/vector stack level advances
**+3.000 per frame wrapping mod 32** with GXSTAT's error bit set on 128 of 128
frames. **The leak predates slice 43; slice 43 only made it visible** by parking
live data in absolute `MATRIX_STORE` levels the pointer walks over.

- **Not the particles.** In the same run `gNdsParticleBatchOpens` flattens
  mid-window; the 37 intervals that open no batch still advance 3.000.
- **Exact ledger from the linked ELF + the per-PC census** (17 `0x04000444` /
  `0x04000448` stores in 10 functions, counts from `b-c181-pc.csv`, 1,600
  regions): PUSH 5,143, POP 4,831, and the residual **+312 is exactly** the raw
  FIFO `MATRIX_POP` word `ndsRendererFinishWhispyNativePacket` emits (312 whole
  match, 27 on the marginal 80). **Every push/pop the ARM9 writes itself nets
  zero**, so the producer is the Task 36 replay stream, which is DMA'd into
  `GFX_FIFO` and invisible to a PC census.
- **Site.** `nds_renderer.c:6218` recorded `run->local_pushed` from
  `capture_push_balance`, a per-run **delta**, where `EndSegment` (`:30424`)
  needs the **state** the stream leaves. `EnsureWorld` pops the previous world
  before pushing its own (`:30290-30294`), so every run after the first records
  balance 0 while the stack is still one push deep; replay assigns the last
  run's value verbatim (`:30783`). `EndSegment.part.0` runs **3.000×/frame** and
  its `MATRIX_POP` executed **0 times in 1,600 frames**;
  `gNdsRendererTask36CaptureSegmentMask` = `0xA1` = **3 replayed segments**.
- **Fix and proof** (`build-c183-gxstackfix`): record the live
  `sNdsNativeStageOwnerExecution.task36_local_pushed` instead. Level **0** and
  error bit **0** on all 128 per-frame samples and at all 17 whole-match ring
  stops (frames 534–2038). Whole-match invariants equal the `c170`/`c174`/`c175`/
  `c176` bank (P1Damage 76, spark 15, shield 1,352, AObj 1,266, packHits 197,
  runaway 0). Boundary green, 0 `Exception:`.
- **Free corroboration**: every Boundary log in `artifacts/` from 2026-08-03 to
  2026-08-15 prints `gxstat=0x6009600` — level 22, error set — on the
  **shipping-default arm**; this cycle's prints `0x6000000`. The defect was on
  the published path, not only the gate arm.
- **`NDS_R2_FIGHTER_GX_COMPOSE` is still `?= 0` and its −13,632 is still
  STALE** (baseline 1,258,112 vs today's 1,177,344). The flag was not flipped;
  re-measure before re-banking.

#### 2026-08-15 — slice 43 RE-MEASURED on the repaired tree: −17,152 at rank-80, and one acceptance item is still missing

`artifacts/performance/2026-08-15_gxcompose-remeasure/GXCOMPOSE.md`, with
`PREDICTION.md` written before the first run. **Cycle stopped early on the
owner's instruction; read §6 of that file for what was not done.**

- **The pair.** `build-c184-gxc-a` (flag 0) vs `build-c184-gxc-b` (flag 1),
  both fresh at HEAD `771cd4b8312` on `smash64ds-battle-playable-tickhud-hwtri`,
  `BOTH_CPU=1`/`BATTLEPACK=1`/`KEEP_CACHE=1`, DLDI on, `DRAW=1`, 1,600 samples
  frames 440–2039, `slips=0` both. Their `nds_build_config.h` differ in
  **exactly one define**.
- **`WORK-H` rank-80 1,189,312 → 1,172,160 = −17,152** (0.53× of the +32,593
  requirement); **P50 947,360 → 939,264 = −8,096**; mean −10,737; P90 −8,960;
  top-1% −14,784; frames over 1,120,380 130 → 129. Both clear their own floor
  (`VERIFYING.md` cycle 100: P95 ≥14,080 sign unreliable, P50 ~5,700 sign kept)
  **in the same direction**.
- **`FTR` rank-80 −9,664 / mean −11,399**, everything else flat except
  **`STG` +3,200 / +2,675, which is NOT attributed** — relink, GX state handed
  to the stage, or FIFO contention; three candidates, none measured, do not
  price one.
- **The flag is compile-time, so the one-byte pair is impossible.**
  `compare-elf-sections.py`: `.main` +2,040 B, `.itcm` +36 B, 18,955 differing
  bytes. The placement floor is **not** zero and this is stated, not hidden;
  the verdict rests on P50 + the bucket + the control reproducing itself
  **exactly** (`c183` and `c184-gxc-a`, different ELFs and sessions, both
  rank-80 **1,189,312**).
- **Engagement total, flip budget zero.** `Declines` **0** whole match;
  `Captures`=`Roots` 63,364 (31.08/fr), `Locals`=`Mults` 110,702 (54.29/fr),
  `Restores` 55,546 (87.7%), `Stores` 41,598 (65.6%), `ProjectionSkips` 59,414
  (**93.8%**). All eight symbols are **absent from arm A's ELF**. All eight
  end-of-match invariants equal the bank on both arms.
- **The withdrawal's acceptance condition holds at flag 1.** GXSTAT
  `0x06000000` at all 17 whole-match stops and on 128/128 per-frame samples;
  `GFX_POLYGON_RAM_USAGE` **432/463.5/510 with zero frames under 350** over four
  full 32-frame wrap periods, against the control's 432/464.5/510. The blink was
  recorded as **106–306 against a 378 median** at the wrap. **Accepted polygons
  are the oracle for this defect; submitted triangles are not.**
- **STILL OPEN, and it is the reason nothing is banked or flipped:** the
  **frame-locked pixel pair was not taken**. Both capture ROMs exist
  (`build-c184-cap-a`/`-cap-b`, proof target, `TICK_HUD 0`, `BOTH_CPU 0`) and no
  capture was run. Slice 43's original "pixel-identical" predates the blink's
  discovery and must not be inherited. Flipping the default stays
  `BLOCKED(decision: shipping default)`.
- **Re-banking the LEVEL was deliberately not done.** Against the `c170` bank
  the residual would be +15,441 net; on this pair's own arm B (1,172,160 raw /
  1,147,213 net) it is +26,833. The two controls differ by 11,392, inside the
  ≥14,080 floor, so **only the within-pair −17,152 is quotable**.
- **Makefile.** The tick-HUD/proof pin now reads one documented escape,
  `NDS_R2_FIGHTER_GX_COMPOSE_LAB=1`. Probed on all three targets before a build
  was spent: tickhud **1**, proof **1**, published
  `smash64ds-battle-playable-hwtri` **0 even with `LAB=1` set**. Hand-editing a
  pin to run a lab arm is what this replaces.
- Both root ROMs byte-identical; Boundary green at the shipping default,
  0 `Exception:`, and its pacing smoke prints `gxstat=0x6000000`.

#### 2026-08-16 — two draw-side falsifiers closed with zero builds

`artifacts/performance/2026-08-16_gxcompose-bank-basis/BASIS.md` §6–§7.

- **ITCM golden reclaim — CLOSED at 632 B, and only 56 B of it is takeable.**
  The **literal-pool modality has no discriminating power**: scanning every
  LOAD section for words equal to a helper's address returns 0 for
  `__aeabi_fadd`, `__mulsf3` and `__divsf3` too, and those are entered
  1,728 / 2,122 / 431 times. Branch reachability (external edges only) finds
  **0 entries** into `__aeabi_frsub`/`__subsf3`/`__addsf3` (456 B), the five
  `fcmp` goldens (120 B) and `__unordsf2` (56 B); independently, a whole-match
  v3 capture has **0 sampled instructions** in `0x01ff802c..0x01ff81e8`.
  **But the live bodies are repo-authored** (`nds_task16_float_addsub.o`,
  `nds_task16_float_compare.o`), so `NDS_TASK16_FLOAT_*=0` **reverts Task 16 to
  stock libgcc — it is not a reclaim**; and `--rename-section .text=.itcm`
  (`Makefile:3767`) renames whole members, trapping 576 B beside live symbols
  that `--gc-sections` cannot split. Only `_arm_unordsf2.itcm.o` (56 B) is
  wholly dead. **Consequence: 616 B free on the shipping configuration + 56 =
  672 B against the camera v3's 916 B overflow — the ITCM route stays closed**
  until the extraction emits per-function sections.
- **`__aeabi_fadd` is FLAT — the only lever is the call count.** Top PC
  **4.63%** across 81 PCs over its 400 B, **96.22% issue stall / 3.78%
  icache**, 18.33 tk/call, **1,805.7 calls/frame** whole match and 3,903.9 on
  the marginal-80 (33,106 / 74,380 tk/fr). Per
  `a-flat-function-only-lever-is-not-entering-it` there is no instruction to
  delete, and it is already ITCM-resident so placement will not move it. The
  brief's "444 B ladder" is `__addsf3`, the *dead* libgcc original.

#### 2026-08-15 — GX compose pixel variance OWNER-ACCEPTED; fresh c185 bank established

The missing matched-tic proof was taken from `build-c184-cap-a/-cap-b`.
Battle-screen differences were **43–209 / 120,000 pixels (0.0358–0.1742%)**;
the owner inspected the actual diff masks and accepted that variance as visually
acceptable. This supersedes the blocker above without claiming pixel identity.
GXSTAT remains **0x06000000**, the accepted-polygon blink signature remains
absent, and gameplay invariants match.

Fresh bank `build-c185-gxcompose-bank`, DRAW=1, BOTH_CPU=1, DLDI on, 1,600
samples frames 440–2039, slips 0: **P50 938,112 · P90 1,088,192 · rank-80
1,174,016 raw / 1,149,069 net · top-1% 1,520,832 · over gate 122/1600**.
Exact gap: **+53,636 raw / +28,689 net**. This is the measured bank level; the
old −17,152 A/B was not subtracted from any prior bank.

Required DRAW=0 cadence sibling: **VBI 2:1850 3:173 4:8 5+:8 max 19**, total
2039, slips 0 = **90.731% two-VBlank**, still below ≥95%. Published GX compose
remains pinned OFF while the overall acceptance set is still red.

1. Whole-match `-RingDump` sampling is the only gate instrument; label every
   figure with its arm **and its coverage**; DLDI-on only. **Coverage is part
   of a baseline's identity, not a footnote** — a window is "whole match" only
   if its fraction was measured against the match clock.

   **FIXED, cycle 80.** `scene_harness.c` used to seed `time_limit = 7` under
   `NDS_R2_BOTH_CPU`, so the gate arm sampled frames 440–2040 of a 420-second
   match — **12.6% coverage, the opening minute** — while the identical window
   on Boundary covered 86.7% and ended at the buzzer. That one line superseded
   every both-CPU tick figure in the campaign. Both arms now run the 60-second
   match and measure identically (86.7%, clock 52 s → 0 s, logic:presented
   2.000); the soak's long match lives on `NDS_R2_SOAK_MATCH_MINUTES`.

   `scripts/probe-match-window.ps1` measures coverage and **reads the match
   timer out of the guest** (`gSCManagerTransferBattleState.time_limit`)
   instead of taking it from the command line — its `-TimeLimitMinutes`
   is now only a cross-check and disagreeing with the guest throws. Run it on
   any new arm before banking that arm's ticks.

   **The window ends 43 frames past the buzzer on both arms** (1998–2040, GAME
   SET, `SRC` < 50,000, gate-arm mean `WORK-H` 711,751). It is the same tail on
   both, so cross-arm comparison is sound; a single-arm figure should say
   whether it is full-window or gameplay-only.

   What the correction cost: the gap went 485,060 → **503,684**, i.e. the old
   early-match window was optimistic by 18,624. What it did **not** overturn:
   the SRC/MISC inversion, which held at 69.6% → 68.9% → 67.4% across a 12.6%,
   an 86.7% and a gameplay-only window. Note shares still drift *within* a
   window (both-CPU `MISC` 104,076–221,815 across 200-frame blocks), so a
   sub-window share is still not a match-level one.
2. Verify a counter is live in the shipped configuration BEFORE the measuring
   run; a proof-scoped counter reads 0, indistinguishable from clean.
3. Eliminate candidates with a liveness probe on an already-built ROM before
   spending a measuring run.
4. `ALL` is VBlank-quantized; judge on `WORK-H`.
5. Do not multiply a number back by what you divided it by and call the
   agreement a finding.
6. New tables/code: byte cost stated + boot probe before measuring (see G2).
7. Prefer one dual-route binary over separately-linked A/B ROMs wherever the
   change can be routed at runtime; this ROM's pacing is placement-sensitive
   and split builds have confused two comparisons. `sample-tick-hud-buckets.ps1
   -SetGlobals name=value` is the mechanism (cycle 79).
8. **A routed arm must prove the route took before its ticks are read.** A poke
   that silently fails still produces a complete, plausible percentile table,
   which reads exactly like a candidate that engaged and saved nothing —
   `-SetGlobals` did this on its first two runs (see `VERIFYING.md`). Pair
   every `-SetGlobals` with an `-ExtraGlobals` counter that cannot be zero if
   the route engaged.

   **The readback is now IN THE ARTIFACT (cycle 100).** The harness always
   printed `SETGLOBAL=<name>,<value>` straight after each poke, but nothing
   parsed it, so the proof lived on the console — the one place this project
   has repeatedly agreed not to read. `sample-tick-hud-buckets.ps1` now parses
   it into a `setGlobals` block (`requested` / `readback` / `stuck`) and
   **throws** rather than emitting a percentile table when a poke did not take.
9. **A poke can land and still not be seen — check the cache line before
   trusting `-SetGlobals`.** The stub writes main RAM; the ARM9 keeps its own
   copy. If the target shares its 32-byte line with anything the guest writes,
   that line is dirty, the guest keeps reading its stale value, and every
   writeback stamps it back over the poke — the readback still says the write
   succeeded. Measured cycle 100 on `gNdsFtrPlanRoute` (`0x0226c560`, sharing a
   line with the per-frame `gNdsTickHudVBlankWaitTicks`): poked 7, read back 7,
   **0** route hits over 1,216 draws, 0 at end of run — while a sibling four
   bytes lower in the **previous** line survived the identical batch, and a
   second variable 12 bytes *higher* in the **same** line was erased with it. A
   4-byte store cannot do that; a 32-byte line writeback does exactly it, and
   the disassembly's only three references to the address are loads.
   **Consequence: a flag that must be routed at runtime needs its own clean
   cache line, or it belongs at build time.** The fighter draw plan took the
   build-time route (`NDS_FTR_PLAN_ROUTE`).

## Acceptance Matrix

As last graded (cycle 76); a row changes state only when its gate runs.

| Acceptance condition | State | Current evidence / blocker |
|---|---|---|
| Mario human vs original level-3 Fox CPU, Dream Land, one-minute Time, items off | Pass configuration | Boundary registry exposes only canonical mode 163 |
| Original Wait -> countdown -> GO, timer, scoring, Time Up, Results | Focused gates pass | Final exact-ROM CPU-on owner run remains red |
| Mario and Fox complete source-equivalent gameplay behavior | Red | Battle-reachable weak callbacks remain |
| Dream Land collision, platforms, blast zones, wind, camera | Pass for current P1 stage | Dynamic presentation debt remains red separately |
| Recognizable Dream Land presentation and required animation | Red | Whispy Route 7 owner-approved and promoted 2026-08-08; remaining stage presentation not regraded |
| Complete overlapping BGM, FGM, voices, announcer, crowd | Red | Exact pitch/composite/voice coverage and listen gates remain |
| Stable 30 FPS, representative P95 <= 1.12M ticks | Red | Gap **503,684 on the both-CPU gate arm**, 60 s match at 86.7% coverage (356,292 is the Boundary figure and is not the gate); lane G1–G4 |
| Stable reserve, no corruption, clean teardown | **Stress battery passes 2026-08-13; both anomalies now closed** | Both-CPU chains to 5 battle entries incl. Sudden Death, plus a 5-minute match: `NO-FREEZE`, heap free-min 67,652–70,384 (floor 32,768), GObj cap never latched, alloc-fail/overflow/objman-panic 0, texture-certificate `SweepFail` 0, picture colour floor 1,305. `gNdsObjAnimRunawayCount` **0** on the five-minute arm since the anim-joint install fix (was 50); ledger high-water **1,598 of 2,048**, `NormalizeFailCount` 0, heap free-min 70,000 (`artifacts/performance/2026-08-13_c-{stress,animjoint-fix}/`) |
| Reproducible public artifact | Red | Current local root ROM differs from the pinned public identity |

## Artifact Identity

Pinned public-build identity from `README.md`:

```text
smash64ds-battle-playable-hwtri.nds
11,428,864 bytes
SHA-256 4D795B4E83B335598B20A3B5953FDB1821797CC5E0A825FA96A0643ABBA4A090
```

Current shipping pair after Fox blaster native promotion, verified 2026-08-09:

```text
smash64ds-battle-playable-hwtri.nds   12,211,200 bytes
SHA-256 C49F2C528F9EA13BA9F05985248C1BA2CCD5681EAA7A2B0C5023F5557F2D7EA4
smash64ds.nds                         11,915,264 bytes
SHA-256 54C07FAC80C50418949908701F7C2BDBF27512C5F96AC09086FABBB0DF6AC68A
tick-HUD sibling (builds/build-tick-hud-buckets)   12,218,368 bytes
SHA-256 B7800E4921E1F2BCC89EB7A4BBECDA279F44111D226BEA32D05EF7FA319C1A4F
```

ROM hashes are not reproducible across rebuilds of identical source; compare
sizes and the build log, not hashes, when attributing a ROM to a tree.
**The mechanism was identified 2026-08-13 (slice 53): NitroFS packs its
directory entries in a nondeterministic ORDER.** Two builds of identical source
into different directories differed by **14 bytes** of that table while
`.text`, `.data` and `.rodata` were byte-identical. So the executable *is*
reproducible, and the right comparison is
`arm-none-eabi-objcopy -O binary --only-section=.text` (and `.data`, `.rodata`)
on the two ELFs — not the `.nds`, and not sizes alone.

### BUGS row — Fox bore offset v2 LANDED, Dream Land BG LOCALIZED (cycle 151)

Owner playtest 2026-08-13 gave three verdicts. This cycle executed two of them.

**Fox — `NDS_FOX_BLASTER_BORE_OFFSET_Y` 24 -> 36, landed and measured.** Owner:
*"Pistol beam and muzzle flash are a little better, could use more height."*
Prediction written before the run (+12 world units at the measured 8 units/row =
−1.500 rows); **measured −1.500 rows exactly**, centroid 146.000 -> 144.500 at
`EXACT_LOCK` tic 1688. Beam width unchanged (x 122–222, **101 px per row in both
arms** — 707/7 = 606/6, so the 7->6 row count is centre-sampling phase on a
6.25-row band, not clipping). Flash moved with it: moved-pixel region spans rows
128–164 and x 55–222 in mirrored pairs. **Negative control: beam-free tic 1694 is
0 of 120,000 battle-screen pixels different**, which also retires the cross-build
floor by proving every commit since `cffcea495a6` visually inert on that frame.
Control was the *reused* `build-c143-bore`; only the candidate was built.
Evidence `artifacts/performance/2026-08-13_c-bore36-bgstretch/BORE_OFFSET_V2.md`.
**Row stays OWNER-QUEUED — the eye is the gate, and §2 of that file offers 30 and
24 as prepared alternatives if 36 reads too high.**

**Dream Land BG edges — LOCALIZED, deliberately NOT implemented.** The row is N64
overscan surfacing on a display that has none: `grWallpaperMakeCommon` (Dream
Land takes the `default:` arm of `grWallpaperMakeDecideKind`) drives a 300x220
sprite whose scale floor is **1.004**, so at maximum camera pull it draws
**301.2 x 220.88 into a 320x240 preview** and both position clamps cap at **+10**.
Predicted exposure 5.87% horizontal against the owner's *"about %5"*, and the
port's own measured comment for the sibling static wallpaper
(`sprite_preview_backend.c:1578-1584`, "an 8-pixel backdrop frame on all four
sides") independently pins both the 300x220 and the mechanism. The shipping
`NDS_FAST_WALLPAPER_AFFINE` path makes it sharper: the seed is captured at
`seed_dist 14000`, which clamps to the same 1.004, so **the seed raster IS the
max-pull frame** and live frames only ever sample inside it — full-bleeding the
seed closes the row, and `K` cancels in the affine ratio so `hdx`/`vdy` and the
cadence do not move. Derived **K = 1.12** about the preview centre (K_min =
1.090909, set by the top edge). Three consumers must move together; the ready-made
proof counter is `gNdsFastWallpaperSeedOpaquePixelCount`, 42,650 -> 49,152.
**Blocking hazard for the implementing cycle, and it is free to check first:** the
correction drives `origin` negative, and `ndsSObjDrawOpaqueWallpaperFinal`'s
destination-range derivation was not read. The `(u32)(dst - origin)` form at
`:734`/`:742` is safe; the O3 mapper's range is not yet known to be. Full
derivation, seam table and proof plan in `…_c-bore36-bgstretch/BG_COVERAGE.md`.

**Whispy — owner playtested and saw NO change ("still isn't smooth").** Not worked
this cycle; the next step is a pixel-cadence probe of the FACE, not another
derivation of the blink, which is already source-correct.

## Lane Ownership

| Surface | Owner |
|---|---|
| Goal, fidelity, milestone, definition of done | `PROJECT_GOAL.md` |
| Dynamic queue, artifact identity, blockers | this file |
| Exact restart surface and next packet | `HANDOFF.md` |
| Verification workflow and measurement law | `VERIFYING.md` + Standing rules above |
| Stable architecture | `ARCHITECTURE.md` |
| Durable unresolved gaps | `KNOWN_ISSUES.md` |
| Measurements and rejected experiments | `PERF_LEDGER.md` |
| Chronological history | `PORTING.md` |

## Integration Rule

Keep only correctness-preserving, verifier-covered progress. Rendering may use
the fidelity budget in `PROJECT_GOAL.md`; gameplay must remain mechanically
equivalent to the original. Run the smallest relevant check, then one widest
relevant verifier for a kept checkpoint.
