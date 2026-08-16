# Fox bore/crouch v5 — shared visual + collision line at +84

> ## STALE, 2026-08-15 — EVERY CLEARANCE NUMBER BELOW IS DERIVED FROM RUNTIME POSES TAKEN INSIDE THE SEGMENT-PHASE DEFECT WINDOW
>
> Kept in full rather than deleted, so the history reads correctly. **Do not quote any
> geometry figure in this file.** The verdict it supports (`bore 84`) is already
> superseded by `docs/BUGS.md` (`bore 0`, owner 2026-08-15).
>
> **1. Both terms of the inequality are live poses, not static geometry.**
> `laser_y = 223.398254` is a GDB print at a breakpoint in
> `gmCollisionCheckWeaponAttackFighterDamageCollide`
> (`artifacts/verification/2026-08-12_fox-crouch-hit.txt:24`); the beam's spawn Y is
> `ftfoxspecialn.c` resolving local `{60,0,0}` through
> `gmCollisionGetFighterPartsWorldPosition` on Fox's joint 17 — an **evaluated pose**.
> `crouch max Y = 242.217606` is `HURT_BOX mode=1 slot=1 joint=12 ... hi=242.217606`
> (`artifacts/verification/2026-08-12_fox-hurtbox-poses.txt:25`) — Mario's **evaluated
> crouch pose**. Both were captured 2026-08-12, inside the window in which
> `ndsR2FtAnimParseDObjFigatree` started every animation segment at phase `0` instead of
> `-anim_wait - anim_speed`, so the first evaluated sample of every segment was a whole
> frame into it (`69ce92e279f` introduced it; `64c41c361a7` repaired it, 2026-08-15).
> **They inherit that defect by construction, exactly as the tuned bore constant did.**
>
> **2. Independently of the staleness, "1.180648" was never the clearance margin.**
> This file measures the same sphere against the same box face from two different
> edges: the `crouch clear gap 45.180648` below uses the laser's **bottom**
> (`307.398254 - 20 = 287.398254`, minus `242.217606`), while "overlapped that crouch
> box by only 1.180648" uses the laser's **top** (`223.398254 + 20 = 243.398254`, minus
> `242.217606`). Read consistently with the bottom edge, the bore-0 figure is
> `203.398254 - 242.217606 = -38.819352` — an overlap **32.9x larger** than the number
> quoted — and the bore required to clear that pose would have been **>= 38.82**, not 84.
>
> **3. Play overrides both.** Owner, 2026-08-15, on `build-c198-bore0`:
> *"i said it was perfect, that includes the mario crouching avoiding the beam"*.
> Crouch clears at bore 0 on the repaired tree. Observed behaviour beats a derived
> number; **there is no defect here and nothing was re-tuned.**

Owner playtest, 2026-08-14: v4 (+72 presentation only) is "noticeably better,
just a little higher" and Mario still cannot duck under the beam.

Final owner playtest, 2026-08-14: v5 (+84 shared visual/collision bore line) is
**"perfect"**. This closes both the pistol/beam visual alignment and Mario's
ability to crouch under the beam; no hitbox-thinning change was needed.

## Change

`NDS_FOX_BLASTER_BORE_OFFSET_Y` is **72 -> 84**. The constant moved from
`nds_renderer.h` to `nds_effects.h` because it is no longer presentation-only:

- beam draw: world **+84 Y** (`nds_renderer.c`);
- muzzle/impact glow draw: world **+84 Y** (`battleship_lbparticle.c`);
- Fox Blaster attack collision: same world **+84 Y**
  (`battleship_fox_blaster.c`).

Weapon root position, velocity, map-collision box, lifetime and source attack
radius are unchanged. In particular **radius stays 20**; this does not solve the
crouch issue by arbitrarily thinning the laser.

## Why the gameplay collision had to move

The existing natural crouch proof measured the source gameplay laser at
**Y=223.398254, radius=20**. The highest crouching Mario hurtbox was
**Y=242.217606**. The old laser therefore reached Y=243.398254 and overlapped
that crouch box by only **1.180648 world units**.

That explains the owner symptom: presentation had moved upward by 72 units, but
gameplay collision still occupied the old low source line.

With the shared +84 line:

```
attack center       223.398254 + 84 = 307.398254
attack Y span       287.398254 .. 327.398254
crouch max Y        242.217606
crouch clear gap     45.180648
```

The standing pose is still hittable. Its measured slot-1 vertical interval is
**233.384460 .. 411.219238**, so the complete 40-unit laser diameter lies inside
that vertical range. This is the intended separation: **standing intersects;
crouching clears**.

Pose evidence is the already-banked
`artifacts/verification/2026-08-12_fox-hurtbox-poses.txt`; the natural old hit is
`artifacts/verification/2026-08-12_fox-crouch-hit.txt`.

## Both Fox facings stay on world +Y

BattleShip sets beam velocity `vel.x = lr * 160` and
`rotate.z = atan2(vel.y, vel.x)`, so the weapon DObj uses Rz(0) facing right and
Rz(pi) facing left. `wpProcessUpdateHitPositions` regenerates the attack position
from `attack_coll.offsets` every frame, and `wpProcessUpdateHitOffsets` scales the
offset, rotates it about Z, then adds the weapon translation.

Therefore v5 stores the collision correction in weapon-local coordinates as:

```
right-facing: local Y +84 --Rz(0)-->  world Y +84
left-facing : local Y -84 --Rz(pi)--> world Y +84
```

That signed local offset is essential; a naïve local +84 would move collision
DOWN when Fox faced left.

Only `attack_coll.offsets[0/1].y` are changed after the BattleShip maker returns.
`attack_pos` is deliberately not touched: `WPStruct` is free-list storage and
BattleShip's `wpProcessUpdateHitPositions` owns initializing/regenerating those
positions from the offsets before collision.

## Linked-binary proof

Candidate:

`builds/build-c155-bore84-collision/smash64ds-battle-playable-proof-hwtri.nds`

SHA-256:

`3EBB8033B9D155DA23DE6AB3F2BE8A4979B2F5ADB650DEB89D0836A211096B95`

The linked beam submit contains **+344,064 Q12 = 84 << 12** at
`nds_renderer.c:14988`. The weapon maker and reflector wrapper carry float
literals **+84.0** (`0x42a80000`) and **-84.0** (`0xc2a80000`) and directly store
one into each validated-zero attack offset. The maker contains **zero
`__aeabi_fadd` calls**. Reflection first executes BattleShip's callback, then
re-signs the two offsets from the updated `lr`. No per-frame collision arithmetic
was added; the existing BattleShip hit-position updater does that work.

Static headroom on the final candidate is **211,808 bytes proven**
(`fake_heap_start 0x02260ca4`).

## Verification note

`scripts/probe-fox-crouch-collision.ps1` now has `-InspectShotOnly`, which stops
after the source hit-position updater so future probes can inspect a fixed shot
without waiting forever for a hit that should no longer occur. The natural-AI
run did not reach its first accepted shot before this command bridge's foreground
limit, so no live-miss claim is made here. The source transform, linked binary,
and measured pose geometry close the mechanism; owner playtest remains the final
visual/gameplay acceptance gate.
