/* WithMeshClsn::GetResultFlag1() const at 0x02035620
 * Reads the COLLISION_EXISTS bit (mask 1) of the low byte of
 * this->sphere.resultFlags (u32 at 0x70 within sphere at 0x20 -> byte 0x90).
 */

typedef int s32;

struct WithMeshClsn { unsigned char pad[0x90]; unsigned char resultFlagsLo; };

s32 _ZNK12WithMeshClsn14GetResultFlag1Ev(const struct WithMeshClsn* self)
{
    return self->resultFlagsLo & 1; // sphere.resultFlags & COLLISION_EXISTS
}
