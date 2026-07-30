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

**`WORK-H` P95 ~1,160,448-1,179,520 / P50 ~976,000**, 8-9/128 over, evidence
`r206-head-control-128` and `r206-e11-control-128`. **The harness does not scatter, but identical
source is not an identical binary** — the build has no reproducible ROM hash, and those two HEAD
controls differ by **P95 +5,376, ±1 over-gate frame, and a 1-frame window shift**. Treat that as
the floor for any cross-build delta, and always run the matched control (E11).

**DLDI-on costs ~29,696 P95 and is required for retail parity (owner), so it is the honest
config.** E69 rebuilt at its own commit `4916656d`, identical source, reads **1,126,464**
DLDI-on vs the **1,096,768** it published DLDI-off. **The gate was already missed by 6,464
before this session, the 23,232 margin was a DLDI-off artifact, and R2-07's particle budget
was sized against it.** Read every pre-`3eb9ecdb` P95 as a lower bound.

**Residual SD reads refuted without a build** (cache `Hits=79 Misses=2` of 81); `SRC +30,912` withdrawn.
**Never compare an anim-cache pair frame-by-frame** — it shifts load timing; order stats only.
## OPEN P1: every over-gate frame is an ASSET-LOAD frame, and clean P95 MEETS THE GATE

**E8.** The Task 75 ring marks the frames running `ndsRelocFinalizeLoadedFile`
(`reloc_backend_assets.c:3398`): **16 of 128, and 8 of the 9 over-gate frames are among them** (842,
the exception, is adjacent to load frame 843). Load frames P50 **1,113,152**; **clean frames P50
974,080, P95 1,056,640 — inside the 1,120,000 gate by 63,360**, 1 of 112 over. The +139,072 premium
is entirely `SRC`; frame 909 is 1,617,152, the event E53 profiled. **The average frame is already
145,920 under budget.** **E7 refuted both previously-named causes** (see "Refuted this cycle"); on
over-gate frames `FTR` is **−1,312** / `STG` **−2,496**, so it is not a render problem, and **E32's
parked flash residual no longer blocks a gate lever.**

**Inside the relocation, do NOT attack the O(n²) scan** — only 17.3% of it (n is 25.4). The shape is the
**payload walked TWICE** using pointer *differences* only, so hoisting to cache-store time is viable (E9).
**E10 ANSWERED the frame-wide premium and there is NO single lever.** Work premium **326,906/frame**
(after removing `armWaitForIrq` 247,439 of quantization slack and 45,917 of tick-HUD printf), spread over
**513 symbols carrying 349,268 — fully attributed.** Relocation 37.0%, `ftAnimParseDObjFigatree` 13.0%.

**STOP ACCUMULATING SMALL LOAD-FRAME CUTS — E11 proves they cannot be banked.** E11 took E10's cleanest
lever (`ndsRelocAssetIDForToken`, 630 calls on load frames, **0 on all 112 clean frames**), removed the
work provably with **negative** bytes added, measured it at **31,808 (−7,667, −5,103 insns)** and the
load-frame set **bit-identical** — yet **P95 +15,744, P99 +59,200, over-gate 9 → 11**, the two added
frames being **load** frames 828/847. Two HEAD controls differ by P95 +5,376, so that is ~3x noise.
**REVERTED.** A ~8,000 load-frame saving cannot survive relinking; only a change clearing ~16,000 of tail
movement, or one that **moves work off the frame**, counts. Guard: `reloc_backend_assets.c:1796`.

## OPEN P1: the freeze class is ROOT-CAUSED — heap OOM spins in the allocator

