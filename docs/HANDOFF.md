# Handoff

Updated: 2026-08-11. **The gate is re-banked at 1,225,280** — measured on
`build-c122-gate` after slice 43 was WITHDRAWN for the fighter blink, NOT derived
from the 1,210,560 slice-44 bank, which had the GX joint compose on. The
withdrawal cost P50 +4,864 / P95 +14,720 against slice 43's claimed −13,632.
**Every 128-frame figure in the archive is unusable** — use `-Samples 1600`.

## The two baselines — label every figure with its arm AND its coverage

Both arms run the **same 60-second match** (coverage 86.7%), windows ending 43
frames past the buzzer. Slips 0 in every row.

| arm | role | `WORK-H` P50 | P95 | over gate |
|---|---|---:|---:|---:|
| **both-CPU** | **THE GATE** | **936,512** | **1,225,280** | re-banked c122, slice 43 out |
| **Boundary** mode 163 | shipped configuration | 920,192 | 1,113,408 | re-banked c116, slice 43 IN — stale |

**Gate 1,225,280. Gap 104,900. The tail is no longer the renderer:** P95-setting
band (gate ranks 60–100) vs baseline is `SRC`/`GCRA` +262,048 on 36/41 — `SHDT`
hit detect +85,184 (4,544 → 89,728), `SINT` interrupt +77,856 — while **`FTR`
−896 and `STG` +448, both 0/41 presence**, dead as P95 levers whatever their size.

**Profile with `NDS_TICK_HUD_DRAW=0` or you profile the instrument** — the HUD
costs ~345,024 tk twice a second on exactly the frames P95 is decided on
(`nds_platform.c:68`), so `--split-top-frames` selects HUD-refresh frames: c122's
first profile put `ndsPlatformRenderDebugHud` at 17.5% of premium on 80/80 and
overlapped the real top-80 by ~20%. Neither harness sets it; the GATE keeps
`DRAW=1`, which is how every bank back to slice 44 was measured. The soak's long
match is `NDS_R2_SOAK_MATCH_MINUTES`; `probe-match-window.ps1` reads the timer
from the guest, so a window cannot claim coverage it lacked. Owner's bar: the
whole match under P95 on the both-CPU config, loading excluded; the shipped ROM
stays the Boundary hwtri pair. `Makefile:382` forbids reporting a both-CPU P95
as Boundary's; **re-pin `EXPECTED_CENSUS_SHA256` when coverage changes.**
**Route to ATTRIBUTE, re-bank to BANK** — slice 31 read −7,104 routed, +576 built.
**Collision paid, and it is BANKED** — slices 35/36/37 memoise the three
`line_id` scans over static geometry, banked −10,752. Their owners are **FLAT**
(no PC over 3.6%), so the lever was calls, not instructions. **The tail's own
composition, at full depth** (`--split-top-frames 80`, the partition that
matches the gate): **game+renderer 44.9%**, FAT/ROM reads 9.6%. An earlier
"asset streaming is the tail / game+renderer are 11.6%" reading was `--top 40`
at 75% coverage — do not re-derive it. Banding the 80 tail frames, **Band A (a
symbol on ≥40 of 80) = 154,496 tk over 213 symbols** — more than the gap. SIZE IS
NOT PERMISSION: float in `gmcollision`/`mp*`/`ftMain*`/`ftComputer` is FROZEN by
the Task 9 hash (`census-softfloat-callers.ps1`) — exact moves only.

## What is dead, so nobody re-derives it

- **Effect DObj submits** — Boundary-only: 99.3% of the Boundary excursion but
  **~12.1%** of the gate arm's. **Projectiles** · **Particles** (flat ~47,000,
  P50 only) · **texture thrash** · **`Find`** · **`Material`** · force-load seam.
- **`FTR` as the *P95 discriminator*** (+13,768). NOT "FTR is exhausted" — the owner re-opened that 2026-08-10; c110–116 took 24.3% off it. `FTR` is **flat**.
- **The AOT animation bake at 20 B/record** (slice 32): proven, gated off, SIZE dead — 10,304 B/animation vs 2,310, ×85 = +679,490 B.
- **The whole animation lane, and now permanently** (slices 34, 41). Playback's
  two largest symbols are already Requirement 4's fixed point at 1.67–1.69
  cyc/insn; idle-joint skip (33), lazy track table (31), AObj walk and track
  dispatch are all under the floor. **Slice 41 spent the last lever**: 30 Hz
  poses, `WORK-H` P95 **+7,040** while the arms diverged into different matches
  (damage 130/51 vs 33/65), so a route A/B cannot price it and level-3-CPU
  equivalence disqualifies it. E61's mix leaves nothing to memoize. **Slice 39's
  table is VOID** — threshold ON the quantum. **Don't blanket-convert
  `ndsBaseGcPlayMObjMatAnim`** — 5 tracks pack 0xRRGGBBAA in f32.
