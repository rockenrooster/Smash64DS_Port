#include "ship/controller/physicaldevice/ConnectedPhysicalDeviceManager.h"
#include <spdlog/spdlog.h>

namespace Ship {
ConnectedPhysicalDeviceManager::ConnectedPhysicalDeviceManager() {
}

ConnectedPhysicalDeviceManager::~ConnectedPhysicalDeviceManager() {
}

std::unordered_map<int32_t, SDL_GameController*>
ConnectedPhysicalDeviceManager::GetConnectedSDLGamepadsForPort(uint8_t portIndex) {
    std::unordered_map<int32_t, SDL_GameController*> result;

    for (const auto& [instanceId, gamepad] : mConnectedSDLGamepads) {
        if (!PortIsIgnoringInstanceId(portIndex, instanceId)) {
            result[instanceId] = gamepad;
        }
    }

    return result;
}

std::unordered_map<int32_t, std::string> ConnectedPhysicalDeviceManager::GetConnectedSDLGamepadNames() {
    return mConnectedSDLGamepadNames;
}

std::unordered_set<int32_t> ConnectedPhysicalDeviceManager::GetIgnoredInstanceIdsForPort(uint8_t portIndex) {
    return mIgnoredInstanceIds[portIndex];
}

bool ConnectedPhysicalDeviceManager::PortIsIgnoringInstanceId(uint8_t portIndex, int32_t instanceId) {
    return GetIgnoredInstanceIdsForPort(portIndex).contains(instanceId);
}

void ConnectedPhysicalDeviceManager::IgnoreInstanceIdForPort(uint8_t portIndex, int32_t instanceId) {
    mIgnoredInstanceIds[portIndex].insert(instanceId);
}

void ConnectedPhysicalDeviceManager::UnignoreInstanceIdForPort(uint8_t portIndex, int32_t instanceId) {
    mIgnoredInstanceIds[portIndex].erase(instanceId);
}

void ConnectedPhysicalDeviceManager::IgnoreVendorIdGlobally(uint16_t vid) {
    if (mIgnoredVendorIds.insert(vid).second) {
        SPDLOG_INFO("ConnectedPhysicalDeviceManager: globally ignoring SDL gamepads with VID 0x{:04x} "
                    "(claimed by another input backend, e.g. Raphnet native)", vid);
    }
}

void ConnectedPhysicalDeviceManager::UnignoreVendorIdGlobally(uint16_t vid) {
    if (mIgnoredVendorIds.erase(vid) > 0) {
        SPDLOG_INFO("ConnectedPhysicalDeviceManager: no longer ignoring SDL gamepads with VID 0x{:04x}", vid);
    }
}

bool ConnectedPhysicalDeviceManager::IsVendorIdIgnoredGlobally(uint16_t vid) const {
    return mIgnoredVendorIds.contains(vid);
}

void ConnectedPhysicalDeviceManager::HandlePhysicalDeviceConnect(int32_t sdlDeviceIndex) {
    RefreshConnectedSDLGamepads();
}

void ConnectedPhysicalDeviceManager::HandlePhysicalDeviceDisconnect(int32_t sdlJoystickInstanceId) {
    RefreshConnectedSDLGamepads();
}

void ConnectedPhysicalDeviceManager::RefreshConnectedSDLGamepads() {
    // Snapshot the instance ids we already knew about. A refresh fires on
    // EVERY controller add/remove event, and the per-port routing (ignore
    // sets) must survive for pads that were already connected — previously
    // each refresh re-ignored every pad for ports 2-4, silently reverting
    // the user's port assignments whenever any device event arrived.
    auto previouslyConnected = std::move(mConnectedSDLGamepads);
    std::vector<int32_t> newInstanceIds;

    mConnectedSDLGamepads.clear();
    mConnectedSDLGamepadNames.clear();
    static SDL_JoystickGUID sZeroGuid;

    for (int32_t i = 0; i < SDL_NumJoysticks(); i++) {

        SDL_JoystickGUID deviceGUID = SDL_JoystickGetDeviceGUID(i);
        if (SDL_memcmp(&deviceGUID, &sZeroGuid, sizeof(deviceGUID)) == 0) {
            SPDLOG_WARN(
                "Calling SDL JoystickGetDeviceGUID with index ({:d}) returned zero GUID. This is likely due to an "
                "invalid index. Refer to https://wiki.libsdl.org/SDL2/SDL_JoystickGetDeviceGUID for more information.",
                i);
            continue;
        }

        char deviceGuidCStr[33] = "";
        SDL_JoystickGetGUIDString(deviceGUID, deviceGuidCStr, sizeof(deviceGuidCStr));

        if (!SDL_IsGameController(i)) {
            SPDLOG_WARN("SDL Joystick (GUID: {}) not recognized as gamepad."
                        "This is likely due to a missing mapping string in gamecontrollerdb.txt."
                        "Refer to https://github.com/mdqinc/SDL_GameControllerDB for more information.",
                        deviceGuidCStr);
            continue;
        }

        // Globally ignored vendor (e.g. Raphnet adapter already claimed via
        // hidapi by RaphnetPhysicalDeviceManager). Skip BEFORE SDL_GameController-
        // Open so SDL never gets a handle to the device — opening here would
        // compete with our raw-SI commands and on Windows DirectInput tends to
        // grab first, breaking native polling.
        uint16_t devVid = SDL_JoystickGetDeviceVendor(i);
        if (devVid != 0 && IsVendorIdIgnoredGlobally(devVid)) {
            SPDLOG_INFO("ConnectedPhysicalDeviceManager: skipping SDL gamepad index={} VID=0x{:04x} (GUID: {}) "
                        "— globally ignored by another input backend",
                        i, devVid, deviceGuidCStr);
            continue;
        }

        auto gamepad = SDL_GameControllerOpen(i);
        if (gamepad == nullptr) {
            SPDLOG_ERROR("SDL GameControllerOpen error (GUID: {}): {}", deviceGuidCStr, SDL_GetError());
            continue;
        }

        auto instanceId = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gamepad));
        if (instanceId < 0) {
            SPDLOG_ERROR("SDL JoystickInstanceID error (GUID: {}): {}", deviceGuidCStr, SDL_GetError());
            continue;
        }

