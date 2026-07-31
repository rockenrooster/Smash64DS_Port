# Handoff

Updated: 2026-07-30. **Restart surface only, capped at 150 lines** — durable detail goes to its
owning doc (board: queue + results; `PERF_LEDGER.md`; `KNOWN_ISSUES.md`; `TASK_STANDING_RULES.md`).

| phase | state |
|---|---|
| R2-00a/b/c, R2-01, R2-02 | gated |
| R2-03 | shipped E12/E28/E29/E46/**E32**/**E64b**/**E65**/**E67**/**E69** — five gate levers, 1,228,928 -> 1,096,768, but all DLDI-off and so lower bounds; only the E32 flash residual is open (KNOWN_ISSUES) and it no longer blocks a lever |
| R2-04 | loading + rate clauses done (E5/E6/E57); budget clause closed by E64b+E65 |
| R2-05 | **COMPLETE** — reproducibility (E0) and zero fighter special cases (E1) |
| R2-06 | E0/E1/E2 + soak done; E4b/E6/E7/E11/**E13/E14/E15** refuted; **E8 finds the event, E10 + E17 both attribute it as SPREAD — no lever left inside the phase** |
| R2-07 | **results-flow clause MEETS ITS GATE** — R0..R4e, Results 39.975 -> 1.04 VBlanks/tic, 581,197 ticks = **0.52× of 1.12M**; R2b and R4c owner-approved and default-on. Particle/audio/HUD clauses untouched. **L6 names the gate lever: float collision (L7).** R2-08 needs the owner's retail play test |

## Where the gate stands — MISSED, and EVERY earlier number is DLDI-off

**`WORK-H` P95 ~1,160,448-1,179,520 / P50 ~976,000**, 8-9/128 over, evidence `r206-head-control-128`
and `r206-e11-control-128`. **The harness does not scatter, but identical source is not an identical
binary** — no reproducible ROM hash, and those two HEAD controls differ by **P95 +5,376, ±1 over-gate
frame**. That is the floor for any cross-build delta; always run the matched control (E11).

**DLDI-on costs ~29,696 P95 and is required for retail parity (owner), so it is the honest config.**
E69 rebuilt at its own commit `4916656d`, identical source, reads **1,126,464** DLDI-on vs the
**1,096,768** it published DLDI-off. **The gate was already missed by 6,464 before this session, the
23,232 margin was a DLDI-off artifact, and R2-07's particle budget was sized against it.** Read every
pre-`3eb9ecdb` P95 as a lower bound. **Never compare an anim-cache pair frame-by-frame** — it shifts
load timing; order stats only.

## OPEN P1: the over-gate frame is FLOAT COLLISION — L6 names the lever

**L6, 2026-07-31, in-run over-gate/clean split (`--split-over-gate`), no cross-build assumption.** An
over-gate frame does **+510,390 cycles of work** (2,536,136 vs 2,025,745) and overshoots the 2,240,760
gate by only **295,376 (147,688 ticks)**; **soft-float is +337,927 = 66.2% of the premium, LARGER than
the overshoot.** Entry-PC counts are binary over 10 frames/2 runs: `gmCollisionSetInvertMatrix` **34 on
every over frame, 0 on every clean one**, `func_ovl2_800ED490` 40/0, `fadd` 2.75x, `fmul` 3.62x.
`ndsRendererAdapterBuildDObjLocalMatrix` is **1.06x — render and animation are FLAT**; the reloc walker
is 0.5%, so **these are not load frames**. **L7 = fixed-point `gm/gmcollision.c`, and it must be ALL of
it**: measured, the float compose is 2,652 cyc/call vs the existing 20.12 one at 670.8 (**4.0x, not the
10x an ops model gives**), so compose+inverse alone save ~187,794 of a **295,376** overshoot — 64%, not
enough. Needs an E64b-style equivalence bound; collision decides hits. **L8 (unroll that compose) is
REFUTED UNBUILT** at ~12,000/frame, under E11's ~16,000 bar — its inner products are already unrolled
and 14 of 20 instructions per cell are the s64 round-and-clamp, not bit-exact to change, so it folds
into L7. **The census window is a COMPILE-TIME constant, so `-NoBuild` measures the LAST build's
window** — arm B did that and returned a byte-identical 7.4 MB CSV, one census labelled twice; the
harness now verifies `builds/$Build/nds_build_config.h` and refuses otherwise. Arm A alone would have
published "soft-float 9.95%", true of the whole run and 6.7x wrong about the excursion. **Always
confirm two deliberately different inputs give different outputs.**

**L2 stands and is corroborated: loads own only ~18% of over-gate frames — E8's headline is
CONTRADICTED.** Per-frame `gNdsTask75AssetLoadCount` (`-PerFrameGlobals`), inside one build: **5 of
28** (**4 of 28** against L1's list). Keep load elimination for R2-04 §3.8 (correctness), never as the
gate's answer. **E8's surviving numbers:** load frames P50 **1,113,152**; **clean frames P50 974,080,
P95 1,056,640 — inside the gate by 63,360**; average frame 145,920 under budget. **E7 refuted both
previously-named causes** (`FTR` −1,312 / `STG` −2,496 — not a render problem).
**Do NOT attack the relocation's O(n²) scan** — only 17.3% of it (n is 25.4); the payload is walked
TWICE by pointer *differences*, so hoisting to cache-store time is the viable shape (E9). **E10: no
single lever** — **326,906/frame** over **513 symbols**, relocation 37.0%, `ftAnimParseDObjFigatree` 13.0%.
**STOP ACCUMULATING SMALL LOAD-FRAME CUTS — E11 proves they cannot be banked**: real work removed,
**negative** bytes added, **bit-identical** load-frame set, yet **P95 +15,744, P99 +59,200, over-gate
9 → 11** — ~3× the +5,376 floor. **REVERTED.** Only a change clearing ~16,000 of tail movement, or one
that **moves work off the frame**, counts.

## OPEN P1: the freeze class is ROOT-CAUSED — heap OOM spins in the allocator

Owner: *"lots of freeze bugs that seem random"*. **One cause explains the class**
(`artifacts/verification/freeze-soak/2026-07-29_202114-*`): `decomp/src/sys/malloc.c:30` is
`while (TRUE);`, so the allocator **hangs instead of returning NULL** on exhaustion, and
`ndsR2AnimCacheStore` was a *speculative* cache calling it from a gameplay frame on the shared
`gSYTaskmanGeneralHeap`. **Do NOT make `syTaskmanMalloc` return NULL globally** — callers don't check.
**One site, not two — the "second exhaustion at battle start" was MY regression.** A static arena is
BSS, and BSS competes with the runtime `calloc` that sizes the heap: crossing the `0x130000` search
floor (`diagnostics.c:7403`) costs **196,608 bytes in one step**. **The arena now lives on the taskman
heap, 92,160 bytes, +32 B of BSS**, using 87,824. **Do NOT lower that floor** — a contract with the
Task 36 replay guard (`nds_renderer.h:124-134`); my earlier authorization is retracted. **Both
configurations complete a full match clean; no passive soak reaches match two** (`mnVSResultsCheckExit`
needs `START`).

**SECOND ENTRY REPRODUCED, matched pair (2026-07-31)** —
`artifacts/visibility/2026-07-31_second-entry-{A-match1-clean,B-rematch-corrupt}.png`: same
ROM/config, `STG` **171,328 -> 378,880 (2.21x)**, `ALL` 1,119,808 -> 1,679,936. Match two is otherwise
CORRECT (GO!, fresh `TIME 01:00`, both fighters, both stock displays) and the texture-name fixes
ENGAGED with **0** violations — **stale texture names are FIXED; stage corruption is the open root
cause**, and the GObj/capture layer is EXONERATED, so it is downstream in geometry/material/matrix
data. **The ~119 KB "SD setup excess" is WITHDRAWN** — a measured high-water minus a derived peak
demand from a 192 KiB-poorer config; SD setup is in fact **19,248 LOWER** than match one (906,568 vs
925,816 from an identical 319,968 baseline). **Sudden Death reproduces in ~90 s**
(`capture-sudden-death-entry.ps1`); **never pause the Fox CPU to force a tie** — it freezes the tic
source. Freeze DISPLACED not fixed (`BOUND-HITS` 0); keep the guard. MObj chains never invalid.
**`syDebugPrintf` covers only the ten `taskman.c` give-up sites. Diag lanes rebuild the ROM and used
to DROP `NDS_R2_SECOND_ENTRY_DIAG`; gdb aborts a command file on a missing symbol SILENTLY, which
reads as an emulator hang — pass `-SecondEntryDiag`.** Open items in BUGS.md.

## VS Results is CLOSED — 581,197 ticks/tic vs 1.12M (0.52×), from 2,814,955

R0c/R0d/R0e/R2a/R4b/R4d/R4e/R4c all graduated, all Latest-green; per-lever detail in the board and
PERF_LEDGER. **Results is no longer the P1 performance problem, and R1's "~30 s dead air" is 6.10 s
and is NOT a load** — the fighter reload is 5.5% of it, so do not build a residency system. Standing
traps it paid for, all still live:
- **`-Os` emits `blx __udivsi3` for a CONSTANT divisor** — whole-repo hazard, `grep __udivsi3`.
- **A lab ROM can differ from battle in CODEGEN.** The `-marm` rule keyed on harness ID 163, so the
  Results lab (164) built `nds_renderer.o` `-mthumb` and every 20.12 multiply was `bl __aeabi_lmul`;
  every pre-R4e Results absolute was inflated. Now `NDS_ARM_RENDERER_HARNESS_IDS` — add new lab IDs.
- **Measure Results with `scripts/census-results-frame-cost.ps1`**: tick-HUD buckets are zeroed only in
  the battle loop, so on Results they free-run — **difference two stops and divide by `sTicks`**.
- **Compare captures with `scripts/compare-capture-pair.ps1`** — it crops to the guest viewport, past
  melonDS's host-FPS title bar, which otherwise reads as a visual regression whenever a candidate is
  faster. The wallpaper mapper **letter-boxes** any non-(0,0) origin, and a faster candidate lands on
  a **different scene tick** at the same wall clock.

## R2-06 IS CLOSED — no lever left inside the phase; L6 found the one outside it

**The animation body is CLOSED — three levers, three refutations, zero builds spent** (E13 f = 0.840;
E14 ~2,900, **10-15 ticks/32-byte line fill**, no prefetcher; E15 straddles the floor).
`gNdsFighterInit*` is proof-build-only, **0 — never cite it.** **Every named load-frame candidate is
sized and none closes the 40,448**: of 139,072/load-frame, ~94,127 (67.7%) has no named owner. Anim
cache CLEAN, `FTR`/`STG` flat (L0), so the excursion is `SRC` — which L6 has now resolved. **Never
target `HUD`** — it holds `ndsPlatformRenderDebugHud`, the tick HUD's own render, which is why
`WORK-H` subtracts it per sample.
**The one open fidelity item: E32** — a **generator gap, not a decision** (E62); no longer blocks a
gate lever (E7). Mario draws *unflashed*, pixel-identical on non-flash frames. **Generator must bake
the flash colours** (E63: 2,164 B).

## Refuted this cycle — do not re-derive

All by measurement, derivations on the board: **E51** `line_id`; **E53** `{base,size}` mirror; **the
flash as vertex data** (E48-E58); **the pose table** (E61); **`.text.hot`** (E66); **E57** hitboxes
walk the live joint chain; **R2-06 E7** fighter fallback + Task 39; **R2-06 E6** the Horner fold (so
E61 §5's other rows are suspect — **a memo is a memory stream**); **pointer arrays as index
arithmetic** (E11); **an AObj pool** (E12); **R2b's transform double-apply**; **R1's loader AND arena
framings**; **the OOM spin and the DL-buffer overflow as the Sudden Death freeze**.

## Restart

Branch `codex/r2-runtime2`, not merged to master. Boundary `battle_playable_realtime`, mode `163`. Lab build dirs hold R2b/R1 ROMs; rebuild before citing a hash.
**PARTICLE-BANK WIP IS ON TWO BRANCHES, NOT IN ANY WORKTREE** — `worktree-agent-a15dedc9b2cf19349`
(generator + checker) and `worktree-agent-a8c9ad131bc0073b0` (`battleship_lbparticle.c`, runtime header,
`gbi.h`/shim). **Unreviewed, unbuilt; start from these, do not rewrite** — BUGS.md's four VFX rows and
the R2-07 gate need them. Cleanup detail in KNOWN_ISSUES.md.

```powershell
$env:DEVKITPRO = 'C:/devkitPro'; $env:DEVKITARM = 'C:/devkitPro/devkitARM'
.\scripts\verify-all.ps1 -Profile Boundary -List; git status --short
```

**Do not rebuild `smash64ds.nds`** (owner, 2026-07-28); **do rebuild the tick-HUD ROM whenever the
published one is** (owner, 2026-07-22), flag-identical. `-j`/`MAKEFLAGS` rules are in AGENTS.md
`## Builds`. A clean checkout must build through `build.ps1`, not bare `make`: four of six generated
`.inc` files are gitignored and only `build.ps1` regenerates them. Preserve canonical mode 163,
renderer mode 9, mip 0, static textures, source countdown, Dream Land water at frame 0, Task 16
`1/1/1`. Do not edit `decomp/`. **Bug #10 is CLOSED** (owner, 2026-07-30) — see `PORTING.md`.
