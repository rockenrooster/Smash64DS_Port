"""Bucket composition by WORK-H percentile band for a four-CPU tick-HUD rows CSV."""
import csv, statistics, sys, collections
fn = sys.argv[1]
start = int(sys.argv[2]) if len(sys.argv) > 2 else 440
rows = [r for r in csv.DictReader(open(fn)) if int(r['frame']) >= start]
n = len(rows)
def pct(k, p):
    v = sorted(int(r[k]) for r in rows); return v[min(n - 1, int(p * n))]
print('rows', n, 'frames', rows[0]['frame'], '-', rows[-1]['frame'])
print('%-6s %9s %9s %9s %9s %9s' % ('bucket', 'mean', 'P50', 'P95', 'P99', 'max'))
for k in ['ALL', 'WORK-H', 'SRC', 'SINT', 'SCPU', 'SPHD', 'SHDT', 'SPRM', 'SCAT', 'FTR', 'STG', 'MISC', 'AUD', 'HUD', 'WAIT']:
    v = [int(r[k]) for r in rows]
    print('%-6s %9.0f %9d %9d %9d %9d' % (k, statistics.mean(v), pct(k, .5), pct(k, .95), pct(k, .99), max(v)))
h = collections.Counter(round(int(r['ALL']) / 559339) for r in rows)
print('VBlanks per presented frame:', sorted(h.items()))
print('mean FPS %.2f' % (33513982 / statistics.mean(int(r['ALL']) for r in rows)))
rows.sort(key=lambda r: int(r['WORK-H']))
bands = {'P40-60': rows[int(.4 * n):int(.6 * n)], 'P90-95': rows[int(.9 * n):int(.95 * n)],
         'P95-99': rows[int(.95 * n):int(.99 * n)], 'P99+': rows[int(.99 * n):]}
ks = ['WORK-H', 'SRC', 'SINT', 'SCPU', 'SPHD', 'SHDT', 'SPRM', 'SCAT', 'FTR', 'STG', 'MISC', 'AUD', 'HUD']
print('band means')
print('%-8s' % 'band' + ''.join('%9s' % k for k in ks))
for b, rs in bands.items():
    print('%-8s' % b + ''.join('%9.0f' % statistics.mean(int(r[k]) for r in rs) for k in ks))
