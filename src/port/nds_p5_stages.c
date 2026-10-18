/* P5: the Remix stages' ground routines and hazards (docs/P5/New_Stages.md),
 * read as assembled from the staged Remix build.
 *
 * Bowser's Stadium (docs/P5/stages/bowsers_stadium.md) runs Peach's Castle's
 * ground maker, whose grCastleInitAll call Remix replaces (Hazards.asm
 * peach_bumper_, patch at 0x8010B4B4): no bumper, no map-node animation, and
 * with hazards on (always: the profile has no hazard toggle) two bombs. A bomb
 * is the source's Race to the Finish bomb (nITKindTaruBomb) made from Remix's
 * own header file (add_item_, File.BOWSER_BOMB_HEADER 0x9B2, whose ITAttributes
 * at 0xA8 name the model in 0x9B1), hovering by a bottom ECB of -416 at
 * x +-2990, y 512. Each side has a controller object
 * (replenish_bowser_bombs_): while its bomb lives and has not exploded its
 * timer holds at 300; once the bomb explodes or is destroyed the timer counts
 * down and at 0 the side's bomb is remade, with its hitbox off and a 120-frame
 * flash (bowser_bomb_flash_main), then the hitbox on and FGM 0xC2.
 *
 * The source's TaruBomb code runs these bombs; the stage's patches to it are
 * the hooks below, called from battleship_item_tarubomb.c, the item destroy,
 * ftmain's hit processing and the damage routine:
 *  - bowser_bomb_explosion_: the explosion plays FGM 0x01, skips the shrapnel
 *    effect and sets the side's respawn flag;
 *  - bowser_bomb_air_collision_: the landing sound is FGM 0xC2, not 0x47;
 *  - track_hit_object_ and bomb_hitstun_: a fighter last hit by a bomb takes
 *    hitstun times 9 / (gravity + 4).
 *
 * Read as assembled, and kept: a remade bomb's main routine is the flash and
 * then none at all (the first two have the TaruBomb's fall), so it hangs where
 * it was made; Remix renumbers a remade bomb 0x11A, which only its own hitstun
 * test reads (and that test takes 0x1A as well), so the port keeps the
 * source's kind and the bomb's remade state here -- a 0x11A kind would index
 * the item tables past their end. Remix keeps the respawn flag in the bomb
 * GObj's word 0x80 (the animation callback) and the controller's state in
 * GObj words 0x30-0x38; the port keeps both in this file's slots. */
#include <nds/nds_p5.h>

#if NDS_P5
#include "nds_p5_native_stages.generated.h"

#include <PR/ultratypes.h>
#include <ft/fighter.h>
#include <gr/ground.h>
#include <it/item.h>
#include <reloc_data.h>
#include <sc/scene.h>
#include <sys/obj.h>
#include <sys/objman.h>
#include <sys/taskman.h>

GObj *itManagerMakeItemSetupCommon(GObj *parent_gobj, s32 kind, Vec3f *pos,
                                   Vec3f *vel, u32 flags);
extern void mpCollisionGetMapObjIDsKind(s32 mapobj_kind, s32 *mapobjs);
extern void mpCollisionGetMapObjPositionID(s32 mapobj_id, Vec3f *pos);
extern void *func_800269C0_275C0(u16 fgm_id);

/* The hazard files the stages load with their ground (stage_generate.py
 * NDS_P5_HAZARD_FILE_ROWS): the bombs' header, which the P5 article rows in
 * the entry-effect packet read the model through. */
void *gNdsP5HazardFileBowserSt;

#define NDS_P5_GKIND_BOWSERST 42

/* Hazards.asm. */
#define NDS_P5_BOMB_RESPAWN_TIME (0x3C * 5)        /* replenish_bowser_bombs_ */
#define NDS_P5_BOMB_FLASH_TIME 120                 /* BOWSER_BOMB_FLASH_TIME */
#define NDS_P5_BOMB_X 2990.0F                      /* 0x453AE000, times the stage's scale */
#define NDS_P5_BOMB_Y 512.0F                       /* 0x44000000 */
#define NDS_P5_BOMB_ECB_BOTTOM (-416.0F)           /* 0xC3D00000 */
#define NDS_P5_BOMB_MAPOBJ_KIND 4                  /* add_item_, a Dream Land stage */
#define NDS_P5_FGM_BOMB_EXPLODE 0x01u              /* bowser_bomb_explosion_ */
#define NDS_P5_FGM_BOMB_READY 0xC2u                /* flash end, air collision */

