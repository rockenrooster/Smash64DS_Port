/* _ZN5Actor11LandingDustEb at 0x0200fc0c
 * Actor::LandingDust(bool doRaycast)
 * Spawns landing dust at the actor's own position by calling HugeLandingDustAt.
 */

typedef int Fix12i;

struct Vector3 {
    Fix12i x, y, z;
};

struct Actor {
    char pad[0x5c];
    struct Vector3 pos; /* 0x5c */
};

extern void _ZN5Actor17HugeLandingDustAtER7Vector3b(struct Actor *thiz, struct Vector3 *vec, int doRaycast); /* 0x0200fb84 */

void _ZN5Actor11LandingDustEb(struct Actor *thiz, int doRaycast)
{
    struct Vector3 vec;
    vec.x = thiz->pos.x;
    vec.y = thiz->pos.y;
    vec.z = thiz->pos.z;
    _ZN5Actor17HugeLandingDustAtER7Vector3b(thiz, &vec, doRaycast);
}
