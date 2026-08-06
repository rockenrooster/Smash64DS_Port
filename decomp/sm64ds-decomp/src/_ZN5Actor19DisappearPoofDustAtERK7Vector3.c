/* Actor::DisappearPoofDustAt(Vector3 const&)
 * Spawns two simple particle effects (IDs 0x127, 0x128) at the given
 * position. `this` (r0) is unused; r1 = &pos.
 *
 * Callee: 0x02022e98 = Particle::System::NewSimple(u32, Fix12i, Fix12i, Fix12i)
 */

struct Vector3 { int x, y, z; }; /* 3 contiguous Fix12i */
struct Actor;

extern void _ZN8Particle6System9NewSimpleEj5Fix12IiES2_S2_(
    unsigned int id, int x, int y, int z);

void _ZN5Actor19DisappearPoofDustAtERK7Vector3(struct Actor *self, const struct Vector3 *pos)
{
    _ZN8Particle6System9NewSimpleEj5Fix12IiES2_S2_(0x127, pos->x, pos->y, pos->z);
    _ZN8Particle6System9NewSimpleEj5Fix12IiES2_S2_(0x128, pos->x, pos->y, pos->z);
}
