#!/usr/bin/env python3
"""Compile examples/probes against an installed devkitPro libnds 2.x SDK.

This is an object compilation and code-inspection aid, not an NDS link/run.
Missing SDK exits 2; no success is claimed. Never adds tests/mocks to includes.
"""
from pathlib import Path
import argparse
import json
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]

def executable(bin_dir: Path, name: str) -> str | None:
    for suffix in ("", ".exe"):
        candidate = bin_dir / (name + suffix)
        if candidate.is_file():
            return str(candidate)
    return shutil.which(name)

def check(sdk: Path, arm: Path, out: Path) -> None:
    cc = executable(arm/"bin", "arm-none-eabi-gcc")
    cxx = executable(arm/"bin", "arm-none-eabi-g++")
    nm = executable(arm/"bin", "arm-none-eabi-nm")
    objdump = executable(arm/"bin", "arm-none-eabi-objdump")
    libnds = Path(os.environ.get("LIBNDS", str(sdk/"libnds")))
    calico = sdk/"calico"
    absent = []
    for name, tool in (("gcc",cc),("g++",cxx),("nm",nm),("objdump",objdump)):
        if not tool: absent.append("arm-none-eabi-"+name)
    for header in (libnds/"include/nds.h", calico/"include/calico/system/thread.h"):
        if not header.is_file(): absent.append(str(header))
    if absent:
        print("SKIP: installed current devkitPro SDK not available:")
        for item in absent: print("  "+item)
        print("No target compilation, link, or device validation performed.")
        raise SystemExit(2)
    out.mkdir(parents=True, exist_ok=True)
    commands = []
    includes = ["-I", str(libnds/"include"), "-I", str(calico/"include")]
    def run(args: list[str]) -> None:
        commands.append(args)
        subprocess.run(args, check=True)
    arm9 = ["-march=armv5te", "-mtune=arm946e-s", "-marm", "-O2", "-DARM9"]
    arm7 = ["-march=armv4t", "-mtune=arm7tdmi", "-mthumb", "-Os", "-DARM7"]
    common = ["-std=gnu11", "-Wall", "-Wextra", "-D__NDS__", "-DNDEBUG"] + includes
    sources = [(p, arm9) for p in sorted((ROOT/"examples").glob("*.c"))]
    sources += [(ROOT/"examples/pxi/arm9.c", arm9),
                (ROOT/"examples/pxi/arm7_service.c", arm7)]
    sources += [(ROOT/"tests"/p, arm9) for p in
                ("portable_codegen.c", "helper_codegen.c", "native_math_codegen.c")]
    objects = []
    for source, flags in sources:
        stem = "__".join(source.relative_to(ROOT).with_suffix("").parts)
        obj = out/(stem+".o")
        run([cc] + common + flags + ["-c", str(source), "-o", str(obj)])
        objects.append(obj)
        with (out/(stem+".asm.txt")).open("w") as handle:
            subprocess.run([objdump, "-dr", str(obj)], stdout=handle, check=True)
    headers = out/"header_check.cpp"
    headers.write_text('#include "examples/fixed_math.h"\n'
                       '#include "examples/shared_mailbox.h"\n'
                       '#include "examples/video_copy16.h"\n'
                       '#include "examples/pxi/protocol.h"\n'
                       '#include "examples/pxi/demo.h"\n'
                       'int32_t cpp_probe(int32_t a,int32_t b) { return q20_12_mul_trunc_zero(a,b); }\n')
    run([cxx, "-std=gnu++17", "-Wall", "-Wextra", "-D__NDS__", "-DNDEBUG"] +
        includes + arm9 + ["-I", str(ROOT), "-c", str(headers), "-o", str(out/"headers.o")])
    portable = out/"tests__portable_codegen.o"
    exported = subprocess.check_output([nm,"-g","--defined-only",str(portable)], text=True)
    for symbol in ("probe_mul_trunc","probe_mul_round","probe_div","probe_copy16","probe_protocol"):
        if symbol not in exported: raise SystemExit("FAIL: optimized probe missing: "+symbol)
    reports = []
    for obj in objects:
        unresolved = subprocess.check_output([nm,"-u",str(obj)], text=True)
        reports.append(str(obj.name)+"\n"+unresolved)
    (out/"undefined-symbols.txt").write_text("\n".join(reports))
    version = subprocess.check_output([cc,"--version"], text=True).splitlines()[0]
    (out/"commands.json").write_text(json.dumps({"compiler":version,"commands":commands},indent=2)+"\n")
    print(f"PASS: {len(objects)} C objects and one C++17 header probe against real SDK headers")
    print("Generated disassembly and unresolved-symbol reports:", out)
    print("Review helpers by hot/cold call site; their mere presence is not a failure.")
    print("Compile-only: no .nds linking, paired-CPU run, cache/DMA test, or timing result.")

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--devkitpro", type=Path, default=Path(os.environ.get("DEVKITPRO","/opt/devkitpro")))
    parser.add_argument("--devkitarm", type=Path, default=None)
    parser.add_argument("--out", type=Path, help="Keep compile/inspection outputs here; otherwise use a temporary directory")
    args = parser.parse_args()
    arm = args.devkitarm or Path(os.environ.get("DEVKITARM", str(args.devkitpro/"devkitARM")))
    if args.out:
        check(args.devkitpro,arm,args.out.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix="nds-sdk-checks-") as folder:
            check(args.devkitpro,arm,Path(folder))
        print("Temporary outputs removed; use --out to retain them.")

if __name__ == "__main__":
    main()
