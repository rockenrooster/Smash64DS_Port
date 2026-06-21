#ifndef SSB64_NDS_MATRIX_STACK_WORK_H
#define SSB64_NDS_MATRIX_STACK_WORK_H

#include <PR/ultratypes.h>
#include <PR/gbi.h>

#define NDS_MTX_FRAC_BITS 12
#define NDS_MTX_ONE (1 << NDS_MTX_FRAC_BITS)
#define NDS_MTX_STACK_MAX 16

typedef s32 NdsMtxFixed;

typedef struct NdsMtx44
{
    NdsMtxFixed m[4][4];
} NdsMtx44;

typedef struct NdsVec3
{
    NdsMtxFixed x, y, z;
} NdsVec3;

typedef struct NdsMatrixStack
{
    NdsMtx44 stack[NDS_MTX_STACK_MAX];
    u8 depth;
    u8 overflow;
    u8 underflow;
} NdsMatrixStack;

NdsMtxFixed ndsMtxFromFloat(f32 value);
f32 ndsMtxToFloat(NdsMtxFixed value);
void ndsMtxIdentity(NdsMtx44 *out);
void ndsMtxMul(NdsMtx44 *out, const NdsMtx44 *a, const NdsMtx44 *b);
void ndsMtxTranslate(NdsMtx44 *out, f32 x, f32 y, f32 z);
void ndsMtxScale(NdsMtx44 *out, f32 x, f32 y, f32 z);
void ndsMtxRotateZ(NdsMtx44 *out, f32 radians);
void ndsMtxRotateY(NdsMtx44 *out, f32 radians);
void ndsMtxRotateX(NdsMtx44 *out, f32 radians);
void ndsMtxLoadN64Mtx(NdsMtx44 *out, const Mtx *src);
NdsVec3 ndsMtxTransformPoint(const NdsMtx44 *m, NdsVec3 p);

void ndsMatrixStackInit(NdsMatrixStack *st);
const NdsMtx44 *ndsMatrixStackTop(const NdsMatrixStack *st);
NdsMtx44 *ndsMatrixStackMutableTop(NdsMatrixStack *st);
void ndsMatrixStackLoad(NdsMatrixStack *st, const NdsMtx44 *m);
void ndsMatrixStackMul(NdsMatrixStack *st, const NdsMtx44 *m);
void ndsMatrixStackPush(NdsMatrixStack *st);
void ndsMatrixStackPop(NdsMatrixStack *st);

#endif
