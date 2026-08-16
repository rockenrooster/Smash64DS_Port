"""The three region masks this cycle scans the v3-c221 capture with.

  whole    all 1,601 regions
  m80prof  the 80 most expensive regions by the profile's OWN non-idle cycles
           (total_cycles - halt_wait).  Self-contained: no cross-build mapping.
  m80gate  the 80 most expensive frames by WORK-H in the c220 BASIS tick-HUD run,
           carried across at the measured alignment `region = frame - 439`
           (../2026-08-16_sitr-excursion/align.py).  This is the population the
           gate's rank-80 is literally the last member of.

Both marginal-80 masks are emitted so the report can state their OVERLAP rather
than assume the cross-build carry is sound.  A saving that only lands on frames
below rank-80 moves the level by nothing, which is why the population matters.
"""
import csv

D = 'artifacts/performance/2026-08-16_ftr-attribution/'
REG = 'artifacts/performance/2026-08-16_sitr-excursion/v3-c221/arm9-profile.regions.csv'
RING = 'artifacts/performance/2026-08-16_camera-ship/ship220-rows.csv'
OFF = 439
TOPN = 80

regions = [{k: int(float(v)) if k != 'average_cycles' else 0
            for k, v in r.items()} for r in csv.DictReader(open(REG))]
nonidle = {r['region']: r['total_cycles'] - r['halt_wait'] for r in regions}
m80prof = set(sorted(nonidle, key=lambda k: -nonidle[k])[:TOPN])

ring = [{k: int(v) for k, v in r.items()} for r in csv.DictReader(open(RING))]
ring.sort(key=lambda r: -r['WORK-H'])
m80gate = set(r['frame'] - OFF for r in ring[:TOPN])
rank80 = ring[TOPN - 1]['WORK-H']

print('regions in profile        %d' % len(regions))
print('tick-HUD frames in basis  %d   rank-80 WORK-H %d' % (len(ring), rank80))
print('m80prof  %d regions, non-idle cycles %d..%d'
      % (len(m80prof), min(nonidle[r] for r in m80prof),
         max(nonidle[r] for r in m80prof)))
print('m80gate  %d regions (frames %d..%d mapped by -%d)'
      % (len(m80gate), min(m80gate) + OFF, max(m80gate) + OFF, OFF))
ov = m80prof & m80gate
print('OVERLAP  %d of %d (%.0f%%)' % (len(ov), TOPN, 100.0 * len(ov) / TOPN))
allr = set(nonidle)
mp = sum(nonidle[r] for r in m80prof) / float(len(m80prof))
mg = sum(nonidle[r] for r in m80gate & allr) / float(len(m80gate & allr))
mw = sum(nonidle.values()) / float(len(nonidle))
print('mean non-idle tk/fr: m80prof %.0f  m80gate %.0f  whole %.0f'
      % (mp / 2.0, mg / 2.0, mw / 2.0))

for name, s in (('m80prof', m80prof), ('m80gate', m80gate)):
    with open(D + 'regions-%s.txt' % name, 'w') as fh:
        for r in sorted(s):
            fh.write('%d\n' % r)
