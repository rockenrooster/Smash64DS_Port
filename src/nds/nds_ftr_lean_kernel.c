/* P2-2p8 Phase 1 -- the lean fighter joint kernel (slices 1 and 3).
 *
 * One forward pass over a fighter's flattened live topology (preorder, parent
 * index < child index) that reproduces ndsRendererAdapterComposeOwnerWorldsSource
 * (src/port/renderer_adapter_matrix.c) bit for bit:
 *
 *   local   the source 16.16 local of each joint, as
 *           ndsRendererAdapterBuildSourceFighterLocalMtx builds it, kept as
 *           plain s32 cells instead of the N64 split Mtx (whose per-cell
 *           decode costs 11,178 tk/fr in the 2026-09-22 attribution). Every
 *           branch of that builder has a class here:
 *             fast     unit scale, cold cache: ndsRendererAdapterBuildFighter-
 *                      TraRotRpyExact's integer cells
 *             nolocal  no FighterParts XObj: no local (world = parent)
 *             scale    scale != 1: syMatrixTraRotRpyRSca's own integer cells
 *                      (slice 3; the builder's call is integer arithmetic on
 *                      the same sin table, so the cells are exact)
 *             warm     a warm gameplay cache (transform_update_mode != 0):
 *                      ndsRendererAdapterF2LFixedWExact's conversions, which
 *                      map source cell (r, c) to local cell (r, c) (slice 3)
 *             lock / convert / xobj / nogobj
 *                      the adapter's own builder through `slow`, so the
 *                      exceptional branches are the source code itself (a
 *                      conversion the integer forms cannot reproduce falls to
 *                      the source's float path there)
 *   compose Q43.20 world = local x parent (basis s32 Q20, translation s64
 *           Q20), rounding half away from zero exactly as
 *           ndsRendererAdapterSourceWorldMulLocal does.
 *   output  each drawn binding's Q20.12 world (ndsRendererAdapterSourceWorldTo20p12)
 *           with the hitlag shuffle folded exactly as
 *           ndsRendererMtxMulAffine20p12(world, [I; sx sy 0]) folds it.
 *
 * The joint table (NDSFtrLeanJoint) is built once per adopted state: it
 * carries each joint's FTParts pointer and its XObj class, which are fixed for
 * a DObj's life (gcAddDObj3TransformsKind), so the per-frame pass reads only
 * the transform, the scale bits and the part's cache mode. It keeps only the
 * joints a drawn binding hangs from (a binding's world depends on its
 * ancestor chain alone, locks included), so joints with no drawn descendant
 * are never composed.
 *
 * ARM state (SMULL/SMLAL; Thumb has neither), in main RAM. Placement was
 * measured both ways on the four-CPU match (slice 1): kernel in ITCM
 * (evicting ndsRendererNativePrepareProductionRun, 2,600 B) cost route 0
 * +20,000 FTR ticks/frame mean and +99,000 at P95 through the evicted
 * function, while the kernel itself runs only ~2,700 ticks/draw slower from
 * main RAM. */

#include <sys/obj.h>
#include <ft/fighter.h>
#include <nds/nds_fighter_matrix_index.h>
#include <nds/renderer_fighter_lean.h>
#if NDS_FTR_LEAN_KTIME
#include <nds/timers.h>
#endif

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

#define NDS_FTR_LEAN_ONE_BITS 0x3f800000u
/* The integer RSca form multiplies a 16.16 rotation cell (|x| <= 2^17 after
 * the row-1/row-2 differences) by the 8.8 scale; below this bound the s32
 * product cannot overflow, so the cells equal the source's exactly. Scales at
 * or past 32.0 take the source builder. */
#define NDS_FTR_LEAN_SCALE_FAST_MAX 0x2000

/* Q43.20 world, the source compose's NDSRendererAdapterSourceWorld: s32
 * basis, s64 translation. */
typedef struct NDSFtrLeanWorld
{
    s32 basis[3][3];
    s32 pad;
    s64 translation[3];
} NDSFtrLeanWorld;

/* ndsRendererAdapterSourceRoundShiftS64: round half away from zero. For a
 * negative x, -((-x + b) >> s) == floor((x + b - 1) / 2^s), so both signs
 * are one add and one arithmetic shift, (x + b - [x < 0]) >> s, with no
 * branch and no 64-bit negates -- identical results for every |x| < 2^62
 * (the compose's sums are products of s32 16.16 cells and s32 Q20 bases,
 * far inside that). */
