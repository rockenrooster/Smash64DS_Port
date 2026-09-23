import csv, sys, statistics
def load(p):
    rows = list(csv.DictReader(open(p)))
    return rows
def pct(vals, q):
    s = sorted(vals); 
    if not s: return 0
    k = (len(s)-1)*q; f = int(k); c = min(f+1, len(s)-1)
    return s[f] + (s[c]-s[f])*(k-f)
def summary(rows, col):
    v = [int(r[col]) for r in rows if r.get(col) not in (None, '')]
    return dict(p50=pct(v,.5), p95=pct(v,.95), p99=pct(v,.99), mean=statistics.mean(v) if v else 0, n=len(v))
def bands(rows, col, key='WORK-H'):
    s = sorted(rows, key=lambda r: int(r[key]))
    n = len(s)
    out = {}
    for name, lo, hi in (('P40-60', .40, .60), ('P90-95', .90, .95), ('P95-99', .95, .99), ('P99+', .99, 1.0)):
        seg = s[int(n*lo):max(int(n*hi), int(n*lo)+1)]
        out[name] = statistics.mean(int(r[col]) for r in seg)
    return out
if __name__ == '__main__':
    files = sys.argv[1:]
    cols = ['WORK-H', 'FTR', 'STG', 'MISC', 'OTHR', 'SRC', 'MTEX']
    for f in files:
        rows = load(f)
        print('==', f, 'rows', len(rows))
        for c in cols:
            if c not in rows[0]: continue
            s = summary(rows, c)
            b = bands(rows, c)
            print(f"{c:7s} p50 {s['p50']:>11,.0f} p95 {s['p95']:>11,.0f} p99 {s['p99']:>11,.0f} mean {s['mean']:>11,.0f} | " +
                  ' '.join(f"{k} {v:,.0f}" for k, v in b.items()))
