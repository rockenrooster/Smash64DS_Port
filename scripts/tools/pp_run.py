"""Preprocess every C compile a `make -n -B` listing holds (-E -P instead of
-c, dependency flags dropped), into OUT/<object path>.i, in parallel.

usage: python pp_run.py MAKE_N_LISTING OUT_DIR
Then compare two OUT dirs with pp_cmp.py."""
import concurrent.futures as cf
import os
import re
import shlex
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
listing, out = Path(sys.argv[1]), Path(sys.argv[2])
out.mkdir(parents=True, exist_ok=True)
cmds = []
for line in listing.read_text(encoding="utf-8", errors="replace").splitlines():
    if "arm-none-eabi-gcc" not in line or " -c " not in line:
        continue
    # A recipe line may chain commands; take the gcc one.
    for part in re.split(r"\s*(?:&&|;)\s*", line):
        if "arm-none-eabi-gcc" in part and " -c " in part:
            cmds.append(part.strip())
            break
jobs = []
for c in cmds:
    try:
        argv = shlex.split(c, posix=True)
    except ValueError:
        continue
    new = []
    obj = None
    i = 0
    while i < len(argv):
        a = argv[i]
        if a in ("-MF", "-MT", "-MQ"):
            i += 2
            continue
        if a in ("-MMD", "-MD", "-MP", "-M", "-MM"):
            i += 1
            continue
        if a == "-o":
            obj = argv[i + 1]
            i += 2
            continue
        if a == "-c":
            new += ["-E", "-P"]
            i += 1
            continue
        new.append(a)
        i += 1
    if obj is None:
        continue
    rel = re.sub(r"[^A-Za-z0-9_.-]", "_", obj)
    jobs.append((new, out / (rel + ".i")))


MSYS = re.compile(r"(^|(?<=^-I)|(?<=^-isystem)|(?<=^-include)|(?<==))/([a-zA-Z])/")


def winpath(a):
    """MSYS drive paths (/d/...) as the Windows compiler reads them."""
    for prefix in ("-I", "-isystem", "-include", ""):
        if a.startswith(prefix + "/") and len(a) > len(prefix) + 2 and a[len(prefix) + 2] == "/":
            d = a[len(prefix) + 1]
            return f"{prefix}{d.upper()}:/{a[len(prefix) + 3:]}"
    if "=/" in a:
        k, v = a.split("=", 1)
        if len(v) > 2 and v[0] == "/" and v[2] == "/":
            return f"{k}={v[1].upper()}:/{v[3:]}"
    return a


def run(job):
    argv, dst = job
    argv = [winpath(a) for a in argv]
    exe = argv[0]
    if exe == "arm-none-eabi-gcc":
        argv = [r"C:\devkitPro\devkitARM\bin\arm-none-eabi-gcc.exe"] + argv[1:]
    r = subprocess.run(argv + ["-o", str(dst)], cwd=REPO, capture_output=True, text=True)
    return dst.name, r.returncode, r.stderr[-400:]


fails = 0
with cf.ThreadPoolExecutor(max_workers=20) as ex:
    for name, rc, err in ex.map(run, jobs):
        if rc != 0:
            fails += 1
            print(f"FAIL {name}: {err.strip()[:300]}")
print(f"preprocessed {len(jobs) - fails} of {len(jobs)} ({len(cmds)} commands) -> {out}")
