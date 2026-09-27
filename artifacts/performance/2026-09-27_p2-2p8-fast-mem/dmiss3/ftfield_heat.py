"""FTStruct field heat from the melonDS read watch and fill census.

Usage: ftfield_heat.py DWATCH DMISS FRAMES POOL_BASE PTYPE [STRUCT_SIZE] [COUNT]
Prints, per top-level field: reads/frame (4 structs summed), the lines it spans
and fills/frame apportioned to it by its share of each line's reads.
"""
import collections
import csv
import re
import sys

dwatch, dmiss, frames = sys.argv[1], sys.argv[2], int(sys.argv[3])
base = int(sys.argv[4], 0)
ptype = sys.argv[5]
size = int(sys.argv[6]) if len(sys.argv) > 6 else 3012
count = int(sys.argv[7]) if len(sys.argv) > 7 else 4

fields = []
for line in open(ptype, encoding='utf-8', errors='replace'):
    m = re.match(r'/\*\s+(\d+)\s+(?::\s*\d+\s+)?\|\s+(\d+)\s+\*/    (?! )(.*?);?\s*$', line)
    if m:
        decl = m.group(3).rstrip(';').strip()
        name = re.findall(r'(\w+)(?:\[[^\]]*\])*\s*$', decl.split(':')[0].strip())
        if name:
            fields.append([int(m.group(1)), int(m.group(2)), name[0]])
# merge bitfield runs sharing an offset
fields.sort()
uniq = []
for f in fields:
    if uniq and uniq[-1][0] == f[0]:
        uniq[-1][2] += '|' + f[2]
        uniq[-1][1] = max(uniq[-1][1], f[1])
    else:
        uniq.append(f)
fields = uniq
offs = [f[0] for f in fields]


def field_of(off):
    import bisect
    i = bisect.bisect_right(offs, off) - 1
    return i if i >= 0 else None


reads = collections.Counter()      # field index -> reads
line_reads = collections.Counter()  # struct-relative line -> reads
line_field_reads = collections.defaultdict(collections.Counter)
for r in csv.DictReader(open(dwatch)):
    a = int(r['addr'], 16)
    if not (base <= a < base + size * count):
        continue
    off = (a - base) % size
    n = int(r['reads'])
    i = field_of(off)
    if i is None:
        continue
    reads[i] += n
    ln = off // 32
    line_reads[ln] += n
    line_field_reads[ln][i] += n
fills = collections.Counter()
for r in csv.DictReader(open(dmiss)):
    line = int(r['line'], 16)
    if not (base <= line + 31 and line < base + size * count):
        continue
    off = (line - base) % size
    fills[off // 32] += int(r['misses'])
field_fills = collections.Counter()
for ln, n in fills.items():
    tot = sum(line_field_reads[ln].values())
    if tot == 0:
        continue
    for i, rd in line_field_reads[ln].items():
        field_fills[i] += n * rd / tot
print('FTStruct reads/frame %.0f, fills/frame %.0f over %d lines' %
      (sum(reads.values()) / frames, sum(fills.values()) / frames, len(fills)))
print('reads/fr  fills/fr  off  size  field')
for i in sorted(reads, key=lambda k: -reads[k])[:80]:
    o, s, n = fields[i]
    print('%8.1f  %7.1f  %4d  %4d  %s' % (reads[i] / frames, field_fills[i] / frames, o, s, n))
