/* Actor::OnAimedAtWithEgg() at 0x02010124 -- vtable slot 28.
 * Returns the egg auto-aim lock-on radius as a Fix12i (20.12 fixed-point).
 * Base Actor returns a default radius of 81920 == 0x14000 == 20.0; leaf
 * classes override to widen/narrow their egg-aimable hitbox.
 */

typedef int Fix12i;

struct Actor;

Fix12i _ZN5Actor16OnAimedAtWithEggEv(struct Actor* self)
{
    return 81920; /* 0x14000 == 20.0 in 20.12 */
}
