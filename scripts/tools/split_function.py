"""Move phases of one oversized function body into fragments the function
includes in place, so the translation unit's text is unchanged.

usage: python split_function.py [--check] PLAN.json
plan: {"file": "src/port/x.c",
       "fragments": [{"from": REGEX, "to": REGEX, "name": "x_phase.inc",
                      "header": [...]}, ...]}

A fragment starts at the line matching `from` and ends before the line
matching `to` (each regex must match one line; add "from_near"/"to_near" with
a line number to pick the match within 50 lines of it). Each end moves up
over the comment attached to its line. Both ends must sit inside a function
body at the same brace and preprocessor depth, outside a comment, and the
fragment never leaves that depth: every brace and #if it opens it closes.
The fragment keeps its lines' indentation; the function gets
`#include "name"` in their place."""
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


def depths(lines):
    """(brace, preprocessor, in-comment) state before each line."""
    out = []
    brace = pp = 0
    in_comment = False
    for line in lines:
        out.append((brace, pp, in_comment))
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
        code = re.sub(r'"(\\.|[^"\\])*"|\'(\\.|[^\'\\])*\'|//.*$', "", code)
        brace += code.count("{") - code.count("}")
    out.append((brace, pp, in_comment))
    return out


def find(rx, near):
    hits = [i for i, line in enumerate(L) if re.search(rx, line)]
    if near is not None:
        hits = sorted((i for i in hits if abs(i + 1 - near) <= 50),
                      key=lambda i: abs(i + 1 - near))[:1]
    if len(hits) != 1:
        raise SystemExit(f"anchor {rx!r}: {len(hits)} matches")
    j = hits[0]
    while j > 0 and L[j - 1].strip() and (L[j - 1].lstrip().startswith(("/*", "*", "//")) or
                                          L[j - 1].rstrip().endswith("*/")):
        j -= 1
    return j


D = depths(L)
cuts = []
for f in plan["fragments"]:
    a = find(f["from"], f.get("from_near"))
    b = find(f["to"], f.get("to_near"))
    if not a < b:
        raise SystemExit(f"{f['name']}: ends before it starts")
    brace, pp, in_comment = D[a]
    if brace < 1 or in_comment or D[b] != D[a]:
        raise SystemExit(f"{f['name']}: lines {a + 1}..{b} start at {D[a]}, end at {D[b]}")
    inside = [D[k] for k in range(a, b + 1)]
    if min(d[0] for d in inside) < brace or min(d[1] for d in inside) < pp:
        raise SystemExit(f"{f['name']}: leaves depth {D[a][:2]} inside")
    while b > a and not L[b - 1].strip():
        b -= 1  # trailing blank lines stay in the function
    cuts.append((a, b, f))
cuts.sort(key=lambda c: c[0])
for (a1, b1, f1), (a2, b2, f2) in zip(cuts, cuts[1:]):
    if a2 < b1:
        raise SystemExit(f"{f1['name']} and {f2['name']} overlap")
print(f"{src}: {len(L)} lines")
for a, b, f in cuts:
    print(f"  {f['name']}: lines {a + 1}..{b} ({b - a} lines)")
if check:
    sys.exit(0)


def write(path, lines):
    t = "\n".join(lines)
    if crlf:
        t = t.replace("\n", "\r\n")
    path.write_bytes(t.encode("utf-8"))


out = []
at = 0
for a, b, f in cuts:
    out += L[at:a] + [f'#include "{f["name"]}"']
    head = ["/* " + f["header"][0]] + [(" * " + h).rstrip() for h in f["header"][1:]]
    head[-1] += " */"
    write(src.parent / f["name"], head + [""] + L[a:b] + [""])
    at = b
out += L[at:]
write(src, out)
print(f"{src}: {len(out)} lines, {len(cuts)} fragments")
