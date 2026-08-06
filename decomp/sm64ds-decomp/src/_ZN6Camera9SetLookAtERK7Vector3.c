/* Camera::SetLookAt(const Vector3& lookAt) at 0x0200ccc8
 * Stores the supplied target point into the camera's lookAt vector (@0x80).
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

void _ZN6Camera9SetLookAtERK7Vector3(struct Camera *self, const struct Vector3 *lookAt)
{
    self->lookAt.x = lookAt->x;
    self->lookAt.y = lookAt->y;
    self->lookAt.z = lookAt->z;
}