typedef struct NDSP5BowserBombSide
{
    GObj *controller;   /* the side's live controller (Remix: a new one per bomb) */
    GObj *bomb;         /* controller word 0x30 */
    s32 timer;          /* controller word 0x38 */
    u32 exploded;       /* the bomb GObj's word 0x80 */
    s32 flash;          /* the remade bomb's ITStruct word 0x2C0 */
    u32 remade;         /* Remix's 0x11A renumbering */
} NDSP5BowserBombSide;

static NDSP5BowserBombSide sNdsP5BowserBombs[2];

/* Hit by: Hazards.asm object_hit_by_table, per player port. */
static GObj *sNdsP5HitBy[GMCOMMON_PLAYERS_MAX];

static void ndsP5BowserBombReplenish(GObj *controller);
static sb32 ndsP5BowserBombFlashProcUpdate(GObj *item_gobj);

/* The battle's ground kind, or ~0 outside one (the renderer's P5 article
 * rows ask, nds_renderer_native_common.c). */
u32 ndsP5BattleGkind(void)
{
    return (gSCManagerBattleState != NULL) ?
        (u32)gSCManagerBattleState->gkind : 0xFFFFFFFFu;
}

/* An owner stage scale (P7 S0, stages.json owner_scale): what the stage's
 * code places across it moves out with it; heights stay (A10). */
static f32 ndsP5StageScale(s32 gkind)
{
#define NDS_P5_SCALE_ROW(g, s) if (gkind == (g)) { return (s); }
    NDS_P5_STAGE_SCALE_ROWS(NDS_P5_SCALE_ROW)
#undef NDS_P5_SCALE_ROW
    (void)gkind;
    return 1.0F;
}

static u32 ndsP5StageIs(s32 gkind)
{
    return ((gSCManagerBattleState != NULL) &&
            (gSCManagerBattleState->gkind == gkind)) ? TRUE : FALSE;
}

/* add_item_ for a Bowser bomb: the header file (loaded once a scene), a
 * controller object running replenish_bowser_bombs_, and the item at the
 * Dream Land item map object's place with x and y replaced, its bottom ECB
 * set for the hover. Returns the bomb or NULL. */
static GObj *ndsP5BowserBombAdd(u32 side)
{
    NDSP5BowserBombSide *s = &sNdsP5BowserBombs[side];
    s32 ids[64];
    Vec3f pos;
    Vec3f vel;
    GObj *bomb;

    if (gNdsP5HazardFileBowserSt == NULL)
    {
        return NULL;
    }
    gGRCommonStruct.bonus3.item_head = gNdsP5HazardFileBowserSt;

    /* 0x80009968: the object (id 0x3F2, link 1, priority 0x80000000) whose
     * per-frame routine is the replenish. */
    s->controller = gcMakeGObjSPAfter(0x3F2, ndsP5BowserBombReplenish, 1,
                                      0x80000000);
    s->bomb = NULL;
    s->exploded = FALSE;

    ids[0] = 0;
    mpCollisionGetMapObjIDsKind(NDS_P5_BOMB_MAPOBJ_KIND, ids);
    mpCollisionGetMapObjPositionID(ids[0], &pos);
    pos.x = ((side == 0u) ? NDS_P5_BOMB_X : -NDS_P5_BOMB_X) * ndsP5StageScale(NDS_P5_GKIND_BOWSERST);
    pos.y = NDS_P5_BOMB_Y;
    vel.x = vel.y = vel.z = 0.0F;

    bomb = itManagerMakeItemSetupCommon(NULL, nITKindTaruBomb, &pos, &vel, 1);
    if (bomb != NULL)
    {
        ITStruct *ip = itGetStruct(bomb);

        DObjGetStruct(bomb)->translate.vec.f.x = pos.x;
        DObjGetStruct(bomb)->translate.vec.f.y = pos.y;
        ip->coll_data.map_coll.bottom = NDS_P5_BOMB_ECB_BOTTOM;
    }
    s->bomb = bomb;
    return bomb;
}

