/* Camera::SetPos(const Vector3& pos) at 0x0200ccac
 * Stores the supplied world position into the camera's pos vector (@0x8c).
 */

typedef int Fix12i;

struct Vector3 { Fix12i x, y, z; };

/* Partial Camera (Camera : View : ActorDerived : ActorBase).
 * Only the fields touched here are named; the rest is padding so the
 * offsets match the real layout (lookAt @0x80, pos @0x8c). */
struct Camera {
    char _pad00[0x80];
    struct Vector3 lookAt; /* 0x80 */
    struct Vector3 pos;    /* 0x8c */
};

void _ZN6Camera6SetPosERK7Vector3(struct Camera *self, const struct Vector3 *pos)
{
    self->pos.x = pos->x;
    self->pos.y = pos->y;
    self->pos.z = pos->z;
}
