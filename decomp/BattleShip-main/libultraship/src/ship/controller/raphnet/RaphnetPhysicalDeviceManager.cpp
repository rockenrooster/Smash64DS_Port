#include "ship/controller/raphnet/RaphnetPhysicalDeviceManager.h"

#include <hidapi.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <thread>

#include "ship/Context.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/controller/raphnet/MockRaphnetTransport.h"
#include "ship/controller/raphnet/RaphnetAdapterEnumerator.h"
#include "ship/utils/StringHelper.h"

namespace Ship {

namespace {

// Local pairing for the deterministic-by-serial sort. Holds a pointer to the
// owning transport plus the channel index reporting CTL_TYPE_N64. We sort by
// (serial, channel) so reboot ordering is plug-independent.
struct N64Candidate {
    std::shared_ptr<RaphnetTransport> Transport;
    uint8_t                           Channel;
};

std::string Wide2Ascii(const std::wstring& w) {
    std::string out;
    out.reserve(w.size());
    for (wchar_t c : w) {
        out.push_back((c >= 0x20 && c <= 0x7E) ? static_cast<char>(c) : '?');
    }
    return out;
}

} // namespace

RaphnetPhysicalDeviceManager::RaphnetPhysicalDeviceManager() : mInitialized(false) {
}

RaphnetPhysicalDeviceManager::~RaphnetPhysicalDeviceManager() {
    Shutdown();
}

bool RaphnetPhysicalDeviceManager::Init() {
    if (mInitialized) {
        SPDLOG_INFO("[raphnet] Init called again; rescanning");
        Shutdown();
    }

    SPDLOG_INFO("[raphnet] Init: hid_init()");
    if (hid_init() != 0) {
        // hid_init returns -1 on failure on every backend.
        SPDLOG_ERROR("[raphnet] hid_init() failed; native Raphnet support disabled this session");
        return false;
    }

    auto adapters = RaphnetAdapterEnumerator::EnumerateAdapters();

    // Mock-adapter injection for hardware-free testing. SSB64_RAPHNET_MOCK
    // env var: parse as integer N (default 0). For N >= 1, prepend N
    // synthetic adapter records to the enumeration list so the open loop
    // builds MockRaphnetTransports for each. Real adapters (if any) are
    // still enumerated and opened normally — the two coexist without issue.
    int mockCount = 0;
    if (const char* mockEnv = std::getenv("SSB64_RAPHNET_MOCK"); mockEnv != nullptr) {
        mockCount = std::atoi(mockEnv);
        if (mockCount < 0) {
            mockCount = 0;
        }
        if (mockCount > MAXCONTROLLERS) {
            mockCount = MAXCONTROLLERS;
        }
        if (mockCount > 0) {
            SPDLOG_INFO("[raphnet] SSB64_RAPHNET_MOCK={}; injecting {} synthetic adapter(s) for "
                        "hardware-free testing",
                        mockEnv, mockCount);
            for (int i = 0; i < mockCount; ++i) {
                RaphnetAdapterInfo info;
                info.HidPath = StringHelper::Sprintf("MOCK:%d", i);
                info.Vid = gRaphnetVid;
                info.Pid = 0xFFFF;  // synthetic PID; real raphnet PIDs do not collide
                info.Serial = L"mock-" + std::to_wstring(i);
                info.ProductName = L"Mock Raphnet Adapter";
                info.InterfaceNumber = 1;
                info.MaxChannelsHint = 1;  // mock only has 1 channel
                adapters.insert(adapters.begin(), std::move(info));
            }
        }
    }

    if (adapters.empty()) {
        SPDLOG_INFO("[raphnet] no adapters found; SDL HID joystick path will handle any plain joysticks");
        // hidapi opens a libudev udev_monitor on hid_init() and keeps it alive
        // for the process lifetime. When no Raphnet adapter is present (the
        // common case) we don't need it — and leaving it alongside SDL's own
        // udev monitor doubles the chance of a slow sysfs read blocking the
        // main thread (issue #137 random hangs). Release it here.
        SPDLOG_INFO("[raphnet] hid_exit() — no Raphnet hardware, releasing hidapi udev monitor");
        hid_exit();
        mInitialized = false;
        return true;
    }

    // Open each adapter and remember the open transport. Probing channels
    // happens after the open loop so we can log the full enumerate→open
    // trace before any per-channel queries spam the log. Mock-tagged paths
    // (HidPath starts with "MOCK:") get a MockRaphnetTransport instead of
    // a real one.
    for (auto& info : adapters) {
        std::shared_ptr<RaphnetTransport> transport;
        if (info.HidPath.rfind("MOCK:", 0) == 0) {
            transport = std::make_shared<MockRaphnetTransport>();
        } else {
            transport = std::make_shared<RaphnetTransport>();
        }
        if (!transport->Open(info.HidPath, info.Vid, info.Pid, info.Serial)) {
            SPDLOG_WARN("[raphnet] failed to open '{}' (vid=0x{:04x} pid=0x{:04x}); skipping",
                        Wide2Ascii(info.Serial), info.Vid, info.Pid);
            continue;
        }
        mTransports.push_back(transport);
        mClaimedVids.insert(info.Vid);
    }

    if (mTransports.empty()) {
        SPDLOG_WARN("[raphnet] {} adapter(s) enumerated but none opened successfully", adapters.size());
        mInitialized = true;
        return true;
    }

    SPDLOG_INFO("[raphnet] {} adapter(s) opened; probing channels for CTL_TYPE_N64", mTransports.size());

    // Probe each channel of each open transport for an N64 controller.
    std::vector<N64Candidate> candidates;
    for (auto& transport : mTransports) {
        for (uint8_t ch = 0; ch < gRntMaxChannelsPerAdapter; ++ch) {
            uint8_t type = gRntCtlTypeNone;
            if (!transport->GetControllerType(ch, type)) {
                // Older firmware (< v3.4) doesn't implement GET_CONTROLLER_TYPE.
                // We can't tell if a controller is plugged in, so fall back to
                // optimistic claim: assume channel 0 has an N64 pad and skip the
                // rest. v3.0..v3.3 firmware is rare; rocking back to RAW_SI
                // probe (issuing N64_GET_CAPABILITIES) is the proper fallback
                // but adds protocol surface. Defer that to a follow-up.
                if (ch == 0) {
                    SPDLOG_WARN("[raphnet] adapter '{}' GET_CONTROLLER_TYPE not supported by firmware "
                                "(version='{}'); claiming channel 0 optimistically",
                                Wide2Ascii(transport->GetSerial()), transport->GetVersionString());
                    candidates.push_back({ transport, 0 });
                }
                break;
            }
            if (type == gRntCtlTypeN64) {
                candidates.push_back({ transport, ch });
            } else if (type == gRntCtlTypeNone) {
                // Empty channel — log but don't claim.
                SPDLOG_DEBUG("[raphnet] adapter '{}' chn={}: empty",
                             Wide2Ascii(transport->GetSerial()), ch);
            } else {
                SPDLOG_INFO("[raphnet] adapter '{}' chn={}: non-N64 controller type 0x{:02x}; "
                            "this is SSB64 — channel skipped",
                            Wide2Ascii(transport->GetSerial()), ch, type);
            }
        }
    }

    // Sort candidates deterministically by (serial, channel) so port
    // assignment is reboot-stable.
    std::sort(candidates.begin(), candidates.end(),
              [](const N64Candidate& a, const N64Candidate& b) {
                  const auto& sa = a.Transport->GetSerial();
                  const auto& sb = b.Transport->GetSerial();
                  if (sa != sb) {
                      return sa < sb;
                  }
                  return a.Channel < b.Channel;
              });

    // Phase 1: honor explicit per-port pinning via CVAR
    // gControllers.PortN.RaphnetSerial. If a candidate's serial matches the
    // pin for port P, claim that exact slot. Pinning trumps the deterministic
    // sort so users with multiple adapters can hard-bind a specific physical
    // adapter to a specific game port.
    auto cvars = Ship::Context::GetInstance()->GetConsoleVariables();
    std::vector<bool> claimed(candidates.size(), false);
    for (int port = 0; port < MAXCONTROLLERS; ++port) {
        std::string cvarKey = StringHelper::Sprintf(
            CVAR_PREFIX_CONTROLLERS ".Port%d.RaphnetSerial", port + 1);
        std::string pinSerial = cvars->GetString(cvarKey.c_str(), "");
        if (pinSerial.empty()) {
            continue;
        }
        for (size_t i = 0; i < candidates.size(); ++i) {
            if (claimed[i]) {
                continue;
            }
            std::string candSerial = Wide2Ascii(candidates[i].Transport->GetSerial());
            if (candSerial == pinSerial) {
                mPortBindings[port].Transport = candidates[i].Transport;
                mPortBindings[port].Channel = candidates[i].Channel;
                claimed[i] = true;
                SPDLOG_INFO("[raphnet] port {} ← PINNED adapter serial='{}' chn={} "
                            "(via {})",
                            port, candSerial, candidates[i].Channel, cvarKey);
                break;
            }
        }
        if (mPortBindings[port].Transport == nullptr) {
            SPDLOG_WARN("[raphnet] {} = '{}' but no enumerated adapter matches; pin ignored",
                        cvarKey, pinSerial);
        }
    }

    // Phase 2: fill remaining unbound ports with unclaimed candidates in
    // sorted order.
    int portIdx = 0;
    for (size_t i = 0; i < candidates.size(); ++i) {
        if (claimed[i]) {
            continue;
        }
        while (portIdx < MAXCONTROLLERS && mPortBindings[portIdx].Transport != nullptr) {
            ++portIdx;
        }
        if (portIdx >= MAXCONTROLLERS) {
            SPDLOG_WARN("[raphnet] more N64 candidates ({}) than game ports ({}); ignoring extras",
                        candidates.size(), MAXCONTROLLERS);
            break;
        }
        mPortBindings[portIdx].Transport = candidates[i].Transport;
        mPortBindings[portIdx].Channel = candidates[i].Channel;
        SPDLOG_INFO("[raphnet] port {} ← adapter serial='{}' chn={} (vid=0x{:04x} pid=0x{:04x})",
                    portIdx, Wide2Ascii(candidates[i].Transport->GetSerial()),
                    candidates[i].Channel, candidates[i].Transport->GetVid(),
                    candidates[i].Transport->GetPid());
        ++portIdx;
    }

    if (portIdx == 0) {
        SPDLOG_INFO("[raphnet] no N64 controllers detected on any open adapter; "
                    "all game ports remain unbound (SDL still handles plain joysticks)");
    }

    mInitialized = true;
    return true;
}

void RaphnetPhysicalDeviceManager::Shutdown() {
    if (!mInitialized && mTransports.empty()) {
        return;
    }

    SPDLOG_INFO("[raphnet] Shutdown: closing {} transport(s)", mTransports.size());

    for (auto& binding : mPortBindings) {
        binding.Transport.reset();
        binding.Channel = 0;
    }

    // Defensive belt-and-suspenders: turn off vibration on every claimed
    // (port, channel) before closing so a long-running rumble effect can't
    // outlive the process. RaphnetTransport::Close() also calls SUSPEND_-
    // POLLING(0) which restores HID joystick mode.
    for (auto& transport : mTransports) {
        if (transport == nullptr) {
            continue;
        }
        for (uint8_t ch = 0; ch < gRntMaxChannelsPerAdapter; ++ch) {
            transport->SetVibration(ch, false);
        }
        transport->Close();
    }
    mTransports.clear();
    mClaimedVids.clear();

    if (hid_exit() != 0) {
        SPDLOG_WARN("[raphnet] hid_exit() returned non-zero");
    } else {
        SPDLOG_INFO("[raphnet] hid_exit() OK");
    }

    mInitialized = false;
}

std::shared_ptr<RaphnetTransport>
RaphnetPhysicalDeviceManager::GetTransportForPort(uint8_t portIndex) const {
    if (portIndex >= MAXCONTROLLERS) {
        return nullptr;
    }
    return mPortBindings[portIndex].Transport;
}

int RaphnetPhysicalDeviceManager::GetChannelForPort(uint8_t portIndex) const {
    if (portIndex >= MAXCONTROLLERS) {
        return -1;
    }
    return mPortBindings[portIndex].Transport != nullptr ? mPortBindings[portIndex].Channel : -1;
}

bool RaphnetPhysicalDeviceManager::IsPortClaimed(uint8_t portIndex) const {
    if (portIndex >= MAXCONTROLLERS) {
        return false;
    }
    return mPortBindings[portIndex].Transport != nullptr;
}

int RaphnetPhysicalDeviceManager::ClaimedPortCount() const {
    int n = 0;
    for (const auto& b : mPortBindings) {
        if (b.Transport != nullptr) {
            ++n;
        }
    }
    return n;
}

bool RaphnetPhysicalDeviceManager::IsClaimedSdlVendor(uint16_t vid) const {
    return mClaimedVids.contains(vid);
}

void RaphnetPhysicalDeviceManager::ReleasePort(uint8_t portIndex) {
    if (portIndex >= MAXCONTROLLERS) {
        return;
    }
    auto& binding = mPortBindings[portIndex];
    if (binding.Transport == nullptr) {
        return;
    }
    SPDLOG_INFO("[raphnet] ReleasePort({}): unbinding port from adapter '{}' chn={} "
                "(transport stays open for other ports / rumble)",
                portIndex, Wide2Ascii(binding.Transport->GetSerial()), binding.Channel);
    binding.Transport.reset();
    binding.Channel = 0;
}

void RaphnetPhysicalDeviceManager::RunSelfTest() {
    SPDLOG_INFO("================== RAPHNET SELF-TEST BEGIN ==================");
    SPDLOG_INFO("[raphnet] open transports: {}", mTransports.size());
    SPDLOG_INFO("[raphnet] claimed game ports: {}", ClaimedPortCount());

    int adapterIdx = 0;
    for (auto& transport : mTransports) {
        SPDLOG_INFO("[raphnet] --- adapter {} (vid=0x{:04x} pid=0x{:04x} serial='{}' version='{}') ---",
                    adapterIdx, transport->GetVid(), transport->GetPid(),
                    Wide2Ascii(transport->GetSerial()), transport->GetVersionString());

        for (uint8_t ch = 0; ch < gRntMaxChannelsPerAdapter; ++ch) {
            uint8_t type = gRntCtlTypeNone;
            transport->GetControllerType(ch, type);
        }

        for (uint8_t ch = 0; ch < gRntMaxChannelsPerAdapter; ++ch) {
            // Only poll channels we know are N64 — avoid spamming non-N64
            // channels. We probe by checking GetControllerType once more.
            uint8_t type = gRntCtlTypeNone;
            if (!transport->GetControllerType(ch, type) || type != gRntCtlTypeN64) {
                continue;
            }
            SPDLOG_INFO("[raphnet] adapter {} chn={}: 10 polls", adapterIdx, ch);
            for (int i = 0; i < 10; ++i) {
                OSContPad pad = {};
                bool ok = transport->Poll(ch, pad);
                if (ok) {
                    SPDLOG_INFO("[raphnet]   poll[{}] button=0x{:04x} stick=({},{})", i,
                                pad.button, pad.stick_x, pad.stick_y);
                } else {
                    SPDLOG_WARN("[raphnet]   poll[{}] FAILED", i);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            }
            SPDLOG_INFO("[raphnet] adapter {} chn={}: vibration test (100 ms on, then off)",
                        adapterIdx, ch);
            transport->SetVibration(ch, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            transport->SetVibration(ch, false);
        }
        ++adapterIdx;
    }
    SPDLOG_INFO("================== RAPHNET SELF-TEST END ==================");
}

} // namespace Ship