Owner: *"lots of freeze bugs that seem random"* + *"sometimes hitting a shielded player causes a
freeze"*. **One cause explains the class** (`artifacts/verification/freeze-soak/2026-07-29_202114-*`):
`decomp/src/sys/malloc.c:30` is `while (TRUE);`, so the allocator **hangs instead of returning NULL**
on exhaustion, and `ndsR2AnimCacheStore` was a *speculative* cache calling it from a gameplay frame
on the shared `gSYTaskmanGeneralHeap`. Chain: damage-fall → aerial interrupt →
`ftMainSetStatus(213)` → on-demand `FTMarioAnimAttackAirD` → `syTaskmanMalloc(3472)` → spin. **Do
NOT make `syTaskmanMalloc` return NULL globally** — decomp callers do not all check.

**One site, not two — the "second exhaustion at battle start" was MY regression.** A static arena is
BSS, and BSS competes with the runtime `calloc` that sizes the heap: crossing the `0x130000` search
floor (`diagnostics.c:7403`) costs **196,608 bytes in one step**. **The arena now lives on the taskman
heap, 92,160 bytes, +32 B of BSS**, using 87,824. **Do NOT lower that floor** — it is a contract with
the Task 36 replay guard (`nds_renderer.h:124-134`); my earlier authorization is retracted. **Both
configurations complete a full match clean.** Four detector defects fixed, two verdicts withdrawn (it
hashed the window **title**, where melonDS renders its FPS counter). **Sudden Death has its own
issues** (owner). **No passive soak reaches match two** — `mnVSResultsCheckExit` needs a
`START_BUTTON` tap; soaks default 2.5 min, ceiling 5.

## OPEN P1: VS Results — R0..R2b CUT 82.4%; 3.94M ticks/frame vs 1.12M. R2b NEEDS VISUAL APPROVAL

**39.975 → 7.025 VB/iter (1.50 → 8.43 FPS, 5.7×), Latest green.** Bit-exact cuts, proven by
`check_sprite_lerp_exact.py`: **R0c** `/255`→`(x*257+257)>>16` (**`-Os` emits `blx __udivsi3` for a
CONSTANT divisor** — whole-repo hazard, `grep __udivsi3`); **R0d** inlined the lerp; **R0e** 16-entry
palette, ONE byte per PAIR of columns, 112 → 9 Thumb instr/px; **R2a** the IA/8b glyph arm indexes
that table, −200,133/frame, FPS flat (banked slack, rule 12).

**R2b is the big one and it is DEFAULT-OFF awaiting the owner's eyes.** The owner named the lever
(affine BGs), so R2 became the **hardware affine BG**, not the planned dirty-flag.
`NDS_R2_RESULTS_AFFINE=1`: **10.125 → 7.025 VB/iter, −1,736,589 ticks/frame** vs the 1,746,558 R0h
sized for the whole background layer (**0.6%**), and the `I/4b 300x220` census row is **gone**.
Confirmed quantum-free by `soak-freeze-watch.ps1` as a wall-clock race: `gNdsVSResultsTickCount`
**500 → 780 (+56%)**, both arms NO-FREEZE, both `PacingPresentedFrames = 2043` (battle unperturbed).
**To graduate: show the owner `artifacts/visibility/2026-07-30_r207-r2b-results-{control-software,candidate-affine}.png`,
then flip the default.** Two traps already paid for: the mapper **letter-boxes** any wallpaper whose
origin is not (0,0), and a faster candidate lands on a **different scene tick** at the same wall
clock (both now standing rules). **Results is now owned by the glyphs and the commit** (`IA/8b 24x37`
alone is 85.1% of the remainder). R0f's `-Phases` split was an INSTRUMENT ARTIFACT, replaced by R0h's
per-PC profile; idle was **830,260 = 1.48 VBlanks of slack**, which R2b's 3.1 VBlanks exceeded.

## NO LEVER LEFT INSIDE R2-06 — the premium has now refused to concentrate TWICE

