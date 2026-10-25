
/* The Sector Z Arwing laser owner admits its object here and executes it in
 * the renderer translation unit, so its pinned constants live in a generated
 * header both can include -- the barrel-cannon actor's shape. */
#include <nds/generated/nds_native_sector_arwing_laser.generated.h>
#include <port/coroutine.h>
#include <nds/generated/nds_native_castle_bumper.generated.h>
#include <nds/generated/nds_native_samus_chargeshot.generated.h>
#include <nds/generated/nds_native_link_bomb.generated.h>
#include <nds/generated/nds_native_yamabuki_marumine.generated.h>
#include <nds/generated/nds_native_item_glucky.generated.h>
#include <nds/generated/nds_native_item_porygon.generated.h>
#include <nds/generated/nds_native_item_hitokage.generated.h>
#include <nds/generated/nds_native_item_fushigibana.generated.h>
#include <nds/generated/nds_native_item_tomato.generated.h>
#include <nds/generated/nds_native_item_star.generated.h>
#include <nds/generated/nds_native_item_sword.generated.h>
#include <nds/generated/nds_native_item_hammer.generated.h>
#include <nds/generated/nds_native_item_mball.generated.h>
#include <nds/generated/nds_native_item_kirbystar.generated.h>
#include <nds/generated/nds_native_item_gshell.generated.h>
#include <nds/generated/nds_native_item_rshell.generated.h>
#include <nds/generated/nds_native_item_bat.generated.h>
#include <nds/generated/nds_native_item_bombhei.generated.h>
#include <nds/generated/nds_native_item_lgun.generated.h>
#include <nds/generated/nds_native_item_harisen.generated.h>
#include <nds/generated/nds_native_item_heart.generated.h>
#include <nds/generated/nds_native_item_starrod.generated.h>
#include <nds/generated/nds_native_item_fflower.generated.h>
#include <nds/generated/nds_native_item_msbomb.generated.h>
#include <nds/generated/nds_native_item_baked.generated.h>
#include <nds/generated/nds_native_item_nbumper.generated.h>
#include <nds/generated/nds_native_item_box.generated.h>
#include <nds/generated/nds_native_item_taru.generated.h>
#include <nds/generated/nds_native_item_egg.generated.h>
#include <nds/generated/nds_native_item_iwark.generated.h>
#include <nds/generated/nds_native_item_capsule.generated.h>
#include <nds/generated/nds_native_inishie_powblock.generated.h>
#include <nds/generated/nds_native_pikachu_thunderjolt.generated.h>
#include <nds/generated/nds_native_pikachu_thunderground.generated.h>
#include <nds/generated/nds_native_pikachu_thunderjolt_effect.generated.h>
#include <nds/generated/nds_native_ness_pkfire.generated.h>
#include <nds/generated/nds_native_ness_pkthunder.generated.h>
#include <nds/generated/nds_native_yoshi_egg.generated.h>
#include <nds/generated/nds_native_yoshi_egglay.generated.h>
#include <nds/generated/nds_native_purin_sing.generated.h>
#include <nds/generated/nds_native_kirby_vulcan.generated.h>
#include <nds/generated/nds_native_pikachu_thunder.generated.h>
#include <nds/generated/nds_native_samus_bomb.generated.h>
#include <nds/generated/nds_native_ness_pktail.generated.h>
#include <nds/generated/nds_native_yoshi_entryegg.generated.h>
#include <nds/generated/nds_native_damage_slash.generated.h>
#include <nds/generated/nds_native_damage_fly_mdust.generated.h>
#include <nds/nds_reloc_assets.h>
#include <nds/nds_preview_pack.h>
#include <nds/nds_native_wallpaper.h>
#include <sys/objman.h>
#include <nds/nds_p5.h>

extern void gcDrawDObjTreeDLLinksForGObj(GObj *gobj);
extern volatile u32 gNdsDamageSlashRootMask;
extern volatile u32 gNdsDamageSlashEffectsSeen;
extern volatile u32 gNdsDamageSlashEffectsRejected;
extern volatile u32 gNdsDamageSlashCandidateStep;
extern volatile u32 gNdsDamageSlashSnapshotFailCount;
extern volatile u32 gNdsDamageSlashDrawCount;
extern volatile u32 gNdsDamageSlashSubmitFailCount;
extern volatile u32 gNdsDamageSlashSubmitStep;
extern volatile u32 gNdsDamageSlashAlphaZeroCount;
extern volatile u32 gNdsDamageSlashTriangleDrawCount;
sb32 ndsRendererSubmitNativeDamageSlash(
    const void *asset_base, u32 asset_bytes, u32 root_offset,
    const NDSRendererNativeMaterial *material,
    const NDSRendererConfig *config, NDSRendererStats *stats);
sb32 ndsRendererPreflightNativeDamageSlash(
    const void *asset_base, u32 asset_bytes, u32 root_offset,
    const NDSRendererNativeMaterial *material,
    const NDSRendererConfig *config);

#if NDS_P2_STAGE_YAMABUKI
extern volatile u32 gNdsYamabukiGluckyCandidateStep;
extern volatile u32 gNdsYamabukiGluckyItemKind;
extern volatile u32 gNdsYamabukiGluckyForeignKindCount;
extern volatile u32 gNdsYamabukiGluckyDrawCount;
extern volatile u32 gNdsYamabukiGluckySubmitFailCount;
extern volatile u32 gNdsYamabukiPorygonCandidateStep;
extern volatile u32 gNdsYamabukiPorygonItemKind;
extern volatile u32 gNdsYamabukiPorygonForeignKindCount;
extern volatile u32 gNdsYamabukiPorygonDrawCount;
extern volatile u32 gNdsYamabukiPorygonSubmitFailCount;
extern volatile u32 gNdsYamabukiHitokageCandidateStep;
extern volatile u32 gNdsYamabukiHitokageItemKind;
extern volatile u32 gNdsYamabukiHitokageForeignKindCount;
extern volatile u32 gNdsYamabukiHitokageDrawCount;
extern volatile u32 gNdsYamabukiHitokageSubmitFailCount;
extern volatile u32 gNdsYamabukiHitokageEffectsSeen;
extern volatile u32 gNdsYamabukiHitokageEffectsRejected;
extern volatile u32 gNdsYamabukiHitokageSnapshotFailCount;
extern volatile u32 gNdsYamabukiHitokageImage;
extern volatile u32 gNdsYamabukiFushigibanaCandidateStep;
extern volatile u32 gNdsYamabukiFushigibanaItemKind;
extern volatile u32 gNdsYamabukiFushigibanaForeignKindCount;
extern volatile u32 gNdsYamabukiFushigibanaDrawCount;
extern volatile u32 gNdsYamabukiFushigibanaSubmitFailCount;
extern volatile u32 gNdsYamabukiFushigibanaEffectsSeen;
extern volatile u32 gNdsYamabukiFushigibanaEffectsRejected;
extern volatile u32 gNdsYamabukiFushigibanaSnapshotFailCount;
extern volatile u32 gNdsYamabukiFushigibanaImage;

sb32 ndsRendererSubmitNativeItemGLucky(
    const void *actor_base_ptr, u32 actor_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats);
sb32 ndsRendererSubmitNativeItemPorygon(
    const void *actor_base_ptr, u32 actor_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats);
sb32 ndsRendererSubmitNativeItemHitokage(
    const void *actor_base_ptr, u32 actor_bytes,
    const NDSRendererNativeMaterial *material,
    const NDSRendererConfig *config, NDSRendererStats *stats);
sb32 ndsRendererSubmitNativeItemFushigibana(
    const void *actor_base_ptr, u32 actor_bytes,
    const NDSRendererNativeMaterial *material,
    const NDSRendererConfig *config, NDSRendererStats *stats);
#endif

extern volatile u32 gNdsItemStarKind;
extern volatile u32 gNdsItemStarForeignKindCount;
extern volatile u32 gNdsItemStarCandidateStep;
extern volatile u32 gNdsItemStarDrawCount;
extern volatile u32 gNdsItemStarSubmitFailCount;
extern volatile u32 gNdsItemStarEffectsSeen;
extern volatile u32 gNdsItemStarEffectsRejected;
extern volatile u32 gNdsItemStarSnapshotFailCount;
extern volatile u32 gNdsItemSwordKind;
extern volatile u32 gNdsItemSwordForeignKindCount;
extern volatile u32 gNdsItemSwordCandidateStep;
extern volatile u32 gNdsItemSwordDrawCount;
extern volatile u32 gNdsItemSwordSubmitFailCount;
extern volatile u32 gNdsItemSwordRoot;
extern volatile u32 gNdsItemHammerKind;
extern volatile u32 gNdsItemHammerForeignKindCount;
extern volatile u32 gNdsItemHammerCandidateStep;
extern volatile u32 gNdsItemHammerDrawCount;
extern volatile u32 gNdsItemHammerSubmitFailCount;
extern volatile u32 gNdsItemMBallKind;
extern volatile u32 gNdsItemMBallForeignKindCount;
extern volatile u32 gNdsItemMBallCandidateStep;
extern volatile u32 gNdsItemMBallDrawCount;
extern volatile u32 gNdsItemMBallSubmitFailCount;
extern volatile u32 gNdsItemMBallRoot;
extern volatile u32 gNdsItemMBallEffectsSeen;
extern volatile u32 gNdsItemMBallEffectsRejected;
extern volatile u32 gNdsItemMBallSnapshotFailCount;
/* The thrown entry Poke Ball shares the item's native owner; these say so
 * out loud. Defined beside the maker in src/import/battleship_efmanager.c,
 * because the whole thrown closure is that file's. */
extern volatile u32 gNdsEntryMBallThrownRootMask;
extern volatile u32 gNdsEntryMBallThrownDrawCount;
extern volatile u32 gNdsEntryMBallThrownSubmitFailCount;
/* K04. ITCommonObject+0x5458, shared by the Star Rod's two weapon swings and
 * both of Kirby's stars; defined beside that closure in
 * src/import/battleship_efmanager.c. */
