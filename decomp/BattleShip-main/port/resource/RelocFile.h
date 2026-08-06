#pragma once

#include <ship/resource/Resource.h>
#include <vector>
#include <cstdint>

/**
 * SSB64 relocatable file resource.
 *
 * Loaded from .o2r archives at runtime. Contains pre-decompressed file data
 * plus relocation metadata needed for pointer patching.
 *
 * The resource data is in its original big-endian N64 format.
 * Pointer relocation happens at load time in the lbReloc bridge layer,
 * not in the factory.
 */
class RelocFile final : public Ship::Resource<void> {
public:
    using Resource::Resource;

    void* GetPointer() override;
    size_t GetPointerSize() override;

    uint32_t FileId;
    uint16_t RelocInternOffset;     // internal reloc chain start (in u32 words), 0xFFFF = none
    uint16_t RelocExternOffset;     // external reloc chain start (in u32 words), 0xFFFF = none
    std::vector<uint16_t> ExternFileIds;   // file IDs referenced by external relocations
    std::vector<uint8_t> Data;             // decompressed file data (big-endian)
};
