// Test-only stubs for port-side `portReloc*` symbols that
// libultraship/src/fast/interpreter.cpp references via `extern "C"`
// declarations. The real implementations live downstream in
// `port/bridge/lbreloc_bridge.cpp` and are linked when libultraship.a
// is consumed by the Battleship binary; the standalone gtest target
// here doesn't pull in the port tree, so without these stubs the
// `lus_tests` link step fails.
//
// Tests in this directory exercise libultraship's internal
// post-process modules — none of them invoke an interpreter codepath
// that would dereference these stubs at runtime — so returning safe
// defaults is sufficient.

#include <cstddef>
#include <cstdint>

extern "C" void* portRelocTryResolvePointer(uint32_t /*token*/) {
    return nullptr;
}

extern "C" bool portRelocFindContainingFile(const void* /*ptr*/,
                                            uintptr_t* /*out_base*/,
                                            std::size_t* /*out_size*/) {
    return false;
}

extern "C" bool portRelocDescribePointer(const void* /*ptr*/,
                                         uintptr_t* /*out_base*/,
                                         std::size_t* /*out_size*/,
                                         uint32_t* /*out_file_id*/,
                                         const char** /*out_path*/) {
    return false;
}

extern "C" void portRelocFixupVertexAtRuntime(const void* /*addr*/,
                                              unsigned int /*num_vtx*/) {
}

extern "C" void portRelocFixupTextureAtRuntime(const void* /*addr*/,
                                               unsigned int /*num_bytes*/) {
}
