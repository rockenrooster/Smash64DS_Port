# TASK 50 — Hardware divider and square root

**Standing rules apply in full: read `docs/optimization/TASK_STANDING_RULES.md`
first.**

Branch: `codex/task50-hardware-divider` — **branch from current master.**
Independent of Tasks 48/49/51/52; needs no new pipeline and no oracle. It may
run in parallel with any of them.

Campaign: `docs/optimization/PROFILE0_NATIVE_CAMPAIGN.md` §5 Task 50.

## The opportunity, and the honest ceiling

The DS has a hardware divider (`REG_DIVCNT` at 0x04000280, `divf32`/`div32`) and
a hardware square-root unit (`sqrtf32`), both in libnds. **`src/` and `include/`
reference none of them** — verified. Meanwhile the Task 37 census
(`artifacts/task37-census/census.json`) spends, per 500,810,896 cycles:

| symbol | cycles | % | kind |
|---|---|---|---|
| `__ieee754_sqrtf` | 4,406,939 | 0.88% | IEEE float sqrt |
| `__aeabi_fdiv` | 3,749,855 | 0.75% | IEEE float divide |
| `__aeabi_ddiv` | 1,345,909 | 0.27% | IEEE **double** divide |
| `__udivsi3` | 1,233,889 | 0.25% | integer divide |
| `sqrtf` | 484,007 | 0.10% | float sqrt wrapper |
| `__divsi3` + `__aeabi_*divmod` | ~330,000 | 0.07% | integer divide/mod |
| **total** | **~11,550,000** | **2.31%** | |

**2.31% is the ceiling, not the estimate.** Two facts pull the recoverable
figure below it, and establishing the real number is this task's first job:

1. **The hardware units are fixed-point; most of this cost is float.** `divf32`
   divides two 20.12 fixed-point values; `sqrtf32` takes a 20.12 fixed input.
   `__aeabi_fdiv` and `__ieee754_sqrtf` operate on IEEE `f32`. Converting a float
   site means f32→fixed→hardware→fixed→f32, and the result is **not** bit-equal
   to the float path. So this is never a symbol-level drop-in — see below.

2. **Gameplay divides must stay bit-exact.** The Task 9 state hash holds gameplay
   state bit-identical (`TASK_STANDING_RULES.md` §fidelity doctrine). A divide or
   sqrt whose result flows into physics, collision, RNG, or the fixed scheduler
   cannot change value. The hardware unit *does* change the value. So a gameplay
   site is ineligible by construction, and only render-derived / non-gameplay
   sites are convertible.

Do not report 2.31% as the deliverable. Report what the census proves is
eligible, and let that be the number.

## Why this is site-specific, not a `--wrap`

Tasks 9/16 replaced float leaves with `-Wl,--wrap=__aeabi_fadd` etc.
(`Makefile:616-625`) because their replacements were **bit-exact** golden
reimplementations — safe to swap globally. The hardware divider is **not**
bit-exact with the float path, so wrapping `__aeabi_fdiv` globally would change
every gameplay divide and fail the state hash immediately.

Task 50 therefore converts **specific call sites**, not symbols: replace a chosen
`a / b` (or `sqrtf(x)`) at a render-side site with an explicit call to a
port-side `ndsTask50DivF32(a, b)` / `ndsTask50SqrtF32(x)` helper that drives the
hardware unit. The stock float path stays the default everywhere else and remains
the comparator. Each converted site is gated by a Makefile flag so a revert is
one line, exactly the Tasks 9/16 shape but applied per site.

## Stage E0 — census the call sites (no shipped code)

The census names the *symbols*; it does not name the *callers*. Find them.

1. Build the Task 37 profile ROM's map, or use `arm-none-eabi-nm` +
   `objdump -d` on `builds/build-task37-profile/…elf`, to list every caller of
   `__ieee754_sqrtf`, `sqrtf`, `__aeabi_fdiv`, `__aeabi_ddiv`, `__udivsi3`,
   `__divsi3`. Cross-reference the census's per-symbol cycles to rank callers by
   cost.