extern volatile u32 gNdsItemKirbyStarCandidateStep;
extern volatile u32 gNdsItemKirbyStarDrawCount;
extern volatile u32 gNdsItemKirbyStarSubmitFailCount;
extern volatile u32 gNdsItemKirbyStarFromEffectCount;
extern volatile u32 gNdsItemGShellKind;
extern volatile u32 gNdsItemGShellForeignKindCount;
extern volatile u32 gNdsItemGShellCandidateStep;
extern volatile u32 gNdsItemGShellDrawCount;
extern volatile u32 gNdsItemGShellSubmitFailCount;
extern volatile u32 gNdsItemGShellEffectsSeen;
extern volatile u32 gNdsItemGShellEffectsRejected;
extern volatile u32 gNdsItemGShellSnapshotFailCount;
extern volatile u32 gNdsItemBatKind;
extern volatile u32 gNdsItemBatForeignKindCount;
extern volatile u32 gNdsItemBatCandidateStep;
extern volatile u32 gNdsItemBatDrawCount;
extern volatile u32 gNdsItemBatSubmitFailCount;
extern volatile u32 gNdsItemBatRoot;
extern volatile u32 gNdsItemCapsuleKind;
extern volatile u32 gNdsItemCapsuleForeignKindCount;
extern volatile u32 gNdsItemCapsuleCandidateStep;
extern volatile u32 gNdsItemCapsuleDrawCount;
extern volatile u32 gNdsItemCapsuleSubmitFailCount;
extern volatile u32 gNdsItemCapsuleRoot;
extern volatile u32 gNdsItemBombHeiKind;
extern volatile u32 gNdsItemBombHeiForeignKindCount;
extern volatile u32 gNdsItemBombHeiCandidateStep;
extern volatile u32 gNdsItemBombHeiDrawCount;
extern volatile u32 gNdsItemBombHeiSubmitFailCount;
extern volatile u32 gNdsItemBombHeiEffectsSeen;
extern volatile u32 gNdsItemBombHeiEffectsRejected;
extern volatile u32 gNdsItemBombHeiSnapshotFailCount;
extern volatile u32 gNdsItemRShellKind;
extern volatile u32 gNdsItemRShellForeignKindCount;
extern volatile u32 gNdsItemRShellCandidateStep;
extern volatile u32 gNdsItemRShellDrawCount;
extern volatile u32 gNdsItemRShellSubmitFailCount;
extern volatile u32 gNdsItemRShellEffectsSeen;
extern volatile u32 gNdsItemRShellEffectsRejected;
extern volatile u32 gNdsItemRShellSnapshotFailCount;
extern volatile u32 gNdsItemLGunKind;
extern volatile u32 gNdsItemLGunForeignKindCount;
extern volatile u32 gNdsItemLGunCandidateStep;
extern volatile u32 gNdsItemLGunDrawCount;
extern volatile u32 gNdsItemLGunSubmitFailCount;
extern volatile u32 gNdsItemHarisenKind;
extern volatile u32 gNdsItemHarisenForeignKindCount;
extern volatile u32 gNdsItemHarisenCandidateStep;
extern volatile u32 gNdsItemHarisenDrawCount;
extern volatile u32 gNdsItemHarisenSubmitFailCount;
extern volatile u32 gNdsItemHeartKind;
extern volatile u32 gNdsItemHeartForeignKindCount;
extern volatile u32 gNdsItemHeartCandidateStep;
extern volatile u32 gNdsItemHeartDrawCount;
extern volatile u32 gNdsItemHeartSubmitFailCount;
extern volatile u32 gNdsItemStarRodKind;
extern volatile u32 gNdsItemStarRodForeignKindCount;
extern volatile u32 gNdsItemStarRodCandidateStep;
extern volatile u32 gNdsItemStarRodDrawCount;
extern volatile u32 gNdsItemStarRodSubmitFailCount;
extern volatile u32 gNdsItemStarRodRoot;
extern volatile u32 gNdsItemFFlowerKind;
extern volatile u32 gNdsItemFFlowerForeignKindCount;
extern volatile u32 gNdsItemFFlowerCandidateStep;
extern volatile u32 gNdsItemFFlowerDrawCount;
extern volatile u32 gNdsItemFFlowerSubmitFailCount;
extern volatile u32 gNdsItemFFlowerRoot;
extern volatile u32 gNdsItemFFlowerEffectsSeen;
extern volatile u32 gNdsItemFFlowerEffectsRejected;
extern volatile u32 gNdsItemFFlowerSnapshotFailCount;
extern volatile u32 gNdsItemMSBombKind;
extern volatile u32 gNdsItemMSBombForeignKindCount;
extern volatile u32 gNdsItemMSBombCandidateStep;
extern volatile u32 gNdsItemMSBombDrawCount;
extern volatile u32 gNdsItemMSBombSubmitFailCount;
extern volatile u32 gNdsItemMSBombRoot;
extern volatile u32 gNdsItemNBumperKind;
extern volatile u32 gNdsItemNBumperForeignKindCount;
extern volatile u32 gNdsItemNBumperCandidateStep;
extern volatile u32 gNdsItemNBumperDrawCount;
extern volatile u32 gNdsItemNBumperSubmitFailCount;
extern volatile u32 gNdsItemNBumperEffectsSeen;
extern volatile u32 gNdsItemNBumperEffectsRejected;
extern volatile u32 gNdsItemNBumperSnapshotFailCount;
extern volatile u32 gNdsItemBoxKind;
extern volatile u32 gNdsItemBoxForeignKindCount;
extern volatile u32 gNdsItemBoxCandidateStep;
extern volatile u32 gNdsItemBoxDrawCount;
extern volatile u32 gNdsItemBoxSubmitFailCount;
extern volatile u32 gNdsItemTaruKind;
extern volatile u32 gNdsItemTaruForeignKindCount;
extern volatile u32 gNdsItemTaruCandidateStep;
extern volatile u32 gNdsItemTaruDrawCount;
extern volatile u32 gNdsItemTaruSubmitFailCount;
extern volatile u32 gNdsItemEggKind;
extern volatile u32 gNdsItemEggForeignKindCount;
extern volatile u32 gNdsItemEggCandidateStep;
extern volatile u32 gNdsItemEggDrawCount;
extern volatile u32 gNdsItemEggSubmitFailCount;
extern volatile u32 gNdsItemIwarkKind;
extern volatile u32 gNdsItemIwarkForeignKindCount;
extern volatile u32 gNdsItemIwarkCandidateStep;
extern volatile u32 gNdsItemIwarkDrawCount;
extern volatile u32 gNdsItemIwarkSubmitFailCount;

sb32 ndsRendererSubmitNativeItemStar(
    const void *file_base_ptr, u32 file_bytes,
    const NDSRendererNativeMaterial *material0,
    const NDSRendererNativeMaterial *material1,
    const NDSRendererConfig *config, NDSRendererStats *stats);
sb32 ndsRendererSubmitNativeItemSword(
    u32 root_offset, const void *file_base_ptr, u32 file_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats);
sb32 ndsRendererSubmitNativeItemHammer(
    const void *file_base_ptr, u32 file_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats);
sb32 ndsRendererSubmitNativeItemMBall(
    u32 root_offset, const void *file_base_ptr, u32 file_bytes,
    const NDSRendererNativeMaterial *material,
    const NDSRendererConfig *config, NDSRendererStats *stats);
sb32 ndsRendererSubmitNativeItemKirbyStar(
    const void *file_base_ptr, u32 file_bytes,
    const NDSRendererNativeMaterial *material,
    const NDSRendererConfig *config, NDSRendererStats *stats);
sb32 ndsRendererSubmitNativeItemGShell(
    const void *file_base_ptr, u32 file_bytes,
    const NDSRendererNativeMaterial *material,
    const NDSRendererConfig *config, NDSRendererStats *stats);
sb32 ndsRendererSubmitNativeItemRShell(
    const void *file_base_ptr, u32 file_bytes,
    const NDSRendererNativeMaterial *material,
    const NDSRendererConfig *config, NDSRendererStats *stats);
sb32 ndsRendererSubmitNativeItemBat(
    u32 root_offset, const void *file_base_ptr, u32 file_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats);
sb32 ndsRendererSubmitNativeItemCapsule(
    u32 root_offset, const void *file_base_ptr, u32 file_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats);
sb32 ndsRendererSubmitNativeItemBombHei(
    const void *file_base_ptr, u32 file_bytes,
    const NDSRendererNativeMaterial *material,
    const NDSRendererConfig *config, NDSRendererStats *stats);
/* The baked roots (nds_native_item_baked.exec.inc). */
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
extern volatile u32 gNdsLabBakedAcc[16];
#endif
const void *ndsNativeBakedItemFind(u32 asset_id, u32 root, u32 gobj_kind,
                                   const Gfx *dl, u32 *material_slots);
sb32 ndsRendererSubmitNativeBaked(
    const void *handle, const void *file_base_ptr, u32 file_bytes,
    const NDSRendererNativeMaterial *materials, u32 material_count,
    const NDSRendererConfig *config, NDSRendererStats *stats);
sb32 ndsNativeBakedRootOffscreen(const void *handle,
                                 const NDSRendererConfig *config);
/* The drawn DObj's translation, around each baked submit, for a group's
 * cross-DObj seeds (Blastoise's Hydro Pump beam); NULL after. */
void ndsNativeBakedSetSeedTranslate(const Vec3f *translate);
sb32 ndsNativeBakedRootIsRoom(const void *handle);
/* The bumper quad's (nds_native_castle_bumper.exec.inc). */
sb32 ndsNativeCastleBumperOffscreen(const NDSRendererConfig *config);
extern volatile u32 gNdsItemBakedDrawCount;
extern volatile u32 gNdsItemBakedSubmitFailCount;
sb32 ndsRendererSubmitNativeItemLGun(
    const void *file_base_ptr, u32 file_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats);
sb32 ndsRendererSubmitNativeItemHarisen(
    const void *file_base_ptr, u32 file_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats);
sb32 ndsRendererSubmitNativeItemHeart(
    const void *file_base_ptr, u32 file_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats);
sb32 ndsRendererSubmitNativeItemStarRod(
    u32 root_offset, const void *file_base_ptr, u32 file_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats);
sb32 ndsRendererSubmitNativeItemFFlower(
    u32 root_offset, const void *file_base_ptr, u32 file_bytes,
    const NDSRendererNativeMaterial *material,
    const NDSRendererConfig *config, NDSRendererStats *stats);
sb32 ndsRendererSubmitNativeItemMSBomb(
    u32 root_offset, const void *file_base_ptr, u32 file_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats);
sb32 ndsRendererSubmitNativeItemNBumper(
    const void *file_base_ptr, u32 file_bytes,
    const NDSRendererNativeMaterial *material,
    const NDSRendererConfig *config, NDSRendererStats *stats);
sb32 ndsRendererSubmitNativeItemBox(
    const void *file_base_ptr, u32 file_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats);
sb32 ndsRendererSubmitNativeItemTaru(
    const void *file_base_ptr, u32 file_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats);
sb32 ndsRendererSubmitNativeItemEgg(
    const void *file_base_ptr, u32 file_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats);
sb32 ndsRendererSubmitNativeItemIwark(
    const void *file_base_ptr, u32 file_bytes,
    const NDSRendererConfig *config, NDSRendererStats *stats);

#define NDS_RENDERER_STAGE_DL_HEADS 4u

static NDSFighterDLDrawState sNdsRendererAdapterStagePersistentState;
static NDSRendererStats sNdsRendererAdapterStagePersistentStats;
static NDSRendererVertexCache sNdsRendererAdapterStageVertexCache;
static sb32 sNdsRendererAdapterStagePersistentActive;
/* Set only while an EFFECT tree submit is on the stack. The stage, the weapons
 * and the effects all reach the hardware through the same SubmitStageDL, and
 * the stage submits hundreds of lists per frame, so publishing the executor's
 * verdict unconditionally would report the last stage list rather than the
 * effect that is being investigated. */
static sb32 sNdsRendererAdapterEffectSubmitActive;
/* Items share the source DObj interpreter with stage/effects, but they are a
 * separate display layer with their own pre-model RDP state.  LinkBomb is the
 * first live client; keeping a distinct flag prevents item ColAnim state from
 * contaminating the effect layer's sticky blend state/diagnostics. */
static sb32 sNdsRendererAdapterItemSubmitActive;
static u32 sNdsRendererAdapterItemSubmitHead;



void ndsRendererAdapterResetDepthDiagnostics(void)
{
    gNdsRendererDepthStageSamples = 0u;
    gNdsRendererDepthFighterP0Samples = 0u;
    gNdsRendererDepthFighterP1Samples = 0u;
}

static sb32 ndsRendererAdapterStatsHasArmedTexture(
    const NDSRendererStats *stats)
{
    return ((stats != NULL) &&
            ((stats->texture_image != 0u) ||
             (stats->texture_tlut_image != 0u) ||
             (stats->texture_on != 0u))) ? TRUE : FALSE;
}

static sb32 ndsRendererAdapterStatsHasArmedTile(
    const NDSRendererStats *stats)
{
    u32 i;

    if (stats == NULL)
    {
        return FALSE;
    }
    for (i = 0u; i < NDS_RENDERER_TILE_COUNT; i++)
    {
        const NDSRendererTileState *tile = &stats->texture_tiles[i];

        if ((tile->set_seen != 0u) || (tile->size_seen != 0u) ||
            (tile->line != 0u) || (tile->width != 0u) ||
            (tile->height != 0u))
        {
            return TRUE;
        }
    }
    return FALSE;
}

/* THE SOURCE PROC'S OWN prim/env, read back out of the DL head span it wrote.
 *
 * gDPSetPrimColor/gDPSetEnvColor now carry their words (include/PR/gbi.h), and
 * a source effect's proc_display emits exactly those two immediately before it
 * draws its model. Nothing executes the head streams on the DS, so this scans
 * the span the proc appended and hands the values to the effect submit, which
 * seeds them into the renderer's RDP state before the model list runs. Without
 * it every source effect drew in whatever prim/env the previous list left
 * behind -- the stage's, which is what made the shield bubble dark.
 *
 * Bounded and cheap: the shield's span is three commands, and a head whose
 * pointer did not move is skipped outright. */
#define NDS_RENDERER_ADAPTER_DISPLAY_PROC_SCAN_MAX 32u

static const Gfx *sNdsRendererAdapterDisplayProcHeadMark[
    NDS_RENDERER_STAGE_DL_HEADS];
static u32 sNdsRendererAdapterEffectColorMask;
static u32 sNdsRendererAdapterEffectPrimColor;
static u32 sNdsRendererAdapterEffectEnvColor;
/* Colour is per-effect; BLEND STATE IS PER-LAYER AND STICKY. The XLU bracket
 * GObj emits G_RM_AA_ZB_XLU_SURF at display order 0 and the CLD bracket
 * switches to G_RM_CLD_SURF at order 3 (efdisplay.c:15, :5), and every effect
 * drawn in between inherits it -- efManagerShieldProcDisplay emits no render
 * mode of its own at all. So this accumulates across display procs instead of
 * being rebuilt per span, which is what the RDP does. */
static u32 sNdsRendererAdapterEffectOtherModeL;
static u32 sNdsRendererAdapterEffectOtherModeValid;
/* Whether THIS display proc wrote a render mode, and which DL head the list
 * being submitted belongs to. The folded value above is one number for every
 * head and outlives the proc that wrote it, which is right for an effect that
 * sets its own mode and wrong for one that sets none: Link's entry beam sits in
 * DObjDLLink list 1, its generic gcDrawDObjTreeDLLinksForGObj proc emits no
 * mode at all, and it inherited whatever opaque mode the previous effect left
 * -- a 36%-alpha column drew solid. */
