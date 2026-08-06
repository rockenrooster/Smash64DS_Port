#pragma once

#include <factories/BaseFactory.h>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace SSB64 {

static constexpr uint32_t RELOC_TABLE_ENTRY_SIZE = 12;

// N64 ROM header country-code byte (z64 / big-endian — Torch normalises
// byte order before factories run). 'E' = NALE (US v1.0), 'J' = NALJ
// (Nintendo All-Star! Dairantou Smash Brothers, Japan).
static constexpr size_t ROM_COUNTRY_CODE_OFF = 0x3E;

// Per-ROM-version reloc table layout. Table ROM addresses come from the
// upstream decomp splat configs (smashbrothers.<v>.yaml:
// `- [<addr>, bin, relocData]`); file counts from
// relocFileDescriptions.<v>.txt. The SSB64:RELOC factory reads the table
// straight from the ROM, so it must use the right base for the loaded ROM.
struct RelocLayout {
    uint32_t tableRomAddr;
    uint32_t fileCount;
    uint32_t entrySize;   // == RELOC_TABLE_ENTRY_SIZE
    uint32_t tableSize;   // (fileCount + 1) * entrySize  (incl. sentinel)
    uint32_t dataStart;   // tableRomAddr + tableSize
};

inline RelocLayout GetRelocLayout(const std::vector<uint8_t>& rom) {
    uint8_t country = rom.size() > ROM_COUNTRY_CODE_OFF
                          ? rom[ROM_COUNTRY_CODE_OFF] : (uint8_t)'E';
    uint32_t addr, count;
    switch (country) {
        case 'J': addr = 0x001ACAF0u; count = 2107u; break; // NALJ
        case 'E':
        default:  addr = 0x001AC870u; count = 2132u; break; // NALE (US v1.0)
    }
    RelocLayout L{};
    L.tableRomAddr = addr;
    L.fileCount    = count;
    L.entrySize    = RELOC_TABLE_ENTRY_SIZE;
    L.tableSize    = (count + 1u) * RELOC_TABLE_ENTRY_SIZE;
    L.dataStart    = addr + L.tableSize;
    return L;
}

/**
 * Parsed reloc file data — holds decompressed file contents and relocation metadata.
 */
class RelocData : public IParsedData {
public:
    uint32_t mFileId;
    uint16_t mRelocInternOffset;    // internal reloc chain start (in u32 words), 0xFFFF = none
    uint16_t mRelocExternOffset;    // external reloc chain start (in u32 words), 0xFFFF = none
    std::vector<uint16_t> mExternFileIds;  // IDs of files referenced by external relocations
    std::vector<uint8_t> mDecompressedData;

    RelocData(uint32_t fileId, uint16_t relocIntern, uint16_t relocExtern,
              std::vector<uint16_t> externIds, std::vector<uint8_t> data)
        : mFileId(fileId)
        , mRelocInternOffset(relocIntern)
        , mRelocExternOffset(relocExtern)
        , mExternFileIds(std::move(externIds))
        , mDecompressedData(std::move(data))
    {}
};

class RelocHeaderExporter : public BaseExporter {
    ExportResult Export(std::ostream& write, std::shared_ptr<IParsedData> data,
                        std::string& entryName, YAML::Node& node,
                        std::string* replacement) override;
};

class RelocBinaryExporter : public BaseExporter {
    ExportResult Export(std::ostream& write, std::shared_ptr<IParsedData> data,
                        std::string& entryName, YAML::Node& node,
                        std::string* replacement) override;
};

class RelocCodeExporter : public BaseExporter {
    ExportResult Export(std::ostream& write, std::shared_ptr<IParsedData> data,
                        std::string& entryName, YAML::Node& node,
                        std::string* replacement) override;
};

class RelocFactory : public BaseFactory {
public:
    std::optional<std::shared_ptr<IParsedData>> parse(std::vector<uint8_t>& buffer,
                                                       YAML::Node& data) override;
    inline std::unordered_map<ExportType, std::shared_ptr<BaseExporter>> GetExporters() override {
        return {
            REGISTER(Header, RelocHeaderExporter)
            REGISTER(Binary, RelocBinaryExporter)
            REGISTER(Code, RelocCodeExporter)
        };
    }
};

} // namespace SSB64
