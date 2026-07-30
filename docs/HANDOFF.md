# Handoff

Updated: 2026-07-30. **Restart surface only, capped at 150 lines** — durable detail goes to its
owning doc (board: queue + results; `PERF_LEDGER.md`; `KNOWN_ISSUES.md`; `TASK_STANDING_RULES.md`).

| phase | state |
|---|---|
| R2-00a/b/c, R2-01, R2-02 | gated |
| R2-03 | shipped E12/E28/E29/E46/**E32**/**E64b**/**E65**/**E67**/**E69** — five gate levers, 1,228,928 -> 1,096,768, but all DLDI-off and so lower bounds; only the E32 flash residual is open (KNOWN_ISSUES) and it no longer blocks a lever |
| R2-04 | loading + rate clauses done (E5/E6/E57); budget clause closed by E64b+E65 |
| R2-05 | **COMPLETE** — reproducibility (E0) and zero fighter special cases (E1) |
| R2-06 | E0/E1/E2 + soak done; E4b/E6/E7/E11 refuted; **E8 finds the event, E10 attributes it fully, E12 names the next lever** |
| R2-07/08 | not started; R2-08 needs the owner's retail play test |

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

**The 33,984 was a P95 tail artifact; both candidates closed, `SRC +30,912` withdrawn.** Residual
SD reads refuted without a build (cache `Hits=79 Misses=2` of 81). **Never compare an anim-cache
pair frame-by-frame** — it shifts load timing, so runs diverge in state; order statistics only.

## OPEN P1: every over-gate frame is an ASSET-LOAD frame, and clean P95 MEETS THE GATE

**E8.** The Task 75 ring marks the frames running `ndsRelocFinalizeLoadedFile`
(`reloc_backend_assets.c:3398`): **16 of 128, and 8 of the 9 over-gate frames are among them** (842,
the exception, is adjacent to load frame 843). Load frames P50 **1,113,152**; **clean frames P50
974,080, P95 1,056,640 — inside the 1,120,000 gate by 63,360**, 1 of 112 over. The +139,072 premium
is entirely `SRC`; frame 909 is 1,617,152, the event E53 profiled. **The average frame is already
145,920 under budget.** **E7 refuted both previously-named causes** (see "Refuted this cycle"); on
over-gate frames `FTR` is **−1,312** / `STG` **−2,496**, so it is not a render problem, and **E32's
parked flash residual no longer blocks a gate lever.**

**Inside the relocation, do NOT attack the O(n²) scan** — only 17.3% of it (n is 25.4). The
shape is the **payload walked TWICE** using pointer *differences* only, so hoisting it to
cache-store time is viable — ~19,400/load frame (E9), judged on load frames only.

**E10 ANSWERED the premium and there is NO single lever.** Profiler regions split by a marker the
profile itself observed: work premium **326,906/frame** (after removing `armWaitForIrq` 247,439 of
quantization slack and 45,917 of tick-HUD printf), spread over **513 symbols carrying 349,268 —
fully attributed.** Relocation family 37.0%, `ftAnimParseDObjFigatree` 13.0%, animation ~19%.

**STOP ACCUMULATING SMALL LOAD-FRAME CUTS — E11 proves they cannot be banked.** E11 took E10's
cleanest lever (`ndsRelocAssetIDForToken`, 630 calls on load frames, **0 on all 112 clean
frames**), removed the work provably and with **negative** bytes added, and measured the function
at **31,808 (−7,667, −5,103 insns)** with the load-frame set **bit-identical**. The gate still got
worse: **P95 +15,744, P99 +59,200, over-gate 9 → 11**, and the two added frames were **load**
frames 828/847. Two HEAD controls differ by only P95 +5,376, so that is ~3x noise. **REVERTED.**
A ~8,000 load-frame saving cannot survive relinking. What is left: one change big enough to clear
~16,000 of tail movement, or **move work off the frame** (E9). Guard: `reloc_backend_assets.c:1796`.

## OPEN P1: the freeze class is ROOT-CAUSED — heap OOM spins in the allocator

Owner: *"lots of freeze bugs that seem random"* + *"sometimes hitting a shielded player causes a
freeze"*. **One cause explains the class** (`artifacts/verification/freeze-soak/2026-07-29_202114-*`):
`decomp/src/sys/malloc.c:30` is `while (TRUE);`, so the allocator **hangs instead of returning NULL**
on exhaustion, and `ndsR2AnimCacheStore` was a *speculative* cache calling it from a gameplay frame
on the shared `gSYTaskmanGeneralHeap`. Chain: damage-fall → aerial interrupt →
`ftMainSetStatus(213)` → on-demand `FTMarioAnimAttackAirD` → `syTaskmanMalloc(3472)` → spin. **Do
NOT make `syTaskmanMalloc` return NULL globally** — decomp callers do not all check.

**One site, not two — the "second exhaustion at battle start" was MY regression.** A static arena
is BSS, and BSS competes with the runtime `calloc` that sizes the heap: crossing the `0x130000`
search floor (`diagnostics.c:7403`) costs **196,608 bytes in one step**. **The arena now lives on
the taskman heap, 92,160 bytes, +32 bytes of BSS**, using 87,824. **Do NOT lower that floor** — it
is a contract with the Task 36 replay guard (`nds_renderer.h:124-134`); my earlier authorization is
retracted. **Both configurations complete a full match clean.** Four detector defects fixed, two
verdicts withdrawn (it hashed the window **title**, where melonDS renders its FPS counter). **Sudden
Death has its own issues** (owner). **No passive soak reaches match two** — `mnVSResultsCheckExit`
needs a `START_BUTTON` tap; soaks default 2.5 min, ceiling 5.

## OPEN P1: the VS Results screen is 21.9M ticks/frame, 1.5 FPS

R2-07's own gate, 19.5x the battle frame. **89.4% is `tfunc->scene_draw()` alone (19,550,631)**:
the native OAM path is gated to `nSCKindVSBattle` only, so Results software-blits two 320x240
layers per frame, and `taskman_seam.c:6950` has no pacing and no HUD. **Ungating the wallpaper
cache is REFUTED** (R0a, a Dream Land specialization). Board has a third, wider candidate.

## NEXT LEVER: the animation body is 146,148/frame and 59.4% of it is STALL (E12)

**146,148/frame** (reproducing E60's 146,942 by a second instrument): **59,329 instructions against
86,819 of STALL**. **Switch plan §4 budgets animation at 100K, so it is 46,148 OVER — larger than the
40,448 the gate is missed by** — one over-budget subsystem accounts for the whole miss, so no hunt for
40,000 spread across the frame is needed. **Body** cost, so **the E11 wall does not apply.**
`ndsR2CubicValueFixed` (48,623, **1.73 cyc/insn**) is the only near-compute-bound member and is the
one E64b/E65/E67 won on; `gcPlayDObjAnimJoint` (40,973) is **already in `.text.hot` and still 65.8%
stall** — **data, not code**. **Every refuted candidate removed instructions or added data; none
improved locality** (see the refuted list).

**E13 — the switch plan prescribes the fix and FORBIDS the shortcut.** Do **not** halve the shared
walk: §3.5 says *"do not begin by compromising the simulation"*, keeping **gameplay 60 Hz / visual
pose 30 Hz**; §3.6 mandates the split — the renderer gets *"a compact generated pose"* and *"they must
not share one expensive runtime representation"*. **The 146,148-tick walk IS that forbidden shared
representation**, and giving the renderer its own pose is R2-04's own title (E6: E5 paid down
loading, not pose). Collision is **hard-bounded at 15 joints** —
`attack_colls[4]` + `damage_colls[11]` (`fighter.h:3141/3148`) — of ~25 live, and the parse/play walk
is **unconditional**, so the remainder is recoverable. **E13 owes the ancestor-chain union**; start
with the free half — `ndsFighterMarioFoxRecordDamageCollShell` already counts live hurtboxes and
discards it (`reloc_backend_fighter_model.c:2513`). Collision itself is still under 4,000.

## The one open fidelity item

- **E32** — blocked on a **generator gap, not a decision** (E62), and E7 showed it no longer blocks
  a gate lever. The flash clears `G_LIGHTING` and draws vertex colours raw; the owner
  hardware-lights with stale diffuse/ambient, so Mario draws *unflashed* — not corrupt,
  pixel-identical on every non-flash frame (510/511: 0 px). E49's runtime half is **refuted** (it
  emits the baked `.rgba`, which holds **normals** — speckle, worse diff 2,199 vs 1,551).
  **Needs the generator to bake the flash variant's vertex colours**; E63: 2,164 bytes.
- **R2-03 E26 — demoted** to 23,844/frame (needs `NDS_TASK91_DRAW_PHASE_CENSUS=1` *and*
  `NDS_R2_SPAN_LEAN_TIMING=1`). **Replace the dispatch, not the writes** (E39); read its spec only
  with E34/E34-b/E39/E43/E45/E56.

## Refuted this cycle — do not re-derive

All by measurement, not opinion. **E51** `line_id` table (`YakumonoCount = 1`, so a 64x4-shaped loop
has trip count **one**); **E53** `{base,size}` mirror (exact, still P95 **+11,584**); **the flash as
vertex data** (E48-E58); **the pose table** (E61, 2.62 MB resident vs 4 MB RAM); **`.text.hot`** (E66,
+24,448); **R2-04 E57** hitboxes walk the live joint chain (`gmcollision.c:489`); **R2-06 E7** the
fighter fallback (0/256) and Task 39 effects (4 sparks/924 frames); **R2-06 E6** the Horner fold
(+7,168 P50, and E61 §5's other rows are suspect with it — a memo is a memory stream); **the
Mario/Fox pointer arrays as index arithmetic** (E11, targets span 1.7 MB non-monotonically); and **an
AObj pool** (E12, `syMallocSet` is a bump allocator, so they are already contiguous).

## Restart

Branch `codex/r2-runtime2`, not merged to master. Boundary `battle_playable_realtime`, mode
`163`. Tick-HUD build dir now holds an E11 ROM; rebuild before citing a hash.

```powershell
$env:DEVKITPRO = 'C:/devkitPro'; $env:DEVKITARM = 'C:/devkitPro/devkitARM'
.\scripts\verify-all.ps1 -Profile Boundary -List; git status --short
```

**Do not rebuild `smash64ds.nds`** (owner, 2026-07-28); **do rebuild the tick-HUD ROM whenever the
published one is** (owner, 2026-07-22), flag-identical. `-j`/`MAKEFLAGS` rules are in AGENTS.md
`## Builds`. A clean checkout must build through `build.ps1`, not bare `make`: four of six
generated `.inc` files are gitignored and only `build.ps1` regenerates them. Preserve canonical
mode 163, renderer mode 9, mip 0, static texture residency, source countdown, Dream Land water at
source frame 0, Task 16 `1/1/1`. Do not edit `decomp/`. **Bug #10 is FIXED and folded in** —
`06992f10812` (from `2cbc6189d15`), with a host fixture, a structural pin, and the `pause_under20` oracle.
