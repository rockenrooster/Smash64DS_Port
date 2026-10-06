
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

#if NDS_RENDERER_HW_TRIANGLES
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
#endif

#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_YAMABUKI && NDS_P2_ITEM_CORE
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

#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE
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
#endif

#if NDS_RENDERER_HW_TRIANGLES
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
#if NDS_RENDERER_PROFILE_LEVEL >= 2
static u32 sNdsRendererAdapterStageOwnerOccurrence;
static u32 sNdsRendererAdapterStageNextOccurrence;
static u32 sNdsRendererAdapterStageListOrdinal;
#endif

#if NDS_RENDERER_PROFILE_LEVEL >= 2
#define NDS_RENDERER_OWNER_HASH_SEED 2166136261u

typedef struct NDSRendererOwnerStatsSnapshot
{
    u32 vertex_command_count;
    u32 source_vertex_count;
    u32 triangle_command_count;
    u32 triangle_count;
    u32 matrix_command_count;
} NDSRendererOwnerStatsSnapshot;

static u32 ndsRendererOwnerHashBytes(u32 hash, const void *data,
                                     size_t bytes)
{
    const u8 *cursor = data;
    size_t i;

    if (hash == 0u)
    {
        hash = NDS_RENDERER_OWNER_HASH_SEED;
    }
    for (i = 0u; i < bytes; i++)
    {
        hash ^= cursor[i];
        hash *= 16777619u;
    }
    /* Zero is the public "not started" sentinel for the compact ledgers.
     * Keep an intermediate hash from ever aliasing that sentinel. */
    if (hash == 0u)
    {
        hash = 1u;
    }
    return hash;
}

static u32 ndsRendererOwnerHashU32(u32 hash, u32 value)
{
    return ndsRendererOwnerHashBytes(hash, &value, sizeof(value));
}

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static u32 ndsRendererOwnerRootBranchPath(
    const NDSRelocLoadedFile *loaded, const Gfx *dl, u32 selected_event)
{
    u32 hash = 0u;

    hash = ndsRendererOwnerHashU32(hash, 0x524f4f54u);
    if ((loaded != NULL) && ((uintptr_t)dl >= (uintptr_t)loaded->data) &&
        ((uintptr_t)dl <
         ((uintptr_t)loaded->data + loaded->data_size)))
    {
        hash = ndsRendererOwnerHashU32(hash, 1u);
        hash = ndsRendererOwnerHashU32(hash, loaded->asset_id);
        hash = ndsRendererOwnerHashU32(hash, loaded->owner_generation);
        hash = ndsRendererOwnerHashU32(
            hash, (u32)((uintptr_t)dl - (uintptr_t)loaded->data));
    }
    else if ((gSYTaskmanGraphicsHeap.start != NULL) &&
             (gSYTaskmanGraphicsHeap.end != NULL) &&
             ((uintptr_t)dl >=
              (uintptr_t)gSYTaskmanGraphicsHeap.start) &&
             ((uintptr_t)dl <
              (uintptr_t)gSYTaskmanGraphicsHeap.end))
    {
        hash = ndsRendererOwnerHashU32(hash, 2u);
        hash = ndsRendererOwnerHashU32(
            hash, (u32)((uintptr_t)dl -
                        (uintptr_t)gSYTaskmanGraphicsHeap.start));
    }
    else
    {
        /* Valid roots are reloc- or taskman-backed. Preserve a stable
         * segmented source token for any future resolver-backed root without
         * hashing its process address. */
        hash = ndsRendererOwnerHashU32(hash, 3u);
        hash = ndsRendererOwnerHashU32(
            hash, (u32)((uintptr_t)dl & 0x00ffffffu));
    }
    hash = ndsRendererOwnerHashU32(hash, selected_event);
    return hash;
}
#endif

#define NDS_RENDERER_OWNER_POINTER_NULL 0u
#define NDS_RENDERER_OWNER_POINTER_EMPTY_SEGMENT 1u
#define NDS_RENDERER_OWNER_POINTER_RELOC 2u
#define NDS_RENDERER_OWNER_POINTER_TASKMAN 3u
#define NDS_RENDERER_OWNER_POINTER_GRAPHICS_HEAP 4u
#define NDS_RENDERER_OWNER_POINTER_SEGMENTED 5u
#define NDS_RENDERER_OWNER_POINTER_RAW 6u

static u32 ndsRendererOwnerHashStablePointer(u32 hash, uintptr_t value)
{
    const NDSRelocLoadedFile *loaded;
    const u8 *arena = ndsTaskmanArenaStart();
    uintptr_t arena_base = (uintptr_t)arena;
    size_t arena_size = ndsTaskmanArenaSize();
    u32 segment = (u32)(value >> 24);

    hash = ndsRendererOwnerHashU32(hash, 0x50545231u);
    if (value == 0u)
    {
        return ndsRendererOwnerHashU32(
            hash, NDS_RENDERER_OWNER_POINTER_NULL);
    }
    if (value == (uintptr_t)sNdsRendererAdapterEmptySegmentEDL)
    {
        return ndsRendererOwnerHashU32(
            hash, NDS_RENDERER_OWNER_POINTER_EMPTY_SEGMENT);
    }

    loaded = ndsRelocFindLoadedFileContaining(
        (const void *)value, 1u);
    if (loaded != NULL)
    {
        hash = ndsRendererOwnerHashU32(
            hash, NDS_RENDERER_OWNER_POINTER_RELOC);
        hash = ndsRendererOwnerHashU32(hash, loaded->asset_id);
        hash = ndsRendererOwnerHashU32(
            hash, loaded->owner_generation);
        return ndsRendererOwnerHashU32(
            hash, (u32)(value - (uintptr_t)loaded->data));
    }
    if ((arena != NULL) && (value >= arena_base) &&
        ((size_t)(value - arena_base) < arena_size))
    {
        hash = ndsRendererOwnerHashU32(
            hash, NDS_RENDERER_OWNER_POINTER_TASKMAN);
        return ndsRendererOwnerHashU32(
            hash, (u32)(value - arena_base));
    }
    if ((gSYTaskmanGraphicsHeap.start != NULL) &&
        (gSYTaskmanGraphicsHeap.end != NULL) &&
        (value >= (uintptr_t)gSYTaskmanGraphicsHeap.start) &&
        (value < (uintptr_t)gSYTaskmanGraphicsHeap.end))
    {
        hash = ndsRendererOwnerHashU32(
            hash, NDS_RENDERER_OWNER_POINTER_GRAPHICS_HEAP);
        return ndsRendererOwnerHashU32(
            hash, (u32)(value -
                        (uintptr_t)gSYTaskmanGraphicsHeap.start));
    }
    if ((segment != 0u) && (segment <= 0x0fu))
    {
        hash = ndsRendererOwnerHashU32(
            hash, NDS_RENDERER_OWNER_POINTER_SEGMENTED);
        hash = ndsRendererOwnerHashU32(hash, segment);
        return ndsRendererOwnerHashU32(
            hash, (u32)(value & 0x00ffffffu));
    }

    /* Valid renderer operands are reloc-, taskman-, or segment-backed. Keep
     * an explicit raw fallback so an unexpected operand mutation is still
     * visible instead of silently aliasing the null provenance. */
    hash = ndsRendererOwnerHashU32(
        hash, NDS_RENDERER_OWNER_POINTER_RAW);
    return ndsRendererOwnerHashU32(hash, (u32)value);
}

static s32 ndsRendererOwnerCommandUsesPointer(u32 op)
{
    return ((op == NDS_FIGHTER_DL_OP_VTX) ||
            (op == NDS_FIGHTER_DL_OP_MTX) ||
            (op == 0xdcu) || /* F3DEX2 G_MOVEMEM */
            (op == NDS_FIGHTER_DL_OP_DL) ||
            (op == NDS_FIGHTER_DL_OP_SETTIMG) ||
            (op == 0xfeu) || /* G_SETZIMG */
            (op == NDS_FIGHTER_DL_OP_SETCIMG)) ? TRUE : FALSE;
}

static u32 ndsRendererOwnerHashDisplayList(
    u32 hash, const Gfx *dl, const NDSRendererConfig *config,
    u32 depth, u32 *remaining_commands)
{
    u32 i;

    hash = ndsRendererOwnerHashU32(hash, 0x4c495354u);
    hash = ndsRendererOwnerHashStablePointer(
        hash, (uintptr_t)dl);
    hash = ndsRendererOwnerHashU32(hash, depth);
    if ((dl == NULL) || (config == NULL) ||
        (remaining_commands == NULL))
    {
        return ndsRendererOwnerHashU32(hash, 0xffffffffu);
    }
    if (depth > config->max_depth)
    {
        return ndsRendererOwnerHashU32(hash, 0xfffffffeu);
    }

    for (i = 0u; i < config->max_list_commands; i++, dl++)
    {
        u32 w0;
        u32 w1;
        u32 op;

        if (*remaining_commands == 0u)
        {
            return ndsRendererOwnerHashU32(hash, 0xfffffffdu);
        }
        if ((config->validate_range != NULL) &&
            (config->validate_range(dl, sizeof(*dl), config->user) == FALSE))
        {
            hash = ndsRendererOwnerHashStablePointer(
                hash, (uintptr_t)dl);
            return ndsRendererOwnerHashU32(hash, 0xfffffffcu);
        }

        w0 = dl->words.w0;
        w1 = dl->words.w1;
        op = w0 >> 24;
        (*remaining_commands)--;
        hash = ndsRendererOwnerHashU32(hash, 0x434d4431u);
        hash = ndsRendererOwnerHashU32(hash, i);
        hash = ndsRendererOwnerHashU32(hash, w0);
        if (ndsRendererOwnerCommandUsesPointer(op) != FALSE)
        {
            hash = ndsRendererOwnerHashStablePointer(
                hash, (uintptr_t)w1);
        }
        else
        {
            hash = ndsRendererOwnerHashU32(hash, w1);
        }

        if (op == NDS_FIGHTER_DL_OP_DL)
        {
            const Gfx *branch = (const Gfx *)(uintptr_t)w1;
            u32 resolve_kind = NDS_RENDERER_RESOLVE_NONE;
            u32 branch_is_jump =
                ((w0 & (1u << 16)) != 0u) ? TRUE : FALSE;

            if (config->resolve_branch != NULL)
            {
                branch = config->resolve_branch(
                    branch, &resolve_kind, config->user);
            }
            hash = ndsRendererOwnerHashU32(hash, 0x4252414eu);
            hash = ndsRendererOwnerHashU32(hash, resolve_kind);
            hash = ndsRendererOwnerHashU32(hash, branch_is_jump);
            hash = ndsRendererOwnerHashDisplayList(
                hash, branch, config, depth + 1u, remaining_commands);
            if (branch_is_jump != FALSE)
            {
                return hash;
            }
        }
        else if (op == NDS_FIGHTER_DL_OP_ENDDL)
        {
            return ndsRendererOwnerHashU32(hash, 0x454e444cu);
        }
    }
    return ndsRendererOwnerHashU32(hash, 0x4e4f454eu);
}

static u32 ndsRendererOwnerHashTileState(
    u32 hash, const NDSRendererTileState *tile)
{
#define NDS_RENDERER_HASH_TILE_FIELD(field) \
    hash = ndsRendererOwnerHashU32(hash, tile->field)

    NDS_RENDERER_HASH_TILE_FIELD(set_seen);
    NDS_RENDERER_HASH_TILE_FIELD(size_seen);
    NDS_RENDERER_HASH_TILE_FIELD(format);
    NDS_RENDERER_HASH_TILE_FIELD(size);
    NDS_RENDERER_HASH_TILE_FIELD(line);
    NDS_RENDERER_HASH_TILE_FIELD(tmem);
    NDS_RENDERER_HASH_TILE_FIELD(palette);
    NDS_RENDERER_HASH_TILE_FIELD(cms);
    NDS_RENDERER_HASH_TILE_FIELD(cmt);
    NDS_RENDERER_HASH_TILE_FIELD(masks);
    NDS_RENDERER_HASH_TILE_FIELD(maskt);
    NDS_RENDERER_HASH_TILE_FIELD(shifts);
    NDS_RENDERER_HASH_TILE_FIELD(shiftt);
    NDS_RENDERER_HASH_TILE_FIELD(uls);
    NDS_RENDERER_HASH_TILE_FIELD(ult);
    NDS_RENDERER_HASH_TILE_FIELD(lrs);
    NDS_RENDERER_HASH_TILE_FIELD(lrt);
    NDS_RENDERER_HASH_TILE_FIELD(width);
    NDS_RENDERER_HASH_TILE_FIELD(height);
    NDS_RENDERER_HASH_TILE_FIELD(flags);

#undef NDS_RENDERER_HASH_TILE_FIELD
    return hash;
}

static u32 ndsRendererOwnerHashTextureLoadState(
    u32 hash, const NDSRendererTextureLoadState *load)
{
    hash = ndsRendererOwnerHashU32(hash, load->image);
    hash = ndsRendererOwnerHashU32(hash, load->sequence);
    hash = ndsRendererOwnerHashU32(hash, load->image_width);
    hash = ndsRendererOwnerHashU32(hash, load->load_uls);
    hash = ndsRendererOwnerHashU32(hash, load->load_ult);
    hash = ndsRendererOwnerHashU32(hash, load->load_lrs);
    hash = ndsRendererOwnerHashU32(hash, load->load_dxt);
    hash = ndsRendererOwnerHashU32(hash, load->load_texels);
    hash = ndsRendererOwnerHashU32(hash, load->load_tmem);
    hash = ndsRendererOwnerHashU32(hash, load->valid);
    hash = ndsRendererOwnerHashU32(hash, load->image_format);
    hash = ndsRendererOwnerHashU32(hash, load->image_size);
    hash = ndsRendererOwnerHashU32(hash, load->load_kind);
    hash = ndsRendererOwnerHashU32(hash, load->load_tile);
    return hash;
}

static u32 ndsRendererOwnerHashRuntimeState(const NDSRendererStats *stats)
{
    u32 hash = 0u;
    u32 i;

    if (stats == NULL)
    {
        return 0u;
    }

    /* Serialize exactly the persistent renderer contract copied by
     * ndsFighterDLDrawCopyPersistentRendererState().  Do not hash the raw
     * tail: it interleaves proof counters and pointer-bearing diagnostics,
     * and struct padding is not semantic state. */
#define NDS_RENDERER_HASH_STATE_FIELD(field) \
    hash = ndsRendererOwnerHashU32(hash, (u32)stats->field)

    NDS_RENDERER_HASH_STATE_FIELD(othermode_h);
    NDS_RENDERER_HASH_STATE_FIELD(othermode_l);
    NDS_RENDERER_HASH_STATE_FIELD(geometry_mode);
    NDS_RENDERER_HASH_STATE_FIELD(geometry_clear_mask);
    NDS_RENDERER_HASH_STATE_FIELD(geometry_set_mask);
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_kind);
    NDS_RENDERER_HASH_STATE_FIELD(texture_scale_s);
    NDS_RENDERER_HASH_STATE_FIELD(texture_scale_t);
    NDS_RENDERER_HASH_STATE_FIELD(texture_level);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile);
    NDS_RENDERER_HASH_STATE_FIELD(texture_on);
    NDS_RENDERER_HASH_STATE_FIELD(texture_xparam);
    NDS_RENDERER_HASH_STATE_FIELD(texture_state_flags);
    NDS_RENDERER_HASH_STATE_FIELD(texture_image);
    NDS_RENDERER_HASH_STATE_FIELD(texture_format);
    NDS_RENDERER_HASH_STATE_FIELD(texture_size);
    NDS_RENDERER_HASH_STATE_FIELD(texture_image_width);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tlut_image);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tlut_count);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tlut_tile);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_format);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_size);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_line);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_tmem);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_palette);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_cms);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_cmt);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_masks);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_maskt);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_shifts);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_shiftt);
    NDS_RENDERER_HASH_STATE_FIELD(texture_render_tile_flags);
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_tile);
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_block_uls);
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_block_ult);
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_block_lrs);
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_block_dxt);
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_texels);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile_size_tile);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile_size_uls);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile_size_ult);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile_size_lrs);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile_size_lrt);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile_width);
    NDS_RENDERER_HASH_STATE_FIELD(texture_tile_height);
    for (i = 0u; i < NDS_RENDERER_TILE_COUNT; i++)
    {
        hash = ndsRendererOwnerHashTileState(hash,
                                              &stats->texture_tiles[i]);
    }
    NDS_RENDERER_HASH_STATE_FIELD(texture_load_sequence);
    for (i = 0u; i < NDS_RENDERER_TEXTURE_LOAD_HISTORY_COUNT; i++)
    {
        hash = ndsRendererOwnerHashTextureLoadState(
            hash, &stats->texture_loads[i]);
    }
    NDS_RENDERER_HASH_STATE_FIELD(texture_combine_w0);
    NDS_RENDERER_HASH_STATE_FIELD(texture_combine_w1);
    NDS_RENDERER_HASH_STATE_FIELD(texture_combine_count);
    NDS_RENDERER_HASH_STATE_FIELD(prim_color);
    NDS_RENDERER_HASH_STATE_FIELD(prim_min_level);
    NDS_RENDERER_HASH_STATE_FIELD(prim_lod_fraction);
    NDS_RENDERER_HASH_STATE_FIELD(env_color);
    NDS_RENDERER_HASH_STATE_FIELD(blend_color);
    NDS_RENDERER_HASH_STATE_FIELD(light_color_1);
    NDS_RENDERER_HASH_STATE_FIELD(light_color_2);
    NDS_RENDERER_HASH_STATE_FIELD(light_color_mask);
    NDS_RENDERER_HASH_STATE_FIELD(light_dir_x);
    NDS_RENDERER_HASH_STATE_FIELD(light_dir_y);
    NDS_RENDERER_HASH_STATE_FIELD(light_dir_z);
    NDS_RENDERER_HASH_STATE_FIELD(light_dir_mask);
    NDS_RENDERER_HASH_STATE_FIELD(prim_depth);
    NDS_RENDERER_HASH_STATE_FIELD(prim_depth_delta);
    NDS_RENDERER_HASH_STATE_FIELD(fog_color);
    NDS_RENDERER_HASH_STATE_FIELD(fog_min);
    NDS_RENDERER_HASH_STATE_FIELD(fog_max);
    NDS_RENDERER_HASH_STATE_FIELD(fog_status);
    NDS_RENDERER_HASH_STATE_FIELD(texture_source_hash1);
    NDS_RENDERER_HASH_STATE_FIELD(texture_source_hash2);

#undef NDS_RENDERER_HASH_STATE_FIELD
    return hash;
}

static u32 ndsRendererOwnerHashVertexCache(
    const NDSRendererVertexCache *cache)
{
    u32 hash = 0u;
    u32 i;
    u32 row;
    u32 col;
    u32 snapshot_count;
    u32 input_mask;
    u32 transformed_mask;
    u32 color_mask;

    if (cache == NULL)
    {
        return 0u;
    }
    input_mask = cache->input_valid_mask;
    transformed_mask = cache->transformed_valid_mask & input_mask;
    color_mask = cache->vertex_color_valid_mask & input_mask;
    hash = ndsRendererOwnerHashU32(hash, input_mask);
    hash = ndsRendererOwnerHashU32(
        hash, cache->raw_vertex_fit_mask & input_mask);
    hash = ndsRendererOwnerHashU32(hash, transformed_mask);
    hash = ndsRendererOwnerHashU32(hash, color_mask);
    snapshot_count = cache->matrix_snapshot_count;
    if (snapshot_count > NDS_RENDERER_MATRIX_SNAPSHOT_CAPACITY)
    {
        snapshot_count = NDS_RENDERER_MATRIX_SNAPSHOT_CAPACITY;
    }
    hash = ndsRendererOwnerHashU32(hash, snapshot_count);
    for (i = 0u; i < NDS_RENDERER_VERTEX_CACHE_SIZE; i++)
    {
        u32 bit = 1u << i;

        if ((input_mask & bit) != 0u)
        {
            const NDSRendererInputVertex *input = &cache->input_vertices[i];

            hash = ndsRendererOwnerHashU32(hash, (u32)(s32)input->x);
            hash = ndsRendererOwnerHashU32(hash, (u32)(s32)input->y);
            hash = ndsRendererOwnerHashU32(hash, (u32)(s32)input->z);
            hash = ndsRendererOwnerHashU32(hash, (u32)(s32)input->s);
            hash = ndsRendererOwnerHashU32(hash, (u32)(s32)input->t);
            hash = ndsRendererOwnerHashU32(hash, input->r);
            hash = ndsRendererOwnerHashU32(hash, input->g);
            hash = ndsRendererOwnerHashU32(hash, input->b);
            hash = ndsRendererOwnerHashU32(hash, input->a);
            hash = ndsRendererOwnerHashU32(
                hash, cache->vertex_matrix_snapshot[i]);
            hash = ndsRendererOwnerHashU32(
                hash, cache->vertex_clip_snapshot[i]);
        }
        if ((transformed_mask & bit) != 0u)
        {
            const NDSRendererClipVertex20p12 *clip =
                &cache->transformed_vertices[i];

            hash = ndsRendererOwnerHashU32(hash, (u32)clip->x);
            hash = ndsRendererOwnerHashU32(hash, (u32)clip->y);
            hash = ndsRendererOwnerHashU32(hash, (u32)clip->z);
            hash = ndsRendererOwnerHashU32(hash, (u32)clip->w);
        }
        if ((color_mask & bit) != 0u)
        {
            hash = ndsRendererOwnerHashU32(hash,
                                           cache->vertex_colors[i]);
        }
    }
    for (i = 0u; i < snapshot_count; i++)
    {
        const NDSRendererMatrixSnapshot *snapshot =
            &cache->matrix_snapshots[i];

        for (row = 0u; row < 4u; row++)
        {
            for (col = 0u; col < 4u; col++)
            {
                hash = ndsRendererOwnerHashU32(
                    hash, (u32)snapshot->matrix.m[row][col]);
            }
        }
        hash = ndsRendererOwnerHashU32(hash, snapshot->generation);
        hash = ndsRendererOwnerHashU32(hash, snapshot->signature);
    }
    return hash;
}

static u32 ndsRendererOwnerHashResolver(
    const NDSFighterDLDrawState *state)
{
    u32 hash = 0u;
    uintptr_t base;
    uintptr_t end;
    size_t bytes;
    size_t i;

    if (state == NULL)
    {
        return 0u;
    }
    if (state->primary_file != NULL)
    {
        hash = ndsRendererOwnerHashU32(hash, 1u);
        hash = ndsRendererOwnerHashU32(
            hash, state->primary_file->asset_id);
        hash = ndsRendererOwnerHashU32(
            hash, state->primary_file->owner_generation);
        hash = ndsRendererOwnerHashU32(
            hash, state->primary_file->data_size);
    }
    else
    {
        hash = ndsRendererOwnerHashU32(hash, 0u);
    }

    base = (uintptr_t)state->segment_e_base;
    end = (uintptr_t)state->segment_e_end;
    if ((base == 0u) || (end <= base))
    {
        return ndsRendererOwnerHashU32(hash, 0u);
    }
    bytes = (size_t)(end - base);
    if (((bytes % sizeof(Gfx)) != 0u) ||
        (ndsFighterDLScanRangeInTaskmanArena(
             state->segment_e_base, bytes) == FALSE))
    {
        hash = ndsRendererOwnerHashU32(hash, 0xffffffffu);
        return ndsRendererOwnerHashU32(hash, (u32)bytes);
    }

    hash = ndsRendererOwnerHashU32(hash, (u32)(bytes / sizeof(Gfx)));
    for (i = 0u; i < (bytes / sizeof(Gfx)); i++)
    {
        const Gfx *command = &state->segment_e_base[i];
        u32 w0 = command->words.w0;
        u32 w1 = command->words.w1;
        u32 op = w0 >> 24;

        hash = ndsRendererOwnerHashU32(
            hash, w0);
        if ((op == NDS_FIGHTER_DL_OP_DL) ||
            (op == NDS_FIGHTER_DL_OP_VTX) ||
            (op == NDS_FIGHTER_DL_OP_MTX) ||
            (op == 0xdcu) || /* F3DEX2 G_MOVEMEM */
            (op == NDS_FIGHTER_DL_OP_SETTIMG))
        {
            const void *pointer = (const void *)(uintptr_t)w1;
            const NDSRelocLoadedFile *loaded = NULL;
            uintptr_t pointer_value = (uintptr_t)pointer;

            if ((pointer_value >= base) && (pointer_value < end))
            {
                hash = ndsRendererOwnerHashU32(hash, 1u);
                hash = ndsRendererOwnerHashU32(
                    hash, (u32)(pointer_value - base));
                continue;
            }
            loaded = ndsRelocFindLoadedFileContaining(pointer, 1u);
            if (loaded != NULL)
            {
                hash = ndsRendererOwnerHashU32(hash, 2u);
                hash = ndsRendererOwnerHashU32(hash, loaded->asset_id);
                hash = ndsRendererOwnerHashU32(
                    hash, loaded->owner_generation);
                hash = ndsRendererOwnerHashU32(
                    hash, (u32)(pointer_value -
                                (uintptr_t)loaded->data));
                continue;
            }
            if ((gSYTaskmanGraphicsHeap.start != NULL) &&
                (gSYTaskmanGraphicsHeap.end != NULL) &&
                (pointer_value >=
                 (uintptr_t)gSYTaskmanGraphicsHeap.start) &&
                (pointer_value <
                 (uintptr_t)gSYTaskmanGraphicsHeap.end))
            {
                hash = ndsRendererOwnerHashU32(hash, 3u);
                hash = ndsRendererOwnerHashU32(
                    hash, (u32)(pointer_value -
                                (uintptr_t)gSYTaskmanGraphicsHeap.start));
                continue;
            }
            hash = ndsRendererOwnerHashU32(hash, 4u);
        }
        hash = ndsRendererOwnerHashU32(hash, w1);
    }
    return hash;
}


static void ndsRendererOwnerSnapshotStats(
    const NDSRendererStats *stats, NDSRendererOwnerStatsSnapshot *snapshot)
{
    snapshot->vertex_command_count = stats->vertex_command_count;
    snapshot->source_vertex_count = stats->source_vertex_count;
    snapshot->triangle_command_count = stats->triangle_command_count;
    snapshot->triangle_count = stats->triangle_count;
    snapshot->matrix_command_count = stats->matrix_command_count;
}

static void ndsRendererOwnerAccumulateList(
    NDSRendererProfileOwner owner_id,
    const NDSRelocLoadedFile *loaded,
    const Gfx *dl,
    u32 selected_event,
    const NDSRendererMatrix20p12 *projection,
    const NDSRendererMatrix20p12 *modelview,
    const NDSRendererConfig *config,
    const NDSRendererOwnerStatsSnapshot *before,
    const NDSRendererStats *after)
{
    volatile NDSRendererOwnerProfile *owner;
    u32 dl_offset;
    u32 remaining_commands;

    if ((u32)owner_id >= (u32)NDS_RENDERER_PROFILE_OWNER_COUNT)
    {
        return;
    }
    owner = &gNdsRendererProfileOwners[(u32)owner_id];
    owner->selected_count++;
    owner->source_command_count += after->command_count;
    owner->vertex_command_count +=
        after->vertex_command_count - before->vertex_command_count;
    owner->source_vertex_count +=
        after->source_vertex_count - before->source_vertex_count;
    owner->triangle_command_count +=
        after->triangle_command_count - before->triangle_command_count;
    owner->triangle_count += after->triangle_count - before->triangle_count;
    /* Every selected list binds its live camera/DObj matrix pair before the
     * source stream runs.  Source MTX commands, when present, are additional
     * changes rather than the whole owner-level matrix census. */
    owner->matrix_change_count += 1u +
        after->matrix_command_count - before->matrix_command_count;

    if ((loaded != NULL) && ((uintptr_t)dl >= (uintptr_t)loaded->data) &&
        ((uintptr_t)dl <
         ((uintptr_t)loaded->data + loaded->data_size)))
    {
        dl_offset = (u32)((uintptr_t)dl - (uintptr_t)loaded->data);
    }
    else if (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(*dl)) != FALSE)
    {
        dl_offset = (u32)((uintptr_t)dl -
                          (uintptr_t)ndsTaskmanArenaStart());
    }
    else
    {
        dl_offset = (u32)((uintptr_t)dl & 0x00ffffffu);
    }
    owner->topology_signature = ndsRendererOwnerHashU32(
        owner->topology_signature, 0x4f574e31u);
    owner->topology_signature = ndsRendererOwnerHashStablePointer(
        owner->topology_signature, (uintptr_t)dl);
    owner->topology_signature = ndsRendererOwnerHashU32(
        owner->topology_signature, after->command_count);
    remaining_commands = (config != NULL) ? config->max_commands : 0u;
    owner->topology_signature = ndsRendererOwnerHashDisplayList(
        owner->topology_signature, dl, config, 0u,
        &remaining_commands);
    owner->topology_signature = ndsRendererOwnerHashU32(
        owner->topology_signature, remaining_commands);
    owner->selected_event_signature = ndsRendererOwnerHashU32(
        owner->selected_event_signature, selected_event);
    owner->selected_event_signature = ndsRendererOwnerHashU32(
        owner->selected_event_signature, dl_offset);
    if (projection != NULL)
    {
        owner->camera_signature = ndsRendererOwnerHashBytes(
            owner->camera_signature, projection, sizeof(*projection));
    }
    if (modelview != NULL)
    {
        owner->dobj_matrix_signature = ndsRendererOwnerHashBytes(
            owner->dobj_matrix_signature, modelview, sizeof(*modelview));
    }
    owner->material_signature = ndsRendererOwnerHashU32(
        owner->material_signature, after->prim_color);
    owner->material_signature = ndsRendererOwnerHashU32(
        owner->material_signature, after->env_color);
    owner->material_signature = ndsRendererOwnerHashU32(
        owner->material_signature, after->blend_color);
    owner->material_signature = ndsRendererOwnerHashU32(
        owner->material_signature, after->texture_combine_w0);
    owner->material_signature = ndsRendererOwnerHashU32(
        owner->material_signature, after->texture_combine_w1);
    owner->light_signature = ndsRendererOwnerHashU32(
        owner->light_signature, after->light_color_1);
    owner->light_signature = ndsRendererOwnerHashU32(
        owner->light_signature, after->light_color_2);
    owner->light_signature = ndsRendererOwnerHashU32(
        owner->light_signature, (u32)after->light_dir_x);
    owner->light_signature = ndsRendererOwnerHashU32(
        owner->light_signature, (u32)after->light_dir_y);
    owner->light_signature = ndsRendererOwnerHashU32(
        owner->light_signature, (u32)after->light_dir_z);
    owner->texture_signature = ndsRendererOwnerHashU32(
        owner->texture_signature, after->texture_image);
    owner->texture_signature = ndsRendererOwnerHashU32(
        owner->texture_signature, after->texture_tlut_image);
    owner->texture_signature = ndsRendererOwnerHashU32(
        owner->texture_signature, after->texture_scale_s);
    owner->texture_signature = ndsRendererOwnerHashU32(
        owner->texture_signature, after->texture_scale_t);
    owner->texture_signature = ndsRendererOwnerHashU32(
        owner->texture_signature, after->texture_render_tile);
}
#endif

#if NDS_RENDERER_PROFILE_LEVEL >= 2
static void ndsRendererAdapterAccumulateDepth(
    const NDSRendererStats *stats,
    volatile u32 *samples,
    volatile s32 *depth_min,
    volatile s32 *depth_max,
    volatile s32 *w_min,
    volatile s32 *w_max)
{
    if ((stats == NULL) || (samples == NULL) || (depth_min == NULL) ||
        (depth_max == NULL) || (w_min == NULL) || (w_max == NULL) ||
        (stats->hardware_projected_depth_sample_count == 0u))
    {
        return;
    }
    if (*samples == 0u)
    {
        *depth_min = stats->hardware_projected_depth_min;
        *depth_max = stats->hardware_projected_depth_max;
        *w_min = stats->hardware_projected_w_min;
        *w_max = stats->hardware_projected_w_max;
    }
    else
    {
        if (stats->hardware_projected_depth_min < *depth_min)
        {
            *depth_min = stats->hardware_projected_depth_min;
        }
        if (stats->hardware_projected_depth_max > *depth_max)
        {
            *depth_max = stats->hardware_projected_depth_max;
        }
        if (stats->hardware_projected_w_min < *w_min)
        {
            *w_min = stats->hardware_projected_w_min;
        }
        if (stats->hardware_projected_w_max > *w_max)
        {
            *w_max = stats->hardware_projected_w_max;
        }
    }
    *samples += stats->hardware_projected_depth_sample_count;
}
#endif

void ndsRendererAdapterResetDepthDiagnostics(void)
{
    gNdsRendererDepthStageSamples = 0u;
    gNdsRendererDepthFighterP0Samples = 0u;
    gNdsRendererDepthFighterP1Samples = 0u;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererAdapterStageOwnerOccurrence = 0u;
    sNdsRendererAdapterStageNextOccurrence = 0u;
    sNdsRendererAdapterStageListOrdinal = 0u;
#endif
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
#if NDS_RENDERER_HW_TRIANGLES
    ndsRendererProfileSetOwner(NDS_RENDERER_PROFILE_OWNER_STAGE);
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    sNdsRendererAdapterStageOwnerOccurrence =
        sNdsRendererAdapterStageNextOccurrence++;
    sNdsRendererAdapterStageListOrdinal = 0u;
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].entry_state_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].entry_state_hash,
            ndsRendererOwnerHashRuntimeState(
                &sNdsRendererAdapterStagePersistentStats));
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].entry_vertex_cache_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].entry_vertex_cache_hash,
            ndsRendererOwnerHashVertexCache(
                &sNdsRendererAdapterStageVertexCache));
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].entry_resolver_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].entry_resolver_hash,
            ndsRendererOwnerHashResolver(
                &sNdsRendererAdapterStagePersistentState));
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].entry_global_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].entry_global_hash,
            ndsRendererProfileGlobalStateHash());
#endif
}

void ndsRendererAdapterEndStageTraversal(void)
{
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].exit_state_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].exit_state_hash,
            ndsRendererOwnerHashRuntimeState(
                &sNdsRendererAdapterStagePersistentStats));
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].exit_vertex_cache_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].exit_vertex_cache_hash,
            ndsRendererOwnerHashVertexCache(
                &sNdsRendererAdapterStageVertexCache));
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].exit_resolver_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].exit_resolver_hash,
            ndsRendererOwnerHashResolver(
                &sNdsRendererAdapterStagePersistentState));
    gNdsRendererProfileOwners[
        NDS_RENDERER_PROFILE_OWNER_STAGE].exit_global_hash =
        ndsRendererOwnerHashU32(
            gNdsRendererProfileOwners[
                NDS_RENDERER_PROFILE_OWNER_STAGE].exit_global_hash,
            ndsRendererProfileGlobalStateHash());
#endif
#if NDS_RENDERER_HW_TRIANGLES
    ndsRendererProfileSetOwner(NDS_RENDERER_PROFILE_OWNER_NONE);
#endif
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

#if NDS_RENDERER_HW_TRIANGLES
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
#define NDS_P2_NDL_OWNER_NONE 0u
#define NDS_P2_NDL_OWNER_IMPACT_WAVE 1u
#define NDS_P2_NDL_OWNER_DAMAGE_SLASH 2u
#define NDS_P2_NDL_OWNER_EFGROUND 3u
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

__attribute__((section(".data"))) volatile u32 gNdsP2Ndl = 1u;
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

#if NDS_R2_IMPACT_WAVE_NATIVE
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
#endif
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
    kind = ndsStageGCDrawAllLoopNdlEfGroundKind(gobj);
    if (kind < NDS_P2_NDL_KIND_COUNT)
    {
        record->owner = NDS_P2_NDL_OWNER_EFGROUND;
        record->kind = (u8)kind;
        record->dobj[0] = DObjGetStruct(gobj);
        record->body_count = 1u;
        NDS_DIAG(gNdsNdlBindCount++);
        return;
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
#if NDS_R2_IMPACT_WAVE_NATIVE
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
#else
    (void)gobj;
    (void)camera_gobj;
    (void)record;
    (void)stats;
    return FALSE;
#endif
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

    if ((gNdsP2Ndl == 0u) || (gNdsSceneManagerCurrIsBattle == 0u) ||
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
    if (record->owner == NDS_P2_NDL_OWNER_EFGROUND)
    {
        handled = ndsStageGCDrawAllLoopSubmitNdlEfGround(
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
#if NDS_TASK49_GX_DIFFER
    /* Task49 needs the M1 stream separated from stage GX.  This is diagnostic
     * attribution only; production builds retain their existing owner state. */
    ndsRendererProfileSetOwner(NDS_RENDERER_PROFILE_OWNER_EFFECT);
#endif
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
#endif

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
/* P2-4n1 step 6: every per-segment fact below reads the active stage's
 * capture row (renderer_adapter_matrix.c). Dream Land's rows are the switch,
 * arrays and ternary that used to live here, value for value. */
#if NDS_P2_STAGE_ZEBES
extern void *ndsGRZebesAcidGObj(void);
#endif
#if NDS_P2_STAGE_YAMABUKI
extern void *ndsGRYamabukiGateGObj(void);
#endif
#if NDS_P2_STAGE_ZEBES || NDS_P2_STAGE_YAMABUKI
extern void gcDrawDObjTreeDLLinksForGObj(GObj *gobj);
#endif
#if NDS_P2_STAGE_INISHIE
extern void *ndsGRInishieScaleStringGObj(u32 index);
extern void *ndsGRInishieScalePlatformGObj(u32 index);
#endif

static GObj *ndsRendererAdapterNativeStageSegmentGObj(u32 segment_index)
{
    const NDSRendererAdapterNativeStageCaptureSegment *row =
        ndsRendererAdapterNativeStageCaptureRow(segment_index);

    if (row == NULL)
    {
        return NULL;
    }
    switch (row->source)
    {
    case NDS_RENDERER_ADAPTER_STAGE_CAPTURE_LAYER:
        return (row->index < 4u) ? gGRCommonLayerGObjs[row->index] : NULL;
    case NDS_RENDERER_ADAPTER_STAGE_CAPTURE_PUPUPU_MAP:
        return (row->index < 4u) ?
            gGRCommonStruct.pupupu.map_gobj[row->index] : NULL;
#if NDS_P2_STAGE_ZEBES
    case NDS_RENDERER_ADAPTER_STAGE_CAPTURE_ZEBES_ACID:
        return (GObj *)ndsGRZebesAcidGObj();
#endif
#if NDS_P2_STAGE_YAMABUKI
    case NDS_RENDERER_ADAPTER_STAGE_CAPTURE_YAMABUKI_GATE:
        return (GObj *)ndsGRYamabukiGateGObj();
#endif
#if NDS_P2_STAGE_INISHIE
    case NDS_RENDERER_ADAPTER_STAGE_CAPTURE_INISHIE_SCALE_TREE:
        return (GObj *)ndsGRInishieScaleStringGObj(0u);
    case NDS_RENDERER_ADAPTER_STAGE_CAPTURE_INISHIE_SCALE_PLATFORM:
        return (GObj *)ndsGRInishieScalePlatformGObj(row->index);
#endif
    default:
        return NULL;
    }
}

static u32 ndsRendererAdapterNativeStageSegmentLink(u32 segment_index)
{
    const NDSRendererAdapterNativeStageCaptureSegment *row =
        ndsRendererAdapterNativeStageCaptureRow(segment_index);

    return (row != NULL) ? row->link : 0xffu;
}

static sb32 ndsRendererAdapterNativeStageProcMatches(
    u32 segment_index, GObj *gobj)
{
    if ((gobj == NULL) || (gobj->proc_display == NULL))
    {
        return FALSE;
    }
    {
        static void (*const layer_procs[4])(GObj *) = {
            grDisplayLayer0PriProcDisplay, grDisplayLayer1PriProcDisplay,
            grDisplayLayer2PriProcDisplay, grDisplayLayer3PriProcDisplay
        };
        static void (*const layer_procs_sec[4])(GObj *) = {
            grDisplayLayer0SecProcDisplay, grDisplayLayer1SecProcDisplay,
            grDisplayLayer2SecProcDisplay, grDisplayLayer3SecProcDisplay
        };
        const NDSRendererAdapterNativeStageCaptureSegment *row =
            ndsRendererAdapterNativeStageCaptureRow(segment_index);

#if NDS_P2_STAGE_ZEBES
        if ((row != NULL) &&
            (row->source == NDS_RENDERER_ADAPTER_STAGE_CAPTURE_ZEBES_ACID))
        {
            return (gobj->proc_display == gcDrawDObjTreeDLLinksForGObj) ? TRUE : FALSE;
        }
#endif
#if NDS_P2_STAGE_YAMABUKI
        if ((row != NULL) &&
            (row->source == NDS_RENDERER_ADAPTER_STAGE_CAPTURE_YAMABUKI_GATE))
        {
            return (gobj->proc_display == gcDrawDObjTreeDLLinksForGObj) ? TRUE : FALSE;
        }
#endif
#if NDS_P2_STAGE_INISHIE
        if ((row != NULL) &&
            (row->source == NDS_RENDERER_ADAPTER_STAGE_CAPTURE_INISHIE_SCALE_TREE))
        {
            return (gobj->proc_display == gcDrawDObjTreeForGObj) ? TRUE : FALSE;
        }
        if ((row != NULL) &&
            (row->source == NDS_RENDERER_ADAPTER_STAGE_CAPTURE_INISHIE_SCALE_PLATFORM))
        {
            return (gobj->proc_display == gcDrawDObjDLHead0) ? TRUE : FALSE;
        }
#endif
        if ((row == NULL) || (row->layer >= 4u))
        {
            return FALSE;
        }
        return (gobj->proc_display ==
                ((row->dl_links != 0u) ? layer_procs_sec : layer_procs)[row->layer]) ?
            TRUE : FALSE;
    }
}

static sb32 ndsRendererAdapterNativeStageGObjLinked(GObj *target, u32 link)
{
    GObj *gobj;
    u32 guard = 0u;

    if ((target == NULL) || (link >= GC_COMMON_MAX_DLLINKS))
    {
        return FALSE;
    }
    for (gobj = gGCCommonDLLinks[link];
         (gobj != NULL) && (guard < 256u);
         gobj = gobj->dl_link_next, guard++)
    {
        if (gobj == target)
        {
            return TRUE;
        }
    }
    return FALSE;
}

static sb32 ndsRendererAdapterNativeStageLayer0OrderMatches(
    GObj *const *segments)
{
    GObj *gobj;
    u32 next = 0u;
    u32 guard = 0u;
    const u32 layer0 = ndsRendererAdapterNativeStageLayer0Count();

    if (segments == NULL)
    {
        return FALSE;
    }
    /* Planet Zebes captures layer 1 only (its layer 0 has no DObjs), so
     * there is no layer-0 order to check; zero rows used to decline here and
     * that was the whole of its reason-4 reject (2026-09-07). */
    if (layer0 == 0u)
    {
        return TRUE;
    }
    for (gobj = gGCCommonDLLinks[4];
         (gobj != NULL) && (guard < 256u) && (next < layer0);
         gobj = gobj->dl_link_next, guard++)
    {
        if (gobj == segments[next])
        {
            next++;
        }
    }
    return (next == layer0) ? TRUE : FALSE;
}

static sb32 ndsRendererAdapterNativeStageTransformFlags(
    const DObj *dobj, u16 *out_flags)
{
    if ((dobj == NULL) || (out_flags == NULL))
    {
        return FALSE;
    }
    if ((dobj->xobjs_num == 1u) && (dobj->xobjs[0] != NULL) &&
        ((dobj->xobjs[0]->kind == nGCMatrixKindTraRotRpyR) ||
         (dobj->xobjs[0]->kind == nGCMatrixKindTraRotRpyRSca) ||
         (dobj->xobjs[0]->kind == nGCMatrixKindTra)))
    {
        /* These are non-camera descriptor shapes. The matrix builder still
         * executes the actual kind: Yamabuki's animated gate is TraRotRpyR;
         * Zebes acid uses translation only. */
        *out_flags = 0u;
        return TRUE;
    }
    if ((dobj->xobjs_num == 2u) && (dobj->xobjs[0] != NULL) &&
        (dobj->xobjs[1] != NULL) &&
        (dobj->xobjs[0]->kind == nGCMatrixKindTra))
    {
        if (dobj->xobjs[1]->kind == nGCMatrixKind48)
        {
            *out_flags = 2u;
            return TRUE;
        }
        if (dobj->xobjs[1]->kind == nGCMatrixKind46)
        {
            *out_flags = 4u;
            return TRUE;
        }
        if (dobj->xobjs[1]->kind == nGCMatrixKindRecalcRotRpyRSca)
        {
            *out_flags = 8u;
            return TRUE;
        }
    }
    return FALSE;
}

#if NDS_P2_1P_GAME
/* Board the Platforms' platforms (sc1PBonusStageInitPlatforms and
 * sc1PBonusStageUpdatePlatformCount, sc1pbonusstage.c:539/614) are DObj trees
 * from Bonus2Common hung under the board's yakumono DObjs at runtime. They are
 * not packet geometry -- every bonus2 descriptor excludes them -- so the
 * topology skips each one whole, and ndsStageGCDrawAllLoopSubmitForeignSubtrees
 * draws it through the per-DObj path, where generate_nds_native_item_baked.py's
 * roots own its lists. Recognised by the file its DLLink array lives in. */
static sb32 ndsRendererAdapterIsForeignStageSubtree(const DObj *dobj)
{
    const NDSRelocLoadedFile *loaded;

    if ((gSCManagerBattleState == NULL) ||
        (gSCManagerBattleState->gkind < nGRKindBonus2Start) ||
        (gSCManagerBattleState->gkind > nGRKindBonus2End) ||
        (dobj == NULL) || (dobj->dl_link == NULL))
    {
        return FALSE;
    }
    loaded = ndsRelocFindLoadedFileContaining(dobj->dl_link,
                                              sizeof(DObjDLLink));
    return ((loaded != NULL) &&
            (loaded->asset_id == NDS_RELOC_ASSET_BONUS2_COMMON)) ? TRUE
                                                                 : FALSE;
}
#endif

static sb32 ndsRendererAdapterCollectNativeStageDObjs(
    DObj *dobj, u32 owner, u16 parent_index, u8 depth, u32 dl_links,
    NDSRendererAdapterNativeStageWorkspace *workspace)
{
    for (; dobj != NULL; dobj = dobj->sib_next)
    {
        NDSRendererNativeStageDObj *live;
        u32 index;
        u16 transform_flags;

#if NDS_P2_1P_GAME
        if (ndsRendererAdapterIsForeignStageSubtree(dobj) != FALSE)
        {
            continue;
        }
#endif
        /* A hidden DObj is part of the topology like any other: whether it
         * draws is a per-frame fact the binding mask applies
         * (ndsRendererAdapterStageDObjHiddenInTree). Refusing it here refused
         * the whole packet, so a map whose AnimJoints hide DObjs drew nothing
         * -- Kirby's Board the Platforms starts with a rail carrier and three
         * of its six animated rails hidden (owner r71: "started the stage
         * without fully rendering the map structure"; topology step 10, 5 of
         * 17 layer-1 DObjs collected). */
        if ((workspace->dobj_count >= NDS_RENDERER_ADAPTER_STAGE_DOBJ_COUNT) ||
            (depth > 31u) ||
            (ndsRendererAdapterNativeStageTransformFlags(
                 dobj, &transform_flags) == FALSE))
        {
            return FALSE;
        }
        index = workspace->dobj_count++;
        workspace->dobjs[index] = dobj;
        live = &workspace->live_dobjs[index];
        live->identity = dobj;
        live->parent_index = parent_index;
        live->transform_flags = transform_flags;
        live->owner = (u8)owner;
        live->depth = depth;
        live->binding_index = 0xffffu;
        if ((dl_links != 0u) && (dobj->dl_link != NULL))
        {
            /* P2-4n1 step 7: a Sec-callback layer draws DObjDLLink arrays --
             * gcDrawDObjTreeDLLinks walks each DObj's links in array order,
             * stopping at the sentinel list_id, and every link with a list
             * is one display list on one head. That is exactly one binding
             * per link, in that order, which is the order the generator
             * compiled the packet in (binding_dobjs / binding_heads). */
            DObjDLLink *dl_link = dobj->dl_link;
            u32 link;

            for (link = 0u; link < GC_COMMON_MAX_DLLINKS; link++, dl_link++)
            {
                u32 binding;

                if (dl_link->list_id == (s32)NDS_RENDERER_STAGE_DL_HEADS)
                {
                    break;
                }
                if ((dl_link->list_id < 0) ||
                    ((u32)dl_link->list_id >= NDS_RENDERER_STAGE_DL_HEADS) ||
                    (dl_link->dl == NULL))
                {
                    continue;
                }
                binding = workspace->binding_count++;
                if (binding >= NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT)
                {
                    return FALSE;
                }
                if (live->binding_index == 0xffffu)
                {
                    live->binding_index = (u16)binding;
                }
                workspace->binding_dobjs[binding] = dobj;
                workspace->binding_display_lists[binding] = dl_link->dl;
                workspace->binding_heads[binding] = (u8)dl_link->list_id;
                workspace->binding_link_index[binding] = (u8)link;
            }
        }
        else if ((dl_links == 0u) && (dobj->dv != NULL))
        {
            u32 binding = workspace->binding_count++;
            if (binding >= NDS_RENDERER_ADAPTER_STAGE_BINDING_COUNT)
            {
                return FALSE;
            }
            live->binding_index = (u16)binding;
            workspace->binding_dobjs[binding] = dobj;
            workspace->binding_display_lists[binding] = dobj->dv;
            workspace->binding_heads[binding] = 0u;
            workspace->binding_link_index[binding] = 0xffu;
        }
        if ((dobj->child != NULL) &&
            (ndsRendererAdapterCollectNativeStageDObjs(
                 dobj->child, owner, (u16)index, (u8)(depth + 1u), dl_links,
                 workspace) == FALSE))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/* The source's tree walk skips a hidden DObj and everything under it
 * (gcDrawDObjTreeDLLinks, objdisplay.c:1707-1744; gcDrawDObjTree likewise),
 * so a binding is hidden while its DObj or any ancestor is. Stage maps are
 * one to three levels deep. */
static sb32 ndsRendererAdapterStageDObjHiddenInTree(const DObj *dobj)
{
    u32 depth = 0u;

    while ((dobj != NULL) && (dobj != DOBJ_PARENT_NULL) && (depth++ < 32u))
    {
        if ((dobj->flags & DOBJ_FLAG_HIDDEN) != 0u)
        {
            return TRUE;
        }
        dobj = dobj->parent;
    }
    return FALSE;
}

static u32 ndsRendererAdapterNativeStageStampValue(u32 stamp, uintptr_t value)
{
    stamp ^= (u32)value;
    stamp *= 16777619u;
    stamp ^= stamp >> 16;
    return stamp;
}

/* Which decline site of the three reason-4 builders ran last (1-based,
 * textual order across BuildNativeStageTopologyStamp, Collect and
 * CaptureTask36StageWorld; 0 = never declined) and the loop index there.
 * The adapter latches only reason 4 for all of them (2026-09-07). */
volatile u32 gNdsRendererAdapterStageTopologyFailStep;
volatile u32 gNdsRendererAdapterStageTopologyFailIndex;
volatile u32 gNdsRendererAdapterStageTopologyCollectMask;
volatile u32 gNdsRendererAdapterStageTopologyCollectCounts;

static sb32 ndsRendererAdapterBuildNativeStageTopologyStamp(
    NDSRendererAdapterNativeStageWorkspace *workspace,
    u32 generation,
    u32 *out_stamp)
{
    u32 stamp = 2166136261u;
    u32 i;

    if ((workspace == NULL) || (out_stamp == NULL) || (generation == 0u) ||
        (workspace->dobj_count !=
         ndsRendererAdapterNativeStageActiveDObjCount()) ||
        (workspace->binding_count !=
         ndsRendererAdapterNativeStageActiveBindingCount()))
    {
        gNdsRendererAdapterStageTopologyFailStep = 1u;
        gNdsRendererAdapterStageTopologyFailIndex = 0xffffffffu;
        return FALSE;
    }
    stamp = ndsRendererAdapterNativeStageStampValue(stamp, generation);
    for (i = 0u; i < ndsRendererAdapterNativeStageActiveAssetCount(); i++)
    {
        NDSRelocLoadedFile *loaded = workspace->loaded[i];

        if ((loaded == NULL) || (loaded->data == NULL) ||
            (loaded->owner_generation != generation))
        {
            gNdsRendererAdapterStageTopologyFailStep = 2u;
            gNdsRendererAdapterStageTopologyFailIndex = i;
            return FALSE;
        }
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)loaded);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)loaded->data);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, loaded->asset_id);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, loaded->data_size);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, loaded->owner_generation);
    }
    for (i = 0u; i < ndsRendererAdapterNativeStageActiveSegmentCount(); i++)
    {
        GObj *gobj = ndsRendererAdapterNativeStageSegmentGObj(i);

        if ((gobj == NULL) || (gobj != workspace->segments[i]) ||
            (DObjGetStruct(gobj) == NULL) ||
            ((gobj->flags & GOBJ_FLAG_HIDDEN) != 0u) ||
            (gobj->dl_link_id != ndsRendererAdapterNativeStageSegmentLink(i)) ||
            (ndsRendererAdapterNativeStageProcMatches(i, gobj) == FALSE) ||
            (ndsRendererAdapterNativeStageGObjLinked(
                 gobj, gobj->dl_link_id) == FALSE))
        {
            gNdsRendererAdapterStageTopologyFailStep = 3u;
            gNdsRendererAdapterStageTopologyFailIndex = i;
            return FALSE;
        }
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)gobj);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)DObjGetStruct(gobj));
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, gobj->flags);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, gobj->dl_link_id);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)gobj->proc_display);
    }
    if (ndsRendererAdapterNativeStageLayer0OrderMatches(
            workspace->segments) == FALSE)
    {
        gNdsRendererAdapterStageTopologyFailStep = 4u;
        gNdsRendererAdapterStageTopologyFailIndex = i;
        return FALSE;
    }
    for (i = 0u; i < workspace->dobj_count; i++)
    {
        DObj *dobj = workspace->dobjs[i];
        NDSRendererNativeStageDObj *live = &workspace->live_dobjs[i];
        u16 transform_flags;
        u32 xobj_index;

        if ((dobj == NULL) || (live->identity != dobj) ||
            (ndsRendererAdapterNativeStageTransformFlags(
                 dobj, &transform_flags) == FALSE) ||
            (transform_flags != live->transform_flags))
        {
            gNdsRendererAdapterStageTopologyFailStep = 5u;
            gNdsRendererAdapterStageTopologyFailIndex = i;
            return FALSE;
        }
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj->parent_gobj);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj->parent);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj->child);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj->sib_next);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj->sib_prev);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj->dv);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)dobj->mobj);
        stamp = ndsRendererAdapterNativeStageStampValue(stamp, dobj->flags);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, dobj->xobjs_num);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, live->parent_index);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, live->binding_index);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, live->owner);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, live->depth);
        for (xobj_index = 0u; xobj_index < dobj->xobjs_num; xobj_index++)
        {
            XObj *xobj = dobj->xobjs[xobj_index];

            if (xobj == NULL)
            {
                gNdsRendererAdapterStageTopologyFailStep = 6u;
                gNdsRendererAdapterStageTopologyFailIndex = i;
                return FALSE;
            }
            stamp = ndsRendererAdapterNativeStageStampValue(
                stamp, (uintptr_t)xobj);
            stamp = ndsRendererAdapterNativeStageStampValue(
                stamp, xobj->kind);
        }
    }
    for (i = 0u; i < workspace->binding_count; i++)
    {
        const DObj *binding_dobj = workspace->binding_dobjs[i];
        const void *live_list;

        if ((binding_dobj == NULL) ||
            (workspace->binding_display_lists[i] == NULL))
        {
            gNdsRendererAdapterStageTopologyFailStep = 7u;
            gNdsRendererAdapterStageTopologyFailIndex = i;
            return FALSE;
        }
        live_list = (workspace->binding_link_index[i] == 0xffu) ?
            (const void *)binding_dobj->dv :
            ((binding_dobj->dl_link != NULL) ?
                 (const void *)binding_dobj->dl_link[
                     workspace->binding_link_index[i]].dl : NULL);
        if (live_list != workspace->binding_display_lists[i])
        {
            gNdsRendererAdapterStageTopologyFailStep = 8u;
            gNdsRendererAdapterStageTopologyFailIndex = i;
            return FALSE;
        }
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)workspace->binding_dobjs[i]);
        stamp = ndsRendererAdapterNativeStageStampValue(
            stamp, (uintptr_t)workspace->binding_display_lists[i]);
    }
    *out_stamp = (stamp != 0u) ? stamp : 1u;
    return TRUE;
}

#if NDS_TASK44_STAGE_STEADY
/* Task 44 item 3: the cheap half of stage admission.
 *
 * The asset-mutation generation proves the four reloc payloads have not been
 * replaced or unloaded; it says nothing about the scene graph that hangs off
 * them. These eight checks cover the graph mutations the stage owner must fail
 * closed on — a segment GObj swapped, hidden, relinked, or given a different
 * display proc — and every one of them is a direct global load. What steady
 * state no longer pays for is the O(n) work: the four loaded-file table scans,
 * the eight DL-link list walks, the two layer-0 order walks, and the 57-DObj /
 * 42-binding stamp rebuild with its per-DObj transform-flag derivation. Any
 * failure here drops through to exactly that full validation. */
static sb32 ndsRendererAdapterNativeStageSegmentsUnchanged(
    const NDSRendererAdapterNativeStageWorkspace *workspace)
{
    u32 i;

    for (i = 0u; i < ndsRendererAdapterNativeStageActiveSegmentCount(); i++)
    {
        GObj *gobj = ndsRendererAdapterNativeStageSegmentGObj(i);

        if ((gobj == NULL) || (gobj != workspace->segments[i]) ||
            ((gobj->flags & GOBJ_FLAG_HIDDEN) != 0u) ||
            (gobj->dl_link_id !=
             ndsRendererAdapterNativeStageSegmentLink(i)) ||
            (DObjGetStruct(gobj) != workspace->task44_segment_roots[i]) ||
            (ndsRendererAdapterNativeStageProcMatches(i, gobj) == FALSE))
        {
            return FALSE;
        }
    }
    return TRUE;
}
#endif

#define NDS_STAGE_SEGMENT_BLOOM_BIT(gobj) \
    (1u << (((u32)(uintptr_t)(gobj) >> 3) & 31u))

static sb32 ndsRendererAdapterCollectNativeStageTopologyBody(
    NDSRendererAdapterNativeStageWorkspace *workspace);

/* The body is the only writer of segments[]; whichever way it returns, the
 * bloom then names every entry it left (P2-2p8, 2026-10-04). */
static sb32 ndsRendererAdapterCollectNativeStageTopology(
    NDSRendererAdapterNativeStageWorkspace *workspace)
{
    const sb32 collected =
        ndsRendererAdapterCollectNativeStageTopologyBody(workspace);
    u32 bloom = 0u;
    u32 i;

    for (i = 0u; i < NDS_RENDERER_ADAPTER_STAGE_SEGMENT_COUNT; i++)
    {
        if (workspace->segments[i] != NULL)
        {
            bloom |= NDS_STAGE_SEGMENT_BLOOM_BIT(workspace->segments[i]);
        }
    }
    workspace->segment_bloom = bloom;
    return collected;
}

static sb32 ndsRendererAdapterCollectNativeStageTopologyBody(
    NDSRendererAdapterNativeStageWorkspace *workspace)
{
    const u32 segment_count = ndsRendererAdapterNativeStageActiveSegmentCount();
    u32 i;

    if (segment_count == 0u)
    {
        gNdsRendererAdapterStageTopologyFailStep = 9u;
        gNdsRendererAdapterStageTopologyFailIndex = 0xffffffffu;
        return FALSE;
    }
    /* Rows past the active count must read as absent to every later walk. */
    for (i = segment_count; i < NDS_RENDERER_ADAPTER_STAGE_SEGMENT_COUNT; i++)
    {
        workspace->segments[i] = NULL;
#if NDS_TASK44_STAGE_STEADY
        workspace->task44_segment_roots[i] = NULL;
#endif
    }
    for (i = 0u; i < segment_count; i++)
    {
        const NDSRendererAdapterNativeStageCaptureSegment *row =
            ndsRendererAdapterNativeStageCaptureRow(i);
        u32 first_dobj = workspace->dobj_count;
        GObj *gobj = ndsRendererAdapterNativeStageSegmentGObj(i);

        workspace->segments[i] = gobj;
        if ((row == NULL) || (gobj == NULL) ||
            ((gobj->flags & GOBJ_FLAG_HIDDEN) != 0u) ||
            (gobj->dl_link_id != ndsRendererAdapterNativeStageSegmentLink(i)) ||
            (ndsRendererAdapterNativeStageProcMatches(i, gobj) == FALSE) ||
            (ndsRendererAdapterNativeStageGObjLinked(
                 gobj, gobj->dl_link_id) == FALSE) ||
            (ndsRendererAdapterCollectNativeStageDObjs(
                 DObjGetStruct(gobj), row->owner,
                 0xffffu, 0u, row->dl_links, workspace) == FALSE) ||
            ((workspace->dobj_count - first_dobj) != row->dobj_count))
        {
            /* Which operand declined, bits in operand order; bit 6 = the
             * DObj collection itself (inferred when nothing else did), and
             * the live/expected DObj counts packed for the same shot. */
            u32 mask = 0u;

            mask |= (row == NULL) ? 1u : 0u;
            mask |= (gobj == NULL) ? 2u : 0u;
            if (gobj != NULL)
            {
                mask |= ((gobj->flags & GOBJ_FLAG_HIDDEN) != 0u) ? 4u : 0u;
                mask |= (gobj->dl_link_id !=
                         ndsRendererAdapterNativeStageSegmentLink(i)) ?
                    8u : 0u;
                mask |= (ndsRendererAdapterNativeStageProcMatches(i, gobj) ==
                         FALSE) ? 16u : 0u;
                mask |= (ndsRendererAdapterNativeStageGObjLinked(
                             gobj, gobj->dl_link_id) == FALSE) ? 32u : 0u;
            }
            if ((mask == 0u) && (row != NULL) &&
                ((workspace->dobj_count - first_dobj) == row->dobj_count))
            {
                mask |= 64u;
            }
            gNdsRendererAdapterStageTopologyCollectMask = mask;
            gNdsRendererAdapterStageTopologyCollectCounts =
                ((workspace->dobj_count - first_dobj) << 16) |
                ((row != NULL) ? (u32)row->dobj_count : 0xffffu);
            gNdsRendererAdapterStageTopologyFailStep = 10u;
            gNdsRendererAdapterStageTopologyFailIndex = i;
            return FALSE;
        }
#if NDS_TASK44_STAGE_STEADY
        workspace->task44_segment_roots[i] = DObjGetStruct(gobj);
#endif
    }
    /* P2-4n1 step 7: on a DLLink packet every captured binding must hang off
     * the DObj and draw into the head the generator compiled it from, or the
     * runs would be emitted under the wrong live world. Layer packets have
     * no head arrays and skip this. */
    if (ndsRendererNativeStageHasBindingHeads() != FALSE)
    {
        for (i = 0u; i < workspace->binding_count; i++)
        {
            u32 dobj_index;
            u32 head;

            if ((ndsRendererNativeStageBindingIdentity(i, &dobj_index,
                                                       &head) == FALSE) ||
                (dobj_index >= workspace->dobj_count) ||
                (workspace->dobjs[dobj_index] != workspace->binding_dobjs[i]) ||
                (head != workspace->binding_heads[i]))
            {
                gNdsRendererAdapterStageTopologyFailStep = 11u;
                gNdsRendererAdapterStageTopologyFailIndex = i;
                return FALSE;
            }
        }
    }
    {
        u32 mask = 0u;

        mask |= (workspace->dobj_count !=
                 ndsRendererAdapterNativeStageActiveDObjCount()) ? 1u : 0u;
        mask |= (workspace->binding_count !=
                 ndsRendererAdapterNativeStageActiveBindingCount()) ? 2u : 0u;
        mask |= (ndsRendererAdapterNativeStageLayer0OrderMatches(
                     workspace->segments) == FALSE) ? 4u : 0u;
        if (mask != 0u)
        {
            gNdsRendererAdapterStageTopologyCollectMask = 0x100u | mask;
            gNdsRendererAdapterStageTopologyCollectCounts =
                (workspace->dobj_count << 24) |
                (ndsRendererAdapterNativeStageActiveDObjCount() << 16) |
                (workspace->binding_count << 8) |
                ndsRendererAdapterNativeStageActiveBindingCount();
            gNdsRendererAdapterStageTopologyFailStep = 14u;
            gNdsRendererAdapterStageTopologyFailIndex = 0u;
            return FALSE;
        }
    }
    return TRUE;
}

#if NDS_TASK36_HW_COMPOSE
typedef struct NDSRendererAdapterStageFrameCamera
{
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;
    u32 projection_valid;
    u32 modelview_valid;
    NDSRendererAdapterMvpCamera recalc;
} NDSRendererAdapterStageFrameCamera;
#endif

#if NDS_TASK103_STAGE_RUN_PHASE
/* Task 103 E5 (2026-09-26): PrepMatrix was 92,426 ticks/frame of the four-CPU
 * stress's 114,133 prepare. These split it: the two camera builds, then per
 * dynamic binding its world, its composition and any MVP recalc. Lab only. */
volatile u32 gNdsTask103MatTask36CameraTicks;
volatile u32 gNdsTask103MatFrameCameraTicks;
volatile u32 gNdsTask103MatWorldTicks;
volatile u32 gNdsTask103MatComposeTicks;
volatile u32 gNdsTask103MatRecalcTicks;
volatile u32 gNdsTask103MatRecalcCount;
volatile u32 gNdsTask103MatBindings;
#endif

#if NDS_TASK44_STAGE_STEADY && NDS_R2_STAGE_VALIDATE_STRIDE
/* Quiet dynamic bindings: revalidated one frame in this many (1, 2, 4 or 8;
 * 1 = every frame, the behaviour before 2026-10-04). Same-ROM A/B word. */
volatile u32 gNdsStageDynStride __attribute__((used, section(".data"))) = 4u;
#define NDS_STAGE_DYN_QUIET_MIN 8u
#include <gr/ground.h>

/* Bindings whose DObj or an ancestor is a yakumono -- map collision that
 * moves. Their render must never lag the collision fighters stand on, so the
 * dynamic stride skips them. Once per topology capture. */
static u64 __attribute__((cold)) ndsRendererAdapterStageYakumonoBindings(
    const NDSRendererAdapterNativeStageWorkspace *workspace)
{
    u64 mask = 0u;
    u32 binding_index;

    if ((gMPCollisionYakumonoDObjs == NULL) || (gMPCollisionYakumonosNum <= 0))
    {
        return 0u;
    }
    for (binding_index = 0u;
         (binding_index < workspace->binding_count) && (binding_index < 64u);
         binding_index++)
    {
        DObj *dobj;

        for (dobj = workspace->binding_dobjs[binding_index];
             (dobj != NULL) && (dobj != DOBJ_PARENT_NULL);
             dobj = dobj->parent)
        {
            s32 i;

            for (i = 0; i < gMPCollisionYakumonosNum; i++)
            {
                if (gMPCollisionYakumonoDObjs->dobjs[i] == dobj)
                {
                    mask |= (u64)1u << binding_index;
                    break;
                }
            }
            if (((mask >> binding_index) & 1u) != 0u)
            {
                break;
            }
        }
    }
    return mask;
}
#endif

static sb32 ndsRendererAdapterPrepareNativeStageBindingMatrix(
    CObj *cobj, NDSRendererAdapterNativeStageWorkspace *workspace,
    u32 binding_index
#if NDS_TASK36_HW_COMPOSE
    , NDSRendererAdapterStageFrameCamera *camera
#endif
    )
{
#if NDS_TASK36_HW_COMPOSE
    DObj *dobj = workspace->binding_dobjs[binding_index];
    NDSRendererMatrix20p12 world;
    const NDSRendererMatrix20p12 *world_ptr;
    NDSRendererMatrix20p12 *out = &workspace->binding_composed[binding_index];
    const NDSRendererMatrix20p12 *projection_ptr = NULL;
    const NDSRendererMatrix20p12 *modelview_ptr = out;
    u32 kind = ndsRendererAdapterDirectMvpRecalcKind(dobj);
#if NDS_TASK103_STAGE_RUN_PHASE
    u32 task103_mark = cpuGetTiming();
    u32 task103_now;
#endif

    if (kind != 0u) { sNdsRendererAdapterMvpRecalcScaleX = 1.0F; }
    /* The cached world itself, not a copy (2026-10-04). */
#if NDS_TASK44_STAGE_STEADY && NDS_R2_STAGE_VALIDATE_STRIDE
    /* P2-2p8 (2026-10-04, owner ruling D12c): a dynamic binding whose chain
     * walk rebuilt nothing for NDS_STAGE_DYN_QUIET_MIN frame validations in a
     * row is revalidated one frame in gNdsStageDynStride (the slice 44 cursor
     * spreads them evenly); on the others its persistent world is reused --
     * the camera compose below still runs every frame. A rebuild returns the
     * binding to every-frame validation, so a part that starts moving after a
     * quiet spell shows it at most stride - 1 frames late. */
    {
        u8 *quiet = &workspace->dyn_quiet[binding_index];
        const u32 stride = gNdsStageDynStride;
        const u32 rebuilds = sNdsRendererAdapterStageWorldRebuilds;
        const sb32 stale =
            ((stride > 1u) && (*quiet >= NDS_STAGE_DYN_QUIET_MIN) &&
             (((workspace->dyn_pinned_mask >> binding_index) & 1u) == 0u) &&
             (((binding_index + workspace->slice44_validate_cursor) &
               (stride - 1u)) != 0u)) ? TRUE : FALSE;

        world_ptr = ndsRendererAdapterPersistentStageWorldPtr(dobj, &world,
                                                              stale);
        if (stale == FALSE)
        {
            if (sNdsRendererAdapterStageWorldRebuilds != rebuilds)
            {
                *quiet = 0u;
            }
            else if (*quiet < 255u)
            {
                (*quiet)++;
            }
        }
    }
#else
    world_ptr = ndsRendererAdapterPersistentStageWorldPtr(dobj, &world, FALSE);
#endif
    if (world_ptr == NULL)
    { return FALSE; }
#if NDS_TASK103_STAGE_RUN_PHASE
    task103_now = cpuGetTiming();
    NDS_DIAG(gNdsTask103MatWorldTicks += task103_now - task103_mark);
    task103_mark = task103_now;
    NDS_DIAG(gNdsTask103MatBindings++);
#endif
    /* Preserve the source multiplication order when a camera owns both parts.
     * Battle cameras normally supply LookAt*Persp in projection alone.
     *
     * P2-2p8 (2026-09-29): an MVP-recalc binding (kind != 0: the stage's
     * billboards, 11 a frame on Dream Land) has rows 0-2 replaced by
     * ApplyMvpRecalc below, which reads only the translation row of this
     * product -- so only that row is formed, bit for bit the full product's.
     * Not the persp-scale kind: its scale-conversion refusal returns with the
     * product itself as the result. */
    if ((kind != 0u) &&
        (kind != NDS_RENDERER_ADAPTER_MVP_RECALC_PERSP_SCA_KIND))
    {
        const NDSRendererMatrix20p12 *lhs = world_ptr;

        if (camera->modelview_valid != FALSE)
        {
            ndsRendererMtxMulRow3_20p12(lhs, &camera->modelview, out);
            lhs = out;
        }
        if (camera->projection_valid != FALSE)
        {
            ndsRendererMtxMulRow3_20p12(lhs, &camera->projection, out);
        }
        else if (camera->modelview_valid == FALSE)
        {
            *out = *world_ptr;
        }
    }
    else if (camera->modelview_valid != FALSE)
    {
        ndsRendererMtxMul20p12(world_ptr, &camera->modelview, out);
        if (camera->projection_valid != FALSE)
        { ndsRendererMtxMul20p12(out, &camera->projection, out); }
    }
    else if (camera->projection_valid != FALSE)
    { ndsRendererMtxMul20p12(world_ptr, &camera->projection, out); }
    else { *out = *world_ptr; }
#if NDS_TASK103_STAGE_RUN_PHASE
    task103_now = cpuGetTiming();
    NDS_DIAG(gNdsTask103MatComposeTicks += task103_now - task103_mark);
    task103_mark = task103_now;
#endif
    if (kind != 0u)
    {
        ndsRendererAdapterApplyMvpRecalc(dobj, kind, cobj,
            &workspace->projection, &projection_ptr, out, &modelview_ptr,
            &camera->recalc);
#if NDS_TASK103_STAGE_RUN_PHASE
        NDS_DIAG(gNdsTask103MatRecalcTicks += cpuGetTiming() - task103_mark);
        NDS_DIAG(gNdsTask103MatRecalcCount++);
#endif
        if ((modelview_ptr == NULL) || (projection_ptr != NULL)) { return FALSE; }
    }
    return TRUE;
#else
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;
    const NDSRendererMatrix20p12 *projection_ptr;
    const NDSRendererMatrix20p12 *modelview_ptr;

    ndsRendererAdapterPrepareInitialMatrices(
        workspace->binding_dobjs[binding_index], cobj, TRUE,
        &projection, &projection_ptr, &modelview, &modelview_ptr);
    /* MVP recalc returns the completed product in modelview with no separate
     * projection. ComposeNativeRootMatrix explicitly supports that shape. */
    if ((projection_ptr == NULL) && (modelview_ptr == NULL))
    {
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
        gNdsRendererTask36AdapterRejectReason = 52u;
#endif
        return FALSE;
    }
#if !NDS_TASK36_HW_COMPOSE
    if ((binding_index != 0u) && (projection_ptr != NULL) &&
        (memcmp(&workspace->projection, projection_ptr,
                sizeof(workspace->projection)) != 0))
    {
        return FALSE;
    }
#endif
    if (ndsRendererAdapterComposeNativeRootMatrix(
            modelview_ptr, projection_ptr,
            &workspace->binding_composed[binding_index]) == FALSE)
    {
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
        gNdsRendererTask36AdapterRejectReason = 54u;
#endif
        return FALSE;
    }
    if ((binding_index == 0u) && (projection_ptr != NULL))
    {
        ndsRendererMatrixCopy20p12(&workspace->projection, projection_ptr);
    }
    return TRUE;
#endif
}

#if NDS_LAB_STAGE_BINDING_CENSUS
/* Lab binding-motion census (the measurement _BLOB_RIGID_MASKS in
 * generate_nds_native_stage.py is pinned from): per stage topology, every
 * binding whose world was built (seen) and every one whose world differed
 * from its previous frame's (moved). A rigid candidate is seen & ~moved, less
 * the billboards. Read with gdb after a natural run and a walk of the map.
 * NDS_LAB_STAGE_BINDING_CENSUS=1 on a lab build only. */
volatile u64 gNdsStageBindingSeenMask;
volatile u64 gNdsStageBindingMovedMask;
volatile u32 gNdsStageBindingCensusFrames;
static u64 sNdsStageBindingCensusValid;
static u32 sNdsStageBindingCensusGeneration = 0xffffffffu;
static NDSRendererMatrix20p12 sNdsStageBindingCensusWorld[64];

static void ndsRendererAdapterStageBindingCensus(
    const NDSRendererAdapterNativeStageWorkspace *workspace)
{
    u32 binding_index;

    if (sNdsStageBindingCensusGeneration != workspace->topology_generation)
    {
        sNdsStageBindingCensusGeneration = workspace->topology_generation;
        sNdsStageBindingCensusValid = 0u;
        gNdsStageBindingSeenMask = 0u;
        gNdsStageBindingMovedMask = 0u;
        gNdsStageBindingCensusFrames = 0u;
    }
    NDS_DIAG(gNdsStageBindingCensusFrames++);
    for (binding_index = 0u;
         (binding_index < workspace->binding_count) && (binding_index < 64u);
         binding_index++)
    {
        const u64 bit = (u64)1u << binding_index;
        NDSRendererMatrix20p12 world;

        if (ndsRendererAdapterBuildDObjWorldMatrixUncached(
                workspace->binding_dobjs[binding_index], &world) == FALSE)
        {
            continue;
        }
        if (((sNdsStageBindingCensusValid & bit) != 0u) &&
            (memcmp(&world, &sNdsStageBindingCensusWorld[binding_index],
                    sizeof(world)) != 0))
        {
            gNdsStageBindingMovedMask |= bit;
        }
        sNdsStageBindingCensusWorld[binding_index] = world;
        sNdsStageBindingCensusValid |= bit;
        gNdsStageBindingSeenMask |= bit;
    }
}
#endif

static sb32 ndsRendererAdapterPrepareNativeStageMatrices(
    CObj *cobj, NDSRendererAdapterNativeStageWorkspace *workspace)
{
    u32 binding_index;

#if NDS_LAB_STAGE_BINDING_CENSUS
    ndsRendererAdapterStageBindingCensus(workspace);
#endif
#if NDS_TASK36_HW_COMPOSE
    NDSRendererAdapterStageFrameCamera camera;
#if NDS_TASK103_STAGE_RUN_PHASE
    u32 task103_mark = cpuGetTiming();
    u32 task103_now;
#endif
    /* The frame camera first: its cache entry then holds the split look-at
     * and perspective the stage camera is made of (see
     * ndsRendererAdapterStageCameraFromFrameCache). */
    ndsRendererAdapterGetFrameCameraMatrices(cobj,
        &camera.projection, &camera.projection_valid,
        &camera.modelview, &camera.modelview_valid, NULL, NULL, NULL);
#if NDS_TASK103_STAGE_RUN_PHASE
    task103_now = cpuGetTiming();
    NDS_DIAG(gNdsTask103MatFrameCameraTicks += task103_now - task103_mark);
    task103_mark = task103_now;
#endif
    if ((ndsRendererAdapterStageCameraFromFrameCache(
             cobj, &workspace->projection,
             &workspace->camera_modelview) == FALSE) &&
        (ndsRendererAdapterBuildTask36StageCameraMatrices(
             cobj, &workspace->projection,
             &workspace->camera_modelview) == FALSE))
    {
#if NDS_RENDERER_PROFILE_LEVEL == 1
        gNdsRendererTask36AdapterRejectReason = 51u;
#endif
        return FALSE;
    }
#if NDS_TASK103_STAGE_RUN_PHASE
    NDS_DIAG(gNdsTask103MatTask36CameraTicks += cpuGetTiming() - task103_mark);
#endif
    camera.recalc.perspective = &workspace->projection;
    camera.recalc.perspective_f_valid = FALSE;
    camera.recalc.mod1_valid = FALSE;
    ndsRendererAdapterMvpMemoReset(&camera.recalc);
#if NDS_RENDERER_M3_PHASE0_PROFILE
    gNdsRendererTask36ObservedDynamicMaskLo = 0u;
    gNdsRendererTask36ObservedDynamicMaskHi = 0u;
    for (binding_index = 0u;
         binding_index < workspace->binding_count;
         binding_index++)
    {
        NDSRendererMatrix20p12 current_world;

        if ((ndsRendererAdapterBuildDObjWorldMatrixUncached(
                 workspace->binding_dobjs[binding_index],
                 &current_world) == FALSE) ||
            (memcmp(&current_world, &workspace->binding_world[binding_index],
                    sizeof(current_world)) != 0))
        {
            if (binding_index < 32u)
            {
                gNdsRendererTask36ObservedDynamicMaskLo |= 1u << binding_index;
            }
            else
            {
                gNdsRendererTask36ObservedDynamicMaskHi |=
                    1u << (binding_index - 32u);
            }
            if ((workspace->task36_runtime_rigid_mask &
                 ((u64)1u << binding_index)) != 0u)
            {
                NDS_DIAG(gNdsRendererTask36RigidConstancyMismatchCount++);
                return FALSE;
            }
        }
    }
#endif
#endif

    for (binding_index = 0u;
         binding_index < workspace->binding_count;
         binding_index++)
    {
#if NDS_TASK36_HW_COMPOSE
        if ((workspace->task36_runtime_rigid_mask &
             ((u64)1u << binding_index)) != 0u)
        {
            continue;
        }
#endif
        if (ndsRendererAdapterPrepareNativeStageBindingMatrix(
                cobj, workspace, binding_index
#if NDS_TASK36_HW_COMPOSE
                , &camera
#endif
                ) == FALSE)
        {
            return FALSE;
        }
    }
    return TRUE;
}

#if NDS_TASK36_HW_COMPOSE
/* Bumped by every rigid world capture; read by the stage GX cull
 * (nds_stage_gx.exec.inc). */
volatile u32 gNdsStageRigidWorldSerial;

static sb32 ndsRendererAdapterCaptureTask36StageWorld(
    NDSRendererAdapterNativeStageWorkspace *workspace)
{
    u32 binding_index;
    const u64 rigid_mask = ndsRendererNativeStageRigidBindingMask();

    for (binding_index = 0u;
         binding_index < workspace->binding_count;
         binding_index++)
    {
        if (ndsRendererAdapterBuildDObjWorldMatrixUncached(
                workspace->binding_dobjs[binding_index],
                &workspace->binding_world[binding_index]) == FALSE)
        {
            gNdsRendererAdapterStageTopologyFailStep = 12u;
            gNdsRendererAdapterStageTopologyFailIndex = binding_index;
            return FALSE;
        }
        if (((rigid_mask &
              ((u64)1u << binding_index)) != 0u) &&
            (ndsRendererAdapterCaptureStageWorldSourceKey(
                 workspace->binding_dobjs[binding_index],
                 &workspace->task36_rigid_source_keys[binding_index]) ==
             FALSE))
        {
            gNdsRendererAdapterStageTopologyFailStep = 13u;
            gNdsRendererAdapterStageTopologyFailIndex = binding_index;
            return FALSE;
        }
    }
    workspace->task36_runtime_rigid_mask =
        rigid_mask;
#if NDS_TASK44_STAGE_STEADY && NDS_R2_STAGE_VALIDATE_STRIDE
    workspace->dyn_pinned_mask =
        ndsRendererAdapterStageYakumonoBindings(workspace);
#endif
    /* The stage GX cull keeps each rigid run's world-space bounds until the
     * worlds are captured again (nds_stage_gx.exec.inc). */
    gNdsStageRigidWorldSerial++;
#if NDS_TASK44_STAGE_STEADY
    workspace->task44_rigid_binding_count = 0u;
    for (binding_index = 0u;
         binding_index < workspace->binding_count;
         binding_index++)
    {
        if ((rigid_mask &
             ((u64)1u << binding_index)) != 0u)
        {
            workspace->task44_rigid_bindings[
                workspace->task44_rigid_binding_count++] = (u8)binding_index;
        }
    }
    workspace->task44_binding_lists_valid = TRUE;
#endif
    return TRUE;
}

static void ndsRendererAdapterValidateTask36StageWorld(
    NDSRendererAdapterNativeStageWorkspace *workspace)
{
    u32 binding_index;
    const u64 rigid_mask = ndsRendererNativeStageRigidBindingMask();
#if NDS_TASK44_STAGE_STEADY
    u32 rigid_slot;
#endif

#if NDS_R2_STAGE_VALIDATE_STRIDE
    /* Slice 44. Demotion is one-way within a topology: a binding that stopped
     * being rigid does not become rigid again until capture re-arms the mask at
     * ndsRendererAdapterCaptureTask36StageWorld. Before the stride the mask was
     * rebuilt from the constant here every frame and cleared again by the sweep,
     * which was equivalent only because the sweep was complete. With a partial
     * sweep, re-arming would resurrect a binding this frame's slice did not
     * look at. */
    if (workspace->task36_runtime_rigid_mask == 0u)
    {
        return;
    }
#endif
    workspace->task36_runtime_rigid_mask =
        rigid_mask;
#if NDS_TASK44_STAGE_STEADY
    /* Task 44 item 4: walk the 26 rigid bindings directly. The list is only
     * consulted when capture built it for this topology; otherwise fall back
     * to the mask scan below so a torn workspace can never skip validation. */
    if (workspace->task44_binding_lists_valid != FALSE)
    {
        for (rigid_slot = 0u;
             rigid_slot < workspace->task44_rigid_binding_count;
             rigid_slot++)
        {
#if NDS_R2_STAGE_VALIDATE_STRIDE
            if ((rigid_slot % NDS_R2_STAGE_VALIDATE_STRIDE) !=
                workspace->slice44_validate_cursor)
            {
                NDS_DIAG(gNdsR2Slice44RigidSkips++);
                continue;
            }
            NDS_DIAG(gNdsR2Slice44RigidChecks++);
#endif
            binding_index = workspace->task44_rigid_bindings[rigid_slot];
            if (ndsRendererAdapterStageWorldSourceKeyMatches(
                    workspace->binding_dobjs[binding_index],
                    &workspace->task36_rigid_source_keys[binding_index]) ==
                FALSE)
            {
                workspace->task36_runtime_rigid_mask = 0u;
#if NDS_RENDERER_PROFILE_LEVEL == 1
                NDS_DIAG(gNdsRendererTask36RigidConstancyMismatchCount++);
#endif
                return;
            }
        }
        return;
    }
#endif
    for (binding_index = 0u;
         binding_index < workspace->binding_count;
         binding_index++)
    {
        if ((rigid_mask &
             ((u64)1u << binding_index)) == 0u)
        {
            continue;
        }
        if (ndsRendererAdapterStageWorldSourceKeyMatches(
                workspace->binding_dobjs[binding_index],
                &workspace->task36_rigid_source_keys[binding_index]) ==
            FALSE)
        {
            workspace->task36_runtime_rigid_mask = 0u;
#if NDS_RENDERER_PROFILE_LEVEL == 1
            NDS_DIAG(gNdsRendererTask36RigidConstancyMismatchCount++);
#endif
            return;
        }
    }
}
#endif

static sb32 ndsRendererAdapterPrepareNativeStageMaterials(
    NDSRendererAdapterNativeStageWorkspace *workspace)
{
    const u32 material_count = ndsRendererAdapterNativeStageActiveMaterialCount();
    u32 i;

    /* Each slot's binding and expected MObj flags come from the packet's
     * material-event table; Dream Land's are {20,22,31,32}/{1,1,0x6b,0x6b}. */
    for (i = 0u; i < material_count; i++)
    {
        u32 binding_index;
        u32 flags;
        u32 mobj_index;
        MObj *mobj;

        if ((ndsRendererNativeStageMaterialBinding(i, &binding_index,
                                                   &flags, &mobj_index) ==
             FALSE) ||
            (binding_index >= workspace->binding_count) ||
            (workspace->binding_dobjs[binding_index] == NULL))
        {
            return FALSE;
        }
        /* The slot's own MObj: the DObj's list in draw order, as
         * gcDrawMObjForDObj walks it. */
        mobj = workspace->binding_dobjs[binding_index]->mobj;
        while ((mobj != NULL) && (mobj_index != 0u))
        {
            mobj = mobj->next;
            mobj_index--;
        }
        if ((mobj == NULL) ||
            (ndsRendererAdapterMaterialFlags(mobj) != (u16)flags) ||
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 mobj, &workspace->materials[i], FALSE,
                 &workspace->material_curr[i],
                 &workspace->material_next[i]) == FALSE))
        {
#if NDS_R2_SECOND_ENTRY_DIAG
            /* Latch the first mismatch against the selected packet before a
             * later scene can overwrite its binding and material identity. */
            NDS_DIAG(gNdsR2StageMaterialRejectCount++);
            if (gNdsR2StageMaterialRejectIndex == 0xFFFFFFFFu)
            {
                gNdsR2StageMaterialRejectIndex = i;
                gNdsR2StageMaterialRejectBinding = binding_index;
                gNdsR2StageMaterialRejectDObj =
                    (u32)(uintptr_t)workspace->binding_dobjs[binding_index];
                gNdsR2StageMaterialRejectMObj = (u32)(uintptr_t)mobj;
                gNdsR2StageMaterialRejectFlagsWant = flags;
                gNdsR2StageMaterialRejectFlagsGot =
                    ndsRendererAdapterMaterialFlags(mobj);
                gNdsR2StageMaterialRejectHeapGen = gNdsTaskmanHeapGeneration;
            }
#endif
            return FALSE;
        }
        workspace->material_mobjs[i] = mobj;
    }
    return TRUE;
}

static void ndsRendererAdapterCommitNativeStageMaterials(
    NDSRendererAdapterNativeStageWorkspace *workspace, u32 segment_index)
{
    u32 mask = ndsRendererNativeStageMaterialMask(segment_index);
    u32 i;

    /* Commit only the material slots owned by this packet's segment. Stages
     * with no material animation have no MObjs in this workspace. */
    for (i = 0u; mask != 0u; i++, mask >>= 1u)
    {
        if ((mask & 1u) == 0u)
        {
            continue;
        }
        workspace->material_mobjs[i]->texture_id_curr =
            workspace->material_curr[i];
        workspace->material_mobjs[i]->texture_id_next =
            workspace->material_next[i];
#if NDS_RENDERER_PROFILE_LEVEL == 1
        NDS_DIAG(gNdsRendererM3MaterialCommitCount++);
#endif
    }
}

#if NDS_TASK103_STAGE_RUN_PHASE
/* Task 103 E2/E4. Defined here rather than in nds_renderer.c because the spans
 * they measure live in this file, and ahead of both users because C needs the
 * declaration first.
 *
 * E2 wraps the segment commit and the material setup ahead of it. E3 then found
 * that path is only 40% of the stage bucket: **236,039 ticks/frame -- 60% of
 * STG and 18% of all frame work -- are inside
 * ndsRendererAdapterPrepareNativeStageOwner, at one call per frame**, which no
 * task had ever profiled. E4's six spans split that function's steady-state
 * body into the steps it actually runs, so the number stops being a function
 * name and becomes a lever. Lab only, default off. */
volatile u32 gNdsTask103CommitTicks;
volatile u32 gNdsTask103CommitCount;
volatile u32 gNdsTask103MaterialTicks;
volatile u32 gNdsTask103PrepAdmitTicks;
volatile u32 gNdsTask103PrepValidateTicks;
volatile u32 gNdsTask103PrepMatrixTicks;
volatile u32 gNdsTask103PrepMaterialTicks;
volatile u32 gNdsTask103PrepConfigTicks;
volatile u32 gNdsTask103PrepOwnerTicks;
volatile u32 gNdsTask103PrepCalls;
#endif

static s32 ndsRendererAdapterPrepareNativeStageOwnerBody(
    void *camera_gobj_ptr);

static unsigned int ndsRendererAdapterPrepareNativeStageOwnerOnHotStack(
    void *camera_gobj_ptr)
{
    return (unsigned int)ndsRendererAdapterPrepareNativeStageOwnerBody(
        camera_gobj_ptr);
}

/* P2-2p8 (2026-09-27): the stage owner prep -- world matrices, MVP
 * recalc, material snapshot, once a frame -- runs on the DTCM hot stack
 * (port/coroutine.h), as the segment commit already did. Same-ROM A/B with
 * the fighter display proc: paired WORK-H -11.8K (STG -8.7K). It reads no
 * storage into a stack buffer and hands no stack address to DMA. */
s32 ndsRendererAdapterPrepareNativeStageOwner(void *camera_gobj_ptr)
{
    return (s32)ndsDtcmHotStackRun(
        ndsRendererAdapterPrepareNativeStageOwnerOnHotStack,
        camera_gobj_ptr);
}

#if NDS_P2_STAGE_SECTOR
extern DObj *ndsGRSectorPlatformDObj(void);
#endif

static s32 ndsRendererAdapterPrepareNativeStageOwnerBody(
    void *camera_gobj_ptr)
{
    NDSRendererAdapterNativeStageWorkspace *workspace =
        &sNdsRendererAdapterNativeStageWorkspace;
    NDSRelocLoadedFile *loaded[NDS_RENDERER_ADAPTER_STAGE_ASSET_COUNT];
    const u32 asset_count = ndsRendererAdapterNativeStageActiveAssetCount();
    GObj *camera_gobj = camera_gobj_ptr;
    CObj *cobj = (camera_gobj != NULL) ? CObjGetStruct(camera_gobj) : NULL;
    u32 topology_generation = 0u;
    u32 topology_stamp = 0u;
    sb32 topology_cached = FALSE;
    u32 i;
#if NDS_TASK44_STAGE_STEADY
    sb32 steady_admitted = FALSE;
#endif
    /* UNCONDITIONAL. The reject label this feeds calls
     * ndsRendererHardwareAbortBattleStaticTextures, which discards the whole
     * hardware texture cache and leaves it unable to re-arm for the rest of the
     * scene. Which branch got there therefore cannot be a profile-only fact --
     * it is the difference between a next cycle that reads one counter and a
     * next cycle that re-derives six branches. See nds_renderer.c's row 6 note. */
    u32 task36_reject_reason = 1u;
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
    gNdsRendererTask36AdapterRejectReason = 0u;
#endif
#if NDS_TASK103_STAGE_RUN_PHASE
    u32 task103_prep_entry = cpuGetTiming();
    u32 task103_prep_mark;
#endif

    workspace->active = FALSE;
    /* P2-4: stage wallpapers are source SObjs on their own display link, so
     * they do not exist in the native DObj packet committed below. Bind the
     * converted BG2 wallpaper to the same live battle camera once per present.
     * Keep this before packet admission: an unrelated stage-packet decline
     * must not blank an otherwise valid native background. */
    if ((cobj != NULL) && (gSCManagerBattleState != NULL) &&
        ((u32)gSCManagerBattleState->gkind <= 8u))
    {
        (void)ndsNativeBattleWallpaperDraw(
            (u32)gSCManagerBattleState->gkind,
            cobj->vec.eye.x, cobj->vec.eye.y, cobj->vec.eye.z,
            cobj->vec.at.x, cobj->vec.at.y, cobj->vec.at.z);
    }
    if (gNdsRendererFastRunMode !=
        NDS_RENDERER_FAST_RUN_NATIVE_COMPLETE_STAGE)
    {
        return FALSE;
    }
    if ((asset_count == 0u) || (asset_count > NDS_RENDERER_ADAPTER_STAGE_ASSET_COUNT))
    {
        return FALSE;
    }
    if (cobj == NULL)
    {
        task36_reject_reason = 2u;
        goto reject;
    }
#if NDS_TASK44_STAGE_STEADY
    /* Task 44 item 3: steady-state admission. One generation compare plus the
     * cheap segment guard replaces the four asset lookups and the whole
     * topology stamp rebuild. Everything the fast path would have recomputed
     * (loaded[], asset_bases[], topology_generation, topology_stamp) is
     * already recorded in the workspace and provably unchanged, because every
     * seam that can change it bumps sNdsRelocStageAssetMutation. */
    if ((workspace->topology_valid != FALSE) &&
        (workspace->task44_admission_generation != 0u) &&
        (workspace->task44_admission_generation ==
         sNdsRelocStageAssetMutation) &&
        (ndsRendererAdapterNativeStageSegmentsUnchanged(workspace) != FALSE))
    {
        steady_admitted = TRUE;
        topology_generation = workspace->topology_generation;
        topology_stamp = workspace->topology_stamp;
        topology_cached = TRUE;
#if NDS_R2_SECOND_ENTRY_DIAG
        /* The fast path that reuses binding_dobjs[] wholesale. It consults
         * neither owner_generation nor the heap generation -- only
         * sNdsRelocStageAssetMutation -- so if that fails to move on a second
         * scene entry, last match's DObj pointers are re-admitted intact. The
         * existing counters here are PROFILE_LEVEL==1 only, which the tick-HUD
         * build is not, so this bug class was invisible to every run. */
        NDS_DIAG(gNdsR2StageSteadyAdmitCount++);
#endif
#if NDS_RENDERER_PROFILE_LEVEL == 1
        NDS_DIAG(gNdsRendererTask44SteadyAdmitCount++);
#endif
    }
    if (steady_admitted == FALSE)
#endif
    {
#if NDS_TASK44_STAGE_STEADY && (NDS_RENDERER_PROFILE_LEVEL == 1)
    NDS_DIAG(gNdsRendererTask44RevalidateCount++);
#endif
    for (i = 0u; i < asset_count; i++)
    {
        loaded[i] = ndsRelocFindLoadedFileByAsset(
            ndsRendererAdapterNativeStageAssetId(i));
        if ((loaded[i] == NULL) || (loaded[i]->data == NULL) ||
            (loaded[i]->data_size !=
             ndsRendererAdapterNativeStageAssetSize(i)) ||
            (loaded[i]->owner_generation == 0u) ||
            ((i != 0u) &&
             (loaded[i]->owner_generation != topology_generation)))
        {
            task36_reject_reason = 3u;
            goto reject;
        }
        topology_generation = loaded[i]->owner_generation;
    }
    if ((workspace->topology_valid != FALSE) &&
        (workspace->topology_generation == topology_generation))
    {
        topology_cached = TRUE;
        for (i = 0u; i < asset_count; i++)
        {
            if (workspace->loaded[i] != loaded[i])
            {
                topology_cached = FALSE;
                break;
            }
        }
        if ((topology_cached != FALSE) &&
            ((ndsRendererAdapterBuildNativeStageTopologyStamp(
                  workspace, topology_generation, &topology_stamp) == FALSE) ||
             (topology_stamp != workspace->topology_stamp)))
        {
            topology_cached = FALSE;
        }
    }
    if (topology_cached == FALSE)
    {
#if NDS_R2_SECOND_ENTRY_DIAG
        /* A full rebuild: binding_dobjs[] is re-collected from the live tree.
         * This is what MUST happen on a second scene entry. */
        NDS_DIAG(gNdsR2StageTopologyRebuildCount++);
#endif
        bzero(workspace, sizeof(*workspace));
#if NDS_R2_STAGE_VALIDATE_STRIDE
        /* Slice 44. binding_dobjs[] is about to be re-collected from the live
         * tree, so every stage world entry keyed on an old DObj address is now
         * meaningless -- and a recycled heap address makes one of them *match*.
         * That was harmless while `validated_frame == frame` forced a rebuild
         * every frame; under the stride a collision would hand back the
         * previous topology's matrix. Free at runtime: this branch is the
         * once-per-scene rebuild that already bzeroes the whole workspace. */
        sNdsRendererAdapterStageWorldCacheCount = 0u;
        memset(sNdsRendererAdapterStageWorldIndex, 0,
               sizeof(sNdsRendererAdapterStageWorldIndex));
#endif
        for (i = 0u; i < asset_count; i++)
        {
            workspace->loaded[i] = loaded[i];
            workspace->frame.asset_bases[i] = loaded[i]->data;
        }
        if ((ndsRendererAdapterCollectNativeStageTopology(workspace) == FALSE) ||
#if NDS_TASK36_HW_COMPOSE
            (ndsRendererAdapterCaptureTask36StageWorld(workspace) == FALSE) ||
#endif
            (ndsRendererAdapterBuildNativeStageTopologyStamp(
                 workspace, topology_generation, &topology_stamp) == FALSE))
        {
            task36_reject_reason = 4u;
            goto reject;
        }
        workspace->topology_generation = topology_generation;
        workspace->topology_stamp = topology_stamp;
        workspace->topology_valid = TRUE;
    }
    else
    {
        for (i = 0u; i < asset_count; i++)
        {
            workspace->frame.asset_bases[i] = loaded[i]->data;
        }
    }
    /* Static DS payloads are source-independent once converted, but their
     * cache keys contain the live O2R source pointers. A stage asset may be
     * replaced at a new taskman-heap address after pre-GO texture preparation;
     * Task 44 already routes every such mutation through this full-validation
     * arm. Refresh only those pointer-derived key words here. This performs no
     * file I/O, conversion, allocation, or VRAM upload. */
    if (ndsRendererHardwareRefreshBattleStaticTexturePointers() == FALSE)
    {
        task36_reject_reason = 3u;
        goto reject;
    }
#if NDS_TASK44_STAGE_STEADY
    /* Only a completed full validation may arm the fast path. */
    workspace->task44_admission_generation = sNdsRelocStageAssetMutation;
#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererTask44AdmissionGeneration = sNdsRelocStageAssetMutation;
#endif
#endif
    }
#if NDS_TASK103_STAGE_RUN_PHASE
    task103_prep_mark = cpuGetTiming();
    NDS_DIAG(gNdsTask103PrepAdmitTicks += task103_prep_mark - task103_prep_entry);
#endif
#if NDS_TASK36_HW_COMPOSE
#if NDS_R2_STAGE_VALIDATE_STRIDE
    /* Slice 44: one advance per frame, here, because this is the only site the
     * c120 profile shows running exactly once -- 26 rigid checks a frame against
     * a 26-entry list. Both the rigid sweep below and the dynamic chain walk in
     * ndsRendererAdapterPrepareNativeStageMatrices read the cursor, so they stay
     * in phase and every frame does the same amount of work. */
    workspace->slice44_validate_cursor =
        (u8)((workspace->slice44_validate_cursor + 1u) %
             NDS_R2_STAGE_VALIDATE_STRIDE);
#endif
    ndsRendererAdapterValidateTask36StageWorld(workspace);
#endif
#if NDS_TASK103_STAGE_RUN_PHASE
    NDS_DIAG(gNdsTask103PrepValidateTicks += cpuGetTiming() - task103_prep_mark);
    task103_prep_mark = cpuGetTiming();
#endif
    if (ndsRendererAdapterPrepareNativeStageMatrices(cobj, workspace) == FALSE)
    {
        task36_reject_reason = 5u;
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
        /* The matrix path publishes a finer sub-reason, but only when
         * profiling is compiled in; 5u is the honest answer without it. */
        if (gNdsRendererTask36AdapterRejectReason != 0u)
        {
            task36_reject_reason = gNdsRendererTask36AdapterRejectReason;
        }
#endif
        goto reject;
    }
#if NDS_TASK103_STAGE_RUN_PHASE
    NDS_DIAG(gNdsTask103PrepMatrixTicks += cpuGetTiming() - task103_prep_mark);
    task103_prep_mark = cpuGetTiming();
#endif
    if (ndsRendererAdapterPrepareNativeStageMaterials(workspace) == FALSE)
    {
        task36_reject_reason = 7u;
        goto reject;
    }
#if NDS_TASK103_STAGE_RUN_PHASE
    NDS_DIAG(gNdsTask103PrepMaterialTicks += cpuGetTiming() - task103_prep_mark);
    task103_prep_mark = cpuGetTiming();
#endif

    bzero(&workspace->resolver, sizeof(workspace->resolver));
    workspace->resolver.primary_file = workspace->loaded[0];
    workspace->config = (NDSRendererConfig){0};
    workspace->config.max_depth = 8u;
    workspace->config.max_commands = 2048u;
    workspace->config.max_list_commands = 512u;
    workspace->config.texture_data_layout =
        NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    workspace->config.validate_range = ndsRendererAdapterStageValidateRange;
    workspace->config.immutable_command_span =
        ndsRendererAdapterImmutableCommandSpan;
    workspace->config.resolve_branch = ndsFighterDLDrawResolveBranch;
    workspace->config.resolve_data = ndsFighterDLDrawResolveRendererData;
    workspace->config.user = &workspace->resolver;
    workspace->frame.dobjs = workspace->live_dobjs;
    workspace->frame.binding_display_lists =
        workspace->binding_display_lists;
    workspace->frame.projection = &workspace->projection;
#if NDS_TASK36_HW_COMPOSE
    workspace->frame.camera_modelview = &workspace->camera_modelview;
    workspace->frame.binding_world = workspace->binding_world;
    workspace->frame.rigid_binding_mask =
        workspace->task36_runtime_rigid_mask;
#else
    workspace->frame.camera_modelview = NULL;
    workspace->frame.binding_world = NULL;
    workspace->frame.rigid_binding_mask = 0u;
#endif
    workspace->frame.binding_composed = workspace->binding_composed;
    workspace->frame.materials = workspace->materials;
    workspace->frame.config = &workspace->config;
    workspace->frame.topology_generation = workspace->topology_generation;
    workspace->frame.topology_stamp = workspace->topology_stamp;
    workspace->frame.hidden_binding_mask = 0u;
    {
#if NDS_P2_STAGE_SECTOR
        /* Owner (r58): the Sector Z wing-platform collision proxy is not
         * drawn (ndsGRSectorPlatformDObj). */
        const DObj *hidden_dobj = ndsGRSectorPlatformDObj();
#else
        const DObj *hidden_dobj = NULL;
#endif

        for (i = 0u; i < workspace->binding_count; i++)
        {
            if (((workspace->binding_dobjs[i]->flags & DOBJ_FLAG_NOTEXTURE) !=
                 0u) ||
                (ndsRendererAdapterStageDObjHiddenInTree(
                     workspace->binding_dobjs[i]) != FALSE) ||
                ((hidden_dobj != NULL) &&
                 (workspace->binding_dobjs[i] == hidden_dobj)))
            {
                workspace->frame.hidden_binding_mask |= (u64)1u << i;
            }
        }
    }
#if NDS_TASK103_STAGE_RUN_PHASE
    NDS_DIAG(gNdsTask103PrepConfigTicks += cpuGetTiming() - task103_prep_mark);
    task103_prep_mark = cpuGetTiming();
#endif
    if (ndsRendererPrepareNativeStageOwner(
            &workspace->frame, &workspace->stats) == FALSE)
    {
        workspace->topology_valid = FALSE;
        task36_reject_reason = 6u;
        goto reject;
    }
#if NDS_TASK103_STAGE_RUN_PHASE
    NDS_DIAG(gNdsTask103PrepOwnerTicks += cpuGetTiming() - task103_prep_mark);
    NDS_DIAG(gNdsTask103PrepCalls++);
#endif
    workspace->next_segment = 0u;
    workspace->active = TRUE;
#if NDS_RENDERER_PROFILE_LEVEL == 1
    gNdsRendererM3DObjCount = workspace->dobj_count;
    gNdsRendererM3BindingCount = workspace->binding_count;
    gNdsRendererM3MaterialShadowCount =
        ndsRendererAdapterNativeStageActiveMaterialCount();
#endif
    return TRUE;

reject:
#if NDS_TASK36_HW_COMPOSE && (NDS_RENDERER_PROFILE_LEVEL == 1)
    gNdsRendererTask36AdapterRejectReason = task36_reject_reason;
#endif
    /* BUGS row 6. Published unconditionally, and the FIRST reason is latched
     * rather than only the last: the abort below is irreversible for the scene,
     * so the branch that caused it is the one worth keeping. A later reject is
     * a consequence of the first one, not independent evidence. */
    NDS_DIAG(gNdsRendererStageOwnerRejectCount++);
    gNdsRendererStageOwnerLastRejectReason = task36_reject_reason;
    if (gNdsRendererStageOwnerFirstRejectReason == 0u)
    {
        gNdsRendererStageOwnerFirstRejectReason = task36_reject_reason;
    }
    workspace->active = FALSE;
    /* A FAILED DISPLAY-GRAPH PREPARE SAYS NOTHING ABOUT TEXTURE RESIDENCY, AND
     * THIS IS WHERE IT USED TO CLAIM OTHERWISE.
     *
     * This block was:
     *
     *     if (gNdsRendererBattleStaticTextureArmCount != 0u)
     *         ndsRendererHardwareAbortBattleStaticTextures();
     *
     * which discards the entire hardware texture cache and clears
     * sNdsRendererBattleStaticTexturePrepared. Both Arm call sites
     * (taskman_seam.c:5251 and :7975) fire only on the Wait->Go TRANSITION, so
     * they never run again inside a match -- and Arm refuses to re-arm without
     * Prepared anyway. One transient frame therefore dropped the 24 pinned
     * statics for the remainder of the match with no path back.
     *
     * Measured on 2026-08-04 (build-row6-v1, two-death match): at frame 427,
     * right after a star KO, abort went 0->1, preparednow 1->0, and
     * staticpin froze at 7144 and never moved again through frame 2174. The
     * scene stayed correctly textured -- the dynamic cache absorbed it -- but
     * frame rate fell from 27.9 to 20.0 for the rest of the match, and the
     * pinned corpus was gone for good.
     *
     * Nothing here justified that. The pinned corpus is uploaded at scene
     * prepare and is untouched by whatever made this frame's display graph
     * unusable; workspace->active = FALSE above is already the whole fallback
     * contract, and the generic path does not read the workspace. So the
     * correct behaviour on a post-arm reject is to degrade for THIS FRAME and
     * leave every texture exactly where it is, which also leaves the next
     * frame free to succeed normally.
     *
     * The event stays loud rather than becoming silent: the reject counters
     * above fire unconditionally, and this one isolates the post-arm case that
     * used to be destructive, so a regression here is still one counter away.
     */
    if (gNdsRendererBattleStaticTextureArmCount != 0u)
    {
        NDS_DIAG(gNdsRendererStageOwnerPostArmRejectCount++);
    }
    return FALSE;
}

/* The tick HUD's STG span for a committed segment (see the caller,
 * ndsStageGCDrawAllLoopRecordCapturedDisplay). */
#if NDS_TICK_HUD && !NDS_TASK103_STAGE_RUN_PHASE && \
    (NDS_RENDERER_PROFILE_LEVEL != 1)
extern volatile u32 gNdsTickHudStageTicks;
#define NDS_STAGE_DISPLAY_SPAN_BEGIN() u32 stage_span_start_ = NDS_TICK_HUD_SPAN_CLOCK()
#define NDS_STAGE_DISPLAY_SPAN_END() \
    (gNdsTickHudStageTicks += NDS_TICK_HUD_SPAN_CLOCK() - stage_span_start_)
#else
#define NDS_STAGE_DISPLAY_SPAN_BEGIN() ((void)0)
#define NDS_STAGE_DISPLAY_SPAN_END() ((void)0)
#endif

/* ndsRendererCommitNativeStageSegment on the DTCM hot stack. */
static unsigned int ndsRendererAdapterCommitSegmentOnHotStack(void *arg)
{
    return (unsigned int)ndsRendererCommitNativeStageSegment(
        *(const u32 *)arg);
}

s32 __attribute__((section(".itcm")))
ndsRendererAdapterCommitNativeStageDisplay(
    void *display_gobj_ptr, s32 link_id)
{
    NDSRendererAdapterNativeStageWorkspace *workspace =
        &sNdsRendererAdapterNativeStageWorkspace;
    GObj *display_gobj = display_gobj_ptr;
    u32 segment_count;
    u32 i;

    if (workspace->active == FALSE)
    {
        return FALSE;
    }
    /* Called for every display GObj of the stage camera, ~34 of ~42 of them
     * no segment: a GObj whose bit is clear matches no entry. */
    if ((workspace->segment_bloom &
         NDS_STAGE_SEGMENT_BLOOM_BIT(display_gobj)) == 0u)
    {
        return FALSE;
    }
    /* Read the loaded descriptor's count once, not once per compared
     * segment. */
    segment_count = ndsRendererAdapterNativeStageActiveSegmentCount();
    for (i = 0u; i < segment_count; i++)
    {
        if (display_gobj == workspace->segments[i])
        {
            NDS_STAGE_DISPLAY_SPAN_BEGIN();

            /* Presentation order, not packet order: the owner commits each
             * segment once, whenever its GObj is drawn (Saffron's gate,
             * segment 3, precedes segment 2). next_segment counts commits. */
            if ((u32)link_id != ndsRendererAdapterNativeStageSegmentLink(i))
            {
                (void)ndsRendererCommitNativeStageSegment(0xffffffffu);
                NDS_STAGE_DISPLAY_SPAN_END();
                return TRUE;
            }
#if NDS_TASK29_GX_CENSUS
            ndsRendererTask29GXSetOwner(NDS_RENDERER_PROFILE_OWNER_STAGE);
#endif
#if NDS_TASK103_STAGE_RUN_PHASE
            /* Task 103 E2. The run loop accounts for only 136,519 of the
             * ~370,000 stage bucket, so these two spans say whether the
             * remainder is per-segment scaffolding inside the commit or
             * material setup ahead of it. Wrapped at the call site because
             * ndsRendererCommitNativeStageSegment has several early returns
             * and an in-function span would miss them. */
            {
                u32 task103_mat_start = cpuGetTiming();
                u32 task103_commit_start;
                s32 task103_committed;

                ndsRendererAdapterCommitNativeStageMaterials(workspace, i);
                task103_commit_start = cpuGetTiming();
                task103_committed = ndsRendererCommitNativeStageSegment(i);
                gNdsTask103MaterialTicks +=
                    task103_commit_start - task103_mat_start;
                gNdsTask103CommitTicks +=
                    cpuGetTiming() - task103_commit_start;
                NDS_DIAG(gNdsTask103CommitCount++);
                if (task103_committed == FALSE)
                {
                    ndsRendererFinishNativeStageOwner();
                    workspace->active = FALSE;
                    return FALSE;
                }
            }
#else
            ndsRendererAdapterCommitNativeStageMaterials(workspace, i);
            if (ndsDtcmHotStackRun(ndsRendererAdapterCommitSegmentOnHotStack,
                                   &i) == FALSE)
            {
                ndsRendererFinishNativeStageOwner();
                workspace->active = FALSE;
                NDS_STAGE_DISPLAY_SPAN_END();
                return FALSE;
            }
#endif
            workspace->next_segment++;
            NDS_STAGE_DISPLAY_SPAN_END();
            return TRUE;
        }
    }
    return FALSE;
}

void ndsRendererAdapterFinishNativeStageOwner(void)
{
    NDSRendererAdapterNativeStageWorkspace *workspace =
        &sNdsRendererAdapterNativeStageWorkspace;

    if (workspace->active != FALSE)
    {
        ndsRendererFinishNativeStageOwner();
    }
#if NDS_TASK29_GX_CENSUS
    ndsRendererTask29GXSetOwner(NDS_RENDERER_PROFILE_OWNER_NONE);
#endif
    workspace->active = FALSE;
    workspace->next_segment = 0u;
}
#else
s32 ndsRendererAdapterPrepareNativeStageOwner(void *camera_gobj)
{
    (void)camera_gobj;
    return FALSE;
}

s32 __attribute__((section(".itcm")))
ndsRendererAdapterCommitNativeStageDisplay(
    void *display_gobj, s32 link_id)
{
    (void)display_gobj;
    (void)link_id;
    return FALSE;
}

void ndsRendererAdapterFinishNativeStageOwner(void)
{
}
#endif

/* Always compiled, unlike the validator below: this counts how often the
 * material write walk's capacity guard actually fires, and a guard whose hit
 * count nobody can read is indistinguishable from one that never runs. */
volatile u32 gNdsR2MaterialWalkBoundHits;
/* Runtime toggle so ONE binary can run both arms. Clearing this from a debugger
 * restores the pre-guard unbounded walk with identical code layout, identical
 * inlining and an identical ROM -- which is the only way to attribute the
 * Sudden Death freeze to the guard rather than to the placement change adding
 * the guard caused. Two different builds cannot answer that question; this
 * campaign has repeatedly measured layout moving results on its own (E11
 * removed real work and P95 still rose 15,744). Defaults to 1: the guard is
 * live in every build, and only a deliberate debugger write disables it. */
volatile u32 gNdsR2MaterialWalkBoundEnabled = 1u;

#if NDS_R2_SECOND_ENTRY_DIAG
/* Second-entry chain validator. Default OFF (Makefile NDS_R2_SECOND_ENTRY_DIAG).
 *
 * The material write walk was running off the end of a four-entry array, which
 * means the MObj chain it walks stops being what pass one measured. Bounding
 * the walk contained the damage; it did not say WHEN the list goes bad. This
 * answers that directly instead of inferring it from the overflow site: the
 * chain is recorded twice per DObj -- once before the counting pass, once
 * immediately before the writing pass -- so a list that is sound at the first
 * probe and broken at the second localises the corruption to the counting pass
 * itself, and one broken at both puts it upstream of this function entirely. */
#define NDS_R2_CHAIN_PROBE_MAX 64u

enum {
    NDS_R2_CHAIN_OK = 0u,
    NDS_R2_CHAIN_OVERLONG = 1u,   /* more nodes than any real DObj has */
    NDS_R2_CHAIN_CYCLE = 2u,      /* next pointer revisits a seen node */
    NDS_R2_CHAIN_OUT_OF_ARENA = 3u/* node or next outside the taskman arena */
};

typedef struct NDSR2ChainProbe {
    u32 status;
    u32 nodes;        /* nodes walked before terminating or failing */
    u32 first_bad;    /* address of the offending node, 0 when clean */
    u32 dobj;         /* which DObj owned the chain */
    u32 generation;   /* taskman-heap generation at probe time */
} NDSR2ChainProbe;

volatile NDSR2ChainProbe gNdsR2ChainProbePass1;
volatile NDSR2ChainProbe gNdsR2ChainProbePass2;
/* Latched at the FIRST failure of the run and never overwritten, so a later
 * clean frame cannot erase the evidence. */
volatile NDSR2ChainProbe gNdsR2ChainProbeFirstBad;
volatile u32 gNdsR2ChainProbeFirstBadPass;
volatile u32 gNdsR2ChainProbeInvalidCount;
volatile u32 gNdsR2ChainProbeCount;

static void ndsR2ChainProbe(DObj *dobj, volatile NDSR2ChainProbe *out, u32 pass)
{
    const MObj *seen[NDS_R2_CHAIN_PROBE_MAX];
    const MObj *mobj;
    u32 count = 0u;
    u32 status = NDS_R2_CHAIN_OK;
    u32 first_bad = 0u;

    NDS_DIAG(gNdsR2ChainProbeCount++);
    for (mobj = (dobj != NULL) ? dobj->mobj : NULL; mobj != NULL;
         mobj = mobj->next)
    {
        u32 i;

        if (ndsFighterDLScanRangeInTaskmanArena(mobj, sizeof(*mobj)) == FALSE)
        {
            status = NDS_R2_CHAIN_OUT_OF_ARENA;
            first_bad = (u32)(uintptr_t)mobj;
            break;
        }
        /* O(n^2) against a 64 bound is 4,096 compares worst case, and this is a
         * default-off diagnostic -- a hash would be more code for no answer. */
        for (i = 0u; i < count; i++)
        {
            if (seen[i] == mobj)
            {
                status = NDS_R2_CHAIN_CYCLE;
                first_bad = (u32)(uintptr_t)mobj;
                break;
            }
        }
        if (status != NDS_R2_CHAIN_OK)
        {
            break;
        }
        if (count >= NDS_R2_CHAIN_PROBE_MAX)
        {
            status = NDS_R2_CHAIN_OVERLONG;
            first_bad = (u32)(uintptr_t)mobj;
            break;
        }
        seen[count++] = mobj;
    }
    out->status = status;
    out->nodes = count;
    out->first_bad = first_bad;
    out->dobj = (u32)(uintptr_t)dobj;
    out->generation = gNdsTaskmanHeapGeneration;

    if (status != NDS_R2_CHAIN_OK)
    {
        NDS_DIAG(gNdsR2ChainProbeInvalidCount++);
        if (gNdsR2ChainProbeFirstBadPass == 0u)
        {
            gNdsR2ChainProbeFirstBadPass = pass;
            gNdsR2ChainProbeFirstBad = *out;
        }
    }
}
#endif

/* Cycle 110. The fighter material snapshot is a pure function of `mobj->sub`
 * plus texture_id_curr/next, lfrac and palette_id -- and it never varies. The
 * NDS_TICK_HUD census that has sat in this file since cycle 98 answered it on a
 * 60-second match: 20,100 builds, 20,069 of them byte-identical to the previous
 * snapshot for the same MObj, 31 first-sights, and **zero** variants. So ~761
 * cycles a build, about twelve times a frame, reconstruct a constant --
 * 13,176 ticks/frame of it, 2,124 of which is the single `mobj->sub.flags`
 * load missing cache at 139 cycles an execution.
 *
 * The key hashes the COMPLETE input set rather than the fields I believe can
 * animate. That is the difference between a skip that is correct by
 * construction and one that is correct until someone adds a texture-scroll
 * track: the builder reads nothing outside that set, so equal inputs means
 * equal output with only a 2^-32 collision to argue about. The heap generation
 * is in the key because MObj pointers are taskman-arena addresses and a scene
 * rewind reuses them.
 *
 * The build's side effect -- writing texture_id_curr/next back into the MObj --
 * is safe to skip for the same reason: the stored hash is taken AFTER the
 * write-back, so a match means the write would store what is already there.
 *
 * Only the production path passes keys. The hierarchy path's materials array is
 * a caller local that does not survive the frame, so it passes NULL and always
 * builds. After this the census counts REBUILDS, which makes gNdsFtrPreMatCalls
 * engagement proof: it should read tens, not tens of thousands. */
#if NDS_TICK_HUD
/* Engagement proof. gNdsFtrPreMatCalls cannot answer this: it counts calls that
 * reach the census inside the build wrapper, and the hierarchy call site passes
 * no keys, so a skipping production path and a never-skipping one can produce
 * the same census. These two count the decision itself. */
volatile u32 gNdsR2MatKeySkip;
volatile u32 gNdsR2MatKeyBuild;
/* And WHY a build happened, because the two answers point at different fixes.
 * Identity: the row holds a different MObj than last frame, so the block for
 * this one is sitting in some other row -- fix is a stable row assignment.
 * Inputs: same MObj, hash moved -- fix would be a narrower key, which is
 * already refuted. Guessing between them once cost a build. */
volatile u32 gNdsR2MatKeyMissIdentity;
volatile u32 gNdsR2MatKeyMissInputs;
#endif

/* Hashes all 30 words of MObjSub, including the six the builder never reads
 * (sub.unk48, sub.unk4C, sub.unk68..unk74). Narrowing it to the builder's exact
 * read set was tried and is REFUTED: the engagement counters came back
 * bit-identical -- 28,786 skips and 30,606 builds either way -- and FTR rose
 * 1,155. The rebuilds are not caused by those words at all. They are
 * `keys[count].mobj != mobj`: the materials array is indexed by (selected-root
 * slot, chain position), and which DObj lands in slot i rotates between frames,
 * so about half the lookups find the right block under the wrong index.
 *
 * Recovering that half needs a per-MObj store -- 33 live MObjs x 100 bytes plus
 * keys, about 7 KB of bss against an arena whose low-water is already under the
 * GObj-cap threshold -- or a stable slot assignment. Neither is this slice.
 * Since the narrow hash bought nothing, keep the one that needs no field audit
 * to stay correct. */
static u32 ndsRendererAdapterMaterialRow(DObj *dobj, u32 fallback_row)
{
    u32 base;
    u32 probe;

    if (dobj == NULL)
    {
        sNdsRendererAdapterMaterialRowClaimMask |= 1u << fallback_row;
        return fallback_row;
    }
    if (sNdsRendererAdapterMaterialRowGeneration != gNdsTaskmanHeapGeneration)
    {
        u32 j;

        for (j = 0u; j < NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED; j++)
        {
            sNdsRendererAdapterMaterialRowOwner[j] = NULL;
        }
        sNdsRendererAdapterMaterialRowGeneration = gNdsTaskmanHeapGeneration;
        sNdsRendererAdapterMaterialRowClaimMask = 0u;
    }
    /* Multiplicative, not `>> 4`: DObjs are allocated contiguously and a shift
     * hash strided them onto a handful of rows, so the probe loop ran several
     * iterations and the whole lookup measured 106 cycles a call (2,024
     * ticks/frame) on a 128-byte table that is always in cache. */
    base = ((u32)(uintptr_t)dobj * 2654435761u) >>
        (32u - NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED_LOG2);
    for (probe = 0u; probe < NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED; probe++)
    {
        u32 row = (base + probe) & (NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED - 1u);

        if (sNdsRendererAdapterMaterialRowOwner[row] == dobj)
        {
            /* The display contract can reference the same material DObj more
             * than once. Sharing in that case is source-equivalent and safe. */
            sNdsRendererAdapterMaterialRowClaimMask |= 1u << row;
            return row;
        }
        if ((sNdsRendererAdapterMaterialRowOwner[row] == NULL) &&
            ((sNdsRendererAdapterMaterialRowClaimMask & (1u << row)) == 0u))
        {
            sNdsRendererAdapterMaterialRowOwner[row] = dobj;
            sNdsRendererAdapterMaterialRowClaimMask |= 1u << row;
            return row;
        }
    }
    /* Every row is owned by some persistent DObj. Reclaim one that this CURRENT
     * owner has not claimed. The input key catches the changed identity and
     * rebuilds that row. selected_count cannot exceed the row count, so unless
     * duplicate DObjs reduced the number of claims, an unclaimed row is always
     * available here. */
    for (probe = 0u; probe < NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED; probe++)
    {
        u32 row = (base + probe) & (NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED - 1u);

        if ((sNdsRendererAdapterMaterialRowClaimMask & (1u << row)) == 0u)
        {
            sNdsRendererAdapterMaterialRowOwner[row] = dobj;
            sNdsRendererAdapterMaterialRowClaimMask |= 1u << row;
            return row;
        }
    }
    /* Defensive only: the collection bound above makes this unreachable. */
    return fallback_row;
}

/* The nine words that actually move during a match, in the two contiguous runs
 * they occupy. `primcolor` through `light2color` is `MObjSub` 0x50..0x67 -- the
 * five colour tracks gcPlayMObjMatAnim writes plus the prim level/min byte pair
 * that shares their run -- and texture_id_curr through palette_id is the twelve
 * bytes immediately after `sub`. Two cache lines instead of five, nine
 * multiply-accumulates instead of thirty-four. */
static u32 __attribute__((section(".itcm")))
ndsRendererAdapterMaterialAnimHash(const MObj *mobj)
{
    const u32 *colors = (const u32 *)(const void *)&mobj->sub.primcolor;
    u32 hash = 2166136261u;
    u32 i;

    for (i = 0u; i < 6u; i++)
    {
        hash = (hash ^ colors[i]) * 16777619u;
    }
    hash = (hash ^ (((u32)mobj->texture_id_curr << 16) |
                    (u32)mobj->texture_id_next)) * 16777619u;
    hash = (hash ^ *(const u32 *)(const void *)&mobj->lfrac) * 16777619u;
    hash = (hash ^ *(const u32 *)(const void *)&mobj->palette_id) * 16777619u;
    return hash;
}

/* `light2color` is the last field of the colour run, so the six words starting
 * at `primcolor` must land exactly on it. If MObjSub is ever reordered this
 * stops compiling rather than silently hashing the wrong bytes. */
_Static_assert(offsetof(MObjSub, light2color) ==
                   offsetof(MObjSub, primcolor) + 20u,
               "material anim hash assumes primcolor..light2color are six "
               "contiguous words");

/* P2-2 fighter packet: the identity of everything the current fighter's
 * material rows were built from -- every prepared MObj's animation hash and
 * pointer plus the colour modulate -- folded into one word per draw and handed
 * to the production owner as its packet key. The hashes already exist (the
 * material memo computes them per MObj per frame), so this is a few XORs. */
static u32 sNdsFighterPacketMaterialIdentity;

/* The animation-state hash and identity of every material the selected roots
 * carry, from the live MObj chains alone -- no rows, no snapshots -- so the
 * adapter can ask whether the packet will replay before it spends the
 * preparation on a frame whose replay reads none of it. Taken before any
 * build on the record path too, so the key means one thing on both paths. */
static u32 ndsRendererAdapterMaterialIdentity(
    DObj *const *material_dobjs, u32 count)
{
    u32 identity = 2166136261u;
    u32 i;

    for (i = 0u; i < count; i++)
    {
        const DObj *dobj = material_dobjs[i];
        const MObj *mobj;

        for (mobj = (dobj != NULL) ? dobj->mobj : NULL;
             mobj != NULL;
             mobj = mobj->next)
        {
            identity = (identity ^ ndsRendererAdapterMaterialAnimHash(mobj)) *
                       16777619u;
            identity ^= (u32)(uintptr_t)mobj;
        }
    }
    return identity;
}

static sb32 ndsRendererAdapterPrepareNativeMaterials(
    DObj *dobj, NDSRendererNativeMaterial *materials,
    u32 capacity, u32 *out_count,
    NDSRendererAdapterMaterialKey *keys,
    s32 *save_curr, s32 *save_next)
{
    MObj *mobj;
    u32 count = 0u;

    if ((materials == NULL) || (out_count == NULL))
    {
        return FALSE;
    }
    *out_count = 0u;
    if ((dobj == NULL) || (dobj->mobj == NULL))
    {
        return TRUE;
    }
#if NDS_R2_SECOND_ENTRY_DIAG
    ndsR2ChainProbe(dobj, &gNdsR2ChainProbePass1, 1u);
    ndsR2ChainProbe(dobj, &gNdsR2ChainProbePass2, 2u);
#endif
    /* There is no counting pre-pass any more. It walked the whole MObj chain a
     * second time -- a dependent pointer chase, 37 chains a frame, 1,215
     * ticks/frame on its `mobj = mobj->next` alone in the c115 per-PC census --
     * purely so an over-capacity chain could be rejected before anything was
     * written. The write walk below already carries that bound of its own, and
     * since the rollback slice it also reports `*out_count` on rejection, so the
     * caller undoes exactly the entries this walk touched. Two passes proved one
     * fact; one pass proves it at the point of use.
     *
     * The chain validator measured 13,938 chains of ONE node against a capacity
     * of four, so the difference between rejecting before and rejecting during
     * is a path that has never been taken. */
    for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next)
    {
        /* Bound the WRITE walk too, not just the counting one above. The count
         * pass only constrains this pass if the list is identical across both,
         * and it is not: ndsRendererAdapterBuildNativeMaterial is the
         * advance_texture_ids=TRUE wrapper, so this loop writes
         * mobj->texture_id_curr/next into every node as it walks. A list that
         * turns cyclic or is corrupted mid-walk ran `materials[count]` off the
         * end of a `capacity`-entry array -- capacity is 4 -- with no check at
         * all. Returning FALSE hands the caller its existing generic fallback
         * instead of corrupting whatever follows the array.
         *
         * Found while reproducing the Sudden Death freeze (docs/BUGS.md): the
         * scene presents two frames and then none, with no overflow assert
         * firing anywhere, and an interrupt landing inside this loop's inlined
         * material build. */
        if ((gNdsR2MaterialWalkBoundEnabled != 0u) && (count >= capacity))
        {
            /* Engagement proof, and it is NOT decoration. The Sudden Death
             * freeze stopped when this guard went in, but the default-off chain
             * validator then found 13,938 clean chains of ONE node against a
             * capacity of four -- so on that evidence this branch can never be
             * reached, and "the guard fixed the freeze" and "the chain is fine"
             * cannot both be true. This counter is what tells them apart: a run
             * that is freeze-free with this still at zero proves the guard was
             * not the cure and the real cause is still live. */
            NDS_DIAG(gNdsR2MaterialWalkBoundHits++);
            /* Report what the snapshot holds so the caller rolls back exactly
             * the entries this walk mutated, no more and no fewer. */
            *out_count = count;
            return FALSE;
        }
        if (save_curr != NULL)
        {
            /* The rollback snapshot, taken here rather than in a walk of its
             * own. ndsRendererAdapterSaveNativeMaterialTextureIds was a second
             * pass over the same chain reading the same two fields -- 3,429
             * ticks/frame in the c110 profile -- and the hash below loads them
             * anyway, so this costs two stores into a line the caller owns. */
            save_curr[count] = mobj->texture_id_curr;
            save_next[count] = mobj->texture_id_next;
        }
        if (keys != NULL)
        {
            u32 hash = ndsRendererAdapterMaterialAnimHash(mobj);

            if ((keys[count].mobj == mobj) &&
                (keys[count].heap_generation == gNdsTaskmanHeapGeneration) &&
                (keys[count].hash == hash))
            {
#if NDS_TICK_HUD
                NDS_DIAG(gNdsR2MatKeySkip++);
#endif
                count++;
                continue;
            }
#if NDS_TICK_HUD
            NDS_DIAG(gNdsR2MatKeyBuild++);
            if (keys[count].mobj != mobj)
            {
                NDS_DIAG(gNdsR2MatKeyMissIdentity++);
            }
            else
            {
                NDS_DIAG(gNdsR2MatKeyMissInputs++);
            }
#endif
        }
        if (ndsRendererAdapterBuildNativeMaterial(
                mobj, &materials[count]) == FALSE)
        {
            if (keys != NULL)
            {
                keys[count].mobj = NULL;
            }
            /* count + 1: entry `count` was snapshotted above and the builder is
             * the advance_texture_ids=TRUE wrapper, so it may have written this
             * MObj's ids before failing. Restoring an untouched entry writes
             * back what is already there, so over-reporting by one is safe and
             * under-reporting is not. */
            *out_count = count + 1u;
            return FALSE;
        }
        if (keys != NULL)
        {
            /* After the build, so the stored hashes describe the MObj the build
             * left behind -- it writes texture_id_curr/next back. Both are
             * stored on every build: the full one is only CHECKED periodically,
             * but it has to be current whenever that check lands. */
            keys[count].mobj = mobj;
            keys[count].heap_generation = gNdsTaskmanHeapGeneration;
            keys[count].hash = ndsRendererAdapterMaterialAnimHash(mobj);
        }
        count++;
    }
    *out_count = count;
    return TRUE;
}

static void ndsRendererAdapterRestoreNativeMaterialTextureIds(
    DObj *dobj,
    const s32 *curr,
    const s32 *next,
    u32 count)
{
    MObj *mobj;
    u32 i = 0u;

    if ((dobj == NULL) || (curr == NULL) || (next == NULL))
    {
        return;
    }
    for (mobj = dobj->mobj;
         (mobj != NULL) && (i < count);
         mobj = mobj->next, i++)
    {
        mobj->texture_id_curr = curr[i];
        mobj->texture_id_next = next[i];
    }
}

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
static sb32 ndsRendererAdapterValidateNativeOwnerMaterials(
    const NDSRendererNativeMaterial *materials,
    u32 material_count)
{
    u32 i;

    if ((material_count != 0u) && (materials == NULL))
    {
        return FALSE;
    }
    for (i = 0u; i < material_count; i++)
    {
        const NDSRendererNativeMaterial *material = &materials[i];
        u32 effects = material->effects;

        if (((effects & NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE) != 0u) &&
            (ndsRelocFindLoadedFileContaining(
                 (const void *)(uintptr_t)material->palette_image,
                 1u) == NULL))
        {
            return FALSE;
        }
        if (((effects & NDS_RENDERER_NATIVE_MATERIAL_BLOCK_IMAGE) != 0u) &&
            (ndsRelocFindLoadedFileContaining(
                 (const void *)(uintptr_t)material->block_image,
                 1u) == NULL))
        {
            return FALSE;
        }
        if (((effects & NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE) != 0u) &&
            (ndsRelocFindLoadedFileContaining(
                 (const void *)(uintptr_t)material->current_image,
                 1u) == NULL))
        {
            return FALSE;
        }
    }
    return TRUE;
}

static void ndsRendererAdapterRestoreNativeOwnerMaterialTextureIds(
    DObj *const *material_dobjs,
    u32 root_count)
{
    if (material_dobjs == NULL)
    {
        return;
    }
    /* A contract may select one material DObj more than once. Roll back in
     * reverse event order so each saved pre-event state is restored and the
     * earliest snapshot remains live for the ordinary renderer fallback. */
    while (root_count != 0u)
    {
        u32 root_index = --root_count;

        ndsRendererAdapterRestoreNativeMaterialTextureIds(
            material_dobjs[root_index],
            sNdsRendererAdapterNativeOwnerTextureCurr[root_index],
            sNdsRendererAdapterNativeOwnerTextureNext[root_index],
            sNdsRendererAdapterNativeOwnerTextureCounts[root_index]);
    }
}
#endif

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
/* A plan hit skips this outright (see "THE DELETION" at the plan-hit branch), and
 * the plan hits every frame -- the c112 cold map found its body inside the
 * driver's third-largest never-executed run. */
static sb32 __attribute__((noinline, cold, optimize("Os")))
ndsRendererAdapterValidateNativeOwnerCached(
    u32 slot,
    u32 battle_slot,
    u32 use_low_detail,
    const NDSRelocLoadedFile *owner_file,
    u32 root_count,
    const u32 *root_offsets,
    const u32 *material_counts)
{
    NDSRendererAdapterNativeOwnerValidationCache *cache;
    NDSRendererAdapterNativeOwnerValidationCache *victim;
    u32 e;
    u32 i;

    if ((slot >= NDS_RENDERER_NATIVE_FIGHTER_OWNER_COUNT) ||
        (owner_file == NULL) ||
        (root_offsets == NULL) || (material_counts == NULL) ||
        (root_count > NDS_FIGHTER_DL_ALL_DRAW_MAX_SELECTED))
    {
#if NDS_TICK_HUD
        gNdsFtrPreValidateReject++;
#endif
        return FALSE;
    }
    victim = &sNdsRendererAdapterNativeOwnerValidationCache[0];
    for (e = 0u; e < NDS_RENDERER_ADAPTER_OWNER_VALIDATION_POOL; e++)
    {
        cache = &sNdsRendererAdapterNativeOwnerValidationCache[e];
        if (cache->valid == 0u)
        {
            victim = cache;
            continue;
        }
        if ((victim->valid != 0u) && (cache->stamp < victim->stamp))
        {
            victim = cache;
        }
        if ((cache->slot != slot) ||
            (cache->data != owner_file->data) ||
            (cache->asset_id != owner_file->asset_id) ||
            (cache->owner_generation != owner_file->owner_generation) ||
            (cache->data_size != owner_file->data_size) ||
            (cache->root_count != root_count) ||
            (cache->battle_slot != battle_slot) ||
            (cache->use_low_detail != use_low_detail))
        {
            continue;
        }
        for (i = 0u; i < root_count; i++)
        {
            if ((cache->root_offsets[i] != root_offsets[i]) ||
                (cache->material_counts[i] != material_counts[i]))
            {
                break;
            }
        }
        if (i == root_count)
        {
#if NDS_TICK_HUD
            /* Cycle 98. THE counter this row existed to add: without it a
             * cache hit and a cache miss are indistinguishable here, and
             * deleting work whose cache already hits is the mistake cycle 93
             * avoided on the stage. */
            gNdsFtrPreValidateReuse++;
#endif
            cache->stamp = ++sNdsRendererAdapterNativeOwnerValidationStamp;
            return TRUE;
        }
    }

#if NDS_TICK_HUD
    gNdsFtrPreValidateBuild++;
#endif
    if (ndsRendererValidateNativeFighterOwner(
            slot, battle_slot, use_low_detail,
            ndsRelocNativeSourceSize(owner_file), root_count,
            root_offsets, material_counts) == FALSE)
    {
        return FALSE;
    }
    cache = victim;
    cache->slot = slot;
    cache->stamp = ++sNdsRendererAdapterNativeOwnerValidationStamp;
    cache->data = owner_file->data;
    cache->asset_id = owner_file->asset_id;
    cache->owner_generation = owner_file->owner_generation;
    cache->data_size = owner_file->data_size;
    cache->root_count = root_count;
    cache->battle_slot = battle_slot;
    cache->use_low_detail = use_low_detail;
    for (i = 0u; i < root_count; i++)
    {
        cache->root_offsets[i] = root_offsets[i];
        cache->material_counts[i] = material_counts[i];
    }
    cache->valid = TRUE;
    return TRUE;
}
#endif

static Gfx *ndsRendererAdapterEmitMaterialCommands(Gfx *branch_dl, MObj *mobj)
{
    u32 flags = ndsRendererAdapterMaterialFlags(mobj);
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

    if ((branch_dl == NULL) || (mobj == NULL))
    {
        return branch_dl;
    }

    ndsRendererAdapterMaterialTextureState(
        mobj, flags, &scau, &scav, &trau, &trav, &scrollu, &scrollv);

    if (((flags & MOBJ_FLAG_PALETTE) == 0u) &&
        (mobj->sub.palettes != NULL))
    {
        const void *palette = ndsRendererAdapterReadPointerEntry(
            mobj->sub.palettes, (s32)mobj->palette_id);

        if (palette != NULL)
        {
            ndsRendererAdapterEmitTextureImage(
                branch_dl++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1u, palette);
        }
    }
    if ((flags & MOBJ_FLAG_PALETTE) != 0)
    {
        ndsRendererAdapterEmitTextureImage(
            branch_dl++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1u,
            ndsRendererAdapterReadPointerEntry(
                mobj->sub.palettes, (s32)mobj->palette_id));
        if ((flags & (MOBJ_FLAG_SPLIT | MOBJ_FLAG_ALPHA)) != 0)
        {
            ndsRendererAdapterEmitSync(branch_dl++,
                                       NDS_FIGHTER_DL_OP_RDPTILESYNC);
            ndsRendererAdapterEmitSetTile(
                branch_dl++, G_IM_FMT_RGBA, G_IM_SIZ_4b, 0u, 0x0100u, 5u,
                0u, NDS_RENDERER_ADAPTER_G_TX_WRAP,
                NDS_RENDERER_ADAPTER_G_TX_NOMASK,
                NDS_RENDERER_ADAPTER_G_TX_NOLOD,
                NDS_RENDERER_ADAPTER_G_TX_WRAP,
                NDS_RENDERER_ADAPTER_G_TX_NOMASK,
                NDS_RENDERER_ADAPTER_G_TX_NOLOD);
            ndsRendererAdapterEmitSync(branch_dl++,
                                       NDS_FIGHTER_DL_OP_RDPLOADSYNC);
            ndsRendererAdapterEmitLoadTlut(
                branch_dl++, 5u,
                (mobj->sub.siz == G_IM_SIZ_8b) ? 0xffu : 0x0fu);
            ndsRendererAdapterEmitSync(branch_dl++,
                                       NDS_FIGHTER_DL_OP_RDPPIPESYNC);
        }
    }
    if ((flags & MOBJ_FLAG_LIGHT1) != 0)
    {
        branch_dl = ndsRendererAdapterEmitLightColor(
            branch_dl, 1u,
            ndsRendererAdapterPackColor(&mobj->sub.light1color));
    }
    if ((flags & MOBJ_FLAG_LIGHT2) != 0)
    {
        branch_dl = ndsRendererAdapterEmitLightColor(
            branch_dl, 2u,
            ndsRendererAdapterPackColor(&mobj->sub.light2color));
    }
    if ((flags & (MOBJ_FLAG_PRIMCOLOR | MOBJ_FLAG_FRAC | 0x8u)) != 0)
    {
        if ((flags & MOBJ_FLAG_FRAC) != 0)
        {
            s32 trunc = (s32)mobj->lfrac;

            ndsRendererAdapterEmitPrimColor(
                branch_dl++, mobj->sub.prim_m,
                ndsRendererAdapterClampU8F32(
                    (mobj->lfrac - (f32)trunc) * 256.0F),
                mobj->sub.primcolor.s.r,
                mobj->sub.primcolor.s.g,
                mobj->sub.primcolor.s.b,
                mobj->sub.primcolor.s.a);
            mobj->texture_id_curr = trunc;
            mobj->texture_id_next = trunc + 1;
        }
        else
        {
            ndsRendererAdapterEmitPrimColor(
                branch_dl++, mobj->sub.prim_m,
                ndsRendererAdapterClampU8F32(mobj->lfrac * 255.0F),
                mobj->sub.primcolor.s.r,
                mobj->sub.primcolor.s.g,
                mobj->sub.primcolor.s.b,
                mobj->sub.primcolor.s.a);
        }
    }
    if ((flags & MOBJ_FLAG_ENVCOLOR) != 0)
    {
        ndsRendererAdapterEmitColor(
            branch_dl++, NDS_FIGHTER_DL_OP_SETENVCOLOR,
            mobj->sub.envcolor.s.r, mobj->sub.envcolor.s.g,
            mobj->sub.envcolor.s.b, mobj->sub.envcolor.s.a);
    }
    if ((flags & MOBJ_FLAG_BLENDCOLOR) != 0)
    {
        ndsRendererAdapterEmitColor(
            branch_dl++, NDS_FIGHTER_DL_OP_SETBLENDCOLOR,
            mobj->sub.blendcolor.s.r, mobj->sub.blendcolor.s.g,
            mobj->sub.blendcolor.s.b, mobj->sub.blendcolor.s.a);
    }
    if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_SPLIT)) != 0)
    {
        s32 block_siz = (mobj->sub.block_siz == G_IM_SIZ_32b) ?
            G_IM_SIZ_32b : G_IM_SIZ_16b;

        ndsRendererAdapterEmitTextureImage(
            branch_dl++, mobj->sub.block_fmt, (u32)block_siz, 1u,
            ndsRendererAdapterReadPointerEntry(
                mobj->sub.sprites, mobj->texture_id_next));
        if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0)
        {
            u32 texels = 0u;
            u32 dxt = 0u;

            ndsRendererAdapterMaterialLoadBlock(mobj, &texels, &dxt);
            ndsRendererAdapterEmitSync(branch_dl++,
                                       NDS_FIGHTER_DL_OP_RDPLOADSYNC);
            ndsRendererAdapterEmitLoadBlock(branch_dl++, 6u, 0u, 0u,
                                            texels, dxt);
            ndsRendererAdapterEmitSync(branch_dl++,
                                       NDS_FIGHTER_DL_OP_RDPLOADSYNC);
        }
    }
    if ((flags & (MOBJ_FLAG_FRAC | MOBJ_FLAG_ALPHA)) != 0)
    {
        ndsRendererAdapterEmitTextureImage(
            branch_dl++, mobj->sub.fmt, mobj->sub.siz, 1u,
            ndsRendererAdapterReadPointerEntry(
                mobj->sub.sprites, mobj->texture_id_curr));
    }
    if ((flags & 0x20u) != 0)
    {
        if (mobj->sub.unk10 == 2)
        {
            uls = (ABSF(scau) > (1.0F / 65535.0F)) ?
                (s32)((((f32)mobj->sub.unk0C * trau) / scau) * 4.0F) : 0;
            ult = (ABSF(scav) > (1.0F / 65535.0F)) ?
                (s32)((((f32)mobj->sub.unk0E * trav) / scav) * 4.0F) : 0;
            if (uls < 0)
            {
                uls = 0;
            }
            if (ult < 0)
            {
                ult = 0;
            }
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
        ndsRendererAdapterEmitTileSize(
            branch_dl++, NDS_RENDERER_ADAPTER_G_TX_RENDERTILE, uls, ult,
            (((s32)mobj->sub.unk0C - 1) << 2) + uls,
            (((s32)mobj->sub.unk0E - 1) << 2) + ult);
    }
    if ((flags & 0x40u) != 0)
    {
        uls = (ABSF(scau) > (1.0F / 65535.0F)) ?
            (s32)(((((f32)mobj->sub.unk38 * scrollu) +
                     (f32)mobj->sub.unk0A) / scau) * 4.0F) : 0;
        ult = (ABSF(scav) > (1.0F / 65535.0F)) ?
            (s32)((((((1.0F - scav) - scrollv) *
                      (f32)mobj->sub.unk3A) +
                     (f32)mobj->sub.unk0A) / scav) * 4.0F) : 0;
        ndsRendererAdapterEmitTileSize(
            branch_dl++, 1u, uls, ult,
            (((s32)mobj->sub.unk38 - 1) << 2) + uls,
            (((s32)mobj->sub.unk3A - 1) << 2) + ult);
    }
    if ((flags & MOBJ_FLAG_TEXTURE) != 0)
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
        if (s > 0xffff)
        {
            s = 0xffff;
        }
        if (t > 0xffff)
        {
            t = 0xffff;
        }
        ndsRendererAdapterEmitTexture(
            branch_dl++, (u32)s, (u32)t, 0u,
            NDS_RENDERER_ADAPTER_G_TX_RENDERTILE,
            NDS_RENDERER_ADAPTER_G_ON);
    }

    ndsRendererAdapterEmitEndDL(branch_dl++);
    return branch_dl;
}

/* Defined beside the other graphics-heap counters in src/port/diagnostics.c.
 * Declared here rather than in a header for the same reason the decomp bodies
 * declare gSYTaskmanGraphicsHeap locally: this file has no startup header. */
extern volatile u32 gNdsTaskmanGraphicsHeapNoRoomCount;

static sb32 ndsRendererAdapterPrepareMaterialSegment(
    DObj *dobj, NDSFighterDLDrawState *state)
{
    MObj *mobj;
    Gfx *table;
    Gfx *branch_dl;
    uintptr_t heap_start;
    uintptr_t heap_end;
    uintptr_t heap_ptr;
    u32 mobj_count = 0u;
    u32 branch_commands = 0u;
    size_t heap_bytes;
    u32 i = 0u;

    if ((dobj == NULL) || (state == NULL) || (dobj->mobj == NULL))
    {
        return FALSE;
    }
    if (ndsRendererAdapterCountMaterialCommands(
            dobj, &mobj_count, &branch_commands) == FALSE)
    {
        return FALSE;
    }
    if ((mobj_count == 0u) ||
        (gSYTaskmanGraphicsHeap.ptr == NULL) ||
        (gSYTaskmanGraphicsHeap.start == NULL) ||
        (gSYTaskmanGraphicsHeap.end == NULL))
    {
        return FALSE;
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererProfileRecordMaterialOperations(mobj_count);
#endif

    heap_start = (uintptr_t)gSYTaskmanGraphicsHeap.start;
    heap_end = (uintptr_t)gSYTaskmanGraphicsHeap.end;
    heap_ptr = (uintptr_t)gSYTaskmanGraphicsHeap.ptr;
    heap_bytes = (size_t)(mobj_count + branch_commands) * sizeof(Gfx);
    if ((heap_ptr < heap_start) || (heap_ptr > heap_end))
    {
        return FALSE;
    }
    if (heap_bytes > (size_t)(heap_end - heap_ptr))
    {
        /* P2-3f9. THE ONE UNBOUNDED GRAPHICS-HEAP WRITER, MADE COUNTABLE.
         * Every other writer on this port has a source bound (see the note on
         * NDS_R2_VSBATTLE_GRAPHICS_ARENA_BYTES in battleship_scvsbattle.c);
         * this table is sized by the DObj's own material chain and so cannot
         * be bounded from a header. It has always refused rather than
         * overrun -- which is why an undersized heap shows up here as a
         * MISSING material branch and not as corruption -- but a refusal that
         * nothing counts is indistinguishable from a DObj that had no
         * materials. gNdsTaskmanGraphicsHeapOverflowCount cannot see it: the
         * pointer never passes `end`, so the sampler has nothing to report.
         * The four-CPU stress harness asserts this at 0. */
        NDS_DIAG(gNdsTaskmanGraphicsHeapNoRoomCount++);
        return FALSE;
    }

    table = (Gfx *)gSYTaskmanGraphicsHeap.ptr;
    branch_dl = table + mobj_count;
    for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next, i++)
    {
        ndsRendererAdapterEmitBranchTableCommand(&table[i], branch_dl);
        branch_dl = ndsRendererAdapterEmitMaterialCommands(branch_dl, mobj);
    }

    gSYTaskmanGraphicsHeap.ptr = branch_dl;
    state->segment_e_base = table;
    state->segment_e_end = branch_dl;
    return TRUE;
}

#if NDS_TICK_HUD && NDS_P2_EFFECT_CENSUS
/* G3 STEP 0 -- THE UNIQUE-TEMPLATE CENSUS, and it is the number a packet arena
 * is sized by. Every G3 figure banked so far counts list INSTANCES
 * (gNdsEffectDLSubmitCount: 1,360 Boundary, 527-563 gate arm); an arena sized
 * from an instance count is wrong by whatever the reuse factor is, and that
 * factor has never been measured.
 *
 * The key is the display-list pointer. That is sound WITHIN a match -- dl points
 * into a loaded-file buffer resident for the scene, the same property G1's
 * texture-site memo relies on -- and it is NOT sound across one: charter 3.12,
 * the taskman arena rewinds and hands the next scene the same addresses. A
 * builder sized by this census must re-derive at scene entry. The census only
 * has to survive the window it measures, and P1 boots straight into one match.
 *
 * StateVariants/CommandVariants are the feasibility guards: if one dl is
 * submitted under two different entry blend modes or yields two different
 * command counts, then "one packet per unique dl" is not a complete key and the
 * arena needs more entries than Unique. They must be read before Unique is
 * trusted as the sizing input.
 *
 * Called from the epilogue, OUTSIDE the Exec bracket that closes above it, so
 * ticks/list stays the interpreter's own cost rather than the census's. */
#define NDS_EFFECT_DL_CENSUS_CAPACITY 256u

static const Gfx *sNdsEffectDLCensusKey[NDS_EFFECT_DL_CENSUS_CAPACITY];
static u32 sNdsEffectDLCensusOtherMode[NDS_EFFECT_DL_CENSUS_CAPACITY];
static u32 sNdsEffectDLCensusCommands[NDS_EFFECT_DL_CENSUS_CAPACITY];
/* PER-TEMPLATE GEOMETRY, AND IT MUST BE THE MAX RATHER THAN THE FIRST SIGHTING.
 * hardware_triangle_count is a POST-CULL count, so the same template submits
 * different geometry on different frames as it moves through the frustum. A
 * packet has to encode the template's whole content, so the sizing input is the
 * largest submission ever seen, not a sample of one. GeomVariants says whether
 * culling moves it at all: 0 means the geometry is frame-invariant and the max
 * is exact; non-zero means the max is the honest lower bound on static content
 * and the arena wants margin over it. */
static u32 sNdsEffectDLCensusTrisMax[NDS_EFFECT_DL_CENSUS_CAPACITY];
static u32 sNdsEffectDLCensusVertsMax[NDS_EFFECT_DL_CENSUS_CAPACITY];

static void ndsEffectDLCensusRecord(const Gfx *dl, u32 commands,
                                    u32 othermode_in, u32 tris, u32 verts)
{
    u32 count = gNdsEffectDLCensusUnique;
    u32 i;

    for (i = 0u; i < count; i++)
    {
        if (sNdsEffectDLCensusKey[i] == dl)
        {
            if (sNdsEffectDLCensusOtherMode[i] != othermode_in)
            {
                gNdsEffectDLCensusStateVariants++;
            }
            if (sNdsEffectDLCensusCommands[i] != commands)
            {
                gNdsEffectDLCensusCommandVariants++;
            }
            if (tris != sNdsEffectDLCensusTrisMax[i])
            {
                gNdsEffectDLCensusGeomVariants++;
            }
            /* Totals are maintained incrementally so the report never needs a
             * second pass over the table. */
            if (tris > sNdsEffectDLCensusTrisMax[i])
            {
                gNdsEffectDLCensusTrisMaxTotal +=
                    tris - sNdsEffectDLCensusTrisMax[i];
                sNdsEffectDLCensusTrisMax[i] = tris;
            }
            if (verts > sNdsEffectDLCensusVertsMax[i])
            {
                gNdsEffectDLCensusVertsMaxTotal +=
                    verts - sNdsEffectDLCensusVertsMax[i];
                sNdsEffectDLCensusVertsMax[i] = verts;
            }
            return;
        }
    }
    if (count >= NDS_EFFECT_DL_CENSUS_CAPACITY)
    {
        /* Overflow is reported, never silently truncated: a capped unique count
         * reads exactly like a small one and would size the arena short. */
        gNdsEffectDLCensusOverflow++;
        return;
    }
    sNdsEffectDLCensusKey[count] = dl;
    sNdsEffectDLCensusOtherMode[count] = othermode_in;
    sNdsEffectDLCensusCommands[count] = commands;
    sNdsEffectDLCensusTrisMax[count] = tris;
    sNdsEffectDLCensusVertsMax[count] = verts;
    gNdsEffectDLCensusUniqueCommandTotal += commands;
    gNdsEffectDLCensusTrisMaxTotal += tris;
    gNdsEffectDLCensusVertsMaxTotal += verts;
    if (commands > gNdsEffectDLCensusCommandMax)
    {
        gNdsEffectDLCensusCommandMax = commands;
    }
    gNdsEffectDLCensusUnique = count + 1u;
}

/* G3 STEP 1 -- the per-template verdict on the captured GX stream. The capture
 * itself is in nds_renderer.c, hooked into the GX record funnel; this is the
 * comparison, and it deliberately reuses the census's own key so the two answer
 * for exactly the same template population.
 *
 * 32 entries against a measured 8 uniques is 4x margin, and the overflow is
 * counted rather than wrapped: a table that silently dropped a template would
 * report perfect agreement for the ones it kept. */
#define NDS_EFFECT_PACKET_TEMPLATE_CAPACITY 32u

static const Gfx *sNdsEffectPacketKey[NDS_EFFECT_PACKET_TEMPLATE_CAPACITY];
static u32 sNdsEffectPacketGeomHashSeen[NDS_EFFECT_PACKET_TEMPLATE_CAPACITY];
static u32 sNdsEffectPacketColorHashSeen[NDS_EFFECT_PACKET_TEMPLATE_CAPACITY];
static u32 sNdsEffectPacketMatrixHashSeen[NDS_EFFECT_PACKET_TEMPLATE_CAPACITY];
static u32 sNdsEffectPacketGeomWordsSeen[NDS_EFFECT_PACKET_TEMPLATE_CAPACITY];

static void ndsEffectPacketVerdictRecord(const Gfx *dl)
{
    u32 count = gNdsEffectPacketTemplates;
    u32 i;

    for (i = 0u; i < count; i++)
    {
        if (sNdsEffectPacketKey[i] != dl)
        {
            continue;
        }
        if (sNdsEffectPacketGeomHashSeen[i] == gNdsEffectPacketGeomHash)
        {
            NDS_DIAG(gNdsEffectPacketGeomMatchCount++);
        }
        else
        {
            NDS_DIAG(gNdsEffectPacketGeomVariantCount++);
        }
        if (sNdsEffectPacketGeomWordsSeen[i] != gNdsEffectPacketGeomWords)
        {
            /* Separate from the hash verdict on purpose: a stream that changed
             * LENGTH is a different failure from one that changed VALUES, and
             * only the second is a candidate for a patch table. */
            NDS_DIAG(gNdsEffectPacketGeomWordVariantCount++);
        }
        if (sNdsEffectPacketColorHashSeen[i] == gNdsEffectPacketColorHash)
        {
            NDS_DIAG(gNdsEffectPacketColorMatchCount++);
        }
        else
        {
            NDS_DIAG(gNdsEffectPacketColorVariantCount++);
        }
        if (sNdsEffectPacketMatrixHashSeen[i] == gNdsEffectPacketMatrixHash)
        {
            NDS_DIAG(gNdsEffectPacketMatrixMatchCount++);
        }
        else
        {
            NDS_DIAG(gNdsEffectPacketMatrixVariantCount++);
        }
        return;
    }
    if (count >= NDS_EFFECT_PACKET_TEMPLATE_CAPACITY)
    {
        NDS_DIAG(gNdsEffectPacketTableOverflow++);
        return;
    }
    sNdsEffectPacketKey[count] = dl;
    sNdsEffectPacketGeomHashSeen[count] = gNdsEffectPacketGeomHash;
    sNdsEffectPacketColorHashSeen[count] = gNdsEffectPacketColorHash;
    sNdsEffectPacketMatrixHashSeen[count] = gNdsEffectPacketMatrixHash;
    sNdsEffectPacketGeomWordsSeen[count] = gNdsEffectPacketGeomWords;
    gNdsEffectPacketTemplates = count + 1u;
}
#endif

#if NDS_ENTRY_EFFECT_DIAG
/* P2-3r6: WHERE THE PIPE BODY'S Y COMES FROM.
 *
 * The renderer already records the modelview each entry-effect root is
 * submitted under, and it says the body (root 0x04c0) sits ~300 units above the
 * rim (0x03c0, a constant -21) for the whole visible life of the effect. That
 * is either what the source animation wrote into the DObj, or something this
 * adapter's matrix build introduced. Recording the DObj's own transform here,
 * beside the matrix build, separates the two without another guess.
 *
 * Floats are stored as their bit patterns: the gdb stub reads globals reliably
 * and the host can decode, whereas printing a float through the stub has
 * already produced one misleading 0.000000. */
volatile u32 gNdsEntryEffectDObjTranslate[2][3];
volatile u32 gNdsEntryEffectDObjScale[2][3];
volatile u32 gNdsEntryEffectDObjRotate[2][3];
volatile u32 gNdsEntryEffectDObjParent[2];
volatile u32 gNdsEntryEffectDObjParentTranslate[2][3];
/* The animation clock beside the value it produced: if the body's translate
 * track is right but its PLAYBACK is twice the source rate, the body leaves in
 * half the frames and the pipe reads as "body missing". anim_speed is what
 * `aobj->length += dobj->anim_speed` advances by, so it is the rate itself. */
volatile u32 gNdsEntryEffectDObjAnim[2][3];
volatile u32 gNdsEntryEffectGObjAnimFrame[2];
/* The DObj address itself, so the AObj track list can be walked from the
 * HOST. Walking it in guest code inside the draw is what this file tried
 * first; one word here and gdb does the rest. */
volatile u32 gNdsEntryEffectDObjPtr[2];

static void ndsEntryEffectDiagRecordDObj(u32 root_offset, DObj *dobj)
{
    u32 slot;
    DObj *parent;

    if (root_offset == 0x03c0u)
    {
        slot = 0u;
    }
    else if (root_offset == 0x04c0u)
    {
        slot = 1u;
    }
    else
    {
        return;
    }
    if (dobj == NULL)
    {
        return;
    }
    gNdsEntryEffectDObjTranslate[slot][0] = *(const u32 *)&dobj->translate.vec.f.x;
    gNdsEntryEffectDObjTranslate[slot][1] = *(const u32 *)&dobj->translate.vec.f.y;
    gNdsEntryEffectDObjTranslate[slot][2] = *(const u32 *)&dobj->translate.vec.f.z;
    gNdsEntryEffectDObjScale[slot][0] = *(const u32 *)&dobj->scale.vec.f.x;
    gNdsEntryEffectDObjScale[slot][1] = *(const u32 *)&dobj->scale.vec.f.y;
    gNdsEntryEffectDObjScale[slot][2] = *(const u32 *)&dobj->scale.vec.f.z;
    gNdsEntryEffectDObjRotate[slot][0] = *(const u32 *)&dobj->rotate.vec.f.x;
    gNdsEntryEffectDObjRotate[slot][1] = *(const u32 *)&dobj->rotate.vec.f.y;
    gNdsEntryEffectDObjRotate[slot][2] = *(const u32 *)&dobj->rotate.vec.f.z;
    gNdsEntryEffectDObjPtr[slot] = (u32)(uintptr_t)dobj;
    gNdsEntryEffectDObjAnim[slot][0] = *(const u32 *)&dobj->anim_speed;
    gNdsEntryEffectDObjAnim[slot][1] = *(const u32 *)&dobj->anim_wait;
    gNdsEntryEffectDObjAnim[slot][2] = *(const u32 *)&dobj->anim_frame;
    if (dobj->parent_gobj != NULL)
    {
        gNdsEntryEffectGObjAnimFrame[slot] =
            *(const u32 *)&dobj->parent_gobj->anim_frame;
    }
    parent = dobj->parent;
    gNdsEntryEffectDObjParent[slot] = (u32)(uintptr_t)parent;
    if (parent != NULL)
    {
        gNdsEntryEffectDObjParentTranslate[slot][0] =
            *(const u32 *)&parent->translate.vec.f.x;
        gNdsEntryEffectDObjParentTranslate[slot][1] =
            *(const u32 *)&parent->translate.vec.f.y;
        gNdsEntryEffectDObjParentTranslate[slot][2] =
            *(const u32 *)&parent->translate.vec.f.z;
    }
}
#endif

static void ndsStageRejectNativeRender(DObj *dobj, const Gfx *dl,
    u32 reason, NDSRendererStats *stats)
{
    NDSRelocLoadedFile *loaded = (dl != NULL) ?
        ndsRelocFindLoadedFileContaining(dl, sizeof(*dl)) : NULL;
    u32 kind = ((dobj != NULL) && (dobj->parent_gobj != NULL)) ?
        dobj->parent_gobj->id : 0xffffu;
    u32 identity = (kind << 16) |
        ((loaded != NULL) ? (loaded->asset_id & 0xffffu) : 0xffffu);

    ndsRendererRecordNativeFailure(NDS_NATIVE_FAILURE_STAGE,
        (u32)gSCManagerSceneData.scene_curr, identity,
        (gSCManagerBattleState != NULL) ? (u32)gSCManagerBattleState->gkind : 0xffffu,
        (loaded != NULL) ? ndsRelocNativeRootOffset(loaded, dl) :
            (u32)(uintptr_t)dl,
        (dobj != NULL) ? (u32)(uintptr_t)dobj->mobj : 0u, reason);
    if (stats != NULL)
    {
        stats->blocker = NDS_RENDERER_BLOCKER_UNSUPPORTED;
    }
}

#if NDS_RENDERER_HW_TRIANGLES
/* One list's matrices, prepared ahead of its submit (Sector Z's Arwing in two
 * passes, ndsRendererAdapterSubmitArwingTwoPass): exactly what
 * ndsRendererAdapterPrepareInitialMatrices returned for this DObj, before the
 * identity fill. Set only around that list's submit. */
typedef struct NDSRendererAdapterEntryPrepared
{
    DObj *dobj;
    const Gfx *dl;
    u32 list_id;
    u32 prepared;
    u32 projection_valid;
    u32 modelview_valid;
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;
} NDSRendererAdapterEntryPrepared;

static const NDSRendererAdapterEntryPrepared *sNdsRendererAdapterEntryPrepared;
#endif

#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
/* LAB ONLY (the any-stage sweep ROM): the FoxSpecial3 Arwing roots this
 * owner draws (Sector Z's hazard, Fox's entry), in the order of the root
 * switch below. Per root: calls, ticks from entry through
 * PrepareInitialMatrices, the executor's ticks, the rest (stats, config,
 * tail), triangles. A call spanning >= 2^20 ticks (the clock's 2^22
 * artifact) is left out. */
volatile u32 gNdsLabArwingRootCensus[8][5] __attribute__((used));
#define NDS_LAB_ARW_MARK(v) ((v) = cpuGetTiming())
#else
#define NDS_LAB_ARW_MARK(v) ((void)0)
#endif

#if NDS_RENDERER_HW_TRIANGLES
/* The entry owner's inherited state, as the executor's stats see it at
 * submit; shared by ndsRendererAdapterTryNativeEntryEffect and the Arwing's
 * replay submit so the two cannot drift. */
static inline __attribute__((always_inline)) void
ndsRendererAdapterEntryEffectSeedStats(DObj *dobj, NDSRendererStats *stats)
{
    ndsRendererInitStats(stats);
    /* Light state. Both lists inherit G_LIGHTING from the battle display and
     * carry their own gSPLightColor words in the packet; what they do not
     * carry is seeded from what the display left in the RSP: the direction
     * scVSBattleFuncLights aimed, re-aimed by the fighter's own
     * ftDisplayLightsDrawReflect when it uses a light (same seed as
     * ndsRendererAdapterBeginStageTraversal), and for a group before the
     * first colour word the last colours written: the effect's own MObj
     * colours when it carries MOBJ_FLAG_LIGHT1/2, otherwise the fighter
     * material drawn before it. */
    if ((sNdsFighterDisplayCurrentLightValid != FALSE) &&
        (sNdsFighterDisplayCurrentLightCount != 0u))
    {
        stats->light_dir_x = sNdsFighterDisplayCurrentLight.l.dir[0];
        stats->light_dir_y = sNdsFighterDisplayCurrentLight.l.dir[1];
        stats->light_dir_z = sNdsFighterDisplayCurrentLight.l.dir[2];
        stats->light_dir_mask = 1u;
    }
    {
        MObj *mobj;

        for (mobj = dobj->mobj; mobj != NULL; mobj = mobj->next)
        {
            u32 flags = mobj->sub.flags;

            if ((flags & MOBJ_FLAG_LIGHT1) != 0u)
            {
                stats->light_color_1 =
                    ndsRendererAdapterPackColor(&mobj->sub.light1color);
                stats->light_color_mask |=
                    NDS_FIGHTER_DISPLAY_LIGHT_COLOR_1_MASK;
            }
            if ((flags & MOBJ_FLAG_LIGHT2) != 0u)
            {
                stats->light_color_2 =
                    ndsRendererAdapterPackColor(&mobj->sub.light2color);
                stats->light_color_mask |=
                    NDS_FIGHTER_DISPLAY_LIGHT_COLOR_2_MASK;
            }
        }
        if (gNdsFighterDisplayContractMaterialLightSeedCount != 0u)
        {
            if ((stats->light_color_mask &
                 NDS_FIGHTER_DISPLAY_LIGHT_COLOR_1_MASK) == 0u)
            {
                stats->light_color_1 =
                    gNdsFighterDisplayContractMaterialLight1;
                stats->light_color_mask |=
                    NDS_FIGHTER_DISPLAY_LIGHT_COLOR_1_MASK;
            }
            if ((stats->light_color_mask &
                 NDS_FIGHTER_DISPLAY_LIGHT_COLOR_2_MASK) == 0u)
            {
                stats->light_color_2 =
                    gNdsFighterDisplayContractMaterialLight2;
                stats->light_color_mask |=
                    NDS_FIGHTER_DISPLAY_LIGHT_COLOR_2_MASK;
            }
        }
    }
    if (sNdsRendererAdapterEffectSubmitActive != FALSE)
    {
        if ((sNdsRendererAdapterEffectColorMask & 1u) != 0u)
        {
            stats->prim_color = sNdsRendererAdapterEffectPrimColor;
        }
        if ((sNdsRendererAdapterEffectColorMask & 2u) != 0u)
        {
            stats->env_color = sNdsRendererAdapterEffectEnvColor;
        }
        if (sNdsRendererAdapterEffectOtherModeValid != FALSE)
        {
            stats->othermode_l = sNdsRendererAdapterEffectOtherModeL;
        }
        /* A list-1 draw whose proc set no mode inherits the battle camera's
         * own XLU head (gmcamera.c:1055 writes G_RM_AA_ZB_XLU_SURF into
         * gSYTaskmanDLHeads[1] before every layer), not the last effect's. */
        if ((sNdsRendererAdapterEffectOtherModeThisProc == 0u) &&
            (sNdsRendererAdapterEffectSubmitHead == 1u))
        {
            stats->othermode_l = G_RM_AA_ZB_XLU_SURF | G_RM_AA_ZB_XLU_SURF2;
        }
    }
}

static inline __attribute__((always_inline)) void
ndsRendererAdapterEntryEffectAccumulate(const NDSRendererStats *stats)
{
    gNdsStageGCDrawAllLoopHardwareTriangleCount += stats->hardware_triangle_count;
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
}
#endif

static sb32 ndsRendererAdapterTryNativeEntryEffect(
    DObj *dobj, const Gfx *dl, GObj *camera_gobj, u32 initial_geometry_mode)
{
#if NDS_RENDERER_HW_TRIANGLES
    extern void *gFTManagerCommonFile;
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    u32 lab_t[5] = { 0u, 0u, 0u, 0u, 0u };
#endif
    const u8 *base = NULL;
    u32 owner_asset_id = 0u;
    u32 root_offset = 0u;
    sb32 candidate = FALSE;
    /* Zeroed where it is filled: every stage list asks here first and almost
     * none is an entry prop, so an initialiser was a memset per list (the
     * ending room's DObjs paid it on every draw). */
    NDSRendererConfig config;
    NDSRendererStats stats;
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;
    const NDSRendererMatrix20p12 *projection_ptr;
    const NDSRendererMatrix20p12 *modelview_ptr;
#if NDS_P2_LINK
    NDSRendererNativeMaterial link_special2_material;
    NDSRendererNativeMaterial link_spin_materials[9];
#endif
    /* EFCommonEffects3 needs at most two live MObj snapshots per root:
     * MBallRays takes two per ray fan, ItemGetSwirl one per drawable child. */
    NDSRendererNativeMaterial efcommon3_materials[2];
    /* Every owner passes these to the native prepare; only Link's two
     * material-snapshot arms fill them, so the pair lives outside his flag. */
    const NDSRendererNativeMaterial *native_materials = NULL;
    u32 native_material_count = 0u;
    NDSRendererNativeMaterial common_effect_material;
    u32 native_texture_variant = 0xffffffffu;

    if ((dobj == NULL) || (dl == NULL))
    {
        return FALSE;
    }
    NDS_LAB_ARW_MARK(lab_t[0]);

    /* Exact source asset + exact generated root is the whole admission test.
     * Do not classify arbitrary effect lists by shape: this path intentionally
     * owns only entry props that the offline source bake emitted. */
    if (gEFManagerFiles[1] != NULL)
    {
        const uintptr_t address = (uintptr_t)dl;
        const uintptr_t effect_base = (uintptr_t)gEFManagerFiles[1];
        if ((address == effect_base + 0x2500u) ||
            (address == effect_base + 0x2588u) ||
            (address == effect_base + 0x2610u) ||
            (address == effect_base + 0x2698u) ||
            (address == effect_base + 0x5218u) ||
            (address == effect_base + 0x52b0u) ||
            (address == effect_base + 0x5310u) ||
            (address == effect_base + 0x31d0u) ||
            (address == effect_base + 0x3258u) ||
            (address == effect_base + 0x32e0u))
        {
            base = (const u8 *)effect_base;
            root_offset = (u32)(address - effect_base);
            owner_asset_id = 84u;
            candidate = TRUE;
        }
    }
    if ((candidate == FALSE) && (gFTManagerCommonFile != NULL) &&
        ((uintptr_t)dl == (uintptr_t)gFTManagerCommonFile + 0x0248u))
    {
        base = (const u8 *)gFTManagerCommonFile;
        root_offset = 0x0248u;
        owner_asset_id = 163u;
        candidate = TRUE;
    }
    if ((candidate == FALSE) && (gFTDataFoxSpecial2 != NULL) &&
        ((uintptr_t)dl == (uintptr_t)gFTDataFoxSpecial2 + 0x01b8u))
    {
        base = (const u8 *)gFTDataFoxSpecial2;
        root_offset = 0x01b8u;
        owner_asset_id = 346u;
        candidate = TRUE;
    }
#if NDS_P2_YOSHI
    /* Yoshi's egg. One source list serves two states that both hide his whole
     * body on purpose: the shield (efmanager.c:490) and the egg-hatching intro
     * (:1375) name the same DObj setup field. The ordinary shield arm above
     * cannot cover him, because ftcommonguard1.c:387-397 and
     * ftcommonguard2.c:20-26 are an if/else on `fkind == nFTKindYoshi` that
     * sends him to efManagerYoshiShieldMakeEffect instead. Guarded because
     * gFTDataYoshiModel lives in ftyoshi.c, which is not built at every roster.
     * The per-frame env fade from fp->shield_health stays runtime-owned. */
    if ((candidate == FALSE) && (gFTDataYoshiModel != NULL) &&
        ((uintptr_t)dl == (uintptr_t)gFTDataYoshiModel + 0xa860u))
    {
        base = (const u8 *)gFTDataYoshiModel;
        root_offset = 0xa860u;
        owner_asset_id = 338u;
        candidate = TRUE;
    }
#endif
    if ((candidate == FALSE) && (gFTMarioFileSpecial2 != NULL) &&
        ((const u8 *)dl >= (const u8 *)gFTMarioFileSpecial2))
    {
        base = (const u8 *)gFTMarioFileSpecial2;
        root_offset = (u32)((const u8 *)dl - base);
        if ((root_offset == 0x03c0u) || (root_offset == 0x04c0u))
        {
            owner_asset_id = 356u;
            candidate = TRUE;
        }
    }
#if NDS_P2_LUIGI
    /* dFTLuigiData points its Special2 slot at llMarioSpecial2FileID exactly
     * like Mario, but ftManager owns a separate destination pointer for each
     * fighter kind.  In a Luigi-vs-Fox match gFTMarioFileSpecial2 is therefore
     * legitimately NULL while gFTDataLuigiSpecial2 contains the same source
     * asset.  Admit that second live base explicitly; otherwise Luigi's pipe
     * falls back to the N64 interpreter even though the AOT packet is already
     * an exact bake of file 356. */
    if ((candidate == FALSE) && (gFTDataLuigiSpecial2 != NULL) &&
        ((const u8 *)dl >= (const u8 *)gFTDataLuigiSpecial2))
    {
        base = (const u8 *)gFTDataLuigiSpecial2;
        root_offset = (u32)((const u8 *)dl - base);
        if ((root_offset == 0x03c0u) || (root_offset == 0x04c0u))
        {
            owner_asset_id = 356u;
            candidate = TRUE;
        }
    }
#endif
    /* Not only Fox's entry: Sector Z's own Arwing is a GROUND object drawing
     * the very same FoxSpecial3 list (GRSectorMap pulls file 161 in as an
     * external dependency, with or without Fox in the match), so this owner is
     * its renderer too. See the texture-lifetime note in
     * battleship_scvsbattle.c. */
    if ((candidate == FALSE) && (gFTDataFoxSpecial3 != NULL) &&
        ((const u8 *)dl >= (const u8 *)gFTDataFoxSpecial3))
    {
        base = (const u8 *)gFTDataFoxSpecial3;
        root_offset = (u32)((const u8 *)dl - base);
        switch (root_offset)
        {
        case 0x1fa0u:
        case 0x2920u:
        case 0x29d0u:
        case 0x29f0u:
        case 0x2a20u:
        case 0x2868u:
        case 0x2a50u:
        case 0x2b00u:
            owner_asset_id = 161u;
            candidate = TRUE;
            break;
        default:
            break;
        }
    }
#if NDS_P2_DONKEY
    /* BattleShip dEFManagerDonkeyEntryTaruEffectDesc uses DonkeySpecial2 and
     * gcDrawDObjTreeForGObj. Its DObjDesc at 0x07c8 contains one drawable child
     * whose exact source display-list root is 0x0620. Keep the source tree and
     * animation live; replace only that immutable N64 DL/texture work. */
    if ((candidate == FALSE) && (gFTDataDonkeySpecial2 != NULL) &&
        ((const u8 *)dl >= (const u8 *)gFTDataDonkeySpecial2))
    {
        base = (const u8 *)gFTDataDonkeySpecial2;
        root_offset = (u32)((const u8 *)dl - base);
        if (root_offset == 0x0620u)
        {
            owner_asset_id = 355u;
            candidate = TRUE;
        }
    }
#endif
#if NDS_P2_SAMUS
    /* BattleShip dEFManagerSamusEntryPointEffectDesc owns one animated child
     * whose DObjDLLink submits two immutable source lists (links 0 and 1).
     * Keep the source DObj tree and EntryPoint AnimJoint live -- notably its
     * near-zero -> full -> near-zero Y scale that opens/closes the point -- and
     * replace only those two N64 Gfx streams with their generated DS packets. */
    if ((candidate == FALSE) && (gFTDataSamusSpecial2 != NULL) &&
        ((const u8 *)dl >= (const u8 *)gFTDataSamusSpecial2))
    {
        base = (const u8 *)gFTDataSamusSpecial2;
        root_offset = (u32)((const u8 *)dl - base);
        if ((root_offset == 0x0930u) || (root_offset == 0x0ad0u) ||
            ((root_offset == 0x02e0u) &&
             (sNdsRendererAdapterEffectSubmitActive != FALSE) &&
             (dobj->parent_gobj != NULL) &&
             (dobj->parent_gobj->id == nGCCommonKindEffect) &&
             (dobj->mobj != NULL)))
        {
            owner_asset_id = 349u;
            candidate = TRUE;
        }
    }
#endif

    if ((owner_asset_id == 349u) && (root_offset == 0x02e0u))
    {
        MObj *mobj = dobj->mobj;
        s32 texture_id_curr = -1;
        s32 texture_id_next = -1;

        /* Catch's grapple glow owns exactly one ALPHA material. Segment-E
         * supplies only the current source image; MatAnim loops TEXID 0/1.
         * Preserve that live selector while the generated owner supplies the
         * immutable tile/load/combine/geometry and preconverted texture pair. */
        bzero(&common_effect_material, sizeof(common_effect_material));
        if ((mobj == NULL) || (mobj->next != NULL) ||
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 mobj, &common_effect_material, FALSE,
                 &texture_id_curr, &texture_id_next) == FALSE) ||
            (common_effect_material.effects !=
                 NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE) ||
            (texture_id_curr < 0) || (texture_id_curr > 1) ||
            (texture_id_next < 0) || (texture_id_next > 1))
        {
            NDS_DIAG(gNdsEntryEffectNativeFallbackCount++);
            return FALSE;
        }
        native_materials = &common_effect_material;
        native_material_count = 1u;
        native_texture_variant = (u32)texture_id_curr;
    }
#if NDS_P2_KIRBY
    /* Final Cutter's Draw/Trail/Up/Down effects are source-owned DObj trees
     * from KirbySpecial2. None has an MObj or MatAnimJoint; Draw attaches to
     * fighter joint 17, while Trail/Up/Down keep their source AnimJoints. Keep
     * those live attachments/transforms and replace only the exact immutable
     * Gfx/texture roots emitted by their source descriptors. */
    if ((candidate == FALSE) &&
        (sNdsRendererAdapterEffectSubmitActive != FALSE) &&
        (gFTDataKirbySpecial2 != NULL) &&
        (dobj->parent_gobj != NULL) &&
        (dobj->parent_gobj->id == nGCCommonKindEffect) &&
        (dobj->mobj == NULL))
    {
        const uintptr_t address = (uintptr_t)dl;
        const uintptr_t kirby_special2_base = (uintptr_t)gFTDataKirbySpecial2;

        base = (const u8 *)gFTDataKirbySpecial2;
        root_offset = ((address >= kirby_special2_base) &&
                       (address < kirby_special2_base + 0x2960u)) ?
            (u32)(address - kirby_special2_base) : 0xffffffffu;
        switch (root_offset)
        {
        case 0x27a0u: /* Draw */
        case 0x0c70u: /* Trail */
        case 0x0ce0u:
        case 0x11b0u: /* Up */
        case 0x1218u:
        case 0x1280u:
        case 0x2210u: /* Down */
        case 0x2270u:
        case 0x22d0u:
        case 0x2330u:
        case 0x1cf8u: /* EntryStar: the warp star of Kirby's intro */
            owner_asset_id = 348u;
            candidate = TRUE;
            break;
        default:
            break;
        }
    }

    /* Final Cutter's travelling weapon is a different source owner from the
     * KirbySpecial2 effects above. BattleShip dWPKirbyCutterWeaponDesc points
     * at KirbyMain+0x08 WPAttributes, whose relocated DObjDesc lives in
     * KirbyModel at 0x1D388 and submits these two immutable lists in order.
     * Keep weapon physics/collision and the live source DObj tree; specialize
     * only this exact nWPKindCutter geometry. */
    if ((candidate == FALSE) &&
        (dobj->parent_gobj != NULL) &&
        (dobj->parent_gobj->id == nGCCommonKindWeapon) &&
        (dobj->mobj == NULL))
    {
        WPStruct *cutter_wp = wpGetStruct(dobj->parent_gobj);
        NDSRelocLoadedFile *cutter_model = NULL;

        if ((cutter_wp != NULL) && (cutter_wp->kind == nWPKindCutter))
        {
            /* Compact battle fighters intentionally do not publish the raw
             * KirbyModel through gFTDataKirbyModel. The source weapon does not
             * need that slot: KirbyMain+0x08's relocated WPAttributes already
             * point its live DObjDesc into the loaded asset-328 closure. Follow
             * that authoritative relocated pointer instead of requiring an
             * unrelated global publication. */
            cutter_model = ndsRelocFindLoadedFileContaining(dl, sizeof(*dl));
            if ((cutter_model != NULL) &&
                (cutter_model->asset_id == 328u) &&
                (cutter_model->data != NULL))
            {
                root_offset = ndsRelocNativeRootOffset(cutter_model, dl);
                if ((root_offset == 0x1d238u) || (root_offset == 0x1d308u))
                {
                    base = (const u8 *)cutter_model->data;
                    owner_asset_id = 328u;
                    candidate = TRUE;
                }
            }
        }
    }
#endif
#if NDS_P2_LINK
    /* BattleShip's Link entry wave/beam and attached grounded Spin EFFECT each
     * own a live animated DObj in LinkSpecial2. The collision weapon is a
     * different owner (LinkModel below); do not conflate its MatAnim payload
     * with weapon geometry again. */
    if ((candidate == FALSE) && (gFTDataLinkSpecial2 != NULL) &&
        ((const u8 *)dl >= (const u8 *)gFTDataLinkSpecial2))
    {
        base = (const u8 *)gFTDataLinkSpecial2;
        root_offset = (u32)((const u8 *)dl - base);
        if ((root_offset == 0x02d8u) || (root_offset == 0x0698u) ||
            (root_offset == 0x1100u))
        {
            owner_asset_id = 353u;
            candidate = TRUE;
        }
    }
    /* Grounded Spin's source WPAttributes live in LinkMain but resolve their
     * DObj/MObj/Anim/MatAnim pointers into LinkModel. The drawable child's
     * DObjDLLink at +0x118f8 submits exactly LinkModel+0x11680. Its segment-E
     * calls select the nine live MObjs built by gcDrawMObjForDObj. */
    if ((candidate == FALSE) &&
        (dobj->parent_gobj != NULL) &&
        (dobj->parent_gobj->id == nGCCommonKindWeapon))
    {
        WPStruct *spin_wp = wpGetStruct(dobj->parent_gobj);
        NDSRelocLoadedFile *spin_model = NULL;

        if ((spin_wp != NULL) && (spin_wp->kind == nWPKindSpinAttack))
        {
            /* Compact battle packs publish the WPAttributes closure, not the
             * raw gFTDataLinkModel slot. Follow the weapon's relocated source
             * root; the native material owner still requires all nine MObjs. */
            spin_model = ndsRelocFindLoadedFileContaining(dl, sizeof(*dl));
            if ((spin_model != NULL) && (spin_model->asset_id == 324u) &&
                (spin_model->data != NULL))
            {
                root_offset = ndsRelocNativeRootOffset(spin_model, dl);
                if (root_offset == 0x11680u)
                {
                    base = (const u8 *)spin_model->data;
                    owner_asset_id = 324u;
                    candidate = TRUE;
                }
            }
        }
    }
    /* Boomerang's source DObj tree and six-tick rotation loop stay live. Its
     * two drawable children submit these exact LinkSpecial3 wrapper roots.
     * Compact battle packing intentionally does not require the raw
     * gFTDataLinkSpecial3 publication, so follow the live weapon's relocated
     * DL back to its authoritative loaded file just like Final Cutter above. */
    if ((candidate == FALSE) &&
        (dobj->parent_gobj != NULL) &&
        (dobj->parent_gobj->id == nGCCommonKindWeapon))
    {
        WPStruct *boomerang_wp = wpGetStruct(dobj->parent_gobj);
        NDSRelocLoadedFile *boomerang_file = NULL;

        if ((boomerang_wp != NULL) && (boomerang_wp->kind == nWPKindBoomerang))
        {
            boomerang_file = ndsRelocFindLoadedFileContaining(dl, sizeof(*dl));
            if ((boomerang_file != NULL) &&
                (boomerang_file->asset_id == 325u) &&
                (boomerang_file->data != NULL))
            {
                root_offset = ndsRelocNativeRootOffset(boomerang_file, dl);
                if ((root_offset == 0x0458u) || (root_offset == 0x0580u))
                {
                    base = (const u8 *)boomerang_file->data;
                    owner_asset_id = 325u;
                    candidate = TRUE;
                }
            }
        }
    }
#endif
#if NDS_P2_CAPTAIN
    /* Falcon Kick/Punch are ordinary source EFDesc GObjs, not entry-car
     * geometry. Keep their live attachment/animation/MObj state and admit only
     * the exact immutable source roots generated for this package. */
    if ((candidate == FALSE) &&
        (sNdsRendererAdapterEffectSubmitActive != FALSE) &&
        (gFTDataCaptainSpecial2 != NULL) &&
        (dobj->parent_gobj != NULL) &&
        (dobj->parent_gobj->id == nGCCommonKindEffect) &&
        (dobj->mobj != NULL) &&
        ((const u8 *)dl >= (const u8 *)gFTDataCaptainSpecial2))
    {
        base = (const u8 *)gFTDataCaptainSpecial2;
        root_offset = (u32)((const u8 *)dl - base);
        if (root_offset == 0x0a30u)
        {
            owner_asset_id = 350u;
            candidate = TRUE;
        }
    }
    if ((candidate == FALSE) &&
        (sNdsRendererAdapterEffectSubmitActive != FALSE) &&
        (gFTDataCaptainSpecial3 != NULL) &&
        (dobj->parent_gobj != NULL) &&
        (dobj->parent_gobj->id == nGCCommonKindEffect) &&
        (dobj->mobj != NULL) &&
        ((const u8 *)dl >= (const u8 *)gFTDataCaptainSpecial3))
    {
        base = (const u8 *)gFTDataCaptainSpecial3;
        root_offset = (u32)((const u8 *)dl - base);
        if (root_offset == 0x0760u)
        {
            owner_asset_id = 333u;
            candidate = TRUE;
        }
    }

    /* BattleShip dEFManagerCaptainEntryCarEffectDesc owns a live 13-node DObj
     * tree. Its 0x6200 main AnimJoint plus 0x6518/0x6598 child animations remain
     * source-owned; only the ten immutable CaptainSpecial2 Gfx roots below are
     * replaced by the exact generated DS packets. */
    if ((candidate == FALSE) && (gFTDataCaptainSpecial2 != NULL) &&
        ((const u8 *)dl >= (const u8 *)gFTDataCaptainSpecial2))
    {
        base = (const u8 *)gFTDataCaptainSpecial2;
        root_offset = (u32)((const u8 *)dl - base);
        switch (root_offset)
        {
        case 0x5690u:
        case 0x5c60u:
        case 0x5d20u:
        case 0x5d50u:
        case 0x5d80u:
        case 0x5db0u:
        case 0x5de0u:
        case 0x5e10u:
        case 0x5e40u:
        case 0x5e70u:
            owner_asset_id = 350u;
            candidate = TRUE;
            break;
        default:
            break;
        }
    }
#endif
    /* Poke Ball entry rays.  dEFManagerMBallRaysEffectDesc's 5-entry DObjDesc
     * at EFCommonEffects3+0x0628 keeps its live DObj tree, AnimJoint and
     * MatAnimJoint; only its two immutable Gfx roots are replaced.  Exact
     * source asset plus exact generated root is the whole admission test -- a
     * whole-image referrer census over all 2,132 O2R files found exactly one
     * pointer reaching each of these two roots and none from any other file,
     * so no live-kind discriminator is needed.  ftcommonentry.c:99 spawns this
     * from ftCommonAppearUpdateEffects for Pikachu and Jigglypuff, i.e. match
     * entry and every respawn, which is why it is live with items off. */
    if ((candidate == FALSE) && (gEFManagerFiles[2] != NULL) &&
        ((const u8 *)dl >= (const u8 *)gEFManagerFiles[2]))
    {
        base = (const u8 *)gEFManagerFiles[2];
        root_offset = (u32)((const u8 *)dl - base);
        if ((root_offset == 0x0440u) || (root_offset == 0x0518u) ||
            (root_offset == 0x2ef0u) || (root_offset == 0x2f80u) ||
            (root_offset == 0x3010u) || (root_offset == 0x30a0u))
        {
            owner_asset_id = 85u;
            candidate = TRUE;
            if ((root_offset == 0x0440u) || (root_offset == 0x0518u))
            {
                NDS_DIAG(gNdsMBallRaysCandidateCount++);
            }
        }
    }
    if (candidate == FALSE)
    {
        return FALSE;
    }

#if NDS_P2_CAPTAIN
    if (((owner_asset_id == 350u) && (root_offset == 0x0a30u)) ||
        ((owner_asset_id == 333u) && (root_offset == 0x0760u)))
    {
        MObj *mobj = dobj->mobj;
        s32 texture_id_curr = -1;
        s32 texture_id_next = -1;
        s32 texture_id_max = (owner_asset_id == 350u) ? 1 : 2;
        const u32 expected_effects =
            NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE |
            NDS_RENDERER_NATIVE_MATERIAL_RENDER_TILE_SIZE |
            NDS_RENDERER_NATIVE_MATERIAL_TEXTURE;

        /* Source MObj flags are 0x00A1 (ALPHA | 0x20 | TEXTURE). MatAnim
         * changes only TEXID; the root DL owns the effective combine/blend/TLUT
         * state. Snapshot without advancing IDs and select the matching AOT
         * CI4 frame so effective source alpha survives end to end. */
        bzero(&common_effect_material, sizeof(common_effect_material));
        if ((mobj == NULL) || (mobj->next != NULL) ||
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 mobj, &common_effect_material, FALSE,
                 &texture_id_curr, &texture_id_next) == FALSE) ||
            (common_effect_material.effects != expected_effects) ||
            (texture_id_curr < 0) || (texture_id_curr > texture_id_max) ||
            (texture_id_next < 0) || (texture_id_next > texture_id_max))
        {
            NDS_DIAG(gNdsEntryEffectNativeFallbackCount++);
            return FALSE;
        }
        native_materials = &common_effect_material;
        native_material_count = 1u;
        native_texture_variant = (u32)texture_id_curr;
    }
#endif

#if NDS_P2_LINK
    if (owner_asset_id == 353u)
    {
        MObj *mobj = dobj->mobj;

        /* LinkSpecial2's entry Wave, entry Beam, and attached grounded Spin
         * effect each select segment 0xE slot 0 from exactly one live MObj.
         * Keep the source MatAnim live and translate that one typed material
         * state instead of freezing its animated PRIM/light values in AOT. */
        if ((mobj == NULL) || (mobj->next != NULL) ||
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 mobj, &link_special2_material, FALSE, NULL, NULL) == FALSE))
        {
            NDS_DIAG(gNdsEntryEffectNativeFallbackCount++);
            return FALSE;
        }
        native_materials = &link_special2_material;
        native_material_count = 1u;
    }
    else if (owner_asset_id == 324u)
    {
        MObj *mobj = dobj->mobj;
        u32 i;

        /* Source LinkModel has exactly nine MObjs for this DObj. Snapshot the
         * live values without advancing texture ids: every source MObj is
         * PRIM-only, and the native owner validates that invariant before GX. */
        for (i = 0u; i < 9u; i++)
        {
            if ((mobj == NULL) ||
                (ndsRendererAdapterBuildNativeMaterialSnapshot(
                     mobj, &link_spin_materials[i], FALSE, NULL, NULL) == FALSE))
            {
                NDS_DIAG(gNdsEntryEffectNativeFallbackCount++);
                return FALSE;
            }
            mobj = mobj->next;
        }
        if (mobj != NULL)
        {
            NDS_DIAG(gNdsEntryEffectNativeFallbackCount++);
            return FALSE;
        }
        native_materials = link_spin_materials;
        native_material_count = 9u;
    }
#endif

    if (owner_asset_id == 85u)
    {
        MObj *mobj = dobj->mobj;
        u32 expected_material_count;
        u32 i;
        sb32 is_mballrays =
            ((root_offset == 0x0440u) || (root_offset == 0x0518u));

        if (is_mballrays != FALSE)
        {
            expected_material_count = 2u;
        }
        else if ((root_offset == 0x2ef0u) || (root_offset == 0x2f80u) ||
                 (root_offset == 0x3010u) || (root_offset == 0x30a0u))
        {
            expected_material_count = 1u;
        }
        else
        {
            NDS_DIAG(gNdsEntryEffectNativeFallbackCount++);
            return FALSE;
        }

        /* MBallRays owns two PRIM-only MObjs per ray fan and its list calls
         * segment 0xE slot 1 then slot 0; ItemGetSwirl owns ONE per drawable
         * child.  Snapshot only that live state, without advancing texture
         * ids, and let the native owner re-validate the closed contract before
         * it touches GX. */
        for (i = 0u; i < expected_material_count; i++)
        {
            if ((mobj == NULL) ||
                (ndsRendererAdapterBuildNativeMaterialSnapshot(
                     mobj, &efcommon3_materials[i], FALSE, NULL, NULL) ==
                 FALSE) ||
                (efcommon3_materials[i].effects !=
                     NDS_RENDERER_NATIVE_MATERIAL_PRIM))
            {
                /* No fallback and NO second record: return FALSE and let the
                 * single loud NO_PROGRAM guard publish this one event. */
                if (is_mballrays != FALSE)
                {
                    NDS_DIAG(gNdsMBallRaysMaterialRejectCount++);
                }
                NDS_DIAG(gNdsEntryEffectNativeFallbackCount++);
                return FALSE;
            }
            mobj = mobj->next;
        }
        if (mobj != NULL)
        {
            if (is_mballrays != FALSE)
            {
                NDS_DIAG(gNdsMBallRaysMaterialRejectCount++);
            }
            NDS_DIAG(gNdsEntryEffectNativeFallbackCount++);
            return FALSE;
        }
        native_materials = efcommon3_materials;
        native_material_count = expected_material_count;
    }

    if (owner_asset_id == 84u)
    {
        u32 expected_effects = NDS_RENDERER_NATIVE_MATERIAL_PRIM;
        if ((root_offset == 0x5218u) || (root_offset == 0x5310u))
        {
            expected_effects |= NDS_RENDERER_NATIVE_MATERIAL_ENV;
        }
        if ((root_offset == 0x31d0u) || (root_offset == 0x3258u) || (root_offset == 0x32e0u))
        {
            expected_effects |= NDS_RENDERER_NATIVE_MATERIAL_LIGHT1;
        }
        /* These source roots select segment-E material slot zero. Keep that
         * material's animated fields live; unselected slots cannot affect it. */
        bzero(&common_effect_material, sizeof(common_effect_material));
        if ((dobj->mobj == NULL) ||
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 dobj->mobj, &common_effect_material, FALSE, NULL, NULL) == FALSE) ||
            (common_effect_material.effects != expected_effects))
        {
            NDS_DIAG(gNdsEntryEffectNativeFallbackCount++);
            /* Material rejection status packs source flags in the low half
             * and prepared effects in the high half; all values come from
             * the CPU, not a stale debugger read of the task stack. */
            ndsRendererRecordNativeFailure(NDS_NATIVE_FAILURE_STAGE,
                (u32)gSCManagerSceneData.scene_curr,
                (((dobj->parent_gobj != NULL) ? dobj->parent_gobj->id : 0xffffu) << 16) | 84u,
                ((common_effect_material.effects & 0xffffu) << 16) |
                    ((dobj->mobj != NULL) ? dobj->mobj->sub.flags : 0xffffu),
                root_offset, (u32)(uintptr_t)dobj->mobj, NDS_NATIVE_FAILURE_BAD_ASSET);
            return FALSE;
        }
        native_materials = &common_effect_material;
        native_material_count = 1u;
    }
#if NDS_ENTRY_EFFECT_DIAG
    ndsEntryEffectDiagRecordDObj(root_offset, dobj);
#endif
    if ((sNdsRendererAdapterEntryPrepared != NULL) &&
        (sNdsRendererAdapterEntryPrepared->dobj == dobj) &&
        (sNdsRendererAdapterEntryPrepared->dl == dl))
    {
        /* Prepared by the Arwing's first pass with this same call; the
         * identity fill below still applies. */
        projection_ptr = (sNdsRendererAdapterEntryPrepared->projection_valid != 0u) ?
            &sNdsRendererAdapterEntryPrepared->projection : NULL;
        modelview_ptr = (sNdsRendererAdapterEntryPrepared->modelview_valid != 0u) ?
            &sNdsRendererAdapterEntryPrepared->modelview : NULL;
    }
    else
    {
        ndsRendererAdapterPrepareInitialMatrices(
            dobj,
            (camera_gobj != NULL) ? CObjGetStruct(camera_gobj) :
                ((gGCCurrentCamera != NULL) ? CObjGetStruct(gGCCurrentCamera) : NULL),
            FALSE, &projection, &projection_ptr, &modelview, &modelview_ptr);
    }
    NDS_LAB_ARW_MARK(lab_t[1]);
    /* The default battle camera has one legitimate split shape where the
     * complete camera transform lives on only one side of the DS pair (the
     * world-quad bridge handles the same contract above).  The generic DL
     * interpreter tolerates that because its matrix stream starts from
     * identity; this fixed owner has no stream to do the implicit fill for it.
     * Make that identity explicit so Fox's six small Arwing glow lists stay on
     * the native path instead of falling back solely because their capture pass
     * supplies one camera half. */
    if ((projection_ptr == NULL) && (modelview_ptr != NULL))
    {
        ndsRendererAdapterMtxIdentity20p12(&projection);
        projection_ptr = &projection;
    }
    if ((modelview_ptr == NULL) && (projection_ptr != NULL))
    {
        ndsRendererAdapterMtxIdentity20p12(&modelview);
        modelview_ptr = &modelview;
    }
    if ((projection_ptr == NULL) || (modelview_ptr == NULL))
    {
        NDS_DIAG(gNdsEntryEffectNativeFallbackCount++);
        ndsStageRejectNativeRender(dobj, dl,
            NDS_NATIVE_FAILURE_REJECTED_PROGRAM, NULL);
        return FALSE;
    }

    ndsRendererAdapterEntryEffectSeedStats(dobj, &stats);
    memset(&config, 0, sizeof(config));
    config.max_depth = 4u;
    config.max_commands = 1u;
    config.max_list_commands = 1u;
    config.initial_projection = projection_ptr;
    config.initial_modelview = modelview_ptr;
    config.initial_geometry_mode = initial_geometry_mode;
    config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;

    NDS_LAB_ARW_MARK(lab_t[2]);
    if (ndsRendererSubmitNativeEntryEffect(
            owner_asset_id, root_offset, native_materials,
            native_material_count, native_texture_variant,
            &config, &stats) == FALSE)
    {
        NDS_DIAG(gNdsEntryEffectNativeFallbackCount++);
        ndsStageRejectNativeRender(dobj, dl,
            NDS_NATIVE_FAILURE_REJECTED_PROGRAM, &stats);
        return FALSE;
    }
    NDS_LAB_ARW_MARK(lab_t[3]);

    ndsRendererAdapterEntryEffectAccumulate(&stats);
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    NDS_LAB_ARW_MARK(lab_t[4]);
    if ((owner_asset_id == 161u) && ((lab_t[4] - lab_t[0]) < 0x100000u))
    {
        static const u16 lab_roots[8] = { 0x1fa0u, 0x2920u, 0x29d0u, 0x29f0u,
                                          0x2a20u, 0x2868u, 0x2a50u, 0x2b00u };
        u32 r;

        for (r = 0u; r < 8u; r++)
        {
            if (root_offset == lab_roots[r])
            {
                NDS_DIAG(gNdsLabArwingRootCensus[r][0]++);
                NDS_DIAG(gNdsLabArwingRootCensus[r][1] += lab_t[1] - lab_t[0]);
                NDS_DIAG(gNdsLabArwingRootCensus[r][2] += lab_t[3] - lab_t[2]);
                gNdsLabArwingRootCensus[r][3] +=
                    (lab_t[2] - lab_t[1]) + (lab_t[4] - lab_t[3]);
                NDS_DIAG(gNdsLabArwingRootCensus[r][4] += stats.hardware_triangle_count);
                break;
            }
        }
    }
#endif
    return TRUE;
#else
    (void)dobj;
    (void)dl;
    (void)camera_gobj;
    (void)initial_geometry_mode;
    return FALSE;
#endif
}

#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_PIKACHU
static sb32 ndsRendererAdapterPikachuThunder(NDSRelocLoadedFile *loaded,
    DObj *dobj, const Gfx *dl, const NDSRendererConfig *config, NDSRendererStats *stats)
{
    NDSRendererNativeMaterial material;
    NDSRendererConfig local = *config;
    NDSRendererMatrix20p12 identity;
    const void *palette = NULL, *image = NULL;
    u32 offset = ndsRelocNativeRootOffset(loaded, dl), root, role;
    if (dobj->parent_gobj == NULL) return FALSE;
    if (loaded->asset_id == NDS_NATIVE_PIKACHU_THUNDER_MODEL && offset == NDS_NATIVE_PIKACHU_THUNDER_ROOT)
    {
        root = 0u;
        if (dobj->parent_gobj->id == nGCCommonKindWeapon)
        {
            WPStruct *wp = wpGetStruct(dobj->parent_gobj);
            if (wp == NULL) return FALSE;
            if (wp->kind == nWPKindThunderHead) role = 1u;
            else if (wp->kind == nWPKindThunderTrail) role = 2u;
            else return FALSE;
        }
        else if (dobj->parent_gobj->id == nGCCommonKindEffect) role = 4u;
        else return FALSE;
    }
    else if (loaded->asset_id == NDS_NATIVE_PIKACHU_THUNDER_SPECIAL &&
             dobj->parent_gobj->id == nGCCommonKindEffect)
    {
        if (offset == NDS_NATIVE_PIKACHU_SHOCK_ROOT0) root = 1u;
        else if (offset == NDS_NATIVE_PIKACHU_SHOCK_ROOT1) root = 2u;
        else return FALSE;
        role = 8u;
    }
    else return FALSE;
    if (root == 1u)
    {
        if (dobj->mobj != NULL || loaded->data_size < 0x13a0u) return FALSE;
#if NDS_P2_1P_GAME || NDS_P2_MENU_SHELL || NDS_P2_SHELL_ARGMAX_ROSTER || NDS_P2_COMPACT_BATTLE_FIGHTERS
        palette = ndsRelocNativeAssetAddress(loaded->data, NDS_NATIVE_PIKACHU_SHOCK_PALETTE);
        image = ndsRelocNativeAssetAddress(loaded->data, NDS_NATIVE_PIKACHU_SHOCK_IMAGE);
#else
        palette = (u8 *)loaded->data + NDS_NATIVE_PIKACHU_SHOCK_PALETTE;
        image = (u8 *)loaded->data + NDS_NATIVE_PIKACHU_SHOCK_IMAGE;
#endif
    }
    else if (dobj->mobj == NULL || dobj->mobj->next != NULL ||
             !ndsRendererAdapterBuildNativeMaterialSnapshot(dobj->mobj, &material, FALSE, NULL, NULL)) return FALSE;
    ndsRendererAdapterMtxIdentity20p12(&identity);
    if (local.initial_projection == NULL) local.initial_projection = &identity;
    if (local.initial_modelview == NULL) local.initial_modelview = &identity;
    return ndsRendererSubmitNativePikachuThunder(root, role,
        (root == 1u) ? NULL : &material, palette, image, &local, stats);
}
#endif

#if NDS_RENDERER_HW_TRIANGLES
static sb32 __attribute__((noinline)) ndsRendererAdapterSubmitDamageFlyMDust(
    DObj *dobj, const NDSRelocLoadedFile *loaded,
    const NDSRendererConfig *config, NDSRendererStats *stats)
{
    NDSRendererNativeMaterial material;
    const NDSRendererNativeMaterial *material_ptr = NULL;
    NDSRendererConfig local = *config;
    NDSRendererMatrix20p12 identity;

    /* Both source makers share this animated DObjDLLink. Preserve its current
     * image and tile state without a synthetic segment-E command stream. */
    if ((dobj->parent_gobj != NULL) &&
        (dobj->parent_gobj->id == nGCCommonKindEffect) &&
        (dobj->mobj != NULL) && (dobj->mobj->next == NULL) &&
        (ndsRendererAdapterBuildNativeMaterialSnapshot(
             dobj->mobj, &material, FALSE, NULL, NULL) != FALSE))
    {
        material_ptr = &material;
    }
    ndsRendererAdapterMtxIdentity20p12(&identity);
    if ((local.initial_projection == NULL) &&
        (local.initial_modelview != NULL)) { local.initial_projection = &identity; }
    else if ((local.initial_modelview == NULL) &&
             (local.initial_projection != NULL)) { local.initial_modelview = &identity; }
    return ndsRendererSubmitNativeDamageFlyMDust(
        loaded->data, loaded->data_size, NDS_NATIVE_DAMAGE_FLY_MDUST_ROOT,
        material_ptr, &local, stats);
}
#endif

#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
/* LAB ONLY (the any-stage sweep ROM): which GObjs reach the generic stage DL
 * submit and what they cost. Key = GObj id | (item/weapon kind << 16) for the
 * item and weapon links (ITStruct/WPStruct kind, both at +0xC), 0xFFFF for
 * everything else. Rows: key, calls, own ticks, GX-drain wait ticks, then
 * own ticks split at the Impl's phase marks: find, seed, material, matrix,
 * config/stats, exec, tail (a call that returns before a mark leaves the
 * rest in [2] only), then PrepareInitialMatrices' camera, world, compose,
 * recalc and its uncached-fallback count (gNdsLabPimAcc). */
#define NDS_LAB_STAGE_DL_CENSUS_ROWS 24u
volatile u32 gNdsLabStageDLCensus[NDS_LAB_STAGE_DL_CENSUS_ROWS][16]
    __attribute__((used));
/* Samples whose own or wait span read >= 2^20 ticks (the clock's 2^22
 * artifact), left out of the rows. */
volatile u32 gNdsLabStageDLCensusOutliers __attribute__((used));
static u32 sNdsLabStageDLMark[6];
/* 1 = drain the geometry engine before each submit (the wait split). */
volatile u32 gNdsLabStageDLDrain __attribute__((used, section(".data"))) = 0u;
#define NDS_LAB_SDL_MARK(i) (sNdsLabStageDLMark[(i)] = cpuGetTiming())
static void ndsRendererAdapterSubmitStageDLImpl(DObj *dobj, const Gfx *dl,
                                                 GObj *camera_gobj,
                                                 u32 initial_geometry_mode);
static void ndsRendererAdapterSubmitStageDL(DObj *dobj, const Gfx *dl,
                                             GObj *camera_gobj,
                                             u32 initial_geometry_mode)
{
    u32 start = cpuGetTiming();
    u32 idle;
    u32 spins = 0u;
    GObj *owner = (dobj != NULL) ? dobj->parent_gobj : NULL;
    u32 id = (owner != NULL) ? (u32)owner->id : 0u;
    u32 kind = 0xffffu;
    u32 key;
    u32 own;
    u32 i;

    /* Drain whatever the geometry engine still holds, so [3] is the
     * wait for earlier work and [2] this submit's own cost. */
    while ((gNdsLabStageDLDrain != 0u) &&
           (((*(volatile u32 *)0x04000600u) & (1u << 27)) != 0u) && /* GXSTAT */
           (spins < 0x40000u)) { spins++; }
    sNdsLabStageDLMark[0] = sNdsLabStageDLMark[1] = 0u;
    sNdsLabStageDLMark[2] = sNdsLabStageDLMark[3] = 0u;
    sNdsLabStageDLMark[4] = sNdsLabStageDLMark[5] = 0u;
    for (i = 0u; i < 5u; i++)
    {
        gNdsLabPimAcc[i] = 0u;
    }
    idle = cpuGetTiming();
    ndsRendererAdapterSubmitStageDLImpl(dobj, dl, camera_gobj,
                                        initial_geometry_mode);
    own = cpuGetTiming() - idle;
    if ((own >= 0x100000u) || ((idle - start) >= 0x100000u))
    {
        NDS_DIAG(gNdsLabStageDLCensusOutliers++);
        return;
    }
    if ((owner != NULL) && ((id == 1012u) || (id == 1013u)) &&
        (owner->user_data.p != NULL))
    {
        kind = (u32)((const s32 *)owner->user_data.p)[3] & 0xffffu;
    }
    else if ((owner != NULL) && (id == 1011u))
    {
        /* Effects have no kind word: key them by the asset whose list this
         * is (0xfffe = in no loaded file). */
        const NDSRelocLoadedFile *effect_file =
            ndsRelocFindLoadedFileContaining(dl, sizeof(*dl));

        kind = (effect_file != NULL) ? (effect_file->asset_id & 0xffffu) :
                                       0xfffeu;
    }
    key = (id & 0xffffu) | (kind << 16);
    for (i = 0u; i < NDS_LAB_STAGE_DL_CENSUS_ROWS; i++)
    {
        if ((gNdsLabStageDLCensus[i][0] == key) ||
            (gNdsLabStageDLCensus[i][1] == 0u))
        {
            gNdsLabStageDLCensus[i][0] = key;
            gNdsLabStageDLCensus[i][1]++;
            gNdsLabStageDLCensus[i][2] += own;
            gNdsLabStageDLCensus[i][3] += idle - start;
            if ((sNdsLabStageDLMark[0] != 0u) && (sNdsLabStageDLMark[1] != 0u) &&
                (sNdsLabStageDLMark[2] != 0u) && (sNdsLabStageDLMark[3] != 0u) &&
                (sNdsLabStageDLMark[4] != 0u) && (sNdsLabStageDLMark[5] != 0u))
            {
                gNdsLabStageDLCensus[i][4] += sNdsLabStageDLMark[0] - idle;
                gNdsLabStageDLCensus[i][5] += sNdsLabStageDLMark[4] - sNdsLabStageDLMark[0];
                gNdsLabStageDLCensus[i][6] += sNdsLabStageDLMark[5] - sNdsLabStageDLMark[4];
                gNdsLabStageDLCensus[i][7] += sNdsLabStageDLMark[1] - sNdsLabStageDLMark[5];
                gNdsLabStageDLCensus[i][8] += sNdsLabStageDLMark[2] - sNdsLabStageDLMark[1];
                gNdsLabStageDLCensus[i][9] += sNdsLabStageDLMark[3] - sNdsLabStageDLMark[2];
                gNdsLabStageDLCensus[i][10] += (idle + own) - sNdsLabStageDLMark[3];
            }
            gNdsLabStageDLCensus[i][11] += gNdsLabPimAcc[0];
            gNdsLabStageDLCensus[i][12] += gNdsLabPimAcc[1];
            gNdsLabStageDLCensus[i][13] += gNdsLabPimAcc[2];
            gNdsLabStageDLCensus[i][14] += gNdsLabPimAcc[3];
            gNdsLabStageDLCensus[i][15] += gNdsLabPimAcc[4];
            break;
        }
    }
}
#else
#define ndsRendererAdapterSubmitStageDLImpl ndsRendererAdapterSubmitStageDL
#define NDS_LAB_SDL_MARK(i) ((void)0)
#endif

/* P2-2p8 (2026-09-29): entry models first, ahead of the general submit.
 * The general submit is ~29 KB with a prologue that initialises dozens of
 * per-owner locals, and it paid all of that before its first statement handed
 * an entry model to its generated owner -- eight times a frame for Sector Z's
 * Arwing. The admission now runs in this small frame and the general body is
 * entered only for what it declines; a candidate the owner refuses records
 * its failure there exactly as before. */

static void __attribute__((noinline)) ndsRendererAdapterSubmitStageDLBody(
    DObj *dobj, const Gfx *dl, GObj *camera_gobj, u32 initial_geometry_mode);

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
/* P2-2p8 (2026-09-29): the fast lane for owners the general body already
 * served. A Samus Charge Shot or a Beam Sword root cost ~42K / ~29K ticks a
 * draw on Dream Land (lab census), a quarter of it the body finding its
 * owner: the entry-model scan, the loaded-file lookup and ~60 candidate
 * tests, a 4 KB frame and 20 KB of Thumb code, every draw. The body now
 * records the route when that owner draws a list, and a later draw of the
 * same list goes straight to the owner with the same inputs: the same
 * PrepareInitialMatrices, config, persistent stats and item colours, the
 * same stats tail. A route holds only immutable admission facts (the loaded
 * file, its asset and data, the root); everything live -- the GObj kind, the
 * item kind, a NULL MObj, the submit context -- is tested again each draw,
 * and anything else falls to the body as before. Owners with an MObj or a
 * segment-E material are not routed. */
volatile u32 gNdsStageDLFastLaneHits;
volatile u32 gNdsStageDLFastLaneFills;

#define NDS_SDL_ROUTE_NONE 0u
#define NDS_SDL_ROUTE_CHARGE_SHOT 1u
/* Not stored: decided per draw from the effect-tree owner's latch. */
#define NDS_SDL_ROUTE_VISUAL 0xffu
/* 2 + an index into sNdsStageDLItemRoutes: the MObj-less item owners, whose
 * admission is the same everywhere -- the item submit, an Item GObj of the
 * owner's kind, no MObj -- and whose call is one of two shapes. */
#define NDS_SDL_ROUTE_ITEM 2u
/* A baked root (nds_native_item_baked.exec.inc) under a ground-display GObj:
 * Board the Platforms' platforms, ~30 lists a frame, which each paid the
 * whole body -- its owner probes, the entry-effect probe and the baked-root
 * search -- for one table lookup. The route keeps the root handle (in `root`)
 * and its material slot count (in `pad`); the MObj materials are live, so they
 * are snapshotted again on every draw, as the body does. */
#define NDS_SDL_ROUTE_BAKED 0xfeu
/* P2-6 (2026-10-01): a baked root under an Item GObj (the Race's Bob-ombs
 * and placed Bumpers): every draw went through the body (the Race: 8 body
 * submits a frame, none routed). Same inputs as the body's baked branch --
 * the item submit's head colours over the reset persistent stats -- and the
 * same off-screen exit as NDS_SDL_ROUTE_BAKED. */
#define NDS_SDL_ROUTE_BAKED_ITEM 0xfdu
/* P2-6 (2026-10-02): the GBumper's quad (the Race places them along the
 * course; Peach's Castle drops one): the body's castle bumper owner with the
 * body's admission -- an Item GObj of kind GBumper under the item submit, one
 * MObj of the owner's flags whose snapshot selects a palette image -- tested
 * again on every draw, and the snapshot taken live, as the body takes it. */
#define NDS_SDL_ROUTE_CASTLE_BUMPER 0xfcu
/* P2-6 (2026-10-02): the ending's room (owner: "End scene in the room with
 * the desk is really slow"; 3-4 VBlanks a frame). Its six GObjs (kind 0) draw
 * their DObj lists through the stage traversal, and every list paid the
 * fast-lane miss, the entry-effect probe and the whole body for one lookup in
 * the room table the ending loads. The route is NDS_SDL_ROUTE_BAKED's, owned
 * by any GObj whose root lies in that table (re-tested per draw: the table
 * leaves with the ending's heap), with the same persistent stats and
 * off-screen exit the body's baked branch reaches. */
#define NDS_SDL_ROUTE_BAKED_ROOM 0xfbu
/* P2-2p8 (2026-10-05): the Fire Flower's live root (0x4608): one MObj whose
 * snapshot selects a palette image, as the body's candidate admits it --
 * an Item GObj of kind FFlower under the item submit, the owner's MObj flags
 * -- tested again on every draw, the snapshot taken live. Its branch root
 * (0x4520, no MObj) is an ordinary item route. Each draw paid the body: two
 * lists a frame for as long as a flower is out (Jungle's sweep match, frame
 * 820 on, ~29K cycles a frame). */
#define NDS_SDL_ROUTE_FFLOWER_LIVE 0xfau
/* P2-2p8 (2026-10-05): the N Bumper's quad (ITCommonObject 0x7558, the list
 * the castle bumper draws, under an item of kind NBumper): the body's
 * admission -- the item submit, one MObj of the owner's flags, a
 * palette-image snapshot taken live -- tested again on every draw. Sector Z's
 * sweep match drew it through the body on 238 frames. */
#define NDS_SDL_ROUTE_NBUMPER 0xf9u
/* P2-2p8 (2026-10-05), the next owners a body-submit census found drawn
 * through the body every frame they live (clean sweep ROM, frames
 * 100-1,900): Sector Z's Arwing laser (226 frames; a weapon, no MObj, its
 * texture in a second file the list's relocated words name -- re-proved each
 * draw), Saffron's Hitokage and Fushigibana (one CURRENT_IMAGE material
 * snapshot, taken live), and the damage-fly dust (an effect; its MObj
 * snapshot taken live by the body's own helper, under the effect layer's
 * seeds and witnesses). Each admission is the body's, tested again on every
 * draw. Saffron's MObj-less Marumine, GLucky and Porygon are item routes. */
#define NDS_SDL_ROUTE_SECTOR_LASER 0xf8u
#define NDS_SDL_ROUTE_HITOKAGE 0xf7u
#define NDS_SDL_ROUTE_FUSHIGIBANA 0xf6u
#define NDS_SDL_ROUTE_DAMAGE_FLY_MDUST 0xf5u
/* Mushroom Kingdom's two Pakkun (file 155 root 0x0B40: one MObj of flags
 * 0x0001 whose CURRENT_IMAGE lies in the same file, the palette in file 107)
 * and its POW block (0x10D0: no MObj, the TLUT in file 107 and both images
 * in file 155, every word proved against those files): items the body drew
 * on every frame of the sweep match (3,600 and 805 body submits). */
#define NDS_SDL_ROUTE_INISHIE_PAKKUN 0xf4u
#define NDS_SDL_ROUTE_INISHIE_POWBLOCK 0xf3u
/* The star quad four owners share (ITCommonObject 0x5458: the Star Rod's two
 * weapons and Kirby's two stars, an effect): no MObj, admitted by asset and
 * root under any owner, as the body admits it; under the effect layer it
 * takes the layer's seeds and witnesses first. */
#define NDS_SDL_ROUTE_KIRBYSTAR 0xf2u
/* P2-2p8 (2026-10-05), owner playtest "Pikachu Down B / Ness Up B cause P95
 * slowdown": Thunder's bolt (its head and trail weapons, the trail effect and
 * the shock roots) and PK Thunder's head and trail weapons drew through the
 * body on every frame they live, a dozen lists a frame with four Pikachus.
 * Admission is the body's owner's own, tested again on every draw: Thunder's
 * adapter re-proves the GObj kind, the weapon kind, the root and its MObj
 * snapshot; PK Thunder's arm re-proves the weapon kind against the recorded
 * root, the trail id and the snapshot's effects. An owner that declines sends
 * the list back to the body, which decides as before. */
#define NDS_SDL_ROUTE_PIKACHU_THUNDER 0xf1u
#define NDS_SDL_ROUTE_NESS_PKTHUNDER 0xf0u
/* Yoshi's Island's capsules and boxes thrashed an 8-slot table (1,231 fills
 * for 1,469 hits a match): the owners are few, but a capsule alone draws three
 * roots, and its header and third root shared a slot under an address-bit
 * index. A multiplicative hash over 64 slots. */
#define NDS_SDL_ROUTES 64u
#define NDS_SDL_ROUTE_SHIFT 26u

#if NDS_P2_ITEM_CORE
typedef sb32 (*NDSStageDLItemSubmit)(const void *base, u32 bytes,
                                     const NDSRendererConfig *config,
                                     NDSRendererStats *stats);
typedef sb32 (*NDSStageDLItemSubmitRooted)(u32 root, const void *base,
                                           u32 bytes,
                                           const NDSRendererConfig *config,
                                           NDSRendererStats *stats);
typedef struct NDSStageDLItemRoute
{
    NDSStageDLItemSubmit submit;
    NDSStageDLItemSubmitRooted submit_rooted;
    volatile u32 *draws;
    volatile u32 *fails;
    u32 item_kind;
} NDSStageDLItemRoute;

enum
{
    nNDSStageDLItemSword,
    nNDSStageDLItemBat,
    nNDSStageDLItemCapsule,
    nNDSStageDLItemStarRod,
    nNDSStageDLItemMSBomb,
    nNDSStageDLItemBox,
    nNDSStageDLItemTaru,
    nNDSStageDLItemEgg,
    nNDSStageDLItemIwark,
    nNDSStageDLItemHammer,
    nNDSStageDLItemLGun,
    nNDSStageDLItemHarisen,
    nNDSStageDLItemHeart,
    nNDSStageDLItemFFlower,
#if NDS_P2_STAGE_YAMABUKI
    nNDSStageDLItemMarumine,
    nNDSStageDLItemGLucky,
    nNDSStageDLItemPorygon,
#endif
    nNDSStageDLItemRouteCount
};

/* The Fire Flower's branch root: no material. */
static sb32 ndsStageDLSubmitFFlowerBranch(u32 root, const void *base,
                                          u32 bytes,
                                          const NDSRendererConfig *config,
                                          NDSRendererStats *stats)
{
    return ndsRendererSubmitNativeItemFFlower(root, base, bytes, NULL, config,
                                              stats);
}

static const NDSStageDLItemRoute sNdsStageDLItemRoutes[nNDSStageDLItemRouteCount] =
{
    [nNDSStageDLItemSword] = { NULL, ndsRendererSubmitNativeItemSword,
        &gNdsItemSwordDrawCount, &gNdsItemSwordSubmitFailCount, nITKindSword },
    [nNDSStageDLItemBat] = { NULL, ndsRendererSubmitNativeItemBat,
        &gNdsItemBatDrawCount, &gNdsItemBatSubmitFailCount, nITKindBat },
    [nNDSStageDLItemCapsule] = { NULL, ndsRendererSubmitNativeItemCapsule,
        &gNdsItemCapsuleDrawCount, &gNdsItemCapsuleSubmitFailCount,
        nITKindCapsule },
    [nNDSStageDLItemStarRod] = { NULL, ndsRendererSubmitNativeItemStarRod,
        &gNdsItemStarRodDrawCount, &gNdsItemStarRodSubmitFailCount,
        nITKindStarRod },
    [nNDSStageDLItemMSBomb] = { NULL, ndsRendererSubmitNativeItemMSBomb,
        &gNdsItemMSBombDrawCount, &gNdsItemMSBombSubmitFailCount,
        nITKindMSBomb },
    [nNDSStageDLItemBox] = { ndsRendererSubmitNativeItemBox, NULL,
        &gNdsItemBoxDrawCount, &gNdsItemBoxSubmitFailCount, nITKindBox },
    [nNDSStageDLItemTaru] = { ndsRendererSubmitNativeItemTaru, NULL,
        &gNdsItemTaruDrawCount, &gNdsItemTaruSubmitFailCount, nITKindTaru },
    [nNDSStageDLItemEgg] = { ndsRendererSubmitNativeItemEgg, NULL,
        &gNdsItemEggDrawCount, &gNdsItemEggSubmitFailCount, nITKindEgg },
    [nNDSStageDLItemIwark] = { ndsRendererSubmitNativeItemIwark, NULL,
        &gNdsItemIwarkDrawCount, &gNdsItemIwarkSubmitFailCount,
        nITKindIwark },
    [nNDSStageDLItemHammer] = { ndsRendererSubmitNativeItemHammer, NULL,
        &gNdsItemHammerDrawCount, &gNdsItemHammerSubmitFailCount,
        nITKindHammer },
    [nNDSStageDLItemLGun] = { ndsRendererSubmitNativeItemLGun, NULL,
        &gNdsItemLGunDrawCount, &gNdsItemLGunSubmitFailCount, nITKindLGun },
    [nNDSStageDLItemHarisen] = { ndsRendererSubmitNativeItemHarisen, NULL,
        &gNdsItemHarisenDrawCount, &gNdsItemHarisenSubmitFailCount,
        nITKindHarisen },
    [nNDSStageDLItemHeart] = { ndsRendererSubmitNativeItemHeart, NULL,
        &gNdsItemHeartDrawCount, &gNdsItemHeartSubmitFailCount,
        nITKindHeart },
    [nNDSStageDLItemFFlower] = { NULL, ndsStageDLSubmitFFlowerBranch,
        &gNdsItemFFlowerDrawCount, &gNdsItemFFlowerSubmitFailCount,
        nITKindFFlower },
#if NDS_P2_STAGE_YAMABUKI
    [nNDSStageDLItemMarumine] = { ndsRendererSubmitNativeYamabukiMarumine,
        NULL, &gNdsYamabukiMarumineDrawCount,
        &gNdsYamabukiMarumineSubmitFailCount, nITKindMarumine },
    [nNDSStageDLItemGLucky] = { ndsRendererSubmitNativeItemGLucky, NULL,
        &gNdsYamabukiGluckyDrawCount, &gNdsYamabukiGluckySubmitFailCount,
        nITKindGLucky },
    [nNDSStageDLItemPorygon] = { ndsRendererSubmitNativeItemPorygon, NULL,
        &gNdsYamabukiPorygonDrawCount, &gNdsYamabukiPorygonSubmitFailCount,
        nITKindPorygon },
#endif
};
#endif

typedef struct NDSStageDLRoute
{
    const Gfx *dl;
    NDSRelocLoadedFile *loaded;
    const void *data;
    u32 data_size;
    u32 root;
    u16 asset_id;
    u8 route;
    u8 pad;
} NDSStageDLRoute;

static NDSStageDLRoute sNdsStageDLRoutes[NDS_SDL_ROUTES];

/* P2-6 (2026-10-02): two ways, the hashed slot and its pair. The Race's
 * bumper quad and a Bob-omb list shared a slot and evicted each other on
 * every draw (1,021 fills for 1,021 body submits in 600 frames). Returns the
 * slot holding `dl`, else the hashed one. */
static inline NDSStageDLRoute *ndsStageDLRouteSlot(const Gfx *dl)
{
    const u32 index = ((u32)(uintptr_t)dl * 2654435761u) >>
        NDS_SDL_ROUTE_SHIFT;
    NDSStageDLRoute *slot = &sNdsStageDLRoutes[index];

    if (slot->dl != dl)
    {
        NDSStageDLRoute *pair = &sNdsStageDLRoutes[index ^ 1u];

        if (pair->dl == dl)
        {
            return pair;
        }
    }
    return slot;
}

static void ndsStageDLRouteRecord(const Gfx *dl, NDSRelocLoadedFile *loaded,
                                  u32 root, u32 route)
{
    NDSStageDLRoute *slot = ndsStageDLRouteSlot(dl);

    if ((loaded == NULL) || (loaded->data == NULL))
    {
        return;
    }
    if ((slot->dl != dl) && (slot->route != NDS_SDL_ROUTE_NONE))
    {
        /* The hashed slot holds another list: take its pair. */
        slot = &sNdsStageDLRoutes[(u32)(slot - sNdsStageDLRoutes) ^ 1u];
    }
    if ((slot->dl != dl) || (slot->route != route))
    {
        NDS_DIAG(gNdsStageDLFastLaneFills++);
    }
    slot->dl = dl;
    slot->loaded = loaded;
    slot->data = loaded->data;
    slot->data_size = loaded->data_size;
    slot->root = root;
    slot->asset_id = (u16)loaded->asset_id;
    slot->route = (u8)route;
}

#if NDS_P2_ITEM_CORE
#include <nds/nds_item_replay.h>

/* P2-2p8 (2026-10-04): ITEM DRAW REPLAY (include/nds/nds_item_replay.h).
 *
 * The gate's late match draws only Beam Swords, one lying and one held, and
 * each root paid ~15K ticks: the stage traversal reset, the fast lane's
 * config, stats reset and seeds, the executor's generated N64 state setup and
 * the per-vertex words -- for an 11-triangle blade and a 2-triangle hilt
 * whose GX output never changes while the inputs below do not. An item draw
 * whose every list is a replayable owner records once (its emits, through
 * ndsNativeItemWave1Emit's sink) and later draws replay: the same tree walk,
 * the same matrix preparation per list (the held item's latch walk included:
 * its FTParts writes are gameplay), the recorded binds, batch states and
 * corners. The persistent stats the skipped owners would have written are
 * dead: the next traversal resets them before any reader.
 *
 * The key is held verbatim, not hashed: the item's root and kind, the
 * initial geometry mode, the traversal's light, and per list its DObj, list,
 * head, route and the head's captured colours and blend modes. A recorded
 * texture must still be resident, else the draw runs its owners and records
 * again. */
#define NDS_ITEM_REPLAY_DRAWS 2u
#define NDS_ITEM_REPLAY_ROOTS 4u
#define NDS_ITEM_REPLAY_KEY_WORDS 56u

typedef struct NDSItemReplayRoot
{
    DObj *dobj;
    const Gfx *dl;
    u8 head;
    u8 emit_first;
    u8 emit_count;
    u8 pad;
} NDSItemReplayRoot;

typedef struct NDSItemReplayDraw
{
    const DObj *root;
    u32 last_used;
    u8 valid;
    u8 root_count;
    u8 key_count;
    u8 emit_count;
    NDSItemReplayRoot roots[NDS_ITEM_REPLAY_ROOTS];
    u32 key[NDS_ITEM_REPLAY_KEY_WORDS];
    NDSItemReplayEmit emits[NDS_ITEM_REPLAY_EMITS];
    u32 pool[NDS_ITEM_REPLAY_POOL_WORDS];
} NDSItemReplayDraw;

__attribute__((used)) volatile u32 gNdsItemReplayDraws;
__attribute__((used)) volatile u32 gNdsItemReplayRecords;
__attribute__((used)) volatile u32 gNdsItemReplayRecordFailed;
__attribute__((used)) volatile u32 gNdsItemReplayNotResident;

static NDSItemReplayDraw sNdsItemReplayDraws[NDS_ITEM_REPLAY_DRAWS];
static NDSItemReplaySink sNdsItemReplaySinkState;
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
/* LAB: 1 = every draw that would replay runs its owners into a scratch
 * recording instead, and counts a recording that differs from the cached one
 * in any emit field or vertex word. */
volatile u32 gNdsItemReplayVerify __attribute__((used, section(".data"))) = 0u;
__attribute__((used)) volatile u32 gNdsItemReplayVerifyRuns;
__attribute__((used)) volatile u32 gNdsItemReplayVerifyFail;
static NDSItemReplayDraw sNdsItemReplayVerifyDraw;

static sb32 ndsItemReplaySameRecording(const NDSItemReplayDraw *a,
                                       const NDSItemReplayDraw *b)
{
    u32 i;

    if ((a->valid != 1u) || (a->emit_count != b->emit_count) ||
        (a->root_count != b->root_count))
    {
        return FALSE;
    }
    for (i = 0u; i < a->root_count; i++)
    {
        if ((a->roots[i].emit_first != b->roots[i].emit_first) ||
            (a->roots[i].emit_count != b->roots[i].emit_count))
        {
            return FALSE;
        }
    }
    for (i = 0u; i < a->emit_count; i++)
    {
        const NDSItemReplayEmit *x = &a->emits[i];
        const NDSItemReplayEmit *y = &b->emits[i];

        if ((x->indices != y->indices) || (x->drawn != y->drawn) ||
            (x->use_texture != y->use_texture) ||
            (x->vertex_count != y->vertex_count) ||
            (x->corner_count != y->corner_count) ||
            (x->triangle_count != y->triangle_count))
        {
            return FALSE;
        }
        if ((x->use_texture != 0u) &&
            ((x->tex_slot != y->tex_slot) || (x->tex_name != y->tex_name) ||
             (x->tex_generation != y->tex_generation) ||
             (x->tex_format != y->tex_format) ||
             (x->tex_width != y->tex_width) ||
             (x->tex_height != y->tex_height)))
        {
            return FALSE;
        }
        if ((x->drawn != 0u) &&
            ((x->poly_fmt != y->poly_fmt) ||
             (x->othermode_l != y->othermode_l) ||
             (x->blend_color != y->blend_color) ||
             (memcmp(&a->pool[x->word_first], &b->pool[y->word_first],
                     (u32)x->vertex_count * 4u * sizeof(u32)) != 0)))
        {
            return FALSE;
        }
    }
    return TRUE;
}
#endif
/* The draw being recorded and the lists it has seen, NULL outside one. */
static NDSItemReplayDraw *sNdsItemReplayRecording;
static u32 sNdsItemReplayRecordRoots;

/* Owners whose GX output is their ndsNativeItemWave1Emit calls and nothing
 * else: the MObj-less item routes (fixed geometry, generated setup, no
 * materials). An owner the sink cannot hold (too many emits or vertices)
 * fails its recording and keeps drawing itself. */
static inline sb32 ndsItemReplayRouteOk(u32 route_kind)
{
    return ((route_kind >= NDS_SDL_ROUTE_ITEM) &&
            (route_kind < (NDS_SDL_ROUTE_ITEM + nNDSStageDLItemRouteCount))) ?
        TRUE : FALSE;
}

/* The fast lane's item branch, around its owner call: the list must be the
 * next one the recording's walk named, through a replayable owner. */
static sb32 ndsItemReplayRootBegin(const DObj *dobj, const Gfx *dl,
                                   u32 route_kind, u32 *emit_first)
{
    NDSItemReplayDraw *draw = sNdsItemReplayRecording;
    const u32 index = sNdsItemReplayRecordRoots;

    if (draw == NULL)
    {
        return FALSE;
    }
    if ((ndsItemReplayRouteOk(route_kind) == FALSE) ||
        (index >= draw->root_count) || (draw->roots[index].dobj != dobj) ||
        (draw->roots[index].dl != dl))
    {
        sNdsItemReplaySinkState.failed = 1u;
        return FALSE;
    }
    *emit_first = sNdsItemReplaySinkState.emit_count;
    return TRUE;
}

static void ndsItemReplayRootEnd(sb32 handled, u32 emit_first)
{
    NDSItemReplayDraw *draw = sNdsItemReplayRecording;
    const u32 index = sNdsItemReplayRecordRoots;

    if (draw == NULL)
    {
        return;
    }
    if (handled == FALSE)
    {
        sNdsItemReplaySinkState.failed = 1u;
    }
    draw->roots[index].emit_first = (u8)emit_first;
    draw->roots[index].emit_count =
        (u8)(sNdsItemReplaySinkState.emit_count - emit_first);
    sNdsItemReplayRecordRoots = index + 1u;
}
#endif

/* The body's item-submit seeds over the reset persistent stats, with its
 * witnesses: the head's prim/env colours and blend modes, each only where the
 * head captured it. */
static inline void ndsStageDLFastItemSeeds(NDSRendererStats *render_stats)
{
    const u32 head = (sNdsRendererAdapterItemSubmitHead <
                      NDS_RENDERER_STAGE_DL_HEADS) ?
        sNdsRendererAdapterItemSubmitHead : 0u;

    gNdsItemRendererLastHead = head;
    gNdsItemRendererLastColorMask = sNdsRendererAdapterItemColorMask[head];
    gNdsItemRendererLastEnvColor = sNdsRendererAdapterItemEnvColor[head];
    gNdsItemRendererLastOtherModeL = sNdsRendererAdapterItemOtherModeL[head];
    gNdsItemRendererLastOtherModeH = sNdsRendererAdapterItemOtherModeH[head];
    if ((sNdsRendererAdapterItemColorMask[head] & 1u) != 0u)
    {
        render_stats->prim_color = sNdsRendererAdapterItemPrimColor[head];
    }
    if ((sNdsRendererAdapterItemColorMask[head] & 2u) != 0u)
    {
        render_stats->env_color = sNdsRendererAdapterItemEnvColor[head];
    }
    if (sNdsRendererAdapterItemOtherModeLValid[head] != 0u)
    {
        render_stats->othermode_l = sNdsRendererAdapterItemOtherModeL[head];
    }
    if (sNdsRendererAdapterItemOtherModeHValid[head] != 0u)
    {
        render_stats->othermode_h = sNdsRendererAdapterItemOtherModeH[head];
    }
}

/* TRUE when a routed owner drew `dl`; FALSE sends it to the body. */
static sb32 __attribute__((noinline)) ndsRendererAdapterSubmitStageDLFast(
    DObj *dobj, const Gfx *dl, GObj *camera_gobj, u32 initial_geometry_mode)
{
    NDSStageDLRoute *route = ndsStageDLRouteSlot(dl);
    NDSRelocLoadedFile *loaded = route->loaded;
    GObj *owner = dobj->parent_gobj;
    u32 route_kind = NDS_SDL_ROUTE_NONE;
#if NDS_P2_STAGE_CASTLE
    NDSRendererNativeMaterial bumper_material;
#endif
#if NDS_P2_ITEM_CORE
    /* The Fire Flower's or the N Bumper's live snapshot. */
    NDSRendererNativeMaterial item_route_material;
#endif
#if NDS_P2_STAGE_SECTOR
    const u8 *laser_tex_base = NULL;
#endif
#if NDS_P2_STAGE_INISHIE
    const u8 *inishie_pal_base = NULL;
#endif
#if NDS_P2_NESS
    NDSRendererNativeMaterial pkthunder_material;
    u32 pkthunder_root_index = 0u;
    u32 pkthunder_trail_color = 0u;
#endif
    /* Zeroed once a route is taken: most lists leave at the route test. */
    NDSRendererConfig config;
    NDSRendererStats *render_stats;
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;
    NDSRendererMatrix20p12 identity;
    const NDSRendererMatrix20p12 *projection_ptr;
    const NDSRendererMatrix20p12 *modelview_ptr;
    void *saved_graphics_heap_ptr;
    sb32 handled = FALSE;

#if NDS_R2_IMPACT_WAVE_NATIVE
    /* The procedural visual templates need no route: the effect-tree owner
     * latched the template for this GObj, and the body claims such a list
     * before any loaded-file owner could -- its list lives in the arena, in
     * no loaded file, so no earlier candidate can match it. The same tests,
     * and the arena check whose failure the body reports instead. */
    if ((sNdsRendererAdapterVisualEffectNativeActive != FALSE) &&
        (sNdsRendererAdapterEffectSubmitActive != FALSE) &&
        (sNdsRendererAdapterImpactWaveNativeActive == FALSE) &&
#if NDS_R2_REBIRTH_HALO_NATIVE
        (sNdsRendererAdapterRebirthHaloNativeActive == FALSE) &&
#endif
        (sNdsRendererAdapterItemSubmitActive == FALSE) &&
        (sNdsRendererAdapterStagePersistentActive != FALSE) &&
        (dobj->dl == dl) && (dobj->child == NULL) &&
        (ndsRendererHardwareNoOracleEnabled() != FALSE) &&
        (ndsRelocFindLoadedFileContaining(dl, sizeof(*dl)) == NULL) &&
        (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(*dl)) != FALSE))
    {
        route_kind = NDS_SDL_ROUTE_VISUAL;
        loaded = NULL;
    }
#endif
    if ((route_kind == NDS_SDL_ROUTE_NONE) &&
        ((route->dl != dl) || (route->route == NDS_SDL_ROUTE_NONE) ||
         (loaded == NULL) || (loaded->data != route->data) ||
         (loaded->data_size != route->data_size) ||
         (loaded->asset_id != (u32)route->asset_id) ||
         ((dobj->mobj != NULL) && (route->route != NDS_SDL_ROUTE_BAKED) &&
          (route->route != NDS_SDL_ROUTE_BAKED_ITEM) &&
          (route->route != NDS_SDL_ROUTE_BAKED_ROOM) &&
          (route->route != NDS_SDL_ROUTE_CASTLE_BUMPER) &&
          (route->route != NDS_SDL_ROUTE_FFLOWER_LIVE) &&
          (route->route != NDS_SDL_ROUTE_NBUMPER) &&
          (route->route != NDS_SDL_ROUTE_HITOKAGE) &&
          (route->route != NDS_SDL_ROUTE_FUSHIGIBANA) &&
          (route->route != NDS_SDL_ROUTE_DAMAGE_FLY_MDUST) &&
          (route->route != NDS_SDL_ROUTE_INISHIE_PAKKUN) &&
          (route->route != NDS_SDL_ROUTE_PIKACHU_THUNDER) &&
          (route->route != NDS_SDL_ROUTE_NESS_PKTHUNDER)) ||
         (owner == NULL) ||
         (sNdsRendererAdapterStagePersistentActive == FALSE) ||
         ((sNdsRendererAdapterEffectSubmitActive != FALSE) &&
          (route->route != NDS_SDL_ROUTE_DAMAGE_FLY_MDUST) &&
          (route->route != NDS_SDL_ROUTE_KIRBYSTAR) &&
          (route->route != NDS_SDL_ROUTE_PIKACHU_THUNDER)) ||
         (ndsRendererHardwareNoOracleEnabled() == FALSE)))
    {
        return FALSE;
    }
    if (route_kind == NDS_SDL_ROUTE_NONE)
    {
        route_kind = route->route;
    }
    switch (route_kind)
    {
    case NDS_SDL_ROUTE_VISUAL:
        break;
    case NDS_SDL_ROUTE_CHARGE_SHOT:
        if ((owner->id != nGCCommonKindWeapon) ||
            (sNdsRendererAdapterItemSubmitActive != FALSE))
        {
            return FALSE;
        }
        break;
#if NDS_P2_STAGE_SECTOR
    case NDS_SDL_ROUTE_SECTOR_LASER:
    {
        /* The texture file, found from the pointer the list carries and
         * proved against both relocated words, as the body proves it. */
        const NDSRelocLoadedFile *laser_tex;

        if ((owner->id != nGCCommonKindWeapon) ||
            (sNdsRendererAdapterItemSubmitActive != FALSE) ||
            (dobj->mobj != NULL))
        {
            return FALSE;
        }
        laser_tex = ndsRelocFindLoadedFileContaining(
            (const void *)(uintptr_t)dl[14].words.w1, 1u);
        if ((laser_tex == NULL) || (laser_tex->data == NULL) ||
            (laser_tex->asset_id != NDS_NATIVE_SECTOR_LASER_TEX_ASSET) ||
            (laser_tex->data_size < NDS_NATIVE_SECTOR_LASER_TEX_END))
        {
            return FALSE;
        }
        laser_tex_base = (const u8 *)laser_tex->data;
        if ((dl[8].words.w1 != (u32)(uintptr_t)(laser_tex_base +
                 NDS_NATIVE_SECTOR_LASER_TLUT_OFFSET)) ||
            (dl[14].words.w1 != (u32)(uintptr_t)(laser_tex_base +
                 NDS_NATIVE_SECTOR_LASER_IMAGE_OFFSET)))
        {
            return FALSE;
        }
        break;
    }
#endif
    case NDS_SDL_ROUTE_DAMAGE_FLY_MDUST:
        /* The body's arm admits the list by asset and root alone; its helper
         * snapshots the MObj under an effect owner. */
        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            return FALSE;
        }
        break;
#if NDS_P2_PIKACHU
    case NDS_SDL_ROUTE_PIKACHU_THUNDER:
        /* The adapter re-proves the rest on every draw. */
        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            return FALSE;
        }
        break;
#endif
#if NDS_P2_NESS
    case NDS_SDL_ROUTE_NESS_PKTHUNDER:
    {
        /* The body's candidate test (PK Thunder's head and trail are
         * NessModel weapons with one live CURRENT_IMAGE MObj). */
        WPStruct *pkthunder_wp;

        if ((owner->id != nGCCommonKindWeapon) ||
            (sNdsRendererAdapterItemSubmitActive != FALSE) ||
            (dobj->mobj == NULL) || (dobj->mobj->next != NULL))
        {
            return FALSE;
        }
        pkthunder_wp = wpGetStruct(owner);
        if ((pkthunder_wp != NULL) &&
            (pkthunder_wp->kind == nWPKindPKThunderHead) &&
            (route->root == NDS_NATIVE_NESS_PKTHUNDER_HEAD_ROOT))
        {
            pkthunder_root_index = NDS_NATIVE_NESS_PKTHUNDER_HEAD_INDEX;
        }
        else if ((pkthunder_wp != NULL) &&
                 (pkthunder_wp->kind == nWPKindPKThunderTrail) &&
                 (route->root == NDS_NATIVE_NESS_PKTHUNDER_TRAIL_ROOT) &&
                 ((u32)pkthunder_wp->weapon_vars.pkthunder_trail.trail_id <
                  NDS_NATIVE_NESS_PKTHUNDER_TRAIL_COLOR_COUNT))
        {
            pkthunder_root_index = NDS_NATIVE_NESS_PKTHUNDER_TRAIL_INDEX;
            pkthunder_trail_color =
                (u32)pkthunder_wp->weapon_vars.pkthunder_trail.trail_id;
        }
        else
        {
            return FALSE;
        }
        if ((ndsRendererAdapterBuildNativeMaterialSnapshot(
                 dobj->mobj, &pkthunder_material, FALSE, NULL, NULL) ==
             FALSE) ||
            (pkthunder_material.effects !=
                 NDS_NATIVE_NESS_PKTHUNDER_MATERIAL_EFFECTS))
        {
            return FALSE;
        }
        break;
    }
#endif
#if NDS_P2_ITEM_CORE
    case NDS_SDL_ROUTE_KIRBYSTAR:
        if ((sNdsRendererAdapterItemSubmitActive != FALSE) ||
            (dobj->mobj != NULL))
        {
            return FALSE;
        }
        break;
#endif
#if NDS_P2_STAGE_INISHIE && NDS_P2_ITEM_CORE
    case NDS_SDL_ROUTE_INISHIE_PAKKUN:
    {
        const NDSRelocLoadedFile *pal;

        if ((owner->id != nGCCommonKindItem) ||
            (sNdsRendererAdapterItemSubmitActive == FALSE) ||
            (dobj->mobj == NULL) || (dobj->mobj->next != NULL) ||
            (ndsRendererAdapterMaterialFlags(dobj->mobj) != 0x0001u) ||
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 dobj->mobj, &item_route_material, FALSE, NULL, NULL) == FALSE) ||
            (item_route_material.effects !=
                 NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE) ||
            (ndsRelocFindLoadedFileContaining(
                 (const void *)(uintptr_t)item_route_material.current_image,
                 1u) != loaded))
        {
            return FALSE;
        }
        pal = ndsRelocFindLoadedFileByAsset(107u);
        if ((pal == NULL) || (pal->data_size < (0x3620u + 32u)))
        {
            return FALSE;
        }
        inishie_pal_base = (const u8 *)pal->data;
        break;
    }
    case NDS_SDL_ROUTE_INISHIE_POWBLOCK:
    {
        const NDSRelocLoadedFile *pal;
        const u8 *pow_base = (const u8 *)loaded->data;

        if ((owner->id != nGCCommonKindItem) ||
            (sNdsRendererAdapterItemSubmitActive == FALSE) ||
            (dobj->mobj != NULL))
        {
            return FALSE;
        }
        pal = ndsRelocFindLoadedFileContaining(
            (const void *)(uintptr_t)dl[8].words.w1, 1u);
        if ((pal == NULL) || (pal->data == NULL) ||
            (pal->asset_id != NDS_NATIVE_INISHIE_POWBLOCK_PAL_ASSET) ||
            (pal->data_size < NDS_NATIVE_INISHIE_POWBLOCK_TLUT_END))
        {
            return FALSE;
        }
        inishie_pal_base = (const u8 *)pal->data;
        if ((dl[8].words.w1 != (u32)(uintptr_t)(inishie_pal_base +
                 NDS_NATIVE_INISHIE_POWBLOCK_TLUT_OFFSET)) ||
            (dl[14].words.w1 != (u32)(uintptr_t)(pow_base +
                 NDS_NATIVE_INISHIE_POWBLOCK_IMAGE_A_OFFSET)) ||
            (dl[26].words.w1 != (u32)(uintptr_t)(pow_base +
                 NDS_NATIVE_INISHIE_POWBLOCK_IMAGE_B_OFFSET)))
        {
            return FALSE;
        }
        break;
    }
#endif
#if NDS_P2_ITEM_CORE
    case NDS_SDL_ROUTE_BAKED:
        if ((owner->id != nGCCommonKindGroundDisplay) ||
            (sNdsRendererAdapterItemSubmitActive != FALSE))
        {
            return FALSE;
        }
        break;
    case NDS_SDL_ROUTE_BAKED_ITEM:
        if ((owner->id != nGCCommonKindItem) ||
            (sNdsRendererAdapterItemSubmitActive == FALSE))
        {
            return FALSE;
        }
        break;
#if NDS_P2_1P_GAME
    case NDS_SDL_ROUTE_BAKED_ROOM:
        if ((sNdsRendererAdapterItemSubmitActive != FALSE) ||
            (ndsNativeBakedRootIsRoom(
                 (const void *)(uintptr_t)route->root) == FALSE))
        {
            return FALSE;
        }
        break;
#endif
#endif
#if NDS_P2_STAGE_CASTLE
    case NDS_SDL_ROUTE_CASTLE_BUMPER:
    {
        ITStruct *ip;

        if ((owner->id != nGCCommonKindItem) ||
            (sNdsRendererAdapterItemSubmitActive == FALSE) ||
            (dobj->mobj == NULL) || (dobj->mobj->next != NULL) ||
            (ndsRendererAdapterMaterialFlags(dobj->mobj) !=
                 NDS_NATIVE_CASTLE_BUMPER_MOBJ_FLAGS) ||
            (route->data_size < NDS_NATIVE_CASTLE_BUMPER_IMAGE_END))
        {
            return FALSE;
        }
        ip = itGetStruct(owner);
        if ((ip == NULL) || (ip->kind != nITKindGBumper) ||
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 dobj->mobj, &bumper_material, FALSE, NULL, NULL) == FALSE) ||
            (bumper_material.effects !=
                 NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE))
        {
            return FALSE;
        }
        break;
    }
#endif
#if NDS_P2_ITEM_CORE
    case NDS_SDL_ROUTE_FFLOWER_LIVE:
    {
        ITStruct *ip;

        if ((owner->id != nGCCommonKindItem) ||
            (sNdsRendererAdapterItemSubmitActive == FALSE) ||
            (dobj->mobj == NULL) || (dobj->mobj->next != NULL) ||
            (ndsRendererAdapterMaterialFlags(dobj->mobj) !=
                 NDS_NATIVE_ITEM_FFLOWER_MOBJ_FLAGS) ||
            (route->data_size < NDS_NATIVE_ITEM_FFLOWER_FILE_END))
        {
            return FALSE;
        }
        ip = itGetStruct(owner);
        if ((ip == NULL) || (ip->kind != nITKindFFlower) ||
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 dobj->mobj, &item_route_material, FALSE, NULL, NULL) == FALSE) ||
            (item_route_material.effects !=
                 NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE))
        {
            return FALSE;
        }
        break;
    }
#if NDS_P2_STAGE_YAMABUKI
    case NDS_SDL_ROUTE_HITOKAGE:
    case NDS_SDL_ROUTE_FUSHIGIBANA:
    {
        ITStruct *ip;

        if ((owner->id != nGCCommonKindItem) ||
            (sNdsRendererAdapterItemSubmitActive == FALSE) ||
            (dobj->mobj == NULL))
        {
            return FALSE;
        }
        ip = itGetStruct(owner);
        if ((ip == NULL) ||
            (ip->kind != ((route_kind == NDS_SDL_ROUTE_HITOKAGE) ?
                              nITKindHitokage : nITKindFushigibana)) ||
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 dobj->mobj, &item_route_material, FALSE, NULL, NULL) == FALSE) ||
            (item_route_material.effects !=
                 NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE))
        {
            return FALSE;
        }
        break;
    }
#endif
    case NDS_SDL_ROUTE_NBUMPER:
    {
        ITStruct *ip;

        if ((owner->id != nGCCommonKindItem) ||
            (sNdsRendererAdapterItemSubmitActive == FALSE) ||
            (dobj->mobj == NULL) || (dobj->mobj->next != NULL) ||
            (ndsRendererAdapterMaterialFlags(dobj->mobj) !=
                 NDS_NATIVE_ITEM_NBUMPER_MOBJ_FLAGS) ||
            (route->data_size < NDS_NATIVE_ITEM_NBUMPER_FILE_END))
        {
            return FALSE;
        }
        ip = itGetStruct(owner);
        if ((ip == NULL) || (ip->kind != nITKindNBumper) ||
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 dobj->mobj, &item_route_material, FALSE, NULL, NULL) == FALSE) ||
            (item_route_material.effects !=
                 NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE))
        {
            return FALSE;
        }
        break;
    }
#endif
    default:
    {
#if NDS_P2_ITEM_CORE
        ITStruct *ip;
        const u32 item_route = route_kind - NDS_SDL_ROUTE_ITEM;

        if ((item_route >= (u32)nNDSStageDLItemRouteCount) ||
            (sNdsRendererAdapterItemSubmitActive == FALSE) ||
            (owner->id != nGCCommonKindItem))
        {
            return FALSE;
        }
        ip = itGetStruct(owner);
        if ((ip == NULL) ||
            ((u32)ip->kind != sNdsStageDLItemRoutes[item_route].item_kind))
        {
            return FALSE;
        }
        break;
#else
        return FALSE;
#endif
    }
    }

#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    const sb32 lab_item = (route_kind >= NDS_SDL_ROUTE_ITEM) ? TRUE : FALSE;
    u32 lab_item_mark = cpuGetTiming();
#endif
    saved_graphics_heap_ptr = gSYTaskmanGraphicsHeap.ptr;
    ndsRendererAdapterPrepareInitialMatrices(dobj,
                                             (camera_gobj != NULL) ?
                                                 CObjGetStruct(camera_gobj) :
                                                 ((gGCCurrentCamera != NULL) ?
                                                      CObjGetStruct(
                                                          gGCCurrentCamera) :
                                                      NULL),
                                             TRUE,
                                             &projection, &projection_ptr,
                                             &modelview, &modelview_ptr);
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    if (lab_item != FALSE)
    {
        u32 lab_now = cpuGetTiming();

        NDS_DIAG(gNdsLabItemAcc[4] += lab_now - lab_item_mark);
        NDS_DIAG(gNdsLabItemAcc[8]++);
        NDS_DIAG(gNdsLabItemAcc[12] += gNdsLabPimAcc[1]);
        gNdsLabItemAcc[13] += gNdsLabPimAcc[0] + gNdsLabPimAcc[2] +
                              gNdsLabPimAcc[3];
        lab_item_mark = lab_now;
    }
#endif
    memset(&config, 0, sizeof(config));
    config.max_depth = 8u;
    config.max_commands = 8192u;
    config.max_list_commands = 512u;
    config.initial_projection = projection_ptr;
    config.initial_modelview = modelview_ptr;
    config.initial_geometry_mode = initial_geometry_mode;
    config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    config.validate_range = ndsRendererAdapterStageValidateRange;
    config.immutable_command_span = ndsRendererAdapterImmutableCommandSpan;
    config.resolve_branch = ndsFighterDLDrawResolveBranch;
    config.resolve_data = ndsFighterDLDrawResolveRendererData;
    /* The body hands its draw state here; no routed owner reads it. */
    config.user = NULL;
    /* The owners' split-camera contract, on this copy as on the body's. */
    if ((config.initial_projection == NULL) &&
        (config.initial_modelview != NULL))
    {
        ndsRendererAdapterMtxIdentity20p12(&identity);
        config.initial_projection = &identity;
    }
    else if ((config.initial_modelview == NULL) &&
             (config.initial_projection != NULL))
    {
        ndsRendererAdapterMtxIdentity20p12(&identity);
        config.initial_modelview = &identity;
    }
#if NDS_P2_ITEM_CORE
    /* P2-6 (2026-10-01): a ground display's baked root wholly outside the
     * view draws nothing; skip it before the stats reset, its MObj snapshots
     * and the submit (Board the Platforms submits ~26 a frame, most of them
     * off screen; see ndsNativeBakedRootOffscreen). */
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    u32 lab_offscreen = cpuGetTiming();
    sb32 lab_culled = ((route_kind == NDS_SDL_ROUTE_BAKED) ||
         (route_kind == NDS_SDL_ROUTE_BAKED_ITEM) ||
         (route_kind == NDS_SDL_ROUTE_BAKED_ROOM)) &&
        (ndsNativeBakedRootOffscreen((const void *)(uintptr_t)route->root,
                                     &config) != FALSE);
    NDS_DIAG(gNdsLabBakedAcc[0] += cpuGetTiming() - lab_offscreen);
    NDS_DIAG(gNdsLabBakedAcc[15] += (lab_culled != FALSE) ? 1u : 0u);
    if (lab_culled != FALSE)
#else
    if (((route_kind == NDS_SDL_ROUTE_BAKED) ||
         (route_kind == NDS_SDL_ROUTE_BAKED_ITEM) ||
         (route_kind == NDS_SDL_ROUTE_BAKED_ROOM)) &&
        (ndsNativeBakedRootOffscreen((const void *)(uintptr_t)route->root,
                                     &config) != FALSE))
#endif
    {
        gSYTaskmanGraphicsHeap.ptr = saved_graphics_heap_ptr;
        NDS_DIAG(gNdsStageDLFastLaneHits++);
        return TRUE;
    }
#endif
#if NDS_P2_STAGE_CASTLE
    /* P2-6 (2026-10-02): the same exit for a bumper quad wholly outside the
     * view (see ndsNativeCastleBumperOffscreen). */
    if ((route_kind == NDS_SDL_ROUTE_CASTLE_BUMPER) &&
        (ndsNativeCastleBumperOffscreen(&config) != FALSE))
    {
        gSYTaskmanGraphicsHeap.ptr = saved_graphics_heap_ptr;
        NDS_DIAG(gNdsStageDLFastLaneHits++);
        return TRUE;
    }
#endif
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    u32 lab_prep = cpuGetTiming();
#endif
    render_stats = &sNdsRendererAdapterStagePersistentStats;
    ndsFighterDLDrawResetRuntimeRendererStats(render_stats);
    NDS_DIAG(gNdsStageGCDrawAllLoopHardwareCarrySeedCount++);

#if NDS_R2_IMPACT_WAVE_NATIVE
    if (route_kind == NDS_SDL_ROUTE_VISUAL)
    {
        /* The body's effect-submit seeds, then its visual owner. */
        if ((sNdsRendererAdapterEffectColorMask & 1u) != 0u)
        {
            render_stats->prim_color = sNdsRendererAdapterEffectPrimColor;
        }
        if ((sNdsRendererAdapterEffectColorMask & 2u) != 0u)
        {
            render_stats->env_color = sNdsRendererAdapterEffectEnvColor;
        }
        if (sNdsRendererAdapterEffectOtherModeValid != 0u)
        {
            render_stats->othermode_l = sNdsRendererAdapterEffectOtherModeL;
        }
        gNdsEffectDLSubmitOtherModeIn = render_stats->othermode_l;
        handled = ndsRendererSubmitNativeVisualEffect(
            sNdsRendererAdapterVisualEffectTemplate, &config, render_stats);
        if (handled != FALSE)
        {
            NDS_DIAG(gNdsVisualEffectNativeDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsVisualEffectNativeDeclineCount++);
            ndsStageRejectNativeRender(dobj, dl,
                NDS_NATIVE_FAILURE_REJECTED_PROGRAM, render_stats);
            /* Recorded with its own reason, as the body settles it. */
            handled = TRUE;
        }
        gNdsEffectDLSubmitOtherModeOut = render_stats->othermode_l;
        NDS_DIAG(gNdsEffectDLSubmitCount++);
        NDS_DIAG(gNdsEffectDLPublishCount++);
    }
    else
#endif
    if (route_kind == NDS_SDL_ROUTE_CHARGE_SHOT)
    {
        handled = ndsRendererSubmitNativeSamusChargeShot(
            loaded->data, loaded->data_size, &config, render_stats);
        if (handled != FALSE)
        {
            NDS_DIAG(gNdsChargeShotDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsChargeShotSubmitFailCount++);
        }
    }
#if NDS_P2_STAGE_SECTOR
    else if (route_kind == NDS_SDL_ROUTE_SECTOR_LASER)
    {
        handled = ndsRendererSubmitNativeSectorArwingLaser(
            laser_tex_base + NDS_NATIVE_SECTOR_LASER_TLUT_OFFSET,
            laser_tex_base + NDS_NATIVE_SECTOR_LASER_IMAGE_OFFSET, &config,
            render_stats);
        if (handled != FALSE)
        {
            NDS_DIAG(gNdsSectorLaserDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsSectorLaserSubmitFailCount++);
        }
    }
#endif
#if NDS_P2_STAGE_INISHIE && NDS_P2_ITEM_CORE
    else if (route_kind == NDS_SDL_ROUTE_INISHIE_PAKKUN)
    {
        ndsStageDLFastItemSeeds(render_stats);
        handled = ndsRendererSubmitNativeInishiePakkun(
            loaded->data, loaded->data_size, inishie_pal_base + 0x3620u,
            &item_route_material, &config, render_stats);
        if (handled != FALSE)
        {
            NDS_DIAG(gNdsInishiePakkunDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsInishiePakkunSubmitFailCount++);
        }
    }
    else if (route_kind == NDS_SDL_ROUTE_INISHIE_POWBLOCK)
    {
        const u8 *pow_base = (const u8 *)loaded->data;

        ndsStageDLFastItemSeeds(render_stats);
        handled = ndsRendererSubmitNativeInishiePowblock(
            inishie_pal_base + NDS_NATIVE_INISHIE_POWBLOCK_TLUT_OFFSET,
            pow_base + NDS_NATIVE_INISHIE_POWBLOCK_IMAGE_A_OFFSET,
            pow_base + NDS_NATIVE_INISHIE_POWBLOCK_IMAGE_B_OFFSET, &config,
            render_stats);
        if (handled != FALSE)
        {
            NDS_DIAG(gNdsInishiePowblockDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsInishiePowblockSubmitFailCount++);
        }
    }
#endif
#if NDS_P2_ITEM_CORE
    else if (route_kind == NDS_SDL_ROUTE_KIRBYSTAR)
    {
        if (sNdsRendererAdapterEffectSubmitActive != FALSE)
        {
            if ((sNdsRendererAdapterEffectColorMask & 1u) != 0u)
            {
                render_stats->prim_color = sNdsRendererAdapterEffectPrimColor;
            }
            if ((sNdsRendererAdapterEffectColorMask & 2u) != 0u)
            {
                render_stats->env_color = sNdsRendererAdapterEffectEnvColor;
            }
            if (sNdsRendererAdapterEffectOtherModeValid != 0u)
            {
                render_stats->othermode_l = sNdsRendererAdapterEffectOtherModeL;
            }
            gNdsEffectDLSubmitOtherModeIn = render_stats->othermode_l;
        }
        handled = ndsRendererSubmitNativeItemKirbyStar(
            loaded->data, loaded->data_size, NULL, &config, render_stats);
        if (handled != FALSE)
        {
            NDS_DIAG(gNdsItemKirbyStarDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsItemKirbyStarSubmitFailCount++);
        }
        if (sNdsRendererAdapterEffectSubmitActive != FALSE)
        {
            gNdsEffectDLSubmitOtherModeOut = render_stats->othermode_l;
            NDS_DIAG(gNdsEffectDLSubmitCount++);
            NDS_DIAG(gNdsEffectDLPublishCount++);
        }
    }
#endif
#if NDS_P2_PIKACHU
    else if (route_kind == NDS_SDL_ROUTE_PIKACHU_THUNDER)
    {
        /* Under the effect layer (the trail effect, the shock roots) the
         * body's effect-submit seeds come first, as for the Kirby star. */
        if (sNdsRendererAdapterEffectSubmitActive != FALSE)
        {
            if ((sNdsRendererAdapterEffectColorMask & 1u) != 0u)
            {
                render_stats->prim_color = sNdsRendererAdapterEffectPrimColor;
            }
            if ((sNdsRendererAdapterEffectColorMask & 2u) != 0u)
            {
                render_stats->env_color = sNdsRendererAdapterEffectEnvColor;
            }
            if (sNdsRendererAdapterEffectOtherModeValid != 0u)
            {
                render_stats->othermode_l = sNdsRendererAdapterEffectOtherModeL;
            }
            gNdsEffectDLSubmitOtherModeIn = render_stats->othermode_l;
        }
        handled = ndsRendererAdapterPikachuThunder(loaded, dobj, dl, &config,
                                                   render_stats);
        if (handled == FALSE)
        {
            /* Declined before drawing anything: the body decides. */
            gSYTaskmanGraphicsHeap.ptr = saved_graphics_heap_ptr;
            return FALSE;
        }
        if (sNdsRendererAdapterEffectSubmitActive != FALSE)
        {
            gNdsEffectDLSubmitOtherModeOut = render_stats->othermode_l;
            NDS_DIAG(gNdsEffectDLSubmitCount++);
            NDS_DIAG(gNdsEffectDLPublishCount++);
        }
    }
#endif
#if NDS_P2_NESS
    else if (route_kind == NDS_SDL_ROUTE_NESS_PKTHUNDER)
    {
        handled = ndsRendererSubmitNativeNessPKThunder(
            pkthunder_root_index, &pkthunder_material, pkthunder_trail_color,
            &config, render_stats);
        if (handled == FALSE)
        {
            /* Declined before drawing anything: the body decides. */
            gSYTaskmanGraphicsHeap.ptr = saved_graphics_heap_ptr;
            return FALSE;
        }
    }
#endif
    else if (route_kind == NDS_SDL_ROUTE_DAMAGE_FLY_MDUST)
    {
        /* The body's effect-submit seeds and witnesses, then its arm. */
        if (sNdsRendererAdapterEffectSubmitActive != FALSE)
        {
            if ((sNdsRendererAdapterEffectColorMask & 1u) != 0u)
            {
                render_stats->prim_color = sNdsRendererAdapterEffectPrimColor;
            }
            if ((sNdsRendererAdapterEffectColorMask & 2u) != 0u)
            {
                render_stats->env_color = sNdsRendererAdapterEffectEnvColor;
            }
            if (sNdsRendererAdapterEffectOtherModeValid != 0u)
            {
                render_stats->othermode_l = sNdsRendererAdapterEffectOtherModeL;
            }
            gNdsEffectDLSubmitOtherModeIn = render_stats->othermode_l;
        }
        if (ndsRendererAdapterSubmitDamageFlyMDust(
                dobj, loaded, &config, render_stats) == FALSE)
        {
            ndsStageRejectNativeRender(dobj, dl,
                NDS_NATIVE_FAILURE_REJECTED_PROGRAM, render_stats);
        }
        if (sNdsRendererAdapterEffectSubmitActive != FALSE)
        {
            gNdsEffectDLSubmitOtherModeOut = render_stats->othermode_l;
            NDS_DIAG(gNdsEffectDLSubmitCount++);
            NDS_DIAG(gNdsEffectDLPublishCount++);
        }
        /* Settled either way, as the body settles it. */
        handled = TRUE;
    }
#if NDS_P2_STAGE_CASTLE
    else if (route_kind == NDS_SDL_ROUTE_CASTLE_BUMPER)
    {
        ndsStageDLFastItemSeeds(render_stats);
        handled = ndsRendererSubmitNativeCastleBumper(
            loaded->data, loaded->data_size, &bumper_material, &config,
            render_stats);
        if (handled != FALSE)
        {
            NDS_DIAG(gNdsCastleBumperDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsCastleBumperSubmitFailCount++);
        }
    }
#endif
#if NDS_P2_ITEM_CORE
    else if (route_kind == NDS_SDL_ROUTE_FFLOWER_LIVE)
    {
        ndsStageDLFastItemSeeds(render_stats);
        handled = ndsRendererSubmitNativeItemFFlower(
            NDS_NATIVE_ITEM_FFLOWER_LIVE_ROOT, loaded->data,
            loaded->data_size, &item_route_material, &config, render_stats);
        if (handled != FALSE)
        {
            NDS_DIAG(gNdsItemFFlowerDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsItemFFlowerSubmitFailCount++);
        }
    }
#if NDS_P2_STAGE_YAMABUKI
    else if ((route_kind == NDS_SDL_ROUTE_HITOKAGE) ||
             (route_kind == NDS_SDL_ROUTE_FUSHIGIBANA))
    {
        ndsStageDLFastItemSeeds(render_stats);
        if (route_kind == NDS_SDL_ROUTE_HITOKAGE)
        {
            handled = ndsRendererSubmitNativeItemHitokage(
                loaded->data, loaded->data_size, &item_route_material,
                &config, render_stats);
            if (handled != FALSE)
            {
                NDS_DIAG(gNdsYamabukiHitokageDrawCount++);
            }
            else
            {
                NDS_DIAG(gNdsYamabukiHitokageSubmitFailCount++);
            }
        }
        else
        {
            handled = ndsRendererSubmitNativeItemFushigibana(
                loaded->data, loaded->data_size, &item_route_material,
                &config, render_stats);
            if (handled != FALSE)
            {
                NDS_DIAG(gNdsYamabukiFushigibanaDrawCount++);
            }
            else
            {
                NDS_DIAG(gNdsYamabukiFushigibanaSubmitFailCount++);
            }
        }
    }
#endif
    else if (route_kind == NDS_SDL_ROUTE_NBUMPER)
    {
        ndsStageDLFastItemSeeds(render_stats);
        handled = ndsRendererSubmitNativeItemNBumper(
            loaded->data, loaded->data_size, &item_route_material, &config,
            render_stats);
        if (handled != FALSE)
        {
            NDS_DIAG(gNdsItemNBumperDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsItemNBumperSubmitFailCount++);
        }
    }
    else if ((route_kind == NDS_SDL_ROUTE_BAKED) ||
             (route_kind == NDS_SDL_ROUTE_BAKED_ITEM) ||
             (route_kind == NDS_SDL_ROUTE_BAKED_ROOM))
    {
        /* The body's baked branch: the DObj's MObjs in order are the root's
         * live segment-E materials. */
        NDSRendererNativeMaterial baked_materials[
            NDS_NATIVE_BAKED_MATERIAL_SLOTS];
        MObj *mobj = dobj->mobj;
        u32 slots = (u32)route->pad;
        u32 i;

        if (route_kind == NDS_SDL_ROUTE_BAKED_ITEM)
        {
            ndsStageDLFastItemSeeds(render_stats);
        }

        for (i = 0u; (i < slots) && (i < NDS_NATIVE_BAKED_MATERIAL_SLOTS);
             i++)
        {
            if ((mobj == NULL) ||
                (ndsRendererAdapterBuildNativeMaterialSnapshot(
                     mobj, &baked_materials[i], FALSE, NULL, NULL) == FALSE))
            {
                break;
            }
            mobj = mobj->next;
        }
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
        NDS_DIAG(gNdsLabBakedAcc[1] += cpuGetTiming() - lab_prep);
#endif
        handled = (i == slots) ?
            ndsRendererSubmitNativeBaked(
                (const void *)(uintptr_t)route->root, loaded->data,
                loaded->data_size, baked_materials, i, &config,
                render_stats) :
            FALSE;
        if (handled != FALSE)
        {
            NDS_DIAG(gNdsItemBakedDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsItemBakedSubmitFailCount++);
        }
    }
    else
    {
        u32 head = (sNdsRendererAdapterItemSubmitHead <
                    NDS_RENDERER_STAGE_DL_HEADS) ?
            sNdsRendererAdapterItemSubmitHead : 0u;

        gNdsItemRendererLastHead = head;
        gNdsItemRendererLastColorMask =
            sNdsRendererAdapterItemColorMask[head];
        gNdsItemRendererLastEnvColor = sNdsRendererAdapterItemEnvColor[head];
        gNdsItemRendererLastOtherModeL =
            sNdsRendererAdapterItemOtherModeL[head];
        gNdsItemRendererLastOtherModeH =
            sNdsRendererAdapterItemOtherModeH[head];
        if ((sNdsRendererAdapterItemColorMask[head] & 1u) != 0u)
        {
            render_stats->prim_color = sNdsRendererAdapterItemPrimColor[head];
        }
        if ((sNdsRendererAdapterItemColorMask[head] & 2u) != 0u)
        {
            render_stats->env_color = sNdsRendererAdapterItemEnvColor[head];
        }
        if (sNdsRendererAdapterItemOtherModeLValid[head] != 0u)
        {
            render_stats->othermode_l =
                sNdsRendererAdapterItemOtherModeL[head];
        }
        if (sNdsRendererAdapterItemOtherModeHValid[head] != 0u)
        {
            render_stats->othermode_h =
                sNdsRendererAdapterItemOtherModeH[head];
        }
        const NDSStageDLItemRoute *item =
            &sNdsStageDLItemRoutes[route_kind - NDS_SDL_ROUTE_ITEM];
        u32 replay_first = 0u;
        const sb32 replay_record =
            ndsItemReplayRootBegin(dobj, dl, route_kind, &replay_first);

#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
        {
            u32 lab_now = cpuGetTiming();

            NDS_DIAG(gNdsLabItemAcc[5] += lab_now - lab_item_mark);
            lab_item_mark = lab_now;
        }
#endif
        handled = (item->submit_rooted != NULL) ?
            item->submit_rooted(route->root, loaded->data,
                                loaded->data_size, &config, render_stats) :
            item->submit(loaded->data, loaded->data_size, &config,
                         render_stats);
        if (replay_record != FALSE)
        {
            ndsItemReplayRootEnd(handled, replay_first);
        }
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
        {
            u32 lab_now = cpuGetTiming();

            NDS_DIAG(gNdsLabItemAcc[6] += lab_now - lab_item_mark);
            lab_item_mark = lab_now;
        }
#endif
        if (handled != FALSE)
        {
            (*item->draws)++;
        }
        else
        {
            (*item->fails)++;
        }
    }
#endif
    if (handled == FALSE)
    {
        /* The body's verdict for a drawn-nothing owner. */
        ndsStageRejectNativeRender(dobj, dl, NDS_NATIVE_FAILURE_NO_PROGRAM,
                                   render_stats);
    }
    ndsTaskmanSampleGraphicsHeap();
    gSYTaskmanGraphicsHeap.ptr = saved_graphics_heap_ptr;
    NDS_DIAG(gNdsStageGCDrawAllLoopHardwareCarryCaptureCount++);
    gNdsStageGCDrawAllLoopHardwareTriangleCount +=
        render_stats->hardware_triangle_count;
    gNdsStageGCDrawAllLoopHardwareZBufferTriangleCount +=
        render_stats->hardware_zbuffer_triangle_count;
    gNdsStageGCDrawAllLoopHardwareProjectedDepthTriangleCount +=
        render_stats->hardware_projected_depth_triangle_count;
    gNdsStageGCDrawAllLoopHardwareDecalDepthTriangleCount +=
        render_stats->hardware_decal_depth_triangle_count;
    gNdsStageGCDrawAllLoopHardwareTextureBindCount +=
        render_stats->hardware_texture_bind_count;
    gNdsStageGCDrawAllLoopHardwareTextureUploadCount +=
        render_stats->hardware_texture_upload_count;
    gNdsStageGCDrawAllLoopHardwareTextureReadyCount +=
        render_stats->hardware_texture_ready_count;
    gNdsStageGCDrawAllLoopHardwareTextureRejectCount +=
        render_stats->hardware_texture_reject_count;
    if (render_stats->hardware_texture_ready_count != 0u)
    {
        if (render_stats->hardware_texture_format < 32u)
        {
            gNdsStageGCDrawAllLoopHardwareTextureFormatMask |=
                1u << render_stats->hardware_texture_format;
        }
        if (render_stats->hardware_texture_width >
            gNdsStageGCDrawAllLoopHardwareTextureMaxWidth)
        {
            gNdsStageGCDrawAllLoopHardwareTextureMaxWidth =
                render_stats->hardware_texture_width;
        }
        if (render_stats->hardware_texture_height >
            gNdsStageGCDrawAllLoopHardwareTextureMaxHeight)
        {
            gNdsStageGCDrawAllLoopHardwareTextureMaxHeight =
                render_stats->hardware_texture_height;
        }
    }
    NDS_DIAG(gNdsStageDLFastLaneHits++);
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    if (lab_item != FALSE)
    {
        NDS_DIAG(gNdsLabItemAcc[7] += cpuGetTiming() - lab_item_mark);
    }
#endif
    return TRUE;
}

/* P2-2p8 (2026-10-04): the fast lane runs on the DTCM hot stack
 * (port/coroutine.h). Its owners' 3,000-byte traversal states and the
 * lane's own frames lived on the main-RAM stack, where the late-match lab
 * profile charged stack-line refills to the lane's pops and frame loads
 * (the lane ~10 cycles an instruction). Static reach from the lane is
 * 5,728 B at most (a baked owner's texture allocation through
 * glTexImage2D); IRQs run on their own stack. The lane reads no storage,
 * switches no coroutine and hands DMA only static buffers (texture scratch,
 * packet words). A lane reached from a subtree already on the hot stack
 * (the fighter display's magnifier) runs in place, as before. */

typedef struct NDSStageDLFastCall
{
    DObj *dobj;
    const Gfx *dl;
    GObj *camera_gobj;
    u32 initial_geometry_mode;
} NDSStageDLFastCall;

static unsigned int ndsRendererAdapterSubmitStageDLFastOnHotStack(void *arg)
{
    const NDSStageDLFastCall *call = (const NDSStageDLFastCall *)arg;

    return (unsigned int)ndsRendererAdapterSubmitStageDLFast(
        call->dobj, call->dl, call->camera_gobj,
        call->initial_geometry_mode);
}

/* The entry-effect owners the lane does not route (entry packets, the
 * rebirth halo's groups) take the same stack. */

static unsigned int ndsRendererAdapterTryNativeEntryEffectOnHotStack(void *arg)
{
    const NDSStageDLFastCall *call = (const NDSStageDLFastCall *)arg;

    return (unsigned int)ndsRendererAdapterTryNativeEntryEffect(
        call->dobj, call->dl, call->camera_gobj,
        call->initial_geometry_mode);
}
#endif

#if NDS_R2_REBIRTH_HALO_NATIVE && NDS_R2_REBIRTH_HALO_FAST_ADAPTER
/* P2-2p8 (2026-10-05): the RebirthHalo arm ahead of the body. It sat at the
 * top of ndsRendererAdapterSubmitStageDLBody, so its three lists (two on the
 * child DObj, one on the rotating grandchild) each paid the entry-effect
 * probe and the body's prologue -- a ~29 KB function's frame and locals --
 * before arriving at the native owner: ~100-160 frames a match after every
 * respawn, on every stage. The same statements now run from the dispatcher
 * right after the fast lane; a list they decline goes on to the entry probe
 * and the body as before (whose own copy of the arm stands down). Same-ROM
 * A/B word gNdsStageDLHaloFirst (0 = the arm in the body). */
volatile u32 gNdsStageDLHaloFirst __attribute__((used, section(".data"))) = 1u;

static sb32 __attribute__((noinline)) ndsRendererAdapterTryRebirthHalo(
    DObj *dobj, const Gfx *dl, GObj *camera_gobj, u32 initial_geometry_mode)
{
    if ((sNdsRendererAdapterRebirthHaloNativeActive != FALSE) &&
        (gEFManagerFiles[2] != NULL) &&
        ((const u8 *)dl >= (const u8 *)gEFManagerFiles[2]))
    {
        uintptr_t rebirth_offset = (uintptr_t)((const u8 *)dl -
                                               (const u8 *)gEFManagerFiles[2]);

        if ((rebirth_offset == 0x2378u) || (rebirth_offset == 0x2a88u) ||
            (rebirth_offset == 0x27e8u))
        {
            NDSRendererConfig rebirth_config = {0};
            NDSRendererStats rebirth_stats;
            NDSRendererStats *rebirth_render_stats;
            NDSRendererMatrix20p12 rebirth_projection;
            NDSRendererMatrix20p12 rebirth_modelview;
            const NDSRendererMatrix20p12 *rebirth_projection_ptr;
            const NDSRendererMatrix20p12 *rebirth_modelview_ptr;
#if NDS_RENDERER_HW_TRIANGLES
            void *rebirth_saved_graphics_heap_ptr = gSYTaskmanGraphicsHeap.ptr;
#endif

            /* 0x2378 and 0x2a88 are the two DL links on the SAME child DObj.
             * Once the first one has emitted both native roots with one matrix
             * setup, the tree walker will immediately offer 0x2a88 again. */
            if ((rebirth_offset == 0x2a88u) &&
                (sNdsRendererAdapterRebirthHaloSkipSecondChildList != FALSE))
            {
                sNdsRendererAdapterRebirthHaloSkipSecondChildList = FALSE;
                return TRUE;
            }

            ndsRendererAdapterPrepareInitialMatrices(
                dobj,
                (camera_gobj != NULL) ? CObjGetStruct(camera_gobj) :
                    ((gGCCurrentCamera != NULL) ? CObjGetStruct(gGCCurrentCamera) : NULL),
                TRUE,
                &rebirth_projection,
                &rebirth_projection_ptr,
                &rebirth_modelview,
                &rebirth_modelview_ptr);

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
            if (sNdsRendererAdapterStagePersistentActive != FALSE)
            {
                rebirth_render_stats = &sNdsRendererAdapterStagePersistentStats;
                ndsFighterDLDrawResetRuntimeRendererStats(rebirth_render_stats);
            }
            else
#endif
            {
                rebirth_render_stats = &rebirth_stats;
                ndsRendererInitStats(rebirth_render_stats);
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
                if (sNdsRendererAdapterStagePersistentActive != FALSE)
                {
                    ndsFighterDLDrawCopyPersistentRendererState(
                        rebirth_render_stats, &sNdsRendererAdapterStagePersistentStats);
                }
#endif
            }
            if ((sNdsRendererAdapterEffectColorMask & 1u) != 0u)
            {
                rebirth_render_stats->prim_color = sNdsRendererAdapterEffectPrimColor;
            }
            if ((sNdsRendererAdapterEffectColorMask & 2u) != 0u)
            {
                rebirth_render_stats->env_color = sNdsRendererAdapterEffectEnvColor;
            }
            if (sNdsRendererAdapterEffectOtherModeValid != 0u)
            {
                rebirth_render_stats->othermode_l = sNdsRendererAdapterEffectOtherModeL;
            }

            rebirth_config.max_depth = 8u;
            rebirth_config.max_commands = 8192u;
            rebirth_config.max_list_commands = 512u;
            rebirth_config.initial_projection = rebirth_projection_ptr;
            rebirth_config.initial_modelview = rebirth_modelview_ptr;
            rebirth_config.initial_geometry_mode = initial_geometry_mode;
            rebirth_config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;

            if (ndsRendererSubmitNativeRebirthHalo(
                    (u32)rebirth_offset, &rebirth_config,
                    rebirth_render_stats) != FALSE)
            {
                NDS_DIAG(gNdsRebirthHaloNativeDrawCount++);
                if (rebirth_offset == 0x2378u)
                {
                    /* Same DObj, same source matrix, adjacent source order.
                     * Keep the live renderer state produced by 0x2378 and emit
                     * its second linked list without rebuilding the adapter. */
                    if (ndsRendererSubmitNativeRebirthHalo(
                            0x2a88u, &rebirth_config,
                            rebirth_render_stats) != FALSE)
                    {
                        NDS_DIAG(gNdsRebirthHaloNativeDrawCount++);
                        sNdsRendererAdapterRebirthHaloSkipSecondChildList = TRUE;
                    }
                    else
                    {
                        NDS_DIAG(gNdsRebirthHaloNativeFallbackCount++);
                    }
                }
                gNdsStageGCDrawAllLoopHardwareTriangleCount +=
                    rebirth_render_stats->hardware_triangle_count;
                gNdsStageGCDrawAllLoopHardwareZBufferTriangleCount +=
                    rebirth_render_stats->hardware_zbuffer_triangle_count;
                gNdsStageGCDrawAllLoopHardwareProjectedDepthTriangleCount +=
                    rebirth_render_stats->hardware_projected_depth_triangle_count;
                gNdsStageGCDrawAllLoopHardwareDecalDepthTriangleCount +=
                    rebirth_render_stats->hardware_decal_depth_triangle_count;
                gNdsStageGCDrawAllLoopHardwareTextureBindCount +=
                    rebirth_render_stats->hardware_texture_bind_count;
                gNdsStageGCDrawAllLoopHardwareTextureUploadCount +=
                    rebirth_render_stats->hardware_texture_upload_count;
                gNdsStageGCDrawAllLoopHardwareTextureReadyCount +=
                    rebirth_render_stats->hardware_texture_ready_count;
                gNdsStageGCDrawAllLoopHardwareTextureRejectCount +=
                    rebirth_render_stats->hardware_texture_reject_count;
#if NDS_RENDERER_HW_TRIANGLES
                ndsTaskmanSampleGraphicsHeap();
                gSYTaskmanGraphicsHeap.ptr = rebirth_saved_graphics_heap_ptr;
#endif
                return TRUE;
            }
            NDS_DIAG(gNdsRebirthHaloNativeFallbackCount++);
#if NDS_RENDERER_HW_TRIANGLES
            ndsTaskmanSampleGraphicsHeap();
            gSYTaskmanGraphicsHeap.ptr = rebirth_saved_graphics_heap_ptr;
#endif
        }
    }
    return FALSE;
}
#else
volatile u32 gNdsStageDLHaloFirst __attribute__((used, section(".data"))) = 0u;
#endif

volatile u32 gNdsStageDLBodyCalls;

static void ndsRendererAdapterSubmitStageDLImpl(DObj *dobj, const Gfx *dl,
                                                 GObj *camera_gobj,
                                                 u32 initial_geometry_mode)
{
    if ((dobj == NULL) || (dl == NULL))
    {
        return;
    }
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    {
        NDSStageDLFastCall call = {
            dobj, dl, camera_gobj, initial_geometry_mode
        };

        if ((sb32)ndsDtcmHotStackRun(
                ndsRendererAdapterSubmitStageDLFastOnHotStack, &call) != FALSE)
        {
            return;
        }
    }
#endif
#if NDS_R2_REBIRTH_HALO_NATIVE && NDS_R2_REBIRTH_HALO_FAST_ADAPTER
    if ((gNdsStageDLHaloFirst != 0u) &&
        (sNdsRendererAdapterRebirthHaloNativeActive != FALSE) &&
        (ndsRendererAdapterTryRebirthHalo(dobj, dl, camera_gobj,
                                          initial_geometry_mode) != FALSE))
    {
        return;
    }
#endif
    {
        sb32 entry_handled;
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
        NDSStageDLFastCall call = {
            dobj, dl, camera_gobj, initial_geometry_mode
        };

        entry_handled = (sb32)ndsDtcmHotStackRun(
            ndsRendererAdapterTryNativeEntryEffectOnHotStack, &call);
#else
        entry_handled = ndsRendererAdapterTryNativeEntryEffect(
            dobj, dl, camera_gobj, initial_geometry_mode);
#endif

        if (entry_handled != FALSE)
        {
            return;
        }
    }
    NDS_DIAG(gNdsStageDLBodyCalls++);
    ndsRendererAdapterSubmitStageDLBody(dobj, dl, camera_gobj,
                                        initial_geometry_mode);
}

static void __attribute__((noinline)) ndsRendererAdapterSubmitStageDLBody(
    DObj *dobj, const Gfx *dl, GObj *camera_gobj, u32 initial_geometry_mode)
{
    NDSRelocLoadedFile *loaded;
    NDSRendererConfig config = {0};
    NDSRendererStats stats;
    NDSRendererStats *render_stats;
    NDSFighterDLDrawState state;
    NDSRendererCommandCallback callback;
    void *callback_user;
    NDSRendererMatrix20p12 initial_projection;
    NDSRendererMatrix20p12 initial_modelview;
    const NDSRendererMatrix20p12 *initial_projection_ptr;
    const NDSRendererMatrix20p12 *initial_modelview_ptr;
#if NDS_R2_IMPACT_WAVE_NATIVE
    NDSRendererNativeMaterial impact_wave_material;
    sb32 impact_wave_native_candidate = FALSE;
    sb32 impact_wave_native_handled = FALSE;
#endif
#if NDS_R2_REBIRTH_HALO_NATIVE
    u32 rebirth_halo_root_offset = 0u;
    sb32 rebirth_halo_native_candidate = FALSE;
    sb32 rebirth_halo_native_handled = FALSE;
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_INISHIE
    NDSRendererNativeMaterial inishie_pakkun_material;
    NDSRelocLoadedFile *inishie_pakkun_palette_file = NULL;
    sb32 inishie_pakkun_native_candidate = FALSE;
    sb32 inishie_pakkun_native_handled = FALSE;
    const void *inishie_powblock_tlut = NULL;
    const void *inishie_powblock_image_a = NULL;
    const void *inishie_powblock_image_b = NULL;
    sb32 inishie_powblock_native_candidate = FALSE;
    sb32 inishie_powblock_native_handled = FALSE;
#endif
#if NDS_RENDERER_HW_TRIANGLES
    sb32 charge_shot_native_candidate = FALSE;
    sb32 charge_shot_native_handled = FALSE;
    sb32 thunder_jolt_native_candidate = FALSE;
    sb32 thunder_jolt_native_handled = FALSE;
    NDSRendererNativeMaterial thunder_ground_material;
    u32 thunder_ground_root_index = 0u;
    sb32 thunder_ground_native_candidate = FALSE;
    sb32 thunder_ground_native_handled = FALSE;
    NDSRendererNativeMaterial thunder_fx_material;
    const void *thunder_fx_base = NULL;
    u32 thunder_fx_bytes = 0u;
    sb32 thunder_fx_native_candidate = FALSE;
    sb32 thunder_fx_native_handled = FALSE;
    NDSRendererNativeMaterial damage_slash_material;
    const void *damage_slash_base = NULL;
    u32 damage_slash_bytes = 0u;
    u32 damage_slash_root = 0u;
    sb32 damage_slash_native_seen = FALSE;
    sb32 damage_slash_native_candidate = FALSE;
    sb32 damage_slash_native_handled = FALSE;
    sb32 damage_slash_native_settled = FALSE;
    sb32 damage_fly_mdust_native_seen = FALSE;
    sb32 damage_fly_mdust_native_settled = FALSE;
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_NESS
    NDSRendererNativeMaterial ness_pkfire_materials[2];
    sb32 ness_pkfire_native_candidate = FALSE;
    sb32 ness_pkfire_native_handled = FALSE;
    NDSRendererNativeMaterial ness_pkthunder_material;
    u32 ness_pkthunder_root_index = 0u;
    u32 ness_pkthunder_trail_color = 0u;
    sb32 ness_pkthunder_native_candidate = FALSE;
    sb32 ness_pkthunder_native_handled = FALSE;
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_PURIN
    NDSRendererNativeMaterial purin_sing_material;
    const void *purin_sing_image = NULL;
    u32 purin_sing_root_index = 0u;
    sb32 purin_sing_native_candidate = FALSE;
    sb32 purin_sing_native_handled = FALSE;
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_KIRBY
    sb32 kirby_vulcan_native_handled = FALSE;
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_PIKACHU
    sb32 pikachu_thunder_native_handled = FALSE;
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_SAMUS
    sb32 samus_bomb_native_handled = FALSE;
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_NESS
    sb32 ness_pktail_native_handled = FALSE;
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_YOSHI
    NDSRendererNativeMaterial yoshi_entryegg_material;
    const void *yoshi_entryegg_palette = NULL;
    sb32 yoshi_entryegg_native_candidate = FALSE;
    sb32 yoshi_entryegg_native_handled = FALSE;
    sb32 yoshi_egg_is_weapon = FALSE;
    sb32 yoshi_egg_native_candidate = FALSE;
    sb32 yoshi_egg_native_handled = FALSE;
    const void *yoshi_egglay_palette = NULL;
    const void *yoshi_egglay_image = NULL;
    sb32 yoshi_egglay_native_candidate = FALSE;
    sb32 yoshi_egglay_native_handled = FALSE;
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_CASTLE
    NDSRendererNativeMaterial castle_bumper_material;
    sb32 castle_bumper_native_candidate = FALSE;
    sb32 castle_bumper_native_handled = FALSE;
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_SECTOR
    const void *sector_laser_tlut = NULL;
    const void *sector_laser_image = NULL;
    sb32 sector_laser_native_candidate = FALSE;
    sb32 sector_laser_native_handled = FALSE;
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_LINK
    u32 link_bomb_root = 0u;
    sb32 link_bomb_native_candidate = FALSE;
    sb32 link_bomb_native_handled = FALSE;
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_YAMABUKI
    sb32 marumine_native_candidate = FALSE;
    sb32 marumine_native_handled = FALSE;
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_YAMABUKI && NDS_P2_ITEM_CORE
    sb32 glucky_native_candidate = FALSE;
    sb32 glucky_native_handled = FALSE;
    sb32 porygon_native_candidate = FALSE;
    sb32 porygon_native_handled = FALSE;
    NDSRendererNativeMaterial hitokage_material;
    sb32 hitokage_native_candidate = FALSE;
    sb32 hitokage_native_handled = FALSE;
    NDSRendererNativeMaterial fushigibana_material;
    sb32 fushigibana_native_candidate = FALSE;
    sb32 fushigibana_native_handled = FALSE;
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE
    sb32 item_tomato_native_candidate = FALSE;
    sb32 item_tomato_native_handled = FALSE;
    NDSRendererNativeMaterial item_star_material0;
    NDSRendererNativeMaterial item_star_material1;
    sb32 item_star_native_candidate = FALSE;
    sb32 item_star_native_handled = FALSE;
    u32 item_sword_root = 0u;
    sb32 item_sword_native_candidate = FALSE;
    sb32 item_sword_native_handled = FALSE;
    sb32 item_hammer_native_candidate = FALSE;
    sb32 item_hammer_native_handled = FALSE;
    u32 item_mball_root = 0u;
    NDSRendererNativeMaterial item_mball_material;
    sb32 item_mball_native_candidate = FALSE;
    sb32 item_mball_native_handled = FALSE;
    sb32 item_mball_from_effect = FALSE;
    sb32 item_kirbystar_native_candidate = FALSE;
    sb32 item_kirbystar_native_handled = FALSE;
    NDSRendererNativeMaterial item_gshell_material;
    sb32 item_gshell_native_candidate = FALSE;
    sb32 item_gshell_native_handled = FALSE;
    NDSRendererNativeMaterial item_rshell_material;
    sb32 item_rshell_native_candidate = FALSE;
    sb32 item_rshell_native_handled = FALSE;
    u32 item_bat_root = 0u;
    sb32 item_bat_native_candidate = FALSE;
    sb32 item_bat_native_handled = FALSE;
    u32 item_capsule_root = 0u;
    sb32 item_capsule_native_candidate = FALSE;
    sb32 item_capsule_native_handled = FALSE;
    NDSRendererNativeMaterial item_bombhei_material;
    sb32 item_bombhei_native_candidate = FALSE;
    sb32 item_bombhei_native_handled = FALSE;
    sb32 item_lgun_native_candidate = FALSE;
    sb32 item_lgun_native_handled = FALSE;
    sb32 item_harisen_native_candidate = FALSE;
    sb32 item_harisen_native_handled = FALSE;
    sb32 item_heart_native_candidate = FALSE;
    sb32 item_heart_native_handled = FALSE;
    u32 item_starrod_root = 0u;
    sb32 item_starrod_native_candidate = FALSE;
    sb32 item_starrod_native_handled = FALSE;
    u32 item_fflower_root = 0u;
    NDSRendererNativeMaterial item_fflower_material;
    sb32 item_fflower_native_candidate = FALSE;
    sb32 item_fflower_native_handled = FALSE;
    u32 item_msbomb_root = 0u;
    sb32 item_msbomb_native_candidate = FALSE;
    sb32 item_msbomb_native_handled = FALSE;
    NDSRendererNativeMaterial item_nbumper_material;
    sb32 item_nbumper_native_candidate = FALSE;
    sb32 item_nbumper_native_handled = FALSE;
    sb32 item_box_native_candidate = FALSE;
    sb32 item_box_native_handled = FALSE;
    sb32 item_taru_native_candidate = FALSE;
    sb32 item_taru_native_handled = FALSE;
    sb32 item_egg_native_candidate = FALSE;
    sb32 item_egg_native_handled = FALSE;
    sb32 item_iwark_native_candidate = FALSE;
    sb32 item_iwark_native_handled = FALSE;
    sb32 item_baked_native_handled = FALSE;
#endif
    u32 visual_effect_template = 0u;
    sb32 visual_effect_native_candidate = FALSE;
    sb32 visual_effect_native_handled = FALSE;
    /* Wider than "handled" on purpose: TRUE when this owner either drew or
     * recorded its own precise REJECTED_PROGRAM failure. A decline must not
     * also trip the generic NO_PROGRAM guards below, or one object publishes
     * two reasons for one event and the first-failure record names the
     * wrong one. */
    sb32 visual_effect_native_settled = FALSE;
    u32 effect_seed_before = 0u;
    u32 effect_matrix_cmd_before = 0u;
    u32 effect_xform_before = 0u;
    u32 effect_hw_vertex_before = 0u;
    u32 effect_hw_triangle_before = 0u;
#if NDS_RENDERER_HW_TRIANGLES
    void *saved_graphics_heap_ptr;
#if NDS_RENDERER_PROFILE_LEVEL < 2
    sb32 detailed_output;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 1
    u32 step_start;
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    u32 adapter_start;
    u32 adapter_ticks;
    NDSRendererOwnerStatsSnapshot owner_stats_before;
#endif
    sb32 inherited_texture = FALSE;
    sb32 inherited_tile = FALSE;
    sb32 inherited_segment = FALSE;
#endif
#if NDS_TICK_HUD && NDS_P2_EFFECT_CENSUS
    /* R2-08 phase split. Latched once at entry rather than re-read per phase:
     * the flag is cleared by the tree submit's own epilogue, and a phase that
     * started inside the effect layer must be charged to it whatever the flag
     * says by the time the phase ends. */
    sb32 phase_effect = FALSE;
    u32 phase_dl_mark = 0u;
    u32 phase_mark = 0u;
#endif

    if ((dobj == NULL) || (dl == NULL))
    {
        return;
    }

    /* Entry models execute their generated native owners (admitted by the
     * caller). Unhandled required roots below report a native failure; no
     * interpreter is available. */

#if NDS_R2_REBIRTH_HALO_NATIVE && NDS_R2_REBIRTH_HALO_FAST_ADAPTER
    /* RebirthHalo is already identified by the effect-tree owner before any
     * child list reaches here. Its six generated groups contain every source
     * state/texture/vertex dependency, so do not pay the generic adapter's
     * loaded-file scan, segment-E material preparation, callback context and
     * command-interpreter setup merely to arrive at the native submitter.
     *
     * Keep this per-DObj for the experiment: it preserves the exact world
     * matrix of the child and rotating grandchild while isolating the cost of
     * generic adapter ceremony. A later all-tree owner can merge the duplicate
     * child matrix/load once this gate has a visual/tick verdict. */
    if ((gNdsStageDLHaloFirst == 0u) &&
        (sNdsRendererAdapterRebirthHaloNativeActive != FALSE) &&
        (gEFManagerFiles[2] != NULL) &&
        ((const u8 *)dl >= (const u8 *)gEFManagerFiles[2]))
    {
        uintptr_t rebirth_offset = (uintptr_t)((const u8 *)dl -
                                               (const u8 *)gEFManagerFiles[2]);

        if ((rebirth_offset == 0x2378u) || (rebirth_offset == 0x2a88u) ||
            (rebirth_offset == 0x27e8u))
        {
            NDSRendererConfig rebirth_config = {0};
            NDSRendererStats rebirth_stats;
            NDSRendererStats *rebirth_render_stats;
            NDSRendererMatrix20p12 rebirth_projection;
            NDSRendererMatrix20p12 rebirth_modelview;
            const NDSRendererMatrix20p12 *rebirth_projection_ptr;
            const NDSRendererMatrix20p12 *rebirth_modelview_ptr;
#if NDS_RENDERER_HW_TRIANGLES
            void *rebirth_saved_graphics_heap_ptr = gSYTaskmanGraphicsHeap.ptr;
#endif

            /* 0x2378 and 0x2a88 are the two DL links on the SAME child DObj.
             * Once the first one has emitted both native roots with one matrix
             * setup, the tree walker will immediately offer 0x2a88 again. */
            if ((rebirth_offset == 0x2a88u) &&
                (sNdsRendererAdapterRebirthHaloSkipSecondChildList != FALSE))
            {
                sNdsRendererAdapterRebirthHaloSkipSecondChildList = FALSE;
                return;
            }

            ndsRendererAdapterPrepareInitialMatrices(
                dobj,
                (camera_gobj != NULL) ? CObjGetStruct(camera_gobj) :
                    ((gGCCurrentCamera != NULL) ? CObjGetStruct(gGCCurrentCamera) : NULL),
                TRUE,
                &rebirth_projection,
                &rebirth_projection_ptr,
                &rebirth_modelview,
                &rebirth_modelview_ptr);

#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
            if (sNdsRendererAdapterStagePersistentActive != FALSE)
            {
                rebirth_render_stats = &sNdsRendererAdapterStagePersistentStats;
                ndsFighterDLDrawResetRuntimeRendererStats(rebirth_render_stats);
            }
            else
#endif
            {
                rebirth_render_stats = &rebirth_stats;
                ndsRendererInitStats(rebirth_render_stats);
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 2)
                if (sNdsRendererAdapterStagePersistentActive != FALSE)
                {
                    ndsFighterDLDrawCopyPersistentRendererState(
                        rebirth_render_stats, &sNdsRendererAdapterStagePersistentStats);
                }
#endif
            }
            if ((sNdsRendererAdapterEffectColorMask & 1u) != 0u)
            {
                rebirth_render_stats->prim_color = sNdsRendererAdapterEffectPrimColor;
            }
            if ((sNdsRendererAdapterEffectColorMask & 2u) != 0u)
            {
                rebirth_render_stats->env_color = sNdsRendererAdapterEffectEnvColor;
            }
            if (sNdsRendererAdapterEffectOtherModeValid != 0u)
            {
                rebirth_render_stats->othermode_l = sNdsRendererAdapterEffectOtherModeL;
            }

            rebirth_config.max_depth = 8u;
            rebirth_config.max_commands = 8192u;
            rebirth_config.max_list_commands = 512u;
            rebirth_config.initial_projection = rebirth_projection_ptr;
            rebirth_config.initial_modelview = rebirth_modelview_ptr;
            rebirth_config.initial_geometry_mode = initial_geometry_mode;
            rebirth_config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;

            if (ndsRendererSubmitNativeRebirthHalo(
                    (u32)rebirth_offset, &rebirth_config,
                    rebirth_render_stats) != FALSE)
            {
                NDS_DIAG(gNdsRebirthHaloNativeDrawCount++);
                if (rebirth_offset == 0x2378u)
                {
                    /* Same DObj, same source matrix, adjacent source order.
                     * Keep the live renderer state produced by 0x2378 and emit
                     * its second linked list without rebuilding the adapter. */
                    if (ndsRendererSubmitNativeRebirthHalo(
                            0x2a88u, &rebirth_config,
                            rebirth_render_stats) != FALSE)
                    {
                        NDS_DIAG(gNdsRebirthHaloNativeDrawCount++);
                        sNdsRendererAdapterRebirthHaloSkipSecondChildList = TRUE;
                    }
                    else
                    {
                        NDS_DIAG(gNdsRebirthHaloNativeFallbackCount++);
                    }
                }
                gNdsStageGCDrawAllLoopHardwareTriangleCount +=
                    rebirth_render_stats->hardware_triangle_count;
                gNdsStageGCDrawAllLoopHardwareZBufferTriangleCount +=
                    rebirth_render_stats->hardware_zbuffer_triangle_count;
                gNdsStageGCDrawAllLoopHardwareProjectedDepthTriangleCount +=
                    rebirth_render_stats->hardware_projected_depth_triangle_count;
                gNdsStageGCDrawAllLoopHardwareDecalDepthTriangleCount +=
                    rebirth_render_stats->hardware_decal_depth_triangle_count;
                gNdsStageGCDrawAllLoopHardwareTextureBindCount +=
                    rebirth_render_stats->hardware_texture_bind_count;
                gNdsStageGCDrawAllLoopHardwareTextureUploadCount +=
                    rebirth_render_stats->hardware_texture_upload_count;
                gNdsStageGCDrawAllLoopHardwareTextureReadyCount +=
                    rebirth_render_stats->hardware_texture_ready_count;
                gNdsStageGCDrawAllLoopHardwareTextureRejectCount +=
                    rebirth_render_stats->hardware_texture_reject_count;
#if NDS_RENDERER_HW_TRIANGLES
                ndsTaskmanSampleGraphicsHeap();
                gSYTaskmanGraphicsHeap.ptr = rebirth_saved_graphics_heap_ptr;
#endif
                return;
            }
            NDS_DIAG(gNdsRebirthHaloNativeFallbackCount++);
#if NDS_RENDERER_HW_TRIANGLES
            ndsTaskmanSampleGraphicsHeap();
            gSYTaskmanGraphicsHeap.ptr = rebirth_saved_graphics_heap_ptr;
#endif
        }
    }
#endif

#if NDS_TICK_HUD && NDS_P2_EFFECT_CENSUS
    phase_effect =
        (sNdsRendererAdapterEffectSubmitActive != FALSE) ? TRUE : FALSE;
    if (phase_effect != FALSE)
    {
        gNdsEffectPhaseDLCount++;
        phase_dl_mark = cpuGetTiming();
        phase_mark = phase_dl_mark;
    }
#endif
    loaded = ndsRelocFindLoadedFileContaining(dl, sizeof(*dl));
    if ((loaded == NULL) &&
        (ndsFighterDLScanRangeInTaskmanArena(dl, sizeof(*dl)) == FALSE))
    {
        ndsStageRejectNativeRender(dobj, dl, NDS_NATIVE_FAILURE_BAD_ASSET, NULL);
#if NDS_TICK_HUD && NDS_P2_EFFECT_CENSUS
        /* The REJECT exit still costs a full loaded-file scan plus an arena
         * scan, so it is charged rather than dropped -- an unmeasured early
         * return is exactly how a phase split acquires a residual. */
        if (phase_effect != FALSE)
        {
            gNdsEffectPhaseFindTicks += cpuGetTiming() - phase_mark;
            gNdsEffectPhaseDLTicks += cpuGetTiming() - phase_dl_mark;
        }
#endif
        return;
    }
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_INISHIE
    /* File 155 root 0x0B40 is not a scale-platform root.  BattleShip's
     * GRInishieMap points the scale map_nodes at 0x05F0; 0x0B40 is the
     * Pakkun ITAttributes DObjDesc child.  Item GObjs are outside the
     * whole-stage capture, so claim this exact item/root here before the
     * generic material-segment preparation can manufacture segment-E Gfx. */
    /* Step-witnessed, because the stage still records NO_PROGRAM at this root
     * and a single nine-clause test cannot say WHICH clause declined. The
     * highest step reached is the one that matters: 9 means every clause
     * passed and the decline is inside the submit itself. */
    if ((loaded != NULL) && (loaded->asset_id == 155u) &&
        (ndsRelocNativeRootOffset(loaded, dl) == 0x0b40u))
    {
        u32 pakkun_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            pakkun_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                pakkun_step = 3u;
                if ((dobj->mobj != NULL) && (dobj->mobj->next == NULL))
                {
                    pakkun_step = 4u;
                    gNdsInishiePakkunMaterialFlags =
                        ndsRendererAdapterMaterialFlags(dobj->mobj);
                    if (gNdsInishiePakkunMaterialFlags == 0x0001u)
                    {
                        pakkun_step = 5u;
                        if (ndsRendererAdapterBuildNativeMaterialSnapshot(
                                dobj->mobj, &inishie_pakkun_material, FALSE,
                                NULL, NULL) != FALSE)
                        {
                            pakkun_step = 6u;
                            gNdsInishiePakkunEffects =
                                inishie_pakkun_material.effects;
                            if (inishie_pakkun_material.effects ==
                                NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE)
                            {
                                NDSRelocLoadedFile *image_file =
                                    ndsRelocFindLoadedFileContaining(
                                        (const void *)(uintptr_t)
                                            inishie_pakkun_material
                                                .current_image, 1u);

                                pakkun_step = 7u;
                                if (image_file == loaded)
                                {
                                    pakkun_step = 8u;
                                    inishie_pakkun_palette_file =
                                        ndsRelocFindLoadedFileByAsset(107u);
                                    if ((inishie_pakkun_palette_file != NULL) &&
                                        (inishie_pakkun_palette_file->data_size >=
                                         0x3620u + 32u))
                                    {
                                        pakkun_step = 9u;
                                        inishie_pakkun_native_candidate = TRUE;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        if (pakkun_step > gNdsInishiePakkunCandidateStep)
        {
            gNdsInishiePakkunCandidateStep = pakkun_step;
        }
    }
    /* File 155 root 0x10D0 is the POW block list, and it is the ONLY thing in
     * the game data that can arrive here with this asset and root.
     * Whole-image pointer census: the only pointer to 155:0x10D0 is file
     * 155's own internal fixup 0x1228 -- the DObjDesc_0x11F8 child that
     * GRInishieMap's PowerBlock ITAttributes name as their data -- and no
     * external fixup anywhere targets it.  p_mobjsubs is NULL and the list
     * has no segment-E call, so an MObj here would be a different draw.
     * Step-witnessed: step 8 means every clause passed and any decline is
     * inside the submit itself.
     *
     * The two SETTIMG words are NOT in size order: word 14 binds the CI4 16x8
     * at 0x0F50 and word 26 the CI4 32x16 at 0x0E48.  Both are measured from
     * the relocated words, not inferred from the tile dimensions. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_INISHIE_POWBLOCK_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) ==
             NDS_NATIVE_INISHIE_POWBLOCK_ROOT))
    {
        u32 powblock_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            powblock_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                powblock_step = 3u;
                if (dobj->mobj == NULL)
                {
                    powblock_step = 4u;
                    if ((loaded->data != NULL) &&
                        (loaded->data_size >=
                         (NDS_NATIVE_INISHIE_POWBLOCK_ROOT +
                          NDS_NATIVE_INISHIE_POWBLOCK_DL_BYTES)) &&
                        (loaded->data_size >=
                         NDS_NATIVE_INISHIE_POWBLOCK_IMAGE_B_END))
                    {
                        powblock_step = 5u;
                        if ((dl[8].words.w0 ==
                             NDS_NATIVE_INISHIE_POWBLOCK_TLUT_W0) &&
                            (dl[14].words.w0 ==
                             NDS_NATIVE_INISHIE_POWBLOCK_IMAGE_A_W0) &&
                            (dl[26].words.w0 ==
                             NDS_NATIVE_INISHIE_POWBLOCK_IMAGE_B_W0))
                        {
                            /* Resolve the palette file FROM the pointer the
                             * list carries, not by asset id: the same
                             * stronger test the laser owner documents above.
                             * An unrelocated chain word lands in no loaded
                             * file at all. */
                            NDSRelocLoadedFile *powblock_pal =
                                ndsRelocFindLoadedFileContaining(
                                    (const void *)(uintptr_t)dl[8].words.w1,
                                    1u);

                            powblock_step = 6u;
                            if ((powblock_pal != NULL) &&
                                (powblock_pal->data != NULL) &&
                                (powblock_pal->asset_id ==
                                 NDS_NATIVE_INISHIE_POWBLOCK_PAL_ASSET) &&
                                (powblock_pal->data_size >=
                                 NDS_NATIVE_INISHIE_POWBLOCK_TLUT_END))
                            {
                                const u8 *pal_base =
                                    (const u8 *)powblock_pal->data;
                                const u8 *pow_base =
                                    (const u8 *)loaded->data;

                                powblock_step = 7u;
                                /* COMPARE the relocated pointers, never
                                 * assume them: the TLUT word must land in
                                 * file 107 at 0x35f8 and both image words
                                 * must land in this loaded file. */
                                if ((dl[8].words.w1 ==
                                     (u32)(uintptr_t)(pal_base +
                                      NDS_NATIVE_INISHIE_POWBLOCK_TLUT_OFFSET)) &&
                                    (dl[14].words.w1 ==
                                     (u32)(uintptr_t)(pow_base +
                                      NDS_NATIVE_INISHIE_POWBLOCK_IMAGE_A_OFFSET)) &&
                                    (dl[26].words.w1 ==
                                     (u32)(uintptr_t)(pow_base +
                                      NDS_NATIVE_INISHIE_POWBLOCK_IMAGE_B_OFFSET)))
                                {
                                    powblock_step = 8u;
                                    inishie_powblock_tlut = pal_base +
                                        NDS_NATIVE_INISHIE_POWBLOCK_TLUT_OFFSET;
                                    inishie_powblock_image_a = pow_base +
                                        NDS_NATIVE_INISHIE_POWBLOCK_IMAGE_A_OFFSET;
                                    inishie_powblock_image_b = pow_base +
                                        NDS_NATIVE_INISHIE_POWBLOCK_IMAGE_B_OFFSET;
                                    inishie_powblock_native_candidate = TRUE;
                                }
                            }
                        }
                    }
                }
            }
        }
        if (powblock_step > gNdsInishiePowblockCandidateStep)
        {
            gNdsInishiePowblockCandidateStep = powblock_step;
        }
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_SECTOR
    /* File 153 root 0x1c50 is the ArwingLaser weapon list, and it is the ONLY
     * thing in the game data that can arrive here with this asset and root.
     * Whole-image pointer census: the only two pointers to 153:0x1c50 are
     * GRSectorMap external fixups 0x00bc and 0x00f0 -- the ArwingLaser2D/3D
     * WPAttributes.data fields -- and no internal fixup inside file 153
     * targets it.  Both kinds share the list and differ in no drawn respect,
     * so one owner serves both and the sharing is not an ambiguity.
     * Step-witnessed: a count alone cannot say WHICH clause declined, and
     * step 7 means every clause passed. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_SECTOR_LASER_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_SECTOR_LASER_ROOT))
    {
        u32 laser_step = 1u;

        if ((dobj->parent_gobj != NULL) &&
            (dobj->parent_gobj->id == nGCCommonKindWeapon))
        {
            laser_step = 2u;
            /* material 0 in the recorded failure: this list carries its own
             * immutable binding, so an MObj here would be a different draw. */
            if (dobj->mobj == NULL)
            {
                laser_step = 3u;
                if (loaded->data_size >=
                    (NDS_NATIVE_SECTOR_LASER_ROOT +
                     NDS_NATIVE_SECTOR_LASER_DL_BYTES))
                {
                    laser_step = 4u;
                    if ((dl[8].words.w0 == NDS_NATIVE_SECTOR_LASER_TLUT_W0) &&
                        (dl[14].words.w0 == NDS_NATIVE_SECTOR_LASER_IMAGE_W0))
                    {
                        /* Resolve the texture file FROM the pointer the list
                         * carries, not by asset id: a by-asset lookup would
                         * still admit a list whose fixup never ran, and the
                         * only public accessor is the containing-file one the
                         * reject path already uses.  This is the stronger
                         * test -- an unrelocated chain word lands in no loaded
                         * file at all. */
                        NDSRelocLoadedFile *laser_tex =
                            ndsRelocFindLoadedFileContaining(
                                (const void *)(uintptr_t)dl[14].words.w1, 1u);

                        laser_step = 5u;
                        if ((laser_tex != NULL) && (laser_tex->data != NULL) &&
                            (laser_tex->asset_id ==
                             NDS_NATIVE_SECTOR_LASER_TEX_ASSET) &&
                            (laser_tex->data_size >=
                             NDS_NATIVE_SECTOR_LASER_TEX_END))
                        {
                            const u8 *tex_base = (const u8 *)laser_tex->data;

                            laser_step = 6u;
                            /* The loader's external-fixup pass rewrites these
                             * two words in place.  If it never ran they are
                             * still chain words, so COMPARE the relocated
                             * pointers -- never assume them, and never bind a
                             * chain word as an image. */
                            if ((dl[8].words.w1 == (u32)(uintptr_t)(tex_base +
                                    NDS_NATIVE_SECTOR_LASER_TLUT_OFFSET)) &&
                                (dl[14].words.w1 == (u32)(uintptr_t)(tex_base +
                                    NDS_NATIVE_SECTOR_LASER_IMAGE_OFFSET)))
                            {
                                laser_step = 7u;
                                sector_laser_tlut = tex_base +
                                    NDS_NATIVE_SECTOR_LASER_TLUT_OFFSET;
                                sector_laser_image = tex_base +
                                    NDS_NATIVE_SECTOR_LASER_IMAGE_OFFSET;
                                sector_laser_native_candidate = TRUE;
                            }
                        }
                    }
                }
            }
        }
        if (laser_step > gNdsSectorLaserCandidateStep)
        {
            gNdsSectorLaserCandidateStep = laser_step;
        }
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_CASTLE
    /* File 86 root 0x7558 is the bumper quad.  TWO item kinds reach it with
     * IDENTICAL evidence: 251_ITCommonData.c:934 (NBumper) and :1843
     * (GBumper) carry the same DObjDesc and the same p_mobjsubs, and file 86
     * holds exactly ONE pointer to this root.  itNBumperAttachedInitVars
     * swaps dobj->dl to the 0x7AF8 wait list in status 5 alone; in every
     * other NBumper status the drawn list IS this one.  Asset, root and MObj
     * therefore cannot discriminate, and nITKindNBumper is already registered
     * in the live maker table, so this is a present hazard and not a future
     * one.  Read the live kind. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_CASTLE_BUMPER_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) ==
             NDS_NATIVE_CASTLE_BUMPER_ROOT))
    {
        u32 bumper_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            bumper_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *bumper_ip = itGetStruct(dobj->parent_gobj);

                bumper_step = 3u;
                if (bumper_ip != NULL)
                {
                    bumper_step = 4u;
                    gNdsCastleBumperItemKind = (u32)bumper_ip->kind;
                    if (bumper_ip->kind != nITKindGBumper)
                    {
                        /* THE DISCRIMINATOR.  An NBumper is a different item
                         * with its own statuses, spin, throw physics and
                         * second display list; it gets its own owner, never
                         * this one.  Decline and let the loud NO_PROGRAM
                         * reject below record it, with a counter that says
                         * WHY rather than leaving it indistinguishable from
                         * a GBumper the submit refused. */
                        NDS_DIAG(gNdsCastleBumperForeignKindCount++);
                    }
                    else if ((dobj->mobj != NULL) &&
                             (dobj->mobj->next == NULL) &&
                             (ndsRendererAdapterMaterialFlags(dobj->mobj) ==
                                  NDS_NATIVE_CASTLE_BUMPER_MOBJ_FLAGS))
                    {
                        bumper_step = 5u;
                        if (ndsRendererAdapterBuildNativeMaterialSnapshot(
                                dobj->mobj, &castle_bumper_material, FALSE,
                                NULL, NULL) != FALSE)
                        {
                            bumper_step = 6u;
                            gNdsCastleBumperEffects =
                                castle_bumper_material.effects;
                            if (castle_bumper_material.effects ==
                                NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE)
                            {
                                bumper_step = 7u;
                                if (loaded->data_size >=
                                    NDS_NATIVE_CASTLE_BUMPER_IMAGE_END)
                                {
                                    bumper_step = 8u;
                                    castle_bumper_native_candidate = TRUE;
                                }
                            }
                        }
                    }
                }
            }
        }
        if (bumper_step > gNdsCastleBumperCandidateStep)
        {
            gNdsCastleBumperCandidateStep = bumper_step;
        }
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES
#if NDS_P2_NESS
    /* Ness PK Fire's WPAttributes live in NessSpecial1, but their `data`
     * pointer resolves to NessSpecial3 root 0x0168.  That root selects exactly
     * two live MObjs (segment-E slots 0 then 1), each LIGHT1|LIGHT2 only.
     * Preserve the source weapon/state machine and admit only that exact
     * cross-file root/material contract to the generated two-triangle owner. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_NESS_PKFIRE_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_NESS_PKFIRE_ROOT) &&
        (dobj->parent_gobj != NULL) &&
        (dobj->parent_gobj->id == nGCCommonKindWeapon))
    {
        WPStruct *pkfire_wp = wpGetStruct(dobj->parent_gobj);
        MObj *mobj = dobj->mobj;
        u32 i;

        if ((pkfire_wp != NULL) && (pkfire_wp->kind == nWPKindPKFire))
        {
            for (i = 0u; i < NDS_NATIVE_NESS_PKFIRE_GROUP_COUNT; i++)
            {
                if ((mobj == NULL) ||
                    (ndsRendererAdapterBuildNativeMaterialSnapshot(
                         mobj, &ness_pkfire_materials[i], FALSE,
                         NULL, NULL) == FALSE) ||
                    (ness_pkfire_materials[i].effects !=
                         NDS_NATIVE_NESS_PKFIRE_MATERIAL_EFFECTS))
                {
                    break;
                }
                mobj = mobj->next;
            }
            if ((i == NDS_NATIVE_NESS_PKFIRE_GROUP_COUNT) && (mobj == NULL) &&
                (loaded->data_size >= (NDS_NATIVE_NESS_PKFIRE_ROOT +
                                       NDS_NATIVE_NESS_PKFIRE_DL_BYTES)))
            {
                ness_pkfire_native_candidate = TRUE;
            }
        }
    }
    /* PK Thunder's head and trail are NessModel-owned weapons.  Both are
     * immutable two-triangle quads with one live CURRENT_IMAGE MObj; the
     * trail's proc_display supplies prim/env from its live trail_id.  Claim
     * only the exact source asset/root/kind tuples, leaving BattleShip in
     * charge of steering, collision, texture-id animation and transforms. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_NESS_PKTHUNDER_ASSET) &&
        (dobj->parent_gobj != NULL) &&
        (dobj->parent_gobj->id == nGCCommonKindWeapon) &&
        (dobj->mobj != NULL) && (dobj->mobj->next == NULL))
    {
        WPStruct *pkthunder_wp = wpGetStruct(dobj->parent_gobj);
        u32 root_offset = ndsRelocNativeRootOffset(loaded, dl);
        sb32 root_match = FALSE;

        if ((pkthunder_wp != NULL) &&
            (pkthunder_wp->kind == nWPKindPKThunderHead) &&
            (root_offset == NDS_NATIVE_NESS_PKTHUNDER_HEAD_ROOT))
        {
            ness_pkthunder_root_index =
                NDS_NATIVE_NESS_PKTHUNDER_HEAD_INDEX;
            root_match = TRUE;
        }
        else if ((pkthunder_wp != NULL) &&
                 (pkthunder_wp->kind == nWPKindPKThunderTrail) &&
                 (root_offset == NDS_NATIVE_NESS_PKTHUNDER_TRAIL_ROOT) &&
                 ((u32)pkthunder_wp->weapon_vars.pkthunder_trail.trail_id <
                  NDS_NATIVE_NESS_PKTHUNDER_TRAIL_COLOR_COUNT))
        {
            ness_pkthunder_root_index =
                NDS_NATIVE_NESS_PKTHUNDER_TRAIL_INDEX;
            ness_pkthunder_trail_color =
                (u32)pkthunder_wp->weapon_vars.pkthunder_trail.trail_id;
            root_match = TRUE;
        }
        if ((root_match != FALSE) &&
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 dobj->mobj, &ness_pkthunder_material, FALSE,
                 NULL, NULL) != FALSE) &&
            (ness_pkthunder_material.effects ==
                 NDS_NATIVE_NESS_PKTHUNDER_MATERIAL_EFFECTS))
        {
            ness_pkthunder_native_candidate = TRUE;
        }
    }
    /* The attached Up-B wave is the third NessModel root in the same packet.
     * It is an Effect GObj rather than a Weapon, but has the identical closed
     * CURRENT_IMAGE material contract. */
    if ((ness_pkthunder_native_candidate == FALSE) &&
        (loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_NESS_PKTHUNDER_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) ==
             NDS_NATIVE_NESS_PKTHUNDER_WAVE_ROOT) &&
        (dobj->parent_gobj != NULL) &&
        (dobj->parent_gobj->id == nGCCommonKindEffect) &&
        (dobj->mobj != NULL) && (dobj->mobj->next == NULL) &&
        (ndsRendererAdapterBuildNativeMaterialSnapshot(
             dobj->mobj, &ness_pkthunder_material, FALSE,
             NULL, NULL) != FALSE) &&
        (ness_pkthunder_material.effects ==
             NDS_NATIVE_NESS_PKTHUNDER_MATERIAL_EFFECTS))
    {
        ness_pkthunder_root_index = NDS_NATIVE_NESS_PKTHUNDER_WAVE_INDEX;
        ness_pkthunder_native_candidate = TRUE;
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_PURIN
    if ((loaded != NULL) && (loaded->asset_id == NDS_NATIVE_PURIN_SING_ASSET) &&
        (dobj->parent_gobj != NULL) &&
        (dobj->parent_gobj->id == nGCCommonKindEffect) &&
        (dobj->mobj != NULL) && (dobj->mobj->next == NULL))
    {
        static const u32 roots[4] = { NDS_NATIVE_PURIN_SING_ROOT_0,
            NDS_NATIVE_PURIN_SING_ROOT_1, NDS_NATIVE_PURIN_SING_ROOT_2,
            NDS_NATIVE_PURIN_SING_ROOT_3 };
        static const u32 images[4] = { NDS_NATIVE_PURIN_SING_IMAGE_0,
            NDS_NATIVE_PURIN_SING_IMAGE_1, NDS_NATIVE_PURIN_SING_IMAGE_2,
            NDS_NATIVE_PURIN_SING_IMAGE_3 };
        u32 root = ndsRelocNativeRootOffset(loaded, dl);
        u32 i;
        for (i = 0u; i < 4u; i++)
        {
            if ((root == roots[i]) &&
                (ndsRendererAdapterBuildNativeMaterialSnapshot(dobj->mobj,
                    &purin_sing_material, FALSE, NULL, NULL) != FALSE) &&
                (purin_sing_material.effects == NDS_NATIVE_PURIN_SING_MATERIAL_EFFECTS))
            {
                purin_sing_root_index = i;
#if NDS_P2_1P_GAME || NDS_P2_MENU_SHELL || NDS_P2_SHELL_ARGMAX_ROSTER || NDS_P2_COMPACT_BATTLE_FIGHTERS
                purin_sing_image = ndsRelocNativeAssetAddress(loaded->data, images[i]);
#else
                purin_sing_image = (const u8 *)loaded->data + images[i];
#endif
                purin_sing_native_candidate = (purin_sing_image != NULL);
                break;
            }
        }
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_YOSHI
    /* Yoshi's match intro is YoshiSpecial2 root 0x0530, not the YoshiModel
     * shield/escape egg. It has one live segment-E MObj whose MatAnimJoint
     * switches between the two source shell images. Keep that MObj live and
     * bake only the immutable DL/geometry. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_YOSHI_ENTRYEGG_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) ==
             NDS_NATIVE_YOSHI_ENTRYEGG_ROOT) &&
        (dobj->parent_gobj != NULL) &&
        (dobj->parent_gobj->id == nGCCommonKindEffect) &&
        (dobj->mobj != NULL) && (dobj->mobj->next == NULL))
    {
#if NDS_P2_1P_GAME || NDS_P2_MENU_SHELL || NDS_P2_SHELL_ARGMAX_ROSTER || \
    NDS_P2_COMPACT_BATTLE_FIGHTERS
        const void *expected_palette = ndsRelocNativeAssetAddress(
            loaded->data, NDS_NATIVE_YOSHI_ENTRYEGG_PALETTE_OFFSET);
        const void *expected_vertex = ndsRelocNativeAssetAddress(
            loaded->data, NDS_NATIVE_YOSHI_ENTRYEGG_VERTEX_OFFSET);
#else
        const void *expected_palette = (const u8 *)loaded->data +
            NDS_NATIVE_YOSHI_ENTRYEGG_PALETTE_OFFSET;
        const void *expected_vertex = (const u8 *)loaded->data +
            NDS_NATIVE_YOSHI_ENTRYEGG_VERTEX_OFFSET;
#endif
        if ((expected_palette != NULL) && (expected_vertex != NULL) &&
            (dl[9].words.w0 == NDS_NATIVE_YOSHI_ENTRYEGG_PALETTE_W0) &&
            (dl[9].words.w1 == (u32)(uintptr_t)expected_palette) &&
            (dl[13].words.w0 == 0xde000000u) &&
            (dl[13].words.w1 == 0x0e000000u) &&
            (dl[17].words.w0 == NDS_NATIVE_YOSHI_ENTRYEGG_VERTEX_W0) &&
            (dl[17].words.w1 == (u32)(uintptr_t)expected_vertex) &&
            (ndsRendererAdapterBuildNativeMaterialSnapshot(
                 dobj->mobj, &yoshi_entryegg_material, FALSE,
                 NULL, NULL) != FALSE) &&
            (yoshi_entryegg_material.effects ==
                 NDS_NATIVE_YOSHI_ENTRYEGG_MATERIAL_EFFECTS))
        {
            yoshi_entryegg_palette = expected_palette;
            yoshi_entryegg_native_candidate = TRUE;
        }
    }
    /* YoshiModel 0xA860 is one shared, self-contained egg quad. YoshiMain's
     * EggThrow WPAttributes point here and both Yoshi egg EFDesc users point
     * here directly.
     *
     * The production compact battle pack intentionally PRUNES this root's
     * palette/image/vertex spans and replaces its Gfx program with one
     * ENDDL+source-offset identity cell.  ndsRelocNativeRootOffset() is the
     * compact-pack-aware proof of source identity.  The SHA-pinned generated
     * owner bakes the immutable palette, texels, state and geometry, so reading
     * dl[10]/dl[16]/dl[21] here would inspect beyond the compact root cell and
     * can never be a valid admission test.  Keep only live semantic identity:
     * exact source asset/root plus the source GObj kind / EggThrow WP kind. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_YOSHI_EGG_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_YOSHI_EGG_ROOT) &&
        (dobj->parent_gobj != NULL) && (dobj->mobj == NULL))
    {
        if (dobj->parent_gobj->id == nGCCommonKindWeapon)
        {
            WPStruct *egg_wp = wpGetStruct(dobj->parent_gobj);

            if ((egg_wp != NULL) && (egg_wp->kind == nWPKindEggThrow))
            {
                yoshi_egg_is_weapon = TRUE;
                yoshi_egg_native_candidate = TRUE;
            }
        }
        else if (dobj->parent_gobj->id == nGCCommonKindEffect)
        {
            yoshi_egg_native_candidate = TRUE;
        }
    }
    /* Neutral-B's victim egg is a separate YoshiSpecial3 effect. Its DObjDesc
     * child points at the fixed root 0x0870. Validate the live relocated
     * palette/image/vertex pointers so this also remains correct if the file is
     * compact-packed. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_YOSHI_EGGLAY_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_YOSHI_EGGLAY_ROOT) &&
        (dobj->parent_gobj != NULL) &&
        (dobj->parent_gobj->id == nGCCommonKindEffect) && (dobj->mobj == NULL))
    {
#if NDS_P2_1P_GAME || NDS_P2_MENU_SHELL || NDS_P2_SHELL_ARGMAX_ROSTER || \
    NDS_P2_COMPACT_BATTLE_FIGHTERS
        const void *expected_palette = ndsRelocNativeAssetAddress(
            loaded->data, NDS_NATIVE_YOSHI_EGGLAY_PALETTE_OFFSET);
        const void *expected_image = ndsRelocNativeAssetAddress(
            loaded->data, NDS_NATIVE_YOSHI_EGGLAY_IMAGE_OFFSET);
        const void *expected_vertex = ndsRelocNativeAssetAddress(
            loaded->data, NDS_NATIVE_YOSHI_EGGLAY_VERTEX_OFFSET);
#else
        const void *expected_palette = (const u8 *)loaded->data +
            NDS_NATIVE_YOSHI_EGGLAY_PALETTE_OFFSET;
        const void *expected_image = (const u8 *)loaded->data +
            NDS_NATIVE_YOSHI_EGGLAY_IMAGE_OFFSET;
        const void *expected_vertex = (const u8 *)loaded->data +
            NDS_NATIVE_YOSHI_EGGLAY_VERTEX_OFFSET;
#endif

        if ((expected_palette != NULL) && (expected_image != NULL) &&
            (expected_vertex != NULL) &&
            (dl[11].words.w0 == NDS_NATIVE_YOSHI_EGGLAY_PALETTE_W0) &&
            (dl[17].words.w0 == NDS_NATIVE_YOSHI_EGGLAY_IMAGE_W0) &&
            (dl[21].words.w0 == NDS_NATIVE_YOSHI_EGGLAY_VERTEX_W0) &&
            (dl[11].words.w1 == (u32)(uintptr_t)expected_palette) &&
            (dl[17].words.w1 == (u32)(uintptr_t)expected_image) &&
            (dl[21].words.w1 == (u32)(uintptr_t)expected_vertex))
        {
            yoshi_egglay_palette = expected_palette;
            yoshi_egglay_image = expected_image;
            yoshi_egglay_native_candidate = TRUE;
        }
    }
#endif
    /* Samus Charge Shot, file 321 root 0x270.  Every pointer the program
     * carries is internal to file 321 and it has no MObj, so the admission is
     * asset, root, a Weapon GObj and a NULL MObj -- and that tuple is enough:
     * Kirby's copied Charge Shot reaches the same attributes and therefore the
     * same root, and it should be served by the same owner. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_CHARGESHOT_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_CHARGESHOT_ROOT))
    {
        u32 shot_step = 1u;

        if ((dobj->parent_gobj != NULL) &&
            (dobj->parent_gobj->id == nGCCommonKindWeapon))
        {
            shot_step = 2u;
            if (dobj->mobj == NULL)
            {
                shot_step = 3u;
                if (loaded->data_size >= (NDS_NATIVE_CHARGESHOT_ROOT +
                                          NDS_NATIVE_CHARGESHOT_DL_BYTES))
                {
                    shot_step = 4u;
                    charge_shot_native_candidate = TRUE;
                }
            }
        }
        if (shot_step > gNdsChargeShotCandidateStep)
        {
            gNdsChargeShotCandidateStep = shot_step;
        }
    }
    /* Pikachu's air Thunder Jolt, file 342 root 0x0270.  Same admission shape
     * as the Charge Shot above and for the same reason: every pointer the
     * program carries is internal to file 342, it has no MObj, and the tuple of
     * asset, root, a Weapon GObj and a NULL MObj is enough on its own.  A
     * whole-image sweep of all 2,132 O2R files finds exactly ONE pointer
     * reaching this root -- PikachuSpecial1's external fixup at slot 0, the
     * WPAttributes whose `data` field IS this display list -- and no `ll`
     * constant names 0x0270 either, so no live-kind discriminator is needed.
     *
     * Note the two files: the attributes are in file 244 and the geometry in
     * file 342, which is why file 342 holds no pointer to its own root. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_THUNDERJOLT_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_THUNDERJOLT_ROOT))
    {
        u32 jolt_step = 1u;

        if ((dobj->parent_gobj != NULL) &&
            (dobj->parent_gobj->id == nGCCommonKindWeapon))
        {
            jolt_step = 2u;
            if (dobj->mobj == NULL)
            {
                jolt_step = 3u;
                if ((loaded->data != NULL) &&
                    (loaded->data_size >= NDS_NATIVE_THUNDERJOLT_FILE_END) &&
                    (loaded->data_size >= (NDS_NATIVE_THUNDERJOLT_ROOT +
                                           NDS_NATIVE_THUNDERJOLT_DL_BYTES)))
                {
                    const u8 *jolt_base = (const u8 *)loaded->data;

                    jolt_step = 4u;
                    /* COMPARE the relocated pointers against this file's own
                     * base -- never assume the loader's fixup pass ran, and
                     * never bind a chain word as an image. */
                    if ((dl[11].words.w0 == NDS_NATIVE_THUNDERJOLT_TLUT_W0) &&
                        (dl[17].words.w0 == NDS_NATIVE_THUNDERJOLT_IMAGE_W0) &&
                        (dl[11].words.w1 ==
                             (u32)(uintptr_t)(jolt_base +
                                 NDS_NATIVE_THUNDERJOLT_TLUT_OFFSET)) &&
                        (dl[17].words.w1 ==
                             (u32)(uintptr_t)(jolt_base +
                                 NDS_NATIVE_THUNDERJOLT_IMAGE_OFFSET)) &&
                        (dl[21].words.w1 ==
                             (u32)(uintptr_t)(jolt_base +
                                 NDS_NATIVE_THUNDERJOLT_VERTEX_OFFSET)))
                    {
                        jolt_step = 5u;
                        thunder_jolt_native_candidate = TRUE;
                    }
                }
            }
        }
        if (jolt_step > gNdsThunderJoltCandidateStep)
        {
            gNdsThunderJoltCandidateStep = jolt_step;
        }
    }
    /* GROUND Thunder Jolt, file 342 DObjDesc 0x1888 reached from
     * llPikachuSpecial1ThunderJoltGroundWeaponAttributes (0x34). Six drawable
     * children at these roots, each one triangle with one segment-0xE hook and
     * its own MObjSub. This began as a reconnaissance witness owning nothing, and
     * the two things it measured are why the owner below is shaped as it is: a
     * root mask of 0x3f, so all six segments are walked and all six must be
     * owned, and an effects word of 0x200 on every one of them, which is
     * CURRENT_IMAGE alone. The witness stays and now drives the admission.
     * The record latched the FOURTH child rather than the first only because a
     * DOBJ_FLAG_HIDDEN child is skipped along with its subtree and records
     * nothing, which is also why the count was never a multiple of six. */
    if ((loaded != NULL) && (loaded->asset_id == NDS_NATIVE_THUNDERJOLT_ASSET) &&
        (dobj->mobj != NULL))
    {
        u32 ground_root = ndsRelocNativeRootOffset(loaded, dl);
        u32 ground_bit = 0u;

        switch (ground_root)
        {
        case 0x1490u: ground_bit = 1u << 0; break;
        case 0x1528u: ground_bit = 1u << 1; break;
        case 0x15c0u: ground_bit = 1u << 2; break;
        case 0x1660u: ground_bit = 1u << 3; break;
        case 0x16f8u: ground_bit = 1u << 4; break;
        case 0x1790u: ground_bit = 1u << 5; break;
        default: break;
        }
        if (ground_bit != 0u)
        {
            u32 ground_step = 1u;

            gNdsThunderGroundRootMask |= ground_bit;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindWeapon))
            {
                ground_step = 2u;
                if (ndsRendererAdapterBuildNativeMaterialSnapshot(
                        dobj->mobj, &thunder_ground_material, FALSE, NULL,
                        NULL) != FALSE)
                {
                    ground_step = 3u;
                    gNdsThunderGroundEffectsSeen |=
                        thunder_ground_material.effects;
                    /* The closed contract, measured at 0x200 across all six
                     * roots: CURRENT_IMAGE and nothing else.  Anything broader
                     * means this specialization would silently drop source
                     * material, so decline and let the loud NO_PROGRAM record
                     * below publish it. */
                    if (thunder_ground_material.effects ==
                        NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE)
                    {
                        ground_step = 4u;
                        /* ground_bit is a one-hot of the six roots in
                         * DObjDesc order, so its trailing zero count IS the
                         * generator's root index. */
                        thunder_ground_root_index = 0u;
                        while (((ground_bit >> thunder_ground_root_index) & 1u)
                                   == 0u)
                        {
                            thunder_ground_root_index++;
                        }
                        thunder_ground_native_candidate = TRUE;
                    }
                }
                else
                {
                    NDS_DIAG(gNdsThunderGroundSnapshotFailCount++);
                }
            }
            if (ground_step > gNdsThunderGroundCandidateStep)
            {
                gNdsThunderGroundCandidateStep = ground_step;
            }
        }
    }
    /* The Thunder Jolt EFFECT, file 342 root 0x2170.  Same asset as both jolts
     * and a third root inside it, reached from
     * llPikachuSpecial3ThunderJoltDObjDesc (0x2258) by the one drawable child
     * of dEFManagerThunderJoltEffectDesc -- so the discriminator that separates
     * it from its two siblings is the EFFECT parent plus the root, and a sweep
     * of the whole image finds no other pointer to 0x2170 at all.
     *
     * Its material must be CURRENT_IMAGE and nothing else.  The list bakes its
     * own palette and takes only the image live, so a broader material here
     * would mean the specialization is dropping source presentation; decline
     * and let the loud NO_PROGRAM record below publish it rather than draw a
     * quietly wrong quad. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_THUNDERJOLTFX_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_THUNDERJOLTFX_ROOT))
    {
        u32 fx_step = 1u;

        if ((dobj->parent_gobj != NULL) &&
            (dobj->parent_gobj->id == nGCCommonKindEffect) &&
            (dobj->mobj != NULL))
        {
            fx_step = 2u;
            if ((loaded->data != NULL) &&
                (loaded->data_size >= NDS_NATIVE_THUNDERJOLTFX_TLUT_END) &&
                (loaded->data_size >= (NDS_NATIVE_THUNDERJOLTFX_ROOT +
                                       NDS_NATIVE_THUNDERJOLTFX_DL_BYTES)))
            {
                const u8 *fx_base = (const u8 *)loaded->data;

                fx_step = 3u;
                /* COMPARE the relocated palette pointer against this file's own
                 * base -- never assume the loader's fixup pass ran -- and
                 * require word 17 to still be the segment-E hook rather than a
                 * baked image, because that one word is the whole difference
                 * between this owner and the air jolt's. */
                if ((dl[11].words.w0 == NDS_NATIVE_THUNDERJOLTFX_TLUT_W0) &&
                    (dl[11].words.w1 ==
                         (u32)(uintptr_t)(fx_base +
                             NDS_NATIVE_THUNDERJOLTFX_TLUT_OFFSET)) &&
                    ((dl[17].words.w0 >> 24) == 0xdeu) &&
                    (dl[21].words.w1 ==
                         (u32)(uintptr_t)(fx_base +
                             NDS_NATIVE_THUNDERJOLTFX_VERTEX_OFFSET)))
                {
                    fx_step = 4u;
                    if (ndsRendererAdapterBuildNativeMaterialSnapshot(
                            dobj->mobj, &thunder_fx_material, FALSE, NULL,
                            NULL) != FALSE)
                    {
                        fx_step = 5u;
                        gNdsThunderJoltFxEffectsSeen |=
                            thunder_fx_material.effects;
                        if (thunder_fx_material.effects ==
                            NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE)
                        {
                            fx_step = 6u;
                            thunder_fx_base = loaded->data;
                            thunder_fx_bytes = loaded->data_size;
                            thunder_fx_native_candidate = TRUE;
                        }
                    }
                    else
                    {
                        NDS_DIAG(gNdsThunderJoltFxSnapshotFailCount++);
                    }
                }
            }
        }
        if (fx_step > gNdsThunderJoltFxCandidateStep)
        {
            gNdsThunderJoltFxCandidateStep = fx_step;
        }
    }

    /* BattleShip DamageSlash, EFCommonEffects1 file 83.  Its two source child
     * roots are 0x75A0 and 0x7668.  Both are immutable quad/list shells with a
     * single segment-E material hook; the live MObj supplies CURRENT_IMAGE,
     * PRIM and both light colours.  Asset+root is source-complete: each root has
     * exactly one referrer in file 83, its own DObjDLLink slot. */
    damage_fly_mdust_native_seen =
        ((loaded != NULL) &&
         (loaded->asset_id == NDS_NATIVE_DAMAGE_FLY_MDUST_ASSET) &&
         (ndsRelocNativeRootOffset(loaded, dl) ==
          NDS_NATIVE_DAMAGE_FLY_MDUST_ROOT)) ? TRUE : FALSE;
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_DAMAGE_SLASH_ASSET))
    {
        u32 slash_root = ndsRelocNativeRootOffset(loaded, dl);
        u32 slash_step = 0u;
        u32 palette_offset = 0u;
        u32 vertex_offset = 0u;
        u32 root_bit = 0u;

        if (slash_root == NDS_NATIVE_DAMAGE_SLASH_ROOT0)
        {
            palette_offset = NDS_NATIVE_DAMAGE_SLASH_PALETTE0_OFFSET;
            vertex_offset = NDS_NATIVE_DAMAGE_SLASH_VERTEX0_OFFSET;
            root_bit = 1u;
        }
        else if (slash_root == NDS_NATIVE_DAMAGE_SLASH_ROOT1)
        {
            palette_offset = NDS_NATIVE_DAMAGE_SLASH_PALETTE1_OFFSET;
            vertex_offset = NDS_NATIVE_DAMAGE_SLASH_VERTEX1_OFFSET;
            root_bit = 2u;
        }
        if (root_bit != 0u)
        {
            const u8 *slash_base = (const u8 *)loaded->data;

            damage_slash_native_seen = TRUE;
            damage_slash_root = slash_root;
            gNdsDamageSlashRootMask |= root_bit;
            slash_step = 1u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindEffect) &&
                (dobj->mobj != NULL) && (dobj->mobj->next == NULL))
            {
                slash_step = 2u;
                if ((slash_base != NULL) &&
                    (loaded->data_size >= NDS_NATIVE_DAMAGE_SLASH_PALETTE_END) &&
                    (loaded->data_size >=
                         (slash_root + NDS_NATIVE_DAMAGE_SLASH_DL_BYTES)) &&
                    (dl[7].words.w1 ==
                         (u32)(uintptr_t)(slash_base + palette_offset)) &&
                    ((dl[13].words.w0 >> 24) == 0xdeu) &&
                    (dl[13].words.w1 == 0x0e000000u) &&
                    (dl[18].words.w1 ==
                         (u32)(uintptr_t)(slash_base + vertex_offset)))
                {
                    slash_step = 3u;
                    if (ndsRendererAdapterBuildNativeMaterialSnapshot(
                            dobj->mobj, &damage_slash_material, FALSE,
                            NULL, NULL) != FALSE)
                    {
                        const u32 want_effects =
                            NDS_RENDERER_NATIVE_MATERIAL_LIGHT1 |
                            NDS_RENDERER_NATIVE_MATERIAL_LIGHT2 |
                            NDS_RENDERER_NATIVE_MATERIAL_PRIM |
                            NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE;

                        slash_step = 4u;
                        gNdsDamageSlashEffectsSeen |=
                            damage_slash_material.effects;
                        if (damage_slash_material.effects == want_effects)
                        {
                            slash_step = 5u;
                            damage_slash_base = loaded->data;
                            damage_slash_bytes = loaded->data_size;
                            damage_slash_native_candidate = TRUE;
                        }
                        else
                        {
                            gNdsDamageSlashEffectsRejected |=
                                damage_slash_material.effects;
                        }
                    }
                    else
                    {
                        NDS_DIAG(gNdsDamageSlashSnapshotFailCount++);
                    }
                }
            }
            if (slash_step > gNdsDamageSlashCandidateStep)
            {
                gNdsDamageSlashCandidateStep = slash_step;
            }
        }
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_LINK
    /* Link's Bomb, file 353 roots 0x16f8 (DL head 0, body) and 0x17e8 (DL
     * head 1, fuse glow).  ONE OBJECT, TWO LISTS: LinkMain's ITAttributes at
     * 0x40 -- llLinkMainBombItemAttributes, named beside nITKindLinkBomb at
     * itlinkbomb.c:20-25 -- points at DObjDesc 353:0x18d8, whose entries 1
     * and 2 carry those two lists, and itDisplayColAnimXLU walks the tree once
     * (itdisplay.c:305) so both arrive per drawn frame, body first.  That
     * ordering is why the failure record named 0x16f8.
     *
     * ONE REFERRER EACH, GAME-WIDE.  A sweep of all 2,132 O2R files finds
     * exactly one pointer to each root -- file 353's own internal fixups
     * 0x18bc and 0x18cc -- and the only external pointers into file 353 at all
     * are LinkMain 0x0004/0x0040/0x0048.  No relocation constant names either
     * root.  So asset+root already discriminate and, unlike the Castle bumper,
     * no live-kind test is REQUIRED.  ITStruct.kind is still read, so that a
     * future second referrer records NO_PROGRAM loudly instead of being drawn
     * by a program baked for the bomb.
     *
     * material 0 in the recorded failure is ITAttributes.p_mobjsubs == NULL
     * (LinkMain 0x44), NOT "untextured": both lists carry their own binding.
     * Step-witnessed, because a count alone cannot say WHICH clause declined;
     * step 8 means every clause passed. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_LINK_BOMB_ASSET))
    {
        u32 bomb_root = ndsRelocNativeRootOffset(loaded, dl);

        if ((bomb_root == NDS_NATIVE_LINK_BOMB_BODY_ROOT) ||
            (bomb_root == NDS_NATIVE_LINK_BOMB_FUSE_ROOT))
        {
            u32 bomb_step = 1u;
            sb32 bomb_is_body =
                (bomb_root == NDS_NATIVE_LINK_BOMB_BODY_ROOT) ? TRUE : FALSE;

            if (sNdsRendererAdapterItemSubmitActive != FALSE)
            {
                bomb_step = 2u;
                if ((dobj->parent_gobj != NULL) &&
                    (dobj->parent_gobj->id == nGCCommonKindItem))
                {
                    ITStruct *bomb_ip = itGetStruct(dobj->parent_gobj);

                    bomb_step = 3u;
                    if (bomb_ip != NULL)
                    {
                        gNdsLinkBombItemKind = (u32)bomb_ip->kind;
                        if (bomb_ip->kind != nITKindLinkBomb)
                        {
                            /* Census says this cannot happen today.  If it
                             * ever does, say WHY rather than leave it
                             * indistinguishable from a submit refusal. */
                            NDS_DIAG(gNdsLinkBombForeignKindCount++);
                        }
                        /* The root ALONE discriminates the two lists, so this
                         * arm does not gate on the DL head.  The source's own
                         * head assignment is DObjDLLink 0x18b8 = list_id 0 and
                         * 0x18c8 = list_id 1, but the port does not reproduce
                         * it: sNdsRendererAdapterItemSubmitHead is written to
                         * 0u once in ndsRendererAdapterSubmitItemDObjTree and
                         * never advanced by the tree walk, so it reads 0 for
                         * BOTH lists.  Gating on it would have declined every
                         * fuse draw at step 3 and left the item half-drawn --
                         * exactly the silent-empty-draw outcome the native
                         * contract forbids.  Witness the observed head instead
                         * so the divergence stays visible; the env-colour
                         * selection above reads the same field, so if it ever
                         * starts tracking the source the witness says so. */
                        else
                        {
                            gNdsLinkBombHead =
                                sNdsRendererAdapterItemSubmitHead;
                            bomb_step = 4u;
                            /* material 0: p_mobjsubs is NULL, so an MObj
                             * here would be a different draw entirely. */
                            if (dobj->mobj == NULL)
                            {
                                bomb_step = 5u;
                                if ((loaded->data != NULL) &&
                                    (loaded->data_size >=
                                         NDS_NATIVE_LINK_BOMB_FILE_END) &&
                                    (loaded->data_size >=
                                         (bomb_root +
                                          ((bomb_is_body != FALSE) ?
                                               NDS_NATIVE_LINK_BOMB_BODY_DL_BYTES :
                                               NDS_NATIVE_LINK_BOMB_FUSE_DL_BYTES))))
                                {
                                    const u8 *bomb_base =
                                        (const u8 *)loaded->data;
                                    u32 bomb_tlut_w0;
                                    u32 bomb_image_w0;
                                    u32 bomb_tlut_w1;
                                    u32 bomb_image_w1;

                                    bomb_step = 6u;
                                    if (bomb_is_body != FALSE)
                                    {
                                        bomb_tlut_w0 = dl[11].words.w0;
                                        bomb_tlut_w1 = dl[11].words.w1;
                                        bomb_image_w0 = dl[17].words.w0;
                                        bomb_image_w1 = dl[17].words.w1;
                                    }
                                    else
                                    {
                                        bomb_tlut_w0 =
                                            NDS_NATIVE_LINK_BOMB_BODY_TLUT_W0;
                                        bomb_tlut_w1 = (u32)(uintptr_t)(
                                            bomb_base +
                                            NDS_NATIVE_LINK_BOMB_TLUT_OFFSET);
                                        bomb_image_w0 = dl[13].words.w0;
                                        bomb_image_w1 = dl[13].words.w1;
                                    }
                                    if ((bomb_tlut_w0 ==
                                             NDS_NATIVE_LINK_BOMB_BODY_TLUT_W0) &&
                                        (bomb_image_w0 ==
                                             ((bomb_is_body != FALSE) ?
                                                  NDS_NATIVE_LINK_BOMB_BODY_IMAGE_W0 :
                                                  NDS_NATIVE_LINK_BOMB_FUSE_IMAGE_W0)))
                                    {
                                        bomb_step = 7u;
                                        /* File 353's fixups are INTERNAL, but
                                         * an unrelocated word is still a chain
                                         * word.  COMPARE the relocated
                                         * pointers against this file's own
                                         * base -- never assume the loader's
                                         * fixup pass ran, and never bind a
                                         * chain word as an image. */
                                        if ((bomb_tlut_w1 ==
                                                 (u32)(uintptr_t)(bomb_base +
                                                     NDS_NATIVE_LINK_BOMB_TLUT_OFFSET)) &&
                                            (bomb_image_w1 ==
                                                 (u32)(uintptr_t)(bomb_base +
                                                     ((bomb_is_body != FALSE) ?
                                                          NDS_NATIVE_LINK_BOMB_BODY_IMAGE_OFFSET :
                                                          NDS_NATIVE_LINK_BOMB_FUSE_IMAGE_OFFSET))))
                                        {
                                            bomb_step = 8u;
                                            link_bomb_root = bomb_root;
                                            link_bomb_native_candidate = TRUE;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (bomb_step > gNdsLinkBombCandidateStep)
            {
                gNdsLinkBombCandidateStep = bomb_step;
            }
        }
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_YAMABUKI
    /* Saffron City's Marumine (Electrode), file 159 root 0x06a0.  ONE OBJECT,
     * ONE LIST: GRYamabukiMap's ITAttributes at 0x104 --
     * llGRYamabukiMapMarumineItemAttributes, named beside nITKindMarumine at
     * itmarumine.c:11-15 -- points at DObjDesc 159:0x0790, whose entry 0 has no
     * display list and whose entry 1 carries this one.  itmanager.c:392 then
     * calls lbCommonEjectTreeDObj, which DELETES that DL-less root and promotes
     * the child, so the GObj ends with a single DObj and
     * itDisplayOPAProcDisplay walks it once (itdisplay.c:189).  Unlike the Link
     * bomb this object offers exactly one list per drawn frame.
     *
     * ONE REFERRER, GAME-WIDE.  A sweep of all 2,132 O2R files finds exactly
     * one pointer to this root -- file 159's own internal fixup 0x07C0, which
     * IS DObjDesc 0x0790 entry 1 -- and the only external pointers into file
     * 159 at all are GRYamabukiMap's thirteen, none of which names 0x06a0.  No
     * relocation constant names the root either.  So asset+root already
     * discriminate and, like the Link bomb and unlike the Castle bumper, no
     * live-kind test is REQUIRED.  ITStruct.kind is still read, so that a
     * future second referrer records NO_PROGRAM loudly instead of being drawn
     * by a program baked for Electrode.
     *
     * material 0 in the recorded failure is ITAttributes.p_mobjsubs == NULL
     * (264_GRYamabukiMap.c:141), NOT "untextured": the list carries its own
     * TLUT and image out of file 159's own internal fixups.  Step-witnessed,
     * because a count alone cannot say WHICH clause declined; step 9 means
     * every clause passed and any decline is inside the submit itself.
     *
     * The DL head is NOT tested.  sNdsRendererAdapterItemSubmitHead is written
     * to 0u once in ndsRendererAdapterSubmitItemDObjTree and never advanced by
     * the tree walk, so it reads 0 for every list of every item; gating on it
     * would be a guard built on a field the port does not maintain.  Asset and
     * root already discriminate this owner completely. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_MARUMINE_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_MARUMINE_ROOT))
    {
        u32 marumine_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            marumine_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *marumine_ip = itGetStruct(dobj->parent_gobj);

                marumine_step = 3u;
                if (marumine_ip != NULL)
                {
                    gNdsYamabukiMarumineItemKind = (u32)marumine_ip->kind;
                    if (marumine_ip->kind != nITKindMarumine)
                    {
                        /* The census says this cannot happen today.  If it ever
                         * does, say WHY rather than leave it indistinguishable
                         * from a submit refusal. */
                        NDS_DIAG(gNdsYamabukiMarumineForeignKindCount++);
                    }
                    else
                    {
                        marumine_step = 4u;
                        /* material 0: p_mobjsubs is NULL, so an MObj here would
                         * be a different draw entirely. */
                        if (dobj->mobj == NULL)
                        {
                            marumine_step = 5u;
                            if ((loaded->data != NULL) &&
                                (loaded->data_size >=
                                     NDS_NATIVE_MARUMINE_FILE_END) &&
                                (loaded->data_size >=
                                     (NDS_NATIVE_MARUMINE_ROOT +
                                      NDS_NATIVE_MARUMINE_DL_BYTES)))
                            {
                                const u8 *marumine_base =
                                    (const u8 *)loaded->data;

                                marumine_step = 6u;
                                if ((dl[11].words.w0 ==
                                         NDS_NATIVE_MARUMINE_TLUT_W0) &&
                                    (dl[17].words.w0 ==
                                         NDS_NATIVE_MARUMINE_IMAGE_W0))
                                {
                                    marumine_step = 7u;
                                    /* File 159's fixups are INTERNAL (it has
                                     * zero external fixups), but an unrelocated
                                     * word is still a chain word.  COMPARE the
                                     * relocated pointers against this file's
                                     * own base -- never assume the loader's
                                     * fixup pass ran, and never bind a chain
                                     * word as an image. */
                                    if ((dl[11].words.w1 ==
                                             (u32)(uintptr_t)(marumine_base +
                                                 NDS_NATIVE_MARUMINE_TLUT_OFFSET)) &&
                                        (dl[17].words.w1 ==
                                             (u32)(uintptr_t)(marumine_base +
                                                 NDS_NATIVE_MARUMINE_IMAGE_OFFSET)))
                                    {
                                        marumine_step = 8u;
                                        if (dl[21].words.w1 ==
                                                (u32)(uintptr_t)(marumine_base +
                                                    NDS_NATIVE_MARUMINE_VERTEX_OFFSET))
                                        {
                                            marumine_step = 9u;
                                            marumine_native_candidate = TRUE;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        if (marumine_step > gNdsYamabukiMarumineCandidateStep)
        {
            gNdsYamabukiMarumineCandidateStep = marumine_step;
        }
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_YAMABUKI && NDS_P2_ITEM_CORE
    /* GLucky and Porygon are the two bake-everything Saffron siblings.  Their
     * thirty-word roots contain no 0xDE call, their MObj is NULL, and both the
     * TLUT and image are immutable file-159 fixups.  Keep the same step-9
     * admission contract as Marumine so an inert owner is distinguishable
     * from a submit refusal. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_GLUCKY_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_GLUCKY_ROOT))
    {
        u32 glucky_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            glucky_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *glucky_ip = itGetStruct(dobj->parent_gobj);

                glucky_step = 3u;
                if (glucky_ip != NULL)
                {
                    gNdsYamabukiGluckyItemKind = (u32)glucky_ip->kind;
                    if (glucky_ip->kind != nITKindGLucky)
                    {
                        NDS_DIAG(gNdsYamabukiGluckyForeignKindCount++);
                    }
                    else
                    {
                        glucky_step = 4u;
                        if (dobj->mobj == NULL)
                        {
                            glucky_step = 5u;
                            if ((loaded->data != NULL) &&
                                (loaded->data_size >= NDS_NATIVE_ITEM_GLUCKY_FILE_END) &&
                                (loaded->data_size >= (NDS_NATIVE_ITEM_GLUCKY_ROOT +
                                                       NDS_NATIVE_ITEM_GLUCKY_DL_BYTES)))
                            {
                                const u8 *glucky_base = (const u8 *)loaded->data;

                                glucky_step = 6u;
                                if ((dl[11].words.w0 == NDS_NATIVE_ITEM_GLUCKY_TLUT_W0) &&
                                    (dl[17].words.w0 == NDS_NATIVE_ITEM_GLUCKY_IMAGE_W0))
                                {
                                    glucky_step = 7u;
                                    if ((dl[11].words.w1 ==
                                             (u32)(uintptr_t)(glucky_base +
                                                 NDS_NATIVE_ITEM_GLUCKY_TLUT_OFFSET)) &&
                                        (dl[17].words.w1 ==
                                             (u32)(uintptr_t)(glucky_base +
                                                 NDS_NATIVE_ITEM_GLUCKY_IMAGE_OFFSET)))
                                    {
                                        glucky_step = 8u;
                                        if (dl[21].words.w1 ==
                                                (u32)(uintptr_t)(glucky_base +
                                                    NDS_NATIVE_ITEM_GLUCKY_VERTEX_OFFSET))
                                        {
                                            glucky_step = 9u;
                                            glucky_native_candidate = TRUE;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        if (glucky_step > gNdsYamabukiGluckyCandidateStep)
        {
            gNdsYamabukiGluckyCandidateStep = glucky_step;
        }
    }

    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_PORYGON_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_PORYGON_ROOT))
    {
        u32 porygon_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            porygon_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *porygon_ip = itGetStruct(dobj->parent_gobj);

                porygon_step = 3u;
                if (porygon_ip != NULL)
                {
                    gNdsYamabukiPorygonItemKind = (u32)porygon_ip->kind;
                    if (porygon_ip->kind != nITKindPorygon)
                    {
                        NDS_DIAG(gNdsYamabukiPorygonForeignKindCount++);
                    }
                    else
                    {
                        porygon_step = 4u;
                        if (dobj->mobj == NULL)
                        {
                            porygon_step = 5u;
                            if ((loaded->data != NULL) &&
                                (loaded->data_size >= NDS_NATIVE_ITEM_PORYGON_FILE_END) &&
                                (loaded->data_size >= (NDS_NATIVE_ITEM_PORYGON_ROOT +
                                                       NDS_NATIVE_ITEM_PORYGON_DL_BYTES)))
                            {
                                const u8 *porygon_base = (const u8 *)loaded->data;

                                porygon_step = 6u;
                                if ((dl[11].words.w0 == NDS_NATIVE_ITEM_PORYGON_TLUT_W0) &&
                                    (dl[17].words.w0 == NDS_NATIVE_ITEM_PORYGON_IMAGE_W0))
                                {
                                    porygon_step = 7u;
                                    if ((dl[11].words.w1 ==
                                             (u32)(uintptr_t)(porygon_base +
                                                 NDS_NATIVE_ITEM_PORYGON_TLUT_OFFSET)) &&
                                        (dl[17].words.w1 ==
                                             (u32)(uintptr_t)(porygon_base +
                                                 NDS_NATIVE_ITEM_PORYGON_IMAGE_OFFSET)))
                                    {
                                        porygon_step = 8u;
                                        if (dl[21].words.w1 ==
                                                (u32)(uintptr_t)(porygon_base +
                                                    NDS_NATIVE_ITEM_PORYGON_VERTEX_OFFSET))
                                        {
                                            porygon_step = 9u;
                                            porygon_native_candidate = TRUE;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        if (porygon_step > gNdsYamabukiPorygonCandidateStep)
        {
            gNdsYamabukiPorygonCandidateStep = porygon_step;
        }
    }

    /* Hitokage and Fushigibana have the complementary shape: one exact
     * segment-E call at word 17 and a live MObj whose only measured effect is
     * CURRENT_IMAGE.  The immutable palette and geometry stay pinned here;
     * the image is accepted only through the typed material snapshot. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_HITOKAGE_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_HITOKAGE_ROOT))
    {
        u32 hitokage_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            hitokage_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *hitokage_ip = itGetStruct(dobj->parent_gobj);

                hitokage_step = 3u;
                if (hitokage_ip != NULL)
                {
                    gNdsYamabukiHitokageItemKind = (u32)hitokage_ip->kind;
                    if (hitokage_ip->kind != nITKindHitokage)
                    {
                        NDS_DIAG(gNdsYamabukiHitokageForeignKindCount++);
                    }
                    else
                    {
                        hitokage_step = 4u;
                        if (dobj->mobj != NULL)
                        {
                            hitokage_step = 5u;
                            if ((loaded->data != NULL) &&
                                (loaded->data_size >= NDS_NATIVE_ITEM_HITOKAGE_TLUT_END) &&
                                (loaded->data_size >= (NDS_NATIVE_ITEM_HITOKAGE_ROOT +
                                                       NDS_NATIVE_ITEM_HITOKAGE_DL_BYTES)))
                            {
                                const u8 *hitokage_base = (const u8 *)loaded->data;

                                hitokage_step = 6u;
                                if ((dl[11].words.w0 == NDS_NATIVE_ITEM_HITOKAGE_TLUT_W0) &&
                                    (dl[11].words.w1 ==
                                         (u32)(uintptr_t)(hitokage_base +
                                             NDS_NATIVE_ITEM_HITOKAGE_TLUT_OFFSET)) &&
                                    (dl[17].words.w0 == NDS_NATIVE_ITEM_HITOKAGE_HOOK_W0) &&
                                    (dl[17].words.w1 == NDS_NATIVE_ITEM_HITOKAGE_HOOK_W1) &&
                                    (dl[21].words.w1 ==
                                         (u32)(uintptr_t)(hitokage_base +
                                             NDS_NATIVE_ITEM_HITOKAGE_VERTEX_OFFSET)))
                                {
                                    hitokage_step = 7u;
                                    if (ndsRendererAdapterBuildNativeMaterialSnapshot(
                                            dobj->mobj, &hitokage_material, FALSE,
                                            NULL, NULL) != FALSE)
                                    {
                                        hitokage_step = 8u;
                                        gNdsYamabukiHitokageEffectsSeen |=
                                            hitokage_material.effects;
                                        gNdsYamabukiHitokageImage =
                                            hitokage_material.current_image;
                                        if (hitokage_material.effects ==
                                            NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE)
                                        {
                                            hitokage_step = 9u;
                                            hitokage_native_candidate = TRUE;
                                        }
                                        else
                                        {
                                            gNdsYamabukiHitokageEffectsRejected =
                                                hitokage_material.effects;
                                        }
                                    }
                                    else
                                    {
                                        NDS_DIAG(gNdsYamabukiHitokageSnapshotFailCount++);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        if (hitokage_step > gNdsYamabukiHitokageCandidateStep)
        {
            gNdsYamabukiHitokageCandidateStep = hitokage_step;
        }
    }

    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_FUSHIGIBANA_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_FUSHIGIBANA_ROOT))
    {
        u32 fushigibana_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            fushigibana_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *fushigibana_ip = itGetStruct(dobj->parent_gobj);

                fushigibana_step = 3u;
                if (fushigibana_ip != NULL)
                {
                    gNdsYamabukiFushigibanaItemKind = (u32)fushigibana_ip->kind;
                    if (fushigibana_ip->kind != nITKindFushigibana)
                    {
                        NDS_DIAG(gNdsYamabukiFushigibanaForeignKindCount++);
                    }
                    else
                    {
                        fushigibana_step = 4u;
                        if (dobj->mobj != NULL)
                        {
                            fushigibana_step = 5u;
                            if ((loaded->data != NULL) &&
                                (loaded->data_size >= NDS_NATIVE_ITEM_FUSHIGIBANA_TLUT_END) &&
                                (loaded->data_size >= (NDS_NATIVE_ITEM_FUSHIGIBANA_ROOT +
                                                       NDS_NATIVE_ITEM_FUSHIGIBANA_DL_BYTES)))
                            {
                                const u8 *fushigibana_base = (const u8 *)loaded->data;

                                fushigibana_step = 6u;
                                if ((dl[11].words.w0 == NDS_NATIVE_ITEM_FUSHIGIBANA_TLUT_W0) &&
                                    (dl[11].words.w1 ==
                                         (u32)(uintptr_t)(fushigibana_base +
                                             NDS_NATIVE_ITEM_FUSHIGIBANA_TLUT_OFFSET)) &&
                                    (dl[17].words.w0 == NDS_NATIVE_ITEM_FUSHIGIBANA_HOOK_W0) &&
                                    (dl[17].words.w1 == NDS_NATIVE_ITEM_FUSHIGIBANA_HOOK_W1) &&
                                    (dl[21].words.w1 ==
                                         (u32)(uintptr_t)(fushigibana_base +
                                             NDS_NATIVE_ITEM_FUSHIGIBANA_VERTEX_OFFSET)))
                                {
                                    fushigibana_step = 7u;
                                    if (ndsRendererAdapterBuildNativeMaterialSnapshot(
                                            dobj->mobj, &fushigibana_material, FALSE,
                                            NULL, NULL) != FALSE)
                                    {
                                        fushigibana_step = 8u;
                                        gNdsYamabukiFushigibanaEffectsSeen |=
                                            fushigibana_material.effects;
                                        gNdsYamabukiFushigibanaImage =
                                            fushigibana_material.current_image;
                                        if (fushigibana_material.effects ==
                                            NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE)
                                        {
                                            fushigibana_step = 9u;
                                            fushigibana_native_candidate = TRUE;
                                        }
                                        else
                                        {
                                            gNdsYamabukiFushigibanaEffectsRejected =
                                                fushigibana_material.effects;
                                        }
                                    }
                                    else
                                    {
                                        NDS_DIAG(gNdsYamabukiFushigibanaSnapshotFailCount++);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        if (fushigibana_step > gNdsYamabukiFushigibanaCandidateStep)
        {
            gNdsYamabukiFushigibanaCandidateStep = fushigibana_step;
        }
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE
    /* Five common-item owners selected from the player-visible end of the
     * spawn census. Every admission path proves the exact asset/root/kind and
     * the source material topology before arming a fixed native executor. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_STAR_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_STAR_ROOT))
    {
        u32 star_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            star_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *ip = itGetStruct(dobj->parent_gobj);

                star_step = 3u;
                if (ip != NULL)
                {
                    gNdsItemStarKind = (u32)ip->kind;
                    if (ip->kind != nITKindStar)
                    {
                        NDS_DIAG(gNdsItemStarForeignKindCount++);
                    }
                    else if ((dobj->mobj != NULL) &&
                             (dobj->mobj->next != NULL) &&
                             (dobj->mobj->next->next == NULL))
                    {
                        const u8 *base = (const u8 *)loaded->data;

                        star_step = 4u;
                        if ((base != NULL) &&
                            (loaded->data_size >= NDS_NATIVE_ITEM_STAR_FILE_END))
                        {
                            star_step = 5u;
                            if ((dl[8].words.w0 == NDS_NATIVE_ITEM_STAR_HOOK0_W0) &&
                                (dl[8].words.w1 == NDS_NATIVE_ITEM_STAR_HOOK0_W1) &&
                                (dl[23].words.w0 == NDS_NATIVE_ITEM_STAR_HOOK1_W0) &&
                                (dl[23].words.w1 == NDS_NATIVE_ITEM_STAR_HOOK1_W1) &&
                                (dl[14].words.w0 == NDS_NATIVE_ITEM_STAR_IMAGE_W0) &&
                                (dl[14].words.w1 ==
                                     (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_STAR_IMAGE_OFFSET)) &&
                                (dl[18].words.w1 ==
                                     (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_STAR_VERTEX0_OFFSET)) &&
                                (dl[29].words.w1 ==
                                     (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_STAR_VERTEX1_OFFSET)))
                            {
                                star_step = 6u;
                                if ((ndsRendererAdapterBuildNativeMaterialSnapshot(
                                         dobj->mobj, &item_star_material0, FALSE,
                                         NULL, NULL) != FALSE) &&
                                    (ndsRendererAdapterBuildNativeMaterialSnapshot(
                                         dobj->mobj->next, &item_star_material1, FALSE,
                                         NULL, NULL) != FALSE))
                                {
                                    star_step = 7u;
                                    gNdsItemStarEffectsSeen |=
                                        item_star_material0.effects |
                                        item_star_material1.effects;
                                    if ((item_star_material0.effects ==
                                             NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE) &&
                                        (item_star_material1.effects ==
                                             NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE))
                                    {
                                        star_step = 9u;
                                        item_star_native_candidate = TRUE;
                                    }
                                    else
                                    {
                                        gNdsItemStarEffectsRejected =
                                            item_star_material0.effects |
                                            item_star_material1.effects;
                                    }
                                }
                                else
                                {
                                    NDS_DIAG(gNdsItemStarSnapshotFailCount++);
                                }
                            }
                        }
                    }
                }
            }
        }
        if (star_step > gNdsItemStarCandidateStep)
        {
            gNdsItemStarCandidateStep = star_step;
        }
    }

    {
        u32 root = (loaded != NULL) ? ndsRelocNativeRootOffset(loaded, dl) : 0u;

        if ((loaded != NULL) &&
            (loaded->asset_id == NDS_NATIVE_ITEM_SWORD_ASSET) &&
            ((root == NDS_NATIVE_ITEM_SWORD_BLADE_ROOT) ||
             (root == NDS_NATIVE_ITEM_SWORD_HILT_ROOT)))
        {
            u32 sword_step = 1u;

            if (sNdsRendererAdapterItemSubmitActive != FALSE)
            {
                sword_step = 2u;
                if ((dobj->parent_gobj != NULL) &&
                    (dobj->parent_gobj->id == nGCCommonKindItem))
                {
                    ITStruct *ip = itGetStruct(dobj->parent_gobj);

                    sword_step = 3u;
                    if (ip != NULL)
                    {
                        gNdsItemSwordKind = (u32)ip->kind;
                        if (ip->kind != nITKindSword)
                        {
                            NDS_DIAG(gNdsItemSwordForeignKindCount++);
                        }
                        else if (dobj->mobj == NULL)
                        {
                            const u8 *base = (const u8 *)loaded->data;

                            sword_step = 4u;
                            if ((base != NULL) &&
                                (loaded->data_size >= NDS_NATIVE_ITEM_SWORD_FILE_END))
                            {
                                sb32 shape_ok = FALSE;

                                sword_step = 5u;
                                if (root == NDS_NATIVE_ITEM_SWORD_BLADE_ROOT)
                                {
                                    shape_ok = (dl[7].words.w1 ==
                                        (u32)(uintptr_t)(base +
                                            NDS_NATIVE_ITEM_SWORD_BLADE_VERTEX_OFFSET));
                                }
                                else
                                {
                                    shape_ok =
                                        (dl[10].words.w0 ==
                                             NDS_NATIVE_ITEM_SWORD_HILT_IMAGE_W0) &&
                                        (dl[10].words.w1 ==
                                             (u32)(uintptr_t)(base +
                                                 NDS_NATIVE_ITEM_SWORD_HILT_IMAGE_OFFSET)) &&
                                        (dl[15].words.w1 ==
                                             (u32)(uintptr_t)(base +
                                                 NDS_NATIVE_ITEM_SWORD_HILT_VERTEX_OFFSET));
                                }
                                if (shape_ok != FALSE)
                                {
                                    sword_step = 9u;
                                    item_sword_root = root;
                                    gNdsItemSwordRoot = root;
                                    item_sword_native_candidate = TRUE;
                                }
                            }
                        }
                    }
                }
            }
            if (sword_step > gNdsItemSwordCandidateStep)
            {
                gNdsItemSwordCandidateStep = sword_step;
            }
        }
    }

    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_HAMMER_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_HAMMER_ROOT))
    {
        u32 hammer_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            hammer_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *ip = itGetStruct(dobj->parent_gobj);

                hammer_step = 3u;
                if (ip != NULL)
                {
                    gNdsItemHammerKind = (u32)ip->kind;
                    if (ip->kind != nITKindHammer)
                    {
                        NDS_DIAG(gNdsItemHammerForeignKindCount++);
                    }
                    else if (dobj->mobj == NULL)
                    {
                        const u8 *base = (const u8 *)loaded->data;

                        hammer_step = 4u;
                        if ((base != NULL) &&
                            (loaded->data_size >= NDS_NATIVE_ITEM_HAMMER_FILE_END) &&
                            (dl[11].words.w0 == NDS_NATIVE_ITEM_HAMMER_TLUT_W0) &&
                            (dl[11].words.w1 ==
                                 (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_HAMMER_TLUT_OFFSET)) &&
                            (dl[17].words.w0 == NDS_NATIVE_ITEM_HAMMER_IMAGE_W0) &&
                            (dl[17].words.w1 ==
                                 (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_HAMMER_IMAGE_OFFSET)) &&
                            (dl[21].words.w1 ==
                                 (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_HAMMER_VERTEX0_OFFSET)) &&
                            (dl[36].words.w1 ==
                                 (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_HAMMER_VERTEX1_OFFSET)))
                        {
                            hammer_step = 9u;
                            item_hammer_native_candidate = TRUE;
                        }
                    }
                }
            }
        }
        if (hammer_step > gNdsItemHammerCandidateStep)
        {
            gNdsItemHammerCandidateStep = hammer_step;
        }
    }

    {
        u32 root = (loaded != NULL) ? ndsRelocNativeRootOffset(loaded, dl) : 0u;

        if ((loaded != NULL) &&
            (loaded->asset_id == NDS_NATIVE_ITEM_MBALL_ASSET) &&
            ((root == NDS_NATIVE_ITEM_MBALL_BAKED_ROOT) ||
             (root == NDS_NATIVE_ITEM_MBALL_LIVE_ROOT)))
        {
            u32 mball_step = 1u;
            /* BOTH POKE BALLS ARE ONE SOURCE DESCRIPTOR, SO THEY ARE ONE
             * NATIVE OWNER.
             *
             * These two roots are the two drawable children of the DObjDesc
             * at ITCommonObject+0x9430. The ground item reaches it as
             * ITAttributes.data at ITCommonData+0x6E4; BattleShip's entry
             * effect, efManagerMBallThrownMakeEffect (efmanager.c:5248),
             * reads that same fixed-up +0x6E4 pointer and SUBTRACTS 0x9430
             * from it to recover ITCommonObject's base -- so the subtrahend
             * IS this descriptor. Same tree, same MObjSub table at +0x9120,
             * same eight CURRENT_IMAGE frames; only the AnimJoint and
             * MatAnimJoint differ (the L/R pair the maker binds from lr).
             *
             * Requiring an ITEM GObj in the item display layer was therefore
             * the whole of the remaining P03 failure: the effect constructed,
             * arrived here as an EFFECT GObj in the effect layer, matched
             * nothing, and was published as DIAG_NATIVE domain 2 root 0x9340
             * reason 1 (NO_PROGRAM) -- which reads like missing geometry and
             * is not. Admit the effect owner against the same asset and the
             * same two roots; do NOT bake this geometry a second time.
             *
             * itGetStruct is deliberately not called on the effect GObj: an
             * EFFECT's obj is not an ITStruct. */
            const sb32 mball_is_effect =
                ((sNdsRendererAdapterEffectSubmitActive != FALSE) &&
                 (dobj->parent_gobj != NULL) &&
                 (dobj->parent_gobj->id == nGCCommonKindEffect)) ?
                    TRUE : FALSE;

            if ((sNdsRendererAdapterItemSubmitActive != FALSE) ||
                (mball_is_effect != FALSE))
            {
                mball_step = 2u;
                if ((dobj->parent_gobj != NULL) &&
                    ((dobj->parent_gobj->id == nGCCommonKindItem) ||
                     (mball_is_effect != FALSE)))
                {
                    ITStruct *ip = (mball_is_effect != FALSE) ? NULL :
                        itGetStruct(dobj->parent_gobj);

                    mball_step = 3u;
                    if ((ip != NULL) || (mball_is_effect != FALSE))
                    {
                        if (mball_is_effect != FALSE)
                        {
                            /* Bit 0 = 0x9250, bit 1 = 0x9340. One counter
                             * cannot say both halves of the ball arrived. */
                            gNdsEntryMBallThrownRootMask |=
                                (root == NDS_NATIVE_ITEM_MBALL_BAKED_ROOT) ?
                                    1u : 2u;
                        }
                        else
                        {
                            gNdsItemMBallKind = (u32)ip->kind;
                        }
                        if ((mball_is_effect == FALSE) &&
                            (ip->kind != nITKindMBall))
                        {
                            NDS_DIAG(gNdsItemMBallForeignKindCount++);
                        }
                        else if ((loaded->data != NULL) &&
                                 (loaded->data_size >= NDS_NATIVE_ITEM_MBALL_FILE_END))
                        {
                            const u8 *base = (const u8 *)loaded->data;

                            mball_step = 4u;
                            if (root == NDS_NATIVE_ITEM_MBALL_BAKED_ROOT)
                            {
                                if ((dobj->mobj == NULL) &&
                                    (dl[11].words.w0 ==
                                         NDS_NATIVE_ITEM_MBALL_BAKED_TLUT_W0) &&
                                    (dl[11].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_MBALL_BAKED_TLUT_OFFSET)) &&
                                    (dl[17].words.w0 ==
                                         NDS_NATIVE_ITEM_MBALL_BAKED_IMAGE_W0) &&
                                    (dl[17].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_MBALL_BAKED_IMAGE_OFFSET)) &&
                                    (dl[21].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_MBALL_BAKED_VERTEX_OFFSET)))
                                {
                                    mball_step = 9u;
                                    item_mball_root = root;
                                    gNdsItemMBallRoot = root;
                                    item_mball_native_candidate = TRUE;
                                    item_mball_from_effect = mball_is_effect;
                                }
                            }
                            else if ((dobj->mobj != NULL) &&
                                     (dobj->mobj->next == NULL) &&
                                     (dl[10].words.w0 ==
                                          NDS_NATIVE_ITEM_MBALL_LIVE_TLUT_W0) &&
                                     (dl[10].words.w1 ==
                                          (u32)(uintptr_t)(base +
                                              NDS_NATIVE_ITEM_MBALL_LIVE_TLUT_OFFSET)) &&
                                     (dl[16].words.w0 ==
                                          NDS_NATIVE_ITEM_MBALL_LIVE_HOOK_W0) &&
                                     (dl[16].words.w1 ==
                                          NDS_NATIVE_ITEM_MBALL_LIVE_HOOK_W1) &&
                                     (dl[21].words.w1 ==
                                          (u32)(uintptr_t)(base +
                                              NDS_NATIVE_ITEM_MBALL_LIVE_VERTEX_OFFSET)))
                            {
                                mball_step = 5u;
                                if (ndsRendererAdapterBuildNativeMaterialSnapshot(
                                        dobj->mobj, &item_mball_material, FALSE,
                                        NULL, NULL) != FALSE)
                                {
                                    mball_step = 6u;
                                    gNdsItemMBallEffectsSeen |=
                                        item_mball_material.effects;
                                    if (item_mball_material.effects ==
                                            NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE)
                                    {
                                        mball_step = 9u;
                                        item_mball_root = root;
                                        gNdsItemMBallRoot = root;
                                        item_mball_native_candidate = TRUE;
                                        item_mball_from_effect =
                                            mball_is_effect;
                                    }
                                    else
                                    {
                                        gNdsItemMBallEffectsRejected =
                                            item_mball_material.effects;
                                    }
                                }
                                else
                                {
                                    NDS_DIAG(gNdsItemMBallSnapshotFailCount++);
                                }
                            }
                        }
                    }
                }
            }
            if (mball_step > gNdsItemMBallCandidateStep)
            {
                gNdsItemMBallCandidateStep = mball_step;
            }
        }
    }

    {
        u32 root = (loaded != NULL) ? ndsRelocNativeRootOffset(loaded, dl) : 0u;

        if ((loaded != NULL) &&
            (loaded->asset_id == NDS_NATIVE_ITEM_KIRBYSTAR_ASSET) &&
            (root == NDS_NATIVE_ITEM_KIRBYSTAR_ROOT))
        {
            u32 kirbystar_step = 1u;
            /* FOUR SOURCE OWNERS, ONE ROOT, ONE BAKE -- AND NO KIND
             * DISCRIMINATOR IS NEEDED.
             *
             * ITCommonObject+0x5458 is dITCommonObject_StarRod_Weapon_data. A
             * whole-image referrer census over ITCommonData and ITCommonObject
             * finds exactly three pointers reaching it and none from any other
             * file: WPAttributes.data at ITCommonData+0x4D4 (StarRod) and
             * +0x508 (StarRodSmash), and the DObjDLLink pair at
             * ITCommonObject+0x550C. Kirby's two stars reach the same DL by
             * subtracting 0x5458 from that first pointer to recover the file
             * base -- the Poke Ball's trick at 0x9430, one file over. All four
             * draw the identical fixed quad with identical fixed material
             * state, so exact asset plus exact root IS the whole admission
             * test and the arm does not have to tell them apart.
             *
             * Admit from the WEAPON layer and the EFFECT layer. Requiring an
             * ITEM GObj was the entirety of the Poke Ball's remaining P03
             * failure (see the MBall arm above); this root has no item owner
             * at all, so the same clause would have refused every one of its
             * four users. itGetStruct is deliberately never called here.
             *
             * Counted separately: FromEffectCount isolates Kirby's two stars
             * from the Star Rod's swings, because a single draw count on a
             * shared root cannot say which owner drew and K04 is only proven
             * by the effect half. */
            const sb32 kirbystar_is_effect =
                ((sNdsRendererAdapterEffectSubmitActive != FALSE) &&
                 (dobj->parent_gobj != NULL) &&
                 (dobj->parent_gobj->id == nGCCommonKindEffect)) ?
                    TRUE : FALSE;

            if ((dobj->parent_gobj != NULL) && (loaded->data != NULL) &&
                (loaded->data_size >= NDS_NATIVE_ITEM_KIRBYSTAR_FILE_END))
            {
                const u8 *base = (const u8 *)loaded->data;

                kirbystar_step = 2u;
                /* No 0xDE and no MObjSub on any of the four owners, so a live
                 * MObj here means the source shape changed and the bake is
                 * stale. Refuse loudly rather than draw frozen material. */
                if ((dobj->mobj == NULL) &&
                    (dl[11].words.w0 == NDS_NATIVE_ITEM_KIRBYSTAR_IMAGE_W0) &&
                    (dl[11].words.w1 ==
                         (u32)(uintptr_t)(base +
                             NDS_NATIVE_ITEM_KIRBYSTAR_IMAGE_OFFSET)) &&
                    (dl[15].words.w1 ==
                         (u32)(uintptr_t)(base +
                             NDS_NATIVE_ITEM_KIRBYSTAR_VERTEX_OFFSET)))
                {
                    kirbystar_step = 9u;
                    item_kirbystar_native_candidate = TRUE;
                    if (kirbystar_is_effect != FALSE)
                    {
                        NDS_DIAG(gNdsItemKirbyStarFromEffectCount++);
                    }
                }
            }
            if (kirbystar_step > gNdsItemKirbyStarCandidateStep)
            {
                gNdsItemKirbyStarCandidateStep = kirbystar_step;
            }
        }
    }

    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_GSHELL_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_GSHELL_ROOT))
    {
        u32 gshell_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            gshell_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *ip = itGetStruct(dobj->parent_gobj);

                gshell_step = 3u;
                if (ip != NULL)
                {
                    gNdsItemGShellKind = (u32)ip->kind;
                    if (ip->kind == nITKindRShell)
                    {
                        /* Red Shell shares this exact source root and has its
                         * own kind-gated owner below. */
                    }
                    else if (ip->kind != nITKindGShell)
                    {
                        NDS_DIAG(gNdsItemGShellForeignKindCount++);
                    }
                    else if ((dobj->mobj != NULL) &&
                             (dobj->mobj->next == NULL) &&
                             (loaded->data != NULL) &&
                             (loaded->data_size >= NDS_NATIVE_ITEM_GSHELL_FILE_END))
                    {
                        const u8 *base = (const u8 *)loaded->data;

                        gshell_step = 4u;
                        if ((dl[12].words.w0 == NDS_NATIVE_ITEM_GSHELL_HOOK_W0) &&
                            (dl[12].words.w1 == NDS_NATIVE_ITEM_GSHELL_HOOK_W1) &&
                            (dl[17].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_GSHELL_VERTEX_OFFSET)))
                        {
                            gshell_step = 5u;
                            if (ndsRendererAdapterBuildNativeMaterialSnapshot(
                                    dobj->mobj, &item_gshell_material, FALSE,
                                    NULL, NULL) != FALSE)
                            {
                                const u32 expected =
                                    NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE |
                                    NDS_RENDERER_NATIVE_MATERIAL_PALETTE_TLUT |
                                    NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE;

                                gshell_step = 6u;
                                gNdsItemGShellEffectsSeen |=
                                    item_gshell_material.effects;
                                if (item_gshell_material.effects == expected)
                                {
                                    gshell_step = 9u;
                                    item_gshell_native_candidate = TRUE;
                                }
                                else
                                {
                                    gNdsItemGShellEffectsRejected =
                                        item_gshell_material.effects;
                                }
                            }
                            else
                            {
                                NDS_DIAG(gNdsItemGShellSnapshotFailCount++);
                            }
                        }
                    }
                }
            }
        }
        if (gshell_step > gNdsItemGShellCandidateStep)
        {
            gNdsItemGShellCandidateStep = gshell_step;
        }
    }

    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_RSHELL_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_RSHELL_ROOT))
    {
        u32 rshell_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            rshell_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *ip = itGetStruct(dobj->parent_gobj);

                rshell_step = 3u;
                if (ip != NULL)
                {
                    gNdsItemRShellKind = (u32)ip->kind;
                    if (ip->kind == nITKindGShell)
                    {
                        /* Green Shell owns the same shared root above. */
                    }
                    else if (ip->kind != nITKindRShell)
                    {
                        NDS_DIAG(gNdsItemRShellForeignKindCount++);
                    }
                    else if ((dobj->mobj != NULL) &&
                             (dobj->mobj->next == NULL) &&
                             (loaded->data != NULL) &&
                             (loaded->data_size >= NDS_NATIVE_ITEM_RSHELL_FILE_END))
                    {
                        const u8 *base = (const u8 *)loaded->data;

                        rshell_step = 4u;
                        if ((dl[12].words.w0 == NDS_NATIVE_ITEM_RSHELL_HOOK_W0) &&
                            (dl[12].words.w1 == NDS_NATIVE_ITEM_RSHELL_HOOK_W1) &&
                            (dl[17].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_RSHELL_VERTEX_OFFSET)))
                        {
                            rshell_step = 5u;
                            if (ndsRendererAdapterBuildNativeMaterialSnapshot(
                                    dobj->mobj, &item_rshell_material, FALSE,
                                    NULL, NULL) != FALSE)
                            {
                                const u32 expected =
                                    NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE |
                                    NDS_RENDERER_NATIVE_MATERIAL_PALETTE_TLUT |
                                    NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE;

                                rshell_step = 6u;
                                gNdsItemRShellEffectsSeen |=
                                    item_rshell_material.effects;
                                if (item_rshell_material.effects == expected)
                                {
                                    rshell_step = 9u;
                                    item_rshell_native_candidate = TRUE;
                                }
                                else
                                {
                                    gNdsItemRShellEffectsRejected =
                                        item_rshell_material.effects;
                                }
                            }
                            else
                            {
                                NDS_DIAG(gNdsItemRShellSnapshotFailCount++);
                            }
                        }
                    }
                }
            }
        }
        if (rshell_step > gNdsItemRShellCandidateStep)
        {
            gNdsItemRShellCandidateStep = rshell_step;
        }
    }

    {
        u32 root = (loaded != NULL) ? ndsRelocNativeRootOffset(loaded, dl) : 0u;

        if ((loaded != NULL) &&
            (loaded->asset_id == NDS_NATIVE_ITEM_BAT_ASSET) &&
            ((root == NDS_NATIVE_ITEM_BAT_HEADER_ROOT) ||
             (root == NDS_NATIVE_ITEM_BAT_SECOND_ROOT)))
        {
            u32 bat_step = 1u;

            if (sNdsRendererAdapterItemSubmitActive != FALSE)
            {
                bat_step = 2u;
                if ((dobj->parent_gobj != NULL) &&
                    (dobj->parent_gobj->id == nGCCommonKindItem))
                {
                    ITStruct *ip = itGetStruct(dobj->parent_gobj);

                    bat_step = 3u;
                    if (ip != NULL)
                    {
                        gNdsItemBatKind = (u32)ip->kind;
                        if (ip->kind != nITKindBat)
                        {
                            NDS_DIAG(gNdsItemBatForeignKindCount++);
                        }
                        else if ((dobj->mobj == NULL) &&
                                 (loaded->data != NULL) &&
                                 (loaded->data_size >= NDS_NATIVE_ITEM_BAT_FILE_END))
                        {
                            const u8 *base = (const u8 *)loaded->data;
                            sb32 shape_ok;

                            bat_step = 4u;
                            if (root == NDS_NATIVE_ITEM_BAT_HEADER_ROOT)
                            {
                                shape_ok =
                                    (dl[9].words.w0 ==
                                         NDS_NATIVE_ITEM_BAT_BRANCH_W0) &&
                                    (dl[9].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_BAT_CALLEE_ROOT));
                            }
                            else
                            {
                                shape_ok =
                                    (dl[14].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_BAT_VERTEX2_OFFSET));
                            }
                            if (shape_ok != FALSE)
                            {
                                bat_step = 9u;
                                item_bat_root = root;
                                gNdsItemBatRoot = root;
                                item_bat_native_candidate = TRUE;
                            }
                        }
                    }
                }
            }
            if (bat_step > gNdsItemBatCandidateStep)
            {
                gNdsItemBatCandidateStep = bat_step;
            }
        }
    }

    {
        u32 root = (loaded != NULL) ? ndsRelocNativeRootOffset(loaded, dl) : 0u;

        if ((loaded != NULL) &&
            (loaded->asset_id == NDS_NATIVE_ITEM_CAPSULE_ASSET) &&
            ((root == NDS_NATIVE_ITEM_CAPSULE_HEADER_ROOT) ||
             (root == NDS_NATIVE_ITEM_CAPSULE_SECOND_ROOT) ||
             (root == NDS_NATIVE_ITEM_CAPSULE_THIRD_ROOT)))
        {
            u32 capsule_step = 1u;

            if (sNdsRendererAdapterItemSubmitActive != FALSE)
            {
                capsule_step = 2u;
                if ((dobj->parent_gobj != NULL) &&
                    (dobj->parent_gobj->id == nGCCommonKindItem))
                {
                    ITStruct *ip = itGetStruct(dobj->parent_gobj);

                    capsule_step = 3u;
                    if (ip != NULL)
                    {
                        gNdsItemCapsuleKind = (u32)ip->kind;
                        if (ip->kind != nITKindCapsule)
                        {
                            NDS_DIAG(gNdsItemCapsuleForeignKindCount++);
                        }
                        else if ((dobj->mobj == NULL) &&
                                 (loaded->data != NULL) &&
                                 (loaded->data_size >= NDS_NATIVE_ITEM_CAPSULE_FILE_END))
                        {
                            const u8 *base = (const u8 *)loaded->data;
                            sb32 shape_ok;

                            capsule_step = 4u;
                            if (root == NDS_NATIVE_ITEM_CAPSULE_HEADER_ROOT)
                            {
                                shape_ok =
                                    (dl[10].words.w0 ==
                                         NDS_NATIVE_ITEM_CAPSULE_BRANCH_W0) &&
                                    (dl[10].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_CAPSULE_CALLEE_ROOT));
                            }
                            else if (root == NDS_NATIVE_ITEM_CAPSULE_SECOND_ROOT)
                            {
                                shape_ok =
                                    (dl[17].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_CAPSULE_VERTEX2_OFFSET));
                            }
                            else
                            {
                                shape_ok =
                                    (dl[9].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_CAPSULE_VERTEX3_OFFSET));
                            }
                            if (shape_ok != FALSE)
                            {
                                capsule_step = 9u;
                                item_capsule_root = root;
                                gNdsItemCapsuleRoot = root;
                                item_capsule_native_candidate = TRUE;
                            }
                        }
                    }
                }
            }
            if (capsule_step > gNdsItemCapsuleCandidateStep)
            {
                gNdsItemCapsuleCandidateStep = capsule_step;
            }
            if ((sNdsRendererAdapterItemSubmitActive != FALSE) &&
                (item_capsule_native_candidate == FALSE))
            {
                NDS_DIAG(gNdsItemCapsuleSubmitFailCount++);
            }
        }
    }

    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_BOMBHEI_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_BOMBHEI_ROOT))
    {
        u32 bombhei_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            bombhei_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *ip = itGetStruct(dobj->parent_gobj);

                bombhei_step = 3u;
                if (ip != NULL)
                {
                    gNdsItemBombHeiKind = (u32)ip->kind;
                    if (ip->kind != nITKindBombHei)
                    {
                        NDS_DIAG(gNdsItemBombHeiForeignKindCount++);
                    }
                    else if ((dobj->mobj != NULL) &&
                             (dobj->mobj->next == NULL) &&
                             (loaded->data != NULL) &&
                             (loaded->data_size >= NDS_NATIVE_ITEM_BOMBHEI_FILE_END))
                    {
                        const u8 *base = (const u8 *)loaded->data;

                        bombhei_step = 4u;
                        if ((dl[11].words.w0 == NDS_NATIVE_ITEM_BOMBHEI_TLUT_W0) &&
                            (dl[11].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_BOMBHEI_TLUT_OFFSET)) &&
                            (dl[17].words.w0 == NDS_NATIVE_ITEM_BOMBHEI_HOOK_W0) &&
                            (dl[17].words.w1 == NDS_NATIVE_ITEM_BOMBHEI_HOOK_W1) &&
                            (dl[21].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_BOMBHEI_VERTEX_OFFSET)))
                        {
                            bombhei_step = 5u;
                            if (ndsRendererAdapterBuildNativeMaterialSnapshot(
                                    dobj->mobj, &item_bombhei_material, FALSE,
                                    NULL, NULL) != FALSE)
                            {
                                bombhei_step = 6u;
                                gNdsItemBombHeiEffectsSeen |=
                                    item_bombhei_material.effects;
                                if (item_bombhei_material.effects ==
                                        NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE)
                                {
                                    bombhei_step = 9u;
                                    item_bombhei_native_candidate = TRUE;
                                }
                                else
                                {
                                    gNdsItemBombHeiEffectsRejected =
                                        item_bombhei_material.effects;
                                }
                            }
                            else
                            {
                                NDS_DIAG(gNdsItemBombHeiSnapshotFailCount++);
                            }
                        }
                    }
                }
            }
        }
        if (bombhei_step > gNdsItemBombHeiCandidateStep)
        {
            gNdsItemBombHeiCandidateStep = bombhei_step;
        }
    }

    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_LGUN_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_LGUN_ROOT))
    {
        u32 lgun_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            lgun_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *ip = itGetStruct(dobj->parent_gobj);

                lgun_step = 3u;
                if (ip != NULL)
                {
                    gNdsItemLGunKind = (u32)ip->kind;
                    if (ip->kind != nITKindLGun)
                    {
                        NDS_DIAG(gNdsItemLGunForeignKindCount++);
                    }
                    else if ((dobj->mobj == NULL) &&
                             (loaded->data != NULL) &&
                             (loaded->data_size >= NDS_NATIVE_ITEM_LGUN_FILE_END))
                    {
                        const u8 *base = (const u8 *)loaded->data;

                        lgun_step = 4u;
                        if ((dl[11].words.w0 == NDS_NATIVE_ITEM_LGUN_TLUT_W0) &&
                            (dl[11].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_LGUN_TLUT_OFFSET)) &&
                            (dl[17].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_LGUN_IMAGE0_OFFSET)) &&
                            (dl[22].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_LGUN_VERTEX0_OFFSET)) &&
                            (dl[28].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_LGUN_IMAGE1_OFFSET)) &&
                            (dl[32].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_LGUN_VERTEX1_OFFSET)) &&
                            (dl[38].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_LGUN_IMAGE2_OFFSET)) &&
                            (dl[43].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_LGUN_VERTEX2_OFFSET)))
                        {
                            lgun_step = 9u;
                            item_lgun_native_candidate = TRUE;
                        }
                    }
                }
            }
        }
        if (lgun_step > gNdsItemLGunCandidateStep)
        {
            gNdsItemLGunCandidateStep = lgun_step;
        }
    }

    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_HARISEN_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_HARISEN_ROOT))
    {
        u32 harisen_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            harisen_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *ip = itGetStruct(dobj->parent_gobj);

                harisen_step = 3u;
                if (ip != NULL)
                {
                    gNdsItemHarisenKind = (u32)ip->kind;
                    if (ip->kind != nITKindHarisen)
                    {
                        NDS_DIAG(gNdsItemHarisenForeignKindCount++);
                    }
                    else if ((dobj->mobj == NULL) &&
                             (loaded->data != NULL) &&
                             (loaded->data_size >= NDS_NATIVE_ITEM_HARISEN_FILE_END))
                    {
                        const u8 *base = (const u8 *)loaded->data;

                        harisen_step = 4u;
                        if ((dl[11].words.w0 == NDS_NATIVE_ITEM_HARISEN_TLUT_W0) &&
                            (dl[11].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_HARISEN_TLUT_OFFSET)) &&
                            (dl[17].words.w0 == NDS_NATIVE_ITEM_HARISEN_IMAGE_W0) &&
                            (dl[17].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_HARISEN_IMAGE_OFFSET)) &&
                            (dl[22].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_HARISEN_VERTEX_OFFSET)))
                        {
                            harisen_step = 9u;
                            item_harisen_native_candidate = TRUE;
                        }
                    }
                }
            }
        }
        if (harisen_step > gNdsItemHarisenCandidateStep)
        {
            gNdsItemHarisenCandidateStep = harisen_step;
        }
    }

    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_HEART_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_HEART_ROOT))
    {
        u32 heart_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            heart_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *ip = itGetStruct(dobj->parent_gobj);

                heart_step = 3u;
                if (ip != NULL)
                {
                    gNdsItemHeartKind = (u32)ip->kind;
                    if (ip->kind != nITKindHeart)
                    {
                        NDS_DIAG(gNdsItemHeartForeignKindCount++);
                    }
                    else if ((dobj->mobj == NULL) &&
                             (loaded->data != NULL) &&
                             (loaded->data_size >= NDS_NATIVE_ITEM_HEART_FILE_END))
                    {
                        const u8 *base = (const u8 *)loaded->data;

                        heart_step = 4u;
                        if ((dl[11].words.w0 == NDS_NATIVE_ITEM_HEART_TLUT_W0) &&
                            (dl[11].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_HEART_TLUT_OFFSET)) &&
                            (dl[17].words.w0 == NDS_NATIVE_ITEM_HEART_IMAGE0_W0) &&
                            (dl[17].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_HEART_IMAGE0_OFFSET)) &&
                            (dl[21].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_HEART_VERTEX0_OFFSET)) &&
                            (dl[32].words.w0 == NDS_NATIVE_ITEM_HEART_IMAGE1_W0) &&
                            (dl[32].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_HEART_IMAGE1_OFFSET)) &&
                            (dl[36].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_HEART_VERTEX1_OFFSET)))
                        {
                            heart_step = 9u;
                            item_heart_native_candidate = TRUE;
                        }
                    }
                }
            }
        }
        if (heart_step > gNdsItemHeartCandidateStep)
        {
            gNdsItemHeartCandidateStep = heart_step;
        }
    }

    {
        u32 root = (loaded != NULL) ? ndsRelocNativeRootOffset(loaded, dl) : 0u;

        if ((loaded != NULL) &&
            (loaded->asset_id == NDS_NATIVE_ITEM_STARROD_ASSET) &&
            ((root == NDS_NATIVE_ITEM_STARROD_HEADER_ROOT) ||
             (root == NDS_NATIVE_ITEM_STARROD_SECOND_ROOT)))
        {
            u32 starrod_step = 1u;

            if (sNdsRendererAdapterItemSubmitActive != FALSE)
            {
                starrod_step = 2u;
                if ((dobj->parent_gobj != NULL) &&
                    (dobj->parent_gobj->id == nGCCommonKindItem))
                {
                    ITStruct *ip = itGetStruct(dobj->parent_gobj);

                    starrod_step = 3u;
                    if (ip != NULL)
                    {
                        gNdsItemStarRodKind = (u32)ip->kind;
                        if (ip->kind != nITKindStarRod)
                        {
                            NDS_DIAG(gNdsItemStarRodForeignKindCount++);
                        }
                        else if ((dobj->mobj == NULL) &&
                                 (loaded->data != NULL) &&
                                 (loaded->data_size >= NDS_NATIVE_ITEM_STARROD_FILE_END))
                        {
                            const u8 *base = (const u8 *)loaded->data;
                            sb32 shape_ok;

                            starrod_step = 4u;
                            if (root == NDS_NATIVE_ITEM_STARROD_HEADER_ROOT)
                            {
                                shape_ok =
                                    (dl[11].words.w0 ==
                                         NDS_NATIVE_ITEM_STARROD_BRANCH_W0) &&
                                    (dl[11].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_STARROD_CALLEE_ROOT));
                            }
                            else
                            {
                                shape_ok =
                                    (dl[5].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_STARROD_TLUT1_OFFSET)) &&
                                    (dl[10].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_STARROD_IMAGE1_OFFSET)) &&
                                    (dl[15].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_STARROD_VERTEX1_OFFSET));
                            }
                            if (shape_ok != FALSE)
                            {
                                starrod_step = 9u;
                                item_starrod_root = root;
                                gNdsItemStarRodRoot = root;
                                item_starrod_native_candidate = TRUE;
                            }
                        }
                    }
                }
            }
            if (starrod_step > gNdsItemStarRodCandidateStep)
            {
                gNdsItemStarRodCandidateStep = starrod_step;
            }
        }
    }

    {
        u32 root = (loaded != NULL) ? ndsRelocNativeRootOffset(loaded, dl) : 0u;

        if ((loaded != NULL) &&
            (loaded->asset_id == NDS_NATIVE_ITEM_FFLOWER_ASSET) &&
            ((root == NDS_NATIVE_ITEM_FFLOWER_BRANCH_ROOT) ||
             (root == NDS_NATIVE_ITEM_FFLOWER_LIVE_ROOT)))
        {
            u32 fflower_step = 1u;

            if (sNdsRendererAdapterItemSubmitActive != FALSE)
            {
                fflower_step = 2u;
                if ((dobj->parent_gobj != NULL) &&
                    (dobj->parent_gobj->id == nGCCommonKindItem))
                {
                    ITStruct *ip = itGetStruct(dobj->parent_gobj);

                    fflower_step = 3u;
                    if (ip != NULL)
                    {
                        gNdsItemFFlowerKind = (u32)ip->kind;
                        if (ip->kind != nITKindFFlower)
                        {
                            NDS_DIAG(gNdsItemFFlowerForeignKindCount++);
                        }
                        else if ((loaded->data != NULL) &&
                                 (loaded->data_size >= NDS_NATIVE_ITEM_FFLOWER_FILE_END))
                        {
                            const u8 *base = (const u8 *)loaded->data;

                            fflower_step = 4u;
                            if ((root == NDS_NATIVE_ITEM_FFLOWER_BRANCH_ROOT) &&
                                (dobj->mobj == NULL) &&
                                (dl[6].words.w1 ==
                                     (u32)(uintptr_t)(base +
                                         NDS_NATIVE_ITEM_FFLOWER_BRANCH_TLUT_OFFSET)) &&
                                (dl[9].words.w0 ==
                                     NDS_NATIVE_ITEM_FFLOWER_BRANCH_W0) &&
                                (dl[9].words.w1 ==
                                     (u32)(uintptr_t)(base +
                                         NDS_NATIVE_ITEM_FFLOWER_CALLEE_ROOT)))
                            {
                                fflower_step = 9u;
                                item_fflower_root = root;
                                gNdsItemFFlowerRoot = root;
                                item_fflower_native_candidate = TRUE;
                            }
                            else if ((root == NDS_NATIVE_ITEM_FFLOWER_LIVE_ROOT) &&
                                     (dobj->mobj != NULL) &&
                                     (dobj->mobj->next == NULL) &&
                                     (ndsRendererAdapterMaterialFlags(dobj->mobj) ==
                                          NDS_NATIVE_ITEM_FFLOWER_MOBJ_FLAGS) &&
                                     (dl[3].words.w0 == NDS_NATIVE_ITEM_FFLOWER_HOOK_W0) &&
                                     (dl[3].words.w1 == NDS_NATIVE_ITEM_FFLOWER_HOOK_W1) &&
                                     (dl[8].words.w1 ==
                                          (u32)(uintptr_t)(base +
                                              NDS_NATIVE_ITEM_FFLOWER_LIVE_IMAGE_OFFSET)) &&
                                     (dl[12].words.w1 ==
                                          (u32)(uintptr_t)(base +
                                              NDS_NATIVE_ITEM_FFLOWER_LIVE_VERTEX_OFFSET)))
                            {
                                fflower_step = 5u;
                                if (ndsRendererAdapterBuildNativeMaterialSnapshot(
                                        dobj->mobj, &item_fflower_material, FALSE,
                                        NULL, NULL) != FALSE)
                                {
                                    fflower_step = 6u;
                                    gNdsItemFFlowerEffectsSeen |=
                                        item_fflower_material.effects;
                                    if (item_fflower_material.effects ==
                                            NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE)
                                    {
                                        fflower_step = 9u;
                                        item_fflower_root = root;
                                        gNdsItemFFlowerRoot = root;
                                        item_fflower_native_candidate = TRUE;
                                    }
                                    else
                                    {
                                        gNdsItemFFlowerEffectsRejected =
                                            item_fflower_material.effects;
                                    }
                                }
                                else
                                {
                                    NDS_DIAG(gNdsItemFFlowerSnapshotFailCount++);
                                }
                            }
                        }
                    }
                }
            }
            if (fflower_step > gNdsItemFFlowerCandidateStep)
            {
                gNdsItemFFlowerCandidateStep = fflower_step;
            }
        }
    }

    {
        u32 root = (loaded != NULL) ? ndsRelocNativeRootOffset(loaded, dl) : 0u;

        if ((loaded != NULL) &&
            (loaded->asset_id == NDS_NATIVE_ITEM_MSBOMB_ASSET) &&
            ((root == NDS_NATIVE_ITEM_MSBOMB_ROOT0) ||
             (root == NDS_NATIVE_ITEM_MSBOMB_ROOT1)))
        {
            u32 msbomb_step = 1u;

            if (sNdsRendererAdapterItemSubmitActive != FALSE)
            {
                msbomb_step = 2u;
                if ((dobj->parent_gobj != NULL) &&
                    (dobj->parent_gobj->id == nGCCommonKindItem))
                {
                    ITStruct *ip = itGetStruct(dobj->parent_gobj);

                    msbomb_step = 3u;
                    if (ip != NULL)
                    {
                        gNdsItemMSBombKind = (u32)ip->kind;
                        if (ip->kind != nITKindMSBomb)
                        {
                            NDS_DIAG(gNdsItemMSBombForeignKindCount++);
                        }
                        else if ((dobj->mobj == NULL) &&
                                 (loaded->data != NULL) &&
                                 (loaded->data_size >= NDS_NATIVE_ITEM_MSBOMB_FILE_END))
                        {
                            const u8 *base = (const u8 *)loaded->data;
                            sb32 shape_ok;

                            msbomb_step = 4u;
                            if (root == NDS_NATIVE_ITEM_MSBOMB_ROOT0)
                            {
                                shape_ok =
                                    (dl[14].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_MSBOMB_TLUT0_OFFSET)) &&
                                    (dl[20].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_MSBOMB_IMAGE0_OFFSET)) &&
                                    (dl[25].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_MSBOMB_VERTEX0_OFFSET));
                            }
                            else
                            {
                                shape_ok =
                                    (dl[10].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_MSBOMB_TLUT1_OFFSET)) &&
                                    (dl[16].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_MSBOMB_IMAGE1_OFFSET)) &&
                                    (dl[21].words.w1 ==
                                         (u32)(uintptr_t)(base +
                                             NDS_NATIVE_ITEM_MSBOMB_VERTEX1_OFFSET));
                            }
                            if (shape_ok != FALSE)
                            {
                                msbomb_step = 9u;
                                item_msbomb_root = root;
                                gNdsItemMSBombRoot = root;
                                item_msbomb_native_candidate = TRUE;
                            }
                        }
                    }
                }
            }
            if (msbomb_step > gNdsItemMSBombCandidateStep)
            {
                gNdsItemMSBombCandidateStep = msbomb_step;
            }
        }
    }

    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_NBUMPER_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_NBUMPER_ROOT))
    {
        u32 nbumper_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            nbumper_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *ip = itGetStruct(dobj->parent_gobj);

                nbumper_step = 3u;
                if (ip != NULL)
                {
                    gNdsItemNBumperKind = (u32)ip->kind;
                    if (ip->kind != nITKindNBumper)
                    {
                        NDS_DIAG(gNdsItemNBumperForeignKindCount++);
                    }
                    else if ((dobj->mobj != NULL) &&
                             (dobj->mobj->next == NULL) &&
                             (ndsRendererAdapterMaterialFlags(dobj->mobj) ==
                                  NDS_NATIVE_ITEM_NBUMPER_MOBJ_FLAGS) &&
                             (loaded->data != NULL) &&
                             (loaded->data_size >= NDS_NATIVE_ITEM_NBUMPER_FILE_END))
                    {
                        const u8 *base = (const u8 *)loaded->data;

                        nbumper_step = 4u;
                        if ((dl[10].words.w0 == NDS_NATIVE_ITEM_NBUMPER_HOOK_W0) &&
                            (dl[10].words.w1 == NDS_NATIVE_ITEM_NBUMPER_HOOK_W1) &&
                            (dl[16].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_NBUMPER_IMAGE_OFFSET)) &&
                            (dl[21].words.w1 ==
                                 (u32)(uintptr_t)(base +
                                     NDS_NATIVE_ITEM_NBUMPER_VERTEX_OFFSET)))
                        {
                            nbumper_step = 5u;
                            if (ndsRendererAdapterBuildNativeMaterialSnapshot(
                                    dobj->mobj, &item_nbumper_material, FALSE,
                                    NULL, NULL) != FALSE)
                            {
                                nbumper_step = 6u;
                                gNdsItemNBumperEffectsSeen |=
                                    item_nbumper_material.effects;
                                if (item_nbumper_material.effects ==
                                        NDS_RENDERER_NATIVE_MATERIAL_PALETTE_IMAGE)
                                {
                                    nbumper_step = 9u;
                                    item_nbumper_native_candidate = TRUE;
                                }
                                else
                                {
                                    gNdsItemNBumperEffectsRejected =
                                        item_nbumper_material.effects;
                                }
                            }
                            else
                            {
                                NDS_DIAG(gNdsItemNBumperSnapshotFailCount++);
                            }
                        }
                    }
                }
            }
        }
        if (nbumper_step > gNdsItemNBumperCandidateStep)
        {
            gNdsItemNBumperCandidateStep = nbumper_step;
        }
    }

    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_BOX_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_BOX_ROOT))
    {
        u32 box_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            box_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *ip = itGetStruct(dobj->parent_gobj);

                box_step = 3u;
                if (ip != NULL)
                {
                    gNdsItemBoxKind = (u32)ip->kind;
                    if (ip->kind != nITKindBox)
                    {
                        NDS_DIAG(gNdsItemBoxForeignKindCount++);
                    }
                    else if ((dobj->mobj == NULL) &&
                             (loaded->data != NULL) &&
                             (loaded->data_size >= NDS_NATIVE_ITEM_BOX_FILE_END))
                    {
                        const u8 *base = (const u8 *)loaded->data;

                        box_step = 4u;
                        if ((dl[8].words.w0 == NDS_NATIVE_ITEM_BOX_TLUT0_W0) &&
                            (dl[8].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_BOX_TLUT0_OFFSET)) &&
                            (dl[14].words.w0 == NDS_NATIVE_ITEM_BOX_IMAGE0_W0) &&
                            (dl[14].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_BOX_IMAGE0_OFFSET)) &&
                            (dl[18].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_BOX_VERTEX0_OFFSET)) &&
                            (dl[24].words.w0 == NDS_NATIVE_ITEM_BOX_TLUT1_W0) &&
                            (dl[24].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_BOX_TLUT1_OFFSET)) &&
                            (dl[28].words.w0 == NDS_NATIVE_ITEM_BOX_IMAGE1_W0) &&
                            (dl[28].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_BOX_IMAGE1_OFFSET)) &&
                            (dl[32].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_BOX_VERTEX1A_OFFSET)) &&
                            (dl[33].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_BOX_VERTEX1B_OFFSET)))
                        {
                            box_step = 9u;
                            item_box_native_candidate = TRUE;
                        }
                    }
                }
            }
        }
        if (box_step > gNdsItemBoxCandidateStep)
        {
            gNdsItemBoxCandidateStep = box_step;
        }
    }

    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_TARU_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_TARU_ROOT))
    {
        u32 taru_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            taru_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *ip = itGetStruct(dobj->parent_gobj);

                taru_step = 3u;
                if (ip != NULL)
                {
                    gNdsItemTaruKind = (u32)ip->kind;
                    if (ip->kind != nITKindTaru)
                    {
                        NDS_DIAG(gNdsItemTaruForeignKindCount++);
                    }
                    else if ((dobj->mobj == NULL) &&
                             (loaded->data != NULL) &&
                             (loaded->data_size >= NDS_NATIVE_ITEM_TARU_FILE_END))
                    {
                        const u8 *base = (const u8 *)loaded->data;

                        taru_step = 4u;
                        if ((dl[11].words.w0 == NDS_NATIVE_ITEM_TARU_TLUT0_W0) &&
                            (dl[11].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_TARU_TLUT0_OFFSET)) &&
                            (dl[17].words.w0 == NDS_NATIVE_ITEM_TARU_IMAGE0_W0) &&
                            (dl[17].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_TARU_IMAGE0_OFFSET)) &&
                            (dl[21].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_TARU_VERTEX0_OFFSET)) &&
                            (dl[29].words.w0 == NDS_NATIVE_ITEM_TARU_TLUT1_W0) &&
                            (dl[29].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_TARU_TLUT1_OFFSET)) &&
                            (dl[34].words.w0 == NDS_NATIVE_ITEM_TARU_IMAGE1_W0) &&
                            (dl[34].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_TARU_IMAGE1_OFFSET)) &&
                            (dl[38].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_TARU_VERTEX1_OFFSET)))
                        {
                            taru_step = 9u;
                            item_taru_native_candidate = TRUE;
                        }
                    }
                }
            }
        }
        if (taru_step > gNdsItemTaruCandidateStep)
        {
            gNdsItemTaruCandidateStep = taru_step;
        }
    }

    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_EGG_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_EGG_ROOT))
    {
        u32 egg_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            egg_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *ip = itGetStruct(dobj->parent_gobj);

                egg_step = 3u;
                if (ip != NULL)
                {
                    gNdsItemEggKind = (u32)ip->kind;
                    if (ip->kind != nITKindEgg)
                    {
                        NDS_DIAG(gNdsItemEggForeignKindCount++);
                    }
                    else if ((dobj->mobj == NULL) &&
                             (loaded->data != NULL) &&
                             (loaded->data_size >= NDS_NATIVE_ITEM_EGG_FILE_END))
                    {
                        const u8 *base = (const u8 *)loaded->data;

                        egg_step = 4u;
                        if ((dl[10].words.w0 == NDS_NATIVE_ITEM_EGG_TLUT_W0) &&
                            (dl[10].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_EGG_TLUT_OFFSET)) &&
                            (dl[16].words.w0 == NDS_NATIVE_ITEM_EGG_IMAGE_W0) &&
                            (dl[16].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_EGG_IMAGE_OFFSET)) &&
                            (dl[21].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_EGG_VERTEX_OFFSET)))
                        {
                            egg_step = 9u;
                            item_egg_native_candidate = TRUE;
                        }
                    }
                }
            }
        }
        if (egg_step > gNdsItemEggCandidateStep)
        {
            gNdsItemEggCandidateStep = egg_step;
        }
    }

    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_IWARK_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_IWARK_ROOT))
    {
        u32 iwark_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            iwark_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *ip = itGetStruct(dobj->parent_gobj);

                iwark_step = 3u;
                if (ip != NULL)
                {
                    gNdsItemIwarkKind = (u32)ip->kind;
                    if (ip->kind != nITKindIwark)
                    {
                        NDS_DIAG(gNdsItemIwarkForeignKindCount++);
                    }
                    else if ((dobj->mobj == NULL) &&
                             (loaded->data != NULL) &&
                             (loaded->data_size >= NDS_NATIVE_ITEM_IWARK_FILE_END))
                    {
                        const u8 *base = (const u8 *)loaded->data;

                        iwark_step = 4u;
                        if ((dl[10].words.w0 == NDS_NATIVE_ITEM_IWARK_TLUT_W0) &&
                            (dl[10].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_IWARK_TLUT_OFFSET)) &&
                            (dl[16].words.w0 == NDS_NATIVE_ITEM_IWARK_IMAGE_W0) &&
                            (dl[16].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_IWARK_IMAGE_OFFSET)) &&
                            (dl[21].words.w1 == (u32)(uintptr_t)(base + NDS_NATIVE_ITEM_IWARK_VERTEX_OFFSET)))
                        {
                            iwark_step = 9u;
                            item_iwark_native_candidate = TRUE;
                        }
                    }
                }
            }
        }
        if (iwark_step > gNdsItemIwarkCandidateStep)
        {
            gNdsItemIwarkCandidateStep = iwark_step;
        }
    }

    /* The Maxim Tomato, file 86 root 0x09c0.  Thirty words, four vertices, two
     * triangles, NO 0xDE opcode anywhere in the list and `p_mobjsubs` NULL --
     * so it owns its whole material and every word of it bakes.  The combiner
     * is TEXEL0 x SHADE in both cycles, with no PRIM or ENV, so the item
     * layer's seeded colours cannot reach this draw either.
     *
     * A whole-image sweep of all 2,132 O2R files finds 69 pointers into asset
     * 86 -- 68 of them file 251's own item attribute slots and one foreign
     * reference from YoshiMain into a different offset entirely -- so asset and
     * root discriminate this owner completely.  The kind is still read and
     * compared, for the same reason the Marumine's is: if a foreign kind ever
     * reaches this root, say WHY rather than leave it indistinguishable from a
     * submit refusal. */
    if ((loaded != NULL) &&
        (loaded->asset_id == NDS_NATIVE_ITEM_TOMATO_ASSET) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_ITEM_TOMATO_ROOT))
    {
        u32 tomato_step = 1u;

        if (sNdsRendererAdapterItemSubmitActive != FALSE)
        {
            tomato_step = 2u;
            if ((dobj->parent_gobj != NULL) &&
                (dobj->parent_gobj->id == nGCCommonKindItem))
            {
                ITStruct *tomato_ip = itGetStruct(dobj->parent_gobj);

                tomato_step = 3u;
                if (tomato_ip != NULL)
                {
                    gNdsItemTomatoKind = (u32)tomato_ip->kind;
                    if (tomato_ip->kind != nITKindTomato)
                    {
                        NDS_DIAG(gNdsItemTomatoForeignKindCount++);
                    }
                    else
                    {
                        tomato_step = 4u;
                        /* material 0 is the NULL MObj, NOT "untextured". */
                        if (dobj->mobj == NULL)
                        {
                            tomato_step = 5u;
                            if ((loaded->data != NULL) &&
                                (loaded->data_size >=
                                     NDS_NATIVE_ITEM_TOMATO_FILE_END) &&
                                (loaded->data_size >=
                                     (NDS_NATIVE_ITEM_TOMATO_ROOT +
                                      NDS_NATIVE_ITEM_TOMATO_DL_BYTES)))
                            {
                                const u8 *tomato_base =
                                    (const u8 *)loaded->data;

                                tomato_step = 6u;
                                if ((dl[11].words.w0 ==
                                         NDS_NATIVE_ITEM_TOMATO_TLUT_W0) &&
                                    (dl[17].words.w0 ==
                                         NDS_NATIVE_ITEM_TOMATO_IMAGE_W0))
                                {
                                    tomato_step = 7u;
                                    /* COMPARE the relocated pointers against
                                     * this file's own base -- never assume the
                                     * loader's fixup pass ran, and never bind a
                                     * chain word as an image. */
                                    if ((dl[11].words.w1 ==
                                             (u32)(uintptr_t)(tomato_base +
                                                 NDS_NATIVE_ITEM_TOMATO_TLUT_OFFSET)) &&
                                        (dl[17].words.w1 ==
                                             (u32)(uintptr_t)(tomato_base +
                                                 NDS_NATIVE_ITEM_TOMATO_IMAGE_OFFSET)))
                                    {
                                        tomato_step = 8u;
                                        if (dl[21].words.w1 ==
                                                (u32)(uintptr_t)(tomato_base +
                                                    NDS_NATIVE_ITEM_TOMATO_VERTEX_OFFSET))
                                        {
                                            tomato_step = 9u;
                                            item_tomato_native_candidate = TRUE;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        if (tomato_step > gNdsItemTomatoCandidateStep)
        {
            gNdsItemTomatoCandidateStep = tomato_step;
        }
    }
#endif
    /* The procedural visual templates. Claimed here, before the loaded-file
     * scan, because the owner needs nothing from `loaded`, from the material
     * segment or from the callback context -- and because the template GObj
     * carries exactly one DObj and exactly one list, so the per-GObj latch
     * cannot mis-attribute across nodes. dobj->child == NULL is an assertion,
     * not an assumption: this owner is baked for a single flat fan or ring and
     * must decline loudly rather than draw a shape it was not baked for. */
    if ((sNdsRendererAdapterVisualEffectNativeActive != FALSE) &&
        (dobj->dl == dl) && (dobj->child == NULL))
    {
        visual_effect_native_candidate = TRUE;
        visual_effect_template = sNdsRendererAdapterVisualEffectTemplate;
    }
#if NDS_R2_REBIRTH_HALO_NATIVE
    if ((sNdsRendererAdapterRebirthHaloNativeActive != FALSE) &&
        (gEFManagerFiles[2] != NULL) &&
        ((const u8 *)dl >= (const u8 *)gEFManagerFiles[2]))
    {
        uintptr_t offset = (uintptr_t)((const u8 *)dl -
                                       (const u8 *)gEFManagerFiles[2]);

        if ((offset == 0x2378u) || (offset == 0x2a88u) ||
            (offset == 0x27e8u))
        {
            rebirth_halo_root_offset = (u32)offset;
            rebirth_halo_native_candidate = TRUE;
        }
    }
#endif
#if NDS_TICK_HUD && NDS_P2_EFFECT_CENSUS
    if (phase_effect != FALSE)
    {
        gNdsEffectPhaseFindTicks += cpuGetTiming() - phase_mark;
    }
#endif

    NDS_LAB_SDL_MARK(0);
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    detailed_output = (ndsRendererHardwareNoOracleEnabled() == FALSE) ?
        TRUE : FALSE;
    if (detailed_output != FALSE)
    {
        bzero(&state, sizeof(state));
    }
    else
    {
        /* Profile 0/1 submit with a null command callback. Only the compact
         * branch/data resolver context is live; software-preview vertices
         * are already retained by the renderer's persistent vertex cache. */
        state.segment_e_base = NULL;
        state.segment_e_end = NULL;
    }
#else
    bzero(&state, sizeof(state));
#endif
    state.primary_file = loaded;
    state.slot = 0u;
#if NDS_RENDERER_HW_TRIANGLES
    if (sNdsRendererAdapterStagePersistentActive != FALSE)
    {
        inherited_texture = ndsRendererAdapterStatsHasArmedTexture(
            &sNdsRendererAdapterStagePersistentStats);
        inherited_tile = ndsRendererAdapterStatsHasArmedTile(
            &sNdsRendererAdapterStagePersistentStats);
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if (detailed_output != FALSE)
        {
            ndsFighterDLDrawSeedPersistentState(
                &state, &sNdsRendererAdapterStagePersistentState);
        }
        else
        {
            state.segment_e_base =
                sNdsRendererAdapterStagePersistentState.segment_e_base;
            state.segment_e_end =
                sNdsRendererAdapterStagePersistentState.segment_e_end;
        }
#else
        ndsFighterDLDrawSeedPersistentState(
            &state, &sNdsRendererAdapterStagePersistentState);
#endif
        inherited_segment = (state.segment_e_base != NULL) ? TRUE : FALSE;
        NDS_DIAG(gNdsStageGCDrawAllLoopHardwareCarrySeedCount++);
        if (inherited_texture != FALSE)
        {
            NDS_DIAG(gNdsStageGCDrawAllLoopHardwareCarryTextureSeedCount++);
        }
        if (inherited_tile != FALSE)
        {
            NDS_DIAG(gNdsStageGCDrawAllLoopHardwareCarryTileSeedCount++);
        }
        if (inherited_segment != FALSE)
        {
            NDS_DIAG(gNdsStageGCDrawAllLoopHardwareCarrySegmentSeedCount++);
        }
    }
    NDS_LAB_SDL_MARK(4);
    saved_graphics_heap_ptr = gSYTaskmanGraphicsHeap.ptr;
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    adapter_start = cpuGetTiming();
    step_start = adapter_start;
#elif NDS_RENDERER_PROFILE_LEVEL >= 1
    step_start = cpuGetTiming();
#endif
#endif
#if NDS_TICK_HUD && NDS_P2_EFFECT_CENSUS
    if (phase_effect != FALSE)
    {
        phase_mark = cpuGetTiming();
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_CASTLE
    if (castle_bumper_native_candidate != FALSE)
    {
        /* The native item owner consumes the typed MObj snapshot directly.
         * Falling through to ndsRendererAdapterPrepareMaterialSegment would
         * build a segment-E Gfx stream for a DObj this owner is about to draw
         * natively, which is the capability the native-only review forbids. */
    }
    else
#endif
#if NDS_RENDERER_HW_TRIANGLES
    if ((damage_slash_native_candidate != FALSE) ||
        (damage_fly_mdust_native_seen != FALSE))
    {
        /* The native owner consumes the typed live MObj snapshot directly at
         * the source segment-E position.  Building a Gfx material branch here
         * would reintroduce the generic N64 command path this owner removes. */
    }
    else
#endif
    if (visual_effect_native_candidate != FALSE)
    {
        /* No MObj exists on a template DObj and none is wanted: the owner's
         * combine, texture state and colours are all baked. Preparing a
         * segment-E material here would manufacture Gfx for a list nothing
         * executes. */
    }
    else
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_INISHIE
    if (inishie_pakkun_native_candidate != FALSE)
    {
        /* The native item owner consumes the typed MObj snapshot directly. */
    }
    else if (inishie_powblock_native_candidate != FALSE)
    {
        /* The POW block owner draws the list's own immutable material; the
         * DObj has no MObj, so preparing a segment-E stream here would
         * manufacture Gfx for a list nothing executes. */
    }
    else
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_NESS
    if (ness_pkfire_native_candidate != FALSE)
    {
        /* The owner already captured the two live LIGHT1|LIGHT2 materials as
         * typed snapshots.  Building a segment-E Gfx stream here would
         * reintroduce the generic command path and duplicate those updates. */
    }
    else
#endif
#if NDS_R2_IMPACT_WAVE_NATIVE
    if ((sNdsRendererAdapterImpactWaveNativeActive != FALSE) &&
        (dobj->mobj != NULL) &&
        (ndsRendererAdapterBuildNativeMaterial(
             dobj->mobj, &impact_wave_material) != FALSE))
    {
        impact_wave_native_candidate = TRUE;
    }
    else
#endif
    {
        ndsRendererAdapterPrepareMaterialSegment(dobj, &state);
    }
    NDS_LAB_SDL_MARK(5);
#if NDS_TICK_HUD && NDS_P2_EFFECT_CENSUS
    if (phase_effect != FALSE)
    {
        gNdsEffectPhaseMaterialTicks += cpuGetTiming() - phase_mark;
        phase_mark = cpuGetTiming();
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 1)
    NDS_DIAG(gNdsRendererProfileMaterialTicks += cpuGetTiming() - step_start);
    step_start = cpuGetTiming();
#endif
    ndsRendererAdapterPrepareInitialMatrices(dobj,
                                             (camera_gobj != NULL) ?
                                                 CObjGetStruct(camera_gobj) :
                                                 ((gGCCurrentCamera != NULL) ?
                                                      CObjGetStruct(
                                                          gGCCurrentCamera) :
                                                      NULL),
                                             TRUE,
                                             &initial_projection,
                                             &initial_projection_ptr,
                                             &initial_modelview,
                                             &initial_modelview_ptr);
#if NDS_TICK_HUD && NDS_P2_EFFECT_CENSUS
    if (phase_effect != FALSE)
    {
        gNdsEffectPhaseMatrixTicks += cpuGetTiming() - phase_mark;
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL >= 1)
    NDS_DIAG(gNdsRendererProfileMatrixTicks += cpuGetTiming() - step_start);
#endif

    NDS_LAB_SDL_MARK(1);
    config.max_depth = 8u;
    config.max_commands = 8192u;
    config.max_list_commands = 512u;
    config.initial_projection = initial_projection_ptr;
    config.initial_modelview = initial_modelview_ptr;
    config.initial_geometry_mode = 0u;
    config.initial_geometry_mode = initial_geometry_mode;
    config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    config.validate_range = ndsRendererAdapterStageValidateRange;
    config.immutable_command_span = ndsRendererAdapterImmutableCommandSpan;
    config.resolve_branch = ndsFighterDLDrawResolveBranch;
    config.resolve_data = ndsFighterDLDrawResolveRendererData;
    config.user = &state;
    callback = (ndsRendererHardwareNoOracleEnabled() != FALSE) ?
        NULL : ndsFighterMarioFoxVisitDLDrawCommand;
    callback_user = &state;

    render_stats = &stats;
#if NDS_RENDERER_HW_TRIANGLES && (NDS_RENDERER_PROFILE_LEVEL < 2)
    if (sNdsRendererAdapterStagePersistentActive != FALSE)
    {
        render_stats = &sNdsRendererAdapterStagePersistentStats;
        if (detailed_output != FALSE)
        {
            ndsFighterDLDrawResetTransientRendererStats(render_stats);
        }
        else
        {
            ndsFighterDLDrawResetRuntimeRendererStats(render_stats);
        }
    }
    else
    {
        ndsRendererInitStats(render_stats);
    }
#else
    ndsRendererInitStats(render_stats);
#if NDS_RENDERER_HW_TRIANGLES
    if (sNdsRendererAdapterStagePersistentActive != FALSE)
    {
        ndsFighterDLDrawCopyPersistentRendererState(
            render_stats, &sNdsRendererAdapterStagePersistentStats);
    }
#endif
#endif
#if NDS_RENDERER_HW_TRIANGLES
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererOwnerSnapshotStats(render_stats, &owner_stats_before);
    step_start = cpuGetTiming();
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererProfileSetSourceProvenance(
        sNdsRendererAdapterStageOwnerOccurrence,
        sNdsRendererAdapterStageListOrdinal,
        ndsRendererOwnerRootBranchPath(
            loaded, dl, sNdsRendererAdapterStageListOrdinal));
    sNdsRendererAdapterStageListOrdinal++;
#endif
#endif
    if (sNdsRendererAdapterEffectSubmitActive != FALSE)
    {
        /* Source order: the proc emits prim/env into the head stream and THEN
         * the model list, so these must land before the list executes and must
         * be allowed to be overridden by the list's own colour commands. */
        if ((sNdsRendererAdapterEffectColorMask & 1u) != 0u)
        {
            render_stats->prim_color = sNdsRendererAdapterEffectPrimColor;
        }
        if ((sNdsRendererAdapterEffectColorMask & 2u) != 0u)
        {
            render_stats->env_color = sNdsRendererAdapterEffectEnvColor;
        }
        /* THE ONLY WRITER OF THE CAPTURED BLEND STATE, and it is inside the
         * effect-submit guard. Without this the effect layer inherits whatever
         * othermode_l the previous list left, which is an opaque stage mode,
         * and ndsRendererHardwareAlpha takes its alpha-31 early return for
         * every effect polygon. */
        if (sNdsRendererAdapterEffectOtherModeValid != 0u)
        {
            render_stats->othermode_l = sNdsRendererAdapterEffectOtherModeL;
        }
        /* Latched HERE, not at the display-proc marker, because the marker
         * publishes the layer's sticky value and a probe stopping at one effect
         * would read another effect's mode. This is the mode THIS list starts
         * with; the Out latch below is what it finishes with. */
        gNdsEffectDLSubmitOtherModeIn = render_stats->othermode_l;
        effect_seed_before = render_stats->hardware_matrix_seed_count;
        effect_matrix_cmd_before = render_stats->matrix_command_count;
        effect_xform_before = render_stats->transformed_vertex_count;
        effect_hw_vertex_before = render_stats->hardware_vertex_count;
        effect_hw_triangle_before = render_stats->hardware_triangle_count;
    }
    if (sNdsRendererAdapterItemSubmitActive != FALSE)
    {
        u32 head = (sNdsRendererAdapterItemSubmitHead <
                    NDS_RENDERER_STAGE_DL_HEADS) ?
            sNdsRendererAdapterItemSubmitHead : 0u;

        gNdsItemRendererLastHead = head;
        gNdsItemRendererLastColorMask =
            sNdsRendererAdapterItemColorMask[head];
        gNdsItemRendererLastEnvColor =
            sNdsRendererAdapterItemEnvColor[head];
        gNdsItemRendererLastOtherModeL =
            sNdsRendererAdapterItemOtherModeL[head];
        gNdsItemRendererLastOtherModeH =
            sNdsRendererAdapterItemOtherModeH[head];

        if ((sNdsRendererAdapterItemColorMask[head] & 1u) != 0u)
        {
            render_stats->prim_color = sNdsRendererAdapterItemPrimColor[head];
        }
        if ((sNdsRendererAdapterItemColorMask[head] & 2u) != 0u)
        {
            render_stats->env_color = sNdsRendererAdapterItemEnvColor[head];
        }
        if (sNdsRendererAdapterItemOtherModeLValid[head] != 0u)
        {
            render_stats->othermode_l =
                sNdsRendererAdapterItemOtherModeL[head];
        }
        if (sNdsRendererAdapterItemOtherModeHValid[head] != 0u)
        {
            render_stats->othermode_h =
                sNdsRendererAdapterItemOtherModeH[head];
        }
    }
#if NDS_TICK_HUD && NDS_P2_EFFECT_CENSUS
    if (phase_effect != FALSE)
    {
        /* Armed OUTSIDE the Exec tick bracket so the capture's own setup is not
         * charged to Exec. The per-word recording inside it necessarily is,
         * which is why this build's Exec ticks are not a performance reading --
         * this run is about the stream's SHAPE, not its cost. */
        ndsEffectPacketCaptureBegin();
        phase_mark = cpuGetTiming();
    }
#endif
    NDS_LAB_SDL_MARK(2);
#if NDS_R2_REBIRTH_HALO_NATIVE
    if (rebirth_halo_native_candidate != FALSE)
    {
        rebirth_halo_native_handled = ndsRendererSubmitNativeRebirthHalo(
            rebirth_halo_root_offset, &config, render_stats);
        if (rebirth_halo_native_handled != FALSE)
        {
            NDS_DIAG(gNdsRebirthHaloNativeDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsRebirthHaloNativeFallbackCount++);
        }
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_INISHIE
    if (inishie_pakkun_native_candidate != FALSE)
    {
        /* Same split-camera contract the entry-effect owner documents at
         * :5142: the default battle camera legitimately supplies the whole
         * transform on one side of the DS pair, and a fixed owner has no
         * matrix stream to fill the other implicitly. Measured here: this
         * item arrives with a live modelview and a NULL projection, and the
         * submit declined all 600 draws of a 300-present run on exactly
         * that. Make the identity explicit on a copy rather than mutating
         * the shared config the other owners below still read. */
        NDSRendererConfig pakkun_config = config;
        NDSRendererMatrix20p12 pakkun_identity;

        if ((pakkun_config.initial_projection == NULL) &&
            (pakkun_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&pakkun_identity);
            pakkun_config.initial_projection = &pakkun_identity;
        }
        else if ((pakkun_config.initial_modelview == NULL) &&
                 (pakkun_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&pakkun_identity);
            pakkun_config.initial_modelview = &pakkun_identity;
        }
        inishie_pakkun_native_handled = ndsRendererSubmitNativeInishiePakkun(
            loaded->data, loaded->data_size,
            (const u8 *)inishie_pakkun_palette_file->data + 0x3620u,
            &inishie_pakkun_material, &pakkun_config, render_stats);
        if (inishie_pakkun_native_handled != FALSE)
        {
            NDS_DIAG(gNdsInishiePakkunDrawCount++);
#if (NDS_RENDERER_PROFILE_LEVEL < 2) && NDS_P2_ITEM_CORE
            ndsStageDLRouteRecord(dl, loaded, 0x0b40u,
                                  NDS_SDL_ROUTE_INISHIE_PAKKUN);
#endif
        }
        else
        {
            NDS_DIAG(gNdsInishiePakkunSubmitFailCount++);
        }
    }
    if (inishie_powblock_native_candidate != FALSE)
    {
        /* Same split-camera contract the Pakkun owner documents above: fill
         * the identity on a COPY, because the owners below still read the
         * shared config. */
        NDSRendererConfig powblock_config = config;
        NDSRendererMatrix20p12 powblock_identity;

        if ((powblock_config.initial_projection == NULL) &&
            (powblock_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&powblock_identity);
            powblock_config.initial_projection = &powblock_identity;
        }
        else if ((powblock_config.initial_modelview == NULL) &&
                 (powblock_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&powblock_identity);
            powblock_config.initial_modelview = &powblock_identity;
        }
        inishie_powblock_native_handled =
            ndsRendererSubmitNativeInishiePowblock(
                inishie_powblock_tlut, inishie_powblock_image_a,
                inishie_powblock_image_b, &powblock_config, render_stats);
        if (inishie_powblock_native_handled != FALSE)
        {
            NDS_DIAG(gNdsInishiePowblockDrawCount++);
#if (NDS_RENDERER_PROFILE_LEVEL < 2) && NDS_P2_ITEM_CORE
            ndsStageDLRouteRecord(dl, loaded,
                                  NDS_NATIVE_INISHIE_POWBLOCK_ROOT,
                                  NDS_SDL_ROUTE_INISHIE_POWBLOCK);
#endif
        }
        else
        {
            /* No fallback: a refusal falls through to the loud NO_PROGRAM
             * record below, never to a generic route. */
            NDS_DIAG(gNdsInishiePowblockSubmitFailCount++);
        }
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES
 #if NDS_P2_NESS
    if (ness_pkfire_native_candidate != FALSE)
    {
        NDSRendererConfig pkfire_config = config;
        NDSRendererMatrix20p12 pkfire_identity;

        if ((pkfire_config.initial_projection == NULL) &&
            (pkfire_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&pkfire_identity);
            pkfire_config.initial_projection = &pkfire_identity;
        }
        else if ((pkfire_config.initial_modelview == NULL) &&
                 (pkfire_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&pkfire_identity);
            pkfire_config.initial_modelview = &pkfire_identity;
        }
        ness_pkfire_native_handled = ndsRendererSubmitNativeNessPKFire(
            ness_pkfire_materials, NDS_NATIVE_NESS_PKFIRE_GROUP_COUNT,
            &pkfire_config, render_stats);
    }
    if (ness_pkthunder_native_candidate != FALSE)
    {
        NDSRendererConfig pkthunder_config = config;
        NDSRendererMatrix20p12 pkthunder_identity;

        if ((pkthunder_config.initial_projection == NULL) &&
            (pkthunder_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&pkthunder_identity);
            pkthunder_config.initial_projection = &pkthunder_identity;
        }
        else if ((pkthunder_config.initial_modelview == NULL) &&
                 (pkthunder_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&pkthunder_identity);
            pkthunder_config.initial_modelview = &pkthunder_identity;
        }
        ness_pkthunder_native_handled =
            ndsRendererSubmitNativeNessPKThunder(
                ness_pkthunder_root_index, &ness_pkthunder_material,
                ness_pkthunder_trail_color, &pkthunder_config, render_stats);
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if (ness_pkthunder_native_handled != FALSE)
        {
            ndsStageDLRouteRecord(dl, loaded,
                (ness_pkthunder_root_index ==
                 NDS_NATIVE_NESS_PKTHUNDER_HEAD_INDEX) ?
                    NDS_NATIVE_NESS_PKTHUNDER_HEAD_ROOT :
                    NDS_NATIVE_NESS_PKTHUNDER_TRAIL_ROOT,
                NDS_SDL_ROUTE_NESS_PKTHUNDER);
        }
#endif
    }
 #endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_PURIN
    if (purin_sing_native_candidate != FALSE)
    {
        NDSRendererConfig sing_config = config;
        NDSRendererMatrix20p12 identity;
        ndsRendererAdapterMtxIdentity20p12(&identity);
        if (sing_config.initial_projection == NULL) sing_config.initial_projection = &identity;
        if (sing_config.initial_modelview == NULL) sing_config.initial_modelview = &identity;
        purin_sing_native_handled = ndsRendererSubmitNativePurinSing(
            purin_sing_root_index, &purin_sing_material, purin_sing_image,
            &sing_config, render_stats);
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_PIKACHU
    if (loaded != NULL && (loaded->asset_id == NDS_NATIVE_PIKACHU_THUNDER_MODEL ||
                          loaded->asset_id == NDS_NATIVE_PIKACHU_THUNDER_SPECIAL))
    {
        pikachu_thunder_native_handled = ndsRendererAdapterPikachuThunder(
            loaded, dobj, dl, &config, render_stats);
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if (pikachu_thunder_native_handled != FALSE)
        {
            ndsStageDLRouteRecord(dl, loaded,
                                  ndsRelocNativeRootOffset(loaded, dl),
                                  NDS_SDL_ROUTE_PIKACHU_THUNDER);
        }
#endif
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_NESS
    /* The last PK Thunder trail segment is an EFFECT, not a weapon
     * (efManagerNessPKThunderTrailMakeEffect): its own list at 0x8F98 with a
     * fixed image and fixed prim/env, drawn through gcDrawDObjDLLinksForGObj. */
    if ((loaded != NULL) && (loaded->asset_id == NDS_NATIVE_NESS_PKTAIL_ASSET) &&
        (dobj->parent_gobj != NULL) && (dobj->parent_gobj->id == nGCCommonKindEffect) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_NESS_PKTAIL_ROOT))
    {
        NDSRendererConfig tail_config = config;
        NDSRendererMatrix20p12 identity;
#if NDS_P2_1P_GAME || NDS_P2_MENU_SHELL || NDS_P2_SHELL_ARGMAX_ROSTER || NDS_P2_COMPACT_BATTLE_FIGHTERS
        const void *tail_image = ndsRelocNativeAssetAddress(loaded->data, NDS_NATIVE_NESS_PKTAIL_IMAGE);
#else
        const void *tail_image = (loaded->data_size >= NDS_NATIVE_NESS_PKTAIL_IMAGE_END) ?
            (const void *)((u8 *)loaded->data + NDS_NATIVE_NESS_PKTAIL_IMAGE) : NULL;
#endif
        ndsRendererAdapterMtxIdentity20p12(&identity);
        if (tail_config.initial_projection == NULL) tail_config.initial_projection = &identity;
        if (tail_config.initial_modelview == NULL) tail_config.initial_modelview = &identity;
        ness_pktail_native_handled = ndsRendererSubmitNativeNessPKTail(
            tail_image, &tail_config, render_stats);
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_SAMUS
    /* Samus Bomb: one SamusModel billboard. The live MObj only swaps the
     * palette; texels, LOADTLUT and tile state are the source list's own. */
    if ((loaded != NULL) && (loaded->asset_id == NDS_NATIVE_SAMUS_BOMB_ASSET) &&
        (dobj->parent_gobj != NULL) && (dobj->parent_gobj->id == nGCCommonKindWeapon) &&
        (dobj->mobj != NULL) && (dobj->mobj->next == NULL) &&
        (ndsRelocNativeRootOffset(loaded, dl) == NDS_NATIVE_SAMUS_BOMB_ROOT))
    {
        NDSRendererNativeMaterial bomb_material;
        NDSRendererConfig bomb_config = config;
        NDSRendererMatrix20p12 identity;
#if NDS_P2_1P_GAME || NDS_P2_MENU_SHELL || NDS_P2_SHELL_ARGMAX_ROSTER || NDS_P2_COMPACT_BATTLE_FIGHTERS
        const void *bomb_image = ndsRelocNativeAssetAddress(loaded->data, NDS_NATIVE_SAMUS_BOMB_IMAGE);
#else
        const void *bomb_image = (loaded->data_size >= NDS_NATIVE_SAMUS_BOMB_IMAGE_END) ?
            (const void *)((u8 *)loaded->data + NDS_NATIVE_SAMUS_BOMB_IMAGE) : NULL;
#endif
        ndsRendererAdapterMtxIdentity20p12(&identity);
        if (bomb_config.initial_projection == NULL) bomb_config.initial_projection = &identity;
        if (bomb_config.initial_modelview == NULL) bomb_config.initial_modelview = &identity;
        if (ndsRendererAdapterBuildNativeMaterialSnapshot(dobj->mobj, &bomb_material, FALSE, NULL, NULL) != FALSE)
            samus_bomb_native_handled = ndsRendererSubmitNativeSamusBomb(
                &bomb_material, bomb_image, &bomb_config, render_stats);
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_KIRBY
    if ((loaded != NULL) && (loaded->asset_id == NDS_NATIVE_KIRBY_VULCAN_ASSET) &&
        (dobj->parent_gobj != NULL) && (dobj->parent_gobj->id == nGCCommonKindEffect) &&
        (dobj->mobj == NULL))
    {
        u32 root = ndsRelocNativeRootOffset(loaded, dl);
        if (root == NDS_NATIVE_KIRBY_VULCAN_ROOT0 || root == NDS_NATIVE_KIRBY_VULCAN_ROOT1)
        {
            NDSRendererConfig vulcan_config = config;
            NDSRendererMatrix20p12 identity;
            ndsRendererAdapterMtxIdentity20p12(&identity);
            if (vulcan_config.initial_projection == NULL) vulcan_config.initial_projection = &identity;
            if (vulcan_config.initial_modelview == NULL) vulcan_config.initial_modelview = &identity;
            kirby_vulcan_native_handled = ndsRendererSubmitNativeKirbyVulcan(
                (root == NDS_NATIVE_KIRBY_VULCAN_ROOT0) ? 0u : 1u, &vulcan_config, render_stats);
        }
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_YOSHI
    if (yoshi_entryegg_native_candidate != FALSE)
    {
        NDSRendererConfig entryegg_config = config;
        NDSRendererMatrix20p12 entryegg_identity;

        if ((entryegg_config.initial_projection == NULL) &&
            (entryegg_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&entryegg_identity);
            entryegg_config.initial_projection = &entryegg_identity;
        }
        else if ((entryegg_config.initial_modelview == NULL) &&
                 (entryegg_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&entryegg_identity);
            entryegg_config.initial_modelview = &entryegg_identity;
        }
        yoshi_entryegg_native_handled = ndsRendererSubmitNativeYoshiEntryEgg(
            yoshi_entryegg_palette, &yoshi_entryegg_material,
            &entryegg_config, render_stats);
    }
    if (yoshi_egg_native_candidate != FALSE)
    {
        NDSRendererConfig egg_config = config;
        NDSRendererMatrix20p12 egg_identity;

        if ((egg_config.initial_projection == NULL) &&
            (egg_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&egg_identity);
            egg_config.initial_projection = &egg_identity;
        }
        else if ((egg_config.initial_modelview == NULL) &&
                 (egg_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&egg_identity);
            egg_config.initial_modelview = &egg_identity;
        }
        if (yoshi_egg_is_weapon != FALSE)
        {
            /* wpYoshiEggThrowProcDisplay writes EnvColor(0,0,0,0) immediately
             * before wpDisplayDLHead1. Effects arrive with their callback ENV
             * already captured into render_stats. */
            render_stats->env_color = 0u;
        }
        yoshi_egg_native_handled = ndsRendererSubmitNativeYoshiEgg(
            &egg_config, render_stats);
    }
    if (yoshi_egglay_native_candidate != FALSE)
    {
        NDSRendererConfig egglay_config = config;
        NDSRendererMatrix20p12 egglay_identity;

        if ((egglay_config.initial_projection == NULL) &&
            (egglay_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&egglay_identity);
            egglay_config.initial_projection = &egglay_identity;
        }
        else if ((egglay_config.initial_modelview == NULL) &&
                 (egglay_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&egglay_identity);
            egglay_config.initial_modelview = &egglay_identity;
        }
        yoshi_egglay_native_handled = ndsRendererSubmitNativeYoshiEggLay(
            yoshi_egglay_palette, yoshi_egglay_image,
            &egglay_config, render_stats);
    }
#endif
    if (charge_shot_native_candidate != FALSE)
    {
        NDSRendererConfig charge_shot_config = config;
        NDSRendererMatrix20p12 charge_shot_identity;

        if ((charge_shot_config.initial_projection == NULL) &&
            (charge_shot_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&charge_shot_identity);
            charge_shot_config.initial_projection = &charge_shot_identity;
        }
        else if ((charge_shot_config.initial_modelview == NULL) &&
                 (charge_shot_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&charge_shot_identity);
            charge_shot_config.initial_modelview = &charge_shot_identity;
        }
        charge_shot_native_handled = ndsRendererSubmitNativeSamusChargeShot(
            loaded->data, loaded->data_size, &charge_shot_config,
            render_stats);
        if (charge_shot_native_handled != FALSE)
        {
            NDS_DIAG(gNdsChargeShotDrawCount++);
#if NDS_RENDERER_PROFILE_LEVEL < 2
            ndsStageDLRouteRecord(dl, loaded, NDS_NATIVE_CHARGESHOT_ROOT,
                                  NDS_SDL_ROUTE_CHARGE_SHOT);
#endif
        }
        else
        {
            NDS_DIAG(gNdsChargeShotSubmitFailCount++);
        }
    }
    if (thunder_jolt_native_candidate != FALSE)
    {
        /* Same split-camera contract every fixed owner documents: fill the
         * identity on a COPY, because later code still reads the shared
         * config. */
        NDSRendererConfig jolt_config = config;
        NDSRendererMatrix20p12 jolt_identity;

        if ((jolt_config.initial_projection == NULL) &&
            (jolt_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&jolt_identity);
            jolt_config.initial_projection = &jolt_identity;
        }
        else if ((jolt_config.initial_modelview == NULL) &&
                 (jolt_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&jolt_identity);
            jolt_config.initial_modelview = &jolt_identity;
        }
        thunder_jolt_native_handled = ndsRendererSubmitNativePikachuThunderJolt(
            loaded->data, loaded->data_size, &jolt_config, render_stats);
        if (thunder_jolt_native_handled != FALSE)
        {
            NDS_DIAG(gNdsThunderJoltDrawCount++);
        }
        else
        {
            /* No fallback: a refusal falls through to the loud NO_PROGRAM
             * record below, never to a generic route. */
            NDS_DIAG(gNdsThunderJoltSubmitFailCount++);
        }
    }
    if (thunder_ground_native_candidate != FALSE)
    {
        /* Same split-camera contract every fixed owner documents: fill the
         * identity on a COPY, because later code reads the shared config. */
        NDSRendererConfig ground_config = config;
        NDSRendererMatrix20p12 ground_identity;

        if ((ground_config.initial_projection == NULL) &&
            (ground_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&ground_identity);
            ground_config.initial_projection = &ground_identity;
        }
        else if ((ground_config.initial_modelview == NULL) &&
                 (ground_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&ground_identity);
            ground_config.initial_modelview = &ground_identity;
        }
        thunder_ground_native_handled =
            ndsRendererSubmitNativePikachuThunderGround(
                thunder_ground_root_index, &thunder_ground_material,
                &ground_config, render_stats);
        if (thunder_ground_native_handled != FALSE)
        {
            NDS_DIAG(gNdsThunderGroundDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsThunderGroundSubmitFailCount++);
        }
    }
    if (thunder_fx_native_candidate != FALSE)
    {
        /* Same split-camera contract every fixed owner documents: fill the
         * identity on a COPY, because later code reads the shared config. */
        NDSRendererConfig fx_config = config;
        NDSRendererMatrix20p12 fx_identity;

        if ((fx_config.initial_projection == NULL) &&
            (fx_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&fx_identity);
            fx_config.initial_projection = &fx_identity;
        }
        else if ((fx_config.initial_modelview == NULL) &&
                 (fx_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&fx_identity);
            fx_config.initial_modelview = &fx_identity;
        }
        thunder_fx_native_handled =
            ndsRendererSubmitNativePikachuThunderJoltEffect(
                thunder_fx_base, thunder_fx_bytes, &thunder_fx_material,
                &fx_config, render_stats);
        if (thunder_fx_native_handled != FALSE)
        {
            NDS_DIAG(gNdsThunderJoltFxDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsThunderJoltFxSubmitFailCount++);
        }
    }
    if (damage_fly_mdust_native_seen != FALSE)
    {
        if (ndsRendererAdapterSubmitDamageFlyMDust(
                dobj, loaded, &config, render_stats) == FALSE)
        {
            ndsStageRejectNativeRender(dobj, dl,
                NDS_NATIVE_FAILURE_REJECTED_PROGRAM, render_stats);
        }
#if NDS_RENDERER_PROFILE_LEVEL < 2
        else
        {
            ndsStageDLRouteRecord(dl, loaded, NDS_NATIVE_DAMAGE_FLY_MDUST_ROOT,
                                  NDS_SDL_ROUTE_DAMAGE_FLY_MDUST);
        }
#endif
        damage_fly_mdust_native_settled = TRUE;
    }
    if (damage_slash_native_seen != FALSE)
    {
        if (damage_slash_native_candidate != FALSE)
        {
            NDSRendererConfig slash_config = config;
            NDSRendererMatrix20p12 slash_identity;

            if ((slash_config.initial_projection == NULL) &&
                (slash_config.initial_modelview != NULL))
            {
                ndsRendererAdapterMtxIdentity20p12(&slash_identity);
                slash_config.initial_projection = &slash_identity;
            }
            else if ((slash_config.initial_modelview == NULL) &&
                     (slash_config.initial_projection != NULL))
            {
                ndsRendererAdapterMtxIdentity20p12(&slash_identity);
                slash_config.initial_modelview = &slash_identity;
            }
            damage_slash_native_handled = ndsRendererSubmitNativeDamageSlash(
                damage_slash_base, damage_slash_bytes, damage_slash_root,
                &damage_slash_material, &slash_config, render_stats);
        }
        if (damage_slash_native_handled != FALSE)
        {
            NDS_DIAG(gNdsDamageSlashDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsDamageSlashSubmitFailCount++);
            ndsStageRejectNativeRender(dobj, dl,
                NDS_NATIVE_FAILURE_REJECTED_PROGRAM, render_stats);
        }
        /* Whether it drew or published its precise rejection, this source root
         * has been resolved by its owner.  Suppress the generic NO_PROGRAM
         * guard below so one event cannot record two contradictory reasons. */
        damage_slash_native_settled = TRUE;
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_CASTLE
    if (castle_bumper_native_candidate != FALSE)
    {
        /* Same split-camera contract the other fixed owners document: fill
         * the identity on a COPY, because the owners below still read the
         * shared config. */
        NDSRendererConfig castle_bumper_config = config;
        NDSRendererMatrix20p12 castle_bumper_identity;

        if ((castle_bumper_config.initial_projection == NULL) &&
            (castle_bumper_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&castle_bumper_identity);
            castle_bumper_config.initial_projection = &castle_bumper_identity;
        }
        else if ((castle_bumper_config.initial_modelview == NULL) &&
                 (castle_bumper_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&castle_bumper_identity);
            castle_bumper_config.initial_modelview = &castle_bumper_identity;
        }
        castle_bumper_native_handled = ndsRendererSubmitNativeCastleBumper(
            loaded->data, loaded->data_size,
            &castle_bumper_material, &castle_bumper_config, render_stats);
        if (castle_bumper_native_handled != FALSE)
        {
            NDS_DIAG(gNdsCastleBumperDrawCount++);
#if (NDS_RENDERER_PROFILE_LEVEL < 2)
            if ((sNdsRendererAdapterEffectSubmitActive == FALSE) &&
                (sNdsRendererAdapterStagePersistentActive != FALSE))
            {
                ndsStageDLRouteRecord(dl, loaded, 0u,
                                      NDS_SDL_ROUTE_CASTLE_BUMPER);
            }
#endif
        }
        else
        {
            NDS_DIAG(gNdsCastleBumperSubmitFailCount++);
        }
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_SECTOR
    if (sector_laser_native_candidate != FALSE)
    {
        /* Same split-camera contract the Pakkun owner documents above: the
         * battle camera can supply the whole transform on one side of the DS
         * pair, and a fixed owner has no matrix stream to fill the other
         * implicitly.  Fill the identity on a COPY -- the impact-wave submit
         * below and the effect witnesses at the end of this function still
         * read the shared config, so mutating it here would corrupt both. */
        NDSRendererConfig laser_config = config;
        NDSRendererMatrix20p12 laser_identity;

        if ((laser_config.initial_projection == NULL) &&
            (laser_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&laser_identity);
            laser_config.initial_projection = &laser_identity;
        }
        else if ((laser_config.initial_modelview == NULL) &&
                 (laser_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&laser_identity);
            laser_config.initial_modelview = &laser_identity;
        }
        sector_laser_native_handled =
            ndsRendererSubmitNativeSectorArwingLaser(
                sector_laser_tlut, sector_laser_image, &laser_config,
                render_stats);
        if (sector_laser_native_handled != FALSE)
        {
            NDS_DIAG(gNdsSectorLaserDrawCount++);
#if NDS_RENDERER_PROFILE_LEVEL < 2
            ndsStageDLRouteRecord(dl, loaded, NDS_NATIVE_SECTOR_LASER_ROOT,
                                  NDS_SDL_ROUTE_SECTOR_LASER);
#endif
        }
        else
        {
            /* No fallback: a refusal falls through to the loud NO_PROGRAM
             * record below, never to a generic route. */
            NDS_DIAG(gNdsSectorLaserSubmitFailCount++);
        }
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_LINK
    if (link_bomb_native_candidate != FALSE)
    {
        /* Same split-camera contract the other fixed owners document: the
         * battle camera can supply the whole transform on one side of the DS
         * pair, and a fixed owner has no matrix stream to fill the other
         * implicitly.  Fill the identity on a COPY -- the impact-wave submit
         * below and the effect witnesses at the end of this function still
         * read the shared config, so mutating it here would corrupt both. */
        NDSRendererConfig link_bomb_config = config;
        NDSRendererMatrix20p12 link_bomb_identity;

        if ((link_bomb_config.initial_projection == NULL) &&
            (link_bomb_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&link_bomb_identity);
            link_bomb_config.initial_projection = &link_bomb_identity;
        }
        else if ((link_bomb_config.initial_modelview == NULL) &&
                 (link_bomb_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&link_bomb_identity);
            link_bomb_config.initial_modelview = &link_bomb_identity;
        }
        /* env_color is ALREADY the item layer's live ColAnim colour by this
         * point (seeded above from sNdsRendererAdapterItemEnvColor[head]) and
         * the body combiner's cycle 1 consumes it.  Do not reseed it here. */
        link_bomb_native_handled = ndsRendererSubmitNativeLinkBomb(
            link_bomb_root, loaded->data, loaded->data_size,
            &link_bomb_config, render_stats);
        if (link_bomb_native_handled != FALSE)
        {
            NDS_DIAG(gNdsLinkBombDrawCount++);
        }
        else
        {
            /* No fallback: a refusal falls through to the loud NO_PROGRAM
             * record below, never to a generic route. */
            NDS_DIAG(gNdsLinkBombSubmitFailCount++);
        }
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_YAMABUKI
    if (marumine_native_candidate != FALSE)
    {
        /* Same split-camera contract the other fixed owners document: fill the
         * identity on a COPY, because later code in this function still reads
         * the shared config. */
        NDSRendererConfig marumine_config = config;
        NDSRendererMatrix20p12 marumine_identity;

        if ((marumine_config.initial_projection == NULL) &&
            (marumine_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&marumine_identity);
            marumine_config.initial_projection = &marumine_identity;
        }
        else if ((marumine_config.initial_modelview == NULL) &&
                 (marumine_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&marumine_identity);
            marumine_config.initial_modelview = &marumine_identity;
        }
        /* The combiner reads TEXEL0 and SHADE only, so the item layer's seeded
         * prim/env in render_stats cannot reach this draw.  Do not reseed. */
        marumine_native_handled = ndsRendererSubmitNativeYamabukiMarumine(
            loaded->data, loaded->data_size, &marumine_config, render_stats);
        if (marumine_native_handled != FALSE)
        {
            NDS_DIAG(gNdsYamabukiMarumineDrawCount++);
#if NDS_RENDERER_PROFILE_LEVEL < 2
            ndsStageDLRouteRecord(dl, loaded, 0u,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemMarumine);
#endif
        }
        else
        {
            /* No fallback: a refusal falls through to the loud NO_PROGRAM
             * record below, never to a generic route. */
            NDS_DIAG(gNdsYamabukiMarumineSubmitFailCount++);
        }
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_YAMABUKI && NDS_P2_ITEM_CORE
    if (glucky_native_candidate != FALSE)
    {
        NDSRendererConfig glucky_config = config;
        NDSRendererMatrix20p12 glucky_identity;

        if ((glucky_config.initial_projection == NULL) &&
            (glucky_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&glucky_identity);
            glucky_config.initial_projection = &glucky_identity;
        }
        else if ((glucky_config.initial_modelview == NULL) &&
                 (glucky_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&glucky_identity);
            glucky_config.initial_modelview = &glucky_identity;
        }
        glucky_native_handled = ndsRendererSubmitNativeItemGLucky(
            loaded->data, loaded->data_size, &glucky_config, render_stats);
        if (glucky_native_handled != FALSE)
        {
            NDS_DIAG(gNdsYamabukiGluckyDrawCount++);
#if NDS_RENDERER_PROFILE_LEVEL < 2
            ndsStageDLRouteRecord(dl, loaded, 0u,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemGLucky);
#endif
        }
        else
        {
            NDS_DIAG(gNdsYamabukiGluckySubmitFailCount++);
        }
    }

    if (porygon_native_candidate != FALSE)
    {
        NDSRendererConfig porygon_config = config;
        NDSRendererMatrix20p12 porygon_identity;

        if ((porygon_config.initial_projection == NULL) &&
            (porygon_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&porygon_identity);
            porygon_config.initial_projection = &porygon_identity;
        }
        else if ((porygon_config.initial_modelview == NULL) &&
                 (porygon_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&porygon_identity);
            porygon_config.initial_modelview = &porygon_identity;
        }
        porygon_native_handled = ndsRendererSubmitNativeItemPorygon(
            loaded->data, loaded->data_size, &porygon_config, render_stats);
        if (porygon_native_handled != FALSE)
        {
            NDS_DIAG(gNdsYamabukiPorygonDrawCount++);
#if NDS_RENDERER_PROFILE_LEVEL < 2
            ndsStageDLRouteRecord(dl, loaded, 0u,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemPorygon);
#endif
        }
        else
        {
            NDS_DIAG(gNdsYamabukiPorygonSubmitFailCount++);
        }
    }

    if (hitokage_native_candidate != FALSE)
    {
        NDSRendererConfig hitokage_config = config;
        NDSRendererMatrix20p12 hitokage_identity;

        if ((hitokage_config.initial_projection == NULL) &&
            (hitokage_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&hitokage_identity);
            hitokage_config.initial_projection = &hitokage_identity;
        }
        else if ((hitokage_config.initial_modelview == NULL) &&
                 (hitokage_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&hitokage_identity);
            hitokage_config.initial_modelview = &hitokage_identity;
        }
        hitokage_native_handled = ndsRendererSubmitNativeItemHitokage(
            loaded->data, loaded->data_size, &hitokage_material,
            &hitokage_config, render_stats);
        if (hitokage_native_handled != FALSE)
        {
            NDS_DIAG(gNdsYamabukiHitokageDrawCount++);
#if NDS_RENDERER_PROFILE_LEVEL < 2
            ndsStageDLRouteRecord(dl, loaded, NDS_NATIVE_ITEM_HITOKAGE_ROOT,
                                  NDS_SDL_ROUTE_HITOKAGE);
#endif
        }
        else
        {
            NDS_DIAG(gNdsYamabukiHitokageSubmitFailCount++);
        }
    }

    if (fushigibana_native_candidate != FALSE)
    {
        NDSRendererConfig fushigibana_config = config;
        NDSRendererMatrix20p12 fushigibana_identity;

        if ((fushigibana_config.initial_projection == NULL) &&
            (fushigibana_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&fushigibana_identity);
            fushigibana_config.initial_projection = &fushigibana_identity;
        }
        else if ((fushigibana_config.initial_modelview == NULL) &&
                 (fushigibana_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&fushigibana_identity);
            fushigibana_config.initial_modelview = &fushigibana_identity;
        }
        fushigibana_native_handled = ndsRendererSubmitNativeItemFushigibana(
            loaded->data, loaded->data_size, &fushigibana_material,
            &fushigibana_config, render_stats);
        if (fushigibana_native_handled != FALSE)
        {
            NDS_DIAG(gNdsYamabukiFushigibanaDrawCount++);
#if NDS_RENDERER_PROFILE_LEVEL < 2
            ndsStageDLRouteRecord(dl, loaded, NDS_NATIVE_ITEM_FUSHIGIBANA_ROOT,
                                  NDS_SDL_ROUTE_FUSHIGIBANA);
#endif
        }
        else
        {
            NDS_DIAG(gNdsYamabukiFushigibanaSubmitFailCount++);
        }
    }
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE
    if (item_star_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_star_native_handled = ndsRendererSubmitNativeItemStar(
            loaded->data, loaded->data_size,
            &item_star_material0, &item_star_material1,
            &item_config, render_stats);
        if (item_star_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemStarDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsItemStarSubmitFailCount++);
        }
    }

    if (item_sword_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_sword_native_handled = ndsRendererSubmitNativeItemSword(
            item_sword_root, loaded->data, loaded->data_size,
            &item_config, render_stats);
        if (item_sword_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemSwordDrawCount++);
#if NDS_RENDERER_PROFILE_LEVEL < 2
            ndsStageDLRouteRecord(dl, loaded, item_sword_root,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemSword);
#endif
        }
        else
        {
            NDS_DIAG(gNdsItemSwordSubmitFailCount++);
        }
    }

    if (item_hammer_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_hammer_native_handled = ndsRendererSubmitNativeItemHammer(
            loaded->data, loaded->data_size, &item_config, render_stats);
        if (item_hammer_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemHammerDrawCount++);
#if NDS_RENDERER_PROFILE_LEVEL < 2
            ndsStageDLRouteRecord(dl, loaded, 0u,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemHammer);
#endif
        }
        else
        {
            NDS_DIAG(gNdsItemHammerSubmitFailCount++);
        }
    }

    if (item_mball_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;
        const NDSRendererNativeMaterial *material =
            (item_mball_root == NDS_NATIVE_ITEM_MBALL_LIVE_ROOT) ?
                &item_mball_material : NULL;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_mball_native_handled = ndsRendererSubmitNativeItemMBall(
            item_mball_root, loaded->data, loaded->data_size,
            material, &item_config, render_stats);
        if (item_mball_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemMBallDrawCount++);
            if (item_mball_from_effect != FALSE)
            {
                NDS_DIAG(gNdsEntryMBallThrownDrawCount++);
            }
        }
        else
        {
            NDS_DIAG(gNdsItemMBallSubmitFailCount++);
            if (item_mball_from_effect != FALSE)
            {
                NDS_DIAG(gNdsEntryMBallThrownSubmitFailCount++);
            }
        }
    }

    if (item_kirbystar_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_kirbystar_native_handled = ndsRendererSubmitNativeItemKirbyStar(
            loaded->data, loaded->data_size, NULL, &item_config,
            render_stats);
        if (item_kirbystar_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemKirbyStarDrawCount++);
#if NDS_RENDERER_PROFILE_LEVEL < 2
            ndsStageDLRouteRecord(dl, loaded, NDS_NATIVE_ITEM_KIRBYSTAR_ROOT,
                                  NDS_SDL_ROUTE_KIRBYSTAR);
#endif
        }
        else
        {
            NDS_DIAG(gNdsItemKirbyStarSubmitFailCount++);
        }
    }

    if (item_gshell_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_gshell_native_handled = ndsRendererSubmitNativeItemGShell(
            loaded->data, loaded->data_size, &item_gshell_material,
            &item_config, render_stats);
        if (item_gshell_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemGShellDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsItemGShellSubmitFailCount++);
        }
    }

    if (item_rshell_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_rshell_native_handled = ndsRendererSubmitNativeItemRShell(
            loaded->data, loaded->data_size, &item_rshell_material,
            &item_config, render_stats);
        if (item_rshell_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemRShellDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsItemRShellSubmitFailCount++);
        }
    }

    if (item_bat_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_bat_native_handled = ndsRendererSubmitNativeItemBat(
            item_bat_root, loaded->data, loaded->data_size,
            &item_config, render_stats);
        if (item_bat_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemBatDrawCount++);
#if NDS_RENDERER_PROFILE_LEVEL < 2
            ndsStageDLRouteRecord(dl, loaded, item_bat_root,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemBat);
#endif
        }
        else
        {
            NDS_DIAG(gNdsItemBatSubmitFailCount++);
        }
    }

    if (item_capsule_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_capsule_native_handled = ndsRendererSubmitNativeItemCapsule(
            item_capsule_root, loaded->data, loaded->data_size,
            &item_config, render_stats);
        if (item_capsule_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemCapsuleDrawCount++);
#if NDS_RENDERER_PROFILE_LEVEL < 2
            ndsStageDLRouteRecord(dl, loaded, item_capsule_root,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemCapsule);
#endif
        }
        else
        {
            NDS_DIAG(gNdsItemCapsuleSubmitFailCount++);
        }
    }

    if (item_bombhei_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_bombhei_native_handled = ndsRendererSubmitNativeItemBombHei(
            loaded->data, loaded->data_size, &item_bombhei_material,
            &item_config, render_stats);
        if (item_bombhei_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemBombHeiDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsItemBombHeiSubmitFailCount++);
        }
    }

    if (item_lgun_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_lgun_native_handled = ndsRendererSubmitNativeItemLGun(
            loaded->data, loaded->data_size, &item_config, render_stats);
        if (item_lgun_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemLGunDrawCount++);
#if NDS_RENDERER_PROFILE_LEVEL < 2
            ndsStageDLRouteRecord(dl, loaded, 0u,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemLGun);
#endif
        }
        else
        {
            NDS_DIAG(gNdsItemLGunSubmitFailCount++);
        }
    }

    if (item_harisen_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_harisen_native_handled = ndsRendererSubmitNativeItemHarisen(
            loaded->data, loaded->data_size, &item_config, render_stats);
        if (item_harisen_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemHarisenDrawCount++);
#if NDS_RENDERER_PROFILE_LEVEL < 2
            ndsStageDLRouteRecord(dl, loaded, 0u,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemHarisen);
#endif
        }
        else
        {
            NDS_DIAG(gNdsItemHarisenSubmitFailCount++);
        }
    }

    if (item_heart_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_heart_native_handled = ndsRendererSubmitNativeItemHeart(
            loaded->data, loaded->data_size, &item_config, render_stats);
        if (item_heart_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemHeartDrawCount++);
#if NDS_RENDERER_PROFILE_LEVEL < 2
            ndsStageDLRouteRecord(dl, loaded, 0u,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemHeart);
#endif
        }
        else
        {
            NDS_DIAG(gNdsItemHeartSubmitFailCount++);
        }
    }

    if (item_starrod_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_starrod_native_handled = ndsRendererSubmitNativeItemStarRod(
            item_starrod_root, loaded->data, loaded->data_size,
            &item_config, render_stats);
        if (item_starrod_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemStarRodDrawCount++);
#if NDS_RENDERER_PROFILE_LEVEL < 2
            ndsStageDLRouteRecord(dl, loaded, item_starrod_root,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemStarRod);
#endif
        }
        else
        {
            NDS_DIAG(gNdsItemStarRodSubmitFailCount++);
        }
    }

    if (item_fflower_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;
        const NDSRendererNativeMaterial *material =
            (item_fflower_root == NDS_NATIVE_ITEM_FFLOWER_LIVE_ROOT) ?
                &item_fflower_material : NULL;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_fflower_native_handled = ndsRendererSubmitNativeItemFFlower(
            item_fflower_root, loaded->data, loaded->data_size, material,
            &item_config, render_stats);
        if (item_fflower_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemFFlowerDrawCount++);
#if NDS_RENDERER_PROFILE_LEVEL < 2
            ndsStageDLRouteRecord(dl, loaded, item_fflower_root,
                (item_fflower_root == NDS_NATIVE_ITEM_FFLOWER_LIVE_ROOT) ?
                    NDS_SDL_ROUTE_FFLOWER_LIVE :
                    (NDS_SDL_ROUTE_ITEM + nNDSStageDLItemFFlower));
#endif
        }
        else
        {
            NDS_DIAG(gNdsItemFFlowerSubmitFailCount++);
        }
    }

    if (item_msbomb_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_msbomb_native_handled = ndsRendererSubmitNativeItemMSBomb(
            item_msbomb_root, loaded->data, loaded->data_size,
            &item_config, render_stats);
        if (item_msbomb_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemMSBombDrawCount++);
#if NDS_RENDERER_PROFILE_LEVEL < 2
            ndsStageDLRouteRecord(dl, loaded, item_msbomb_root,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemMSBomb);
#endif
        }
        else
        {
            NDS_DIAG(gNdsItemMSBombSubmitFailCount++);
        }
    }

    if (item_nbumper_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_nbumper_native_handled = ndsRendererSubmitNativeItemNBumper(
            loaded->data, loaded->data_size, &item_nbumper_material,
            &item_config, render_stats);
        if (item_nbumper_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemNBumperDrawCount++);
#if NDS_RENDERER_PROFILE_LEVEL < 2
            ndsStageDLRouteRecord(dl, loaded,
                                  NDS_NATIVE_ITEM_NBUMPER_ROOT,
                                  NDS_SDL_ROUTE_NBUMPER);
#endif
        }
        else
        {
            NDS_DIAG(gNdsItemNBumperSubmitFailCount++);
        }
    }

    if (item_box_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_box_native_handled = ndsRendererSubmitNativeItemBox(
            loaded->data, loaded->data_size, &item_config, render_stats);
        if (item_box_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemBoxDrawCount++);
#if NDS_RENDERER_PROFILE_LEVEL < 2
            ndsStageDLRouteRecord(dl, loaded, 0u,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemBox);
#endif
        }
        else
        {
            NDS_DIAG(gNdsItemBoxSubmitFailCount++);
        }
    }

    if (item_taru_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_taru_native_handled = ndsRendererSubmitNativeItemTaru(
            loaded->data, loaded->data_size, &item_config, render_stats);
        if (item_taru_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemTaruDrawCount++);
#if NDS_RENDERER_PROFILE_LEVEL < 2
            ndsStageDLRouteRecord(dl, loaded, 0u,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemTaru);
#endif
        }
        else
        {
            NDS_DIAG(gNdsItemTaruSubmitFailCount++);
        }
    }

    if (item_egg_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_egg_native_handled = ndsRendererSubmitNativeItemEgg(
            loaded->data, loaded->data_size, &item_config, render_stats);
        if (item_egg_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemEggDrawCount++);
#if NDS_RENDERER_PROFILE_LEVEL < 2
            ndsStageDLRouteRecord(dl, loaded, 0u,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemEgg);
#endif
        }
        else
        {
            NDS_DIAG(gNdsItemEggSubmitFailCount++);
        }
    }

    if (item_iwark_native_candidate != FALSE)
    {
        NDSRendererConfig item_config = config;
        NDSRendererMatrix20p12 identity;

        if ((item_config.initial_projection == NULL) &&
            (item_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_projection = &identity;
        }
        else if ((item_config.initial_modelview == NULL) &&
                 (item_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&identity);
            item_config.initial_modelview = &identity;
        }
        item_iwark_native_handled = ndsRendererSubmitNativeItemIwark(
            loaded->data, loaded->data_size, &item_config, render_stats);
        if (item_iwark_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemIwarkDrawCount++);
#if NDS_RENDERER_PROFILE_LEVEL < 2
            ndsStageDLRouteRecord(dl, loaded, 0u,
                                  NDS_SDL_ROUTE_ITEM + nNDSStageDLItemIwark);
#endif
        }
        else
        {
            NDS_DIAG(gNdsItemIwarkSubmitFailCount++);
        }
    }

    /* 2026-09-30: the baked roots (generate_nds_native_item_baked.py) -- the
     * roots a full-match lab census found drawn with no owner: PK Fire's
     * pillar, Bob-omb walking left, the placed Bumper, the Ray Gun's shot,
     * Razor Leaf -- and the Poke Ball Pokemon and their weapons, which it
     * could not reach. One lookup names the root for its GObj kind; its live
     * segment-E materials are the DObj's MObjs in order. */
    if ((loaded != NULL) && (dobj != NULL) && (dobj->parent_gobj != NULL) &&
        (NDS_NATIVE_BAKED_ASSET_MATCH(loaded->asset_id)
#if NDS_P2_1P_GAME
         /* The ending's room (the table it loads into its own heap). */
         || NDS_NATIVE_BAKED_ROOM_ASSET_MATCH(loaded->asset_id)
#endif
        ))
    {
        u32 baked_slots = 0u;
        const void *baked = ndsNativeBakedItemFind(
            loaded->asset_id, ndsRelocNativeRootOffset(loaded, dl),
            (u32)dobj->parent_gobj->id, dl, &baked_slots);

        if (baked != NULL)
        {
            NDSRendererNativeMaterial baked_materials[
                NDS_NATIVE_BAKED_MATERIAL_SLOTS];
            NDSRendererConfig item_config = config;
            NDSRendererMatrix20p12 identity;
            MObj *mobj = dobj->mobj;
            u32 i;

            for (i = 0u; i < baked_slots; i++)
            {
                if ((mobj == NULL) ||
                    (ndsRendererAdapterBuildNativeMaterialSnapshot(
                         mobj, &baked_materials[i], FALSE,
                         NULL, NULL) == FALSE))
                {
                    break;
                }
                mobj = mobj->next;
            }
            if ((item_config.initial_projection == NULL) &&
                (item_config.initial_modelview != NULL))
            {
                ndsRendererAdapterMtxIdentity20p12(&identity);
                item_config.initial_projection = &identity;
            }
            else if ((item_config.initial_modelview == NULL) &&
                     (item_config.initial_projection != NULL))
            {
                ndsRendererAdapterMtxIdentity20p12(&identity);
                item_config.initial_modelview = &identity;
            }
            item_baked_native_handled = (i == baked_slots) ?
                ndsRendererSubmitNativeBaked(
                    baked, loaded->data, loaded->data_size,
                    baked_materials, i, &item_config, render_stats) :
                FALSE;
            if (item_baked_native_handled != FALSE)
            {
                NDS_DIAG(gNdsItemBakedDrawCount++);
#if NDS_RENDERER_PROFILE_LEVEL < 2
                /* A ground display's baked root draws through the fast lane
                 * from now on (NDS_SDL_ROUTE_BAKED); its admission is the
                 * GObj kind and the same submit context, checked per draw. */
                if ((dobj->parent_gobj->id == nGCCommonKindGroundDisplay) &&
                    (sNdsRendererAdapterItemSubmitActive == FALSE) &&
                    (sNdsRendererAdapterEffectSubmitActive == FALSE))
                {
                    ndsStageDLRouteRecord(dl, loaded, (u32)(uintptr_t)baked,
                                          NDS_SDL_ROUTE_BAKED);
                    ndsStageDLRouteSlot(dl)->pad = (u8)baked_slots;
                }
                else if ((dobj->parent_gobj->id == nGCCommonKindItem) &&
                         (sNdsRendererAdapterItemSubmitActive != FALSE) &&
                         (sNdsRendererAdapterEffectSubmitActive == FALSE) &&
                         (sNdsRendererAdapterStagePersistentActive != FALSE))
                {
                    ndsStageDLRouteRecord(dl, loaded, (u32)(uintptr_t)baked,
                                          NDS_SDL_ROUTE_BAKED_ITEM);
                    ndsStageDLRouteSlot(dl)->pad = (u8)baked_slots;
                }
#if NDS_P2_1P_GAME
                else if ((sNdsRendererAdapterItemSubmitActive == FALSE) &&
                         (sNdsRendererAdapterEffectSubmitActive == FALSE) &&
                         (sNdsRendererAdapterStagePersistentActive != FALSE) &&
                         (ndsNativeBakedRootIsRoom(baked) != FALSE))
                {
                    ndsStageDLRouteRecord(dl, loaded, (u32)(uintptr_t)baked,
                                          NDS_SDL_ROUTE_BAKED_ROOM);
                    ndsStageDLRouteSlot(dl)->pad = (u8)baked_slots;
                }
#endif
#endif
            }
            else
            {
                NDS_DIAG(gNdsItemBakedSubmitFailCount++);
            }
        }
    }

    if (item_tomato_native_candidate != FALSE)
    {
        /* Same split-camera contract every fixed owner documents: fill the
         * identity on a COPY, because later code still reads the shared
         * config. */
        NDSRendererConfig tomato_config = config;
        NDSRendererMatrix20p12 tomato_identity;

        if ((tomato_config.initial_projection == NULL) &&
            (tomato_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&tomato_identity);
            tomato_config.initial_projection = &tomato_identity;
        }
        else if ((tomato_config.initial_modelview == NULL) &&
                 (tomato_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&tomato_identity);
            tomato_config.initial_modelview = &tomato_identity;
        }
        item_tomato_native_handled = ndsRendererSubmitNativeItemTomato(
            loaded->data, loaded->data_size, &tomato_config, render_stats);
        if (item_tomato_native_handled != FALSE)
        {
            NDS_DIAG(gNdsItemTomatoDrawCount++);
        }
        else
        {
            NDS_DIAG(gNdsItemTomatoSubmitFailCount++);
        }
    }
#endif
    if (visual_effect_native_candidate != FALSE)
    {
        /* Same split-camera contract the other fixed owners document: the
         * battle camera can supply the whole transform on one side of the DS
         * pair, and a fixed owner has no matrix stream to fill the other
         * implicitly. Fill the identity on a COPY. */
        NDSRendererConfig visual_config = config;
        NDSRendererMatrix20p12 visual_identity;

        if ((visual_config.initial_projection == NULL) &&
            (visual_config.initial_modelview != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&visual_identity);
            visual_config.initial_projection = &visual_identity;
        }
        else if ((visual_config.initial_modelview == NULL) &&
                 (visual_config.initial_projection != NULL))
        {
            ndsRendererAdapterMtxIdentity20p12(&visual_identity);
            visual_config.initial_modelview = &visual_identity;
        }
        visual_effect_native_handled = ndsRendererSubmitNativeVisualEffect(
            visual_effect_template, &visual_config, render_stats);
        if (visual_effect_native_handled != FALSE)
        {
            NDS_DIAG(gNdsVisualEffectNativeDrawCount++);
        }
        else
        {
            /* LOUD, AND WITH THE RIGHT REASON. An owner exists and refused, so
             * this is REJECTED_PROGRAM, not the generic NO_PROGRAM the guards
             * below publish for "nothing claimed this root". Recording it here
             * is what lets `settled` suppress those guards without turning a
             * refusal into a successful empty draw. */
            NDS_DIAG(gNdsVisualEffectNativeDeclineCount++);
            ndsStageRejectNativeRender(dobj, dl,
                NDS_NATIVE_FAILURE_REJECTED_PROGRAM, render_stats);
        }
        visual_effect_native_settled = TRUE;
    }
#if NDS_R2_IMPACT_WAVE_NATIVE
    if (
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_INISHIE
        (inishie_pakkun_native_handled == FALSE) &&
        (inishie_powblock_native_handled == FALSE) &&
#endif
#if NDS_R2_REBIRTH_HALO_NATIVE
        (rebirth_halo_native_handled == FALSE) &&
#endif
#if NDS_RENDERER_HW_TRIANGLES
        (charge_shot_native_handled == FALSE) &&
        (thunder_jolt_native_handled == FALSE) &&
        (thunder_ground_native_handled == FALSE) &&
        (thunder_fx_native_handled == FALSE) &&
        (damage_slash_native_settled == FALSE) &&
        (damage_fly_mdust_native_settled == FALSE) &&
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_NESS
        (ness_pkfire_native_handled == FALSE) &&
        (ness_pkthunder_native_handled == FALSE) &&
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_YOSHI
        (yoshi_entryegg_native_handled == FALSE) &&
        (yoshi_egg_native_handled == FALSE) &&
        (yoshi_egglay_native_handled == FALSE) &&
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_PURIN
        (purin_sing_native_handled == FALSE) &&
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_KIRBY
        (kirby_vulcan_native_handled == FALSE) &&
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_PIKACHU
        (pikachu_thunder_native_handled == FALSE) &&
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_SAMUS
        (samus_bomb_native_handled == FALSE) &&
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_NESS
        (ness_pktail_native_handled == FALSE) &&
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_CASTLE
        (castle_bumper_native_handled == FALSE) &&
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_SECTOR
        (sector_laser_native_handled == FALSE) &&
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_LINK
        (link_bomb_native_handled == FALSE) &&
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_YAMABUKI
        (marumine_native_handled == FALSE) &&
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_YAMABUKI && NDS_P2_ITEM_CORE
        (glucky_native_handled == FALSE) &&
        (porygon_native_handled == FALSE) &&
        (hitokage_native_handled == FALSE) &&
        (fushigibana_native_handled == FALSE) &&
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE
        (item_star_native_handled == FALSE) &&
        (item_sword_native_handled == FALSE) &&
        (item_hammer_native_handled == FALSE) &&
        (item_mball_native_handled == FALSE) &&
        (item_gshell_native_handled == FALSE) &&
        (item_rshell_native_handled == FALSE) &&
        (item_bat_native_handled == FALSE) &&
        (item_capsule_native_handled == FALSE) &&
        (item_bombhei_native_handled == FALSE) &&
        (item_lgun_native_handled == FALSE) &&
        (item_harisen_native_handled == FALSE) &&
        (item_heart_native_handled == FALSE) &&
        (item_starrod_native_handled == FALSE) &&
        (item_fflower_native_handled == FALSE) &&
        (item_msbomb_native_handled == FALSE) &&
        (item_nbumper_native_handled == FALSE) &&
        (item_box_native_handled == FALSE) &&
        (item_taru_native_handled == FALSE) &&
        (item_egg_native_handled == FALSE) &&
        (item_iwark_native_handled == FALSE) &&
        (item_tomato_native_handled == FALSE) &&
        (item_kirbystar_native_handled == FALSE) &&
        (item_baked_native_handled == FALSE) &&
#endif
        (visual_effect_native_settled == FALSE) &&
        (impact_wave_native_candidate != FALSE))
    {
        impact_wave_native_handled = ndsRendererSubmitNativeImpactWave(
            sNdsImpactWaveVertices,
            (u32)(sizeof(sNdsImpactWaveVertices) /
                  sizeof(sNdsImpactWaveVertices[0])),
            sNdsImpactWaveTriangles,
            (u32)(sizeof(sNdsImpactWaveTriangles) / 3u),
            dl,
            &impact_wave_material,
            sNdsRendererAdapterImpactWaveVariant,
            &config,
            render_stats);
    }
    if (
#if NDS_R2_REBIRTH_HALO_NATIVE
        (rebirth_halo_native_handled == FALSE) &&
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_INISHIE
        /* The Pakkun owner is checked here as well as in the impact-wave
         * OFF arm below. Without this term the item drew natively 600
         * times in a 300-present run and the stage still recorded 600
         * NO_PROGRAM failures at its own root, because the default build
         * has NDS_R2_IMPACT_WAVE_NATIVE = 1 and takes this branch. */
        (inishie_pakkun_native_handled == FALSE) &&
        /* The POW block owner is checked here for the identical reason. */
        (inishie_powblock_native_handled == FALSE) &&
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_SECTOR
        /* The laser owner is checked here AND in the OFF arm below, for the
         * identical reason the Pakkun comment above records. */
        (sector_laser_native_handled == FALSE) &&
#endif
#if NDS_RENDERER_HW_TRIANGLES
        (charge_shot_native_handled == FALSE) &&
        (thunder_jolt_native_handled == FALSE) &&
        (thunder_ground_native_handled == FALSE) &&
        (thunder_fx_native_handled == FALSE) &&
        (damage_slash_native_settled == FALSE) &&
        (damage_fly_mdust_native_settled == FALSE) &&
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_NESS
        (ness_pkfire_native_handled == FALSE) &&
        (ness_pkthunder_native_handled == FALSE) &&
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_YOSHI
        (yoshi_entryegg_native_handled == FALSE) &&
        (yoshi_egg_native_handled == FALSE) &&
        (yoshi_egglay_native_handled == FALSE) &&
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_PURIN
        (purin_sing_native_handled == FALSE) &&
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_KIRBY
        (kirby_vulcan_native_handled == FALSE) &&
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_PIKACHU
        (pikachu_thunder_native_handled == FALSE) &&
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_SAMUS
        (samus_bomb_native_handled == FALSE) &&
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_NESS
        (ness_pktail_native_handled == FALSE) &&
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_CASTLE
        (castle_bumper_native_handled == FALSE) &&
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_LINK
        /* The bomb owner is checked here AND in the OFF arm below, for the
         * identical reason the Pakkun comment above records. */
        (link_bomb_native_handled == FALSE) &&
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_YAMABUKI
        /* The Marumine owner is checked here AND in the OFF arm below, for the
         * identical reason the Pakkun comment above records. */
        (marumine_native_handled == FALSE) &&
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_YAMABUKI && NDS_P2_ITEM_CORE
        (glucky_native_handled == FALSE) &&
        (porygon_native_handled == FALSE) &&
        (hitokage_native_handled == FALSE) &&
        (fushigibana_native_handled == FALSE) &&
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE
        (item_star_native_handled == FALSE) &&
        (item_sword_native_handled == FALSE) &&
        (item_hammer_native_handled == FALSE) &&
        (item_mball_native_handled == FALSE) &&
        (item_gshell_native_handled == FALSE) &&
        (item_rshell_native_handled == FALSE) &&
        (item_bat_native_handled == FALSE) &&
        (item_capsule_native_handled == FALSE) &&
        (item_bombhei_native_handled == FALSE) &&
        (item_lgun_native_handled == FALSE) &&
        (item_harisen_native_handled == FALSE) &&
        (item_heart_native_handled == FALSE) &&
        (item_starrod_native_handled == FALSE) &&
        (item_fflower_native_handled == FALSE) &&
        (item_msbomb_native_handled == FALSE) &&
        (item_nbumper_native_handled == FALSE) &&
        (item_box_native_handled == FALSE) &&
        (item_taru_native_handled == FALSE) &&
        (item_egg_native_handled == FALSE) &&
        (item_iwark_native_handled == FALSE) &&
        (item_tomato_native_handled == FALSE) &&
        (item_kirbystar_native_handled == FALSE) &&
        (item_baked_native_handled == FALSE) &&
#endif
        /* Unconditional: this owner has no build flag, so it must be excluded
         * from BOTH the impact-wave ON arm here and the OFF arm below. */
        (visual_effect_native_settled == FALSE) &&
        (impact_wave_native_handled == FALSE))
    {
        if (impact_wave_native_candidate != FALSE)
        {
            /* Keep the existing rejection witness; this cannot select a
             * different renderer or discard the failure as an empty draw. */
            NDS_DIAG(gNdsImpactWaveNativeFallbackCount++);
        }
        ndsStageRejectNativeRender(dobj, dl,
            NDS_NATIVE_FAILURE_NO_PROGRAM, render_stats);
    }
    else
    {
        NDS_DIAG(gNdsImpactWaveNativeDrawCount++);
    }
#else
/* Leading-and form with a constant seed, so each native owner contributes ONE
 * self-contained #if block instead of a hand-glued "&&" between two #ifs.  The
 * old trailing-and shape needed a cross-owner fragment inside a nested #if for
 * every owner added, and that fragment is what got forgotten when the Pakkun
 * owner landed with only the ON arm's term. */
#if NDS_R2_REBIRTH_HALO_NATIVE || \
    NDS_RENDERER_HW_TRIANGLES || \
    (NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_INISHIE) || \
    (NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_SECTOR) || \
    (NDS_RENDERER_HW_TRIANGLES && NDS_P2_LINK) || \
    (NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_YAMABUKI)
    if (TRUE
#if NDS_R2_REBIRTH_HALO_NATIVE
        && (rebirth_halo_native_handled == FALSE)
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_INISHIE
        && (inishie_pakkun_native_handled == FALSE)
        && (inishie_powblock_native_handled == FALSE)
#endif
#if NDS_RENDERER_HW_TRIANGLES
        && (charge_shot_native_handled == FALSE)
        && (thunder_jolt_native_handled == FALSE)
        && (thunder_ground_native_handled == FALSE)
        && (thunder_fx_native_handled == FALSE)
        && (damage_slash_native_settled == FALSE)
        && (damage_fly_mdust_native_settled == FALSE)
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_NESS
        && (ness_pkfire_native_handled == FALSE)
        && (ness_pkthunder_native_handled == FALSE)
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_YOSHI
        && (yoshi_entryegg_native_handled == FALSE)
        && (yoshi_egg_native_handled == FALSE)
        && (yoshi_egglay_native_handled == FALSE)
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_PURIN
        && (purin_sing_native_handled == FALSE)
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_KIRBY
        && (kirby_vulcan_native_handled == FALSE)
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_PIKACHU
        && (pikachu_thunder_native_handled == FALSE)
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_SAMUS
        && (samus_bomb_native_handled == FALSE)
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_NESS
        && (ness_pktail_native_handled == FALSE)
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_CASTLE
        && (castle_bumper_native_handled == FALSE)
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_SECTOR
        && (sector_laser_native_handled == FALSE)
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_LINK
        && (link_bomb_native_handled == FALSE)
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_YAMABUKI
        && (marumine_native_handled == FALSE)
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_STAGE_YAMABUKI && NDS_P2_ITEM_CORE
        && (glucky_native_handled == FALSE)
        && (porygon_native_handled == FALSE)
        && (hitokage_native_handled == FALSE)
        && (fushigibana_native_handled == FALSE)
#endif
#if NDS_RENDERER_HW_TRIANGLES && NDS_P2_ITEM_CORE
        && (item_star_native_handled == FALSE)
        && (item_sword_native_handled == FALSE)
        && (item_hammer_native_handled == FALSE)
        && (item_mball_native_handled == FALSE)
        && (item_gshell_native_handled == FALSE)
        && (item_rshell_native_handled == FALSE)
        && (item_bat_native_handled == FALSE)
        && (item_capsule_native_handled == FALSE)
        && (item_bombhei_native_handled == FALSE)
        && (item_lgun_native_handled == FALSE)
        && (item_harisen_native_handled == FALSE)
        && (item_heart_native_handled == FALSE)
        && (item_starrod_native_handled == FALSE)
        && (item_fflower_native_handled == FALSE)
        && (item_msbomb_native_handled == FALSE)
        && (item_nbumper_native_handled == FALSE)
        && (item_box_native_handled == FALSE)
        && (item_taru_native_handled == FALSE)
        && (item_egg_native_handled == FALSE)
        && (item_iwark_native_handled == FALSE)
        && (item_tomato_native_handled == FALSE)
        && (item_kirbystar_native_handled == FALSE)
        && (item_baked_native_handled == FALSE)
#endif
        && (visual_effect_native_settled == FALSE)
       )
#endif
    {
        ndsStageRejectNativeRender(dobj, dl,
            NDS_NATIVE_FAILURE_NO_PROGRAM, render_stats);
    }
#endif
    NDS_LAB_SDL_MARK(3);
#if NDS_TICK_HUD && NDS_P2_EFFECT_CENSUS
    if (phase_effect != FALSE)
    {
        gNdsEffectPhaseExecTicks += cpuGetTiming() - phase_mark;
        ndsEffectPacketCaptureEnd();
    }
#endif
    if (sNdsRendererAdapterEffectSubmitActive != FALSE)
    {
        /* The config's matrices read from OUR locals, and the executor's own
         * verdict on them read as a delta. Cycles 53-55 tried to read the same
         * two pointers out of the callee's argument register and got three
         * different answers; nds_effects.h records why that read can never
         * settle it. */
        /* Out - In names an asset list that carries its own render mode: the
         * rebirth halo's DObj entry[2] list-0 leaves set TEX_EDGE and restore
         * OPA_SURF (85.vpk0.bin 0x23a8/0x2490), while its list-1 beam
         * (0x2890) emits no G_SETOTHERMODE_L at all and leaves Out == In. */
        gNdsEffectDLSubmitOtherModeOut = render_stats->othermode_l;
        NDS_DIAG(gNdsEffectDLSubmitCount++);
        gNdsEffectDLCfgMask =
            ((config.initial_projection != NULL) ? 1u : 0u) |
            ((config.initial_modelview != NULL) ? 2u : 0u);
        if (config.initial_modelview != NULL)
        {
            gNdsEffectDLCfgMvT[0] = config.initial_modelview->m[3][0];
            gNdsEffectDLCfgMvT[1] = config.initial_modelview->m[3][1];
            gNdsEffectDLCfgMvT[2] = config.initial_modelview->m[3][2];
        }
        gNdsEffectDLMatrixSeed =
            render_stats->hardware_matrix_seed_count - effect_seed_before;
        gNdsEffectDLMatrixCmd =
            render_stats->matrix_command_count - effect_matrix_cmd_before;
        gNdsEffectDLXformVertexCount =
            render_stats->transformed_vertex_count - effect_xform_before;
        gNdsEffectDLHwVertexCount =
            render_stats->hardware_vertex_count - effect_hw_vertex_before;
        gNdsEffectDLHwTriangleCount =
            render_stats->hardware_triangle_count - effect_hw_triangle_before;
#if NDS_RENDERER_HW_TRIANGLES
        if (sNdsRendererAdapterStagePersistentActive != FALSE)
        {
            gNdsEffectDLVtx0[0] =
                sNdsRendererAdapterStageVertexCache.transformed_vertices[0].x;
            gNdsEffectDLVtx0[1] =
                sNdsRendererAdapterStageVertexCache.transformed_vertices[0].y;
            gNdsEffectDLVtx0[2] =
                sNdsRendererAdapterStageVertexCache.transformed_vertices[0].z;
            gNdsEffectDLVtx0[3] =
                sNdsRendererAdapterStageVertexCache.transformed_vertices[0].w;
        }
#endif
        gNdsEffectDLBlocker = render_stats->blocker;
        gNdsEffectDLCommandCount = render_stats->command_count;
#if NDS_TICK_HUD && NDS_P2_EFFECT_CENSUS
        /* R2-08 CAP-VERSUS-END. The two lines above are LAST-VALUE-WINS, so a
         * stop reads one list; these are cumulative, so a stop reads the whole
         * window. The question they settle is whether the interpreter stops at
         * the list's end or runs to config->max_commands (8192): the executor
         * sets blocker = BUDGET on exactly that path (nds_renderer.c:28303),
         * and BLOCKER_NONE means it reached G_ENDDL under its own steam.
         *
         * This exists because an arithmetic COINCIDENCE nearly bought a
         * deferral: 8192 x 12.54 = 102,727 against a measured 102,730 per list
         * looks like proof the loop runs to its cap, but the 12.54 was obtained
         * by dividing 102,730 BY 8192, so the agreement is a tautology and
         * carries no information. Mean commands per list is the honest form of
         * the same question and it is two adds. */
        gNdsEffectDLCommandTotal += render_stats->command_count;
        if (render_stats->blocker == NDS_RENDERER_BLOCKER_BUDGET)
        {
            gNdsEffectDLTermCapCount++;
        }
        else if (render_stats->blocker == NDS_RENDERER_BLOCKER_NONE)
        {
            gNdsEffectDLTermEndCount++;
        }
        else
        {
            gNdsEffectDLTermOtherCount++;
            gNdsEffectDLTermOtherMask |= 1u << (render_stats->blocker & 31u);
        }
#endif
        gNdsEffectDLFirstOpcode = render_stats->first_opcode;
        gNdsEffectDLUnsupportedOpcode = render_stats->unsupported_opcode;
        /* vertex_count/triangle_count, NOT the *_command_count pair: every
         * site that increments those is wrapped in
         * NDS_RENDERER_RECORD_PROOF_ONLY, which is ((void)0) whenever
         * NDS_RENDERER_HW_TRIANGLES is set -- i.e. dead in every build that
         * can draw. Reading them cost this investigation one wrong conclusion. */
        gNdsEffectDLVertexCount = render_stats->vertex_count;
        gNdsEffectDLTriangleCount = render_stats->triangle_count;
        NDS_DIAG(gNdsEffectDLPublishCount++);
#if NDS_TICK_HUD && NDS_P2_EFFECT_CENSUS
        /* Cumulative twins of the two last-value-wins deltas above: a stop reads
         * one list from those, and the census needs the whole window. */
        {
            u32 census_tris = render_stats->hardware_triangle_count -
                effect_hw_triangle_before;
            u32 census_verts = render_stats->hardware_vertex_count -
                effect_hw_vertex_before;

            gNdsEffectDLTriangleTotal += census_tris;
            gNdsEffectDLVertexTotal += census_verts;
            /* OtherModeIn was latched for THIS list before the executor ran, so
             * it is the entry state; render_stats->othermode_l is now exit. */
            ndsEffectDLCensusRecord(dl, render_stats->command_count,
                                    (u32)gNdsEffectDLSubmitOtherModeIn,
                                    census_tris, census_verts);
            /* Same key, same instant, same population as the census above. If
             * the capture's own arming condition ever disagreed with this one,
             * gNdsEffectPacketCaptureCount would diverge from
             * gNdsEffectDLSubmitCount -- both are published, so the
             * disagreement would be visible rather than silent. */
            ndsEffectPacketVerdictRecord(dl);
        }
#endif
    }
#if NDS_RENDERER_HW_TRIANGLES
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    ndsRendererAdapterAccumulateDepth(
        render_stats,
        &gNdsRendererDepthStageSamples,
        &gNdsRendererDepthStageMin,
        &gNdsRendererDepthStageMax,
        &gNdsRendererDepthStageWMin,
        &gNdsRendererDepthStageWMax);
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    NDS_DIAG(gNdsRendererProfileDLTicks += cpuGetTiming() - step_start);
    adapter_ticks = cpuGetTiming() - adapter_start;
    NDS_DIAG(gNdsRendererProfileStageAdapterTicks += adapter_ticks);
    ndsRendererOwnerAccumulateList(
        NDS_RENDERER_PROFILE_OWNER_STAGE, loaded, dl,
        gNdsRendererProfileOwners[
            NDS_RENDERER_PROFILE_OWNER_STAGE].selected_count,
        initial_projection_ptr, initial_modelview_ptr,
        &config,
        &owner_stats_before, render_stats);
#endif
    /* P2-3r13: the fighter's own graphics-heap peak, before it is rolled back
     * and becomes invisible to the end-of-frame sample. */
    ndsTaskmanSampleGraphicsHeap();
    gSYTaskmanGraphicsHeap.ptr = saved_graphics_heap_ptr;
    if (sNdsRendererAdapterStagePersistentActive != FALSE)
    {
#if NDS_RENDERER_PROFILE_LEVEL < 2
        if (detailed_output != FALSE)
        {
            ndsFighterDLDrawCapturePersistentState(
                &sNdsRendererAdapterStagePersistentState, &state);
        }
        else
        {
            sNdsRendererAdapterStagePersistentState.segment_e_base =
                state.segment_e_base;
            sNdsRendererAdapterStagePersistentState.segment_e_end =
                state.segment_e_end;
        }
#else
        ndsFighterDLDrawCapturePersistentState(
            &sNdsRendererAdapterStagePersistentState, &state);
#endif
#if NDS_RENDERER_PROFILE_LEVEL >= 2
        ndsFighterDLDrawCopyPersistentRendererState(
            &sNdsRendererAdapterStagePersistentStats, render_stats);
#endif
        NDS_DIAG(gNdsStageGCDrawAllLoopHardwareCarryCaptureCount++);
        if (render_stats->command_count <= 5u)
        {
            if (inherited_texture != FALSE)
            {
                NDS_DIAG(gNdsStageGCDrawAllLoopHardwareCarryShortTextureSeedCount++);
            }
            if (inherited_tile != FALSE)
            {
                NDS_DIAG(gNdsStageGCDrawAllLoopHardwareCarryShortTileSeedCount++);
            }
        }
    }
#if NDS_RENDERER_PROFILE_LEVEL >= 2
    if ((gNdsRendererProfileHardwareTriangles > 2048u) ||
        (gNdsRendererProfileHardwareVertices > 6144u))
    {
        gNdsRendererProfileHardwareOverLimit = 1u;
    }
#endif
#endif
    gNdsStageGCDrawAllLoopHardwareTriangleCount +=
        render_stats->hardware_triangle_count;
    gNdsStageGCDrawAllLoopHardwareZBufferTriangleCount +=
        render_stats->hardware_zbuffer_triangle_count;
    gNdsStageGCDrawAllLoopHardwareProjectedDepthTriangleCount +=
        render_stats->hardware_projected_depth_triangle_count;
    gNdsStageGCDrawAllLoopHardwareDecalDepthTriangleCount +=
        render_stats->hardware_decal_depth_triangle_count;
    gNdsStageGCDrawAllLoopHardwareTextureBindCount +=
        render_stats->hardware_texture_bind_count;
    gNdsStageGCDrawAllLoopHardwareTextureUploadCount +=
        render_stats->hardware_texture_upload_count;
    gNdsStageGCDrawAllLoopHardwareTextureReadyCount +=
        render_stats->hardware_texture_ready_count;
    gNdsStageGCDrawAllLoopHardwareTextureRejectCount +=
        render_stats->hardware_texture_reject_count;
    if (render_stats->hardware_texture_ready_count != 0u)
    {
        if (render_stats->hardware_texture_format < 32u)
        {
            gNdsStageGCDrawAllLoopHardwareTextureFormatMask |=
                1u << render_stats->hardware_texture_format;
        }
        if (render_stats->hardware_texture_width >
            gNdsStageGCDrawAllLoopHardwareTextureMaxWidth)
        {
            gNdsStageGCDrawAllLoopHardwareTextureMaxWidth =
                render_stats->hardware_texture_width;
        }
        if (render_stats->hardware_texture_height >
            gNdsStageGCDrawAllLoopHardwareTextureMaxHeight)
        {
            gNdsStageGCDrawAllLoopHardwareTextureMaxHeight =
                render_stats->hardware_texture_height;
        }
    }
#if NDS_TICK_HUD && NDS_P2_EFFECT_CENSUS
    if (phase_effect != FALSE)
    {
        gNdsEffectPhaseDLTicks += cpuGetTiming() - phase_dl_mark;
    }
#endif
}

/* ONE NODE IS NOT A TREE, AND THAT IS WHY THE SHIELD WAS A QUARTER CIRCLE.
 *
 * This submitted the DObj it was handed and stopped. The source does not:
 * gcDrawDObjTree (objdisplay.c) recurses into `child` and, from the first
 * sibling, walks the whole `sib_next` chain. Every multi-node effect model
 * therefore drew only its ROOT node here -- the owner's own words on the first
 * build that routed the real asset were "it is using the correct asset but its
 * only like 1/2 or 1/4 of it, like a 1/4 slice of the complete circle".
 *
 * That single omission is what four BUGS.md rows have in common. The shield,
 * Fox's reflector, the rebirth halo and the impact wave are all EFDescs whose
 * geometry is a DObj tree, so all four were being drawn one node deep, and no
 * amount of atlas resolution or palette work could ever have shown the rest of
 * them.
 *
 * The sibling rule is the source's, kept exactly: only a node with no
 * `sib_prev` walks the chain, so a tree is traversed once rather than once per
 * sibling. Recursion depth is the model's own node depth, which these effect
 * descs keep shallow.
 *
 * NOT copied from the source: gcPrepDObjMatrix and the matching gSPPopMatrix.
 * The DS path composes its transform inside ndsRendererAdapterSubmitStageDL
 * per display list rather than pushing an N64 matrix stack here, so adding a
 * push/pop pair around the recursion would double-transform every child. */
static void ndsRendererAdapterSubmitStageDObjNode(DObj *dobj, u32 kind,
                                                  GObj *camera_gobj,
                                                  u32 initial_geometry_mode);

/* BOUNDED, because this walks a tree the PORT builds from resolved offsets and
 * not one the N64 shipped. Those offsets have held raw symbol addresses before
 * -- the note at Makefile:1393 records gcSetupCustomDObjs walking garbage and
 * allocating a DObj per bogus node until the allocator gave up -- so a cycle or
 * a wild pointer here is a real possibility, and unbounded recursion over one
 * is a hung handheld rather than a wrong picture. The first build of this walk
 * timed out a 300-second probe, which is exactly that failure.
 *
 * The limits are far above any real effect model (these descs are a root plus a
 * handful of parts) and both overruns are counted, so hitting one is a
 * diagnosable defect instead of a freeze. */
#define NDS_RENDERER_STAGE_DOBJ_MAX_DEPTH 16u
#define NDS_RENDERER_STAGE_DOBJ_MAX_SIBLINGS 64u

/* The three counters live in diagnostics.c, not here. Defining them inside this
 * `#if NDS_RENDERER_HW_TRIANGLES` block would make them exist only in the
 * configurations that increment them, and a probe naming an absent symbol loses
 * its whole gdb run. nds_effects.h declares them; cliff_ledge.c resets them,
 * which is also what keeps --gc-sections from collecting them. */

static void ndsRendererAdapterSubmitStageDObjTreeDepth(
    DObj *dobj, u32 kind, GObj *camera_gobj, u32 initial_geometry_mode,
    u32 depth)
{
    DObj *sibling;
    u32 seen;

    if (dobj == NULL)
    {
        return;
    }
    if (depth >= NDS_RENDERER_STAGE_DOBJ_MAX_DEPTH)
    {
        NDS_DIAG(gNdsRendererStageDObjDepthOverrunCount++);
        return;
    }
    NDS_DIAG(gNdsRendererStageDObjNodeCount++);
#if NDS_TICK_HUD && NDS_P2_EFFECT_CENSUS
    if (sNdsRendererAdapterEffectSubmitActive != FALSE)
    {
        gNdsEffectPhaseNodeCount++;
    }
#endif
    /* BattleShip gcDrawDObjTree makes HIDDEN a subtree visibility flag: the
     * node's own draw AND its child walk live inside the same !HIDDEN block.
     * The sibling walk is outside that block, so a hidden node must not hide
     * its siblings.  The port used to submit the node through the drawable
     * gate but recurse into its child unconditionally; Mario's pipe exposes
     * that at source frame 100, when the rim becomes HIDDEN for the final 20
     * frames but its barrel child was still emitted as a flat gray/white slab.
     * Keep NOTEXTURE behavior unchanged: it suppresses only this node's DL,
     * not descendants. */
    if ((dobj->flags & DOBJ_FLAG_HIDDEN) == 0u)
    {
        ndsRendererAdapterSubmitStageDObjNode(dobj, kind, camera_gobj,
                                              initial_geometry_mode);
        if (dobj->child != NULL)
        {
            ndsRendererAdapterSubmitStageDObjTreeDepth(
                dobj->child, kind, camera_gobj, initial_geometry_mode,
                depth + 1u);
        }
    }
    if (dobj->sib_prev == NULL)
    {
        seen = 0u;
        for (sibling = dobj->sib_next; sibling != NULL;
             sibling = sibling->sib_next)
        {
            if (++seen > NDS_RENDERER_STAGE_DOBJ_MAX_SIBLINGS)
            {
                NDS_DIAG(gNdsRendererStageDObjSiblingOverrunCount++);
                break;
            }
            ndsRendererAdapterSubmitStageDObjTreeDepth(
                sibling, kind, camera_gobj, initial_geometry_mode,
                depth + 1u);
        }
    }
}

/* EXPLICIT TREE OWNERS ONLY, AND THE MEASUREMENT IS WHY.
 *
 * The first version of this recursed inside ndsRendererAdapterSubmitStageDObj,
 * which is the STAGE entry point -- the stage, the weapons and the effects all
 * reach the hardware through it. A synchronized tick-HUD A/B on identical
 * frames priced that at one whole VBlank:
 *
 *     frame   control      candidate
 *       441   1,119,936    1,120,000
 *       443   1,119,872    1,120,000
 *       447   1,119,488    1,680,384   <- 2 VBlanks -> 3
 *       449   1,119,872    1,680,256   <- 2 VBlanks -> 3
 *
 * +560,896 ticks is 560,190-per-VBlank almost exactly, on frames that were
 * inside the 1.12M gate. The stage carries 57 DObjs (M3_NATIVE_STAGE_CHECK
 * dobjs=57) and the native stage path already handles their geometry, so
 * re-walking them bought nothing and cost a frame.
 *
 * The source models that actually use tree display callbacks still need their
 * descendants. So recursion lives on the explicit effect/item/weapon call
 * sites, while the shared stage entry stays a single-node submit. This keeps
 * the measured stage regression out of normal frames without flattening a
 * source DObj tree such as Link's Boomerang into its transform-only root. */
void ndsRendererAdapterSubmitEffectDObjTree(void *dobj_ptr, u32 kind,
                                            void *camera_gobj_ptr,
                                            u32 initial_geometry_mode)
{
    DObj *root = (DObj *)dobj_ptr;

#if NDS_TASK49_GX_DIFFER
    ndsRendererProfileSetOwner(NDS_RENDERER_PROFILE_OWNER_EFFECT);
#endif

    /* The procedural visual templates: proc plus vars, resolved once per GObj,
     * exactly as the impact wave's latch does. The DObj cannot answer this --
     * sNdsVisualTemplates is static to battleship_efmanager.c and the list has
     * no asset id -- so the effect owner is asked. */
    sNdsRendererAdapterVisualEffectTemplate = 0u;
    sNdsRendererAdapterVisualEffectNativeActive =
        ((root != NULL) && (root->parent_gobj != NULL) &&
         (ndsEFManagerVisualTemplateIndex(
              root->parent_gobj,
              &sNdsRendererAdapterVisualEffectTemplate) != FALSE)) ?
            TRUE : FALSE;
#if NDS_R2_IMPACT_WAVE_NATIVE

    sNdsRendererAdapterImpactWaveVariant = 0u;
    sNdsRendererAdapterImpactWaveNativeActive =
        ((root != NULL) && (root->parent_gobj != NULL) &&
         (ndsEFManagerImpactWaveVariant(
              root->parent_gobj,
              &sNdsRendererAdapterImpactWaveVariant) != FALSE)) ?
            TRUE : FALSE;
#endif
#if NDS_R2_REBIRTH_HALO_NATIVE
    sNdsRendererAdapterRebirthHaloNativeActive =
        ((root != NULL) && (root->xobjs_num != 0) &&
         (root->xobjs[0] != NULL) &&
         (root->xobjs[0]->kind == NDS_RENDERER_ADAPTER_JOINT_ATTACH_TRA_MTX_KIND) &&
         (root->child != NULL) && (gEFManagerFiles[2] != NULL) &&
         (root->child->dl_link ==
              (DObjDLLink *)((u8 *)gEFManagerFiles[2] + 0x2a98u))) ?
            TRUE : FALSE;
#if NDS_R2_REBIRTH_HALO_FAST_ADAPTER
    sNdsRendererAdapterRebirthHaloSkipSecondChildList = FALSE;
#endif
#endif
    sNdsRendererAdapterEffectSubmitActive = TRUE;
#if NDS_TICK_HUD && NDS_P2_EFFECT_CENSUS
    gNdsEffectPhaseActive = 1u;
#endif
    ndsRendererAdapterSubmitStageDObjTreeDepth(dobj_ptr, kind, camera_gobj_ptr,
                                               initial_geometry_mode, 0u);
#if NDS_TICK_HUD && NDS_P2_EFFECT_CENSUS
    gNdsEffectPhaseActive = 0u;
#endif
    sNdsRendererAdapterEffectSubmitActive = FALSE;
    sNdsRendererAdapterVisualEffectNativeActive = FALSE;
    sNdsRendererAdapterVisualEffectTemplate = 0u;
#if NDS_R2_IMPACT_WAVE_NATIVE
    sNdsRendererAdapterImpactWaveNativeActive = FALSE;
    sNdsRendererAdapterImpactWaveVariant = 0u;
#endif
#if NDS_R2_REBIRTH_HALO_NATIVE
    sNdsRendererAdapterRebirthHaloNativeActive = FALSE;
#if NDS_R2_REBIRTH_HALO_FAST_ADAPTER
    sNdsRendererAdapterRebirthHaloSkipSecondChildList = FALSE;
#endif
#endif
}

/* P2-6 (2026-10-03): the Race's rolling barrel bombs and the stage bumpers
 * (Race, the bonus boards, Peach's Castle) wholly outside the view, skipped
 * before any of their lists pays the route, its MObj snapshots and its world
 * matrix (~21K ticks an off-screen barrel, ~13K a bumper; the Race drew three
 * barrels and four bumpers every frame, a camera showing one or two). The
 * bound is the item's root position with a sphere around it: the model's
 * radius (barrel +-222 cube, bumper quad +-180), widened by each child joint's
 * offset and scale and by the root's scale (a bumper doubles on a hit), in
 * the 20.12 camera matrices the lists' own culls use (the baked roots' and the
 * bumper quad's planes, ndsNativeBakedRootOutsideView). A held item, a deeper
 * tree or another kind draws as before. */
__attribute__((used)) volatile u32 gNdsItemPreCulled;

static sb32 ndsRendererAdapterItemOffscreen(DObj *root, GObj *camera_gobj)
{
    GObj *item_gobj;
    const ITStruct *ip;
    const DObj *child;
    CObj *cobj;
    NDSRendererMatrix20p12 projection;
    NDSRendererMatrix20p12 modelview;
    NDSRendererMatrix20p12 m;
    u32 projection_valid = FALSE;
    u32 modelview_valid = FALSE;
    f32 model_radius;
    f32 extent;
    f32 scale;
    s64 radius;
    s32 p[3];
    u32 plane;
    u32 axis;

    if ((root == NULL) || (camera_gobj == NULL) ||
        (root->parent != DOBJ_PARENT_NULL))
    {
        return FALSE;
    }
    item_gobj = root->parent_gobj;
    ip = (item_gobj != NULL) ? itGetStruct(item_gobj) : NULL;
    cobj = CObjGetStruct(camera_gobj);
    if ((ip == NULL) || (cobj == NULL) || (ip->is_hold != FALSE))
    {
        return FALSE;
    }
    if (ip->kind == nITKindGBumper)
    {
        model_radius = 256.0F;
    }
    else if (ip->kind == nITKindTaruBomb)
    {
        model_radius = 400.0F;
    }
    else
    {
        return FALSE;
    }
    extent = model_radius;
    for (child = root->child; child != NULL; child = child->sib_next)
    {
        f32 t = 0.0F;
        f32 s = 0.0F;

        if (child->child != NULL)
        {
            return FALSE;
        }
        for (axis = 0u; axis < 3u; axis++)
        {
            f32 tc = (&child->translate.vec.f.x)[axis];
            f32 sc = (&child->scale.vec.f.x)[axis];

            t += (tc < 0.0F) ? -tc : tc;
            sc = (sc < 0.0F) ? -sc : sc;
            s = (sc > s) ? sc : s;
        }
        t += model_radius * s;
        extent = (t > extent) ? t : extent;
    }
    scale = 0.0F;
    for (axis = 0u; axis < 3u; axis++)
    {
        f32 sr = (&root->scale.vec.f.x)[axis];

        sr = (sr < 0.0F) ? -sr : sr;
        scale = (sr > scale) ? sr : scale;
        p[axis] = (s32)(&root->translate.vec.f.x)[axis];
    }
    extent *= scale;
    /* A sibling of the root is another world-space DObj the tree draws: its
     * offset from the root, unscaled by it. */
    for (child = root->sib_next; child != NULL; child = child->sib_next)
    {
        f32 t = 0.0F;
        f32 s = 0.0F;

        if (child->child != NULL)
        {
            return FALSE;
        }
        for (axis = 0u; axis < 3u; axis++)
        {
            f32 tc = (&child->translate.vec.f.x)[axis] -
                     (&root->translate.vec.f.x)[axis];
            f32 sc = (&child->scale.vec.f.x)[axis];

            t += (tc < 0.0F) ? -tc : tc;
            sc = (sc < 0.0F) ? -sc : sc;
            s = (sc > s) ? sc : s;
        }
        t += model_radius * s;
        extent = (t > extent) ? t : extent;
    }
    /* +2: the position truncates toward zero. */
    radius = (s64)extent + 2;
    ndsRendererAdapterGetFrameCameraMatrices(cobj, &projection,
                                             &projection_valid, &modelview,
                                             &modelview_valid, NULL, NULL,
                                             NULL);
    if (projection_valid == FALSE)
    {
        return FALSE;
    }
    /* The battle camera folds its look-at into the projection and leaves no
     * camera modelview: the lists' modelview is then the world matrix alone
     * (ndsRendererAdapterPrepareInitialMatrices). */
    if (modelview_valid != FALSE)
    {
        ndsRendererMtxMul20p12(&modelview, &projection, &m);
    }
    else
    {
        m = projection;
    }
    for (plane = 0u; plane < 4u; plane++)
    {
        const u32 col = plane >> 1;
        const s64 sign = ((plane & 1u) != 0u) ? 1 : -1;
        s64 d = (s64)m.m[3][3] + (sign * (s64)m.m[3][col]);
        s64 span = 0;

        for (axis = 0u; axis < 3u; axis++)
        {
            s64 c = (s64)m.m[axis][3] + (sign * (s64)m.m[axis][col]);

            d += c * (s64)p[axis];
            span += (c >= 0) ? c : -c;
        }
        if ((d + (span * radius)) < 0)
        {
            NDS_DIAG(gNdsItemPreCulled++);
            return TRUE;
        }
    }
    return FALSE;
}

void ndsRendererAdapterSubmitItemDObjTree(void *dobj_ptr, u32 kind,
                                          void *camera_gobj_ptr,
                                          u32 initial_geometry_mode)
{
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    u32 lab_offscreen = cpuGetTiming();
#endif
    const sb32 offscreen = ndsRendererAdapterItemOffscreen(
        (DObj *)dobj_ptr, (GObj *)camera_gobj_ptr);

#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    NDS_DIAG(gNdsLabItemAcc[11] += cpuGetTiming() - lab_offscreen);
#endif

    if (offscreen != FALSE)
    {
        return;
    }
    sNdsRendererAdapterItemSubmitActive = TRUE;
    sNdsRendererAdapterItemSubmitHead = 0u;
    ndsRendererAdapterSubmitStageDObjTreeDepth(dobj_ptr, kind, camera_gobj_ptr,
                                               initial_geometry_mode, 0u);
    sNdsRendererAdapterItemSubmitActive = FALSE;
    sNdsRendererAdapterItemSubmitHead = 0u;
}

#if NDS_P2_ITEM_CORE
/* The lists ndsRendererAdapterSubmitStageDObjTreeDepth would submit, in its
 * order: hidden subtrees skipped, siblings walked from the first child, each
 * drawable node's list (or DLLink lists) with the head the submit names.
 * Returns the count, or NDS_ITEM_REPLAY_ROOTS + 1 when it cannot hold them. */
static u32 ndsItemReplayWalk(DObj *dobj, u32 kind, NDSItemReplayRoot *roots,
                             u32 count, u32 depth)
{
    DObj *sibling;
    u32 seen;

    if ((dobj == NULL) || (count > NDS_ITEM_REPLAY_ROOTS))
    {
        return count;
    }
    if (depth >= NDS_RENDERER_STAGE_DOBJ_MAX_DEPTH)
    {
        return NDS_ITEM_REPLAY_ROOTS + 1u;
    }
    if ((dobj->flags & DOBJ_FLAG_HIDDEN) == 0u)
    {
        if (ndsRendererAdapterStageDObjDrawable(dobj, kind) != FALSE)
        {
            if ((kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE) ||
                (kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD0) ||
                (kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1))
            {
                if (dobj->dv != NULL)
                {
                    if (count >= NDS_ITEM_REPLAY_ROOTS)
                    {
                        return NDS_ITEM_REPLAY_ROOTS + 1u;
                    }
                    roots[count].dobj = dobj;
                    roots[count].dl = dobj->dl;
                    roots[count].head =
                        (kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1) ?
                            1u : 0u;
                    count++;
                }
            }
            else if ((kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS) ||
                     (kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLLINKS))
            {
                const DObjDLLink *dl_link = dobj->dl_link;
                u32 link;

                for (link = 0u; (dl_link != NULL) &&
                     (link < GC_COMMON_MAX_DLLINKS); link++, dl_link++)
                {
                    if (dl_link->list_id == (s32)NDS_RENDERER_STAGE_DL_HEADS)
                    {
                        break;
                    }
                    if ((dl_link->list_id >= 0) &&
                        ((u32)dl_link->list_id < NDS_RENDERER_STAGE_DL_HEADS) &&
                        (dl_link->dl != NULL))
                    {
                        if (count >= NDS_ITEM_REPLAY_ROOTS)
                        {
                            return NDS_ITEM_REPLAY_ROOTS + 1u;
                        }
                        roots[count].dobj = dobj;
                        roots[count].dl = dl_link->dl;
                        roots[count].head = (u8)dl_link->list_id;
                        count++;
                    }
                }
            }
            else
            {
                return NDS_ITEM_REPLAY_ROOTS + 1u;
            }
        }
        if (dobj->child != NULL)
        {
            count = ndsItemReplayWalk(dobj->child, kind, roots, count,
                                      depth + 1u);
        }
    }
    if (dobj->sib_prev == NULL)
    {
        seen = 0u;
        for (sibling = dobj->sib_next;
             (sibling != NULL) && (count <= NDS_ITEM_REPLAY_ROOTS);
             sibling = sibling->sib_next)
        {
            if (++seen > NDS_RENDERER_STAGE_DOBJ_MAX_SIBLINGS)
            {
                return NDS_ITEM_REPLAY_ROOTS + 1u;
            }
            count = ndsItemReplayWalk(sibling, kind, roots, count,
                                      depth + 1u);
        }
    }
    return count;
}

/* Everything an owner's GX output for these lists depends on besides the
 * matrices and the resident textures. Returns the key length, 0 when the
 * draw cannot be keyed. */
static u32 ndsItemReplayKey(const DObj *root, u32 item_kind,
                            u32 initial_geometry_mode,
                            const NDSItemReplayRoot *roots, u32 root_count,
                            u32 *key)
{
    u32 head_seen = 0u;
    u32 n = 0u;
    u32 i;

    key[n++] = (u32)(uintptr_t)root;
    key[n++] = item_kind;
    key[n++] = initial_geometry_mode;
    key[n++] = (u32)sNdsFighterDisplayCurrentLightValid;
    key[n++] = sNdsFighterDisplayCurrentLightCount;
    key[n++] = ((u32)(u8)sNdsFighterDisplayCurrentLight.l.dir[0]) |
               ((u32)(u8)sNdsFighterDisplayCurrentLight.l.dir[1] << 8) |
               ((u32)(u8)sNdsFighterDisplayCurrentLight.l.dir[2] << 16);
    key[n++] = (u32)ndsRendererHardwareNoOracleEnabled();
    for (i = 0u; i < root_count; i++)
    {
        const NDSItemReplayRoot *r = &roots[i];
        const NDSStageDLRoute *route = ndsStageDLRouteSlot(r->dl);
        const u32 head = r->head;

        if ((route->dl != r->dl) ||
            (ndsItemReplayRouteOk(route->route) == FALSE) ||
            (route->loaded == NULL) ||
            (route->loaded->data != route->data))
        {
            return 0u;
        }
        key[n++] = (u32)(uintptr_t)r->dobj;
        key[n++] = (u32)(uintptr_t)r->dl;
        key[n++] = head | ((u32)route->route << 8);
        key[n++] = (u32)(uintptr_t)route->data;
        key[n++] = route->root;
        if ((head_seen & (1u << head)) == 0u)
        {
            head_seen |= 1u << head;
            key[n++] = sNdsRendererAdapterItemColorMask[head];
            key[n++] = sNdsRendererAdapterItemPrimColor[head];
            key[n++] = sNdsRendererAdapterItemEnvColor[head];
            key[n++] = sNdsRendererAdapterItemOtherModeL[head];
            key[n++] = sNdsRendererAdapterItemOtherModeLValid[head];
            key[n++] = sNdsRendererAdapterItemOtherModeH[head];
            key[n++] = sNdsRendererAdapterItemOtherModeHValid[head];
        }
    }
    return n;
}

/* One item GObj's tree: replayed when a recorded draw matches, else drawn by
 * its owners inside the stage traversal (recording when every list is a
 * replayable owner). */
void ndsRendererAdapterSubmitItemDObjTreeReplay(void *dobj_ptr, u32 kind,
                                                void *camera_gobj_ptr,
                                                u32 initial_geometry_mode,
                                                u32 item_kind)
{
    DObj *root = (DObj *)dobj_ptr;
    GObj *camera_gobj = (GObj *)camera_gobj_ptr;
    NDSItemReplayRoot roots[NDS_ITEM_REPLAY_ROOTS];
    u32 key[NDS_ITEM_REPLAY_KEY_WORDS];
    NDSItemReplayDraw *draw = NULL;
    NDSItemReplayDraw *victim = &sNdsItemReplayDraws[0];
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
    const NDSItemReplayDraw *verify_against = NULL;
#endif
    sb32 same_root = FALSE;
    u32 root_count = 0u;
    u32 key_count = 0u;
    u32 i;

    if ((sNdsItemReplayRecording == NULL) && (root != NULL) &&
        (ndsRendererAdapterItemOffscreen(root, camera_gobj) == FALSE))
    {
        root_count = ndsItemReplayWalk(root, kind, roots, 0u, 0u);
        if ((root_count != 0u) && (root_count <= NDS_ITEM_REPLAY_ROOTS) &&
            ((7u + (root_count * 12u)) <= NDS_ITEM_REPLAY_KEY_WORDS))
        {
            key_count = ndsItemReplayKey(root, item_kind,
                                         initial_geometry_mode, roots,
                                         root_count, key);
        }
    }
    if (key_count != 0u)
    {
        for (i = 0u; i < NDS_ITEM_REPLAY_DRAWS; i++)
        {
            NDSItemReplayDraw *d = &sNdsItemReplayDraws[i];

            if ((d->valid != 0u) && (d->root == root) &&
                (d->key_count == key_count) && (d->root_count == root_count) &&
                (memcmp(d->key, key, key_count * sizeof(u32)) == 0))
            {
                /* valid 2: this key failed its recording -- its owners draw
                 * it, unrecorded, until the key changes. */
                draw = (d->valid == 1u) ? d : NULL;
                if (draw == NULL)
                {
                    d->last_used = gNdsRendererProfileFrameCount;
                    key_count = 0u;
                }
                break;
            }
            if (d->root == root)
            {
                victim = d;       /* this item's stale draw: replace it */
                same_root = TRUE;
            }
            else if ((same_root == FALSE) &&
                     ((d->valid == 0u) ||
                      ((victim->valid != 0u) &&
                       (d->last_used < victim->last_used))))
            {
                victim = d;
            }
        }
        if ((draw != NULL) &&
            (ndsNativeItemReplayEmitsResident(draw->emits, draw->emit_count) ==
             FALSE))
        {
            NDS_DIAG(gNdsItemReplayNotResident++);
            victim = draw;
            draw = NULL;
        }
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
        if ((draw != NULL) && (gNdsItemReplayVerify != 0u))
        {
            /* Draw through the owners into a scratch recording and compare
             * it with the cached one (below). */
            verify_against = draw;
            victim = &sNdsItemReplayVerifyDraw;
            draw = NULL;
        }
#endif
    }
    if (draw != NULL)
    {
        CObj *cobj = (camera_gobj != NULL) ? CObjGetStruct(camera_gobj) :
            ((gGCCurrentCamera != NULL) ? CObjGetStruct(gGCCurrentCamera) :
                                          NULL);
        u32 triangles = 0u;

        sNdsRendererAdapterItemSubmitActive = TRUE;
        for (i = 0u; i < draw->root_count; i++)
        {
            const NDSItemReplayRoot *r = &draw->roots[i];
            NDSRendererMatrix20p12 projection;
            NDSRendererMatrix20p12 modelview;
            NDSRendererMatrix20p12 identity;
            const NDSRendererMatrix20p12 *projection_ptr;
            const NDSRendererMatrix20p12 *modelview_ptr;
            void *saved_graphics_heap_ptr = gSYTaskmanGraphicsHeap.ptr;

#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
            u32 lab_replay_mark = cpuGetTiming();
            const u32 lab_pim0 = gNdsLabPimAcc[0];
            const u32 lab_pim1 = gNdsLabPimAcc[1];
            const u32 lab_pim2 = gNdsLabPimAcc[2];
#endif
            /* The fast lane's matrix preparation for this list, unchanged:
             * a held item's attach builds (and latches) exactly here. */
            ndsRendererAdapterPrepareInitialMatrices(
                r->dobj, cobj, TRUE, &projection, &projection_ptr,
                &modelview, &modelview_ptr);
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
            {
                const u32 lab_now = cpuGetTiming();

                NDS_DIAG(gNdsLabItemAcc[14] += lab_now - lab_replay_mark);
                NDS_DIAG(gNdsLabItemAcc[5] += gNdsLabPimAcc[0] - lab_pim0);
                NDS_DIAG(gNdsLabItemAcc[6] += gNdsLabPimAcc[1] - lab_pim1);
                NDS_DIAG(gNdsLabItemAcc[7] += gNdsLabPimAcc[2] - lab_pim2);
                lab_replay_mark = lab_now;
            }
#endif
            if ((projection_ptr == NULL) && (modelview_ptr != NULL))
            {
                ndsRendererAdapterMtxIdentity20p12(&identity);
                projection_ptr = &identity;
            }
            else if ((modelview_ptr == NULL) && (projection_ptr != NULL))
            {
                ndsRendererAdapterMtxIdentity20p12(&identity);
                modelview_ptr = &identity;
            }
            if ((projection_ptr != NULL) && (modelview_ptr != NULL))
            {
                /* The persistent stats are dead between traversals (every
                 * reader begins one first): the batch opens borrow them. */
                triangles += ndsNativeItemReplayEmits(
                    &draw->emits[r->emit_first], r->emit_count, draw->pool,
                    projection_ptr, modelview_ptr,
                    &sNdsRendererAdapterStagePersistentStats);
            }
            gSYTaskmanGraphicsHeap.ptr = saved_graphics_heap_ptr;
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
            NDS_DIAG(gNdsLabItemAcc[3] += cpuGetTiming() - lab_replay_mark);
#endif
        }
        sNdsRendererAdapterItemSubmitActive = FALSE;
        sNdsRendererAdapterItemSubmitHead = 0u;
        gNdsStageGCDrawAllLoopHardwareTriangleCount += triangles;
        NDS_DIAG(gNdsStageGCDrawAllLoopHardwareZBufferTriangleCount += triangles);
        draw->last_used = gNdsRendererProfileFrameCount;
        NDS_DIAG(gNdsItemReplayDraws++);
        return;
    }
    if (key_count != 0u)
    {
        /* Record this draw while its owners run. */
        victim->valid = 0u;
        victim->root = root;
        victim->root_count = (u8)root_count;
        victim->key_count = (u8)key_count;
        victim->emit_count = 0u;
        for (i = 0u; i < root_count; i++)
        {
            victim->roots[i] = roots[i];
            victim->roots[i].emit_first = 0u;
            victim->roots[i].emit_count = 0u;
        }
        memcpy(victim->key, key, key_count * sizeof(u32));
        sNdsItemReplaySinkState.emits = victim->emits;
        sNdsItemReplaySinkState.pool = victim->pool;
        sNdsItemReplaySinkState.emit_count = 0u;
        sNdsItemReplaySinkState.word_count = 0u;
        sNdsItemReplaySinkState.failed = 0u;
        sNdsItemReplayRecording = victim;
        sNdsItemReplayRecordRoots = 0u;
        ndsNativeItemReplaySetSink(&sNdsItemReplaySinkState);
    }
    ndsRendererAdapterBeginStageTraversal();
    ndsRendererAdapterSubmitItemDObjTree(dobj_ptr, kind, camera_gobj_ptr,
                                         initial_geometry_mode);
    ndsRendererAdapterEndStageTraversal();
    if (key_count != 0u)
    {
        ndsNativeItemReplaySetSink(NULL);
        sNdsItemReplayRecording = NULL;
        if ((sNdsItemReplaySinkState.failed == 0u) &&
            (sNdsItemReplayRecordRoots == root_count))
        {
            victim->emit_count = (u8)sNdsItemReplaySinkState.emit_count;
            victim->last_used = gNdsRendererProfileFrameCount;
            victim->valid = 1u;
            NDS_DIAG(gNdsItemReplayRecords++);
        }
        else
        {
            victim->last_used = gNdsRendererProfileFrameCount;
            victim->valid = 2u;
            NDS_DIAG(gNdsItemReplayRecordFailed++);
        }
#if defined(NDS_LAB_FOURCPU_SWEEP) && NDS_LAB_FOURCPU_SWEEP
        if (verify_against != NULL)
        {
            NDS_DIAG(gNdsItemReplayVerifyRuns++);
            if (ndsItemReplaySameRecording(victim, verify_against) == FALSE)
            {
                NDS_DIAG(gNdsItemReplayVerifyFail++);
            }
            victim->valid = 0u;
        }
#endif
    }
}
#endif

void ndsRendererAdapterSubmitWeaponDObjTree(void *dobj_ptr, u32 kind,
                                            void *camera_gobj_ptr,
                                            u32 initial_geometry_mode)
{
    ndsRendererAdapterSubmitStageDObjTreeDepth(dobj_ptr, kind, camera_gobj_ptr,
                                               initial_geometry_mode, 0u);
}

void ndsRendererAdapterSubmitStageDObj(void *dobj_ptr, u32 kind,
                                       void *camera_gobj_ptr,
                                       u32 initial_geometry_mode)
{
    ndsRendererAdapterSubmitStageDObjNode(dobj_ptr, kind, camera_gobj_ptr,
                                          initial_geometry_mode);
}

/* P2 (2026-10-04): SECTOR Z'S ARWING IN TWO PASSES.
 *
 * The Arwing draws seven FoxSpecial3 roots a frame (the body and six glow
 * quads), each through the scan, the stage DL submit, the entry-effect
 * adapter, matrix preparation and the 15 KB entry-effect executor -- 20-25 KB
 * of code per list against an 8 KB instruction cache. The lab profile of the
 * owner's 4P all-items match (artifacts/task37-census/sz-szprof01) put the
 * Arwing frames' premium at ~230K cycles, nearly all of it memory stall
 * (7-18 cycles per instruction), and a two-triangle glow cost 13-18K ticks.
 *
 * So the tree is walked once, every root's matrices are prepared in one pass
 * (the same ndsRendererAdapterPrepareInitialMatrices call, results kept per
 * list), and then every list is submitted in source order through the
 * unchanged path, which takes the prepared matrices instead of preparing them
 * again. Matrix preparation reads only the world, camera and recalc caches,
 * none of which a submit touches, so moving it ahead changes no value. The
 * walk mirrors ndsStageGCDrawAllLoopScanDObjs + SubmitStageDObjNode for
 * TREE_DLLINKS (hidden subtrees skipped, NOTEXTURE lists skipped, list ids
 * below NDS_RENDERER_STAGE_DL_HEADS); anything it cannot hold declines before
 * drawing. */
#define NDS_ARWING_TWO_PASS_MAX 12u
static NDSRendererAdapterEntryPrepared
    sNdsRendererAdapterArwingItems[NDS_ARWING_TWO_PASS_MAX];
volatile u32 gNdsArwingTwoPassDraws;
volatile u32 gNdsArwingTwoPassDeclines;

static sb32 ndsRendererAdapterIsArwingRoot(const Gfx *dl)
{
    u32 offset;

    if ((gFTDataFoxSpecial3 == NULL) ||
        ((const u8 *)dl < (const u8 *)gFTDataFoxSpecial3))
    {
        return FALSE;
    }
    offset = (u32)((const u8 *)dl - (const u8 *)gFTDataFoxSpecial3);
    switch (offset)
    {
    case 0x1fa0u:
    case 0x2920u:
    case 0x29d0u:
    case 0x29f0u:
    case 0x2a20u:
    case 0x2868u:
    case 0x2a50u:
    case 0x2b00u:
        return TRUE;
    default:
        return FALSE;
    }
}

/* A prepared Arwing root whose recorded packet replays: what the ordinary
 * submit does for it (the stage DL submit admits FoxSpecial3 lists to
 * ndsRendererAdapterTryNativeEntryEffect, whose stats seed, config and
 * counters these are) around the executor's replay branch alone
 * (ndsRendererReplayNativeEntryEffectFox). FALSE before any GX write
 * whenever that branch is not the one the executor would take; the caller
 * then takes the ordinary submit. */
static sb32 ndsRendererAdapterSubmitArwingRootReplay(
    const NDSRendererAdapterEntryPrepared *item, u32 initial_geometry_mode)
{
    NDSRendererConfig config;
    NDSRendererStats stats;
    NDSRendererMatrix20p12 identity;
    const NDSRendererMatrix20p12 *projection_ptr =
        (item->projection_valid != 0u) ? &item->projection : NULL;
    const NDSRendererMatrix20p12 *modelview_ptr =
        (item->modelview_valid != 0u) ? &item->modelview : NULL;

    if ((gFTDataFoxSpecial3 == NULL) ||
        ((const u8 *)item->dl < (const u8 *)gFTDataFoxSpecial3))
    {
        return FALSE;
    }
    /* The adapter's split-camera fill. */
    if ((projection_ptr == NULL) && (modelview_ptr != NULL))
    {
        ndsRendererAdapterMtxIdentity20p12(&identity);
        projection_ptr = &identity;
    }
    else if ((modelview_ptr == NULL) && (projection_ptr != NULL))
    {
        ndsRendererAdapterMtxIdentity20p12(&identity);
        modelview_ptr = &identity;
    }
    if ((projection_ptr == NULL) || (modelview_ptr == NULL))
    {
        return FALSE;
    }
    ndsRendererAdapterEntryEffectSeedStats(item->dobj, &stats);
    memset(&config, 0, sizeof(config));
    config.max_depth = 4u;
    config.max_commands = 1u;
    config.max_list_commands = 1u;
    config.initial_projection = projection_ptr;
    config.initial_modelview = modelview_ptr;
    config.initial_geometry_mode = initial_geometry_mode;
    config.texture_data_layout = NDS_RENDERER_TEXTURE_DATA_O2R_WORD_SWAPPED;
    if (ndsRendererReplayNativeEntryEffectFox(
            (u32)((const u8 *)item->dl - (const u8 *)gFTDataFoxSpecial3),
            &config, &stats) == FALSE)
    {
        return FALSE;
    }
    ndsRendererAdapterEntryEffectAccumulate(&stats);
    return TRUE;
}

s32 ndsRendererAdapterSubmitArwingTwoPass(void *root_ptr, void *camera_gobj_ptr,
                                          u32 initial_geometry_mode,
                                          u32 *submitted_dobjs)
{
    DObj *root = (DObj *)root_ptr;
    GObj *camera_gobj = (GObj *)camera_gobj_ptr;
    NDSRendererAdapterEntryPrepared *items = sNdsRendererAdapterArwingItems;
    DObj *stack[32];
    CObj *cobj;
    u32 stack_count = 0u;
    u32 scanned = 0u;
    u32 count = 0u;
    u32 dobjs = 0u;
    u32 i;

    if ((root == NULL) ||
        (submitted_dobjs == NULL))
    {
        return FALSE;
    }
    stack[stack_count++] = root;
    while ((stack_count != 0u) && (scanned < ARRAY_COUNT(stack)))
    {
        DObj *dobj = stack[--stack_count];

        if (dobj == NULL)
        {
            continue;
        }
        scanned++;
        /* Two pushes at most below; a walk that could drop one declines. */
        if (stack_count + 2u > ARRAY_COUNT(stack))
        {
            NDS_DIAG(gNdsArwingTwoPassDeclines++);
            return FALSE;
        }
        if ((dobj->flags & DOBJ_FLAG_HIDDEN) != 0)
        {
            if (dobj->sib_next != NULL)
            {
                stack[stack_count++] = dobj->sib_next;
            }
            continue;
        }
        if ((dobj->dv != NULL) && ((dobj->flags & DOBJ_FLAG_NOTEXTURE) == 0))
        {
            DObjDLLink *dl_link = dobj->dl_link;

            dobjs++;
            for (i = 0u; (dl_link != NULL) && (i < GC_COMMON_MAX_DLLINKS);
                 i++, dl_link++)
            {
                if (dl_link->list_id == (s32)NDS_RENDERER_STAGE_DL_HEADS)
                {
                    break;
                }
                if ((dl_link->list_id >= 0) &&
                    ((u32)dl_link->list_id < NDS_RENDERER_STAGE_DL_HEADS) &&
                    (dl_link->dl != NULL))
                {
                    if (count >= NDS_ARWING_TWO_PASS_MAX)
                    {
                        NDS_DIAG(gNdsArwingTwoPassDeclines++);
                        return FALSE;
                    }
                    items[count].dobj = dobj;
                    items[count].dl = dl_link->dl;
                    items[count].list_id = (u32)dl_link->list_id;
                    count++;
                }
            }
        }
        if (dobj->sib_next != NULL)
        {
            stack[stack_count++] = dobj->sib_next;
        }
        if (dobj->child != NULL)
        {
            stack[stack_count++] = dobj->child;
        }
    }
    if (stack_count != 0u)
    {
        /* More DObjs than the walk holds: let the scan draw it. */
        NDS_DIAG(gNdsArwingTwoPassDeclines++);
        return FALSE;
    }

    cobj = (camera_gobj != NULL) ? CObjGetStruct(camera_gobj) :
        ((gGCCurrentCamera != NULL) ? CObjGetStruct(gGCCurrentCamera) : NULL);
    for (i = 0u; i < count; i++)
    {
        NDSRendererAdapterEntryPrepared *item = &items[i];
        const NDSRendererMatrix20p12 *projection_ptr = NULL;
        const NDSRendererMatrix20p12 *modelview_ptr = NULL;

        item->prepared = FALSE;
        if (ndsRendererAdapterIsArwingRoot(item->dl) == FALSE)
        {
            continue;
        }
        ndsRendererAdapterPrepareInitialMatrices(
            item->dobj, cobj, FALSE, &item->projection, &projection_ptr,
            &item->modelview, &modelview_ptr);
        item->projection_valid = (projection_ptr != NULL) ? 1u : 0u;
        item->modelview_valid = (modelview_ptr != NULL) ? 1u : 0u;
        item->prepared = TRUE;
    }
    for (i = 0u; i < count; i++)
    {
        NDSRendererAdapterEntryPrepared *item = &items[i];

        sNdsRendererAdapterEffectSubmitHead = item->list_id;
        if ((item->prepared != FALSE) &&
            (ndsRendererAdapterSubmitArwingRootReplay(
                 item, initial_geometry_mode) != FALSE))
        {
            continue;
        }
        sNdsRendererAdapterEntryPrepared =
            (item->prepared != FALSE) ? item : NULL;
        ndsRendererAdapterSubmitStageDL(item->dobj, item->dl, camera_gobj,
                                        initial_geometry_mode);
        sNdsRendererAdapterEntryPrepared = NULL;
    }
    *submitted_dobjs = dobjs;
    NDS_DIAG(gNdsArwingTwoPassDraws++);
    return TRUE;
}

static void ndsRendererAdapterSubmitStageDObjNode(DObj *dobj, u32 kind,
                                                  GObj *camera_gobj,
                                                  u32 initial_geometry_mode)
{
    DObjDLLink *dl_link;
    u32 i;

    if (ndsRendererAdapterStageDObjDrawable(dobj, kind) == FALSE)
    {
        return;
    }

    switch (kind)
    {
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD0:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1:
        if (dobj->dv != NULL)
        {
            if (sNdsRendererAdapterItemSubmitActive != FALSE)
            {
                sNdsRendererAdapterItemSubmitHead =
                    (kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1) ?
                        1u : 0u;
            }
            sNdsRendererAdapterEffectSubmitHead =
                (kind == NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLHEAD1) ? 1u : 0u;
            ndsRendererAdapterSubmitStageDL(dobj, dobj->dl, camera_gobj,
                                            initial_geometry_mode);
        }
        break;

    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_TREE_DLLINKS:
    case NDS_OPENING_ROOM_DRAW_CALLBACK_DOBJ_DLLINKS:
        dl_link = dobj->dl_link;
        if (dl_link == NULL)
        {
            return;
        }
        for (i = 0u; i < GC_COMMON_MAX_DLLINKS; i++, dl_link++)
        {
            if (dl_link->list_id == (s32)NDS_RENDERER_STAGE_DL_HEADS)
            {
                break;
            }
            if ((dl_link->list_id >= 0) &&
                ((u32)dl_link->list_id < NDS_RENDERER_STAGE_DL_HEADS) &&
                (dl_link->dl != NULL))
            {
                if (sNdsRendererAdapterItemSubmitActive != FALSE)
                {
                    sNdsRendererAdapterItemSubmitHead =
                        (u32)dl_link->list_id;
                }
                sNdsRendererAdapterEffectSubmitHead = (u32)dl_link->list_id;
                ndsRendererAdapterSubmitStageDL(dobj, dl_link->dl,
                                                camera_gobj,
                                                initial_geometry_mode);
            }
        }
        break;

    default:
        break;
    }
}

#else
s32 ndsRendererAdapterNdlDispatchEffect(void *camera_gobj,
                                        void *display_gobj,
                                        s32 link_id)
{
    (void)camera_gobj;
    (void)display_gobj;
    (void)link_id;
    return FALSE;
}

void ndsRendererAdapterBeginStageTraversal(void)
{
}

void ndsRendererAdapterEndStageTraversal(void)
{
}

void ndsRendererAdapterSubmitStageDObj(void *dobj, u32 kind,
                                       void *camera_gobj,
                                       u32 initial_geometry_mode)
{
    (void)dobj;
    (void)kind;
    (void)camera_gobj;
    (void)initial_geometry_mode;
}

void ndsRendererAdapterSubmitEffectDObjTree(void *dobj, u32 kind,
                                            void *camera_gobj,
                                            u32 initial_geometry_mode)
{
    (void)dobj;
    (void)kind;
    (void)camera_gobj;
    (void)initial_geometry_mode;
}

void ndsRendererAdapterSubmitWeaponDObjTree(void *dobj, u32 kind,
                                            void *camera_gobj,
                                            u32 initial_geometry_mode)
{
    (void)dobj;
    (void)kind;
    (void)camera_gobj;
    (void)initial_geometry_mode;
}

void ndsRendererAdapterSubmitItemDObjTree(void *dobj, u32 kind,
                                          void *camera_gobj,
                                          u32 initial_geometry_mode)
{
    (void)dobj;
    (void)kind;
    (void)camera_gobj;
    (void)initial_geometry_mode;
}

void ndsRendererAdapterSubmitItemDObjTreeReplay(void *dobj, u32 kind,
                                                void *camera_gobj,
                                                u32 initial_geometry_mode,
                                                u32 item_kind)
{
    (void)item_kind;
    ndsRendererAdapterBeginStageTraversal();
    ndsRendererAdapterSubmitItemDObjTree(dobj, kind, camera_gobj,
                                         initial_geometry_mode);
    ndsRendererAdapterEndStageTraversal();
}

void ndsRendererAdapterMarkDisplayProcHeads(void)
{
}

void ndsRendererAdapterCaptureDisplayProcColors(void)
{
}

void ndsRendererAdapterCaptureItemDisplayProcState(void)
{
}

#endif
