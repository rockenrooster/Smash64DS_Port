/* Bounded Planet Zebes (Zebes) ground setup import, P2-4 stage 4.
 *
 * Mirrors the three stage wrappers beside it: this translation unit imports
 * decomp gr/grcommon/grzebes.c verbatim, so the acid's five-state rise cycle
 * is mechanically equivalent to the source, and exposes ndsGRZebesSetupInitAll
 * for the shared grCommonSetupInitAll gate. Only built when
 * NDS_P2_STAGE_ZEBES=1.
 *
 * WHAT MAKES THIS STAGE DIFFERENT. Dream Land's wind pushes velocity, Yoshi's
 * Island's clouds are collision, Peach's Castle's bumper is an item, and Congo
 * Jungle's cannon captures the fighter. Planet Zebes' acid DAMAGES a fighter
 * in place: grZebesAcidCheckGetDamageKind (grzebes.c:225) is registered with
 * ftMainCheckAddGroundHazard (grzebes.c:219), the ground-HAZARD seam rather
 * than the ground-OBSTACLE one, and it hands back the map's own GRAttackColl.
 * That seam did not exist in this port until P2-4h2 imported it; Mushroom
 * Kingdom's POW block is its only other caller.
 */
#if NDS_P2_STAGE_ZEBES

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

/* Forward declarations mirroring the other stage wrappers: the gc*, sy* and
 * ef* names live in decomp headers this TU does not otherwise pull in.
 * Signatures are the decomp ones verbatim. */
void gcAddMObjAll(GObj *gobj, MObjSub ***p_mobjsubs);
void gcPlayAnimAll(GObj *gobj);
s32 syUtilsRandIntRange(s32 range);
f32 syUtilsRandFloat(void);
GObj *efManagerQuakeMakeEffect(s32 id);

/* Offsets from decomp reloc_data.us.h:3898-3903, the authoritative symbol
 * table. Zebes has no map-head entry there: unlike Yoshi's Island and Congo
 * Jungle, its acid is reached through its own DObj descriptor rather than
 * through a joint root offset from a map base. */
#define NDS_RELOC_LVALUE(offset) (*(uintptr_t *)(uintptr_t)(offset))
/* THE MAP HEADER AS AN OFFSET, NOT AS A VARIABLE.
 *
 * The source subtracts `&ll<Stage>MapMapHeader` from gMPCollisionGroundData
 * to recover the file base, which works there because the symbol is a
 * link-time constant equal to its offset. Here it is a real uintptr_t in
 * .data, so `&` yields its RAM ADDRESS and the subtraction produces a wild
 * pointer -- and ndsRelocGetFileData hands an unrecognised file back
 * unchanged, so the wild pointer travels on rather than failing. Planet
 * Zebes aborted on exactly that: its acid handed a GRAttackColl of
 * 0x0019e214 to ftMainUpdateDamageStatGround, which loaded [r3, #4].
 *
 * Shadowing it with the offset restores the source's arithmetic. All nine
 * stage map files carry their header at 0x14 (reloc_data.us.h:3845-4030). */
#define llGRZebesMapMapHeader NDS_RELOC_LVALUE(0x14u)
#define llGRZebesMapAcidGRAttackColl NDS_RELOC_LVALUE(0xbcu)
#define llGRZebesMapAcidMObjSub NDS_RELOC_LVALUE(0x8c0u)
#define llGRZebesMapAcidDObjDesc NDS_RELOC_LVALUE(0xb08u)
#define llGRZebesMapAcidAnimJoint NDS_RELOC_LVALUE(0xb90u)
#define llGRZebesMapAcidMatAnimJoint NDS_RELOC_LVALUE(0xbd0u)

void ndsBaseGRCommonSetupInitAll(void);

#include "../../decomp/BattleShip-main/decomp/src/gr/grcommon/grzebes.c"

/* Gameplay transcription notes (all numeric behaviour is the included source,
 * cited per constant -- nothing below re-states a number):
 *
 * - Acid cycle, five states dispatched by grZebesProcUpdate (grzebes.c:190):
 *   Wait (:118) leaves the intro, Normal (:139) counts its wait down, Shake
 *   (:152) runs its rumble window, Rise (:167) accumulates the level step into
 *   the acid DObj's y over the source's own frame count, and Rumble (:127)
 *   spawns the quake effect.
 * - The per-cycle target, minimum, maximum and level come from
 *   dGRZebesAcidAttributes (grzebes.c:13), sixteen rows the cycle walks.
 * - Damage: grZebesAcidCheckGetDamageKind (grzebes.c:225) tests the fighter's
 *   y against the acid's, gated on the fighter's own acid_wait, and yields
 *   nGMHitEnvironmentAcid with the map's GRAttackColl.
 * - Bounds come from the source MPGroundData, not from any port constant:
 *   camera 4700/-2400/4500/-4500, blast 9000/-4200/9500/-9500
 *   (257_GRZebesMap.c:50-76).
 *
 * PRESENTATION ADAPTATIONS (gameplay untouched):
 * - The acid surface, Ridley and the background draw through the port's
 *   existing DObj renderer rather than a Zebes-specific native packet; the law
 *   8 packet for every stage is one pipeline job (P2-4n1).
 */
void ndsGRZebesSetupInitAll(void)
{
    /* Same contract as the Pupupu, Yoster, Castle and Jungle arms of
     * grCommonSetupInitAll: the caller has already checked
     * gkind == nGRKindZebes, the collision ground data, and the stage-ready
     * flag. */
    ndsBaseGRCommonSetupInitAll();
}

#endif /* NDS_P2_STAGE_ZEBES */
