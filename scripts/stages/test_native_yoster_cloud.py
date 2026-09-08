"""Host C execution verification for the integrated Yoster cloud renderer.

The previous --prove was a Python shape proof: it never compiled the
executor and could not catch compile/wiring bugs (nested Jungle gate,
#ifndef on a C variable, undefined private arrays, missing dispatch, skipped
kind48). This test compiles the REAL production packet tables and the REAL
executor ndsRendererSubmitNativeYosterCloud into a host harness and executes
tri emission per alpha, distinct MVP generations, format/material words,
corner/run/uv ranges, and fail-closed paths. Camera-facing (kind48) math is
verified against the cited source tokens plus a host float mirror of the
Mod1/yaw-removal construction. No ROM, no emulator.
"""

from __future__ import annotations

import re
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

_p = Path(__file__).resolve().parent
while _p.name != "scripts":
    _p = _p.parent
sys.path.insert(0, str(_p))
import _paths  # noqa: E402,F401

import generate_nds_native_yoster_clouds as clouds  # noqa: E402

REPO_ROOT = _paths.REPO_ROOT
PACKET_PATH = REPO_ROOT / "src/nds/generated/nds_native_actor_yoster_cloud.generated.inc"
HEADER_PATH = REPO_ROOT / "include/nds/nds_native_actor_yoster_cloud.h"
EXEC_PATH = REPO_ROOT / "src/nds/nds_native_actor_yoster_cloud.exec.inc"
ADAPTER_PATH = REPO_ROOT / "src/port/renderer_adapter_matrix.c"
ROUTE_PATH = REPO_ROOT / "src/port/reloc_backend_movement.c"
LOGIC_PATH = REPO_ROOT / "decomp/BattleShip-main/decomp/src/gr/grcommon/gryoster.c"
OBJDISPLAY_PATH = REPO_ROOT / "decomp/BattleShip-main/decomp/src/sys/objdisplay.c"
MATRIX_PATH = REPO_ROOT / "decomp/BattleShip-main/decomp/src/sys/matrix.c"


def _token(path: Path, token: str, fix: str) -> None:
    text = path.read_text()
    assert token in text, (
        f"DEFECT {path.name} missing {token!r}. Exact fix: {fix}")


def test_cloud_packet_regenerates_byte_exact():
    assert PACKET_PATH.is_file(), "run generate_nds_native_yoster_clouds.py --emit-packet"
    parsed = clouds.decode()
    assert not clouds.FAILURES, clouds.FAILURES
    text, _counts, _slab = clouds.render_packet(parsed)
    assert PACKET_PATH.read_text() == text, (
        "packet drifted: re-run --emit-packet and review the diff")


def test_cloud_decode_pins():
    parsed = clouds.decode()
    assert not clouds.FAILURES, clouds.FAILURES
    assert parsed["corners"] == [2, 1, 0, 0, 3, 2]
    assert len(parsed["tris"]) == 2 and len(parsed["dense"]) == 4
    assert len(parsed["setup_a"]) == 12 and len(parsed["setup_b"]) == 4
    assert len(parsed["finish"]) == 5


def test_cloud_real_executor_host_execution(tmp_path):
    """Compile the actual executor + packet; run tri/alpha/generation/range/failure checks."""
    compiler = shutil.which("gcc") or shutil.which("clang")
    assert compiler is not None, "host C check requires gcc or clang"
    header_defs = "\n".join(
        line for line in HEADER_PATH.read_text().splitlines()
        if line.startswith("#define NDS_NATIVE_ACTOR_YOSTER_CLOUD_"))
    packet = PACKET_PATH.read_text()
    executor = EXEC_PATH.read_text()
    executor = re.sub(r"^#include .*\n", "", executor, flags=re.MULTILINE)
    harness = (Path(__file__).with_name(
        "native_actor_yoster_cloud_host.c")).read_text()
    test_c = tmp_path / "yoster_cloud.c"
    test_c.write_text(harness.replace(
        "/* NATIVE_ACTOR_IMPLEMENTATION */",
        header_defs + "\n" + packet + "\n" + executor))
    executable = tmp_path / "yoster_cloud.exe"
    build = subprocess.run(
        [compiler, "-std=c11", "-Werror=implicit-function-declaration",
         str(test_c), "-o", str(executable), "-lm"],
        capture_output=True, text=True)
    assert build.returncode == 0, (
        f"DEFECT cloud executor/packet does not host-compile. "
        f"Exact fix: read the error, fix the owning source, re-run. "
        f"{build.stdout}{build.stderr}")
    run = subprocess.run([str(executable)], capture_output=True, text=True)
    assert run.returncode == 0, (
        f"DEFECT real C executor failed closed wrong. Exact fix: map the "
        f"FAIL line to the executor branch, fix the owning source. "
        f"{run.stdout}{run.stderr}")
    assert "CLOUD-HOST-OK" in run.stdout


