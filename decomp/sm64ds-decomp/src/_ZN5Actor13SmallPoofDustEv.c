/* _ZN5Actor13SmallPoofDustEv at 0x0200fd40
 * Actor::SmallPoofDust()
 * Spawns a small disappear poof dust at the actor's own position.
 */

typedef int Fix12i;

struct Vector3 {
    Fix12i x, y, z;
};

struct Actor {
    char pad[0x5c];
    struct Vector3 pos; /* 0x5c */
};

extern void _ZN5Actor19DisappearPoofDustAtERK7Vector3(struct Actor *thiz, const struct Vector3 *vec); /* 0x0200fd04 */

void _ZN5Actor13SmallPoofDustEv(struct Actor *thiz)
{
    struct Vector3 vec;
    vec.x = thiz->pos.x;
    vec.y = thiz->pos.y;
    vec.z = thiz->pos.z;
    _ZN5Actor19DisappearPoofDustAtERK7Vector3(thiz, &vec);
}
