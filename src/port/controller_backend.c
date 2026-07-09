#include <nds.h>
#include <string.h>

#include <PR/os.h>
#include <nds/nds_boot.h>
#include <nds/nds_controller.h>
#include <nds/nds_platform.h>

static OSContStatus sControllerStatus[MAXCONTROLLERS];
static NDSControllerPlaybackPad sControllerPlaybackPads[MAXCONTROLLERS];
static u32 sControllerPlaybackConnectedMask;
static sb32 sControllerPlaybackEnabled;

volatile u32 gNdsControllerPollCount;
volatile u32 gNdsControllerPlaybackEnabled;
volatile u32 gNdsControllerPlaybackConnectedMask;
volatile u32 gNdsControllerPlaybackFrameCount;
volatile u32 gNdsControllerPlaybackReadCount;
volatile u32 gNdsControllerLiveReadCount;
volatile u32 gNdsControllerLiveConnectedMask;
volatile u32 gNdsControllerLivePad0Button;
volatile s32 gNdsControllerLivePad0StickX;
volatile s32 gNdsControllerLivePad0StickY;
volatile u32 gNdsControllerLivePad1Button;
volatile s32 gNdsControllerLivePad1StickX;
volatile s32 gNdsControllerLivePad1StickY;
volatile u32 gNdsControllerLiveMapCount;
volatile u32 gNdsControllerPlaybackPad0Button;
volatile u32 gNdsControllerPlaybackPad1Button;
volatile s32 gNdsControllerPlaybackPad0StickX;
volatile s32 gNdsControllerPlaybackPad1StickX;
volatile s32 gNdsControllerPlaybackPad0StickY;
volatile s32 gNdsControllerPlaybackPad1StickY;

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

static void ndsControllerRefreshStatusForPlayback(void)
{
    s32 i;

    memset(sControllerStatus, 0, sizeof(sControllerStatus));
    for (i = 0; i < MAXCONTROLLERS; i++) {
        if (((sControllerPlaybackConnectedMask >> i) & 1u) != 0u) {
            sControllerStatus[i].type = CONT_TYPE_NORMAL;
        } else {
            sControllerStatus[i].errno = CONT_NO_RESPONSE_ERROR;
        }
    }
}

void ndsControllerPlaybackReset(void)
{
    memset(sControllerPlaybackPads, 0, sizeof(sControllerPlaybackPads));
    sControllerPlaybackEnabled = FALSE;
    sControllerPlaybackConnectedMask = 0u;
    gNdsControllerPlaybackEnabled = 0u;
    gNdsControllerPlaybackConnectedMask = 0u;
    gNdsControllerPlaybackFrameCount = 0u;
    gNdsControllerPlaybackReadCount = 0u;
    gNdsControllerLiveReadCount = 0u;
    gNdsControllerLiveConnectedMask = 0u;
    gNdsControllerLivePad0Button = 0u;
    gNdsControllerLivePad0StickX = 0;
    gNdsControllerLivePad0StickY = 0;
    gNdsControllerLivePad1Button = 0u;
    gNdsControllerLivePad1StickX = 0;
    gNdsControllerLivePad1StickY = 0;
    gNdsControllerLiveMapCount = 0u;
    gNdsControllerPlaybackPad0Button = 0u;
    gNdsControllerPlaybackPad1Button = 0u;
    gNdsControllerPlaybackPad0StickX = 0;
    gNdsControllerPlaybackPad1StickX = 0;
    gNdsControllerPlaybackPad0StickY = 0;
    gNdsControllerPlaybackPad1StickY = 0;
}

void ndsControllerPlaybackSetEnabled(sb32 is_enabled)
{
    sControllerPlaybackEnabled = (is_enabled != FALSE) ? TRUE : FALSE;
    gNdsControllerPlaybackEnabled =
        (sControllerPlaybackEnabled != FALSE) ? 1u : 0u;
}

void ndsControllerPlaybackSetConnectedMask(u32 mask)
{
    sControllerPlaybackConnectedMask = mask & ((1u << MAXCONTROLLERS) - 1u);
    gNdsControllerPlaybackConnectedMask = sControllerPlaybackConnectedMask;
    if (sControllerPlaybackEnabled != FALSE) {
        ndsControllerRefreshStatusForPlayback();
    }
}

