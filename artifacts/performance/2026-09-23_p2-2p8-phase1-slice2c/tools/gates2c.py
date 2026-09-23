"""Slice 2b/2c gate tables: percentiles, bands, per-range means, VBlank shares.

Usage: gates2b.py CONTROL_ARM CANDIDATE_ARM [more arms]
Arms are artifact names (ARM-rows.csv + ARM.json in the slice 2c folder; prefix 2b: for the slice 2b folder).
Per-frame deltas are paired by frame (valid only when the replay digests are
identical: scripts/compare-replay-digest.py).
"""
import csv
import json
import os
import statistics
import sys

ART = r'D:\Stuff\DevFolder\Smash64DS_Port\artifacts\performance\2026-09-23_p2-2p8-phase1-slice2c'
ART2B = ART.replace('slice2c', 'slice2b')
RANGES = (('2-195 entry', 2, 195), ('196-797', 196, 797), ('798-1043 P0 episode', 798, 1043),
          ('1044-1974', 1044, 1974))


def load(arm):
    base = ART
    if arm.startswith('2b:'):
        base, arm = ART2B, arm[3:]
    rows = list(csv.DictReader(open(os.path.join(base, arm + '-rows.csv'))))
    d = json.load(open(os.path.join(base, arm + '.json')))
    return {int(r['frame']): r for r in rows}, d


def pct(vals, q):
    s = sorted(vals)
    k = (len(s) - 1) * q
    f = int(k)
    c = min(f + 1, len(s) - 1)
    return s[f] + (s[c] - s[f]) * (k - f)


def col(rows, name, frames=None):
    return [int(r[name]) for f, r in sorted(rows.items()) if frames is None or f in frames]


def bands(rows, name, key='WORK-H'):
    s = sorted(rows.values(), key=lambda r: int(r[key]))
    n = len(s)
    out = []
    for label, lo, hi in (('P40-60', .40, .60), ('P90-95', .90, .95), ('P95-99', .95, .99), ('P99+', .99, 1.0)):
        seg = s[int(n * lo):max(int(n * hi), int(n * lo) + 1)]
        out.append((label, statistics.mean(int(r[name]) for r in seg)))
    return out


def main():
    arms = sys.argv[1:]
    data = [(a,) + load(a) for a in arms]
    print('arms:', ', '.join('%s (rom %s, %d rows)' % (a, d.get('romSha256', '')[:12], len(rows))
                             for a, rows, d in data))
    print()
    print('%-22s' % 'metric' + ''.join('%16s' % a[-16:] for a, _, _ in data))
    for name in ('WORK-H', 'FTR', 'MTEX'):
        for q, label in ((.5, 'P50'), (.95, 'P95'), (.99, 'P99')):
            print('%-22s' % ('%s %s' % (name, label)) +
                  ''.join('%16s' % '{:,.0f}'.format(pct(col(rows, name), q)) for _, rows, _ in data))
        print('%-22s' % ('%s mean' % name) +
              ''.join('%16s' % '{:,.0f}'.format(statistics.mean(col(rows, name))) for _, rows, _ in data))
    for key, label in (('vbi2', '2-VBlank frames'), ('vbi3', '3-VBlank'), ('vbi4', '4-VBlank'), ('vbi5plus', '5+-VBlank')):
        print('%-22s' % label + ''.join('%16s' % '{} ({:.1%})'.format(
            d.get(key), (d.get(key) or 0) / max(1, d.get('vbiTotal') or len(rows))) for _, rows, d in data))
    print()
    print('bands (WORK-H rank within each run)')
    for name in ('FTR', 'WORK-H'):
        for i, (label, _) in enumerate(bands(data[0][1], name)):
            print('  %-8s %-6s' % (name, label) +
                  ''.join('%16s' % '{:,.0f}'.format(bands(rows, name)[i][1]) for _, rows, _ in data))
    print()
    base = data[0][1]
    print('per-range means (frame 3 = the lab admission frame: the poke lands at the first frame-complete '
          'marker, the admission runs at the next frame end; listed apart, excluded from its range)')
    print('  %-22s %-6s' % ('range', 'col') + ''.join('%16s' % a[-16:] for a, _, _ in data) + '%16s' % 'paired d(last)')
    for label, lo, hi in (('frame 3 (admission)', 3, 3),) + RANGES:
        frames = set(f for f in base if lo <= f <= hi and (f != 3 or lo == 3))
        for name in ('FTR', 'WORK-H', 'MTEX'):
            means = []
            for _, rows, _ in data:
                v = col(rows, name, frames)
                means.append(statistics.mean(v) if v else 0)
            last = data[-1][1]
            common = [f for f in frames if f in last]
            paired = statistics.mean(int(last[f][name]) - int(base[f][name]) for f in common) if common else 0
            print('  %-22s %-6s' % (label, name) + ''.join('%16s' % '{:,.0f}'.format(m) for m in means) +
                  '%16s' % '{:+,.0f}'.format(paired))


if __name__ == '__main__':
    main()
