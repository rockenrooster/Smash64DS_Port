"""Run the real arena chooser across main-heap capacities, including campaign.

The old coarse fallback threw away usable pages below the 1.19 MiB range.
The allocator mock models available bytes plus newlib's untrimmed top chunk,
which every probe loses until malloc_trim runs. The chooser must hand libc
exactly NDS_TASKMAN_LIBC_TOP_BYTES (to 256 B) whatever that top chunk was.
"""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest
from source_test_helpers import function

ROOT = Path(__file__).resolve().parents[2]
PRELUDE = r'''
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
typedef uint8_t u8; typedef uint32_t u32;
#define NDS_TASKMAN_ARENA_SIZE 0x1a7000u
#define NDS_TASKMAN_ARENA_ALIGN_SLACK 0x400u
static void *sNdsTaskmanArenaAlloc;
static u8 *sNdsTaskmanArenaBytes;
static u32 gNdsTaskmanArenaChosenSize, gNdsTaskmanArenaAllocFailCount;
static u32 gNdsTaskmanArenaRefineBytes, gNdsTaskmanArenaPreTrimTop;
static void ndsTaskmanLibcResetAfterShrink(void) {}
/* capacity: bytes from the arena's chunk to the heap end. waste: the free
 * top chunk newlib's extend_top does not count until it is trimmed. */
static size_t capacity, waste, allocated, resized;
static int trims;
typedef struct { u32 f[9]; u32 keepcost; } NDSNewlibMallinfo;
static NDSNewlibMallinfo mallinfo(void) {
    NDSNewlibMallinfo m = {{0}, 0};
    m.keepcost = (u32)(allocated ? capacity - allocated : waste);
    return m;
}
static int malloc_trim(size_t pad) { (void)pad; waste = 0; trims++; return 1; }
static void *mock_calloc(size_t count, size_t bytes) {
    if (count != 1 || bytes + waste > capacity) return NULL;
    allocated = bytes;
    return (void *)(uintptr_t)0x10000008u;
}
static void *mock_realloc(void *ptr, size_t bytes) {
    if (!ptr || bytes > allocated) abort();
    resized = bytes;
    allocated = bytes;
    return ptr;
}
static void mock_free(void *ptr) { (void)ptr; allocated = 0; }
#define calloc mock_calloc
#define realloc mock_realloc
#define free mock_free
'''
MAIN = r'''
int main(int argc, char **argv) {
    if (argc != 3) return 2;
    capacity = (size_t)strtoul(argv[1], NULL, 0);
    waste = (size_t)strtoul(argv[2], NULL, 0);
    u8 *first = ndsTaskmanArenaBytes();
    if (ndsTaskmanArenaBytes() != first) return 3;
    printf("%u %zu %u %d\n", gNdsTaskmanArenaChosenSize,
           first ? capacity - resized : 0u,
           first ? (unsigned)((uintptr_t)first & 0x3ffu) : 0u, trims);
    return 0;
}
'''


def define(source, name):
    match = re.search(rf'#define\s+{name}\s+(.+)', source)
    if match is None:
        raise AssertionError(f'Missing #define {name}')
    return match.group(0)


class ArenaCapacityTests(unittest.TestCase):
    def test_fixed_libc_top_whatever_the_untrimmed_top(self):
        cc = shutil.which('gcc') or shutil.which('clang')
        self.assertIsNotNone(cc, 'Host compiler required')
        source = (ROOT / 'src/port/diagnostics_taskman_heap.c').read_text()
        reserve = int(re.search(
            r'#define\s+NDS_TASKMAN_LIBC_RUNTIME_RESERVE\s+0x([0-9A-Fa-f]+)u',
            source).group(1), 16)
        top_define = define(source, 'NDS_TASKMAN_LIBC_TOP_BYTES')
        body = function(source, 'ndsTaskmanArenaBytes')
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            cfile, exe = path / 'arena.c', path / 'arena.exe'
            cfile.write_text(
                PRELUDE +
                f'\n#define NDS_TASKMAN_LIBC_RUNTIME_RESERVE 0x{reserve:X}u\n' +
                top_define + '\n' + body + MAIN)
            subprocess.run([cc, '-std=c11', str(cfile), '-o', str(exe)],
                           check=True, capture_output=True)
            extra_top = int(re.search(
                r'NDS_TASKMAN_LIBC_RUNTIME_RESERVE\s*\+\s*0x([0-9A-Fa-f]+)u',
                top_define).group(1), 16)
            libc_top = reserve + extra_top
            for capacity in (0x3ffff, 0x40410, 0x80410, 0xe0910,
                             0x112410, 0x12c560, 0x150abc, 0x200000):
                page = min(0x1a7000, (capacity - 0x400) & ~0xfff)
                refine = next((e for e in range(0xf00, 0, -0x100)
                               if page + e + 0x400 <= capacity), 0)
                top_after = capacity - (page + refine + 0x400)
                give = ((libc_top - top_after + 0xff) & ~0xff
                        if top_after < libc_top else 0)
                expected = page + refine - give if page >= 0x40000 else 0
                for waste in (0, 0x3960, 0x5200):
                    with self.subTest(capacity=capacity, waste=waste):
                        values = list(map(int, subprocess.check_output(
                            [str(exe), str(capacity), str(waste)],
                            text=True).split()))
                        # One trim per chooser run; a failed run is retried.
                        self.assertEqual(values[3], 1 if expected else 2)
                        self.assertEqual(values[0], expected)
                        if expected:
                            self.assertEqual(values[2], 0)
                            if give:
                                self.assertGreaterEqual(values[1], libc_top)
                                self.assertLess(values[1], libc_top + 0x100)


if __name__ == '__main__':
    unittest.main()