static u32 sNdsRendererAdapterEffectOtherModeThisProc;
static u32 sNdsRendererAdapterEffectSubmitHead;
static u32 sNdsRendererAdapterItemColorMask[NDS_RENDERER_STAGE_DL_HEADS];
static u32 sNdsRendererAdapterItemPrimColor[NDS_RENDERER_STAGE_DL_HEADS];
static u32 sNdsRendererAdapterItemEnvColor[NDS_RENDERER_STAGE_DL_HEADS];
static u32 sNdsRendererAdapterItemOtherModeL[NDS_RENDERER_STAGE_DL_HEADS];
static u32 sNdsRendererAdapterItemOtherModeLValid[NDS_RENDERER_STAGE_DL_HEADS];
static u32 sNdsRendererAdapterItemOtherModeH[NDS_RENDERER_STAGE_DL_HEADS];
static u32 sNdsRendererAdapterItemOtherModeHValid[NDS_RENDERER_STAGE_DL_HEADS];

/* One G_SETOTHERMODE_L packet folded into the running value, using the same
 * shift/length decode as ndsRendererRecordOtherMode (nds_renderer.c:5869) so
 * the two cannot disagree about what a word means. */
static void ndsRendererAdapterFoldDisplayProcOtherModeL(u32 w0, u32 w1)
{
    u32 bits = (w0 & 0xffu) + 1u;
    u32 pos = (w0 >> 8) & 0xffu;
    u32 shift;
    u32 mask;

    if ((bits > 32u) || (pos >= 32u) || ((bits + pos) > 32u))
    {
        return;
    }
    shift = 32u - pos - bits;
    mask = (bits >= 32u) ? 0xffffffffu : (((1u << bits) - 1u) << shift);
    sNdsRendererAdapterEffectOtherModeL =
        (sNdsRendererAdapterEffectOtherModeL & ~mask) | (w1 & mask);
    sNdsRendererAdapterEffectOtherModeValid = 1u;
    sNdsRendererAdapterEffectOtherModeThisProc = 1u;
}

static void ndsRendererAdapterFoldItemOtherMode(u32 *value, u32 *valid,
                                                u32 w0, u32 w1)
{
    u32 bits = (w0 & 0xffu) + 1u;
    u32 pos = (w0 >> 8) & 0xffu;
    u32 shift;
    u32 mask;

    if ((value == NULL) || (valid == NULL) || (bits > 32u) ||
        (pos >= 32u) || ((bits + pos) > 32u))
    {
        return;
    }
    shift = 32u - pos - bits;
    mask = (bits >= 32u) ? 0xffffffffu : (((1u << bits) - 1u) << shift);
    *value = (*value & ~mask) | (w1 & mask);
    *valid = 1u;
}

/* Both ends of a display-proc span must be real main-RAM DL pointers before
 * anything walks between them. On the bounded fast target nothing presents,
 * so `gSYTaskmanDLHeads[]` never receives live DL cursors and holds small
 * non-NULL residue (the P2-3r3 abort read cursor=0x230 end=0x240: the NULL
 * guard passed, `ldr [0x230]` took the MPU data abort, and the nested abort
 * was every "pc=0xfffffffc" corpse the proof harness autopsied). Realtime
 * builds always carry >= 0x02000000 pointers here, so this is behavior-free
 * for every shipping configuration. */
static inline sb32 ndsRendererAdapterDisplayProcSpanValid(const Gfx *cursor,
                                                          const Gfx *end)
{
    return (((uintptr_t)cursor >= 0x02000000u) &&
            ((uintptr_t)end >= 0x02000000u) &&
            (cursor < end)) ? TRUE : FALSE;
}

/* The span one display proc emitted. Folding is idempotent (last writer wins
 * per field), so scanning a span twice -- which the effect path does, once at
 * its own draw and once when the next proc re-marks -- costs nothing. */
static void ndsRendererAdapterScanDisplayProcOtherMode(void)
{
    u32 head;

    for (head = 0u; head < NDS_RENDERER_STAGE_DL_HEADS; head++)
    {
        const Gfx *cursor = sNdsRendererAdapterDisplayProcHeadMark[head];
        const Gfx *end = gSYTaskmanDLHeads[head];
        u32 scanned = 0u;

        if (ndsRendererAdapterDisplayProcSpanValid(cursor, end) == FALSE)
        {
            continue;
        }
        while ((cursor < end) &&
               (scanned < NDS_RENDERER_ADAPTER_DISPLAY_PROC_SCAN_MAX))
        {
            if ((cursor->words.w0 >> 24) ==
                NDS_FIGHTER_DL_OP_SETOTHERMODE_L)
            {
                ndsRendererAdapterFoldDisplayProcOtherModeL(
                    cursor->words.w0, cursor->words.w1);
            }
            cursor++;
            scanned++;
        }
    }
}

void __attribute__((section(".itcm")))
ndsRendererAdapterMarkDisplayProcHeads(void)
{
    u32 i;

    /* Close the previous proc's span before opening this one. The XLU and CLD
     * brackets draw NOTHING, so they never reach the effect submit path and
     * their span would otherwise be overwritten unread -- which is precisely
     * the state the shield depends on. */
    ndsRendererAdapterScanDisplayProcOtherMode();
    for (i = 0u; i < NDS_RENDERER_STAGE_DL_HEADS; i++)
    {
        sNdsRendererAdapterDisplayProcHeadMark[i] = gSYTaskmanDLHeads[i];
    }
}

void ndsRendererAdapterCaptureDisplayProcColors(void)
{
    u32 head;

    sNdsRendererAdapterEffectColorMask = 0u;
    sNdsRendererAdapterEffectOtherModeThisProc = 0u;
    for (head = 0u; head < NDS_RENDERER_STAGE_DL_HEADS; head++)
    {
        const Gfx *cursor = sNdsRendererAdapterDisplayProcHeadMark[head];
        const Gfx *end = gSYTaskmanDLHeads[head];
        u32 scanned = 0u;

        if (ndsRendererAdapterDisplayProcSpanValid(cursor, end) == FALSE)
        {
            continue;
        }
        while ((cursor < end) &&
               (scanned < NDS_RENDERER_ADAPTER_DISPLAY_PROC_SCAN_MAX))
        {
            u32 op = cursor->words.w0 >> 24;

            if (op == NDS_FIGHTER_DL_OP_SETPRIMCOLOR)
            {
                sNdsRendererAdapterEffectPrimColor = cursor->words.w1;
                sNdsRendererAdapterEffectColorMask |= 1u;
            }
            else if (op == NDS_FIGHTER_DL_OP_SETENVCOLOR)
            {
                sNdsRendererAdapterEffectEnvColor = cursor->words.w1;
                sNdsRendererAdapterEffectColorMask |= 2u;
            }
            else if (op == NDS_FIGHTER_DL_OP_SETOTHERMODE_L)
            {
                /* The impact wave sets its own mode at the top of its own
                 * proc (efmanager.c:3286), so the current span matters too. */
                ndsRendererAdapterFoldDisplayProcOtherModeL(
                    cursor->words.w0, cursor->words.w1);
            }
            cursor++;
            scanned++;
        }
    }
    gNdsEffectDLColorMask = sNdsRendererAdapterEffectColorMask;
    gNdsEffectDLPrimColor = sNdsRendererAdapterEffectPrimColor;
    gNdsEffectDLEnvColor = sNdsRendererAdapterEffectEnvColor;
    gNdsEffectDLOtherModeL = sNdsRendererAdapterEffectOtherModeL;
    gNdsEffectDLOtherModeValid = sNdsRendererAdapterEffectOtherModeValid;
}

/* Snapshot only the commands emitted by the current ITEM proc before its DObj
 * draw. BattleShip itDisplayColAnimOPA/XLU writes cycle/render mode + EnvColor
 * immediately before gcDrawDObjTree*, and the DObj hook reaches this function
 * before the proc writes its post-draw restore commands.  The state is kept per
 * DL head because XLU items seed OPA head 0 and XLU head 1 differently. */
void ndsRendererAdapterCaptureItemDisplayProcState(void)
{
    u32 head;

    for (head = 0u; head < NDS_RENDERER_STAGE_DL_HEADS; head++)
    {
        const Gfx *cursor = sNdsRendererAdapterDisplayProcHeadMark[head];
        const Gfx *end = gSYTaskmanDLHeads[head];
        u32 scanned = 0u;

        /* P2-6 (2026-10-02): the head's slots cleared here rather than by
         * seven bzero calls over four-entry arrays (44 captures a frame in
         * the Race: 9K cycles of memset). */
        sNdsRendererAdapterItemColorMask[head] = 0u;
        sNdsRendererAdapterItemPrimColor[head] = 0u;
        sNdsRendererAdapterItemEnvColor[head] = 0u;
        sNdsRendererAdapterItemOtherModeL[head] = 0u;
        sNdsRendererAdapterItemOtherModeLValid[head] = 0u;
        sNdsRendererAdapterItemOtherModeH[head] = 0u;
        sNdsRendererAdapterItemOtherModeHValid[head] = 0u;
        if (ndsRendererAdapterDisplayProcSpanValid(cursor, end) == FALSE)
        {
            continue;
        }
        while ((cursor < end) &&
               (scanned < NDS_RENDERER_ADAPTER_DISPLAY_PROC_SCAN_MAX))
        {
            u32 op = cursor->words.w0 >> 24;

            if (op == NDS_FIGHTER_DL_OP_SETPRIMCOLOR)
            {
                sNdsRendererAdapterItemPrimColor[head] = cursor->words.w1;
                sNdsRendererAdapterItemColorMask[head] |= 1u;
            }
            else if (op == NDS_FIGHTER_DL_OP_SETENVCOLOR)
            {
                sNdsRendererAdapterItemEnvColor[head] = cursor->words.w1;
                sNdsRendererAdapterItemColorMask[head] |= 2u;
            }
            else if (op == NDS_FIGHTER_DL_OP_SETOTHERMODE_L)
            {
                ndsRendererAdapterFoldItemOtherMode(
                    &sNdsRendererAdapterItemOtherModeL[head],
                    &sNdsRendererAdapterItemOtherModeLValid[head],
                    cursor->words.w0, cursor->words.w1);
            }
            else if (op == 0xe3u) /* F3DEX2 G_SETOTHERMODE_H */
            {
                ndsRendererAdapterFoldItemOtherMode(
                    &sNdsRendererAdapterItemOtherModeH[head],
                    &sNdsRendererAdapterItemOtherModeHValid[head],
                    cursor->words.w0, cursor->words.w1);
            }
            cursor++;
            scanned++;
        }
    }
}

void ndsRendererAdapterBeginStageTraversal(void)
{
    bzero(&sNdsRendererAdapterStagePersistentState,
          sizeof(sNdsRendererAdapterStagePersistentState));
    ndsRendererInitStats(&sNdsRendererAdapterStagePersistentStats);
    if ((sNdsFighterDisplayCurrentLightValid != FALSE) &&
        (sNdsFighterDisplayCurrentLightCount != 0u))
    {
        sNdsRendererAdapterStagePersistentStats.light_dir_x =
            sNdsFighterDisplayCurrentLight.l.dir[0];
        sNdsRendererAdapterStagePersistentStats.light_dir_y =
            sNdsFighterDisplayCurrentLight.l.dir[1];
        sNdsRendererAdapterStagePersistentStats.light_dir_z =
            sNdsFighterDisplayCurrentLight.l.dir[2];
        sNdsRendererAdapterStagePersistentStats.light_dir_mask = 1u;
    }
    ndsRendererInitVertexCache(&sNdsRendererAdapterStageVertexCache);
    sNdsRendererAdapterStagePersistentActive = TRUE;
    ndsRendererProfileSetOwner(NDS_RENDERER_PROFILE_OWNER_STAGE);
}

void ndsRendererAdapterEndStageTraversal(void)
{
    ndsRendererProfileSetOwner(NDS_RENDERER_PROFILE_OWNER_NONE);
    sNdsRendererAdapterStagePersistentActive = FALSE;
}

static s32 ndsRendererAdapterStageValidateRange(const Gfx *dl, size_t bytes,
                                                void *user)
{
    (void)user;

    if ((((uintptr_t)dl & (sizeof(u32) - 1u)) != 0u) ||
        ((ndsFighterDLScanRangeInTaskmanArena(dl, bytes) == FALSE) &&
         (ndsRelocFindLoadedFileContaining(dl, bytes) == NULL) &&
         (ndsRendererAdapterRangeIsEmptySegmentEDL(dl, bytes) == FALSE)))
    {
        return FALSE;
    }
    return TRUE;
}

static sb32 ndsRendererAdapterStageDObjDrawable(DObj *dobj, u32 kind)
{
    if (dobj == NULL)
    {
        return FALSE;
    }
    if ((dobj->flags & DOBJ_FLAG_HIDDEN) != 0)
    {
        return FALSE;
    }

    switch (kind)
    {
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS:
        return ((dobj->flags & DOBJ_FLAG_NOTEXTURE) == 0) ? TRUE : FALSE;

    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLLINKS:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD0:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1:
        return (dobj->flags == DOBJ_FLAG_NONE) ? TRUE : FALSE;

    default:
        return FALSE;
    }
}

