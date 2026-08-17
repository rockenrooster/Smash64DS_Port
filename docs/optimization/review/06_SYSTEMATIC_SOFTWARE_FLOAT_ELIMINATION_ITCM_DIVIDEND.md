# Campaign 06 — Systematic Software-Float Elimination / ITCM Dividend

> **Planning baseline:** `codex/r2-runtime2` at `a63dd0e4b3af9c6601713f70c179c96c0aa67735` (2026-08-16).
>
> If HEAD has moved when implementation begins, re-run the inventory/measurement steps first and update symbol names/line references rather than blindly applying this document.
>
> **Campaign rule:** optimize toward a DS-native architecture and four-fighter headroom. The current two-fighter P95 gate is a checkpoint, not the architectural finish line. Never bank projected savings; measure the shipping configuration. Prefer same-binary route A/B when practical because this tree is placement-sensitive.
>
> **Basis (2026-08-17):** the shipping level is **+26,449** at rank-80 and the fresh per-PC census is `artifacts/performance/2026-08-17_shipping-rebank/v4-c238`. `c200` and `v3-c221` are retired. **Read `SHIPPING_REBANK.md` §7.7 before quoting any figure in this brief** — it lists what the new census contradicts, and mask the census by the GATE's own rank-80 frames.

## Objective

Treat fixed/native math as an architectural end-state.

Convert complete domains until software-float routines and conversion helpers become **unreachable from the shipping gameplay binary**, then physically remove their ITCM footprint and hand the space to Campaign 01.

The known lower bound on that footprint is the six relocated stock libgcc
members totalling **1,952 B** (`_arm_addsubsf3.o` 684 + `_arm_muldivsf3.o` 760 +
`_arm_cmpsf2.o` 276 + `_arm_unordsf2.o` 56 + `_arm_fixsfsi.o` 92 +
`_arm_fixunssfsi.o` 84 — the table `scripts/check-task9-float-itcm.ps1` pins by
hash), plus the Task16 replacement bodies and r2 sqrt support also resident in
ITCM. The frequently quoted "~2.6 KB" is a soft aggregate: **Phase 0 must state
the exact current occupancy from the linked ELF** before any dividend is
promised.

**2026-08-17 — the ITCM half of this campaign's case is spent, and its urgency
is now ordinary.** The re-knapsack (`ITCM_REPACK2.md`) found **13,188 B** in the
generic display-list renderer without touching a helper family, admitted
14,350 B, and banked **−44,544 at rank-80**. `.itcm` is back to **88 B free**, so
a freed input section still has an immediate consumer — but the next-best
consumers are archive members needing an extract-and-rename, not a starved
ranked list. Do not justify a numerics change by the ITCM dividend alone: the
one member this campaign can actually free is `_arm_addsubsf3.itcm.o`, 684 B, of
which 456 B is already dead and 228 B is live and hot (3,544.7 tk/fr).

**What DID go up is the arithmetic half.** The shipping census prices the whole
soft-float + integer-divide helper class at **160,996 tk/fr on the gate's own
rank-80 frames, concentration 2.02**, cross-checked against caller attribution
to 0.34%. That is the largest single class left on the board.

Success is not “fewer `__aeabi_f*` calls.” It is **zero shipping reachability for a helper family** followed by linker-level removal.

## The granularity constraint (measured 2026-08-16 — do not plan around it)

`docs/P1_EXECUTION_BOARD.md` (`ITCM_FRSUB.md` entry): `_arm_addsubsf3.o` is
**one 684-byte `.itcm` input section** — 456 B dead (`__aeabi_frsub` /
`__subsf3` / `__addsf3`, 0 executing PCs, unreachable by construction) welded
to 228 B **live** (`__aeabi_ui2f` / `__floatsisf` / `__aeabi_ul2f` /
`__aeabi_l2f`: 42 executing PCs, 2,396.6 instr/frame whole match, 3,544.7
tk/fr on the marginal-80, served from zero-wait ITCM today). **A linker cannot
split an input section: 684 bytes move or none do.** `__aeabi_ui2f` has 99
call sites in 21 functions; `__aeabi_l2f` is called from
`ndsRendererHardwarePrepareLitDirection` (shipping census: `__aeabi_l2f`
3,016,910 cycles, `__floatsisf` 876,227, `__aeabi_ui2f` 456,083, `frsub`/
`__addsf3` 0). Consequences:

- **No partial trims.** A member is reclaimed only when its *live* half's
  callers are gone — i.e. Campaign 12/13/03 must close the int↔float
  conversion families before this member moves at all. The dead 456 B rides
  along then, for free, and not before.
