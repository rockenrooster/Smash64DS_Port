"""Native-only gate controls compile/link ARM objects; they never package ROMs."""
import importlib.util
import shutil
import subprocess
from pathlib import Path

import pytest

SCRIPT = Path(__file__).with_name("check_native_only_rom.py")
spec = importlib.util.spec_from_file_location("native_gate", SCRIPT)
gate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)
CC = shutil.which("arm-none-eabi-gcc") or "C:/devkitPro/devkitARM/bin/arm-none-eabi-gcc.exe"


def compile_object(directory, name, text, dependencies=True):
    directory.mkdir(parents=True, exist_ok=True)
    source = directory / (name + ".c")
    obj = directory / (name + ".o")
    source.write_text(text)
    command = [CC, "-mcpu=arm946e-s", "-marm", "-ffreestanding", "-c", str(source), "-o", str(obj)]
    if dependencies:
        command += ["-MMD", "-MP", "-MF", str(obj.with_suffix(".d"))]
    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return obj


def link(directory, objects, success=True):
    elf = directory / "control.elf"
    result = subprocess.run([CC, "-nostdlib", "-Wl,-e,native_entry", *map(str, objects),
                             "-o", str(elf)], capture_output=True, text=True)
    if success:
        assert result.returncode == 0, result.stderr
    else:
        assert result.returncode != 0 and "undefined reference" in result.stderr
    return elf


def test_native_object_passes(tmp_path):
    obj = compile_object(tmp_path, "native", "void native_entry(void) {}")
    assert gate.audit(link(tmp_path, [obj]), [obj], tmp_path) == []


def test_forbidden_reference_cannot_be_accepted(tmp_path):
    caller = compile_object(tmp_path, "caller", """
extern void ndsRendererExecuteDisplayListWithVertexCache(void);
void native_entry(void) { ndsRendererExecuteDisplayListWithVertexCache(); }
""")
    link(tmp_path, [caller], success=False)
    # Even deliberately supplying that implementation cannot pass the binary gate.
    contamination = compile_object(tmp_path, "interpreter", """
void ndsRendererExecuteDisplayListWithVertexCache(void) {}
""")
    failures = gate.audit(link(tmp_path, [caller, contamination]), [caller, contamination], tmp_path)
    assert any("forbidden linked definition" in error for error in failures)
    assert any("compiler inputs" in error for error in failures)


def test_renamed_host_implementation_in_manifest_fails(tmp_path):
    native = compile_object(tmp_path, "native", "void native_entry(void) {}")
    hidden = compile_object(tmp_path / "src/host/graphics_reference", "renamed",
                            "void innocuous_name(void) {}")
    # No forbidden symbol: the actual compiler input still exposes the host unit.
    failures = gate.audit(link(tmp_path, [native, hidden]), [native, hidden], tmp_path)
    assert any("host graphics implementation" in error for error in failures)


def test_executable_object_without_input_provenance_fails(tmp_path):
    obj = compile_object(tmp_path, "unknown", "void native_entry(void) {}", False)
    assert any("lacks compiler dependencies" in error
               for error in gate.audit(link(tmp_path, [obj]), [obj], tmp_path))


def test_empty_or_non_arm_inputs_fail(tmp_path):
    fake = tmp_path / "fake.elf"
    fake.write_bytes(b"not ELF")
    assert gate.audit(fake, [], tmp_path)
    with pytest.raises(ValueError):
        gate.elf_symbols(fake)


def test_relocatable_object_cannot_masquerade_as_linked_executable(tmp_path):
    obj = compile_object(tmp_path, "native", "void native_entry(void) {}")
    with pytest.raises(ValueError, match="wrong ELF type"):
        gate.audit(obj, [obj], tmp_path)


def test_packaging_uses_unconditional_actual_object_gate():
    make = (SCRIPT.parents[1] / "Makefile").read_text()
    assert "$(OUTPUT).nds: | native-only-rom-check" in make
    assert "native-only-rom-check: $(OUTPUT).elf" in make
    assert "native-rom-objects.list,$(OFILES)" in make


def test_real_reference_header_refuses_arm9(tmp_path):
    header = SCRIPT.parents[1] / "src/host/graphics_reference/nds_renderer_reference.h"
    source = tmp_path / "forbidden.c"
    source.write_text('#include "' + header.as_posix() + '"\n')
    result = subprocess.run([CC, "-DARM9", "-I", str(SCRIPT.parents[1] / "include"),
                             "-E", str(source)], capture_output=True, text=True)
    assert result.returncode != 0
    assert "Reference graphics APIs are host-only" in result.stderr


def test_interpreter_definitions_are_outside_rom_unity():
    root = SCRIPT.parents[1]
    for name in ("nds_renderer_native_fighter_production.c", "nds_renderer_dispatch_profile.c",
                 "nds_renderer_native_common.c", "nds_renderer_dl_core.c"):
        assert not gate.forbidden_definitions(root / "src/nds" / name)
    host = root / "src/host/graphics_reference/nds_renderer_reference.c"
    assert "ndsRendererScanList" in gate.forbidden_definitions(host)
    assert "graphics_reference" not in (root / "src/nds/nds_renderer.c").read_text()


@pytest.mark.parametrize("target", (
    "smash64ds-battle-playable-forensic-hwtri",
    "smash64ds-battle-playable-coarse-triangle-noop-hwtri",
    "smash64ds-battle-playable-coarse-cpu-prep-no-gx-hwtri",
    "smash64ds-battle-playable-coarse-warm-no-upload-hwtri",
))
def test_legacy_renderer_configuration_is_rejected_without_building(target):
    make = shutil.which("make") or "C:/devkitPro/msys2/usr/bin/make.exe"
    result = subprocess.run([make, "--eval=native-policy-probe:",
                             "TARGET=" + target, "native-policy-probe"],
                            cwd=SCRIPT.parents[1], capture_output=True, text=True)
    assert result.returncode != 0
    assert "host-only" in result.stderr