2. Classify each hot caller **render-side** vs **gameplay-side**:
   - render-side: matrix/projection/screen-space math, lighting, texture
     coordinate derivation, anything downstream of a GX emission and upstream of
     nothing gameplay. Eligible.
   - gameplay-side: `src/import/battleship_*`, collision (`gmcollision`),
     physics, camera math that feeds fighter tracking, RNG, `mp*` stage
     collision. **Ineligible** — the state hash forbids the value change.
   - `syMatrixLookAtReflectF` (661,058 cycles, uses sqrt) and
     `battleship_ftAnimParseDObjFigatree` are the kind of sites to classify
     carefully: the *camera transform* is render-derived, but "camera meaning"
     (what the camera tracks) is gameplay. Classify by whether the divide's
     result reaches the state hash, not by the function's name.
3. Note the `__aeabi_ddiv` (double) sites specially: doubles are almost always an
   accidental promotion (an `int / int` written as float, or a literal `1.0`).
   Several of these may be removable outright by fixing the type, which is a
   *bit-exact* win with no hardware divider at all. Flag those.

**E0 deliverable:** a ranked table — caller, cycles, class, eligible y/n, and the
summed eligible cycles. That sum is the task's real budget. **If the eligible sum
is < ~0.5% of total cycles, say so and stop at E0** — the hardware divider is not
worth a campaign task at that size, and the honest finding is the deliverable.

## Stage E1 — convert the eligible sites

Only the render-side sites from E0.

1. New `include/nds/nds_task50_hwdiv.h` + `src/nds/nds_task50_hwdiv.c` (or `.s`),
   following the **Task 46 header idiom**: no `#include <nds_build_config.h>`
   (it is force-included via `CFLAGS -include`, `Makefile:513`), `#error` if the
   flag is undefined. Helpers `ndsTask50DivF32`/`ndsTask50SqrtF32` drive
   `REG_DIVCNT`/`sqrtf32` with the correct busy-wait.
2. `NDS_TASK50_HWDIV ?= 0` in the Makefile beside the census flags; emit into
   `nds_build_config.h` and `print-benchmark-flags`; CFILES conditional.
3. Convert each eligible site behind the flag: `#if NDS_TASK50_HWDIV` uses the
   helper, `#else` keeps the stock float expression. Default off ⇒ published ROM
   byte-identical (`1818AA77…`); prove it.

## Gates

- **Task 9 state hash EXACT on every arm.**
  `.\scripts\verify-task37-itcm-state-hash-ab.ps1` (or the current state-hash
  A/B) must show all records identical with the flag on. If any eligible site
  turns out to reach the state hash after all, the hash catches it — that site
  reverts, no exceptions. This is the gate that makes "render-side" a proven
  claim rather than a guess.
- **Render-side visual sites additionally gate on the Task 49 differ** where they
  affect the GX stream: Tier 1 bit-exact on geometry/color, Tier 2 ≤ 1.0
  screen-space px on matrices (`ClaudeFable5_Task49_…md`). A divide feeding a
  matrix element can move a vertex; the differ is exactly the instrument that
  bounds it. Owner visual approval on any site that clears the differ but is not
  bit-exact.
- **Measure per call site, not wholesale.** The DS divider is asynchronous with a
  busy-wait; at low call density a naive swap is *slower* than the inline float.
  Report melonDS typed A/B per converted site and keep only sites that measure a
  win. Note the Task 10 calibration multiplier does not model the divider unit —
  this is a candidate for a queued device A/B, not a melonDS-final claim.
- Published ROM `1818AA77…` and tick-HUD `60C68AFF…`-class byte-identity at
  default (use the master-vs-mine fresh-dir test Task 49 established, not the
  stale `60C68AFF` pin).

## Constraints

