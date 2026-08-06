#pragma once

#include <cstdint>

namespace SSB64 {

enum class ResourceType : uint32_t {
    SSB64Reloc = 0x52454C4F,  // "RELO" — matches Torch::ResourceType::SSB64Reloc
};

} // namespace SSB64