- **The 20.12 kernels' ARITHMETIC (slice 42)** — sub-floor and non-additive.
  **The local-matrix memo is dead twice** — E8's +16,301 key cost, payload 302
  tk/call since `MTX_DIRECT`. **The flower rigid-mask prices at +3,200, wrong
  sign** — board has the arithmetic.

## RAM: both budgets are near their floor — price a change before writing it

- **Static/boot.** `check-boot-headroom.ps1 -Build <dir>` after every lab build.
  Highest `fake_heap_start` proven to boot **`0x02294804`**, lowest proven to
  fail **`0x02294b24`**. **Text counts as much as bss**; a failing arm reads as a
  hung emulator. **`gSYTaskmanGeneralHeap`** free-min is **72,188** after the RAM
  campaign (−169,152 B static, arena at `0x150000`); the inherited "42,136" was
  wrong — the true pre-campaign low-water was **15,120**, i.e. 17,648 UNDER the
  anim cache's 32,768 `KEEP_FREE`. Coupled: freeing `.bss` enlarges it.

## FTR is re-opened and moving: −93,612 landed (cycle 116)

**Banked `FTR` mean is 291,896** (P50 301,760, P95 304,768) against a pre-slice
baseline of **385,508** — **−93,612, 24.3%**; the *mean* beats the owner's 300K
target, P50 is 1,760 over. Measured on the SHIPPED strips.

**The big one is DS-native AOT geometry: Task 56 fighter strips now SHIP**
(`NDS_TASK56_FIGHTER_PRIMITIVES ?= 2`) — 1,878 `GL_TRIANGLES` corners became
**1,014 strip corners in 163 groups**, host-compiled, for **`WORK-H` P50 −10,112,
P95 −14,144**. **It SHIPPED BROKEN once and the owner caught it in minutes — read
the board before touching this.** An undirected edge made **35.6% of the fighter
BACKFACING**, and `BEGIN_VTXS` on group TYPE change alone **welded adjacent
strips into one vertex list**. `check_fighter_primitive_streams.py` now models
the runtime's BEGIN policy rather than assuming one per group. **A passing
verifier is not visual verification**: Boundary passed on the broken build.
**Hand the owner a ROM.**

**The emit stalls per VERTEX, not per word** (c115 `--pc-detail`): a corner is
40.5 cycles untextured, **~28 of it the GX write**, and textured pays the same
~28 for a fourth word — why strips work, and why a baked/DMA'd GX stream and
`VTX_10` are **refuted**: both trade words, not vertices.

**The other big lever was the I-cache, not arithmetic.** `…DLAllDrawForSlot`
was the ROM's largest non-idle symbol at **4.21 cyc/insn**, 10,708 bytes against
an **8 KB** I-cache, **73.6% never executed**; outlining the never-*entered*
bodies took it to **7,516** (slice 43 put it back to 8,556). **Entry count
discriminates**: cold bytes in an *entered* body cost **+14,963**.

- **The DObj world cache had ZERO readers** while `Store` burned 4,744,740 cycles a frame. **Ask what reads a cache before optimising what fills it.**
- **The material block is built 30 times a match, not 59,392** — a (MObj, heap gen, animatable-input hash) key owned by the material **DObj**; `BindingParents` is the nearest *bound* ancestor, not the DObj parent.
- **A per-PC census charges a miss to the instruction that TAKES it**: two redundant first-reader passes came out for **+1,055**.
- **Compiling the frame-summary counters out is refuted** (FTR −7,378 / STG −2,776): it **breaks the gate** — `…gcrunall-loop-harness.ps1` asserts exact batch and texture-prepare accounting off those globals. **Tick factor 0.4993 tk/cyc** comes from `ALL` vs total cycles; deriving it from the FTR sum is circular.

