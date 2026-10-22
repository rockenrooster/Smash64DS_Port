"""Split a C slice file into consecutive topic slices that the same parent
includes in order, so the translation unit's text is unchanged.

A plan (JSON) names the file, its parent, and for each new slice an anchor
regex (the first line of its first definition), the new file name and a
short header comment. Each cut moves up over the comment block attached to
the anchor (no blank line between) and must sit at brace depth 0 and
preprocessor depth 0. The original keeps everything before the first cut;
the parent gets one #include per new slice right after the original's.

usage: python split_slices.py [--check] PLAN_JSON
plan: {"file": ..., "parent": ..., "slices": [{"anchor": ..., "name": ..., "header": [...]}, ...]}
"""
import json
import re
import sys
from pathlib import Path

check = '--check' in sys.argv
plan = json.load(open([a for a in sys.argv[1:] if a != '--check'][0]))
src = Path(plan['file'])
parent = Path(plan['parent'])
raw = src.read_bytes()
crlf = b'\r\n' in raw
L = raw.decode('utf-8').replace('\r\n', '\n').split('\n')


def depths(lines):
    """Brace and preprocessor depth before each line."""
    out = []
    brace = pp = 0
    in_comment = False
    for line in lines:
        out.append((brace, pp))
        m = re.match(r'^\s*#\s*(if|ifdef|ifndef|endif)\b', line)
        if m and not in_comment:
            pp += -1 if m.group(1) == 'endif' else 1
            continue
        code = line
        if in_comment:
            if '*/' not in code:
                continue
            code = code.split('*/', 1)[1]
            in_comment = False
        code = re.sub(r'/\*.*?\*/', '', code)
        if '/*' in code:
            code = code.split('/*', 1)[0]
            in_comment = True
        code = re.sub(r'"(\\.|[^"\\])*"|\'(\\.|[^\'\\])*\'|//.*$', '', code)
        brace += code.count('{') - code.count('}')
    out.append((brace, pp))
    return out


D = depths(L)
cuts = []
for s in plan['slices']:
    rx = re.compile(s['anchor'])
    hits = [i for i, line in enumerate(L) if rx.search(line)]
    if 'near' in s:  # a repeated anchor: the match closest to `near`, within 50 lines
        hits = sorted((i for i in hits if abs(i + 1 - s['near']) <= 50), key=lambda i: abs(i + 1 - s['near']))[:1]
    if len(hits) != 1:
        raise SystemExit(f"anchor {s['anchor']!r}: {len(hits)} matches")
    i = hits[0]
    # move up over an attached comment block
    j = i
    while j > 0 and L[j - 1].strip() and (L[j - 1].lstrip().startswith(('/*', '*', '//')) or L[j - 1].rstrip().endswith('*/')):
        j -= 1
    if D[j] != (0, 0):
        raise SystemExit(f"cut before line {j + 1} ({s['name']}) is at depth {D[j]}")
    cuts.append((j, s))
if [c[0] for c in cuts] != sorted(c[0] for c in cuts):
    raise SystemExit('anchors out of order')
bounds = [c[0] for c in cuts] + [len(L)]
pieces = []
for k, (start, s) in enumerate(cuts):
    body = L[start:bounds[k + 1]]
    while body and not body[-1].strip():
        body.pop()
    header = ['/* ' + s['header'][0]] + [' * ' + h for h in s['header'][1:]]
    header[-1] += ' */'
    pieces.append((s['name'], header + [''] + body))
head = L[:cuts[0][0]]
while head and not head[-1].strip():
    head.pop()
print(f"{src}: {len(L)} lines -> {len(head)} kept")
for name, lines in pieces:
    print(f"  {name}: {len(lines)} lines")
if check:
    sys.exit(0)


def write(path, lines):
    t = '\n'.join(lines) + '\n'
    if crlf:
        t = t.replace('\n', '\r\n')
    Path(path).write_bytes(t.encode('utf-8'))


if parent.resolve() == src.resolve():
    # A translation unit of its own: it includes its slices last, in order.
    write(src, head + [''] + [f'#include "{name}"' for name, _ in pieces])
    for name, lines in pieces:
        write(src.parent / name, lines)
    print(f"{src}: {len(pieces)} includes added at its end")
    sys.exit(0)
write(src, head)
for name, lines in pieces:
    write(src.parent / name, lines)
# parent: include the new slices right after the original
p_raw = parent.read_bytes()
p_crlf = b'\r\n' in p_raw
P = p_raw.decode('utf-8').replace('\r\n', '\n').split('\n')
inc = [i for i, line in enumerate(P) if re.match(rf'^\s*#\s*include\s+"{re.escape(src.name)}"\s*$', line)]
if len(inc) != 1:
    raise SystemExit(f'{parent}: {len(inc)} includes of {src.name}')
k = inc[0]
P[k + 1:k + 1] = [f'#include "{name}"' for name, _ in pieces]
t = '\n'.join(P)
if p_crlf:
    t = t.replace('\n', '\r\n')
parent.write_bytes(t.encode('utf-8'))
print(f"{parent}: {len(pieces)} includes added after line {k + 1}")