def test_cloud_header_counts_match_production():
    text = HEADER_PATH.read_text()
    for name, want in clouds.HEADER_WANTS.items():
        match = re.search(rf"#define\s+{name}\s+(0x[0-9a-fA-F]+|\d+)u?", text)
        assert match is not None and int(match.group(1), 0) == want, (
            f"DEFECT header {name} must be {want}. Exact fix: correct the "
            f"define in {HEADER_PATH.name}, never the generator")


def test_cloud_adapter_dispatch_wiring():
    """Adapter must compute live drawable MVPs and dispatch kind48, not skip it."""
    adapter = ADAPTER_PATH.read_text()
    for token, fix in (
        ("ndsRendererAdapterSubmitNativeYosterCloud",
         "restore the adapter slot in renderer_adapter_matrix.c"),
        ("ndsRendererAdapterApplyMvpRecalc(draws[i], nGCMatrixKind48, cobj",
         "restore the per-drawable kind48 dispatch in the cloud slot"),
        ("ndsRendererSubmitNativeYosterCloud(loaded->data, loaded->data_size,",
         "restore the executor call forwarding drawable_mvps + live alphas"),
        ("alphas[i] = (u32)draws[i]->mobj->sub.primcolor.s.a",
         "restore live prim-alpha reads from the drawable MObjs"),
        ("NDS_NATIVE_ACTOR_YOSTER_CLOUD_XOBJ_KIND1 48u",
         "restore kind48 in the header child-XObj allowlist"),
    ):
        assert token in adapter or token in HEADER_PATH.read_text(), (
            f"DEFECT missing {token!r}. Exact fix: {fix}")
    _token(ADAPTER_PATH, "ndsRendererAdapterBuildDObjLocalMatrix(joint,",
           "rebuild live locals (Tra+kind48 recalc) instead of baking worlds")
    _token(ADAPTER_PATH, "case nGCMatrixKind48:",
           "restore kind48 in the recalc-local builder")
    _token(ADAPTER_PATH, "(kind == nGCMatrixKind48)",
           "restore kind48 in ndsRendererAdapterIsMvpRecalcKind")


def test_cloud_route_and_executor_wiring():
    route = ROUTE_PATH.read_text()
    assert "ndsRendererAdapterSubmitNativeYosterCloud(root, cobj," in route, (
        "DEFECT cloud route bypasses the adapter slot. Exact fix: route the "
        "cloud display callback through ndsRendererAdapterSubmitNativeYosterCloud")
    assert "draws[i]->mobj" not in route, (
        "route must not reach into drawables; the adapter owns alpha reads")
    executor = EXEC_PATH.read_text()
    assert "ndsRendererNextMatrixGeneration()" in executor, (
        "DEFECT executor reuses one generation. Exact fix: take a fresh "
        "generation per drawable so GX loads 3 distinct MVPs")
    assert executor.count(
        "ndsRendererLoadHardwareRawComposedMatrix(&drawable_mvps[d], generation)"
    ) == 1, (
        "DEFECT executor must load each drawable MVP through "
        "LoadHardwareRawComposedMatrix. Exact fix: restore the per-drawable load")
    assert "stats->prim_color = 0xffffff00u | (alphas[d] & 0xffu)" in executor, (
        "DEFECT executor drops live material alpha. Exact fix: forward "
        "alphas[d] through stats->prim_color into ndsRendererHardwareAlpha")


