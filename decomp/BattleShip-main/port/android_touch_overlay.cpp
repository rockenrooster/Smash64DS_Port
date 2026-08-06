// android_touch_overlay.cpp — virtual SDL gamepad fed by Java touch events.
//
// Architecture:
//   1. SDL_main thread (port.cpp's pre-init block) brings up
//      SDL_INIT_GAMECONTROLLER. We attach a virtual joystick on first
//      JNI call from TouchOverlay so we don't pay the cost in builds
//      where the user has a paired Bluetooth/USB gamepad and never
//      touches the on-screen buttons.
//
//   2. Java's TouchOverlay overlays buttons on top of the SDL surface
//      and forwards touch DOWN/UP events to setButton() / setAxis()
//      JNI methods. SDL_JoystickSetVirtualButton/Axis is documented as
//      thread-safe; SDL_PumpEvents on the SDL thread translates the
//      state changes into SDL_CONTROLLERBUTTONDOWN/UP events that LUS
//      already knows how to map to OSContPad bits via SDLButtonToAnyMapping.
//
// Lifetime: the virtual joystick lives for the duration of the process.

#if defined(__ANDROID__)

#include <SDL2/SDL.h>
#include <SDL2/SDL_joystick.h>

#include <libultraship/libultraship.h>
#include <ship/Context.h>
#include <ship/window/Window.h>
#include <ship/window/gui/Gui.h>

#include <android/log.h>
#include <jni.h>

#include <atomic>

#define LOG_TAG "ssb64.touch"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

SDL_Joystick *sJoystick = nullptr;
int sDeviceIndex = -1;
std::atomic<bool> sAttachAttempted{false};

// Xbox-360-style descriptor. SDL2 promotes any virtual joystick with this
// vendor/product signature to SDL_GameController, so LUS's existing
// SDLButtonToAnyMapping pipeline picks us up without changes.
constexpr int kAxisCount   = SDL_CONTROLLER_AXIS_MAX;        // 6
constexpr int kButtonCount = SDL_CONTROLLER_BUTTON_MAX;      // 15

bool ensureAttached() {
    if (sJoystick != nullptr) {
        return true;
    }
    if (sAttachAttempted.exchange(true)) {
        return false;
    }

    SDL_VirtualJoystickDesc desc;
    SDL_zero(desc);
    desc.version    = SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
    desc.type       = SDL_JOYSTICK_TYPE_GAMECONTROLLER;
    desc.naxes      = kAxisCount;
    desc.nbuttons   = kButtonCount;
    desc.vendor_id  = 0x045E;
    desc.product_id = 0x028E;
    desc.name       = "SSB64 Touch Overlay";

    sDeviceIndex = SDL_JoystickAttachVirtualEx(&desc);
    if (sDeviceIndex < 0) {
        LOGE("SDL_JoystickAttachVirtualEx failed: %s", SDL_GetError());
        return false;
    }
    sJoystick = SDL_JoystickOpen(sDeviceIndex);
    if (sJoystick == nullptr) {
        LOGE("SDL_JoystickOpen(%d) failed: %s", sDeviceIndex, SDL_GetError());
        SDL_JoystickDetachVirtual(sDeviceIndex);
        sDeviceIndex = -1;
        return false;
    }
    // SDL_GameController normalizes the joystick's trigger axes via the
    // standard Xbox-360 mapping ("lefttrigger:a4,righttrigger:a5"). With
    // that mapping, raw joystick value -32768 -> trigger 0 (released);
    // raw +32767 -> trigger 32767 (fully pressed). SDL_JoystickAttachVirtual
    // defaults all axes to 0, which would land the triggers at ~50% — i.e.
    // LUS sees N64 Z/R as held the entire time we haven't touched them.
    // Park them at -32768 explicitly.
    SDL_JoystickSetVirtualAxis(sJoystick, SDL_CONTROLLER_AXIS_TRIGGERLEFT,  -32768);
    SDL_JoystickSetVirtualAxis(sJoystick, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, -32768);

    LOGI("Virtual joystick attached (device_index=%d, instance_id=%d, %d axes, %d buttons)",
         sDeviceIndex, SDL_JoystickInstanceID(sJoystick), kAxisCount, kButtonCount);
    return true;
}

} // namespace