- Symbol-level "recoverable bytes" estimates over-count: the census's own
  "+1,858 B recoverable by eviction" includes the unsplittable frsub blob
  (over-counts by 456).
- Hand-authoring replacement soft-float leaves to work around the granularity
  is **refused** (board verdict).

## Named call-elimination candidates (measured 2026-08-17, none started)

**The lever is the CALL, not the representation.** The float→fixed conversion
class is closed: the leaf route measured **R = 0.83× and 1.00×** because an
f32↔Q edge conversion costs **31–42 cycles**, so above ~0.5 conversions per
deleted operation the exchange rate is negative. Nothing below proposes
converting arithmetic.

Ranked by helper cost attributed to the caller on the **gate's own rank-80
frames** (`softfloat-callers.txt`), with the caller's own self-time
concentration beside it — because `[[cluster-where-the-percentile-lives]]`: a
saving on frames that sit *above* rank-80 converts terribly (the card-FS lane
was 23,908 gate-80 and re-ranked to only −11,003, conversion 0.264).

| caller | g80 helper tk/fr | helper calls / gate frame | self conc | note |
|---|---:|---:|---:|---|
| `ndsBaseGcPlayMObjMatAnim` | **11,334** | **7,923** | **1.15** | the #1 candidate |
| `ndsStageMPAdjustFloorLoopWallSweep` | 10,924 | 6,136 | 1.20 | **collision — FROZEN** |
| `ndsR2FtAnimParseDObjFigatree` | 7,237 | 3,656 | 1.82 | Campaign 03 owns |
| `ndsRendererSubmitParticleQuad` | 6,856 | 5,252 | 1.79 | Campaign 13 |
| `mpCollisionGetFCCommonFloor` | 5,482 | 3,330 | 1.23 | **collision — FROZEN** |
| `syMatrixLookAtF` | 4,703 | 2,452 | — | camera/particle basis |
| `guMtxCatF` | 4,546 | 2,491 | — | 4×4 float concat |
| `syUtilsArcTan.part.0` | 4,044 | 1,439 | — | |
| `ndsBaseGcPlayDObjAnimJoint` | 3,789 | 3,309 | 1.34 | Campaign 03 |

A concentration near 1.00 is **good**, not bad: a uniform cut converts at ratio
1.000 on this basis, so a flat 11,334 is worth ~11,334 at rank-80.

**1. `ndsBaseGcPlayMObjMatAnim` — the largest, and the shape is a memo, not a
conversion.** The decomp body (`sys/objanim.c:1244`) walks every AObj of every
MObj and, before any track work, does `aobj->length += mobj->anim_speed` — one
`__aeabi_fadd` per AObj per frame, unconditionally, whenever
`anim_wait != AOBJ_ANIM_END`. Each scalar track then costs 1 mul + 1 add
(Linear) or ~10 mul + ~8 add (Cubic), and stores `value` into
`mobj->sub.trau/trav/scau/scav/scrollu/scrollv`, `texture_id_*`, `lfrac`,
`palette_id`. **On a stage whose water is frozen at source frame 0, an unknown
but plausibly large fraction of those AObjs have `anim_speed == 0` and therefore
produce a value identical to last frame's.** That is exactly the redundancy
shape this repo has already banked twice (62.12% with two compares; the draw
contract at 88–96%).
**Do not build it yet — measure the redundancy first.** One counter pair
(AObjs walked / AObjs whose `length` or output value did not change), read
through `-ExtraGlobals` on a run that is happening anyway, costs one build and
settles it. `[[declared-bound-is-not-trip-count]]`: 7,923 helper calls/frame is
measured, the *fraction that is redundant* is not.
It is now ITCM-resident (2026-08-17 pack, 732 B), so its I-cache half is already
paid; what is left is the arithmetic.

**2. The particle-camera basis — the memo exists, is ON, and misses 31.4%.**
`gNdsParticleCameraCacheEnabled` is `NDS_R2_PARTICLE_CAMERA_CACHE = 1` in the
shipping config (audited, not assumed), and the c239 run read its engagement
pair at end of match: **Hit 4,324 / Miss 1,889 — 31.4% miss** over 2,039
presented frames. Each miss rebuilds a perspective matrix, a look-at basis with
three `sqrtf`, and a full 4×4 float `guMtxCatF`. But the counters also **refute
the obvious follow-up**: hits+misses total ≈3.0 calls per presented frame, while
`guMtxCatF` alone takes 2,491 helper calls per gate frame — orders apart, so the
float-concat class has **other, larger callers** and tightening this key would
not touch most of it. Find those callers before proposing anything here.

