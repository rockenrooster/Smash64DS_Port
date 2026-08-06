#include "ship/controller/controldevice/controller/mapping/raphnet/RaphnetRumbleMapping.h"

#include <spdlog/spdlog.h>

#include "ship/controller/raphnet/RaphnetTransport.h"
#include "ship/utils/StringHelper.h"

namespace Ship {

RaphnetRumbleMapping::RaphnetRumbleMapping(uint8_t portIndex, uint8_t lowFrequencyIntensityPercentage,
                                           uint8_t highFrequencyIntensityPercentage,
                                           std::weak_ptr<RaphnetTransport> transport, uint8_t channel)
    : ControllerRumbleMapping(PhysicalDeviceType::Raphnet, portIndex, lowFrequencyIntensityPercentage,
                              highFrequencyIntensityPercentage),
      mTransport(std::move(transport)),
      mChannel(channel) {
    SPDLOG_INFO("[raphnet] rumble mapping installed on port {} chn={}", portIndex, channel);
}

void RaphnetRumbleMapping::StartRumble() {
    auto t = mTransport.lock();
    if (t == nullptr) {
        SPDLOG_DEBUG("[raphnet] StartRumble port={} chn={}: transport gone (hot-unplug?), no-op",
                     mPortIndex, mChannel);
        return;
    }
    if (!t->SetVibration(mChannel, true)) {
        SPDLOG_DEBUG("[raphnet] StartRumble port={} chn={}: SET_VIBRATION returned false",
                     mPortIndex, mChannel);
    }
}

void RaphnetRumbleMapping::StopRumble() {
    auto t = mTransport.lock();
    if (t == nullptr) {
        SPDLOG_DEBUG("[raphnet] StopRumble port={} chn={}: transport gone, no-op", mPortIndex,
                     mChannel);
        return;
    }
    if (!t->SetVibration(mChannel, false)) {
        SPDLOG_DEBUG("[raphnet] StopRumble port={} chn={}: SET_VIBRATION returned false",
                     mPortIndex, mChannel);
    }
}

std::string RaphnetRumbleMapping::GetRumbleMappingId() {
    return StringHelper::Sprintf("RaphnetP%d", mPortIndex);
}

void RaphnetRumbleMapping::SaveToConfig() {
    // No-op: the RaphnetPhysicalDeviceManager re-installs this mapping at
    // every launch from live hardware state, so persisting it would only
    // produce stale entries that the factory would reject.
}

void RaphnetRumbleMapping::EraseFromConfig() {
    // No-op: nothing was written.
}

std::string RaphnetRumbleMapping::GetPhysicalDeviceName() {
    return "Raphnet (native)";
}

} // namespace Ship
