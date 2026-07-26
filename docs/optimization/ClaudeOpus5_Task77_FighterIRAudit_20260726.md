# Task 77 E0 — The fighter compiler already exists; the roadmap order is wrong

**Date:** 2026-07-26
**Status:** E0 complete. **The plan's task ordering (77 IR → 78 animation → 79
render) does not match this repo and should be reordered.** No runtime change.

Task 77's stated deliverable is "a host tool capable of generating canonical
Mario and Fox fighter IR." Before writing one, this audited what
`scripts/generate_nds_native_owners.py` already produces. Its own docstring
reads: *"Generate the canonical Mario/Fox native-owner IR from exact O2R
inputs"* — 3,164 lines of it, emitting a 406 KB generated include.

## 1. What already exists

`src/nds/nds_native_fighter_owner.generated.inc` carries, for Mario and Fox:

| Table | Extent | Covers |
|---|---|---|
| `sNdsNativeFighterDenseVertices` | 541 | geometry |
| `sNdsNativeFighterDenseCorners` / `PackedCorners` | 1878 | topology, emit order |
| `sNdsNativeFighterRuns` + `RunFirstCorner`/`RunUniqueDense` | 67 runs | render runs |
| `sNdsNativeFighterStateDeltas` / `StateSequence` | 70 / 196 | material + polygon state |
| `sNdsNativeFighterVertexActions` / `EpochDirectPolicy` | 76 / 49 | submission policy |
| `sNdsNativeMarioBindingParents` / `BindingJoints` | 14 | **skeleton parentage** |
| `sNdsNativeFoxBindingParents` / `BindingJoints` | 18 | **skeleton parentage** |
| `sNdsNativeMarioJointSchedule` / `Fox` | 25 / 27 | traversal order |
| `sNdsNativeMarioFifoWords` + `FifoMatrixPatches` | 4034 / 14 | **prepared GX stream** |

Against the `FighterIR` the plan specifies:

- `skeleton.bones[].parent` — **present** (`BindingParents`)
- `skeleton.bones[].render parts[]` — **present** (bindings → runs → corners)
- fixed traversal order — **present** (`JointSchedule`)
- `materials[]` / `textures[]` — **present** (state deltas, palette slots, epochs)
- `render_runs[]` — **present** (67 runs, with submit class and mask)
- `animations[].tracks[]` — **absent**
- `bones[].gameplay_load_bearing` — **absent**

The generator mentions animation exactly once, in a closure name. Animation is
genuinely not in the IR; everything else largely is.

## 2. The roadmap order is wrong for this repo

`sNdsNativeMarioFifoWords[4034]` with `sNdsNativeMarioFifoMatrixPatches[14]` is a
**prepared GX command stream with matrix patch points** — a compiled command list
whose only per-frame varying input is the live matrices patched into it. That is Task
79's target ("prepared GX run", "known part order, known material, known texture,
known matrix source, direct GX submission"), already generated.

So the plan's sequence inverts this repo's actual state:

```text
plan order:   77 IR -> 78 animation -> 79 render -> 80 material
real state:   IR mostly built, render built, material state built,
              animation not started
```

