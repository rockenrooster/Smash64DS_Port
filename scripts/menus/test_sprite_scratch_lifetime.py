"""Scene-owned sprite scratch lifetime.

The 153600-byte sOriginalSpritePreview lives on the scene heap, not in
permanent BSS: pointer + generation, allocated once per using generation in
ndsPlatformBeginOriginalSpritePreview; ndsPlatformClearOriginalSpritePreview
resets extents without touching stale/freed memory; commit rejects a stale
generation. (The software-triangle build's static array went with that path,
1c8636a6632.) This test host-executes the ACTUAL production Begin/Clear bodies
plus the real commit validity gate and scaler (extracted verbatim via
source_test_helpers, not Python replicas), and audits live callers for holds
that would block the change.

Measured benefit (narrow): a native CSS round with no SObj draw never takes
the allocation path, so its scenes keep the freed BSS. This test does NOT
claim full-roster RAM closure.

USAGE
  python scripts/menus/test_sprite_scratch_lifetime.py
"""
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from source_test_helpers import function

ROOT = Path(__file__).resolve().parents[2]
PLATFORM = ROOT / "src/nds/nds_platform.c"
PROD = PLATFORM.read_text()

BEGIN = function(PROD, "ndsPlatformBeginOriginalSpritePreview")
RESERVE = function(PROD, "ndsPlatformReserveOriginalSpritePreview")
CLEAR = function(PROD, "ndsPlatformClearOriginalSpritePreview")
COMMIT = function(PROD, "ndsPlatformCommitOriginalSpritePreviewLayer")
SCALE = function(PROD, "ndsPlatformScaleOriginalSpritePreviewNearest")
ADVANCE = function(PROD, "ndsPlatformAdvanceOriginalSpriteOverlayEpoch")
CLEARPIX = function(PROD, "ndsPlatformOriginalSpriteOverlayClearPixels")

SCRATCH_BYTES = 320 * 240 * 2  # 153600


def host_source():
    return r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t s32;
#define TRUE 1
#define FALSE 0
#define NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH 320u
#define NDS_ORIGINAL_SPRITE_PREVIEW_MAX_HEIGHT 240u
#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 192
#define NDS_SCENE_MIP_CACHE_LAB 0
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static u16 *sOriginalSpritePreview;
static u32 sOriginalSpritePreviewGeneration;
volatile u32 gNdsTaskmanHeapGeneration;
static u16 heap_pool[4][320u * 240u];
static int heap_next;
static int malloc_calls;
static size_t malloc_last_size;
static u32 malloc_last_align;
static int malloc_fail;
struct SYMallocRegion { int unused; } gSYTaskmanGeneralHeap;
static int fit_fail;
s32 ndsSyMallocWouldFit(const struct SYMallocRegion *region, size_t size, u32 align)
{
    CHECK(region == &gSYTaskmanGeneralHeap && size == 153600 && align == 4u);
    return !fit_fail;
}
void *syTaskmanMalloc(size_t size, u32 align)
{
    malloc_calls++;
    malloc_last_size = size;
    malloc_last_align = align;
    if (malloc_fail) return NULL;
    if (heap_next >= 4) { fprintf(stderr, "heap pool exhausted\n"); exit(1); }
    return heap_pool[heap_next++];
}
static u32 sOriginalSpritePreviewWidth;
static u32 sOriginalSpritePreviewHeight;
static s32 sOriginalSpritePreviewX;
static s32 sOriginalSpritePreviewY;
static u32 sOriginalSpritePreviewReady;
static u32 sOriginalSpriteDisplayPreviewWidth;
static u32 sOriginalSpriteDisplayPreviewHeight;
static u32 sOriginalSpriteDecodeCacheEpoch = 1u;
volatile u32 gNdsOriginalSpritePreviewReady;
volatile u32 gNdsOriginalSpritePreviewCommitCount;
volatile u32 gNdsOriginalSpritePreviewDrawCount;
volatile u32 gNdsOriginalSpritePreviewDisplayWidth;
volatile u32 gNdsOriginalSpritePreviewDisplayHeight;
volatile u32 gNdsOriginalSpriteBg2ClearBytes;
volatile u32 gNdsOriginalSpriteBg2CopyBytes;
volatile u32 gNdsOriginalSpriteBg2FinalWriteBytes;
volatile u32 gNdsOriginalSpriteBg3ClearBytes;
volatile u32 gNdsOriginalSpriteBg3CopyBytes;
volatile u32 gNdsOriginalSpriteBg3FinalWriteBytes;
static int sOriginalSpriteOverlayBg = -1;
static int sOriginalSpriteOverlayForegroundBg = -1;
static u32 sOriginalSpriteOverlayLayerMask = 0u;
static u32 sOriginalSpriteOverlayEpoch[2] = {1u, 1u};
static void *bgGetGfxPtr(int bg)
{
    (void)bg;
    fprintf(stderr, "bgGetGfxPtr must not run with mask 0\n");
    exit(1);
    return NULL;
}
static void dmaFillHalfWords(u16 value, void *dst, unsigned size)
{
    (void)value; (void)dst; (void)size;
    fprintf(stderr, "dmaFill must not run with mask 0\n");
    exit(1);
}
/* The commit's transition and BG3-loan seams: no snapshot is held and BG3 is
 * never lent here. */
