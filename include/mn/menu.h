#ifndef SSB64_NDS_MENU_H
#define SSB64_NDS_MENU_H

#include <reloc_data.h>
#include <mn/mntypes.h>
#include <sys/audio.h>
#include <sys/obj.h>
#include <sys/objdef.h>
#include <sys/objman.h>
#include <sys/objhelper.h>
#include <sys/rdp.h>
#include <sys/taskman.h>

/* Port-convenience function-pointer typedefs. The original BattleShip headers
 * spell these as `void (*)(GObj*)` inline; these aliases keep the port's
 * scene/backend code readable without changing the ABI. */
typedef void (*GObjFunc)(GObj*);
typedef void (*GObjDisplayFunc)(GObj*);

void gcSleepCurrentGObjThread(s32 tics);
void lbCommonDrawSprite(GObj *gobj);
void lbCommonDrawSObjAttr(GObj *gobj);
void lbCommonDrawSObjNoAttr(GObj *gobj);
void lbCommonPrepSObjAttr(Gfx **dls, SObj *sobj);
void lbCommonPrepSObjDraw(Gfx **dls, SObj *sobj);
void lbCommonClearExternSpriteParams(void);
void ndsSObjPreviewBeginFrame(void);
void ndsSObjPreviewEndFrame(void);
void lbCommonSetExternSpriteParams(Sprite *sprite);
SObj *lbCommonMakeSObjForGObj(GObj *gobj, Sprite *sprite);
GObj *lbCommonMakeSpriteGObj(u32 id, void (*func_run)(GObj *), s32 link,
                             u32 link_priority,
                             void (*proc_display)(GObj *), s32 dl_link,
                             u32 dl_link_priority, u32 camera_tag,
                             Sprite *sprite, u8 gobjproc_kind,
                             void (*proc)(GObj *), u32 gobjproc_priority);
DObj *lbCommonGetTreeDObjNextFromRoot(DObj *dobj, DObj *root);
void lbFadeMakeActor(u32 id, u8 link, u32 priority, SYColorRGBA *color,
                     s32 duration, ub8 is_reverse, void *callback);
void lbBackupWrite(void);

void mn1PModeStartScene(void);
void mnBackupClearStartScene(void);
void mnCharactersStartScene(void);
void mnCongraStartScene(void);
void mnDataStartScene(void);
void mnMapsStartScene(void);
void mnMessageStartScene(void);
void mnModeSelectStartScene(void);
void mnNoControllerStartScene(void);
void mnOptionStartScene(void);
void mnPlayers1PBonusStartScene(void);
void mnPlayers1PGameContinueStartScene(void);
void mnPlayers1PGameStartScene(void);
void mnPlayers1PTrainingStartScene(void);
void mnPlayersVSStartScene(void);
void mnScreenAdjustStartScene(void);
void mnSoundTestStartScene(void);
void mnStartupLogoThreadUpdate(GObj *gobj);
void mnStartupActorFuncRun(GObj *gobj);
void mnStartupFuncStart(void);
void mnStartupFuncLights(Gfx **dls);
void mnStartupStartScene(void);
void mnTitleStartScene(void);
void mnTitleLoadFiles(void);
void ndsMNTitleRunBoundedUpdates(u32 count);
void mnUnusedFightersStartScene(void);
void mnVSItemSwitchStartScene(void);
void mnVSModeStartScene(void);
void mnVSOptionsStartScene(void);
void mnVSRecordStartScene(void);
void mnVSResultsStartScene(void);
void ndsMNVSResultsRecordFrame(void);

#endif
