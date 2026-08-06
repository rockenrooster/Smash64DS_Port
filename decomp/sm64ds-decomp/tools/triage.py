"""Triage the unmatched functions into tiers, to size the tractable pile before
spending LLM time.

For every unmatched arm function it tries every template rule. A rule may produce
candidate C/C++ that does not byte-match in isolation only because the compiler
colored registers differently here than it did in the function's real translation
unit (it used ip/r12, we used r1). That source is still CORRECT; it just needs the
surrounding TU to color the same. So we compare two ways:

  strict   : word-for-word, reloc slots wildcarded (the real oracle; == sweep)
  regperm  : same, but a differing word still counts as OK if both words decode to
             the same instruction modulo a CONSISTENT register renaming

A function that is regperm-only is "template-correct, regalloc-blocked" -- a cheap
win once verified in TU context, and exactly the pile to point Opus/subagents at
first. Everything no rule touches is real-logic / LLM territory.

Usage:
    python tools/triage.py                 # full report
    python tools/triage.py --module ov002  # one module
    python tools/triage.py --max 0x80      # size cap (default 0x200)
"""
import argparse
import pathlib
import re
import sys
import tempfile
import collections

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import swarm as S
import match as M
import relocs as R
import modules as MOD
import sweep
import ledger as L

ALIAS = {"sb": "r9", "sl": "r10", "fp": "r11", "ip": "r12", "sp": "r13", "lr": "r14", "pc": "r15"}
FIXED = {"r13", "r14", "r15"}                       # sp / lr / pc never get permuted
REG_RE = re.compile(r"\b(r\d+|sb|sl|fp|ip|sp|lr|pc)\b")


def _skeleton(op):
    return REG_RE.sub("#", op)


def _regs(op):
    return [ALIAS.get(t, t) for t in REG_RE.findall(op)]


def classify(target, code, relocs):
    """Return 'strict', 'regperm', or 'none' for a candidate vs the ROM bytes."""
    if len(target) != len(code):
        return "none"
    strict = True
    c2t, t2c = {}, {}
    for i in range(0, len(target), 4):
        if i in relocs:                              # reloc slot: wildcard
            continue
        tw, cw = target[i:i + 4], code[i:i + 4]
        if tw == cw:
            continue
        strict = False
        ti = next(S.md.disasm(tw, 0), None)
        ci = next(S.md.disasm(cw, 0), None)
        if not (ti and ci) or ti.mnemonic != ci.mnemonic \
                or _skeleton(ti.op_str) != _skeleton(ci.op_str):
            return "none"
        tr, cr = _regs(ti.op_str), _regs(ci.op_str)
        if len(tr) != len(cr):
            return "none"
        for a, bb in zip(cr, tr):                    # a = candidate reg, bb = target reg
            if (a in FIXED or bb in FIXED) and a != bb:
                return "none"
            if c2t.setdefault(a, bb) != bb or t2c.setdefault(bb, a) != a:
                return "none"
    return "strict" if strict else "regperm"


def compile_candidate(csrc, name):
    cpp = csrc.startswith("//cpp")
    with tempfile.TemporaryDirectory() as td:
        cf = pathlib.Path(td) / ("c.cpp" if cpp else "c.c")
        cf.write_text(csrc)
        obj = M.compile_c(cf, M.CANONICAL, S.CPP_FLAGS if cpp else M.DEFAULT_FLAGS)
    if obj is None:
        return None, None
    return M.extract_func(obj, name)


def best_for(name, ins, tgt, addr, relocs, syms):
    """Try every rule; return (tier, label) for the best candidate, tier in
    strict > regperm > none."""
    best = ("none", None)
    cands = []
    for rule in S.RULES:
        c = rule(name, ins, tgt)
        if c:
            cands.append(c)
    for rule in S.RELOC_RULES:
        c = rule(name, ins, tgt, addr, relocs, syms)
        if c:
            cands.append(c)
    for csrc, label in cands:
        code, crel = compile_candidate(csrc, name)
        if code is None:
            continue
        tier = classify(tgt, code, crel)
        if tier == "strict":
            return ("strict", label)
        if tier == "regperm" and best[0] != "strict":
            best = ("regperm", label)
    return best


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--module", default=None)
    ap.add_argument("--max", type=lambda x: int(x, 0), default=0x200)
    args = ap.parse_args()

    done = L.load_done()
    gsyms = R.load_all_syms()
    tiers = collections.Counter()
    regperm_by_rule = collections.Counter()
    regperm_examples = []
    for mod in MOD.modules():
        label = "arm9" if mod["name"] == "main" else mod["name"]
        if args.module and mod["name"] != args.module:
            continue
        relocs = R.load_relocs_file(mod["relocs"])
        data = mod["bin"].read_bytes()
        for name, addr, size in sweep.funcs(mod):
            if (label, addr) in done or size > args.max:
                continue
            tgt = data[addr - mod["base"]:addr - mod["base"] + size]
            ins = list(S.md.disasm(tgt, 0))
            if not ins or S.is_thunk(ins):
                continue
            tier, rule = best_for(name, ins, tgt, addr, relocs, gsyms)
            tiers[tier] += 1
            if tier == "regperm":
                regperm_by_rule[rule] += 1
                if len(regperm_examples) < 12:
                    regperm_examples.append((label, addr, size, rule))

    total = sum(tiers.values())
    print(f"=== triage over {total} unmatched arm funcs (size <= 0x{args.max:x}) ===")
    print(f"  strict-matchable now : {tiers['strict']:5}  (sweep would already take these)")
    print(f"  regperm-only         : {tiers['regperm']:5}  (template-correct, regalloc-blocked)")
    print(f"  no template          : {tiers['none']:5}  (real-logic / LLM tier)")
    if regperm_by_rule:
        print("\n  regperm wins by rule:")
        for rule, n in regperm_by_rule.most_common():
            print(f"    {n:4}  {rule}")
        print("\n  examples:")
        for lbl, a, s, rule in regperm_examples:
            print(f"    {lbl} 0x{a:08x} (0x{s:x})  via {rule}")


if __name__ == "__main__":
    main()
