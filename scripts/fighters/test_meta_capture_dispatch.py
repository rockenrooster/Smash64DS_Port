"""Execute the actual capture wrapper: Meta's exemption belongs to the captor."""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class MetaCaptureDispatch(unittest.TestCase):
    def test_captor_policy_preserves_vanilla_dk_and_victim_identity(self):
        compiler = shutil.which("gcc") or shutil.which("clang")
        if not compiler:
            self.skipTest("host C compiler unavailable")
        source = (ROOT / "src/port/reloc_backend_compat_shims.c").read_text()
        match = re.search(r"^void ftCommonCaptureShoulderedProcInterrupt\(GObj \*fighter_gobj\)\s*\{",
                          source, re.MULTILINE)
        begin = source.index("{", match.start())
        depth, end = 1, begin + 1
        while depth:
            depth += (source[end] == "{") - (source[end] == "}")
            end += 1
        wrapper = source[match.start():end]
        unit = r'''
#include <assert.h>
#include <stddef.h>
#define NDS_P4_METAKNIGHT 1
#define NDS_P4_RUNTIME_METAKNIGHT 29
#define FALSE 0
typedef int s32;
typedef struct GObj GObj;
typedef struct FTStruct { s32 fkind; GObj *capture_gobj; } FTStruct;
struct GObj { FTStruct *fighter; };
#define ftGetStruct(gobj) ((gobj)->fighter)
static unsigned ordinary_calls, meta_calls;
static GObj *expected_victim;
static void ndsBaseFTCommonCaptureShoulderedProcInterrupt(GObj *gobj) {
    assert(gobj == expected_victim); ordinary_calls++;
}
static int ndsMetaKnightCaptureDKInterrupt(GObj *gobj) {
    assert(gobj == expected_victim); meta_calls++; return 1;
}
''' + wrapper + r'''
int main(void) {
    FTStruct victim = { 0, NULL }, captor = { 29, NULL };
    GObj victim_obj = { &victim }, captor_obj = { &captor };
    victim.capture_gobj = &captor_obj; expected_victim = &victim_obj;
    ftCommonCaptureShoulderedProcInterrupt(&victim_obj);
    assert(meta_calls == 1 && ordinary_calls == 0 && victim.fkind == 0);
    victim.fkind = 29; captor.fkind = 2;
    ftCommonCaptureShoulderedProcInterrupt(&victim_obj);
    assert(meta_calls == 1 && ordinary_calls == 1 && victim.fkind == 29);
    victim.fkind = 0; captor.fkind = 2;
    ftCommonCaptureShoulderedProcInterrupt(&victim_obj);
    assert(meta_calls == 1 && ordinary_calls == 2);
    victim.capture_gobj = NULL;
    ftCommonCaptureShoulderedProcInterrupt(&victim_obj);
    assert(meta_calls == 1 && ordinary_calls == 3);
    return 0;
}
'''
        with tempfile.TemporaryDirectory(prefix="meta-capture-") as directory:
            folder = Path(directory)
            path = folder / "dispatch.c"
            executable = folder / "dispatch.exe"
            path.write_text(unit, encoding="utf-8")
            built = subprocess.run([compiler, "-std=c11", "-Wall", "-Wextra", "-Werror",
                                    str(path), "-o", str(executable)], capture_output=True, text=True)
            self.assertEqual(built.returncode, 0, built.stderr)
            subprocess.run([str(executable)], check=True, capture_output=True, text=True)


if __name__ == "__main__":
    unittest.main()
