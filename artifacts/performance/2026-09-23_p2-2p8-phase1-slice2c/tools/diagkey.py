"""Decode the per-slot outside-key diagnostic (first outside upload per slot
vs the admitted entry with the same image)."""
import json
import os
import sys

ART = r'D:\Stuff\DevFolder\Smash64DS_Port\artifacts\performance\2026-09-23_p2-2p8-phase1-slice2c'
F = ['image', 'image_format', 'image_size', 'image_width', 'tlut_image', 'tlut_count', 'data_layout', 'format',
     'size', 'width', 'height', 'render_tile', 'render_tmem', 'render_palette', 'render_tile_cms',
     'render_tile_cmt', 'render_tile_masks', 'render_tile_maskt', 'render_tile_shifts', 'render_tile_shiftt',
     'load_tile', 'load_uls', 'load_ult', 'load_lrs', 'load_dxt', 'load_texels', 'tile_uls', 'tile_ult',
     'tile_lrs', 'tile_lrt', 'line', 'flags', 'texel1_image', 'texel1_image_format', 'texel1_image_size',
     'texel1_image_width', 'texel1_load_kind', 'texel1_render_tmem', 'texel1_render_line',
     'texel1_render_palette', 'texel1_render_tile_cms', 'texel1_render_tile_cmt', 'texel1_render_tile_masks',
     'texel1_render_tile_maskt', 'texel1_render_tile_shifts', 'texel1_render_tile_shiftt', 'texel1_load_tile',
     'texel1_load_uls', 'texel1_load_ult', 'texel1_load_lrs', 'texel1_load_dxt', 'texel1_load_texels',
     'texel1_tile_uls', 'texel1_tile_ult', 'texel1_tile_lrs', 'texel1_tile_lrt', 'prim_lod_fraction',
     'combine_w0', 'combine_w1']

arm = sys.argv[1]
d = json.load(open(os.path.join(ART, arm + '.json')))
ex = {e['name']: e.get('value') for e in d['extras']}
for slot in range(4):
    st = ex.get('gNdsFtrAdmitLab.diag_state[%d]' % slot)
    same = ex.get('gNdsFtrAdmitLab.diag_same_image[%d]' % slot)
    print('slot', slot, 'state', st, '(1 = same image admitted, 2 = image never admitted)', 'same_image', same)
    for k in range(59):
        a = ex.get('gNdsFtrAdmitLab.diag_outside_key[%d][%d]' % (slot, k)) or 0
        b = ex.get('gNdsFtrAdmitLab.diag_admit_key[%d][%d]' % (slot, k)) or 0
        if a != b or k in (0, 4, 9, 10, 31):
            print('   %-26s outside %08x admit %08x %s' % (F[k], a, b, '<<' if a != b else ''))
print('outside per slot', [ex.get('gNdsFtrAdmitLab.outside[%d]' % i) for i in range(4)],
      'entries_admitted', ex.get('gNdsFtrAdmitLab.entries_admitted'),
      'applied', ex.get('gNdsFtrAdmitLab.applied'), 'fails', ex.get('gNdsFtrAdmitLab.fails'),
      'asset_fails', ex.get('gNdsFtrAdmitLab.asset_fails'))
