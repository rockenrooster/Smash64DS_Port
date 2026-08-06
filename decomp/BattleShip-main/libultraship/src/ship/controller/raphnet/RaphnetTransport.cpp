#include "ship/controller/raphnet/RaphnetTransport.h"

#include <hidapi.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <cstring>
#include <sstream>
#include <thread>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
extern "C" {
#include <hidsdi.h>
#include <hidpi.h>
}
#pragma comment(lib, "hid.lib")
#endif

namespace Ship {

namespace {

#if defined(_WIN32)
// Ask Windows what feature-report buffer size the HID stack expects for this
// device. hidapi 0.14.0's hid_send_feature_report auto-pads up to caps.-
// FeatureReportByteLength (windows/hid.c:1189-1204) but hid_get_feature_report
// passes our length straight to DeviceIoControl(IOCTL_HID_GET_FEATURE) — so a
// caller-supplied buffer that's even one byte off from caps fails the read
// with ERROR_INVALID_PARAMETER (0x57) while the matching send silently
// works. raphnet's gc_n64_usb-v3.6.1 firmware declares REPORT_COUNT 63 with
// no Report ID, which Windows reports as caps=64 in the common case, but
// driver/OS-version interactions can produce other values (or transiently
// fail until the HID handle settles). Querying caps directly here removes
// that whole class of off-by-one.
//
// Logs all HIDP_CAPS fields at WARN so the values reach release-build logs
// (default level WARN). Returns 0 on any failure; caller falls back to
// auto-negotiation.
int QueryWindowsCapsBufferSize(const std::string& hidPath) {
    HANDLE h = CreateFileA(hidPath.c_str(), 0,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                           OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        DWORD lastErr = GetLastError();
        SPDLOG_WARN("[raphnet-diag] CreateFileA('{}') for caps query failed: GetLastError={}",
                    hidPath, (unsigned)lastErr);
        return 0;
    }
    int size = 0;
    PHIDP_PREPARSED_DATA preparsed = nullptr;
    if (!HidD_GetPreparsedData(h, &preparsed) || preparsed == nullptr) {
        DWORD lastErr = GetLastError();
        SPDLOG_WARN("[raphnet-diag] HidD_GetPreparsedData failed for '{}': GetLastError={}",
                    hidPath, (unsigned)lastErr);
        CloseHandle(h);
        return 0;
    }
    HIDP_CAPS caps = {};
    NTSTATUS st = HidP_GetCaps(preparsed, &caps);
    if (st == HIDP_STATUS_SUCCESS) {
        size = static_cast<int>(caps.FeatureReportByteLength);
        SPDLOG_WARN("[raphnet-diag] HidP_GetCaps OK: UsagePage=0x{:04x} Usage=0x{:04x} "
                    "InputReportByteLength={} OutputReportByteLength={} "
                    "FeatureReportByteLength={} NumberLinkCollectionNodes={} "
                    "NumberInputButtonCaps={} NumberInputValueCaps={} "
                    "NumberFeatureButtonCaps={} NumberFeatureValueCaps={}",
                    (unsigned)caps.UsagePage, (unsigned)caps.Usage,
                    (unsigned)caps.InputReportByteLength,
                    (unsigned)caps.OutputReportByteLength,
                    (unsigned)caps.FeatureReportByteLength,
                    (unsigned)caps.NumberLinkCollectionNodes,
                    (unsigned)caps.NumberInputButtonCaps,
                    (unsigned)caps.NumberInputValueCaps,
                    (unsigned)caps.NumberFeatureButtonCaps,
                    (unsigned)caps.NumberFeatureValueCaps);
    } else {
        SPDLOG_WARN("[raphnet-diag] HidP_GetCaps NTSTATUS=0x{:08x}", (unsigned)st);
    }
    HidD_FreePreparsedData(preparsed);
    CloseHandle(h);
    return size;
}
#endif

// Format `len` bytes of buf as "AA BB CC DD …" for log messages. Caps at 32
// bytes to keep log lines bounded; longer payloads get a trailing ellipsis.
std::string HexBytes(const uint8_t* buf, size_t len) {
    constexpr size_t kCap = 32;
    std::ostringstream os;
    os << std::hex << std::uppercase;
    size_t shown = std::min(len, kCap);
    for (size_t i = 0; i < shown; ++i) {
        if (i > 0) {
            os << ' ';
        }
        if (buf[i] < 0x10) {
            os << '0';
        }
        os << (unsigned)buf[i];
    }
    if (len > kCap) {
        os << " ... (+" << std::dec << (len - kCap) << ')';
    }
    return os.str();
}

} // namespace

RaphnetTransport::RaphnetTransport()
    : mDevice(nullptr), mVid(0), mPid(0), mReportSize(gRntDefaultReportSize) {
    for (int i = 0; i < gRntMaxChannelsPerAdapter; ++i) {
        mFirstPollLogged[i] = false;
        mFirstErrorLogged[i] = false;
        mErrorCount[i] = 0;
    }
}

RaphnetTransport::~RaphnetTransport() {
    Close();
}

std::string RaphnetTransport::ToUtf8(const std::wstring& w) {
    // hidapi's wide-string outputs (error text, product name, serial) are
    // ASCII for raphnet adapters in practice. Defensive cast keeps non-ASCII
    // characters visible as '?' rather than crashing on conversion.
    std::string out;
    out.reserve(w.size());
    for (wchar_t c : w) {
        out.push_back((c >= 0x20 && c <= 0x7E) ? static_cast<char>(c) : '?');
    }
    return out;
}

std::string RaphnetTransport::FormatHidError(hid_device* dev) {
    if (dev == nullptr) {
        return "(no device)";
    }
    const wchar_t* w = hid_error(dev);
    if (w == nullptr) {
        return "(no error string)";
    }
    return ToUtf8(std::wstring(w));
}

bool RaphnetTransport::Open(const std::string& hidPath, uint16_t vid, uint16_t pid,
                            const std::wstring& serial) {
    std::lock_guard<std::mutex> lock(mLock);

    if (mDevice != nullptr) {
        SPDLOG_WARN("[raphnet] Open called on an already-open transport (path={}); closing first", hidPath);
        hid_close(mDevice);
        mDevice = nullptr;
    }

    mHidPath = hidPath;
    mVid = vid;
    mPid = pid;
    mSerial = serial;

    // Open()-time logs use WARN so they reach release-build log files (which
    // default to spdlog::level::warn — see Context::InitLogging). Open runs
    // once per session so noise is bounded; per-frame Poll logs stay at
    // INFO/TRACE. The "[raphnet-diag]" prefix is grep-friendly for triage.
    SPDLOG_WARN("[raphnet-diag] Open() vid=0x{:04x} pid=0x{:04x} serial='{}' path='{}'",
                vid, pid, ToUtf8(serial), hidPath);

    mDevice = hid_open_path(hidPath.c_str());
    if (mDevice == nullptr) {
        // Common cause on Linux is missing udev rule (EACCES on /dev/hidraw*).
        // hid_error(NULL) returns a global error message in this case.
        const wchar_t* err = hid_error(nullptr);
        std::string errStr = (err != nullptr) ? ToUtf8(std::wstring(err)) : "(no global error)";
        SPDLOG_ERROR("[raphnet] hid_open_path('{}') failed: {}", hidPath, errStr);
#if defined(__linux__) || defined(__OpenBSD__)
        SPDLOG_ERROR("[raphnet] on Linux/BSD, ensure raphnet udev rules are installed: "
                     "https://www.raphnet.net/electronique/gc_n64_usb/index_en.php (scripts/10-raphnet.rules)");
#endif
        return false;
    }
    SPDLOG_WARN("[raphnet-diag] hid_open_path OK");

#if defined(_WIN32)
    // Brief settle delay after hid_open_path. Without this, the very first
    // DeviceIoControl on the freshly-opened HID handle has been observed to
    // return ERROR_INVALID_PARAMETER intermittently on real raphnet v3.6.1
    // hardware (user reports vid=0x289b pid=0x0061, "N64 to USB v3.6"); the
    // same handle works on retry milliseconds later. 50 ms is invisible to
    // the user but covers the Windows HID stack's post-open initialization.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    SPDLOG_WARN("[raphnet-diag] post-open settle delay 50 ms complete");
#endif

    // 1. Determine the feature-report buffer size for this device.
    //    On Windows the HID stack reports it authoritatively via HidP_GetCaps;
    //    we query directly so reads use the exact size the OS expects (any
    //    mismatch fails hid_get_feature_report with ERROR_INVALID_PARAMETER).
    //    On Linux/macOS hidapi's hidraw/IOKit backends are size-tolerant, so
    //    we keep the SUSPEND_POLLING(0) opcode-echo probe to discover the
    //    firmware's accepted size when caps aren't directly queryable. As a
    //    side effect either path also clears any prior wedged state from a
    //    previous run that crashed before SUSPEND_POLLING(0).
    bool sized = false;
#if defined(_WIN32)
    {
        int caps_size = QueryWindowsCapsBufferSize(hidPath);
        if (caps_size > 1) {
            // caps reports total report length including the 1-byte report id.
            mReportSize = caps_size - 1;
            SPDLOG_WARN("[raphnet-diag] sized via HidP_GetCaps: caps_size={} mReportSize={} "
                        "(skipping auto-negotiation)",
                        caps_size, mReportSize);
            // Defensive SUSPEND_POLLING(0) at the queried size to clear any
            // wedged state. We don't gate Open() on its reply — the device
            // may legitimately not respond if it was already in a clean state
            // and Windows handed us a stale handle that the stack will refresh
            // on the next request.
            uint8_t cmd[2] = { gRntRqSuspendPolling, 0 };
            uint8_t rep[8] = {};
            int n = Exchange(cmd, sizeof(cmd), rep, sizeof(rep), 1000);
            SPDLOG_WARN("[raphnet-diag] defensive SUSPEND_POLLING(0): n={} rep[0]=0x{:02x} "
                        "rep_bytes='{}'",
                        n, n > 0 ? rep[0] : 0,
                        HexBytes(rep, n > 0 ? (size_t)n : 0));
            sized = true;
        } else {
            SPDLOG_WARN("[raphnet-diag] HidP_GetCaps returned {} for path '{}' — falling back to "
                        "auto-negotiation",
                        caps_size, hidPath);
        }
    }
#endif
    if (!sized) {
        // Fallback (and the non-Windows path): auto-negotiate by probing
        // SUSPEND_POLLING(0) at each candidate size and keep the first that
        // echoes the opcode.
        constexpr int kCandidates[] = { gRntDefaultReportSize, gRntLegacyReportSize };
        bool negotiated = false;
        int lastN = 0;
        uint8_t lastRep0 = 0;
        for (int candidate : kCandidates) {
            mReportSize = candidate;
            uint8_t cmd[2] = { gRntRqSuspendPolling, 0 };
            uint8_t rep[8] = {};
            int n = Exchange(cmd, sizeof(cmd), rep, sizeof(rep), 1000);
            lastN = n;
            lastRep0 = (n > 0) ? rep[0] : 0;
            SPDLOG_WARN("[raphnet-diag] auto-neg probe size={}: n={} rep[0]=0x{:02x} "
                        "rep_bytes='{}'",
                        candidate, n, lastRep0,
                        HexBytes(rep, n > 0 ? (size_t)n : 0));
            if (n >= 1 && rep[0] == gRntRqSuspendPolling) {
                SPDLOG_WARN("[raphnet-diag] auto-neg accepted size={} (opcode echo confirmed)",
                            mReportSize);
                negotiated = true;
                break;
            }
        }
        if (!negotiated) {
            SPDLOG_ERROR("[raphnet] auto-negotiation failed: tried {} and {} bytes, "
                         "neither produced a valid SUSPEND_POLLING(0) response "
                         "(last n={}, last rep[0]=0x{:02x}). Adapter firmware unsupported "
                         "or device wedged — closing. Replug the adapter and try again; "
                         "if that doesn't help, file a bug with the adapter's serial "
                         "and firmware version (visible on raphnet's website by "
                         "running their gcn64ctl --get_version).",
                         gRntDefaultReportSize, gRntLegacyReportSize, lastN, lastRep0);
            hid_close(mDevice);
            mDevice = nullptr;
            return false;
        }
    }

    // 2. Read firmware version.
    if (!GetVersion(mVersionString)) {
        SPDLOG_ERROR("[raphnet] GET_VERSION failed on '{}' — closing", hidPath);
        hid_close(mDevice);
        mDevice = nullptr;
        return false;
    }
    SPDLOG_WARN("[raphnet-diag] firmware version: '{}' (adapter serial='{}' vid=0x{:04x} "
                "pid=0x{:04x})",
                mVersionString, ToUtf8(serial), vid, pid);

    // 3. Suspend firmware auto-polling so RAW_SI commands have exclusive
    //    SI bus access.
    {
        uint8_t cmd[2] = { gRntRqSuspendPolling, 1 };
        uint8_t rep[8] = {};
        int n = Exchange(cmd, sizeof(cmd), rep, sizeof(rep), 1000);
        if (n < 0) {
            SPDLOG_ERROR("[raphnet] SUSPEND_POLLING(1) failed (n={}, err={})", n, FormatHidError(mDevice));
            hid_close(mDevice);
            mDevice = nullptr;
            return false;
        }
        SPDLOG_WARN("[raphnet-diag] SUSPEND_POLLING(1) reply: n={} rep[0]=0x{:02x} "
                    "rep_bytes='{}' — native SI access armed",
                    n, n > 0 ? rep[0] : 0,
                    HexBytes(rep, n > 0 ? (size_t)n : 0));
    }

    SPDLOG_WARN("[raphnet-diag] Open() complete — entering Poll loop with mReportSize={}",
                mReportSize);
    return true;
}

void RaphnetTransport::Close() {
    std::lock_guard<std::mutex> lock(mLock);
    if (mDevice == nullptr) {
        return;
    }

    // Resume firmware auto-polling so the adapter is usable as a plain HID
    // joystick after the game exits. Important: a process that exits without
    // sending this leaves the firmware wedged until next physical replug.
    {
        uint8_t cmd[2] = { gRntRqSuspendPolling, 0 };
        uint8_t rep[8] = {};
        int n = Exchange(cmd, sizeof(cmd), rep, sizeof(rep), 1000);
        if (n < 0) {
            SPDLOG_WARN("[raphnet] SUSPEND_POLLING(0) on close failed (n={}, err={}); "
                        "adapter may need replug before next use as HID joystick",
                        n, FormatHidError(mDevice));
        } else {
            SPDLOG_INFO("[raphnet] adapter polling resumed (HID joystick mode restored)");
        }
    }

    hid_close(mDevice);
    mDevice = nullptr;
    SPDLOG_INFO("[raphnet] adapter closed: path='{}'", mHidPath);
}

bool RaphnetTransport::GetVersion(std::string& outVersion) {
    // Caller may or may not hold mLock — Exchange handles the lock idempotently
    // via std::lock_guard. We must not double-lock here since Exchange takes
    // mLock too. Open() calls this BEFORE releasing — so we need the public
    // GetVersion path to lock and the internal Open path to call Exchange
    // directly. Restructure: Exchange does NOT take the lock internally; the
    // caller holds it. Public methods take the lock.
    //
    // Above, Open() already holds mLock. Below, public callers must enter via
    // a thin wrapper that takes mLock. The only public caller right now is
    // Open() itself (internal usage). Other future public callers should use
    // a Locked variant. For now, this method is internal-helper-only.
    //
    // (There's no nice way to express "take this lock, but only if not already
    //  held" in standard C++, so we structure call sites to be unambiguous.)

    uint8_t cmd[1] = { gRntRqGetVersion };
    uint8_t rep[64] = {};
    int n = Exchange(cmd, sizeof(cmd), rep, sizeof(rep), 1000);
    if (n < 0 || n < 1) {
        SPDLOG_ERROR("[raphnet] GET_VERSION exchange failed (n={}, err={})", n,
                     FormatHidError(mDevice));
        return false;
    }
    SPDLOG_WARN("[raphnet-diag] GET_VERSION reply: n={} bytes='{}'", n, HexBytes(rep, n));
    // Format follows v3.x firmware: ASCII null-terminated string starting at
    // rep[1] (rep[0] echoes the opcode 0x04). Older firmware may pack version
    // bytes raw; we accept either.
    if (n >= 2 && rep[0] == gRntRqGetVersion) {
        size_t maxScan = static_cast<size_t>(n - 1);
        size_t end = 1;
        while (end < static_cast<size_t>(n) && rep[end] != 0) {
            ++end;
        }
        outVersion.assign(reinterpret_cast<const char*>(rep + 1), end - 1);
        // Sanity: if the version doesn't look printable, fall through to
        // raw-byte format.
        bool printable = !outVersion.empty();
        for (char c : outVersion) {
            if (c < 0x20 || c > 0x7E) {
                printable = false;
                break;
            }
        }
        if (printable) {
            return true;
        }
    }
    // Fallback: present the raw bytes as a hex string.
    std::ostringstream os;
    os << "raw:" << HexBytes(rep, n);
    outVersion = os.str();
    return true;
}

bool RaphnetTransport::GetControllerType(uint8_t channel, uint8_t& outType) {
    std::lock_guard<std::mutex> lock(mLock);
    if (mDevice == nullptr) {
        outType = gRntCtlTypeNone;
        return false;
    }
    uint8_t cmd[2] = { gRntRqGetControllerType, channel };
    uint8_t rep[16] = {};
    int n = Exchange(cmd, sizeof(cmd), rep, sizeof(rep), 1000);
    if (n < 0) {
        SPDLOG_ERROR("[raphnet] GET_CONTROLLER_TYPE chn={} failed (n={}, err={})", channel, n,
                     FormatHidError(mDevice));
        outType = gRntCtlTypeNone;
        return false;
    }
    SPDLOG_DEBUG("[raphnet] GET_CONTROLLER_TYPE chn={} response ({} bytes): {}", channel, n,
                 HexBytes(rep, n));
    // Response layout: rep[0]=opcode echo (0x06), rep[1]=channel echo, rep[2]=type.
    // Older v3 firmware (< v3.4) doesn't implement this command and may
    // return garbage / repeat the opcode; we guard via opcode-echo check.
    if (n >= 3 && rep[0] == gRntRqGetControllerType && rep[1] == channel) {
        outType = rep[2];
        const char* typeName = "unknown";
        switch (outType) {
            case gRntCtlTypeNone:     typeName = "NONE"; break;
            case gRntCtlTypeN64:      typeName = "N64"; break;
            case gRntCtlTypeGameCube: typeName = "GAMECUBE"; break;
            default:                  typeName = "OTHER"; break;
        }
        SPDLOG_INFO("[raphnet] adapter '{}' chn={} controller type: {} (0x{:02x})",
                    ToUtf8(mSerial), channel, typeName, outType);
        return true;
    }
    SPDLOG_WARN("[raphnet] GET_CONTROLLER_TYPE chn={} unexpected response: {}", channel,
                HexBytes(rep, n));
    outType = gRntCtlTypeNone;
    return false;
}

bool RaphnetTransport::SuspendPolling(bool suspend) {
    std::lock_guard<std::mutex> lock(mLock);
    if (mDevice == nullptr) {
        return false;
    }
    uint8_t cmd[2] = { gRntRqSuspendPolling, static_cast<uint8_t>(suspend ? 1 : 0) };
    uint8_t rep[8] = {};
    int n = Exchange(cmd, sizeof(cmd), rep, sizeof(rep), 1000);
    if (n < 0) {
        SPDLOG_ERROR("[raphnet] SUSPEND_POLLING({}) failed (n={}, err={})", suspend ? 1 : 0, n,
                     FormatHidError(mDevice));
        return false;
    }
    SPDLOG_DEBUG("[raphnet] SUSPEND_POLLING({}) OK", suspend ? 1 : 0);
    return true;
}

bool RaphnetTransport::SetVibration(uint8_t channel, bool on) {
    std::lock_guard<std::mutex> lock(mLock);
    if (mDevice == nullptr) {
        return false;
    }
    uint8_t cmd[3] = { gRntRqSetVibration, channel, static_cast<uint8_t>(on ? 1 : 0) };
    uint8_t rep[8] = {};
    int n = Exchange(cmd, sizeof(cmd), rep, sizeof(rep), 1000);
    if (n < 0) {
        SPDLOG_WARN("[raphnet] SET_VIBRATION chn={} on={} failed (n={}, err={})", channel, on ? 1 : 0,
                    n, FormatHidError(mDevice));
        return false;
    }
    SPDLOG_TRACE("[raphnet] SET_VIBRATION chn={} on={} OK", channel, on ? 1 : 0);
    return true;
}

bool RaphnetTransport::Poll(uint8_t channel, OSContPad& pad) {
    std::lock_guard<std::mutex> lock(mLock);
    if (mDevice == nullptr) {
        return false;
    }
    if (channel >= gRntMaxChannelsPerAdapter) {
        SPDLOG_ERROR("[raphnet] Poll: channel {} out of range (max {})", channel,
                     gRntMaxChannelsPerAdapter - 1);
        return false;
    }

    // Build RAW_SI request: [0x80][channel][tx_len=1][N64_GET_STATUS=0x01].
    uint8_t cmd[4] = { gRntRqGcn64RawSi, channel, 1, gN64GetStatus };
    // Response: [0x80 echo][channel echo][rx_len][btn_hi][btn_lo][stickX][stickY].
    uint8_t rep[16] = {};
    int n = Exchange(cmd, sizeof(cmd), rep, sizeof(rep), 50);

    if (n < 0 || n < 3 || rep[0] != gRntRqGcn64RawSi || rep[1] != channel ||
        rep[2] != gN64GetStatusReplyLen || n < 3 + gN64GetStatusReplyLen) {
        ++mErrorCount[channel];
        if (!mFirstErrorLogged[channel]) {
            mFirstErrorLogged[channel] = true;
            SPDLOG_ERROR(
                "[raphnet] FIRST poll FAILURE chn={} on adapter '{}' vid=0x{:04x} pid=0x{:04x}: "
                "n={} response='{}' (request='{}', mReportSize={}). Subsequent failures "
                "throttled to 1/60.",
                channel, ToUtf8(mSerial), mVid, mPid, n, HexBytes(rep, n > 0 ? (size_t)n : 0),
                HexBytes(cmd, sizeof(cmd)), mReportSize);
        } else if ((mErrorCount[channel] % 60) == 0) {
            SPDLOG_WARN("[raphnet] poll FAILURE chn={} (count={}, n={}, err={})", channel,
                        mErrorCount[channel], n, FormatHidError(mDevice));
        }
        return false;
    }

    const uint8_t* status = rep + 3;
    pad.button = static_cast<uint16_t>((status[0] << 8) | status[1]);
    pad.stick_x = static_cast<int8_t>(status[2]);
    pad.stick_y = static_cast<int8_t>(status[3]);
    pad.err_no = 0;
    pad.gyro_x = 0.0f;
    pad.gyro_y = 0.0f;
    pad.right_stick_x = 0;
    pad.right_stick_y = 0;

    if (!mFirstPollLogged[channel]) {
        mFirstPollLogged[channel] = true;
        // WARN-level so this lands in release log files (default WARN+) — it's
        // the canonical signal that native Raphnet polling is alive on this
        // port. Subsequent polls drop back to TRACE.
        SPDLOG_WARN("[raphnet-diag] FIRST successful poll chn={} on adapter '{}': "
                    "request='{}' response='{}' → button=0x{:04x} stick=({},{})",
                    channel, ToUtf8(mSerial), HexBytes(cmd, sizeof(cmd)), HexBytes(rep, n),
                    pad.button, pad.stick_x, pad.stick_y);
    } else {
        SPDLOG_TRACE("[raphnet] poll chn={} button=0x{:04x} stick=({},{})", channel, pad.button,
                     pad.stick_x, pad.stick_y);
    }
    return true;
}

int RaphnetTransport::Exchange(const uint8_t* tx, size_t txLen, uint8_t* rx, size_t rxMax,
                               int timeoutMs) {
    // Caller MUST hold mLock. We don't take it here — see GetVersion()'s
    // commentary about the lock contract.
    if (mDevice == nullptr) {
        return -1;
    }
    if (txLen == 0 || tx == nullptr) {
        return -1;
    }

    // Send: [report_id=0x00][...tx], padded to mReportSize+1.
    std::vector<uint8_t> sendBuf(mReportSize + 1, 0);
    sendBuf[0] = 0x00;
    size_t copyLen = std::min(txLen, static_cast<size_t>(mReportSize));
    std::memcpy(sendBuf.data() + 1, tx, copyLen);

    int sent = hid_send_feature_report(mDevice, sendBuf.data(), sendBuf.size());
    if (sent < 0) {
        SPDLOG_ERROR("[raphnet] hid_send_feature_report failed (sent={}, err={}, "
                     "buffer_size={}, cmd='{}')",
                     sent, FormatHidError(mDevice), sendBuf.size(),
                     HexBytes(tx, copyLen));
        return -1;
    }
    SPDLOG_TRACE("[raphnet] TX {} bytes (cmd='{}')", sent, HexBytes(tx, copyLen));

    // Receive: hid_get_feature_report is BLOCKING/synchronous on every backend;
    // hid_set_nonblocking only affects hid_read. The device usually replies in
    // <1 ms, but commands like GET_VERSION can take longer than the round-trip
    // — the firmware needs a moment to populate the response feature report.
    // Until it does, hidapi reports a "no payload" read: on Windows the
    // backend adds 1 to the byte count for the report-id, so an empty response
    // surfaces as got == 1, while Linux/macOS surface it as got == 0. Either
    // way the payload length (got - 1) is 0 and we must retry until the device
    // produces real data or the deadline expires. This matches raphnet's
    // gcn64tools rnt_exchange (rntlib/raphnetadapter.c), which polls
    // HidD_GetFeature in a loop until res_len > 0. timeoutMs bounds the wait;
    // a wedged adapter is rare so log loudly when the deadline trips.
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    int retryCount = 0;
    while (true) {
        std::vector<uint8_t> recvBuf(mReportSize + 1, 0);
        recvBuf[0] = 0x00;  // Windows requires the report id to be set on input.
        int got = hid_get_feature_report(mDevice, recvBuf.data(), recvBuf.size());
        if (got > 1) {
            int payloadLen = got - 1;
            size_t copyOut = std::min(static_cast<size_t>(payloadLen), rxMax);
            std::memcpy(rx, recvBuf.data() + 1, copyOut);
            SPDLOG_TRACE("[raphnet] RX {} bytes (payload='{}')", got, HexBytes(rx, copyOut));
            return static_cast<int>(copyOut);
        }
        if (got < 0) {
            // Include buffer_size + cmd in the error so we can correlate Mode A
            // INVALID_PARAMETER failures against the actual size we passed (vs.
            // what HidP_GetCaps reports — see [raphnet-diag] caps line above).
            SPDLOG_ERROR("[raphnet] hid_get_feature_report failed (got={}, err={}, "
                         "buffer_size={}, retry_count={}, cmd='{}')",
                         got, FormatHidError(mDevice), recvBuf.size(), retryCount,
                         HexBytes(tx, copyLen));
            return -1;
        }
        // got == 0 (Linux/macOS) or got == 1 (Windows hidapi report-id+1
        // accounting): device hasn't produced a response yet. Sleep briefly
        // and retry until deadline.
        ++retryCount;
        if (std::chrono::steady_clock::now() >= deadline) {
            SPDLOG_WARN("[raphnet] hid_get_feature_report timed out after {} ms "
                        "(retry_count={}, last_got={}, cmd='{}', buffer_size={})",
                        timeoutMs, retryCount, got, HexBytes(tx, copyLen), recvBuf.size());
            return 0;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

} // namespace Ship
