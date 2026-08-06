#include "ship/controller/raphnet/MockRaphnetTransport.h"

#include <spdlog/spdlog.h>

#if __APPLE__
#include <SDL_keyboard.h>
#include <SDL_scancode.h>
#else
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_scancode.h>
#endif

namespace Ship {

namespace {

// Default keymap for synthesizing N64 input from SDL keyboard state. The
// stick-quadrant magnitude (80) matches what a real N64 controller emits
// at full deflection (roughly ±80 in s8 from the firmware after dead-zone
// handling). These keys deliberately overlap NO default LUS keyboard
// mappings — when the mock is bound to a port, the LUS::Controller raphnet
// branch bypasses the mapping pipeline entirely, so anything else that
// would map keyboard → buttons does NOT compete; we own all of port 0's
// input from this function.
constexpr int8_t kStickMag = 80;

// Apply the keymap into pad based on the SDL keyboard state pointer.
void ApplyKeymap(const uint8_t* state, OSContPad& pad) {
    pad.button = 0;
    pad.stick_x = 0;
    pad.stick_y = 0;
    pad.err_no = 0;
    pad.gyro_x = 0.0f;
    pad.gyro_y = 0.0f;
    pad.right_stick_x = 0;
    pad.right_stick_y = 0;

    // Buttons (right hand)
    if (state[SDL_SCANCODE_J]) {
        pad.button |= BTN_A;
    }
    if (state[SDL_SCANCODE_K]) {
        pad.button |= BTN_B;
    }
    if (state[SDL_SCANCODE_SPACE]) {
        pad.button |= BTN_Z;
    }
    if (state[SDL_SCANCODE_Q]) {
        pad.button |= BTN_L;
    }
    if (state[SDL_SCANCODE_E]) {
        pad.button |= BTN_R;
    }
    if (state[SDL_SCANCODE_RETURN]) {
        pad.button |= BTN_START;
    }

    // C-stick (smashes) — arrow keys.
    if (state[SDL_SCANCODE_UP]) {
        pad.button |= BTN_CUP;
    }
    if (state[SDL_SCANCODE_DOWN]) {
        pad.button |= BTN_CDOWN;
    }
    if (state[SDL_SCANCODE_LEFT]) {
        pad.button |= BTN_CLEFT;
    }
    if (state[SDL_SCANCODE_RIGHT]) {
        pad.button |= BTN_CRIGHT;
    }

    // Analog stick — WASD. N64 stick_y is +up / -down.
    int sx = 0, sy = 0;
    if (state[SDL_SCANCODE_A]) {
        sx -= kStickMag;
    }
    if (state[SDL_SCANCODE_D]) {
        sx += kStickMag;
    }
    if (state[SDL_SCANCODE_W]) {
        sy += kStickMag;
    }
    if (state[SDL_SCANCODE_S]) {
        sy -= kStickMag;
    }
    pad.stick_x = static_cast<int8_t>(sx);
    pad.stick_y = static_cast<int8_t>(sy);
}

} // namespace

bool MockRaphnetTransport::Open(const std::string& hidPath, uint16_t vid, uint16_t pid,
                                const std::wstring& serial) {
    std::lock_guard<std::mutex> lock(mLock);
    mHidPath = hidPath;
    mVid = vid;
    mPid = pid;
    mSerial = serial;
    mVersionString = "MOCK 3.6.0";
    mDevice = nullptr;  // we never own a real hid_device
    mIsOpen = true;
    SPDLOG_INFO("[raphnet:MOCK] adapter '{}' (vid=0x{:04x} pid=0x{:04x}) opened (synthetic). "
                "Default keymap: WASD=stick, J=A, K=B, Space=Z, Q/E=L/R, Enter=Start, Arrows=C-stick",
                ToUtf8(serial), vid, pid);
    return true;
}

void MockRaphnetTransport::Close() {
    std::lock_guard<std::mutex> lock(mLock);
    if (!mIsOpen) {
        return;
    }
    mIsOpen = false;
    SPDLOG_INFO("[raphnet:MOCK] adapter closed: path='{}'", mHidPath);
}

bool MockRaphnetTransport::GetVersion(std::string& outVersion) {
    outVersion = mVersionString;
    return true;
}

bool MockRaphnetTransport::GetControllerType(uint8_t channel, uint8_t& outType) {
    if (channel == 0) {
        outType = gRntCtlTypeN64;
        SPDLOG_INFO("[raphnet:MOCK] adapter '{}' chn=0 controller type: N64 (synthetic)",
                    ToUtf8(mSerial));
        return true;
    }
    outType = gRntCtlTypeNone;
    return true;
}

bool MockRaphnetTransport::SuspendPolling(bool suspend) {
    SPDLOG_DEBUG("[raphnet:MOCK] SUSPEND_POLLING({}) — synthetic ack", suspend ? 1 : 0);
    return true;
}

bool MockRaphnetTransport::SetVibration(uint8_t channel, bool on) {
    SPDLOG_INFO("[raphnet:MOCK] SET_VIBRATION chn={} on={} — synthetic (game requested rumble)",
                channel, on ? 1 : 0);
    return true;
}

bool MockRaphnetTransport::Poll(uint8_t channel, OSContPad& pad) {
    if (channel != 0) {
        return false;
    }
    if (!mIsOpen) {
        return false;
    }

    // SDL_GetKeyboardState returns a pointer to internal state. Safe to call
    // any time after SDL_Init has run — and PreInitRaphnet (which sets us up)
    // happens after SDL_Init in the boot sequence by the time Poll is called
    // from the game loop, so this is reliable. Returns nullptr if SDL is not
    // initialized; defensive check below.
    int numKeys = 0;
    const uint8_t* state = SDL_GetKeyboardState(&numKeys);
    if (state == nullptr || numKeys < SDL_NUM_SCANCODES) {
        // SDL not yet initialized — return zero pad. Should never happen in
        // normal flow but keeps us crash-free.
        pad = {};
        return true;
    }

    ApplyKeymap(state, pad);

    if (!mFirstPollLogged[channel]) {
        mFirstPollLogged[channel] = true;
        SPDLOG_INFO("[raphnet:MOCK] FIRST poll chn={} → button=0x{:04x} stick=({},{}) "
                    "(synthetic from SDL keyboard)",
                    channel, pad.button, pad.stick_x, pad.stick_y);
    } else {
        SPDLOG_TRACE("[raphnet:MOCK] poll chn={} button=0x{:04x} stick=({},{})", channel,
                     pad.button, pad.stick_x, pad.stick_y);
    }
    return true;
}

} // namespace Ship