**3. `syUtilsArcTan.part.0` (4,044) and `syMatrixLookAtF` (4,703)** are unowned
and unsized for concentration; neither has a named mechanism yet.

## Current repo anchors

- `scripts/census-softfloat-callers.ps1`
- `scripts/classify-softfloat-caller-phase.py`
- `scripts/check-task9-float-itcm.ps1`
- `scripts/task37_softfloat_callers.py`
- state-hash A/B scripts
- `src/nds/nds_task16_float_addsub.s`
- `src/nds/nds_task16_float_compare.s`
- `src/nds/nds_task16_float_i2f.s`
- `src/nds/r2/nds_r2_sqrtf.c`
- `src/port/nds_r2_sim_mac_fixed.c`
- Campaigns 03, 12, 13

## Scope separation

- Campaign 06 owns helper reachability and ITCM reclamation.
- Campaign 12 owns simulation Q chains.
- Campaign 13 owns draw/render Q chains.
- Campaign 03 owns animation representation.

Those campaigns convert domains; Campaign 06 continuously re-links and closes helper families.

## Hard constraints

1. Do not replace IEEE behavior with approximation merely to remove a symbol.
2. Do not create float→Q→float sandwiches.
3. Preserve gameplay branch/decision behavior unless a separately authorized tolerance exists.
4. Randomness paths such as `syUtilsRandFloat` are not automatic targets.
5. Remove helper code only when the shipping linker proves it unreachable.

## Phase 0 — Helper-family graph

Enumerate from shipping ELF:

- float add/sub/mul/div;
- compares;
- int↔float conversions;
- double helpers if present;
- custom fallback helpers;
- sqrt/normalize support;
- Task16 replacements.

For every callsite classify simulation, animation, draw, particles/VFX, UI/menu, diagnostic, or cold fallback.

Build:

`domain -> operation -> helper -> ITCM bytes`

with calls/frame and marginal-80 concentration.

## Phase 1 — Delete dead/generic dependencies

Before changing numerics, remove calls reachable only through:

- shipping diagnostics;
- dead fallback paths for P1-native assets;
- redundant API conversions;
- generic code superseded by Campaign 03/08/11 for qualified P1 content.

Each removal must follow an architectural ownership change, not a hardcoded one-off skip.

## Phase 2 — Close conversion families

Conversions disappear when producer and consumer agree on representation.

Examples:

- AOT animation coefficients stay fixed through evaluator;
- matrices/vertices stay native through GX submit;
- simulation state stays Q through a complete chain.

Maintain a “last callers” report for each helper family.

## Phase 3 — Close arithmetic families

After Campaign 12/13 slices land, re-census float arithmetic.

If one float multiply remains because persistent state is float, fix the state/chain rather than wrapping that single operation.

Use Campaign 07 for repeated/invariant division.

## Phase 4 — Prove link unreachability

For each family:

1. inspect all ELF references;
2. confirm zero reloc/call references;
3. ensure wildcard linker rules are not retaining it;
4. remove explicit ITCM placement/object;
5. relink;
6. verify symbol and bytes disappear.

Record exact reclaimed bytes per family — **at input-section granularity, not
symbol granularity** (see the granularity constraint above: a member with one
live symbol is fully retained). The unreachability proof for a member is the
union of proofs for every symbol it carries.

## Phase 5 — Separate math gain from ITCM gain

Measure:

A. runtime benefit from eliminated float work;
B. helper removed with ITCM space otherwise empty;
C. Campaign 01 refill benefit.

Do not conflate B/C with the numerical conversion gain.

## Phase 6 — Add regression guards

Once a helper/domain is retired, add a shipping checker that fails if a new caller reintroduces it and reports the caller chain.

## Verification

- same-binary A/B for risky numeric chains;
- gameplay state hashes;
- renderer parity;
- animation transition corpus;
- one-minute match;
- final soft-float caller census;
- ELF/map proof;
- any banked gate claim reports the 2/3/4/5+ VBlank-interval histogram and max
  interval (AGENTS.md device-report law);
- final ITCM occupancy.

## Completion criteria

All float-helper families reasonably removable from P1 shipping gameplay are link-unreachable, their ITCM bytes are physically gone (whole input sections, exact byte count recorded per family), guards prevent reintroduction, and Campaign 01 can repack the reclaimed space.
