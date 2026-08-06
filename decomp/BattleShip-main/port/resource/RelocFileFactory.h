#pragma once

#include <ship/resource/ResourceFactoryBinary.h>

/**
 * LUS resource factory for SSB64 relocatable files.
 *
 * Reads the SSB64Reloc resource type (0x52454C4F / "RELO") from .o2r archives.
 * The binary format matches what the Torch SSB64:RELOC exporter writes:
 *
 *   u32  file_id
 *   u16  reloc_intern_offset
 *   u16  reloc_extern_offset
 *   u32  num_extern_file_ids
 *   u16[num_extern_file_ids]  extern_file_ids
 *   u32  decompressed_data_size
 *   u8[decompressed_data_size]  decompressed_data
 */
class ResourceFactoryBinaryRelocFileV0 final : public Ship::ResourceFactoryBinary {
public:
    std::shared_ptr<Ship::IResource> ReadResource(
        std::shared_ptr<Ship::File> file,
        std::shared_ptr<Ship::ResourceInitData> initData) override;
};
