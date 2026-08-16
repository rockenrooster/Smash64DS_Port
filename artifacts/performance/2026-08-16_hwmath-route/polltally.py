import csv, re, sys

DIS = sys.argv[1]
PCCSV = sys.argv[2]
DIV = float(sys.argv[3]) if len(sys.argv) > 3 else 160.0

func_re = re.compile(r'^([0-9a-f]{8}) <(.+)>:$')
insn_re = re.compile(r'^\s*([0-9a-f]+):\t([0-9a-f ]+)\t(.*)$')

rows = {}
with open(PCCSV, newline='') as fh:
    for row in csv.DictReader(fh):
        rows[int(row['pc'], 16)] = row

def st(a, k):
    r = rows.get(a)
    return int(r[k]) if r else 0

seq = []
cur = None
for raw in open(DIS, 'r', errors='replace'):
    raw = raw.rstrip('\n')
    m = func_re.match(raw)
    if m:
        cur = m.group(2)
        continue
    m = insn_re.match(raw)
    if m:
        seq.append((int(m.group(1), 16), cur, m.group(3).strip()))
pos = {a: i for i, (a, f, t) in enumerate(seq)}

CTL = {0x80, 0xb0}                       # DIVCNT / SQRTCNT via the 0x04000200 base
PARAM = {0x90, 0x94, 0x98, 0x9c, 0xb8, 0xbc}
store_re = re.compile(r'^str(d|h|b)?(eq|ne|cs|cc|mi|pl)?\s+\S+,\s*\[\S+(?:,\s*#(-?\d+))?\]')

def store_kind(text):
    m = store_re.match(text)
    if not m:
        return None
    off = int(m.group(3)) if m.group(3) else 0
    if off in CTL:
        return 'ctl'
    if off in PARAM:
        return 'param'
    return None

brre = re.compile(r'^(b|bne|bmi|bcs|bhi|blt|bpl)(\.n|\.w)?\s+([0-9a-f]+)')
out = []
for i, (a, f, t) in enumerate(seq):
    m = brre.match(t)
    if not m:
        continue
    tgt = int(m.group(3), 16)
    if not (0 < a - tgt <= 16) or tgt not in pos:
        continue
    body = seq[pos[tgt]:i + 1]
    txt = ' | '.join(b[2] for b in body)
    if 'ldrh' not in txt:
        continue
    if not re.search(r'#\s*(128|176)\]|0x0?4000(280|2b0)', txt):
        continue
    if '32768' not in txt:
        continue
    kind = '?'
    for j in range(pos[tgt] - 1, max(0, pos[tgt] - 60), -1):
        k = store_kind(seq[j][2])
        if k == 'ctl':
            kind = 'LEAD'
            break
        if k == 'param':
            kind = 'trail'
            break
    cyc = sum(st(x[0], 'marg_total_cycles') for x in body)
    allcyc = sum(st(x[0], 'all_total_cycles') for x in body)
    out.append((f, tgt, a, kind, st(tgt, 'marg_instructions'), cyc, allcyc))

lead = trail = unk = 0
alead = 0
print(f"{'function':46s} {'loop':>22s} {'kind':5s} {'iters':>8s} {'marg_cyc':>9s} {'tk/fr':>8s} {'all_cyc':>11s}")
for f, tgt, a, kind, execs, cyc, allcyc in out:
    if cyc == 0 and allcyc == 0:
        continue
    print(f"{f:46s} 0x{tgt:08x}-0x{a:08x} {kind:5s} {execs:8d} {cyc:9d} {cyc/DIV:8.1f} {allcyc:11d}")
    if kind == 'LEAD':
        lead += cyc
        alead += allcyc
    elif kind == 'trail':
        trail += cyc
    else:
        unk += cyc
print()
print(f"LEADING   marg_cyc={lead:9d} = {lead/DIV:8.1f} tk/fr   (all_cyc={alead})")
print(f"TRAILING  marg_cyc={trail:9d} = {trail/DIV:8.1f} tk/fr")
print(f"UNKNOWN   marg_cyc={unk:9d} = {unk/DIV:8.1f} tk/fr")
