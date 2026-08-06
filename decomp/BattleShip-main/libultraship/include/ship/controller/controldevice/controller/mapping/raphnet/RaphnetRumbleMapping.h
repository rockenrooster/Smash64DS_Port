#pragma once

// RaphnetRumbleMapping
// --------------------
// Rumble mapping for an N64 controller plugged into a raphnet adapter that
// the RaphnetPhysicalDeviceManager has claimed for this port. Drives the
// Rumble Pak via RQ_RNT_SET_VIBRATION (firmware does the SI accessory write
// internally; no Controller Pak interaction needed from us).
//
// The mapping holds a weak_ptr<RaphnetTransport> + channel so a hot-unplug
// (transport destroyed by the manager) decays cleanly to a silent no-op
// rather than crashing. The destructor does NOT touch the transport, so the
// destructor-singleton-reentry trap documented in
// project_libultraship_destructor_singleton_reentry does not apply.
// Cleanup of in-flight rumble is handled twice over: by ControlDeck::Stop-
// AllRumble (which calls StopRumble before sContext.reset) and by
// RaphnetPhysicalDeviceManager::Shutdown (which sets vibration off on every
// claimed channel before closing the transport).
//
// Raphnet's SET_VIBRATION is binary (on/off), not analog. The base class
// stores low/high intensity percentages for config compatibility but we
// ignore them — any non-zero start treats as on, stop as off. Config
// persistence is intentionally a no-op: the manager re-installs the mapping
// each launch from live hardware state, so writing to config would just
// produce stale entries.

#include <cstdint>
#include <memory>
#include <string>

#include "ship/controller/controldevice/controller/mapping/ControllerRumbleMapping.h"

namespace Ship {

class RaphnetTransport;  // ship/controller/raphnet/RaphnetTransport.h

class RaphnetRumbleMapping final : public ControllerRumbleMapping {
  public:
    RaphnetRumbleMapping(uint8_t portIndex, uint8_t lowFrequencyIntensityPercentage,
                         uint8_t highFrequencyIntensityPercentage,
                         std::weak_ptr<RaphnetTransport> transport, uint8_t channel);
    ~RaphnetRumbleMapping() = default;

    void StartRumble() override;
    void StopRumble() override;

    std::string GetRumbleMappingId() override;
    void        SaveToConfig() override;     // intentional no-op
    void        EraseFromConfig() override;  // intentional no-op

    std::string GetPhysicalDeviceName() override;

  private:
    std::weak_ptr<RaphnetTransport> mTransport;
    uint8_t                         mChannel;
};

} // namespace Ship
