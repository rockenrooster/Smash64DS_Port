/* P2-2p8 Phase 1 slice 1 -- the lean fighter joint kernel.
 *
 * One forward pass over a fighter's flattened live topology (preorder, parent
 * index < child index) that reproduces ndsRendererAdapterComposeOwnerWorldsSource
 * (src/port/renderer_adapter_matrix.c) bit for bit:
 *
 *   local   the source 16.16 local of each joint. The common case -- exactly
 *           one FighterParts XObj, no animation locks, a cold gameplay cache
 *           and unit scale -- is computed here with the same integer
 *           arithmetic as ndsRendererAdapterBuildFighterTraRotRpyExact, kept
 *           as plain s32 cells instead of the N64 split Mtx (whose per-cell
 *           decode costs 11,178 tk/fr in the 2026-09-22 attribution). Every
 *           other joint is handed to the adapter's own builder through
 *           `slow`, so the exceptional branches are the source code itself.
 *   compose Q43.20 world = local x parent (basis s32 Q20, translation s64
 *           Q20), rounding half away from zero exactly as
 *           ndsRendererAdapterSourceWorldMulLocal does.
 *   output  each drawn binding's Q20.12 world (ndsRendererAdapterSourceWorldTo20p12)
 *           with the hitlag shuffle folded exactly as
 *           ndsRendererMtxMulAffine20p12(world, [I; sx sy 0]) folds it.
 *
 * ARM state (SMULL/SMLAL; Thumb has neither), in main RAM. Placement was
 * measured both ways on the four-CPU match: kernel in ITCM (evicting
 * ndsRendererNativePrepareProductionRun, 2,600 B) cost route 0 +20,000 FTR
 * ticks/frame mean and +99,000 at P95 through the evicted function, while
 * the kernel itself runs only ~2,700 ticks/draw slower from main RAM. */

#include <sys/obj.h>
#include <ft/fighter.h>
#include <nds/nds_fighter_matrix_index.h>
#include <nds/renderer_fighter_lean.h>

extern u16 gSYSinTable[0x800];

#if defined(__arm__)
#define NDS_FTR_LEAN_KERNEL_CODE \
    __attribute__((noinline, target("arm")))
/* Helpers are ARM too, so they inline into the ARM kernel (a Thumb helper
 * cannot be inlined into an ARM caller and became a main-RAM call). */
#define NDS_FTR_LEAN_KERNEL_INLINE \
    static inline __attribute__((always_inline, target("arm")))
#else
#define NDS_FTR_LEAN_KERNEL_CODE __attribute__((noinline))
#define NDS_FTR_LEAN_KERNEL_INLINE static inline
#endif

#define NDS_FTR_LEAN_FIGHTER_PARTS_KIND 0x4Bu
#define NDS_FTR_LEAN_ONE_BITS 0x3f800000u

typedef struct NDSFtrLeanWorld
{
    s32 basis[3][3];
    s32 pad;
    s64 translation[3];
} NDSFtrLeanWorld;

static NDSFtrLeanWorld sNdsFtrLeanWorlds[NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX];

NDS_FTR_LEAN_KERNEL_INLINE s64 ndsFtrLeanRoundShiftS64(s64 value, u32 shift)
{
    s64 bias = (s64)1 << (shift - 1u);

    if (value < 0)
    {
        return -(((-value) + bias) >> shift);
    }
    return (value + bias) >> shift;
}

