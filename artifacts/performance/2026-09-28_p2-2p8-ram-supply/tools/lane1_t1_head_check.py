"""Run source-slicing unittest modules twice: against the working tree, and with
reads of src/port/reloc_backend_assets.c served from HEAD (the pre-change file).
Usage: python head_check.py <HEAD copy of reloc_backend_assets.c> <module> [...]
"""
import builtins
import io
import os
import sys
import unittest
from pathlib import Path

REPO = Path(r"D:\Stuff\DevFolder\Smash64DS_Port")
for sub in ("scripts/menus", "scripts", "scripts/stages", "scripts/fighters",
            "scripts/3d_vfx"):
    sys.path.insert(0, str(REPO / sub))
os.chdir(REPO)

head = Path(sys.argv[1]).read_text(encoding="utf-8")
mods = sys.argv[2:]
TARGET = "src/port/reloc_backend_assets.c"


def run(label, use_head):
    orig_read = Path.read_text
    real_open = builtins.open

    def patched_read(self, *a, **k):
        if use_head and self.as_posix().endswith(TARGET):
            return head
        return orig_read(self, *a, **k)

    def patched_open(file, *a, **k):
        text_mode = not a or "b" not in a[0]
        if (use_head and text_mode and isinstance(file, (str, Path)) and
                str(file).replace(chr(92), "/").endswith(TARGET)):
            return io.StringIO(head)
        return real_open(file, *a, **k)

    Path.read_text = patched_read
    builtins.open = patched_open
    try:
        suite = unittest.TestSuite()
        for m in mods:
            sys.modules.pop(m, None)
            suite.addTests(unittest.defaultTestLoader.loadTestsFromName(m))
        res = unittest.TextTestRunner(verbosity=0, stream=io.StringIO()).run(suite)
    finally:
        Path.read_text = orig_read
        builtins.open = real_open
    bad = sorted(t.id() for t, _ in res.failures + res.errors)
    print(f"{label}: run={res.testsRun} failures={len(res.failures)} errors={len(res.errors)}")
    for b in bad:
        print("   ", b)
    return bad


now = run("working tree        ", False)
old = run("HEAD (pre-change)   ", True)
print("SAME FAILURE SET" if now == old else "DIFFERENT FAILURE SET")
