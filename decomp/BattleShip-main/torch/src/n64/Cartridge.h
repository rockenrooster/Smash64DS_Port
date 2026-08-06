#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace N64 {
enum class CountryCode {
    Japan,
    NorthAmerica,
    Europe,
    Unknown
};

class Cartridge {
public:
    explicit Cartridge(const std::vector<uint8_t>& romData)
      : gRomData(romData), gCountryCode(CountryCode::Unknown), gVersion(0), gGameTitle("Unknown"), gRomCRC(0) {
  }
  void Initialize();
    // Detects the ROM byte order from the first 4 bytes and converts it
    // in-place to big-endian z64 layout. Accepts:
    //   .z64 (big-endian, 80 37 12 40) — passthrough
    //   .v64 (byteswapped 16-bit,  37 80 40 12) — swap each u16
    //   .n64 (little-endian 32-bit, 40 12 37 80) — swap each u32
    // Returns false if the magic doesn't match any known order (caller
    // should still try Initialize() and let the hash check reject it).
    static bool Normalize(std::vector<uint8_t>& romData);
    const std::string& GetGameTitle();
    std::string GetCountryCode();
    CountryCode GetCountry();
    uint8_t GetVersion() const;
    std::string GetHash();
    uint32_t GetCRC();
private:
    std::vector<uint8_t> gRomData;
    CountryCode gCountryCode;
    uint8_t gVersion;
    std::string gGameTitle;
    std::string gHash;
    uint32_t gRomCRC;
};
}