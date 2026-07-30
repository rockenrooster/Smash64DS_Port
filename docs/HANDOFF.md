# Handoff

Updated: 2026-07-29. **Restart surface only, capped at 150 lines.** Anything
durable goes to its owning doc: the board owns the queue and every result,
`PERF_LEDGER.md` measurements, `KNOWN_ISSUES.md` durable gaps and harness traps,
`optimization/TASK_STANDING_RULES.md` how a task is run.

Runtime 2 phase status. **Five gate levers graduated 2026-07-29: E32 (-52,416 P95),
E64b (-26,944), E65 (-35,584), E67 (-4,672), E69 (-12,544) — cumulative 1,228,928 ->
1,096,768, but all DLDI-off and so lower bounds; see the gate section.**

| phase | state |
|---|---|
| R2-00a/b/c, R2-01, R2-02 | gated |
| R2-03 | shipped E12/E28/E29/E46/**E32**/**E64b**/**E65**/**E67**/**E69**; only the E32 flash residual is open (KNOWN_ISSUES) |
| R2-04 | loading + rate clauses done (E5/E6/E57); budget clause closed by E64b+E65 |
| R2-05 | **COMPLETE** — reproducibility (E0) and zero fighter special cases (E1) |
| R2-06 | E0 + E1 + E2 done; soak clause has a validated instrument, one standing result |
| R2-07/08 | not started; R2-08 needs the owner's retail play test |

## Where the gate stands — MISSED, and EVERY earlier number is DLDI-off

**`WORK-H` P95 1,160,448 / P50 976,064**, 8/128 over, frames 796..923, evidence
`r206-head-control-128` — a rebuild from HEAD that reproduces `r206-arena-heap-128` in
**all eleven buckets identically**, VBlank histogram and counters included. **This
harness has zero run-to-run noise:** disagreement on a fixed configuration is the
binary or the emulator settings, never scatter.

**DLDI-on costs ~29,696 P95 and is required for retail parity (owner), so it is the
honest config.** E69 rebuilt at its own commit `4916656d`, identical source, reads
**1,126,464** DLDI-on vs the **1,096,768** it published DLDI-off (`r206-e69-recheck-128`).
**The gate was already missed by 6,464 before this session, the 23,232 margin was a
DLDI-off artifact, and R2-07's particle budget was sized against it.** Read every
pre-`3eb9ecdb` P95 as a lower bound.

Like-for-like this session costs **+33,984** and **`SRC +30,912` is WITHDRAWN — `SRC`
is 1,600 better.** It is all `OTHR` +30,528 / `WAIT` +31,168 with every named bucket
inside ~1,300 — DLDI's own signature, I/O in the remainder. Suspect residual animation
loads reaching the SD driver, or the 92,160-byte reservation's heap layout; board
§"R2-06 E4b" has the method. E4 already refuted the `syMallocSet` call overhead.

**R2-06 E6 REFUTED — E61 §5's −56,774 Horner route is not available, and its other rows
are now suspect.** Bounded green, measured **+7,168 P50 / 117 of 128 paired frames
worse**: a memo is a memory stream and that table priced only arithmetic. Two durable
facts: **this configuration passes `aobjs_num = 0`** (AObjs are malloc'd individually, so
nothing can index them), and two ROMs with identical arithmetic differed **23,040 at
P95** on layout alone — **judge at P50 and on paired frames.** Board has the full price.

## OPEN P1: the freeze class is ROOT-CAUSED — heap OOM spins in the allocator

Owner, 2026-07-29: *"lots of freeze bugs that seem random"* + *"sometimes hitting a
shielded player causes a freeze"*. **One cause explains the class**, caught 3.5 min
into the both-CPU ROM: `artifacts/verification/freeze-soak/2026-07-29_202114-*`.

`decomp/src/sys/malloc.c:30` is `while (TRUE);` — the BattleShip allocator **hangs
instead of returning NULL** on exhaustion, and `ndsR2AnimCacheStore` was a
*speculative* cache calling it from inside a gameplay frame on the shared
`gSYTaskmanGeneralHeap`, with its `payload == NULL` reject path therefore dead code.
Trigger chain: damage-fall → aerial interrupt → `ftMainSetStatus(213)` → on-demand
`FTMarioAnimAttackAirD` load → `syTaskmanMalloc(3472)` → spin. Registers refute the
interrupts-disabled wait, a GX deadlock, an IPC wait and the audio/FGM hypothesis;
`x/1i $pc` shows `b.n <self>`. Do NOT make `syTaskmanMalloc` return NULL globally;
decomp callers do not all check.

**The "second exhaustion site at battle start" was MY regression; there is one site,
not two.** A static arena is BSS, and BSS competes with the runtime `calloc` that
sizes the heap — crossing the `0x130000` search floor (`diagnostics.c:7403`) costs
**196,608 bytes in one step**, so 128 KiB dropped the shipped arena 1,286,144 →
1,048,576 and killed battle start. 32 KiB then failed **by 32 bytes** (the budget
belongs to the *tightest* build, not the shipped one), and 16 KiB fit but cost the
gate. **The arena now lives on the taskman heap, 92,160 bytes, +32 bytes of BSS.**
`NDS_R2_BOTH_CPU` adds no BSS of its own. **Do NOT lower that floor** — it is a
contract with the Task 36 replay guard (`nds_renderer.h:124-134`); my earlier
authorization is retracted. **Both configurations complete a full match clean**:
`gNdsSyMallocOverflowCount=0`, arenas above the floor with replay admitted, and every
arena overflow rejects safely where each one used to hang.

**Four detector defects fixed; two soak verdicts withdrawn.** It hashed the window
**title**, where melonDS renders its FPS counter; `Invoke-SoakGdb` was called and
never defined, so every `NO-FREEZE` was pixels-only; the 40 s trip threshold was
shorter than the game's ~30 s scene-load dead air; and a static picture was called a
hang without reading `x/1i $pc`. All fixed, command resolution now machine-checked.
Only the pre-fix both-CPU **FROZEN 210 s** stands. **Sudden Death has its own issues**
(owner) — separate row, not the allocator class. **No passive soak reaches match two:**
`mnVSResultsCheckExit` (decomp `mnvsresults.c:266`) exits on a `START_BUTTON` tap with
no timeout, and the results loop (`taskman_seam.c:6968`) is update-bounded only under
`NDS_HARNESS_FAST_LOGIC != 0`, which every shipped target pins to `0`. Cross-match
drift needs a **synthesized START**; soaks default to 2.5 min, ceiling 5.

## OPEN P1: the VS Results screen is 21.9M ticks/frame, 1.5 FPS

R2-07's own gate, 19.5× the battle frame. **89.4% is `tfunc->scene_draw()` alone
(19,550,631)**. The native OAM path is gated to `nSCKindVSBattle` only and Results
software-blits two 320×240 layers per frame; the loop at `taskman_seam.c:6950` has no
pacing, no HUD, and never advances `gNdsBattlePlayablePacingPresentedFrames`, so the
on-screen FPS reads `0.0`. **Ungating the wallpaper cache is REFUTED** (R0a) — it is
a Dream Land specialization. Board has the method and a third, wider candidate.

## The other half is ANIMATION, not collision (E60/E61 — replaces the `SRC` row)

**"float→fixed on the collision path" was wrong by ~20x and that row is deleted.** A
leaf helper is charged to itself, never its caller, so animation float was booked to
`__aeabi_fadd`/`__aeabi_fmul` and read as a separate, larger family. Caller-attributed,
animation is **146,942/frame, 15.2% of WORK 969,487**; collision is **under 4,000**.
E61: **the cubic is 99.6% of that float** — 149.4 nodes/frame at **405 ticks each**.
**Task 78 stopped the animation compiler on a self-vs-inclusive error** (corrected
164,236, **1.64x its target, not 0.85x**). Refuted and not to be re-proposed: the
*layout* route (Tasks 95/96), the pose table (size), and now the Horner fold (E6).

## The one open fidelity item

- **E32** — blocked on a **generator gap, not a decision** (E62). The flash clears
  `G_LIGHTING` and draws vertex colours raw; the owner hardware-lights with stale
  diffuse/ambient, so Mario draws *unflashed* — not corrupt, pixel-identical on every
  non-flash frame (510/511: 0 px). E49's runtime half is **refuted** (it emits the
  baked `.rgba`, which holds **normals** — speckle, worse diff 2,199 vs 1,551).
  **Needs the generator to bake the flash variant's vertex colours**; E63: 2,164 bytes.
- **R2-03 E26 — demoted** to 23,844/frame (needs `NDS_TASK91_DRAW_PHASE_CENSUS=1` *and*
  `NDS_R2_SPAN_LEAN_TIMING=1`). **Must replace the dispatch, not the writes** (E39);
  read its spec only with E34/E34-b/E39/E43/E45/E56.

## Refuted this cycle — do not re-derive

- **E51** `line_id` table (`YakumonoCount = 1`, so a 64×4-shaped loop has a trip
  count of **one**); **E53** `{base,size}` mirror (exact, still P95 **+11,584**);
  **the flash as vertex data** (E48–E58); **the pose table** (E61, 2.62 MB resident
  vs 4 MB RAM); **fixed-point collision** (E60, under 4,000/frame); **`.text.hot`**
  (E66, +24,448); **R2-04 E57** — hitboxes walk the live joint chain
  (`gmcollision.c:489`), so halving it is a gameplay change.

## Restart

Branch `codex/r2-runtime2`, not merged to master. Boundary
`battle_playable_realtime`, mode `163`. Tick-HUD ROM is current at HEAD (`1C1136BA`).

```powershell
$env:DEVKITPRO = 'C:/devkitPro'; $env:DEVKITARM = 'C:/devkitPro/devkitARM'
.\scripts\verify-all.ps1 -Profile Boundary -List; git status --short
```

**Do not rebuild `smash64ds.nds` for P1 work** (owner, 2026-07-28). **Do rebuild the
tick-HUD ROM whenever the published one is** (owner, 2026-07-22) — keep its Makefile
block flag-identical. `-j`/`MAKEFLAGS` rules live in AGENTS.md `## Builds`. A clean
checkout must build through `build.ps1`, not bare `make`: four of the six generated
`.inc` files are gitignored and only `build.ps1` regenerates them. Preserve canonical
mode 163, renderer mode 9, mip 0, static texture residency, source countdown, Dream
Land water at source frame 0, Task 16 `1/1/1`. Do not edit `decomp/`. **Bug #10 is
FIXED and folded in** — `06992f10812`, cherry-picked from `2cbc6189d15` to preserve
authorship, with a host fixture, a structural pin, and the `pause_under20` oracle.
