"""Map runtime (asset, offset) texture keys back to relocData source offsets
through the battle core pack spans, then to estimator objects."""
import json, os, sys
ROOT = r'D:\Stuff\DevFolder\Smash64DS_Port'
ART = os.path.join(ROOT, r'artifacts\performance\2026-09-23_p2-2p8-phase1-slice2a')
PACKS = os.path.join(ROOT, r'builds\build-p2-fourcpu-tickhud\battle-core')
sys.path.insert(0, os.path.join(ROOT, 'scripts', 'fighters'))
import generate_preview_core_packs as fpc  # noqa: E402
import estimate_fighter_pack as est  # noqa: E402

KINDS = [('Donkey', '02'), ('Samus', '03'), ('Link', '05'), ('Kirby', '08')]
spans_by_asset = {}
for kind, num in KINDS:
    p = fpc.decode_pack(open(os.path.join(PACKS, num + '.fpc'), 'rb').read())
    for (aid, doff, dbytes, src, first, count, roff, rcnt) in p['sections']:
        spans_by_asset[aid] = p['spans'][first:first + count]


def to_source(aid, off):
    spans = spans_by_asset.get(aid)
    if spans is None:
        return None
    for old, new, size in spans:
        if new <= off < new + size:
            return old + (off - new)
    return None


types = est.TypeTable()
types.load_dirs(est.HEADER_DIRS)
census = est.parse_native_image_census(est.NATIVE_IMAGE_FLAGS_BY_NAME['hwtri'])
ledgers = est.build_fighter_ledgers([k for k, _ in KINDS], types, 'hwtri', census)
keys = json.load(open(os.path.join(ART, 'census-b3-keys.json')))
for slot, (kind, _) in enumerate(KINDS):
    L = ledgers[kind]
    disp = {(a.row.file_id, a.row.offset): a.disposition for a in L.assignments}
    objs = list(L.idx.objects)
    print('==', kind)
    for k in keys['slots'][slot]['keys']:
        for label, ref in (('img', k[0]), ('tlut', k[1])):
            if not isinstance(ref, list):
                continue
            aid, off = ref
            src = to_source(aid, off)
            if src is None:
                print('   %-4s 0x%x+0x%05x -> no span' % (label, aid, off))
                continue
            hit = [o for o in objs if o.file_id == aid and o.offset <= src < o.offset + o.size]
            o = hit[0] if hit else None
            print('   %-4s 0x%x+0x%05x -> src 0x%05x %-44s %-6s +%-5d size %5d %-20s %s' % (
                label, aid, off, src, (o.symbol[:44] if o else 'NO OBJECT'), (o.type_name if o else ''),
                (src - o.offset if o else 0), (o.size if o else 0),
                disp.get((o.file_id, o.offset), '-') if o else '',
                sorted({r.type_name for r in L.graph.readers_of(o)}) if o else ''))