**The animation body is CLOSED — three levers, three refutations, zero builds spent.** 146,148/frame,
86,819 stall, 46,148 over §4's 100K, none reachable here: **E13** pose-fewer-joints refuted (collision's
ancestor closure is **f = 0.840** of live joints, `cosmetic-only` EMPTY); **E14** reorder refuted at
~2,900 (**10-15 ticks/32-byte line fill**, no prefetcher; `AObj` 36 B, N 221 — both memos wrong);
**E15** unbuilt, ~16-22K straddling the floor. `gNdsFighterInit*` is proof-build-only, **0 by
construction — never cite it**; the census script reports the 60 Hz set.

**Every named load-frame candidate is sized and none closes the 40,448.** Of the 139,072/load-frame
premium: relocation 33,632, action-change re-add 11,313, E9's payload walks 21,788 (a relocation
subset; a full offsets refactor still lands ~1,138,660, 18,660 over) — **~94,127 (67.7%) has NO named
owner**, and E17 killed E8's hypothesis (16 load frames, only 7 whole-GObj re-adds, so the marker is
*not* a proxy for "a fighter changed action"). **Second time a premium here refused to concentrate.**
**Before a fourth bracket, ask whether R2-06 is the right phase** — two independent attributions both
say spread, which points at the switch plan's §3 structural change. Derivations on the board.

## The one open fidelity item

- **E32** — blocked on a **generator gap, not a decision** (E62), and E7 showed it no longer blocks a gate
  lever. The flash clears `G_LIGHTING` and draws vertex colours raw; the owner hardware-lights with stale
  diffuse/ambient, so Mario draws *unflashed* — not corrupt, pixel-identical on non-flash frames (510/511:
  0 px). E49's runtime half **refuted** (emits the baked `.rgba`, which holds **normals** — speckle, 2,199 vs
  1,551). **Needs the generator to bake the flash variant's colours**; E63: 2,164 B.
- **R2-03 E26 — demoted** to 23,844/frame; **replace the dispatch, not the writes** (E39). Spec in board.

## Refuted this cycle — do not re-derive

All by measurement, not opinion. **E51** `line_id` table (`YakumonoCount = 1`, so a 64x4-shaped loop
has trip count **one**); **E53** `{base,size}` mirror (exact, still P95 **+11,584**); **the flash as
vertex data** (E48-E58); **the pose table** (E61, 2.62 MB resident vs 4 MB RAM); **`.text.hot`** (E66,
+24,448); **R2-04 E57** hitboxes walk the live joint chain (`gmcollision.c:489`); **R2-06 E7** the
fighter fallback (0/256) and Task 39 effects (4 sparks/924 frames); **R2-06 E6** the Horner fold
(+7,168 P50, so E61 §5's other rows are suspect too — a memo is a memory stream); **the Mario/Fox
pointer arrays as index arithmetic** (E11, targets span 1.7 MB non-monotonically); **an AObj pool**
(E12, `syMallocSet` is a bump allocator); and **R2b's transform double-apply** (the mapper bakes it).

## Restart

Branch `codex/r2-runtime2`, not merged to master. Boundary `battle_playable_realtime`, mode
`163`. Tick-HUD build dir now holds an E11 ROM; rebuild before citing a hash.

```powershell
$env:DEVKITPRO = 'C:/devkitPro'; $env:DEVKITARM = 'C:/devkitPro/devkitARM'
.\scripts\verify-all.ps1 -Profile Boundary -List; git status --short
```

**Do not rebuild `smash64ds.nds`** (owner, 2026-07-28); **do rebuild the tick-HUD ROM whenever the published
one is** (owner, 2026-07-22), flag-identical. `-j`/`MAKEFLAGS` rules are in AGENTS.md `## Builds`. A clean
checkout must build through `build.ps1`, not bare `make`: four of six generated `.inc` files are gitignored
and only `build.ps1` regenerates them. Preserve canonical mode 163, renderer mode 9, mip 0, static texture
residency, source countdown, Dream Land water at source frame 0, Task 16 `1/1/1`. Do not edit `decomp/`.
**Bug #10 is FIXED and folded in** — `06992f10812` (from `2cbc6189d15`), host fixture + structural pin +
`pause_under20` oracle.