static u32 sOriginalSpriteOverlayBg3Lent;
static u32 sOriginalSpriteOverlayBg3ReturnPending;
volatile u32 gNdsVramBg3RefusedWrites;
static void ndsPlatformTransitionThaw(void) {}
static void ndsPlatformTransitionSnapshotLeaveBg2(void) {}
static void ndsPlatformTransitionSnapshotLeaveBg3(void) {}
static void ndsPlatformVramReturnBankDNow(void)
{
    fprintf(stderr, "bank D return must not run without a lent BG3\n");
    exit(1);
}
/* P8 V5: bank C is never lent to textures here, so BG2 is always usable. */
static s32 ndsPlatformVramBg2Usable(u32 write) { (void)write; return 1; }
''' + SCALE + "\n" + ADVANCE + "\n" + CLEARPIX + "\n" + RESERVE + "\n" + BEGIN + "\n" + COMMIT + "\n" + CLEAR + r'''
static void check_window_cleared(u16 *base, u32 w, u32 h, u16 sentinel)
{
    for (u32 r = 0; r < 240u; r++) {
        for (u32 c = 0; c < 320u; c++) {
            u16 want = (r < h && c < w) ? (u16)0 : sentinel;
            if (base[r * 320u + c] != want) {
                fprintf(stderr, "row %u col %u: 0x%04x want 0x%04x\n",
                        r, c, base[r * 320u + c], want);
                exit(1);
            }
        }
    }
}
int main(void)
{
    u32 pitch = 0;
    u16 *p, *q;
    u32 cc;
    /* Invalid dimensions: NULL, no allocation attempt. */
    {
        static const u32 bad[][2] = {{0,240},{320,0},{321,240},{320,241},{0,0},{512,512}};
        for (unsigned i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
            pitch = 0xdead;
            p = ndsPlatformBeginOriginalSpritePreview(bad[i][0], bad[i][1], 0, 0, &pitch);
            CHECK(p == NULL);
        }
        CHECK(malloc_calls == 0);
    }
    /* First valid allocation: exact 153600 bytes, align 4, pitch 320, zeroed. */
    memset(heap_pool, 0xA5, sizeof(heap_pool));
    gNdsTaskmanHeapGeneration = 5;
    CHECK(ndsPlatformReserveOriginalSpritePreview());
    CHECK(malloc_calls == 1 && sOriginalSpritePreviewWidth == 0u);
    p = ndsPlatformBeginOriginalSpritePreview(320u, 240u, 10, 20, &pitch);
    CHECK(p != NULL && malloc_calls == 1);
    CHECK(malloc_last_size == 153600 && malloc_last_align == 4u);
    CHECK(pitch == 320u);
    CHECK(sOriginalSpritePreviewWidth == 320u && sOriginalSpritePreviewHeight == 240u);
    CHECK(sOriginalSpritePreviewReady == 0u && gNdsOriginalSpritePreviewReady == 0u);
    CHECK(sOriginalSpriteDisplayPreviewWidth == 0u && sOriginalSpriteDisplayPreviewHeight == 0u);
    for (unsigned i = 0; i < 320u * 240u; i++) CHECK(p[i] == 0);
    /* Repeat Begin/Clear on the same generation reuses without reallocating. */
    q = ndsPlatformBeginOriginalSpritePreview(320u, 240u, 0, 0, &pitch);
    CHECK(q == p && malloc_calls == 1);
    ndsPlatformClearOriginalSpritePreview();
    CHECK(sOriginalSpritePreviewWidth == 0u && sOriginalSpritePreviewHeight == 0u);
    CHECK(sOriginalSpritePreviewReady == 0u && gNdsOriginalSpritePreviewReady == 0u);
    CHECK(sOriginalSpriteDisplayPreviewWidth == 0u && sOriginalSpriteDisplayPreviewHeight == 0u);
    CHECK(sOriginalSpritePreview == p && sOriginalSpritePreviewGeneration == 5u);
    /* Clear touches no pixel memory: stale/freed bytes stay exactly as left. */
    for (unsigned i = 0; i < 320u * 240u; i++) p[i] = (u16)0x5A5A;
    {
        u32 epoch = sOriginalSpriteDecodeCacheEpoch;
        ndsPlatformClearOriginalSpritePreview();
        CHECK(sOriginalSpriteDecodeCacheEpoch == epoch + 1u);
        for (unsigned i = 0; i < 320u * 240u; i++) CHECK(p[i] == (u16)0x5A5A);
    }
    q = ndsPlatformBeginOriginalSpritePreview(64u, 48u, 0, 0, &pitch);
    CHECK(q == p && malloc_calls == 1);
    /* Row pitch, per-row clearing, no overflow past the window. */
    memset(heap_pool, 0xA5, sizeof(heap_pool));
    p = ndsPlatformBeginOriginalSpritePreview(64u, 48u, 0, 0, &pitch);
    CHECK(p != NULL && pitch == 320u && malloc_calls == 1);
    check_window_cleared(p, 64u, 48u, (u16)0xA5A5);
    q = ndsPlatformBeginOriginalSpritePreview(64u, 48u, 0, 0, NULL);
    CHECK(q == p);
    /* New generation: fresh storage, old buffer byte-identical. */
    p = ndsPlatformBeginOriginalSpritePreview(320u, 240u, 0, 0, &pitch);
    for (unsigned i = 0; i < 320u * 240u; i++) p[i] = (u16)(0x1234u + (i & 0xffu));
    {
        static u16 oldcopy[320u * 240u];
        memcpy(oldcopy, p, sizeof(oldcopy));
        gNdsTaskmanHeapGeneration = 6;
        q = ndsPlatformBeginOriginalSpritePreview(320u, 240u, 0, 0, &pitch);
        CHECK(q != NULL && q != p && malloc_calls == 2);
        CHECK(sOriginalSpritePreviewGeneration == 6u);
        CHECK(memcmp(p, oldcopy, sizeof(oldcopy)) == 0);
        for (unsigned i = 0; i < 320u * 240u; i++) CHECK(q[i] == 0);
    }
    /* Failed allocation exposes neither old extents nor old data. */
    for (unsigned i = 0; i < 320u * 240u; i++) q[i] = (u16)0x7777;
    cc = gNdsOriginalSpritePreviewCommitCount;
    gNdsTaskmanHeapGeneration = 7;
    fit_fail = 1;
    CHECK(ndsPlatformBeginOriginalSpritePreview(100u, 100u, 0, 0, &pitch) == NULL);
    CHECK(malloc_calls == 2); /* no call to the target's fatal allocator */
    CHECK(sOriginalSpritePreview == NULL && gNdsOriginalSpritePreviewReady == 0u);
    CHECK(gNdsOriginalSpritePreviewDisplayWidth == 0u && gNdsOriginalSpritePreviewDisplayHeight == 0u);
    fit_fail = 0;
    malloc_fail = 1;
    {
        u16 *r = ndsPlatformBeginOriginalSpritePreview(100u, 100u, 0, 0, &pitch);
        CHECK(r == NULL);
        CHECK(malloc_calls == 3);
        CHECK(sOriginalSpritePreviewWidth == 0u && sOriginalSpritePreviewHeight == 0u);
        CHECK(sOriginalSpritePreviewReady == 0u);
        CHECK(sOriginalSpriteDisplayPreviewWidth == 0u && sOriginalSpriteDisplayPreviewHeight == 0u);
        CHECK(sOriginalSpritePreviewGeneration != gNdsTaskmanHeapGeneration);
        for (unsigned i = 0; i < 320u * 240u; i++) CHECK(q[i] == (u16)0x7777);
        ndsPlatformCommitOriginalSpritePreviewLayer(FALSE);
        CHECK(gNdsOriginalSpritePreviewCommitCount == cc);
    }
    malloc_fail = 0;
    /* Commit gate: valid commits, stale generation and empty extents do not. */
    p = ndsPlatformBeginOriginalSpritePreview(100u, 80u, 0, 0, &pitch);
    CHECK(p != NULL && malloc_calls == 4);
    ndsPlatformCommitOriginalSpritePreviewLayer(FALSE);
    CHECK(gNdsOriginalSpritePreviewCommitCount == cc + 1u);
    CHECK(sOriginalSpriteDisplayPreviewWidth == 100u && sOriginalSpriteDisplayPreviewHeight == 80u);
    CHECK(sOriginalSpritePreviewReady == 1u && gNdsOriginalSpritePreviewReady == 1u);
    CHECK(gNdsOriginalSpritePreviewDisplayWidth == 100u && gNdsOriginalSpritePreviewDisplayHeight == 80u);
    gNdsTaskmanHeapGeneration = 8;
    ndsPlatformCommitOriginalSpritePreviewLayer(FALSE);
    CHECK(gNdsOriginalSpritePreviewCommitCount == cc + 1u);
    CHECK(sOriginalSpriteDisplayPreviewWidth == 100u && sOriginalSpriteDisplayPreviewHeight == 80u);
    ndsPlatformClearOriginalSpritePreview();
    ndsPlatformCommitOriginalSpritePreviewLayer(FALSE);
    CHECK(gNdsOriginalSpritePreviewCommitCount == cc + 1u);
    printf("HW PASS\n");
    return 0;
}
'''


def compile_and_run(compiler, directory):
    source = Path(directory) / "scratch_hw.c"
    program = Path(directory) / "scratch_hw.exe"
    source.write_text(host_source())
    flags = ["-std=c11", "-Wall", "-Wextra", str(source), "-o", str(program)]
    subprocess.run([compiler] + flags, check=True, capture_output=True,
                   text=True)
    result = subprocess.run([str(program)], capture_output=True, text=True)
    return result


class SpriteScratchLifetimeTest(unittest.TestCase):
    def test_production_shape_pins(self):
        self.assertEqual(SCRATCH_BYTES, 153600)
        self.assertIn("static u16 *sOriginalSpritePreview;", PROD)
        self.assertIn("static u32 sOriginalSpritePreviewGeneration;", PROD)
        self.assertRegex(
            RESERVE,
            r"syTaskmanMalloc\(\s*NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH\s*\*"
            r"\s*NDS_ORIGINAL_SPRITE_PREVIEW_MAX_HEIGHT\s*\*\s*sizeof\(u16\), 4u\)")
        self.assertIn(
            "(sOriginalSpritePreviewGeneration != gNdsTaskmanHeapGeneration)",
            RESERVE)
        self.assertIn(
            "(sOriginalSpritePreviewGeneration != gNdsTaskmanHeapGeneration)",
            COMMIT)
        # Clear resets extents and bumps the decode epoch; no pixel memory.
        self.assertIn("sOriginalSpritePreviewWidth = 0;", CLEAR)
        self.assertIn("sOriginalSpriteDecodeCacheEpoch++;", CLEAR)
        self.assertNotIn("memset", CLEAR)
        # No retained static frame: the scratch is the heap allocation only.
        self.assertNotIn("static u16 sOriginalSpritePreview[", PROD)
        self.assertNotIn("sOriginalSpriteDisplayPreview[", PROD)

    def test_host_hw_lifetime(self):
        compiler = next((shutil.which(c) for c in ("clang", "gcc", "cc")
                         if shutil.which(c)), None)
        self.assertIsNotNone(compiler, "Host C compiler required")
        with tempfile.TemporaryDirectory() as directory:
            result = compile_and_run(compiler, directory)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn("HW PASS", result.stdout)

    def test_caller_blockers(self):
        """Production ROM code must not reacquire the retired software scratch."""
        failures = []
        # The generic software sprite/staffroll renderer moved under
        # src/host/graphics_reference. Those files deliberately retain the old
        # scratch API for source-comparison tests, but they are not ROM inputs.
        # Any caller elsewhere under src/ would reintroduce software ownership.
        holders = set()
        for path in (ROOT / "src").rglob("*.c"):
            if path.name == "nds_platform.c":
                continue
            rel = path.relative_to(ROOT).as_posix()
            if rel.startswith("src/host/graphics_reference/"):
                continue
            if "ndsPlatformBeginOriginalSpritePreview(" in path.read_text():
                holders.add(rel)
        if holders:
            failures.append(
                f"production Begin caller set is nonempty: {sorted(holders)}")
        print("Production Begin holders: none; software scratch callers remain "
              "host-reference-only under src/host/graphics_reference.")
        self.assertEqual(failures, [])


if __name__ == "__main__":
    unittest.main()
