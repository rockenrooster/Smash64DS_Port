"""Print the slice 2b gate / lab counters of one or more sampler runs."""
import json
import os
import sys

ART = r'D:\Stuff\DevFolder\Smash64DS_Port\artifacts\performance\2026-09-23_p2-2p8-phase1-slice2b'
KEYS = [
    'gNdsFtrLeanAdmit', 'gNdsFtrLeanRoute',
    'gNdsFtrAdmitLab.runs', 'gNdsFtrAdmitLab.frame', 'gNdsFtrAdmitLab.regions', 'gNdsFtrAdmitLab.bg3_not_empty',
    'gNdsFtrAdmitLab.lock_before', 'gNdsFtrAdmitLab.lock_after', 'gNdsFtrAdmitLab.locked', 'gNdsFtrAdmitLab.lock_frame',
    'gNdsFtrAdmitLab.bases', 'gNdsFtrAdmitLab.records', 'gNdsFtrAdmitLab.skipped_costume', 'gNdsFtrAdmitLab.skipped_hat',
    'gNdsFtrAdmitLab.applied', 'gNdsFtrAdmitLab.fails', 'gNdsFtrAdmitLab.asset_fails', 'gNdsFtrAdmitLab.pruned', 'gNdsFtrAdmitLab.absent', 'gNdsFtrAdmitLab.libc_skipped', 'gNdsFtrAdmitLab.slot_skipped', 'gNdsFtrAdmitLab.libc_top_min', 'gNdsFtrAdmitLab.read_ticks', 'gNdsFtrAdmitLab.apply_ticks',
    'gNdsFtrAdmitLab.reject_mask', 'gNdsFtrAdmitLab.reject_first[0]', 'gNdsFtrAdmitLab.reject_first[1]',
    'gNdsFtrAdmitLab.reject_first[2]', 'gNdsFtrAdmitLab.reject_first[3]', 'gNdsFtrAdmitLab.reject_first[4]',
    'gNdsFtrAdmitLab.reject_first[5]',
    'gNdsFtrAdmitLab.entries_admitted', 'gNdsFtrAdmitLab.bytes_admitted', 'gNdsFtrAdmitLab.bytes_in_ab',
    'gNdsFtrAdmitLab.bytes_in_d', 'gNdsFtrAdmitLab.free_before', 'gNdsFtrAdmitLab.largest_before',
    'gNdsFtrAdmitLab.free_after', 'gNdsFtrAdmitLab.largest_after', 'gNdsFtrAdmitLab.ticks',
    'gNdsFtrAdmitLab.per_fighter_applied[0]', 'gNdsFtrAdmitLab.per_fighter_applied[1]',
    'gNdsFtrAdmitLab.per_fighter_applied[2]', 'gNdsFtrAdmitLab.per_fighter_applied[3]',
    'gNdsFtrAdmitLab.per_fighter_bytes[0]', 'gNdsFtrAdmitLab.per_fighter_bytes[1]',
    'gNdsFtrAdmitLab.per_fighter_bytes[2]', 'gNdsFtrAdmitLab.per_fighter_bytes[3]',
    'gNdsFtrAdmitLab.outside[0]', 'gNdsFtrAdmitLab.outside[1]', 'gNdsFtrAdmitLab.outside[2]',
    'gNdsFtrAdmitLab.outside[3]', 'gNdsFtrAdmitLab.outside_count',
    'gNdsFtrLeanAdmitFail', 'gNdsFtrLeanAdmitFailFirst[0]', 'gNdsFtrLeanAdmitFailFirst[1]',
    'gNdsFtrLeanAdmitFailFirst[2]', 'gNdsFtrLeanAdmitFailFirst[3]',
    'gNdsFtrAdmitDynReadyHigh', 'gNdsFtrAdmitDynAdmittedHigh', 'gNdsFtrAdmitDynTouchedHigh', 'gNdsFtrAdmitDynFull',
    'gNdsVramBankDLent', 'gNdsVramBg3RefusedWrites', 'gNdsVramBankDTakes',
    'gNdsRendererNativeFailure.count', 'gNdsRendererNativeFailure.status', 'gNdsRendererNativeFailure.reason',
    'gNdsRendererNativeDirectReject.count',
    'gNdsFtrLean.go_frame', 'gNdsFtrLean.fighter_uploads', 'gNdsFtrLean.fighter_uploads_after_go',
    'gNdsFtrLean.fighter_upload_bytes_after_go', 'gNdsFtrLean.reject_count', 'gNdsFtrLean.reject_mask',
    'gNdsFighterPacketHits', 'gNdsFighterPacketRecords',
    'gNdsVramCensus.key_uploads[0]', 'gNdsVramCensus.key_uploads[1]', 'gNdsVramCensus.key_uploads[2]',
    'gNdsVramCensus.key_uploads[3]', 'gNdsTaskmanGeneralHeapFreeMin', 'gNdsTaskmanLibcTopChunkMin', 'gNdsTaskmanLibcRuntimeHighWater',
    'gNdsVramCensus.go.tex_bytes', 'gNdsVramCensus.go.tex_bytes_d', 'gNdsVramCensus.go.tex_free_usable',
    'gNdsVramCensus.go.tex_largest_usable', 'gNdsVramCensus.after.tex_bytes', 'gNdsVramCensus.after.tex_bytes_d',
    'gNdsVramCensus.after.tex_free_usable', 'gNdsVramCensus.after.tex_largest_usable',
    'gNdsVramCensus.min_largest_free', 'gNdsVramCensus.min_largest_free_frame', 'gNdsVramCensus.peak_tex_bytes',
]


def load(arm):
    p = arm if arm.endswith('.json') else os.path.join(ART, arm + '.json')
    d = json.load(open(p))
    return d, {e['name']: e.get('value') for e in d['extras']}


def main():
    runs = [(a,) + load(a) for a in sys.argv[1:]]
    print('%-44s' % 'field' + ''.join('%16s' % a[-16:] for a, _, _ in runs))
    for k in KEYS:
        vals = [ex.get(k) for _, _, ex in runs]
        if all(v is None for v in vals):
            continue
        print('%-44s' % k.replace('gNdsFtrAdmitLab.', 'lab.').replace('gNdsVramCensus.', 'vc.') +
              ''.join('%16s' % ('-' if v is None else '{:,}'.format(v)) for v in vals))
    for a, d, ex in runs:
        print(a, 'rom', d.get('romSha256', '')[:16], 'vbi2..5+', d.get('vbi2'), d.get('vbi3'), d.get('vbi4'),
              d.get('vbi5plus'), 'rows', len(d.get('rows', [])))
        for i in range(8):
            w = [ex.get('gNdsFtrAdmitLab.outside_first[%d][%d]' % (i, j)) for j in range(3)]
            if w[0]:
                print('   outside[%d] slot %d asset* %03x off %05x tlut %08x w2 %08x' % (
                    i, w[0] >> 28, (w[0] >> 20) & 0xff, w[0] & 0xfffff, w[1], w[2]))


if __name__ == '__main__':
    main()
