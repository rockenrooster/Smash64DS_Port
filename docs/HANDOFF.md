# Handoff

Updated: 2026-07-30. **Restart surface only, capped at 150 lines** — durable detail goes to its
owning doc (board: queue + results; `PERF_LEDGER.md`; `KNOWN_ISSUES.md`; `TASK_STANDING_RULES.md`).

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
eleven buckets identically**. **This harness has zero run-to-run noise:** disagreement on a
fixed configuration is the binary or the emulator settings, never scatter.

**DLDI-on costs ~29,696 P95 and is required for retail parity (owner), so it is the honest
config.** E69 rebuilt at its own commit `4916656d`, identical source, reads **1,126,464**
DLDI-on vs the **1,096,768** it published DLDI-off. **The gate was already missed by 6,464
before this session, the 23,232 margin was a DLDI-off artifact, and R2-07's particle budget
was sized against it.** Read every pre-`3eb9ecdb` P95 as a lower bound.

**This session did NOT cost 33,984 — a P95 tail artifact; both candidates are closed.** Body
inside the noise floor (P50 **+4,352**) and **frames over 1.12M went 9/128 → 8/128**, so by
the gate's own criterion the tree is marginally *better*. `SRC +30,912` stays withdrawn.
Residual SD reads **refuted without a build** (`WarmFailed=0`, cache `Hits=79 Misses=2` of
81 — **two file loads reach the SD driver in the whole run**); heap layout moot. **Never
compare those two runs paired by frame** — the anim cache shifts load timing so they diverge
in state (P10 −89,088 / P90 +115,712); order statistics only.

**R2-06 E6 REFUTED — E61 §5's −56,774 Horner route is unavailable and its other rows are now
suspect.** Bounded green, measured **+7,168 P50 / 117 of 128 paired frames worse**: a memo is a
memory stream, that table priced only arithmetic. Durable: **`aobjs_num = 0` here**, so AObjs
are malloc'd individually and nothing can index them.

## OPEN P1: every over-gate frame is an ASSET-LOAD frame, and clean P95 MEETS THE GATE

**E8.** The Task 75 census ring marks the frames running `ndsRelocFinalizeLoadedFile`
(`reloc_backend_assets.c:3398`): **16 of 128, and 8 of the 9 over-gate frames are among
them** (842, the exception, is adjacent to load frame 843). Load frames P50 **1,113,152**;
**clean frames P50 974,080, P95 1,056,640 — inside the 1,120,000 gate by 63,360**, 1 of 112
over. The +139,072 premium is entirely `SRC` (+139,328). Frame 909 is 1,617,152, the event
E53 profiled. **The average frame is already 145,920 under budget; the milestone turns on
the load frames.**

**E7 refuted both previously-named causes** (see "Refuted this cycle"), and on over-gate frames
`FTR` is **−1,312** / `STG` **−2,496**: not a render problem. **E32's parked flash residual no
longer blocks a gate lever.**

**The relocation is 21.5% of the premium and that is the CEILING on fixing it.** In-window
(`NDS_R2_RELOC_FIXUP_TIMING=1`; counters are cumulative from boot, so difference across the
window): 18 calls, 478,080 ticks, **`ndsRelocNormalizeFighterAObj16File` = 88.4%**. Inside it:
**per-script normalize 40.7%, lane swap 31.2%, O(n²) scan only 17.3%** — naming the O(n²) from
reading the code was an inference the measurement refuted (n is 25.4). **The real shape is the
payload walked TWICE**; all its sub-passes use only pointer *differences*, so hoisting them to
cache-store time IS viable — worth ~19,400 of the 139,072 per load frame, P95 → ~1,142,000.
**Bank it, not the gate**; judge on load frames only (averaged it is 3,735/frame, under the
noise floor).

**E10: it is NOT the animation setup either.** Only **7 action changes** in 128 frames
(`gcAddAnimJointAll`, 14,482 each = 101,376 = **4.6%**), and `gcAddDObjAnimJoint`'s 320 calls
are ~1,501/frame of *body* cost, not premium. **26% attributed, 74% UNATTRIBUTED**, and frame
909 is +650,000 alone. **Stop naming functions and sample:** `NDS_TASK37_PROFILE=1` with the
window pinned to the load frames the Task 75 ring identifies (843, 869, 890, 898, 909, 924)
against a matched clean control — E53 ran that instrument but chose its window by WORK-H and
could not separate draw from update. Board §§"R2-06 E8"/"E10" have the tables.

## OPEN P1: the freeze class is ROOT-CAUSED — heap OOM spins in the allocator

