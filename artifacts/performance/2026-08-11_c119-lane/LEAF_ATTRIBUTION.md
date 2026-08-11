# The arithmetic leaves, attributed to their callers exactly, on the true top-80

Cycle 119. `task37_census.py --split-top-frames 80 --exclude-regions 1558
--attribute-leaves`, over `arm9-profile.csv` from `build-c119-profile-gate`.
No extra run, no GDB, no sampling.

## Method — and why the previous attempt could not answer this

A per-PC profiler charges a leaf helper to ITSELF and never to whoever ran it,
which is why the soft-float class had resisted ranking for a year:
`__aeabi_fadd` is one row worth tens of thousands of ticks and the row names
nobody. `census-softfloat-callers.ps1` attacked it with a GDB breakpoint on the
helper's exact entry address, reading `lr` — correct, but it can only sample a
**contiguous wall-clock window**, which on this ROM is a handful of frames and
almost certainly control frames. `whole-match-instrument-only` is the rule that
kills that reading.

The profiler already had the answer. It emits an exact **instruction count per
(region, pc)**, so the count at a `bl` IS the number of calls that site made
**on that frame**. Find every `bl` to the leaf in the linked ELF, look up its
per-frame execution count, and the attribution is exact, per frame, and can be
restricted to the eighty frames the percentile is actually made of.

Two traps, both now handled in the tool:

- **`blt`/`ble`/`bls` are BRANCHES**, not calls (`b` + `lt`). Splitting on the
  string "bl" turns every less-than branch in the binary into a call site.
  `bllt`/`blle`/`blls` *are* calls. The tool tests the suffix against the ARM
  condition set.
- **`__aeabi_fsub` sits four bytes before `__aeabi_fadd` and falls through into
  it.** A call to fsub therefore executes fadd's entry PC — so it is in the
  ground truth — while its `bl` targets an address the scan never looks at.
  Unfolded this reads as "36.8% of float adds come from nowhere". `A+B` folds
  the secondary entry in; the ground truth counts the PRIMARY entry only, or it
  double-counts the fall-through and reports complete attribution as a
  shortfall.

Every leaf below reconciles at **100.0%** against its own entry-PC count.

## What a tail frame costs

| leaf | calls/frame | cyc/call | tk/frame |
|---|---:|---:|---:|
| `__aeabi_fadd` (+fsub/frsub/addsf3) | 2,336.0 | 36.7 | **42,842** |
| `__aeabi_fmul` | 2,429.6 | 25.9 | **31,399** |
| `__aeabi_fdiv` | 220.1 | 118.6 | **13,034** |
| `ndsRendererMtxMul20p12` | 18.6 | 1,161.4 | **10,757** |
| `sqrtf` | 57.2 | 311.4 | **8,897** |
| `__udivsi3` | 124.3 | 123.3 | **7,653** |
| `__aeabi_f2iz` | 357.9 | 12.7 | 2,277 |
| `__aeabi_i2f` | 228.4 | 16.3 | 1,855 |

## The owners, grouped (ticks on a tail frame)

| group | tk/frame | status |
|---|---:|---|
| **20.12 matrix kernels** | **62,891** | renderer, already fixed point — bit-exact work available |
| legacy N64 float camera/matrix lane | 55,865 | renderer/camera, fidelity-gated |
| collision `mp*` / `gmCollision*` | ~11,100 | FROZEN — exact transforms only |
| animation parse/play | ~10,800 | parked (slice 41) |
| instrument (`RenderDebugHud`, `_strtol_l`, `uidivmod`) | ~7,100 | **excluded** — tick-HUD only, not in the published ROM |
| particles | ~3,500 | 60–63/80 |

The two groups overlap by construction (the float lane feeds the fixed one);
they are listed separately because they need different permissions.

### The 20.12 kernels, per tail frame

| symbol | calls | self tk | tk/call |
|---|---:|---:|---:|
| `ndsRendererMtxMulAffine20p12` | 54.21 | **19,175** | 354 |
| `ndsRendererAdapterBuildDObjXObjMatrix` | 57.50 | 12,233 | 213 |
| `ndsRendererLoadHardwareSplitMatrices` | 31.82 | 11,172 | 351 |
| `ndsRendererMtxMul20p12` | 18.55 | **10,757** | 580 |
| `ndsRendererAdapterBuildPersistentStageWorldMatrix` | 16.35 | 9,555 | 584 |

**Seventeen of the 18.55 general multiplies are one function.**
`ndsRendererAdapterPrepareNativeStageOwner` runs once a frame, builds
`view_projection` once, and composes it into all sixteen dynamic stage
bindings — 17.0 calls on hot frames and 17.0 on control frames, 80/80.
Its own self time is a further 6,216 tk, so that single call is **16,074
ticks of a tail frame**.

## The camera lane, sized

`gmCameraLookAtFuncMatrix` runs **2.00**/frame; `syMatrixLookAtReflectF` runs
**4.00**. The other two are the DS renderer's own: `ndsRendererAdapterBuild
CameraMatrices` and friends call `syMatrixLookAtReflect` and `syMatrixPerspFast`
— the N64 `Mtx` builders — so the renderer computes its camera in **software
float**, converts float→**s15.16** (`syMatrixF2L`), then converts
s15.16→**20.12** for the GX. A double conversion, twice a frame.

The `max > 32000` rescale in `gmCameraLookAtFuncMatrix` does **not** fire:
`ndsCameraCatCamera` is 2.00/frame, exactly one per camera call, so W1 is
working as documented. The second look-at is the renderer's, not the rescale's.

`syUtilsArcTan` and `syMatrixOrthoF` are **0.00 calls/frame** — their fdiv rows
come from elsewhere in the same symbol range and neither is a lever.