void ndsControllerPlaybackSetPad(u32 slot, u16 button, s8 stick_x,
                                 s8 stick_y)
{
    if (slot >= MAXCONTROLLERS) {
        return;
    }
    sControllerPlaybackPads[slot].button = button;
    sControllerPlaybackPads[slot].stick_x = stick_x;
    sControllerPlaybackPads[slot].stick_y = stick_y;
    sControllerPlaybackPads[slot].errno_value =
        (((sControllerPlaybackConnectedMask >> slot) & 1u) != 0u) ?
        0u : CONT_NO_RESPONSE_ERROR;

    if (slot == 0u) {
        gNdsControllerPlaybackPad0Button = button;
        gNdsControllerPlaybackPad0StickX = stick_x;
        gNdsControllerPlaybackPad0StickY = stick_y;
    } else if (slot == 1u) {
        gNdsControllerPlaybackPad1Button = button;
        gNdsControllerPlaybackPad1StickX = stick_x;
        gNdsControllerPlaybackPad1StickY = stick_y;
    }
}

void ndsControllerPlaybackCommitFrame(void)
{
    if (sControllerPlaybackEnabled != FALSE) {
        gNdsControllerPlaybackFrameCount++;
    }
}

s32 osContInit(OSMesgQueue *queue, u8 *controller_bits,
               OSContStatus *status)
{
    s32 i;

    (void)queue;
    if (sControllerPlaybackEnabled != FALSE) {
        ndsControllerRefreshStatusForPlayback();
    } else {
        memset(sControllerStatus, 0, sizeof(sControllerStatus));
        sControllerStatus[0].type = CONT_TYPE_NORMAL;
        sControllerStatus[1].type = CONT_TYPE_NORMAL;
        for (i = 2; i < MAXCONTROLLERS; i++) {
            sControllerStatus[i].errno = CONT_NO_RESPONSE_ERROR;
        }
    }
    if (controller_bits != NULL) {
        *controller_bits = (sControllerPlaybackEnabled != FALSE) ?
            (u8)sControllerPlaybackConnectedMask : 3;
    }
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
    if (sControllerPlaybackEnabled != FALSE) {
        ndsControllerRefreshStatusForPlayback();
    }
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
    u32 keys;
    s32 i;

    if (pad == NULL) return;
    memset(pad, 0, sizeof(*pad) * MAXCONTROLLERS);
    if (sControllerPlaybackEnabled != FALSE) {
        for (i = 0; i < MAXCONTROLLERS; i++) {
            if (((sControllerPlaybackConnectedMask >> i) & 1u) != 0u) {
                pad[i].button = sControllerPlaybackPads[i].button;
                pad[i].stick_x = sControllerPlaybackPads[i].stick_x;
                pad[i].stick_y = sControllerPlaybackPads[i].stick_y;
                pad[i].errno = 0u;
            } else {
                pad[i].errno = CONT_NO_RESPONSE_ERROR;
            }
        }
        gNdsControllerPlaybackReadCount++;
        gNdsControllerPollCount++;
        return;
    }
    keys = ndsPlatformHeldKeys();
    ndsControllerMapPad(keys, &pad[0]);
    pad[0].errno = 0u;
    pad[1].errno = 0u;

    for (i = 2; i < MAXCONTROLLERS; i++) {
        pad[i].errno = CONT_NO_RESPONSE_ERROR;
    }
    gNdsControllerLiveConnectedMask = 3u;
    gNdsControllerLivePad0Button = pad[0].button;
    gNdsControllerLivePad0StickX = pad[0].stick_x;
    gNdsControllerLivePad0StickY = pad[0].stick_y;
    gNdsControllerLivePad1Button = pad[1].button;
    gNdsControllerLivePad1StickX = pad[1].stick_x;
    gNdsControllerLivePad1StickY = pad[1].stick_y;
    gNdsControllerLiveMapCount++;
    gNdsControllerLiveReadCount++;
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
