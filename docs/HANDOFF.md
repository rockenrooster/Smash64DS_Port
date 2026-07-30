# Handoff

Updated: 2026-07-29. **Restart surface only, capped at 150 lines.** Anything durable
goes to its owning doc: the board owns the queue and every result, `PERF_LEDGER.md`
measurements, `KNOWN_ISSUES.md` durable gaps and harness traps,
`optimization/TASK_STANDING_RULES.md` how a task is run.

| phase | state |
|---|---|
| R2-00a/b/c, R2-01, R2-02 | gated |
| R2-03 | shipped E12/E28/E29/E46/**E32**/**E64b**/**E65**/**E67**/**E69** — five gate levers, 1,228,928 -> 1,096,768, but all DLDI-off and so lower bounds; only the E32 flash residual is open (KNOWN_ISSUES) and it no longer blocks a lever |
| R2-04 | loading + rate clauses done (E5/E6/E57); budget clause closed by E64b+E65 |
| R2-05 | **COMPLETE** — reproducibility (E0) and zero fighter special cases (E1) |
| R2-06 | E0/E1/E2 done; soak instrument validated; **E4b + E6 refuted, E7 names the excursion** |
| R2-07/08 | not started; R2-08 needs the owner's retail play test |

## Where the gate stands — MISSED, and EVERY earlier number is DLDI-off

**`WORK-H` P95 1,160,448 / P50 976,064**, 8/128 over, frames 796..923, evidence
`r206-head-control-128` — a rebuild from HEAD that reproduces `r206-arena-heap-128` in **all
eleven buckets identically**, histogram and counters included. **This harness has zero
run-to-run noise:** disagreement on a fixed configuration is the binary or the emulator
settings, never scatter.

**DLDI-on costs ~29,696 P95 and is required for retail parity (owner), so it is the honest
config.** E69 rebuilt at its own commit `4916656d`, identical source, reads **1,126,464**
DLDI-on vs the **1,096,768** it published DLDI-off. **The gate was already missed by 6,464
before this session, the 23,232 margin was a DLDI-off artifact, and R2-07's particle budget
was sized against it.** Read every pre-`3eb9ecdb` P95 as a lower bound.

**This session did NOT cost 33,984 — that was a P95 tail artifact and both follow-up
candidates are closed.** Against the E69 rebuild: P25 +1,216, **P50 +4,352**, P75 +4,160,
P90 +8,512, P95 +33,984 — and **frames over 1.12M went 9/128 → 8/128**, so by the gate's own
criterion the current tree is marginally *better*. `SRC +30,912` stays withdrawn. Candidate
1, residual SD reads, **refuted without a build**: `gNdsR2AnimWarmFailed=0`, cache `Hits=79
Misses=2` of 81 force-loads, zero overflows — **two file loads reach the SD driver in the
whole run.** Candidate 2, heap layout, is moot. **Never compare those two runs paired by
frame** — the anim cache shifts load timing so they diverge in state (P10 −89,088 / P90
+115,712); order statistics only. Board §"R2-06 E4b CORRECTION" has the table.

**R2-06 E6 REFUTED — E61 §5's −56,774 Horner route is not available and its other rows are
now suspect.** Bounded green, measured **+7,168 P50 / 117 of 128 paired frames worse**: a
memo is a memory stream, that table priced only arithmetic. Durable: **this configuration
passes `aobjs_num = 0`**, so AObjs are malloc'd individually and nothing can index them.

## OPEN P1: the excursion is `SRC`, and the two named causes are REFUTED (E7)

The 8 over-gate frames are 809/842/843/869/890/898/909/924 — gaps 33/1/26/21/8/11/15, a
condition firing on ~6% of frames. Clean vs over-gate medians: **`SRC` +242,688**, `MISC`
+20,032, but **`FTR` −1,312 and `STG` −2,496** — the specialized draw owners are *cheaper*
on the expensive frames. `SRC` (`gNdsTickHudSourceTicks`, the source update) is **~92% of
the +233,472 WORK-H rise**; `OTHR`'s +341,920 is mostly the VBlank wait that `WORK-H`
already subtracts. **The fighter fallback is refuted: 256 of 256 draws native, `animLock:0`,
0 of 128 frames** — E32's shuffle fold closed E53's leading story, so **E32's parked flash
residual no longer blocks a gate lever.** Effects refuted free: 4 hit sparks in 924 frames.
Frame 809 is a separate audio cause (`AUD` 91,904 vs 1,280). **Next: attribute by BRACKET,
not by symbol** — E53 profiled symbols with `NDS_TICK_HUD_DRAW=0` and could not separate
draw from update. Do not re-run the fallback census.

## OPEN P1: the freeze class is ROOT-CAUSED — heap OOM spins in the allocator

Owner: *"lots of freeze bugs that seem random"* + *"sometimes hitting a shielded player
causes a freeze"*. **One cause explains the class**, caught 3.5 min into the both-CPU ROM
(`artifacts/verification/freeze-soak/2026-07-29_202114-*`): `decomp/src/sys/malloc.c:30` is
`while (TRUE);`, so the allocator **hangs instead of returning NULL** on exhaustion, and
`ndsR2AnimCacheStore` was a *speculative* cache calling it from a gameplay frame on the
shared `gSYTaskmanGeneralHeap`. Chain: damage-fall → aerial interrupt →
`ftMainSetStatus(213)` → on-demand `FTMarioAnimAttackAirD` → `syTaskmanMalloc(3472)` → spin.
Do NOT make `syTaskmanMalloc` return NULL globally; decomp callers do not all check.

**One site, not two — the "second exhaustion at battle start" was MY regression.** A static
arena is BSS, and BSS competes with the runtime `calloc` that sizes the heap: crossing the
`0x130000` search floor (`diagnostics.c:7403`) costs **196,608 bytes in one step**. **The
arena now lives on the taskman heap, 92,160 bytes, +32 bytes of BSS**, using 87,824. **Do
NOT lower that floor** — it is a contract with the Task 36 replay guard
(`nds_renderer.h:124-134`); my earlier authorization is retracted. **Both configurations
complete a full match clean**: `gNdsSyMallocOverflowCount=0`, replay admitted, every arena
overflow rejecting safely where each one used to hang.

**Four detector defects fixed; two soak verdicts withdrawn** — it hashed the window
**title**, where melonDS renders its FPS counter; `Invoke-SoakGdb` was called and never
defined; the 40 s trip threshold was under the game's ~30 s scene-load dead air; and a
static picture was called a hang without reading `x/1i $pc`. Only the pre-fix both-CPU
**FROZEN 210 s** stands. **Sudden Death has its own issues** (owner) — separate row. **No
passive soak reaches match two:** `mnVSResultsCheckExit` (`mnvsresults.c:266`) needs a
`START_BUTTON` tap and the results loop (`taskman_seam.c:6968`) is update-bounded only under
`NDS_HARNESS_FAST_LOGIC != 0`, pinned `0` everywhere shipped. Cross-match drift needs a
**synthesized START**; soaks default to 2.5 min, ceiling 5.

## OPEN P1: the VS Results screen is 21.9M ticks/frame, 1.5 FPS

R2-07's own gate, 19.5× the battle frame. **89.4% is `tfunc->scene_draw()` alone
(19,550,631)**: the native OAM path is gated to `nSCKindVSBattle` only, so Results
software-blits two 320×240 layers per frame, and `taskman_seam.c:6950` has no pacing, no HUD,
and never advances the presented-frame counter (on-screen FPS reads `0.0`). **Ungating the
wallpaper cache is REFUTED** (R0a) — a Dream Land specialization. Board has the method and a
third, wider candidate.

## The other half is ANIMATION, not collision (E60/E61 — replaces the `SRC` row)

**"float→fixed on the collision path" was wrong by ~20x and that row is deleted.** A leaf
helper is charged to itself, not its caller, so animation float was booked to
`__aeabi_fadd`/`__aeabi_fmul`. Caller-attributed, animation is **146,942/frame, 15.2% of
WORK 969,487**; collision is **under 4,000**; the cubic is **99.6% of that float** (E61:
149.4 nodes at **405 ticks each**). **Task 78 stopped the animation compiler on a
self-vs-inclusive error** (corrected 164,236, **1.64x its target, not 0.85x**).

## The one open fidelity item

- **E32** — blocked on a **generator gap, not a decision** (E62). The flash clears
  `G_LIGHTING` and draws vertex colours raw; the owner hardware-lights with stale
  diffuse/ambient, so Mario draws *unflashed* — not corrupt, pixel-identical on every
  non-flash frame (510/511: 0 px). E49's runtime half is **refuted** (it emits the baked
  `.rgba`, which holds **normals** — speckle, worse diff 2,199 vs 1,551). **Needs the
  generator to bake the flash variant's vertex colours**; E63: 2,164 bytes.
- **R2-03 E26 — demoted** to 23,844/frame (needs `NDS_TASK91_DRAW_PHASE_CENSUS=1` *and*
  `NDS_R2_SPAN_LEAN_TIMING=1`). **Must replace the dispatch, not the writes** (E39); read
  its spec only with E34/E34-b/E39/E43/E45/E56.

## Refuted this cycle — do not re-derive

**E51** `line_id` table (`YakumonoCount = 1`, so a 64×4-shaped loop has a trip count of
**one**); **E53** `{base,size}` mirror (exact, still P95 **+11,584**); **the flash as
vertex data** (E48–E58); **the pose table** (E61, 2.62 MB resident vs 4 MB RAM);
**fixed-point collision** (E60, under 4,000/frame); **`.text.hot`** (E66, +24,448);
**R2-04 E57** — hitboxes walk the live joint chain (`gmcollision.c:489`), so halving it
is a gameplay change; **R2-06 E6** the Horner fold (+7,168 P50).

## Restart

Branch `codex/r2-runtime2`, not merged to master. Boundary `battle_playable_realtime`,
mode `163`. Tick-HUD ROM is current at HEAD (`1C1136BA`).

```powershell
$env:DEVKITPRO = 'C:/devkitPro'; $env:DEVKITARM = 'C:/devkitPro/devkitARM'
.\scripts\verify-all.ps1 -Profile Boundary -List; git status --short
```

**Do not rebuild `smash64ds.nds` for P1 work** (owner, 2026-07-28). **Do rebuild the
tick-HUD ROM whenever the published one is** (owner, 2026-07-22) — keep its Makefile
block flag-identical. `-j`/`MAKEFLAGS` rules live in AGENTS.md `## Builds`. A clean
checkout must build through `build.ps1`, not bare `make`: four of the six generated
`.inc` files are gitignored and only `build.ps1` regenerates them. Preserve canonical
mode 163, renderer mode 9, mip 0, static texture residency, source countdown, Dream Land
water at source frame 0, Task 16 `1/1/1`. Do not edit `decomp/`. **Bug #10 is FIXED and
folded in** — `06992f10812`, cherry-picked from `2cbc6189d15` to preserve authorship,
with a host fixture, a structural pin, and the `pause_under20` oracle.
