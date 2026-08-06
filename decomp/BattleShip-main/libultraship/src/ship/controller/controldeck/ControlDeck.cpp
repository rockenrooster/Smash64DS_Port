#include "ship/controller/controldeck/ControlDeck.h"

#include "ship/Context.h"
#include "ship/controller/controldevice/controller/Controller.h"
#include "ship/controller/controldevice/controller/mapping/raphnet/RaphnetRumbleMapping.h"
#include "ship/controller/raphnet/RaphnetPhysicalDeviceManager.h"
#include "ship/utils/StringHelper.h"
#include "ship/config/ConsoleVariable.h"
#include <imgui.h>
#include "ship/controller/controldevice/controller/mapping/mouse/WheelHandler.h"

namespace Ship {

ControlDeck::ControlDeck(std::vector<CONTROLLERBUTTONS_T> additionalBitmasks,
                         std::shared_ptr<ControllerDefaultMappings> controllerDefaultMappings,
                         std::unordered_map<CONTROLLERBUTTONS_T, std::string> buttonNames) {
    mConnectedPhysicalDeviceManager = std::make_shared<ConnectedPhysicalDeviceManager>();
    mGlobalSDLDeviceSettings = std::make_shared<GlobalSDLDeviceSettings>();
    mControllerDefaultMappings = controllerDefaultMappings == nullptr ? std::make_shared<ControllerDefaultMappings>()
                                                                      : controllerDefaultMappings;
}

ControlDeck::~ControlDeck() {
    SPDLOG_TRACE("destruct control deck");
}

void ControlDeck::PreInitRaphnet() {
    if (mRaphnetPhysicalDeviceManager != nullptr) {
        SPDLOG_WARN("ControlDeck::PreInitRaphnet called twice; ignoring");
        return;
    }

    // Master kill switch — lets users force-disable native mode without
    // unplugging the adapter (it then appears as a plain SDL HID joystick).
    int32_t enabled = Ship::Context::GetInstance()->GetConsoleVariables()->GetInteger(
        CVAR_PREFIX_CONTROLLERS ".Raphnet.Enabled", 1);
    if (enabled == 0) {
        SPDLOG_INFO("ControlDeck::PreInitRaphnet: gControllers.Raphnet.Enabled=0; native adapter "
                    "support disabled by CVAR (adapter will fall back to SDL HID joystick)");
        return;
    }

    mRaphnetPhysicalDeviceManager = std::make_shared<RaphnetPhysicalDeviceManager>();
    if (!mRaphnetPhysicalDeviceManager->Init()) {
        SPDLOG_WARN("Raphnet manager Init failed; native adapter support disabled this session");
        // Keep the manager around so accessors return a non-null but empty
        // instance — simpler than nullptr-checking everywhere.
        return;
    }
    // Tell the SDL device manager to skip every adapter VID we just claimed,
    // BEFORE SDL_Init(SDL_INIT_GAMECONTROLLER) (which is the next thing the
    // caller does). This wins the Windows DirectInput grab race against the
    // raphnet HID joystick surface.
    for (uint16_t vid : mRaphnetPhysicalDeviceManager->GetClaimedVids()) {
        mConnectedPhysicalDeviceManager->IgnoreVendorIdGlobally(vid);
    }
    SPDLOG_INFO("ControlDeck::PreInitRaphnet: {} adapter(s), {} port(s) claimed",
                mRaphnetPhysicalDeviceManager->GetOpenTransports().size(),
                mRaphnetPhysicalDeviceManager->ClaimedPortCount());

    // One-shot diagnostic dump for remote testers. Runs after enumeration
    // but before normal game startup; the game still launches normally
    // afterward so the user can close the window when satisfied.
    int32_t selfTest = Ship::Context::GetInstance()->GetConsoleVariables()->GetInteger(
        CVAR_PREFIX_CONTROLLERS ".Raphnet.SelfTest", 0);
    if (selfTest != 0) {
        SPDLOG_INFO("ControlDeck::PreInitRaphnet: gControllers.Raphnet.SelfTest=1; running diagnostic dump");
        mRaphnetPhysicalDeviceManager->RunSelfTest();
    }
}

void ControlDeck::ShutdownRaphnet() {
    if (mRaphnetPhysicalDeviceManager == nullptr) {
        return;
    }
    SPDLOG_INFO("ControlDeck::ShutdownRaphnet");
    mRaphnetPhysicalDeviceManager->Shutdown();
    mRaphnetPhysicalDeviceManager.reset();
}

void ControlDeck::Init(uint8_t* controllerBits) {
    mControllerBits = controllerBits;
    *mControllerBits |= 1 << 0;

    for (auto port : mPorts) {
        if (port->GetConnectedController()->HasConfig()) {
            port->GetConnectedController()->ReloadAllMappingsFromConfig();
        }
    }

    // if we don't have a config for controller 1, set default bindings
    if (!mPorts[0]->GetConnectedController()->HasConfig()) {
        mPorts[0]->GetConnectedController()->AddDefaultMappings(PhysicalDeviceType::Keyboard);
        mPorts[0]->GetConnectedController()->AddDefaultMappings(PhysicalDeviceType::Mouse);
        mPorts[0]->GetConnectedController()->AddDefaultMappings(PhysicalDeviceType::SDLGamepad);
    }

    // Gamepad defaults for ports 2-4 as well: SDL mappings only fire for
    // pads routed to their port, so these stay dormant until a pad is
    // assigned there (second pad connected, or the user toggles routing in
    // the input editor) — at which point the pad just works instead of
    // requiring a manual "Set Defaults" per port. Keyboard/mouse stay
    // port-1-only.
    for (size_t i = 1; i < mPorts.size(); i++) {
        if (!mPorts[i]->GetConnectedController()->HasConfig()) {
            mPorts[i]->GetConnectedController()->AddDefaultMappings(PhysicalDeviceType::SDLGamepad);
        }
    }

    // Install Raphnet rumble mappings on any port the RaphnetPhysicalDeviceManager
    // has claimed. Polling (the input read path) wires up in L7 — this commit
    // only handles rumble wiring, which can land independently because
    // ControllerRumble already supports adding mappings via AddRumbleMapping.
    if (mRaphnetPhysicalDeviceManager != nullptr) {
        for (size_t i = 0; i < mPorts.size(); ++i) {
            const uint8_t portIndex = static_cast<uint8_t>(i);
            auto transport = mRaphnetPhysicalDeviceManager->GetTransportForPort(portIndex);
            if (transport == nullptr) {
                continue;
            }
            const int channel = mRaphnetPhysicalDeviceManager->GetChannelForPort(portIndex);
            if (channel < 0) {
                continue;
            }
            auto controller = mPorts[i]->GetConnectedController();
            if (controller == nullptr) {
                continue;
            }
            auto rumble = controller->GetRumble();
            if (rumble == nullptr) {
                continue;
            }
            auto mapping = std::make_shared<RaphnetRumbleMapping>(
                portIndex, DEFAULT_LOW_FREQUENCY_RUMBLE_PERCENTAGE,
                DEFAULT_HIGH_FREQUENCY_RUMBLE_PERCENTAGE, std::weak_ptr<RaphnetTransport>(transport),
                static_cast<uint8_t>(channel));
            rumble->AddRumbleMapping(mapping);
            // Switch the controller's read path to native raw-SI polling.
            // No-op on bases that don't override; LUS::Controller overrides.
            controller->SetRaphnetBinding(std::weak_ptr<RaphnetTransport>(transport),
                                          static_cast<uint8_t>(channel));
            SPDLOG_INFO("ControlDeck::Init: port {} raphnet binding installed "
                        "(chn={}, rumble + native polling active)",
                        portIndex, channel);
        }
    }
}

bool ControlDeck::ProcessKeyboardEvent(KbEventType eventType, KbScancode scancode) {
    bool result = false;
    for (auto port : mPorts) {
        auto controller = port->GetConnectedController();

        if (controller != nullptr) {
            result = controller->ProcessKeyboardEvent(eventType, scancode) || result;
        }
    }

    return result;
}

bool ControlDeck::ProcessMouseButtonEvent(bool isPressed, MouseBtn button) {
    bool result = false;
    for (auto port : mPorts) {
        auto controller = port->GetConnectedController();

        if (controller != nullptr) {
            result = controller->ProcessMouseButtonEvent(isPressed, button) || result;
        }
    }

    return result;
}

bool ControlDeck::AllGameInputBlocked() {
    return !mGameInputBlockers.empty();
}

bool ControlDeck::GamepadGameInputBlocked() {
    // block controller input when using the controller to navigate imgui menus
    return AllGameInputBlocked() ||
           Context::GetInstance()->GetWindow()->GetGui()->GetMenuOrMenubarVisible() &&
               Ship::Context::GetInstance()->GetConsoleVariables()->GetInteger(CVAR_IMGUI_CONTROLLER_NAV, 0);
}

bool ControlDeck::KeyboardGameInputBlocked() {
    // block keyboard input when typing in imgui
    ImGuiWindow* activeIDWindow = ImGui::GetCurrentContext()->ActiveIdWindow;
    return AllGameInputBlocked() ||
           (activeIDWindow != NULL &&
            activeIDWindow->ID != Context::GetInstance()->GetWindow()->GetGui()->GetMainGameWindowID()) ||
           ImGui::GetTopMostPopupModal() != NULL; // ImGui::GetIO().WantCaptureKeyboard, but ActiveId check altered
}

bool ControlDeck::MouseGameInputBlocked() {
    // block mouse input when user interacting with gui
    ImGuiWindow* window = ImGui::GetCurrentContext()->HoveredWindow;
    if (window == NULL) {
        return true;
    }
    return AllGameInputBlocked() ||
           (window->ID != Context::GetInstance()->GetWindow()->GetGui()->GetMainGameWindowID());
}

std::shared_ptr<Controller> ControlDeck::GetControllerByPort(uint8_t port) {
    return mPorts[port]->GetConnectedController();
}

// Send "motors off" to every connected gamepad. Must be called while Context,
// ControlDeck, and SDL are all still alive — i.e. before sContext.reset() at
// shutdown. Without this, a clean app exit leaves the last in-flight FF effect
// (uploaded with SDL_MAX_RUMBLE_DURATION_MS ≈ 32s on the Linux evdev backend)
// running on the device until the kernel timer expires.
void ControlDeck::StopAllRumble() {
    for (auto& port : mPorts) {
        auto controller = port->GetConnectedController();
        if (controller == nullptr) {
            continue;
        }
        auto rumble = controller->GetRumble();
        if (rumble != nullptr) {
            rumble->StopRumble();
        }
    }
}

void ControlDeck::BlockGameInput(int32_t blockId) {
    mGameInputBlockers[blockId] = true;
}

void ControlDeck::UnblockGameInput(int32_t blockId) {
    mGameInputBlockers.erase(blockId);
}

std::shared_ptr<ConnectedPhysicalDeviceManager> ControlDeck::GetConnectedPhysicalDeviceManager() {
    return mConnectedPhysicalDeviceManager;
}

std::shared_ptr<RaphnetPhysicalDeviceManager> ControlDeck::GetRaphnetPhysicalDeviceManager() {
    return mRaphnetPhysicalDeviceManager;
}

std::shared_ptr<GlobalSDLDeviceSettings> ControlDeck::GetGlobalSDLDeviceSettings() {
    return mGlobalSDLDeviceSettings;
}

std::shared_ptr<ControllerDefaultMappings> ControlDeck::GetControllerDefaultMappings() {
    return mControllerDefaultMappings;
}

const std::unordered_map<CONTROLLERBUTTONS_T, std::string>& ControlDeck::GetAllButtonNames() const {
    return mButtonNames;
}

std::string ControlDeck::GetButtonNameForBitmask(CONTROLLERBUTTONS_T bitmask) {
    // if we don't have a name for this bitmask,
    // return the stringified bitmask
    if (!mButtonNames.contains(bitmask)) {
        return std::to_string(bitmask);
    }

    return mButtonNames[bitmask];
}
} // namespace Ship