static u32 ndsRendererAdapterMaterialFlags(const MObj *mobj)
{
    u32 flags;

    if (mobj == NULL)
    {
        return MOBJ_FLAG_NONE;
    }
    flags = mobj->sub.flags;
    return (flags == MOBJ_FLAG_NONE) ?
        (MOBJ_FLAG_TEXTURE | 0x20u | MOBJ_FLAG_ALPHA) : flags;
}

static u32 ndsRendererAdapterMaterialPositiveOrOne(s32 value)
{
    return (value <= 0) ? 1u : (u32)value;
}

static void ndsRendererAdapterMaterialLoadBlock(const MObj *mobj,
                                                u32 *texels,
                                                u32 *dxt)
{
    s32 load_texels = 0;
    u32 divisor = 1u;

    if ((mobj == NULL) || (texels == NULL) || (dxt == NULL))
    {
        return;
    }

    switch (mobj->sub.block_siz)
    {
    case G_IM_SIZ_4b:
        load_texels =
            ((((s32)mobj->sub.block_dxt * (s32)mobj->sub.unk36) + 3) >> 2) -
            1;
        divisor = ndsRendererAdapterMaterialPositiveOrOne(
            (s32)mobj->sub.block_dxt / 16);
        break;
    case G_IM_SIZ_8b:
        load_texels =
            ((((s32)mobj->sub.block_dxt * (s32)mobj->sub.unk36) + 1) >> 1) -
            1;
        divisor = ndsRendererAdapterMaterialPositiveOrOne(
            (s32)mobj->sub.block_dxt / 8);
        break;
    case G_IM_SIZ_16b:
        load_texels =
            ((s32)mobj->sub.block_dxt * (s32)mobj->sub.unk36) - 1;
        divisor = ndsRendererAdapterMaterialPositiveOrOne(
            ((s32)mobj->sub.block_dxt * 2) / 8);
        break;
    case G_IM_SIZ_32b:
        load_texels =
            ((s32)mobj->sub.block_dxt * (s32)mobj->sub.unk36) - 1;
        divisor = ndsRendererAdapterMaterialPositiveOrOne(
            ((s32)mobj->sub.block_dxt * 4) / 8);
        break;
    default:
        break;
    }

    *texels = (load_texels > 0) ? (u32)load_texels : 0u;
    *dxt = (divisor + 0x7ffu) / divisor;
}

static u32 ndsRendererAdapterMaterialCommandCount(const MObj *mobj, u32 flags)
{
    u32 count = 1u;

    if (((flags & MOBJ_FLAG_PALETTE) == 0u) &&
        (mobj != NULL) &&
        (mobj->sub.palettes != NULL))
    {
        count++;
    }
    if ((flags & MOBJ_FLAG_PALETTE) != 0)
    {
        count++;
        if ((flags & (MOBJ_FLAG_SPLIT | MOBJ_FLAG_ALPHA)) != 0)
        {
            count += 5u;
        }
    }
    if ((flags & MOBJ_FLAG_LIGHT1) != 0)
    {
        count += 2u;
    }
    if ((flags & MOBJ_FLAG_LIGHT2) != 0)
    {
        count += 2u;
    }
    if ((flags & (MOBJ_FLAG_PRIMCOLOR | MOBJ_FLAG_FRAC | 0x8u)) != 0)
    {
        count++;
    }
    if ((flags & MOBJ_FLAG_ENVCOLOR) != 0)
    {
        count++;
    }
    if ((flags & MOBJ_FLAG_BLENDCOLOR) != 0)
    {
        count++;
    }
    if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_SPLIT)) != 0)
    {
        count++;
        if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0)
        {
            count += 3u;
        }
    }
    if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0)
    {
        count++;
    }
    if ((flags & 0x20u) != 0)
    {
        count++;
    }
    if ((flags & 0x40u) != 0)
    {
        count++;
    }
    if ((flags & MOBJ_FLAG_TEXTURE) != 0)
    {
        count++;
    }
    return count;
}

static sb32 ndsRendererAdapterCountMaterialCommands(DObj *dobj,
                                                    u32 *mobj_count,
                                                    u32 *branch_commands)
{
    MObj *mobj;
    u32 count = 0u;
    u32 commands = 0u;

    if ((dobj == NULL) || (mobj_count == NULL) ||
        (branch_commands == NULL))
    {
        return FALSE;
    }
    for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next)
    {
        count++;
        if (count > NDS_RENDERER_ADAPTER_MATERIAL_MOBJ_MAX)
        {
            return FALSE;
        }
        commands += ndsRendererAdapterMaterialCommandCount(
            mobj, ndsRendererAdapterMaterialFlags(mobj));
    }
    *mobj_count = count;
    *branch_commands = commands;
    return TRUE;
}

static void ndsRendererAdapterMaterialTextureState(
    const MObj *mobj,
    u32 flags,
    f32 *scau,
    f32 *scav,
    f32 *trau,
    f32 *trav,
    f32 *scrollu,
    f32 *scrollv)
{
    if ((mobj == NULL) || (scau == NULL) || (scav == NULL) ||
        (trau == NULL) || (trav == NULL) || (scrollu == NULL) ||
        (scrollv == NULL) ||
        ((flags & (MOBJ_FLAG_TEXTURE | 0x40u | 0x20u)) == 0))
    {
        return;
    }

    *scau = mobj->sub.scau;
    *scav = mobj->sub.scav;
    *trau = mobj->sub.trau;
    *trav = mobj->sub.trav;
    *scrollu = mobj->sub.scrollu;
    *scrollv = mobj->sub.scrollv;

    if (mobj->sub.unk10 == 1)
    {
        *scau *= 0.5F;
        *trau =
            ((*trau - mobj->sub.unk24) + 1.0F -
             (mobj->sub.unk28 * 0.5F)) *
            0.5F;
        *scrollu =
            ((*scrollu - mobj->sub.unk44) + 1.0F -
             (mobj->sub.unk28 * 0.5F)) *
            0.5F;
    }
}

static u32 ndsRendererAdapterClampU8S32(s32 value)
{
    if (value < 0)
    {
        return 0u;
    }
    return (value > 0xff) ? 0xffu : (u32)value;
}

static u32 ndsRendererAdapterClampU8F32(f32 value)
{
    return ndsRendererAdapterClampU8S32((s32)value);
}

static u32 ndsRendererAdapterPackColor(const SYColorPack *color)
{
    if (color == NULL)
    {
        return 0u;
    }
    return ((u32)color->s.r << 24) |
           ((u32)color->s.g << 16) |
           ((u32)color->s.b << 8) |
           (u32)color->s.a;
}

static const void *ndsRendererAdapterReadPointerEntry(void **items,
                                                      s32 index)
{
    if ((items == NULL) || (index < 0))
    {
        return NULL;
    }
    return items[index];
}

static void ndsRendererAdapterEmitBranchTableCommand(Gfx *cmd,
                                                     const Gfx *branch)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 = (NDS_FIGHTER_DL_OP_DL << 24) | (1u << 16);
    cmd->words.w1 = (u32)(uintptr_t)branch;
}

static void ndsRendererAdapterEmitEndDL(Gfx *cmd)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 = NDS_FIGHTER_DL_OP_ENDDL << 24;
    cmd->words.w1 = 0u;
}

static void ndsRendererAdapterEmitSync(Gfx *cmd, u32 op)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 = op << 24;
    cmd->words.w1 = 0u;
}

static void ndsRendererAdapterEmitTextureImage(Gfx *cmd,
                                               u32 fmt,
                                               u32 siz,
                                               u32 width,
                                               const void *image)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 =
        (NDS_FIGHTER_DL_OP_SETTIMG << 24) |
        ((fmt & 0x7u) << 21) |
        ((siz & 0x3u) << 19) |
        (((width != 0u) ? (width - 1u) : 0u) & 0x0fffu);
    cmd->words.w1 = (u32)(uintptr_t)image;
}

static void ndsRendererAdapterEmitSetTile(Gfx *cmd,
                                          u32 fmt,
                                          u32 siz,
                                          u32 line,
                                          u32 tmem,
                                          u32 tile,
                                          u32 palette,
                                          u32 cmt,
                                          u32 maskt,
                                          u32 shiftt,
                                          u32 cms,
                                          u32 masks,
                                          u32 shifts)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 =
        (NDS_FIGHTER_DL_OP_SETTILE << 24) |
        ((fmt & 0x7u) << 21) |
        ((siz & 0x3u) << 19) |
        ((line & 0x01ffu) << 9) |
        (tmem & 0x01ffu);
    cmd->words.w1 =
        ((tile & 0x7u) << 24) |
        ((palette & 0x0fu) << 20) |
        ((cmt & 0x3u) << 18) |
        ((maskt & 0x0fu) << 14) |
        ((shiftt & 0x0fu) << 10) |
        ((cms & 0x3u) << 8) |
        ((masks & 0x0fu) << 4) |
        (shifts & 0x0fu);
}

static void ndsRendererAdapterEmitLoadTlut(Gfx *cmd, u32 tile, u32 count)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 = NDS_FIGHTER_DL_OP_LOADTLUT << 24;
    cmd->words.w1 =
        ((tile & 0x7u) << 24) |
        ((count & 0x03ffu) << 14);
}

static void ndsRendererAdapterEmitMoveWord(Gfx *cmd,
                                           u32 index,
                                           u32 offset,
                                           u32 data)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 =
        (NDS_FIGHTER_DL_OP_MOVEWORD << 24) |
        ((index & 0xffu) << 16) |
        (offset & 0xffffu);
    cmd->words.w1 = data;
}

static Gfx *ndsRendererAdapterEmitLightColor(Gfx *branch_dl,
                                             u32 light,
                                             u32 color)
{
    u32 offset_a = NDS_RENDERER_ADAPTER_G_MWO_A_LIGHT_1;
    u32 offset_b = NDS_RENDERER_ADAPTER_G_MWO_B_LIGHT_1;

    if (branch_dl == NULL)
    {
        return branch_dl;
    }
    if (light == 2u)
    {
        offset_a = NDS_RENDERER_ADAPTER_G_MWO_A_LIGHT_2;
        offset_b = NDS_RENDERER_ADAPTER_G_MWO_B_LIGHT_2;
    }
    ndsRendererAdapterEmitMoveWord(branch_dl++,
                                   NDS_RENDERER_ADAPTER_G_MW_LIGHTCOL,
                                   offset_a,
                                   color);
    ndsRendererAdapterEmitMoveWord(branch_dl++,
                                   NDS_RENDERER_ADAPTER_G_MW_LIGHTCOL,
                                   offset_b,
                                   color);
    return branch_dl;
}

static void ndsRendererAdapterEmitPrimColor(Gfx *cmd,
                                            u32 m,
                                            u32 l,
                                            u32 r,
                                            u32 g,
                                            u32 b,
                                            u32 a)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 =
        (NDS_FIGHTER_DL_OP_SETPRIMCOLOR << 24) |
        ((m & 0xffu) << 8) |
        (l & 0xffu);
    cmd->words.w1 =
        ((r & 0xffu) << 24) |
        ((g & 0xffu) << 16) |
        ((b & 0xffu) << 8) |
        (a & 0xffu);
}

static void ndsRendererAdapterEmitColor(Gfx *cmd,
                                        u32 op,
                                        u32 r,
                                        u32 g,
                                        u32 b,
                                        u32 a)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 = op << 24;
    cmd->words.w1 =
        ((r & 0xffu) << 24) |
        ((g & 0xffu) << 16) |
        ((b & 0xffu) << 8) |
        (a & 0xffu);
}

static void ndsRendererAdapterEmitLoadBlock(Gfx *cmd,
                                            u32 tile,
                                            u32 uls,
                                            u32 ult,
                                            u32 lrs,
                                            u32 dxt)
{
    if (cmd == NULL)
    {
        return;
    }
    if (lrs > NDS_RENDERER_ADAPTER_G_TX_LDBLK_MAX_TXL)
    {
        lrs = NDS_RENDERER_ADAPTER_G_TX_LDBLK_MAX_TXL;
    }
    cmd->words.w0 =
        (NDS_FIGHTER_DL_OP_LOADBLOCK << 24) |
        ((uls & 0x0fffu) << 12) |
        (ult & 0x0fffu);
    cmd->words.w1 =
        ((tile & 0x7u) << 24) |
        ((lrs & 0x0fffu) << 12) |
        (dxt & 0x0fffu);
}

static void ndsRendererAdapterEmitTileSize(Gfx *cmd,
                                           u32 tile,
                                           s32 uls,
                                           s32 ult,
                                           s32 lrs,
                                           s32 lrt)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 =
        (NDS_FIGHTER_DL_OP_SETTILESIZE << 24) |
        (((u32)uls & 0x0fffu) << 12) |
        ((u32)ult & 0x0fffu);
    cmd->words.w1 =
        ((tile & 0x7u) << 24) |
        (((u32)lrs & 0x0fffu) << 12) |
        ((u32)lrt & 0x0fffu);
}

static void ndsRendererAdapterEmitTexture(Gfx *cmd,
                                          u32 s,
                                          u32 t,
                                          u32 level,
                                          u32 tile,
                                          u32 on)
{
    if (cmd == NULL)
    {
        return;
    }
    cmd->words.w0 =
        (NDS_FIGHTER_DL_OP_TEXTURE << 24) |
        ((level & 0x7u) << 11) |
        ((tile & 0x7u) << 8) |
        ((on & 0x7fu) << 1);
    cmd->words.w1 =
        ((s & 0xffffu) << 16) |
        (t & 0xffffu);
}

