"""Host C execution verification for the efground billboard row cores.

The kind72 (0x48, func_ovl0_800CAB48, lr +/-1) and kind46 (objdisplay.c:960,
lr +/-3) ROWS functions are extracted verbatim from
src/port/renderer_adapter_matrix.c, where the generic effect walk's
ApplyMvpRecalc arms call them for every efground actor (Lakitu, Bronto Burt
and the baked actors of generate_nds_native_item_baked.py), and
host-compiled against independent double-precision regroupings of the
source constructions over an angle/scale sweep covering both L/R classes.
No ROM, no emulator.
"""

from __future__ import annotations

import shutil
import subprocess
import sys
from pathlib import Path

_p = Path(__file__).resolve().parent
while _p.name != "scripts":
    _p = _p.parent
sys.path.insert(0, str(_p))
import _paths  # noqa: E402,F401
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts/menus"))
from source_test_helpers import source as read_split  # noqa: E402

REPO_ROOT = _paths.REPO_ROOT

BILLBOARD_HOST = Path(__file__).with_name("native_actor_ef_ground_billboard_host.c")
KIND46_HOST = Path(__file__).with_name("native_actor_ef_ground_kind46_host.c")
ADAPTER_PATH = REPO_ROOT / "src/port/renderer_adapter_matrix.c"
LOGIC_PATH = REPO_ROOT / "decomp/BattleShip-main/decomp/src/ef/efground.c"
OBJDISPLAY_PATH = REPO_ROOT / "decomp/BattleShip-main/decomp/src/sys/objdisplay.c"
LBCOMMON_PATH = REPO_ROOT / "decomp/BattleShip-main/decomp/src/lb/lbcommon.c"


def _extract_rows(begin_marker: str, end_marker: str, signature: str,
                  writes: tuple[str, str]) -> str:
    """Verbatim Rows function body from the production adapter (single source)."""
    text = read_split(ADAPTER_PATH)
    begin = text.index(begin_marker)
    impl_start = text.index(signature, begin)
    end = text.index(end_marker)
    impl_end = text.rindex("\n}\n", begin, end) + len("\n}\n")
    impl = text[impl_start:impl_end]
    assert all(w in impl for w in writes), (
        f"DEFECT {signature} extraction missed the row writes. Exact fix: "
        "keep the function between the BEGIN/END markers contiguous")
    assert "(void)" not in impl, (
        f"DEFECT {signature} is a FALSE stub again. Exact fix: restore the "
        "source row construction in renderer_adapter_matrix.c")
    return impl


def _host_run(tmp_path, name: str, host: Path, impl: str, fn: str, marker: str):
    compiler = shutil.which("gcc") or shutil.which("clang")
    assert compiler is not None, "host C check requires gcc or clang"
    harness_text = host.read_text()
    assert harness_text.count(fn) == 2, (
        f"DEFECT {name} host must call the real Rows fn (in main) and hold "
        "no second copy. Exact fix: splice the adapter text at the marker; "
        "keep the oracle clearly separate")
    test_c = tmp_path / f"{name}.c"
    test_c.write_text(harness_text.replace(
        "/* NATIVE_ACTOR_IMPLEMENTATION */", impl))
    executable = tmp_path / f"{name}.exe"
    build = subprocess.run(
        [compiler, "-std=c11", "-Werror=implicit-function-declaration",
         str(test_c), "-o", str(executable), "-lm"],
        capture_output=True, text=True)
    assert build.returncode == 0, (
        f"DEFECT {name} Rows does not host-compile. Exact fix: keep the "
        f"Rows body portable (float + sinf/cosf only). "
        f"{build.stdout}{build.stderr}")
    run = subprocess.run([str(executable)], capture_output=True, text=True)
    assert run.returncode == 0, (
        f"DEFECT {name} Rows mismatches its source oracle. Exact fix: map "
        f"the FAIL line to the row formula, fix the owning source. "
        f"{run.stdout}{run.stderr}")
    assert marker in run.stdout


