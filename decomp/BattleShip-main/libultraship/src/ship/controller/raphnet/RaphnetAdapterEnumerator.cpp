#include "ship/controller/raphnet/RaphnetAdapterEnumerator.h"

#include <hidapi.h>
#include <spdlog/spdlog.h>

#include <cstring>

namespace Ship {

bool RaphnetAdapterEnumerator::IsRaphnetVendor(uint16_t vid) {
    return vid == gRaphnetVid || vid == gRaphnetVidLegacy1781 || vid == gRaphnetVidLegacy1740;
}

namespace {

// Pull every device under one VID and append matching command-channel HID
// interfaces to out. Skips interface 0 (the joystick surface) so we don't
// open the joystick interface and risk EBUSY collisions with SDL.
void EnumerateOneVid(uint16_t vid, std::vector<RaphnetAdapterInfo>& out) {
    hid_device_info* head = hid_enumerate(vid, 0);
    if (head == nullptr) {
        SPDLOG_TRACE("[raphnet] hid_enumerate(vid=0x{:04x}) returned no devices", vid);
        return;
    }

    int totalSeen = 0;
    int kept = 0;
    for (hid_device_info* cur = head; cur != nullptr; cur = cur->next) {
        ++totalSeen;
        // The HID joystick surface lives on interface 0 on every multi-
        // interface raphnet product. Vendor commands live on interface 1
        // (single-port adapter) or 2 (4-port adapter). Single-interface
        // legacy adapters with command channel on interface 0 / -1 are not
        // supported in native mode — users with those should fall back to
        // SDL HID joystick mode.
        if (cur->interface_number <= 0) {
            SPDLOG_DEBUG("[raphnet] skipping vid=0x{:04x} pid=0x{:04x} interface={} (joystick surface)",
                         cur->vendor_id, cur->product_id, cur->interface_number);
            continue;
        }
        RaphnetAdapterInfo info;
        info.HidPath = (cur->path != nullptr) ? cur->path : "";
        info.Vid = cur->vendor_id;
        info.Pid = cur->product_id;
        info.Serial = (cur->serial_number != nullptr) ? std::wstring(cur->serial_number) : std::wstring();
        info.ProductName = (cur->product_string != nullptr) ? std::wstring(cur->product_string) : std::wstring();
        info.InterfaceNumber = cur->interface_number;
        info.MaxChannelsHint = 4;  // probed per-channel later via GetControllerType
        out.push_back(std::move(info));
        ++kept;
    }
    hid_free_enumeration(head);

    SPDLOG_DEBUG("[raphnet] enumerate vid=0x{:04x}: {} HID interface(s) seen, {} kept", vid, totalSeen, kept);
}

} // namespace

std::vector<RaphnetAdapterInfo> RaphnetAdapterEnumerator::EnumerateAdapters() {
    std::vector<RaphnetAdapterInfo> result;
    EnumerateOneVid(gRaphnetVid, result);
    EnumerateOneVid(gRaphnetVidLegacy1781, result);
    EnumerateOneVid(gRaphnetVidLegacy1740, result);
    SPDLOG_INFO("[raphnet] enumerate: {} adapter command interface(s) found", result.size());
    return result;
}

} // namespace Ship
