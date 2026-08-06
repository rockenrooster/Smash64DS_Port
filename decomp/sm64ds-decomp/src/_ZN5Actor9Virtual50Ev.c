/* Actor::Virtual50() at 0x0201014c -- vtable slot 19 (purpose unknown).
 * Base Actor returns the VirtualFuncSuccess code VS_FAIL (1); leaf classes override.
 */

typedef int bool;

struct Actor;

bool _ZN5Actor9Virtual50Ev(struct Actor* self)
{
    return 1; /* VS_FAIL */
}
