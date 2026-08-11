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
RAM, not CPU work. Gap ~138,112.** The soak's long match is
`NDS_R2_SOAK_MATCH_MINUTES`; `probe-match-window.ps1` reads the timer from the
guest, so a window cannot claim coverage it lacked. Owner's bar: the whole match
under P95 on the both-CPU config, loading excluded; the shipped ROM stays the
Boundary hwtri pair. `Makefile:382` forbids reporting a both-CPU P95 as
Boundary's; **re-pin `EXPECTED_CENSUS_SHA256` when coverage changes.**
**Route to ATTRIBUTE, re-bank to BANK** — slice 31 read −7,104 routed, +576 built.
**Collision paid, and it is BANKED.** **Slices 35/36/37 memoise the three
`line_id` scans over static geometry — routed −7,552 / −4,864 / −1,984, banked
−10,752**, Boundary green; slice 35 read −7,232 and −7,552 on two separately-
linked binaries. All owners are **FLAT** — no PC over 3.6% — so the lever is
calls: `…GetFCCommonFloor` 45,372 x 818 cyc, `…FindLineEndpoints` 38,890 x 543,
`…SweepFloorLoopSweep` 11,544 x 2,643. **The tail's own composition, at full
depth** (`--split-top-frames 80`, the partition that matches the gate):
**game+renderer 44.9%**, FAT/ROM reads 9.6%. An earlier "asset streaming is the
tail / game+renderer are 11.6%" reading was `--top 40` at 75% coverage — do not
re-derive it. The animation half of that streaming is GONE: the RAM campaign
took the arena to 262,144 and anim rejects to **0/0**. Banding the 80 tail
frames, **Band A (a symbol on ≥40 of 80) = 154,496 tk over 213 symbols** — more
than the ~138,112 gap. SIZE IS NOT PERMISSION: float in
`gmcollision`/`mp*`/`ftMain*`/`ftComputer` is FROZEN by the Task 9 hash
(`census-softfloat-callers.ps1`) — exact moves only, which is why 35–37 are memos.

## What is dead, so nobody re-derives it

- **Effect DObj submits** — Boundary-only: 99.3% of the Boundary excursion but
  **~12.1%** of the gate arm's. **Projectiles** · **Particles** (flat ~47,000,
  P50 only) · **texture thrash** · **`Find`** · **`Material`** · force-load seam.
- **`FTR` as the *P95 discriminator*** (+13,768). NOT "FTR is exhausted" — the
  owner re-opened that 2026-08-10; c110–116 took 24.3% off it. `FTR` is **flat**.
- **The AOT animation bake at 20 B/record** (slice 32). Proven and gated off; the
  SIZE is dead — 10,304 B an animation against 2,310, ×85 needs +679,490 B.
- **The whole animation lane, and now permanently** (slices 34, 41). Playback's
  two largest symbols are already Requirement 4's fixed point at 1.67–1.69
  cyc/insn; idle-joint skip (33), lazy track table (31), AObj walk and track
  dispatch are all under the floor. **Slice 41 spent the last lever**: 30 Hz
  poses deleted half of every evaluation, engagement proven, and `WORK-H` P95
  went **+7,040** while the arms diverged into different matches (damage 130/51
  vs 33/65, `SCPU` −20%) — so a route A/B cannot price it and
  `PROJECT_GOAL.md`'s level-3-CPU-equivalence requirement disqualifies it. With
  E61's mix (Cubic 54.8% / Step 43.6%, **zero** discarded evaluations) there is
  nothing left to memoize either. **Slice 39's table is VOID** — threshold ON
  the quantum. **Don't blanket-convert `ndsBaseGcPlayMObjMatAnim`** — 5 tracks
  pack 0xRRGGBBAA in f32.
- **A single >=16K lever (slice 42).** c119 attributed every arithmetic leaf on
  the true top-80 exactly; the soft-float owners ARE the camera/matrix lane. The
  two 20.12 multiplies measured **sub-floor** and were reverted, and nothing left
  is over 12,233 tk. The LANES are (20.12 kernels 62,891, float camera 55,865) —
  **batch bit-exact deletions into ONE compile-time build and measure
  cross-build**; slice 42 proved a multi-bit route over hot code is not additive.

## RAM: both budgets are near their floor — price a change before writing it

