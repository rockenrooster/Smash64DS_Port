/* Actor::OnYoshiTryEat() at 0x02010160 -- vtable slot 17.
 * Returns an OnYoshiEatReturnVal telling Yoshi whether/how this actor can be eaten.
 * Base Actor returns 0 (not eatable); leaf classes override.
 */

typedef int OnYoshiEatReturnVal;

struct Actor;

OnYoshiEatReturnVal _ZN5Actor13OnYoshiTryEatEv(struct Actor* self)
{
    return 0;
}
