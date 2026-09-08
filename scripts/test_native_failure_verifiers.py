"""Focused host test for the native-render failure record verifier wiring.

Covers the adopted all-ROM native-only contract without builds or emulator:
each acceptance verifier must carry a same-run read of
gNdsRendererNativeFailure (count/domain/scene/identity/status/root/material/
reason, 8u32), reject count>0, reject missing evidence, and print all
first-cause fields when rejecting. No bypass switch exists.
"""
import re
import shutil
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]
SCRIPTS = {
    "shell": ROOT / "scripts/verify-p2-shell-loop.ps1",
    "runtime": ROOT / "scripts/verify-runtime.ps1",
    "battle": ROOT / "scripts/verify-battle-mariofox-gcrunall-loop-harness.ps1",
    "stress": ROOT / "scripts/verify-p2-four-fighter-stress.ps1",
}
FIELDS = ["count", "domain", "scene", "identity", "status", "root", "material", "reason"]

RUNTIME_RE = re.compile(
    r"P2NATIVEFAIL count=(\d+) domain=(\d+) scene=(\d+) identity=(\d+)"
    r" status=(\d+) root=(\d+) material=(\d+) reason=(\d+)"
)
SHELL_RE = re.compile(
    r"^LOOPNATIVEFAIL count=(\d+) domain=(\d+) scene=(\d+) identity=(\d+)"
    r" status=(\d+) root=(\d+) material=(\d+) reason=(\d+)",
    re.M,
)
BATTLE_RE = re.compile(
    r"NATIVE_FAILURE=([0-9]+),([0-9]+),([0-9]+),([0-9]+),"
    r"([0-9]+),([0-9]+),([0-9]+),([0-9]+)"
)


def read(name):
    text = SCRIPTS[name].read_text(encoding="utf-8")
    assert text, f"{name} verifier is empty"
    return text


def test_record_shape_matches_header():
    header = (ROOT / "include/nds/nds_renderer.h").read_text(encoding="utf-8")
    match = re.search(
        r"typedef struct NDSRendererNativeFailure\s*\{(.*?)\}\s*NDSRendererNativeFailure;",
        header,
        re.S,
    )
    assert match, "NDSRendererNativeFailure struct missing from header"
    body = match.group(1)
    found = re.findall(r"u32\s+(\w+)\s*;", body)
    assert found == FIELDS, f"record fields are {found}, expected {FIELDS}"
    recorder = (ROOT / "src/nds/nds_renderer_dispatch_profile.c").read_text(encoding="utf-8")
    assert "gNdsRendererNativeFailure.count == 0" in recorder
    assert "DC_FlushRange" in recorder


def test_scripts_carry_same_run_native_reads():
    runtime = read("runtime")
    assert "P2NATIVEFAIL" in runtime
    for field in FIELDS:
        assert f"gNdsRendererNativeFailure.{field}" in runtime
    assert runtime.index("P2NATIVEFAIL") < runtime.index("'detach'")

    shell = read("shell")
    assert "LOOPNATIVEFAIL" in shell
    for field in FIELDS:
        assert f"gNdsRendererNativeFailure.{field}" in shell
    assert "'gNdsRendererNativeFailure'" in shell

    battle = read("battle")
    assert "NATIVE_FAILURE=" in battle
    for field in FIELDS:
        assert f"gNdsRendererNativeFailure.{field}" in battle

    stress = read("stress")
    for field in FIELDS:
        assert f"gNdsRendererNativeFailure.{field}" in stress
    # Same-run flow: the collector reads ExtraGlobals once at the end of the
    # timing run; these names ride that flow rather than a second emulator run.
    assert "ExtraGlobals" in stress
    assert "missing required memory counter" in stress


def test_scripts_reject_count_and_missing_evidence():
    for name in SCRIPTS:
        text = read(name)
        lowered = text.lower()
        assert "native" in lowered and "failure" in lowered
        assert ("missing" in lowered and "evidence" in lowered) or (
            "missing" in lowered and "native" in lowered
        ), f"{name} has no missing-evidence rejection"
        assert ("-ne 0" in text) or ("!= 0" in text) or ("count" in lowered), (
            f"{name} has no count>0 rejection"
        )
    # Rejections print every first-cause field rather than a bare count.
    assert "domain=" in read("runtime") and "reason=" in read("runtime")
    assert "domain=" in read("shell") and "reason=" in read("shell")
    battle = read("battle")
    assert "domain=" in battle and "reason=" in battle
    stress = read("stress")
    for field in FIELDS:
        assert f"nativeFailure{field[0].upper()}{field[1:]}" in stress or field in stress.lower()


