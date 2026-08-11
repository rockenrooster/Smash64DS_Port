# Handoff

Updated: 2026-08-11. **The gate is re-banked at 1,258,112** (RAM recovery, −36,032
cross-build). Requirement 4 shipped before it: the fighter `AObj` is fixed point,
`WORK-H` P50 −23,360 / P95 −37,504 routed. **Every 128-frame figure in the archive
is unusable** — it reads the cheapest 6% of the match. Use `-Samples 1600`.

## The two baselines — label every figure with its arm AND its coverage

Both arms run the **same 60-second match** (coverage 86.7%), windows ending 43
frames past the buzzer. Slips 0 in every row.

| arm | role | `WORK-H` P50 | P95 | over gate |
|---|---|---:|---:|---:|
| **both-CPU** | **THE GATE** | **958,592** | **1,258,112** | re-banked c119 |
| **Boundary** mode 163 | shipped configuration | 920,192 | 1,113,408 | re-banked c116 |

**Gate 1,258,112, re-banked after the RAM campaign — P95 −36,032 / P50 −2,560,
4.2x the ±8,544 floor and the campaign's largest single P95 move; it came from
RAM, not CPU work.** **Gap ~138,112.** The
soak's long match is `NDS_R2_SOAK_MATCH_MINUTES`; `probe-match-window.ps1` reads
the timer from the guest, so a window cannot claim coverage it lacked. Owner's bar:
the whole match under P95 on the both-CPU config, loading excluded; the shipped
ROM stays the Boundary hwtri pair. `Makefile:382` forbids reporting a both-CPU
P95 as Boundary's; **re-pin `EXPECTED_CENSUS_SHA256` when coverage changes.**
**Route to ATTRIBUTE, re-bank to BANK.** Slice 31: −7,104 routed, +576 built — real AND under the floor.
**Collision paid, and it is BANKED.** **Slices 35/36/37 memoise the three
`line_id` scans over static geometry — routed −7,552 / −4,864 / −1,984, banked
−10,752**, Boundary green; slice 35 read −7,232 and −7,552 on two separately-
linked binaries. All owners are **FLAT** — no PC over 3.6% — so the lever is
calls: `…GetFCCommonFloor` 45,372 x 818 cyc, `…FindLineEndpoints` 38,890 x 543,
`…SweepFloorLoopSweep` 11,544 x 2,643. **But collision was never a P95 owner** —
nor was animation. **THE TAIL IS ASSET STREAMING** (`--split-top-frames 80`, 1st
partition matching the gate): FAT+movers+attach+locks = **29.3–39.7% of a tail
frame = 184,414 tk** vs the **174,144 gap**; game+renderer are 11.6%. **CAUSE:
anim arena 200,400/200,704 = 99.85% FULL**, 38 overflows = 38 rejects, 91/128
entries so BYTES bind, NO eviction → those 38 re-read from ROM all match; ~82 KB short. SIZE IS NOT PERMISSION:
float in `gmcollision`/`mp*`/`ftMain*`/`ftComputer` is FROZEN by the Task 9 hash
(`census-softfloat-callers.ps1`) — exact moves only, which is why 35–37 are memos.

## What is dead, so nobody re-derives it

- **Effect DObj submits** — Boundary-only: 99.3% of the Boundary excursion but
  **~12.1%** of the gate arm's; G3 refuted cycles 88–91. **Projectiles** (44
  tk/fr) · **Particles** (flat ~47,000, P50 only) · **texture thrash** ·
  **`Find`** · **`Material`** · **the force-load seam**.
- **`FTR` as the *P95 discriminator*** (+13,768). NOT "FTR is exhausted" — that
  reading is what the owner re-opened 2026-08-10; cycles 110–116 took 24.3% off
  it. `FTR` is **flat**, on nearly every frame, which is why.
- **The AOT animation bake at 20 B/record** (slice 32). Reader, bake, emitter,
  layout guard and wiring are PROVEN and gated off; the SIZE is dead — 10,304 B
  an animation against the source's 2,310, ×85 cached needs +679,490 B, and
  dropping every init record leaves 3.27×. Attach IS on the tail, but as 8.4% of
  it — the FAT read that feeds it costs more than the parse.
- **The animation PLAYBACK path, on MEAN cost (slice 34).** Its two largest
  self-time symbols are already Requirement 4's fixed point at 1.67–1.69
  cyc/insn; the rest is **8,772 tk/fr** of soft float, all under the floor — as
  are idle-joint skip (33), lazy track table (31), AObj walk and track dispatch.
  At the tail too: animation+collision+renderer = **11.6%**. **Slice 39's table is
  VOID** — threshold ON the quantum, 616 frames sorted by 0.154% jitter. **Don't
  blanket-convert `ndsBaseGcPlayMObjMatAnim`** — 5 tracks pack 0xRRGGBBAA in f32.

## RAM: both budgets are near their floor — price a change before writing it

