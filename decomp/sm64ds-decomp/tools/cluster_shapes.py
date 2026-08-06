"""Cluster the unmatched leaf functions by instruction shape.

The cheapest LLM tier isn't "send every hard function to a model" -- it's: find
the shapes that recur, crack ONE representative of each (by hand / subscription),
then turn that into a free template rule that clears the whole cluster.

This tool does the free, deterministic half:
  - filter the pool to functions a template could plausibly match
    (pure leaves: no calls, no coprocessor intrinsics, not oversized)
  - group them by instruction-shape signature
  - print clusters biggest-first, each with a representative address

Usage:
    python tools/cluster_shapes.py [--max 0x200] [--cap 0x40] [--top 25]
"""
import argparse
import sys
import pathlib
from collections import defaultdict

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import swarm as S
import match as M

CALL = {"bl", "b", "blx", "bxj"}
COPROC = {"mcr", "mrc", "mcrr", "mrrc", "cdp", "cdp2", "mcr2", "mrc2"}


def leaf_sig(ins):
    """Mnemonic sequence up to the first `bx lr`; None if not a clean leaf."""
    out = []
    for i in ins:
        if i.mnemonic in CALL or i.mnemonic in COPROC:
            return None
        out.append(i.mnemonic)
        if i.mnemonic == "bx" and S.squash(i.op_str) == "lr":
            return tuple(out)
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--min", type=lambda x: int(x, 0), default=0x4)
    ap.add_argument("--max", type=lambda x: int(x, 0), default=0x200)
    ap.add_argument("--cap", type=lambda x: int(x, 0), default=0x40,
                    help="skip functions larger than this (low LLM odds)")
    ap.add_argument("--top", type=int, default=25)
    args = ap.parse_args()

    clusters = defaultdict(list)
    skipped_size = thunks = nonleaf = 0
    for t in S.enumerate_targets(args.min, args.max, "arm", 0):
        tgt = M.target_bytes(t["addr"], t["size"])
        ins = list(S.md.disasm(tgt, 0))
        if S.is_thunk(ins):
            thunks += 1
            continue
        if any(rule(t["name"], ins, tgt) for rule in S.RULES):
            continue  # a template already handles it
        if t["size"] > args.cap:
            skipped_size += 1
            continue
        sg = leaf_sig(ins)
        if sg is None:
            nonleaf += 1
            continue
        clusters[sg].append(t)

    ranked = sorted(clusters.items(), key=lambda kv: -len(kv[1]))
    total = sum(len(v) for v in clusters.values())
    print(f"filtered leaf pool: {total} functions in {len(clusters)} shapes "
          f"(skipped: {thunks} thunks, {skipped_size} oversized, {nonleaf} non-leaf/calls)\n")
    print(f"top {args.top} clusters (crack the representative -> template the rest):")
    for sg, fns in ranked[:args.top]:
        rep = fns[0]
        print(f"  {len(fns):4}x  rep 0x{rep['addr']:08x} (0x{rep['size']:x})  {' '.join(sg)}")


if __name__ == "__main__":
    main()
