#include "RelocFileFactory.h"
#include "RelocFile.h"

std::shared_ptr<Ship::IResource>
ResourceFactoryBinaryRelocFileV0::ReadResource(std::shared_ptr<Ship::File> file,
                                                std::shared_ptr<Ship::ResourceInitData> initData) {
    if (!FileHasValidFormatAndReader(file, initData)) {
        return nullptr;
    }

    auto relocFile = std::make_shared<RelocFile>(initData);
    auto reader = std::get<std::shared_ptr<Ship::BinaryReader>>(file->Reader);

    relocFile->FileId = reader->ReadUInt32();
    relocFile->RelocInternOffset = reader->ReadUInt16();
    relocFile->RelocExternOffset = reader->ReadUInt16();

    uint32_t numExternIds = reader->ReadUInt32();
    relocFile->ExternFileIds.reserve(numExternIds);
    for (uint32_t i = 0; i < numExternIds; i++) {
        relocFile->ExternFileIds.push_back(reader->ReadUInt16());
    }

    uint32_t dataSize = reader->ReadUInt32();
    relocFile->Data.resize(dataSize);
    reader->Read(reinterpret_cast<char*>(relocFile->Data.data()), static_cast<int32_t>(dataSize));

    return relocFile;
}
