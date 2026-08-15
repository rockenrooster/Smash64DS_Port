/* R2-07 slice 52 -- the wired ring. include/nds/nds_r2_collision_ring.h states
 * what is converted, what is not, and why the split is the correctness
 * argument; this file is the transcription of func_ovl2_800EDBA4,
 * func_ovl2_800EDE00 and func_ovl2_800EDE5C (gm/gmcollision.c:332, :455, :472)
 * with the arithmetic type changed and nothing else.
 *
 * THIS TRANSLATION UNIT IS THUMB ON PURPOSE. Every 64-bit product in the
 * cluster lives behind the ndsR2CollisionFixed* entry points in
 * src/port/nds_r2_collision_fixed.c, which the Makefile builds -marm and
 * scripts/check-r2-collision-fixed.ps1 disassembles: it fails the build on any
 * __aeabi_lmul and on soft float outside the declared sine-table edge. Inlining
 * that arithmetic here instead would put SMULL-shaped code in a translation
 * unit with no SMULL, turn every multiply into a library call, and no gate
 * would notice -- memory "Thumb hides 64-bit cost" is the record of what that
 * costs. So this file holds only control flow, f32 loads and f32 stores, and
 * pays one interworking branch per stage for it.
 *
 * ORDER IS THE SOURCE'S. gmCollisionCheckFighterAttackDamageCollide runs
 * func_ovl2_800EDE00 and then func_ovl2_800EDE5C, and each runs
 * func_ovl2_800EDBA4 first if unk_dobjtrans_0x5 is clear. Chain, then inverse,
 * then scales, reading the same latches in the same sequence.
 */

#include <ft/fighter.h>
#include <macros.h>

#include <nds/nds_r2_collision_fixed.h>
#include <nds/nds_r2_collision_ring.h>

/* The source's own bound: FTParts chains climb into a DObj *setup_dobj[18].
 * Reproducing the overflow rather than the bound would be reproducing a defect,
 * so a deeper chain declines to the float path, which has the same array and
 * the same limit. */
#define NDS_R2_CFX_CHAIN_MAX 18

/* THE FALSIFIER ARM, and it is a `volatile` for a measurement reason rather
 * than a style one.
 *
 * The cross-build P95 floor on this ROM is ~17,000 and a measured one-line
 * candidate spread is +/-24,064, so a two-build comparison of a change this
 * size measures the linker. The usual answer -- flag on, dispatch reverted --
 * does not work here, because with nothing calling the ring `--gc-sections`
 * takes the whole cluster back out and the "candidate layout" arm is the
 * control layout again.
 *
 * A volatile read the compiler cannot fold fixes that: BOTH arms compile and
 * link byte-identical code, every kernel stays referenced, and the arms differ
 * in one initialised word of `.data`. `arm-none-eabi-objcopy --only-section
 * .text` on the two ELFs is then expected to report IDENTICAL, which turns the
 * placement floor from "+/-17,000, assumed" into "zero, verified".
 *
 * `section(".data")` is load-bearing and was added after reading the first
 * arm's symbol table. Without it the falsifier arm's zero initialiser puts this
 * word in `.bss` while the candidate arm's 1 puts it in `.data` -- so the two
 * arms would differ by four bytes of `.data` size and four of `.bss`, moving
 * every global after it and reintroducing exactly the data-placement confound
 * the volatile was there to remove. Forced into `.data` it occupies the same
 * four bytes at the same address in both. */
volatile u32 gNdsCfxRingEnable __attribute__((used, section(".data"))) =
    NDS_R2_COLLISION_FIXED_DISPATCH;

#if NDS_TICK_HUD
volatile u32 gNdsCfxRingPrepareCalls __attribute__((used));
volatile u32 gNdsCfxRingChainFixed __attribute__((used));
volatile u32 gNdsCfxRingChainDeclined __attribute__((used));
volatile u32 gNdsCfxRingLocalsBuilt __attribute__((used));
volatile u32 gNdsCfxRingComposes __attribute__((used));
volatile u32 gNdsCfxRingInvertFixed __attribute__((used));
volatile u32 gNdsCfxRingInvertDeclined __attribute__((used));
volatile u32 gNdsCfxRingScaleFixed __attribute__((used));
volatile u32 gNdsCfxRingScaleDeclined __attribute__((used));
#define NDS_R2_CFX_COUNT(counter) ((counter)++)
#else
#define NDS_R2_CFX_COUNT(counter) ((void)0)
#endif

#if NDS_R2_COLLISION_FIXED_NARROW
/* R2-07 slice 53's switch. Same construction, same reasons, as
 * gNdsCfxRingEnable above -- `volatile` so no fold can reach it and
 * section(".data") so a zero initialiser cannot migrate it to .bss and move
 * every global after it. */
