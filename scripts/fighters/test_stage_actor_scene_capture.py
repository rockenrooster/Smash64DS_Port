"""Execute the real display-entry guards against the registered scene flags."""
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def test_battle_capture_guards_and_scene_lifecycle(tmp_path):
    movement = (ROOT / "src/port/reloc_backend_movement.c").read_text()
    manager = (ROOT / "src/port/nds_scene_manager.c").read_text()
    taskman = (ROOT / "src/import/battleship_sys_taskman.c").read_text()
    header = (ROOT / "include/nds/nds_scene_manager.h").read_text()
    battle_flag = re.search(r"#define NDS_SCENE_FLAG_BATTLE (.+)", header)[1]
    rows = dict(re.findall(r"\{\s*\(u8\)(nSCKind\w+),\s*([^,]+),", manager))
    cases = [(name, int("NDS_SCENE_FLAG_BATTLE" in rows[name])) for name in (
        "nSCKindVSBattle", "nSCKind1PGame", "nSCKind1PTrainingMode",
        "nSCKind1PBonusStage", "nSCKindTitle", "nSCKindMaps")]
    assert [flag for _, flag in cases] == [1, 1, 1, 1, 0, 0]
    # The scene registry publishes the flag before imported task setup/display.
    assert taskman.index("ndsSceneManagerEnter(tsetup") < taskman.index(
        "ndsBaseSyTaskmanStartTask(tsetup)")
    publish = re.search(r"gNdsSceneManagerCurrIsBattle\s*=.*?;", manager, re.S)[0]
    # Skip the global initializer and take the actual Enter assignment.
    publish = re.search(r"gNdsSceneManagerCurrIsBattle\s*=.*?;",
                        manager[manager.index("void ndsSceneManagerEnter("):], re.S)[0]

    def prefix(name, marker):
        start = movement.index(name + "(")
        start = movement.index("{", start) + 1
        end = movement.index(marker, start)
        body = movement[start:end]
        assert "gNdsSceneManagerCurrIsBattle" in body
        assert "nSCKindVSBattle" not in body
        return body.replace("return;", "return FALSE;")

    camera = prefix("ndsStageGCDrawAllLoopRecordCameraCallback",
                    "gNdsStageGCDrawAllLoopCameraCallbackCount++;")
    capture = prefix("ndsStageGCDrawAllLoopRecordCapturedDisplay",
                     "gNdsStageGCDrawAllLoopCapturedDisplayCount++;")
    draw = prefix("ndsStageGCDrawAllLoopRecordDObjDraw",
                  "if (ndsStageGCDrawAllLoopClassifyGObj(")
    present = prefix("ndsStageGCDrawAllLoopPresentHardwareFrame",
                     "#if NDS_FAST_WALLPAPER_AFFINE")
    code = """
#include <assert.h>
#include <stddef.h>
typedef unsigned u32; typedef int s32; typedef int sb32; typedef void GObj;
#define TRUE 1
#define FALSE 0
#define NDS_RENDERER_HW_TRIANGLES 1
#define NDS_RENDERER_PROFILE_LEVEL 0
#define NDS_TICK_HUD 0
#define NDS_RENDERER_M3_PHASE0_PROFILE 0
static u32 gNdsSceneManagerCurrIsBattle, enabled=1, prepared=1;
static u32 sNdsFighterGCDrawAllLoopDisplayActive;
static u32 sNdsStageGCDrawAllLoopHardwareSubmitActive=1;
static u32 gNdsStageGCDrawAllLoopUnexpectedSceneCount;
static u32 gNdsStageGCDrawAllLoopManualDisplayCallCount;
static sb32 ndsFighterMarioFoxStageGCDrawAllLoopProofActive(void)
{ return enabled && prepared; }
"""
    code += f"#define NDS_SCENE_FLAG_BATTLE {battle_flag}\n"
    code += "typedef struct { u32 flags; } Scene;\n"
    code += "static void enter(const Scene *desc) { " + publish + " }\n"
    code += "static int camera(void) {" + camera + "return TRUE;}\n"
    code += "static int capture(void *display_gobj) {" + capture + "return TRUE;}\n"
    code += "static int draw(void *gobj,u32 kind) {" + draw + "return TRUE;}\n"
    code += "static int present(void) {" + present + "return TRUE;}\n"
    code += "int main(void) { Scene scene;\n"
    for name, flag in cases:
        code += f"/* {name} */ scene.flags={flag} ? NDS_SCENE_FLAG_BATTLE : 0; "
        code += f"enter(&scene); assert(camera()=={flag}); assert(capture(NULL)=={flag}); "
        code += f"assert(draw(NULL,0)=={flag}); assert(present()=={flag});\n"
    code += """
enter(NULL); assert(!camera() && !capture(NULL) && !draw(NULL,0) && !present());
scene.flags=NDS_SCENE_FLAG_BATTLE; enter(&scene);
enabled=0; assert(!camera() && !capture(NULL) && !draw(NULL,0)); enabled=1;
prepared=0; assert(!camera() && !capture(NULL) && !draw(NULL,0)); prepared=1;
/* The frame owner clears active after gcDrawAll. A retained scene flag on
 * Exit must not admit a callback after the draw bracket closed. */
sNdsStageGCDrawAllLoopHardwareSubmitActive=0;
assert(!camera() && !capture(NULL) && !draw(NULL,0));
sNdsFighterGCDrawAllLoopDisplayActive=1;
assert(camera() && capture(NULL) && draw(NULL,0));
return 0; }
"""
    compiler = shutil.which("gcc") or shutil.which("clang")
    assert compiler, "scene capture host check requires a C compiler"
    source = tmp_path / "capture.c"
    binary = tmp_path / "capture.exe"
    source.write_text(code)
    subprocess.run([compiler, "-std=c11", str(source), "-o", str(binary)],
                   capture_output=True, check=True)
    subprocess.run([str(binary)], capture_output=True, check=True)
