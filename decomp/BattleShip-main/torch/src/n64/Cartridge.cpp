#include "Cartridge.h"

#include "lib/binarytools/BinaryReader.h"
#include <Companion.h>

bool N64::Cartridge::Normalize(std::vector<uint8_t>& romData) {
    if (romData.size() < 4) {
        return false;
    }
    const uint8_t b0 = romData[0];
    const uint8_t b1 = romData[1];
    const uint8_t b2 = romData[2];
    const uint8_t b3 = romData[3];

    // .z64 — big-endian, already canonical
    if (b0 == 0x80 && b1 == 0x37 && b2 == 0x12 && b3 == 0x40) {
        return true;
    }
    // .v64 — byteswapped (each u16 has its two bytes swapped)
    if (b0 == 0x37 && b1 == 0x80 && b2 == 0x40 && b3 == 0x12) {
        const size_t n = romData.size() & ~size_t{1};
        for (size_t i = 0; i < n; i += 2) {
            std::swap(romData[i], romData[i + 1]);
        }
        return true;
    }
    // .n64 — little-endian (each u32 has its four bytes reversed)
    if (b0 == 0x40 && b1 == 0x12 && b2 == 0x37 && b3 == 0x80) {
        const size_t n = romData.size() & ~size_t{3};
        for (size_t i = 0; i < n; i += 4) {
            std::swap(romData[i + 0], romData[i + 3]);
            std::swap(romData[i + 1], romData[i + 2]);
        }
        return true;
    }
    return false;
}

void N64::Cartridge::Initialize() {
    LUS::BinaryReader reader((char*)this->gRomData.data(), this->gRomData.size());
    reader.SetEndianness(Torch::Endianness::Big);
    reader.Seek(0x10, LUS::SeekOffsetType::Start);
    this->gRomCRC = BSWAP32(reader.ReadUInt32());
    reader.Seek(0x20, LUS::SeekOffsetType::Start);
    this->gGameTitle = std::string(reader.ReadCString());
    this->gGameTitle.pop_back(); // Remove null terminator
    reader.Seek(0x3E, LUS::SeekOffsetType::Start);
    uint8_t country = reader.ReadUByte();
    this->gVersion = reader.ReadUByte();
    this->gHash = Companion::CalculateHash(this->gRomData);
    switch (country) {
        case 'J':
            this->gCountryCode = CountryCode::Japan;
            break;
        case 'E':
            this->gCountryCode = CountryCode::NorthAmerica;
            break;
        case 'P':
            this->gCountryCode = CountryCode::Europe;
            break;
        default:
            this->gCountryCode = CountryCode::Unknown;
            break;
    }
    reader.Close();
}

const std::string& N64::Cartridge::GetGameTitle() {
    return this->gGameTitle;
}

N64::CountryCode N64::Cartridge::GetCountry() {
    return this->gCountryCode;
}

uint8_t N64::Cartridge::GetVersion() const {
    return this->gVersion;
}

std::string N64::Cartridge::GetHash() {
    return this->gHash;
}

std::string N64::Cartridge::GetCountryCode() {
    switch (this->gCountryCode) {
        case CountryCode::Japan:
            return "jp";
        case CountryCode::NorthAmerica:
            return "us";
        case CountryCode::Europe:
            return "eu";
        default:
            return "unk";
    }
}

uint32_t N64::Cartridge::GetCRC() {
    return this->gRomCRC;
}
