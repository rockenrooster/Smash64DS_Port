#!/usr/bin/env python3
"""Run host tests and available freestanding ARM codegen checks; never claim an NDS build."""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--report", type=Path, help="optional JSON execution report")
    args = parser.parse_args()
    results: list[dict] = []
    versions: dict[str, str] = {}

    def run(label: str, command: list[str], expected: int = 0) -> str:
        proc = subprocess.run(command, cwd=ROOT, text=True, capture_output=True, timeout=45)
        if proc.returncode != expected:
            raise RuntimeError(f"{label}: exit {proc.returncode}, expected {expected}\n{proc.stdout}\n{proc.stderr}")
        results.append({"check": label, "result": "PASS", "command": command})
        print(f"PASS: {label}", flush=True)
        return proc.stdout + proc.stderr

    try:
        run("pack integrity", [sys.executable, "tests/check_pack.py"])
        log = run("Python semantic-tool tests", [sys.executable, "-m", "unittest", "discover", "-s", "tests", "-p", "test_tools.py", "-v"])
        results[-1]["output"] = log
        with tempfile.TemporaryDirectory(prefix="n64-nds-checks-") as work:
            tmp = Path(work)
            for tool, sample in (("compile_vertex_plan.py", "vertex_history.json"), ("live_set.py", "live_set.json")):
                output = tmp / (tool + ".json")
                command = [sys.executable, str(ROOT / "tools" / tool), str(ROOT / "examples" / sample), str(output)]
                run(f"CLI {tool}", command)
                first = output.read_bytes()
                run(f"deterministic CLI {tool}", command)
                if output.read_bytes() != first: raise RuntimeError("nondeterministic output")
                invalid = tmp / "invalid.json"
                invalid.write_text("{}", encoding="utf-8")
                run(f"invalid CLI leaves existing output: {tool}", [command[0], command[1], str(invalid), str(output)], expected=2)
                if output.read_bytes() != first: raise RuntimeError("invalid input overwrote output")
            for compiler in ("gcc", "clang"):
                cc = shutil.which(compiler)
                if not cc:
                    results.append({"check": compiler + " host tests", "result": "SKIP", "reason": "not installed"})
                    continue
                versions[compiler] = subprocess.check_output([cc, "--version"], text=True).splitlines()[0]
                variants = {"debug": ["-O0"], "optimized": ["-O2"], "release": ["-O2", "-DNDEBUG"],
                            "ubsan": ["-O1", "-g", "-fsanitize=undefined", "-fno-sanitize-recover=all"]}
                for label, flags in variants.items():
                    exe = tmp / f"{compiler}-{label}"
                    run(f"{compiler} {label} compile", [cc, "-std=c11", "-Wall", "-Wextra", "-Werror", *flags,
                        str(ROOT / "tests/test_helpers.c"), "-o", str(exe)])
                    run(f"{compiler} {label} execution", [str(exe)])
            for compiler in ("g++", "clang++"):
                cc = shutil.which(compiler)
                if cc:
                    run(f"{compiler} C++17 header integration", [cc, "-x", "c++", "-std=c++17", "-O2",
                        "-Wall", "-Wextra", "-Werror", "-c", str(ROOT / "tests/codegen.c"), "-o", str(tmp / (compiler + ".o"))])
                else:
                    results.append({"check": compiler + " C++17", "result": "SKIP", "reason": "not installed"})
            clang = shutil.which("clang")
            if clang:
                for cpu in ("arm946e-s", "arm7tdmi"):
                    for mode in ("arm", "thumb"):
                        base = [clang, "--target=arm-none-eabi", "-mcpu=" + cpu, "-m" + mode,
                                "-mfloat-abi=soft", "-ffreestanding", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror"]
                        stem = tmp / (cpu + "-" + mode)
                        run(f"freestanding {cpu} {mode} object", base + ["-c", str(ROOT / "tests/codegen.c"), "-o", str(stem) + ".o"])
                        run(f"freestanding {cpu} {mode} assembly", base + ["-S", str(ROOT / "tests/codegen.c"), "-o", str(stem) + ".s"])
                        assembly = Path(str(stem) + ".s").read_text()
                        # Selected leaf wrappers should be entirely inline; this is
                        # not a whole-program ban on runtime helper calls.
                        if re.search(r"\b(?:bl|blx)\s+|__aeabi_|__divdi3|__udivdi3|__muldi3", assembly):
                            raise RuntimeError(f"unexpected call/helper in {cpu}/{mode} selected leaf code")
                        results.append({"check": f"{cpu} {mode}: no calls/helpers in selected leaf wrappers", "result": "PASS"})
                        print(f"PASS: {cpu} {mode} selected leaf code has no calls/helpers")
            else:
                results.append({"check": "ARM cross-codegen", "result": "SKIP", "reason": "Clang unavailable"})
        report = {"versions": versions, "checks": results,
                  "not_executed": ["devkitARM/libnds/Calico integration build", "NDS ROM link",
                                   "N64 execution oracle", "DS emulator/device execution", "performance measurement"]}
        if args.report:
            args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(f"\n{sum(r['result'] == 'PASS' for r in results)} checks passed; "
              f"{sum(r['result'] == 'SKIP' for r in results)} skipped.")
        print("No .nds build, device/emulator execution, or performance claim is made.")
        return 0
    except (OSError, RuntimeError, subprocess.SubprocessError) as exc:
        print(str(exc), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
