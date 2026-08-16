#!/usr/bin/env python3
"""Per-symbol ITCM census: bytes resident vs bytes actually executed.

Inputs, both already on disk, no build required:
  1. objdump -d --section=.itcm of a linked ELF  (the block decomposition)
  2. a per-PC execution CSV from the profiler     (which PCs ever executed)

The disassembly's linear walk IS the non-overlapping block decomposition:
objdump emits one header per address, so overlapping aliases of one blob
(__aeabi_frsub / __subsf3 / __addsf3) come out as three adjacent blocks whose
sizes sum to the blob.  Summing nm sizes instead double-counts and produced a
total larger than the region last time.

Usage: census.py <disasm> <pc-csv> [--col all_instructions]
"""
import csv
import re
import sys

HDR = re.compile(r'^([0-9a-f]{8}) <(.+)>:$')
INS = re.compile(r'^\s*([0-9a-f]+):\t([0-9a-f ]+)\t(.*)$')


def main() -> int:
    dis_path, csv_path = sys.argv[1], sys.argv[2]
    col = 'all_instructions'
    if '--col' in sys.argv:
        col = sys.argv[sys.argv.index('--col') + 1]

    executed = set()
    with open(csv_path, newline='') as fh:
        for row in csv.DictReader(fh):
            if int(row[col]) > 0:
                executed.add(int(row['pc'], 16))

    blocks = []          # name, addr, [(pc, size, is_word)]
    cur = None
    for line in open(dis_path, encoding='utf-8', errors='replace'):
        line = line.rstrip('\n')
        m = HDR.match(line)
        if m:
            cur = {'name': m.group(2), 'addr': int(m.group(1), 16), 'ins': []}
            blocks.append(cur)
            continue
        m = INS.match(line)
        if m and cur is not None:
            pc = int(m.group(1), 16)
            size = 4 if len(m.group(2).replace(' ', '')) >= 8 else 2
            cur['ins'].append((pc, size, m.group(3).startswith('.word')))

    rows = []
    for b in blocks:
        resident = sum(s for _, s, _ in b['ins'])
        words = sum(s for _, s, w in b['ins'] if w)
        code = resident - words
        hot = sum(s for pc, s, w in b['ins'] if not w and pc in executed)
        rows.append((b['addr'], b['name'], resident, code, words, hot,
                     len(b['ins'])))

    rows.sort(key=lambda r: -r[2])
    tot_res = sum(r[2] for r in rows)
    tot_hot = sum(r[5] for r in rows)
    dead = [r for r in rows if r[5] == 0]
    print('PER-SYMBOL ITCM CENSUS  (%s, column %s)' % (dis_path.split('/')[-1], col))
    print('%-46s %8s %8s %8s %8s %7s' %
          ('symbol', 'addr', 'resident', 'code', 'exec', 'exec%'))
    for addr, name, res, code, words, hot, _ in rows:
        print('%-46s %08x %8d %8d %8d %6.1f%%' %
              (name[:46], addr, res, code, hot,
               (100.0 * hot / code) if code else 0.0))
    print('-' * 92)
    print('TOTAL resident %d   executed %d   never-executed %d   (%.1f%% of ITCM cold)'
          % (tot_res, tot_hot, tot_res - tot_hot,
             100.0 * (tot_res - tot_hot) / tot_res))
    print('BLOCKS WITH ZERO EXECUTED BYTES: %d blocks, %d bytes'
          % (len(dead), sum(r[2] for r in dead)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