NDS_FTR_LEAN_KERNEL_INLINE s64 ndsFtrLeanRoundShiftS64(s64 value, u32 shift)
{
    s64 bias = ((s64)1 << (shift - 1u)) - (s64)(value < 0);

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

/* The TraRotRpy / TraRotRpyRSca cells, unscaled when `scaled` is 0 (the
 * scales are then ignored). Returns FALSE where the integer forms cannot
 * reproduce the source's conversions (the source builder then runs). */
NDS_FTR_LEAN_KERNEL_INLINE s32 ndsFtrLeanTrsCells(DObj *dobj, s32 *cells,
                                                 u32 scaled)
{
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
    s32 a;
    s32 b;

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
        return FALSE;
    }
    sinr = ndsFtrLeanSinFromIndex(indexr);
    cosr = ndsFtrLeanSinFromIndex(indexr + 0x400);
    sinp = ndsFtrLeanSinFromIndex(indexp);
    cosp = ndsFtrLeanSinFromIndex(indexp + 0x400);
    siny = ndsFtrLeanSinFromIndex(indexy);
    cosy = ndsFtrLeanSinFromIndex(indexy + 0x400);

    if (scaled == 0u)
    {
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
    }
    else
    {
        /* decomp sys/matrix.c syMatrixTraRotRpyRSca: `scalex = sx * 256`
         * truncates toward zero, as FloatPow2ToS32 at 8 fraction bits does;
         * every product below is computed in s32 exactly as the source's. */
        s32 scalex;
        s32 scaley;
        s32 scalez;

        if ((ndsFtrLeanFloatPow2ToS32(dobj->scale.vec.f.x, 8u,
                                      &scalex) == FALSE) ||
            (ndsFtrLeanFloatPow2ToS32(dobj->scale.vec.f.y, 8u,
                                      &scaley) == FALSE) ||
            (ndsFtrLeanFloatPow2ToS32(dobj->scale.vec.f.z, 8u,
                                      &scalez) == FALSE) ||
            (scalex >= NDS_FTR_LEAN_SCALE_FAST_MAX) ||
            (scalex <= -NDS_FTR_LEAN_SCALE_FAST_MAX) ||
            (scaley >= NDS_FTR_LEAN_SCALE_FAST_MAX) ||
            (scaley <= -NDS_FTR_LEAN_SCALE_FAST_MAX) ||
            (scalez >= NDS_FTR_LEAN_SCALE_FAST_MAX) ||
            (scalez <= -NDS_FTR_LEAN_SCALE_FAST_MAX))
        {
            return FALSE;
        }
        cells[0] = (((cosp * cosy) >> 14) * scalex) >> 8;
        cells[1] = (((cosp * siny) >> 14) * scalex) >> 8;
        cells[2] = (-sinp * scalex) >> 7;
        a = (((sinr * sinp) >> 15) * cosy) >> 14;
        b = (cosr * siny) >> 14;
        cells[3] = ((a - b) * scaley) >> 8;
        a = (((sinr * sinp) >> 15) * siny) >> 14;
        b = (cosr * cosy) >> 14;
        cells[4] = ((a + b) * scaley) >> 8;
        cells[5] = (((sinr * cosp) >> 14) * scaley) >> 8;
        a = (((cosr * sinp) >> 15) * cosy) >> 14;
        b = (sinr * siny) >> 14;
        cells[6] = ((a + b) * scalez) >> 8;
        a = (((cosr * sinp) >> 15) * siny) >> 14;
        b = (sinr * cosy) >> 14;
        cells[7] = ((a - b) * scalez) >> 8;
        cells[8] = (((cosr * cosp) >> 14) * scalez) >> 8;
    }
    cells[9] = fixed_x;
    cells[10] = fixed_y;
    cells[11] = fixed_z;
    return TRUE;
}

/* ndsRendererAdapterF2LFixedWExact's conversions: source cell (r, c) lands in
 * local cell (r, c) (the pairing ndsRendererAdapterF2LDirect20p12 relies on).
 * FALSE where the builder would take syMatrixF2LFixedW instead. */
