/* Actor::TriplePoofDustAt(Vector3 const&)
 * Spawns three simple particle effects (IDs 0x124, 0x125, 0x126) at the
 * given position. `this` (r0) is unused; r1 = &pos.
 *
 * Callee: 0x02022e98 = Particle::System::NewSimple(u32, Fix12i, Fix12i, Fix12i)
 */

struct Vector3 { int x, y, z; }; /* 3 contiguous Fix12i */
struct Actor;

extern void _ZN8Particle6System9NewSimpleEj5Fix12IiES2_S2_(
    unsigned int id, int x, int y, int z);

void _ZN5Actor16TriplePoofDustAtERK7Vector3(struct Actor *self, const struct Vector3 *pos)
{
    _ZN8Particle6System9NewSimpleEj5Fix12IiES2_S2_(0x124, pos->x, pos->y, pos->z);
    _ZN8Particle6System9NewSimpleEj5Fix12IiES2_S2_(0x125, pos->x, pos->y, pos->z);
    _ZN8Particle6System9NewSimpleEj5Fix12IiES2_S2_(0x126, pos->x, pos->y, pos->z);
}