- `decomp/` is read-only. `decomp/sm64ds-decomp/src/` has the Nintendo divider
  usage as reference: `cstd::fdiv`/`cstd::ldiv` route through the div unit; read
  `Geometry_MatrixMultiply3x3.c` and the `Fix12i` sites for the fixed-point
  convention. No headers there — algorithms only.
- One writer per touched TU; new flag defaults 0.
- Long builds detached; time-box open-ended debugging ~10 runs / ~1 hour.
- Separate commits: (1) E0 census, (2) helper + first site, (3) remaining sites,
  (4) docs. Cite `file:line` for every converted site and its class.
- KEEP ships enabled in the published profile at merge if it survives its gates;
  otherwise the branch is the checkpoint. **Never push.**

## Deliverables

1. E0 caller census: `artifacts/performance/2026-07-…_task50-divide-census.md`
   + the ranked eligibility table, with the eligible-cycle sum called out.
2. `ndsTask50DivF32`/`SqrtF32` helpers + per-site conversions behind
   `NDS_TASK50_HWDIV`, if E0 clears the ~0.5% bar.
3. State-hash A/B green; per-site melonDS A/B; Task 49 differ results for visual
   sites; a queued device A/B pair in `builds/device-queue/` for the divider
   timing (device-only class).
4. Results section here; one `PERF_LEDGER.md` entry; brief `HANDOFF.md` note.
5. `.\scripts\verify-dev-fast.ps1` green (bar the known pacing red), then
   `.\scripts\New-Smash64DSSnapshot.ps1 -Mode Lean` as the final action.

## Result (2026-07-23): STOP at E0

**Outcome: STOP at E0. No shipped code. Nothing merges.** The branch is the
checkpoint.

The E0 caller census (`artifacts/performance/2026-07-23_task50-divide-census.md`,
from `artifacts/task37-census/census.json` + `arm-none-eabi-objdump -d` on every
caller object) classifies every divide/sqrt call site and finds the eligible
render-side ceiling at **~0.55% of the battle budget** under generous static-site
attribution — and the realistic *recoverable* figure below the ~0.5% bar:

- The majority of the 2.31% callee cost lives in **gameplay code the state hash
  forbids changing** — `gmcollision`, `mp*` stage collision, `ftMainProcPhysicsMap`,
  `ftComputer` AI, shield/jump/damage/twister physics, `wp*` collision,
  `lbCommonSim2D`, `gcParse*AnimJoint`. Ineligible by construction; the densest,
  hottest sites are here.
- The hot eligible callers are a small set: the billboard-look-at matrix builders
  (`ndsRendererAdapterBuildBillboardMtxF`, `reloc_backend_renderer_dl.c:598–711`,
  kinds 33–40), the lighting-direction normalize
  (`ndsRendererHardwarePrepareLitDirection`, `nds_renderer.c:7742–7760`), and
  `syMatrixLookAtReflectF`/`PerspFastF`. The DS hardware divider is asynchronous
  with a busy-wait; at the battle frame's call density (a few dozen divides/frame)
  the per-call overhead can equal or exceed the inline IEEE float path. melonDS
  cannot referee divider timing (device-only class), so even a candidate site
  cannot be proven a win in the standard A/B.
- The `__aeabi_ddiv` "free win" the spec anticipated is **absent in battle**: every
  double-divide caller is cold (0 cycles in the census) — the `syMatrix*D` double
  builders (`matrix.c:1441–1486`) are not reached; the adapter uses the `*F`/`*R`
  float/radix paths via the `kind` switch. The only warm ddiv caller is
  `ftDisplayLightsDrawReflect` (0.0634%, 2 sites, render) — too small to chase, and
  not a `1.0`-promotion class fixable bit-exact.

This is the honest finding the spec asked for ("I'd rather Codex deliver an
honest '3 sites, 0.4%, here's the table' than force conversions that trip the
state hash"). E1 (helper + per-site conversion) does not run. The hardware
divider is not worth a campaign task at this eligible size.

Deliverables shipped: 1 (E0 census) only. Deliverables 2–3 (helpers, conversions,
state-hash A/B, device queue) are not produced — no code converted.
