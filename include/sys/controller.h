#ifndef SSB64_NDS_SYS_CONTROLLER_H
#define SSB64_NDS_SYS_CONTROLLER_H

#include <PR/os.h>
#include <ssb_types.h>

typedef struct SYController {
    u16 button_hold;
    u16 button_tap;
    u16 button_update;
    u16 button_release;
    Vec2b stick_range;
} SYController;

void syControllerThreadMain(void *arg);
void syControllerInitRumble(s32 controller);
void syControllerStopRumble(s32 controller);
void syControllerFuncRead(void);
void syControllerUpdateGlobalData(void);
void syControllerSetStatusDelay(s32 delay);
void syControllerScheduleRead(void);
void syControllerSetAutoRead(s32 is_scheduled);

extern u32 gSYControllerConnectedNum;
extern SYController gSYControllerDevices[MAXCONTROLLERS];
extern SYController gSYControllerMain;
extern s8 gSYControllerDeviceStatuses[MAXCONTROLLERS];

#endif
