"""Host-execute the compact first native-render failure publisher."""
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/menus"))
from source_test_helpers import function


def test_first_cause_survives_later_failures_and_count_saturation(tmp_path):
    header = (ROOT / "include/nds/nds_renderer.h").read_text()
    record = re.search(r"typedef struct NDSRendererNativeFailure\s*\{.*?\}\s*NDSRendererNativeFailure;",
                       header, re.S)[0]
    body = function((ROOT / "src/nds/nds_renderer_dispatch_profile.c").read_text(),
                    "ndsRendererRecordNativeFailure")
    source = r'''
#include <assert.h>
#include <stdint.h>
#include <string.h>
typedef uint32_t u32;
#define ARM9 1
''' + record + r'''
volatile NDSRendererNativeFailure gNdsRendererNativeFailure;
static unsigned flushes;
static void DC_FlushRange(const void *p, unsigned bytes)
{
    assert(p == (const void *)&gNdsRendererNativeFailure);
    assert(bytes == 32);
    flushes++;
}
''' + body + r'''
int main(void)
{
    assert(sizeof(NDSRendererNativeFailure) == 32);
    assert(gNdsRendererNativeFailure.count == 0 && flushes == 0);
    ndsRendererRecordNativeFailure(1, 22, 0x00060152, 221, 0x2248, 0x1234, 2);
    NDSRendererNativeFailure first = gNdsRendererNativeFailure;
    assert(first.count == 1 && first.domain == 1 && first.scene == 22);
    assert(first.identity == 0x00060152 && first.status == 221);
    assert(first.root == 0x2248 && first.material == 0x1234 && first.reason == 2);
    ndsRendererRecordNativeFailure(3, 7, 99, 12, 0x5678, 0x9876, 4);
    NDSRendererNativeFailure second = gNdsRendererNativeFailure;
    assert(second.count == 2 && flushes == 2);
    assert(memcmp(&first.domain, &second.domain, 28) == 0);
    gNdsRendererNativeFailure.count = UINT32_MAX;
    ndsRendererRecordNativeFailure(3, 0, 0, 0, 0, 0, 0);
    NDSRendererNativeFailure saturated = gNdsRendererNativeFailure;
    assert(saturated.count == UINT32_MAX && flushes == 3);
    assert(memcmp(&first.domain, &saturated.domain, 28) == 0);
    return 0;
}
'''
    compiler = shutil.which("clang") or shutil.which("gcc")
    assert compiler
    c = tmp_path / "failure.c"
    exe = tmp_path / "failure.exe"
    c.write_text(source)
    result = subprocess.run([compiler, "-std=c11", "-Wall", "-Werror", str(c), "-o", str(exe)],
                            capture_output=True, text=True)
    assert result.returncode == 0, result.stderr[-3000:]
    result = subprocess.run([str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr[-3000:]
