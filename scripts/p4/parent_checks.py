#!/usr/bin/env python3
"""P4: the vanilla parent-kind checks, against Remix's assembled build.

A Remix fighter carries its own character id, so a vanilla compare with its
parent's kind (`fkind == nFTKindYoshi`) sends it down the else branch on the
N64 unless Remix patched that code to include it. On the DS a P4 content
carries its parent's fkind and takes the parent's branch, so every check
Remix left alone is a divergence the fighter's card must resolve (Bowser
must not raise Yoshi's egg shield), while a patched one is read for which
contents it names.

For each decomp function with such a check this lists the check's source
line, whether Remix's build differs from the vanilla ROM inside the
function (its hooks overwrite the original instructions with a jump), the
Remix routines the function now jumps to, and the contents their source
names.

    parent_checks.py --staging <remix staging> [--parent Yoshi ...] [--out file.md]
"""
from __future__ import annotations

import argparse
import bisect
import re
import struct
from collections import defaultdict
from pathlib import Path

import remix_rom as R

ROOT = Path(__file__).resolve().parents[2]
DECOMP_SRC = ROOT / "decomp" / "BattleShip-main" / "decomp" / "src"
DECOMP_SYMBOLS = ROOT / "decomp" / "BattleShip-main" / "decomp" / "symbols" / "symbols_us.txt"
DECOMP_YAML = ROOT / "decomp" / "BattleShip-main" / "decomp" / "smashbrothers.us.yaml"
FUNC_RE = re.compile(r"^[A-Za-z_][\w\s\*]*?\b([A-Za-z_]\w*)\s*\([^;]*\)\s*$")


def battle_functions() -> set[str]:
    """Functions defined in overlays 2 and 3's source files.

    Menu, movie and scene overlays share overlay 3's VRAM (0x80131B00), so a
    battle function ends at the next battle symbol, not the next symbol."""
    files, segment = set(), None
    for line in DECOMP_YAML.read_text(encoding="utf-8").splitlines():
        m = re.match(r"\s*- name: (\w+)$", line)
        if m:
            segment = m.group(1)
            continue
        m = re.match(r"\s*- \[0x[0-9A-Fa-f]+, c, ([\w/]+)\]", line)
        if m and segment in ("ovl2", "ovl3"):
            files.add(m.group(1) + ".c")
    names = set()
    for rel in files:
        path = DECOMP_SRC / rel
        if not path.exists():
            continue
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
        for n, line in enumerate(lines, 1):
            m = FUNC_RE.match(line)
            if m and n < len(lines) and lines[n].strip() == "{":
                names.add(m.group(1))
    return names


# FTKind (ft/ftdef.h) of the parents P4 contents alias.
KIND_VALUES = {"Mario": 0, "Fox": 1, "Donkey": 2, "Samus": 3, "Luigi": 4, "Link": 5,
               "Yoshi": 6, "Captain": 7, "Kirby": 8, "Pikachu": 9, "Purin": 10, "Ness": 11}
FKIND_OFFSET = 0x8  # FTStruct.fkind