        std::string gamepadName;
        auto name = SDL_GameControllerName(gamepad);
        if (name == nullptr) {
            gamepadName = deviceGuidCStr;
            SPDLOG_WARN("SDL_GameControllerName returned null. Setting name to GUID \"{}\" instead.", gamepadName);
        } else {
            gamepadName = name;
        }

        mConnectedSDLGamepads[instanceId] = gamepad;
        mConnectedSDLGamepadNames[instanceId] = gamepadName;

        if (!previouslyConnected.contains(instanceId)) {
            newInstanceIds.push_back(instanceId);
        }
    }

    // Default routing for newly connected pads: each goes to the first port
    // that doesn't have a pad yet (1st pad → port 1, 2nd pad → port 2, ...)
    // so local multiplayer works out of the box with any mix of controller
    // types. Insert into every port's ignore set first so the occupancy
    // scan below doesn't see the pad we're currently placing.
    for (int32_t newId : newInstanceIds) {
        for (uint8_t port = 0; port < 4; port++) {
            mIgnoredInstanceIds[port].insert(newId);
        }
    }
    for (int32_t newId : newInstanceIds) {
        uint8_t freePort = UINT8_MAX;
        for (uint8_t port = 0; port < 4; port++) {
            bool occupied = false;
            for (const auto& [instanceId, gamepad] : mConnectedSDLGamepads) {
                if (!mIgnoredInstanceIds[port].contains(instanceId)) {
                    occupied = true;
                    break;
                }
            }
            if (!occupied) {
                freePort = port;
                break;
            }
        }
        if (freePort == UINT8_MAX) {
            SPDLOG_INFO("ConnectedPhysicalDeviceManager: all 4 ports already have a gamepad; "
                        "leaving '{}' (instanceId {}) unassigned",
                        mConnectedSDLGamepadNames[newId], newId);
            continue;
        }
        mIgnoredInstanceIds[freePort].erase(newId);
        SPDLOG_INFO("ConnectedPhysicalDeviceManager: assigned new gamepad '{}' (instanceId {}) to port {}",
                    mConnectedSDLGamepadNames[newId], newId, freePort + 1);
    }
}
} // namespace Ship
