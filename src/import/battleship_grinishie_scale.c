/* Mushroom Kingdom (Inishie) moving-scale gameplay import.
 *
 * Source reference:
 * decomp/BattleShip-main/decomp/src/gr/grcommon/grinishie.c
 *
 * This file carries the live seesaw-scale gameplay: fighter weight/pressure
 * sampling and the Wait/Fall/Sleep/Retract update cycle dispatched by
 * grInishieScaleProcUpdate, plus the proof-shell DObj setup used when the
 * full stage import is absent. The retired source-scale preview (raw-asset
 * decode, native display-list reconstruction, generic display-list
 * interpretation, and the software execute-and-rasterize thumbnail) lives
 * host-only in src/host/graphics_reference/inishie_scale_reference.c and
 * must never re-enter a ROM link. The retired presentation entry points
 * below report a native failure instead of success.
 *
 * The full stage import (src/import/battleship_grinishie_ground.c, built
 * with the native stage flag) owns the complete source setup including
 * Pakkun, Power Block, and stage model loading.
 */
#include <PR/gbi.h>
#include <PR/ultratypes.h>
#include <ef/effect.h>
#include <ft/fighter.h>
#include <gr/ground.h>
#include <macros.h>
#include <nds/nds_renderer.h>
#include "nds_scene_harness_config.h"
#include <reloc_data.h>
#include <sc/scene.h>
#include <sys/objanim.h>
#include <sys/objman.h>

#define NDS_RELOC_LVALUE(offset) (*(uintptr_t *)(uintptr_t)(offset))
#define llGRInishieMapScaleDObjDesc NDS_RELOC_LVALUE(0x380u)
#define llGRInishieMapMapHead NDS_RELOC_LVALUE(0x5f0u)
#define llGRInishieMapScaleRetractAnimJoint NDS_RELOC_LVALUE(0x734u)

/* P2-4s7. These two tables are the source's own (grinishie.c:14,17), and
 * the full Mushroom Kingdom import defines them too. When that stage is
 * compiled it owns them; this file keeps them only for the older scale-only
 * configuration, which is the one that has no grinishie.c in the link. */
#if NDS_P2_STAGE_INISHIE
/* Owned by the full stage import; this file only reads them. */
extern u16 dGRInishieScaleMapObjKinds[];
extern u8 dGRInishieScaleLineGroups[];
#else
u16 dGRInishieScaleMapObjKinds[/* */] = { nMPMapObjKindScaleL,
                                          nMPMapObjKindScaleR };

u8 dGRInishieScaleLineGroups[/* */] = { 0x01, 0x02 };
#endif

/* Retired source-scale setup asset provider. The implementation lives
 * host-only; this stub reports the retired presentation path as a native
 * failure and returns no asset, never success for empty rendering. */
void *ndsGRInishieScaleGetSourceSetupMapHead(void)
{
    ndsRendererRecordNativeFailure(NDS_NATIVE_FAILURE_STAGE,
        (u32)gSCManagerSceneData.scene_curr, 0xffffffffu, nGRKindInishie,
        0x5f0u, 0u, NDS_NATIVE_FAILURE_NO_PROGRAM);
    return NULL;
}

enum grInishieScaleStatus
{
    nGRInishieScaleStatusWait,
    nGRInishieScaleStatusFall,
    nGRInishieScaleStatusSleep,
    nGRInishieScaleStatusRetract
};

#if !NDS_P2_STAGE_INISHIE
void grInishieScaleUpdateFighterStatsGA(void)
{
    GObj *fighter_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];

    while (fighter_gobj != NULL)
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);
        s32 player = fp->player;

        if (fp->ga == nMPKineticsGround)
        {
            if (gGRCommonStruct.inishie.players_ga[player] !=
                nMPKineticsGround)
            {
                gGRCommonStruct.inishie.players_tt[player] = 1;
            }
            else if (gGRCommonStruct.inishie.players_tt[player] != 0)
            {
                gGRCommonStruct.inishie.players_tt[player]--;
            }
        }
        else
        {
            gGRCommonStruct.inishie.players_tt[player] = 0;
        }

        gGRCommonStruct.inishie.players_ga[player] = fp->ga;

        fighter_gobj = fighter_gobj->link_next;
    }
}
#endif

