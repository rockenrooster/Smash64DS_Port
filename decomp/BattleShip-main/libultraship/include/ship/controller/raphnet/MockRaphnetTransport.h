#pragma once

// MockRaphnetTransport
// --------------------
// Hardware-free RaphnetTransport for end-to-end testing the entire native-
// raphnet code path on a developer machine. Activated by setting the env
// var SSB64_RAPHNET_MOCK=1 (or =N for N synthetic adapters); the manager
// then creates these instead of real RaphnetTransports for those slots.
//
// Behavior:
//   * Open()                — skips hid_open_path, just records identifying
//                             info and sets mVersionString = "MOCK 3.6.0".
//   * GetControllerType(0)  — returns CTL_TYPE_N64 so the manager claims it.
//   * GetControllerType(>0) — returns CTL_TYPE_NONE.
//   * Poll(0, &pad)         — reads SDL keyboard state via SDL_GetKeyboard-
//                             State and synthesizes an OSContPad. Default
//                             mapping is documented in the cpp.
//   * SetVibration(...)     — logs only (so testers can confirm rumble
//                             dispatch reached this point in the pipeline).
//   * Close()               — logs only.
//
// What this catches: every code path in our libultraship-side stack —
// PreInitRaphnet, port claim sort, SDL skip-list (mock vid 0x289b matches
// real raphnet vid so ConnectedPhysicalDeviceManager skip-lists fire),
// LUS::Controller raphnet read branch, RaphnetRumbleMapping dispatch,
// ShutdownRaphnet ordering. What it does NOT catch: real hidapi behavior,
// USB stack quirks, firmware-side state machine. Use a real adapter +
// SelfTest CVAR for those.

#include "ship/controller/raphnet/RaphnetTransport.h"

namespace Ship {

class MockRaphnetTransport final : public RaphnetTransport {
  public:
    MockRaphnetTransport() = default;
    ~MockRaphnetTransport() override = default;

    bool Open(const std::string& hidPath, uint16_t vid, uint16_t pid,
              const std::wstring& serial) override;
    void Close() override;
    bool IsOpen() const override { return mIsOpen; }

    bool GetVersion(std::string& outVersion) override;
    bool GetControllerType(uint8_t channel, uint8_t& outType) override;
    bool SuspendPolling(bool suspend) override;
    bool SetVibration(uint8_t channel, bool on) override;

    bool Poll(uint8_t channel, OSContPad& pad) override;

  private:
    bool mIsOpen = false;
};

} // namespace Ship
