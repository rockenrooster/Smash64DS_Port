typedef int s32;
typedef unsigned int u32;
typedef unsigned char u8;

typedef struct { s32 x, y, z; } Vector3;
typedef struct { s32 m[12]; } Matrix4x3;
typedef s32 Fix12i;

struct Clipper { u32 vtable; };

extern void MulVec3Mat4x3(const Vector3* v, const Matrix4x3* m, Vector3* res); /* 0x02052858 */
extern void _ZN7Clipper13Func_020150E8ER7Vector35Fix12IiEPh(struct Clipper* thiz, Vector3* vec, Fix12i scale, u8* arg3); /* 0x020150e8 */

void _ZN7Clipper13Func_02015560ER9Matrix4x3R7Vector35Fix12IiES3_(struct Clipper* thiz, Matrix4x3* mat, Vector3* srcVec, Fix12i scale, Vector3* dstVec)
{
    MulVec3Mat4x3(srcVec, mat, dstVec);
    _ZN7Clipper13Func_020150E8ER7Vector35Fix12IiEPh(thiz, dstVec, scale, (u8*)0);
}
