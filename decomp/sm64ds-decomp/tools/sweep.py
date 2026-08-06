"""Run the free template tier (leaf + reloc rules) across EVERY module.

swarm.py only knew about arm9-main. This sweeps main + all overlays, reading each
module's own binary at its own base address and resolving relocs per module.
Matches are keyed by (module, addr) because overlay addresses overlap.

Usage:
    python tools/sweep.py                 # dry-run, per-module + total
    python tools/sweep.py --apply         # record wins + write src/*.c
    python tools/sweep.py --module ov006  # one module
"""
import argparse
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import swarm as S
import relocs as R
import modules as MOD
import ledger as L

LINE_RE = re.compile(
    r"^(\S+)\s+kind:function\((arm|thumb),size=0x([0-9a-fA-F]+)\)\s+addr:0x([0-9a-fA-F]+)")


def funcs(mod):
    """arm-mode functions in a module that live inside its binary image."""
    limit = mod["base"] + mod["bin"].stat().st_size
    out = []
    for line in mod["syms"].read_text(errors="ignore").splitlines():
        m = LINE_RE.match(line)
        if not m or m.group(2) != "arm":
            continue
        size, addr = int(m.group(3), 16), int(m.group(4), 16)
        if mod["base"] <= addr < limit:
            out.append((m.group(1), addr, size))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--min", type=lambda x: int(x, 0), default=0x4)
    ap.add_argument("--max", type=lambda x: int(x, 0), default=0x200)
    ap.add_argument("--module", default=None, help="only this module (e.g. ov006)")
    ap.add_argument("--apply", action="store_true")
    args = ap.parse_args()

    done = L.load_done()
    gsyms = R.load_all_syms()                       # cross-module name resolution
    grand = 0
    rows = []
    for mod in MOD.modules():
        label = "arm9" if mod["name"] == "main" else mod["name"]
        if args.module and mod["name"] != args.module:
            continue
        relocs = R.load_relocs_file(mod["relocs"])
        data = mod["bin"].read_bytes()
        wins = []
        for name, addr, size in funcs(mod):
            if (label, addr) in done or not (args.min <= size <= args.max):
                continue
            off = addr - mod["base"]
            tgt = data[off:off + size]
            ins = list(S.md.disasm(tgt, 0))
            if S.is_thunk(ins):
                continue
            cand = None
            for rule in S.RULES:
                cand = rule(name, ins, tgt)
                if cand:
                    break
            if not cand:
                for rule in S.RELOC_RULES:
                    cand = rule(name, ins, tgt, addr, relocs, gsyms)
                    if cand:
                        break
            if not cand:
                continue
            csrc, lbl = cand
            if S.oracle_ok(csrc, name, tgt):
                wins.append((name, addr, size, lbl, csrc))
        if args.apply and wins:
            for name, addr, size, lbl, csrc in wins:
                body = csrc if csrc.startswith("//cpp") else \
                    (S.pretty(csrc) if " { " in csrc else csrc)
                L.bank({"addr": addr, "name": name, "size": size,
                        "module": label, "versions": [f"template:{lbl}"]}, body)
        if wins:
            rows.append((label, len(wins)))
            grand += len(wins)
            print(f"  {label:8} +{len(wins)}")
    print("=" * 40)
    print(f"{'APPLIED' if args.apply else 'DRY-RUN'} new matches across "
          f"{len(rows)} modules: {grand}")


if __name__ == "__main__":
    main()
