"""Item A. The in-match card-read census, from two whole-match per-stop runs.

Both runs are the same ROM (builds/build-c219-animitcm-ship) at the same
invocation as the basis run; only -RingStopStride and -PerStopGlobals differ.
Their 1,600 bucket rows are byte-identical to the basis run's, which is the
determinism control this cycle relies on everywhere else.
"""
import json, csv, sys

A = 'artifacts/performance/2026-08-16_match-io-audit/'
def stops(p): return json.load(open(A + p))['ringStopReads']

print('DETERMINISM CONTROL -- three separate emulator sessions, one ROM')
def key(p): return [tuple(r.values()) for r in csv.DictReader(open(p))]
base = key('artifacts/performance/2026-08-16_anim-itcm/ship-rows.csv')
for p in ('io-rows.csv', 'io8-rows.csv'):
    print(f'  {p:14s} identical to the basis 1,600 rows: {key(A+p) == base}')

for name, f in (('stride 96, 17 stops', 'io.json'), ('stride 8, 200 stops', 'io8.json')):
    rs = stops(f)
    print(f'\n{name}   frames {rs[0]["fromFrame"]}..{rs[-1]["frame"]}')
    for k in sorted(k for k in rs[0] if k.startswith('gNds') and not k.endswith('Delta')):
        print(f'  {k:44s} {rs[0][k]:>8,} -> {rs[-1][k]:>8,}   {rs[-1][k]-rs[0][k]:+,}')

rs = stops('io8.json')
print('\nEVERY 8-FRAME WINDOW THAT READ THE CARD')
for r in rs:
    if r['gNdsRelocAssetPayloadReadCountDelta']:
        print(f'  frames {r["fromFrame"]:>4}..{r["frame"]:<4}  '
              f'payload +{r["gNdsRelocAssetPayloadReadCountDelta"]}  '
              f'header +{r["gNdsRelocAssetHeaderReadCountDelta"]}  '
              f'cache-miss +{r["gNdsR2AnimCacheMissesDelta"]}  '
              f'pack-miss +{r["gNdsBattlePackMissesDelta"]}')
