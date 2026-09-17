# P2-2p8 N04.09 candidate selection

Read with `census.txt` (symbol ranking) and `softfloat-callers.txt` (leaf
attribution) in this directory. Both were taken on a **clean-payload** build, so
they survive the 2026-09-16 re-baseline.

Ticks are `cycles / (2 * regions)` — the ARM9 runs at 2x the tick timer
(`scripts/analyze-profile-region-split.py:32`). `__aeabi_fadd + __aeabi_fmul` is
**90,169 tk/fr**.

## The float class by caller NAME

The census log prints one row per return address, so a function with two call
sites appears twice. Aggregated by name:

| share | tk/fr | caller | gate |
|---|---|---|---|
| 18.79% | 16,940 | `func_ovl2_800ED490` | UNRESOLVED |
| 10.87% | 9,801 | `gmCollisionSetInvertMatrix` | GAMEPLAY |
| 10.80% | 9,738 | `gmCollisionGetFighterPartsWorldPosition` | GAMEPLAY |
| 9.29% | 8,377 | `gmCollisionGetWorldPosition` | GAMEPLAY |
| 5.61% | 5,058 | `gmCollisionTransformMatrixAll` | GAMEPLAY |
| 5.07% | 4,572 | `ndsStageMPAdjustFloorLoopWallSweep` | RENDERER (**mislabelled**) |
| 4.74% | 4,275 | `guMtxCatF` | RENDERER |

**The collision matrix family is 55.36% of the class, about 49,900 tk/fr — 3.5x
the 14,080-tick floor.** That is the lane, not any single symbol.

`func_ovl2_800ED490` is `ndsR2SimMacBaseCompose`
(`src/import/battleship_gmcollision.c:217`), the decomp's unnamed 4x4 float
matrix compose on the collision path.

## Two corrections to how this ranking is read

1. **The RENDERER/GAMEPLAY gate is a hardcoded name list**, not analysis
   (`scripts/census-softfloat-callers.ps1:90-106`). Its `ndsStage` prefix
   fallback labels `ndsStageMPAdjustFloorLoopWallSweep` RENDERER, while
   `docs/optimization/review/06_*.md` marks it collision-FROZEN. Re-gate any
   caller before treating its label as permission.
2. An earlier board cursor called `guMtxCatF` the top caller. That was the
   double-row artifact above; it is 7th.

## Rejected here

- **`guMtxCatF`.** One live site only
  (`renderer_adapter_matrix.c:3980`, the kind-48 arm, 4.961 calls/frame); both
  camera sites are dead behind the shipped fixed camera
  (`NDS_R2_CAMERA_MATRIX_LEAN`, `NDS_R2_CAMERA_FIXED`), and the lbparticle site
  is cold. Deleting the function entirely is <=5,282 tk/fr, **0.38x the floor**.
  A Q20.12 concat is also the wrong mechanism: its look-at and perspective
  producers are float, so it would pay ~32 edge conversions per call against
  ~1,065 ticks removed.
- **`func_ovl2_800ED490` as the first slice.** It is the largest single caller,
  but `docs/optimization/review/12_COMPLETE_SIMULATION_MATH_CHAINS_TO_FIXED_POINT.md`
  Phase 1 rules it out for a first slice on two grounds, both of which still
  hold: the renderer consumes `parts->mtx_translate` from that chain, so a Q
  conversion "moves **drawn geometry** as well as hit results" and needs Tier
  A/B proof on both consumers; and "the board records the fixed-point route
  there as already declined once". The campaign says to prefer chains without a
  cross-subsystem consumer first.

## The governing economics, from Campaign 12

An f32 to Q edge conversion costs **31-42 cycles** against `__aeabi_fmul` at
26.5, so the exchange rate is set by **conversions per deleted operation**, a
property of the candidate's signature:

- **1.00 conv/op cannot pay**
- **0.57 breaks even**
- the 5.14x prior had **conv/op = 0**

Compute conv/op from the signature *before* building. Closed chains beat wrapped
leaves because conv/op falls as the chain lengthens. This is why the lane is
worth 49,900 tk/fr but a single wrapped leaf inside it may be worth nothing.

## Next action

Campaign 12 Phase 0/1 against the current baseline: re-census the simulation
float reservoir on the clean build masked to the gate's own rank-80 frames, then
pick a **closed chain** — velocity update, gravity/terminal velocity, position
integration, or a knockback subchain — with conv/op below 0.57 and no
cross-subsystem consumer. Infrastructure already exists: `NDS_R2_SIM_MAC_SHADOW`
(`battleship_gmcollision.c:213-258`) gives a same-binary shadow arm with
`gNdsR2SimMacShadowArm` and a call counter, which is Campaign 12's Phase 4.