NDS_FTR_LEAN_KERNEL_INLINE s32 ndsFtrLeanWarmCells(const FTParts *parts,
                                                  s32 *cells)
{
    const f32 (*src)[4] = (const f32 (*)[4])parts->unk_dobjtrans_0x10;
    u32 row;
    u32 col;

    for (row = 0u; row < 4u; row++)
    {
        for (col = 0u; col < 3u; col++)
        {
            if (ndsFtrLeanFloatPow2ToS32(src[row][col], 16u,
                                         &cells[(row * 3u) + col]) == FALSE)
            {
                return FALSE;
            }
        }
    }
    return TRUE;
}

/* One joint's local (no animation locks): returns its class. `*need_slow`
 * says the source builder must produce the local instead (a conversion the
 * integer forms cannot reproduce, an XObj the builder refuses, or `no_fast`
 * for the scale/warm classes); otherwise `*has_local` and the cells are the
 * builder's exact result. */
NDS_FTR_LEAN_KERNEL_INLINE u32 ndsFtrLeanFastLocal(const NDSFtrLeanJoint *joint,
                                                  s32 *cells, u32 *has_local,
                                                  u32 no_fast, u32 *need_slow)
{
    DObj *dobj = joint->dobj;
    const FTParts *parts = joint->parts;
    union
    {
        f32 f;
        u32 u;
    } sx, sy, sz;

    *need_slow = FALSE;
    switch (joint->local_kind)
    {
    case NDS_FTR_LEAN_LOCAL_NONE:
        *has_local = FALSE;
        return nNDSFtrLeanJointNoLocal;
    case NDS_FTR_LEAN_LOCAL_PARTS:
        break;
    case NDS_FTR_LEAN_LOCAL_NO_GOBJ:
        *need_slow = TRUE;
        return nNDSFtrLeanJointNoGObj;
    default:
        *need_slow = TRUE;
        return nNDSFtrLeanJointXObj;
    }
    *has_local = TRUE;
    if (parts->transform_update_mode != 0)
    {
        if ((no_fast != 0u) || (ndsFtrLeanWarmCells(parts, cells) == FALSE))
        {
            *need_slow = TRUE;
            return (no_fast != 0u) ? nNDSFtrLeanJointWarm :
                nNDSFtrLeanJointConvert;
        }
        return nNDSFtrLeanJointWarm;
    }
    sx.f = dobj->scale.vec.f.x;
    sy.f = dobj->scale.vec.f.y;
    sz.f = dobj->scale.vec.f.z;
    if ((sx.u != NDS_FTR_LEAN_ONE_BITS) || (sy.u != NDS_FTR_LEAN_ONE_BITS) ||
        (sz.u != NDS_FTR_LEAN_ONE_BITS))
    {
        if ((no_fast != 0u) || (ndsFtrLeanTrsCells(dobj, cells, 1u) == FALSE))
        {
            *need_slow = TRUE;
            return (no_fast != 0u) ? nNDSFtrLeanJointScale :
                nNDSFtrLeanJointConvert;
        }
        return nNDSFtrLeanJointScale;
    }
    if (ndsFtrLeanTrsCells(dobj, cells, 0u) == FALSE)
    {
        *need_slow = TRUE;
        return nNDSFtrLeanJointConvert;
    }
    return nNDSFtrLeanJointFast;
}

/* ndsRendererAdapterSourceRoundShiftS64(v, 8) for an s32 v, which is also
 * ndsRendererRoundShiftS32Signed(v, 8) (TFX): round half away from zero in
 * the 32-bit magnitude domain, exact for every s32 including INT_MIN. */
NDS_FTR_LEAN_KERNEL_INLINE s32 ndsFtrLeanRoundShift8S32(s32 value)
{
    u32 bits = (u32)value;
    u32 magnitude = ((bits & 0x80000000u) != 0u) ? (0u - bits) : bits;
    u32 rounded = (magnitude + 0x80u) >> 8;

    return ((bits & 0x80000000u) != 0u) ? -(s32)rounded : (s32)rounded;
}

