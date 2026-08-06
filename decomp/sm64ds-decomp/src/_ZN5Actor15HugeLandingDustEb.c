/* _ZN5Actor15HugeLandingDustEb at 0x0200fb4c
 * Actor::HugeLandingDust(bool doRaycast)
 * Spawns huge landing dust at the actor's own position by calling LandingDustAt.
 */

typedef int Fix12i;

struct Vector3 {
    Fix12i x, y, z;
};

struct Actor {
    char pad[0x5c];
    struct Vector3 pos; /* 0x5c */
};

extern void _ZN5Actor13LandingDustAtER7Vector3b(struct Actor *thiz, struct Vector3 *vec, int doRaycast); /* 0x0200fac4 */

void _ZN5Actor15HugeLandingDustEb(struct Actor *thiz, int doRaycast)
{
    struct Vector3 vec;
    vec.x = thiz->pos.x;
    vec.y = thiz->pos.y;
    vec.z = thiz->pos.z;
    _ZN5Actor13LandingDustAtER7Vector3b(thiz, &vec, doRaycast);
}