static void ndsRendererAdapterNativeMaterialImage(
    u32 fmt, u32 siz, u32 width, const void *image,
    u32 *out_w0, u32 *out_image)
{
    if ((out_w0 == NULL) || (out_image == NULL))
    {
        return;
    }
    *out_w0 =
        (NDS_FIGHTER_DL_OP_SETTIMG << 24) |
        ((fmt & 0x7u) << 21) |
        ((siz & 0x3u) << 19) |
        (((width != 0u) ? (width - 1u) : 0u) & 0x0fffu);
    *out_image = (u32)(uintptr_t)image;
}

static void ndsRendererAdapterNativeMaterialTile(
    u32 fmt, u32 siz, u32 line, u32 tmem, u32 tile, u32 palette,
    u32 cmt, u32 maskt, u32 shiftt, u32 cms, u32 masks, u32 shifts,
    u32 *out_w0, u32 *out_w1)
{
    if ((out_w0 == NULL) || (out_w1 == NULL))
    {
        return;
    }
    *out_w0 =
        (NDS_FIGHTER_DL_OP_SETTILE << 24) |
        ((fmt & 0x7u) << 21) |
        ((siz & 0x3u) << 19) |
        ((line & 0x01ffu) << 9) |
        (tmem & 0x01ffu);
    *out_w1 =
        ((tile & 0x7u) << 24) |
        ((palette & 0x0fu) << 20) |
        ((cmt & 0x3u) << 18) |
        ((maskt & 0x0fu) << 14) |
        ((shiftt & 0x0fu) << 10) |
        ((cms & 0x3u) << 8) |
        ((masks & 0x0fu) << 4) |
        (shifts & 0x0fu);
}

static void ndsRendererAdapterNativeMaterialTileSize(
    u32 tile, s32 uls, s32 ult, s32 lrs, s32 lrt,
    u32 *out_w0, u32 *out_w1)
{
    if ((out_w0 == NULL) || (out_w1 == NULL))
    {
        return;
    }
    *out_w0 =
        (NDS_FIGHTER_DL_OP_SETTILESIZE << 24) |
        (((u32)uls & 0x0fffu) << 12) |
        ((u32)ult & 0x0fffu);
    *out_w1 =
        ((tile & 0x7u) << 24) |
        (((u32)lrs & 0x0fffu) << 12) |
        ((u32)lrt & 0x0fffu);
}

static sb32 ndsRendererAdapterBuildNativeMaterialSnapshot(
    MObj *mobj, NDSRendererNativeMaterial *out, sb32 advance_texture_ids,
    s32 *out_curr, s32 *out_next)
{
    u32 flags;
    f32 scau = 0.0F;
    f32 scav = 0.0F;
    f32 trau = 0.0F;
    f32 trav = 0.0F;
    f32 scrollu = 0.0F;
    f32 scrollv = 0.0F;
    s32 uls;
    s32 ult;
    s32 s;
    s32 t;
    s32 texture_id_curr;
    s32 texture_id_next;

    if ((mobj == NULL) || (out == NULL))
    {
        return FALSE;
    }
    texture_id_curr = mobj->texture_id_curr;
    texture_id_next = mobj->texture_id_next;
    bzero(out, sizeof(*out));
    flags = ndsRendererAdapterMaterialFlags(mobj);
    out->command_count = 1u; /* ENDDL */
    ndsRendererAdapterMaterialTextureState(
        mobj, flags, &scau, &scav, &trau, &trav, &scrollu, &scrollv);

    if (((flags & MOBJ_FLAG_PALETTE) == 0u) &&
        (mobj->sub.palettes != NULL))
    {
        const void *palette = ndsRendererAdapterReadPointerEntry(
            mobj->sub.palettes, (s32)mobj->palette_id);

        if (palette != NULL)
        {
            out->effects |= NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE;
            ndsRendererAdapterNativeMaterialImage(
                G_IM_FMT_RGBA, G_IM_SIZ_16b, 1u, palette,
                &out->palette_image_w0, &out->palette_image);
            out->command_count++;
        }
    }
    if ((flags & MOBJ_FLAG_PALETTE) != 0u)
    {
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE;
        ndsRendererAdapterNativeMaterialImage(
            G_IM_FMT_RGBA, G_IM_SIZ_16b, 1u,
            ndsRendererAdapterReadPointerEntry(
                mobj->sub.palettes, (s32)mobj->palette_id),
            &out->palette_image_w0, &out->palette_image);
        out->command_count++;
        if ((flags & (MOBJ_FLAG_SPLIT | MOBJ_FLAG_ALPHA)) != 0u)
        {
            out->effects |= NDS_RENDERER_NATIVE_MATERIAL_PALETTE_TLUT;
            ndsRendererAdapterNativeMaterialTile(
                G_IM_FMT_RGBA, G_IM_SIZ_4b, 0u, 0x0100u, 5u, 0u,
                NDS_RENDERER_ADAPTER_G_TX_WRAP,
                NDS_RENDERER_ADAPTER_G_TX_NOMASK,
                NDS_RENDERER_ADAPTER_G_TX_NOLOD,
                NDS_RENDERER_ADAPTER_G_TX_WRAP,
                NDS_RENDERER_ADAPTER_G_TX_NOMASK,
                NDS_RENDERER_ADAPTER_G_TX_NOLOD,
                &out->palette_tile_w0, &out->palette_tile_w1);
            out->palette_tlut_w1 =
                (5u << 24) |
                (((mobj->sub.siz == G_IM_SIZ_8b) ? 0xffu : 0x0fu) << 14);
            out->sync_count += 3u;
            out->command_count += 5u;
        }
    }
    if ((flags & MOBJ_FLAG_LIGHT1) != 0u)
    {
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_LIGHT1;
        out->light1 = ndsRendererAdapterPackColor(&mobj->sub.light1color);
        out->command_count += 2u;
    }
    if ((flags & MOBJ_FLAG_LIGHT2) != 0u)
    {
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_LIGHT2;
        out->light2 = ndsRendererAdapterPackColor(&mobj->sub.light2color);
        out->command_count += 2u;
    }
    if ((flags & (MOBJ_FLAG_PRIMCOLOR | MOBJ_FLAG_FRAC | 0x8u)) != 0u)
    {
        u32 level;

        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_PRIM;
        if ((flags & MOBJ_FLAG_FRAC) != 0u)
        {
            s32 trunc = (s32)mobj->lfrac;

            level = ndsRendererAdapterClampU8F32(
                (mobj->lfrac - (f32)trunc) * 256.0F);
            texture_id_curr = trunc;
            texture_id_next = trunc + 1;
        }
        else
        {
            level = ndsRendererAdapterClampU8F32(mobj->lfrac * 255.0F);
        }
        out->prim_w0 =
            (NDS_FIGHTER_DL_OP_SETPRIMCOLOR << 24) |
            (((u32)mobj->sub.prim_m & 0xffu) << 8) |
            (level & 0xffu);
        out->prim_w1 = ndsRendererAdapterPackColor(&mobj->sub.primcolor);
        out->command_count++;
    }
    if ((flags & MOBJ_FLAG_ENVCOLOR) != 0u)
    {
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_ENV;
        out->env_color = ndsRendererAdapterPackColor(&mobj->sub.envcolor);
        out->command_count++;
    }
    if ((flags & MOBJ_FLAG_BLENDCOLOR) != 0u)
    {
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_BLEND;
        out->blend_color =
            ndsRendererAdapterPackColor(&mobj->sub.blendcolor);
        out->command_count++;
    }
    if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_SPLIT)) != 0u)
    {
        u32 block_siz = (mobj->sub.block_siz == G_IM_SIZ_32b) ?
            G_IM_SIZ_32b : G_IM_SIZ_16b;

        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_BLOCK_IMAGE;
        ndsRendererAdapterNativeMaterialImage(
            mobj->sub.block_fmt, block_siz, 1u,
            ndsRendererAdapterReadPointerEntry(
                mobj->sub.sprites, texture_id_next),
            &out->block_image_w0, &out->block_image);
        out->command_count++;
        if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0u)
        {
            u32 texels = 0u;
            u32 dxt = 0u;

            ndsRendererAdapterMaterialLoadBlock(mobj, &texels, &dxt);
            if (texels > NDS_RENDERER_ADAPTER_G_TX_LDBLK_MAX_TXL)
            {
                texels = NDS_RENDERER_ADAPTER_G_TX_LDBLK_MAX_TXL;
            }
            out->effects |= NDS_RENDERER_NATIVE_MATERIAL_LOAD_BLOCK;
            out->load_block_w0 = NDS_FIGHTER_DL_OP_LOADBLOCK << 24;
            out->load_block_w1 =
                (6u << 24) | ((texels & 0x0fffu) << 12) |
                (dxt & 0x0fffu);
            out->sync_count += 2u;
            out->command_count += 3u;
        }
    }
    if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0u)
    {
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE;
        ndsRendererAdapterNativeMaterialImage(
            mobj->sub.fmt, mobj->sub.siz, 1u,
            ndsRendererAdapterReadPointerEntry(
                mobj->sub.sprites, texture_id_curr),
            &out->current_image_w0, &out->current_image);
        out->command_count++;
    }
    if ((flags & 0x20u) != 0u)
    {
        if (mobj->sub.unk10 == 2)
        {
            uls = (ABSF(scau) > (1.0F / 65535.0F)) ?
                (s32)((((f32)mobj->sub.unk0C * trau) / scau) * 4.0F) : 0;
            ult = (ABSF(scav) > (1.0F / 65535.0F)) ?
                (s32)((((f32)mobj->sub.unk0E * trav) / scav) * 4.0F) : 0;
            if (uls < 0) { uls = 0; }
            if (ult < 0) { ult = 0; }
        }
        else
        {
            uls = (ABSF(scau) > (1.0F / 65535.0F)) ?
                (s32)(((((f32)mobj->sub.unk0C * trau) +
                         (f32)mobj->sub.unk0A) / scau) * 4.0F) : 0;
            ult = (ABSF(scav) > (1.0F / 65535.0F)) ?
                (s32)((((((1.0F - scav) - trav) *
                          (f32)mobj->sub.unk0E) +
                         (f32)mobj->sub.unk0A) / scav) * 4.0F) : 0;
        }
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_RENDER_TILE_SIZE;
        ndsRendererAdapterNativeMaterialTileSize(
            NDS_RENDERER_ADAPTER_G_TX_RENDERTILE, uls, ult,
            (((s32)mobj->sub.unk0C - 1) << 2) + uls,
            (((s32)mobj->sub.unk0E - 1) << 2) + ult,
            &out->render_tile_size_w0, &out->render_tile_size_w1);
        out->command_count++;
    }
    if ((flags & 0x40u) != 0u)
    {
        uls = (ABSF(scau) > (1.0F / 65535.0F)) ?
            (s32)(((((f32)mobj->sub.unk38 * scrollu) +
                     (f32)mobj->sub.unk0A) / scau) * 4.0F) : 0;
        ult = (ABSF(scav) > (1.0F / 65535.0F)) ?
            (s32)((((((1.0F - scav) - scrollv) *
                      (f32)mobj->sub.unk3A) +
                     (f32)mobj->sub.unk0A) / scav) * 4.0F) : 0;
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_SCROLL_TILE_SIZE;
        ndsRendererAdapterNativeMaterialTileSize(
            1u, uls, ult,
            (((s32)mobj->sub.unk38 - 1) << 2) + uls,
            (((s32)mobj->sub.unk3A - 1) << 2) + ult,
            &out->scroll_tile_size_w0, &out->scroll_tile_size_w1);
        out->command_count++;
    }
    if ((flags & MOBJ_FLAG_TEXTURE) != 0u)
    {
        if (mobj->sub.unk10 == 2)
        {
            s = (ABSF(scau) > (1.0F / 65535.0F)) ?
                (s32)(((f32)mobj->sub.unk0C * 64.0F) / scau) : 0;
            t = (ABSF(scav) > (1.0F / 65535.0F)) ?
                (s32)(((f32)mobj->sub.unk0E * 64.0F) / scav) : 0;
        }
        else
        {
            s = ((mobj->sub.unk08 != 0) &&
                 (ABSF(scau) > (1.0F / 65535.0F))) ?
                (s32)((2097152.0F / (f32)mobj->sub.unk08) / scau) : 0;
            t = ((mobj->sub.unk08 != 0) &&
                 (ABSF(scav) > (1.0F / 65535.0F))) ?
                (s32)((2097152.0F / (f32)mobj->sub.unk08) / scav) : 0;
        }
        if (s > 0xffff) { s = 0xffff; }
        if (t > 0xffff) { t = 0xffff; }
        out->effects |= NDS_RENDERER_NATIVE_MATERIAL_TEXTURE;
        out->texture_w0 =
            (NDS_FIGHTER_DL_OP_TEXTURE << 24) |
            (NDS_RENDERER_ADAPTER_G_TX_RENDERTILE << 8) |
            (NDS_RENDERER_ADAPTER_G_ON << 1);
        out->texture_w1 =
            (((u32)s & 0xffffu) << 16) | ((u32)t & 0xffffu);
        out->command_count++;
    }
    if (out_curr != NULL)
    {
        *out_curr = texture_id_curr;
    }
    if (out_next != NULL)
    {
        *out_next = texture_id_next;
    }
    if (advance_texture_ids != FALSE)
    {
        mobj->texture_id_curr = texture_id_curr;
        mobj->texture_id_next = texture_id_next;
    }
    return TRUE;
}

