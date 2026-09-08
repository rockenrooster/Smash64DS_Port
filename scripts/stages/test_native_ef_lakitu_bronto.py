"""Host C execution verification for the Lakitu + Bronto efground renderers.

Python shape proof alone is insufficient (it never compiles the executor and
cannot catch wiring bugs: nested stage gates, #ifndef on a C array,
undefined private arrays, missing dispatch, kind72 treated as an ordinary
local). Each test below compiles the REAL production packet tables and the
REAL executor into a host harness and executes tri emission per epoch/frame,
distinct MVP generations, format/material words, corner/run/uv ranges, and
fail-closed paths. The kind72 billboard ROWS function and the kind46 ROWS
function are extracted verbatim from src/port/renderer_adapter_matrix.c and
host-compiled against independent double-precision regroupings of the
800CAB48 (lr +/-1) and objdisplay.c:960 (lr +/-3) constructions over an
angle/scale sweep covering both L/R classes with shared geometry. Adapter
slots, route gates and owners includes are live in the tree and token-pinned
here. No ROM, no emulator.
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

import generate_nds_native_ef_lakitu_bronto as ef  # noqa: E402

REPO_ROOT = _paths.REPO_ROOT


def test_bronto_frame_order_matches_source_sprite_table():
    source = (REPO_ROOT / "decomp/BattleShip-main/decomp/src/relocData/104_StagePupupuFile2.c").read_text()
    table = re.search(r"spritelink_0x31F8\[4\]\s*=\s*\{([^}]+)\}", source)
    assert table is not None
    offsets = [int(value, 16) for value in re.findall(r"Tex_0x([0-9A-Fa-f]+)", table[1])]
    assert [offset for offset, _size in ef.BRONTO_FRAMES] == offsets

LAKITU_PACKET = REPO_ROOT / "src/nds/generated/nds_native_actor_ef_lakitu.generated.inc"
BRONTO_PACKET = REPO_ROOT / "src/nds/generated/nds_native_actor_ef_bronto.generated.inc"
LAKITU_HEADER = REPO_ROOT / "include/nds/nds_native_actor_ef_lakitu.h"
BRONTO_HEADER = REPO_ROOT / "include/nds/nds_native_actor_ef_bronto.h"
LAKITU_EXEC = REPO_ROOT / "src/nds/nds_native_actor_ef_lakitu.exec.inc"
BRONTO_EXEC = REPO_ROOT / "src/nds/nds_native_actor_ef_bronto.exec.inc"
LAKITU_HOST = Path(__file__).with_name("native_actor_ef_lakitu_host.c")
BRONTO_HOST = Path(__file__).with_name("native_actor_ef_bronto_host.c")
BILLBOARD_HOST = Path(__file__).with_name("native_actor_ef_ground_billboard_host.c")
KIND46_HOST = Path(__file__).with_name("native_actor_ef_ground_kind46_host.c")
PATCH = REPO_ROOT / "builds/resume-20260906/ef-lakitu-bronto.patch"
ADAPTER_PATH = REPO_ROOT / "src/port/renderer_adapter_matrix.c"
ROUTE_PATH = REPO_ROOT / "src/port/reloc_backend_movement.c"
OWNERS_PATH = REPO_ROOT / "src/nds/nds_renderer_native_owners.c"
LOGIC_PATH = REPO_ROOT / "decomp/BattleShip-main/decomp/src/ef/efground.c"
OBJDISPLAY_PATH = REPO_ROOT / "decomp/BattleShip-main/decomp/src/sys/objdisplay.c"
LBCOMMON_PATH = REPO_ROOT / "decomp/BattleShip-main/decomp/src/lb/lbcommon.c"


def _build_and_check(tmp_path, name, header, packet, executor, harness, marker):
    compiler = shutil.which("gcc") or shutil.which("clang")
    assert compiler is not None, "host C check requires gcc or clang"
    prefix = ("NDS_NATIVE_ACTOR_EF_LAKITU_" if "lakitu" in name
              else "NDS_NATIVE_ACTOR_EF_BRONTO_")
    header_defs = "\n".join(
        line for line in header.read_text().splitlines()
        if line.startswith(f"#define {prefix}"))
    packet_text = packet.read_text()
    assert "static const" in packet_text, f"{name} packet has no tables"
    executor_text = executor.read_text()
    executor_text = re.sub(r"^#include .*\n", "", executor_text, flags=re.MULTILINE)
    for bad in ("#ifndef sNds", "#ifdef sNds", "#if defined(sNds"):
        assert bad not in executor_text, (
            f"DEFECT {name} executor conditions on a C array ({bad}). "
            f"Exact fix: guard on the _EXEC_INC macro only; arrays are "
            f"always present via the packet include")
    harness_text = harness.read_text()
    test_c = tmp_path / f"{name}.c"
    test_c.write_text(harness_text.replace(
        "/* NATIVE_ACTOR_IMPLEMENTATION */",
        header_defs + "\n" + packet_text + "\n" + executor_text))
    executable = tmp_path / f"{name}.exe"
    build = subprocess.run(
        [compiler, "-std=c11", "-Werror=implicit-function-declaration",
         "-fmax-errors=8",
         str(test_c), "-o", str(executable), "-lm"],
        capture_output=True, text=True, encoding="utf-8", errors="replace")
    assert build.returncode == 0, (
        f"DEFECT {name} executor/packet does not host-compile. "
        f"Exact fix: read the error, fix the owning source, re-run. "
        f"{build.stdout[:2000]}{build.stderr[-2000:]}")
    run = subprocess.run([str(executable)], capture_output=True, text=True,
                         encoding="utf-8", errors="replace")
    assert run.returncode == 0, (
        f"DEFECT real C executor failed closed wrong. Exact fix: map the "
        f"FAIL line to the executor branch, fix the owning source. "
        f"{run.stdout[:2000]}{run.stderr[-2000:]}")
    assert marker in run.stdout


def test_packets_regenerate_byte_exact():
    assert LAKITU_PACKET.is_file(), "run generate_nds_native_ef_lakitu_bronto.py --emit-packet"
    assert BRONTO_PACKET.is_file(), "run generate_nds_native_ef_lakitu_bronto.py --emit-packet"
    lak = ef.decode_lakitu()
    assert not ef.FAILURES, ef.FAILURES
    lak_text, _slab = ef.render_lakitu(lak)
    assert LAKITU_PACKET.read_text() == lak_text, (
        "lakitu packet drifted: re-run --emit-packet and review the diff")
    ef.FAILURES.clear()
    bro = ef.decode_bronto()
    assert not ef.FAILURES, ef.FAILURES
    bro_text, _slab, _tail = ef.render_bronto(bro)
    assert BRONTO_PACKET.read_text() == bro_text, (
        "bronto packet drifted: re-run --emit-packet and review the diff")


def test_decode_pins():
    lak = ef.decode_lakitu()
    assert not ef.FAILURES, ef.FAILURES
    assert lak["quads"]["sub"][1] == [(3, 2, 1), (0, 3, 1)]
    assert lak["quads"]["quad1"][1] == [(3, 2, 1), (0, 3, 1)]
    assert lak["quads"]["quad2"][1] == [(3, 2, 1), (2, 0, 1)]
    ef.FAILURES.clear()
    bro = ef.decode_bronto()
    assert not ef.FAILURES, ef.FAILURES
    assert bro["tris"] == [(3, 2, 1), (0, 3, 1)]
    assert len(bro["dense"]) == 4


def test_lakitu_real_executor_host_execution(tmp_path):
    """Compile the actual Lakitu executor + packet; run epoch/matrix/failure checks."""
    _build_and_check(tmp_path, "ef_lakitu", LAKITU_HEADER, LAKITU_PACKET,
                     LAKITU_EXEC, LAKITU_HOST, "LAKITU-HOST-OK")


def test_lakitu_two_phase_submit_tokens():
    """The executor must prove every epoch resident BEFORE any triangle.

    The partial-emission defect: bind drawable 0, emit it, then fail
    binding 1/2 -> FALSE -> the route reruns the legacy tree over already-
    emitted geometry. The fix shape is structural: a warm pure-resident
    preflight for all three drawables first; on a miss, ONE bounded live
    upload pass (zero triangles); then a PURE reverify over all three
    (a live upload can evict, so three binds alone never imply residency);
    then a rebind-by-name emission loop whose bind call cannot fail.
    Drift back to an interleaved live bind fails here first.
    """
    text = LAKITU_EXEC.read_text()
    resolves = re.findall(r"ndsRendererHardwareResolveResidentTexture\(", text)
    live_binds = re.findall(r"ndsRendererHardwareBindTexture\(", text)
    name_binds = re.findall(r"ndsRendererHardwareBindTextureName\(", text)
    assert len(resolves) == 1, (
        "DEFECT lakitu preflight must call ResolveResidentTexture exactly "
        "once (inside the drawable loop, run for both the warm attempt and "
        "the post-upload reverify). Exact fix: restore the resident "
        "resolve loop")
    assert len(live_binds) == 1, (
        "DEFECT lakitu cold path must call the live (upload-on-miss) bind "
        "exactly once, in a bounded zero-emission upload pass. Exact fix: "
        "restore the single BindTexture upload loop between the pure "
        "attempt and the pure reverify")
    assert len(name_binds) == 1, (
        "DEFECT lakitu emission must rebind by name (void, cannot fail). "
        "Exact fix: keep ndsRendererHardwareBindTextureName in the "
        "emission loop")
    assert text.index("ndsRendererHardwareResolveResidentTexture(") < text.index(
        "ndsRendererHardwareBindTexture(") < text.index(
        "ndsRendererHardwareBeginTriangleBatch("), (
        "DEFECT the pure warm attempt and the bounded upload pass must "
        "both precede the first triangle batch. Exact fix: keep attempt / "
        "upload / reverify before emission")
    assert text.index("ndsRendererHardwareBindTexture(") < text.index(
        "ndsRendererHardwareBindTextureName("), (
        "DEFECT the upload pass must precede the rebind-by-name emission. "
        "Exact fix: keep the live bind loop ahead of the emission loop")
    pre_emit = text.split("ndsRendererHardwareBeginTriangleBatch(")[0]
    assert pre_emit.count("return FALSE") >= 3, (
        "DEFECT warm miss, upload failure, and reverify miss must each own "
        "a fail-closed return before emission. Exact fix: keep all three "
        "FALSE returns ahead of the first triangle batch")


def test_bronto_real_executor_host_execution(tmp_path):
    """Compile the actual Bronto executor + packet; run frame/matrix/failure checks."""
    _build_and_check(tmp_path, "ef_bronto", BRONTO_HEADER, BRONTO_PACKET,
                     BRONTO_EXEC, BRONTO_HOST, "BRONTO-HOST-OK")


def _extract_billboard_rows():
    """Verbatim Rows function body from the production adapter (single source)."""
    text = ADAPTER_PATH.read_text()
    begin = text.index("/* BEGIN-EF-GROUND-BILLBOARD-ROWS")
    impl_start = text.index(
        "static void ndsRendererAdapterEfGroundBillboardRows", begin)
    end = text.index("/* END-EF-GROUND-BILLBOARD-ROWS")
    impl_end = text.rindex("\n}\n", begin, end) + len("\n}\n")
    impl = text[impl_start:impl_end]
    assert "out_rows[0][c]" in impl and "out_rows[2][c]" in impl, (
        "DEFECT billboard Rows extraction missed the row writes. Exact fix: "
        "keep the function between the BEGIN/END markers contiguous")
    assert "(void)" not in impl, (
        "DEFECT billboard Rows is a FALSE stub again. Exact fix: restore the "
        "800CAB48 row construction in renderer_adapter_matrix.c")
    return impl


def _extract_kind46_rows():
    """Verbatim kind46 Rows body from the production adapter (single source)."""
    text = ADAPTER_PATH.read_text()
    begin = text.index("/* BEGIN-EF-GROUND-KIND46-ROWS")
    impl_start = text.index(
        "static void ndsRendererAdapterEfGroundKind46Rows", begin)
    end = text.index("/* END-EF-GROUND-KIND46-ROWS")
    impl_end = text.rindex("\n}\n", begin, end) + len("\n}\n")
    impl = text[impl_start:impl_end]
    assert "out_rows[0][0]" in impl and "out_rows[2][3]" in impl, (
        "DEFECT kind46 Rows extraction missed the row writes. Exact fix: "
        "keep the function between the BEGIN/END markers contiguous")
    assert "(void)" not in impl, (
        "DEFECT kind46 Rows is a FALSE stub again. Exact fix: restore the "
        "objdisplay.c:960 row construction in renderer_adapter_matrix.c")
    return impl


def test_billboard_real_source_host_execution(tmp_path):
    """Compile the ACTUAL adapter Rows fn; sweep it vs an independent oracle."""
    compiler = shutil.which("gcc") or shutil.which("clang")
    assert compiler is not None, "host C check requires gcc or clang"
    impl = _extract_billboard_rows()
    harness_text = BILLBOARD_HOST.read_text()
    assert harness_text.count("ndsRendererAdapterEfGroundBillboardRows") == 2, (
        "DEFECT billboard host must call the real Rows fn (in main) and hold "
        "no second copy. Exact fix: splice the adapter text at the marker; "
        "keep the oracle clearly separate")
    test_c = tmp_path / "ef_billboard.c"
    test_c.write_text(harness_text.replace(
        "/* NATIVE_ACTOR_IMPLEMENTATION */", impl))
    executable = tmp_path / "ef_billboard.exe"
    build = subprocess.run(
        [compiler, "-std=c11", "-Werror=implicit-function-declaration",
         str(test_c), "-o", str(executable), "-lm"],
        capture_output=True, text=True)
    assert build.returncode == 0, (
        f"DEFECT billboard Rows does not host-compile. Exact fix: keep the "
        f"Rows body portable (float + sinf/cosf only). "
        f"{build.stdout}{build.stderr}")
    run = subprocess.run([str(executable)], capture_output=True, text=True)
    assert run.returncode == 0, (
        f"DEFECT billboard Rows mismatches the 800CAB48 oracle. Exact fix: "
        f"map the FAIL line to the row formula, fix the owning source. "
        f"{run.stdout}{run.stderr}")
    assert "BILLBOARD-HOST-OK" in run.stdout


def test_kind46_real_source_host_execution(tmp_path):
    """Compile the ACTUAL adapter kind46 Rows fn; sweep it vs an oracle."""
    compiler = shutil.which("gcc") or shutil.which("clang")
    assert compiler is not None, "host C check requires gcc or clang"
    impl = _extract_kind46_rows()
    harness_text = KIND46_HOST.read_text()
    assert harness_text.count("ndsRendererAdapterEfGroundKind46Rows") == 2, (
        "DEFECT kind46 host must call the real Rows fn (in main) and hold "
        "no second copy. Exact fix: splice the adapter text at the marker; "
        "keep the oracle clearly separate")
    test_c = tmp_path / "ef_kind46.c"
    test_c.write_text(harness_text.replace(
        "/* NATIVE_ACTOR_IMPLEMENTATION */", impl))
    executable = tmp_path / "ef_kind46.exe"
    build = subprocess.run(
        [compiler, "-std=c11", "-Werror=implicit-function-declaration",
         str(test_c), "-o", str(executable), "-lm"],
        capture_output=True, text=True)
    assert build.returncode == 0, (
        f"DEFECT kind46 Rows does not host-compile. Exact fix: keep the "
        f"Rows body portable (float + sinf/cosf only). "
        f"{build.stdout}{build.stderr}")
    run = subprocess.run([str(executable)], capture_output=True, text=True)
    assert run.returncode == 0, (
        f"DEFECT kind46 Rows mismatches the objdisplay.c:960 oracle. "
        f"Exact fix: map the FAIL line to the row formula, fix the owning "
        f"source. {run.stdout}{run.stderr}")
    assert "KIND46-HOST-OK" in run.stdout


def test_live_adapter_slots_are_real():
    """Adapter slots + billboards live in the tree (not FALSE stubs)."""
    adapter = ADAPTER_PATH.read_text()
    for token, fix in (
        ("sb32 ndsRendererAdapterSubmitNativeEfLakitu(void *root_ptr",
         "restore the lakitu adapter slot in renderer_adapter_matrix.c"),
        ("sb32 ndsRendererAdapterSubmitNativeEfBronto(void *root_ptr",
         "restore the bronto adapter slot in renderer_adapter_matrix.c"),
        ("else if (kind == NDS_RENDERER_ADAPTER_EF_GROUND_BILLBOARD_KIND)",
         "restore the kind72 arm in ndsRendererAdapterApplyMvpRecalc"),
        ("static void ndsRendererAdapterEfGroundKind46Rows",
         "restore the kind46 row core beside the kind72 core"),
        ("ndsRendererAdapterEfGroundKind46Rows(perspective_f,",
         "restore the kind46 arm delegation in ndsRendererAdapterApplyMvpRecalc"),
        ("billboard_kind, cobj,",
         "restore the per-drawable live-kind seam dispatch in both slots"),
        ("NDS_NATIVE_ACTOR_EF_LAKITU_XOBJ_KIND_BILLBOARD_46",
         "restore the lakitu lr +/-3 (kind46) allowlist arm"),
        ("NDS_NATIVE_ACTOR_EF_BRONTO_XOBJ_KIND_BILLBOARD_46",
         "restore the bronto lr +/-3 (kind46) allowlist arm"),
        ("ndsRendererSubmitNativeEfLakitu(loaded->data, loaded->data_size,",
         "restore the lakitu executor call in the adapter slot"),
        ("ndsRendererSubmitNativeEfBronto(loaded->data, loaded->data_size,",
         "restore the bronto executor call in the adapter slot"),
        ("texture_id_curr",
         "restore the live MObj frame read in the bronto slot"),
        ("NDS_RENDERER_ADAPTER_EF_GROUND_BILLBOARD_KIND 72u",
         "restore the kind72 recalc-kind define beside the 0x46/0x47 kinds"),
        ("(kind == NDS_RENDERER_ADAPTER_EF_GROUND_BILLBOARD_KIND)",
         "restore kind72 in ndsRendererAdapterIsMvpRecalcKind so "
         "BuildDObjLocalMatrix skips it like the other recalc kinds"),
    ):
        assert token in adapter, f"DEFECT missing {token!r}. Exact fix: {fix}"


def test_live_slot_topology_matches_generator():
    """Slot-local parent tables must equal the generator pins (drift fail-closes)."""
    adapter = ADAPTER_PATH.read_text()
    for name, table, wants in (
        ("sEfLakituParents", 6, ef.LAK_PARENTS),
        ("sEfBrontoParents", 3, ef.BRONTO_PARENTS),
    ):
        match = re.search(
            rf"static const u8 {name}\[{table}\] = \{{([^{{}}]*)\}};", adapter)
        assert match is not None, (
            f"DEFECT slot-local {name} missing from the adapter. Exact fix: "
            f"restore the packet-pinned parent table beside the slot")
        got = tuple(int(v.strip().rstrip("u")) for v in
                    match.group(1).split(",") if v.strip())
        assert got == wants, (
            f"DEFECT {name} drifted from the generator pins: {got} != "
            f"{wants}. Exact fix: re-derive the tree from efground.c "
            f"efGroundSetupEffectDObjs, fix the owning source")
    # XObj allowlists are positional header-kind rules, not tables: all
    # kind rules must be present in each slot, both lr classes (72/46).
    for token in (
        "NDS_NATIVE_ACTOR_EF_LAKITU_XOBJ_KIND_TRA_ROTRPYRSCA",
        "NDS_NATIVE_ACTOR_EF_LAKITU_XOBJ_KIND_TRA_ROTRPYR",
        "NDS_NATIVE_ACTOR_EF_LAKITU_XOBJ_KIND_BILLBOARD",
        "NDS_NATIVE_ACTOR_EF_LAKITU_XOBJ_KIND_BILLBOARD_46",
        "NDS_NATIVE_ACTOR_EF_BRONTO_XOBJ_KIND_TRA_ROTRPYRSCA",
        "NDS_NATIVE_ACTOR_EF_BRONTO_XOBJ_KIND_TRA_ROTRPYR",
        "NDS_NATIVE_ACTOR_EF_BRONTO_XOBJ_KIND_BILLBOARD",
        "NDS_NATIVE_ACTOR_EF_BRONTO_XOBJ_KIND_BILLBOARD_46",
    ):
        assert token in adapter, (
            f"DEFECT slot XObj rule lost {token!r}. Exact fix: restore the "
            f"positional kind check in the slot")


def test_live_route_and_owners():
    """Route gates + owners includes live in the tree."""
    route = ROUTE_PATH.read_text()
    for token, fix in (
        ("ndsStageGCDrawAllLoopIsEfLakitu",
         "restore the lakitu route gate in reloc_backend_movement.c"),
        ("ndsStageGCDrawAllLoopIsEfBronto",
         "restore the bronto route gate in reloc_backend_movement.c"),
        ("ndsStageGCDrawAllLoopSubmitEfLakituDObj(effect_gobj,",
         "restore the lakitu native submit inside SubmitEffectDObj"),
        ("ndsStageGCDrawAllLoopSubmitEfBrontoDObj(effect_gobj,",
         "restore the bronto native submit inside SubmitEffectDObj"),
    ):
        assert token in route, f"DEFECT missing {token!r}. Exact fix: {fix}"
    owners = OWNERS_PATH.read_text()
    for token, fix in (
        ("nds_native_actor_ef_lakitu.exec.inc",
         "restore the owners.c lakitu include"),
        ("nds_native_actor_ef_bronto.exec.inc",
         "restore the owners.c bronto include"),
    ):
        assert token in owners, f"DEFECT missing {token!r}. Exact fix: {fix}"


def test_header_counts_match_production():
    for header, wants in ((LAKITU_HEADER, ef.LAKITU_HEADER_WANTS),
                          (BRONTO_HEADER, ef.BRONTO_HEADER_WANTS)):
        text = header.read_text()
        for name, want in wants.items():
            match = re.search(rf"#define\s+{name}\s+(0x[0-9a-fA-F]+|\d+)u?", text)
            assert match is not None and int(match.group(1), 0) == want, (
                f"DEFECT header {name} must be {want}. Exact fix: correct the "
                f"define in {header.name}, never the generator")


def test_executors_share_no_private_state():
    """Packet tables are TU-local statics; executors are textual includes."""
    for exec_path, packet_sym in (
            (LAKITU_EXEC, "sNdsNativeActorEfLakitu"),
            (BRONTO_EXEC, "sNdsNativeActorEfBronto")):
        text = exec_path.read_text()
        assert "TEXTUAL INCLUDE ONLY" in text
        assert "extern" not in text, (
            f"DEFECT {exec_path.name} reaches across TUs with extern. "
            f"Exact fix: consume only the packet tables above the include")
        assert packet_sym in text


def test_patch_carries_shared_integration():
    """The patch record mirrors the live adapter/route/owners edits."""
    assert PATCH.is_file(), "write builds/resume-20260906/ef-lakitu-bronto.patch first"
    patch = PATCH.read_text()
    for token, fix in (
        ("ndsRendererAdapterSubmitNativeEfLakitu",
         "restore the lakitu adapter slot in the patch"),
        ("ndsRendererAdapterSubmitNativeEfBronto",
         "restore the bronto adapter slot in the patch"),
        ("NDS_RENDERER_ADAPTER_EF_GROUND_BILLBOARD_KIND",
         "restore the kind72 seam kind in the patch"),
        ("ndsRendererSubmitNativeEfLakitu(loaded->data, loaded->data_size,",
         "restore the lakitu executor call in the patch"),
        ("ndsRendererSubmitNativeEfBronto(loaded->data, loaded->data_size,",
         "restore the bronto executor call in the patch"),
        ("texture_id_curr",
         "restore the live MObj frame read in the bronto slot"),
        ("IsEfLakitu", "restore the lakitu route gate in the patch"),
        ("IsEfBronto", "restore the bronto route gate in the patch"),
        ("nds_native_actor_ef_lakitu.exec.inc",
         "restore the owners.c lakitu include in the patch"),
        ("nds_native_actor_ef_bronto.exec.inc",
         "restore the owners.c bronto include in the patch"),
    ):
        assert token in patch, f"DEFECT patch missing {token!r}. Exact fix: {fix}"
    assert "BuildDObjLocalMatrix" in patch, (
        "DEFECT patch must cite why kind72 keeps a dedicated recalc path. "
        "Exact fix: document that BuildDObjLocalMatrix has no camera/persp")


def test_kind72_math_against_source():
    """Cited tokens the host kind72 mirror depends on; drift fails here first."""
    adapter = ADAPTER_PATH.read_text()
    for token in (
        "has_mvp_recalc_rpy_0x47",
        "ndsRendererAdapterIsMvpRecalcKind",
    ):
        assert token in adapter, (
            f"shared adapter contract moved at {token!r}; re-check the patch")
    lbcommon = LBCOMMON_PATH.read_text()
    for token in (
        "func_ovl0_800CAB48",
        "gGCMatrixPerspF[0][0] * cosy",
        "gGCScaleX *= *p",
    ):
        assert token in lbcommon, (
            f"source contract moved at {token!r}; re-derive the kind72 mirror")
    objdisplay = OBJDISPLAY_PATH.read_text()
    assert "mobj->sub.sprites[mobj->texture_id_curr]" in objdisplay, (
        "source frame contract moved; re-derive the bronto hook")
    logic = LOGIC_PATH.read_text()
    for token in ("gcAddXObjForDObjFixed(current_dobj, 0x48, 0)",
                  "gcAddXObjForDObjFixed(current_dobj, 0x2E, 0)",
                  "efGroundSetupEffectDObjs",
                  "efGroundUpdatePhysics",
                  "&llGRCastleMapLakituDObjDesc",
                  "&llGRPupupuMapBrontoDObjDesc"):
        assert token in logic, (
            f"DEFECT efground.c lost {token!r}; actor hierarchy contract broken")


def test_kind46_math_against_source():
    """Cited tokens the host kind46 mirror depends on; drift fails here first."""
    adapter = ADAPTER_PATH.read_text()
    for token in (
        "ndsRendererAdapterEfGroundKind46Rows",
        "BEGIN-EF-GROUND-KIND46-ROWS",
        "END-EF-GROUND-KIND46-ROWS",
        "billboard_kind, cobj,",
    ):
        assert token in adapter, (
            f"shared adapter contract moved at {token!r}; re-check the slot")
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
    for token in ("gcAddXObjForDObjFixed(current_dobj, 0x2E, 0)",
                  "lr_bool",
                  "((lr != -3) && (lr != 3))"):
        assert token in logic, (
            f"DEFECT efground.c lost {token!r}; lr +/-3 arm contract broken")


def test_lr_classes_share_geometry():
    """L/R classes share one DObjDesc + DLs (same geometry); only the live
    billboard kind (lr +/-1 -> 72, lr +/-3 -> 46) and the anim joints differ.

    Source: each actor's two descs point at ONE DObjDesc with distinct
    R/L AnimJoints (efground.c:27-106, :671-749). Port: one slot per actor,
    one DL-offset table, dual-kind allowlist, same packet either way."""
    logic = LOGIC_PATH.read_text()
    assert logic.count("&llGRCastleMapLakituDObjDesc") == 2, (
        "DEFECT Lakitu R/L must share one DObjDesc; re-check efground.c:27-106")
    for joint in ("&llGRCastleMapLakituRAnimJoint",
                  "&llGRCastleMapLakituLAnimJoint"):
        assert logic.count(joint) == 1, (
            f"DEFECT Lakitu class joint {joint!r} moved; re-check efground.c")
    assert logic.count("&llGRPupupuMapBrontoDObjDesc") == 2, (
        "DEFECT Bronto L/R must share one DObjDesc; re-check efground.c:671-749")
    for joint in ("&llGRPupupuMapBrontoLAnimJoint",
                  "&llGRPupupuMapBrontoRAnimJoint"):
        assert logic.count(joint) == 1, (
            f"DEFECT Bronto class joint {joint!r} moved; re-check efground.c")
    adapter = ADAPTER_PATH.read_text()
    assert adapter.count(
        "sb32 ndsRendererAdapterSubmitNativeEfLakitu(void *root_ptr") == 1, (
        "DEFECT one Lakitu slot must serve both classes; no per-class fork")
    assert adapter.count(
        "sb32 ndsRendererAdapterSubmitNativeEfBronto(void *root_ptr") == 1, (
        "DEFECT one Bronto slot must serve both classes; no per-class fork")
    assert "sEfLakituDlOffs[3] = { 0x3f20u, 0x3ff8u, 0x4070u }" in adapter, (
        "DEFECT Lakitu geometry offsets must be class-independent")
    assert "(draw->dv != (void *)((u8 *)loaded->data + 0x32c8u))" in adapter, (
        "DEFECT Bronto geometry offset must be class-independent")


def test_patch_kind72_mirror_tokens():
    """The patch's billboard mirror must show its derivation, not a fallback."""
    patch = PATCH.read_text()
    for token in (
        "800CAB48",
        "persp",
        "cosy",
        "texture_id_curr",
        "FRAME_MAX",
    ):
        assert token in patch, (
            f"DEFECT patch kind72/frame mirror lost {token!r}")


# NOT-PROVED (residual gaps this file deliberately does not cover):
NOT_PROVED = (
    "full adapter slots + route gates: billboard rows host-executed, the "
    "DObj/CObj/heap remainder token-pinned, not host-executed (needs DObj/CObj/heap);",
    "gGCMatrixPerspF internals: consumed as opaque words, not re-derived;",
    "GX byte output and frame timing: needs ROM/emulator, out of scope;",
    "efGround spawn/bounds/eject state machine: source logic, not executed;",
)

def test_not_proved_footprint():
    """Documents the residual gap; fails if the gap statement is deleted."""
    assert len(NOT_PROVED) == 4
    assert "token-pinned" in NOT_PROVED[0]
    assert "opaque words" in NOT_PROVED[1]
    assert "ROM/emulator" in NOT_PROVED[2]
    assert "not executed" in NOT_PROVED[3]
