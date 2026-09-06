"""Run the real arena chooser across main-heap capacities, including campaign.

The old coarse fallback threw away usable pages below the 1.19 MiB range.
The allocator mock models only available bytes; allocation lifetime and the
reserved newlib tail are asserted separately.
"""
from pathlib import Path
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
#define NDS_TASKMAN_LIBC_RUNTIME_RESERVE 0x1000u
static void *sNdsTaskmanArenaAlloc;
static u8 *sNdsTaskmanArenaBytes;
static u32 gNdsTaskmanArenaChosenSize, gNdsTaskmanArenaAllocFailCount;
static size_t capacity, allocated, resized;
static void *mock_calloc(size_t count, size_t bytes) {
    if (count != 1 || bytes > capacity) return NULL;
    allocated = bytes;
    return (void *)(uintptr_t)0x10000003u;
}
static void *mock_realloc(void *ptr, size_t bytes) {
    if (!ptr || bytes > allocated) abort();
    resized = bytes;
    return ptr;
}
static void mock_free(void *ptr) { (void)ptr; }
#define calloc mock_calloc
#define realloc mock_realloc
#define free mock_free
'''
MAIN = r'''
int main(int argc, char **argv) {
    if (argc != 2) return 2;
    capacity = (size_t)strtoul(argv[1], NULL, 0);
    u8 *first = ndsTaskmanArenaBytes();
    if (ndsTaskmanArenaBytes() != first) return 3;
    printf("%u %zu %zu %u\n", gNdsTaskmanArenaChosenSize,
           allocated, resized, first ? (unsigned)((uintptr_t)first & 15u) : 0u);
    return 0;
}
'''


class ArenaCapacityTests(unittest.TestCase):
    def test_largest_page_and_reserved_tail(self):
        cc = shutil.which('gcc') or shutil.which('clang')
        self.assertIsNotNone(cc, 'Host compiler required')
        body = function((ROOT / 'src/port/diagnostics_taskman_heap.c').read_text(),
                        'ndsTaskmanArenaBytes')
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            cfile, exe = path / 'arena.c', path / 'arena.exe'
            cfile.write_text(PRELUDE + body + MAIN)
            subprocess.run([cc, '-std=c11', str(cfile), '-o', str(exe)],
                           check=True, capture_output=True)
            for capacity in (0x3ffff, 0x40010, 0x80010, 0xe0910,
                             0x112010, 0x130010, 0x150abc, 0x200000):
                with self.subTest(capacity=capacity):
                    values = list(map(int, subprocess.check_output(
                        [str(exe), str(capacity)], text=True).split()))
                    page = min(0x1a7000, (capacity - 16) & ~4095)
                    chosen = page - 4096 if page >= 0x40000 else 0
                    self.assertEqual(values[0], chosen)
                    if chosen:
                        self.assertEqual(values[1] - values[2], 4096)
                        self.assertEqual(values[2], chosen + 16)
                        self.assertEqual(values[3], 0)


if __name__ == '__main__':
    unittest.main()