#if NDS_TICK_HUD
/* CYCLE 98 -- is the fighter material snapshot re-deriving a constant?
 *
 * BuildNativeMaterialSnapshot has no cache, so unlike the owner validate there
 * is no hit/miss to count. The equivalent question is content invariance: hash
 * the snapshot it just produced and compare it with the last snapshot produced
 * for that same MObj. `Same` is then the number of calls that recomputed a
 * value they had already computed, which is exactly the size of the deletion.
 *
 * Direct-mapped and O(1) on purpose. A linear table would search up to 128
 * entries per call at tens of calls per fighter per frame -- instrument cost
 * inside the very span being priced. Collisions are not hidden: a slot holding
 * a different key is counted as Evict, so a thrashing table reads as thrash
 * rather than as "every material varies".
 *
 * Charter 3.12: keyed on MObj pointers, which are arena addresses valid only
 * within a scene. The census only has to survive the match it measures, and P1
 * boots straight into one. */
#define NDS_FTR_PRE_MAT_TABLE_SIZE 256u

static const MObj *sNdsFtrPreMatKey[NDS_FTR_PRE_MAT_TABLE_SIZE];
static u32 sNdsFtrPreMatHash[NDS_FTR_PRE_MAT_TABLE_SIZE];

static void ndsFtrPreMaterialCensus(const MObj *mobj,
                                    const NDSRendererNativeMaterial *out)
{
    const u32 *words = (const u32 *)(const void *)out;
    u32 hash = 2166136261u;
    u32 i;
    u32 index;

    /* The whole struct is bzero'd at the top of the snapshot builder, so any
     * padding is deterministic and a word-wise hash is stable. */
    for (i = 0u; i < (u32)(sizeof(*out) / sizeof(u32)); i++)
    {
        hash ^= words[i];
        hash *= 16777619u;
    }
    /* Multiplicative rather than a low-bit mask: MObjs come out of the taskman
     * arena at a fixed stride, so their low address bits are the least
     * distinguishing ones they have. */
    index = ((u32)(uintptr_t)mobj * 2654435761u) >> 24;
    gNdsFtrPreMatCalls++;
    if (sNdsFtrPreMatKey[index] == mobj)
    {
        if (sNdsFtrPreMatHash[index] == hash)
        {
            gNdsFtrPreMatSame++;
        }
        else
        {
            gNdsFtrPreMatVariant++;
            sNdsFtrPreMatHash[index] = hash;
        }
        return;
    }
    if (sNdsFtrPreMatKey[index] != NULL)
    {
        gNdsFtrPreMatEvict++;
    }
    else
    {
        gNdsFtrPreMatNew++;
    }
    sNdsFtrPreMatKey[index] = mobj;
    sNdsFtrPreMatHash[index] = hash;
}
#endif

static sb32 ndsRendererAdapterBuildNativeMaterial(
    MObj *mobj, NDSRendererNativeMaterial *out)
{
#if NDS_TICK_HUD
    sb32 built = ndsRendererAdapterBuildNativeMaterialSnapshot(
        mobj, out, TRUE, NULL, NULL);

    if (built != FALSE)
    {
        ndsFtrPreMaterialCensus(mobj, out);
    }
    return built;
#else
    return ndsRendererAdapterBuildNativeMaterialSnapshot(
        mobj, out, TRUE, NULL, NULL);
#endif
}

/*
 * P2-2p8 M1 Native Draw List (NDL), first closed owners.
 *
 * The camera capture loop calls this before RecordCapturedDisplay.  A source
 * effect GObj is bound once for its current allocation lifetime, then the hot
 * path jumps straight to the existing typed native owner.  The binding stores
 * the exact DObjs that source traversal would reach; matrices and live MObj
 * state are still rebuilt from those source objects each frame.
 */
#define NDS_P2_NDL_RECORD_COUNT 64u
#define NDS_P2_NDL_OWNER_IMPACT_WAVE 1u
#define NDS_P2_NDL_OWNER_DAMAGE_SLASH 2u
/* 3 was the efground owner (Lakitu, Bronto Burt): their roots are baked now. */
#define NDS_P2_NDL_OWNER_GROUND 4u
#define NDS_P2_NDL_OWNER_WEAPON 5u
#define NDS_P2_NDL_OWNER_ITEM 6u
#define NDS_P2_NDL_OWNER_NEGATIVE 0xffffu
#define NDS_P2_NDL_FLAG_BOUND 1u
#define NDS_P2_NDL_DOBJ_MAX 2u

typedef struct NDSNdlRecord
{
    GObj *gobj;
    u32 serial;
    u16 owner;
    u8 kind;
    u8 flags;
    DObj *dobj[NDS_P2_NDL_DOBJ_MAX];
    const Gfx *body[NDS_P2_NDL_DOBJ_MAX];
    u32 hdr[NDS_P2_NDL_DOBJ_MAX];
    u8 body_count;
    u8 pad[3];
} NDSNdlRecord;

volatile u32 gNdsNdlDispatch[NDS_P2_NDL_KIND_COUNT];
volatile u32 gNdsNdlFallback[NDS_P2_NDL_KIND_COUNT];
volatile u32 gNdsNdlProcsSkipped;
volatile u32 gNdsNdlBindCount;
volatile u32 gNdsNdlNegativeBindCount;
volatile u32 gNdsNdlItemRejectStep;

static NDSNdlRecord sNdsP2NdlRecords[NDS_P2_NDL_RECORD_COUNT];

static u32 ndsP2NdlRecordIndex(const GObj *gobj)
{
    return ((u32)(uintptr_t)gobj >> 4) & (NDS_P2_NDL_RECORD_COUNT - 1u);
}

static sb32 ndsP2NdlDObjVisible(const DObj *dobj)
{
    const DObj *walk = dobj;
    u32 depth = 0u;

    if ((dobj == NULL) || ((dobj->flags & DOBJ_FLAG_NOTEXTURE) != 0u))
    {
        return FALSE;
    }
    /*
     * BattleShip roots end at DOBJ_PARENT_NULL ((DObj *)1), not at C NULL.
     * The source tree walkers stop on that sentinel; dereferencing it here
     * faults on the first direct ImpactWave draw.
     */
    while ((walk != NULL) && (walk != DOBJ_PARENT_NULL) &&
           (depth++ < NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX))
    {
        if ((walk->flags & DOBJ_FLAG_HIDDEN) != 0u)
        {
            return FALSE;
        }
        walk = walk->parent;
    }
    return (walk == DOBJ_PARENT_NULL) ? TRUE : FALSE;
}

static void ndsP2NdlScanDamageSlashTree(DObj *dobj, const u8 *asset_base,
                                         NDSNdlRecord *record, u32 depth)
{
    DObj *sibling;

    if ((dobj == NULL) || (asset_base == NULL) || (record == NULL) ||
        (depth >= NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX))
    {
        return;
    }
    if (dobj->dl_link != NULL)
    {
        DObjDLLink *link = dobj->dl_link;
        u32 i;

        for (i = 0u; i < 8u; i++, link++)
        {
            u32 root = 0u;
            u32 j;

            if (link->list_id == NDS_RENDERER_STAGE_DL_HEADS)
            {
                break;
            }
            if ((link->list_id < 0) ||
                ((u32)link->list_id >= NDS_RENDERER_STAGE_DL_HEADS))
            {
                break;
            }
            if (link->dl == (const Gfx *)(asset_base +
                    NDS_NATIVE_DAMAGE_SLASH_ROOT0))
            {
                root = NDS_NATIVE_DAMAGE_SLASH_ROOT0;
            }
            else if (link->dl == (const Gfx *)(asset_base +
                         NDS_NATIVE_DAMAGE_SLASH_ROOT1))
            {
                root = NDS_NATIVE_DAMAGE_SLASH_ROOT1;
            }
            if (root == 0u)
            {
                continue;
            }
            for (j = 0u; j < record->body_count; j++)
            {
                if (record->hdr[j] == root)
                {
                    root = 0u;
                    break;
                }
            }
            if ((root != 0u) &&
                (record->body_count < NDS_P2_NDL_DOBJ_MAX))
            {
                u32 out = record->body_count++;
                record->dobj[out] = dobj;
                record->body[out] = link->dl;
                record->hdr[out] = root;
            }
        }
    }
    if (dobj->child != NULL)
    {
        ndsP2NdlScanDamageSlashTree(
            dobj->child, asset_base, record, depth + 1u);
    }
    if (dobj->sib_prev == NULL)
    {
        sibling = dobj->sib_next;
        while (sibling != NULL)
        {
            ndsP2NdlScanDamageSlashTree(
                sibling, asset_base, record, depth + 1u);
            sibling = sibling->sib_next;
        }
    }
}

#if NDS_P2_LINK
extern void itDisplayColAnimXLUProcDisplay(GObj *item_gobj);

static void ndsP2NdlScanLinkBombTree(DObj *dobj, const u8 *asset_base,
                                     NDSNdlRecord *record, u32 depth)
{
    DObj *sibling;

    if ((dobj == NULL) || (asset_base == NULL) || (record == NULL) ||
        (depth >= NDS_RENDERER_ADAPTER_DOBJ_PARENT_MAX))
    {
        return;
    }
    if (dobj->dl_link != NULL)
    {
        DObjDLLink *link = dobj->dl_link;
        u32 i;

        for (i = 0u; i < GC_COMMON_MAX_DLLINKS; i++, link++)
        {
            u32 root = 0u;
            u32 j;

            if (link->list_id == NDS_RENDERER_STAGE_DL_HEADS)
            {
                break;
            }
            if ((link->list_id < 0) ||
                ((u32)link->list_id >= NDS_RENDERER_STAGE_DL_HEADS))
            {
                break;
            }
            if (link->dl == (const Gfx *)(asset_base +
                    NDS_NATIVE_LINK_BOMB_BODY_ROOT))
            {
                root = NDS_NATIVE_LINK_BOMB_BODY_ROOT;
            }
            else if (link->dl == (const Gfx *)(asset_base +
                         NDS_NATIVE_LINK_BOMB_FUSE_ROOT))
            {
                root = NDS_NATIVE_LINK_BOMB_FUSE_ROOT;
            }
            if (root == 0u)
            {
                continue;
            }
            for (j = 0u; j < record->body_count; j++)
            {
                if (record->hdr[j] == root)
                {
                    root = 0u;
                    break;
                }
            }
            if ((root != 0u) &&
                (record->body_count < NDS_P2_NDL_DOBJ_MAX))
            {
                u32 out = record->body_count++;
                record->dobj[out] = dobj;
                record->body[out] = link->dl;
                record->hdr[out] = root;
            }
        }
    }
    if (dobj->child != NULL)
    {
        ndsP2NdlScanLinkBombTree(
            dobj->child, asset_base, record, depth + 1u);
    }
    if (dobj->sib_prev == NULL)
    {
        sibling = dobj->sib_next;
        while (sibling != NULL)
        {
            ndsP2NdlScanLinkBombTree(
                sibling, asset_base, record, depth + 1u);
            sibling = sibling->sib_next;
        }
    }
}

static sb32 ndsP2NdlValidateLinkBombRecord(
    const NDSNdlRecord *record, const u8 *asset_base, u32 asset_bytes)
{
    u32 mask = 0u;
    u32 i;

    if ((record == NULL) || (asset_base == NULL) ||
        (asset_bytes < NDS_NATIVE_LINK_BOMB_FILE_END) ||
        (record->body_count != 2u))
    {
        return FALSE;
    }
    for (i = 0u; i < record->body_count; i++)
    {
        const Gfx *dl = record->body[i];

        if ((record->dobj[i] == NULL) || (record->dobj[i]->mobj != NULL) ||
            (dl == NULL))
        {
            return FALSE;
        }
        if (record->hdr[i] == NDS_NATIVE_LINK_BOMB_BODY_ROOT)
        {
            if ((asset_bytes <
                    (NDS_NATIVE_LINK_BOMB_BODY_ROOT +
                     NDS_NATIVE_LINK_BOMB_BODY_DL_BYTES)) ||
                (dl[11].words.w0 != NDS_NATIVE_LINK_BOMB_BODY_TLUT_W0) ||
                (dl[11].words.w1 != (u32)(uintptr_t)(
                    asset_base + NDS_NATIVE_LINK_BOMB_TLUT_OFFSET)) ||
                (dl[17].words.w0 != NDS_NATIVE_LINK_BOMB_BODY_IMAGE_W0) ||
                (dl[17].words.w1 != (u32)(uintptr_t)(
                    asset_base + NDS_NATIVE_LINK_BOMB_BODY_IMAGE_OFFSET)))
            {
                return FALSE;
            }
            mask |= 1u;
        }
        else if (record->hdr[i] == NDS_NATIVE_LINK_BOMB_FUSE_ROOT)
        {
            if ((asset_bytes <
                    (NDS_NATIVE_LINK_BOMB_FUSE_ROOT +
                     NDS_NATIVE_LINK_BOMB_FUSE_DL_BYTES)) ||
                (dl[13].words.w0 != NDS_NATIVE_LINK_BOMB_FUSE_IMAGE_W0) ||
                (dl[13].words.w1 != (u32)(uintptr_t)(
                    asset_base + NDS_NATIVE_LINK_BOMB_FUSE_IMAGE_OFFSET)))
            {
                return FALSE;
            }
            mask |= 2u;
        }
        else
        {
            return FALSE;
        }
    }
    return (mask == 3u) ? TRUE : FALSE;
}
#endif

