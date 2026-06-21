#include <nds.h>
#include <string.h>

#include <PR/os.h>
#include <nds/nds_boot.h>
#include <nds/nds_controller.h>
#include <nds/nds_platform.h>

static OSContStatus sControllerStatus[MAXCONTROLLERS];

volatile u32 gNdsControllerPollCount;

static u16 ndsControllerMapButtons(u32 keys)
{
    u16 buttons = 0;

    if (keys & KEY_A) buttons |= A_BUTTON;
    if (keys & KEY_B) buttons |= B_BUTTON;
    if (keys & (KEY_X | KEY_Y)) buttons |= U_CBUTTONS;
    if (keys & KEY_L) buttons |= Z_TRIG;
    if (keys & KEY_R) buttons |= R_TRIG;
    if (keys & KEY_START) buttons |= START_BUTTON;
    return buttons;
}

static void ndsControllerMapPad(u32 keys, OSContPad *pad)
{
    memset(pad, 0, sizeof(*pad));
    pad->button = ndsControllerMapButtons(keys);
    if (keys & KEY_LEFT) pad->stick_x = -80;
    if (keys & KEY_RIGHT) pad->stick_x = 80;
    if (keys & KEY_DOWN) pad->stick_y = -80;
    if (keys & KEY_UP) pad->stick_y = 80;
}

int ndsControllerBackendSelfTest(void)
{
    OSContPad pad;

    ndsControllerMapPad(KEY_A | KEY_B | KEY_X | KEY_L | KEY_R |
                        KEY_START | KEY_LEFT | KEY_UP, &pad);
    if (pad.button != (A_BUTTON | B_BUTTON | U_CBUTTONS | Z_TRIG |
                       R_TRIG | START_BUTTON)) return 1;
    if (pad.stick_x != -80 || pad.stick_y != 80) return 2;

    ndsControllerMapPad(KEY_Y | KEY_RIGHT | KEY_DOWN, &pad);
    if (pad.button != U_CBUTTONS) return 3;
    if (pad.stick_x != 80 || pad.stick_y != -80) return 4;
    return 0;
}

static void ndsControllerComplete(OSMesgQueue *queue)
{
    if (queue != NULL) {
        osSendMesg(queue, (OSMesg)1, OS_MESG_NOBLOCK);
    }
}

s32 osContInit(OSMesgQueue *queue, u8 *controller_bits,
               OSContStatus *status)
{
    s32 i;

    (void)queue;
    memset(sControllerStatus, 0, sizeof(sControllerStatus));
    sControllerStatus[0].type = CONT_TYPE_NORMAL;
    for (i = 1; i < MAXCONTROLLERS; i++) {
        sControllerStatus[i].errno = CONT_NO_RESPONSE_ERROR;
    }
    if (controller_bits != NULL) *controller_bits = 1;
    if (status != NULL) {
        memcpy(status, sControllerStatus, sizeof(sControllerStatus));
    }
    gNdsOriginalBootStage |= NDS_BOOT_CONTROLLER_READY;
    return 0;
}

s32 osContStartQuery(OSMesgQueue *queue)
{
    ndsControllerComplete(queue);
    return 0;
}

void osContGetQuery(OSContStatus *status)
{
    if (status != NULL) {
        memcpy(status, sControllerStatus, sizeof(sControllerStatus));
    }
}

s32 osContStartReadData(OSMesgQueue *queue)
{
    ndsControllerComplete(queue);
    return 0;
}

void osContGetReadData(OSContPad *pad)
{
    u32 keys = ndsPlatformHeldKeys();
    s32 i;

    if (pad == NULL) return;
    memset(pad, 0, sizeof(*pad) * MAXCONTROLLERS);
    ndsControllerMapPad(keys, &pad[0]);

    for (i = 1; i < MAXCONTROLLERS; i++) {
        pad[i].errno = CONT_NO_RESPONSE_ERROR;
    }
    gNdsControllerPollCount++;
}

s32 osMotorInit(OSMesgQueue *queue, OSPfs *pfs, int channel)
{
    (void)queue;
    if (pfs != NULL) pfs->channel = channel;
    return -1;
}

s32 osMotorStart(OSPfs *pfs)
{
    (void)pfs;
    return -1;
}

s32 osMotorStop(OSPfs *pfs)
{
    (void)pfs;
    return -1;
}