/* ndsRendererAdapterFloatPow2ToS32 (renderer_adapter_matrix.c), verbatim. */
NDS_FTR_LEAN_KERNEL_INLINE s32 ndsFtrLeanFloatPow2ToS32(f32 value, u32 scale_bits,
                                           s32 *out)
{
    union
    {
        f32 f;
        u32 u;
    } bits = { value };
    u32 exponent = (bits.u >> 23) & 0xffu;
    u32 magnitude;
    s32 shift;

    if (exponent == 0xffu)
    {
        return FALSE;
    }
    if (exponent == 0u)
    {
        *out = 0;
        return TRUE;
    }
    magnitude = (bits.u & 0x7fffffu) | 0x800000u;
    shift = (s32)exponent - 127 - 23 + (s32)scale_bits;
    if (shift < 0)
    {
        magnitude = (shift <= -24) ? 0u : magnitude >> (u32)-shift;
    }
    else
    {
        if (shift > 8)
        {
            return FALSE;
        }
        magnitude <<= (u32)shift;
        if (((bits.u & 0x80000000u) == 0u) && (magnitude > 0x7fffffffu))
        {
            return FALSE;
        }
    }
    *out = ((bits.u & 0x80000000u) != 0u) ?
        (s32)(0u - magnitude) : (s32)magnitude;
    return TRUE;
}

/* ndsRendererAdapterFighterSinFromIndex, verbatim. */
NDS_FTR_LEAN_KERNEL_INLINE s32 ndsFtrLeanSinFromIndex(s32 index)
{
    u32 id = (u32)index & 0xfffu;
    s32 value = (s32)gSYSinTable[id & 0x7ffu];

    return ((id & 0x800u) != 0u) ? -value : value;
}

static NDS_FTR_LEAN_KERNEL_CODE s32
ndsFtrLeanAngleIndex(f32 angle, s32 *out)
{
    return ndsFighterMatrixAngleToIndexExact(angle, out);
}

/* The common case of ndsRendererAdapterBuildSourceFighterLocalMtx. Returns
 * 1 = local built, 0 = no FighterParts XObj (no local), -1 = not the common
 * case (hand it to the adapter's builder). Cells are row-major 4x3. */
NDS_FTR_LEAN_KERNEL_INLINE s32 ndsFtrLeanFastLocal(DObj *dobj, s32 *cells)
{
    FTStruct *fp;
    FTParts *parts;
    u32 parts_count = 0u;
    u32 i;
    s32 indexr;
    s32 indexp;
    s32 indexy;
    s32 fixed_x;
    s32 fixed_y;
    s32 fixed_z;
    s32 sinr;
    s32 sinp;
    s32 siny;
    s32 cosr;
    s32 cosp;
    s32 cosy;
    union
    {
        f32 f;
        u32 u;
    } sx, sy, sz;

    if (dobj->parent_gobj == NULL)
    {
        return -1;
    }
    for (i = 0u; i < dobj->xobjs_num; i++)
    {
        XObj *xobj = dobj->xobjs[i];

        if ((xobj == NULL) || (xobj->kind == nGCMatrixKindNull))
        {
            continue;
        }
        if (xobj->kind != NDS_FTR_LEAN_FIGHTER_PARTS_KIND)
        {
            return -1;
        }
        parts_count++;
    }
    if (parts_count == 0u)
    {
        return 0;
    }
    if (parts_count != 1u)
    {
        return -1;
    }
    fp = ftGetStruct(dobj->parent_gobj);
    parts = ftGetParts(dobj);
    if ((fp == NULL) || (parts == NULL) || (fp->is_use_animlocks != FALSE) ||
        (parts->transform_update_mode != 0))
    {
        return -1;
    }
    sx.f = dobj->scale.vec.f.x;
    sy.f = dobj->scale.vec.f.y;
    sz.f = dobj->scale.vec.f.z;
    if ((sx.u != NDS_FTR_LEAN_ONE_BITS) || (sy.u != NDS_FTR_LEAN_ONE_BITS) ||
        (sz.u != NDS_FTR_LEAN_ONE_BITS))
    {
        return -1;
    }
    if ((ndsFtrLeanAngleIndex(dobj->rotate.vec.f.x, &indexr) == 0) ||
        (ndsFtrLeanAngleIndex(dobj->rotate.vec.f.y, &indexp) == 0) ||
        (ndsFtrLeanAngleIndex(dobj->rotate.vec.f.z, &indexy) == 0) ||
        (ndsFtrLeanFloatPow2ToS32(dobj->translate.vec.f.x, 16u,
                                  &fixed_x) == FALSE) ||
        (ndsFtrLeanFloatPow2ToS32(dobj->translate.vec.f.y, 16u,
                                  &fixed_y) == FALSE) ||
        (ndsFtrLeanFloatPow2ToS32(dobj->translate.vec.f.z, 16u,
                                  &fixed_z) == FALSE))
    {
        return -1;
    }
    sinr = ndsFtrLeanSinFromIndex(indexr);
    cosr = ndsFtrLeanSinFromIndex(indexr + 0x400);
    sinp = ndsFtrLeanSinFromIndex(indexp);
    cosp = ndsFtrLeanSinFromIndex(indexp + 0x400);
    siny = ndsFtrLeanSinFromIndex(indexy);
    cosy = ndsFtrLeanSinFromIndex(indexy + 0x400);

    /* ndsRendererAdapterBuildFighterTraRotRpyExact's cells, in the order
     * ndsRendererMtxCellS16p16 decodes them (row, col). */
    cells[0] = (cosp * cosy) >> 14;
    cells[1] = (cosp * siny) >> 14;
    cells[2] = -sinp * 2;
    cells[3] = (s32)((u32)((((sinr * sinp) >> 15) * cosy) >> 14) -
                     (u32)((cosr * siny) >> 14));
    cells[4] = (s32)((u32)((((sinr * sinp) >> 15) * siny) >> 14) +
                     (u32)((cosr * cosy) >> 14));
    cells[5] = (sinr * cosp) >> 14;
    cells[6] = (s32)((u32)((((cosr * sinp) >> 15) * cosy) >> 14) +
                     (u32)((sinr * siny) >> 14));
    cells[7] = (s32)((u32)((((cosr * sinp) >> 15) * siny) >> 14) -
                     (u32)((sinr * cosy) >> 14));
    cells[8] = (cosr * cosp) >> 14;
    cells[9] = fixed_x;
    cells[10] = fixed_y;
    cells[11] = fixed_z;
    return 1;
}

