#include "ship/resource/Resource.h"
#include <spdlog/spdlog.h>

namespace Ship {
IResource::IResource(std::shared_ptr<ResourceInitData> initData) : mInitData(initData) {
}

IResource::~IResource() {
    // Intentionally silent.  Logging from here is unsafe: resources can
    // outlive Ship::Context (held by external shared_ptrs) and run during
    // __cxa_finalize_ranges, after spdlog::shutdown() has nulled the
    // default logger — the SPDLOG_* call then crashes in should_log().
}

bool IResource::IsDirty() {
    return mIsDirty;
}

void IResource::Dirty() {
    mIsDirty = true;
}

std::shared_ptr<ResourceInitData> IResource::GetInitData() {
    return mInitData;
}
} // namespace Ship
