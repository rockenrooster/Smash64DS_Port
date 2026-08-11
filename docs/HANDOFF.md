# Handoff

Updated: 2026-08-10. **Requirement 4 shipped: the fighter `AObj` is fixed point,
`WORK-H` P50 −23,360 / P95 −37,504 on one binary.** Before it, the gate arm's
tail was cartridge I/O — the animation cache arena was full and refusing loads
all match, fixed at `f082b3c8`: `WORK-H` P95 1,639,299 → 1,447,318.
**Every 128-frame measurement in the archive is unusable** — that window reads
the cheapest 6% of the match, understating P95 ~306,000 and the over-gate rate
five times. Use `sample-tick-hud-buckets.ps1 -Samples 1600`.

## The two baselines — label every figure with its arm AND its coverage

Both arms run the **same 60-second match** (coverage 86.7%), windows ending 43
frames past the buzzer. Slips 0 in every row.

| arm | role | `WORK-H` P50 | P95 | over gate |
|---|---|---:|---:|---:|
| **both-CPU** | **THE GATE** | 970,112 | **1,310,528** | slice 28; +31 route |
| **Boundary** mode 163 | shipped configuration | 920,192 | 1,113,408 | re-banked c116 |

**Gate 1,305,472**, re-banked after slice 31. Every c117 slice is UNDER the
±8,544 floor cross-build; 1,317,440 → 1,305,472 is drift, not banked wins.
**Gap ~185,472.** Both rows are
current; re-bank before judging a new slice. The soak's long match is
`NDS_R2_SOAK_MATCH_MINUTES`; `probe-match-window.ps1` reads the match timer from
the guest, so a window cannot claim coverage it lacked. Owner's bar: the whole
match under P95 on the both-CPU config, loading excluded; the shipped ROM stays
the Boundary hwtri pair. `Makefile:305-308` forbids reporting a both-CPU P95 as
the Boundary figure; **re-pin `EXPECTED_CENSUS_SHA256` when its coverage changes.**
**Route to ATTRIBUTE, re-bank to BANK — never swap them.** Slice 31 read P95
**−7,104** on one binary (`ALL` exactly 0) and **+576** across builds (untouched
FTR −3,328). Both true: the work is gone, the win is under the floor.
**Collision: cut VISITS, not ops** — slice 28 skips 91.9% of sweep visits (`SPHD` P95 −15,744); `ndsMPFindLineEndpoints` 13,205 cyc/frame has no reject.

## What is dead, so nobody re-derives it

- **Effect DObj submits** — Boundary-only: 99.3% of the Boundary excursion but
  **~12.1%** of the gate arm's; G3 refuted cycles 88–91. **Projectiles**
  (44 tk/fr) · **Particles** (flat ~47,000, a P50 lever only, retiring SwitchPlan
  §7 option 2 as a *gate* answer) · **texture thrash** · **`Find`** ·
  **`Material`** · **the force-load seam**.
- **`FTR` as the *P95 discriminator*** (+13,768). NOT "FTR is exhausted" —
  reading it that way is what the owner re-opened 2026-08-10; cycles 110–116 then
  took 24.3% off it. `FTR` is **flat**, on nearly every frame, which is why.

## RAM: both budgets are near their floor — price a change before writing it

- **Static/boot.** `check-boot-headroom.ps1 -Build <dir>` after every lab build.
  Highest `fake_heap_start` proven to boot **`0x02294804`**, lowest proven to
  fail **`0x02294b24`**. **Text counts as much as bss**; a failing arm reads as a
  hung emulator. **`gSYTaskmanGeneralHeap`** free-min **42,136** against the anim
  cache's 32,768 `KEEP_FREE`, coupled since freeing `.bss` enlarges it.

## FTR is re-opened and moving: −93,612 landed (cycle 116)

**Banked `FTR` mean is 291,896** (P50 301,760, P95 304,768) against a pre-slice
baseline of **385,508** built for the purpose, which equals the owner's stated
~385–390K. **−93,612, 24.3%** — the *mean* is under the owner's 300K target, the
P50 is 1,760 over it. Measured on the SHIPPED (fixed) strips; the −94,666 quoted
earlier came off the build that was losing geometry, so it is withdrawn.
Boundary passes on every landed slice.

**The big one is DS-native AOT geometry: Task 56 fighter strips now SHIP**
(`NDS_TASK56_FIGHTER_PRIMITIVES ?= 2`). 626 triangles submitted as 1,878
`GL_TRIANGLES` corners are now **1,014 strip corners in 163 groups**, compiled
host-side. One-binary A/B on `gNdsR2FighterStripRoute` (identical `romSha256`,
route read back 0 and 1): **`FTR` P50 −10,432, P95 −10,368, `WORK-H` P50
−10,112, P95 −14,144**; `STG` +64 and `ALL` identical to the tick are controls.

**It SHIPPED BROKEN once and the owner caught it in minutes — read the board
before touching this.** Two defects, invisible to every gate that passed:
`_stripify_run` used `(t0[0], t0[2])`, not a directed edge, so **35.6% of the
fighter came out BACKFACING**; and the emitter issued `BEGIN_VTXS` only on a
group TYPE change, **welding adjacent strip groups into one vertex list** (the
first run has six consecutive strips). `check_fighter_primitive_streams.py` now
**models the runtime's BEGIN policy instead of assuming one per group** — that
assumption is what let the second ship green; under the old policy it reports
mode 2 drawing **744 triangles against 626 source**. Run it after touching
either. **A passing verifier is not visual verification**: Boundary passed on
the broken build and `latest.png` showed both fighters complete, because that
canonical frame does not show the affected joints. **Hand the owner a ROM.**