Do not start from the biggest symbol. Start from the chain with the best
conv/op that no other subsystem reads.

---

## CORRECTION 2026-09-16: the sampled census over-attributes by 3.0x

**The ranking above is wrong and must not be used for sizing again.** It came
from `softfloat-callers.txt`, a 29,846-sample return-address census. Replacing it
with an **exact** count — every `bl`/`blx` site in the linked ELF joined to its
execution count in `arm9-profile.csv` — changes the answer. The exact method
captures 409,912 of 409,916 `__aeabi_fmul` entries (99.999%) and reproduces
`guMtxCatF` at 4.961 calls/frame independently.

Five symbols move by more than 2x in either direction. The two that matter:

| symbol | sampled census | exact | |
|---|---|---|---|
| `guMtxCatF` | 4.74%, 4,275 tk/fr, ranked **7th** | **9,599 tk/fr leaf, 13,485 with self, ranked 1st** | 317.5 fmul + 317.5 fadd per frame |
| `func_ovl2_800ED490` | 18.79%, 16,940 tk/fr, ranked **1st** | **7,108 tk/fr leaf, 9,783 with self** | 2.4x over-attributed |

The collision matrix family's true size is **25,022 tk/fr** (8,367 self + 16,655
leaf), not the ~49,900 this document claimed.

`guMtxCatF`'s rejection above still stands on its own terms — one live site, and
its look-at and perspective producers are float, so a Q concat pays ~32 edge
conversions per call — but its size was understated by 2.6x and it is the largest
single float consumer in the build, not the seventh.

## The collision lane is spent — it was built, engaged and measured as a cost

`NDS_R2_COLLISION_FIXED` (`Makefile:2065`, 0 in this build) already implements
the Q chain: `src/port/nds_r2_collision_ring.c` and
`src/port/nds_r2_collision_fixed.c` transcribe `func_ovl2_800EDBA4` /
`800EDE00` / `800EDE5C`. It was wired and run —
`artifacts/performance/2026-08-15_cfx-ring-wiring/RING.md`: *"The ring is wired
and engaged, no collision decision changed, and the gate did not move"* — at
WORK-H P50 **+64**, P95 **+896**, rank-80 **+3,648**, zero domain declines.

conv/op was never the binding constraint. The whole-chain form clears the
exchange rate by 13x (0.043 against the 0.57 break-even), and it still did not
pay: `…/2026-08-15_cfx-ring-split/SPLIT.md` decomposes it as issue **-1,717
tk/fr** against icache_fill **+1,854 tk/fr** — *"The arithmetic win is real and
it is 1.08x cancelled by fetch."* The resident variant sizes at **-6,261**.
`…/2026-08-15_cfx-narrow-exchange/EXCHANGE.md` closes the consumer half: *"Even
at an exchange rate of 0.00 -- fixed point free -- the lane's ceiling is 0.47x
the requirement."*

Do not rebuild it.

## The real finding next door: 97% over-invalidation

`ndsFTParamsInvalidateSubtree` (`src/port/reloc_backend_compat_shims.c:2955`)
flattens every descendant and clears `unk_dobjtrans_word` on each,
unconditionally. Measured: **474.5 part-word clears per frame against 14.3 matrix
recomputes — 3.0% utilisation.** It costs **20,744 tk/fr to protect 25,022** of
recompute, a ratio of 0.83: the memo barely pays for itself.

20,349 tk/fr of that is `InvalidateSubtree` self time at **5.6 cycles per
instruction** — data-stall bound on scattered `FTParts` writes, not arithmetic.
Split: clear loops 56%, descendant flatten 21%, prologue and flat-walk check 23%.

**Candidate: replace the O(parts) clear and descendant flatten with an O(1)
generation stamp** — a per-fighter counter, validity by stamp compare. Worth
**-12,000 to -18,000 tk/fr**, needs no fidelity argument, and a same-ROM A/B is
already proven in this repo: a `volatile u32 __attribute__((used,
section(".data")))` switch gives byte-identical `.text` in both arms, asserted by
`scripts/compare-elf-sections.py --max-diff 1`.

Falsifier: if the recompute pulls the same `FTParts` cache lines anyway, only the
issue slots go — 56% of 20,349 is about **-6,700**, not -18,000. A per-PC
issue/dcache split on the byte-identical pair settles it in one build.