def compare_sites(vanilla: bytes, remix: bytes, base: int, kind: int) -> list[tuple[int, list[int]]]:
    """The function's compares of a loaded fkind with `kind`: (branch address,
    changed word addresses from the load through the delay slot).

    IDO loads fkind (`lw rX, 0x8(rY)`), puts the kind in `at` (Mario's 0 is
    `zero`) and branches on the pair; a Remix hook overwrites the branch with
    a jump."""
    words = struct.unpack(">%dI" % (len(vanilla) // 4), vanilla)
    rwords = struct.unpack(">%dI" % (len(remix) // 4), remix)
    sites = []
    for i, w in enumerate(words):
        if (w >> 26) not in (4, 5, 0x14, 0x15):  # beq bne beql bnel
            continue
        rs, rt = (w >> 21) & 31, (w >> 16) & 31
        if kind == 0:
            if rt != 0:
                continue
            reg = rs
        else:
            if 1 not in (rs, rt) or not any(words[j] == (0x24010000 | kind) for j in range(max(0, i - 4), i)):
                continue
            reg = rt if rs == 1 else rs
        load = next((j for j in range(i - 1, max(-1, i - 12), -1)
                     if (words[j] >> 26) == 0x23 and ((words[j] >> 16) & 31) == reg
                     and (words[j] & 0xFFFF) == FKIND_OFFSET), None)
        if load is None:
            continue
        changed = [base + 4 * j for j in range(load, min(i + 2, len(words))) if rwords[j] != words[j]]
        sites.append((base + 4 * i, changed))
    return sites


def checks_for(parent: str) -> dict[str, list[tuple[str, int, str]]]:
    """Function name -> [(file, line, text)] for its parent-kind checks."""
    pattern = re.compile(r"fkind\s*(==|!=)\s*nFTKind%s\b|case\s+nFTKind%s\s*:" % (parent, parent))
    found: dict[str, list[tuple[str, int, str]]] = defaultdict(list)
    for path in sorted(DECOMP_SRC.rglob("*.c")):
        rel = path.relative_to(DECOMP_SRC).as_posix()
        if rel.startswith("relocData/"):
            continue
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
        current = None
        for n, line in enumerate(lines, 1):
            m = FUNC_RE.match(line)
            if m and n < len(lines) and lines[n].strip() == "{":
                current = m.group(1)
            if current and pattern.search(line):
                found[current].append((rel, n, line.strip()))
    return found


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--staging", type=Path, required=True)
    ap.add_argument("--parent", action="append", default=[],
                    help="decomp kind name (Yoshi, Fox, Captain, Mario); default all four")
    ap.add_argument("--out", type=Path)
    args = ap.parse_args()
    parents = args.parent or ["Yoshi", "Fox", "Captain", "Mario"]

    remix = R.Rom(args.staging / "ssb64asm.z64")
    vanilla = R.Rom(args.staging / "roms" / "ssb.rom")
    sym, by_addr = R.load_symbols(args.staging / "logfile.log")
    # Overlays share VRAM, so one address can carry several names: read the
    # file by name rather than through load_decomp_symbols' address map.
    by_name = {}
    for line in DECOMP_SYMBOLS.read_text(encoding="utf-8").splitlines():
        m = re.match(r"\s*(\w+)\s*=\s*(0x[0-9A-Fa-f]+)\s*;", line)
        if m:
            by_name.setdefault(m.group(1), int(m.group(2), 16))
    battle = battle_functions()
    starts = sorted({addr for name, addr in by_name.items() if name in battle})
    remix_starts = sorted(by_addr)
    sources = {p: p.read_text(encoding="utf-8", errors="replace")
               for p in sorted((args.staging / "src").rglob("*.asm"))}

    def remix_scope(addr: int) -> str:
        i = bisect.bisect_right(remix_starts, addr) - 1
        return by_addr[remix_starts[i]][0] if i >= 0 else f"{addr:#x}"

    def ids_named(scope: str) -> list[str]:
        """Character ids the scope's source block names."""
        leaf = scope.split(".")[-1]
        named = set()
        for text in sources.values():
            for m in re.finditer(r"scope\s+%s\s*:?\s*\{" % re.escape(leaf), text):
                block = text[m.end():m.end() + 6000]
                depth = 1
                for i, ch in enumerate(block):
                    depth += (ch == "{") - (ch == "}")
                    if depth == 0:
                        block = block[:i]
                        break
                named |= set(re.findall(r"Character\.id\.(\w+)", block))
        return sorted(named)

    out = ["# Vanilla parent-kind checks against Remix (scripts/p4/parent_checks.py)", ""]
    for parent in parents:
        out += [f"## {parent}", "",
                "| Function | Checks | Remix | Compare sites | Hooks | Ids named |",
                "|---|---|---|---|---|---|"]
        for func, sites in sorted(checks_for(parent).items()):
            addr = by_name.get(func)
            where = "; ".join(f"{f}:{n}" for f, n, _ in sites)
            if addr is None or func not in battle:
                out.append(f"| `{func}` | {where} | outside battle code | | | |")
                continue
            i = bisect.bisect_right(starts, addr)
            end = starts[i] if i < len(starts) else addr + 0x400
            size = end - addr
            changed = remix.read_ram(addr, size) != vanilla.read_ram(addr, size)
            hooks = set()
            for off in range(0, size, 4):
                word = struct.unpack(">I", remix.read_ram(addr + off, 4))[0]
                if (word >> 26) in (2, 3):
                    target = ((addr + off + 4) & 0xF0000000) | ((word & 0x3FFFFFF) << 2)
                    if target >= R.REMIX_CODE_RAM:
                        hooks.add(remix_scope(target))
            ids = sorted({i for h in hooks for i in ids_named(h)})
            cells = []
            for branch, diff in compare_sites(vanilla.read_ram(addr, size), remix.read_ram(addr, size),
                                              addr, KIND_VALUES[parent]):
                if not diff:
                    cells.append(f"{branch:#x} raw")
                    continue
                jumps = set()
                for at in diff:
                    word = struct.unpack(">I", remix.read_ram(at, 4))[0]
                    if (word >> 26) in (2, 3):
                        jumps.add(remix_scope(((at + 4) & 0xF0000000) | ((word & 0x3FFFFFF) << 2)))
                cells.append(f"{branch:#x} hooked {'/'.join(sorted(jumps)) or '(rewritten)'}")
            out.append(f"| `{func}` | {where} | {'patched' if changed else 'vanilla'} | "
                       f"{'; '.join(cells) or 'none found'} | {', '.join(sorted(hooks))} | {', '.join(ids)} |")
        out.append("")
    text = "\n".join(out) + "\n"
    if args.out:
        args.out.write_text(text, encoding="utf-8", newline="\n")
    print(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
