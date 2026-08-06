/* WithMeshClsn::IsOnWall() const at 0x0203562c
 * Reads the ON_WALL bit (mask 8) of the low byte of
 * this->sphere.resultFlags (u32 at 0x70 within sphere at 0x20 -> byte 0x90).
 */

typedef int s32;

struct WithMeshClsn { unsigned char pad[0x90]; unsigned char resultFlagsLo; };

s32 _ZNK12WithMeshClsn8IsOnWallEv(const struct WithMeshClsn* self)
{
    return self->resultFlagsLo & 8; // sphere.resultFlags & ON_WALL
}
