# Attachment-position cluster — Fox blaster spawn, fighter burn joints

Opened 2026-08-12 after the owner playtested the visibility fixes and confirmed
**both effects are now visible**. The visibility rows are closed; these two are
about **where** the effects appear. Do not reopen the visibility work.

Both rows terminate at the same helper,
`gmCollisionGetFighterPartsWorldPosition` (`gmcollision.c:491`), which is why
they are one cluster and why the shared seam is measured before either fix.

**Zero builds spent so far.** Everything below is read from the oracle and the
port.

---

## Row A — Fox muzzle flash and beam Y

### The source contract, read not assumed

`ftfoxspecialn.c` spawns from **joint 17** at local `{60, 0, 0}`:

```c
pos.x = FTFOX_BLASTER_SPAWN_OFF_X;   /* 60.0F */
pos.y = 0.0F;
pos.z = 0.0F;
gmCollisionGetFighterPartsWorldPosition(fp->joints[FTFOX_BLASTER_HOLD_JOINT], &pos);
wpFoxBlasterMakeWeapon(fighter_gobj, &pos);
```

`wpFoxBlasterMakeWeapon` hands that same `pos` to
`efManagerFoxBlasterGlowMakeEffect`, so **beam origin == flash position == the
joint-17 transform of `{60,0,0}`**, by construction. There is no second offset
to find and none may be added.

**The port runs this code verbatim.** `src/import/battleship_fox_blaster.c:90`
`#include`s the decomp `ftfoxspecialn.c`, and `:58` includes the decomp
`wpfoxblaster.c` under a rename. So the spawn arithmetic is not the defect —
**observing local `Y = 0.0F` proves nothing**, exactly as the owner said. The
defect is downstream, in what the shared helper returns for joint 17.

### Where the two routes actually diverge, and it is structural

`gmCollisionGetFighterPartsWorldPosition` has **two arms selected by
`fp->is_use_animlocks`**:

| arm | what it reads |
|---|---|
| `is_use_animlocks == FALSE` | walks joint 17 **up the parent chain**, applying each joint's **local** `unk_dobjtrans_0x10`; rebuilds a level only when `transform_update_mode == 0`; short-circuits to `mtx_translate` at the first level whose `unk_dobjtrans_0x5 != 0` |
| `is_use_animlocks != FALSE` | calls `func_ovl2_800EDBA4` when `unk_dobjtrans_0x5 == 0`, then reads the composed `mtx_translate` |

The gun overlay takes the **second** shape (rebuild, then read
`mtx_translate`). The projectile takes whichever arm the flag selects. **The two
are only equal while every local matrix on the chain is current**, which is a
cache-freshness property, not an arithmetic one.

The port's invalidation is not the source function. Source
`ftParamsUpdateFighterPartsTransform` (`ftparam.c:2283`) walks root-inclusive and
clears `transform_update_mode` **and** `unk_dobjtrans_word` on every node. The
port replaces it with `ndsFTParamsInvalidateRootParts` +
`ndsFTParamsInvalidateSubtree` over a cached flattened joint list
(`reloc_backend_compat_shims.c:1815`), and **`ftParamsUpdateFighterPartsTransformAll`
passes `reset_mode = FALSE` to the descendants**, so their
`transform_update_mode` is deliberately left set while only
`unk_dobjtrans_word` is cleared. That asymmetry is the first thing the A-vs-B
measurement should discriminate, because a descendant left at
`transform_update_mode == 1` makes the walking arm reuse a **local matrix built
on an earlier frame** while the rebuilding arm does not.

### The measurement, with its predictions written first

On the actual Neutral-B spawn frame, for joint 17 and local `{60,0,0}`:

| | route | predicted if the seam is sound |
|---|---|---|
| **A** | `gmCollisionGetFighterPartsWorldPosition(joints[17], {60,0,0})` | the muzzle |
| **B** | `func_ovl2_800EDBA4(joints[17])`, then transform `{60,0,0}` by `parts->mtx_translate` | identical to A |
| **delta** | A − B | `~0` on all three axes |