/* replenish_bowser_bombs_, the controller's routine. */
static void ndsP5BowserBombReplenish(GObj *controller)
{
    u32 side;
    NDSP5BowserBombSide *s = NULL;
    GObj *bomb;

    for (side = 0u; side < 2u; side++)
    {
        if (sNdsP5BowserBombs[side].controller == controller)
        {
            s = &sNdsP5BowserBombs[side];
            break;
        }
    }
    if (s == NULL)
    {
        return;
    }
    if ((s->bomb != NULL) && (s->exploded == FALSE))
    {
        s->timer = NDS_P5_BOMB_RESPAWN_TIME;
        return;
    }
    s->bomb = NULL;
    s->timer--;
    if (s->timer != 0)
    {
        return;
    }
    /* Stop this controller (its routine cleared, the object off) and make
     * the side's next bomb with a new one. */
    controller->func_run = NULL;
    controller->flags = GOBJ_FLAG_HIDDEN;
    bomb = ndsP5BowserBombAdd(side);
    if (bomb != NULL)
    {
        ITStruct *ip = itGetStruct(bomb);

        ip->attack_coll.attack_state = nGMAttackStateOff;
        s->flash = NDS_P5_BOMB_FLASH_TIME;
        ip->proc_update = ndsP5BowserBombFlashProcUpdate;
        s->remade = TRUE;
    }
}

/* bowser_bomb_flash_main: drawn every other two frames, then solid. */
static sb32 ndsP5BowserBombFlashProcUpdate(GObj *item_gobj)
{
    ITStruct *ip = itGetStruct(item_gobj);
    u32 side;

    for (side = 0u; side < 2u; side++)
    {
        NDSP5BowserBombSide *s = &sNdsP5BowserBombs[side];

        if (s->bomb != item_gobj)
        {
            continue;
        }
        s->flash--;
        item_gobj->flags = ((u32)s->flash & 2u) >> 1;
        if (s->flash == 0)
        {
            item_gobj->flags = 0u;
            ip->proc_update = NULL;
            ip->attack_coll.attack_state = nGMAttackStateInterpolate;
            func_800269C0_275C0(NDS_P5_FGM_BOMB_READY);
        }
        break;
    }
    return FALSE;
}

/* grMainSetupMakeGround for the Remix kinds (battleship_grpupupu_ground.c):
 * TRUE when the kind is a Remix stage (made here, or nothing for a stage
 * whose ground routine is Remix's CLONE). */
sb32 ndsP5StageMakeGround(u32 gkind)
{
    u32 i;

    if (ndsP5StageIsRemix(gkind) == FALSE)
    {
        return FALSE;
    }
    for (i = 0u; i < 2u; i++)
    {
        sNdsP5BowserBombs[i].controller = NULL;
        sNdsP5BowserBombs[i].bomb = NULL;
        sNdsP5BowserBombs[i].timer = 0;
        sNdsP5BowserBombs[i].exploded = FALSE;
        sNdsP5BowserBombs[i].flash = 0;
        sNdsP5BowserBombs[i].remade = FALSE;
    }
    for (i = 0u; i < GMCOMMON_PLAYERS_MAX; i++)
    {
        sNdsP5HitBy[i] = NULL;
    }
    gNdsP5HazardFileBowserSt = NULL;
#define NDS_P5_HAZARD_LOAD(g, file) \
    if ((gkind == (u32)(g)) && ((g) == NDS_P5_GKIND_BOWSERST)) \
    { \
        const void *id = (const void *)(uintptr_t)(file); \
        size_t bytes = lbRelocGetFileSize(id); \
        void *heap = (bytes != 0u) ? syTaskmanMalloc(bytes, 0x10) : NULL; \
        gNdsP5HazardFileBowserSt = (heap != NULL) ? \
            lbRelocGetExternHeapFile(id, heap) : NULL; \
    }
    NDS_P5_HAZARD_FILE_ROWS(NDS_P5_HAZARD_LOAD)
#undef NDS_P5_HAZARD_LOAD

    if (gkind == NDS_P5_GKIND_BOWSERST)
    {
        /* peach_bumper_'s Bowser's Stadium arm: right bomb (id 0), then
         * left (id 1). */
        (void)ndsP5BowserBombAdd(0u);
        (void)ndsP5BowserBombAdd(1u);
    }
    return TRUE;
}

/* bowser_bomb_explosion_ (patch at 0x80184DD0 in itTaruBombCommonProcHit):
 * TRUE when the stage takes the explosion (FGM 0x01, respawn flag, no
 * shrapnel); FALSE runs the source's. */
