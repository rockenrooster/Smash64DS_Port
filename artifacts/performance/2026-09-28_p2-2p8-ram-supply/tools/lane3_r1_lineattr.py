#!/usr/bin/env python3
"""lane3_r1_lineattr.py -- bytes of one function attributed to source line ranges (DWARF).

Usage: lane3_r1_lineattr.py <objdump -d -l --no-show-raw-insn text> <func-end-addr-hex>
                            <file-suffix> <label:lo-hi> [<label:lo-hi> ...]

The objdump text is one function.  Every instruction takes the file:line marker that
precedes it; its size is the distance to the next instruction (literal-pool words inside
the function are attributed to the last marker seen).  Prints the bytes per label for
lines of <file-suffix> inside [lo, hi], plus the total attributed to that file.
Optimised code interleaves lines, so read the result as an estimate of what constant
folding can remove, not as an exact figure.
"""
import collections
import re
import sys

RE_LINE = re.compile(r'^(.*):(\d+)(?: \(discriminator \d+\))?$')
RE_INSN = re.compile(r'^\s*([0-9a-f]+):\t')


def main():
    path, end, suffix = sys.argv[1], int(sys.argv[2], 16), sys.argv[3]
    ranges = []
    for a in sys.argv[4:]:
        label, span = a.split(':')
        lo, hi = span.split('-')
        ranges.append((label, int(lo), int(hi)))
    insns = []
    cur = None
    for raw in open(path, errors='replace'):
        line = raw.rstrip('\n')
        m = RE_INSN.match(line)
        if m:
            insns.append((int(m.group(1), 16), cur))
            continue
        m = RE_LINE.match(line)
        if m and ('/' in m.group(1) or '\\' in m.group(1)):
            cur = (m.group(1).replace('\\', '/'), int(m.group(2)))
    per_line = collections.Counter()
    for k, (addr, loc) in enumerate(insns):
        nxt = insns[k + 1][0] if k + 1 < len(insns) else end
        per_line[loc] += nxt - addr
    total = sum(per_line.values())
    in_file = sum(v for (loc, v) in ((l, b) for l, b in per_line.items()) if loc and loc[0].endswith(suffix))
    print('function bytes %d, attributed to %s: %d' % (total, suffix, in_file))
    for label, lo, hi in ranges:
        n = sum(b for l, b in per_line.items() if l and l[0].endswith(suffix) and lo <= l[1] <= hi)
        print('  %-34s lines %5d-%-5d %6d B' % (label, lo, hi, n))
    other = collections.Counter()
    for l, b in per_line.items():
        if l and not l[0].endswith(suffix):
            other[l[0].split('/')[-1]] += b
    for f, b in other.most_common(6):
        print('  inlined from %-30s %6d B' % (f, b))


if __name__ == '__main__':
    main()