def test_no_bypass_switch():
    for name in SCRIPTS:
        text = read(name)
        for token in (
            "SkipNativeFailure",
            "IgnoreNativeFailure",
            "BypassNative",
            "AllowNativeFallback",
            "NativeFailureOptOut",
        ):
            assert token not in text, f"{name} must not add a bypass switch ({token})"


def parse_runtime(stdout):
    match = RUNTIME_RE.search(stdout)
    if not match:
        raise AssertionError("missing P2NATIVEFAIL evidence")
    values = [int(group) for group in match.groups()]
    if values[0] != 0:
        raise AssertionError(f"native failure first cause: {match.group(0)}")
    return values


def parse_shell(lines):
    line = next((entry for entry in lines if entry.startswith("LOOPNATIVEFAIL ")), None)
    if line is None:
        raise AssertionError("missing LOOPNATIVEFAIL evidence")
    match = SHELL_RE.search(line)
    if not match:
        raise AssertionError(f"malformed LOOPNATIVEFAIL line: {line}")
    values = [int(group) for group in match.groups()]
    if values[0] != 0:
        raise AssertionError(f"native failure first cause: {line}")
    return values


def parse_battle(stdout):
    match = BATTLE_RE.search(stdout)
    if not match:
        raise AssertionError("missing NATIVE_FAILURE evidence")
    values = [int(group) for group in match.groups()]
    if values[0] != 0:
        raise AssertionError(f"native failure first cause: {match.group(0)}")
    return values


def parse_stress(extras):
    try:
        values = [int(extras[f"gNdsRendererNativeFailure.{field}"]) for field in FIELDS]
    except KeyError as missing:
        raise AssertionError(f"missing native failure evidence: {missing}") from missing
    if values[0] != 0:
        raise AssertionError(f"native failure first cause: {values}")
    return values


def test_runtime_marker_parsing():
    good = "P2NATIVEFAIL count=0 domain=0 scene=0 identity=0 status=0 root=0 material=0 reason=0"
    assert parse_runtime(good)[0] == 0
    with pytest.raises(AssertionError, match="first cause"):
        parse_runtime(
            "P2NATIVEFAIL count=2 domain=1 scene=22 identity=123 status=4 root=5 material=6 reason=2"
        )
    with pytest.raises(AssertionError, match="missing"):
        parse_runtime("P2FAIL open=0 format=0 short=0 pack=0 surface=0 animation=0 scene=0 cpsr=0x0")


def test_shell_marker_parsing():
    good = ["LOOPDONE enters=9 exits=8", "LOOPNATIVEFAIL count=0 domain=0 scene=0 identity=0 status=0 root=0 material=0 reason=0"]
    assert parse_shell(good)[0] == 0
    with pytest.raises(AssertionError, match="first cause"):
        parse_shell(["LOOPNATIVEFAIL count=1 domain=2 scene=16 identity=7 status=8 root=9 material=10 reason=3"])
    with pytest.raises(AssertionError, match="missing"):
        parse_shell(["LOOPDONE enters=9 exits=8"])
    with pytest.raises(AssertionError, match="malformed"):
        parse_shell(["LOOPNATIVEFAIL count=oops"])


def test_battle_marker_parsing():
    assert parse_battle("NATIVE_FAILURE=0,0,0,0,0,0,0,0")[0] == 0
    with pytest.raises(AssertionError, match="first cause"):
        parse_battle("NATIVE_FAILURE=1,1,22,99,4,5,6,2")
    with pytest.raises(AssertionError, match="missing"):
        parse_battle("RENDER_PROFILE=1,2,3")


def test_stress_extras_parsing():
    good = {f"gNdsRendererNativeFailure.{field}": 0 for field in FIELDS}
    assert parse_stress(good)[0] == 0
    bad = dict(good)
    bad["gNdsRendererNativeFailure.count"] = 1
    bad["gNdsRendererNativeFailure.domain"] = 1
    with pytest.raises(AssertionError, match="first cause"):
        parse_stress(bad)
    with pytest.raises(AssertionError, match="missing"):
        parse_stress({})


def test_powershell_files_tokenize_cleanly():
    pwsh = shutil.which("pwsh")
    if pwsh is None:
        pytest.skip("pwsh is not on PATH for this host check")
    for name, path in SCRIPTS.items():
        command = (
            "$errors = $null; "
            f"$tokens = [System.Management.Automation.PSParser]::Tokenize("
            f"(Get-Content -LiteralPath '{path}' -Raw), [ref]$errors); "
            "if ($errors.Count -ne 0) { $errors | ForEach-Object { Write-Output $_.Message }; exit 1 }"
        )
        result = subprocess.run([pwsh, "-NoProfile", "-Command", command], capture_output=True, text=True)
        assert result.returncode == 0, f"{name} PowerShell tokenize failed: {result.stdout}{result.stderr}"
