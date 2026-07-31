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
| R2-07 | **results-flow clause MEETS ITS GATE** — R0..R4e, Results 39.975 -> 1.04 VBlanks/tic, 581,197 ticks = **0.52× of 1.12M**; R2b and R4c both owner-approved and default-on. Particle/audio/HUD clauses untouched; **Sudden Death is now in P1 and has no deterministic entry** (needs a seeded harness mode). R2-08 needs the owner's retail play test |

## Where the gate stands — MISSED, and EVERY earlier number is DLDI-off

**`WORK-H` P95 ~1,160,448-1,179,520 / P50 ~976,000**, 8-9/128 over, evidence `r206-head-control-128`
and `r206-e11-control-128`. **The harness does not scatter, but identical source is not an identical
binary** — no reproducible ROM hash, and those two HEAD controls differ by **P95 +5,376, ±1 over-gate
frame, a 1-frame window shift**. That is the floor for any cross-build delta; always run the matched
control (E11).

**DLDI-on costs ~29,696 P95 and is required for retail parity (owner), so it is the honest config.**
E69 rebuilt at its own commit `4916656d`, identical source, reads **1,126,464** DLDI-on vs the
**1,096,768** it published DLDI-off. **The gate was already missed by 6,464 before this session, the
23,232 margin was a DLDI-off artifact, and R2-07's particle budget was sized against it.** Read every
pre-`3eb9ecdb` P95 as a lower bound.

**Never compare an anim-cache pair frame-by-frame** — it shifts load timing; order stats only.

## OPEN P1: every over-gate frame is an ASSET-LOAD frame, and clean P95 MEETS THE GATE

**E8.** The Task 75 ring marks the frames running `ndsRelocFinalizeLoadedFile`
(`reloc_backend_assets.c:3398`): **16 of 128, and 8 of the 9 over-gate frames are among them.** Load
frames P50 **1,113,152**; **clean frames P50 974,080, P95 1,056,640 — inside the 1,120,000 gate by
63,360**, 1 of 112 over. The +139,072 premium is entirely `SRC`. **The average frame is already
145,920 under budget.** **E7 refuted both previously-named causes**; on over-gate frames `FTR` is
**−1,312** / `STG` **−2,496** — not a render problem — and **E32's flash residual no longer blocks a
gate lever.**

**Inside the relocation, do NOT attack the O(n²) scan** — only 17.3% of it (n is 25.4). The shape is the
**payload walked TWICE** using pointer *differences* only, so hoisting to cache-store time is viable (E9).
**E10 ANSWERED the frame-wide premium and there is NO single lever:** **326,906/frame** spread over
**513 symbols carrying 349,268 — fully attributed**, relocation 37.0%, `ftAnimParseDObjFigatree` 13.0%.

**STOP ACCUMULATING SMALL LOAD-FRAME CUTS — E11 proves they cannot be banked.** It removed real work
(`ndsRelocAssetIDForToken`) with **negative** bytes added and a **bit-identical** load-frame set, yet
**P95 +15,744, P99 +59,200, over-gate 9 → 11** — ~3× the +5,376 cross-build floor. **REVERTED.** Only
a change clearing ~16,000 of tail movement, or one that **moves work off the frame**, counts.

## OPEN P1: the freeze class is ROOT-CAUSED — heap OOM spins in the allocator

Owner: *"lots of freeze bugs that seem random"*. **One cause explains the class**
(`artifacts/verification/freeze-soak/2026-07-29_202114-*`): `decomp/src/sys/malloc.c:30` is
`while (TRUE);`, so the allocator **hangs instead of returning NULL** on exhaustion, and
`ndsR2AnimCacheStore` was a *speculative* cache calling it from a gameplay frame on the shared
`gSYTaskmanGeneralHeap`. **Do NOT make `syTaskmanMalloc` return NULL globally** — decomp callers do
not all check.

**One site, not two — the "second exhaustion at battle start" was MY regression.** A static arena is
BSS, and BSS competes with the runtime `calloc` that sizes the heap: crossing the `0x130000` search
floor (`diagnostics.c:7403`) costs **196,608 bytes in one step**. **The arena now lives on the taskman
heap, 92,160 bytes, +32 B of BSS**, using 87,824. **Do NOT lower that floor** — it is a contract with
the Task 36 replay guard (`nds_renderer.h:124-134`); my earlier authorization is retracted. **Both
configurations complete a full match clean.** **No passive soak reaches match two** —
`mnVSResultsCheckExit` needs `START`.

