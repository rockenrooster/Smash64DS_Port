"""Static RAM of the symbols slice 2b touched, from `arm-none-eabi-nm -S`.

Usage: ramdelta.py NM_TXT [NM_TXT ...]   (one column per nm dump)
Prints each symbol's size per dump (bss/data/dtcm symbols only) and the
section totals when `size -A` dumps sit next to them (same stem + .size).
"""
import sys

SYMBOLS = [
    # lean BSS diet
    'gNdsFtrLean', 'sNdsFtrLeanStats', 'sNdsFtrLeanInputs', 'sNdsFtrLeanInstance', 'sNdsFtrLeanWorlds',
    'sNdsFighterPackets', 'sNdsFtrLeanSlots',
    # texture cache
    'sNdsRendererHardwareTextureCache', 'sNdsRendererHardwareTextureKeyPool',
    'sNdsRendererHardwareTextureLookup',
    # slice 2b admission (shipping part)
    'sNdsFtrAdmitIndex', 'sNdsFtrAdmitChunk', 'sNdsFtrAdmitRegions', 'sNdsFtrAdmitLocked',
    'sNdsFtrAdmitSavedLock', 'sNdsFtrAdmitBaseAsset', 'sNdsFtrAdmitBaseData', 'sNdsFtrAdmitBaseCount',
    'sNdsFtrAdmitGen', 'sNdsFtrAdmitCount', 'sNdsFtrAdmitKind', 'sNdsFtrAdmitCostume', 'sNdsFtrAdmitDetail',
    'sNdsFtrAdmitPlayer', 'sNdsFtrAdmitDone', 'gNdsFtrLeanAdmitFail', 'gNdsFtrLeanAdmitFailFirst',
    'gNdsVramBankDLent', 'gNdsVramBg3RefusedWrites', 'gNdsVramBankDTakes', 'gNdsVramBankDReturns',
    'gNdsVramBankDMissedExits',
    # lab only
    'gNdsFtrAdmitLab', 'gNdsVramCensus', 'gNdsFtrAdmitDynReadyHigh',
]


def load(path):
    out = {}
    for line in open(path):
        p = line.split()
        if len(p) == 4 and p[2] in 'bBdDgGsS':
            out.setdefault(p[3], []).append(int(p[1], 16))
    return out


def main():
    dumps = [(p, load(p)) for p in sys.argv[1:]]
    print('%-40s' % 'symbol' + ''.join('%14s' % p.split('\\')[-1].split('/')[-1][-14:] for p, _ in dumps))
    for s in SYMBOLS:
        vals = [sum(d.get(s, [])) if s in d else None for _, d in dumps]
        if all(v is None for v in vals):
            continue
        print('%-40s' % s + ''.join('%14s' % ('-' if v is None else '{:,}'.format(v)) for v in vals))


if __name__ == '__main__':
    main()
