#include <nds.h>

#include <nds/nds_platform.h>
#include <port/port_probe.h>
#include <sys/utils.h>
#include <sys/vector.h>
#include <sys/controller.h>

typedef struct PortProbeState {
    Vec3f position;
    u16 color;
} PortProbeState;

static PortProbeState sProbe;

static void portProbeReset(void)
{
    sProbe.position.x = 128.0f;
    sProbe.position.y = 96.0f;
    sProbe.position.z = 0.0f;
    sProbe.color = RGB15(0, 16, 18);
}

void portProbeInit(void)
{
    syUtilsSetRandomSeedPtr(NULL);
    portProbeReset();
}

void portProbeUpdate(void)
{
    Vec3f velocity = { 0.0f, 0.0f, 0.0f };

    velocity.x = gSYControllerMain.stick_range.x * (1.5f / 80.0f);
    velocity.y = -gSYControllerMain.stick_range.y * (1.5f / 80.0f);

    /* This update executes BattleShip's original vector helper unchanged. */
    syVectorAddScaled3D(&sProbe.position, &velocity, 1.0f);

    if (gSYControllerMain.button_tap & A_BUTTON) portProbeReset();

    if (sProbe.position.x < 4.0f) sProbe.position.x = 4.0f;
    if (sProbe.position.x > 251.0f) sProbe.position.x = 251.0f;
    if (sProbe.position.y < 4.0f) sProbe.position.y = 4.0f;
    if (sProbe.position.y > 187.0f) sProbe.position.y = 187.0f;

    /* Keep this legacy probe stable; the original-asset previews are now the
     * visual signal, and per-frame color RNG reads as emulator flicker. */
}

void portProbeRender(void)
{
    /* Original asset previews now own the top-screen visual signal. */
}