volatile u32 gNdsCfxNarrowEnable __attribute__((used, section(".data"))) =
    NDS_R2_COLLISION_FIXED_NARROW_DISPATCH;

#if NDS_TICK_HUD
volatile u32 gNdsCfxNarrowCalls __attribute__((used));
volatile u32 gNdsCfxNarrowAnswered __attribute__((used));
volatile u32 gNdsCfxNarrowHits __attribute__((used));
volatile u32 gNdsCfxNarrowDeclined __attribute__((used));
#endif
#endif

/* gmCollisionTransformMatrixAll (gm/gmcollision.c:29) in fixed point.
 *
 * The three Vec3f are copied into plain float[3] rather than cast in place:
 * Vec3f is three f32 with no padding on this target and the cast would work,
 * but it is an aliasing assumption for no gain -- these are register moves next
 * to what the call does.
 *
 * The source skips the per-row scale multiply when the scale is exactly 1.0F.
 * ndsR2CfxBuildLocal always applies it, which is the same number: 1.0F converts
 * to exactly 2^26 at Q26 and the reduction is exact. The falsifier's float
 * reference keeps the conditionals, so the graded comparison covers the
 * difference rather than assuming it away. */
static int ndsR2CfxLocalFromDObj(NDSR2CfxMtx *dst, DObj *dobj)
{
    float rotate[3];
    float scale[3];
    float translate[3];

    rotate[0] = dobj->rotate.vec.f.x;
    rotate[1] = dobj->rotate.vec.f.y;
    rotate[2] = dobj->rotate.vec.f.z;
    scale[0] = dobj->scale.vec.f.x;
    scale[1] = dobj->scale.vec.f.y;
    scale[2] = dobj->scale.vec.f.z;
    translate[0] = dobj->translate.vec.f.x;
    translate[1] = dobj->translate.vec.f.y;
    translate[2] = dobj->translate.vec.f.z;

    NDS_R2_CFX_COUNT(gNdsCfxRingLocalsBuilt);
    return ndsR2CollisionFixedBuildLocal(dst, gSYSinTable, rotate, scale,
                                         translate);
}

/* func_ovl2_800EDBA4 (gm/gmcollision.c:332), is_use_animlocks == FALSE branch.
 *
 * The animlocks branch is NOT here. It builds its locals with
 * gmCollisionSetMatrixNcs, which divides column c by scale_mul[c] and is the
 * one thing in this cluster that can make the rows non-orthogonal; it is
 * outside the falsifier's domain and outside this cycle. Such a joint declines
 * and the float path takes it, vec_scale included -- which matters, because on
 * that branch vec_scale is the accumulated DObj scale and not the row
 * magnitudes, so a fixed axis-scale write there would be a different quantity,
 * not a rounder one.
 *
 * A decline part-way down the descent is safe and is not unwound. Every level
 * this function commits is complete and correct on its own; the decomp's own
 * chain walk restarts from main_dobj and stops at the first ancestor whose
 * unk_dobjtrans_0x5 is set, so it picks up exactly where this left off.
 *
 * Returns 1 when main_dobj's own world matrix is now valid. */
