#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <SDL2/SDL.h>

namespace Ship {

class ConnectedPhysicalDeviceManager {
  public:
    ConnectedPhysicalDeviceManager();
    ~ConnectedPhysicalDeviceManager();

    std::unordered_map<int32_t, SDL_GameController*> GetConnectedSDLGamepadsForPort(uint8_t portIndex);
    std::unordered_map<int32_t, std::string> GetConnectedSDLGamepadNames();
    std::unordered_set<int32_t> GetIgnoredInstanceIdsForPort(uint8_t portIndex);
    bool PortIsIgnoringInstanceId(uint8_t portIndex, int32_t instanceId);
    void IgnoreInstanceIdForPort(uint8_t portIndex, int32_t instanceId);
    void UnignoreInstanceIdForPort(uint8_t portIndex, int32_t instanceId);

    // Globally refuse to open SDL gamepads with this VID. Used by
    // RaphnetPhysicalDeviceManager: once it has claimed an adapter via
    // hidapi, the same physical USB device also exposes an HID joystick
    // surface on interface 0. We must not let SDL open that surface — it
    // would compete with our raw-SI commands for the device handle and on
    // Windows DirectInput tends to grab first, breaking native polling.
    // Refusing in RefreshConnectedSDLGamepads keeps SDL out entirely.
    void IgnoreVendorIdGlobally(uint16_t vid);
    void UnignoreVendorIdGlobally(uint16_t vid);
    bool IsVendorIdIgnoredGlobally(uint16_t vid) const;

    void HandlePhysicalDeviceConnect(int32_t sdlDeviceIndex);
    void HandlePhysicalDeviceDisconnect(int32_t sdlJoystickInstanceId);
    void RefreshConnectedSDLGamepads();

  private:
    std::unordered_map<int32_t, SDL_GameController*> mConnectedSDLGamepads;
    std::unordered_map<int32_t, std::string> mConnectedSDLGamepadNames;
    std::unordered_map<uint8_t, std::unordered_set<int32_t>> mIgnoredInstanceIds;
    std::unordered_set<uint16_t> mIgnoredVendorIds;
};
} // namespace Ship
