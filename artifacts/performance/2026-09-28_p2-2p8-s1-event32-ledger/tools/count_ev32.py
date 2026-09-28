"""Count event32 command words (ledger entries) in AObj32 fighter clips.

Mirrors ndsAObjEvent32PlanStream for DObj owners: each figatree entry is a
script; commands are walked until End, Jump/SetAnim targets are followed, and
every distinct command word is one normalized-ledger entry."""
import struct, sys, os

NITRO = r'D:\Stuff\DevFolder\Smash64DS_Port\builds\build\nitrofs\reloc\reloc_animations'
HDR = 0x40 + 0x10


def words(body):
    return [struct.unpack('>I', body[i:i + 4])[0] for i in range(0, len(body) - 3, 4)]


def reloc_targets(w, first):
    """Follow the internal reloc chain from `first`: entry -> target offset."""
    tgt = {}
    i = first
    if first == 0xffff:
        return tgt
    seen = set()
    while i not in seen and i < len(w):
        seen.add(i)
        v = w[i]
        tgt[i] = (v & 0xffff) * 4
        nxt = v >> 16
        if nxt == 0xffff:
            break
        i = nxt
    return tgt


def popcount(x):
    return bin(x).count('1')


def count_file(path):
    b = open(path, 'rb').read()
    body = b[HDR:]
    first = struct.unpack('<I', b[0x44:0x48])[0] & 0xffff
    w = words(body)
    rel = reloc_targets(w, first)
    # figatree: leading words that are reloc slots or zero, until the first
    # word that is neither (script data begins).
    entries = []
    i = 0
    while i < len(w) and (i in rel or w[i] == 0):
        entries.append(rel.get(i))
        i += 1
    cmds = set()

    def walk(off, depth=0):
        k = off // 4
        while 0 <= k < len(w) and depth < 16:
            if k in cmds:
                return
            v = w[k]
            op = (v >> 25) & 0x7f
            fl = (v >> 15) & 0x3ff
            cmds.add(k)
            if op == 0:
                return
            if op in (1, 14):
                t = rel.get(k + 1)
                if t is not None:
                    walk(t, depth + 1)
                if op == 1:
                    return
                k += 2
                continue
            if op in (2, 12, 15, 16):
                n = 0
            elif op in (3, 4, 7, 8, 9, 10, 11, 17):
                n = popcount(fl)
            elif op in (5, 6):
                n = popcount(fl) * 2
            elif op == 13:
                n = 1
            else:
                return
            k += 1 + n
    for e in entries:
        if e is not None:
            walk(e)
    return len(entries), sum(1 for e in entries if e is not None), len(cmds)


total = 0
for name in sys.argv[1:]:
    p = os.path.join(NITRO, name)
    n, live, c = count_file(p)
    total += c
    print(f'{name}: entries {n} live {live} commands {c}')
print('total commands', total)
