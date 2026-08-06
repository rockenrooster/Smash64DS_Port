/* Actor::OnHitByMegaChar(Player&) at 0x02010130 -- vtable slot 26, combat hook.
 * Base Actor does nothing; leaf classes override to react to a mega-form player.
 */

struct Actor;
struct Player;

void _ZN5Actor15OnHitByMegaCharER6Player(struct Actor* self, struct Player* player)
{
}