static int ndsR2CfxBuildChain(DObj *main_dobj)
{
    DObj *setup_dobj[NDS_R2_CFX_CHAIN_MAX];
    DObj *second_dobj;
    FTStruct *fp;
    FTParts *parts;
    NDSR2CfxMtx acc;
    s32 depth;
    s32 i;

    if (main_dobj->parent_gobj == NULL)
    {
        return 0;
    }
    fp = ftGetStruct(main_dobj->parent_gobj);
    if ((fp == NULL) || (fp->is_use_animlocks != FALSE))
    {
        return 0;
    }

    second_dobj = main_dobj;
    depth = 0;

    for (;;)
    {
        parts = ftGetParts(second_dobj);

        if (parts == NULL)
        {
            return 0;
        }
        if (parts->unk_dobjtrans_0x5 != 0)
        {
            /* The boundary. This ancestor's world matrix was built by someone
             * -- this function on an earlier joint, or the decomp float path,
             * or the renderer -- and is f32, so it is read back in. One
             * quantisation per chain, not one per level. */
            if (ndsR2CollisionFixedLoadF32(&acc, parts->mtx_translate) == 0)
            {
                return 0;
            }
            break;
        }
        if (second_dobj->parent == DOBJ_PARENT_NULL)
        {
            if (parts->transform_update_mode == 0)
            {
                if (ndsR2CfxLocalFromDObj(&acc, second_dobj) == 0)
                {
                    return 0;
                }
                ndsR2CollisionFixedStoreF32(parts->unk_dobjtrans_0x10, &acc);
                parts->transform_update_mode = 1;
            }
            else if (ndsR2CollisionFixedLoadF32(&acc,
                                                parts->unk_dobjtrans_0x10) == 0)
            {
                return 0;
            }
            /* gmCollisionCopyMatrix(mtx_translate, unk_dobjtrans_0x10) */
            ndsR2CollisionFixedStoreF32(parts->mtx_translate, &acc);
            parts->unk_dobjtrans_0x5 = 1;
            break;
        }
        if (depth >= NDS_R2_CFX_CHAIN_MAX)
        {
            return 0;
        }
        setup_dobj[depth] = second_dobj;
        second_dobj = second_dobj->parent;
        depth++;
    }

    for (i = depth - 1; i >= 0; i--)
    {
        FTParts *current = ftGetParts(setup_dobj[i]);
        NDSR2CfxMtx local;
        NDSR2CfxMtx world;

        if (current == NULL)
        {
            return 0;
        }
        if (current->transform_update_mode == 0)
        {
            if (ndsR2CfxLocalFromDObj(&local, setup_dobj[i]) == 0)
            {
                return 0;
            }
            ndsR2CollisionFixedStoreF32(current->unk_dobjtrans_0x10, &local);
            current->transform_update_mode = 1;
        }
        else if (ndsR2CollisionFixedLoadF32(&local,
                                            current->unk_dobjtrans_0x10) == 0)
        {
            return 0;
        }

        NDS_R2_CFX_COUNT(gNdsCfxRingComposes);
        if (ndsR2CollisionFixedCompose(&world, &acc, &local) == 0)
        {
            return 0;
        }
        ndsR2CollisionFixedStoreF32(current->mtx_translate, &world);
        current->unk_dobjtrans_0x5 = 1;
        acc = world;
    }
    return 1;
}

void ndsR2CfxPrepareFighterJoint(DObj *main_dobj)
{
    FTParts *parts;

    if (gNdsCfxRingEnable == 0u)
    {
        return; /* falsifier arm: same bytes, no dispatch, every counter 0 */
    }
    if (main_dobj == NULL)
    {
        return;
    }
    parts = ftGetParts(main_dobj);
    if (parts == NULL)
    {
        return;
    }
    NDS_R2_CFX_COUNT(gNdsCfxRingPrepareCalls);

    if ((parts->unk_dobjtrans_0x7 != 0) && (parts->unk_dobjtrans_0x6 != 0))
    {
        return; /* both latches already set: the decomp would do nothing too */
    }

    if (parts->unk_dobjtrans_0x5 == 0)
    {
        if (ndsR2CfxBuildChain(main_dobj) == 0)
        {
            NDS_R2_CFX_COUNT(gNdsCfxRingChainDeclined);
            return; /* leave every latch clear; the float path owns this joint */
        }
        NDS_R2_CFX_COUNT(gNdsCfxRingChainFixed);
    }

    /* func_ovl2_800EDE00. unk_dobjtrans_0x9C keeps its Mtx44f layout and all
     * nine of its readers; only the arithmetic that fills it changes. */
    if (parts->unk_dobjtrans_0x7 == 0)
    {
        if (ndsR2CollisionFixedInvertF32(parts->unk_dobjtrans_0x9C,
                                         parts->mtx_translate) != 0)
        {
            parts->unk_dobjtrans_0x7 = 1;
            NDS_R2_CFX_COUNT(gNdsCfxRingInvertFixed);
        }
        else
        {
            NDS_R2_CFX_COUNT(gNdsCfxRingInvertDeclined);
        }
    }

    /* func_ovl2_800EDE5C. Only reachable with unk_dobjtrans_0x6 clear, which on
     * the non-animlocks branch means vec_scale is the row magnitudes of
     * mtx_translate and nothing else -- the same quantity this writes. */
    if (parts->unk_dobjtrans_0x6 == 0)
    {
        float scales[3];

        if (ndsR2CollisionFixedAxisScalesF32(scales, parts->mtx_translate) != 0)
        {
            parts->vec_scale.x = scales[0];
            parts->vec_scale.y = scales[1];
            parts->vec_scale.z = scales[2];
            parts->unk_dobjtrans_0x6 = 1;
            NDS_R2_CFX_COUNT(gNdsCfxRingScaleFixed);
        }
        else
        {
            NDS_R2_CFX_COUNT(gNdsCfxRingScaleDeclined);
        }
    }
}

#if NDS_R2_COLLISION_FIXED_NARROW
/* Q12 conversion with the same domain guard ndsR2CfxLoadF32 applies to row 3.
 * Integer only -- ndsR2CollisionF32ToFixed is exponent arithmetic on the IEEE
 * bits with no 64-bit product, so it stays legal in this Thumb translation unit
 * where an SMULL would have become __aeabi_lmul. */