def test_billboard_real_source_host_execution(tmp_path):
    """Compile the ACTUAL adapter kind72 Rows fn; sweep it vs an oracle."""
    impl = _extract_rows("/* BEGIN-EF-GROUND-BILLBOARD-ROWS",
                         "/* END-EF-GROUND-BILLBOARD-ROWS",
                         "static void ndsRendererAdapterEfGroundBillboardRows",
                         ("out_rows[0][c]", "out_rows[2][c]"))
    _host_run(tmp_path, "ef_billboard", BILLBOARD_HOST, impl,
              "ndsRendererAdapterEfGroundBillboardRows", "BILLBOARD-HOST-OK")


def test_kind46_real_source_host_execution(tmp_path):
    """Compile the ACTUAL adapter kind46 Rows fn; sweep it vs an oracle."""
    impl = _extract_rows("/* BEGIN-EF-GROUND-KIND46-ROWS",
                         "/* END-EF-GROUND-KIND46-ROWS",
                         "static void ndsRendererAdapterEfGroundKind46Rows",
                         ("out_rows[0][0]", "out_rows[2][3]"))
    _host_run(tmp_path, "ef_kind46", KIND46_HOST, impl,
              "ndsRendererAdapterEfGroundKind46Rows", "KIND46-HOST-OK")


def test_kind72_math_against_source():
    """Cited tokens the host kind72 mirror depends on; drift fails here first."""
    adapter = read_split(ADAPTER_PATH)
    for token in ("has_mvp_recalc_rpy_0x47", "ndsRendererAdapterIsMvpRecalcKind"):
        assert token in adapter, (
            f"shared adapter contract moved at {token!r}; re-check the arm")
    lbcommon = LBCOMMON_PATH.read_text()
    for token in ("func_ovl0_800CAB48", "gGCMatrixPerspF[0][0] * cosy",
                  "gGCScaleX *= *p"):
        assert token in lbcommon, (
            f"source contract moved at {token!r}; re-derive the kind72 mirror")
    logic = LOGIC_PATH.read_text()
    for token in ("gcAddXObjForDObjFixed(current_dobj, 0x48, 0)",
                  "gcAddXObjForDObjFixed(current_dobj, 0x2E, 0)",
                  "efGroundSetupEffectDObjs"):
        assert token in logic, (
            f"DEFECT efground.c lost {token!r}; actor hierarchy contract broken")


def test_kind46_math_against_source():
    """Cited tokens the host kind46 mirror depends on; drift fails here first."""
    adapter = read_split(ADAPTER_PATH)
    for token in ("ndsRendererAdapterEfGroundKind46Rows",
                  "BEGIN-EF-GROUND-KIND46-ROWS", "END-EF-GROUND-KIND46-ROWS"):
        assert token in adapter, (
            f"shared adapter contract moved at {token!r}; re-check the arm")
    objdisplay = OBJDISPLAY_PATH.read_text()
    for token in (
        "sGCMatrixMvpF[0][0] = gGCMatrixPerspF[0][0] * gGCScaleX * cosz",
        "sGCMatrixMvpF[1][0] = gGCMatrixPerspF[0][0] * gGCScaleX * -sinz",
        "sGCMatrixMvpF[0][1] = gGCMatrixPerspF[1][1] * f12 * sinz",
        "sGCMatrixMvpF[2][2] = gGCMatrixPerspF[2][2] * gGCScaleX",
        "f12 = dobj->scale.vec.f.y * gGCScaleX",
    ):
        assert token in objdisplay, (
            f"source contract moved at {token!r}; re-derive the kind46 mirror")
    logic = LOGIC_PATH.read_text()
    for token in ("gcAddXObjForDObjFixed(current_dobj, 0x2E, 0)", "lr_bool",
                  "((lr != -3) && (lr != 3))"):
        assert token in logic, (
            f"DEFECT efground.c lost {token!r}; lr +/-3 arm contract broken")