s32 NDS_FTR_LEAN_KERNEL_CODE
ndsFtrLeanKernelCompose(const NDSFtrLeanJoint *joints, u32 joint_count,
                        NDSRendererMatrix20p12 *binding_worlds,
                        u32 binding_count,
                        s32 shuffle_x, s32 shuffle_y,
                        NDSFtrLeanSlowLocalFn slow)
{
    u32 j;
    u32 slow_joints = 0u;

    if ((joints == NULL) || (binding_worlds == NULL) || (slow == NULL) ||
        (joint_count == 0u) ||
        (joint_count > NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX))
    {
        return FALSE;
    }
    for (j = 0u; j < joint_count; j++)
    {
        const NDSFtrLeanJoint *joint = &joints[j];
        NDSFtrLeanWorld *out = &sNdsFtrLeanWorlds[j];
        const NDSFtrLeanWorld *parent = NULL;
        s32 cells[12];
        s32 kind;
        u32 has_local;
        u32 row;
        u32 col;

        if (joint->parent != 0xffu)
        {
            if ((u32)joint->parent >= j)
            {
                return FALSE;
            }
            parent = &sNdsFtrLeanWorlds[joint->parent];
        }
        kind = ndsFtrLeanFastLocal(joint->dobj, cells);
        if (kind < 0)
        {
            slow_joints++;
            if (slow(joint->dobj, cells, &has_local) == FALSE)
            {
                NDS_FTR_LEAN_CTR(gNdsFtrLean.kernel_slow_joints += slow_joints);
                return FALSE;
            }
        }
        else
        {
            has_local = (kind != 0) ? TRUE : FALSE;
        }

        if (parent == NULL)
        {
            if (has_local != FALSE)
            {
                /* ndsRendererAdapterSourceWorldFromLocal */
                for (row = 0u; row < 3u; row++)
                {
                    for (col = 0u; col < 3u; col++)
                    {
                        out->basis[row][col] = cells[(row * 3u) + col] * 16;
                    }
                }
                for (col = 0u; col < 3u; col++)
                {
                    out->translation[col] = (s64)cells[9u + col] * 16;
                }
            }
            else
            {
                /* ndsRendererAdapterSourceWorldIdentity */
                for (row = 0u; row < 3u; row++)
                {
                    for (col = 0u; col < 3u; col++)
                    {
                        out->basis[row][col] = (row == col) ? (1 << 20) : 0;
                    }
                    out->translation[row] = 0;
                }
            }
        }
        else if (has_local != FALSE)
        {
            /* ndsRendererAdapterSourceWorldMulLocal (out != parent: preorder) */
            for (row = 0u; row < 3u; row++)
            {
                s32 l0 = cells[(row * 3u) + 0u];
                s32 l1 = cells[(row * 3u) + 1u];
                s32 l2 = cells[(row * 3u) + 2u];

                for (col = 0u; col < 3u; col++)
                {
                    s64 sum = (s64)l0 * parent->basis[0][col] +
                        (s64)l1 * parent->basis[1][col] +
                        (s64)l2 * parent->basis[2][col];

                    out->basis[row][col] =
                        (s32)ndsFtrLeanRoundShiftS64(sum, 16u);
                }
            }
            for (col = 0u; col < 3u; col++)
            {
                s64 sum = (s64)cells[9] * parent->basis[0][col] +
                    (s64)cells[10] * parent->basis[1][col] +
                    (s64)cells[11] * parent->basis[2][col];

                out->translation[col] =
                    ndsFtrLeanRoundShiftS64(sum, 16u) +
                    parent->translation[col];
            }
        }
        else
        {
            *out = *parent;
        }

        if (joint->binding != 0xffu)
        {
            NDSRendererMatrix20p12 *world;

            if ((u32)joint->binding >= binding_count)
            {
                return FALSE;
            }
            world = &binding_worlds[joint->binding];
            /* ndsRendererAdapterSourceWorldTo20p12 */
            for (row = 0u; row < 3u; row++)
            {
                for (col = 0u; col < 3u; col++)
                {
                    world->m[row][col] = (s32)ndsFtrLeanRoundShiftS64(
                        out->basis[row][col], 8u);
                }
                world->m[row][3] = 0;
            }
            for (col = 0u; col < 3u; col++)
            {
                s64 value = ndsFtrLeanRoundShiftS64(out->translation[col], 8u);

                if ((value < (s64)(-2147483647 - 1)) ||
                    (value > (s64)2147483647))
                {
                    NDS_FTR_LEAN_CTR(gNdsFtrLean.kernel_slow_joints += slow_joints);
                    return FALSE;
                }
                world->m[3][col] = (s32)value;
            }
            world->m[3][3] = 1 << 12;
            /* ndsRendererMtxMulAffine20p12(world, [I; sx sy 0]): every basis
             * product is a multiple of 4096 and rounds back exactly; row 3 is
             * (t + s) * 4096 >> 12 with the s32 clamp. */
            if ((shuffle_x != 0) || (shuffle_y != 0))
            {
                s64 tx = (s64)world->m[3][0] + shuffle_x;
                s64 ty = (s64)world->m[3][1] + shuffle_y;

                world->m[3][0] = (tx > (s64)2147483647) ? 2147483647 :
                    (tx < (s64)(-2147483647 - 1)) ? (-2147483647 - 1) :
                    (s32)tx;
                world->m[3][1] = (ty > (s64)2147483647) ? 2147483647 :
                    (ty < (s64)(-2147483647 - 1)) ? (-2147483647 - 1) :
                    (s32)ty;
            }
        }
    }
    NDS_FTR_LEAN_CTR(gNdsFtrLean.kernel_joints += joint_count);
    NDS_FTR_LEAN_CTR(gNdsFtrLean.kernel_slow_joints += slow_joints);
    (void)slow_joints;
    return TRUE;
}