Owner: *"lots of freeze bugs that seem random"* + *"sometimes hitting a shielded player
causes a freeze"*. **One cause explains the class** (`artifacts/verification/freeze-soak/
2026-07-29_202114-*`): `decomp/src/sys/malloc.c:30` is `while (TRUE);`, so the allocator
**hangs instead of returning NULL** on exhaustion, and `ndsR2AnimCacheStore` was a
*speculative* cache calling it from a gameplay frame on the shared `gSYTaskmanGeneralHeap`.
Chain: damage-fall → aerial interrupt → `ftMainSetStatus(213)` → on-demand
`FTMarioAnimAttackAirD` → `syTaskmanMalloc(3472)` → spin. Do NOT make `syTaskmanMalloc`
return NULL globally; decomp callers do not all check.

**One site, not two — the "second exhaustion at battle start" was MY regression.** A static
arena is BSS, and BSS competes with the runtime `calloc` that sizes the heap: crossing the
`0x130000` search floor (`diagnostics.c:7403`) costs **196,608 bytes in one step**. **The
arena now lives on the taskman heap, 92,160 bytes, +32 bytes of BSS**, using 87,824. **Do NOT
lower that floor** — it is a contract with the Task 36 replay guard
(`nds_renderer.h:124-134`); my earlier authorization is retracted. **Both configurations
complete a full match clean.** Four detector defects fixed, two verdicts withdrawn (it hashed
the window **title**, where melonDS renders its FPS counter). **Sudden Death has its own
issues** (owner). **No passive soak reaches match two** — `mnVSResultsCheckExit` needs a
`START_BUTTON` tap; soaks default 2.5 min, ceiling 5.

## OPEN P1: the VS Results screen is 21.9M ticks/frame, 1.5 FPS

R2-07's own gate, 19.5x the battle frame. **89.4% is `tfunc->scene_draw()` alone (19,550,631)**:
the native OAM path is gated to `nSCKindVSBattle` only, so Results software-blits two 320x240
layers per frame, and `taskman_seam.c:6950` has no pacing and no HUD. **Ungating the wallpaper
cache is REFUTED** (R0a) — a Dream Land specialization. Board has a third, wider candidate.

## The other half is ANIMATION, not collision (E60/E61 — replaces the `SRC` row)

**"float→fixed on the collision path" was wrong by ~20x and that row is deleted.** A leaf
helper is charged to itself, not its caller, so animation float was booked to
`__aeabi_fadd`/`__aeabi_fmul`. Caller-attributed, animation is **146,942/frame**, collision is
**under 4,000**, and the cubic is **99.6% of that float** (149.4 nodes @ **405 ticks**).
Refuted, do not re-propose: the *layout* route (Tasks 95/96), the pose table (size), the
Horner fold (E6), and Task 78's self-vs-inclusive animation-compiler error.

## The one open fidelity item

- **E32** — blocked on a **generator gap, not a decision** (E62), and E7 showed it no longer
  blocks a gate lever. The flash clears `G_LIGHTING` and draws vertex colours raw; the owner
  hardware-lights with stale diffuse/ambient, so Mario draws *unflashed* — not corrupt,
  pixel-identical on every non-flash frame (510/511: 0 px). E49's runtime half is **refuted**
  (it emits the baked `.rgba`, which holds **normals** — speckle, worse diff 2,199 vs 1,551).
  **Needs the generator to bake the flash variant's vertex colours**; E63: 2,164 bytes.
- **R2-03 E26 — demoted** to 23,844/frame (needs `NDS_TASK91_DRAW_PHASE_CENSUS=1` *and*
  `NDS_R2_SPAN_LEAN_TIMING=1`). **Must replace the dispatch, not the writes** (E39); read its
  spec only with E34/E34-b/E39/E43/E45/E56.

## Refuted this cycle — do not re-derive

**E51** `line_id` table (`YakumonoCount = 1`, so a 64x4-shaped loop has a trip count of
**one**); **E53** `{base,size}` mirror (exact, still P95 **+11,584**); **the flash as vertex
data** (E48-E58); **the pose table** (E61, 2.62 MB resident vs 4 MB RAM); **fixed-point
collision** (E60, under 4,000/frame); **`.text.hot`** (E66, +24,448); **R2-04 E57** — hitboxes
walk the live joint chain (`gmcollision.c:489`); **R2-06 E6** the Horner fold (+7,168 P50);
**R2-06 E7** the fighter fallback (0/256) and Task 39 effects (4 sparks/924 frames).

## Restart

Branch `codex/r2-runtime2`, not merged to master. Boundary `battle_playable_realtime`, mode
`163`. Tick-HUD ROM current at HEAD (`1C1136BA`).

```powershell
$env:DEVKITPRO = 'C:/devkitPro'; $env:DEVKITARM = 'C:/devkitPro/devkitARM'
.\scripts\verify-all.ps1 -Profile Boundary -List; git status --short
```

**Do not rebuild `smash64ds.nds` for P1 work** (owner, 2026-07-28). **Do rebuild the tick-HUD
ROM whenever the published one is** (owner, 2026-07-22) — keep its Makefile block
flag-identical. `-j`/`MAKEFLAGS` rules live in AGENTS.md `## Builds`. A clean checkout must
build through `build.ps1`, not bare `make`: four of the six generated `.inc` files are
gitignored and only `build.ps1` regenerates them. Preserve canonical mode 163, renderer mode 9,
mip 0, static texture residency, source countdown, Dream Land water at source frame 0, Task 16
`1/1/1`. Do not edit `decomp/`. **Bug #10 is FIXED and folded in** — `06992f10812`,
cherry-picked from `2cbc6189d15`, with a host fixture, a structural pin and the `pause_under20`
oracle.
