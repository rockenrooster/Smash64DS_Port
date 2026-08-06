#pragma once

// RaphnetAdapterEnumerator
// ------------------------
// Discovers raphnet GC/N64-to-USB adapters connected to the host. The adapter
// firmware (github.com/raphnet/gc_n64_usb-v3) exposes two HID interfaces:
//   * interface 0 — generic HID joystick (what SDL sees as a gamepad)
//   * interface ≥ 1 — raphnet vendor command channel (what we use for native
//     N64 access via RQ_GCN64_RAW_SI_COMMAND etc.)
// We enumerate only command-channel interfaces; the joystick path is what
// users get today through SDL and is suppressed once the device is claimed
// (see ConnectedPhysicalDeviceManager VID skip-list, L5).

#include <cstdint>
#include <string>
#include <vector>

namespace Ship {

// raphnet vendor IDs (firmware: github.com/raphnet/gc_n64_usb-v3)
constexpr uint16_t gRaphnetVid          = 0x289b;  // current
constexpr uint16_t gRaphnetVidLegacy1781 = 0x1781;
constexpr uint16_t gRaphnetVidLegacy1740 = 0x1740;

struct RaphnetAdapterInfo {
    std::string  HidPath;          // hidapi-opaque path; pass to hid_open_path
    uint16_t     Vid;
    uint16_t     Pid;
    std::wstring Serial;           // empty if access denied / device omits it
    std::wstring ProductName;
    int          InterfaceNumber;  // 1 for single-port, 2 for multi-port v3
    int          MaxChannelsHint;  // upper bound to probe (4); real channels
                                   // are discovered via GetControllerType
};

class RaphnetAdapterEnumerator {
  public:
    // Calls hid_init() if not already, then hid_enumerate over each known
    // raphnet VID. Filters to interface_number > 0 (skips the HID joystick
    // surface). Safe to call repeatedly; performs no allocations on the
    // returned objects beyond what the result vector requires.
    static std::vector<RaphnetAdapterInfo> EnumerateAdapters();

    // True if vid is a raphnet vendor id we recognize.
    static bool IsRaphnetVendor(uint16_t vid);
};

} // namespace Ship