#if !NDS_P2_STAGE_INISHIE
f32 grInishieScaleGetPressure(s32 line_id)
{
    GObj *fighter_gobj = gGCCommonLinks[nGCCommonLinkIDFighter];
    f32 pressure = 0.0F;

    while (fighter_gobj != NULL)
    {
        FTStruct *fp = ftGetStruct(fighter_gobj);

        if (fp->ga == nMPKineticsGround)
        {
            if ((fp->coll_data.floor_line_id != -2) &&
                (mpCollisionSetDObjNoID(fp->coll_data.floor_line_id) ==
                    line_id))
            {
                f32 weight = (1.0F - fp->attr->weight) + 1.4F;

                if (gGRCommonStruct.inishie.players_tt[fp->player] != 0)
                {
                    pressure += (weight * 8.0F);
                }
                else
                {
                    pressure += weight;
                }
            }
        }
        fighter_gobj = fighter_gobj->link_next;
    }
    return pressure;
}
#endif

#if !NDS_P2_STAGE_INISHIE
void grInishieScaleUpdateWait(void)
{
    DObj *l_dobj;
    DObj *r_dobj;
    f32 l_weight;
    f32 r_weight;
    f32 alt;
    sb32 ud;

    grInishieScaleUpdateFighterStatsGA();

    l_weight = grInishieScaleGetPressure(dGRInishieScaleLineGroups[0]);
    r_weight = grInishieScaleGetPressure(dGRInishieScaleLineGroups[1]);

    if ((l_weight == 0.0F) && (r_weight == 0.0F))
    {
        if (gGRCommonStruct.inishie.splat_alt != 0.0F)
        {
            if (gGRCommonStruct.inishie.splat_alt < 0.0F)
            {
                gGRCommonStruct.inishie.splat_alt += 8.0F;

                if (gGRCommonStruct.inishie.splat_alt > 0.0F)
                {
                    gGRCommonStruct.inishie.splat_alt = 0.0F;
                }
            }
            else
            {
                gGRCommonStruct.inishie.splat_alt -= 8.0F;

                if (gGRCommonStruct.inishie.splat_alt < 0.0F)
                {
                    gGRCommonStruct.inishie.splat_alt = 0.0F;
                }
            }
        }
        gGRCommonStruct.inishie.splat_accelerate = 0.0F;
    }
    else
    {
        gGRCommonStruct.inishie.splat_accelerate += (r_weight - l_weight);

        if ((l_weight != 0.0F) && (r_weight != 0.0F) &&
            (gGRCommonStruct.inishie.splat_accelerate != 0.0F))
        {
            gGRCommonStruct.inishie.splat_accelerate *= 0.93F;
        }
        else if (gGRCommonStruct.inishie.splat_accelerate > 0.0F)
        {
            gGRCommonStruct.inishie.splat_accelerate -= 0.9F;

            if (gGRCommonStruct.inishie.splat_accelerate < 0.0F)
            {
                gGRCommonStruct.inishie.splat_accelerate = 0.0F;
            }
        }
        else if (gGRCommonStruct.inishie.splat_accelerate < 0.0F)
        {
            gGRCommonStruct.inishie.splat_accelerate += 0.9F;

            if (gGRCommonStruct.inishie.splat_accelerate > 0.0F)
            {
                gGRCommonStruct.inishie.splat_accelerate = 0.0F;
            }
        }
        gGRCommonStruct.inishie.splat_alt +=
            gGRCommonStruct.inishie.splat_accelerate;
    }
    alt = ABSF(gGRCommonStruct.inishie.splat_alt);

    l_dobj = gGRCommonStruct.inishie.scale[0].platform_dobj;
    r_dobj = gGRCommonStruct.inishie.scale[1].platform_dobj;

    if (alt > 1100.0F)
    {
        ud = 0;

        if (gGRCommonStruct.inishie.splat_alt < 0.0F)
        {
            ud = 1;

            if (gGRCommonStruct.inishie.splat_accelerate != 0.0F)
            {
            }
        }
        gGRCommonStruct.inishie.splat_accelerate = 0.0F;

        if (ud != 0)
        {
            gGRCommonStruct.inishie.splat_alt = -1100.0F;
        }
        else
        {
            gGRCommonStruct.inishie.splat_alt = 1100.0F;
        }

        gGRCommonStruct.inishie.splat_status = nGRInishieScaleStatusFall;

        efManagerSparkleWhiteScaleMakeEffect(&l_dobj->translate.vec.f, 1.0F);
        efManagerSparkleWhiteScaleMakeEffect(&r_dobj->translate.vec.f, 1.0F);
    }
    l_dobj->translate.vec.f.y =
        gGRCommonStruct.inishie.scale[0].platform_base_y +
        gGRCommonStruct.inishie.splat_alt;
    r_dobj->translate.vec.f.y =
        gGRCommonStruct.inishie.scale[1].platform_base_y -
        gGRCommonStruct.inishie.splat_alt;

    gGRCommonStruct.inishie.scale[0].string_dobj->translate.vec.f.y =
        l_dobj->translate.vec.f.y -
        gGRCommonStruct.inishie.scale[0].string_length;
    gGRCommonStruct.inishie.scale[1].string_dobj->translate.vec.f.y =
        r_dobj->translate.vec.f.y -
        gGRCommonStruct.inishie.scale[1].string_length;
}
#endif

