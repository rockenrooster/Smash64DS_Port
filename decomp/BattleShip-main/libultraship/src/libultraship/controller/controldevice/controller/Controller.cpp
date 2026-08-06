#include "libultraship/controller/controldevice/controller/Controller.h"
#include <memory>
#include <algorithm>
#include "ship/Context.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/controller/controldeck/ControlDeck.h"
#include "ship/controller/raphnet/RaphnetPhysicalDeviceManager.h"
#include "ship/controller/raphnet/RaphnetTransport.h"
#if __APPLE__
#include <SDL_events.h>
#else
#include <SDL2/SDL_events.h>
#endif
#include <spdlog/spdlog.h>
#include "ship/utils/StringHelper.h"

#define M_TAU 6.2831853071795864769252867665590057 // 2 * pi
#define MINIMUM_RADIUS_TO_MAP_NOTCH 0.9

namespace LUS {
Controller::Controller(uint8_t portIndex, std::vector<CONTROLLERBUTTONS_T> bitmasks)
    : Ship::Controller(portIndex, bitmasks) {
}

void Controller::ReadToPad(void* pad) {
    ReadToOSContPad((OSContPad*)pad);
}

// Drop the native raphnet binding for one game port: zero the auto-fallback
// counter, mark the binding inactive, and ask the manager to forget the
// (port, transport, channel) tuple so the InputEditor reflects the change
// (banner disappears, normal SDL/keyboard mapping UI appears for that port).
// Underlying transport stays open for other ports / rumble.
static void RaphnetDropPortBinding(uint8_t portIndex) {
    auto controlDeck = Ship::Context::GetInstance()->GetControlDeck();
    if (controlDeck == nullptr) {
        return;
    }
    auto manager = controlDeck->GetRaphnetPhysicalDeviceManager();
    if (manager == nullptr) {
        return;
    }
    manager->ReleasePort(portIndex);
}

// ~1 second at 60 fps; tuned so a transient single-frame drop (USB stall) doesn't
// instantly bounce the user out of native mode, but a real broken connection
// (firmware not responding to RAW_SI commands, controller disconnected after
// boot, wrong protocol assumption on the user's hardware) returns control
// to them quickly enough that they can play the game with SDL/keyboard.
static constexpr uint32_t kRaphnetMaxConsecutivePollFailures = 60;

void Controller::SetRaphnetBinding(std::weak_ptr<Ship::RaphnetTransport> transport, uint8_t channel) {
    mRaphnetTransport = std::move(transport);
    mRaphnetChannel = channel;
    mRaphnetBindingActive = !mRaphnetTransport.expired();
    mRaphnetConsecutivePollFailures = 0;
    SPDLOG_INFO("[raphnet] LUS::Controller port={} bound to raphnet chn={} "
                "(simulated-input-lag CVAR bypassed on this port)",
                GetPortIndex(), channel);
}

void Controller::ReadToOSContPad(OSContPad* pad) {
    // Native raphnet path: poll the bound adapter channel directly, write
    // straight into pad, skip mappings and skip the input-lag pad buffer.
    // Combining real adapter USB latency with CVAR_SIMULATED_INPUT_LAG would
    // double-count delay and feel terrible.
    if (mRaphnetBindingActive && pad != nullptr) {
        if (auto t = mRaphnetTransport.lock()) {
            if (t->Poll(mRaphnetChannel, *pad)) {
                mRaphnetConsecutivePollFailures = 0;
                return;  // pad fully populated by Poll
            }
            // Poll returned false. RaphnetTransport already throttled the log
            // (FIRST + every-60th); we just track how long this has been
            // happening so we can auto-disable native mode if it persists.
            ++mRaphnetConsecutivePollFailures;
            if (mRaphnetConsecutivePollFailures < kRaphnetMaxConsecutivePollFailures) {
                // Within tolerance: zero pad so the game doesn't see stale state
                // from before the failure run started.
                pad->button = 0;
                pad->stick_x = 0;
                pad->stick_y = 0;
                pad->err_no = 0;
                pad->gyro_x = 0.0f;
                pad->gyro_y = 0.0f;
                pad->right_stick_x = 0;
                pad->right_stick_y = 0;
                return;
            }
            // Threshold breached. Disable native polling on this port and let
            // the standard SDL/keyboard mapping pipeline below run. Without
            // this fallback the user has no way to recover except editing
            // gControllers.Raphnet.Enabled=0 and relaunching.
            SPDLOG_ERROR("[raphnet] LUS::Controller port={} disabling native polling after "
                         "{} consecutive Poll failures; falling back to SDL/keyboard mapping. "
                         "Look earlier in the log for '[raphnet] FIRST poll FAILURE chn=' "
                         "for the underlying error. To force-disable native mode permanently, "
                         "set CVAR gControllers.Raphnet.Enabled=0 and relaunch.",
                         GetPortIndex(), mRaphnetConsecutivePollFailures);
            mRaphnetBindingActive = false;
            RaphnetDropPortBinding(GetPortIndex());
            // Fall through to mapping pipeline.
        } else {
            // Transport gone (manager shutdown). Fall through to the mapping
            // path so the user gets at least keyboard/SDL controls back instead
            // of a frozen controller.
            mRaphnetBindingActive = false;
            SPDLOG_WARN("[raphnet] LUS::Controller port={} transport expired; falling back to mapping pipeline",
                        GetPortIndex());
        }
    }

    OSContPad padToBuffer = { 0 };

    // Button Inputs
    for (auto [bitmask, button] : mButtons) {
        button->UpdatePad(padToBuffer.button);
    }

    // Stick Inputs
    GetLeftStick()->UpdatePad(padToBuffer.stick_x, padToBuffer.stick_y);
    GetRightStick()->UpdatePad(padToBuffer.right_stick_x, padToBuffer.right_stick_y);

    // Gyro
    GetGyro()->UpdatePad(padToBuffer.gyro_x, padToBuffer.gyro_y);

    mPadBuffer.push_front(padToBuffer);
    if (pad != nullptr) {
        auto& padFromBuffer = mPadBuffer[std::min(
            mPadBuffer.size() - 1,
            (size_t)Ship::Context::GetInstance()->GetConsoleVariables()->GetInteger(CVAR_SIMULATED_INPUT_LAG, 0))];

        pad->button |= padFromBuffer.button;

        if (pad->stick_x == 0) {
            pad->stick_x = padFromBuffer.stick_x;
        }
        if (pad->stick_y == 0) {
            pad->stick_y = padFromBuffer.stick_y;
        }

        if (pad->right_stick_x == 0) {
            pad->right_stick_x = padFromBuffer.right_stick_x;
        }
        if (pad->right_stick_y == 0) {
            pad->right_stick_y = padFromBuffer.right_stick_y;
        }

        if (pad->gyro_x == 0) {
            pad->gyro_x = padFromBuffer.gyro_x;
        }
        if (pad->gyro_y == 0) {
            pad->gyro_y = padFromBuffer.gyro_y;
        }
    }

    while (mPadBuffer.size() > 6) {
        mPadBuffer.pop_back();
    }
}
} // namespace LUS
