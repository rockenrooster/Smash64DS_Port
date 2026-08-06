#pragma once

// RaphnetTransport
// ----------------
// Owns one open hid_device handle to a single raphnet adapter and serializes
// vendor-protocol exchanges over it. Exchanges use HID feature reports per
// the raphnet protocol (host library: github.com/raphnet/gcn64tools).
//
// The exchange format on the wire:
//   TX: hid_send_feature_report(buf) where buf[0]=0x00 (HID report id),
//       buf[1]=RQ_RNT_*, buf[2..]=payload, padded to mReportSize+1.
//   RX: hid_get_feature_report(buf) — synchronous; on success buf[1] echoes
//       the request opcode and buf[2..] is the response payload.
//
// Threading: every command-level method takes mLock. Polling, rumble,
// version queries are mutex-serialized so a controller-read on the game
// thread cannot interleave with a rumble write from the audio path.
//
// Logging policy (see plan: heavy SPDLOG so remote testers' logs are
// diagnostic on the first try):
//   * INFO at every state transition (Open/Close/Suspend/version negotiated).
//   * INFO once per (channel) at first successful Poll, with full bytes.
//   * WARN throttled (1 / 60 frames after first) on transient errors.
//   * ERROR on the FIRST poll failure of each channel with full context.
//   * TRACE per-frame poll bytes.

#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>

#include "libultraship/libultra/controller.h"  // OSContPad

struct hid_device_;
typedef struct hid_device_ hid_device;

namespace Ship {

// raphnet vendor request opcodes (gcn64tools/src/rntlib/requests.h).
constexpr uint8_t gRntRqSuspendPolling     = 0x03;
constexpr uint8_t gRntRqGetVersion         = 0x04;
constexpr uint8_t gRntRqGetControllerType  = 0x06;
constexpr uint8_t gRntRqSetVibration       = 0x07;
constexpr uint8_t gRntRqGcn64RawSi         = 0x80;

// N64 SI controller protocol opcodes (gcn64tools/src/rntlib/gcn64_protocol.h).
constexpr uint8_t gN64GetCapabilities = 0x00;
constexpr uint8_t gN64GetStatus       = 0x01;
constexpr uint8_t gN64GetStatusReplyLen = 4;

// Returned by RQ_RNT_GET_CONTROLLER_TYPE.
constexpr uint8_t gRntCtlTypeNone     = 0;
constexpr uint8_t gRntCtlTypeN64      = 1;
constexpr uint8_t gRntCtlTypeGameCube = 2;

// Max channels per adapter (4-port versions exist; single-port reports 1).
constexpr int gRntMaxChannelsPerAdapter = 4;

// Report-size candidates for the raphnet command interface. The adapter
// accepts exactly the descriptor-declared payload size after the report-id
// byte; v3 firmware uses 63, legacy (pre-v3) firmware uses 32. The protocol
// has no version handshake, so Open() probes a benign command (SUSPEND_-
// POLLING(0)) at each size in order and keeps the first that gets an
// opcode-echo response. List is ordered newest-first so modern hardware
// settles on the first try.
constexpr int gRntDefaultReportSize = 63;
constexpr int gRntLegacyReportSize  = 32;

class RaphnetTransport {
  public:
    RaphnetTransport();
    virtual ~RaphnetTransport();

    RaphnetTransport(const RaphnetTransport&) = delete;
    RaphnetTransport& operator=(const RaphnetTransport&) = delete;

    // Opens hid_open_path(hidPath). On success, performs the boot sequence:
    //   1. SUSPEND_POLLING(0)  defensive — clears prior wedged state if a
    //      previous launch crashed mid-session and left polling suspended.
    //   2. GET_VERSION         logs adapter firmware version.
    //   3. SUSPEND_POLLING(1)  hands the channel to us; firmware will not
    //      auto-poll the controller during native operation.
    // Returns true on full success; false if any step fails (the device is
    // closed automatically on failure). Heavy logging at every step.
    virtual bool Open(const std::string& hidPath, uint16_t vid, uint16_t pid,
                      const std::wstring& serial);

    // Sends SUSPEND_POLLING(0) (so adapter is usable as plain HID joystick
    // again after game exit), then hid_close. Idempotent.
    virtual void Close();

    virtual bool IsOpen() const { return mDevice != nullptr; }

    // Pulls firmware version into outVersion (e.g. "3.6.1"). Format depends
    // on firmware; we log the raw response bytes alongside the formatted
    // string so testers' logs show exactly what came back.
    virtual bool GetVersion(std::string& outVersion);

    // Sends RQ_RNT_GET_CONTROLLER_TYPE [chn]. Returns true on response;
    // outType is one of gRntCtlType* values. v3.4+ firmware required —
    // older firmware returns the GET_VERSION echo instead, which we treat
    // as "unknown" (false return).
    virtual bool GetControllerType(uint8_t channel, uint8_t& outType);

    // Sends RQ_RNT_SUSPEND_POLLING [0|1]. When suspended, the firmware
    // stops auto-polling the controller so RAW_SI commands have exclusive
    // access to the SI bus.
    virtual bool SuspendPolling(bool suspend);

    // Sends RQ_RNT_SET_VIBRATION [chn][on?1:0]. Firmware writes the SI
    // accessory write to the rumble pak; no-op (silently) if the
    // controller doesn't have a rumble pak inserted.
    virtual bool SetVibration(uint8_t channel, bool on);

    // Sends RAW_SI(N64_GET_STATUS) on channel and decodes the 4-byte
    // response into pad.button (16-bit, MSB=byte0, matches LUS BTN_*
    // bit layout exactly), pad.stick_x (s8), pad.stick_y (s8). Other
    // OSContPad fields (gyro, right stick) are zeroed; err_no is left
    // for the caller to set.
    //
    // On success: returns true. On any failure: returns false, pad
    // unchanged, throttled WARN logged. First failure per channel is
    // ERROR with full context; first success per channel is INFO with
    // full bytes.
    virtual bool Poll(uint8_t channel, OSContPad& pad);

    // Identifying info, populated by Open(). Non-virtual — subclasses set
    // the protected state directly in their own Open override.
    const std::string&  GetHidPath()       const { return mHidPath; }
    uint16_t            GetVid()           const { return mVid; }
    uint16_t            GetPid()           const { return mPid; }
    const std::wstring& GetSerial()        const { return mSerial; }
    const std::string&  GetVersionString() const { return mVersionString; }

  protected:
    // ToUtf8 / FormatHidError exposed to subclasses so they can format log
    // lines consistently with the real transport's style.
    static std::string FormatHidError(hid_device* dev);
    static std::string ToUtf8(const std::wstring& w);

    hid_device*  mDevice;
    std::mutex   mLock;
    std::string  mHidPath;
    std::wstring mSerial;
    uint16_t     mVid;
    uint16_t     mPid;
    int          mReportSize;
    std::string  mVersionString;

    // Per-channel logging state; bounded by gRntMaxChannelsPerAdapter.
    bool     mFirstPollLogged   [gRntMaxChannelsPerAdapter];
    bool     mFirstErrorLogged  [gRntMaxChannelsPerAdapter];
    uint32_t mErrorCount        [gRntMaxChannelsPerAdapter];

  private:
    // Send command; receive response. Returns the response payload length
    // (bytes after the report-id byte) on success, -1 on USB error, 0 on
    // timeout. Caller holds mLock.
    int Exchange(const uint8_t* tx, size_t txLen,
                 uint8_t* rx, size_t rxMax,
                 int timeoutMs);
};

} // namespace Ship