Also record `fp->is_use_animlocks`, and joint 17's
`transform_update_mode` / `unk_dobjtrans_0x5` at the instant A is taken —
those three values decide which arm ran and whether it rebuilt.

**If A != B** the bug is the shared cache seam and **Fox must not be touched**:
audit the invalidation asymmetry above, and check every other consumer of the
helper, because they inherit it. **If A == B** the gameplay position is
self-consistent and the comparison moves to the gun mesh's own muzzle vertex
against local `{60,0,0}`, projected through one camera.

**No `pos.y += ...` may be added under either outcome.**

### Crouch-under-laser

Stays open and becomes the acceptance test for this row rather than a separate
investigation: correct the spawn world position first, then retest, and only
then look at hurtboxes or the weapon's collision radius.

---

## Row B — burn flames on the wrong body parts

### The source contract

`ftparam.c:1899-1912` — all three Flame kinds **discard** the generic effect
position and re-resolve it:

```c
case nEFKindFlameLR:      ftParamGetEffectJointPosition(fp, &pos);
                          effect = efManagerFlameLRMakeEffect(&pos, lr);      break;
case nEFKindFlameRandom:  ftParamGetEffectJointPosition(fp, &pos);
                          effect = efManagerFlameRandomMakeEffect(&pos);      break;
case nEFKindFlameStatic:  ftParamGetEffectJointPosition(fp, &pos);
                          effect = efManagerFlameStaticMakeEffect(&pos);      break;
```

and the helper (`ftparam.c:1783`) **rotates through the fighter's configured
effect joints**, one per emission:

```c
fp->effect_joint_array_id++;
if (fp->effect_joint_array_id == ARRAY_COUNT(attr->effect_joint_ids))
    fp->effect_joint_array_id = 0;
pos->x = pos->y = pos->z = 0.0F;
gmCollisionGetFighterPartsWorldPosition(
    fp->joints[attr->effect_joint_ids[fp->effect_joint_array_id]], pos);
```

So the burn is **meant** to move around the body. Flames all originating from
one point is the defect, not the fix.

### The port's divergence — confirmed in source, not inferred

`ftParamMakeEffect` (`reloc_backend_compat_shims.c:8237`) computes `pos` once via
`ndsFTParamGetVisualPosition` — which uses the **caller-supplied `joint_id`**
plus scatter and inverse-size — and hands that straight to
`ndsFTParamMakeSourceEffect`, whose Flame cases (`:8161-8169`) use it unchanged.
`ftParamGetEffectJointPosition` **exists nowhere in the port**; the file's own
comment at `:8135` already records that it would be needed.

Both struct fields the helper needs are already present and mirror the oracle:
`FTStruct::effect_joint_array_id` (`include/ft/fighter.h:3153`, `u32 : 4`) and
`FTAttributes::effect_joint_ids[5]` (`:3574`), against `fttypes.h:1084` and
`:950`.

### The fix, and where it goes

Add a source-exact `ftParamGetEffectJointPosition(FTStruct *fp, Vec3f *pos)` and
call it in **`ftParamMakeEffect`**, after `ndsFTParamGetVisualPosition` and
before dispatch, for the three Flame kinds only. That is the smallest
architecture that preserves source behaviour: `ftParamMakeEffect` already holds
`fighter_gobj` and already calls `ftGetStruct`, `ndsFTParamMakeSourceEffect`
needs no new parameter, and the override also covers the
`NDS_R2_SOURCE_EFFECTS_PARTICLE=0` substitute arm — which the source's single
path implies and which a fix inside the maker switch would miss. No globals.

### The verification

Instrument one burn and record per emission: effect kind,
`effect_joint_array_id`, the selected `attr->effect_joint_ids[]` value, that
joint's world X/Y/Z, and the position the Flame maker receives. The sequence
must advance through the array and wrap at 5, and the positions must move around
the body. **Flames sharing one origin is a failure of this row, not a success.**

---

## Order of work, and why

The owner's order stands: **establish A-vs-B first.** Both rows consume
`gmCollisionGetFighterPartsWorldPosition`, so if that helper is wrong, restoring
Flame's joint rotation would distribute flames to five joints whose world
positions are all systematically displaced — a fix that looks like a fix and
measures like one.
