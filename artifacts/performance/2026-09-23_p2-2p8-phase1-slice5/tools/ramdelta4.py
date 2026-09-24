"""Shipping static RAM delta between two ELFs (slice 2c).

Usage: ramdelta2c.py BEFORE.elf AFTER.elf
  1. data/bss/dtcm symbols (nm types b/B/d/D/s/S/g/G): every symbol whose size
     differs, and the total -- the slice 2b methodology, widened from a fixed
     symbol list to the whole image so nothing moved can hide
  2. `size -A` section totals (text included: on the DS the whole ARM9 image
     lives in main RAM, and binary growth costs the taskman arena one for one)
"""
import collections
import subprocess
import sys

NM = r'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
SIZE = r'C:\devkitPro\devkitARM\bin\arm-none-eabi-size.exe'


def nm(elf):
    out = collections.defaultdict(int)
    kinds = {}
    for line in subprocess.check_output([NM, '-S', elf], text=True).splitlines():
        p = line.split()
        if len(p) == 4:
            out[p[3]] += int(p[1], 16)
            kinds[p[3]] = p[2]
    return out, kinds


def sections(elf):
    out = {}
    for line in subprocess.check_output([SIZE, '-A', elf], text=True).splitlines():
        p = line.split()
        if len(p) == 3 and p[0].startswith('.') and p[1].isdigit():
            out[p[0]] = int(p[1])
    return out


def main():
    before, after = sys.argv[1], sys.argv[2]
    (sa, ka), (sb, kb) = nm(before), nm(after)
    data_types = set('bBdDsSgG')
    rows = []
    total = 0
    for name in sorted(set(sa) | set(sb)):
        kind = kb.get(name, ka.get(name))
        if kind not in data_types:
            continue
        d = sb.get(name, 0) - sa.get(name, 0)
        if d != 0:
            rows.append((d, name, sa.get(name, 0), sb.get(name, 0)))
            total += d
    print('data/bss/dtcm symbols whose size changed (%s -> %s)' % (before.split('\\')[-1], after.split('\\')[-1]))
    print('%-52s %10s %10s %9s' % ('symbol', 'before', 'after', 'delta'))
    for d, name, a, b in sorted(rows, key=lambda r: (r[0], r[1])):
        print('%-52s %10s %10s %+9d' % (name, '{:,}'.format(a), '{:,}'.format(b), d))
    print('%-52s %10s %10s %+9d' % ('TOTAL data/bss/dtcm symbol delta', '', '', total))
    text_total = 0
    for name in set(sa) | set(sb):
        kind = kb.get(name, ka.get(name))
        if kind in ('t', 'T'):
            text_total += sb.get(name, 0) - sa.get(name, 0)
    print('%-52s %+10d' % ('code symbols (t/T) delta', text_total))
    print()
    xa, xb = sections(before), sections(after)
    print('%-28s %12s %12s %9s' % ('section', 'before', 'after', 'delta'))
    grand = 0
    for sec in sorted(set(xa) | set(xb)):
        if sec.startswith('.debug') or sec in ('.comment', '.ARM.attributes'):
            continue
        a, b = xa.get(sec, 0), xb.get(sec, 0)
        grand += b - a
        if a != b:
            print('%-28s %12s %12s %+9d' % (sec, '{:,}'.format(a), '{:,}'.format(b), b - a))
    print('%-28s %12s %12s %+9d' % ('TOTAL loaded sections', '', '', grand))


if __name__ == '__main__':
    main()
