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
| R2-07 | **results-flow clause STARTED** — R0..R2b, Results 39.975 -> 7.025 VB/iter (5.7×), still 3.5× out; **R2b default-off pending visual approval**. Particle/audio/HUD clauses untouched. R2-08 needs the owner's retail play test |

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
configurations complete a full match clean.** Four detector defects fixed, two verdicts withdrawn (it
hashed the window **title**, where melonDS renders its FPS counter). **Sudden Death has its own
issues** (owner). **No passive soak reaches match two** — `mnVSResultsCheckExit` needs `START`.

## OPEN P1: VS Results — R0..R2b CUT 82.4%; 3.94M ticks/frame vs 1.12M. R2b IS GRADUATED

**39.975 → 7.025 VB/iter (1.50 → 8.43 FPS, 5.7×), Latest green.** Bit-exact cuts proven by
`check_sprite_lerp_exact.py` — **R0c** `/255`→`(x*257+257)>>16` (**`-Os` emits `blx __udivsi3` for a
CONSTANT divisor**, whole-repo hazard, `grep __udivsi3`); **R0d** inlined lerp; **R0e** 16-entry
palette, ONE byte per PAIR of columns, 112 → 9 Thumb instr/px; **R2a** glyphs index that table
(−200,133/frame, FPS flat, banked slack per rule 12). Details in PERF_LEDGER.

**R2b is the big one and it is now DEFAULT-ON.** The owner named the lever
(affine BGs), so R2 became the **hardware affine BG**, not the planned dirty-flag.
`NDS_R2_RESULTS_AFFINE=1`: **10.125 → 7.025 VB/iter, −1,736,589 ticks/frame** vs the 1,746,558 R0h
sized for the whole background layer (**0.6%**), and the `I/4b 300x220` census row is **gone**.
Confirmed quantum-free by `soak-freeze-watch.ps1` as a wall-clock race: `gNdsVSResultsTickCount`
**500 → 780 (+56%)**, both arms NO-FREEZE, both `PacingPresentedFrames = 2043` (battle unperturbed).
**Owner approved the matched-tic pair on sight (2026-07-30): `NDS_R2_RESULTS_AFFINE ?= 1`, and the
checker now pins it default-ON.** It is inert wherever `NDS_FAST_WALLPAPER_AFFINE` is 0 (seed capture
compiled out), which is every target except the differ. Two traps already paid for: the mapper
**letter-boxes** any wallpaper whose origin is not (0,0), and a faster candidate lands on a
**different scene tick** at the same wall clock (both now standing rules). Idle was **830,260 = 1.48
VBlanks of slack**, which R2b's 3.1 exceeded. **RETRACTED: `IA/8b 24x37` is NOT 85.1% of the
remainder** — that row is a next-hit interval spanning commit, fighters, camera/display and the
platform wait, so it locates the unpartitioned tail and attributes nothing. The post-R2b owner is
unmeasured; the census that settles it is the next step.

**R1 is ANSWERED and folded into R2 — the "~30 s dead air" is 6.10 s and is NOT a load.** The 30 s was
captured before R0c at 39.975 VB/iter and never re-measured after the 3.9×. GAME SET → "FOX WINS" is
**6.10 s control, 4.85 s with R2b**; the transition is 0.735 s (12%), of which the two
`ftManagerSetupFilesAllKind` calls are 0.334 s — **the fighter reload is 5.5% of the dead air, so do
not build a residency system for it.** The player waits on the source's own schedule (wallpaper held to
tic 80, panels to tic 120, `mnvsresults.c:2843-2844`) at the scene's per-frame cost; residue is near
the ~4 s floor a 30 Hz port must pay. Both earlier framings — loader, then arena lifecycle — refuted.

## NO LEVER LEFT INSIDE R2-06 — the premium has now refused to concentrate TWICE

**The animation body is CLOSED — three levers, three refutations, zero builds spent.** 146,148/frame,
46,148 over §4's 100K, none reachable: **E13** pose-fewer-joints refuted (collision's ancestor closure
is **f = 0.840** of live joints); **E14** reorder refuted at ~2,900 (**10-15 ticks/32-byte line fill**,
no prefetcher; `AObj` 36 B, N 221 — both memos wrong); **E15** unbuilt, straddling the floor.
`gNdsFighterInit*` is proof-build-only, **0 by construction — never cite it.**

**Every named load-frame candidate is sized and none closes the 40,448.** Of the 139,072/load-frame
premium: relocation 33,632, re-add 11,313, E9's walks 21,788 — **~94,127 (67.7%) has NO named owner**,
and E17 killed E8's hypothesis (16 load frames, only 7 whole-GObj re-adds). **Second time a premium
here refused to concentrate. Before a fourth bracket, ask whether R2-06 is the right phase** — two
independent attributions say spread, pointing at the switch plan's §3 structural change.

## The one open fidelity item

- **E32** — blocked on a **generator gap, not a decision** (E62), and E7 showed it no longer blocks a gate
  lever. The flash clears `G_LIGHTING` and draws vertex colours raw; the owner hardware-lights with stale
  diffuse/ambient, so Mario draws *unflashed* — not corrupt, pixel-identical on non-flash frames. E49's
  runtime half **refuted** (the baked `.rgba` holds **normals** — speckle). **Needs the generator to
  bake the flash variant's colours**; E63: 2,164 B.

## Refuted this cycle — do not re-derive

All by measurement, not opinion; derivations on the board. **E51** `line_id` table (`YakumonoCount =
1`); **E53** `{base,size}` mirror (P95 **+11,584**); **the flash as vertex data** (E48-E58); **the pose
table** (E61, 2.62 MB vs 4 MB RAM); **`.text.hot`** (E66, +24,448); **R2-04 E57** hitboxes walk the
live joint chain; **R2-06 E7** fighter fallback (0/256) and Task 39 effects; **R2-06 E6** the Horner
fold (+7,168 P50, so E61 §5's other rows are suspect — a memo is a memory stream); **Mario/Fox pointer
arrays as index arithmetic** (E11); **an AObj pool** (E12, `syMallocSet` already bumps); **R2b's
transform double-apply** (the mapper bakes it); and **R1's loader AND arena framings** (the dead air
is per-frame cost).

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
