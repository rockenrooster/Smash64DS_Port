#pragma once

#include "ControlPort.h"
#include <vector>
#include "ship/config/Config.h"
#include "ship/controller/controldevice/controller/mapping/keyboard/KeyboardScancodes.h"
#include "ship/controller/physicaldevice/ConnectedPhysicalDeviceManager.h"
#include "ship/controller/physicaldevice/GlobalSDLDeviceSettings.h"
#include "ship/controller/controldevice/controller/mapping/ControllerDefaultMappings.h"

namespace Ship {

class RaphnetPhysicalDeviceManager;  // ship/controller/raphnet/RaphnetPhysicalDeviceManager.h

class ControlDeck {
  public:
    ControlDeck(std::vector<CONTROLLERBUTTONS_T> additionalBitmasks,
                std::shared_ptr<ControllerDefaultMappings> controllerDefaultMappings,
                std::unordered_map<CONTROLLERBUTTONS_T, std::string> buttonNames);
    ~ControlDeck();

    // Constructs and initializes the RaphnetPhysicalDeviceManager — opens
    // every detected raphnet adapter via hidapi, claims ports deterministic-
    // ally, and tells the SDL ConnectedPhysicalDeviceManager to skip-list
    // claimed VIDs. MUST be called before SDL_Init(SDL_INIT_GAMECONTROLLER)
    // so we win the Windows DirectInput grab race; see osContInit.
    void PreInitRaphnet();

    // Closes every raphnet transport (each Close sends SUSPEND_POLLING(0) so
    // the adapter is usable as a plain HID joystick after process exit) and
    // calls hid_exit. MUST be called BEFORE StopAllRumble + sContext.reset
    // at shutdown.
    void ShutdownRaphnet();

    void Init(uint8_t* controllerBits);
    virtual void WriteToPad(void* pads) = 0;
    uint8_t* GetControllerBits();
    std::shared_ptr<Controller> GetControllerByPort(uint8_t port);
    void BlockGameInput(int32_t blockId);
    void UnblockGameInput(int32_t blockId);
    bool GamepadGameInputBlocked();
    bool KeyboardGameInputBlocked();
    bool MouseGameInputBlocked();
    bool ProcessKeyboardEvent(KbEventType eventType, KbScancode scancode);
    bool ProcessMouseButtonEvent(bool isPressed, MouseBtn button);
    void StopAllRumble();

    std::shared_ptr<ConnectedPhysicalDeviceManager> GetConnectedPhysicalDeviceManager();
    std::shared_ptr<RaphnetPhysicalDeviceManager> GetRaphnetPhysicalDeviceManager();
    std::shared_ptr<GlobalSDLDeviceSettings> GetGlobalSDLDeviceSettings();
    std::shared_ptr<ControllerDefaultMappings> GetControllerDefaultMappings();
    const std::unordered_map<CONTROLLERBUTTONS_T, std::string>& GetAllButtonNames() const;
    std::string GetButtonNameForBitmask(CONTROLLERBUTTONS_T bitmask);

  protected:
    bool AllGameInputBlocked();
    std::vector<std::shared_ptr<ControlPort>> mPorts = {};

  private:
    uint8_t* mControllerBits = nullptr;
    std::unordered_map<int32_t, bool> mGameInputBlockers;
    std::shared_ptr<ConnectedPhysicalDeviceManager> mConnectedPhysicalDeviceManager;
    std::shared_ptr<RaphnetPhysicalDeviceManager> mRaphnetPhysicalDeviceManager;
    std::shared_ptr<GlobalSDLDeviceSettings> mGlobalSDLDeviceSettings;
    std::shared_ptr<ControllerDefaultMappings> mControllerDefaultMappings;
    std::unordered_map<CONTROLLERBUTTONS_T, std::string> mButtonNames;
};
} // namespace Ship