- **Static/boot.** `check-boot-headroom.ps1 -Build <dir>` after every lab build.
  Highest `fake_heap_start` proven to boot **`0x02294804`**, lowest proven to
  fail **`0x02294b24`**. **Text counts as much as bss**; a failing arm reads as a
  hung emulator. **`gSYTaskmanGeneralHeap`** free-min is **72,188** after the RAM
  campaign (−169,152 B static, arena at its long-requested `0x150000`); the
  inherited "42,136" was wrong — the true pre-campaign low-water was **15,120**,
  i.e. 17,648 UNDER the anim cache's 32,768 `KEEP_FREE`. Coupled: freeing `.bss`
  enlarges it.

## FTR is re-opened and moving: −93,612 landed (cycle 116)

**Banked `FTR` mean is 291,896** (P50 301,760, P95 304,768) against a pre-slice
baseline of **385,508** built for the purpose — **−93,612, 24.3%**; the *mean*
beats the owner's 300K target, P50 is 1,760 over. Measured on the SHIPPED strips;
the −94,666 quoted earlier came off the build losing geometry, withdrawn.

**The big one is DS-native AOT geometry: Task 56 fighter strips now SHIP**
(`NDS_TASK56_FIGHTER_PRIMITIVES ?= 2`) — 1,878 `GL_TRIANGLES` corners became
**1,014 strip corners in 163 groups**, host-compiled, for **`WORK-H` P50 −10,112,
P95 −14,144** on one binary. **It SHIPPED BROKEN once and the owner caught it in
minutes — read the board before touching this.** An undirected edge made **35.6%
of the fighter BACKFACING**, and `BEGIN_VTXS` on group TYPE change alone **welded
adjacent strips into one vertex list**. `check_fighter_primitive_streams.py` now
**models the runtime's BEGIN policy rather than assuming one per group**. **A
passing verifier is not visual verification**: Boundary passed on the broken
build. **Hand the owner a ROM.**

**The emit stalls per VERTEX, not per word** (c115 `--pc-detail`, no build): a
corner is 40.5 cycles untextured, **~28 of it the GX write**, and textured pays
the same ~28 for a fourth word — which is why strips work and why a baked/DMA'd
GX stream and `VTX_10` are **refuted**: both trade words, not vertices.

**The other big lever was the I-cache, not arithmetic.** `…DLAllDrawForSlot`
was the ROM's largest non-idle symbol at **4.21 cyc/insn**, 10,708 bytes against
an **8 KB** I-cache, **73.6% never executed**; outlining the never-*entered*
bodies took it to **7,516**. Recipe, no build: `--pc-detail SYM[,SYM…]`, diff
`objdump`, `addr2line`. **Entry count discriminates**: cold bytes in an
*entered* body cost **+14,963**.

- **The DObj world cache had ZERO readers** while `Store` burned 4,744,740 cycles
  a frame. **Ask what reads a cache before optimising what fills it.**
- **The material block is built 30 times a match, not 59,392** — a (MObj, heap
  gen, animatable-input hash) key owned by the material **DObj**; `BindingParents`
  is the nearest *bound* ancestor, not the DObj parent.
- **A per-PC census charges a miss to the instruction that TAKES it**, not to the
  work you can delete: two redundant first-reader passes came out for **+1,055**.

**Compiling the frame-summary counters out is refuted** (FTR −7,378 / STG
−2,776): it **breaks the gate** — `…gcrunall-loop-harness.ps1` asserts exact
batch and texture-prepare accounting off those globals. **Tick factor 0.4993
tk/cyc** comes from `ALL` vs total cycles; deriving it from the FTR sum is
circular.

**Next, priced** (c115 census, tk/fr). **`Task36ReplayRun` 17,796 is STAGE, not
FTR**. In FTR: `ExecuteNativeFighterOwnerProduction` **26,307** +
`NativePrepareProductionRun` **25,277** (five-phase split on the board);
`BuildFighterTraRotRpyDirect` **17,698** (fixed-point inside; six conversions a
joint is its only float boundary); `LoadHardwareSplitMatrices` **13,122**.

**The `SINT` split is DONE.** `SINT` +88,082 = `ftMainPlayAnim` **+60,559**
(animation) + `ftComputerProcessAll` +24,386 (map collision, not AI). **`SINT`
is the fighter INTERRUPT proc with `SCPU` nested inside it, not an animation
bucket** — reading it as one mis-attributed an A/B in c119. **Force-load seam
closed:** `ftmain.c:4623` **discards the return value**.

**Fighter animation is fixed point — Requirement 4 shipped** (slice 25). One
binary, `gNdsR2AnimCutRoute` 7 vs 15: **`WORK-H` P50 −23,360, P95 −37,504**;
`FTR` ±64 and `gNdsR2CubicEvals` 285,210 in both arms are the controls. What is
LEFT is ~70,000 cyc/frame of memory stall, not arithmetic.

