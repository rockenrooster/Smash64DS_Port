#ifndef NDS_SHIELD_POSE_H
#define NDS_SHIELD_POSE_H

#include <ft/fighter.h>

/* P2-2 compact Guard pose owner. Return values are deliberately tri-state:
 *  1 = native package handled the operation;
 *  0 = this is not one of the native base-fighter packages, use BattleShip;
 * -1 = a native package was selected but its generated contract was invalid.
 * The -1 case is fail-closed: never reinterpret compact u16 handles as source
 * AObjEvent32 pointers. */
s32 ndsShieldPoseResolveExternalFixup(u32 owner_asset, u32 dep_asset,
                                      u32 target_offset, void **resolved);
/* FPC compact Main keeps source Main offsets but NULLs dependencies outside
 * its compact sections. Restore the generated nine Main->ShieldPose pointers
 * from the native package after the FPC has been normalized. */
s32 ndsShieldPosePatchCompactMain(s32 fkind, void *main_data, u32 main_bytes);
s32 ndsShieldPoseReplacesSourceFile(s32 fkind);
s32 ndsShieldPoseTryApplySingle(DObj *dobj, f32 angle);
s32 ndsShieldPoseTryApplyAll(DObj *root_dobj, f32 angle);
s32 ndsShieldPoseTryPlayBatch(GObj *fighter_gobj);

extern volatile u32 gNdsShieldPoseNativeFixupCount;
extern volatile u32 gNdsShieldPoseNativeFixupRejectCount;
extern volatile u32 gNdsShieldPoseSingleApplyCount;
extern volatile u32 gNdsShieldPoseAllApplyCount;
extern volatile u32 gNdsShieldPoseBatchPlayCount;
extern volatile u32 gNdsShieldPoseDecodeFailCount;
extern volatile u32 gNdsShieldPoseLoadCount;
extern volatile u32 gNdsShieldPoseLoadFailCount;
extern volatile u32 gNdsShieldPoseResidentBytes;

#endif