**Sudden Death: REPRODUCIBLE in ~90 s, and its freeze is FIXED** —
`scripts/capture-sudden-death-entry.ps1`, no harness mode (a mode would have to fork source control
flow; withdrawn). A match nobody scores in IS a 0-0 tie, so it runs mode 163 with Fox **live** and
shortens `time_remain`. **Never pause the Fox CPU to force a tie** — that flag also freezes the tic
source, so the clock never expires. The freeze was an unbounded WRITE walk past a bounded counting
walk (`reloc_backend_renderer_dl.c:7850`, 4-entry array); bounding it took 1 distinct frame per 60 s
to 12, Latest green. **A `syDebugPrintf` breakpoint is a total detector for this bug class** — all
eleven `while (TRUE);` give-up sites print first. Open items in BUGS.md. **The R2 anim cache is
CLEAN** (Rejects 0, Overflows 0) — the 109/109 reading was stale; see board R2-07 L0.

## VS Results is CLOSED — 581,197 ticks/tic vs 1.12M (0.52×), from 2,814,955

R0c/R0d/R0e/R2a/R4b/R4d/R4e/R4c all graduated, all Latest-green; per-lever detail in the board and
PERF_LEDGER. **Results is no longer the P1 performance problem, and R1's "~30 s dead air" is 6.10 s
and is NOT a load** — the fighter reload is 5.5% of it, so do not build a residency system.

Standing traps it paid for, all still live:

- **`-Os` emits `blx __udivsi3` for a CONSTANT divisor** — whole-repo hazard, `grep __udivsi3`.
- **A lab ROM can differ from battle in CODEGEN.** The `-marm` rule keyed on harness ID 163, so the
  Results lab (164) built `nds_renderer.o` `-mthumb` and every 20.12 multiply was `bl __aeabi_lmul`.
  Every pre-R4e Results absolute was inflated. Keyed on `NDS_ARM_RENDERER_HARNESS_IDS` now — put a
  new latency-surface harness ID in that list.
- **Measure Results with `scripts/census-results-frame-cost.ps1`**: the tick-HUD buckets are zeroed
  only in the battle loop, so on Results they free-run and must be **differenced across two stops and
  divided by `sTicks`**, never read once and divided by a scene clock.
- **Compare captures with `scripts/compare-capture-pair.ps1`** — it crops to the guest viewport,
  because melonDS's title bar carries a host-FPS readout that changes whenever a candidate is faster
  and otherwise reads as a visual regression.
- The wallpaper mapper **letter-boxes** any origin that is not (0,0), and a faster candidate lands on
  a **different scene tick** at the same wall clock.

## NO LEVER LEFT INSIDE R2-06 — the premium has now refused to concentrate TWICE

**The animation body is CLOSED — three levers, three refutations, zero builds spent.** 146,148/frame,
46,148 over §4's 100K, none reachable: **E13** pose-fewer-joints refuted (collision's ancestor closure
is **f = 0.840** of live joints); **E14** reorder refuted at ~2,900 (**10-15 ticks/32-byte line fill**,
no prefetcher; `AObj` 36 B, N 221 — both memos wrong); **E15** unbuilt, straddling the floor.
`gNdsFighterInit*` is proof-build-only, **0 by construction — never cite it.**

**Every named load-frame candidate is sized and none closes the 40,448.** Of the 139,072/load-frame
premium: relocation 33,632, re-add 11,313, E9's walks 21,788 — **~94,127 (67.7%) has NO named owner**,
and E17 killed E8's hypothesis (16 load frames, only 7 whole-GObj re-adds). Two independent
attributions say spread. **R2-07 L0 moved this on: the anim cache is clean, and the unattributed
spread shows up as `HUD` P50 1,024 / P95 346,816 — a 338x bucket nobody has priced. Attribute that
next (board R2-07 L0/L1), not a fourth bracket here.**

## The one open fidelity item

- **E32** — a **generator gap, not a decision** (E62), and E7 showed it no longer blocks a gate lever.
  Mario draws *unflashed* — not corrupt, pixel-identical on non-flash frames. **Needs the generator to
  bake the flash variant's colours** (E63: 2,164 B). Derivation on the board.

## Refuted this cycle — do not re-derive

All by measurement; derivations on the board. **E51** `line_id` table; **E53** `{base,size}` mirror;
**the flash as vertex data** (E48-E58); **the pose table** (E61); **`.text.hot`** (E66); **R2-04 E57**
hitboxes walk the live joint chain; **R2-06 E7** fighter fallback and Task 39 effects; **R2-06 E6**
the Horner fold (so E61 §5's other rows are suspect — a memo is a memory stream); **pointer arrays as
index arithmetic** (E11); **an AObj pool** (E12); **R2b's transform double-apply**; **R1's loader AND
arena framings**; and **the `syMallocSet` OOM spin and the DL-buffer overflow as the Sudden Death
freeze** (a `syDebugPrintf` breakpoint catches all eleven give-up sites; none fired).

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