sb32 ndsP5TaruBombExplode(GObj *item_gobj)
{
    u32 side;

    if (ndsP5StageIs(NDS_P5_GKIND_BOWSERST) == FALSE)
    {
        return FALSE;
    }
    for (side = 0u; side < 2u; side++)
    {
        if (sNdsP5BowserBombs[side].bomb == item_gobj)
        {
            sNdsP5BowserBombs[side].exploded = TRUE;
        }
    }
    func_800269C0_275C0(NDS_P5_FGM_BOMB_EXPLODE);
    return TRUE;
}

/* bowser_bomb_air_collision_ (patch at 0x80184F84): the landing sound. */
u16 ndsP5TaruBombMapFgm(u16 fgm_id)
{
    return (ndsP5StageIs(NDS_P5_GKIND_BOWSERST) != FALSE) ?
        (u16)NDS_P5_FGM_BOMB_READY : fgm_id;
}

/* itMainDestroyItem: a destroyed bomb (Remix reads its GObj's DObj word, 0
 * once ejected) leaves its side. */
void ndsP5ItemDestroyed(GObj *item_gobj)
{
    u32 side;

    for (side = 0u; side < 2u; side++)
    {
        if (sNdsP5BowserBombs[side].bomb == item_gobj)
        {
            sNdsP5BowserBombs[side].bomb = NULL;
        }
    }
}

/* track_hit_object_ (patch at 0x800E3F14, ftMainProcessHitCollisionStatsMain
 * after its prologue): the first hit log's attacker, per port. */
void ndsP5TrackHitObject(u32 player, GObj *attacker_gobj)
{
    if (player < GMCOMMON_PLAYERS_MAX)
    {
        sNdsP5HitBy[player] = attacker_gobj;
    }
}

/* bomb_hitstun_ (patch at 0x80140F44, ftCommonDamageInitDamageVars after
 * ftParamGetHitStun): on Bowser's Stadium, a fighter whose last hit came from
 * a bomb -- an item object (id 0x3F5) in the item list whose kind is the Race
 * bomb (or Remix's renumbered 0x11A) -- takes 9 / (gravity + 4) times the
 * hitstun. battleship_ftcommon_damage.c arms the one-shot with the victim when
 * that function reads its fighter, and its ftParamGetHitStun spends it. */
static FTStruct *sNdsP5HitStunVictim;

FTStruct *ndsP5HitStunArm(FTStruct *fp)
{
    sNdsP5HitStunVictim = fp;
    return fp;
}

f32 ndsP5HitStunTake(f32 hitstun_tics)
{
    FTStruct *fp = sNdsP5HitStunVictim;
    GObj *hit_by;
    GObj *gobj;

    sNdsP5HitStunVictim = NULL;
    if ((fp == NULL) || (ndsP5StageIs(NDS_P5_GKIND_BOWSERST) == FALSE) ||
        (fp->player >= GMCOMMON_PLAYERS_MAX))
    {
        return hitstun_tics;
    }
    hit_by = sNdsP5HitBy[fp->player];
    for (gobj = gGCCommonLinks[nGCCommonLinkIDItem]; gobj != NULL;
         gobj = gobj->link_next)
    {
        if (gobj != hit_by)
        {
            continue;
        }
        if ((gobj->id == nGCCommonKindItem) &&
            (itGetStruct(gobj)->kind == nITKindTaruBomb))
        {
            hitstun_tics *= 9.0F / (fp->attr->gravity + 4.0F);
        }
        break;
    }
    return hitstun_tics;
}

/* The P3 sync digest's stage arm (nds_net_session.c): each side's timer,
 * flags and flash. */
u32 ndsP5StageDigest(u32 hash, u32 (*mix)(u32, u32))
{
    u32 side;

    if (ndsP5StageIs(NDS_P5_GKIND_BOWSERST) == FALSE)
    {
        return hash;
    }
    for (side = 0u; side < 2u; side++)
    {
        const NDSP5BowserBombSide *s = &sNdsP5BowserBombs[side];

        hash = mix(hash, (u32)s->timer);
        hash = mix(hash, (u32)s->flash);
        hash = mix(hash, ((s->bomb != NULL) ? 1u : 0u) | (s->exploded << 1) |
                         (s->remade << 2));
    }
    return hash;
}
#endif
