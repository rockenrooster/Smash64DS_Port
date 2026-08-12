# Attachment-position cluster — Fox blaster spawn, fighter burn joints

---

## MEASURED 2026-08-12 — one build, one run, and the two rows split apart

`builds/build-c131-position` (`NDS_R2_BOTH_CPU=1`, `NDS_R2_POSITION_PROBE=1`),
`scripts/probe-attachment-position.ps1`, capture
`artifacts/verification/2026-08-12_attachment-position.txt`. The probe only
records, so this ROM still behaves like the one the owner played.

### Fox — **A == B. The shared helper is sound and Fox is not a cache bug.**

| shot | A (`gmCollisionGetFighterPartsWorldPosition`) | B (`func_ovl2_800EDBA4` + `mtx_translate`) | A − B |
|---|---|---|---|
| `tr=3113` | (−963.692871, **2189.746826**, 51.119053) | (−963.692871, **2189.746582**, 51.119061) | (0, **+0.000244**, −0.000008) |
| `tr=2075` | (−552.316528, **223.398254**, 88.820457) | (−552.316589, **223.398254**, 88.820457) | (+0.000061, **0**, 0) |

Both deltas are float epsilon against magnitudes of 10²–10³, i.e. ~1e-7
relative. `animlocks=0 mode=1 trans5=0` on both shots, so **A took the
parent-chain walking arm and never rebuilt a world matrix, and still agreed with
the rebuilt arm exactly.** The invalidation asymmetry this contract flagged
(`ftParamsUpdateFighterPartsTransformAll` passing `reset_mode = FALSE` to
descendants, unlike source `ftparam.c:2283`) is real in the code but **does not
produce a stale pose here** — it is not the defect and must not be "fixed" on
suspicion.

**Two consequences, and the second matters more than the first.**

1. Per the owner's own branch, the comparison moves off gameplay and onto the
   gun overlay: the beam and flash originate at the source-correct joint-17
   world position, so any remaining Fox error is the **drawn gun's muzzle vertex
   versus local `{60,0,0}`** — a mesh question, not a transform one.
2. **The crouch-under-laser divergence cannot be explained by a wrong spawn Y.**
   The spawn Y is source-correct to seven digits. That row keeps its own
   evidence and its own investigation — hurtbox or weapon collision — and must
   not be closed as a side effect of this cluster.

### Fire — **worse than a missing rotation: Y and Z were exactly zero**

| # | kind | sel | joint | source joint world XYZ | position the ROM actually used |
|---|---|---|---|---|---|
| 0 | 6 | 1 | 15 | (1012.33, **334.60**, **128.18**) | (1039.04, **0.000000**, **0.000000**) |
| 1 | 6 | 2 | 20 | (931.53, **172.55**, **−97.67**) | (1039.04, **0.000000**, **0.000000**) |
| 2 | 6 | 3 | 25 | (946.44, **109.95**, **22.11**) | (1039.04, **0.000000**, **0.000000**) |
| 3 | 6 | 4 | 9 | (1167.46, **326.46**, **−104.21**) | (1207.04, **0.000000**, **0.000000**) |
| 4 | — | 0 | 12 | (1188.31, **338.31**, **−5.48**) | (1190.29, **0.000000**, **0.000000**) |

The shadowed source rotation advanced 1→2→3→4→0 and wrapped at 5 exactly as
`ftparam.c:1783` prescribes, over five distinct joints with healthy varied world
positions — **the same helper Fox just proved sound**. Meanwhile the position the
shipped code passed to the Flame makers had **Y and Z exactly 0 on every
emission**: `ndsFTParamGetVisualPosition` takes its early return when the
colanim's `joint_id` does not resolve and returns the fighter's root translate,
so every flame of every burn spawned on the stage plane at the victim's feet.
That is the owner's symptom, and it is a stronger defect than the missing
rotation alone.

(Row 4's `kind` reads 0 while its position fields are populated: the probe's
stores are plain globals the compiler may reorder, and GDB halted mid-sequence.
It does not affect any figure above.)

### The candidate is NOT accepted — a frame conversion sits below the override

**The first verification run was worthless and the probe was the reason.** Its
ring columns printed byte-identical tables for `build-c131-position` (no
override) and `build-c132-flamejoint` (override), because `generic` is the value
the override *replaces* and `joint` is the source-selected joint — both are the
same with the fix in or out. That is "one run relabelled", not agreement. The
probe now also prints what the **maker** receives, which is the only value that
moves.

On `build-c132-flamejoint`:

```
FLAMEARG f=1 pos    0.000000    0.000000     0.000000
FLAMEARG f=2 pos   97.665802  172.549408  -107.509979
FLAMEARG f=3 pos  -22.107510  109.954025   -92.605942
FLAMEARG f=4 pos 1207.044189    0.000000     0.000000
```

**The override is live**: `f=2` and `f=3` carry joint 20's and joint 25's world Y
digit for digit (172.549408, 109.954025) against the joint table above. Before
the fix that column was 0.

**But the vector reaching the maker is not the world position the override
wrote.** Against joint 20 = (931.534180, 172.549408, −97.665802) and joint 25 =
(946.438232, 109.954025, 22.107510):

| component | relation, exact on both samples |
|---|---|
| `maker.x` | **= −joint.z** (97.665802 and −22.107510) |
| `maker.y` | = joint.y — correct |
| `maker.z` | **= joint.x − 1039.044189**, and 1039.044189 is the fighter's root X |

X and Z are swapped, X is negated, and Z is relative to the fighter root. That
is a basis change plus a translation sitting between `ftParamMakeEffect` and the
maker's `pos`, and **it is not accounted for**. Two emissions (`f=1`, `f=4`) also
still arrive with Y = 0.

**So the row is not fixed and must not be reported as fixed.** The next question
is exactly one: *what converts the frame between `ftParamMakeEffect` writing
`pos` and `efManagerFlame*MakeEffect` reading it* — the port's particle spawn
path, or something inside the imported `efmanager.c`. Answer that before
touching the override again, and do not "correct" the axes at the override,
which would be compensating for one wrong transform with another.

### Order, resolved

The shared seam is measured sound, so the Flame fix is safe to land on its own
and does **not** risk scattering flames across five systematically displaced
joints. Fix landed in `ndsFTParamGetEffectJointPosition` +
`ftParamMakeEffect`; verification run on `builds/build-c132-flamejoint`.

---


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
