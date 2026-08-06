#define NOMINMAX

#ifdef _WIN32
#include <windows.h>
#endif

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>

#include <any>
#include <atomic>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>
#include <list>
#include <stack>
#include "fast/resource/type/Light.h"

#ifndef _LANGUAGE_C
#define _LANGUAGE_C
#endif
#include "fast/debug/GfxDebugger.h"
#include "fast/types.h"
#include <string>

#include "fast/interpreter.h"
#include "fast/lus_gbi.h"
#include "fast/backends/gfx_window_manager_api.h"
#include "fast/backends/gfx_rendering_api.h"

/* DIAG-ONLY: pre-deref ASan check in gfx_step(). When the interpreter is
   about to read a Gfx command from poisoned memory (typically a stale
   segment binding resolving to a freed/past-end heap address), dump the
   full segment table + recent segment writes + recent G_DL calls so the
   PR #133 lead-gap can be identified before ASan halts.

   Nested #if rather than a single `defined(__has_feature) && __has_feature(...)`
   because MSVC's preprocessor doesn't reliably short-circuit on unknown
   identifiers — it tries to parse __has_feature(address_sanitizer) even
   when defined(__has_feature) is false, producing C1012. GCC defines
   __SANITIZE_ADDRESS__ when -fsanitize=address; clang has __has_feature;
   MSVC defines __SANITIZE_ADDRESS__ for /fsanitize=address. */
#if defined(__SANITIZE_ADDRESS__)
#define PORT_DIAG_HAVE_ASAN 1
#elif defined(__has_feature)
#  if __has_feature(address_sanitizer)
#    define PORT_DIAG_HAVE_ASAN 1
#  endif
#endif
#ifdef PORT_DIAG_HAVE_ASAN
#include <sanitizer/asan_interface.h>
#endif

extern "C" void* portRelocTryResolvePointer(uint32_t token);
extern "C" bool portRelocFindContainingFile(const void* ptr, uintptr_t* out_base, size_t* out_size);
extern "C" bool portRelocDescribePointer(const void* ptr, uintptr_t* out_base, size_t* out_size,
                                         uint32_t* out_file_id, const char** out_path);
extern "C" void portRelocFixupVertexAtRuntime(const void *addr, unsigned int num_vtx);
extern "C" void portRelocFixupTextureAtRuntime(const void *addr, unsigned int num_bytes);

/* Game-specific DL safety hooks. The embedding game (e.g. SSB64) registers
 * a bounds-check and an address classifier via the public API in
 * fast/interpreter.h. libultraship calls these from gfx_step and the diag
 * dump but otherwise has no knowledge of game-specific memory layouts.
 * Default behavior with no callbacks registered is identical to the
 * un-hooked walker. */
static Fast::DLBoundsCheckFn sDLBoundsCheck = nullptr;
static Fast::AddressClassifierFn sAddressClassifier = nullptr;

/* Return-code constants matching public API documentation. */
static constexpr int kDLBoundsUnknown    = 0;
static constexpr int kDLBoundsInRange    = 1;
static constexpr int kDLBoundsWalkedPast = 2;

#include "ship/window/gui/Gui.h"
#include "ship/resource/ResourceManager.h"
#include "ship/utils/Utils.h"
#include "ship/Context.h"
#include "fast/postprocess/PostProcessSourceLoader.h"
#include "ship/config/ConsoleVariable.h"

#include "libultraship/libultra/os.h"

#include <spdlog/fmt/fmt.h>

std::stack<std::string> currentDir;

#define SEG_ADDR(seg, addr) (addr | (seg << 24) | 1)
#define SUPPORT_CHECK(x) assert(x)

// SCALE_M_N: upscale/downscale M-bit integer to N-bit
#define SCALE_5_8(VAL_) (((VAL_)*0xFF) / 0x1F)
#define SCALE_8_5(VAL_) ((((VAL_) + 4) * 0x1F) / 0xFF)
#define SCALE_4_8(VAL_) ((VAL_)*0x11)
#define SCALE_8_4(VAL_) ((VAL_) / 0x11)
#define SCALE_3_8(VAL_) ((VAL_)*0x24)
#define SCALE_8_3(VAL_) ((VAL_) / 0x24)

// Based off the current set native dimensions or active framebuffer
#define HALF_SCREEN_WIDTH(activeFb) ((mFbActive ? activeFb->second.orig_width : mNativeDimensions.width) / 2)
#define HALF_SCREEN_HEIGHT(activeFb) ((mFbActive ? activeFb->second.orig_height : mNativeDimensions.height) / 2)

// Ratios for current window dimensions or active framebuffer scaled size
#define RATIO_X(activeFb, dims) \
    ((mFbActive ? activeFb->second.applied_width : dims.width) / (2.0f * HALF_SCREEN_WIDTH(activeFb)))
#define RATIO_Y(activeFb, dims) \
    ((mFbActive ? activeFb->second.applied_height : dims.height) / (2.0f * HALF_SCREEN_HEIGHT(activeFb)))

#define TEXTURE_CACHE_MAX_SIZE 1024

namespace {

constexpr size_t PORT_PACKED_GFX_SIZE = sizeof(uint32_t) * 2;

struct PortPackedDisplayListInfo {
    const void* source;
    uintptr_t fileBase;
    size_t fileSize;
    std::shared_ptr<std::vector<Fast::F3DGfx>> commands;
};

std::unordered_map<const void*, PortPackedDisplayListInfo> sPortPackedDisplayListCache;

bool portFindNormalizedDisplayListCommand(const Fast::F3DGfx* cmd, const PortPackedDisplayListInfo** outInfo,
                                          size_t* outIndex) {
    for (const auto& [source, info] : sPortPackedDisplayListCache) {
        const Fast::F3DGfx* begin = info.commands->data();
        const Fast::F3DGfx* end = begin + info.commands->size();

        if ((cmd >= begin) && (cmd < end)) {
            if (outInfo != nullptr) {
                *outInfo = &info;
            }
            if (outIndex != nullptr) {
                *outIndex = static_cast<size_t>(cmd - begin);
            }
            return true;
        }
    }

    return false;
}

Fast::F3DGfx* portNormalizeDisplayListPointer(Fast::F3DGfx* dlist) {
    uintptr_t fileBase = 0;
    size_t fileSize = 0;

    if (dlist == nullptr || !portRelocFindContainingFile(dlist, &fileBase, &fileSize)) {
        return dlist;
    }

    // Guard: runtime display lists (built on the graphics heap with native 16-byte
    // Gfx structs) can fall within a reloc file's address range because both share
    // the same game heap.  Detect native format by checking the upper 32 bits of
    // uintptr_t w0 fields at THREE command positions.
    //
    // Layout comparison (as uint32_t[]):
    //   Native 16-byte:  [w0_lo] [w0_hi=0] [w1_lo] [w1_hi] [w0_lo] [w0_hi=0] ...
    //   Packed  8-byte:  [w0]    [w1]      [w0]    [w1]    [w0]    [w1]      ...
    //
    // For native format, probe[1], probe[5], and probe[9] are upper halves of
    // uintptr_t w0 fields — always zero because GBI opcodes fit in 32 bits.
    // For packed format, these are w1 of packed commands 0, 2, and 4.  Having
    // ALL THREE be zero requires 5+ commands where every other one has w1=0
    // (e.g. alternating syncs), which is virtually impossible in real DLs.
    {
        uintptr_t rawAddr_chk = reinterpret_cast<uintptr_t>(dlist);
        uintptr_t fileEnd_chk = fileBase + fileSize;
        const uint32_t* probe = reinterpret_cast<const uint32_t*>(dlist);

        // DISABLED: All data-inspection heuristics have false positives.
        // Packed DLs often start with commands that have w1=0 (sync, fog, geometry
        // mode).  Instead of guessing, we ALWAYS normalize data found in a reloc
        // file range.  If a native runtime DL somehow lands in a reloc range, the
        // normalization loop's opcode validator will bail out early and produce a
        // harmless truncated DL with G_ENDDL.
        (void)probe;
        (void)rawAddr_chk;
        (void)fileEnd_chk;
    }

    auto cached = sPortPackedDisplayListCache.find(dlist);

    if (cached != sPortPackedDisplayListCache.end()) {
        return cached->second.commands->data();
    }

    uintptr_t rawAddr = reinterpret_cast<uintptr_t>(dlist);
    uintptr_t fileEnd = fileBase + fileSize;

    if ((rawAddr < fileBase) || ((fileEnd - rawAddr) < PORT_PACKED_GFX_SIZE)) {
        return dlist;
    }

    auto translated = std::make_shared<std::vector<Fast::F3DGfx>>();
    translated->reserve((fileEnd - rawAddr) / PORT_PACKED_GFX_SIZE);

    while ((rawAddr + PORT_PACKED_GFX_SIZE) <= fileEnd) {
        const uint32_t* rawWords = reinterpret_cast<const uint32_t*>(rawAddr);
        uint8_t opcode = (uint8_t)(rawWords[0] >> 24);

        // Valid F3DEX2 opcodes: 0x00-0x0F (SP geometry) and 0xD7-0xFF (SP/RDP).
        // Anything in 0x10-0xD6 is not a real GBI command — we've entered
        // non-DL data (textures, vertices, structs) adjacent in the blob.
        if (opcode > 0x0F && opcode < 0xD7) {
            break;
        }

        Fast::F3DGfx hostCmd = {};

        hostCmd.words.w0 = rawWords[0];

        // Rewrite segment E (intra-file) references to absolute file addresses.
        // Resource DLs use 0x0E + offset for textures, vertices, matrices, and
        // sub-display-lists within the same reloc file blob.
        //
        // EXCEPTION: G_DL.  Some scenes (e.g. mvOpeningRoom) build per-MObj
        // material setup sub-DLs in the graphics heap at runtime and patch
        // segment 0x0E to point at that heap via gsSPSegment(0xE, ...).  Stored
        // model DLs then call into them with `gsSPDisplayList(0x0E000000+X)`.
        // For these, we MUST defer resolution to interpreter execution time so
        // the runtime segment table is consulted.  Other scenes (e.g. the N64
        // logo) use seg=0x0E G_DL as intra-file branches; gfx_dl_handler_common
        // falls back to fileBase + offset when the runtime segment is unset.
        uint32_t w1_raw = rawWords[1];
        uint8_t seg = (w1_raw >> 24) & 0xFF;
        uint32_t offset = w1_raw & 0x00FFFFFF;

        if (seg == 0x0E && offset < fileSize &&
            opcode != (uint8_t)Fast::F3DEX2_G_DL) {
            hostCmd.words.w1 = fileBase + offset;
        } else {
            hostCmd.words.w1 = w1_raw;
        }

        translated->push_back(hostCmd);

        rawAddr += PORT_PACKED_GFX_SIZE;

        if (opcode == (uint8_t)Fast::F3DEX2_G_ENDDL) {
            break;
        }
    }

    // Ensure the widened DL always ends with G_ENDDL.  The opcode validation
    // can terminate the loop early, and some source DLs use gSPBranchList
    // (never returns) instead of gSPEndDisplayList.  Without a terminator
    // the interpreter reads past the vector into uninitialized heap.
    {
        bool hasEndDL = !translated->empty() &&
            ((uint8_t)(translated->back().words.w0 >> 24) == (uint8_t)Fast::F3DEX2_G_ENDDL);
        if (!hasEndDL) {
            Fast::F3DGfx endCmd = {};
            endCmd.words.w0 = ((uintptr_t)(uint8_t)Fast::F3DEX2_G_ENDDL) << 24;
            translated->push_back(endCmd);
        }
    }

    Fast::F3DGfx* translatedPtr = translated->data();
    sPortPackedDisplayListCache.emplace(dlist, PortPackedDisplayListInfo{ dlist, fileBase, fileSize, translated });

    return translatedPtr;
}

size_t portGetDisplayListStride(uintptr_t segmentBase) {
    uintptr_t fileBase = 0;
    size_t fileSize = 0;

    if ((segmentBase != 0) && portRelocFindContainingFile(reinterpret_cast<const void*>(segmentBase), &fileBase, &fileSize)) {
        return PORT_PACKED_GFX_SIZE;
    }

    return sizeof(Fast::F3DGfx);
}

bool gfxPointerHasReadableBytes(const void* ptr, size_t size) {
    if (ptr == nullptr || size == 0) {
        return false;
    }

#ifdef _WIN32
    const uint8_t* cursor = reinterpret_cast<const uint8_t*>(ptr);
    size_t remaining = size;

    while (remaining != 0) {
        MEMORY_BASIC_INFORMATION mbi = {};

        if (VirtualQuery(cursor, &mbi, sizeof(mbi)) != sizeof(mbi)) {
            return false;
        }
        if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) != 0) {
            return false;
        }

        DWORD protect = mbi.Protect & 0xFF;

        switch (protect) {
            case PAGE_READONLY:
            case PAGE_READWRITE:
            case PAGE_WRITECOPY:
            case PAGE_EXECUTE_READ:
            case PAGE_EXECUTE_READWRITE:
            case PAGE_EXECUTE_WRITECOPY:
                break;

            default:
                return false;
        }

        uintptr_t regionEnd = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        size_t available = regionEnd - reinterpret_cast<uintptr_t>(cursor);

        if (available >= remaining) {
            return true;
        }

        remaining -= available;
        cursor += available;
    }
#endif

    return true;
}

} // namespace

extern "C" void portResetPackedDisplayListCache(void) {
    sPortPackedDisplayListCache.clear();
}

// Evict cached widened display lists whose source heap address falls in a
// range that's about to be overwritten by a fresh reloc-file load.  Same
// pattern as Interpreter::TextureCacheDeleteRange / portTextureCacheDeleteRange,
// applied to the packed-DL widening cache.  Without this, bump-reset heaps
// (stage-select wallpaper rewinds, classic-mode round transitions reusing the
// extern heap, CSS hover reloads) hit a stale cached widening that points at
// the prior file's bytes via cached fileBase/fileSize — manifests as G_VTX
// unresolved-segment skips and Unhandled OP-code bursts (issue #103/#128).
extern "C" void portPackedDisplayListCacheDeleteRange(const void* base, size_t size) {
    if (base == nullptr || size == 0) {
        return;
    }
    const uintptr_t lo = reinterpret_cast<uintptr_t>(base);
    const uintptr_t hi = lo + size;
    for (auto it = sPortPackedDisplayListCache.begin(); it != sPortPackedDisplayListCache.end(); ) {
        const uintptr_t srcAddr = reinterpret_cast<uintptr_t>(it->first);
        bool evict = srcAddr >= lo && srcAddr < hi;
        if (!evict) {
            for (const Fast::F3DGfx& cmd : *it->second.commands) {
                const uintptr_t w1 = cmd.words.w1;
                if (w1 >= lo && w1 < hi) {
                    evict = true;
                    break;
                }
            }
        }
        if (evict) {
            it = sPortPackedDisplayListCache.erase(it);
        } else {
            ++it;
        }
    }
}

namespace Fast {

static UcodeHandlers ucode_handler_index = ucode_f3dex2;

/* GBI trace callback — forward declaration (defined near g_exec_stack below) */
static GbiTraceCallbackFn sGbiTraceCallback = nullptr;

/* Hi-res texture pack hook — set by gfx_register_hires_hook(). NULL until
 * the host registers; see ImportTexture for the call site and the docblock
 * on GfxHiResHookFn (interpreter.h) for semantics. */
GfxHiResHookFn gHiResHook = nullptr;

const static uint32_t f3dex2AttrHandler[] = {
    F3DEX2_G_MTX_PROJECTION, F3DEX2_G_MTX_LOAD,  F3DEX2_G_MTX_PUSH,  F3DEX_G_MTX_NOPUSH,
    F3DEX2_G_CULL_FRONT,     F3DEX2_G_CULL_BACK, F3DEX2_G_CULL_BOTH,
};

const static uint32_t f3dexAttrHandler[] = { F3DEX_G_MTX_PROJECTION, F3DEX_G_MTX_LOAD,   F3DEX_G_MTX_PUSH,
                                             F3DEX_G_MTX_NOPUSH,     F3DEX_G_CULL_FRONT, F3DEX_G_CULL_BACK,
                                             F3DEX_G_CULL_BOTH };

static constexpr std::array ucode_attr_handlers = {
    &f3dexAttrHandler,  // ucode_f3db
    &f3dexAttrHandler,  // ucode_f3d
    &f3dexAttrHandler,  // ucode_f3dex
    &f3dexAttrHandler,  // ucode_f3exb
    &f3dex2AttrHandler, // ucode_f3ex2
    &f3dex2AttrHandler, // ucode_s2dex
};

static uint32_t get_attr(Attribute attr) {
    const auto ucode_map = ucode_attr_handlers[ucode_handler_index];
    // assert(ucode_map->contains(attr) && "Attribute not found in the current ucode handler");
    return (*ucode_map)[attr];
}

static std::string GetPathWithoutFileName(char* filePath) {
    size_t len = strlen(filePath);

    for (size_t i = len - 1; (long)i >= 0; i--) {
        if (filePath[i] == '/' || filePath[i] == '\\') {
            return std::string(filePath).substr(0, i);
        }
    }

    return filePath;
}

constexpr size_t MAX_TRI_BUFFER = 256;

Interpreter::Interpreter() {
    mRsp = new RSP();
    mRdp = new RDP();
    // Point palette pointers at staging buffers so CI8 rendering always has
    // valid pointers even when only one palette half has been loaded via TLUT.
    mRdp->palettes[0] = mRdp->palette_staging[0];
    mRdp->palettes[1] = mRdp->palette_staging[1];
    mBufVbo = new float[MAX_TRI_BUFFER * (32 * 3)];
}

Interpreter::~Interpreter() {
    delete mRsp;
    delete mRdp;
    delete[] mBufVbo;
}

static std::weak_ptr<Interpreter> mInstance;

// Set a cached pointer to the instance so we don't need to go through the window every time
void GfxSetInstance(std::shared_ptr<Interpreter> gfx) {
    mInstance = gfx;
}

/* ─── DIAG: stale-DL-pointer tracing (PR #133 lead-gap + variant 5) ──────
   Ring buffers of recent segment writes and recent DL pushes. Dumped on
   any SIGSEGV (via port_dump_dl_diag, called from CrashHandler::ErrorHandler)
   so we can identify which holder pushed the stale pointer that crashed
   gfx_step. Also dumps from gfx_step itself under ASan when poisoned memory
   is detected. Memory cost: ~3 KB BSS for the ring buffers, 2 atomic
   increments per push/segwrite. Always-on so the diag survives release
   builds where the heap-layout-dependent stale-pointer crashes actually
   manifest (Linux/glibc, NVIDIA OpenGL). */
namespace {
struct DiagSegWrite {
    int segNum;
    uintptr_t oldVal;
    uintptr_t newVal;
    const char* where;  // string literal — never freed
    uint32_t ucode;
    uint64_t frame;
};
struct DiagDLPush {
    void* caller_addr;          /* F3DGfx* of the calling instruction — tells us
                                 * which DL contains the bad bytes (heap vs reloc
                                 * file vs stale memory). Critical for tracing
                                 * stale-DL crashes back to the writing code path. */
    uintptr_t caller_w0;
    uintptr_t caller_w1;
    void* callee;
    void* callee_normalized;
    const char* where;
    uint32_t ucode;
    uint64_t frame;
};
constexpr size_t DIAG_RING = 32;
DiagSegWrite sDiagSegWrites[DIAG_RING] = {};
size_t sDiagSegWritesIdx = 0;
DiagDLPush sDiagDLPushes[DIAG_RING] = {};
size_t sDiagDLPushesIdx = 0;
uint64_t sDiagFrame = 0;
}  // namespace

extern "C" void portDiagBumpFrame() {
    ++sDiagFrame;
}

static inline void diagRecordSegWrite(int segNum, uintptr_t oldVal, uintptr_t newVal,
                                      const char* where, uint32_t ucode) {
    sDiagSegWrites[sDiagSegWritesIdx] = { segNum, oldVal, newVal, where, ucode, sDiagFrame };
    sDiagSegWritesIdx = (sDiagSegWritesIdx + 1) % DIAG_RING;
}

static inline void diagRecordDLPush(F3DGfx* caller, F3DGfx* callee, F3DGfx* normalized,
                                    const char* where) {
    sDiagDLPushes[sDiagDLPushesIdx] = {
        (void*)caller,
        caller ? (uintptr_t)caller->words.w0 : 0,
        caller ? (uintptr_t)caller->words.w1 : 0,
        (void*)callee, (void*)normalized, where, 0, sDiagFrame
    };
    sDiagDLPushesIdx = (sDiagDLPushesIdx + 1) % DIAG_RING;
}

/* Defined in port/bridge/lbreloc_bridge.cpp. Classifies an address into
 * scene_arena+offset / reloc[id]+offset / n64_seg / low_brk / high_heap.
 * Weak — diag still runs even in builds that don't link the port-side
 * classifier (e.g. unit tests with a stub lbreloc_bridge). */
static void diagClassify(uintptr_t addr, char* buf, size_t buf_size) {
    if (sAddressClassifier && sAddressClassifier(addr, buf, buf_size)) {
        return;
    }
    std::snprintf(buf, buf_size, "0x%lx", (unsigned long)addr);
}

static void diagDumpAll(F3DGfx* badCmd, const char* reason) {
    char clsbuf[160];
    SPDLOG_CRITICAL("==== GFX STALE-DL DIAG ({}) ====", reason);
    diagClassify((uintptr_t)badCmd, clsbuf, sizeof(clsbuf));
    SPDLOG_CRITICAL("badCmd host={} class={} frame={}", (void*)badCmd, clsbuf, sDiagFrame);
    if (auto inst = mInstance.lock()) {
        SPDLOG_CRITICAL("-- segment table --");
        for (int i = 0; i < MAX_SEGMENT_POINTERS; ++i) {
            if (inst->mSegmentPointers[i] != 0) {
                diagClassify(inst->mSegmentPointers[i], clsbuf, sizeof(clsbuf));
                SPDLOG_CRITICAL("  seg[{:#x}] = {:#x} class={}", i, inst->mSegmentPointers[i], clsbuf);
            }
        }
    }
    SPDLOG_CRITICAL("-- recent segment writes (oldest first, {} slots) --", DIAG_RING);
    for (size_t i = 0; i < DIAG_RING; ++i) {
        size_t k = (sDiagSegWritesIdx + i) % DIAG_RING;
        const auto& e = sDiagSegWrites[k];
        if (!e.where) continue;
        diagClassify(e.newVal, clsbuf, sizeof(clsbuf));
        SPDLOG_CRITICAL("  [frame={}] seg[{:#x}] {:#x} -> {:#x} class={} ({})",
                        e.frame, e.segNum, e.oldVal, e.newVal, clsbuf, e.where);
    }
    SPDLOG_CRITICAL("-- recent DL pushes (oldest first) --");
    for (size_t i = 0; i < DIAG_RING; ++i) {
        size_t k = (sDiagDLPushesIdx + i) % DIAG_RING;
        const auto& e = sDiagDLPushes[k];
        if (!e.where) continue;
        char callerCls[160];
        diagClassify((uintptr_t)e.caller_addr, callerCls, sizeof(callerCls));
        diagClassify((uintptr_t)e.callee_normalized, clsbuf, sizeof(clsbuf));
        SPDLOG_CRITICAL("  [frame={}] caller_addr={} caller_class={} w0={:#x} w1={:#x} callee={} norm={} callee_class={} ({})",
                        e.frame, e.caller_addr, callerCls,
                        e.caller_w0, e.caller_w1, e.callee, e.callee_normalized, clsbuf, e.where);
    }
    /* cmd_stack: chain of currently-executing DLs (top is innermost). The top
     * entry is the DL whose bytes the walker was reading when the crash hit —
     * if it's in stale memory (post-terminator heap leftovers, recycled
     * arena), classifier will say so.
     *
     * gfx_path: parallel chain of caller F3DGfx* — each entry is the *address*
     * of the G_DL command that pushed onto cmd_stack. Combined, these give
     * the full back-trace through nested DL calls. */
    if (auto inst = mInstance.lock()) {
        SPDLOG_CRITICAL("-- exec cmd_stack (top is current DL, oldest at bottom) --");
        std::stack<F3DGfx*> tmp = g_exec_stack.cmd_stack;
        size_t depth = 0;
        while (!tmp.empty()) {
            F3DGfx* p = tmp.top();
            tmp.pop();
            diagClassify((uintptr_t)p, clsbuf, sizeof(clsbuf));
            SPDLOG_CRITICAL("  [depth={}] cmd={} class={}", depth, (void*)p, clsbuf);
            ++depth;
            if (depth > 64) {
                SPDLOG_CRITICAL("  ... (truncated)");
                break;
            }
        }
        SPDLOG_CRITICAL("-- gfx_path (caller F3DGfx* per nesting level, oldest first) --");
        for (size_t i = 0; i < g_exec_stack.gfx_path.size() && i < 64; ++i) {
            const F3DGfx* p = g_exec_stack.gfx_path[i];
            diagClassify((uintptr_t)p, clsbuf, sizeof(clsbuf));
            uintptr_t w0 = 0, w1 = 0;
            if (p != nullptr) {
                w0 = (uintptr_t)p->words.w0;
                w1 = (uintptr_t)p->words.w1;
            }
            SPDLOG_CRITICAL("  [lvl={}] caller={} class={} w0={:#x} w1={:#x}",
                            i, (const void*)p, clsbuf, w0, w1);
        }
    }
    SPDLOG_CRITICAL("==== END DIAG ====");
}

/* Public API — see fast/interpreter.h. Called from CrashHandler.cpp and
 * port_watchdog.cpp on SIGSEGV. badCmd is best-effort (typically
 * siginfo->si_addr); the ring buffers tell the rest of the story. Safe
 * to call from a signal handler: only touches static buffers + spdlog. */
void DumpDLDiag(void* badCmd, const char* reason) {
    diagDumpAll((F3DGfx*)badCmd, reason ? reason : "SIGSEGV");
}

void RegisterDLBoundsCheck(DLBoundsCheckFn fn) {
    sDLBoundsCheck = fn;
}

void RegisterAddressClassifier(AddressClassifierFn fn) {
    sAddressClassifier = fn;
}

void Interpreter::Flush() {
    if (mBufVboLen > 0) {
        mRapi->DrawTriangles(mBufVbo, mBufVboLen, mBufVboNumTris);
        mBufVboLen = 0;
        mBufVboNumTris = 0;
    }
}

static std::set<std::pair<uint64_t,uint64_t>> sFailedShaderIds;

ShaderProgram* Interpreter::LookupOrCreateShaderProgram(uint64_t id0, uint64_t id1) {
    auto key = std::make_pair(id0, id1);
    if (sFailedShaderIds.count(key)) {
        return nullptr; // Previously failed — don't retry
    }
    ShaderProgram* prg = mRapi->LookupShader(id0, id1);
    if (prg == nullptr) {
        mRapi->UnloadShader(mRenderingState.mShaderProgram);
        prg = mRapi->CreateAndLoadNewShader(id0, id1);
        mRenderingState.mShaderProgram = prg;
        if (prg == nullptr) {
            sFailedShaderIds.insert(key);
        }
    }
    return prg;
}

const char* Interpreter::CCMUXtoStr(uint32_t ccmux) {
    static constexpr std::array tbl = {
        "G_CCMUX_COMBINED",
        "G_CCMUX_TEXEL0",
        "G_CCMUX_TEXEL1",
        "G_CCMUX_PRIMITIVE",
        "G_CCMUX_SHADE",
        "G_CCMUX_ENVIRONMENT",
        "G_CCMUX_1",
        "G_CCMUX_COMBINED_ALPHA",
        "G_CCMUX_TEXEL0_ALPHA",
        "G_CCMUX_TEXEL1_ALPHA",
        "G_CCMUX_PRIMITIVE_ALPHA",
        "G_CCMUX_SHADE_ALPHA",
        "G_CCMUX_ENV_ALPHA",
        "G_CCMUX_LOD_FRACTION",
        "G_CCMUX_PRIM_LOD_FRAC",
        "G_CCMUX_K5",
    };
    if (ccmux > tbl.size()) {
        return "G_CCMUX_0";
    }
    return tbl[ccmux];
}

// Seems unused
const char* Interpreter::ACMUXtoStr(uint32_t acmux) {
    static constexpr std::array tbl = {
        "G_ACMUX_COMBINED or G_ACMUX_LOD_FRACTION",
        "G_ACMUX_TEXEL0",
        "G_ACMUX_TEXEL1",
        "G_ACMUX_PRIMITIVE",
        "G_ACMUX_SHADE",
        "G_ACMUX_ENVIRONMENT",
        "G_ACMUX_1 or G_ACMUX_PRIM_LOD_FRAC",
        "G_ACMUX_0",
    };
    return tbl[acmux];
}

void Interpreter::GenerateCC(ColorCombiner* comb, const ColorCombinerKey& key) {
    const bool is2Cyc = (key.options & SHADER_OPT(_2CYC)) != 0;

    uint8_t c[2][2][4];
    uint64_t shaderId0 = 0;
    uint64_t shaderId1 = key.options;
    uint8_t shaderInputMapping[2][7] = { { 0 } };
    bool usedTextures[2]{};
    for (uint32_t i = 0; i < 2 && (i == 0 || is2Cyc); i++) {
        uint32_t rgbA = (key.combine_mode >> (i * 28)) & 0xf;
        uint32_t rgbB = (key.combine_mode >> (i * 28 + 4)) & 0xf;
        uint32_t rgbC = (key.combine_mode >> (i * 28 + 8)) & 0x1f;
        uint32_t rgbD = (key.combine_mode >> (i * 28 + 13)) & 7;
        uint32_t alphaA = (key.combine_mode >> (i * 28 + 16)) & 7;
        uint32_t alphaB = (key.combine_mode >> (i * 28 + 16 + 3)) & 7;
        uint32_t alphaC = (key.combine_mode >> (i * 28 + 16 + 6)) & 7;
        uint32_t alphaD = (key.combine_mode >> (i * 28 + 16 + 9)) & 7;

        if (rgbA >= 8) {
            rgbA = G_CCMUX_0;
        }
        if (rgbB >= 8) {
            rgbB = G_CCMUX_0;
        }
        if (rgbC >= 16) {
            rgbC = G_CCMUX_0;
        }
        if (rgbD == 7) {
            rgbD = G_CCMUX_0;
        }

        if (rgbA == rgbB || rgbC == G_CCMUX_0) {
            // Normalize
            rgbA = G_CCMUX_0;
            rgbB = G_CCMUX_0;
            rgbC = G_CCMUX_0;
        }
        if (alphaA == alphaB || alphaC == G_ACMUX_0) {
            // Normalize
            alphaA = G_ACMUX_0;
            alphaB = G_ACMUX_0;
            alphaC = G_ACMUX_0;
        }
        if (i == 1) {
            if (rgbA != G_CCMUX_COMBINED && rgbB != G_CCMUX_COMBINED && rgbC != G_CCMUX_COMBINED &&
                rgbD != G_CCMUX_COMBINED) {
                // First cycle RGB not used, so clear it away
                c[0][0][0] = c[0][0][1] = c[0][0][2] = c[0][0][3] = G_CCMUX_0;
            }
            if (rgbC != G_CCMUX_COMBINED_ALPHA && alphaA != G_ACMUX_COMBINED && alphaB != G_ACMUX_COMBINED &&
                alphaD != G_ACMUX_COMBINED) {
                // First cycle ALPHA not used, so clear it away
                c[0][1][0] = c[0][1][1] = c[0][1][2] = c[0][1][3] = G_ACMUX_0;
            }
        }

        c[i][0][0] = rgbA;
        c[i][0][1] = rgbB;
        c[i][0][2] = rgbC;
        c[i][0][3] = rgbD;
        c[i][1][0] = alphaA;
        c[i][1][1] = alphaB;
        c[i][1][2] = alphaC;
        c[i][1][3] = alphaD;
    }
    if (!is2Cyc) {
        for (uint32_t i = 0; i < 2; i++) {
            for (uint32_t k = 0; k < 4; k++) {
                c[1][i][k] = i == 0 ? G_CCMUX_0 : G_ACMUX_0;
            }
        }
    }
    {
        uint8_t inputNumber[32] = { 0 };
        uint32_t nextInputNumber = SHADER_INPUT_1;
        for (uint32_t i = 0; i < 2 && (i == 0 || is2Cyc); i++) {
            for (uint32_t j = 0; j < 4; j++) {
                uint32_t val = 0;
                switch (c[i][0][j]) {
                    case G_CCMUX_0:
                        val = SHADER_0;
                        break;
                    case G_CCMUX_1:
                        val = SHADER_1;
                        break;
                    case G_CCMUX_TEXEL0:
                        val = SHADER_TEXEL0;
                        // Set the opposite texture when reading from the second cycle color options
                        if (i == 0) {
                            usedTextures[0] = true;
                        } else {
                            usedTextures[1] = true;
                        }
                        break;
                    case G_CCMUX_TEXEL1:
                        val = SHADER_TEXEL1;
                        if (i == 0) {
                            usedTextures[1] = true;
                        } else {
                            usedTextures[0] = true;
                        }
                        break;
                    case G_CCMUX_TEXEL0_ALPHA:
                        val = SHADER_TEXEL0A;
                        if (i == 0) {
                            usedTextures[0] = true;
                        } else {
                            usedTextures[1] = true;
                        }
                        break;
                    case G_CCMUX_TEXEL1_ALPHA:
                        val = SHADER_TEXEL1A;
                        if (i == 0) {
                            usedTextures[1] = true;
                        } else {
                            usedTextures[0] = true;
                        }
                        break;
                    case G_CCMUX_NOISE:
                        val = SHADER_NOISE;
                        break;
                    case G_CCMUX_PRIMITIVE:
                    case G_CCMUX_PRIMITIVE_ALPHA:
                    case G_CCMUX_PRIM_LOD_FRAC:
                    case G_CCMUX_SHADE:
                    case G_CCMUX_ENVIRONMENT:
                    case G_CCMUX_ENV_ALPHA:
                    case G_CCMUX_LOD_FRACTION:
                        if (inputNumber[c[i][0][j]] == 0) {
                            shaderInputMapping[0][nextInputNumber - 1] = c[i][0][j];
                            inputNumber[c[i][0][j]] = nextInputNumber++;
                        }
                        val = inputNumber[c[i][0][j]];
                        break;
                    case G_CCMUX_COMBINED:
                        val = SHADER_COMBINED;
                        break;
                    default:
                        fprintf(stderr, "Unsupported ccmux: %d\n", c[i][0][j]);
                        break;
                }
                shaderId0 |= (uint64_t)val << (i * 32 + j * 4);
            }
        }
    }
    {
        uint8_t inputNumber[16] = { 0 };
        uint32_t nextInputNumber = SHADER_INPUT_1;
        for (uint32_t i = 0; i < 2; i++) {
            for (uint32_t j = 0; j < 4; j++) {
                uint32_t val = 0;
                switch (c[i][1][j]) {
                    case G_ACMUX_0:
                        val = SHADER_0;
                        break;
                    case G_ACMUX_TEXEL0:
                        val = SHADER_TEXEL0;
                        // Set the opposite texture when reading from the second cycle color options
                        if (i == 0) {
                            usedTextures[0] = true;
                        } else {
                            usedTextures[1] = true;
                        }
                        break;
                    case G_ACMUX_TEXEL1:
                        val = SHADER_TEXEL1;
                        if (i == 0) {
                            usedTextures[1] = true;
                        } else {
                            usedTextures[0] = true;
                        }
                        break;
                    case G_ACMUX_LOD_FRACTION:
                        // case G_ACMUX_COMBINED: same numerical value
                        if (j != 2) {
                            val = SHADER_COMBINED;
                            break;
                        }
                        c[i][1][j] = G_CCMUX_LOD_FRACTION;
                        [[fallthrough]]; // for G_ACMUX_LOD_FRACTION
                    case G_ACMUX_1:
                        // case G_ACMUX_PRIM_LOD_FRAC: same numerical value
                        if (j != 2) {
                            val = SHADER_1;
                            break;
                        }
                        [[fallthrough]]; // for G_ACMUX_PRIM_LOD_FRAC
                    case G_ACMUX_PRIMITIVE:
                    case G_ACMUX_SHADE:
                    case G_ACMUX_ENVIRONMENT:
                        if (inputNumber[c[i][1][j]] == 0) {
                            shaderInputMapping[1][nextInputNumber - 1] = c[i][1][j];
                            inputNumber[c[i][1][j]] = nextInputNumber++;
                        }
                        val = inputNumber[c[i][1][j]];
                        break;
                }
                shaderId0 |= (uint64_t)val << (i * 32 + 16 + j * 4);
            }
        }
    }
    comb->shader_id0 = shaderId0;
    comb->shader_id1 = shaderId1;
    comb->usedTextures[0] = usedTextures[0];
    comb->usedTextures[1] = usedTextures[1];
    // comb->prg = gfx_lookup_or_create_mShaderProgram(shader_id0, shader_id1);
    memcpy(comb->shader_input_mapping, shaderInputMapping, sizeof(shaderInputMapping));
}

ColorCombiner* Interpreter::LookupOrCreateColorCombiner(const ColorCombinerKey& key) {
    if (mPrevCombiner != mColorCombinerPool.end() && mPrevCombiner->first == key) {
        return &mPrevCombiner->second;
    }
    mPrevCombiner = mColorCombinerPool.find(key);
    if (mPrevCombiner != mColorCombinerPool.end()) {
        return &mPrevCombiner->second;
    }
    Flush();
    mPrevCombiner = mColorCombinerPool.insert(std::make_pair(key, ColorCombiner())).first;
    GenerateCC(&mPrevCombiner->second, key);
    return &mPrevCombiner->second;
}

void Interpreter::TextureCacheClear() {
    for (const auto& entry : mTextureCache.map) {
        mTextureCache.free_texture_ids.push_back(entry.second.texture_id);
    }
    mTextureCache.map.clear();
    mTextureCache.lru.clear();
    // Pre-allocate buckets so the map never rehashes during normal operation.
    // Rehashing invalidates all iterators, including those stored in LRU entries.
    mTextureCache.map.reserve(TEXTURE_CACHE_MAX_SIZE);
    // Null rendering-state pointers — they pointed into map nodes that are now freed.
    std::fill(std::begin(mRenderingState.mTextures), std::end(mRenderingState.mTextures), nullptr);
}

bool Interpreter::TextureCacheLookup(int i, const TextureCacheKey& key) {
    TextureCacheMap::iterator it = mTextureCache.map.find(key);
    TextureCacheNode** n = &mRenderingState.mTextures[i];

    if (it != mTextureCache.map.end()) {
        mRapi->SelectTexture(i, it->second.texture_id);
        *n = &*it;
        mTextureCache.lru.splice(mTextureCache.lru.end(), mTextureCache.lru,
                                 it->second.lru_location); // move to back
        return true;
    }

    if (mTextureCache.map.size() >= TEXTURE_CACHE_MAX_SIZE) {
        // Remove the texture that was least recently used
        it = mTextureCache.lru.front().it;
        mTextureCache.free_texture_ids.push_back(it->second.texture_id);
        for (int j = 0; j < SHADER_MAX_TEXTURES; j++) {
            if (mRenderingState.mTextures[j] == &*it)
                mRenderingState.mTextures[j] = nullptr;
        }
        mTextureCache.map.erase(it);
        mTextureCache.lru.pop_front();
    }

    uint32_t texture_id;
    if (!mTextureCache.free_texture_ids.empty()) {
        texture_id = mTextureCache.free_texture_ids.back();
        mTextureCache.free_texture_ids.pop_back();
    } else {
        texture_id = mRapi->NewTexture();
    }

    it = mTextureCache.map.insert(std::make_pair(key, TextureCacheValue())).first;
    TextureCacheNode* node = &*it;
    node->second.texture_id = texture_id;
    node->second.lru_location = mTextureCache.lru.insert(mTextureCache.lru.end(), { it });

    mRapi->SelectTexture(i, texture_id);
    mRapi->SetSamplerParameters(i, false, 0, 0);
    *n = node;
    return false;
}

std::string_view Interpreter::GetBaseTexturePath(std::string_view path) {
    if (path.starts_with(Ship::IResource::gAltAssetPrefix)) {
        return path.substr(Ship::IResource::gAltAssetPrefix.length());
    }

    return path;
}

/* Guarded masked-texture lookup for the ImportTexture* replacement paths.
 * The registry can lose an entry (UnregisterBlendedTexture) while a loaded
 * tile still carries importReplacement state, and a registration name can
 * differ from the resource path — an unchecked find()->second dereference
 * is UB on miss. Returns nullptr on miss; every caller already null-checks
 * the resulting addr and falls back gracefully. */
static const uint8_t* GetMaskedReplacementData(const std::map<std::string, MaskedTextureEntry, std::less<>>& masked,
                                               std::string_view basePath) {
    auto it = masked.find(basePath);
    if (it == masked.end()) {
        SPDLOG_ERROR("Masked texture replacement missing for '{}'", std::string(basePath));
        return nullptr;
    }
    return it->second.replacementData;
}

void Interpreter::TextureCacheDeleteRange(const uint8_t* base, size_t size) {
    if (base == nullptr || size == 0) {
        return;
    }
    const uint8_t* end = base + size;
    for (auto it = mTextureCache.map.begin(); it != mTextureCache.map.end(); ) {
        if (it->first.texture_addr >= base && it->first.texture_addr < end) {
            for (int j = 0; j < SHADER_MAX_TEXTURES; j++) {
                if (mRenderingState.mTextures[j] == &*it) {
                    mRenderingState.mTextures[j] = nullptr;
                }
            }
            mTextureCache.lru.erase(it->second.lru_location);
            mTextureCache.free_texture_ids.push_back(it->second.texture_id);
            it = mTextureCache.map.erase(it);
        } else {
            ++it;
        }
    }
}

extern "C" void portTextureCacheDeleteRange(const void* base, size_t size) {
    auto inst = mInstance.lock();
    if (!inst) {
        return;
    }
    inst->TextureCacheDeleteRange(static_cast<const uint8_t*>(base), size);
}

void Interpreter::ResetRdpTextureState() {
    mRdp->texture_to_load.addr = nullptr;
    mRdp->texture_to_load.siz = 0;
    mRdp->texture_to_load.width = 0;
    mRdp->texture_to_load.tex_flags = 0;
    mRdp->texture_to_load.raw_tex_metadata = {};

    for (int i = 0; i < 2; i++) {
        mRdp->loaded_texture[i].addr = nullptr;
        mRdp->loaded_texture[i].orig_size_bytes = 0;
        mRdp->loaded_texture[i].size_bytes = 0;
        mRdp->loaded_texture[i].full_image_line_size_bytes = 0;
        mRdp->loaded_texture[i].line_size_bytes = 0;
        mRdp->loaded_texture[i].tex_flags = 0;
        mRdp->loaded_texture[i].raw_tex_metadata = {};
        mRdp->loaded_texture[i].masked = false;
        mRdp->loaded_texture[i].blended = false;
        mRdp->textures_changed[i] = true;
    }
}

void Interpreter::TextureCacheDelete(const uint8_t* origAddr) {
    while (mTextureCache.map.bucket_count() > 0) {
        TextureCacheKey key = { origAddr, { 0 }, 0, 0, 0, 0, 0, 0, 0 }; // bucket index only depends on the address
        size_t bucket = mTextureCache.map.bucket(key);
        bool again = false;
        for (auto it = mTextureCache.map.begin(bucket); it != mTextureCache.map.end(bucket); ++it) {
            if (it->first.texture_addr == origAddr) {
                for (int j = 0; j < SHADER_MAX_TEXTURES; j++) {
                    if (mRenderingState.mTextures[j] == &*it)
                        mRenderingState.mTextures[j] = nullptr;
                }
                mTextureCache.lru.erase(it->second.lru_location);
                mTextureCache.free_texture_ids.push_back(it->second.texture_id);
                mTextureCache.map.erase(it->first);
                again = true;
                break;
            }
        }
        if (!again) {
            break;
        }
    }
}

// Pick the per-line byte width for texture decode. Prefer the DRAM stride from
// loaded_texture when it looks like real per-line info (differs from total size).
// Fall back to the TMEM tile stride when loaded sizes match total (LoadBlock with
// width=1, where line_size == full_image_line_size == size).
static uint32_t GetEffectiveLineSize(uint32_t lineSizeBytes, uint32_t fullImageLineSizeBytes, uint32_t sizeBytes,
                                     uint32_t tileLineSizeBytes) {
    if ((lineSizeBytes != sizeBytes || fullImageLineSizeBytes != sizeBytes) && lineSizeBytes > 0) {
        return lineSizeBytes;
    }
    return tileLineSizeBytes;
}

// Clamp an upload-width to the tile's SetTileSize extent.  The game sets
// SetTileSize(0, 0, (width-1)<<2, (height-1)<<2) to mark the drawable
// tile area — on N64 hardware the sampler clamps s/t past these bounds
// and the out-of-bounds texels are never read.  On the port, though,
// ImportTexture* uploads the full TMEM line stride (rounded up from
// `width` to qword alignment, so `width_img` ≥ `width`) as the GPU
// texture width, while `GfxSpTri1` normalizes UV by the SetTileSize
// clamped width.  The mismatch stretches the texture: sampling at the
// rightmost drawn pixel lands on GPU col `width_img - 1` instead of
// `width - 1`, so any trailing bytes in [width, width_img) bleed into
// the render (the original MARIO title-card smear) and the bilinear
// filter at the right edge can't average with a real clamped-edge
// value (the later title-border dim slice).
//
// Fix: when SetTileSize declares a drawable extent smaller than the
// natural TMEM line size, also clamp the upload width to match so the
// GPU texture is exactly `width` pixels wide.  Then the UV
// normalization, the SpTri1 tex_width clamp, and OpenGL's
// CLAMP_TO_EDGE all agree on the same right-edge pixel.
//
// `naturalWidthPixels` is the per-row texel count before clamping.
// `tile_uls` / `tile_lrs` are in S10.2 (texel * 4) units.
static uint32_t ClampUploadWidthToTile(uint32_t naturalWidthPixels, uint32_t tile_uls, uint32_t tile_lrs) {
    if (tile_lrs > tile_uls) {
        uint32_t clampPixels = (tile_lrs - tile_uls + 4) / 4;
        if (clampPixels > 0 && clampPixels < naturalWidthPixels) {
            return clampPixels;
        }
    }
    return naturalWidthPixels;
}

static bool Ssb64RenderDiagEnabled() {
    static int enabled = -1;
    if (enabled < 0) {
        const char* value = getenv("SSB64_RENDER_DIAG");
        enabled = (value != nullptr && value[0] != '\0' && value[0] != '0') ? 1 : 0;
    }
    return enabled != 0;
}

static int Ssb64RenderDiagLimit() {
    static int limit = -1;
    if (limit < 0) {
        const char* value = getenv("SSB64_RENDER_DIAG_LIMIT");
        limit = value != nullptr && value[0] != '\0' ? atoi(value) : 400;
        if (limit <= 0) {
            limit = 400;
        }
    }
    return limit;
}

static bool Ssb64RenderDiagTakeSlot() {
    static int count = 0;
    if (!Ssb64RenderDiagEnabled()) {
        return false;
    }
    if (count >= Ssb64RenderDiagLimit()) {
        return false;
    }
    count++;
    return true;
}

static std::string Ssb64RenderDiagTexturePath(const RawTexMetadata* metadata, const void* addr) {
    if (metadata != nullptr && metadata->resource != nullptr) {
        return metadata->resource->GetInitData()->Path;
    }

    const char* relocPath = nullptr;
    if (addr != nullptr && portRelocDescribePointer(addr, nullptr, nullptr, nullptr, &relocPath) && relocPath != nullptr) {
        return relocPath;
    }

    return "(raw)";
}

static bool Ssb64RenderDiagDescribePointer(const void* addr, uintptr_t* base, size_t* size, uint32_t* fileId) {
    if (addr == nullptr) {
        return false;
    }
    return portRelocDescribePointer(addr, base, size, fileId, nullptr);
}

static bool Ssb64RenderDiagFileIdMatches(uint32_t fileId, const char* list) {
    if (list == nullptr || list[0] == '\0') {
        return false;
    }

    const char* cursor = list;
    while (*cursor != '\0') {
        char* end = nullptr;
        unsigned long parsed = strtoul(cursor, &end, 0);
        if (end != cursor && static_cast<uint32_t>(parsed) == fileId) {
            return true;
        }
        cursor = end != cursor ? end : cursor + 1;
        while (*cursor == ',' || *cursor == ' ' || *cursor == ';' || *cursor == ':') {
            cursor++;
        }
    }

    return false;
}

static bool Ssb64RenderDiagParseUlongEnv(const char* name, unsigned long* out) {
    const char* value = getenv(name);
    if (value == nullptr || value[0] == '\0') {
        return false;
    }

    char* end = nullptr;
    unsigned long parsed = strtoul(value, &end, 0);
    if (end == value) {
        return false;
    }

    *out = parsed;
    return true;
}

static bool Ssb64RenderDiagNamedFilterMatches(uint32_t fileId, const char* filter) {
    if (filter == nullptr) {
        return false;
    }

    if (strcmp(filter, "Star") == 0 || strcmp(filter, "star") == 0) {
        return fileId == 0xfb || fileId == 0xff || fileId == 0x58 || fileId == 0x2c;
    }
    if (strcmp(filter, "Samus") == 0 || strcmp(filter, "samus") == 0) {
        return fileId == 0xd9 || fileId == 0x140 || fileId == 0x135 || fileId == 0xda || fileId == 0x15d;
    }
    if (strcmp(filter, "DreamLand") == 0 || strcmp(filter, "dreamland") == 0 || strcmp(filter, "Pupupu") == 0 ||
        strcmp(filter, "pupupu") == 0) {
        return fileId == 0x58 || fileId == 0xff || fileId == 0x100 || fileId == 0x102;
    }

    return false;
}

static bool Ssb64RenderDiagMatches(const RawTexMetadata* metadata, const void* addr) {
    if (!Ssb64RenderDiagEnabled()) {
        return false;
    }

    static bool announced = false;
    if (!announced) {
        const char* filter = getenv("SSB64_RENDER_DIAG_FILTER");
        const char* fileIds = getenv("SSB64_RENDER_DIAG_FILE_ID");
        const char* minOff = getenv("SSB64_RENDER_DIAG_MIN_OFF");
        const char* maxOff = getenv("SSB64_RENDER_DIAG_MAX_OFF");
        SPDLOG_INFO("SSB64_RENDER_DIAG active filter='{}' file_ids='{}' min_off='{}' max_off='{}' limit={}",
                    filter != nullptr ? filter : "", fileIds != nullptr ? fileIds : "", minOff != nullptr ? minOff : "",
                    maxOff != nullptr ? maxOff : "", Ssb64RenderDiagLimit());
        announced = true;
    }

    uintptr_t base = 0;
    size_t size = 0;
    uint32_t fileId = 0;
    bool hasReloc = Ssb64RenderDiagDescribePointer(addr, &base, &size, &fileId);
    unsigned long minOff = 0;
    unsigned long maxOff = 0;
    bool hasMinOff = Ssb64RenderDiagParseUlongEnv("SSB64_RENDER_DIAG_MIN_OFF", &minOff);
    bool hasMaxOff = Ssb64RenderDiagParseUlongEnv("SSB64_RENDER_DIAG_MAX_OFF", &maxOff);
    if (hasMinOff || hasMaxOff) {
        if (!hasReloc) {
            return false;
        }

        uintptr_t offset = reinterpret_cast<uintptr_t>(addr) - base;
        if (hasMinOff && offset < minOff) {
            return false;
        }
        if (hasMaxOff && offset > maxOff) {
            return false;
        }
    }

    const char* fileIdFilter = getenv("SSB64_RENDER_DIAG_FILE_ID");
    if (hasReloc && Ssb64RenderDiagFileIdMatches(fileId, fileIdFilter)) {
        return true;
    }

    const char* filter = getenv("SSB64_RENDER_DIAG_FILTER");
    if ((filter == nullptr || filter[0] == '\0') && (fileIdFilter == nullptr || fileIdFilter[0] == '\0')) {
        return true;
    }

    std::string path = Ssb64RenderDiagTexturePath(metadata, addr);
    if (filter != nullptr && filter[0] != '\0' && path.find(filter) != std::string::npos) {
        return true;
    }
    if (hasReloc && Ssb64RenderDiagNamedFilterMatches(fileId, filter)) {
        return true;
    }

    return false;
}

static std::atomic<int> sArmedImportCaptureN{0};

extern "C" void portDiagArmImportCapture(int n) {
    sArmedImportCaptureN.store(n > 0 ? n : 0, std::memory_order_relaxed);
}

static bool Ssb64RenderDiagArmedTake() {
    int prev = sArmedImportCaptureN.load(std::memory_order_relaxed);
    while (prev > 0) {
        if (sArmedImportCaptureN.compare_exchange_weak(prev, prev - 1, std::memory_order_relaxed)) {
            return true;
        }
    }
    return false;
}

static void Ssb64RenderDiagLogImport(const char* stage, const RawTexMetadata* metadata, const void* addr, int slot,
                                     int tile, uint32_t tmemIndex, uint8_t fmt, uint8_t siz, uint8_t cms,
                                     uint8_t cmt, uint8_t masks, uint8_t maskt, uint8_t shifts, uint8_t shiftt,
                                     uint32_t lineBytes, uint32_t loadedLineBytes, uint32_t loadedFullLineBytes,
                                     uint32_t sizeBytes, uint32_t origSizeBytes, uint32_t uls, uint32_t ult,
                                     uint32_t lrs, uint32_t lrt, bool replacement) {
    bool armed = Ssb64RenderDiagArmedTake();
    if (!armed && (!Ssb64RenderDiagMatches(metadata, addr) || !Ssb64RenderDiagTakeSlot())) {
        return;
    }

    uint32_t tileW = lrs > uls ? (lrs - uls + 4) / 4 : 0;
    uint32_t tileH = lrt > ult ? (lrt - ult + 4) / 4 : 0;
    uintptr_t base = 0;
    size_t relocSize = 0;
    uint32_t fileId = 0;
    bool hasReloc = Ssb64RenderDiagDescribePointer(addr, &base, &relocSize, &fileId);
    SPDLOG_INFO(
        "SSB64_RENDER_DIAG {} path='{}' addr={} file={} off=0x{:x} slot={} tile={} tmem={} fmt={} siz={} cms={} "
        "cmt={} masks={} maskt={} shifts={} shiftt={} line={} loadedLine={} fullLine={} size={} origSize={} "
        "tileST=({},{})->({},{}), tileWH={}x{} replacement={}",
        stage, Ssb64RenderDiagTexturePath(metadata, addr), addr, hasReloc ? fileId : 0,
        hasReloc ? static_cast<uintptr_t>(reinterpret_cast<uintptr_t>(addr) - base) : 0, slot, tile, tmemIndex, fmt,
        siz, cms, cmt, masks, maskt, shifts, shiftt, lineBytes, loadedLineBytes, loadedFullLineBytes, sizeBytes,
        origSizeBytes, uls, ult, lrs, lrt, tileW, tileH, replacement);
}

static void Ssb64RenderDiagLogUpload(const char* decoder, const RawTexMetadata* metadata, const void* addr, int tile,
                                     uint32_t tmemIndex, uint32_t width, uint32_t height) {
    if (!Ssb64RenderDiagMatches(metadata, addr) || !Ssb64RenderDiagTakeSlot()) {
        return;
    }

    uintptr_t base = 0;
    size_t size = 0;
    uint32_t fileId = 0;
    bool hasReloc = Ssb64RenderDiagDescribePointer(addr, &base, &size, &fileId);
    SPDLOG_INFO("SSB64_RENDER_DIAG upload decoder={} path='{}' addr={} file={} off=0x{:x} tile={} tmem={} uploadWH={}x{}",
                decoder, Ssb64RenderDiagTexturePath(metadata, addr), addr, hasReloc ? fileId : 0,
                hasReloc ? static_cast<uintptr_t>(reinterpret_cast<uintptr_t>(addr) - base) : 0, tile, tmemIndex, width,
                height);
}

static void Ssb64RenderDiagLogDraw(const RawTexMetadata* metadata, const void* addr, int slot, int tile,
                                   uint32_t tmemIndex, uint8_t fmt, uint8_t siz, uint8_t cms, uint8_t cmt,
                                   uint8_t masks, uint8_t maskt, uint8_t shifts, uint8_t shiftt,
                                   uint32_t texWidth, uint32_t texHeight, uint32_t tileWidth, uint32_t tileHeight,
                                   float rawUMin, float rawUMax, float rawVMin, float rawVMax, float normUMin,
                                   float normUMax, float normVMin, float normVMax, float x0, float y0, float z0,
                                   float w0, float x1, float y1, float z1, float w1, float x2, float y2, float z2,
                                   float w2) {
    if (!Ssb64RenderDiagMatches(metadata, addr) || !Ssb64RenderDiagTakeSlot()) {
        return;
    }

    uintptr_t base = 0;
    size_t relocSize = 0;
    uint32_t fileId = 0;
    bool hasReloc = Ssb64RenderDiagDescribePointer(addr, &base, &relocSize, &fileId);
    SPDLOG_INFO(
        "SSB64_RENDER_DIAG draw path='{}' addr={} file={} off=0x{:x} slot={} tile={} tmem={} fmt={} siz={} cms={} cmt={} masks={} maskt={} "
        "shifts={} shiftt={} texWH={}x{} tileWH={}x{} rawUV=({:.3f}..{:.3f},{:.3f}..{:.3f}) "
        "normUV=({:.5f}..{:.5f},{:.5f}..{:.5f}) clip=({:.3f},{:.3f},{:.3f},{:.3f}) "
        "({:.3f},{:.3f},{:.3f},{:.3f}) ({:.3f},{:.3f},{:.3f},{:.3f})",
        Ssb64RenderDiagTexturePath(metadata, addr), addr, hasReloc ? fileId : 0,
        hasReloc ? static_cast<uintptr_t>(reinterpret_cast<uintptr_t>(addr) - base) : 0, slot, tile, tmemIndex, fmt,
        siz, cms, cmt, masks, maskt, shifts, shiftt, texWidth, texHeight, tileWidth, tileHeight, rawUMin, rawUMax,
        rawVMin, rawVMax, normUMin, normUMax, normVMin, normVMax, x0, y0, z0, w0, x1, y1, z1, w1, x2, y2, z2, w2);
}

void Interpreter::ImportTextureRgba16(int tile, bool importReplacement) {
    const RawTexMetadata* metadata = &mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].raw_tex_metadata;
    const uint8_t* addr =
        importReplacement && (metadata->resource != nullptr)
            ? GetMaskedReplacementData(mMaskedTextures, GetBaseTexturePath(metadata->resource->GetInitData()->Path))
            : mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].addr;

    if (addr == nullptr) {
        SPDLOG_ERROR("ImportTextureRgba16: null texture address for tile {}", tile);
        return;
    }

    uint32_t sizeBytes = mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].size_bytes;
    uint32_t fullImageLineSizeBytes =
        mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].full_image_line_size_bytes;
    uint32_t line_size_bytes = mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].line_size_bytes;

    uint32_t widthBytes = GetEffectiveLineSize(line_size_bytes, fullImageLineSizeBytes, sizeBytes,
                                               mRdp->texture_tile[tile].line_size_bytes);
    // naturalWidth = pre-clamp TMEM-line-derived pixel count. Used for the
    // DRAM stride reassignment below so that the decode's source indexing
    // stays consistent with the real row stride when `width` (the decode
    // extent) gets clamped tighter by tile/mask bounds. Mirrors the
    // ClampUploadWidthToTile fix landed for IA8/I4 (see
    // docs/bugs/title_border_right_edge_slice_2026-04-14.md); without it the
    // reassignment used the clamped width and pulled each subsequent row
    // from the wrong offset → diagonal shear across every RGBA16 sprite
    // whose `bitmap->width < bitmap->width_img`.
    uint32_t naturalWidth = widthBytes / 2;
    uint32_t width = naturalWidth;
    uint32_t height = widthBytes > 0 ? sizeBytes / widthBytes : 0;

    // Clamp to tile dimensions from SetTileSize (mipmap pyramids include all levels).
    uint32_t tile_w = (uint32_t)((mRdp->texture_tile[tile].lrs - mRdp->texture_tile[tile].uls + 4) / 4);
    uint32_t tile_h = (uint32_t)((mRdp->texture_tile[tile].lrt - mRdp->texture_tile[tile].ult + 4) / 4);
    if (tile_w > 0 && tile_w < width) {
        width = tile_w;
    }
    if (tile_h > 0 && tile_h < height) {
        height = tile_h;
    }

    // PORT: clamp to masks/maskt wrap bounds. See ImportTextureCi4 for rationale.
    if (mRdp->texture_tile[tile].masks > 0) {
        uint32_t mask_w = 1u << mRdp->texture_tile[tile].masks;
        if (mask_w > 0 && mask_w < width) {
            width = mask_w;
        }
    }
    if (mRdp->texture_tile[tile].maskt > 0) {
        uint32_t mask_h = 1u << mRdp->texture_tile[tile].maskt;
        if (mask_h > 0 && mask_h < height) {
            height = mask_h;
        }
    }

    // Set DRAM row stride from the *unclamped* naturalWidth when the
    // LoadBlock was flat (no explicit width). See naturalWidth comment above.
    if (fullImageLineSizeBytes == sizeBytes) {
        fullImageLineSizeBytes = naturalWidth * 2;
    }

    uint32_t i = 0;

    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t clrIdx = (y * (fullImageLineSizeBytes / 2)) + (x);

            uint16_t col16 = (addr[2 * clrIdx] << 8) | addr[2 * clrIdx + 1];
            uint8_t a = col16 & 1;
            uint8_t r = col16 >> 11;
            uint8_t g = (col16 >> 6) & 0x1f;
            uint8_t b = (col16 >> 1) & 0x1f;
            mTexUploadBuffer[4 * i + 0] = SCALE_5_8(r);
            mTexUploadBuffer[4 * i + 1] = SCALE_5_8(g);
            mTexUploadBuffer[4 * i + 2] = SCALE_5_8(b);
            mTexUploadBuffer[4 * i + 3] = a ? 255 : 0;

            i++;
        }
    }

    Ssb64RenderDiagLogUpload("RGBA16", metadata, addr, tile, mRdp->texture_tile[tile].tmem_index, width, height);
    // Hi-res texture pack hook (port-side). Host hashes the decoded RGBA8
    // and substitutes a higher-resolution buffer if the pack contains one.
    // Skipped on importReplacement uploads (LUS's own alt-asset path).
    if (!importReplacement) {
        if (GfxHiResHookFn hook = gHiResHook) {
            const uint8_t* packBuf = nullptr;
            uint16_t packW = 0, packH = 0;
            if (hook(G_IM_FMT_RGBA, G_IM_SIZ_16b, mTexUploadBuffer, (uint16_t)width, (uint16_t)height,
                     &packBuf, &packW, &packH)
                && packBuf != nullptr && packW > 0 && packH > 0) {
                mRapi->UploadTexture(packBuf, packW, packH);
                return;
            }
        }
    }
    mRapi->UploadTexture(mTexUploadBuffer, width, height);
}

void Interpreter::ImportTextureRgba32(int tile, bool importReplacement) {
    const RawTexMetadata* metadata = &mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].raw_tex_metadata;
    const uint8_t* addr =
        importReplacement && (metadata->resource != nullptr)
            ? GetMaskedReplacementData(mMaskedTextures, GetBaseTexturePath(metadata->resource->GetInitData()->Path))
            : mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].addr;

    if (addr == nullptr) {
        SPDLOG_ERROR("ImportTextureRgba32: null texture address for tile {}", tile);
        return;
    }

    uint32_t size_bytes = mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].size_bytes;
    uint32_t full_image_line_size_bytes =
        mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].full_image_line_size_bytes;
    uint32_t line_size_bytes = mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].line_size_bytes;

    uint32_t widthBytes = GetEffectiveLineSize(line_size_bytes, full_image_line_size_bytes, size_bytes,
                                               mRdp->texture_tile[tile].line_size_bytes * 2);
    // See ImportTextureRgba16 for the naturalWidth / ClampUploadWidthToTile
    // rationale — same class of bug, stretched across 4-byte pixels.
    uint32_t naturalWidth = widthBytes / 4;
    uint32_t width = naturalWidth;
    uint32_t height = widthBytes > 0 ? size_bytes / widthBytes : 0;

    // Clamp to tile dimensions from SetTileSize
    uint32_t tile_w = (uint32_t)((mRdp->texture_tile[tile].lrs - mRdp->texture_tile[tile].uls + 4) / 4);
    uint32_t tile_h = (uint32_t)((mRdp->texture_tile[tile].lrt - mRdp->texture_tile[tile].ult + 4) / 4);
    if (tile_w > 0 && tile_w < width) {
        width = tile_w;
    }
    if (tile_h > 0 && tile_h < height) {
        height = tile_h;
    }

    if (full_image_line_size_bytes == size_bytes) {
        full_image_line_size_bytes = naturalWidth * 4;
    }

    // Copy pixel by pixel, respecting full image stride (handles sub-tile loads)
    uint32_t fullImageStridePixels = full_image_line_size_bytes / 4;
    uint32_t i = 0;
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t srcIdx = y * fullImageStridePixels + x;
            mTexUploadBuffer[4 * i + 0] = addr[4 * srcIdx + 0];
            mTexUploadBuffer[4 * i + 1] = addr[4 * srcIdx + 1];
            mTexUploadBuffer[4 * i + 2] = addr[4 * srcIdx + 2];
            mTexUploadBuffer[4 * i + 3] = addr[4 * srcIdx + 3];
            i++;
        }
    }
    if (!importReplacement) {
        if (GfxHiResHookFn hook = gHiResHook) {
            const uint8_t* packBuf = nullptr;
            uint16_t packW = 0, packH = 0;
            if (hook(G_IM_FMT_RGBA, G_IM_SIZ_32b, mTexUploadBuffer, (uint16_t)width, (uint16_t)height,
                     &packBuf, &packW, &packH)
                && packBuf != nullptr && packW > 0 && packH > 0) {
                mRapi->UploadTexture(packBuf, packW, packH);
                return;
            }
        }
    }
    mRapi->UploadTexture(mTexUploadBuffer, width, height);
}

void Interpreter::ImportTextureIA4(int tile, bool importReplacement) {
    const RawTexMetadata* metadata = &mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].raw_tex_metadata;
    const uint8_t* addr =
        importReplacement && (metadata->resource != nullptr)
            ? GetMaskedReplacementData(mMaskedTextures, GetBaseTexturePath(metadata->resource->GetInitData()->Path))
            : mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].addr;

    if (addr == nullptr) {
        SPDLOG_ERROR("ImportTextureIA4: null texture address for tile {}", tile);
        return;
    }

    uint32_t sizeBytes = mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].size_bytes;
    uint32_t fullImageLineSizeBytes =
        mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].full_image_line_size_bytes;
    uint32_t lineSizeBytes = mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].line_size_bytes;

    uint32_t widthBytes = GetEffectiveLineSize(lineSizeBytes, fullImageLineSizeBytes, sizeBytes,
                                               mRdp->texture_tile[tile].line_size_bytes);
    uint32_t naturalWidth = widthBytes * 2;
    uint32_t height = widthBytes > 0 ? sizeBytes / widthBytes : 0;

    if (fullImageLineSizeBytes == sizeBytes) {
        fullImageLineSizeBytes = widthBytes;
    }

    // Clamp upload width to the SetTileSize extent. Same fix family as IA8/I4
    // (see docs/bugs/title_border_right_edge_slice_2026-04-14.md). VS Record
    // digit sprites are 4×7 IA4 tiles whose TMEM line stride rounds up to a
    // 16-pixel-wide upload; without this clamp the upload is 16 pixels wide
    // but `GfxSpTri1` normalises UVs by the SetTileSize-clamped tex_width=4,
    // so each output pixel samples GPU column ~4·n instead of n — only ~1
    // texel of useful digit data is visible per output column, producing the
    // "squished" appearance reported in issue #2.
    uint32_t width = ClampUploadWidthToTile(naturalWidth, mRdp->texture_tile[tile].uls,
                                            mRdp->texture_tile[tile].lrs);

    uint32_t i = 0;
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t srcPixelIdx = y * (fullImageLineSizeBytes * 2) + x;
            uint8_t byte = addr[srcPixelIdx / 2];
            uint8_t part = (byte >> (4 - (srcPixelIdx % 2) * 4)) & 0xf;
            uint8_t intensity = part >> 1;
            uint8_t alpha = part & 1;
            mTexUploadBuffer[4 * i + 0] = SCALE_3_8(intensity);
            mTexUploadBuffer[4 * i + 1] = SCALE_3_8(intensity);
            mTexUploadBuffer[4 * i + 2] = SCALE_3_8(intensity);
            mTexUploadBuffer[4 * i + 3] = alpha ? 255 : 0;
            i++;
        }
    }

    if (!importReplacement) {
        if (GfxHiResHookFn hook = gHiResHook) {
            const uint8_t* packBuf = nullptr;
            uint16_t packW = 0, packH = 0;
            if (hook(G_IM_FMT_IA, G_IM_SIZ_4b, mTexUploadBuffer, (uint16_t)width, (uint16_t)height,
                     &packBuf, &packW, &packH)
                && packBuf != nullptr && packW > 0 && packH > 0) {
                mRapi->UploadTexture(packBuf, packW, packH);
                return;
            }
        }
    }
    mRapi->UploadTexture(mTexUploadBuffer, width, height);
}

void Interpreter::ImportTextureIA8(int tile, bool importReplacement) {
    const RawTexMetadata* metadata = &mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].raw_tex_metadata;
    const uint8_t* addr =
        importReplacement && (metadata->resource != nullptr)
            ? GetMaskedReplacementData(mMaskedTextures, GetBaseTexturePath(metadata->resource->GetInitData()->Path))
            : mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].addr;

    if (addr == nullptr) {
        SPDLOG_ERROR("ImportTextureIA8: null texture address for tile {}", tile);
        return;
    }

    uint32_t sizeBytes = mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].size_bytes;
    uint32_t fullImageLineSizeBytes =
        mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].full_image_line_size_bytes;
    uint32_t lineSizeBytes = mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].line_size_bytes;

    uint32_t naturalWidth = GetEffectiveLineSize(lineSizeBytes, fullImageLineSizeBytes, sizeBytes,
                                                 mRdp->texture_tile[tile].line_size_bytes);
    uint32_t height = naturalWidth > 0 ? sizeBytes / naturalWidth : 0;

    if (fullImageLineSizeBytes == sizeBytes) {
        fullImageLineSizeBytes = naturalWidth;
    }

    uint32_t width = ClampUploadWidthToTile(naturalWidth, mRdp->texture_tile[tile].uls,
                                            mRdp->texture_tile[tile].lrs);

    uint32_t i = 0;
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t srcIdx = y * fullImageLineSizeBytes + x;
            uint8_t intensity = addr[srcIdx] >> 4;
            uint8_t alpha = addr[srcIdx] & 0xf;
            mTexUploadBuffer[4 * i + 0] = SCALE_4_8(intensity);
            mTexUploadBuffer[4 * i + 1] = SCALE_4_8(intensity);
            mTexUploadBuffer[4 * i + 2] = SCALE_4_8(intensity);
            mTexUploadBuffer[4 * i + 3] = SCALE_4_8(alpha);
            i++;
        }
    }

    if (!importReplacement) {
        if (GfxHiResHookFn hook = gHiResHook) {
            const uint8_t* packBuf = nullptr;
            uint16_t packW = 0, packH = 0;
            if (hook(G_IM_FMT_IA, G_IM_SIZ_8b, mTexUploadBuffer, (uint16_t)width, (uint16_t)height,
                     &packBuf, &packW, &packH)
                && packBuf != nullptr && packW > 0 && packH > 0) {
                mRapi->UploadTexture(packBuf, packW, packH);
                return;
            }
        }
    }
    mRapi->UploadTexture(mTexUploadBuffer, width, height);
}

void Interpreter::ImportTextureIA16(int tile, bool importReplacement) {
    const RawTexMetadata* metadata = &mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].raw_tex_metadata;
    const uint8_t* addr =
        importReplacement && (metadata->resource != nullptr)
            ? GetMaskedReplacementData(mMaskedTextures, GetBaseTexturePath(metadata->resource->GetInitData()->Path))
            : mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].addr;

    if (addr == nullptr) {
        SPDLOG_ERROR("ImportTextureIA16: null texture address for tile {}", tile);
        return;
    }

    uint32_t size_bytes = mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].size_bytes;
    uint32_t full_image_line_size_bytes =
        mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].full_image_line_size_bytes;
    uint32_t line_size_bytes = mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].line_size_bytes;

    uint32_t widthBytes = GetEffectiveLineSize(line_size_bytes, full_image_line_size_bytes, size_bytes,
                                               mRdp->texture_tile[tile].line_size_bytes);
    uint32_t width = widthBytes / 2;
    uint32_t height = widthBytes > 0 ? size_bytes / widthBytes : 0;

    // A single line of pixels should not equal the entire image (height == 1 non-withstanding)
    if (full_image_line_size_bytes == size_bytes) {
        full_image_line_size_bytes = width * 2;
    }

    uint32_t i = 0;

    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t clrIdx = (y * (full_image_line_size_bytes / 2)) + (x);

            uint8_t intensity = addr[2 * clrIdx];
            uint8_t alpha = addr[2 * clrIdx + 1];
            uint8_t r = intensity;
            uint8_t g = intensity;
            uint8_t b = intensity;
            mTexUploadBuffer[4 * i + 0] = r;
            mTexUploadBuffer[4 * i + 1] = g;
            mTexUploadBuffer[4 * i + 2] = b;
            mTexUploadBuffer[4 * i + 3] = alpha;

            i++;
        }
    }

    if (!importReplacement) {
        if (GfxHiResHookFn hook = gHiResHook) {
            const uint8_t* packBuf = nullptr;
            uint16_t packW = 0, packH = 0;
            if (hook(G_IM_FMT_IA, G_IM_SIZ_16b, mTexUploadBuffer, (uint16_t)width, (uint16_t)height,
                     &packBuf, &packW, &packH)
                && packBuf != nullptr && packW > 0 && packH > 0) {
                mRapi->UploadTexture(packBuf, packW, packH);
                return;
            }
        }
    }
    mRapi->UploadTexture(mTexUploadBuffer, width, height);
}

void Interpreter::ImportTextureI4(int tile, bool importReplacement) {
    const RawTexMetadata* metadata = &mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].raw_tex_metadata;
    const uint8_t* addr =
        importReplacement && (metadata->resource != nullptr)
            ? GetMaskedReplacementData(mMaskedTextures, GetBaseTexturePath(metadata->resource->GetInitData()->Path))
            : mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].addr;

    if (addr == nullptr) {
        SPDLOG_ERROR("ImportTextureI4: null texture address for tile {}", tile);
        return;
    }

    uint32_t sizeBytes = mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].size_bytes;
    uint32_t fullImageLineSizeBytes =
        mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].full_image_line_size_bytes;
    uint32_t lineSizeBytes = mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].line_size_bytes;

    uint32_t widthBytes = GetEffectiveLineSize(lineSizeBytes, fullImageLineSizeBytes, sizeBytes,
                                               mRdp->texture_tile[tile].line_size_bytes);
    uint32_t naturalWidth = widthBytes * 2;
    uint32_t height = widthBytes > 0 ? sizeBytes / widthBytes : 0;

    // A single line of pixels should not equal the entire image (height == 1 non-withstanding)
    if (fullImageLineSizeBytes == sizeBytes) {
        fullImageLineSizeBytes = naturalWidth / 2;
    }

    uint32_t width = ClampUploadWidthToTile(naturalWidth, mRdp->texture_tile[tile].uls,
                                            mRdp->texture_tile[tile].lrs);

    uint32_t i = 0;

    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t clrIdx = (y * (fullImageLineSizeBytes * 2)) + (x);

            uint8_t byte = addr[clrIdx / 2];
            uint8_t part = (byte >> (4 - (clrIdx % 2) * 4)) & 0xf;
            uint8_t intensity = part;
            uint8_t r = intensity;
            uint8_t g = intensity;
            uint8_t b = intensity;
            uint8_t a = intensity;
            mTexUploadBuffer[4 * i + 0] = SCALE_4_8(r);
            mTexUploadBuffer[4 * i + 1] = SCALE_4_8(g);
            mTexUploadBuffer[4 * i + 2] = SCALE_4_8(b);
            mTexUploadBuffer[4 * i + 3] = SCALE_4_8(a);

            i++;
        }
    }

    if (!importReplacement) {
        if (GfxHiResHookFn hook = gHiResHook) {
            const uint8_t* packBuf = nullptr;
            uint16_t packW = 0, packH = 0;
            if (hook(G_IM_FMT_I, G_IM_SIZ_4b, mTexUploadBuffer, (uint16_t)width, (uint16_t)height,
                     &packBuf, &packW, &packH)
                && packBuf != nullptr && packW > 0 && packH > 0) {
                mRapi->UploadTexture(packBuf, packW, packH);
                return;
            }
        }
    }
    mRapi->UploadTexture(mTexUploadBuffer, width, height);
}

void Interpreter::ImportTextureI8(int tile, bool importReplacement) {
    const RawTexMetadata* metadata = &mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].raw_tex_metadata;
    const uint8_t* addr =
        importReplacement && (metadata->resource != nullptr)
            ? GetMaskedReplacementData(mMaskedTextures, GetBaseTexturePath(metadata->resource->GetInitData()->Path))
            : mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].addr;

    if (addr == nullptr) {
        SPDLOG_ERROR("ImportTextureI8: null texture address for tile {}", tile);
        return;
    }

    uint32_t sizeBytes = mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].size_bytes;
    uint32_t fullImageLineSizeBytes =
        mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].full_image_line_size_bytes;
    uint32_t lineSizeBytes = mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].line_size_bytes;

    uint32_t width = GetEffectiveLineSize(lineSizeBytes, fullImageLineSizeBytes, sizeBytes,
                                          mRdp->texture_tile[tile].line_size_bytes);
    uint32_t height = width > 0 ? sizeBytes / width : 0;

    if (fullImageLineSizeBytes == sizeBytes) {
        fullImageLineSizeBytes = width;
    }

    uint32_t i = 0;
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint8_t intensity = addr[y * fullImageLineSizeBytes + x];
            mTexUploadBuffer[4 * i + 0] = intensity;
            mTexUploadBuffer[4 * i + 1] = intensity;
            mTexUploadBuffer[4 * i + 2] = intensity;
            mTexUploadBuffer[4 * i + 3] = intensity;
            i++;
        }
    }

    if (!importReplacement) {
        if (GfxHiResHookFn hook = gHiResHook) {
            const uint8_t* packBuf = nullptr;
            uint16_t packW = 0, packH = 0;
            if (hook(G_IM_FMT_I, G_IM_SIZ_8b, mTexUploadBuffer, (uint16_t)width, (uint16_t)height,
                     &packBuf, &packW, &packH)
                && packBuf != nullptr && packW > 0 && packH > 0) {
                mRapi->UploadTexture(packBuf, packW, packH);
                return;
            }
        }
    }
    mRapi->UploadTexture(mTexUploadBuffer, width, height);
}

void Interpreter::ImportTextureCi4(int tile, bool importReplacement) {
    uint32_t fullImageLineSizeBytes =
        mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].full_image_line_size_bytes;
    const RawTexMetadata* metadata = &mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].raw_tex_metadata;
    const uint8_t* addr =
        importReplacement && (metadata->resource != nullptr)
            ? GetMaskedReplacementData(mMaskedTextures, GetBaseTexturePath(metadata->resource->GetInitData()->Path))
            : mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].addr;

    if (addr == nullptr) {
        SPDLOG_ERROR("ImportTextureCi4: null texture address for tile {}", tile);
        return;
    }

    uint32_t sizeBytes = mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].size_bytes;
    uint32_t lineSizeBytes = mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].line_size_bytes;
    uint32_t palIdx = mRdp->texture_tile[tile].palette; // 0-15

    const uint8_t* palette;

    if (mRdp->palettes[palIdx / 8] == nullptr) {
        SPDLOG_WARN("CI4: null palette slot {} for palIdx={}", palIdx / 8, palIdx);
        return;
    }
    palette = mRdp->palettes[palIdx / 8] + (palIdx % 8) * 16 * 2;

    uint32_t baseLineSizeBytes = GetEffectiveLineSize(lineSizeBytes, fullImageLineSizeBytes, sizeBytes,
                                                      mRdp->texture_tile[tile].line_size_bytes);
    uint32_t resultLineSizeBytes = baseLineSizeBytes;

    if (metadata->h_byte_scale != 1) {
        resultLineSizeBytes *= metadata->h_byte_scale;
    }

    // CI4: 2 pixels per byte
    uint32_t width = resultLineSizeBytes * 2;
    uint32_t height = resultLineSizeBytes > 0 ? sizeBytes / resultLineSizeBytes : 0;

    // Clamp to tile dimensions from SetTileSize
    uint32_t tile_w = (uint32_t)((mRdp->texture_tile[tile].lrs - mRdp->texture_tile[tile].uls + 4) / 4);
    uint32_t tile_h = (uint32_t)((mRdp->texture_tile[tile].lrt - mRdp->texture_tile[tile].ult + 4) / 4);
    if (tile_w > 0 && tile_w < width) {
        width = tile_w;
    }
    if (tile_h > 0 && tile_h < height) {
        height = tile_h;
    }

    // PORT: Clamp decoded width/height to the tile's `masks/maskt` wrap
    // bounds if they're tighter than the line-derived width.  N64 hardware
    // uses masks to define the "active" texture size, with the wrap mode
    // repeating it beyond.  Fast3D's line_size_bytes-based width can be
    // larger than the actual logical texture (e.g. a 16px-wide line with
    // masks=3 = 8px-wide texture), so the trailing columns are padding
    // whose indices decode to whatever's lying in TMEM — typically zero,
    // yielding a half-black image for CI4 materials whose palette entry 0
    // is opaque black.  Clamping width/height to the masks-based size
    // matches the N64 sampler's behaviour and eliminates the black padding.
    if (mRdp->texture_tile[tile].masks > 0) {
        uint32_t mask_w = 1u << mRdp->texture_tile[tile].masks;
        if (mask_w > 0 && mask_w < width) {
            width = mask_w;
        }
    }
    if (mRdp->texture_tile[tile].maskt > 0) {
        uint32_t mask_h = 1u << mRdp->texture_tile[tile].maskt;
        if (mask_h > 0 && mask_h < height) {
            height = mask_h;
        }
    }

    if (fullImageLineSizeBytes == sizeBytes) {
        fullImageLineSizeBytes = resultLineSizeBytes;
    }

    uint32_t i = 0;
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t srcPixelIdx = y * (fullImageLineSizeBytes * 2) + x;
            uint8_t byte = addr[srcPixelIdx / 2];
            uint8_t idx = (byte >> (4 - (srcPixelIdx % 2) * 4)) & 0xf;
            uint16_t col16 = (palette[idx * 2] << 8) | palette[idx * 2 + 1]; // Big endian load
            uint8_t a = col16 & 1;
            uint8_t r = col16 >> 11;
            uint8_t g = (col16 >> 6) & 0x1f;
            uint8_t b = (col16 >> 1) & 0x1f;
            mTexUploadBuffer[4 * i + 0] = SCALE_5_8(r);
            mTexUploadBuffer[4 * i + 1] = SCALE_5_8(g);
            mTexUploadBuffer[4 * i + 2] = SCALE_5_8(b);
            mTexUploadBuffer[4 * i + 3] = a ? 255 : 0;
            i++;
        }
    }

    Ssb64RenderDiagLogUpload("CI4", metadata, addr, tile, mRdp->texture_tile[tile].tmem_index, width, height);
    if (!importReplacement) {
        if (GfxHiResHookFn hook = gHiResHook) {
            const uint8_t* packBuf = nullptr;
            uint16_t packW = 0, packH = 0;
            if (hook(G_IM_FMT_CI, G_IM_SIZ_4b, mTexUploadBuffer, (uint16_t)width, (uint16_t)height,
                     &packBuf, &packW, &packH)
                && packBuf != nullptr && packW > 0 && packH > 0) {
                mRapi->UploadTexture(packBuf, packW, packH);
                return;
            }
        }
    }
    mRapi->UploadTexture(mTexUploadBuffer, width, height);
}

void Interpreter::ImportTextureCi8(int tile, bool importReplacement) {
    const RawTexMetadata* metadata = &mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].raw_tex_metadata;
    const uint8_t* addr =
        importReplacement && (metadata->resource != nullptr)
            ? GetMaskedReplacementData(mMaskedTextures, GetBaseTexturePath(metadata->resource->GetInitData()->Path))
            : mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].addr;

    if (addr == nullptr) {
        SPDLOG_ERROR("ImportTextureCi8: null texture address for tile {}", tile);
        return;
    }

    uint32_t sizeBytes = mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].size_bytes;
    uint32_t fullImageLineSizeBytes =
        mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].full_image_line_size_bytes;
    uint32_t lineSizeBytes = mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].line_size_bytes;

    if (mRdp->palettes[0] == nullptr || mRdp->palettes[1] == nullptr) {
        SPDLOG_WARN("CI8: null palette (pal0={}, pal1={})", static_cast<const void*>(mRdp->palettes[0]),
                    static_cast<const void*>(mRdp->palettes[1]));
        return;
    }

    // Compute width/height + clamps *before* decode so the decode loop
    // produces a `width × height` packed buffer that matches the
    // UploadTexture call below.  Previously the decode wrote
    // `lineSizeBytes × (sizeBytes/lineSizeBytes)` pixels contiguously and
    // the post-decode width clamp made the subsequent upload read rows at
    // the wrong offset — diagonal shear across every CI8 sprite where
    // `bitmap->width < bitmap->width_img` (e.g. the tutorial "How to Play"
    // banner and the textbox).  Structurally mirrors the RGBA16 /
    // ImportTextureCi4 paths.
    uint32_t baseLineSizeBytes = GetEffectiveLineSize(lineSizeBytes, fullImageLineSizeBytes, sizeBytes,
                                                      mRdp->texture_tile[tile].line_size_bytes);
    uint32_t resultLineSizeBytes = baseLineSizeBytes;
    if (metadata->h_byte_scale != 1) {
        resultLineSizeBytes *= metadata->h_byte_scale;
    }

    uint32_t naturalWidth = resultLineSizeBytes;
    uint32_t width = naturalWidth;
    uint32_t height = resultLineSizeBytes > 0 ? sizeBytes / resultLineSizeBytes : 0;

    // Clamp to tile dimensions from SetTileSize
    uint32_t tile_w = (uint32_t)((mRdp->texture_tile[tile].lrs - mRdp->texture_tile[tile].uls + 4) / 4);
    uint32_t tile_h = (uint32_t)((mRdp->texture_tile[tile].lrt - mRdp->texture_tile[tile].ult + 4) / 4);
    if (tile_w > 0 && tile_w < width) {
        width = tile_w;
    }
    if (tile_h > 0 && tile_h < height) {
        height = tile_h;
    }

    // PORT: clamp to masks/maskt wrap bounds. See ImportTextureCi4 for rationale.
    if (mRdp->texture_tile[tile].masks > 0) {
        uint32_t mask_w = 1u << mRdp->texture_tile[tile].masks;
        if (mask_w > 0 && mask_w < width) {
            width = mask_w;
        }
    }
    if (mRdp->texture_tile[tile].maskt > 0) {
        uint32_t mask_h = 1u << mRdp->texture_tile[tile].maskt;
        if (mask_h > 0 && mask_h < height) {
            height = mask_h;
        }
    }

    // Flat-LoadBlock fallback: the real DRAM row stride is naturalWidth
    // bytes (CI8 = 1 byte per texel), not the whole image.  Matches
    // ImportTextureCi4 / ImportTextureRgba16.
    if (fullImageLineSizeBytes == sizeBytes) {
        fullImageLineSizeBytes = naturalWidth;
    }

    uint32_t i = 0;
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint8_t idx = addr[y * fullImageLineSizeBytes + x];
            uint16_t col16 = (mRdp->palettes[idx / 128][(idx % 128) * 2] << 8) |
                             mRdp->palettes[idx / 128][(idx % 128) * 2 + 1]; // Big endian load
            uint8_t a = col16 & 1;
            uint8_t r = col16 >> 11;
            uint8_t g = (col16 >> 6) & 0x1f;
            uint8_t b = (col16 >> 1) & 0x1f;
            mTexUploadBuffer[4 * i + 0] = SCALE_5_8(r);
            mTexUploadBuffer[4 * i + 1] = SCALE_5_8(g);
            mTexUploadBuffer[4 * i + 2] = SCALE_5_8(b);
            mTexUploadBuffer[4 * i + 3] = a ? 255 : 0;
            i++;
        }
    }

    Ssb64RenderDiagLogUpload("CI8", metadata, addr, tile, mRdp->texture_tile[tile].tmem_index, width, height);
    if (!importReplacement) {
        if (GfxHiResHookFn hook = gHiResHook) {
            const uint8_t* packBuf = nullptr;
            uint16_t packW = 0, packH = 0;
            if (hook(G_IM_FMT_CI, G_IM_SIZ_8b, mTexUploadBuffer, (uint16_t)width, (uint16_t)height,
                     &packBuf, &packW, &packH)
                && packBuf != nullptr && packW > 0 && packH > 0) {
                mRapi->UploadTexture(packBuf, packW, packH);
                return;
            }
        }
    }
    mRapi->UploadTexture(mTexUploadBuffer, width, height);
}

void Interpreter::ImportTextureImg(int tile, bool importReplacement) {
    const RawTexMetadata* metadata = &mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].raw_tex_metadata;
    const uint8_t* addr =
        importReplacement && (metadata->resource != nullptr)
            ? GetMaskedReplacementData(mMaskedTextures, GetBaseTexturePath(metadata->resource->GetInitData()->Path))
            : mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].addr;

    if (addr == nullptr) {
        SPDLOG_ERROR("ImportTextureImg: null texture address for tile {}", tile);
        return;
    }

    uint16_t width = metadata->width;
    uint16_t height = metadata->height;
    Ssb64RenderDiagLogUpload("IMG", metadata, addr, tile, mRdp->texture_tile[tile].tmem_index, width, height);
    mRapi->UploadTexture(addr, width, height);
}

void Interpreter::ImportTextureRaw(int tile, bool importReplacement) {
    const RawTexMetadata* metadata = &mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].raw_tex_metadata;
    const uint8_t* addr =
        importReplacement && (metadata->resource != nullptr)
            ? GetMaskedReplacementData(mMaskedTextures, GetBaseTexturePath(metadata->resource->GetInitData()->Path))
            : mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].addr;

    if (addr == nullptr) {
        SPDLOG_ERROR("ImportTextureRaw: null texture address for tile {}", tile);
        return;
    }

    uint16_t width = metadata->width;
    uint16_t height = metadata->height;
    Fast::TextureType type = metadata->type;
    std::shared_ptr<Fast::Texture> resource = metadata->resource;

    // if texture type is CI4 or CI8 we need to apply tlut to it
    switch (type) {
        case Fast::TextureType::Palette4bpp:
            ImportTextureCi4(tile, importReplacement);
            return;
        case Fast::TextureType::Palette8bpp:
            ImportTextureCi8(tile, importReplacement);
            return;
        default:
            break;
    }

    uint32_t numLoadedBytes = mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].size_bytes;
    uint32_t numOriginallyLoadedBytes = mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].orig_size_bytes;

    uint32_t resultOrigLineSize = mRdp->texture_tile[tile].line_size_bytes;
    switch (mRdp->texture_tile[tile].siz) {
        case G_IM_SIZ_32b:
            resultOrigLineSize *= 2;
            break;
    }
    uint32_t resultOrigHeight = numOriginallyLoadedBytes / resultOrigLineSize;
    uint32_t resultNewLineSize = resultOrigLineSize * metadata->h_byte_scale;
    uint32_t resultNewHeight = resultOrigHeight * metadata->v_pixel_scale;

    if (resultNewLineSize == 4 * width && resultNewHeight == height) {
        // Can use the texture directly since it has the correct dimensions
        Ssb64RenderDiagLogUpload("RAW-direct", metadata, addr, tile, mRdp->texture_tile[tile].tmem_index, width, height);
        mRapi->UploadTexture(addr, width, height);
        return;
    }

    uint32_t fullImageLineSizeBytes =
        mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].full_image_line_size_bytes;
    uint32_t line_size_bytes = mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].line_size_bytes;

    // Get the resource's true image size
    uint32_t resourceImageSizeBytes = resource->ImageDataSize;
    uint32_t safeFullImageLineSizeBytes = fullImageLineSizeBytes;
    uint32_t safeLineSizeBytes = line_size_bytes;
    uint32_t safeLoadedBytes = numLoadedBytes;

    // Sometimes the texture load commands will specify a size larger than the authentic texture
    // Normally the OOB info is read as garbage, but will cause a crash on some platforms
    // Restrict the bytes to a safe amount
    if (numLoadedBytes > resourceImageSizeBytes) {
        safeLoadedBytes = resourceImageSizeBytes;
        safeLineSizeBytes = resourceImageSizeBytes;
        safeFullImageLineSizeBytes = resourceImageSizeBytes;
    }

    // Safely only copy the amount of bytes the resource can allow
    for (uint32_t i = 0, j = 0; i < safeLoadedBytes; i += safeLineSizeBytes, j += safeFullImageLineSizeBytes) {
        memcpy(mTexUploadBuffer + i, addr + j, safeLineSizeBytes);
    }

    // Set the remaining bytes to load as 0
    if (numLoadedBytes > resourceImageSizeBytes) {
        memset(mTexUploadBuffer + resourceImageSizeBytes, 0, numLoadedBytes - resourceImageSizeBytes);
    }

    Ssb64RenderDiagLogUpload("RAW-copy", metadata, addr, tile, mRdp->texture_tile[tile].tmem_index,
                             resultNewLineSize / 4, resultNewHeight);
    mRapi->UploadTexture(mTexUploadBuffer, resultNewLineSize / 4, resultNewHeight);
}

void Interpreter::ImportTexture(int i, int tile, bool importReplacement) {
    uint8_t fmt = mRdp->texture_tile[tile].fmt;
    uint8_t siz = mRdp->texture_tile[tile].siz;
    uint32_t texFlags = mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].tex_flags;
    uint32_t tmemIdex = mRdp->texture_tile[tile].tmem_index;
    uint8_t paletteIndex = mRdp->texture_tile[tile].palette;
    uint32_t origSizeBytes = mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].orig_size_bytes;
    uint16_t tileWidth =
        (uint16_t)(int32_t)((mRdp->texture_tile[tile].lrs - mRdp->texture_tile[tile].uls + 4) / 4);
    uint16_t tileHeight =
        (uint16_t)(int32_t)((mRdp->texture_tile[tile].lrt - mRdp->texture_tile[tile].ult + 4) / 4);

    // Check TLUT mode early -- before cache lookup -- so the fmt override
    // affects both the cache key and the decode path.
    // Only override to CI for 4-bit and 8-bit texels, which are valid CI sizes.
    // 16-bit and 32-bit texels are full-color formats that the N64 RDP decodes
    // natively regardless of TLUT mode.
    uint32_t tlutMode = mRdp->other_mode_h & (3U << G_MDSFT_TEXTLUT);
    if (tlutMode != G_TT_NONE && fmt != G_IM_FMT_CI && (siz == G_IM_SIZ_4b || siz == G_IM_SIZ_8b)) {
        fmt = G_IM_FMT_CI;
    }

    const RawTexMetadata* metadata = &mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].raw_tex_metadata;
    const uint8_t* origAddr =
        importReplacement && (metadata->resource != nullptr)
            ? GetMaskedReplacementData(mMaskedTextures, GetBaseTexturePath(metadata->resource->GetInitData()->Path))
            : mRdp->loaded_texture[tmemIdex].addr;

    if (origAddr == nullptr) {
        // Try the other TMEM slot -- some multi-tile setups only load one slot
        // and expect both tiles to reference it.
        uint32_t otherTmem = tmemIdex ^ 1;
        origAddr = mRdp->loaded_texture[otherTmem].addr;
        if (origAddr == nullptr) {
            SPDLOG_WARN("ImportTexture: null texture address for tile {} (both TMEM slots empty)", tile);
            return;
        }
        SPDLOG_WARN("ImportTexture: tile {} TMEM slot {} empty, falling back to slot {}", tile, tmemIdex, otherTmem);
        tmemIdex = otherTmem;
        origSizeBytes = mRdp->loaded_texture[otherTmem].orig_size_bytes;
        texFlags = mRdp->loaded_texture[otherTmem].tex_flags;
    }

    // FB-texture passthrough: if the caller has registered a range that
    // contains this CPU address as a mirror of a GPU framebuffer
    // (port_capture_register_fb_for_subrect), bypass the CPU decode/upload
    // path and bind the FB directly via SelectTextureFb. Also stash the
    // registration's source-FB UV sub-rect into mFbUvTransform[i] so
    // GfxSpTri1 can remap the consumer's local UV (0..1) to the right
    // slice of the bigger FB -- without this, a multi-tile sprite (TMEM is
    // 4 KB, FB-sized region must be tiled) would render every tile sampling
    // the WHOLE FB.
    //
    // On a miss, reset the transform to identity so a previous hit's
    // sub-rect doesn't leak into a fresh non-FB binding on this tile slot.
    //
    // Skipped for replacement uploads -- those go through the masked-texture path.
    if (i >= 0 && i < 2) {
        mFbUvTransform[i] = FbUvTransform{ 1.0f, 1.0f, 0.0f, 0.0f };
    }
    if (!importReplacement && !mFbTextures.empty()) {
        uintptr_t a = reinterpret_cast<uintptr_t>(origAddr);
        auto it = mFbTextures.upper_bound(a); // first base > a
        if (it != mFbTextures.begin()) {
            --it; // last base <= a
            if (a < it->second.end) {
                Flush();
                mRapi->SelectTextureFb(it->second.fbId);
                if (i >= 0 && i < 2) {
                    const FbTextureRange& r = it->second;

                    /* Per-call UV slice: one registration can cover an entire
                     * tiled image (N64 TMEM is 4 KB so any FB-sized texture
                     * is sampled as N stripes via N gsDPSetTextureImage +
                     * LoadBlock cycles, each with its own offset). The hook
                     * uses the stride and load-size that the prior LoadBlock
                     * already stashed into loaded_texture to figure out which
                     * vertical slice of the registered FB sub-rect this call
                     * represents.
                     *
                     * For the trivial single-tile case (one big SetTextureImage
                     * at base, one LoadBlock that covers the whole range) the
                     * math degenerates to the registered (u0,v0,u1,v1) verbatim. */
                    uint32_t lineBytes = mRdp->loaded_texture[tmemIdex].line_size_bytes;
                    uint32_t loadBytes = mRdp->loaded_texture[tmemIdex].size_bytes;
                    uintptr_t totalBytes = r.end - it->first;
                    float v0Slice = r.v0;
                    float v1Slice = r.v1;
                    if (lineBytes > 0 && loadBytes >= lineBytes && totalBytes >= lineBytes) {
                        uintptr_t totalRows = totalBytes / lineBytes;
                        uintptr_t offBytes = a - it->first;
                        uintptr_t offRows = offBytes / lineBytes;
                        uintptr_t loadRows = loadBytes / lineBytes;
                        if (totalRows > 0 && offRows + loadRows <= totalRows) {
                            float vRange = r.v1 - r.v0;
                            float invTotal = 1.0f / (float)totalRows;
                            v0Slice = r.v0 + (float)offRows * invTotal * vRange;
                            v1Slice = r.v0 + (float)(offRows + loadRows) * invTotal * vRange;
                        }
                    }

                    mFbUvTransform[i] = FbUvTransform{
                        r.u1 - r.u0, v1Slice - v0Slice,
                        r.u0,        v0Slice
                    };
                    // Backends whose FB-as-texture sample direction is
                    // opposite of their FB render direction (OpenGL with
                    // invertY=true) need a V-flip applied here so consumer
                    // UVs remain consistent across backends. D3D11 and Metal
                    // return false from FbNeedsSampleVFlip, so this is a
                    // no-op there.
                    if (mRapi->FbNeedsSampleVFlip(it->second.fbId)) {
                        mFbUvTransform[i].scaleV = -mFbUvTransform[i].scaleV;
                        mFbUvTransform[i].offsetV = 1.0f - mFbUvTransform[i].offsetV;
                    }
                }
                mRdp->textures_changed[i] = false;
                return;
            }
        }
    }

    // Use palette_dram_addr (the original DRAM source) instead of palettes[]
    // (which always points to the staging buffer) so the same texture drawn
    // with different palettes gets distinct cache entries.
    TextureCacheKey key;
    if (fmt == G_IM_FMT_CI) {
        if (siz == G_IM_SIZ_4b) {
            uint8_t palSlot = paletteIndex / 8;
            key = { origAddr,
                    { palSlot == 0 ? mRdp->palette_dram_addr[0] : nullptr,
                      palSlot == 1 ? mRdp->palette_dram_addr[1] : nullptr },
                    fmt,
                    siz,
                    paletteIndex,
                    origSizeBytes,
                    mRdp->texture_tile[tile].masks,
                    mRdp->texture_tile[tile].maskt,
                    tileWidth,
                    tileHeight };
        } else {
            // CI8 uses both palette halves
            key = { origAddr,     { mRdp->palette_dram_addr[0], mRdp->palette_dram_addr[1] }, fmt, siz, paletteIndex,
                    origSizeBytes, mRdp->texture_tile[tile].masks, mRdp->texture_tile[tile].maskt, tileWidth,
                    tileHeight };
        }
    } else {
        key = { origAddr, {}, fmt, siz, paletteIndex, origSizeBytes, mRdp->texture_tile[tile].masks,
                mRdp->texture_tile[tile].maskt, tileWidth, tileHeight };
    }

    Ssb64RenderDiagLogImport("import", metadata, origAddr, i, tile, tmemIdex, fmt, siz, mRdp->texture_tile[tile].cms,
                             mRdp->texture_tile[tile].cmt, mRdp->texture_tile[tile].masks,
                             mRdp->texture_tile[tile].maskt, mRdp->texture_tile[tile].shifts,
                             mRdp->texture_tile[tile].shiftt, mRdp->texture_tile[tile].line_size_bytes,
                             mRdp->loaded_texture[tmemIdex].line_size_bytes,
                             mRdp->loaded_texture[tmemIdex].full_image_line_size_bytes,
                             mRdp->loaded_texture[tmemIdex].size_bytes, origSizeBytes, mRdp->texture_tile[tile].uls,
                             mRdp->texture_tile[tile].ult, mRdp->texture_tile[tile].lrs, mRdp->texture_tile[tile].lrt,
                             importReplacement);

    if (TextureCacheLookup(i, key)) {
        Ssb64RenderDiagLogImport("cache-hit", metadata, origAddr, i, tile, tmemIdex, fmt, siz,
                                 mRdp->texture_tile[tile].cms, mRdp->texture_tile[tile].cmt,
                                 mRdp->texture_tile[tile].masks, mRdp->texture_tile[tile].maskt,
                                 mRdp->texture_tile[tile].shifts, mRdp->texture_tile[tile].shiftt,
                                 mRdp->texture_tile[tile].line_size_bytes,
                                 mRdp->loaded_texture[tmemIdex].line_size_bytes,
                                 mRdp->loaded_texture[tmemIdex].full_image_line_size_bytes,
                                 mRdp->loaded_texture[tmemIdex].size_bytes, origSizeBytes,
                                 mRdp->texture_tile[tile].uls, mRdp->texture_tile[tile].ult,
                                 mRdp->texture_tile[tile].lrs, mRdp->texture_tile[tile].lrt, importReplacement);
        return;
    }

    Ssb64RenderDiagLogImport("cache-miss", metadata, origAddr, i, tile, tmemIdex, fmt, siz,
                             mRdp->texture_tile[tile].cms, mRdp->texture_tile[tile].cmt,
                             mRdp->texture_tile[tile].masks, mRdp->texture_tile[tile].maskt,
                             mRdp->texture_tile[tile].shifts, mRdp->texture_tile[tile].shiftt,
                             mRdp->texture_tile[tile].line_size_bytes,
                             mRdp->loaded_texture[tmemIdex].line_size_bytes,
                             mRdp->loaded_texture[tmemIdex].full_image_line_size_bytes,
                             mRdp->loaded_texture[tmemIdex].size_bytes, origSizeBytes, mRdp->texture_tile[tile].uls,
                             mRdp->texture_tile[tile].ult, mRdp->texture_tile[tile].lrs, mRdp->texture_tile[tile].lrt,
                             importReplacement);

    // Guard against zero-sized textures that would cause divide-by-zero
    // or GPU API errors in UploadTexture.
    if (mRdp->texture_tile[tile].line_size_bytes == 0 || mRdp->loaded_texture[tmemIdex].size_bytes == 0 ||
        origAddr == nullptr) {
        return;
    }

    if ((texFlags & TEX_FLAG_LOAD_AS_IMG) != 0) {
        ImportTextureImg(tile, importReplacement);
        return;
    }

    // if load as raw is set then we load_raw();
    if ((texFlags & TEX_FLAG_LOAD_AS_RAW) != 0) {
        ImportTextureRaw(tile, importReplacement);
        return;
    }

    switch (fmt) {
        case G_IM_FMT_RGBA:
            if (siz == G_IM_SIZ_16b) {
                ImportTextureRgba16(tile, importReplacement);
            } else if (siz == G_IM_SIZ_32b) {
                ImportTextureRgba32(tile, importReplacement);
            } else {
                static int sRgbaErrLogCount = 0;
                if (sRgbaErrLogCount < 40) {
                    SPDLOG_ERROR("RGBA Texture invalid siz={} tile={} fmt={} addr={} line={} width={} height={} palette={} origSize={}",
                        siz, tile, fmt, (const void *)origAddr,
                        mRdp->texture_tile[tile].line_size_bytes,
                        mRdp->texture_tile[tile].lrs - mRdp->texture_tile[tile].uls + 1,
                        mRdp->texture_tile[tile].lrt - mRdp->texture_tile[tile].ult + 1,
                        paletteIndex, origSizeBytes);
                    sRgbaErrLogCount++;
                }
            }
            break;
        case G_IM_FMT_IA:
            if (siz == G_IM_SIZ_4b) {
                ImportTextureIA4(tile, importReplacement);
            } else if (siz == G_IM_SIZ_8b) {
                ImportTextureIA8(tile, importReplacement);
            } else if (siz == G_IM_SIZ_16b) {
                ImportTextureIA16(tile, importReplacement);
            } else {
                SPDLOG_ERROR("IA Texture invalid siz={} tile={}", siz, tile);
            }
            break;
        case G_IM_FMT_CI:
            if (siz == G_IM_SIZ_4b) {
                ImportTextureCi4(tile, importReplacement);
            } else if (siz == G_IM_SIZ_8b) {
                ImportTextureCi8(tile, importReplacement);
            } else if (siz == G_IM_SIZ_16b) {
                // CI+16b is hardware-invalid on N64. The tile's fmt is likely
                // stale from a prior draw. Decode as RGBA16 instead.
                ImportTextureRgba16(tile, importReplacement);
            } else if (siz == G_IM_SIZ_32b) {
                ImportTextureRgba32(tile, importReplacement);
            } else {
                SPDLOG_ERROR("CI Texture with unexpected size = {}", siz);
            }
            break;
        case G_IM_FMT_I:
            if (siz == G_IM_SIZ_4b) {
                ImportTextureI4(tile, importReplacement);
            } else if (siz == G_IM_SIZ_8b) {
                ImportTextureI8(tile, importReplacement);
            } else {
                SPDLOG_ERROR("I Texture that isn't 4 or 8 bit. Size = {}", siz);
            }
            break;
        case G_IM_FMT_YUV:
            SPDLOG_ERROR("YUV Textures not supported");
            break;
        default:
            SPDLOG_ERROR("Invalid texture format. Fmt = {}", fmt);
            break;
    }
}

void Interpreter::ImportTextureMask(int i, int tile) {
    uint32_t tmemIndex = mRdp->texture_tile[tile].tmem_index;
    RawTexMetadata metadata = mRdp->loaded_texture[tmemIndex].raw_tex_metadata;

    if (metadata.resource == nullptr) {
        return;
    }

    auto maskIter = mMaskedTextures.find(GetBaseTexturePath(metadata.resource->GetInitData()->Path));
    if (maskIter == mMaskedTextures.end()) {
        return;
    }

    const uint8_t* orig_addr = maskIter->second.mask;

    if (orig_addr == nullptr) {
        return;
    }

    TextureCacheKey key = { orig_addr, {}, 0, 0, 0, 0, 0, 0, 0, 0 };

    if (TextureCacheLookup(i, key)) {
        return;
    }

    uint32_t width = mRdp->texture_tile[tile].line_size_bytes;
    uint32_t height = mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].orig_size_bytes /
                      mRdp->texture_tile[tile].line_size_bytes;
    switch (mRdp->texture_tile[tile].siz) {
        case G_IM_SIZ_4b:
            width *= 2;
            break;
        case G_IM_SIZ_8b:
        default:
            break;
        case G_IM_SIZ_16b:
            width /= 2;
            break;
        case G_IM_SIZ_32b:
            width /= 4;
            break;
    }

    for (uint32_t texIndex = 0; texIndex < width * height; texIndex++) {
        uint8_t masked = orig_addr[texIndex];
        if (masked) {
            mTexUploadBuffer[4 * texIndex + 0] = 0;
            mTexUploadBuffer[4 * texIndex + 1] = 0;
            mTexUploadBuffer[4 * texIndex + 2] = 0;
            mTexUploadBuffer[4 * texIndex + 3] = 0xFF;
        } else {
            mTexUploadBuffer[4 * texIndex + 0] = 0;
            mTexUploadBuffer[4 * texIndex + 1] = 0;
            mTexUploadBuffer[4 * texIndex + 2] = 0;
            mTexUploadBuffer[4 * texIndex + 3] = 0;
        }
    }

    mRapi->UploadTexture(mTexUploadBuffer, width, height);
}

void Interpreter::NormalizeVector(float v[3]) {
    float s = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    v[0] /= s;
    v[1] /= s;
    v[2] /= s;
}

void Interpreter::TransposedMatrixMul(float res[3], const float a[3], const float b[4][4]) {
    res[0] = a[0] * b[0][0] + a[1] * b[0][1] + a[2] * b[0][2];
    res[1] = a[0] * b[1][0] + a[1] * b[1][1] + a[2] * b[1][2];
    res[2] = a[0] * b[2][0] + a[1] * b[2][1] + a[2] * b[2][2];
}

void Interpreter::MatrixMul(float res[4][4], const float a[4][4], const float b[4][4]) {
    float tmp[4][4];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            tmp[i][j] = a[i][0] * b[0][j] + a[i][1] * b[1][j] + a[i][2] * b[2][j] + a[i][3] * b[3][j];
        }
    }
    memcpy(res, tmp, sizeof(tmp));
}

void Interpreter::CalculateNormalDir(const F3DLight_t* light, float coeffs[3]) {
    float light_dir[3] = { light->dir[0] / 127.0f, light->dir[1] / 127.0f, light->dir[2] / 127.0f };

    Interpreter::TransposedMatrixMul(coeffs, light_dir,
                                     mRsp->modelview_matrix_stack[mRsp->modelview_matrix_stack_size - 1]);
    Interpreter::NormalizeVector(coeffs);
}

void Interpreter::GfxSpMatrix(uint8_t parameters, const int32_t* addr) {
    float matrix[4][4];

    if (auto it = mCurMtxReplacements->find((Mtx*)addr); it != mCurMtxReplacements->end()) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                float v = it->second.mf[i][j];
                int as_int = (int)(v * 65536.0f);
                matrix[i][j] = as_int * (1.0f / 65536.0f);
            }
        }
    } else {
#ifndef GBI_FLOATS
        // Original GBI where fixed point matrices are used
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j += 2) {
                int32_t int_part = addr[i * 2 + j / 2];
                uint32_t frac_part = addr[8 + i * 2 + j / 2];
                matrix[i][j] = (int32_t)((int_part & 0xffff0000) | (frac_part >> 16)) / 65536.0f;
                matrix[i][j + 1] = (int32_t)((int_part << 16) | (frac_part & 0xffff)) / 65536.0f;
            }
        }
#else
        // For a modified GBI where fixed point values are replaced with floats
        memcpy(matrix, addr, sizeof(matrix));
#endif
    }

    const int8_t mtx_projection = get_attr(MTX_PROJECTION);
    const int8_t mtx_load = get_attr(MTX_LOAD);
    const int8_t mtx_push = get_attr(MTX_PUSH);

    if (parameters & mtx_projection) {
        if (parameters & mtx_load) {
            memcpy(mRsp->P_matrix, matrix, sizeof(matrix));
        } else {
            MatrixMul(mRsp->P_matrix, matrix, mRsp->P_matrix);
        }
    } else { // G_MTX_MODELVIEW
        if ((parameters & mtx_push) && mRsp->modelview_matrix_stack_size < 11) {
            ++mRsp->modelview_matrix_stack_size;
            memcpy(mRsp->modelview_matrix_stack[mRsp->modelview_matrix_stack_size - 1],
                   mRsp->modelview_matrix_stack[mRsp->modelview_matrix_stack_size - 2], sizeof(matrix));
        }
        if (parameters & mtx_load) {
            if (mRsp->modelview_matrix_stack_size == 0)
                ++mRsp->modelview_matrix_stack_size;
            memcpy(mRsp->modelview_matrix_stack[mRsp->modelview_matrix_stack_size - 1], matrix, sizeof(matrix));
        } else {
            MatrixMul(mRsp->modelview_matrix_stack[mRsp->modelview_matrix_stack_size - 1], matrix,
                      mRsp->modelview_matrix_stack[mRsp->modelview_matrix_stack_size - 1]);
        }
        mRsp->lights_changed = 1;
    }
    MatrixMul(mRsp->MP_matrix, mRsp->modelview_matrix_stack[mRsp->modelview_matrix_stack_size - 1], mRsp->P_matrix);
}

void Interpreter::GfxSpPopMatrix(uint32_t count) {
    while (count--) {
        if (mRsp->modelview_matrix_stack_size > 0) {
            --mRsp->modelview_matrix_stack_size;
            if (mRsp->modelview_matrix_stack_size > 0) {
                MatrixMul(mRsp->MP_matrix, mRsp->modelview_matrix_stack[mRsp->modelview_matrix_stack_size - 1],
                          mRsp->P_matrix);
            }
        }
    }
    mRsp->lights_changed = true;
}

float Interpreter::GetWidescreenClipXScale() const {
    // SSB64 port: gated on the port-managed mWidescreenActive flag (set by
    // port/widescreen/widescreen.cpp from the gEnhancements.Widescreen CVar).
    // When off, returns 1.0f and 4:3 GBI stretches to fill the window
    // (matching pre-widescreen-feature behavior). When on, returns
    // (4/3)/window_aspect so callers can compress post-projection clip-space
    // X — expanding the visible 4:3 frustum into the wider window. Reads the
    // OS window aspect from mGameWindowViewport rather than mCurDimensions
    // because the latter can be forced to 4:3 by the Advanced Resolution
    // CVar tree (which the SSB64 port also uses for the CVar-off-default 4:3
    // pillarbox); mGameWindowViewport always reflects the actual SDL window
    // size.
    if (!mWidescreenActive) {
        return 1.0f;
    }
    const float win_w = (float)mGameWindowViewport.width;
    const float win_h = (float)mGameWindowViewport.height;
    if (win_w <= 0.0f || win_h <= 0.0f) {
        return 1.0f;
    }
    const float win_aspect = win_w / win_h;
    if (win_aspect <= (4.0f / 3.0f)) {
        return 1.0f;
    }
    return (4.0f / 3.0f) / win_aspect;
}

float Interpreter::AdjXForAspectRatio(float x) const {
    if (mFbActive) {
        return x;
    }
    return x * GetWidescreenClipXScale();
}

// Scale the width and height value based on the ratio of the viewport to the native size
void Interpreter::AdjustWidthHeightForScale(uint32_t& width, uint32_t& height, uint32_t nativeWidth,
                                            uint32_t nativeHeight) const {
    width = round(width * (mCurDimensions.width / (2.0f * (nativeWidth / 2))));
    height = round(height * (mCurDimensions.height / (2.0f * (nativeHeight / 2))));

    if (width == 0) {
        width = 1;
    }
    if (height == 0) {
        height = 1;
    }
}

void Interpreter::GfxSpVertex(size_t n_vertices, size_t dest_index, const F3DVtx* vertices) {
    for (size_t i = 0; i < n_vertices; i++, dest_index++) {
        const F3DVtx_t* v = &vertices[i].v;
        const F3DVtx_tn* vn = &vertices[i].n;
        struct LoadedVertex* d = &mRsp->loaded_vertices[dest_index];

        if (v == nullptr) {
            return;
        }

        float x = v->ob[0] * mRsp->MP_matrix[0][0] + v->ob[1] * mRsp->MP_matrix[1][0] +
                  v->ob[2] * mRsp->MP_matrix[2][0] + mRsp->MP_matrix[3][0];
        float y = v->ob[0] * mRsp->MP_matrix[0][1] + v->ob[1] * mRsp->MP_matrix[1][1] +
                  v->ob[2] * mRsp->MP_matrix[2][1] + mRsp->MP_matrix[3][1];
        float z = v->ob[0] * mRsp->MP_matrix[0][2] + v->ob[1] * mRsp->MP_matrix[1][2] +
                  v->ob[2] * mRsp->MP_matrix[2][2] + mRsp->MP_matrix[3][2];
        float w = v->ob[0] * mRsp->MP_matrix[0][3] + v->ob[1] * mRsp->MP_matrix[1][3] +
                  v->ob[2] * mRsp->MP_matrix[2][3] + mRsp->MP_matrix[3][3];

        float world_pos[3] = { 0.0 };
        if (mRsp->geometry_mode & G_LIGHTING_POSITIONAL) {
            float(*mtx)[4] = mRsp->modelview_matrix_stack[mRsp->modelview_matrix_stack_size - 1];
            world_pos[0] = v->ob[0] * mtx[0][0] + v->ob[1] * mtx[1][0] + v->ob[2] * mtx[2][0] + mtx[3][0];
            world_pos[1] = v->ob[0] * mtx[0][1] + v->ob[1] * mtx[1][1] + v->ob[2] * mtx[2][1] + mtx[3][1];
            world_pos[2] = v->ob[0] * mtx[0][2] + v->ob[1] * mtx[1][2] + v->ob[2] * mtx[2][2] + mtx[3][2];
        }

        x = AdjXForAspectRatio(x);

        short U = v->tc[0] * mRsp->texture_scaling_factor.s >> 16;
        short V = v->tc[1] * mRsp->texture_scaling_factor.t >> 16;

        if (mRsp->geometry_mode & G_LIGHTING) {
            if (mRsp->lights_changed) {
                for (int i = 0; i < mRsp->current_num_lights - 1; i++) {
                    CalculateNormalDir(&mRsp->current_lights[i].l, mRsp->current_lights_coeffs[i]);
                }
                /*static const Light_t lookat_x = {{0, 0, 0}, 0, {0, 0, 0}, 0, {127, 0, 0}, 0};
                static const Light_t lookat_y = {{0, 0, 0}, 0, {0, 0, 0}, 0, {0, 127, 0}, 0};*/
                CalculateNormalDir(&mRsp->lookat[0], mRsp->current_lookat_coeffs[0]);
                CalculateNormalDir(&mRsp->lookat[1], mRsp->current_lookat_coeffs[1]);
                mRsp->lights_changed = false;
            }

            int r = mRsp->current_lights[mRsp->current_num_lights - 1].l.col[0];
            int g = mRsp->current_lights[mRsp->current_num_lights - 1].l.col[1];
            int b = mRsp->current_lights[mRsp->current_num_lights - 1].l.col[2];

            for (int i = 0; i < mRsp->current_num_lights - 1; i++) {
                float intensity = 0;
                if ((mRsp->geometry_mode & G_LIGHTING_POSITIONAL) && (mRsp->current_lights[i].p.unk3 != 0)) {
                    // Calculate distance from the light to the vertex
                    float dist_vec[3] = { mRsp->current_lights[i].p.pos[0] - world_pos[0],
                                          mRsp->current_lights[i].p.pos[1] - world_pos[1],
                                          mRsp->current_lights[i].p.pos[2] - world_pos[2] };
                    float dist_sq =
                        dist_vec[0] * dist_vec[0] + dist_vec[1] * dist_vec[1] +
                        dist_vec[2] * dist_vec[2] * 2; // The *2 comes from GLideN64, unsure of why it does it
                    float dist = sqrt(dist_sq);

                    // Transform distance vector (which acts as a direction light vector) into model's space
                    float light_model[3];
                    TransposedMatrixMul(light_model, dist_vec,
                                        mRsp->modelview_matrix_stack[mRsp->modelview_matrix_stack_size - 1]);

                    // Calculate intensity for each axis using standard formula for intensity
                    float light_intensity[3];
                    for (int light_i = 0; light_i < 3; light_i++) {
                        light_intensity[light_i] = 4.0f * light_model[light_i] / dist_sq;
                        light_intensity[light_i] = std::clamp(light_intensity[light_i], -1.0f, 1.0f);
                    }

                    // Adjust intensity based on surface normal and sum up total
                    float total_intensity =
                        light_intensity[0] * vn->n[0] + light_intensity[1] * vn->n[1] + light_intensity[2] * vn->n[2];
                    total_intensity = std::clamp(total_intensity, -1.0f, 1.0f);

                    // Attenuate intensity based on attenuation values.
                    // Example formula found at https://ogldev.org/www/tutorial20/tutorial20.html
                    // Specific coefficients for MM's microcode sourced from GLideN64
                    // https://github.com/gonetz/GLideN64/blob/3b43a13a80dfc2eb6357673440b335e54eaa3896/src/gSP.cpp#L636
                    float distf = floorf(dist);
                    float attenuation = (distf * mRsp->current_lights[i].p.unk7 * 2.0f +
                                         distf * distf * mRsp->current_lights[i].p.unkE / 8.0f) /
                                            (float)0xFFFF +
                                        1.0f;
                    intensity = total_intensity / attenuation;
                } else {
                    intensity += vn->n[0] * mRsp->current_lights_coeffs[i][0];
                    intensity += vn->n[1] * mRsp->current_lights_coeffs[i][1];
                    intensity += vn->n[2] * mRsp->current_lights_coeffs[i][2];
                    intensity /= 127.0f;
                }
                if (intensity > 0.0f) {
                    r += intensity * mRsp->current_lights[i].l.col[0];
                    g += intensity * mRsp->current_lights[i].l.col[1];
                    b += intensity * mRsp->current_lights[i].l.col[2];
                }
            }

            d->color.r = r > 255 ? 255 : r;
            d->color.g = g > 255 ? 255 : g;
            d->color.b = b > 255 ? 255 : b;

            if (mRsp->geometry_mode & G_TEXTURE_GEN) {
                float dotx = 0, doty = 0;
                dotx += vn->n[0] * mRsp->current_lookat_coeffs[0][0];
                dotx += vn->n[1] * mRsp->current_lookat_coeffs[0][1];
                dotx += vn->n[2] * mRsp->current_lookat_coeffs[0][2];
                doty += vn->n[0] * mRsp->current_lookat_coeffs[1][0];
                doty += vn->n[1] * mRsp->current_lookat_coeffs[1][1];
                doty += vn->n[2] * mRsp->current_lookat_coeffs[1][2];

                dotx /= 127.0f;
                doty /= 127.0f;

                dotx = Ship::Math::clamp(dotx, -1.0f, 1.0f);
                doty = Ship::Math::clamp(doty, -1.0f, 1.0f);

                if (mRsp->geometry_mode & G_TEXTURE_GEN_LINEAR) {
                    // Not sure exactly what formula we should use to get accurate values
                    /*dotx = (2.906921f * dotx * dotx + 1.36114f) * dotx;
                    doty = (2.906921f * doty * doty + 1.36114f) * doty;
                    dotx = (dotx + 1.0f) / 4.0f;
                    doty = (doty + 1.0f) / 4.0f;*/
                    dotx = acosf(-dotx) /* M_PI */ * 0.159155f;
                    doty = acosf(-doty) /* M_PI */ * 0.159155f;
                } else {
                    dotx = (dotx + 1.0f) / 4.0f;
                    doty = (doty + 1.0f) / 4.0f;
                }

                U = (int32_t)(dotx * mRsp->texture_scaling_factor.s);
                V = (int32_t)(doty * mRsp->texture_scaling_factor.t);
            }
        } else {
            d->color.r = v->cn[0];
            d->color.g = v->cn[1];
            d->color.b = v->cn[2];
        }

        d->u = U;
        d->v = V;

        // trivial clip rejection
        d->clip_rej = 0;
        if (x < -w) {
            d->clip_rej |= 1; // CLIP_LEFT
        }
        if (x > w) {
            d->clip_rej |= 2; // CLIP_RIGHT
        }
        if (y < -w) {
            d->clip_rej |= 4; // CLIP_BOTTOM
        }
        if (y > w) {
            d->clip_rej |= 8; // CLIP_TOP
        }
        // if (z < -w) d->clip_rej |= 16; // CLIP_NEAR
        if (z > w) {
            d->clip_rej |= 32; // CLIP_FAR
        }

        d->x = x;
        d->y = y;
        d->z = z;
        d->w = w;

        if (mRsp->geometry_mode & G_FOG) {
            if (fabsf(w) < 0.001f) {
                // To avoid division by zero
                w = 0.001f;
            }

            float winv = 1.0f / w;
            if (winv < 0.0f) {
                winv = std::numeric_limits<int16_t>::max();
            }

            float fog_z = z * winv * mRsp->fog_mul + mRsp->fog_offset;
            fog_z = Ship::Math::clamp(fog_z, 0.0f, 255.0f);
            d->color.a = fog_z; // Use alpha variable to store fog factor
        } else {
            d->color.a = v->cn[3];
        }
    }
}

void Interpreter::GfxSpModifyVertex(uint16_t vtx_idx, uint8_t where, uint32_t val) {
    SUPPORT_CHECK(where == G_MWO_POINT_ST);

    int16_t s = (int16_t)(val >> 16);
    int16_t t = (int16_t)val;

    LoadedVertex* v = &mRsp->loaded_vertices[vtx_idx];
    v->u = s;
    v->v = t;
}

void Interpreter::GfxSpTri1(uint8_t vtx1_idx, uint8_t vtx2_idx, uint8_t vtx3_idx, bool is_rect) {
    struct LoadedVertex* v1 = &mRsp->loaded_vertices[vtx1_idx];
    struct LoadedVertex* v2 = &mRsp->loaded_vertices[vtx2_idx];
    struct LoadedVertex* v3 = &mRsp->loaded_vertices[vtx3_idx];
    struct LoadedVertex* v_arr[3] = { v1, v2, v3 };

    // if (rand()%2) return;

    if (v1->clip_rej & v2->clip_rej & v3->clip_rej) {
        // The whole triangle lies outside the visible area
        return;
    }

    const uint32_t cull_both = get_attr(CULL_BOTH);
    const uint32_t cull_front = get_attr(CULL_FRONT);
    const uint32_t cull_back = get_attr(CULL_BACK);

    if ((mRsp->geometry_mode & cull_both) != 0) {
        float dx1 = v1->x / (v1->w) - v2->x / (v2->w);
        float dy1 = v1->y / (v1->w) - v2->y / (v2->w);
        float dx2 = v3->x / (v3->w) - v2->x / (v2->w);
        float dy2 = v3->y / (v3->w) - v2->y / (v2->w);
        float cross = dx1 * dy2 - dy1 * dx2;

        if ((v1->w < 0) ^ (v2->w < 0) ^ (v3->w < 0)) {
            // If one vertex lies behind the eye, negating cross will give the correct result.
            // If all vertices lie behind the eye, the triangle will be rejected anyway.
            cross = -cross;
        }

        // If inverted culling is requested, negate the cross
        if (ucode_handler_index == UcodeHandlers::ucode_f3dex2 &&
            (mRsp->extra_geometry_mode & G_EX_INVERT_CULLING) == 1) {
            cross = -cross;
        }

        auto cull_type = mRsp->geometry_mode & cull_both;

        if (cull_type == cull_front) {
            if (cross <= 0) {
                return;
            }
        } else if (cull_type == cull_back) {
            if (cross >= 0) {
                return;
            }
        } else if (cull_type == cull_both) {
            // Why is this even an option?
            return;
        }
    }

    bool depth_test = (mRsp->geometry_mode & G_ZBUFFER) == G_ZBUFFER;
    bool depth_mask = (mRdp->other_mode_l & Z_UPD) == Z_UPD;
    // PORT: SSB64's mvOpeningRoom transition Overlay/Outline use the N64
    // "redirect color image to Z buffer" idiom: tris are drawn with G_ZBUFFER
    // set in geometry_mode but no Z_CMP/Z_UPD in render mode, so on real
    // hardware they bypass depth comparison and write into the ZB as if it
    // were a colour buffer. Fast3D ignores the colour-image redirect (so
    // those tris land on the primary FB instead) and also derives
    // depth_test from G_ZBUFFER alone — which Z-rejects them against the
    // stale ZB content from the previous scene draw, making the explosion
    // sprite invisible. When the redirect is active, gate depth_test on
    // Z_CMP from other_mode_l (real-hardware semantics) so the Overlay's
    // white tris reach the framebuffer.
    bool redirect_active = mRdp->color_image_address == mRdp->z_buf_address && mRdp->color_image_address != nullptr;
    if (redirect_active) {
        depth_test = (mRdp->other_mode_l & Z_CMP) == Z_CMP;
        depth_mask = (mRdp->other_mode_l & Z_UPD) == Z_UPD;
    }
    uint8_t depth_test_and_mask = (depth_test ? 1 : 0) | (depth_mask ? 2 : 0);
    if (depth_test_and_mask != mRenderingState.depth_test_and_mask) {
        Flush();
        mRapi->SetDepthTestAndMask(depth_test, depth_mask);
        mRenderingState.depth_test_and_mask = depth_test_and_mask;
    }

    bool zmode_decal = (mRdp->other_mode_l & ZMODE_DEC) == ZMODE_DEC;
    if (zmode_decal != mRenderingState.decal_mode) {
        Flush();
        mRapi->SetZmodeDecal(zmode_decal);
        mRenderingState.decal_mode = zmode_decal;
    }

    if (mRdp->viewport_or_scissor_changed) {
        if (memcmp(&mRdp->viewport, &mRenderingState.viewport, sizeof(mRdp->viewport)) != 0) {
            Flush();
            mRapi->SetViewport(mRdp->viewport.x, mRdp->viewport.y, mRdp->viewport.width, mRdp->viewport.height);
            mRenderingState.viewport = mRdp->viewport;
        }
        if (memcmp(&mRdp->scissor, &mRenderingState.scissor, sizeof(mRdp->scissor)) != 0) {
            Flush();
            mRapi->SetScissor(mRdp->scissor.x, mRdp->scissor.y, mRdp->scissor.width, mRdp->scissor.height);
            mRenderingState.scissor = mRdp->scissor;
        }
        mRdp->viewport_or_scissor_changed = false;
    }

    uint64_t cc_id = mRdp->combine_mode;
    uint64_t cc_options = 0;
    bool use_alpha = ((mRdp->other_mode_l & (3 << 20)) == (G_BL_CLR_MEM << 20) &&
                      (mRdp->other_mode_l & (3 << 16)) == (G_BL_1MA << 16)) ||
                     ((mRdp->other_mode_l & (3 << 22)) == (G_BL_CLR_MEM << 22) &&
                      (mRdp->other_mode_l & (3 << 18)) == (G_BL_1MA << 18));
    uint8_t blend_src = mRdp->other_mode_l >> 30;
    bool use_blend_color = blend_src == G_BL_CLR_BL;
    bool use_fog = blend_src == G_BL_CLR_FOG || use_blend_color;
    bool texture_edge = (mRdp->other_mode_l & CVG_X_ALPHA) == CVG_X_ALPHA;
    bool use_noise = (mRdp->other_mode_l & (3U << G_MDSFT_ALPHACOMPARE)) == G_AC_DITHER;
    bool use_2cyc = (mRdp->other_mode_h & (3U << G_MDSFT_CYCLETYPE)) == G_CYC_2CYCLE;
    bool alpha_threshold = (mRdp->other_mode_l & (3U << G_MDSFT_ALPHACOMPARE)) == G_AC_THRESHOLD;
    bool invisible =
        (mRdp->other_mode_l & (3 << 24)) == (G_BL_0 << 24) && (mRdp->other_mode_l & (3 << 20)) == (G_BL_CLR_MEM << 20);
    bool use_grayscale = mRdp->grayscale;

    if (texture_edge) {
        if (use_alpha) {
            alpha_threshold = true;
            texture_edge = false;
        }
        use_alpha = true;
    }

    if (use_alpha) {
        cc_options |= SHADER_OPT(ALPHA);
    }
    if (use_fog) {
        cc_options |= SHADER_OPT(FOG);
    }
    if (texture_edge) {
        cc_options |= SHADER_OPT(TEXTURE_EDGE);
    }
    if (use_noise) {
        cc_options |= SHADER_OPT(NOISE);
    }
    if (use_2cyc) {
        cc_options |= SHADER_OPT(_2CYC);
    }
    if (alpha_threshold) {
        cc_options |= SHADER_OPT(ALPHA_THRESHOLD);
    }
    if (invisible) {
        cc_options |= SHADER_OPT(INVISIBLE);
    }
    if (use_grayscale) {
        cc_options |= SHADER_OPT(GRAYSCALE);
    }

    if (!mShaderStack.empty()) {
        cc_options |= (mShaderStack.top() << SHADER_ID_SHIFT);
    } else {
        cc_options |= -1 << SHADER_ID_SHIFT;
    }

    if (mRdp->loaded_texture[0].masked) {
        cc_options |= SHADER_OPT(TEXEL0_MASK);
    }
    if (mRdp->loaded_texture[1].masked) {
        cc_options |= SHADER_OPT(TEXEL1_MASK);
    }
    if (mRdp->loaded_texture[0].blended) {
        cc_options |= SHADER_OPT(TEXEL0_BLEND);
    }
    if (mRdp->loaded_texture[1].blended) {
        cc_options |= SHADER_OPT(TEXEL1_BLEND);
    }

    ColorCombinerKey key;
    key.combine_mode = mRdp->combine_mode;
    key.options = cc_options;
    key.shader_id = 0;

    ColorCombiner* comb = LookupOrCreateColorCombiner(key);

    uint32_t tm = 0;
    uint32_t tex_width[2], tex_height[2], tex_width2[2], tex_height2[2];
    uint32_t effective_tile[2];

    for (int i = 0; i < 2; i++) {
        uint32_t tile = mRdp->first_tile_index + i;

        // No LOD support: force both slots to the base mip level.
        if (i == 1 && mRdp->first_tile_index >= 2) {
            tile = mRdp->first_tile_index;
        }
        effective_tile[i] = tile;

        if (comb->usedTextures[i]) {
            if (mRdp->textures_changed[i]) {
                Flush();
                ImportTexture(i, tile, false);
                if (mRdp->loaded_texture[i].masked) {
                    ImportTextureMask(SHADER_FIRST_MASK_TEXTURE + i, tile);
                }
                if (mRdp->loaded_texture[i].blended) {
                    ImportTexture(SHADER_FIRST_REPLACEMENT_TEXTURE + i, tile, true);
                }
                mRdp->textures_changed[i] = false;
            }

            uint8_t cms = mRdp->texture_tile[tile].cms;
            uint8_t cmt = mRdp->texture_tile[tile].cmt;

            uint32_t loaded_line_size = mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].line_size_bytes;
            uint32_t loaded_size = mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].size_bytes;
            uint32_t loaded_full_line =
                mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].full_image_line_size_bytes;
            uint32_t tex_size_bytes;
            uint32_t line_size;
            if ((loaded_line_size != loaded_size || loaded_full_line != loaded_size) && loaded_line_size > 0) {
                line_size = loaded_line_size;
                tex_size_bytes = loaded_size;
            } else {
                line_size = mRdp->texture_tile[tile].line_size_bytes;
                tex_size_bytes = mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].orig_size_bytes;
                // RGBA32: texture_tile stores TMEM-interleaved stride (half of actual DRAM stride).
                if (mRdp->texture_tile[tile].siz == G_IM_SIZ_32b) {
                    line_size *= 2;
                }
            }

            if (line_size == 0) {
                line_size = 1;
            }

            tex_height[i] = tex_size_bytes / line_size;
            switch (mRdp->texture_tile[tile].siz) {
                case G_IM_SIZ_4b:
                    line_size <<= 1;
                    break;
                case G_IM_SIZ_8b:
                    break;
                case G_IM_SIZ_16b:
                    line_size /= G_IM_SIZ_16b_LINE_BYTES;
                    break;
                case G_IM_SIZ_32b:
                    line_size /= 4; // RGBA32: 4 bytes per pixel (line_size is now actual DRAM stride)
                    break;
            }
            tex_width[i] = line_size;

            tex_width2[i] = (uint32_t)(int32_t)((mRdp->texture_tile[tile].lrs - mRdp->texture_tile[tile].uls + 4) / 4);
            tex_height2[i] = (uint32_t)(int32_t)((mRdp->texture_tile[tile].lrt - mRdp->texture_tile[tile].ult + 4) / 4);

            // Clamp to tile bounds (mipmap loads include all levels).
            if (tex_width2[i] > 0 && tex_width2[i] < tex_width[i]) {
                tex_width[i] = tex_width2[i];
            }
            if (tex_height2[i] > 0 && tex_height2[i] < tex_height[i]) {
                tex_height[i] = tex_height2[i];
            }

            // Keep UV normalization in lockstep with the importers that shrink padded TMEM rows to mask bounds.
            bool uploadClampsToMask =
                ((mRdp->texture_tile[tile].fmt == G_IM_FMT_RGBA || mRdp->texture_tile[tile].fmt == G_IM_FMT_CI) &&
                 mRdp->texture_tile[tile].siz == G_IM_SIZ_16b) ||
                (mRdp->texture_tile[tile].fmt == G_IM_FMT_CI &&
                 (mRdp->texture_tile[tile].siz == G_IM_SIZ_4b || mRdp->texture_tile[tile].siz == G_IM_SIZ_8b));
            if (uploadClampsToMask && mRdp->texture_tile[tile].masks > 0) {
                uint32_t maskWidth = 1u << mRdp->texture_tile[tile].masks;
                if (maskWidth > 0 && maskWidth < tex_width[i]) {
                    tex_width[i] = maskWidth;
                }
            }
            if (uploadClampsToMask && mRdp->texture_tile[tile].maskt > 0) {
                uint32_t maskHeight = 1u << mRdp->texture_tile[tile].maskt;
                if (maskHeight > 0 && maskHeight < tex_height[i]) {
                    tex_height[i] = maskHeight;
                }
            }

            uint32_t tex_width1 = tex_width[i] << (cms & G_TX_MIRROR);
            uint32_t tex_height1 = tex_height[i] << (cmt & G_TX_MIRROR);

            if ((cms & G_TX_CLAMP) && ((cms & G_TX_MIRROR) || tex_width1 != tex_width2[i])) {
                tm |= 1 << 2 * i;
                cms &= ~G_TX_CLAMP;
            }
            if ((cmt & G_TX_CLAMP) && ((cmt & G_TX_MIRROR) || tex_height1 != tex_height2[i])) {
                tm |= 1 << 2 * i + 1;
                cmt &= ~G_TX_CLAMP;
            }

            if (mRenderingState.mTextures[i] == nullptr) {
                continue;
            }

            bool linear_filter = (mRdp->other_mode_h & (3U << G_MDSFT_TEXTFILT)) != G_TF_POINT;
            if (linear_filter != mRenderingState.mTextures[i]->second.linear_filter ||
                cms != mRenderingState.mTextures[i]->second.cms || cmt != mRenderingState.mTextures[i]->second.cmt) {
                Flush();

                // Set the same sampler params on the blended texture. Needed for opengl.
                if (mRdp->loaded_texture[i].blended) {
                    mRapi->SetSamplerParameters(SHADER_FIRST_REPLACEMENT_TEXTURE + i, linear_filter, cms, cmt);
                }

                mRapi->SetSamplerParameters(i, linear_filter, cms, cmt);
                mRenderingState.mTextures[i]->second.linear_filter = linear_filter;
                mRenderingState.mTextures[i]->second.cms = cms;
                mRenderingState.mTextures[i]->second.cmt = cmt;
            }
        }
    }

    struct ShaderProgram* prg = comb->prg[tm];
    if (prg == NULL) {
        comb->prg[tm] = prg =
            LookupOrCreateShaderProgram(comb->shader_id0, comb->shader_id1 | tm * SHADER_OPT(TEXEL0_CLAMP_S));
    }
    if (prg == NULL) {
        return; // Shader compile failed — skip this draw call
    }
    if (prg != mRenderingState.mShaderProgram) {
        Flush();
        mRapi->UnloadShader(mRenderingState.mShaderProgram);
        mRapi->LoadShader(prg);
        mRenderingState.mShaderProgram = prg;
    }
    if (use_alpha != mRenderingState.alpha_blend) {
        Flush();
        mRapi->SetUseAlpha(use_alpha);
        mRenderingState.alpha_blend = use_alpha;
    }
    uint8_t numInputs;
    bool usedTextures[2];

    mRapi->ShaderGetInfo(prg, &numInputs, usedTextures);

    struct GfxClipParameters clip_parameters = mRapi->GetClipParameters();

    // PORT: G_ZS_PRIM source overrides per-vertex Z with the constant Z set by
    // gDPSetPrimDepth. SSB64 (and many other N64 titles) use this to place 2D
    // sprites at a specific Z-buffer depth so subsequent 3D geometry layers
    // correctly. We map the 16-bit N64 Z linearly to clip-space Z; combined with
    // the per-API z_is_from_0_to_1 flag this lands the sprite at the intended
    // depth on both Metal/D3D ([0,1]) and OpenGL ([-1,1]). dz is ignored — the
    // depth-slope is irrelevant once we're producing post-projection Z directly.
    bool use_prim_depth = (mRdp->other_mode_l & G_ZS_PRIM) == G_ZS_PRIM;
    float prim_depth_ndc = (float)mRdp->prim_depth_z / 65535.0f; // 0..1 (Metal-style)
    if (!clip_parameters.z_is_from_0_to_1) {
        prim_depth_ndc = prim_depth_ndc * 2.0f - 1.0f; // OpenGL [-1,1]
    }

    for (int t = 0; t < 2; t++) {
        if (!usedTextures[t] || tex_width[t] == 0 || tex_height[t] == 0) {
            continue;
        }

        uint32_t uv_tile = effective_tile[t];
        uint32_t tmemIndex = mRdp->texture_tile[uv_tile].tmem_index;
        const RawTexMetadata* metadata = &mRdp->loaded_texture[tmemIndex].raw_tex_metadata;
        const void* addr = mRdp->loaded_texture[tmemIndex].addr;
        float rawUMin = 0.0f;
        float rawUMax = 0.0f;
        float rawVMin = 0.0f;
        float rawVMax = 0.0f;
        float normUMin = 0.0f;
        float normUMax = 0.0f;
        float normVMin = 0.0f;
        float normVMax = 0.0f;

        for (int vi = 0; vi < 3; vi++) {
            float rawU = v_arr[vi]->u / 32.0f;
            float rawV = v_arr[vi]->v / 32.0f;
            float u = rawU;
            float v = rawV;

            int shifts = mRdp->texture_tile[uv_tile].shifts;
            int shiftt = mRdp->texture_tile[uv_tile].shiftt;
            if (shifts != 0) {
                if (shifts <= 10) {
                    u /= 1 << shifts;
                } else {
                    u *= 1 << (16 - shifts);
                }
            }
            if (shiftt != 0) {
                if (shiftt <= 10) {
                    v /= 1 << shiftt;
                } else {
                    v *= 1 << (16 - shiftt);
                }
            }

            u -= mRdp->texture_tile[uv_tile].uls / 4.0f;
            v -= mRdp->texture_tile[uv_tile].ult / 4.0f;

            if ((mRdp->other_mode_h & (3U << G_MDSFT_TEXTFILT)) != G_TF_POINT && !is_rect) {
                u += 0.5f;
                v += 0.5f;
            }

            float normU = u / tex_width[t];
            float normV = v / tex_height[t];

            if (vi == 0) {
                rawUMin = rawUMax = rawU;
                rawVMin = rawVMax = rawV;
                normUMin = normUMax = normU;
                normVMin = normVMax = normV;
            } else {
                rawUMin = rawU < rawUMin ? rawU : rawUMin;
                rawUMax = rawU > rawUMax ? rawU : rawUMax;
                rawVMin = rawV < rawVMin ? rawV : rawVMin;
                rawVMax = rawV > rawVMax ? rawV : rawVMax;
                normUMin = normU < normUMin ? normU : normUMin;
                normUMax = normU > normUMax ? normU : normUMax;
                normVMin = normV < normVMin ? normV : normVMin;
                normVMax = normV > normVMax ? normV : normVMax;
            }
        }

        Ssb64RenderDiagLogDraw(metadata, addr, t, uv_tile, tmemIndex, mRdp->texture_tile[uv_tile].fmt,
                               mRdp->texture_tile[uv_tile].siz, mRdp->texture_tile[uv_tile].cms,
                               mRdp->texture_tile[uv_tile].cmt, mRdp->texture_tile[uv_tile].masks,
                               mRdp->texture_tile[uv_tile].maskt, mRdp->texture_tile[uv_tile].shifts,
                               mRdp->texture_tile[uv_tile].shiftt, tex_width[t], tex_height[t], tex_width2[t],
                               tex_height2[t], rawUMin, rawUMax, rawVMin, rawVMax, normUMin, normUMax, normVMin,
                               normVMax, v_arr[0]->x, v_arr[0]->y, v_arr[0]->z, v_arr[0]->w, v_arr[1]->x,
                               v_arr[1]->y, v_arr[1]->z, v_arr[1]->w, v_arr[2]->x, v_arr[2]->y, v_arr[2]->z,
                               v_arr[2]->w);
    }

    for (int i = 0; i < 3; i++) {
        float z = v_arr[i]->z, w = v_arr[i]->w;
        if (use_prim_depth) {
            // Multiply by w so the Metal/GL perspective divide produces prim_depth_ndc.
            z = prim_depth_ndc * w;
        } else if (clip_parameters.z_is_from_0_to_1) {
            z = (z + w) / 2.0f;
        }

        mBufVbo[mBufVboLen++] = v_arr[i]->x;
        mBufVbo[mBufVboLen++] = clip_parameters.invertY ? -v_arr[i]->y : v_arr[i]->y;
        mBufVbo[mBufVboLen++] = z;
        mBufVbo[mBufVboLen++] = w;

        for (int t = 0; t < 2; t++) {
            if (!usedTextures[t]) {
                continue;
            }
            float u = v_arr[i]->u / 32.0f;
            float v = v_arr[i]->v / 32.0f;

            uint32_t uv_tile = effective_tile[t];
            int shifts = mRdp->texture_tile[uv_tile].shifts;
            int shiftt = mRdp->texture_tile[uv_tile].shiftt;
            if (shifts != 0) {
                if (shifts <= 10) {
                    u /= 1 << shifts;
                } else {
                    u *= 1 << (16 - shifts);
                }
            }
            if (shiftt != 0) {
                if (shiftt <= 10) {
                    v /= 1 << shiftt;
                } else {
                    v *= 1 << (16 - shiftt);
                }
            }

            u -= mRdp->texture_tile[uv_tile].uls / 4.0f;
            v -= mRdp->texture_tile[uv_tile].ult / 4.0f;

            if ((mRdp->other_mode_h & (3U << G_MDSFT_TEXTFILT)) != G_TF_POINT) {
                // Linear filter adds 0.5f to the coordinates
                if (!is_rect) {
                    u += 0.5f;
                    v += 0.5f;
                }
            }

            // Apply the FB-mirror UV transform (identity unless the bound
            // texture is a registered sub-rect of a snapshot FB). This remaps
            // a tile's local UV (0..1) into the FB sub-rect [u0,u1] x [v0,v1]
            // so a multi-tile sprite samples its slice of the captured frame
            // instead of the whole thing.
            const FbUvTransform& xf = mFbUvTransform[t];
            float normU = u / tex_width[t];
            float normV = v / tex_height[t];
            mBufVbo[mBufVboLen++] = normU * xf.scaleU + xf.offsetU;
            mBufVbo[mBufVboLen++] = normV * xf.scaleV + xf.offsetV;

            bool clampS = tm & (1 << 2 * t);
            bool clampT = tm & (1 << 2 * t + 1);

            if (clampS) {
                float clampNormU = (tex_width2[t] - 0.5f) / tex_width[t];
                mBufVbo[mBufVboLen++] = clampNormU * xf.scaleU + xf.offsetU;
            }

            if (clampT) {
                float clampNormV = (tex_height2[t] - 0.5f) / tex_height[t];
                mBufVbo[mBufVboLen++] = clampNormV * xf.scaleV + xf.offsetV;
            }
        }

        if (use_fog) {
            if (use_blend_color) {
                // Shroud/blend mode: blend toward blend_color using fog alpha as factor
                mBufVbo[mBufVboLen++] = mRdp->blend_color.r / 255.0f;
                mBufVbo[mBufVboLen++] = mRdp->blend_color.g / 255.0f;
                mBufVbo[mBufVboLen++] = mRdp->blend_color.b / 255.0f;
                mBufVbo[mBufVboLen++] = mRdp->fog_color.a / 255.0f;
            } else {
                mBufVbo[mBufVboLen++] = mRdp->fog_color.r / 255.0f;
                mBufVbo[mBufVboLen++] = mRdp->fog_color.g / 255.0f;
                mBufVbo[mBufVboLen++] = mRdp->fog_color.b / 255.0f;
                // Cycle-1 alpha-source mux (other_mode_l bits 26..27): G_BL_A_FOG (1) means
                // the blender's alpha comes from the fog color register itself; G_BL_A_SHADE
                // (2, default for OoT-style depth fog) means per-vertex shade alpha (which the
                // RSP fills with the depth-based fog factor when G_FOG geomode is on).
                // Without this branch, G_RM_FOG_PRIM_A degenerates to vertex alpha and the
                // game's intended SetFogColor.a tint is lost (e.g. SSB64 respawn-platform
                // white flash, fighter hit/F-Smash colour flashes).
                uint8_t fog_alpha_src = (mRdp->other_mode_l >> 26) & 3;
                float fog_a = (fog_alpha_src == G_BL_A_FOG) ? mRdp->fog_color.a / 255.0f
                                                            : v_arr[i]->color.a / 255.0f;
                mBufVbo[mBufVboLen++] = fog_a;
            }
        }

        if (use_grayscale) {
            mBufVbo[mBufVboLen++] = mRdp->grayscale_color.r / 255.0f;
            mBufVbo[mBufVboLen++] = mRdp->grayscale_color.g / 255.0f;
            mBufVbo[mBufVboLen++] = mRdp->grayscale_color.b / 255.0f;
            mBufVbo[mBufVboLen++] = mRdp->grayscale_color.a / 255.0f; // lerp interpolation factor (not alpha)
        }

        for (int j = 0; j < numInputs; j++) {
            RGBA* color;
            RGBA tmp;
            for (int k = 0; k < 1 + (use_alpha ? 1 : 0); k++) {
                switch (comb->shader_input_mapping[k][j]) {
                        // Note: CCMUX constants and ACMUX constants used here have same value, which is why this works
                        // (except LOD fraction).
                    case G_CCMUX_PRIMITIVE:
                        color = &mRdp->prim_color;
                        break;
                    case G_CCMUX_SHADE:
                        color = &v_arr[i]->color;
                        break;
                    case G_CCMUX_ENVIRONMENT:
                        color = &mRdp->env_color;
                        break;
                    case G_CCMUX_PRIMITIVE_ALPHA: {
                        tmp.r = tmp.g = tmp.b = mRdp->prim_color.a;
                        color = &tmp;
                        break;
                    }
                    case G_CCMUX_ENV_ALPHA: {
                        tmp.r = tmp.g = tmp.b = mRdp->env_color.a;
                        color = &tmp;
                        break;
                    }
                    case G_CCMUX_PRIM_LOD_FRAC: {
                        tmp.r = tmp.g = tmp.b = mRdp->prim_lod_fraction;
                        color = &tmp;
                        break;
                    }
                    case G_CCMUX_LOD_FRACTION: {
                        if (mRdp->other_mode_l & G_TL_LOD) {
                            // "Hack" that works for Bowser - Peach painting
                            float distance_frac = (v1->w - 3000.0f) / 3000.0f;
                            if (distance_frac < 0.0f) {
                                distance_frac = 0.0f;
                            }
                            if (distance_frac > 1.0f) {
                                distance_frac = 1.0f;
                            }
                            tmp.r = tmp.g = tmp.b = tmp.a = distance_frac * 255.0f;
                        } else {
                            tmp.r = tmp.g = tmp.b = tmp.a = 255.0f;
                        }
                        color = &tmp;
                        break;
                    }
                    case G_ACMUX_PRIM_LOD_FRAC:
                        tmp.a = mRdp->prim_lod_fraction;
                        color = &tmp;
                        break;
                    default:
                        memset(&tmp, 0, sizeof(tmp));
                        color = &tmp;
                        break;
                }
                if (k == 0) {
                    mBufVbo[mBufVboLen++] = color->r / 255.0f;
                    mBufVbo[mBufVboLen++] = color->g / 255.0f;
                    mBufVbo[mBufVboLen++] = color->b / 255.0f;
                } else {
                    if (use_fog && !use_blend_color && color == &v_arr[i]->color) {
                        // Shade alpha is 100% for standard fog, blend color mode preserves
                        // it since fog alpha is the blend factor
                        mBufVbo[mBufVboLen++] = 1.0f;
                    } else {
                        mBufVbo[mBufVboLen++] = color->a / 255.0f;
                    }
                }
            }
        }

        // struct RGBA *color = &v_arr[i]->color;
        // mBufVbo[mBufVboLen++] = color->r / 255.0f;
        // mBufVbo[mBufVboLen++] = color->g / 255.0f;
        // mBufVbo[mBufVboLen++] = color->b / 255.0f;
        // mBufVbo[mBufVboLen++] = color->a / 255.0f;
    }

    if (++mBufVboNumTris == MAX_TRI_BUFFER) {
        // if (++mBufVbo_num_tris == 1) {
        Flush();
    }
}

void Interpreter::GfxSpGeometryMode(uint32_t clear, uint32_t set) {
    mRsp->geometry_mode &= ~clear;
    mRsp->geometry_mode |= set;
}

void Interpreter::GfxSpExtraGeometryMode(uint32_t clear, uint32_t set) {
    mRsp->extra_geometry_mode &= ~clear;
    mRsp->extra_geometry_mode |= set;
}

void Interpreter::AdjustVIewportOrScissor(XYWidthHeight* area) {
    if (!mFbActive) {
        // Adjust the y origin based on the y-inversion for the active framebuffer
        GfxClipParameters clipParameters = mRapi->GetClipParameters();
        if (clipParameters.invertY) {
            area->y -= area->height;
        } else {
            area->y = mNativeDimensions.height - area->y;
        }

        area->width *= RATIO_X(mActiveFrameBuffer, mCurDimensions);
        area->height *= RATIO_Y(mActiveFrameBuffer, mCurDimensions);
        area->x *= RATIO_X(mActiveFrameBuffer, mCurDimensions);
        area->y *= RATIO_Y(mActiveFrameBuffer, mCurDimensions);

        if (!mRendersToFb || (mMsaaLevel > 1 && mCurDimensions.width == mGameWindowViewport.width &&
                              mCurDimensions.height == mGameWindowViewport.height)) {
            area->x += mGameWindowViewport.x;
            area->y += mGfxCurrentWindowDimensions.height - (mGameWindowViewport.y + mGameWindowViewport.height);
        }
    } else {
        area->y = mActiveFrameBuffer->second.orig_height - area->y;

        if (mActiveFrameBuffer->second.resize) {
            area->width *= RATIO_X(mActiveFrameBuffer, mCurDimensions);
            area->height *= RATIO_Y(mActiveFrameBuffer, mCurDimensions);
            area->x *= RATIO_X(mActiveFrameBuffer, mCurDimensions);
            area->y *= RATIO_Y(mActiveFrameBuffer, mCurDimensions);
        }
    }
}

void Interpreter::CalcAndSetViewport(const F3DVp_t* viewport) {
    // 2 bits fraction
    float width = 2.0f * viewport->vscale[0] / 4.0f;
    float height = 2.0f * viewport->vscale[1] / 4.0f;
    float x = (viewport->vtrans[0] / 4.0f) - width / 2.0f;
    float y = ((viewport->vtrans[1] / 4.0f) + height / 2.0f);

    mRdp->viewport.x = x;
    mRdp->viewport.y = y;
    mRdp->viewport.width = width;
    mRdp->viewport.height = height;

    AdjustVIewportOrScissor(&mRdp->viewport);

    mRdp->viewport_or_scissor_changed = true;
}

void Interpreter::GfxSpMovememF3dex2(uint8_t index, uint8_t offset, const void* data) {
    switch (index) {
        case F3DEX2_G_MV_VIEWPORT:
            CalcAndSetViewport((const F3DVp_t*)data);
            break;
        case F3DEX2_G_MV_LIGHT: {
            int lightidx = offset / 24 - 2;
            if (lightidx >= 0 && lightidx <= MAX_LIGHTS) { // skip lookat
                // NOTE: reads out of bounds if it is an ambient light
                memcpy(mRsp->current_lights + lightidx, data, sizeof(F3DLight));
            } else if (lightidx < 0) {
                memcpy(mRsp->lookat + offset / 24, data, sizeof(F3DLight_t)); // TODO Light?
            }
            break;
        }
    }
}

void Interpreter::GfxSpMovememF3d(uint8_t index, uint8_t offset, const void* data) {
    switch (index) {
        case F3DEX_G_MV_VIEWPORT:
            CalcAndSetViewport((const F3DVp_t*)data);
            break;
        case F3DEX_G_MV_LOOKATY:
        case F3DEX_G_MV_LOOKATX:
            memcpy(mRsp->lookat + (index - F3DEX_G_MV_LOOKATY) / 2, data, sizeof(F3DLight_t));
            break;
        case F3DEX_G_MV_L0:
        case F3DEX_G_MV_L1:
        case F3DEX_G_MV_L2:
        case F3DEX_G_MV_L3:
        case F3DEX_G_MV_L4:
        case F3DEX_G_MV_L5:
        case F3DEX_G_MV_L6:
        case F3DEX_G_MV_L7:
            // NOTE: reads out of bounds if it is an ambient light
            memcpy(mRsp->current_lights + (index - F3DEX_G_MV_L0) / 2, data, sizeof(F3DLight_t));
            break;
    }
}

void Interpreter::GfxSpMovewordF3dex2(uint8_t index, uint16_t offset, uintptr_t data) {
    switch (index) {
        case G_MW_NUMLIGHT:
            mRsp->current_num_lights = data / 24 + 1; // add ambient light
            mRsp->lights_changed = true;
            break;
        case G_MW_LIGHTCOL: {
            // gSPLightColor(pkt, n, col) expands to two gMoveWd(G_MW_LIGHTCOL)
            // commands: one at offset aLIGHT_n (populates light.l.col) and one
            // at offset bLIGHT_n (populates light.l.colc).  For F3DEX2 the
            // light slots are 24 bytes apart (aLIGHT_1=0, bLIGHT_1=4,
            // aLIGHT_2=0x18, bLIGHT_2=0x1C, aLIGHT_3=0x30, ...).
            //
            // Without this case the command was a no-op, so per-material
            // light color overrides (e.g. SSB64's fighters) never reached the
            // RSP and every lit material rendered with whatever stale light
            // the RSP last held — typically the scene-default white from
            // ftDisplayLightsDrawReflect.
            //
            // `data` carries an N64-format packcol where R/G/B occupy the top
            // three bytes: (r << 24) | (g << 16) | (b << 8) | a.  This is the
            // same byte layout the F3DLight_t struct uses in DMEM, so we
            // write directly into col/colc.
            const int light_idx = offset / 24;   // 0-based light slot
            const bool is_b = (offset & 4) != 0; // bLIGHT (colc) vs aLIGHT (col)
            if (light_idx >= 0 && light_idx <= MAX_LIGHTS) {
                F3DLight& light = mRsp->current_lights[light_idx];
                uint8_t r = (uint8_t)(data >> 24);
                uint8_t g = (uint8_t)(data >> 16);
                uint8_t b = (uint8_t)(data >> 8);
                if (is_b) {
                    light.l.colc[0] = r;
                    light.l.colc[1] = g;
                    light.l.colc[2] = b;
                } else {
                    light.l.col[0] = r;
                    light.l.col[1] = g;
                    light.l.col[2] = b;
                }
                mRsp->lights_changed = true;
            }
        } break;
        case G_MW_FOG:
            mRsp->fog_mul = (int16_t)(data >> 16);
            mRsp->fog_offset = (int16_t)data;
            break;
        case G_MW_SEGMENT: {
            int segNumber = offset / 4;
            uintptr_t _old = mSegmentPointers[segNumber];
            mSegmentPointers[segNumber] = data;
            diagRecordSegWrite(segNumber, _old, data, "MovewordF3dex2/G_MW_SEGMENT", 0);
        } break;
        case G_MW_SEGMENT_INTERP: {
            int segNumber = offset % 16;
            int segIndex = offset / 16;

            if (segIndex == mInterpolationIndex) {
                uintptr_t _old = mSegmentPointers[segNumber];
                mSegmentPointers[segNumber] = data;
                diagRecordSegWrite(segNumber, _old, data, "MovewordF3dex2/G_MW_SEGMENT_INTERP", 0);
            }
        } break;
        case G_MW_MATRIX: {
            // SSB64 (and other titles using draw types that build custom MVPs)
            // emits gSPMvpRecalc + a sequence of gMoveWd(G_MW_MATRIX, ...) to
            // patch individual integer/fractional halves of the RSP's MVP
            // matrix in flight.  On N64 these write directly into the matrix
            // DMEM slots; the next vertex transform reads the patched MVP.
            //
            // Fast3D keeps MP_matrix recomputed eagerly in GfxSpMatrix, so we
            // emulate the patches by decomposing the relevant float entries
            // back into the s15.16 fixed-point pair, overwriting just the
            // half the gMoveWd targets, and recomposing.  The next G_MTX call
            // will overwrite MP_matrix from M*P again, wiping the patches —
            // same as the N64 hardware.
            //
            // Layout (mirrors GfxSpMatrix decoder above):
            //   bytes 0x00..0x1F = integer halves, 8 u32 entries
            //   bytes 0x20..0x3F = fractional halves, 8 u32 entries
            //   each u32 packs two adjacent matrix elements in the same row:
            //     high16 = m[row][col_a] half, low16 = m[row][col_b] half
            //   slot index = (offset & 0x1F) / 4
            //   row = slot / 2, col_a = (slot % 2) * 2, col_b = col_a + 1
            const bool is_frac = (offset & 0x20) != 0;
            const int slot = (offset & 0x1F) >> 2; // 0..7
            const int row = slot >> 1;
            const int col_a = (slot & 1) << 1;
            const int col_b = col_a + 1;
            const uint32_t v = (uint32_t)data;

            auto encode = [](float f) -> int32_t {
                return (int32_t)lrintf(f * 65536.0f);
            };
            auto decode = [](int32_t fx) -> float {
                return (float)fx * (1.0f / 65536.0f);
            };

            int32_t fx_a = encode(mRsp->MP_matrix[row][col_a]);
            int32_t fx_b = encode(mRsp->MP_matrix[row][col_b]);

            const uint16_t new_a = (uint16_t)(v >> 16);
            const uint16_t new_b = (uint16_t)(v & 0xFFFF);

            // Replace half of each fixed-point pair without disturbing the
            // other half.  All math is done as uint32_t to avoid signed-shift
            // UB; the final cast back to int32_t reinterprets bit 31 as the
            // sign of the s15.16 value, matching the existing decoder.
            const uint32_t fx_a_u = (uint32_t)fx_a;
            const uint32_t fx_b_u = (uint32_t)fx_b;
            if (is_frac) {
                fx_a = (int32_t)((fx_a_u & 0xFFFF0000u) | new_a);
                fx_b = (int32_t)((fx_b_u & 0xFFFF0000u) | new_b);
            } else {
                fx_a = (int32_t)(((uint32_t)new_a << 16) | (fx_a_u & 0xFFFFu));
                fx_b = (int32_t)(((uint32_t)new_b << 16) | (fx_b_u & 0xFFFFu));
            }

            mRsp->MP_matrix[row][col_a] = decode(fx_a);
            mRsp->MP_matrix[row][col_b] = decode(fx_b);
        } break;
    }
}

void Interpreter::GfxSpMovewordF3d(uint8_t index, uint16_t offset, uintptr_t data) {
    switch (index) {
        case G_MW_NUMLIGHT:
            // Ambient light is included
            // The 31st bit is a flag that lights should be recalculated
            mRsp->current_num_lights = (data - 0x80000000U) / 32;
            mRsp->lights_changed = true;
            break;
        case G_MW_FOG:
            mRsp->fog_mul = (int16_t)(data >> 16);
            mRsp->fog_offset = (int16_t)data;
            break;
        case G_MW_SEGMENT: {
            int segNumber = offset / 4;
            uintptr_t _old = mSegmentPointers[segNumber];
            mSegmentPointers[segNumber] = data;
            diagRecordSegWrite(segNumber, _old, data, "MovewordF3d/G_MW_SEGMENT", 0);
        } break;
        case G_MW_SEGMENT_INTERP: {
            int segNumber = offset % 16;
            int segIndex = offset / 16;

            if (segIndex == mInterpolationIndex) {
                uintptr_t _old = mSegmentPointers[segNumber];
                mSegmentPointers[segNumber] = data;
                diagRecordSegWrite(segNumber, _old, data, "MovewordF3d/G_MW_SEGMENT_INTERP", 0);
            }
        } break;
    }
}

void Interpreter::GfxSpTexture(uint16_t sc, uint16_t tc, uint8_t level, uint8_t tile, uint8_t on) {
    mRsp->texture_scaling_factor.s = sc;
    mRsp->texture_scaling_factor.t = tc;
    if (mRdp->first_tile_index != tile) {
        mRdp->textures_changed[0] = true;
        mRdp->textures_changed[1] = true;
    }

    mRdp->first_tile_index = tile;
}

void Interpreter::GfxDpSetScissor(uint32_t mode, uint32_t ulx, uint32_t uly, uint32_t lrx, uint32_t lry) {
    float x = ulx / 4.0f;
    float y = lry / 4.0f;
    float width = (lrx - ulx) / 4.0f;
    float height = (lry - uly) / 4.0f;

    mRdp->scissor.x = x;
    mRdp->scissor.y = y;
    mRdp->scissor.width = width;
    mRdp->scissor.height = height;

    AdjustVIewportOrScissor(&mRdp->scissor);

    // SSB64 port: when the tight-4:3-scissor hook is active alongside
    // widescreen, narrow the GPU scissor to the centered 4:3 sub-region of
    // the wider FB. Game code flips this for scene-specific effects whose
    // mesh geometry would otherwise show perspective slants beyond the 4:3
    // crop (e.g. OpeningRun impact-flash starburst).
    if (mWidescreenActive && mTight4_3ScissorWindow && !mFbActive) {
        const float win_w = (float)mGameWindowViewport.width;
        const float win_h = (float)mGameWindowViewport.height;
        if (win_w > 0.0f && win_h > 0.0f) {
            const float win_aspect = win_w / win_h;
            const float base_aspect = 4.0f / 3.0f;
            if (win_aspect > base_aspect) {
                const float target_w = mRdp->scissor.height * base_aspect;
                const float margin = (mRdp->scissor.width - target_w) * 0.5f;
                if (margin > 0.0f) {
                    mRdp->scissor.x += margin;
                    mRdp->scissor.width = target_w;
                }
            }
        }
    }

    mRdp->viewport_or_scissor_changed = true;
}

void Interpreter::GfxDpSetTextureImage(uint32_t format, uint32_t size, uint32_t width, const char* texPath,
                                       uint32_t texFlags, RawTexMetadata rawTexMetdata, const void* addr) {
    // fprintf(stderr, "GfxDpSetTextureImage: %s (width=%d; size=0x%X)\n",
    //         rawTexMetdata.resource ? rawTexMetdata.resource->GetInitData()->Path.c_str() : nullptr, width, size);
    mRdp->texture_to_load.addr = (const uint8_t*)addr;
    mRdp->texture_to_load.siz = size;
    mRdp->texture_to_load.width = width;
    mRdp->texture_to_load.tex_flags = texFlags;
    mRdp->texture_to_load.raw_tex_metadata = rawTexMetdata;
}

void Interpreter::GfxDpSetTile(uint8_t fmt, uint32_t siz, uint32_t line, uint32_t tmem, uint8_t tile, uint32_t palette,
                               uint32_t cmt, uint32_t maskt, uint32_t shiftt, uint32_t cms, uint32_t masks,
                               uint32_t shifts) {
    // OTRTODO:
    // SUPPORT_CHECK(tmem == 0 || tmem == 256);

    if (cms == G_TX_WRAP && masks == G_TX_NOMASK) {
        cms = G_TX_CLAMP;
    }
    if (cmt == G_TX_WRAP && maskt == G_TX_NOMASK) {
        cmt = G_TX_CLAMP;
    }

    mRdp->texture_tile[tile].palette = palette; // palette should set upper 4 bits of color index in 4b mode
    mRdp->texture_tile[tile].fmt = fmt;
    mRdp->texture_tile[tile].siz = siz;
    mRdp->texture_tile[tile].cms = cms;
    mRdp->texture_tile[tile].cmt = cmt;
    mRdp->texture_tile[tile].shifts = shifts;
    mRdp->texture_tile[tile].shiftt = shiftt;
    mRdp->texture_tile[tile].masks = (uint8_t)masks;
    mRdp->texture_tile[tile].maskt = (uint8_t)maskt;
    mRdp->texture_tile[tile].line_size_bytes = line * 8;

    mRdp->texture_tile[tile].tmem = tmem;
    // mRdp->texture_tile[tile].tmem_index = tmem / 256; // tmem is the 64-bit word offset, so 256 words means 2 kB

    mRdp->texture_tile[tile].tmem_index =
        tmem != 0; // assume one texture is loaded at address 0 and another texture at any other address

    mRdp->textures_changed[0] = true;
    mRdp->textures_changed[1] = true;
}

void Interpreter::GfxDpSetTileSize(uint8_t tile, uint16_t uls, uint16_t ult, uint16_t lrs, uint16_t lrt) {
    mRdp->texture_tile[tile].uls = uls;
    mRdp->texture_tile[tile].ult = ult;
    mRdp->texture_tile[tile].lrs = lrs;
    mRdp->texture_tile[tile].lrt = lrt;
    mRdp->textures_changed[0] = true;
    mRdp->textures_changed[1] = true;
}

void Interpreter::GfxDpLoadTlut(uint8_t tile, uint32_t high_index) {
    SUPPORT_CHECK(mRdp->texture_to_load.siz == G_IM_SIZ_16b);

    uint16_t tmem = mRdp->texture_tile[tile].tmem;
    const uint8_t* src = mRdp->texture_to_load.addr;
    if (src == nullptr) {
        SPDLOG_ERROR("GfxDpLoadTlut: missing texture image for tile {}", tile);
        return;
    }
    uint32_t entryCount = high_index + 1;
    uint32_t byteCount = entryCount * 2;

    // PORT (issue #4): N64 RDP transfers DMA in 8-byte (qword) chunks, so a
    // LOADTLUT with high_index_plus_one*2 not a multiple of 8 still pulls a
    // full 8-byte qword from DRAM into TMEM. Game DLs lean on this: e.g.
    // Mario's BTT arrow uses count=3 (6 bytes) and samples palette[3] in its
    // image, expecting the 4th palette entry sitting at src+6..7 to also be
    // loaded. libultraship's exact-byteCount memcpy left palette[3] as stale
    // bytes from the previous LOADTLUT, manifesting as the per-character
    // "blue inside the chevron" bug after the count=3 outline was already
    // black. Round byteCount up to 8 bytes to match N64 DMA semantics.
    byteCount = (byteCount + 7u) & ~7u;

    // PORT: lazy palette byte-order fixup.  CI texture palettes are 16-bit
    // RGBA5551 entries that pass1 BSWAP32 has corrupted (each 4-byte word
    // has its bytes reversed, swapping the two pixels in the word AND each
    // pixel's byte order).  Apply BSWAP32 to restore N64 BE order before
    // copying into palette_staging.  Idempotent.
    portRelocFixupTextureAtRuntime(src, byteCount);

    if (tmem >= 256) {
        // N64 TMEM palette area starts at tmem word 256. Each CI4 palette = 16 entries = 16 tmem words.
        uint32_t paletteByteOffset = (tmem - 256) * 2;

        if (high_index == 255 && paletteByteOffset == 0) {
            // CI8: full 256-entry palette spanning both halves (fast path).
            memcpy(mRdp->palette_staging[0], src, 256);
            memcpy(mRdp->palette_staging[1], src + 256, 256);
            mRdp->palettes[0] = mRdp->palette_staging[0];
            mRdp->palettes[1] = mRdp->palette_staging[1];
            mRdp->palette_dram_addr[0] = src;
            mRdp->palette_dram_addr[1] = src + 256;
        } else if (paletteByteOffset < 256) {
            // Load starts in the first palette half (palettes 0-7).
            // Fill staging[0] up to the half boundary.
            uint32_t firstLen = (paletteByteOffset + byteCount <= 256) ? byteCount : (256 - paletteByteOffset);
            memcpy(mRdp->palette_staging[0] + paletteByteOffset, src, firstLen);
            mRdp->palettes[0] = mRdp->palette_staging[0];
            mRdp->palette_dram_addr[0] = src;

            // PORT: if the load crosses the half boundary, spill the remaining
            // bytes into staging[1].  On real N64 hardware a single LOADTLUT
            // copies (high_index + 1) entries starting at the tile's tmem
            // offset, and a load that begins in the lower half but extends
            // past it naturally continues into the upper half.  SSB64 uses
            // this pattern for several CI8 materials whose authors chose a
            // non-standard high_index (e.g. textbooks at hi=254, wall posters
            // at hi=212/226) — without this spill the upper half of the
            // palette is left holding stale data and roughly half of each
            // affected texture samples garbage colors.
            if (paletteByteOffset + byteCount > 256) {
                uint32_t spilloverLen = (paletteByteOffset + byteCount) - 256;
                if (spilloverLen > 256) spilloverLen = 256;
                memcpy(mRdp->palette_staging[1], src + firstLen, spilloverLen);
                mRdp->palettes[1] = mRdp->palette_staging[1];
                mRdp->palette_dram_addr[1] = src + firstLen;
            }
        } else {
            // Load starts in the second palette half (palettes 8-15).
            uint32_t offset = paletteByteOffset - 256;
            uint32_t copyLen = (offset + byteCount <= 256) ? byteCount : (256 - offset);
            memcpy(mRdp->palette_staging[1] + offset, src, copyLen);
            mRdp->palettes[1] = mRdp->palette_staging[1];
            mRdp->palette_dram_addr[1] = src;
        }
    } else {
        // tmem < 256: non-standard location, fall back to direct pointer
        mRdp->palettes[1] = src;
        mRdp->palette_dram_addr[1] = src;
    }

    // CI textures bake palette data into the decoded GPU texture, so a TLUT
    // load must invalidate any previously imported texture using that palette.
    mRdp->textures_changed[0] = true;
    mRdp->textures_changed[1] = true;
}

void Interpreter::GfxDpLoadBlock(uint8_t tile, uint32_t uls, uint32_t ult, uint32_t lrs, uint32_t dxt) {
    SUPPORT_CHECK(uls == 0);
    SUPPORT_CHECK(ult == 0);

    if (mRdp->texture_to_load.addr == nullptr) {
        SPDLOG_ERROR("GfxDpLoadBlock: missing texture image for tile {}", tile);
        return;
    }

    // The lrs field rather seems to be number of pixels to load
    uint32_t word_size_shift = 0;
    switch (mRdp->texture_to_load.siz) {
        case G_IM_SIZ_4b:
            word_size_shift = -1;
            break;
        case G_IM_SIZ_8b:
            word_size_shift = 0;
            break;
        case G_IM_SIZ_16b:
            word_size_shift = 1;
            break;
        case G_IM_SIZ_32b:
            word_size_shift = 2;
            break;
    }
    uint32_t orig_size_bytes =
        word_size_shift > 0 ? (lrs + 1) << word_size_shift : (lrs + 1) >> (-(int64_t)word_size_shift);
    uint32_t size_bytes = orig_size_bytes;
    if (mRdp->texture_to_load.raw_tex_metadata.h_byte_scale != 1 ||
        mRdp->texture_to_load.raw_tex_metadata.v_pixel_scale != 1) {
        size_bytes *= mRdp->texture_to_load.raw_tex_metadata.h_byte_scale;
        size_bytes *= mRdp->texture_to_load.raw_tex_metadata.v_pixel_scale;
    }

    // PORT: lazy texture byte-order fixup.  For data that lives inside a
    // reloc file, the on-disk texture is in N64 BE byte order; pass1 BSWAP32
    // reversed each u32 word.  This call applies BSWAP32 again idempotently
    // the first time a texture is loaded.  Catches runtime-built fighter
    // material loadblocks that pass2 and the chain walk both miss.
    portRelocFixupTextureAtRuntime(mRdp->texture_to_load.addr, orig_size_bytes);

    mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].orig_size_bytes = orig_size_bytes;
    mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].size_bytes = size_bytes;
    // Compute actual per-line DRAM stride from SetTextureImage width when available.
    // The standard gDPLoadTextureBlock macro sets width=1, but manually-built DL
    // commands may set the real pixel width.
    uint32_t actual_line_bytes = size_bytes;
    if (mRdp->texture_to_load.width > 1) {
        uint32_t candidate;
        switch (mRdp->texture_to_load.siz) {
            case G_IM_SIZ_4b:
                candidate = (mRdp->texture_to_load.width + 1) / 2;
                break;
            case G_IM_SIZ_8b:
                candidate = mRdp->texture_to_load.width;
                break;
            case G_IM_SIZ_16b:
                candidate = mRdp->texture_to_load.width * 2;
                break;
            case G_IM_SIZ_32b:
                candidate = mRdp->texture_to_load.width * 4;
                break;
            default:
                candidate = mRdp->texture_to_load.width;
                break;
        }
        if (candidate > 0 && candidate < size_bytes && size_bytes % candidate == 0) {
            actual_line_bytes = candidate;
        }
    }
    mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].line_size_bytes = actual_line_bytes;
    mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].full_image_line_size_bytes = actual_line_bytes;
    // assert(size_bytes <= 4096 && "bug: too big texture");
    mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].tex_flags = mRdp->texture_to_load.tex_flags;
    mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].raw_tex_metadata = mRdp->texture_to_load.raw_tex_metadata;
    mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].addr = mRdp->texture_to_load.addr;
    // fprintf(stderr, "GfxDpLoadBlock: line_size = 0x%x; orig = 0x%x; bpp=%d; lrs=%d\n", size_bytes,
    // orig_size_bytes,
    //         mRdp->texture_to_load.siz, lrs);

    const std::string_view texPath =
        mRdp->texture_to_load.raw_tex_metadata.resource != nullptr
            ? GetBaseTexturePath(mRdp->texture_to_load.raw_tex_metadata.resource->GetInitData()->Path)
            : std::string_view{};
    auto maskedTextureIter = mMaskedTextures.find(texPath);
    if (maskedTextureIter != mMaskedTextures.end()) {
        mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].masked = true;
        mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].blended =
            maskedTextureIter->second.replacementData != nullptr;
    } else {
        mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].masked = false;
        mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].blended = false;
    }

    mRdp->textures_changed[mRdp->texture_tile[tile].tmem_index] = true;
}

void Interpreter::GfxDpLoadTile(uint8_t tile, uint32_t uls, uint32_t ult, uint32_t lrs, uint32_t lrt) {
    SUPPORT_CHECK(tile == G_TX_LOADTILE);

    if (mRdp->texture_to_load.addr == nullptr) {
        SPDLOG_ERROR("GfxDpLoadTile: missing texture image for tile {}", tile);
        return;
    }

    uint32_t word_size_shift = 0;
    switch (mRdp->texture_to_load.siz) {
        case G_IM_SIZ_4b:
            word_size_shift = 0;
            break;
        case G_IM_SIZ_8b:
            word_size_shift = 0;
            break;
        case G_IM_SIZ_16b:
            word_size_shift = 1;
            break;
        case G_IM_SIZ_32b:
            word_size_shift = 2;
            break;
    }

    uint32_t offset_x = uls >> G_TEXTURE_IMAGE_FRAC;
    uint32_t offset_y = ult >> G_TEXTURE_IMAGE_FRAC;
    uint32_t tile_width = ((lrs - uls) >> G_TEXTURE_IMAGE_FRAC) + 1;
    uint32_t tile_height = ((lrt - ult) >> G_TEXTURE_IMAGE_FRAC) + 1;
    uint32_t full_image_width = mRdp->texture_to_load.width;

    uint32_t offset_x_in_bytes = offset_x << word_size_shift;
    uint32_t tile_line_size_bytes = tile_width << word_size_shift;
    uint32_t full_image_line_size_bytes = full_image_width << word_size_shift;

    uint32_t orig_size_bytes = tile_line_size_bytes * tile_height;
    uint32_t size_bytes = orig_size_bytes;
    uint32_t start_offset_bytes = full_image_line_size_bytes * offset_y + offset_x_in_bytes;

    float h_byte_scale = mRdp->texture_to_load.raw_tex_metadata.h_byte_scale;
    float v_pixel_scale = mRdp->texture_to_load.raw_tex_metadata.v_pixel_scale;

    if (h_byte_scale != 1 || v_pixel_scale != 1) {
        start_offset_bytes = h_byte_scale * (v_pixel_scale * offset_y * full_image_line_size_bytes + offset_x_in_bytes);
        size_bytes *= h_byte_scale * v_pixel_scale;
        full_image_line_size_bytes *= h_byte_scale;
        tile_line_size_bytes *= h_byte_scale;
    }

    // PORT: lazy texture byte-order fixup for the LoadTile path.  The decode
    // loop in ImportTexture* reads pixels at (y * full_image_line_size_bytes
    // + x * pixel_size) for y in [0, tile_height), x in [0, tile_width), so
    // we must fix up bytes all the way to
    //   start_offset_bytes + (tile_height - 1) * full_image_line_size_bytes
    //                      + tile_line_size_bytes.
    // An earlier version used `start_offset_bytes + orig_size_bytes`, which
    // equals `start_offset_bytes + tile_line_size_bytes * tile_height` and
    // falls short whenever tile_width < full_image_width: the last row's
    // rightmost (full_image_line_size_bytes - tile_line_size_bytes) bytes
    // would stay in pass1-swapped (LE) state, rendering a byte-reversed
    // opaque strip at the bottom of the tile (Yoster "thin line" bug).
    // Fixup is idempotent so over-covering by one stride row is harmless.
    portRelocFixupTextureAtRuntime(mRdp->texture_to_load.addr,
                                   start_offset_bytes + tile_height * full_image_line_size_bytes);

    mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].orig_size_bytes = orig_size_bytes;
    mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].size_bytes = size_bytes;
    mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].full_image_line_size_bytes = full_image_line_size_bytes;
    mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].line_size_bytes = tile_line_size_bytes;

    //    assert(size_bytes <= 4096 && "bug: too big texture");
    mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].tex_flags = mRdp->texture_to_load.tex_flags;
    mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].raw_tex_metadata = mRdp->texture_to_load.raw_tex_metadata;
    mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].addr = mRdp->texture_to_load.addr + start_offset_bytes;

    const std::string_view texPath =
        mRdp->texture_to_load.raw_tex_metadata.resource != nullptr
            ? GetBaseTexturePath(mRdp->texture_to_load.raw_tex_metadata.resource->GetInitData()->Path)
            : std::string_view{};
    auto maskedTextureIter = mMaskedTextures.find(texPath);
    if (maskedTextureIter != mMaskedTextures.end()) {
        mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].masked = true;
        mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].blended =
            maskedTextureIter->second.replacementData != nullptr;
    } else {
        mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].masked = false;
        mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index].blended = false;
    }

    mRdp->texture_tile[tile].uls = uls;
    mRdp->texture_tile[tile].ult = ult;
    mRdp->texture_tile[tile].lrs = lrs;
    mRdp->texture_tile[tile].lrt = lrt;

    mRdp->textures_changed[mRdp->texture_tile[tile].tmem_index] = true;
}

/*static uint8_t color_comb_component(uint32_t v) {
    switch (v) {
        case G_CCMUX_TEXEL0:
            return CC_TEXEL0;
        case G_CCMUX_TEXEL1:
            return CC_TEXEL1;
        case G_CCMUX_PRIMITIVE:
            return CC_PRIM;
        case G_CCMUX_SHADE:
            return CC_SHADE;
        case G_CCMUX_ENVIRONMENT:
            return CC_ENV;
        case G_CCMUX_TEXEL0_ALPHA:
            return CC_TEXEL0A;
        case G_CCMUX_LOD_FRACTION:
            return CC_LOD;
        default:
            return CC_0;
    }
}

static inline uint32_t color_comb(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    return color_comb_component(a) |
           (color_comb_component(b) << 3) |
           (color_comb_component(c) << 6) |
           (color_comb_component(d) << 9);
}

static void GfxDpSetCombineMode(uint32_t rgb, uint32_t alpha) {
    mRdp->combine_mode = rgb | (alpha << 12);
}*/

void Interpreter::GfxDpSetCombineMode(uint32_t rgb, uint32_t alpha, uint32_t rgb_cyc2, uint32_t alpha_cyc2) {
    mRdp->combine_mode = rgb | (alpha << 16) | ((uint64_t)rgb_cyc2 << 28) | ((uint64_t)alpha_cyc2 << 44);
}

static inline uint32_t color_comb(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    return (a & 0xf) | ((b & 0xf) << 4) | ((c & 0x1f) << 8) | ((d & 7) << 13);
}

static inline uint32_t alpha_comb(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    return (a & 7) | ((b & 7) << 3) | ((c & 7) << 6) | ((d & 7) << 9);
}

void Interpreter::GfxDpSetGrayscaleColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    mRdp->grayscale_color.r = r;
    mRdp->grayscale_color.g = g;
    mRdp->grayscale_color.b = b;
    mRdp->grayscale_color.a = a;
}

void Interpreter::GfxDpSetEnvColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    mRdp->env_color.r = r;
    mRdp->env_color.g = g;
    mRdp->env_color.b = b;
    mRdp->env_color.a = a;
}

void Interpreter::GfxDpSetPrimColor(uint8_t m, uint8_t l, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    mRdp->prim_lod_fraction = l;
    mRdp->prim_color.r = r;
    mRdp->prim_color.g = g;
    mRdp->prim_color.b = b;
    mRdp->prim_color.a = a;
}

void Interpreter::GfxDpSetFogColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    mRdp->fog_color.r = r;
    mRdp->fog_color.g = g;
    mRdp->fog_color.b = b;
    mRdp->fog_color.a = a;
}

void Interpreter::GfxDpSetBlendColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    mRdp->blend_color.r = r;
    mRdp->blend_color.g = g;
    mRdp->blend_color.b = b;
    mRdp->blend_color.a = a;
}

void Interpreter::GfxDpSetFillColor(uint32_t packed_color) {
    uint16_t col16 = (uint16_t)packed_color;
    uint32_t r = col16 >> 11;
    uint32_t g = (col16 >> 6) & 0x1f;
    uint32_t b = (col16 >> 1) & 0x1f;
    uint32_t a = col16 & 1;
    mRdp->fill_color.r = SCALE_5_8(r);
    mRdp->fill_color.g = SCALE_5_8(g);
    mRdp->fill_color.b = SCALE_5_8(b);
    mRdp->fill_color.a = a * 255;
}

void Interpreter::GfxDrawRectangle(int32_t ulx, int32_t uly, int32_t lrx, int32_t lry) {
    uint32_t saved_other_mode_h = mRdp->other_mode_h;
    uint32_t cycle_type = (mRdp->other_mode_h & (3U << G_MDSFT_CYCLETYPE));

    if (cycle_type == G_CYC_COPY) {
        mRdp->other_mode_h = (mRdp->other_mode_h & ~(3U << G_MDSFT_TEXTFILT)) | G_TF_POINT;
    }

    // U10.2 coordinates
    float ulxf = ulx;
    float ulyf = uly;
    float lrxf = lrx;
    float lryf = lry;

    ulxf = ulxf / (4.0f * HALF_SCREEN_WIDTH(mActiveFrameBuffer)) - 1.0f;
    ulyf = -(ulyf / (4.0f * HALF_SCREEN_HEIGHT(mActiveFrameBuffer))) + 1.0f;
    lrxf = lrxf / (4.0f * HALF_SCREEN_WIDTH(mActiveFrameBuffer)) - 1.0f;
    lryf = -(lryf / (4.0f * HALF_SCREEN_HEIGHT(mActiveFrameBuffer))) + 1.0f;

    // SSB64 port widescreen: do NOT apply the clip-x compression to
    // TextureRectangle ops. The 3D world (Interpreter::GfxSpVertex) widens
    // via AdjXForAspectRatio so the camera shows more world horizontally,
    // but stage backgrounds and HUD elements are authored as full-screen
    // 2D rects whose UV mapping is fixed to a 4:3 layout — compressing
    // their X bounds shrinks them inward and leaves black side strips. We
    // keep them at their authored 4:3 NDC range; the FB clear (black) fills
    // the side strips. Bringing those into widescreen needs per-game rect
    // anchoring (cf. SoH's GFX_DIMENSIONS_FROM_LEFT_EDGE), which is Phase 2
    // scope.
    if (mWidescreenActive) {
        // Phase 1: leave rect coords at their 4:3-authored NDC values.
    } else {
        ulxf = AdjXForAspectRatio(ulxf);
        lrxf = AdjXForAspectRatio(lrxf);
    }

    struct LoadedVertex* ul = &mRsp->loaded_vertices[MAX_VERTICES + 0];
    struct LoadedVertex* ll = &mRsp->loaded_vertices[MAX_VERTICES + 1];
    struct LoadedVertex* lr = &mRsp->loaded_vertices[MAX_VERTICES + 2];
    struct LoadedVertex* ur = &mRsp->loaded_vertices[MAX_VERTICES + 3];

    ul->x = ulxf;
    ul->y = ulyf;
    ul->z = -1.0f;
    ul->w = 1.0f;

    ll->x = ulxf;
    ll->y = lryf;
    ll->z = -1.0f;
    ll->w = 1.0f;

    lr->x = lrxf;
    lr->y = lryf;
    lr->z = -1.0f;
    lr->w = 1.0f;

    ur->x = lrxf;
    ur->y = ulyf;
    ur->z = -1.0f;
    ur->w = 1.0f;

    // The coordinates for texture rectangle shall bypass the viewport setting
    struct XYWidthHeight default_viewport;
    if (!mFbActive) {
        default_viewport = { 0, (int16_t)mNativeDimensions.height, mNativeDimensions.width, mNativeDimensions.height };
    } else {
        default_viewport = { 0, (int16_t)mActiveFrameBuffer->second.orig_height, mActiveFrameBuffer->second.orig_width,
                             mActiveFrameBuffer->second.orig_height };
    }

    struct XYWidthHeight viewport_saved = mRdp->viewport;
    uint32_t geometry_mode_saved = mRsp->geometry_mode;

    AdjustVIewportOrScissor(&default_viewport);

    mRdp->viewport = default_viewport;
    mRdp->viewport_or_scissor_changed = true;
    mRsp->geometry_mode = 0;

    GfxSpTri1(MAX_VERTICES + 0, MAX_VERTICES + 1, MAX_VERTICES + 3, true);
    GfxSpTri1(MAX_VERTICES + 1, MAX_VERTICES + 2, MAX_VERTICES + 3, true);

    mRsp->geometry_mode = geometry_mode_saved;
    mRdp->viewport = viewport_saved;
    mRdp->viewport_or_scissor_changed = true;

    if (cycle_type == G_CYC_COPY) {
        mRdp->other_mode_h = saved_other_mode_h;
    }
}

void Interpreter::GfxDpTextureRectangle(int32_t ulx, int32_t uly, int32_t lrx, int32_t lry, uint8_t tile, int16_t uls,
                                        int16_t ult, int16_t dsdx, int16_t dtdy, bool flip) {
    // Skip draws targeting the Z buffer.  N64 game code sometimes flips the
    // color image to the Z buffer address, draws a texture or fill to it to
    // manipulate depth values (e.g. for an alpha-based stencil that a
    // following XLU draw AA-samples), then flips back to the primary FB.
    // Fast3D doesn't emulate that address redirection — all draws go to the
    // primary FB regardless of `color_image_address` — so the "mask" draw
    // otherwise appears as a solid opaque rectangle behind the visible
    // result. SSB64's off-screen fighter magnifier bubble in
    // ifCommonPlayerMagnifyUpdateRender is the canonical case (rendered as
    // a black square with the circle inside). Matches the same skip in
    // GfxDpFillRectangle ("Don't clear Z buffer here since we already did
    // it with glClear").
    if (mRdp->color_image_address == mRdp->z_buf_address) {
        return;
    }
    // printf("render %d at %d\n", tile, lrx);
    uint64_t saved_combine_mode = mRdp->combine_mode;
    if ((mRdp->other_mode_h & (3U << G_MDSFT_CYCLETYPE)) == G_CYC_COPY) {
        // Per RDP Command Summary Set Tile's shift s and this dsdx should be set to 4 texels
        // Divide by 4 to get 1 instead
        dsdx >>= 2;

        // Color combiner is turned off in copy mode
        GfxDpSetCombineMode(color_comb(0, 0, 0, G_CCMUX_TEXEL0), alpha_comb(0, 0, 0, G_ACMUX_TEXEL0), 0, 0);

        // Per documentation one extra pixel is added in this modes to each edge
        lrx += 1 << 2;
        lry += 1 << 2;
    }

    // uls and ult are S10.5
    // dsdx and dtdy are S5.10
    // lrx, lry, ulx, uly are U10.2
    // lrs, lrt are S10.5
    if (flip) {
        dsdx = -dsdx;
        dtdy = -dtdy;
    }
    int16_t width = !flip ? lrx - ulx : lry - uly;
    int16_t height = !flip ? lry - uly : lrx - ulx;
    float lrs = ((uls << 7) + dsdx * width) >> 7;
    float lrt = ((ult << 7) + dtdy * height) >> 7;

    LoadedVertex* ul = &mRsp->loaded_vertices[MAX_VERTICES + 0];
    LoadedVertex* ll = &mRsp->loaded_vertices[MAX_VERTICES + 1];
    LoadedVertex* lr = &mRsp->loaded_vertices[MAX_VERTICES + 2];
    LoadedVertex* ur = &mRsp->loaded_vertices[MAX_VERTICES + 3];
    ul->u = uls;
    ul->v = ult;
    lr->u = lrs;
    lr->v = lrt;
    if (!flip) {
        ll->u = uls;
        ll->v = lrt;
        ur->u = lrs;
        ur->v = ult;
    } else {
        ll->u = lrs;
        ll->v = ult;
        ur->u = uls;
        ur->v = lrt;
    }

    uint8_t saved_tile = mRdp->first_tile_index;
    if (saved_tile != tile) {
        mRdp->textures_changed[0] = true;
        mRdp->textures_changed[1] = true;
    }
    mRdp->first_tile_index = tile;

    GfxDrawRectangle(ulx, uly, lrx, lry);
    if (saved_tile != tile) {
        mRdp->textures_changed[0] = true;
        mRdp->textures_changed[1] = true;
    }
    mRdp->first_tile_index = saved_tile;
    mRdp->combine_mode = saved_combine_mode;
}

void Interpreter::GfxDpImageRectangle(int32_t tile, int32_t w, int32_t h, int32_t ulx, int32_t uly, int16_t uls,
                                      int16_t ult, int32_t lrx, int32_t lry, int16_t lrs, int16_t lrt) {

    LoadedVertex* ul = &mRsp->loaded_vertices[MAX_VERTICES + 0];
    LoadedVertex* ll = &mRsp->loaded_vertices[MAX_VERTICES + 1];
    LoadedVertex* lr = &mRsp->loaded_vertices[MAX_VERTICES + 2];
    LoadedVertex* ur = &mRsp->loaded_vertices[MAX_VERTICES + 3];
    ul->u = uls * 32;
    ul->v = ult * 32;
    lr->u = lrs * 32;
    lr->v = lrt * 32;
    ll->u = uls * 32;
    ll->v = lrt * 32;
    ur->u = lrs * 32;
    ur->v = ult * 32;

    // ensure we have the correct texture size, format and starting position
    mRdp->texture_tile[tile].siz = G_IM_SIZ_8b;
    mRdp->texture_tile[tile].fmt = G_IM_FMT_RGBA;
    mRdp->texture_tile[tile].cms = 0;
    mRdp->texture_tile[tile].cmt = 0;
    mRdp->texture_tile[tile].shifts = 0;
    mRdp->texture_tile[tile].shiftt = 0;
    mRdp->texture_tile[tile].uls = 0 * 4;
    mRdp->texture_tile[tile].ult = 0 * 4;
    mRdp->texture_tile[tile].lrs = w * 4;
    mRdp->texture_tile[tile].lrt = h * 4;
    mRdp->texture_tile[tile].line_size_bytes = w << (mRdp->texture_tile[tile].siz >> 1);

    auto& loadtex = mRdp->loaded_texture[mRdp->texture_tile[tile].tmem_index];
    loadtex.full_image_line_size_bytes = loadtex.line_size_bytes = mRdp->texture_tile[tile].line_size_bytes;
    loadtex.size_bytes = loadtex.orig_size_bytes = loadtex.line_size_bytes * h;

    uint8_t saved_tile = mRdp->first_tile_index;
    if (saved_tile != tile) {
        mRdp->textures_changed[0] = true;
        mRdp->textures_changed[1] = true;
    }
    mRdp->first_tile_index = tile;

    GfxDrawRectangle(ulx, uly, lrx, lry);
    if (saved_tile != tile) {
        mRdp->textures_changed[0] = true;
        mRdp->textures_changed[1] = true;
    }
    mRdp->first_tile_index = saved_tile;
}

void Interpreter::GfxDpFillRectangle(int32_t ulx, int32_t uly, int32_t lrx, int32_t lry) {
    if (mRdp->color_image_address == mRdp->z_buf_address) {
        // PORT: SSB64's mvOpeningRoom transition Outline issues a full-screen
        // FillRectangle to ZB right before drawing the visible silhouette tris
        // with G_RM_AA_OPA_SURF (Z_CMP enabled). On real hardware that fill
        // rewrites Z to a uniform value so the subsequent tris always pass
        // depth comparison. Previously we just skipped this entirely under
        // the assumption that the framebuffer's depth attachment had already
        // been cleared by a glClear at start-of-frame, but that assumption
        // doesn't hold mid-frame: the wallpaper drew with G_RM_AA_ZB_OPA_SURF
        // (Z_UPD set) and filled the depth buffer with its own values, so
        // Outline tris Z-fail and the transition silhouette never appears.
        // Treat the redirect-fill as an actual depth clear so the Outline's
        // subsequent OPA tris have a clean Z buffer to draw against.
        Flush();
        mRapi->ClearFramebuffer(false, true);
        mRenderingState.depth_test_and_mask = 0xff; /* invalidate cached state */
        return;
    }
    uint32_t mode = (mRdp->other_mode_h & (3U << G_MDSFT_CYCLETYPE));

    // OTRTODO: This is a bit of a hack for widescreen screen fades, but it'll work for now...
    if (ulx == 0 && uly == 0 && lrx == (319 * 4) && lry == (239 * 4)) {
        ulx = -1024;
        uly = -1024;
        lrx = 2048;
        lry = 2048;
    }

    if (mode == G_CYC_COPY || mode == G_CYC_FILL) {
        // Per documentation one extra pixel is added in this modes to each edge
        lrx += 1 << 2;
        lry += 1 << 2;
    }

    for (int i = MAX_VERTICES; i < MAX_VERTICES + 4; i++) {
        LoadedVertex* v = &mRsp->loaded_vertices[i];
        v->color = mRdp->fill_color;
    }

    uint64_t saved_combine_mode = mRdp->combine_mode;

    if (mode == G_CYC_FILL) {
        GfxDpSetCombineMode(color_comb(0, 0, 0, G_CCMUX_SHADE), alpha_comb(0, 0, 0, G_ACMUX_SHADE), 0, 0);
    }

    GfxDrawRectangle(ulx, uly, lrx, lry);
    mRdp->combine_mode = saved_combine_mode;
}

void Interpreter::GfxDpSetZImage(void* zBufAddr) {
    mRdp->z_buf_address = zBufAddr;
}

void Interpreter::GfxDpSetColorImage(uint32_t format, uint32_t size, uint32_t width, void* address) {
    mRdp->color_image_address = address;
}

void Interpreter::GfxSpSetOtherMode(uint32_t shift, uint32_t num_bits, uint64_t mode) {
    uint64_t mask = (((uint64_t)1 << num_bits) - 1) << shift;
    uint64_t om = mRdp->other_mode_l | ((uint64_t)mRdp->other_mode_h << 32);
    om = (om & ~mask) | mode;
    mRdp->other_mode_l = (uint32_t)om;
    mRdp->other_mode_h = (uint32_t)(om >> 32);
}

void Interpreter::GfxDpSetOtherMode(uint32_t h, uint32_t l) {
    mRdp->other_mode_h = h;
    mRdp->other_mode_l = l;
}

void Interpreter::Gfxs2dexBgCopy(F3DuObjBg* bg) {
    /*
    bg->b.imageX = 0;
    bg->b.imageW = width * 4;
    bg->b.frameX = frameX * 4;
    bg->b.imageY = 0;
    bg->b.imageH = height * 4;
    bg->b.frameY = frameY * 4;
    bg->b.imagePtr = source;
    bg->b.imageLoad = G_BGLT_LOADTILE;
    bg->b.imageFmt = fmt;
    bg->b.imageSiz = siz;
    bg->b.imagePal = 0;
    bg->b.imageFlip = 0;
    */

    uintptr_t data = (uintptr_t)bg->b.imagePtr;

    uint32_t texFlags = 0;
    RawTexMetadata rawTexMetadata = {};

    if ((bool)gfx_check_image_signature((char*)data)) {
        std::shared_ptr<Fast::Texture> tex = std::static_pointer_cast<Fast::Texture>(
            Ship::Context::GetInstance()->GetResourceManager()->LoadResourceProcess((char*)data));
        texFlags = tex->Flags;
        rawTexMetadata.width = tex->Width;
        rawTexMetadata.height = tex->Height;
        rawTexMetadata.h_byte_scale = tex->HByteScale;
        rawTexMetadata.v_pixel_scale = tex->VPixelScale;
        rawTexMetadata.type = tex->Type;
        rawTexMetadata.resource = tex;
        data = (uintptr_t) reinterpret_cast<char*>(tex->ImageData);
    }

    s16 dsdx = 4 << 10;
    s16 uls = bg->b.imageX << 3;
    // Flip flag only flips horizontally
    if (bg->b.imageFlip == G_BG_FLAG_FLIPS) {
        dsdx = -dsdx;
        uls = (bg->b.imageW - bg->b.imageX) << 3;
    }

    SUPPORT_CHECK(bg->b.imageSiz == G_IM_SIZ_16b);
    GfxDpSetTextureImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, nullptr, texFlags, rawTexMetadata, (void*)data);
    GfxDpSetTile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0);
    GfxDpLoadBlock(G_TX_LOADTILE, 0, 0, (bg->b.imageW * bg->b.imageH >> 4) - 1, 0);
    GfxDpSetTile(bg->b.imageFmt, G_IM_SIZ_16b, bg->b.imageW >> 4, 0, G_TX_RENDERTILE, bg->b.imagePal, 0, 0, 0, 0, 0, 0);
    GfxDpSetTileSize(G_TX_RENDERTILE, 0, 0, bg->b.imageW, bg->b.imageH);
    GfxDpTextureRectangle(bg->b.frameX, bg->b.frameY, bg->b.frameX + bg->b.imageW - 4, bg->b.frameY + bg->b.imageH - 4,
                          G_TX_RENDERTILE, uls, bg->b.imageY << 3, dsdx, 1 << 10, false);
}

void Interpreter::Gfxs2dexBg1cyc(F3DuObjBg* bg) {
    uintptr_t data = (uintptr_t)bg->b.imagePtr;

    uint32_t texFlags = 0;
    RawTexMetadata rawTexMetadata = {};

    if ((bool)gfx_check_image_signature((char*)data)) {
        std::shared_ptr<Fast::Texture> tex = std::static_pointer_cast<Fast::Texture>(
            Ship::Context::GetInstance()->GetResourceManager()->LoadResourceProcess((char*)data));
        texFlags = tex->Flags;
        rawTexMetadata.width = tex->Width;
        rawTexMetadata.height = tex->Height;
        rawTexMetadata.h_byte_scale = tex->HByteScale;
        rawTexMetadata.v_pixel_scale = tex->VPixelScale;
        rawTexMetadata.type = tex->Type;
        rawTexMetadata.resource = tex;
        data = (uintptr_t) reinterpret_cast<char*>(tex->ImageData);
    }

    // TODO: Implement bg scaling correctly
    s16 uls = bg->b.imageX >> 2;
    s16 lrs = bg->b.imageW >> 2;

    s16 dsdxRect = 1 << 10;
    s16 ulsRect = bg->b.imageX << 3;
    // Flip flag only flips horizontally
    if (bg->b.imageFlip == G_BG_FLAG_FLIPS) {
        dsdxRect = -dsdxRect;
        ulsRect = (bg->b.imageW - bg->b.imageX) << 3;
    }

    GfxDpSetTextureImage(bg->b.imageFmt, bg->b.imageSiz, bg->b.imageW >> 2, nullptr, texFlags, rawTexMetadata,
                         (void*)data);
    GfxDpSetTile(bg->b.imageFmt, bg->b.imageSiz, 0, 0, G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0);
    GfxDpLoadBlock(G_TX_LOADTILE, 0, 0, (bg->b.imageW * bg->b.imageH >> 4) - 1, 0);
    GfxDpSetTile(bg->b.imageFmt, bg->b.imageSiz, (((lrs - uls) * bg->b.imageSiz) + 7) >> 3, 0, G_TX_RENDERTILE,
                 bg->b.imagePal, 0, 0, 0, 0, 0, 0);
    GfxDpSetTileSize(G_TX_RENDERTILE, 0, 0, bg->b.imageW, bg->b.imageH);

    GfxDpTextureRectangle(bg->b.frameX, bg->b.frameY, bg->b.frameW, bg->b.frameH, G_TX_RENDERTILE, ulsRect,
                          bg->b.imageY << 3, dsdxRect, 1 << 10, false);
}

void Interpreter::Gfxs2dexRecyCopy(F3DuObjSprite* spr) {
    s16 dsdx = 4 << 10;
    [[maybe_unused]] s16 uls = spr->s.objX << 3;
    // Flip flag only flips horizontally
    if (spr->s.imageFlags == G_BG_FLAG_FLIPS) {
        dsdx = -dsdx;
        uls = (spr->s.imageW - spr->s.objX) << 3;
    }

    int realX = spr->s.objX >> 2;
    int realY = spr->s.objY >> 2;
    int realW = (((spr->s.imageW)) >> 5);
    int realH = (((spr->s.imageH)) >> 5);
    float realSW = spr->s.scaleW / 1024.0f;
    float realSH = spr->s.scaleH / 1024.0f;

    int testX = (realX + (realW / realSW));
    int testY = (realY + (realH / realSH));

    GfxDpTextureRectangle(realX << 2, realY << 2, testX << 2, testY << 2, G_TX_RENDERTILE,
                          (s32)mRdp->texture_tile[0].uls << 3, (s32)mRdp->texture_tile[0].ult << 3,
                          (float)(1 << 10) * realSW, (float)(1 << 10) * realSH, false);
}

void* Interpreter::SegAddr(uintptr_t w1) {
    if (w1 <= UINT32_MAX) {
        void* relocPtr = portRelocTryResolvePointer((uint32_t)w1);

        if (relocPtr != nullptr) {
            return relocPtr;
        }
    }

    // Raw reloc-file display lists still use classic N64 segmented addresses,
    // while some libultraship-generated commands tag the low bit. Support both.
    uint32_t segNum = (uint32_t)(w1 >> 24);

    if ((segNum < MAX_SEGMENT_POINTERS) && (mSegmentPointers[segNum] != 0)) {
        uint32_t offset = (uint32_t)(w1 & 0x00FFFFFF);

        if ((w1 & 1) != 0) {
            offset &= ~1u;
        }
        return (void*)(mSegmentPointers[segNum] + offset);
    }

    return (void*)w1;
}

#define C0(pos, width) ((cmd->words.w0 >> (pos)) & ((1U << width) - 1))
#define C1(pos, width) ((cmd->words.w1 >> (pos)) & ((1U << width) - 1))

void GfxExecStack::start(F3DGfx* dlist) {
    while (!cmd_stack.empty())
        cmd_stack.pop();
    gfx_path.clear();
    F3DGfx* normalized = portNormalizeDisplayListPointer(dlist);
    diagRecordDLPush(nullptr, dlist, normalized, "ExecStack::start");
    cmd_stack.push(normalized);
    disp_stack.clear();
}

void GfxExecStack::stop() {
    while (!cmd_stack.empty())
        cmd_stack.pop();
    gfx_path.clear();
}

F3DGfx*& GfxExecStack::currCmd() {
    return cmd_stack.top();
}

void GfxExecStack::openDisp(const char* file, int line) {
    disp_stack.push_back({ file, line });
}
void GfxExecStack::closeDisp() {
    disp_stack.pop_back();
}
const std::vector<GfxExecStack::CodeDisp>& GfxExecStack::getDisp() const {
    return disp_stack;
}

void GfxExecStack::branch(F3DGfx* caller) {
    F3DGfx* old = cmd_stack.top();
    cmd_stack.pop();
    cmd_stack.push(nullptr);
    F3DGfx* normalized = portNormalizeDisplayListPointer(old);
    diagRecordDLPush(caller, old, normalized, "ExecStack::branch");
    cmd_stack.push(normalized);

    gfx_path.push_back(caller);
}

void GfxExecStack::call(F3DGfx* caller, F3DGfx* callee) {
    F3DGfx* normalized = portNormalizeDisplayListPointer(callee);
    diagRecordDLPush(caller, callee, normalized, "ExecStack::call");
    cmd_stack.push(normalized);
    gfx_path.push_back(caller);
}

F3DGfx* GfxExecStack::ret() {
    F3DGfx* cmd = cmd_stack.top();

    cmd_stack.pop();
    if (!gfx_path.empty()) {
        gfx_path.pop_back();
    }

    while (cmd_stack.size() > 0 && cmd_stack.top() == nullptr) {
        cmd_stack.pop();
        if (!gfx_path.empty()) {
            gfx_path.pop_back();
        }
    }
    return cmd;
}

void gfx_set_framebuffer(int fb, float noise_scale);
void gfx_reset_framebuffer();
void gfx_copy_framebuffer(int fb_dst_id, int fb_src_id, bool copyOnce, bool* hasCopiedPtr);

// The main type of the handler function. These function will take a pointer to a pointer to a Gfx. It needs to be a
// double pointer because we sometimes need to increment and decrement the underlying pointer Returns false if the
// current opcode should be incremented after the handler ends.
typedef bool (*GfxOpcodeHandlerFunc)(F3DGfx** gfx);

bool gfx_load_ucode_handler_f3dex2(F3DGfx** cmd) {
    Interpreter* gfx = mInstance.lock().get();
    gfx->mRsp->fog_mul = 0;
    gfx->mRsp->fog_offset = 0;
    return false;
}

bool gfx_cull_dl_handler_f3dex2(F3DGfx** cmd) {
    // TODO:
    return false;
}

bool gfx_marker_handler_otr(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    (*cmd0)++;
    F3DGfx* cmd = (*cmd0);
    gfx->mMarkerOn = true;
    return false;
}

bool gfx_invalidate_tex_cache_handler_f3dex2(F3DGfx** cmd) {
    Interpreter* gfx = mInstance.lock().get();
    const uintptr_t texAddr = (*cmd)->words.w1;

    if (texAddr == 0) {
        gfx->TextureCacheClear();
    } else {
        gfx->TextureCacheDelete((const uint8_t*)texAddr);
    }
    return false;
}

bool gfx_noop_handler_f3dex2(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;
    const char* filename = (const char*)(cmd)->words.w1;
    uint32_t p = C0(16, 8);
    uint32_t l = C0(0, 16);
    if (p == 7) {
        g_exec_stack.openDisp(filename, l);
    } else if (p == 8) {
        if (g_exec_stack.disp_stack.size() == 0) {
            SPDLOG_WARN("CLOSE_DISPS without matching open {}:{}", p, l);
        } else {
            g_exec_stack.closeDisp();
        }
    }
    return false;
}

bool gfx_mtx_handler_f3dex2(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;
    uintptr_t mtxAddr = cmd->words.w1;

    gfx->GfxSpMatrix(C0(0, 8) ^ F3DEX2_G_MTX_PUSH, (const int32_t*)gfx->SegAddr(mtxAddr));
    return false;
}
// Seems to be the same for all other non F3DEX2 microcodes...
bool gfx_mtx_handler_f3d(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;
    uintptr_t mtxAddr = cmd->words.w1;

    gfx->GfxSpMatrix(C0(16, 8), (const int32_t*)gfx->SegAddr(cmd->words.w1));
    return false;
}

bool gfx_mtx_otr_filepath_handler_custom_f3dex2(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;
    const char* fileName = (const char*)gfx->SegAddr(cmd->words.w1);
    const int32_t* mtx = (const int32_t*)Ship::Context::GetInstance()->GetResourceManager()->GetResourceRawPointer(
        (const char*)fileName);

    if (mtx != NULL) {
        gfx->GfxSpMatrix(C0(0, 8) ^ F3DEX2_G_MTX_PUSH, mtx);
    }

    return false;
}

bool gfx_mtx_otr_filepath_handler_custom_f3d(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;
    const char* fileName = (const char*)gfx->SegAddr(cmd->words.w1);
    const int32_t* mtx = (const int32_t*)Ship::Context::GetInstance()->GetResourceManager()->GetResourceRawPointer(
        (const char*)fileName);

    if (mtx != NULL) {
        gfx->GfxSpMatrix(C0(16, 8), mtx);
    }

    return false;
}

bool gfx_mtx_otr_filepath_handler_custom(F3DGfx** cmd0) {
    if (ucode_handler_index == ucode_f3dex2) {
        return gfx_mtx_otr_filepath_handler_custom_f3dex2(cmd0);
    } else {
        return gfx_mtx_otr_filepath_handler_custom_f3d(cmd0);
    }
}

bool gfx_mtx_otr_handler_custom_f3dex2(F3DGfx** cmd0) {
    (*cmd0)++;
    F3DGfx* cmd = *cmd0;

    const uint64_t hash = ((uint64_t)cmd->words.w0 << 32) + cmd->words.w1;
    const int32_t* mtx =
        (const int32_t*)Ship::Context::GetInstance()->GetResourceManager()->GetResourceRawPointer(hash);

    if (mtx != NULL) {
        Interpreter* gfx = mInstance.lock().get();
        cmd--;
        gfx->GfxSpMatrix(C0(0, 8) ^ F3DEX2_G_MTX_PUSH, mtx);
        cmd++;
    }

    return false;
}

bool gfx_mtx_otr_handler_custom_f3d(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    (*cmd0)++;
    F3DGfx* cmd = *cmd0;

    const uint64_t hash = ((uint64_t)cmd->words.w0 << 32) + cmd->words.w1;
    const int32_t* mtx =
        (const int32_t*)Ship::Context::GetInstance()->GetResourceManager()->GetResourceRawPointer(hash);
    if (mtx != nullptr) {
        cmd--;
        gfx->GfxSpMatrix(C0(16, 8), mtx);
        cmd++;
    }
    return false;
}

bool gfx_mtx_otr_handler_custom(F3DGfx** cmd0) {
    if (ucode_handler_index == ucode_f3dex2) {
        return gfx_mtx_otr_handler_custom_f3dex2(cmd0);
    } else {
        return gfx_mtx_otr_handler_custom_f3d(cmd0);
    }
}

bool gfx_pop_mtx_handler_f3dex2(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxSpPopMatrix((uint32_t)(cmd->words.w1 / 64));

    return false;
}

bool gfx_pop_mtx_handler_f3d(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxSpPopMatrix(1);

    return false;
}

bool gfx_movemem_handler_f3dex2(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxSpMovememF3dex2(C0(0, 8), C0(8, 8) * 8, gfx->SegAddr(cmd->words.w1));

    return false;
}

bool gfx_movemem_handler_f3d(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxSpMovememF3d(C0(16, 8), 0, gfx->SegAddr(cmd->words.w1));

    return false;
}

bool gfx_movemem_handler_otr(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    const uint8_t index = C1(24, 8);
    const uint8_t offset = C1(16, 8);
    const uint8_t hasOffset = C1(8, 8);

    (*cmd0)++;

    const uint64_t hash = ((uint64_t)(*cmd0)->words.w0 << 32) + (*cmd0)->words.w1;

    if (ucode_handler_index == ucode_f3dex2) {
        gfx->GfxSpMovememF3dex2(index, offset,
                                Ship::Context::GetInstance()->GetResourceManager()->GetResourceRawPointer(hash));
    } else {
        auto light = (Fast::LightEntry*)Ship::Context::GetInstance()->GetResourceManager()->GetResourceRawPointer(hash);
        uintptr_t data = (uintptr_t)&light->Ambient;
        gfx->GfxSpMovememF3d(index, offset, (void*)(data + (hasOffset == 1 ? 0x8 : 0)));
    }
    return false;
}

bool gfx_push_shader(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;
    const char* shader = (const char*)gfx->SegAddr(cmd->words.w1);

    gfx->mShaders[gfx->mShadersIndex] = shader;
    gfx->mShaderStack.push(gfx->mShadersIndex);
    gfx->mShadersIndex++;

    return false;
}

bool gfx_pop_shader(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    /* Defensive empty-check. The shader stack can be unbalanced in two
     * scenarios:
     *   1. A garbage byte from a runaway DL walk happens to encode
     *      OTR_G_POP_SHADER — a stale-pointer crash family.
     *   2. The walk was aborted mid-flight (via g_exec_stack.stop() from
     *      the bounds-check in gfx_step) after a matching G_PUSH_SHADER
     *      had executed but before its G_POP_SHADER — leaves the stack
     *      with a stray entry that survives to the next frame.
     * Either way, popping an empty std::stack hits a libstdc++ assertion
     * (stl_stack.h:333) and SIGABRTs. Skipping the pop preserves
     * correctness when (1) and is harmless when (2). */
    if (gfx->mShaderStack.empty()) {
        static int sUnbalancedPopWarnCount = 0;
        if (sUnbalancedPopWarnCount < 10) {
            sUnbalancedPopWarnCount++;
            SPDLOG_WARN("gfx_pop_shader: unbalanced pop on empty mShaderStack — skipping (cmd=0x{:x})",
                        (unsigned long long)(uintptr_t)cmd);
        }
        return false;
    }
    gfx->mShaderStack.pop();

    return false;
}

const char* gfx_get_shader(int16_t id) {
    Interpreter* gfx = mInstance.lock().get();

    for (const std::pair<size_t, const char*>& shader : gfx->mShaders) {
        if (shader.first == id) {
            return shader.second;
        }
    }

    return nullptr; // Use no shader
}

bool gfx_moveword_handler_f3dex2(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxSpMovewordF3dex2(C0(16, 8), C0(0, 16), cmd->words.w1);

    return false;
}

bool gfx_moveword_handler_f3d(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxSpMovewordF3d(C0(0, 8), C0(8, 16), cmd->words.w1);

    return false;
}

bool gfx_texture_handler_f3dex2(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxSpTexture(C1(16, 16), C1(0, 16), C0(11, 3), C0(8, 3), C0(1, 7));

    return false;
}

// Seems to be the same for all other non F3DEX2 microcodes...
bool gfx_texture_handler_f3d(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxSpTexture(C1(16, 16), C1(0, 16), C0(11, 3), C0(8, 3), C0(0, 8));

    return false;
}

// If SegAddr fell through every resolution path, w1 came back unchanged as a
// raw value. Sub-segment-range values (<= 0x0FFFFFFF) cannot be valid host
// pointers; dereferencing them in GfxSpVertex SIGSEGVs at fault_addr =
// resolved_w1. Skip the load and log so the upstream state corruption that
// seeded the bad pointer remains traceable. Mirrors the SETTIMG guard added
// in upstream LUS PR #1042.
static inline bool gfx_vtx_addr_is_unresolved(const void* addr) {
    return (uintptr_t)addr <= 0x0FFFFFFFu;
}

// Almost all versions of the microcode have their own version of this opcode
bool gfx_vtx_handler_f3dex2(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    uint32_t n_vertices = C0(12, 8);
    uint32_t v_dest_end = C0(1, 7);
    const F3DVtx* vertices = (const F3DVtx*)gfx->SegAddr(cmd->words.w1);

    if (gfx_vtx_addr_is_unresolved(vertices)) {
        SPDLOG_ERROR("G_VTX(f3dex2) skipped: unresolved addr=0x{:x} n={} dest_end={} (raw w1=0x{:x})",
                     (uintptr_t)vertices, n_vertices, v_dest_end, (uintptr_t)cmd->words.w1);
        return false;
    }

    // Lazy vertex byte-order fixup (port-side, Option A).
    // Per-vertex idempotency handles overlapping sub-region reloads.
    portRelocFixupVertexAtRuntime((const void*)vertices, n_vertices);

    gfx->GfxSpVertex(n_vertices, v_dest_end - n_vertices, vertices);

    return false;
}

bool gfx_vtx_handler_f3dex(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    const F3DVtx* vertices = (const F3DVtx*)gfx->SegAddr(cmd->words.w1);
    if (gfx_vtx_addr_is_unresolved(vertices)) {
        SPDLOG_ERROR("G_VTX(f3dex) skipped: unresolved addr=0x{:x} n={} dest={} (raw w1=0x{:x})",
                     (uintptr_t)vertices, C0(10, 6), C0(17, 7), (uintptr_t)cmd->words.w1);
        return false;
    }
    gfx->GfxSpVertex(C0(10, 6), C0(17, 7), vertices);

    return false;
}

bool gfx_vtx_handler_f3d(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    const F3DVtx* vertices = (const F3DVtx*)gfx->SegAddr(cmd->words.w1);
    if (gfx_vtx_addr_is_unresolved(vertices)) {
        SPDLOG_ERROR("G_VTX(f3d) skipped: unresolved addr=0x{:x} n={} dest={} (raw w1=0x{:x})",
                     (uintptr_t)vertices, (C0(0, 16)) / sizeof(F3DVtx), C0(16, 4),
                     (uintptr_t)cmd->words.w1);
        return false;
    }
    gfx->GfxSpVertex((C0(0, 16)) / sizeof(F3DVtx), C0(16, 4), vertices);

    return false;
}

bool gfx_vtx_hash_handler_custom(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    // Offset added to the start of the vertices
    const uintptr_t offset = (*cmd0)->words.w1;
    // This is a two-part display list command, so increment the instruction pointer so we can get the CRC64
    // hash from the second
    (*cmd0)++;
    const uint64_t hash = ((uint64_t)(*cmd0)->words.w0 << 32) + (*cmd0)->words.w1;

    // We need to know if the offset is a cached pointer or not. An offset greater than one million is not a
    // real offset, so it must be a real pointer
    if (offset > 0xFFFFF) {
        (*cmd0)--;
        F3DGfx* cmd = *cmd0;
        gfx->GfxSpVertex(C0(12, 8), C0(1, 7) - C0(12, 8), (F3DVtx*)offset);
        (*cmd0)++;
    } else {
        F3DVtx* vtx = (F3DVtx*)Ship::Context::GetInstance()->GetResourceManager()->GetResourceRawPointer(hash);

        if (vtx != NULL) {
            vtx = (F3DVtx*)((char*)vtx + offset);

            (*cmd0)--;
            F3DGfx* cmd = *cmd0;

            // TODO: WTF??
            cmd->words.w1 = (uintptr_t)vtx;

            gfx->GfxSpVertex(C0(12, 8), C0(1, 7) - C0(12, 8), vtx);
            (*cmd0)++;
        }
    }
    return false;
}

bool gfx_vtx_otr_filepath_handler_custom(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;
    char* fileName = (char*)gfx->SegAddr(cmd->words.w1);
    (*cmd0)++;
    cmd = *cmd0;
    size_t vtxCnt = cmd->words.w0;
    size_t vtxIdxOff = cmd->words.w1 >> 16;
    size_t vtxDataOff = cmd->words.w1 & 0xFFFF;
    F3DVtx* vtx =
        (F3DVtx*)Ship::Context::GetInstance()->GetResourceManager()->GetResourceRawPointer((const char*)fileName);
    vtx += vtxDataOff;

    gfx->GfxSpVertex(vtxCnt, vtxIdxOff, vtx);
    return false;
}

bool gfx_dl_otr_filepath_handler_custom(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;
    char* fileName = (char*)gfx->SegAddr(cmd->words.w1);
    F3DGfx* nDL =
        (F3DGfx*)Ship::Context::GetInstance()->GetResourceManager()->GetResourceRawPointer((const char*)fileName);

    if (C0(16, 1) == 0 && nDL != nullptr) {
        g_exec_stack.call(*cmd0, nDL);
    } else {
        if (nDL != nullptr) {
            (*cmd0) = nDL;
            g_exec_stack.branch(cmd);
            return true; // shortcut cmd increment
        } else {
            assert(0 && "???");
            // gfx_path.pop_back();
            // cmd = cmd_stack.top();
            // cmd_stack.pop();
        }
    }
    return false;
}

// The original F3D microcode doesn't seem to have this opcode. Glide handles it as part of moveword
bool gfx_modify_vtx_handler_f3dex2(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;
    gfx->GfxSpModifyVertex(C0(1, 15), C0(16, 8), (uint32_t)cmd->words.w1);
    return false;
}

// F3D, F3DEX, and F3DEX2 do the same thing but F3DEX2 has its own opcode number
bool gfx_dl_handler_common(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;
    F3DGfx* subGFX = (F3DGfx*)gfx->SegAddr(cmd->words.w1);

    // PORT FIX: stride correction for runtime segment 0x0E DLs.
    //
    // SSB64 builds per-MObj material setup sub-DLs in the graphics heap and
    // points segment 0x0E at them via gsSPSegment(0xE, runtime_buf).  The
    // stored model DL then calls into them with `gsSPDisplayList(0x0E000000 + N)`
    // where N is the BYTE offset within the runtime buffer.
    //
    // On N64, each Gfx is 8 bytes, so N=8 means "second cmd".  On the port,
    // game-built runtime DLs use the libultraship native 16-byte Gfx layout
    // (uintptr_t w0; uintptr_t w1;), so "second cmd" is at byte offset 16.
    //
    // SegAddr returns mSegmentPointers[0x0E] + N (raw byte offset).  When the
    // segment base is heap-allocated (game-built widened DL), we need to
    // convert: cmd_index = N / 8; widened_offset = cmd_index * sizeof(F3DGfx).
    //
    // Without this, the interpreter lands halfway through a 16-byte widened
    // cmd and reads w1-of-cmdN, w0-of-cmdN+1 as if they were one cmd —
    // producing a pointer-shaped "opcode" and a cmd-word-shaped "pointer",
    // which then dispatches as garbage G_VTX with bogus vertex addresses.
    {
        uint8_t segByte = (uint8_t)((cmd->words.w1 >> 24) & 0xFF);
        if (segByte == 0x0E) {
            uint32_t segNum = 0x0E;
            uintptr_t segBase = (segNum < MAX_SEGMENT_POINTERS) ? gfx->mSegmentPointers[segNum] : 0;
            if (segBase != 0) {
                uintptr_t segFileBase = 0;
                size_t segFileSize = 0;
                bool baseInFile = portRelocFindContainingFile(reinterpret_cast<const void*>(segBase),
                                                              &segFileBase, &segFileSize);
                if (!baseInFile) {
                    /* Heap-built widened DL: convert N64 byte offset → widened. */
                    uint32_t n64_offset = (uint32_t)(cmd->words.w1 & 0x00FFFFFF);
                    if ((n64_offset & 7u) == 0u) {
                        uint32_t cmd_index = n64_offset / PORT_PACKED_GFX_SIZE;
                        subGFX = (F3DGfx*)(segBase + cmd_index * sizeof(F3DGfx));
                    }
                }
            }
        }
    }

    // Fallback for packed seg=0x0E G_DL commands left unresolved by
    // portNormalizeDisplayListPointer.  SegAddr returns the runtime segment
    // 0x0E base + offset when the game has set it via gsSPSegment(0xE, ...).
    // If no runtime segment is set, SegAddr returns the raw w1 (a garbage
    // pointer like 0x0E000000+offset); in that case fall back to the calling
    // DL's containing reloc file (intra-file sub-DL branch).
    {
        uint8_t segByte = (uint8_t)((cmd->words.w1 >> 24) & 0xFF);
        if (segByte == 0x0E && (uintptr_t)subGFX == cmd->words.w1) {
            uintptr_t fileBase = 0;
            size_t fileSize = 0;
            if (portRelocFindContainingFile(cmd, &fileBase, &fileSize)) {
                uint32_t offset = (uint32_t)(cmd->words.w1 & 0x00FFFFFF);
                if (offset < fileSize) {
                    subGFX = (F3DGfx*)(fileBase + offset);
                }
            }
        }
    }

    /* DL-range bounds check. Reject push if subGFX would walk into a
     * page that's about to fault. Two cases caught here:
     *   - Unresolved N64 segments (≤ 0x0FFFFFFF): no segment-table entry,
     *     SegAddr returned the raw N64 token. (Variant 4 in
     *     docs/bugs/linux_stale_scene_data_family_2026-05-11.md.)
     *   - Walked-past-registered-range: subGFX is just past the end of a
     *     known DL allocation. (Variant 5 — game-built DL without an
     *     explicit gsSPEndDisplayList terminator runs off the scene
     *     arena.)
     * Unregistered addresses are allowed through — the check is
     * conservative to avoid false-rejecting valid pushes from subsystems
     * that haven't been hooked into the registry yet. */
    {
        /* DL push bounds check. Reject pushes that would lead the walker
         * into a registered range it just walked past (variant 5 — runaway
         * DL without a gsSPEndDisplayList terminator). We deliberately do
         * NOT reject sub-N64-segment addresses (subAddr <= 0x0FFFFFFFu)
         * here: SSB64 routinely emits `gsSPDisplayList(0x012792c0)`-style
         * segment-relative references where the segment isn't bound for
         * this frame (e.g. segment 1 is bound only by lbtransition for
         * VS-results photocopy). SegAddr returns raw w1 in those cases,
         * and rejecting the push silently drops legitimate sub-DLs across
         * the entire scene. The walker handles unresolved cmds via
         * gfx_step's deref + variant-4's separate ASan diag path; the
         * cost of an occasional bad-cmd deref on Linux/glibc is one
         * SIGSEGV per crash, which the WALKED_PAST guard plus the
         * gcDrawMObjForDObj G_ENDDL terminator (decomp side) avoid in
         * the cases we've measured. */
        uintptr_t subAddr = (uintptr_t)subGFX;
        if (sDLBoundsCheck && sDLBoundsCheck(subAddr) == kDLBoundsWalkedPast) {
            static int sRejectCount = 0;
            if (sRejectCount < 10) {
                sRejectCount++;
                SPDLOG_WARN("gfx_dl_handler: rejecting DL push just past a "
                            "registered range (w1=0x{:x}, subGFX=0x{:x}) — "
                            "would have walked into unmapped memory",
                            (unsigned long long)cmd->words.w1,
                            (unsigned long long)subAddr);
                Fast::DumpDLDiag(subGFX, "gfx_dl_handler: walked-past");
            }
            return false;
        }
    }

    if (C0(16, 1) == 0) {
        // Push return address
        if (subGFX != nullptr) {
            g_exec_stack.call(*cmd0, subGFX);
        }
    } else {
        (*cmd0) = subGFX;
        g_exec_stack.branch(cmd);
        return true; // shortcut cmd increment
    }
    return false;
}

bool gfx_dl_otr_hash_handler_custom(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;
    if (C0(16, 1) == 0) {
        // Push return address
        (*cmd0)++;

        uint64_t hash = ((uint64_t)(*cmd0)->words.w0 << 32) + (*cmd0)->words.w1;

        F3DGfx* gfx = (F3DGfx*)Ship::Context::GetInstance()->GetResourceManager()->GetResourceRawPointer(hash);

        if (gfx != 0) {
            g_exec_stack.call(cmd, gfx);
        }
    } else {
        Interpreter* gfx = mInstance.lock().get();
        assert(0 && "????");
        (*cmd0) = (F3DGfx*)gfx->SegAddr((*cmd0)->words.w1);
        return true;
    }
    return false;
}
bool gfx_dl_index_handler(F3DGfx** cmd0) {
    // Compute seg addr by converting an index value to a offset value
    // handling 32 vs 64 bit size differences for Gfx
    // adding 1 to trigger the segaddr flow
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = (*cmd0);
    uint8_t segNum = (uint8_t)(cmd->words.w1 >> 24);
    uint32_t index = (uint32_t)(cmd->words.w1 & 0x00FFFFFF);
    uintptr_t segmentBase = (segNum < MAX_SEGMENT_POINTERS) ? gfx->mSegmentPointers[segNum] : 0;
    uintptr_t segAddr = (segNum << 24) | (index * portGetDisplayListStride(segmentBase)) + 1;

    F3DGfx* subGFX = (F3DGfx*)gfx->SegAddr(segAddr);
    if (C0(16, 1) == 0) {
        // Push return address
        if (subGFX != nullptr) {
            g_exec_stack.call((*cmd0), subGFX);
        }
    } else {
        (*cmd0) = subGFX;
        g_exec_stack.branch(cmd);
        return true; // shortcut cmd increment
    }
    return false;
}

// TODO handle special OTR opcodes later...
bool gfx_pushcd_handler_custom(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    gfx_push_current_dir((char*)gfx->SegAddr((*cmd0)->words.w1));
    return false;
}

// TODO handle special OTR opcodes later...
bool gfx_branch_z_otr_handler_f3dex2(F3DGfx** cmd0) {
    // Push return address
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = (*cmd0);

    uint8_t vbidx = (uint8_t)((*cmd0)->words.w0 & 0x00000FFF);
    uint32_t zval = (uint32_t)((*cmd0)->words.w1);

    (*cmd0)++;

    if (gfx->mRsp->loaded_vertices[vbidx].z <= zval ||
        (gfx->mRsp->extra_geometry_mode & G_EX_ALWAYS_EXECUTE_BRANCH) != 0) {
        uint64_t hash = ((uint64_t)(*cmd0)->words.w0 << 32) + (*cmd0)->words.w1;

        F3DGfx* gfx = (F3DGfx*)Ship::Context::GetInstance()->GetResourceManager()->GetResourceRawPointer(hash);

        if (gfx != 0) {
            (*cmd0) = gfx;
            g_exec_stack.branch(cmd);
            return true; // shortcut cmd increment
        }
    }
    return false;
}

// F3D, F3DEX, and F3DEX2 do the same thing but F3DEX2 has its own opcode number
bool gfx_end_dl_handler_common(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    gfx->mMarkerOn = false;

    g_exec_stack.ret();
    return true;
}

bool gfx_set_prim_depth_handler_rdp(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;
    // gDPSetPrimDepth packs `(z, dz)` as `_SHIFTL(z,16,16) | _SHIFTL(dz,0,16)`
    // into words.w1; both are u16 N64 Z-buffer values. Stored here for later
    // consumption when a draw uses other_mode_l & G_ZS_PRIM.
    gfx->mRdp->prim_depth_z = (uint16_t)((cmd->words.w1 >> 16) & 0xFFFF);
    gfx->mRdp->prim_depth_dz = (uint16_t)(cmd->words.w1 & 0xFFFF);
    return false;
}

// Only on F3DEX2
bool gfx_geometry_mode_handler_f3dex2(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxSpGeometryMode(~C0(0, 24), (uint32_t)cmd->words.w1);
    return false;
}

// Only on F3DEX and older
bool gfx_set_geometry_mode_handler_f3d(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxSpGeometryMode(0, (uint32_t)cmd->words.w1);
    return false;
}

// Only on F3DEX and older
bool gfx_clear_geometry_mode_handler_f3d(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxSpGeometryMode((uint32_t)cmd->words.w1, 0);
    return false;
}

bool gfx_tri1_otr_handler_f3dex2(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();

    F3DGfx* cmd = *cmd0;
    uint8_t v00 = (uint8_t)(cmd->words.w0 & 0x0000FFFF);
    uint8_t v01 = (uint8_t)(cmd->words.w1 >> 16);
    uint8_t v02 = (uint8_t)(cmd->words.w1 & 0x0000FFFF);
    gfx->GfxSpTri1(v00, v01, v02, false);

    return false;
}

bool gfx_tri1_handler_f3dex2(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxSpTri1(C0(16, 8) / 2, C0(8, 8) / 2, C0(0, 8) / 2, false);

    return false;
}

bool gfx_tri1_handler_f3dex(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxSpTri1(C1(17, 7), C1(9, 7), C1(1, 7), false);

    return false;
}

bool gfx_tri1_handler_f3d(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxSpTri1(C1(16, 8) / 10, C1(8, 8) / 10, C1(0, 8) / 10, false);

    return false;
}

// F3DEX, and F3DEX2 share a tri2 function, however F3DEX has a different quad function.
bool gfx_tri2_handler_f3dex(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxSpTri1(C0(17, 7), C0(9, 7), C0(1, 7), false);
    gfx->GfxSpTri1(C1(17, 7), C1(9, 7), C1(1, 7), false);
    return false;
}

bool gfx_quad_handler_f3dex2(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxSpTri1(C0(16, 8) / 2, C0(8, 8) / 2, C0(0, 8) / 2, false);
    gfx->GfxSpTri1(C1(16, 8) / 2, C1(8, 8) / 2, C1(0, 8) / 2, false);
    return false;
}

bool gfx_quad_handler_f3dex(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxSpTri1(C1(16, 8) / 2, C1(8, 8) / 2, C1(0, 8) / 2, false);
    gfx->GfxSpTri1(C1(16, 8) / 2, C1(0, 8) / 2, C1(24, 8) / 2, false);
    return false;
}

bool gfx_othermode_l_handler_f3dex2(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxSpSetOtherMode(31 - C0(8, 8) - C0(0, 8), C0(0, 8) + 1, cmd->words.w1);

    return false;
}

bool gfx_othermode_l_handler_f3d(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxSpSetOtherMode(C0(8, 8), C0(0, 8), cmd->words.w1);

    return false;
}

bool gfx_othermode_h_handler_f3dex2(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxSpSetOtherMode(63 - C0(8, 8) - C0(0, 8), C0(0, 8) + 1, (uint64_t)cmd->words.w1 << 32);

    return false;
}

// Only on F3DEX and older
bool gfx_set_geometry_mode_handler_f3dex(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxSpGeometryMode(0, (uint32_t)cmd->words.w1);
    return false;
}

// Only on F3DEX and older
bool gfx_clear_geometry_mode_handler_f3dex(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxSpGeometryMode((uint32_t)cmd->words.w1, 0);
    return false;
}

bool gfx_othermode_h_handler_f3d(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxSpSetOtherMode(C0(8, 8) + 32, C0(0, 8), (uint64_t)cmd->words.w1 << 32);

    return false;
}

bool gfx_set_timg_handler_rdp(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;
    uintptr_t i = (uintptr_t)gfx->SegAddr(cmd->words.w1);
    char* imgData = (char*)i;
    uint32_t texFlags = 0;
    RawTexMetadata rawTexMetdata = {};

    if ((i & 1) != 1) {
        if (gfx_check_image_signature(imgData) == 1) {
            std::shared_ptr<Fast::Texture> tex = std::static_pointer_cast<Fast::Texture>(
                Ship::Context::GetInstance()->GetResourceManager()->LoadResourceProcess(imgData));

            if (tex == nullptr) {
                (*cmd0)++;
                return false;
            }

            i = (uintptr_t) reinterpret_cast<char*>(tex->ImageData);
            texFlags = tex->Flags;
            rawTexMetdata.width = tex->Width;
            rawTexMetdata.height = tex->Height;
            rawTexMetdata.h_byte_scale = tex->HByteScale;
            rawTexMetdata.v_pixel_scale = tex->VPixelScale;
            rawTexMetdata.type = tex->Type;
            rawTexMetdata.resource = tex;
        }
    }

    // If the resolved address is still in the N64 segmented range, SegAddr
    // failed to resolve it (segment not set up). Skip to avoid dereferencing
    // invalid memory. Don't widen the cap to 4 GB on 64-bit hosts — non-PIE
    // Linux binaries hand out valid brk-arena pointers below 4 GB, and a
    // wider guard drops legitimate texture-set commands.
    if (i <= 0x0FFFFFFF) {
        return false;
    }

    gfx->GfxDpSetTextureImage(C0(21, 3), C0(19, 2), C0(0, 12) + 1, imgData, texFlags, rawTexMetdata, (void*)i);

    return false;
}

bool gfx_set_timg_otr_hash_handler_custom(F3DGfx** cmd0) {
    uintptr_t addr = (*cmd0)->words.w1;
    (*cmd0)++;
    uint64_t hash = ((uint64_t)(*cmd0)->words.w0 << 32) + (uint64_t)(*cmd0)->words.w1;

    const char* fileName = Ship::Context::GetInstance()->GetResourceManager()->GetArchiveManager()->HashToCString(hash);
    uint32_t texFlags = 0;
    RawTexMetadata rawTexMetadata = {};

    if (fileName == nullptr) {
        (*cmd0)++;
        return false;
    }

    std::shared_ptr<Fast::Texture> texture =
        std::static_pointer_cast<Fast::Texture>(Ship::Context::GetInstance()->GetResourceManager()->LoadResourceProcess(
            Ship::Context::GetInstance()->GetResourceManager()->GetArchiveManager()->HashToCString(hash)));
    if (texture != nullptr) {
        texFlags = texture->Flags;
        rawTexMetadata.width = texture->Width;
        rawTexMetadata.height = texture->Height;
        rawTexMetadata.h_byte_scale = texture->HByteScale;
        rawTexMetadata.v_pixel_scale = texture->VPixelScale;
        rawTexMetadata.type = texture->Type;
        rawTexMetadata.resource = texture;

        // OTRTODO: We have disabled caching for now to fix a texture corruption issue with HD texture
        // support. In doing so, there is a potential performance hit since we are not caching lookups. We
        // need to do proper profiling to see whether or not it is worth it to keep the caching system.

        char* tex = reinterpret_cast<char*>(texture->ImageData);

        if (tex != nullptr) {
            (*cmd0)--;
            uintptr_t oldData = (*cmd0)->words.w1;
            // TODO: wtf??
            (*cmd0)->words.w1 = (uintptr_t)tex;

            // if (ourHash != (uint64_t)-1) {
            //     auto res = ResourceLoad(ourHash);
            // }

            (*cmd0)++;
        }

        (*cmd0)--;
        F3DGfx* cmd = (*cmd0);
        uint32_t fmt = C0(21, 3);
        uint32_t size = C0(19, 2);
        uint32_t width = C0(0, 12) + 1;

        if (tex != NULL) {
            Interpreter* gfx = mInstance.lock().get();
            gfx->GfxDpSetTextureImage(fmt, size, width, fileName, texFlags, rawTexMetadata, tex);
        }
    } else {
        SPDLOG_ERROR("G_SETTIMG_OTR_HASH: Texture is null");
    }

    (*cmd0)++;
    return false;
}

bool gfx_set_timg_otr_filepath_handler_custom(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;
    const char* fileName = (const char*)gfx->SegAddr(cmd->words.w1);

    uint32_t texFlags = 0;
    RawTexMetadata rawTexMetadata = {};

    std::shared_ptr<Fast::Texture> texture = std::static_pointer_cast<Fast::Texture>(
        Ship::Context::GetInstance()->GetResourceManager()->LoadResourceProcess(fileName));
    if (texture != nullptr) {
        texFlags = texture->Flags;
        rawTexMetadata.width = texture->Width;
        rawTexMetadata.height = texture->Height;
        rawTexMetadata.h_byte_scale = texture->HByteScale;
        rawTexMetadata.v_pixel_scale = texture->VPixelScale;
        rawTexMetadata.type = texture->Type;
        rawTexMetadata.resource = texture;

        uint32_t fmt = C0(21, 3);
        uint32_t size = C0(19, 2);
        uint32_t width = C0(0, 12) + 1;

        gfx->GfxDpSetTextureImage(fmt, size, width, fileName, texFlags, rawTexMetadata,
                                  reinterpret_cast<char*>(texture->ImageData));
    } else {
        SPDLOG_ERROR("G_SETTIMG_OTR_FILEPATH: Texture is null");
    }
    return false;
}

bool gfx_set_fb_handler_custom(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;
    Interpreter* gfx = mInstance.lock().get();
    gfx->Flush();

    if (cmd->words.w1) {
        gfx->SetFrameBuffer((int32_t)cmd->words.w1, 1.0f);
        gfx->mActiveFrameBuffer = gfx->mFrameBuffers.find((int32_t)cmd->words.w1);
        gfx->mFbActive = true;
    } else {
        gfx->ResetFrameBuffer();
        gfx->mFbActive = false;
        gfx->mActiveFrameBuffer = gfx->mFrameBuffers.end();
    }
    return false;
}

bool gfx_reset_fb_handler_custom(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    gfx->Flush();
    gfx->mFbActive = false;
    gfx->mActiveFrameBuffer = gfx->mFrameBuffers.end();
    gfx->mRapi->StartDrawToFramebuffer(gfx->mRendersToFb ? gfx->mGameFb : 0,
                                       (float)gfx->mCurDimensions.height / gfx->mNativeDimensions.height);
    // Force viewport and scissor to reapply against the main framebuffer, in case a previous smaller
    // framebuffer truncated the values
    gfx->mRdp->viewport_or_scissor_changed = true;
    gfx->mRenderingState.viewport = {};
    gfx->mRenderingState.scissor = {};
    return false;
}

bool gfx_copy_fb_handler_custom(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;
    bool* hasCopiedPtr = (bool*)gfx->SegAddr(cmd->words.w1);

    gfx->Flush();
    gfx->CopyFrameBuffer(C0(11, 11), C0(0, 11), (bool)C0(22, 1), hasCopiedPtr);
    return false;
}

bool gfx_read_fb_handler_custom(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    int32_t width, height;
    [[maybe_unused]] int32_t ulx, uly;
    uint16_t* rgba16Buffer = (uint16_t*)gfx->SegAddr(cmd->words.w1);
    int fbId = C0(0, 8);
    bool bswap = C0(8, 1);
    ++(*cmd0);
    cmd = *cmd0;
    // Specifying the upper left origin value is unused and unsupported at the renderer level
    ulx = C0(0, 16);
    uly = C0(16, 16);
    width = C1(0, 16);
    height = C1(16, 16);

    gfx->Flush();
    gfx->mRapi->ReadFramebufferToCPU(fbId, width, height, rgba16Buffer);

#ifndef IS_BIGENDIAN
    // byteswap the output to BE
    if (bswap) {
        for (size_t i = 0; i < (size_t)width * height; i++) {
            rgba16Buffer[i] = BE16SWAP(rgba16Buffer[i]);
        }
    }
#endif

    return false;
}

bool gfx_register_blended_texture_handler_custom(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    // Flush incase we are replacing a previous blended texture that hasn't been finialized to the GPU
    gfx->Flush();

    char* timg = (char*)gfx->SegAddr(cmd->words.w1);

    ++(*cmd0);
    cmd = *cmd0;

    uint8_t* mask = (uint8_t*)gfx->SegAddr(cmd->words.w0);
    uint8_t* replacementTex = (uint8_t*)gfx->SegAddr(cmd->words.w1);

    if (!gfx_check_image_signature(timg)) {
        SPDLOG_ERROR("OTR_G_REGBLENDEDTEX: invalid texture resource pointer cmd={} timg={} mask={} replacement={}",
                     fmt::ptr(cmd), fmt::ptr(timg), fmt::ptr(mask), fmt::ptr(replacementTex));
        return false;
    }

    // With no mask, we should clear the blended texture
    if (mask == nullptr) {
        gfx->UnregisterBlendedTexture(timg);
    } else {
        gfx->RegisterBlendedTexture(timg, mask, replacementTex);
    }

    return false;
}

bool gfx_set_timg_fb_handler_custom(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->Flush();
    gfx->mRapi->SelectTextureFb((uint32_t)cmd->words.w1);
    gfx->mRdp->textures_changed[0] = false;
    gfx->mRdp->textures_changed[1] = false;
    return false;
}

bool gfx_set_grayscale_handler_custom(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->mRdp->grayscale = cmd->words.w1;
    return false;
}

bool gfx_load_block_handler_rdp(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxDpLoadBlock(C1(24, 3), C0(12, 12), C0(0, 12), C1(12, 12), C1(0, 12));
    return false;
}

bool gfx_load_tile_handler_rdp(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxDpLoadTile(C1(24, 3), C0(12, 12), C0(0, 12), C1(12, 12), C1(0, 12));
    return false;
}

bool gfx_set_tile_handler_rdp(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxDpSetTile(C0(21, 3), C0(19, 2), C0(9, 9), C0(0, 9), C1(24, 3), C1(20, 4), C1(18, 2), C1(14, 4), C1(10, 4),
                      C1(8, 2), C1(4, 4), C1(0, 4));
    return false;
}

bool gfx_set_tile_size_handler_rdp(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxDpSetTileSize(C1(24, 3), C0(12, 12), C0(0, 12), C1(12, 12), C1(0, 12));
    return false;
}

bool gfx_set_tile_size_interp_handler_rdp(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;
    Interpreter* gfx = mInstance.lock().get();

    if (gfx->mInterpolationIndex == gfx->mInterpolationIndexTarget) {
        int tile = C1(24, 3);
        gfx->GfxDpSetTileSize(C1(24, 3), C0(12, 12), C0(0, 12), C1(12, 12), C1(0, 12));
        ++(*cmd0);
        memcpy(&gfx->mRdp->texture_tile[tile].uls, &(*cmd0)->words.w0, sizeof(float));
        memcpy(&gfx->mRdp->texture_tile[tile].ult, &(*cmd0)->words.w1, sizeof(float));
        ++(*cmd0);
        memcpy(&gfx->mRdp->texture_tile[tile].lrs, &(*cmd0)->words.w0, sizeof(float));
        memcpy(&gfx->mRdp->texture_tile[tile].lrt, &(*cmd0)->words.w1, sizeof(float));
    } else {
        ++(*cmd0);
        ++(*cmd0);
    }

    return false;
}

bool gfx_set_interpolation_index_target(F3DGfx** cmd0) {
    F3DGfx* cmd = *cmd0;
    Interpreter* gfx = mInstance.lock().get();

    gfx->mInterpolationIndexTarget = cmd->words.w1;
    return false;
}

bool gfx_load_tlut_handler_rdp(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxDpLoadTlut(C1(24, 3), C1(14, 10));
    return false;
}

bool gfx_set_env_color_handler_rdp(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxDpSetEnvColor(C1(24, 8), C1(16, 8), C1(8, 8), C1(0, 8));
    return false;
}

bool gfx_set_prim_color_handler_rdp(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxDpSetPrimColor(C0(8, 8), C0(0, 8), C1(24, 8), C1(16, 8), C1(8, 8), C1(0, 8));
    return false;
}

bool gfx_set_fog_color_handler_rdp(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxDpSetFogColor(C1(24, 8), C1(16, 8), C1(8, 8), C1(0, 8));
    return false;
}

bool gfx_set_blend_color_handler_rdp(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxDpSetBlendColor(C1(24, 8), C1(16, 8), C1(8, 8), C1(0, 8));
    return false;
}

bool gfx_set_fill_color_handler_rdp(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxDpSetFillColor((uint32_t)cmd->words.w1);
    return false;
}

bool gfx_set_intensity_handler_custom(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxDpSetGrayscaleColor(C1(24, 8), C1(16, 8), C1(8, 8), C1(0, 8));
    return false;
}

bool gfx_set_combine_handler_rdp(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;

    gfx->GfxDpSetCombineMode(
        color_comb(C0(20, 4), C1(28, 4), C0(15, 5), C1(15, 3)), alpha_comb(C0(12, 3), C1(12, 3), C0(9, 3), C1(9, 3)),
        color_comb(C0(5, 4), C1(24, 4), C0(0, 5), C1(6, 3)), alpha_comb(C1(21, 3), C1(3, 3), C1(18, 3), C1(0, 3)));
    return false;
}

bool gfx_tex_rect_and_flip_handler_rdp(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;
    int8_t opcode = (int8_t)(cmd->words.w0 >> 24);
    int32_t lrx, lry, tile, ulx, uly;
    uint32_t uls, ult, dsdx, dtdy;

    lrx = C0(12, 12);
    lry = C0(0, 12);
    tile = C1(24, 3);
    ulx = C1(12, 12);
    uly = C1(0, 12);
    // TODO make sure I don't need to increment cmd0
    ++(*cmd0);
    cmd = *cmd0;
    uls = C1(16, 16);
    ult = C1(0, 16);
    ++(*cmd0);
    cmd = *cmd0;
    dsdx = C1(16, 16);
    dtdy = C1(0, 16);

    gfx->GfxDpTextureRectangle(ulx, uly, lrx, lry, tile, uls, ult, dsdx, dtdy, opcode == RDP_G_TEXRECTFLIP);
    return false;
}

bool gfx_tex_rect_wide_handler_custom(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;
    int8_t opcode = (int8_t)(cmd->words.w0 >> 24);
    int32_t lrx, lry, tile, ulx, uly;
    uint32_t uls, ult, dsdx, dtdy;

    lrx = static_cast<int32_t>((C0(0, 24) << 8)) >> 8;
    lry = static_cast<int32_t>((C1(0, 24) << 8)) >> 8;
    tile = C1(24, 3);
    ++(*cmd0);
    cmd = *cmd0;
    ulx = static_cast<int32_t>((C0(0, 24) << 8)) >> 8;
    uly = static_cast<int32_t>((C1(0, 24) << 8)) >> 8;
    ++(*cmd0);
    cmd = *cmd0;
    uls = C0(16, 16);
    ult = C0(0, 16);
    dsdx = C1(16, 16);
    dtdy = C1(0, 16);
    gfx->GfxDpTextureRectangle(ulx, uly, lrx, lry, tile, uls, ult, dsdx, dtdy, opcode == RDP_G_TEXRECTFLIP);
    return false;
}

bool gfx_image_rect_handler_custom(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *cmd0;
    int16_t tile, iw, ih;
    int16_t x0, y0, s0, t0;
    int16_t x1, y1, s1, t1;
    tile = C0(0, 3);
    iw = C1(16, 16);
    ih = C1(0, 16);
    cmd = ++(*cmd0);
    x0 = C0(16, 16);
    y0 = C0(0, 16);
    s0 = C1(16, 16);
    t0 = C1(0, 16);
    cmd = ++(*cmd0);
    x1 = C0(16, 16);
    y1 = C0(0, 16);
    s1 = C1(16, 16);
    t1 = C1(0, 16);
    gfx->GfxDpImageRectangle(tile, iw, ih, x0, y0, s0, t0, x1, y1, s1, t1);

    return false;
}

bool gfx_fill_rect_handler_rdp(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *(cmd0);

    gfx->GfxDpFillRectangle(C1(12, 12), C1(0, 12), C0(12, 12), C0(0, 12));
    return false;
}

bool gfx_fill_wide_rect_handler_custom(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *(cmd0);
    int32_t lrx, lry, ulx, uly;

    lrx = (int32_t)(C0(0, 24) << 8) >> 8;
    lry = (int32_t)(C1(0, 24) << 8) >> 8;
    cmd = ++(*cmd0);
    ulx = (int32_t)(C0(0, 24) << 8) >> 8;
    uly = (int32_t)(C1(0, 24) << 8) >> 8;
    gfx->GfxDpFillRectangle(ulx, uly, lrx, lry);

    return false;
}

bool gfx_SetScissor_handler_rdp(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *(cmd0);

    gfx->GfxDpSetScissor(C1(24, 2), C0(12, 12), C0(0, 12), C1(12, 12), C1(0, 12));
    return false;
}

bool gfx_set_z_img_handler_rdp(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *(cmd0);

    gfx->GfxDpSetZImage(gfx->SegAddr(cmd->words.w1));
    return false;
}

bool gfx_set_c_img_handler_rdp(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *(cmd0);

    gfx->GfxDpSetColorImage(C0(21, 3), C0(19, 2), C0(0, 11), gfx->SegAddr(cmd->words.w1));
    return false;
}

bool gfx_rdp_set_other_mode_rdp(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *(cmd0);

    gfx->GfxDpSetOtherMode(C0(0, 24), (uint32_t)cmd->words.w1);
    return false;
}

bool gfx_bg_copy_handler_s2dex(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *(cmd0);

    if (!gfx->mMarkerOn) {
        gfx->Gfxs2dexBgCopy((F3DuObjBg*)cmd->words.w1); // not gfx->SegAddr here it seems
    }
    return false;
}

bool gfx_bg_1cyc_handler_s2dex(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *(cmd0);

    gfx->Gfxs2dexBg1cyc((F3DuObjBg*)cmd->words.w1);
    return false;
}

bool gfx_obj_rectangle_handler_s2dex(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *(cmd0);

    if (!gfx->mMarkerOn) {
        gfx->Gfxs2dexRecyCopy((F3DuObjSprite*)cmd->words.w1); // not gfx->SegAddr here it seems
    }
    return false;
}

bool gfx_extra_geometry_mode_handler_custom(F3DGfx** cmd0) {
    Interpreter* gfx = mInstance.lock().get();
    F3DGfx* cmd = *(cmd0);

    gfx->GfxSpExtraGeometryMode(~C0(0, 24), (uint32_t)cmd->words.w1);
    return false;
}

bool gfx_stubbed_command_handler(F3DGfx** cmd0) {
    return false;
}

bool gfx_spnoop_command_handler_f3dex2(F3DGfx** cmd0) {
    return false;
}

class UcodeHandler {
  public:
    inline constexpr UcodeHandler(
        std::initializer_list<std::pair<int8_t, std::pair<const char*, GfxOpcodeHandlerFunc>>> initializer) {
        std::fill(std::begin(mHandlers), std::end(mHandlers),
                  std::pair<const char*, GfxOpcodeHandlerFunc>(nullptr, nullptr));

        for (const auto& [opcode, handler] : initializer) {
            mHandlers[static_cast<uint8_t>(opcode)] = handler;
        }
    }

    inline bool contains(int8_t opcode) const {
        return mHandlers[static_cast<uint8_t>(opcode)].first != nullptr;
    }

    inline std::pair<const char*, GfxOpcodeHandlerFunc> at(int8_t opcode) const {
        return mHandlers[static_cast<uint8_t>(opcode)];
    }

  private:
    std::pair<const char*, GfxOpcodeHandlerFunc> mHandlers[std::numeric_limits<uint8_t>::max() + 1];
};

static constexpr UcodeHandler rdpHandlers = {
    { RDP_G_SETTARGETINTERPINDEX,
      { "G_SETTARGETINTERPINDEX", gfx_set_interpolation_index_target } }, // G_SETTARGETINTERPINDEX
    { RDP_G_SETTILESIZE_INTERP,
      { "G_SETTILESIZE_INTERP", gfx_set_tile_size_interp_handler_rdp } },            // G_SETTILESIZE_INTERP
    { RDP_G_TEXRECT, { "G_TEXRECT", gfx_tex_rect_and_flip_handler_rdp } },           // G_TEXRECT (-28)
    { RDP_G_TEXRECTFLIP, { "G_TEXRECTFLIP", gfx_tex_rect_and_flip_handler_rdp } },   // G_TEXRECTFLIP (-27)
    { RDP_G_RDPLOADSYNC, { "mRdpLOADSYNC", gfx_stubbed_command_handler } },          // mRdpLOADSYNC (-26)
    { RDP_G_RDPPIPESYNC, { "mRdpPIPESYNC", gfx_stubbed_command_handler } },          // mRdpPIPESYNC (-25)
    { RDP_G_RDPTILESYNC, { "mRdpTILESYNC", gfx_stubbed_command_handler } },          // mRdpPIPESYNC (-24)
    { RDP_G_RDPFULLSYNC, { "mRdpFULLSYNC", gfx_stubbed_command_handler } },          // mRdpFULLSYNC (-23)
    { RDP_G_SETSCISSOR, { "G_SETSCISSOR", gfx_SetScissor_handler_rdp } },            // G_SETSCISSOR (-19)
    { RDP_G_SETPRIMDEPTH, { "G_SETPRIMDEPTH", gfx_set_prim_depth_handler_rdp } },    // G_SETPRIMDEPTH (-18)
    { RDP_G_SETCONVERT, { "G_SETCONVERT", gfx_stubbed_command_handler } },            // G_SETCONVERT (-20)
    { RDP_G_RDPSETOTHERMODE, { "mRdpSETOTHERMODE", gfx_rdp_set_other_mode_rdp } },   // mRdpSETOTHERMODE (-17)
    { RDP_G_LOADTLUT, { "G_LOADTLUT", gfx_load_tlut_handler_rdp } },                 // G_LOADTLUT (-16)
    { RDP_G_SETTILESIZE, { "G_SETTILESIZE", gfx_set_tile_size_handler_rdp } },       // G_SETTILESIZE (-14)
    { RDP_G_LOADBLOCK, { "G_LOADBLOCK", gfx_load_block_handler_rdp } },              // G_LOADBLOCK (-13)
    { RDP_G_LOADTILE, { "G_LOADTILE", gfx_load_tile_handler_rdp } },                 // G_LOADTILE (-12)
    { RDP_G_SETTILE, { "G_SETTILE", gfx_set_tile_handler_rdp } },                    // G_SETTILE (-11)
    { RDP_G_FILLRECT, { "G_FILLRECT", gfx_fill_rect_handler_rdp } },                 // G_FILLRECT (-10)
    { RDP_G_SETFILLCOLOR, { "G_SETFILLCOLOR", gfx_set_fill_color_handler_rdp } },    // G_SETFILLCOLOR (-9)
    { RDP_G_SETFOGCOLOR, { "G_SETFOGCOLOR", gfx_set_fog_color_handler_rdp } },       // G_SETFOGCOLOR (-8)
    { RDP_G_SETBLENDCOLOR, { "G_SETBLENDCOLOR", gfx_set_blend_color_handler_rdp } }, // G_SETBLENDCOLOR (-7)
    { RDP_G_SETPRIMCOLOR, { "G_SETPRIMCOLOR", gfx_set_prim_color_handler_rdp } },    // G_SETPRIMCOLOR (-6)
    { RDP_G_SETENVCOLOR, { "G_SETENVCOLOR", gfx_set_env_color_handler_rdp } },       // G_SETENVCOLOR (-5)
    { RDP_G_SETCOMBINE, { "G_SETCOMBINE", gfx_set_combine_handler_rdp } },           // G_SETCOMBINE (-4)
    { RDP_G_SETTIMG, { "G_SETTIMG", gfx_set_timg_handler_rdp } },                    // G_SETTIMG (-3)
    { RDP_G_SETZIMG, { "G_SETZIMG", gfx_set_z_img_handler_rdp } },                   // G_SETZIMG (-2)
    { RDP_G_SETCIMG, { "G_SETCIMG", gfx_set_c_img_handler_rdp } },                   // G_SETCIMG (-1)
};

static constexpr UcodeHandler otrHandlers = {
    { OTR_G_SETTIMG_OTR_HASH,
      { "G_SETTIMG_OTR_HASH", gfx_set_timg_otr_hash_handler_custom } },       // G_SETTIMG_OTR_HASH (0x20)
    { OTR_G_SETFB, { "G_SETFB", gfx_set_fb_handler_custom } },                // G_SETFB (0x21)
    { OTR_G_RESETFB, { "G_RESETFB", gfx_reset_fb_handler_custom } },          // G_RESETFB (0x22)
    { OTR_G_SETTIMG_FB, { "G_SETTIMG_FB", gfx_set_timg_fb_handler_custom } }, // G_SETTIMG_FB (0x23)
    { OTR_G_VTX_OTR_FILEPATH,
      { "G_VTX_OTR_FILEPATH", gfx_vtx_otr_filepath_handler_custom } }, // G_VTX_OTR_FILEPATH (0x24)
    { OTR_G_SETTIMG_OTR_FILEPATH,
      { "G_SETTIMG_OTR_FILEPATH", gfx_set_timg_otr_filepath_handler_custom } }, // G_SETTIMG_OTR_FILEPATH (0x25)
    { OTR_G_TRI1_OTR, { "G_TRI1_OTR", gfx_tri1_otr_handler_f3dex2 } },          // G_TRI1_OTR (0x26)
    { OTR_G_DL_OTR_FILEPATH, { "G_DL_OTR_FILEPATH", gfx_dl_otr_filepath_handler_custom } }, // G_DL_OTR_FILEPATH (0x27)
    { OTR_G_PUSHCD, { "G_PUSHCD", gfx_pushcd_handler_custom } },                            // G_PUSHCD (0x28)
    { OTR_G_MTX_OTR_FILEPATH,
      { "G_MTX_OTR_FILEPATH", gfx_mtx_otr_filepath_handler_custom } },          // G_MTX_OTR_FILEPATH (0x29)
    { OTR_G_DL_OTR_HASH, { "G_DL_OTR_HASH", gfx_dl_otr_hash_handler_custom } }, // G_DL_OTR_HASH (0x31)
    { OTR_G_VTX_OTR_HASH, { "G_VTX_OTR_HASH", gfx_vtx_hash_handler_custom } },  // G_VTX_OTR_HASH (0x32)
    { OTR_G_MARKER, { "G_MARKER", gfx_marker_handler_otr } },                   // G_MARKER (0X33)
    { OTR_G_INVALTEXCACHE, { "G_INVALTEXCACHE", gfx_invalidate_tex_cache_handler_f3dex2 } }, // G_INVALTEXCACHE (0X34)
    { OTR_G_BRANCH_Z_OTR, { "G_BRANCH_Z_OTR", gfx_branch_z_otr_handler_f3dex2 } },           // G_BRANCH_Z_OTR (0x35)
    { OTR_G_MTX_OTR, { "G_MTX_OTR", gfx_mtx_otr_handler_custom } },                          // G_MTX_OTR (0x36)
    { OTR_G_TEXRECT_WIDE, { "G_TEXRECT_WIDE", gfx_tex_rect_wide_handler_custom } },          // G_TEXRECT_WIDE (0x37)
    { OTR_G_FILLWIDERECT, { "G_FILLWIDERECT", gfx_fill_wide_rect_handler_custom } },         // G_FILLWIDERECT (0x38)
    { OTR_G_SETGRAYSCALE, { "G_SETGRAYSCALE", gfx_set_grayscale_handler_custom } },          // G_SETGRAYSCALE (0x39)
    { OTR_G_EXTRAGEOMETRYMODE,
      { "G_EXTRAGEOMETRYMODE", gfx_extra_geometry_mode_handler_custom } }, // G_EXTRAGEOMETRYMODE (0x3a)
    { OTR_G_COPYFB, { "G_COPYFB", gfx_copy_fb_handler_custom } },          // G_COPYFB (0x3b)
    { OTR_G_IMAGERECT, { "G_IMAGERECT", gfx_image_rect_handler_custom } }, // G_IMAGERECT (0x3c)
    { OTR_G_DL_INDEX, { "G_DL_INDEX", gfx_dl_index_handler } },            // G_DL_INDEX (0x3d)
    { OTR_G_READFB, { "G_READFB", gfx_read_fb_handler_custom } },          // G_READFB (0x3e)
    { OTR_G_REGBLENDEDTEX,
      { "G_REGBLENDEDTEX", gfx_register_blended_texture_handler_custom } },         // G_REGBLENDEDTEX (0x3f)
    { OTR_G_SETINTENSITY, { "G_SETINTENSITY", gfx_set_intensity_handler_custom } }, // G_SETINTENSITY (0x40)
    { OTR_G_MOVEMEM_HASH, { "OTR_G_MOVEMEM_HASH", gfx_movemem_handler_otr } },      // OTR_G_MOVEMEM_HASH
    { OTR_G_PUSH_SHADER, { "G_PUSH_SHADER", gfx_push_shader } },
    { OTR_G_POP_SHADER, { "G_POP_SHADER", gfx_pop_shader } },
};

static constexpr UcodeHandler f3dex2Handlers = {
    { F3DEX2_G_NOOP, { "G_NOOP", gfx_noop_handler_f3dex2 } },
    { F3DEX2_G_SPNOOP, { "G_SPNOOP", gfx_noop_handler_f3dex2 } },
    // gSPMvpRecalc — SSB64 emits this before patching the MVP via G_MW_MATRIX
    // moveword writes (see src/sys/objdisplay.c draw types 41-46).  On N64 the
    // microcode marks the MVP cache dirty so the next vertex transform recomputes
    // it from M*P; the patches that follow then overwrite individual MVP slots.
    // Fast3D recomputes MP_matrix eagerly inside GfxSpMatrix, so this command is
    // a no-op for us — the G_MW_MATRIX case in GfxSpMovewordF3dex2 does the
    // actual work.  Without this entry the interpreter spammed [critical]
    // "Unhandled OP code: 0xD5" once per emission.
    { F3DEX2_G_SPECIAL_1, { "G_SPECIAL_1", gfx_noop_handler_f3dex2 } },
    { F3DEX2_G_CULLDL, { "G_CULLDL", gfx_cull_dl_handler_f3dex2 } },
    { F3DEX2_G_MTX, { "G_MTX", gfx_mtx_handler_f3dex2 } },
    { F3DEX2_G_POPMTX, { "G_POPMTX", gfx_pop_mtx_handler_f3dex2 } },
    { F3DEX2_G_MOVEMEM, { "G_MOVEMEM", gfx_movemem_handler_f3dex2 } },
    { F3DEX2_G_MOVEWORD, { "G_MOVEWORD", gfx_moveword_handler_f3dex2 } },
    { F3DEX2_G_TEXTURE, { "G_TEXTURE", gfx_texture_handler_f3dex2 } },
    { F3DEX2_G_VTX, { "G_VTX", gfx_vtx_handler_f3dex2 } },
    { F3DEX2_G_MODIFYVTX, { "G_MODIFYVTX", gfx_modify_vtx_handler_f3dex2 } },
    { F3DEX2_G_DL, { "G_DL", gfx_dl_handler_common } },
    { F3DEX2_G_ENDDL, { "G_ENDDL", gfx_end_dl_handler_common } },
    { F3DEX2_G_GEOMETRYMODE, { "G_GEOMETRYMODE", gfx_geometry_mode_handler_f3dex2 } },
    { F3DEX2_G_TRI1, { "G_TRI1", gfx_tri1_handler_f3dex2 } },
    { F3DEX2_G_TRI2, { "G_TRI2", gfx_tri2_handler_f3dex } },
    { F3DEX2_G_QUAD, { "G_QUAD", gfx_quad_handler_f3dex2 } },
    { F3DEX2_G_SETOTHERMODE_L, { "G_SETOTHERMODE_L", gfx_othermode_l_handler_f3dex2 } },
    { F3DEX2_G_SETOTHERMODE_H, { "G_SETOTHERMODE_H", gfx_othermode_h_handler_f3dex2 } },
    // SSB64 mixes S2DEX BG commands into F3DEX2 display lists without a G_LOAD_UCODE switch.
    // The RSP was already running the right ucode from task setup; on PC we handle them here.
    { F3DEX2_G_BG_1CYC, { "G_BG_1CYC", gfx_bg_1cyc_handler_s2dex } },
    { F3DEX2_G_BG_COPY, { "G_BG_COPY", gfx_bg_copy_handler_s2dex } },
};

static constexpr UcodeHandler f3dexHandlers = {
    { F3DEX_G_NOOP, { "G_NOOP", gfx_noop_handler_f3dex2 } },
    { F3DEX_G_CULLDL, { "G_CULLDL", gfx_cull_dl_handler_f3dex2 } },
    { F3DEX_G_MTX, { "G_MTX", gfx_mtx_handler_f3d } },
    { F3DEX_G_POPMTX, { "G_POPMTX", gfx_pop_mtx_handler_f3d } },
    { F3DEX_G_MOVEMEM, { "G_POPMEM", gfx_movemem_handler_f3d } },
    { F3DEX_G_MOVEWORD, { "G_MOVEWORD", gfx_moveword_handler_f3d } },
    { F3DEX_G_TEXTURE, { "G_TEXTURE", gfx_texture_handler_f3d } },
    { F3DEX_G_SETOTHERMODE_L, { "G_SETOTHERMODE_L", gfx_othermode_l_handler_f3d } },
    { F3DEX_G_SETOTHERMODE_H, { "G_SETOTHERMODE_H", gfx_othermode_h_handler_f3d } },
    { F3DEX_G_SETGEOMETRYMODE, { "G_SETGEOMETRYMODE", gfx_set_geometry_mode_handler_f3dex } },
    { F3DEX_G_CLEARGEOMETRYMODE, { "G_CLEARGEOMETRYMODE", gfx_clear_geometry_mode_handler_f3dex } },
    { F3DEX_G_VTX, { "G_VTX", gfx_vtx_handler_f3dex } },
    { F3DEX_G_TRI1, { "G_TRI1", gfx_tri1_handler_f3dex } },
    { F3DEX_G_MODIFYVTX, { "G_MODIFYVTX", gfx_modify_vtx_handler_f3dex2 } },
    { F3DEX_G_DL, { "G_DL", gfx_dl_handler_common } },
    { F3DEX_G_ENDDL, { "G_ENDDL", gfx_end_dl_handler_common } },
    { F3DEX_G_TRI2, { "G_TRI2", gfx_tri2_handler_f3dex } },
    { F3DEX_G_SPNOOP, { "G_SPNOOP", gfx_spnoop_command_handler_f3dex2 } },
    { F3DEX_G_RDPHALF_1, { "mRdpHALF_1", gfx_stubbed_command_handler } },
    { F3DEX_G_QUAD, { "G_QUAD", gfx_quad_handler_f3dex } },
};

static constexpr UcodeHandler f3dHandlers = {
    { F3DEX_G_NOOP, { "G_NOOP", gfx_noop_handler_f3dex2 } },
    { F3DEX_G_CULLDL, { "G_CULLDL", gfx_cull_dl_handler_f3dex2 } },
    { F3DEX_G_MTX, { "G_MTX", gfx_mtx_handler_f3d } },
    { F3DEX_G_POPMTX, { "G_POPMTX", gfx_pop_mtx_handler_f3d } },
    { F3DEX_G_MOVEMEM, { "G_POPMEM", gfx_movemem_handler_f3d } },
    { F3DEX_G_MOVEWORD, { "G_MOVEWORD", gfx_moveword_handler_f3d } },
    { F3DEX_G_TEXTURE, { "G_TEXTURE", gfx_texture_handler_f3d } },
    { F3DEX_G_SETOTHERMODE_L, { "G_SETOTHERMODE_L", gfx_othermode_l_handler_f3d } },
    { F3DEX_G_SETOTHERMODE_H, { "G_SETOTHERMODE_H", gfx_othermode_h_handler_f3d } },
    { F3DEX_G_SETGEOMETRYMODE, { "G_SETGEOMETRYMODE", gfx_set_geometry_mode_handler_f3dex } },
    { F3DEX_G_CLEARGEOMETRYMODE, { "G_CLEARGEOMETRYMODE", gfx_clear_geometry_mode_handler_f3dex } },
    { F3DEX_G_VTX, { "G_VTX", gfx_vtx_handler_f3d } },
    { F3DEX_G_TRI1, { "G_TRI1", gfx_tri1_handler_f3d } },
    { F3DEX_G_MODIFYVTX, { "G_MODIFYVTX", gfx_modify_vtx_handler_f3dex2 } },
    { F3DEX_G_DL, { "G_DL", gfx_dl_handler_common } },
    { F3DEX_G_ENDDL, { "G_ENDDL", gfx_end_dl_handler_common } },
    { F3DEX_G_TRI2, { "G_TRI2", gfx_tri2_handler_f3dex } },
    { F3DEX_G_SPNOOP, { "G_SPNOOP", gfx_spnoop_command_handler_f3dex2 } },
    { F3DEX_G_RDPHALF_1, { "mRdpHALF_1", gfx_stubbed_command_handler } },
};

// LUSTODO: These S2DEX commands have different opcode numbers on F3DEX2 vs other ucodes. More research needs to be done
// to see if the implementations are different.
static constexpr UcodeHandler s2dexHandlers = {
    { F3DEX2_G_BG_COPY, { "G_BG_COPY", gfx_bg_copy_handler_s2dex } },
    { F3DEX2_G_BG_1CYC, { "G_BG_1CYC", gfx_bg_1cyc_handler_s2dex } },
    { F3DEX2_G_OBJ_RENDERMODE, { "G_OBJ_RENDERMODE", gfx_stubbed_command_handler } },
    { F3DEX2_G_OBJ_RECTANGLE_R, { "G_OBJ_RECTANGLE_R", gfx_stubbed_command_handler } },
    { F3DEX2_G_OBJ_RECTANGLE, { "G_OBJ_RECTANGLE", gfx_obj_rectangle_handler_s2dex } },
    { F3DEX2_G_DL, { "G_DL", gfx_dl_handler_common } },
    { F3DEX2_G_ENDDL, { "G_ENDDL", gfx_end_dl_handler_common } },
};

static constexpr std::array ucode_handlers = {
    &f3dHandlers,    // ucode_f3db
    &f3dHandlers,    // ucode_f3d
    &f3dexHandlers,  // ucode_f3dex
    &f3dexHandlers,  // ucode_f3dexb
    &f3dex2Handlers, // ucode_f3dex2
    &s2dexHandlers,  // ucode_s2dex
};

const char* GfxGetOpcodeName(int8_t opcode) {
    if (otrHandlers.contains(opcode)) {
        return otrHandlers.at(opcode).first;
    }

    if (rdpHandlers.contains(opcode)) {
        return rdpHandlers.at(opcode).first;
    }

    if (ucode_handler_index < ucode_handlers.size()) {
        if (ucode_handlers[ucode_handler_index]->contains(opcode)) {
            return ucode_handlers[ucode_handler_index]->at(opcode).first;
        } else {
            SPDLOG_CRITICAL("Unhandled OP code: 0x{:X}, for loaded ucode: {}", (uint8_t)opcode,
                            (uint32_t)ucode_handler_index);
        }
    } else {
        SPDLOG_CRITICAL("Unhandled OP code: 0x{:X}, invalid ucode: {}", (uint8_t)opcode, (uint32_t)ucode_handler_index);
    }

    return nullptr;
}

// TODO, implement a system where we can get the current opcode handler by writing to the GWords. If the powers that be
// are OK with that...
static void gfx_set_ucode_handler(UcodeHandlers ucode) {
    // Loaded ucode must be in range of the supported ucode_handlers
    assert(ucode < ucode_max);
    Interpreter* gfx = mInstance.lock().get();
    ucode_handler_index = ucode;

    // Reset some RSP state values upon ucode load to deal with hardware quirks discovered by emulators
    switch (ucode) {
        case ucode_f3d:
        case ucode_f3db:
        case ucode_f3dex:
        case ucode_f3dexb:
        case ucode_f3dex2:
            gfx->mRsp->fog_mul = 0;
            gfx->mRsp->fog_offset = 0;
            break;
        default:
            break;
    }
}

static void gfx_step() {
    auto& cmd = g_exec_stack.currCmd();
    auto cmd0 = cmd;
#ifdef PORT_DIAG_HAVE_ASAN
    /* DIAG: PR #133 lead-gap. If the cmd we're about to deref is in poisoned
       memory (typically: stale segment binding to a freed/past-end heap
       address), dump the segment table + recent segment writes + DL pushes
       so we can identify which lead's coverage has the hole. Then let the
       deref proceed — ASan will halt with its own report and the diag dump
       will be in the log just above. */
    if (cmd != nullptr && __asan_region_is_poisoned((void*)cmd, sizeof(*cmd)) != nullptr) {
        diagDumpAll(cmd, "gfx_step: about to deref poisoned cmd");
    }
#endif

    /* Walk-bounds check. The G_DL handler's bounds check (gfx_dl_handler_common
     * above) rejects pushes outside any registered range, but a DL inside a
     * registered range can still walk off the end if it lacks a
     * gsSPEndDisplayList terminator (variant 5: per-MObj material setup DL
     * via segment-0xE stride correction). Catch that here: if cmd has stepped
     * just past the end of any registered range, terminate this frame as if
     * G_ENDDL had been encountered. Pop the frame with `ret()` and return —
     * Run's outer loop will pick up the parent frame (or exit if stack is
     * empty). */
    {
        if (sDLBoundsCheck && sDLBoundsCheck((uintptr_t)cmd) == kDLBoundsWalkedPast) {
            static int sCount = 0;
            if (sCount < 10) {
                sCount++;
                SPDLOG_WARN("gfx_step: cmd 0x{:x} walked past registered DL range "
                            "— stopping entire walk (likely missing gsSPEndDisplayList "
                            "in a game-built DL; parent frames may also be in garbage)",
                            (unsigned long long)(uintptr_t)cmd);
                Fast::DumpDLDiag(cmd, "gfx_step: walked past range");
            }
            /* Stop the ENTIRE walk, not just this frame. A frame that has
             * walked past its source's end was reading garbage opcodes for
             * an unknown duration before we caught it; parent frames may
             * also have consumed garbage (out-of-band G_DL, G_BRANCH).
             * Popping only this frame resumes the parent at a stale cmd
             * and the garbage opcode stream continues — observed
             * `gfx_copy_fb_handler_custom` crashing on bad args this way.
             * Tear down the whole exec stack and let Run's loop exit
             * cleanly; this frame's draws are lost but the next frame
             * recovers. */
            g_exec_stack.stop();
            return;
        }
    }

    int8_t opcode = (int8_t)(cmd->words.w0 >> 24);

    if (sGbiTraceCallback) {
        sGbiTraceCallback((uintptr_t)cmd->words.w0, (uintptr_t)cmd->words.w1,
                          (int)g_exec_stack.cmd_stack.size() - 1);
    }

#ifdef USE_GBI_TRACE
    if (cmd->words.trace.valid &&
        Ship::Context::GetInstance()->GetConsoleVariables()->GetInteger("gEnableGFXTrace", 0)) {
#define TRACE                                  \
    "\n====================================\n" \
    " - CMD: {:02X}\n"                         \
    " - Path: {}:{}\n"                         \
    " - W0: {:08X}\n"                          \
    " - W1: {:08X}\n"                          \
    "===================================="
        SPDLOG_INFO(TRACE, (uint8_t)opcode, cmd->words.trace.file, cmd->words.trace.idx, cmd->words.w0, cmd->words.w1);
    }
#endif

    if (opcode == F3DEX2_G_LOAD_UCODE) {
        gfx_set_ucode_handler((UcodeHandlers)(cmd->words.w0 & 0xFFFFFF));
        ++cmd;
        return;
        // Instead of having a handler for each ucode for switching ucode, just check for it early and return.
    }

    if (otrHandlers.contains(opcode)) {
        // OTR filepath handlers expect w1 to be a valid string pointer.
        // Guard against null or N64-segment addresses that would crash in strlen/strncmp.
        if (opcode == OTR_G_VTX_OTR_FILEPATH || opcode == OTR_G_SETTIMG_OTR_FILEPATH ||
            opcode == OTR_G_DL_OTR_FILEPATH || opcode == OTR_G_PUSHCD || opcode == OTR_G_MTX_OTR_FILEPATH ||
            opcode == OTR_G_PUSH_SHADER || opcode == OTR_G_REGBLENDEDTEX) {
            Interpreter* gfx = mInstance.lock().get();
            uintptr_t resolvedW1 = (uintptr_t)gfx->SegAddr((uintptr_t)cmd->words.w1);
            if (resolvedW1 < 0x10000
#if UINTPTR_MAX > 0xFFFFFFFFu
                // On 64-bit: filter kernel/sentinel addresses.
                || resolvedW1 > 0x0000FFFFFFFFFFFFull
#endif
                || !gfxPointerHasReadableBytes(reinterpret_cast<const void*>(resolvedW1), 8)
            ) {
                ++g_exec_stack.currCmd();
                return;
            }
        }
        if (otrHandlers.at(opcode).second(&cmd)) {
            return;
        }
    } else if (rdpHandlers.contains(opcode)) {
        if (rdpHandlers.at(opcode).second(&cmd)) {
            return;
        }
    } else if (ucode_handler_index < ucode_handlers.size()) {
        if (ucode_handlers[ucode_handler_index]->contains(opcode)) {
            if (ucode_handlers[ucode_handler_index]->at(opcode).second(&cmd)) {
                return;
            }
        } else {
            const PortPackedDisplayListInfo* packedInfo = nullptr;
            size_t packedIndex = 0;

            if (portFindNormalizedDisplayListCommand(cmd, &packedInfo, &packedIndex)) {
                const uintptr_t rawCmd = reinterpret_cast<uintptr_t>(packedInfo->source) + (packedIndex * PORT_PACKED_GFX_SIZE);
                const uintptr_t fileOffset = rawCmd - packedInfo->fileBase;
                uintptr_t describedBase = 0;
                size_t describedSize = 0;
                uint32_t fileId = UINT32_MAX;
                const char* filePath = nullptr;

                portRelocDescribePointer(packedInfo->source, &describedBase, &describedSize, &fileId, &filePath);

                SPDLOG_CRITICAL(
                    "Unhandled OP code: 0x{:X}, for loaded ucode: {} cmd={} w0=0x{:08X} w1=0x{:016X} raw_source={} raw_cmd=0x{:X} file_offset=0x{:X} cmd_index={} file_id={} file_path={} file_base=0x{:X} file_size=0x{:X}",
                    (uint8_t)opcode, (uint32_t)ucode_handler_index, fmt::ptr(cmd), (uint32_t)cmd->words.w0,
                    (uint64_t)cmd->words.w1, fmt::ptr(packedInfo->source), rawCmd, fileOffset, packedIndex, fileId,
                    (filePath != nullptr) ? filePath : "(unknown)", describedBase, describedSize);
            } else {
                SPDLOG_CRITICAL("Unhandled OP code: 0x{:X}, for loaded ucode: {} cmd={} w0=0x{:08X} w1=0x{:016X}",
                                (uint8_t)opcode, (uint32_t)ucode_handler_index, fmt::ptr(cmd),
                                (uint32_t)cmd->words.w0, (uint64_t)cmd->words.w1);
            }
        }
    } else {
        SPDLOG_CRITICAL("Unhandled OP code: 0x{:X}, invalid ucode: {} cmd={} w0=0x{:08X} w1=0x{:016X}",
                        (uint8_t)opcode, (uint32_t)ucode_handler_index, fmt::ptr(cmd), (uint32_t)cmd->words.w0,
                        (uint64_t)cmd->words.w1);
    }

    ++cmd;
}

void Interpreter::SpReset() {
    while (!mShaderStack.empty()) {
        mShaderStack.pop();
    }
    mRsp->modelview_matrix_stack_size = 1;
    mRsp->current_num_lights = 2;
    mRsp->lights_changed = true;
    mRsp->lookat[0].dir[0] = 0;
    mRsp->lookat[0].dir[1] = 127;
    mRsp->lookat[0].dir[2] = 0;
    mRsp->lookat[1].dir[0] = 127;
    mRsp->lookat[1].dir[1] = 0;
    mRsp->lookat[1].dir[2] = 0;
    CalculateNormalDir(&mRsp->lookat[0], mRsp->current_lookat_coeffs[0]);
    CalculateNormalDir(&mRsp->lookat[1], mRsp->current_lookat_coeffs[1]);
    // Clear the SP segment table at the start of each frame.  On real N64,
    // each RSP task loads fresh ucode and starts with no segments mapped;
    // games re-bind segments via gSPSegment within the frame's display list.
    // Without this clear, a segment written in frame N (e.g. lbtransition's
    // gSPSegment(0x1, ...) for VS-results photocopy) survives into frame N+1
    // pointing at heap memory the game has already freed.  Subsequent G_VTX
    // commands that resolve through that stale segment then either deref
    // freed memory (SIGSEGV) or fall through SegAddr unresolved and produce
    // the addr=0x01... fingerprint of issue #103 / #128.
    for (int i = 0; i < MAX_SEGMENT_POINTERS; i++) {
        uintptr_t _old = mSegmentPointers[i];
        mSegmentPointers[i] = 0;
        if (_old != 0) {
            diagRecordSegWrite(i, _old, 0, "SpReset", 0);
        }
    }
    /* DIAG: bump frame counter so log entries line up with frames. SpReset is
       called at the start of every Run/RunGuiOnly invocation. */
    portDiagBumpFrame();
    ResetRdpTextureState();

    /* Reset the shader-tracking stack to balanced state per frame. If a
     * previous frame's walk was aborted by g_exec_stack.stop() (variant 5
     * walked-past guard) between G_PUSH_SHADER and the matching
     * G_POP_SHADER, the stack carries a stray entry. Garbage opcode
     * streams during runaway walks can also push/pop unbalanced. Clearing
     * per frame ensures each frame starts with a known-empty stack. */
    while (!mShaderStack.empty()) mShaderStack.pop();
}

void Interpreter::GetDimensions(uint32_t* width, uint32_t* height, int32_t* posX, int32_t* posY) {
    mWapi->GetDimensions(width, height, posX, posY);
}

void Interpreter::Init(class GfxWindowBackend* wapi, class GfxRenderingAPI* rapi, const char* game_name,
                       bool start_in_fullscreen, uint32_t width, uint32_t height, uint32_t posX, uint32_t posY) {
    mWapi = wapi;
    mRapi = rapi;
    mWapi->Init(game_name, rapi->GetName(), start_in_fullscreen, width, height, posX, posY);
    mRapi->Init();
    mRapi->UpdateFramebufferParameters(0, width, height, 1, false, true, true, true);
    mCurDimensions.internal_mul =
        Ship::Context::GetInstance()->GetConsoleVariables()->GetFloat(CVAR_INTERNAL_RESOLUTION, 1);
    mMsaaLevel = Ship::Context::GetInstance()->GetConsoleVariables()->GetInteger(CVAR_MSAA_VALUE, 1);

    mCurDimensions.width = width;
    mCurDimensions.height = height;

    mGameFb = mRapi->CreateFramebuffer();
    mGameFbMsaaResolved = mRapi->CreateFramebuffer();

    // Post-process pipeline. On backends that don't yet implement
    // post-process this is a no-op — see GfxRenderingAPI::SupportsPostProcess.
    mPostProcessChain.Init(mRapi);

    mNativeDimensions.width = SCREEN_WIDTH;
    mNativeDimensions.height = SCREEN_HEIGHT;

    for (int i = 0; i < MAX_SEGMENT_POINTERS; i++) {
        mSegmentPointers[i] = 0;
    }

    if (mTexUploadBuffer == nullptr) {
        // We cap texture max to 8k, because why would you need more?
        int max_tex_size = std::min(8192, mRapi->GetMaxTextureSize());
        mTexUploadBuffer = (uint8_t*)malloc(max_tex_size * max_tex_size * 4);
    }

    ucode_handler_index = UcodeHandlers::ucode_f3dex2;

    // Pre-allocate texture cache buckets to prevent rehash-induced iterator invalidation.
    mTextureCache.map.reserve(TEXTURE_CACHE_MAX_SIZE);
}

void Interpreter::Destroy() {
    // TODO: should also destroy rapi, and any other resources acquired in fast3d
    free(mTexUploadBuffer);
    mWapi->Destroy();

    // Texture cache and loaded textures store references to Resources which need to be unreferenced.
    TextureCacheClear();
    mRdp->texture_to_load.raw_tex_metadata.resource = nullptr;
    mRdp->loaded_texture[0].raw_tex_metadata.resource = nullptr;
    mRdp->loaded_texture[1].raw_tex_metadata.resource = nullptr;
}

GfxRenderingAPI* Interpreter::GetCurrentRenderingAPI() {
    return mRapi;
}

void Interpreter::HandleWindowEvents() {
    mWapi->HandleEvents();
}

bool Interpreter::IsFrameReady() {
    return mWapi->IsFrameReady();
}

bool Interpreter::ViewportMatchesRendererResolution() {
#ifdef __APPLE__
    // Always treat the viewport as not matching the render resolution on mac
    // to avoid issues with retina scaling.
    return false;
#else
    // Port-driven override: pinning this to false keeps mRendersToFb=true so
    // mGameFb is populated every frame, which the GPU-readback bridge depends
    // on. Without it, Windows D3D11 in the default 1x / no-MSAA config would
    // draw straight to the swap-chain back buffer (FB 0) and the prior
    // gameplay frame would be unrecoverable post-Present.
    if (mForceRenderToFb) {
        return false;
    }
    if (mCurDimensions.width == mGameWindowViewport.width && mCurDimensions.height == mGameWindowViewport.height) {
        return true;
    }
    return false;
#endif
}

void Interpreter::SetForceRenderToFb(bool force) {
    mForceRenderToFb = force;
}

void Interpreter::StartFrame() {
    mWapi->GetDimensions(&mGfxCurrentWindowDimensions.width, &mGfxCurrentWindowDimensions.height, &mCurWindowPosX,
                         &mCurWindowPosY);
    // Reconcile post-process state from CVars before the FBO-size
    // decisions below, since a fresh shader load flips
    // mPostProcessChain.IsActive() true and forces mRendersToFb.
    UpdatePostProcessFromCVars();
    if (mCurDimensions.height == 0) {
        // Avoid division by zero
        mCurDimensions.height = 1;
    }
    mCurDimensions.aspect_ratio = (float)mCurDimensions.width / (float)mCurDimensions.height;

    // Update the framebuffer sizes when the viewport or native dimension changes
    if (mCurDimensions.width != mPrvDimensions.width || mCurDimensions.height != mPrvDimensions.height ||
        mNativeDimensions.width != mPrevNativeDimensions.width ||
        mNativeDimensions.height != mPrevNativeDimensions.height) {

        for (auto& fb : mFrameBuffers) {
            uint32_t width = fb.second.orig_width, height = fb.second.orig_height;
            if (fb.second.resize) {
                AdjustWidthHeightForScale(width, height, fb.second.native_width, fb.second.native_height);
            }
            if (width != fb.second.applied_width || height != fb.second.applied_height) {
                mRapi->UpdateFramebufferParameters(fb.first, width, height, 1, true, true, true, true);
                fb.second.applied_width = width;
                fb.second.applied_height = height;
            }
        }
    }

    mPrvDimensions = mCurDimensions;
    mPrevNativeDimensions = mNativeDimensions;
    const bool viewportMatches = ViewportMatchesRendererResolution();
    if (!viewportMatches || mMsaaLevel > 1 || mPostProcessChain.IsActive()) {
        mRendersToFb = true;
        // Track the dimensions mGameFb is sized to so the MSAA resolve
        // target below can match exactly — ResolveSubresource requires
        // identical source/destination dimensions.
        uint32_t gameFbW, gameFbH;
        if (!viewportMatches) {
            gameFbW = mCurDimensions.width;
            gameFbH = mCurDimensions.height;
            mRapi->UpdateFramebufferParameters(mGameFb, gameFbW, gameFbH, mMsaaLevel, true, true, true, true);
        } else {
            // MSAA framebuffer needs to be resolved to an equally sized target when complete, which must therefore
            // match the window size
            gameFbW = mGfxCurrentWindowDimensions.width;
            gameFbH = mGfxCurrentWindowDimensions.height;
            mRapi->UpdateFramebufferParameters(mGameFb, gameFbW, gameFbH, mMsaaLevel, false, true, true, true);
        }
        // Allocate the single-sample resolve target whenever ComposeFinalFrame
        // will consume it: that path resolves into mGameFbMsaaResolved (and
        // samples it as the post-process Source) when MSAA is on AND either
        // the viewport doesn't match the render resolution OR a post-process
        // chain is active. The activation condition here must stay in lockstep
        // with ComposeFinalFrame's — if the resolve target is missing when
        // the chain runs, ResolveSubresource hits a null texture, the backend
        // RunPostProcess early-returns on the null SRV, and the screen goes
        // black (surfaced on Windows/D3D11 with MSAA + a bundled CRT shader,
        // where viewport == render resolution is the default).
        if (mMsaaLevel > 1 && (mPostProcessChain.IsActive() || !viewportMatches)) {
            mRapi->UpdateFramebufferParameters(mGameFbMsaaResolved, gameFbW, gameFbH, 1, false, false, false, false);
        }
    } else {
        mRendersToFb = false;
    }

    // Resize the post-process output FBO to track the window. The chain
    // bails out internally when dimensions are unchanged or zero, and is
    // a no-op when no shader is loaded / the backend lacks support.
    mPostProcessChain.OnResize(mRapi, mGfxCurrentWindowDimensions.width,
                               mGfxCurrentWindowDimensions.height);

    mFbActive = false;
}

GfxExecStack g_exec_stack = {};

extern "C" void gfx_set_trace_callback(GbiTraceCallbackFn callback) {
    sGbiTraceCallback = callback;
}

void Interpreter::RunGuiOnly() {
    SpReset();

    mGetPixelDepthPending.clear();
    mGetPixelDepthCached.clear();

    mRapi->UpdateFramebufferParameters(0, mGfxCurrentWindowDimensions.width, mGfxCurrentWindowDimensions.height, 1,
                                       false, true, true, !mRendersToFb);
    mRapi->StartFrame();
    // Always clear the swap-chain back buffer (fb 0) to a known color before
    // anything else.  With DXGI FLIP_DISCARD swap effect the back buffer
    // contents are undefined after Present(), and the dockspace gap around
    // the "Main Game" ImGui window otherwise shows stale content from two
    // frames ago — visible as a flickering two-tone gray border.
    mRapi->StartDrawToFramebuffer(0, 1);
    mRapi->ClearFramebuffer(true, false);
    mRapi->StartDrawToFramebuffer(mRendersToFb ? mGameFb : 0, (float)mCurDimensions.height / mNativeDimensions.height);
    // SSB64 port widescreen: when active, the game's 4:3-authored scissor
    // covers only ~93% of the FB width, leaving uncleared side strips that
    // show prior-frame garbage. Force a color clear in that mode. Outside
    // widescreen we keep the depth-only clear so the GPU-readback bridge
    // (port_capture_*) still has prior color contents available.
    mRapi->ClearFramebuffer(mWidescreenActive, true);
    mRdp->viewport_or_scissor_changed = true;
    mRenderingState.viewport = {};
    mRenderingState.scissor = {};

    Flush();
    mGfxFrameBuffer = 0;

    if (mRendersToFb) {
        ComposeFinalFrame();
    } else if (mFbActive) {
        // Failsafe reset to main framebuffer to prevent softlocking the renderer
        mFbActive = 0;
        mRapi->StartDrawToFramebuffer(0, 1);

        assert(0 && "active framebuffer was never reset back to original");
    }
}

void Interpreter::Run(Gfx* commands, const std::unordered_map<Mtx*, MtxF>& mtx_replacements) {
    SpReset();

    mGetPixelDepthPending.clear();
    mGetPixelDepthCached.clear();

    mCurMtxReplacements = &mtx_replacements;

    mRapi->UpdateFramebufferParameters(0, mGfxCurrentWindowDimensions.width, mGfxCurrentWindowDimensions.height, 1,
                                       false, true, true, !mRendersToFb);
    mRapi->StartFrame();
    // See RunGuiOnly() for why we clear fb 0 unconditionally.
    mRapi->StartDrawToFramebuffer(0, 1);
    mRapi->ClearFramebuffer(true, false);
    mRapi->StartDrawToFramebuffer(mRendersToFb ? mGameFb : 0, (float)mCurDimensions.height / mNativeDimensions.height);
    // SSB64 port widescreen: when active, the game's 4:3-authored scissor
    // covers only ~93% of the FB width, leaving uncleared side strips that
    // show prior-frame garbage. Force a color clear in that mode. Outside
    // widescreen we keep the depth-only clear so the GPU-readback bridge
    // (port_capture_*) still has prior color contents available.
    mRapi->ClearFramebuffer(mWidescreenActive, true);
    mRdp->viewport_or_scissor_changed = true;
    mRenderingState.viewport = {};
    mRenderingState.scissor = {};

    auto dbg = Ship::Context::GetInstance()->GetGfxDebugger();
    g_exec_stack.start((F3DGfx*)commands);
    while (!g_exec_stack.cmd_stack.empty()) {
        auto cmd = g_exec_stack.cmd_stack.top();

        if (dbg->IsDebugging()) {
            g_exec_stack.gfx_path.push_back(cmd);
            if (dbg->HasBreakPoint(g_exec_stack.gfx_path)) {
                if (mFbActive) {
                    mFbActive = 0;
                    mRapi->StartDrawToFramebuffer(mRendersToFb ? mGameFb : 0, 1);
                }

                break;
            }
            g_exec_stack.gfx_path.pop_back();
        }
        gfx_step();
    }

    Flush();
    mGfxFrameBuffer = 0;
    currentDir = std::stack<std::string>();

    if (mRendersToFb) {
        ComposeFinalFrame();
    } else if (mFbActive) {
        // Failsafe reset to main framebuffer to prevent softlocking the renderer
        mFbActive = 0;
        mRapi->StartDrawToFramebuffer(0, 1);

        assert(0 && "active framebuffer was never reset back to original");
    }
}

void Interpreter::UpdatePostProcessFromCVars() {
    auto cvars = Ship::Context::GetInstance()->GetConsoleVariables();
    if (cvars == nullptr) {
        return;
    }
    const bool enabled = cvars->GetInteger(CVAR_POSTPROCESS_ENABLED, 0) != 0;
    const char* nameCStr = cvars->GetString(CVAR_POSTPROCESS_SHADER, "");
    const std::string name = nameCStr != nullptr ? nameCStr : "";

    const bool wantActive = enabled && !name.empty();
    const bool nameChanged = name != mPostProcessName;
    const bool enabledChanged = enabled != mPostProcessEnabled;

    // Clear the latch on any user-visible state change so the new
    // (name, enabled) pair gets a fresh attempt — otherwise tweaking
    // either cvar after a failed load would be a no-op.
    if (nameChanged || enabledChanged) {
        mPostProcessLoadFailed = false;
    }

    if (!wantActive) {
        if (mPostProcessChain.IsActive()) {
            mPostProcessChain.UnloadShader(mRapi);
        }
    } else if (nameChanged || enabledChanged ||
               (!mPostProcessChain.IsActive() && !mPostProcessLoadFailed)) {
        // Either the user changed something, or the chain isn't active
        // and we haven't already established that this combo can't
        // load. Without the mPostProcessLoadFailed gate, a broken
        // preset would retry once per frame (60 Hz) and spam the log.
        bool loadedOk = false;

        // Phase 3F: probe the slang load path first. .slangp / .slang
        // files exist for this name only if the user (or a builtin)
        // ships one — the loader logs at debug and returns false when
        // neither is present, so the legacy probe below handles the
        // common case.
        PostProcessSlangShaderBundle slangBundle;
        if (LoadPostProcessSlangShader(name, slangBundle)) {
            if (mPostProcessChain.LoadSlangPasses(mRapi, slangBundle.artifacts,
                                                   slangBundle.configs,
                                                   slangBundle.diagnosticNames)) {
                loadedOk = true;
            } else {
                const std::string err = fmt::format(
                    "Slang post-process shader '{}' loaded but backend rejected it", name);
                SPDLOG_ERROR("{}; falling back to legacy probe", err);
                Fast::internal::SetPostProcessRuntimeError(err);
            }
        }
        if (!loadedOk) {
            PostProcessShaderBundle bundle;
            if (LoadPostProcessShader(name, bundle)) {
                if (mPostProcessChain.LoadPasses(mRapi, bundle.sources, bundle.configs,
                                                  bundle.externalTextures)) {
                    loadedOk = true;
                } else {
                    const std::string err = fmt::format(
                        "Post-process shader '{}' loaded but backend rejected it; disabling", name);
                    SPDLOG_ERROR("{}", err);
                    Fast::internal::SetPostProcessRuntimeError(err);
                }
            } else {
                const std::string err = fmt::format(
                    "Post-process shader '{}' not found on disk or in archive", name);
                Fast::internal::SetPostProcessRuntimeError(err);
                if (mPostProcessChain.IsActive()) {
                    // New shader name failed to load; unload the previous
                    // one rather than silently keeping a stale program live.
                    mPostProcessChain.UnloadShader(mRapi);
                }
            }
        }
        mPostProcessLoadFailed = !loadedOk;
    }

    mPostProcessEnabled = enabled;
    mPostProcessName = name;
}

void Interpreter::ComposeFinalFrame() {
    mRapi->StartDrawToFramebuffer(0, 1);
    mRapi->ClearFramebuffer(true, true);

    // The texture-backed FBO whose color attachment the post-process pass
    // samples (or, when no pass is active, the GUI samples directly).
    // -1 means "the MSAA result went straight to the swap-chain back
    // buffer; no texture is available this frame" — only possible on the
    // existing MSAA fast path when post-process is inactive.
    int srcFb = -1;
    if (mMsaaLevel > 1) {
        // §3.1 of the plan: with post-process active we always resolve to
        // mGameFbMsaaResolved so the pass has a sampleable input, even on
        // the matched-resolution fast path.
        if (mPostProcessChain.IsActive() || !ViewportMatchesRendererResolution()) {
            mRapi->ResolveMSAAColorBuffer(mGameFbMsaaResolved, mGameFb);
            srcFb = mGameFbMsaaResolved;
        } else {
            mRapi->ResolveMSAAColorBuffer(0, mGameFb);
        }
    } else {
        srcFb = mGameFb;
    }

    if (mPostProcessChain.IsActive() && srcFb >= 0) {
        PostProcessParams params{};
        params.srcWidth = mCurDimensions.width;
        params.srcHeight = mCurDimensions.height;
        // The video signal the game itself targets is N64 320x240
        // (mNativeDimensions). Per-source-row CRT shaders use the
        // source/input ratio to space scanlines correctly relative to
        // game pixels rather than output pixels.
        params.inputWidth = mNativeDimensions.width;
        params.inputHeight = mNativeDimensions.height;
        params.dstWidth = mGfxCurrentWindowDimensions.width;
        params.dstHeight = mGfxCurrentWindowDimensions.height;
        params.frameCount = mFrameCounter++;
        params.frameDeltaSeconds = 0.0f;
        const int outFb = mPostProcessChain.Run(mRapi, srcFb, params);
        mGfxFrameBuffer = (uintptr_t)mRapi->GetFramebufferTextureId(outFb);
        // Re-bind the swap-chain FB so the ImGui dispatch that follows
        // (gui->EndDraw -> ImGui_ImplOpenGL3_RenderDrawData) targets
        // FB 0 rather than the chain's intermediate output FB that
        // RunPostProcess left bound. Without this, the GUI draws into
        // the off-screen post-process FBO and the on-screen back buffer
        // never receives the menu / Image draws.
        mRapi->StartDrawToFramebuffer(0, 1);
    } else if (srcFb >= 0) {
        mGfxFrameBuffer = (uintptr_t)mRapi->GetFramebufferTextureId(srcFb);
    }
}

void Interpreter::PresentCurrentFramebuffer() {
    mRapi->UpdateFramebufferParameters(0, mGfxCurrentWindowDimensions.width, mGfxCurrentWindowDimensions.height, 1,
                                       false, true, true, !mRendersToFb);
    mRapi->StartFrame();
    mRapi->StartDrawToFramebuffer(0, 1);
    mRapi->ClearFramebuffer(true, false);
}

void Interpreter::EndFrame() {
    mRapi->EndFrame();
    mWapi->SwapBuffersBegin();
    mRapi->FinishRender();
    mWapi->SwapBuffersEnd();
}

void gfx_set_target_ucode(UcodeHandlers ucode) {
    ucode_handler_index = ucode;
}

int Interpreter::GetTargetFps() {
    return mWapi->GetTargetFps();
}

void Interpreter::SetTargetFps(int fps) {
    mWapi->SetTargetFps(fps);
}

void Interpreter::SetMaxFrameLatency(int latency) {
    mWapi->SetMaxFrameLatency(latency);
}

int Interpreter::CreateFrameBuffer(uint32_t width, uint32_t height, uint32_t native_width, uint32_t native_height,
                                   uint8_t resize) {
    uint32_t orig_width = width, orig_height = height;
    if (resize) {
        AdjustWidthHeightForScale(width, height, native_width, native_height);
    }

    int fb = mRapi->CreateFramebuffer();
    mRapi->UpdateFramebufferParameters(fb, width, height, 1, true, true, true, true);

    mFrameBuffers[fb] = {
        orig_width, orig_height, width, height, native_width, native_height, static_cast<bool>(resize)
    };
    return fb;
}

void Interpreter::SetFrameBuffer(int fb, float noiseScale) {
    mRapi->StartDrawToFramebuffer(fb, noiseScale);
    mRapi->ClearFramebuffer(false, true);
}

void Interpreter::CopyFrameBuffer(int fb_dst_id, int fb_src_id, bool copyOnce, bool* hasCopiedPtr) {
    // Do not copy again if we have already copied before
    if (copyOnce && hasCopiedPtr != nullptr && *hasCopiedPtr) {
        return;
    }

    if (fb_src_id == 0 && mRendersToFb) {
        // read from the framebuffer we've been rendering to
        fb_src_id = mGameFb;
    }

    int srcX0, srcY0, srcX1, srcY1;
    int dstX0, dstY0, dstX1, dstY1;

    // When rendering to the main window buffer or MSAA is enabled with a buffer size equal to the view port,
    // then the source coordinates must account for any docked ImGui elements
    if (fb_src_id == 0 || (mMsaaLevel > 1 && mCurDimensions.width == mGameWindowViewport.width &&
                           mCurDimensions.height == mGameWindowViewport.height)) {
        srcX0 = mGameWindowViewport.x;
        srcY0 = mGameWindowViewport.y;
        srcX1 = mGameWindowViewport.x + mGameWindowViewport.width;
        srcY1 = mGameWindowViewport.y + mGameWindowViewport.height;
    } else {
        srcX0 = 0;
        srcY0 = 0;
        srcX1 = mCurDimensions.width;
        srcY1 = mCurDimensions.height;
    }

    dstX0 = 0;
    dstY0 = 0;
    dstX1 = mCurDimensions.width;
    dstY1 = mCurDimensions.height;

    mRapi->CopyFramebuffer(fb_dst_id, fb_src_id, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1);

    // Set the copied pointer if we have one
    if (hasCopiedPtr != nullptr) {
        *hasCopiedPtr = true;
    }
}

void Interpreter::ResetFrameBuffer() {
    mRapi->StartDrawToFramebuffer(0, (float)mCurDimensions.height / mNativeDimensions.height);
}

void Interpreter::AdjustPixelDepthCoordinates(float& x, float& y) {
    x = x * RATIO_X(mActiveFrameBuffer, mCurDimensions) -
        (mNativeDimensions.width * RATIO_X(mActiveFrameBuffer, mCurDimensions) - mCurDimensions.width) / 2;
    y *= RATIO_Y(mActiveFrameBuffer, mCurDimensions);
    if (!mRendersToFb || (mMsaaLevel > 1 && mCurDimensions.width == mGameWindowViewport.width &&
                          mCurDimensions.height == mGameWindowViewport.height)) {
        x += mGameWindowViewport.x;
        y += mGfxCurrentWindowDimensions.height - (mGameWindowViewport.y + mGameWindowViewport.height);
    }
}

void Interpreter::GetPixelDepthPrepare(float x, float y) {
    AdjustPixelDepthCoordinates(x, y);
    mGetPixelDepthPending.emplace(x, y);
}

uint16_t Interpreter::GetPixelDepth(float x, float y) {
    AdjustPixelDepthCoordinates(x, y);

    if (auto it = mGetPixelDepthCached.find(std::make_pair(x, y)); it != mGetPixelDepthCached.end()) {
        return it->second;
    }

    mGetPixelDepthPending.emplace(x, y);

    std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff> res =
        mRapi->GetPixelDepth(mRendersToFb ? mGameFb : 0, mGetPixelDepthPending);
    mGetPixelDepthCached.merge(res);
    mGetPixelDepthPending.clear();

    return mGetPixelDepthCached.find(std::make_pair(x, y))->second;
}

void gfx_push_current_dir(char* path) {
    if (gfx_check_image_signature(path) == 1)
        path = &path[7];

    currentDir.push(GetPathWithoutFileName(path));
}

int32_t gfx_check_image_signature(const char* imgData) {
    uintptr_t i = (uintptr_t)(imgData);

    if ((i & 1) == 1) {
        return 0;
    }

    // Filter addresses that are obviously not valid string pointers before
    // attempting to dereference for the "__OTR__" check.
    if (i == 0 || i < 0x10000) {
        return 0;
    }
#if UINTPTR_MAX > 0xFFFFFFFFu
    // On 64-bit: filter kernel/sentinel addresses. Upper bound covers all
    // user-space layouts (x86_64 47-bit canonical, ARM64 48-bit VA, etc.).
    if (i > 0x0000FFFFFFFFFFFFull) {
        return 0;
    }
#endif
    if (!gfxPointerHasReadableBytes(imgData, 8)) {
        return 0;
    }

    return Ship::Context::GetInstance()->GetResourceManager()->OtrSignatureCheck(imgData);
}

void Interpreter::RegisterBlendedTexture(const char* name, uint8_t* mask, uint8_t* replacement) {
    if (gfx_check_image_signature(name)) {
        name += 7;
    }

    if (gfx_check_image_signature(reinterpret_cast<char*>(replacement))) {
        Fast::Texture* tex = std::static_pointer_cast<Fast::Texture>(
                                 Ship::Context::GetInstance()->GetResourceManager()->LoadResourceProcess(
                                     reinterpret_cast<char*>(replacement)))
                                 .get();

        replacement = tex->ImageData;
    }

    mMaskedTextures[name] = MaskedTextureEntry{ mask, replacement };
}

void Interpreter::UnregisterBlendedTexture(const char* name) {
    if (gfx_check_image_signature(name)) {
        name += 7;
    }

    mMaskedTextures.erase(name);
}

void Interpreter::RegisterFbTexture(const void* base, size_t sizeBytes, int fbId,
                                    float u0, float v0, float u1, float v1) {
    if (base == nullptr || sizeBytes == 0) {
        return;
    }
    uintptr_t b = reinterpret_cast<uintptr_t>(base);
    mFbTextures[b] = FbTextureRange{ b + sizeBytes, fbId, u0, v0, u1, v1 };
}

void Interpreter::UnregisterFbTexture(const void* base) {
    mFbTextures.erase(reinterpret_cast<uintptr_t>(base));
}

void Interpreter::ClearFbTextures() {
    mFbTextures.clear();
}

// New getters and setters
void Interpreter::SetNativeDimensions(float width, float height) {
    mNativeDimensions.width = width;
    mNativeDimensions.height = height;
}

void Interpreter::SetResolutionMultiplier(float multiplier) {
    mCurDimensions.internal_mul = multiplier;
}

void Interpreter::SetMsaaLevel(uint32_t level) {
    mMsaaLevel = level;
}

void Interpreter::GetCurDimensions(uint32_t* width, uint32_t* height) {
    *width = mCurDimensions.width;
    *height = mCurDimensions.height;
}

} // namespace Fast

void gfx_cc_get_features(uint64_t shader_id0, uint64_t shader_id1, struct CCFeatures* cc_features) {
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            for (int k = 0; k < 4; k++) {
                cc_features->c[i][j][k] = shader_id0 >> i * 32 + j * 16 + k * 4 & 0xf;
            }
        }
    }

    cc_features->opt_alpha = (shader_id1 & SHADER_OPT(ALPHA)) != 0;
    cc_features->opt_fog = (shader_id1 & SHADER_OPT(FOG)) != 0;
    cc_features->opt_texture_edge = (shader_id1 & SHADER_OPT(TEXTURE_EDGE)) != 0;
    cc_features->opt_noise = (shader_id1 & SHADER_OPT(NOISE)) != 0;
    cc_features->opt_2cyc = (shader_id1 & SHADER_OPT(_2CYC)) != 0;
    cc_features->opt_alpha_threshold = (shader_id1 & SHADER_OPT(ALPHA_THRESHOLD)) != 0;
    cc_features->opt_invisible = (shader_id1 & SHADER_OPT(INVISIBLE)) != 0;
    cc_features->opt_grayscale = (shader_id1 & SHADER_OPT(GRAYSCALE)) != 0;

    cc_features->clamp[0][0] = shader_id1 & SHADER_OPT(TEXEL0_CLAMP_S);
    cc_features->clamp[0][1] = shader_id1 & SHADER_OPT(TEXEL0_CLAMP_T);
    cc_features->clamp[1][0] = shader_id1 & SHADER_OPT(TEXEL1_CLAMP_S);
    cc_features->clamp[1][1] = shader_id1 & SHADER_OPT(TEXEL1_CLAMP_T);

    cc_features->usedTextures[0] = false;
    cc_features->usedTextures[1] = false;
    cc_features->used_masks[0] = false;
    cc_features->used_masks[1] = false;
    cc_features->used_blend[0] = false;
    cc_features->used_blend[1] = false;
    cc_features->numInputs = 0;

    for (int c = 0; c < 2; c++) {
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 4; j++) {
                if (cc_features->c[c][i][j] >= SHADER_INPUT_1 && cc_features->c[c][i][j] <= SHADER_INPUT_7) {
                    if (cc_features->c[c][i][j] > cc_features->numInputs) {
                        cc_features->numInputs = cc_features->c[c][i][j];
                    }
                }
                if (cc_features->c[c][i][j] == SHADER_TEXEL0 || cc_features->c[c][i][j] == SHADER_TEXEL0A) {
                    cc_features->usedTextures[0] = true;
                    if (cc_features->opt_2cyc) {
                        cc_features->usedTextures[1] = true;
                    }
                }
                if (cc_features->c[c][i][j] == SHADER_TEXEL1 || cc_features->c[c][i][j] == SHADER_TEXEL1A) {
                    cc_features->usedTextures[1] = true;
                    if (cc_features->opt_2cyc) {
                        cc_features->usedTextures[0] = true;
                    }
                }
            }
        }
    }

    for (int c = 0; c < 2; c++) {
        cc_features->do_single[c][0] = cc_features->c[c][0][2] == SHADER_0;
        cc_features->do_single[c][1] = cc_features->c[c][1][2] == SHADER_0;
        cc_features->do_multiply[c][0] = cc_features->c[c][0][1] == SHADER_0 && cc_features->c[c][0][3] == SHADER_0;
        cc_features->do_multiply[c][1] = cc_features->c[c][1][1] == SHADER_0 && cc_features->c[c][1][3] == SHADER_0;
        cc_features->do_mix[c][0] = cc_features->c[c][0][1] == cc_features->c[c][0][3];
        cc_features->do_mix[c][1] = cc_features->c[c][1][1] == cc_features->c[c][1][3];
        cc_features->color_alpha_same[c] = (shader_id0 >> c * 32 & 0xffff) == (shader_id0 >> c * 32 + 16 & 0xffff);
    }

    if (cc_features->usedTextures[0] && shader_id1 & SHADER_OPT(TEXEL0_MASK)) {
        cc_features->used_masks[0] = true;
    }
    if (cc_features->usedTextures[1] && shader_id1 & SHADER_OPT(TEXEL1_MASK)) {
        cc_features->used_masks[1] = true;
    }

    if (cc_features->usedTextures[0] && shader_id1 & SHADER_OPT(TEXEL0_BLEND)) {
        cc_features->used_blend[0] = true;
    }
    if (cc_features->usedTextures[1] && shader_id1 & SHADER_OPT(TEXEL1_BLEND)) {
        cc_features->used_blend[1] = true;
    }

    cc_features->shader_id = Fast::ShaderIdUnmask(shader_id1);
}

extern "C" int gfx_create_framebuffer(uint32_t width, uint32_t height, uint32_t native_width, uint32_t native_height,
                                      uint8_t resize) {
    return Fast::mInstance.lock().get()->CreateFrameBuffer(width, height, native_width, native_height, resize);
}

extern "C" void gfx_texture_cache_clear() {
    Fast::mInstance.lock().get()->TextureCacheClear();
}

extern "C" void gfx_register_hires_hook(GfxHiResHookFn callback) {
    Fast::gHiResHook = callback;
}