- **Static/boot.** `check-boot-headroom.ps1 -Build <dir>` after every lab build.
  Highest `fake_heap_start` proven to boot **`0x02294804`**, lowest proven to
  fail **`0x02294b24`**. **Text counts as much as bss**; a failing arm reads as a
  hung emulator. **`gSYTaskmanGeneralHeap`** free-min **42,136** against the anim
  cache's 32,768 `KEEP_FREE`, coupled since freeing `.bss` enlarges it.

## FTR is re-opened and moving: −93,612 landed (cycle 116)

**Banked `FTR` mean is 291,896** (P50 301,760, P95 304,768) against a pre-slice
baseline of **385,508** built for the purpose, matching the owner's ~385–390K.
**−93,612, 24.3%** — the *mean* beats the owner's 300K target, P50 is 1,760 over.
Measured on the SHIPPED strips; the −94,666 quoted earlier came off the build that
was losing geometry, withdrawn. Boundary passes on each.

**The big one is DS-native AOT geometry: Task 56 fighter strips now SHIP**
(`NDS_TASK56_FIGHTER_PRIMITIVES ?= 2`). 626 triangles submitted as 1,878
`GL_TRIANGLES` corners are now **1,014 strip corners in 163 groups**, compiled
host-side. One-binary A/B on `gNdsR2FighterStripRoute` (identical `romSha256`,
route read back 0 and 1): **`FTR` P50 −10,432, P95 −10,368, `WORK-H` P50
−10,112, P95 −14,144**; `STG` +64 and `ALL` identical to the tick are controls.

**It SHIPPED BROKEN once and the owner caught it in minutes — read the board
before touching this.** An undirected edge made **35.6% of the fighter
BACKFACING**, and `BEGIN_VTXS` on group TYPE change alone **welded adjacent
strips into one vertex list**. `check_fighter_primitive_streams.py` now **models
the runtime's BEGIN policy rather than assuming one per group** — run it after
touching either. **A passing verifier is not visual verification**: Boundary
passed on the broken build. **Hand the owner a ROM.**

**The emit stalls per VERTEX, not per word** (c115 `--pc-detail`, no build): a
corner is 40.5 cycles untextured, **~28 of it the GX write**, and textured pays
the same ~28 for a fourth word. That is why strips work, and it **refutes a
baked/DMA'd GX stream and `VTX_10`** — both trade words, not vertices; the win is
~50% of prediction, so part is **per polygon**. **Dense UVs are immutable too.**

