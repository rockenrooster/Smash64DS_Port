#include "RelocFile.h"

void* RelocFile::GetPointer() {
    return Data.data();
}

size_t RelocFile::GetPointerSize() {
    return Data.size();
}
