#pragma once

// RaphnetPhysicalDeviceManager
// ----------------------------
// Owns the open RaphnetTransport instances for all detected raphnet adapters
// and binds adapter (transport, channel) pairs to N64 game ports. Parallels
// Ship::ConnectedPhysicalDeviceManager (which owns SDL gamepads); the two
// device backends coexist by way of an SDL VID skip-list (L5) — when this
// manager has claimed an adapter, the SDL manager refuses to open it.
//
// Claim policy is DETERMINISTIC across reboots: candidates (adapter, channel)
// pairs reporting CTL_TYPE_N64 are sorted by (serial, channel) and bound to
// game ports in order. Same physical adapter → same port assignment, plug
// order independent. CVAR pinning (gControllers.PortN.RaphnetSerial) lands
// in L9.
//
// Init() must run BEFORE SDL_Init(SDL_INIT_GAMECONTROLLER) so we win the
// Windows DirectInput grab race against the joystick HID surface. ControlDeck
// exposes this via PreInitRaphnet() in L6.

#include <array>
#include <cstdint>
#include <memory>
#include <unordered_set>
#include <vector>

#include "libultraship/libultra/controller.h"  // MAXCONTROLLERS
#include "ship/controller/raphnet/RaphnetTransport.h"

namespace Ship {

struct RaphnetPortBinding {
    std::shared_ptr<RaphnetTransport> Transport;
    uint8_t                           Channel;
};

class RaphnetPhysicalDeviceManager {
  public:
    RaphnetPhysicalDeviceManager();
    ~RaphnetPhysicalDeviceManager();

    RaphnetPhysicalDeviceManager(const RaphnetPhysicalDeviceManager&) = delete;
    RaphnetPhysicalDeviceManager& operator=(const RaphnetPhysicalDeviceManager&) = delete;

    // Calls hid_init(), enumerates adapters, opens each, probes channels
    // 0..gRntMaxChannelsPerAdapter-1 with GetControllerType filtering for
    // CTL_TYPE_N64, then deterministically claims game ports. Returns true
    // on success regardless of how many ports were claimed (zero is fine —
    // means no native adapter present and SDL handles all input). Returns
    // false only if hid_init itself failed.
    bool Init();

    // Tears down every open transport (each Close() sends SUSPEND_POLLING(0)
    // so the adapter is usable as a plain HID joystick after process exit),
    // clears port bindings, and calls hid_exit(). Idempotent.
    void Shutdown();

    bool IsInitialized() const { return mInitialized; }

    // Runs an exhaustive diagnostic dump for every claimed (transport, channel):
    //   * GetVersion + GetControllerType for every channel
    //   * 10 sequential polls with full byte logging
    //   * 100 ms vibration on / 100 ms off
    // Everything goes to ssb64.log at INFO. Designed for remote testers to
    // capture-and-share via `gControllers.Raphnet.SelfTest=1; run game`.
    void RunSelfTest();

    // Per-port binding accessors. Return nullptr / -1 if portIndex is
    // unclaimed.
    std::shared_ptr<RaphnetTransport> GetTransportForPort(uint8_t portIndex) const;
    int                               GetChannelForPort(uint8_t portIndex) const;
    bool                              IsPortClaimed(uint8_t portIndex) const;
    int                               ClaimedPortCount() const;

    // Drop the binding for a single game port without tearing down the
    // underlying transport (other ports on the same multi-channel adapter
    // stay live). Used by LUS::Controller's auto-fallback when native
    // polling has failed for ~1 second straight, and by the InputEditor's
    // "Disable Raphnet for this port" button. After this returns,
    // IsPortClaimed(port) is false, so the InputEditor stops drawing the
    // Raphnet banner and the standard SDL/keyboard mapping pipeline takes
    // over for that port.
    void ReleasePort(uint8_t portIndex);

    // For ConnectedPhysicalDeviceManager (L5) skip-list. True if vid is the
    // VID of any open transport.
    bool IsClaimedSdlVendor(uint16_t vid) const;
    const std::unordered_set<uint16_t>& GetClaimedVids() const { return mClaimedVids; }

    // Diagnostic accessor — every open transport, including those whose
    // channels did not get bound to a game port.
    const std::vector<std::shared_ptr<RaphnetTransport>>& GetOpenTransports() const {
        return mTransports;
    }

  private:
    bool                                                 mInitialized;
    std::vector<std::shared_ptr<RaphnetTransport>>       mTransports;
    std::array<RaphnetPortBinding, MAXCONTROLLERS>       mPortBindings;
    std::unordered_set<uint16_t>                         mClaimedVids;
};

} // namespace Ship
