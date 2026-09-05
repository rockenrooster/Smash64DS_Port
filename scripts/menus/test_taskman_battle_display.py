"""Host-execute the real DS task-entry wrapper with scene/hardware boundaries mocked."""
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def wrapper_source():
    source = (ROOT / "src/import/battleship_sys_taskman.c").read_text()
    signature = "void syTaskmanStartTask(SYTaskmanSetup *tsetup)\n{"
    return signature + source.split(signature, 1)[1].split("\n}\n", 1)[0] + "\n}\n"


def host_source(wrapper):
    registry = (ROOT / "src/port/nds_scene_manager.c").read_text()
    cases = []
    for kind, battle in (("1PMode", 0), ("1PIntro", 0), ("VSBattle", 1),
                         ("1PGame", 1), ("1PBonusStage", 1),
                         ("1PTrainingMode", 1), ("AutoDemo", 1), ("Explain", 1)):
        row = re.search(r"\{\s*\(u8\)nSCKind" + kind + r",\s*([^,]+),", registry)
        if row is None:
            raise AssertionError(f"Missing scene descriptor: {kind}")
        cases.append("{" + row[1].strip() + f", {battle}" + "}")
    return r'''
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
typedef uint32_t u32;
typedef struct { void *arena_start; size_t arena_size; } SceneSetup;
typedef struct { SceneSetup scene_setup; int budgeted; } SYTaskmanSetup;
typedef struct { u32 flags; } NdsSceneDesc;
enum { FALSE = 0, TRUE = 1, NDS_SCENE_FLAG_ARENA_RESET = 1,
       NDS_SCENE_FLAG_MENU = 2, NDS_SCENE_FLAG_BATTLE = 4 };
static struct { u32 scene_curr; } gSCManagerSceneData;
static char arena[32], old_arena[8];
static NdsSceneDesc descriptor;
static int registered, expected_battle, enables, entered, ran, exited;
static int rebudgets, expect_rebudget;
static const NdsSceneDesc *ndsSceneManagerFind(u32 kind)
{ (void)kind; return registered ? &descriptor : NULL; }
static void *ndsTaskmanArenaStart(void) { return arena; }
static size_t ndsTaskmanArenaSize(void) { return sizeof(arena); }
static void ndsPlatformSet3DLayerEnabled(int enabled)
{ assert(enabled == TRUE && !entered && !ran); enables++; }
static int ndsBattleSetupIsRebudgeted(const SYTaskmanSetup *setup)
{ return setup->budgeted; }
static void ndsBattleRebudgetSceneSetup(SYTaskmanSetup *setup)
{ assert(!entered); rebudgets++; setup->budgeted = TRUE; }
static void ndsSceneManagerEnter(const void *start, u32 size)
{
    assert(enables == expected_battle && !entered && !ran);
    assert(start == (registered ? arena : old_arena));
    assert(size == (registered ? sizeof(arena) : sizeof(old_arena)));
    entered++;
}
static void ndsBaseSyTaskmanStartTask(SYTaskmanSetup *setup)
{
    assert(entered == 1 && !ran && !exited);
    if (expected_battle) assert(setup->budgeted);
    assert(rebudgets == expect_rebudget);
    ran++;
}
static void ndsSceneManagerExit(void)
{ assert(entered == 1 && ran == 1 && !exited); exited++; }
''' + wrapper + r'''
int main(void)
{
    const struct { u32 flags; int battle; } cases[] = {
''' + ",\n".join(cases) + r'''
    };
    for (size_t i = 0; i <= sizeof(cases) / sizeof(cases[0]); i++)
    {
        registered = i < sizeof(cases) / sizeof(cases[0]);
        descriptor.flags = registered ? cases[i].flags : 0;
        expected_battle = registered ? cases[i].battle : 0;
        for (int budgeted = 0; budgeted <= 1; budgeted++)
        {
            for (int entry = 0; entry < 2; entry++)
            {
                SYTaskmanSetup setup = {{old_arena, sizeof(old_arena)}, budgeted};
                SYTaskmanSetup original = setup;
                enables = entered = ran = exited = rebudgets = 0;
                expect_rebudget = expected_battle && !budgeted;
                syTaskmanStartTask(&setup);
                assert(enables == expected_battle && exited == 1);
                assert(memcmp(&setup, &original, sizeof(setup)) == 0);
            }
        }
    }
    return 0;
}
'''


class TaskmanBattleDisplayTest(unittest.TestCase):
    def test_scene_entry_and_reentry(self):
        compiler = next((shutil.which(c) for c in ("clang", "gcc", "cc")
                         if shutil.which(c)), None)
        self.assertIsNotNone(compiler, "Host C compiler required (clang/gcc/cc)")
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "taskman_display.c"
            program = Path(directory) / "taskman_display.exe"
            source.write_text(host_source(wrapper_source()))
            subprocess.run([compiler, "-std=c11", "-Wall", "-Wextra", "-Werror",
                            str(source), "-o", str(program)], check=True)
            subprocess.run([str(program)], check=True)


if __name__ == "__main__":
    unittest.main()
