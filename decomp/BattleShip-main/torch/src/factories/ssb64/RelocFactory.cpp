#include "RelocFactory.h"

#include "Companion.h"
#include "spdlog/spdlog.h"

#include <iomanip>
#include <stdexcept>

extern "C" {
#include <libvpk0/vpk0.h>
}

// ============================================================================
//  Torch → .o2r binary export
// ============================================================================
//
//  Binary format written into the .o2r archive (after the 0x40-byte LUS header):
//
//    u32  file_id
//    u16  reloc_intern_offset      (word offset, 0xFFFF = none)
//    u16  reloc_extern_offset      (word offset, 0xFFFF = none)
//    u32  num_extern_file_ids
//    u16[num_extern_file_ids]  extern_file_ids
//    u32  decompressed_data_size   (bytes)
//    u8[decompressed_data_size]  decompressed_data
//
// ============================================================================

// ----------------------------------------------------------------------------
//  Header exporter (generates C header declarations / OTR path strings)
// ----------------------------------------------------------------------------

ExportResult SSB64::RelocHeaderExporter::Export(std::ostream& write,
                                                 std::shared_ptr<IParsedData> raw,
                                                 std::string& entryName,
                                                 YAML::Node& node,
                                                 std::string* replacement) {
    const auto symbol = GetSafeNode(node, "symbol", entryName);

    if (Companion::Instance->IsOTRMode()) {
        write << "static const ALIGN_ASSET(2) char " << symbol
              << "[] = \"__OTR__" << (*replacement) << "\";\n\n";
        return std::nullopt;
    }

    write << "extern u8 " << symbol << "[];\n";
    return std::nullopt;
}

// ----------------------------------------------------------------------------
//  Code exporter (generates C arrays — for decomp builds, not the port)
// ----------------------------------------------------------------------------

ExportResult SSB64::RelocCodeExporter::Export(std::ostream& write,
                                               std::shared_ptr<IParsedData> raw,
                                               std::string& entryName,
                                               YAML::Node& node,
                                               std::string* replacement) {
    auto symbol = GetSafeNode(node, "symbol", entryName);
    auto offset = GetSafeNode<uint32_t>(node, "offset");
    auto reloc = std::static_pointer_cast<SSB64::RelocData>(raw);

    if (Companion::Instance->IsOTRMode()) {
        write << "static const ALIGN_ASSET(2) char " << symbol
              << "[] = \"__OTR__" << (*replacement) << "\";\n\n";
        return std::nullopt;
    }

    // Emit as a raw u8 array of the decompressed data
    write << "u8 " << symbol << "[] = {\n" << tab_t;
    for (size_t i = 0; i < reloc->mDecompressedData.size(); i++) {
        if ((i % 15 == 0) && i != 0) {
            write << "\n" << tab_t;
        }
        write << "0x" << std::hex << std::setw(2) << std::setfill('0')
              << (int)reloc->mDecompressedData[i] << ", ";
    }
    write << "\n};\n";

    return offset + reloc->mDecompressedData.size();
}

// ----------------------------------------------------------------------------
//  Binary exporter (writes into .o2r archive)
// ----------------------------------------------------------------------------

ExportResult SSB64::RelocBinaryExporter::Export(std::ostream& write,
                                                 std::shared_ptr<IParsedData> raw,
                                                 std::string& entryName,
                                                 YAML::Node& node,
                                                 std::string* replacement) {
    auto reloc = std::static_pointer_cast<SSB64::RelocData>(raw);
    auto writer = LUS::BinaryWriter();

    WriteHeader(writer, Torch::ResourceType::SSB64Reloc, 0);

    writer.Write(reloc->mFileId);
    writer.Write(reloc->mRelocInternOffset);
    writer.Write(reloc->mRelocExternOffset);

    writer.Write((uint32_t)reloc->mExternFileIds.size());
    for (uint16_t id : reloc->mExternFileIds) {
        writer.Write(id);
    }

    writer.Write((uint32_t)reloc->mDecompressedData.size());
    writer.Write((char*)reloc->mDecompressedData.data(),
                 reloc->mDecompressedData.size());

    writer.Finish(write);
    return std::nullopt;
}