static int ndsR2CfxPosQ12(int32_t *out, float value)
{
    int32_t q = ndsR2CollisionF32ToFixed(value, NDS_R2_CFX_POS_BITS);

    if ((q == NDS_R2_COLLISION_F32_OVERFLOW) ||
        (ndsR2CfxAbs32(q) >= NDS_R2_CFX_POS_MAX))
    {
        return 0;
    }
    *out = q;
    return 1;
}

static int ndsR2CfxPosQ12Vec(int32_t out[3], const Vec3f *v)
{
    return ndsR2CfxPosQ12(&out[0], v->x) && ndsR2CfxPosQ12(&out[1], v->y) &&
           ndsR2CfxPosQ12(&out[2], v->z);
}

/* gmCollisionCheckFighterAttackDamageCollide's tail (gm/gmcollision.c:1379).
 *
 * The source's two producer calls are absent because they already ran: the
 * wrapper calls ndsR2CfxPrepareFighterJoint first, which is func_ovl2_800EDE00
 * and func_ovl2_800EDE5C in fixed point and sets the same three latches. This
 * requires all three, so a joint prepare declined is a joint the decomp body
 * takes -- side effects included.
 *
 * The frame is built from mtx_translate rather than read from
 * unk_dobjtrans_0x9C. Those are the same transform, but ndsR2CfxMakeFrameCofactor
 * keeps the forward translation and subtracts it from the query point instead of
 * storing -t.R^-1, which is the numerically better half of the pair and the one
 * the falsifier grades. It also yields inv_scale, so the source's three
 * `radius / scale` divides become three multiplies of values already in hand. */
int ndsR2CfxTestFighterDamage(struct FTAttackColl *attack_coll,
                              struct FTDamageColl *damage_coll)
{
    FTAttackColl *attack = (FTAttackColl *)attack_coll;
    FTDamageColl *damage = (FTDamageColl *)damage_coll;
    FTParts *parts;
    NDSR2CfxMtx world;
    NDSR2CfxFrame frame;
    int32_t pos_curr[3];
    int32_t pos_prev[3];
    int32_t offset[3];
    int32_t size[3];
    int32_t radius;
    int result;

    if (gNdsCfxNarrowEnable == 0u)
    {
        return NDS_R2_CFX_NARROW_DECLINE; /* falsifier arm: same bytes, no dispatch */
    }
    NDS_R2_CFX_COUNT(gNdsCfxNarrowCalls);

    parts = ftGetParts(damage->joint);
    if (parts == NULL)
    {
        NDS_R2_CFX_COUNT(gNdsCfxNarrowDeclined);
        return NDS_R2_CFX_NARROW_DECLINE;
    }
    /* All three latches, because the decomp body the caller would otherwise run
     * is what fills unk_dobjtrans_0x9C and vec_scale. Prepare set them or it
     * did not. */
    if ((parts->unk_dobjtrans_0x5 == 0) || (parts->unk_dobjtrans_0x6 == 0) ||
        (parts->unk_dobjtrans_0x7 == 0))
    {
        NDS_R2_CFX_COUNT(gNdsCfxNarrowDeclined);
        return NDS_R2_CFX_NARROW_DECLINE;
    }

    if ((ndsR2CollisionFixedLoadF32(&world, parts->mtx_translate) == 0) ||
        (ndsR2CollisionFixedMakeFrame(&frame, &world) == 0) ||
        (ndsR2CfxPosQ12Vec(pos_curr, &attack->pos_curr) == 0) ||
        (ndsR2CfxPosQ12Vec(pos_prev, &attack->pos_prev) == 0) ||
        (ndsR2CfxPosQ12Vec(offset, &damage->offset) == 0) ||
        (ndsR2CfxPosQ12Vec(size, &damage->size) == 0) ||
        (ndsR2CfxPosQ12(&radius, attack->size) == 0))
    {
        NDS_R2_CFX_COUNT(gNdsCfxNarrowDeclined);
        return NDS_R2_CFX_NARROW_DECLINE;
    }

    result = ndsR2CollisionFixedTestRectangle(pos_curr, pos_prev, radius,
                                              attack->attack_state == 2, &frame,
                                              offset, size, frame.inv_scale);
    if (result == NDS_R2_CFX_DECLINE)
    {
        NDS_R2_CFX_COUNT(gNdsCfxNarrowDeclined);
        return NDS_R2_CFX_NARROW_DECLINE;
    }
    NDS_R2_CFX_COUNT(gNdsCfxNarrowAnswered);
    if (result != 0)
    {
        NDS_R2_CFX_COUNT(gNdsCfxNarrowHits);
    }
    return result;
}
#endif /* NDS_R2_COLLISION_FIXED_NARROW */
