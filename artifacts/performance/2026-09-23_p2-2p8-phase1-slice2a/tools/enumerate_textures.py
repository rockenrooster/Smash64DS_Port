"""Slice 2a: every fighter's reachable texel/palette banks, enumerated from
the relocData closure (scripts/fighters/estimate_fighter_pack.py, the same
walk the pack estimator uses), cross-checked against the runtime keys a
census-b run recorded. Host only; nothing is drawn."""
import collections, json, os, sys

ROOT = r'D:\Stuff\DevFolder\Smash64DS_Port'
ART = os.path.join(ROOT, r'artifacts\performance\2026-09-23_p2-2p8-phase1-slice2a')
sys.path.insert(0, os.path.join(ROOT, 'scripts', 'fighters'))
import estimate_fighter_pack as est  # noqa: E402

types = est.TypeTable()
types.load_dirs(est.HEADER_DIRS)
census = est.parse_native_image_census(est.NATIVE_IMAGE_FLAGS_BY_NAME['hwtri'])
manifest = est.load_manifest()
fighters = [f['fighter'] for f in manifest['fighters']]
if len(sys.argv) > 1:
    fighters = sys.argv[1].split(',')
ledgers = est.build_fighter_ledgers(fighters, types, 'hwtri', census)

out = {}
for f, L in ledgers.items():
    banks = []
    for a in L.assignments:
        if a.disposition not in ('TEXEL_BANK', 'PALETTE_BANK'):
            continue
        r = a.row
        readers = L.graph.readers_of(r)
        banks.append({
            'kind': 'texel' if a.disposition == 'TEXEL_BANK' else 'palette',
            'file_id': r.file_id, 'offset': r.offset, 'size': r.size,
            'symbol': r.symbol, 'type': r.type_name,
            'costumes': (sorted(a.costume_index) if isinstance(a.costume_index, (set, frozenset, list, tuple)) else a.costume_index),
            'readers': sorted({x.type_name for x in readers}),
            'reader_symbols': sorted({x.symbol for x in readers})[:6],
        })
    files = sorted({b['file_id'] for b in banks})
    out[f] = {'costume_ids': list(L.costume_ids), 'files': files, 'banks': banks}
    tb = [b for b in banks if b['kind'] == 'texel']
    pb = [b for b in banks if b['kind'] == 'palette']
    print('%-10s files %-40s texel banks %3d %7d B | palette banks %3d %6d B' % (
        f, ','.join('0x%x' % x for x in files), len(tb), sum(b['size'] for b in tb),
        len(pb), sum(b['size'] for b in pb)))
json.dump(out, open(os.path.join(ART, 'enumerated-banks.json'), 'w'), indent=1)
print('wrote enumerated-banks.json')