**Recommended reorder:** the animation compiler becomes the next task, consuming
the skeleton the owner IR already emits. Generated render programs (plan's 79)
collapse into "finish wiring what is generated", not a new subsystem. Task 80's
prepared material bindings are already `StateDeltas`/`StateSequence`; the open
question there is whether the runtime consults them or still re-resolves.

This does not weaken the plan's thesis — it confirms it. The compiler-first
approach is already in production here for fighter geometry, which is why
`NDS_BATTLE_PROFILE=0` exists as a declared-but-unbuilt axis.

## 3. The gameplay-bone classification substrate

The plan makes `gameplay_load_bearing` mandatory per bone. This locates the
source of truth for it. Joint references in `include/ft/fighter.h` are a 7-bit
field (`s32 joint_id : 7`, so ≤128 joints) and appear in exactly 21 places
across 16 structs. Classified by what consumes them:

**Gameplay — verifier-gated, exactness-preserving transforms only:**

| Struct | Binding |
|---|---|
| `FTAttackColl` | hitbox → joint |
| `FTDamageColl`, `FTDamageCollDesc` | hurtbox → joint |
| `FTSpecialColl` | shield/reflector collision → joint |
| `FTMotionEventMakeAttack1` | attack spawn → joint |
| `FTMotionEventSetHitStatusPartID` | per-part hit status |
| `FTMotionEventSetDamageCollPartID1` | per-part hurtbox selection |
| `FTAttributes.joint_rfoot_id` | ground/ECB reference |

**Cosmetic — visual fidelity budget, quantization and rate reduction allowed:**

| Struct | Binding |
|---|---|
| `FTMotionEventMakeEffect1`, `GMColEventMakeEffect1` | effect spawn |
| `FTMotionEventSetModelPartID`, `FTParts` | model part selection |
| `FTHiddenPart` | part visibility |
| `FTTexturePart` | texture swap |
| `FTAccessPart` | accessory attachment |

A joint is gameplay-load-bearing for a fighter iff any member of the first set
references it in that fighter's data. `GMColEventMakeEffect1` is classified
cosmetic deliberately: it is spawned *by* a collision event but its own joint
binding only places a visual effect.

## 4. Most of the classification is static, in `FTAttributes`

`fp->attr` (`decomp/.../src/ft/fttypes.h:946`) turns out to carry the majority of
the joint bindings directly, per fighter, with no motion-script parsing:

**Gameplay:**

| Field | Extent | Binding |
|---|---|---|
| `damage_coll_descs[11].joint_id` | 11 | hurtboxes |
| `joint_rfoot_id`, `joint_lfoot_id` | 2 | slope contour — moves the fighter |
| `joint_itemheavy_id`, `joint_itemlight_id` | 2 | item attachment |
| `animlock` | bitfield | **joints that must not be animated** |
| `setup_parts` | bitfield | joints initialized on creation |

**Cosmetic:**

| Field | Extent | Binding |
|---|---|---|
| `effect_joint_ids[5]` | 5 | particle attachment (electricity, flames) |
| `shield_anim_joints[8]` | 8 | shield animation, one per ordinal direction |
| `hiddenparts`, `modelparts_container`, `textureparts_container`, `accesspart` | — | presentation swaps |

### `animlock` is the same bit Task 70 measured

`animlock` points at flags "marking joints that should not be animated". A joint
under animation lock is driven by code rather than by the animation, which makes
it gameplay-load-bearing **by construction** — it is exactly the class the
animation compiler must not quantize, because the animation is not what places
it.

This is the same mechanism as `fp->is_use_animlocks` at
`reloc_backend_renderer_dl.c:11582`, the rejection that Task 69 measured
accounting for all 10 native-owner fallback frames and Task 70 priced at 0.44%
of the P95. It was a performance nuisance there. Here it is a correctness
signal, and it is already extractable.

## 5. What E1 must do

Read `fp->attr` for Mario and Fox and emit `gameplay_load_bearing` per entry in
`BindingJoints` and `JointSchedule` from the table above. The runtime already
holds this struct, so a GDB snapshot on the Boundary configuration is sufficient
ground truth and needs no new host-side asset parser — the same instrument every
census in this campaign has used.

### `animlock` bit layout, from the existing reader

`ftParamSetAnimLocks` (`src/port/reloc_backend_compat_shims.c:7843`) already
decodes it, so the encoding is settled rather than guessed:

```c
animlock = fp->attr->animlock;   /* two u32 words -> 64 joint bits */
flags0 = animlock[0];
flags1 = animlock[1];
for (i = nFTPartsJointCommonStart; (flags0 != 0) || (flags1 != 0); i++) {
    current_flags = (i < ARRAY_COUNT(fp->joints) - 1) ? flags0 : flags1;
    if (current_flags & (1u << 31)) { /* joint i is animation-locked */ }
    ...shift...
}
```

Joint indices start at `nFTPartsJointCommonStart` and consume **bit 31 first**
(MSB-first), draining `flags0` for the first `ARRAY_COUNT(fp->joints) - 1`
joints and `flags1` thereafter. Two GDB word reads per fighter yield the whole
code-driven joint set. The confirmed absence of `animlock`, `setup_parts` and
`damage_coll_descs` from
`NDS_NATIVE_FIGHTER_CONSUMED_FIELDS.generated.json` is what establishes that the
existing generator does not already carry this.

**Hitboxes are the residue.** `FTAttackColl.joint_id` is set per-motion by
motion-event scripts inside the animation files, so it cannot be read from
`FTAttributes` and a single snapshot only shows currently-active hitboxes. Until
the animation compiler parses those scripts, every joint not *provably* cosmetic
is classified gameplay. That default is conservative in the safe direction: an
unflagged gameplay joint would let the animation compiler quantize a hitbox into
a different position and gate it on the owner's eye instead of the verifier,
which is the specific failure the plan's amendment exists to prevent.

Narrowing the conservative default is a by-product of the animation compiler, not
a prerequisite for it.

## 6. Checker obligation, unchanged

Per the plan and the `scripts/dreamland_world_mesh.py` `check_ir` precedent, the
extended IR ships with determinism (rebuilt IR hashes identically) and coverage
(every source joint represented exactly once, mapped back through provenance)
proofs. The existing generator already emits
`NDS_NATIVE_FIGHTER_CONSUMED_FIELDS.generated.json` and has companion checkers
in `scripts/check_nds_native_owner_hierarchy.py` and
`check_nds_native_owner_packet.py`, so the pattern is established — the new
fields extend those checkers rather than adding a third.

## 7. Cost avoided

Writing the fighter compiler the plan describes would have duplicated a 3,164-line
generator, a 406 KB generated IR, and two existing checkers. `AGENTS.md` — prefer
existing helpers, at equal cost less code wins — makes the audit the correct
first move, and it is the reason this task produced a reordering instead of a
second compiler.

## 8. E1 result — the free-quantization set is currently empty

`scripts/census-fighter-gameplay-joints.ps1` (new) reads `fp->attr` for both
battle fighters in one stop and classifies every joint.
`artifacts/task77-fighter-joints.json`:

| | Mario (fkind 0) | Fox (fkind 1) |
|---|---|---|
| joints present (`setup_parts`) | 24 | 26 |
| gameplay-load-bearing | **18** | **19** |
| unclassified | 7 | 8 |
| **provably cosmetic-only** | **0** | **0** |

Identical across both fighters: `animlock` = `0x10425200` → joints
{7, 13, 18, 21, 23, 26}; effect joints {9, 12, 15, 20, 25}; foot joints
{18, 23}. Item joints differ only in the heavy slot (Mario 28, Fox 30).

**The last row is the finding.** The plan's Task 78 splits techniques by bone
class and gives quantization and rate reduction to the cosmetic set. From static
data that set is empty: every joint in `effect_joint_ids` is *also* a hurtbox
joint, so nothing is safe to quantize on the grounds of being decorative. The
only candidates are the 7–8 unclassified joints, and the conservative rule
classifies those as gameplay until motion-event scripts prove otherwise.

This does not kill the animation compiler — it kills two of its listed
techniques and leaves the rest intact. Still fully available, because they are
exactness-preserving reorganizations rather than approximations:

- fixed skeleton order and fixed parent indices
- precomputed traversal order
- fixed-point coefficients replacing soft-float
- flat contiguous arrays replacing pointer-chased tree walking
- deleting figatree parsing and per-animation file interpretation

What is *not* available on current evidence is quantized rotations, reduced
skeletal update rates, and lossy pose tables — on any joint, for either fighter.

So Task 78's ≥100K target has to come from removing interpretation and memory
stalls, not from approximating the pose. That is consistent with Task 65's
finding that 62% of frame work is stall, but it should be stated before the task
starts rather than discovered when the verifier goes red.

### Measurement note

The runtime `sizeof` guard in the census earned itself immediately: it fired on
the first run with "joint array is 17". That was a parse bug of mine — the
printf leads with the slot, so every payload field sat one index further along
and `17` was the item-light joint id — but a census that had silently decoded
the animlock words against the wrong array bound would have produced a
plausible-looking, wrong classification with nothing to catch it.