def test_cloud_kind48_math_against_source():
    """Cited tokens the host kind48 mirror depends on; drift fails here first."""
    adapter = ADAPTER_PATH.read_text()
    for token in (
        "f32 dx = cobj->vec.at.x - cobj->vec.eye.x",
        "f32 dz = cobj->vec.at.z - cobj->vec.eye.z",
        "f32 eye_z = sqrtf((dz * dz) + (dx * dx))",
        "syMatrixLookAtF(&zrot_f, 0.0F, cobj->vec.eye.y, eye_z,",
        "0.0F, cobj->vec.at.y, 0.0F, 0.0F, 1.0F, 0.0F)",
        "guMtxCatF(zrot_f, perspective_f, zrot_f)",
        "if (eye_z < 0.0001F)",
        "recalc_scale_x = parent_scale_x * dobj->scale.vec.f.x",
        "recalc_scale_y = parent_scale_x * dobj->scale.vec.f.y",
        "f32 scale = (row == 1u) ? recalc_scale_y : recalc_scale_x",
    ):
        assert token in adapter, (
            f"DEFECT adapter kind48 math changed at {token!r}. Exact fix: "
            f"restore the Mod1 construction or update the host mirror + tokens together")
    objdisplay = OBJDISPLAY_PATH.read_text()
    for token in (
        "eye_z = sqrtf(SQUARE(cobj->vec.at.z - cobj->vec.eye.z) + SQUARE(cobj->vec.at.x - cobj->vec.eye.x))",
        "syMatrixLookAtF(&sGCMatrixMod1F, 0.0F, eye_y, eye_z, 0.0F, at_y, 0.0F, 0.0F, 1.0F, 0.0F)",
        "guMtxCatF(sGCMatrixMod1F, gGCMatrixPerspF, sGCMatrixMod1F)",
        "sGCMatrixMvpF[1][0] = sGCMatrixMod1F[1][0] * f12",
        "sGCMatrixMvpF[0][0] = sGCMatrixMod1F[0][0] * gGCScaleX",
    ):
        assert token in objdisplay, (
            f"source contract moved at {token!r}; re-derive the adapter mirror")
    matrix = MATRIX_PATH.read_text()
    assert "Right = Up x Look" in matrix, "LookAt source anchor moved"
    logic = LOGIC_PATH.read_text()
    for token in ("gcAddChildForDObj", "llGRYosterMapCloudDisplayList",
                  "nGCMatrixKindTra", "nGCMatrixKind48",
                  "lbCommonAddMObjForTreeDObjs",
                  "mpCollisionSetYakumonoPosID",
                  "mobj->sub.primcolor.s.a" if "mobj->sub.primcolor.s.a" in logic
                  else "dGRYosterCloudMatAnimJoints"):
        assert token in logic, (
            f"DEFECT gryoster.c lost {token!r}; cloud hierarchy/material contract broken")


# NOT-PROVED (residual gaps this file deliberately does not cover; see
# handoff for the full statement):
NOT_PROVED = (
    "adapter slot (SubmitNativeYosterCloud + ApplyMvpRecalc) needs DObj/CObj/heap: "
    "token-pinned only, not host-executed;",
    "syMatrixPerspFastF/gGCMatrixPerspF internals: consumed as opaque words, "
    "not re-derived on host;",
    "GX byte output and frame timing: needs ROM/emulator, out of scope;",
    "yakumono pose/collision/evaporate state machine: source logic, not executed;",
)

def test_cloud_not_proved_footprint():
    """Documents the residual gap; fails if the gap statement is deleted."""
    assert len(NOT_PROVED) == 4
    assert "token-pinned only" in NOT_PROVED[0]
    assert "opaque words" in NOT_PROVED[1]
    assert "ROM/emulator" in NOT_PROVED[2]
    assert "not executed" in NOT_PROVED[3]
