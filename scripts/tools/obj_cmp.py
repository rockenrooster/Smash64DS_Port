"""Compile the changed preprocessed units of two pp_run.py trees with each
unit's real flags (from the make -n listing) and compare their non-debug
sections byte for byte.

usage: python obj_cmp.py MAKE_N_LISTING BEFORE_DIR AFTER_DIR WORK_DIR

A unit's sections are read straight from the object (one readelf call for the
section table), so an object with thousands of -ffunction-sections sections
compares in seconds. Debug, comment, note and attribute sections are skipped;
relocation sections are compared too (a changed call target shows there).
"""
import concurrent.futures as cf
import hashlib
import re
import shlex
import subprocess
import sys
from pathlib import Path

GCC = r"C:\devkitPro\devkitARM\bin\arm-none-eabi-gcc.exe"
READELF = r"C:\devkitPro\devkitARM\bin\arm-none-eabi-readelf.exe"
SKIP = (".debug", ".comment", ".ARM.attributes", ".note", ".rel.debug", ".group")


def unit_flags(listing):
    flags = {}
    for line in Path(listing).read_text(encoding="utf-8", errors="replace").splitlines():
        if "arm-none-eabi-gcc" not in line or " -c " not in line:
            continue
        for part in re.split(r"\s*(?:&&|;)\s*", line):
            if "arm-none-eabi-gcc" not in part or " -c " not in part:
                continue
            try:
                argv = shlex.split(part, posix=True)
            except ValueError:
                continue
            obj = argv[argv.index("-o") + 1] if "-o" in argv else None
            if obj is None:
                continue
            keep, skip = [], 0
            for a in argv[1:]:
                if skip:
                    skip -= 1
                    continue
                if a in ("-o", "-MF", "-MT", "-MQ", "-include", "-I", "-isystem"):
                    skip = 1
                    continue
                if a.startswith(("-I", "-D", "-U", "-M", "-include")) or a == "-c" or a.endswith((".c", ".s", ".S")):
                    continue
                keep.append(a)
            flags[re.sub(r"[^A-Za-z0-9_.-]", "_", obj) + ".i"] = keep
            break
    return flags


def sections(obj):
    """{section name: sha256 of its bytes} for the object's compared sections."""
    out = subprocess.run([READELF, "-W", "-S", str(obj)], capture_output=True, text=True).stdout
    data = Path(obj).read_bytes()
    res = {}
    for line in out.splitlines():
        m = re.match(r"\s*\[\s*\d+\]\s+(\S+)\s+(\S+)\s+[0-9a-f]+\s+([0-9a-f]+)\s+([0-9a-f]+)", line)
        if not m:
            continue
        name, kind, off, size = m.group(1), m.group(2), int(m.group(3), 16), int(m.group(4), 16)
        if name.startswith(SKIP) or kind in ("NULL", "SYMTAB", "STRTAB"):
            continue
        blob = b"" if kind == "NOBITS" else data[off:off + size]
        res[name] = hashlib.sha256(blob + size.to_bytes(8, "little")).hexdigest()
    return res


def compare(name, f, before, after, work):
    objs = []
    for tag, d in (("b", before), ("a", after)):
        o = work / f"{tag}_{name}.o"
        r = subprocess.run([GCC] + f + ["-x", "cpp-output", "-c", str(d / name), "-o", str(o)],
                           capture_output=True, text=True)
        if r.returncode != 0:
            return f"{name}: COMPILE FAIL ({tag}) {r.stderr[-300:]}"
        objs.append(o)
    sb, sa = sections(objs[0]), sections(objs[1])
    diff = sorted(k for k in set(sb) | set(sa) if sb.get(k) != sa.get(k))
    return f"{name}: {len(sb)} sections before, {len(sa)} after, differing: {diff[:8]}"


def main():
    listing, before, after, work = (Path(a) for a in sys.argv[1:5])
    work.mkdir(parents=True, exist_ok=True)
    changed = [a.name for a in sorted(after.glob("*.i"))
               if (before / a.name).is_file() and (before / a.name).read_bytes() != a.read_bytes()]
    flags = unit_flags(listing)
    with cf.ThreadPoolExecutor(max_workers=8) as ex:
        jobs = []
        for name in changed:
            f = flags.get(name)
            if f is None:
                print("no flags for", name)
                continue
            jobs.append(ex.submit(compare, name, f, before, after, work))
        for j in jobs:
            print(j.result())
    if not changed:
        print("no unit differs")


if __name__ == "__main__":
    main()