static void ndsP2NdlBindRecord(GObj *gobj, u32 serial, NDSNdlRecord *record)
{
    DObj *root;
    u32 variant;
    u32 kind;

    bzero(record, sizeof(*record));
    record->gobj = gobj;
    record->serial = serial;
    record->owner = NDS_P2_NDL_OWNER_NEGATIVE;
    record->kind = 0xffu;
    record->flags = NDS_P2_NDL_FLAG_BOUND;

    if ((gobj->dl_link_id == 10) &&
        (ndsEFManagerImpactWaveVariant(gobj, &variant) != FALSE))
    {
        root = DObjGetStruct(gobj);
        if ((root != NULL) && (root->dl != NULL) && (root->mobj != NULL))
        {
            record->owner = NDS_P2_NDL_OWNER_IMPACT_WAVE;
            record->kind = 0u;
            record->dobj[0] = root;
            record->body[0] = root->dl;
            record->hdr[0] = variant;
            record->body_count = 1u;
            NDS_DIAG(gNdsNdlBindCount++);
            return;
        }
    }
    if ((gobj->dl_link_id == 18) &&
        (gobj->proc_display == gcDrawDObjTreeDLLinksForGObj))
    {
        const void *asset_view = NULL;
        u32 asset_bytes = 0u;

        if ((ndsRelocGetLoadedAssetView(
                 NDS_NATIVE_DAMAGE_SLASH_ASSET,
                 &asset_view, &asset_bytes) != FALSE) &&
            (asset_view != NULL) &&
            (asset_bytes >= NDS_NATIVE_DAMAGE_SLASH_PALETTE_END))
        {
            root = DObjGetStruct(gobj);
            ndsP2NdlScanDamageSlashTree(
                root, (const u8 *)asset_view, record, 0u);
            if (record->body_count == 2u)
            {
                record->owner = NDS_P2_NDL_OWNER_DAMAGE_SLASH;
                record->kind = 1u;
                NDS_DIAG(gNdsNdlBindCount++);
                return;
            }
            record->body_count = 0u;
        }
    }
    kind = ndsStageGCDrawAllLoopNdlGroundKind(gobj);
    if (kind < NDS_P2_NDL_KIND_COUNT)
    {
        record->owner = NDS_P2_NDL_OWNER_GROUND;
        record->kind = (u8)kind;
        record->dobj[0] = DObjGetStruct(gobj);
        record->body_count = 1u;
        NDS_DIAG(gNdsNdlBindCount++);
        return;
    }
    kind = ndsStageGCDrawAllLoopNdlWeaponKind(gobj);
    if (kind < NDS_P2_NDL_KIND_COUNT)
    {
        record->owner = NDS_P2_NDL_OWNER_WEAPON;
        record->kind = (u8)kind;
        record->dobj[0] = DObjGetStruct(gobj);
        record->body_count = 1u;
        NDS_DIAG(gNdsNdlBindCount++);
        return;
    }
#if NDS_P2_LINK
    if ((gobj->id == nGCCommonKindItem) &&
        (gobj->dl_link_id == 11) &&
        (gobj->proc_display == itDisplayColAnimXLUProcDisplay))
    {
        ITStruct *ip = itGetStruct(gobj);
        const void *asset_view = NULL;
        u32 asset_bytes = 0u;

        if ((ip != NULL) && (ip->kind == nITKindLinkBomb) &&
            (ndsRelocGetLoadedAssetView(
                 NDS_NATIVE_LINK_BOMB_ASSET,
                 &asset_view, &asset_bytes) != FALSE) &&
            (asset_view != NULL) &&
            (asset_bytes >= NDS_NATIVE_LINK_BOMB_FILE_END))
        {
            root = DObjGetStruct(gobj);
            ndsP2NdlScanLinkBombTree(
                root, (const u8 *)asset_view, record, 0u);
            if (ndsP2NdlValidateLinkBombRecord(
                    record, (const u8 *)asset_view, asset_bytes) != FALSE)
            {
                record->owner = NDS_P2_NDL_OWNER_ITEM;
                record->kind = NDS_P2_NDL_KIND_IT_LINK_BOMB;
                NDS_DIAG(gNdsNdlBindCount++);
                return;
            }
            record->body_count = 0u;
        }
    }
#endif
    NDS_DIAG(gNdsNdlNegativeBindCount++);
}

static void ndsP2NdlAccumulateStats(const NDSRendererStats *stats)
{
    if (stats == NULL)
    {
        return;
    }
    gNdsStageGCDrawAllLoopHardwareTriangleCount +=
        stats->hardware_triangle_count;
    gNdsStageGCDrawAllLoopHardwareZBufferTriangleCount +=
        stats->hardware_zbuffer_triangle_count;
    gNdsStageGCDrawAllLoopHardwareProjectedDepthTriangleCount +=
        stats->hardware_projected_depth_triangle_count;
    gNdsStageGCDrawAllLoopHardwareDecalDepthTriangleCount +=
        stats->hardware_decal_depth_triangle_count;
    gNdsStageGCDrawAllLoopHardwareTextureBindCount +=
        stats->hardware_texture_bind_count;
    gNdsStageGCDrawAllLoopHardwareTextureUploadCount +=
        stats->hardware_texture_upload_count;
    gNdsStageGCDrawAllLoopHardwareTextureReadyCount +=
        stats->hardware_texture_ready_count;
    gNdsStageGCDrawAllLoopHardwareTextureRejectCount +=
        stats->hardware_texture_reject_count;
    if (stats->hardware_texture_ready_count != 0u)
    {
        if (stats->hardware_texture_format < 32u)
        {
            gNdsStageGCDrawAllLoopHardwareTextureFormatMask |=
                1u << stats->hardware_texture_format;
        }
        if (stats->hardware_texture_width >
            gNdsStageGCDrawAllLoopHardwareTextureMaxWidth)
        {
            gNdsStageGCDrawAllLoopHardwareTextureMaxWidth =
                stats->hardware_texture_width;
        }
        if (stats->hardware_texture_height >
            gNdsStageGCDrawAllLoopHardwareTextureMaxHeight)
        {
            gNdsStageGCDrawAllLoopHardwareTextureMaxHeight =
                stats->hardware_texture_height;
        }
    }
}

static sb32 ndsP2NdlPrepareConfig(
    DObj *dobj, GObj *camera_gobj, NDSRendererConfig *config,
    NDSRendererMatrix20p12 *projection,
    NDSRendererMatrix20p12 *modelview,
    NDSRendererMatrix20p12 *identity)
{
    const NDSRendererMatrix20p12 *projection_ptr;
    const NDSRendererMatrix20p12 *modelview_ptr;

    if ((dobj == NULL) || (camera_gobj == NULL) || (config == NULL))
    {
        return FALSE;
    }
    ndsRendererAdapterPrepareInitialMatrices(
        dobj, CObjGetStruct(camera_gobj), TRUE,
        projection, &projection_ptr, modelview, &modelview_ptr);
    if ((projection_ptr == NULL) && (modelview_ptr == NULL))
    {
        return FALSE;
    }
    ndsRendererAdapterMtxIdentity20p12(identity);
    if (projection_ptr == NULL)
    {
        projection_ptr = identity;
    }
    if (modelview_ptr == NULL)
    {
        modelview_ptr = identity;
    }
    bzero(config, sizeof(*config));
    config->max_depth = 8u;
    config->max_commands = 8192u;
    config->max_list_commands = 512u;
    config->initial_projection = projection_ptr;
    config->initial_modelview = modelview_ptr;
    config->initial_geometry_mode =
        NDS_RENDERER_GEOM_RESET_MODE |
        NDS_RENDERER_GEOM_LIGHTING |
        NDS_RENDERER_GEOM_ZBUFFER;
    config->texture_data_layout =
        NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    return TRUE;
}

static sb32 ndsP2NdlEmitImpactWave(GObj *gobj, GObj *camera_gobj,
                                    NDSNdlRecord *record,
                                    NDSRendererStats *stats)
{
    NDSRendererConfig config;
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;
    NDSRendererMatrix20p12 identity;
    NDSRendererNativeMaterial material;
    NDSRendererStats trial_stats;
    u32 variant;
    u32 prim;
    u32 env;

    if ((record->body_count != 1u) ||
        (ndsP2NdlDObjVisible(record->dobj[0]) == FALSE) ||
        (record->dobj[0]->mobj == NULL) ||
        (ndsEFManagerImpactWaveNdlHeader(
             gobj, &variant, &prim, &env) == FALSE) ||
        (ndsRendererAdapterBuildNativeMaterial(
             record->dobj[0]->mobj, &material) == FALSE) ||
        (ndsP2NdlPrepareConfig(
             record->dobj[0], camera_gobj, &config,
             &projection, &modelview, &identity) == FALSE))
    {
        return FALSE;
    }
    /* Exact header emitted by efManagerImpactWaveProcDisplay before its DObj.
     * Keep the persistent renderer state transactional until the native owner
     * has crossed all rejectable checks; the submit's first GX mutation is now
     * its resident texture bind. */
    trial_stats = *stats;
    trial_stats.othermode_l = G_RM_AA_ZB_XLU_SURF | G_RM_AA_ZB_XLU_SURF2;
    trial_stats.prim_color = prim;
    trial_stats.env_color = env;
    if (ndsRendererSubmitNativeImpactWave(
            sNdsImpactWaveVertices,
            (u32)(sizeof(sNdsImpactWaveVertices) /
                  sizeof(sNdsImpactWaveVertices[0])),
            sNdsImpactWaveTriangles,
            (u32)(sizeof(sNdsImpactWaveTriangles) / 3u),
            record->body[0], &material, variant, &config,
            &trial_stats) == FALSE)
    {
        NDS_DIAG(gNdsImpactWaveNativeFallbackCount++);
        return FALSE;
    }
    *stats = trial_stats;
    NDS_DIAG(gNdsImpactWaveNativeDrawCount++);
    return TRUE;
}

static sb32 ndsP2NdlEmitDamageSlash(GObj *camera_gobj,
                                     NDSNdlRecord *record,
                                     NDSRendererStats *stats)
{
    const void *asset_view = NULL;
    u32 asset_bytes = 0u;
    NDSRendererConfig config[2];
    NDSRendererMatrix20p12 projection[2];
    NDSRendererMatrix20p12 modelview[2];
    NDSRendererMatrix20p12 identity[2];
    NDSRendererNativeMaterial material[2];
    u8 visible[2] = { FALSE, FALSE };
    u32 i;
    const u32 want_effects =
        NDS_RENDERER_NATIVE_MATERIAL_LIGHT1 |
        NDS_RENDERER_NATIVE_MATERIAL_LIGHT2 |
        NDS_RENDERER_NATIVE_MATERIAL_PRIM |
        NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE;

    if ((record->body_count != 2u) ||
        (ndsRelocGetLoadedAssetView(
             NDS_NATIVE_DAMAGE_SLASH_ASSET,
             &asset_view, &asset_bytes) == FALSE) ||
        (asset_view == NULL))
    {
        return FALSE;
    }
    /* First pass: validate both source children and their resident frame names
     * before either child can touch GX.  A FALSE return from this pass is a
     * clean source-proc fallback for the whole effect. */
    for (i = 0u; i < record->body_count; i++)
    {
        if (ndsP2NdlDObjVisible(record->dobj[i]) == FALSE)
        {
            continue;
        }
        visible[i] = TRUE;
        if ((record->dobj[i]->mobj == NULL) ||
            (record->dobj[i]->mobj->next != NULL) ||
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 record->dobj[i]->mobj, &material[i], FALSE,
                 NULL, NULL) == FALSE) ||
            (material[i].effects != want_effects) ||
            (ndsP2NdlPrepareConfig(
                 record->dobj[i], camera_gobj, &config[i],
                 &projection[i], &modelview[i], &identity[i]) == FALSE) ||
            (ndsRendererPreflightNativeDamageSlash(
                 asset_view, asset_bytes, record->hdr[i],
                 &material[i], &config[i]) == FALSE))
        {
            NDS_DIAG(gNdsDamageSlashSubmitFailCount++);
            return FALSE;
        }
    }
    for (i = 0u; i < record->body_count; i++)
    {
        if (visible[i] == FALSE)
        {
            continue;
        }
        if (ndsRendererSubmitNativeDamageSlash(
                asset_view, asset_bytes, record->hdr[i],
                &material[i], &config[i], stats) == FALSE)
        {
            /* Preflight made every route-1 rejection impossible without a
             * state change.  Count an invariant failure, but consume this proc
             * so a partially emitted owner can never be drawn a second time by
             * the source fallback.  Acceptance requires this counter to stay 0. */
            NDS_DIAG(gNdsDamageSlashSubmitFailCount++);
            return TRUE;
        }
        gNdsDamageSlashRootMask |=
            (record->hdr[i] == NDS_NATIVE_DAMAGE_SLASH_ROOT0) ? 1u : 2u;
        gNdsDamageSlashEffectsSeen |= material[i].effects;
        NDS_DIAG(gNdsDamageSlashDrawCount++);
    }
    return TRUE;
}