#if !NDS_P2_STAGE_INISHIE
void grInishieScaleUpdateFall(void)
{
    f32 deadzone;

    gGRCommonStruct.inishie.splat_accelerate += 3.0F;

    if (gGRCommonStruct.inishie.splat_accelerate > 70.0F)
    {
        gGRCommonStruct.inishie.splat_accelerate = 70.0F;
    }
    gGRCommonStruct.inishie.scale[0].platform_dobj->translate.vec.f.y -=
        gGRCommonStruct.inishie.splat_accelerate;
    gGRCommonStruct.inishie.scale[1].platform_dobj->translate.vec.f.y -=
        gGRCommonStruct.inishie.splat_accelerate;

    deadzone = gMPCollisionGroundData->map_bound_bottom + (-1000.0F);

    if ((gGRCommonStruct.inishie.scale[0].platform_dobj->translate.vec.f.y <
            deadzone) &&
        (gGRCommonStruct.inishie.scale[1].platform_dobj->translate.vec.f.y <
            deadzone))
    {
        gGRCommonStruct.inishie.splat_status = nGRInishieScaleStatusSleep;
        gGRCommonStruct.inishie.splat_accelerate = 0.0F;

        mpCollisionSetYakumonoOffID(dGRInishieScaleLineGroups[0]);
        mpCollisionSetYakumonoOffID(dGRInishieScaleLineGroups[1]);

        gGRCommonStruct.inishie.splat_wait = 180;
    }
}
#endif

#if !NDS_P2_STAGE_INISHIE
void grInishieScaleUpdateStep(void)
{
    gGRCommonStruct.inishie.splat_wait--;

    if (gGRCommonStruct.inishie.splat_wait == 0)
    {
        gGRCommonStruct.inishie.splat_status =
            nGRInishieScaleStatusRetract;

        gcAddDObjAnimJoint(
            gGRCommonStruct.inishie.scale[0].platform_dobj,
            (AObjEvent32 *)((intptr_t)&llGRInishieMapScaleRetractAnimJoint +
                            (uintptr_t)gGRCommonStruct.inishie.map_head),
            0.0F);
        gcAddDObjAnimJoint(
            gGRCommonStruct.inishie.scale[1].platform_dobj,
            (AObjEvent32 *)((intptr_t)&llGRInishieMapScaleRetractAnimJoint +
                            (uintptr_t)gGRCommonStruct.inishie.map_head),
            0.0F);
    }
}
#endif

#if !NDS_P2_STAGE_INISHIE
void grInishieScaleUpdateRetract(void)
{
    DObj *l_dobj;
    DObj *r_dobj;
    sb32 is_complete = FALSE;

    if (gGRCommonStruct.inishie.splat_alt != 0.0F)
    {
        if (gGRCommonStruct.inishie.splat_alt < 0.0F)
        {
            gGRCommonStruct.inishie.splat_alt += 10.0F;

            if (gGRCommonStruct.inishie.splat_alt >= 0.0F)
            {
                is_complete = TRUE;
            }
        }
        else
        {
            gGRCommonStruct.inishie.splat_alt -= 10.0F;

            if (gGRCommonStruct.inishie.splat_alt <= 0.0F)
            {
                is_complete = TRUE;
            }
        }
    }
    l_dobj = gGRCommonStruct.inishie.scale[0].platform_dobj;
    r_dobj = gGRCommonStruct.inishie.scale[1].platform_dobj;

    if (is_complete != FALSE)
    {
        gGRCommonStruct.inishie.splat_alt = 0.0F;

        l_dobj->anim_wait = AOBJ_ANIM_NULL;
        l_dobj->flags = DOBJ_FLAG_NONE;

        mpCollisionSetYakumonoOnID(dGRInishieScaleLineGroups[0]);

        r_dobj->anim_wait = AOBJ_ANIM_NULL;
        r_dobj->flags = DOBJ_FLAG_NONE;

        mpCollisionSetYakumonoOnID(dGRInishieScaleLineGroups[1]);

        gGRCommonStruct.inishie.splat_status = nGRInishieScaleStatusWait;
    }
    l_dobj->translate.vec.f.y =
        gGRCommonStruct.inishie.scale[0].platform_base_y +
        gGRCommonStruct.inishie.splat_alt;
    r_dobj->translate.vec.f.y =
        gGRCommonStruct.inishie.scale[1].platform_base_y -
        gGRCommonStruct.inishie.splat_alt;

    gGRCommonStruct.inishie.scale[0].string_dobj->translate.vec.f.y =
        l_dobj->translate.vec.f.y -
        gGRCommonStruct.inishie.scale[0].string_length;
    gGRCommonStruct.inishie.scale[1].string_dobj->translate.vec.f.y =
        r_dobj->translate.vec.f.y -
        gGRCommonStruct.inishie.scale[1].string_length;
}
#endif

