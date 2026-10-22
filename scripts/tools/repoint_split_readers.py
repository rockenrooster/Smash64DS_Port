"""Point the scripts that read a split source file at
source_test_helpers.source() (scripts/menus), which reads it whole.

usage: python repoint_split_readers.py FILE_NAME READER.py...

FILE_NAME is the split file's base name (renderer_adapter_matrix.c). In each
reader, `(<path expression naming FILE_NAME>).read_text(...)` and
`NAME.read_text(...)` for a module constant NAME assigned such a path become
read_split(...); the import is added when missing. Prints what changed;
fails on a reader where nothing matched."""
import re
import sys
from pathlib import Path

name = sys.argv[1]
ROOT = Path(__file__).resolve().parents[2]
esc = re.escape(name)
direct = re.compile(r"\((?P<expr>[^()\n]*" + esc + r"[\"'])\s*\)\s*\.read_text\([^)]*\)")
const_def = re.compile(r"^(?P<const>[A-Za-z_][A-Za-z0-9_]*)\s*=\s*[^\n]*" + esc + r"[\"']\s*\)?\s*$", re.M)
local_def = re.compile(r"^\s+(?P<const>[A-Za-z_][A-Za-z0-9_]*)\s*=\s*[^\n]*" + esc + r"[\"']\s*\)?\s*$", re.M)
for rel in sys.argv[2:]:
    path = ROOT / rel
    raw = path.read_bytes()
    crlf = b"\r\n" in raw
    text = raw.decode("utf-8").replace("\r\n", "\n")
    count = 0
    text, n = direct.subn(lambda m: f"read_split({m.group('expr')})", text)
    count += n
    for m in list(const_def.finditer(text)) + list(local_def.finditer(text)):
        const = m.group("const")
        pattern = re.compile(r"\b" + re.escape(const) + r"\.read_text\([^)]*\)")
        text, n = pattern.subn(f"read_split({const})", text)
        count += n
    if count == 0:
        raise SystemExit(f"{rel}: no read of {name} found")
    if not re.search(r"source_test_helpers import [^\n]*\bread_split\b", text):
        depth = len(Path(rel).parts) - 1  # scripts/x.py -> 1, scripts/a/x.py -> 2
        line = ("from source_test_helpers import source as read_split  # noqa: E402\n")
        prefix = ""
        if Path(rel).parts[:2] != ("scripts", "menus"):
            prefix = (f"sys.path.insert(0, str(Path(__file__).resolve().parents[{depth}]"
                      f" / \"scripts/menus\"))\n")
        block = prefix + line
        lines = text.split("\n")
        last = max(i for i, l in enumerate(lines)
                   if re.match(r"(import |from \S+ import )", l))
        need = []
        if "import sys" not in text and prefix:
            need.append("import sys")
        if not re.search(r"^from pathlib import [^\n]*\bPath\b", text, re.M) and prefix:
            need.append("from pathlib import Path")
        lines[last + 1:last + 1] = need + [block.rstrip("\n")]
        text = "\n".join(lines)
    path.write_bytes((text.replace("\n", "\r\n") if crlf else text).encode("utf-8"))
    print(f"{rel}: {count} read(s) repointed")
