"""Compare two preprocessed trees (pp_run.py outputs): every .i must match,
exactly or after dropping blank lines (directive removal shifts gcc -P's
blank-line runs). Prints the mismatching translation units.

usage: python pp_cmp.py BEFORE_DIR AFTER_DIR
"""
import sys
from pathlib import Path


def squeeze(data: bytes) -> bytes:
    return b"\n".join(l.rstrip() for l in data.split(b"\n") if l.strip())


before, after = Path(sys.argv[1]), Path(sys.argv[2])
exact = blank = 0
bad = []
for b in sorted(before.glob("*.i")):
    a = after / b.name
    if not a.is_file():
        bad.append(f"{b.name}: missing after")
        continue
    x, y = b.read_bytes(), a.read_bytes()
    if x == y:
        exact += 1
    elif squeeze(x) == squeeze(y):
        blank += 1
    else:
        bad.append(b.name)
extra = sorted(set(p.name for p in after.glob("*.i")) - set(p.name for p in before.glob("*.i")))
print(f"{before.name} vs {after.name}: exact {exact}, blank-lines-only {blank}, DIFFERENT {len(bad)}, new {len(extra)}")
for name in bad[:20]:
    print("  DIFF", name)