#if !NDS_P2_STAGE_INISHIE
void grInishieScaleProcUpdate(GObj *ground_gobj)
{
    (void)ground_gobj;

    switch (gGRCommonStruct.inishie.splat_status)
    {
    case nGRInishieScaleStatusWait:
        grInishieScaleUpdateWait();
        break;

    case nGRInishieScaleStatusFall:
        grInishieScaleUpdateFall();
        break;

    case nGRInishieScaleStatusSleep:
        grInishieScaleUpdateStep();
        break;

    case nGRInishieScaleStatusRetract:
        grInishieScaleUpdateRetract();
        break;
    }
    mpCollisionSetYakumonoPosID(
        dGRInishieScaleLineGroups[0],
        &gGRCommonStruct.inishie.scale[0].platform_dobj->translate.vec.f);
    mpCollisionSetYakumonoPosID(
        dGRInishieScaleLineGroups[1],
        &gGRCommonStruct.inishie.scale[1].platform_dobj->translate.vec.f);
}
#endif

#if !NDS_P2_STAGE_INISHIE
/* Retired source-scale ground setup. The implementation lives host-only;
 * this stub reports the retired presentation path as a native failure. */
void grInishieMakeScale(void)
{
    ndsRendererRecordNativeFailure(NDS_NATIVE_FAILURE_STAGE,
        (u32)gSCManagerSceneData.scene_curr, 0xffffffffu, nGRKindInishie,
        0x380u, 0u, NDS_NATIVE_FAILURE_NO_PROGRAM);
}
#endif

static DObj *ndsGRInishieScaleMakeProofDObj(f32 x, f32 y)
{
    GObj *gobj =
        gcMakeGObjSPAfter(nGCCommonKindGround, NULL, nGCCommonLinkIDGround,
                          GOBJ_PRIORITY_DEFAULT);
    DObj *dobj;

    if (gobj == NULL)
    {
        return NULL;
    }

    dobj = gcAddDObjForGObj(gobj, NULL);
    if (dobj == NULL)
    {
        return NULL;
    }

    gcAddXObjForDObjFixed(dobj, nGCMatrixKindTra, 0);
    dobj->translate.vec.f.x = x;
    dobj->translate.vec.f.y = y;
    dobj->translate.vec.f.z = 0.0F;
    return dobj;
}

void ndsGRInishieScaleMakeProofShell(void)
{
    static const Vec3f positions[2] = {
        { -417.0F, 363.0F, 0.0F },
        { 420.0F, 362.0F, 0.0F },
    };
    s32 i;

    gGRCommonStruct.inishie.item_head = NULL;
    gGRCommonStruct.inishie.splat_status = nGRInishieScaleStatusWait;
    gGRCommonStruct.inishie.splat_alt = 80.0F;
    gGRCommonStruct.inishie.splat_accelerate = 0.0F;
    gGRCommonStruct.inishie.splat_wait = 0;

    for (i = 0; i < 4; i++)
    {
        gGRCommonStruct.inishie.players_tt[i] = 0;
        gGRCommonStruct.inishie.players_ga[i] = 0;
    }

    for (i = 0; i < 2; i++)
    {
        gGRCommonStruct.inishie.scale[i].platform_dobj =
            ndsGRInishieScaleMakeProofDObj(positions[i].x, positions[i].y);
        gGRCommonStruct.inishie.scale[i].string_dobj =
            ndsGRInishieScaleMakeProofDObj(positions[i].x,
                                           positions[i].y - 240.0F);
        gGRCommonStruct.inishie.scale[i].string_length = 240.0F;
        gGRCommonStruct.inishie.scale[i].platform_base_y = positions[i].y;
    }
}
