#!/usr/bin/env python3
"""Fill a fighter's native-owner tables from the generator's own falsifiers.

The owner generator (generate_nds_native_owners.py) checks every owner's
cross-binding palette slots, plan counts and model census against what it
derives from the O2R model, and stops at the first mismatch. For a NEW owner
those tables start as seeds (no cross slots, zero plan counts, zero census),
so this driver runs the inventory, reads the one falsifier it reports, edits
exactly that row, and repeats until the inventory is green. It is the loop
every P2-3 owner admission ran by hand (Yoshi: 2026-09-02).

usage: python scripts/fighters/derive_native_owner_tables.py --repo-root . --fighter ness
"""
from __future__ import annotations

import argparse
import importlib.util
import json
import re
import subprocess
import sys
from pathlib import Path


def run_inventory(root: Path, fighter: str) -> str:
    code = f"""
import importlib.util, sys, json
from pathlib import Path
root = Path({str(root)!r})
spec = importlib.util.spec_from_file_location('g', root / 'scripts/fighters/generate_nds_native_owners.py')
g = importlib.util.module_from_spec(spec); sys.modules['g'] = g; spec.loader.exec_module(g)
try:
    inv = g.build_p2_owner_model_inventory(root, {fighter!r})
    print('OK ' + json.dumps({{d: inv['details'][d].get('hierarchy') for d in inv['details']}}))
except Exception as e:
    print('ERR ' + type(e).__name__ + ' ' + str(e))
"""
    out = subprocess.run([sys.executable, "-c", code], capture_output=True, text=True)
    return (out.stdout.strip() or out.stderr.strip())


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo-root", type=Path, default=Path("."))
    ap.add_argument("--fighter", required=True)
    ap.add_argument("--max-attempts", type=int, default=80)
    a = ap.parse_args()
    root = a.repo_root.resolve()
    gen = root / "scripts/fighters/generate_nds_native_owners.py"
    name = a.fighter
    plan_row = rf'\s+"{name}": \(\d+, \d+, \d+, \d+, \d+\),$'

    def read() -> str:
        return gen.read_text(encoding="utf-8", newline="")

    def write(text: str) -> None:
        gen.write_text(text, encoding="utf-8", newline="")

    def split(text: str):
        nl = "\r\n" if "\r\n" in text[:4000] else "\n"
        return nl, text.split(nl)

    def plan_rows(text: str):
        nl, lines = split(text)
        rows = [i for i, line in enumerate(lines) if re.match(plan_row, line.rstrip())]
        if len(rows) != 2:
            raise SystemExit(f"expected two {name} plan rows, found {len(rows)}")
        return nl, lines, rows

    def plan_field(line: str, field: int) -> int:
        return int(re.search(r"\((\d+), (\d+), (\d+), (\d+), (\d+)\)", line).group(field + 1))

    def set_plan_field(line: str, field: int, value: int) -> str:
        vals = list(re.search(r"\((\d+), (\d+), (\d+), (\d+), (\d+)\)", line).groups())
        vals[field] = str(value)
        return re.sub(r"\(\d+, \d+, \d+, \d+, \d+\)", "(" + ", ".join(vals) + ")", line)

    for attempt in range(a.max_attempts):
        out = run_inventory(root, name)
        if out.startswith("OK"):
            print(f"{name}: inventory green after {attempt} edits")
            print(out[:600])
            return 0
        print(f"attempt {attempt}: {out[:240]}")
        text = read()
        nl, lines = split(text)
        m = re.search(rf"{name} (high|low) (?:current )?binding (\d+) has no (?:restorable )?GX palette slot", out)
        if m:
            detail, binding = m.group(1), int(m.group(2))
            # The High table's row is the first `"name": (` line inside the
            # OWNER_CROSS_BINDING_SLOTS table, the Low table's the second.
            rows = [i for i, l in enumerate(lines) if re.match(rf'\s+"{name}": \(', l) and not re.match(plan_row, l.rstrip())
                    and not re.match(rf'\s+"{name}": \(0x', l)]
            rows = [i for i in rows if "(" in lines[i] and re.search(r"\(\(|\(\)", lines[i])]
            if len(rows) != 2:
                raise SystemExit(f"expected two {name} cross-slot rows, found {len(rows)}")
            target = rows[0] if detail == "high" else rows[1]
            pairs = [(int(x), int(y)) for x, y in re.findall(r"\((\d+), (\d+)\)", lines[target])]
            if any(b == binding for b, _ in pairs):
                raise SystemExit(f"binding {binding} already has a slot in the {detail} row")
            pairs = [(int(x), int(y)) for x, y in re.findall(r"\((\d+), (\d+)\)", lines[target])]
            pairs.append((binding, (max(s for _, s in pairs) + 1) if pairs else 16))
            pairs.sort()
            lines[target] = f'    "{name}": (' + ", ".join(f"({b}, {s})" for b, s in pairs) + ",)," if len(pairs) == 1 else \
                f'    "{name}": (' + ", ".join(f"({b}, {s})" for b, s in pairs) + "),"
            write(nl.join(lines))
            continue
        m = re.search(rf"{name} (high|low) native-model census \(([^)]*)\) != \(([^)]*)\)", out)
        if m:
            detail, got = m.group(1), m.group(2)
            target = f'        "{detail}": (0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),'
            if text.count(target + nl) < 1:
                raise SystemExit("no zero census row for " + detail)
            write(text.replace(target + nl, f'        "{detail}": ({got}),' + nl, 1))
            continue
        m = re.search(r"selected part count (\d+) != (\d+)|(\d+) parts, expected (\d+)|drawable root count (\d+) != (\d+)", out)
        handled = False
        for pattern, field in ((rf"{name} (high|low) GX store count (\d+) != (\d+)", 3),
                               (rf"{name} (high|low) GX restore count (\d+) != (\d+)", 4)):
            m = re.search(pattern, out)
            if m:
                detail, got = m.group(1), int(m.group(2))
                nl, lines, rows = plan_rows(text)
                # DETAIL_GX_PLAN_COUNTS lists the High rows first, then Low.
                target = rows[0] if detail == "high" else rows[1]
                lines[target] = set_plan_field(lines[target], field, got)
                write(nl.join(lines))
                handled = True
                break
        if handled:
            continue
        m = re.search(r"hierarchy accounting changed: seed/push/pop=(\d+)/(\d+)/(\d+)", out)
        if m:
            seed, push, pop = (int(v) for v in m.groups())
            nl, lines, rows = plan_rows(text)
            targets = [i for i in rows if plan_field(lines[i], 1) != push or plan_field(lines[i], 2) != pop]
            if not targets:
                raise SystemExit("plan rows already carry the reported push/pop")
            i = targets[0]
            lines[i] = set_plan_field(set_plan_field(set_plan_field(lines[i], 0, seed), 1, push), 2, pop)
            write(nl.join(lines))
            continue
        # OWNER_PLAN_COUNTS (parts, roots): each falsifier names the derived value.
        pc = re.search(rf'(    "{name}": )\((\d+), (\d+)\),', text)
        if pc:
            parts, roots = int(pc.group(2)), int(pc.group(3))
            m = re.search(r"drawable root cardinality changed: (\d+) != (\d+)", out) or \
                re.search(r"logical binding count (\d+) != (\d+)", out)
            if m:
                write(text.replace(pc.group(0), f'{pc.group(1)}({parts}, {int(m.group(1))}),', 1))
                continue
            m = re.search(r"hierarchy expects \d+ joints, got (\d+)", out) or \
                re.search(r"live joint count (\d+) != \d+", out)
            if m:
                write(text.replace(pc.group(0), f'{pc.group(1)}({int(m.group(1))}, {roots}),', 1))
                continue
        print("unhandled:", out[:600])
        return 1
    print("gave up")
    return 1


if __name__ == "__main__":
    sys.exit(main())