**The emit stalls per VERTEX, not per word** (c115 `--pc-detail`, no build): a
corner is 40.5 cycles untextured, **~28 of it the GX write**, and the textured
path pays the same ~28 for a fourth word. That is why strips work, and it
**refutes a baked/DMA'd packed GX stream and `VTX_10`** — both trade words, not
vertices; the win is ~50% of the vertex prediction, so part is **per polygon**.
**The prepared dense UVs are immutable state too** (−1,744; 15 runs a match).

**The other big lever was the I-cache, not arithmetic.** `…DLAllDrawForSlot`
was the ROM's largest non-idle symbol at **4.21 cyc/insn**, 10,708 bytes against
an **8 KB** I-cache, **73.6% never executed**; outlining the never-*entered*
bodies took it to **7,516**/**7,236**. Recipe, no build: `--pc-detail SYM`, diff
`objdump`, `addr2line` four points per cold run. **Entry count discriminates, not
cold bytes** — cold bytes in an *entered* body cost **+14,963**; under 8 KB, spent.

Sixteen landed slices; the board carries each one's evidence. What generalises:

- **The DObj world cache had ZERO readers** while `Store` burned 4,744,740
  cycles and ~4 KB a frame through a 4 KB D-cache. **Ask what reads a cache
  before optimising what fills it**, and **read the counters a previous cycle
  left** — the UV proof and Requirement 4's sizing were both already in-tree.
- **The compose does not fold its base in until a joint contributes** (−10,804):
  one call per binding was *copy the base in, multiply it straight back out*.
- **The material block is built 30 times a match, not 59,392** — a (MObj, heap
  gen, animatable-input hash) key in a row owned by the material **DObj**.
  `BindingParents` is the nearest *bound* ancestor, not the DObj parent.
- **A per-PC census charges a miss to the instruction that TAKES it**, not to the
  work you can delete: two redundant first-reader passes came out for **+1,055**,
  because the data was still consumed and the fills only moved.

**Compiling the frame-summary counters out is refuted** (FTR −7,378 / STG
−2,776): it **breaks the gate**, since `…gcrunall-loop-harness.ps1` asserts exact
batch and texture-prepare accounting off those globals. **A census row is not an
FTR row** either — the bracket is `ndsFighterDisplayContractSubmit` only.

**Reconciliation on c115 with an INDEPENDENT tick factor** (0.4993 tk/cyc, from
`ALL` vs total cycles — deriving it from the FTR sum is circular and overstated
coverage 22%): **35 named symbols = 244,774 tk/fr, 78.1% of `FTR`**; the 68,647
residual is bounded by shared leaves (float lib 66,750, `memcpy`/`memset`
29,895, binds 22,596, whole-frame).

**Next, priced** (c115 census, tk/fr). **`Task36ReplayRun` 17,796 is STAGE, not
FTR** — it takes `NDSNativeStageRun`; the board's old "next slice" row was
mis-attributed. In FTR: `ExecuteNativeFighterOwnerProduction` **26,307** +
`NativePrepareProductionRun` **25,277** (five-phase split on the board, Uv
quarter deleted); `BuildFighterTraRotRpyDirect` **17,698** (already all
fixed-point inside — its only float boundary is six conversions a joint);
`BuildDObjXObjMatrix` 14,244; `LoadHardwareSplitMatrices` **13,122**, E23
projection-skip still refuted. The animation lane above them is spent.

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
  **`ALL` is VBlank-quantized** — it hid a +52,928 once and it hid the strips
  entirely (P50 identical, the saving reappearing as `WAIT`). And **1.85 cycles
  of `FTR` mean per byte of added ARM text** — beat your footprint.
- **Disassemble the loop, read the caller, TAKE THE ENTRY-PC COUNT, and check
  WHICH bucket the symbol is in, before designing around it.** Cycle 108 built a
  loader `ftmain.c` discards; c109 aimed a `FTParts` fix at two `DObj` fields;
  c110 divided a symbol total by a guessed per-call cost; c116 found the board's
  named next slice was a STAGE symbol. All free (`--pc-detail`). **Resolve line
  numbers against the build's own `NDS_TASK10_GIT_SHORT`**, not HEAD — c106
  against HEAD was ~85 lines adrift.

## Restart surface — parked items live on the board's **Parked** list

```powershell
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

`docs/P1_EXECUTION_BOARD.md` is the only dynamic queue (history in
`docs/optimization/archive/P1_EXECUTION_BOARD_pre-cycle79.md`);
`Smash64DS_Runtime2_SwitchPlan.md` is the charter; `docs/BUGS.md` carries the
owner's verdicts — preserve their wording. A clean checkout must build through
`build.ps1`, not bare `make`: four of six generated `.inc` files are gitignored,
and **`build.ps1`'s generator is not run by `make`, so it can rot unnoticed —
it had, until c116**. `make p1-tick` builds the measuring ROM, `make p1` the
published battle pair; bare `make` builds the P2 ROM P1 does not ship. Never
pass `-j`, never override `MAKEFLAGS`, one build at a time, never build a
published target name for lab work. Preserve canonical mode 163, renderer mode
9, mip 0, static textures, source countdown, Dream Land water frame 0, Task 16
`1/1/1`. Never edit `decomp/`. Run `New-Smash64DSSnapshot.ps1` last.
