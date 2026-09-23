"""Per kind x detail x costume admission record counts (slice 2c D3 evidence).

Usage: costume_counts.py PAYLOAD_2C.bin [PAYLOAD_2B.bin]
A record belongs to costume c when bit c of its costume mask (word 1) is set;
hat records (flag 0x10, Kirby copy hats) are counted apart because the runtime
admits a hat only when its copied kind is in the match.
"""
import struct
import sys

KINDS = ['Mario', 'Fox', 'Donkey', 'Samus', 'Luigi', 'Link', 'Yoshi', 'Captain',
         'Kirby', 'Pikachu', 'Purin', 'Ness']
F_HAT = 0x10


def load(path):
    data = open(path, 'rb').read()
    magic, version, kinds, words = struct.unpack_from('<4I', data, 0)
    index = [struct.unpack_from('<2I', data, 16 + 8 * i) for i in range(kinds * 2)]
    base = 16 + 8 * kinds * 2
    out = {}
    for k in range(kinds):
        for d in (0, 1):
            first, count = index[k * 2 + d]
            per = [0] * 8
            hats = 0
            for r in range(first, first + count):
                w = struct.unpack_from('<%dI' % words, data, base + r * words * 4)
                if w[0] & F_HAT:
                    hats += 1
                    continue
                for c in range(8):
                    if (w[1] >> c) & 1:
                        per[c] += 1
            out[(KINDS[k], d)] = (count, hats, per)
    return out


def main():
    a = load(sys.argv[1])
    b = load(sys.argv[2]) if len(sys.argv) > 2 else None
    print('%-8s %-4s %6s %5s  %s' % ('kind', 'det', 'recs', 'hats', 'non-hat records per costume 0..5'
                                     + ('   | 2b recs, per costume' if b else '')))
    for k in KINDS:
        for d in (1, 0):
            count, hats, per = a[(k, d)]
            line = '%-8s %-4s %6d %5d  %s' % (k, 'LOW' if d else 'HIGH', count, hats,
                                            ' '.join('%4d' % x for x in per[:6]))
            if b:
                bc, bh, bp = b[(k, d)]
                line += '   | %5d  %s' % (bc, ' '.join('%4d' % x for x in bp[:6]))
            print(line)


if __name__ == '__main__':
    main()