// ----------------------------------------------------------------------------
//  Factory — parse reloc file from ROM buffer
// ----------------------------------------------------------------------------

std::optional<std::shared_ptr<IParsedData>>
SSB64::RelocFactory::parse(std::vector<uint8_t>& buffer, YAML::Node& node) {
    auto fileId = GetSafeNode<uint32_t>(node, "file_id");

    // Reloc table location/size depends on the loaded ROM's region
    // (the factory reads the table straight from ROM).
    const SSB64::RelocLayout L = SSB64::GetRelocLayout(buffer);

    if (fileId >= L.fileCount) {
        throw std::runtime_error(
            "SSB64:RELOC file_id " + std::to_string(fileId) + " out of range");
    }

    // --- Read this file's table entry and the next (for computing extern region) ---
    size_t tableOffset = L.tableRomAddr + fileId * L.entrySize;
    if (tableOffset + L.entrySize * 2 > buffer.size()) {
        throw std::runtime_error("ROM too small to read table entry for file " +
                                  std::to_string(fileId));
    }

    LUS::BinaryReader tableReader(
        reinterpret_cast<char*>(buffer.data() + tableOffset),
        L.entrySize * 2);
    tableReader.SetEndianness(Torch::Endianness::Big);

    // Current entry
    uint32_t firstWord = tableReader.ReadUInt32();
    bool isCompressed = (firstWord >> 31) != 0;
    uint32_t dataOffset = firstWord & 0x7FFFFFFF;
    uint16_t relocIntern = tableReader.ReadUInt16();
    uint16_t compressedSizeWords = tableReader.ReadUInt16();
    uint16_t relocExtern = tableReader.ReadUInt16();
    uint16_t decompressedSizeWords = tableReader.ReadUInt16();

    // Next entry (sentinel exists for the last file)
    uint32_t nextFirstWord = tableReader.ReadUInt32();
    uint32_t nextDataOffset = nextFirstWord & 0x7FFFFFFF;

    uint32_t compressedSizeBytes = (uint32_t)compressedSizeWords * 4;
    uint32_t decompressedSizeBytes = (uint32_t)decompressedSizeWords * 4;

    // --- Locate data in ROM ---
    size_t dataRomAddr = L.dataStart + dataOffset;

    if (dataRomAddr + compressedSizeBytes > buffer.size()) {
        throw std::runtime_error(
            "ROM too small to read file data for file " + std::to_string(fileId));
    }

    const uint8_t* fileData = buffer.data() + dataRomAddr;

    // --- Decompress if VPK0-compressed ---
    std::vector<uint8_t> decompressed;

    if (isCompressed) {
        decompressed.resize(decompressedSizeBytes);

        uint32_t result = vpk0_decode(fileData, compressedSizeBytes,
                                       decompressed.data(), decompressedSizeBytes);
        if (result == 0) {
            throw std::runtime_error(
                "VPK0 decompression failed for file " + std::to_string(fileId) +
                " (compressed=" + std::to_string(compressedSizeBytes) +
                " decompressed=" + std::to_string(decompressedSizeBytes) + ")");
        }

        spdlog::debug("SSB64:RELOC file {} decompressed {} -> {} bytes",
                       fileId, compressedSizeBytes, decompressedSizeBytes);
    } else {
        decompressed.assign(fileData, fileData + decompressedSizeBytes);
    }

    // --- Extract external file ID list ---
    // External file IDs are stored as big-endian u16 values in ROM
    // immediately after the compressed data.
    size_t externRegionStart = dataRomAddr + compressedSizeBytes;
    size_t externRegionEnd = L.dataStart + nextDataOffset;

    std::vector<uint16_t> externFileIds;

    if (externRegionEnd > externRegionStart && externRegionEnd <= buffer.size()) {
        size_t externBytes = externRegionEnd - externRegionStart;
        size_t numExternIds = externBytes / 2;

        for (size_t i = 0; i < numExternIds; i++) {
            size_t addr = externRegionStart + i * 2;
            uint16_t extId = ((uint16_t)buffer[addr] << 8) | buffer[addr + 1];
            externFileIds.push_back(extId);
        }
    }

    return std::make_shared<SSB64::RelocData>(
        fileId, relocIntern, relocExtern,
        std::move(externFileIds), std::move(decompressed));
}