s32 NDS_FTR_LEAN_KERNEL_CODE
ndsFtrLeanKernelCompose(const NDSFtrLeanJoint *joints, u32 joint_count,
                        u32 *const *mv_sites,
                        NDSRendererMatrix20p12 *binding_worlds,
                        u32 world_mask,
                        u32 binding_count,
                        s32 shuffle_x, s32 shuffle_y,
                        u32 flags,
                        NDSFtrLeanSlowLocalFn slow,
                        u32 *class_counts,
                        u32 *part_ticks)
{
    /* Preorder keeps a joint's parent the latest joint one level up, so the
     * worlds (and the lock accumulators) live on a per-depth stack: small,
     * reused by every sibling subtree, and so cache-resident after the first
     * touch -- a per-joint array was a fresh, uncached line per joint on a
     * data cache that does not allocate on write. */
    NDSFtrLeanWorld stack[NDS_FTR_LEAN_DEPTH_MAX];
    f32 accum[NDS_FTR_LEAN_DEPTH_MAX][3];
    u32 counts[nNDSFtrLeanJointClassCount];
    u32 use_locks = flags & NDS_FTR_LEAN_KERNEL_LOCKS;
    u32 no_fast = flags & NDS_FTR_LEAN_KERNEL_NO_FAST;
#if NDS_FTR_LEAN_KTIME
    u32 mark = 0u;
#endif
    u32 j;

    if ((joints == NULL) || (slow == NULL) || (joint_count == 0u) ||
        (joint_count > NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX))
    {
        return FALSE;
    }
    for (j = 0u; j < nNDSFtrLeanJointClassCount; j++)
    {
        counts[j] = 0u;
    }
    for (j = 0u; j < joint_count; j++)
    {
        const NDSFtrLeanJoint *joint = &joints[j];
        u32 depth = joint->depth;
        NDSFtrLeanWorld *out;
        const NDSFtrLeanWorld *parent = NULL;
        s32 cells[12];
        u32 klass;
        u32 has_local = FALSE;
        u32 row;
        u32 col;

#if NDS_FTR_LEAN_KTIME
        if (part_ticks != NULL)
        {
            mark = cpuGetTiming();
        }
#endif
        if (depth >= NDS_FTR_LEAN_DEPTH_MAX)
        {
            return FALSE;
        }
        out = &stack[depth];
        /* The source compose's per-draw link check (fail 4) for the joints
         * the table keeps: every output depends only on its ancestor chain,
         * so unchanged parent links are all the live tree must still prove. */
        if (joint->parent != 0xffu)
        {
            if (((u32)joint->parent >= j) || (depth == 0u) ||
                (joint->dobj->parent != joints[joint->parent].dobj))
            {
                return FALSE;
            }
            parent = &stack[depth - 1u];
        }
        else if ((depth != 0u) || (joint->dobj->parent != DOBJ_PARENT_NULL))
        {
            return FALSE;
        }
        if (use_locks != 0u)
        {
            /* The source compose threads the lock accumulator down the tree:
             * unit at a topology root, the parent's (as the builder left it)
             * below. Every FighterParts joint takes the source builder, which
             * writes the part's vec_scale exactly as the old path's draw does;
             * a joint without one returns before touching the accumulator
             * (no local, world = parent), which needs no call. */
            if (parent == NULL)
            {
                accum[depth][0] = 1.0F;
                accum[depth][1] = 1.0F;
                accum[depth][2] = 1.0F;
            }
            else
            {
                accum[depth][0] = accum[depth - 1u][0];
                accum[depth][1] = accum[depth - 1u][1];
                accum[depth][2] = accum[depth - 1u][2];
            }
            if (joint->local_kind == NDS_FTR_LEAN_LOCAL_NONE)
            {
                klass = nNDSFtrLeanJointNoLocal;
                has_local = FALSE;
            }
            else
            {
                klass = (joint->local_kind == NDS_FTR_LEAN_LOCAL_PARTS) ?
                    nNDSFtrLeanJointLock :
                    (joint->local_kind == NDS_FTR_LEAN_LOCAL_NO_GOBJ) ?
                        nNDSFtrLeanJointNoGObj : nNDSFtrLeanJointXObj;
                if (slow(joint->dobj, accum[depth], cells, &has_local) ==
                    FALSE)
                {
                    return FALSE;
                }
            }
        }
        else
        {
            u32 need_slow;

            klass = ndsFtrLeanFastLocal(joint, cells, &has_local, no_fast,
                                        &need_slow);
            if (need_slow != FALSE)
            {
                f32 unit[3] = { 1.0F, 1.0F, 1.0F };

                /* No locks this draw: the builder passes the accumulator
                 * through untouched, so its value is never read. */
                if (slow(joint->dobj, unit, cells, &has_local) == FALSE)
                {
                    return FALSE;
                }
            }
        }
        counts[klass]++;
#if NDS_FTR_LEAN_KTIME
        if (part_ticks != NULL)
        {
            u32 now = cpuGetTiming();

            part_ticks[(klass >= nNDSFtrLeanJointLock) ? 3u : 0u] +=
                now - mark;
            mark = now;
        }
#endif

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
            /* ndsRendererAdapterSourceWorldMulLocal (out != parent: they sit
             * at different depths) */
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

#if NDS_FTR_LEAN_KTIME
        if (part_ticks != NULL)
        {
            u32 now = cpuGetTiming();

            part_ticks[1] += now - mark;
            mark = now;
        }
#endif
        if (joint->binding != 0xffu)
        {
            u32 binding = joint->binding;
            s32 basis[3][3];
            s32 translation[3];

            if (binding >= binding_count)
            {
                return FALSE;
            }
            /* ndsRendererAdapterSourceWorldTo20p12 */
            for (row = 0u; row < 3u; row++)
            {
                for (col = 0u; col < 3u; col++)
                {
                    basis[row][col] =
                        ndsFtrLeanRoundShift8S32(out->basis[row][col]);
                }
            }
            for (col = 0u; col < 3u; col++)
            {
                s64 value = ndsFtrLeanRoundShiftS64(out->translation[col], 8u);

                if ((value < (s64)(-2147483647 - 1)) ||
                    (value > (s64)2147483647))
                {
                    return FALSE;
                }
                translation[col] = (s32)value;
            }
            /* ndsRendererMtxMulAffine20p12(world, [I; sx sy 0]): every basis
             * product is a multiple of 4096 and rounds back exactly; row 3 is
             * (t + s) * 4096 >> 12 with the s32 clamp. */
            if ((shuffle_x != 0) || (shuffle_y != 0))
            {
                s64 tx = (s64)translation[0] + shuffle_x;
                s64 ty = (s64)translation[1] + shuffle_y;

                translation[0] = (tx > (s64)2147483647) ? 2147483647 :
                    (tx < (s64)(-2147483647 - 1)) ? (-2147483647 - 1) :
                    (s32)tx;
                translation[1] = (ty > (s64)2147483647) ? 2147483647 :
                    (ty < (s64)(-2147483647 - 1)) ? (-2147483647 - 1) :
                    (s32)ty;
            }
            if ((binding_worlds != NULL) && (binding < 32u) &&
                (((world_mask >> binding) & 1u) != 0u))
            {
                NDSRendererMatrix20p12 *world = &binding_worlds[binding];

                for (row = 0u; row < 3u; row++)
                {
                    world->m[row][0] = basis[row][0];
                    world->m[row][1] = basis[row][1];
                    world->m[row][2] = basis[row][2];
                    world->m[row][3] = 0;
                }
                world->m[3][0] = translation[0];
                world->m[3][1] = translation[1];
                world->m[3][2] = translation[2];
                world->m[3][3] = 1 << 12;
            }
            if ((mv_sites != NULL) && (mv_sites[binding] != NULL))
            {
                /* ndsFighterPacketStoreSplitModelview of that world, written
                 * straight into the list's LOAD4x4 parameters: rows 0-2 as
                 * they are, row 3 across the world-unit seam (4096 -> 16). */
                u32 *dst = mv_sites[binding];

                for (row = 0u; row < 3u; row++)
                {
                    dst[0] = (u32)basis[row][0];
                    dst[1] = (u32)basis[row][1];
                    dst[2] = (u32)basis[row][2];
                    dst[3] = 0u;
                    dst += 4;
                }
                dst[0] = (u32)ndsFtrLeanRoundShift8S32(translation[0]);
                dst[1] = (u32)ndsFtrLeanRoundShift8S32(translation[1]);
                dst[2] = (u32)ndsFtrLeanRoundShift8S32(translation[2]);
                dst[3] = (u32)ndsFtrLeanRoundShift8S32(1 << 12);
            }
        }
#if NDS_FTR_LEAN_KTIME
        if (part_ticks != NULL)
        {
            part_ticks[2] += cpuGetTiming() - mark;
        }
#endif
    }
#if !NDS_FTR_LEAN_KTIME
    (void)part_ticks;
#endif
    if (class_counts != NULL)
    {
        for (j = 0u; j < nNDSFtrLeanJointClassCount; j++)
        {
            class_counts[j] += counts[j];
        }
    }
    return TRUE;
}
