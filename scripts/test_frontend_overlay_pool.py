"""Host-execute the actual front-end loan owner with only DS services mocked.

This checks the lifecycle and bounds of a separate scene-asset region. It does
not prove target code loading, source callbacks, or rendered menu restoration.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[1]


def test_frontend_loan_lifecycle_and_bounds():
    compiler = next((shutil.which(c) for c in ("gcc", "clang", "cc")
                     if shutil.which(c)), None)
    assert compiler, "A host C compiler is required"
    source = (ROOT / "src/nds/nds_frontend_overlay.c").read_text()
    # Linker addresses become a real host array; all owner code stays unchanged.
    source = source.replace("extern u8 __nds_frontend_start[];",
                            "#define __nds_frontend_start test_overlay")
    source = source.replace("extern u8 __nds_frontend_end[];",
                            "#define __nds_frontend_end (test_overlay + 256)")
    source = "\n".join(line for line in source.splitlines()
                       if not line.startswith("#include"))
    fixture = r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
typedef uint8_t u8;
typedef uint32_t u32;
typedef int32_t sb32;
enum { FALSE = 0, nSCKindVSBattle = 22 };
static _Alignas(32) u8 test_overlay[256];
static u8 general_asset[512];
static unsigned loads, deactivations, initializations, locks, allocations;
static int ovlInit(void) { ++initializations; return 1; }
static int ovlLoadAndActivate(unsigned id) {
    assert(id == 0 && locks == 1);
    ++loads;
    memset(test_overlay, 0xa5, sizeof(test_overlay));
    return 1;
}
static void ovlDeactivate(unsigned id) { assert(id == 0); ++deactivations; }
static void ndsFsLock(void) { assert(locks++ == 0); }
static void ndsFsUnlock(void) { assert(--locks == 0); }
static void ndsRelocReleaseSceneStatusBuffers(void) {}
static void *syTaskmanMalloc(size_t bytes, u32 alignment) {
    assert(bytes <= sizeof(general_asset) && alignment == 16);
    ++allocations;
    return general_asset;
}
''' + source + r'''
int main(void) {
    assert(ndsFrontendOverlayTryAlloc(16, 16) == NULL);
    ndsFrontendOverlayPrepareDispatch(1);
    ndsFrontendOverlayBeginScene(1);
    assert(loads == 1 && initializations == 1);
    assert(ndsFrontendOverlayTryAlloc(16, 16) == NULL);
    // Nested non-VS scenes retain code; the parent may still return into it.
    ndsFrontendOverlayBeginScene(20);
    assert(ndsFrontendOverlayTryAlloc(16, 16) == NULL);
    ndsFrontendOverlayEndScene();
    ndsFrontendOverlayPrepareDispatch(nSCKindVSBattle);
    assert(deactivations == 1);
    assert(ndsFrontendOverlayTryAlloc(16, 16) == NULL);
    ndsFrontendOverlayBeginScene(nSCKindVSBattle);
    assert(ndsFrontendOverlayTryAlloc(0, 16) == NULL);
    assert(ndsFrontendOverlayTryAlloc(8, 0) == NULL);
    assert(ndsFrontendOverlayTryAlloc(8, 3) == NULL);
    assert(ndsFrontendOverlayTryAlloc(SIZE_MAX, 16) == NULL);
    assert(ndsFrontendOverlayTryAlloc(1, 1) == test_overlay);
    assert(ndsFrontendOverlayTryAlloc(16, 32) == test_overlay + 32);
    assert(gNdsFrontendOverlayUsedBytes == 48);
    assert(ndsFrontendOverlayTryAlloc(209, 1) == NULL);
    assert(gNdsFrontendOverlayUsedBytes == 48);
    assert(ndsFrontendOverlayTryAlloc(208, 1) == test_overlay + 48);
    assert(gNdsFrontendOverlayUsedBytes == 256);
    assert(ndsSceneAssetAlloc(16, 16) == general_asset);
    assert(allocations == 1 && gNdsFrontendOverlaySpillBytes == 16);
    ndsFrontendOverlayEndScene();
    assert(ndsFrontendOverlayTryAlloc(16, 16) == NULL);
    // Same-kind re-entry must reuse the region, not retain an exhausted cursor.
    ndsFrontendOverlayBeginScene(nSCKindVSBattle);
    assert(gNdsFrontendOverlayUsedBytes == 0);
    assert(ndsFrontendOverlayTryAlloc(256, 16) == test_overlay);
    memset(test_overlay, 0x33, sizeof(test_overlay));
    ndsFrontendOverlayEndScene();
    ndsFrontendOverlayPrepareDispatch(1);
    assert(loads == 2 && initializations == 1);
    assert(test_overlay[0] == 0xa5 && test_overlay[255] == 0xa5);
    ndsFrontendOverlayPrepareDispatch(2);
    assert(loads == 2); // menu-to-menu does not reload or reset mutable state
    assert(gNdsFrontendOverlayLoadFailCount == 0 && locks == 0);
    return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix="frontend-pool-", dir=ROOT / "builds") as tmp:
        cpath = Path(tmp) / "pool.c"
        binary = Path(tmp) / "pool.exe"
        cpath.write_text(fixture)
        subprocess.run([compiler, "-std=c11", "-O2", str(cpath), "-o", str(binary)],
                       check=True, capture_output=True, text=True)
        subprocess.run([str(binary)], check=True, capture_output=True, text=True)