#if NDS_P2_LINK
static u32 ndsP2NdlPackItemColor(const GMColKeys *color)
{
    return ((u32)color->r << 24) | ((u32)color->g << 16) |
           ((u32)color->b << 8) | (u32)color->a;
}

static sb32 ndsP2NdlEmitLinkBomb(GObj *gobj, GObj *camera_gobj,
                                 NDSNdlRecord *record,
                                 NDSRendererStats *stats)
{
    ITStruct *ip;
    const void *asset_view = NULL;
    u32 asset_bytes = 0u;
    NDSRendererConfig config[NDS_P2_NDL_DOBJ_MAX];
    NDSRendererMatrix20p12 projection[NDS_P2_NDL_DOBJ_MAX];
    NDSRendererMatrix20p12 modelview[NDS_P2_NDL_DOBJ_MAX];
    NDSRendererMatrix20p12 identity[NDS_P2_NDL_DOBJ_MAX];
    u8 visible[NDS_P2_NDL_DOBJ_MAX] = { FALSE, FALSE };
    u32 i;

    gNdsNdlItemRejectStep = 1u;
    if ((record->body_count != 2u) ||
        (gobj->proc_display != itDisplayColAnimXLUProcDisplay))
    {
        return FALSE;
    }
    gNdsNdlItemRejectStep = 2u;
    ip = itGetStruct(gobj);
    if ((ip == NULL) || (ip->kind != nITKindLinkBomb) ||
        (ndsRelocGetLoadedAssetView(
             NDS_NATIVE_LINK_BOMB_ASSET,
             &asset_view, &asset_bytes) == FALSE) ||
        (asset_view == NULL) ||
        (asset_bytes < NDS_NATIVE_LINK_BOMB_FILE_END))
    {
        return FALSE;
    }
    gNdsNdlItemRejectStep = 3u;
    /* Source itDisplayColAnimXLUProcDisplay consumes the callback while a held
     * item is hidden with its fighter.  No tree walk and no GX output occur. */
    if (itDisplayCheckItemVisible(ip) == FALSE)
    {
        gNdsNdlItemRejectStep = 10u;
        return TRUE;
    }
    gNdsNdlItemRejectStep = 4u;
    /* Validate both live DObjs and matrix inputs before either native list can
     * touch GX.  Asset bytes are immutable for this allocation lifetime; the
     * per-frame fields below are the only state that can invalidate the bind. */
    for (i = 0u; i < record->body_count; i++)
    {
        const Gfx *expected;

        if ((record->dobj[i] == NULL) ||
            (record->dobj[i]->parent_gobj != gobj) ||
            (record->dobj[i]->mobj != NULL))
        {
            return FALSE;
        }
        gNdsNdlItemRejectStep = 5u;
        expected = (const Gfx *)((const u8 *)asset_view + record->hdr[i]);
        if ((record->body[i] != expected) ||
            (ndsP2NdlDObjVisible(record->dobj[i]) == FALSE))
        {
            gNdsNdlItemRejectStep = 6u;
            continue;
        }
        visible[i] = TRUE;
        gNdsNdlItemRejectStep = 7u;
        if (ndsP2NdlPrepareConfig(
                record->dobj[i], camera_gobj, &config[i],
                &projection[i], &modelview[i], &identity[i]) == FALSE)
        {
            return FALSE;
        }
        gNdsNdlItemRejectStep = 8u;
    }
    gNdsNdlItemRejectStep = 9u;
    for (i = 0u; i < record->body_count; i++)
    {
        NDSRendererStats trial_stats;

        if (visible[i] == FALSE)
        {
            continue;
        }
        trial_stats = *stats;
        /* itDisplayColAnimXLU writes the same live EnvColor into both heads.
         * The body consumes it; the fuse list immediately overwrites it. */
        trial_stats.env_color = (ip->colanim.is_use_color1 != FALSE) ?
            ndsP2NdlPackItemColor(&ip->colanim.color1) : 0u;
        if (ndsRendererSubmitNativeLinkBomb(
                record->hdr[i], asset_view, asset_bytes,
                &config[i], &trial_stats) == FALSE)
        {
            /* The owner has crossed into its native executor.  Never replay the
             * source proc after a possible GX mutation; keep the rejection loud. */
            NDS_DIAG(gNdsLinkBombSubmitFailCount++);
            return TRUE;
        }
        *stats = trial_stats;
        NDS_DIAG(gNdsLinkBombDrawCount++);
    }
    gNdsNdlItemRejectStep = 10u;
    return TRUE;
}
#endif

/*
 * Return TRUE only when the source proc_display is fully consumed. Any stale
 * lifetime, unknown owner, visibility/material mismatch or native decline
 * returns FALSE and the camera loop executes the original source route.
 */
static s32 ndsRendererAdapterNdlDispatchEffectBody(GObj *camera_gobj,
                                                  GObj *gobj);

/* P2-2p8 (2026-10-04): an admitted GObj's dispatch -- its record, its
 * owner's native emit -- runs on the DTCM hot stack, as the stage DL fast
 * lane does. With the one-slot modelview stack (nds_renderer_preamble.c)
 * the deepest owner (an impact wave's native ring) stays inside the 6 KB
 * stack; none reads storage, switches a coroutine or hands DMA a stack
 * address. */

typedef struct NDSNdlDispatchCall
{
    GObj *camera_gobj;
    GObj *gobj;
} NDSNdlDispatchCall;

static unsigned int ndsRendererAdapterNdlDispatchEffectOnHotStack(void *arg)
{
    const NDSNdlDispatchCall *call = (const NDSNdlDispatchCall *)arg;

    return (unsigned int)ndsRendererAdapterNdlDispatchEffectBody(
        call->camera_gobj, call->gobj);
}

s32 ndsRendererAdapterNdlDispatchEffect(void *camera_gobj_ptr,
                                        void *display_gobj_ptr,
                                        s32 link_id)
{
    GObj *camera_gobj = camera_gobj_ptr;
    GObj *gobj = display_gobj_ptr;

    if ((gNdsSceneManagerCurrIsBattle == 0u) ||
        (camera_gobj == NULL) || (gobj == NULL) ||
        ((gobj->id != nGCCommonKindEffect) &&
         (gobj->id != nGCCommonKindGround) &&
         (gobj->id != nGCCommonKindWeapon) &&
         (gobj->id != nGCCommonKindItem)) ||
        (gobj->dl_link_id != (u8)link_id))
    {
        return FALSE;
    }
    /* A GObj already bound as no NDL owner (the stage segments, every frame)
     * declines here, as the body would, without the hot-stack switch. An
     * unbound or stale record still goes to the body, which binds it. */
    {
        const u32 serial = ndsGcGetGObjLifetimeSerial(gobj);
        const NDSNdlRecord *record =
            &sNdsP2NdlRecords[ndsP2NdlRecordIndex(gobj)];

        if ((serial == 0u) ||
            ((record->gobj == gobj) && (record->serial == serial) &&
             ((record->flags & NDS_P2_NDL_FLAG_BOUND) != 0u) &&
             ((record->owner == NDS_P2_NDL_OWNER_NEGATIVE) ||
              (record->kind >= NDS_P2_NDL_KIND_COUNT))))
        {
            return FALSE;
        }
    }
    {
        NDSNdlDispatchCall call = { camera_gobj, gobj };

        return (s32)ndsDtcmHotStackRun(
            ndsRendererAdapterNdlDispatchEffectOnHotStack, &call);
    }
}

static s32 ndsRendererAdapterNdlDispatchEffectBody(GObj *camera_gobj,
                                                  GObj *gobj)
{
    NDSNdlRecord *record;
    NDSRendererStats *stats;
    u32 serial;
    u32 kind;
    sb32 handled = FALSE;
    void *saved_graphics_heap_ptr;

    serial = ndsGcGetGObjLifetimeSerial(gobj);
    if (serial == 0u)
    {
        return FALSE;
    }
    record = &sNdsP2NdlRecords[ndsP2NdlRecordIndex(gobj)];
    if ((record->gobj != gobj) || (record->serial != serial) ||
        ((record->flags & NDS_P2_NDL_FLAG_BOUND) == 0u))
    {
        ndsP2NdlBindRecord(gobj, serial, record);
    }
    if ((record->owner == NDS_P2_NDL_OWNER_NEGATIVE) ||
        (record->kind >= NDS_P2_NDL_KIND_COUNT))
    {
        return FALSE;
    }
    kind = record->kind;
    if (record->owner == NDS_P2_NDL_OWNER_GROUND)
    {
        handled = ndsStageGCDrawAllLoopSubmitNdlGround(
            camera_gobj, gobj, kind);
        if (handled != FALSE)
        {
            NDS_DIAG(gNdsNdlDispatch[kind]++);
            NDS_DIAG(gNdsNdlProcsSkipped++);
        }
        else
        {
            NDS_DIAG(gNdsNdlFallback[kind]++);
        }
        return handled;
    }
    if (record->owner == NDS_P2_NDL_OWNER_WEAPON)
    {
        handled = ndsStageGCDrawAllLoopSubmitNdlWeapon(
            camera_gobj, gobj, kind);
        if (handled != FALSE)
        {
            NDS_DIAG(gNdsNdlDispatch[kind]++);
            NDS_DIAG(gNdsNdlProcsSkipped++);
        }
        else
        {
            NDS_DIAG(gNdsNdlFallback[kind]++);
        }
        return handled;
    }
    if (record->owner == NDS_P2_NDL_OWNER_ITEM)
    {
#if NDS_P2_LINK
        saved_graphics_heap_ptr = gSYTaskmanGraphicsHeap.ptr;
        ndsRendererAdapterBeginStageTraversal();
        stats = &sNdsRendererAdapterStagePersistentStats;
        handled = (kind == NDS_P2_NDL_KIND_IT_LINK_BOMB) ?
            ndsP2NdlEmitLinkBomb(gobj, camera_gobj, record, stats) : FALSE;
        if (handled != FALSE)
        {
            ndsP2NdlAccumulateStats(stats);
            NDS_DIAG(gNdsNdlDispatch[kind]++);
            NDS_DIAG(gNdsNdlProcsSkipped++);
            ndsStageGCDrawAllLoopRecordNdlItemSubmit(
                gobj,
                stats->hardware_triangle_count,
                stats->hardware_texture_ready_count,
                stats->hardware_texture_reject_count);
        }
        else
        {
            NDS_DIAG(gNdsNdlFallback[kind]++);
        }
        ndsRendererAdapterEndStageTraversal();
        ndsTaskmanSampleGraphicsHeap();
        gSYTaskmanGraphicsHeap.ptr = saved_graphics_heap_ptr;
        return handled;
#else
        return FALSE;
#endif
    }
    saved_graphics_heap_ptr = gSYTaskmanGraphicsHeap.ptr;
    ndsRendererAdapterBeginStageTraversal();
    stats = &sNdsRendererAdapterStagePersistentStats;
    if (record->owner == NDS_P2_NDL_OWNER_IMPACT_WAVE)
    {
        handled = ndsP2NdlEmitImpactWave(gobj, camera_gobj, record, stats);
    }
    else if (record->owner == NDS_P2_NDL_OWNER_DAMAGE_SLASH)
    {
        handled = ndsP2NdlEmitDamageSlash(camera_gobj, record, stats);
    }
    if (handled != FALSE)
    {
        ndsP2NdlAccumulateStats(stats);
        NDS_DIAG(gNdsNdlDispatch[kind]++);
        NDS_DIAG(gNdsNdlProcsSkipped++);
        ndsStageGCDrawAllLoopRecordNdlEffectSubmit(
            stats->hardware_triangle_count,
            stats->hardware_texture_ready_count,
            stats->hardware_texture_reject_count);
    }
    else
    {
        NDS_DIAG(gNdsNdlFallback[kind]++);
    }
    ndsRendererAdapterEndStageTraversal();
    ndsTaskmanSampleGraphicsHeap();
    gSYTaskmanGraphicsHeap.ptr = saved_graphics_heap_ptr;
    return handled;
}
