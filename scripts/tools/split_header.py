"""Split a header into topic headers it includes in place, inside its own
include guard, so every translation unit's text is unchanged.

usage: python split_header.py [--check] PLAN.json
plan: {"file": "include/nds/x.h",
       "slices": [{"line": N, "name": "x_topic.h", "header": [...]}, ...]}

Each slice starts at its line (moving up over the comment attached to it)
and runs to the next slice; the last runs to the header's closing #endif.
A cut must sit at brace depth 0, outside a comment, and at preprocessor
depth 1 (inside the guard)."""
import json
import re
import sys
from pathlib import Path

check = "--check" in sys.argv
plan = json.load(open([a for a in sys.argv[1:] if a != "--check"][0]))
ROOT = Path(__file__).resolve().parents[2]
src = ROOT / plan["file"]
raw = src.read_bytes()
crlf = b"\r\n" in raw
L = raw.decode("utf-8").replace("\r\n", "\n").split("\n")
while L and not L[-1].strip():
    L.pop()
if L[-1].strip().split("/*")[0].strip() != "#endif":
    raise SystemExit(f"{src}: does not end with its guard's #endif")

depth = []
brace = pp = 0
in_comment = False
for line in L:
    depth.append((brace, pp, in_comment))
    m = re.match(r"^\s*#\s*(if|ifdef|ifndef|endif)\b", line)
    if m and not in_comment:
        pp += -1 if m.group(1) == "endif" else 1
        continue
    code = line
    if in_comment:
        if "*/" not in code:
            continue
        code = code.split("*/", 1)[1]
        in_comment = False
    code = re.sub(r"/\*.*?\*/", "", code)
    if "/*" in code:
        code = code.split("/*", 1)[0]
        in_comment = True
    code = re.sub(r'"(\\.|[^"\\])*"|//.*$', "", code)
    brace += code.count("{") - code.count("}")

cuts = []
for s in plan["slices"]:
    j = s["line"] - 1
    while j > 0 and L[j - 1].strip() and (L[j - 1].lstrip().startswith(("/*", "*", "//")) or
                                          L[j - 1].rstrip().endswith("*/")):
        j -= 1
    if depth[j] != (0, 1, False):
        raise SystemExit(f"cut before line {j + 1} ({s['name']}) is at {depth[j]}")
    cuts.append((j, s))
if [c[0] for c in cuts] != sorted(c[0] for c in cuts):
    raise SystemExit("cuts out of order")
end = len(L) - 1  # the #endif
bounds = [c[0] for c in cuts] + [end]
pieces = []
for k, (start, s) in enumerate(cuts):
    body = L[start:bounds[k + 1]]
    while body and not body[-1].strip():
        body.pop()
    head = ["/* " + s["header"][0]] + [" * " + h for h in s["header"][1:]]
    head[-1] += " */"
    pieces.append((s["name"], head + [""] + body))
kept = L[:cuts[0][0]]
while kept and not kept[-1].strip():
    kept.pop()
print(f"{src}: {len(L)} lines -> {len(kept) + len(pieces) + 3} kept")
for name, lines in pieces:
    print(f"  {name}: {len(lines)} lines")
if check:
    sys.exit(0)


def write(path, lines):
    t = "\n".join(lines) + "\n"
    if crlf:
        t = t.replace("\n", "\r\n")
    path.write_bytes(t.encode("utf-8"))


write(src, kept + [""] + [f'#include "{name}"' for name, _ in pieces] + ["", L[end]])
for name, lines in pieces:
    write(src.parent / name, lines)