**The other big lever was the I-cache, not arithmetic.** `…DLAllDrawForSlot`
was the ROM's largest non-idle symbol at **4.21 cyc/insn**, 10,708 bytes against
an **8 KB** I-cache, **73.6% never executed**; outlining the never-*entered*
bodies took it to **7,516**/**7,236**. Recipe, no build: `--pc-detail SYM[,SYM…]`
(one CSV pass serves N), diff `objdump`, `addr2line` four points per cold run.
**Entry count discriminates**: cold bytes in an *entered* body cost **+14,963**.

- **The DObj world cache had ZERO readers** while `Store` burned 4,744,740
  cycles and ~4 KB a frame through a 4 KB D-cache. **Ask what reads a cache
  before optimising what fills it**, and **read the counters a prior cycle left**.
- **The compose does not fold its base in until a joint contributes** (−10,804):
  one call per binding was *copy the base in, multiply it straight back out*.
- **The material block is built 30 times a match, not 59,392** — a (MObj, heap
  gen, animatable-input hash) key owned by the material **DObj**; `BindingParents`
  is the nearest *bound* ancestor, not the DObj parent.
- **A per-PC census charges a miss to the instruction that TAKES it**, not to the
  work you can delete: two redundant first-reader passes came out for **+1,055**,
  because the data was still consumed and the fills only moved.

**Compiling the frame-summary counters out is refuted** (FTR −7,378 / STG
−2,776): it **breaks the gate** — `…gcrunall-loop-harness.ps1` asserts exact
batch and texture-prepare accounting off those globals. **A census row is not an
FTR row**: the bracket is `ndsFighterDisplayContractSubmit` only. **Tick factor
0.4993 tk/cyc** comes from `ALL` vs total cycles; deriving it from the FTR sum is
circular and overstated coverage 22%. On c115 that reconciled **35 named symbols
= 244,774 tk/fr, 78.1% of `FTR`**, residual bounded by shared leaves.

**Next, priced** (c115 census, tk/fr). **`Task36ReplayRun` 17,796 is STAGE, not
FTR** — it takes `NDSNativeStageRun`. In FTR:
`ExecuteNativeFighterOwnerProduction` **26,307** + `NativePrepareProductionRun`
**25,277** (five-phase split on the board); `BuildFighterTraRotRpyDirect`
**17,698** (fixed-point inside; six conversions a joint is its only float
boundary); `LoadHardwareSplitMatrices` **13,122**, E23 projection-skip refuted.

**The `SINT` split is DONE.** `SINT` +88,082 = `ftMainPlayAnim` **+60,559**
(animation) + `ftComputerProcessAll` +24,386 (map collision, not AI), retiring
`SRC_CPI_OPTIMIZATION.md` items 4-6. **Force-load seam closed:** `ftmain.c:4623`
**discards the return value**. D-cache census: loads 7.07 cyc/ex, excess 17.83%.

**Fighter animation is fixed point — Requirement 4 shipped** (slice 25). One
binary, `gNdsR2AnimCutRoute` 7 vs 15: **`SINT` P50 −24,896, `WORK-H` P50
−23,360, P95 −37,504**; `FTR` ±64 and `gNdsR2CubicEvals` 285,210 in both arms
are the controls. **It does not move `FTR`.** What is LEFT is ~70,000 cyc/frame
of memory stall, not arithmetic — board prices it. **No float cache beside it.**

**Do not bring a micro-fix** — R2-06 E11: a load-frame-only ~8,000 cannot be
banked, because relinking moves the tail more than the saving. Clear ~16,000 in
one change, stack proven deletions into one arm, or **use the `.data` route and
measure on ONE binary** — that is how the strips landed. Load frame priced on
the board (premium 650,610/frame; `ndsRelocAssetIDForToken` **CLOSED**).
**Every change needs an engagement counter** — cycle 110 read FTR −13,587 off a
skip it could not prove fired.

**Do not re-derive these.** The Makefile's `?= 0` defaults are not the shipped
config (41 overridden). `.text.hot` is closed both directions
(`linker/nds_hot_text.ld:179-201`), **3.30 cyc/insn, worse than `.main`**;
census sections C/D are a cost ranking, never a placement prediction.
**Latent cliff, unowned:** `sNdsAObjEvent32NormalizedCount` reads **973 of
1,024** after a minute; overflow silently **skips the animation attach**.
**The load-frame exclusion is REFUTED — do not apply it** (`SRC > 2x median` is
circular for SRC, swings the gap **3.08x**, drops non-loads). **Boundary for all
of it** — a change altering a visible pixel of the shield, revival platform,
impact wave or reflector needs the owner (`BUGS.md`, by eye).

## Measurement rules that change your FIRST action — board owns the rest

- **The sampler is bit-deterministic — never repeat a run.** Same ROM twice gives
  byte-identical buckets, variance 0. So the 14,080 cross-build figure is
  **placement, not noise**; anything under it needs the `.data` route. Use
  `-Samples 1600 -RingDump -AllowRepeatedFrames`; a *faster* ROM trips the
  repeated-presented-frame guard on per-frame stops. Payload IDENTICAL is a
  stale read and always fatal; DIFFERS is a real second iteration.
- **Judge on `WORK-H`**; buckets locate, they never decide (floor ≥8,544).
  **`ALL` is VBlank-quantized** — it hid a +52,928 once and the strips entirely.
  **1.85 cycles of `FTR` mean per byte of added ARM text** — beat your footprint.
- **A census row in tk/fr sizes a P50 lever, NOT a P95 one.** Slices 36 and 37
  had the same mean self cost (2,666 / 2,588) and P95 wins **2.45x apart**
  (−4,864 / −1,984): the tail pays for work that CLUSTERS on heavy frames. Mean
  predicted both P50s to 74–84% and neither P95. Get the per-frame distribution
  (`-PerFrameGlobals`) before predicting a gate win. This is why every c117
  animation lever landed at its mean — animation work is frame-uniform.
- **Disassemble the loop, read the caller, TAKE THE ENTRY-PC COUNT, and check
  WHICH bucket the symbol is in, before designing around it.** Cycle 108 built a
  loader `ftmain.c` discards; c109 aimed a `FTParts` fix at two `DObj` fields;
  c110 divided a symbol total by a guessed per-call cost; c116 found the board's
  named next slice was a STAGE symbol. All free (`--pc-detail`). **Resolve line
  numbers against the build's own `NDS_TASK10_GIT_SHORT`** — c106 was ~85 adrift.

## Restart surface — parked items live on the board's **Parked** list

`AGENTS.md` owns the start-of-cycle commands; do not duplicate them here.
`docs/P1_EXECUTION_BOARD.md` is the only dynamic queue (history in
`docs/optimization/archive/`); `Smash64DS_Runtime2_SwitchPlan.md` is the
charter; `docs/BUGS.md` carries the owner's verdicts — preserve their wording. A
clean checkout builds through `build.ps1`, not bare `make`: four of six `.inc` are
gitignored, and **`build.ps1`'s generator is not run by `make`, so it can rot
unnoticed — it had, until c116**. `make p1-tick` builds the measuring ROM, `make
p1` the published battle pair; bare `make` builds the P2 ROM P1 does not ship.
Never pass `-j`, never override `MAKEFLAGS`, one build at a time, never build a
published target name for lab work. Preserve canonical mode 163, renderer mode 9,
mip 0, static textures, source countdown, Dream Land water frame 0, Task 16 `1/1/1`. Never edit `decomp/`. Run `New-Smash64DSSnapshot.ps1` last.