**SLICE 43 WITHDRAWN 2026-08-11.** Owner retest still blinked after the parent-slot
union repair, so the earlier "fixed" claim was false — the `0x00F80000` overlap was
real but incomplete. All published/measurement/proof targets force
`NDS_R2_FIGHTER_GX_COMPOSE=0`. Re-banked at the top of this file; do not re-enable
without owner proof. The standing lead is at `nds_platform.c:3197` — the matrix
stack leaks ~3 pushes/frame and wraps mod 32, harmless until something parks live
matrices in absolute slots the pointer walks over.
**SLICE 44 KEPT — banked 1,244,480 → 1,210,560** (superseded by the c122 re-bank
above; full evidence `artifacts/performance/2026-08-11_c121-slice44/`).
`NDS_R2_STAGE_VALIDATE_STRIDE=8` strides the stage's 42-binding revalidation:
**WORK-H −17,088 / −35,904, STG −19,904 / −24,192.** Round-robin, NOT "sweep
every 8th frame" — the second shape makes 12.5% of frames expensive and P95
lands on one. Demotion is one-way within a topology: a partial sweep must not
re-arm what it did not look at.

**The fighter LOCAL matrix build is NOT a P95 lane — refuted c122.** It is real
size (~24,314 tk/frame, 80/80) and memory-bound, not arithmetic-bound (top PC
6.7%, `ldr` at 11–28 cyc/insn on cold DObj fields), so it was the post-slice-44
plan — but `FTR` separates the P95 band from baseline by **−896 on 0/41 frames**.
It is a P50 lane. Its only remaining shape is a local-matrix memo, **DO-NOT-RETRY:
built and killed twice**; the Task 91 comment at `reloc_backend_renderer_dl.c:1790`
argues for it anyway and is not an invitation.

**The `SINT` split is DONE.** `SINT` +88,082 = `ftMainPlayAnim` **+60,559** +
`ftComputerProcessAll` +24,386 (map collision, not AI). **`SINT` is the fighter
INTERRUPT proc with `SCPU` nested inside it, not an animation bucket** — reading
it as one mis-attributed an A/B in c119. **Force-load seam closed:**
`ftmain.c:4623` **discards the return value**.

**Fighter animation is fixed point — Requirement 4 shipped** (slice 25). One
binary, `gNdsR2AnimCutRoute` 7 vs 15: **`WORK-H` P50 −23,360, P95 −37,504**;
`FTR` ±64 and `gNdsR2CubicEvals` 285,210 in both arms are the controls. What is
LEFT is ~70,000 cyc/frame of memory stall, not arithmetic.

**Do not bring a micro-fix** — R2-06 E11: a load-frame-only ~8,000 cannot be
banked, relinking moves the tail more than the saving. Clear ~16,000 in one
change, stack proven deletions into one arm, or **use the `.data` route on ONE
binary** (only if the change cannot alter gameplay state). **Every change needs
an engagement counter, on BOTH sides** — c110 read FTR −13,587 off a skip it
could not prove fired; slice 43 read full engagement off a declining producer.

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
  had the same mean self cost and P95 wins **2.45x apart**: the tail pays for
  work that CLUSTERS on heavy frames. Slice 44 is the same law with the sign
  flipped — mean −10,838, P95 −35,904.
- **A route A/B is only valid for a change that cannot alter gameplay state**
  (slice 41). It deletes the ±8,544 floor by holding the binary fixed, but still
  assumes both arms walk the same trajectory. Read an end-of-match gameplay
  counter (damage, KO) from the SAME run first.
- **`--pc-detail` BEFORE designing the fix — it costs no build and routinely
  names a different lever than the source reads like.** Slice 44's guard looked
  like a compare to make cheaper; four cold `ldr`s were 39% of it and the compare
  rounded to nothing, so the lever was *not touching the objects*. Same call
  answered c108, c110 and c116. **Resolve line numbers against the build's
  `NDS_TASK10_GIT_SHORT`.**

## Restart surface — parked items live on the board's **Parked** list

`AGENTS.md` owns the start-of-cycle commands; do not duplicate them here.
`docs/P1_EXECUTION_BOARD.md` is the only dynamic queue (history in
`docs/optimization/archive/`); `Smash64DS_Runtime2_SwitchPlan.md` is the charter;
`docs/BUGS.md` carries the owner's verdicts — preserve their wording. A clean
checkout builds through `build.ps1`, not bare `make`: four of six `.inc` are
gitignored and **`build.ps1`'s generator is not run by `make`, so it can rot**.
`make p1-tick` builds the measuring ROM, `make p1` the published battle pair;
bare `make` builds the P2 ROM P1 does not ship. Never pass `-j`, never
override `MAKEFLAGS`, one build at a time, never build a
published target name for lab work. Preserve canonical mode 163, renderer mode 9,
mip 0, static textures, source countdown, Dream Land water frame 0, Task 16 `1/1/1`. Never edit `decomp/`. Run `New-Smash64DSSnapshot.ps1` last.
