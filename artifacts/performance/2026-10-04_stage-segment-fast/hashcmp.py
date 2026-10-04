"""Compare two per-logic-frame hash logs (lines 'H l=<n> h=<hex> w=<n> ...')."""
import re
import sys


def load(path):
    rows = {}
    for line in open(path, errors='ignore'):
        if not line.startswith('H '):
            continue
        fields = dict(kv.split('=', 1) for kv in line.split()[1:] if '=' in kv)
        rows[int(fields['l'])] = (fields.get('h'), fields.get('w'),
                                  fields.get('s'), line.strip())
    return rows


a = load(sys.argv[1])
b = load(sys.argv[2])
common = sorted(set(a) & set(b))
same = [l for l in common if a[l][:3] == b[l][:3]]
diff = [l for l in common if a[l][:3] != b[l][:3]]
print(f"common {len(common)} same {len(same)} diff {len(diff)}")
for l in diff[:6]:
    print('  A', a[l][3])
    print('  B', b[l][3])
