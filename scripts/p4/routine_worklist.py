#!/usr/bin/env python3
"""P4 S5: each content's donor routine work list.

Reads a lab build's generated manifests (NDS_P4_LAB_FALLBACK=1, whose
lab_fallbacks name every donor routine that runs a stand-in), the staged
Remix build's symbol log and its asm sources, and writes, under --out:

  worklist.json   per content: each routine with its Remix scope, file and
                  line, size in bytes (to the next symbol), the status
                  slots or tables that use it, and the other contents that
                  share it
  worklist.md     the same as a checklist per content

The routine names come from Remix's own sources; nothing here is ROM data,
but the output lives in a build directory like every other P4 product.
"""
from __future__ import annotations

import argparse
import json
import re
from collections import defaultdict
from pathlib import Path

import remix_rom as R

SCOPE_RE = re.compile(r"^\s*scope\s+([A-Za-z_][A-Za-z0-9_]*)\s*:?\s*\{")
LABEL_RE = re.compile(r"^\s*([A-Za-z_][A-Za-z0-9_]*):\s*(//.*)?$")


def index_sources(src: Path) -> dict[str, tuple[str, int]]:
    """Fully scoped label -> (file, line), from bass `scope X {` nesting."""
    found: dict[str, tuple[str, int]] = {}
    for path in sorted(src.rglob("*.asm")):
        stack: list[tuple[str, int]] = []  # (scope, brace depth at open)
        depth = 0
        rel = path.relative_to(src).as_posix()
        for n, raw in enumerate(path.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
            line = raw.split("//")[0]
            m = SCOPE_RE.match(line)
            if m:
                stack.append((m.group(1), depth))
                found.setdefault(".".join(s for s, _ in stack), (rel, n))
            else:
                m = LABEL_RE.match(raw)
                if m and stack:
                    found.setdefault(".".join(s for s, _ in stack) + "." + m.group(1), (rel, n))
            depth += line.count("{") - line.count("}")
            while stack and depth <= stack[-1][1]:
                stack.pop()
    return found


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--staging", type=Path, required=True)
    ap.add_argument("--lab", type=Path, required=True, help="lab build directory (its p4/<name>/manifest.json)")
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()

    sym, by_addr = R.load_symbols(args.staging / "logfile.log")
    starts = sorted(by_addr)
    sources = index_sources(args.staging / "src")

    def size_of(addr: int) -> int:
        later = [a for a in starts if a > addr]
        return (later[0] - addr) if later else 0

    rows: dict[str, list[dict]] = {}
    users: dict[str, set[str]] = defaultdict(set)
    for manifest in sorted((args.lab / "p4").glob("*/manifest.json")):
        name = manifest.parent.name
        data = json.loads(manifest.read_text(encoding="utf-8"))
        routines: dict[str, dict] = {}
        for fb in data.get("lab_fallbacks", []):
            routine = fb.get("routine")
            if not routine or fb["status"] == "files" or routine.startswith("0x"):
                continue
            row = routines.setdefault(routine, {"routine": routine, "uses": []})
            row["uses"].append(f"{fb['status']} {fb['slot']}")
        for routine, row in routines.items():
            addr = sym.get(routine)
            row["address"] = f"{addr:#010x}" if addr is not None else None
            row["bytes"] = size_of(addr) if addr is not None else 0
            where = sources.get(routine)
            row["source"] = f"{where[0]}:{where[1]}" if where else None
            users[routine].add(name)
        rows[name] = sorted(routines.values(), key=lambda r: (r["source"] or "", r["routine"]))

    for name, routines in rows.items():
        for row in routines:
            row["shared_with"] = sorted(users[row["routine"]] - {name})

    # Remix hooks into shared code that test a content's id: every
    # `Character.id.<REMIX>` outside the content's own folder. A compare that
    # sends it down its parent's branch needs nothing (the parent-kind
    # identity already takes it); the list is what to read.
    contents = json.loads((Path(__file__).with_name("contents.json")).read_text(encoding="utf-8"))
    remix_names = {row["name"]: row["remix"] for row in contents["contents"]}
    asm = {p: p.read_text(encoding="utf-8", errors="replace").splitlines()
           for p in sorted((args.staging / "src").rglob("*.asm"))}
    hooks: dict[str, list[str]] = {}
    for name in rows:
        remix = remix_names.get(name)
        if remix is None:
            continue
        needle = re.compile(r"Character\.id\." + re.escape(remix) + r"\b")
        found = []
        for path, text in asm.items():
            rel = path.relative_to(args.staging / "src").as_posix()
            if rel.lower().startswith(remix.lower() + "/"):
                continue
            found += [f"{rel}:{n}" for n, line in enumerate(text, 1) if needle.search(line)]
        hooks[name] = found

    args.out.mkdir(parents=True, exist_ok=True)
    (args.out / "worklist.json").write_text(json.dumps({"routines": rows, "id_hooks": hooks}, indent=1),
                                            encoding="utf-8", newline="\n")
    lines = ["# P4 donor routine work list (generated by scripts/p4/routine_worklist.py)", ""]
    for name, routines in rows.items():
        total = sum(r["bytes"] for r in routines)
        lines += [f"## {name}: {len(routines)} routines, {total} bytes of MIPS "
                  f"(to the next symbol; helpers they call not counted)", "",
                  "| Routine | Source | Bytes | Used by | Shared with |", "|---|---|---|---|---|"]
        for r in routines:
            lines.append(f"| `{r['routine']}` | {r['source'] or '?'} | {r['bytes']} | "
                         f"{', '.join(r['uses'])} | {', '.join(r['shared_with'])} |")
        lines += ["", f"Id tests in shared code ({len(hooks.get(name, []))}): " +
                  (", ".join(hooks.get(name, [])) or "none"), ""]
    (args.out / "worklist.md").write_text("\n".join(lines), encoding="utf-8", newline="\n")
    print(json.dumps({n: [len(r), sum(x["bytes"] for x in r)] for n, r in rows.items()}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