extern "C" {

// Called from PortShutdown. These statics outlive SDL_Quit — Android keeps
// the process (and its globals) alive across Activity relaunches, so a
// stale sJoystick from the previous SDL session would be a use-after-free
// on the first touch after relaunch.
void port_touch_overlay_shutdown(void) {
    if (sJoystick != nullptr) {
        SDL_JoystickClose(sJoystick);
        sJoystick = nullptr;
    }
    if (sDeviceIndex >= 0) {
        SDL_JoystickDetachVirtual(sDeviceIndex);
        sDeviceIndex = -1;
    }
    sAttachAttempted.store(false);
}

JNIEXPORT void JNICALL
Java_com_jrickey_battleship_TouchOverlay_setButton(
    JNIEnv * /*env*/, jclass /*clazz*/, jint btn, jboolean down) {
    if (!ensureAttached()) return;
    if (btn < 0 || btn >= kButtonCount) {
        LOGW("setButton: out-of-range btn=%d", (int)btn);
        return;
    }
    SDL_JoystickSetVirtualButton(sJoystick, btn, down ? 1 : 0);
}

JNIEXPORT void JNICALL
Java_com_jrickey_battleship_TouchOverlay_setAxis(
    JNIEnv * /*env*/, jclass /*clazz*/, jint axis, jint value) {
    if (!ensureAttached()) return;
    if (axis < 0 || axis >= kAxisCount) {
        LOGW("setAxis: out-of-range axis=%d", (int)axis);
        return;
    }
    if (value < -32768) value = -32768;
    if (value >  32767) value =  32767;
    SDL_JoystickSetVirtualAxis(sJoystick, axis, (Sint16)value);
}

// Menu toggle: set an atomic flag here on the Android UI thread; the
// SDL_main thread's PortPushFrame drains it via port_drain_pending_menu_toggle()
// (see port/gameloop.cpp) and calls libultraship's
// Gui->GetMenu()->ToggleVisibility() from the same thread that's running
// the ImGui frame. Pushing SDL_KEYDOWN(F1) was unreliable — Gui.cpp's
// IsKeyPressed(TOGGLE_BTN, false) is an edge detector that needs the
// keydown to land in the right frame slot, and SDL backend event timing
// from a JNI-thread push didn't consistently line up.
static std::atomic<bool> sMenuTogglePending{false};

extern "C" bool port_drain_pending_menu_toggle(void) {
    return sMenuTogglePending.exchange(false);
}

JNIEXPORT void JNICALL
Java_com_jrickey_battleship_TouchOverlay_toggleMenu(
    JNIEnv * /*env*/, jclass /*clazz*/) {
    sMenuTogglePending.store(true);
    LOGI("toggleMenu: queued");
}

// Polled every ~100ms by TouchOverlay.installMenuVisibilityPoller so the
// overlay can hide itself while the menu is up — otherwise the analog
// stick view eats taps on menu items before they reach ImGui.
JNIEXPORT jboolean JNICALL
Java_com_jrickey_battleship_TouchOverlay_isMenuVisible(
    JNIEnv * /*env*/, jclass /*clazz*/) {
    auto ctx = Ship::Context::GetInstance();
    if (!ctx || !ctx->GetWindow() || !ctx->GetWindow()->GetGui()) {
        return JNI_FALSE;
    }
    return ctx->GetWindow()->GetGui()->GetMenuOrMenubarVisible()
        ? JNI_TRUE : JNI_FALSE;
}

} // extern "C"

#endif // __ANDROID__