**Do not bring a micro-fix** — R2-06 E11: a load-frame-only ~8,000 cannot be
banked, because relinking moves the tail more than the saving. Clear ~16,000 in
one change, stack proven deletions into one arm, or **use the `.data` route and
measure on ONE binary** (valid only if the change cannot alter gameplay state —
slice 41). **Every change needs an engagement counter** — cycle 110 read FTR
−13,587 off a skip it could not prove fired.

**Do not re-derive these.** The Makefile's `?= 0` defaults are not the shipped
config (41 overridden). `.text.hot` is closed both directions
(`linker/nds_hot_text.ld:179-201`), **3.30 cyc/insn, worse than `.main`**;
census sections C/D are a cost ranking, never a placement prediction.
**Latent cliff, unowned:** `sNdsAObjEvent32NormalizedCount` reads **973 of
1,024** after a minute; overflow silently **skips the animation attach**.
**The load-frame exclusion is REFUTED — do not apply it** (`SRC > 2x median` is
circular for SRC, swings the gap **3.08x**). **Boundary for all of it** — a
change altering a visible pixel needs the owner (`BUGS.md`, by eye).

## Measurement rules that change your FIRST action — board owns the rest

- **The sampler is bit-deterministic — never repeat a run.** Same ROM twice gives
  byte-identical buckets, variance 0. So the 14,080 cross-build figure is
  **placement, not noise**; anything under it needs the `.data` route. Use
  `-Samples 1600 -RingDump`. A duplicate frame LABEL at a ring-stop seam is now
  classified and warned rather than failed (slice 41); IDENTICAL payload is
  still a stale read and always fatal, as is a duplicate away from a seam.
- **Judge on `WORK-H`**; buckets locate, they never decide (floor ≥8,544).
  **`ALL` is VBlank-quantized** — it hid a +52,928 once and the strips entirely.
  **1.85 cycles of `FTR` mean per byte of added ARM text** — beat your footprint.
- **A census row in tk/fr sizes a P50 lever, NOT a P95 one.** Slices 36 and 37
  had the same mean self cost (2,666 / 2,588) and P95 wins **2.45x apart**
  (−4,864 / −1,984): the tail pays for work that CLUSTERS on heavy frames. Mean
  predicted both P50s to 74–84% and neither P95. Get the per-frame distribution
  (`-PerFrameGlobals`) before predicting a gate win. This is why every c117
  animation lever landed at its mean — animation work is frame-uniform.
- **A route A/B is only valid for a change that cannot alter gameplay state**
  (slice 41). It deletes the ±8,544 floor by holding the binary fixed, but still
  assumes both arms walk the same trajectory; a change collision reads
  decorrelates them and the delta then prices two different matches. Read an
  end-of-match gameplay counter (damage, KO) from the SAME run first.
- **Disassemble the loop, read the caller, TAKE THE ENTRY-PC COUNT, and check
  WHICH bucket the symbol is in, before designing around it.** Cycle 108 built a
  loader `ftmain.c` discards; c109 aimed a `FTParts` fix at two `DObj` fields;
  c110 divided a symbol total by a guessed per-call cost; c116 found the board's
  named next slice was a STAGE symbol. All free (`--pc-detail`). **Resolve line
  numbers against the build's `NDS_TASK10_GIT_SHORT`** — c106 was ~85 adrift.

## Restart surface — parked items live on the board's **Parked** list

`AGENTS.md` owns the start-of-cycle commands; do not duplicate them here.
`docs/P1_EXECUTION_BOARD.md` is the only dynamic queue (history in
`docs/optimization/archive/`); `Smash64DS_Runtime2_SwitchPlan.md` is the charter;
`docs/BUGS.md` carries the owner's verdicts — preserve their wording. A
clean checkout builds through `build.ps1`, not bare `make`: four of six `.inc` are
gitignored and **`build.ps1`'s generator is not run by `make`, so it can rot**.
`make p1-tick` builds the measuring ROM, `make p1` the published battle pair;
bare `make` builds the P2 ROM P1 does not ship.
Never pass `-j`, never override `MAKEFLAGS`, one build at a time, never build a
published target name for lab work. Preserve canonical mode 163, renderer mode 9,
mip 0, static textures, source countdown, Dream Land water frame 0, Task 16 `1/1/1`. Never edit `decomp/`. Run `New-Smash64DSSnapshot.ps1` last.
