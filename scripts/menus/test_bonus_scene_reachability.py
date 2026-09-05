"""Guard bonus/boss reloc arithmetic and cold collision-list consumers."""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


def function_body(source, name):
    match = re.search(r"\b" + name + r"\([^;{}]*\)\s*\{", source)
    if match is None:
        raise AssertionError(f"Missing function {name}")
    begin = match.end() - 1
    depth = 1
    for index in range(begin + 1, len(source)):
        depth += (source[index] == "{") - (source[index] == "}")
        if depth == 0:
            return source[begin:index + 1]
    raise AssertionError(f"Unterminated function {name}")


class BonusSceneReachabilityTests(unittest.TestCase):
    def test_arithmetic_offsets_equal_original_linker_symbols(self):
        upstream = (ROOT / "decomp/BattleShip-main/include/reloc_data.us.h").read_text()
        original = dict(re.findall(
            r"#define\s+(ll\w+)\s+\(\(intptr_t\)(0x[0-9A-Fa-f]+)\)", upstream))
        for name, count in (("grbonus3", 4), ("sc1pbonusstage", 17), ("sc1pgameboss", 21)):
            with self.subTest(scene=name):
                source = (ROOT / f"src/import/battleship_{name}.c").read_text()
                rows = re.findall(r"#define\s+(ll\w+)\s+NDS_RELOC_LVALUE\((0x[0-9a-fA-F]+)u\)",
                                  source)
                self.assertEqual(len(rows), count)
                self.assertIn("#define NDS_RELOC_LVALUE(offset) (*(uintptr_t *)(uintptr_t)(offset))",
                              source)
                for symbol, value in rows:
                    self.assertEqual(int(value, 16), int(original[symbol], 16), symbol)

    def test_collision_getters_initialize_before_first_read(self):
        compiler = shutil.which("gcc") or shutil.which("clang")
        if compiler is None:
            self.skipTest("host C compiler required")
        source = (ROOT / "src/port/reloc_backend_mp_collision.c").read_text()
        functions = (
            "s32 mpCollisionGetLineCountType(s32 line_type)" +
            function_body(source, "mpCollisionGetLineCountType") +
            "\nvoid mpCollisionGetLineIDsTypeCount(s32 type, s32 count, s32 *line_ids)" +
            function_body(source, "mpCollisionGetLineIDsTypeCount"))
        unit = r'''
#include <assert.h>
#include <stddef.h>
typedef int s32;
static struct { unsigned short line_count; s32 *line_id; } gMPCollisionLineGroups[4];
static s32 ids[] = { 4, 9, 12 };
static int ready, builds;
static void ndsMPCollisionEnsureLineGroups(void) {
    if (!ready) {
        ++builds;
        ready = 1;
        gMPCollisionLineGroups[0].line_count = 3;
        gMPCollisionLineGroups[0].line_id = ids;
    }
}
static void invalidate(void) {
    ready = builds = 0;
    gMPCollisionLineGroups[0].line_count = 0;
    gMPCollisionLineGroups[0].line_id = NULL;
}
''' + functions + r'''
int main(void) {
    s32 output[] = { -1, -1, -1, 777 };
    invalidate();
    assert(mpCollisionGetLineCountType(0) == 3);
    assert(mpCollisionGetLineCountType(0) == 3 && builds == 1);
    invalidate();
    mpCollisionGetLineIDsTypeCount(0, 3, output);
    assert(output[0] == 4 && output[1] == 9 && output[2] == 12 && output[3] == 777);
    assert(mpCollisionGetLineCountType(0) == 3 && builds == 1);
    assert(mpCollisionGetLineCountType(1) == 0);
    return 0;
}
'''
        with tempfile.TemporaryDirectory(prefix="smash64ds-line-groups-") as directory:
            source_path = Path(directory) / "line_groups.c"
            program = Path(directory) / "line_groups.exe"
            source_path.write_text(unit)
            result = subprocess.run([compiler, "-std=c99", "-O2", "-Wall", "-Werror",
                                     str(source_path), "-o", str(program)],
                                    capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            result = subprocess.run([str(program)], capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
