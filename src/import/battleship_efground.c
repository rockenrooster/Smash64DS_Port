/* Whole-TU import of decomp ef/efground.c: the seven VS stages' flyover and
 * background actors (dEFGroundDatas, efground.c:991-1056: Castle Lakitu,
 * Sector Z rocket and ship, Jungle bird, Zebes Ridley and ship, Yoster clouds
 * and birds, Dream Land Bronto Burt and King Dedede, Saffron City Fearow and
 * friends). grcommonsetup.c:34 calls efGroundMakeAppearActor at every stage
 * start; the port answered it with a witness stub
 * (reloc_backend_compat_shims.c:17559, strong) that must be DELETED now that
 * this TU defines the real symbol, or the link fails twice-defined.
 *
 * The source bodies stay verbatim below; the adaptations above the include
 * are the seams the other whole-TU imports adapt:
 *
 * - Types: none. include/ef/effect.h:29-66 already spells EFDesc,
 *   EFGroundParam, EFGroundDesc, EFGroundData and EFGroundActor exactly as
 *   decomp ef/eftypes.h:11-67 does, and the remaining headers (efvars,
 *   efground, sys/obj) resolve to the decomp tree through the build's
 *   include path. No shared header was widened.
 * - Symbols no port header publishes are declared locally with the decomp
 *   line, the battleship_efmanager.c:103-126 / battleship_item_box.c:74-87
 *   shape. Signatures are the decomp ones verbatim. The include block above
 *   mirrors the sibling stage-ground TUs (e.g. battleship_grzebes_ground.c)
 *   so the gc*/lbCommon*/sy* names reach this TU through the same headers.
 * - The 75 ll* map-file offsets this source takes the address of
 *   (dEFGroundDatas at :991-1056, the descs at :27-988) are two-arg X rows in
 *   include/reloc_data.h beside each map's existing rows, values from
 *   decomp/BattleShip-main/tools/reloc_data_symbols.us.txt. Those rows are
 *   real uintptr_t variables HOLDING the offset, so the source's `&llFoo`
 *   spelling yields a RAM address, not the offset -- the exact trap
 *   battleship_efmanager.c:990-1035 documents for efmanager.c's 182
 *   references. ndsEFGroundResolveOffsets below recovers each offset with
 *   the same one-dereference guard (there, ndsEFManagerResolveOffset), over
 *   every desc's four o_* fields and every EFGroundData.o_data, idempotent
 *   so re-running it per stage start is free. It runs BEFORE the renamed
 *   source body: the body derives file_head from o_data at :1594, so
 *   resolving after it would leave file_head corrupt.
 * - Display: efGroundMakeEffect installs each desc's proc_display with
 *   gcAddGObjDisplay at :1406, and every one of the 24 descs names
 *   gcDrawDObjTreeForGObj on DL link 4. The DS hardware effect-submit gate
 *   (reloc_backend_movement.c:11887) admits nGCCommonKindEffect GObjs only
 *   on links 2/10/15/18/20, so link-4 ground-effect trees are excluded from
 *   that path today. Finding only; this TU does not work around it.
 */
#include <PR/gbi.h>
#include <PR/os.h>
#include <PR/ultratypes.h>
#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <gr/ground.h>
#include <if/interface.h>
#include <it/item.h>
#include <mn/menu.h>
#include <nds/nds_renderer.h>
#include <nds/nds_startup.h>
#include <reloc_data.h>
#include <sc/scene.h>
#include <sys/obj.h>
#include <sys/objhelper.h>
#include <sys/objman.h>
#include <sys/rdp.h>
#include <sys/taskman.h>

/* decomp ef/efmanager.h:10 */
extern EFStruct *efManagerGetEffectNoForce(void);
/* decomp ef/efmanager.h:12 */
extern void efManagerSetPrevStructAlloc(EFStruct *ep);
/* decomp ef/efmanager.h:18 */
extern void efManagerFuncRun(GObj *effect_gobj);
/* decomp lb/lbcommon.h:82 */
extern void lbCommonAddMObjForTreeDObjs(DObj *root_dobj,
                                        MObjSub ***p_mobjsubs);
/* decomp lb/lbcommon.h:47 */
extern void lbCommonAddTreeDObjsAnimAll(DObj *root_dobj,
                                        AObjEvent32 **anim_joints,
                                        AObjEvent32 ***p_matanim_joints,
                                        f32 anim_frame);
/* decomp sys/objanim.h:132 */
extern void gcAddDObj3TransformsKind(DObj *dobj, u8 tk1, u8 tk2, u8 tk3);
/* decomp sys/utils.h:19 */
extern f32 syUtilsRandFloat(void);
/* decomp sys/utils.h:20 */
extern s32 syUtilsRandIntRange(s32 range);

#define efGroundMakeAppearActor ndsBaseEFGroundMakeAppearActor

#include "../../decomp/BattleShip-main/decomp/src/ef/efground.c"

#undef efGroundMakeAppearActor

/* One dereference recovers the file offset a `&llFoo` spelling left as a RAM
 * address; anything below 0x01000000 is already an offset (0 passes through,
 * and most MObjSub/MatAnim slots are literal 0x0). Same guard as
 * ndsEFManagerResolveOffset (battleship_efmanager.c:1024-1035). */
static intptr_t ndsEFGroundResolveOffset(intptr_t value)
{
    if ((uintptr_t)value >= 0x01000000u)
    {
        return (intptr_t) *(uintptr_t *)value;
    }
    return value;
}

/* Desc counts per dEFGroundDatas index (Castle, Sector, Jungle, Zebes,
 * Hyrule, Yoster, Pupupu, Yamabuki), counted from the desc initializers at
 * efground.c:27, :152, :109, :351, :433, :671 and :831. Hyrule is 0: its
 * entry is NULL params / NULL descs at :1025-1031. */
static const u8 sNdsEFGroundDescCounts[8] = { 2, 5, 1, 2, 0, 6, 4, 4 };

static void ndsEFGroundResolveOffsets(void)
{
    s32 i;
    u32 j;

    for (i = 0; i < 8; i++)
    {
        EFGroundData *data = &dEFGroundDatas[i];
        EFGroundDesc *descs;

        data->o_data = ndsEFGroundResolveOffset(data->o_data);
        descs = data->effect_descs;

        if (descs == NULL)
        {
            continue;
        }
        for (j = 0; j < sNdsEFGroundDescCounts[i]; j++)
        {
            descs[j].effect_desc.o_dobjsetup =
                ndsEFGroundResolveOffset(descs[j].effect_desc.o_dobjsetup);
            descs[j].effect_desc.o_mobjsub =
                ndsEFGroundResolveOffset(descs[j].effect_desc.o_mobjsub);
            descs[j].effect_desc.o_anim_joint =
                ndsEFGroundResolveOffset(descs[j].effect_desc.o_anim_joint);
            descs[j].effect_desc.o_matanim_joint =
                ndsEFGroundResolveOffset(descs[j].effect_desc.o_matanim_joint);
        }
    }
}

void efGroundMakeAppearActor(void)
{
    ndsEFGroundResolveOffsets();
    ndsBaseEFGroundMakeAppearActor();
}
